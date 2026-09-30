#pragma once
#ifndef CATA_SRC_WORLDGEN_PREVIEW_H
#define CATA_SRC_WORLDGEN_PREVIEW_H

#include <string>
#include <utility>
#include <vector>

/**
 * Headless map-generation preview (tools/astral/overmap_preview.py):
 * makes a temporary world with the given world-option overrides, generates the
 * overmaps within `radius` of the origin and writes them as a binary PPM, one
 * pixel per overmap tile coloured by terrain class, then deletes the world.
 */
namespace worldgen_preview
{
int run( const std::string &out_path, const std::vector<std::pair<std::string, std::string>> &overrides,
         int radius );
} // namespace worldgen_preview

#endif // CATA_SRC_WORLDGEN_PREVIEW_H
