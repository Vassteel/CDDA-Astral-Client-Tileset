#include "construction_hybrid_ui.h"

#include <algorithm>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "avatar.h"
#include "cata_imgui.h"
#include "cata_tiles.h"
#include "cata_utility.h"
#include "character.h"
#include "construction.h"
#include "construction_category.h"
#include "construction_group.h"
#include "game.h"
#include "imgui/imgui.h"
#include <imgui/imgui_internal.h>
#include "input_context.h"
#include "input_popup.h"
#include "inventory.h"
#include "localized_comparator.h"
#include "map.h"
#include "mapdata.h"
#include "output.h"
#include "requirements.h"
#include "sdltiles.h"
#include "skill.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_hybrid_chrome.h"
#include "ui_manager.h"
#include "uistate.h"

static const construction_category_id construction_category_ALL( "ALL" );
static const construction_category_id construction_category_FILTER( "FILTER" );
static const trait_id trait_DEBUG_HS( "DEBUG_HS" );

namespace
{

// 0 = all, 1 = ready (wrong spot), 2 = buildable here
static int g_filter_mode = 0;

// Public-header can_construct needs a point; mirror vanilla nearby check.
bool can_construct_near( const construction &con )
{
    const tripoint_bub_ms avatar_pos = get_player_character().pos_bub();
    for( const tripoint_bub_ms &pt : get_map().points_in_radius( avatar_pos, 1 ) ) {
        if( pt != avatar_pos && can_construct( con, pt ) ) {
            return true;
        }
    }
    return false;
}

bool player_can_build_group( Character &you, const temp_crafting_inventory &inv,
                             const construction_group_str_id &group )
{
    for( const construction *con : constructions_by_group( group ) ) {
        if( player_can_build( you, inv, *con ) ) {
            return true;
        }
    }
    return false;
}

void load_available( std::vector<construction_group_str_id> &available,
                     std::map<construction_category_id, std::vector<construction_group_str_id>> &cat_available,
                     int filter_mode )
{
    cat_available.clear();
    available.clear();
    avatar &pc = get_avatar();
    for( const construction &it : get_constructions() ) {
        if( !it.on_display || it.is_blacklisted() ) {
            continue;
        }
        bool ok = false;
        if( filter_mode == 0 ) {
            ok = true;
        } else if( filter_mode == 1 ) {
            ok = player_can_build( pc, pc.crafting_inventory(), it, true ) && !can_construct_near( it );
        } else if( filter_mode == 2 ) {
            ok = player_can_build( pc, pc.crafting_inventory(), it, true ) && can_construct_near( it );
        }
        if( !ok ) {
            continue;
        }
        bool already = false;
        for( const construction_group_str_id &avail_it : available ) {
            if( avail_it == it.group ) {
                already = true;
                break;
            }
        }
        if( !already ) {
            available.push_back( it.group );
            cat_available[it.category].push_back( it.group );
        }
    }
}

nc_color group_color( const construction_group_str_id &group, bool highlight )
{
    Character &pc = get_player_character();
    nc_color col = c_dark_gray;
    if( pc.has_trait( trait_DEBUG_HS ) ) {
        col = c_white;
    } else if( player_can_build_group( pc, pc.crafting_inventory(), group ) ) {
        col = c_white;
    } else {
        std::vector<const construction *> cons = constructions_by_group( group );
        bool almost = false;
        for( const construction *c : cons ) {
            if( player_can_build( pc, pc.crafting_inventory(), *c, true ) ) {
                almost = true;
                break;
            }
        }
        col = almost ? c_yellow : c_dark_gray;
    }
    return highlight ? hilite( col ) : col;
}

class construction_hybrid_ui : public cataimgui::window
{
    public:
        explicit construction_hybrid_ui( bool blueprint_mode )
            // NoNav: Deck stick/D-pad must not drive ImGui TabBar nav while CDDA
            // LEFT/RIGHT/NEXT_TAB also cycle categories (Consume same pattern).
            : cataimgui::window( _( "Construction" ),
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav ),
              blueprint( blueprint_mode ) {
            set_shell( 0, "build" );
            load_available( available, cat_available, g_filter_mode );
            cats = construction_categories::get_all();
            if( uistate.construction_tab.is_valid() ) {
                for( size_t i = 0; i < cats.size(); ++i ) {
                    if( cats[i].id == uistate.construction_tab ) {
                        tabindex = static_cast<int>( i );
                        break;
                    }
                }
            }
            rebuild_constructs();
        }

