#include "ui_telemetry.h"
#include "rpg_equipment_ui.h"
#include "equipment_layout.h"
#include "equipment_actions.h"

#include <algorithm>
#include <array>
#include <set>
#include <optional>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "advanced_inv.h"
#include "avatar_action.h"
#include "activity_actor_definitions.h"
#include "bodypart.h"
#include "cata_imgui.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "ui_hybrid_chrome.h"
#include "ui_hybrid_textures.h"
#include "ui_hybrid_widgets.h"
#include "catacharset.h"
#include "character.h"
#include "color.h"
#include "debug.h"
#include "enums.h"
#include "flag.h"
#include "game.h"
#include "game_inventory.h"
#include "imgui/imgui.h"
#include <imgui/imgui_internal.h>
#include "input_context.h"
#include "item.h"
#include "item_pocket.h"
#include "iuse_actor.h"
#include "iuse.h"
#include "item_context_menu.h"
#include "item_location.h"
#include "itype.h"
#include "map.h"
#include "map_selector.h"
#include "mapdata.h"
#include "mutation.h"
#include "units.h"
#include "point.h"
#include "messages.h"
#include "options.h"
#include "output.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_iteminfo.h"
#include "ui_manager.h"

#if defined(TILES)
#  include "cata_tiles.h"
#  include "sdltiles.h"
#endif

static const flag_id flag_ASTRAL_SHIELD( "ASTRAL_SHIELD" );

rpg_equipment_ui::storage_slot rpg_equipment_ui::storage_slot_for( const item &it )
{
    return equipment_layout::storage_slot_for( it );
}

