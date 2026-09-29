#include "ui_hybrid_showcase.h"

#if defined(TILES)

#include <array>
#include <cstdlib>
#include <string>

#include "imgui/imgui.h"

#include "cata_imgui.h"
#include "input_context.h"
#include "translations.h"
#include "ui_hybrid_chrome.h"
#include "ui_hybrid_textures.h"
#include "ui_hybrid_widgets.h"
#include "ui_manager.h"

namespace
{

namespace w = ui_hybrid_widgets;
namespace theme = ui_hybrid_chrome::theme;

class showcase_window : public cataimgui::window
{
    public:
        showcase_window() : cataimgui::window( "Astral UI showcase",
                                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove ),
            ctxt( "HELP_KEYBINDINGS" ) {
            set_shell( 0, "gear" );
            ctxt.register_action( "QUIT" );
            ctxt.register_action( "SELECT" );
            ctxt.register_action( "FILTER" );
            ctxt.register_action( "HELP_KEYBINDINGS" );
            ctxt.register_action( "ANY_INPUT" );
        }
        void run();
        bool narrow = false;
        int level_override = -1;
        int page = 0;
        int selected_row = 1;
        int focused_row = 2;
        float meter_value = 0.72f;
        bool show_scrim = false;
        char field[96] = "";
    protected:
        cataimgui::bounds get_bounds() override {
            const ImVec2 vp = ImGui::GetMainViewport()->Size;
            const ImVec2 origin = ImGui::GetMainViewport()->Pos;
            const float s = theme::scale();
            const float width = narrow ? std::min( vp.x * 0.96f, 640.f * s ) : std::min( vp.x * 0.94f, 1500.f * s );
            const float height = std::min( vp.y * 0.94f, 1000.f * s );
            return { origin.x + ( vp.x - width ) * 0.5f, origin.y + ( vp.y - height ) * 0.5f, width, height };
        }
        void draw_controls() override;
    private:
        input_context ctxt;
        void page_components();
        void page_rows();
        void page_dialogs();
};

void showcase_window::page_components()
{
    w::section_label( _( "Buttons" ), "gear" );
    w::action_button( "Primary", w::button_kind::primary );
    ImGui::SameLine();
    w::action_button( "Secondary", w::button_kind::secondary );
    ImGui::SameLine();
    w::action_button( "Tertiary", w::button_kind::tertiary );
    ImGui::SameLine();
    w::action_button( "Delete world", w::button_kind::danger );
    ImGui::SameLine();
    w::action_button( "Disabled", w::button_kind::primary, ImVec2( 0, 0 ), false,
                      "Select an item first." );
    ImGui::SameLine();
    w::action_button( "Disabled", w::button_kind::secondary, ImVec2( 0, 0 ), false,
                      "Nothing is worn on this slot." );
    w::action_button( "A very long primary action label that should still fit inside its button",
                      w::button_kind::primary );
    ImGui::SameLine();
    w::action_button( "With icon", w::button_kind::secondary, ImVec2( 0, 0 ), true, nullptr, "search" );
    ImGui::SameLine();
    w::icon_button( "ib1", "grid", 42.f, false, "Grid view" );
    ImGui::SameLine();
    w::icon_button( "ib2", "list", 42.f, true, "List view (active)" );
    ImGui::SameLine();
    w::icon_button( "ib3", "nearby", 42.f, false, "Disabled", false );
    ImGui::SameLine();
    w::close_button( "demo" );

    w::section_label( _( "Tabs, fields, hints" ), "tab_inventory" );
    w::tab( "Equipment", page == 0, "tab_equipment" );
    ImGui::SameLine( 0.f, 2.f );
    w::tab( "Inventory", false, "tab_inventory" );
    ImGui::SameLine( 0.f, 2.f );
    w::tab( "A tab with a much longer translated label", false );
    ImGui::SetNextItemWidth( 320.f * theme::scale() );
    ImGui::InputTextWithHint( "##field", _( "Find carried items…" ), field, sizeof( field ) );
    ImGui::SameLine();
    w::hint( ctxt, "FILTER", _( "filter" ) );
    ImGui::SameLine();
    w::hint( ctxt, "SELECT", _( "select" ), true );
    ImGui::SameLine();
    w::hint( ctxt, "QUIT", _( "close" ) );
    ImGui::SameLine();
    w::hint_key( "Esc", _( "back (popup)" ) );

    w::section_label( _( "Meters" ), "hp" );
    const float s = theme::scale();
    ImGui::SliderFloat( "##mv", &meter_value, 0.f, 1.f, "value %.2f" );
    w::meter( "m1", meter_value, w::meter_kind::health, string_format( "Health %d%%",
              static_cast<int>( meter_value * 100 ) ), 260.f );
    ImGui::SameLine();
    w::meter( "m2", meter_value, w::meter_kind::stamina, "Stamina", 200.f );
    ImGui::SameLine();
    w::meter( "m3", 0.55f, w::meter_kind::morale, "+12 morale", 160.f );
    ImGui::SameLine();
    w::meter( "m4", 0.9f, w::meter_kind::danger, "Pain 90", 120.f );
    ImGui::SameLine();
    w::meter( "m5", 0.0f, w::meter_kind::neutral, "0%", 100.f );

    w::section_label( _( "Icons (atlas, tinted)" ), "star" );
    static const std::array<const char *, 20> names = {{
            "cat_head", "cat_body", "cat_arms", "cat_hands", "cat_waist", "cat_legs", "cat_feet", "cat_back",
            "cat_main_hand", "cat_off_hand", "cat_other", "search", "filter", "link", "warning", "info",
            "attack", "guard", "evade", "missing_icon_name"
        }
    };
    for( size_t i = 0; i < names.size(); ++i ) {
        w::icon( names[i], 28.f, ui_hybrid_chrome::theme::get().accent );
        if( i + 1 < names.size() ) {
            ImGui::SameLine( 0.f, 6.f * s );
        }
    }
    ImGui::TextDisabled( "%s", _( "Last cell: missing icon name → dashed fallback." ) );

    w::section_label( _( "Cards and panels" ), "cat_other" );
    if( w::card_begin( "card1", ImVec2( 260.f * s, 90.f * s ), true ) ) {
        ImGui::TextDisabled( "%s", _( "Main hand" ) );
        w::push_font_section();
        ImGui::TextUnformatted( "longsword" );
        w::pop_font();
        w::card_end();
    }
    ImGui::SameLine();
    if( w::card_begin( "card2", ImVec2( 260.f * s, 90.f * s ), false ) ) {
        ImGui::TextDisabled( "%s", _( "Off hand" ) );
        w::push_font_section();
        ImGui::TextDisabled( "%s", _( "Uses both hands" ) );
        w::pop_font();
        w::card_end();
    }
    ImGui::SameLine();
    if( w::panel_begin( "panel1", ImVec2( 300.f * s, 90.f * s ) ) ) {
        w::empty_state( _( "No results" ), _( "Try a different filter." ) );
        w::panel_end();
    }
}

void showcase_window::page_rows()
{
    w::section_label( _( "Rows: normal, hover, selected, focused, disabled, long" ), "list" );
    const std::array<std::string, 6> labels = {{
            "Head", "Body", "Arms", "Hands (disabled row)", "Waist",
            "A very long row label that must ellipsize before it reaches the count on the right side"
        }
    };
    const std::array<const char *, 6> icons = {{ "cat_head", "cat_body", "cat_arms", "cat_hands", "cat_waist", "cat_legs" }};
    for( int i = 0; i < 6; ++i ) {
        w::row_state st;
        st.selected = selected_row == i;
        st.focused = focused_row == i;
        st.disabled = i == 3;
        st.featured = true;
        const w::row_result r = w::tree_row( ( "row" + std::to_string( i ) ).c_str(), labels[i],
                                             i == 0 ? _( "No items" ) : string_format( "%d items", i * 2 ),
                                             icons[i], nullptr, 0, true, i == 1, st );
        if( r.clicked ) {
            selected_row = i;
        }
        if( i == 1 ) {
            w::row_state sub;
            w::tree_row( "sub1", "Neck", _( "No items" ), "cat_neck", nullptr, 1, true, false, sub );
            sub.selected = true;
            w::tree_row( "sub2", "Torso", "8 items", "cat_body", nullptr, 1, true, true, sub );
            w::row_state leaf;
            leaf.focused = true;
            w::tree_row( "leaf1", "cyan t-shirt", "Torso, Arms", "link", nullptr, 3, false, false, leaf );
            w::row_state leaf2;
            w::tree_row( "leaf2", "brown t-shirt", "Torso, Arms", "link", nullptr, 3, false, false, leaf2 );
        }
    }
    w::section_label( _( "Plain list rows with custom painter" ) );
    for( int i = 0; i < 4; ++i ) {
        w::row_state st;
        st.selected = i == 1;
        const w::icon_painter painter = []( ImDrawList * d, const ImVec2 & a, const ImVec2 & b ) {
            d->AddRectFilled( a, b, IM_COL32( 90, 120, 160, 255 ), 3.f );
            d->AddRect( a, b, IM_COL32( 20, 30, 40, 255 ), 3.f );
        };
        w::selectable_row( ( "list" + std::to_string( i ) ).c_str(),
                           string_format( "inventory item %d", i ), string_format( "×%d", ( i + 1 ) * 17 ),
                           nullptr, painter, st, 36.f );
    }
}

void showcase_window::page_dialogs()
{
    w::section_label( _( "Nested popups, tooltips, scrim" ), "info" );
    if( w::action_button( "Open popup", w::button_kind::secondary ) ) {
        ImGui::OpenPopup( "showcase_popup" );
    }
    ImGui::SameLine();
    w::action_button( "Hover me for a tooltip", w::button_kind::tertiary );
    w::tooltip( "Tooltips wrap at 480 px and use the light popup frame. They are never the only route to information: everything shown here is also in the footer or the row itself." );
    ImGui::SameLine();
    ImGui::Checkbox( _( "Scrim behind window" ), &show_scrim );
    if( ImGui::BeginPopup( "showcase_popup" ) ) {
        ImGui::TextUnformatted( _( "Level 1 popup" ) );
        if( ImGui::MenuItem( _( "Wield" ) ) ) {}
        if( ImGui::MenuItem( _( "Use" ) ) ) {}
        if( ImGui::BeginMenu( _( "More…" ) ) ) {
            ImGui::MenuItem( _( "Clothing layers…" ) );
            ImGui::MenuItem( _( "Classic inventory…" ) );
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }
    w::section_label( _( "Long text and non-Latin glyphs" ), "world" );
    ImGui::TextWrapped( "%s",
                        "Die Ausrüstungsänderung wurde abgebrochen. Équipement prêt. Экипировка готова. Ελληνικά. 装备已准备好。 装備の準備ができました。 Large numbers: 1,234,567 × 3.4k. Empty: —." );
    w::section_label( _( "Decoration level" ), "gear" );
    ImGui::RadioButton( "Option", &level_override, -1 );
    ImGui::SameLine();
    ImGui::RadioButton( "Full", &level_override, 0 );
    ImGui::SameLine();
    ImGui::RadioButton( "Reduced", &level_override, 1 );
    ImGui::SameLine();
    ImGui::RadioButton( "None", &level_override, 2 );
    theme::override_level( level_override );
    ImGui::TextDisabled( "textures: %d live, %d created since start, %.2f MiB decoded",
                         ui_hybrid_textures::live_texture_count(), ui_hybrid_textures::texture_creations(),
                         ui_hybrid_textures::decoded_bytes() / 1048576.0 );
    ImGui::TextDisabled( "ui scale %.2f, font %.0f px", theme::scale(), ImGui::GetFontSize() );
}

void showcase_window::draw_controls()
{
    if( show_scrim ) {
        w::scrim();
    }
    if( w::tab( _( "Components" ), page == 0 ) ) {
        page = 0;
    }
    ImGui::SameLine( 0.f, 2.f );
    if( w::tab( _( "Rows and trees" ), page == 1 ) ) {
        page = 1;
    }
    ImGui::SameLine( 0.f, 2.f );
    if( w::tab( _( "Dialogs and text" ), page == 2 ) ) {
        page = 2;
    }
    ImGui::SameLine();
    if( w::action_button( narrow ? "Wide layout" : "Narrow layout", w::button_kind::tertiary ) ) {
        narrow = !narrow;
        mark_resized();
    }
    if( w::body_begin( "showcase_body", 64.f ) ) {
        switch( page ) {
            case 0:
                page_components();
                break;
            case 1:
                page_rows();
                break;
            default:
                page_dialogs();
                break;
        }
    }
    w::body_end();
    if( w::footer_begin( "showcase_footer" ) ) {
        w::hint( ctxt, "QUIT", _( "close" ) );
        ImGui::SameLine();
        ImGui::TextDisabled( "%s", _( "Footer stays fixed while the body scrolls." ) );
        ImGui::SameLine();
        const float s = theme::scale();
        w::footer_align_right( 2.f * 130.f * s + 8.f * s );
        w::action_button( "Secondary", w::button_kind::secondary, ImVec2( 130.f, 0 ) );
        ImGui::SameLine();
        if( w::action_button( "Close", w::button_kind::primary, ImVec2( 130.f, 0 ) ) ) {
            is_open = false;
        }
    }
    w::footer_end();
}

void showcase_window::run()
{
    while( is_open ) {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input( 5 );
        if( action == "QUIT" ) {
            is_open = false;
        }
    }
    theme::override_level( -1 );
}

} // namespace

namespace ui_hybrid_showcase
{
void show()
{
    showcase_window win;
    win.run();
}
} // namespace ui_hybrid_showcase

#else

namespace ui_hybrid_showcase
{
void show() {}
} // namespace ui_hybrid_showcase

#endif
