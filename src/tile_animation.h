#pragma once
#ifndef CATA_SRC_TILE_ANIMATION_H
#define CATA_SRC_TILE_ANIMATION_H

#include <cstdint>

// Tile weights are durations in 17 ms ticks. A shared tick and no position phase
// keep adjacent pieces of one animated object on the same frame.
inline unsigned int tile_animation_index( uint64_t tick, unsigned int position_phase,
        int loop_ticks, bool synchronized )
{
    if( loop_ticks <= 0 ) {
        return 0;
    }
    return ( tick + ( synchronized ? 0 : position_phase ) ) % loop_ticks;
}

#endif // CATA_SRC_TILE_ANIMATION_H
