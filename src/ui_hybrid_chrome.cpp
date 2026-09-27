#include "ui_hybrid_chrome.h"

#include "imgui/imgui.h"

namespace ui_hybrid_chrome
{
namespace palette
{

// Warm dark wood / bronze panels (BG3-mock Hybrid): charcoal base + bronze grit.
ImVec4 window_bg()
{
    return ImVec4( 0.09f, 0.07f, 0.05f, 0.97f );
}
ImVec4 child_bg()
{
    return ImVec4( 0.12f, 0.10f, 0.07f, 0.94f );
}
ImVec4 popup_bg()
{
    return ImVec4( 0.11f, 0.09f, 0.06f, 0.98f );
}
ImVec4 border()
{
    return ImVec4( 0.36f, 0.28f, 0.16f, 0.88f ); // bronze edge
}
ImVec4 border_accent()
{
    return ImVec4( 0.78f, 0.58f, 0.28f, 0.95f ); // amber-bronze
}
ImVec4 button()
{
    return ImVec4( 0.17f, 0.14f, 0.10f, 1.00f );
}
ImVec4 button_hovered()
{
    return ImVec4( 0.24f, 0.19f, 0.12f, 1.00f );
}
ImVec4 button_active()
{
    return ImVec4( 0.34f, 0.26f, 0.12f, 1.00f );
}
ImVec4 header()
{
    return ImVec4( 0.20f, 0.15f, 0.09f, 1.00f );
}
ImVec4 header_hovered()
{
    return ImVec4( 0.28f, 0.21f, 0.11f, 1.00f );
}
ImVec4 text()
{
    return ImVec4( 0.92f, 0.86f, 0.74f, 1.00f ); // warm parchment-white
}
ImVec4 text_muted()
{
    return ImVec4( 0.58f, 0.50f, 0.38f, 1.00f );
}
ImVec4 accent()
{
    return ImVec4( 0.82f, 0.62f, 0.30f, 1.00f );
}
ImVec4 accent_dim()
{
    return ImVec4( 0.48f, 0.36f, 0.16f, 0.85f );
}
ImVec4 separator()
{
    return ImVec4( 0.40f, 0.30f, 0.16f, 0.72f );
}
ImVec4 title_bg()
{
    return ImVec4( 0.10f, 0.08f, 0.05f, 1.00f );
}
ImVec4 scrollbar_grab()
{
    return ImVec4( 0.40f, 0.30f, 0.16f, 0.82f );
}
ImVec4 slot_empty()
{
    return ImVec4( 0.14f, 0.11f, 0.08f, 1.00f );
}
ImVec4 slot_selected()
{
    return ImVec4( 0.32f, 0.24f, 0.10f, 1.00f );
}
ImVec4 grid_selected()
{
    return ImVec4( 0.30f, 0.23f, 0.10f, 1.00f );
}
ImVec4 toolbar_active()
{
    return ImVec4( 0.34f, 0.26f, 0.10f, 1.00f );
}
ImVec4 toolbar_active_hovered()
{
    return ImVec4( 0.44f, 0.34f, 0.14f, 1.00f );
}
ImVec4 toolbar_active_pressed()
{
    return ImVec4( 0.24f, 0.18f, 0.08f, 1.00f );
}

} // namespace palette

namespace
{

thread_local int g_push_color_count = 0;
thread_local int g_push_var_count = 0;

} // namespace

void push()
{
    g_push_color_count = 0;
    g_push_var_count = 0;

    auto push_col = [&]( ImGuiCol idx, const ImVec4 & c ) {
        ImGui::PushStyleColor( idx, c );
        ++g_push_color_count;
    };

    push_col( ImGuiCol_Text, palette::text() );
    push_col( ImGuiCol_TextDisabled, palette::text_muted() );
    push_col( ImGuiCol_WindowBg, palette::window_bg() );
    push_col( ImGuiCol_ChildBg, palette::child_bg() );
    push_col( ImGuiCol_PopupBg, palette::popup_bg() );
    push_col( ImGuiCol_Border, palette::border() );
    push_col( ImGuiCol_FrameBg, palette::button() );
    push_col( ImGuiCol_FrameBgHovered, palette::button_hovered() );
    push_col( ImGuiCol_FrameBgActive, palette::button_active() );
    push_col( ImGuiCol_TitleBg, palette::title_bg() );
    push_col( ImGuiCol_TitleBgActive, palette::header() );
    push_col( ImGuiCol_TitleBgCollapsed, palette::title_bg() );
    push_col( ImGuiCol_ScrollbarBg, palette::window_bg() );
    push_col( ImGuiCol_ScrollbarGrab, palette::scrollbar_grab() );
    push_col( ImGuiCol_ScrollbarGrabHovered, palette::accent_dim() );
    push_col( ImGuiCol_ScrollbarGrabActive, palette::accent() );
    push_col( ImGuiCol_CheckMark, palette::accent() );
    push_col( ImGuiCol_SliderGrab, palette::accent_dim() );
    push_col( ImGuiCol_SliderGrabActive, palette::accent() );
    push_col( ImGuiCol_Button, palette::button() );
    push_col( ImGuiCol_ButtonHovered, palette::button_hovered() );
    push_col( ImGuiCol_ButtonActive, palette::button_active() );
    push_col( ImGuiCol_Header, palette::header() );
    push_col( ImGuiCol_HeaderHovered, palette::header_hovered() );
    push_col( ImGuiCol_HeaderActive, palette::slot_selected() );
    push_col( ImGuiCol_Separator, palette::separator() );
    push_col( ImGuiCol_SeparatorHovered, palette::accent_dim() );
    push_col( ImGuiCol_SeparatorActive, palette::accent() );
    push_col( ImGuiCol_Tab, palette::button() );
    push_col( ImGuiCol_TabHovered, palette::header_hovered() );
    push_col( ImGuiCol_TabSelected, palette::slot_selected() );
    push_col( ImGuiCol_TextSelectedBg, ImVec4( 0.45f, 0.35f, 0.15f, 0.45f ) );
    // ProgressBar / plot fill (sidebar meters, HP, carry weight, etc.)
    push_col( ImGuiCol_PlotHistogram, palette::accent() );
    push_col( ImGuiCol_PlotHistogramHovered, palette::border_accent() );

    ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 4.f );
    ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 3.f );
    ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 3.f );
    ImGui::PushStyleVar( ImGuiStyleVar_PopupRounding, 3.f );
    ImGui::PushStyleVar( ImGuiStyleVar_ScrollbarRounding, 3.f );
    ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 1.f );
    ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 1.f );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1.f );
    g_push_var_count = 8;
}

