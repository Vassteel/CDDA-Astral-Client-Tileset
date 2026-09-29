#pragma once
#ifndef CATA_SRC_EQUIPMENT_ACTIONS_H
#define CATA_SRC_EQUIPMENT_ACTIONS_H

#include "bodypart.h"
#include "ret_val.h"

class Character;
class item_location;

namespace equipment_actions
{
/** Stow the wielded item without a disposal menu or a ground-drop fallback. */
ret_val<void> unwield_to_inventory( Character &who );
/** Validate an explicit side before moving the source, then use native wear/side operations. */
ret_val<void> wear_on_side( Character &who, item_location source, side requested,
                            bool interactive = true );
}

#endif
