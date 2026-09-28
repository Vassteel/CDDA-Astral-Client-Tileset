#pragma once
#ifndef CATA_SRC_LIVE_VIEW_H
#define CATA_SRC_LIVE_VIEW_H

#include <memory>

#include "point.h"

#if defined(TILES)
// TILES: mouse-view content is a sibling Hybrid panel (ui_hybrid_sidebar), beside the sidebar.
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
        bool active = false;
#else
        catacurses::window win;
        std::unique_ptr<ui_adaptor> ui;
#endif
};

#endif // CATA_SRC_LIVE_VIEW_H
