#include "progression_ui.h"

#if defined(TILES)
#include <algorithm>
#include <functional>
#include <set>
#include <utility>
#include <vector>

#include "avatar.h"
#include "cata_imgui.h"
#include "cata_utility.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "localized_comparator.h"
#include "math_parser_diag_value.h"
#include "mutation.h"
#include "npc.h"
#include "output.h"
#include "string_formatter.h"
#include "talker.h"
#include "talker_topic.h"
#include "text.h"
#include "translations.h"
#include "ui_hybrid_window.h"
#include "ui_manager.h"

namespace
{
struct perk_choice {
    trait_id id;
    std::string name;
    std::string description;
    std::string requirements;
    std::string category;
    talk_response response;
    bool playstyle = false;
};

std::string category_name( const std::string &topic )
{
    if( topic.find( "PLAYSTYLE" ) != std::string::npos ) {
        return _( "Playstyle" );
    }
    if( topic.find( "METAMAGIC" ) != std::string::npos ) {
        return _( "Magic" );
    }
    if( topic.find( "TEMPO" ) != std::string::npos ) {
        return _( "Tempo" );
    }
    if( topic.find( "MOMENTUM" ) != std::string::npos ) {
        return _( "Momentum" );
    }
    if( topic.find( "INSIGHT" ) != std::string::npos ) {
        return _( "Insight" );
    }
    if( topic.find( "POISE" ) != std::string::npos ) {
        return _( "Defense" );
    }
    return topic.find( "TALK_MA_" ) == 0 ? _( "Standalone" ) : _( "Lifestyle" );
}

bool selection_topic( const std::string &topic )
{
    return topic == "TALK_PERK_MENU_SELECT" || topic == "TALK_PERK_MENU_SELECT_PLAYSTYLE" ||
           topic == "TALK_MA_PERK_MENU_SELECT";
}

dialogue make_dialogue( avatar &you, const std::string &topic )
{
    return dialogue( get_talker_for( you ), get_talker_for( std::vector<std::string> { topic } ) );
}

std::vector<perk_choice> available_choices( avatar &you, const std::string &root )
{
    std::vector<perk_choice> choices;
    std::set<std::string> visited;
    std::set<trait_id> seen_traits;
    std::function<void( const std::string & )> visit = [&]( const std::string & topic ) {
        if( !visited.insert( topic ).second ) {
            return;
        }
        dialogue d = make_dialogue( you, root );
        d.gen_responses( talk_topic( topic ) );
        const std::vector<talk_response> responses = d.responses;
        for( const talk_response &response : responses ) {
            if( response.condition && !response.condition( d ) ) {
                continue;
            }
            const std::string &next = response.success.next_topic.id;
            if( selection_topic( next ) ) {
                // Selection effects only prepare description and prerequisite
                // context. The separate confirmation response buys the perk.
                dialogue preview = make_dialogue( you, root );
                response.success.apply( preview );
                const trait_id id( preview.get_value( "trait_id" ).str() );
                if( !id.is_valid() || you.has_trait( id ) || !seen_traits.insert( id ).second ) {
                    continue;
                }
                std::string requirements = preview.get_value( "trait_requirement_description" ).str();
                parse_tags( requirements, you, you, preview );
                std::string extra = preview.get_value( "trait_additional_details" ).str();
                parse_tags( extra, you, you, preview );
                choices.push_back( { id, id->name(), id->desc() + ( extra.empty() ? "" : "\n\n" + extra ),
                                     requirements, category_name( topic ), response,
                                     next == "TALK_PERK_MENU_SELECT_PLAYSTYLE" } );
            } else if( next.rfind( "TALK_PERK_MENU_PLAYSTYLE", 0 ) == 0 ||
                       next == "TALK_PERK_MENU_METAMAGIC" ||
                       ( next.rfind( "TALK_MA_PERK_MENU_", 0 ) == 0 &&
                         next != "TALK_MA_PERK_MENU_HELP" && next != root ) ) {
                visit( next );
            }
        }
    };
    visit( root );
    std::sort( choices.begin(), choices.end(), []( const perk_choice & a, const perk_choice & b ) {
        return localized_comparator()( a.name, b.name );
    } );
    return choices;
}
} // namespace
#endif

