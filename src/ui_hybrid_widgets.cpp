#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#undef IMGUI_DEFINE_MATH_OPERATORS

#include "ui_hybrid_widgets.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>

#include "cata_imgui.h"
#include "input_context.h"
#include "translations.h"
#include "ui_hybrid_chrome.h"
#include "ui_hybrid_textures.h"

namespace ui_hybrid_widgets
{

using ui_hybrid_chrome::decoration;
namespace theme = ui_hybrid_chrome::theme;
namespace tex = ui_hybrid_textures;

namespace
{

const theme::tokens &T()
{
    return theme::get();
}

float S()
{
    return theme::scale();
}

// Apply the current style alpha (BeginDisabled) to a token color.
uint32_t C( uint32_t c )
{
    return ImGui::GetColorU32( c );
}

uint32_t alpha( uint32_t c, float a )
{
    return ( c & 0x00FFFFFFu ) | ( static_cast<uint32_t>( std::clamp( a, 0.f, 1.f ) * 255.f ) << 24 );
}

// Solid one-pixel-strip rectangle outline: crisp on the software renderer and
// with bitmap font atlases, unlike thin antialiased AddRect strokes.
void solid_outline( ImDrawList *draw, const ImVec2 &min, const ImVec2 &max, uint32_t col,
                    float thickness )
{
    draw->AddRectFilled( min, ImVec2( max.x, min.y + thickness ), col );
    draw->AddRectFilled( ImVec2( min.x, max.y - thickness ), max, col );
    draw->AddRectFilled( min, ImVec2( min.x + thickness, max.y ), col );
    draw->AddRectFilled( ImVec2( max.x - thickness, min.y ), max, col );
}

struct frame_asset {
    const char *full;
    const char *reduced;
    float margin_src;   // slice margin in source px
    float content_inset; // logical px
};

frame_asset asset_for( frame_kind kind )
{
    switch( kind ) {
        case frame_kind::large:
            return { tex::asset::window_large, tex::asset::window_large_reduced, 48.f, 16.f };
        case frame_kind::dialog:
            return { tex::asset::dialog_compact, tex::asset::dialog_compact_reduced, 32.f, 12.f };
        case frame_kind::popup:
            return { tex::asset::popup_light, tex::asset::popup_light, 24.f, 8.f };
        case frame_kind::none:
            break;
    }
    return { nullptr, nullptr, 0.f, 8.f };
}

// Code-drawn fallback / "none" decoration frame.
void draw_code_frame( ImDrawList *draw, const ImVec2 &min, const ImVec2 &max, frame_kind kind,
                      bool bronze )
{
    const float r = T().radius_window * S();
    draw->AddRectFilled( min, max, T().surface, r );
    if( kind == frame_kind::popup || !bronze ) {
        draw->AddRect( min, max, T().edge_quiet, r, 0, std::max( 1.f, T().border_quiet * S() ) );
        return;
    }
    // outer dark line, bronze edge, inner dark line
    draw->AddRect( min - ImVec2( 1, 1 ), max + ImVec2( 1, 1 ), alpha( T().edge_dark, 0.8f ), r + 1.f );
    solid_outline( draw, min, max, T().edge_bronze, std::max( 2.f, T().border_frame * S() ) );
    const float in = std::max( 2.f, T().border_frame * S() );
    solid_outline( draw, min + ImVec2( in, in ), max - ImVec2( in, in ), alpha( T().edge_dark, 0.6f ), 1.f );
}

// Draw the decorative frame for a rect. Returns the content inset (logical).
float frame_rect( ImDrawList *draw, const ImVec2 &min, const ImVec2 &max, frame_kind kind )
{
    const frame_asset a = asset_for( kind );
    if( kind == frame_kind::none ) {
        return a.content_inset;
    }
    const decoration lvl = theme::level();
    bool drawn = false;
    if( lvl != decoration::none && a.full != nullptr ) {
        const char *which = lvl == decoration::full ? a.full : a.reduced;
        const tex::texture t = tex::get( which );
        // Source assets are authored at 2x: 0.5 px per source px at scale 1.
        drawn = tex::draw_nine_slice( draw, t, min, max, a.margin_src, 0.5f * S() );
    }
    if( !drawn ) {
        draw_code_frame( draw, min, max, kind, lvl != decoration::none );
    }
    return a.content_inset;
}

void text_clipped( ImDrawList *draw, const ImVec2 &pos, const ImVec2 &clip_min, const ImVec2 &clip_max,
                   uint32_t col, const std::string &text )
{
    draw->PushClipRect( clip_min, clip_max, true );
    draw->AddText( ImGui::GetFont(), ImGui::GetFontSize(), pos, col, text.c_str() );
    draw->PopClipRect();
}

void chevron_glyph( ImDrawList *draw, const ImVec2 &center, float half, bool down, uint32_t col )
{
    // Code-drawn chevron: crisp 2px polyline.
    ImVec2 p[3];
    if( down ) {
        p[0] = ImVec2( center.x - half, center.y - half * 0.5f );
        p[1] = ImVec2( center.x, center.y + half * 0.5f );
        p[2] = ImVec2( center.x + half, center.y - half * 0.5f );
    } else {
        p[0] = ImVec2( center.x - half * 0.5f, center.y - half );
        p[1] = ImVec2( center.x + half * 0.5f, center.y );
        p[2] = ImVec2( center.x - half * 0.5f, center.y + half );
    }
    draw->AddPolyline( p, 3, col, 0, std::max( 2.f, 2.f * S() ) );
}

void draw_chevron( ImDrawList *draw, const ImVec2 &center, float size, bool down, uint32_t col )
{
    const char *name = down ? "chevron_down" : "chevron_right";
    const ImVec2 min( center.x - size * 0.5f, center.y - size * 0.5f );
    if( theme::level() == decoration::none || !tex::draw_icon( draw, name, min, size, col ) ) {
        chevron_glyph( draw, center, size * 0.3f, down, col );
    }
}

} // namespace

// ---------------------------------------------------------------------------
// fonts / text

void push_font_section()
{
    cataimgui::PushGuiFont1_25x();
}

void push_font_title()
{
    cataimgui::PushGuiFont1_5x();
}

void pop_font()
{
    ImGui::PopFont();
}

std::string fit_text( const std::string &text, float width_px )
{
    if( width_px <= 0.f ) {
        return std::string();
    }
    if( ImGui::CalcTextSize( text.c_str() ).x <= width_px ) {
        return text;
    }
    const char *ell = "…";
    const float ell_w = ImGui::CalcTextSize( ell ).x;
    // Walk UTF-8 code points and keep as many as fit before the ellipsis.
    size_t end = 0;
    size_t best = 0;
    while( end < text.size() ) {
        const unsigned char c = static_cast<unsigned char>( text[end] );
        size_t len = 1;
        if( c >= 0xF0 ) {
            len = 4;
        } else if( c >= 0xE0 ) {
            len = 3;
        } else if( c >= 0xC0 ) {
            len = 2;
        }
        const size_t next = std::min( text.size(), end + len );
        const float w = ImGui::CalcTextSize( text.c_str(), text.c_str() + next ).x;
        if( w + ell_w > width_px ) {
            break;
        }
        best = next;
        end = next;
    }
    return text.substr( 0, best ) + ell;
}

// ---------------------------------------------------------------------------
// window shell and layout

bool window_shell( const std::string &title, frame_kind kind, bool show_close, const char *icon_name,
                   bool show_title )
{
    ImGuiWindow *w = ImGui::GetCurrentWindow();
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const ImVec2 min = w->Pos;
    const ImVec2 max = w->Pos + w->Size;
    frame_rect( draw, min, max, kind );
    if( probe::enabled() ) {
        probe::record( "window", title, min, max );
    }
    if( !show_title ) {
        return false;
    }

    const float s = S();
    const float bar_h = T().title_bar * s;
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    bool closed = false;
    // Title
    ImGui::PushID( "##astral_shell" );
    const float close_size = T().close_hit * s;
    float text_x = origin.x;
    if( icon_name != nullptr ) {
        const float isz = T().icon * s;
        if( draw_icon_at( draw, icon_name, ImVec2( origin.x, origin.y + ( bar_h - isz ) * 0.5f ), isz,
                          T().accent ) ) {
            text_x += isz + T().sm * s;
        }
    }
    push_font_title();
    const float fs = ImGui::GetFontSize();
    const float right_limit = max.x - close_size - T().lg * s;
    text_clipped( draw, ImVec2( text_x, origin.y + ( bar_h - fs ) * 0.5f ), ImVec2( text_x, origin.y ),
                  ImVec2( right_limit, origin.y + bar_h ), T().text, title );
    pop_font();
    if( show_close ) {
        ImGui::SetCursorScreenPos( ImVec2( max.x - close_size - T().md * s,
                                           origin.y + ( bar_h - close_size ) * 0.5f ) );
        closed = close_button( "close" );
    }
    // Bronze rule under the title, fading to the right.
    const float rule_y = origin.y + bar_h;
    const float rule_w = max.x - T().md * s - origin.x;
    if( theme::level() != decoration::none ) {
        draw->AddRectFilledMultiColor( ImVec2( origin.x, rule_y ), ImVec2( origin.x + rule_w, rule_y + 2.f * s ),
                                       T().edge_bronze, alpha( T().edge_quiet, 0.f ), alpha( T().edge_quiet, 0.f ),
                                       T().edge_bronze );
    } else {
        draw->AddRectFilled( ImVec2( origin.x, rule_y ), ImVec2( origin.x + rule_w, rule_y + 1.f ),
                             T().edge_quiet );
    }
    ImGui::PopID();
    ImGui::SetCursorScreenPos( ImVec2( origin.x, rule_y + T().md * s ) );
    return closed;
}

bool body_begin( const char *id, float footer_height_logical, ImGuiWindowFlags flags )
{
    const float footer = footer_height_logical > 0.f ? footer_height_logical * S() : 0.f;
    const float avail = ImGui::GetContentRegionAvail().y;
    const float h = std::max( 60.f * S(), avail - footer );
    return ImGui::BeginChild( id, ImVec2( 0.f, h ), ImGuiChildFlags_None, flags );
}

void body_end()
{
    ImGui::EndChild();
}

bool footer_begin( const char *id, float height_logical )
{
    const float s = S();
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    draw->AddRectFilled( ImVec2( p.x, p.y ), ImVec2( p.x + w, p.y + 1.f ), T().edge_quiet );
    ImGui::Dummy( ImVec2( 0.f, T().sm * s ) );
    const float h = ( height_logical > 0.f ? height_logical : T().footer ) * s - T().sm * s;
    return ImGui::BeginChild( id, ImVec2( 0.f, h ), ImGuiChildFlags_None,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
}

void footer_end()
{
    ImGui::EndChild();
}

void footer_align_right( float items_width )
{
    const float avail = ImGui::GetContentRegionAvail().x;
    if( items_width < avail ) {
        ImGui::SetCursorPosX( ImGui::GetCursorPosX() + avail - items_width );
    }
}

void section_label( const std::string &text, const char *icon_name )
{
    const float s = S();
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    float x = p.x;
    push_font_section();
    const float fs = ImGui::GetFontSize();
    const float h = fs + T().xs * s;
    if( icon_name != nullptr ) {
        const float isz = fs;
        if( draw_icon_at( draw, icon_name, ImVec2( x, p.y + ( h - isz ) * 0.5f ), isz, T().accent ) ) {
            x += isz + T().sm * s;
        }
    }
    const ImVec2 tsz = ImGui::CalcTextSize( text.c_str() );
    draw->AddText( ImGui::GetFont(), fs, ImVec2( x, p.y + ( h - fs ) * 0.5f ), T().accent, text.c_str() );
    pop_font();
    const float rule_x = x + tsz.x + T().md * s;
    if( rule_x < p.x + w ) {
        draw->AddRectFilled( ImVec2( rule_x, p.y + h * 0.5f ), ImVec2( p.x + w, p.y + h * 0.5f + 1.f ),
                             T().edge_quiet );
    }
    ImGui::Dummy( ImVec2( w, h ) );
    ImGui::Dummy( ImVec2( 0.f, T().xs * s ) );
}

// ---------------------------------------------------------------------------
// buttons

float action_button_width( const char *label, button_kind kind, const char *icon_name )
{
    const float s = S();
    const bool has_icon = icon_name != nullptr && tex::has_icon( icon_name );
    const float icon_w = has_icon ? T().icon * 0.75f * s + T().sm * s : 0.f;
    const float text_w = ImGui::CalcTextSize( label ).x + icon_w;
    return kind == button_kind::tertiary ? text_w + T().lg * s
           : std::max( T().button_min_w * s, text_w + T().xl * 2.f * s );
}

bool action_button( const char *label, button_kind kind, const ImVec2 &size_logical, bool enabled,
                    const char *disabled_reason, const char *icon_name )
{
    const float s = S();
    const float h = ( size_logical.y > 0.f ? size_logical.y : T().button ) * s;
    float w = size_logical.x * s;
    const bool has_icon = icon_name != nullptr && tex::has_icon( icon_name );
    const float icon_w = has_icon ? T().icon * 0.75f * s + T().sm * s : 0.f;
    if( w <= 0.f ) {
        w = action_button_width( label, kind, icon_name );
    }
    if( !enabled ) {
        ImGui::BeginDisabled( true );
    }
    ImGui::PushStyleColor( ImGuiCol_Button, 0 );
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, 0 );
    ImGui::PushStyleColor( ImGuiCol_ButtonActive, 0 );
    ImGui::PushStyleColor( ImGuiCol_NavCursor, 0 ); // the focus ring is drawn below
    ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.f );
    const bool clicked = ImGui::Button( ( std::string( "##btn_" ) + label ).c_str(), ImVec2( w, h ) );
    ImGui::PopStyleVar();
    ImGui::PopStyleColor( 4 );
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();
    const bool focused = ImGui::IsItemFocused() && ImGui::GetIO().NavVisible;
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    if( probe::enabled() ) {
        probe::record( enabled ? "button" : "button_disabled", label, min, max );
    }
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const float r = T().radius_control * s;
    uint32_t text_col = T().text;
    bool textured = false;
    if( kind == button_kind::primary || kind == button_kind::secondary ) {
        const bool use_tex = theme::level() == decoration::full;
        const tex::texture t = use_tex ? tex::get( kind == button_kind::primary ? tex::asset::button_primary :
                               tex::asset::button_secondary ) : tex::texture{};
        uint32_t tint = 0xFFFFFFFFu;
        if( held ) {
            tint = IM_COL32( 200, 200, 200, 255 );
        } else if( hovered ) {
            tint = IM_COL32( 255, 255, 255, 255 );
        } else {
            tint = IM_COL32( 235, 235, 235, 255 );
        }
        textured = tex::draw_nine_slice( draw, t, min, max, 16.f, 0.5f * s, C( tint ) );
        if( !textured ) {
            uint32_t fill = kind == button_kind::primary ? ( held ? T().accent_dim : hovered ? T().accent :
                            T().accent_dim ) : ( held ? T().selected_bg : hovered ? T().raised_hover : T().raised );
            draw->AddRectFilled( min, max, C( fill ), r );
            draw->AddRect( min, max, C( kind == button_kind::primary ? T().bronze_dark : T().edge_quiet ), r );
        } else if( hovered && !held ) {
            draw->AddRectFilled( min, max, IM_COL32( 255, 255, 255, 18 ), r );
        }
        text_col = kind == button_kind::primary ? T().text_on_accent : T().text;
    } else {
        // tertiary / danger: text only, quiet edge on hover
        if( hovered || held ) {
            draw->AddRectFilled( min, max, C( held ? T().selected_bg : T().raised_hover ), r );
            draw->AddRect( min, max, C( T().edge_quiet ), r );
        }
        text_col = kind == button_kind::danger ? T().danger : ( hovered ? T().text : T().text_muted );
    }
    if( focused ) {
        draw->AddRect( min + ImVec2( 1, 1 ), max - ImVec2( 1, 1 ), C( T().accent ), r, 0, T().border_focus * s );
    }
    // label (+ icon)
    const ImVec2 tsz = ImGui::CalcTextSize( label );
    const float total_w = tsz.x + icon_w;
    float x = min.x + ( max.x - min.x - total_w ) * 0.5f;
    if( has_icon ) {
        const float isz = T().icon * 0.75f * s;
        draw_icon_at( draw, icon_name, ImVec2( x, min.y + ( h - isz ) * 0.5f ), isz, C( text_col ) );
        x += isz + T().sm * s;
    }
    text_clipped( draw, ImVec2( x, min.y + ( h - tsz.y ) * 0.5f ), min, max, C( text_col ), label );
    if( !enabled ) {
        ImGui::EndDisabled();
        if( disabled_reason != nullptr && ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) ) {
            tooltip( disabled_reason );
        }
    }
    return clicked && enabled;
}

