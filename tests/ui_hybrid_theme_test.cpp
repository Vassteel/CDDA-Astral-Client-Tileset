#include <cmath>
#include <string>

#include "cata_catch.h"
#include "ui_hybrid_chrome.h"
#include "ui_hybrid_textures.h"

#if defined(TILES)
#include "cata_scope_helpers.h"
#include "imgui/imgui.h"
#include "ui_hybrid_widgets.h"

TEST_CASE( "astral_auto_sized_shell_does_not_grow_or_retain_stale_width", "[ui][popup]" )
{
    ImGuiContext *previous = ImGui::GetCurrentContext();
    ImGuiContext *context = ImGui::CreateContext();
    on_out_of_scope cleanup( [&]() {
        ui_hybrid_chrome::theme::override_level( -1 );
        ImGui::DestroyContext( context );
        ImGui::SetCurrentContext( previous );
    } );
    ui_hybrid_chrome::theme::override_level( 2 );
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2( 3840, 2160 );
    io.DeltaTime = 1.f / 60.f;
    ImFontConfig font;
    font.SizePixels = GENERATE( 16.f, 24.f );
    io.Fonts->AddFontDefault( &font );
    io.Fonts->AddFontDefault( &font );
    font.SizePixels *= 1.5f;
    io.Fonts->AddFontDefault( &font );
    io.Fonts->Build();
    const auto kind = GENERATE( ui_hybrid_widgets::frame_kind::dialog,
                               ui_hybrid_widgets::frame_kind::large,
                               ui_hybrid_widgets::frame_kind::popup );
    const float inset = ( kind == ui_hybrid_widgets::frame_kind::large ? 20.f :
                          kind == ui_hybrid_widgets::frame_kind::dialog ? 14.f : 10.f ) *
                        ( font.SizePixels / 1.5f / 16.f );
    float settled_width = 0.f;
    for( int frame = 0; frame < 120; ++frame ) {
        ImGui::NewFrame();
        ImGui::SetNextWindowSize( ImVec2( 1400.f, 200.f ), ImGuiCond_Once );
        ImGui::SetNextWindowSizeConstraints( ImVec2( 500.f, 0.f ), ImVec2( 3500.f, 1000.f ) );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( inset, inset ) );
        ImGui::Begin( "stat-popup-regression", nullptr,
                      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar |
                      ImGuiWindowFlags_NoSavedSettings );
        ui_hybrid_widgets::window_shell( "Set new strength (between 4 and 20):", kind );
        int value = 8;
        ImGui::SetNextItemWidth( 160.f );
        ImGui::InputInt( "##stat", &value );
        ImGui::Button( "Apply" );
        ImGui::SameLine();
        ImGui::Button( "Cancel" );
        const float width = ImGui::GetWindowWidth();
        if( frame == 8 ) {
            settled_width = width;
            CHECK( width == Approx( 500.f ) );
        } else if( frame > 8 ) {
            CHECK( width == Approx( settled_width ) );
        }
        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::Render();
    }
}
#endif

// Shared Astral UI foundation: pure logic that does not need a renderer.

static float srgb_to_linear( float c )
{
    return c <= 0.03928f ? c / 12.92f : std::pow( ( c + 0.055f ) / 1.055f, 2.4f );
}

static float luminance( uint32_t col )
{
    const float r = ( col & 0xFF ) / 255.f;
    const float g = ( ( col >> 8 ) & 0xFF ) / 255.f;
    const float b = ( ( col >> 16 ) & 0xFF ) / 255.f;
    return 0.2126f * srgb_to_linear( r ) + 0.7152f * srgb_to_linear( g ) + 0.0722f * srgb_to_linear( b );
}

static float contrast( uint32_t a, uint32_t b )
{
    const float la = luminance( a );
    const float lb = luminance( b );
    return ( std::max( la, lb ) + 0.05f ) / ( std::min( la, lb ) + 0.05f );
}

TEST_CASE( "astral_theme_tokens_contrast", "[ui][astral]" )
{
    const ui_hybrid_chrome::theme::tokens &t = ui_hybrid_chrome::theme::get();
    // WCAG AA for body text on every reading surface.
    CHECK( contrast( t.text, t.surface ) >= 4.5f );
    CHECK( contrast( t.text, t.raised ) >= 4.5f );
    CHECK( contrast( t.text, t.selected_bg ) >= 4.5f );
    CHECK( contrast( t.text_muted, t.surface ) >= 4.5f );
    CHECK( contrast( t.text_muted, t.raised ) >= 4.5f );
    CHECK( contrast( t.text_on_accent, t.accent ) >= 4.5f );
    // Essential boundaries at 3:1.
    CHECK( contrast( t.edge_bronze, t.surface ) >= 3.0f );
    CHECK( contrast( t.accent, t.surface ) >= 3.0f );
    CHECK( contrast( t.danger, t.surface ) >= 3.0f );
    // Spacing ladder and control sizes are the documented values.
    CHECK( t.xs == 4.f );
    CHECK( t.xl == 24.f );
    CHECK( t.row < t.row_featured );
    CHECK( t.button >= 40.f );
}

TEST_CASE( "astral_nine_slice_geometry", "[ui][astral]" )
{
    using ui_hybrid_textures::compute_nine_slice;
    // 192 px source, 48 px margin at 2x authoring -> 24 logical px at scale 1.
    const auto g = compute_nine_slice( 100.f, 200.f, 500.f, 400.f, 48.f, 0.5f, 192, 192 );
    CHECK( g.margin_px == Approx( 24.f ) );
    CHECK( g.xs[0] == 100.f );
    CHECK( g.xs[1] == Approx( 124.f ) );
    CHECK( g.xs[2] == Approx( 476.f ) );
    CHECK( g.xs[3] == 500.f );
    CHECK( g.us[1] == Approx( 0.25f ) );
    CHECK( g.vs[2] == Approx( 0.75f ) );
    // A rect smaller than two margins clamps the margin so corners never overlap.
    const auto small = compute_nine_slice( 0.f, 0.f, 20.f, 30.f, 48.f, 0.5f, 192, 192 );
    CHECK( small.margin_px == Approx( 10.f ) );
    CHECK( small.xs[1] <= small.xs[2] );
    CHECK( small.ys[1] <= small.ys[2] );
    // UI scale 2 doubles the screen margin, never the UVs.
    const auto scaled = compute_nine_slice( 0.f, 0.f, 800.f, 600.f, 48.f, 1.0f, 192, 192 );
    CHECK( scaled.margin_px == Approx( 48.f ) );
    CHECK( scaled.us[1] == Approx( 0.25f ) );
}

TEST_CASE( "astral_decoration_level_override", "[ui][astral]" )
{
    using ui_hybrid_chrome::decoration;
    namespace theme = ui_hybrid_chrome::theme;
    theme::override_level( 2 );
    CHECK( theme::level() == decoration::none );
    theme::override_level( 1 );
    CHECK( theme::level() == decoration::reduced );
    theme::override_level( -1 );
    CHECK( theme::level() == decoration::full );
}
