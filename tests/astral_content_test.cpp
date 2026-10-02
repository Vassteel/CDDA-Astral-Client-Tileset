#include <string>

#include "cata_catch.h"
#include "flag.h"
#include "item.h"
#include "itype.h"
#include "mapdata.h"
#include "monstergenerator.h"
#include "mtype.h"
#include "type_id.h"

// The critical core list (tools/astral/gen_content.py) loads: one of each generated kind.
TEST_CASE( "astral_generated_content_loads", "[astral][astral_content]" )
{
    for( const std::string &id : { "astral_ration", "astral_fungal_glowcap_raw", "astral_duskiron_bar",
                                   "astral_drowned_verdigris_knife", "astral_spore_mask",
                                   "astral_core_heart_fragment" } ) {
        INFO( id );
        CHECK( itype_id( id ).is_valid() );
    }
    CHECK( ter_str_id( "t_astral_root_duskiron_vein" ).is_valid() );
    CHECK( ter_str_id( "t_astral_drowned_ghostsalt_pan" ).is_valid() );
    CHECK( ter_str_id( "t_astral_meadow_hearthwood_tree" ).is_valid() );
    CHECK( ter_str_id( "t_astral_meadow_hearthwood_tree_harvested" ).is_valid() );
    CHECK( furn_str_id( "f_astral_fungal_glowcap_cluster" ).is_valid() );
    CHECK( mtype_id( "mon_astral_drowned_reed_eel" ).is_valid() );
    CHECK( mtype_id( "mon_astral_fungal_cap_beetle" ).is_valid() );
    CHECK( flag_id( "ASTRAL_TIER_7" ).is_valid() );

    // A mined vein breaks into its ore, and a tier flag rides on every item.
    const ter_t &vein = ter_str_id( "t_astral_root_duskiron_vein" ).obj();
    CHECK( vein.has_flag( ter_furn_flag::TFLAG_MINEABLE ) );
    REQUIRE( vein.bash.has_value() );
    CHECK( vein.bash->ter_set == ter_str_id( "t_dirt" ) );
    CHECK( item( itype_id( "astral_ration" ) ).has_flag( flag_id( "ASTRAL_TIER_1" ) ) );
    CHECK( item( itype_id( "astral_core_heart_fragment" ) ).has_flag( flag_id( "ASTRAL_TIER_7" ) ) );
}
