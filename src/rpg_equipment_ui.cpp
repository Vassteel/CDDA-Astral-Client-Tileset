#include "ui_telemetry.h"
#include "rpg_equipment_ui.h"

#include <algorithm>
#include <optional>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "avatar_action.h"
#include "activity_actor_definitions.h"
#include "bodypart.h"
#include "cata_imgui.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "ui_hybrid_chrome.h"
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

namespace
{

static const sub_bodypart_str_id sub_body_part_torso_hanging_back( "torso_hanging_back" );

struct doll_slot {
    enum class kind { body, body_outer, back, weapon, offhand } type;
    bodypart_id bp;
    std::string label;
};

/** Prefer word-boundary cut, then utf8-safe ellipsis. */
static std::string ellipsize_label( const std::string &raw, int max_cells )
{
    if( max_cells <= 1 ) {
        return "…";
    }
    if( utf8_width( raw ) <= max_cells ) {
        return raw;
    }
    // Prefer breaking on space / hyphen / slash near the end.
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
    // Trim trailing whitespace from the break
    while( !out.empty() && out.back() == ' ' ) {
        out.pop_back();
    }
    if( out.empty() ) {
        out = utf8_truncate( raw, static_cast<size_t>( max_cells - 1 ) );
    }
    return out + "…";
}

static std::vector<doll_slot> make_doll_slots( Character &you )
{
    std::vector<doll_slot> slots;
    const auto add_bp = [&]( const bodypart_str_id & id, const char *fallback_label ) {
        if( !you.has_part( id.id() ) ) {
            return;
        }
        doll_slot s;
        s.type = doll_slot::kind::body;
        s.bp = id.id();
        s.label = body_part_name_as_heading( s.bp, 1 );
        if( s.label.empty() ) {
            s.label = fallback_label;
        }
        slots.push_back( s );
    };

    add_bp( body_part_eyes, "Eyes" );
    add_bp( body_part_head, "Head" );
    add_bp( body_part_mouth, "Mouth" );
    add_bp( body_part_torso, "Torso" );

    // Dedicated outer-layer torso slot (coats / outer armor).
    {
        doll_slot outer;
        outer.type = doll_slot::kind::body_outer;
        outer.bp = body_part_torso.id();
        outer.label = _( "Outer" );
        slots.push_back( outer );
    }
    // Dedicated hanging-back / backpack slot (BELTED back storage).
    {
        doll_slot back;
        back.type = doll_slot::kind::back;
        back.bp = body_part_torso.id();
        back.label = _( "Back" );
        slots.push_back( back );
    }

    add_bp( body_part_arm_l, "Arm L" );
    add_bp( body_part_arm_r, "Arm R" );
    add_bp( body_part_hand_l, "Hand L" );
    add_bp( body_part_hand_r, "Hand R" );
    add_bp( body_part_leg_l, "Leg L" );
    add_bp( body_part_leg_r, "Leg R" );
    add_bp( body_part_foot_l, "Foot L" );
    add_bp( body_part_foot_r, "Foot R" );

    doll_slot weapon;
    weapon.type = doll_slot::kind::weapon;
    weapon.bp = bodypart_str_id::NULL_ID().id();
    weapon.label = _( "Weapon" );
    slots.push_back( weapon );

    doll_slot offhand;
    offhand.type = doll_slot::kind::offhand;
    offhand.bp = bodypart_str_id::NULL_ID().id();
    offhand.label = _( "Offhand" );
    slots.push_back( offhand );

    return slots;
}

static bool covers_hanging_back( const item &it )
{
    for( const sub_bodypart_id &sbp : it.get_covered_sub_body_parts() ) {
        if( sbp.id() == sub_body_part_torso_hanging_back ) {
            return true;
        }
    }
    // Fallback: belted torso gear without detailed sub-coverage (backpacks).
    return it.has_flag( flag_BELTED ) && it.covers( body_part_torso ) &&
           it.has_layer( { layer_level::BELTED } );
}

static bool is_outer_on_torso( const item &it )
{
    return it.covers( body_part_torso ) && it.has_layer( { layer_level::OUTER }, body_part_torso );
}

/** Outermost worn item covering bp that belongs on a plain body slot. */
static item_location worn_on_body_slot( Character &you, const bodypart_id &bp )
{
    const std::vector<item_location> worn = you.top_items_loc();
    for( auto it = worn.rbegin(); it != worn.rend(); ++it ) {
        if( !*it || !you.is_worn( **it ) || !( *it )->covers( bp ) ) {
            continue;
        }
        // Keep torso under-layer free of outer coats / backpacks (those have dedicated slots).
        if( bp == body_part_torso ) {
            if( covers_hanging_back( **it ) || is_outer_on_torso( **it ) ) {
                continue;
            }
        }
        return *it;
    }
    return item_location::nowhere;
}

static item_location worn_outer_torso( Character &you )
{
    const std::vector<item_location> worn = you.top_items_loc();
    for( auto it = worn.rbegin(); it != worn.rend(); ++it ) {
        if( *it && you.is_worn( **it ) && is_outer_on_torso( **it ) && !covers_hanging_back( **it ) ) {
            return *it;
        }
    }
    return item_location::nowhere;
}

static item_location worn_on_back( Character &you )
{
    const std::vector<item_location> worn = you.top_items_loc();
    for( auto it = worn.rbegin(); it != worn.rend(); ++it ) {
        if( *it && you.is_worn( **it ) && covers_hanging_back( **it ) ) {
            return *it;
        }
    }
    return item_location::nowhere;
}

static item_location worn_offhand_shield( Character &you )
{
    // Prefer CDDA's worn BLOCK_WHILE_WORN shield; never show the wielded weapon here.
    item *shield = you.worn.best_shield();
    if( shield == nullptr ) {
        return item_location::nowhere;
    }
    if( you.is_wielding( *shield ) ) {
        return item_location::nowhere;
    }
    return item_location( you, shield );
}

static item_location item_on_slot( Character &you, const doll_slot &slot )
{
    switch( slot.type ) {
        case doll_slot::kind::weapon:
            return you.get_wielded_item();
        case doll_slot::kind::offhand:
            return worn_offhand_shield( you );
        case doll_slot::kind::back:
            return worn_on_back( you );
        case doll_slot::kind::body_outer:
            return worn_outer_torso( you );
        case doll_slot::kind::body:
        default:
            return worn_on_body_slot( you, slot.bp );
    }
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
        }

