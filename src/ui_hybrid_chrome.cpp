#include "ui_hybrid_chrome.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "imgui/imgui.h"
#include "options.h"

namespace ui_hybrid_chrome
{

namespace
{

constexpr uint32_t rgba( uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255 )
{
    return IM_COL32( r, g, b, a );
}

// Mirrors data/ui/astral/theme.json v1.
const theme::tokens g_tokens = {
    /* deep_bg      */ rgba( 0x11, 0x15, 0x16 ),
    /* surface      */ rgba( 0x1B, 0x1D, 0x1D ),
    /* raised       */ rgba( 0x25, 0x27, 0x25 ),
    /* raised_hover */ rgba( 0x2E, 0x30, 0x2D ),
    /* edge_dark    */ rgba( 0x0B, 0x0D, 0x0E ),
    /* edge_quiet   */ rgba( 0x58, 0x46, 0x2E ),
    /* edge_bronze  */ rgba( 0x9B, 0x75, 0x44 ),
    /* bronze_light */ rgba( 0xC6, 0x9A, 0x5D ),
    /* bronze_dark  */ rgba( 0x6A, 0x4F, 0x2C ),
    /* accent       */ rgba( 0xD6, 0xA4, 0x57 ),
    /* accent_dim   */ rgba( 0x8E, 0x6D, 0x3A ),
    /* selected_bg  */ rgba( 0x3A, 0x2D, 0x1A ),
    /* focus_bg     */ rgba( 0x2F, 0x2A, 0x20 ),
    /* text         */ rgba( 0xEE, 0xE3, 0xCB ),
    /* text_muted   */ rgba( 0xB7, 0xAE, 0x9D ),
    /* text_on_acc  */ rgba( 0x1A, 0x14, 0x08 ),
    /* danger       */ rgba( 0xCF, 0x70, 0x64 ),
    /* success      */ rgba( 0x91, 0xB1, 0x7C ),
    /* info         */ rgba( 0x8F, 0xAE, 0xBB ),
    /* warning      */ rgba( 0xD9, 0xB0, 0x54 ),
    /* meter_track  */ rgba( 0x15, 0x18, 0x17 ),
    /* scrim        */ rgba( 0x00, 0x00, 0x00, 140 ),
    /* xs sm md lg xl */ 4.f, 8.f, 12.f, 16.f, 24.f,
    /* radius window/panel/control */ 6.f, 4.f, 3.f,
    /* border frame/quiet/focus */ 2.f, 1.f, 2.f,
    /* row, row_featured, row_compact, button, button_min_w, footer, title_bar, close_hit */
    40.f, 48.f, 32.f, 42.f, 120.f, 64.f, 44.f, 36.f,
    /* icon, icon_featured, tree_indent, scrollbar */ 28.f, 36.f, 20.f, 12.f,
};

int g_level_override = -1;
decoration g_level = decoration::full;

thread_local std::vector<int> style_var_stack;

ImVec4 v4( uint32_t c )
{
    return ImGui::ColorConvertU32ToFloat4( c );
}

ImVec4 with_alpha( uint32_t c, float a )
{
    ImVec4 v = v4( c );
    v.w = a;
    return v;
}

} // namespace

namespace theme
{

const tokens &get()
{
    return g_tokens;
}

float scale()
{
    if( ImGui::GetCurrentContext() == nullptr ) {
        return 1.f;
    }
    return std::max( 1.f, ImGui::GetFontSize() / 16.f );
}

float px( float logical )
{
    return logical * scale();
}

ImVec2 large_window_size()
{
    const ImVec2 vp = ImGui::GetMainViewport()->Size;
    const float s = scale();
    return ImVec2( std::min( vp.x * 0.94f, std::max( 1280.f * s, vp.x * 0.62f ) ),
                   std::min( vp.y * 0.92f, std::max( 800.f * s, vp.y * 0.78f ) ) );
}

decoration level()
{
    if( g_level_override >= 0 ) {
        return static_cast<decoration>( g_level_override );
    }
    return g_level;
}

void refresh_options()
{
    // The option may not exist yet during early startup; keep the default.
    if( !get_options().has_option( "ASTRAL_UI_DECORATION" ) ) {
        return;
    }
    const std::string v = get_option<std::string>( "ASTRAL_UI_DECORATION" );
    if( v == "reduced" ) {
        g_level = decoration::reduced;
    } else if( v == "none" ) {
        g_level = decoration::none;
    } else {
        g_level = decoration::full;
    }
}

void override_level( int level )
{
    g_level_override = level;
}

} // namespace theme

namespace palette
{

ImVec4 from_u32( uint32_t c )
{
    return v4( c );
}
ImVec4 window_bg()
{
    return v4( g_tokens.surface );
}
ImVec4 child_bg()
{
    return with_alpha( g_tokens.surface, 0.f );
}
ImVec4 popup_bg()
{
    return with_alpha( g_tokens.surface, 0.97f );
}
ImVec4 border()
{
    return v4( g_tokens.edge_quiet );
}
ImVec4 border_accent()
{
    return v4( g_tokens.accent );
}
ImVec4 button()
{
    return v4( g_tokens.raised );
}
ImVec4 button_hovered()
{
    return v4( g_tokens.raised_hover );
}
ImVec4 button_active()
{
    return v4( g_tokens.selected_bg );
}
ImVec4 header()
{
    return v4( g_tokens.raised );
}
ImVec4 header_hovered()
{
    return v4( g_tokens.raised_hover );
}
ImVec4 text()
{
    return v4( g_tokens.text );
}
ImVec4 text_muted()
{
    return v4( g_tokens.text_muted );
}
ImVec4 accent()
{
    return v4( g_tokens.accent );
}
ImVec4 accent_dim()
{
    return v4( g_tokens.accent_dim );
}
ImVec4 separator()
{
    return v4( g_tokens.edge_quiet );
}
ImVec4 title_bg()
{
    return v4( g_tokens.deep_bg );
}
ImVec4 scrollbar_grab()
{
    return v4( g_tokens.accent_dim );
}
ImVec4 slot_empty()
{
    return v4( g_tokens.deep_bg );
}
ImVec4 slot_selected()
{
    return v4( g_tokens.selected_bg );
}
ImVec4 grid_selected()
{
    return v4( g_tokens.selected_bg );
}
ImVec4 toolbar_active()
{
    return v4( g_tokens.selected_bg );
}
ImVec4 toolbar_active_hovered()
{
    return v4( g_tokens.focus_bg );
}
ImVec4 toolbar_active_pressed()
{
    return v4( g_tokens.accent_dim );
}
ImVec4 danger()
{
    return v4( g_tokens.danger );
}
ImVec4 success()
{
    return v4( g_tokens.success );
}
ImVec4 info()
{
    return v4( g_tokens.info );
}
ImVec4 warning()
{
    return v4( g_tokens.warning );
}

} // namespace palette

namespace
{

// Every ImGuiCol_ slot the theme owns, with its token color.
struct color_slot {
    ImGuiCol idx;
    ImVec4 color;
};

std::vector<color_slot> theme_colors()
{
    const theme::tokens &t = g_tokens;
    return {
        { ImGuiCol_Text, v4( t.text ) },
        { ImGuiCol_TextDisabled, v4( t.text_muted ) },
        { ImGuiCol_WindowBg, v4( t.surface ) },
        { ImGuiCol_ChildBg, with_alpha( t.surface, 0.f ) },
        { ImGuiCol_PopupBg, with_alpha( t.surface, 0.97f ) },
        { ImGuiCol_Border, v4( t.edge_quiet ) },
        { ImGuiCol_BorderShadow, with_alpha( t.edge_dark, 0.f ) },
        { ImGuiCol_FrameBg, v4( t.deep_bg ) },
        { ImGuiCol_FrameBgHovered, v4( t.raised ) },
        { ImGuiCol_FrameBgActive, v4( t.raised_hover ) },
        { ImGuiCol_TitleBg, v4( t.deep_bg ) },
        { ImGuiCol_TitleBgActive, v4( t.deep_bg ) },
        { ImGuiCol_TitleBgCollapsed, v4( t.deep_bg ) },
        { ImGuiCol_MenuBarBg, v4( t.raised ) },
        { ImGuiCol_ScrollbarBg, v4( t.deep_bg ) },
        { ImGuiCol_ScrollbarGrab, v4( t.accent_dim ) },
        { ImGuiCol_ScrollbarGrabHovered, v4( t.edge_bronze ) },
        { ImGuiCol_ScrollbarGrabActive, v4( t.accent ) },
        { ImGuiCol_CheckMark, v4( t.accent ) },
        { ImGuiCol_CheckboxSelectedBg, v4( t.selected_bg ) },
        { ImGuiCol_SliderGrab, v4( t.accent_dim ) },
        { ImGuiCol_SliderGrabActive, v4( t.accent ) },
        { ImGuiCol_Button, v4( t.raised ) },
        { ImGuiCol_ButtonHovered, v4( t.raised_hover ) },
        { ImGuiCol_ButtonActive, v4( t.selected_bg ) },
        { ImGuiCol_Header, v4( t.raised ) },
        { ImGuiCol_HeaderHovered, v4( t.raised_hover ) },
        { ImGuiCol_HeaderActive, v4( t.selected_bg ) },
        { ImGuiCol_Separator, v4( t.edge_quiet ) },
        { ImGuiCol_SeparatorHovered, v4( t.edge_bronze ) },
        { ImGuiCol_SeparatorActive, v4( t.accent ) },
        { ImGuiCol_ResizeGrip, with_alpha( t.edge_quiet, 0.5f ) },
        { ImGuiCol_ResizeGripHovered, v4( t.edge_bronze ) },
        { ImGuiCol_ResizeGripActive, v4( t.accent ) },
        { ImGuiCol_Tab, with_alpha( t.raised, 0.f ) },
        { ImGuiCol_TabHovered, v4( t.raised_hover ) },
        { ImGuiCol_TabSelected, v4( t.raised ) },
        { ImGuiCol_TabSelectedOverline, v4( t.accent ) },
        { ImGuiCol_TabDimmed, with_alpha( t.raised, 0.f ) },
        { ImGuiCol_TabDimmedSelected, v4( t.raised ) },
        { ImGuiCol_TabDimmedSelectedOverline, v4( t.accent_dim ) },
        { ImGuiCol_PlotLines, v4( t.info ) },
        { ImGuiCol_PlotLinesHovered, v4( t.accent ) },
        { ImGuiCol_PlotHistogram, v4( t.accent ) },
        { ImGuiCol_PlotHistogramHovered, v4( t.bronze_light ) },
        { ImGuiCol_TableHeaderBg, v4( t.raised ) },
        { ImGuiCol_TableBorderStrong, v4( t.edge_quiet ) },
        { ImGuiCol_TableBorderLight, with_alpha( t.edge_quiet, 0.5f ) },
        { ImGuiCol_TableRowBg, with_alpha( t.surface, 0.f ) },
        { ImGuiCol_TableRowBgAlt, with_alpha( t.raised, 0.35f ) },
        { ImGuiCol_TextLink, v4( t.accent ) },
        { ImGuiCol_TextSelectedBg, with_alpha( t.accent_dim, 0.45f ) },
        { ImGuiCol_DragDropTarget, v4( t.accent ) },
        { ImGuiCol_NavCursor, v4( t.accent ) },
        { ImGuiCol_NavWindowingHighlight, with_alpha( t.accent, 0.7f ) },
        { ImGuiCol_NavWindowingDimBg, with_alpha( t.edge_dark, 0.4f ) },
        { ImGuiCol_ModalWindowDimBg, with_alpha( t.edge_dark, 0.55f ) },
    };
}

void push_structure( ImGuiStyle *dst_or_null )
{
    const theme::tokens &t = g_tokens;
    const float s = theme::scale();
    struct var {
        ImGuiStyleVar idx;
        float value;
    };
    struct var2 {
        ImGuiStyleVar idx;
        ImVec2 value;
    };
    const var vars[] = {
        { ImGuiStyleVar_WindowRounding, t.radius_window * s },
        { ImGuiStyleVar_ChildRounding, t.radius_panel * s },
        { ImGuiStyleVar_FrameRounding, t.radius_control * s },
        { ImGuiStyleVar_PopupRounding, t.radius_panel * s },
        { ImGuiStyleVar_ScrollbarRounding, t.radius_control * s },
        { ImGuiStyleVar_GrabRounding, t.radius_control * s },
        { ImGuiStyleVar_TabRounding, t.radius_control * s },
        { ImGuiStyleVar_FrameBorderSize, t.border_quiet },
        { ImGuiStyleVar_ChildBorderSize, t.border_quiet },
        { ImGuiStyleVar_WindowBorderSize, t.border_quiet },
        { ImGuiStyleVar_PopupBorderSize, t.border_quiet },
        { ImGuiStyleVar_ScrollbarSize, t.scrollbar * s },
        { ImGuiStyleVar_TabBarBorderSize, t.border_quiet },
    };
    const var2 vars2[] = {
        { ImGuiStyleVar_WindowPadding, ImVec2( t.md * s, t.md * s ) },
        { ImGuiStyleVar_FramePadding, ImVec2( t.sm * s, t.xs * s + 2.f * s ) },
        { ImGuiStyleVar_ItemSpacing, ImVec2( t.sm * s, t.xs * s + 2.f * s ) },
        { ImGuiStyleVar_ItemInnerSpacing, ImVec2( t.xs * s + 2.f * s, t.xs * s ) },
        { ImGuiStyleVar_CellPadding, ImVec2( t.sm * s, t.xs * s ) },
        { ImGuiStyleVar_WindowTitleAlign, ImVec2( 0.f, 0.5f ) },
    };
    if( dst_or_null ) {
        ImGuiStyle &st = *dst_or_null;
        st.WindowRounding = vars[0].value;
        st.ChildRounding = vars[1].value;
        st.FrameRounding = vars[2].value;
        st.PopupRounding = vars[3].value;
        st.ScrollbarRounding = vars[4].value;
        st.GrabRounding = vars[5].value;
        st.TabRounding = vars[6].value;
        st.FrameBorderSize = vars[7].value;
        st.ChildBorderSize = vars[8].value;
        st.WindowBorderSize = vars[9].value;
        st.PopupBorderSize = vars[10].value;
        st.ScrollbarSize = vars[11].value;
        st.TabBarBorderSize = vars[12].value;
        st.WindowPadding = vars2[0].value;
        st.FramePadding = vars2[1].value;
        st.ItemSpacing = vars2[2].value;
        st.ItemInnerSpacing = vars2[3].value;
        st.CellPadding = vars2[4].value;
        st.WindowTitleAlign = vars2[5].value;
        st.WindowMenuButtonPosition = ImGuiDir_None;
        st.SeparatorTextBorderSize = 1.f;
        st.SeparatorTextAlign = ImVec2( 0.f, 0.5f );
        return;
    }
    int count = 0;
    for( const var &v : vars ) {
        ImGui::PushStyleVar( v.idx, v.value );
        ++count;
    }
    for( const var2 &v : vars2 ) {
        ImGui::PushStyleVar( v.idx, v.value );
        ++count;
    }
    style_var_stack.push_back( count );
}

} // namespace

bool theme_sets_color( int imgui_col )
{
    for( const color_slot &c : theme_colors() ) {
        if( c.idx == imgui_col ) {
            return true;
        }
    }
    return false;
}

void push()
{
    push_structure( nullptr );
}

void pop()
{
    if( !style_var_stack.empty() ) {
        const int count = style_var_stack.back();
        style_var_stack.pop_back();
        ImGui::PopStyleVar( count );
    }
}

void apply_defaults()
{
    ImGuiStyle &style = ImGui::GetStyle();
    for( const color_slot &c : theme_colors() ) {
        style.Colors[c.idx] = c.color;
    }
    push_structure( &style );
}

void draw_item_bezel( bool selected, bool hovered, bool empty )
{
    ImDrawList *dl = ImGui::GetWindowDrawList();
    if( dl == nullptr ) {
        return;
    }
    const theme::tokens &t = g_tokens;
    const ImVec2 rmin = ImGui::GetItemRectMin();
    const ImVec2 rmax = ImGui::GetItemRectMax();
    uint32_t stroke = t.edge_quiet;
    float thickness = 1.0f;
    if( selected ) {
        stroke = t.accent;
        thickness = 2.0f;
    } else if( hovered ) {
        stroke = t.edge_bronze;
        thickness = 1.5f;
    } else if( empty ) {
        stroke = IM_COL32( 0x38, 0x33, 0x2E, 180 );
    }
    const float rounding = ImGui::GetStyle().FrameRounding;
    dl->AddRect( rmin, rmax, stroke, rounding, 0, thickness );
}

int push_slot_button( bool selected, bool empty )
{
    if( selected ) {
        ImGui::PushStyleColor( ImGuiCol_Button, palette::slot_selected() );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, palette::toolbar_active_hovered() );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, palette::toolbar_active_pressed() );
        ImGui::PushStyleColor( ImGuiCol_Border, palette::border_accent() );
        return 4;
    }
    if( empty ) {
        ImGui::PushStyleColor( ImGuiCol_Button, palette::slot_empty() );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, palette::button_hovered() );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, palette::button_active() );
        ImGui::PushStyleColor( ImGuiCol_Text, palette::text_muted() );
        return 4;
    }
    ImGui::PushStyleColor( ImGuiCol_Button, palette::button() );
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, palette::button_hovered() );
    ImGui::PushStyleColor( ImGuiCol_ButtonActive, palette::button_active() );
    return 3;
}

