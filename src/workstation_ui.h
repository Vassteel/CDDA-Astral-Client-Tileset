#pragma once
#ifndef CATA_SRC_WORKSTATION_UI_H
#define CATA_SRC_WORKSTATION_UI_H

#include <string>
#include <functional>
#include <optional>
#include "item_location.h"

#include "coords_fwd.h"

class uilist;

namespace workstation_ui
{
/** Nearby furniture with storage, crafting or an examine workflow. */
std::string name( const tripoint_bub_ms &where );
bool can_manage( const tripoint_bub_ms &where );
void open( const tripoint_bub_ms &where );
void open_nearby();
void unload( const tripoint_bub_ms &where );
void request_nearby();
bool has_request();
bool process_request();
/** Return to the same station after a native transfer/ignition activity. */
bool resume_if_ready();
void reset();
/** Render a station's existing action list within its manager when active. */
/** Inline selection when managed; nullopt keeps the original UI outside the manager.
 * limit returns zero to exclude an item, otherwise the maximum transferable count. */
std::optional<drop_locations> select_materials( const tripoint_bub_ms &where,
        const std::function<int( const item & )> &limit, int radius = 1 );
void query( uilist &menu, const tripoint_bub_ms &where );
}

#endif
