#include "ui_hybrid_textures.h"

#include <algorithm>

namespace ui_hybrid_textures
{

nine_slice_geometry compute_nine_slice( float min_x, float min_y, float max_x, float max_y,
                                        float margin_src, float scale, int tex_w, int tex_h )
{
    nine_slice_geometry g{};
    const float w = max_x - min_x;
    const float h = max_y - min_y;
    // Never stretch corners: shrink the margin if the rect is too small.
    float m = margin_src * scale;
    m = std::max( 0.f, std::min( m, std::min( w, h ) * 0.5f ) );
    g.margin_px = m;
    const float um = tex_w > 0 ? margin_src / tex_w : 0.f;
    const float vm = tex_h > 0 ? margin_src / tex_h : 0.f;
    const float xs[4] = { min_x, min_x + m, max_x - m, max_x };
    const float ys[4] = { min_y, min_y + m, max_y - m, max_y };
    const float us[4] = { 0.f, um, 1.f - um, 1.f };
    const float vs[4] = { 0.f, vm, 1.f - vm, 1.f };
    for( int i = 0; i < 4; ++i ) {
        g.xs[i] = xs[i];
        g.ys[i] = ys[i];
        g.us[i] = us[i];
        g.vs[i] = vs[i];
    }
    return g;
}

} // namespace ui_hybrid_textures

#if defined(TILES)

#include <algorithm>
#include <map>
#include <memory>
#include <string>

#include "cata_path.h"
#include "debug.h"
#include "filesystem.h"
#include "flexbuffer_json.h"
#include "imgui/imgui.h"
#include "json_loader.h"
#include "path_info.h"
#include "sdl_wrappers.h"
#include "sdltiles.h"