        bool is_done() const {
            return done;
        }
        construction_id result() const {
            return ret;
        }

        void pump_input( input_context &ctxt ) {
            // Short timeout so ImGui keeps receiving frames while SDL mouse
            // events are forwarded; without this, WantCaptureMouse swallows
            // clicks/scroll and handle_input blocks until a keyboard action,
            // so Buttons/Selectables/scrollbars never activate.
            ui_manager::redraw_invalidated();
            // Run deferred actions AFTER ImGui::End for this window — filter /
            // confirm popups nest redraws safely when not mid-Begin/End.
            process_pending();
            std::string action;
            if( has_button_action() ) {
                action = get_button_action();
            } else {
                action = ctxt.handle_input();
            }
            if( action.empty() || action == "ERROR" || action == "TIMEOUT" ||
                action == "ANY_INPUT" ) {
                return;
            }
            handle_key( action );
        }

    protected:
        void draw() override {
            ui_hybrid_chrome::push();
            cataimgui::window::draw();
            ui_hybrid_chrome::pop();
        }

        cataimgui::bounds get_bounds() override {
            return { -1.f, -1.f, ui_hybrid_chrome::theme::large_window_size().x,
                     ui_hybrid_chrome::theme::large_window_size().y };
        }

        void draw_controls() override {
            hide_if_hidden();
            if( !get_is_open() ) {
                done = true;
                return;
            }

            draw_filter_buttons();
            draw_category_tabs();

            const float footer_h = ImGui::GetFrameHeightWithSpacing() * 2.2f;
            if( ImGui::BeginChild( "##CON_BODY", ImVec2( 0, -footer_h ), ImGuiChildFlags_None ) ) {
                const float avail_w = ImGui::GetContentRegionAvail().x;
                const float list_w = avail_w * 0.38f;
                if( ImGui::BeginChild( "##CON_LIST", ImVec2( list_w, 0 ), ImGuiChildFlags_Borders ) ) {
                    draw_list();
                }
                ImGui::EndChild();
                ImGui::SameLine();
                if( ImGui::BeginChild( "##CON_DETAIL", ImVec2( 0, 0 ), ImGuiChildFlags_Borders ) ) {
                    draw_detail();
                }
                ImGui::EndChild();
            }
            ImGui::EndChild();

            // OpenPopup / BeginPopup on parent (outside list child) — Equipment pattern.
            if( pending_ctx_open ) {
                ImGui::OpenPopup( "##con_row_ctx" );
                pending_ctx_open = false;
            }
            ImGui::SetNextWindowPos( ImGui::GetMousePos(), ImGuiCond_Appearing );
            if( ImGui::BeginPopup( "##con_row_ctx", ImGuiWindowFlags_NoNav ) ) {
                ImGui::BringWindowToDisplayFront( ImGui::GetCurrentWindow() );
                const bool can_confirm = !constructs.empty() && select >= 0 &&
                                         select < static_cast<int>( constructs.size() );
                if( ImGui::MenuItem( _( "Construct" ), nullptr, false, can_confirm ) ) {
                    ImGui::CloseCurrentPopup();
                    pending_confirm = true;
                }
                if( ImGui::MenuItem( _( "Search…" ) ) ) {
                    ImGui::CloseCurrentPopup();
                    filter_focus = true;
                }
                ImGui::EndPopup();
            }

            draw_footer();
            // NOTE: process_pending() must NOT run inside draw_controls.
            // string_input_popup / popup() call ui_manager::redraw which re-enters
            // this window's ImGui::Begin/End and SIGSEGVs in ImGui::End().
        }

