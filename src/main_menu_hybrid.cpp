#include "main_menu_hybrid.h"

#if defined(TILES)

#include <algorithm>
#include <string>
#include <vector>

#include "imgui/imgui.h"

#include "get_version.h"
#include "main_menu.h"
#include "output.h"
#include "translations.h"
#include "ui_hybrid_chrome.h"
#include "ui_hybrid_widgets.h"
#include "worldfactory.h"

namespace
{

namespace w = ui_hybrid_widgets;
namespace theme = ui_hybrid_chrome::theme;

enum class opt : int {
    MOTD = 0, NEWCHAR, LOADCHAR, WORLD, TUTORIAL, SETTINGS, HELP, CREDITS, QUIT, NUM
};

/** "<N|n>ew Game" → ("New Game", "n"). Uses the same markup as shortcut_text(). */
std::pair<std::string, std::string> split_hotkey( const std::string &markup )
{
    std::string label;
    std::string key;
    size_t i = 0;
    while( i < markup.size() ) {
        if( markup[i] == '<' ) {
            const size_t bar = markup.find( '|', i );
            const size_t close = markup.find( '>', i );
            if( bar != std::string::npos && close != std::string::npos && bar < close ) {
                const std::string shown = markup.substr( i + 1, bar - i - 1 );
                label += shown;
                if( key.empty() ) {
                    key = shown;
                }
                i = close + 1;
                continue;
            }
        }
        label += markup[i];
        ++i;
    }
    return { label, key };
}

uint32_t alpha_u32( uint32_t c, float a )
{
    return ( c & 0x00FFFFFFu ) | ( static_cast<uint32_t>( a * 255.f ) << 24 );
}

bool has_drawer( int sel )
{
    const opt o = static_cast<opt>( sel );
    return o == opt::NEWCHAR || o == opt::LOADCHAR || o == opt::WORLD || o == opt::SETTINGS;
}

bool has_text( int sel )
{
    const opt o = static_cast<opt>( sel );
    return o == opt::MOTD || o == opt::CREDITS;
}

} // namespace

main_menu_overlay::main_menu_overlay( main_menu &menu_ ) :
    cataimgui::window( _( "Astral" ), ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollbar |
                       ImGuiWindowFlags_NoScrollWithMouse ),
    menu( menu_ )
{
    set_shell( 1, "world" );
}

std::string main_menu_overlay::take_action()
{
    std::string a = queued;
    queued.clear();
    return a;
}

cataimgui::bounds main_menu_overlay::get_bounds()
{
    const ImVec2 vp = ImGui::GetMainViewport()->Size;
    const ImVec2 origin = ImGui::GetMainViewport()->Pos;
    const float s = theme::scale();
    float width = 440.f * s;
    if( has_drawer( menu.sel1 ) ) {
        width += 400.f * s;
    } else if( has_text( menu.sel1 ) ) {
        width += std::min( 760.f * s, vp.x * 0.5f );
    }
    width = std::min( width, vp.x - 48.f * s );
    // Keep the title lettering in the art visible: start below the top ~28 %.
    const float top = vp.y * 0.28f;
    const float height = std::min( vp.y - top - 24.f * s, 560.f * s );
    const float x = std::min( 72.f * s, vp.x * 0.05f );
    return { origin.x + x, origin.y + top, width, height };
}

