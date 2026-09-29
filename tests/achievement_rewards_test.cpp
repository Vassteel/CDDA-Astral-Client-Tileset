#include <sstream>
#include <algorithm>
#include "damage.h"
#include "recipe.h"
#include "string_formatter.h"
#include "math_parser_diag_value.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "talker.h"
#include "iuse.h"
#include "itype.h"
#include "item_location.h"
#include "mapdata.h"
#include "player_activity.h"
#include "achievement.h"
#include "achievement_rewards.h"
#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "construction.h"
#include "game.h"
#include "item.h"
#include "json.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers.h"
#include "morale.h"
#include "monster.h"
#include "player_helpers.h"
#include "type_id.h"
#include "vitamin.h"

namespace
{
void reset_rewards()
{
    clear_avatar();
    clear_map_without_vision();
    get_achievements().clear();
    get_achievements().deserialize( json_loader::from_string(
                                        "{\"enabled\":true,\"initial_achievements\":[\"achievement_survive_one_day\"],\"achievements_status\":{}}" ).get_object() );
}
std::string save_tracker()
{
    std::ostringstream out;
    JsonOut js( out );
    get_achievements().serialize( js );
    return out.str();
}
void load_tracker( const std::string &data )
{
    get_achievements().clear();
    get_achievements().deserialize( json_loader::from_string( data ).get_object() );
}
}
TEST_CASE( "Astral_reward_claims_persist_and_cannot_repeat", "[astral_rewards]" )
{
    reset_rewards();
    auto &t = get_achievements();
    auto &b = t.reward_bank;
    REQUIRE_FALSE( b.claim( t, "achievement_survive_one_day", 0 ) );
    achievement_rewards::complete( "achievement_survive_one_day" );
    achievement_rewards::complete( "achievement_survive_one_day" );
    REQUIRE_FALSE( b.claim( t, "achievement_survive_one_day", 99 ) );
    REQUIRE( b.claim( t, "achievement_survive_one_day", 0 ) );
    CHECK( b.credits["second_wind"] == 1 );
    REQUIRE_FALSE( b.claim( t, "achievement_survive_one_day", 0 ) );
    const std::string saved = save_tracker();
    load_tracker( saved );
    CHECK( b.credits["second_wind"] == 1 );
    REQUIRE_FALSE( b.claim( t, "achievement_survive_one_day", 0 ) );
    t.clear();
    CHECK( b.credits.empty() );
    CHECK( b.claims.empty() );
    load_tracker( "{\"enabled\":true,\"initial_achievements\":[],\"achievements_status\":{}}" );
    CHECK( b.credits.empty() );
    CHECK( t.is_completed( achievement_id( "achievement_survive_one_day" ) ) ==
           achievement_completion::pending );
    CHECK( t.valid_achievements().empty() );
    t.set_enabled( false );
    achievement_rewards::complete( "achievement_survive_one_day" );
    CHECK( t.is_completed( achievement_id( "achievement_survive_one_day" ) ) ==
           achievement_completion::pending );
}
TEST_CASE( "Astral_recovery_is_selective_and_preserves_unused_credits", "[astral_rewards]" )
{
    reset_rewards();
    avatar &u = get_avatar();
    auto &b = get_achievements().reward_bank;
    const bodypart_id arm( "arm_l" );
    b.credits = { {"field_recovery", 2}, {"boneknit", 1}, {"second_wind", 1}, {"nourished", 1}, {"fresh_start", 1} };
    const int max_hp = u.get_part_hp_max( arm );
    u.set_part_hp_cur( arm, max_hp );
    CHECK_FALSE( b.redeem( u, "field_recovery", arm ) );
    CHECK_FALSE( b.redeem( u, "field_recovery", bodypart_id() ) );
    CHECK( b.credits["field_recovery"] == 2 );
    u.set_part_hp_cur( arm, max_hp / 2 );
    u.add_effect( efftype_id( "bleed" ), 10_minutes, arm );
    REQUIRE( b.redeem( u, "field_recovery", arm ) );
    CHECK( u.get_part_hp_cur( arm ) >= max_hp * 0.9 - 1 );
    CHECK_FALSE( u.has_effect( efftype_id( "bleed" ), arm ) );
    CHECK( b.credits["field_recovery"] == 1 );
    u.set_part_hp_cur( arm, 0 );
    u.add_effect( efftype_id( "mending" ), 2_days, arm, true );
    CHECK_FALSE( b.redeem( u, "field_recovery", arm ) );
    REQUIRE( b.redeem( u, "boneknit", arm ) );
    CHECK( u.get_part_hp_cur( arm ) == max_hp / 2 );
    CHECK_FALSE( u.has_effect( efftype_id( "mending" ), arm ) );
    u.set_sleepiness( 200 );
    u.set_sleep_deprivation( 300 );
    u.set_stamina( 10 );
    const time_point before = calendar::turn;
    REQUIRE( b.redeem( u, "second_wind", bodypart_id() ) );
    CHECK( u.get_sleepiness() == 0 );
    CHECK( u.get_sleep_deprivation() == 0 );
    CHECK( u.get_stamina() == u.get_stamina_max() );
    CHECK( calendar::turn == before );
    u.set_stored_kcal( u.get_healthy_kcal() / 2 );
    u.set_thirst( 300 );
    u.vitamin_set( vitamin_id( "vitC" ), -100 );
    std::map<vitamin_id, int> other;
    for( const auto &v : vitamin::all() ) {
        if( v.second.type() != vitamin_type::VITAMIN ) {
            other[v.first] = u.vitamin_get( v.first );
        }
    }
    REQUIRE( b.redeem( u, "nourished", bodypart_id() ) );
    CHECK( u.get_stored_kcal() == u.get_healthy_kcal() );
    CHECK( u.get_thirst() == 0 );
    CHECK( u.vitamin_get( vitamin_id( "vitC" ) ) == 0 );
    for( const auto &v : other ) {
        CHECK( u.vitamin_get( v.first ) == v.second );
    }
    u.add_morale( morale_type( "morale_food_good" ), 15, 15, 1_hours, 1_hours );
    u.add_morale( morale_type( "morale_food_bad" ), -15, -15, 1_hours, 1_hours );
    REQUIRE( b.redeem( u, "fresh_start", bodypart_id() ) );
    CHECK( u.has_morale( morale_type( "morale_food_good" ) ) == 15 );
    CHECK( u.has_morale( morale_type( "morale_food_bad" ) ) == 0 );
    CHECK( u.has_morale( morale_type( "morale_astral_achievement" ) ) == 30 );
    const std::string saved = save_tracker();
    load_tracker( saved );
    CHECK_FALSE( b.redeem( u, "second_wind", bodypart_id() ) );
    CHECK( b.credits["second_wind"] == 0 );
}
TEST_CASE( "Astral_lantern_and_bedroll_keep_upgrades_when_transformed", "[astral_rewards]" )
{
    for( const std::string id : {
             "astral_nightwatch", "astral_everlight"
         } ) {
        item lamp{ itype_id( id ) };
        int capacity = id == "astral_nightwatch" ? 3000 : 6000;
        lamp.ammo_set( itype_id( "lamp_oil" ), capacity );
        CHECK( lamp.ammo_remaining() == capacity );
        lamp.convert( itype_id( id + "_on" ) );
        CHECK( lamp.ammo_remaining() == capacity );
        lamp.convert( itype_id( id ) );
        CHECK( lamp.ammo_remaining() == capacity );
    }
    item bag( itype_id( "astral_dreamweave_roll" ) );
    CHECK( bag.weight() == 350_gram );
    bag.convert( itype_id( "astral_dreamweave" ) );
    CHECK( bag.weight() == 350_gram );
}