    private:
        bool blueprint = false;
        bool done = false;
        construction_id ret = construction_id( -1 );

        std::vector<construction_group_str_id> available;
        std::map<construction_category_id, std::vector<construction_group_str_id>> cat_available;
        std::vector<construction_category> cats;
        std::vector<construction_group_str_id> constructs;
        int tabindex = 0;
        int select = 0;
        int stage_idx = 0;
        std::string filter;
        bool force_tab = true;
        bool filter_focus = false;
        bool search_active = false;

        int pending_tab = -1;
        int pending_select = -1;
        bool pending_confirm = false;
        bool pending_quit = false;
        bool pending_reset_filter = false;
        int pending_filter_mode = -1;
        int pending_stage_delta = 0;
        // RMB context (Equipment same-cell gate + parent OpenPopup).
        bool pending_ctx_open = false;
        int rmb_down_row = -1;

        void apply_filter_text( const std::string &new_filter ) {
            if( new_filter == filter ) {
                return;
            }
            filter = new_filter;
            if( !filter.empty() ) {
                for( size_t i = 0; i < cats.size(); ++i ) {
                    if( cats[i].id == construction_category_FILTER ) {
                        if( tabindex != static_cast<int>( i ) ) {
                            tabindex = static_cast<int>( i );
                            force_tab = true;
                        }
                        break;
                    }
                }
            } else if( !cats.empty() && cats[tabindex].id == construction_category_FILTER ) {
                tabindex = 0;
                force_tab = true;
            }
            select = 0;
            rebuild_constructs();
        }

        // Skip hidden FILTER tab when search is empty.
        int next_visible_tab( int delta ) const {
            if( cats.empty() ) {
                return 0;
            }
            const int n = static_cast<int>( cats.size() );
            int t = tabindex;
            for( int step = 0; step < n; ++step ) {
                t = ( t + delta + n ) % n;
                if( cats[t].id == construction_category_FILTER && filter.empty() ) {
                    continue;
                }
                return t;
            }
            return tabindex;
        }

        bool input_blocked_by_search() const {
            return search_active || cataimgui::client::want_text_input();
        }

        void rebuild_constructs() {
            constructs.clear();
            if( cats.empty() ) {
                return;
            }
            if( tabindex < 0 || tabindex >= static_cast<int>( cats.size() ) ) {
                tabindex = 0;
            }
            const construction_category_id &cid = cats[tabindex].id;
            if( cid == construction_category_ALL ) {
                constructs = available;
            } else if( cid == construction_category_FILTER ) {
                for( const construction_group_str_id &g : available ) {
                    if( lcmatch( g->name(), filter ) ) {
                        constructs.push_back( g );
                    }
                }
            } else {
                constructs = cat_available[cid];
            }
            if( select >= static_cast<int>( constructs.size() ) ) {
                select = std::max( 0, static_cast<int>( constructs.size() ) - 1 );
            }
            stage_idx = 0;
        }

        void draw_filter_buttons() {
            ui_hybrid_chrome::section_header( _( "Build" ) );
            const char *labels[3] = {
                translate_marker( "All" ),
                translate_marker( "Ready (wrong spot)" ),
                translate_marker( "Buildable here" )
            };
            for( int i = 0; i < 3; ++i ) {
                if( i > 0 ) {
                    ImGui::SameLine();
                }
                const int n = ui_hybrid_chrome::push_toolbar_button( g_filter_mode == i );
                if( ImGui::Button( _( labels[i] ) ) ) {
                    pending_filter_mode = i;
                }
                ImGui::PopStyleColor( n );
            }
            ImGui::SameLine();
            {
                const int n = ui_hybrid_chrome::push_toolbar_button( false );
                if( ImGui::Button( _( "Search…" ) ) ) {
                    filter_focus = true;
                }
                ImGui::PopStyleColor( n );
            }
            // Inline filter (Consume pattern). Avoids nested string_input_popup
            // and keeps the FILTER category tab in sync while typing.
            ImGui::SameLine();
            ImGui::SetNextItemWidth( 220.f );
            if( filter_focus ) {
                ImGui::SetKeyboardFocusHere();
                filter_focus = false;
            }
            char buf[256];
            std::snprintf( buf, sizeof( buf ), "%s", filter.c_str() );
            if( ImGui::InputTextWithHint( "##con_filter", _( "Search constructions…" ), buf,
                                          sizeof( buf ) ) ) {
                apply_filter_text( buf );
            }
            search_active = ImGui::IsItemActive();
            if( !filter.empty() ) {
                ImGui::SameLine();
                const int n = ui_hybrid_chrome::push_toolbar_button( false );
                if( ImGui::Button( _( "Clear search" ) ) ) {
                    pending_reset_filter = true;
                }
                ImGui::PopStyleColor( n );
            }
        }

