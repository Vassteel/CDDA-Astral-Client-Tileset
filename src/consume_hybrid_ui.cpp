#include "consume_hybrid_ui.h"

#if defined(TILES)

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "cata_imgui.h"
#include "character.h"
#include "color.h"
#include "flag.h"
#include "input_context.h"
#include "inventory_ui.h"
#include "item.h"
#include "itype.h"
#include "item_category.h"
#include "item_location.h"
#include "output.h"
#include "popup.h"
#include "translations.h"
#include "item_context_menu.h"
#include "ui_hybrid_chrome.h"
#include "ui_manager.h"

#include "imgui/imgui.h"
#include <imgui/imgui_internal.h>

namespace
{

std::vector<std::string> split_lines( const std::string &s )
{
    std::vector<std::string> out;
    std::string cur;
    for( char c : s ) {
        if( c == '\n' ) {
            out.push_back( cur );
            cur.clear();
        } else {
            cur.push_back( c );
        }
    }
    out.push_back( cur );
    return out;
}

/** Expose protected column list so we can render without the classic UI. */
class consume_collector : public inventory_pick_selector
{
    public:
        consume_collector( Character &u, const inventory_selector_preset &preset )
            : inventory_pick_selector( u, preset ) {}

        const std::vector<inventory_column *> &columns() const {
            return get_all_columns();
        }

        const inventory_selector_preset &preset_ref() const {
            return preset;
        }
};

struct row_t {
    inventory_entry *entry = nullptr;
    std::string name;
    std::string category;
    std::vector<std::string> cells;
    std::string denial;
    bool selectable = false;
};

std::string strip_for_filter( const std::string &s )
{
    std::string out = remove_color_tags( s );
    for( char &c : out ) {
        c = static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );
    }
    return out;
}

bool filter_match( const row_t &row, const std::string &filter_lc )
{
    if( filter_lc.empty() ) {
        return true;
    }
    if( strip_for_filter( row.name ).find( filter_lc ) != std::string::npos ) {
        return true;
    }
    if( strip_for_filter( row.category ).find( filter_lc ) != std::string::npos ) {
        return true;
    }
    return false;
}

bool tab_match( const item &it, const std::string &tab )
{
    if( tab.empty() || tab == "ALL" ) {
        return true;
    }
    if( tab == "FOOD" ) {
        return ( it.is_comestible() && it.get_comestible()->comesttype == "FOOD" ) ||
               it.has_flag( flag_USE_EAT_VERB );
    }
    if( tab == "DRINK" ) {
        return it.is_comestible() && it.get_comestible()->comesttype == "DRINK" &&
               !it.has_flag( flag_USE_EAT_VERB );
    }
    if( tab == "MED" ) {
        return it.is_medication() || it.is_medical_tool();
    }
    return true;
}


/** Primary consume-menu verb: Drink / Eat / Take / Apply / Consume.
 *  Matches soft-fork item_context_menu::consume_label_for (+ Apply for tools). */
std::string consume_action_label( const item &it )
{
    if( it.is_medical_tool() && !it.is_medication() && !it.is_comestible() ) {
        return _( "Apply" );
    }
    if( it.is_medication() || ( it.is_comestible() &&
                                it.get_comestible()->comesttype == "MED" ) ) {
        return _( "Take" );
    }
    if( !it.is_comestible() ) {
        return _( "Consume" );
    }
    if( it.has_flag( flag_USE_EAT_VERB ) ) {
        return _( "Eat" );
    }
    if( it.get_comestible()->comesttype == "DRINK" ) {
        return _( "Drink" );
    }
    return _( "Eat" );
}

class consume_hybrid_window : public cataimgui::window
{
    public:
        consume_hybrid_window( Character &you, consume_collector &collector,
                               const std::string &title, const std::string &hint,
                               std::string filter, std::string tab )
            : cataimgui::window( title,
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav ),
              you( you ), collector( collector ), hint( hint ),
              filter_text( std::move( filter ) ), active_tab( std::move( tab ) ) {
            if( active_tab.empty() ) {
                active_tab = "ALL";
            }
            rebuild_rows();
        }

