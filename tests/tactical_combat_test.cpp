#include <sstream>
#include <algorithm>
#include "json.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers_tests.h"
#include "avatar.h"
#include "avatar_action.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "damage.h"
#include "game.h"
#include "item.h"
#include "item_location.h"
#include "map_helpers.h"
#include "monster.h"
#include "options_helpers.h"
#include "options.h"
#include "player_helpers.h"
#include "ret_val.h"
#include "tactical_combat.h"
#include "type_id.h"

using tactical_combat::action;

TEST_CASE( "Tactical_defense_spends_time_and_never_stacks", "[tactical_combat]" )
{
    override_option enabled( "TACTICAL_COMBAT", "true" );
    clear_avatar();
    avatar &you = get_avatar();
    g->safe_mode = SAFE_MODE_OFF;
    REQUIRE( you.wear_item( item( itype_id( "astral_shield_round" ) ), false ) );
    you.set_moves( 500 );
    const int stamina = you.get_stamina();
    REQUIRE( tactical_combat::execute( you, action::guard ) );
    CHECK( you.get_moves() == 400 );
    CHECK( you.get_stamina() == stamina - 150 );
    CHECK( you.has_effect( efftype_id( "astral_guard" ) ) );
    REQUIRE( tactical_combat::execute( you, action::evade ) );
    CHECK_FALSE( you.has_effect( efftype_id( "astral_guard" ) ) );
    CHECK( you.has_effect( efftype_id( "astral_evade" ) ) );
    CHECK( you.get_moves() == 300 );
    const float evading = you.get_dodge();
    tactical_combat::clear_defense( you );
    CHECK( evading > you.get_dodge() );
    you.set_stamina( you.get_stamina_max() / 2 );
    const int before = you.get_stamina();
    REQUIRE( tactical_combat::execute( you, action::recover ) );
    CHECK( you.get_stamina() == before + you.get_stamina_max() / 20 );
    CHECK( you.get_moves() == 150 );
    CHECK_FALSE( you.has_effect( efftype_id( "astral_evade" ) ) );
    monster attacker( mtype_id( "mon_zombie" ) );
    bodypart_id part( "torso" );
    damage_instance damage( damage_type_id( "bash" ), 10 );
    you.blocks_left = 1;
    CHECK_FALSE( you.block_hit( &attacker, part, damage ) );
}

TEST_CASE( "Tactical_invalid_actions_do_not_spend_resources", "[tactical_combat]" )
{
    override_option enabled( "TACTICAL_COMBAT", "true" );
    clear_avatar();
    avatar &you = get_avatar();
    you.set_moves( 500 );
    const int stamina = you.get_stamina();
    CHECK_FALSE( tactical_combat::execute( you, action::guard ) );
    CHECK_FALSE( tactical_combat::execute( you, action::bash ) );
    CHECK_FALSE( tactical_combat::execute( you, action::attack ) );
    CHECK_FALSE( tactical_combat::execute( you, action::recover ) );
    CHECK( you.get_moves() == 500 );
    CHECK( you.get_stamina() == stamina );
    you.add_effect( efftype_id( "stunned" ), 2_turns );
    CHECK_FALSE( tactical_combat::execute( you, action::evade ) );
    CHECK( you.get_moves() == 500 );
}

TEST_CASE( "Tactical_auto_choices_react_to_windup_and_stamina", "[tactical_combat]" )
{
    override_option enabled( "TACTICAL_COMBAT", "true" );
    clear_avatar();
    avatar &you = get_avatar();
    monster enemy( mtype_id( "mon_zombie_brute" ) );
    CHECK( tactical_combat::choose_auto_action( you, enemy, "balanced" ) == action::attack );
    enemy.add_effect( efftype_id( "astral_windup" ), 4_turns );
    REQUIRE( you.wear_item( item( itype_id( "astral_shield_round" ) ), false ) );
    CHECK( tactical_combat::choose_auto_action( you, enemy, "balanced" ) == action::guard );
    enemy.remove_effect( efftype_id( "astral_windup" ) );
    you.set_stamina( you.get_stamina_max() / 3 );
    CHECK( tactical_combat::choose_auto_action( you, enemy, "balanced" ) == action::recover );
    CHECK( tactical_combat::choose_auto_action( you, enemy, "aggressive" ) == action::attack );
}

