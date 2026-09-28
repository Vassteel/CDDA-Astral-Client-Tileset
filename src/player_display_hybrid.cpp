#include "player_display_hybrid.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "addiction.h"
#include "avatar.h"
#include "avatar_action.h"
#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_imgui.h"
#include "character.h"
#include "cata_scope_helpers.h"
#include "bodygraph.h"
#include "color.h"
#include "display.h"
#include "dialogue.h"
#include "effect.h"
#include "effect_on_condition.h"
#include "flag.h"
#include "enum_conversions.h"
#include "game.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "input_popup.h"
#include "magic_enchantment.h"
#include "mutation.h"
#include "output.h"
#include "proficiency.h"
#include "skill.h"
#include "skill_ui.h"
#include "string_formatter.h"
#include "talker.h"
#include "translations.h"
#include "ui_hybrid_chrome.h"
#include "ui_manager.h"
#include "units.h"
#include "units_utility.h"
#include "weather.h"
#include "wound.h"

static const bionic_id bio_cqb( "bio_cqb" );
static const skill_id skill_dodge( "dodge" );
static const trait_id trait_TROGLO( "TROGLO" );
static const trait_id trait_TROGLO2( "TROGLO2" );
static const trait_id trait_TROGLO3( "TROGLO3" );

static const efftype_id effect_bite( "bite" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_mending( "mending" );


namespace
{

enum class sheet_tab : int {
    stats = 0,
    health,
    morale,
    bodygraph,
    encumbrance,
    speed,
    skills,
    traits,
    bionics,
    effects,
    proficiencies,
    customize,
    count
};

struct speed_row {
    bool is_speed = true;
    std::string description;
    int val = 0;
    bool percent = false;
};

struct bio_row {
    std::string name;
    int count = 1;
    std::string description;
};

class player_display_hybrid_ui : public cataimgui::window
{
    public:
        player_display_hybrid_ui( Character &you_in, bool customize_in )
            : cataimgui::window( _( "Character" ), ImGuiWindowFlags_None ),
              you( you_in ), customize( customize_in ) {
            rebuild();
        }

        bool is_done() const {
            return done;
        }

        int visible_tab_count() const {
            // Hide Customize tab unless chargen / debug customize mode.
            return customize ? static_cast<int>( sheet_tab::count )
                   : static_cast<int>( sheet_tab::customize );
        }

        void select_tab( sheet_tab t ) {
            if( t == sheet_tab::customize && !customize ) {
                return;
            }
            if( static_cast<int>( t ) >= visible_tab_count() ) {
                return;
            }
            curtab = t;
            line = 0;
            force_tab = true;
        }

        void flush_deferred_ui() {
            if( !pending_detail.empty() ) {
                restore_on_out_of_scope<bool> restore_visibility( hide_ui );
                hide_ui = true;
                const std::string detail = std::exchange( pending_detail, {} );
                if( detail == "profession" ) {
                    string_input_popup_imgui popup( 50, you.custom_profession, _( "Profession name" ) );
                    const std::string result = popup.query();
                    if( !popup.cancelled() ) {
                        you.custom_profession = result;
                    }
                } else if( detail == "armor" ) {
                    change_armor_sprite( you );
                } else if( detail == "body" ) {
                    display_bodygraph( you );
                } else if( detail == "proficiency" ) {
                    show_proficiencies_window( you, line < profs.size() ?
                                               std::make_optional( profs[line].id ) : std::nullopt );
                } else if( detail == "variant" && line < traits.size() ) {
                    const mutation_variant *variant = traits[line].trait->pick_variant_menu();
                    you.set_mut_variant( traits[line].trait, variant );
                } else if( ( detail == "perks" || detail == "martial" ) && you.is_avatar() ) {
                    dialogue d( get_talker_for( you ), nullptr );
                    const bool martial = detail == "martial";
                    effect_on_condition_id( martial ? "EOC_give_ma_perk_menu" :
                                            "EOC_give_perk_menu" )->activate( d );
                    effect_on_condition_id( martial ? "EOC_open_ma_perk_menu" :
                                            "EOC_open_perk_menu" )->activate( d );
                }
                rebuild();
            }
            // Nested dialogs AFTER ImGui::End — avoids Begin/End re-entry CTD.
            // Morale / Body / Medical / Customize are now in-sheet tabs; only
            // true nested UIs (use item, treat, rename) remain deferred.
            if( pending_toggle_gender ) {
                pending_toggle_gender = false;
                you.male = !you.male;
                rebuild();
            }
            if( pending_name_edit ) {
                pending_name_edit = false;
                string_input_popup_imgui popup( 50, you.play_name.value_or( std::string() ),
                                                _( "New name (leave empty to reset)" ) );
                popup.set_label( _( "Name:" ) );
                const std::string result = popup.query();
                if( !popup.cancelled() ) {
                    if( result.empty() ) {
                        you.play_name.reset();
                    } else {
                        you.play_name = result;
                    }
                    rebuild();
                }
            }
            if( pending_use_item ) {
                pending_use_item = false;
                if( you.is_avatar() ) {
                    avatar_action::use_item( *you.as_avatar() );
                    rebuild();
                }
            }
            if( pending_treat ) {
                pending_treat = false;
                const std::vector<bodypart_id> parts =
                    you.get_all_body_parts( get_body_part_flags::only_main );
                if( !parts.empty() ) {
                    if( line >= parts.size() ) {
                        line = static_cast<unsigned>( parts.size() - 1 );
                    }
                    if( you.pick_wound_fix( parts[line] ) ) {
                        rebuild();
                    }
                }
            }
        }

        void pump_input( input_context &ctxt ) {
            // Short timeout so ImGui keeps receiving frames while SDL mouse
            // events are forwarded (same pattern as Consume / Equipment / Build).
            ui_manager::redraw_invalidated();
            flush_deferred_ui();
            std::string action = has_button_action() ? get_button_action() : ctxt.handle_input();
            if( action.empty() || action == "ERROR" || action == "TIMEOUT" ||
                action == "ANY_INPUT" ) {
                return;
            }
            if( action == "QUIT" ) {
                done = true;
            } else if( action == "CHANGE_PROFESSION_NAME" ) {
                pending_detail = "profession";
            } else if( action == "CHANGE_ARMOR_SPRITE" ) {
                pending_detail = "armor";
            } else if( action == "SELECT_TRAIT_VARIANT" && curtab == sheet_tab::traits ) {
                pending_detail = "variant";
            } else if( action == "VIEW_PROFICIENCIES" ) {
                pending_detail = "proficiency";
            } else if( action == "morale" ) {
                select_tab( sheet_tab::morale );
            } else if( action == "MEDICAL_MENU" ) {
                select_tab( sheet_tab::health );
            } else if( customize && action == "SWITCH_GENDER" ) {
                select_tab( sheet_tab::customize );
            } else if( action == "VIEW_BODYSTAT" ) {
                select_tab( sheet_tab::bodygraph );
            } else if( action == "NEXT_TAB" || action == "RIGHT" ) {
                const int n = visible_tab_count();
                curtab = static_cast<sheet_tab>( ( static_cast<int>( curtab ) + 1 ) % n );
                line = 0;
                force_tab = true;
            } else if( action == "PREV_TAB" || action == "LEFT" ) {
                const int n = visible_tab_count();
                int t = static_cast<int>( curtab ) - 1;
                if( t < 0 ) {
                    t = n - 1;
                }
                curtab = static_cast<sheet_tab>( t );
                line = 0;
                force_tab = true;
            } else if( action == "UP" || action == "SCROLL_UP" ) {
                if( line > 0 ) {
                    --line;
                }
            } else if( action == "DOWN" || action == "SCROLL_DOWN" ) {
                ++line;
            }
        }

    protected:
        void draw() override {
            ui_hybrid_chrome::push();
            cataimgui::window::draw();
            ui_hybrid_chrome::pop();
        }

        cataimgui::bounds get_bounds() override {
            const ImVec2 vp = ImGui::GetMainViewport()->Size;
            const float scale = std::max( 1.f, ImGui::GetFontSize() / 16.f );
            return { -1.f, -1.f, std::min( vp.x * 0.94f, 1280.f * scale ),
                     std::min( vp.y * 0.92f, 800.f * scale ) };
        }

        void draw_controls() override {
            if( hide_ui ) {
                hide_if_hidden();
                return;
            }
            if( ImGui::SmallButton( _( "Profession name…" ) ) ) {
                pending_detail = "profession";
            }
            ImGui::SameLine();
            if( ImGui::SmallButton( _( "Armor appearance…" ) ) ) {
                pending_detail = "armor";
            }
            if( !get_is_open() ) {
                done = true;
                return;
            }

            draw_header();
            if( you.is_avatar() && you.has_trait( trait_id( "perk_perk_menu" ) ) ) {
                if( ImGui::Button( _( "Perks" ) ) ) {
                    pending_detail = "perks";
                }
                ImGui::SameLine();
                if( ImGui::Button( _( "Martial Mastery" ) ) ) {
                    pending_detail = "martial";
                }
            }
            draw_tabs();

            const float footer_h = ImGui::GetFrameHeightWithSpacing() * 1.6f;
            if( ImGui::BeginChild( "##SHEET_BODY", ImVec2( 0, -footer_h ), ImGuiChildFlags_Borders ) ) {
                switch( curtab ) {
                    case sheet_tab::stats:
                        draw_stats();
                        break;
                    case sheet_tab::health:
                        draw_health();
                        break;
                    case sheet_tab::morale:
                        draw_morale();
                        break;
                    case sheet_tab::bodygraph:
                        draw_bodygraph();
                        break;
                    case sheet_tab::encumbrance:
                        draw_encumbrance();
                        break;
                    case sheet_tab::speed:
                        draw_speed();
                        break;
                    case sheet_tab::skills:
                        draw_skills();
                        break;
                    case sheet_tab::traits:
                        draw_traits();
                        break;
                    case sheet_tab::bionics:
                        draw_bionics();
                        break;
                    case sheet_tab::effects:
                        draw_effects();
                        break;
                    case sheet_tab::proficiencies:
                        draw_proficiencies();
                        break;
                    case sheet_tab::customize:
                        draw_customize();
                        break;
                    case sheet_tab::count:
                        break;
                }
            }
            ImGui::EndChild();

            ImGui::Separator();
            {
                const int n = ui_hybrid_chrome::push_toolbar_button( false );
                if( ImGui::Button( _( "Close" ), ImVec2( 120.f, 0 ) ) ) {
                    done = true;
                }
                ImGui::PopStyleColor( n );
            }
            // Shortcuts → in-sheet tabs (no nested windows / z-order fights).
            ImGui::SameLine();
            {
                const int n = ui_hybrid_chrome::push_toolbar_button(
                                  curtab == sheet_tab::morale );
                if( ImGui::Button( _( "Morale" ), ImVec2( 120.f, 0 ) ) ) {
                    select_tab( sheet_tab::morale );
                }
                ImGui::PopStyleColor( n );
            }
            ImGui::SameLine();
            {
                const int n = ui_hybrid_chrome::push_toolbar_button(
                                  curtab == sheet_tab::bodygraph );
                if( ImGui::Button( _( "Body graph" ), ImVec2( 120.f, 0 ) ) ) {
                    select_tab( sheet_tab::bodygraph );
                }
                ImGui::PopStyleColor( n );
            }
            ImGui::SameLine();
            {
                const int n = ui_hybrid_chrome::push_toolbar_button(
                                  curtab == sheet_tab::health );
                if( ImGui::Button( _( "Medical" ), ImVec2( 120.f, 0 ) ) ) {
                    select_tab( sheet_tab::health );
                }
                ImGui::PopStyleColor( n );
            }
            if( customize ) {
                ImGui::SameLine();
                const int n = ui_hybrid_chrome::push_toolbar_button(
                                  curtab == sheet_tab::customize );
                if( ImGui::Button( _( "Customize" ), ImVec2( 120.f, 0 ) ) ) {
                    select_tab( sheet_tab::customize );
                }
                ImGui::PopStyleColor( n );
            }
        }

    private:
        Character &you;
        bool customize = false;
        bool done = false;
        bool pending_toggle_gender = false;
        bool pending_name_edit = false;
        bool pending_use_item = false;
        bool pending_treat = false;
        std::string pending_detail;
        bodygraph_var bodygraph_mode = bodygraph_var::hp;
        sheet_tab curtab = sheet_tab::stats;
        unsigned line = 0;
        bool force_tab = true;
        int move_cost = 100;

        std::string race;
        std::vector<std::pair<std::string, std::string>> effects;
        std::vector<trait_and_var> traits;
        std::vector<bio_row> bionics;
        std::vector<HeaderSkill> skills;
        std::vector<speed_row> speed_rows;
        std::vector<display_proficiency> profs;

        void rebuild() {
            // Race / threshold mutation name (post-humanity)
            race.clear();
            if( you.crossed_threshold() ) {
                for( const trait_and_var &mut : you.get_mutations_variants() ) {
                    if( mut.trait->threshold ) {
                        race = mut.name();
                        break;
                    }
                }
            }

            // Effects (same assembly as vanilla disp_info)
            effects.clear();
            for( const effect &eff : you.get_effects() ) {
                const std::string name = eff.disp_name();
                if( name.empty() ) {
                    continue;
                }
                effects.emplace_back( name, eff.disp_desc() + '\n' + eff.disp_mod_source_info() );
            }
            if( you.get_perceived_pain() > 0 ) {
                const stat_mod ppen = you.read_pain_penalty();
                std::pair<std::string, nc_color> pain_desc = display::pain_text_color( you );
                std::string pain_text = colorize( string_format( _( "You are in %s\n" ),
                                                  pain_desc.first ), pain_desc.second );
                const auto add_if = [&]( const int amount, const char *const name ) {
                    if( amount > 0 ) {
                        pain_text += string_format( name, amount ) + "   ";
                    }
                };
                add_if( ppen.strength, _( "Strength -%d" ) );
                add_if( ppen.dexterity, _( "Dexterity -%d" ) );
                add_if( ppen.intelligence, _( "Intelligence -%d" ) );
                add_if( ppen.perception, _( "Perception -%d" ) );
                add_if( ppen.speed, _( "Speed -%d %%" ) );
                effects.emplace_back( _( "Pain" ), pain_text );
            }
            const float bmi = you.get_bmi_fat();
            if( bmi < character_weight_category::underweight ) {
                std::string starvation_name;
                std::string starvation_text;
                if( bmi < character_weight_category::emaciated ) {
                    starvation_name = _( "Severely Malnourished" );
                    starvation_text =
                        _( "Your body is severely weakened by starvation.  You might die if you don't start eating regular meals!\n\n" );
                } else {
                    starvation_name = _( "Malnourished" );
                    starvation_text =
                        _( "Your body is weakened by starvation.  Only time and regular meals will help you recover.\n\n" );
                }
                if( bmi < character_weight_category::normal ) {
                    const stat_mod wpen = you.get_weight_penalty();
                    starvation_text += string_format( _( "Strength -%d\n" ), wpen.strength );
                    starvation_text += string_format( _( "Dexterity -%d\n" ), wpen.dexterity );
                    starvation_text += string_format( _( "Intelligence -%d" ), wpen.intelligence );
                }
                effects.emplace_back( starvation_name, starvation_text );
            }
            if( you.has_trait( trait_TROGLO3 ) && g->is_in_sunlight( you.pos_bub() ) ) {
                effects.emplace_back( _( "In Sunlight" ),
                                      _( "The sunlight irritates you terribly.\nStrength - 4; Dexterity - 4; Intelligence - 4; Perception - 4" ) );
            } else if( you.has_trait( trait_TROGLO2 ) && g->is_in_sunlight( you.pos_bub() ) ) {
                if( incident_sun_irradiance( get_weather().weather_id, calendar::turn ) > irradiance::moderate ) {
                    effects.emplace_back( _( "In Sunlight" ),
                                          _( "The sunlight irritates you badly.\nStrength - 2; Dexterity - 2; Intelligence - 2; Perception - 2" ) );
                } else if( incident_sun_irradiance( get_weather().weather_id, calendar::turn ) > irradiance::low ) {
                    effects.emplace_back( _( "In Sunlight" ),
                                          _( "The sunlight irritates you badly.\nStrength - 1; Dexterity - 1; Intelligence - 1; Perception - 1" ) );
                }
            } else if( you.has_trait( trait_TROGLO ) && g->is_in_sunlight( you.pos_bub() ) &&
                       incident_sun_irradiance( get_weather().weather_id, calendar::turn ) > irradiance::moderate ) {
                effects.emplace_back( _( "In Sunlight" ),
                                      _( "The sunlight irritates you.\nStrength - 1; Dexterity - 1; Intelligence - 1; Perception - 1" ) );
            }
            for( addiction &elem : you.addictions ) {
                if( elem.sated < 0_turns && elem.intensity >= MIN_ADDICTION_LEVEL ) {
                    effects.emplace_back( elem.type->get_name().translated(),
                                          elem.type->get_description().translated() );
                }
            }
            for( const std::pair<std::string, std::string> &detail : you.enchantment_cache->details ) {
                effects.emplace_back( detail );
            }
            effects.erase( std::remove_if( effects.begin(), effects.end(),
            []( const std::pair<std::string, std::string> &e ) {
                return e.first.empty();
            } ), effects.end() );

            traits = you.get_mutations_variants( false );
            std::sort( traits.begin(), traits.end(), trait_var_display_sort );

            bionics.clear();
            {
                const auto bio_comp = []( const bionic_data & lhs, const bionic_data & rhs ) {
                    return lhs.name.translated_lt( rhs.name );
                };
                std::map<const bionic_data, int, decltype( bio_comp )> bmap( bio_comp );
                for( const bionic &bio : *you.my_bionics ) {
                    bmap[bio.info()]++;
                }
                for( const auto &pr : bmap ) {
                    bio_row row;
                    row.name = pr.first.name.translated();
                    row.count = pr.second;
                    row.description = pr.first.description.translated();
                    bionics.push_back( row );
                }
            }

            const std::vector<const Skill *> player_skill =
                Skill::get_skills_for_chr_display( you,
            []( const Skill & a, const Skill & b ) {
                return a.get_sort_rank() < b.get_sort_rank();
            } );
            skills = get_HeaderSkills( player_skill );

            // Speed / move cost rows
            speed_rows.clear();
            std::map<std::string, int> speed_effects;
            for( const effect &it : you.get_effects() ) {
                bool reduced = you.resists_effect( it );
                int move_adjust = it.get_mod( "SPEED", reduced );
                if( move_adjust != 0 ) {
                    speed_effects[it.get_speed_name()] += move_adjust;
                }
            }
            for( const auto &speed_effect : speed_effects ) {
                if( speed_effect.second != 0 ) {
                    speed_rows.push_back( { true, speed_effect.first, speed_effect.second, false } );
                }
            }
            for( const speed_bonus_effect &effect : you.get_speed_bonus_effects() ) {
                if( effect.bonus != 0 && speed_effects.find( effect.description ) == speed_effects.end() ) {
                    speed_rows.push_back( { true, effect.description, effect.bonus, false } );
                }
            }
            float movecost = 100;
            for( const run_cost_effect &effect : you.run_cost_effects( movecost ) ) {
                const int percent = std::trunc( effect.times * 100 - 100 );
                const int constant = std::trunc( effect.plus );
                if( ( effect.times != 1.0 && percent == 0 ) || ( effect.plus != 0 && constant == 0 ) ) {
                    continue;
                }
                speed_rows.push_back( { false, effect.description, percent != 0 ? percent : constant,
                                        percent != 0 } );
            }
            move_cost = static_cast<int>( movecost );

            profs = you.display_proficiencies();
        }

        void draw_header() {
            std::string title;
            if( you.crossed_threshold() && !race.empty() ) {
                title = string_format( _( "%1$s | %2$s | %3$s" ), you.get_name(),
                                       you.male ? _( "Male" ) : _( "Female" ), race );
            } else {
                const std::string profession = you.disp_profession();
                if( !profession.empty() ) {
                    title = string_format( _( "%1$s | %2$s | %3$s" ), you.get_name(),
                                           you.male ? _( "Male" ) : _( "Female" ), profession );
                } else {
                    title = string_format( _( "%1$s | %2$s" ), you.get_name(),
                                           you.male ? _( "Male" ) : _( "Female" ) );
                }
            }
            ui_hybrid_chrome::section_header( title.c_str() );
        }

        void draw_tabs() {
            static const char *labels[] = {
                translate_marker( "Stats" ),
                translate_marker( "Health" ),
                translate_marker( "Morale" ),
                translate_marker( "Body" ),
                translate_marker( "Encumbrance" ),
                translate_marker( "Speed" ),
                translate_marker( "Skills" ),
                translate_marker( "Traits" ),
                translate_marker( "Bionics" ),
                translate_marker( "Effects" ),
                translate_marker( "Proficiencies" ),
                translate_marker( "Customize" )
            };
            static_assert( sizeof( labels ) / sizeof( labels[0] ) ==
                           static_cast<size_t>( sheet_tab::count ),
                           "sheet_tab labels must match enum" );
            if( !ImGui::BeginTabBar( "##SHEET_TABS", ImGuiTabBarFlags_FittingPolicyScroll ) ) {
                return;
            }
            const int ntabs = visible_tab_count();
            for( int i = 0; i < ntabs; ++i ) {
                const bool should = force_tab && ( static_cast<int>( curtab ) == i );
                if( cataimgui::BeginTabItem( _( labels[i] ), should ) ) {
                    if( static_cast<int>( curtab ) != i ) {
                        curtab = static_cast<sheet_tab>( i );
                        line = 0;
                    }
                    ImGui::EndTabItem();
                }
            }
            force_tab = false;
            ImGui::EndTabBar();
        }


        void draw_stats() {
            // Two-pane: list (vanilla rows) + detail (vanilla draw_stats_info).
            const float avail = ImGui::GetContentRegionAvail().x;
            const float list_w = avail * 0.42f;
            if( ImGui::BeginChild( "##STAT_LIST", ImVec2( list_w, 0 ), ImGuiChildFlags_Borders ) ) {
                ui_hybrid_chrome::section_header( _( "STATS" ) );
                struct row_t {
                    const char *label;
                    std::string value;
                };
                const std::string blood = io::enum_to_string( you.my_blood_type ) +
                                          ( you.blood_rh_factor ? "+" : "-" );
                const row_t rows[] = {
                    {
                        translate_marker( "Strength" ),
                        string_format( "%d (%d)", you.get_str(), you.get_str_base() )
                    },
                    {
                        translate_marker( "Dexterity" ),
                        string_format( "%d (%d)", you.get_dex(), you.get_dex_base() )
                    },
                    {
                        translate_marker( "Intelligence" ),
                        string_format( "%d (%d)", you.get_int(), you.get_int_base() )
                    },
                    {
                        translate_marker( "Perception" ),
                        string_format( "%d (%d)", you.get_per(), you.get_per_base() )
                    },
                    { translate_marker( "Weight" ), display::weight_string( you ) },
                    { translate_marker( "Lifestyle" ), display::health_string( you ) },
                    { translate_marker( "Height" ), you.height_string() },
                    { translate_marker( "Age" ), you.age_string() },
                    { translate_marker( "Blood type" ), blood },
                };
                constexpr unsigned n_rows = sizeof( rows ) / sizeof( rows[0] );
                if( line >= n_rows ) {
                    line = n_rows - 1;
                }
                for( unsigned i = 0; i < n_rows; ++i ) {
                    const bool sel = ( i == line );
                    std::string label = string_format( "%-14s  %s", _( rows[i].label ),
                                                       remove_color_tags( rows[i].value ) );
                    if( ImGui::Selectable( label.c_str(), sel ) ) {
                        line = i;
                    }
                }
            }
            ImGui::EndChild();
            ImGui::SameLine();
            if( ImGui::BeginChild( "##STAT_DETAIL", ImVec2( 0, 0 ), ImGuiChildFlags_Borders ) ) {
                ui_hybrid_chrome::section_header( _( "Details" ) );
                draw_stats_detail( line );
            }
            ImGui::EndChild();
        }

        void draw_stats_detail( unsigned line_idx ) {
            const auto detail_text = []( const std::string & text, const nc_color & color ) {
                cataimgui::draw_colored_text( text, color, ImGui::GetContentRegionAvail().x );
            };
            // Mirrors vanilla draw_stats_info content.
            if( line_idx == 0 ) {
                detail_text(
                    _( "Strength affects your melee damage, the amount of weight you can carry, your total HP, "
                       "your resistance to many diseases, and the effectiveness of actions which require brute force." ),
                    c_magenta );
                detail_text(
                    string_format( _( "Base HP: %d" ),
                                   you.get_part_hp_max( you.get_root_body_part() ) ), c_light_gray );
                detail_text(
                    string_format( _( "Carry weight (%s): %.1f" ), weight_units(),
                                   convert_weight( you.weight_capacity() ) ), c_light_gray );
                detail_text(
                    string_format( _( "Bash damage: %.1f" ), you.bonus_damage( false ) ), c_light_gray );
            } else if( line_idx == 1 ) {
                detail_text(
                    _( "Dexterity affects your chance to hit in melee combat, helps you steady your "
                       "gun for ranged combat, and enhances many actions that require finesse." ),
                    c_magenta );
                detail_text(
                    string_format( _( "Melee to-hit bonus: %+.1lf" ), you.get_melee_hit_base() ),
                    c_light_gray );
                detail_text(
                    string_format( _( "Ranged penalty: %+d" ),
                                   -std::abs( you.ranged_dex_mod() ) ), c_light_gray );
                detail_text(
                    string_format( _( "Throwing penalty per target's dodge: %+d" ),
                                   you.throw_dispersion_per_dodge( false ) ), c_light_gray );
            } else if( line_idx == 2 ) {
                detail_text(
                    _( "Intelligence is less important in most situations, but it is vital for more complex tasks like "
                       "electronics crafting.  It also affects how much skill you can pick up from reading a book." ),
                    c_magenta );
                detail_text(
                    string_format( _( "Read times: %d%%" ), you.read_speed() ), c_light_gray );
                detail_text(
                    string_format( _( "Crafting bonus: %d%%" ), you.get_int() ), c_light_gray );
            } else if( line_idx == 3 ) {
                detail_text(
                    _( "Perception is the most important stat for ranged combat.  It's also used for "
                       "detecting traps and other things of interest." ),
                    c_magenta );
                detail_text(
                    string_format( _( "Trap detection level: %d" ), you.get_per() ), c_light_gray );
                if( you.ranged_per_mod() > 0 ) {
                    detail_text(
                        string_format( _( "Aiming penalty: %+d" ), -you.ranged_per_mod() ),
                        c_light_gray );
                }
            } else if( line_idx == 4 ) {
                detail_text(
                    _( "Your weight is a general indicator of how much fat your body has stored up,"
                       " which in turn shows how prepared you are to survive for a time without food."
                       "  Having too much, or too little, can be unhealthy." ),
                    c_magenta );
                detail_text( display::weight_long_description( you ), c_light_gray );
            } else if( line_idx == 5 ) {
                detail_text(
                    _( "How healthy you feel.  Exercise, vitamins, sleep and not ingesting poison will increase this over time." ),
                    c_magenta );
            } else if( line_idx == 6 ) {
                detail_text( _( "Your height.  Simply how tall you are." ), c_magenta );
                detail_text( you.height_string(), c_light_gray );
            } else if( line_idx == 7 ) {
                detail_text( _( "This is how old you are." ), c_magenta );
                detail_text( you.age_string(), c_light_gray );
            } else if( line_idx == 8 ) {
                detail_text(
                    _( "This is your blood type and Rh factor." ), c_magenta );
                detail_text(
                    string_format( _( "Blood type: %s" ),
                                   io::enum_to_string( you.my_blood_type ) ), c_light_gray );
                detail_text(
                    string_format( _( "Rh factor: %s" ),
                                   you.blood_rh_factor ? _( "positive (+)" ) : _( "negative (-)" ) ),
                    c_light_gray );
            }
        }

        void draw_morale() {
            ui_hybrid_chrome::section_header( _( "MORALE" ) );
            const std::vector<Character::morale_sheet_row> rows = you.get_morale_sheet_rows();
            if( rows.empty() ) {
                ImGui::TextDisabled( "%s", _( "Nothing affects your morale" ) );
                return;
            }
            if( ImGui::BeginTable( "##MORALE_ROWS", 2,
                                   ImGuiTableFlags_SizingStretchProp |
                                   ImGuiTableFlags_RowBg ) ) {
                ImGui::TableSetupColumn( "left", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "right", ImGuiTableColumnFlags_WidthFixed, 80.f );
                for( const Character::morale_sheet_row &r : rows ) {
                    if( r.separator ) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Separator();
                        ImGui::TableNextColumn();
                        ImGui::Separator();
                        continue;
                    }
                    nc_color col = c_white;
                    if( r.favor > 0 ) {
                        col = c_green;
                    } else if( r.favor < 0 ) {
                        col = c_light_red;
                    } else if( r.right.empty() ) {
                        col = c_light_gray;
                    }
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    cataimgui::draw_colored_text( r.left, col );
                    ImGui::TableNextColumn();
                    if( !r.right.empty() ) {
                        cataimgui::draw_colored_text( r.right, col );
                    }
                }
                ImGui::EndTable();
            }
        }

        void draw_bodygraph() {
            ui_hybrid_chrome::section_header( _( "BODY GRAPH" ) );
            if( ImGui::Button( _( "Inspect body parts…" ) ) ) {
                pending_detail = "body";
            }
            static const char *mode_labels[] = {
                translate_marker( "HP" ),
                translate_marker( "Temp" ),
                translate_marker( "Encumbrance" ),
                translate_marker( "Status" ),
                translate_marker( "Wetness" )
            };
            for( int i = 0; i < static_cast<int>( bodygraph_var::last ); ++i ) {
                if( i > 0 ) {
                    ImGui::SameLine();
                }
                const bool active = static_cast<int>( bodygraph_mode ) == i;
                const int n = ui_hybrid_chrome::push_toolbar_button( active );
                if( ImGui::Button( _( mode_labels[i] ) ) ) {
                    bodygraph_mode = static_cast<bodygraph_var>( i );
                }
                ImGui::PopStyleColor( n );
            }
            ImGui::Spacing();
            int height = 0;
            const std::string graph = display::colorized_bodygraph_text(
                                          you, "full_body", bodygraph_mode, 0, 0, height );
            if( graph.empty() ) {
                ImGui::TextDisabled( "%s", _( "Body graph unavailable." ) );
                return;
            }
            cataimgui::PushMonoFont();
            // colorized_bodygraph_text already embeds color tags + newlines.
            cataimgui::draw_colored_text( graph, c_white );
            ImGui::PopFont();
        }

        void draw_customize() {
            ui_hybrid_chrome::section_header( _( "CUSTOMIZE" ) );
            cataimgui::draw_colored_text(
                _( "Debug / chargen identity edits.  Changes apply when you leave this sheet." ),
                c_light_gray );
            ImGui::Spacing();
            ImGui::Text( "%s", string_format( _( "Name: %s" ), you.get_name() ).c_str() );
            ImGui::Text( "%s", string_format( _( "Gender: %s" ),
                                              you.male ? _( "Male" ) : _( "Female" ) ).c_str() );
            ImGui::Spacing();
            {
                const int n = ui_hybrid_chrome::push_toolbar_button( false );
                if( ImGui::Button( _( "Change gender" ), ImVec2( 180.f, 0 ) ) ) {
                    pending_toggle_gender = true;
                }
                ImGui::PopStyleColor( n );
            }
            ImGui::SameLine();
            {
                const int n = ui_hybrid_chrome::push_toolbar_button( false );
                if( ImGui::Button( _( "Change name…" ), ImVec2( 180.f, 0 ) ) ) {
                    pending_name_edit = true;
                }
                ImGui::PopStyleColor( n );
            }
        }

        void draw_health() {
            // Surfaces the same limb HP / bleed / bite / infection / wound / ailment
            // data vanilla Medical UI and look-at body dump already expose.
            ui_hybrid_chrome::section_header( _( "HEALTH" ) );
            const auto pain = display::pain_text_color( you );
            cataimgui::draw_colored_text(
                string_format( _( "Pain: %s" ), pain.first ), pain.second );
            ImGui::Spacing();

            const std::vector<bodypart_id> parts =
                you.get_all_body_parts( get_body_part_flags::only_main );
            if( parts.empty() ) {
                ImGui::TextDisabled( "%s", _( "No body parts." ) );
                return;
            }
            if( line >= parts.size() ) {
                line = static_cast<unsigned>( parts.size() - 1 );
            }

            const float avail = ImGui::GetContentRegionAvail().x;
            const float list_w = avail * 0.42f;
            if( ImGui::BeginChild( "##HP_LIST", ImVec2( list_w, 0 ), ImGuiChildFlags_Borders ) ) {
                ui_hybrid_chrome::section_header( _( "Limbs" ) );
                for( size_t i = 0; i < parts.size(); ++i ) {
                    const bodypart_id &bp = parts[i];
                    const int hp_cur = you.get_part_hp_cur( bp );
                    const int hp_max = you.get_part_hp_max( bp );
                    const nc_color state = display::limb_color( you, bp, true, true, true );
                    std::string flags;
                    if( you.is_limb_broken( bp ) ) {
                        if( you.has_effect( effect_mending, bp ) ) {
                            flags += _( " [splinted]" );
                        } else {
                            flags += _( " [BROKEN]" );
                        }
                    }
                    if( you.get_effect_int( effect_bleed, bp ) > 0 ) {
                        flags += _( " [bleed]" );
                    }
                    if( you.has_effect( effect_bite, bp.id() ) ) {
                        flags += _( " [bite]" );
                    }
                    if( you.has_effect( effect_infected, bp.id() ) ) {
                        flags += _( " [infected]" );
                    }
                    const bodypart *bpp = you.get_part( bp );
                    if( bpp && !bpp->get_wounds().empty() ) {
                        flags += string_format( _( " [%zu wound(s)]" ), bpp->get_wounds().size() );
                    }
                    std::string label = string_format( "%-12s %3d/%-3d%s",
                                                       body_part_name_as_heading( bp, 1 ),
                                                       hp_cur, hp_max, flags );
                    ImGui::PushStyleColor( ImGuiCol_Text, cataimgui::imvec4_from_color(
                                               i == line ? hilite( state ) : state ) );
                    if( ImGui::Selectable( label.c_str(), i == line ) ) {
                        line = static_cast<unsigned>( i );
                    }
                    ImGui::PopStyleColor();
                }
            }
            ImGui::EndChild();
            ImGui::SameLine();
            if( ImGui::BeginChild( "##HP_DETAIL", ImVec2( 0, 0 ), ImGuiChildFlags_Borders ) ) {
                const bodypart_id &bp = parts[line];
                ui_hybrid_chrome::section_header( body_part_name( bp ).c_str() );
                cataimgui::draw_colored_text(
                    string_format( _( "HP: %d / %d" ), you.get_part_hp_cur( bp ),
                                   you.get_part_hp_max( bp ) ),
                    c_white );
                if( you.is_limb_broken( bp ) ) {
                    if( you.has_effect( effect_mending, bp ) ) {
                        const effect &eff = you.get_effect( effect_mending, bp );
                        cataimgui::draw_colored_text(
                            string_format( _( "Broken — mending (%s remaining)" ),
                                           to_string( eff.get_duration() ) ),
                            c_yellow );
                    } else {
                        cataimgui::draw_colored_text( _( "Broken — needs a splint." ), c_red );
                    }
                }

                const int bleed_i = you.get_effect_int( effect_bleed, bp );
                if( bleed_i > 0 ) {
                    const effect bleed_eff = you.get_effect( effect_bleed, bp );
                    cataimgui::draw_colored_text(
                        string_format( "[ %s ] - %s",
                                       bleed_eff.get_speed_name(),
                                       string_format( bleed_eff.disp_short_desc(),
                                                      body_part_name( bp ) ) ),
                        colorize_bleeding_intensity( bleed_i ) );
                }
                if( you.has_effect( effect_bite, bp.id() ) ) {
                    const effect bite_eff = you.get_effect( effect_bite, bp );
                    cataimgui::draw_colored_text(
                        string_format( "[ %s ] - %s",
                                       bite_eff.get_speed_name(),
                                       string_format( bite_eff.disp_short_desc(),
                                                      body_part_name( bp ) ) ),
                        c_yellow );
                }
                if( you.has_effect( effect_infected, bp.id() ) ) {
                    const effect inf_eff = you.get_effect( effect_infected, bp );
                    cataimgui::draw_colored_text(
                        string_format( "[ %s ] - %s",
                                       inf_eff.get_speed_name(),
                                       string_format( inf_eff.disp_short_desc(),
                                                      body_part_name( bp ) ) ),
                        c_pink );
                }
                for( const effect &eff : you.get_effects_from_bp( bp ) ) {
                    if( eff.get_id() == effect_bleed || eff.get_id() == effect_bite ||
                        eff.get_id() == effect_infected || eff.get_id() == effect_mending ) {
                        continue;
                    }
                    if( eff.disp_name().empty() ) {
                        continue;
                    }
                    cataimgui::draw_colored_text(
                        string_format( "[ %s ] - %s",
                                       eff.get_speed_name(),
                                       string_format( eff.disp_short_desc(),
                                                      body_part_name( bp ) ) ),
                        c_light_gray );
                }
                const bodypart *bpp = you.get_part( bp );
                if( bpp ) {
                    const std::vector<wound> &wds = bpp->get_wounds();
                    if( !wds.empty() ) {
                        ui_hybrid_chrome::section_header( _( "Wounds" ) );
                        for( const wound &wd : wds ) {
                            cataimgui::draw_colored_text(
                                string_format( "%s — %s", wd.type->get_name(),
                                               wd.type->get_description() ),
                                c_cyan );
                        }
                    }
                }

                ImGui::Spacing();
                ui_hybrid_chrome::section_header( _( "Ailments" ) );
                // Reuse the same effect/pain/starvation/addiction assembly as Effects tab
                // (vanilla player_display effect list).
                if( effects.empty() ) {
                    ImGui::TextDisabled( "%s", _( "No active ailments." ) );
                } else {
                    for( const auto &e : effects ) {
                        cataimgui::draw_colored_text( e.first, c_light_gray );
                        if( !e.second.empty() ) {
                            ImGui::Indent();
                            cataimgui::draw_colored_text( e.second, c_dark_gray );
                            ImGui::Unindent();
                        }
                    }
                }

                // Vanilla Medical APPLY / CONFIRM — deferred outside Begin/End.
                ImGui::Spacing();
                ImGui::Separator();
                {
                    const int n = ui_hybrid_chrome::push_toolbar_button( false );
                    if( ImGui::Button( _( "Use item…" ), ImVec2( 140.f, 0 ) ) ) {
                        pending_use_item = true;
                    }
                    ImGui::PopStyleColor( n );
                }
                ImGui::SameLine();
                {
                    const int n = ui_hybrid_chrome::push_toolbar_button( false );
                    if( ImGui::Button( _( "Treat…" ), ImVec2( 140.f, 0 ) ) ) {
                        pending_treat = true;
                    }
                    ImGui::PopStyleColor( n );
                }
            }
            ImGui::EndChild();
        }

        void draw_encumbrance() {
            ui_hybrid_chrome::section_header( _( "ENCUMBRANCE AND WARMTH" ) );
            // Pair opposite limbs when encumb/temp/HP match (vanilla list_and_combine_bps).
            std::vector<std::pair<bodypart_id, bool>> bps;
            for( const bodypart_id &bp : you.get_all_body_parts( get_body_part_flags::sorted ) ) {
                const bodypart_id opp = bp->opposite_part.id();
                const bool combine = bp != opp &&
                                     bp == opp->opposite_part && opp == bp->opposite_part &&
                                     you.compare_encumbrance_data( bp, opp ) &&
                                     you.get_part_temp_conv( bp ) == you.get_part_temp_conv( opp ) &&
                                     you.get_part_hp_cur( bp ) == you.get_part_hp_cur( opp );
                if( combine ) {
                    bool already = false;
                    for( const auto &e : bps ) {
                        if( e.first == opp && e.second ) {
                            already = true;
                            break;
                        }
                    }
                    if( !already ) {
                        bps.emplace_back( bp, true );
                    }
                } else {
                    bps.emplace_back( bp, false );
                }
            }
            if( bps.empty() ) {
                ImGui::TextDisabled( "%s", _( "None" ) );
                return;
            }
            if( line >= bps.size() ) {
                line = static_cast<unsigned>( bps.size() - 1 );
            }
            const float avail = ImGui::GetContentRegionAvail().x;
            const float list_w = avail * 0.48f;
            if( ImGui::BeginChild( "##ENC_LIST", ImVec2( list_w, 0 ), ImGuiChildFlags_Borders ) ) {
                for( size_t i = 0; i < bps.size(); ++i ) {
                    const bodypart_id &bp = bps[i].first;
                    const bool combine = bps[i].second;
                    const int enc = you.get_part_encumbrance( bp );
                    const int layer = you.get_part_layer_penalty( bp );
                    const int warmth = static_cast<int>(
                                           ( units::to_legacy_bodypart_temp( you.get_part_temp_conv( bp ) ) / 100.0 ) * 2 - 100 );
                    const nc_color enc_col = display::encumb_color( enc );
                    const nc_color warm_col = display::bodytemp_color( you, bp );
                    std::string label = string_format( "%-12s  ",
                                                       body_part_name_as_heading( bp, combine ? 2 : 1 ) );
                    if( layer > 0 ) {
                        label += string_format( "Enc %d+%d  ", enc - layer, layer );
                    } else {
                        label += string_format( "Enc %d  ", enc );
                    }
                    label += string_format( "(%+d)", warmth );
                    ImGui::PushStyleColor( ImGuiCol_Text, cataimgui::imvec4_from_color(
                                               i == line ? hilite( enc_col ) : enc_col ) );
                    if( ImGui::Selectable( label.c_str(), i == line ) ) {
                        line = static_cast<unsigned>( i );
                    }
                    ImGui::PopStyleColor();
                    ( void )warm_col;
                }
            }
            ImGui::EndChild();
            ImGui::SameLine();
            if( ImGui::BeginChild( "##ENC_DETAIL", ImVec2( 0, 0 ), ImGuiChildFlags_Borders ) ) {
                ui_hybrid_chrome::section_header( _( "Details" ) );
                const bodypart_id &bp = bps[line].first;
                cataimgui::draw_colored_text(
                    string_format( _( "%s — HP %d/%d" ), body_part_name( bp ),
                                   you.get_part_hp_cur( bp ), you.get_part_hp_max( bp ) ),
                    c_white );
                if( !bp->encumb_text.empty() ) {
                    cataimgui::draw_colored_text(
                        string_format( _( "Encumbrance effects: %s" ), bp->encumb_text ),
                        c_magenta );
                }
                const int enc = you.get_part_encumbrance( bp );
                const int layer = you.get_part_layer_penalty( bp );
                cataimgui::draw_colored_text(
                    string_format( _( "Total encumbrance: %d (layering penalty %d)" ), enc, layer ),
                    c_light_gray );
            }
            ImGui::EndChild();
        }

        void draw_speed() {
            ui_hybrid_chrome::section_header( _( "SPEED" ) );
            ImGui::Text( "%s: %d", _( "Base speed" ), you.get_speed() );
            ImGui::Text( "%s: %d", _( "Move cost" ), move_cost );
            ImGui::Spacing();
            ui_hybrid_chrome::section_header( _( "Modifiers" ) );
            if( speed_rows.empty() ) {
                ImGui::TextDisabled( "%s", _( "None" ) );
                return;
            }
            if( line >= speed_rows.size() ) {
                line = speed_rows.size() - 1;
            }
            for( size_t i = 0; i < speed_rows.size(); ++i ) {
                const speed_row &r = speed_rows[i];
                const bool sel = ( i == line );
                std::string label = string_format( "%s  %+d%s", r.description, r.val,
                                                   r.percent ? "%" : "" );
                if( ImGui::Selectable( label.c_str(), sel ) ) {
                    line = static_cast<unsigned>( i );
                }
                if( sel ) {
                    ImGui::Indent();
                    cataimgui::draw_colored_text(
                        string_format( _( "Cause: %s" ), r.description ), c_light_gray );
                    if( r.val != 0 ) {
                        cataimgui::draw_colored_text(
                            string_format( r.val < 0 ? _( "Effect: %1$s reduced by %2$d%3$c" )
                                           : _( "Effect: %1$s increased by %2$d%3$c" ),
                                           r.is_speed ? _( "Speed" ) : _( "Move Cost" ),
                                           std::abs( r.val ), r.percent ? '%' : ' ' ),
                            c_light_gray );
                    }
                    ImGui::Unindent();
                }
            }
        }

        void draw_skills() {
            ui_hybrid_chrome::section_header( _( "SKILLS" ) );
            if( skills.empty() ) {
                ImGui::TextDisabled( "%s", _( "None" ) );
                return;
            }
            if( line >= skills.size() ) {
                line = skills.size() - 1;
            }
            for( size_t i = 0; i < skills.size(); ++i ) {
                if( skills[i].is_header ) {
                    const SkillDisplayType t =
                        SkillDisplayType::get_skill_type( skills[i].skill->display_category() );
                    ui_hybrid_chrome::section_header( t.display_string().c_str() );
                    continue;
                }
                const Skill *aSkill = skills[i].skill;
                const SkillLevel &level = you.get_skill_level_object( aSkill->ident() );
                int exercise = level.knowledgeExperience();
                int level_num = level.knowledgeLevel();
                bool locked = false;
                // CQB bionic locks melee skills at 5 (mirror vanilla player_display).
                static const skill_id cqb_skills[] = {
                    skill_id( "bashing" ), skill_id( "cutting" ), skill_id( "melee" ),
                    skill_id( "stabbing" ), skill_id( "unarmed" )
                };
                if( you.has_active_bionic( bio_cqb ) ) {
                    for( const skill_id &sid : cqb_skills ) {
                        if( aSkill->ident() == sid ) {
                            level_num = 5;
                            exercise = 0;
                            locked = true;
                            break;
                        }
                    }
                }
                level_num = you.enchantment_cache->modify_value( aSkill->ident(), level_num );
                std::string label;
                if( aSkill->ident() == skill_dodge ) {
                    label = string_format( "%s: %4.1f/%-2d(%2d%%)", aSkill->name(),
                                           you.get_dodge(), level_num, std::max( 0, exercise ) );
                } else {
                    label = string_format( "%s: %-2d(%2d%%)", aSkill->name(),
                                           level_num, std::max( 0, exercise ) );
                }
                if( locked ) {
                    label += _( " [CQB]" );
                }
                const bool sel = ( i == line );
                if( ImGui::Selectable( label.c_str(), sel ) ) {
                    line = static_cast<unsigned>( i );
                }
                if( sel ) {
                    ImGui::Indent();
                    cataimgui::draw_colored_text( aSkill->description(), c_light_gray );
                    cataimgui::draw_colored_text(
                        aSkill->get_level_description( level.knowledgeLevel(), false ), c_light_gray );
                    cataimgui::draw_colored_text(
                        aSkill->get_level_description( level.level(), true ), c_light_gray );
                    ImGui::Unindent();
                }
            }
        }

        void draw_traits() {
            ui_hybrid_chrome::section_header( _( "TRAITS" ) );
            if( !traits.empty() && ImGui::Button( _( "Choose appearance variant…" ) ) ) {
                pending_detail = "variant";
            }
            if( traits.empty() ) {
                ImGui::TextDisabled( "%s", _( "None" ) );
                return;
            }
            if( line >= traits.size() ) {
                line = traits.size() - 1;
            }
            const float avail = ImGui::GetContentRegionAvail().x;
            const float list_w = avail * 0.42f;
            if( ImGui::BeginChild( "##TRAIT_LIST", ImVec2( list_w, 0 ), ImGuiChildFlags_Borders ) ) {
                for( size_t i = 0; i < traits.size(); ++i ) {
                    const bool sel = ( i == line );
                    const nc_color col = traits[i].trait->get_display_color();
                    ImGui::PushStyleColor( ImGuiCol_Text, cataimgui::imvec4_from_color(
                                               sel ? hilite( col ) : col ) );
                    if( ImGui::Selectable( traits[i].name().c_str(), sel ) ) {
                        line = static_cast<unsigned>( i );
                    }
                    ImGui::PopStyleColor();
                }
            }
            ImGui::EndChild();
            ImGui::SameLine();
            if( ImGui::BeginChild( "##TRAIT_DETAIL", ImVec2( 0, 0 ), ImGuiChildFlags_Borders ) ) {
                const trait_and_var &cur = traits[line];
                ui_hybrid_chrome::section_header( cur.name().c_str() );
                std::string trait_desc = you.mutation_desc( cur.trait );
                if( trait_desc.empty() ) {
                    trait_desc = cur.desc();
                }
                cataimgui::draw_colored_text( trait_desc, c_light_gray );
                if( !you.purifiable( cur.trait ) ) {
                    cataimgui::draw_colored_text(
                        _( "This trait is an intrinsic part of you now, purifier won't be able to remove it." ),
                        c_yellow );
                }
            }
            ImGui::EndChild();
        }

        void draw_bionics() {
            ui_hybrid_chrome::section_header( _( "BIONICS" ) );
            ImGui::Text( "%s: %d / %d", _( "Power" ),
                         static_cast<int>( units::to_kilojoule( you.get_power_level() ) ),
                         static_cast<int>( units::to_kilojoule( you.get_max_power_level() ) ) );
            ImGui::Spacing();
            if( bionics.empty() ) {
                ImGui::TextDisabled( "%s", _( "None" ) );
                return;
            }
            if( line >= bionics.size() ) {
                line = bionics.size() - 1;
            }
            for( size_t i = 0; i < bionics.size(); ++i ) {
                const bool sel = ( i == line );
                std::string label = bionics[i].name;
                if( bionics[i].count > 1 ) {
                    label = string_format( "%s (%d)", label, bionics[i].count );
                }
                if( ImGui::Selectable( label.c_str(), sel ) ) {
                    line = static_cast<unsigned>( i );
                }
                if( sel ) {
                    ImGui::Indent();
                    cataimgui::draw_colored_text( bionics[i].description, c_light_gray );
                    ImGui::Unindent();
                }
            }
        }

        void draw_effects() {
            ui_hybrid_chrome::section_header( _( "EFFECTS" ) );
            if( effects.empty() ) {
                ImGui::TextDisabled( "%s", _( "None" ) );
                return;
            }
            if( line >= effects.size() ) {
                line = effects.size() - 1;
            }
            for( size_t i = 0; i < effects.size(); ++i ) {
                const bool sel = ( i == line );
                if( ImGui::Selectable( effects[i].first.c_str(), sel ) ) {
                    line = static_cast<unsigned>( i );
                }
                if( sel ) {
                    ImGui::Indent();
                    cataimgui::draw_colored_text( effects[i].second, c_light_gray );
                    ImGui::Unindent();
                }
            }
        }

        void draw_proficiencies() {
            ui_hybrid_chrome::section_header( _( "PROFICIENCIES" ) );
            if( ImGui::Button( _( "Proficiency details…" ) ) ) {
                pending_detail = "proficiency";
            }
            if( profs.empty() ) {
                ImGui::TextDisabled( "%s", _( "None" ) );
                return;
            }
            if( line >= profs.size() ) {
                line = profs.size() - 1;
            }
            for( size_t i = 0; i < profs.size(); ++i ) {
                const display_proficiency &p = profs[i];
                const bool sel = ( i == line );
                std::string label = p.id->name();
                if( !p.known ) {
                    label = string_format( _( "%s (%.0f%%)" ), label, p.practice * 100.0 );
                }
                if( ImGui::Selectable( label.c_str(), sel ) ) {
                    line = static_cast<unsigned>( i );
                }
                if( sel ) {
                    ImGui::Indent();
                    cataimgui::draw_colored_text( p.id->description(), c_light_gray );
                    ImGui::Unindent();
                }
            }
        }
};

} // namespace

void player_display_hybrid( Character &you, bool customize_character )
{
    customize_character |= debug_mode;

    input_context ctxt( "PLAYER_INFO" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "LEFT" );
    ctxt.register_action( "RIGHT" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "SCROLL_UP" );
    ctxt.register_action( "SCROLL_DOWN" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "morale" );
    ctxt.register_action( "VIEW_BODYSTAT" );
    ctxt.register_action( "CHANGE_PROFESSION_NAME" );
    ctxt.register_action( "CHANGE_ARMOR_SPRITE" );
    ctxt.register_action( "SELECT_TRAIT_VARIANT" );
    ctxt.register_action( "VIEW_PROFICIENCIES" );
    ctxt.register_action( "MEDICAL_MENU" );
    ctxt.register_action( "ANY_INPUT" );
    if( customize_character ) {
        ctxt.register_action( "SWITCH_GENDER" );
    }
    ctxt.set_timeout( 10 );

    player_display_hybrid_ui ui( you, customize_character );
    while( !ui.is_done() && ui.get_is_open() ) {
        ui.pump_input( ctxt );
    }
}