        bool done() const {
            return finished;
        }
        item_location result() const {
            return selected;
        }

        /** Run deferred Examine outside ImGui Begin/End (avoids nested-UI CTD). */
        void flush_deferred() {
            if( pending_examine_loc ) {
                item_location loc = pending_examine_loc;
                pending_examine_loc = item_location::nowhere;
                item_context_menu::perform( you, loc, item_context_menu::action::examine );
            }
        }

        void process_action( const std::string &action ) {
            if( action == "QUIT" ) {
                selected = item_location::nowhere;
                finished = true;
                is_open = false;
            } else if( action == "CONFIRM" || action == "SELECT" ) {
                try_confirm();
            } else if( action == "FILTER" ) {
                filter_focus = true;
            } else if( action == "UP" || action == "SCROLL_UP" ) {
                if( selected_row > 0 ) {
                    --selected_row;
                }
            } else if( action == "DOWN" || action == "SCROLL_DOWN" ) {
                if( selected_row + 1 < static_cast<int>( visible.size() ) ) {
                    ++selected_row;
                }
            } else if( action == "PAGE_UP" ) {
                selected_row = std::max( 0, selected_row - 10 );
            } else if( action == "PAGE_DOWN" ) {
                selected_row = std::min( std::max( 0,
                                                    static_cast<int>( visible.size() ) - 1 ),
                                         selected_row + 10 );
            } else if( action == "LEFT" ) {
                cycle_tab( -1 );
            } else if( action == "RIGHT" ) {
                cycle_tab( 1 );
            }
        }

    protected:
        void draw() override {
            ui_hybrid_chrome::push();
            cataimgui::window::draw();
            ui_hybrid_chrome::pop();
        }

        cataimgui::bounds get_bounds() override {
            const ImVec2 vp = ImGui::GetMainViewport()->Size;
            const float scale = std::max( 1.f, ImGui::GetFontSize() / 16.f );
            return { -1.f, -1.f, std::min( vp.x * 0.94f, 1280.f * scale ),
                     std::min( vp.y * 0.92f, 720.f * scale ) };
        }

