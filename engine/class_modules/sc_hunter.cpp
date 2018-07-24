//==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

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
  simple_sample_data_t iter;

  void update_ready( const action_t* action, timespan_t cd )
  {
    const cooldown_t* cooldown = action -> cooldown;
    sim_t* sim = action -> sim;
    if ( ( cd > timespan_t::zero() || ( cd <= timespan_t::zero() && cooldown -> duration > timespan_t::zero() ) ) &&
         cooldown -> current_charge == cooldown -> charges && cooldown -> last_charged > timespan_t::zero() &&
         cooldown -> last_charged < sim -> current_time() )
    {
      double time_ = ( sim -> current_time() - cooldown -> last_charged ).total_seconds();
      if ( sim -> debug )
      {
        sim -> out_debug.print( "{} {} cooldown waste tracking waste={:.3f} exec_time={:.3f}",
                                action -> player -> name(), action -> name(),
                                time_, action -> time_to_execute.total_seconds() );
      }
      time_ -= action -> time_to_execute.total_seconds();

      if ( time_ > 0 )
      {
        exec.add( time_ );
        iter.add( time_ );
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
      rec.second -> iter.reset();
  }

  void datacollection_end()
  {
    for ( auto& rec : data_ )
      rec.second -> cumulative.add( rec.second -> iter.sum() );
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
 *  Marksmanship
 *   - Double Tap AiS 'delay' (do we actually care? it seems to matter *only* for the
 *     initial 50% bonus damage in the sub ~20yds range)
 *  Beast Mastery
 *   - review Barbed Shot refresh mechanic
 *  Survival
 *   - review Bloodseeker dot refresh mechanics
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
  bool helbrined = false;

  struct debuffs_t
  {
    buff_t* mark_of_helbrine;
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
    pets::hunter_main_pet_t* main;
    pets::hunter_main_pet_base_t* animal_companion;
    pets::dire_critter_t* dire_beast;
    pet_t* spitting_cobra;
  } pets;

  struct legendary_t
  {
    // Beast Mastery
    spell_data_ptr_t bm_feet;
    spell_data_ptr_t bm_ring;
    spell_data_ptr_t bm_shoulders;
    spell_data_ptr_t bm_waist;
    spell_data_ptr_t bm_chest;

    // Marksmanship
    spell_data_ptr_t mm_feet;
    spell_data_ptr_t mm_gloves;
    spell_data_ptr_t mm_waist;
    spell_data_ptr_t mm_wrist;
    spell_data_ptr_t mm_cloak;

    // Survival
    spell_data_ptr_t sv_chest;
    spell_data_ptr_t sv_feet;
    spell_data_ptr_t sv_ring;
    spell_data_ptr_t sv_waist;
    spell_data_ptr_t wrist;

    // Generic
    spell_data_ptr_t sephuzs_secret;
    spell_data_ptr_t generic_damage;
    unsigned generic_damage_count = 0;
  } legendary;

  struct azerite_t
  {
    // Beast Mastery
    azerite_power_t dance_of_death;
    azerite_power_t feeding_frenzy;
    azerite_power_t haze_of_rage;
    azerite_power_t pack_alpha;
    azerite_power_t primal_instincts;
    azerite_power_t serrated_jaws;
    // Marksmanship
    azerite_power_t arcane_flurry;
    azerite_power_t focused_fire;
    azerite_power_t in_the_rhythm;
    azerite_power_t rapid_reload;
    azerite_power_t steady_aim;
    azerite_power_t unerring_vision;
    // Survival
    azerite_power_t blur_of_talons;
    azerite_power_t latent_poison;
    azerite_power_t up_close_and_personal;
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
    buff_t* lethal_shots;

    // Survival
    buff_t* coordinated_assault;
    buff_t* vipers_venom;
    buff_t* tip_of_the_spear;
    buff_t* mongoose_fury;
    buff_t* predator;
    buff_t* terms_of_engagement;
    buff_t* aspect_of_the_eagle;

    // sets
    buff_t* t20_4p_precision;
    buff_t* t20_2p_critical_aimed_damage;
    buff_t* pre_t20_2p_critical_aimed_damage;
    buff_t* t20_4p_bestial_rage;

    // legendaries
    buff_t* sentinels_sight;
    buff_t* butchers_bone_apron;
    buff_t* gyroscopic_stabilization;
    buff_t* the_mantle_of_command;
    buff_t* parsels_tongue;
    buff_t* sephuzs_secret;

    // azerite
    buff_t* arcane_flurry;
    buff_t* blur_of_talons;
    buff_t* dance_of_death;
    buff_t* haze_of_rage;
    buff_t* in_the_rhythm;
    buff_t* primal_instincts;
    buff_t* unerring_vision_driver;
    buff_t* unerring_vision;
    buff_t* up_close_and_personal;
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
    cooldown_t* harpoon;
    cooldown_t* wildfire_bomb;
  } cooldowns;

  // Custom Parameters
  std::string summon_pet_str;

  // Gains
  struct gains_t
  {
    gain_t* barbed_shot;
    gain_t* aspect_of_the_wild;
    gain_t* spitting_cobra;
    gain_t* nesingwarys_trapping_treads;
    gain_t* hunters_mark;
    gain_t* terms_of_engagement;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* wild_call;
    proc_t* zevrims_hunger;
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
  struct {
    bool enabled = false;
    double cost_multiplier = 0;
  } t19_mm_4pc;

  hunter_t( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) :
    player_t( sim, HUNTER, name, r ),
    pets(),
    legendary(),
    buffs(),
    cooldowns(),
    summon_pet_str( "cat" ),
    gains(),
    procs(),
    talents(),
    specs(),
    mastery(),
    current_hunters_mark_target( nullptr )
  {
    // Cooldowns
    cooldowns.bestial_wrath   = get_cooldown( "bestial_wrath" );
    cooldowns.trueshot        = get_cooldown( "trueshot" );
    cooldowns.barbed_shot     = get_cooldown( "barbed_shot" );
    cooldowns.kill_command    = get_cooldown( "kill_command" );
    cooldowns.harpoon         = get_cooldown( "harpoon" );
    cooldowns.aspect_of_the_wild  = get_cooldown( "aspect_of_the_wild" );
    cooldowns.a_murder_of_crows   = get_cooldown( "a_murder_of_crows" );
    cooldowns.aimed_shot      = get_cooldown( "aimed_shot" );
    cooldowns.wildfire_bomb   = get_cooldown( "wildfire_bomb" );

    base_gcd = timespan_t::from_seconds( 1.5 );

    regen_type = REGEN_DYNAMIC;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;

    talent_points.register_validity_fn( [ this ] ( const spell_data_t* spell )
    {
      // Soul of the Huntmaster
      if ( find_item( 151641 ) )
      {
        switch ( specialization() )
        {
          case HUNTER_BEAST_MASTERY:
            return spell -> id() == 257944; // Thrill of the Hunt
          case HUNTER_MARKSMANSHIP:
            return spell -> id() == 194595; // Lock and Load
          case HUNTER_SURVIVAL:
            return spell -> id() == 268501; // Viper's Venom
          default:
            return false;
        }
      }
      return false;
    } );
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
  action_t* create_action( const std::string& name, const std::string& options ) override;
  pet_t*    create_pet( const std::string& name, const std::string& type = std::string() ) override;
  void      create_pets() override;
  resource_e primary_resource() const override { return RESOURCE_FOCUS; }
  role_e    primary_role() const override { return ROLE_ATTACK; }
  stat_e    primary_stat() const override { return STAT_AGILITY; }
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

  bool track_cd_waste;
  cdwaste::action_data_t* cd_waste;

  struct {
    // bm
    bool aotw_crit_chance;
    bool aotw_gcd_reduce;
    bool bestial_wrath;
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
    track_cd_waste( s -> cooldown() > timespan_t::zero() || s -> charge_cooldown() > timespan_t::zero() ),
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
    affected_by.thrill_of_the_hunt = ab::data().affected_by( p() -> talents.thrill_of_the_hunt -> effectN( 1 ).trigger() -> effectN( 1 ) );

    affected_by.lone_wolf = ab::data().affected_by( p() -> specs.lone_wolf -> effectN( 1 ) );
    affected_by.sniper_training = ab::data().affected_by( p() -> mastery.sniper_training -> effectN( 2 ) );

    affected_by.spirit_bond = ab::data().affected_by( p() -> mastery.spirit_bond -> effectN( 1 ) );
    affected_by.coordinated_assault = ab::data().affected_by( p() -> specs.coordinated_assault -> effectN( 1 ) );
  }

  hunter_t* p()             { return static_cast<hunter_t*>( ab::player ); }
  const hunter_t* p() const { return static_cast<hunter_t*>( ab::player ); }

  hunter_td_t* td( player_t* t )             { return p() -> get_target_data( t ); }
  const hunter_td_t* find_td( const player_t* t ) const { return p() -> find_target_data( t ); }

  void init() override
  {
    ab::init();

    if ( track_cd_waste )
      cd_waste = p() -> cd_waste.get( this );
  }

  timespan_t gcd() const override
  {
    timespan_t g = ab::gcd();

    if ( g == timespan_t::zero() )
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

  void consume_resource() override
  {
    ab::consume_resource();

    if ( ab::last_resource_cost > 0 && p() -> sets -> has_set_bonus( HUNTER_MARKSMANSHIP, T19, B2 ) )
    {
      const double set_value = p() -> sets -> set( HUNTER_MARKSMANSHIP, T19, B2 ) -> effectN( 1 ).base_value();
      p() -> cooldowns.trueshot
        -> adjust( timespan_t::from_seconds( -1.0 * ab::last_resource_cost / set_value ) );
    }
  }

  double action_multiplier() const override
  {
    double am = ab::action_multiplier();

    if ( affected_by.bestial_wrath )
      am *= 1.0 + p() -> buffs.bestial_wrath -> check_value();

    if ( affected_by.coordinated_assault )
      am *= 1.0 + p() -> buffs.coordinated_assault -> check_value();

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

  double cost() const override
  {
    double cost = ab::cost();

    if ( p() -> t19_mm_4pc.enabled && p() -> buffs.trueshot -> check() )
      cost += cost * p() -> t19_mm_4pc.cost_multiplier;

    if ( p() -> legendary.bm_waist -> ok() && p() -> buffs.bestial_wrath -> check() )
      cost *= 1.0 + p() -> legendary.bm_waist -> effectN( 1 ).trigger() -> effectN( 1 ).percent();

    return cost;
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

  virtual void try_t20_2p_mm()
  {
    if ( !ab::background && p() -> sets -> has_set_bonus( HUNTER_MARKSMANSHIP, T20, B2 ) )
      p() -> buffs.pre_t20_2p_critical_aimed_damage -> expire();
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
};

void trigger_sephuzs_secret( hunter_t* p, const action_state_t* state, spell_mechanic type )
{
  if ( ! p -> legendary.sephuzs_secret -> ok() )
    return;

  // trigger by default on interrupts and on adds/lower level stuff
  if ( type == MECHANIC_INTERRUPT || state -> target -> is_add() ||
       ( state -> target -> level() < p -> sim -> max_player_level + 3 ) )
  {
    p -> buffs.sephuzs_secret -> trigger();
  }
}

void trigger_bloodseeker_update( hunter_t* );

struct hunter_ranged_attack_t: public hunter_action_t < ranged_attack_t >
{
  bool may_proc_mm_feet;

  hunter_ranged_attack_t( const std::string& n, hunter_t* player,
                          const spell_data_t* s = spell_data_t::nil() ):
                          hunter_action_t( n, player, s ),
                          may_proc_mm_feet( false )
  {}

  bool usable_moving() const override
  { return true; }

  void execute() override
  {
    hunter_action_t::execute();
    try_steady_focus();
    try_t20_2p_mm();

    if ( may_proc_mm_feet && p() -> legendary.mm_feet -> ok() )
      p() -> cooldowns.trueshot -> adjust( p() -> legendary.mm_feet -> effectN( 1 ).time_value() );
  }
};

struct hunter_melee_attack_t: public hunter_action_t < melee_attack_t >
{
  hunter_melee_attack_t( const std::string& n, hunter_t* p,
                          const spell_data_t* s = spell_data_t::nil() ):
                          hunter_action_t( n, p, s )
  {}
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
    try_t20_2p_mm();
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
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
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
  }

  void create_buffs() override
  {
    hunter_pet_t::create_buffs();

    buffs.bestial_wrath =
      make_buff( this, "bestial_wrath", find_spell( 186254 ) )
        -> set_default_value( find_spell( 186254 ) -> effectN( 1 ).percent() )
        -> set_cooldown( timespan_t::zero() )
        -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

    buffs.beast_cleave =
      make_buff( this, "beast_cleave", find_spell( 118455 ) )
        -> set_default_value( o() -> specs.beast_cleave -> effectN( 1 ).percent() );

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

    base_gcd = timespan_t::from_seconds( 1.50 );

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

  void summon( timespan_t duration = timespan_t::zero() ) override
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

  timespan_t available() const
  {
    // XXX: this will have to be changed if we ever add other foreground attacks to pets besides Basic Attacks
    if ( ! active.basic_attack )
      return hunter_main_pet_base_t::available();

    const auto time_to_fc = timespan_t::from_seconds( ( active.basic_attack -> base_cost() - resources.current[ RESOURCE_FOCUS ] ) /
                                                        resource_regen_per_second( RESOURCE_FOCUS ) );
    const auto time_to_cd = active.basic_attack -> cooldown -> remains();
    const auto remains = std::max( time_to_cd, time_to_fc );
    // 23/07/2018 - hunter pets seem to have a "generic" lag of about .6s on basic attack usage
    const auto lag = o() -> bugs ? timespan_t::from_millis( rng().gauss( 600, 100 ) ) : timespan_t::zero();
    return std::max( remains + lag, timespan_t::from_millis( 100 ) );
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

  void summon( timespan_t duration = timespan_t::zero() ) override
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

  dire_critter_t( hunter_t* owner ):
    hunter_pet_t( owner, "dire_beast", PET_HUNTER, true /*GUARDIAN*/ )
  {
    owner_coeff.ap_from_ap = 0.15;
    regen_type = REGEN_DISABLED;
  }

  void init_spells() override;

  void summon( timespan_t duration = timespan_t::zero() ) override
  {
    hunter_pet_t::summon( duration );

    if ( o() -> talents.stomp -> ok() )
      active.stomp -> execute();

    if ( main_hand_attack )
      main_hand_attack -> execute();
  }
};

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
    // Kill Command seems to use base damage in BfA
    base_dd_min = data().effectN( 1 ).min( p );
    base_dd_max = data().effectN( 1 ).max( p );

    if ( o() -> sets -> has_set_bonus( HUNTER_BEAST_MASTERY, T21, B2 ) )
      base_multiplier *= 1.0 + o() -> sets -> set( HUNTER_BEAST_MASTERY, T21, B2 ) -> effectN( 1 ).percent();

    if ( o() -> talents.killer_instinct -> ok() )
    {
      killer_instinct.percent = o() -> talents.killer_instinct -> effectN( 2 ).base_value();
      killer_instinct.multiplier = 1.0 + o() -> talents.killer_instinct -> effectN( 1 ).percent();
      killer_instinct.benefit = o() -> get_benefit( "killer_instinct" );
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = kill_command_base_t::bonus_da( s );
    /* It looks like KC kept the owner ap part of its damage going into bfa,
     * with the same coefficient. Model it as a simple bonus damage adder.
     */
    constexpr double owner_coeff_ = 3.0 / 4.0;
    return b + o() -> cache.attack_power() * o() -> composite_attack_power_multiplier() * owner_coeff_;
  }

  double action_multiplier() const override
  {
    double am = kill_command_base_t::action_multiplier();

    am *= 1.0 + o() -> buffs.t20_4p_bestial_rage -> value();

    return am;
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

    if ( ! o() -> talents.bloodseeker -> ok() )
      dot_duration = timespan_t::zero();

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
    trigger_gcd = timespan_t::zero();
  }

  void execute() override
  {
    player -> main_hand_attack -> schedule_execute();
  }

  bool ready() override
  { return ! target -> is_sleeping() && player -> main_hand_attack -> execute_event == nullptr; } // not swinging
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
  const double pack_alpha_bonus_da;

  basic_attack_t( hunter_main_pet_t* p, const std::string& n, const std::string& options_str ):
    hunter_main_pet_attack_t( n, p, p -> find_pet_spell( n ) ),
    venomous_fangs_bonus_da( p -> o() -> azerite.venomous_fangs.value( 1 ) ),
    pack_alpha_bonus_da( p -> o() -> azerite.pack_alpha.value( 1 ) )
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

    if ( o() -> azerite.pack_alpha.ok() )
    {
      const pet_t* pets[] = { o() -> pets.animal_companion, o() -> pets.dire_beast, o() -> pets.spitting_cobra };
      auto pet_count = range::count_if( pets, []( const pet_t* p ) { return p && !p -> is_sleeping(); } );
      // 28-06-2018: Pack Alpha seems to count the main pet if there are other ones up from the looks of it
      if ( pet_count > 0 && o() -> bugs )
        pet_count++;
      b += pack_alpha_bonus_da * pet_count;
    }

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

// T20 BM 2pc trigger
void trigger_t20_2pc_bm( hunter_t* p )
{
  if ( p -> sets -> has_set_bonus( HUNTER_BEAST_MASTERY, T20, B2 ) && p -> buffs.bestial_wrath -> check() )
  {
    const spell_data_t* driver = p -> sets -> set( HUNTER_BEAST_MASTERY, T20, B2 ) -> effectN( 1 ).trigger();
    const double bonus = driver -> effectN( 1 ).percent() / 10.0;
    p -> buffs.bestial_wrath -> current_value += bonus;
    for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p -> pets.main, p -> pets.animal_companion ) )
      pet -> buffs.bestial_wrath -> current_value += bonus;

    if ( p -> sim -> debug )
    {
      p -> sim -> out_debug.print( "{} triggers t20 2pc: {} ( value={:.3f} )",
                                   p -> name(), p -> buffs.bestial_wrath -> name(),
                                   p -> buffs.bestial_wrath -> check_value() );
    }
  }
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

namespace attacks
{

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

struct auto_shot_t: public hunter_action_t < ranged_attack_t >
{
  volley_t* volley = nullptr;
  bool first_shot = true;

  auto_shot_t( hunter_t* p ):
    hunter_action_t( "auto_shot", p, p -> find_spell( 75 ) )
  {
    background = true;
    repeating = true;
    interrupt_auto_attack = false;
    special = false;
    trigger_gcd = timespan_t::zero();

    weapon = &( p -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;

    if ( p -> talents.volley -> ok() )
    {
      volley = p -> get_background_action<volley_t>( "volley" );
      add_child( volley );
    }
  }

  void reset() override
  {
    hunter_action_t::reset();
    first_shot = true;
  }

  timespan_t execute_time() const override
  {
    if ( first_shot )
      return timespan_t::from_millis( 100 );
    return hunter_action_t::execute_time();
  }

  void execute() override
  {
    if ( first_shot )
      first_shot = false;

    hunter_action_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    hunter_action_t::impact( s );

    if ( p() -> buffs.lock_and_load -> trigger() )
      p() -> cooldowns.aimed_shot -> reset( true );

    if ( s -> result == RESULT_CRIT && p() -> specialization() == HUNTER_BEAST_MASTERY )
    {
      double wild_call_chance = p() -> specs.wild_call -> proc_chance() +
                                p() -> talents.one_with_the_pack -> effectN( 1 ).percent();

      if ( rng().roll( wild_call_chance ) )
      {
        p() -> cooldowns.barbed_shot -> reset( true );
        p() -> procs.wild_call -> occur();
      }
    }

    if ( volley && rng().roll( p() -> talents.volley -> proc_chance() ) )
    {
      volley -> set_target( s -> target );
      volley -> execute();
    }
  }
};

struct start_attack_t: public action_t
{
  start_attack_t( hunter_t* p, const std::string& options_str ):
    action_t( ACTION_OTHER, "start_auto_shot", p )
  {
    parse_options( options_str );

    p -> main_hand_attack = new auto_shot_t( p );
    stats = p -> main_hand_attack -> stats;
    ignore_false_positive = true;

    trigger_gcd = timespan_t::zero();
  }

  void execute() override
  {
    player -> main_hand_attack -> schedule_execute();
  }

  bool ready() override
  { return ! target -> is_sleeping() && player -> main_hand_attack -> execute_event == nullptr; } // not swinging
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
    callbacks = false;
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
      p() -> main_hand_attack -> reschedule_execute( dot_duration * composite_haste() + timespan_t::from_millis( 500 ) );
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
      // TODO: check if it reduces aotw cd
    }
  };

  struct {
    rapid_reload_t* action = nullptr;
    int min_targets = std::numeric_limits<int>::max();
  } rapid_reload;

  multi_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "multishot", p, p -> find_specialization_spell( "Multi-Shot" ) )
  {
    parse_options( options_str );
    may_proc_mm_feet = true;
    aoe = -1;

    base_multiplier *= 1.0 + p -> sets -> set( HUNTER_BEAST_MASTERY, T19, B4 ) -> effectN( 1 ).percent();

    if ( p -> azerite.rapid_reload.ok() )
    {
      rapid_reload.action = p -> get_background_action<rapid_reload_t>( "multishot_rapid_reload" );
      rapid_reload.min_targets = p -> azerite.rapid_reload.spell() -> effectN( 2 ).base_value();
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

    am *= 1.0 + p() -> buffs.t20_4p_bestial_rage -> value();
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

    trigger_t20_2pc_bm( p() );

    if ( rapid_reload.action && num_targets_hit >= rapid_reload.min_targets )
    {
      rapid_reload.action -> set_target( target );
      rapid_reload.action -> execute();
    }
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    p() -> buffs.sentinels_sight -> trigger();
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

    callbacks = false;
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

    base_multiplier *= 1.0 + p() -> sets -> set( HUNTER_BEAST_MASTERY, T19, B2 ) -> effectN( 1 ).percent();
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> cooldowns.kill_command -> adjust( -kill_command_reduction );

    if ( p() -> talents.venomous_bite -> ok() )
      p() -> cooldowns.bestial_wrath -> adjust( -venomous_bite_reduction );

    if ( p() -> talents.killer_cobra -> ok() && p() -> buffs.bestial_wrath -> check() )
      p() -> cooldowns.kill_command -> reset( true );

    p() -> buffs.parsels_tongue -> trigger();

    trigger_t20_2pc_bm( p() );
  }

  double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    am *= 1.0 + p() -> buffs.t20_4p_bestial_rage -> value();

    return am;
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

    base_ta_adder += p -> azerite.feeding_frenzy.value( 2 );
  }

  void init_finished() override
  {
    for ( auto pet : p() -> pet_list )
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

    if ( p() -> legendary.bm_feet -> ok() )
      p() -> cooldowns.kill_command -> adjust( p() -> legendary.bm_feet -> effectN( 1 ).time_value() );

    p() -> buffs.the_mantle_of_command -> trigger();

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

  void init() override
  {
    if ( p() -> legendary.mm_wrist -> ok() )
      base_multiplier *= 1.0 + p() -> legendary.mm_wrist -> effectN( 2 ).percent();

    hunter_ranged_attack_t::init();
  }
};

// Aimed Shot base class ==============================================================

struct aimed_shot_base_t: public hunter_ranged_attack_t
{
  benefit_t *const careful_aim;
  const int trick_shots_targets;

  aimed_shot_base_t( const std::string& name, hunter_t* p ):
    hunter_ranged_attack_t( name, p, p -> specs.aimed_shot ),
    careful_aim( p -> talents.careful_aim -> ok() ? p -> get_benefit( "careful_aim" ) : nullptr ),
    trick_shots_targets( static_cast<int>( p -> specs.trick_shots -> effectN( 1 ).base_value() ) )
  {
    radius = 8.0;
    base_aoe_multiplier = p -> specs.trick_shots -> effectN( 4 ).percent();

    base_multiplier *= 1.0 + p -> sets -> set( HUNTER_MARKSMANSHIP, T21, B4 ) -> effectN( 1 ).percent();
  }

  timespan_t execute_time() const override
  {
    timespan_t t = hunter_ranged_attack_t::execute_time();

    if ( p() -> buffs.t20_4p_precision -> check() )
      t *= 1.0 + p() -> buffs.t20_4p_precision -> data().effectN( 1 ).percent();

    return t;
  }

  int n_targets() const override
  {
    if ( p() -> buffs.trick_shots -> check() )
      return trick_shots_targets + 1;
    return hunter_ranged_attack_t::n_targets();
  }

  double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    am *= 1.0 + p() -> buffs.sentinels_sight -> stack_value();

    return am;
  }

  double composite_crit_chance() const override
  {
    double cc = hunter_ranged_attack_t::composite_crit_chance();

    cc += p() -> buffs.gyroscopic_stabilization -> value();

    if ( p() -> buffs.lethal_shots -> up() )
      cc += p() -> buffs.lethal_shots -> data().effectN( 2 ).percent();

    return cc;
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = hunter_ranged_attack_t::composite_target_da_multiplier( t );

    auto td = find_td( t );
    if ( !( td && td -> damaged ) )
      m *= 1.0 + data().effectN( 2 ).percent();

    if ( careful_aim )
    {
      const bool active =
        t -> health_percentage() > p() -> talents.careful_aim -> effectN( 1 ).base_value() ||
        t -> health_percentage() < p() -> talents.careful_aim -> effectN( 2 ).base_value();
      const bool procced = active && rng().roll( p() -> talents.careful_aim -> effectN( 4 ).percent() );
      careful_aim -> update( procced );
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

  benefit_t* aimed_in_critical_aimed;
  aimed_shot_secondary_t* double_tap = nullptr;
  bool lock_and_loaded = false;
  struct {
    proc_t* double_tap;
    proc_t* lethal_shots;
  } procs;

  aimed_shot_t( hunter_t* p, const std::string& options_str ) :
    aimed_shot_base_t( "aimed_shot", p ),
    aimed_in_critical_aimed( p -> get_benefit( "aimed_in_critical_aimed" ) )
  {
    parse_options( options_str );

    may_proc_mm_feet = true;

    if ( p -> talents.double_tap -> ok() )
    {
      double_tap = p -> get_background_action<aimed_shot_secondary_t>( "aimed_shot_double_tap" );
      add_child( double_tap );
    }

    procs.double_tap = p -> get_proc( "double_tap_aimed" );
    procs.lethal_shots = p -> get_proc( "lethal_shots_aimed" );
  }

  double cost() const override
  {
    if ( lock_and_loaded )
      return 0;

    double cost = aimed_shot_base_t::cost();

    if ( p() -> buffs.t20_4p_precision -> check() )
      cost *= 1.0 + p() -> buffs.t20_4p_precision -> check_value();

    return cost;
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

    if ( p() -> buffs.lethal_shots -> check() )
      procs.lethal_shots -> occur();
    p() -> buffs.lethal_shots -> decrement();

    p() -> buffs.sentinels_sight -> up(); // benefit tracking
    p() -> buffs.sentinels_sight -> expire();

    if ( p() -> buffs.gyroscopic_stabilization -> up() )
      p() -> buffs.gyroscopic_stabilization -> expire();
    else
      p() -> buffs.gyroscopic_stabilization -> trigger();

    p() -> buffs.precise_shots -> trigger( 1 + rng().range( p() -> buffs.precise_shots -> max_stack() ) );
    p() -> buffs.master_marksman -> trigger();
    p() -> buffs.t20_4p_precision -> trigger();

    if ( p() -> sets -> has_set_bonus( HUNTER_MARKSMANSHIP, T20, B2 ) )
    {
      p() -> buffs.pre_t20_2p_critical_aimed_damage -> trigger();
      if ( p() -> buffs.pre_t20_2p_critical_aimed_damage -> check() == 2 )
      {
        p() -> buffs.t20_2p_critical_aimed_damage -> trigger();
        p() -> buffs.pre_t20_2p_critical_aimed_damage-> expire();
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    aimed_shot_base_t::impact( s );
    aimed_in_critical_aimed -> update( p() -> buffs.t20_2p_critical_aimed_damage -> check() != 0 );
  }

  timespan_t execute_time() const override
  {
    if ( p() -> buffs.lock_and_load -> check() )
      return timespan_t::zero();

    return aimed_shot_base_t::execute_time();
  }

  bool usable_moving() const override
  {
    if ( p() -> buffs.gyroscopic_stabilization -> check() )
      return true;
    return false;
  }

  void try_t20_2p_mm() override { }
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

    p() -> buffs.arcane_flurry -> trigger();

    if ( p() -> talents.calling_the_shots -> ok() )
      p() -> cooldowns.trueshot -> adjust( - p() -> talents.calling_the_shots -> effectN( 1 ).time_value() );
  }

  double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    am *= 1.0 + p() -> buffs.precise_shots -> value();

    return am;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = hunter_ranged_attack_t::bonus_da( s );

    b += p() -> buffs.arcane_flurry -> stack_value();

    return b;
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

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> buffs.lethal_shots -> trigger();
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

    rapid_fire_damage_t( const std::string& n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> find_spell( 257044 ) -> effectN( 2 ).trigger() ),
      trick_shots_targets( as<int>( p -> specs.trick_shots -> effectN( 3 ).base_value() ) )
    {
      dual = true;
      direct_tick = true;
      radius = 8.0;

      parse_effect_data( p -> find_spell( 263585 ) -> effectN( 1 ) );

      base_dd_adder += p -> azerite.focused_fire.value( 2 );
      if ( p -> azerite.focused_fire.ok() )
      {
        auto trigger_ = p -> azerite.focused_fire.spell() -> effectN( 1 ).trigger();
        focused_fire.chance = trigger_ -> proc_chance();
        focused_fire.amount = trigger_ -> effectN( 1 ).trigger() -> effectN( 1 ).base_value();
        focused_fire.gain = p -> get_gain( "focused_fire" );
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

    double composite_crit_chance() const override
    {
      double cc = hunter_ranged_attack_t::composite_crit_chance();

      cc += p() -> buffs.lethal_shots -> value();

      return cc;
    }
  };

  rapid_fire_damage_t* damage;
  int base_num_ticks;
  struct {
    proc_t* double_tap;
    proc_t* lethal_shots;
  } procs;

  rapid_fire_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "rapid_fire", p, p -> find_spell( 257044 ) ),
    damage( p -> get_background_action<rapid_fire_damage_t>( "rapid_fire_damage" ) ),
    base_num_ticks( data().effectN( 1 ).base_value() )
  {
    parse_options( options_str );

    may_miss = may_crit = false;
    callbacks = false;
    channeled = true;
    tick_zero = true;

    base_num_ticks *= 1.0 + p -> talents.streamline -> effectN( 2 ).percent();
    dot_duration += p -> talents.streamline -> effectN( 1 ).time_value();

    procs.double_tap = p -> get_proc( "double_tap_rapid_fire" );
    procs.lethal_shots = p -> get_proc( "lethal_shots_rapid_fire" );
  }

  void init()
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

    if ( p() -> buffs.lethal_shots -> check() )
      procs.lethal_shots -> occur();
    p() -> buffs.lethal_shots -> decrement();

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
      num_ticks_ *= 1.0 + p() -> talents.double_tap -> effectN( 3 ).percent();
    return num_ticks_;
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

struct explosive_shot_t: public hunter_spell_t
{
  static constexpr timespan_t precombat_travel_time = timespan_t::from_millis( 100 );
  bool in_combat = false;

  struct explosive_shot_impact_t: hunter_ranged_attack_t
  {
    explosive_shot_impact_t( const std::string& n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> find_spell( 212680 ) )
    {
      background = true;
      dual = true;
      aoe = -1;
    }
  };

  explosive_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "explosive_shot", p, p -> talents.explosive_shot )
  {
    parse_options( options_str );

    travel_speed = 20.0;
    may_miss = false;

    impact_action = p -> get_background_action<explosive_shot_impact_t>( "explosive_shot_impact" );
    impact_action -> stats = stats;
  }

  timespan_t travel_time() const override
  {
    if ( in_combat )
      return hunter_spell_t::travel_time();

    return precombat_travel_time;
  }

  void execute() override
  {
    in_combat = player -> in_combat;

    hunter_spell_t::execute();

    if ( ! in_combat )
    {
      in_combat = player -> in_combat;
      const timespan_t travel_time_ = travel_time() - precombat_travel_time;
      // TODO: handle standing in melee range?
      cooldown -> adjust( -travel_time_ );
      player -> regen( travel_time_ );
    }
  }

  void reset() override
  {
    hunter_spell_t::reset();
    in_combat = false;
  }
};
constexpr timespan_t explosive_shot_t::precombat_travel_time;

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

struct melee_t: public hunter_melee_attack_t
{
  bool first = true;

  melee_t( hunter_t* player ):
    hunter_melee_attack_t( "auto_attack_mh", player )
  {
    school             = SCHOOL_PHYSICAL;
    base_execute_time  = player -> main_hand_weapon.swing_time;
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier  = 1.0;
    background         = true;
    repeating          = true;
    may_glance         = true;
    special            = false;
    trigger_gcd        = timespan_t::zero();

    // technically there is a separate effect for auto attacks, but meh
    affected_by.coordinated_assault = true;
  }

  timespan_t execute_time() const override
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );
    if ( first )
      return timespan_t::zero();
    else
      return hunter_melee_attack_t::execute_time();
  }

  void execute() override
  {
    if ( first )
      first = false;
    hunter_melee_attack_t::execute();
  }
};

// Auto attack =======================================================================

struct auto_attack_t: public action_t
{
  auto_attack_t( hunter_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "auto_attack", player )
  {
    parse_options( options_str );
    player -> main_hand_attack = new melee_t( player );
    ignore_false_positive = true;
    range = 5;
    trigger_gcd = timespan_t::zero();
  }

  void execute() override
  {
    player -> main_hand_attack -> schedule_execute();
  }

  bool ready() override
  {
    if ( player -> is_moving() )
      return false;

    return ! target -> is_sleeping() && player -> main_hand_attack -> execute_event == nullptr; // not swinging
  }
};

// Internal Bleeding (Wildfire Infusion) ===============================================

struct internal_bleeding_t
{
  struct internal_bleeding_action_t: hunter_melee_attack_t
  {
    internal_bleeding_action_t( const std::string& n, hunter_t* p ):
      hunter_melee_attack_t( n, p, p -> find_spell( 270343 ) )
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
    auto p = static_cast<const hunter_t*>( s -> action -> player );
    auto td = p -> find_target_data( s -> target );
    if ( action && td && td -> dots.shrapnel_bomb -> is_ticking() )
    {
      action -> set_target( s -> target );
      action -> execute();
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
    base_dd_adder += p -> azerite.wilderness_survival.value( 2 );

    if ( p -> azerite.latent_poison.ok() )
      latent_poison = p -> get_background_action<latent_poison_t>( "latent_poison" );
  }

  double cost() const override
  {
    double c = hunter_melee_attack_t::cost();

    c -= p() -> buffs.up_close_and_personal -> check_value();

    return c;
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    p() -> buffs.vipers_venom -> trigger();
    if ( p() -> buffs.coordinated_assault -> check() )
      p() -> buffs.blur_of_talons -> trigger();
    p() -> buffs.butchers_bone_apron -> trigger();

    trigger_birds_of_prey( p(), target );
    if ( wilderness_survival_reduction != timespan_t::zero() )
      p() -> cooldowns.wildfire_bomb -> adjust( -wilderness_survival_reduction );

    p() -> buffs.up_close_and_personal -> expire();
  }

  void impact( action_state_t* s ) override
  {
    hunter_melee_attack_t::impact( s );

    internal_bleeding.trigger( s );

    if ( latent_poison )
      latent_poison -> trigger( s -> target );
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = hunter_melee_attack_t::bonus_da( s );

    if ( p() -> buffs.up_close_and_personal -> up() )
      b += p() -> azerite.up_close_and_personal.value( 1 );

    return b;
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

    add_child( damage );
  }

  void init_finished() override
  {
    for ( auto pet : p() -> pet_list )
      add_pet_stats( pet, { "flanking_strike" } );

    hunter_melee_attack_t::init_finished();
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    damage -> set_target( target );
    damage -> execute();

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

  void init()
  {
    hunter_melee_attack_t::init();

    base_multiplier *= 1.0 + p() -> legendary.sv_ring -> effectN( 1 ).percent();
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    auto reduction = wfb_reduction * std::min( num_targets_hit, wfb_reduction_target_cap );
    p() -> cooldowns.wildfire_bomb -> adjust( -reduction, true );

    p() -> buffs.butchers_bone_apron -> expire();
  }

  void impact( action_state_t* s ) override
  {
    hunter_melee_attack_t::impact( s );

    trigger_birds_of_prey( p(), s -> target );
    internal_bleeding.trigger( s );
  }

  double action_multiplier() const override
  {
    double am = hunter_melee_attack_t::action_multiplier();

    am *= 1.0 + p() -> buffs.butchers_bone_apron -> stack_value();

    return am;
  }
};

// Carve =============================================================================

struct carve_t: public carve_base_t
{
  carve_t( hunter_t* p, const std::string& options_str ):
    carve_base_t( "carve", p, p -> specs.carve ,
                  p -> specs.carve -> effectN( 2 ).time_value(),
                  p -> specs.carve -> effectN( 3 ).base_value() )
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
                  p -> talents.butchery -> effectN( 3 ).base_value() )
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
    base_multiplier *= 1.0 + p -> find_spell( 262839 ) -> effectN( 1 ).percent(); // Raptor Strike (Rank 2)
    base_multiplier *= 1.0 + p -> sets -> set( HUNTER_SURVIVAL, T21, B4 ) -> effectN( 1 ).percent();

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
  struct terms_of_engagement_t : hunter_melee_attack_t
  {
    terms_of_engagement_t( const std::string& n, hunter_t* p ):
      hunter_melee_attack_t( n, p, p -> find_spell( 271625 ) )
    {
      dual = true;
    }

    void impact( action_state_t* s ) override
    {
      hunter_melee_attack_t::impact( s );

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

    cooldown -> duration += p -> find_spell( 231550 ) -> effectN( 1 ).time_value(); // Harpoon (Rank 2)

    if ( p -> talents.terms_of_engagement -> ok() )
    {
      terms_of_engagement = p -> get_background_action<terms_of_engagement_t>( "harpoon_terms_of_engagement" );
      add_child( terms_of_engagement );
    }
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    if ( terms_of_engagement )
    {
      terms_of_engagement -> set_target( target );
      terms_of_engagement -> execute();
    }

    if ( p() -> legendary.sv_waist -> ok() )
    {
      if ( ! td( target ) -> helbrined )
      {
        td( target ) -> debuffs.mark_of_helbrine -> trigger();
        td( target ) -> helbrined = true;
      }
    }

    p() -> buffs.up_close_and_personal -> trigger();
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
    tick_may_crit = false;

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

    m *= 1.0 + p() -> buffs.vipers_venom -> value();

    return m;
  }

  double composite_crit_chance() const override
  {
    double cc = hunter_ranged_attack_t::composite_crit_chance();

    cc += p() -> sets -> set( HUNTER_SURVIVAL, T19, B2 ) -> effectN( 1 ).percent();

    return cc;
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
        [ this ]( const player_t* t ) {
          return !( find_td( t ) && find_td( t ) -> dots.serpent_sting -> is_ticking() );
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

  bool ready() override
  {
    if ( ! target -> debuffs.casting || ! target -> debuffs.casting -> check() ) return false;
    return hunter_spell_t::ready();
  }

  void execute() override
  {
    hunter_spell_t::execute();

    trigger_sephuzs_secret( p(), execute_state, MECHANIC_INTERRUPT );
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

    double action_multiplier() const override
    {
      double am = hunter_ranged_attack_t::action_multiplier();

      if ( p() -> mastery.master_of_beasts -> ok() )
          am *= 1.0 + p() -> cache.mastery_value();

      if ( p() -> buffs.the_mantle_of_command -> up() )
        am *= 1.0 + p() -> buffs.the_mantle_of_command -> data().effectN( 2 ).percent();

      return am;
    }
  };

  moc_t( hunter_t* p, const std::string& options_str ) :
    hunter_spell_t( "a_murder_of_crows", p, p -> talents.a_murder_of_crows )
  {
    parse_options( options_str );

    hasted_ticks = false;
    callbacks = false;
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

  summon_pet_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "summon_pet", player ),
    opt_disabled( false ), pet( nullptr )
  {
    add_option( opt_string( "name", pet_name ) );
    parse_options( options_str );

    harmful = may_hit = false;
    callbacks = false;
    ignore_false_positive = true;

    if ( pet_name.empty() )
      pet_name = p() -> summon_pet_str;
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

  void execute() override
  {
    hunter_spell_t::execute();

    if ( p() -> legendary.sv_feet -> ok() )
      p() -> resource_gain( RESOURCE_FOCUS, p() -> find_spell( 212575 ) -> effectN( 1 ).resource( RESOURCE_FOCUS ), p() -> gains.nesingwarys_trapping_treads );
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

  void execute() override
  {
    hunter_spell_t::execute();

    if ( p() -> legendary.sv_feet -> ok() )
      p() -> resource_gain( RESOURCE_FOCUS, p() -> find_spell( 212575 ) -> effectN( 1 ).resource( RESOURCE_FOCUS ), p() -> gains.nesingwarys_trapping_treads );
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

  kill_command_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "kill_command", p, p -> specs.kill_command ),
    flankers_advantage( p -> get_proc( "flankers_advantage" ) )
  {
    parse_options( options_str );

    harmful = may_hit = false;

    cooldown -> charges += static_cast<int>( p -> talents.alpha_predator -> effectN( 1 ).base_value() );
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

    trigger_t20_2pc_bm( p() );

    if ( p() -> sets -> has_set_bonus( HUNTER_BEAST_MASTERY, T21, B4 ) )
    {
      auto reduction = p() -> sets -> set( HUNTER_BEAST_MASTERY, T21, B4 ) -> effectN( 1 ).time_value();
      p() -> cooldowns.aspect_of_the_wild -> adjust( - reduction );
    }
  }

  bool ready() override
  {
    if ( p() -> pets.main && p() -> pets.main -> active.kill_command -> ready() ) // Range check from the pet.
      return hunter_spell_t::ready();

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
          return timespan_t::zero();
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
  dire_beast_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "dire_beast", player, player -> talents.dire_beast )
  {
    parse_options( options_str );

    harmful = may_hit = false;
    dot_duration = timespan_t::zero();
  }

  void init_finished() override
  {
    add_pet_stats( p() -> pets.dire_beast, { "dire_beast_melee", "stomp" } );

    hunter_spell_t::init_finished();
  }

  void execute() override
  {
    hunter_spell_t::execute();

    // Dire beast gets a chance for an extra attack based on haste
    // rather than discrete plateaus.  At integer numbers of attacks,
    // the beast actually has a 50% chance of n-1 attacks and 50%
    // chance of n.  It (apparently) scales linearly between n-0.5
    // attacks to n+0.5 attacks.  This uses beast duration to
    // effectively alter the number of attacks as the duration itself
    // isn't important and combat log testing shows some variation in
    // attack speeds.  This is not quite perfect but more accurate
    // than plateaus.
    auto beast = p() -> pets.dire_beast;
    const timespan_t base_duration = data().duration();
    const timespan_t swing_time = beast -> main_hand_weapon.swing_time * beast -> composite_melee_speed();
    double partial_attacks_per_summon = base_duration / swing_time;
    int base_attacks_per_summon = static_cast<int>(partial_attacks_per_summon);
    partial_attacks_per_summon -= static_cast<double>(base_attacks_per_summon );

    if ( rng().roll( partial_attacks_per_summon ) )
      base_attacks_per_summon += 1;

    timespan_t summon_duration = base_attacks_per_summon * swing_time;

    sim -> print_debug( "Dire Beast summoned with {:4.1f} autoattacks", base_attacks_per_summon );

    beast -> summon( summon_duration );

    p() -> buffs.dire_beast -> trigger();
  }
};

// Bestial Wrath ============================================================

struct bestial_wrath_t: public hunter_spell_t
{
  bestial_wrath_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "bestial_wrath", player, player -> specs.bestial_wrath )
  {
    parse_options( options_str );
    harmful = may_hit = false;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.bestial_wrath  -> trigger();
    p() -> buffs.t20_4p_bestial_rage -> trigger();
    p() -> buffs.haze_of_rage -> trigger();

    for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p() -> pets.main, p() -> pets.animal_companion ) )
      pet -> buffs.bestial_wrath -> trigger();
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
  aspect_of_the_wild_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "aspect_of_the_wild", p, p -> specs.aspect_of_the_wild )
  {
    parse_options( options_str );

    harmful = may_hit = false;
    dot_duration = timespan_t::zero();
  }

  void execute() override
  {
    p() -> buffs.aspect_of_the_wild -> trigger();

    hunter_spell_t::execute();

    if ( p() -> buffs.primal_instincts -> trigger() )
      p() -> cooldowns.barbed_shot -> reset( true );
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

    base_tick_time = timespan_t::from_millis( 667 );
    dot_duration = data().duration();
    hasted_ticks = false;
    radius = 8;
    school = SCHOOL_PHYSICAL;

    tick_action = new stampede_tick_t( p );
  }

  double action_multiplier() const override
  {
    double am = hunter_spell_t::action_multiplier();

    if ( p() -> mastery.master_of_beasts -> ok() )
      am *= 1.0 + p() -> cache.mastery_value();

    if ( p() -> buffs.the_mantle_of_command -> up() )
      am *= 1.0 + p() -> buffs.the_mantle_of_command -> data().effectN( 2 ).percent();

    return am;
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
    dot_duration = timespan_t::zero();
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
  trueshot_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "trueshot", p, p -> specs.trueshot )
  {
    parse_options( options_str );

    harmful = may_hit = false;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> cooldowns.aimed_shot -> reset( true );
    p() -> buffs.trueshot -> trigger();
    p() -> buffs.unerring_vision_driver -> trigger();
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
  timespan_t precast_time = timespan_t::zero();

  double_tap_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "double_tap", p, p -> talents.double_tap )
  {
    add_option( opt_timespan( "precast_time", precast_time ) );
    parse_options( options_str );

    harmful = may_hit = false;
    dot_duration = timespan_t::zero();

    precast_time = clamp( precast_time, timespan_t::zero(), data().duration() );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.double_tap -> trigger();

    // adjust for precasting
    if ( ! player -> in_combat && precast_time != timespan_t::zero() )
    {
      p() -> buffs.double_tap -> extend_duration( player, -precast_time );
      cooldown -> adjust( -precast_time );
    }
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

    void execute() override
    {
      hunter_spell_t::execute();

      if ( p() -> legendary.sv_feet -> ok() )
        p() -> resource_gain( RESOURCE_FOCUS, p() -> find_spell( 212575 ) -> effectN( 1 ).resource( RESOURCE_FOCUS ), p() -> gains.nesingwarys_trapping_treads );
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

hunter_td_t::hunter_td_t( player_t* target, hunter_t* p ):
  actor_target_data_t( target, p ),
  debuffs(),
  dots()
{
  debuffs.mark_of_helbrine =
    make_buff( *this, "mark_of_helbrine", p -> find_spell( 213156 ) )
      -> set_default_value( p -> find_spell( 213154 ) -> effectN( 1 ).percent() )
      -> set_cooldown( p -> sim -> max_time * 3 );

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

  damaged = false;
  helbrined = false;
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
            return timespan_t::zero().total_seconds();

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

          if ( trueshot_cd -> last_charged == timespan_t::zero() || trueshot_cd -> remains() == trueshot_cd -> duration )
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
      return make_fn_expr( expression_str, [ this ]() { return next_wi_bomb == WILDFIRE_INFUSION_SHRAPNEL; } );
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
  if ( name == "auto_attack"           ) return new            auto_attack_t( this, options_str );
  if ( name == "auto_shot"             ) return new           start_attack_t( this, options_str );
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
  if ( !util::str_compare_ci( summon_pet_str, "disabled" ) )
    create_pet( summon_pet_str, summon_pet_str );

  if ( talents.animal_companion -> ok() )
    pets.animal_companion = new pets::animal_companion_t( this );

  if ( talents.dire_beast -> ok() )
    pets.dire_beast = new pets::dire_critter_t( this );

  if ( talents.spitting_cobra -> ok() )
    pets.spitting_cobra = new pets::spitting_cobra_t( this );
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
  azerite.feeding_frenzy        = find_azerite_spell( "Feeding Frenzy" );
  azerite.haze_of_rage          = find_azerite_spell( "Haze of Rage" );
  azerite.pack_alpha            = find_azerite_spell( "Pack Alpha" );
  azerite.primal_instincts      = find_azerite_spell( "Primal Instincts" );
  azerite.serrated_jaws         = find_azerite_spell( "Serrated Jaws" );

  azerite.arcane_flurry         = find_azerite_spell( "Arcane Flurry" );
  azerite.focused_fire          = find_azerite_spell( "Focused Fire" );
  azerite.in_the_rhythm         = find_azerite_spell( "In The Rhythm" );
  azerite.rapid_reload          = find_azerite_spell( "Rapid Reload" );
  azerite.steady_aim            = find_azerite_spell( "Steady Aim" );
  azerite.unerring_vision       = find_azerite_spell( "Unerring Vision" );

  azerite.blur_of_talons        = find_azerite_spell( "Blur of Talons" );
  azerite.latent_poison         = find_azerite_spell( "Latent Poison" );
  azerite.up_close_and_personal = find_azerite_spell( "Up Close And Personal" );
  azerite.venomous_fangs        = find_azerite_spell( "Venomous Fangs" );
  azerite.wilderness_survival   = find_azerite_spell( "Wilderness Survival" );
  azerite.wildfire_cluster      = find_azerite_spell( "Wildfire Cluster" );

  t19_mm_4pc.enabled = sets -> has_set_bonus( HUNTER_MARKSMANSHIP, T19, B4 );
  t19_mm_4pc.cost_multiplier = find_spell( 211327 ) -> effectN( 1 ).percent();
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
}

// hunter_t::init_buffs =====================================================

void hunter_t::create_buffs()
{
  player_t::create_buffs();

  // Beast Mastery

  buffs.aspect_of_the_wild =
    make_buff( this, "aspect_of_the_wild", specs.aspect_of_the_wild )
      -> set_cooldown( timespan_t::zero() )
      -> set_activated( true )
      -> set_default_value( specs.aspect_of_the_wild -> effectN( 1 ).percent() )
      -> set_tick_callback( [ this ]( buff_t *b, int, const timespan_t& ){
                        resource_gain( RESOURCE_FOCUS, b -> data().effectN( 2 ).resource( RESOURCE_FOCUS ), gains.aspect_of_the_wild );
                        if ( auto pet = pets.main )
                          pet -> resource_gain( RESOURCE_FOCUS, b -> data().effectN( 5 ).resource( RESOURCE_FOCUS ), pet -> gains.aspect_of_the_wild );
                      } );

  buffs.bestial_wrath =
    make_buff( this, "bestial_wrath", specs.bestial_wrath )
      -> set_cooldown( timespan_t::zero() )
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
    make_buff( this, "dire_beast", talents.dire_beast -> effectN( 2 ).trigger() )
      -> set_default_value( talents.dire_beast -> effectN( 2 ).trigger() -> effectN( 1 ).percent() )
      ->add_invalidate(CACHE_HASTE);

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
      -> set_default_value( specs.trueshot -> effectN( 1 ).percent() )
      -> set_cooldown( timespan_t::zero() )
      -> set_activated( true )
      ->add_invalidate(CACHE_HASTE);

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
      -> set_activated( true )
      -> set_default_value( talents.double_tap -> effectN( 1 ).percent() );

  buffs.lethal_shots =
    make_buff( this, "lethal_shots", talents.lethal_shots -> effectN( 1 ).trigger() )
      -> set_default_value( talents.lethal_shots -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
      -> set_trigger_spell( talents.lethal_shots );

  // Survival

  buffs.coordinated_assault =
    make_buff( this, "coordinated_assault", specs.coordinated_assault )
      -> set_cooldown( timespan_t::zero() )
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
      -> set_cooldown( timespan_t::zero() );

  // Legendaries

  buffs.sentinels_sight =
    make_buff( this, "sentinels_sight", find_spell( 208913 ) )
    ->set_default_value( find_spell( 208913 ) -> effectN( 1 ).percent() )
    ->set_max_stack( find_spell( 208913 ) -> max_stacks() );

  buffs.butchers_bone_apron =
    make_buff( this, "butchers_bone_apron", find_spell( 236446 ) )
    ->set_default_value( find_spell( 236446 ) -> effectN( 1 ).percent() );

  buffs.gyroscopic_stabilization =
    make_buff( this, "gyroscopic_stabilization", find_spell( 235712 ) )
    ->set_default_value( find_spell( 235712 ) -> effectN( 2 ).percent() );

  buffs.the_mantle_of_command =
    make_buff( this, "the_mantle_of_command", find_spell( 247993 ) )
    ->set_default_value( find_spell( 247993 ) -> effectN( 1 ).percent() );

  buffs.parsels_tongue =
    make_buff( this, "parsels_tongue", find_spell( 248085 ) )
    ->set_default_value( find_spell( 248085 ) -> effectN( 2 ).percent() )
    ->set_max_stack( find_spell( 248085 ) -> max_stacks() )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.sephuzs_secret =
    make_buff( this, "sephuzs_secret", find_spell( 208052 ) )
      -> set_default_value( find_spell( 208052 ) -> effectN( 2 ).percent() )
      -> set_cooldown( find_spell( 226262 ) -> duration() )
      ->add_invalidate(CACHE_HASTE);

  // Sets

  buffs.t20_4p_precision =
    make_buff( this, "t20_4p_precision", find_spell( 246153 ) )
      -> set_default_value( find_spell( 246153 ) -> effectN( 2 ).percent() )
      -> set_trigger_spell( sets -> set( HUNTER_MARKSMANSHIP, T20, B4 ) );

  buffs.pre_t20_2p_critical_aimed_damage =
    make_buff( this, "pre_t20_2p_critical_aimed_damage" )
    ->set_max_stack( 2 )
    ->set_quiet( true );

  buffs.t20_2p_critical_aimed_damage =
    make_buff( this, "t20_2p_critical_aimed_damage", find_spell( 242243 ) )
    ->set_default_value( find_spell( 242243 ) -> effectN( 1 ).percent() );

  buffs.t20_4p_bestial_rage =
    make_buff( this, "t20_4p_bestial_rage", find_spell( 246116 ) )
      -> set_default_value( find_spell( 246116 ) -> effectN( 1 ).percent() )
      -> set_trigger_spell( sets -> set( HUNTER_BEAST_MASTERY, T20, B4 ) );

  // Azerite

  buffs.arcane_flurry =
    make_buff( this, "arcane_flurry", find_spell( 273267 ) )
      -> set_default_value( azerite.arcane_flurry.value( 1 ) )
      -> set_trigger_spell( azerite.arcane_flurry );

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

  buffs.unerring_vision_driver =
    make_buff( this, "unerring_vision_driver", find_spell( 274446 ) )
      -> set_quiet( true )
      -> set_tick_callback( [ this ]( buff_t*, int, const timespan_t& ) { buffs.unerring_vision -> trigger(); } )
      -> set_trigger_spell( azerite.unerring_vision );

  buffs.unerring_vision =
    make_buff<stat_buff_t>( this, "unerring_vision", find_spell( 274447 ) )
      -> add_stat( STAT_CRIT_RATING, azerite.unerring_vision.value( 1 ) );

  buffs.up_close_and_personal =
    make_buff( this, "up_close_and_personal", find_spell( 279593 ) )
      -> set_default_value( azerite.up_close_and_personal.spell() -> effectN( 2 ).base_value() )
      -> set_trigger_spell( azerite.up_close_and_personal );
}

// hunter_t::init_special_effects ===========================================

void hunter_t::init_special_effects()
{
  player_t::init_special_effects();

  // Cooldown adjustments

  if ( legendary.wrist -> ok() )
  {
    cooldowns.aspect_of_the_wild -> duration *= 1.0 + legendary.wrist -> effectN( 1 ).percent();
    get_cooldown( "aspect_of_the_eagle" ) -> duration *= 1.0 + legendary.wrist -> effectN( 1 ).percent();
  }

  buffs.sentinels_sight -> set_trigger_spell( legendary.mm_waist );
  buffs.butchers_bone_apron -> set_trigger_spell( legendary.sv_chest );
  buffs.gyroscopic_stabilization -> set_trigger_spell( legendary.mm_gloves );
  buffs.the_mantle_of_command -> set_trigger_spell( legendary.bm_shoulders );
  buffs.parsels_tongue -> set_trigger_spell( legendary.bm_chest );
}

// hunter_t::init_gains =====================================================

void hunter_t::init_gains()
{
  player_t::init_gains();

  gains.barbed_shot          = get_gain( "barbed_shot" );
  gains.aspect_of_the_wild   = get_gain( "aspect_of_the_wild" );
  gains.spitting_cobra       = get_gain( "spitting_cobra" );
  gains.nesingwarys_trapping_treads = get_gain( "nesingwarys_trapping_treads" );
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

  procs.wild_call                    = get_proc( "wild_call" );
  procs.zevrims_hunger               = get_proc( "zevrims_hunger" );
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

  if ( specialization() == HUNTER_MARKSMANSHIP )
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
  return ( true_level >= 100 ) ? "prolonged_power" :
         ( true_level >= 90  ) ? "draenic_agility" :
         ( true_level >= 85  ) ? "virmens_bite":
         "disabled";
}

// hunter_t::default_flask ==================================================

std::string hunter_t::default_flask() const
{
  return ( true_level >  100 ) ? "seventh_demon" :
         ( true_level >= 90  ) ? "greater_draenic_agility_flask" :
         ( true_level >= 85  ) ? "spring_blossoms" :
         ( true_level >= 80  ) ? "winds" :
         "disabled";
}

// hunter_t::default_food ===================================================

std::string hunter_t::default_food() const
{
	std::string lvl100_food =
		(specialization() == HUNTER_SURVIVAL) ? "pickled_eel" : "salty_squid_roll";

	std::string lvl110_food =
		(specialization() == HUNTER_MARKSMANSHIP ||
		(specialization() == HUNTER_BEAST_MASTERY && !talents.stomp->ok())) ? "nightborne_delicacy_platter" :
			(specialization() == HUNTER_BEAST_MASTERY) ? "the_hungry_magister" : "azshari_salad";

	return (true_level >  100) ? lvl110_food :
		(true_level >  90) ? lvl100_food :
		(true_level == 90) ? "sea_mist_rice_noodles" :
		(true_level >= 80) ? "seafood_magnifique_feast" :
		"disabled";
}

// hunter_t::default_rune ===================================================

std::string hunter_t::default_rune() const
{
  return ( true_level >= 110 ) ? "defiled" :
         ( true_level >= 100 ) ? "hyper" :
         "disabled";
}

// hunter_t::init_actions ===================================================

void hunter_t::init_action_list()
{
  const auto weapon_group = main_hand_weapon.group();
  if ( specialization() == HUNTER_SURVIVAL ? weapon_group != WEAPON_2H : weapon_group != WEAPON_RANGED )
  {
    sim -> error( "Player {} does not have a proper weapon at the Main Hand slot.", name() );
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

  // Precombat actions
  precombat -> add_action( this, "Aspect of the Wild" );

  default_list -> add_action( "auto_shot" );
  default_list -> add_action( this, "Counter Shot", "if=equipped.sephuzs_secret&target.debuff.casting.react&cooldown.buff_sephuzs_secret.up&!buff.sephuzs_secret.up" );

  // Item Actions
  default_list -> add_action( "use_items" );

  // Racials
  for ( std::string racial : { "berserking", "blood_fury", "ancestral_call", "fireblood" } )
    default_list -> add_action( racial + ",if=cooldown.bestial_wrath.remains>30" );
  default_list -> add_action( "lights_judgment" );

  // In-combat potion
  default_list -> add_action( "potion,if=buff.bestial_wrath.up&buff.aspect_of_the_wild.up" );

  // Generic APL
  default_list -> add_action( this, "Barbed Shot", "if=pet.cat.buff.frenzy.up&pet.cat.buff.frenzy.remains<=gcd.max" );
  default_list -> add_talent( this, "A Murder of Crows" );
  default_list -> add_talent( this, "Spitting Cobra" );
  default_list -> add_talent( this, "Stampede", "if=buff.bestial_wrath.up|cooldown.bestial_wrath.remains<gcd|target.time_to_die<15" );
  default_list -> add_action( this, "Aspect of the Wild" );
  default_list -> add_action( this, "Bestial Wrath", "if=!buff.bestial_wrath.up" );
  default_list -> add_action( this, "Multi-Shot", "if=spell_targets>2&(pet.cat.buff.beast_cleave.remains<gcd.max|pet.cat.buff.beast_cleave.down)" );
  default_list -> add_talent( this, "Chimaera Shot" );
  default_list -> add_action( this, "Kill Command" );
  default_list -> add_talent( this, "Dire Beast" );
  default_list -> add_action( this, "Barbed Shot", "if=pet.cat.buff.frenzy.down&charges_fractional>1.4|full_recharge_time<gcd.max|target.time_to_die<9" );
  default_list -> add_talent( this, "Barrage" );
  default_list -> add_action( this, "Multi-Shot", "if=spell_targets>1&(pet.cat.buff.beast_cleave.remains<gcd.max|pet.cat.buff.beast_cleave.down)" );
  default_list -> add_action( this, "Cobra Shot", "if=(active_enemies<2|cooldown.kill_command.remains>focus.time_to_max)&(buff.bestial_wrath.up&active_enemies>1|cooldown.kill_command.remains>1+gcd&cooldown.bestial_wrath.remains>focus.time_to_max|focus-cost+focus.regen*(cooldown.kill_command.remains-1)>action.kill_command.cost)" );
}

// Marksman Action List ======================================================================

void hunter_t::apl_mm()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );
  action_priority_list_t* precombat    = get_action_priority_list( "precombat" );

  // Precombat actions
  precombat -> add_talent( this, "Hunter's Mark" );
  precombat -> add_talent( this, "Double Tap", "precast_time=5" );
  precombat -> add_action( this, "Aimed Shot", "if=active_enemies<3" );
  precombat -> add_talent( this, "Explosive Shot", "if=active_enemies>2" );

  default_list -> add_action( "auto_shot" );
  default_list -> add_action( this, "Counter Shot", "if=equipped.sephuzs_secret&target.debuff.casting.react&cooldown.buff_sephuzs_secret.up&!buff.sephuzs_secret.up" );

  // Item Actions
  default_list -> add_action( "use_items" );

  default_list -> add_talent( this, "Hunter's Mark", "if=debuff.hunters_mark.down" );
  default_list -> add_talent( this, "Double Tap", "if=cooldown.rapid_fire.remains<gcd" );

  // Racials
  for ( std::string racial : { "berserking", "blood_fury", "ancestral_call", "fireblood" } )
    default_list -> add_action( racial + ",if=cooldown.trueshot.remains>30" );
  default_list -> add_action( "lights_judgment" );

  // In-combat potion
  default_list -> add_action( "potion,if=(buff.trueshot.react&buff.bloodlust.react)|((consumable.prolonged_power&target.time_to_die<62)|target.time_to_die<31)" );

  // Generic APL
  default_list -> add_action( this, "Trueshot", "if=cooldown.aimed_shot.charges<1" );
  default_list -> add_talent( this, "Barrage", "if=active_enemies>1" );
  default_list -> add_talent( this, "Explosive Shot", "if=active_enemies>1" );
  default_list -> add_action( this, "Multi-Shot", "if=active_enemies>2&buff.precise_shots.up&cooldown.aimed_shot.full_recharge_time<gcd*buff.precise_shots.stack+action.aimed_shot.cast_time" );
  default_list -> add_action( this, "Arcane Shot", "if=active_enemies<3&buff.precise_shots.up&cooldown.aimed_shot.full_recharge_time<gcd*buff.precise_shots.stack+action.aimed_shot.cast_time" );
  default_list -> add_action( this, "Aimed Shot", "if=buff.precise_shots.down&buff.double_tap.down&(active_enemies>2&buff.trick_shots.up|active_enemies<3&full_recharge_time<cast_time+gcd)" );
  default_list -> add_action( this, "Rapid Fire", "if=active_enemies<3|buff.trick_shots.up" );
  default_list -> add_talent( this, "Explosive Shot" );
  default_list -> add_talent( this, "Barrage" );
  default_list -> add_talent( this, "Piercing Shot" );
  default_list -> add_talent( this, "A Murder of Crows" );
  default_list -> add_action( this, "Multi-Shot", "if=active_enemies>2&buff.trick_shots.down" );
  default_list -> add_action( this, "Aimed Shot", "if=buff.precise_shots.down&(focus>70|buff.steady_focus.down)" );
  default_list -> add_action( this, "Multi-Shot", "if=active_enemies>2&(focus>90|buff.precise_shots.up&(focus>70|buff.steady_focus.down&focus>45))" );
  default_list -> add_action( this, "Arcane Shot", "if=active_enemies<3&(focus>70|buff.steady_focus.down&(focus>60|buff.precise_shots.up))" );
  default_list -> add_talent( this, "Serpent Sting", "if=refreshable" );
  default_list -> add_action( this, "Steady Shot" );
}

// Survival Action List ===================================================================

void hunter_t::apl_surv()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );
  action_priority_list_t* precombat    = get_action_priority_list( "precombat" );

  // Precombat actions
  precombat -> add_talent( this, "Steel Trap" );
  precombat -> add_action( this, "Harpoon" );

  default_list -> add_action( "auto_attack" );
  default_list -> add_action( this, "Muzzle", "if=equipped.sephuzs_secret&target.debuff.casting.react&cooldown.buff_sephuzs_secret.up&!buff.sephuzs_secret.up" );

  // Item Actions
  default_list -> add_action( "use_items" );

  // Racials
  for ( std::string racial : { "berserking", "blood_fury", "ancestral_call", "fireblood" } )
    default_list -> add_action( racial + ",if=cooldown.coordinated_assault.remains>30" );
  default_list -> add_action( "lights_judgment" );

  // In-combat potion
  default_list -> add_action( "potion,if=buff.coordinated_assault.up&(buff.berserking.up|buff.blood_fury.up|!race.troll&!race.orc)" );

  // Generic APL
  default_list -> add_action( "variable,name=can_gcd,value=!talent.mongoose_bite.enabled|buff.mongoose_fury.down|(buff.mongoose_fury.remains-(((buff.mongoose_fury.remains*focus.regen+focus)%action.mongoose_bite.cost)*gcd.max)>gcd.max)" );
  default_list -> add_talent( this, "Steel Trap" );
  default_list -> add_talent( this, "A Murder of Crows" );
  default_list -> add_action( this, "Coordinated Assault" );
  default_list -> add_talent( this, "Chakrams", "if=active_enemies>1" );
  default_list -> add_action( this, "Kill Command", "target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max&buff.tip_of_the_spear.stack<3&active_enemies<2" );
  default_list -> add_action( this, "Wildfire Bomb", "if=(focus+cast_regen<focus.max|active_enemies>1)&(dot.wildfire_bomb.refreshable&buff.mongoose_fury.down|full_recharge_time<gcd)" );
  default_list -> add_action( this, "Kill Command", "target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max&buff.tip_of_the_spear.stack<3" );
  default_list -> add_talent( this, "Butchery", "if=(!talent.wildfire_infusion.enabled|full_recharge_time<gcd)&active_enemies>3|(dot.shrapnel_bomb.ticking&dot.internal_bleeding.stack<3)" );
  default_list -> add_action( this, "Serpent Sting", "if=(active_enemies<2&refreshable&(buff.mongoose_fury.down|(variable.can_gcd&!talent.vipers_venom.enabled)))|buff.vipers_venom.up" );
  default_list -> add_action( this, "Carve", "if=active_enemies>2&(active_enemies<6&active_enemies+gcd<cooldown.wildfire_bomb.remains|5+gcd<cooldown.wildfire_bomb.remains)" );
  default_list -> add_action( this, "Harpoon", "if=talent.terms_of_engagement.enabled" );
  default_list -> add_talent( this, "Flanking Strike" );
  default_list -> add_talent( this, "Chakrams" );
  default_list -> add_action( this, "Serpent Sting", "target_if=min:remains,if=refreshable&buff.mongoose_fury.down|buff.vipers_venom.up" );
  default_list -> add_action( this, "Aspect of the Eagle", "if=target.distance>=6" );
  default_list -> add_action( "mongoose_bite_eagle,target_if=min:dot.internal_bleeding.stack,if=buff.mongoose_fury.up|focus>60,if=buff.aspect_of_the_eagle.up" );
  default_list -> add_talent( this, "Mongoose Bite", "target_if=min:dot.internal_bleeding.stack,if=buff.mongoose_fury.up|focus>60" );
  default_list -> add_talent( this, "Butchery" );
  default_list -> add_action( "raptor_strike_eagle,target_if=min:dot.internal_bleeding.stack&buff.aspect_of_the_eagle.up" );
  default_list -> add_action( this, "Raptor Strike", "target_if=min:dot.internal_bleeding.stack" );
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
    make_repeating_event( *sim, timespan_t::from_seconds( 1 ),
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

  if ( buffs.trueshot -> check() )
    h *= 1.0 / ( 1.0 + buffs.trueshot -> check_value() );

  if ( buffs.dire_beast -> check() )
    h *= 1.0 / ( 1.0 + buffs.dire_beast -> check_value() );

  if ( buffs.sephuzs_secret -> check() )
    h *= 1.0 / ( 1.0 + buffs.sephuzs_secret -> check_value() );

  if ( legendary.sephuzs_secret -> ok() )
    h *= 1.0 / ( 1.0 + legendary.sephuzs_secret -> effectN( 3 ).percent() );

  if ( legendary.mm_cloak -> ok() )
    h *= 1.0 / ( 1.0 + legendary.mm_cloak -> effectN( 1 ).percent() );

  if ( sets -> has_set_bonus( HUNTER_SURVIVAL, T20, B2 ) )
    h *= 1.0 / ( 1.0 + sets -> set( HUNTER_SURVIVAL, T20, B2 ) -> effectN( 1 ).percent() );

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

  if ( buffs.trueshot -> check() )
    h *= 1.0 / ( 1.0 + buffs.trueshot -> check_value() );

  if ( buffs.dire_beast -> check() )
    h *= 1.0 / ( 1.0 + buffs.dire_beast -> check_value() );

  if ( buffs.sephuzs_secret -> check() )
    h *= 1.0 / ( 1.0 + buffs.sephuzs_secret -> check_value() );

  if ( legendary.sephuzs_secret -> ok() )
    h *= 1.0 / ( 1.0 + legendary.sephuzs_secret -> effectN( 3 ).percent() );

  if ( legendary.mm_cloak -> ok() )
    h *= 1.0 / ( 1.0 + legendary.mm_cloak -> effectN( 1 ).percent() );

  if ( sets -> has_set_bonus( HUNTER_SURVIVAL, T20, B2 ) )
    h *= 1.0 / ( 1.0 + sets -> set( HUNTER_SURVIVAL, T20, B2 ) -> effectN( 1 ).percent() );

  return h;
}

// hunter_t::composite_player_critical_damage_multiplier ====================

double hunter_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double cdm = player_t::composite_player_critical_damage_multiplier( s );

  cdm *= 1.0 + buffs.t20_2p_critical_aimed_damage -> check_value();

  return cdm;
}

// hunter_t::composite_player_multiplier ====================================

double hunter_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buffs.parsels_tongue -> check() )
    m *= 1.0 + buffs.parsels_tongue -> data().effectN( 1 ).percent() * buffs.parsels_tongue -> check();

  m *= 1.0 + legendary.generic_damage -> effectN( 1 ).percent() * legendary.generic_damage_count;

  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) )
  {
    m *= 1.0 + sets -> set( HUNTER_SURVIVAL, T20, B4 ) -> effectN( 1 ).percent();
    m *= 1.0 + sets -> set( HUNTER_MARKSMANSHIP, T21, B2 ) -> effectN( 1 ).percent();
  }

  m *= 1.0 + sets -> set( HUNTER_SURVIVAL, T21, B2 ) -> effectN( 1 ).percent();

  return m;
}

// composite_player_target_multiplier ====================================

double hunter_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double d = player_t::composite_player_target_multiplier( target, school );

  if ( auto td = find_target_data( target ) )
  {
    d *= 1.0 + td -> debuffs.mark_of_helbrine -> check_value();
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
  m *= 1.0 + buffs.the_mantle_of_command -> check_value();
  m *= 1.0 + buffs.parsels_tongue -> check_stack_value();
  m *= 1.0 + legendary.bm_ring -> effectN( 3 ).percent();
  m *= 1.0 + legendary.generic_damage -> effectN( 2 ).percent() * legendary.generic_damage_count;

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

  add_option( opt_string( "summon_pet", summon_pet_str ) );
  add_option( opt_obsoleted( "hunter_fixed_time" ) );
}

// hunter_t::create_profile =================================================

std::string hunter_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  profile_str += "summon_pet=" + summon_pet_str + "\n";

  return profile_str;
}

// hunter_t::copy_from ======================================================

void hunter_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  hunter_t* p = debug_cast<hunter_t*>( source );

  summon_pet_str = p -> summon_pet_str;
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

using legendary_spell_ptr_t = spell_data_ptr_t hunter_t::legendary_t::*;

void register_legendary_effect( unsigned spell_id, specialization_e spec, legendary_spell_ptr_t ptr )
{
  struct initializer_t : public unique_gear::scoped_actor_callback_t<hunter_t>
  {
    legendary_spell_ptr_t ptr;

    initializer_t( legendary_spell_ptr_t ptr ):
      super( HUNTER ), ptr( ptr )
    {}

    initializer_t( specialization_e spec, legendary_spell_ptr_t ptr ):
      super( spec ), ptr( ptr )
    {}

    void manipulate( hunter_t* p, const special_effect_t& e ) override
    {
      p -> legendary.*ptr = e.driver();
    }
  };

  if ( spec == SPEC_NONE )
    unique_gear::register_special_effect( spell_id, initializer_t( ptr ) );
  else
    unique_gear::register_special_effect( spell_id, initializer_t( spec, ptr ) );
}

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
    register_legendary_effect( 236447, HUNTER_SURVIVAL,      &hunter_t::legendary_t::sv_chest );
    register_legendary_effect( 212574, HUNTER_SURVIVAL,      &hunter_t::legendary_t::sv_feet );
    register_legendary_effect( 281262, HUNTER_SURVIVAL,      &hunter_t::legendary_t::sv_ring );
    register_legendary_effect( 213154, HUNTER_SURVIVAL,      &hunter_t::legendary_t::sv_waist );
    register_legendary_effect( 212278, HUNTER_BEAST_MASTERY, &hunter_t::legendary_t::bm_feet );
    register_legendary_effect( 212329, SPEC_NONE,            &hunter_t::legendary_t::bm_ring );
    register_legendary_effect( 235721, HUNTER_BEAST_MASTERY, &hunter_t::legendary_t::bm_shoulders );
    register_legendary_effect( 207280, HUNTER_BEAST_MASTERY, &hunter_t::legendary_t::bm_waist );
    register_legendary_effect( 248084, HUNTER_BEAST_MASTERY, &hunter_t::legendary_t::bm_chest );
    register_legendary_effect( 206889, HUNTER_MARKSMANSHIP,  &hunter_t::legendary_t::mm_feet );
    register_legendary_effect( 235691, HUNTER_MARKSMANSHIP,  &hunter_t::legendary_t::mm_gloves );
    register_legendary_effect( 208912, HUNTER_MARKSMANSHIP,  &hunter_t::legendary_t::mm_waist );
    register_legendary_effect( 226841, HUNTER_MARKSMANSHIP,  &hunter_t::legendary_t::mm_wrist );
    register_legendary_effect( 281297, SPEC_NONE,            &hunter_t::legendary_t::mm_cloak );
    register_legendary_effect( 206332, SPEC_NONE,            &hunter_t::legendary_t::wrist );
    register_legendary_effect( 208051, SPEC_NONE,            &hunter_t::legendary_t::sephuzs_secret );

    struct generic_damage_t : public unique_gear::scoped_actor_callback_t<hunter_t>
    {
      generic_damage_t() : super( HUNTER ) {}

      void manipulate( hunter_t* p, const special_effect_t& e ) override
      {
        p -> legendary.generic_damage = e.driver();
        p -> legendary.generic_damage_count++;
      }
    };
    unique_gear::register_special_effect( 280737, generic_damage_t{} );
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
