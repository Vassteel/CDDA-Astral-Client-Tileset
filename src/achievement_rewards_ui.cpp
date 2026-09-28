#include "achievement_rewards_ui.h"

#include "achievement.h"
#include "achievement_rewards.h"
#include "output.h"
#include "translations.h"

#if defined(TILES)
#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <string>
#include "avatar.h"
#include "bodypart.h"
#include "cata_imgui.h"
#include "filesystem.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "path_info.h"
#include "sdltiles.h"
#include "sdl_wrappers.h"
#include "string_formatter.h"
#include "ui_hybrid_window.h"
#include "ui_manager.h"

namespace achievement_rewards_ui
{
namespace
{
std::map<std::string, SDL_Texture_Ptr> images;
std::function<void()> pending;
std::string status;
std::string selected;
std::string category;
int view = 0;
int choice = 0;
char search[128] = {};

class completion_window : public hybrid_window
{
    public:
        using hybrid_window::hybrid_window;
    protected:
        cataimgui::bounds get_bounds() override {
            const ImVec2 size = ImGui::GetMainViewport()->Size;
            return { -1.f, -1.f, std::min( size.x * 0.9f, 700.f ),
                     std::min( size.y * 0.9f, 560.f ) };
        }
};

void art( const std::string &key, float size )
{
    if( key.empty() ||
        key.find_first_not_of( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-" ) !=
        std::string::npos || renderer_should_abort_frame() ) {
        return;
    }
    auto it = images.find( key );
    if( it == images.end() ) {
        const cata_path path = PATH_INFO::datadir_path() / "achievement_art" / ( key + ".png" );
        SDL_Texture_Ptr texture;
        if( file_exist( path ) ) {
            try {
                const SDL_Surface_Ptr surface = load_image( path.get_unrelative_path().u8string().c_str() );
                texture = CreateTextureFromSurface( get_sdl_renderer(), surface );
            } catch( const std::exception & ) {
                // Missing or invalid optional artwork must never interrupt play.
            }
        }
        it = images.emplace( key, std::move( texture ) ).first;
    }
    if( it->second ) {
        ImGui::Image( reinterpret_cast<ImTextureID>( it->second.get() ), ImVec2( size, size ) );
    }
}
void draw_bank()
{
    auto &bank = get_achievements().reward_bank;
    ImGui::TextWrapped( "%s",
                        _( "Recovery stays here until you use it. Item deliveries are collected one at a time; failed deliveries remain available." ) );
    for( const auto &benefit : bank.benefits ) {
        ImGui::TextWrapped( "%s", achievement_rewards::credit_description( benefit ).c_str() );
    }
    bool any = false;
    for( const auto &entry : bank.credits ) {
        if( entry.second <= 0 ) {
            continue;
        }
        any = true;
        ImGui::PushID( entry.first.c_str() );
        ImGui::Separator();
        ImGui::Text( "%s × %d", achievement_rewards::credit_name( entry.first ).c_str(), entry.second );
        ImGui::TextWrapped( "%s", achievement_rewards::credit_description( entry.first ).c_str() );
        const auto use = [&]( const bodypart_id & part ) {
            const std::string credit = entry.first;
            pending = [credit, part]() {
                status = get_achievements().is_enabled() &&
                         get_achievements().reward_bank.redeem( get_avatar(), credit, part ) ?
                         _( "Reward used." ) : _( "Nothing changed. The credit is still in your bank." );
            };
        };
        if( entry.first == "field_recovery" || entry.first == "boneknit" ) {
            for( const bodypart_id &bp : get_avatar().get_all_body_parts( get_body_part_flags::only_main ) ) {
                const bool broken = get_avatar().is_limb_broken( bp );
                const bool eligible = entry.first == "boneknit" ? broken : !broken &&
                                      ( get_avatar().get_part_hp_cur( bp ) < get_avatar().get_part_hp_max( bp ) ||
                                        get_avatar().has_effect( efftype_id( "bleed" ), bp ) );
                ImGui::BeginDisabled( !eligible );
                if( ImGui::Button( string_format( "%s (%d/%d)###%s", body_part_name( bp ),
                                                  get_avatar().get_part_hp_cur( bp ), get_avatar().get_part_hp_max( bp ),
                                                  bp.id().str() ).c_str() ) ) {
                    use( bp );
                }
                ImGui::EndDisabled();
            }
        } else if( ImGui::Button( _( "Use recovery" ) ) ) {
            use( bodypart_id() );
        }
        ImGui::PopID();
    }
    for( size_t i = 0; i < bank.parcels.size(); ++i ) {
        any = true;
        ImGui::PushID( static_cast<int>( i ) );
        ImGui::Separator();
        achievement_rewards::option preview;
        preview.items = { bank.parcels[i] };
        ImGui::TextWrapped( "%s", achievement_rewards::describe( preview ).c_str() );
        const auto deliver = [i]( bool feet ) {
            pending = [i, feet]() {
                status = get_achievements().is_enabled() &&
                         get_achievements().reward_bank.deliver( get_avatar(), i, feet ) ?
                         _( "Item collected." ) :
                         _( "Cannot deliver there. Make room or choose another destination; your item remains banked." );
            };
        };
        if( ImGui::Button( _( "Collect one into inventory" ) ) ) {
            deliver( false );
        }
        ImGui::SameLine();
        if( ImGui::Button( _( "Place one at my feet" ) ) ) {
            deliver( true );
        }
        ImGui::PopID();
    }
    if( !any ) {
        ImGui::TextWrapped( "%s",
                            _( "No unused rewards. Complete achievements, then claim their rewards in Ready to claim." ) );
    }
}
} // namespace

void release_gpu_resources()
{
    images.clear();
}
void process_actions()
{
    if( pending ) {
        auto action = std::move( pending );
        pending = nullptr;
        action();
    }
}
void draw()
{
    auto &tracker = get_achievements();
    if( !tracker.is_enabled() ) {
        ImGui::TextWrapped( "%s",
                            _( "Achievements and reward claims are disabled for debug characters." ) );
        return;
    }
    const char *tabs[] = { _( "In progress" ), _( "Ready to claim" ), _( "Completed" ), _( "Reward bank" ) };
    if( ImGui::BeginTabBar( "##astral_achievements" ) ) {
        for( int i = 0; i < 4; ++i ) {
            if( ImGui::BeginTabItem( tabs[i] ) ) {
                view = i;
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
    if( view == 3 ) {
        draw_bank();
    } else {
        ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x * 0.6f );
        ImGui::InputTextWithHint( "##achievement_search", _( "Search achievements…" ), search,
                                  sizeof( search ) );
        ImGui::SameLine();
        ImGui::SetNextItemWidth( -1 );
        if( ImGui::BeginCombo( "##achievement_category",
                               category.empty() ? _( "All categories" ) : category.c_str() ) ) {
            if( ImGui::Selectable( _( "All categories" ), category.empty() ) ) {
                category.clear();
            }
            std::set<std::string> categories = { _( "Milestones" ) };
            for( const auto &d : achievement_rewards::all() ) {
                categories.insert( _( d.second.category ) );
            }
            for( const std::string &c : categories ) {
                if( ImGui::Selectable( c.c_str(), c == category ) ) {
                    category = c;
                }
            }
            ImGui::EndCombo();
        }
        std::vector<const achievement *> shown;
        for( const achievement *a : tracker.valid_achievements() ) {
            if( a->is_conduct() || tracker.is_hidden( a ) ) {
                continue;
            }
            const auto completion = tracker.is_completed( a->id );
            const bool done = completion == achievement_completion::completed;
            const bool claimed = tracker.reward_bank.claims.count( a->id.str() );
            const auto *d = achievement_rewards::find( a->id.str() );
            if( ( view == 0 && completion != achievement_completion::pending ) || ( view == 1 && ( !done ||
                    claimed || !d ) ) || ( view == 2 &&
                                           !done ) ) {
                continue;
            }
            if( !category.empty() && category != ( d ? _( d->category ) : _( "Milestones" ) ) ) {
                continue;
            }
            if( !lcmatch( a->name().translated(), search ) &&
                !lcmatch( a->description().translated(), search ) ) {
                continue;
            }
            shown.push_back( a );
        }
        std::sort( shown.begin(), shown.end(), []( const achievement * a, const achievement * b ) {
            return a->name().translated() < b->name().translated();
        } );
        auto current = std::find_if( shown.begin(), shown.end(), []( const achievement * a ) {
            return a->id.str() == selected;
        } );
        if( current == shown.end() ) {
            selected = shown.empty() ? "" : shown.front()->id.str();
            choice = 0;
        }
        const float height = std::max( 150.f,
                                       ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 3 );
        if( ImGui::BeginChild( "##achievement_list", ImVec2( ImGui::GetContentRegionAvail().x * 0.38f,
                               height ), ImGuiChildFlags_Borders ) ) {
            ImGui::Text( _( "%d achievements" ), static_cast<int>( shown.size() ) );
            for( const achievement *a : shown ) {
                if( ImGui::Selectable( ( a->name().translated() + "###" + a->id.str() ).c_str(),
                                       selected == a->id.str() ) ) {
                    selected = a->id.str();
                    choice = 0;
                }
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();
        if( ImGui::BeginChild( "##achievement_details", ImVec2( 0, height ), ImGuiChildFlags_Borders ) ) {
            const achievement_id id( selected );
            if( !selected.empty() && id.is_valid() ) {
                const auto *d = achievement_rewards::find( selected );
                art( d ? d->art : selected, 128.f );
                cataimgui::draw_colored_text( tracker.ui_text_for( &id.obj() ) );
                if( d ) {
                    if( tracker.is_completed( id ) == achievement_completion::pending ) {
                        const auto &b = tracker.reward_bank;
                        int progress = 0;
                        int target = 0;
                        const auto seen_count = [&]( const std::string & key ) {
                            const auto it = b.seen.find( key );
                            return it == b.seen.end() ? 0 : static_cast<int>( it->second.size() );
                        };
                        if( selected == "astral_002" ) {
                            progress = seen_count( "six_hour_sleep_days" );
                            target = 7;
                        } else if( selected == "astral_021" || selected == "astral_022" ) {
                            progress = seen_count( "construction_stages" );
                            target = selected == "astral_021" ? 10 : 50;
                        } else if( selected == "astral_025" ) {
                            progress = seen_count( "kiln_collected" );
                            target = 3;
                        } else if( selected == "astral_209" ) {
                            const auto it = b.counters.find( "rested_episodes" );
                            progress = it == b.counters.end() ? 0 : it->second;
                            target = 3;
                        }
                        static const std::map<std::string, std::pair<std::string, int>> counters = {
                            { "astral_084", { "hostile_blocks", 100 } },
                            { "astral_136", { "night_crafts", 25 } },
                            { "astral_199", { "bed_healing", 100 } }
                        };
                        const auto counter = counters.find( selected );
                        if( counter != counters.end() ) {
                            const auto value = b.counters.find( counter->second.first );
                            progress = value == b.counters.end() ? 0 : value->second;
                            target = counter->second.second;
                        }
                        if( target ) {
                            ImGui::ProgressBar( std::clamp( static_cast<float>( progress ) / target, 0.f, 1.f ),
                                                ImVec2( -1.f, 0.f ), string_format( "%d / %d", progress, target ).c_str() );
                        }
                    }
                    const auto claim = tracker.reward_bank.claims.find( selected );
                    const std::vector<achievement_rewards::option> reward_choices = claim ==
                            tracker.reward_bank.claims.end() ?
                            d->choices : std::vector<achievement_rewards::option> { claim->second };
                    if( static_cast<size_t>( choice ) >= reward_choices.size() ) {
                        choice = 0;
                    }
                    ImGui::SeparatorText( _( "Reward" ) );
                    for( size_t i = 0; i < reward_choices.size(); ++i ) {
                        if( ImGui::RadioButton( _( reward_choices[i].name ).c_str(), choice == static_cast<int>( i ) ) ) {
                            choice = i;
                        }
                    }
                    if( choice >= 0 && static_cast<size_t>( choice ) < reward_choices.size() ) {
                        ImGui::TextWrapped( "%s", achievement_rewards::describe( reward_choices[choice] ).c_str() );
                        const bool claimed = tracker.reward_bank.claims.count( selected );
                        if( claimed ) {
                            ImGui::TextWrapped( "%s", _( "Claimed. Unused items and recovery are in Reward bank." ) );
                        } else {
                            ImGui::BeginDisabled( tracker.is_completed( id ) != achievement_completion::completed );
                            if( ImGui::Button( _( "Claim selected reward" ) ) ) {
                                const std::string claim_id = selected;
                                const int claim_choice = choice;
                                pending = [claim_id, claim_choice]() {
                                    auto &t = get_achievements();
                                    status = t.reward_bank.claim( t, claim_id, claim_choice ) ?
                                             _( "Claimed. Open Reward bank to collect items or use recovery." ) :
                                             _( "Reward could not be claimed." );
                                };
                            }
                            ImGui::EndDisabled();
                        }
                    }
                }
            } else {
                ImGui::TextWrapped( "%s", _( "No matching achievements in this view." ) );
            }
        }
        ImGui::EndChild();
    }
    if( !status.empty() ) {
        ImGui::TextWrapped( "%s", status.c_str() );
    }
}
void popup( const achievement &a )
{
    bool done = false;
    completion_window window( _( "Achievement completed!" ), [&]() {
        const auto *d = achievement_rewards::find( a.id.str() );
        art( d ? d->art : a.id.str(), 128.f );
        cataimgui::draw_colored_text( get_achievements().ui_text_for( &a ) );
        if( d ) {
            ImGui::Separator();
            ImGui::TextWrapped( "%s",
                                _( "A reward is ready. Open Achievements → Ready to claim whenever you want to collect it." ) );
            for( const auto &o : d->choices ) {
                ImGui::TextWrapped( "%s", _( o.name ).c_str() );
                ImGui::TextWrapped( "%s", achievement_rewards::describe( o ).c_str() );
            }
        }
        if( ImGui::Button( _( "Continue" ) ) ) {
            done = true;
        }
    } );
    input_context ctxt( "ASTRAL_ACHIEVEMENT_POPUP" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.set_timeout( 16 );
    while( !done && window.get_is_open() ) {
        ui_manager::redraw_invalidated();
        const std::string action = ctxt.handle_input();
        if( action == "QUIT" || action == "CONFIRM" ) {
            break;
        }
    }
}
} // namespace achievement_rewards_ui
#else
namespace achievement_rewards_ui
{
void draw() {}
void process_actions() {}
void release_gpu_resources() {}
void popup( const achievement &a )
{
    ::popup( get_achievements().ui_text_for( &a ) );
}
}
#endif
