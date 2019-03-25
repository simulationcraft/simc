//==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "player/pet_spawner.hpp"

namespace
{ // UNNAMED NAMESPACE

// helper smartpointer-like struct for spell data pointers
struct spell_data_ptr_t
{
  spell_data_ptr_t():
    data_( spell_data_t::not_found() ) {}

  spell_data_ptr_t( const spell_data_t* s ):
    data_( s ? s : spell_data_t::not_found() ) {}

  spell_data_ptr_t& operator=( const spell_data_t* s )
  {
    data_ = s ? s : spell_data_t::not_found();
    return *this;
  }

  const spell_data_t* operator->() const { return data_; }

  operator const spell_data_t*() const { return data_; }

  const spell_data_t* data_;
};

void parse_affecting_aura( action_t *const action, const spell_data_t *const spell )
{
  for ( size_t i = 1; i <= spell -> effect_count(); i++ )
  {
    const spelleffect_data_t& effect = spell -> effectN( i );
    if ( ! effect.ok() || effect.type() != E_APPLY_AURA )
      continue;

    if ( action -> data().affected_by( effect ) )
    {
      switch ( effect.subtype() )
      {
      case A_HASTED_GCD:
        action -> gcd_haste = HASTE_ATTACK;
        break;

      case A_HASTED_COOLDOWN:
        action -> cooldown -> hasted = true;
        break;

      case A_ADD_FLAT_MODIFIER:
        switch( effect.misc_value1() )
        {
        case P_RESOURCE_COST:
          action -> base_costs[ RESOURCE_FOCUS ] += effect.base_value();
          break;

        default: break;
        }
        break;

      case A_ADD_PCT_MODIFIER:
        switch ( effect.misc_value1() )
        {
        case P_GENERIC:
          action -> base_dd_multiplier *= 1.0 + effect.percent();
          break;

        case P_TICK_DAMAGE:
          action -> base_td_multiplier *= 1.0 + effect.percent();
          break;

        default: break;
        }
        break;

      default: break;
      }
    }
    else if ( action -> data().category() == as<unsigned>(effect.misc_value1()) )
    {
      switch ( effect.subtype() )
      {
      case A_HASTED_CATEGORY:
        action -> cooldown -> hasted = true;
        break;

      default: break;
      }
    }
  }
}

namespace cdwaste {

struct action_data_t
{
  simple_sample_data_with_min_max_t exec;
  simple_sample_data_with_min_max_t cumulative;
  timespan_t iter_sum;

  void update_ready( const action_t* action, timespan_t cd )
  {
    const cooldown_t* cooldown = action -> cooldown;
    sim_t* sim = action -> sim;
    if ( ( cd > 0_ms || ( cd <= 0_ms && cooldown -> duration > 0_ms ) ) &&
         cooldown -> current_charge == cooldown -> charges && cooldown -> last_charged > 0_ms &&
         cooldown -> last_charged < sim -> current_time() )
    {
      timespan_t time_ = sim -> current_time() - cooldown -> last_charged;
      if ( sim -> debug )
      {
        sim -> out_debug.print( "{} {} cooldown waste tracking waste={} exec_time={}",
                                action -> player -> name(), action -> name(),
                                time_, action -> time_to_execute );
      }
      time_ -= action -> time_to_execute;

      if ( time_ > 0_ms )
      {
        exec.add( time_.total_seconds() );
        iter_sum += time_;
      }
    }
  }
};

struct player_data_t
{
  typedef std::pair<std::string, std::unique_ptr<action_data_t>> record_t;
  std::vector<record_t> data_;

  action_data_t* get( const action_t* a )
  {
    auto it = range::find_if( data_, [ a ] ( const record_t& r ) { return a -> name_str == r.first; } );
    if ( it != data_.cend() )
      return it -> second.get();

    data_.push_back( record_t( a -> name_str, std::unique_ptr<action_data_t>( new action_data_t() ) ) );
    return data_.back().second.get();
  }

  void merge( const player_data_t& other )
  {
    for ( size_t i = 0, end = data_.size(); i < end; i++ )
    {
      data_[ i ].second -> exec.merge( other.data_[ i ].second -> exec );
      data_[ i ].second -> cumulative.merge( other.data_[ i ].second -> cumulative );
    }
  }

  void datacollection_begin()
  {
    for ( auto& rec : data_ )
      rec.second -> iter_sum = 0_ms;
  }

  void datacollection_end()
  {
    for ( auto& rec : data_ )
      rec.second -> cumulative.add( rec.second -> iter_sum.total_seconds() );
  }
};

void print_html_report( const player_t& player, const player_data_t& data, report::sc_html_stream& os )
{
  if ( data.data_.empty() )
    return;

  os << "\t\t\t\t\t<h3 class=\"toggle open\">Cooldown waste details</h3>\n"
     << "\t\t\t\t\t<div class=\"toggle-content\">\n";

  os << "<table class=\"sc\" style=\"float: left;margin-right: 10px;\">\n"
     << "<tr>\n"
     << "<th></th>\n"
     << "<th colspan=\"3\">Seconds per Execute</th>\n"
     << "<th colspan=\"3\">Seconds per Iteration</th>\n"
     << "</tr>\n"
     << "<tr>\n"
     << "<th>Ability</th>\n"
     << "<th>Average</th>\n"
     << "<th>Minimum</th>\n"
     << "<th>Maximum</th>\n"
     << "<th>Average</th>\n"
     << "<th>Minimum</th>\n"
     << "<th>Maximum</th>\n"
     << "</tr>\n";

  size_t n = 0;
  for ( const auto& rec : data.data_ )
  {
    const auto& entry = rec.second -> exec;
    if ( entry.count() == 0 )
      continue;

    const auto& iter_entry = rec.second -> cumulative;

    action_t* a = player.find_action( rec.first );
    std::string name_str = rec.first;
    if ( a )
      name_str = report::action_decorator_t( a ).decorate();

    std::string row_class_str = "";
    if ( ++n & 1 )
      row_class_str = " class=\"odd\"";

    os.printf( "<tr%s>", row_class_str.c_str() );
    os << "<td class=\"left\">" << name_str << "</td>";
    os.printf( "<td class=\"right\">%.3f</td>", entry.mean() );
    os.printf( "<td class=\"right\">%.3f</td>", entry.min() );
    os.printf( "<td class=\"right\">%.3f</td>", entry.max() );
    os.printf( "<td class=\"right\">%.3f</td>", iter_entry.mean() );
    os.printf( "<td class=\"right\">%.3f</td>", iter_entry.min() );
    os.printf( "<td class=\"right\">%.3f</td>", iter_entry.max() );
    os << "</tr>\n";
  }

  os << "</table>\n";

  os << "\t\t\t\t\t</div>\n";

  os << "<div class=\"clear\"></div>\n";
}

} // namespace cd_waste

// ==========================================================================
// Hunter
// ==========================================================================

/* BfA TODO
 *  Survival
 *   - movement support for proper Harpoon & Terms of Engagement use
 */

// somewhat arbitrary number of the maximum count of barbed shot buffs possible simultaneously
constexpr unsigned BARBED_SHOT_BUFFS_MAX = 10;

// different types of Wildfire Infusion bombs
enum wildfire_infusion_e {
  WILDFIRE_INFUSION_SHRAPNEL = 0,
  WILDFIRE_INFUSION_PHEROMONE,
  WILDFIRE_INFUSION_VOLATILE,
};

struct hunter_t;

namespace pets
{
struct hunter_main_pet_base_t;
struct hunter_main_pet_t;
struct dire_critter_t;
}

struct hunter_td_t: public actor_target_data_t
{
  bool damaged = false;

  struct debuffs_t
  {
    buff_t* hunters_mark;
    buff_t* latent_poison;
    buff_t* steady_aim;
  } debuffs;

  struct dots_t
  {
    dot_t* serpent_sting;
    dot_t* a_murder_of_crows;
    dot_t* pheromone_bomb;
    dot_t* shrapnel_bomb;
  } dots;

  hunter_td_t( player_t* target, hunter_t* p );

  void target_demise();
};

struct hunter_t: public player_t
{
public:

  struct pets_t
  {
    pets::hunter_main_pet_t* main = nullptr;
    pets::hunter_main_pet_base_t* animal_companion = nullptr;
    pets::dire_critter_t* dire_beast = nullptr;
    pet_t* spitting_cobra = nullptr;
    spawner::pet_spawner_t<pets::dire_critter_t, hunter_t> dc_dire_beast;

    pets_t( hunter_t* p ) : dc_dire_beast( "dire_beast_(dc)", p ) {}
  } pets;

  struct azerite_t
  {
    // Beast Mastery
    azerite_power_t dance_of_death;
    azerite_power_t dire_consequences;
    azerite_power_t feeding_frenzy;
    azerite_power_t haze_of_rage;
    azerite_power_t primal_instincts;
    azerite_power_t serrated_jaws;
    // Marksmanship
    azerite_power_t focused_fire;
    azerite_power_t in_the_rhythm;
    azerite_power_t rapid_reload;
    azerite_power_t steady_aim;
    azerite_power_t surging_shots;
    azerite_power_t unerring_vision;
    // Survival
    azerite_power_t blur_of_talons;
    azerite_power_t latent_poison;
    azerite_power_t primeval_intuition;
    azerite_power_t venomous_fangs;
    azerite_power_t wilderness_survival;
    azerite_power_t wildfire_cluster;
  } azerite;

  // Buffs
  struct buffs_t
  {
    // Beast Mastery
    buff_t* aspect_of_the_wild;
    buff_t* bestial_wrath;
    std::array<buff_t*, BARBED_SHOT_BUFFS_MAX> barbed_shot;
    buff_t* dire_beast;
    buff_t* thrill_of_the_hunt;
    buff_t* spitting_cobra;

    // Marksmanship
    buff_t* steady_focus;
    buff_t* lock_and_load;
    buff_t* precise_shots;
    buff_t* trick_shots;
    buff_t* trueshot;
    buff_t* master_marksman;
    buff_t* double_tap;

    // Survival
    buff_t* coordinated_assault;
    buff_t* vipers_venom;
    buff_t* tip_of_the_spear;
    buff_t* mongoose_fury;
    buff_t* predator;
    buff_t* terms_of_engagement;
    buff_t* aspect_of_the_eagle;

    // azerite
    buff_t* blur_of_talons;
    buff_t* dance_of_death;
    buff_t* haze_of_rage;
    buff_t* in_the_rhythm;
    buff_t* primal_instincts;
    buff_t* primeval_intuition;
    buff_t* unerring_vision_driver;
    buff_t* unerring_vision;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* bestial_wrath;
    cooldown_t* trueshot;
    cooldown_t* barbed_shot;
    cooldown_t* kill_command;
    cooldown_t* aspect_of_the_wild;
    cooldown_t* a_murder_of_crows;
    cooldown_t* aimed_shot;
    cooldown_t* rapid_fire;
    cooldown_t* harpoon;
    cooldown_t* wildfire_bomb;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* barbed_shot;
    gain_t* aspect_of_the_wild;
    gain_t* spitting_cobra;
    gain_t* hunters_mark;
    gain_t* terms_of_engagement;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* wild_call;
    proc_t* lethal_shots;
  } procs;

  // Talents
  struct talents_t
  {
    // tier 15
    spell_data_ptr_t killer_instinct;
    spell_data_ptr_t animal_companion;
    spell_data_ptr_t dire_beast;

    spell_data_ptr_t master_marksman;
    spell_data_ptr_t serpent_sting;

    spell_data_ptr_t vipers_venom;
    spell_data_ptr_t terms_of_engagement;
    spell_data_ptr_t alpha_predator;

    // tier 30
    spell_data_ptr_t scent_of_blood;
    spell_data_ptr_t one_with_the_pack;
    spell_data_ptr_t chimaera_shot;

    spell_data_ptr_t careful_aim;
    spell_data_ptr_t volley;
    spell_data_ptr_t explosive_shot;

    spell_data_ptr_t guerrilla_tactics;
    spell_data_ptr_t hydras_bite;
    spell_data_ptr_t butchery;

    // tier 45
    spell_data_ptr_t trailblazer;
    spell_data_ptr_t natural_mending;
    spell_data_ptr_t camouflage;

    // tier 60
    spell_data_ptr_t venomous_bite;
    spell_data_ptr_t thrill_of_the_hunt;
    spell_data_ptr_t a_murder_of_crows;

    spell_data_ptr_t steady_focus;
    spell_data_ptr_t streamline;
    spell_data_ptr_t hunters_mark;

    spell_data_ptr_t bloodseeker;
    spell_data_ptr_t steel_trap;

    // tier 75
    spell_data_ptr_t born_to_be_wild;
    spell_data_ptr_t posthaste;
    spell_data_ptr_t binding_shot;

    // tier 90
    spell_data_ptr_t stomp;
    spell_data_ptr_t barrage;
    spell_data_ptr_t stampede;

    spell_data_ptr_t lethal_shots;
    spell_data_ptr_t double_tap;

    spell_data_ptr_t tip_of_the_spear;
    spell_data_ptr_t mongoose_bite;
    spell_data_ptr_t flanking_strike;

    // tier 100
    spell_data_ptr_t killer_cobra;
    spell_data_ptr_t aspect_of_the_beast;
    spell_data_ptr_t spitting_cobra;

    spell_data_ptr_t calling_the_shots;
    spell_data_ptr_t lock_and_load;
    spell_data_ptr_t piercing_shot;

    spell_data_ptr_t birds_of_prey;
    spell_data_ptr_t wildfire_infusion;
    spell_data_ptr_t chakrams;
  } talents;

  // Specialization Spells
  struct specs_t
  {
    spell_data_ptr_t critical_strikes;
    spell_data_ptr_t hunter;
    spell_data_ptr_t beast_mastery_hunter;
    spell_data_ptr_t marksmanship_hunter;
    spell_data_ptr_t survival_hunter;

    // Beast Mastery
    spell_data_ptr_t aspect_of_the_wild;
    spell_data_ptr_t barbed_shot;
    spell_data_ptr_t beast_cleave;
    spell_data_ptr_t bestial_wrath;
    spell_data_ptr_t kill_command;
    spell_data_ptr_t kindred_spirits;
    spell_data_ptr_t wild_call;

    // Marksmanship
    spell_data_ptr_t aimed_shot;
    spell_data_ptr_t lone_wolf;
    spell_data_ptr_t precise_shots;
    spell_data_ptr_t rapid_fire;
    spell_data_ptr_t trick_shots;
    spell_data_ptr_t trueshot;

    // Survival
    spell_data_ptr_t coordinated_assault;
    spell_data_ptr_t harpoon;
    spell_data_ptr_t raptor_strike;
    spell_data_ptr_t wildfire_bomb;
    spell_data_ptr_t carve;
    spell_data_ptr_t aspect_of_the_eagle;
  } specs;

  struct mastery_spells_t
  {
    spell_data_ptr_t master_of_beasts;
    spell_data_ptr_t sniper_training;
    spell_data_ptr_t spirit_bond;
  } mastery;

  cdwaste::player_data_t cd_waste;

  player_t* current_hunters_mark_target;
  wildfire_infusion_e next_wi_bomb = WILDFIRE_INFUSION_SHRAPNEL;

  struct options_t {
    std::string summon_pet_str = "cat";
    timespan_t pet_attack_speed = 2.0_s;
    timespan_t pet_basic_attack_delay = 0.15_s;
  } options;

  hunter_t( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) :
    player_t( sim, HUNTER, name, r ),
    pets( this ),
    buffs(),
    cooldowns(),
    gains(),
    procs(),
    talents(),
    specs(),
    mastery(),
    current_hunters_mark_target( nullptr )
  {
    // Cooldowns
    cooldowns.bestial_wrath       = get_cooldown( "bestial_wrath" );
    cooldowns.trueshot            = get_cooldown( "trueshot" );
    cooldowns.barbed_shot         = get_cooldown( "barbed_shot" );
    cooldowns.kill_command        = get_cooldown( "kill_command" );
    cooldowns.harpoon             = get_cooldown( "harpoon" );
    cooldowns.aspect_of_the_wild  = get_cooldown( "aspect_of_the_wild" );
    cooldowns.a_murder_of_crows   = get_cooldown( "a_murder_of_crows" );
    cooldowns.aimed_shot          = get_cooldown( "aimed_shot" );
    cooldowns.rapid_fire          = get_cooldown( "rapid_fire" );
    cooldowns.wildfire_bomb       = get_cooldown( "wildfire_bomb" );

    base_gcd = 1.5_s;

    regen_type = REGEN_DYNAMIC;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }

  // Character Definition
  void      init_spells() override;
  void      init_base_stats() override;
  void      create_buffs() override;
  void      init_special_effects() override;
  void      init_gains() override;
  void      init_position() override;
  void      init_procs() override;
  void      init_rng() override;
  void      init_scaling() override;
  void      init_assessors() override;
  void      init_action_list() override;
  void      reset() override;
  void      merge( player_t& other ) override;
  void      combat_begin() override;

  void datacollection_begin() override;
  void datacollection_end() override;

  double    composite_attack_power_multiplier() const override;
  double    composite_melee_crit_chance() const override;
  double    composite_spell_crit_chance() const override;
  double    composite_melee_haste() const override;
  double    composite_melee_speed() const override;
  double    composite_spell_haste() const override;
  double    composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  double    composite_player_multiplier( school_e school ) const override;
  double    composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double    composite_player_pet_damage_multiplier( const action_state_t* ) const override;
  double    matching_gear_multiplier( attribute_e attr ) const override;
  void      invalidate_cache( cache_e ) override;
  void      regen( timespan_t periodicity ) override;
  void      create_options() override;
  expr_t*   create_expression( const std::string& name ) override;
  expr_t*     create_action_expression( action_t&, const std::string& name ) override;
  action_t* create_action( const std::string& name, const std::string& options ) override;
  pet_t*    create_pet( const std::string& name, const std::string& type = std::string() ) override;
  void      create_pets() override;
  resource_e primary_resource() const override { return RESOURCE_FOCUS; }
  role_e    primary_role() const override { return ROLE_ATTACK; }
  stat_e    convert_hybrid_stat( stat_e s ) const override;
  std::string      create_profile( save_e ) override;
  void      copy_from( player_t* source ) override;

  void      moving( ) override;

  void              apl_default();
  void              apl_surv();
  void              apl_bm();
  void              apl_mm();
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;

  std::string special_use_item_action( const std::string& item_name, const std::string& condition = std::string() ) const;

  target_specific_t<hunter_td_t> target_data;

  hunter_td_t* get_target_data( player_t* target ) const override
  {
    hunter_td_t*& td = target_data[target];
    if ( !td ) td = new hunter_td_t( target, const_cast<hunter_t*>( this ) );
    return td;
  }

  const hunter_td_t* find_target_data( const player_t* target ) const
  {
    return target_data[ target ];
  }

  std::vector<action_t*> background_actions;

  template <typename T, typename... Ts>
  T* get_background_action( const std::string& n, Ts&&... args )
  {
    auto it = range::find_if( background_actions, [ &n ]( action_t* a ) { return a -> name_str == n; } );
    if ( it != background_actions.cend() )
      return dynamic_cast<T*>( *it );

    auto action = new T( n, this, std::forward<Ts>( args )... );
    action -> background = true;
    background_actions.push_back( action );
    return action;
  }
};

// Template for common hunter action code.
template <class Base>
struct hunter_action_t: public Base
{
private:
  typedef Base ab;
public:

  bool precombat = false;
  bool track_cd_waste;
  cdwaste::action_data_t* cd_waste;

  struct {
    // bm
    bool aotw_crit_chance;
    bool aotw_gcd_reduce;
    bool bestial_wrath;
    bool master_of_beasts;
    bool thrill_of_the_hunt;
    // mm
    bool lone_wolf;
    bool sniper_training;
    // surv
    bool spirit_bond;
    bool coordinated_assault;
  } affected_by;

  hunter_action_t( const std::string& n, hunter_t* player, const spell_data_t* s = spell_data_t::nil() ):
    ab( n, player, s ),
    track_cd_waste( s -> cooldown() > 0_ms || s -> charge_cooldown() > 0_ms ),
    cd_waste( nullptr ),
    affected_by()
  {
    ab::special = true;
    ab::gcd_haste = HASTE_NONE;
    ab::may_crit = true;
    ab::tick_may_crit = true;

    parse_affecting_aura( this, p() -> specs.hunter );
    parse_affecting_aura( this, p() -> specs.beast_mastery_hunter );
    parse_affecting_aura( this, p() -> specs.marksmanship_hunter );
    parse_affecting_aura( this, p() -> specs.survival_hunter );

    affected_by.aotw_crit_chance = ab::data().affected_by( p() -> specs.aspect_of_the_wild -> effectN( 1 ) );
    affected_by.aotw_gcd_reduce = ab::data().affected_by( p() -> specs.aspect_of_the_wild -> effectN( 3 ) );
    affected_by.bestial_wrath = ab::data().affected_by( p() -> specs.bestial_wrath -> effectN( 1 ) );
    affected_by.master_of_beasts = ab::data().affected_by( p() -> mastery.master_of_beasts -> effectN( 3 ) );
    affected_by.thrill_of_the_hunt = ab::data().affected_by( p() -> talents.thrill_of_the_hunt -> effectN( 1 ).trigger() -> effectN( 1 ) );

    affected_by.lone_wolf = ab::data().affected_by( p() -> specs.lone_wolf -> effectN( 1 ) );
    affected_by.sniper_training = ab::data().affected_by( p() -> mastery.sniper_training -> effectN( 2 ) );

    affected_by.spirit_bond = ab::data().affected_by( p() -> mastery.spirit_bond -> effectN( 1 ) );
    affected_by.coordinated_assault = ab::data().affected_by( p() -> specs.coordinated_assault -> effectN( 1 ) );
  }

  hunter_t* p()             { return static_cast<hunter_t*>( ab::player ); }
  const hunter_t* p() const { return static_cast<hunter_t*>( ab::player ); }

  hunter_td_t* td( player_t* t ) { return p() -> get_target_data( t ); }
  const hunter_td_t* td( player_t* t ) const { return p() -> get_target_data( t ); }
  const hunter_td_t* find_td( const player_t* t ) const { return p() -> find_target_data( t ); }

  void init() override
  {
    ab::init();

    if ( track_cd_waste )
      cd_waste = p() -> cd_waste.get( this );
  }

  void init_finished() override
  {
    ab::init_finished();

    if( ab::action_list )
      precombat = ab::action_list -> name_str == "precombat";
  }

  timespan_t gcd() const override
  {
    timespan_t g = ab::gcd();

    if ( g == 0_ms )
      return g;

    // recalculate the gcd while under the effect of AotW as it applies before haste reduction
    if ( affected_by.aotw_gcd_reduce && p() -> buffs.aspect_of_the_wild -> check() )
    {
      g = ab::trigger_gcd;
      g += p() -> specs.aspect_of_the_wild -> effectN( 3 ).time_value();
      if ( ab::gcd_haste != HASTE_NONE )
        g *= ab::composite_haste();
    }

    if ( g < ab::min_gcd )
      g = ab::min_gcd;

    return g;
  }

  double action_multiplier() const override
  {
    double am = ab::action_multiplier();

    if ( affected_by.bestial_wrath )
      am *= 1.0 + p() -> buffs.bestial_wrath -> check_value();

    if ( affected_by.coordinated_assault )
      am *= 1.0 + p() -> buffs.coordinated_assault -> check_value();

    if ( affected_by.master_of_beasts )
      am *= 1.0 + p() -> cache.mastery() * p() -> mastery.master_of_beasts -> effectN( 3 ).mastery_value();

    if ( affected_by.sniper_training )
      am *= 1.0 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( 2 ).mastery_value();

    if ( affected_by.spirit_bond )
      am *= 1.0 + p() -> cache.mastery() * p() -> mastery.spirit_bond -> effectN( 1 ).mastery_value();

    if ( affected_by.lone_wolf && p() -> pets.main == nullptr )
      am *= 1.0 + p() -> specs.lone_wolf -> effectN( 1 ).percent();

    return am;
  }

