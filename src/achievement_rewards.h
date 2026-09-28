#pragma once
#ifndef CATA_SRC_ACHIEVEMENT_REWARDS_H
#define CATA_SRC_ACHIEVEMENT_REWARDS_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "type_id.h"
#include "coordinates.h"

class Character;
class Creature;
class recipe;
class JsonObject;
class JsonOut;
class achievements_tracker;
class monster;
class item;
struct construction;

namespace achievement_rewards
{
struct item_award {
    itype_id item;
    int count = 1;
    int charges = -1;
    itype_id container;
    void serialize( JsonOut & ) const;
    void deserialize( const JsonObject & );
};
struct option {
    std::string name;
    std::vector<item_award> items;
    std::map<std::string, int> credits;
    void serialize( JsonOut & ) const;
    void deserialize( const JsonObject & );
};
struct definition {
    std::string achievement;
    std::string category;
    std::string art;
    mtype_id requires_monster;
    bool enroll = false;
    std::vector<option> choices;
};
void load( const JsonObject & );
void reset();
void check();
const definition *find( const std::string &id );
bool available( const std::string &id );
const std::map<std::string, definition> &all();
std::string describe( const option & );
std::string credit_name( const std::string & );
std::string credit_description( const std::string & );

// Saved inside the character's achievements. Pending item deliveries and selected
// payloads are snapshots, so a data update cannot re-credit or change a claim.
class bank
{
    public:
        std::map<std::string, option> claims;
        std::map<std::string, int> credits;
        std::vector<item_award> parcels;
        std::set<std::string> benefits;
        std::map<std::string, int> counters;
        std::map<std::string, std::set<std::string>> seen;
        bool claim( const achievements_tracker &, const std::string &id, size_t choice );
        bool redeem( Character &, const std::string &credit, const bodypart_id &part );
        bool deliver( Character &, size_t parcel, bool at_feet );
        void serialize( JsonOut & ) const;
        void deserialize( const JsonObject & );
};
void complete( const std::string &id );
void on_sleep( Character & );
void on_wake( Character & );
void on_construction( Character &, const construction &, const tripoint_abs_ms & );
void on_item_acquire( Character &, const item & );
void mark_kiln_batch( Character &, item & );
void on_tame( Character &, const monster & );
void on_block( Character &, const Creature &, float damage_blocked );
void on_craft( Character &, const recipe &, const std::vector<item> &results );
void on_natural_healing( Character &, int recovered_hp );
double mounted_move_multiplier( const Character &, const monster & );
bool applying_reward();
} // namespace achievement_rewards
#endif