namespace
{

struct doll_slot {
    enum class kind {
        body, body_layer, body_outer, back, scabbard, sheath, holster, weapon, offhand,
        region, region_layer
    } type = kind::region;
    bodypart_id bp;
    std::string label;
    layer_level layer = layer_level::NORMAL;
    equipment_layout::region_id region = equipment_layout::region_id::fallback;
    equipment_layout::display_layer display = equipment_layout::display_layer::any;
    equipment_layout::region_side side = equipment_layout::region_side::none;
    /** Waist prominent positions 0/1; overflow uses >=2. */
    int position_index = 0;
};

static const char *layer_name( layer_level layer )
{
    switch( layer ) {
        case layer_level::PERSONAL:
            return _( "Personal" );
        case layer_level::SKINTIGHT:
            return _( "Close to skin" );
        case layer_level::NORMAL:
            return _( "Normal" );
        case layer_level::WAIST:
            return _( "Waist" );
        case layer_level::OUTER:
            return _( "Outer" );
        case layer_level::BELTED:
            return _( "Strapped" );
        case layer_level::AURA:
            return _( "Aura" );
        default:
            return "";
    }
}

static std::string labeled_region( const equipment_layout::region_info &info )
{
    return _( info.label );
}

enum class doll_group { overview, head, body, arms, hands, waist, legs, feet, back, weapons, other };

static doll_group group_for( equipment_layout::region_id id )
{
    using equipment_layout::region_id;
    switch( id ) {
        case region_id::head:
        case region_id::ear_l:
        case region_id::ear_r:
        case region_id::forehead:
        case region_id::eyes:
        case region_id::face:
            return doll_group::head;
        case region_id::neck:
        case region_id::torso:
            return doll_group::body;
        case region_id::arm_l:
        case region_id::arm_r:
            return doll_group::arms;
        case region_id::wrist_l:
        case region_id::wrist_r:
        case region_id::hand_l:
        case region_id::hand_r:
        case region_id::rings_l:
        case region_id::rings_r:
            return doll_group::hands;
        case region_id::waist:
            return doll_group::waist;
        case region_id::pants:
        case region_id::leg_lower_l:
        case region_id::leg_lower_r:
            return doll_group::legs;
        case region_id::foot_l:
        case region_id::foot_r:
            return doll_group::feet;
        case region_id::back_bag:
        case region_id::back_weapon:
            return doll_group::back;
        case region_id::main_hand:
        case region_id::off_hand:
            return doll_group::weapons;
        default:
            return doll_group::other;
    }
}

static const char *group_label( doll_group group )
{
    switch( group ) {
        case doll_group::head:
            return _( "Head" );
        case doll_group::body:
            return _( "Body" );
        case doll_group::arms:
            return _( "Arms" );
        case doll_group::hands:
            return _( "Hands" );
        case doll_group::waist:
            return _( "Waist" );
        case doll_group::legs:
            return _( "Legs" );
        case doll_group::feet:
            return _( "Feet" );
        case doll_group::back:
            return _( "Back" );
        case doll_group::weapons:
            return _( "Weapons" );
        case doll_group::other:
            return _( "Other equipment" );
        default:
            return _( "Equipment" );
    }
}

/** Atlas icon for an equipment group (data/ui/astral/icons). */
static const char *group_icon( doll_group group )
{
    switch( group ) {
        case doll_group::head:
            return "cat_head";
        case doll_group::body:
            return "cat_body";
        case doll_group::arms:
            return "cat_arms";
        case doll_group::hands:
            return "cat_hands";
        case doll_group::waist:
            return "cat_waist";
        case doll_group::legs:
            return "cat_legs";
        case doll_group::feet:
            return "cat_feet";
        case doll_group::back:
            return "cat_back";
        case doll_group::weapons:
            return "cat_weapons";
        case doll_group::other:
            return "cat_other";
        default:
            return "cat_all";
    }
}

/** Atlas icon for an anatomical region row. */
static const char *region_icon( equipment_layout::region_id id )
{
    using equipment_layout::region_id;
    switch( id ) {
        case region_id::head:
        case region_id::forehead:
            return "cat_head";
        case region_id::ear_l:
        case region_id::ear_r:
            return "cat_ears";
        case region_id::eyes:
            return "cat_eyes";
        case region_id::face:
            return "cat_face";
        case region_id::neck:
            return "cat_neck";
        case region_id::torso:
            return "cat_body";
        case region_id::arm_l:
        case region_id::arm_r:
            return "cat_arms";
        case region_id::wrist_l:
        case region_id::wrist_r:
        case region_id::hand_l:
        case region_id::hand_r:
        case region_id::rings_l:
        case region_id::rings_r:
            return "cat_hands";
        case region_id::waist:
            return "cat_waist";
        case region_id::pants:
        case region_id::leg_lower_l:
        case region_id::leg_lower_r:
            return "cat_legs";
        case region_id::foot_l:
        case region_id::foot_r:
            return "cat_feet";
        case region_id::back_bag:
        case region_id::back_weapon:
            return "cat_back";
        case region_id::main_hand:
            return "cat_main_hand";
        case region_id::off_hand:
            return "cat_off_hand";
        default:
            return "cat_other";
    }
}

static bool is_overview_slot( const doll_slot &slot )
{
    return slot.type == doll_slot::kind::region || slot.type == doll_slot::kind::weapon ||
           slot.type == doll_slot::kind::offhand || slot.type == doll_slot::kind::back;
}

static const std::array<doll_group, 10> overview_groups = {{
        doll_group::head, doll_group::body, doll_group::arms, doll_group::hands,
        doll_group::waist, doll_group::legs, doll_group::feet, doll_group::back,
        doll_group::weapons, doll_group::other
    }
};

static void draw_equipment_icon( const item &it, ImVec2 min, ImVec2 max )
{
#if defined(TILES)
    if( get_option<bool>( "USE_TILES" ) && tilecontext ) {
        const auto data = tilecontext->get_texture_draw_data( it.typeId().str(),
                          TILE_CATEGORY::ITEM, tripoint_bub_ms() );
        if( data ) {
            ImGui::GetWindowDrawList()->AddImage( reinterpret_cast<ImTextureID>( data->texture ),
                                                  min, max, ImVec2( data->uv0.first, data->uv0.second ),
                                                  ImVec2( data->uv1.first, data->uv1.second ) );
            return;
        }
    }
#endif
    ImGui::GetWindowDrawList()->AddText( min, ImGui::GetColorU32( ImGuiCol_Text ),
                                         it.symbol().c_str() );
}

static std::vector<doll_slot> make_doll_slots( Character &you )
{
    using equipment_layout::region_id;
    using equipment_layout::display_layer;
    using equipment_layout::region_side;

    std::vector<doll_slot> slots;
    const std::vector<region_id> visible = equipment_layout::visible_regions_for( you );
    const auto is_visible = [&]( region_id id ) {
        return std::find( visible.begin(), visible.end(), id ) != visible.end();
    };

    const auto add_region = [&]( region_id id, doll_slot::kind type = doll_slot::kind::region,
                                 display_layer display = display_layer::any, int position_index = 0,
    const char *label_override = nullptr ) {
        if( !is_visible( id ) ) {
            return;
        }
        const equipment_layout::region_info &info = equipment_layout::info_for( id );
        doll_slot s;
        s.type = type;
        s.region = id;
        s.display = display;
        s.side = info.side;
        s.position_index = position_index;
        s.label = label_override ? _( label_override ) : labeled_region( info );
        s.bp = bodypart_str_id::NULL_ID().id();
        // Keep a body-part hint for encumbrance readout; layer controls no
        // longer key off shared bp IDs (avoids Neck/Torso duplicate panels).
        switch( id ) {
            case region_id::head:
            case region_id::ear_l:
            case region_id::ear_r:
            case region_id::forehead:
                s.bp = body_part_head.id();
                break;
            case region_id::eyes:
                s.bp = body_part_eyes.id();
                break;
            case region_id::face:
                s.bp = body_part_mouth.id();
                break;
            case region_id::neck:
            case region_id::torso:
            case region_id::waist:
            case region_id::back_bag:
            case region_id::back_weapon:
                s.bp = body_part_torso.id();
                break;
            case region_id::arm_l:
                s.bp = body_part_arm_l.id();
                break;
            case region_id::arm_r:
                s.bp = body_part_arm_r.id();
                break;
            case region_id::wrist_l:
            case region_id::hand_l:
            case region_id::rings_l:
                s.bp = body_part_hand_l.id();
                break;
            case region_id::wrist_r:
            case region_id::hand_r:
            case region_id::rings_r:
                s.bp = body_part_hand_r.id();
                break;
            case region_id::pants:
            case region_id::leg_lower_l:
                s.bp = body_part_leg_l.id();
                break;
            case region_id::leg_lower_r:
                s.bp = body_part_leg_r.id();
                break;
            case region_id::foot_l:
                s.bp = body_part_foot_l.id();
                break;
            case region_id::foot_r:
                s.bp = body_part_foot_r.id();
                break;
            default:
                break;
        }
        slots.push_back( s );
    };

    // Anatomical overview (Package B layout).
    add_region( region_id::head );
    add_region( region_id::forehead );
    add_region( region_id::eyes );
    add_region( region_id::face );
    add_region( region_id::ear_l );
    add_region( region_id::ear_r );
    add_region( region_id::neck );
    add_region( region_id::torso );
    add_region( region_id::arm_l );
    add_region( region_id::arm_r );
    add_region( region_id::wrist_l );
    add_region( region_id::wrist_r );
    add_region( region_id::hand_l );
    add_region( region_id::hand_r );
    add_region( region_id::rings_l );
    add_region( region_id::rings_r );
    // Two prominent waist equipment positions + overflow list elsewhere.
    add_region( region_id::waist, doll_slot::kind::region, display_layer::any, 0, "Waist 1" );
    add_region( region_id::waist, doll_slot::kind::region, display_layer::any, 1, "Waist 2" );
    add_region( region_id::pants );
    add_region( region_id::leg_lower_l );
    add_region( region_id::leg_lower_r );
    add_region( region_id::foot_l );
    add_region( region_id::foot_r );
    add_region( region_id::back_bag, doll_slot::kind::back );
    add_region( region_id::back_weapon );
    add_region( region_id::main_hand, doll_slot::kind::weapon );
    add_region( region_id::off_hand, doll_slot::kind::offhand );
    add_region( region_id::fallback );

    // Legacy holder overview labels remain reachable as waist/back filters.
    slots.push_back( { doll_slot::kind::scabbard, bodypart_str_id::NULL_ID().id(),
                       _( "Scabbards" ), layer_level::NORMAL, region_id::waist,
                       display_layer::uncommon, region_side::none, 0 } );
    slots.push_back( { doll_slot::kind::sheath, bodypart_str_id::NULL_ID().id(),
                       _( "Sheaths" ), layer_level::NORMAL, region_id::waist,
                       display_layer::uncommon, region_side::none, 0 } );
    slots.push_back( { doll_slot::kind::holster, bodypart_str_id::NULL_ID().id(),
                       _( "Holsters" ), layer_level::NORMAL, region_id::waist,
                       display_layer::uncommon, region_side::none, 0 } );

    // Compact skin/middle/outer (+ uncommon) layer targets per clothing region.
    // Do NOT emit seven legacy body_layer slots per region — Neck/Torso shared
    // the same bp and duplicated the layer panel. Uncommon expands native
    // layers for the selected region only at draw time.
    const size_t overview_count = slots.size();
    for( size_t i = 0; i < overview_count; ++i ) {
        const equipment_layout::region_info &info = equipment_layout::info_for( slots[i].region );
        if( !info.skin_middle_outer || slots[i].type == doll_slot::kind::scabbard ||
            slots[i].type == doll_slot::kind::sheath || slots[i].type == doll_slot::kind::holster ||
            slots[i].region == region_id::waist ) {
            continue;
        }
        // Only one compact set per region (skip duplicate waist positions).
        bool already = false;
        for( size_t j = overview_count; j < slots.size(); ++j ) {
            if( slots[j].type == doll_slot::kind::region_layer &&
                slots[j].region == slots[i].region ) {
                already = true;
                break;
            }
        }
        if( already ) {
            continue;
        }
        for( display_layer display : {
                 display_layer::skin, display_layer::middle,
                 display_layer::outer, display_layer::uncommon
             } ) {
            doll_slot layer_slot = slots[i];
            layer_slot.type = doll_slot::kind::region_layer;
            layer_slot.display = display;
            layer_slot.layer = equipment_layout::to_native_layer( display );
            layer_slot.position_index = 0;
            layer_slot.label = string_format( "%s",
                                              equipment_layout::display_layer_label( display ) );
            slots.push_back( layer_slot );
        }
    }
    return slots;
}

static bool is_outer_on_torso( const item &it )
{
    return it.covers( body_part_torso ) && it.has_layer( { layer_level::OUTER }, body_part_torso );
}

static bool slot_matches( const item &it, const doll_slot &slot )
{
    using storage = rpg_equipment_ui::storage_slot;
    using equipment_layout::region_id;
    using equipment_layout::display_layer;
    const storage place = rpg_equipment_ui::storage_slot_for( it );
    const equipment_layout::item_profile profile = equipment_layout::classify_item( it );

    switch( slot.type ) {
        case doll_slot::kind::scabbard:
            return place == storage::scabbard;
        case doll_slot::kind::sheath:
            return place == storage::sheath;
        case doll_slot::kind::holster:
            return place == storage::holster;
        case doll_slot::kind::back:
            return place == storage::back;
        case doll_slot::kind::body_outer:
            return is_outer_on_torso( it ) && place == storage::none;
        case doll_slot::kind::body_layer:
            return it.covers( slot.bp ) && it.has_layer( { slot.layer }, slot.bp );
        case doll_slot::kind::body:
            return it.covers( slot.bp ) && place == storage::none &&
                   !( it.has_flag( flag_BLOCK_WHILE_WORN ) && it.has_flag( flag_RESTRICT_HANDS ) ) &&
                   ( slot.bp != body_part_torso || !is_outer_on_torso( it ) );
        case doll_slot::kind::offhand:
            return it.has_flag( flag_BLOCK_WHILE_WORN ) && it.has_flag( flag_RESTRICT_HANDS );
        case doll_slot::kind::weapon:
            return true;
        case doll_slot::kind::region:
        case doll_slot::kind::region_layer:
            if( slot.region == region_id::main_hand ) {
                return true;
            }
            if( slot.region == region_id::off_hand ) {
                return it.has_flag( flag_BLOCK_WHILE_WORN ) && it.has_flag( flag_RESTRICT_HANDS );
            }
            if( slot.type == doll_slot::kind::region_layer ) {
                return equipment_layout::is_plausible_drop_destination( it, slot.region, slot.display );
            }
            return equipment_layout::is_plausible_drop_destination( it, slot.region, display_layer::any ) ||
                   profile.equip_destination.id == slot.region;
    }
    return false;
}

static std::vector<item_location> items_on_slot( Character &you, const doll_slot &slot )
{
    using equipment_layout::region_id;
    using equipment_layout::display_layer;
    std::vector<item_location> result;

    if( slot.type == doll_slot::kind::weapon || slot.region == region_id::main_hand ) {
        const equipment_layout::hand_occupancy hands = equipment_layout::hands_for( you );
        if( hands.main_hand ) {
            result.push_back( hands.main_hand );
        }
        return result;
    }
    if( slot.type == doll_slot::kind::offhand || slot.region == region_id::off_hand ) {
        const equipment_layout::hand_occupancy hands = equipment_layout::hands_for( you );
        if( hands.off_hand ) {
            result.push_back( hands.off_hand );
        }
        return result;
    }

    if( slot.type == doll_slot::kind::region || slot.type == doll_slot::kind::region_layer ) {
        const equipment_layout::character_map mapped = equipment_layout::map_character( you );
        const display_layer layer = ( slot.type == doll_slot::kind::region_layer )
                                    ? slot.display : display_layer::any;
        // Occupants ≠ incidental coverage highlight (helmet on empty ear).
        result = equipment_layout::items_visible_on( mapped, slot.region, layer );
        if( slot.region == region_id::waist ) {
            // Two prominent waist positions; index >=2 is overflow (shown on
            // both tiles as a count via the full list when selected).
            std::vector<item_location> holders;
            std::vector<item_location> overflow;
            for( const item_location &loc : result ) {
                if( !loc ) {
                    continue;
                }
                const equipment_layout::item_profile profile =
                    equipment_layout::classify_item( *loc );
                if( profile.role == equipment_layout::equipment_role::holder ||
                    profile.storage != equipment_layout::storage_slot::none ) {
                    holders.push_back( loc );
                } else {
                    overflow.push_back( loc );
                }
            }
            std::vector<item_location> positioned;
            if( slot.position_index >= 0 &&
                slot.position_index < static_cast<int>( holders.size() ) ) {
                positioned.push_back( holders[slot.position_index] );
            }
            // Overflow (3rd+ holders and non-holder waist clothing) appears on
            // the selected waist tile's item list only when that tile is empty
            // of a dedicated holder — always append for position 0 as overflow
            // entry point when more than two holders exist.
            if( slot.position_index == 0 && holders.size() > 2 ) {
                for( size_t i = 2; i < holders.size(); ++i ) {
                    positioned.push_back( holders[i] );
                }
            }
            if( slot.position_index == 0 ) {
                positioned.insert( positioned.end(), overflow.begin(), overflow.end() );
            }
            return positioned;
        }
        return result;
    }

    for( const item_location &loc : you.top_items_loc() ) {
        if( loc && you.is_worn( *loc ) && slot_matches( *loc, slot ) ) {
            result.push_back( loc );
        }
    }
    return result;
}

static item_location item_on_slot( Character &you, const doll_slot &slot )
{
    const std::vector<item_location> items = items_on_slot( you, slot );
    return items.empty() ? item_location::nowhere : items.back();
}

/** Prefer word-boundary cut, then utf8-safe ellipsis. */
static std::string ellipsize_label( const std::string &raw, int max_cells )
{
    if( max_cells <= 1 ) {
        return "…";
    }
    if( utf8_width( raw ) <= max_cells ) {
        return raw;
    }
    const std::string truncated = utf8_truncate( raw, static_cast<size_t>( max_cells - 1 ) );
    size_t break_at = std::string::npos;
    const size_t min_keep = truncated.size() / 3;
    for( size_t i = truncated.size(); i > min_keep; --i ) {
        const char c = truncated[i - 1];
        if( c == ' ' || c == '-' || c == '/' || c == '_' ) {
            break_at = i - 1;
            break;
        }
    }
    std::string out = ( break_at != std::string::npos )
                      ? truncated.substr( 0, break_at )
                      : truncated;
    while( !out.empty() && out.back() == ' ' ) {
        out.pop_back();
    }
    if( out.empty() ) {
        out = utf8_truncate( raw, static_cast<size_t>( max_cells - 1 ) );
    }
    return out + "…";
}

static void imgui_cdda_tooltip( const std::string &tip )
{
    if( tip.empty() ) {
        return;
    }
    // Parse CDDA <color_…> tags into real ImGui colors (hides raw markup; greens ++).
    ImGui::BeginTooltip();
    cataimgui::draw_colored_text( tip, ImGui::GetFontSize() * 35.0f );
    ImGui::EndTooltip();
}


// Temporary RMB context-menu diagnostics (remove after root-cause confirmed).
// Always uses D_MAIN so lines reach config/debug.log without raising debug filters.
static constexpr bool RPG_EQ_CTX_TELEM = false;

static void rpg_eq_ctx_telem( std::string &ui_line, const std::string &msg )
{
    ui_telemetry::record( "equipment.context", {{ "detail", msg }} );
    if( !RPG_EQ_CTX_TELEM ) {
        return;
    }
    DebugLog( D_INFO, D_MAIN ) << "rpg_eq_ctx: " << msg;
    ui_line = msg;
}

static std::string rpg_eq_short_tname( const item_location &loc )
{
    if( !loc || !loc.get_item() ) {
        return "<none>";
    }
    std::string n = remove_color_tags( loc->tname( 1, false ) );
    if( utf8_width( n ) > 24 ) {
        n = utf8_truncate( n, 24 );
    }
    return n;
}


/**
 * Short readable grid / doll label: prefer type_name + count, word-boundary ellipsis.
 * Full identity stays in the tooltip via display_name().
 */
static std::string cell_label( const item &it, int stack_count = 1, int max_chars = 18 )
{
    // type_name is usually shorter / cleaner than full tname with prefixes.
    std::string name = it.type_name( 1, /*use_variant=*/true );
    if( name.empty() ) {
        name = remove_color_tags( it.tname( 1, false ) );
    } else {
        name = remove_color_tags( name );
    }
    std::string count_suffix;
    if( it.count_by_charges() && it.charges > 1 ) {
        count_suffix = string_format( "×%d", it.charges );
    } else if( stack_count > 1 ) {
        count_suffix = string_format( "×%d", stack_count );
    } else if( it.count() > 1 ) {
        count_suffix = string_format( "×%d", it.count() );
    }
    const int suffix_w = utf8_width( count_suffix );
    const int name_budget = std::max( 4, max_chars - suffix_w );
    name = ellipsize_label( name, name_budget );
    return name + count_suffix;
}


/** Stack / charges count for a corner badge; 0 means no badge. */
static int cell_stack_badge( const item &it, int stack_count )
{
    if( it.count_by_charges() && it.charges > 1 ) {
        return static_cast<int>( it.charges );
    }
    if( stack_count > 1 ) {
        return stack_count;
    }
    if( it.count() > 1 ) {
        return it.count();
    }
    return 0;
}

/** One-glyph fallback when tiles are off or the itype has no sprite. */
static std::string cell_fallback_glyph( const item &it )
{
    std::string name = it.type_name( 1, /*use_variant=*/true );
    if( name.empty() ) {
        name = remove_color_tags( it.tname( 1, false ) );
    } else {
        name = remove_color_tags( name );
    }
    if( name.empty() ) {
        return "?";
    }
    return utf8_truncate( name, 1 );
}

/** Compact ×N / ×Nk / ×N.NM for corner badges that must fit inside a cell. */
static std::string format_stack_badge( int n )
{
    if( n < 1000 ) {
        return string_format( "×%d", n );
    }
    if( n < 1000000 ) {
        const int whole = n / 1000;
        const int frac = ( n % 1000 ) / 100;
        if( frac == 0 ) {
            return string_format( "×%dk", whole );
        }
        return string_format( "×%d.%dk", whole, frac );
    }
    const int whole = n / 1000000;
    const int frac = ( n % 1000000 ) / 100000;
    if( frac == 0 ) {
        return string_format( "×%dM", whole );
    }
    return string_format( "×%d.%dM", whole, frac );
}

/**
 * After an ImGui button/item: paint the default tileset ITEM sprite (and optional
 * ×N badge / text fallback) via the window draw list — NEVER ImGui::Image /
 * TextUnformatted. Those submit new items that become GetItemRect* for
 * SameLine / BeginDragDropSource / BeginPopupContextItem, which staggered the
 * inventory grid and ate right-click + drag hits. Draw-list overlays leave the
 * Button as the sole interactive + layout item (Hybrid bezel stays).
 */
static void overlay_item_sprite_on_last_item( const item &it, int stack_count,
        const std::string &fallback_label, float icon_pad = 4.f )
{
    const ImVec2 rmin = ImGui::GetItemRectMin();
    const ImVec2 rmax = ImGui::GetItemRectMax();
    const float cw = rmax.x - rmin.x;
    const float ch = rmax.y - rmin.y;
    if( cw < 4.f || ch < 4.f ) {
        return;
    }

    ImDrawList *dl = ImGui::GetWindowDrawList();
    const ImU32 text_col = ImGui::GetColorU32( ImGuiCol_Text );

    bool drew_sprite = false;
#if defined(TILES)
    if( get_option<bool>( "USE_TILES" ) && tilecontext ) {
        const itype_id &iid = it.typeId();
        if( iid.is_valid() ) {
            // get_texture_draw_data already follows looks_like / variants.
            const std::optional<texture_draw_data> data =
                tilecontext->get_texture_draw_data( iid.str(), TILE_CATEGORY::ITEM,
                                                    tripoint_bub_ms() );
            if( data ) {
                const float sz = std::max( 8.f, std::min( cw, ch ) - icon_pad * 2.f );
                const ImVec2 p0( rmin.x + ( cw - sz ) * 0.5f,
                                 rmin.y + ( ch - sz ) * 0.5f );
                const ImVec2 p1( p0.x + sz, p0.y + sz );
                dl->AddImage( reinterpret_cast<ImTextureID>( data->texture ), p0, p1,
                              ImVec2( data->uv0.first, data->uv0.second ),
                              ImVec2( data->uv1.first, data->uv1.second ) );
                drew_sprite = true;
            }
        }
    }
#endif
    // Text fallback only when no tile — hide truncated names under successful sprites.
    if( !drew_sprite ) {
        const std::string &fb = !fallback_label.empty() ? fallback_label
                                : cell_fallback_glyph( it );
        if( !fb.empty() ) {
            const ImVec2 ts = ImGui::CalcTextSize( fb.c_str() );
            dl->AddText( ImVec2( rmin.x + ( cw - ts.x ) * 0.5f,
                                 rmin.y + ( ch - ts.y ) * 0.5f ),
                         text_col, fb.c_str() );
        }
    }

    const int badge_n = cell_stack_badge( it, stack_count );
    if( badge_n > 1 ) {
        const std::string badge = format_stack_badge( badge_n );
        ImFont *font = ImGui::GetFont();
        const float fs = ImGui::GetFontSize() * 0.80f;
        const ImVec2 ts = font->CalcTextSizeA( fs, FLT_MAX, 0.f, badge.c_str() );
        // Clip to cell bezel so huge counts cannot spill into neighbors.
        dl->PushClipRect( rmin, rmax, true );
        dl->AddText( font, fs,
                     ImVec2( rmax.x - ts.x - 2.f, rmin.y + 1.f ),
                     text_col, badge.c_str() );
        dl->PopClipRect();
    }
}

static bool item_looks_usable( const item &it )
{
    if( it.is_comestible() || it.is_book() || it.is_craft() || it.is_medical_tool() ) {
        return true;
    }
    if( it.has_relic_activation() ) {
        return true;
    }
    return it.type->has_use();
}

enum class pending_action {
    none,
    equip,
    takeoff,
    wield,
    use_item,
    drop_item,
    examine_item,
    drag_equip,
    // Shared item_context_menu actions (inv grid + paper-doll)
    ctx_consume,
    ctx_read,
    ctx_unload,
    ctx_reload,
    ctx_wear,
    ctx_pickup
};

class rpg_equipment_window : public cataimgui::window
{
    public:
        explicit rpg_equipment_window( Character *guy )
            : cataimgui::window( _( "Character Equipment" ),
                                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoNav ) {
            you = guy;
            slots = make_doll_slots( *you );
            // Astral shell: frame, title and close control drawn by the shared widgets.
            set_shell( 0, "tab_equipment" );
        }

        bool execute();
        item_location reload_after_close() const {
            return deferred_reload_loc;
        }
        bool classic_after_close() const {
            return open_classic;
        }
        bool storage_after_close() const {
            return open_storage;
        }
        bool layers_after_close() const {
            return open_layers;
        }

