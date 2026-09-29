#include "tactical_combat.h"

#include <algorithm>
#include <array>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "creature.h"
#include "damage.h"
#include "event.h"
#include "event_bus.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "melee.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "memory_fast.h"
#include "ui_telemetry.h"

#include "ret_val.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"


namespace tactical_combat
{
static const efftype_id guard_effect( "astral_guard" );
static const efftype_id evade_effect( "astral_evade" );
static const efftype_id recover_effect( "astral_recover" );
static const efftype_id windup_effect( "astral_windup" );
static const efftype_id cooldown_effect( "astral_heavy_recovery" );
static const flag_id shield_flag( "ASTRAL_SHIELD" );
static const skill_id melee_skill( "melee" );
static const mon_flag_str_id heavy_flag( "ASTRAL_HEAVY_STRIKE" );
static weak_ptr_fast<Creature> hud_target;
static weak_ptr_fast<Creature> confirmed_target;

void clear_target()
{
    hud_target.reset();
    confirmed_target.reset();
}

Creature *selected_target( avatar &who )
{
    const shared_ptr_fast<Creature> target = hud_target.lock();
    if( !target || target->is_dead_state() || !who.sees( get_map(), *target ) ||
        !get_option<bool>( "TACTICAL_COMBAT" ) ) {
        clear_target();
        return nullptr;
    }
    return target.get();
}

bool select_target( avatar &who, Creature *target )
{
    if( !target || target == &who || target->is_dead_state() ||
        !get_option<bool>( "TACTICAL_COMBAT" ) || !who.sees( get_map(), *target ) ) {
        return false;
    }
    if( selected_target( who ) != target ) {
        confirmed_target.reset();
    }
    hud_target = g->shared_from( *target );
    return !hud_target.expired();
}

std::string name( action choice )
{
    switch( choice ) {
        case action::attack:
            return _( "Attack" );
        case action::guard:
            return _( "Guard" );
        case action::evade:
            return _( "Evade" );
        case action::bash:
            return _( "Shield bash" );
        case action::recover:
            return _( "Recover" );
    }
    return {};
}

void clear_defense( Character &who )
{
    who.remove_effect( guard_effect );
    who.remove_effect( evade_effect );
    who.remove_effect( recover_effect );
}

std::string stance( const Character &who )
{
    if( who.has_effect( guard_effect ) ) {
        return _( "Guarding: stronger shield block" );
    }
    if( who.has_effect( evade_effect ) ) {
        return _( "Evading: improved dodge" );
    }
    if( who.has_effect( recover_effect ) ) {
        return _( "Recovering: defense reduced" );
    }
    return _( "Ready" );
}

assessment assess( Character &who, action choice, Creature *target )
{
    assessment result;
    if( !get_option<bool>( "TACTICAL_COMBAT" ) ) {
        result.reason = _( "Tactical combat is disabled in options." );
        return result;
    }
    if( who.is_dead_state() || who.in_sleep_state() || who.has_effect( efftype_id( "narcosis" ) ) ||
        who.has_effect( efftype_id( "stunned" ) ) || who.has_effect( efftype_id( "fearparalyze" ) ) ||
        who.has_effect( efftype_id( "incorporeal" ) ) || who.is_mounted() || who.is_driving() ) {
        result.reason = _( "You cannot perform this combat action in your current state." );
        return result;
    }
    item_location shield = who.best_shield();
    switch( choice ) {
        case action::attack: {
            item_location weapon = who.used_weapon();
            result.moves = who.attack_speed( weapon ? *weapon : null_item_reference() );
            result.stamina = -who.get_total_melee_stamina_cost( weapon ? weapon.get_item() : nullptr );
            break;
        }
        case action::guard:
            result.moves = 100;
            result.stamina = 150;
            break;
        case action::evade:
            result.moves = 100;
            result.stamina = 200;
            if( !who.can_try_dodge( true ).success() || !who.enough_working_legs() ||
                who.has_effect( efftype_id( "downed" ) ) ) {
                result.reason = _( "You cannot prepare to evade in your current condition." );
            }
            break;
        case action::bash:
            result.moves = shield ? std::max( 100, who.attack_speed( *shield ) ) : 100;
            result.stamina = shield ? -who.get_standard_stamina_cost( shield.get_item() ) : 200;
            break;
        case action::recover:
            result.moves = 150;
            result.stamina = -std::max( 1, who.get_stamina_max() / 20 );
            if( who.get_stamina() == who.get_stamina_max() ) {
                result.reason = _( "Your stamina is already full." );
            }
            break;
    }
    if( choice == action::guard || choice == action::bash ) {
        if( !shield || !shield->has_flag( shield_flag ) || !who.is_worn( *shield ) ||
            !who.has_two_arms_lifting() ||
            ( who.get_wielded_item() && who.get_wielded_item()->is_two_handed( who ) ) ) {
            result.reason = _( "Requires an equipped shield and a usable shield hand." );
        } else if( who.get_stamina() < 2000 + result.stamina ) {
            result.reason = _( "Too exhausted to use your shield effectively." );
        }
    }
    if( result.reason.empty() && ( choice == action::attack || choice == action::bash ) ) {
        if( who.has_flag( flag_id( "CANNOT_ATTACK" ) ) ) {
            result.reason = _( "You are incapable of attacking." );
        } else if( !target || target == &who || target->is_dead_state() ||
                   !who.is_adjacent( target, false ) || !who.sees( get_map(), *target ) ||
                   !who.can_reach_attack( *target ) ) {
            result.reason = _( "Choose a visible adjacent target." );
        } else if( target->is_underwater() && !who.is_underwater() ) {
            result.reason = _( "You cannot reach a submerged target with this action." );
        }
    }
    if( result.reason.empty() && result.stamina > who.get_stamina() ) {
        result.reason = _( "Not enough stamina." );
    }
    result.available = result.reason.empty();
    return result;
}

bool execute( Character &who, action choice, Creature *target )
{
    const assessment check = assess( who, choice, target );
    if( !check.available ) {
        who.add_msg_if_player( m_info, "%s", check.reason );
        return false;
    }
    if( who.is_avatar() && !g->check_safe_mode_allowed() ) {
        return false;
    }
    clear_defense( who );
    if( choice == action::attack ) {
        return who.melee_attack( *target, true );
    }
    who.recoil = MAX_RECOIL;
    who.last_target_pos = std::nullopt;
    who.set_activity_level( choice == action::recover ? NO_EXERCISE : MODERATE_EXERCISE );
    who.mod_moves( -check.moves );
    who.mod_stamina( -check.stamina );
    if( choice == action::guard ) {
        who.add_effect( guard_effect, 2_turns );
    } else if( choice == action::evade ) {
        who.add_effect( evade_effect, 2_turns );
    } else if( choice == action::recover ) {
        who.add_effect( recover_effect, 2_turns );
    } else {
        // Explicit shield attack: never swap ownership with the main-hand weapon.
        item_location shield = who.best_shield();
        const itype_id shield_type = shield->typeId();
        who.set_activity_level( EXTRA_EXERCISE );
        const int spread = target->deal_melee_attack( &who,
                           melee::melee_hit_range( who.get_hit_base() + who.get_hit_weapon( *shield ) ) );
        sfx::generate_melee_sound( *shield, who.pos_bub(), target->pos_bub(), spread >= 0, false );
        dealt_damage_instance dealt;
        if( spread >= 0 ) {
            damage_instance damage( damage_type_id( "bash" ),
                                    shield->damage_melee( damage_type_id( "bash" ) ) + who.get_arm_str() / 2.0f );
            target->deal_melee_hit( &who, spread, false, damage, dealt );
            if( dealt.total_damage() > 0 && target->get_size() <= who.get_size() + 1 &&
                !target->is_immune_effect( efftype_id( "stunned" ) ) &&
                x_in_y( 25 + 5 * who.get_skill_level( melee_skill ), 100 ) ) {
                target->remove_effect( windup_effect );
                target->remove_value( "astral_strike_target" );
                target->add_effect( cooldown_effect, 4_turns );
                target->add_effect( efftype_id( "staggered" ), 2_turns );
                target->mod_moves( -50 );
                who.add_msg_if_player( m_good, _( "Your shield bash interrupts %s!" ), target->disp_name() );
            }
        }
        if( target->is_monster() ) {
            const cata::event event = cata::event::make<event_type::character_melee_attacks_monster>
                                      ( who.getID(),
                                        shield_type, spread >= 0, target->as_monster()->type->id );
            get_event_bus().send_with_talker( &who, target, event );
        } else if( target->as_character() != nullptr ) {
            const cata::event event = cata::event::make<event_type::character_melee_attacks_character>
                                      ( who.getID(),
                                        shield_type, spread >= 0, target->as_character()->getID(), target->get_name() );
            get_event_bus().send_with_talker( &who, target, event );
        }
        if( !target->is_hallucination() ) {
            who.handle_melee_wear( shield );
            target->add_effect( efftype_id( "hit_by_player" ), 10_minutes );
            if( target->times_combatted_player < 100 ) {
                who.practice( melee_skill, spread >= 0 ? 3 : 1 );
                ++target->times_combatted_player;
            }
        }
        if( npc *person = target->as_npc() ) {
            person->on_attacked( who );
        }
        target->check_dead_state( &get_map() );
        who.check_dead_state( &get_map() );
        who.add_msg_if_player( spread >= 0 ? m_good : m_warning,
                               _( "Shield bash against %1$s: %2$d damage." ),
                               target->disp_name(), dealt.total_damage() );
        return true;
    }
    who.add_msg_if_player( m_info, "%s", stance( who ) );
    return true;
}

action choose_auto_action( Character &who, Creature &target, const std::string &profile )
{
    if( target.has_effect( windup_effect ) ) {
        if( profile == "aggressive" && assess( who, action::bash, &target ).available ) {
            return action::bash;
        }
        if( assess( who, action::guard ).available ) {
            return action::guard;
        }
        if( assess( who, action::evade ).available ) {
            return action::evade;
        }
    }
    const int recover_percent = profile == "defensive" ? 55 : profile == "aggressive" ? 25 : 40;
    if( who.get_stamina() * 100 < who.get_stamina_max() * recover_percent &&
        assess( who, action::recover ).available ) {
        return action::recover;
    }
    return action::attack;
}

bool can_target_from_map( const avatar &who, const Creature *target )
{
    if( !get_option<bool>( "TACTICAL_COMBAT" ) || !target || target == &who ||
        target->is_dead_state() ||
        who.pos_bub() == target->pos_bub() || !who.sees( get_map(), *target ) ) {
        return false;
    }
    const monster *mon = target->as_monster();
    return ( mon && mon->friendly == 0 && !mon->has_effect( efftype_id( "pet" ) ) ) ||
           target->attitude_to( who ) == Creature::Attitude::HOSTILE;
}

assessment assess_manual( avatar &who, action choice, Creature *target )
{
    assessment check = assess( who, choice, target );
    if( check.available && ( choice == action::attack || choice == action::bash ) &&
        target->attitude_to( who ) != Creature::Attitude::HOSTILE && g->safe_mode == SAFE_MODE_ON ) {
        check.available = false;
        check.reason = _( "Safe mode prevents attacking a non-hostile target." );
    }
    return check;
}

std::vector<Creature *> manual_targets( avatar &who )
{
    return g->get_creatures_if( [&who]( const Creature & target ) {
        return &target != &who && !target.is_dead_state() &&
               who.pos_bub() != target.pos_bub() && who.is_adjacent( &target, false ) &&
               who.sees( get_map(), target );
    } );
}

static bool perform_manual( avatar &who, action choice, Creature *target )
{
    const assessment check = assess_manual( who, choice, target );
    if( !check.available ) {
        add_msg( m_info, "%s", check.reason );
        return false;
    }
    if( !g->check_safe_mode_allowed() ) {
        return false;
    }
    if( choice == action::attack || choice == action::bash ) {
        if( target->attitude_to( who ) != Creature::Attitude::HOSTILE ) {
            if( g->safe_mode == SAFE_MODE_ON ) {
                add_msg( m_warning, _( "Safe mode prevents attacking a non-hostile target." ) );
                return false;
            }
            if( confirmed_target.lock().get() != target &&
                !query_yn( _( "Really attack %s?" ), target->disp_name() ) ) {
                return false;
            }
        }
        if( !who.try_break_relax_gas( _( "Your willpower asserts itself, and so do you!" ),
                                      _( "You're too pacified to strike anything…" ) ) ) {
            return false;
        }
    }
    const bool completed = execute( who, choice, target );
    if( completed && target && ( choice == action::attack || choice == action::bash ) ) {
        confirmed_target = g->shared_from( *target );
    }
    ui_telemetry::record( "combat.manual", {{ "action", name( choice ) },
        { "completed", completed ? "true" : "false" }
    } );
    return completed;
}

std::string description( action choice )
{
    switch( choice ) {
        case action::attack:
            return _( "Strike with your current weapon. Weapon and martial-art techniques trigger automatically." );
        case action::guard:
            return _( "Brace your equipped shield for stronger blocking until your next action." );
        case action::evade:
            return _( "Prepare to dodge until your next action. You stay on this tile." );
        case action::bash:
            return _( "Strike with your shield; a damaging hit may stagger and interrupt a heavy strike." );
        case action::recover:
            return _( "Regain stamina. Dodge is reduced and blocking is unavailable until your next action." );
    }
    return {};
}

void stop_auto_combat()
{
    if( get_option<bool>( "AUTO_COMBAT" ) ) {
        get_options().get_option( "AUTO_COMBAT" ).setValue( "false" );
        get_options().save();
    }
}

void menu( avatar &who, Creature *target )
{
    // The legacy combat-menu binding now focuses the HUD without interrupting play.
    stop_auto_combat();
    if( target ) {
        select_target( who, target );
    } else if( !selected_target( who ) ) {
        std::vector<Creature *> targets = manual_targets( who );
        targets.erase( std::remove_if( targets.begin(), targets.end(), [&who]( Creature *candidate ) {
            return candidate->attitude_to( who ) != Creature::Attitude::HOSTILE;
        } ), targets.end() );
        if( targets.size() == 1 ) {
            select_target( who, targets.front() );
        } else {
            add_msg( m_info, _( "Select a creature on the map to use the combat hotbar." ) );
        }
    }
    ui_telemetry::record( "combat.target", {{ "selected", selected_target( who ) ? "true" : "false" }} );
}

void manual_action( avatar &who, action choice )
{
    stop_auto_combat();
    Creature *target = nullptr;
    if( choice == action::attack || choice == action::bash ) {
        if( !selected_target( who ) ) {
            menu( who );
        }
        target = selected_target( who );
        if( !target ) {
            return;
        }
    }
    perform_manual( who, choice, target );
}

bool prepare_monster_strike( monster &who, Creature &target, bool &heavy )
{
    heavy = false;
    if( !get_option<bool>( "TACTICAL_COMBAT" ) || !who.has_flag( heavy_flag.id() ) ||
        who.is_hallucination() ) {
        return false;
    }
    const std::string aimed_tile = target.pos_abs().to_string();
    if( who.has_effect( windup_effect ) ) {
        heavy = who.get_value( "astral_strike_target" ).str() == aimed_tile;
        who.remove_effect( windup_effect );
        who.remove_value( "astral_strike_target" );
        who.add_effect( cooldown_effect, 4_turns );
        if( !heavy ) {
            add_msg_if_player_sees( who, m_info, _( "%s loses its opening for a heavy strike." ),
                                    who.disp_name() );
            return true;
        }
        add_msg_if_player_sees( who, m_warning, _( "%s unleashes its heavy strike!" ), who.disp_name() );
        return false;
    }
    if( who.has_effect( cooldown_effect ) ) {
        return false;
    }
    who.set_value( "astral_strike_target", aimed_tile );
    who.add_effect( windup_effect, 4_turns );
    // Ensure even a fast creature cannot wind up and hit within this action.
    who.mod_moves( -std::max( who.get_moves(), who.get_speed() ) );
    add_msg_if_player_sees( who, m_warning, _( "%s winds up a heavy strike!" ), who.disp_name() );
    return true;
}
} // namespace tactical_combat
