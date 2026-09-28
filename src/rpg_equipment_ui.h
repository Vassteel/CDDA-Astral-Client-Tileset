#pragma once
#ifndef CATA_SRC_RPG_EQUIPMENT_UI_H
#define CATA_SRC_RPG_EQUIPMENT_UI_H

class item;

/**
 * Soft-fork RPG UI shell: Diablo/PoE-style paper-doll equipment + grid inventory.
 * Opens from ACTION_INVENTORY (when option RPG_EQUIPMENT_UI is on) and the mouse
 * toolbar Inv button. Classic inventory remains reachable from inside this UI.
 */
namespace rpg_equipment_ui
{

/** Dedicated storage destinations, independent of clothing layer order. */
enum class storage_slot { none, back, scabbard, sheath, holster };
storage_slot storage_slot_for( const item &it );

/** Run the Character Equipment window (blocking until closed). */
void open();

} // namespace rpg_equipment_ui

#endif // CATA_SRC_RPG_EQUIPMENT_UI_H