bool icon_button( const char *id, const char *icon_name, float size_logical, bool active,
                  const char *tip, bool enabled )
{
    const float s = S();
    const float sz = size_logical * s;
    if( !enabled ) {
        ImGui::BeginDisabled( true );
    }
    ImGui::PushStyleColor( ImGuiCol_Button, 0 );
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, 0 );
    ImGui::PushStyleColor( ImGuiCol_ButtonActive, 0 );
    ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.f );
    const bool clicked = ImGui::Button( id, ImVec2( sz, sz ) );
    ImGui::PopStyleVar();
    ImGui::PopStyleColor( 3 );
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();
    const bool focused = ImGui::IsItemFocused() && ImGui::GetIO().NavVisible;
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    if( probe::enabled() ) {
        probe::record( "icon_button", id, min, max );
    }
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const float r = T().radius_control * s;
    if( active ) {
        draw->AddRectFilled( min, max, C( T().selected_bg ), r );
        draw->AddRect( min, max, C( T().accent ), r );
    } else if( hovered || held ) {
        draw->AddRectFilled( min, max, C( held ? T().selected_bg : T().raised_hover ), r );
        draw->AddRect( min, max, C( T().edge_quiet ), r );
    } else {
        draw->AddRect( min, max, C( alpha( T().edge_quiet, 0.6f ) ), r );
    }
    if( focused ) {
        draw->AddRect( min + ImVec2( 1, 1 ), max - ImVec2( 1, 1 ), C( T().accent ), r, 0, T().border_focus * s );
    }
    const float isz = sz * 0.6f;
    const uint32_t col = C( active ? T().accent : hovered ? T().text : T().text_muted );
    if( !draw_icon_at( draw, icon_name, ImVec2( min.x + ( sz - isz ) * 0.5f, min.y + ( sz - isz ) * 0.5f ),
                       isz, col ) ) {
        // fallback: first letter of the id
        std::string letter = std::string( 1, id[0] == '#' ? '?' : id[0] );
        const ImVec2 tsz = ImGui::CalcTextSize( letter.c_str() );
        draw->AddText( ImVec2( min.x + ( sz - tsz.x ) * 0.5f, min.y + ( sz - tsz.y ) * 0.5f ), col,
                       letter.c_str() );
    }
    if( !enabled ) {
        ImGui::EndDisabled();
    }
    if( tip != nullptr && ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) ) {
        tooltip( tip );
    }
    return clicked && enabled;
}

