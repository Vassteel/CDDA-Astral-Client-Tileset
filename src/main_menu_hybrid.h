#pragma once
#ifndef CATA_SRC_MAIN_MENU_HYBRID_H
#define CATA_SRC_MAIN_MENU_HYBRID_H

#if defined(TILES)

#include <string>

#include "cata_imgui.h"

class main_menu;

/**
 * Native ImGui presentation of the main menu (Astral shell) drawn over the
 * title artwork: a row of category buttons along the bottom of the screen and,
 * when a category is open, one dialog above the bar — the same size and frame
 * for every category (New Game, Load, Worlds, Settings, MOTD, Credits).
 *
 * It is an adapter: the menu state (sel1/sel2/sel_line), the hotkeys and every
 * action handler stay in main_menu; this window only draws and queues actions
 * (the classic "CONFIRM"/"QUIT" or the hybrid ones main_menu::handle_hybrid_action
 * runs) for the existing input loop.
 */
class main_menu_overlay : public cataimgui::window
{
    public:
        explicit main_menu_overlay( main_menu &menu );
        ~main_menu_overlay() override;
        /** Action queued by a mouse activation, consumed once. */
        std::string take_action();
        bool has_action() const {
            return !queued.empty();
        }
        /** Call when sel1 changes so the window can resize for the drawer. */
        void layout_changed() {
            mark_resized();
        }
        /** Escape while a category dialog is open closes it; returns true if it did. */
        bool close_popup();
        /** A category hotkey / arrow key selected sel1: open its dialog. */
        void category_selected();
    protected:
        cataimgui::bounds get_bounds() override;
        void draw_controls() override;
    private:
        main_menu &menu;
        std::string queued;
        int last_sel1 = -1;
        int last_text_line = -1;
        bool popup_open = false;
        float bar_height = 0.f;
        void draw_bar();
        void draw_popup();
        void draw_text_panel( const std::string &text );
        void draw_new_game();
        void draw_load();
        void draw_worlds();
        void draw_settings();
        /** World list column shared by Load / Worlds / New Game; returns the selected name. */
        void draw_world_list( const char *id, bool with_new_entry, bool empty_only );
        void queue( const std::string &action );
};

#endif // TILES
#endif // CATA_SRC_MAIN_MENU_HYBRID_H
