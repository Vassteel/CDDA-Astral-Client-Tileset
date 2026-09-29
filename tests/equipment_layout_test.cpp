#include "avatar.h"
#include <algorithm>
#include "cata_catch.h"
#include "character.h"
#include "equipment_layout.h"
#include "rpg_equipment_ui.h"
#include "item.h"
#include "item_location.h"
#include "player_helpers.h"
#include "type_id.h"
#include "string_formatter.h"
#include "bodypart.h"

using equipment_layout::classify_item;
using equipment_layout::display_layer;
using equipment_layout::equipment_role;
using equipment_layout::map_character;
using equipment_layout::region_id;
using equipment_layout::storage_slot;

static bool covers_region( const equipment_layout::item_profile &profile, region_id id )
{
    for( const equipment_layout::region_ref &ref : profile.covered_regions ) {
        if( ref.id == id ) {
            return true;
        }
    }
    return false;
}

TEST_CASE( "Equipment_layout_sleeves_vs_sleeveless", "[equipment_layout]" )
{
    const auto sleeved = classify_item( item( itype_id( "longshirt" ) ) );
    CHECK( sleeved.equip_destination.id == region_id::torso );
    CHECK( covers_region( sleeved, region_id::torso ) );
    CHECK( covers_region( sleeved, region_id::arm_l ) );
    CHECK( covers_region( sleeved, region_id::arm_r ) );

    const auto short_sleeve = classify_item( item( itype_id( "tshirt" ) ) );
    CHECK( covers_region( short_sleeve, region_id::torso ) );
    CHECK( covers_region( short_sleeve, region_id::arm_l ) );
    CHECK( covers_region( short_sleeve, region_id::arm_r ) );

    const auto sleeveless = classify_item( item( itype_id( "tank_top" ) ) );
    CHECK( covers_region( sleeveless, region_id::torso ) );
    CHECK_FALSE( covers_region( sleeveless, region_id::arm_l ) );
    CHECK_FALSE( covers_region( sleeveless, region_id::arm_r ) );
}

TEST_CASE( "Equipment_layout_shorts_vs_full_pants", "[equipment_layout]" )
{
    const auto shorts = classify_item( item( itype_id( "shorts" ) ) );
    CHECK( covers_region( shorts, region_id::pants ) );
    CHECK_FALSE( covers_region( shorts, region_id::leg_lower_l ) );
    CHECK_FALSE( covers_region( shorts, region_id::leg_lower_r ) );

    const auto jeans = classify_item( item( itype_id( "jeans" ) ) );
    CHECK( covers_region( jeans, region_id::pants ) );
    CHECK( covers_region( jeans, region_id::leg_lower_l ) );
    CHECK( covers_region( jeans, region_id::leg_lower_r ) );
}

TEST_CASE( "Equipment_layout_full_suit_links_same_item", "[equipment_layout]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    item suit( itype_id( "hazmat_suit" ) );
    REQUIRE( you.wear_item( suit, false ).has_value() );

    const equipment_layout::character_map mapped = map_character( you );
    int suit_entries = 0;
    const item *suit_ptr = nullptr;
    for( const equipment_layout::mapped_item &entry : mapped.items ) {
        if( entry.location && entry.location->typeId() == itype_id( "hazmat_suit" ) ) {
            ++suit_entries;
            suit_ptr = entry.location.get_item();
            CHECK( covers_region( classify_item( *entry.location ), region_id::torso ) );
            CHECK( covers_region( classify_item( *entry.location ), region_id::arm_l ) );
            CHECK( covers_region( classify_item( *entry.location ), region_id::leg_lower_l ) );
            CHECK( covers_region( classify_item( *entry.location ), region_id::foot_l ) );
            // One authoritative ownership record (linked coverage, not duplicates).
            CHECK( entry.covered_regions.size() >= 5 );
        }
    }
    CHECK( suit_entries == 1 );
    CHECK( suit_ptr != nullptr );
}