bool progression_ui::show( const std::string &topic )
{
#if defined(TILES)
    const bool martial = topic == "TALK_MA_PERK_MENU_MAIN";
    if( !martial && topic != "TALK_PERK_MENU_MAIN" ) {
        return false;
    }
    avatar &you = get_avatar();
    dialogue initial = make_dialogue( you, topic );
    effect_on_condition_id( martial ? "EOC_give_ma_perk_menu" : "EOC_give_perk_menu" )->activate(
        initial );
    std::vector<perk_choice> choices = available_choices( you, topic );
    std::vector<size_t> shown;
    std::string category;
    std::string status;
    char search[128] = {};
    int selected = -1;
    bool learn = false;
    bool settings = false;
    bool done = false;
    const auto value = [&]( const std::string & key ) {
        const diag_value &v = you.get_value( key );
        return v.is_empty() ? 0.0 : v.dbl();
    };
    hybrid_window window( martial ? _( "Martial Mastery" ) : _( "Perks" ), [&]() {
        const double level = value( martial ? "ma_current_level" : "current_level" );
        const double points = value( martial ? "num_ma_perks" : "num_perks" );
        const double xp = value( martial ? "ma_available_exp" : "available_exp" );
        const double needed = value( martial ? "ma_exp_to_perk" : "exp_to_perk" );
        ImGui::Text( "%s", string_format(
                         _( "Level %.0f   |   %.0f points available   |   XP %.0f / %.0f" ),
                         level, points, xp, needed ).c_str() );
        if( !martial ) {
            ImGui::Text( "%s", string_format( _( "Playstyle points: %.0f" ),
                                              value( "playstyle_perks_available" ) ).c_str() );
        }
        ImGui::ProgressBar( needed > 0 ? std::clamp( static_cast<float>( xp / needed ), 0.f, 1.f ) : 0.f,
                            ImVec2( -1, ImGui::GetTextLineHeight() ), "" );
        ImGui::TextDisabled( "%s",
                             _( "Available choices only. Select a perk to review it before learning." ) );
        ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x * 0.62f );
        ImGui::InputTextWithHint( "##search", _( "Search perks…" ), search, sizeof( search ) );
        ImGui::SameLine();
        ImGui::SetNextItemWidth( -1 );
        if( ImGui::BeginCombo( "##category",
                               category.empty() ? _( "All categories" ) : category.c_str() ) ) {
            if( ImGui::Selectable( _( "All categories" ), category.empty() ) ) {
                category.clear();
            }
            std::set<std::string> categories;
            for( const perk_choice &entry : choices ) {
                categories.insert( entry.category );
            }
            for( const std::string &name : categories ) {
                if( ImGui::Selectable( name.c_str(), category == name ) ) {
                    category = name;
                }
            }
            ImGui::EndCombo();
        }
        shown.clear();
        for( size_t i = 0; i < choices.size(); ++i ) {
            const perk_choice &entry = choices[i];
            if( ( category.empty() || category == entry.category ) &&
                ( lcmatch( entry.name, search ) || lcmatch( entry.description, search ) ) ) {
                shown.push_back( i );
            }
        }
        if( std::find( shown.begin(), shown.end(), static_cast<size_t>( selected ) ) == shown.end() ) {
            selected = shown.empty() ? -1 : static_cast<int>( shown.front() );
        }
        const float height = std::max( 80.f, ImGui::GetContentRegionAvail().y -
                                       ImGui::GetFrameHeightWithSpacing() * 2.5f );
        const float width = ImGui::GetContentRegionAvail().x;
        if( ImGui::BeginChild( "##available", ImVec2( width * 0.4f, height ), ImGuiChildFlags_Borders ) ) {
            if( shown.empty() ) {
                ImGui::TextWrapped( "%s", points < 1 ? _( "Earn XP to gain your next perk point." ) :
                                    choices.empty() ? _( "No perks currently meet your requirements and point budget." ) :
                                    _( "No matching perks. Try a different search or category." ) );
            }
            for( size_t i : shown ) {
                ImGui::PushID( static_cast<int>( i ) );
                if( ImGui::Selectable( choices[i].name.c_str(), selected == static_cast<int>( i ) ) ) {
                    selected = static_cast<int>( i );
                }
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();
        if( ImGui::BeginChild( "##details", ImVec2( 0, height ), ImGuiChildFlags_Borders ) ) {
            if( selected >= 0 ) {
                const perk_choice &entry = choices[selected];
                ImGui::TextWrapped( "%s", entry.name.c_str() );
                ImGui::TextDisabled( "%s", entry.category.c_str() );
                ImGui::Separator();
                cataimgui::TextColoredParagraph( c_light_gray, entry.description );
                ImGui::Spacing();
                ImGui::TextWrapped( "%s", entry.playstyle ? _( "Cost: 1 perk point + 1 playstyle point" ) :
                                    martial ? _( "Cost: 1 martial point" ) : _( "Cost: 1 perk point" ) );
                ImGui::TextWrapped( "%s", entry.requirements.c_str() );
            } else {
                ImGui::TextWrapped( "%s", _( "Choose an available perk to see what it does." ) );
            }
        }
        ImGui::EndChild();
        ImGui::BeginDisabled( selected < 0 );
        if( ImGui::Button( _( "Learn selected perk" ) ) ) {
            learn = true;
        }
        ImGui::EndDisabled();
        if( !martial ) {
            ImGui::SameLine();
            if( ImGui::Button( _( "Settings" ) ) ) {
                settings = true;
            }
        }
        ImGui::SameLine();
        if( ImGui::Button( _( "Close" ) ) ) {
            done = true;
        }
        ImGui::TextWrapped( "%s", status.empty() ?
                            _( "Learned perks are permanent and disappear from this list." ) :
                            status.c_str() );
    } );
    input_context ctxt( "ASTRAL_PROGRESSION" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.set_timeout( 16 );
    while( !done && window.get_is_open() ) {
        ui_manager::redraw_invalidated();
        const std::string action = ctxt.handle_input();
        if( action == "QUIT" && !cataimgui::client::want_text_input() ) {
            break;
        }
        if( settings ) {
            settings = false;
            window.set_hidden( true );
            you.talk_to( get_talker_for( std::vector<std::string> { "TALK_PERK_MENU_CONFIG" } ),
                         false, false, true );
            window.set_hidden( false );
            choices = available_choices( you, topic );
            selected = -1;
        }
        if( learn && selected >= 0 ) {
            learn = false;
            const perk_choice entry = choices[selected];
            dialogue purchase = make_dialogue( you, topic );
            if( !entry.response.condition || entry.response.condition( purchase ) ) {
                const talk_topic confirmation = entry.response.success.apply( purchase );
                purchase.gen_responses( confirmation );
                if( !purchase.responses.empty() ) {
                    const talk_response buy = purchase.responses.front();
                    if( buy.success.next_topic.id == topic && ( !buy.condition || buy.condition( purchase ) ) ) {
                        buy.success.apply( purchase );
                        status = string_format( _( "Learned %s." ), entry.name );
                    }
                }
            }
            choices = available_choices( you, topic );
            selected = -1;
        }
    }
    return true;
#else
    ( void )topic;
    return false;
#endif
}
