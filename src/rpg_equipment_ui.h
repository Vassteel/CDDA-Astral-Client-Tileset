#pragma once
#ifndef CATA_SRC_RPG_EQUIPMENT_UI_H
#define CATA_SRC_RPG_EQUIPMENT_UI_H

#include "equipment_layout.h"

class item;

/**
 * Soft-fork RPG UI shell: Diablo/PoE-style paper-doll equipment + grid inventory.
 * Opens from ACTION_INVENTORY (when option RPG_EQUIPMENT_UI is on) and the mouse
 * toolbar Inv button. Classic inventory remains reachable from inside this UI.
 *
 * Doll classification lives in equipment_layout; this header keeps the historical
 * storage_slot API as a compatible wrapper.
 */
namespace rpg_equipment_ui
{

/** Dedicated storage destinations, independent of clothing layer order. */
using storage_slot = equipment_layout::storage_slot;

/** Compatible wrapper around equipment_layout::storage_slot_for. */
storage_slot storage_slot_for( const item &it );

/** Run the Character Equipment window (blocking until closed). */
void open();

} // namespace rpg_equipment_ui

#endif // CATA_SRC_RPG_EQUIPMENT_UI_H