        void draw_category_tabs() {
            if( !ImGui::BeginTabBar( "##CON_CATS", ImGuiTabBarFlags_FittingPolicyScroll ) ) {
                return;
            }
            // ImGui may still report the old tab active while a programmatic
            // switch is pending. Keep the requested index stable throughout
            // this loop, including when Search creates a new FILTER tab.
            const int requested_tab = force_tab ? tabindex : -1;
            bool requested_tab_active = false;
            for( size_t i = 0; i < cats.size(); ++i ) {
                if( cats[i].id == construction_category_FILTER && filter.empty() ) {
                    continue;
                }
                const bool should = requested_tab == static_cast<int>( i );
                // Stable ImGui IDs (###) so changing "Search: …" label does not
                // remount the FILTER tab and fight selection every keystroke.
                std::string tab_id;
                if( cats[i].id == construction_category_FILTER ) {
                    const std::string label = string_format( _( "Search: %s" ), filter );
                    tab_id = string_format( "%s###CON_FILTER", label.c_str() );
                } else {
                    tab_id = string_format( "%s###CON_CAT_%s", cats[i].name().c_str(),
                                            cats[i].id.c_str() );
                }
                // Search is created while typing; keep it before the regular
                // categories instead of appending it to ImGui's tab list.
                const ImGuiTabItemFlags flags = cats[i].id == construction_category_FILTER ?
                                                ImGuiTabItemFlags_Leading : ImGuiTabItemFlags_None;
                if( cataimgui::BeginTabItem( tab_id.c_str(), should, nullptr, flags ) ) {
                    requested_tab_active = requested_tab_active || should;
                    if( requested_tab < 0 && tabindex != static_cast<int>( i ) ) {
                        // ImGui click: apply immediately (Character sheet pattern).
                        // Do NOT set force_tab — that SetSelected loop was fighting
                        // ImGui/nav and flipping categories rapidly on Deck.
                        tabindex = static_cast<int>( i );
                        select = 0;
                        rebuild_constructs();
                    }
                    ImGui::EndTabItem();
                }
            }
            force_tab = requested_tab >= 0 && !requested_tab_active;
            ImGui::EndTabBar();
        }