        bool execute();
        item_location reload_after_close() const { return deferred_reload_loc; }
        bool classic_after_close() const { return open_classic; }

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
            return { -1.f, -1.f, std::min( vp.x * 0.94f, 1200.f * scale ),
                     std::min( vp.y * 0.92f, 800.f * scale ) };
        }

    private:
        Character *you = nullptr;
        std::vector<doll_slot> slots;
        int selected_slot = -1;
        item_location selected_inv;
        item_location selected_worn;
        item_location drag_payload; // owned for ImGui drag-drop lifetime
        // Reload (select_ammo) must run AFTER the ImGui equipment window is
        // destroyed — nesting the ammo picker under an open Hybrid panel made
        // select_ammo return empty for MAGAZINE_WELL tools (fire_drill).
        item_location deferred_reload_loc;
        std::string last_action;
        input_context ctxt;
        bool open_classic = false;
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
        void accept_equipment_drop( int slot );
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
    ctxt.set_timeout( 16 );

    while( true ) {
        ui_manager::redraw_invalidated();
        // Inventory mutations must run after ImGui finishes the frame so tooltips /
        // remaining grid cells never touch dangling item_location pointers.
        flush_pending_action();
        last_action = ctxt.handle_input();

        if( want_close ) {
            break;
        }
        if( open_classic ) {
            break;
        }
        if( ( last_action == "QUIT" && !cataimgui::client::want_text_input() ) || !get_is_open() ) {
            break;
        }
        if( last_action == "CONFIRM" && !cataimgui::client::want_text_input() ) {
            try_equip_selected();
        }
    }

    return false;
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
    // Dropping a compatible magazine/ammo onto an occupied doll slot (e.g.
    // notched stick → bow fire drill) should Reload, not try to replace the
    // wielded item (which only offers Store/Drop/Wear via dispose_item).
    item_location on_slot = item_on_slot( *you, slot );
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
    if( slot.bp != bodypart_str_id::NULL_ID().id() && !selected_inv->covers( slot.bp ) ) {
        status_line = _( "That item does not fit the selected body part." );
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
        item_location loc = selected_inv;
        clear_selections();
        if( you->wear( loc ) ) {
            status_line = string_format( _( "Equipped to %s." ), slot.label );
        } else {
            status_line = _( "Could not wear that." );
        }
        return;
    }
    // Non-armor dropped on a wear slot: fall back to wield.
    try_wield_selected();
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
        if( you->can_unwield( *loc ).success() && you->unwield() ) {
            status_line = _( "Unwielded." );
            clear_selections();
        } else {
            status_line = you->can_unwield( *loc ).str();
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
    const std::vector<texture_draw_data> layers = tilecontext->get_character_preview( *you );
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
        ImGui::TextWrapped( "%s", _( "Drag an item onto the survivor or an equipment slot, then Apply equipment change." ) );
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
                ImGui::TextDisabled( "%s", _( "Wearing adds a layer; existing clothing stays on. Layering can add extra encumbrance." ) );
            }
        } else {
            ImGui::TextDisabled( "%s", _( "This item does not cover the selected body part." ) );
        }
    }
}

