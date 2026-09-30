#pragma once
#ifndef CATA_SRC_UI_HYBRID_WINDOW_H
#define CATA_SRC_UI_HYBRID_WINDOW_H

#if defined(TILES)
#include <algorithm>
#include <functional>
#include <utility>

#include "cata_imgui.h"
#include "ui_hybrid_chrome.h"
#include "imgui/imgui.h"

/** Common resizable-viewport shell. Callbacks only render and queue actions;
 * activities and nested interfaces must run after the frame has ended. */
class hybrid_window : public cataimgui::window
{
    public:
        hybrid_window( const std::string &title, std::function<void()> controls ) :
            cataimgui::window( title, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                              ImGuiWindowFlags_NoMove ), controls( std::move( controls ) ) {
            set_shell( 0 );
        }
        void close() { is_open = false; }
        void set_hidden( bool hidden ) { hide_ui = hidden; }
    protected:
        cataimgui::bounds get_bounds() override {
            // One geometry for every large window: centred, clear of the main
            // menu's bar when that is showing.
            const ImVec4 b = ui_hybrid_chrome::theme::dialog_bounds();
            return { b.x, b.y, b.z, b.w };
        }
        void draw_controls() override {
            if( hide_ui ) {
                hide_if_hidden();
                return;
            }
            controls();
        }
    private:
        std::function<void()> controls;
};
#endif
#endif
