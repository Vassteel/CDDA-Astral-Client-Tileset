#include <string>
#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "game.h"
#include "item.h"
#include "magic_ter_furn_transform.h"
#include "map.h"
#include "map_helpers.h"
#include "mapdata.h"
#include "player_helpers.h"
#include "point.h"
#include "talker.h"
#include "type_id.h"

// Project Astral — spine milestone S2 ("one portal, one persistent world").
// See doc/astral/dungeon-implementation-plan.md.  Data lives in data/json/astral/dungeons/.

static const dimension_id dimension_astral_test_world( "astral_test_world" );
static const dimension_id dimension_default( "default" );

static const effect_on_condition_id effect_on_condition_EOC_ASTRAL_DEBUG_SET_ACTIVE(
    "EOC_ASTRAL_DEBUG_SET_ACTIVE" );
static const effect_on_condition_id effect_on_condition_EOC_ASTRAL_DEBUG_SET_INACTIVE(
    "EOC_ASTRAL_DEBUG_SET_INACTIVE" );
static const effect_on_condition_id effect_on_condition_EOC_ASTRAL_DEBUG_SET_RUINED(
    "EOC_ASTRAL_DEBUG_SET_RUINED" );
static const effect_on_condition_id effect_on_condition_EOC_ASTRAL_DEBUG_PLACE_PORTAL(
    "EOC_ASTRAL_DEBUG_PLACE_PORTAL" );
static const effect_on_condition_id effect_on_condition_EOC_ASTRAL_PORTAL_ENTER_DO(
    "EOC_ASTRAL_PORTAL_ENTER_DO" );
static const effect_on_condition_id effect_on_condition_EOC_ASTRAL_PORTAL_RETURN_DO(
    "EOC_ASTRAL_PORTAL_RETURN_DO" );
static const effect_on_condition_id effect_on_condition_EOC_ASTRAL_PORTAL_THRESHOLD(
    "EOC_ASTRAL_PORTAL_THRESHOLD" );

static const std::vector<std::string> astral_states = { "active", "inactive", "ruined" };

// Row 0 is the north (back) edge of the arch; the entrance faces south.
// Must match ROLES in tools/astral/gen_astral_portal_data.py.
static const bool astral_passable[5][5] = {
    { false, false, false, false, false },
    { false, false, true,  false, false },
    { false, true,  true,  true,  false },
    { true,  true,  true,  true,  true  },
    { true,  true,  true,  true,  true  },
};

static ter_str_id portal_ter( const std::string &state, int r, int c )
{
    return ter_str_id( "t_astral_portal_" + state + "_r" + std::to_string( r ) + "c" +
                       std::to_string( c ) );
}

// Paint a whole portal of the given state with its r2c2 (the arrival/standing tile) at `center`.
static void paint_portal( map &here, const std::string &state, const tripoint_bub_ms &center )
{
    for( int r = 0; r < 5; r++ ) {
        for( int c = 0; c < 5; c++ ) {
            here.ter_set( center + point( c - 2, r - 2 ), portal_ter( state, r, c ) );
        }
    }
}

static void require_portal( map &here, const std::string &state, const tripoint_bub_ms &center )
{
    for( int r = 0; r < 5; r++ ) {
        for( int c = 0; c < 5; c++ ) {
            CAPTURE( state, r, c );
            CHECK( here.ter( center + point( c - 2, r - 2 ) ).id() == portal_ter( state, r, c ) );
        }
    }
}

TEST_CASE( "astral_portal_terrain_definitions", "[astral][terrain]" )
{
    REQUIRE( ter_str_id( "t_astral_veil" ).is_valid() );
    const ter_t &veil = ter_str_id( "t_astral_veil" ).obj();
    CHECK( veil.movecost == 0 );
    CHECK_FALSE( veil.has_flag( ter_furn_flag::TFLAG_TRANSPARENT ) );
    CHECK_FALSE( veil.is_smashable() );

    for( const std::string &state : astral_states ) {
        for( int r = 0; r < 5; r++ ) {
            for( int c = 0; c < 5; c++ ) {
                const ter_str_id id = portal_ter( state, r, c );
                CAPTURE( id.str() );
                REQUIRE( id.is_valid() );
                const ter_t &t = id.obj();
                if( astral_passable[r][c] ) {
                    CHECK( t.movecost > 0 );
                } else {
                    CHECK( t.movecost == 0 );
                    // The gateway is monumental: nothing in S2 can bash it down.
                    CHECK_FALSE( t.is_smashable() );
                }
                if( r == 1 && c == 2 ) {
                    // The threshold is the only interactable tile.
                    CHECK( t.can_examine( tripoint_bub_ms::zero ) );
                }
            }
        }
    }
}

