#include "ui_hybrid_sidebar.h"

#if defined(TILES)

#include <algorithm>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_imgui.h"
#include "character.h"
#include "creature.h"
#include "character_martial_arts.h"
#include "color.h"
#include "coordinates.h"
#include "display.h"
#include "game.h"
#include "map.h"
#include "messages.h"
#include "options.h"
#include "output.h"
#include "panels.h"
#include "sdltiles.h"
#include "widget.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_hybrid_chrome.h"
#include "units.h"
#include "weather.h"

#include "imgui/imgui.h"

namespace
{

/**
 * Sidebar stat/body-part labels on charcoal — brighter than palette::text_muted
 * so thin monospace stays legible without cozy/Elin chrome wash.
 */
ImVec4 sidebar_label_col()
{
    return ImVec4( 0.82f, 0.74f, 0.56f, 1.f );
}

/**
 * HP meter row: short label + colored bar + value on charcoal (not overlaid
 * on the fill — ImGui ProgressBar clamps overlay onto the fill when full,
 * which washed out white 97/97 on green).
 */
void hp_meter_row( Character &u, const bodypart_id &bp, const char *label )
{
    if( !u.has_part( bp ) ) {
        return;
    }
    const int cur = u.get_part_hp_cur( bp );
    const int mx = std::max( 1, u.get_part_hp_max( bp ) );
    const float frac = static_cast<float>( cur ) / static_cast<float>( mx );
    char value_buf[32];
    std::snprintf( value_buf, sizeof( value_buf ), "%d/%d", cur, mx );

    ImGui::TextColored( sidebar_label_col(), "%-6s", label );
    ImGui::SameLine();

    const float value_w = ImGui::CalcTextSize( value_buf ).x +
                          ImGui::GetStyle().ItemSpacing.x;
    const float bar_w = std::max( 24.f, ImGui::GetContentRegionAvail().x - value_w );

    // Color the bar by health band
    ImVec4 fill = ui_hybrid_chrome::palette::accent();
    if( frac < 0.25f ) {
        fill = ImVec4( 0.75f, 0.18f, 0.14f, 1.f );
    } else if( frac < 0.50f ) {
        fill = ImVec4( 0.85f, 0.45f, 0.15f, 1.f );
    } else if( frac < 0.75f ) {
        fill = ImVec4( 0.80f, 0.70f, 0.20f, 1.f );
    } else {
        fill = ImVec4( 0.30f, 0.65f, 0.28f, 1.f );
    }
    ImGui::PushStyleColor( ImGuiCol_PlotHistogram, fill );
    ImGui::PushStyleColor( ImGuiCol_FrameBg, ui_hybrid_chrome::palette::button() );
    // Empty overlay: value is drawn beside the bar on charcoal for contrast.
    ImGui::ProgressBar( frac, ImVec2( bar_w, 0.f ), "" );
    ImGui::PopStyleColor( 2 );
    ImGui::SameLine( 0.f, ImGui::GetStyle().ItemSpacing.x );
    ImGui::TextColored( ui_hybrid_chrome::palette::text(), "%s", value_buf );
}

class hybrid_sidebar_window : public cataimgui::window
{
    public:
        hybrid_sidebar_window()
            : cataimgui::window( "HYBRID_SIDEBAR",
                                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus ) {
            force_to_back = true;
        }

    protected:
        void draw() override {
            ui_hybrid_chrome::push();
            cataimgui::window::draw();
            ui_hybrid_chrome::pop();
        }

        cataimgui::bounds get_bounds() override {
            panel_manager &mgr = panel_manager::get_manager();
            const bool sidebar_right = get_option<std::string>( "SIDEBAR_POSITION" ) == "right";
            const int width_cells = sidebar_right ? mgr.get_width_right() : mgr.get_width_left();
            const float w = static_cast<float>( std::max<size_t>( 1, str_width_to_pixels( width_cells ) ) );
            const ImVec2 display = ImGui::GetMainViewport()->Size;
            const float x = sidebar_right ? std::max( 0.f, display.x - w ) : 0.f;
            return { x, 0.f, w, std::max( 1.f, display.y -
                        ui_hybrid_sidebar::minimap_height() * fontheight ) };
        }