        void draw_list() {
            if( constructs.empty() ) {
                ImGui::TextDisabled( "%s", _( "Nothing in this category." ) );
                return;
            }
            ImGuiListClipper clipper;
            clipper.Begin( static_cast<int>( constructs.size() ) );
            while( clipper.Step() ) {
                for( int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i ) {
                    ImGui::PushID( i );
                    const nc_color col = group_color( constructs[i], i == select );
                    ImGui::PushStyleColor( ImGuiCol_Text, cataimgui::imvec4_from_color( col ) );
                    if( ImGui::Selectable( constructs[i]->name().c_str(), i == select ) ) {
                        pending_select = i;
                    }
                    const bool hovered = ImGui::IsItemHovered();
                    if( hovered && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) ) {
                        pending_select = i;
                        pending_confirm = true;
                    }
                    // Same-cell RMB gate (Equipment inventory pattern).
                    if( hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Right ) ) {
                        rmb_down_row = i;
                    }
                    if( hovered && ImGui::IsMouseReleased( ImGuiMouseButton_Right ) ) {
                        if( rmb_down_row == i ) {
                            pending_select = i;
                            pending_ctx_open = true;
                        }
                        rmb_down_row = -1;
                    }
                    ImGui::PopStyleColor();
                    ImGui::PopID();
                }
            }
            if( ImGui::IsMouseReleased( ImGuiMouseButton_Right ) ) {
                rmb_down_row = -1;
            }
        }

        void draw_detail() {
            if( constructs.empty() || select < 0 ||
                select >= static_cast<int>( constructs.size() ) ) {
                ImGui::TextDisabled( "%s", _( "Select a construction." ) );
                return;
            }
            const construction_group_str_id &group = constructs[select];
            ui_hybrid_chrome::section_header( group->name().c_str() );

            avatar &pc = get_avatar();
            const temp_crafting_inventory &inv = pc.crafting_inventory();
            std::vector<const construction *> options = constructions_by_group( group );

            std::vector<const construction *> stages;
            for( const construction *c : options ) {
                if( g_filter_mode == 1 ) {
                    if( !( player_can_build( pc, inv, *c, true ) && !can_construct_near( *c ) ) ) {
                        continue;
                    }
                } else if( g_filter_mode == 2 ) {
                    if( !( player_can_build( pc, inv, *c, true ) && can_construct_near( *c ) ) ) {
                        continue;
                    }
                }
                stages.push_back( c );
            }
            if( stages.empty() ) {
                stages = options;
            }
            if( stages.empty() ) {
                ImGui::TextDisabled( "%s", _( "No stages." ) );
                return;
            }
            if( stage_idx < 0 || stage_idx >= static_cast<int>( stages.size() ) ) {
                stage_idx = 0;
            }

            if( stages.size() > 1 ) {
                ImGui::Text( "%s", string_format( _( "Stage %d / %d" ), stage_idx + 1,
                                                  static_cast<int>( stages.size() ) ).c_str() );
                ImGui::SameLine();
                {
                    const int n = ui_hybrid_chrome::push_toolbar_button( false );
                    ImGui::BeginDisabled( stage_idx <= 0 );
                    if( ImGui::Button( _( "Prev stage" ) ) ) {
                        pending_stage_delta = -1;
                    }
                    ImGui::EndDisabled();
                    ImGui::PopStyleColor( n );
                }
                ImGui::SameLine();
                {
                    const int n = ui_hybrid_chrome::push_toolbar_button( false );
                    ImGui::BeginDisabled( stage_idx >= static_cast<int>( stages.size() ) - 1 );
                    if( ImGui::Button( _( "Next stage" ) ) ) {
                        pending_stage_delta = 1;
                    }
                    ImGui::EndDisabled();
                    ImGui::PopStyleColor( n );
                }
            }

            const construction *con = stages[stage_idx];
            con->requirements->can_make_with_inventory( &pc, inv, is_crafting_component, 1,
                    craft_flags::none, false );

            if( !con->post_terrain.empty() ) {
                std::string result_name;
                std::string result_desc;
                if( con->post_is_furniture ) {
                    const furn_str_id fid( con->post_terrain );
                    result_name = fid.obj().name();
                    result_desc = fid.obj().description.translated();
                } else {
                    const ter_str_id tid( con->post_terrain );
                    result_name = tid.obj().name();
                    result_desc = tid.obj().description.translated();
                }
                cataimgui::draw_colored_text( string_format( _( "Result: %s" ), result_name ),
                                              c_light_green );
                if( !result_desc.empty() ) {
                    cataimgui::draw_colored_text( result_desc, c_light_gray );
                }
            }

            if( !con->pre_note.empty() ) {
                cataimgui::draw_colored_text( con->pre_note.translated(), c_light_gray );
            }

            ui_hybrid_chrome::section_header( _( "Skills" ) );
            if( con->required_skills.empty() ) {
                ImGui::TextDisabled( "%s", _( "N/A" ) );
            } else {
                for( const auto &sk : con->required_skills ) {
                    const int lvl = pc.get_skill_level( sk.first );
                    const nc_color col = lvl >= sk.second ? c_light_green : c_red;
                    cataimgui::draw_colored_text(
                        string_format( "%s %d/%d", sk.first->name(), lvl, sk.second ), col );
                }
            }

            const float wrap = ImGui::GetContentRegionAvail().x;
            const int wrap_chars = std::max( 20,
                                             static_cast<int>( wrap / ImGui::CalcTextSize( "X" ).x ) );

            ui_hybrid_chrome::section_header( _( "Time" ) );
            for( const std::string &tline : con->get_folded_time_string( wrap_chars ) ) {
                cataimgui::draw_colored_text( tline, c_white );
            }

            ui_hybrid_chrome::section_header( _( "Tools" ) );
            for( const std::string &line :
                 con->requirements->get_folded_tools_list( &pc, wrap_chars, c_white, inv, 1 ) ) {
                cataimgui::draw_colored_text( line, c_white );
            }

            ui_hybrid_chrome::section_header( _( "Components" ) );
            for( const std::string &line :
                 con->requirements->get_folded_components_list( &pc, wrap_chars, c_white, inv,
                         is_crafting_component, 1 ) ) {
                cataimgui::draw_colored_text( line, c_white );
            }

            const bool can = player_can_build_group( pc, inv, group );
            const bool here = can_construct_near( *con );
            ImGui::Spacing();
            if( can && here ) {
                cataimgui::draw_colored_text( _( "You can build this here." ), c_light_green );
            } else if( can ) {
                cataimgui::draw_colored_text( _( "Materials/skills OK — find a valid location." ),
                                              c_yellow );
            } else {
                cataimgui::draw_colored_text( _( "Missing skills, tools, or materials." ), c_red );
            }
        }

        void draw_footer() {
            ImGui::Separator();
            {
                const int n = ui_hybrid_chrome::push_toolbar_button( false );
                if( ImGui::Button( _( "Construct" ), ImVec2( 140.f, 0 ) ) ) {
                    pending_confirm = true;
                }
                ImGui::PopStyleColor( n );
            }
            ImGui::SameLine();
            {
                const int n = ui_hybrid_chrome::push_toolbar_button( false );
                if( ImGui::Button( _( "Cancel" ), ImVec2( 120.f, 0 ) ) ) {
                    pending_quit = true;
                }
                ImGui::PopStyleColor( n );
            }
            ImGui::SameLine();
            ImGui::TextDisabled( "%s",
                                 _( "Tabs: categories · Double-click / RMB / Construct to build" ) );
        }

        void process_pending() {
            if( pending_filter_mode >= 0 ) {
                g_filter_mode = pending_filter_mode;
                pending_filter_mode = -1;
                load_available( available, cat_available, g_filter_mode );
                rebuild_constructs();
            }
            if( pending_tab >= 0 ) {
                tabindex = pending_tab;
                pending_tab = -1;
                select = 0;
                rebuild_constructs();
                force_tab = true;
            }
            if( pending_select >= 0 ) {
                select = pending_select;
                pending_select = -1;
                stage_idx = 0;
            }
            if( pending_stage_delta != 0 ) {
                stage_idx += pending_stage_delta;
                pending_stage_delta = 0;
            }
            if( pending_reset_filter ) {
                pending_reset_filter = false;
                apply_filter_text( "" );
            }
            if( pending_quit ) {
                pending_quit = false;
                done = true;
                ret = construction_id( -1 );
            }
            if( pending_confirm ) {
                pending_confirm = false;
                try_confirm();
            }
        }

        void try_confirm() {
            if( constructs.empty() || select < 0 ||
                select >= static_cast<int>( constructs.size() ) ) {
                return;
            }
            avatar &pc = get_avatar();
            const construction_group_str_id &group = constructs[select];
            if( blueprint ) {
                const std::vector<construction> &list = get_constructions();
                for( int i = 0; i < static_cast<int>( list.size() ); ++i ) {
                    if( group == list[i].group ) {
                        ret = construction_id( i );
                        break;
                    }
                }
                uistate.last_construction = group;
                uistate.construction_tab = cats[tabindex].id;
                done = true;
                return;
            }
            if( !player_can_build_group( pc, pc.crafting_inventory(), group ) ) {
                popup( _( "You can't build that!" ) );
                return;
            }
            if( !g->warn_player_maybe_anger_local_faction( true ) ) {
                return;
            }
            is_open = false;
            uistate.last_construction = group;
            uistate.construction_tab = cats[tabindex].id;
            place_construction( { group } );
            done = true;
            ret = construction_id( -1 );
        }

        void handle_key( const std::string &action ) {
            // While the inline search box owns the keyboard, do not let CDDA
            // keybinds steal letters / arrows / tab (was flipping categories
            // while typing and making Search appear broken).
            // Allow QUIT to still close the Build menu while search is focused.
            if( input_blocked_by_search() && action != "QUIT" ) {
                return;
            }
            if( action == "QUIT" ) {
                pending_quit = true;
            } else if( action == "CONFIRM" ) {
                pending_confirm = true;
            } else if( action == "FILTER" ) {
                filter_focus = true;
            } else if( action == "RESET_FILTER" ) {
                pending_reset_filter = true;
            } else if( action == "TOGGLE_UNAVAILABLE_CONSTRUCTIONS" ) {
                pending_filter_mode = ( g_filter_mode + 1 ) % 3;
            } else if( action == "NEXT_TAB" || action == "RIGHT" ) {
                pending_tab = next_visible_tab( +1 );
            } else if( action == "PREV_TAB" || action == "LEFT" ) {
                pending_tab = next_visible_tab( -1 );
            } else if( action == "UP" || action == "SCROLL_UP" ) {
                if( !constructs.empty() ) {
                    pending_select = select <= 0 ?
                                     static_cast<int>( constructs.size() ) - 1 : select - 1;
                }
            } else if( action == "DOWN" || action == "SCROLL_DOWN" ) {
                if( !constructs.empty() ) {
                    pending_select = ( select + 1 ) % static_cast<int>( constructs.size() );
                }
            } else if( action == "SCROLL_STAGE_UP" ) {
                pending_stage_delta = -1;
            } else if( action == "SCROLL_STAGE_DOWN" ) {
                pending_stage_delta = 1;
            } else if( action == "PAGE_UP" ) {
                pending_select = std::max( 0, select - 10 );
            } else if( action == "PAGE_DOWN" ) {
                if( !constructs.empty() ) {
                    pending_select = std::min( static_cast<int>( constructs.size() ) - 1,
                                               select + 10 );
                }
            }
        }

};

