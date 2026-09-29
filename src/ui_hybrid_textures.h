#pragma once
#ifndef CATA_SRC_UI_HYBRID_TEXTURES_H
#define CATA_SRC_UI_HYBRID_TEXTURES_H

#include <cstdint>
#include <string>

struct ImDrawList;
struct ImVec2;

/**
 * Decorative UI textures for the Astral theme (data/ui/astral/).
 *
 * Ownership: one cache for the process. PNGs are decoded once into SDL
 * surfaces (kept for the lifetime of the cache). GPU textures are created
 * lazily on a healthy frame and re-created after renderer recovery: the cache
 * tracks renderer_resource_generation() and release_all() is called from the
 * renderer teardown path so no texture outlives the renderer that owns it.
 *
 * Missing or corrupt assets are reported once and yield nullptr; callers fall
 * back to code-drawn geometry (see ui_hybrid_widgets). No disk access happens
 * per frame after the first lookup.
 *
 * Draw helpers split large quads into bounded sections (<= 256 px) because the
 * SDL software renderer overflows its textured-triangle math on big quads.
 */
namespace ui_hybrid_textures
{

struct texture {
    uint64_t id = 0;   // ImTextureID-compatible handle, 0 when unavailable this frame
    int width = 0;
    int height = 0;
};

/** Well-known asset ids (relative paths under data/ui/astral/). */
namespace asset
{
constexpr const char *window_large = "frames/window_large.png";
constexpr const char *window_large_reduced = "frames/window_large_reduced.png";
constexpr const char *dialog_compact = "frames/dialog_compact.png";
constexpr const char *dialog_compact_reduced = "frames/dialog_compact_reduced.png";
constexpr const char *popup_light = "frames/popup_light.png";
constexpr const char *panel_inset = "frames/panel_inset.png";
constexpr const char *card = "frames/card.png";
constexpr const char *portrait = "frames/portrait.png";
constexpr const char *charcoal_grain = "materials/charcoal_grain.png";
constexpr const char *bronze_grain = "materials/bronze_grain.png";
constexpr const char *button_primary = "buttons/primary.png";
constexpr const char *button_secondary = "buttons/secondary.png";
constexpr const char *meter_track = "meters/track.png";
constexpr const char *meter_fill = "meters/fill.png";
constexpr const char *icon_atlas = "icons/atlas.png";
} // namespace asset

/**
 * Fetch (loading on first use) a texture usable in the current ImGui frame.
 * Returns a texture with id == 0 when the asset is unavailable or the renderer
 * is not healthy; callers must handle that.
 */
texture get( const std::string &relative_path );

/** True when the file exists and decoded (does not require a renderer). */
bool available( const std::string &relative_path );

/** Drop GPU textures (keeps decoded surfaces). Safe to call at any time. */
void release_all();
/** Drop everything including decoded surfaces (process shutdown). */
void shutdown();

/** Approximate decoded bytes held by the cache (surfaces + textures). */
size_t decoded_bytes();
/** Number of GPU textures currently alive. */
int live_texture_count();
/** Number of textures created since process start (recovery instrumentation). */
int texture_creations();

/** Pure nine-slice geometry (also used by tests): screen edges and UVs. */
struct nine_slice_geometry {
    float xs[4];
    float ys[4];
    float us[4];
    float vs[4];
    float margin_px; // effective margin after clamping so corners never overlap
};
nine_slice_geometry compute_nine_slice( float min_x, float min_y, float max_x, float max_y,
                                        float margin_src, float scale, int tex_w, int tex_h );

/**
 * Draw a nine-slice frame. margin_src is the slice inset in source pixels;
 * scale converts source px to screen px (0.5 for 2x-authored assets at UI
 * scale 1; multiply by theme::scale()). Returns false when the texture is
 * unavailable (nothing drawn).
 */
bool draw_nine_slice( ImDrawList *draw, const texture &tex, const ImVec2 &min, const ImVec2 &max,
                      float margin_src, float scale, uint32_t tint = 0xFFFFFFFFu );

/** Tile a seamless texture across the rect with the given alpha (0-1). */
bool draw_tiled( ImDrawList *draw, const texture &tex, const ImVec2 &min, const ImVec2 &max,
                 float tile_scale, float alpha );

/** Draw an icon from the atlas by name, tinted. Returns false if missing. */
bool draw_icon( ImDrawList *draw, const std::string &name, const ImVec2 &min, float size,
                uint32_t tint );
/** True if the atlas defines the icon. */
bool has_icon( const std::string &name );

} // namespace ui_hybrid_textures

#endif // CATA_SRC_UI_HYBRID_TEXTURES_H