TEST_CASE( "Tactical_creature_windup_commits_time_and_target_tile", "[tactical_combat]" )
{
    override_option enabled( "TACTICAL_COMBAT", "true" );
    clear_avatar();
    avatar &you = get_avatar();
    monster enemy( mtype_id( "mon_zombie_brute" ) );
    enemy.set_moves( 100 );
    bool heavy = false;
    REQUIRE( tactical_combat::prepare_monster_strike( enemy, you, heavy ) );
    CHECK_FALSE( heavy );
    CHECK( enemy.get_moves() <= 0 );
    CHECK( enemy.has_effect( efftype_id( "astral_windup" ) ) );
    SECTION( "same_tile_releases_heavy_attack" ) {
        CHECK_FALSE( tactical_combat::prepare_monster_strike( enemy, you, heavy ) );
        CHECK( heavy );
    }
    SECTION( "moving_away_spoils_heavy_attack" ) {
        you.setpos( get_map(), you.pos_bub() + tripoint::east );
        CHECK( tactical_combat::prepare_monster_strike( enemy, you, heavy ) );
        CHECK_FALSE( heavy );
    }
    CHECK_FALSE( enemy.has_effect( efftype_id( "astral_windup" ) ) );
    CHECK( enemy.has_effect( efftype_id( "astral_heavy_recovery" ) ) );
    CHECK_FALSE( tactical_combat::prepare_monster_strike( enemy, you, heavy ) );
    CHECK_FALSE( heavy );
}

TEST_CASE( "Tactical_unflagged_creatures_keep_normal_attacks", "[tactical_combat]" )
{
    override_option enabled( "TACTICAL_COMBAT", "true" );
    clear_avatar();
    monster enemy( mtype_id( "mon_zombie" ) );
    enemy.set_moves( 100 );
    bool heavy = false;
    CHECK_FALSE( tactical_combat::prepare_monster_strike( enemy, get_avatar(), heavy ) );
    CHECK_FALSE( heavy );
    CHECK( enemy.get_moves() == 100 );
}

TEST_CASE( "Tactical_shield_bash_keeps_main_weapon_and_costs_moves", "[tactical_combat]" )
{
    override_option enabled( "TACTICAL_COMBAT", "true" );
    clear_map_without_vision();
    clear_avatar();
    avatar &you = get_avatar();
    g->safe_mode = SAFE_MODE_OFF;
    set_time_to_day();
    you.setpos( get_map(), tripoint_bub_ms( 60, 60, 0 ) );
    item sword( itype_id( "machete" ) );
    REQUIRE( you.wield( sword ) );
    REQUIRE( you.wear_item( item( itype_id( "astral_shield_round" ) ), false ) );
    monster &enemy = spawn_test_monster( "mon_zombie", you.pos_bub() + tripoint::east );
    get_map().update_visibility_cache( you.pos_bub().z() );
    you.set_moves( 500 );
    const int stamina = you.get_stamina();
    const auto check = tactical_combat::assess( you, action::bash, &enemy );
    INFO( check.reason );
    REQUIRE( check.available );
    REQUIRE( tactical_combat::execute( you, action::bash, &enemy ) );
    REQUIRE( you.get_wielded_item() );
    CHECK( you.get_wielded_item()->typeId() == sword.typeId() );
    CHECK( you.get_moves() == 500 - check.moves );
    CHECK( you.get_stamina() == stamina - check.stamina );
    REQUIRE( you.best_shield() );
    CHECK( you.best_shield()->typeId() == itype_id( "astral_shield_round" ) );
}