        void draw_controls() override {
            hide_ui = false;
            hide_if_hidden();

            avatar &u = get_avatar();

            const auto &panels = panel_manager::get_manager().get_current_layout().panels();
            const bool show_log = std::any_of( panels.begin(), panels.end(),
            []( const window_panel &panel ) {
                return panel.get_id() == "Log" && panel.toggle && panel.render();
            } );
            // Keep the log and map reachable even when a small screen cannot
            // show every enabled status panel at once.
            const float available = ImGui::GetContentRegionAvail().y;
            const float stats_height = show_log ? std::min( last_stats_height,
                                       std::max( 1.f, available * 0.65f ) ) : available;
            ImGui::BeginChild( "hybrid_status", ImVec2( 0.f, stats_height ),
                               ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar );
            const float start_y = ImGui::GetCursorPosY();
            ui_hybrid_chrome::section_header( _( "Character" ) );
            for( const bodypart_id &bp :
                 u.get_all_body_parts( get_body_part_flags::only_main |
                                       get_body_part_flags::sorted ) ) {
                const std::string label = body_part_name_as_heading( bp, 1 );
                hp_meter_row( u, bp, label.c_str() );
            }
            ui_hybrid_chrome::section_header( _( "Status" ) );
            const float text_width = ImGui::GetContentRegionAvail().x;
            const int columns = std::max( 1, static_cast<int>( text_width /
                                         ImGui::CalcTextSize( "X" ).x ) );
            for( const window_panel &panel : panels ) {
                if( !panel.toggle || !panel.render() || !panel.get_widget().is_valid() ) {
                    continue;
                }
                // Use the same widget trees, conditional clauses, colors and
                // label widths as vanilla, including mod-provided panels.
                widget row = panel.get_widget().obj();
                const std::string text = row.layout( u, columns, row._label_width,
                                         row.has_flag( "W_NO_PADDING" ) );
                if( !text.empty() ) {
                    const float natural_width = ImGui::CalcTextSize( remove_color_tags( text ).c_str() ).x;
                    // Fixed-width vanilla rows need a little room for Hybrid
                    // padding and the scrollbar. Preserve their columns; very
                    // wide mod panels can scroll horizontally instead.
                    const float font_scale = std::clamp( text_width / std::max( 1.f, natural_width ),
                                                       0.85f, 1.f );
                    ImGui::SetWindowFontScale( font_scale );
                    cataimgui::draw_colored_text( text );
                    ImGui::SetWindowFontScale( 1.f );
                }
            }
            last_stats_height = ImGui::GetCursorPosY() - start_y +
                                ImGui::GetStyle().WindowPadding.y * 2.f;
            ImGui::EndChild();
            if( show_log ) {
                draw_message_log();
            }
        }

    private:
        float last_stats_height = 1000.f;

        void draw_message_log() {
            ui_hybrid_chrome::section_header( _( "Messages" ) );
            // Remaining vertical space for the scrollable log
            const float remain = std::max( 1.f, ImGui::GetContentRegionAvail().y );
            ImGui::BeginChild( "hybrid_msg_log", ImVec2( 0.f, remain ),
                               ImGuiChildFlags_Borders, ImGuiWindowFlags_None );
            const auto msgs = Messages::recent_messages( 40 );
            for( const auto &entry : msgs ) {
                // entry: { time_of_day, message_text }
                ImGui::TextColored( sidebar_label_col(), "%s", entry.first.c_str() );
                ImGui::SameLine();
                cataimgui::draw_colored_text( entry.second, ImGui::GetContentRegionAvail().x );
            }
            // Keep scrolled to bottom for newest messages
            if( ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.f ) {
                ImGui::SetScrollHereY( 1.f );
            }
            ImGui::EndChild();
        }
};

/** Separate Mouse view panel — sits beside the Hybrid sidebar's top edge
 *  (just left when sidebar is right; just right when sidebar is left), never
 *  stacked over sidebar content. */
class hybrid_mouse_view_window : public cataimgui::window
{
    public:
        hybrid_mouse_view_window()
            : cataimgui::window( "HYBRID_MOUSE_VIEW",
                                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus ) {
            force_to_back = true;
        }

        void set_mouse_tile( const tripoint &p ) {
            if( mouse_tile && *mouse_tile == p ) {
                return;
            }
            mouse_tile = p;
            mark_resized();
        }
        void clear_mouse_tile() {
            if( !mouse_tile ) {
                return;
            }
            mouse_tile.reset();
            mark_resized();
        }
        bool has_mouse_tile() const {
            return mouse_tile.has_value();
        }

