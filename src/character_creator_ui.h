#include <memory>

#include "avatar.h"
#include "cata_imgui.h"
#include "input_context.h"
#include "uilist.h"
#include "scenario.h"

const int CHARACTER_CREATOR_TAB_COUNT = 8;
const int CHARACTER_CREATOR_SUMMARY_LINES = 5;
const ImGuiTableFlags_ CHARACTER_CREATOR_TABLE_FLAGS = ImGuiTableFlags_ScrollY;
const translation CHARACTER_CREATOR_UILIST_ALL = to_translation( "ALL" );
const translation CHARACTER_CREATOR_TRAITS_COSMETIC = to_translation( "COSMETIC" );
const translation CHARACTER_CREATOR_TRAITS_POSITIVE = to_translation( "POSITIVE" );
const translation CHARACTER_CREATOR_TRAITS_NEGATIVE = to_translation( "NEGATIVE" );
const translation CHARACTER_CREATOR_TRAITS_NEUTRAL = to_translation( "NEUTRAL" );

enum character_creator_tab : int {
    CHARCREATOR_SCENARIO,
    CHARCREATOR_PROFESSION,
    CHARCREATOR_BACKGROUND,
    CHARCREATOR_STATS,
    CHARCREATOR_TRAITS,
    CHARCREATOR_SKILLS,
    CHARCREATOR_EQUIPMENT,
    CHARCREATOR_SUMMARY,
    character_creator_tab_LAST
};

// One starting-kit row in the EQUIPMENT chargen tab (editable loadout).
struct character_creator_equipment_choice {
    item it;
    // 0 = wielded, 1 = worn, 2 = inventory (matches auto_wield / armor / else)
    int category = 2;
};

class character_creator_ui;

template<>
struct enum_traits<character_creator_tab> {
    static constexpr character_creator_tab first = character_creator_tab::CHARCREATOR_SCENARIO;
    static constexpr character_creator_tab last = character_creator_tab::character_creator_tab_LAST;
};

// only for handling additional inputs, draws nothing
class character_creator_callback : public uilist_callback
{
        character_creator_ui *ui_parent;
    public:
        character_creator_callback() = default;
        explicit character_creator_callback( character_creator_ui *parent ) : ui_parent( parent ) {};
        // handles any non-uilist (character_creator) inputs
        bool key( const input_context &ctxt, const input_event &event, int, uilist *menu ) override;
        // this override stores and resets uilist's return value so that the uilist doesn't redraw
        void confirm( uilist *menu ) override;
        void select( uilist *menu ) override;
        void refresh( uilist *menu ) override;
};

// never serialized, just to keep state in one place
struct character_creator_uistate {

    std::vector<const scenario *> sorted_scenarios;
    std::vector<profession_id> sorted_professions;
    //TODO: this inventory only exists as an example;
    // it is NOT the inventory used on game start
    std::list<item> cached_profession_inventory;
    // Customized starting kit from the EQUIPMENT tab. When locked and matching
    // the current profession/outfit/gender, add_profession_items() grants this
    // list instead of re-rolling profession item groups. Profession defaults
    // are only the seed; Replace/Add can introduce any non-blacklisted itype.
    // Also serialized into character templates / Last Character as equipment_kit.
    std::vector<character_creator_equipment_choice> equipment_choices;
    profession_id equipment_source_prof = profession_id::NULL_ID();
    bool equipment_source_outfit = true;
    bool equipment_source_male = true;
    // True after EQUIPMENT tab seeds or edits a kit for the current source.
    // Distinguishes "empty customized kit" from "not yet visited".
    bool equipment_locked = false;
    std::vector<profession_id> sorted_hobbies;
    std::vector<trait_id> sorted_traits;
    std::vector<const Skill *> sorted_skills;

    std::string rating_string;
    std::string top_bar_button_action;

    std::vector<bool> sorted_hobbies_taken;

    character_type generation_type = character_type::CUSTOM;

    int selected_profession_index = 0;
    int selected_scenario_index = 0;
    int selected_hobby_index = 0;
    int selected_stat_index = 0;
    int selected_trait_index = 0;
    int selected_skill_index = 0;
    int selected_equipment_index = 0;

    // the currently selected tab
    character_creator_tab selected_tab = CHARCREATOR_SCENARIO;
    // for circumventing ImGui inputs; set when you need to switch the tab by key
    character_creator_tab switched_tab = character_creator_tab_LAST;
    character_creator_tab previous_tab = character_creator_tab_LAST;

    std::array<int, 4> stats = { 8, 8, 8, 8 };

    bool recalc_rating = true;
    bool recalc_scenarios = true;
    bool recalc_professions = true;
    bool recalc_hobbies = true;
    bool recalc_hobbies_taken = true;
    bool recalc_traits = true;
    bool recalc_skills = true;

    bool no_name_entered = false;
    bool scrolled_up = false;
    bool scrolled_down = false;
    bool quit_to_main_menu = false;
    bool finished_character_creator = false;
    bool top_bar_is_open = true;

    void set_initial_tab( character_creator_tab first_tab );

    const scenario *get_selected_scenario();
    profession_id get_selected_profession();
    profession_id get_selected_hobby();
    trait_id get_selected_trait();
    skill_id get_selected_skill();