TEST_CASE( "Tactical_windup_survives_save_reload", "[tactical_combat]" )
{
    override_option enabled( "TACTICAL_COMBAT", "true" );
    clear_avatar();
    monster original( mtype_id( "mon_zombie_brute" ) );
    bool heavy = false;
    REQUIRE( tactical_combat::prepare_monster_strike( original, get_avatar(), heavy ) );
    std::ostringstream stream;
    JsonOut writer( stream );
    original.serialize( writer );
    monster loaded;
    loaded.deserialize( json_loader::from_string( stream.str() ).get_object() );
    CHECK( loaded.has_effect( efftype_id( "astral_windup" ) ) );
    CHECK_FALSE( tactical_combat::prepare_monster_strike( loaded, get_avatar(), heavy ) );
    CHECK( heavy );
}

TEST_CASE( "Tactical_auto_combat_stops_before_spending_resources_at_low_health",
           "[tactical_combat]" )
{
    override_option enabled( "TACTICAL_COMBAT", "true" );
    clear_map_without_vision();
    clear_avatar();
    avatar &you = get_avatar();
    g->safe_mode = SAFE_MODE_OFF;
    you.set_moves( 500 );
    you.set_part_hp_cur( bodypart_id( "torso" ), 1 );
    const int stamina = you.get_stamina();
    CHECK_FALSE( avatar_action::auto_combat( you, get_map() ) );
    CHECK( you.get_moves() == 500 );
    CHECK( you.get_stamina() == stamina );
}

TEST_CASE( "Tactical_creature_first_swing_is_a_warning_not_damage", "[tactical_combat]" )
{
    override_option enabled( "TACTICAL_COMBAT", "true" );
    clear_map_without_vision();
    clear_avatar();
    avatar &you = get_avatar();
    set_time_to_day();
    you.setpos( get_map(), tripoint_bub_ms( 60, 60, 0 ) );
    monster &enemy = spawn_test_monster( "mon_zombie_brute", you.pos_bub() + tripoint::east );
    get_map().update_visibility_cache( you.pos_bub().z() );
    enemy.set_moves( 100 );
    const int hp = you.get_hp();
    REQUIRE( enemy.melee_attack( you ) );
    CHECK( you.get_hp() == hp );
    CHECK( enemy.get_moves() < 0 );
    CHECK( enemy.has_effect( efftype_id( "astral_windup" ) ) );
}

TEST_CASE( "Tactical_stopping_auto_combat_persists_without_spending_a_turn", "[tactical_combat]" )
{
    override_option auto_on( "AUTO_COMBAT", "true" );
    clear_avatar();
    avatar &you = get_avatar();
    you.set_moves( 100 );
    tactical_combat::stop_auto_combat();
    CHECK_FALSE( get_option<bool>( "AUTO_COMBAT" ) );
    get_options().get_option( "AUTO_COMBAT" ).setValue( "true" );
    get_options().load();
    CHECK_FALSE( get_option<bool>( "AUTO_COMBAT" ) );
    CHECK( you.get_moves() == 100 );
}