construction_id construction_menu_hybrid_impl( const bool blueprint )
{
    std::vector<construction_group_str_id> available;
    std::map<construction_category_id, std::vector<construction_group_str_id>> cat_available;
    load_available( available, cat_available, g_filter_mode );
    if( available.empty() ) {
        const int saved = g_filter_mode;
        g_filter_mode = 0;
        load_available( available, cat_available, g_filter_mode );
        if( available.empty() ) {
            g_filter_mode = saved;
            popup( _( "You can not construct anything here." ) );
            return construction_id( -1 );
        }
    }

#if defined( TILES )
    tilecontext->set_disable_occlusion( true );
    g->invalidate_main_ui_adaptor();
#endif

    input_context ctxt( "CONSTRUCTION" );
    ctxt.register_navigate_ui_list();
    ctxt.register_leftright();
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "SCROLL_STAGE_UP" );
    ctxt.register_action( "SCROLL_STAGE_DOWN" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "TOGGLE_UNAVAILABLE_CONSTRUCTIONS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );

    ctxt.register_action( "ANY_INPUT" );
    ctxt.set_timeout( 10 );

    construction_hybrid_ui ui( blueprint );
    while( !ui.is_done() && ui.get_is_open() ) {
        ui.pump_input( ctxt );
    }

#if defined( TILES )
    tilecontext->set_disable_occlusion( false );
    g->invalidate_main_ui_adaptor();
#endif

    return ui.result();
}

} // namespace

construction_id construction_menu_hybrid( const bool blueprint )
{
    return construction_menu_hybrid_impl( blueprint );
}
