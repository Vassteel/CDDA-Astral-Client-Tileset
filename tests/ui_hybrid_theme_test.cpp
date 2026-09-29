#include <cmath>
#include <string>

#include "cata_catch.h"
#include "ui_hybrid_chrome.h"
#include "ui_hybrid_textures.h"

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
