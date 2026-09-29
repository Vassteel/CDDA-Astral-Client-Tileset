#include "avatar.h"
#include "cata_catch.h"
#include "equipment_actions.h"
#include "flag.h"
#include "item.h"
#include "item_location.h"
#include "player_helpers.h"

TEST_CASE( "Clicked_unwield_stows_without_a_disposal_menu", "[equipment_actions][unwield]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    REQUIRE( you.wear_item( item( itype_id( "backpack" ) ), false ) );
    item knife( itype_id( "knife_combat" ) );
    knife.set_var( "unwield_identity", "original" );
    REQUIRE( you.wield( knife ) );
    const int moves = you.get_moves();
    REQUIRE( equipment_actions::unwield_to_inventory( you ).success() );
    CHECK_FALSE( you.get_wielded_item() );
    CHECK( you.get_moves() < moves );
    const auto stored = you.items_with( []( const item & it ) {
        return it.get_var( "unwield_identity" ) == "original";
    } );
    REQUIRE( stored.size() == 1 );
    CHECK( stored.front()->typeId() == itype_id( "knife_combat" ) );
}

TEST_CASE( "Clicked_unwield_without_space_keeps_weapon_and_moves", "[equipment_actions][unwield]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    item weapon( itype_id( "backpack" ) );
    REQUIRE( you.wield( weapon ) );
    item_location original = you.get_wielded_item();
    const int moves = you.get_moves();
    // The wielded backpack must not be chosen as its own destination.
    CHECK_FALSE( equipment_actions::unwield_to_inventory( you ).success() );
    REQUIRE( you.get_wielded_item() );
    CHECK( you.get_wielded_item().get_item() == original.get_item() );
    CHECK( you.get_moves() == moves );
}

TEST_CASE( "Clicked_unwield_obeys_weapon_restrictions", "[equipment_actions][unwield]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    REQUIRE( you.wear_item( item( itype_id( "backpack" ) ), false ) );
    item knife( itype_id( "knife_combat" ) );
    REQUIRE( you.wield( knife ) );
    you.get_wielded_item()->set_flag( flag_id( "NO_UNWIELD" ) );
    const int moves = you.get_moves();
    CHECK_FALSE( equipment_actions::unwield_to_inventory( you ).success() );
    CHECK( you.get_wielded_item() );
    CHECK( you.get_moves() == moves );
}

TEST_CASE( "Explicit_side_wear_preserves_existing_same_type_item", "[equipment_actions]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    item first( itype_id( "wristwatch" ) );
    first.set_side( side::LEFT );
    first.set_var( "review_identity", "original" );
    const auto existing = you.wear_item( first, false );
    REQUIRE( existing );
    item *original = & **existing;
    item next( itype_id( "wristwatch" ) );
    next.set_var( "review_identity", "new" );
    item_location source = you.i_add( next );
    REQUIRE( equipment_actions::wear_on_side( you, source, side::RIGHT, false ).success() );
    CHECK( you.is_worn( *original ) );
    CHECK( original->get_side() == side::LEFT );
    int count = 0;
    for( const auto &loc : you.top_items_loc() ) {
        if( loc && you.is_worn( *loc ) && loc->typeId() == itype_id( "wristwatch" ) ) {
            ++count;
            if( loc->get_var( "review_identity" ) == "new" ) {
                CHECK( loc->get_side() == side::RIGHT );
            }
        }
    }
    CHECK( count == 2 );
}

TEST_CASE( "Occupied_explicit_side_rejects_without_moving_items", "[equipment_actions]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    item first( itype_id( "wristwatch" ) );
    first.set_flag( flag_id( "ONE_PER_LAYER" ) );
    first.set_side( side::RIGHT );
    REQUIRE( you.wear_item( first, false ) );
    item next( itype_id( "wristwatch" ) );
    next.set_flag( flag_id( "ONE_PER_LAYER" ) );
    item_location source = you.i_add( next );
    const item *identity = source.get_item();
    const int moves = you.get_moves();
    const side before = source->get_side();
    CHECK_FALSE( equipment_actions::wear_on_side( you, source, side::RIGHT, false ).success() );
    REQUIRE( source );
    CHECK( source.get_item() == identity );
    CHECK( source->get_side() == before );
    CHECK_FALSE( you.is_worn( *source ) );
    CHECK( you.get_moves() == moves );
}
