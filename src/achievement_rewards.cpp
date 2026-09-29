#include "achievement_rewards.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include "achievement.h"
#include "avatar.h"
#include "bodypart.h"
#include "character.h"
#include "character_attire.h"
#include "debug.h"
#include "construction.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "recipe.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "talker.h"
#include "math_parser_diag_value.h"
#include "json.h"
#include "map.h"
#include "mapdata.h"
#include "monster.h"
#include "mtype.h"
#include "string_formatter.h"
#include "translations.h"
#include "vitamin.h"

namespace achievement_rewards
{
namespace
{
std::map<std::string, definition> definitions;
bool applying = false;
bool expansion_enabled = false;
struct reward_guard {
    bool previous = applying;
    reward_guard() {
        applying = true;
    }
    ~reward_guard() {
        applying = previous;
    }
};
const std::map<std::string, std::pair<std::string, std::string>> recovery_types = {
    { "perk_point", { "Perk point", "Add one spendable general perk point. Normal choice requirements still apply." } },
    { "martial_point", { "Martial point", "Add one spendable Martial Mastery point. Normal choice requirements still apply." } },
    { "playstyle_point", { "Playstyle point", "Add one playstyle point. Learning a playstyle still requires its normal general perk point and prerequisites." } },
    { "reading_desk_plans", { "Archivist's desk plans", "Permanently learn to craft a folding Archivist's desk with an improved work surface." } },
    { "fresh_start", { "Fresh Start", "Clear temporary negative morale; preserve positive and permanent morale. Gain +30 morale for six hours. Ongoing causes can return." } },
    { "field_recovery", { "Field Recovery", "Heal one selected nonbroken body part by 40% of its maximum HP and stop bleeding there." } },
    { "boneknit", { "Boneknit", "Mend one selected broken limb and restore it to 50% HP." } },
    { "second_wind", { "Second Wind", "Clear sleepiness and sleep deprivation; refill stamina. Time does not advance." } },
    { "nourished", { "Nourished", "Restore deficient calories and nutritional vitamins to healthy levels; relieve hunger and thirst. Preserve existing excesses and non-nutrient counters." } },
    { "complete_recovery", { "Complete Recovery", "Restore existing body parts, mend fractures, stop bleeding, recover rest, stamina and deficient nutrition; apply Fresh Start. Diseases and radiation remain." } },
    { "warm_up", { "Warm-up", "Refill stamina." } },
    { "giant_horse_rider", { "No More Horsing Around", "Permanent: travel 15% faster while riding a tamed giant horse. Combat and other mounts are unaffected." } }
};
// Construct and validate the whole parcel before accepting a claim or delivery.
// A default container must never silently truncate an oversized liquid award.
bool prepare_item( const item_award &award, item &result )
{
    if( !award.item.is_valid() || award.count < 1 || award.count > 10000 || award.charges < -1 ) {
        return false;
    }
    result = item( award.item, calendar::turn );
    if( award.charges >= 0 ) {
        if( result.count_by_charges() ) {
            result.charges = award.charges;
        } else {
            if( result.ammo_default().is_null() ) {
                // Battery tools can have no ammo type until a compatible cell
                // is inserted. Use their real default magazine, not a guessed
                // battery size or a synthetic charge counter.
                const itype_id magazine_id = result.magazine_default();
                if( magazine_id.is_null() || !magazine_id.is_valid() ) {
                    return false;
                }
                item magazine( magazine_id, calendar::turn );
                if( magazine.ammo_default().is_null() ) {
                    return false;
                }
                magazine.ammo_set( magazine.ammo_default(), award.charges );
                if( magazine.ammo_remaining() != award.charges ||
                    !result.put_in( magazine, pocket_type::MAGAZINE_WELL, false, nullptr, true ).success() ) {
                    return false;
                }
            } else {
                result.ammo_set( result.ammo_default(), award.charges );
            }
            if( result.ammo_remaining() != award.charges ) {
                return false;
            }
        }
    }
    result.set_var( "astral_reward", "yes" );
    const itype_id container_id = award.container.is_empty() ?
                                  result.type->default_container.value_or( itype_id::NULL_ID() ) : award.container;
    if( !container_id.is_null() ) {
        if( !container_id.is_valid() ) {
            return false;
        }
        item container( container_id, calendar::turn );
        if( award.container.is_empty() && result.type->default_container_variant ) {
            container.set_itype_variant( *result.type->default_container_variant );
        }
        if( !container.can_contain_directly( result ).success() ||
            !container.put_in( result, pocket_type::CONTAINER, false, nullptr, true ).success() ) {
            return false;
        }
        container.seal();
        container.set_var( "astral_reward", "yes" );
        result = std::move( container );
    }
    return true;
}
bool valid_option( const option &o )
{
    if( o.items.empty() && o.credits.empty() ) {
        return false;
    }
    for( const item_award &i : o.items ) {
        item sample;
        if( !prepare_item( i, sample ) ) {
            return false;
        }
    }
    for( const auto &c : o.credits ) {
        if( !recovery_types.count( c.first ) || c.second < 1 || c.second > 100 ) {
            return false;
        }
    }
    return true;
}
bool nourished_needed( const Character &who )
{
    if( who.get_stored_kcal() < who.get_healthy_kcal() || who.get_hunger() > 0 ||
        who.get_thirst() > 0 ) {
        return true;
    }
    for( const auto &vit : vitamin::all() ) {
        if( vit.second.type() == vitamin_type::VITAMIN && who.vitamin_get( vit.first ) < 0 ) {
            return true;
        }
    }
    return false;
}
void nourish( Character &who )
{
    who.set_stored_kcal( std::max( who.get_stored_kcal(), who.get_healthy_kcal() ) );
    who.set_hunger( std::min( 0, who.get_hunger() ) );
    who.set_thirst( std::min( 0, who.get_thirst() ) );
    for( const auto &vit : vitamin::all() ) {
        if( vit.second.type() == vitamin_type::VITAMIN && who.vitamin_get( vit.first ) < 0 ) {
            who.vitamin_set( vit.first, 0 );
        }
    }
}
void rest( Character &who )
{
    who.set_sleepiness( std::min( 0, who.get_sleepiness() ) );
    who.set_sleep_deprivation( 0 );
    who.set_stamina( who.get_stamina_max() );
}
void fresh_start( Character &who )
{
    who.clear_temporary_negative_morale();
    who.add_morale( morale_type( "morale_astral_achievement" ), 30, 30, 6_hours, 6_hours, true );
}
} // namespace

bool applying_reward()
{
    return applying;
}

void item_award::serialize( JsonOut &js ) const
{
    js.start_object();
    js.member( "item", item );
    js.member( "count", count );
    js.member( "charges", charges );
    js.member( "container", container );
    js.end_object();
}
void item_award::deserialize( const JsonObject &jo )
{
    item = itype_id( jo.get_string( "item" ) );
    count = jo.get_int( "count", 1 );
    charges = jo.get_int( "charges", -1 );
    container = itype_id( jo.get_string( "container", "" ) );
}
void option::serialize( JsonOut &js ) const
{
    js.start_object();
    js.member( "name", name );
    js.member( "items", items );
    js.member( "credits", credits );
    js.end_object();
}
void option::deserialize( const JsonObject &jo )
{
    name = jo.get_string( "name" );
    jo.read( "items", items );
    jo.read( "credits", credits );
}
void load( const JsonObject &jo )
{
    definition d;
    d.achievement = jo.get_string( "achievement" );
    d.category = jo.get_string( "category", "Milestones" );
    d.art = jo.get_string( "art", d.achievement );
    d.requires_monster = mtype_id( jo.get_string( "requires_monster", "" ) );
    d.enroll = jo.get_bool( "enroll", false );
    jo.read( "choices", d.choices );
    if( d.art.find_first_not_of( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-" ) !=
        std::string::npos ) {
        jo.throw_error( "Artwork key must be a plain filename without extension" );
    }
    expansion_enabled = expansion_enabled || d.enroll;
    definitions[d.achievement] = std::move( d );
}
void reset()
{
    definitions.clear();
    expansion_enabled = false;
}
const std::map<std::string, definition> &all()
{
    return definitions;
}
const definition *find( const std::string &id )
{
    auto it = definitions.find( id );
    return it == definitions.end() ? nullptr : &it->second;
}
bool available( const std::string &id )
{
    const definition *d = find( id );
    return !d || d->requires_monster.is_empty() || d->requires_monster.is_valid();
}
void check()
{
    for( const auto &entry : definitions ) {
        if( !achievement_id( entry.first ).is_valid() || entry.second.choices.empty() ) {
            debugmsg( "Invalid achievement reward definition %s", entry.first );
        }
        for( const option &o : entry.second.choices ) {
            if( !valid_option( o ) ) {
                debugmsg( "Invalid achievement reward option in %s: %s", entry.first, o.name );
            }
        }
    }
}
std::string credit_name( const std::string &id )
{
    auto it = recovery_types.find( id );
    return it == recovery_types.end() ? id : _( it->second.first );
}
std::string credit_description( const std::string &id )
{
    auto it = recovery_types.find( id );
    return it == recovery_types.end() ? _( "This saved reward is currently unavailable." ) : _
           ( it->second.second );
}
std::string describe( const option &o )
{
    std::string result;
    for( const item_award &i : o.items ) {
        result += string_format( "%d × %s", i.count,
                                 i.item.is_valid() ? item::nname( i.item ) : i.item.str() );
        if( i.charges >= 0 ) {
            result += string_format( _( " (%d charges each)" ), i.charges );
        }
        if( !i.container.is_empty() && i.container.is_valid() ) {
            result += string_format( _( " in %s" ), item::nname( i.container ) );
        }
        result += "\n";
        if( i.item.is_valid() && i.item.str().rfind( "astral_", 0 ) == 0 ) {
            result += i.item->description.translated() + "\n";
        }
    }
    for( const auto &c : o.credits ) {
        result += string_format( "%d × %s\n%s\n", c.second, credit_name( c.first ),
                                 credit_description( c.first ) );
    }
    return result;
}
bool bank::claim( const achievements_tracker &tracker, const std::string &id, size_t choice )
{
    const definition *d = find( id );
    if( !tracker.is_enabled() || !available( id ) || !d || choice >= d->choices.size() ||
        claims.count( id ) ||
        tracker.is_completed( achievement_id( id ) ) != achievement_completion::completed ||
        !valid_option( d->choices[choice] ) ) {
        return false;
    }
    const option &o = d->choices[choice];
    claims.emplace( id, o );
    parcels.insert( parcels.end(), o.items.begin(), o.items.end() );
    for( const auto &c : o.credits ) {
        if( c.first == "giant_horse_rider" ) {
            benefits.insert( c.first );
        } else {
            credits[c.first] += c.second;
        }
    }
    return true;
}
bool bank::redeem( Character &who, const std::string &credit, const bodypart_id &part )
{
    auto balance = credits.find( credit );
    if( balance == credits.end() || balance->second <= 0 ) {
        return false;
    }
    const std::vector<bodypart_id> parts = who.get_all_body_parts( get_body_part_flags::only_main );
    const bool valid_part = std::find( parts.begin(), parts.end(), part ) != parts.end();
    reward_guard guard;
    if( credit == "perk_point" || credit == "martial_point" || credit == "playstyle_point" ) {
        if( !who.is_avatar() ) {
            return false;
        }
        const bool martial = credit == "martial_point";
        const effect_on_condition_id init( martial ? "EOC_give_ma_perk_menu" : "EOC_give_perk_menu" );
        if( !init.is_valid() ) {
            return false;
        }
        dialogue d( get_talker_for( who ), nullptr );
        init->activate( d );
        const std::string key = martial ? "num_ma_perks" : credit == "perk_point" ?
                                "num_perks" : "playstyle_perks_available";
        const diag_value &old = who.get_value( key );
        const double points = old.is_empty() ? 0 : old.dbl();
        who.set_value( key, points + 1 );
    } else if( credit == "reading_desk_plans" ) {
        const recipe_id plans( "astral_archivist_desk" );
        if( !plans.is_valid() || who.knows_recipe( &plans.obj() ) ) {
            return false;
        }
        who.learn_recipe( &plans.obj() );
    } else if( credit == "field_recovery" ) {
        if( !valid_part || who.is_limb_broken( part ) ||
            ( who.get_part_hp_cur( part ) >= who.get_part_hp_max( part ) &&
              !who.has_effect( efftype_id( "bleed" ), part ) ) ) {
            return false;
        }
        who.set_part_hp_cur( part, std::min( who.get_part_hp_max( part ), who.get_part_hp_cur( part ) +
                                             static_cast<int>( std::ceil( who.get_part_hp_max( part ) * 0.4 ) ) ) );
        who.remove_effect( efftype_id( "bleed" ), part );
    } else if( credit == "boneknit" ) {
        if( !valid_part || !who.is_limb_broken( part ) ) {
            return false;
        }
        who.set_part_hp_cur( part, std::max( 1, who.get_part_hp_max( part ) / 2 ) );
        who.remove_effect( efftype_id( "mending" ), part );
        who.recalc_limb_energy_usage();
    } else if( credit == "second_wind" ) {
        if( who.get_sleepiness() <= 0 && who.get_sleep_deprivation() <= 0 &&
            who.get_stamina() >= who.get_stamina_max() ) {
            return false;
        }
        rest( who );
    } else if( credit == "nourished" ) {
        if( !nourished_needed( who ) ) {
            return false;
        }
        nourish( who );
    } else if( credit == "fresh_start" ) {
        if( !who.has_temporary_negative_morale() &&
            who.has_morale( morale_type( "morale_astral_achievement" ) ) >= 30 ) {
            return false;
        }
        fresh_start( who );
    } else if( credit == "warm_up" ) {
        if( who.get_stamina() >= who.get_stamina_max() ) {
            return false;
        }
        who.set_stamina( who.get_stamina_max() );
    } else if( credit == "complete_recovery" ) {
        bool needed = nourished_needed( who ) || who.get_sleepiness() > 0 ||
                      who.get_sleep_deprivation() > 0 ||
                      who.get_stamina() < who.get_stamina_max() || who.has_temporary_negative_morale() ||
                      who.has_morale( morale_type( "morale_astral_achievement" ) ) < 30;
        for( const bodypart_id &bp : parts ) {
            needed |= who.get_part_hp_cur( bp ) < who.get_part_hp_max( bp ) ||
                      who.has_effect( efftype_id( "bleed" ), bp );
        }
        if( !needed ) {
            return false;
        }
        for( const bodypart_id &bp : parts ) {
            who.set_part_hp_cur( bp, who.get_part_hp_max( bp ) );
            who.remove_effect( efftype_id( "mending" ), bp );
        }
        who.remove_effect( efftype_id( "bleed" ) );
        who.recalc_limb_energy_usage();
        nourish( who );
        rest( who );
        fresh_start( who );
    } else {
        return false;
    }
    --balance->second;
    return true;
}
bool bank::deliver( Character &who, size_t index, bool at_feet )
{
    if( index >= parcels.size() ) {
        return false;
    }
    item_award &p = parcels[index];
    reward_guard guard;
    item reward;
    if( !prepare_item( p, reward ) ) {
        return false;
    }
    if( at_feet ) {
        map &here = get_map();
        if( !here.can_put_items( who.pos_bub() ) ||
            here.has_flag( ter_furn_flag::TFLAG_DESTROY_ITEM, who.pos_bub() ) ||
            here.free_volume( who.pos_bub() ) < reward.volume() || ( here.has_items( who.pos_bub() ) &&
                    here.i_at( who.pos_bub() ).size() >= 4095 ) ) {
            return false;
        }
        if( here.add_item_or_charges( who.pos_bub(), reward, false ).is_null() ) {
            return false;
        }
    } else {
        if( !who.can_pickWeight( reward ) || !who.can_stash( reward ) ) {
            return false;
        }
        if( !who.i_add( reward ) ) {
            return false;
        }
    }
    if( --p.count == 0 ) {
        parcels.erase( parcels.begin() + index );
    }
    return true;
}
void bank::serialize( JsonOut &js ) const
{
    js.start_object();
    js.member( "version", 1 );
    js.member( "claims", claims );
    js.member( "credits", credits );
    js.member( "parcels", parcels );
    js.member( "benefits", benefits );
    js.member( "counters", counters );
    js.member( "seen", seen );
    js.end_object();
}
void bank::deserialize( const JsonObject &jo )
{
    *this = bank();
    jo.get_int( "version", 1 );
    jo.read( "claims", claims );
    jo.read( "credits", credits );
    jo.read( "parcels", parcels );
    jo.read( "benefits", benefits );
    jo.read( "counters", counters );
    jo.read( "seen", seen );
}
void complete( const std::string &id )
{
    achievements_tracker &tracker = get_achievements();
    const achievement_id a( id );
    if( applying || !tracker.is_enabled() || !a.is_valid() || !available( id ) ||
        tracker.is_completed( a ) != achievement_completion::pending ) {
        return;
    }
    tracker.enroll_astral();
    tracker.report_achievement( &a.obj(), achievement_completion::completed );
}
namespace
{
bool is_bed( const std::string &f )
{
    static const std::set<std::string> beds = { "f_bed", "f_bed_down", "f_bunkbed", "f_bunkbed_down", "f_triple_bunkbed", "f_makeshift_bed", "f_straw_bed" };
    return beds.count( f );
}
}
void on_sleep( Character &who )
{
    if( !expansion_enabled ) {
        return;
    }
    if( !who.is_avatar() || !who.has_effect( efftype_id( "sleep" ) ) || applying ||
        !get_achievements().is_enabled() ) {
        return;
    }
    auto &b = get_achievements().reward_bank;
    b.counters["sleep_start"] = to_turns<int>( calendar::turn - calendar::turn_zero );
    b.counters["sleep_deprivation_start"] = who.get_sleep_deprivation();
    b.seen["sleep_position"] = { who.pos_abs().to_string() };
    const bool bed = is_bed( get_map().furn( who.pos_bub() ).id().str() );
    b.counters["sleep_built_bed"] = bed && b.seen["built_beds"].count( who.pos_abs().to_string() );
    if( bed && who.worn.empty() ) {
        complete( "astral_014" );
    }
}
void on_wake( Character &who )
{
    if( !expansion_enabled ) {
        return;
    }
    if( !who.is_avatar() || applying || !get_achievements().is_enabled() ) {
        return;
    }
    auto &b = get_achievements().reward_bank;
    auto start = b.counters.find( "sleep_start" );
    if( start == b.counters.end() ) {
        return;
    }
    const int duration = to_turns<int>( calendar::turn - calendar::turn_zero ) - start->second;
    b.counters.erase( start );
    if( duration >= to_turns<int>( 6_hours ) &&
        b.seen["sleep_position"].count( who.pos_abs().to_string() ) ) {
        const std::string day = std::to_string( to_days<int>( calendar::turn - calendar::turn_zero ) );
        b.seen["six_hour_sleep_days"].insert( day );
        if( b.seen["six_hour_sleep_days"].size() >= 7 ) {
            complete( "astral_002" );
        }
        if( b.counters["sleep_built_bed"] && is_bed( get_map().furn( who.pos_bub() ).id().str() ) ) {
            complete( "astral_001" );
        }
    }
    if( duration > 0 && b.counters["sleep_deprivation_start"] > 0 &&
        who.get_sleep_deprivation() <= 0 ) {
        if( ++b.counters["rested_episodes"] >= 3 ) {
            complete( "astral_209" );
        }
    }
}
void on_construction( Character &who, const construction &built, const tripoint_abs_ms &pos )
{
    if( !expansion_enabled ) {
        return;
    }
    if( !who.is_avatar() || applying || !get_achievements().is_enabled() ) {
        return;
    }
    auto &b = get_achievements().reward_bank;
    b.seen["construction_stages"].insert( pos.to_string() + ":" + built.id.str() );
    if( built.post_is_furniture && is_bed( built.post_terrain ) ) {
        b.seen["built_beds"].insert( pos.to_string() );
    }
    const size_t stages = b.seen["construction_stages"].size();
    if( stages >= 10 ) {
        complete( "astral_021" );
    }
    if( stages >= 50 ) {
        complete( "astral_022" );
    }
}
void mark_kiln_batch( Character &who, item &result )
{
    if( !expansion_enabled ) {
        return;
    }
    if( !who.is_avatar() || who.is_fake() || applying || !get_achievements().is_enabled() ) {
        return;
    }
    auto &b = get_achievements().reward_bank;
    const std::string batch = who.pos_abs().to_string() + ":" +
                              std::to_string( to_turns<int>( calendar::turn - calendar::turn_zero ) ) + ":" +
                              std::to_string( ++b.counters["kiln_batch_serial"] );
    b.seen["kiln_started"].insert( batch );
    result.set_var( "astral_kiln_batch", batch );
}
void on_item_acquire( Character &who, const item &it )
{
    if( !expansion_enabled ) {
        return;
    }
    if( !who.is_avatar() || who.is_fake() || applying || !get_achievements().is_enabled() ||
        it.typeId() != itype_id( "charcoal" ) || it.charges <= 0 || it.has_var( "astral_reward" ) ) {
        return;
    }
    auto &b = get_achievements().reward_bank;
    const std::string batch = it.get_var( "astral_kiln_batch" );
    if( !batch.empty() && b.seen["kiln_started"].count( batch ) ) {
        b.seen["kiln_collected"].insert( batch );
        if( b.seen["kiln_collected"].size() >= 3 ) {
            complete( "astral_025" );
        }
    }
}
void on_tame( Character &who, const monster &mon )
{
    if( !expansion_enabled ) {
        return;
    }
    if( who.is_avatar() && !mon.is_hallucination() && mon.type->id.str() == "mon_giant_horse" &&
        mon.has_effect( efftype_id( "pet" ) ) ) {
        complete( "astral_080" );
    }
}
void on_block( Character &who, const Creature &source, float damage_blocked )
{
    if( !expansion_enabled ) {
        return;
    }
    if( !who.is_avatar() || who.is_fake() || applying || !get_achievements().is_enabled() ||
        source.is_hallucination() || source.attitude_to( who ) != Creature::Attitude::HOSTILE ||
        damage_blocked <= 0 ) {
        return;
    }
    int &blocks = get_achievements().reward_bank.counters["hostile_blocks"];
    blocks = std::min( 100, blocks + 1 );
    if( blocks == 100 ) {
        complete( "astral_084" );
    }
}
void on_craft( Character &who, const recipe &making, const std::vector<item> &results )
{
    if( !expansion_enabled ) {
        return;
    }
    if( !who.is_avatar() || who.is_fake() || applying || !get_achievements().is_enabled() ||
        making.is_practice() || results.empty() || calendar::eternal_day() ||
        ( !calendar::eternal_night() && is_day( calendar::turn ) ) ) {
        return;
    }
    // One completed activity, regardless of batch size. This hook is not called
    // when starting, cancelling, or failing an unfinished craft.
    int &crafts = get_achievements().reward_bank.counters["night_crafts"];
    crafts = std::min( 25, crafts + 1 );
    if( crafts == 25 ) {
        complete( "astral_136" );
    }
}
void on_natural_healing( Character &who, int recovered_hp )
{
    if( !expansion_enabled ) {
        return;
    }
    if( !who.is_avatar() || who.is_fake() || applying || !get_achievements().is_enabled() ||
        recovered_hp <= 0 || !who.has_effect( efftype_id( "sleep" ) ) ||
        !is_bed( get_map().furn( who.pos_bub() ).id().str() ) ) {
        return;
    }
    int &healed = get_achievements().reward_bank.counters["bed_healing"];
    healed += std::min( recovered_hp, 100 - healed );
    if( healed == 100 ) {
        complete( "astral_199" );
    }
}
double chopping_time_multiplier( const item &tool )
{
    if( tool.typeId() == itype_id( "astral_lumberjack_axe" ) ) {
        return 1.0 / 1.25;
    }
    if( tool.typeId() == itype_id( "astral_forester_axe" ) ) {
        return 1.0 / 1.4;
    }
    return 1.0;
}
double mounted_move_multiplier( const Character &who, const monster &mount )
{
    return expansion_enabled && who.is_avatar() && mount.type->id.str() == "mon_giant_horse" &&
           mount.has_effect( efftype_id( "pet" ) ) &&
           get_achievements().reward_bank.benefits.count( "giant_horse_rider" ) ? 1.0 / 1.15 : 1.0;
}
} // namespace achievement_rewards
