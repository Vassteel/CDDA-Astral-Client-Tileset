#include "mouse_toolbar.h"

#include <algorithm>
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
#include "panels.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_manager.h"

#if defined(TILES)

namespace
{

bool in_default_mode_wait = false;
std::optional<action_id> hud_pending;

const std::vector<std::pair<action_id, translation>> &toolbar_buttons()
{
    static const std::vector<std::pair<action_id, translation>> buttons = {
        { ACTION_INVENTORY, to_translation( "Inv" ) },
        { ACTION_EAT, to_translation( "Consume" ) },
        { ACTION_CRAFT, to_translation( "Craft" ) },
        { ACTION_CONSTRUCT, to_translation( "Build" ) },
        { ACTION_MAP, to_translation( "Map" ) },
        { ACTION_MISSIONS, to_translation( "Missions" ) },
        { ACTION_PL_INFO, to_translation( "Char" ) },
        { ACTION_WAIT, to_translation( "Wait" ) },
        { ACTION_MESSAGES, to_translation( "Log" ) },
        { ACTION_ZONES, to_translation( "Zones" ) },
        { ACTION_OPEN_MOVEMENT, to_translation( "Move" ) },
        { ACTION_TOGGLE_SAFEMODE, to_translation( "Safe" ) },
        { ACTION_ACTIONMENU, to_translation( "More" ) },
    };
    return buttons;
}

class mouse_toolbar_window : public cataimgui::window
{
    public:
        mouse_toolbar_window() : cataimgui::window( "MOUSE_TOOLBAR",
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing |
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
            const ImVec2 display = ImGui::GetMainViewport()->Size;
            const panel_manager &mgr = panel_manager::get_manager();
            const float left = str_width_to_pixels( mgr.get_width_left() );
            const float right = str_width_to_pixels( mgr.get_width_right() );
            const float map_width = std::max( 100.f, display.x - left - right );
            const float padding = ImGui::GetStyle().WindowPadding.x * 2.f;
            const float max_content = std::max( 80.f, map_width - 16.f - padding );
            float row_width = 0.f;
            float widest = 0.f;
            int rows = 1;
            auto measure = [&]( const std::string & label ) {
                const float width = ImGui::CalcTextSize( label.c_str() ).x + 20.f;
                if( row_width > 0.f && row_width + 6.f + width > max_content ) {
                    widest = std::max( widest, row_width );
                    row_width = 0.f;
                    ++rows;
                }
                row_width += ( row_width > 0.f ? 6.f : 0.f ) + width;
            };
            for( const auto &button : toolbar_buttons() ) {
                measure( button.second.translated() );
            }
            if( get_option<bool>( "MOUSE_TOOLBAR_AUTO_TOGGLES" ) ) {
                // Reserve enough room for the active state labels.
                measure( _( "Pick●" ) );
                measure( std::string( _( "Forage●" ) ) + ":x" );
                measure( _( "Combat●" ) );
                measure( _( "Eat●" ) );
            }
            const float width = std::max( widest, row_width ) + padding;
            const float height = rows * ( ImGui::GetFontSize() + 12.f ) +
                                 ( rows - 1 ) * 4.f + ImGui::GetStyle().WindowPadding.y * 2.f;
            return { left + ( map_width - width ) * 0.5f, display.y - height - 8.f,
                     width, height };
        }

        void draw() override {
            // Same Begin-then-hide flash as Hybrid mouse-view: skip Begin when
            // the strip should not show (between DEFAULTMODE waits / menus).
            if( !should_draw() ) {
                return;
            }
            // Hybrid chrome before Begin so the strip WindowBg matches.
            ui_hybrid_chrome::push();
            const cataimgui::bounds next = get_bounds();
            if( next.x != last_bounds.x || next.y != last_bounds.y ||
                next.w != last_bounds.w || next.h != last_bounds.h ) {
                last_bounds = next;
                mark_resized();
            }
            cataimgui::window::draw();
            ui_hybrid_chrome::pop();
        }

