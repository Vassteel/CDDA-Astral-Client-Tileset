#pragma once
#ifndef CATA_SRC_UI_HYBRID_CHROME_H
#define CATA_SRC_UI_HYBRID_CHROME_H

#include <cstdint>
#include <string>

/**
 * Astral client shared UI theme ("Hybrid chrome").
 *
 * One place for the palette, spacing ladder, type roles, control sizes and
 * decoration level used by every ImGui screen (equipment tree, HUD, crafting,
 * dialogs, uilist/popup engines). Screens draw with the primitives declared in
 * ui_hybrid_widgets.h instead of pushing their own colors or drawing their own
 * frames. Texture-backed decoration (nine-slice frames, icon atlas) lives in
 * ui_hybrid_textures.h and degrades to code-drawn geometry when assets are
 * missing or decoration is set to "none".
 *
 * Token values mirror data/ui/astral/theme.json (see doc/astral/ui-art-theme.md);
 * keep both in sync when changing a value.
 *
 * Theme precedence (cataimgui::init_colors): ImGui dark → Astral tokens →
 * the user's config/imgui_style.json colors on top. Per-window scoped_style
 * pushes only structural style vars, never colors, so the style picker wins.
 */

struct ImVec4;
struct ImVec2;

namespace ui_hybrid_chrome
{

/** Decoration strength, from the ASTRAL_UI_DECORATION option. */
enum class decoration : int {
    full = 0,    // textured surface, bevelled bronze frame, corner detail
    reduced = 1, // flat surfaces, single bronze line
    none = 2,    // flat surfaces, quiet edge only
};

namespace theme
{
/** Colors as 0xAABBGGRR (ImU32 layout) at 1.0 alpha unless noted. */
struct tokens {
    uint32_t deep_bg, surface, raised, raised_hover, edge_dark, edge_quiet, edge_bronze,
             bronze_light, bronze_dark, accent, accent_dim, selected_bg, focus_bg, text,
             text_muted, text_on_accent, danger, success, info, warning, meter_track, scrim;
    // spacing ladder (logical px at scale 1)
    float xs, sm, md, lg, xl;
    float radius_window, radius_panel, radius_control;
    float border_frame, border_quiet, border_focus;
    float row, row_featured, row_compact, button, button_min_w, footer, title_bar, close_hit,
          icon, icon_featured, tree_indent, scrollbar;
};
const tokens &get();
/** Current UI scale: GUI font size / 16, never below 1. */
float scale();
/** token * scale() */
float px( float logical );
/**
 * Size of a large (full-screen class) hybrid window: 1280×800 logical at the
 * UI scale, but never smaller than most of a large viewport, so a 4K display
 * with a moderate font still gets a window that uses the screen.
 */
ImVec2 large_window_size();
decoration level();
/** Re-read the decoration option (cheap; called once per frame by the client). */
void refresh_options();
/** Force a level (tests / showcase); pass -1 to return to the option. */
void override_level( int level );
} // namespace theme

/** Palette accessors (ImVec4) kept for existing callers; all derive from theme::get(). */
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
ImVec4 danger();
ImVec4 success();
ImVec4 info();
ImVec4 warning();
ImVec4 from_u32( uint32_t c );
} // namespace palette

/**
 * Push structural style vars (rounding, borders, padding) for a themed window.
 * Colors are NOT pushed: they come from the base style set by apply_defaults()
 * plus the user's style file. Pair with pop().
 */
void push();
void pop();
/**
 * Write the Astral tokens into ImGui::GetStyle() (colors + structure). Called by
 * cataimgui::init_colors() before the user's style file is overlaid.
 */
void apply_defaults();
/** Names of ImGuiCol_ slots the theme sets, for the style picker/json overlay. */
bool theme_sets_color( int imgui_col );

class scoped_style
{
    public:
        scoped_style() { push(); }
        ~scoped_style() { pop(); }
        scoped_style( const scoped_style & ) = delete;
        scoped_style &operator=( const scoped_style & ) = delete;
};

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

/** Amber section label + separator. Prefer ui_hybrid_widgets::section_label. */
void section_header( const char *title );

/** Themed horizontal meter. Prefer ui_hybrid_widgets::meter. */
void progress_meter( float fraction, const char *overlay_text = nullptr );

} // namespace ui_hybrid_chrome

#endif // CATA_SRC_UI_HYBRID_CHROME_H
