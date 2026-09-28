#include <sstream>
#include <algorithm>
#include "damage.h"
#include "recipe.h"
#include "string_formatter.h"
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
    CHECK( t.is_completed( achievement_id( "achievement_survive_one_day" ) ) == achievement_completion::pending );
    CHECK( t.valid_achievements().empty() );
    t.set_enabled( false );
    achievement_rewards::complete( "achievement_survive_one_day" );
    CHECK( t.is_completed( achievement_id( "achievement_survive_one_day" ) ) == achievement_completion::pending );
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
    CHECK( achievement_rewards::all().size() == 6 );
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
