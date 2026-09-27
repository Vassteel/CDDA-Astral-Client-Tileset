#include "mouse_toolbar.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cata_imgui.h"
#include "ui_hybrid_chrome.h"
#include "action.h"
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

        void draw() override {
            // Hybrid chrome before Begin so the strip WindowBg matches.
            ui_hybrid_chrome::push();
            cataimgui::window::draw();
            ui_hybrid_chrome::pop();
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
                const int tb_cols = ui_hybrid_chrome::push_toolbar_button( false );
                if( ImGui::Button( label.c_str() ) ) {
                    pending = btn.first;
                }
                ui_hybrid_chrome::draw_item_bezel( false, ImGui::IsItemHovered(), false );
                ImGui::PopStyleColor( tb_cols );
            }

            // Compact auto-action toggles (upstream AUTO_PICKUP / AUTO_FORAGING).
            // Left-click: toggle / cycle.  Right-click: configure (manager / mode menu).
            if( get_option<bool>( "MOUSE_TOOLBAR_AUTO_TOGGLES" ) ) {
                const bool pickup_on = get_option<bool>( "AUTO_PICKUP" );
                const bool forage_on = get_option<bool>( "AUTO_FEATURES" ) &&
                                       get_option<std::string>( "AUTO_FORAGING" ) != "off";
                const bool combat_on = get_option<bool>( "AUTO_COMBAT" );
                const std::string forage_mode = get_option<std::string>( "AUTO_FORAGING" );

                auto draw_styled_button = [&]( const char *id, const std::string & label,
                bool active ) {
                    if( !first ) {
                        ImGui::SameLine();
                    }
                    first = false;
                    const int n = ui_hybrid_chrome::push_toolbar_button( active );
                    ImGui::PushID( id );
                    ImGui::Button( label.c_str() );
                    const bool left = ImGui::IsItemClicked( ImGuiMouseButton_Left );
                    const bool right = ImGui::IsItemClicked( ImGuiMouseButton_Right );
                    ui_hybrid_chrome::draw_item_bezel( active, ImGui::IsItemHovered(), false );
                    ImGui::PopID();
                    ImGui::PopStyleColor( n );
                    return std::pair<bool, bool> { left, right };
                };

                // Stable owned labels (avoid temporary .c_str() lifetime issues).
                const std::string pick_label = pickup_on ? _( "Pick●" ) : _( "Pick" );
                std::string forage_label = forage_on ? _( "Forage●" ) : _( "Forage" );
                if( forage_on && forage_mode != "bushes" ) {
                    // Hint active mode when not the default bushes setting.
                    forage_label += ":" + forage_mode.substr( 0, 1 );
                }
                const std::string combat_label = combat_on ? _( "Combat●" ) : _( "Combat" );

                const auto pick_clicks = draw_styled_button( "tb_pick", pick_label, pickup_on );
                if( pick_clicks.first ) {
                    pending = ACTION_TOGGLE_AUTO_PICKUP;
                } else if( pick_clicks.second ) {
                    // Open existing Auto Pickup Manager (filters / Global vs Character rules).
                    pending = ACTION_AUTOPICKUP;
                }

                const auto forage_clicks = draw_styled_button( "tb_forage", forage_label,
                                           forage_on );
                if( forage_clicks.first ) {
                    pending = ACTION_TOGGLE_AUTO_FORAGING;
                } else if( forage_clicks.second ) {
                    ImGui::OpenPopup( "tb_forage_modes" );
                }

                if( ImGui::BeginPopup( "tb_forage_modes" ) ) {
                    ImGui::TextUnformatted( _( "Auto forage mode" ) );
                    ImGui::Separator();
                    ImGui::TextWrapped( "%s",
                                        _( "Requires Additional auto features.  "
                                           "Runs on adjacent tiles while walking; "
                                           "paused when monsters are visible." ) );
                    ImGui::Spacing();
                    auto pick_mode = [&]( const char *id, const char *label ) {
                        const bool selected = forage_mode == id;
                        if( ImGui::Selectable( label, selected ) ) {
                            get_options().get_option( "AUTO_FORAGING" ).setValue( id );
                            if( std::string( id ) != "off" &&
                                !get_option<bool>( "AUTO_FEATURES" ) ) {
                                get_options().get_option( "AUTO_FEATURES" ).setValue( "true" );
                            }
                            get_options().save();
                            ImGui::CloseCurrentPopup();
                        }
                        if( selected ) {
                            ImGui::SetItemDefaultFocus();
                        }
                    };
                    pick_mode( "off", _( "Off" ) );
                    pick_mode( "bushes", _( "Bushes" ) );
                    pick_mode( "trees", _( "Trees" ) );
                    pick_mode( "crops", _( "Crops" ) );
                    pick_mode( "all", _( "Everything" ) );
                    ImGui::EndPopup();
                }

                const auto combat_clicks = draw_styled_button( "tb_combat", combat_label,
                                           combat_on );
                if( combat_clicks.first ) {
                    pending = ACTION_TOGGLE_AUTO_COMBAT;
                } else if( combat_clicks.second ) {
                    ImGui::OpenPopup( "tb_combat_help" );
                }

                if( ImGui::BeginPopup( "tb_combat_help" ) ) {
                    ImGui::TextUnformatted( _( "Auto combat" ) );
                    ImGui::Separator();
                    ImGui::TextWrapped( "%s",
                                        _( "When Combat● is on, each of your turns fights "
                                           "automatically if a hostile is in range.\n\n"
                                           "Melee / reach: normal attack (martial style + "
                                           "weapon + worn armor techniques).  Blocks and "
                                           "dodges still use the normal defense path when "
                                           "you are hit.\n\n"
                                           "Ranged: aims/fires your wielded gun or bow "
                                           "without the targeting UI.  Does not switch "
                                           "weapons, throw, or cast spells.\n\n"
                                           "If nothing is fightable, you keep normal "
                                           "control so you can move.  Safe mode still "
                                           "stops actions.  Toggle off to cancel." ) );
                    ImGui::Spacing();
                    if( ImGui::Selectable( _( "One-shot autoattack (Tab)" ) ) ) {
                        pending = ACTION_AUTOATTACK;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
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