TEST_CASE( "Astral_morale_preserves_permanent_and_positive_entries", "[astral_rewards]" )
{
    player_morale m;
    m.set_permanent( morale_type( "morale_perm_badtemper" ), -20 );
    m.add( morale_type( "morale_food_good" ), 10 );
    m.add( morale_type( "morale_food_bad" ), -10 );
    REQUIRE( m.has_temporary_negative() );
    m.clear_temporary_negative();
    CHECK( m.has( morale_type( "morale_perm_badtemper" ) ) == -20 );
    CHECK( m.has( morale_type( "morale_food_good" ) ) == 10 );
    CHECK( m.has( morale_type( "morale_food_bad" ) ) == 0 );
    CHECK_FALSE( m.has_temporary_negative() );
}

TEST_CASE( "Astral_oversized_or_missing_containers_leave_rewards_pending", "[astral_rewards]" )
{
    reset_rewards();
    auto &b = get_achievements().reward_bank;
    achievement_rewards::item_award parcel;
    parcel.item = itype_id( "coffee" );
    parcel.charges = 8;
    SECTION( "default_bottle_must_not_discard_coffee" ) {}
    SECTION( "explicit_container_too_small" ) {
        parcel.container = itype_id( "thermos" );
    }
    SECTION( "missing_container_from_saved_payload" ) {
        parcel.container = itype_id( "nonexistent_astral_container" );
    }
    b.parcels.push_back( parcel );
    CHECK_FALSE( b.deliver( get_avatar(), 0, true ) );
    load_tracker( save_tracker() );
    REQUIRE( b.parcels.size() == 1 );
    CHECK( b.parcels[0].count == 1 );
    CHECK( b.parcels[0].charges == 8 );
}


