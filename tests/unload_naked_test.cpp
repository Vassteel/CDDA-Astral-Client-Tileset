#include <functional>
#include <vector>

#include "avatar.h"
#include "activity_actor_definitions.h"
#include "cata_catch.h"
#include "character.h"
#include "item.h"
#include "item_location.h"
#include "map_helpers.h"
#include "map.h"
#include "map_selector.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "type_id.h"

static const itype_id itype_sw629( "sw629" );

TEST_CASE( "unload_activity_survives_removed_target", "[unload][crash_regression]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &you = get_avatar();
    map &here = get_map();
    item_location target;
    SECTION( "missing_target" ) {
        CHECK_FALSE( target );
    }
    SECTION( "target_removed_during_activity" ) {
        item &gun = here.add_item_or_charges( you.pos_bub(), item( itype_sw629 ) );
        target = item_location( map_cursor( you.pos_bub() ), &gun );
        REQUIRE( target );
        you.assign_activity( unload_activity_actor( 1, target ) );
        target.remove_item();
        REQUIRE_FALSE( target );
    }
    if( !you.activity ) {
        you.assign_activity( unload_activity_actor( 1, target ) );
    }
    you.set_moves( 100 );
    you.activity.do_turn( you );
    CHECK_FALSE( you.activity );
    CHECK( here.i_at( you.pos_bub() ).empty() );
}

/**
 * When a player has no open inventory to place unloaded bullets, but there is room in the gun being
 * unloaded the bullets are dropped.
 * issue# 29839
 */

TEST_CASE( "unload_revolver_naked_one_bullet", "[unload][nonmagzine]" )
{
    clear_avatar();
    clear_map_without_vision();

    Character &dummy = get_player_character();
    avatar &player_character = get_avatar();

    // revolver with only one of six bullets
    item revolver( itype_sw629 );
    revolver.ammo_set( revolver.ammo_default(), 1 );

    // wield the revolver
    REQUIRE( !player_character.is_armed( ) );
    REQUIRE( player_character.wield( revolver ) );
    REQUIRE( player_character.is_armed( ) );

    CHECK( player_character.get_wielded_item()->ammo_remaining( ) == 1 );

    // Unload weapon
    item_location revo_loc = player_character.get_wielded_item();
    player_character.set_moves( 200 );
    REQUIRE( player_character.unload( revo_loc ) );
    player_character.activity.do_turn( player_character );

    // No bullets in wielded gun
    CHECK( player_character.get_wielded_item()->ammo_remaining( ) == 0 );

    // No bullets in inventory
    const std::vector<item *> bullets = dummy.items_with( []( const item & item ) {
        return item.is_ammo();
    } );
    CHECK( bullets.empty() );
}

TEST_CASE( "unload_revolver_naked_fully_loaded", "[unload][nonmagzine]" )
{
    clear_avatar();
    clear_map_without_vision();

    Character &dummy = get_player_character();
    avatar &player_character = get_avatar();

    // revolver fully loaded
    item revolver( itype_sw629 );
    revolver.ammo_set( revolver.ammo_default(), revolver.remaining_ammo_capacity() );

    // wield the revolver
    REQUIRE( !player_character.is_armed( ) );
    REQUIRE( player_character.wield( revolver ) );
    REQUIRE( player_character.is_armed( ) );

    CHECK( player_character.get_wielded_item()->remaining_ammo_capacity() == 0 );

    // Unload weapon
    item_location revo_loc = player_character.get_wielded_item();
    player_character.set_moves( 200 );
    REQUIRE( player_character.unload( revo_loc ) );
    while( player_character.activity ) {
        player_character.set_moves( 200 );
        player_character.activity.do_turn( player_character );
    }

    // No bullets in wielded gun
    CHECK( player_character.get_wielded_item()->ammo_remaining( ) == 0 );

    // No bullets in inventory
    const std::vector<item *> bullets = dummy.items_with( []( const item & item ) {
        return item.is_ammo();
    } );
    CHECK( bullets.empty() );
}