    protected:
        void draw() override {
            // Push Hybrid chrome before Begin so WindowBg / borders apply.
            ui_hybrid_chrome::push();
            cataimgui::window::draw();
            // window::draw() may BringWindowToDisplayFront(Equipment) after
            // draw_controls; re-front RMB context popups so they stay visible
            // above the Character Equipment panel.
            refront_ctx_popup( doll_ctx_popup_id );
            refront_ctx_popup( inv_ctx_popup_id );
            ui_hybrid_chrome::pop();
        }
        void draw_controls() override;
        cataimgui::bounds get_bounds() override {
            const ImVec2 vp = ImGui::GetMainViewport()->Size;
            const float scale = std::max( 1.f, ImGui::GetFontSize() / 16.f );
            const float width = std::min( vp.x * 0.96f, 2800.f * scale );
            const float height = std::min( vp.y * 0.92f, 1600.f * scale );
            const ImVec2 origin = ImGui::GetMainViewport()->Pos;
            // Center explicitly: this tree uses available height rather than the
            // shared auto-centered dialog cap, while keeping viewport margins.
            return { origin.x + ( vp.x - width ) * 0.5f, origin.y + ( vp.y - height ) * 0.5f,
                     width, height };
        }

    private:
        Character *you = nullptr;
        std::vector<doll_slot> slots;
        bool scroll_to_active_group = false;
        int selected_slot = -1;
        item_location selected_inv;
        item_location selected_worn;
        item_location drag_payload; // owned for ImGui drag-drop lifetime
        // Reload (select_ammo) must run AFTER the ImGui equipment window is
        // destroyed — nesting the ammo picker under an open Hybrid panel made
        // select_ammo return empty for MAGAZINE_WELL tools (fire_drill).
        item_location deferred_reload_loc;
        std::string last_action;
        bool open_layers = false;
        input_context ctxt;
        bool open_classic = false;
        bool open_storage = false;
        bool want_close = false;
        std::string status_line;
        pending_action pending = pending_action::none;
        // Non-empty when context menu picked a typed use_methods key (Turn on/off).
        std::string pending_use_method;
        // Temporary RMB telemetry / shared popup target (inv + doll).
        std::string rmb_telem_line;
        item_location ctx_menu_loc;
        bool ctx_menu_from_worn = false;
        int ctx_menu_slot = -1;
        // Parent-scoped ImGuiIDs for ##Popup_%08x re-front after draw().
        ImGuiID doll_ctx_popup_id = 0;
        ImGuiID inv_ctx_popup_id = 0;
        // RMB press slot/cell: open context only when release hits the same id
        // (Deck stick/finger drift otherwise opens the neighbor under RMB_UP).
        int doll_rmb_down_slot = -1;
        int inv_rmb_down_cell = -1;

        static void refront_ctx_popup( ImGuiID id ) {
            if( id == 0 ) {
                return;
            }
            char name[32];
            std::snprintf( name, sizeof( name ), "##Popup_%08x",
                           static_cast<unsigned>( id ) );
            ImGuiWindow *w = ImGui::FindWindowByName( name );
            if( w != nullptr && w->Active ) {
                ImGui::BringWindowToDisplayFront( w );
            }
        }

