#pragma once
#ifndef CATA_SRC_TACTICAL_COMBAT_H
#define CATA_SRC_TACTICAL_COMBAT_H

#include <string>
#include <vector>

class avatar;
class Character;
class Creature;
class monster;

namespace tactical_combat
{
enum class action { attack, guard, evade, bash, recover };
struct assessment {
    bool available = false;
    int moves = 0;
    int stamina = 0;
    std::string reason;
};
std::string name( action choice );
std::string description( action choice );
void clear_target();
Creature *selected_target( avatar &who );
bool select_target( avatar &who, Creature *target );
assessment assess( Character &who, action choice, Creature *target = nullptr );
bool execute( Character &who, action choice, Creature *target = nullptr );
void stop_auto_combat();
// Manual selection includes neutral creatures; automation keeps its hostile-only filter.
bool can_target_from_map( const avatar &who, const Creature *target );
std::vector<Creature *> manual_targets( avatar &who );
assessment assess_manual( avatar &who, action choice, Creature *target = nullptr );
void menu( avatar &who, Creature *target = nullptr );
void manual_action( avatar &who, action choice );
void clear_defense( Character &who );
std::string stance( const Character &who );
action choose_auto_action( Character &who, Creature &target, const std::string &profile );
// Called only from the normal melee path. A windup spends an action and returns true.
bool prepare_monster_strike( monster &who, Creature &target, bool &heavy );
} // namespace tactical_combat
#endif