    void recalc_scenario_list( const avatar &u );
    void recalc_profession_list( const avatar &u );
    void recalc_hobby_list( const avatar &u );
    void recalc_hobbies_taken_list( const avatar &u );
    void recalc_trait_list( const avatar &u );
    void recalc_skill_list();

    // Seed equipment_choices from the profession kit when unlocked/stale.
    // Changing profession/outfit/gender/traits clears the lock and reseeds.
    void ensure_equipment_pool( const avatar &u );
    void clear_equipment_customization();
    // True when EQUIPMENT tab has locked a kit for the current prof/outfit/gender.
    bool has_custom_starting_equipment( const avatar &u ) const;
    std::list<item> custom_starting_items() const;

    bool hobby_conflict_check( const avatar &u );
    void reset();
};

// One window with an embedded selection list and independently scrolling details.
class character_creator_ui
{
        character_creator_callback cc_callback;
        std::array<std::shared_ptr<uilist>, CHARACTER_CREATOR_TAB_COUNT> cc_uilist;
        std::array<input_context, CHARACTER_CREATOR_TAB_COUNT> cc_inputs;

    public:
        character_creator_ui();
        // returns true if character creation completed successfully
        bool display();
        // setup new uilist if necessary, assumes current tab has been set
        void setup_new_uilist();
        // setup the avatar for switching to a different tab
        void setup_avatar();
        // update uilist_entries for current tab, but do not modify uilist_entry count
        void update_uilist_entries();
        void upon_switching_tab();
        // @param quick_value_change - if true, add inputs for changing a value (stat or skill)
        void setup_input_context( input_context &cc_ictxt, bool quick_value_change );
        // @return action was handled
        bool handle_action( const std::string &action );

        std::shared_ptr<uilist> get_current_tab_uilist();
        std::shared_ptr<uilist> get_tab_uilist( character_creator_tab tab );
        void set_current_tab_uilist( const std::shared_ptr<uilist> &new_uilist );
        input_context &get_current_tab_input();
        void set_current_tab_input( const input_context &new_input );
};

class character_creator_ui_impl : public cataimgui::window
{
    public:
        character_creator_ui *ui_parent;
        explicit character_creator_ui_impl( character_creator_ui *parent );

        void draw_top_bar( const avatar &u ) const;
        void draw_scenarios() const;
        void draw_professions() const;
        void draw_backgrounds();
        // STR, DEX, etc...
        void draw_stats();
        void draw_traits();
        void draw_skills();
        void draw_equipment();
        void draw_summary();
        void setup_list_detail_ui( const std::string &header = std::string(),
                                   float uilist_width = 0.33f ) const;

    protected:
        void draw_controls() override;
        cataimgui::bounds get_bounds() override;
};

namespace char_creation
{
// an ImGui button that, when clicked, sends an action to cc_uistate
void draw_action_button( const std::string &button_text, const std::string &button_action );
void draw_name( const avatar &you, bool no_name_entered );
void draw_gender( const avatar &you );
void draw_outfit();
void draw_height( const Character &you );
void draw_age( const Character &you );
void draw_blood( const Character &you );
void draw_scenario_profession( const avatar &you );
void draw_time_cataclysm_start();
void draw_time_game_start();
void draw_location( const avatar &you );
void draw_starting_city( const avatar &you );
std::string get_gender_string( bool male );

const mutation_variant *variant_trait_selection_menu( const trait_id &cur_trait );

void draw_character_stats( const Character &who, bool draw_effective_stats );
void draw_character_skills( const Character &who );
void draw_starting_vehicle( const Character &who );
void draw_character_proficiencies( const Character &who );
void draw_character_traits( const Character &who );
void draw_stat_details( avatar &u );
std::string stat_level_description( int stat_value );
std::string get_character_stat_header( int selected_stat_index );
void draw_profession_header( const avatar &u );
void draw_header( bool &drawn_anything, const std::string &title );
void draw_profession_addictions( bool &drawn_anything, const std::string &header,
                                 const profession &selected_profession );
void draw_profession_traits( bool &drawn_anything, const std::string &header,
                             const profession &selected_profession );
void draw_profession_proficiencies( bool &drawn_anything, const std::string &header,
                                    const profession &selected_profession );
void draw_profession_bionics( bool &drawn_anything, const std::string &header,
                              const profession &selected_profession );
void draw_profession_spells( bool &drawn_anything, const std::string &header,
                             const profession &selected_profession );
void draw_profession_missions( bool &drawn_anything, const std::string &header,
                               const profession &selected_profession );
void draw_profession_details();
void draw_profession_inventory_items( const std::string &category,
                                      const std::vector<std::string> &item_names );
void draw_profession_inventory( const avatar &u );
void draw_equipment_details( const avatar &u );
void draw_hobby_header( const avatar &u );
void draw_hobby_details();
void draw_character_craft( const avatar &u );
void draw_hobby_selected( const avatar &u );
void draw_skill_details( const avatar &u,
                         const std::map<skill_id, int> &prof_skills, skill_id currentSkill );
void draw_scenario_details( const avatar &u );
} // namespace char_creation
