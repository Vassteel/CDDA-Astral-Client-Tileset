#pragma once
#ifndef CATA_SRC_CONSUME_HYBRID_UI_H
#define CATA_SRC_CONSUME_HYBRID_UI_H

/**
 * Soft-fork Hybrid (ImGui) consume menu — charcoal + muted amber chrome.
 * Replaces the classic inventory_selector consume UI on TILES builds while
 * keeping the same columns / needs header / category filters.
 */

#include <string>

#include "item_location.h"

class Character;
class inventory_selector_preset;

#if defined(TILES)

namespace consume_hybrid
{

/**
 * Modal Hybrid consume picker.
 * @param preset  comestible inventory preset (columns + is_shown + denial)
 * @param title   window title
 * @param none_message  popup when no items match
 * @param hint    needs / kcal / vitamin header text (may contain color tags)
 * @param container  if set, only look inside this container
 * @param initial_filter  optional text filter (uistate)
 * @param comest_tab  "FOOD" / "DRINK" / "MED" / "" (All)
 * @return selected item, or nowhere if cancelled / empty
 */
item_location select( Character &you, const inventory_selector_preset &preset,
                      const std::string &title, const std::string &none_message,
                      const std::string &hint, const item_location &container,
                      const std::string &initial_filter,
                      const std::string &comest_tab );

} // namespace consume_hybrid

#endif // TILES

#endif // CATA_SRC_CONSUME_HYBRID_UI_H
