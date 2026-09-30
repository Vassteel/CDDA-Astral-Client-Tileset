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
#include "mapsharing.h"
#include "mod_manager.h"
#include "cata_utility.h"

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
                       ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                       ImGuiWindowFlags_NoBringToFrontOnFocus ),
    menu( menu_ )
{
    // The overlay covers the whole viewport and the drawer popup is a
    // separate ImGui window begun from inside draw_controls().  The base
    // class brings the overlay to the display front after draw_controls()
    // every frame (window::draw, is_on_top path), which put the popup
    // *behind* a full-screen invisible window: rows highlighted on hover
    // for a frame but never received the click.  Keep the overlay at the
    // back so the popup created after it stays on top, and don't let a
    // click on the bar buttons re-raise the overlay over it either.
    force_to_back = true;
}

main_menu_overlay::~main_menu_overlay()
{
    // In-game windows centre on the whole viewport again.
    theme::set_reserved_bottom( 0.f );
}

void main_menu_overlay::queue( const std::string &action )
{
    queued = action;
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
    // Dialogs opened from here (ours and the blocking ones: options, world
    // creation, autopickup...) all centre in the space above the bar.
    theme::set_reserved_bottom( bar_height + tk.md * s );
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
                menu.sel2 = 0;
                menu.sel_line = 0;
                menu.on_move();
                popup_open = has_drawer( e.index ) || has_text( e.index );
                if( e.index == static_cast<int>( opt::LOADCHAR ) && menu.hybrid_world.empty() &&
                    menu.last_world_pos < world_generator->get_all_worlds().size() ) {
                    menu.hybrid_world = world_generator->get_world_name( menu.last_world_pos );
                }
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

namespace
{
std::string character_count( const WORLD &world )
{
    const size_t n = world.world_saves.size();
    return n == 0 ? std::string( _( "no characters" ) ) :
           string_format( n_gettext( "%d character", "%d characters", n ), n );
}
} // namespace

void main_menu_overlay::draw_world_list( const char *id, bool with_new_entry, bool empty_only )
{
    const float s = theme::scale();
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    if( !w::panel_begin( id, ImVec2( 0.f, 0.f ), true ) ) {
        w::panel_end();
        return;
    }
    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( tk.sm * s, tk.xs * s ) );
    if( with_new_entry ) {
        w::row_state st;
        st.selected = menu.hybrid_world.empty();
        st.featured = true;
        const w::row_result r = w::selectable_row( "new_world", _( "New world" ),
                                empty_only ? _( "made for you" ) : _( "opens the creator" ), "world", nullptr, st );
        if( r.clicked ) {
            menu.hybrid_world.clear();
            menu.hybrid_save = -1;
        }
    }
    int shown = 0;
    for( const auto &kv : world_generator->get_all_worlds() ) {
        const WORLD &world = *kv.second;
        if( empty_only && !world.world_saves.empty() ) {
            continue;
        }
        w::row_state st;
        st.selected = menu.hybrid_world == kv.first;
        const w::row_result r = w::selectable_row( kv.first.c_str(), kv.first, character_count( world ), nullptr,
                                nullptr, st );
        if( r.clicked || r.double_clicked ) {
            if( menu.hybrid_world != kv.first ) {
                menu.hybrid_save = -1;
            }
            menu.hybrid_world = kv.first;
            menu.on_move();
        }
        ++shown;
    }
    if( shown == 0 && !with_new_entry ) {
        w::empty_state( _( "No worlds yet" ), _( "Create one from the Worlds screen or start a new game." ) );
    }
    ImGui::PopStyleVar();
    w::panel_end();
}

