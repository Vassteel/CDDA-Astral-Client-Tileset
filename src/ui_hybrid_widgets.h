#pragma once
#ifndef CATA_SRC_UI_HYBRID_WIDGETS_H
#define CATA_SRC_UI_HYBRID_WIDGETS_H

#include <cstdint>
#include <functional>
#include <string>

#include "imgui/imgui.h"

class input_context;

/**
 * Reusable Astral UI primitives. Every primitive uses native ImGui widgets
 * (InvisibleButton / Button / BeginChild ...) for input, focus and navigation
 * semantics and only replaces the visual drawing, so keyboard, mouse, drag/drop
 * and controller-emulated navigation keep working.
 *
 * All sizes are logical px scaled by ui_hybrid_chrome::theme::scale().
 * Decoration follows ui_hybrid_chrome::theme::level(): textures when available
 * and level == full/reduced, code-drawn geometry otherwise. Content never
 * depends on decoration.
 *
 * Nested native menus / activities must not run from inside these callbacks;
 * queue the action and run it after the ImGui frame ends (window::execute loops).
 */
namespace ui_hybrid_widgets
{

enum class frame_kind { large, dialog, popup, none };

/**
 * Draw the window shell (background, frame, title bar, close control) for the
 * current ImGui window. The window must have been begun with
 * ImGuiWindowFlags_NoTitleBar | NoBackground (cataimgui::window::set_shell does
 * this). Leaves the cursor at the content origin below the title bar.
 * Returns true when the close control was activated this frame.
 */
bool window_shell( const std::string &title, frame_kind kind, bool show_close = true,
                   const char *icon = nullptr, bool show_title = true );

/** Content region that scrolls, leaving footer_height for a fixed footer. */
bool body_begin( const char *id, float footer_height_logical, ImGuiWindowFlags flags = 0 );
void body_end();
/** Fixed footer strip (buttons, hints, status). Call after body_end(). */
bool footer_begin( const char *id, float height_logical = 0.f );
void footer_end();
/** Right-align the following items inside the footer: pass total width of them. */
void footer_align_right( float items_width );

/** Amber section label with quiet rule. */
void section_label( const std::string &text, const char *icon = nullptr );

enum class button_kind { primary, secondary, tertiary, danger };
/**
 * Themed button. size.x <= 0 → auto (min width token); size.y <= 0 → button token.
 * Disabled buttons stay legible; pass a reason for the tooltip.
 */
/** Width action_button() will use for `label` when no explicit width is given. */
float action_button_width( const char *label, button_kind kind = button_kind::secondary,
                           const char *icon = nullptr );
bool action_button( const char *label, button_kind kind = button_kind::secondary,
                    const ImVec2 &size_logical = ImVec2( 0.f, 0.f ), bool enabled = true,
                    const char *disabled_reason = nullptr, const char *icon = nullptr );
/** Square icon-only control (search, close, grid/list toggle...). */
bool icon_button( const char *id, const char *icon, float size_logical, bool active = false,
                  const char *tooltip = nullptr, bool enabled = true );
/** Close "×" control sized close_hit. */
bool close_button( const char *id );
/**
 * HUD toolbar button: [icon] label [key]. `active` marks an on/off state (amber
 * edge + filled dot in the label is the caller's choice). Returns the ImGui
 * button result; the caller reads IsItemClicked for right-click.
 */
bool toolbar_button( const char *id, const std::string &label, const char *icon,
                     const std::string &key_hint, bool active, float height_logical = 36.f );
/** Width the toolbar button will occupy (for wrapping decisions). */
float toolbar_button_width( const std::string &label, const char *icon, const std::string &key_hint );

struct row_state {
    bool selected = false;
    bool focused = false;   // keyboard focus, distinct from selection
    bool disabled = false;
    bool featured = false;  // section-size label, taller row, quiet edge
};
struct row_result {
    bool clicked = false;
    bool double_clicked = false;
    bool right_clicked = false; // press+release on the same row
    bool hovered = false;
    bool truncated = false;     // label was ellipsized; caller may show a tooltip
    unsigned int id = 0;        // ImGuiID of the row's hit item (drop targets, custom logic)
    ImVec2 min;
    ImVec2 max;
};
/**
 * Draw only the row background/edges for a rect (for callers that own their
 * own hit item, e.g. drag-source buttons in the inventory pane).
 */
void draw_row_background( ImDrawList *draw, const ImVec2 &min, const ImVec2 &max,
                          const row_state &state, bool hovered );
/** Callback drawing custom content (item sprite) into the icon box. */
using icon_painter = std::function<void( ImDrawList *, const ImVec2 &min, const ImVec2 &max )>;

/**
 * List row: [icon] label ........ detail [chevron]. Handles ellipsis, states,
 * hover, and keeps the whole row as one hit target (drag/drop targets attach
 * to the last item as usual). height_logical <= 0 → row / row_featured token.
 */
row_result selectable_row( const char *id, const std::string &label, const std::string &detail,
                           const char *icon, const icon_painter &painter, const row_state &state,
                           float height_logical = 0.f, const char *chevron = nullptr );
/**
 * Tree row with indent, connector lines and expansion chevron. depth 0 rows are
 * featured; deeper rows are plain. `branch` rows show a chevron (open → down).
 */
row_result tree_row( const char *id, const std::string &label, const std::string &detail,
                     const char *icon, const icon_painter &painter, int depth, bool branch,
                     bool open, const row_state &state, float height_logical = 0.f );

enum class meter_kind { neutral, health, stamina, morale, danger, success, info, warning };
/** Meter with track/fill textures and always-visible text. width <= 0 → fill available. */
void meter( const char *id, float fraction, meter_kind kind, const std::string &text,
            float width_logical = -1.f, float height_logical = 0.f );

/** Icon from the atlas as an inline item (advances the cursor). */
void icon( const char *name, float size_logical, uint32_t tint = 0 );
/** Draw an icon at an absolute position without an item. Returns false if missing. */
bool draw_icon_at( ImDrawList *draw, const char *name, const ImVec2 &min, float size_px,
                   uint32_t tint );
/** Word-wrapped themed tooltip for the last item when hovered. */
void tooltip( const std::string &text );
/** Binding-aware hint: [key] description, key from the context (never hard-coded). */
void hint( const input_context &ctxt, const std::string &action, const std::string &description,
           bool active = false );
/** Hint with an explicit key label (only for keys ImGui owns, e.g. Escape in popups). */
void hint_key( const std::string &key, const std::string &description );

/** Inset panel (deep surface, quiet edge). Wraps BeginChild/EndChild. */
bool panel_begin( const char *id, const ImVec2 &size = ImVec2( 0.f, 0.f ), bool inset = true,
                  ImGuiWindowFlags flags = 0 );
void panel_end();
/** Raised card region (hand cards, detail cards). */
bool card_begin( const char *id, const ImVec2 &size = ImVec2( 0.f, 0.f ), bool selected = false,
                 ImGuiWindowFlags flags = 0 );
void card_end();

/** Text tab strip. Returns true when the tab was clicked. */
bool tab( const char *label, bool selected, const char *icon = nullptr );

/** Empty / no-results state, centred. */
void empty_state( const std::string &title, const std::string &detail = std::string() );

/** Draw a dim scrim over the whole viewport (behind a modal dialog). */
void scrim();

/** Draw a decorative frame directly (for non-window regions such as a portrait). */
void draw_frame_rect( ImDrawList *draw, const ImVec2 &min, const ImVec2 &max, frame_kind kind );
/** Portrait backdrop frame; content drawn by the caller inside the returned inset rect. */
void portrait_frame( ImDrawList *draw, const ImVec2 &min, const ImVec2 &max, ImVec2 &inner_min,
                     ImVec2 &inner_max );

/** Push the section (1.25x) or title (1.5x) gui font. */
void push_font_section();
void push_font_title();
void pop_font();

/** Ellipsize to fit width_px with the current font. */
std::string fit_text( const std::string &text, float width_px );

/**
 * Widget probe for native interaction checks (dev/test only).
 *
 * When the environment variable CDDA_UI_PROBE names a file, every shared primitive
 * records its label and screen rectangle each frame and the list is written to that
 * file (JSON) once per frame. The capture harness reads it to click widgets by name
 * instead of hard-coded coordinates. Disabled (no cost beyond a bool test) otherwise.
 */
namespace probe
{
bool enabled();
void record( const char *kind, const std::string &label, const ImVec2 &min, const ImVec2 &max );
/** Called by the ImGui client once per frame, before NewFrame. */
void flush_frame();
} // namespace probe

} // namespace ui_hybrid_widgets

#endif // CATA_SRC_UI_HYBRID_WIDGETS_H
