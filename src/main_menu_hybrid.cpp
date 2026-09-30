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

const char *category_icon( opt o )
{
    switch( o ) {
        case opt::NEWCHAR:
            return "person";
        case opt::LOADCHAR:
            return "save";
        case opt::WORLD:
            return "world";
        case opt::TUTORIAL:
            return "info";
        case opt::SETTINGS:
            return "gear";
        case opt::MOTD:
            return "log";
        case opt::HELP:
            return "info";
        case opt::CREDITS:
            return "star";
        case opt::QUIT:
            return "close";
        default:
            return nullptr;
    }
}

} // namespace

main_menu_overlay::main_menu_overlay( main_menu &menu_ ) :
    // Untitled: no Astral shell of its own; the bar strip and the popup draw their frames.
    cataimgui::window( "", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav |
                       ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ),
    menu( menu_ )
{
}

std::string main_menu_overlay::take_action()
{
    std::string a = queued;
    queued.clear();
    return a;
}

bool main_menu_overlay::close_popup()
{
    if( !popup_open ) {
        return false;
    }
    popup_open = false;
    return true;
}

void main_menu_overlay::category_selected()
{
    popup_open = has_drawer( menu.sel1 ) || has_text( menu.sel1 );
}

cataimgui::bounds main_menu_overlay::get_bounds()
{
    // The window covers the whole viewport (bar at the bottom, popup above it); the
    // background is transparent so the title art stays visible.
    const ImVec2 vp = ImGui::GetMainViewport()->Size;
    const ImVec2 origin = ImGui::GetMainViewport()->Pos;
    return { origin.x, origin.y, vp.x, vp.y };
}

void main_menu_overlay::draw_bar()
{
    const float s = theme::scale();
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    const ImVec2 win_pos = ImGui::GetWindowPos();
    const ImVec2 win_size = ImGui::GetWindowSize();
    const float button_h = 40.f;
    bar_height = ( button_h + tk.lg * 2.f ) * s;
    const float bar_top = win_pos.y + win_size.y - bar_height;
    ImDrawList *draw = ImGui::GetWindowDrawList();
    // Translucent charcoal strip with a quiet bronze rule on top.
    draw->AddRectFilled( ImVec2( win_pos.x, bar_top ), ImVec2( win_pos.x + win_size.x, win_pos.y + win_size.y ),
                         alpha_u32( tk.surface, 0.86f ) );
    draw->AddLine( ImVec2( win_pos.x, bar_top ), ImVec2( win_pos.x + win_size.x, bar_top ),
                   alpha_u32( tk.edge_bronze, 0.9f ), std::max( 1.f, 1.f * s ) );

    const int count = static_cast<int>( menu.vMenuItems.size() );
    // Order: play group, then the information/settings group, then Quit.
    const std::vector<int> order = {
        static_cast<int>( opt::NEWCHAR ), static_cast<int>( opt::LOADCHAR ), static_cast<int>( opt::WORLD ),
        static_cast<int>( opt::TUTORIAL ), -1,
        static_cast<int>( opt::SETTINGS ), static_cast<int>( opt::MOTD ), static_cast<int>( opt::HELP ),
        static_cast<int>( opt::CREDITS ), -1,
        static_cast<int>( opt::QUIT )
    };
    struct entry {
        int index;
        std::string label;
        std::string key;
        float width;
    };
    std::vector<entry> entries;
    const float gap = tk.sm * s;
    const float group_gap = tk.xl * s;
    float total_w = 0.f;
    for( int index : order ) {
        if( index < 0 ) {
            total_w += group_gap;
            entries.push_back( { -1, "", "", group_gap } );
            continue;
        }
        if( index >= count ) {
            continue;
        }
        const auto [label, key] = split_hotkey( menu.vMenuItems[index] );
        const float wdt = w::toolbar_button_width( label, category_icon( static_cast<opt>( index ) ), key );
        entries.push_back( { index, label, key, wdt } );
        total_w += wdt + gap;
    }
    // Centre the row; if the viewport is too narrow the buttons simply wrap.
    float x = win_pos.x + std::max( tk.lg * s, ( win_size.x - total_w ) * 0.5f );
    const float y = bar_top + tk.lg * s;
    ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.f );
    for( const entry &e : entries ) {
        if( e.index < 0 ) {
            x += e.width;
            continue;
        }
        if( x + e.width > win_pos.x + win_size.x - tk.lg * s ) {
            break;
        }
        ImGui::SetCursorScreenPos( ImVec2( x, y ) );
        const std::string id = "##cat" + std::to_string( e.index );
        const opt o = static_cast<opt>( e.index );
        const bool active = menu.sel1 == e.index && ( popup_open || !( has_drawer( e.index ) || has_text( e.index ) ) );
        if( w::toolbar_button( id.c_str(), e.label, category_icon( o ), e.key, active, button_h ) ) {
            if( menu.sel1 != e.index ) {
                menu.sel1 = e.index;
                menu.sel2 = e.index == static_cast<int>( opt::LOADCHAR ) ? static_cast<int>( menu.last_world_pos ) : 0;
                menu.sel_line = 0;
                menu.on_move();
                popup_open = has_drawer( e.index ) || has_text( e.index );
            } else if( has_drawer( e.index ) || has_text( e.index ) ) {
                popup_open = !popup_open;
            }
            if( o == opt::HELP || o == opt::TUTORIAL || o == opt::QUIT ) {
                queued = o == opt::QUIT ? "QUIT" : "CONFIRM";
            }
        }
        x += e.width + gap;
    }
    ImGui::PopStyleVar();

    // Version, muted, bottom-right of the strip; last played, bottom-left.
    const std::string ver = w::fit_text( getVersionString(), win_size.x * 0.25f );
    const ImVec2 vsz = ImGui::CalcTextSize( ver.c_str() );
    draw->AddText( ImVec2( win_pos.x + win_size.x - vsz.x - tk.md * s,
                           win_pos.y + win_size.y - vsz.y - tk.xs * s ),
                   alpha_u32( tk.text_muted, 0.8f ), ver.c_str() );
    if( !world_generator->last_world_name.empty() ) {
        std::string last = world_generator->last_world_name;
        if( !world_generator->last_character_name.empty() ) {
            last = world_generator->last_character_name + " · " + last;
        }
        const std::string line = w::fit_text( std::string( _( "Last played: " ) ) + last, win_size.x * 0.3f );
        const ImVec2 lsz = ImGui::CalcTextSize( line.c_str() );
        draw->AddText( ImVec2( win_pos.x + tk.md * s, win_pos.y + win_size.y - lsz.y - tk.xs * s ),
                       alpha_u32( tk.text_muted, 0.8f ), line.c_str() );
    }
}