bool close_button( const char *id )
{
    const float s = S();
    const float sz = T().close_hit * s;
    ImGui::PushStyleColor( ImGuiCol_Button, 0 );
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, 0 );
    ImGui::PushStyleColor( ImGuiCol_ButtonActive, 0 );
    ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.f );
    const bool clicked = ImGui::Button( ( std::string( "##close_" ) + id ).c_str(), ImVec2( sz, sz ) );
    ImGui::PopStyleVar();
    ImGui::PopStyleColor( 3 );
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    if( probe::enabled() ) {
        probe::record( "close", id, min, max );
    }
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const float r = T().radius_control * s;
    if( hovered || held ) {
        draw->AddRectFilled( min, max, held ? T().selected_bg : T().raised_hover, r );
        draw->AddRect( min, max, T().edge_quiet, r );
    }
    const uint32_t col = hovered ? T().text : T().text_muted;
    const float isz = sz * 0.5f;
    const ImVec2 imin( min.x + ( sz - isz ) * 0.5f, min.y + ( sz - isz ) * 0.5f );
    if( theme::level() == decoration::none || !tex::draw_icon( draw, "close", imin, isz, col ) ) {
        const float th = std::max( 2.f, 2.f * s );
        draw->AddLine( imin, imin + ImVec2( isz, isz ), col, th );
        draw->AddLine( imin + ImVec2( isz, 0 ), imin + ImVec2( 0, isz ), col, th );
    }
    if( hovered ) {
        tooltip( _( "Close" ) );
    }
    return clicked;
}