  double composite_crit_chance() const override
  {
    double cc = ab::composite_crit_chance();

    if ( affected_by.aotw_crit_chance )
      cc += p() -> buffs.aspect_of_the_wild -> check_value();

    if ( affected_by.thrill_of_the_hunt )
      cc += p() -> buffs.thrill_of_the_hunt -> check_stack_value();

    return cc;
  }

  void update_ready( timespan_t cd ) override
  {
    if ( cd_waste )
      cd_waste -> update_ready( this, cd );

    ab::update_ready( cd );
  }

  virtual double cast_regen( const action_state_t* s ) const
  {
    const timespan_t cast_time = std::max( this -> execute_time(), this -> gcd() );
    const double regen = p() -> resource_regen_per_second( RESOURCE_FOCUS );
    size_t targets_hit = 1;
    if ( this -> energize_type_() == ENERGIZE_PER_HIT && ab::is_aoe() )
    {
      size_t tl_size = this -> target_list().size();
      int num_targets = this -> n_targets();
      targets_hit = ( num_targets < 0 ) ? tl_size : std::min( tl_size, as<size_t>( num_targets ) );
    }
    return ( regen * cast_time.total_seconds() ) +
           ( targets_hit * this -> composite_energize_amount( s ) );
  }

  // action list expressions
  expr_t* create_expression( const std::string& name ) override
  {
    if ( util::str_compare_ci( name, "cast_regen" ) )
    {
      // Return the focus that will be regenerated during the cast time or GCD of the target action.
      struct cast_regen_expr_t : public expr_t
      {
        hunter_action_t& action;
        std::unique_ptr<action_state_t> state;
        cast_regen_expr_t( hunter_action_t& a ):
          expr_t( "cast_regen" ), action( a ), state( a.get_state() )
        {}

        double evaluate() override
        {
          action.snapshot_state( state.get(), RESULT_TYPE_NONE );
          state -> target = action.target;
          return action.cast_regen( state.get() );
        }
      };
      return new cast_regen_expr_t( *this );
    }

    // fudge wildfire bomb dot name
    std::vector<std::string> splits = util::string_split( name, "." );
    if ( splits.size() == 3 && splits[ 0 ] == "dot" && splits[ 1 ] == "wildfire_bomb" )
      return ab::create_expression( "dot.wildfire_bomb_dot." + splits[ 2 ] );

    return ab::create_expression( name );
  }

  virtual void try_steady_focus()
  {
    if ( !ab::background && p() -> talents.steady_focus -> ok() )
      p() -> buffs.steady_focus -> expire();
  }

  void add_pet_stats( pet_t* pet, std::initializer_list<std::string> names )
  {
    if ( ! pet )
      return;

    for ( const auto& n : names )
    {
      stats_t* s = pet -> find_stats( n );
      if ( s )
        ab::stats -> add_child( s );
    }
  }

  bool trigger_buff( buff_t *const buff, timespan_t precast_time ) const
  {
    const bool in_combat = ab::player -> in_combat;
    const bool triggered = buff -> trigger();
    if ( triggered && precombat && !in_combat && precast_time > 0_ms )
    {
      buff -> extend_duration( ab::player, -std::min( precast_time, buff -> buff_duration ) );
      buff -> cooldown -> adjust( -precast_time );
    }
    return triggered;
  }

  void adjust_precast_cooldown( timespan_t precast_time ) const
  {
    const bool in_combat = ab::player -> in_combat;
    if ( precombat && !in_combat && precast_time > 0_ms )
      ab::cooldown -> adjust( -precast_time );
  }
};

void trigger_bloodseeker_update( hunter_t* );

struct hunter_ranged_attack_t: public hunter_action_t < ranged_attack_t >
{
  hunter_ranged_attack_t( const std::string& n, hunter_t* player,
                          const spell_data_t* s = spell_data_t::nil() ):
                          hunter_action_t( n, player, s )
  {}

  bool usable_moving() const override
  { return true; }

  void execute() override
  {
    hunter_action_t::execute();
    try_steady_focus();
  }
};

struct hunter_melee_attack_t: public hunter_action_t < melee_attack_t >
{
  hunter_melee_attack_t( const std::string& n, hunter_t* p,
                          const spell_data_t* s = spell_data_t::nil() ):
                          hunter_action_t( n, p, s )
  {}

  void init()
  {
    hunter_action_t::init();

    if ( weapon && weapon -> group() != WEAPON_2H )
      background = true;
  }
};

struct hunter_spell_t: public hunter_action_t < spell_t >
{
public:
  hunter_spell_t( const std::string& n, hunter_t* player,
                  const spell_data_t* s = spell_data_t::nil() ):
                  hunter_action_t( n, player, s )
  {}

  void execute() override
  {
    hunter_action_t::execute();
    try_steady_focus();
  }
};

namespace pets
{
// ==========================================================================
// Hunter Pet
// ==========================================================================

struct hunter_pet_t: public pet_t
{
  hunter_pet_t( hunter_t* owner, const std::string& pet_name, pet_e pt = PET_HUNTER, bool guardian = false, bool dynamic = false ) :
    pet_t( owner -> sim, owner, pet_name, pt, guardian, dynamic )
  {
    owner_coeff.ap_from_ap = 0.15;

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = 2.0_s;
  }

  hunter_t* o()             { return static_cast<hunter_t*>( owner ); }
  const hunter_t* o() const { return static_cast<hunter_t*>( owner ); }
};

// Template for common hunter pet action code.
template <class T_PET, class Base>
struct hunter_pet_action_t: public Base
{
private:
  typedef Base ab;
public:

  hunter_pet_action_t( const std::string& n, T_PET* p, const spell_data_t* s = spell_data_t::nil() ):
    ab( n, p, s )
  {
    ab::may_crit = true;
    ab::tick_may_crit = true;

    // If pets are not reported separately, create single stats_t objects for the various pet
    // abilities.
    if ( ! ab::sim -> report_pets_separately )
    {
      auto first_pet = p -> owner -> find_pet( p -> name_str );
      if ( first_pet != nullptr && first_pet != p )
      {
        auto it = range::find( p -> stats_list, ab::stats );
        if ( it != p -> stats_list.end() )
        {
          p -> stats_list.erase( it );
          delete ab::stats;
          ab::stats = first_pet -> get_stats( ab::name_str, this );
        }
      }
    }
  }

  T_PET* p()             { return static_cast<T_PET*>( ab::player ); }
  const T_PET* p() const { return static_cast<T_PET*>( ab::player ); }

  hunter_t* o()             { return p() -> o(); }
  const hunter_t* o() const { return p() -> o(); }

  void init() override
  {
    ab::init();

    parse_affecting_aura( this, o() -> specs.hunter );
    parse_affecting_aura( this, o() -> specs.beast_mastery_hunter );
    parse_affecting_aura( this, o() -> specs.marksmanship_hunter );
    parse_affecting_aura( this, o() -> specs.survival_hunter );
  }

  bool usable_moving() const override { return true; }
};

template <typename Pet>
struct hunter_pet_melee_t: public hunter_pet_action_t<Pet, melee_attack_t>
{
private:
  using ab = hunter_pet_action_t<Pet, melee_attack_t>;
public:

  hunter_pet_melee_t( const std::string &n, Pet* p ):
    ab( n, p )
  {
    ab::background = ab::repeating = true;
    ab::special = false;

    ab::weapon = &( p -> main_hand_weapon );
    ab::weapon_multiplier = 1.0;

    ab::base_execute_time = ab::weapon -> swing_time;
    ab::school = SCHOOL_PHYSICAL;
  }

  timespan_t execute_time() const override
  {
    // there is a cap of ~.25s for pet auto attacks
    timespan_t t = ab::execute_time();
    if ( t < 0.25_s )
      t = 0.25_s;
    return t;
  }
};

// ==========================================================================
// Hunter Main Pet
// ==========================================================================

// Base class for main pet & Animal Companion
struct hunter_main_pet_base_t : public hunter_pet_t
{
  struct actives_t
  {
    action_t* basic_attack = nullptr;
    action_t* kill_command = nullptr;
    attack_t* beast_cleave = nullptr;
    action_t* stomp = nullptr;
    action_t* flanking_strike = nullptr;
  } active;

  struct buffs_t
  {
    buff_t* bestial_wrath = nullptr;
    buff_t* beast_cleave = nullptr;
    buff_t* frenzy = nullptr;
    buff_t* predator = nullptr;
  } buffs;

  hunter_main_pet_base_t( hunter_t* owner, const std::string& pet_name, pet_e pt ):
    hunter_pet_t( owner, pet_name, pt )
  {
    stamina_per_owner = 0.7;
    owner_coeff.ap_from_ap = 0.6;

    initial.armor_multiplier *= 1.05;

    main_hand_weapon.swing_time = owner -> options.pet_attack_speed;
  }

  void create_buffs() override
  {
    hunter_pet_t::create_buffs();

    buffs.bestial_wrath =
      make_buff( this, "bestial_wrath", find_spell( 186254 ) )
        -> set_default_value( find_spell( 186254 ) -> effectN( 1 ).percent() )
        -> set_cooldown( 0_ms )
        -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

    buffs.beast_cleave =
      make_buff( this, "beast_cleave", find_spell( 118455 ) )
        -> set_default_value( o() -> specs.beast_cleave -> effectN( 1 ).percent() )
        -> set_chance( o() -> specs.beast_cleave -> ok() );

    buffs.frenzy =
      make_buff( this, "frenzy", o() -> find_spell( 272790 ) )
        -> set_default_value ( o() -> find_spell( 272790 ) -> effectN( 1 ).percent() )
        -> add_invalidate( CACHE_ATTACK_SPEED );
    if ( o() -> azerite.feeding_frenzy.ok() )
      buffs.frenzy -> set_duration( o() -> azerite.feeding_frenzy.spell() -> effectN( 1 ).time_value() );
  }

  double composite_melee_speed() const override
  {
    double ah = hunter_pet_t::composite_melee_speed();

    if ( buffs.frenzy -> check() )
      ah *= 1.0 / ( 1.0 + buffs.frenzy -> check_stack_value() );

    if ( buffs.predator && buffs.predator -> check() )
      ah *= 1.0 / ( 1.0 + buffs.predator -> check_stack_value() );

    return ah;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = hunter_pet_t::composite_player_multiplier( school );

    m *= 1.0 + buffs.bestial_wrath -> check_value();

    return m;
  }

  void init_spells() override;

  void moving() override
  { return; }
};

struct hunter_main_pet_td_t: public actor_target_data_t
{
public:
  struct dots_t
  {
    dot_t* bloodseeker = nullptr;
  } dots;

  hunter_main_pet_td_t( player_t* target, hunter_main_pet_t* p );
};

struct hunter_main_pet_t : public hunter_main_pet_base_t
{
  struct gains_t
  {
    gain_t* aspect_of_the_wild = nullptr;
  } gains;

  hunter_main_pet_t( hunter_t* owner, const std::string& pet_name, pet_e pt ):
    hunter_main_pet_base_t( owner, pet_name, pt )
  {
    // FIXME work around assert in pet specs
    // Set default specs
    _spec = default_spec();
  }

  specialization_e default_spec()
  {
    if ( pet_type > PET_NONE          && pet_type < PET_FEROCITY_TYPE ) return PET_FEROCITY;
    if ( pet_type > PET_FEROCITY_TYPE && pet_type < PET_TENACITY_TYPE ) return PET_TENACITY;
    if ( pet_type > PET_TENACITY_TYPE && pet_type < PET_CUNNING_TYPE ) return PET_CUNNING;
    return PET_FEROCITY;
  }

  void init_base_stats() override
  {
    hunter_main_pet_base_t::init_base_stats();

    resources.base[RESOURCE_HEALTH] = 6373;
    resources.base[RESOURCE_FOCUS] = 100 + o() -> specs.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS );

    base_gcd = 1.5_s;

    resources.infinite_resource[RESOURCE_FOCUS] = o() -> resources.infinite_resource[RESOURCE_FOCUS];
  }

  void create_buffs() override
  {
    hunter_main_pet_base_t::create_buffs();

    buffs.predator =
      make_buff( this, "predator", o() -> find_spell( 260249 ) )
        -> set_default_value( o() -> find_spell( 260249 ) -> effectN( 1 ).percent() )
        -> add_invalidate( CACHE_ATTACK_SPEED );
  }

  void init_gains() override
  {
    hunter_main_pet_base_t::init_gains();

    gains.aspect_of_the_wild = get_gain( "aspect_of_the_wild" );
  }

  void init_action_list() override
  {
    if ( action_list_str.empty() )
    {
      action_list_str += "/auto_attack";
      action_list_str += "/snapshot_stats";
      action_list_str += "/claw";
      use_default_action_list = true;
    }

    hunter_main_pet_base_t::init_action_list();
  }

  double composite_melee_crit_chance() const override
  {
    double ac = hunter_main_pet_base_t::composite_melee_crit_chance();

    if ( o() -> buffs.aspect_of_the_wild -> check() )
      ac += o() -> specs.aspect_of_the_wild -> effectN( 4 ).percent();

    return ac;
  }

  double resource_regen_per_second( resource_e r ) const override
  {
    if ( r == RESOURCE_FOCUS )
      return o() -> resource_regen_per_second( r ) * 1.25;
    return hunter_main_pet_base_t::resource_regen_per_second( r );
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    hunter_main_pet_base_t::summon( duration );

    o() -> pets.main = this;
    if ( o() -> pets.animal_companion )
      o() -> pets.animal_companion -> summon();
  }

  void demise() override
  {
    hunter_main_pet_base_t::demise();

    if ( o() -> pets.main == this )
      o() -> pets.main = nullptr;
    if ( o() -> pets.animal_companion )
      o() -> pets.animal_companion -> demise();
  }

  target_specific_t<hunter_main_pet_td_t> target_data;

  hunter_main_pet_td_t* get_target_data( player_t* target ) const override
  {
    hunter_main_pet_td_t*& td = target_data[target];
    if ( !td )
      td = new hunter_main_pet_td_t( target, const_cast<hunter_main_pet_t*>( this ) );
    return td;
  }

  resource_e primary_resource() const override
  { return RESOURCE_FOCUS; }

  timespan_t available() const override
  {
    // XXX: this will have to be changed if we ever add other foreground attacks to pets besides Basic Attacks
    if ( ! active.basic_attack )
      return hunter_main_pet_base_t::available();

    const auto time_to_fc = timespan_t::from_seconds( ( active.basic_attack -> base_cost() - resources.current[ RESOURCE_FOCUS ] ) /
                                                        resource_regen_per_second( RESOURCE_FOCUS ) );
    const auto time_to_cd = active.basic_attack -> cooldown -> remains();
    const auto remains = std::max( time_to_cd, time_to_fc );
    // 23/07/2018 - hunter pets seem to have a "generic" lag of about .6s on basic attack usage
    const auto delay_mean = o() -> options.pet_basic_attack_delay;
    const auto delay_stddev = 100_ms;
    const auto lag = o() -> bugs ? rng().gauss( delay_mean, delay_stddev ) : 0_ms;
    return std::max( remains + lag, 100_ms );
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override;

  void init_spells() override;
};

// ==========================================================================
// Animal Companion
// ==========================================================================

struct animal_companion_t : public hunter_main_pet_base_t
{
  animal_companion_t( hunter_t* owner ):
    hunter_main_pet_base_t( owner, "animal_companion", PET_HUNTER )
  {
    regen_type = REGEN_DISABLED;
  }

  void init_spells() override;

  void summon( timespan_t duration = 0_ms ) override
  {
    hunter_main_pet_base_t::summon( duration );

    if ( main_hand_attack )
      main_hand_attack -> schedule_execute();
  }
};

// ==========================================================================
// Dire Critter
// ==========================================================================

struct dire_critter_t: public hunter_pet_t
{
  struct dire_beast_melee_t: public hunter_pet_melee_t<dire_critter_t>
  {
    dire_beast_melee_t( dire_critter_t* p ):
      hunter_pet_melee_t( "dire_beast_melee", p )
    { }
  };

  struct actives_t
  {
    action_t* stomp;
  } active;

  dire_critter_t( hunter_t* owner, const std::string& n = "dire_beast" ):
    hunter_pet_t( owner, n, PET_HUNTER, true /*GUARDIAN*/ )
  {
    owner_coeff.ap_from_ap = 0.15;
    regen_type = REGEN_DISABLED;
  }

  void init_spells() override;

  void summon( timespan_t duration = 0_ms ) override
  {
    hunter_pet_t::summon( duration );

    o() -> buffs.dire_beast -> trigger();

    if ( o() -> talents.stomp -> ok() )
      active.stomp -> execute();

    if ( main_hand_attack )
      main_hand_attack -> execute();
  }
};

std::pair<timespan_t, int> dire_beast_duration( hunter_t* p )
{
  // Dire beast gets a chance for an extra attack based on haste
  // rather than discrete plateaus.  At integer numbers of attacks,
  // the beast actually has a 50% chance of n-1 attacks and 50%
  // chance of n.  It (apparently) scales linearly between n-0.5
  // attacks to n+0.5 attacks.  This uses beast duration to
  // effectively alter the number of attacks as the duration itself
  // isn't important and combat log testing shows some variation in
  // attack speeds.  This is not quite perfect but more accurate
  // than plateaus.
  const timespan_t base_duration = p -> find_spell( 120679 ) -> duration();
  const timespan_t swing_time = 2.0_s * p -> cache.attack_speed();
  double partial_attacks_per_summon = base_duration / swing_time;
  int base_attacks_per_summon = static_cast<int>(partial_attacks_per_summon);
  partial_attacks_per_summon -= static_cast<double>(base_attacks_per_summon );

  if ( p -> rng().roll( partial_attacks_per_summon ) )
    base_attacks_per_summon += 1;

  return { base_attacks_per_summon * swing_time, base_attacks_per_summon };
}

// ==========================================================================
// Spitting Cobra
// ==========================================================================

struct spitting_cobra_t: public hunter_pet_t
{
  struct cobra_spit_t: public hunter_pet_action_t<spitting_cobra_t, spell_t>
  {
    cobra_spit_t( spitting_cobra_t* p, const std::string& options_str ):
      hunter_pet_action_t( "cobra_spit", p, p -> o() -> find_spell( 206685 ) )
    {
      parse_options( options_str );
    }

    // the cobra double dips off versatility & haste
    double composite_versatility( const action_state_t* s ) const override
    {
      double cdv = hunter_pet_action_t::composite_versatility( s );
      return cdv * cdv;
    }

    double composite_haste() const override
    {
      double css = hunter_pet_action_t::composite_haste();
      return css * css;
    }
  };

  spitting_cobra_t( hunter_t* o ):
    hunter_pet_t( o, "spitting_cobra", PET_HUNTER, true )
  {
    owner_coeff.ap_from_ap = 0.15;
    regen_type = REGEN_DISABLED;

    action_list_str = "cobra_spit";
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "cobra_spit" )
      return new cobra_spit_t( this, options_str );

    return hunter_pet_t::create_action( name, options_str );
  }

  // for some reason it gets the player's multipliers
  double composite_player_multiplier( school_e school ) const override
  {
    return owner -> composite_player_multiplier( school );
  }

  void schedule_ready( timespan_t delta_time, bool waiting ) override
  {
    /* nuoHep 2017-02-15 data from a couple krosus logs from wcl
     *      N           Min           Max        Median           Avg        Stddev
     *   2146           0.0         805.0         421.0     341.03262     168.89531
     */
    if ( last_foreground_action )
      delta_time += timespan_t::from_millis( rng().gauss( 340, 170 ) );
    hunter_pet_t::schedule_ready( delta_time, waiting );
  }
};

namespace actions
{

// Template for common hunter main pet action code.
template <class Base>
struct hunter_main_pet_action_t: public hunter_pet_action_t < hunter_main_pet_t, Base >
{
private:
  typedef hunter_pet_action_t<hunter_main_pet_t, Base> ab;
public:

  struct {
    // bm
    bool aspect_of_the_beast;
    // sv
    bool spirit_bond;
  } affected_by;

  hunter_main_pet_action_t( const std::string& n, hunter_main_pet_t* p, const spell_data_t* s = spell_data_t::nil() ):
                            ab( n, p, s ), affected_by()
  {
    affected_by.aspect_of_the_beast = ab::data().affected_by( ab::o() -> talents.aspect_of_the_beast -> effectN( 1 ) );

    affected_by.spirit_bond = ab::data().affected_by( ab::o() -> mastery.spirit_bond -> effectN( 1 ) );
  }

  void init() override
  {
    ab::init();

    if ( affected_by.aspect_of_the_beast )
      ab::base_multiplier *= 1.0 + ab::o() -> talents.aspect_of_the_beast -> effectN( 1 ).percent();
  }

  hunter_main_pet_td_t* td( player_t* t = nullptr ) const
  { return ab::p() -> get_target_data( t ? t : ab::target ); }

  double action_multiplier() const override
  {
    double am = ab::action_multiplier();

    if ( affected_by.spirit_bond && ab::o() -> mastery.spirit_bond -> ok() )
      am *= 1.0 + ab::o() -> cache.mastery() * ab::o() -> mastery.spirit_bond -> effectN( 1 ).mastery_value();

    return am;
  }
};

using hunter_main_pet_attack_t = hunter_main_pet_action_t< melee_attack_t >;

// ==========================================================================
// Hunter Pet Attacks
// ==========================================================================

// Kill Command (pet) =======================================================

struct kill_command_base_t: public hunter_pet_action_t<hunter_main_pet_base_t, melee_attack_t>
{
  struct {
    double chance = 0;
    double energize_amount = 0;
    double bonus_da = 0;
    gain_t* gain = nullptr;
    bool procced = false;
  } serrated_jaws;

  kill_command_base_t( hunter_main_pet_base_t* p, const spell_data_t* s ):
    hunter_pet_action_t( "kill_command", p, s )
  {
    background = true;
    proc = true;

    if ( o() -> azerite.serrated_jaws.ok() )
    {
      serrated_jaws.chance = o() -> azerite.serrated_jaws.spell() -> effectN( 3 ).percent();
      serrated_jaws.energize_amount = o() -> azerite.serrated_jaws.spell() -> effectN( 2 ).base_value();
      serrated_jaws.bonus_da = o() -> azerite.serrated_jaws.value( 1 );
      serrated_jaws.gain = p -> regen_type != REGEN_DISABLED ? p -> get_gain( "serrated_jaws" ) : nullptr;
    }
  }

  void execute() override
  {
    serrated_jaws.procced = rng().roll( serrated_jaws.chance );

    hunter_pet_action_t::execute();

    if ( serrated_jaws.procced && serrated_jaws.gain )
      p() -> resource_gain( RESOURCE_FOCUS, serrated_jaws.energize_amount, serrated_jaws.gain, this );
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = hunter_pet_action_t::bonus_da( s );

    if ( serrated_jaws.procced )
      b += serrated_jaws.bonus_da;

    return b;
  }
};

struct kill_command_bm_t: public kill_command_base_t
{
  struct {
    double percent = 0;
    double multiplier = 1.0;
    benefit_t* benefit = nullptr;
  } killer_instinct;

  kill_command_bm_t( hunter_main_pet_base_t* p ):
    kill_command_base_t( p, p -> find_spell( 83381 ) )
  {
    attack_power_mod.direct = o() -> specs.kill_command -> effectN( 2 ).percent();
    base_dd_adder += o() -> azerite.dire_consequences.value( 1 );

    if ( o() -> talents.killer_instinct -> ok() )
    {
      killer_instinct.percent = o() -> talents.killer_instinct -> effectN( 2 ).base_value();
      killer_instinct.multiplier = 1.0 + o() -> talents.killer_instinct -> effectN( 1 ).percent();
      killer_instinct.benefit = o() -> get_benefit( "killer_instinct" );
    }
  }

