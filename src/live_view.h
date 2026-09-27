#pragma once
#ifndef CATA_SRC_LIVE_VIEW_H
#define CATA_SRC_LIVE_VIEW_H

#include <memory>

#include "point.h"

#if defined(TILES)
namespace cataimgui
{
class window;
}
#else
#include "cursesdef.h"
class ui_adaptor;
#endif

class live_view
{
    public:
        live_view();
        ~live_view();

        void init();
        void show( const tripoint &p );
        bool is_enabled();
        void hide();

    private:
        tripoint mouse_position;

#if defined(TILES)
        std::unique_ptr<cataimgui::window> imgui_win;
#else
        catacurses::window win;
        std::unique_ptr<ui_adaptor> ui;
#endif
};

#endif // CATA_SRC_LIVE_VIEW_H