TEST_CASE( "Astral_expansion_is_shelved_while_vanilla_rewards_remain", "[astral_rewards]" )
{
    reset_rewards();
    CHECK( achievement_rewards::all().size() == 211 );
    for( const auto &entry : achievement_rewards::all() ) {
        CHECK_FALSE( entry.second.enroll );
        CHECK( entry.first.find( "astral_" ) != 0 );
        CHECK( achievement_id( entry.first ).is_valid() );
    }
    for( int number = 1; number <= 211; ++number ) {
        CHECK_FALSE( achievement_id( string_format( "astral_%03d", number ) ).is_valid() );
    }
    avatar &u = get_avatar();
    u.add_effect( efftype_id( "sleep" ), 1_hours );
    get_map().furn_set( u.pos_bub(), furn_id( "f_bed" ) );
    achievement_rewards::on_sleep( u );
    achievement_rewards::on_natural_healing( u, 100 );
    achievement_rewards::on_wake( u );
    CHECK( get_achievements().reward_bank.counters.empty() );
    CHECK( get_achievements().reward_bank.seen.empty() );
    achievement_rewards::complete( "astral_014" );
    CHECK_FALSE( get_achievements().reward_bank.claim( get_achievements(), "astral_014", 0 ) );
}

TEST_CASE( "Every_vanilla_reward_choice_can_be_claimed_and_delivered", "[astral_rewards]" )
{
    reset_rewards();
    auto &t = get_achievements();
    avatar &u = get_avatar();
    for( const auto &entry : achievement_rewards::all() ) {
        for( size_t choice = 0; choice < entry.second.choices.size(); ++choice ) {
            INFO( entry.first );
            INFO( choice );
            const std::string fixture = string_format(
                                            "{\"enabled\":true,\"initial_achievements\":[\"%s\"],\"achievements_status\":{\"%s\":{\"completion\":\"completed\",\"last_state_change\":1,\"final_values\":[]}}}",
                                            entry.first, entry.first );
            load_tracker( fixture );
            auto &b = t.reward_bank;
            REQUIRE( b.claim( t, entry.first, choice ) );
            load_tracker( save_tracker() );
            CHECK_FALSE( b.claim( t, entry.first, choice ) );
            get_map().i_clear( u.pos_bub() );
            while( !b.parcels.empty() ) {
                REQUIRE( b.deliver( u, 0, true ) );
            }
            for( const item &it : get_map().i_at( u.pos_bub() ) ) {
                CHECK( it.get_var( "astral_reward" ) == "yes" );
            }
            load_tracker( save_tracker() );
            CHECK( b.parcels.empty() );
            CHECK_FALSE( b.claim( t, entry.first, choice ) );
        }
    }
}
TEST_CASE( "Reward_points_survive_progression_initialization_and_only_spend_once",
           "[astral_rewards]" )
{
    reset_rewards();
    avatar &u = get_avatar();
    auto &b = get_achievements().reward_bank;
    for( const auto &p : std::map<std::string, std::string> {
    {"perk_point", "num_perks"}, {"martial_point", "num_ma_perks"},
    {"playstyle_point", "playstyle_perks_available"}
} ) {
        b.credits[p.first] = 1;
        REQUIRE( b.redeem( u, p.first, bodypart_id() ) );
        CHECK( u.get_value( p.second ).dbl() == 1 );
        load_tracker( save_tracker() );
        CHECK_FALSE( b.redeem( u, p.first, bodypart_id() ) );
        CHECK( u.get_value( p.second ).dbl() == 1 );
    }
    dialogue d( get_talker_for( u ), nullptr );
    effect_on_condition_id( "EOC_give_perk_menu" )->activate( d );
    effect_on_condition_id( "EOC_give_ma_perk_menu" )->activate( d );
    CHECK( u.get_value( "num_perks" ).dbl() == 1 );
    CHECK( u.get_value( "num_ma_perks" ).dbl() == 1 );
    CHECK( u.get_value( "playstyle_perks_available" ).dbl() == 1 );
    b.credits["reading_desk_plans"] = 1;
    const recipe &plans = recipe_id( "astral_archivist_desk" ).obj();
    REQUIRE_FALSE( u.knows_recipe( &plans ) );
    REQUIRE( b.redeem( u, "reading_desk_plans", bodypart_id() ) );
    CHECK( u.knows_recipe( &plans ) );
    load_tracker( save_tracker() );
    CHECK_FALSE( b.redeem( u, "reading_desk_plans", bodypart_id() ) );
    CHECK( furn_id( "f_astral_archivist_desk" ).obj().workbench->multiplier == Approx( 1.3 ) );
}
TEST_CASE( "Named_reward_gear_has_real_improvements", "[astral_rewards]" )
{
    reset_rewards();
    item normal( itype_id( "machete" ) );
    item legendary( itype_id( "astral_last_word" ) );
    CHECK( legendary.damage_melee( damage_type_id( "cut" ) ) >= normal.damage_melee(
               damage_type_id( "cut" ) ) * 1.5 );
    item pistol( itype_id( "glock_19" ) );
    item final( itype_id( "astral_legend_pistol" ) );
    CHECK( final.weight() == pistol.weight() / 2 );
    CHECK( final.type->gun->dispersion == Approx( pistol.type->gun->dispersion * 0.75 ) );
    CHECK( final.type->gun->recoil == Approx( pistol.type->gun->recoil * 0.85 ).margin( 1 ) );
    avatar &u = get_avatar();
    const int read_before = u.read_speed();
    REQUIRE( u.wear_item( item( itype_id( "astral_archivist_plain" ) ), false ).has_value() );
    u.recalculate_enchantment_cache();
    CHECK( u.read_speed() == Approx( read_before * 0.85 ).margin( 1 ) );
    const tripoint_bub_ms tree = u.pos_bub() + tripoint::east;
    get_map().ter_set( tree, ter_id( "t_tree" ) );
    item axe( itype_id( "ax" ) );
    iuse::chop_tree( &u, &axe, tree );
    const int ordinary_moves = u.activity.moves_total;
    REQUIRE( ordinary_moves > 0 );
    u.cancel_activity();
    item better( itype_id( "astral_forester_axe" ) );
    iuse::chop_tree( &u, &better, tree );
    CHECK( u.activity.moves_total == Approx( ordinary_moves / 1.4 ).margin( 1 ) );
    u.cancel_activity();
}