void main_menu_overlay::draw_new_game()
{
    const float s = theme::scale();
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    const float footer = tk.footer;
    if( w::body_begin( "newgame_body", footer, ImGuiWindowFlags_NoScrollbar ) ) {
        const float gap = tk.lg * s;
        const float left_w = std::floor( ImGui::GetContentRegionAvail().x * 0.45f );
        const float h = ImGui::GetContentRegionAvail().y;
        // Left: how to make the character.
        ImGui::BeginChild( "newgame_left", ImVec2( left_w, h ), ImGuiChildFlags_None,
                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
        w::section_label( _( "Character" ), "person" );
        const bool preset = menu.sel2 == 1;
        const float list_h = preset ? std::floor( h * 0.45f ) : 0.f;
        if( w::panel_begin( "modes", ImVec2( 0.f, list_h ), true, ImGuiWindowFlags_NoScrollbar ) ) {
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( tk.sm * s, tk.xs * s ) );
            for( int i = 0; i < static_cast<int>( menu.vNewGameSubItems.size() ); ++i ) {
                const auto [label, key] = split_hotkey( menu.vNewGameSubItems[i] );
                w::row_state st;
                st.selected = menu.sel2 == i;
                const std::string id = "mode" + std::to_string( i );
                const w::row_result r = w::selectable_row( id.c_str(), label, std::string(), nullptr, nullptr, st );
                if( r.hovered && i < static_cast<int>( menu.vNewGameHints.size() ) ) {
                    w::tooltip( menu.vNewGameHints[i] );
                }
                if( r.clicked ) {
                    if( menu.sel2 != i ) {
                        menu.on_move();
                    }
                    menu.sel2 = i;
                    // Play Now uses an empty world made for it unless one is chosen.
                    if( i == 3 || i == 4 ) {
                        WORLD *chosen = menu.hybrid_world.empty() ? nullptr : world_generator->get_world( menu.hybrid_world );
                        if( chosen != nullptr && !chosen->world_saves.empty() ) {
                            menu.hybrid_world.clear();
                        }
                    }
                }
                if( r.double_clicked ) {
                    menu.sel2 = i;
                    queue( "NEWCHAR_START" );
                }
            }
            ImGui::PopStyleVar();
        }
        w::panel_end();
        if( preset ) {
            ImGui::Dummy( ImVec2( 0.f, tk.xs * s ) );
            w::section_label( _( "Templates" ), "save" );
            if( w::panel_begin( "templates", ImVec2( 0.f, 0.f ), true ) ) {
                ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( tk.sm * s, tk.xs * s ) );
                if( menu.templates.empty() ) {
                    w::empty_state( _( "No templates" ), _( "Save one from the character creator or a loaded character." ) );
                }
                for( int i = 0; i < static_cast<int>( menu.templates.size() ); ++i ) {
                    w::row_state st;
                    st.selected = menu.hybrid_template == i;
                    const std::string id = "tmpl" + std::to_string( i );
                    const w::row_result r = w::selectable_row( id.c_str(), menu.templates[i], std::string(), nullptr,
                                            nullptr, st );
                    if( r.clicked ) {
                        menu.hybrid_template = i;
                    }
                    if( r.double_clicked ) {
                        menu.hybrid_template = i;
                        queue( "NEWCHAR_START" );
                    }
                }
                ImGui::PopStyleVar();
            }
            w::panel_end();
        }
        ImGui::EndChild();
        ImGui::SameLine( 0.f, gap );
        // Right: the world to play in.
        ImGui::BeginChild( "newgame_right", ImVec2( 0.f, h ), ImGuiChildFlags_None,
                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
        const bool play_now = menu.sel2 == 3 || menu.sel2 == 4;
        w::section_label( _( "World" ), "world" );
        ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s", play_now ?
                            _( "Play Now starts in an empty world; pick one or let the game make it." ) :
                            _( "A world with characters in it warns before adding another." ) );
        draw_world_list( "newgame_worlds", true, play_now );
        ImGui::EndChild();
    }
    w::body_end();
    if( w::footer_begin( "newgame_footer" ) ) {
        const std::string hint = menu.sel2 >= 0 && static_cast<size_t>( menu.sel2 ) < menu.vNewGameHints.size() ?
                                 menu.vNewGameHints[menu.sel2] : std::string();
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s",
                            w::fit_text( hint, ImGui::GetContentRegionAvail().x * 0.55f ).c_str() );
        ImGui::SameLine();
        const float del_w = menu.sel2 == 1 ? w::action_button_width( _( "Delete template" ) ) +
                            ImGui::GetStyle().ItemSpacing.x : 0.f;
        const float start_w = w::action_button_width( _( "Start" ), w::button_kind::primary );
        w::footer_align_right( del_w + start_w );
        if( menu.sel2 == 1 ) {
            if( w::action_button( _( "Delete template" ), w::button_kind::danger, ImVec2( 0, 0 ),
                                  menu.hybrid_template >= 0, _( "Select a template first." ) ) ) {
                queue( "TEMPLATE_DELETE" );
            }
            ImGui::SameLine();
        }
        const bool can_start = menu.sel2 != 1 || menu.hybrid_template >= 0;
        if( w::action_button( _( "Start" ), w::button_kind::primary, ImVec2( 0, 0 ), can_start,
                              _( "Select a template first." ) ) ) {
            queue( "NEWCHAR_START" );
        }
    }
    w::footer_end();
}

