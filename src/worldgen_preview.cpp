#include "worldgen_preview.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "cached_options.h"
#include "coordinates.h"
#include "debug.h"
#include "game.h"
#include "mod_manager.h"
#include "omdata.h"
#include "options.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
#include "regional_settings.h"
#include "worldfactory.h"
#include "worldgen_options.h"

namespace
{
struct rgb {
    unsigned char r, g, b;
};

rgb colour_for( const oter_id &oter )
{
    const std::string id = oter->get_type_id().str();
    const auto has = [&]( const char *needle ) {
        return id.find( needle ) != std::string::npos;
    };
    if( oter->is_ocean() || oter->is_ocean_shore() ) {
        return { 20, 40, 110 };
    }
    if( oter->is_lake() || oter->is_lake_shore() ) {
        return { 40, 90, 170 };
    }
    if( oter->is_river() ) {
        return { 70, 130, 210 };
    }
    if( oter->is_ravine() || oter->is_ravine_edge() ) {
        return { 90, 60, 40 };
    }
    if( has( "swamp" ) ) {
        return { 60, 100, 60 };
    }
    if( has( "forest_thick" ) ) {
        return { 20, 70, 20 };
    }
    if( has( "forest_trail" ) ) {
        return { 120, 100, 60 };
    }
    if( has( "forest" ) ) {
        return { 40, 110, 40 };
    }
    if( has( "highway" ) ) {
        return { 240, 220, 60 };
    }
    if( oter->is_road() || has( "road" ) || has( "bridge" ) ) {
        return { 200, 200, 200 };
    }
    if( has( "field" ) ) {
        return { 190, 180, 110 };
    }
    if( has( "house" ) || has( "s_" ) || has( "apartments" ) || has( "office" ) || has( "park" ) ) {
        return { 150, 60, 60 };
    }
    if( has( "farm" ) || has( "orchard" ) || has( "ranch" ) ) {
        return { 210, 140, 60 };
    }
    if( has( "lab" ) || has( "bunker" ) || has( "mil" ) ) {
        return { 220, 60, 220 };
    }
    // anything else: a special or unusual terrain
    return { 240, 240, 240 };
}
} // namespace

int worldgen_preview::run( const std::string &out_path,
                           const std::vector<std::pair<std::string, std::string>> &overrides, int radius )
{
    worldgen_options::set_preview_mode( true );
    // Debug messages go to the log instead of a blocking prompt (an extreme
    // setting can legitimately make a generator step give up).
    test_mode = true;
    world_generator->init();
    WORLD *world = world_generator->make_new_world( world_generator->get_mod_manager().get_default_mods() );
    if( world == nullptr ) {
        std::fprintf( stderr, "preview: could not create a temporary world\n" );
        return 2;
    }
    for( const auto &kv : overrides ) {
        auto it = world->WORLD_OPTIONS.find( kv.first );
        if( it == world->WORLD_OPTIONS.end() ) {
            std::fprintf( stderr, "preview: unknown world option %s\n", kv.first.c_str() );
            continue;
        }
        it->second.setValue( kv.second );
    }
    world->save();
    world_generator->set_active_world( world );
    try {
        g->setup();
    } catch( const std::exception &err ) {
        std::fprintf( stderr, "preview: setup failed: %s\n", err.what() );
        return 3;
    }
    const int side = 2 * radius + 1;
    const int width = side * OMAPX;
    const int height = side * OMAPY;
    std::vector<rgb> pixels( static_cast<size_t>( width ) * height, rgb{ 0, 0, 0 } );
    for( int oy = -radius; oy <= radius; ++oy ) {
        for( int ox = -radius; ox <= radius; ++ox ) {
            const overmap &om = overmap_buffer.get( point_abs_om( ox, oy ) );
            for( int y = 0; y < OMAPY; ++y ) {
                for( int x = 0; x < OMAPX; ++x ) {
                    const oter_id &t = om.ter( tripoint_om_omt( x, y, 0 ) );
                    const size_t px = static_cast<size_t>( ( oy + radius ) * OMAPY + y ) * width +
                                      ( ox + radius ) * OMAPX + x;
                    pixels[px] = colour_for( t );
                }
            }
            std::fprintf( stderr, "preview: overmap %d,%d cities=%zu\n", ox, oy, om.cities.size() );
        }
    }
    std::ofstream out( out_path, std::ios::binary );
    out << "P6\n" << width << " " << height << "\n255\n";
    out.write( reinterpret_cast<const char *>( pixels.data() ), pixels.size() * 3 );
    out.close();
    std::fprintf( stderr, "preview: wrote %s (%dx%d)\n", out_path.c_str(), width, height );
    const std::string name = world->world_name;
    overmap_buffer.clear();
    world_generator->set_active_world( nullptr );
    world_generator->delete_world( name, true );
    return 0;
}