float toolbar_button_width( const std::string &label, const char *icon_name, const std::string &key )
{
    const float s = S();
    const bool has_icon = icon_name != nullptr && tex::has_icon( icon_name );
    float w = ImGui::CalcTextSize( label.c_str() ).x + T().md * 2.f * s;
    if( has_icon ) {
        w += 18.f * s + T().xs * s;
    }
    if( !key.empty() ) {
        w += ImGui::CalcTextSize( key.c_str() ).x * 0.85f + T().sm * s;
    }
    return w;
}

bool toolbar_button( const char *id, const std::string &label, const char *icon_name,
                     const std::string &key, bool active, float height_logical )
{
    const float s = S();
    const float h = height_logical * s;
    const float w = toolbar_button_width( label, icon_name, key );
    ImGui::PushStyleColor( ImGuiCol_Button, 0 );
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, 0 );
    ImGui::PushStyleColor( ImGuiCol_ButtonActive, 0 );
    ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.f );
    const bool clicked = ImGui::Button( id, ImVec2( w, h ) );
    ImGui::PopStyleVar();
    ImGui::PopStyleColor( 3 );
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    if( probe::enabled() ) {
        probe::record( "toolbar", label, min, max );
    }
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const float r = T().radius_control * s;
    uint32_t fill = active ? T().selected_bg : held ? T().selected_bg : hovered ? T().raised_hover : T().raised;
    draw->AddRectFilled( min, max, fill, r );
    draw->AddRect( min, max, active ? T().accent : T().edge_quiet, r );
    float x = min.x + T().md * s;
    const bool has_icon = icon_name != nullptr && tex::has_icon( icon_name );
    if( has_icon ) {
        const float isz = 18.f * s;
        draw_icon_at( draw, icon_name, ImVec2( x, min.y + ( h - isz ) * 0.5f ), isz, active ? T().accent : T().text_muted );
        x += isz + T().xs * s;
    }
    const ImVec2 tsz = ImGui::CalcTextSize( label.c_str() );
    draw->AddText( ImVec2( x, min.y + ( h - tsz.y ) * 0.5f ), T().text, label.c_str() );
    x += tsz.x + T().sm * s;
    if( !key.empty() ) {
        const float kfs = ImGui::GetFontSize() * 0.85f;
        draw->AddText( ImGui::GetFont(), kfs, ImVec2( x, min.y + ( h - kfs ) * 0.5f ), T().accent_dim,
                       key.c_str() );
    }
    return clicked;
}

// ---------------------------------------------------------------------------
// rows