int push_grid_button( bool selected )
{
    if( selected ) {
        ImGui::PushStyleColor( ImGuiCol_Button, palette::grid_selected() );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, palette::toolbar_active_hovered() );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, palette::toolbar_active_pressed() );
        ImGui::PushStyleColor( ImGuiCol_Border, palette::border_accent() );
        return 4;
    }
    ImGui::PushStyleColor( ImGuiCol_Button, palette::button() );
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, palette::button_hovered() );
    ImGui::PushStyleColor( ImGuiCol_ButtonActive, palette::button_active() );
    return 3;
}

int push_toolbar_button( bool active )
{
    if( active ) {
        ImGui::PushStyleColor( ImGuiCol_Button, palette::toolbar_active() );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, palette::toolbar_active_hovered() );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, palette::toolbar_active_pressed() );
        ImGui::PushStyleColor( ImGuiCol_Border, palette::border_accent() );
        return 4;
    }
    ImGui::PushStyleColor( ImGuiCol_Button, palette::button() );
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, palette::button_hovered() );
    ImGui::PushStyleColor( ImGuiCol_ButtonActive, palette::button_active() );
    return 3;
}

void section_header( const char *title )
{
    if( title == nullptr || title[0] == '\0' ) {
        return;
    }
    ImGui::Spacing();
    ImGui::TextColored( palette::accent(), "%s", title );
    ImGui::PushStyleColor( ImGuiCol_Separator, palette::separator() );
    ImGui::Separator();
    ImGui::PopStyleColor();
}

void progress_meter( float fraction, const char *overlay_text )
{
    const float f = std::clamp( fraction, 0.f, 1.f );
    ImGui::PushStyleColor( ImGuiCol_PlotHistogram, palette::accent() );
    ImGui::PushStyleColor( ImGuiCol_FrameBg, v4( g_tokens.meter_track ) );
    ImGui::ProgressBar( f, ImVec2( -1.f, 0.f ), overlay_text );
    ImGui::PopStyleColor( 2 );
}

} // namespace ui_hybrid_chrome