  double composite_attack_power() const override
  {
    return o() -> cache.attack_power() * o() -> composite_attack_power_multiplier();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double am = kill_command_base_t::composite_target_multiplier( t );

    if ( killer_instinct.percent )
    {
      const bool active = t -> health_percentage() < killer_instinct.percent;
      killer_instinct.benefit -> update( active );
      if ( active )
        am *= killer_instinct.multiplier;
    }

    return am;
  }
};

struct kill_command_sv_t: public kill_command_base_t
{
  kill_command_sv_t( hunter_main_pet_base_t* p ):
    kill_command_base_t( p, p -> find_spell( 259277 ) )
  {
    attack_power_mod.direct = o() -> specs.kill_command -> effectN( 1 ).percent();
    attack_power_mod.tick = o() -> talents.bloodseeker -> effectN( 1 ).percent();
    base_dd_adder += o() -> azerite.dire_consequences.value( 2 );

    hasted_ticks = true;

    if ( ! o() -> talents.bloodseeker -> ok() )
      dot_duration = 0_ms;

    base_dd_multiplier *= 1.0 + o() -> talents.alpha_predator -> effectN( 2 ).percent();
  }

  // Override behavior so that Kill Command uses hunter's attack power rather than the pet's
  double composite_attack_power() const override
  { return o() -> cache.attack_power() * o() -> composite_attack_power_multiplier(); }

  void trigger_dot( action_state_t* s ) override
  {
    kill_command_base_t::trigger_dot( s );

    trigger_bloodseeker_update( o() );
  }

  void last_tick( dot_t* d ) override
  {
    kill_command_base_t::last_tick( d );

    trigger_bloodseeker_update( o() );
  }

  // does not pandemic
  timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const override
  {
    return dot -> time_to_next_tick() + triggered_duration;
  }
};

// Beast Cleave ==============================================================

struct beast_cleave_attack_t: public hunter_pet_action_t<hunter_main_pet_base_t, melee_attack_t>
{
  beast_cleave_attack_t( hunter_main_pet_base_t* p ):
    hunter_pet_action_t( "beast_cleave", p, p -> find_spell( 118459 ) )
  {
    aoe = -1;
    background = true;
    callbacks = false;
    may_miss = false;
    may_crit = false;
    proc = false;
    // The starting damage includes all the buffs
    base_dd_min = base_dd_max = 0;
    spell_power_mod.direct = attack_power_mod.direct = 0;
    weapon_multiplier = 0;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    hunter_pet_action_t::available_targets( tl );

    // Cannot hit the original target.
    tl.erase( std::remove( tl.begin(), tl.end(), target ), tl.end() );

    return tl.size();
  }
};

static void trigger_beast_cleave( action_state_t* s )
{
  if ( !s -> action -> result_is_hit( s -> result ) )
    return;

  if ( s -> action -> sim -> active_enemies == 1 )
    return;

  auto p = debug_cast<hunter_main_pet_base_t*>( s -> action -> player );

  if ( !p -> buffs.beast_cleave -> check() )
    return;

  const double cleave = s -> result_total * p -> buffs.beast_cleave -> check_value();
  p -> active.beast_cleave -> set_target( s -> target );
  p -> active.beast_cleave -> base_dd_min = cleave;
  p -> active.beast_cleave -> base_dd_max = cleave;
  p -> active.beast_cleave -> execute();
}

// Pet Melee ================================================================

struct pet_melee_t : public hunter_pet_melee_t<hunter_main_pet_base_t>
{
  pet_melee_t( const std::string& n, hunter_main_pet_base_t* p ):
    hunter_pet_melee_t( n, p )
  {
  }

  void impact( action_state_t* s ) override
  {
    hunter_pet_melee_t::impact( s );

    trigger_beast_cleave( s );
  }
};

// Pet Auto Attack ==========================================================

struct pet_auto_attack_t: public action_t
{
  pet_auto_attack_t( hunter_main_pet_t* player, const std::string& options_str ):
    action_t( ACTION_OTHER, "auto_attack", player )
  {
    parse_options( options_str );

    player -> main_hand_attack = new pet_melee_t( "melee", player );

    school = SCHOOL_PHYSICAL;
    ignore_false_positive = true;
    range = 5;
    trigger_gcd = 0_ms;
  }

  void execute() override
  {
    player -> main_hand_attack -> schedule_execute();
  }

  bool ready() override
  { return player -> main_hand_attack -> execute_event == nullptr; } // not swinging
};

// Pet Claw/Bite/Smack ======================================================

struct basic_attack_t : public hunter_main_pet_attack_t
{
  struct {
    double cost_pct = 0;
    double multiplier = 1.0;
    benefit_t* benefit = nullptr;
  } wild_hunt;
  const double venomous_fangs_bonus_da;

  basic_attack_t( hunter_main_pet_t* p, const std::string& n, const std::string& options_str ):
    hunter_main_pet_attack_t( n, p, p -> find_pet_spell( n ) ),
    venomous_fangs_bonus_da( p -> o() -> azerite.venomous_fangs.value( 1 ) )
  {
    parse_options( options_str );

    school = SCHOOL_PHYSICAL;

    attack_power_mod.direct = 1.0 / 3.0;
    // 28-06-2018: While spell data says it has a base damage in-game testing shows that it doesn't use it.
    base_dd_min = base_dd_max = 0;

    auto wild_hunt_spell = p -> find_spell( 62762 );
    wild_hunt.cost_pct = 1.0 + wild_hunt_spell -> effectN( 2 ).percent();
    wild_hunt.multiplier = 1.0 + wild_hunt_spell -> effectN( 1 ).percent();
    wild_hunt.benefit = p -> get_benefit( "wild_hunt" );

    p -> active.basic_attack = this;
  }

  // Override behavior so that Basic Attacks use hunter's attack power rather than the pet's
  double composite_attack_power() const override
  { return o() -> cache.attack_power() * o() -> composite_attack_power_multiplier(); }

  bool use_wild_hunt() const
  {
    return p() -> resources.current[RESOURCE_FOCUS] > 50;
  }

  void impact( action_state_t* s ) override
  {
    hunter_main_pet_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      trigger_beast_cleave( s );
  }

  double action_multiplier() const override
  {
    double am = hunter_main_pet_attack_t::action_multiplier();

    const bool used_wild_hunt = use_wild_hunt();
    if ( used_wild_hunt )
      am *= wild_hunt.multiplier;
    wild_hunt.benefit -> update( used_wild_hunt );

    return am;
  }

  double cost() const override
  {
    double c = hunter_main_pet_attack_t::cost();

    if ( use_wild_hunt() )
      c *= wild_hunt.cost_pct;

    return c;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = hunter_main_pet_attack_t::bonus_da( s );

    if ( o() -> get_target_data( s -> target ) -> dots.serpent_sting -> is_ticking() )
      b += venomous_fangs_bonus_da;

    return b;
  }
};

// Flanking Strike (pet) ===================================================

struct flanking_strike_t: public hunter_main_pet_attack_t
{
  flanking_strike_t( hunter_main_pet_t* p ):
    hunter_main_pet_attack_t( "flanking_strike", p, p -> find_spell( 259516 ) )
  {
    background = true;

    parse_effect_data( o() -> find_spell( 269752 ) -> effectN( 1 ) );
  }

  double composite_attack_power() const override
  { return o() -> cache.attack_power() * o() -> composite_attack_power_multiplier(); }
};

// Stomp ===================================================================

struct stomp_t : public hunter_pet_action_t<hunter_pet_t, attack_t>
{
  stomp_t( hunter_pet_t* p ):
    hunter_pet_action_t( "stomp", p, p -> find_spell( 201754 ) )
  {
    aoe = -1;
  }
};

} // end namespace pets::actions

hunter_main_pet_td_t::hunter_main_pet_td_t( player_t* target, hunter_main_pet_t* p ):
  actor_target_data_t( target, p )
{
  dots.bloodseeker = target -> get_dot( "kill_command", p );
}

// hunter_pet_t::create_action ==============================================

action_t* hunter_main_pet_t::create_action( const std::string& name,
                                            const std::string& options_str )
{
  using namespace actions;

  if ( name == "auto_attack" ) return new          pet_auto_attack_t( this, options_str );
  if ( name == "claw" ) return new                 basic_attack_t( this, "Claw", options_str );
  if ( name == "bite" ) return new                 basic_attack_t( this, "Bite", options_str );
  if ( name == "smack" ) return new                basic_attack_t( this, "Smack", options_str );
  return hunter_main_pet_base_t::create_action( name, options_str );
}

// hunter_t::init_spells ====================================================

void hunter_main_pet_base_t::init_spells()
{
  hunter_pet_t::init_spells();

  if ( o() -> specialization() == HUNTER_BEAST_MASTERY )
    active.kill_command = new actions::kill_command_bm_t( this );
  else if ( o() -> specialization() == HUNTER_SURVIVAL )
    active.kill_command = new actions::kill_command_sv_t( this );

  if ( o() -> specs.beast_cleave -> ok() )
    active.beast_cleave = new actions::beast_cleave_attack_t( this );

  if ( o() -> talents.stomp -> ok() )
    active.stomp = new actions::stomp_t( this );
}

void hunter_main_pet_t::init_spells()
{
  hunter_main_pet_base_t::init_spells();

  if ( o() -> talents.flanking_strike -> ok() )
    active.flanking_strike = new actions::flanking_strike_t( this );
}

void animal_companion_t::init_spells()
{
  hunter_main_pet_base_t::init_spells();

  main_hand_attack = new actions::pet_melee_t( "animal_companion_melee", this );
}

void dire_critter_t::init_spells()
{
  hunter_pet_t::init_spells();

  main_hand_attack = new dire_beast_melee_t( this );

  if ( o() -> talents.stomp -> ok() )
    active.stomp = new actions::stomp_t( this );
}

template <typename Pet, size_t N>
struct active_pets_t
{
  using data_t = std::array<Pet*, N>;

  data_t data_;
  arv::array_view<Pet*> active_;

  active_pets_t( data_t d, size_t n ):
    data_( d ), active_( data_.data(), n )
  {}

  typename arv::array_view<Pet*>::iterator begin() const { return active_.begin(); }
  typename arv::array_view<Pet*>::iterator end() const { return active_.end(); }
};

// returns the active pets from the list 'cast' to the supplied pet type
template <typename Pet, typename... Pets>
auto active( Pets... pets_ ) -> active_pets_t<Pet, sizeof...(Pets)>
{
  Pet* pets[] = { pets_... };
  typename active_pets_t<Pet, sizeof...(Pets)>::data_t active_pets;
  size_t active_pet_count = 0;
  for ( auto pet : pets )
  {
    if ( pet && ! pet -> is_sleeping() )
      active_pets[ active_pet_count++ ] = pet;
  }

  return { active_pets, active_pet_count };
}

} // end namespace pets

void trigger_birds_of_prey( hunter_t* p, player_t* t )
{
  if ( ! p -> talents.birds_of_prey -> ok() )
    return;

  if ( ! p -> pets.main )
    return;

  if ( t == p -> pets.main -> target )
    p -> buffs.coordinated_assault -> extend_duration( p, p -> talents.birds_of_prey -> effectN( 1 ).time_value() );
}

void trigger_bloodseeker_update( hunter_t* p )
{
  if ( !p -> talents.bloodseeker -> ok() )
    return;

  int bleeding_targets = 0;
  for ( const player_t* t : p -> sim -> target_non_sleeping_list )
  {
    if ( t -> is_enemy() && t -> debuffs.bleeding -> check() )
      bleeding_targets++;
  }
  bleeding_targets = std::min( bleeding_targets, p -> buffs.predator -> max_stack() );

  const int current = p -> buffs.predator -> check();
  if ( current < bleeding_targets )
  {
    p -> buffs.predator -> trigger( bleeding_targets - current );
    if ( auto pet = p -> pets.main )
      pet -> buffs.predator -> trigger( bleeding_targets - current );
  }
  else if ( current > bleeding_targets )
  {
    p -> buffs.predator -> decrement( current - bleeding_targets );
    if ( auto pet = p -> pets.main )
      pet -> buffs.predator -> decrement( current - bleeding_targets );
  }
}

void trigger_lethal_shots( hunter_t* p )
{
  if ( !p -> talents.lethal_shots -> ok() )
    return;

  if ( p -> rng().roll( p -> talents.lethal_shots -> proc_chance() ) )
  {
    const auto base_value = p -> talents.lethal_shots -> effectN( 1 ).base_value();
    p -> cooldowns.rapid_fire -> adjust( -timespan_t::from_seconds( base_value / 10 ) );
    p -> procs.lethal_shots -> occur();
  }
}

namespace attacks
{

template <typename Base>
struct auto_attack_base_t : hunter_action_t<Base>
{
private:
  using ab = hunter_action_t<Base>;

public:
  bool first = true;

  auto_attack_base_t( const std::string& n, hunter_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, p, s )
  {
    ab::background = ab::repeating = true;
    ab::interrupt_auto_attack = false;
    ab::special = false;
    ab::trigger_gcd = 0_ms;

    ab::weapon = &( p -> main_hand_weapon );
    ab::base_execute_time = ab::weapon -> swing_time;
  }

  void reset() override
  {
    ab::reset();
    first = true;
  }

  timespan_t execute_time() const override
  {
    if ( !ab::player -> in_combat )
      return 10_ms;
    if ( first )
      return 100_ms;
    return ab::execute_time();
  }

  void execute() override
  {
    first = false;
    ab::execute();
  }
};

// ==========================================================================
// Hunter Attacks
// ==========================================================================

// Volley ============================================================================

struct volley_t: hunter_ranged_attack_t
{
  volley_t( const std::string& n, hunter_t* p ):
    hunter_ranged_attack_t( n, p, p -> talents.volley -> effectN( 1 ).trigger() )
  {
    background = true;
    aoe = -1;
  }
};

// Auto Shot ================================================================

struct auto_shot_state_t : public action_state_t
{
  auto_shot_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target )
  { }

  proc_types2 cast_proc_type2() const override
  {
    // Auto shot seems to trigger Meticulous Scheming (and possibly other
    // effects that care about casts).
    return PROC2_CAST_DAMAGE;
  }
};

struct auto_shot_t : public auto_attack_base_t<ranged_attack_t>
{
  volley_t* volley = nullptr;
  double wild_call_chance = 0;

  auto_shot_t( hunter_t* p ) :
    auto_attack_base_t( "auto_shot", p, p -> find_spell( 75 ) )
  {
    if ( p -> talents.volley -> ok() )
    {
      volley = p -> get_background_action<volley_t>( "volley" );
      add_child( volley );
    }

    wild_call_chance = p -> specs.wild_call -> proc_chance() +
                       p -> talents.one_with_the_pack -> effectN( 1 ).percent();
  }

  action_state_t* new_state() override
  {
    return new auto_shot_state_t( this, target );
  }

  void impact( action_state_t* s ) override
  {
    auto_attack_base_t::impact( s );

    if ( p() -> buffs.lock_and_load -> trigger() )
      p() -> cooldowns.aimed_shot -> reset( true );

    if ( s -> result == RESULT_CRIT && p() -> specs.wild_call -> ok() && rng().roll( wild_call_chance ) )
    {
      p() -> cooldowns.barbed_shot -> reset( true );
      p() -> procs.wild_call -> occur();
    }

    if ( volley && rng().roll( p() -> talents.volley -> proc_chance() ) )
    {
      volley -> set_target( s -> target );
      volley -> execute();
    }
  }
};

//==============================
// Shared attacks
//==============================

// Barrage ==================================================================

struct barrage_t: public hunter_spell_t
{
  struct barrage_damage_t: public hunter_ranged_attack_t
  {
    barrage_damage_t( const std::string& n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> talents.barrage -> effectN( 1 ).trigger() )
    {
      aoe = -1;
      radius = 0; //Barrage attacks all targets in front of the hunter, so setting radius to 0 will prevent distance targeting from using a 40 yard radius around the target.
      // Todo: Add in support to only hit targets in the frontal cone.
    }
  };

  barrage_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "barrage", p, p -> talents.barrage )
  {
    parse_options( options_str );

    may_miss = may_crit = false;
    channeled = true;

    tick_zero = true;
    travel_speed = 0.0;

    tick_action = p -> get_background_action<barrage_damage_t>( "barrage_damage" );
    starved_proc = p -> get_proc( "starved: barrage" );
  }

  void schedule_execute( action_state_t* state = nullptr ) override
  {
    hunter_spell_t::schedule_execute( state );

    // Delay auto shot, add 500ms to simulate "wind up"
    if ( p() -> main_hand_attack && p() -> main_hand_attack -> execute_event )
      p() -> main_hand_attack -> reschedule_execute( dot_duration * composite_haste() + 500_ms );
  }
};

// Multi Shot Attack =================================================================

struct multi_shot_t: public hunter_ranged_attack_t
{
  struct rapid_reload_t: public hunter_ranged_attack_t
  {
    rapid_reload_t( const std::string& n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> find_spell( 278565 ) )
    {
      aoe = -1;
      base_dd_min = base_dd_max = p -> azerite.rapid_reload.value( 1 );
    }
  };

  struct {
    rapid_reload_t* action = nullptr;
    int min_targets = std::numeric_limits<int>::max();
    timespan_t reduction = 0_ms;
  } rapid_reload;

  multi_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "multishot", p, p -> find_specialization_spell( "Multi-Shot" ) )
  {
    parse_options( options_str );
    aoe = -1;

    if ( p -> azerite.rapid_reload.ok() )
    {
      rapid_reload.action = p -> get_background_action<rapid_reload_t>( "multishot_rapid_reload" );
      rapid_reload.min_targets = as<int>(p -> azerite.rapid_reload.spell() -> effectN( 2 ).base_value());
      rapid_reload.reduction = ( timespan_t::from_seconds( p -> azerite.rapid_reload.spell() -> effectN ( 3 ).base_value() ) );
      add_child( rapid_reload.action );
    }
  }

  double cost() const override
  {
    double cost = hunter_ranged_attack_t::cost();

    if ( p() -> buffs.master_marksman -> check() )
      cost *= 1.0 + p() -> buffs.master_marksman -> check_value();

    return cost;
  }

  double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    am *= 1.0 + p() -> buffs.precise_shots -> value();

    return am;
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> buffs.master_marksman -> up(); // benefit tracking
    p() -> buffs.master_marksman -> decrement();

    p() -> buffs.precise_shots -> decrement();

    for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p() -> pets.main, p() -> pets.animal_companion ) )
      pet -> buffs.beast_cleave -> trigger();

    if ( num_targets_hit >= p() -> specs.trick_shots -> effectN( 2 ).base_value() )
      p() -> buffs.trick_shots -> trigger();

    if ( p() -> talents.calling_the_shots -> ok() )
      p() -> cooldowns.trueshot -> adjust( - p() -> talents.calling_the_shots -> effectN( 1 ).time_value() );

    trigger_lethal_shots( p() );

    if ( rapid_reload.action && num_targets_hit > rapid_reload.min_targets )
    {
      rapid_reload.action -> set_target( target );
      p() -> cooldowns.aspect_of_the_wild -> adjust( - ( rapid_reload.reduction * num_targets_hit ) );
      rapid_reload.action -> execute();
    }
  }
};

//==============================
// Beast Mastery attacks
//==============================

// Chimaera Shot =====================================================================

struct chimaera_shot_t: public hunter_ranged_attack_t
{
  struct chimaera_shot_impact_t: public hunter_ranged_attack_t
  {
    chimaera_shot_impact_t( const std::string& n, hunter_t* p, const spell_data_t* s ):
      hunter_ranged_attack_t( n, p, s )
    {
      dual = true;
      parse_effect_data( p -> find_spell( 204304 ) -> effectN( 1 ) );
    }
  };

  std::array<chimaera_shot_impact_t*, 2> damage;
  unsigned current_damage_action;

  chimaera_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "chimaera_shot", p, p -> talents.chimaera_shot ),
    current_damage_action( 0 )
  {
    parse_options( options_str );

    aoe = 2;
    radius = 5.0;

    damage[ 0 ] = p -> get_background_action<chimaera_shot_impact_t>( "chimaera_shot_frost", p -> find_spell( 171454 ) );
    damage[ 1 ] = p -> get_background_action<chimaera_shot_impact_t>( "chimaera_shot_nature", p -> find_spell( 171457 ) );
    for ( auto a : damage )
      add_child( a );

    school = SCHOOL_FROSTSTRIKE; // Just so the report shows a mixture of the two colors.
  }

  void do_schedule_travel( action_state_t* s, const timespan_t& ) override
  {
    damage[ current_damage_action ] -> set_target( s -> target );
    damage[ current_damage_action ] -> execute();
    current_damage_action = ( current_damage_action + 1 ) % damage.size();
    action_state_t::release( s );
  }

  double cast_regen( const action_state_t* s ) const override
  {
    const size_t targets_hit = std::min( target_list().size(), as<size_t>( n_targets() ) );
    return hunter_ranged_attack_t::cast_regen( s ) +
           ( targets_hit * damage[ 0 ] -> composite_energize_amount( nullptr ) );
  }
};

// Cobra Shot Attack =================================================================

struct cobra_shot_t: public hunter_ranged_attack_t
{
  const timespan_t kill_command_reduction;
  const timespan_t venomous_bite_reduction;

  cobra_shot_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "cobra_shot", player, player -> find_specialization_spell( "Cobra Shot" ) ),
    kill_command_reduction( timespan_t::from_seconds( data().effectN( 3 ).base_value() ) ),
    venomous_bite_reduction( timespan_t::from_millis( player -> talents.venomous_bite -> effectN( 1 ).base_value() * 100 ) )
  {
    parse_options( options_str );

    base_multiplier *= 1.0 + p() -> find_spell( 262838 ) -> effectN( 1 ).percent(); // Cobra Shot (Rank 3)
    base_costs[ RESOURCE_FOCUS ] += player -> find_spell( 262837 ) -> effectN( 1 ).base_value(); // Cobra Shot (Rank 2)
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> cooldowns.kill_command -> adjust( -kill_command_reduction );

    if ( p() -> talents.venomous_bite -> ok() )
      p() -> cooldowns.bestial_wrath -> adjust( -venomous_bite_reduction );

    if ( p() -> talents.killer_cobra -> ok() && p() -> buffs.bestial_wrath -> check() )
      p() -> cooldowns.kill_command -> reset( true );
  }
};

// Barbed Shot ===============================================================

struct barbed_shot_t: public hunter_ranged_attack_t
{
  barbed_shot_t( hunter_t* p, const std::string& options_str ) :
    hunter_ranged_attack_t( "barbed_shot", p, p -> specs.barbed_shot )
  {
    parse_options(options_str);

    tick_may_crit = false;
    tick_zero = true;

    base_ta_adder += p -> azerite.feeding_frenzy.value( 2 );
  }

  void init_finished() override
  {
    pet_t* pets[] = { p() -> pets.main, p() -> pets.animal_companion };
    for ( auto pet : pets )
      add_pet_stats( pet, { "stomp" } );

    hunter_ranged_attack_t::init_finished();
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    // trigger regen buff
    auto it = range::find_if( p() -> buffs.barbed_shot, []( buff_t* b ) { return !b -> check(); } );
    if ( it != p() -> buffs.barbed_shot.end() )
      (*it) -> trigger(); // TODO: error when don't have enough buffs?

    p() -> buffs.thrill_of_the_hunt -> trigger();

    // Adjust BW cd
    timespan_t t = timespan_t::from_seconds( p() -> specs.bestial_wrath -> effectN( 3 ).base_value() );
    p() -> cooldowns.bestial_wrath -> adjust( -t );

    if ( p() -> azerite.dance_of_death.ok() && rng().roll( p() -> cache.attack_crit_chance() ) )
      p() -> buffs.dance_of_death -> trigger();

    for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p() -> pets.main, p() -> pets.animal_companion ) )
    {
      if ( p() -> talents.stomp -> ok() )
        pet -> active.stomp -> execute();

      pet -> buffs.frenzy -> trigger();
    }
  }

  // does not pandemic
  timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const override
  {
    return dot -> time_to_next_tick() + triggered_duration;
  }
};

//==============================
// Marksmanship attacks
//==============================

// Bursting Shot ======================================================================

struct bursting_shot_t : public hunter_ranged_attack_t
{
  bursting_shot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "bursting_shot", player, player -> find_spell( 186387 ) )
  {
    parse_options( options_str );
  }
};

// Aimed Shot base class ==============================================================

