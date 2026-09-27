#include "mouse_toolbar.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cata_imgui.h"
#include "game.h"
#include "imgui/imgui.h"
#include "options.h"
#include "translations.h"
#include "ui_manager.h"

#if defined(TILES)

namespace
{

bool in_default_mode_wait = false;

class mouse_toolbar_window : public cataimgui::window
{
    public:
        mouse_toolbar_window() : cataimgui::window( "MOUSE_TOOLBAR",
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
                    ImGuiWindowFlags_NoNav ) {
            force_to_back = true;
        }

        std::optional<action_id> take_pending() {
            std::optional<action_id> out = pending;
            pending.reset();
            return out;
        }

        bool has_pending() const {
            return pending.has_value();
        }

    protected:
        cataimgui::bounds get_bounds() override {
            // Bottom-center strip: keep the map center clear on Deck/720p.
            const ImVec2 display = ImGui::GetMainViewport()->Size;
            const float approx_w = 720.f;
            const float x = ( display.x - approx_w ) * 0.5f;
            const float y = display.y - 52.f;
            return { x < 8.f ? 8.f : x, y < 8.f ? 8.f : y, -1.f, -1.f };
        }

        void draw_controls() override {
            hide_ui = !should_draw();
            hide_if_hidden();
            if( hide_ui ) {
                return;
            }

            static const std::vector<std::pair<action_id, translation>> buttons = {
                { ACTION_INVENTORY, to_translation( "Inv" ) },
                { ACTION_CRAFT, to_translation( "Craft" ) },
                { ACTION_CONSTRUCT, to_translation( "Build" ) },
                { ACTION_MAP, to_translation( "Map" ) },
                { ACTION_PL_INFO, to_translation( "Char" ) },
                { ACTION_WAIT, to_translation( "Wait" ) },
                { ACTION_MESSAGES, to_translation( "Log" ) },
                { ACTION_ZONES, to_translation( "Zones" ) },
            };

            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 10.f, 6.f ) );
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 6.f, 4.f ) );
            bool first = true;
            for( const auto &btn : buttons ) {
                if( !first ) {
                    ImGui::SameLine();
                }
                first = false;
                // Own the label string — ImGui may keep the pointer until end of frame.
                const std::string label = btn.second.translated();
                if( ImGui::Button( label.c_str() ) ) {
                    pending = btn.first;
                }
            }

            // Compact auto-action toggles (upstream AUTO_PICKUP / AUTO_FORAGING).
            if( get_option<bool>( "MOUSE_TOOLBAR_AUTO_TOGGLES" ) ) {
                const bool pickup_on = get_option<bool>( "AUTO_PICKUP" );
                const bool forage_on = get_option<bool>( "AUTO_FEATURES" ) &&
                                       get_option<std::string>( "AUTO_FORAGING" ) != "off";

                auto draw_toggle = [&]( const char *id, const std::string &label, bool active,
                action_id act ) {
                    if( !first ) {
                        ImGui::SameLine();
                    }
                    first = false;
                    if( active ) {
                        ImGui::PushStyleColor( ImGuiCol_Button,
                                               ImVec4( 0.20f, 0.45f, 0.25f, 1.f ) );
                        ImGui::PushStyleColor( ImGuiCol_ButtonHovered,
                                               ImVec4( 0.25f, 0.55f, 0.30f, 1.f ) );
                        ImGui::PushStyleColor( ImGuiCol_ButtonActive,
                                               ImVec4( 0.15f, 0.35f, 0.20f, 1.f ) );
                    }
                    ImGui::PushID( id );
                    if( ImGui::Button( label.c_str() ) ) {
                        pending = act;
                    }
                    ImGui::PopID();
                    if( active ) {
                        ImGui::PopStyleColor( 3 );
                    }
                };

                // Stable owned labels (avoid temporary .c_str() lifetime issues).
                const std::string pick_label = pickup_on ? _( "Pick●" ) : _( "Pick" );
                const std::string forage_label = forage_on ? _( "Forage●" ) : _( "Forage" );
                draw_toggle( "tb_pick", pick_label, pickup_on, ACTION_TOGGLE_AUTO_PICKUP );
                draw_toggle( "tb_forage", forage_label, forage_on, ACTION_TOGGLE_AUTO_FORAGING );
            }
            ImGui::PopStyleVar( 2 );
        }

    private:
        std::optional<action_id> pending;

        static bool should_draw() {
            if( !get_option<bool>( "MOUSE_TOOLBAR" ) ) {
                return false;
            }
            if( !g || g->uquit != QUIT_NO ) {
                return false;
            }
            // Only while get_player_input is waiting (DEFAULTMODE). Other menus
            // push their own contexts; the flag is cleared when that wait ends.
            return in_default_mode_wait;
        }
};

std::shared_ptr<mouse_toolbar_window> g_toolbar;

} // namespace

namespace mouse_toolbar
{

void ensure_shown()
{
    if( !get_option<bool>( "MOUSE_TOOLBAR" ) ) {
        hide();
        return;
    }
    if( !g_toolbar ) {
        g_toolbar = std::make_shared<mouse_toolbar_window>();
    }
}

void hide()
{
    in_default_mode_wait = false;
    g_toolbar.reset();
}

void set_default_mode_wait( bool waiting )
{
    in_default_mode_wait = waiting;
    if( waiting ) {
        ensure_shown();
    }
}

std::optional<action_id> take_pending_action()
{
    ensure_shown();
    if( !g_toolbar ) {
        return std::nullopt;
    }
    return g_toolbar->take_pending();
}

bool has_pending_action()
{
    return g_toolbar && g_toolbar->has_pending();
}

} // namespace mouse_toolbar

#else // !TILES

namespace mouse_toolbar
{

void ensure_shown() {}
void hide() {}
void set_default_mode_wait( bool ) {}
std::optional<action_id> take_pending_action()
{
    return std::nullopt;
}

bool has_pending_action()
{
    return false;
}

} // namespace mouse_toolbar

#endif // TILES