TEST_CASE( "Equipment_layout_paired_ear_equipment", "[equipment_layout]" )
{
    const auto muffs = classify_item( item( itype_id( "powered_earmuffs" ) ) );
    CHECK( muffs.role == equipment_role::accessory );
    CHECK( ( muffs.equip_destination.id == region_id::ear_l ||
             muffs.equip_destination.id == region_id::ear_r ) );
    CHECK( covers_region( muffs, region_id::ear_l ) );
    CHECK( covers_region( muffs, region_id::ear_r ) );
    // Helmets that cover ears are not ear accessories.
    const auto helmet = classify_item( item( itype_id( "helmet_army" ) ) );
    CHECK( helmet.equip_destination.id == region_id::head );
    CHECK( helmet.role == equipment_role::clothing );
    CHECK_FALSE( helmet.equip_destination.id == region_id::ear_l );
    CHECK_FALSE( helmet.equip_destination.id == region_id::ear_r );
}

TEST_CASE( "Equipment_layout_ordinary_accessories", "[equipment_layout]" )
{
    const auto glasses = classify_item( item( itype_id( "glasses_eye" ) ) );
    CHECK( glasses.equip_destination.id == region_id::eyes );

    const auto mask = classify_item( item( itype_id( "mask_dust" ) ) );
    CHECK( mask.equip_destination.id == region_id::face );

    const auto watch = classify_item( item( itype_id( "wristwatch" ) ) );
    CHECK( watch.role == equipment_role::accessory );
    CHECK( ( watch.equip_destination.id == region_id::wrist_l ||
             watch.equip_destination.id == region_id::wrist_r ) );

    const auto ring = classify_item( item( itype_id( "gold_ring" ) ) );
    CHECK( ring.role == equipment_role::accessory );
    CHECK( ( ring.equip_destination.id == region_id::rings_l ||
             ring.equip_destination.id == region_id::rings_r ) );

    const auto bandana = classify_item( item( itype_id( "bandana_head" ) ) );
    CHECK( bandana.equip_destination.id == region_id::forehead );
}

TEST_CASE( "Equipment_layout_unusual_layers_remain_reachable", "[equipment_layout]" )
{
    // BELTED backpack maps to back bag (uncommon/strapped presentation).
    const auto pack = classify_item( item( itype_id( "backpack" ) ) );
    CHECK( pack.storage == storage_slot::back );
    CHECK( pack.equip_destination.id == region_id::back_bag );
    CHECK( pack.equip_destination.layer == display_layer::uncommon );

    // Outer jacket still classifies on torso outer.
    const auto jacket = classify_item( item( itype_id( "jacket_leather" ) ) );
    CHECK( covers_region( jacket, region_id::torso ) );
    CHECK( jacket.equip_destination.id == region_id::torso );
}

TEST_CASE( "Equipment_layout_exact_holder_and_backpack_with_sheath", "[equipment_layout]" )
{
    CHECK( classify_item( item( itype_id( "baldric" ) ) ).storage == storage_slot::scabbard );
    CHECK( classify_item( item( itype_id( "sheath" ) ) ).storage == storage_slot::sheath );
    CHECK( classify_item( item( itype_id( "back_holster" ) ) ).storage == storage_slot::holster );

    item loaded_pack( itype_id( "backpack" ) );
    loaded_pack.put_in( item( itype_id( "sheath" ) ), pocket_type::CONTAINER );
    // Classification must inspect the container itself, not nested holster actions.
    CHECK( classify_item( loaded_pack ).storage == storage_slot::back );
    CHECK( classify_item( loaded_pack ).equip_destination.id == region_id::back_bag );

    clear_avatar();
    avatar &you = get_avatar();
    REQUIRE( you.wear_item( item( itype_id( "backpack" ) ), false ).has_value() );
    item *pack = nullptr;
    for( item_location &loc : you.top_items_loc() ) {
        if( loc && loc->typeId() == itype_id( "backpack" ) ) {
            pack = loc.get_item();
            break;
        }
    }
    REQUIRE( pack != nullptr );
    item sheath( itype_id( "sheath" ) );
    REQUIRE( pack->put_in( sheath, pocket_type::CONTAINER ).success() );

    const equipment_layout::character_map mapped = map_character( you );
    bool saw_pack = false;
    bool saw_sheath_in_pack = false;
    item_location pack_loc;
    for( const equipment_layout::mapped_item &entry : mapped.items ) {
        if( entry.location && entry.location->typeId() == itype_id( "backpack" ) &&
            !entry.holder_location ) {
            saw_pack = true;
            pack_loc = entry.location;
            CHECK( entry.equip_destination.id == region_id::back_bag );
        }
    }
    for( const equipment_layout::mapped_item &entry : mapped.items ) {
        if( entry.location && entry.location->typeId() == itype_id( "sheath" ) &&
            entry.holder_location ) {
            saw_sheath_in_pack = true;
            CHECK( entry.holder_location == pack_loc );
            CHECK( entry.role == equipment_role::held );
        }
    }
    CHECK( saw_pack );
    CHECK( saw_sheath_in_pack );

    // Waist scabbard remains a waist holder destination.
    const auto scabbard = classify_item( item( itype_id( "baldric" ) ) );
    CHECK( scabbard.role == equipment_role::holder );
    CHECK( scabbard.equip_destination.id == region_id::waist );

    const auto back_scabbard = classify_item( item( itype_id( "bscabbard" ) ) );
    CHECK( back_scabbard.role == equipment_role::holder );
    CHECK( back_scabbard.equip_destination.id == region_id::back_weapon );
}