struct aimed_shot_base_t: public hunter_ranged_attack_t
{
  struct {
    benefit_t* benefit = nullptr;
    double percent = 0.0;
    double high, low;
  } careful_aim;
  const int trick_shots_targets;

  aimed_shot_base_t( const std::string& name, hunter_t* p ):
    hunter_ranged_attack_t( name, p, p -> specs.aimed_shot ),
    trick_shots_targets( static_cast<int>( p -> specs.trick_shots -> effectN( 1 ).base_value() ) )
  {
    radius = 8.0;
    base_aoe_multiplier = p -> specs.trick_shots -> effectN( 4 ).percent();

    if ( p -> talents.careful_aim -> ok() )
    {
      careful_aim.benefit = p -> get_benefit( "careful_aim" );
      careful_aim.percent = p -> talents.careful_aim -> effectN( 4 ).percent();
      careful_aim.high = p -> talents.careful_aim -> effectN( 1 ).base_value();
      careful_aim.low = p -> talents.careful_aim -> effectN( 2 ).base_value();
    }
  }

  int n_targets() const override
  {
    if ( p() -> buffs.trick_shots -> check() )
      return trick_shots_targets + 1;
    return hunter_ranged_attack_t::n_targets();
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = hunter_ranged_attack_t::composite_target_da_multiplier( t );

    if ( careful_aim.percent )
    {
      const bool active = t -> health_percentage() > careful_aim.high || t -> health_percentage() < careful_aim.low;
      const bool procced = active && rng().roll( careful_aim.percent );
      careful_aim.benefit -> update( procced );
      if ( procced )
        m *= 1.0 + p() -> talents.careful_aim -> effectN( 3 ).percent();
    }

    return m;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = hunter_ranged_attack_t::bonus_da( s );

    if ( auto td_ = find_td( s -> target ) )
      b += td_ -> debuffs.steady_aim -> stack_value();

    return b;
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( auto td_ = find_td( s -> target ) )
      td_ -> debuffs.steady_aim -> expire();
  }
};

// Aimed Shot =========================================================================

struct aimed_shot_t : public aimed_shot_base_t
{
  // class for 'secondary' aimed shots
  struct aimed_shot_secondary_t: public aimed_shot_base_t
  {
    aimed_shot_secondary_t( const std::string& n, hunter_t* p ):
      aimed_shot_base_t( n, p )
    {
      background = true;
      dual       = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
    }
  };

  aimed_shot_secondary_t* double_tap = nullptr;
  bool lock_and_loaded = false;
  struct {
    proc_t* double_tap;
  } procs;
  struct {
    double chance = 0;
    proc_t* proc;
  } surging_shots;

  aimed_shot_t( hunter_t* p, const std::string& options_str ) :
    aimed_shot_base_t( "aimed_shot", p )
  {
    parse_options( options_str );

    if ( p -> talents.double_tap -> ok() )
    {
      double_tap = p -> get_background_action<aimed_shot_secondary_t>( "aimed_shot_double_tap" );
      add_child( double_tap );
    }

    procs.double_tap = p -> get_proc( "double_tap_aimed" );

    if ( p -> azerite.surging_shots.ok() )
    {
      surging_shots.chance = p -> azerite.surging_shots.spell_ref().effectN( 1 ).percent();
      surging_shots.proc = p -> get_proc( "Surging Shots Rapid Fire reset" );
    }
  }

  double cost() const override
  {
    if ( lock_and_loaded )
      return 0;

    return aimed_shot_base_t::cost();
  }

  void schedule_execute( action_state_t* s ) override
  {
    lock_and_loaded = p() -> buffs.lock_and_load -> up();

    aimed_shot_base_t::schedule_execute( s );
  }

  void execute() override
  {
    aimed_shot_base_t::execute();

    if ( double_tap && p() -> buffs.double_tap -> check() )
    {
      double_tap -> set_target( target );
      double_tap -> execute();
      p() -> buffs.double_tap -> decrement();
      procs.double_tap -> occur();
    }

    p() -> buffs.trick_shots -> up(); // benefit tracking
    p() -> buffs.trick_shots -> decrement();

    if ( lock_and_loaded )
      p() -> buffs.lock_and_load -> decrement();
    lock_and_loaded = false;

    p() -> buffs.precise_shots -> trigger( 1 + rng().range( p() -> buffs.precise_shots -> max_stack() ) );
    p() -> buffs.master_marksman -> trigger();

    if ( rng().roll( surging_shots.chance ) )
    {
      surging_shots.proc -> occur();
      p() -> cooldowns.rapid_fire -> reset( true );
    }
  }

  timespan_t execute_time() const override
  {
    if ( p() -> buffs.lock_and_load -> check() )
      return 0_ms;

    auto et = aimed_shot_base_t::execute_time();

    if ( p() -> buffs.trueshot -> check() )
      et *= 1.0 + p() -> buffs.trueshot -> check_value();

    return et;
  }

  double recharge_multiplier() const override
  {
    double m = aimed_shot_base_t::recharge_multiplier();

    // XXX [8.1]: Spell Data indicates that it's reducing Aimed Shot recharge rate by 225% (12s/3.25 = 3.69s)
    // m /= 1.0 + .6;  // The information from the bluepost
    // m /= 1.0 + 2.25; // The bugged (in spelldata) value for Aimed Shot.
    if ( p() -> buffs.trueshot -> check() )
      m /= 1.0 + p() -> specs.trueshot -> effectN( 3 ).percent();

    return m;
  }

  bool usable_moving() const override
  {
    return false;
  }
};

// Arcane Shot Attack ================================================================

struct arcane_shot_t: public hunter_ranged_attack_t
{
  arcane_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "arcane_shot", p, p -> find_specialization_spell( "Arcane Shot" ) )
  {
    parse_options( options_str );
  }

  double cost() const override
  {
    double cost = hunter_ranged_attack_t::cost();

    if ( p() -> buffs.master_marksman -> check() )
      cost *= 1.0 + p() -> buffs.master_marksman -> check_value();

    return cost;
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> buffs.precise_shots -> decrement();

    p() -> buffs.master_marksman -> up(); // benefit tracking
    p() -> buffs.master_marksman -> decrement();

    if ( p() -> talents.calling_the_shots -> ok() )
      p() -> cooldowns.trueshot -> adjust( - p() -> talents.calling_the_shots -> effectN( 1 ).time_value() );

    trigger_lethal_shots( p() );
  }

  double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    am *= 1.0 + p() -> buffs.precise_shots -> value();

    return am;
  }
};

// Piercing Shot  =========================================================================

struct piercing_shot_t: public hunter_ranged_attack_t
{
  piercing_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "piercing_shot", p, p -> talents.piercing_shot )
  {
    parse_options( options_str );

    aoe = -1;
    base_aoe_multiplier = 1.0 / ( data().effectN( 1 ).base_value() / 10 ) ;
  }

  double target_armor( player_t* ) const override
  {
    return 0.0;
  }
};

// Steady Shot ========================================================================

struct steady_shot_t: public hunter_ranged_attack_t
{
  steady_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "steady_shot", p, p -> find_specialization_spell( "Steady Shot" ) )
  {
    parse_options( options_str );

    energize_type = ENERGIZE_ON_CAST;
    energize_resource = RESOURCE_FOCUS;
    energize_amount = data().effectN( 2 ).base_value();
  }

  timespan_t execute_time() const override
  {
    timespan_t t = hunter_ranged_attack_t::execute_time();

    if ( p() -> buffs.steady_focus -> check() )
      t *= 1.0 + p() -> buffs.steady_focus -> data().effectN( 1 ).percent();

    return t;
  }

  timespan_t gcd() const override
  {
    timespan_t g = hunter_ranged_attack_t::gcd();

    if ( p() -> buffs.steady_focus -> check() )
      g *= 1.0 + p() -> buffs.steady_focus -> check_value();

    return g < min_gcd ? min_gcd : g;
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( p() -> azerite.steady_aim.ok() )
      td( s -> target ) -> debuffs.steady_aim -> trigger();
  }

  void try_steady_focus() override
  {
    p() -> buffs.steady_focus -> trigger();
  }
};

// Rapid Fire =========================================================================

struct rapid_fire_t: public hunter_spell_t
{
  // this is required because Double Tap 'snapshots' on channel start
  struct rapid_fire_state_t : public action_state_t
  {
    bool double_tapped = false;
    rapid_fire_state_t( action_t* a, player_t* t ) : action_state_t( a, t ) {}

    void initialize() override
    {
      action_state_t::initialize();
      double_tapped = false;
    }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    {
      action_state_t::debug_str( s );
      s << " double_tapped=" << double_tapped;
      return s;
    }

    void copy_state( const action_state_t* o ) override
    {
      action_state_t::copy_state( o );
      double_tapped = debug_cast<const rapid_fire_state_t*>( o ) -> double_tapped;
    }
  };

  struct rapid_fire_damage_t: public hunter_ranged_attack_t
  {
    const int trick_shots_targets;
    struct {
      double chance = 0.0;
      double amount = 0.0;
      gain_t* gain = nullptr;
    } focused_fire;
    struct {
      double value = 0;
      int max_num_ticks = 0;
    } surging_shots;

    rapid_fire_damage_t( const std::string& n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> find_spell( 257045 ) ),
      trick_shots_targets( as<int>( p -> specs.trick_shots -> effectN( 3 ).base_value() ) )
    {
      dual = true;
      direct_tick = true;
      radius = 8.0;
      base_aoe_multiplier = p -> specs.trick_shots -> effectN( 5 ).percent();

      parse_effect_data( p -> find_spell( 263585 ) -> effectN( 1 ) );

      base_dd_adder += p -> azerite.focused_fire.value( 2 );
      if ( p -> azerite.focused_fire.ok() )
      {
        auto trigger_ = p -> azerite.focused_fire.spell() -> effectN( 1 ).trigger();
        focused_fire.chance = trigger_ -> proc_chance();
        focused_fire.amount = trigger_ -> effectN( 1 ).trigger() -> effectN( 1 ).base_value();
        focused_fire.gain = p -> get_gain( "focused_fire" );
      }

      if ( p -> azerite.surging_shots.ok() )
      {
        // XXX: The multipliers are emperically hardcoded. The tooltip is wrong.
        base_dd_adder += p -> azerite.surging_shots.value( 2 ) * .5;
        surging_shots.value = p -> azerite.surging_shots.value( 2 ) * .2;
        // XXX: Does not scale beyond the base number of ticks
        surging_shots.max_num_ticks = as<int>(p -> specs.rapid_fire -> effectN( 1 ).base_value());
      }
    }

    int n_targets() const override
    {
      if ( p() -> buffs.trick_shots -> check() )
        return 1 + trick_shots_targets;
      return hunter_ranged_attack_t::n_targets();
    }

    void execute() override
    {
      hunter_ranged_attack_t::execute();

      p() -> buffs.trick_shots -> up(); // benefit tracking

      if ( rng().roll( focused_fire.chance ) )
        p() -> resource_gain( RESOURCE_FOCUS, focused_fire.amount, focused_fire.gain, this );
    }

    double bonus_da( const action_state_t* s ) const override
    {
      double b = hunter_ranged_attack_t::bonus_da( s );

      b += surging_shots.value * std::min( parent_dot -> current_tick, surging_shots.max_num_ticks );

      return b;
    }
  };

  rapid_fire_damage_t* damage;
  int base_num_ticks;
  struct {
    proc_t* double_tap;
  } procs;

  rapid_fire_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "rapid_fire", p, p -> specs.rapid_fire ),
    damage( p -> get_background_action<rapid_fire_damage_t>( "rapid_fire_damage" ) ),
    base_num_ticks( as<int>(data().effectN( 1 ).base_value()) )
  {
    parse_options( options_str );

    may_miss = may_crit = false;
    channeled = true;
    tick_zero = true;

    base_num_ticks = as<int>(base_num_ticks * (1 + p -> talents.streamline -> effectN( 2 ).percent()));
    dot_duration += p -> talents.streamline -> effectN( 1 ).time_value();

    procs.double_tap = p -> get_proc( "double_tap_rapid_fire" );
  }

  void init() override
  {
    hunter_spell_t::init();

    damage -> stats      = stats;
    damage -> parent_dot = target -> get_dot( name_str, player );
    stats -> action_list.push_back( damage );
  }

  void schedule_execute( action_state_t* state ) override
  {
    hunter_spell_t::schedule_execute( state );

    // RF does not simply delay auto shot, but completely resets it
    if ( p() -> main_hand_attack && p() -> main_hand_attack -> execute_event )
      event_t::cancel( p() -> main_hand_attack -> execute_event );
  }

  void tick( dot_t* d ) override
  {
    hunter_spell_t::tick( d );
    damage -> set_target( d -> target );
    damage -> execute();
  }

  void last_tick( dot_t* d ) override
  {
    hunter_spell_t::last_tick( d );

    p() -> buffs.trick_shots -> decrement();

    if ( p() -> buffs.double_tap -> check() )
      procs.double_tap -> occur();
    p() -> buffs.double_tap -> decrement();

    // XXX: this triggers *only* after a *full* uninterrupted channel
    p() -> buffs.in_the_rhythm -> trigger();

    // schedule auto shot
    if ( p() -> main_hand_attack )
      p() -> main_hand_attack -> schedule_execute();
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    timespan_t t = hunter_spell_t::tick_time( s );

    if ( debug_cast<const rapid_fire_state_t*>( s ) -> double_tapped )
      t *= 1.0 + p() -> talents.double_tap -> effectN( 1 ).percent();

    return t;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    // substract 1 here because RF has a tick at zero
    return ( num_ticks( s ) - 1 ) * tick_time( s );
  }

  double cast_regen( const action_state_t* s ) const override
  {
    return hunter_spell_t::cast_regen( s ) +
           num_ticks( s ) * damage -> composite_energize_amount( nullptr );
  }

  int num_ticks( const action_state_t* s ) const
  {
    int num_ticks_ = base_num_ticks;
    if ( debug_cast<const rapid_fire_state_t*>( s ) -> double_tapped )
      num_ticks_ = as<int>(num_ticks_ * (1 + p() -> talents.double_tap -> effectN( 3 ).percent()));
    return num_ticks_;
  }

  double recharge_multiplier() const override
  {
    double m = hunter_spell_t::recharge_multiplier();

    // XXX [8.1]: Spell Data indicates that it's reducing Rapid Fire by 240% (20s/3.4 = 5.88s)
    // m /= 1.0 + .6;  // The information from the bluepost
    // m /= 1.0 + 2.4; // The bugged (in spelldata) value for Rapid Fire.
    if ( p() -> buffs.trueshot -> check() )
      m /= 1.0 + p() -> specs.trueshot -> effectN( 1 ).percent();

    return m;
  }

  action_state_t* new_state() override
  {
    return new rapid_fire_state_t( this, target );
  }

  void snapshot_state( action_state_t* s, dmg_e type ) override
  {
    hunter_spell_t::snapshot_state( s, type );
    debug_cast<rapid_fire_state_t*>( s ) -> double_tapped = p() -> buffs.double_tap -> check();
  }
};

// Explosive Shot  ====================================================================

struct explosive_shot_t: public hunter_ranged_attack_t
{
  struct explosive_shot_aoe_t: hunter_ranged_attack_t
  {
    explosive_shot_aoe_t( const std::string& n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> find_spell( 212680 ) )
    {
      background = true;
      dual = true;
      aoe = -1;
    }
  };

  explosive_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "explosive_shot", p, p -> talents.explosive_shot )
  {
    parse_options( options_str );

    may_miss = false;
    hasted_ticks = false;

    tick_action = p -> get_background_action<explosive_shot_aoe_t>( "explosive_shot_aoe" );
    tick_action -> stats = stats;
  }
};

// Serpent Sting (Marksmanship) ==============================================

struct serpent_sting_mm_t: public hunter_ranged_attack_t
{
  serpent_sting_mm_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "serpent_sting", p, p -> talents.serpent_sting )
  {
    parse_options( options_str );

    hasted_ticks = true;
  }
};

//==============================
// Survival attacks
//==============================

// Melee attack ==============================================================

struct melee_t : public auto_attack_base_t<melee_attack_t>
{
  melee_t( hunter_t* player ) :
    auto_attack_base_t( "auto_attack_mh", player )
  {
    school             = SCHOOL_PHYSICAL;
    weapon_multiplier  = 1.0;
    may_glance         = true;

    // technically there is a separate effect for auto attacks, but meh
    affected_by.coordinated_assault = true;
  }
};

// Internal Bleeding (Wildfire Infusion) ===============================================

struct internal_bleeding_t
{
  struct internal_bleeding_action_t: hunter_ranged_attack_t
  {
    internal_bleeding_action_t( const std::string& n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> find_spell( 270343 ) )
    {
      dual = true;
      dot_max_stack = as<int>( data().max_stacks() );
      hasted_ticks = true;
    }
  };
  internal_bleeding_action_t* action = nullptr;

  internal_bleeding_t( hunter_t* p )
  {
    if ( p -> talents.wildfire_infusion -> ok() )
      action = p -> get_background_action<internal_bleeding_action_t>( "internal_bleeding" );
  }

  void trigger( const action_state_t* s )
  {
    if ( action )
    {
      auto td = action->td( s -> target );
      if (td -> dots.shrapnel_bomb -> is_ticking())
      {
        action -> set_target( s -> target );
        action -> execute();
      }
    }
  }
};

// Raptor Strike (Base) ================================================================
// Shared part between Raptor Strike & Mongoose Bite

struct melee_focus_spender_t: hunter_melee_attack_t
{
  struct latent_poison_t: hunter_spell_t
  {
    latent_poison_t( const std::string& n, hunter_t* p ):
      hunter_spell_t( n, p, p -> find_spell( 273289 ) )
    {}

    void trigger( player_t* target )
    {
      auto debuff = td( target ) -> debuffs.latent_poison;
      if ( ! debuff -> check() )
        return;

      base_dd_min = base_dd_max = debuff -> check_stack_value();
      set_target( target );
      execute();

      debuff -> expire();
    }
  };

  internal_bleeding_t internal_bleeding;
  latent_poison_t* latent_poison = nullptr;
  timespan_t wilderness_survival_reduction;

  melee_focus_spender_t( const std::string& n, hunter_t* p, const spell_data_t* s ):
    hunter_melee_attack_t( n, p, s ),
    internal_bleeding( p ),
    wilderness_survival_reduction( p -> azerite.wilderness_survival.spell() -> effectN( 1 ).time_value() )
  {
    if ( p -> azerite.latent_poison.ok() )
      latent_poison = p -> get_background_action<latent_poison_t>( "latent_poison" );
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    p() -> buffs.vipers_venom -> trigger();
    if ( p() -> buffs.coordinated_assault -> check() )
      p() -> buffs.blur_of_talons -> trigger();
    p() -> buffs.primeval_intuition -> trigger();

    trigger_birds_of_prey( p(), target );
    if ( wilderness_survival_reduction != 0_ms )
      p() -> cooldowns.wildfire_bomb -> adjust( -wilderness_survival_reduction );
  }

  void impact( action_state_t* s ) override
  {
    hunter_melee_attack_t::impact( s );

    internal_bleeding.trigger( s );

    if ( latent_poison )
      latent_poison -> trigger( s -> target );
  }

  bool ready() override
  {
    const bool has_eagle = p() -> buffs.aspect_of_the_eagle -> check();
    return ( range > 10 ? has_eagle : !has_eagle ) && hunter_melee_attack_t::ready();
  }
};

// Mongoose Bite =======================================================================

struct mongoose_bite_base_t: melee_focus_spender_t
{
  struct {
    std::array<proc_t*, 7> at_fury;
  } stats_;

  mongoose_bite_base_t( const std::string& n, hunter_t* p, spell_data_ptr_t s ):
    melee_focus_spender_t( n, p, s )
  {
    base_dd_adder += p -> azerite.wilderness_survival.value( 2 );

    for ( size_t i = 0; i < stats_.at_fury.size(); i++ )
      stats_.at_fury[ i ] = p -> get_proc( "bite_at_" + std::to_string( i ) + "_fury" );

    background = ! p -> talents.mongoose_bite -> ok();
  }

  void execute() override
  {

    melee_focus_spender_t::execute();

    stats_.at_fury[ p() -> buffs.mongoose_fury -> check() ] -> occur();

    p() -> buffs.mongoose_fury -> trigger();
  }

  double action_multiplier() const override
  {
    double am = melee_focus_spender_t::action_multiplier();

    am *= 1.0 + p() -> buffs.mongoose_fury -> stack_value();

    return am;
  }
};

struct mongoose_bite_t : mongoose_bite_base_t
{
  mongoose_bite_t( hunter_t* p, const std::string& options_str ):
    mongoose_bite_base_t( "mongoose_bite", p, p -> talents.mongoose_bite )
  {
    parse_options( options_str );
  }
};

struct mongoose_bite_eagle_t : mongoose_bite_base_t
{
  mongoose_bite_eagle_t( hunter_t* p, const std::string& options_str ):
    mongoose_bite_base_t( "mongoose_bite_eagle", p, p -> find_spell( 265888 ) )
  {
    parse_options( options_str );
  }
};

// Flanking Strike =====================================================================

struct flanking_strike_t: hunter_melee_attack_t
{
  struct flanking_strike_damage_t : hunter_melee_attack_t
  {
    flanking_strike_damage_t( const std::string& n, hunter_t* p ):
      hunter_melee_attack_t( n, p, p -> find_spell( 269752 ) )
    {
      background = true;
      dual = true;
    }
  };
  flanking_strike_damage_t* damage;

  flanking_strike_t( hunter_t* p, const std::string& options_str ):
    hunter_melee_attack_t( "flanking_strike", p, p -> talents.flanking_strike ),
    damage( p -> get_background_action<flanking_strike_damage_t>( "flanking_strike_damage" ) )
  {
    parse_options( options_str );

    base_teleport_distance  = data().max_range();
    movement_directionality = MOVEMENT_OMNI;
    may_parry = may_dodge = may_block = false;

    add_child( damage );
  }

  void init_finished() override
  {
    for ( auto pet : p() -> pet_list )
      add_pet_stats( pet, { "flanking_strike" } );

    hunter_melee_attack_t::init_finished();
  }

  double cast_regen( const action_state_t* s ) const override
  {
    return hunter_melee_attack_t::cast_regen( s ) + damage -> composite_energize_amount( nullptr );
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    if ( p() -> main_hand_weapon.group() == WEAPON_2H )
    {
      damage -> set_target( target );
      damage -> execute();
    }

    if ( auto pet = p() -> pets.main )
    {
      pet -> active.flanking_strike -> set_target( target );
      pet -> active.flanking_strike -> execute();
    }
  }
};

// Carve (Base) ======================================================================
// Shared part between Carve & Butchery

struct carve_base_t: public hunter_melee_attack_t
{
  const timespan_t wfb_reduction;
  const int wfb_reduction_target_cap;
  internal_bleeding_t internal_bleeding;

  carve_base_t( const std::string& n, hunter_t* p, const spell_data_t* s,
                timespan_t wfb_reduction, int wfb_reduction_target_cap ):
    hunter_melee_attack_t( n, p, s ),
    wfb_reduction( wfb_reduction ),
    wfb_reduction_target_cap( wfb_reduction_target_cap ),
    internal_bleeding( p )
  {
    aoe = -1;
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    auto reduction = wfb_reduction * std::min( num_targets_hit, wfb_reduction_target_cap );
    p() -> cooldowns.wildfire_bomb -> adjust( -reduction, true );
  }

  void impact( action_state_t* s ) override
  {
    hunter_melee_attack_t::impact( s );

    trigger_birds_of_prey( p(), s -> target );
    internal_bleeding.trigger( s );
  }
};

// Carve =============================================================================

struct carve_t: public carve_base_t
{
  carve_t( hunter_t* p, const std::string& options_str ):
    carve_base_t( "carve", p, p -> specs.carve ,
                  p -> specs.carve -> effectN( 2 ).time_value(),
                  as<int>(p -> specs.carve -> effectN( 3 ).base_value() ))
  {
    parse_options( options_str );

    if ( p -> talents.butchery -> ok() )
      background = true;
  }
};

// Butchery ==========================================================================