        void draw_paper_doll();
        void accept_equipment_drop( int slot, const item_location &target = item_location::nowhere );
        void draw_survivor( const ImVec2 &min, const ImVec2 &max );
        void draw_equipment_inspection();
        bool equip_preview = false;
        item_location preview_item;
        int preview_slot = -1;
        item_context_menu::action inspector_action = item_context_menu::action::none;
        item_location inspector_item;
        std::string inspector_method;
        char inventory_filter[128] = "";
        int inventory_category = 0;
        bool inventory_list = false; // icon grid by default; the toggle switches to the list
        bool show_nearby = false;
        bool focus_inventory_search = false;
        std::set<int> expanded_slots;
        struct tree_node {
            doll_group group;
            int slot = -1;
            item_location location;
        };
        std::vector<tree_node> visible_nodes;
        int tree_focus = 0;
        void activate_tree_node( const tree_node &node, bool toggle );
        doll_group active_group = doll_group::overview;
        bool focus_changed = false;
        bool overview_keyboard_focus = false;
        void select_group( doll_group group );
        void draw_inventory_grid();
        void draw_action_bar();
        void try_equip_selected();
        void try_takeoff_selected();
        void try_wield_selected();
        void try_use_selected( const std::string &use_method = {} );
        void try_drop_selected();
        void try_examine_selected();
        void try_drag_equip();
        void refresh_selection_validity();
        void clear_selections();
        void flush_pending_action();
        /** Keyboard/controller-style directional nav across doll overview slots. */
        void navigate_doll( int dcol, int drow );
        int focus_list_index = 0; // item list within selected region
};

void rpg_equipment_window::clear_selections()
{
    equip_preview = false;
    selected_inv = item_location::nowhere;
    selected_worn = item_location::nowhere;
}

bool rpg_equipment_window::execute()
{
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM", to_translation( "Wear / wield selected" ) );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "EXAMINE" );
    ctxt.register_action( "UP", to_translation( "Move doll selection up" ) );
    ctxt.register_action( "DOWN", to_translation( "Move doll selection down" ) );
    ctxt.register_action( "LEFT", to_translation( "Move doll selection left" ) );
    ctxt.register_action( "RIGHT", to_translation( "Move doll selection right" ) );
    ctxt.register_action( "PAGE_UP", to_translation( "Previous item in region list" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Next item in region list" ) );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.set_timeout( 16 );

    while( true ) {
        ui_manager::redraw_invalidated();
        // Inventory mutations must run after ImGui finishes the frame so tooltips /
        // remaining grid cells never touch dangling item_location pointers.
        flush_pending_action();
        const bool popup_open = ImGui::IsPopupOpen( nullptr,
                                ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel );
        last_action = ctxt.handle_input();
        if( popup_open && last_action == "QUIT" ) {
            last_action = "TIMEOUT";
        }

        if( want_close ) {
            break;
        }
        if( open_classic || open_storage || open_layers ) {
            break;
        }
        if( last_action == "QUIT" && !cataimgui::client::want_text_input() ) {
            if( equip_preview ) {
                equip_preview = false;
                status_line = _( "Equipment change cancelled." );
                continue;
            }
            if( active_group != doll_group::overview ) {
                active_group = doll_group::overview;
                clear_selections();
                continue;
            }
        }
        if( ( last_action == "QUIT" && !cataimgui::client::want_text_input() ) || !get_is_open() ) {
            break;
        }
        if( last_action == "EXAMINE" && !cataimgui::client::want_text_input() ) {
            inspector_item = selected_inv ? selected_inv : selected_worn;
            inspector_action = item_context_menu::action::more_actions;
        }
        if( !cataimgui::client::want_text_input() && !popup_open ) {
            if( last_action == "UP" ) {
                navigate_doll( 0, -1 );
            } else if( last_action == "DOWN" ) {
                navigate_doll( 0, 1 );
            } else if( last_action == "LEFT" ) {
                navigate_doll( -1, 0 );
            } else if( last_action == "RIGHT" ) {
                navigate_doll( 1, 0 );
            } else if( last_action == "PAGE_UP" || last_action == "PAGE_DOWN" ) {
                if( selected_slot >= 0 && selected_slot < static_cast<int>( slots.size() ) ) {
                    const std::vector<item_location> items =
                        items_on_slot( *you, slots[selected_slot] );
                    if( !items.empty() ) {
                        const int delta = ( last_action == "PAGE_DOWN" ) ? 1 : -1;
                        focus_list_index = ( focus_list_index + delta +
                                             static_cast<int>( items.size() ) ) %
                                           static_cast<int>( items.size() );
                        selected_worn = items[focus_list_index];
                        selected_inv = item_location::nowhere;
                    }
                }
            }
        }
        if( last_action == "ANY_INPUT" && !cataimgui::client::want_text_input() ) {
            const input_event &event = ctxt.get_raw_input();
            if( event.type == input_event_t::keyboard_char && !event.sequence.empty() ) {
                for( const item_location &loc : you->all_items_loc() ) {
                    if( loc && loc->invlet && loc->invlet == event.sequence.front() ) {
                        selected_inv = loc;
                        selected_worn = item_location::nowhere;
                        inspector_item = loc;
                        inspector_action = item_context_menu::action::more_actions;
                        break;
                    }
                }
            }
        }
        if( last_action == "CONFIRM" && !popup_open && !cataimgui::client::want_text_input() ) {
            if( equip_preview ) {
                pending = pending_action::drag_equip;
                equip_preview = false;
            } else if( !selected_inv && !visible_nodes.empty() ) {
                activate_tree_node( visible_nodes[std::clamp( tree_focus, 0,
                                                              static_cast<int>( visible_nodes.size() ) - 1 )], true );
            } else if( selected_worn && !selected_inv ) {
                inspector_item = selected_worn;
                inspector_action = item_context_menu::action::more_actions;
            } else {
                pending = selected_slot >= 0 ? pending_action::drag_equip : pending_action::equip;
            }
        }
    }

    return false;
}

void rpg_equipment_window::select_group( doll_group group )
{
    active_group = group;
    scroll_to_active_group = true;
    clear_selections();
    for( int i = 0; i < static_cast<int>( slots.size() ); ++i ) {
        if( is_overview_slot( slots[i] ) && group_for( slots[i].region ) == group &&
            ( group != doll_group::body || slots[i].region == equipment_layout::region_id::torso ) ) {
            selected_slot = i;
            const auto items = items_on_slot( *you, slots[i] );
            selected_worn = items.empty() ? item_location::nowhere : items.front();
            focus_list_index = 0;
            break;
        }
    }
    status_line.clear();
}

void rpg_equipment_window::activate_tree_node( const tree_node &node, bool toggle )
{
    if( node.slot < 0 ) {
        active_group = toggle && active_group == node.group ? doll_group::overview : node.group;
        clear_selections();
        selected_slot = -1;
        return;
    }
    selected_slot = node.slot;
    selected_inv = item_location::nowhere;
    const auto items = items_on_slot( *you, slots[node.slot] );
    selected_worn = node.location ? node.location :
                    items.empty() ? item_location::nowhere : items.front();
    if( toggle && node.location ) {
        inspector_item = node.location;
        inspector_action = item_context_menu::action::more_actions;
    } else if( toggle ) {
        if( expanded_slots.count( node.slot ) ) {
            expanded_slots.erase( node.slot );
        } else {
            expanded_slots.insert( node.slot );
        }
    }
}

void rpg_equipment_window::navigate_doll( int dcol, int drow )
{
    if( visible_nodes.empty() ) {
        return;
    }
    const int count = static_cast<int>( visible_nodes.size() );
    tree_focus = std::clamp( tree_focus, 0, count - 1 );
    if( drow ) {
        tree_focus = ( tree_focus + drow + count ) % count;
    } else {
        const tree_node &node = visible_nodes[tree_focus];
        if( dcol > 0 ) {
            activate_tree_node( node, false );
            if( node.slot >= 0 ) {
                expanded_slots.insert( node.slot );
            }
        } else if( node.slot >= 0 && expanded_slots.count( node.slot ) ) {
            expanded_slots.erase( node.slot );
        } else {
            active_group = doll_group::overview;
            tree_focus = 0;
        }
    }
    selected_inv = item_location::nowhere;
    overview_keyboard_focus = true;
    focus_changed = true;
}

void rpg_equipment_window::flush_pending_action()
{
    if( pending == pending_action::none && inspector_action == item_context_menu::action::none ) {
        return;
    }
    const ui_telemetry::scope trace( "equipment.action", {
        { "pending", std::to_string( static_cast<int>( pending ) ) },
        { "inspector", std::to_string( static_cast<int>( inspector_action ) ) },
        { "slot", std::to_string( selected_slot ) },
        { "item", selected_inv ? selected_inv->typeId().str() : "" },
        { "worn", selected_worn ? selected_worn->typeId().str() : "" }
    }, pending != pending_action::none || inspector_action != item_context_menu::action::none );
    // Legacy prompts (quantities, confirmations and some item actions) draw
    // beneath ImGui, so suspend the equipment shell while they own input.
    restore_on_out_of_scope<bool> restore_visibility( hide_ui );
    hide_ui = true;
    if( inspector_action != item_context_menu::action::none ) {
        const item_context_menu::action act = inspector_action;
        inspector_action = item_context_menu::action::none;
        const item_location loc = inspector_item;
        inspector_item = item_location::nowhere;
        if( act == item_context_menu::action::reload ) {
            deferred_reload_loc = loc;
            want_close = true;
            return;
        }
        status_line = item_context_menu::perform( *you, loc, act, inspector_method );
        inspector_method.clear();
        refresh_selection_validity();
    }
    const pending_action act = pending;
    pending = pending_action::none;
    const std::string use_method = std::move( pending_use_method );
    pending_use_method.clear();
    switch( act ) {
        case pending_action::equip:
            try_equip_selected();
            break;
        case pending_action::takeoff:
            try_takeoff_selected();
            break;
        case pending_action::wield:
            try_wield_selected();
            break;
        case pending_action::use_item:
            try_use_selected( use_method );
            break;
        case pending_action::drop_item:
            try_drop_selected();
            break;
        case pending_action::examine_item:
            try_examine_selected();
            break;
        case pending_action::drag_equip:
            try_drag_equip();
            break;
        case pending_action::ctx_consume:
        case pending_action::ctx_read:
        case pending_action::ctx_unload:
        case pending_action::ctx_reload:
        case pending_action::ctx_wear:
        case pending_action::ctx_pickup: {
            refresh_selection_validity();
            item_location loc = selected_inv;
            if( ( !loc || !loc.get_item() ) && selected_worn ) {
                loc = selected_worn;
            }
            if( !loc || !loc.get_item() ) {
                status_line = _( "Select an item first." );
                break;
            }
            item_context_menu::action ctx = item_context_menu::action::none;
            switch( act ) {
                case pending_action::ctx_consume:
                    ctx = item_context_menu::action::consume;
                    break;
                case pending_action::ctx_read:
                    ctx = item_context_menu::action::read;
                    break;
                case pending_action::ctx_unload:
                    ctx = item_context_menu::action::unload;
                    break;
                case pending_action::ctx_reload:
                    ctx = item_context_menu::action::reload;
                    break;
                case pending_action::ctx_wear:
                    ctx = item_context_menu::action::wear;
                    break;
                case pending_action::ctx_pickup:
                    ctx = item_context_menu::action::pickup;
                    break;
                default:
                    break;
            }
            // Nested use/read UIs need the equipment window closed first;
            // otherwise menus cancel under the still-open panel.
            // Consume (Eat/Drink/Take) is direct — stay in Equipment.
            // Reload is fully deferred until after execute() tears down the
            // window (select_ammo nested under Hybrid returned empty for
            // fire_drill + notched_stick even with want_close set mid-flush).
            if( ctx == item_context_menu::action::read ||
                ctx == item_context_menu::action::use ||
                ctx == item_context_menu::action::reload ) {
                want_close = true;
            }
            if( ctx == item_context_menu::action::reload ) {
                deferred_reload_loc = loc;
                clear_selections();
                status_line = _( "Reloading…" );
                rpg_eq_ctx_telem( rmb_telem_line, string_format(
                                      "flush ctx_reload deferred want_close=1 t='%s'",
                                      rpg_eq_short_tname( loc ) ) );
                break;
            }
            clear_selections();
            status_line = item_context_menu::perform( *you, loc, ctx );
            break;
        }
        case pending_action::none:
            break;
    }
}

void rpg_equipment_window::refresh_selection_validity()
{
    if( selected_inv && !selected_inv.get_item() ) {
        selected_inv = item_location::nowhere;
    }
    if( selected_worn && !selected_worn.get_item() ) {
        selected_worn = item_location::nowhere;
    }
    if( drag_payload && !drag_payload.get_item() ) {
        drag_payload = item_location::nowhere;
    }
}

void rpg_equipment_window::try_equip_selected()
{
    refresh_selection_validity();
    if( !selected_inv ) {
        status_line = _( "Select an inventory item first." );
        return;
    }

    item &it = *selected_inv;
    if( selected_slot >= 0 && selected_slot < static_cast<int>( slots.size() ) &&
        slots[selected_slot].type == doll_slot::kind::offhand &&
        !( it.has_flag( flag_BLOCK_WHILE_WORN ) && it.has_flag( flag_RESTRICT_HANDS ) ) ) {
        status_line = _( "The off-hand slot takes a shield." );
        return;
    }

    // Weapon slot selected → prefer wield
    if( selected_slot >= 0 && selected_slot < static_cast<int>( slots.size() ) &&
        slots[selected_slot].type == doll_slot::kind::weapon ) {
        try_wield_selected();
        return;
    }

    if( it.is_armor() || it.is_pet_armor() ) {
        const ret_val<void> can = you->can_wear( it );
        if( !can.success() ) {
            status_line = can.str();
            add_msg( m_info, can.str() );
            return;
        }
        item_location loc = selected_inv;
        clear_selections();
        if( you->wear( loc ) ) {
            status_line = _( "Worn." );
        } else {
            status_line = _( "Could not wear that." );
        }
        return;
    }

    // Non-armor: try wield
    try_wield_selected();
}

void rpg_equipment_window::try_drag_equip()
{
    refresh_selection_validity();
    if( !selected_inv ) {
        status_line = _( "Nothing to equip." );
        return;
    }
    if( selected_slot < 0 || selected_slot >= static_cast<int>( slots.size() ) ) {
        try_equip_selected();
        return;
    }
    const doll_slot &slot = slots[selected_slot];
    if( slot.type == doll_slot::kind::offhand &&
        !( selected_inv->has_flag( flag_BLOCK_WHILE_WORN ) &&
           selected_inv->has_flag( flag_RESTRICT_HANDS ) ) ) {
        status_line = _( "The off-hand slot takes a shield." );
        return;
    }
    // Dropping a compatible magazine/ammo onto an occupied doll slot (e.g.
    // notched stick → bow fire drill) should Reload, not try to replace the
    // wielded item (which only offers Store/Drop/Wear via dispose_item).
    item_location on_slot = selected_worn && slot_matches( *selected_worn, slot ) ?
                            selected_worn : item_on_slot( *you, slot );
    using equipment_layout::region_id;
    const bool holder_region =
        slot.type == doll_slot::kind::scabbard ||
        slot.type == doll_slot::kind::sheath ||
        slot.type == doll_slot::kind::holster ||
        slot.region == region_id::waist ||
        slot.region == region_id::back_weapon;

    const auto try_store_in = [&]( const item_location & holder ) -> bool {
        if( !holder || !holder.get_item() || holder == selected_inv )
        {
            return false;
        }
        // Must be an equipped holder (holster use), not an arbitrary container.
        const use_function *use = holder->type->get_use( "holster" );
        const auto *actor = use ? dynamic_cast<const holster_actor *>( use->get_actor_ptr() ) : nullptr;
        if( actor == nullptr )
        {
            return false;
        }
        if( !actor->can_holster( *holder, *selected_inv ) )
        {
            status_line = _( "That holder cannot store this item (incompatible or full)." );
            return true; // handled (rejected)
        }
        item_location source = selected_inv;
        item_location dest_holder = holder;
        clear_selections();
        status_line = actor->store( *you, *dest_holder, *source ) ?
        _( "Stored." ) : _( "Could not store that item." );
        return true;
    };

    if( holder_region && selected_inv && selected_inv.get_item() ) {
        const equipment_layout::item_profile inv_profile =
            equipment_layout::classify_item( *selected_inv );
        const bool equipping_holder =
            inv_profile.role == equipment_layout::equipment_role::holder ||
            ( selected_inv->is_armor() && slot_matches( *selected_inv, slot ) &&
              inv_profile.storage != equipment_layout::storage_slot::none );
        if( !equipping_holder ) {
            // Resolve the exact selected equipped holder independently of
            // presentation kind (anatomical Waist/Back are kind::region).
            item_location holder = item_location::nowhere;
            if( selected_worn && selected_worn.get_item() &&
                selected_worn->type->get_use( "holster" ) &&
                slot_matches( *selected_worn, slot ) ) {
                holder = selected_worn;
            } else if( on_slot && on_slot.get_item() &&
                       on_slot->type->get_use( "holster" ) ) {
                holder = on_slot;
            } else {
                // Prefer an exact holder on this region / waist position.
                for( const item_location &cand : items_on_slot( *you, slot ) ) {
                    if( cand && cand->type->get_use( "holster" ) ) {
                        holder = cand;
                        break;
                    }
                }
                // Back (weapon) / waist: also search other holders on the region.
                if( !holder && ( slot.region == region_id::waist ||
                                 slot.region == region_id::back_weapon ) ) {
                    const equipment_layout::character_map mapped =
                        equipment_layout::map_character( *you );
                    for( const item_location &cand :
                         equipment_layout::items_equipping_to( mapped, slot.region ) ) {
                        if( cand && cand->type->get_use( "holster" ) ) {
                            holder = cand;
                            break;
                        }
                    }
                }
            }
            if( holder ) {
                if( try_store_in( holder ) ) {
                    return;
                }
            } else if( !selected_inv->is_armor() ) {
                status_line = _( "No equipped holder in this slot to store that item." );
                return;
            }
        }
    }
    if( on_slot && on_slot.get_item() && selected_inv.get_item() &&
        on_slot.get_item() != selected_inv.get_item() ) {
        const bool accepts = on_slot->can_reload_with( *selected_inv, /*now=*/true );
        rpg_eq_ctx_telem( rmb_telem_line, string_format(
                              "drag_equip onto '%s' payload='%s' can_reload=%d",
                              rpg_eq_short_tname( on_slot ),
                              rpg_eq_short_tname( selected_inv ),
                              accepts ? 1 : 0 ) );
        if( accepts ) {
            item_location ammo = selected_inv;
            item_location target = on_slot;
            clear_selections();
            // Direct reload_option — known ammo source, no nested picker needed.
            item::reload_option opt( you, target, ammo,
                                     item::reload_option::POCKET_FALLBACK );
            if( opt && opt.ammo.get_item() != nullptr ) {
                you->assign_activity( reload_activity_actor( std::move( opt ) ) );
                status_line = _( "Reloading…" );
                want_close = true;
                return;
            }
            // Fallback: deferred select_ammo on the target after close.
            deferred_reload_loc = target;
            status_line = _( "Reloading…" );
            want_close = true;
            return;
        }
    }
    if( slot.type == doll_slot::kind::weapon ) {
        try_wield_selected();
        return;
    }
    if( !slot_matches( *selected_inv, slot ) ) {
        status_line = _( "That item does not belong in this slot or clothing layer." );
        return;
    }
    // Offhand / body / outer / back → wear covering that part when possible.
    item &it = *selected_inv;
    if( it.is_armor() || it.is_pet_armor() ) {
        const ret_val<void> can = you->can_wear( it );
        if( !can.success() ) {
            status_line = can.str();
            add_msg( m_info, can.str() );
            return;
        }
        const std::optional<side> want_side = equipment_layout::side_intent_for( slot.region );
        item_location loc = selected_inv;
        const bool sided = loc->is_sided();
        clear_selections();
        if( want_side && sided ) {
            const auto result = equipment_actions::wear_on_side( *you, loc, *want_side );
            status_line = result.success() ? string_format( _( "Equipped to %s." ), slot.label ) : result.str();
        } else {
            status_line = you->wear( loc ) ? string_format( _( "Equipped to %s." ), slot.label ) :
                          _( "Could not wear that." );
        }
        return;
    }
    status_line = _( "This slot takes wearable equipment." );
}

void rpg_equipment_window::try_wield_selected()
{
    refresh_selection_validity();
    if( !selected_inv ) {
        status_line = _( "Select an inventory item to wield." );
        return;
    }
    const ret_val<void> can = you->can_wield( *selected_inv );
    if( !can.success() ) {
        status_line = can.str();
        add_msg( m_info, can.str() );
        return;
    }
    item_location loc = selected_inv;
    clear_selections();
    if( you->wield( loc ) ) {
        status_line = _( "Wielded." );
    } else {
        status_line = _( "Could not wield that." );
    }
}

void rpg_equipment_window::try_takeoff_selected()
{
    refresh_selection_validity();
    item_location loc = selected_worn;
    if( !loc && selected_slot >= 0 && selected_slot < static_cast<int>( slots.size() ) ) {
        loc = item_on_slot( *you, slots[selected_slot] );
    }
    if( !loc || !loc.get_item() ) {
        status_line = _( "Select a worn / wielded item to remove." );
        return;
    }

    if( you->is_wielding( *loc ) ) {
        const auto result = equipment_actions::unwield_to_inventory( *you );
        if( result.success() ) {
            status_line = _( "Unwielded." );
            clear_selections();
        } else {
            status_line = result.str();
        }
        return;
    }

    if( !you->is_worn( *loc ) ) {
        status_line = _( "That item is not worn." );
        return;
    }
    const ret_val<void> can = you->can_takeoff( *loc );
    if( !can.success() ) {
        status_line = can.str();
        add_msg( m_info, can.str() );
        return;
    }
    item_location obtained = loc.obtain( *you );
    clear_selections();
    if( you->takeoff( obtained ) ) {
        status_line = _( "Taken off." );
    } else {
        status_line = _( "Could not take that off." );
    }
}

void rpg_equipment_window::try_use_selected( const std::string &use_method )
{
    refresh_selection_validity();
    if( !selected_inv || !selected_inv.get_item() ) {
        status_line = _( "Select an inventory item to use." );
        return;
    }
    avatar *av = you->as_avatar();
    if( av == nullptr ) {
        status_line = _( "Only the player can use items here." );
        return;
    }
    item_location loc = selected_inv;
    clear_selections();
    // Close before nested use UIs / activities (eat, apply, read) take over.
    want_close = true;
    if( !use_method.empty() ) {
        avatar_action::use_item( *av, loc, use_method );
        status_line = _( "Used." );
        return;
    }
    if( loc->is_comestible() || loc->is_medical_tool() ) {
        avatar_action::eat_or_use( *av, loc );
        status_line = _( "Using…" );
        return;
    }
    if( loc->is_book() ) {
        av->read( loc );
        status_line = _( "Reading…" );
        return;
    }
    avatar_action::use_item( *av, loc );
    status_line = _( "Used." );
}

void rpg_equipment_window::try_drop_selected()
{
    refresh_selection_validity();
    if( !selected_inv || !selected_inv.get_item() ) {
        status_line = _( "Select an inventory item to drop." );
        return;
    }
    const ret_val<void> can = you->can_drop( *selected_inv );
    if( !can.success() ) {
        status_line = can.str();
        add_msg( m_info, can.str() );
        return;
    }
    item_location loc = selected_inv;
    clear_selections();
    you->drop( loc, you->pos_bub() );
    status_line = _( "Dropped." );
}

void rpg_equipment_window::try_examine_selected()
{
    refresh_selection_validity();
    item_location loc = selected_inv;
    if( !loc || !loc.get_item() ) {
        loc = selected_worn;
    }
    if( !loc || !loc.get_item() ) {
        status_line = _( "Select an item to examine." );
        return;
    }
    // Owned strings for the info window title (SIGSEGV-safe).
    std::vector<iteminfo> vThisItem;
    std::vector<iteminfo> vDummy;
    loc->info( true, vThisItem );
    const std::string title = loc->tname( 1, tname::unprefixed_tname, true );
    const std::string type_nm = loc->type_name();
    item_info_data data( title, type_nm, vThisItem, vDummy );
    data.handle_scrolling = true;
    data.arrow_scrolling = true;
    const int maxwidth = std::max( FULL_SCREEN_WIDTH, TERMX );
    const int width = std::min( 80, maxwidth );
    iteminfo_window info_window( data, point( maxwidth / 2 - width / 2, -1 ), width, 0 );
    info_window.execute();
    status_line = _( "Examined." );
}

// The survivor preview uses the active tileset's character and clothing overlays.
// It never paints an invented equipment silhouette or alters the world renderer.
void rpg_equipment_window::draw_survivor( const ImVec2 &min, const ImVec2 &max )
{
#if defined(TILES)
    if( !tilecontext || !get_option<bool>( "USE_TILES" ) ) {
        return;
    }
    // Portrait pack first: a pack that defines player_male / player_female (any
    // resolution, optionally "animated" with several fg frames for an idle
    // loop) replaces the 32 px map doll. Overlays the pack lacks are skipped.
    // Otherwise the map tileset's doll with its worn/mutation overlays.
    std::vector<texture_draw_data> layers;
    if( portrait_tilecontext && portrait_tilecontext->is_valid() ) {
        layers = portrait_tilecontext->get_character_preview( *you );
    }
    if( layers.empty() ) {
        layers = tilecontext->get_character_preview( *you );
    }
    if( layers.empty() ) {
        return;
    }
    float left = 0.f, top = 0.f, right = 32.f, bottom = 32.f;
    for( const auto &layer : layers ) {
        left = std::min( left, float( layer.offset.x ) );
        top = std::min( top, float( layer.offset.y ) );
        right = std::max( right, layer.offset.x + layer.dimensions.w * layer.pixelscale );
        bottom = std::max( bottom, layer.offset.y + layer.dimensions.h * layer.pixelscale );
    }
    const float scale = std::max( 0.1f, std::min( ( max.x - min.x ) / ( right - left ),
                                  ( max.y - min.y ) / ( bottom - top ) ) );
    const ImVec2 anchor( ( min.x + max.x - ( right + left ) * scale ) * 0.5f,
                         ( min.y + max.y - ( bottom + top ) * scale ) * 0.5f );
    ImDrawList *draw = ImGui::GetWindowDrawList();
    for( const auto &layer : layers ) {
        const ImVec2 p0( anchor.x + layer.offset.x * scale, anchor.y + layer.offset.y * scale );
        const ImVec2 p1( p0.x + layer.dimensions.w * layer.pixelscale * scale,
                         p0.y + layer.dimensions.h * layer.pixelscale * scale );
        draw->AddImage( reinterpret_cast<ImTextureID>( layer.texture ), p0, p1,
                        ImVec2( layer.uv0.first, layer.uv0.second ),
                        ImVec2( layer.uv1.first, layer.uv1.second ) );
    }
#else
    ( void )min;
    ( void )max;
#endif
}

void rpg_equipment_window::draw_equipment_inspection()
{
    refresh_selection_validity();
    item_location loc = selected_inv ? selected_inv : selected_worn;
    if( !loc ) {
        ImGui::TextWrapped( "%s",
                            _( "Select an item to inspect it." ) );
        return;
    }
    const item_context_menu::action action = item_context_menu::draw_inspector( *you, loc,
            &inspector_method );
    if( action != item_context_menu::action::none ) {
        inspector_action = action;
        inspector_item = loc;
    }
    if( selected_slot >= 0 && selected_slot < static_cast<int>( slots.size() ) &&
        slots[selected_slot].bp != bodypart_str_id::NULL_ID().id() && loc->is_armor() ) {
        const bodypart_id bp = slots[selected_slot].bp;
        if( loc->covers( bp ) ) {
            const item_location current = selected_worn;
            ImGui::Text( "%s", string_format( _( "%s: encumbrance %d   warmth %d   coverage %d%%" ),
                                              slots[selected_slot].label, loc->get_encumber( *you, bp ), loc->get_warmth( bp ),
                                              loc->get_coverage( bp ) ).c_str() );
            ImGui::Text( "%s", string_format( _( "Protection: bash %.1f   cut %.1f" ),
                                              loc->resist( damage_type_id( "bash" ), false, bp ),
                                              loc->resist( damage_type_id( "cut" ), false, bp ) ).c_str() );
            if( selected_inv && current && current != loc ) {
                ImGui::Text( "%s", string_format( _( "Compared with %s: encumbrance %+d   warmth %+d" ),
                                                  current->type_name(), loc->get_encumber( *you, bp ) - current->get_encumber( *you, bp ),
                                                  loc->get_warmth( bp ) - current->get_warmth( bp ) ).c_str() );
                ImGui::TextDisabled( "%s",
                                     _( "Wearing adds a layer; existing clothing stays on. Layering can add extra encumbrance." ) );
            }
        } else {
            ImGui::TextDisabled( "%s", _( "This item does not cover the selected body part." ) );
        }
    }
}

void rpg_equipment_window::accept_equipment_drop( int slot, const item_location &target )
{
    if( !ImGui::BeginDragDropTarget() ) {
        return;
    }
    if( const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(
                                          "RPG_EQ_ITEM", ImGuiDragDropFlags_AcceptBeforeDelivery ) ) {
        if( payload->IsDelivery() && drag_payload && drag_payload.get_item() ) {
            // Releasing the drag is the confirmation: the change applies now.
            selected_inv = drag_payload;
            selected_slot = slot;
            selected_worn = target ? target :
                            slot >= 0 ? item_on_slot( *you, slots[slot] ) : item_location::nowhere;
            equip_preview = false;
            preview_item = item_location::nowhere;
            preview_slot = -1;
            pending = pending_action::drag_equip;
            status_line = string_format( _( "Equipping %s…" ), selected_inv->type_name() );
            ui_telemetry::record( "equipment.drop", {
                { "type", selected_inv->typeId().str() },
                { "target", slot >= 0 ? slots[slot].label : "survivor" }
            } );
        }
    }
    ImGui::EndDragDropTarget();
}

void rpg_equipment_window::draw_paper_doll()
{
    namespace w = ui_hybrid_widgets;
    namespace theme = ui_hybrid_chrome::theme;
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    const float s = theme::scale();
    const float gap = tk.lg * s;
    const float available_width = ImGui::GetContentRegionAvail().x;
    // Portrait column: wide enough for a readable survivor and hand cards, never
    // a narrow strip stranded in a tall panel.
    const float portrait_width = std::clamp( available_width * 0.2f, 220.f * s, 420.f * s );
    const float tree_width = ( available_width - portrait_width - gap * 2.f ) * 0.5f;
    bool doll_ctx_request = false;
    int doll_ctx_index = -1;
    item_location doll_ctx_loc;
    const auto mapped = equipment_layout::map_character( *you );
    visible_nodes.clear();

    const auto request_context = [&]( int slot, const item_location & loc ) {
        if( loc ) {
            doll_ctx_request = true;
            doll_ctx_index = slot;
            doll_ctx_loc = loc;
        }
    };
    const auto sprite_painter = [this]( const item * it ) -> w::icon_painter {
        if( it == nullptr ) {
            return nullptr;
        }
        return [it]( ImDrawList *, const ImVec2 & min, const ImVec2 & max ) {
            draw_equipment_icon( *it, min, max );
        };
    };

    // ---- left column: live survivor, hand cards, other equipment -------------
    // These controls remain visible while either the equipment tree or inventory scrolls.
    ImGui::BeginChild( "survivor_column", ImVec2( portrait_width, 0.f ),
                       ImGuiChildFlags_None, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollbar );
    const float left_width = ImGui::GetContentRegionAvail().x;
    const float card_h = 88.f * s;
    const float other_h = tk.button * s;
    const float reserved = card_h * 2.f + other_h + tk.md * s * 2.f + tk.xs * s * 2.f +
                           ImGui::GetStyle().ItemSpacing.y * 6.f;
    // The portrait keeps a portrait-shaped frame (about 4:5) instead of stretching
    // to whatever the column leaves free; the hand cards follow directly below.
    const float portrait_height = std::clamp( ImGui::GetContentRegionAvail().y - reserved,
                                  160.f * s, left_width * 1.25f );
    const ImVec2 portrait_min = ImGui::GetCursorScreenPos();
    const ImVec2 portrait_max( portrait_min.x + left_width, portrait_min.y + portrait_height );
    ImVec2 inner_min;
    ImVec2 inner_max;
    w::portrait_frame( ImGui::GetWindowDrawList(), portrait_min, portrait_max, inner_min, inner_max );
    draw_survivor( inner_min, inner_max );
    ImGui::InvisibleButton( "survivor_drop", ImVec2( left_width, portrait_height ) );
    accept_equipment_drop( -1 );
    if( ImGui::IsItemHovered() && drag_payload ) {
        w::tooltip( _( "Drop here to equip on the best matching slot." ) );
    }
    ImGui::Dummy( ImVec2( 0.f, tk.md * s - ImGui::GetStyle().ItemSpacing.y ) );
    for( int i = 0; i < static_cast<int>( slots.size() ); ++i ) {
        if( slots[i].type != doll_slot::kind::weapon && slots[i].type != doll_slot::kind::offhand ) {
            continue;
        }
        const auto items = items_on_slot( *you, slots[i] );
        const item_location occupant = items.empty() ? item_location::nowhere : items.front();
        ImGui::PushID( i );
        const bool selected = selected_slot == i;
        // One hit target per card; the card art is drawn underneath.
        ImGui::PushStyleColor( ImGuiCol_Button, 0 );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, 0 );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, 0 );
        ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.f );
        if( ImGui::Button( "##held_slot", ImVec2( left_width, card_h ) ) ) {
            selected_slot = i;
            selected_worn = occupant;
            selected_inv = item_location::nowhere;
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor( 3 );
        const bool hovered = ImGui::IsItemHovered();
        if( hovered && ImGui::IsMouseReleased( ImGuiMouseButton_Right ) ) {
            request_context( i, occupant );
        }
        accept_equipment_drop( i );
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        ImDrawList *draw = ImGui::GetWindowDrawList();
        {
            // card art
            ui_hybrid_textures::texture card = theme::level() != ui_hybrid_chrome::decoration::none ?
                                               ui_hybrid_textures::get( ui_hybrid_textures::asset::card ) : ui_hybrid_textures::texture{};
            if( !ui_hybrid_textures::draw_nine_slice( draw, card, min, max, 16.f, 0.5f * s,
                    hovered ? IM_COL32( 255, 255, 255, 255 ) : IM_COL32( 235, 235, 235, 255 ) ) ) {
                draw->AddRectFilled( min, max, hovered ? tk.raised_hover : tk.raised, tk.radius_panel * s );
                draw->AddRect( min, max, tk.edge_quiet, tk.radius_panel * s );
            }
            if( selected ) {
                draw->AddRectFilled( min, ImVec2( max.x, min.y + 1.f * s ), tk.accent );
                draw->AddRectFilled( ImVec2( min.x, max.y - 1.f * s ), max, tk.accent );
                draw->AddRectFilled( min, ImVec2( min.x + 1.f * s, max.y ), tk.accent );
                draw->AddRectFilled( ImVec2( max.x - 1.f * s, min.y ), max, tk.accent );
            }
        }
        draw->PushClipRect( min, max, true );
        const float pad = tk.md * s;
        draw->AddText( ImVec2( min.x + pad, min.y + tk.sm * s ), tk.text_muted, slots[i].label.c_str() );
        const float isz = tk.icon_featured * s;
        const float icon_y = min.y + card_h - pad - isz;
        if( occupant ) {
            draw_equipment_icon( *occupant, ImVec2( min.x + pad, icon_y ), ImVec2( min.x + pad + isz, icon_y + isz ) );
        } else {
            ui_hybrid_textures::draw_icon( draw, slots[i].type == doll_slot::kind::weapon ? "cat_main_hand" :
                                           "cat_off_hand", ImVec2( min.x + pad, icon_y ), isz, tk.edge_quiet );
        }
        const bool reserved_hand = slots[i].type == doll_slot::kind::offhand && occupant &&
                                   you->is_wielding( *occupant );
        const std::string label = reserved_hand ? _( "Uses both hands" ) :
                                  occupant ? remove_color_tags( occupant->type_name() ) : _( "Empty" );
        w::push_font_section();
        const float fs = ImGui::GetFontSize();
        const float text_x = min.x + pad + isz + tk.md * s;
        const std::string fitted = w::fit_text( label, max.x - pad - text_x );
        draw->AddText( ImGui::GetFont(), fs, ImVec2( text_x, icon_y + ( isz - fs ) * 0.5f ),
                       occupant && !reserved_hand ? tk.text : tk.text_muted, fitted.c_str() );
        w::pop_font();
        draw->PopClipRect();
        if( hovered && occupant ) {
            w::tooltip( occupant->display_name() );
        }
        ImGui::PopID();
        ImGui::Dummy( ImVec2( 0.f, tk.xs * s ) );
    }
    if( w::action_button( _( "Other equipment" ), w::button_kind::secondary, ImVec2( left_width / s, 0.f ),
                          true, nullptr, "cat_other" ) ) {
        select_group( doll_group::other );
    }
    ImGui::EndChild();
    ImGui::SameLine( 0.f, gap );

    // ---- middle column: equipment tree -------------------------------------
    ImGui::BeginChild( "equipment_panel", ImVec2( tree_width, 0.f ), ImGuiChildFlags_None,
                       ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollbar );
    w::section_label( _( "Equipment" ), "tab_equipment" );
    {
        ImGui::BeginChild( "equipment_tree", ImVec2( 0.f, 0.f ), ImGuiChildFlags_None,
                           ImGuiWindowFlags_NoNav );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( tk.sm * s, tk.xs * s + 2.f * s ) );
        // One shared row primitive; sprite/text drawing never steals the drag/drop hit target.
        const auto row = [&]( const std::string & label, const std::string & detail,
                              const char *icon_name, const item * sprite, int depth, bool branch,
                              bool open, bool selected, const tree_node & node ) {
            const int index = static_cast<int>( visible_nodes.size() );
            visible_nodes.push_back( node );
            w::row_state st;
            st.selected = selected;
            st.focused = overview_keyboard_focus && tree_focus == index;
            const std::string id = "n" + std::to_string( index );
            const w::row_result r = w::tree_row( id.c_str(), label, detail, icon_name,
                                                 sprite_painter( sprite ), depth, branch, open, st );
            if( node.slot >= 0 ) {
                accept_equipment_drop( node.slot, node.location );
                if( r.right_clicked ) {
                    const auto occupants = items_on_slot( *you, slots[node.slot] );
                    request_context( node.slot, node.location ? node.location : occupants.empty() ?
                                     item_location::nowhere : occupants.front() );
                }
            } else if( drag_payload ) {
                // Collapsed group headers remain useful drop targets.  Choose
                // a compatible region; explicit rows still provide side/layer intent.
                for( int i = 0; i < static_cast<int>( slots.size() ); ++i ) {
                    if( is_overview_slot( slots[i] ) && group_for( slots[i].region ) == node.group &&
                        slot_matches( *drag_payload, slots[i] ) ) {
                        accept_equipment_drop( i );
                        break;
                    }
                }
            }
            if( r.hovered ) {
                const std::string tip = node.location ? node.location->display_name() : label;
                if( node.location || r.truncated || !detail.empty() ) {
                    w::tooltip( detail.empty() ? tip : tip + "\n" + detail );
                }
            }
            if( st.focused && focus_changed ) {
                ImGui::SetScrollHereY();
            }
            if( r.clicked ) {
                tree_focus = index;
                overview_keyboard_focus = false;
            }
            return r.clicked;
        };
        const auto count_label = []( size_t count ) {
            return count == 0 ? std::string( _( "No items" ) ) :
                   string_format( n_gettext( "%d item", "%d items", count ), count );
        };
        const auto draw_items = [&]( int slot_index, int depth ) {
            const auto items = items_on_slot( *you, slots[slot_index] );
            for( const item_location &loc : items ) {
                std::set<doll_group> covers;
                for( const auto &ref : equipment_layout::coverage_for( mapped, loc ) ) {
                    covers.insert( group_for( ref.id ) );
                }
                std::string linked;
                if( covers.size() > 1 ) {
                    for( doll_group group : covers ) {
                        if( !linked.empty() ) {
                            linked += ", ";
                        }
                        linked += group == doll_group::body ? _( "Torso" ) : group_label( group );
                    }
                }
                if( slots[slot_index].display == equipment_layout::display_layer::uncommon && loc->is_armor() ) {
                    const auto layers = loc->get_layer();
                    if( !layers.empty() ) {
                        linked = layer_name( layers.front() );
                    }
                }
                tree_node node{ group_for( slots[slot_index].region ), slot_index, loc };
                if( row( remove_color_tags( loc->type_name() ), linked, "link", loc.get_item(), depth,
                         false, false, selected_worn == loc, node ) ) {
                    activate_tree_node( node, false );
                }
            }
        };
        if( active_group == doll_group::other || active_group == doll_group::weapons ) {
            if( w::action_button( _( "All equipment" ), w::button_kind::tertiary, ImVec2( 0, 0 ), true,
                                  nullptr, "back" ) ) {
                active_group = doll_group::overview;
            }
        }
        for( const doll_group group : overview_groups ) {
            if( group == doll_group::weapons || ( group == doll_group::other && active_group != group ) ) {
                continue;
            }
            std::set<const item *> occupants;
            for( const auto &region : equipment_layout::region_catalog() ) {
                if( group_for( region.id ) == group ) {
                    for( const auto &loc : equipment_layout::items_visible_on( mapped, region.id ) ) {
                        occupants.insert( loc.get_item() );
                    }
                }
            }
            tree_node group_node{ group, -1, item_location::nowhere };
            if( row( group_label( group ), count_label( occupants.size() ), group_icon( group ), nullptr,
                     0, true, active_group == group, false, group_node ) ) {
                activate_tree_node( group_node, true );
            }
            if( active_group != group ) {
                continue;
            }
            if( scroll_to_active_group ) {
                ImGui::SetScrollHereY( 0.f );
                scroll_to_active_group = false;
            }
            for( int i = 0; i < static_cast<int>( slots.size() ); ++i ) {
                if( !is_overview_slot( slots[i] ) || group_for( slots[i].region ) != group ) {
                    continue;
                }
                const auto items = items_on_slot( *you, slots[i] );
                tree_node node{ group, i, item_location::nowhere };
                if( row( slots[i].label, count_label( items.size() ), region_icon( slots[i].region ),
                         items.empty() ? nullptr : items.front().get_item(),
                         1, true, expanded_slots.count( i ), selected_slot == i, node ) ) {
                    activate_tree_node( node, true );
                }
                if( !expanded_slots.count( i ) ) {
                    continue;
                }
                if( !equipment_layout::info_for( slots[i].region ).skin_middle_outer ) {
                    draw_items( i, 2 );
                    continue;
                }
                for( int j = 0; j < static_cast<int>( slots.size() ); ++j ) {
                    if( slots[j].type != doll_slot::kind::region_layer || slots[j].region != slots[i].region ) {
                        continue;
                    }
                    const auto layer_items = items_on_slot( *you, slots[j] );
                    tree_node layer_node{ group, j, item_location::nowhere };
                    if( row( slots[j].label, count_label( layer_items.size() ), region_icon( slots[j].region ),
                             layer_items.empty() ? nullptr : layer_items.front().get_item(), 2, true,
                             expanded_slots.count( j ), selected_slot == j, layer_node ) ) {
                        activate_tree_node( layer_node, true );
                    }
                    if( expanded_slots.count( j ) ) {
                        draw_items( j, 3 );
                    }
                }
            }
        }
        ImGui::PopStyleVar();
        ImGui::EndChild();
    }
    ImGui::EndChild();
    ImGui::SameLine( 0.f, gap );

