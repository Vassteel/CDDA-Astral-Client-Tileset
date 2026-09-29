#pragma once
#ifndef CATA_SRC_UI_HYBRID_SHOWCASE_H
#define CATA_SRC_UI_HYBRID_SHOWCASE_H

/**
 * Development-only component showcase for the Astral UI foundation: every
 * primitive in every state, long labels, disabled controls, nested popups and a
 * narrow-width toggle, rendered with the real renderer and fonts.
 * Reachable from Main menu → Settings → Astral UI showcase, or automatically at
 * start when the environment variable CDDA_UI_SHOWCASE is set (capture runs).
 */
namespace ui_hybrid_showcase
{
void show();
} // namespace ui_hybrid_showcase

#endif // CATA_SRC_UI_HYBRID_SHOWCASE_H
