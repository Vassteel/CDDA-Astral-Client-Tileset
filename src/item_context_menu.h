#pragma once
#ifndef CATA_SRC_ITEM_CONTEXT_MENU_H
#define CATA_SRC_ITEM_CONTEXT_MENU_H

/**
 * Shared ImGui right-click item context menu builder.
 * Eligibility mirrors vanilla game::inventory_item_menu / rate_action_* paths
 * using public Character / avatar_action APIs (no invented actions).
 * Consume labels follow real comesttype / USE_EAT_VERB (Drink vs Eat vs Take).
 */

#include <string>

#include "item_location.h"

class Character;

namespace item_context_menu
{

enum class action {
    none = 0,
    consume,   // eat / drink / take medication
    use,       // activate / apply (tools, relics, medical tools, books via use)
    read,
    wear,
    wield,
    takeoff,
    drop,
    pickup,    // soft-fork: take from ground / vehicle cargo into inventory
    unload,
    reload,
    examine,
    always_pickup,
    never_pickup
};

/**
 * Draw ImGui MenuItem entries for @p loc into an already-open popup.
 * @param from_worn true when the item is selected from the paper-doll (worn/wielded).
 * @param chosen_use_method if non-null and the user picked a type->use_methods entry
 *        (e.g. transform "Turn on"/"Turn off"), receives that method key for invoke.
 * @return chosen action, or action::none if the user clicked nothing.
 */
action draw_imgui_menu( Character &you, const item_location &loc, bool from_worn,
                        std::string *chosen_use_method = nullptr );

/** Compact shared inspection card. The returned action must be deferred. */
action draw_inspector( Character &you, const item_location &loc,
                       std::string *chosen_use_method = nullptr );

/**
 * Execute a chosen action. May start activities / nested UIs.
 * @param use_method optional type->use_methods key when act == action::use
 *        (empty → avatar_action::use_item default / method picker).
 * @return short status string for the caller UI (may be empty).
 */
std::string perform( Character &you, item_location loc, action act,
                     const std::string &use_method = {} );

} // namespace item_context_menu

#endif // CATA_SRC_ITEM_CONTEXT_MENU_H