    // ---- right column: inventory ---------------------------------------------
    ImGui::BeginChild( "inventory_panel", ImVec2( 0.f, 0.f ), ImGuiChildFlags_None,
                       ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollbar );
    draw_inventory_grid();
    ImGui::EndChild();
    focus_changed = false;

    if( ImGui::IsMouseReleased( ImGuiMouseButton_Right ) ) {
        doll_rmb_down_slot = -1;
    }

    if( doll_ctx_request ) {
        ctx_menu_loc = doll_ctx_loc;
        ctx_menu_from_worn = true;
        ctx_menu_slot = doll_ctx_index;
        ImGui::OpenPopup( "rpg_doll_ctx" );
        doll_ctx_popup_id = ImGui::GetID( "rpg_doll_ctx" );
        rpg_eq_ctx_telem( rmb_telem_line, string_format(
                              "DOLL OpenPopup slot=%d sharedId=0x%08X IsPopupOpen=%d t='%s'",
                              doll_ctx_index,
                              static_cast<unsigned>( doll_ctx_popup_id ),
                              ImGui::IsPopupOpen( "rpg_doll_ctx" ) ? 1 : 0,
                              rpg_eq_short_tname( doll_ctx_loc ) ) );
    }
    // Place at cursor on first frame the popup appears.
    ImGui::SetNextWindowPos( ImGui::GetMousePos(), ImGuiCond_Appearing );
    doll_ctx_popup_id = ImGui::GetID( "rpg_doll_ctx" );
    const bool doll_popup_was_open = ImGui::IsPopupOpen( "rpg_doll_ctx" );
    const bool doll_begin = ImGui::BeginPopup( "rpg_doll_ctx",
                            ImGuiWindowFlags_NoNav );
    if( doll_ctx_request ) {
        rpg_eq_ctx_telem( rmb_telem_line, string_format(
                              "DOLL BeginPopup=%d IsPopupOpen=%d slot=%d t='%s'",
                              doll_begin ? 1 : 0, doll_popup_was_open ? 1 : 0,
                              doll_ctx_index, rpg_eq_short_tname( ctx_menu_loc ) ) );
    }
    if( doll_begin ) {
        // cataimgui::window::draw() may BringWindowToDisplayFront(Equipment)
        // after draw_controls; keep this popup above that panel.
        ImGui::BringWindowToDisplayFront( ImGui::GetCurrentWindow() );
        selected_slot = ctx_menu_slot;
        selected_worn = ctx_menu_loc;
        selected_inv = item_location::nowhere;
        refresh_selection_validity();
        std::string use_method;
        const item_context_menu::action chosen =
            item_context_menu::draw_imgui_menu( *you, selected_worn, /*from_worn=*/true,
                                                &use_method );
        switch( chosen ) {
            case item_context_menu::action::consume:
                selected_inv = selected_worn;
                pending = pending_action::ctx_consume;
                break;
            case item_context_menu::action::use:
                selected_inv = selected_worn;
                pending = pending_action::use_item;
                pending_use_method = std::move( use_method );
                break;
            case item_context_menu::action::read:
                selected_inv = selected_worn;
                pending = pending_action::ctx_read;
                break;
            case item_context_menu::action::takeoff:
                pending = pending_action::takeoff;
                break;
            case item_context_menu::action::unload:
                selected_inv = selected_worn;
                pending = pending_action::ctx_unload;
                break;
            case item_context_menu::action::reload:
                selected_inv = selected_worn;
                pending = pending_action::ctx_reload;
                rpg_eq_ctx_telem( rmb_telem_line, string_format(
                                      "DOLL chose Reload t='%s'",
                                      rpg_eq_short_tname( selected_worn ) ) );
                break;
            case item_context_menu::action::examine:
                pending = pending_action::examine_item;
                break;
            case item_context_menu::action::wear:
            case item_context_menu::action::wield:
            case item_context_menu::action::drop:
            case item_context_menu::action::drop_stack:
            case item_context_menu::action::pickup:
            case item_context_menu::action::always_pickup:
            case item_context_menu::action::never_pickup:
            case item_context_menu::action::more_actions:
                inspector_action = chosen;
                inspector_item = ctx_menu_loc;
                break;
            case item_context_menu::action::none:
                break;
        }
        ImGui::EndPopup();
    }
}


