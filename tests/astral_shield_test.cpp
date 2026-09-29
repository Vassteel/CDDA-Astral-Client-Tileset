#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character_martial_arts.h"
#include "damage.h"
#include "flag.h"
#include "item.h"
#include "item_location.h"
#include "map_helpers.h"
#include "monster.h"
#include "pimpl.h"
#include "player_helpers.h"
#include "ret_val.h"
#include "type_id.h"

TEST_CASE( "Astral_shield_leaves_weapon_in_main_hand", "[astral_shield][wield]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    item sword( itype_id( "machete" ) );
    item shield( itype_id( "astral_shield_round" ) );
    REQUIRE( you.wield( sword ) );
    REQUIRE( you.can_wear( shield ).success() );
    REQUIRE( you.wear_item( shield, false ).has_value() );
    CHECK( you.get_wielded_item()->typeId() == sword.typeId() );
    REQUIRE( you.best_shield() );
    CHECK( you.best_shield()->typeId() == shield.typeId() );
    CHECK_FALSE( you.can_wear( item( itype_id( "astral_shield_buckler" ) ) ).success() );
    item twohanded( itype_id( "machete" ) );
    twohanded.set_flag( flag_ALWAYS_TWOHAND );
    CHECK_FALSE( you.can_wield( twohanded ).success() );
    you.clear_worn();
    CHECK( you.can_wield( twohanded ).success() );
    you.remove_weapon();
    REQUIRE( you.wield( twohanded ) );
    CHECK_FALSE( you.can_wear( shield ).success() );
}

TEST_CASE( "Astral_shield_blocks_without_limb_block_training", "[astral_shield][melee]" )
{
    clear_map_without_vision();
    clear_avatar();
    avatar &you = get_avatar();
    you.set_skill_level( skill_id( "unarmed" ), 0 );
    you.set_skill_level( skill_id( "melee" ), 0 );
    you.martial_arts_data->reset_style();
    REQUIRE_FALSE( you.martial_arts_data->can_arm_block( you ) );
    REQUIRE( you.wear_item( item( itype_id( "astral_shield_round" ) ), false ).has_value() );
    SECTION( "one_handed_weapon" ) {
        item sword( itype_id( "machete" ) );
        REQUIRE( you.wield( sword ) );
    }
    SECTION( "empty_main_hand" ) {
        REQUIRE_FALSE( you.is_armed() );
    }
    monster attacker( mtype_id( "mon_zombie" ) );
    int blocks = 0;
    for( int attempt = 0; attempt < 100; ++attempt ) {
        you.blocks_left = 1;
        you.set_stamina( you.get_stamina_max() );
        bodypart_id target( "torso" );
        damage_instance hit( damage_type_id( "bash" ), 10 );
        if( you.block_hit( &attacker, target, hit ) ) {
            ++blocks;
            CHECK( hit.total_damage() < 10 );
            CHECK( target == bodypart_id( "torso" ) );
            CHECK( you.blocks_left == 0 );
            CHECK( you.get_stamina() < you.get_stamina_max() );
        }
    }
    CHECK( blocks > 0 );
    you.set_stamina( 100 );
    you.blocks_left = 1;
    bodypart_id target( "torso" );
    damage_instance hit( damage_type_id( "bash" ), 10 );
    CHECK_FALSE( you.block_hit( &attacker, target, hit ) );
    CHECK( hit.total_damage() == 10 );
}

TEST_CASE( "Astral_shield_requires_working_arms", "[astral_shield][wield]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    you.set_part_hp_cur( bodypart_id( "arm_l" ), 0 );
    CHECK_FALSE( you.can_wear( item( itype_id( "astral_shield_round" ) ) ).success() );
}