        void draw_controls() override {
            hide_if_hidden();
            if( !get_is_open() ) {
                return;
            }

            ui_hybrid_chrome::section_header( _( "Status" ) );
            for( const std::string &line : split_lines( hint ) ) {
                if( line.empty() ) {
                    continue;
                }
                cataimgui::draw_colored_text( line, c_light_gray );
            }

            ImGui::Spacing();
            ui_hybrid_chrome::section_header( _( "Consume" ) );

            static const std::pair<const char *, const char *> tabs[] = {
                { "ALL",   translate_marker( "All" ) },
                { "FOOD",  translate_marker( "Food" ) },
                { "DRINK", translate_marker( "Drink" ) },
                { "MED",   translate_marker( "Med" ) },
            };
            for( size_t i = 0; i < sizeof( tabs ) / sizeof( tabs[0] ); ++i ) {
                if( i > 0 ) {
                    ImGui::SameLine();
                }
                const bool active = active_tab == tabs[i].first;
                const int n = ui_hybrid_chrome::push_toolbar_button( active );
                if( ImGui::Button( _( tabs[i].second ) ) ) {
                    active_tab = tabs[i].first;
                    apply_filters();
                }
                ui_hybrid_chrome::draw_item_bezel( active, ImGui::IsItemHovered(), false );
                ImGui::PopStyleColor( n );
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth( 220.f );
            if( filter_focus ) {
                ImGui::SetKeyboardFocusHere();
                filter_focus = false;
            }
            char buf[256];
            std::snprintf( buf, sizeof( buf ), "%s", filter_text.c_str() );
            if( ImGui::InputTextWithHint( "##consume_filter", _( "Filter…" ), buf,
                                          sizeof( buf ) ) ) {
                filter_text = buf;
                apply_filters();
            }

            ImGui::SameLine();
            ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s",
                                string_format( _( "%d items" ),
                                               static_cast<int>( visible.size() ) ).c_str() );

            ImGui::Spacing();

            const inventory_selector_preset &preset = collector.preset_ref();
            const size_t ncells = std::max<size_t>( 1, preset.get_cells_count() );

            std::vector<std::string> headers;
            headers.emplace_back( _( "Food" ) );
            static const char *const col_titles[] = {
                translate_marker( "Calories" ),
                translate_marker( "Quench" ),
                translate_marker( "Joy/Max" ),
                translate_marker( "Health" ),
                translate_marker( "Shelf life" ),
                translate_marker( "Volume" ),
                translate_marker( "Satiety" ),
                translate_marker( "Consume time" ),
                translate_marker( "Freshness" ),
                translate_marker( "Spoils in" ),
            };
            for( size_t i = 1; i < ncells && ( i - 1 ) < ( sizeof( col_titles ) / sizeof( col_titles[0] ) );
                 ++i ) {
                headers.emplace_back( _( col_titles[i - 1] ) );
            }
            while( headers.size() < ncells ) {
                headers.emplace_back( string_format( _( "Col %d" ),
                                                     static_cast<int>( headers.size() ) ) );
            }

            const ImGuiTableFlags tflags =
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable |
                ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Hideable;
            const float table_h = std::max( 120.f, ImGui::GetContentRegionAvail().y - 40.f );
            if( ImGui::BeginTable( "##consume_table", static_cast<int>( headers.size() ) + 1,
                                   tflags, ImVec2( -1.f, table_h ) ) ) {
                ImGui::TableSetupScrollFreeze( 0, 1 );
                ImGui::TableSetupColumn( _( "Cat" ), ImGuiTableColumnFlags_WidthFixed, 90.f );
                for( size_t i = 0; i < headers.size(); ++i ) {
                    const ImGuiTableColumnFlags flags = ( i == 0 )
                                                        ? ImGuiTableColumnFlags_WidthStretch
                                                        : ImGuiTableColumnFlags_WidthFixed;
                    const float width = ( i == 0 ) ? 0.f : 78.f;
                    ImGui::TableSetupColumn( headers[i].c_str(), flags, width );
                }
                ImGui::TableHeadersRow();

                if( visible.empty() ) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s",
                                        _( "No matching items." ) );
                }