namespace
{

row_result row_impl( const char *id, const std::string &label, const std::string &detail,
                     const char *icon_name, const icon_painter &painter, int depth, bool branch,
                     bool open, bool show_chevron, const row_state &state, float height_logical )
{
    const float s = S();
    row_result res;
    const bool featured = state.featured;
    const float h = ( height_logical > 0.f ? height_logical : featured ? T().row_featured : T().row ) * s;
    const float indent = depth * T().tree_indent * s;
    if( indent > 0.f ) {
        ImGui::SetCursorPosX( ImGui::GetCursorPosX() + indent );
    }
    const float w = std::max( 40.f * s, ImGui::GetContentRegionAvail().x );
    ImGui::PushID( id );
    if( state.disabled ) {
        ImGui::BeginDisabled( true );
    }
    res.clicked = ImGui::InvisibleButton( "row", ImVec2( w, h ), ImGuiButtonFlags_MouseButtonLeft );
    if( state.disabled ) {
        ImGui::EndDisabled();
    }
    res.hovered = ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled );
    res.double_clicked = res.hovered && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left );
    // Right click: press and release on the same row (avoids stick drift opening a neighbour).
    static ImGuiID rmb_down_id = 0;
    const ImGuiID my_id = ImGui::GetItemID();
    if( res.hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Right ) ) {
        rmb_down_id = my_id;
    }
    if( res.hovered && ImGui::IsMouseReleased( ImGuiMouseButton_Right ) && rmb_down_id == my_id ) {
        res.right_clicked = true;
        rmb_down_id = 0;
    }
    const bool nav_focus = ImGui::IsItemFocused() && ImGui::GetIO().NavVisible;
    const bool focused = state.focused || nav_focus;
    res.id = my_id;
    res.min = ImGui::GetItemRectMin();
    res.max = ImGui::GetItemRectMax();
    if( probe::enabled() ) {
        probe::record( state.disabled ? "row_disabled" : state.selected ? "row_selected" : "row", label, res.min,
                       res.max );
    }
    ImDrawList *draw = ImGui::GetWindowDrawList();
    row_state bg_state = state;
    bg_state.focused = focused;
    draw_row_background( draw, res.min, res.max, bg_state, res.hovered );
    // tree connectors (quiet)
    if( depth > 0 ) {
        const float cx = res.min.x - T().tree_indent * s * 0.5f;
        const float cy = res.min.y + h * 0.5f;
        draw->AddRectFilled( ImVec2( cx, res.min.y - T().xs * s - 2.f * s ), ImVec2( cx + 1.f, cy + 1.f ),
                             alpha( T().edge_quiet, 0.9f ) );
        draw->AddRectFilled( ImVec2( cx, cy ), ImVec2( res.min.x - 2.f * s, cy + 1.f ), alpha( T().edge_quiet, 0.9f ) );
    }
    // icon box
    float x = res.min.x + T().md * s;
    const float isz = ( featured ? T().icon_featured : T().icon ) * s;
    const ImVec2 imin( x, res.min.y + ( h - isz ) * 0.5f );
    const ImVec2 imax( x + isz, imin.y + isz );
    bool has_icon = false;
    if( painter ) {
        painter( draw, imin, imax );
        has_icon = true;
    } else if( icon_name != nullptr ) {
        const uint32_t col = state.disabled ? T().text_muted : featured || state.selected ? T().accent :
                             T().text_muted;
        has_icon = draw_icon_at( draw, icon_name, imin, isz, col );
    }
    if( has_icon ) {
        x += isz + T().md * s;
    }
    // chevron and detail on the right
    float right = res.max.x - T().md * s;
    if( show_chevron ) {
        const float csz = T().lg * s;
        const uint32_t ccol = branch && open ? T().accent : T().text_muted;
        draw_chevron( draw, ImVec2( right - csz * 0.5f, res.min.y + h * 0.5f ), csz, branch && open, ccol );
        right -= csz + T().sm * s;
    }
    const uint32_t label_col = state.disabled ? T().text_muted : T().text;
    if( featured ) {
        push_font_section();
    }
    const float fs = ImGui::GetFontSize();
    float detail_w = 0.f;
    if( !detail.empty() ) {
        if( featured ) {
            pop_font();
        }
        const ImVec2 dsz = ImGui::CalcTextSize( detail.c_str() );
        detail_w = std::min( dsz.x, ( right - x ) * 0.45f );
        const std::string d = fit_text( detail, detail_w );
        const float dfs = ImGui::GetFontSize();
        draw->AddText( ImVec2( right - detail_w, res.min.y + ( h - dfs ) * 0.5f ), T().text_muted, d.c_str() );
        right -= detail_w + T().md * s;
        if( featured ) {
            push_font_section();
        }
    }
    const std::string l = fit_text( label, std::max( 10.f, right - x ) );
    draw->AddText( ImGui::GetFont(), fs, ImVec2( x, res.min.y + ( h - fs ) * 0.5f ), label_col, l.c_str() );
    if( featured ) {
        pop_font();
    }
    res.truncated = l != label;
    ImGui::PopID();
    return res;
}

} // namespace

void draw_row_background( ImDrawList *draw, const ImVec2 &min, const ImVec2 &max,
                          const row_state &state, bool hovered )
{
    const float s = S();
    const float r = T().radius_control * s;
    uint32_t fill = T().raised;
    if( state.selected ) {
        fill = T().selected_bg;
    } else if( state.focused ) {
        fill = T().focus_bg;
    } else if( hovered && !state.disabled ) {
        fill = T().raised_hover;
    }
    draw->AddRectFilled( min, max, state.disabled ? alpha( fill, 0.6f ) : fill, r );
    // edge: featured rows quiet edge; selected accent; focused accent ring
    if( state.selected ) {
        solid_outline( draw, min, max, T().accent, std::max( 1.f, 1.f * s ) );
    } else if( state.featured ) {
        solid_outline( draw, min, max, alpha( T().edge_quiet, 0.85f ), 1.f );
    }
    if( state.focused ) {
        draw->AddRect( min + ImVec2( 2, 2 ) * s, max - ImVec2( 2, 2 ) * s, T().accent, r, 0,
                       T().border_focus * s );
    }
}

