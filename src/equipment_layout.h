#pragma once
#ifndef CATA_SRC_EQUIPMENT_LAYOUT_H
#define CATA_SRC_EQUIPMENT_LAYOUT_H

/**
 * Presentation model for the anatomical equipment doll.
 * Classification and coverage mapping live here; ImGui rendering stays in
 * rpg_equipment_ui.cpp. Native wear/storage checks remain authoritative.
 */

#include <optional>
#include <string>
#include <vector>

#include "bodypart.h"
#include "enums.h"
#include "item_location.h"

class Character;
class item;

namespace equipment_layout
{

/** Dedicated storage destinations, independent of clothing layer order. */
enum class storage_slot {
    none,
    back,
    scabbard,
    sheath,
    holster
};

storage_slot storage_slot_for( const item &it );

/** Stable anatomical / equipment region IDs for the doll. */
enum class region_id {
    head,
    ear_l,
    ear_r,
    forehead,
    eyes,
    face,
    neck,
    torso,
    arm_l,
    arm_r,
    wrist_l,
    wrist_r,
    hand_l,
    hand_r,
    rings_l,
    rings_r,
    waist,
    pants,
    leg_lower_l,
    leg_lower_r,
    foot_l,
    foot_r,
    back_bag,
    back_weapon,
    main_hand,
    off_hand,
    fallback,
    num_regions
};

enum class region_side {
    none,
    left,
    right,
    both
};

/**
 * Compact doll layer labels.
 * skin/middle/outer map to Close to skin / Normal / Outer.
 * uncommon exposes Personal / Waist / Strapped / Aura (and unmatched layers).
 */
enum class display_layer {
    skin,
    middle,
    outer,
    uncommon,
    any
};

enum class equipment_role {
    clothing,
    accessory,
    holder,
    bag,
    strapped_large,
    held,
    fallback
};

/** One visible/logical position on the doll. */
struct region_ref {
    region_id id = region_id::fallback;
    display_layer layer = display_layer::any;
    int position_index = 0; // waist overflow / ring expansion index
};

/** Static catalog entry for UI labels and layout. */
struct region_info {
    region_id id;
    region_side side;
    const char *label; // English source label; UI translates
    bool skin_middle_outer; // offers compact layer controls
    bool accessory_position;
};

const std::vector<region_info> &region_catalog();
const region_info &info_for( region_id id );
std::string region_label( region_id id );
display_layer to_display_layer( layer_level layer );
layer_level to_native_layer( display_layer layer );
const char *display_layer_label( display_layer layer );

/**
 * One worn/held item as seen by the doll.
 * Linked coverage shares this single authoritative item_location.
 */
struct mapped_item {
    item_location location;
    region_ref equip_destination;
    std::vector<region_ref> covered_regions;
    equipment_role role = equipment_role::clothing;
    storage_slot storage = storage_slot::none;
    /** When set, location is content stored inside this holder. */
    item_location holder_location;
    /** True when equip_destination is a best-effort fallback list entry. */
    bool uses_fallback = false;
};

/** Prospective classification of an item type/instance (no Character required). */
struct item_profile {
    region_ref equip_destination;
    std::vector<region_ref> covered_regions;
    equipment_role role = equipment_role::clothing;
    storage_slot storage = storage_slot::none;
    bool uses_fallback = false;
};

item_profile classify_item( const item &it );

/**
 * Full mapping for a character: every worn item exactly once as ownership,
 * plus optional held contents resolved to exact holders.
 */
struct character_map {
    std::vector<mapped_item> items;
    std::vector<item_location> unmapped_fallback;
};

character_map map_character( Character &you );

/** Items whose equip destination is this region (optionally filtered by layer). */
std::vector<item_location> items_equipping_to( const character_map &map,
        region_id id, display_layer layer = display_layer::any );

/** Items that cover this region (includes linked garments). */
std::vector<item_location> items_covering( const character_map &map,
        region_id id, display_layer layer = display_layer::any );

/** All covered region refs for a given authoritative location. */
std::vector<region_ref> coverage_for( const character_map &map,
                                      const item_location &loc );

/** Whether dropping `it` onto equip destination `dest` is a plausible UI target. */
bool is_plausible_drop_destination( const item &it, region_id dest,
                                    display_layer layer = display_layer::any );

/**
 * Items that should appear as occupants of a region list.
 * Clothing regions merge equip destinations with linked coverage; accessory
 * regions only show true accessories (helmet ear coverage is highlight-only).
 */
std::vector<item_location> items_visible_on( const character_map &map,
        region_id id, display_layer layer = display_layer::any );

/**
 * Anatomical regions the doll should show for this character after the limb
 * filter (always includes main/off hand via the hand adapter contract).
 */
std::vector<region_id> visible_regions_for( const Character &you );

/**
 * Preferred native wear side for a sided doll region, if any.
 */
std::optional<side> side_intent_for( region_id id );

/**
 * Renderer-facing hand adapter.
 * Does not own or copy items. Initially: current main weapon + shield-only off-hand.
 * Codex will replace the off-hand resolution with the shared hand API later.
 */
struct hand_occupancy {
    item_location main_hand;
    item_location off_hand; // worn shield occupying off-hand presentation
    bool main_reserves_both_hands = false;
};

hand_occupancy hands_for( Character &you );

} // namespace equipment_layout

#endif // CATA_SRC_EQUIPMENT_LAYOUT_H