TEST_CASE( "astral_portal_eocs_and_transforms_exist", "[astral][eoc]" )
{
    for( const std::string &state : astral_states ) {
        CHECK( ter_furn_transform_id( "astral_portal_to_" + state ).is_valid() );
    }
    CHECK( effect_on_condition_EOC_ASTRAL_PORTAL_THRESHOLD.is_valid() );
    CHECK( effect_on_condition_EOC_ASTRAL_PORTAL_ENTER_DO.is_valid() );
    CHECK( effect_on_condition_EOC_ASTRAL_PORTAL_RETURN_DO.is_valid() );
    CHECK( effect_on_condition_EOC_ASTRAL_DEBUG_PLACE_PORTAL.is_valid() );
    CHECK( dimension_astral_test_world.is_valid() );
}

TEST_CASE( "astral_portal_state_transforms_cover_every_cell", "[astral][eoc]" )
{
    clear_avatar();
    clear_map_without_vision();
    map &here = get_map();
    avatar &u = get_avatar();
    const tripoint_bub_ms center = u.pos_bub();
    dialogue d( get_talker_for( u ), nullptr );

    paint_portal( here, "inactive", center );
    require_portal( here, "inactive", center );

    effect_on_condition_EOC_ASTRAL_DEBUG_SET_ACTIVE->activate( d );
    require_portal( here, "active", center );

    effect_on_condition_EOC_ASTRAL_DEBUG_SET_RUINED->activate( d );
    require_portal( here, "ruined", center );

    effect_on_condition_EOC_ASTRAL_DEBUG_SET_INACTIVE->activate( d );
    require_portal( here, "inactive", center );

    effect_on_condition_EOC_ASTRAL_DEBUG_SET_ACTIVE->activate( d );
    require_portal( here, "active", center );
}

TEST_CASE( "astral_debug_placer_draws_an_active_portal", "[astral][mapgen]" )
{
    clear_avatar();
    clear_map_without_vision();
    map &here = get_map();
    avatar &u = get_avatar();
    dialogue d( get_talker_for( u ), nullptr );

    effect_on_condition_EOC_ASTRAL_DEBUG_PLACE_PORTAL->activate( d );

    // update_mapgen astral_place_portal_active draws rows 8-12 / cols 9-13 of the player's OMT.
    const tripoint_abs_omt omt = project_to<coords::omt>( u.pos_abs() );
    const tripoint_bub_ms omt_origin = here.get_bub( project_to<coords::ms>( omt ) );
    const tripoint_bub_ms center = omt_origin + point( 11, 10 );
    require_portal( here, "active", center );
}

TEST_CASE( "astral_portal_travel_persists_the_pocket_world", "[astral][dimension][slow]" )
{
    clear_avatar();
    clear_map_without_vision();
    map &here = get_map();
    avatar &u = get_avatar();
    REQUIRE( g->get_dimension_prefix() == dimension_default );

    // Stand on the platform tile in front of the threshold, as a player would.
    const tripoint_bub_ms center = u.pos_bub();
    paint_portal( here, "active", center );
    const tripoint_abs_ms overworld_anchor = u.pos_abs();

    dialogue d( get_talker_for( u ), nullptr );
    effect_on_condition_EOC_ASTRAL_PORTAL_ENTER_DO->activate( d );

    REQUIRE( g->get_dimension_prefix() == dimension_astral_test_world );
    map &there = get_map();
    CHECK( there.ter( u.pos_bub() ).id() == portal_ter( "active", 2, 2 ) );
    const tripoint_abs_ms arrival = u.pos_abs();

    // Leave something behind on the tile south of the arrival pad.
    const tripoint_bub_ms drop = u.pos_bub() + point::south;
    REQUIRE( there.i_at( drop ).empty() );
    there.add_item_or_charges( drop, item( itype_id( "rock" ) ) );
    REQUIRE_FALSE( there.i_at( drop ).empty() );

    dialogue d2( get_talker_for( u ), nullptr );
    effect_on_condition_EOC_ASTRAL_PORTAL_RETURN_DO->activate( d2 );
    REQUIRE( g->get_dimension_prefix() == dimension_default );
    CHECK( u.pos_abs() == overworld_anchor );
    CHECK( get_map().ter( u.pos_bub() ).id() == portal_ter( "active", 2, 2 ) );

    // Re-enter: same world, same landing spot, the rock is still there.
    dialogue d3( get_talker_for( u ), nullptr );
    effect_on_condition_EOC_ASTRAL_PORTAL_ENTER_DO->activate( d3 );
    REQUIRE( g->get_dimension_prefix() == dimension_astral_test_world );
    CHECK( u.pos_abs() == arrival );
    CHECK_FALSE( get_map().i_at( u.pos_bub() + point::south ).empty() );

    // And back, so later tests start on Earth.
    dialogue d4( get_talker_for( u ), nullptr );
    effect_on_condition_EOC_ASTRAL_PORTAL_RETURN_DO->activate( d4 );
    REQUIRE( g->get_dimension_prefix() == dimension_default );
}