TEST_CASE( "Equipment_layout_storage_wrapper_matches_legacy_api", "[equipment_layout][doll_slots]" )
{
    using slot = rpg_equipment_ui::storage_slot;
    CHECK( rpg_equipment_ui::storage_slot_for( item( itype_id( "backpack" ) ) ) == slot::back );
    CHECK( rpg_equipment_ui::storage_slot_for( item( itype_id( "baldric" ) ) ) == slot::scabbard );
    CHECK( rpg_equipment_ui::storage_slot_for( item( itype_id( "sheath" ) ) ) == slot::sheath );
    CHECK( rpg_equipment_ui::storage_slot_for( item( itype_id( "back_holster" ) ) ) == slot::holster );
}


TEST_CASE( "Equipment_layout_visible_regions_include_hand_occupancy", "[equipment_layout][doll_review]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    const std::vector<equipment_layout::region_id> visible =
        equipment_layout::visible_regions_for( you );
    const auto has = [&]( region_id id ) {
        return std::find( visible.begin(), visible.end(), id ) != visible.end();
    };
    CHECK( has( region_id::main_hand ) );
    CHECK( has( region_id::off_hand ) );
    CHECK( has( region_id::torso ) );
    CHECK( has( region_id::hand_l ) );
    CHECK( has( region_id::hand_r ) );
}

TEST_CASE( "Equipment_layout_paired_gloves_either_hand", "[equipment_layout][doll_review]" )
{
    const item gloves( itype_id( "gloves_leather" ) );
    REQUIRE( gloves.covers( body_part_hand_l ) );
    REQUIRE( gloves.covers( body_part_hand_r ) );
    CHECK( equipment_layout::is_plausible_drop_destination( gloves, region_id::hand_l ) );
    CHECK( equipment_layout::is_plausible_drop_destination( gloves, region_id::hand_r ) );
}

TEST_CASE( "Equipment_layout_sleeves_accept_arm_drop", "[equipment_layout][doll_review]" )
{
    const item shirt( itype_id( "longshirt" ) );
    REQUIRE( shirt.covers( body_part_arm_l ) );
    CHECK( equipment_layout::is_plausible_drop_destination( shirt, region_id::arm_l ) );
}

TEST_CASE( "Equipment_layout_coverless_accessories_and_waist_belt", "[equipment_layout][doll_review]" )
{
    const auto plugs = classify_item( item( itype_id( "ear_plugs" ) ) );
    CHECK( ( plugs.equip_destination.id == region_id::ear_l ||
             plugs.equip_destination.id == region_id::ear_r ) );
    CHECK( classify_item( item( itype_id( "gold_necklace" ) ) ).equip_destination.id ==
           region_id::neck );
    CHECK( classify_item( item( itype_id( "judo_belt_black" ) ) ).equip_destination.id ==
           region_id::waist );
}