TEST_CASE( "Tactical_manual_selection_includes_neutral_animals_without_attacking",
           "[tactical_combat]" )
{
    override_option enabled( "TACTICAL_COMBAT", "true" );
    clear_map_without_vision();
    clear_avatar();
    avatar &you = get_avatar();
    set_time_to_day();
    you.setpos( get_map(), tripoint_bub_ms( 60, 60, 0 ) );
    monster &horse = spawn_test_monster( "mon_horse", you.pos_bub() + tripoint::east );
    horse.anger = 0;
    horse.morale = 0;
    get_map().update_visibility_cache( you.pos_bub().z() );
    const int moves = you.get_moves();
    const int stamina = you.get_stamina();
    const int hp = horse.get_hp();
    REQUIRE( horse.attitude_to( you ) != Creature::Attitude::HOSTILE );
    CHECK( tactical_combat::can_target_from_map( you, &horse ) );
    const auto targets = tactical_combat::manual_targets( you );
    CHECK( std::find( targets.begin(), targets.end(), &horse ) != targets.end() );
    g->safe_mode = SAFE_MODE_OFF;
    CHECK( tactical_combat::assess_manual( you, action::attack, &horse ).available );
    g->safe_mode = SAFE_MODE_ON;
    const auto blocked = tactical_combat::assess_manual( you, action::attack, &horse );
    CHECK_FALSE( blocked.available );
    CHECK_FALSE( blocked.reason.empty() );
    CHECK( tactical_combat::assess_manual( you, action::evade, &horse ).available );
    CHECK( you.get_moves() == moves );
    CHECK( you.get_stamina() == stamina );
    CHECK( horse.get_hp() == hp );
    g->safe_mode = SAFE_MODE_OFF;

    SECTION( "Pets_keep_their_normal_primary_interaction" ) {
        horse.add_effect( efftype_id( "pet" ), 1_turns, true );
        CHECK_FALSE( tactical_combat::can_target_from_map( you, &horse ) );
    }
    SECTION( "Disabled_tactical_combat_keeps_normal_controls" ) {
        override_option disabled( "TACTICAL_COMBAT", "false" );
        CHECK_FALSE( tactical_combat::can_target_from_map( you, &horse ) );
    }
    SECTION( "Distant_visible_creatures_can_be_selected_but_not_struck" ) {
        horse.setpos( get_map(), you.pos_bub() + tripoint( 3, 0, 0 ) );
        CHECK( tactical_combat::can_target_from_map( you, &horse ) );
        const auto distant_targets = tactical_combat::manual_targets( you );
        CHECK( std::find( distant_targets.begin(), distant_targets.end(),
                          &horse ) == distant_targets.end() );
        CHECK_FALSE( tactical_combat::assess_manual( you, action::attack, &horse ).available );
    }
    SECTION( "Dead_creatures_do_not_open_a_target_panel" ) {
        horse.set_hp( 0 );
        CHECK_FALSE( tactical_combat::can_target_from_map( you, &horse ) );
    }
}

TEST_CASE( "Tactical_hotbar_selection_is_free_and_clears_when_target_dies", "[tactical_combat]" )
{
    override_option enabled( "TACTICAL_COMBAT", "true" );
    override_option auto_off( "AUTO_COMBAT", "false" );
    tactical_combat::clear_target();
    clear_map_without_vision();
    clear_avatar();
    avatar &you = get_avatar();
    g->safe_mode = SAFE_MODE_OFF;
    set_time_to_day();
    you.setpos( get_map(), tripoint_bub_ms( 60, 60, 0 ) );
    monster &horse = spawn_test_monster( "mon_horse", you.pos_bub() + tripoint::east );
    horse.anger = 0;
    horse.morale = 0;
    get_map().update_visibility_cache( you.pos_bub().z() );
    const int moves = you.get_moves();
    const int stamina = you.get_stamina();
    const int hp = horse.get_hp();
    // No picker and no attack when there is no selected hostile target.
    tactical_combat::manual_action( you, action::attack );
    CHECK( tactical_combat::selected_target( you ) == nullptr );
    REQUIRE( tactical_combat::select_target( you, &horse ) );
    REQUIRE( tactical_combat::selected_target( you ) == &horse );
    CHECK( you.get_moves() == moves );
    CHECK( you.get_stamina() == stamina );
    CHECK( horse.get_hp() == hp );
    horse.setpos( get_map(), you.pos_bub() + tripoint( 3, 0, 0 ) );
    CHECK( tactical_combat::selected_target( you ) == &horse );
    const auto unavailable = tactical_combat::assess_manual( you, action::attack, &horse );
    CHECK_FALSE( unavailable.available );
    CHECK_FALSE( unavailable.reason.empty() );
    tactical_combat::manual_action( you, action::attack );
    CHECK( you.get_moves() == moves );
    CHECK( you.get_stamina() == stamina );
    CHECK( horse.get_hp() == hp );
    horse.set_hp( 0 );
    CHECK( tactical_combat::selected_target( you ) == nullptr );
}