void rpg_equipment_window::accept_equipment_drop( int slot )
{
    if( !ImGui::BeginDragDropTarget() ) {
        return;
    }
    if( const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(
                "RPG_EQ_ITEM", ImGuiDragDropFlags_AcceptBeforeDelivery ) ) {
        if( payload->IsDelivery() && drag_payload && drag_payload.get_item() ) {
            selected_inv = drag_payload;
            selected_slot = slot;
            selected_worn = slot >= 0 ? item_on_slot( *you, slots[slot] ) : item_location::nowhere;
            equip_preview = true;
            preview_item = selected_inv;
            preview_slot = slot;
            status_line = string_format( _( "%s is ready. Choose Apply equipment change." ),
                                         selected_inv->type_name() );
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
    ImGui::BeginChild( "paper_doll", ImVec2( ImGui::GetContentRegionAvail().x * 0.44f, 0 ),
                       ImGuiChildFlags_Borders, ImGuiWindowFlags_NoNav );
    ui_hybrid_chrome::section_header( _( "Worn and wielded" ) );
    const float available = ImGui::GetContentRegionAvail().x;
    const float slot_w = std::max( 54.f, std::min( 110.f, available * 0.27f ) );
    const float slot_h = std::max( 48.f, ImGui::GetTextLineHeight() * 2.9f );
    const float gap = 6.f;
    const ImVec2 origin = ImGui::GetCursorPos();
    const ImVec2 screen = ImGui::GetCursorScreenPos();
    const float doll_height = 7.f * ( slot_h + gap );
    // The visible survivor is a drop target too, not just the small slot buttons.
    ImGui::SetCursorPos( ImVec2( origin.x + slot_w + gap, origin.y + slot_h + gap ) );
    ImGui::InvisibleButton( "survivor_drop", ImVec2(
                               std::max( 1.f, available - 2.f * ( slot_w + gap ) ),
                               doll_height - 2.f * ( slot_h + gap ) ) );
    accept_equipment_drop( -1 );
    draw_survivor( ImVec2( screen.x + slot_w + gap, screen.y + slot_h + gap ),
                   ImVec2( screen.x + available - slot_w - gap, screen.y + doll_height - slot_h - gap ) );
    const int item_name_chars = 12;
    auto slot_position = [&]( const doll_slot & slot ) -> ImVec2 {
        int column = 0, row = 0;
        if( slot.type == doll_slot::kind::weapon ) { row = 6; }
        else if( slot.type == doll_slot::kind::offhand ) { column = 2; row = 6; }
        else if( slot.type == doll_slot::kind::back ) { column = 1; row = 6; }
        else if( slot.type == doll_slot::kind::body_outer ) { column = 2; row = 1; }
        else {
            const std::string id = slot.bp.id().str();
            if( id == "head" ) { column = 1; }
            else if( id == "mouth" ) { column = 2; }
            else if( id == "torso" ) { row = 1; }
            else if( id == "arm_l" ) { row = 2; }
            else if( id == "arm_r" ) { column = 2; row = 2; }
            else if( id == "hand_l" ) { row = 3; }
            else if( id == "hand_r" ) { column = 2; row = 3; }
            else if( id == "leg_l" ) { row = 4; }
            else if( id == "leg_r" ) { column = 2; row = 4; }
            else if( id == "foot_l" ) { row = 5; }
            else if( id == "foot_r" ) { column = 2; row = 5; }
        }
        return ImVec2( origin.x + column * ( available - slot_w ) * 0.5f,
                       origin.y + row * ( slot_h + gap ) );
    };

    bool doll_ctx_request = false;
    int doll_ctx_index = -1;
    item_location doll_ctx_loc;
    for( int i = 0; i < static_cast<int>( slots.size() ); i++ ) {
        const doll_slot &slot = slots[i];
        item_location worn_loc = item_on_slot( *you, slot );
        if( selected_slot == i && selected_worn && you->is_worn( *selected_worn ) &&
            slot.bp != bodypart_str_id::NULL_ID().id() && selected_worn->covers( slot.bp ) ) {
            worn_loc = selected_worn;
        }
        ImGui::SetCursorPos( slot_position( slot ) );
        // Resolve names into owned strings before any ImGui call that may hold
        // a const char* past this expression (SetTooltip keeps until end of frame).
        std::string right;
        std::string tip;
        ImVec4 tint = ImVec4( 0.7f, 0.7f, 0.7f, 1.f );
        if( worn_loc && worn_loc.get_item() ) {
            right = cell_label( *worn_loc, 1, item_name_chars );
            tip = worn_loc->display_name();
            tint = cataimgui::imvec4_from_color( worn_loc->color_in_inventory( you ) );
        } else {
            right = _( "— empty —" );
            worn_loc = item_location::nowhere;
        }

        const bool selected = ( selected_slot == i );
        const bool empty = !worn_loc;

        ImGui::PushID( i );
        const int slot_cols = ui_hybrid_chrome::push_slot_button( selected, empty );

        // Leading gutter so a left-side ITEM sprite does not cover the slot name.
        const std::string row = slot.label + "\n ";
        // Leave left padding in the label so a tileset sprite can sit in the
        // slot without covering the body-part name when drawn as an overlay.
        if( ImGui::Button( row.c_str(), ImVec2( slot_w, slot_h ) ) ) {
            selected_slot = i;
            selected_inv = item_location::nowhere;
            if( worn_loc ) {
                selected_worn = worn_loc;
                // Selection never removes gear. Use the explicit action or context menu.
            } else {
                selected_worn = item_location::nowhere;
            }
        }
        // Plain hover: do NOT use AllowWhenBlockedByPopup — while the RMB
        // menu is open that flag keeps cells under the popup "hovered", which
        // spawns tooltips that fight the menu and can scroll/jitter the grid.
        const bool hovered = ImGui::IsItemHovered();
        const bool rmb_down = ImGui::IsMouseClicked( ImGuiMouseButton_Right );
        const bool rmb_up = ImGui::IsMouseReleased( ImGuiMouseButton_Right );
        const bool want_cap = ImGui::GetIO().WantCaptureMouse;
        const ImGuiID slot_item_id = ImGui::GetItemID();
        const ImGuiID per_slot_popup_id = ImGui::GetID( "rpg_doll_ctx" );

        if( hovered && rmb_down ) {
            doll_rmb_down_slot = i;
        }
        if( hovered && ( rmb_down || rmb_up ) ) {
            rpg_eq_ctx_telem( rmb_telem_line, string_format(
                                  "DOLL %s slot=%d down=%d id=0x%08X popupId=0x%08X wantCap=%d t='%s'",
                                  rmb_down ? "RMB_DOWN" : "RMB_UP",
                                  i, doll_rmb_down_slot,
                                  static_cast<unsigned>( slot_item_id ),
                                  static_cast<unsigned>( per_slot_popup_id ),
                                  want_cap ? 1 : 0,
                                  rpg_eq_short_tname( worn_loc ) ) );
        }

        // Drag-drop target: inventory grid → this doll slot (needs Button LastItem).
        accept_equipment_drop( i );

        // Defer shared doll popup until after PopID + EndChild (parent id stack).
        // Require same slot for RMB_DOWN and RMB_UP so adjacent rows cannot steal
        // the release (Deck drift: DOWN slot=14 UP slot=13 → wrong popup).
        if( hovered && rmb_up && worn_loc && worn_loc.get_item() ) {
            if( doll_rmb_down_slot == i ) {
                doll_ctx_request = true;
                doll_ctx_index = i;
                doll_ctx_loc = worn_loc;
            } else if( doll_rmb_down_slot >= 0 ) {
                rpg_eq_ctx_telem( rmb_telem_line, string_format(
                                      "DOLL RMB_UP MISMATCH down=%d up=%d t='%s'",
                                      doll_rmb_down_slot, i,
                                      rpg_eq_short_tname( worn_loc ) ) );
            }
        }

        ui_hybrid_chrome::draw_item_bezel( selected, hovered, empty );
        if( worn_loc && worn_loc.get_item() ) {
            // Draw-list sprite only — ImGui::Image would steal last-item from the
            // slot Button and break drag-drop target + right-click context.
            const ImVec2 rmin = ImGui::GetItemRectMin();
            const ImVec2 rmax = ImGui::GetItemRectMax();
            const float row_h = rmax.y - rmin.y;
            const float icon_sz = std::clamp( row_h - ImGui::GetTextLineHeight() - 6.f, 14.f, 36.f );
#if defined(TILES)
            if( get_option<bool>( "USE_TILES" ) && tilecontext ) {
                const itype_id &iid = worn_loc->typeId();
                if( iid.is_valid() ) {
                    const std::optional<texture_draw_data> data =
                        tilecontext->get_texture_draw_data( iid.str(), TILE_CATEGORY::ITEM,
                                                            tripoint_bub_ms() );
                    if( data ) {
                        const ImVec2 p0( ( rmin.x + rmax.x - icon_sz ) * 0.5f,
                                         rmax.y - icon_sz - 3.f );
                        const ImVec2 p1( p0.x + icon_sz, p0.y + icon_sz );
                        ImGui::GetWindowDrawList()->AddImage(
                            reinterpret_cast<ImTextureID>( data->texture ), p0, p1,
                            ImVec2( data->uv0.first, data->uv0.second ),
                            ImVec2( data->uv1.first, data->uv1.second ) );
                    }
                }
            }
#endif
        }
        if( hovered && !tip.empty() ) {
            imgui_cdda_tooltip( tip );
        }

        // Tint hint via draw-list (avoid TextUnformatted stealing LastItem).
        {
            const ImVec2 rmin = ImGui::GetItemRectMin();
            const ImVec2 rmax = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2( rmax.x - 5.f, rmin.y + 3.f ),
                ImVec2( rmax.x - 2.f, rmax.y - 3.f ),
                ImGui::ColorConvertFloat4ToU32( tint ) );
        }

        ImGui::PopStyleColor( slot_cols );
        ImGui::PopID();
    }

    ImGui::SetCursorPos( ImVec2( origin.x, origin.y + doll_height ) );
    ImGui::Dummy( ImVec2( 1.f, 1.f ) );
    if( selected_slot >= 0 && selected_slot < static_cast<int>( slots.size() ) &&
        slots[selected_slot].bp != bodypart_str_id::NULL_ID().id() ) {
        ui_hybrid_chrome::section_header( _( "Clothing layers" ) );
        int layer_index = 0;
        for( const item_location &loc : you->top_items_loc() ) {
            if( !loc || !you->is_worn( *loc ) || !loc->covers( slots[selected_slot].bp ) ) {
                continue;
            }
            ImGui::PushID( 5000 + layer_index++ );
            if( ImGui::Selectable( remove_color_tags( loc->display_name() ).c_str(), selected_worn == loc ) ) {
                selected_worn = loc;
                selected_inv = item_location::nowhere;
            }
            ImGui::PopID();
        }
        if( layer_index == 0 ) {
            ImGui::TextDisabled( "%s", _( "Nothing worn here." ) );
        }
    }

    // OpenPopup/BeginPopup must run OUTSIDE paper_doll child so the popup is
    // hashed at the parent Character Equipment window (not clipped/stacked
    // under the child) and can draw above the panel.
    ImGui::EndChild();

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
            case item_context_menu::action::pickup:
            case item_context_menu::action::always_pickup:
            case item_context_menu::action::never_pickup:
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
    // NoNav: prevent gamepad L-stick from scrolling the inventory grid while
    // a context popup is open (NavWindow would otherwise be this child).
    ImGui::BeginChild( "inv_grid", ImVec2( 0, 0 ), ImGuiChildFlags_Borders,
                       ImGuiWindowFlags_NoNav );
    ImGui::TextColored( ui_hybrid_chrome::palette::accent(), "%s",
                        _( "Inventory" ) );
    ImGui::Separator();

    ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x * 0.62f );
    ImGui::InputTextWithHint( "##inventory_filter", _( "Find carried items…" ),
                              inventory_filter, sizeof( inventory_filter ) );
    ImGui::SameLine();
    const char *categories[] = { _( "All" ), _( "Clothing" ), _( "Food / drink" ),
                                _( "Tools" ), _( "Weapons" ) };
    ImGui::SetNextItemWidth( -1 );
    ImGui::Combo( "##category", &inventory_category, categories, 5 );

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
        if( inventory_filter[0] != '\0' &&
            !lcmatch( remove_color_tags( loc->display_name() ), inventory_filter ) ) {
            continue;
        }
        if( ( inventory_category == 1 && !loc->is_armor() ) ||
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
    const float min_cell = 40.f;
    const float max_cell = 52.f;
    const float cell_gap = 2.f;
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
    const float cell_h = cell_w; // square-ish inventory cells
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
        ImGui::TextDisabled( "%s", _( "No carried items in containers." ) );
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
        const int grid_cols = ui_hybrid_chrome::push_grid_button( is_sel );

        // Invisible label (PushID scopes uniqueness); sprite / glyph drawn as overlay.
        if( ImGui::Button( "##inv_cell", ImVec2( cell_w, cell_h ) ) ) {
            if( is_sel ) {
                // Second click: Use when applicable, otherwise equip onto selected doll slot
                if( cell.usable ) {
                    pending = pending_action::use_item;
                    pending_use_method.clear();
                } else {
                    pending = pending_action::equip;
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

        ui_hybrid_chrome::draw_item_bezel( is_sel, hovered, false );
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
            overlay_item_sprite_on_last_item( *cell.loc,
                                              static_cast<int>( cell.locs.size() ), fb );
        }
        if( hovered ) {
            imgui_cdda_tooltip( cell.tip );
        }

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

    // Soft-fork: Nearby — ground items in the 3x3 around the avatar (underfoot
    // + 8 adjacent). Lets the player grab a corpse under tall grass without
    // leaving the Hybrid inventory UI.
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored( ui_hybrid_chrome::palette::accent(), "%s",
                        _( "Nearby" ) );
    ImGui::SameLine();
    ImGui::TextDisabled( "%s", _( "(3×3 around you)" ) );
    ImGui::Separator();

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
    {
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

    if( nearby_items.empty() ) {
        ImGui::TextDisabled( "%s", _( "Nothing nearby on the ground." ) );
    } else {
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

            ui_hybrid_chrome::draw_item_bezel( is_sel, hovered, false );
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
    ImGui::EndChild();

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
            case item_context_menu::action::always_pickup:
            case item_context_menu::action::never_pickup:
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
    auto action_btn = []( const char *label ) {
        const int n = ui_hybrid_chrome::push_toolbar_button( false );
        const bool clicked = ImGui::Button( label );
        ui_hybrid_chrome::draw_item_bezel( false, ImGui::IsItemHovered(), false );
        ImGui::PopStyleColor( n );
        return clicked;
    };

    if( action_btn( _( "Wear / Wield" ) ) ) {
        pending = pending_action::equip;
    }
    ImGui::SameLine();
    if( action_btn( _( "Take Off" ) ) ) {
        pending = pending_action::takeoff;
    }
    ImGui::SameLine();
    if( action_btn( _( "Wield" ) ) ) {
        pending = pending_action::wield;
    }
    ImGui::SameLine();
    if( action_btn( _( "Use" ) ) ) {
        pending = pending_action::use_item;
    }
    ImGui::SameLine();
    if( action_btn( _( "Classic Inv…" ) ) ) {
        open_classic = true;
    }
    ImGui::SameLine();
    if( action_btn( _( "Close" ) ) ) {
        want_close = true;
    }

    if( !status_line.empty() ) {
        ImGui::TextWrapped( "%s", status_line.c_str() );
    } else {
        ImGui::TextWrapped( "%s",
                             _( "Drag onto the survivor or a slot, then Apply. Right-click any item for actions." ) );
    }
    if( RPG_EQ_CTX_TELEM && !rmb_telem_line.empty() ) {
        ImGui::TextColored( ImVec4( 0.95f, 0.75f, 0.25f, 1.f ),
                            "RMB dbg: %s", rmb_telem_line.c_str() );
    }
}

void rpg_equipment_window::draw_controls()
{
    if( hide_ui ) {
        hide_if_hidden();
        return;
    }
    const float footer = ImGui::GetTextLineHeightWithSpacing() * 8.f + 30.f;
    ImGui::BeginChild( "equipment_body", ImVec2( 0.f,
                       std::max( 180.f, ImGui::GetContentRegionAvail().y - footer ) ) );
    draw_paper_doll();
    ImGui::SameLine();
    draw_inventory_grid();
    ImGui::EndChild();
    ImGui::Separator();
    if( equip_preview && ( !preview_item || selected_inv != preview_item || selected_slot != preview_slot ) ) {
        equip_preview = false;
    }
    if( equip_preview ) {
        if( ImGui::Button( _( "Apply equipment change" ) ) ) {
            selected_inv = preview_item;
            selected_slot = preview_slot;
            pending = pending_action::drag_equip;
            equip_preview = false;
        }
        ImGui::SameLine();
        if( ImGui::Button( _( "Cancel change" ) ) ) {
            equip_preview = false;
            status_line = _( "Equipment change cancelled." );
        }
    }
    // Keep Apply/Cancel above details, which may wrap over several lines.
    ImGui::BeginChild( "equipment_inspector", ImVec2( 0.f,
                       std::max( ImGui::GetTextLineHeightWithSpacing(),
                                 ImGui::GetContentRegionAvail().y -
                                 ImGui::GetFrameHeightWithSpacing() * 3.f ) ) );
    draw_equipment_inspection();
    ImGui::EndChild();
    draw_action_bar();

    // Parent cataimgui::window::draw() BringWindowToDisplayFront(Equipment)
    // after this returns — that would bury our context popups under the panel.
    // Skip that front-bring while a ctx popup is open (IDs hashed at parent).
    force_to_back = ImGui::IsPopupOpen( "rpg_inv_ctx" ) ||
                    ImGui::IsPopupOpen( "rpg_doll_ctx" );

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
    {
        rpg_equipment_window win( &you );
        win.execute();
        reload = win.reload_after_close();
        classic = win.classic_after_close();
    }
    if( classic ) {
        game_menus::inv::common();
    }
    if( reload ) {
        item_context_menu::perform( you, reload, item_context_menu::action::reload );
    }
}

} // namespace rpg_equipment_ui
