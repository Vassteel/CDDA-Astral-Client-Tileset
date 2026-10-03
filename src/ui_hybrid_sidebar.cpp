#include "tactical_combat.h"
#include "ui_hybrid_sidebar.h"

#if defined(TILES)

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "avatar_action.h"
#include "mission.h"
#include "action.h"
#include "cached_options.h"
#include "mouse_toolbar.h"
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
#include "magic.h"
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
#include "ui_hybrid_widgets.h"
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

    ImGui::BeginGroup();
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored( sidebar_label_col(), "%-6s", label );
    ImGui::SameLine();

    const float value_w = ImGui::CalcTextSize( value_buf ).x +
                          ImGui::GetStyle().ItemSpacing.x;
    const float bar_w = std::max( 24.f, ImGui::GetContentRegionAvail().x - value_w );
    // Shared meter: semantic health bands, value drawn beside the bar on
    // charcoal for contrast (text overlay stays empty on short bars).
    ui_hybrid_widgets::meter( label, frac, ui_hybrid_widgets::meter_kind::health, std::string(),
                              bar_w / ui_hybrid_chrome::theme::scale(), 14.f );
    ImGui::SameLine( 0.f, ImGui::GetStyle().ItemSpacing.x );
    ImGui::TextColored( ui_hybrid_chrome::palette::text(), "%s", value_buf );
    ImGui::EndGroup();
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s", _( "Click for health and treatment details." ) );
    }
    if( ImGui::IsItemClicked() ) {
        mouse_toolbar::queue_action( ACTION_MEDICAL );
    }
}

/**
 * Mana row for the Craft: only shown once the character knows a spell. Same
 * layout as the HP rows (label, bar, value beside it); click opens the spell menu.
 */
void mana_meter_row( Character &u )
{
    if( !u.magic->knows_spell() ) {
        return;
    }
    const int cur = u.magic->available_mana();
    const int mx = std::max( 1, u.magic->max_mana( u ) );
    const float frac = std::clamp( static_cast<float>( cur ) / static_cast<float>( mx ), 0.f, 1.f );
    char value_buf[32];
    std::snprintf( value_buf, sizeof( value_buf ), "%d/%d", cur, mx );
    const std::string label = _( "Mana" );

    ImGui::BeginGroup();
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored( sidebar_label_col(), "%-6s", label.c_str() );
    ImGui::SameLine();
    const float value_w = ImGui::CalcTextSize( value_buf ).x +
                          ImGui::GetStyle().ItemSpacing.x;
    const float bar_w = std::max( 24.f, ImGui::GetContentRegionAvail().x - value_w );
    ui_hybrid_widgets::meter( "astral_mana", frac, ui_hybrid_widgets::meter_kind::info, std::string(),
                              bar_w / ui_hybrid_chrome::theme::scale(), 14.f );
    ImGui::SameLine( 0.f, ImGui::GetStyle().ItemSpacing.x );
    ImGui::TextColored( ui_hybrid_chrome::palette::text(), "%s", value_buf );
    ImGui::EndGroup();
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s", _( "Click to cast a spell." ) );
    }
    if( ImGui::IsItemClicked() ) {
        mouse_toolbar::queue_action( ACTION_CAST_SPELL );
    }
}

class hybrid_sidebar_window : public cataimgui::window
{
    public:
        hybrid_sidebar_window()
            : cataimgui::window( "HYBRID_SIDEBAR",
                                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus ) {
            force_to_back = true;
        }

