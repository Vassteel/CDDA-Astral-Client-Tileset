#include "equipment_layout.h"
#include <optional>

#include <algorithm>
#include <set>
#include <unordered_set>

#include "avatar.h"
#include "body_part_set.h"
#include "character.h"
#include "flag.h"
#include "item.h"
#include "item_pocket.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "translations.h"
#include "type_id.h"

namespace equipment_layout
{
namespace
{

const std::vector<region_info> &catalog_storage()
{
    static const std::vector<region_info> catalog = {
        { region_id::head, region_side::none, "Head", true, false },
        { region_id::ear_l, region_side::left, "Left ear", false, true },
        { region_id::ear_r, region_side::right, "Right ear", false, true },
        { region_id::forehead, region_side::none, "Forehead", false, true },
        { region_id::eyes, region_side::none, "Eyes", false, true },
        { region_id::face, region_side::none, "Face", false, true },
        { region_id::neck, region_side::none, "Neck", true, true },
        { region_id::torso, region_side::none, "Torso", true, false },
        { region_id::arm_l, region_side::left, "Left arm", true, false },
        { region_id::arm_r, region_side::right, "Right arm", true, false },
        { region_id::wrist_l, region_side::left, "Left wrist", false, true },
        { region_id::wrist_r, region_side::right, "Right wrist", false, true },
        { region_id::hand_l, region_side::left, "Left hand", true, false },
        { region_id::hand_r, region_side::right, "Right hand", true, false },
        { region_id::rings_l, region_side::left, "Left rings", false, true },
        { region_id::rings_r, region_side::right, "Right rings", false, true },
        { region_id::waist, region_side::none, "Waist", false, false },
        { region_id::pants, region_side::both, "Pants", true, false },
        { region_id::leg_lower_l, region_side::left, "Left lower leg", true, false },
        { region_id::leg_lower_r, region_side::right, "Right lower leg", true, false },
        { region_id::foot_l, region_side::left, "Left foot", true, false },
        { region_id::foot_r, region_side::right, "Right foot", true, false },
        { region_id::back_bag, region_side::none, "Back (bag)", false, false },
        { region_id::back_weapon, region_side::none, "Back (weapon)", false, false },
        { region_id::main_hand, region_side::right, "Main hand", false, false },
        { region_id::off_hand, region_side::left, "Off hand", false, false },
        { region_id::fallback, region_side::none, "Other equipment", false, false },
    };
    return catalog;
}

bool region_ref_equal( const region_ref &a, const region_ref &b )
{
    return a.id == b.id && a.layer == b.layer && a.position_index == b.position_index;
}

void push_unique_region( std::vector<region_ref> &out, region_ref ref )
{
    for( const region_ref &existing : out ) {
        if( region_ref_equal( existing, ref ) ) {
            return;
        }
    }
    out.push_back( ref );
}

display_layer layer_from_native( layer_level layer )
{
    switch( layer ) {
        case layer_level::SKINTIGHT:
            return display_layer::skin;
        case layer_level::NORMAL:
            return display_layer::middle;
        case layer_level::OUTER:
            return display_layer::outer;
        case layer_level::PERSONAL:
        case layer_level::WAIST:
        case layer_level::BELTED:
        case layer_level::AURA:
            return display_layer::uncommon;
        case layer_level::NUM_LAYER_LEVELS:
            break;
    }
    return display_layer::uncommon;
}

layer_level native_from_display( display_layer layer )
{
    switch( layer ) {
        case display_layer::skin:
            return layer_level::SKINTIGHT;
        case display_layer::middle:
            return layer_level::NORMAL;
        case display_layer::outer:
            return layer_level::OUTER;
        case display_layer::uncommon:
        case display_layer::any:
            break;
    }
    return layer_level::NORMAL;
}

display_layer primary_display_layer( const item &it, const bodypart_id &bp )
{
    if( bp != bodypart_str_id::NULL_ID().id() ) {
        const std::vector<layer_level> layers = it.get_layer( bp );
        if( !layers.empty() ) {
            return layer_from_native( layers.front() );
        }
    }
    const std::vector<layer_level> layers = it.get_layer();
    if( !layers.empty() ) {
        return layer_from_native( layers.front() );
    }
    if( it.has_flag( flag_SKINTIGHT ) ) {
        return display_layer::skin;
    }
    if( it.has_flag( flag_OUTER ) ) {
        return display_layer::outer;
    }
    if( it.has_flag( flag_BELTED ) || it.has_flag( flag_WAIST ) ||
        it.has_flag( flag_PERSONAL ) || it.has_flag( flag_AURA ) ) {
        return display_layer::uncommon;
    }
    return display_layer::middle;
}

// Homologous mutation/bionic subparts share suffixes with the human defaults
// (e.g. head_bionic_basic_ear_l, hand_slime_wrist_r). Match by semantic token,
// never by translated item names.
bool id_has_token( const std::string &id, const std::string &token )
{
    return id.find( token ) != std::string::npos;
}

bool is_ear_sub( const sub_bodypart_id &sbp )
{
    const std::string id = sbp.id().str();
    return id_has_token( id, "_ear_l" ) || id_has_token( id, "_ear_r" ) ||
           id == "head_ear_l" || id == "head_ear_r";
}

bool is_forehead_sub( const sub_bodypart_id &sbp )
{
    return id_has_token( sbp.id().str(), "_forehead" ) ||
           sbp.id().str() == "head_forehead";
}

bool is_wrist_sub( const sub_bodypart_id &sbp )
{
    return id_has_token( sbp.id().str(), "_wrist_" ) ||
           id_has_token( sbp.id().str(), "_wrist_l" ) ||
           id_has_token( sbp.id().str(), "_wrist_r" ) ||
           sbp.id().str() == "hand_wrist_l" || sbp.id().str() == "hand_wrist_r";
}

bool is_finger_sub( const sub_bodypart_id &sbp )
{
    const std::string id = sbp.id().str();
    return id_has_token( id, "_fingers_" ) || id_has_token( id, "_fingers_l" ) ||
           id_has_token( id, "_fingers_r" ) ||
           id == "hand_fingers_l" || id == "hand_fingers_r";
}

bool is_neck_sub( const sub_bodypart_id &sbp )
{
    const std::string id = sbp.id().str();
    return id_has_token( id, "_neck" ) || id_has_token( id, "_throat" ) ||
           id == "torso_neck" || id == "head_throat";
}

bool is_upper_leg_sub( const sub_bodypart_id &sbp )
{
    const std::string id = sbp.id().str();
    if( id.find( "leg" ) == std::string::npos ) {
        return false;
    }
    return id_has_token( id, "_hip_" ) || id_has_token( id, "_hip_l" ) ||
           id_has_token( id, "_hip_r" ) || id_has_token( id, "_upper_" ) ||
           id_has_token( id, "_upper_l" ) || id_has_token( id, "_upper_r" ) ||
           id_has_token( id, "_draped_" );
}

bool is_lower_leg_sub( const sub_bodypart_id &sbp )
{
    const std::string id = sbp.id().str();
    if( id.find( "leg" ) == std::string::npos ) {
        return false;
    }
    return id_has_token( id, "_lower_" ) || id_has_token( id, "_lower_l" ) ||
           id_has_token( id, "_lower_r" ) || id_has_token( id, "_knee_" ) ||
           id_has_token( id, "_knee_l" ) || id_has_token( id, "_knee_r" );
}

bool is_back_hang_sub( const sub_bodypart_id &sbp )
{
    return id_has_token( sbp.id().str(), "hanging_back" );
}

bool is_waist_sub( const sub_bodypart_id &sbp )
{
    return id_has_token( sbp.id().str(), "_waist" ) ||
           sbp.id().str() == "torso_waist";
}

bool is_leftish_sub( const std::string &id )
{
    return id.size() >= 2 && ( id.compare( id.size() - 2, 2, "_l" ) == 0 ||
                               id.find( "_l_" ) != std::string::npos );
}

bool is_rightish_sub( const std::string &id )
{
    return id.size() >= 2 && ( id.compare( id.size() - 2, 2, "_r" ) == 0 ||
                               id.find( "_r_" ) != std::string::npos );
}

bool is_shield_offhand( const item &it )
{
    return it.has_flag( flag_BLOCK_WHILE_WORN ) && it.has_flag( flag_RESTRICT_HANDS );
}

std::vector<sub_bodypart_id> covered_subs( const item &it )
{
    return it.get_covered_sub_body_parts();
}

body_part_set covered_parts( const item &it )
{
    return it.get_covered_body_parts();
}

bool only_ear_head_coverage( const item &it )
{
    const std::vector<sub_bodypart_id> subs = covered_subs( it );
    if( subs.empty() ) {
        return false;
    }
    bool any_ear = false;
    for( const sub_bodypart_id &sbp : subs ) {
        if( is_ear_sub( sbp ) ) {
            any_ear = true;
            continue;
        }
        // Any non-ear subpart means this is not an ear-only accessory
        // (helmets cover crown/forehead/etc.).
        return false;
    }
    const body_part_set parts = covered_parts( it );
    for( const bodypart_str_id &bp : parts ) {
        const std::string id = bp.str();
        // Ignore homologous head bodyparts (head_bionic_basic, etc.).
        if( id == "eyes" || id == "mouth" || id.find( "eyes" ) != std::string::npos ||
            id.find( "mouth" ) != std::string::npos ) {
            return false;
        }
        if( id != "head" && id.rfind( "head", 0 ) != 0 ) {
            return false;
        }
    }
    return any_ear;
}

bool only_forehead_head_coverage( const item &it )
{
    const std::vector<sub_bodypart_id> subs = covered_subs( it );
    if( subs.empty() ) {
        return false;
    }
    bool any_forehead = false;
    for( const sub_bodypart_id &sbp : subs ) {
        if( is_forehead_sub( sbp ) ) {
            any_forehead = true;
            continue;
        }
        // Forehead accessories must not also cover crown/nape/ears/etc.
        return false;
    }
    const body_part_set parts = covered_parts( it );
    for( const bodypart_str_id &bp : parts ) {
        const std::string id = bp.str();
        if( id != "head" && id.rfind( "head", 0 ) != 0 ) {
            return false;
        }
    }
    return any_forehead;
}

bool only_wrist_coverage( const item &it )
{
    const std::vector<sub_bodypart_id> subs = covered_subs( it );
    if( subs.empty() ) {
        return false;
    }
    bool any = false;
    for( const sub_bodypart_id &sbp : subs ) {
        if( !is_wrist_sub( sbp ) ) {
            return false;
        }
        any = true;
    }
    return any;
}

bool only_finger_coverage( const item &it )
{
    const std::vector<sub_bodypart_id> subs = covered_subs( it );
    if( subs.empty() ) {
        return false;
    }
    bool any = false;
    for( const sub_bodypart_id &sbp : subs ) {
        if( !is_finger_sub( sbp ) ) {
            return false;
        }
        any = true;
    }
    return any;
}

bool only_neck_coverage( const item &it )
{
    const std::vector<sub_bodypart_id> subs = covered_subs( it );
    if( subs.empty() ) {
        return false;
    }
    bool any = false;
    for( const sub_bodypart_id &sbp : subs ) {
        if( !is_neck_sub( sbp ) ) {
            return false;
        }
        any = true;
    }
    return any;
}

bool covers_upper_leg( const item &it, side s )
{
    const std::vector<sub_bodypart_id> subs = it.get_covered_sub_body_parts( s );
    bool has_specific = false;
    for( const sub_bodypart_id &sbp : subs ) {
        if( is_upper_leg_sub( sbp ) ) {
            return true;
        }
        if( is_lower_leg_sub( sbp ) ) {
            has_specific = true;
        }
    }
    // Full-leg garments without sub-part data cover upper legs.
    const bodypart_str_id bp = ( s == side::LEFT ) ? body_part_leg_l : body_part_leg_r;
    if( it.covers( bp.id() ) && !has_specific && subs.empty() ) {
        return true;
    }
    // If it has any leg sub coverage including upper, already returned.
    // If it covers the leg body part and has lower-only subs, upper is false.
    if( it.covers( bp.id() ) && subs.empty() ) {
        return true;
    }
    // Mixed: specifically covers upper
    for( const sub_bodypart_id &sbp : covered_subs( it ) ) {
        if( is_upper_leg_sub( sbp ) ) {
            const std::string id = sbp.id().str();
            if( s == side::LEFT && id.back() == 'l' ) {
                return true;
            }
            if( s == side::RIGHT && id.back() == 'r' ) {
                return true;
            }
            if( s == side::BOTH ) {
                return true;
            }
        }
    }
    // Jeans-style: covers leg with no specifically_covers → both upper and lower
    if( it.covers( bp.id() ) ) {
        bool any_leg_sub = false;
        for( const sub_bodypart_id &sbp : covered_subs( it ) ) {
            const std::string id = sbp.id().str();
            if( id.rfind( "leg_", 0 ) == 0 ) {
                any_leg_sub = true;
                break;
            }
        }
        if( !any_leg_sub ) {
            return true;
        }
    }
    return false;
}

bool covers_lower_leg( const item &it, side s )
{
    const bodypart_str_id bp = ( s == side::LEFT ) ? body_part_leg_l : body_part_leg_r;
    for( const sub_bodypart_id &sbp : covered_subs( it ) ) {
        if( !is_lower_leg_sub( sbp ) ) {
            continue;
        }
        const std::string id = sbp.id().str();
        if( s == side::BOTH ) {
            return true;
        }
        if( s == side::LEFT && id.back() == 'l' ) {
            return true;
        }
        if( s == side::RIGHT && id.back() == 'r' ) {
            return true;
        }
    }
    // Full-leg cover without subparts includes lower legs.
    if( it.covers( bp.id() ) ) {
        bool any_leg_sub = false;
        bool any_lower = false;
        bool any_upper = false;
        for( const sub_bodypart_id &sbp : covered_subs( it ) ) {
            const std::string id = sbp.id().str();
            if( id.rfind( "leg_", 0 ) == 0 ) {
                any_leg_sub = true;
            }
            if( is_lower_leg_sub( sbp ) ) {
                any_lower = true;
            }
            if( is_upper_leg_sub( sbp ) ) {
                any_upper = true;
            }
        }
        if( !any_leg_sub ) {
            return true;
        }
        if( any_lower ) {
            return true;
        }
        // Shorts: upper only → not lower
        if( any_upper && !any_lower ) {
            return false;
        }
    }
    return false;
}

bool covers_arm_side( const item &it, side s )
{
    const bodypart_str_id bp = ( s == side::LEFT ) ? body_part_arm_l : body_part_arm_r;
    return it.covers( bp.id() );
}

void add_layer_coverage( std::vector<region_ref> &covered, region_id id,
                         const item &it, const bodypart_id &bp )
{
    region_ref ref;
    ref.id = id;
    ref.layer = primary_display_layer( it, bp );
    push_unique_region( covered, ref );
}

void append_clothing_coverage( const item &it, std::vector<region_ref> &covered )
{
    const body_part_set parts = covered_parts( it );

    if( it.covers( body_part_head ) ) {
        // Ear-only and forehead-only accessories still list their accessory
        // regions; helmets that also cover ears list head (+ incidental ears
        // as coverage, not equip destination).
        if( only_ear_head_coverage( it ) ) {
            // handled as accessory destinations
        } else if( only_forehead_head_coverage( it ) ) {
            // forehead accessory
        } else {
            add_layer_coverage( covered, region_id::head, it, body_part_head );
            // Incidental ear coverage from helmets appears as covered regions
            // so the UI can highlight, but equip destination stays head.
            for( const sub_bodypart_id &sbp : covered_subs( it ) ) {
                if( is_ear_sub( sbp ) ) {
                    const std::string id = sbp.id().str();
                    if( is_leftish_sub( id ) ) {
                        push_unique_region( covered, { region_id::ear_l, display_layer::any, 0 } );
                    }
                    if( is_rightish_sub( id ) ) {
                        push_unique_region( covered, { region_id::ear_r, display_layer::any, 0 } );
                    }
                } else if( is_forehead_sub( sbp ) ) {
                    push_unique_region( covered, { region_id::forehead, display_layer::any, 0 } );
                }
            }
        }
    }
    if( it.covers( body_part_eyes ) ) {
        push_unique_region( covered, { region_id::eyes, display_layer::any, 0 } );
    }
    if( it.covers( body_part_mouth ) ) {
        push_unique_region( covered, { region_id::face, display_layer::any, 0 } );
    }
    if( only_neck_coverage( it ) || [&]() {
    for( const sub_bodypart_id &sbp : covered_subs( it ) ) {
            if( is_neck_sub( sbp ) ) {
                return true;
            }
        }
        return false;
    }() ) {
        add_layer_coverage( covered, region_id::neck, it, body_part_torso );
    }
    if( it.covers( body_part_torso ) && !only_neck_coverage( it ) ) {
        bool back_only = false;
        bool waist_only = false;
        const std::vector<sub_bodypart_id> subs = covered_subs( it );
        if( !subs.empty() ) {
            bool any_non_special = false;
            bool any_back = false;
            bool any_waist = false;
            for( const sub_bodypart_id &sbp : subs ) {
                if( is_back_hang_sub( sbp ) ) {
                    any_back = true;
                } else if( is_waist_sub( sbp ) ) {
                    any_waist = true;
                } else if( is_neck_sub( sbp ) ) {
                    // neck handled above
                } else {
                    any_non_special = true;
                }
            }
            back_only = any_back && !any_non_special && !any_waist;
            waist_only = any_waist && !any_non_special && !any_back;
        }
        if( waist_only ) {
            // Waist-only clothing (judo belts, etc.) equips at the waist region,
            // not as a torso garment and not only when a holster action exists.
            push_unique_region( covered, {
                region_id::waist,
                primary_display_layer( it, body_part_torso ),
                0
            } );
        } else if( !back_only ) {
            add_layer_coverage( covered, region_id::torso, it, body_part_torso );
        }
    }
    if( covers_arm_side( it, side::LEFT ) ) {
        add_layer_coverage( covered, region_id::arm_l, it, body_part_arm_l );
    }
    if( covers_arm_side( it, side::RIGHT ) ) {
        add_layer_coverage( covered, region_id::arm_r, it, body_part_arm_r );
    }
    if( only_wrist_coverage( it ) ) {
        for( const sub_bodypart_id &sbp : covered_subs( it ) ) {
            if( !is_wrist_sub( sbp ) ) {
                continue;
            }
            const std::string id = sbp.id().str();
            if( is_leftish_sub( id ) ) {
                push_unique_region( covered, { region_id::wrist_l, display_layer::any, 0 } );
            }
            if( is_rightish_sub( id ) ) {
                push_unique_region( covered, { region_id::wrist_r, display_layer::any, 0 } );
            }
        }
    } else {
        if( it.covers( body_part_hand_l ) && !only_finger_coverage( it ) ) {
            add_layer_coverage( covered, region_id::hand_l, it, body_part_hand_l );
        }
        if( it.covers( body_part_hand_r ) && !only_finger_coverage( it ) ) {
            add_layer_coverage( covered, region_id::hand_r, it, body_part_hand_r );
        }
    }
    if( only_finger_coverage( it ) ) {
        for( const sub_bodypart_id &sbp : covered_subs( it ) ) {
            if( !is_finger_sub( sbp ) ) {
                continue;
            }
            const std::string id = sbp.id().str();
            if( is_leftish_sub( id ) ) {
                push_unique_region( covered, { region_id::rings_l, display_layer::any, 0 } );
            }
            if( is_rightish_sub( id ) ) {
                push_unique_region( covered, { region_id::rings_r, display_layer::any, 0 } );
            }
        }
    }
    const bool upper_l = covers_upper_leg( it, side::LEFT );
    const bool upper_r = covers_upper_leg( it, side::RIGHT );
    if( upper_l || upper_r ) {
        add_layer_coverage( covered, region_id::pants, it,
                            upper_l ? body_part_leg_l.id() : body_part_leg_r.id() );
    }
    if( covers_lower_leg( it, side::LEFT ) ) {
        add_layer_coverage( covered, region_id::leg_lower_l, it, body_part_leg_l );
    }
    if( covers_lower_leg( it, side::RIGHT ) ) {
        add_layer_coverage( covered, region_id::leg_lower_r, it, body_part_leg_r );
    }
    if( it.covers( body_part_foot_l ) ) {
        add_layer_coverage( covered, region_id::foot_l, it, body_part_foot_l );
    }
    if( it.covers( body_part_foot_r ) ) {
        add_layer_coverage( covered, region_id::foot_r, it, body_part_foot_r );
    }

    ( void )parts;
}

region_ref choose_equip_destination( const item &it, const std::vector<region_ref> &covered,
                                     storage_slot storage, equipment_role &role )
{
    region_ref dest;

    if( is_shield_offhand( it ) ) {
        role = equipment_role::held;
        dest.id = region_id::off_hand;
        dest.layer = display_layer::any;
        return dest;
    }

    if( storage == storage_slot::back ) {
        role = equipment_role::bag;
        dest.id = region_id::back_bag;
        dest.layer = display_layer::uncommon;
        return dest;
    }
    if( storage == storage_slot::scabbard || storage == storage_slot::sheath ||
        storage == storage_slot::holster ) {
        role = equipment_role::holder;
        // Resolve the actual attachment region: hanging-back → back weapon,
        // waist-only → waist, otherwise torso (chest/shoulder rigs) instead of
        // defaulting every holster to Waist.
        bool on_back = false;
        bool on_waist = false;
        bool on_torso = false;
        for( const sub_bodypart_id &sbp : covered_subs( it ) ) {
            if( is_back_hang_sub( sbp ) ) {
                on_back = true;
            } else if( is_waist_sub( sbp ) ) {
                on_waist = true;
            } else if( !is_neck_sub( sbp ) ) {
                on_torso = true;
            }
        }
        if( on_back ) {
            dest.id = region_id::back_weapon;
        } else if( on_waist && !on_torso ) {
            dest.id = region_id::waist;
        } else if( on_torso ) {
            dest.id = region_id::torso;
        } else {
            dest.id = region_id::waist;
        }
        dest.layer = display_layer::uncommon;
        return dest;
    }

    if( only_ear_head_coverage( it ) ) {
        role = equipment_role::accessory;
        // Paired ear gear links both ears; sided gear uses the worn side.
        bool left = false;
        bool right = false;
        for( const sub_bodypart_id &sbp : covered_subs( it ) ) {
            if( !is_ear_sub( sbp ) ) {
                continue;
            }
            const std::string id = sbp.id().str();
            if( is_leftish_sub( id ) ) {
                left = true;
            }
            if( is_rightish_sub( id ) ) {
                right = true;
            }
        }
        if( left && !right ) {
            dest.id = region_id::ear_l;
        } else if( right && !left ) {
            dest.id = region_id::ear_r;
        } else {
            // Paired: equip destination is left ear; right is linked coverage.
            dest.id = region_id::ear_l;
        }
        dest.layer = display_layer::any;
        return dest;
    }

    if( only_forehead_head_coverage( it ) ) {
        role = equipment_role::accessory;
        dest.id = region_id::forehead;
        dest.layer = display_layer::any;
        return dest;
    }

    if( it.covers( body_part_eyes ) ) {
        const body_part_set parts = covered_parts( it );
        bool only_eyes = true;
        for( const bodypart_str_id &bp : parts ) {
            if( bp != body_part_eyes ) {
                only_eyes = false;
                break;
            }
        }
        if( only_eyes && parts.any() ) {
            role = equipment_role::accessory;
            dest.id = region_id::eyes;
            dest.layer = display_layer::any;
            return dest;
        }
    }

    if( it.covers( body_part_mouth ) && !it.covers( body_part_head ) &&
        !it.covers( body_part_torso ) ) {
        role = equipment_role::accessory;
        dest.id = region_id::face;
        dest.layer = display_layer::any;
        return dest;
    }

    if( only_neck_coverage( it ) ) {
        role = equipment_role::accessory;
        dest.id = region_id::neck;
        dest.layer = primary_display_layer( it, body_part_torso );
        return dest;
    }

    if( only_wrist_coverage( it ) ) {
        role = equipment_role::accessory;
        bool left = false;
        bool right = false;
        for( const sub_bodypart_id &sbp : covered_subs( it ) ) {
            if( !is_wrist_sub( sbp ) ) {
                continue;
            }
            const std::string id = sbp.id().str();
            if( is_leftish_sub( id ) ) {
                left = true;
            }
            if( is_rightish_sub( id ) ) {
                right = true;
            }
        }
        // Sided jewelry: respect item side when BOTH isn't forced.
        if( it.is_sided() && it.get_side() == side::LEFT ) {
            dest.id = region_id::wrist_l;
        } else if( it.is_sided() && it.get_side() == side::RIGHT ) {
            dest.id = region_id::wrist_r;
        } else if( left && !right ) {
            dest.id = region_id::wrist_l;
        } else if( right && !left ) {
            dest.id = region_id::wrist_r;
        } else {
            dest.id = region_id::wrist_l;
        }
        dest.layer = display_layer::any;
        return dest;
    }

    if( only_finger_coverage( it ) ) {
        role = equipment_role::accessory;
        if( it.is_sided() && it.get_side() == side::RIGHT ) {
            dest.id = region_id::rings_r;
        } else {
            dest.id = region_id::rings_l;
        }
        dest.layer = display_layer::any;
        return dest;
    }

    // Prefer a primary clothing destination from covered regions.
    static const region_id preference[] = {
        region_id::waist, region_id::torso, region_id::head, region_id::pants,
        region_id::arm_l, region_id::arm_r,
        region_id::hand_l, region_id::hand_r,
        region_id::leg_lower_l, region_id::leg_lower_r,
        region_id::foot_l, region_id::foot_r,
        region_id::neck, region_id::face, region_id::eyes,
        region_id::fallback
    };
    role = equipment_role::clothing;
    for( region_id want : preference ) {
        for( const region_ref &ref : covered ) {
            if( ref.id == want ) {
                return ref;
            }
        }
    }

    role = equipment_role::fallback;
    dest.id = region_id::fallback;
    dest.layer = display_layer::any;
    return dest;
}

} // namespace

const std::vector<region_info> &region_catalog()
{
    return catalog_storage();
}

const region_info &info_for( region_id id )
{
    const std::vector<region_info> &cat = catalog_storage();
    for( const region_info &info : cat ) {
        if( info.id == id ) {
            return info;
        }
    }
    return cat.back();
}

std::string region_label( region_id id )
{
    return _( info_for( id ).label );
}

display_layer to_display_layer( layer_level layer )
{
    return layer_from_native( layer );
}

layer_level to_native_layer( display_layer layer )
{
    return native_from_display( layer );
}

const char *display_layer_label( display_layer layer )
{
    switch( layer ) {
        case display_layer::skin:
            return _( "Skin" );
        case display_layer::middle:
            return _( "Middle" );
        case display_layer::outer:
            return _( "Outer" );
        case display_layer::uncommon:
            return _( "Other layers" );
        case display_layer::any:
            return _( "Any" );
    }
    return "";
}

storage_slot storage_slot_for( const item &it )
{
    if( !it.is_armor() ) {
        return storage_slot::none;
    }
    // Use pocket capabilities, not translated names or a list of item IDs.
    // A backpack with a bottle holster remains a backpack.
    if( it.type->get_use( "holster" ) ) {
        bool sheath = false;
        for( const item_pocket *pocket : it.get_container_pockets() ) {
            for( const flag_id &flag : pocket->get_pocket_data()->get_flag_restrictions() ) {
                if( flag.str() == "SHEATH_SWORD" ) {
                    return storage_slot::scabbard;
                }
                sheath = sheath || flag.str() == "SHEATH_KNIFE" ||
                         flag.str() == "SHEATH_AXE" || flag.str() == "SHEATH_SPEAR";
            }
        }
        return sheath ? storage_slot::sheath : storage_slot::holster;
    }
    if( it.has_layer( { layer_level::BELTED }, body_part_torso ) ) {
        for( const sub_bodypart_id &bp : it.get_covered_sub_body_parts() ) {
            if( bp.id().str() == "torso_hanging_back" ) {
                return storage_slot::back;
            }
        }
    }
    return storage_slot::none;
}

item_profile classify_item( const item &it )
{
    item_profile profile;
    profile.storage = storage_slot_for( it );

    if( !it.is_armor() && !is_shield_offhand( it ) ) {
        profile.role = equipment_role::fallback;
        profile.equip_destination.id = region_id::fallback;
        profile.uses_fallback = true;
        return profile;
    }

    append_clothing_coverage( it, profile.covered_regions );

    // Ensure accessory-only destinations appear in covered list.
    if( only_ear_head_coverage( it ) ) {
        for( const sub_bodypart_id &sbp : covered_subs( it ) ) {
            if( !is_ear_sub( sbp ) ) {
                continue;
            }
            const std::string id = sbp.id().str();
            if( is_leftish_sub( id ) ) {
                push_unique_region( profile.covered_regions,
                { region_id::ear_l, display_layer::any, 0 } );
            }
            if( is_rightish_sub( id ) ) {
                push_unique_region( profile.covered_regions,
                { region_id::ear_r, display_layer::any, 0 } );
            }
        }
    }
    if( only_forehead_head_coverage( it ) ) {
        push_unique_region( profile.covered_regions,
        { region_id::forehead, display_layer::any, 0 } );
    }

    if( profile.storage == storage_slot::back ) {
        push_unique_region( profile.covered_regions,
        { region_id::back_bag, display_layer::uncommon, 0 } );
    } else if( profile.storage == storage_slot::scabbard ||
               profile.storage == storage_slot::sheath ||
               profile.storage == storage_slot::holster ) {
        bool on_back = false;
        bool on_waist = false;
        bool on_torso = false;
        for( const sub_bodypart_id &sbp : covered_subs( it ) ) {
            if( is_back_hang_sub( sbp ) ) {
                on_back = true;
            } else if( is_waist_sub( sbp ) ) {
                on_waist = true;
            } else if( !is_neck_sub( sbp ) ) {
                on_torso = true;
            }
        }
        region_id holder_dest = region_id::waist;
        if( on_back ) {
            holder_dest = region_id::back_weapon;
        } else if( on_waist && !on_torso ) {
            holder_dest = region_id::waist;
        } else if( on_torso ) {
            holder_dest = region_id::torso;
        }
        push_unique_region( profile.covered_regions, {
            holder_dest, display_layer::uncommon, 0
        } );
    }

    profile.equip_destination = choose_equip_destination( it, profile.covered_regions,
                                profile.storage, profile.role );
    profile.uses_fallback = profile.equip_destination.id == region_id::fallback ||
                            profile.role == equipment_role::fallback;

    // Narrow coverless-accessory overrides (itype id / semantic flags only —
    // never translated display names). Ordinary ear plugs and neck jewelry
    // lack armor coverage in source data but are required doll destinations.
    if( profile.covered_regions.empty() && it.is_armor() ) {
        const std::string tid = it.typeId().str();
        std::optional<region_id> override_dest;
        if( it.has_flag( flag_DEAF ) ) {
            // Coverless hearing protection (ear_plugs and similar).
            override_dest = region_id::ear_l;
        } else if( tid.find( "necklace" ) != std::string::npos ||
                   tid.find( "pendant" ) != std::string::npos ) {
            override_dest = region_id::neck;
        }
        if( override_dest ) {
            profile.role = equipment_role::accessory;
            profile.equip_destination.id = *override_dest;
            profile.equip_destination.layer = display_layer::any;
            profile.uses_fallback = false;
            if( *override_dest == region_id::ear_l ) {
                push_unique_region( profile.covered_regions,
                { region_id::ear_l, display_layer::any, 0 } );
                push_unique_region( profile.covered_regions,
                { region_id::ear_r, display_layer::any, 0 } );
            } else {
                push_unique_region( profile.covered_regions,
                { *override_dest, display_layer::any, 0 } );
            }
        } else {
            // Genuinely unknown / unmapped mod equipment stays in fallback.
            profile.role = equipment_role::fallback;
            profile.equip_destination.id = region_id::fallback;
            profile.uses_fallback = true;
            push_unique_region( profile.covered_regions,
            { region_id::fallback, display_layer::any, 0 } );
        }
    }

    return profile;
}

character_map map_character( Character &you )
{
    character_map result;
    std::unordered_set<const item *> seen;

    const auto add_worn = [&]( item_location loc ) {
        if( !loc || !loc.get_item() ) {
            return;
        }
        const item *ptr = loc.get_item();
        if( !seen.insert( ptr ).second ) {
            return;
        }
        const item_profile profile = classify_item( *loc );
        mapped_item entry;
        entry.location = loc;
        entry.equip_destination = profile.equip_destination;
        entry.covered_regions = profile.covered_regions;
        entry.role = profile.role;
        entry.storage = profile.storage;
        entry.uses_fallback = profile.uses_fallback;
        result.items.push_back( entry );
        if( entry.uses_fallback ) {
            result.unmapped_fallback.push_back( loc );
        }

        // Resolve contents inside holders to the exact holder + content.
        if( entry.role == equipment_role::holder || entry.role == equipment_role::bag ) {
            item *holder_ptr = loc.get_item();
            if( holder_ptr == nullptr ) {
                return;
            }
            for( item *content : holder_ptr->all_items_top( pocket_type::CONTAINER ) ) {
                if( content == nullptr ) {
                    continue;
                }
                item_location content_loc( loc, content );
                if( !seen.insert( content ).second ) {
                    continue;
                }
                mapped_item nested;
                nested.location = content_loc;
                nested.holder_location = loc;
                nested.role = equipment_role::held;
                nested.equip_destination = entry.equip_destination;
                nested.covered_regions = entry.covered_regions;
                nested.storage = storage_slot::none;
                result.items.push_back( nested );
            }
        }
    };

    for( const item_location &loc : you.top_items_loc() ) {
        if( loc && you.is_worn( *loc ) ) {
            add_worn( loc );
        }
    }

    // Main hand via adapter semantics (wielded item).
    item_location wielded = you.get_wielded_item();
    if( wielded && wielded.get_item() && seen.insert( wielded.get_item() ).second ) {
        mapped_item entry;
        entry.location = wielded;
        entry.role = equipment_role::held;
        entry.equip_destination.id = region_id::main_hand;
        entry.covered_regions.push_back( entry.equip_destination );
        if( wielded->is_two_handed( you ) ) {
            entry.covered_regions.push_back( { region_id::off_hand, display_layer::any, 0 } );
        }
        result.items.push_back( entry );
    }

    return result;
}

static bool layer_matches_filter( display_layer value, display_layer filter )
{
    return filter == display_layer::any || value == filter || value == display_layer::any;
}

std::vector<item_location> items_equipping_to( const character_map &map, region_id id,
        display_layer layer )
{
    std::vector<item_location> out;
    for( const mapped_item &entry : map.items ) {
        if( entry.holder_location ) {
            continue; // contents are not equip destinations
        }
        if( entry.equip_destination.id == id &&
            layer_matches_filter( entry.equip_destination.layer, layer ) ) {
            out.push_back( entry.location );
        }
    }
    return out;
}

std::vector<item_location> items_covering( const character_map &map, region_id id,
        display_layer layer )
{
    std::vector<item_location> out;
    std::set<const item *> seen;
    for( const mapped_item &entry : map.items ) {
        if( entry.holder_location ) {
            continue;
        }
        for( const region_ref &ref : entry.covered_regions ) {
            if( ref.id == id && layer_matches_filter( ref.layer, layer ) ) {
                const item *ptr = entry.location.get_item();
                if( ptr && seen.insert( ptr ).second ) {
                    out.push_back( entry.location );
                }
                break;
            }
        }
    }
    return out;
}

std::vector<region_ref> coverage_for( const character_map &map, const item_location &loc )
{
    for( const mapped_item &entry : map.items ) {
        if( entry.location == loc ) {
            return entry.covered_regions;
        }
    }
    if( loc && loc.get_item() ) {
        return classify_item( *loc ).covered_regions;
    }
    return {};
}

bool is_plausible_drop_destination( const item &it, region_id dest, display_layer layer )
{
    const item_profile profile = classify_item( it );
    if( profile.equip_destination.id == dest &&
        layer_matches_filter( profile.equip_destination.layer, layer ) ) {
        return true;
    }
    for( const region_ref &ref : profile.covered_regions ) {
        if( ref.id != dest || !layer_matches_filter( ref.layer, layer ) ) {
            continue;
        }
        // Helmets that happen to cover ears are not ear-accessory targets.
        if( dest == region_id::ear_l || dest == region_id::ear_r ) {
            return only_ear_head_coverage( it ) ||
                   profile.equip_destination.id == region_id::ear_l ||
                   profile.equip_destination.id == region_id::ear_r;
        }
        if( dest == region_id::forehead ) {
            return only_forehead_head_coverage( it ) ||
                   profile.equip_destination.id == region_id::forehead;
        }
        // Accessory destinations: allow paired/sided coverage (wrists, rings).
        if( info_for( dest ).accessory_position ) {
            return profile.role == equipment_role::accessory ||
                   profile.equip_destination.id == dest;
        }
        // Clothing: linked coverage (shirt→arm, gloves→either hand) is a valid
        // drop target. Native wear remains the Apply authority.
        if( profile.role == equipment_role::clothing ||
            profile.role == equipment_role::holder ||
            profile.role == equipment_role::bag ||
            profile.role == equipment_role::strapped_large ) {
            return true;
        }
        return profile.equip_destination.id == dest;
    }
    // Separating "equip a holder/bag" from "insert into a holder":
    // storage class alone must not authorize a different anatomical region.
    if( dest == region_id::waist || dest == region_id::back_weapon ) {
        if( profile.equip_destination.id == dest ) {
            return true;
        }
        // Non-armor content may be stored into an equipped holder here.
        return !it.is_armor();
    }
    if( dest == region_id::back_bag ) {
        return profile.equip_destination.id == dest ||
               profile.storage == storage_slot::back;
    }
    if( dest == region_id::main_hand ) {
        return true;
    }
    if( dest == region_id::off_hand ) {
        return is_shield_offhand( it );
    }
    if( dest == region_id::fallback ) {
        return true;
    }
    return false;
}

std::vector<item_location> items_visible_on( const character_map &map, region_id id,
        display_layer layer )
{
    std::vector<item_location> out;
    std::set<const item *> seen;
    const region_info &info = info_for( id );
    const auto push = [&]( const item_location &loc ) {
        const item *ptr = loc.get_item();
        if( ptr && seen.insert( ptr ).second ) {
            out.push_back( loc );
        }
    };

    for( const item_location &loc : items_equipping_to( map, id, layer ) ) {
        push( loc );
    }

    if( info.accessory_position ) {
        // Accessories only: paired ear gear may equip on one ear and cover both.
        // Incidental helmet ear coverage must not appear as an ear occupant.
        for( const mapped_item &entry : map.items ) {
            if( entry.holder_location || !entry.location ) {
                continue;
            }
            if( entry.role != equipment_role::accessory ) {
                continue;
            }
            for( const region_ref &ref : entry.covered_regions ) {
                if( ref.id == id && layer_matches_filter( ref.layer, layer ) ) {
                    push( entry.location );
                    break;
                }
            }
        }
        return out;
    }

    // Clothing / holder regions: merge linked coverage with direct equipment.
    for( const item_location &loc : items_covering( map, id, layer ) ) {
        // Skip incidental accessory-only highlights that are not clothing.
        bool skip = false;
        for( const mapped_item &entry : map.items ) {
            if( entry.location != loc ) {
                continue;
            }
            if( entry.role == equipment_role::accessory &&
                entry.equip_destination.id != id ) {
                // e.g. do not list a wristwatch as a hand occupant via coverage
                skip = true;
            }
            break;
        }
        if( !skip ) {
            push( loc );
        }
    }
    return out;
}

std::optional<side> side_intent_for( region_id id )
{
    switch( id ) {
        case region_id::ear_l:
        case region_id::arm_l:
        case region_id::wrist_l:
        case region_id::hand_l:
        case region_id::rings_l:
        case region_id::leg_lower_l:
        case region_id::foot_l:
        case region_id::off_hand:
            return side::LEFT;
        case region_id::ear_r:
        case region_id::arm_r:
        case region_id::wrist_r:
        case region_id::hand_r:
        case region_id::rings_r:
        case region_id::leg_lower_r:
        case region_id::foot_r:
        case region_id::main_hand:
            return side::RIGHT;
        default:
            return std::nullopt;
    }
}

std::vector<region_id> visible_regions_for( const Character &you )
{
    std::vector<region_id> out;
    for( const region_info &info : catalog_storage() ) {
        if( info.side == region_side::left || info.side == region_side::right ) {
            // Hand-occupancy slots always show; the adapter reports emptiness.
            if( info.id == region_id::main_hand || info.id == region_id::off_hand ) {
                out.push_back( info.id );
                continue;
            }
            std::optional<bodypart_id> need;
            switch( info.id ) {
                case region_id::arm_l:
                    need = body_part_arm_l;
                    break;
                case region_id::arm_r:
                    need = body_part_arm_r;
                    break;
                case region_id::hand_l:
                case region_id::wrist_l:
                case region_id::rings_l:
                    need = body_part_hand_l;
                    break;
                case region_id::hand_r:
                case region_id::wrist_r:
                case region_id::rings_r:
                    need = body_part_hand_r;
                    break;
                case region_id::leg_lower_l:
                    need = body_part_leg_l;
                    break;
                case region_id::leg_lower_r:
                    need = body_part_leg_r;
                    break;
                case region_id::foot_l:
                    need = body_part_foot_l;
                    break;
                case region_id::foot_r:
                    need = body_part_foot_r;
                    break;
                case region_id::ear_l:
                case region_id::ear_r:
                    need = body_part_head;
                    break;
                default:
                    break;
            }
            // Unspecified limb requirement (nullopt) is not the same as
            // NULL_ID, which is_valid() but not present on the character.
            if( need && !you.has_part( *need ) ) {
                continue;
            }
        }
        out.push_back( info.id );
    }
    return out;
}

hand_occupancy hands_for( Character &you )
{
    hand_occupancy hands;
    hands.main_hand = you.get_wielded_item();
    if( hands.main_hand && hands.main_hand.get_item() ) {
        hands.main_reserves_both_hands = hands.main_hand->is_two_handed( you );
    }
    // Current engine: shield-only off-hand via worn shield selection.
    item_location shield = you.best_shield();
    if( shield && shield.get_item() ) {
        hands.off_hand = shield;
    } else {
        // Fallback scan matching existing doll off-hand slot rules.
        for( const item_location &loc : you.top_items_loc() ) {
            if( loc && you.is_worn( *loc ) && is_shield_offhand( *loc ) ) {
                hands.off_hand = loc;
                break;
            }
        }
    }
    return hands;
}

} // namespace equipment_layout