        void draw_controls() override {
            hide_ui = false;

            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 10.f, 6.f ) );
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 6.f, 4.f ) );
            bool first = true;
            auto place_button = [&]( const std::string & label ) {
                const float width = ImGui::CalcTextSize( label.c_str() ).x + 20.f;
                if( !first && ImGui::GetItemRectMax().x + 6.f + width <=
                    ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x ) {
                    ImGui::SameLine();
                }
                first = false;
            };
            for( const auto &btn : toolbar_buttons() ) {
                // Own the label string — ImGui may keep the pointer until end of frame.
                const std::string label = btn.second.translated();
                place_button( label );
                const int tb_cols = ui_hybrid_chrome::push_toolbar_button(
                                        btn.first == ACTION_TOGGLE_SAFEMODE && g->safe_mode != SAFE_MODE_OFF );
                if( ImGui::Button( label.c_str() ) ) {
                    pending = btn.first;
                }
                if( ImGui::IsItemHovered() ) {
                    ImGui::SetTooltip( "%s", press_x( btn.first ).c_str() );
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
                const bool eat_on = get_option<bool>( "AUTO_EAT" );
                const std::string forage_mode = get_option<std::string>( "AUTO_FORAGING" );

                auto draw_styled_button = [&]( const char *id, const std::string & label,
                bool active ) {
                    place_button( label );
                    const int n = ui_hybrid_chrome::push_toolbar_button( active );
                    ImGui::PushID( id );
                    ImGui::Button( label.c_str() );
                    const bool left = ImGui::IsItemClicked( ImGuiMouseButton_Left );
                    const bool right = ImGui::IsItemClicked( ImGuiMouseButton_Right );
                    if( ImGui::IsItemHovered() ) {
                        ImGui::SetTooltip( "%s", _( "Left-click: toggle. Right-click: settings and help." ) );
                    }
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
                const std::string eat_label = eat_on ? _( "Eat●" ) : _( "Eat" );

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
                    if( ImGui::Selectable( string_format( "%s (%s)", _( "One-shot autoattack" ),
                                                          press_x( ACTION_AUTOATTACK ) ).c_str() ) ) {
                        pending = ACTION_AUTOATTACK;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                const auto eat_clicks = draw_styled_button( "tb_eat", eat_label, eat_on );
                if( eat_clicks.first ) {
                    pending = ACTION_TOGGLE_AUTO_EAT;
                } else if( eat_clicks.second ) {
                    ImGui::OpenPopup( "tb_eat_help" );
                }

                if( ImGui::BeginPopup( "tb_eat_help" ) ) {
                    ImGui::TextUnformatted( _( "Auto Eat / Drink" ) );
                    ImGui::Separator();
                    ImGui::TextWrapped( "%s",
                                        _( "When Eat● is on, each turn automatically eats "
                                           "or drinks from inventory if you are hungry or "
                                           "thirsty.\n\n"
                                           "Skips items that fail will_eat (inedible, "
                                           "rotten, parasites, allergy, cannibalism, "
                                           "nausea, already full) plus poison, strong "
                                           "health penalties, addiction risk, and major "
                                           "joy dumps.  Prefers items matching the "
                                           "current need and vitamin deficiencies.  "
                                           "Stops once Satisfied / not thirsty.  "
                                           "Medications are never auto-taken.\n\n"
                                           "If nothing safe is available, does nothing "
                                           "(one info message).  Toggle off to cancel." ) );
                    ImGui::EndPopup();
                }
            }
            ImGui::PopStyleVar( 2 );
        }

    private:
        std::optional<action_id> pending;
        cataimgui::bounds last_bounds = { 0.f, 0.f, 0.f, 0.f };

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
    hud_pending.reset();
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

void queue_action( action_id action )
{
    hud_pending = action;
}

std::optional<action_id> take_pending_action()
{
    if( hud_pending ) {
        const auto result = hud_pending;
        hud_pending.reset();
        return result;
    }
    ensure_shown();
    if( !g_toolbar ) {
        return std::nullopt;
    }
    return g_toolbar->take_pending();
}

bool has_pending_action()
{
    return hud_pending.has_value() || ( g_toolbar && g_toolbar->has_pending() );
}

} // namespace mouse_toolbar

#else // !TILES

namespace mouse_toolbar
{

void ensure_shown() {}
void queue_action( action_id ) {}
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