void main_menu_overlay::draw_categories( float width )
{
    const float s = theme::scale();
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    ImGui::BeginChild( "categories", ImVec2( width, 0.f ), ImGuiChildFlags_None,
                       ImGuiWindowFlags_NoScrollbar );
    // Kicker: last played world / character.
    if( !world_generator->last_world_name.empty() ) {
        std::string last = world_generator->last_world_name;
        if( !world_generator->last_character_name.empty() ) {
            last = world_generator->last_character_name + " · " + last;
        }
        ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s", _( "Last played:" ) );
        ImGui::SameLine();
        ImGui::TextUnformatted( w::fit_text( last, ImGui::GetContentRegionAvail().x ).c_str() );
        ImGui::Dummy( ImVec2( 0.f, tk.xs * s ) );
    }
    const int count = static_cast<int>( menu.vMenuItems.size() );
    const auto row = [&]( int index, bool featured ) {
        const auto [label, key] = split_hotkey( menu.vMenuItems[index] );
        w::row_state st;
        st.selected = menu.sel1 == index;
        st.featured = featured;
        const std::string id = "cat" + std::to_string( index );
        std::string detail;
        const opt o = static_cast<opt>( index );
        if( o == opt::LOADCHAR || o == opt::WORLD ) {
            const size_t n = world_generator->get_all_worlds().size();
            detail = n == 0 ? std::string() : string_format( n_gettext( "%d world", "%d worlds", n ), n );
        }
        // Detail (counts) sits left of the hotkey letter; both right-aligned.
        const std::string detail_key = detail.empty() ? key : detail + "   " + key;
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( tk.sm * s, tk.xs * s ) );
        const w::row_result r = w::selectable_row( id.c_str(), label, detail_key, nullptr, nullptr, st, 0.f,
                                has_drawer( index ) || has_text( index ) ? "chevron" : nullptr );
        ImGui::PopStyleVar();
        // hotkey letter re-drawn in the accent color over the muted detail text
        if( !key.empty() ) {
            ImDrawList *draw = ImGui::GetWindowDrawList();
            const ImVec2 ksz = ImGui::CalcTextSize( key.c_str() );
            const float kx = r.max.x - tk.md * s - ( has_drawer( index ) || has_text( index ) ? tk.lg * s +
                             tk.sm * s : 0.f ) - ksz.x;
            const float ky = r.min.y + ( r.max.y - r.min.y - ksz.y ) * 0.5f;
            draw->AddRectFilled( ImVec2( kx - 1.f, ky ), ImVec2( kx + ksz.x + 1.f, ky + ksz.y ),
                                 st.selected ? tk.selected_bg : tk.raised );
            draw->AddText( ImVec2( kx, ky ), st.selected ? tk.accent : tk.accent_dim, key.c_str() );
        }
        if( r.clicked ) {
            if( menu.sel1 != index ) {
                menu.sel1 = index;
                menu.sel2 = index == static_cast<int>( opt::LOADCHAR ) ? static_cast<int>( menu.last_world_pos ) : 0;
                menu.sel_line = 0;
                menu.on_move();
                mark_resized();
            }
            if( o == opt::HELP || o == opt::TUTORIAL || o == opt::QUIT ) {
                queued = o == opt::QUIT ? "QUIT" : "CONFIRM";
            }
        }
    };
    // Featured: New Game, Load, World. Plain: Tutorial. Demoted: MOTD, Settings, Help, Credits, Quit.
    for( int i : { static_cast<int>( opt::NEWCHAR ), static_cast<int>( opt::LOADCHAR ), static_cast<int>( opt::WORLD ) } ) {
        if( i < count ) {
            row( i, true );
        }
    }
    if( static_cast<int>( opt::TUTORIAL ) < count ) {
        row( static_cast<int>( opt::TUTORIAL ), false );
    }
    ImGui::Dummy( ImVec2( 0.f, tk.sm * s ) );
    ImGui::Separator();
    ImGui::Dummy( ImVec2( 0.f, tk.sm * s ) );
    for( int i : { static_cast<int>( opt::MOTD ), static_cast<int>( opt::SETTINGS ), static_cast<int>( opt::HELP ),
                   static_cast<int>( opt::CREDITS ) } ) {
        if( i < count ) {
            row( i, false );
        }
    }
    if( static_cast<int>( opt::QUIT ) < count ) {
        ImGui::Dummy( ImVec2( 0.f, tk.sm * s ) );
        row( static_cast<int>( opt::QUIT ), false );
    }
    ImGui::EndChild();
}

void main_menu_overlay::draw_text_panel( const std::string &text, float width )
{
    const float s = theme::scale();
    if( w::panel_begin( "textpanel", ImVec2( width, 0.f ), true ) ) {
        // Keyboard scrolling from the existing UP/DOWN handling (sel_line).
        if( menu.sel_line != last_text_line ) {
            ImGui::SetScrollY( menu.sel_line * ImGui::GetTextLineHeightWithSpacing() );
            last_text_line = menu.sel_line;
        }
        ImGui::PushTextWrapPos( ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 4.f * s );
        cataimgui::draw_colored_text( text, ImGui::GetContentRegionAvail().x - 4.f * s );
        ImGui::PopTextWrapPos();
        w::panel_end();
    }
}

