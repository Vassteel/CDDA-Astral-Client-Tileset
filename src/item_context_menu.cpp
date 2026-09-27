#include "item_context_menu.h"

#include <string>
#include <vector>

#include "avatar.h"
#include "avatar_action.h"
#include "character.h"
#include "enums.h"
#include "flag.h"
#include "activity_actor_definitions.h"
#include "game_inventory.h"
#include "imgui/imgui.h"
#include "item.h"
#include "itype.h"
#include "messages.h"
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

action draw_imgui_menu( Character &you, const item_location &loc, bool from_worn )
{
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

    const bool can_use = item_looks_activatable( it );
    if( menu_entry( _( "Use" ), can_use ) ) {
        chosen = action::use;
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
        const bool can_drop = you.can_drop( it ).success();
        if( menu_entry( _( "Drop" ), can_drop ) ) {
            chosen = action::drop;
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

    if( menu_entry( _( "Examine" ), true ) ) {
        chosen = action::examine;
    }

    return chosen;
}

std::string perform( Character &you, item_location loc, action act )
{
    if( act == action::none || !loc || !loc.get_item() ) {
        return {};
    }
    avatar *av = you.as_avatar();

    switch( act ) {
        case action::consume: {
            if( av == nullptr ) {
                return _( "Only the player can consume items here." );
            }
            // Match game::inventory_item_menu 'E':
            // non-container → eat directly; container → consume picker on contents.
            if( !loc->is_container() ) {
                avatar_action::eat( *av, loc );
                return _( "Consuming…" );
            }
            item_location picked = game_menus::inv::consume( std::string(), loc );
            if( picked ) {
                avatar_action::eat_or_use( *av, picked );
                return _( "Consuming…" );
            }
            return _( "Nothing to consume." );
        }
        case action::use: {
            if( av == nullptr ) {
                return _( "Only the player can use items here." );
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
        case action::unload: {
            if( you.unload( loc ) ) {
                return _( "Unloaded." );
            }
            return _( "Could not unload." );
        }
        case action::reload: {
            // Mirror the public path used by game::reload without calling the private method:
            // select ammo, then assign reload_activity_actor.
            if( loc->type->can_use( "holster" ) && loc->num_item_stacks() == 1 ) {
                loc = item_location( loc, &loc->only_item() );
            }
            item::reload_option opt = you.select_ammo( loc, /*prompt=*/true );
            if( !opt || opt.ammo.get_item() == nullptr ) {
                return _( "Reload canceled." );
            }
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
