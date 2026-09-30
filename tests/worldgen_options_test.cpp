#include <sstream>
#include <string>

#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "json.h"
#include "json_loader.h"
#include "options.h"
#include "options_helpers.h"
#include "regional_settings.h"
#include "worldfactory.h"
#include "worldgen_options.h"

TEST_CASE( "worldgen_options_preserve_no_cities_region", "[worldgen][crash_regression]" )
{
    REQUIRE( DEFAULT_REGION.is_valid() );
    const region_settings_city_id no_cities( "no_cities" );
    REQUIRE( no_cities.is_valid() );
    region_settings &region = const_cast<region_settings &>( DEFAULT_REGION.obj() );
    region_settings_city &city = const_cast<region_settings_city &>( no_cities.obj() );
    on_out_of_scope reset_overrides( []() {
        worldgen_options::on_data_reset();
        worldgen_options::apply_region_overrides();
    } );
    restore_on_out_of_scope restore_region( region.city_spec );
    restore_on_out_of_scope restore_city( city );
    region.city_spec = no_cities;
    REQUIRE( city.city_size == 0 );
    REQUIRE( city.houses.buildings.empty() );
    worldgen_options::on_data_reset();
    override_option density( "WG_CITY_DENSITY", "10" );
    override_option size( "WG_CITY_SIZE", "3" );
    override_option spacing( "WG_CITY_SPACING", "8" );
    worldgen_options::apply_region_overrides();
    CHECK( city.city_size == 0 );
    CHECK( city.city_spacing == 0 );
    worldgen_options::apply_region_overrides();
    CHECK( city.city_size == 0 );
}

TEST_CASE( "empty_finalized_building_bin_has_no_selection", "[worldgen][crash_regression]" )
{
    building_bin empty;
    empty.finalize();
    CHECK( empty.pick().is_null() );
    const auto &houses = DEFAULT_REGION->get_settings_city().houses;
    REQUIRE_FALSE( houses.buildings.empty() );
    CHECK( houses.pick().is_valid() );
}

// The map-generation options live on the world_default page, so they travel
// with the world through worldoptions.json like every other world option.
TEST_CASE( "worldgen_options_round_trip_through_world_options", "[worldgen][option]" )
{
    WORLD world;
    world.world_name = "wg_option_round_trip";
    REQUIRE( world.WORLD_OPTIONS.count( "WG_FOREST_DENSITY" ) == 1 );
    REQUIRE( world.WORLD_OPTIONS.count( "WG_POCKET_ROUTE_SCALE" ) == 1 );
    world.WORLD_OPTIONS["WG_FOREST_DENSITY"].setValue( 250 );
    world.WORLD_OPTIONS["WG_HIGHWAYS"].setValue( "false" );
    world.WORLD_OPTIONS["WG_POCKET_ROUTE_SCALE"].setValue( "vast" );

    // Serialize the same way WORLD::save() does, then load it into a fresh world.
    std::ostringstream out;
    JsonOut jout( out );
    jout.start_array();
    for( const auto &elem : world.WORLD_OPTIONS ) {
        if( !elem.second.getDefaultText().empty() ) {
            jout.start_object();
            jout.member( "name", elem.first );
            jout.member( "value", elem.second.getValue( true ) );
            jout.end_object();
        }
    }
    jout.end_array();

    WORLD loaded;
    JsonValue jv = json_loader::from_string( out.str() );
    loaded.load_options( jv.get_array() );
    CHECK( loaded.WORLD_OPTIONS["WG_FOREST_DENSITY"].value_as<int>() == 250 );
    CHECK( loaded.WORLD_OPTIONS["WG_HIGHWAYS"].value_as<bool>() == false );
    CHECK( loaded.WORLD_OPTIONS["WG_POCKET_ROUTE_SCALE"].getValue() == "vast" );

    // A world saved before the options existed gets the defaults (vanilla behaviour).
    WORLD old_world;
    JsonValue empty = json_loader::from_string( "[]" );
    old_world.load_options( empty.get_array() );
    CHECK( old_world.WORLD_OPTIONS["WG_FOREST_DENSITY"].value_as<int>() == 100 );
    CHECK( old_world.WORLD_OPTIONS["WG_HIGHWAYS"].value_as<bool>() == true );
}

// The overrides are applied to the region data at finalize; applying them
// once from the active options must change the loaded values exactly once
// (the factories reload from JSON per data load, so a second application in
// the same load must not compound).
TEST_CASE( "worldgen_options_region_override_applies_once", "[worldgen][option]" )
{
    REQUIRE( DEFAULT_REGION.is_valid() );
    const region_settings &region = DEFAULT_REGION.obj();
    REQUIRE( region.overmap_forest.has_value() );
    const double base_forest = region.overmap_forest->obj().noise_threshold_forest;
    const int base_city = region.city_spec->obj().city_size;

    {
        override_option forest( "WG_FOREST_DENSITY", "200" );
        override_option city( "WG_CITY_SIZE", "12" );
        worldgen_options::apply_region_overrides();
        CHECK( region.overmap_forest->obj().noise_threshold_forest == Approx( base_forest / 2.0 ) );
        CHECK( region.city_spec->obj().city_size == 12 );
        // Applying again in the same data load starts from the captured base:
        // the result is the same as applying once.
        const double once = region.overmap_forest->obj().noise_threshold_forest;
        worldgen_options::apply_region_overrides();
        CHECK( region.overmap_forest->obj().noise_threshold_forest == Approx( once ) );
    }
    // Back to the defaults: the base values come back.
    worldgen_options::apply_region_overrides();
    CHECK( region.overmap_forest->obj().noise_threshold_forest == Approx( base_forest ) );
    CHECK( region.city_spec->obj().city_size == base_city );
}
