#include "rpg_equipment_ui.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "cata_imgui.h"
#include "character.h"
#include "color.h"
#include "game.h"
#include "game_inventory.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "item.h"
#include "item_location.h"
#include "messages.h"
#include "options.h"
#include "output.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_manager.h"

namespace
{

struct doll_slot {
    enum class kind { body, weapon } type;
    bodypart_id bp;
    std::string label;
};

static std::vector<doll_slot> make_doll_slots( Character &you )
{
    std::vector<doll_slot> slots;
    const auto add_bp = [&]( const bodypart_str_id &id, const char *fallback_label ) {
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

    return slots;
}

/** Outermost worn item covering bp, or nowhere. */
static item_location worn_on_slot( Character &you, const bodypart_id &bp )
{
    const std::vector<item_location> worn = you.top_items_loc();
    for( auto it = worn.rbegin(); it != worn.rend(); ++it ) {
        if( *it && ( *it )->covers( bp ) ) {
            return *it;
        }
    }
    return item_location::nowhere;
}

static item_location item_on_slot( Character &you, const doll_slot &slot )
{
    if( slot.type == doll_slot::kind::weapon ) {
        return you.get_wielded_item();
    }
    return worn_on_slot( you, slot.bp );
}

static std::string cell_label( const item &it )
{
    std::string name = it.tname( 1, false );
    if( name.size() > 18 ) {
        name = name.substr( 0, 16 ) + "…";
    }
    if( it.count_by_charges() && it.charges > 1 ) {
        return string_format( "%s×%d", name, it.charges );
    }
    if( it.count() > 1 ) {
        return string_format( "%s×%d", name, it.count() );
    }
    // Letter fallback when name is awkward
    if( it.invlet != 0 ) {
        return string_format( "[%c] %s", it.invlet, name );
    }
    return name;
}

enum class pending_action {
    none,
    equip,
    takeoff,
    wield
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
            // Close this UI first so classic inventory owns the screen.
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

void rpg_equipment_window::draw_paper_doll()
{
    ImGui::BeginChild( "paper_doll", ImVec2( ImGui::GetContentRegionAvail().x * 0.42f, 0 ),
                       ImGuiChildFlags_Borders );
    ImGui::TextUnformatted( _( "Equipment (paper doll)" ) );
    ImGui::Separator();

    for( int i = 0; i < static_cast<int>( slots.size() ); i++ ) {
        const doll_slot &slot = slots[i];
        item_location worn_loc = item_on_slot( *you, slot );
        // Resolve names into owned strings before any ImGui call that may hold
        // a const char* past this expression (SetTooltip keeps until end of frame).
        std::string right;
        std::string tip;
        ImVec4 tint = ImVec4( 0.7f, 0.7f, 0.7f, 1.f );
        if( worn_loc && worn_loc.get_item() ) {
            right = cell_label( *worn_loc );
            tip = worn_loc->display_name();
            tint = cataimgui::imvec4_from_color( worn_loc->color_in_inventory( you ) );
        } else {
            right = _( "— empty —" );
            worn_loc = item_location::nowhere;
        }

        const bool selected = ( selected_slot == i );

        ImGui::PushID( i );
        if( selected ) {
            ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.35f, 0.45f, 0.25f, 1.f ) );
        }

        const std::string row = string_format( "%-10s  %s", slot.label, right );
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
        if( ImGui::IsItemHovered() && !tip.empty() ) {
            ImGui::SetTooltip( "%s", tip.c_str() );
        }
        // Colored underline hint
        ImGui::PushStyleColor( ImGuiCol_Text, tint );
        ImGui::TextUnformatted( " " );
        ImGui::PopStyleColor();

        if( selected ) {
            ImGui::PopStyleColor();
        }
        ImGui::PopID();
    }

    ImGui::EndChild();
}

void rpg_equipment_window::draw_inventory_grid()
{
    ImGui::BeginChild( "inv_grid", ImVec2( 0, 0 ), ImGuiChildFlags_Borders );
    ImGui::TextUnformatted( _( "Inventory (grid)" ) );
    ImGui::Separator();

    // Snapshot labels up front so ImGui never sees a temporary .c_str(), and so
    // a later deferred wear/takeoff cannot leave dangling item* mid-draw.
    struct grid_cell {
        item_location loc;
        std::string label;
        std::string tip;
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
        grid_cell cell;
        cell.loc = loc;
        cell.label = cell_label( *loc );
        cell.tip = loc->display_name();
        grid_items.push_back( std::move( cell ) );
    }

    if( grid_items.empty() ) {
        ImGui::TextDisabled( "%s", _( "No carried items in containers." ) );
    }

    const float cell_w = 110.f;
    const float avail = ImGui::GetContentRegionAvail().x;
    const int columns = std::max( 1, static_cast<int>( avail / ( cell_w + 8.f ) ) );

    int col = 0;
    for( int i = 0; i < static_cast<int>( grid_items.size() ); i++ ) {
        grid_cell &cell = grid_items[i];
        ImGui::PushID( 1000 + i );

        const bool is_sel = selected_inv && selected_inv == cell.loc;
        if( is_sel ) {
            ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.25f, 0.40f, 0.55f, 1.f ) );
        }

        if( ImGui::Button( cell.label.c_str(), ImVec2( cell_w, 40.f ) ) ) {
            if( is_sel ) {
                // Double-click-ish: second click equips onto selected doll slot
                pending = pending_action::equip;
            } else {
                selected_inv = cell.loc;
            }
        }
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s", cell.tip.c_str() );
        }
        if( ImGui::IsItemClicked( ImGuiMouseButton_Right ) ) {
            selected_inv = cell.loc;
            pending = pending_action::equip;
        }

        if( is_sel ) {
            ImGui::PopStyleColor();
        }
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
    if( ImGui::Button( _( "Wear / Wield" ) ) ) {
        pending = pending_action::equip;
    }
    ImGui::SameLine();
    if( ImGui::Button( _( "Take Off" ) ) ) {
        pending = pending_action::takeoff;
    }
    ImGui::SameLine();
    if( ImGui::Button( _( "Wield" ) ) ) {
        pending = pending_action::wield;
    }
    ImGui::SameLine();
    if( ImGui::Button( _( "Classic Inv…" ) ) ) {
        open_classic = true;
    }
    ImGui::SameLine();
    if( ImGui::Button( _( "Close" ) ) ) {
        want_close = true;
    }

    if( !status_line.empty() ) {
        ImGui::TextWrapped( "%s", status_line.c_str() );
    } else {
        ImGui::TextDisabled( "%s",
                             _( "Click a doll slot, then an inventory item (or Wear/Wield). "
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