void main_menu_overlay::draw_drawer( float width )
{
    const float s = theme::scale();
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    const opt o = static_cast<opt>( menu.sel1 );
    std::vector<std::pair<std::string, std::string>> entries; // label, hotkey
    std::vector<std::string> details;
    if( o == opt::NEWCHAR ) {
        for( const std::string &item : menu.vNewGameSubItems ) {
            entries.push_back( split_hotkey( item ) );
            details.emplace_back();
        }
    } else if( o == opt::SETTINGS ) {
        for( const std::string &item : menu.vSettingsSubItems ) {
            entries.push_back( split_hotkey( item ) );
            details.emplace_back();
        }
    } else if( o == opt::LOADCHAR || o == opt::WORLD ) {
        if( o == opt::WORLD ) {
            entries.emplace_back( _( "Create World" ), "" );
            details.emplace_back();
        }
        for( const auto &kv : world_generator->get_all_worlds() ) {
            entries.emplace_back( kv.first, "" );
            const size_t n = kv.second->world_saves.size();
            details.push_back( n == 0 ? std::string( _( "no characters" ) ) :
                               string_format( n_gettext( "%d character", "%d characters", n ), n ) );
        }
    }
    const char *heading = o == opt::NEWCHAR ? _( "New game" ) : o == opt::SETTINGS ? _( "Settings" ) :
                          o == opt::LOADCHAR ? _( "Load" ) : _( "Worlds" );
    ImGui::BeginChild( "drawer", ImVec2( width, 0.f ), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar );
    w::section_label( heading );
    if( entries.empty() ) {
        w::empty_state( _( "No worlds yet" ), _( "Start a new game to create one." ) );
        ImGui::EndChild();
        return;
    }
    if( o == opt::NEWCHAR && menu.sel2 >= 0 && static_cast<size_t>( menu.sel2 ) < menu.vNewGameHints.size() ) {
        ImGui::PushTextWrapPos( ImGui::GetCursorPosX() + width - tk.md * s );
        ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s", menu.vNewGameHints[menu.sel2].c_str() );
        ImGui::PopTextWrapPos();
        ImGui::Dummy( ImVec2( 0.f, tk.xs * s ) );
    }
    ImGui::BeginChild( "drawer_list", ImVec2( 0.f, 0.f ), ImGuiChildFlags_None );
    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( tk.sm * s, tk.xs * s ) );
    for( int i = 0; i < static_cast<int>( entries.size() ); ++i ) {
        w::row_state st;
        st.selected = menu.sel2 == i;
        const std::string id = "sub" + std::to_string( i );
        const w::row_result r = w::selectable_row( id.c_str(), entries[i].first, details[i], nullptr, nullptr, st );
        if( !entries[i].second.empty() ) {
            ImDrawList *draw = ImGui::GetWindowDrawList();
            const ImVec2 ksz = ImGui::CalcTextSize( entries[i].second.c_str() );
            draw->AddText( ImVec2( r.max.x - tk.md * s - ksz.x, r.min.y + ( r.max.y - r.min.y - ksz.y ) * 0.5f ),
                           st.selected ? tk.accent : tk.accent_dim, entries[i].second.c_str() );
        }
        if( st.selected && menu.sel1 != last_sel1 ) {
            ImGui::SetScrollHereY();
        }
        if( r.clicked || r.double_clicked ) {
            if( menu.sel2 != i ) {
                menu.on_move();
            }
            menu.sel2 = i;
            menu.sel_line = 0;
            queued = "CONFIRM";
        }
        if( r.right_clicked && o == opt::LOADCHAR ) {
            menu.sel2 = i;
            queued = "CONFIRM";
        }
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::EndChild();
}

void main_menu_overlay::draw_controls()
{
    const float s = theme::scale();
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    const float total = ImGui::GetContentRegionAvail().x;
    const bool drawer = has_drawer( menu.sel1 );
    const bool text = has_text( menu.sel1 );
    const float cat_w = ( drawer || text ) ? std::min( 400.f * s, total * 0.42f ) : total;
    draw_categories( cat_w );
    if( drawer || text ) {
        ImGui::SameLine( 0.f, tk.lg * s );
        const float rest = ImGui::GetContentRegionAvail().x;
        if( drawer ) {
            draw_drawer( rest );
        } else {
            draw_text_panel( menu.sel1 == static_cast<int>( opt::MOTD ) ? menu.mmenu_motd : menu.mmenu_credits,
                             rest );
        }
    }
    // Version, muted, bottom-right inside the frame.
    {
        ImDrawList *draw = ImGui::GetWindowDrawList();
        const std::string ver = w::fit_text( getVersionString(), total * 0.5f );
        const ImVec2 sz = ImGui::CalcTextSize( ver.c_str() );
        const ImVec2 wmax( ImGui::GetWindowPos().x + ImGui::GetWindowSize().x,
                           ImGui::GetWindowPos().y + ImGui::GetWindowSize().y );
        draw->AddText( ImVec2( wmax.x - sz.x - 18.f * s, wmax.y - sz.y - 12.f * s ),
                       alpha_u32( tk.text_muted, 0.8f ), ver.c_str() );
    }
    if( menu.sel1 != last_sel1 ) {
        // Drawer/text panel presence changed: resize on the next frame.
        mark_resized();
    }
    last_sel1 = menu.sel1;
}

#endif // TILES
