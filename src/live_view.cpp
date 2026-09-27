#include "live_view.h"

#include <algorithm>
#include <memory>
#include <string>

#include "cached_options.h"
#include "coordinates.h"
#include "game.h"
#include "map.h"
#include "options.h"
#include "panels.h"
#include "translations.h"

#if defined(TILES)

#include "cata_imgui.h"
#include "imgui/imgui.h"
#include "ui_hybrid_chrome.h"

namespace
{

class live_view_window : public cataimgui::window
{
    public:
        live_view_window()
            : cataimgui::window( "MOUSE_VIEW_HYBRID",
                                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs |
                                 ImGuiWindowFlags_NoScrollbar ) {
            force_to_back = true;
        }

        void set_tile( const tripoint &p ) {
            mouse_position = p;
            mark_resized();
        }

    protected:
        void draw() override {
            // Hybrid chrome before Begin so WindowBg / borders match Equipment + toolbar.
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
            // Cap height like the old curses live_view (half screen when minimap on).
            const float max_h = pixel_minimap_option ? display.y * 0.50f : display.y * 0.72f;
            const float h = std::max( 96.f, max_h );
            return { x, 0.f, w, h };
        }

        void draw_controls() override {
            hide_ui = false;
            hide_if_hidden();

            ImGui::TextColored( ui_hybrid_chrome::palette::accent(), "%s", _( "Mouse view" ) );
            ImGui::PushStyleColor( ImGuiCol_Separator, ui_hybrid_chrome::palette::separator() );
            ImGui::Separator();
            ImGui::PopStyleColor();

            map &here = get_map();
            const visibility_variables &cache = here.get_visibility_variables_cache();
            const float body_h = std::max( 40.f, ImGui::GetContentRegionAvail().y );
            ImGui::BeginChild( "mouse_view_body", ImVec2( 0.f, body_h ), ImGuiChildFlags_Borders,
                               ImGuiWindowFlags_None );
            g->draw_tile_info_imgui( tripoint_bub_ms( mouse_position ), cache );
            ImGui::EndChild();
        }

    private:
        tripoint mouse_position;
};

} // namespace

live_view::live_view() = default;
live_view::~live_view() = default;

void live_view::init()
{
    hide();
}

void live_view::hide()
{
    imgui_win.reset();
}

void live_view::show( const tripoint &p )
{
    mouse_position = p;
    if( !imgui_win ) {
        imgui_win = std::make_unique<live_view_window>();
    }
    static_cast<live_view_window *>( imgui_win.get() )->set_tile( p );
}

bool live_view::is_enabled()
{
    return imgui_win != nullptr;
}

#else // !TILES

#include "color.h"
#include "cursesdef.h"
#include "output.h"
#include "ui_manager.h"

namespace
{

constexpr int START_LINE = 1;
constexpr int MIN_BOX_HEIGHT = 3;

} //namespace

live_view::live_view() = default;
live_view::~live_view() = default;

void live_view::init()
{
    hide();
}

void live_view::hide()
{
    ui = nullptr;
}

void live_view::show( const tripoint &p )
{
    map &here = get_map();

    mouse_position = p;
    if( !ui ) {
        ui = std::make_unique<ui_adaptor>();
        ui->on_screen_resize( [this, &here]( ui_adaptor & ui ) {
            panel_manager &mgr = panel_manager::get_manager();
            const bool sidebar_right = get_option<std::string>( "SIDEBAR_POSITION" ) == "right";
            const int width = sidebar_right ? mgr.get_width_right() : mgr.get_width_left();

            const int max_height = pixel_minimap_option ? TERMY / 2 : TERMY;
            const int line_limit = max_height - 2;
            const visibility_variables &cache = here.get_visibility_variables_cache();
            int line_out = START_LINE;
            win = catacurses::newwin( 1, width, point::zero );
            g->pre_print_all_tile_info( tripoint_bub_ms( mouse_position ), win, line_out, line_limit, cache );
            const int live_view_box_height = std::min( max_height, std::max( line_out + 2, MIN_BOX_HEIGHT ) );

            win = catacurses::newwin( live_view_box_height, width,
                                      point( sidebar_right ? TERMX - width : 0, 0 ) );
            ui.position_from_window( win );
        } );
        ui->on_redraw( [this, &here]( const ui_adaptor & ) {
            werase( win );
            const visibility_variables &cache = here.get_visibility_variables_cache();
            int line_out = START_LINE;
            g->pre_print_all_tile_info( tripoint_bub_ms( mouse_position ), win, line_out, getmaxy( win ) - 2,
                                        cache );
            draw_border( win );
            center_print( win, 0, c_white, _( "< <color_green>Mouse view</color> >" ) );
            wnoutrefresh( win );
        } );
    }
    ui->mark_resize();
}

bool live_view::is_enabled()
{
    return ui != nullptr;
}

#endif // TILES
