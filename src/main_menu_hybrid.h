#pragma once
#ifndef CATA_SRC_MAIN_MENU_HYBRID_H
#define CATA_SRC_MAIN_MENU_HYBRID_H

#if defined(TILES)

#include <string>

#include "cata_imgui.h"

class main_menu;

/**
 * Native ImGui presentation of the main menu (Astral shell) drawn over the
 * title artwork. It is an adapter: the menu state (sel1/sel2/sel_line), the
 * hotkeys and every action handler stay in main_menu; this window only draws
 * and queues actions for the existing input loop.
 */
class main_menu_overlay : public cataimgui::window
{
    public:
        explicit main_menu_overlay( main_menu &menu );
        /** Action queued by a mouse activation ("CONFIRM", "QUIT"), consumed once. */
        std::string take_action();
        bool has_action() const {
            return !queued.empty();
        }
        /** Call when sel1 changes so the window can resize for the drawer. */
        void layout_changed() {
            mark_resized();
        }
    protected:
        cataimgui::bounds get_bounds() override;
        void draw_controls() override;
    private:
        main_menu &menu;
        std::string queued;
        int last_sel1 = -1;
        int last_text_line = -1;
        void draw_categories( float width );
        void draw_drawer( float width );
        void draw_text_panel( const std::string &text, float width );
};

#endif // TILES
#endif // CATA_SRC_MAIN_MENU_HYBRID_H