/** Compass label for a 3x3 offset (CDDA: -y = north). Underfoot = "@". */
static const char *nearby_dir_label( int dx, int dy )
{
    static const char *const labels[3][3] = {
        { "NW", "N", "NE" },
        { "W",  "@", "E"  },
        { "SW", "S", "SE" }
    };
    const int ix = dx + 1;
    const int iy = dy + 1;
    if( ix < 0 || ix > 2 || iy < 0 || iy > 2 ) {
        return "?";
    }
    return labels[iy][ix];
}

void rpg_equipment_window::draw_inventory_grid()
{
    namespace w = ui_hybrid_widgets;
    namespace theme = ui_hybrid_chrome::theme;
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    const float s = theme::scale();
    w::section_label( _( "Inventory" ), "tab_inventory" );
    // Header: search, category, nearby storage, view toggle.
    {
        const float btn = tk.button * s;
        const float right_controls = btn * 2.f + ImGui::GetStyle().ItemSpacing.x * 2.f;
        const float combo_w = std::clamp( ImGui::GetContentRegionAvail().x * 0.28f, 110.f * s, 220.f * s );
        ImGui::SetNextItemWidth( std::max( 80.f * s,
                                           ImGui::GetContentRegionAvail().x - combo_w - right_controls - ImGui::GetStyle().ItemSpacing.x ) );
        if( focus_inventory_search ) {
            ImGui::SetKeyboardFocusHere();
            focus_inventory_search = false;
        }
        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( tk.md * s, ( btn - ImGui::GetFontSize() ) * 0.5f ) );
        ImGui::InputTextWithHint( "##inventory_filter", _( "Find carried items…" ),
                                  inventory_filter, sizeof( inventory_filter ) );
        if( w::probe::enabled() ) {
            w::probe::record( "input", "inventory_filter", ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
        }
        ImGui::SameLine();
        const char *categories[] = { _( "All" ), _( "Gear" ), _( "Food / drink" ),
                                     _( "Tools" ), _( "Weapons" )
                                   };
        ImGui::SetNextItemWidth( combo_w );
        ImGui::Combo( "##category", &inventory_category, categories, 5 );
        ImGui::PopStyleVar();
        ImGui::SameLine();
        if( w::icon_button( "##nearby_storage", "nearby", tk.button, false,
                            _( "Nearby storage: store or take items from nearby furniture, buildings, containers and vehicle cargo." ) ) ) {
            open_storage = true;
        }
        ImGui::SameLine();
        if( w::icon_button( "##inventory_view", inventory_list ? "grid" : "list", tk.button, false,
                            inventory_list ? _( "Switch to icon grid" ) : _( "Switch to list view" ) ) ) {
            inventory_list = !inventory_list;
        }
    }
    // Destination hint: where Equip will put the selected inventory item.
    const bool has_target = selected_slot >= 0 && selected_slot < static_cast<int>( slots.size() );
    {
        std::string target_name = _( "Automatic" );
        if( has_target ) {
            const doll_slot &target = slots[selected_slot];
            target_name = target.type == doll_slot::kind::region_layer ?
                          equipment_layout::region_label( target.region ) + " › " + target.label : target.label;
        }
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s", _( "Equip destination:" ) );
        ImGui::SameLine();
        const float avail = ImGui::GetContentRegionAvail().x - 90.f * s;
        const std::string fitted = w::fit_text( target_name, std::max( 40.f * s, avail ) );
        ImGui::TextColored( ui_hybrid_chrome::palette::from_u32( has_target ? tk.accent : tk.text_muted ), "%s",
                            fitted.c_str() );
        if( ImGui::IsItemHovered() && fitted != target_name ) {
            w::tooltip( target_name );
        }
        if( has_target ) {
            ImGui::SameLine();
            if( w::action_button( _( "clear" ), w::button_kind::tertiary, ImVec2( 0.f, 28.f ) ) ) {
                selected_slot = -1;
                selected_worn = item_location::nowhere;
            }
        }
    }
    // NoNav: prevent gamepad L-stick from scrolling the inventory grid while
    // a context popup is open (NavWindow would otherwise be this child).
    w::panel_begin( "inv_grid", ImVec2( 0, 0 ), true, ImGuiWindowFlags_NoNav );

    // Snapshot labels up front so ImGui never sees a temporary .c_str(), and so
    // a later deferred wear/takeoff cannot leave dangling item* mid-draw.
    // Aggregate visually identical stackables with item::display_stacked_with
    // (classic inventory rules): charge-counted objects stay separate cells and
    // already show ×charges via cell_label; non-charge siblings that stacks_with
    // fold into one cell. Wear/Wield/selection use the first location in the group.
    struct grid_cell {
        item_location loc;                 // representative for Wear / Wield / Use
        std::vector<item_location> locs;   // full display stack
        std::string label;
        std::string tip;
        bool usable = false;
    };
    std::vector<grid_cell> grid_items;
    for( item_location &loc : you->all_items_loc() ) {
        if( !loc || !loc.get_item() ) {
            continue;
        }
        // Skip worn clothing / wielded weapon (those live on the doll).
        if( you->is_worn( *loc ) ) {
            continue;
        }
        if( you->is_wielding( *loc ) ) {
            continue;
        }
        // Liquids, gases and anything in a non-container pocket (magazine
        // wells, ammo, mod slots) belong to their container; the container's
        // own cell already names them ("aluminum tank > 200L clean water").
        if( loc.has_parent() ) {
            const item_pocket *pocket = loc.parent_pocket();
            // Frozen liquids (ice in a canteen, water in a no-spoil pocket that never
            // warmed up) are solid-phase but still belong to their container.
            if( loc->made_of( phase_id::LIQUID ) || loc->made_of( phase_id::GAS ) ||
                loc->is_frozen_liquid() ||
                ( pocket != nullptr && !pocket->is_type( pocket_type::CONTAINER ) ) ) {
                continue;
            }
        }
        if( inventory_filter[0] != '\0' &&
            !lcmatch( remove_color_tags( loc->display_name() ), inventory_filter ) ) {
            continue;
        }
        if( ( inventory_category == 1 && !loc->is_armor() && !loc->is_gun() && !loc->is_melee() ) ||
            ( inventory_category == 2 && !loc->is_comestible() ) ||
            ( inventory_category == 3 && !loc->is_tool() ) ||
            ( inventory_category == 4 && !loc->is_gun() && !loc->is_melee() ) ) {
            continue;
        }

        bool folded = false;
        for( grid_cell &existing : grid_items ) {
            // display_stacked_with excludes count_by_charges and requires stacks_with
            // (same type, rot/dirty, contents, mods, etc.) — do not merge unlike items.
            if( loc->display_stacked_with( *existing.loc ) ) {
                existing.locs.push_back( loc );
                folded = true;
                break;
            }
        }
        if( folded ) {
            continue;
        }

        grid_cell cell;
        cell.loc = loc;
        cell.locs.push_back( loc );
        grid_items.push_back( std::move( cell ) );
    }

    // Dense pack of square-ish cells (~48–64px). Default CDDA tileset ITEM
    // sprites via draw-list AddImage (Button stays last item); Hybrid bezel stays.
    // Equipped gear on doll/slots only — no duplicate equipped-item list.
    // Soft-fork: denser inventory cells (Hybrid charcoal/amber grid).
    const float avail = ImGui::GetContentRegionAvail().x;
    // Cells follow the UI scale so the grid stays legible at 4K.
    const float grid_s = ui_hybrid_chrome::theme::scale();
    const float min_cell = 56.f * grid_s;
    const float max_cell = 72.f * grid_s;
    const float cell_gap = 6.f * grid_s;
    int columns = std::max( 1, static_cast<int>( ( avail + cell_gap ) /
                            ( min_cell + cell_gap ) ) );
    columns = std::min( columns, 16 );
    // N cells share (N-1) gaps; keep cell_w fixed so every row aligns to the same columns.
    float cell_w = ( avail - cell_gap * static_cast<float>( columns - 1 ) ) /
                   static_cast<float>( columns );
    cell_w = std::clamp( cell_w, min_cell, max_cell );
    // If clamp hit max_cell, recompute how many fixed-size cells actually fit.
    if( cell_w >= max_cell - 0.01f ) {
        columns = std::max( 1, static_cast<int>( ( avail + cell_gap ) /
                            ( max_cell + cell_gap ) ) );
        columns = std::min( columns, 16 );
        cell_w = max_cell;
    }
    if( inventory_list ) {
        columns = 1;
        cell_w = avail;
    }
    const float cell_h = inventory_list ? 36.f * ui_hybrid_chrome::theme::scale() : cell_w;
    const int label_chars = std::max( 4,
                                      static_cast<int>( ( cell_w - 6.f ) /
                                              std::max( 1.f, ImGui::CalcTextSize( "W" ).x ) ) );

    for( grid_cell &cell : grid_items ) {
        const int stack_n = static_cast<int>( cell.locs.size() );
        cell.label = cell_label( *cell.loc, stack_n, label_chars );
        cell.tip = cell.loc->display_name( static_cast<unsigned int>( std::max( 1, stack_n ) ) );
        cell.usable = item_looks_usable( *cell.loc );
        if( stack_n > 1 ) {
            cell.tip = string_format( _( "%s\nStack of %d — actions use one item." ),
                                      cell.tip, stack_n );
        }
        if( cell.usable ) {
            cell.tip += _( "\nClick again to Use. Right-click for menu." );
        } else {
            cell.tip += _( "\nClick again to Wear/Wield. Right-click for menu." );
        }
    }

    if( grid_items.empty() ) {
        ImGui::Dummy( ImVec2( 0.f, 8.f * ui_hybrid_chrome::theme::scale() ) );
        ui_hybrid_widgets::empty_state( _( "No items match this view." ),
                                        inventory_filter[0] != '\0' ? _( "Clear the search to see everything you carry." ) : std::string() );
    }

    // Match SameLine spacing to the gap baked into column math so rows stay
    // rectangular (every cell lands in a fixed column under the one above).
    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( cell_gap, cell_gap ) );
    int col = 0;
    bool inv_ctx_request = false;
    int inv_ctx_index = -1;
    item_location inv_ctx_loc;
    for( int i = 0; i < static_cast<int>( grid_items.size() ); i++ ) {
        grid_cell &cell = grid_items[i];
        ImGui::PushID( 1000 + i );

        const bool is_sel = selected_inv && std::any_of( cell.locs.begin(), cell.locs.end(),
        [&]( const item_location & l ) {
            return selected_inv == l;
        } );
        int grid_cols = 0;
        if( inventory_list ) {
            // Row look is drawn by the shared row background; the button stays
            // transparent so it remains the drag source / hit target.
            ImGui::PushStyleColor( ImGuiCol_Button, 0 );
            ImGui::PushStyleColor( ImGuiCol_ButtonHovered, 0 );
            ImGui::PushStyleColor( ImGuiCol_ButtonActive, 0 );
            grid_cols = 3;
        } else {
            grid_cols = ui_hybrid_chrome::push_grid_button( is_sel );
        }
        ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, is_sel && !inventory_list ? 1.f : 0.f );

        // Invisible label (PushID scopes uniqueness); sprite / glyph drawn as overlay.
        if( ImGui::Button( "##inv_cell", ImVec2( cell_w, cell_h ) ) ) {
            if( is_sel ) {
                // Second click: Use when applicable, otherwise equip onto selected doll slot
                if( cell.usable ) {
                    pending = pending_action::use_item;
                    pending_use_method.clear();
                } else {
                    pending = selected_slot >= 0 ? pending_action::drag_equip : pending_action::equip;
                }
            } else {
                // Operate on the first / representative item of the display stack.
                selected_inv = cell.loc;
            }
        }
        // Plain hover: do NOT use AllowWhenBlockedByPopup — while the RMB
        // menu is open that flag keeps cells under the popup "hovered", which
        // spawns tooltips that fight the menu and can scroll/jitter the grid.
        const bool hovered = ImGui::IsItemHovered();
        const bool rmb_down = ImGui::IsMouseClicked( ImGuiMouseButton_Right );
        const bool rmb_up = ImGui::IsMouseReleased( ImGuiMouseButton_Right );
        const bool want_cap = ImGui::GetIO().WantCaptureMouse;
        const ImGuiID cell_item_id = ImGui::GetItemID();
        // Per-cell hashed id (old OpenPopupOnItemClick path) — log to compare
        // against the shared popup id used after PopID.
        const ImGuiID per_cell_popup_id = ImGui::GetID( "rpg_inv_ctx" );

        if( hovered && rmb_down ) {
            inv_rmb_down_cell = i;
        }
        if( hovered && ( rmb_down || rmb_up ) ) {
            rpg_eq_ctx_telem( rmb_telem_line, string_format(
                                  "INV %s cell=%d down=%d id=0x%08X popupId=0x%08X wantCap=%d t='%s'",
                                  rmb_down ? "RMB_DOWN" : "RMB_UP",
                                  i, inv_rmb_down_cell,
                                  static_cast<unsigned>( cell_item_id ),
                                  static_cast<unsigned>( per_cell_popup_id ),
                                  want_cap ? 1 : 0,
                                  rpg_eq_short_tname( cell.loc ) ) );
        }

        // Drag source → doll slots (needs Button as LastItem; LMB only).
        bool dds_active = false;
        if( ImGui::BeginDragDropSource( ImGuiDragDropFlags_SourceAllowNullID ) ) {
            dds_active = true;
            drag_payload = cell.loc;
            int token = i;
            ImGui::SetDragDropPayload( "RPG_EQ_ITEM", &token, sizeof( token ) );
            ImGui::TextUnformatted( cell.label.c_str() );
            ImGui::EndDragDropSource();
        }
        if( hovered && rmb_up && dds_active ) {
            rpg_eq_ctx_telem( rmb_telem_line, string_format(
                                  "INV RMB_UP+DDS CONFLICT cell=%d t='%s'",
                                  i, rpg_eq_short_tname( cell.loc ) ) );
        }

        // Defer shared popup open until after PopID + EndChild so OpenPopup/
        // BeginPopup share one stable id at the parent Equipment window.
        // Same-cell gate: RMB_DOWN and RMB_UP must match (neighbor steal fix).
        if( hovered && rmb_up ) {
            if( inv_rmb_down_cell == i ) {
                inv_ctx_request = true;
                inv_ctx_index = i;
                inv_ctx_loc = cell.loc;
            } else if( inv_rmb_down_cell >= 0 ) {
                rpg_eq_ctx_telem( rmb_telem_line, string_format(
                                      "INV RMB_UP MISMATCH down=%d up=%d t='%s'",
                                      inv_rmb_down_cell, i,
                                      rpg_eq_short_tname( cell.loc ) ) );
            }
        }

        if( !inventory_list ) {
            ui_hybrid_chrome::draw_item_bezel( is_sel, hovered, false );
        }
        // Default tileset ITEM sprite (looks_like / variants via get_texture_draw_data).
        // Fallback: truncated label / first glyph. Stack ×N badge when stacked.
        {
            // Name without ×N — badge is drawn by the overlay helper.
            std::string fb = remove_color_tags(
                                 cell.loc->type_name( 1, /*use_variant=*/true ) );
            if( fb.empty() ) {
                fb = cell_fallback_glyph( *cell.loc );
            } else {
                fb = ellipsize_label( fb, label_chars );
            }
            if( ui_hybrid_widgets::probe::enabled() ) {
                // Grid cells and list rows are the same inventory entries to the harness.
                ui_hybrid_widgets::probe::record( is_sel ? "inv_row_selected" : "inv_row",
                                                  remove_color_tags( cell.loc->type_name() ),
                                                  ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
            }
            if( inventory_list ) {
                const ImVec2 min = ImGui::GetItemRectMin();
                const ImVec2 max = ImGui::GetItemRectMax();
                ImDrawList *draw = ImGui::GetWindowDrawList();
                ui_hybrid_widgets::row_state rs;
                rs.selected = is_sel;
                ui_hybrid_widgets::draw_row_background( draw, min, max, rs, hovered );
                const float isz = ui_hybrid_chrome::theme::get().icon * ui_hybrid_chrome::theme::scale();
                const float pad = ui_hybrid_chrome::theme::get().md * ui_hybrid_chrome::theme::scale();
                draw_equipment_icon( *cell.loc, ImVec2( min.x + pad, min.y + ( cell_h - isz ) * 0.5f ),
                                     ImVec2( min.x + pad + isz, min.y + ( cell_h + isz ) * 0.5f ) );
                const int amount = cell.loc->count_by_charges() ? cell.loc->charges :
                                   static_cast<int>( cell.locs.size() );
                float right = max.x - pad;
                if( amount > 1 ) {
                    const std::string qty = format_stack_badge( amount );
                    const float qw = ImGui::CalcTextSize( qty.c_str() ).x;
                    draw->AddText( ImVec2( right - qw, min.y + ( cell_h - ImGui::GetFontSize() ) * 0.5f ),
                                   ImGui::GetColorU32( ImGuiCol_TextDisabled ), qty.c_str() );
                    right -= qw + pad;
                }
                const float text_x = min.x + pad + isz + pad;
                const std::string name = ui_hybrid_widgets::fit_text( remove_color_tags( cell.loc->type_name() ),
                                         std::max( 20.f, right - text_x ) );
                draw->AddText( ImVec2( text_x, min.y + ( cell_h - ImGui::GetFontSize() ) * 0.5f ),
                               ImGui::GetColorU32( ImGuiCol_Text ), name.c_str() );
            } else {
                overlay_item_sprite_on_last_item( *cell.loc,
                                                  static_cast<int>( cell.locs.size() ), fb );
            }
        }
        if( hovered ) {
            imgui_cdda_tooltip( cell.tip );
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor( grid_cols );
        ImGui::PopID();

        col++;
        if( col < columns ) {
            ImGui::SameLine( 0.f, cell_gap );
        } else {
            col = 0;
        }
    }
    ImGui::PopStyleVar(); // ItemSpacing

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Checkbox( _( "Show nearby ground items" ), &show_nearby );

    struct nearby_cell {
        item_location loc;
        std::vector<item_location> locs;
        std::string label;
        std::string tip;
        std::string dir;
        int dx = 0;
        int dy = 0;
    };
    std::vector<nearby_cell> nearby_items;
    if( show_nearby ) {
        map &here = get_map();
        const tripoint_bub_ms origin = you->pos_bub();
        for( int dy = -1; dy <= 1; dy++ ) {
            for( int dx = -1; dx <= 1; dx++ ) {
                const tripoint_bub_ms tp = origin + tripoint_rel_ms( dx, dy, 0 );
                if( !here.inbounds( tp ) ) {
                    continue;
                }
                if( here.has_flag( ter_furn_flag::TFLAG_SEALED, tp ) ) {
                    continue;
                }
                if( !here.has_items( tp ) ) {
                    continue;
                }
                map_cursor mc( tp );
                const char *dir = nearby_dir_label( dx, dy );
                for( item &it : here.i_at( tp ) ) {
                    if( it.has_flag( json_flag_HIDDEN_ITEM ) ) {
                        continue;
                    }
                    item_location loc( mc, &it );
                    if( !loc || !loc.get_item() ) {
                        continue;
                    }
                    bool folded = false;
                    for( nearby_cell &existing : nearby_items ) {
                        // Only fold stacks that share the same tile.
                        if( existing.dx != dx || existing.dy != dy ) {
                            continue;
                        }
                        if( existing.loc && existing.loc.get_item() &&
                            existing.loc->display_stacked_with( it ) ) {
                            existing.locs.push_back( loc );
                            folded = true;
                            break;
                        }
                    }
                    if( !folded ) {
                        nearby_cell cell;
                        cell.loc = loc;
                        cell.locs.push_back( loc );
                        cell.dir = dir;
                        cell.dx = dx;
                        cell.dy = dy;
                        nearby_items.push_back( std::move( cell ) );
                    }
                }
            }
        }
    }

    if( show_nearby && nearby_items.empty() ) {
        ImGui::TextDisabled( "%s", _( "Nothing nearby on the ground." ) );
    } else if( show_nearby ) {
        // Slightly denser than inventory for the ground strip.
        const float navail = ImGui::GetContentRegionAvail().x;
        const float nmin = 36.f;
        const float nmax = 48.f;
        const float ngap = 2.f;
        int ncols = std::max( 1, static_cast<int>( ( navail + ngap ) / ( nmin + ngap ) ) );
        ncols = std::min( ncols, 16 );
        float ncell_w = ( navail - ngap * static_cast<float>( ncols - 1 ) ) /
                        static_cast<float>( ncols );
        ncell_w = std::clamp( ncell_w, nmin, nmax );
        if( ncell_w >= nmax - 0.01f ) {
            ncols = std::max( 1, static_cast<int>( ( navail + ngap ) / ( nmax + ngap ) ) );
            ncols = std::min( ncols, 16 );
            ncell_w = nmax;
        }
        const float ncell_h = ncell_w;
        const int nlabel_chars = std::max( 3,
                                           static_cast<int>( ( ncell_w - 6.f ) /
                                                   std::max( 1.f, ImGui::CalcTextSize( "W" ).x ) ) );

        for( nearby_cell &cell : nearby_items ) {
            const int stack_n = static_cast<int>( cell.locs.size() );
            cell.label = cell_label( *cell.loc, stack_n, nlabel_chars );
            cell.tip = string_format( _( "[%s] %s" ), cell.dir,
                                      cell.loc->display_name(
                                          static_cast<unsigned int>( std::max( 1, stack_n ) ) ) );
            if( stack_n > 1 ) {
                cell.tip = string_format( _( "%s\nStack of %d — actions use one item." ),
                                          cell.tip, stack_n );
            }
            cell.tip += _( "\nClick again to Pick up. Right-click for menu." );
        }

        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( ngap, ngap ) );
        int ncol = 0;
        for( int i = 0; i < static_cast<int>( nearby_items.size() ); i++ ) {
            nearby_cell &cell = nearby_items[i];
            ImGui::PushID( 5000 + i );

            const bool is_sel = selected_inv && std::any_of( cell.locs.begin(), cell.locs.end(),
            [&]( const item_location & l ) {
                return selected_inv == l;
            } );
            const int grid_cols = ui_hybrid_chrome::push_grid_button( is_sel );
            ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, is_sel ? 1.f : 0.f );

            if( ImGui::Button( "##nearby_cell", ImVec2( ncell_w, ncell_h ) ) ) {
                if( is_sel ) {
                    pending = pending_action::ctx_pickup;
                } else {
                    selected_inv = cell.loc;
                    selected_worn = item_location::nowhere;
                }
            }
            const bool hovered = ImGui::IsItemHovered();
            const bool rmb_down = ImGui::IsMouseClicked( ImGuiMouseButton_Right );
            const bool rmb_up = ImGui::IsMouseReleased( ImGuiMouseButton_Right );

            if( hovered && rmb_down ) {
                inv_rmb_down_cell = 5000 + i;
            }
            if( hovered && rmb_up ) {
                if( inv_rmb_down_cell == 5000 + i ) {
                    inv_ctx_request = true;
                    inv_ctx_index = 5000 + i;
                    inv_ctx_loc = cell.loc;
                }
            }

            if( !inventory_list ) {
                ui_hybrid_chrome::draw_item_bezel( is_sel, hovered, false );
            }
            {
                std::string fb = remove_color_tags(
                                     cell.loc->type_name( 1, /*use_variant=*/true ) );
                if( fb.empty() ) {
                    fb = cell_fallback_glyph( *cell.loc );
                } else {
                    fb = ellipsize_label( fb, nlabel_chars );
                }
                overlay_item_sprite_on_last_item( *cell.loc,
                                                  static_cast<int>( cell.locs.size() ), fb );
                // Subtle dir badge (top-left) — underfoot "@" / compass letter.
                {
                    ImDrawList *dl = ImGui::GetWindowDrawList();
                    const ImVec2 rmin = ImGui::GetItemRectMin();
                    const ImU32 col = ImGui::GetColorU32( ui_hybrid_chrome::palette::accent() );
                    dl->AddText( ImVec2( rmin.x + 2.f, rmin.y + 1.f ), col, cell.dir.c_str() );
                }
            }
            if( hovered ) {
                imgui_cdda_tooltip( cell.tip );
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleColor( grid_cols );
            ImGui::PopID();

            ncol++;
            if( ncol < ncols ) {
                ImGui::SameLine( 0.f, ngap );
            } else {
                ncol = 0;
            }
        }
        ImGui::PopStyleVar();
    }

    // OpenPopup/BeginPopup OUTSIDE inv_grid child — parent Equipment id stack
    // so the menu draws above the panel instead of under/inside the child.
    ui_hybrid_widgets::panel_end();

    if( ImGui::IsMouseReleased( ImGuiMouseButton_Right ) ) {
        inv_rmb_down_cell = -1;
    }

    if( inv_ctx_request ) {
        ctx_menu_loc = inv_ctx_loc;
        ctx_menu_from_worn = false;
        ctx_menu_slot = -1;
        ImGui::OpenPopup( "rpg_inv_ctx" );
        inv_ctx_popup_id = ImGui::GetID( "rpg_inv_ctx" );
        rpg_eq_ctx_telem( rmb_telem_line, string_format(
                              "INV OpenPopup cell=%d sharedId=0x%08X IsPopupOpen=%d t='%s'",
                              inv_ctx_index,
                              static_cast<unsigned>( inv_ctx_popup_id ),
                              ImGui::IsPopupOpen( "rpg_inv_ctx" ) ? 1 : 0,
                              rpg_eq_short_tname( inv_ctx_loc ) ) );
    }
    ImGui::SetNextWindowPos( ImGui::GetMousePos(), ImGuiCond_Appearing );
    inv_ctx_popup_id = ImGui::GetID( "rpg_inv_ctx" );
    const bool inv_popup_was_open = ImGui::IsPopupOpen( "rpg_inv_ctx" );
    // NoNav: RMB menus are mouse-only. With NavEnableGamepad, Deck stick
    // drift races MenuItem highlight / scrolls the popup without input.
    const bool inv_begin = ImGui::BeginPopup( "rpg_inv_ctx",
                           ImGuiWindowFlags_NoNav );
    // Log BeginPopup only on the RMB open attempt (not every frame while open).
    if( inv_ctx_request ) {
        rpg_eq_ctx_telem( rmb_telem_line, string_format(
                              "INV BeginPopup=%d IsPopupOpen=%d cell=%d t='%s'",
                              inv_begin ? 1 : 0, inv_popup_was_open ? 1 : 0,
                              inv_ctx_index, rpg_eq_short_tname( ctx_menu_loc ) ) );
    }
    if( inv_begin ) {
        ImGui::BringWindowToDisplayFront( ImGui::GetCurrentWindow() );
        selected_inv = ctx_menu_loc;
        selected_worn = item_location::nowhere;
        refresh_selection_validity();
        std::string use_method;
        const item_context_menu::action chosen =
            item_context_menu::draw_imgui_menu( *you, selected_inv, /*from_worn=*/false,
                                                &use_method );
        switch( chosen ) {
            case item_context_menu::action::consume:
                pending = pending_action::ctx_consume;
                break;
            case item_context_menu::action::use:
                pending = pending_action::use_item;
                pending_use_method = std::move( use_method );
                break;
            case item_context_menu::action::read:
                pending = pending_action::ctx_read;
                break;
            case item_context_menu::action::wear:
                pending = pending_action::ctx_wear;
                break;
            case item_context_menu::action::wield:
                pending = pending_action::wield;
                break;
            case item_context_menu::action::takeoff:
                pending = pending_action::takeoff;
                break;
            case item_context_menu::action::drop:
                pending = pending_action::drop_item;
                break;
            case item_context_menu::action::pickup:
                pending = pending_action::ctx_pickup;
                break;
            case item_context_menu::action::unload:
                pending = pending_action::ctx_unload;
                break;
            case item_context_menu::action::reload:
                pending = pending_action::ctx_reload;
                rpg_eq_ctx_telem( rmb_telem_line, string_format(
                                      "INV chose Reload t='%s'",
                                      rpg_eq_short_tname( selected_inv ) ) );
                break;
            case item_context_menu::action::examine:
                pending = pending_action::examine_item;
                break;
            case item_context_menu::action::drop_stack:
            case item_context_menu::action::always_pickup:
            case item_context_menu::action::never_pickup:
            case item_context_menu::action::more_actions:
                inspector_action = chosen;
                inspector_item = ctx_menu_loc;
                break;
            case item_context_menu::action::none:
                break;
        }
        ImGui::EndPopup();
    }
}