                ImGuiListClipper clipper;
                clipper.Begin( static_cast<int>( visible.size() ) );
                while( clipper.Step() ) {
                    for( int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i ) {
                        const row_t &row = *visible[i];
                        ImGui::TableNextRow();
                        ImGui::PushID( i );

                        const bool is_sel = ( i == selected_row );
                        if( is_sel ) {
                            ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0,
                                                    ImGui::GetColorU32(
                                                        ui_hybrid_chrome::palette::grid_selected() ) );
                        }

                        ImGui::TableSetColumnIndex( 0 );
                        ImGui::TextUnformatted( row.category.c_str() );

                        for( size_t c = 0; c < headers.size(); ++c ) {
                            ImGui::TableSetColumnIndex( static_cast<int>( c + 1 ) );
                            const std::string &cell = ( c < row.cells.size() )
                                                      ? row.cells[c] : std::string();
                            if( c == 0 ) {
                                std::string label = remove_color_tags( cell );
                                if( !row.denial.empty() ) {
                                    label = string_format( "%s  [%s]", label,
                                                           remove_color_tags( row.denial ) );
                                }
                                ImGui::PushStyleColor( ImGuiCol_Text,
                                                       row.selectable
                                                       ? ui_hybrid_chrome::palette::text()
                                                       : ui_hybrid_chrome::palette::text_muted() );
                                if( ImGui::Selectable( label.c_str(), is_sel,
                                                       ImGuiSelectableFlags_SpanAllColumns |
                                                       ImGuiSelectableFlags_AllowDoubleClick ) ) {
                                    selected_row = i;
                                    if( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) ) {
                                        try_confirm();
                                    }
                                }
                                ImGui::PopStyleColor();
                                // RMB: same-cell gate + defer OpenPopup after EndTable
                                // (Equipment inventory pattern — avoids ghost/neighbor steal).
                                const bool hovered = ImGui::IsItemHovered();
                                if( hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Right ) ) {
                                    rmb_down_row = static_cast<int>( i );
                                }
                                if( hovered && ImGui::IsMouseReleased( ImGuiMouseButton_Right ) ) {
                                    if( rmb_down_row == static_cast<int>( i ) ) {
                                        selected_row = static_cast<int>( i );
                                        pending_ctx_open = true;
                                    }
                                    rmb_down_row = -1;
                                }
                                if( !row.denial.empty() && ImGui::IsItemHovered() ) {
                                    ImGui::SetTooltip( "%s",
                                                       remove_color_tags( row.denial ).c_str() );
                                }
                            } else if( cell.empty() ) {
                                ImGui::TextUnformatted( "—" );
                            } else {
                                cataimgui::draw_colored_text( cell, c_light_gray );
                            }
                        }
                        ImGui::PopID();
                    }
                }
                ImGui::EndTable();
            }
            if( ImGui::IsMouseReleased( ImGuiMouseButton_Right ) ) {
                rmb_down_row = -1;
            }

            if( pending_ctx_open ) {
                ImGui::OpenPopup( "##consume_row_ctx" );
                pending_ctx_open = false;
            }
            ImGui::SetNextWindowPos( ImGui::GetMousePos(), ImGuiCond_Appearing );
            // NoNav: mouse-only RMB; Deck stick must not scroll the popup.
            if( ImGui::BeginPopup( "##consume_row_ctx", ImGuiWindowFlags_NoNav ) ) {
                ImGui::BringWindowToDisplayFront( ImGui::GetCurrentWindow() );
                if( selected_row >= 0 &&
                    selected_row < static_cast<int>( visible.size() ) ) {
                    row_t &crow = *visible[selected_row];
                    if( crow.entry && crow.entry->is_item() ) {
                        const item_location loc = crow.entry->any_item();
                        if( loc ) {
                            const std::string primary = consume_action_label( *loc );
                            if( ImGui::MenuItem( primary.c_str(), nullptr, false,
                                                 crow.selectable ) ) {
                                ImGui::CloseCurrentPopup();
                                try_confirm();
                            }
                            if( ImGui::MenuItem( _( "Examine" ) ) ) {
                                ImGui::CloseCurrentPopup();
                                // Defer examine until after ImGui::End (nested UI CTD).
                                pending_examine_loc = loc;
                            }
                        }
                    }
                }
                ImGui::EndPopup();
            }

            std::string consume_btn = _( "Consume" );
            if( selected_row >= 0 &&
                selected_row < static_cast<int>( visible.size() ) &&
                visible[selected_row]->entry &&
                visible[selected_row]->entry->is_item() ) {
                const item_location loc = visible[selected_row]->entry->any_item();
                if( loc ) {
                    consume_btn = consume_action_label( *loc );
                }
            }
            // Stable id so Drink/Eat/Take label swaps do not reset button state.
            const int n_ok = ui_hybrid_chrome::push_toolbar_button( false );
            if( ImGui::Button( ( consume_btn + "##consume_ok" ).c_str() ) ) {
                try_confirm();
            }
            ImGui::PopStyleColor( n_ok );
            ImGui::SameLine();
            const int n_cancel = ui_hybrid_chrome::push_toolbar_button( false );
            if( ImGui::Button( _( "Cancel" ) ) ) {
                selected = item_location::nowhere;
                finished = true;
                is_open = false;
            }
            ImGui::PopStyleColor( n_cancel );
            ImGui::SameLine();
            ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s",
                                _( "[Enter] confirm  [Esc] cancel  [RMB] actions  [←/→] category  [/] filter" ) );
        }

    private:
        Character &you;
        consume_collector &collector;
        std::string hint;
        std::string filter_text;
        std::string active_tab;
        std::vector<row_t> rows;
        std::vector<row_t *> visible;
        int selected_row = 0;
        bool finished = false;
        bool filter_focus = false;
        bool pending_ctx_open = false;
        int rmb_down_row = -1;
        item_location pending_examine_loc;
        item_location selected;

        void rebuild_rows() {
            rows.clear();
            const inventory_selector_preset &preset = collector.preset_ref();
            for( inventory_column *col : collector.columns() ) {
                if( col == nullptr ) {
                    continue;
                }
                for( inventory_entry *e : col->get_entries(
                []( const inventory_entry & ent ) {
                return ent.is_item();
                } ) ) {
                    if( e == nullptr || !e->is_item() ) {
                        continue;
                    }
                    if( e->is_collation_entry() ) {
                        continue;
                    }
                    row_t row;
                    row.entry = e;
                    const item_location &loc = e->any_item();
                    if( !loc ) {
                        continue;
                    }
                    e->cache_denial( preset );
                    row.denial = e->denial.value_or( std::string() );
                    row.selectable = e->is_selectable() && row.denial.empty();
                    const item_category *cat = e->get_category_ptr();
                    row.category = cat ? cat->name_header() : std::string();

                    e->make_entry_cell_cache( preset, false );
                    const auto &cache = e->get_entry_cell_cache( preset );
                    row.cells = cache.text;
                    if( !row.cells.empty() ) {
                        row.name = row.cells[0];
                    } else {
                        row.name = loc->display_name();
                        row.cells.push_back( row.name );
                    }
                    rows.push_back( std::move( row ) );
                }
            }
            apply_filters();
        }

        void apply_filters() {
            visible.clear();
            const std::string filter_lc = strip_for_filter( filter_text );
            for( row_t &row : rows ) {
                if( !row.entry || !row.entry->is_item() ) {
                    continue;
                }
                const item &it = *row.entry->any_item();
                if( !tab_match( it, active_tab ) ) {
                    continue;
                }
                if( !filter_match( row, filter_lc ) ) {
                    continue;
                }
                visible.push_back( &row );
            }
            if( selected_row >= static_cast<int>( visible.size() ) ) {
                selected_row = std::max( 0, static_cast<int>( visible.size() ) - 1 );
            }
        }

        void cycle_tab( int dir ) {
            static const char *const order[] = { "ALL", "FOOD", "DRINK", "MED" };
            int idx = 0;
            for( int i = 0; i < 4; ++i ) {
                if( active_tab == order[i] ) {
                    idx = i;
                    break;
                }
            }
            idx = ( idx + dir + 4 ) % 4;
            active_tab = order[idx];
            apply_filters();
        }

        void try_confirm() {
            if( selected_row < 0 || selected_row >= static_cast<int>( visible.size() ) ) {
                return;
            }
            row_t &row = *visible[selected_row];
            if( !row.selectable || !row.entry || !row.entry->is_item() ) {
                return;
            }
            selected = row.entry->any_item();
            finished = true;
            is_open = false;
        }
};

} // namespace