row_result selectable_row( const char *id, const std::string &label, const std::string &detail,
                           const char *icon_name, const icon_painter &painter, const row_state &state,
                           float height_logical, const char *chevron )
{
    const bool show = chevron != nullptr;
    return row_impl( id, label, detail, icon_name, painter, 0, show, false, show, state, height_logical );
}

row_result tree_row( const char *id, const std::string &label, const std::string &detail,
                     const char *icon_name, const icon_painter &painter, int depth, bool branch,
                     bool open, const row_state &state, float height_logical )
{
    row_state st = state;
    if( depth == 0 ) {
        st.featured = true;
    }
    return row_impl( id, label, detail, icon_name, painter, depth, branch, open, branch, st,
                     height_logical );
}

// ---------------------------------------------------------------------------
// meters, icons, tooltips, hints

void meter( const char *id, float fraction, meter_kind kind, const std::string &text,
            float width_logical, float height_logical )
{
    const float s = S();
    const float f = std::clamp( fraction, 0.f, 1.f );
    const float w = width_logical > 0.f ? width_logical * s : ImGui::GetContentRegionAvail().x;
    const float h = ( height_logical > 0.f ? height_logical : 16.f ) * s;
    uint32_t col = T().accent;
    switch( kind ) {
        case meter_kind::neutral:
            col = T().accent;
            break;
        case meter_kind::health:
            col = f > 0.6f ? T().success : f > 0.3f ? T().warning : T().danger;
            break;
        case meter_kind::stamina:
            col = f > 0.35f ? T().warning : T().danger;
            break;
        case meter_kind::morale:
        case meter_kind::info:
            col = T().info;
            break;
        case meter_kind::danger:
            col = T().danger;
            break;
        case meter_kind::success:
            col = T().success;
            break;
        case meter_kind::warning:
            col = T().warning;
            break;
    }
    ImGui::PushID( id );
    ImGui::InvisibleButton( "meter", ImVec2( w, h ) );
    ImGui::PopID();
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const float r = T().radius_control * s;
    bool drawn = false;
    if( theme::level() == decoration::full ) {
        drawn = tex::draw_nine_slice( draw, tex::get( tex::asset::meter_track ), min, max, 8.f, 0.5f * s );
        if( drawn && f > 0.f ) {
            const ImVec2 fmax( min.x + std::max( 6.f * s, ( max.x - min.x ) * f ), max.y );
            tex::draw_nine_slice( draw, tex::get( tex::asset::meter_fill ), min + ImVec2( 1, 1 ), fmax - ImVec2( 1, 1 ),
                                  8.f, 0.5f * s, col );
        }
    }
    if( !drawn ) {
        draw->AddRectFilled( min, max, T().meter_track, r );
        draw->AddRect( min, max, T().edge_quiet, r );
        if( f > 0.f ) {
            draw->AddRectFilled( min + ImVec2( 1, 1 ), ImVec2( min.x + std::max( 4.f, ( max.x - min.x - 2.f ) * f ),
                                 max.y - 1.f ), col, r );
        }
    }
    if( !text.empty() ) {
        const ImVec2 tsz = ImGui::CalcTextSize( text.c_str() );
        const ImVec2 tp( min.x + ( max.x - min.x - tsz.x ) * 0.5f, min.y + ( max.y - min.y - tsz.y ) * 0.5f );
        draw->AddText( tp + ImVec2( 1, 1 ), IM_COL32( 0, 0, 0, 200 ), text.c_str() );
        draw->AddText( tp, T().text, text.c_str() );
    }
}

bool draw_icon_at( ImDrawList *draw, const char *name, const ImVec2 &min, float size_px, uint32_t tint )
{
    if( name == nullptr ) {
        return false;
    }
    return tex::draw_icon( draw, name, min, size_px, tint == 0 ? T().text : tint );
}

void icon( const char *name, float size_logical, uint32_t tint )
{
    const float sz = size_logical * S();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy( ImVec2( sz, sz ) );
    if( !draw_icon_at( ImGui::GetWindowDrawList(), name, p, sz, tint ) ) {
        // missing-art fallback: dashed square
        ImGui::GetWindowDrawList()->AddRect( p, p + ImVec2( sz, sz ), T().edge_quiet, 2.f );
    }
}

void tooltip( const std::string &text )
{
    if( text.empty() ) {
        return;
    }
    const float s = S();
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( T().md * s, T().sm * s ) );
    ImGui::PushStyleColor( ImGuiCol_Border, ui_hybrid_chrome::palette::from_u32( T().edge_bronze ) );
    if( ImGui::BeginTooltip() ) {
        const float wrap = std::min( 480.f * s, ImGui::GetMainViewport()->Size.x * 0.6f );
        ImGui::PushTextWrapPos( wrap );
        cataimgui::draw_colored_text( text, wrap );
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void hint( const input_context &ctxt, const std::string &action, const std::string &description,
           bool active )
{
    // Prefer a keyboard binding that is not a bare digit (keypad aliases such as
    // "1" read badly as hints); fall back to any keyboard binding.
    const auto keyboard = []( const input_event & e ) {
        return e.type == input_event_t::keyboard_char || e.type == input_event_t::keyboard_code;
    };
    std::string key = ctxt.get_desc( action, 1, [&]( const input_event & e ) {
        if( !keyboard( e ) ) {
            return false;
        }
        const std::string d = e.long_description();
        return !( d.size() == 1 && d[0] >= '0' && d[0] <= '9' );
    } );
    if( key.empty() ) {
        key = ctxt.get_desc( action, 1, keyboard );
    }
    ImGui::PushStyleColor( ImGuiCol_Text, ui_hybrid_chrome::palette::from_u32( active ? T().accent :
                           T().accent_dim ) );
    ImGui::TextUnformatted( key.c_str() );
    ImGui::PopStyleColor();
    ImGui::SameLine( 0.f, T().xs * S() );
    ImGui::PushStyleColor( ImGuiCol_Text, ui_hybrid_chrome::palette::from_u32( active ? T().text :
                           T().text_muted ) );
    ImGui::TextUnformatted( description.c_str() );
    ImGui::PopStyleColor();
}

void hint_key( const std::string &key, const std::string &description )
{
    ImGui::PushStyleColor( ImGuiCol_Text, ui_hybrid_chrome::palette::from_u32( T().accent_dim ) );
    ImGui::TextUnformatted( key.c_str() );
    ImGui::PopStyleColor();
    ImGui::SameLine( 0.f, T().xs * S() );
    ImGui::PushStyleColor( ImGuiCol_Text, ui_hybrid_chrome::palette::from_u32( T().text_muted ) );
    ImGui::TextUnformatted( description.c_str() );
    ImGui::PopStyleColor();
}

// ---------------------------------------------------------------------------
// panels, cards, tabs, empty state, scrim, frames

bool panel_begin( const char *id, const ImVec2 &size, bool inset, ImGuiWindowFlags flags )
{
    const float s = S();
    ImGui::PushStyleColor( ImGuiCol_ChildBg, 0 );
    ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 0.f );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( T().sm * s, T().sm * s ) );
    const bool open = ImGui::BeginChild( id, size, ImGuiChildFlags_AlwaysUseWindowPadding, flags );
    ImGui::PopStyleVar( 2 );
    ImGui::PopStyleColor();
    // Background on the child's own draw list (behind its content).
    ImGuiWindow *w = ImGui::GetCurrentWindow();
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const ImVec2 min = w->Pos;
    const ImVec2 max = w->Pos + w->Size;
    bool drawn = false;
    if( inset && theme::level() != decoration::none ) {
        drawn = tex::draw_nine_slice( draw, tex::get( tex::asset::panel_inset ), min, max, 16.f, 0.5f * s );
    }
    if( !drawn ) {
        const float r = T().radius_panel * s;
        draw->AddRectFilled( min, max, inset ? T().deep_bg : T().surface, r );
        draw->AddRect( min, max, T().edge_quiet, r );
    }
    return open;
}

