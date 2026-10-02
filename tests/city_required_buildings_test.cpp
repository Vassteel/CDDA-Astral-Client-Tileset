#include <string>

#include "cata_catch.h"
#include "city.h"
#include "coordinates.h"
#include "omdata.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"

// Every town on a default Earth overmap gets one Astral guild building
// (region_settings_city.required_buildings), sized by the town.
TEST_CASE( "towns_get_their_required_guild_building", "[overmap][required_buildings][slow]" )
{
    overmap_buffer.clear();
    int towns = 0;
    int with_guild = 0;
    for( const point_abs_om &addr : { point_abs_om( 0, 0 ), point_abs_om( 1, 0 ), point_abs_om( 0, 1 ) } ) {
        overmap &om = overmap_buffer.get( addr );
        for( const city &c : om.cities ) {
            if( c.size < 2 ) {
                continue;
            }
            towns++;
            int guilds = 0;
            for( int dx = -c.size - 2; dx <= c.size + 2; ++dx ) {
                for( int dy = -c.size - 2; dy <= c.size + 2; ++dy ) {
                    const tripoint_om_omt p( c.pos.x() + dx, c.pos.y() + dy, 0 );
                    if( !overmap::inbounds( p ) ) {
                        continue;
                    }
                    const std::string ot = om.ter( p )->get_type_id().str();
                    if( ot.rfind( "astral_guild_", 0 ) == 0 ) {
                        guilds++;
                    }
                }
            }
            if( guilds > 0 ) {
                with_guild++;
            }
        }
    }
    INFO( "towns " << towns << ", with a guild building " << with_guild );
    REQUIRE( towns > 0 );
    // Cramped towns may have no free lot at all; nearly every town must have one.
    CHECK( with_guild * 10 >= towns * 9 );
}
