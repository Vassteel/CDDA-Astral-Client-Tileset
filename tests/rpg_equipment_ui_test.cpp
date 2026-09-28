#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "item.h"
#include "player_helpers.h"
#include "ret_val.h"
#include "rpg_equipment_ui.h"
#include "type_id.h"

TEST_CASE( "Doll_storage_slots_separate_weapon_holders_from_backpacks", "[doll_slots]" )
{
    using slot = rpg_equipment_ui::storage_slot;
    for( const char *id : { "baldric", "bscabbard", "scabbard" } ) {
        CAPTURE( id );
        CHECK( rpg_equipment_ui::storage_slot_for( item( itype_id( id ) ) ) == slot::scabbard );
    }
    for( const char *id : { "sheath", "sheath_birchbark", "bootsheath_birchbark", "leg_sheath6" } ) {
        CAPTURE( id );
        CHECK( rpg_equipment_ui::storage_slot_for( item( itype_id( id ) ) ) == slot::sheath );
    }
    CHECK( rpg_equipment_ui::storage_slot_for( item( itype_id( "back_holster" ) ) ) == slot::holster );
    item loaded_pack( itype_id( "backpack" ) );
    loaded_pack.put_in( item( itype_id( "sheath" ) ), pocket_type::CONTAINER );
    // Item actions can search contents recursively; classification must use the container itself.
    CHECK( rpg_equipment_ui::storage_slot_for( loaded_pack ) == slot::back );
    // Backpack pockets include holsters for bottles/long items; those are not scabbards.
    CHECK( rpg_equipment_ui::storage_slot_for( item( itype_id( "backpack" ) ) ) == slot::back );
    for( const char *id : { "jacket_leather", "tshirt", "jeans", "longsword" } ) {
        CAPTURE( id );
        CHECK( rpg_equipment_ui::storage_slot_for( item( itype_id( id ) ) ) == slot::none );
    }
}

TEST_CASE( "Doll_scabbard_and_backpack_keep_native_wear_and_storage", "[doll_slots]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    REQUIRE( you.wear_item( item( itype_id( "backpack" ) ), false ).has_value() );
    item scabbard( itype_id( "bscabbard" ) );
    REQUIRE( you.can_wear( scabbard ).success() );
    REQUIRE( you.wear_item( scabbard, false ).has_value() );
    CHECK( scabbard.can_holster( item( itype_id( "longsword" ) ) ) );
    CHECK_FALSE( scabbard.can_holster( item( itype_id( "backpack" ) ) ) );
}