struct butchery_t: public carve_base_t
{
  butchery_t( hunter_t* p, const std::string& options_str ):
    carve_base_t( "butchery", p, p -> talents.butchery,
                  p -> talents.butchery -> effectN( 2 ).time_value() ,
                  as<int>(p -> talents.butchery -> effectN( 3 ).base_value() ))
  {
    parse_options( options_str );
  }
};

// Raptor Strike =====================================================================

struct raptor_strike_base_t: public melee_focus_spender_t
{
  raptor_strike_base_t( const std::string& n, hunter_t* p, spell_data_ptr_t s ):
    melee_focus_spender_t( n, p, s )
  {
    base_dd_adder += p -> azerite.wilderness_survival.value( 3 );
    base_multiplier *= 1.0 + p -> find_spell( 262839 ) -> effectN( 1 ).percent(); // Raptor Strike (Rank 2)

    background = p -> talents.mongoose_bite -> ok();
  }

  void execute() override
  {
    melee_focus_spender_t::execute();

    p() -> buffs.tip_of_the_spear -> expire();
  }

  double action_multiplier() const override
  {
    double am = melee_focus_spender_t::action_multiplier();

    am *= 1.0 + p() -> buffs.tip_of_the_spear -> stack_value();

    return am;
  }
};

struct raptor_strike_t: public raptor_strike_base_t
{
  raptor_strike_t( hunter_t* p, const std::string& options_str ):
    raptor_strike_base_t( "raptor_strike", p, p -> specs.raptor_strike )
  {
    parse_options( options_str );
  }
};

struct raptor_strike_eagle_t: public raptor_strike_base_t
{
  raptor_strike_eagle_t( hunter_t* p, const std::string& options_str ):
    raptor_strike_base_t( "raptor_strike_eagle", p, p -> find_spell( 265189 ) )
  {
    parse_options( options_str );
  }
};

// Harpoon ==================================================================

struct harpoon_t: public hunter_melee_attack_t
{
  struct terms_of_engagement_t : hunter_ranged_attack_t
  {
    terms_of_engagement_t( const std::string& n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> find_spell( 271625 ) )
    {
      dual = true;
      may_parry = may_dodge = may_block = false;
    }

    void impact( action_state_t* s ) override
    {
      hunter_ranged_attack_t::impact( s );

      p() -> buffs.terms_of_engagement -> trigger();
    }
  };
  terms_of_engagement_t* terms_of_engagement = nullptr;

  harpoon_t( hunter_t* p, const std::string& options_str ):
    hunter_melee_attack_t( "harpoon", p, p -> specs.harpoon )
  {
    parse_options( options_str );

    harmful = false;
    base_teleport_distance  = data().max_range();
    movement_directionality = MOVEMENT_OMNI;
    may_parry = may_dodge = may_block = false;

    cooldown -> duration += p -> find_spell( 231550 ) -> effectN( 1 ).time_value(); // Harpoon (Rank 2)

    if ( p -> talents.terms_of_engagement -> ok() )
    {
      terms_of_engagement = p -> get_background_action<terms_of_engagement_t>( "harpoon_terms_of_engagement" );
      add_child( terms_of_engagement );
    }

    if ( p -> main_hand_weapon.group() != WEAPON_2H )
      background = true;
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    if ( terms_of_engagement )
    {
      terms_of_engagement -> set_target( target );
      terms_of_engagement -> execute();
    }
  }

  bool ready() override
  {
    // XXX: disable this for now to actually make it usable without explicit apl support for movement
    //if ( p() -> current.distance_to_move < data().min_range() )
    //  return false;

    return hunter_melee_attack_t::ready();
  }
};

// Serpent Sting (Survival) ===================================================

struct serpent_sting_sv_t: public hunter_ranged_attack_t
{
  serpent_sting_sv_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "serpent_sting", p, p -> find_spell( 259491 ) )
  {
    parse_options( options_str );

    tick_zero = false;
    hasted_ticks = true;

    if ( p -> talents.hydras_bite -> ok() )
      aoe = 1 + static_cast<int>( p -> talents.hydras_bite -> effectN( 1 ).base_value() );

    base_td_multiplier *= 1.0 + p -> talents.hydras_bite -> effectN( 2 ).percent();
  }

  void init() override
  {
    hunter_ranged_attack_t::init();

    update_flags &= ~STATE_HASTE;

    if ( action_t* lp = p() -> find_action( "latent_poison" ) )
      add_child( lp );
  }

  void execute() override
  {
    // have to always reset target_cache because of smart targeting
    if ( is_aoe() )
      target_cache.is_valid = false;

    hunter_ranged_attack_t::execute();

    p() -> buffs.vipers_venom -> up(); // benefit tracking
    p() -> buffs.vipers_venom -> decrement();
  }

  double cost() const override
  {
    if ( p() -> buffs.vipers_venom -> check() )
      return 0;

    return hunter_ranged_attack_t::cost();
  }

  double action_da_multiplier() const override
  {
    double m = hunter_ranged_attack_t::action_da_multiplier();

    m *= 1.0 + p() -> buffs.vipers_venom -> check_value();

    return m;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }

  void assess_damage( dmg_e type, action_state_t* s ) override
  {
    hunter_ranged_attack_t::assess_damage( type, s );

    if ( s -> result_amount > 0 && p() -> azerite.latent_poison.ok() )
      td( s -> target ) -> debuffs.latent_poison -> trigger();
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    hunter_ranged_attack_t::available_targets( tl );

    if ( is_aoe() && tl.size() > 1 )
    {
      // 04-07-2018: HB smart targeting simply prefers targets without serpent
      // sting (instead of the ones with the lowest remaining duration)
      // simply move targets without ss to the front of the list
      auto start = tl.begin();
      std::partition( *start == target ? std::next( start ) : start, tl.end(),
        [ this ]( player_t* t ) {
          return !( this -> td( t ) -> dots.serpent_sting -> is_ticking() );
        } );
    }

    return tl.size();
  }
};

// Chakrams ===================================================================

struct chakrams_t : public hunter_ranged_attack_t
{
  /**
   * In game Chakrams is actually around 5 different spells with 3 'damage' spells alone:
   *  259398 - the actual 'projectile' that hits the main target, once, and returns
   *  259396 - the 'aoe' part, does the damage on each projectile impact, except the main target
   *  267666 - the second spell hitting the main target on impact
   *
   * Our implementation here does everything using only 259398 with a bit of
   * multipliers fiddling. While it does not exactly match the in-game behaviour
   * it produces exactly the same numbers.
   *
   * The big difference to the game are dynamic multipliers because of the timing
   * of impacts. In-game only 259398, the single hit to the main target, is calculated
   * on execute. Every other spell is calculated (and 'executed') on the projectile
   * impacting the respective target.
   * Unfortunately we don't have support for 'travelling' projectiles (or
   * for any non-circular aoe for that matter).
   * Fortunately Survival is melee, so the impact of this should be pretty low.
   */

  struct chakrams_damage_t : public hunter_ranged_attack_t
  {
    chakrams_damage_t( const std::string& n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> talents.chakrams -> effectN( 1 ).trigger() )
    {
      dual = true;

      // Chakrams hits all targets in it's path, as we don't have support for this
      // just make it hit everything.
      aoe = -1;
      radius = 0;

      base_multiplier *= 2.0;
      base_aoe_multiplier = 0.5;
    }
  };
  chakrams_damage_t* damage = nullptr;

  chakrams_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "chakrams", p, p -> talents.chakrams )
  {
    parse_options( options_str );

    damage = p -> get_background_action<chakrams_damage_t>( "chakrams_damage" );
    add_child( damage );
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    damage -> set_target( target );
    damage -> execute();
    damage -> execute(); // to simulate the 'return' & hitting the main target twice
  }
};

} // end attacks

// ==========================================================================
// Hunter Spells
// ==========================================================================

namespace spells
{

// Base Interrupt ===========================================================

struct interrupt_base_t: public hunter_spell_t
{
  interrupt_base_t( const std::string &n, hunter_t* p, const spell_data_t* s ):
    hunter_spell_t( n, p, s )
  {
    may_miss = may_block = may_dodge = may_parry = false;
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( ! candidate_target -> debuffs.casting || ! candidate_target -> debuffs.casting -> check() ) return false;
    return hunter_spell_t::target_ready( candidate_target );
  }
};

// A Murder of Crows ========================================================

struct moc_t : public hunter_spell_t
{
  struct peck_t : public hunter_ranged_attack_t
  {
    peck_t( const std::string& n, hunter_t* p ) :
      hunter_ranged_attack_t( n, p, p -> find_spell( 131900 ) )
    {
      may_parry = may_block = may_dodge = false;
      travel_speed = 0.0;
    }
  };

  moc_t( hunter_t* p, const std::string& options_str ) :
    hunter_spell_t( "a_murder_of_crows", p, p -> talents.a_murder_of_crows )
  {
    parse_options( options_str );

    hasted_ticks = false;
    may_miss = may_crit = false;

    tick_zero = true;

    tick_action = p -> get_background_action<peck_t>( "crow_peck" );
    starved_proc = p -> get_proc( "starved: a_murder_of_crows" );
  }
};

//==============================
// Shared spells
//==============================

// Summon Pet ===============================================================

struct summon_pet_t: public hunter_spell_t
{
  std::string pet_name;
  bool opt_disabled;
  pet_t* pet;

  summon_pet_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "summon_pet", p, p -> find_spell( 883 ) ),
    opt_disabled( false ), pet( nullptr )
  {
    add_option( opt_string( "name", pet_name ) );
    parse_options( options_str );

    harmful = may_hit = false;
    callbacks = false;
    ignore_false_positive = true;

    if ( pet_name.empty() )
      pet_name = p -> options.summon_pet_str;
    opt_disabled = util::str_compare_ci( pet_name, "disabled" );
  }

  void init_finished() override
  {
    if ( ! pet && ! opt_disabled )
      pet = player -> find_pet( pet_name );

    if ( ! pet && p() -> specialization() != HUNTER_MARKSMANSHIP )
    {
      throw std::invalid_argument(fmt::format("Unable to find pet '{}' for summons.", pet_name));
    }

    hunter_spell_t::init_finished();
  }

  void execute() override
  {
    hunter_spell_t::execute();
    pet -> type = PLAYER_PET;
    pet -> summon();

    if ( p() -> main_hand_attack ) p() -> main_hand_attack -> cancel();
  }

  bool ready() override
  {
    if ( opt_disabled || p() -> pets.main == pet )
      return false;

    return hunter_spell_t::ready();
  }
};

// Tar Trap =====================================================================

struct tar_trap_t : public hunter_spell_t
{
  tar_trap_t( hunter_t* p, const std::string& options_str ) :
    hunter_spell_t( "tar_trap", p, p -> find_class_spell( "Tar Trap" ) )
  {
    parse_options( options_str );

    cooldown -> duration = data().cooldown();
  }
};

// Freezing Trap =====================================================================

struct freezing_trap_t : public hunter_spell_t
{
  freezing_trap_t( hunter_t* p, const std::string& options_str ) :
    hunter_spell_t( "freezing_trap", p, p -> find_class_spell( "Freezing Trap" ) )
  {
    parse_options( options_str );

    cooldown -> duration = data().cooldown();
  }
};

// Counter Shot ======================================================================

struct counter_shot_t: public interrupt_base_t
{
  counter_shot_t( hunter_t* p, const std::string& options_str ):
    interrupt_base_t( "counter_shot", p, p -> find_specialization_spell( "Counter Shot" ) )
  {
    parse_options( options_str );
  }
};

// Kill Command =============================================================

struct kill_command_t: public hunter_spell_t
{
  proc_t* flankers_advantage;
  struct {
    real_ppm_t* rppm = nullptr;
    proc_t* proc;
    gain_t* gain;
    double gain_amount;
  } dire_consequences;

  kill_command_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "kill_command", p, p -> specs.kill_command ),
    flankers_advantage( p -> get_proc( "flankers_advantage" ) )
  {
    parse_options( options_str );

    cooldown -> charges += static_cast<int>( p -> talents.alpha_predator -> effectN( 1 ).base_value() );

    if ( p -> azerite.dire_consequences.ok() )
    {
      dire_consequences.rppm = p -> get_rppm( "dire_consequences", 1.0, 1.0, RPPM_ATTACK_SPEED );
      if ( p -> specialization() == HUNTER_BEAST_MASTERY )
        dire_consequences.rppm -> set_modifier( .65 );
      dire_consequences.proc = p -> get_proc( "Dire Consequences" );
      dire_consequences.gain = p -> get_gain( "dire_beast_(dc)" );
      dire_consequences.gain_amount = p -> find_spell( 120694 ) -> effectN( 1 ).base_value() +
                                      p -> talents.scent_of_blood -> effectN( 1 ).base_value();
    }
  }

  void init_finished() override
  {
    for ( auto pet : p() -> pet_list )
      add_pet_stats( pet, { "kill_command" } );

    hunter_spell_t::init_finished();
  }

  void execute() override
  {
    hunter_spell_t::execute();

    for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p() -> pets.main, p() -> pets.animal_companion ) )
    {
      pet -> active.kill_command -> set_target( target );
      pet -> active.kill_command -> execute();
    }

    p() -> buffs.tip_of_the_spear -> trigger();

    if ( p() -> specialization() == HUNTER_SURVIVAL )
    {
      double chance = data().effectN( 2 ).percent();
      if ( p() -> buffs.coordinated_assault -> check() )
        chance += p() -> specs.coordinated_assault -> effectN( 4 ).percent();
      if ( td( target ) -> dots.pheromone_bomb -> is_ticking() )
        chance += p() -> find_spell( 270323 ) -> effectN( 2 ).percent();
      if ( rng().roll( chance ) )
      {
        flankers_advantage -> occur();
        cooldown -> reset( true );
      }
    }

    // XXX: Not sure if the trigger comes before the icd check or after. Typically it's the former afaik.
    if ( dire_consequences.rppm && dire_consequences.rppm -> trigger() )
    {
      p() -> pets.dc_dire_beast.spawn( pets::dire_beast_duration( p() ).first );
      p() -> resource_gain( RESOURCE_FOCUS, dire_consequences.gain_amount, dire_consequences.gain );
      dire_consequences.proc -> occur();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( p() -> pets.main &&
         p() -> pets.main -> active.kill_command -> target_ready( candidate_target ) )
      return hunter_spell_t::target_ready( candidate_target );

    return false;
  }

  bool ready() override
  {
    if ( p() -> pets.main &&
         p() -> pets.main -> active.kill_command -> ready() ) // Range check from the pet.
    {
        return hunter_spell_t::ready();
    }

    return false;
  }

  expr_t* create_expression(const std::string& expression_str) override
  {
    // this is somewhat unfortunate but we can't get at the pets dot in any other way
    auto splits = util::string_split( expression_str, "." );
    if ( splits.size() == 2 && splits[ 0 ] == "bloodseeker" && splits[ 1 ] == "remains" )
    {
      return make_fn_expr( expression_str, [ this ] () {
          if ( auto pet = p() -> pets.main )
            return pet -> get_target_data( target ) -> dots.bloodseeker -> remains();
          return 0_ms;
        } );
    }

    return hunter_spell_t::create_expression( expression_str );
  }
};

//==============================
// Beast Mastery spells
//==============================

// Dire Beast ===============================================================

struct dire_beast_t: public hunter_spell_t
{
  dire_beast_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "dire_beast", p, p -> talents.dire_beast )
  {
    parse_options( options_str );

    harmful = may_hit = false;

    parse_effect_data( p -> find_spell( 120694 ) -> effectN( 1 ) );
    energize_amount += p -> talents.scent_of_blood -> effectN( 1 ).base_value();
  }

  void init_finished() override
  {
    add_pet_stats( p() -> pets.dire_beast, { "dire_beast_melee", "stomp" } );

    hunter_spell_t::init_finished();
  }

  void execute() override
  {
    hunter_spell_t::execute();

    timespan_t summon_duration;
    int base_attacks_per_summon;
    std::tie( summon_duration, base_attacks_per_summon ) = pets::dire_beast_duration( p() );

    sim -> print_debug( "Dire Beast summoned with {:4.1f} autoattacks", base_attacks_per_summon );

    p() -> pets.dire_beast -> summon( summon_duration );
  }
};

// Bestial Wrath ============================================================

struct bestial_wrath_t: public hunter_spell_t
{
  timespan_t precast_time = 0_ms;

  bestial_wrath_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "bestial_wrath", player, player -> specs.bestial_wrath )
  {
    add_option( opt_timespan( "precast_time", precast_time ) );
    parse_options( options_str );
    harmful = may_hit = false;

    precast_time = clamp( precast_time, 0_ms, data().duration() );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    trigger_buff( p() -> buffs.bestial_wrath, precast_time );
    trigger_buff( p() -> buffs.haze_of_rage, precast_time );

    for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p() -> pets.main, p() -> pets.animal_companion ) )
      trigger_buff( pet -> buffs.bestial_wrath, precast_time );

    adjust_precast_cooldown( precast_time );
  }

  bool ready() override
  {
    if ( !p() -> pets.main )
      return false;

    return hunter_spell_t::ready();
  }
};

// Aspect of the Wild =======================================================

struct aspect_of_the_wild_t: public hunter_spell_t
{
  timespan_t precast_time = 0_ms;

  aspect_of_the_wild_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "aspect_of_the_wild", p, p -> specs.aspect_of_the_wild )
  {
    add_option( opt_timespan( "precast_time", precast_time ) );
    parse_options( options_str );

    harmful = may_hit = false;
    dot_duration = 0_ms;

    precast_time = clamp( precast_time, 0_ms, data().duration() );
  }

  void schedule_execute( action_state_t* s ) override
  {
    // AotW buff is applied before the spell is cast, allowing it to
    // reduce GCD of the action that triggered it.
    if ( !precombat )
      p() -> buffs.aspect_of_the_wild -> trigger();

    hunter_spell_t::schedule_execute( s );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    // Precombat actions skip schedule_execute, so the buff needs to be
    // triggered here for precombat actions.
    if ( precombat )
      trigger_buff( p() -> buffs.aspect_of_the_wild, precast_time );

    if ( trigger_buff( p() -> buffs.primal_instincts, precast_time ) )
      p() -> cooldowns.barbed_shot -> reset( true );

    adjust_precast_cooldown( precast_time );
  }
};

// Stampede =================================================================

struct stampede_t: public hunter_spell_t
{
  struct stampede_tick_t: public hunter_spell_t
  {
    stampede_tick_t( hunter_t* p ):
      hunter_spell_t( "stampede_tick", p, p -> find_spell( 201594 ) )
    {
      aoe = -1;
      background = true;
      may_crit = true;
    }
  };

  stampede_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "stampede", p, p -> talents.stampede )
  {
    parse_options( options_str );

    base_tick_time = 667_ms;
    dot_duration = data().duration();
    hasted_ticks = false;
    radius = 8;
    school = SCHOOL_PHYSICAL;

    tick_action = new stampede_tick_t( p );
  }
};

// Spitting Cobra ====================================================================

struct spitting_cobra_t: public hunter_spell_t
{
  spitting_cobra_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "spitting_cobra", p, p -> talents.spitting_cobra )
  {
    parse_options( options_str );

    harmful = may_hit = false;
    dot_duration = 0_ms;
  }

  void init_finished() override
  {
    add_pet_stats( p() -> pets.spitting_cobra, { "cobra_spit" } );

    hunter_spell_t::init_finished();
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> pets.spitting_cobra -> summon( data().duration() );

    p() -> buffs.spitting_cobra -> trigger();
  }
};

//==============================
// Marksmanship spells
//==============================

// Trueshot =================================================================

struct trueshot_t: public hunter_spell_t
{
  timespan_t precast_time = 0_ms;

  trueshot_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "trueshot", p, p -> specs.trueshot )
  {
    add_option( opt_timespan( "precast_time", precast_time ) );
    parse_options( options_str );
    harmful = may_hit = false;

    precast_time = clamp( precast_time, 0_ms, data().duration() );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    trigger_buff( p() -> buffs.trueshot, precast_time );
    trigger_buff( p() -> buffs.unerring_vision_driver, precast_time );

    adjust_precast_cooldown( precast_time );
  }
};

// Hunter's Mark ============================================================

struct hunters_mark_t: public hunter_spell_t
{
  hunters_mark_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "hunters_mark", p, p -> talents.hunters_mark )
  {
    parse_options( options_str );

    harmful = may_hit = false;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    if ( p() -> current_hunters_mark_target != nullptr )
      td( p() -> current_hunters_mark_target ) -> debuffs.hunters_mark -> expire();

    p() -> current_hunters_mark_target = target;
    td( target ) -> debuffs.hunters_mark -> trigger();
  }
};

// Double Tap ===================================================================

struct double_tap_t: public hunter_spell_t
{
  timespan_t precast_time = 0_ms;

  double_tap_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "double_tap", p, p -> talents.double_tap )
  {
    add_option( opt_timespan( "precast_time", precast_time ) );
    parse_options( options_str );

    harmful = may_hit = false;
    dot_duration = 0_ms;

    precast_time = clamp( precast_time, 0_ms, data().duration() );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    trigger_buff( p() -> buffs.double_tap, precast_time );

    adjust_precast_cooldown( precast_time );
  }
};

//==============================
// Survival spells
//==============================

// Coordinated Assault ==============================================================

struct coordinated_assault_t: public hunter_spell_t
{
  coordinated_assault_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "coordinated_assault", p, p -> specs.coordinated_assault )
  {
    parse_options( options_str );

    harmful = may_hit = false;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.coordinated_assault -> trigger();
  }
};

// Steel Trap =======================================================================

struct steel_trap_t: public hunter_spell_t
{
  struct steel_trap_impact_t : public hunter_spell_t
  {
    steel_trap_impact_t( const std::string& n, hunter_t* p ):
      hunter_spell_t( n, p, p -> find_spell( 162487 ) )
    {
      background = true;
      dual = true;
    }
  };

  steel_trap_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "steel_trap", p, p -> talents.steel_trap )
  {
    parse_options( options_str );

    harmful = may_hit = false;

    impact_action = p -> get_background_action<steel_trap_impact_t>( "steel_trap_impact" );
    add_child( impact_action );
  }
};

// Wildfire Bomb ==============================================================

struct wildfire_bomb_t: public hunter_spell_t
{
  struct wildfire_cluster_t : public hunter_spell_t
  {
    wildfire_cluster_t( const std::string& n, hunter_t* p ):
      hunter_spell_t( n, p, p -> find_spell( 272745 ) )
    {
      aoe = -1;
      base_dd_min = base_dd_max = p -> azerite.wildfire_cluster.value( 1 );
    }
  };

  struct bomb_base_t : public hunter_spell_t
  {
    struct dot_action_t : public hunter_spell_t
    {
      dot_action_t( const std::string& n, hunter_t* p, const spell_data_t* s ):
        hunter_spell_t( n, p, s )
      {
        dual = true;
        hasted_ticks = false;
      }

      // does not pandemic
      timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const override
      {
        return dot -> time_to_next_tick() + triggered_duration;
      }
    };
    dot_action_t* dot_action;

    bomb_base_t( const std::string& n, wildfire_bomb_t* a, const spell_data_t* s, const std::string& dot_n, const spell_data_t* dot_s ):
      hunter_spell_t( n, a -> p(), s ),
      dot_action( a -> p() -> get_background_action<dot_action_t>( dot_n, dot_s ) )
    {
      dual = true;
      aoe = -1;
      // XXX: It's actually a circle + cone, but we sadly can't really model that
      radius = 5.0;

      base_dd_multiplier *= 1.0 + p() -> talents.guerrilla_tactics -> effectN( 2 ).percent();

      a -> add_child( this );
      a -> add_child( dot_action );
    }

    void impact( action_state_t* s ) override
    {
      hunter_spell_t::impact( s );
      dot_action -> set_target( s -> target );
      dot_action -> execute();
    }
  };

  struct wildfire_bomb_damage_t : public bomb_base_t
  {
    wildfire_bomb_damage_t( const std::string& n, hunter_t* p, wildfire_bomb_t* a ):
      bomb_base_t( n, a, p -> find_spell( 265157 ), "wildfire_bomb_dot", p -> find_spell( 269747 ) )
    {}
  };

