#pragma once
#ifndef CATA_SRC_WORLDGEN_OPTIONS_H
#define CATA_SRC_WORLDGEN_OPTIONS_H

#include <string>

#include "options.h"

class options_manager;
class overmap_special;

/**
 * World-creation map-generation options ("Map generation" section of the
 * World options tab).
 *
 * Every option lives on the `world_default` options page under a WG_ prefix,
 * so it is stored per world in worldoptions.json like the difficulty values.
 * The values are read once when a world's data is loaded (region settings
 * finalize) and cached here; the generators read the cache. Worlds created
 * before an option existed carry no value for it and get the default, which is
 * always "what vanilla did", so nothing changes retroactively.
 *
 * Region settings are JSON data shared by every world in the process; the
 * overrides below are applied to the region sub-settings that the `default`
 * region references (city, forest, river, lake, ocean, ravine and the region's
 * NPC spawn time) each time data is loaded for a world.
 */
namespace worldgen_options
{

struct values {
    // Settlement
    float city_density = 1.f;      // multiplier on the number of cities per overmap
    int city_size = 8;             // region_settings_city::city_size
    int city_spacing = 4;          // region_settings_city::city_spacing
    float road_density = 1.f;      // border exits and extra inter-city road nodes
    bool highways = true;
    float rural_density = 1.f;     // specials flagged WILDERNESS / away from cities
    // Water
    float rivers = 1.f;            // 0 = none; otherwise reshapes river_frequency
    float river_width = 1.f;       // river_scale multiplier
    float creeks = 1.f;            // river branch chance multiplier
    float lakes = 1.f;             // 0 = none; noise threshold divisor
    float lake_size = 1.f;         // lake_size_min multiplier
    bool ocean = true;
    float ocean_distance = 1.f;    // ocean_start_* multiplier
    float swamps = 1.f;            // 0 = none; swamp threshold divisor
    // Land
    float forest = 1.f;            // 0 = none; forest threshold divisor
    float forest_limit = 1.f;      // forest_threshold_limit multiplier (forest vs plains cap)
    float forest_clumping = 1.f;   // noise feature size multiplier
    int ravines = 0;               // region_settings_ravine::num_ravines
    float fields = 1.f;            // specials flagged FARM
    // Spawns
    float hordes = 1.f;            // city horde scalar multiplier; 0 = no city hordes
    bool wander_spawns = true;     // hordes move between overmaps
    float npc_density = 1.f;       // 0 = no random NPCs
    float wildlife = 1.f;          // animal spawn multiplier
    // Loot
    float vehicles = 1.f;          // vehicle placement chance multiplier
    float fuel = 1.f;              // vehicle fuel at spawn multiplier
    float food = 1.f;              // comestible spawn multiplier
    // Specials
    float specials = 1.f;          // every overmap special's occurrences
    // Astral (consumed by the portal / pocket-world branches; see TODO markers)
    int portal_sites = 2;          // astral_portal_site occurrences per overmap
    std::string pocket_route_scale = "normal";
    std::string pocket_size_cap = "unbounded";
    int pocket_max_active = 8;
};

// add_options( options_manager & ) is declared in options.h (friend of options_manager):
// it registers the options on the world_default page.

/** Re-read the active world's options into the cache. */
void refresh();

/** Cached values for the active world (refresh() decides when they change). */
const values &get();

/**
 * Apply the overrides to the region settings the `default` region references.
 * Called from region_settings::finalize_all() after the factories finalized;
 * idempotent per data load because the factories reload from JSON each time.
 */
void apply_region_overrides();
/** Region data was reset (a new data load starts): forget the captured base values. */
void on_data_reset();

/**
 * Occurrence multiplier for an overmap special: the global factor times the
 * category factors that apply to it (FARM → fields, WILDERNESS / rural → rural).
 * Designed so a per-faction table (LAB, MILITARY, MI-GO, FUNGAL, ...) can be
 * added by extending category_factor() without touching the callers.
 */
float specials_factor( const overmap_special &special );

/** True while a preview/headless run wants generation without a game. */
bool preview_mode();
void set_preview_mode( bool on );

} // namespace worldgen_options

#endif // CATA_SRC_WORLDGEN_OPTIONS_H