namespace consume_hybrid
{

item_location select( Character &you, const inventory_selector_preset &preset,
                      const std::string &title, const std::string &none_message,
                      const std::string &hint, const item_location &container,
                      const std::string &initial_filter,
                      const std::string &comest_tab )
{
    consume_collector collector( you, preset );
    collector.clear_items();
    if( container ) {
        item_location cont = container;
        collector.add_contained_items( cont );
    } else {
        collector.add_character_items( you );
        collector.add_nearby_items( 1 );
        collector.add_vehicle_tank_items();
    }

    if( collector.empty() ) {
        popup( none_message.empty() ? _( "You have nothing to consume." ) : none_message,
               PF_GET_KEY );
        return item_location::nowhere;
    }

    consume_hybrid_window ui( you, collector, title, hint, initial_filter, comest_tab );

    input_context ctxt( "CONSUME_HYBRID" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "LEFT" );
    ctxt.register_action( "RIGHT" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "SCROLL_UP" );
    ctxt.register_action( "SCROLL_DOWN" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.set_timeout( 10 );

    while( !ui.done() && ui.get_is_open() ) {
        ui_manager::redraw_invalidated();
        ui.flush_deferred();
        const std::string action = ctxt.handle_input();
        if( action != "ERROR" && action != "TIMEOUT" && action != "ANY_INPUT" ) {
            ui.process_action( action );
        }
    }
    return ui.result();
}

} // namespace consume_hybrid

#endif // TILES
