#if defined(TILES)

#include "pixel_minimap_projectors.h"

#include <algorithm>

// Tiles per axis that cover `pixels` (fill) or fit inside it.
static int tile_extent( int pixels, int tiles, bool fill, int minimum )
{
    const int fit = pixels / tiles;
    const int cover = ( pixels + tiles - 1 ) / tiles;
    return std::max( fill ? cover : fit, minimum );
}

pixel_minimap_ortho_projector::pixel_minimap_ortho_projector(
    const point &total_tiles_count,
    const SDL_Rect &max_screen_rect,
    bool square_pixels, bool fill )
{
    tile_size.x = tile_extent( max_screen_rect.w, total_tiles_count.x, fill, 1 );
    tile_size.y = tile_extent( max_screen_rect.h, total_tiles_count.y, fill, 1 );

    if( square_pixels ) {
        // Fill keeps the larger extent so both axes are covered and the
        // overflow is cropped by the screen rect.
        tile_size.x = tile_size.y = fill ? std::max( tile_size.x, tile_size.y )
                                    : std::min( tile_size.x, tile_size.y );
    }
}

point pixel_minimap_ortho_projector::get_tile_size() const
{
    return tile_size;
}

point pixel_minimap_ortho_projector::get_tiles_size( const point &tiles_count ) const
{
    return {
        tiles_count.x * tile_size.x,
        tiles_count.y *tile_size.y
    };
}

point pixel_minimap_ortho_projector::get_tile_pos( const point &p,
        const point &/*tiles_count*/ ) const
{
    return { p.x * tile_size.x, p.y * tile_size.y };
}

pixel_minimap_iso_projector::pixel_minimap_iso_projector(
    const point &total_tiles_count,
    const SDL_Rect &max_screen_rect,
    bool square_pixels, bool fill ) :

    total_tiles_count( total_tiles_count )
{
    tile_size.x = tile_extent( max_screen_rect.w, 2 * total_tiles_count.x - 1, fill, 2 );
    tile_size.y = tile_extent( max_screen_rect.h, total_tiles_count.y, fill, 2 );

    if( square_pixels ) {
        tile_size.x = tile_size.y = fill ? std::max( tile_size.x, tile_size.y )
                                    : std::min( tile_size.x, tile_size.y );
    }
}

point pixel_minimap_iso_projector::get_tile_size() const
{
    return tile_size;
}

point pixel_minimap_iso_projector::get_tiles_size( const point &tiles_count ) const
{
    return {
        tile_size.x *( 2 * tiles_count.x - 1 ),
        tile_size.y *tiles_count.y
    };
}

point pixel_minimap_iso_projector::get_tile_pos( const point &p, const point &tiles_count ) const
{
    return {
        tile_size.x *( p.x + p.y ),
        tile_size.y *( tiles_count.y + p.y - p.x - 1 ) / 2,
    };
}

#endif // TILES