  struct shrapnel_bomb_t : public bomb_base_t
  {
    shrapnel_bomb_t( const std::string& n, hunter_t* p, wildfire_bomb_t* a ):
      bomb_base_t( n, a, p -> find_spell( 270338 ), "shrapnel_bomb", p -> find_spell( 270339 ) )
    {
      attacks::internal_bleeding_t internal_bleeding( p );
      a -> add_child( internal_bleeding.action );
    }
  };

  struct pheromone_bomb_t : public bomb_base_t
  {
    pheromone_bomb_t( const std::string& n, hunter_t* p, wildfire_bomb_t* a ):
      bomb_base_t( n, a, p -> find_spell( 270329 ), "pheromone_bomb", p -> find_spell( 270332 ) )
    {}
  };

  struct volatile_bomb_t : public bomb_base_t
  {
    struct violent_reaction_t : public hunter_spell_t
    {
      violent_reaction_t( const std::string& n, hunter_t* p ):
        hunter_spell_t( n, p, p -> find_spell( 260231 ) )
      {
        dual = true;
        aoe = -1;
      }
    };
    violent_reaction_t* violent_reaction;

    volatile_bomb_t( const std::string& n, hunter_t* p, wildfire_bomb_t* a ):
      bomb_base_t( n, a, p -> find_spell( 271048 ), "volatile_bomb", p -> find_spell( 271049 ) ),
      violent_reaction( p -> get_background_action<violent_reaction_t>( "violent_reaction" ) )
    {
      a -> add_child( violent_reaction );
    }

    void execute() override
    {
      bomb_base_t::execute();

      if ( td( target ) -> dots.serpent_sting -> is_ticking() )
      {
        violent_reaction -> set_target( target );
        violent_reaction -> execute();
      }
    }

    void impact( action_state_t* s ) override
    {
      bomb_base_t::impact( s );

      td( s -> target ) -> dots.serpent_sting -> refresh_duration();
    }
  };

  std::array<bomb_base_t*, 3> bombs;
  bomb_base_t* current_bomb = nullptr;
  wildfire_cluster_t* wildfire_cluster = nullptr;

  wildfire_bomb_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "wildfire_bomb", p, p -> specs.wildfire_bomb )
  {
    parse_options( options_str );

    may_miss = false;
    school = SCHOOL_FIRE; // for report coloring

    cooldown -> charges += static_cast<int>( p -> talents.guerrilla_tactics -> effectN( 1 ).base_value() );

    if ( p -> talents.wildfire_infusion -> ok() )
    {
      bombs[ WILDFIRE_INFUSION_SHRAPNEL ] =  p -> get_background_action<shrapnel_bomb_t>( "shrapnel_bomb_impact", this );
      bombs[ WILDFIRE_INFUSION_PHEROMONE ] = p -> get_background_action<pheromone_bomb_t>( "pheromone_bomb_impact", this );
      bombs[ WILDFIRE_INFUSION_VOLATILE ] =  p -> get_background_action<volatile_bomb_t>( "volatile_bomb_impact", this );
    }
    else
    {
      bombs[ WILDFIRE_INFUSION_SHRAPNEL ] = p -> get_background_action<wildfire_bomb_damage_t>( "wildfire_bomb_impact", this );
    }

    if ( p -> azerite.wildfire_cluster.ok() )
    {
      wildfire_cluster = p -> get_background_action<wildfire_cluster_t>( "wildfire_cluster" );
      add_child( wildfire_cluster );
    }
  }

  void execute() override
  {
    current_bomb = bombs[ p() -> next_wi_bomb ];

    hunter_spell_t::execute();

    if ( p() -> talents.wildfire_infusion -> ok() )
    {
      // assume that we can't get 2 same bombs in a row
      int slot = rng().range( 2 );
      if ( slot == p() -> next_wi_bomb )
        slot += 2 - p() -> next_wi_bomb;
      p() -> next_wi_bomb = static_cast<wildfire_infusion_e>( slot );
    }
  }

  void impact( action_state_t* s ) override
  {
    hunter_spell_t::impact( s );

    current_bomb -> set_target( s -> target );
    current_bomb -> execute();

    if ( wildfire_cluster )
    {
      // XXX: they are 'missiles' that explode after a short delay, whatever
      wildfire_cluster -> set_target( s -> target );
      for ( int i = 0; i < 3; i++ )
        wildfire_cluster -> execute();
    }
  }
};

// Aspect of the Eagle ======================================================

struct aspect_of_the_eagle_t: public hunter_spell_t
{
  aspect_of_the_eagle_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "aspect_of_the_eagle", p, p -> specs.aspect_of_the_eagle )
  {
    parse_options( options_str );

    harmful = may_hit = false;

    cooldown -> duration *= 1.0 + p -> talents.born_to_be_wild -> effectN( 1 ).percent();
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.aspect_of_the_eagle -> trigger();
  }
};

// Muzzle =============================================================

struct muzzle_t: public interrupt_base_t
{
  muzzle_t( hunter_t* p, const std::string& options_str ):
    interrupt_base_t( "muzzle", p, p -> find_specialization_spell( "Muzzle" ) )
  {
    parse_options( options_str );
  }
};

//end spells
}

namespace actions {

// Auto attack =======================================================================

struct auto_attack_t: public action_t
{
  auto_attack_t( hunter_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "auto_attack", p )
  {
    parse_options( options_str );

    ignore_false_positive = true;
    trigger_gcd = 0_ms;

    if ( p -> main_hand_weapon.type == WEAPON_NONE )
    {
      background = true;
    }
    else if ( p -> main_hand_weapon.group() == WEAPON_RANGED )
    {
      p -> main_hand_attack = new attacks::auto_shot_t( p );
    }
    else
    {
      p -> main_hand_attack = new attacks::melee_t( p );
      range = 5;
    }
  }

  void execute() override
  {
    player -> main_hand_attack -> schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() && !usable_moving() )
      return false;

    return player -> main_hand_attack -> execute_event == nullptr; // not swinging
  }
};

} // namespace actions

hunter_td_t::hunter_td_t( player_t* target, hunter_t* p ):
  actor_target_data_t( target, p ),
  debuffs(),
  dots()
{
  debuffs.hunters_mark =
    make_buff( *this, "hunters_mark", p -> talents.hunters_mark )
      -> set_default_value( p -> talents.hunters_mark -> effectN( 1 ).percent() );

  debuffs.latent_poison =
    make_buff( *this, "latent_poison", p -> find_spell( 273286 ) )
      -> set_default_value( p -> azerite.latent_poison.value( 1 ) )
      -> set_trigger_spell( p -> azerite.latent_poison );

  debuffs.steady_aim =
    make_buff( *this, "steady_aim", p -> find_spell( 277959 ) )
      -> set_default_value( p -> azerite.steady_aim.value( 1 ) )
      -> set_trigger_spell( p -> azerite.steady_aim );

  dots.serpent_sting = target -> get_dot( "serpent_sting", p );
  dots.a_murder_of_crows = target -> get_dot( "a_murder_of_crows", p );
  dots.pheromone_bomb = target -> get_dot( "pheromone_bomb", p );
  dots.shrapnel_bomb = target -> get_dot( "shrapnel_bomb", p );

  target -> callbacks_on_demise.push_back( std::bind( &hunter_td_t::target_demise, this ) );
}

void hunter_td_t::target_demise()
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( source -> sim -> event_mgr.canceled )
    return;

  hunter_t* p = static_cast<hunter_t*>( source );
  if ( p -> talents.a_murder_of_crows -> ok() && dots.a_murder_of_crows -> is_ticking() )
  {
    p -> sim -> print_debug( "{} a_murder_of_crows cooldown reset on target death.", p -> name() );

    p -> cooldowns.a_murder_of_crows -> reset( true );
  }

  if ( debuffs.hunters_mark -> check() )
  {
    p -> resource_gain( RESOURCE_FOCUS, p -> find_spell( 259558 ) -> effectN( 1 ).resource( RESOURCE_FOCUS ), p -> gains.hunters_mark );
    p -> current_hunters_mark_target = nullptr;
  }

  if ( p -> talents.terms_of_engagement -> ok() && damaged )
  {
    p -> sim -> print_debug( "{} harpoon cooldown reset on damaged target death.", p -> name() );
    p -> cooldowns.harpoon -> reset( true );
  }

  damaged = false;
}

/**
 * Hunter specific action expression
 *
 * Use this function for expressions which are bound to an action property such as target, cast_time etc.
 * If you need an expression tied to the player itself use the normal hunter_t::create_expression override.
 */
expr_t* hunter_t::create_action_expression ( action_t& action, const std::string& expression_str )
{
  std::vector<std::string> splits = util::string_split( expression_str, "." );

  //Careful Aim expression
  if ( splits.size() == 1 && splits[ 0 ] == "ca_execute" )
  {
      return make_fn_expr( expression_str, [ this, &action ]
      {
        if ( !talents.careful_aim->ok() )
          return false;

        if (action.target->health_percentage() > talents.careful_aim->effectN( 1 ).base_value() || action.target->health_percentage() < talents.careful_aim->effectN( 2 ).base_value())
          return true;
        else
         return false;
      } );
  }

  return player_t::create_action_expression( action, expression_str );
}

expr_t* hunter_t::create_expression( const std::string& expression_str )
{
  std::vector<std::string> splits = util::string_split( expression_str, "." );

  if ( splits.size() == 3 && splits[ 0 ] == "cooldown" && splits[ 1 ] == "trueshot" )
  {
    if ( splits[ 2 ] == "remains_guess" )
    {
      struct trueshot_remains_guess_t : public expr_t
      {
        hunter_t* hunter;

        trueshot_remains_guess_t( hunter_t* h, const std::string& str ) :
          expr_t( str ), hunter( h )
        { }

        double evaluate() override
        {
          cooldown_t* trueshot_cd = hunter -> cooldowns.trueshot;

          if ( trueshot_cd -> remains() == trueshot_cd -> duration )
            return trueshot_cd -> duration.total_seconds();

          if ( trueshot_cd -> up() )
            return 0.0;

          double reduction = (hunter -> sim -> current_time() - trueshot_cd -> last_start) / (trueshot_cd -> duration - trueshot_cd -> remains());
          return trueshot_cd -> remains().total_seconds() * reduction;
        }
      };
      return new trueshot_remains_guess_t( this, expression_str );
    }
    else if ( splits[ 2 ] == "duration_guess" )
    {
      struct trueshot_duration_guess_t : public expr_t
      {
        hunter_t* hunter;

        trueshot_duration_guess_t( hunter_t* h, const std::string& str ) :
          expr_t( str ), hunter( h )
        { }

        double evaluate() override
        {
          cooldown_t* trueshot_cd = hunter -> cooldowns.trueshot;

          if ( trueshot_cd -> last_charged == 0_ms || trueshot_cd -> remains() == trueshot_cd -> duration )
            return trueshot_cd -> duration.total_seconds();

          if ( trueshot_cd -> up() )
            return ( trueshot_cd -> last_charged - trueshot_cd -> last_start ).total_seconds();

          double reduction = ( hunter -> sim -> current_time() - trueshot_cd -> last_start ) / ( trueshot_cd -> duration - trueshot_cd -> remains() );
          return trueshot_cd -> duration.total_seconds() * reduction;
        }
      };
      return new trueshot_duration_guess_t( this, expression_str );
    }
  }
  else if ( splits.size() == 2 && splits[ 0 ] == "next_wi_bomb" )
  {
    if ( splits[ 1 ] == "shrapnel" )
      return make_fn_expr( expression_str, [ this ]() { return talents.wildfire_infusion -> ok() && next_wi_bomb == WILDFIRE_INFUSION_SHRAPNEL; } );
    if ( splits[ 1 ] == "pheromone" )
      return make_fn_expr( expression_str, [ this ]() { return next_wi_bomb == WILDFIRE_INFUSION_PHEROMONE; } );
    if ( splits[ 1 ] == "volatile" )
      return make_fn_expr( expression_str, [ this ]() { return next_wi_bomb == WILDFIRE_INFUSION_VOLATILE; } );
  }

  return player_t::create_expression( expression_str );
}

// hunter_t::create_action ==================================================

action_t* hunter_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  using namespace attacks;
  using namespace spells;

  if ( name == "a_murder_of_crows"     ) return new                    moc_t( this, options_str );
  if ( name == "aimed_shot"            ) return new             aimed_shot_t( this, options_str );
  if ( name == "arcane_shot"           ) return new            arcane_shot_t( this, options_str );
  if ( name == "aspect_of_the_wild"    ) return new     aspect_of_the_wild_t( this, options_str );
  if ( name == "auto_attack"           ) return new   actions::auto_attack_t( this, options_str );
  if ( name == "auto_shot"             ) return new   actions::auto_attack_t( this, options_str );
  if ( name == "barrage"               ) return new                barrage_t( this, options_str );
  if ( name == "bestial_wrath"         ) return new          bestial_wrath_t( this, options_str );
  if ( name == "bursting_shot"         ) return new          bursting_shot_t( this, options_str );
  if ( name == "butchery"              ) return new               butchery_t( this, options_str );
  if ( name == "chimaera_shot"         ) return new          chimaera_shot_t( this, options_str );
  if ( name == "cobra_shot"            ) return new             cobra_shot_t( this, options_str );
  if ( name == "counter_shot"          ) return new           counter_shot_t( this, options_str );
  if ( name == "dire_beast"            ) return new             dire_beast_t( this, options_str );
  if ( name == "barbed_shot"           ) return new            barbed_shot_t( this, options_str );
  if ( name == "explosive_shot"        ) return new         explosive_shot_t( this, options_str );
  if ( name == "freezing_trap"         ) return new          freezing_trap_t( this, options_str );
  if ( name == "flanking_strike"       ) return new        flanking_strike_t( this, options_str );
  if ( name == "harpoon"               ) return new                harpoon_t( this, options_str );
  if ( name == "kill_command"          ) return new           kill_command_t( this, options_str );
  if ( name == "mongoose_bite"         ) return new          mongoose_bite_t( this, options_str );
  if ( name == "mongoose_bite_eagle"   ) return new    mongoose_bite_eagle_t( this, options_str );
  if ( name == "multishot"             ) return new             multi_shot_t( this, options_str );
  if ( name == "multi_shot"            ) return new             multi_shot_t( this, options_str );
  if ( name == "muzzle"                ) return new                 muzzle_t( this, options_str );
  if ( name == "piercing_shot"         ) return new          piercing_shot_t( this, options_str );
  if ( name == "raptor_strike"         ) return new          raptor_strike_t( this, options_str );
  if ( name == "raptor_strike_eagle"   ) return new    raptor_strike_eagle_t( this, options_str );
  if ( name == "spitting_cobra"        ) return new         spitting_cobra_t( this, options_str );
  if ( name == "stampede"              ) return new               stampede_t( this, options_str );
  if ( name == "steel_trap"            ) return new             steel_trap_t( this, options_str );
  if ( name == "summon_pet"            ) return new             summon_pet_t( this, options_str );
  if ( name == "tar_trap"              ) return new               tar_trap_t( this, options_str );
  if ( name == "trueshot"              ) return new               trueshot_t( this, options_str );
  if ( name == "steady_shot"           ) return new            steady_shot_t( this, options_str );
  if ( name == "rapid_fire"            ) return new             rapid_fire_t( this, options_str );
  if ( name == "hunters_mark"          ) return new           hunters_mark_t( this, options_str );
  if ( name == "double_tap"            ) return new             double_tap_t( this, options_str );
  if ( name == "wildfire_bomb"         ) return new          wildfire_bomb_t( this, options_str );
  if ( name == "coordinated_assault"   ) return new    coordinated_assault_t( this, options_str );
  if ( name == "chakrams"              ) return new               chakrams_t( this, options_str );
  if ( name == "carve"                 ) return new                  carve_t( this, options_str );
  if ( name == "aspect_of_the_eagle"   ) return new    aspect_of_the_eagle_t( this, options_str );

  if ( name == "serpent_sting" )
  {
    if ( specialization() == HUNTER_MARKSMANSHIP )
      return new serpent_sting_mm_t( this, options_str );
    if ( specialization() == HUNTER_SURVIVAL )
      return new serpent_sting_sv_t( this, options_str );
  }

  return player_t::create_action( name, options_str );
}

// hunter_t::create_pet =====================================================

pet_t* hunter_t::create_pet( const std::string& pet_name,
                             const std::string& pet_type )
{
  using namespace pets;

  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  pet_e type = util::parse_pet_type( pet_type );
  if ( type > PET_NONE && type < PET_HUNTER )
    return new pets::hunter_main_pet_t( this, pet_name, type );

  if ( !pet_type.empty() )
  {
    throw std::invalid_argument(fmt::format("Pet '{}' has unknown type '{}'.", pet_name, pet_type ));
  }

  return nullptr;
}

// hunter_t::create_pets ====================================================

void hunter_t::create_pets()
{
  if ( !util::str_compare_ci( options.summon_pet_str, "disabled" ) )
    create_pet( options.summon_pet_str, options.summon_pet_str );

  if ( talents.animal_companion -> ok() )
    pets.animal_companion = new pets::animal_companion_t( this );

  if ( talents.dire_beast -> ok() )
    pets.dire_beast = new pets::dire_critter_t( this );

  if ( talents.spitting_cobra -> ok() )
    pets.spitting_cobra = new pets::spitting_cobra_t( this );

  if ( azerite.dire_consequences.ok() )
  {
    pets.dc_dire_beast.set_creation_callback(
      [] ( hunter_t* p ) { return new pets::dire_critter_t( p, "dire_beast_(dc)" ); });
  }
}

// hunter_t::init_spells ====================================================

void hunter_t::init_spells()
{
  player_t::init_spells();

  // tier 15
  talents.killer_instinct                   = find_talent_spell( "Killer Instinct" );
  talents.animal_companion                  = find_talent_spell( "Animal Companion" );
  talents.dire_beast                        = find_talent_spell( "Dire Beast" );

  talents.master_marksman                   = find_talent_spell( "Master Marksman" );
  talents.serpent_sting                     = find_talent_spell( "Serpent Sting" );

  talents.vipers_venom                      = find_talent_spell( "Viper's Venom" );
  talents.terms_of_engagement               = find_talent_spell( "Terms of Engagement" );
  talents.alpha_predator                    = find_talent_spell( "Alpha Predator" );

  // tier 30
  talents.scent_of_blood                    = find_talent_spell( "Scent of Blood" );
  talents.one_with_the_pack                 = find_talent_spell( "One with the Pack" );
  talents.chimaera_shot                     = find_talent_spell( "Chimaera Shot" );

  talents.careful_aim                       = find_talent_spell( "Careful Aim" );
  talents.volley                            = find_talent_spell( "Volley" );
  talents.explosive_shot                    = find_talent_spell( "Explosive Shot" );

  talents.guerrilla_tactics                 = find_talent_spell( "Guerrilla Tactics" );
  talents.hydras_bite                       = find_talent_spell( "Hydra's Bite" );
  talents.butchery                          = find_talent_spell( "Butchery" );

  // tier 45
  talents.trailblazer                       = find_talent_spell( "Trailblazer" );
  talents.natural_mending                   = find_talent_spell( "Natural Mending" );
  talents.camouflage                        = find_talent_spell( "Camouflage" );

  // tier 60
  talents.venomous_bite                     = find_talent_spell( "Venomous Bite" );
  talents.thrill_of_the_hunt                = find_talent_spell( "Thrill of the Hunt" );
  talents.a_murder_of_crows                 = find_talent_spell( "A Murder of Crows" );

  talents.steady_focus                      = find_talent_spell( "Steady Focus" );
  talents.streamline                        = find_talent_spell( "Streamline" );
  talents.hunters_mark                      = find_talent_spell( "Hunter's Mark" );

  talents.bloodseeker                       = find_talent_spell( "Bloodseeker" );
  talents.steel_trap                        = find_talent_spell( "Steel Trap" );

  // tier 75
  talents.born_to_be_wild                   = find_talent_spell( "Born To Be Wild" );
  talents.posthaste                         = find_talent_spell( "Posthaste" );
  talents.binding_shot                      = find_talent_spell( "Binding Shot" );

  // tier 90
  talents.stomp                             = find_talent_spell( "Stomp" );
  talents.barrage                           = find_talent_spell( "Barrage" );
  talents.stampede                          = find_talent_spell( "Stampede" );

  talents.lethal_shots                      = find_talent_spell( "Lethal Shots" );
  talents.double_tap                        = find_talent_spell( "Double Tap" );

  talents.tip_of_the_spear                  = find_talent_spell( "Tip of the Spear" );
  talents.mongoose_bite                     = find_talent_spell( "Mongoose Bite" );
  talents.flanking_strike                   = find_talent_spell( "Flanking Strike" );

  // tier 100
  talents.killer_cobra                      = find_talent_spell( "Killer Cobra" );
  talents.aspect_of_the_beast               = find_talent_spell( "Aspect of the Beast" );
  talents.spitting_cobra                    = find_talent_spell( "Spitting Cobra" );

  talents.calling_the_shots                 = find_talent_spell( "Calling the Shots" );
  talents.lock_and_load                     = find_talent_spell( "Lock and Load" );
  talents.piercing_shot                     = find_talent_spell( "Piercing Shot" );

  talents.birds_of_prey                     = find_talent_spell( "Birds of Prey" );
  talents.wildfire_infusion                 = find_talent_spell( "Wildfire Infusion" );
  talents.chakrams                          = find_talent_spell( "Chakrams" );

  // Mastery
  mastery.master_of_beasts     = find_mastery_spell( HUNTER_BEAST_MASTERY );
  mastery.sniper_training      = find_mastery_spell( HUNTER_MARKSMANSHIP );
  mastery.spirit_bond          = find_mastery_spell( HUNTER_SURVIVAL );

  // Spec spells
  specs.critical_strikes     = find_spell( 157443 );
  specs.hunter               = find_spell( 137014 );
  specs.beast_mastery_hunter = find_specialization_spell( "Beast Mastery Hunter" );
  specs.marksmanship_hunter  = find_specialization_spell( "Marksmanship Hunter" );
  specs.survival_hunter      = find_specialization_spell( "Survival Hunter" );

  specs.kill_command         = find_specialization_spell( "Kill Command" );

  // Beast Mastery
  specs.aspect_of_the_wild   = find_specialization_spell( "Aspect of the Wild" );
  specs.barbed_shot          = find_specialization_spell( "Barbed Shot" );
  specs.beast_cleave         = find_specialization_spell( "Beast Cleave" );
  specs.bestial_wrath        = find_specialization_spell( "Bestial Wrath" );
  specs.kindred_spirits      = find_specialization_spell( "Kindred Spirits" );
  specs.wild_call            = find_specialization_spell( "Wild Call" );

  // Marksmanship
  specs.aimed_shot           = find_specialization_spell( "Aimed Shot" );
  specs.lone_wolf            = find_specialization_spell( "Lone Wolf" );
  specs.precise_shots        = find_specialization_spell( "Precise Shots" );
  specs.rapid_fire           = find_specialization_spell( "Rapid Fire" );
  specs.trick_shots          = find_specialization_spell( "Trick Shots" );
  specs.trueshot             = find_specialization_spell( "Trueshot" );

  // Survival
  specs.coordinated_assault  = find_specialization_spell( "Coordinated Assault" );
  specs.harpoon              = find_specialization_spell( "Harpoon" );
  specs.raptor_strike        = find_specialization_spell( "Raptor Strike" );
  specs.wildfire_bomb        = find_specialization_spell( "Wildfire Bomb" );
  specs.carve                = find_specialization_spell( "Carve" );
  specs.aspect_of_the_eagle  = find_specialization_spell( "Aspect of the Eagle" );

  // Azerite

  azerite.dance_of_death        = find_azerite_spell( "Dance of Death" );
  azerite.dire_consequences     = find_azerite_spell( "Dire Consequences" );
  azerite.feeding_frenzy        = find_azerite_spell( "Feeding Frenzy" );
  azerite.haze_of_rage          = find_azerite_spell( "Haze of Rage" );
  azerite.primal_instincts      = find_azerite_spell( "Primal Instincts" );
  azerite.serrated_jaws         = find_azerite_spell( "Serrated Jaws" );

  azerite.focused_fire          = find_azerite_spell( "Focused Fire" );
  azerite.in_the_rhythm         = find_azerite_spell( "In The Rhythm" );
  azerite.rapid_reload          = find_azerite_spell( "Rapid Reload" );
  azerite.steady_aim            = find_azerite_spell( "Steady Aim" );
  azerite.surging_shots         = find_azerite_spell( "Surging Shots" );
  azerite.unerring_vision       = find_azerite_spell( "Unerring Vision" );

  azerite.blur_of_talons        = find_azerite_spell( "Blur of Talons" );
  azerite.latent_poison         = find_azerite_spell( "Latent Poison" );
  azerite.primeval_intuition    = find_azerite_spell( "Primeval Intuition" );
  azerite.venomous_fangs        = find_azerite_spell( "Venomous Fangs" );
  azerite.wilderness_survival   = find_azerite_spell( "Wilderness Survival" );
  azerite.wildfire_cluster      = find_azerite_spell( "Wildfire Cluster" );
}

