#pragma once
#ifndef CATA_SRC_RPG_EQUIPMENT_UI_H
#define CATA_SRC_RPG_EQUIPMENT_UI_H

/**
 * Soft-fork RPG UI shell: Diablo/PoE-style paper-doll equipment + grid inventory.
 * Opens from ACTION_INVENTORY (when option RPG_EQUIPMENT_UI is on) and the mouse
 * toolbar Inv button. Classic inventory remains reachable from inside this UI.
 */
namespace rpg_equipment_ui
{

/** Run the Character Equipment window (blocking until closed). */
void open();

} // namespace rpg_equipment_ui

#endif // CATA_SRC_RPG_EQUIPMENT_UI_H
