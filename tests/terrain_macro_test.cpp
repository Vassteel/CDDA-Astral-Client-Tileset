#include "cata_catch.h"
#include "terrain_macro.h"

#include <climits>
#include <numeric>

TEST_CASE( "terrain_macro_world_phase", "[terrain_macro]" )
{
    for( int size : {
             1, 2, 4, 8, 16
         } ) {
        terrain_macro macro;
        macro.size = size;
        macro.variants.emplace_back( size * size );
        std::iota( macro.variants.front().begin(), macro.variants.front().end(), 0 );
        for( int y = -49; y <= 49; ++y ) {
            for( int x = -49; x <= 49; ++x ) {
                const int expected = ( ( y % size + size ) % size ) * size +
                                     ( x % size + size ) % size;
                CHECK( macro.sprite_at( x, y, 0 ) == expected );
                CHECK( macro.sprite_at( x + size, y - size, 0 ) == expected );
            }
        }
        CHECK( macro.sprite_at( INT_MIN, INT_MIN, 0 ) >= 0 );
    }
}

TEST_CASE( "terrain_macro_variants_share_block_and_keep_phase", "[terrain_macro]" )
{
    terrain_macro macro;
    macro.size = 8;
    for( int v = 0; v < 3; ++v ) {
        macro.variants.emplace_back( 64 );
        std::iota( macro.variants.back().begin(), macro.variants.back().end(), v * 64 );
    }
    for( int by = -4; by <= 4; ++by ) {
        for( int bx = -4; bx <= 4; ++bx ) {
            const int variant = macro.sprite_at( bx * 8, by * 8, 0 ) / 64;
            for( int y = 0; y < 8; ++y ) {
                for( int x = 0; x < 8; ++x ) {
                    CHECK( macro.sprite_at( bx * 8 + x, by * 8 + y, 0 ) == variant * 64 + y * 8 + x );
                }
            }
        }
    }
    macro.size = 12;
    CHECK( macro.sprite_at( 0, 0, 0 ) == -1 );
    macro.size = 17;
    CHECK( macro.sprite_at( 0, 0, 0 ) == -1 );
}