void rpg_equipment_window::draw_action_bar()
{
    namespace w = ui_hybrid_widgets;
    namespace theme = ui_hybrid_chrome::theme;
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    const float s = theme::scale();
    refresh_selection_validity();
    const item_location inspected = selected_inv ? selected_inv : selected_worn;
    const bool can_takeoff = selected_worn && you->is_worn( *selected_worn );

    // Right-aligned action group. Widths are fixed so the layout is stable.
    const float primary_w = 150.f;
    const float secondary_w = 130.f;
    const float tertiary_w = 96.f;
    float group_w = ( primary_w + secondary_w + tertiary_w * 2.f ) * s + ImGui::GetStyle().ItemSpacing.x * 3.f;
    if( equip_preview ) {
        group_w += ( secondary_w + 20.f ) * s + ImGui::GetStyle().ItemSpacing.x;
    }
    // Left: binding hints and the current status / selected target. On narrow footers
    // (small window at a large font) the hints give way to the status text.
    const bool show_hints = ImGui::GetContentRegionAvail().x > group_w + 460.f * s;
    if( show_hints ) {
        w::hint( ctxt, "CONFIRM", _( "select" ) );
        ImGui::SameLine();
        w::hint( ctxt, "RIGHT", _( "expand" ) );
        ImGui::SameLine();
        w::hint( ctxt, "QUIT", _( "back" ) );
        ImGui::SameLine( 0.f, tk.lg * s );
    }
    const float status_w = ImGui::GetContentRegionAvail().x - group_w - tk.md * s;
    {
        std::string status = status_line;
        if( status.empty() && inspected ) {
            status = remove_color_tags( inspected->display_name() );
        } else if( status.empty() ) {
            status = _( "Select an item to inspect it." );
        }
        const std::string fitted = w::fit_text( status, std::max( 40.f * s, status_w ) );
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s", fitted.c_str() );
        if( w::probe::enabled() ) {
            w::probe::record( "status", status, ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
        }
        if( ImGui::IsItemHovered() && fitted != status ) {
            w::tooltip( status );
        }
        ImGui::SameLine();
    }
    w::footer_align_right( group_w );
    if( w::action_button( _( "Details" ), w::button_kind::tertiary, ImVec2( tertiary_w, 0.f ),
                          static_cast<bool>( inspected ), _( "Select an item first." ) ) ) {
        ImGui::OpenPopup( "equipment_details" );
    }
    ImGui::SameLine();
    if( w::action_button( _( "More" ), w::button_kind::tertiary, ImVec2( tertiary_w, 0.f ), true, nullptr,
                          "chevron_down" ) ) {
        ImGui::OpenPopup( "equipment_more" );
    }
    if( ImGui::BeginPopup( "equipment_more" ) ) {
        if( ImGui::MenuItem( _( "Wield" ), nullptr, false, static_cast<bool>( inspected ) ) ) {
            pending = pending_action::wield;
        }
        if( ImGui::MenuItem( _( "Use" ), nullptr, false, static_cast<bool>( inspected ) ) ) {
            pending = pending_action::use_item;
        }
        ImGui::Separator();
        if( ImGui::MenuItem( _( "Clothing layers…" ) ) ) {
            open_layers = true;
        }
        if( ImGui::MenuItem( _( "Classic inventory…" ) ) ) {
            open_classic = true;
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if( equip_preview ) {
        if( w::action_button( _( "Cancel change" ), w::button_kind::secondary, ImVec2( secondary_w + 20.f, 0.f ) ) ) {
            equip_preview = false;
            status_line = _( "Equipment change cancelled." );
        }
        ImGui::SameLine();
    }
    if( w::action_button( _( "Take off" ), w::button_kind::secondary, ImVec2( secondary_w, 0.f ), can_takeoff,
                          _( "Select a worn item to take it off." ) ) ) {
        pending = pending_action::takeoff;
    }
    ImGui::SameLine();
    if( equip_preview ) {
        if( w::action_button( _( "Apply change" ), w::button_kind::primary, ImVec2( primary_w, 0.f ) ) ) {
            selected_inv = preview_item;
            selected_slot = preview_slot;
            pending = pending_action::drag_equip;
            equip_preview = false;
        }
    } else if( w::action_button( _( "Equip" ), w::button_kind::primary, ImVec2( primary_w, 0.f ) ) ) {
        if( selected_inv ) {
            pending = selected_slot >= 0 ? pending_action::drag_equip : pending_action::equip;
        } else {
            focus_inventory_search = true;
            status_line = _( "Choose an inventory item, then Equip." );
        }
    }
    if( RPG_EQ_CTX_TELEM && !rmb_telem_line.empty() ) {
        ImGui::TextColored( ImVec4( 0.95f, 0.75f, 0.25f, 1.f ),
                            "RMB dbg: %s", rmb_telem_line.c_str() );
    }
}

void rpg_equipment_window::draw_controls()
{
    namespace w = ui_hybrid_widgets;
    if( hide_ui ) {
        hide_if_hidden();
        return;
    }
    if( equip_preview && ( !preview_item || selected_inv != preview_item ||
                           selected_slot != preview_slot ) ) {
        equip_preview = false;
    }
    // Content scrolls per column; the footer with the actions stays fixed.
    if( w::body_begin( "equipment_body", ui_hybrid_chrome::theme::get().footer, ImGuiWindowFlags_NoScrollbar ) ) {
        draw_paper_doll();
    }
    w::body_end();
    if( w::footer_begin( "equipment_footer" ) ) {
        draw_action_bar();
    }
    w::footer_end();

    ImGui::SetNextWindowSize( ImVec2( std::min( 650.f, ImGui::GetMainViewport()->Size.x * 0.85f ),
                                      std::min( 400.f, ImGui::GetMainViewport()->Size.y * 0.6f ) ), ImGuiCond_Appearing );
    if( ImGui::BeginPopup( "equipment_details", ImGuiWindowFlags_NoNav ) ) {
        draw_equipment_inspection();
        if( inspector_action != item_context_menu::action::none ) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Parent cataimgui::window::draw() BringWindowToDisplayFront(Equipment)
    // after this returns — that would bury our context popups under the panel.
    // Skip that front-bring while a ctx popup is open (IDs hashed at parent).
    force_to_back = ImGui::IsPopupOpen( "rpg_inv_ctx" ) ||
                    ImGui::IsPopupOpen( "rpg_doll_ctx" ) || ImGui::IsPopupOpen( "equipment_more" ) ||
                    ImGui::IsPopupOpen( "equipment_details" );

    if( !get_is_open() ) {
        want_close = true;
    }
}

} // namespace

namespace rpg_equipment_ui
{

void open()
{
    const bool opt = get_option<bool>( "RPG_EQUIPMENT_UI" );
    DebugLog( D_INFO, D_MAIN ) << "rpg_eq_ctx: equipment UI open; RPG_EQUIPMENT_UI="
                               << ( opt ? "true" : "false" );
    Character &you = get_player_character();
    item_location reload;
    bool classic = false;
    bool storage = false;
    bool layers = false;
    {
        rpg_equipment_window win( &you );
        win.execute();
        reload = win.reload_after_close();
        classic = win.classic_after_close();
        storage = win.storage_after_close();
        layers = win.layers_after_close();
    }
    if( layers ) {
        you.worn.sort_armor( you );
    }
    if( storage ) {
        create_nearby_storage();
    }
    if( classic ) {
        game_menus::inv::common();
    }
    if( reload ) {
        item_context_menu::perform( you, reload, item_context_menu::action::reload );
    }
}

} // namespace rpg_equipment_ui
