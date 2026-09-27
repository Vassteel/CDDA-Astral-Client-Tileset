#include "rpg_equipment_ui.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "avatar_action.h"
#include "bodypart.h"
#include "cata_imgui.h"
#include "ui_hybrid_chrome.h"
#include "catacharset.h"
#include "character.h"
#include "color.h"
#include "enums.h"
#include "flag.h"
#include "game.h"
#include "game_inventory.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "messages.h"
#include "options.h"
#include "output.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_iteminfo.h"
#include "ui_manager.h"

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
        if( !*it || !( *it )->covers( bp ) ) {
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
        if( *it && is_outer_on_torso( **it ) && !covers_hanging_back( **it ) ) {
            return *it;
        }
    }
    return item_location::nowhere;
}

static item_location worn_on_back( Character &you )
{
    const std::vector<item_location> worn = you.top_items_loc();
    for( auto it = worn.rbegin(); it != worn.rend(); ++it ) {
        if( *it && covers_hanging_back( **it ) ) {
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
    drag_equip
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

    protected:
        void draw() override {
            // Push Hybrid chrome before Begin so WindowBg / borders apply.
            ui_hybrid_chrome::push();
            cataimgui::window::draw();
            ui_hybrid_chrome::pop();
        }
        void draw_controls() override;
        cataimgui::bounds get_bounds() override {
            const float window_width = std::clamp( float( str_width_to_pixels( EVEN_MINIMUM_TERM_WIDTH ) ),
                                                   ImGui::GetMainViewport()->Size.x * 0.55f,
                                                   ImGui::GetMainViewport()->Size.x * 0.92f );
            const float window_height = std::clamp( float( str_height_to_pixels( EVEN_MINIMUM_TERM_HEIGHT ) ),
                                                    ImGui::GetMainViewport()->Size.y * 0.55f,
                                                    ImGui::GetMainViewport()->Size.y * 0.92f );
            return { -1.f, -1.f, window_width, window_height };
        }

    private:
        Character *you = nullptr;
        std::vector<doll_slot> slots;
        int selected_slot = -1;
        item_location selected_inv;
        item_location selected_worn;
        item_location drag_payload; // owned for ImGui drag-drop lifetime
        std::string last_action;
        input_context ctxt;
        bool open_classic = false;
        bool want_close = false;
        std::string status_line;
        pending_action pending = pending_action::none;

        void draw_paper_doll();
        void draw_inventory_grid();
        void draw_action_bar();
        void try_equip_selected();
        void try_takeoff_selected();
        void try_wield_selected();
        void try_use_selected();
        void try_drop_selected();
        void try_examine_selected();
        void try_drag_equip();
        void refresh_selection_validity();
        void clear_selections();
        void flush_pending_action();
};

void rpg_equipment_window::clear_selections()
{
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
        if( last_action == "QUIT" || !get_is_open() ) {
            break;
        }
        if( last_action == "CONFIRM" ) {
            try_equip_selected();
        }
    }

    if( open_classic ) {
        game_menus::inv::common();
    }
    return false;
}

void rpg_equipment_window::flush_pending_action()
{
    const pending_action act = pending;
    pending = pending_action::none;
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
            try_use_selected();
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
    if( slot.type == doll_slot::kind::weapon ) {
        try_wield_selected();
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

void rpg_equipment_window::try_use_selected()
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

void rpg_equipment_window::draw_paper_doll()
{
    ImGui::BeginChild( "paper_doll", ImVec2( ImGui::GetContentRegionAvail().x * 0.44f, 0 ),
                       ImGuiChildFlags_Borders );
    ImGui::TextColored( ui_hybrid_chrome::palette::accent(), "%s",
                        _( "Equipped (slots)" ) );
    ImGui::Separator();

    const float slot_w = ImGui::GetContentRegionAvail().x;
    // Leave room for the slot label (~10 cells) + gap; rest is the item name.
    const int item_name_chars = std::max( 10,
                                          static_cast<int>( ( slot_w - 90.f ) / std::max( 1.f,
                                                  ImGui::CalcTextSize( "W" ).x ) ) );

    for( int i = 0; i < static_cast<int>( slots.size() ); i++ ) {
        const doll_slot &slot = slots[i];
        item_location worn_loc = item_on_slot( *you, slot );
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

        const std::string row = string_format( "%-9s %s", slot.label, right );
        if( ImGui::Button( row.c_str(), ImVec2( -1.f, 0.f ) ) ) {
            selected_slot = i;
            if( worn_loc ) {
                selected_worn = worn_loc;
                // Click worn item again (already selected) → take off after frame
                if( selected && selected_worn ) {
                    pending = pending_action::takeoff;
                }
            } else {
                selected_worn = item_location::nowhere;
            }
        }
        const bool hovered = ImGui::IsItemHovered();
        ui_hybrid_chrome::draw_item_bezel( selected, hovered, empty );
        if( hovered && !tip.empty() ) {
            imgui_cdda_tooltip( tip );
        }

        // Drag-drop target: inventory grid → this doll slot
        if( ImGui::BeginDragDropTarget() ) {
            if( const ImGuiPayload *payload =
                    ImGui::AcceptDragDropPayload( "RPG_EQ_ITEM" ) ) {
                ( void )payload;
                if( drag_payload && drag_payload.get_item() ) {
                    selected_inv = drag_payload;
                    selected_slot = i;
                    pending = pending_action::drag_equip;
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Keep inventory-tint hint as a tiny colored marker (chrome stays Hybrid).
        ImGui::PushStyleColor( ImGuiCol_Text, tint );
        ImGui::TextUnformatted( " " );
        ImGui::PopStyleColor();

        ImGui::PopStyleColor( slot_cols );
        ImGui::PopID();
    }

    ImGui::EndChild();
}

void rpg_equipment_window::draw_inventory_grid()
{
    ImGui::BeginChild( "inv_grid", ImVec2( 0, 0 ), ImGuiChildFlags_Borders );
    ImGui::TextColored( ui_hybrid_chrome::palette::accent(), "%s",
                        _( "Inventory" ) );
    ImGui::Separator();

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

    // ~50% smaller than BG3 mock showcase icons: dense pack of small cells
    // (chrome-only; icon atlases later). Equipped gear on doll/slots only —
    // no duplicate equipped-item list beside the slot ring.
    const float avail = ImGui::GetContentRegionAvail().x;
    const float min_cell = 48.f;
    const float max_cell = 64.f;
    const float cell_gap = 3.f;
    int columns = std::max( 1, static_cast<int>( avail / ( min_cell + cell_gap ) ) );
    columns = std::min( columns, 16 );
    float cell_w = ( avail - cell_gap * static_cast<float>( columns ) ) /
                   static_cast<float>( columns );
    cell_w = std::clamp( cell_w, min_cell, max_cell );
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

    int col = 0;
    for( int i = 0; i < static_cast<int>( grid_items.size() ); i++ ) {
        grid_cell &cell = grid_items[i];
        ImGui::PushID( 1000 + i );

        const bool is_sel = selected_inv && std::any_of( cell.locs.begin(), cell.locs.end(),
        [&]( const item_location & l ) {
            return selected_inv == l;
        } );
        const int grid_cols = ui_hybrid_chrome::push_grid_button( is_sel );

        if( ImGui::Button( cell.label.c_str(), ImVec2( cell_w, 22.f ) ) ) {
            if( is_sel ) {
                // Second click: Use when applicable, otherwise equip onto selected doll slot
                if( cell.usable ) {
                    pending = pending_action::use_item;
                } else {
                    pending = pending_action::equip;
                }
            } else {
                // Operate on the first / representative item of the display stack.
                selected_inv = cell.loc;
            }
        }
        const bool hovered = ImGui::IsItemHovered();
        ui_hybrid_chrome::draw_item_bezel( is_sel, hovered, false );
        if( hovered ) {
            imgui_cdda_tooltip( cell.tip );
        }

        // Drag source → drop on paper-doll slots
        if( ImGui::BeginDragDropSource( ImGuiDragDropFlags_SourceAllowNullID ) ) {
            drag_payload = cell.loc;
            int token = i;
            ImGui::SetDragDropPayload( "RPG_EQ_ITEM", &token, sizeof( token ) );
            ImGui::TextUnformatted( cell.label.c_str() );
            ImGui::EndDragDropSource();
        }

        // Right-click context menu (BeginPopupContextItem keeps ID with this cell).
        if( ImGui::BeginPopupContextItem( "rpg_inv_ctx" ) ) {
            selected_inv = cell.loc;
            refresh_selection_validity();
            const bool has = selected_inv && selected_inv.get_item();
            const bool usable = has && item_looks_usable( *selected_inv );
            if( ImGui::MenuItem( _( "Use" ), nullptr, false, usable ) ) {
                pending = pending_action::use_item;
            }
            if( ImGui::MenuItem( _( "Wear / Wield" ), nullptr, false, has ) ) {
                pending = pending_action::equip;
            }
            if( ImGui::MenuItem( _( "Wield" ), nullptr, false, has ) ) {
                pending = pending_action::wield;
            }
            if( ImGui::MenuItem( _( "Drop" ), nullptr, false, has ) ) {
                pending = pending_action::drop_item;
            }
            if( ImGui::MenuItem( _( "Examine" ), nullptr, false, has ) ) {
                pending = pending_action::examine_item;
            }
            ImGui::EndPopup();
        }

        ImGui::PopStyleColor( grid_cols );
        ImGui::PopID();

        col++;
        if( col < columns ) {
            ImGui::SameLine();
        } else {
            col = 0;
        }
    }

    ImGui::EndChild();
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
        ImGui::TextDisabled( "%s",
                             _( "Drag inventory → doll slot to equip. Click item again to Use (or Wear/Wield). "
                                "Right-click for Use / Wear-Wield / Drop / Examine. "
                                "Click a worn slot twice to take off. Esc closes." ) );
    }
}

void rpg_equipment_window::draw_controls()
{
    draw_action_bar();
    ImGui::Separator();
    draw_paper_doll();
    ImGui::SameLine();
    draw_inventory_grid();

    if( !get_is_open() ) {
        want_close = true;
    }
}

} // namespace

namespace rpg_equipment_ui
{

void open()
{
    Character &you = get_player_character();
    rpg_equipment_window win( &you );
    win.execute();
}

} // namespace rpg_equipment_ui