void pop()
{
    if( g_push_var_count > 0 ) {
        ImGui::PopStyleVar( g_push_var_count );
        g_push_var_count = 0;
    }
    if( g_push_color_count > 0 ) {
        ImGui::PopStyleColor( g_push_color_count );
        g_push_color_count = 0;
    }
}

void draw_item_bezel( bool selected, bool hovered, bool empty )
{
    ImDrawList *dl = ImGui::GetWindowDrawList();
    if( dl == nullptr ) {
        return;
    }
    const ImVec2 rmin = ImGui::GetItemRectMin();
    const ImVec2 rmax = ImGui::GetItemRectMax();
    ImVec4 stroke = palette::border();
    float thickness = 1.0f;
    if( selected ) {
        stroke = palette::border_accent();
        thickness = 2.0f;
    } else if( hovered ) {
        stroke = palette::accent_dim();
        thickness = 1.5f;
    } else if( empty ) {
        stroke = ImVec4( 0.22f, 0.20f, 0.18f, 0.70f );
    }
    const float rounding = ImGui::GetStyle().FrameRounding;
    dl->AddRect( rmin, rmax, ImGui::ColorConvertFloat4ToU32( stroke ), rounding, 0,
                 thickness );
    // Subtle inner grit line on selected cells (reads as a metal bezel without textures).
    if( selected ) {
        const ImVec2 inner_min( rmin.x + 1.5f, rmin.y + 1.5f );
        const ImVec2 inner_max( rmax.x - 1.5f, rmax.y - 1.5f );
        if( inner_max.x > inner_min.x && inner_max.y > inner_min.y ) {
            ImVec4 inner = palette::accent_dim();
            inner.w = 0.35f;
            dl->AddRect( inner_min, inner_max, ImGui::ColorConvertFloat4ToU32( inner ),
                         rounding, 0, 1.0f );
        }
    }
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
        return 3;
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
    float f = fraction;
    if( f < 0.f ) {
        f = 0.f;
    } else if( f > 1.f ) {
        f = 1.f;
    }
    ImGui::PushStyleColor( ImGuiCol_PlotHistogram, palette::accent() );
    ImGui::PushStyleColor( ImGuiCol_FrameBg, palette::button() );
    ImGui::ProgressBar( f, ImVec2( -1.f, 0.f ), overlay_text );
    ImGui::PopStyleColor( 2 );
}

} // namespace ui_hybrid_chrome