// hunter_t::init_base ======================================================

void hunter_t::init_base_stats()
{
  if ( base.distance < 1 )
  {
    base.distance = 40;
    if ( specialization() == HUNTER_SURVIVAL )
      base.distance = 5;
  }

  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;

  resources.base_regen_per_second[ RESOURCE_FOCUS ] = 10.0;
  for ( auto spell : { specs.marksmanship_hunter, specs.survival_hunter } )
  {
    for ( size_t i = 1; i <= spell -> effect_count(); i++ )
    {
      const spelleffect_data_t& effect = spell -> effectN( i );
      if ( effect.ok() && effect.type() == E_APPLY_AURA && effect.subtype() == A_MOD_POWER_REGEN_PERCENT )
        resources.base_regen_per_second[ RESOURCE_FOCUS ] *= 1.0 + effect.percent();
    }
  }

  resources.base[RESOURCE_FOCUS] = 100 + specs.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS );
  if ( azerite.primeval_intuition.ok() )
    resources.base[RESOURCE_FOCUS] += find_spell( 288571 ) -> effectN( 2 ).base_value();
}

// hunter_t::init_buffs =====================================================

void hunter_t::create_buffs()
{
  player_t::create_buffs();

  // Beast Mastery

  buffs.aspect_of_the_wild =
    make_buff( this, "aspect_of_the_wild", specs.aspect_of_the_wild )
      -> set_cooldown( 0_ms )
      -> set_activated( true )
      -> set_default_value( specs.aspect_of_the_wild -> effectN( 1 ).percent() )
      -> set_tick_callback( [ this ]( buff_t *b, int, const timespan_t& ){
                        resource_gain( RESOURCE_FOCUS, b -> data().effectN( 2 ).resource( RESOURCE_FOCUS ), gains.aspect_of_the_wild );
                        if ( auto pet = pets.main )
                          pet -> resource_gain( RESOURCE_FOCUS, b -> data().effectN( 5 ).resource( RESOURCE_FOCUS ), pet -> gains.aspect_of_the_wild );
                      } );

  buffs.bestial_wrath =
    make_buff( this, "bestial_wrath", specs.bestial_wrath )
      -> set_cooldown( 0_ms )
      -> set_activated( true )
      -> set_default_value( specs.bestial_wrath -> effectN( 1 ).percent() );

  const spell_data_t* barbed_shot = find_spell( 246152 );
  for ( size_t i = 0; i < buffs.barbed_shot.size(); i++ )
  {
    buffs.barbed_shot[ i ] =
      make_buff( this, "barbed_shot_" + util::to_string( i + 1 ), barbed_shot )
        -> set_default_value( barbed_shot -> effectN( 1 ).resource( RESOURCE_FOCUS ) +
                              talents.scent_of_blood -> effectN( 1 ).base_value() )
        -> set_tick_callback( [ this ]( buff_t* b, int, const timespan_t& ) {
                          resource_gain( RESOURCE_FOCUS, b -> default_value, gains.barbed_shot );
                        } );
  }

  buffs.dire_beast =
    make_buff( this, "dire_beast", find_spell( 120679 ) -> effectN( 2 ).trigger() )
      -> set_default_value( find_spell( 120679 ) -> effectN( 2 ).trigger() -> effectN( 1 ).percent() )
      -> add_invalidate( CACHE_HASTE );

  buffs.thrill_of_the_hunt =
    make_buff( this, "thrill_of_the_hunt", talents.thrill_of_the_hunt -> effectN( 1 ).trigger() )
      -> set_default_value( talents.thrill_of_the_hunt -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
      -> set_trigger_spell( talents.thrill_of_the_hunt );

  buffs.spitting_cobra =
    make_buff( this, "spitting_cobra", talents.spitting_cobra )
      -> set_default_value( find_spell( 194407 ) -> effectN( 2 ).resource( RESOURCE_FOCUS ) )
      -> set_tick_callback( [ this ]( buff_t *buff, int, const timespan_t& ){
                        resource_gain( RESOURCE_FOCUS, buff -> default_value, gains.spitting_cobra );
                      } );

  // Marksmanship

  buffs.precise_shots =
    make_buff( this, "precise_shots", find_spell( 260242 ) )
      -> set_default_value( find_spell( 260242 ) -> effectN( 1 ).percent() );

  buffs.trick_shots =
    make_buff( this, "trick_shots", find_spell( 257622 ) )
      -> set_chance( specs.trick_shots -> ok() );

  buffs.trueshot =
    make_buff( this, "trueshot", specs.trueshot )
      -> set_cooldown( 0_ms )
      -> set_activated( true );
  buffs.trueshot -> set_default_value( specs.trueshot -> effectN( 4 ).percent() );
  buffs.trueshot -> set_stack_change_callback( [this]( buff_t*, int, int ) {
      cooldowns.aimed_shot -> adjust_recharge_multiplier();
      cooldowns.rapid_fire -> adjust_recharge_multiplier();
    } );

  buffs.lock_and_load =
    make_buff( this, "lock_and_load", talents.lock_and_load -> effectN( 1 ).trigger() )
      -> set_trigger_spell( talents.lock_and_load );

  buffs.master_marksman =
    make_buff( this, "master_marksman", talents.master_marksman -> effectN( 1 ).trigger() )
      -> set_default_value( talents.master_marksman -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
      -> set_trigger_spell( talents.master_marksman );

  buffs.steady_focus =
    make_buff( this, "steady_focus", find_spell( 193534 ) )
      -> set_default_value( find_spell( 193534 ) -> effectN( 2 ).percent() )
      -> set_trigger_spell( talents.steady_focus );

  buffs.double_tap =
    make_buff( this, "double_tap", talents.double_tap )
      -> set_cooldown( 0_ms )
      -> set_activated( true )
      -> set_default_value( talents.double_tap -> effectN( 1 ).percent() );

  // Survival

  buffs.coordinated_assault =
    make_buff( this, "coordinated_assault", specs.coordinated_assault )
      -> set_cooldown( 0_ms )
      -> set_activated( true )
      -> set_default_value( specs.coordinated_assault -> effectN( 1 ).percent() );

  buffs.vipers_venom =
    make_buff( this, "vipers_venom", talents.vipers_venom -> effectN( 1 ).trigger() )
      -> set_default_value( talents.vipers_venom -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
      -> set_trigger_spell( talents.vipers_venom );

  buffs.tip_of_the_spear =
    make_buff( this, "tip_of_the_spear", find_spell( 260286 ) )
      -> set_default_value( find_spell( 260286 ) -> effectN( 1 ).percent() )
      -> set_chance( talents.tip_of_the_spear -> ok() );

  buffs.mongoose_fury =
    make_buff( this, "mongoose_fury", find_spell( 259388 ) )
      -> set_default_value( find_spell( 259388 ) -> effectN( 1 ).percent() )
      -> set_refresh_behavior( buff_refresh_behavior::DISABLED );

  buffs.predator =
    make_buff( this, "predator", find_spell( 260249 ) )
      -> set_default_value( find_spell( 260249 ) -> effectN( 1 ).percent() )
      -> add_invalidate( CACHE_ATTACK_SPEED );

  buffs.terms_of_engagement =
    make_buff( this, "terms_of_engagement", find_spell( 265898 ) )
      -> set_default_value( find_spell( 265898 ) -> effectN( 1 ).base_value() / 5.0 )
      -> set_affects_regen( true );

  buffs.aspect_of_the_eagle =
    make_buff( this, "aspect_of_the_eagle", specs.aspect_of_the_eagle )
      -> set_cooldown( 0_ms );

  // Azerite

  buffs.blur_of_talons =
    make_buff<stat_buff_t>( this, "blur_of_talons", find_spell( 277969 ) )
      -> add_stat( STAT_AGILITY, azerite.blur_of_talons.value( 1 ) )
      -> add_stat( STAT_SPEED_RATING, azerite.blur_of_talons.value( 2 ) )
      -> set_trigger_spell( azerite.blur_of_talons );

  buffs.dance_of_death =
    make_buff<stat_buff_t>( this, "dance_of_death", find_spell( 274443 ) )
      -> add_stat( STAT_AGILITY, azerite.dance_of_death.value( 1 ) );

  buffs.haze_of_rage =
    make_buff<stat_buff_t>( this, "haze_of_rage", find_spell( 273264 ) )
      -> add_stat( STAT_AGILITY, azerite.haze_of_rage.value( 1 ) )
      -> set_trigger_spell( azerite.haze_of_rage );

  buffs.in_the_rhythm =
    make_buff<stat_buff_t>( this, "in_the_rhythm", find_spell( 272733 ) )
      -> add_stat( STAT_HASTE_RATING, azerite.in_the_rhythm.value( 1 ) )
      -> set_trigger_spell( azerite.in_the_rhythm );

  buffs.primal_instincts =
    make_buff<stat_buff_t>( this, "primal_instincts", find_spell( 279810 ) )
      -> add_stat( STAT_MASTERY_RATING, azerite.primal_instincts.value( 1 ) )
      -> set_trigger_spell( azerite.primal_instincts );

  buffs.primeval_intuition =
    make_buff<stat_buff_t>( this, "primeval_intuition", find_spell( 288573 ) )
      -> add_stat( STAT_CRIT_RATING, azerite.primeval_intuition.value( 1 ) )
      -> set_trigger_spell( azerite.primeval_intuition );

  buffs.unerring_vision_driver =
    make_buff( this, "unerring_vision_driver", find_spell( 274446 ) )
      -> set_quiet( true )
      -> set_tick_callback( [ this ]( buff_t*, int, const timespan_t& ) { buffs.unerring_vision -> trigger(); } )
      -> set_trigger_spell( azerite.unerring_vision );

  buffs.unerring_vision =
    make_buff<stat_buff_t>( this, "unerring_vision", find_spell( 274447 ) )
      -> add_stat( STAT_CRIT_RATING, azerite.unerring_vision.value( 1 ) );
}

// hunter_t::init_special_effects ===========================================

void hunter_t::init_special_effects()
{
  player_t::init_special_effects();
}

// hunter_t::init_gains =====================================================

void hunter_t::init_gains()
{
  player_t::init_gains();

  gains.barbed_shot          = get_gain( "barbed_shot" );
  gains.aspect_of_the_wild   = get_gain( "aspect_of_the_wild" );
  gains.spitting_cobra       = get_gain( "spitting_cobra" );
  gains.hunters_mark         = get_gain( "hunters_mark" );
  gains.terms_of_engagement  = get_gain( "terms_of_engagement" );
}

// hunter_t::init_position ==================================================

void hunter_t::init_position()
{
  player_t::init_position();

  if ( specialization() == HUNTER_SURVIVAL )
  {
    base.position = POSITION_BACK;
    position_str = util::position_type_string( base.position );
  }
  else
  {
    if ( base.position == POSITION_FRONT )
    {
      base.position = POSITION_RANGED_FRONT;
      position_str = util::position_type_string( base.position );
    }
    else if ( initial.position == POSITION_BACK )
    {
      base.position = POSITION_RANGED_BACK;
      position_str = util::position_type_string( base.position );
    }
  }

  sim -> print_debug( "{}: Position adjusted to {}", name(), position_str );
}

// hunter_t::init_procs =====================================================

void hunter_t::init_procs()
{
  player_t::init_procs();

  procs.wild_call    = get_proc( "Wild Call" );
  procs.lethal_shots = get_proc( "Lethal Shots" );
}

// hunter_t::init_rng =======================================================

void hunter_t::init_rng()
{
  player_t::init_rng();
}

// hunter_t::init_scaling ===================================================

void hunter_t::init_scaling()
{
  player_t::init_scaling();

  scaling -> disable( STAT_STRENGTH );
}

// hunter_t::init_assessors =================================================

void hunter_t::init_assessors()
{
  player_t::init_assessors();

  if ( talents.terms_of_engagement -> ok() )
  {
    assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, [this]( dmg_e, action_state_t* s ) {
      if ( s -> result_amount > 0 )
        get_target_data( s -> target ) -> damaged = true;
      return assessor::CONTINUE;
    } );
  }
}

// hunter_t::default_potion =================================================

std::string hunter_t::default_potion() const
{
  return ( true_level >  110 ) ? "battle_potion_of_agility" :
         ( true_level >= 100 ) ? "prolonged_power" :
         ( true_level >= 90  ) ? "draenic_agility" :
         ( true_level >= 85  ) ? "virmens_bite":
         "disabled";
}

// hunter_t::default_flask ==================================================

std::string hunter_t::default_flask() const
{
  return ( true_level >  110 ) ? "currents" :
         ( true_level >  100 ) ? "seventh_demon" :
         ( true_level >= 90  ) ? "greater_draenic_agility_flask" :
         ( true_level >= 85  ) ? "spring_blossoms" :
         ( true_level >= 80  ) ? "winds" :
         "disabled";
}

// hunter_t::default_food ===================================================

std::string hunter_t::default_food() const
{
  return ( true_level >  110 ) ? "bountiful_captains_feast" :
         ( true_level >  100 ) ? "lavish_suramar_feast" :
         ( true_level >= 90  ) ? "sea_mist_rice_noodles" :
         ( true_level >= 80  ) ? "seafood_magnifique_feast" :
         "disabled";
}

// hunter_t::default_rune ===================================================

std::string hunter_t::default_rune() const
{
  return ( true_level >= 120 ) ? "battle_scarred" :
         ( true_level >= 110 ) ? "defiled" :
         ( true_level >= 100 ) ? "hyper" :
         "disabled";
}

// hunter_t::init_actions ===================================================

void hunter_t::init_action_list()
{
  if ( main_hand_weapon.group() == WEAPON_RANGED )
  {
    const weapon_e type = main_hand_weapon.type;
    if ( type != WEAPON_BOW && type != WEAPON_CROSSBOW && type != WEAPON_GUN )
    {
      sim -> error( "Player {} does not have a proper weapon type at the Main Hand slot: {}.",
                    name(), util::weapon_subclass_string( items[ main_hand_weapon.slot ].parsed.data.item_subclass ) );
      if ( specialization() != HUNTER_SURVIVAL )
        sim -> cancel();
    }
  }

  if ( specialization() == HUNTER_SURVIVAL && main_hand_weapon.group() != WEAPON_2H )
  {
    sim -> error( "Player {} does not have a proper weapon at the Main Hand slot: {}.",
                  name(), util::weapon_type_string( main_hand_weapon.type ) );
  }

  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    action_priority_list_t* precombat = get_action_priority_list( "precombat" );

    // Flask, Rune, Food
    precombat -> add_action( "flask" );
    precombat -> add_action( "augmentation" );
    precombat -> add_action( "food" );

    if ( specialization() != HUNTER_MARKSMANSHIP )
      precombat -> add_action( "summon_pet" );

    precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

    // Pre-pot
    precombat -> add_action( "potion" );

    switch ( specialization() )
    {
    case HUNTER_SURVIVAL:
      apl_surv();
      break;
    case HUNTER_BEAST_MASTERY:
      apl_bm();
      break;
    case HUNTER_MARKSMANSHIP:
      apl_mm();
      break;
    default:
      apl_default(); // DEFAULT
      break;
    }

    // Default
    use_default_action_list = true;
    player_t::init_action_list();
  }
}

// Item Actions =======================================================================

std::string hunter_t::special_use_item_action( const std::string& item_name, const std::string& condition ) const
{
  auto item = range::find_if( items, [ &item_name ]( const item_t& item ) {
    return item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) && item.name_str == item_name;
  });
  if ( item == items.end() )
    return std::string();

  std::string action = "use_item,name=" + item -> name_str;
  if ( !condition.empty() )
    action += "," + condition;
  return action;
}

// Beastmastery Action List =============================================================

void hunter_t::apl_bm()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );
  action_priority_list_t* precombat    = get_action_priority_list( "precombat" );
  action_priority_list_t* cds          = get_action_priority_list( "cds" );
  action_priority_list_t* st           = get_action_priority_list( "st" );
  action_priority_list_t* cleave       = get_action_priority_list( "cleave" );

  // Precombat actions
  precombat -> add_action( this, "Aspect of the Wild", "precast_time=1.1,if=!azerite.primal_instincts.enabled",
          "Adjusts the duration and cooldown of Aspect of the Wild and Primal Instincts by the duration of an unhasted GCD when they're used precombat. As AotW has a 1.3s GCD and affects itself this is 1.1s." );
  precombat -> add_action( this, "Bestial Wrath", "precast_time=1.5,if=azerite.primal_instincts.enabled",
          "Adjusts the duration and cooldown of Bestial Wrath and Haze of Rage by the duration of an unhasted GCD when they're used precombat." );

  // Generic APL
  default_list -> add_action( "auto_shot" );
  default_list -> add_action( "use_items" );
  default_list -> add_action( "call_action_list,name=cds" );
  default_list -> add_action( "call_action_list,name=st,if=active_enemies<2" );
  default_list -> add_action( "call_action_list,name=cleave,if=active_enemies>1" );
  // Arcane torrent if nothing else is available
  default_list -> add_action( this, "Arcane Torrent" );

  // Racials
  for ( std::string racial : { "ancestral_call", "fireblood" } )
    cds -> add_action( racial + ",if=cooldown.bestial_wrath.remains>30");

  cds -> add_action("berserking,if=buff.aspect_of_the_wild.up&(target.time_to_die>cooldown.berserking.duration+duration|(target.health.pct<35|!talent.killer_instinct.enabled))|target.time_to_die<13");
  cds -> add_action("blood_fury,if=buff.aspect_of_the_wild.up&(target.time_to_die>cooldown.blood_fury.duration+duration|(target.health.pct<35|!talent.killer_instinct.enabled))|target.time_to_die<16");
  cds -> add_action("lights_judgment,if=pet.cat.buff.frenzy.up&pet.cat.buff.frenzy.remains>gcd.max|!pet.cat.buff.frenzy.up");

  // In-combat potion
  cds -> add_action( "potion,if=buff.bestial_wrath.up&buff.aspect_of_the_wild.up&(target.health.pct<35|!talent.killer_instinct.enabled)|target.time_to_die<25" );

  st -> add_action( this, "Barbed Shot", "if=pet.cat.buff.frenzy.up&pet.cat.buff.frenzy.remains<=gcd.max|full_recharge_time<gcd.max&cooldown.bestial_wrath.remains|azerite.primal_instincts.enabled&cooldown.aspect_of_the_wild.remains<gcd" );
  st -> add_action( this, "Aspect of the Wild" );
  st -> add_talent( this, "A Murder of Crows" );
  st -> add_talent( this, "Stampede", "if=buff.aspect_of_the_wild.up&buff.bestial_wrath.up|target.time_to_die<15" );
  st -> add_action( this, "Bestial Wrath", "if=cooldown.aspect_of_the_wild.remains>20|target.time_to_die<15" );
  st -> add_action( this, "Kill Command" );
  st -> add_talent( this, "Chimaera Shot" );
  st -> add_talent( this, "Dire Beast" );
  st -> add_action( this, "Barbed Shot", "if=pet.cat.buff.frenzy.down&(charges_fractional>1.8|buff.bestial_wrath.up)|cooldown.aspect_of_the_wild.remains<pet.cat.buff.frenzy.duration-gcd&azerite.primal_instincts.enabled|azerite.dance_of_death.rank>1&buff.dance_of_death.down&crit_pct_current>40|target.time_to_die<9" );
  st -> add_talent( this, "Barrage" );
  st -> add_action( this, "Cobra Shot", "if=(focus-cost+focus.regen*(cooldown.kill_command.remains-1)>action.kill_command.cost|cooldown.kill_command.remains>1+gcd)&cooldown.kill_command.remains>1" );
  st -> add_talent( this, "Spitting Cobra" );
  st -> add_action( this, "Barbed Shot", "if=charges_fractional>1.4" );

  cleave -> add_action( this, "Barbed Shot", "if=pet.cat.buff.frenzy.up&pet.cat.buff.frenzy.remains<=gcd.max" );
  cleave -> add_action( this, "Multi-Shot", "if=gcd.max-pet.cat.buff.beast_cleave.remains>0.25" );
  cleave -> add_action( this, "Barbed Shot", "if=full_recharge_time<gcd.max&cooldown.bestial_wrath.remains" );
  cleave -> add_action( this, "Aspect of the Wild" );
  cleave -> add_talent( this, "Stampede", "if=buff.aspect_of_the_wild.up&buff.bestial_wrath.up|target.time_to_die<15" );
  cleave -> add_action( this, "Bestial Wrath", "if=cooldown.aspect_of_the_wild.remains>20|target.time_to_die<15" );
  cleave -> add_talent( this, "Chimaera Shot" );
  cleave -> add_talent( this, "A Murder of Crows" );
  cleave -> add_talent( this, "Barrage" );
  cleave -> add_action( this, "Kill Command", "if=active_enemies<4|!azerite.rapid_reload.enabled" );
  cleave -> add_talent( this, "Dire Beast" );
  cleave -> add_action( this, "Barbed Shot", "if=pet.cat.buff.frenzy.down&(charges_fractional>1.8|buff.bestial_wrath.up)|cooldown.aspect_of_the_wild.remains<pet.cat.buff.frenzy.duration-gcd&azerite.primal_instincts.enabled|target.time_to_die<9" );
  cleave -> add_action( this, "Multi-Shot", "if=azerite.rapid_reload.enabled&active_enemies>2");
  cleave -> add_action( this, "Cobra Shot", "if=cooldown.kill_command.remains>focus.time_to_max&(active_enemies<3|!azerite.rapid_reload.enabled)" );
  cleave -> add_talent( this, "Spitting Cobra" );
}

// Marksman Action List ======================================================================