TEST_CASE( "Reward_battery_tools_receive_their_real_charged_cell", "[astral_rewards]" )
{
    reset_rewards();
    auto &b = get_achievements().reward_bank;
    achievement_rewards::item_award flashlight;
    flashlight.item = itype_id( "flashlight" );
    flashlight.charges = 56;
    b.parcels.push_back( flashlight );
    REQUIRE( b.deliver( get_avatar(), 0, true ) );
    const item &light = get_map().i_at( get_avatar().pos_bub() ).only_item();
    CHECK( light.ammo_remaining() == 56 );
    REQUIRE( light.magazine_current() != nullptr );
    CHECK( light.magazine_current()->typeId() == itype_id( "medium_battery_cell" ) );
}

TEST_CASE( "Research_light_retains_its_real_cell_across_switch_states", "[astral_rewards]" )
{
    reset_rewards();
    auto &b = get_achievements().reward_bank;
    achievement_rewards::item_award parcel;
    parcel.item = itype_id( "astral_research_light" );
    parcel.charges = 168;
    b.parcels.push_back( parcel );
    REQUIRE( b.deliver( get_avatar(), 0, true ) );
    item &light = get_map().i_at( get_avatar().pos_bub() ).only_item();
    REQUIRE( light.magazine_current() != nullptr );
    CHECK( light.magazine_current()->typeId() == itype_id( "astral_research_cell" ) );
    CHECK( light.ammo_remaining() == 168 );
    light.convert( itype_id( "astral_research_light_on" ) );
    CHECK( light.ammo_remaining() == 168 );
    light.convert( itype_id( "astral_research_light" ) );
    CHECK( light.ammo_remaining() == 168 );
}