    protected:
        void draw() override {
            // Loading and returning to the menu can temporarily clear the
            // avatar. Never enter ImGui or sort body parts in that state.
            if( !g || get_avatar().get_body().empty() ) {
                return;
            }
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
            []( const window_panel & panel ) {
                return panel.get_id() == "Log" && panel.toggle && panel.render();
            } );
            namespace w = ui_hybrid_widgets;
            namespace theme = ui_hybrid_chrome::theme;
            const ui_hybrid_chrome::theme::tokens &tk = theme::get();
            const float s = theme::scale();
            // Quiet edge on the map side: persistent HUD panels stay thinner
            // and quieter than modal windows.
            {
                ImDrawList *bg = ImGui::GetWindowDrawList();
                const ImVec2 wmin = ImGui::GetWindowPos();
                const ImVec2 wmax( wmin.x + ImGui::GetWindowSize().x, wmin.y + ImGui::GetWindowSize().y );
                const bool right = get_option<std::string>( "SIDEBAR_POSITION" ) == "right";
                const float x = right ? wmin.x : wmax.x - 2.f;
                bg->AddRectFilled( ImVec2( x, wmin.y ), ImVec2( x + 2.f, wmax.y ), tk.edge_quiet );
            }
            // Keep the section sizes independent of the previous frame's
            // content. Scrollbar changes must not resize the status/log split.
            // Keep a stopped safe mode visible even when status rows are scrolled.
            if( g->safe_mode == SAFE_MODE_STOP ) {
                // Critical warning band: danger edge, icon and text, never only color.
                const ImVec2 p = ImGui::GetCursorScreenPos();
                const float band_h = tk.button * s;
                ImDrawList *dl = ImGui::GetWindowDrawList();
                const float wdt = ImGui::GetContentRegionAvail().x;
                dl->AddRectFilled( p, ImVec2( p.x + wdt, p.y + band_h ), IM_COL32( 0x2A, 0x1A, 0x17, 255 ),
                                   tk.radius_control * s );
                dl->AddRect( p, ImVec2( p.x + wdt, p.y + band_h ), tk.danger, tk.radius_control * s );
                const float isz = tk.icon * 0.7f * s;
                w::draw_icon_at( dl, "warning", ImVec2( p.x + tk.sm * s, p.y + ( band_h - isz ) * 0.5f ), isz,
                                 tk.danger );
                ImGui::SetCursorScreenPos( ImVec2( p.x + tk.sm * s * 2.f + isz, p.y + ( band_h - ImGui::GetFontSize() ) * 0.5f ) );
                ImGui::TextColored( ui_hybrid_chrome::palette::danger(), "%s", _( "Safe mode: danger detected" ) );
                ImGui::SameLine();
                const float btn_w = 84.f;
                ImGui::SetCursorScreenPos( ImVec2( p.x + wdt - btn_w * s - tk.xs * s, p.y + ( band_h - 32.f * s ) * 0.5f ) );
                if( w::action_button( _( "Ignore" ), w::button_kind::tertiary, ImVec2( btn_w, 32.f ) ) ) {
                    mouse_toolbar::queue_action( ACTION_IGNORE_ENEMY );
                }
                ImGui::SetCursorScreenPos( ImVec2( p.x, p.y + band_h + tk.xs * s ) );
            }
            if( mission *objective = u.get_active_mission() ) {
                w::row_state st;
                const w::row_result r = w::selectable_row( "objective", objective->name(), std::string(), "star",
                                        nullptr, st, tk.row_compact );
                if( r.hovered ) {
                    w::tooltip( objective->name() + "\n" + _( "Click to open Missions." ) );
                }
                if( r.clicked ) {
                    mouse_toolbar::queue_action( ACTION_MISSIONS );
                }
            }
            // Automation state, one compact line each, icon marks automatic behaviour.
            const auto auto_line = [&]( const std::string & text ) {
                w::icon( "auto", 14.f, tk.text_muted );
                ImGui::SameLine( 0.f, tk.xs * s );
                ImGui::PushTextWrapPos( ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x );
                ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s", text.c_str() );
                ImGui::PopTextWrapPos();
            };
            if( get_option<bool>( "AUTO_PICKUP" ) ) {
                const char *state = u.is_mounted() ? _( "Paused while mounted" ) :
                                    u.is_hauling() ? _( "Paused while hauling" ) :
                                    get_option<bool>( "AUTO_PICKUP_SAFEMODE" ) &&
                                    u.get_mon_visible().has_dangerous_creature_in_proximity ?
                                    _( "Paused near danger" ) : _( "On movement; pickup rules apply" );
                auto_line( string_format( _( "Pick: %s" ), state ) );
            }
            if( get_option<std::string>( "AUTO_FORAGING" ) != "off" ) {
                const char *state = !get_option<bool>( "AUTO_FEATURES" ) ? _( "Auto features disabled" ) :
                                    u.is_mounted() ? _( "Paused while mounted" ) :
                                    g->mostseen > 0 ? _( "Paused while creatures are visible" ) :
                                    _( "On movement; selected plants only" );
                auto_line( string_format( _( "Forage: %s" ), state ) );
            }
            if( get_option<bool>( "TACTICAL_COMBAT" ) ) {
                // Combat group: real dispatcher actions only; stance from the native state.
                w::section_label( _( "Combat" ), "attack" );
                ImGui::PushTextWrapPos( ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x );
                ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s", tactical_combat::stance( u ).c_str() );
                ImGui::PopTextWrapPos();
            }
            if( get_option<bool>( "AUTO_COMBAT" ) ) {
                auto_line( string_format( _( "Combat: %s" ), avatar_action::auto_combat_status() ) );
            }
            if( get_option<bool>( "AUTO_EAT" ) ) {
                auto_line( string_format( _( "Eat: %s" ), avatar_action::auto_eat_status() ) );
            }
            // Character section label with the HUD settings gear at the right.
            {
                const float gear = 30.f;
                const ImVec2 p = ImGui::GetCursorScreenPos();
                const float wdt = ImGui::GetContentRegionAvail().x;
                w::section_label( _( "Character" ), "hp" );
                const ImVec2 after = ImGui::GetCursorScreenPos();
                ImGui::SetCursorScreenPos( ImVec2( p.x + wdt - gear * s, p.y ) );
                if( w::icon_button( "##hud_settings", "gear", gear, false, _( "HUD settings" ) ) ) {
                    ImGui::OpenPopup( "hud_settings" );
                }
                ImGui::SetCursorScreenPos( after );
            }
            if( ImGui::BeginPopup( "hud_settings" ) ) {
                bool overview = get_option<bool>( "HYBRID_HP_OVERVIEW" );
                if( ImGui::Checkbox( _( "Health overview" ), &overview ) ) {
                    get_options().get_option( "HYBRID_HP_OVERVIEW" ).setValue( overview ? "true" : "false" );
                    get_options().save();
                }
                int percent = get_option<int>( "HYBRID_STATUS_PERCENT" );
                if( ImGui::SliderInt( _( "Status height (%)" ), &percent, 20, 90 ) ) {
                    get_options().get_option( "HYBRID_STATUS_PERCENT" ).setValue( std::to_string( percent ) );
                }
                if( ImGui::IsItemDeactivatedAfterEdit() ) {
                    get_options().save();
                }
                if( ImGui::MenuItem( _( "Sidebar panels…" ) ) ) {
                    mouse_toolbar::queue_action( ACTION_PANEL_MGMT );
                }
                ImGui::EndPopup();
            }
            // The status section takes only the height its rows need, up to the
            // configured share of the sidebar; the message log gets the rest.
            // No scrollbars in the HUD: a status section taller than its cap
            // still scrolls with the wheel, the log follows its newest line.
            const float available = ImGui::GetContentRegionAvail().y;
            const float status_cap = show_log ? std::max( 1.f,
                                     std::floor( available * get_option<int>( "HYBRID_STATUS_PERCENT" ) / 100.f ) ) :
                                     available;
            ImGui::SetNextWindowSizeConstraints( ImVec2( 0.f, 0.f ), ImVec2( FLT_MAX, status_cap ) );
            ImGui::BeginChild( "hybrid_status", ImVec2( 0.f, 0.f ),
                               ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar );
            if( get_option<bool>( "HYBRID_HP_OVERVIEW" ) ) {
                for( const bodypart_id &bp :
                     u.get_all_body_parts( get_body_part_flags::only_main |
                                           get_body_part_flags::sorted ) ) {
                    const std::string label = body_part_name_as_heading( bp, 1 );
                    hp_meter_row( u, bp, label.c_str() );
                }
            }
            mana_meter_row( u );
            if( w::action_button( _( "Health" ), w::button_kind::tertiary, ImVec2( 0, 30.f ), true, nullptr, "hp" ) ) {
                mouse_toolbar::queue_action( ACTION_MEDICAL );
            }
            ImGui::SameLine();
            if( w::action_button( _( "Mood" ), w::button_kind::tertiary, ImVec2( 0, 30.f ), true, nullptr, "info" ) ) {
                mouse_toolbar::queue_action( ACTION_MORALE );
            }
            ImGui::SameLine();
            if( w::action_button( _( "Gear" ), w::button_kind::tertiary, ImVec2( 0, 30.f ), true, nullptr,
                                  "tab_equipment" ) ) {
                mouse_toolbar::queue_action( ACTION_INVENTORY );
            }
            w::section_label( _( "Status" ) );
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
                    // Keep one readable font size as values change. Columns
                    // follow the section width, which no scrollbar changes.
                    ImGui::BeginGroup();
                    cataimgui::draw_colored_text( text );
                    ImGui::EndGroup();
                    if( ImGui::IsItemHovered() ) {
                        ImGui::SetTooltip( "%s", _( "Click for character details. Right-click for actions." ) );
                    }
                    if( ImGui::IsItemClicked() ) {
                        mouse_toolbar::queue_action( ACTION_PL_INFO );
                    } else if( ImGui::IsItemClicked( ImGuiMouseButton_Right ) ) {
                        mouse_toolbar::queue_action( ACTION_ACTIONMENU );
                    }
                }
            }
            ImGui::EndChild();
            if( show_log ) {
                draw_message_log();
            }
        }

    private:
        void draw_message_log() {
            ui_hybrid_widgets::section_label( _( "Messages" ), "list" );
            // Remaining vertical space for the scrollable log
            const float remain = std::max( 1.f, ImGui::GetContentRegionAvail().y );
            ui_hybrid_widgets::panel_begin( "hybrid_msg_log", ImVec2( 0.f, remain ), true,
                                            ImGuiWindowFlags_NoScrollbar );
            const bool follow = log_from_top ? ImGui::GetScrollY() <= 4.f :
                                ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.f;
            const auto msgs = Messages::sidebar_messages( 200 );
            for( const auto &entry : msgs ) {
                // entry: { time_of_day, message_text }
                ImGui::TextColored( sidebar_label_col(), "%s", entry.time.c_str() );
                ImGui::SameLine();
                cataimgui::draw_colored_text( entry.text, entry.color, ImGui::GetContentRegionAvail().x );
            }
            // Keep scrolled to bottom for newest messages
            if( follow ) {
                if( log_from_top ) {
                    ImGui::SetScrollY( 0.f );
                } else {
                    ImGui::SetScrollHereY( 1.f );
                }
            }
            ui_hybrid_widgets::panel_end();
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

int width_cells( int layout_cells )
{
    if( layout_cells <= 0 ) {
        return 0;
    }
    namespace theme = ui_hybrid_chrome::theme;
    const float s = theme::scale();
    // Window padding on both sides plus the quiet edge on the map side.
    const float chrome_px = 2.f * theme::get().md * s + 4.f * s;
    const int chrome = fontwidth > 0 ? static_cast<int>( std::ceil( chrome_px / fontwidth ) ) : 2;
    const int percent = get_options().has_option( "HYBRID_SIDEBAR_WIDTH" ) ?
                        get_option<int>( "HYBRID_SIDEBAR_WIDTH" ) : 100;
    return std::max( 1, ( layout_cells + chrome ) * percent / 100 );
}

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