void hunter_t::apl_mm()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );
  action_priority_list_t* precombat    = get_action_priority_list( "precombat" );
  action_priority_list_t* cds          = get_action_priority_list( "cds" );
  action_priority_list_t* st           = get_action_priority_list( "st" );
  action_priority_list_t* trickshots   = get_action_priority_list( "trickshots" );

  // Precombat actions
  precombat -> add_talent( this, "Hunter's Mark" );
  precombat -> add_talent( this, "Double Tap", "precast_time=10",
        "Precast this as early as possible to potentially gain another cast during the fight." );
  precombat -> add_action( this, "Trueshot", "precast_time=1.5,if=active_enemies>2" );
  precombat -> add_action( this, "Aimed Shot", "if=active_enemies<3" );

  // Generic APL
  default_list -> add_action( "auto_shot" );
  default_list -> add_action( "use_items,if=buff.trueshot.up|!talent.calling_the_shots.enabled|target.time_to_die<20",
	  "Try to line up activated trinkets with Trueshot" );
  default_list -> add_action( "call_action_list,name=cds" );
  default_list -> add_action( "call_action_list,name=st,if=active_enemies<3" );
  default_list -> add_action( "call_action_list,name=trickshots,if=active_enemies>2" );

  cds -> add_talent( this, "Hunter's Mark", "if=debuff.hunters_mark.down&!buff.trueshot.up" );
  cds -> add_talent( this, "Double Tap", "if=cooldown.rapid_fire.remains<gcd|cooldown.rapid_fire.remains<cooldown.aimed_shot.remains|target.time_to_die<20" );

  // Racials
  cds -> add_action( "berserking,if=buff.trueshot.up&(target.time_to_die>cooldown.berserking.duration+duration|(target.health.pct<20|!talent.careful_aim.enabled))|target.time_to_die<13" );
  cds -> add_action( "blood_fury,if=buff.trueshot.up&(target.time_to_die>cooldown.blood_fury.duration+duration|(target.health.pct<20|!talent.careful_aim.enabled))|target.time_to_die<16" );
  cds -> add_action( "ancestral_call,if=buff.trueshot.up&(target.time_to_die>cooldown.ancestral_call.duration+duration|(target.health.pct<20|!talent.careful_aim.enabled))|target.time_to_die<16" );
  cds -> add_action( "fireblood,if=buff.trueshot.up&(target.time_to_die>cooldown.fireblood.duration+duration|(target.health.pct<20|!talent.careful_aim.enabled))|target.time_to_die<9" );
  cds -> add_action( "lights_judgment" );

  // In-combat potion
  cds -> add_action( "potion,if=buff.trueshot.react&buff.bloodlust.react|buff.trueshot.up&ca_execute|target.time_to_die<25" );

  cds -> add_action( this, "Trueshot", "if=focus>60&(buff.precise_shots.down&cooldown.rapid_fire.remains&target.time_to_die>cooldown.trueshot.duration_guess+duration|target.health.pct<20|!talent.careful_aim.enabled)|target.time_to_die<15" );

  st -> add_talent( this, "Explosive Shot" );
  st -> add_talent( this, "Barrage", "if=active_enemies>1" );
  st -> add_talent( this, "A Murder of Crows" );
  st -> add_talent( this, "Serpent Sting", "if=refreshable&!action.serpent_sting.in_flight" );
  st -> add_action( this, "Rapid Fire", "if=buff.trueshot.down|focus<70");
  st -> add_action( this, "Arcane Shot", "if=buff.trueshot.up&buff.master_marksman.up");
  st -> add_action( this, "Aimed Shot", "if=buff.trueshot.up|(buff.double_tap.down|ca_execute)&buff.precise_shots.down|full_recharge_time<cast_time" );
  st -> add_talent( this, "Piercing Shot" );
  st -> add_action( this, "Arcane Shot", "if=buff.trueshot.down&(buff.precise_shots.up&(focus>41|buff.master_marksman.up)|(focus>50&azerite.focused_fire.enabled|focus>75)&(cooldown.trueshot.remains>5|focus>80)|target.time_to_die<5)" );
  st -> add_action( this, "Steady Shot" );

  trickshots -> add_talent( this, "Barrage" );
  trickshots -> add_talent( this, "Explosive Shot" );
  trickshots -> add_action( this, "Aimed Shot", "if=buff.trick_shots.up&ca_execute&buff.double_tap.up");
  trickshots -> add_action( this, "Rapid Fire", "if=buff.trick_shots.up&(azerite.focused_fire.enabled|azerite.in_the_rhythm.rank>1|azerite.surging_shots.enabled|talent.streamline.enabled)" );
  trickshots -> add_action( this, "Aimed Shot", "if=buff.trick_shots.up&(buff.precise_shots.down|cooldown.aimed_shot.full_recharge_time<action.aimed_shot.cast_time|buff.trueshot.up)" );
  trickshots -> add_action( this, "Rapid Fire", "if=buff.trick_shots.up" );
  trickshots -> add_action( this, "Multi-Shot", "if=buff.trick_shots.down|buff.precise_shots.up&!buff.trueshot.up|focus>70" );
  trickshots -> add_talent( this, "Piercing Shot" );
  trickshots -> add_talent( this, "A Murder of Crows" );
  trickshots -> add_talent( this, "Serpent Sting", "if=refreshable&!action.serpent_sting.in_flight" );
  trickshots -> add_action( this, "Steady Shot" );
}

// Survival Action List ===================================================================

void hunter_t::apl_surv()
{
  action_priority_list_t* default_list   = get_action_priority_list( "default" );
  action_priority_list_t* precombat      = get_action_priority_list( "precombat" );
  action_priority_list_t* cds            = get_action_priority_list( "cds" );
  action_priority_list_t* st             = get_action_priority_list( "st" );
  action_priority_list_t* wfi_st         = get_action_priority_list( "wfi_st" );
  action_priority_list_t* mb_ap_wfi_st   = get_action_priority_list( "mb_ap_wfi_st" );
  action_priority_list_t* cleave         = get_action_priority_list( "cleave" );

  // Precombat actions
  precombat -> add_talent( this, "Steel Trap" );
  precombat -> add_action( this, "Harpoon" );

  // Generic APL
  default_list -> add_action( "auto_attack" );
  default_list -> add_action( "use_items" );
  default_list -> add_action( "call_action_list,name=cds" );
  default_list -> add_action( "call_action_list,name=mb_ap_wfi_st,if=active_enemies<3&talent.wildfire_infusion.enabled&talent.alpha_predator.enabled&talent.mongoose_bite.enabled" );
  default_list -> add_action( "call_action_list,name=wfi_st,if=active_enemies<3&talent.wildfire_infusion.enabled");
  default_list -> add_action( "call_action_list,name=st,if=active_enemies<2|azerite.blur_of_talons.enabled&talent.birds_of_prey.enabled&buff.coordinated_assault.up" );
  default_list -> add_action( "call_action_list,name=cleave,if=active_enemies>1" );
  // Arcane torrent if nothing else is available
  default_list -> add_action( "arcane_torrent" );

  // Racials
  for ( std::string racial : { "blood_fury", "ancestral_call", "fireblood" } )
    cds -> add_action( racial + ",if=cooldown.coordinated_assault.remains>30" );
  cds -> add_action( "lights_judgment" );
  cds -> add_action( "berserking,if=cooldown.coordinated_assault.remains>60|time_to_die<13");

  // In-combat potion
  cds -> add_action( "potion,if=buff.coordinated_assault.up&(buff.berserking.up|buff.blood_fury.up|!race.troll&!race.orc)|time_to_die<26" );

  cds -> add_action( this, "Aspect of the Eagle", "if=target.distance>=6" );

  wfi_st -> add_talent( this, "A Murder of Crows" );
  wfi_st -> add_action( this, "Coordinated Assault" );
  wfi_st -> add_talent( this, "Mongoose Bite", "if=azerite.wilderness_survival.enabled&next_wi_bomb.volatile&dot.serpent_sting.remains>2.1*gcd&dot.serpent_sting.remains<3.5*gcd&cooldown.wildfire_bomb.remains>2.5*gcd",
                        "To simulate usage for Mongoose Bite or Raptor Strike during Aspect of the Eagle, copy each occurrence of the action and append _eagle to the action name." );
  wfi_st -> add_action( this, "Wildfire Bomb", "if=full_recharge_time<gcd|(focus+cast_regen<focus.max)&(next_wi_bomb.volatile&dot.serpent_sting.ticking&dot.serpent_sting.refreshable|next_wi_bomb.pheromone&!buff.mongoose_fury.up&focus+cast_regen<focus.max-action.kill_command.cast_regen*3)" );
  wfi_st -> add_action( this, "Kill Command", "if=focus+cast_regen<focus.max&buff.tip_of_the_spear.stack<3&(!talent.alpha_predator.enabled|buff.mongoose_fury.stack<5|focus<action.mongoose_bite.cost)" );
  wfi_st -> add_action( this, "Raptor Strike", "if=dot.internal_bleeding.stack<3&dot.shrapnel_bomb.ticking&!talent.mongoose_bite.enabled" );
  wfi_st -> add_action( this, "Wildfire Bomb", "if=next_wi_bomb.shrapnel&buff.mongoose_fury.down&(cooldown.kill_command.remains>gcd|focus>60)&!dot.serpent_sting.refreshable" );
  wfi_st -> add_talent( this, "Steel Trap" );
  wfi_st -> add_talent( this, "Flanking Strike", "if=focus+cast_regen<focus.max" );
  wfi_st -> add_action( this, "Serpent Sting", "if=buff.vipers_venom.react|refreshable&(!talent.mongoose_bite.enabled|!talent.vipers_venom.enabled|next_wi_bomb.volatile&!dot.shrapnel_bomb.ticking|azerite.latent_poison.enabled|azerite.venomous_fangs.enabled|buff.mongoose_fury.stack=5)" );
  wfi_st -> add_action( this, "Harpoon", "if=talent.terms_of_engagement.enabled" );
  wfi_st -> add_talent( this, "Mongoose Bite", "if=buff.mongoose_fury.up|focus>60|dot.shrapnel_bomb.ticking" );
  wfi_st -> add_action( this, "Raptor Strike" );
  wfi_st -> add_action( this, "Serpent Sting", "if=refreshable" );
  wfi_st -> add_action( this, "Wildfire Bomb", "if=next_wi_bomb.volatile&dot.serpent_sting.ticking|next_wi_bomb.pheromone|next_wi_bomb.shrapnel&focus>50" );

  mb_ap_wfi_st -> add_action( this, "Serpent Sting", "if=!dot.serpent_sting.ticking" );
  mb_ap_wfi_st -> add_action( this, "Wildfire Bomb", "if=full_recharge_time<gcd|(focus+cast_regen<focus.max)&(next_wi_bomb.volatile&dot.serpent_sting.ticking&dot.serpent_sting.refreshable|next_wi_bomb.pheromone&!buff.mongoose_fury.up&focus+cast_regen<focus.max-action.kill_command.cast_regen*3)" );
  mb_ap_wfi_st -> add_action( this, "Coordinated Assault");
  mb_ap_wfi_st -> add_talent( this, "A Murder of Crows");
  mb_ap_wfi_st -> add_talent( this, "Steel Trap" );
  mb_ap_wfi_st -> add_talent( this, "Mongoose Bite", "if=buff.mongoose_fury.remains&next_wi_bomb.pheromone",
                        "To simulate usage for Mongoose Bite or Raptor Strike during Aspect of the Eagle, copy each occurrence of the action and append _eagle to the action name." );
  mb_ap_wfi_st -> add_action( this, "Kill Command", "if=focus+cast_regen<focus.max&(buff.mongoose_fury.stack<5|focus<action.mongoose_bite.cost)" );
  mb_ap_wfi_st -> add_action( this, "Wildfire Bomb", "if=next_wi_bomb.shrapnel&focus>60&dot.serpent_sting.remains>3*gcd" );
  mb_ap_wfi_st -> add_action( this, "Serpent Sting", "if=refreshable&(next_wi_bomb.volatile&!dot.shrapnel_bomb.ticking|azerite.latent_poison.enabled|azerite.venomous_fangs.enabled)" );
  mb_ap_wfi_st -> add_talent( this, "Mongoose Bite", "if=buff.mongoose_fury.up|focus>60|dot.shrapnel_bomb.ticking" );
  mb_ap_wfi_st -> add_action( this, "Serpent Sting", "if=refreshable" );
  mb_ap_wfi_st -> add_action( this, "Wildfire Bomb", "if=next_wi_bomb.volatile&dot.serpent_sting.ticking|next_wi_bomb.pheromone|next_wi_bomb.shrapnel&focus>50" );

  st -> add_talent( this, "A Murder of Crows" );
  st -> add_talent( this, "Mongoose Bite", "if=talent.birds_of_prey.enabled&buff.coordinated_assault.up&(buff.coordinated_assault.remains<gcd|buff.blur_of_talons.up&buff.blur_of_talons.remains<gcd)",
    "To simulate usage for Mongoose Bite or Raptor Strike during Aspect of the Eagle, copy each occurrence of the action and append _eagle to the action name." );
  st -> add_action( this, "Raptor Strike", "if=talent.birds_of_prey.enabled&buff.coordinated_assault.up&(buff.coordinated_assault.remains<gcd|buff.blur_of_talons.up&buff.blur_of_talons.remains<gcd)" );
  st -> add_action( this, "Serpent Sting", "if=buff.vipers_venom.react&buff.vipers_venom.remains<gcd" );
  st -> add_action( this, "Kill Command", "if=focus+cast_regen<focus.max&(!talent.alpha_predator.enabled|talent.alpha_predator.enabled&full_recharge_time<1.5*gcd&focus+cast_regen<focus.max-20)" );
  st -> add_action( this, "Wildfire Bomb", "if=focus+cast_regen<focus.max&(full_recharge_time<gcd|!dot.wildfire_bomb.ticking&(buff.mongoose_fury.down|full_recharge_time<4.5*gcd))" );
  st -> add_action( this, "Serpent Sting", "if=buff.vipers_venom.react&dot.serpent_sting.remains<4*gcd|!talent.vipers_venom.enabled&!dot.serpent_sting.ticking&!buff.coordinated_assault.up" );
  st -> add_action( this, "Serpent Sting", "if=refreshable&(azerite.latent_poison.rank>2|azerite.latent_poison.enabled&azerite.venomous_fangs.enabled|(azerite.latent_poison.enabled|azerite.venomous_fangs.enabled)&(!azerite.blur_of_talons.enabled|!talent.birds_of_prey.enabled|!buff.coordinated_assault.up))" );
  st -> add_talent( this, "Steel Trap" );
  st -> add_action( this, "Harpoon", "if=talent.terms_of_engagement.enabled" );
  st -> add_action( this, "Coordinated Assault" );
  st -> add_talent( this, "Chakrams" );
  st -> add_talent( this, "Flanking Strike", "if=focus+cast_regen<focus.max" );
  st -> add_action( this, "Kill Command", "if=focus+cast_regen<focus.max&(buff.mongoose_fury.stack<4|focus<action.mongoose_bite.cost)" );
  st -> add_talent( this, "Mongoose Bite", "if=buff.mongoose_fury.up|(focus+cast_regen>focus.max-10|talent.vipers_venom.enabled&focus+cast_regen>focus.max-20)|buff.coordinated_assault.up" );
  st -> add_action( this, "Raptor Strike" );
  st -> add_action( this, "Serpent Sting", "if=dot.serpent_sting.refreshable&!buff.coordinated_assault.up" );
  st -> add_action( this, "Wildfire Bomb", "if=dot.wildfire_bomb.refreshable" );

  cleave -> add_action( "variable,name=carve_cdr,op=setif,value=active_enemies,value_else=5,condition=active_enemies<5" );
  cleave -> add_talent( this, "A Murder of Crows" );
  cleave -> add_action( this, "Coordinated Assault" );
  cleave -> add_action( this, "Carve", "if=dot.shrapnel_bomb.ticking" );
  cleave -> add_action( this, "Wildfire Bomb", "if=!talent.guerrilla_tactics.enabled|full_recharge_time<gcd" );
  cleave -> add_talent( this, "Mongoose Bite", "target_if=max:debuff.latent_poison.stack,if=debuff.latent_poison.stack=10" );
  cleave -> add_talent( this, "Chakrams" );
  cleave -> add_action( this, "Kill Command", "target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max" );
  cleave -> add_talent( this, "Butchery", "if=full_recharge_time<gcd|!talent.wildfire_infusion.enabled|dot.shrapnel_bomb.ticking&dot.internal_bleeding.stack<3" );
  cleave -> add_action( this, "Carve", "if=talent.guerrilla_tactics.enabled" );
  cleave -> add_talent( this, "Flanking Strike", "if=focus+cast_regen<focus.max" );
  cleave -> add_action( this, "Wildfire Bomb", "if=dot.wildfire_bomb.refreshable|talent.wildfire_infusion.enabled" );
  cleave -> add_action( this, "Serpent Sting", "target_if=min:remains,if=buff.vipers_venom.react" );
  cleave -> add_action( this, "Carve", "if=cooldown.wildfire_bomb.remains>variable.carve_cdr%2" );
  cleave -> add_talent( this, "Steel Trap" );
  cleave -> add_action( this, "Harpoon", "if=talent.terms_of_engagement.enabled" );
  cleave -> add_action( this, "Serpent Sting", "target_if=min:remains,if=refreshable&buff.tip_of_the_spear.stack<3" );
  cleave -> add_talent( this, "Mongoose Bite", "target_if=max:debuff.latent_poison.stack",
                        "To simulate usage for Mongoose Bite or Raptor Strike during Aspect of the Eagle, copy each occurrence of the action and append _eagle to the action name." );
  cleave -> add_action( this, "Raptor Strike", "target_if=max:debuff.latent_poison.stack" );
}

// NO Spec Combat Action Priority List ======================================

void hunter_t::apl_default()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );

  default_list -> add_action( this, "Arcane Shot" );
}

// hunter_t::reset ==========================================================

void hunter_t::reset()
{
  player_t::reset();

  // Active
  pets.main = nullptr;
  current_hunters_mark_target = nullptr;
  next_wi_bomb = WILDFIRE_INFUSION_SHRAPNEL;
}

// hunter_t::merge ==========================================================

void hunter_t::merge( player_t& other )
{
  player_t::merge( other );

  cd_waste.merge( static_cast<hunter_t&>( other ).cd_waste );
}

// hunter_t::combat_begin ==================================================

void hunter_t::combat_begin()
{
  if ( talents.bloodseeker -> ok() && ( talents.wildfire_infusion -> ok() || sim -> player_no_pet_list.size() > 1 ) )
  {
    make_repeating_event( *sim, 1_s,
      [ this ]() {
        trigger_bloodseeker_update( this );
      } );
  }

  player_t::combat_begin();
}

// hunter_t::datacollection_begin ===========================================

void hunter_t::datacollection_begin()
{
  if ( active_during_iteration )
    cd_waste.datacollection_begin();

  player_t::datacollection_begin();
}

// hunter_t::datacollection_end =============================================

void hunter_t::datacollection_end()
{
  if ( requires_data_collection() )
    cd_waste.datacollection_end();

  player_t::datacollection_end();
}

// hunter_t::composite_attack_power_multiplier ==============================

double hunter_t::composite_attack_power_multiplier() const
{
  return player_t::composite_attack_power_multiplier();
}

// hunter_t::composite_melee_crit_chance ===========================================

double hunter_t::composite_melee_crit_chance() const
{
  double crit = player_t::composite_melee_crit_chance();

  crit += specs.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// hunter_t::composite_spell_crit_chance ===========================================

double hunter_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  crit += specs.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// hunter_t::composite_melee_haste ===========================================

double hunter_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.dire_beast -> check() )
    h *= 1.0 / ( 1.0 + buffs.dire_beast -> check_value() );

  return h;
}

// hunter_t::composite_melee_speed ==========================================

double hunter_t::composite_melee_speed() const
{
  double s = player_t::composite_melee_speed();

  if ( buffs.predator -> check() )
    s *= 1.0 / ( 1.0 + buffs.predator -> check_stack_value() );

  return s;
}

// hunter_t::composite_spell_haste ===========================================

double hunter_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.dire_beast -> check() )
    h *= 1.0 / ( 1.0 + buffs.dire_beast -> check_value() );

  return h;
}

// hunter_t::composite_player_critical_damage_multiplier ====================

double hunter_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double cdm = player_t::composite_player_critical_damage_multiplier( s );

  return cdm;
}

// hunter_t::composite_player_multiplier ====================================

double hunter_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  return m;
}

// composite_player_target_multiplier ====================================

double hunter_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double d = player_t::composite_player_target_multiplier( target, school );

  if ( auto td = find_target_data( target ) )
  {
    d *= 1.0 + td -> debuffs.hunters_mark -> check_value();
  }

  return d;
}

// hunter_t::composite_player_pet_damage_multiplier ======================

double hunter_t::composite_player_pet_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s );

  if ( mastery.master_of_beasts -> ok() )
    m *= 1.0 + cache.mastery_value();

  m *= 1.0 + specs.beast_mastery_hunter -> effectN( 3 ).percent();
  m *= 1.0 + specs.survival_hunter -> effectN( 3 ).percent();
  m *= 1.0 + specs.marksmanship_hunter -> effectN( 3 ).percent();

  m *= 1.0 + talents.animal_companion -> effectN( 2 ).percent();

  m *= 1.0 + buffs.coordinated_assault -> check_value();

  return m;
}

// hunter_t::invalidate_cache ==============================================

void hunter_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
  case CACHE_MASTERY:
    if ( mastery.sniper_training -> ok() )
    {
      if ( sim -> distance_targeting_enabled && mastery.sniper_training -> ok() )
      {
        // Marksman is a unique butterfly, since mastery changes the max range of abilities. We need to regenerate every target cache.
        for ( size_t i = 0, end = action_list.size(); i < end; i++ )
        {
          action_list[i] -> target_cache.is_valid = false;
        }
      }
    }
    break;
  default: break;
  }
}

// hunter_t::regen =========================================================

void hunter_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( resources.is_infinite( RESOURCE_FOCUS ) )
    return;

  if ( buffs.terms_of_engagement -> check() )
      resource_gain( RESOURCE_FOCUS, buffs.terms_of_engagement -> check_value() * periodicity.total_seconds(), gains.terms_of_engagement );
}

// hunter_t::matching_gear_multiplier =======================================

double hunter_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_AGILITY )
    return 0.05;

  return 0.0;
}

// hunter_t::create_options =================================================

void hunter_t::create_options()
{
  player_t::create_options();

  add_option( opt_string( "summon_pet", options.summon_pet_str ) );
  add_option( opt_timespan( "hunter.pet_attack_speed", options.pet_attack_speed,
                            0.5_s, 4_s ) );
  add_option( opt_timespan( "hunter.pet_basic_attack_delay", options.pet_basic_attack_delay,
                            0_ms, 0.6_s ) );
  add_option( opt_obsoleted( "hunter_fixed_time" ) );
}

// hunter_t::create_profile =================================================

std::string hunter_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  const options_t defaults{};

  if ( options.summon_pet_str != defaults.summon_pet_str )
    profile_str += "summon_pet=" + options.summon_pet_str + "\n";
  if ( options.pet_attack_speed != defaults.pet_attack_speed )
    profile_str += "hunter.pet_attack_speed=" + util::to_string( options.pet_attack_speed.total_seconds() ) + "\n";
  if ( options.pet_basic_attack_delay != defaults.pet_basic_attack_delay )
    profile_str += "hunter.pet_basic_attack_delay=" + util::to_string( options.pet_basic_attack_delay.total_seconds() ) + "\n";

  return profile_str;
}

// hunter_t::copy_from ======================================================

void hunter_t::copy_from( player_t* source )
{
  player_t::copy_from( source );
  options = debug_cast<hunter_t*>( source ) -> options;
}

// hunter_::convert_hybrid_stat ==============================================

stat_e hunter_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  case STAT_STR_AGI_INT:
  case STAT_AGI_INT:
  case STAT_STR_AGI:
    return STAT_AGILITY;
    // This is a guess at how STR/INT gear will work for Rogues, TODO: confirm
    // This should probably never come up since rogues can't equip plate, but....
  case STAT_STR_INT:
    return STAT_NONE;
  case STAT_SPIRIT:
    return STAT_NONE;
  case STAT_BONUS_ARMOR:
    return STAT_NONE;
  default: return s;
  }
}

// hunter_t::moving() =======================================================

/* Override moving() so that it doesn't suppress auto_shot and only interrupts the few shots that cannot be used while moving.
*/
void hunter_t::moving()
{
  if ( ( executing && ! executing -> usable_moving() ) || ( channeling && ! channeling -> usable_moving() ) )
    player_t::interrupt();
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class hunter_report_t: public player_report_extension_t
{
public:
  hunter_report_t( hunter_t& player ):
    p( player )
  {
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n";

    cdwaste::print_html_report( p, p.cd_waste, os );

    os << "\t\t\t\t\t</div>\n";
  }
private:
  hunter_t& p;
};

// HUNTER MODULE INTERFACE ==================================================

struct hunter_module_t: public module_t
{
  hunter_module_t(): module_t( HUNTER ) {}

  player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new hunter_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new hunter_report_t( *p ) );
    return p;
  }

  bool valid() const override { return true; }

  void static_init() const override
  {
  }

  void init( player_t* ) const override
  {
  }

  void register_hotfixes() const override
  {
  }

  void combat_begin( sim_t* ) const override {}
  void combat_end( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::hunter()
{
  static hunter_module_t m;
  return &m;
}
