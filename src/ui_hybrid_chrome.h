#pragma once
#ifndef CATA_SRC_UI_HYBRID_CHROME_H
#define CATA_SRC_UI_HYBRID_CHROME_H

/**
 * Soft-fork "D Hybrid" ImGui chrome (BG3-mock reference): warm dark wood /
 * bronze panels + muted amber accents, charcoal grit. Phase-1 = colors /
 * frames / slot bezels / dense grid chrome only — no item-icon atlases,
 * centered silhouette art, or radial slot layout (later phases).
 *
 * Equipped gear lives ONLY on the paper-doll / slot ring — do not add a
 * duplicate equipped-item list beside the doll.
 *
 * Scoped push/pop so Character Equipment, mouse toolbar, and later chargen
 * EQUIPMENT can share the look without rewriting each panel.
 */

struct ImVec4;

namespace ui_hybrid_chrome
{

/** Palette constants (float RGBA 0–1). */
namespace palette
{
ImVec4 window_bg();
ImVec4 child_bg();
ImVec4 popup_bg();
ImVec4 border();
ImVec4 border_accent();
ImVec4 button();
ImVec4 button_hovered();
ImVec4 button_active();
ImVec4 header();
ImVec4 header_hovered();
ImVec4 text();
ImVec4 text_muted();
ImVec4 accent();
ImVec4 accent_dim();
ImVec4 separator();
ImVec4 title_bg();
ImVec4 scrollbar_grab();
ImVec4 slot_empty();
ImVec4 slot_selected();
ImVec4 grid_selected();
ImVec4 toolbar_active();
ImVec4 toolbar_active_hovered();
ImVec4 toolbar_active_pressed();
} // namespace palette

/**
 * Push Hybrid window / child / button / border / header colors + light
 * rounding / border size. Pair with pop().
 */
void push();
void pop();

/**
 * Draw a thin chrome bezel around the last ImGui item.
 * selected → amber accent stroke; empty → dimmer charcoal edge; else muted.
 */
void draw_item_bezel( bool selected, bool hovered, bool empty = false );

/** Push button colors for a paper-doll slot. Returns PushStyleColor count. */
int push_slot_button( bool selected, bool empty );

/** Push button colors for an inventory grid cell. Returns PushStyleColor count. */
int push_grid_button( bool selected );

/**
 * Push button colors for a toolbar (or action-bar) button.
 * active=true uses amber-tinted "on" state. Returns PushStyleColor count.
 */
int push_toolbar_button( bool active );

} // namespace ui_hybrid_chrome

#endif // CATA_SRC_UI_HYBRID_CHROME_H
