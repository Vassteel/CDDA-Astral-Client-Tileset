#pragma once
#ifndef CATA_SRC_UI_HYBRID_SIDEBAR_H
#define CATA_SRC_UI_HYBRID_SIDEBAR_H

/**
 * Soft-fork Hybrid ImGui right-info column (TILES). Replaces the classic ASCII
 * panel_manager sidebar stack with one charcoal/amber window: character HP /
 * stats, environment, and message log. Mouse-view tile info is a sibling
 * panel anchored beside the sidebar's top edge (not drawn over it).
 */

#include "point.h"

#if defined(TILES)

namespace ui_hybrid_sidebar
{

/** Bottom minimap height in terminal rows, respecting the selected layout. */
int minimap_height();

/** Create / keep the Hybrid sidebar window alive (idempotent). */
void ensure();

/** Mouse-view tile under cursor; invalidates when cleared. */
void set_mouse_tile( const tripoint &p );
void clear_mouse_tile();
bool has_mouse_tile();

/** Destroy the window (game shutdown). */
void hide();

} // namespace ui_hybrid_sidebar

#endif // TILES

#endif // CATA_SRC_UI_HYBRID_SIDEBAR_H