void main_menu_overlay::draw_load()
{
    const float s = theme::scale();
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    if( w::body_begin( "load_body", tk.footer, ImGuiWindowFlags_NoScrollbar ) ) {
        const float gap = tk.lg * s;
        const float left_w = std::floor( ImGui::GetContentRegionAvail().x * 0.4f );
        const float h = ImGui::GetContentRegionAvail().y;
        ImGui::BeginChild( "load_left", ImVec2( left_w, h ), ImGuiChildFlags_None,
                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
        w::section_label( _( "Worlds" ), "world" );
        draw_world_list( "load_worlds", false, false );
        ImGui::EndChild();
        ImGui::SameLine( 0.f, gap );
        ImGui::BeginChild( "load_right", ImVec2( 0.f, h ), ImGuiChildFlags_None,
                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
        w::section_label( _( "Characters" ), "person" );
        WORLD *world = menu.hybrid_world.empty() ? nullptr : world_generator->get_world( menu.hybrid_world );
        if( w::panel_begin( "load_saves", ImVec2( 0.f, 0.f ), true ) ) {
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( tk.sm * s, tk.xs * s ) );
            if( world == nullptr ) {
                w::empty_state( _( "Choose a world" ), _( "Its characters are listed here." ) );
            } else if( world->world_saves.empty() ) {
                w::empty_state( _( "No characters in this world" ), _( "Start a new game to add one." ) );
            } else {
                for( int i = 0; i < static_cast<int>( world->world_saves.size() ); ++i ) {
                    const save_t &save = world->world_saves[i];
                    if( MAP_SHARING::isSharing() && save.decoded_name() != MAP_SHARING::getUsername() ) {
                        continue;
                    }
                    w::row_state st;
                    st.selected = menu.hybrid_save == i;
                    const std::string id = "save" + std::to_string( i );
                    const w::row_result r = w::selectable_row( id.c_str(), save.decoded_name(),
                                            menu.playtime_text( world, save ), "person", nullptr, st );
                    if( r.clicked ) {
                        menu.hybrid_save = i;
                    }
                    if( r.double_clicked ) {
                        menu.hybrid_save = i;
                        queue( "LOAD_START" );
                    }
                }
            }
            ImGui::PopStyleVar();
        }
        w::panel_end();
        ImGui::EndChild();
    }
    w::body_end();
    if( w::footer_begin( "load_footer" ) ) {
        const bool have = !menu.hybrid_world.empty() && menu.hybrid_save >= 0;
        if( w::action_button( _( "Manage world…" ), w::button_kind::tertiary, ImVec2( 0, 0 ),
                              !menu.hybrid_world.empty(), _( "Choose a world first." ) ) ) {
            menu.sel1 = static_cast<int>( opt::WORLD );
            menu.on_move();
        }
        ImGui::SameLine();
        const float del_w = w::action_button_width( _( "Delete character" ) );
        const float load_w = w::action_button_width( _( "Load" ), w::button_kind::primary );
        w::footer_align_right( del_w + load_w + ImGui::GetStyle().ItemSpacing.x );
        if( w::action_button( _( "Delete character" ), w::button_kind::danger, ImVec2( 0, 0 ), have,
                              _( "Select a character first." ) ) ) {
            queue( "LOAD_DELETE" );
        }
        ImGui::SameLine();
        if( w::action_button( _( "Load" ), w::button_kind::primary, ImVec2( 0, 0 ), have,
                              _( "Select a character first." ) ) ) {
            queue( "LOAD_START" );
        }
    }
    w::footer_end();
}

void main_menu_overlay::draw_worlds()
{
    const float s = theme::scale();
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    if( w::body_begin( "worlds_body", tk.footer, ImGuiWindowFlags_NoScrollbar ) ) {
        const float gap = tk.lg * s;
        const float left_w = std::floor( ImGui::GetContentRegionAvail().x * 0.4f );
        const float h = ImGui::GetContentRegionAvail().y;
        ImGui::BeginChild( "worlds_left", ImVec2( left_w, h ), ImGuiChildFlags_None,
                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
        if( w::action_button( _( "Create world…" ), w::button_kind::primary, ImVec2( -1.f, 0.f ), true, nullptr,
                              "world" ) ) {
            queue( "WORLD_CREATE" );
        }
        ImGui::Dummy( ImVec2( 0.f, tk.xs * s ) );
        draw_world_list( "worlds_list", false, false );
        ImGui::EndChild();
        ImGui::SameLine( 0.f, gap );
        ImGui::BeginChild( "worlds_right", ImVec2( 0.f, h ), ImGuiChildFlags_None,
                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
        WORLD *world = menu.hybrid_world.empty() ? nullptr : world_generator->get_world( menu.hybrid_world );
        if( world == nullptr ) {
            w::empty_state( _( "Choose a world" ), _( "Its mods, characters and settings show here." ) );
        } else {
            w::push_font_section();
            ImGui::TextColored( ui_hybrid_chrome::palette::accent(), "%s", world->world_name.c_str() );
            w::pop_font();
            ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s",
                                string_format( _( "%s · save compression %s" ), character_count( *world ),
                                               world->has_compression_enabled() ? _( "on" ) : _( "off" ) ).c_str() );
            ImGui::Dummy( ImVec2( 0.f, tk.xs * s ) );
            const float half = std::floor( ( ImGui::GetContentRegionAvail().y - tk.sm * s ) * 0.5f );
            w::section_label( _( "Characters" ), "person" );
            if( w::panel_begin( "world_saves", ImVec2( 0.f, half - ImGui::GetTextLineHeightWithSpacing() ), true ) ) {
                ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( tk.sm * s, tk.xs * s ) );
                if( world->world_saves.empty() ) {
                    w::empty_state( _( "No characters" ) );
                }
                for( int i = 0; i < static_cast<int>( world->world_saves.size() ); ++i ) {
                    const save_t &save = world->world_saves[i];
                    w::row_state st;
                    st.selected = menu.hybrid_save == i;
                    const std::string id = "wsave" + std::to_string( i );
                    if( w::selectable_row( id.c_str(), save.decoded_name(), menu.playtime_text( world, save ),
                                           "person", nullptr, st ).clicked ) {
                        menu.hybrid_save = i;
                    }
                }
                ImGui::PopStyleVar();
            }
            w::panel_end();
            w::section_label( string_format( _( "Mods (%d)" ), world->active_mod_order.size() ), "list" );
            if( w::panel_begin( "world_mods", ImVec2( 0.f, 0.f ), true ) ) {
                for( const mod_id &mod : world->active_mod_order ) {
                    const std::string name = mod.is_valid() ? mod->name() : mod.str();
                    ImGui::BulletText( "%s", name.c_str() );
                    if( mod.is_valid() && ImGui::IsItemHovered() && !mod->description.empty() ) {
                        w::tooltip( mod->description.translated() );
                    }
                }
            }
            w::panel_end();
        }
        ImGui::EndChild();
    }
    w::body_end();
    if( w::footer_begin( "worlds_footer" ) ) {
        WORLD *world = menu.hybrid_world.empty() ? nullptr : world_generator->get_world( menu.hybrid_world );
        const bool have = world != nullptr;
        const bool have_save = have && menu.hybrid_save >= 0 &&
                               static_cast<size_t>( menu.hybrid_save ) < world->world_saves.size();
        if( w::action_button( _( "Copy settings…" ), w::button_kind::secondary, ImVec2( 0, 0 ), have,
                              _( "Choose a world first." ) ) ) {
            queue( "WORLD_COPY" );
        }
        ImGui::SameLine();
        if( w::action_button( _( "Character to template" ), w::button_kind::secondary, ImVec2( 0, 0 ), have_save,
                              _( "Select a character first." ) ) ) {
            queue( "WORLD_TEMPLATE" );
        }
        ImGui::SameLine();
        if( w::action_button( have && world->has_compression_enabled() ? _( "Compression off" ) : _( "Compression on" ),
                              w::button_kind::tertiary, ImVec2( 0, 0 ), have, _( "Choose a world first." ) ) ) {
            queue( "WORLD_COMPRESSION" );
        }
        ImGui::SameLine();
        const float reset_w = w::action_button_width( _( "Reset" ) );
        const float del_w = w::action_button_width( _( "Delete" ) );
        w::footer_align_right( reset_w + del_w + ImGui::GetStyle().ItemSpacing.x );
        if( w::action_button( _( "Reset" ), w::button_kind::danger, ImVec2( 0, 0 ), have,
                              _( "Choose a world first." ) ) ) {
            queue( "WORLD_RESET" );
        }
        ImGui::SameLine();
        if( w::action_button( _( "Delete" ), w::button_kind::danger, ImVec2( 0, 0 ), have,
                              _( "Choose a world first." ) ) ) {
            queue( "WORLD_DELETE" );
        }
    }
    w::footer_end();
}

void main_menu_overlay::draw_settings()
{
    const float s = theme::scale();
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    static const char *descriptions[] = {
        translate_marker( "Every game option: interface, graphics, world defaults, debug." ),
        translate_marker( "Choose the map tileset." ),
        translate_marker( "Rebind keys and gamepad buttons." ),
        translate_marker( "Rules for what is picked up automatically." ),
        translate_marker( "Rules for when safe mode stops you." ),
        translate_marker( "Colour palette of the text-drawn screens." ),
        translate_marker( "Colour theme of the native windows." ),
        translate_marker( "Dear ImGui's demo window (development)." ),
        translate_marker( "Every Astral component on one page (development)." )
    };
    if( w::body_begin( "settings_body", 0.f, ImGuiWindowFlags_NoScrollbar ) ) {
        const float gap = tk.lg * s;
        const float left_w = std::floor( ImGui::GetContentRegionAvail().x * 0.45f );
        const float h = ImGui::GetContentRegionAvail().y;
        ImGui::BeginChild( "settings_left", ImVec2( left_w, h ), ImGuiChildFlags_None,
                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
        if( w::panel_begin( "settings_list", ImVec2( 0.f, 0.f ), true ) ) {
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( tk.sm * s, tk.xs * s ) );
            for( int i = 0; i < static_cast<int>( menu.vSettingsSubItems.size() ); ++i ) {
                const auto [label, key] = split_hotkey( menu.vSettingsSubItems[i] );
                w::row_state st;
                st.selected = menu.sel2 == i;
                const std::string id = "set" + std::to_string( i );
                const w::row_result r = w::selectable_row( id.c_str(), label, std::string(), nullptr, nullptr, st,
                                        0.f, "chevron_right" );
                if( !key.empty() ) {
                    ImDrawList *draw = ImGui::GetWindowDrawList();
                    const ImVec2 ksz = ImGui::CalcTextSize( key.c_str() );
                    draw->AddText( ImVec2( r.max.x - tk.md * s - ksz.x - tk.lg * s,
                                           r.min.y + ( r.max.y - r.min.y - ksz.y ) * 0.5f ),
                                   st.selected ? tk.accent : tk.accent_dim, key.c_str() );
                }
                if( r.hovered ) {
                    menu.sel2 = i;
                }
                if( r.clicked || r.double_clicked ) {
                    menu.sel2 = i;
                    menu.sel_line = 0;
                    queue( "CONFIRM" );
                }
            }
            ImGui::PopStyleVar();
        }
        w::panel_end();
        ImGui::EndChild();
        ImGui::SameLine( 0.f, gap );
        ImGui::BeginChild( "settings_right", ImVec2( 0.f, h ), ImGuiChildFlags_None,
                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
        if( menu.sel2 >= 0 && static_cast<size_t>( menu.sel2 ) < menu.vSettingsSubItems.size() ) {
            const auto [label, key] = split_hotkey( menu.vSettingsSubItems[menu.sel2] );
            w::push_font_section();
            ImGui::TextColored( ui_hybrid_chrome::palette::accent(), "%s", label.c_str() );
            w::pop_font();
            const size_t n = sizeof( descriptions ) / sizeof( descriptions[0] );
            if( static_cast<size_t>( menu.sel2 ) < n ) {
                ImGui::PushTextWrapPos( ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x );
                ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s", _( descriptions[menu.sel2] ) );
                ImGui::PopTextWrapPos();
            }
            ImGui::Dummy( ImVec2( 0.f, tk.sm * s ) );
            if( w::action_button( _( "Open" ), w::button_kind::primary ) ) {
                queue( "CONFIRM" );
            }
        }
        ImGui::EndChild();
    }
    w::body_end();
}

void main_menu_overlay::draw_popup()
{
    const float s = theme::scale();
    const ImVec4 b = theme::dialog_bounds();
    const opt o = static_cast<opt>( menu.sel1 );
    const bool text = has_text( menu.sel1 );
    const char *heading = o == opt::NEWCHAR ? _( "New game" ) : o == opt::SETTINGS ? _( "Settings" ) :
                          o == opt::LOADCHAR ? _( "Load game" ) : o == opt::WORLD ? _( "Worlds" ) :
                          o == opt::MOTD ? _( "Message of the day" ) : _( "Credits" );
    // A child of the overlay window (not a separate top-level window): the
    // overlay covers the whole viewport and is kept in front, so a separate
    // window underneath it would never receive the mouse.
    ImGui::SetCursorScreenPos( ImVec2( b.x, b.y ) );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 20.f * s, 20.f * s ) );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.f );
    if( ImGui::BeginChild( "##main_menu_popup", ImVec2( b.z, b.w ), ImGuiChildFlags_AlwaysUseWindowPadding,
                           ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
                           ImGuiWindowFlags_NoScrollWithMouse ) ) {
        if( w::window_shell( heading, w::frame_kind::large, true, category_icon( o ), true ) ) {
            popup_open = false;
        }
        if( text ) {
            draw_text_panel( o == opt::MOTD ? menu.mmenu_motd : menu.mmenu_credits );
        } else if( o == opt::NEWCHAR ) {
            draw_new_game();
        } else if( o == opt::LOADCHAR ) {
            draw_load();
        } else if( o == opt::WORLD ) {
            draw_worlds();
        } else if( o == opt::SETTINGS ) {
            draw_settings();
        }
    }
    ImGui::EndChild();
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
