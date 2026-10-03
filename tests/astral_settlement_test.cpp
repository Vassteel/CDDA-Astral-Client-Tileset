// Project Astral: settlement grammar tests.
//
// Places the canal city mutable special many times on the placement test's blank overmap
// (like "mutable_overmap_placement" in overmap_test.cpp) and then, on the real overmap buffer,
// force-places one city and runs mapgen for every tile it occupies so that palette symbols,
// nested chunk ids and join conditions are all exercised.  Any debugmsg is a failure.

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "city.h"
#include "coordinates.h"
#include "debug.h"
#include "global_vars.h"
#include "map.h"
#include "mapbuffer.h"
#include "omdata.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
#include "rng.h"
#include "type_id.h"

static const overmap_special_id overmap_special_astral_canal_city( "astral_canal_city" );

TEST_CASE( "astral_canal_city_mutable_placement", "[overmap][astral][slow]" )
{
    const overmap_special &special = *overmap_special_astral_canal_city;
    const city cit;

    constexpr int num_overmaps = 20;
    constexpr int num_trials_per_overmap = 50;

    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    int total_successes = 0;
    for( int j = 0; j < num_overmaps; ++j ) {
        std::unique_ptr<overmap> om = std::make_unique<overmap>( point_abs_om::zero );
        om_direction::type dir = om_direction::type::north;

        for( int i = 0; i < num_trials_per_overmap; ++i ) {
            tripoint_om_omt try_pos( rng( 8, OMAPX - 9 ), rng( 8, OMAPY - 9 ), 0 );
            if( debug_has_error_been_observed() ) {
                return;
            }
            if( om->can_place_special( special, try_pos, dir, false ) ) {
                std::vector<tripoint_om_omt> placed_points =
                    om->place_special( special, try_pos, dir, cit, false, false );
                CHECK( !placed_points.empty() );
                // footprint sanity: the grammar is tuned for 8-13 OMT across
                int minx = OMAPX;
                int maxx = 0;
                int miny = OMAPY;
                int maxy = 0;
                for( const tripoint_om_omt &p : placed_points ) {
                    minx = std::min( minx, p.x() );
                    maxx = std::max( maxx, p.x() );
                    miny = std::min( miny, p.y() );
                    maxy = std::max( maxy, p.y() );
                }
                CAPTURE( placed_points.size() );
                CHECK( placed_points.size() >= 40 );
                CHECK( std::max( maxx - minx, maxy - miny ) + 1 >= 8 );
                ++total_successes;
            }
        }
    }
    CHECK( total_successes > 0 );
    CHECK( !debug_has_error_been_observed() );
}

TEST_CASE( "astral_canal_city_mapgen", "[mapgen][astral][slow]" )
{
    const overmap_special &special = *overmap_special_astral_canal_city;
    // somewhere in the middle of the origin overmap, away from the reality bubble
    const tripoint_abs_omt origin( 90, 90, 0 );

    std::optional<std::vector<tripoint_abs_omt>> placed;
    const std::string place_msg = capture_debugmsg_during( [&]() {
        placed = overmap_buffer.place_special( special, origin, om_direction::type::north, false,
                                               true );
    } );
    CHECK( place_msg.empty() );
    REQUIRE( placed.has_value() );
    REQUIRE( !placed->empty() );

    for( const tripoint_abs_omt &pos : *placed ) {
        const std::string oter = overmap_buffer.ter( pos ).id().str();
        CAPTURE( oter );
        const std::string msg = capture_debugmsg_during( [pos]() {
            MAPBUFFER.clear_outside_reality_bubble();
            smallmap tm;
            tm.generate( pos, calendar::turn, false );
            tm.delete_unmerged_submaps();
        } );
        CHECK( msg.empty() );
    }
}