TEST_CASE( "Equipment_layout_backpack_not_weapon_or_waist_target", "[equipment_layout][doll_review]" )
{
    const item pack( itype_id( "backpack" ) );
    CHECK_FALSE( equipment_layout::is_plausible_drop_destination( pack, region_id::waist ) );
    CHECK_FALSE( equipment_layout::is_plausible_drop_destination( pack, region_id::back_weapon ) );
    CHECK( equipment_layout::is_plausible_drop_destination( pack, region_id::back_bag ) );
}

TEST_CASE( "Equipment_layout_wristwatch_right_wrist", "[equipment_layout][doll_review]" )
{
    const item watch( itype_id( "wristwatch" ) );
    REQUIRE( watch.is_sided() );
    CHECK( equipment_layout::is_plausible_drop_destination( watch, region_id::wrist_r ) );
    CHECK( equipment_layout::is_plausible_drop_destination( watch, region_id::wrist_l ) );
}

TEST_CASE( "Equipment_layout_visible_occupants_shirt_and_helmet", "[equipment_layout][doll_review]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    REQUIRE( you.wear_item( item( itype_id( "longshirt" ) ), false ).has_value() );
    // Arm-local layer should not hide the linked shirt from the arm list.
    REQUIRE( you.wear_item( item( itype_id( "arm_warmers" ) ), false ).has_value() );

    // Shirt remains visible on arm via linked coverage even if an arm-local
    // item is present (or absent).
    {
        const auto mapped = map_character( you );
        const auto arm_items = equipment_layout::items_visible_on( mapped, region_id::arm_l );
        bool saw_shirt = false;
        for( const item_location &loc : arm_items ) {
            if( loc && loc->typeId() == itype_id( "longshirt" ) ) {
                saw_shirt = true;
            }
        }
        CHECK( saw_shirt );
    }

    REQUIRE( you.wear_item( item( itype_id( "helmet_army" ) ), false ).has_value() );
    {
        const auto mapped = map_character( you );
        const auto ear_items = equipment_layout::items_visible_on( mapped, region_id::ear_l );
        for( const item_location &loc : ear_items ) {
            REQUIRE( loc );
            CHECK( loc->typeId() != itype_id( "helmet_army" ) );
        }
    }
    REQUIRE( you.wear_item( item( itype_id( "powered_earmuffs" ) ), false ).has_value() );
    {
        const auto mapped = map_character( you );
        const auto ear_items = equipment_layout::items_visible_on( mapped, region_id::ear_l );
        bool saw_muffs = false;
        for( const item_location &loc : ear_items ) {
            if( loc && loc->typeId() == itype_id( "powered_earmuffs" ) ) {
                saw_muffs = true;
            }
            CHECK_FALSE( ( loc && loc->typeId() == itype_id( "helmet_army" ) ) );
        }
        CHECK( saw_muffs );
    }
}

TEST_CASE( "Equipment_layout_holder_store_regions_and_shoulder", "[equipment_layout][doll_review]" )
{
    // Shoulder holster attaches to torso, not default waist.
    const auto shoulder = classify_item( item( itype_id( "shoulder_holster" ) ) );
    CHECK( shoulder.role == equipment_role::holder );
    CHECK( shoulder.equip_destination.id == region_id::torso );

    clear_avatar();
    avatar &you = get_avatar();
    REQUIRE( you.wear_item( item( itype_id( "bscabbard" ) ), false ).has_value() );
    const auto mapped = map_character( you );
    bool saw_back = false;
    for( const auto &entry : mapped.items ) {
        if( entry.location && entry.location->typeId() == itype_id( "bscabbard" ) ) {
            CHECK( entry.equip_destination.id == region_id::back_weapon );
            saw_back = true;
        }
    }
    CHECK( saw_back );

    // Non-armor weapon is a plausible content drop on back_weapon / waist.
    const item sword( itype_id( "longsword" ) );
    CHECK( equipment_layout::is_plausible_drop_destination( sword, region_id::back_weapon ) );
    CHECK( equipment_layout::is_plausible_drop_destination( sword, region_id::waist ) );
}
