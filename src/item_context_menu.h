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
    unload,
    reload,
    examine
};

/**
 * Draw ImGui MenuItem entries for @p loc into an already-open popup.
 * @param from_worn true when the item is selected from the paper-doll (worn/wielded).
 * @return chosen action, or action::none if the user clicked nothing.
 */
action draw_imgui_menu( Character &you, const item_location &loc, bool from_worn );

/**
 * Execute a chosen action. May start activities / nested UIs.
 * @return short status string for the caller UI (may be empty).
 */
std::string perform( Character &you, item_location loc, action act );

} // namespace item_context_menu

#endif // CATA_SRC_ITEM_CONTEXT_MENU_H