void main_menu_overlay::draw_text_panel( const std::string &text )
{
    const float s = theme::scale();
    if( w::panel_begin( "textpanel", ImVec2( 0.f, 0.f ), true ) ) {
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

void main_menu_overlay::draw_drawer()
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
    if( entries.empty() ) {
        w::empty_state( _( "No worlds yet" ), _( "Start a new game to create one." ) );
        return;
    }
    if( o == opt::NEWCHAR && menu.sel2 >= 0 && static_cast<size_t>( menu.sel2 ) < menu.vNewGameHints.size() ) {
        ImGui::PushTextWrapPos( ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - tk.md * s );
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
}

void main_menu_overlay::draw_popup()
{
    const float s = theme::scale();
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    const ImVec2 win_pos = ImGui::GetWindowPos();
    const ImVec2 win_size = ImGui::GetWindowSize();
    const bool text = has_text( menu.sel1 );
    const float width = std::min( win_size.x - 48.f * s, ( text ? 820.f : 560.f ) * s );
    // Lists size to their entries (title bar + optional hint + rows), text panels to a page.
    const opt sel = static_cast<opt>( menu.sel1 );
    size_t rows = 0;
    if( sel == opt::NEWCHAR ) {
        rows = menu.vNewGameSubItems.size();
    } else if( sel == opt::SETTINGS ) {
        rows = menu.vSettingsSubItems.size();
    } else if( sel == opt::LOADCHAR || sel == opt::WORLD ) {
        rows = world_generator->get_all_worlds().size() + ( sel == opt::WORLD ? 1 : 0 );
    }
    const float list_h = ( 112.f + ( sel == opt::NEWCHAR ? 44.f : 0.f ) + std::max<size_t>( rows, 3 ) * 44.f ) * s;
    const float height = std::min( win_size.y * 0.66f, text ? 520.f * s : list_h );
    const ImVec2 pos( win_pos.x + ( win_size.x - width ) * 0.5f,
                      win_pos.y + win_size.y - bar_height - tk.md * s - height );
    const opt o = static_cast<opt>( menu.sel1 );
    const char *heading = o == opt::NEWCHAR ? _( "New game" ) : o == opt::SETTINGS ? _( "Settings" ) :
                          o == opt::LOADCHAR ? _( "Load game" ) : o == opt::WORLD ? _( "Worlds" ) :
                          o == opt::MOTD ? _( "Message of the day" ) : _( "Credits" );
    ImGui::SetNextWindowPos( pos, ImGuiCond_Always );
    ImGui::SetNextWindowSize( ImVec2( width, height ), ImGuiCond_Always );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 14.f * s, 14.f * s ) );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.f );
    if( ImGui::Begin( "##main_menu_popup", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav |
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings ) ) {
        if( w::window_shell( heading, w::frame_kind::dialog, true, category_icon( o ), true ) ) {
            popup_open = false;
        }
        if( text ) {
            draw_text_panel( o == opt::MOTD ? menu.mmenu_motd : menu.mmenu_credits );
        } else {
            draw_drawer();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar( 2 );
}

void main_menu_overlay::draw_controls()
{
    if( menu.sel1 != last_sel1 ) {
        // A hotkey or arrow key changed the category: show its popup.
        if( last_sel1 >= 0 ) {
            popup_open = has_drawer( menu.sel1 ) || has_text( menu.sel1 );
        }
        last_text_line = -1;
    }
    draw_bar();
    if( popup_open && ( has_drawer( menu.sel1 ) || has_text( menu.sel1 ) ) ) {
        draw_popup();
    }
    last_sel1 = menu.sel1;
}

#endif // TILES
