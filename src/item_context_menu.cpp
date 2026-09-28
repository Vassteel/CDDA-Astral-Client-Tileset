#include "ui_telemetry.h"
#include "item_context_menu.h"

#include <string>
#include <vector>

#include "avatar.h"
#include "auto_pickup.h"
#include "output.h"
#include "units.h"
#include "avatar_action.h"
#include "character.h"
#include "enums.h"
#include "flag.h"
#include "activity_actor_definitions.h"
#include "game_inventory.h"
#include "game.h"
#include "imgui/imgui.h"
#include "item.h"
#include "itype.h"
#include "messages.h"
#include "debug.h"
#include "string_formatter.h"
#include "ret_val.h"
#include "translations.h"
#include "ui_iteminfo.h"
#include "visitable.h"

namespace item_context_menu
{
namespace
{

static const std::string comesttype_DRINK( "DRINK" );
static const std::string comesttype_MED( "MED" );

/** First consumable node under it (self or nested), or nullptr. */
static const item *first_consumable( const Character &you, const item &it )
{
    const item *found = nullptr;
    // visit_items is const on item but yields non-const node pointers.
    it.visit_items( [&]( item * node, item * ) {
        if( you.can_consume_as_is( *node ) ) {
            found = node;
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } );
    return found;
}

/**
 * Mirrors static rate_action_eat in game.cpp — good/iffy/cant for consume.
 */
static hint_rating rate_consume( const Character &you, const item &it )
{
    if( it.is_container() ) {
        hint_rating best_rate = hint_rating::cant;
        it.visit_items( [&]( item * node, item * ) {
            if( you.can_consume_as_is( *node ) ) {
                ret_val<edible_rating> rate = you.will_eat( *node );
                if( rate.success() ) {
                    best_rate = hint_rating::good;
                    return VisitResponse::ABORT;
                } else if( rate.value() != INEDIBLE && rate.value() != INEDIBLE_MUTATION ) {
                    best_rate = hint_rating::iffy;
                }
            }
            return VisitResponse::NEXT;
        } );
        return best_rate;
    }

    if( !you.can_consume_as_is( it ) ) {
        return hint_rating::cant;
    }
    const ret_val<edible_rating> rating = you.will_eat( it );
    if( rating.success() ) {
        return hint_rating::good;
    } else if( rating.value() == INEDIBLE || rating.value() == INEDIBLE_MUTATION ) {
        return hint_rating::cant;
    }
    return hint_rating::iffy;
}

/** Drink / Eat / Take from real comesttype + USE_EAT_VERB (same as consumption.cpp). */
static std::string consume_label_for( const Character &you, const item &it )
{
    const item *food = first_consumable( you, it );
    if( food == nullptr || !food->is_comestible() ) {
        return _( "Consume" );
    }
    if( food->get_comestible()->comesttype == comesttype_MED || food->is_medication() ) {
        return _( "Take" );
    }
    if( food->has_flag( flag_USE_EAT_VERB ) ) {
        return _( "Eat" );
    }
    if( food->get_comestible()->comesttype == comesttype_DRINK ) {
        return _( "Drink" );
    }
    return _( "Eat" );
}

static bool item_looks_activatable( const item &it )
{
    if( it.is_book() || it.is_craft() || it.is_medical_tool() ) {
        return true;
    }
    if( it.has_relic_activation() ) {
        return true;
    }
    // Pure comestibles are handled by Consume (Drink/Eat/Take).
    if( it.is_comestible() && !it.type->has_use() ) {
        return false;
    }
    return it.type->has_use() || it.item_has_uses_recursive();
}

static hint_rating rate_read( const Character &you, const item &it )
{
    if( !it.is_book() ) {
        return hint_rating::cant;
    }
    std::vector<std::string> dummy;
    // get_book_reader is on Character / avatar
    const Character *reader = you.get_book_reader( it, dummy );
    return reader ? hint_rating::good : hint_rating::iffy;
}

static hint_rating rate_wear( const Character &you, const item &it )
{
    if( !it.is_armor() && !it.is_pet_armor() ) {
        return hint_rating::cant;
    }
    if( you.is_worn( it ) ) {
        return hint_rating::iffy;
    }
    return you.can_wear( it ).success() ? hint_rating::good : hint_rating::iffy;
}

static hint_rating rate_takeoff( const Character &you, const item &it )
{
    if( you.is_wielding( it ) ) {
        return you.can_unwield( it ).success() ? hint_rating::good : hint_rating::cant;
    }
    if( !it.is_armor() || it.has_flag( flag_NO_TAKEOFF ) || it.has_flag( flag_INTEGRATED ) ) {
        return hint_rating::cant;
    }
    if( you.is_worn( it ) ) {
        return hint_rating::good;
    }
    return hint_rating::cant;
}

static bool menu_entry( const char *label, bool enabled )
{
    return ImGui::MenuItem( label, nullptr, false, enabled );
}

} // namespace

action draw_imgui_menu( Character &you, const item_location &loc, bool from_worn,
                        std::string *chosen_use_method )
{
    if( chosen_use_method ) {
        chosen_use_method->clear();
    }
    if( !loc || !loc.get_item() ) {
        return action::none;
    }
    const item &it = *loc;
    action chosen = action::none;

    const hint_rating consume_r = rate_consume( you, it );
    if( consume_r != hint_rating::cant ) {
        const std::string label = consume_label_for( you, it );
        if( menu_entry( label.c_str(), true ) ) {
            chosen = action::consume;
        }
    }

    // Surface each itype use_methods entry under its real CDDA label
    // (iuse_transform::get_name → menu_text, else item_actions.json name —
    // e.g. flashlight transform defaults to "Turn on", on-state menu_text
    // "Turn off"). Mirrors avatar::invoke_item's method list.
    bool listed_use_method = false;
    for( const auto &e : it.type->use_methods ) {
        listed_use_method = true;
        const ret_val<void> can = e.second.can_call( you, it, you.pos_bub() );
        const std::string label = e.second.get_name();
        if( menu_entry( label.c_str(), can.success() ) ) {
            chosen = action::use;
            if( chosen_use_method ) {
                *chosen_use_method = e.first;
            }
        }
    }
    // Books / medical / relics / recursive uses without a typed use_methods map.
    if( !listed_use_method ) {
        const bool can_use = item_looks_activatable( it );
        if( menu_entry( _( "Use" ), can_use ) ) {
            chosen = action::use;
        }
    }

    if( rate_read( you, it ) != hint_rating::cant ) {
        if( menu_entry( _( "Read" ), true ) ) {
            chosen = action::read;
        }
    }

    if( !from_worn ) {
        if( rate_wear( you, it ) != hint_rating::cant ) {
            if( menu_entry( _( "Wear" ), you.can_wear( it ).success() ) ) {
                chosen = action::wear;
            }
        }
        if( menu_entry( _( "Wield" ), you.can_wield( it ).success() ) ) {
            chosen = action::wield;
        }
    }

    if( rate_takeoff( you, it ) != hint_rating::cant ) {
        const char *toff = you.is_wielding( it ) ? _( "Unwield" ) : _( "Take off" );
        if( menu_entry( toff, true ) ) {
            chosen = action::takeoff;
        }
    }

    if( !from_worn ) {
        const item_location::type where = loc.where();
        const bool on_ground = where == item_location::type::map ||
                               where == item_location::type::vehicle;
        if( on_ground ) {
            if( menu_entry( _( "Pick up" ), true ) ) {
                chosen = action::pickup;
            }
        } else {
            const bool can_drop = you.can_drop( it ).success();
            if( menu_entry( _( "Drop" ), can_drop ) ) {
                chosen = action::drop;
            }
            if( menu_entry( _( "Drop stack" ), can_drop && loc.held_by( you ) ) ) {
                chosen = action::drop_stack;
            }
        }
    }

    if( you.rate_action_unload( it ) != hint_rating::cant ) {
        if( menu_entry( _( "Unload" ), you.rate_action_unload( it ) == hint_rating::good ) ) {
            chosen = action::unload;
        }
    }

    if( you.rate_action_reload( it ) != hint_rating::cant ) {
        if( menu_entry( _( "Reload" ), you.rate_action_reload( it ) == hint_rating::good ) ) {
            chosen = action::reload;
        }
    }

    if( menu_entry( _( "More item actions…" ), true ) ) {
        chosen = action::more_actions;
    }
    if( menu_entry( _( "Examine" ), true ) ) {
        chosen = action::examine;
    }

    ImGui::Separator();
    if( menu_entry( _( "Always pick up this item" ), true ) ) {
        chosen = action::always_pickup;
    }
    if( menu_entry( _( "Never pick up this item" ), true ) ) {
        chosen = action::never_pickup;
    }

    return chosen;
}

action draw_inspector( Character &you, const item_location &loc, std::string *chosen_use_method )
{
    if( !loc ) {
        ImGui::TextDisabled( "%s", _( "Select an item to inspect it." ) );
        return action::none;
    }
    ImGui::TextWrapped( "%s", remove_color_tags( loc->display_name() ).c_str() );
    ImGui::Text( "%s", string_format( _( "Weight %.2f kg   Volume %.2f L" ),
                                      units::to_gram( loc->weight() ) / 1000.0,
                                      units::to_milliliter( loc->volume() ) / 1000.0 ).c_str() );
    if( loc->is_armor() && !you.is_worn( *loc ) ) {
        const ret_val<void> wear = you.can_wear( *loc );
        if( !wear.success() ) {
            ImGui::TextWrapped( "%s", wear.str().c_str() );
        }
    }
    action chosen = action::none;
    if( ImGui::Button( _( "Details" ) ) ) {
        chosen = action::examine;
    }
    ImGui::SameLine();
    if( ImGui::Button( _( "Item actions" ) ) ) {
        ImGui::OpenPopup( "item_actions" );
    }
    if( ImGui::BeginPopup( "item_actions" ) ) {
        const action clicked = draw_imgui_menu( you, loc, you.is_worn( *loc ) ||
                                                you.is_wielding( *loc ), chosen_use_method );
        if( clicked != action::none ) {
            chosen = clicked;
        }
        ImGui::EndPopup();
    }
    return chosen;
}

std::string perform( Character &you, item_location loc, action act,
                     const std::string &use_method )
{
    if( act == action::none || !loc || !loc.get_item() ) {
        return {};
    }
    static const char *const names[] = { "none", "consume", "use", "read", "wear", "wield",
                                         "takeoff", "drop", "drop_stack", "pickup", "unload", "reload", "examine", "always_pickup", "never_pickup", "more_actions"
                                       };
    const ui_telemetry::scope trace( "item.action", {{ "action", names[static_cast<int>( act )] },
        { "type", loc->typeId().str() }, { "charges", std::to_string( loc->charges ) },
        { "method", use_method }
    } );
    avatar *av = you.as_avatar();

    switch( act ) {
        case action::more_actions:
            g->inventory_item_menu( loc );
            return {};
        case action::always_pickup:
        case action::never_pickup:
            get_auto_pickup().remove_rule( &*loc );
            get_auto_pickup().add_rule( &*loc, act == action::always_pickup );
            get_auto_pickup().save_character();
            return _( "Autopickup rule updated." );
        case action::consume: {
            if( av == nullptr ) {
                return _( "Only the player can consume items here." );
            }
            // Soft-fork: inventory RMB Eat/Drink/Take must consume immediately.
            // Never open Hybrid Consume UI from the context menu (containers
            // used to call game_menus::inv::consume and jump to that tab).
            item_location target = loc;
            if( !av->can_consume_as_is( *loc ) && !loc->is_medical_tool() ) {
                item &food = av->get_consumable_from( *loc );
                if( food.is_null() ) {
                    return _( "Nothing to consume." );
                }
                if( &food != loc.get_item() ) {
                    target = item_location( loc, &food );
                }
            }
            avatar_action::eat_or_use( *av, target );
            return _( "Consuming…" );
        }
        case action::use: {
            if( av == nullptr ) {
                return _( "Only the player can use items here." );
            }
            // Specific use_methods key (Turn on/off transform, etc.) — skip
            // comestible/book shortcuts so the typed iuse runs.
            if( !use_method.empty() ) {
                avatar_action::use_item( *av, loc, use_method );
                return _( "Used." );
            }
            if( loc->is_comestible() || loc->is_medical_tool() ) {
                avatar_action::eat_or_use( *av, loc );
                return _( "Using…" );
            }
            if( loc->is_book() ) {
                av->read( loc );
                return _( "Reading…" );
            }
            avatar_action::use_item( *av, loc );
            return _( "Used." );
        }
        case action::read: {
            if( av == nullptr ) {
                return _( "Only the player can read here." );
            }
            av->read( loc );
            return _( "Reading…" );
        }
        case action::wear: {
            const ret_val<void> can = you.can_wear( *loc );
            if( !can.success() ) {
                add_msg( m_info, can.str() );
                return can.str();
            }
            if( you.wear( loc ) ) {
                return _( "Worn." );
            }
            return _( "Could not wear that." );
        }
        case action::wield: {
            const ret_val<void> can = you.can_wield( *loc );
            if( !can.success() ) {
                add_msg( m_info, can.str() );
                return can.str();
            }
            if( you.wield( loc ) ) {
                return _( "Wielded." );
            }
            return _( "Could not wield that." );
        }
        case action::takeoff: {
            if( you.is_wielding( *loc ) ) {
                if( you.can_unwield( *loc ).success() && you.unwield() ) {
                    return _( "Unwielded." );
                }
                return you.can_unwield( *loc ).str();
            }
            const ret_val<void> can = you.can_takeoff( *loc );
            if( !can.success() ) {
                add_msg( m_info, can.str() );
                return can.str();
            }
            item_location obtained = loc.obtain( you );
            if( you.takeoff( obtained ) ) {
                return _( "Taken off." );
            }
            return _( "Could not take that off." );
        }
        case action::drop: {
            const ret_val<void> can = you.can_drop( *loc );
            if( !can.success() ) {
                add_msg( m_info, can.str() );
                return can.str();
            }
            you.drop( loc, you.pos_bub() );
            return _( "Dropped." );
        }
        case action::drop_stack: {
            if( !loc.held_by( you ) || you.is_worn( *loc ) || you.is_wielding( *loc ) ) {
                return _( "Select a stack in your inventory." );
            }
            const ret_val<void> can = you.can_drop( *loc );
            if( !can.success() ) {
                add_msg( m_info, can.str() );
                return can.str();
            }
            // Match the equipment grid's display stack, including items in
            // different pockets, but never include equipped or unlike items.
            // Charge-counted items occupy one cell; count() drops every charge.
            drop_locations what;
            for( const item_location &candidate : you.all_items_loc() ) {
                if( candidate && !you.is_worn( *candidate ) && !you.is_wielding( *candidate ) &&
                    ( candidate == loc || candidate->display_stacked_with( *loc ) ) ) {
                    what.emplace_back( candidate, candidate->count() );
                }
            }
            you.drop( what, you.pos_bub() );
            you.invalidate_inventory_validity_cache();
            return _( "Dropping stack." );
        }
        case action::pickup: {
            const item_location::type where = loc.where();
            if( where != item_location::type::map &&
                where != item_location::type::vehicle ) {
                return _( "That item is not on the ground." );
            }
            drop_locations what;
            const int qty = loc->count_by_charges() ? loc->charges : 0;
            what.emplace_back( loc, qty );
            you.pick_up( what );
            return _( "Picking up…" );
        }
        case action::unload: {
            if( you.unload( loc ) ) {
                return _( "Unloaded." );
            }
            return _( "Could not unload." );
        }
        case action::reload: {
            // Mirror game::reload (public APIs only): select ammo, then
            // reload_activity_actor. Soft-fork adds find_ammo / list_ammo
            // telemetry so empty pickers for MAGAZINE_WELL tools (fire_drill)
            // are diagnosable.
            if( loc->type->can_use( "holster" ) && loc->num_item_stacks() == 1 ) {
                loc = item_location( loc, &loc->only_item() );
            }
            const std::vector<item_location> found = you.find_ammo( *loc, /*empty=*/true, /*radius=*/1 );
            std::vector<item::reload_option> ammo_list;
            const bool list_match = you.list_ammo( loc, ammo_list, /*empty=*/true,
                                                   /*per_well_targets=*/true );
            int accept_now = 0;
            int reject_now = 0;
            for( const item_location &cand : found ) {
                if( !cand || !cand.get_item() ) {
                    continue;
                }
                if( loc->can_reload_with( *cand, true ) ) {
                    ++accept_now;
                } else {
                    ++reject_now;
                }
            }
            DebugLog( D_INFO, D_MAIN ) << string_format(
                                           "rpg_eq_ctx: select_ammo begin for '%s' "
                                           "find_ammo=%zu list_ammo_match=%d list_ammo_size=%zu "
                                           "can_reload_with_now accept=%d reject=%d "
                                           "is_reloadable=%d rate=%d",
                                           loc->tname(), found.size(), list_match ? 1 : 0,
                                           ammo_list.size(), accept_now, reject_now,
                                           loc->is_reloadable() ? 1 : 0,
                                           static_cast<int>( you.rate_action_reload( *loc ) ) );
            item::reload_option opt = you.select_ammo( loc, /*prompt=*/true );
            if( !opt || opt.ammo.get_item() == nullptr ) {
                DebugLog( D_INFO, D_MAIN ) << string_format(
                                               "rpg_eq_ctx: select_ammo empty/canceled for '%s' "
                                               "(find_ammo=%zu list_ammo_size=%zu)",
                                               loc->tname(), found.size(), ammo_list.size() );
                // If the ammo UI came up empty but list_ammo found candidates
                // (nested-UI / filter glitch), fall back to the best list option.
                if( !ammo_list.empty() ) {
                    DebugLog( D_INFO, D_MAIN ) << "rpg_eq_ctx: select_ammo fallback list_ammo[0]="
                                               << ammo_list.front().ammo->tname();
                    you.assign_activity( reload_activity_actor( std::move( ammo_list.front() ) ) );
                    return _( "Reloading…" );
                }
                return _( "Reload canceled." );
            }
            DebugLog( D_INFO, D_MAIN ) << "rpg_eq_ctx: select_ammo ok ammo="
                                       << opt.ammo->tname();
            you.assign_activity( reload_activity_actor( std::move( opt ) ) );
            return _( "Reloading…" );
        }
        case action::examine: {
            std::vector<iteminfo> vThisItem;
            std::vector<iteminfo> vDummy;
            loc->info( true, vThisItem );
            const std::string title = loc->tname( 1, tname::unprefixed_tname, true );
            const std::string type_nm = loc->type_name();
            item_info_data data( title, type_nm, vThisItem, vDummy );
            data.handle_scrolling = true;
            data.arrow_scrolling = true;
            const int maxwidth = std::max( FULL_SCREEN_WIDTH, TERMX );
            const int width = std::min( 80, maxwidth );
            iteminfo_window info_window( data, point( maxwidth / 2 - width / 2, -1 ), width, 0 );
            info_window.execute();
            return _( "Examined." );
        }
        case action::none:
            break;
    }
    return {};
}

} // namespace item_context_menu
