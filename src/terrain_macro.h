#pragma once
#ifndef CATA_SRC_TERRAIN_MACRO_H
#define CATA_SRC_TERRAIN_MACRO_H
#include <cstddef>
#include <cstdint>
#include <vector>

// Pure world-coordinate lookup, independent of camera and reality-bubble origin.
struct terrain_macro {
    int size = 0;
    std::vector<std::vector<int>> variants;

    static bool valid_size( int n ) {
        return n >= 1 && n <= 16 && n != 12;
    }
    int sprite_at( int x, int y, int z ) const {
        if( !valid_size( size ) || variants.empty() ) {
            return -1;
        }
        const int sx = ( x % size + size ) % size;
        const int sy = ( y % size + size ) % size;
        // Widen before subtracting, including INT_MIN coordinates.
        const int64_t bx = ( static_cast<int64_t>( x ) - sx ) / size;
        const int64_t by = ( static_cast<int64_t>( y ) - sy ) / size;
        uint64_t hash = static_cast<uint64_t>( bx ) * 0x9e3779b97f4a7c15ULL;
        hash ^= static_cast<uint64_t>( by ) * 0xbf58476d1ce4e5b9ULL;
        hash ^= static_cast<uint64_t>( z ) * 0x94d049bb133111ebULL;
        hash ^= hash >> 30;
        hash *= 0xbf58476d1ce4e5b9ULL;
        hash ^= hash >> 27;
        const auto &cells = variants[hash % variants.size()];
        const int index = sy * size + sx;
        return cells.size() == static_cast<size_t>( size * size ) ? cells[index] : -1;
    }
};
#endif