    protected:
        void draw() override {
            // Skip Begin entirely when empty. hide_if_hidden() runs AFTER Begin
            // has already queued WindowBg; HiddenFramesCanSkipItems only lasts
            // one frame, so the empty AlwaysAutoResize panel flashed charcoal
            // every other redraw (HUD blink on each step / TIMEOUT tick).
            if( !mouse_tile ) {
                return;
            }
            // Lock column width to the sidebar's pixel width; cap height so a
            // tall tile dump scrolls instead of covering the map.
            const float w = sidebar_pixel_width();
            const float max_h = ImGui::GetMainViewport()->Size.y * 0.55f;
            ImGui::SetNextWindowSizeConstraints( ImVec2( w, 0.f ), ImVec2( w, max_h ) );
            ui_hybrid_chrome::push();
            cataimgui::window::draw();
            ui_hybrid_chrome::pop();
        }

        cataimgui::bounds get_bounds() override {
            const float w = sidebar_pixel_width();
            const ImVec2 display = ImGui::GetMainViewport()->Size;
            const bool sidebar_right =
                get_option<std::string>( "SIDEBAR_POSITION" ) == "right";
            // Sidebar rect: right → [display.x - w, display.x]; left → [0, w].
            const float sidebar_x = sidebar_right ? std::max( 0.f, display.x - w ) : 0.f;
            // Abut the sidebar's top corner from the map side (never overlap):
            //   right sidebar → mouse panel ends at sidebar_x
            //   left sidebar  → mouse panel starts at sidebar_x + w
            const float x = sidebar_right ? std::max( 0.f, sidebar_x - w )
                            : sidebar_x + w;
            // AlwaysAutoResize drives height; width locked via SizeConstraints.
            return { x, 0.f, -1.f, -1.f };
        }

        void draw_controls() override {
            // draw() already gated on mouse_tile; keep content-only here.
            hide_ui = false;

            ui_hybrid_chrome::section_header( _( "Mouse view" ) );
            // Window is AlwaysAutoResize + height-capped; scrollbar appears if
            // tile dump exceeds the cap (NoScrollbar not set).
            map &here = get_map();
            const visibility_variables &cache = here.get_visibility_variables_cache();
            g->draw_tile_info_imgui( tripoint_bub_ms( *mouse_tile ), cache );
        }

    private:
        std::optional<tripoint> mouse_tile;

        float sidebar_pixel_width() {
            panel_manager &mgr = panel_manager::get_manager();
            const bool sidebar_right =
                get_option<std::string>( "SIDEBAR_POSITION" ) == "right";
            const int width_cells = sidebar_right ? mgr.get_width_right()
                                    : mgr.get_width_left();
            return static_cast<float>(
                       std::max<size_t>( 1, str_width_to_pixels( width_cells ) ) );
        }
};

std::unique_ptr<hybrid_sidebar_window> g_sidebar;
std::unique_ptr<hybrid_mouse_view_window> g_mouse_view;

} // namespace

namespace ui_hybrid_sidebar
{

int minimap_height()
{
    for( const window_panel &panel : panel_manager::get_manager().get_current_layout().panels() ) {
        if( panel.get_id() == "Map" && panel.toggle && panel.render() ) {
            return std::clamp( panel.get_height(), 0, std::max( 0, TERMY / 3 ) );
        }
    }
    return 0;
}

void ensure()
{
    if( !g_sidebar ) {
        g_sidebar = std::make_unique<hybrid_sidebar_window>();
    }
    // Map toggles, layout changes and resolution changes alter the reserved
    // bottom strip. Refresh geometry without recreating the scroll state.
    static int previous_height = -1;
    static int previous_width = -1;
    static int previous_map_height = -1;
    const int map_height = minimap_height();
    if( previous_height != TERMY || previous_width != TERMX ||
        previous_map_height != map_height ) {
        g_sidebar->mark_resized();
        previous_height = TERMY;
        previous_width = TERMX;
        previous_map_height = map_height;
    }
    if( !g_mouse_view ) {
        g_mouse_view = std::make_unique<hybrid_mouse_view_window>();
    }
}

void set_mouse_tile( const tripoint &p )
{
    ensure();
    g_mouse_view->set_mouse_tile( p );
}

void clear_mouse_tile()
{
    if( g_mouse_view ) {
        g_mouse_view->clear_mouse_tile();
    }
}

bool has_mouse_tile()
{
    return g_mouse_view && g_mouse_view->has_mouse_tile();
}

void hide()
{
    // Destroy Hybrid ImGui windows.  Must run while ImGui/SDL are still
    // alive (see CheckMessages CATA_QUIT path in sdltiles.cpp — g.reset()
    // before endwin).  window::~window already no-ops ImGui calls when
    // GImGui is null; reset order mouse-view then sidebar matches create.
    g_mouse_view.reset();
    g_sidebar.reset();
}

} // namespace ui_hybrid_sidebar

#endif // TILES
