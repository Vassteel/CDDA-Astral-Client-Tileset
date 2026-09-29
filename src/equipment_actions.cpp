#include "equipment_actions.h"

#include "character.h"
#include "crafting.h"
#include "flag.h"
#include "item.h"
#include "item_location.h"
#include "translations.h"

namespace equipment_actions
{
ret_val<void> unwield_to_inventory( Character &who )
{
    item_location source = who.get_wielded_item();
    if( !source ) {
        return ret_val<void>::make_failure( _( "Nothing is wielded." ) );
    }
    const auto can = who.can_unwield( *source );
    if( !can.success() ) {
        return can;
    }
    // Retract bionic weapons through their native deactivation path.
    if( source->has_flag( flag_id( "NO_UNWIELD" ) ) ) {
        return who.unwield() ? ret_val<void>::make_success() :
               ret_val<void>::make_failure( _( "Could not deactivate that weapon." ) );
    }
    if( source->will_spill() && !source->is_container_empty() ) {
        return ret_val<void>::make_failure( _( "Empty or seal that container before stowing it." ) );
    }
    const int cost = who.item_handling_cost( *source );
    // Avoid storing a wielded container inside itself. try_add never drops the
    // item, and the original remains wielded if no suitable pocket is available.
    const item_location stored = who.try_add( *source, source.get_item(), source.get_item(), false );
    if( !stored ) {
        return ret_val<void>::make_failure( _( "No room in inventory; item remains wielded." ) );
    }
    source.remove_item();
    who.mod_moves( -cost );
    craft_relocated( stored );
    who.invalidate_crafting_inventory();
    who.invalidate_weight_carried_cache();
    return ret_val<void>::make_success();
}

ret_val<void> wear_on_side( Character &who, item_location source, side requested,
                            bool interactive )
{
    if( !source || who.is_worn( *source ) ) {
        return ret_val<void>::make_failure( _( "Select an unworn item." ) );
    }
    if( !source->is_sided() || ( requested != side::LEFT && requested != side::RIGHT ) ) {
        return ret_val<void>::make_failure( _( "This item does not use a single side." ) );
    }
    // Validate a copy: even a rejected drop must leave the original location,
    // item side, and character's moves intact.
    item candidate( *source );
    candidate.set_side( requested );
    const auto can = who.can_wear( candidate );
    if( !can.success() ) {
        return can;
    }
    const auto rigid = who.worn.check_rigid_conflicts( candidate, requested );
    if( !rigid.success() ) {
        return rigid;
    }
    static const flag_id one_per_layer( "ONE_PER_LAYER" );
    const auto conflicts = [&]( const item & existing ) {
        return ( candidate.has_flag( one_per_layer ) || existing.has_flag( one_per_layer ) ) &&
               candidate.covers_overlaps( existing ).has_value();
    };
    for( const item_location &loc : who.top_items_loc() ) {
        if( !loc || !who.is_worn( *loc ) ) {
            continue;
        }
        if( conflicts( *loc ) ) {
            return ret_val<void>::make_failure( _( "That side is already occupied." ) );
        }
        if( loc->is_ablative() ) {
            for( const item *plate : loc->all_ablative_armor() ) {
                if( conflicts( *plate ) ) {
                    return ret_val<void>::make_failure( _( "That side is already occupied." ) );
                }
            }
        }
    }
    const auto worn = who.wear( source, interactive );
    if( !worn ) {
        return ret_val<void>::make_failure( _( "Could not wear that." ) );
    }
    // Native wear returns the exact new instance. Never search by type: another
    // watch/ring of the same type may already be worn on the opposite side.
    item &equipped = **worn;
    if( equipped.get_side() != requested && !who.change_side( equipped, interactive ) ) {
        return ret_val<void>::make_failure( _( "Worn, but could not change to the requested side." ) );
    }
    return ret_val<void>::make_success();
}
}
