#include <string>
#include <utility>

#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "coordinates.h"
#include "json.h"
#include "json_loader.h"
#include "omdata.h"
#include "options_helpers.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
#include "rng.h"
#include "scenario.h"
#include "start_location.h"
#include "type_id.h"
#include "worldgen_options.h"

static const start_location_id start_location_sloc_ocean_shore( "sloc_ocean_shore" );
static const string_id<scenario> scenario_beach_day( "beach_day" );

TEST_CASE( "start_location_tries_other_allowed_terrains", "[start_location][worldgen]" )
{
    // One allowed site is absent; a different valid site is in the origin
    // overmap. Every random ordering must find it without searching farther,
    // and return that site's parameters rather than the missing site's.
    restore_on_out_of_scope restore_rng( rng_get_engine() );
    const point_abs_om origin;
    overmap &omap = overmap_buffer.get( origin );
    const tripoint_om_omt marker( 17, 19, 0 );
    const oter_id previous = omap.ter( marker );
    on_out_of_scope restore_terrain( [&]() {
        omap.ter_set( marker, previous );
    } );
    omap.ter_set( marker, oter_str_id( "field" ).id() );
    const JsonValue json = json_loader::from_string( R"({
        "name": "Missing site with a valid alternate",
        "terrain": [
            { "om_terrain": "absent_start_location_test_site", "parameters": { "test": "missing" } },
            { "om_terrain": "field", "parameters": { "test": "field" } }
        ]
    })" );
    start_location location;
    location.load( json.get_object(), "test" );
    for( int seed = 1; seed <= 8; ++seed ) {
        rng_set_engine_seed( seed );
        const auto result = location.find_player_initial_location( origin );
        REQUIRE_FALSE( result.first.is_invalid() );
        CHECK( project_to<coords::om>( result.first.xy() ) == origin );
        CHECK( overmap_buffer.ter( result.first ) == oter_str_id( "field" ).id() );
        REQUIRE( result.second.count( "test" ) == 1 );
        CHECK( result.second.at( "test" ) == "field" );
    }
}

TEST_CASE( "middle_of_nowhere_start_with_sparse_settlements", "[start_location][worldgen]" )
{
    on_out_of_scope reset_overrides( []() {
        overmap_buffer.clear();
        worldgen_options::apply_region_overrides();
    } );
    override_option city_density( "WG_CITY_DENSITY", "25" );
    override_option city_size( "WG_CITY_SIZE", "4" );
    override_option city_spacing( "WG_CITY_SPACING", "8" );
    override_option road_density( "WG_ROAD_DENSITY", "25" );
    override_option rural_density( "WG_RURAL_DENSITY", "50" );
    override_option forest_density( "WG_FOREST_DENSITY", "40" );
    worldgen_options::apply_region_overrides();
    overmap_buffer.clear();
    const start_location &location = start_location_id( "sloc_middle_of_nowhere" ).obj();
    const auto result = location.find_player_initial_location( point_abs_om::zero );
    REQUIRE_FALSE( result.first.is_invalid() );
    const oter_id terrain = overmap_buffer.ter( result.first );
    INFO( terrain.id().str() );
    CHECK( ( is_ot_match( "state_park_1_0", terrain, ot_match_type::type ) ||
             is_ot_match( "sewage_treatment_1_0_0", terrain, ot_match_type::type ) ||
             is_ot_match( "pwr_sub_s", terrain, ot_match_type::type ) ||
             is_ot_match( "derelict_property", terrain, ot_match_type::type ) ||
             is_ot_match( "ws_regional_dump_2_0", terrain, ot_match_type::type ) ||
             is_ot_match( "waiting_area", terrain, ot_match_type::type ) ) );
}

TEST_CASE( "Test_origin_offset" )
{
    const point_rel_om &offset = scenario_beach_day.obj().get_origin_offset();
    const start_location &beach = start_location_sloc_ocean_shore.obj();
    const point_abs_om origin = point_abs_om::zero + offset;

    tripoint_abs_omt omtstart = tripoint_abs_omt::invalid;

    auto ret = beach.find_player_initial_location( origin );
    omtstart = ret.first;

    CHECK( !omtstart.is_invalid() );
}
