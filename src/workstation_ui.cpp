#include "workstation_ui.h"

#include <algorithm>
#include <optional>
#include <limits>
#include <string>
#include <vector>

#include "action.h"
#include "line.h"
#include "activity_actor_definitions.h"
#include "avatar.h"
#include "calendar.h"
#include "flag.h"
#include "game.h"
#include "game_inventory.h"
#include "pickup.h"
#include "iexamine.h"
#include "input_context.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "handle_liquid.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "options.h"
#include "output.h"
#include "player_activity.h"
#include "popup.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "translations.h"
#include "uilist.h"
#include "ui_manager.h"
#include "ui_telemetry.h"
#include "vehicle.h"
#include "vpart_position.h"
#if defined(TILES)
#include "cata_imgui.h"
#include "imgui/imgui.h"
#include "ui_hybrid_chrome.h"
#include "ui_hybrid_window.h"
#endif

namespace workstation_ui
{
namespace
{
bool requested = false;
bool closing = false;
unsigned menus_shown = 0;
std::optional<tripoint_abs_ms> active_station;
std::optional<tripoint_abs_ms> return_to_station;
constexpr int load_items = 10001;
constexpr int unload_items = 10002;
constexpr int operate = 10003;
constexpr int craft = 10004;

bool accessible( const tripoint_bub_ms &where )
{
    return where.z() == get_avatar().pos_bub().z() &&
           rl_dist( where, get_avatar().pos_bub() ) <= 1 &&
           !get_map().impassable_field_at( where );
}

bool has_native_controls( const tripoint_bub_ms &where )
{
    const optional_vpart_position vp = get_map().veh_at( where );
    if( vp && vp->vehicle().is_appliance() ) {
        return true;
    }
    const furn_t &f = get_map().furn( where ).obj();
    const map_stack contents = get_map().i_at( where );
    const bool loaded_liquid_station = !contents.empty() &&
                                       ( f.has_examine( iexamine::keg ) || f.has_examine( iexamine::fvat_empty ) ||
                                         f.has_examine( iexamine::compost_empty ) );
    return loaded_liquid_station || f.has_examine( iexamine::smoker_options ) ||
           f.has_examine( iexamine::quern_examine ) ||
           f.has_examine( iexamine::fireplace ) ||
           get_map().ter( where ).obj().has_examine( iexamine::fireplace );
}

void examine( const tripoint_bub_ms &where )
{
    map &here = get_map();
    const optional_vpart_position vp = here.veh_at( where );
    if( vp && vp->vehicle().is_appliance() ) {
        g->exam_appliance( vp->vehicle(), vp->mount_pos() );
    } else if( here.furn( where ).obj().can_examine( where ) ) {
        here.furn( where ).obj().examine( get_avatar(), where );
    } else if( here.ter( where ).obj().can_examine( where ) ) {
        here.ter( where ).obj().examine( get_avatar(), where );
    }
}

std::string primary_label( const tripoint_bub_ms &where )
{
    const furn_t &f = get_map().furn( where ).obj();
    if( f.has_examine( iexamine::kiln_empty ) ) {
        return _( "Light kiln" );
    }
    if( f.has_examine( iexamine::kiln_full ) || f.has_examine( iexamine::arcfurnace_full ) ||
        f.has_examine( iexamine::autoclave_full ) || f.has_examine( iexamine::stook_full ) ) {
        return _( "Check progress / finish cycle" );
    }
    if( f.has_examine( iexamine::arcfurnace_empty ) ||
        f.has_examine( iexamine::autoclave_empty ) || f.has_examine( iexamine::stook_empty ) ) {
        return _( "Start processing" );
    }
    if( f.has_examine( iexamine::fvat_empty ) ) {
        return _( "Load brew / start fermentation" );
    }
    if( f.has_examine( iexamine::fvat_full ) ) {
        return _( "Check fermentation / bottle contents" );
    }
    if( f.has_examine( iexamine::compost_empty ) || f.has_examine( iexamine::compost_full ) ) {
        return _( "Load / process / collect biogas" );
    }
    if( f.has_examine( iexamine::reload_furniture ) ) {
        return _( "Reload fuel or power" );
    }
    if( f.has_examine( iexamine::keg ) ) {
        return _( "Fill / dispense liquid" );
    }
    return _( "Station controls" );
}

#if defined(TILES)
class station_window : public hybrid_window
{
    public:
        station_window( std::function<void()> draw, float height ) : hybrid_window( _( "Workstation" ),
                    std::move( draw ) ), desired_height( height ) {}
    protected:
        cataimgui::bounds get_bounds() override {
            const ImVec2 vp = ImGui::GetMainViewport()->Size;
            const float scale = std::max( 1.f, ImGui::GetFontSize() / 16.f );
            return { -1.f, -1.f, std::min( vp.x * 0.94f, 900.f * scale ),
                     std::min( vp.y * 0.90f, desired_height * scale ) };
        }
    private:
        float desired_height;
};

void draw_station( const tripoint_bub_ms &where, const std::string &details = "" )
{
    map &here = get_map();
    ImGui::TextUnformatted( name( where ).c_str() );
    const furn_t &f = here.furn( where ).obj();
    std::optional<time_duration> remaining;
    if( !here.i_at( where ).empty() ) {
        const item &first = *here.i_at( where ).begin();
        if( f.has_examine( iexamine::kiln_full ) ) {
            remaining = 24_hours - first.age();
        } else if( f.has_examine( iexamine::arcfurnace_full ) ) {
            remaining = 2_hours - first.age();
        } else if( f.has_examine( iexamine::autoclave_full ) ) {
            remaining = 90_minutes - first.age();
        } else if( f.has_examine( iexamine::stook_full ) ) {
            remaining = 336_hours - first.age();
        } else if( f.has_examine( iexamine::compost_full ) && first.is_compostable() ) {
            remaining = first.composting_time() - first.age();
        } else if( f.has_examine( iexamine::fvat_full ) && first.is_brewable() ) {
            remaining = first.brewing_time() - first.age();
        }
    }
    for( const item &it : here.i_at( where ) ) {
        if( it.typeId() == itype_id( "fake_milling_item" ) ||
            it.typeId() == itype_id( "fake_smoke_plume" ) ) {
            remaining = time_duration::from_turns( it.item_counter );
        }
    }
    if( remaining ) {
        ImGui::TextWrapped( "%s", *remaining > 0_turns ?
                            string_format( _( "Processing: %s remaining" ), to_string_clipped( *remaining ) ).c_str() :
                            _( "Processing complete. Finish the cycle to collect the output." ) );
    }
    ImGui::Separator();
    ImGui::BeginChild( "contents", ImVec2( 0.f, ImGui::GetTextLineHeightWithSpacing() * 5.f ),
                       ImGuiChildFlags_Borders );
    if( !details.empty() ) {
        ImGui::TextWrapped( "%s", remove_color_tags( details ).c_str() );
    } else if( here.i_at( where ).empty() ) {
        ImGui::TextDisabled( "%s", _( "Empty" ) );
    }
    for( const item &it : here.i_at( where ) ) {
        if( it.typeId() == itype_id( "fake_milling_item" ) ||
            it.typeId() == itype_id( "fake_smoke_plume" ) ) {
            continue;
        }
        ImGui::TextWrapped( "%s", remove_color_tags( it.display_name() ).c_str() );
    }
    ImGui::EndChild();
}

std::string source_name( const item_location &loc )
{
    if( loc.has_parent() ) {
        return remove_color_tags( loc.parent_item()->type_name() );
    }
    if( loc.held_by( get_avatar() ) ) {
        return _( "Inventory" );
    }
    const tripoint_bub_ms p = loc.pos_bub( get_map() );
    return get_map().has_furn( p ) ? get_map().furnname( p ) : loc.describe( &get_avatar() );
}

drop_locations choose_items( const tripoint_bub_ms &where, bool loading,
                             const std::function<int( const item & )> &limit, int radius )
{
    map &here = get_map();
    avatar &you = get_avatar();
    std::vector<std::vector<item_location>> items;
    const auto add = [&]( const item_location & loc ) {
        if( !loc || limit( *loc ) <= 0 || ( loading && ( !you.can_drop( *loc ).success() ||
                                            loc->made_of( phase_id::LIQUID ) ||
                                            !iexamine::furniture_accepts_item( where, *loc ).value_or( true ) ) ) ) {
            return;
        }
        for( item_location parent = loc; parent.has_parent(); parent = parent.parent_item() ) {
            if( parent.parent_pocket()->sealed() ) {
                return;
            }
        }
        for( auto &stack : items ) {
            if( stack.front()->display_stacked_with( *loc ) &&
                source_name( stack.front() ) == source_name( loc ) ) {
                stack.push_back( loc );
                return;
            }
        }
        items.push_back( { loc } );
    };
    if( loading ) {
        for( const item_location &loc : you.all_items_loc() ) {
            if( !you.is_worn( *loc ) && !you.is_wielding( *loc ) ) {
                add( loc );
            }
        }
        for( const tripoint_bub_ms &p : here.points_in_radius( you.pos_bub(), radius ) ) {
            if( p != where && !here.has_flag( ter_furn_flag::TFLAG_SEALED, p ) &&
                here.clear_path( you.pos_bub(), p, radius, 1, 100 ) ) {
                std::function<void( item_location )> add_contents;
                add_contents = [&]( item_location loc ) {
                    if( loc.has_parent() && loc.parent_pocket()->sealed() ) {
                        return;
                    }
                    add( loc );
                    for( item *child : loc->all_items_top( pocket_type::CONTAINER ) ) {
                        add_contents( item_location( loc, child ) );
                    }
                };
                const auto all_items = []( const item & ) {
                    return true;
                };
                for( const item_location &loc : here.items_with( p, all_items ) ) {
                    add_contents( loc );
                }
            }
        }
    } else {
        for( item &it : here.i_at( where ) ) {
            add( item_location( map_cursor( where ), &it ) );
        }
    }
    const auto count = [&]( int row ) {
        int total = 0;
        for( const item_location &loc : items[row] ) {
            if( loc ) {
                total += loc->count();
            }
        }
        return items[row].front() ? std::min( total, limit( *items[row].front() ) ) : 0;
    };
    int selected = items.empty() ? -1 : 0;
    int quantity = 1;
    bool submit = false;
    bool back = false;
    input_context ctxt( "WORKSTATION" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    ctxt.set_timeout( 16 );
    station_window window( [&]() {
        draw_station( where );
        ImGui::TextWrapped( "%s", loading ? _( "Load materials: only compatible solid items are shown." ) :
                            _( "Unload contents" ) );
        const float body_height = std::max( 80.f, ImGui::GetContentRegionAvail().y -
                                            ImGui::GetFrameHeightWithSpacing() * 4.f );
        ImGui::BeginChild( "transfer_items", ImVec2( 0.f, body_height ), ImGuiChildFlags_Borders );
        for( size_t i = 0; i < items.size(); ++i ) {
            if( items[i].empty() || !items[i].front() ) {
                continue;
            }
            ImGui::PushID( static_cast<int>( i ) );
            const std::string label = string_format( "%s × %d — %s",
                                      remove_color_tags( items[i].front()->tname( 1 ) ), count( i ),
                                      source_name( items[i].front() ) );
            if( ImGui::Selectable( label.c_str(), selected == static_cast<int>( i ) ) ) {
                selected = static_cast<int>( i );
                quantity = 1;
            }
            ImGui::PopID();
        }
        if( items.empty() ) {
            ImGui::TextWrapped( "%s", _( "No eligible items available." ) );
        }
        ImGui::EndChild();
        const bool valid = selected >= 0 && !items[selected].empty() && items[selected].front();
        ImGui::BeginDisabled( !valid );
        ImGui::SetNextItemWidth( 180.f );
        ImGui::InputInt( _( "Quantity" ), &quantity );
        if( valid ) {
            quantity = std::clamp( quantity, 1, std::max( 1, count( selected ) ) );
        }
        if( ImGui::Button( loading ? _( "Load selected" ) : _( "Unload selected" ) ) ) {
            submit = true;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if( ImGui::Button( _( "Back" ) ) ) {
            back = true;
        }
    }, std::clamp( 320.f + items.size() * 22.f, 400.f, 660.f ) );
    while( window.get_is_open() && !submit && !back ) {
        ui_manager::redraw_invalidated();
        const std::string action = ctxt.handle_input();
        if( action == "QUIT" ) {
            break;
        }
        if( action == "CONFIRM" && selected >= 0 && !ImGui::GetIO().WantTextInput ) {
            submit = true;
        }
    }
    if( !window.get_is_open() ) {
        closing = true;
    }
    window.set_hidden( true );
    if( !submit || selected < 0 || !items[selected].front() || !accessible( where ) ) {
        return {};
    }
    item_location loc = items[selected].front();
    quantity = std::clamp( quantity, 1, std::max( 1, count( selected ) ) );
    ui_telemetry::record( "workstation.transfer", {{ "station", name( where ) },
        { "direction", loading ? "load" : "unload" }, { "item", loc->typeId().str() },
        { "quantity", std::to_string( quantity ) }
    } );
    drop_locations selected_items;

    for( const item_location &candidate : items[selected] ) {
        if( !candidate || quantity <= 0 ) {
            continue;
        }
        const int amount = std::min( quantity, candidate->count() );
        selected_items.emplace_back( candidate, amount );

        quantity -= amount;
    }
    return selected_items;
}

void transfer( const tripoint_bub_ms &where, bool loading )
{
    map &here = get_map();
    avatar &you = get_avatar();
    if( get_option<bool>( "SCREEN_READER_MODE" ) ) {
        if( loading ) {
            const item_location_filter filter = [&]( const item_location & loc ) {
                return loc && !loc->made_of( phase_id::LIQUID ) && you.can_drop( *loc ).success() &&
                       ( loc.held_by( you ) || loc.pos_bub( here ) != where ) &&
                       iexamine::furniture_accepts_item( where, *loc ).value_or( true );
            };
            const drop_locations chosen = game_menus::inv::titled_multi_filter_menu(
                                              filter, you, _( "Load materials" ), 1, _( "No suitable materials." ), true );
            if( !chosen.empty() ) {
                you.drop( chosen, where );
            }
        } else {
            g->pickup( where );
        }
        return;
    }
    const drop_locations selected = choose_items( where, loading,
    []( const item & ) {
        return std::numeric_limits<int>::max();
    }, 1 );
    if( selected.empty() ) {
        return;
    }
    const item_location &loc = selected.front().first;
    if( loading ) {
        if( here.can_put_items( where ) &&
            iexamine::furniture_accepts_item( where, *loc ).value_or( true ) ) {
            you.drop( selected, where );
        }
    } else if( loc->made_of( phase_id::LIQUID ) ) {
        item_location liquid = loc;
        liquid_handler::handle_liquid( liquid );
    } else if( !here.has_flag( ter_furn_flag::TFLAG_SEALED, where ) ) {
        std::vector<item_location> items;
        std::vector<int> counts;
        for( const drop_location &entry : selected ) {
            items.push_back( entry.first );
            counts.push_back( entry.second );
        }
        you.assign_activity( pickup_activity_actor( items, counts, you.pos_bub(), false ) );
    }
}
#endif
} // namespace

void unload( const tripoint_bub_ms &where )
{
#if defined(TILES)
    if( active_station && *active_station == get_map().get_abs( where ) ) {
        transfer( where, false );
        return;
    }
#endif
    g->pickup( where );
}

std::optional<drop_locations> select_materials( [[maybe_unused]] const tripoint_bub_ms &where,
        [[maybe_unused]] const std::function<int( const item & )> &limit, [[maybe_unused]] int radius )
{
#if defined(TILES)
    if( active_station && *active_station == get_map().get_abs( where ) &&
        !get_option<bool>( "SCREEN_READER_MODE" ) ) {
        return choose_items( where, true, limit, radius );
    }
#endif
    return std::nullopt;
}

std::string name( const tripoint_bub_ms &where )
{
    const optional_vpart_position vp = get_map().veh_at( where );
    return vp && vp->vehicle().is_appliance() ? vp->vehicle().name : get_map().name( where );
}

bool can_manage( const tripoint_bub_ms &where )
{
    map &here = get_map();
    if( !accessible( where ) ) {
        return false;
    }
    const optional_vpart_position vp = here.veh_at( where );
    if( vp && vp->vehicle().is_appliance() ) {
        return true;
    }
    const furn_t &f = here.furn( where ).obj();
    return ( here.has_furn( where ) && ( f.can_examine( where ) ||
                                         f.has_flag( ter_furn_flag::TFLAG_CONTAINER ) ||
                                         f.has_flag( ter_furn_flag::TFLAG_PLACE_ITEM ) || f.workbench ) ) ||
           here.ter( where ).obj().has_examine( iexamine::fireplace );
}

void query( uilist &menu, [[maybe_unused]] const tripoint_bub_ms &where )
{
    ++menus_shown;
#if defined(TILES)
    if( active_station && *active_station == get_map().get_abs( where ) &&
        !get_option<bool>( "SCREEN_READER_MODE" ) ) {
        std::optional<int> choice;
        const bool keycode = std::any_of( menu.entries.begin(), menu.entries.end(),
        []( const uilist_entry & entry ) {
            return entry.hotkey && entry.hotkey->type == input_event_t::keyboard_code;
        } );
        input_context ctxt( "WORKSTATION", keycode ? keyboard_mode::keycode : keyboard_mode::keychar );
        ctxt.register_action( "QUIT" );
        ctxt.register_action( "ANY_INPUT" );
        ctxt.register_navigate_ui_list();
        ctxt.register_action( "CONFIRM" );
        ctxt.set_timeout( 16 );
        int selected = 0;
        std::string automatic_keys = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        for( uilist_entry &entry : menu.entries ) {
            if( entry.retval == -1 ) {
                entry.retval = &entry - menu.entries.data();
            }
            if( !entry.hotkey ) {
                for( char key : automatic_keys ) {
                    const input_event candidate( key, input_event_t::keyboard_char );
                    const bool taken = std::any_of( menu.entries.begin(), menu.entries.end(),
                    [&]( const uilist_entry & other ) {
                        return other.hotkey && *other.hotkey == candidate;
                    } );
                    if( !taken ) {
                        entry.hotkey = candidate;
                        break;
                    }
                }
            }
        }
        station_window window( [&]() {
            draw_station( where, menu.text );
            const float buttons_height = std::max( 80.f, ImGui::GetContentRegionAvail().y -
                                                   ImGui::GetTextLineHeightWithSpacing() * 5.f );
            ImGui::BeginChild( "station_controls", ImVec2( 0.f, buttons_height ) );
            for( size_t i = 0; i < menu.entries.size(); ++i ) {
                const uilist_entry &entry = menu.entries[i];
                ImGui::PushID( static_cast<int>( i ) );
                ImGui::BeginDisabled( !entry.enabled );
                std::string label = remove_color_tags( entry.txt );
                const size_t detail = label.find( "…" );
                if( detail != std::string::npos ) {
                    label.resize( detail );
                }
                if( entry.hotkey && !entry.hotkey->sequence.empty() ) {
                    label += " [" + entry.hotkey->short_description() + "]";
                }
                if( selected == static_cast<int>( i ) ) {
                    ImGui::PushStyleColor( ImGuiCol_Button, ui_hybrid_chrome::palette::header() );
                }
                if( ImGui::Button( label.c_str(), ImVec2( -1.f, 0.f ) ) ) {
                    choice = entry.retval;
                }
                if( selected == static_cast<int>( i ) ) {
                    ImGui::PopStyleColor();
                }
                ImGui::EndDisabled();
                if( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) ) {
                    ImGui::SetTooltip( "%s\n%s", remove_color_tags( entry.txt ).c_str(),
                                       remove_color_tags( entry.desc ).c_str() );
                }
                ImGui::PopID();
            }
            ImGui::EndChild();
            const float feedback_height = std::max( ImGui::GetTextLineHeightWithSpacing(),
                                                    ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() );
            ImGui::BeginChild( "station_feedback", ImVec2( 0.f, feedback_height ) );
            const auto messages = Messages::recent_messages( 2 );
            for( const auto &msg : messages ) {
                ImGui::TextWrapped( "%s", remove_color_tags( msg.second ).c_str() );
            }
            ImGui::EndChild();
            if( ImGui::Button( _( "Close" ) ) ) {
                closing = true;
            }
        }, std::clamp( 280.f + menu.entries.size() * 30.f, 360.f, 660.f ) );
        while( window.get_is_open() && !choice && !closing ) {
            ui_manager::redraw_invalidated();
            const std::string action = ctxt.handle_input();
            if( action == "QUIT" ) {
                closing = true;
            } else if( action == "UP" || action == "DOWN" ) {
                const int size = static_cast<int>( menu.entries.size() );
                if( size > 0 ) {
                    selected = ( selected + size + ( action == "UP" ? -1 : 1 ) ) % size;
                }
            } else if( action == "CONFIRM" && !menu.entries.empty() && menu.entries[selected].enabled ) {
                choice = menu.entries[selected].retval;
            } else if( action == "ANY_INPUT" ) {
                const input_event &event = ctxt.get_raw_input();
                for( const uilist_entry &entry : menu.entries ) {
                    if( entry.enabled && entry.hotkey && ( *entry.hotkey == event ||
                                                           ( keycode && entry.hotkey->type == input_event_t::keyboard_char &&
                                                                   entry.hotkey->sequence == event.sequence ) ) ) {
                        choice = entry.retval;
                        break;
                    }
                }
            }
        }
        if( !window.get_is_open() ) {
            closing = true;
        }
        menu.ret = choice.value_or( UILIST_CANCEL );
        ui_telemetry::record( "workstation.action", {{ "station", name( where ) },
            { "choice", std::to_string( menu.ret ) }
        } );
        return;
    }
#endif
    menu.query();
    if( menu.ret < 0 && active_station ) {
        closing = true;
    }
}

void open( const tripoint_bub_ms &where )
{
    if( !can_manage( where ) ) {
        return;
    }
    const ui_telemetry::scope trace( "workstation.open", {{ "station", name( where ) } } );
    map &here = get_map();
    avatar &you = get_avatar();
    active_station = here.get_abs( where );
    return_to_station.reset();
    closing = false;
    bool use_native_controls = has_native_controls( where );
    while( !closing && can_manage( where ) ) {
        const int moves = you.get_moves();
        if( use_native_controls ) {
            const unsigned before = menus_shown;
            examine( where );
            if( menus_shown == before ) {
                // Native handlers can reject invalid location or station state
                // before opening their menu. Keep a usable manager in that case.
                use_native_controls = false;
            }
        } else {
            const furn_t &f = here.furn( where ).obj();
            uilist menu;
            menu.title = name( where );
            if( here.can_put_items( where ) ) {
                menu.addentry( load_items, true, 'l', f.has_examine( iexamine::kiln_empty ) ?
                               _( "Load fuel" ) : _( "Load materials" ) );
            }
            if( f.can_examine( where ) || here.ter( where ).obj().can_examine( where ) ) {
                menu.addentry( operate, true, 's', primary_label( where ) );
            }
            if( !here.has_flag( ter_furn_flag::TFLAG_SEALED, where ) ) {
                menu.addentry( unload_items, !here.i_at( where ).empty(), 'u', _( "Unload contents" ) );
            }
            if( f.workbench ) {
                menu.addentry( craft, true, 'c', _( "Craft" ) );
            }
            query( menu, where );
            if( menu.ret == operate ) {
                examine( where );
            } else if( menu.ret == craft ) {
                you.craft();
            } else if( menu.ret == load_items || menu.ret == unload_items ) {
#if defined(TILES)
                transfer( where, menu.ret == load_items );
#else
                examine( where );
#endif
            }
        }
        if( !you.activity.is_null() || you.get_moves() < moves ) {
            return_to_station = active_station;
            break;
        }
    }
    active_station.reset();
}

void open_nearby()
{
    std::vector<tripoint_bub_ms> stations;
    uilist list;
    list.title = _( "Nearby workstations" );
    map &here = get_map();
    for( const tripoint_bub_ms &where : here.points_in_radius( get_avatar().pos_bub(), 1 ) ) {
        if( can_manage( where ) ) {
            list.addentry( static_cast<int>( stations.size() ), true, MENU_AUTOASSIGN,
                           string_format( "%s (%s)", name( where ),
                                          direction_name_short( direction_from( get_avatar().pos_bub(), where ) ) ) );
            stations.push_back( where );
        }
    }
    if( stations.empty() ) {
        popup( _( "No accessible workstation nearby. Stand beside the station you want to manage." ) );
        return;
    }
    if( stations.size() == 1 ) {
        open( stations.front() );
        return;
    }
    list.query();
    if( list.ret >= 0 && static_cast<size_t>( list.ret ) < stations.size() ) {
        open( stations[list.ret] );
    }
}

void request_nearby()
{
    requested = true;
}
bool has_request()
{
    return requested;
}
bool process_request()
{
    if( !requested ) {
        return false;
    }
    requested = false;
    open_nearby();
    return true;
}
bool resume_if_ready()
{
    if( !return_to_station || !get_avatar().activity.is_null() ) {
        return false;
    }
    const tripoint_bub_ms where = get_map().get_bub( *return_to_station );
    return_to_station.reset();
    if( !can_manage( where ) || g->safe_mode == SAFE_MODE_STOP ) {
        return false;
    }
    open( where );
    return true;
}
void reset()
{
    requested = false;
    closing = false;
    active_station.reset();
    return_to_station.reset();
}
} // namespace workstation_ui
