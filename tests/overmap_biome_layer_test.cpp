#include <cmath>
#include <string>

#include "cata_catch.h"
#include "coordinates.h"
#include "overmap_noise.h"
#include "regional_settings.h"

// The data-defined biome noise must reproduce vanilla forest noise with its
// default parameters, so a layer that copies the defaults paints the same
// shapes the overworld already uses.
TEST_CASE( "overmap_biome_layer_defaults_match_forest_noise", "[overmap][biome_layer]" )
{
    const overmap_biome_layer layer;
    const point_abs_omt base( 180 * 3, -180 * 2 );
    const unsigned seed = 1234567;
    const om_noise::om_noise_layer_forest forest( base, seed );
    const om_noise::om_noise_layer_biome biome( base, seed, layer );
    for( int x = 0; x < 180; x += 7 ) {
        for( int y = 0; y < 180; y += 11 ) {
            const point_om_omt p( x, y );
            CHECK( biome.noise_at( p ) == Approx( forest.noise_at( p ) ).margin( 1e-6 ) );
        }
    }
}

TEST_CASE( "overmap_biome_layer_noise_is_bounded_and_seeded", "[overmap][biome_layer]" )
{
    overmap_biome_layer layer;
    layer.scale = 0.05f;
    layer.power = 1.5f;
    const point_abs_omt base( 0, 0 );
    const om_noise::om_noise_layer_biome a( base, 42, layer );
    const om_noise::om_noise_layer_biome b( base, 42 + 9001, layer );
    int differing = 0;
    for( int x = 0; x < 180; x += 5 ) {
        for( int y = 0; y < 180; y += 5 ) {
            const point_om_omt p( x, y );
            const float na = a.noise_at( p );
            CHECK( na >= 0.0f );
            CHECK( na <= 1.0f );
            if( std::fabs( na - b.noise_at( p ) ) > 1e-4f ) {
                differing++;
            }
        }
    }
    // A different seed offset gives a different pattern.
    CHECK( differing > 100 );
}

TEST_CASE( "overmap_biome_layer_bands_sort_highest_first", "[overmap][biome_layer]" )
{
    overmap_biome_layer layer;
    layer.terrains = {
        { 0.25f, oter_str_id( "forest" ) },
        { 0.4f, oter_str_id( "forest_water" ) },
        { 0.3f, oter_str_id( "forest_thick" ) },
    };
    layer.finalize();
    REQUIRE( layer.terrains.size() == 3 );
    CHECK( layer.terrains[0].first == Approx( 0.4f ) );
    CHECK( layer.terrains[1].first == Approx( 0.3f ) );
    CHECK( layer.terrains[2].first == Approx( 0.25f ) );
}

// The pocket biome regions declare dominant-biome mixing and every region they
// name exists, so the 15 % intrusions can be painted.
TEST_CASE( "astral_biome_regions_declare_biome_mix", "[overmap][biome_layer][biome_mix]" )
{
    for( const std::string &rid : { "astral_biome_meadows", "astral_biome_lowlands",
                                    "astral_biome_fungal", "astral_biome_rootland" } ) {
        const region_settings_id id( rid );
        REQUIRE( id.is_valid() );
        const region_biome_mix &mix = id->biome_mix;
        INFO( rid );
        CHECK( mix.enabled() );
        CHECK( mix.dominant_share == Approx( 0.85f ) );
        REQUIRE( !mix.regions.empty() );
        for( const region_biome_mix_entry &e : mix.regions ) {
            CHECK( e.region.is_valid() );
            CHECK( e.region != id );
            CHECK( e.weight > 0 );
        }
    }
    // A region without the field has mixing off.
    CHECK_FALSE( region_settings_id( "default" )->biome_mix.enabled() );
}