namespace ui_hybrid_textures
{

namespace
{

struct entry {
    SDL_Surface_Ptr surface;   // decoded once; nullptr when the file is missing/corrupt
    SDL_Texture_Ptr gpu;       // renderer-owned; dropped on release_all()
    uint64_t generation = 0;   // renderer_resource_generation() when gpu was created
    bool load_failed = false;
    bool reported = false;
    int width = 0;
    int height = 0;
};

std::map<std::string, entry> g_cache;
bool g_shutdown = false;
int g_live = 0;
int g_creations = 0;

struct atlas_icon {
    int x;
    int y;
};
std::map<std::string, atlas_icon> g_atlas;
int g_atlas_cell = 64;
bool g_atlas_loaded = false;
bool g_atlas_failed = false;

cata_path asset_path( const std::string &rel )
{
    return PATH_INFO::datadir_path() / "ui" / "astral" / rel;
}

void load_atlas_index()
{
    if( g_atlas_loaded ) {
        return;
    }
    g_atlas_loaded = true;
    const cata_path p = asset_path( "icons/atlas.json" );
    try {
        std::optional<JsonValue> jv = json_loader::from_path_opt( p );
        if( !jv ) {
            g_atlas_failed = true;
            DebugLog( D_WARNING, D_MAIN ) << "Astral UI icon atlas index missing: " << p.generic_u8string();
            return;
        }
        JsonObject root = jv->get_object();
        root.allow_omitted_members();
        g_atlas_cell = root.get_int( "cell", 64 );
        JsonObject icons = root.get_object( "icons" );
        icons.allow_omitted_members();
        for( const JsonMember &m : icons ) {
            JsonObject o = m.get_object();
            o.allow_omitted_members();
            g_atlas[m.name()] = atlas_icon{ o.get_int( "x" ), o.get_int( "y" ) };
        }
    } catch( const JsonError &err ) {
        g_atlas_failed = true;
        DebugLog( D_ERROR, D_MAIN ) << "Astral UI icon atlas index invalid: " << err.what();
    }
}

entry &lookup( const std::string &rel )
{
    auto it = g_cache.find( rel );
    if( it != g_cache.end() ) {
        return it->second;
    }
    entry &e = g_cache[rel];
    const cata_path p = asset_path( rel );
    if( !file_exist( p.get_unrelative_path() ) ) {
        e.load_failed = true;
    } else {
        try {
            e.surface = load_image( p.generic_u8string().c_str() );
        } catch( const std::exception &err ) {
            e.load_failed = true;
            DebugLog( D_ERROR, D_MAIN ) << "Astral UI asset failed to decode: " << rel << ": " << err.what();
            e.reported = true;
        }
    }
    if( e.load_failed && !e.reported ) {
        e.reported = true;
        DebugLog( D_WARNING, D_MAIN ) << "Astral UI asset missing, using code-drawn fallback: " << rel;
    }
    if( e.surface ) {
        e.width = e.surface->w;
        e.height = e.surface->h;
    }
    return e;
}

bool ensure_gpu( entry &e )
{
    if( !e.surface ) {
        return false;
    }
    const uint64_t gen = renderer_resource_generation();
    if( e.gpu && e.generation == gen ) {
        return true;
    }
    if( renderer_should_abort_frame() ) {
        return false;
    }
    const SDL_Renderer_Ptr &renderer = get_sdl_renderer();
    if( !renderer ) {
        return false;
    }
    if( e.gpu ) {
        --g_live;
        e.gpu.reset();
    }
    e.gpu = CreateTextureFromSurface( renderer, e.surface );
    if( !e.gpu ) {
        return false;
    }
    SDL_SetTextureScaleMode( e.gpu.get(), SDL_SCALEMODE_LINEAR );
    SetTextureBlendMode( e.gpu, SDL_BLENDMODE_BLEND );
    e.generation = gen;
    ++g_live;
    ++g_creations;
    return true;
}

// Split a textured quad into <= section px pieces (SDL software renderer safety).
void add_image_bounded( ImDrawList *draw, ImTextureID id, const ImVec2 &p0, const ImVec2 &p1,
                        const ImVec2 &uv0, const ImVec2 &uv1, uint32_t tint )
{
    constexpr float section = 256.f;
    const float w = p1.x - p0.x;
    const float h = p1.y - p0.y;
    if( w <= 0.f || h <= 0.f ) {
        return;
    }
    if( w <= section && h <= section ) {
        draw->AddImage( id, p0, p1, uv0, uv1, tint );
        return;
    }
    for( float y = 0.f; y < h; y += section ) {
        const float y1 = std::min( y + section, h );
        for( float x = 0.f; x < w; x += section ) {
            const float x1 = std::min( x + section, w );
            const ImVec2 a( p0.x + x, p0.y + y );
            const ImVec2 b( p0.x + x1, p0.y + y1 );
            const ImVec2 ua( uv0.x + ( uv1.x - uv0.x ) * ( x / w ), uv0.y + ( uv1.y - uv0.y ) * ( y / h ) );
            const ImVec2 ub( uv0.x + ( uv1.x - uv0.x ) * ( x1 / w ), uv0.y + ( uv1.y - uv0.y ) * ( y1 / h ) );
            draw->AddImage( id, a, b, ua, ub, tint );
        }
    }
}

} // namespace

texture get( const std::string &rel )
{
    texture t;
    if( g_shutdown ) {
        return t;
    }
    entry &e = lookup( rel );
    t.width = e.width;
    t.height = e.height;
    if( ensure_gpu( e ) ) {
        t.id = reinterpret_cast<uint64_t>( e.gpu.get() );
    }
    return t;
}

bool available( const std::string &rel )
{
    if( g_shutdown ) {
        return false;
    }
    return lookup( rel ).surface != nullptr;
}

void release_all()
{
    for( auto &kv : g_cache ) {
        if( kv.second.gpu ) {
            kv.second.gpu.reset();
            --g_live;
        }
    }
}

void shutdown()
{
    release_all();
    g_cache.clear();
    g_atlas.clear();
    g_atlas_loaded = false;
    g_shutdown = true;
}

size_t decoded_bytes()
{
    size_t total = 0;
    for( const auto &kv : g_cache ) {
        const entry &e = kv.second;
        if( e.surface ) {
            total += static_cast<size_t>( e.width ) * e.height * 4;
        }
        if( e.gpu ) {
            total += static_cast<size_t>( e.width ) * e.height * 4;
        }
    }
    return total;
}

int live_texture_count()
{
    return g_live;
}

int texture_creations()
{
    return g_creations;
}

bool draw_nine_slice( ImDrawList *draw, const texture &tex, const ImVec2 &min, const ImVec2 &max,
                      float margin_src, float scale, uint32_t tint )
{
    if( tex.id == 0 || draw == nullptr || tex.width <= 0 ) {
        return false;
    }
    const float w = max.x - min.x;
    const float h = max.y - min.y;
    if( w <= 1.f || h <= 1.f ) {
        return false;
    }
    const nine_slice_geometry g = compute_nine_slice( min.x, min.y, max.x, max.y, margin_src, scale,
                                  tex.width, tex.height );
    const ImTextureID id = static_cast<ImTextureID>( tex.id );
    for( int row = 0; row < 3; ++row ) {
        for( int col = 0; col < 3; ++col ) {
            const ImVec2 p0( g.xs[col], g.ys[row] );
            const ImVec2 p1( g.xs[col + 1], g.ys[row + 1] );
            if( p1.x <= p0.x || p1.y <= p0.y ) {
                continue;
            }
            const ImVec2 uv0( g.us[col], g.vs[row] );
            const ImVec2 uv1( g.us[col + 1], g.vs[row + 1] );
            if( row == 1 || col == 1 ) {
                add_image_bounded( draw, id, p0, p1, uv0, uv1, tint );
            } else {
                draw->AddImage( id, p0, p1, uv0, uv1, tint );
            }
        }
    }
    return true;
}

bool draw_tiled( ImDrawList *draw, const texture &tex, const ImVec2 &min, const ImVec2 &max,
                 float tile_scale, float alpha )
{
    if( tex.id == 0 || draw == nullptr || tex.width <= 0 || alpha <= 0.f ) {
        return false;
    }
    const float tw = tex.width * tile_scale;
    const float th = tex.height * tile_scale;
    if( tw < 4.f || th < 4.f ) {
        return false;
    }
    const uint32_t tint = IM_COL32( 255, 255, 255, static_cast<int>( std::clamp( alpha, 0.f, 1.f ) * 255 ) );
    const ImTextureID id = static_cast<ImTextureID>( tex.id );
    draw->PushClipRect( min, max, true );
    for( float y = min.y; y < max.y; y += th ) {
        for( float x = min.x; x < max.x; x += tw ) {
            draw->AddImage( id, ImVec2( x, y ), ImVec2( x + tw, y + th ), ImVec2( 0, 0 ), ImVec2( 1, 1 ),
                            tint );
        }
    }
    draw->PopClipRect();
    return true;
}

bool has_icon( const std::string &name )
{
    load_atlas_index();
    return g_atlas.count( name ) > 0;
}

bool draw_icon( ImDrawList *draw, const std::string &name, const ImVec2 &min, float size,
                uint32_t tint )
{
    load_atlas_index();
    const auto it = g_atlas.find( name );
    if( it == g_atlas.end() || draw == nullptr ) {
        return false;
    }
    const texture atlas = get( asset::icon_atlas );
    if( atlas.id == 0 ) {
        return false;
    }
    const float u0 = static_cast<float>( it->second.x ) / atlas.width;
    const float v0 = static_cast<float>( it->second.y ) / atlas.height;
    const float u1 = static_cast<float>( it->second.x + g_atlas_cell ) / atlas.width;
    const float v1 = static_cast<float>( it->second.y + g_atlas_cell ) / atlas.height;
    draw->AddImage( static_cast<ImTextureID>( atlas.id ), min, ImVec2( min.x + size, min.y + size ),
                    ImVec2( u0, v0 ), ImVec2( u1, v1 ), tint );
    return true;
}

} // namespace ui_hybrid_textures

#else // !TILES

namespace ui_hybrid_textures
{
texture get( const std::string & )
{
    return {};
}
bool available( const std::string & )
{
    return false;
}
void release_all() {}
void shutdown() {}
size_t decoded_bytes()
{
    return 0;
}
int live_texture_count()
{
    return 0;
}
int texture_creations()
{
    return 0;
}
bool draw_nine_slice( ImDrawList *, const texture &, const ImVec2 &, const ImVec2 &, float, float,
                      uint32_t )
{
    return false;
}
bool draw_tiled( ImDrawList *, const texture &, const ImVec2 &, const ImVec2 &, float, float )
{
    return false;
}
bool draw_icon( ImDrawList *, const std::string &, const ImVec2 &, float, uint32_t )
{
    return false;
}
bool has_icon( const std::string & )
{
    return false;
}
} // namespace ui_hybrid_textures

#endif // TILES
