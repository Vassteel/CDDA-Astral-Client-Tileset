#pragma once
#ifndef CATA_SRC_OPTIONS_HYBRID_H
#define CATA_SRC_OPTIONS_HYBRID_H

#if defined(TILES)

#include <string>
#include <unordered_map>
#include <vector>

#include "options.h"

/**
 * Native (ImGui / Astral) presentation of the options pages: tab strip, search
 * box, one row per option with the widget that fits its type (switch, drop-down,
 * slider, text field), collapsible groups, and a description panel for the
 * option under the mouse or keyboard focus.
 *
 * The view edits the containers it is given directly (the same containers the
 * classic screen edits); saving / reverting / applying stays with
 * options_manager::show(), which hosts this view under TILES. World creation
 * hosts it too (world_only = true) for a draft world's options.
 */
class options_hybrid_view
{
    public:
        /** `world` is the container the world page edits (nullptr = the global one). */
        options_hybrid_view( options_manager &mgr, options_manager::options_container *world,
                             bool ingame, bool world_only );

        /** Draw into the current window, leaving footer_logical px (× scale) free below. */
        void draw( float footer_logical );

        /** True once any option was changed through the view. */
        bool changed() const {
            return dirty;
        }
        /** True if an option was edited since the last call (for "custom" markers). */
        bool take_edit() {
            const bool e = edited;
            edited = false;
            return e;
        }
        int current_page() const {
            return page;
        }

    private:
        options_manager &mgr;
        options_manager::options_container &global;
        options_manager::options_container &world;
        bool ingame;
        bool world_only;
        int page = 0;
        int world_page = -1;
        bool dirty = false;
        bool edited = false;
        std::string filter;
        std::string described;      // option name shown in the description panel
        std::string described_group;
        std::unordered_map<std::string, bool> groups_open;
        std::unordered_map<std::string, std::string> text_edits; // string_input scratch

        options_manager::options_container &container_for_page( int p );
        void draw_tabs();
        void draw_page( float height );
        void draw_option_row( options_manager::cOpt &opt, bool enabled, const std::string &reason );
        void draw_description( float width, float height );
        bool matches_filter( const options_manager::cOpt &opt ) const;
};

#endif // TILES
#endif // CATA_SRC_OPTIONS_HYBRID_H
