#pragma once
#ifndef CATA_SRC_UI_MOUSE_ACTIONS_H
#define CATA_SRC_UI_MOUSE_ACTIONS_H

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "catacharset.h"
#include "cuboid_rectangle.h"
#include "input_context.h"
#include "output.h"
#include "translations.h"
#include "uilist.h"

// Mouse controls dispatch the same action identifiers as the keyboard context.
// Each bar owns its hit regions; coordinates are relative to its actual window.
class mouse_action_bar
{
    public:
        static void register_input( input_context &ctxt ) {
            for( const char *action : { "SELECT", "MOUSE_MOVE", "COORDINATE", "SCROLL_UP", "SCROLL_DOWN" } ) {
                ctxt.register_action( action );
            }
        }

        void draw( const catacurses::window &win, int start_y,
                   const std::vector<std::pair<std::string, std::string>> &actions ) {
            buttons.clear();
            overflow.clear();
            window = win;
            int x = 1;
            int y = start_y;
            const int right = getmaxx( win ) - 1;
            for( const auto &action : actions ) {
                const std::string label = "[" + action.second + "]";
                const int width = utf8_width( label, true );
                if( x + width > right ) {
                    x = 1;
                    ++y;
                }
                if( y >= getmaxy( win ) - 1 ) {
                    // Localized labels may need more room than the English
                    // layout. Keep every action accessible in a mouse list.
                    const int last = getmaxy( win ) - 2;
                    buttons.erase( std::remove_if( buttons.begin(), buttons.end(),
                    [last]( const auto &button ) { return button.first.p_min.y >= last; } ), buttons.end() );
                    mvwhline( win, point( 1, last ), ' ', right - 1 );
                    const std::string more = "[" + std::string( _( "More" ) ) + "]";
                    mvwprintz( win, point( 1, last ), c_light_gray, "%s", more );
                    buttons.emplace_back( inclusive_rectangle<point>( point( 1, last ),
                                          point( utf8_width( more ), last ) ), "MOUSE_MORE" );
                    overflow = actions;
                    break;
                }
                nc_color color = c_light_gray;
                print_colored_text( win, point( x, y ), color, c_light_gray, label );
                buttons.emplace_back( inclusive_rectangle<point>( point( x, y ),
                                      point( x + width - 1, y ) ), action.first );
                x += width + 1;
            }
        }

        std::string process( input_context &ctxt, const std::string &action ) const {
            if( action == "SELECT" ) {
                if( const auto p = ctxt.get_coordinates_text( window ) ) {
                    for( const auto &button : buttons ) {
                        if( button.first.contains( *p ) ) {
                            if( button.second == "MOUSE_MORE" ) {
                                uilist menu;
                                menu.title = _( "Actions" );
                                for( size_t i = 0; i < overflow.size(); ++i ) {
                                    menu.addentry( static_cast<int>( i ), true, MENU_AUTOASSIGN,
                                                   overflow[i].second );
                                }
                                menu.query();
                                if( menu.ret < 0 || menu.ret >= static_cast<int>( overflow.size() ) ) {
                                    return "MOUSE_MOVE";
                                }
                                const std::string &chosen = overflow[menu.ret].first;
                                if( chosen != "HELP_KEYBINDINGS" ) {
                                    return chosen;
                                }
                                ctxt.display_menu();
                                return "MOUSE_MOVE";
                            }
                            if( button.second == "HELP_KEYBINDINGS" ) {
                                ctxt.display_menu();
                                return "MOUSE_MOVE";
                            }
                            return button.second;
                        }
                    }
                }
            }
            if( action == "SCROLL_UP" ) {
                return "UP";
            }
            if( action == "SCROLL_DOWN" ) {
                return "DOWN";
            }
            return action;
        }

        static bool contains( const catacurses::window &win, const point &p ) {
            return p.x >= 0 && p.y >= 0 && p.x < getmaxx( win ) && p.y < getmaxy( win );
        }

    private:
        catacurses::window window;
        std::vector<std::pair<inclusive_rectangle<point>, std::string>> buttons;
        std::vector<std::pair<std::string, std::string>> overflow;
};
#endif