void panel_end()
{
    ImGui::EndChild();
}

bool card_begin( const char *id, const ImVec2 &size, bool selected, ImGuiWindowFlags flags )
{
    const float s = S();
    ImGui::PushStyleColor( ImGuiCol_ChildBg, 0 );
    ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 0.f );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( T().md * s, T().sm * s ) );
    const bool open = ImGui::BeginChild( id, size, ImGuiChildFlags_AlwaysUseWindowPadding, flags );
    ImGui::PopStyleVar( 2 );
    ImGui::PopStyleColor();
    ImGuiWindow *w = ImGui::GetCurrentWindow();
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const ImVec2 min = w->Pos;
    const ImVec2 max = w->Pos + w->Size;
    bool drawn = false;
    if( theme::level() != decoration::none ) {
        drawn = tex::draw_nine_slice( draw, tex::get( tex::asset::card ), min, max, 16.f, 0.5f * s );
    }
    const float r = T().radius_panel * s;
    if( !drawn ) {
        draw->AddRectFilled( min, max, T().raised, r );
        draw->AddRect( min, max, T().edge_quiet, r );
    }
    if( selected ) {
        solid_outline( draw, min, max, T().accent, std::max( 1.f, 1.f * s ) );
    }
    return open;
}

void card_end()
{
    ImGui::EndChild();
}

bool tab( const char *label, bool selected, const char *icon_name )
{
    const float s = S();
    const bool has_icon = icon_name != nullptr && tex::has_icon( icon_name );
    const float isz = has_icon ? ImGui::GetFontSize() : 0.f;
    const float w = ImGui::CalcTextSize( label ).x + T().lg * 2.f * s + ( has_icon ? isz + T().sm * s : 0.f );
    const float h = T().button * s;
    ImGui::PushStyleColor( ImGuiCol_Button, 0 );
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, 0 );
    ImGui::PushStyleColor( ImGuiCol_ButtonActive, 0 );
    ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.f );
    const bool clicked = ImGui::Button( ( std::string( "##tab_" ) + label ).c_str(), ImVec2( w, h ) );
    ImGui::PopStyleVar();
    ImGui::PopStyleColor( 3 );
    const bool hovered = ImGui::IsItemHovered();
    const bool focused = ImGui::IsItemFocused() && ImGui::GetIO().NavVisible;
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    if( probe::enabled() ) {
        probe::record( selected ? "tab_selected" : "tab", label, min, max );
    }
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const float r = T().radius_control * s;
    if( selected ) {
        draw->AddRectFilled( min, max, T().raised, r, ImDrawFlags_RoundCornersTop );
        draw->AddRectFilled( ImVec2( min.x, max.y - 2.f * s ), max, T().accent );
    } else if( hovered ) {
        draw->AddRectFilled( min, max, T().raised_hover, r, ImDrawFlags_RoundCornersTop );
    }
    if( focused ) {
        draw->AddRect( min + ImVec2( 1, 1 ), max - ImVec2( 1, 1 ), T().accent, r, 0, T().border_focus * s );
    }
    float x = min.x + T().lg * s;
    const uint32_t col = selected ? T().text : hovered ? T().text : T().text_muted;
    if( has_icon ) {
        draw_icon_at( draw, icon_name, ImVec2( x, min.y + ( h - isz ) * 0.5f ), isz, selected ? T().accent : col );
        x += isz + T().sm * s;
    }
    const ImVec2 tsz = ImGui::CalcTextSize( label );
    draw->AddText( ImVec2( x, min.y + ( h - tsz.y ) * 0.5f ), col, label );
    return clicked;
}

