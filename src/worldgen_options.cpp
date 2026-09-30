#include "worldgen_options.h"

#include <algorithm>
#include <cmath>
#include <optional>

#include "omdata.h"
#include "options.h"
#include "regional_settings.h"
#include "translations.h"

namespace worldgen_options
{

namespace
{
values g_values;
bool g_preview = false;

float pct( const std::string &name )
{
    return get_option<int>( name ) / 100.f;
}

// Lower a noise threshold to get more of a feature, raise it to get less;
// a factor of 0 disables the feature outright (threshold above any noise).
double scaled_threshold( double base, float factor )
{
    if( factor <= 0.f ) {
        return 10.0;
    }
    return base / factor;
}
} // namespace

bool preview_mode()
{
    return g_preview;
}

void set_preview_mode( bool on )
{
    g_preview = on;
}

void add_options( options_manager &mgr )
{
    const std::string page = "world_default";
    const auto pct_opt = [&]( const char *name, const char *text, const char *tip, int lo, int hi ) {
        mgr.add( name, page, to_translation( text ), to_translation( tip ), lo, hi, 100,
                 options_manager::COPT_NO_HIDE, "%i%%" );
    };

    mgr.add_option_group( page, options_manager::Group( "wg_settlement", to_translation( "Map generation: settlements" ),
                          to_translation( "Cities, roads and the buildings between them.  100% is what the game did before these options existed." ) ),
    [&]( const std::string & p ) {
        pct_opt( "WG_CITY_DENSITY", "City density",
                 "How many cities are seeded per overmap.  0% removes cities (roads and city-bound buildings go with them).",
                 0, 400 );
        mgr.add( "WG_CITY_SIZE", p, to_translation( "City size" ),
                 to_translation( "Base radius of cities in overmap tiles.  Urban and forested areas still adjust it locally." ),
                 1, 16, 8 );
        mgr.add( "WG_CITY_SPACING", p, to_translation( "City spacing" ),
                 to_translation( "How far apart cities keep from each other; each step halves how much of the map cities cover." ),
                 0, 8, 4 );
        pct_opt( "WG_ROAD_DENSITY", "Road density",
                 "How many roads leave each overmap and how many extra junctions connect the countryside.  0% keeps only the streets inside cities.",
                 0, 300 );
        mgr.add( "WG_HIGHWAYS", p, to_translation( "Highways" ),
                 to_translation( "Generate the highway network." ), true );
        pct_opt( "WG_RURAL_DENSITY", "Rural buildings",
                 "Farms, cabins, outposts, roadside sites and other wilderness buildings (specials outside cities).",
                 0, 400 );
    } );

    mgr.add_option_group( page, options_manager::Group( "wg_water", to_translation( "Map generation: water" ),
                          to_translation( "Rivers, creeks, lakes, oceans and swamps." ) ),
    [&]( const std::string & p ) {
        pct_opt( "WG_RIVERS", "Rivers",
                 "How likely each overmap is to start a major river.  0% removes rivers (and the creeks and swamps that follow them).",
                 0, 400 );
        pct_opt( "WG_RIVER_WIDTH", "River width", "Width of major rivers.", 50, 400 );
        pct_opt( "WG_CREEKS", "Creeks", "How often rivers branch into creeks.", 0, 400 );
        pct_opt( "WG_LAKES", "Lakes", "How much of the land the lake noise claims.  0% removes lakes.", 0, 400 );
        pct_opt( "WG_LAKE_SIZE", "Lake size (minimum)",
                 "Smallest lake that is kept; ponds below it are filled in.  Higher keeps only big lakes.", 25, 400 );
        mgr.add( "WG_OCEAN", p, to_translation( "Ocean" ),
                 to_translation( "Generate the ocean at the edges the region defines." ), true );
        pct_opt( "WG_OCEAN_DISTANCE", "Ocean distance",
                 "Distance from the start to the ocean on every side the region has one.", 25, 400 );
        pct_opt( "WG_SWAMPS", "Swamps", "Swamp along rivers and in wet hollows.  0% removes swamps.", 0, 400 );
    } );

    mgr.add_option_group( page, options_manager::Group( "wg_land", to_translation( "Map generation: land" ),
                          to_translation( "Forests, ravines, farmland." ) ),
    [&]( const std::string & p ) {
        pct_opt( "WG_FOREST_DENSITY", "Forest density",
                 "How much of the land the forest noise claims.  0% removes forests.", 0, 400 );
        pct_opt( "WG_FOREST_LIMIT", "Forest vs plains",
                 "Cap on how forested a region may become as you travel; higher lets whole overmaps turn to woods.",
                 25, 250 );
        pct_opt( "WG_FOREST_CLUMPING", "Forest clumping",
                 "Size of the forest patches: lower gives many small woods, higher a few vast ones.", 25, 400 );
        mgr.add( "WG_RAVINES", p, to_translation( "Ravines" ),
                 to_translation( "Number of ravines cut through each overmap." ), 0, 10, 0 );
        pct_opt( "WG_FIELDS", "Farms and orchards", "Farmland specials (farms, orchards, ranches).", 0, 400 );
    } );

    mgr.add_option_group( page, options_manager::Group( "wg_spawns", to_translation( "Map generation: spawns" ),
                          to_translation( "Monsters, hordes, NPCs and wildlife." ) ),
    [&]( const std::string & p ) {
        pct_opt( "WG_HORDES", "City hordes",
                 "Size of the zombie hordes seeded in cities.  0% seeds none.", 0, 400 );
        mgr.add( "WG_WANDER_SPAWNS", p, to_translation( "Wandering hordes" ),
                 to_translation( "Hordes drift across the map over time.  Off keeps them where they spawned." ), true );
        pct_opt( "WG_NPC_DENSITY", "NPC density",
                 "How often random NPCs appear near you.  0% stops random NPC spawns.", 0, 400 );
        pct_opt( "WG_WILDLIFE", "Wildlife", "Animal spawns.", 0, 400 );
    } );

    mgr.add_option_group( page, options_manager::Group( "wg_loot", to_translation( "Map generation: loot" ),
                          to_translation( "Items, vehicles, fuel and food." ) ),
    [&]( const std::string & p ) {
        ( void ) p;
        pct_opt( "WG_VEHICLES", "Vehicle wrecks", "How often vehicles are placed by map generation.", 0, 400 );
        pct_opt( "WG_FUEL", "Fuel in vehicles", "Fuel left in spawned vehicles.", 0, 400 );
        pct_opt( "WG_FOOD", "Food", "How much food spawns.  Applied on top of the item spawn rate.", 0, 400 );
    } );

    mgr.add_option_group( page, options_manager::Group( "wg_specials", to_translation( "Map generation: special sites" ),
                          to_translation( "Labs, bunkers, military sites and every other overmap special." ) ),
    [&]( const std::string & p ) {
        ( void ) p;
        pct_opt( "WG_SPECIALS", "Special sites",
                 "Multiplier on how many of every special site an overmap may hold.  Unique story sites are not affected.",
                 0, 400 );
    } );

    mgr.add_option_group( page, options_manager::Group( "wg_astral", to_translation( "Map generation: Astral" ),
                          to_translation( "Portal sites and pocket worlds." ) ),
    [&]( const std::string & p ) {
        mgr.add( "WG_PORTAL_SITES", p, to_translation( "Portal sites" ),
                 to_translation( "Overworld portal sites per overmap.  0 = none." ), 0, 20, 2 );
        mgr.add( "WG_POCKET_ROUTE_SCALE", p, to_translation( "Pocket-world route scale" ),
        to_translation( "How long the route through a pocket world is." ), {
            { "test", to_translation( "Test" ) }, { "small", to_translation( "Small" ) },
            { "normal", to_translation( "Normal" ) }, { "vast", to_translation( "Vast" ) }
        }, "normal" );
        mgr.add( "WG_POCKET_SIZE_CAP", p, to_translation( "Pocket-world size cap" ),
        to_translation( "Bounded pocket worlds stop generating past their route; unbounded ones grow as you explore." ), {
            { "unbounded", to_translation( "Unbounded" ) }, { "bounded", to_translation( "Bounded" ) }
        }, "unbounded" );
        mgr.add( "WG_POCKET_MAX_ACTIVE", p, to_translation( "Maximum active pocket worlds" ),
                 to_translation( "Size of the pocket-world instance pool." ), 1, 32, 8 );
    } );
}

void refresh()
{
    values v;
    const options_manager &o = get_options();
    if( !o.has_option( "WG_CITY_DENSITY" ) ) {
        g_values = v;
        return;
    }
    v.city_density = pct( "WG_CITY_DENSITY" );
    v.city_size = get_option<int>( "WG_CITY_SIZE" );
    v.city_spacing = get_option<int>( "WG_CITY_SPACING" );
    v.road_density = pct( "WG_ROAD_DENSITY" );
    v.highways = get_option<bool>( "WG_HIGHWAYS" );
    v.rural_density = pct( "WG_RURAL_DENSITY" );
    v.rivers = pct( "WG_RIVERS" );
    v.river_width = pct( "WG_RIVER_WIDTH" );
    v.creeks = pct( "WG_CREEKS" );
    v.lakes = pct( "WG_LAKES" );
    v.lake_size = pct( "WG_LAKE_SIZE" );
    v.ocean = get_option<bool>( "WG_OCEAN" );
    v.ocean_distance = pct( "WG_OCEAN_DISTANCE" );
    v.swamps = pct( "WG_SWAMPS" );
    v.forest = pct( "WG_FOREST_DENSITY" );
    v.forest_limit = pct( "WG_FOREST_LIMIT" );
    v.forest_clumping = pct( "WG_FOREST_CLUMPING" );
    v.ravines = get_option<int>( "WG_RAVINES" );
    v.fields = pct( "WG_FIELDS" );
    v.hordes = pct( "WG_HORDES" );
    v.wander_spawns = get_option<bool>( "WG_WANDER_SPAWNS" );
    v.npc_density = pct( "WG_NPC_DENSITY" );
    v.wildlife = pct( "WG_WILDLIFE" );
    v.vehicles = pct( "WG_VEHICLES" );
    v.fuel = pct( "WG_FUEL" );
    v.food = pct( "WG_FOOD" );
    v.specials = pct( "WG_SPECIALS" );
    v.portal_sites = get_option<int>( "WG_PORTAL_SITES" );
    v.pocket_route_scale = get_option<std::string>( "WG_POCKET_ROUTE_SCALE" );
    v.pocket_size_cap = get_option<std::string>( "WG_POCKET_SIZE_CAP" );
    v.pocket_max_active = get_option<int>( "WG_POCKET_MAX_ACTIVE" );
    g_values = v;
}

const values &get()
{
    return g_values;
}

namespace
{
// Base (JSON) values of everything the overrides touch, captured the first
// time they are applied after a data load, so applying again in the same load
// starts from the same base instead of compounding.
struct base_values {
    bool captured = false;
    int city_size = 0;
    int city_spacing = 0;
    int river_scale = 0;
    double river_frequency = 0;
    double river_branch_chance = 0;
    double lake_threshold = 0;
    int lake_size_min = 0;
    std::optional<int> ocean_n, ocean_e, ocean_w, ocean_s;
    double forest = 0, forest_thick = 0, swamp_adjacent = 0, swamp_isolated = 0;
    float max_forest = 0;
    int num_ravines = 0;
    double npc_spawn_time = 0;
};
base_values g_base;
} // namespace

void on_data_reset()
{
    g_base = base_values();
}

void apply_region_overrides()
{
    refresh();
    const values &v = g_values;
    if( !DEFAULT_REGION.is_valid() ) {
        return;
    }
    // The factories hand out const objects; the overrides are a finalize-time
    // adjustment of loaded data, the same moment the JSON-driven finalize
    // steps mutate it, so casting the constness away here is deliberate.
    region_settings &region = const_cast<region_settings &>( DEFAULT_REGION.obj() );
    if( !g_base.captured ) {
        g_base.captured = true;
        if( region.city_spec ) {
            g_base.city_size = region.city_spec->obj().city_size;
            g_base.city_spacing = region.city_spec->obj().city_spacing;
        }
        if( region.overmap_river ) {
            g_base.river_scale = region.overmap_river->obj().river_scale;
            g_base.river_frequency = region.overmap_river->obj().river_frequency;
            g_base.river_branch_chance = region.overmap_river->obj().river_branch_chance;
        }
        if( region.overmap_lake ) {
            g_base.lake_threshold = region.overmap_lake->obj().noise_threshold_lake;
            g_base.lake_size_min = region.overmap_lake->obj().lake_size_min;
        }
        if( region.overmap_ocean ) {
            const region_settings_ocean &o = region.overmap_ocean->obj();
            g_base.ocean_n = o.ocean_start_north;
            g_base.ocean_e = o.ocean_start_east;
            g_base.ocean_w = o.ocean_start_west;
            g_base.ocean_s = o.ocean_start_south;
        }
        if( region.overmap_forest ) {
            const region_settings_forest &f = region.overmap_forest->obj();
            g_base.forest = f.noise_threshold_forest;
            g_base.forest_thick = f.noise_threshold_forest_thick;
            g_base.swamp_adjacent = f.noise_threshold_swamp_adjacent_water;
            g_base.swamp_isolated = f.noise_threshold_swamp_isolated;
            g_base.max_forest = f.max_forest;
        }
        if( region.overmap_ravine ) {
            g_base.num_ravines = region.overmap_ravine->obj().num_ravines;
        }
        g_base.npc_spawn_time = region.npc_spawn_time;
    } else {
        // Restore the base before applying again.
        if( region.city_spec ) {
            region_settings_city &c = const_cast<region_settings_city &>( region.city_spec->obj() );
            c.city_size = g_base.city_size;
            c.city_spacing = g_base.city_spacing;
        }
        if( region.overmap_river ) {
            region_settings_river &r = const_cast<region_settings_river &>( region.overmap_river->obj() );
            r.river_scale = g_base.river_scale;
            r.river_frequency = g_base.river_frequency;
            r.river_branch_chance = g_base.river_branch_chance;
        }
        if( region.overmap_lake ) {
            region_settings_lake &l = const_cast<region_settings_lake &>( region.overmap_lake->obj() );
            l.noise_threshold_lake = g_base.lake_threshold;
            l.lake_size_min = g_base.lake_size_min;
        }
        if( region.overmap_ocean ) {
            region_settings_ocean &o = const_cast<region_settings_ocean &>( region.overmap_ocean->obj() );
            o.ocean_start_north = g_base.ocean_n;
            o.ocean_start_east = g_base.ocean_e;
            o.ocean_start_west = g_base.ocean_w;
            o.ocean_start_south = g_base.ocean_s;
        }
        if( region.overmap_forest ) {
            region_settings_forest &f = const_cast<region_settings_forest &>( region.overmap_forest->obj() );
            f.noise_threshold_forest = g_base.forest;
            f.noise_threshold_forest_thick = g_base.forest_thick;
            f.noise_threshold_swamp_adjacent_water = g_base.swamp_adjacent;
            f.noise_threshold_swamp_isolated = g_base.swamp_isolated;
            f.max_forest = g_base.max_forest;
        }
        if( region.overmap_ravine ) {
            const_cast<region_settings_ravine &>( region.overmap_ravine->obj() ).num_ravines = g_base.num_ravines;
        }
        region.npc_spawn_time = g_base.npc_spawn_time;
    }

    if( region.city_spec ) {
        region_settings_city &city = const_cast<region_settings_city &>( region.city_spec->obj() );
        if( v.city_density <= 0.f ) {
            city.city_size = 0; // no cities at all: size 0 is how the generator spells that
        } else {
            city.city_size = std::clamp( v.city_size, 1, 16 );
            city.city_spacing = std::clamp( v.city_spacing, 0, 8 );
        }
    }
    if( region.overmap_river ) {
        region_settings_river &river = const_cast<region_settings_river &>( region.overmap_river->obj() );
        if( v.rivers <= 0.f ) {
            river.river_scale = 0; // the generator skips rivers for scale 0
        } else {
            // chance to continue a river is 1 / frequency^count: raising the
            // frequency to 1/factor makes rivers more (factor > 1) or less common.
            river.river_frequency = std::pow( std::max( 1.0, river.river_frequency ), 1.0 / v.rivers );
            river.river_scale = std::max( 1, static_cast<int>( std::lround( river.river_scale * v.river_width ) ) );
            // branches are one_in( chance ): fewer creeks = larger chance
            river.river_branch_chance = v.creeks <= 0.f ? 1e9 : river.river_branch_chance / v.creeks;
        }
    }
    if( region.overmap_lake ) {
        region_settings_lake &lake = const_cast<region_settings_lake &>( region.overmap_lake->obj() );
        lake.noise_threshold_lake = scaled_threshold( lake.noise_threshold_lake, v.lakes );
        lake.lake_size_min = std::max( 1, static_cast<int>( std::lround( lake.lake_size_min * v.lake_size ) ) );
    }
    if( region.overmap_ocean ) {
        region_settings_ocean &ocean = const_cast<region_settings_ocean &>( region.overmap_ocean->obj() );
        const auto scale = [&]( std::optional<int> &d ) {
            if( !v.ocean ) {
                d.reset();
            } else if( d ) {
                *d = std::max( 1, static_cast<int>( std::lround( *d * v.ocean_distance ) ) );
            }
        };
        scale( ocean.ocean_start_north );
        scale( ocean.ocean_start_east );
        scale( ocean.ocean_start_west );
        scale( ocean.ocean_start_south );
    }
    if( region.overmap_forest ) {
        region_settings_forest &forest = const_cast<region_settings_forest &>( region.overmap_forest->obj() );
        forest.noise_threshold_forest = scaled_threshold( forest.noise_threshold_forest, v.forest );
        forest.noise_threshold_forest_thick = scaled_threshold( forest.noise_threshold_forest_thick, v.forest );
        forest.max_forest = static_cast<float>( forest.max_forest * v.forest_limit );
        forest.noise_threshold_swamp_adjacent_water =
            scaled_threshold( forest.noise_threshold_swamp_adjacent_water, v.swamps );
        forest.noise_threshold_swamp_isolated = scaled_threshold( forest.noise_threshold_swamp_isolated, v.swamps );
    }
    if( region.overmap_ravine ) {
        region_settings_ravine &ravine = const_cast<region_settings_ravine &>( region.overmap_ravine->obj() );
        ravine.num_ravines = std::max( 0, v.ravines );
    }
    // NPC density: the region's spawn time is the mean days between random NPCs.
    if( v.npc_density <= 0.f ) {
        region.npc_spawn_time = 1e9;
    } else {
        region.npc_spawn_time = region.npc_spawn_time / v.npc_density;
    }
}

namespace
{
float category_factor( const overmap_special &special )
{
    const values &v = g_values;
    float f = 1.f;
    if( special.has_flag( "FARM" ) ) {
        f *= v.fields;
    }
    // Rural: wilderness sites, or man-made ones that must keep away from cities.
    const bool away_from_cities = special.get_constraints().city_distance.min > 0;
    if( special.has_flag( "WILDERNESS" ) || ( special.has_flag( "MAN_MADE" ) && away_from_cities &&
            !special.has_flag( "URBAN" ) ) ) {
        f *= v.rural_density;
    }
    // TODO(astral-worldgen-options follow-up): per-faction factors keyed on
    // LAB / MILITARY / MI-GO / FUNGAL / ... flags go here.
    return f;
}
} // namespace

float specials_factor( const overmap_special &special )
{
    if( special.has_flag( "OVERMAP_UNIQUE" ) || special.has_flag( "GLOBALLY_UNIQUE" ) ) {
        return 1.f; // deck-drawn story sites keep their odds
    }
    return g_values.specials * category_factor( special );
}

} // namespace worldgen_options