void empty_state( const std::string &title, const std::string &detail )
{
    const float s = S();
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const float isz = T().icon_featured * s;
    const float block_h = isz + T().sm * s + ImGui::GetTextLineHeight() * ( detail.empty() ? 1.f : 2.2f );
    float y = p.y + std::max( 0.f, ( avail.y - block_h ) * 0.4f );
    draw_icon_at( draw, "empty_slot", ImVec2( p.x + ( avail.x - isz ) * 0.5f, y ), isz, T().edge_quiet );
    y += isz + T().sm * s;
    const ImVec2 tsz = ImGui::CalcTextSize( title.c_str() );
    draw->AddText( ImVec2( p.x + ( avail.x - tsz.x ) * 0.5f, y ), T().text_muted, title.c_str() );
    if( !detail.empty() ) {
        y += ImGui::GetTextLineHeight() * 1.2f;
        const ImVec2 dsz = ImGui::CalcTextSize( detail.c_str() );
        draw->AddText( ImVec2( p.x + ( avail.x - dsz.x ) * 0.5f, y ), alpha( T().text_muted, 0.75f ),
                       detail.c_str() );
    }
    ImGui::Dummy( ImVec2( avail.x, std::max( block_h, avail.y ) ) );
}

void scrim()
{
    ImGuiViewport *vp = ImGui::GetMainViewport();
    ImGui::GetBackgroundDrawList( vp )->AddRectFilled( vp->Pos, vp->Pos + vp->Size, T().scrim );
}

void draw_frame_rect( ImDrawList *draw, const ImVec2 &min, const ImVec2 &max, frame_kind kind )
{
    frame_rect( draw, min, max, kind );
}

void portrait_frame( ImDrawList *draw, const ImVec2 &min, const ImVec2 &max, ImVec2 &inner_min,
                     ImVec2 &inner_max )
{
    const float s = S();
    bool drawn = false;
    if( theme::level() != decoration::none ) {
        drawn = tex::draw_nine_slice( draw, tex::get( tex::asset::portrait ), min, max, 32.f, 0.5f * s );
    }
    if( !drawn ) {
        const float r = T().radius_panel * s;
        draw->AddRectFilled( min, max, T().deep_bg, r );
        solid_outline( draw, min, max, theme::level() == decoration::none ? T().edge_quiet : T().edge_bronze,
                       std::max( 1.f, 2.f * s ) );
    }
    const float inset = T().md * s;
    inner_min = min + ImVec2( inset, inset );
    inner_max = max - ImVec2( inset, inset );
}

// ---------------------------------------------------------------------------
// widget probe (native interaction checks)

namespace probe
{
namespace
{
struct entry {
    std::string kind;
    std::string label;
    ImVec2 min;
    ImVec2 max;
    bool visible;
};
std::vector<entry> &entries()
{
    static std::vector<entry> e;
    return e;
}
const char *probe_path()
{
    static const char *path = std::getenv( "CDDA_UI_PROBE" );
    return path;
}
std::string json_escape( const std::string &in )
{
    std::string out;
    out.reserve( in.size() + 2 );
    for( const char c : in ) {
        switch( c ) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            default:
                if( static_cast<unsigned char>( c ) < 0x20 ) {
                    out += ' ';
                } else {
                    out += c;
                }
        }
    }
    return out;
}
} // namespace

bool enabled()
{
    const char *p = probe_path();
    return p != nullptr && *p != '\0';
}

void record( const char *kind, const std::string &label, const ImVec2 &min, const ImVec2 &max )
{
    if( !enabled() ) {
        return;
    }
    std::string clean = label;
    const std::size_t hh = clean.find( "##" );
    if( hh != std::string::npos ) {
        clean.erase( hh );
    }
    // Rows outside the parent's clip rect are submitted but not on screen; the harness
    // must not click those coordinates.
    bool visible = true;
    if( ImGuiWindow *w = ImGui::GetCurrentWindowRead() ) {
        const ImRect clip = w->ClipRect;
        visible = max.x > clip.Min.x && min.x < clip.Max.x && max.y > clip.Min.y && min.y < clip.Max.y;
    }
    entries().push_back( { kind, clean, min, max, visible } );
}

void flush_frame()
{
    if( !enabled() ) {
        return;
    }
    static int frame = 0;
    ++frame;
    std::vector<entry> &e = entries();
    // Scrollbar audit: every window/child that showed a vertical scrollbar last frame,
    // with how much it actually overflows. A scrollbar over a few pixels of overflow
    // ("tiny scrollbar") is a sizing bug in the screen that owns it.
    if( ImGuiContext *g = ImGui::GetCurrentContext() ) {
        for( ImGuiWindow *w : g->Windows ) {
            if( !w->WasActive || w->Hidden || !w->ScrollbarY ) {
                continue;
            }
            const float overflow = w->ScrollMax.y;
            const float view = w->InnerRect.GetHeight();
            std::string label = std::string( "scrollbar:" ) + w->Name;
            const std::size_t hh = label.find( "##" );
            if( hh != std::string::npos ) {
                label.erase( hh );
            }
            label += " overflow=" + std::to_string( static_cast<int>( overflow ) ) + " view=" +
                     std::to_string( static_cast<int>( view ) );
            e.push_back( { overflow < view * 0.25f ? "scrollbar_tiny" : "scrollbar", label,
                           w->Pos, ImVec2( w->Pos.x + w->Size.x, w->Pos.y + w->Size.y ), true } );
        }
    }
    // Written every frame (atomic rename) so the harness can wait for a frame that
    // follows its input; the file is small and this is a dev-only path.
    {
        const std::string path = probe_path();
        const std::string tmp = path + std::string( ".tmp" );
        std::ofstream out( tmp, std::ios::trunc );
        if( out ) {
            out << "{\"frame\":" << frame << ",\"widgets\":[";
            bool first = true;
            for( const entry &it : e ) {
                if( !first ) {
                    out << ',';
                }
                first = false;
                out << "{\"kind\":\"" << it.kind << "\",\"label\":\"" << json_escape( it.label )
                    << "\",\"min\":[" << static_cast<int>( it.min.x ) << ',' << static_cast<int>( it.min.y )
                    << "],\"max\":[" << static_cast<int>( it.max.x ) << ',' << static_cast<int>( it.max.y )
                    << "],\"visible\":" << ( it.visible ? "true" : "false" ) << '}';
            }
            out << "]}\n";
            out.close();
            std::rename( tmp.c_str(), path.c_str() );
        }
    }
    e.clear();
}

} // namespace probe

} // namespace ui_hybrid_widgets
