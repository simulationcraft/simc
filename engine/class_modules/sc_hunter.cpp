//==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <memory>
#include <optional>

#include "simulationcraft.hpp"
#include "player/pet_spawner.hpp"
#include "class_modules/apl/apl_hunter.hpp"

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

  bool ok() const { return data_ -> ok(); }

  const spell_data_t* data_;
};

static void print_affected_by( const action_t* a, const spelleffect_data_t& effect, util::string_view label = {} )
{
  fmt::memory_buffer out;
  const spell_data_t& spell = *effect.spell();
  const auto& spell_text = a->player->dbc->spell_text( spell.id() );

  fmt::format_to( std::back_inserter(out), "{} {} is affected by {}", *a->player, *a, spell.name_cstr() );
  if ( spell_text.rank() )
    fmt::format_to( std::back_inserter(out), " (desc={})", spell_text.rank() );
  fmt::format_to( std::back_inserter(out), " (id={}) effect#{}", spell.id(), effect.spell_effect_num() + 1 );
  if ( !label.empty() )
    fmt::format_to( std::back_inserter(out), ": {}", label );

  a -> sim -> print_debug( "{}", util::string_view( out.data(), out.size() ) );
}

static bool check_affected_by( action_t* a, const spelleffect_data_t& effect )
{
  bool affected = a->data().affected_by( effect ) || a->data().affected_by_label( effect );
  if ( affected && a->sim->debug )
    print_affected_by( a, effect );
  return affected;
}

struct damage_affected_by {
  uint8_t direct = 0;
  uint8_t tick = 0;
};

static damage_affected_by parse_damage_affecting_aura( action_t* a, spell_data_ptr_t spell )
{
  damage_affected_by affected_by;
  for ( const spelleffect_data_t& effect : spell -> effects() )
  {
    if ( effect.type() != E_APPLY_AURA )
      continue;

    if ( ( effect.subtype() == A_MOD_DAMAGE_FROM_CASTER_SPELLS && a->data().affected_by( effect ) ) ||
         ( effect.subtype() == A_MOD_DAMAGE_FROM_CASTER_SPELLS_LABEL && a->data().affected_by_label( effect ) ) )
    {
      affected_by.direct = as<uint8_t>( effect.spell_effect_num() + 1 );
      affected_by.tick   = as<uint8_t>( effect.spell_effect_num() + 1 );
      print_affected_by( a, effect, "spell damage taken increase" );

      return affected_by;
    }

    if ( ( effect.subtype() == A_ADD_PCT_MODIFIER && a->data().affected_by( effect ) ) ||
         ( effect.subtype() == A_ADD_PCT_LABEL_MODIFIER && a->data().affected_by_label( effect ) ) )
    {
      if ( effect.misc_value1() == P_GENERIC )
      {
        affected_by.direct = as<uint8_t>( effect.spell_effect_num() + 1 );
        print_affected_by( a, effect, "direct damage increase" );
      }
      else if ( effect.misc_value1() == P_TICK_DAMAGE )
      {
        affected_by.tick = as<uint8_t>( effect.spell_effect_num() + 1 );
        print_affected_by( a, effect, "tick damage increase" );
      }
    }
  }
  return affected_by;
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
  using record_t = std::pair<std::string, std::unique_ptr<action_data_t>>;
  std::vector<record_t> data_;

  action_data_t* get( const action_t* a )
  {
    auto it = range::find( data_, a -> name_str, &record_t::first );
    if ( it != data_.cend() )
      return it -> second.get();

    data_.emplace_back( a -> name_str, std::make_unique<action_data_t>( ) );
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

  os << "<h3 class='toggle open'>Cooldown waste details</h3>\n"
     << "<div class='toggle-content'>\n";

  os << "<table class='sc' style='float: left;margin-right: 10px;'>\n"
     << "<tr>"
     << "<th></th>"
     << "<th colspan='3'>Seconds per Execute</th>"
     << "<th colspan='3'>Seconds per Iteration</th>"
     << "</tr>\n"
     << "<tr>"
     << "<th>Ability</th>"
     << "<th>Average</th><th>Minimum</th><th>Maximum</th>"
     << "<th>Average</th><th>Minimum</th><th>Maximum</th>"
     << "</tr>\n";

  size_t n = 0;
  for ( const auto& rec : data.data_ )
  {
    const auto& entry = rec.second -> exec;
    if ( entry.count() == 0 )
      continue;

    const auto& iter_entry = rec.second -> cumulative;
    const action_t* a = player.find_action( rec.first );

    ++n;
    fmt::print( os,
      "<tr{}>"
      "<td class='left'>{}</td>"
      "<td class='right'>{:.3f}</td><td class='right'>{:.3f}</td><td class='right'>{:.3f}</td>"
      "<td class='right'>{:.3f}</td><td class='right'>{:.3f}</td><td class='right'>{:.3f}</td>"
      "</tr>\n",
      n & 1 ? " class='odd'" : "",
      a ? report_decorators::decorated_action( *a ) : util::encode_html( rec.first ),
      entry.mean(), entry.min(), entry.max(),
      iter_entry.mean(), iter_entry.min(), iter_entry.max()
    );
  }

  os << "</table>\n"
     << "</div>\n"
     << "<div class='clear'></div>\n";
}

} // end namespace cd_waste

// ==========================================================================
// Hunter
// ==========================================================================

enum howl_of_the_pack_leader_beast
{
  WYVERN,
  BOAR,
  BEAR
};

struct maybe_bool {

  enum class value_e : uint8_t {
    None, True, False
  };

  constexpr maybe_bool() = default;

  constexpr maybe_bool& operator=( bool val ) {
    set( val );
    return *this;
  }

  constexpr void set( bool val ) {
    value_ = val ? value_e::True : value_e::False;
  }

  constexpr bool is_none() const { return value_ == value_e::None; }

  constexpr operator bool() const { return value_ == value_e::True; }

  value_e value_ = value_e::None;
};

template <typename Data, typename Base = action_state_t>
struct hunter_action_state_t : public Base, public Data
{
  static_assert( std::is_base_of_v<action_state_t, Base> );
  static_assert( std::is_default_constructible_v<Data> ); // required for initialize
  static_assert( std::is_copy_assignable_v<Data> ); // required for copy_state

  using Base::Base;

  void initialize() override
  {
    Base::initialize();
    *static_cast<Data*>( this ) = Data{};
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    Base::debug_str( s );
    if constexpr ( fmt::is_formattable<Data>::value )
      fmt::print( s, " {}", *static_cast<const Data*>( this ) );
    return s;
  }

  void copy_state( const action_state_t* o ) override
  {
    Base::copy_state( o );
    *static_cast<Data*>( this ) = *static_cast<const Data*>( debug_cast<const hunter_action_state_t*>( o ) );
  }
};

struct pet_amount_expr_t : public expr_t
{
public:
  action_t& action;
  action_t& pet_action;
  action_state_t* state;

  pet_amount_expr_t( util::string_view name, action_t& a, action_t& pet_a )
    : expr_t( name ), action( a ), pet_action( pet_a ), state( pet_a.get_state() )
  {
    state->n_targets = 1;
    state->chain_target = 0;
    state->result = RESULT_HIT;
  }

  double evaluate() override
  {
    state->target = action.target;
    pet_action.snapshot_state( state, result_amount_type::DMG_DIRECT );

    state->result_amount = pet_action.calculate_direct_amount( state );
    state->target->target_mitigation( action.get_school(), result_amount_type::DMG_DIRECT, state );

    return state->result_amount;
  }

  ~pet_amount_expr_t() override
  {
    delete state;
  }
};

struct hunter_t;

namespace pets
{
struct natures_ally_pet_t;
struct dire_critter_t;
struct dire_beast_t;
struct dark_hound_t;
struct dark_minion_t;
struct fenryr_t;
struct hati_t;
struct bear_t;
struct stable_pet_t;
struct hunter_main_pet_base_t;
struct animal_companion_t;
struct hunter_main_pet_t;
}

namespace events
{
struct tar_trap_aoe_t;
}

struct hunter_td_t: public actor_target_data_t
{
  bool damaged = false;

  struct debuffs_t
  {
    buff_t* outland_venom;

    buff_t* spotters_mark;

    buff_t* sentinels_mark;

    buff_t* headshot;
  } debuffs;

  struct dots_t
  {
    dot_t* explosive_shot;
    
    dot_t* barbed_shot;
    dot_t* laceration;

    dot_t* wildfire_bomb;

    dot_t* sanctified_armaments;

    dot_t* black_arrow;
  } dots;

  hunter_td_t( player_t* target, hunter_t* p );

  void target_demise();
};

struct hunter_t final : public player_t
{
public:

  struct pets_t
  {
    pets::hunter_main_pet_t* main = nullptr;
    pets::animal_companion_t* animal_companion = nullptr;
    spawner::pet_spawner_t<pets::natures_ally_pet_t, hunter_t> natures_ally_pet;
    spawner::pet_spawner_t<pets::dire_beast_t, hunter_t> dire_beast;
    spawner::pet_spawner_t<pets::dark_hound_t, hunter_t> dark_hound;
    spawner::pet_spawner_t<pets::dark_minion_t, hunter_t> dark_minion;
    spawner::pet_spawner_t<pets::fenryr_t, hunter_t> fenryr;
    spawner::pet_spawner_t<pets::hati_t, hunter_t> hati;
    spawner::pet_spawner_t<pets::bear_t, hunter_t> bear;

    pets_t( hunter_t* p ) : 
      natures_ally_pet( "natures_ally_pet", p ),
      dire_beast( "dire_beast", p ),
      dark_hound( "dark_hound", p ),
      dark_minion( "dark_minion", p ),
      fenryr( "fenryr", p ),
      hati( "hati", p ),
      bear( "bear", p )
    {
    }
  } pets;

  struct proc_data_entries_t
  {
    proc_data_t lethal_barbs_energize;
  } proc_data_entries;

  struct tier_sets_t
  {
    // Midnight Season 1 - Whatever the raid is called
    spell_data_ptr_t mid_s1_bm_2pc;
    spell_data_ptr_t mid_s1_bm_4pc;

    spell_data_ptr_t mid_s1_mm_2pc;
    spell_data_ptr_t mid_s1_mm_4pc;
    spell_data_ptr_t mid_s1_mm_4pc_damage;

    spell_data_ptr_t mid_s1_sv_2pc;
    spell_data_ptr_t mid_s1_sv_4pc;
  } tier_set;

  struct buffs_t
  {
    // Hunter Tree
    buff_t* deathblow;

    // Marksmanship Tree
    buff_t* precise_shots;
    buff_t* trick_shots;
    buff_t* lock_and_load;
    buff_t* trueshot;
    buff_t* bullseye;
    buff_t* bulletstorm;
    buff_t* volley;
    buff_t* double_tap;
    buff_t* focus_fire;
    buff_t* precision_detonation_hidden;

    // Beast Mastery Tree
    buff_t* barbed_shot;
    buff_t* bestial_wrath;
    buff_t* beast_cleave; 
    buff_t* huntmasters_call;
    buff_t* summon_fenryr;
    buff_t* summon_hati;
    buff_t* heart_of_the_pack;
    buff_t* natures_ally_3;
    buff_t* bloody_frenzy;

    // Survival Tree
    buff_t* tip_of_the_spear;
    buff_t* tip_of_the_spear_boomstick;
    buff_t* tip_of_the_spear_chakram;
    buff_t* mongoose_fury;
    buff_t* bloodseeker;
    buff_t* aspect_of_the_eagle;
    buff_t* wallop;
    buff_t* takedown;
    buff_t* wildfire_imbuement;
    buff_t* raptor_swipe;
    buff_t* shrapnel_bomb;

    // Pet family buffs
    buff_t* endurance_training;
    buff_t* pathfinding;
    buff_t* predators_thirst;

    // Tier Set Bonuses

    // Hero Talents 

    // Pack Leader
    buff_t* howl_of_the_pack_leader_wyvern;
    buff_t* howl_of_the_pack_leader_boar;
    buff_t* howl_of_the_pack_leader_bear;
    buff_t* howl_of_the_pack_leader_cooldown;
    buff_t* wyverns_cry;
    buff_t* hogstrider;
    buff_t* stampede;
    buff_t* stampede_incoming;

    // Sentinel
    buff_t* stargazer;
    buff_t* moonlight_chakram;

    // Dark Ranger
    buff_t* withering_fire;
    buff_t* wailing_arrow;
  } buffs;

  struct cooldowns_t
  {
    cooldown_t* kill_shot;
    cooldown_t* explosive_shot;
    
    cooldown_t* aimed_shot;
    cooldown_t* rapid_fire;
    cooldown_t* trueshot;
    cooldown_t* volley;
    cooldown_t* salvo;
    cooldown_t* shrapnel_shot;
    
    cooldown_t* dire_beast;
    cooldown_t* kill_command;
    cooldown_t* wild_thrash;

    cooldown_t* barbed_shot;
    cooldown_t* bestial_wrath;

    cooldown_t* wildfire_bomb;
    cooldown_t* harpoon;
    cooldown_t* boomstick;
    cooldown_t* strike_as_one;
    cooldown_t* takedown;
    cooldown_t* flamefang_pitch;

    cooldown_t* black_arrow;
    cooldown_t* bleak_powder;
  } cooldowns;

  struct gains_t
  {
    gain_t* barbed_shot;
    gain_t* pack_tactics;
    gain_t* invigorating_pulse;
    gain_t* serpentine_strikes;
    gain_t* lethal_barbs;
    gain_t* shrapnel_bomb;
    gain_t* disruptive_rounds;
  } gains;

  struct procs_t
  {
    proc_t* snakeskin_quiver;
    proc_t* dire_command;

    proc_t* deathblow;

    proc_t* windrunner_quiver;
    proc_t* precision_detonation;
    proc_t* eagles_mark;

    proc_t* dire_beast_spawn;
    proc_t* dark_minion_spawn;
    proc_t* dark_hound_spawn;
  } procs;

  struct rppm_t
  {
    real_ppm_t* corpsecaller;
    real_ppm_t* shadow_surge;

    real_ppm_t* let_fly;
  } rppm;

  struct accumulated_rngs_t
  {
    accumulated_rng_t* dire_command;
  } accumulated_rng;

  struct talents_t
  {
    // Hunter Tree
    spell_data_ptr_t rejuvenating_winds; //Utility talent, won't implement
    spell_data_ptr_t survival_of_the_fittest; //Utility talent, won't implement
    spell_data_ptr_t posthaste; //Utility talent, won't implement

    spell_data_ptr_t natural_mending; //Utility talent, won't implement
    spell_data_ptr_t padded_armor; //Utility talent, won't implement
    spell_data_ptr_t hunters_avoidance; //Utility talent, won't implement

    spell_data_ptr_t wilderness_medicine; //Utility talent, won't implement
    spell_data_ptr_t combat_experience;
    spell_data_ptr_t improved_aspect_of_the_cheetah; //Utility talent, won't implement
    spell_data_ptr_t concussive_shot; //Not implemented - probably not needed

    spell_data_ptr_t precision_strikes;
    spell_data_ptr_t counter_shot;
    spell_data_ptr_t muzzle;
    spell_data_ptr_t serrated_tips;

    spell_data_ptr_t tranquilizing_shot; //Not implemented - probably not needed
    spell_data_ptr_t pathfinding; //Utility talent, won't implement
    spell_data_ptr_t disruptive_rounds;
    spell_data_ptr_t improved_feign_death; //Utility talent, won't implement
    spell_data_ptr_t misdirection; //Utility talent, won't implement

    spell_data_ptr_t kodo_tranquilizer; //Utility talent, won't implement
    spell_data_ptr_t devilsaur_tranquilizer; //Utility talent, won't implement
    spell_data_ptr_t kindling_flare; //Utility talent, won't implement
    spell_data_ptr_t trigger_finger;
    spell_data_ptr_t tar_trap;
    spell_data_ptr_t scare_beast; //Utility talent, won't implement
    spell_data_ptr_t touch_of_grass; //Utility talent, won't implement
    spell_data_ptr_t camouflage; //Utility talent, won't implement
    spell_data_ptr_t no_hard_feelings; //Utility talent, won't implement

    spell_data_ptr_t improved_aspect_of_the_turtle; //Utility talent, won't implement
    spell_data_ptr_t specialized_arsenal;
    spell_data_ptr_t scouts_instincts; //Utility talent, won't implement

    spell_data_ptr_t shell_wall; //Utility talent, won't implement
    spell_data_ptr_t intimidation; //Utility talent, won't implement
    spell_data_ptr_t improved_snaring; //Utility talent, won't implement
    spell_data_ptr_t lone_survivor; //Utility talent, won't implement
    spell_data_ptr_t catlike_reflexes; //Utility talent, won't implement
    spell_data_ptr_t binding_shot; //Utility talent, won't implement
    spell_data_ptr_t trailblazer; //Utility talent, won't implement
    spell_data_ptr_t moment_of_opportunity; //Utility talent, won't implement

    spell_data_ptr_t cold_feet; //Utility talent, won't implement
    spell_data_ptr_t territorial_instincts; //Utility talent, won't implement
    spell_data_ptr_t guttural_roar; //Utility talent, won't implement
    spell_data_ptr_t born_to_be_wild; //Utility talent, won't implement
    spell_data_ptr_t keen_eyesight;
    spell_data_ptr_t tar_coated_bindings; //Utility talent, won't implement
    spell_data_ptr_t horsehair_tether; //Utility talent, won't implement
    spell_data_ptr_t improved_traps; //Utility talent, won't implement

    spell_data_ptr_t emergency_salve; //Utility talent, won't implement
    spell_data_ptr_t roar_of_sacrifice; //Utility talent, won't implement
    spell_data_ptr_t guardians_hide; //Utility talent, won't implement
    spell_data_ptr_t unnatural_causes;
    spell_data_ptr_t unnatural_causes_debuff;

    spell_data_ptr_t deathblow_buff;
    
    // Beast Mastery Tree
    spell_data_ptr_t kill_command_bm_player;
    spell_data_ptr_t kill_command_bm_pet;

    spell_data_ptr_t animal_companion;
    spell_data_ptr_t solitary_companion;
    spell_data_ptr_t barbed_shot;
    spell_data_ptr_t barbed_shot_buff;

    spell_data_ptr_t alpha_predator;
    spell_data_ptr_t dire_beast;
    spell_data_ptr_t stomp;
    spell_data_ptr_t stomp_dmg;
    spell_data_ptr_t war_orders;

    spell_data_ptr_t wild_thrash_player;
    spell_data_ptr_t wild_thrash_pet;
    spell_data_ptr_t bestial_wrath;
    spell_data_ptr_t cobra_shot;
    spell_data_ptr_t cobra_shot_data;

    spell_data_ptr_t beast_cleave;
    spell_data_ptr_t scent_of_blood;
    spell_data_ptr_t thundering_hooves;
    spell_data_ptr_t go_for_the_throat;

    spell_data_ptr_t laceration;
    spell_data_ptr_t laceration_driver;
    spell_data_ptr_t laceration_bleed;
    spell_data_ptr_t kill_cleave;
    spell_data_ptr_t training_expert;
    spell_data_ptr_t the_beast_within;
    spell_data_ptr_t thrill_of_the_hunt;
    spell_data_ptr_t pack_tactics;
    spell_data_ptr_t pack_tactics_energize;
    spell_data_ptr_t barbed_scales;

    spell_data_ptr_t aspect_of_the_beast;
    spell_data_ptr_t dire_cleave;
    spell_data_ptr_t dire_command;
    spell_data_ptr_t jagged_wounds;
    spell_data_ptr_t serpentine_strikes;
    spell_data_ptr_t serpentine_strikes_energize;
    spell_data_ptr_t snakeskin_quiver;
    spell_data_ptr_t cobra_senses;

    spell_data_ptr_t dire_frenzy;
    spell_data_ptr_t frenzy;
    spell_data_ptr_t killer_instinct;

    spell_data_ptr_t brutal_companion;
    spell_data_ptr_t huntmasters_call;
    spell_data_ptr_t heart_of_the_pack;
    spell_data_ptr_t heart_of_the_pack_buff;
    spell_data_ptr_t bloodshed;
    spell_data_ptr_t bloodshed_dot;
    spell_data_ptr_t savagery_bm;
    spell_data_ptr_t killer_cobra;
    spell_data_ptr_t master_handler;

    spell_data_ptr_t wildspeaker;
    spell_data_ptr_t wildspeaker_kill_command;
    spell_data_ptr_t wildspeaker_bestial_wrath;
    spell_data_ptr_t wild_instincts;
    spell_data_ptr_t bloody_frenzy;
    spell_data_ptr_t bloody_frenzy_buff;
    spell_data_ptr_t piercing_fangs;

    spell_data_ptr_t natures_ally_1;
    spell_data_ptr_t natures_ally_1_summon;
    spell_data_ptr_t natures_ally_2;
    spell_data_ptr_t natures_ally_3;
    spell_data_ptr_t natures_ally_3_buff;

    // Marksmanship Tree
    spell_data_ptr_t aimed_shot;

    spell_data_ptr_t rapid_fire;
    spell_data_ptr_t rapid_fire_tick;
    spell_data_ptr_t rapid_fire_energize;
    spell_data_ptr_t precise_shots;
    spell_data_ptr_t precise_shots_buff;

    spell_data_ptr_t quick_draw; //TODO implement move speed buff?
    spell_data_ptr_t lock_and_load; // TODO how does its blp work
    spell_data_ptr_t lock_and_load_buff;

    spell_data_ptr_t surging_shots;
    spell_data_ptr_t avian_specialization;
    spell_data_ptr_t unbreakable_bond;
    spell_data_ptr_t trick_shots;
    spell_data_ptr_t trick_shots_data;
    spell_data_ptr_t trick_shots_buff;
    spell_data_ptr_t aspect_of_the_hydra;

    spell_data_ptr_t penetrating_shots;
    spell_data_ptr_t tenacious; //Utility talent, won't implement
    spell_data_ptr_t cunning; //Utility talent, won't implement
    spell_data_ptr_t master_marksman;
    spell_data_ptr_t master_marksman_bleed;
    spell_data_ptr_t light_ammo;

    spell_data_ptr_t obsidian_arrowhead;
    spell_data_ptr_t on_target;
    spell_data_ptr_t trueshot;
    spell_data_ptr_t kill_shot;

    spell_data_ptr_t explosive_shot;
    spell_data_ptr_t explosive_shot_damage;

    spell_data_ptr_t precision_detonation;
    spell_data_ptr_t precision_detonation_buff;

    spell_data_ptr_t critical_precision;
    spell_data_ptr_t no_scope;
    spell_data_ptr_t feathered_frenzy;
    spell_data_ptr_t headshot;
    spell_data_ptr_t headshot_debuff;
    spell_data_ptr_t deadeye;
    spell_data_ptr_t deathblow;

    spell_data_ptr_t take_aim_1;
    spell_data_ptr_t unmatched_precision;
    spell_data_ptr_t bullseye;
    spell_data_ptr_t bullseye_buff;
    spell_data_ptr_t calling_the_shots;
    spell_data_ptr_t unerring_vision; 
    spell_data_ptr_t small_game_hunter;
    spell_data_ptr_t eagles_accuracy;

    spell_data_ptr_t take_aim_2;
    spell_data_ptr_t focused_aim;
    spell_data_ptr_t bulletstorm;
    spell_data_ptr_t bulletstorm_buff;
    spell_data_ptr_t tensile_bowstring;
    spell_data_ptr_t volley;
    spell_data_ptr_t volley_data;
    spell_data_ptr_t volley_dmg;
    spell_data_ptr_t focus_fire;
    spell_data_ptr_t focus_fire_buff;

    spell_data_ptr_t take_aim_3;
    spell_data_ptr_t windrunner_quiver;
    spell_data_ptr_t incendiary_ammunition;
    spell_data_ptr_t double_tap;
    spell_data_ptr_t double_tap_buff;
    spell_data_ptr_t salvo;
    spell_data_ptr_t shrapnel_shot;
    spell_data_ptr_t unload;

    // Survival Tree
    spell_data_ptr_t kill_command_sv_player;
    spell_data_ptr_t kill_command_sv_pet;

    spell_data_ptr_t wildfire_bomb;
    spell_data_ptr_t wildfire_bomb_data;
    spell_data_ptr_t wildfire_bomb_dmg;
    spell_data_ptr_t wildfire_bomb_dot;
    spell_data_ptr_t raptor_strike;
    spell_data_ptr_t raptor_strike_eagle;

    spell_data_ptr_t raptor_swipe_1;
    spell_data_ptr_t raptor_swipe_2;
    spell_data_ptr_t raptor_swipe_3;
    spell_data_ptr_t raptor_swipe_spell;
    spell_data_ptr_t raptor_swipe_buff;

    spell_data_ptr_t guerrilla_tactics;
    spell_data_ptr_t tip_of_the_spear;
    spell_data_ptr_t tip_of_the_spear_buff;
    spell_data_ptr_t tip_of_the_spear_boomstick_buff;
    spell_data_ptr_t tip_of_the_spear_chakram_buff;

    spell_data_ptr_t lunge;
    spell_data_ptr_t boomstick;
    spell_data_ptr_t strike_as_one;
    spell_data_ptr_t strike_as_one_dmg;

    spell_data_ptr_t shrapnel_bomb;
    spell_data_ptr_t shrapnel_bomb_bleed;
    spell_data_ptr_t shrapnel_bomb_buff;
    spell_data_ptr_t flamebreak;
    spell_data_ptr_t bloodseeker;
    spell_data_ptr_t quick_reload;
    spell_data_ptr_t flankers_advantage;
    spell_data_ptr_t two_against_many;

    spell_data_ptr_t mongoose_fury;
    spell_data_ptr_t mongoose_fury_buff;
    spell_data_ptr_t mongoose_rounds;
    spell_data_ptr_t wildfire_shells;
    spell_data_ptr_t shellshock;
    spell_data_ptr_t sic_em;
    spell_data_ptr_t sic_em_bleed;

    spell_data_ptr_t bloody_claws;
    spell_data_ptr_t wallop;
    spell_data_ptr_t wallop_buff;
    spell_data_ptr_t improved_wildfire_bomb;
    spell_data_ptr_t bonding;
    spell_data_ptr_t sweeping_spear;
    spell_data_ptr_t vulnerability;
    spell_data_ptr_t blackrock_munitions;
    spell_data_ptr_t shower_of_blood;
    spell_data_ptr_t outland_venom;
    spell_data_ptr_t outland_venom_debuff;

    spell_data_ptr_t explosives_expert;
    spell_data_ptr_t takedown;
    spell_data_ptr_t takedown_energize;
    spell_data_ptr_t takedown_dmg;
    spell_data_ptr_t takedown_pet;
    spell_data_ptr_t killer_companion;

    spell_data_ptr_t flamefang_pitch;
    spell_data_ptr_t flamefang_pitch_data;
    spell_data_ptr_t flamefang_pitch_dmg;
    spell_data_ptr_t flamefang_pitch_aoe;
    spell_data_ptr_t twin_fangs;
    spell_data_ptr_t savagery_sv;
    spell_data_ptr_t wildfire_infusion;

    spell_data_ptr_t grenade_juggler;
    spell_data_ptr_t wildfire_imbuement;
    spell_data_ptr_t wildfire_imbuement_dmg;
    spell_data_ptr_t wildfire_imbuement_buff;
    spell_data_ptr_t flanked;
    spell_data_ptr_t lethal_calibration;
    spell_data_ptr_t primal_surge;

    // Dark Ranger
    spell_data_ptr_t black_arrow;
    spell_data_ptr_t black_arrow_spell;
    spell_data_ptr_t black_arrow_dot;
    
    spell_data_ptr_t bleak_arrows;
    spell_data_ptr_t bleak_arrows_spell;
    spell_data_ptr_t soul_drinker;
    spell_data_ptr_t bleak_powder;
    spell_data_ptr_t bleak_powder_spell;
    spell_data_ptr_t corpsecaller;
    spell_data_ptr_t corpsecaller_minion_summon;
    spell_data_ptr_t corpsecaller_hound_summon;

    spell_data_ptr_t ebon_bowstring;
    spell_data_ptr_t through_the_eyes;
    spell_data_ptr_t smoke_screen; //Utility talent, won't implement
    spell_data_ptr_t dark_chains; //Utility talent, won't implement
    spell_data_ptr_t shadow_dagger; //Utility talent, won't implement
    spell_data_ptr_t wailing_dead;
    spell_data_ptr_t wailing_arrow;
    spell_data_ptr_t wailing_arrow_buff;
    spell_data_ptr_t wailing_arrow_damage;

    spell_data_ptr_t blighted_quiver;
    spell_data_ptr_t banshees_mark;
    spell_data_ptr_t the_bell_tolls;
    spell_data_ptr_t umbral_reach;
    spell_data_ptr_t pact_of_the_hollow;

    spell_data_ptr_t withering_fire;
    spell_data_ptr_t withering_fire_black_arrow;
    spell_data_ptr_t withering_fire_buff;

    // Pack Leader
    spell_data_ptr_t howl_of_the_pack_leader;
    spell_data_ptr_t howl_of_the_pack_leader_wyvern_ready_buff;
    spell_data_ptr_t howl_of_the_pack_leader_boar_ready_buff;
    spell_data_ptr_t howl_of_the_pack_leader_bear_ready_buff;
    spell_data_ptr_t howl_of_the_pack_leader_cooldown_buff;
    spell_data_ptr_t howl_of_the_pack_leader_wyvern_summon;
    spell_data_ptr_t howl_of_the_pack_leader_wyvern_buff;
    spell_data_ptr_t howl_of_the_pack_leader_boar_charge_trigger;
    spell_data_ptr_t howl_of_the_pack_leader_boar_charge_impact;
    spell_data_ptr_t howl_of_the_pack_leader_boar_charge_cleave;
    spell_data_ptr_t howl_of_the_pack_leader_bear_summon;
    spell_data_ptr_t howl_of_the_pack_leader_bear_buff;
    spell_data_ptr_t howl_of_the_pack_leader_bear_bleed;

    spell_data_ptr_t pack_mentality;
    spell_data_ptr_t dire_summons;
    spell_data_ptr_t better_together;
    spell_data_ptr_t slicked_shoes; //Utility talent, won't implement
    spell_data_ptr_t masterful_call; //Utility talent, won't implement

    spell_data_ptr_t ursine_fury;
    spell_data_ptr_t dire_beast_summon;
    spell_data_ptr_t sharpened_claws;
    spell_data_ptr_t fury_of_the_wyvern;
    spell_data_ptr_t hogstrider;
    spell_data_ptr_t hogstrider_buff;
    spell_data_ptr_t lethal_barbs;
    spell_data_ptr_t lethal_barbs_energize;

    spell_data_ptr_t no_mercy;
    spell_data_ptr_t shell_cover; //Utility talent, won't implement
    spell_data_ptr_t hoof_and_blade;
    spell_data_ptr_t wyverns_gaze;
    spell_data_ptr_t sharpened_fangs;
    
    spell_data_ptr_t stampede;
    spell_data_ptr_t stampede_incoming_buff;
    spell_data_ptr_t stampede_trigger;
    spell_data_ptr_t stampede_dmg;
    
    // Sentinel
    spell_data_ptr_t sentinel;
    spell_data_ptr_t sentinels_mark;

    spell_data_ptr_t dont_look_back; //Utility talent, won't implement
    spell_data_ptr_t moons_blessing;
    spell_data_ptr_t sanctified_armaments;
    spell_data_ptr_t sanctified_armaments_dot;
    spell_data_ptr_t moonlight_chakram;
    spell_data_ptr_t moonlight_chakram_spell;
    spell_data_ptr_t moonlight_chakram_damage;
    spell_data_ptr_t moonlight_chakram_buff;

    spell_data_ptr_t stargazer;
    spell_data_ptr_t stargazer_buff;
    spell_data_ptr_t open_fire;
    spell_data_ptr_t cant_miss_wont_miss;
    spell_data_ptr_t invigorating_pulse;
    spell_data_ptr_t twilight_requiem;
    spell_data_ptr_t twilight_requiem_damage;
    spell_data_ptr_t stalk_and_strike;

    spell_data_ptr_t arcane_talons;
    spell_data_ptr_t lunar_calling;
    spell_data_ptr_t conditioning; //Utility talent, won't implement
    spell_data_ptr_t scouts_vigil; //Utility talent, won't implement
    spell_data_ptr_t radiant_edge;

    spell_data_ptr_t lunar_storm;
    spell_data_ptr_t lunar_storm_dmg;
  } talents;

  // Specialization Spells
  struct specs_t
  {
    spell_data_ptr_t pet_damage; // 2026-02-03: Generic "Pet Damage" buff, used as a tuning knob for Dire Beasts
    spell_data_ptr_t hunter;
    spell_data_ptr_t beast_mastery_hunter;
    spell_data_ptr_t marksmanship_hunter;
    spell_data_ptr_t survival_hunter;

    spell_data_ptr_t auto_shot;
    spell_data_ptr_t freezing_trap;
    spell_data_ptr_t arcane_shot;
    spell_data_ptr_t steady_shot;
    spell_data_ptr_t steady_shot_energize;
    spell_data_ptr_t flare;
    spell_data_ptr_t call_pet;

    // SV
    spell_data_ptr_t aspect_of_the_eagle;
    spell_data_ptr_t harpoon;
    spell_data_ptr_t hatchet_toss;

    // MM
    spell_data_ptr_t multishot;
    spell_data_ptr_t spotters_mark_data;
    spell_data_ptr_t spotters_mark_debuff;
  } specs;

  struct mastery_spells_t
  {
    spell_data_ptr_t master_of_beasts; // BM
    spell_data_ptr_t sniper_training; // MM
    spell_data_ptr_t spirit_bond; // SV
    spell_data_ptr_t spirit_bond_buff;
  } mastery;

  struct {
    action_t* barbed_shot = nullptr;
    action_t* snakeskin_quiver = nullptr;
    action_t* laceration = nullptr;

    action_t* boar_charge = nullptr;

    action_t* lunar_storm = nullptr;

    action_t* stampede = nullptr;
    action_t* wild_instincts = nullptr;

    action_t* let_fly = nullptr;
  } actions;

  cdwaste::player_data_t cd_waste;

  struct {
    events::tar_trap_aoe_t* tar_trap_aoe = nullptr;
    event_t* current_volley = nullptr;
    event_t* precision_detonation_expiry = nullptr;
    howl_of_the_pack_leader_beast howl_of_the_pack_leader_next_beast = WYVERN;
    timespan_t fury_of_the_wyvern_extension = 0_s;
    bool fury_of_the_wyvern_extendable = false;
  } state;

  struct options_t {
    std::string summon_pet_str = "duck";
    timespan_t pet_attack_speed = 2_s;
    timespan_t pet_basic_attack_delay = 0.15_s;
    bool max_prio_damage = true;
  } options;

  hunter_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) :
    player_t( sim, HUNTER, name, r ),
    pets( this ),
    buffs(),
    cooldowns(),
    gains(),
    procs()
  {
    cooldowns.kill_shot       = get_cooldown( "kill_shot" );
    cooldowns.explosive_shot  = get_cooldown( "explosive_shot" );

    cooldowns.aimed_shot                = get_cooldown( "aimed_shot" );
    cooldowns.rapid_fire                = get_cooldown( "rapid_fire" );
    cooldowns.trueshot                  = get_cooldown( "trueshot" );
    cooldowns.volley                    = get_cooldown( "volley" );
    cooldowns.salvo                     = get_cooldown( "salvo_icd" );
    cooldowns.shrapnel_shot             = get_cooldown( "shrapnel_shot" );
    
    cooldowns.kill_command  = get_cooldown( "kill_command" );
    cooldowns.barbed_shot   = get_cooldown( "barbed_shot" );
    cooldowns.bestial_wrath = get_cooldown( "bestial_wrath" );
    cooldowns.dire_beast    = get_cooldown( "dire_beast" );
    cooldowns.wild_thrash   = get_cooldown( "wild_thrash" );

    cooldowns.wildfire_bomb       = get_cooldown( "wildfire_bomb" );
    cooldowns.harpoon             = get_cooldown( "harpoon" );
    cooldowns.boomstick           = get_cooldown( "boomstick" );
    cooldowns.strike_as_one       = get_cooldown( "strike_as_one" );
    cooldowns.takedown            = get_cooldown( "takedown" );
    cooldowns.flamefang_pitch     = get_cooldown( "flamefang_pitch" );

    cooldowns.black_arrow = get_cooldown( "black_arrow" );
    cooldowns.bleak_powder = get_cooldown( "bleak_powder_icd" );

    base_gcd = 1.5_s;

    resource_regeneration = regen_type::DYNAMIC;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }

  // Character Definition
  void init() override;
  void init_spells() override;
  void init_base_stats() override;
  void create_actions() override;
  void create_buffs() override;
  void init_gains() override;
  void init_position() override;
  void init_procs() override;
  void init_rng() override;
  void init_scaling() override;
  void init_assessors() override;
  void init_action_list() override;
  void init_blizzard_action_list() override;
  parsed_assisted_combat_rule_t parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule, const assisted_combat_step_data_t& step ) const override;
  std::vector<std::string> action_names_from_spell_id( unsigned int spell_id ) const override;
  void parse_assisted_combat_step( const assisted_combat_step_data_t& step, action_priority_list_t* assisted_combat ) override;
  void init_special_effects() override;
  void init_finished() override;
  void reset() override;
  void merge( player_t& other ) override;
  void arise() override;
  void combat_begin() override;
  bool validate_actor() override;

  void datacollection_begin() override;
  void datacollection_end() override;

  double composite_attribute_multiplier( attribute_e ) const override;
  double composite_melee_crit_chance() const override;
  double composite_spell_crit_chance() const override;
  double composite_rating_multiplier( rating_e ) const override;
  double composite_melee_auto_attack_speed() const override;
  double composite_player_critical_damage_multiplier( const action_state_t*, school_e ) const override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double composite_player_pet_damage_multiplier( const action_state_t*, bool ) const override;
  double composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const override;
  double composite_leech() const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  double stacking_movement_modifier() const override;
  void invalidate_cache( cache_e ) override;
  void regen( timespan_t periodicity ) override;
  double resource_gain( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;
  void create_options() override;
  std::unique_ptr<expr_t> create_expression( util::string_view expression_str ) override;
  std::unique_ptr<expr_t> create_action_expression( action_t&, util::string_view expression_str ) override;
  action_t* create_action( util::string_view name, util::string_view options ) override;
  pet_t* create_pet( util::string_view name, util::string_view type ) override;
  void create_pets() override;
  double resource_loss( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;
  resource_e primary_resource() const override { return RESOURCE_FOCUS; }
  role_e primary_role() const override { return ROLE_ATTACK; }
  stat_e convert_hybrid_stat( stat_e s ) const override;
  std::string create_profile( save_e ) override;
  void copy_from( player_t* source ) override;
  void moving( ) override;

  std::string default_potion() const override { return hunter_apl::potion( this ); }
  std::string default_flask() const override { return hunter_apl::flask( this ); }
  std::string default_food() const override { return hunter_apl::food( this ); }
  std::string default_rune() const override { return hunter_apl::rune( this ); }
  std::string default_temporary_enchant() const override { return hunter_apl::temporary_enchant( this ); }

  target_specific_t<hunter_td_t> target_data;

  const hunter_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

  hunter_td_t* get_target_data( player_t* target ) const override
  {
    hunter_td_t*& td = target_data[target];
    if ( !td ) td = new hunter_td_t( target, const_cast<hunter_t*>( this ) );
    return td;
  }

  std::vector<action_t*> background_actions;

  template <typename T, typename... Ts>
  T* get_background_action( util::string_view n, Ts&&... args )
  {
    auto it = range::find( background_actions, n, &action_t::name_str );
    if ( it != background_actions.cend() )
      return dynamic_cast<T*>( *it );

    auto action = new T( n, this, std::forward<Ts>( args )... );
    action -> background = true;
    background_actions.push_back( action );
    return action;
  }

  void trigger_bloodseeker_update();
  int ticking_dots( hunter_td_t* td );
  void trigger_outland_venom_update();
  void consume_trick_shots();
  void trigger_deathblow( bool activated = false );
  void trigger_lunar_storm( player_t* target );
  void consume_precise_shots();
  void trigger_eagles_mark( player_t* target, bool sentinel, bool force = false );
  bool consume_howl_of_the_pack_leader( player_t* target );
  void trigger_howl_of_the_pack_leader();
  void trigger_natures_ally_3();
  void trigger_huntmasters_call();
  void spawn_dire_beast( timespan_t base_duration, bool force_hound = false );
};

// Template for common hunter action code.
template <class Base>
struct hunter_action_t: public Base
{
private:
  using ab = Base;
public:

  bool track_cd_waste;
  maybe_bool decrements_tip_of_the_spear;
  double dire_beast_chance = 0;

  struct {
    // Hunter
    // TODO 26/4/25: possibly entirely scripted now, now that the passive mods are gone we know the target debuff 459529 should be ignored by 
    // residual bleeds but they see behavior identical to normal dots: 10% is correctly applied outside of execute but in execute range a final
    // value of about 15.23% is seen now, so using the same assumed behavior of the bleed mods originally, execute range simply applies another 4.76% mod
    damage_affected_by unnatural_causes;

    // Beast Mastery
    damage_affected_by bestial_wrath;
    damage_affected_by master_of_beasts;

    // Marksmanship
    bool trueshot_crit_damage_bonus = false;
    bool bullseye_crit_chance = false;
    damage_affected_by lone_wolf;
    damage_affected_by sniper_training;
    damage_affected_by headshot;

    // Survival
    bool outland_venom = false;
    damage_affected_by spirit_bond;
    damage_affected_by tip_of_the_spear;
    damage_affected_by mongoose_fury;
    damage_affected_by wallop;
    damage_affected_by takedown;

    // Sentinel
    damage_affected_by sentinels_mark;
    bool stargazer = false;

    // Pack Leader
    damage_affected_by wyverns_cry;

    // Dark Ranger
    damage_affected_by through_the_eyes;
  } affected_by;

  cdwaste::action_data_t* cd_waste = nullptr;

  hunter_action_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
    ab( n, p, s ),
    track_cd_waste( s -> cooldown() > 0_ms || s -> charge_cooldown() > 0_ms )
  {
    ab::special = true;

    affected_by.unnatural_causes = parse_damage_affecting_aura( this, p->talents.unnatural_causes_debuff );
    
    affected_by.sniper_training = parse_damage_affecting_aura( this, p->mastery.sniper_training );
    affected_by.headshot = parse_damage_affecting_aura( this, p->talents.headshot_debuff );
    affected_by.trueshot_crit_damage_bonus = check_affected_by( this, p->talents.trueshot->effectN( 4 ) );
    affected_by.bullseye_crit_chance = check_affected_by( this, p->talents.bullseye->effectN( 1 ).trigger()->effectN( 1 ) );

    affected_by.bestial_wrath = parse_damage_affecting_aura( this, p->talents.bestial_wrath );
    affected_by.master_of_beasts = parse_damage_affecting_aura( this, p->mastery.master_of_beasts );

    affected_by.spirit_bond = parse_damage_affecting_aura( this, p->mastery.spirit_bond );
    affected_by.tip_of_the_spear = parse_damage_affecting_aura( this, p->talents.tip_of_the_spear_buff );
    affected_by.outland_venom = check_affected_by( this, p->talents.outland_venom_debuff->effectN( 1 ) );
    affected_by.mongoose_fury = parse_damage_affecting_aura( this, p->talents.mongoose_fury_buff );
    affected_by.wallop = parse_damage_affecting_aura( this, p->talents.wallop_buff );
    affected_by.takedown = parse_damage_affecting_aura( this, p->talents.takedown );

    affected_by.sentinels_mark = parse_damage_affecting_aura( this, p->talents.sentinels_mark );
    affected_by.stargazer      = check_affected_by( this, p->talents.stargazer_buff->effectN( 1 ) );

    affected_by.wyverns_cry = parse_damage_affecting_aura( this, p->talents.howl_of_the_pack_leader_wyvern_buff );

    affected_by.through_the_eyes = parse_damage_affecting_aura( this, p->talents.black_arrow_dot );
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

    if ( p()->talents.tip_of_the_spear.ok() )
    {
      if ( decrements_tip_of_the_spear.is_none() )
        decrements_tip_of_the_spear = affected_by.tip_of_the_spear.direct > 0;
    }
    else
    {
      decrements_tip_of_the_spear = false;
    }

    if ( p()->talents.dire_beast.ok() )
    {
      if ( dire_beast_chance == 0 )
      {
        for ( size_t i = 1; i <= ab::data().effect_count(); i++ )
        {
          if ( ab::data().effectN( i ).subtype() == effect_subtype_t::A_PERIODIC_DAMAGE &&
            ab::data().get_school_type() == SCHOOL_PHYSICAL &&
            ( ab::data().effectN( i ).mechanic() == MECHANIC_BLEED || ab::data().mechanic() == MECHANIC_BLEED ) )
          {
            dire_beast_chance = p()->talents.dire_beast->effectN( 1 ).percent();
            break;
          }
        }
      }
    }

    if ( decrements_tip_of_the_spear )
      ab::sim->print_debug( "{} action {} set to decrement Tip of the Spear", ab::player->name(), ab::name() );

    if ( dire_beast_chance > 0 )
      ab::sim->print_debug( "{} action {} set to trigger Dire Beast with {}% chance", ab::player->name(), ab::name(), dire_beast_chance * 100 );
  }

  timespan_t gcd() const override
  {
    timespan_t g = ab::gcd();

    if ( g == 0_ms )
      return g;

    if ( g < ab::min_gcd )
      g = ab::min_gcd;

    return g;
  }

  void execute() override
  {
    ab::execute();

    if ( affected_by.wallop.direct )
      p()->buffs.wallop->expire();
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    // Tip removal and effects are triggered on impact but only once
    if ( decrements_tip_of_the_spear && s->chain_target == 0 && p()->buffs.tip_of_the_spear->check() )
    {
      p()->buffs.tip_of_the_spear->decrement();
      p()->buffs.stargazer->trigger();

      // 2026-02-13: For Survival, Sentinel's Mark applies to a random target hit for AoE spells.
      //             Tipped Wildfire Bombs can also trigger an additional mark after consuming one so make an event.
      make_event( p()->sim, [ this ]() { p()->trigger_eagles_mark( get_random_valid_target(), true ); } );

      if ( p()->cooldowns.strike_as_one->up() )
      {
        auto pet = p()->pets.main;
        if ( pet )
        {
          pet->actions.strike_as_one->execute_on_target( p()->target );
          p()->cooldowns.strike_as_one->start();
        }
      }
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double am = ab::composite_da_multiplier( s );

    if ( affected_by.bestial_wrath.direct )
      am *= 1 + p()->buffs.bestial_wrath->check_value();

    if ( affected_by.master_of_beasts.direct )
      am *= 1 + p()->cache.mastery() * p()->mastery.master_of_beasts->effectN( affected_by.master_of_beasts.direct ).mastery_value();

    if ( affected_by.sniper_training.direct )
      am *= 1 + p()->cache.mastery() * p()->mastery.sniper_training->effectN( affected_by.sniper_training.direct ).mastery_value();

    if ( affected_by.spirit_bond.direct )
    {
      double bonus = p()->cache.mastery() * p()->mastery.spirit_bond->effectN( affected_by.spirit_bond.direct ).mastery_value();
      bonus *= 1 + p()->mastery.spirit_bond_buff->effectN( 1 ).percent();
      am *= 1 + bonus;
    }

    if ( affected_by.mongoose_fury.direct && p()->buffs.mongoose_fury->check() )
      am *= 1 + p()->buffs.mongoose_fury->stack_value();

    if ( affected_by.wallop.direct && p()->buffs.wallop->check() )
      am *= 1 + p()->buffs.wallop->value();

    if ( affected_by.takedown.direct && p()->buffs.takedown->check() )
      am *= 1 + p()->talents.takedown->effectN( affected_by.takedown.direct ).percent();

    if ( affected_by.tip_of_the_spear.direct && p()->buffs.tip_of_the_spear->check() )
      am *= 1 + p()->talents.tip_of_the_spear_buff->effectN( 1 ).percent();

    if ( affected_by.wyverns_cry.direct )
      am *= 1 + p()->buffs.wyverns_cry->check_stack_value();

    return am;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double am = ab::composite_ta_multiplier( s );

    if ( affected_by.bestial_wrath.tick )
      am *= 1 + p()->buffs.bestial_wrath->check_value();

    if ( affected_by.master_of_beasts.tick )
      am *= 1 + p()->cache.mastery() * p()->mastery.master_of_beasts->effectN( affected_by.master_of_beasts.tick ).mastery_value();

    if ( affected_by.sniper_training.tick )
      am *= 1 + p()->cache.mastery() * p()->mastery.sniper_training->effectN( affected_by.sniper_training.tick ).mastery_value();

    if ( affected_by.spirit_bond.tick )
    {
      double bonus = p()->cache.mastery() * p()->mastery.spirit_bond->effectN( affected_by.spirit_bond.tick ).mastery_value();
      bonus *= 1 + p()->mastery.spirit_bond_buff->effectN( 3 ).percent();
      am *= 1 + bonus;
    }

    if ( affected_by.takedown.tick && p()->buffs.takedown->check() )
      am *= 1 + p()->talents.takedown->effectN( affected_by.takedown.tick ).percent();

    if ( affected_by.wyverns_cry.tick )
      am *= 1 + p()->buffs.wyverns_cry->check_stack_value();

    return am;
  }

  double composite_crit_chance() const override
  {
    double cc = ab::composite_crit_chance();

    if ( affected_by.bullseye_crit_chance )
      cc += p()->buffs.bullseye->check_stack_value();

    return cc;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double cm = ab::composite_crit_damage_bonus_multiplier();

    if ( affected_by.trueshot_crit_damage_bonus && p()->buffs.trueshot->check() )
      cm *= 1 + p()->talents.trueshot->effectN( 4 ).percent();

    if ( affected_by.stargazer && p()->buffs.stargazer->check() )
      cm *= 1 + p()->buffs.stargazer->stack_value();

    return cm;
  }

  double composite_target_crit_damage_bonus_multiplier( player_t* target ) const override
  {
    double cm = ab::composite_target_crit_damage_bonus_multiplier( target );

    if ( affected_by.outland_venom )
      cm *= 1 + td( target )->debuffs.outland_venom->check_stack_value();

    return cm;
  }

  double composite_target_da_multiplier( player_t* target ) const override
  {
    double da = ab::composite_target_da_multiplier( target );

    if ( affected_by.unnatural_causes.direct )
    {
      da *= 1 + p()->talents.unnatural_causes->effectN( 1 ).percent();

      if ( target->health_percentage() < p()->talents.unnatural_causes->effectN( 3 ).base_value() )
        da *= 1.0476;
    }

    if ( affected_by.sentinels_mark.direct )
      da *= 1 + td( target )->debuffs.sentinels_mark->check_value();

    if ( affected_by.headshot.direct && td( target )->debuffs.headshot->check() )
      da *= 1 + td( target )->debuffs.headshot->stack_value();

    if ( p()->specialization() == HUNTER_BEAST_MASTERY 
      && affected_by.through_the_eyes.direct 
      && td( target )->dots.black_arrow->is_ticking() )
      da *= 1 + p()->talents.black_arrow_dot->effectN( 2 ).percent();

    return da;
  }

  double composite_target_ta_multiplier( player_t* target ) const override
  {
    double ta = ab::composite_target_ta_multiplier( target );

    if ( affected_by.unnatural_causes.tick )
    {
      ta *= 1 + p()->talents.unnatural_causes->effectN( 1 ).percent();

      if ( target->health_percentage() < p()->talents.unnatural_causes->effectN( 3 ).base_value() )
        ta *= 1.0476;
    }

    if ( affected_by.headshot.tick && td( target )->debuffs.headshot->check() )
      ta *= 1 + td( target )->debuffs.headshot->stack_value();

    if ( p()->specialization() == HUNTER_BEAST_MASTERY 
      && affected_by.through_the_eyes.tick 
      && td( target )->dots.black_arrow->is_ticking() )
      ta *= 1 + p()->talents.black_arrow_dot->effectN( 2 ).percent();

    return ta;
  }

  void tick( dot_t* dot ) override
  {
    ab::tick( dot );

    // 2026-04-03: Dire Beast seems to start it's internal cooldown on any bleed tick if the ICD isn't already running regardless of proc or not.
    if ( p()->cooldowns.dire_beast->up() && dire_beast_chance > 0 )
    {
      p()->cooldowns.dire_beast->start();
      if ( p()->rng().roll( dire_beast_chance ) )
      {
        p()->spawn_dire_beast( p()->talents.dire_beast_summon->duration() );
      }
    }
  }

  void update_ready( timespan_t cd ) override
  {
    if ( cd_waste )
      cd_waste -> update_ready( this, cd );

    ab::update_ready( cd );
  }

  virtual double energize_cast_regen( const action_state_t* s ) const
  {
    const int num_targets = this -> n_targets();
    size_t targets_hit = 1;
    if ( ab::energize_type == action_energize::PER_HIT && ( num_targets == -1 || num_targets > 0 ) )
    {
      size_t tl_size = this -> target_list().size();
      targets_hit = ( num_targets < 0 ) ? tl_size : std::min( tl_size, as<size_t>( num_targets ) );
    }
    return targets_hit * this -> composite_energize_amount( s );
  }

  virtual double cast_regen( const action_state_t* s ) const
  {
    const timespan_t execute_time = this -> execute_time();
    const timespan_t cast_time = std::max( execute_time, this -> gcd() );
    const double regen = p() -> resource_regen_per_second( RESOURCE_FOCUS );

    double total_regen = regen * cast_time.total_seconds();
    double total_energize = energize_cast_regen( s );

    return total_regen + floor( total_energize );
  }

  // action list expressions
  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    if ( util::str_compare_ci( name, "cast_regen" ) )
    {
      // Return the focus that will be regenerated during the cast time or GCD of the target action.
      return make_fn_expr( "cast_regen",
        [ this, state = std::unique_ptr<action_state_t>( this -> get_state() ) ] {
          this -> snapshot_state( state.get(), result_amount_type::NONE );
          state -> target = this -> target;
          return this -> cast_regen( state.get() );
        } );
    }

    // fudge wildfire bomb dot name
    auto splits = util::string_split<util::string_view>( name, "." );
    if ( splits.size() == 3 && splits[ 0 ] == "dot" && splits[ 1 ] == "wildfire_bomb" )
      return ab::create_expression( fmt::format( "dot.wildfire_bomb_dot.{}", splits[ 2 ] ) );

    return ab::create_expression( name );
  }

  void add_pet_stats( pet_t* pet, std::initializer_list<util::string_view> names )
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

  bool trigger_buff( buff_t *const buff, timespan_t precast_time, timespan_t duration = timespan_t::min() ) const
  {
    const bool in_combat = ab::player -> in_combat;
    const bool triggered = buff -> trigger(duration);
    if ( triggered && ab::is_precombat && !in_combat && precast_time > 0_ms )
    {
      buff -> extend_duration( -std::min( precast_time, buff -> buff_duration() ) );
      buff -> cooldown -> adjust( -precast_time );
    }
    return triggered;
  }

  void adjust_precast_cooldown( timespan_t precast_time ) const
  {
    const bool in_combat = ab::player -> in_combat;
    if ( ab::is_precombat && !in_combat && precast_time > 0_ms )
      ab::cooldown -> adjust( -precast_time );
  }

  player_t* get_random_valid_target( std::optional<int> aoe_override = std::nullopt ) const
  {
    const int aoe = aoe_override.value_or( ab::aoe );

    switch ( aoe )
    {
      case 0: 
        return ab::target;

      case -1:
      {
        const auto tl = ab::target_list();
        if ( !tl.empty() )
          return p()->rng().range( tl );

        break;
      }

      // Capped targets
      default:
      {
        const auto tl = ab::target_list();
        if ( !tl.empty() && aoe > 0 )
        {
          const size_t cap = std::min<size_t>( aoe, tl.size() );
          const size_t t   = p()->rng().template range<size_t>( 0, cap );
          return tl[ t ];
        }
        break;
      }
    }
    return ab::target;
  }
};

struct hunter_spell_t : public hunter_action_t<spell_t>
{
  hunter_spell_t( util::string_view n, hunter_t* p, const spell_data_t* s = spell_data_t::nil() ) : hunter_action_t( n, p, s ) {}
  bool usable_moving() const override { return true; }
};

struct hunter_ranged_attack_t : public hunter_action_t<ranged_attack_t>
{
  hunter_ranged_attack_t( util::string_view n, hunter_t* p, const spell_data_t* s = spell_data_t::nil() ) : hunter_action_t( n, p, s ) {}
  bool usable_moving() const override { return true; }
};

struct hunter_melee_attack_t : public hunter_action_t<melee_attack_t>
{
  hunter_melee_attack_t( util::string_view n, hunter_t* p, const spell_data_t* s = spell_data_t::nil() ) : hunter_action_t( n, p, s ) {}

  void init() override
  {
    hunter_action_t::init();

    if ( weapon )
    {
      const weapon_e group = weapon->group();
      if ( group != WEAPON_2H && group != WEAPON_1H && group != WEAPON_SMALL )
        background = true;
    }
  }
};

namespace pets
{
// ==========================================================================
// Hunter Pet
// ==========================================================================

struct hunter_pet_t: public pet_t
{
  struct buffs_t
  {
    buff_t* beast_cleave = nullptr;
  } buffs;

  struct actions_t
  {
    action_t* beast_cleave = nullptr;
  } actions;

  hunter_pet_t( hunter_t* owner, util::string_view pet_name, pet_e pt = PET_HUNTER, bool guardian = false, bool dynamic = false ) :
    pet_t( owner -> sim, owner, pet_name, pt, guardian, dynamic )
  {
    owner_coeff.ap_from_ap = 0.15;

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = 2_s;
  }

  void schedule_ready( timespan_t delta_time, bool waiting ) override
  {
    if ( main_hand_attack && !main_hand_attack->execute_event )
      main_hand_attack->schedule_execute();

    pet_t::schedule_ready( delta_time, waiting );
  }

  double composite_melee_attack_power() const override
  {
    double ap = pet_t::composite_melee_attack_power();

    return ap;
  }

  void create_buffs() override
  {
    pet_t::create_buffs();

    buffs.beast_cleave =
      make_buff( this, "beast_cleave", find_spell( 118455 ) )
      -> set_default_value( o()->talents.beast_cleave.ok() ? o() -> talents.beast_cleave -> effectN( 1 ).percent() : 1.0 );
  }

  hunter_t* o()             { return static_cast<hunter_t*>( owner ); }
  const hunter_t* o() const { return static_cast<hunter_t*>( owner ); }

  void init_spells() override;
};

static std::pair<timespan_t, int> dire_beast_duration( hunter_t* p, timespan_t base_duration )
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
  const timespan_t swing_time       = 2_s * p->cache.auto_attack_speed();
  double partial_attacks_per_summon = base_duration / swing_time;
  int base_attacks_per_summon       = static_cast<int>( partial_attacks_per_summon );
  partial_attacks_per_summon -= static_cast<double>( base_attacks_per_summon );

  if ( p->rng().roll( partial_attacks_per_summon ) )
    base_attacks_per_summon += 1;

  return { base_attacks_per_summon * swing_time, base_attacks_per_summon };
}

// ==========================================================================
// Dark Minion (Corpsecaller)
// ==========================================================================

struct dark_minion_t final : public hunter_pet_t
{
  struct 
  {
    action_t* shoot          = nullptr;
    action_t* blighted_arrow = nullptr;
  } actions;

  dark_minion_t( hunter_t* owner, util::string_view n = "dark_minion" ) 
    : hunter_pet_t( owner, n, PET_HUNTER, true /* GUARDIAN */, true /* dynamic */ )
  {
    resource_regeneration = regen_type::DISABLED;
    owner_coeff.ap_from_ap = 1;
  }

  void update_stats() override
  {
    // 2026-01-25: Dark Minions only seem to inherit AP and Crit from the player.
    current_pet_stats.attack_power_from_ap = owner->composite_total_attack_power_by_type( owner->default_ap_type() ) * owner_coeff.ap_from_ap;
    sim->print_debug( "{} refreshed AP from owner (ap={})", name(), composite_melee_attack_power() );

    current_pet_stats.composite_melee_crit = owner->cache.attack_crit_chance();
    current_pet_stats.composite_spell_crit = owner->cache.spell_crit_chance();
    sim->print_debug( "{} refreshed Critical Strike from owner (crit={})", name(), current_pet_stats.composite_melee_crit, owner->cache.attack_crit_chance() );

    this->adjust_dynamic_cooldowns();
  }

  void init_action_list() override
  {
    pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "shoot" );
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    hunter_pet_t::summon( duration );

    o()->procs.dark_minion_spawn->occur();
  }

  void arise() override
  {
    pet_t::arise();

    /* 2026-01-25: Dark Minions don't cast Shoot for ~1.25s after they spawn.
                   Further log data required for more accurate range. */
    actions.shoot->cooldown->start( owner->rng().range( 1000_ms, 1500_ms ) );
  }

  void init_spells() override;

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
};

// ==========================================================================
// Dire Critter
// ==========================================================================

struct dire_critter_t : public hunter_pet_t
{
  struct buffs_t
  {
    buff_t* bestial_wrath;
    buff_t* pet_damage;
  } buffs;

  struct actions_t
  {
    action_t* kill_command = nullptr;
  } actions;

  bool triggers_heart_of_the_pack = false;

  dire_critter_t( hunter_t* owner, util::string_view n = "dire_beast" )
    : hunter_pet_t( owner, n, PET_HUNTER, true /* GUARDIAN */, true /* dynamic */ )
  {
    resource_regeneration = regen_type::DISABLED;
  }

  // Used to trigger any behaviour that needs to run after base summon() but before auto attacks start...
  // for inheriting child classes.
  virtual void additional_summon_behavior() {};

  void create_buffs() override
  {
    hunter_pet_t::create_buffs();

    buffs.bestial_wrath =
      make_buff( this, "bestial_wrath", o()->talents.wildspeaker_bestial_wrath )
        ->set_default_value_from_effect( 1 );

    buffs.pet_damage = 
      make_buff( this, "pet_damage", o()->specs.pet_damage )
        ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_DONE );
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    hunter_pet_t::summon( duration );

    if ( o()->talents.dire_cleave.ok() )
      hunter_pet_t::buffs.beast_cleave->trigger( o()->talents.dire_cleave->effectN( 2 ).time_value() );

    if ( o()->talents.wildspeaker.ok() && o()->buffs.bestial_wrath->check() )
      buffs.bestial_wrath->trigger( o()->buffs.bestial_wrath->remains() );

    if ( triggers_heart_of_the_pack && o()->talents.heart_of_the_pack.ok() )
      o()->buffs.heart_of_the_pack->trigger();

    buffs.pet_damage->trigger();
    
    additional_summon_behavior();

    if ( main_hand_attack )
      main_hand_attack->execute();
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = hunter_pet_t::composite_player_multiplier( school );

    if ( o()->talents.dire_frenzy.ok() )
      m *= 1 + o()->talents.dire_frenzy->effectN( 2 ).percent();

    if ( buffs.pet_damage->check() )
      m *= 1 + buffs.pet_damage->check_value();

    if ( buffs.bestial_wrath->has_common_school( school ) )
      m *= 1 + buffs.bestial_wrath->check_value();

    return m;
  }

  void init_spells() override;
};

// ==========================================================================
// Dark Hound (Corpsecaller)
// ==========================================================================

struct dark_hound_t final : public dire_critter_t
{
  struct
  {
    action_t* shadow_thrash = nullptr;
  } actions;

  dark_hound_t( hunter_t* owner, util::string_view n = "dark_hound" ) : dire_critter_t( owner, n )
  {
    resource_regeneration  = regen_type::DISABLED;
    owner_coeff.ap_from_ap = 1.5;
    auto_attack_multiplier = 4;
    triggers_heart_of_the_pack = true;
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    dire_critter_t::summon( duration );

    o()->procs.dark_hound_spawn->occur();
  }

  void init_spells() override;
};

// ==========================================================================
// Dire Beast
// ==========================================================================

struct dire_beast_t final : public dire_critter_t
{
  dire_beast_t( hunter_t* owner, util::string_view n = "dire_beast" ) : dire_critter_t( owner, n )
  {
    // 11-10-22 Dire Beast - Damage increased by 400%. (15% -> 60%)
    // 13-10-22 Dire Beast damage increased by 50%. (60% -> 90%)
    // 22-7-24 Dire Beast damage increased by 10% (90% -> 100%)
    owner_coeff.ap_from_ap = 1;
    triggers_heart_of_the_pack = true;
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    dire_critter_t::summon( duration );

    o()->procs.dire_beast_spawn->occur();
  }
};

// =========================================================================
// Fenryr
// =========================================================================

struct fenryr_td_t final : public actor_target_data_t
{
public:
  struct dots_t
  {
    dot_t* ravenous_leap = nullptr;
  } dots;

  fenryr_td_t( player_t* target, fenryr_t* p );
};

struct fenryr_t final : public dire_critter_t
{
  struct actions_t
  {
    action_t* ravenous_leap = nullptr;
  } actions;

  target_specific_t<fenryr_td_t> target_data;

  fenryr_t( hunter_t* owner, util::string_view n = "fenryr" ) : dire_critter_t( owner, n )
  {
    // 9-7-25 Hati and Fenryr base damage increased to about 2x of a normal Dire Beast's damage.
    owner_coeff.ap_from_ap = 2;
    triggers_heart_of_the_pack = true;
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    dire_critter_t::summon( duration );

    actions.ravenous_leap->execute_on_target( target );
  }

  const fenryr_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

  fenryr_td_t* get_target_data( player_t* target ) const override
  {
    fenryr_td_t*& td = target_data[target];
    if ( !td )
      td = new fenryr_td_t( target, const_cast<fenryr_t*>( this ) );
    return td;
  }

  void init_spells() override;
};

// ==========================================================================
// Hati
// ==========================================================================

struct hati_t final : public dire_critter_t
{
  hati_t( hunter_t* owner, util::string_view n = "hati" ) : dire_critter_t( owner, n )
  {
   // 9-7-25 Hati and Fenryr base damage increased to about 2x of a normal Dire Beast's damage.
    owner_coeff.ap_from_ap = 2;
    triggers_heart_of_the_pack = true;
  }
};

// ==========================================================================
// Bear
// ==========================================================================

struct bear_td_t final : public actor_target_data_t
{
public:
  struct dots_t
  {
    dot_t* rend_flesh = nullptr;
  } dots;

  bear_td_t( player_t* target, bear_t* p );
};

struct bear_t final : public dire_critter_t
{
  struct buffs_t
  {
    buff_t* bear_summon;
  } buffs;

  struct actions_t
  {
    action_t* rend_flesh = nullptr;
  } actions;
  
  target_specific_t<bear_td_t> target_data;

  bear_t( hunter_t* owner, util::string_view n = "bear" ) : dire_critter_t( owner, n )
  {
    owner_coeff.ap_from_ap = 0.6;
    auto_attack_multiplier = 7;
    main_hand_weapon.swing_time = 1.5_s;
    triggers_heart_of_the_pack = true;
  }

  void additional_summon_behavior() override
  {
    buffs.bear_summon->trigger();

    o()->trigger_huntmasters_call();
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    dire_critter_t::summon( duration );

    if ( actions.rend_flesh )
      actions.rend_flesh->execute_on_target( target );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = dire_critter_t::composite_player_multiplier( school );

    if ( buffs.bear_summon->has_common_school( school ) )
      m *= 1 + buffs.bear_summon->check_value();
    
    return m;
  }

  void create_buffs() override
  {
    dire_critter_t::create_buffs();

    buffs.bear_summon = make_buff( this, "bear_summon", o()->talents.howl_of_the_pack_leader_bear_buff )
      ->set_default_value_from_effect( 1 );
  }

  const bear_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

  bear_td_t* get_target_data( player_t* target ) const override
  {
    bear_td_t*& td = target_data[target];
    if ( !td )
      td = new bear_td_t( target, const_cast<bear_t*>( this ) );
    return td;
  }

  void init_spells() override;
};

// Base class for pets from player stable
// TODO move code to hunter_main_pet_base_t and remove
struct stable_pet_t : public hunter_pet_t
{
  struct actions_t
  {
    action_t* stomp = nullptr;
    action_t* thundering_hooves = nullptr;
  } actions;

  stable_pet_t( hunter_t* owner, util::string_view pet_name, pet_e pet_type ):
    hunter_pet_t( owner, pet_name, pet_type, false /* GUARDIAN */, true /* dynamic */ )
  {
    stamina_per_owner = 0.7;
    owner_coeff.ap_from_ap = 0.6;

    initial.armor_multiplier *= 1.05;

    main_hand_weapon.swing_time = owner -> options.pet_attack_speed;
  }
  
  void init_spells() override;
};

// ==========================================================================
// Main Pet base
// ==========================================================================

struct hunter_main_pet_base_td_t: public actor_target_data_t
{
public:
  struct dots_t
  {
    dot_t* bloodshed = nullptr;
  } dots;

  hunter_main_pet_base_td_t( player_t* target, hunter_main_pet_base_t* p );
};

struct hunter_main_pet_base_t : public stable_pet_t
{
  struct dots_t
  {
    dot_t* bloodshed = nullptr;
  } dots;

  struct actions_t
  {
    action_t* kill_command = nullptr;
    action_t* kill_cleave = nullptr;
    action_t* bestial_wrath = nullptr;
    action_t* bloodshed = nullptr;
    action_t* wild_thrash = nullptr;
  } actions;

  struct buffs_t
  {
    buff_t* bestial_wrath = nullptr;
    buff_t* piercing_fangs = nullptr;
  } buffs;

  target_specific_t<hunter_main_pet_base_td_t> target_data;

  hunter_main_pet_base_t( hunter_t* owner, util::string_view pet_name, pet_e pet_type ) : stable_pet_t( owner, pet_name, pet_type ) {}

  void create_buffs() override
  {
    stable_pet_t::create_buffs();

    buffs.bestial_wrath =
      make_buff( this, "bestial_wrath", find_spell( 186254 ) )
        -> set_default_value_from_effect( 1 )
        -> set_cooldown( 0_ms )
        -> set_stack_change_callback( [ this ]( buff_t*, int old, int cur ) {
          if ( cur == 0 )
          {
            buffs.piercing_fangs -> expire();
          }
          else if (old == 0) {
            buffs.piercing_fangs -> trigger();
          }
        } );

    buffs.piercing_fangs =
      make_buff( this, "piercing_fangs", o() -> find_spell( 392054 ) )
        -> set_default_value_from_effect( 1 )
        -> set_chance( o() -> talents.piercing_fangs.ok() );
  }

  double composite_melee_auto_attack_speed() const override
  {
    double as = stable_pet_t::composite_melee_auto_attack_speed();

    if ( o()->talents.frenzy.ok() )
      as /= 1 + o()->talents.frenzy->effectN( 1 ).percent();

    return as;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = stable_pet_t::composite_player_multiplier( school );

    if ( buffs.bestial_wrath -> has_common_school( school ) )
      m *= 1 + buffs.bestial_wrath -> check_value();
    
    return m;
  }

  double composite_player_critical_damage_multiplier( const action_state_t* s, school_e school ) const override
  {
    double m = stable_pet_t::composite_player_critical_damage_multiplier( s, school );

    if ( buffs.piercing_fangs -> data().effectN( 1 ).has_common_school( school ) )
      m *= 1 + buffs.piercing_fangs -> check_value();

    return m;
  }

  const hunter_main_pet_base_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

  hunter_main_pet_base_td_t* get_target_data( player_t* target ) const override
  {
    hunter_main_pet_base_td_t*& td = target_data[target];
    if ( !td )
      td = new hunter_main_pet_base_td_t( target, const_cast<hunter_main_pet_base_t*>( this ) );
    return td;
  }

  void moving() override { return; }

  void init_spells() override;
  void init_special_effects() override;
};

// ==========================================================================
// Animal Companion
// ==========================================================================

struct animal_companion_t final : public hunter_main_pet_base_t
{
  animal_companion_t( hunter_t* owner ) : hunter_main_pet_base_t( owner, "animal_companion", PET_HUNTER )
  {
    resource_regeneration = regen_type::DISABLED;
  }

  void init_spells() override;
};

// ==========================================================================
// Nature's Ally Pet
// ==========================================================================

struct natures_ally_pet_t final : public hunter_main_pet_base_t
{
  natures_ally_pet_t( hunter_t* owner ) : hunter_main_pet_base_t( owner, "natures_ally_pet", PET_HUNTER )
  {
    resource_regeneration = regen_type::DISABLED;
  }

  void create_buffs() override
  {
    hunter_main_pet_base_t::create_buffs();

    // Nature's Ally pets have a unique Bestial Wrath aura
    buffs.bestial_wrath =
      make_buff( this, "bestial_wrath_apex", find_spell( 1285912 ) )
        ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_DONE )
        ->set_cooldown( 0_ms )
        ->set_stack_change_callback( [ this ]( buff_t*, int old, int cur ) {
          if ( cur == 0 )
          {
            buffs.piercing_fangs->expire();
          }
          else if ( old == 0 )
          {
            buffs.piercing_fangs->trigger();
          }
        } );

    buffs.piercing_fangs =
      make_buff( this, "piercing_fangs", o()->find_spell( 392054 ) )
        ->set_default_value_from_effect( 1 )
        ->set_chance( o()->talents.piercing_fangs.ok() );
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    hunter_main_pet_base_t::summon( duration );

    if ( main_hand_attack )
      main_hand_attack->execute();
  }
};

// ==========================================================================
// Main Pet
// ==========================================================================

struct hunter_main_pet_td_t: public hunter_main_pet_base_td_t
{
public:
  hunter_main_pet_td_t( player_t* target, hunter_main_pet_t* p );
};

struct hunter_main_pet_t final : public hunter_main_pet_base_t
{
  struct actions_t
  {
    action_t* basic_attack = nullptr;
    action_t* brutal_companion_ba = nullptr;

    action_t* sic_em              = nullptr;
    action_t* strike_as_one       = nullptr;
    action_t* strike_as_one_swipe = nullptr;
    action_t* takedown            = nullptr;

  } actions;

  struct buffs_t
  {
    buff_t* solitary_companion = nullptr;

    buff_t* bloodseeker = nullptr;
  } buffs;
  
  target_specific_t<hunter_main_pet_td_t> target_data;

  hunter_main_pet_t( hunter_t* owner, util::string_view pet_name, pet_e pt ) : hunter_main_pet_base_t( owner, pet_name, pt )
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

  buff_t* spec_passive() const
  {
    switch ( specialization() )
    {
      case PET_CUNNING:  return o() -> buffs.pathfinding;
      case PET_FEROCITY: return o() -> buffs.predators_thirst;
      case PET_TENACITY: return o() -> buffs.endurance_training;
      default: assert( false && "Invalid pet spec" );
    }
    return nullptr;
  }

  void init_base_stats() override
  {
    hunter_main_pet_base_t::init_base_stats();

    resources.base[RESOURCE_HEALTH] = 6373;
    resources.base[RESOURCE_FOCUS] = 100;

    base_gcd = 1.5_s;

    resources.infinite_resource[RESOURCE_FOCUS] = o() -> resources.infinite_resource[RESOURCE_FOCUS];
  }

  void create_buffs() override
  {
    hunter_main_pet_base_t::create_buffs();

    buffs.bloodseeker =
      make_buff( this, "bloodseeker", o() -> find_spell( 260249 ) )
        -> set_default_value_from_effect( 1 )
        -> add_invalidate( CACHE_AUTO_ATTACK_SPEED );

    buffs.solitary_companion = 
      make_buff( this, "solitary_companion", find_spell( 474751 ) )
      ->set_default_value_from_effect( 2 );
  }

  void init_action_list() override
  {
    if ( action_list_str.empty() )
    {
      action_list_str += "/snapshot_stats";
      action_list_str += "/claw";
      use_default_action_list = true;
    }

    hunter_main_pet_base_t::init_action_list();
  }

  double resource_regen_per_second( resource_e r ) const override
  {
    if ( r == RESOURCE_FOCUS )
      return owner -> resource_regen_per_second( RESOURCE_FOCUS ) * 1.25;

    return hunter_main_pet_base_t::resource_regen_per_second( r );
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    hunter_main_pet_base_t::summon( duration );

    o() -> pets.main = this;
    
    if ( o()->talents.solitary_companion.ok() )
    {
       o()->pets.main->buffs.solitary_companion->trigger();
    }

    if ( o() -> pets.animal_companion )
    {
      o() -> pets.animal_companion -> summon();
      o() -> pets.animal_companion -> schedule_ready(0_s, false);
    }

    spec_passive() -> trigger();
  }

  void demise() override
  {
    hunter_main_pet_base_t::demise();

    if ( o() -> pets.main == this )
    {
      o() -> pets.main = nullptr;

      spec_passive() -> expire();
    }
    if ( o() -> pets.animal_companion )
      o() -> pets.animal_companion -> demise();
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = hunter_main_pet_base_t::composite_player_multiplier( school );

    if ( o()->talents.solitary_companion.ok() && buffs.solitary_companion->up() )
      m *= 1 + buffs.solitary_companion->check_value();
    
    return m;
  }

  double composite_melee_auto_attack_speed() const override
  {
    double ah = hunter_main_pet_base_t::composite_melee_auto_attack_speed();

    if ( buffs.bloodseeker && buffs.bloodseeker->check() )
      ah /= 1 + buffs.bloodseeker->check_stack_value();

    return ah;
  }

  const hunter_main_pet_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

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
    if ( !actions.basic_attack )
      return hunter_main_pet_base_t::available();

    const auto time_to_fc = timespan_t::from_seconds( ( actions.basic_attack->base_cost() - resources.current[ RESOURCE_FOCUS ] ) /
                                                        resource_regen_per_second( RESOURCE_FOCUS ) );
    const auto time_to_cd = actions.basic_attack->cooldown->remains();
    const auto remains = std::max( time_to_cd, time_to_fc );
    const auto delay_mean = o() -> options.pet_basic_attack_delay;
    const auto delay_stddev = 100_ms;
    const auto lag = rng().gauss( delay_mean, delay_stddev );
    return std::max( remains + lag, 100_ms );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
  void init_spells() override;
};

namespace actions
{

static void trigger_beast_cleave( const action_state_t* s )
{
  if ( !s->action->result_is_hit( s->result ) )
    return;

  if ( s->action->sim->active_enemies == 1 )
    return;

  auto p = debug_cast<hunter_pet_t*>( s->action->player );

  if ( !p->buffs.beast_cleave->up() )
    return;

  // Target multipliers do not replicate to secondary targets
  const double target_da_multiplier = ( 1.0 / s->target_da_multiplier );
  const double target_pet_multiplier = ( 1.0 / s->target_pet_multiplier );

  const double amount = s->result_total * p->buffs.beast_cleave->check_value() * target_da_multiplier * target_pet_multiplier;
  p->actions.beast_cleave->execute_on_target( s->target, amount );
}

// Template for common hunter pet action code.
template <class T_PET, class Base>
struct hunter_pet_action_t : public Base
{
private:
  using ab = Base;
public:

  double dire_beast_chance = 0;

  struct {
    // Hunter
    damage_affected_by unnatural_causes;

    // Beast Mastery
    damage_affected_by bestial_wrath;
    damage_affected_by master_of_beasts;

    // Survival
    damage_affected_by spirit_bond;
    damage_affected_by tip_of_the_spear;
    damage_affected_by mongoose_fury;

    // Pack Leader
    damage_affected_by wyverns_cry;

    // Sentinel
    bool stargazer = false;
  } affected_by;

  hunter_pet_action_t( util::string_view n, T_PET* p, const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, p, s )
  {
    // If pets are not reported separately, create single stats_t objects for the various pet abilities.
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

    affected_by.unnatural_causes = parse_damage_affecting_aura( this, o()->talents.unnatural_causes_debuff );

    affected_by.bestial_wrath = parse_damage_affecting_aura( this, o()->talents.bestial_wrath );
    affected_by.master_of_beasts = parse_damage_affecting_aura( this, o()->mastery.master_of_beasts );

    affected_by.spirit_bond = parse_damage_affecting_aura( this, o()->mastery.spirit_bond );
    affected_by.tip_of_the_spear = parse_damage_affecting_aura( this, o()->talents.tip_of_the_spear_buff );
    affected_by.mongoose_fury = parse_damage_affecting_aura( this, o()->talents.mongoose_fury_buff );

    affected_by.wyverns_cry = parse_damage_affecting_aura( this, o()->talents.howl_of_the_pack_leader_wyvern_buff );

    affected_by.stargazer = check_affected_by( this, o()->talents.stargazer_buff->effectN( 1 ) );
  }
  
  void init() override
  {
    ab::init();

    if ( o()->talents.dire_beast.ok() )
    {
      if ( dire_beast_chance == 0 )
      {
        for ( size_t i = 1; i <= ab::data().effect_count(); i++ )
        {
          if ( ab::data().effectN( i ).subtype() == effect_subtype_t::A_PERIODIC_DAMAGE &&
            ab::data().get_school_type() == SCHOOL_PHYSICAL &&
            ( ab::data().effectN( i ).mechanic() == MECHANIC_BLEED || ab::data().mechanic() == MECHANIC_BLEED ) )
          {
            dire_beast_chance = o()->talents.dire_beast->effectN( 1 ).percent();
            break;
          }
        }
      }
    }

    if ( dire_beast_chance > 0 )
      ab::sim->print_debug( "{} action {} set to trigger Dire Beast with {}% chance", ab::player->name(), ab::name(), dire_beast_chance * 100 );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double am = ab::composite_da_multiplier( s );

    if ( affected_by.bestial_wrath.direct )
      am *= 1 + o()->buffs.bestial_wrath->check_value();

    if ( affected_by.master_of_beasts.direct )
      am *= 1 + o()->cache.mastery() * o()->mastery.master_of_beasts->effectN( affected_by.master_of_beasts.direct ).mastery_value();

    if ( affected_by.spirit_bond.direct )
    {
      double bonus = o()->cache.mastery() * o()->mastery.spirit_bond->effectN( affected_by.spirit_bond.direct ).mastery_value();
      bonus *= 1 + o()->mastery.spirit_bond_buff->effectN( 1 ).percent();
      am *= 1 + bonus;
    }

    if ( affected_by.mongoose_fury.direct && o()->buffs.mongoose_fury->check() )
    {
      /* 2026-01-17: Strike as One has a unique spell effect in Mongoose Fury's buff, conditioned on a talent (Bloody Claws).
                     So use that for the special case. */
      if ( s->action->name_str == "strike_as_one" || s->action->name_str == "strike_as_one_swipe" )
      {
        am *= 1 + o()->talents.mongoose_fury_buff->effectN( 2 ).percent() * o()->buffs.mongoose_fury->stack();
      }
      else
      {
        am *= 1 + o()->buffs.mongoose_fury->check_stack_value();
      }
    }

    if ( affected_by.tip_of_the_spear.direct && o()->buffs.tip_of_the_spear->check() )
      am *= 1 + o()->talents.tip_of_the_spear_buff->effectN( 1 ).percent();

    if ( affected_by.wyverns_cry.direct )
      am *= 1 + o()->buffs.wyverns_cry->check_stack_value();

    return am;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double am = ab::composite_ta_multiplier( s );

    if ( affected_by.bestial_wrath.tick )
      am *= 1 + o()->buffs.bestial_wrath->check_value();

    if ( affected_by.master_of_beasts.tick )
      am *= 1 + o()->cache.mastery() * o()->mastery.master_of_beasts->effectN( affected_by.master_of_beasts.tick ).mastery_value();

    if ( affected_by.spirit_bond.tick )
    {
      double bonus = o()->cache.mastery() * o()->mastery.spirit_bond->effectN( affected_by.spirit_bond.tick ).mastery_value();
      bonus *= 1 + o()->mastery.spirit_bond_buff->effectN( 3 ).percent();
      am *= 1 + bonus;
    }

    if ( affected_by.wyverns_cry.tick )
      am *= 1 + o()->buffs.wyverns_cry->check_stack_value();

    return am;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double cm = ab::composite_crit_damage_bonus_multiplier();

    if ( affected_by.stargazer && o()->buffs.stargazer->check() )
      cm *= 1 + o()->buffs.stargazer->stack_value();

    return cm;
  }

  double composite_target_da_multiplier( player_t* target ) const override
  {
    double da = ab::composite_target_da_multiplier( target );

    if ( affected_by.unnatural_causes.direct )
    {
      da *= 1 + o()->talents.unnatural_causes->effectN( 1 ).percent();

      if ( target->health_percentage() < o()->talents.unnatural_causes->effectN( 3 ).base_value() )
        da *= 1.0476;
    }

    return da;
  }

  double composite_target_ta_multiplier( player_t* target ) const override
  {
    double ta = ab::composite_target_ta_multiplier( target );

    if ( affected_by.unnatural_causes.tick )
    {
      ta *= 1 + o()->talents.unnatural_causes->effectN( 1 ).percent();

      if ( target->health_percentage() < o()->talents.unnatural_causes->effectN( 3 ).base_value() )
        ta *= 1.0476;
    }

    return ta;
  }

  void tick( dot_t* dot ) override
  {
    ab::tick( dot );

    // 2026-04-03: Dire Beast seems to start it's internal cooldown on any bleed tick if the ICD isn't already running regardless of proc or not.
    if ( o()->cooldowns.dire_beast->up() && dire_beast_chance > 0 )
    {
      o()->cooldowns.dire_beast->start();
      if ( o()->rng().roll( dire_beast_chance ) )
      {
        o()->spawn_dire_beast( o()->talents.dire_beast_summon->duration() );
      }
    }
  }

  T_PET* p() { return static_cast<T_PET*>( ab::player ); }
  const T_PET* p() const { return static_cast<T_PET*>( ab::player ); }

  hunter_t* o() { return p()->o(); }
  const hunter_t* o() const { return p()->o(); }

  bool usable_moving() const override { return true; }
};

template <typename Pet>
struct hunter_pet_attack_t: public hunter_pet_action_t<Pet, melee_attack_t>
{
private:
  using ab = hunter_pet_action_t<Pet, melee_attack_t>;
public:

  hunter_pet_attack_t( util::string_view n, Pet* p, const spell_data_t* s ) : ab( n, p, s ) {}
};

template <typename Pet>
struct hunter_pet_melee_t: public hunter_pet_action_t<Pet, melee_attack_t>
{
private:
  using ab = hunter_pet_action_t<Pet, melee_attack_t>;
public:

  hunter_pet_melee_t( util::string_view n, Pet* p ):
    ab( n, p )
  {
    ab::background = ab::repeating = true;
    ab::special = false;

    ab::weapon = &( p->main_hand_weapon );
    ab::weapon_multiplier = 1;

    ab::base_execute_time = ab::weapon->swing_time;
    ab::school = SCHOOL_PHYSICAL;
    ab::may_crit = true;
  }

  timespan_t execute_time() const override
  {
    // There is a cap of ~.25s for pet auto attacks
    timespan_t t = ab::execute_time();
    if ( t < 0.25_s )
      t = 0.25_s;
    return t;
  }
};

// ==========================================================================
// Hunter Pet Attacks
// ==========================================================================

// Kill Command ============================================================

struct kill_command_bm_t: public hunter_pet_attack_t<hunter_main_pet_base_t>
{
  kill_command_bm_t( hunter_main_pet_base_t* p, const spell_data_t* s ) : hunter_pet_attack_t( "kill_command", p, s )
  {
    background = dual = proc = true;
  }

  void execute() override
  {
    hunter_pet_attack_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    hunter_pet_attack_t::impact( s );

    if ( o() -> talents.kill_cleave.ok() && s -> action -> result_is_hit( s -> result ) &&
      s -> action -> sim -> active_enemies > 1 && p() -> hunter_pet_t::buffs.beast_cleave -> up() )
    {
      // Target multipliers do not replicate to secondary targets
      const double target_da_multiplier = ( 1.0 / s->target_da_multiplier );
      const double target_pet_multiplier = ( 1.0 / s->target_pet_multiplier );

      const double amount = s->result_total * o()->talents.kill_cleave->effectN( 1 ).percent() * target_da_multiplier * target_pet_multiplier;
      p()->actions.kill_cleave->execute_on_target( s->target, amount );
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double da = hunter_pet_attack_t::composite_da_multiplier( s );

    // TODO 16/2/25: Alpha Predator and Pack Mentality are being combined additively before being applied.
    // 2026-01-18:   Still present.
    double bonus = o()->talents.alpha_predator->effectN( 2 ).percent();

    if ( o()->buffs.howl_of_the_pack_leader_wyvern->check()
      || o()->buffs.howl_of_the_pack_leader_boar->check()
      || o()->buffs.howl_of_the_pack_leader_bear->check() )
    {
      bonus += o()->talents.pack_mentality->effectN( 1 ).percent();
    }

    da *= 1 + bonus;

    if ( o()->buffs.natures_ally_3->check() )
      da *= 1 + o()->talents.natures_ally_3_buff->effectN( 1 ).percent();

    return da;
  }
  
  double composite_crit_damage_bonus_multiplier() const override
  {
    double cm = hunter_pet_attack_t::composite_crit_damage_bonus_multiplier();

    if ( o() -> talents.go_for_the_throat.ok() )
    {
      cm *= 1 + o() -> talents.go_for_the_throat -> effectN( 2 ).percent() * o() -> cache.attack_crit_chance();
    }

    return cm;
  }
};

struct kill_command_sv_t : public hunter_pet_attack_t<hunter_main_pet_t>
{
  kill_command_sv_t( hunter_main_pet_t* p ) : hunter_pet_attack_t( "kill_command", p, p->o()->talents.kill_command_sv_pet ) {}

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double da = hunter_pet_attack_t::composite_da_multiplier( s );

    if ( o()->buffs.howl_of_the_pack_leader_wyvern->check()
      || o()->buffs.howl_of_the_pack_leader_boar->check()
      || o()->buffs.howl_of_the_pack_leader_bear->check() )
    {
      da *= o()->talents.pack_mentality->effectN( 1 ).percent();
    }

    return da;
  }
  
  void trigger_dot( action_state_t* s ) override
  {
    hunter_pet_attack_t::trigger_dot( s );

    o() -> trigger_bloodseeker_update();
  }

  void last_tick( dot_t* d ) override
  {
    hunter_pet_attack_t::last_tick( d );

    o() -> trigger_bloodseeker_update();
  }
};

// Wild Thrash ===============================================================

struct wild_thrash_t : public hunter_pet_attack_t<hunter_pet_t>
{
  wild_thrash_t( hunter_main_pet_base_t* p, const spell_data_t* s ) : hunter_pet_attack_t( "wild_thrash", p, s )
  {
    background = dual = proc = true;
    aoe = -1;
    reduced_aoe_targets      = data().effectN( 2 ).base_value();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double dm = hunter_pet_attack_t::composite_da_multiplier( s );

    if ( s->n_targets >= o()->talents.wild_thrash_player->effectN( 2 ).base_value() )
      dm *= 1 + o()->talents.wild_thrash_player->effectN( 1 ).percent();

    return dm;
  }
};

// Sic 'Em ===================================================================

struct sic_em_t : public hunter_pet_attack_t<hunter_main_pet_t>
{
  sic_em_t( hunter_main_pet_t* p ) : hunter_pet_attack_t( "sic_em", p, p->o()->talents.sic_em_bleed )
  {
    background = dual = true;

    auto sao = p->find_action( "strike_as_one" );
    if ( sao )
      sao->add_child( this );
  }
};

// Strike as One =============================================================

struct strike_as_one_t : public hunter_pet_attack_t<hunter_main_pet_t>
{
  strike_as_one_t( hunter_main_pet_t* p, double effectiveness = 1.0 ) : hunter_pet_attack_t( "strike_as_one", p, p->o()->talents.strike_as_one_dmg )
  {
    background = dual = true;
    base_dd_multiplier *= effectiveness;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double dm = hunter_pet_attack_t::composite_da_multiplier( s );

    if ( s->n_targets > 1 )
      dm *= 1 + ( s->n_targets - 1 ) * o()->talents.two_against_many->effectN( 2 ).percent();

    return dm;
  }

  void impact( action_state_t* s ) override
  {
    hunter_pet_attack_t::impact( s );

    if ( o()->talents.sic_em.ok() && s->result == RESULT_CRIT )
      p()->actions.sic_em->execute_on_target( s->target );
  }
};

// Beast Cleave ==============================================================

struct beast_cleave_attack_t: public hunter_pet_attack_t<hunter_pet_t>
{
  beast_cleave_attack_t( hunter_pet_t* p ) : hunter_pet_attack_t( "beast_cleave", p, p->find_spell( 118459 ) )
  {
    background = true;
    callbacks = proc = false;
    may_miss = may_crit = false;
    // The starting damage includes all the buffs
    base_dd_min = base_dd_max = 0;
    spell_power_mod.direct = attack_power_mod.direct = 0;
    weapon_multiplier = 0;

    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();
    target_filter_callback = secondary_targets_only();
  }

  void init() override
  {
    hunter_pet_attack_t::init();

    snapshot_flags |= STATE_TGT_MUL_DA;
    snapshot_flags |= STATE_TGT_MUL_PET;
  }
};

// Kill Cleave ==============================================================

struct kill_cleave_t: public hunter_pet_attack_t<hunter_pet_t>
{
  kill_cleave_t( hunter_pet_t* p ) : hunter_pet_attack_t( "kill_cleave", p, p->find_spell( 389448 ) )
  {
    background = true;
    callbacks = proc = false;
    may_miss = may_crit = false;
    // The starting damage includes all the buffs
    base_dd_min = base_dd_max = 0;
    spell_power_mod.direct = attack_power_mod.direct = 0;
    weapon_multiplier = 0;

    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();
    target_filter_callback = secondary_targets_only();
  }

  void init() override
  {
    hunter_pet_attack_t::init();

    snapshot_flags |= STATE_TGT_MUL_DA;
    snapshot_flags |= STATE_TGT_MUL_PET;
  }
};

// Melee ================================================================

struct pet_melee_t : public hunter_pet_melee_t<hunter_pet_t>
{
  pet_melee_t( util::string_view n, hunter_pet_t* p ) : hunter_pet_melee_t( n, p ) {}

  void impact( action_state_t* s ) override
  {
    hunter_pet_melee_t::impact( s );

    trigger_beast_cleave( s );
  }
};

struct main_pet_base_melee_t : public hunter_pet_melee_t<hunter_main_pet_base_t>
{
  struct wildfire_imbuement_t : public hunter_pet_attack_t<hunter_main_pet_base_t>
  {
    wildfire_imbuement_t( hunter_main_pet_base_t* p )
      : hunter_pet_attack_t( "wildfire_imbuement", p, p->o()->talents.wildfire_imbuement_dmg )
    {
      background = dual = true;
    }
  };

  wildfire_imbuement_t* wildfire_imbuement = nullptr;

  main_pet_base_melee_t( util::string_view n, hunter_main_pet_base_t* p )
    : hunter_pet_melee_t( n, p )
  {
    if ( o()->talents.wildfire_imbuement.ok() )
    {
      wildfire_imbuement = new wildfire_imbuement_t( p );
      add_child( wildfire_imbuement );
    }
  }

  void execute() override
  {
    hunter_pet_melee_t::execute();

    if ( o()->buffs.wildfire_imbuement->check() )
      wildfire_imbuement->execute_on_target( target );
  }

  void impact( action_state_t* s ) override
  {
    hunter_pet_melee_t::impact( s );

    trigger_beast_cleave( s );

    if ( o()->buffs.wyverns_cry->check() )
      o()->buffs.wyverns_cry->increment( 1, buff_t::DEFAULT_VALUE(), o()->buffs.wyverns_cry->remains() );
  }
};

// Claw/Bite/Smack ======================================================

struct basic_attack_base_t : public hunter_pet_attack_t<hunter_main_pet_t>
{
  basic_attack_base_t( hunter_main_pet_t* p, util::string_view n, util::string_view suffix ) : 
    hunter_pet_attack_t( fmt::format("{}{}", n, suffix), p, p->find_pet_spell( n ) )
  {
    school = SCHOOL_PHYSICAL;
  }

  void impact( action_state_t* s ) override
  {
    hunter_pet_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      trigger_beast_cleave( s );
  }
};

struct basic_attack_main_t final : public basic_attack_base_t
{
  struct {
    double cost_pct = 0;
    double multiplier = 1;
    benefit_t* benefit = nullptr;
  } wild_hunt;

  basic_attack_main_t( hunter_main_pet_t* p, util::string_view n, util::string_view options_str ) :
    basic_attack_base_t( p, n, "" )
  {
    parse_options( options_str );

    auto wild_hunt_spell = p -> find_spell( 62762 );
    wild_hunt.cost_pct = 1 + wild_hunt_spell -> effectN( 2 ).percent();
    wild_hunt.multiplier = 1 + wild_hunt_spell -> effectN( 1 ).percent();
    wild_hunt.benefit = p -> get_benefit( "wild_hunt" );

    p->actions.basic_attack = this;

    if ( p->actions.brutal_companion_ba )
      add_child( p->actions.brutal_companion_ba );
  }

  bool use_wild_hunt() const
  {
    return p() -> resources.current[RESOURCE_FOCUS] > 50;
  }

  double action_multiplier() const override
  {
    double am = basic_attack_base_t::action_multiplier();

    const bool used_wild_hunt = use_wild_hunt();
    if ( used_wild_hunt )
      am *= wild_hunt.multiplier;
    wild_hunt.benefit -> update( used_wild_hunt );

    return am;
  }

  double cost_pct_multiplier() const override
  {
    double c = basic_attack_base_t::cost_pct_multiplier();

    if ( use_wild_hunt() )
      c *= wild_hunt.cost_pct;

    return c;
  }
};

struct brutal_companion_ba_t : public basic_attack_base_t
{
  brutal_companion_ba_t( hunter_main_pet_t* p, util::string_view n ) : basic_attack_base_t( p, n, "_brutal_companion" )
  {
    background = proc = true;
    base_multiplier *= 1 + o()->talents.brutal_companion->effectN( 2 ).percent();
  }
};

// Takedown ===============================================================

struct takedown_t : public hunter_pet_attack_t<hunter_main_pet_t>
{
  takedown_t( hunter_main_pet_t* p ) : hunter_pet_attack_t( "takedown", p, p->o()->talents.takedown_pet )
  {
    background = true;

    // 2026-01-17: Takedown's pet damage spell effect has an energize baked in which is not reflected in-game. 
    energize_type = action_energize::NONE;
  }
};

// Stomp ==================================================================

struct stomp_t : public hunter_pet_attack_t<hunter_pet_t>
{
  bool thundering_hooves = false;

  stomp_t( hunter_pet_t* p, util::string_view n = "stomp", bool is_thundering_hooves = false ) 
    : hunter_pet_attack_t( n, p, p->o()->talents.stomp_dmg )
  {
    background = true;
    aoe = -1;
    thundering_hooves = is_thundering_hooves;
    base_dd_multiplier *= thundering_hooves ? o()->talents.thundering_hooves->effectN( 1 ).percent() : 1.0;
  };

  void execute() override
  {
    hunter_pet_attack_t::execute();

    if ( o()->talents.wild_instincts.ok() )
    {
      auto tl = target_list();

      if ( p() == o()->pets.main )
      {
        // Prioritise targets without Barbed Shot ticking.
        // Thundering Hooves stomps can trigger Wild Instincts on the primary target.
        range::erase_remove( tl, [ this ]( player_t* t ) {
          return ( !thundering_hooves && t == target ) || o()->get_target_data( t )->dots.barbed_shot->is_ticking();
        } );
        target_cache.is_valid = false;

        if ( !tl.empty() )
          o()->actions.wild_instincts->execute_on_target( tl.front() );
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    hunter_pet_attack_t::impact( s );

    /* Wild Instincts edge case in ST where the target, if unaffected by 
       Barbed Shot, can receive a double Barbed Shot. */
    if ( o()->talents.wild_instincts.ok() && target_list().size() == 1 )
      if ( p() == o()->pets.main && !o()->get_target_data( s->target )->dots.barbed_shot->is_ticking() )
        o()->actions.wild_instincts->execute_on_target( s->target );
  }
};

// Bloodshed ===============================================================

struct bloodshed_t : hunter_pet_attack_t<hunter_main_pet_base_t>
{
  bloodshed_t( hunter_main_pet_base_t* p ) : hunter_pet_attack_t( "bloodshed", p, p->o()->talents.bloodshed_dot )
  {
    background = true;

    // 2026-01-31: Bloodshed is affected by Unnatural Causes but this is not reflected in spell data.
    if ( o()->bugs )
      affected_by.unnatural_causes.tick = as<uint8_t>( 2 );
  }

  void init() override
  {
    hunter_pet_attack_t::init();

    // 2026-02-12: Bloodshed is bugged and cannot proc Dire Beasts.
    if ( o()->bugs )
    {
      dire_beast_chance = 0;
      sim->print_debug( "{} action {} updated to trigger Dire Beast with {}% chance", player->name(), name(), dire_beast_chance * 100 );
    }
  }
};

// Bestial Wrath ===========================================================

struct bestial_wrath_t : hunter_pet_attack_t<hunter_main_pet_base_t>
{
  bestial_wrath_t( hunter_main_pet_base_t* p ) : hunter_pet_attack_t( "bestial_wrath", p, p->find_spell( 344572 ) )
  {
    background = true;
  }

  void impact( action_state_t* s ) override
  {
    hunter_pet_attack_t::impact( s );

    trigger_beast_cleave( s );
  }
};

// Kill Command - Wildspeaker ( Dire Beast ) ===============================

struct kill_command_wildspeaker_t: public hunter_pet_attack_t<dire_critter_t>
{
  kill_command_wildspeaker_t( dire_critter_t* p ) : hunter_pet_attack_t( "kill_command", p, p->o()->talents.wildspeaker_kill_command )
  {
    background = dual = proc = true;
  }

  void impact( action_state_t* s ) override
  {
    hunter_pet_attack_t::impact( s );

    if ( o()->talents.kill_cleave.ok() && s->action->result_is_hit( s->result ) &&
      s->action->sim->active_enemies > 1 && p()->hunter_pet_t::buffs.beast_cleave->up() )
    {
      // Target multipliers do not replicate to secondary targets
      const double target_da_multiplier = ( 1.0 / s->target_da_multiplier );
      const double target_pet_multiplier = ( 1.0 / s->target_pet_multiplier );

      const double amount = s->result_total * o()->talents.kill_cleave->effectN( 1 ).percent() * target_da_multiplier * target_pet_multiplier;
      // Damage is represented as Beast Cleave
      p()->hunter_pet_t::actions.beast_cleave->execute_on_target( s->target, amount );
    }
  }
  
  double composite_crit_damage_bonus_multiplier() const override
  {
    double cm = hunter_pet_attack_t::composite_crit_damage_bonus_multiplier();

    if ( o() -> talents.go_for_the_throat.ok() )
    {
      cm *= 1 + o() -> talents.go_for_the_throat -> effectN( 2 ).percent() * o() -> cache.attack_crit_chance();
    }

    return cm;
  }
};

// Ravenous Leap (Fenryr) ===================================================

struct ravenous_leap_t : public hunter_pet_attack_t<fenryr_t>
{
  ravenous_leap_t( fenryr_t* p ) : hunter_pet_attack_t( "ravenous_leap", p, p->find_spell( 459753 ) )
  {
    background = true;
    dire_beast_chance = -1;
  }
};

// Rend Flesh (Bear) ===================================================

struct rend_flesh_t : public hunter_pet_attack_t<bear_t>
{
  rend_flesh_t( bear_t* p ) : hunter_pet_attack_t( "rend_flesh", p, p->o()->talents.howl_of_the_pack_leader_bear_bleed )
  {
    background = true;
    aoe = as<int>( data().effectN( 2 ).base_value() );
    dire_beast_chance = -1;
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( !t )
      t = target;
    if ( !t )
      return nullptr;

    return p()->get_target_data( t )->dots.rend_flesh;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double am = hunter_pet_attack_t::composite_ta_multiplier( s );

    // 2026-02-07: Rend Flesh is double-dipping Spirit Bond's modifier.
    if ( o()->mastery.spirit_bond.ok() )
    {
      double bonus = o()->cache.mastery() * o()->mastery.spirit_bond->effectN( affected_by.spirit_bond.tick ).mastery_value();
      bonus *= 1 + o()->mastery.spirit_bond_buff->effectN( 3 ).percent();
      am *= 1 + bonus;
    }

    return am;
  }

  void impact( action_state_t* s ) override
  {
    hunter_pet_attack_t::impact( s );
  }
};

// Shoot (Dark Minion) =============================================================

struct shoot_t final : public hunter_pet_attack_t<dark_minion_t>
{
  shoot_t( dark_minion_t* p ) : hunter_pet_attack_t( "shoot", p, p->find_spell( 1264357 ) ) 
  {
    /* 2026-01-25: The pet stands around for a variable amount of time between casts.
                   Log testing puts it between 350ms and 650ms but longer testing required. */
    cooldown->duration = rng().range( 350_ms, 650_ms );
  }
};

// Blighted Arrow (Dark Minion) ====================================================

struct blighted_arrow_t final : public hunter_pet_attack_t<dark_minion_t>
{
  blighted_arrow_t( dark_minion_t* p ) : hunter_pet_attack_t( "blighted_arrow", p, p->find_spell( 1264364 ) )
  {
    background = true;
    aoe = -1;
  }
};

// Shadow Thrash (Dark Hound) ======================================================

struct shadow_thrash_t final : public hunter_pet_attack_t<dark_hound_t>
{
  shadow_thrash_t( dark_hound_t* p ) : hunter_pet_attack_t( "shadow_thrash", p, p->find_spell( 1264485 ) )
  {
    background = true;
    aoe = data().max_targets();
  }
};

} // end namespace pets::actions

fenryr_td_t::fenryr_td_t( player_t* target, fenryr_t* p ) : actor_target_data_t( target, p ), dots()
{
  dots.ravenous_leap = target->get_dot( "ravenous_leap", p );
};

bear_td_t::bear_td_t( player_t* target, bear_t* p ) : actor_target_data_t( target, p ), dots()
{
  dots.rend_flesh = target->get_dot( "rend_flesh", p );
};

hunter_main_pet_base_td_t::hunter_main_pet_base_td_t( player_t* target, hunter_main_pet_base_t* p ) : actor_target_data_t( target, p ), dots()
{
  dots.bloodshed = target->get_dot( "bloodshed", p );
}

hunter_main_pet_td_t::hunter_main_pet_td_t( player_t* target, hunter_main_pet_t* p ) : hunter_main_pet_base_td_t( target, p ) {}

action_t* hunter_main_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "claw" ) return new        actions::basic_attack_main_t( this, "Claw", options_str );
  if ( name == "bite" ) return new        actions::basic_attack_main_t( this, "Bite", options_str );
  if ( name == "smack" ) return new       actions::basic_attack_main_t( this, "Smack", options_str );

  return hunter_main_pet_base_t::create_action( name, options_str );
}

void hunter_pet_t::init_spells()
{
  pet_t::init_spells();

  if ( !main_hand_attack )
    main_hand_attack = new actions::pet_melee_t( "melee", this );
  
  actions.beast_cleave = new actions::beast_cleave_attack_t( this );
}

void stable_pet_t::init_spells()
{
  hunter_pet_t::init_spells();

  if ( o()->talents.thundering_hooves.ok() )
    actions.thundering_hooves = new actions::stomp_t( this, "thundering_hooves", o()->talents.thundering_hooves.ok() );
}

void hunter_main_pet_base_t::init_spells()
{
  main_hand_attack = new actions::main_pet_base_melee_t( "melee", this );

  stable_pet_t::init_spells();

  if ( o()->specialization() == HUNTER_BEAST_MASTERY )
  {
    actions.kill_command  = new actions::kill_command_bm_t( this, o()->talents.kill_command_bm_pet );
    actions.bestial_wrath = new actions::bestial_wrath_t( this );
    actions.wild_thrash   = new actions::wild_thrash_t( this, o()->talents.wild_thrash_pet );
    
    if ( o() -> talents.kill_cleave.ok() )
      actions.kill_cleave = new actions::kill_cleave_t( this );

    if ( !stable_pet_t::actions.stomp && o()->talents.stomp.ok() )
      stable_pet_t::actions.stomp = new actions::stomp_t( this );

    if ( o()->talents.bloodshed.ok() )
      actions.bloodshed = new actions::bloodshed_t( this );
  }
}

void animal_companion_t::init_spells()
{
  hunter_main_pet_base_t::init_spells();
}

void hunter_main_pet_t::init_spells()
{
  hunter_main_pet_base_t::init_spells();

  if ( o()->specialization() == HUNTER_SURVIVAL )
  {
    hunter_main_pet_base_t::actions.kill_command = new actions::kill_command_sv_t( this );

    if ( o()->talents.takedown.ok() )
      actions.takedown = new actions::takedown_t( this );

    if ( o()->talents.strike_as_one.ok() )
      actions.strike_as_one = new actions::strike_as_one_t( this );

    if ( o()->talents.raptor_swipe_3.ok() && o()->talents.strike_as_one.ok() )
      actions.strike_as_one_swipe = new actions::strike_as_one_t( this, o()->talents.raptor_swipe_3->effectN( 2 ).percent() );

    if ( o()->talents.sic_em.ok() )
      actions.sic_em = new actions::sic_em_t( this );
  }
  else if ( o()->specialization() == HUNTER_BEAST_MASTERY )
  {
    if ( o()->talents.brutal_companion.ok() )
      actions.brutal_companion_ba = new actions::brutal_companion_ba_t( this, "Claw" );
  }
}

void dire_critter_t::init_spells()
{
  hunter_pet_t::init_spells();

  if ( o() -> talents.wildspeaker.ok() )
    actions.kill_command = new actions::kill_command_wildspeaker_t( this );
}

void dark_hound_t::init_spells()
{
  dire_critter_t::init_spells();

  main_hand_attack->school = SCHOOL_SHADOW;

  actions.shadow_thrash = new actions::shadow_thrash_t( this );
}

void dark_minion_t::init_spells()
{ 
  // Calling pet_t's init_spells() to skip the autoattack setup
  pet_t::init_spells();

  actions.blighted_arrow = new actions::blighted_arrow_t( this );
}

action_t* dark_minion_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "shoot" )
  {
    actions.shoot = new actions::shoot_t( this );
    return actions.shoot;
  }

  return hunter_pet_t::create_action( name, options_str );
}

void fenryr_t::init_spells()
{
  dire_critter_t::init_spells();

  actions.ravenous_leap = new actions::ravenous_leap_t( this );
}

void bear_t::init_spells()
{
  dire_critter_t::init_spells();

  actions.rend_flesh = new actions::rend_flesh_t( this );
}

void hunter_main_pet_base_t::init_special_effects()
{
  stable_pet_t::init_special_effects();

  if ( o() -> talents.laceration.ok() )
  {
    struct laceration_cb_t : public dbc_proc_callback_t
    {
      double bleed_amount; 
      action_t* bleed; 

      laceration_cb_t( const special_effect_t& e, double amount, action_t* bleed ) : dbc_proc_callback_t( e.player, e ),
        bleed_amount( amount ), bleed( bleed )
      {
      }

      void execute( const spell_data_t*, player_t* t, action_state_t* s ) override
      {
        if ( t && t->is_sleeping() )
          return;

        if ( s )
        {
          double amount = s->result_amount * bleed_amount;
          if ( amount > 0 )
            residual_action::trigger( bleed, t, amount );
        }
      }
    };

    auto const effect = new special_effect_t( this );
    effect -> name_str = "laceration";
    effect -> spell_id =  o()->talents.laceration_driver->id();
    effect -> proc_flags2_ = PF2_CRIT;
    special_effects.push_back( effect );

    auto cb = new laceration_cb_t( *effect, o()->talents.laceration_driver->effectN( 1 ).percent(), o()->actions.laceration );
    cb -> initialize();
  }
}

template <typename Pet, size_t N>
struct active_pets_t
{
  using data_t = std::array<Pet*, N>;

  data_t data_;
  size_t active_;

  active_pets_t( data_t d, size_t n ):
    data_( d ), active_( n )
  {}

  auto begin() const { return data_.begin(); }
  auto end() const { return data_.begin() + active_; }
};

// returns the active pets from the list 'cast' to the supplied pet type
template <typename Pet, typename... Pets>
auto active( Pets... pets_ ) -> active_pets_t<Pet, sizeof...(Pets)>
{
  Pet* pets[] = { pets_... };
  typename active_pets_t<Pet, sizeof...(Pets)>::data_t active_pets{};
  size_t active_pet_count = 0;
  for ( auto pet : pets )
  {
    if ( pet && ! pet -> is_sleeping() )
      active_pets[ active_pet_count++ ] = pet;
  }

  return { active_pets, active_pet_count };
}

} // end namespace pets

namespace events {

struct tar_trap_aoe_t : public event_t
{
  hunter_t* p;
  double x_position, y_position;

  tar_trap_aoe_t( hunter_t* p, player_t* target, timespan_t t ) :
    event_t( *p -> sim, t ), p( p ),
    x_position( target -> x_position ), y_position( target -> y_position )
  { }

  const char* name() const override
  { return "Hunter-TarTrap-Aoe"; }

  void execute() override
  {
    if ( p -> state.tar_trap_aoe == this )
      p -> state.tar_trap_aoe = nullptr;
    p -> sim -> print_debug( "{} Tar Trap at {:.3f}:{:.3f} expired ({})", p -> name(), x_position, y_position, *this );
  }
};

} // end namespace events

namespace buffs {
} // end namespace buffs

void hunter_t::trigger_bloodseeker_update()
{
  if ( !talents.bloodseeker.ok() )
    return;

  int bleeding_targets = 0;
  for ( const player_t* t : sim -> target_non_sleeping_list )
  {
    // TODO: This is in fact insufficient, as debuffs.bleeding is solely based on physical dots being applied or removed or forced with overrides.bleeding,
    // whereas it's been demonstrated with bleeds lacking other qualities, e.g., Rend Flesh before its fix to properly ignore armor in 11.2, to us apparently
    // accomplished by adding the bleed mechanic flags to both the spell and the dot effect, not applying a stack of Bloodseeker, that more stringent requirements
    // exist for Bloodseeker to count the target as "bleeding". This requirement is also apparent with the new Dire Beast behavior of 11.2, neither for which 
    // Rend Flesh was an eligible bleed until the aforesaid fix. This is further complicated by Flayed Shot, a shadow damage dot that is yet referred to as a 
    // bleed in its descriptions and has the bleed mechanic set on the ability (but not the dot effect), working as a valid candidate for Bloodseeker (though 
    // seemingly still not a candidate for 11.2 Dire Beast).
    if ( t -> is_enemy() && t -> debuffs.bleeding -> check() )
      bleeding_targets++;
  }
  bleeding_targets = std::min( bleeding_targets, buffs.bloodseeker -> max_stack() );

  const int current = buffs.bloodseeker -> check();
  if ( current < bleeding_targets )
  {
    buffs.bloodseeker -> trigger( bleeding_targets - current );
    if ( auto pet = pets.main )
      pet -> buffs.bloodseeker -> trigger( bleeding_targets - current );
  }
  else if ( current > bleeding_targets )
  {
    buffs.bloodseeker -> decrement( current - bleeding_targets );
    if ( auto pet = pets.main )
      pet -> buffs.bloodseeker -> decrement( current - bleeding_targets );
  }
}

// Currently only relevant for Survival's Outland Venom.
int hunter_t::ticking_dots( hunter_td_t* td )
{
  int dots = 0;

  auto hunter_dots = td->dots;
  dots += hunter_dots.wildfire_bomb->is_ticking();
  dots += hunter_dots.sanctified_armaments->is_ticking();

  return dots;
}

void hunter_t::trigger_outland_venom_update()
{
  if ( !talents.outland_venom.ok() )
    return;

  for ( player_t* t : sim->target_non_sleeping_list )
  {
    if ( t->is_enemy() )
    {
      auto td = get_target_data( t );
      int current = td->debuffs.outland_venom->check();
      int new_stacks = ticking_dots( td );

      new_stacks = std::min( new_stacks, td->debuffs.outland_venom->max_stack() );

      if ( current < new_stacks )
        td->debuffs.outland_venom->trigger( new_stacks - current );
      else if ( current > new_stacks )
        td->debuffs.outland_venom->decrement( current - new_stacks );
    }
  }
}

void hunter_t::consume_trick_shots()
{
  if ( buffs.volley -> up() )
    return;

  buffs.trick_shots -> decrement();
}

void hunter_t::consume_precise_shots()
{
  if ( !buffs.precise_shots->check() )
    return;

  cooldowns.aimed_shot->adjust( -talents.focused_aim->effectN( 1 ).time_value() );

  buffs.precise_shots->expire();
  buffs.stargazer->trigger();

  if ( rng().roll( talents.windrunner_quiver->effectN( 3 ).percent() ) )
  {
    buffs.precise_shots->trigger();
    procs.windrunner_quiver->occur();
  }
}

void hunter_t::trigger_eagles_mark( player_t* target, bool sentinel, bool force )
{
  if ( !talents.sentinel.ok() && !specs.spotters_mark_data.ok() )
    return;

  if ( force )
  {
    auto td = get_target_data( target );
    sentinel ? td->debuffs.sentinels_mark->trigger() : td->debuffs.spotters_mark->trigger();

    procs.eagles_mark->occur();

    cooldowns.aimed_shot->adjust( -talents.moons_blessing->effectN( 2 ).time_value() );
    cooldowns.wildfire_bomb->adjust( -talents.moons_blessing->effectN( 3 ).time_value() );

    return;
  }

  auto spec = specialization();
  double chance = 0;
  double lunar_calling_bonus = talents.lunar_calling->effectN( spec == HUNTER_MARKSMANSHIP ? 1 : 2 ).percent();

  if ( spec == HUNTER_MARKSMANSHIP )
  {
    chance += specs.spotters_mark_data->effectN( 1 ).percent();

    // 2026-01-15: Moon's Blessing spell data is applied to Survival but not Marksmanship, so do it manually.
    chance += talents.moons_blessing->effectN( 1 ).percent();

    if ( buffs.trueshot->check() )
    {
      if ( talents.feathered_frenzy.ok() )
        chance *= 1 + talents.feathered_frenzy->effectN( 1 ).percent();

      chance += lunar_calling_bonus;
    }
  }
  else if ( spec == HUNTER_SURVIVAL )
  {
    chance += talents.sentinel->effectN( 1 ).percent();

    if ( buffs.takedown->check() )
      chance += lunar_calling_bonus;
  }

  if ( rng().roll( chance ) )
  {
    auto td = get_target_data( target );
    sentinel ? td->debuffs.sentinels_mark->trigger() : td->debuffs.spotters_mark->trigger();

    procs.eagles_mark->occur();

    cooldowns.aimed_shot->adjust( -talents.moons_blessing->effectN( 2 ).time_value() );
    cooldowns.wildfire_bomb->adjust( -talents.moons_blessing->effectN( 3 ).time_value() );
  }
}

void hunter_t::trigger_huntmasters_call()
{
  if ( !talents.huntmasters_call.ok() )
    return;

  buffs.huntmasters_call->trigger();
  if ( buffs.huntmasters_call->at_max_stacks() )
  {
    buffs.huntmasters_call->expire();
    if ( rng().roll( 0.5 ) )
    {
      buffs.summon_fenryr->trigger();
      pets.fenryr.despawn();
      make_event( sim, [ this ]() { pets.fenryr.spawn( buffs.summon_fenryr->buff_duration() ); } );
    }
    else
    {
      buffs.summon_hati->trigger();
      pets.hati.despawn();
      pets.hati.spawn( buffs.summon_hati->buff_duration() );
    }
  }
}

void hunter_t::trigger_deathblow( bool activated )
{
  if ( !talents.deathblow_buff.ok() )
    return;

  procs.deathblow->occur();
  // Kill Shot/Black Arrow is set up by default to require reacting to Deathblow,
  // and Deathblow by default is set to be reactable and non activated to force reactions and aura delay,
  // so that needs to be temporarily flipped here for the one case it's considered immediately available after pressing Trueshot.
  if ( activated )
  {
    buffs.deathblow->reactable = false;
    buffs.deathblow->activated = true;
    buffs.deathblow->trigger();
    buffs.deathblow->reactable = true;
    buffs.deathblow->activated = false;
  }
  else
  {
    buffs.deathblow->trigger();
  }

  talents.black_arrow.ok() ? cooldowns.black_arrow->reset( !activated ) : cooldowns.kill_shot->reset( !activated );
}

void hunter_t::trigger_lunar_storm( player_t* target )
{
  if ( !talents.lunar_storm.ok() )
    return;

  for ( int i = 1; i <= as<int>( talents.lunar_storm->effectN( 1 ).base_value() ); i++ )
  {
    // No spell data for the Lunar Storm interval, it was ~150ms in testing.
    make_event( sim, 150_ms * i, [ this, target ]() { actions.lunar_storm->execute_on_target( target ); } );
  }
}

bool hunter_t::consume_howl_of_the_pack_leader( player_t* target )
{
  int up = 0;

  if ( buffs.howl_of_the_pack_leader_wyvern->check() )
  {
    up++;
    buffs.wyverns_cry->trigger( as<int>( talents.howl_of_the_pack_leader->effectN( 3 ).base_value() ) );
    buffs.howl_of_the_pack_leader_wyvern->expire();
  }

  if ( buffs.howl_of_the_pack_leader_boar->check() )
  {
    up++;
    actions.boar_charge->execute_on_target( target );
    buffs.howl_of_the_pack_leader_boar->expire();
  }

  if ( buffs.howl_of_the_pack_leader_bear->check() )
  {
    up++;
    pets.bear.spawn( talents.howl_of_the_pack_leader_bear_summon->duration() );

    if ( talents.ursine_fury.ok() )
    {
      for ( int i = 0; i < as<int>( talents.ursine_fury->effectN( 1 ).base_value() ); i++ )
        spawn_dire_beast( talents.dire_beast_summon->duration() );
    }

    buffs.howl_of_the_pack_leader_bear->expire();
  }

  if ( up )
  {
    cooldowns.barbed_shot->adjust( -talents.pack_mentality->effectN( 2 ).time_value() * up );
    cooldowns.wildfire_bomb->adjust( -talents.pack_mentality->effectN( 3 ).time_value() * up );

    if ( buffs.stampede_incoming->check() )
    {
      buffs.stampede_incoming->expire();
      buffs.stampede->trigger();
      actions.stampede->execute_on_target( target );
    }
  }

  return up;
}

void hunter_t::trigger_howl_of_the_pack_leader()
{
  if ( state.howl_of_the_pack_leader_next_beast == WYVERN )
  {
    state.howl_of_the_pack_leader_next_beast = BOAR;
    buffs.howl_of_the_pack_leader_wyvern->trigger();
  }
  else if ( state.howl_of_the_pack_leader_next_beast == BOAR )
  {
    state.howl_of_the_pack_leader_next_beast = BEAR;
    buffs.howl_of_the_pack_leader_boar->trigger();
  }
  else if ( state.howl_of_the_pack_leader_next_beast == BEAR )
  {
    state.howl_of_the_pack_leader_next_beast = WYVERN;
    buffs.howl_of_the_pack_leader_bear->trigger();
  }
}

void hunter_t::trigger_natures_ally_3()
{
  if ( talents.natures_ally_3.ok() )
    buffs.natures_ally_3->trigger();
}

void hunter_t::spawn_dire_beast( timespan_t base_duration, bool force_hound )
{
  util::string_view name = "Dire Beast";

  bool dark_hound = false;

  timespan_t summon_duration  = 0_ms;
  int base_attacks_per_summon = 0;

  if ( talents.corpsecaller_hound_summon.ok() 
    && ( force_hound || rng().roll( talents.corpsecaller->effectN( 1 ).percent() ) ) )
  {
    dark_hound = true;
    name = "Dark Hound";
  }

  std::tie( summon_duration, base_attacks_per_summon ) = pets::dire_beast_duration( this, base_duration );

  if ( dark_hound )
    pets.dark_hound.spawn( summon_duration );
  else
    pets.dire_beast.spawn( summon_duration );

  sim->print_debug( "{} summoned with {} autoattacks", name, base_attacks_per_summon );

  trigger_huntmasters_call();
}

// ==========================================================================
// Hunter Attacks
// ==========================================================================

namespace attacks
{

template <typename Base>
struct auto_attack_base_t : hunter_action_t<Base>
{
private:
  using ab = hunter_action_t<Base>;

public:
  bool first = true;

  auto_attack_base_t( util::string_view n, hunter_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, p, s )
  {
    ab::allow_class_ability_procs = ab::not_a_proc = true;
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

// Auto Shot ================================================================

struct auto_shot_base_t : public auto_attack_base_t<ranged_attack_t>
{
  struct state_t : public action_state_t
  {
    using action_state_t::action_state_t;

    proc_types2 cast_proc_type2() const override
    {
      // Auto Shot seems to trigger Meticulous Scheming
      // (and possibly other effects that care about casts).
      return PROC2_CAST_DAMAGE;
    }
  };

  double snakeskin_quiver_chance = 0;
  double lock_and_load_chance = 0;

  auto_shot_base_t( util::string_view n, hunter_t* p, const spell_data_t* s ) : auto_attack_base_t( n, p, s )
  {
    snakeskin_quiver_chance = p->talents.snakeskin_quiver->effectN( 1 ).percent();
    lock_and_load_chance = p->talents.lock_and_load->effectN( 1 ).percent();
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  void execute() override
  {
    auto_attack_base_t::execute();

    if ( rng().roll( snakeskin_quiver_chance ) )
    {
      p()->procs.snakeskin_quiver->occur();
      p()->actions.snakeskin_quiver->execute_on_target( target );
    }
  }

  void impact( action_state_t* s ) override
  {
    auto_attack_base_t::impact( s );

    if ( rng().roll( lock_and_load_chance ) )
    {
      p()->buffs.lock_and_load->trigger();
    }

    if ( p()->talents.lethal_barbs.ok() )
    {
      double amount = p()->talents.lethal_barbs_energize->effectN( 1 ).base_value();

      p()->resource_gain( RESOURCE_FOCUS, amount, p()->gains.lethal_barbs, this );
      p()->trigger_aura_applied_callbacks( p()->proc_data_entries.lethal_barbs_energize, p() );
      for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p()->pets.main, p()->pets.animal_companion ) )
        pet->resource_gain( RESOURCE_FOCUS, amount, p()->gains.lethal_barbs, this );
    }
  }

  double action_multiplier() const override
  {
    double am = auto_attack_base_t::action_multiplier();

    if ( player -> buffs.heavens_nemesis )
      am *= 1 + player -> buffs.heavens_nemesis -> stack_value();

    return am;
  }
};

struct auto_shot_t : public auto_shot_base_t
{
  auto_shot_t(hunter_t* p) : auto_shot_base_t( "auto_shot", p, p->specs.auto_shot )
  {
  }
};

//==============================
// Shared attacks
//==============================

struct residual_bleed_base_t : public residual_action::residual_periodic_action_t<hunter_ranged_attack_t>
{
  residual_bleed_base_t( util::string_view n, hunter_t* p, const spell_data_t* s )
    : residual_periodic_action_t( n, p, s )
  {
  }

  double base_ta( const action_state_t* s ) const override
  {
    double amount = residual_periodic_action_t::base_ta( s );
    
    if ( affected_by.unnatural_causes.tick )
    {
      amount *= 1 + p()->talents.unnatural_causes->effectN( 1 ).percent();

      if ( target->health_percentage() < p()->talents.unnatural_causes->effectN( 3 ).base_value() )
        amount *= 1.0476;
    }

    return amount;
  }
};

// Steady Shot ========================================================================

struct steady_shot_t: public hunter_ranged_attack_t
{
  steady_shot_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "steady_shot", p, p -> specs.steady_shot )
  {
    parse_options( options_str );

    energize_type = action_energize::ON_CAST;
    energize_resource = RESOURCE_FOCUS;
    energize_amount = p->specs.steady_shot_energize->effectN( 1 ).base_value();

    if ( p->talents.barbed_shot.ok() )
      background = true;
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p()->cooldowns.aimed_shot->adjust( -data().effectN( 2 ).time_value() );
  }
};

// Arcane Shot ========================================================================

struct arcane_shot_base_t: public hunter_ranged_attack_t
{
  struct state_data_t
  {
    bool empowered_by_precise_shots = false;

    friend void sc_format_to( const state_data_t& data, fmt::format_context::iterator out )
    {
      fmt::format_to( out, "empowered_by_precise_shots={}", data.empowered_by_precise_shots );
    }
  };
  using state_t = hunter_action_state_t<state_data_t>;

  arcane_shot_base_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->specs.arcane_shot ) {}

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double am = hunter_ranged_attack_t::composite_da_multiplier( s );

    am *= 1 + p()->buffs.precise_shots->check_stack_value();

    return am;
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( debug_cast<state_t*>( s )->empowered_by_precise_shots )
      p()->trigger_eagles_mark( s->target, p()->talents.sentinel.ok() );
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  double composite_crit_chance() const override
  {
    double cc = hunter_ranged_attack_t::composite_crit_chance();

    if ( p()->talents.critical_precision.ok() && p()->buffs.precise_shots->up() )
    {
      cc += p()->talents.critical_precision->effectN( 1 ).percent();
    }

    return cc;
  }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    hunter_ranged_attack_t::snapshot_internal( s, flags, rt );

    debug_cast<state_t*>( s )->empowered_by_precise_shots = p()->buffs.precise_shots->up();
  }
};

struct arcane_shot_t : public arcane_shot_base_t
{
  struct arcane_shot_aspect_of_the_hydra_t : arcane_shot_base_t
  {
    arcane_shot_aspect_of_the_hydra_t( util::string_view n, hunter_t* p ) : arcane_shot_base_t( n, p )
    {
      background = dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
      base_multiplier *= p->talents.aspect_of_the_hydra->effectN( 1 ).percent();
    }
  };

  arcane_shot_aspect_of_the_hydra_t* aspect_of_the_hydra = nullptr;

  arcane_shot_t( hunter_t* p, util::string_view options_str ) : arcane_shot_base_t( "arcane_shot", p )
  {
    parse_options( options_str );

    if ( p->talents.aspect_of_the_hydra.ok() )
    {
      aspect_of_the_hydra = p->get_background_action<arcane_shot_aspect_of_the_hydra_t>( "arcane_shot_aspect_of_the_hydra" );
      add_child( aspect_of_the_hydra );
    }
  }

  void execute() override
  {
    arcane_shot_base_t::execute();

    auto tl = target_list();
    if ( aspect_of_the_hydra && tl.size() > 1 )
      aspect_of_the_hydra->execute_on_target( tl[ 1 ] );

    p()->consume_precise_shots();
  }

  void impact( action_state_t* s ) override
  {
    arcane_shot_base_t::impact( s );

    if ( debug_cast<state_t*>( s )->empowered_by_precise_shots )
    {
      if ( p()->tier_set.mid_s1_mm_4pc.ok() && p()->rppm.let_fly->trigger() )
        make_event( sim, 300_ms, [ this ]() { p()->actions.let_fly->execute_on_target( target ); } );
    }
  }

  double cost_pct_multiplier() const override
  {
    double c = arcane_shot_base_t::cost_pct_multiplier();

    if ( p()->buffs.precise_shots->check() )
      c *= 1 + p()->talents.precise_shots_buff->effectN( 3 ).percent();

    return c;
  }
};

// Counter Shot (Marksmanship/Beast Mastery Talent) ===========================================================

struct counter_shot_t : public hunter_ranged_attack_t
{
  counter_shot_t( hunter_t* p, util::string_view options_str ) : hunter_ranged_attack_t( "counter_shot", p, p->talents.counter_shot )
  {
    parse_options( options_str );

    may_miss = may_block = may_dodge = may_parry = false;
    is_interrupt = true;
  }

  void impact( action_state_t* s ) override
  {
    if( s->target->debuffs.casting->check() && p()->talents.disruptive_rounds.ok() )
      p()->resource_gain( RESOURCE_FOCUS, p()->talents.disruptive_rounds->effectN( 1 ).base_value(), p()->gains.disruptive_rounds,  this );

    hunter_ranged_attack_t::impact( s );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !candidate_target->debuffs.casting || !candidate_target->debuffs.casting->check() ) return false;
    return hunter_ranged_attack_t::target_ready( candidate_target );
  }
};

// Explosive Shot ===============================================================================
struct explosive_shot_base_t : public hunter_ranged_attack_t
{
  struct damage_t final : hunter_ranged_attack_t
  {
    damage_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->talents.explosive_shot_damage )
    {
      background = dual = true;
      reduced_aoe_targets = p->talents.explosive_shot->effectN( 2 ).base_value();
      aoe = -1;
    }

    void execute() override
    {
      // Need to update here with the damage spell multipliers since the state came from the cast
      // and the execute() call will skip player modifiers when a pre_execute_state exists
      if ( pre_execute_state )
      {
        update_state( pre_execute_state, result_amount_type::DMG_DIRECT );
      }

      hunter_ranged_attack_t::execute();
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = hunter_ranged_attack_t::composite_da_multiplier( s );

      m *= 1 + p()->buffs.precision_detonation_hidden->check_value();

      return m;
    }
  };

  damage_t* explosion = nullptr;

  explosive_shot_base_t( util::string_view n, hunter_t* p, const spell_data_t* s ) : hunter_ranged_attack_t( n, p, s )
  {
    may_miss = may_crit = false;

    explosion = p->get_background_action<damage_t>( "explosive_shot_damage" );
  }

  void init() override
  {
    hunter_ranged_attack_t::init();

    snapshot_flags = STATE_MUL_PERSISTENT;
  }

  void update_state( action_state_t* s, result_amount_type rt ) override
  {
    hunter_ranged_attack_t::update_state( s, rt );

    s->persistent_multiplier = 1.0;
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( !t )
      t = target;
    if ( !t )
      return nullptr;

    return td( t )->dots.explosive_shot;
  }

  void impact( action_state_t* s ) override
  {
    dot_t* dot = td( s->target )->dots.explosive_shot;

    bool refresh = dot->is_ticking();
    if ( refresh )
    {
      if ( !explosion->pre_execute_state )
        explosion->pre_execute_state = explosion->get_state();
      
      // Either of the following scenarios will cause the detonation to have an effectiveness bonus and the last_tick() to have no bonus:
      // - an existing dot applied with an effectiveness bonus being detonated by a normal cast
      // - an existing dot applied by a normal cast being detonated by a cast with an effectiveness bonus
      // There is no way to test if a competing effectiveness bonus would be combined, overwritten, or would carry on to the last_tick(),
      // so just use the effectiveness bonus if it exists then clear the bonus from the dot state.
      if ( s->action->snapshot_flags & STATE_MUL_PERSISTENT )
        dot->state->persistent_multiplier = s->persistent_multiplier;

      explosion->pre_execute_state->copy_state( dot->state );
      explosion->execute_on_target( s->target );
    }

    hunter_ranged_attack_t::impact( s );

    if ( refresh )
      update_state( dot->state, dot->state->result_type );
  }

  void tick( dot_t* ) override
  {
    // Prevent tick() from updating state so it can be used to clear the effectiveness bonus.
  }

  void last_tick( dot_t* d ) override
  {
    hunter_ranged_attack_t::last_tick( d );

    if ( !explosion->pre_execute_state )
      explosion->pre_execute_state = explosion->get_state();

    // The dot should have the state from the cast that triggered it, so forward it to the explosion.
    explosion->pre_execute_state->copy_state( d->state );
    explosion->execute_on_target( d->target );
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( p()->talents.shrapnel_shot.ok() 
        && p()->rng().roll( p()->talents.shrapnel_shot->effectN( 1 ).percent() )
        && p()->cooldowns.shrapnel_shot->up() )
    {
      p()->buffs.lock_and_load->trigger();
      p()->cooldowns.shrapnel_shot->start();
    }
  }
};

struct explosive_shot_t : public explosive_shot_base_t
{
  explosive_shot_t( hunter_t* p, util::string_view options_str ) : explosive_shot_base_t( "explosive_shot", p, p->talents.explosive_shot )
  {
    parse_options( options_str );
  }

  void init() override
  {
    explosive_shot_base_t::init();

    explosion->stats = stats;
    stats->action_list.push_back( explosion );
  }
};

struct explosive_shot_background_t : public explosive_shot_base_t
{
  explosive_shot_background_t( util::string_view n, hunter_t* p, const spell_data_t* s ) : explosive_shot_base_t( n, p, s )
  {
    background = dual = proc = true;
  }
};

// Kill Shot (Hunter Talent) ====================================================================

struct kill_shot_base_t : hunter_ranged_attack_t
{
  double health_threshold_pct;

  struct state_data_t
  {
    bool empowered_by_precise_shots = false;

    friend void sc_format_to( const state_data_t& data, fmt::format_context::iterator out )
    {
      fmt::format_to( out, "empowered_by_precise_shots={}", data.empowered_by_precise_shots );
    }
  };

  using state_t = hunter_action_state_t<state_data_t>;

  kill_shot_base_t( util::string_view n, hunter_t* p, spell_data_ptr_t s ) :
    hunter_ranged_attack_t( n, p, s ),
    health_threshold_pct( p -> talents.kill_shot -> effectN( 2 ).base_value() ) {}

  double cost() const override
  {
    if ( p()->buffs.deathblow->up() )
      return 0;

    return hunter_ranged_attack_t::cost();
  }

  double cost_pct_multiplier() const override
  {
    double c = hunter_ranged_attack_t::cost_pct_multiplier();

    if ( p()->buffs.precise_shots->check() )
      c *= 1 + p()->talents.precise_shots_buff->effectN( 3 ).percent();

    return c;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double am = hunter_ranged_attack_t::composite_da_multiplier( s );
  
    // Can't use check_stack_value() as Kill Shot uses a different effect for its mod
    am *= 1 + ( p()->buffs.precise_shots->check() * p()->talents.precise_shots_buff->effectN( 2 ).percent() );  

    return am;
  }

  double composite_crit_chance() const override
  {
    double cc = hunter_ranged_attack_t::composite_crit_chance();

    if ( p()->talents.critical_precision.ok() && p()->buffs.precise_shots->up() )
    {
      cc += p()->talents.critical_precision->effectN( 1 ).percent();
    }

    return cc;
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    hunter_ranged_attack_t::snapshot_internal( s, flags, rt );

    debug_cast<state_t*>( s )->empowered_by_precise_shots = p()->buffs.precise_shots->up();
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( debug_cast<state_t*>( s )->empowered_by_precise_shots )
      p()->trigger_eagles_mark( s->target, p()->talents.sentinel.ok() );

    if ( p()->talents.headshot.ok() )
      td( s->target )->debuffs.headshot->trigger();
  }

  bool target_ready( player_t* candidate_target ) override
  {
    return hunter_ranged_attack_t::target_ready( candidate_target ) && ( candidate_target->health_percentage() <= health_threshold_pct );
  }

  double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    return am;
  }

  bool ready() override
  {
    // Force the cooldown reset reaction because apparently that was just implemented for apl checks :/
    return hunter_ranged_attack_t::ready() && cooldown->reset_react <= sim->current_time();
  }

  std::unique_ptr<expr_t> create_expression( util::string_view expression_str ) override
  {
    if ( expression_str == "ready" )
    {
      return make_fn_expr( expression_str, [ this ] {
        // Must meet both ready() and target_ready() conditions to be considered ready:
        // ready(): Must either be off cooldown normally (does not need to be reacted to) or reset by a Deathblow (must be reacted to).
        // target_ready(): Must either be within the proper health thresholds or have had an active Deathblow longer than the reaction period.
        return ready() && target_ready( target );
      } );
    }

    return hunter_ranged_attack_t::create_expression( expression_str );
  }
};

struct kill_shot_t : public kill_shot_base_t
{
  kill_shot_t( hunter_t* p, util::string_view options_str )
    : kill_shot_base_t( "kill_shot", p, p->talents.kill_shot )
  {
    if ( p->talents.black_arrow.ok() )
      background = true;
    
    parse_options( options_str );
  }

  void execute() override
  {
    kill_shot_base_t::execute();

    p()->buffs.deathblow->expire();
    p()->consume_precise_shots();
  }

  bool target_ready( player_t* candidate_target ) override
  {
    return kill_shot_base_t::target_ready( candidate_target ) || p()->buffs.deathblow->may_react();
  }
};

// Moonlight Chakram (Sentinel) ======================================================

struct moonlight_chakram_t final : public hunter_ranged_attack_t
{
  struct damage_t final : hunter_ranged_attack_t
  {
    unsigned int bounce_tally = 0;

    damage_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->talents.moonlight_chakram_damage )
    {
      background = dual = true;
    }

    void impact( action_state_t* s ) override
    {
      hunter_ranged_attack_t::impact( s );

      bounce_tally++;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double am = hunter_ranged_attack_t::composite_da_multiplier( s );

      // Moonlight Chakram has a unique Tip buff and is not affected by base Tip (260286) so apply it here.
      if ( p()->buffs.tip_of_the_spear_chakram->up() )
        am *= 1 + p()->talents.tip_of_the_spear_chakram_buff->effectN( 1 ).percent();

      if ( p()->talents.radiant_edge.ok() )
      {
        // Radiant Edge also affects the first hit so calc using tally + 1.
        am *= pow( 1 + p()->talents.radiant_edge->effectN( 1 ).percent(), bounce_tally + 1 );
      }

      return am;
    }
  };

  struct twilight_requiem_t final : hunter_ranged_attack_t
  {
    twilight_requiem_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->talents.twilight_requiem_damage )
    {
      background = dual = true;
      aoe = -1;
      reduced_aoe_targets = p->talents.twilight_requiem->effectN( 1 ).base_value();
    }
  };

  damage_t* damage = nullptr;
  twilight_requiem_t* twilight_requiem = nullptr;

  moonlight_chakram_t( hunter_t* p, util::string_view options_str ) 
    : hunter_ranged_attack_t( "moonlight_chakram", p, p->talents.moonlight_chakram_spell ),
      damage( p->get_background_action<damage_t>( "moonlight_chakram_damage" ) ), 
      twilight_requiem( p->get_background_action<twilight_requiem_t>( "twilight_requiem" ) ) 
  {
    parse_options( options_str );
    add_child( damage );
    add_child( twilight_requiem );

    aoe = 0;
  }

  bool ready() override
  {
    return hunter_ranged_attack_t::ready() && p()->buffs.moonlight_chakram->check();
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( p()->buffs.tip_of_the_spear->check() )
    {
      p()->buffs.tip_of_the_spear->decrement();
      p()->buffs.stargazer->trigger();
      p()->buffs.tip_of_the_spear_chakram->trigger();

      // 2026-01-23: Chakram cannot proc Sentinel's Mark
      if ( p()->cooldowns.strike_as_one->up() )
      {
        auto pet = p()->pets.main;
        if ( pet )
        {
          p()->pets.main->actions.strike_as_one->execute_on_target( target );
          p()->cooldowns.strike_as_one->start();
        }
      }
    }

    damage->bounce_tally = 0;

    p()->buffs.moonlight_chakram->expire();

    if ( p()->talents.stalk_and_strike.ok() )
    {
      p()->buffs.lock_and_load->trigger();
      p()->cooldowns.wildfire_bomb->adjust( -p()->talents.stalk_and_strike->effectN( 1 ).time_value() );
    }
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    auto tl = target_list();
    unsigned int bounce_limit = as<unsigned int>( p()->talents.moonlight_chakram_spell->effectN( 2 ).base_value() );

    // 2026-01-23: Spell data count doesn't include the initial hit so use <= in the loop.
    for ( unsigned int bounce = 0; bounce <= bounce_limit; bounce++ )
    {
      // 200ms estimation based on log data.
      timespan_t time = 200_ms * bounce;
      make_event( sim, time, [ this, tl, bounce ]() { damage->execute_on_target( tl[ bounce % tl.size() ] ); } );

      if ( bounce == bounce_limit )
      {
        if ( p()->talents.twilight_requiem.ok() )
          make_event( sim, time, [ this, s ]() { twilight_requiem->execute_on_target( s->target ); } );

        make_event( sim, time, [ this ]() { p()->buffs.tip_of_the_spear_chakram->expire(); } );
      }
    }
  }
};

// Sanctified Armaments (Sentinel) ===================================================
struct sanctified_armaments_t : public residual_action::residual_periodic_action_t<hunter_ranged_attack_t>
{
  sanctified_armaments_t( util::string_view n, hunter_t* p )
    : residual_periodic_action_t( n, p, p->talents.sanctified_armaments_dot )
  {
    background = dual = true;
    may_crit = false;
  }
};

// Black Arrow (Dark Ranger) =========================================================

struct black_arrow_base_t : public kill_shot_base_t
{
  struct black_arrow_dot_t : public hunter_ranged_attack_t
  {
    black_arrow_dot_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->talents.black_arrow_dot )
    {
      background = dual = true;
      hasted_ticks = false;
    }

    void tick( dot_t* d ) override
    {
      hunter_ranged_attack_t::tick( d );

      if ( p()->talents.corpsecaller_minion_summon.ok() && p()->rppm.corpsecaller->trigger() )
        p()->pets.dark_minion.spawn( p()->talents.corpsecaller_minion_summon->duration() );
    }
  };

  struct bleak_powder_t : public hunter_ranged_attack_t
  {
    struct
    {
      black_arrow_dot_t* dot = nullptr;
      size_t targets         = 0;
    } umbral_reach;

    bleak_powder_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->talents.bleak_powder_spell )
    {
      background = dual      = true;
      aoe                    = -1;
      reduced_aoe_targets    = p->talents.bleak_powder->effectN( 2 ).base_value();
      target_filter_callback = secondary_targets_only();

      if ( p->talents.umbral_reach.ok() )
      {
        umbral_reach.dot     = p->get_background_action<black_arrow_dot_t>( "black_arrow_dot" );
        umbral_reach.targets = as<size_t>( p->talents.umbral_reach->effectN( 2 ).base_value() );
      }
    }

    void impact( action_state_t* s ) override
    {
      hunter_ranged_attack_t::impact( s );

      if ( umbral_reach.dot )
      {
        umbral_reach.dot->execute_on_target( s->target );
        if ( s->chain_target == 0 && s->n_targets >= umbral_reach.targets )
        {
          if ( p()->specialization() == HUNTER_BEAST_MASTERY && p()->talents.beast_cleave.ok() )
          {
            p()->buffs.beast_cleave->trigger();
            for ( auto pet : pets::active<pets::hunter_pet_t>( p()->pets.main, p()->pets.animal_companion, p()->pets.natures_ally_pet.active_pet() ) )
              pet->buffs.beast_cleave->trigger();
          }
          else if ( p()->specialization() == HUNTER_MARKSMANSHIP && p()->talents.trick_shots.ok() )
            p()->buffs.trick_shots->trigger();
        }
      }
    }
  };

  bleak_powder_t* bleak_powder = nullptr;
  black_arrow_dot_t* dot;

  double lower_health_threshold_pct;
  double upper_health_threshold_pct;

  black_arrow_base_t( util::string_view n, hunter_t* p, const spell_data_t* s ) : kill_shot_base_t( n, p, s ), 
    dot( p->get_background_action<black_arrow_dot_t>( "black_arrow_dot" ) ),
    lower_health_threshold_pct( p->talents.black_arrow_spell->effectN( 2 ).base_value() ),
    upper_health_threshold_pct( p->talents.black_arrow_spell->effectN( 3 ).base_value() )
  {
    impact_action = dot;

    if ( p->talents.bleak_powder.ok() )
      bleak_powder = p->get_background_action<bleak_powder_t>( "bleak_powder" );
  }

  void execute() override
  {
    kill_shot_base_t::execute();

    if ( rng().roll( p()->talents.ebon_bowstring->effectN( 1 ).percent() ) )
      p()->trigger_deathblow();
  }

  void impact( action_state_t* s ) override
  {
    kill_shot_base_t::impact( s );

    if ( bleak_powder && p()->cooldowns.bleak_powder->up() )
    {
      bleak_powder->execute_on_target( s->target );
      p()->cooldowns.bleak_powder->start();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    /* Black Arrow has different target ready conditionals than regular Kill Shot, so we don't call Kill Shot base.
       Deathblow check moved to black_arrow_t for Unload. */
    return hunter_ranged_attack_t::target_ready( candidate_target ) &&
           ( candidate_target->health_percentage() <= lower_health_threshold_pct ||
             candidate_target->health_percentage() >= upper_health_threshold_pct );
  }
};

struct black_arrow_t final : public black_arrow_base_t
{
  // Withering Fire (Dark Ranger) =========================================================
  struct withering_fire_t final : hunter_ranged_attack_t
  {
    black_arrow_dot_t* dot;

    withering_fire_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->talents.withering_fire_black_arrow ),
      dot( p->get_background_action<black_arrow_dot_t>( "black_arrow_dot" ) )
    {
      background = dual = true;
      impact_action = dot;
    }
  };

  struct
  {
    int count = 0;
    timespan_t interval = 0_ms;
    withering_fire_t* action = nullptr;
  } withering_fire;

  black_arrow_t( hunter_t* p, util::string_view options_str ) : black_arrow_base_t( "black_arrow", p, p->talents.black_arrow_spell )
  {
    parse_options( options_str );

    if ( p->talents.withering_fire.ok() )
    {
      withering_fire.count = as<int>( p->talents.withering_fire->effectN( 3 ).base_value() );
      withering_fire.interval = p->talents.withering_fire->effectN( 2 ).time_value();
      withering_fire.action = p->get_background_action<withering_fire_t>( "black_arrow_withering_fire" );
      add_child( withering_fire.action );
    }

    auto dot = p->find_action( "black_arrow_dot" );
    add_child( dot );
  }

  void execute() override
  {
    black_arrow_base_t::execute();

    if ( p()->buffs.withering_fire->up() )
    {
      // Prefer targets without Black Arrow ticking.
      auto tl = target_list();
      range::erase_remove( tl, [ this ]( player_t* t ) { return t != target && td( t )->dots.black_arrow->is_ticking(); } );
      target_cache.is_valid = false;

      for ( int i = 1; i <= withering_fire.count; i++ )
      {
        int t = ( i + 1 ) % tl.size();
        make_event( sim, withering_fire.interval * i, [ this, tl, t ]() { withering_fire.action->execute_on_target( tl[ t ] ); } );
      }
    }

    p()->buffs.deathblow->expire();

    p()->trigger_natures_ally_3();
    p()->consume_precise_shots();
  }

  bool target_ready( player_t* candidate_target ) override
  {
    return black_arrow_base_t::target_ready( candidate_target ) || p()->buffs.deathblow->may_react();
  }
};

// Bleak Arrows (Dark Ranger)

struct bleak_arrows_t : public auto_shot_base_t
{
  bleak_arrows_t( hunter_t* p ) : auto_shot_base_t( "bleak_arrows", p, p->talents.bleak_arrows_spell ) {}
};

// Wailing Arrow (Dark Ranger) ========================================================

struct wailing_arrow_t final : public hunter_ranged_attack_t
{
  struct primary_t final : hunter_ranged_attack_t
  {
    primary_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->talents.wailing_arrow_damage )
    {
      background = dual = true;
      attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
    }
  };

  struct cleave_t final : hunter_ranged_attack_t
  {
    cleave_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->talents.wailing_arrow_damage )
    {
      background = dual = true;
      attack_power_mod.direct = data().effectN( 2 ).ap_coeff();
      aoe = -1;

      // 2026-01-22: Wailing Arrow's cleave also hits the primary target.
      //target_filter_callback = secondary_targets_only();
    }
  };

  primary_t* primary = nullptr;
  cleave_t* cleave = nullptr;

  wailing_arrow_t( hunter_t* p, util::string_view options_str )
    : hunter_ranged_attack_t( "wailing_arrow", p, p->talents.wailing_arrow ),
      primary( p->get_background_action<primary_t>( "wailing_arrow_primary" ) ),
      cleave( p->get_background_action<cleave_t>( "wailing_arrow_cleave" ) )
  {
    parse_options( options_str );
    add_child( primary );
    add_child( cleave );
  }

  bool ready() override
  {
    return hunter_ranged_attack_t::ready() && p()->buffs.wailing_arrow->check();
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p()->buffs.wailing_arrow->expire();

    p()->trigger_deathblow( true );
  }

  void impact( action_state_t* state ) override
  {
    hunter_ranged_attack_t::impact( state );

    primary->execute_on_target( state->target );
    cleave->execute_on_target( state->target );
  }
};

// Howl of the Pack Leader (Pack Leader)

struct boar_charge_t final : hunter_ranged_attack_t
{
  struct cleave_t final : hunter_ranged_attack_t
  {
    cleave_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->talents.howl_of_the_pack_leader_boar_charge_cleave )
    {
      background = dual = true;
      travel_speed = 50; // 2026-01-19: Not in spelldata, estimating based on log data.

      // 2026-02-12: Boar Charge's Cleave is softcapped when it should be capped to 8.
      const double cleave_targets = data().effectN( 2 ).base_value();
      aoe = p->bugs ? -1 : as<int>( cleave_targets );
      reduced_aoe_targets = cleave_targets;

      // TODO 31/1/25: currently hits primary target
      // 2026-01-19: still hits primary target
      // target_filter_callback = secondary_targets_only();
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double am = hunter_ranged_attack_t::composite_da_multiplier( s );

      // 2026-03-30: Boar Charges double dip Spirit Bond's bonus
      if ( p()->bugs && p()->specialization() == HUNTER_SURVIVAL )
      {
        double bonus = p()->cache.mastery() * p()->mastery.spirit_bond->effectN( affected_by.spirit_bond.direct ).mastery_value();
        bonus *= 1 + p()->mastery.spirit_bond_buff->effectN( 1 ).percent();
        am *= 1 + bonus;
      }

      return am;
    }

    void impact( action_state_t* s ) override
    {
      hunter_ranged_attack_t::impact( s );

      p()->buffs.hogstrider->increment();
    }
  };

  cleave_t* cleave = nullptr;

  boar_charge_t( hunter_t* p ) : hunter_ranged_attack_t( "boar_charge", p, p->talents.howl_of_the_pack_leader_boar_charge_impact ), 
    cleave( p->get_background_action<cleave_t>( "boar_charge_cleave" ) )
  {
    background = dual = true;
    travel_speed = 50; // 2026-01-19: Not in spelldata, estimating based on log data.

    add_child( cleave );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double am = hunter_ranged_attack_t::composite_da_multiplier( s );

    // 2026-03-30: Boar Charges double dip Spirit Bond's bonus
    if ( p()->bugs && p()->specialization() == HUNTER_SURVIVAL )
    {
      double bonus = p()->cache.mastery() * p()->mastery.spirit_bond->effectN( affected_by.spirit_bond.direct ).mastery_value();
      bonus *= 1 + p()->mastery.spirit_bond_buff->effectN( 1 ).percent();
      am *= 1 + bonus;
    }

    return am;
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    cleave->execute_on_target( target );
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    p()->buffs.hogstrider->increment();
  }
};

// Lunar Storm (Sentinel) ============================================================

struct lunar_storm_t : hunter_ranged_attack_t
{
  lunar_storm_t( hunter_t* p ) : hunter_ranged_attack_t( "lunar_storm", p, p->talents.lunar_storm_dmg )
  {
    background = dual = true;
    aoe = -1;
    reduced_aoe_targets = 8; // TEMP use spelldata when it exists
  }
};

// Stampede (Pack Leader) ============================================================

struct stampede_t : hunter_ranged_attack_t
{
  struct damage_t : public hunter_ranged_attack_t
  {
    damage_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->talents.stampede_dmg )
    {
      aoe = -1;
      background = dual = true;
    }
  };

  damage_t* damage;

  stampede_t( hunter_t* p ) : hunter_ranged_attack_t( "stampede", p, p->talents.stampede_trigger ),
    damage( p->get_background_action<damage_t>( "stampede_tick" ) )
  {
    background = dual = true;
    tick_zero = true;
  }

  void tick( dot_t* d ) override
  {
    hunter_ranged_attack_t::tick( d );

    // There's a gap of about 300ms between each successive damage event triggered by a tick, the first starting with a travel time of about 600ms.
    damage->min_travel_time = 0.6;
    damage->execute_on_target( d->target );
    
    damage->min_travel_time = 0.9;
    damage->execute_on_target( d->target );
    
    damage->min_travel_time = 1.2;
    damage->execute_on_target( d->target );
  }
};

// Let Fly (Marksmanship Midnight Season 1 4pc) ===============================

struct let_fly_t final : hunter_ranged_attack_t
{
  let_fly_t( hunter_t* p ) : hunter_ranged_attack_t( "let_fly", p, p->tier_set.mid_s1_mm_4pc_damage )
  {
    background = dual = true;
    aoe = -1;
  }
};

//==============================
// Beast Mastery attacks
//==============================

// Cobra Shot =================================================================

struct cobra_shot_base_t: public hunter_ranged_attack_t
{
  const timespan_t kill_command_reduction;
  const double serpentine_strikes_amount = p()->talents.serpentine_strikes_energize->effectN( 1 ).base_value();

  cobra_shot_base_t( hunter_t* p, util::string_view n, const spell_data_t* s ): 
    hunter_ranged_attack_t( n, p, s ),
    kill_command_reduction( -timespan_t::from_seconds( data().effectN( 3 ).base_value() ) )
  {
  }

  int n_targets() const override
  {
    int n = hunter_ranged_attack_t::n_targets();

    n += p()->buffs.hogstrider->check();

    return n;
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( p() -> talents.killer_cobra.ok() && p() -> buffs.bestial_wrath -> check() )
      p() -> cooldowns.kill_command -> reset( true );
    
    if ( p()->talents.barbed_scales.ok() )
      p()->cooldowns.barbed_shot->adjust( -p()->talents.barbed_scales->effectN( 1 ).time_value() );

    p()->buffs.howl_of_the_pack_leader_cooldown->extend_duration( -p()->talents.dire_summons->effectN( 3 ).time_value() );

    p()->buffs.hogstrider->expire();

    p()->trigger_natures_ally_3();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = hunter_ranged_attack_t::composite_da_multiplier( s );

    if ( p()->buffs.hogstrider->up() )
      m *= 1 + p()->talents.hogstrider_buff->effectN( 1 ).percent();

    return m;
  }

  void schedule_travel( action_state_t* s ) override
  {
    hunter_ranged_attack_t::schedule_travel( s );

    p() -> cooldowns.kill_command -> adjust( kill_command_reduction );
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( s->result == RESULT_CRIT && p()->talents.serpentine_strikes.ok() ) 
      p()->resource_gain( RESOURCE_FOCUS, serpentine_strikes_amount, p()->gains.serpentine_strikes, this );
  }
};

struct cobra_shot_t : public cobra_shot_base_t
{
  cobra_shot_t( hunter_t* p, util::string_view options_str ) : cobra_shot_base_t( p, "cobra_shot", p->talents.cobra_shot )
  {
    parse_options( options_str );
  }
};

// Cobra Shot (Snakeskin Quiver)

struct cobra_shot_snakeskin_quiver_t : public cobra_shot_base_t
{
  cobra_shot_snakeskin_quiver_t( hunter_t* p ): cobra_shot_base_t( p, "cobra_shot_snakeskin_quiver", p->talents.cobra_shot_data )
  {
    background = dual = true;
    base_costs[ RESOURCE_FOCUS ] = 0;
  }
};

// Barbed Shot ===============================================================

struct barbed_shot_base_t : public hunter_ranged_attack_t
{
  barbed_shot_base_t( hunter_t* p, util::string_view n, const spell_data_t* s ) : hunter_ranged_attack_t( n, p, s )
  {
    tick_zero = true;
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p()->buffs.barbed_shot->trigger();

    auto pet = p()->pets.main;
    if ( pet && p()->rng().roll( p()->talents.brutal_companion->effectN( 1 ).percent() ) )
      pet->actions.brutal_companion_ba->execute_on_target( target );

    p()->trigger_natures_ally_3();

    if ( p()->talents.pack_tactics )
      p()->resource_gain( RESOURCE_FOCUS, p()->talents.pack_tactics_energize->effectN( 1 ).base_value(), p()->gains.pack_tactics, this );
  }

  void tick( dot_t* d ) override
  {
    hunter_ranged_attack_t::tick( d );

    if ( p()->talents.master_handler->ok() )
      p()->cooldowns.kill_command->adjust( -p()->talents.master_handler->effectN( 1 ).time_value() );
  }

  double tick_time_pct_multiplier( const action_state_t* s ) const override
  {
    double tt = hunter_ranged_attack_t::tick_time_pct_multiplier( s );

    if ( p()->buffs.bloody_frenzy->up() )
      tt *= 1 + p()->talents.bloody_frenzy_buff->effectN( 1 ).percent();

    return tt;
  }
};

struct barbed_shot_t : public barbed_shot_base_t
{
  struct {
    double chance = 0;
  } deathblow;

  barbed_shot_t( hunter_t* p, util::string_view options_str )
    : barbed_shot_base_t( p, "barbed_shot", p->talents.barbed_shot )
  {
    parse_options( options_str );

    if ( p->talents.soul_drinker.ok() )
    {
      deathblow.chance = p->talents.soul_drinker->effectN( 2 ).percent();
    }
  }

  void execute() override
  {
    barbed_shot_base_t::execute();

    if ( p()->talents.war_orders.ok() )
      p()->cooldowns.kill_command->adjust( -p()->talents.war_orders->effectN( 3 ).time_value() );

    for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p()->pets.main, p()->pets.animal_companion, p()->pets.natures_ally_pet.active_pet() ) )
    {
      if ( p()->talents.stomp.ok() )
        pet->stable_pet_t::actions.stomp->execute_on_target( target );
    }

    if ( p()->talents.soul_drinker.ok() )
    {
      if ( p()->rng().roll( deathblow.chance ) )
        p()->trigger_deathblow();
    }
  }
};

struct barbed_shot_wild_instincts_t : public barbed_shot_base_t
{
  barbed_shot_wild_instincts_t( hunter_t* p )
    : barbed_shot_base_t( p, "barbed_shot", p->talents.barbed_shot )
  {
    background = dual = true;
  }
};

// Laceration (Beast Mastery Talent) ==============================================

struct laceration_t : public residual_bleed_base_t
{
  laceration_t( hunter_t* p ) : residual_bleed_base_t( "laceration", p, p->talents.laceration_bleed ) {}
};

//==============================
// Marksmanship attacks
//==============================

// Master Marksman ====================================================================

struct master_marksman_t : public residual_bleed_base_t
{
  master_marksman_t( hunter_t* p ) : residual_bleed_base_t( "master_marksman", p, p->talents.master_marksman_bleed ) {}
};

// Multi-Shot =================================================================

struct multishot_t: public hunter_ranged_attack_t
{
  struct state_data_t
  {
    bool empowered_by_precise_shots = false;

    friend void sc_format_to( const state_data_t& data, fmt::format_context::iterator out )
    {
      fmt::format_to( out, "empowered_by_precise_shots={}", data.empowered_by_precise_shots );
    }
  };
  using state_t = hunter_action_state_t<state_data_t>;

  multishot_t( hunter_t* p, util::string_view options_str ) : hunter_ranged_attack_t( "multishot", p, p->specs.multishot )
  {
    parse_options( options_str );

    aoe = -1;
    reduced_aoe_targets = p -> find_spell( 2643 ) -> effectN( 1 ).base_value();
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p()->consume_precise_shots();

    // Delay this since secondary Aimed Shots can cleave with a Trick Shots from Volley, but will not be affected by a Trick Shots 
    // from a queued Multi-Shot that might be executed before they are since they are delayed 10 ms.
    if ( ( p() -> talents.trick_shots.ok() && num_targets_hit >= p() -> talents.trick_shots -> effectN( 2 ).base_value() ) )
      make_event( p()->sim, 10_ms, [ this ]() { p()->buffs.trick_shots->trigger(); } );
  }

  void schedule_travel( action_state_t* s ) override
  {
    hunter_ranged_attack_t::schedule_travel( s );
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( debug_cast<state_t*>( s )->empowered_by_precise_shots )
    {
      // Multi-Shot only ever seems to trigger Spotter's Mark on the primary target
      if ( s->chain_target == 0 )
        p()->trigger_eagles_mark( s->target, p()->talents.sentinel.ok() );

      if ( p()->tier_set.mid_s1_mm_4pc.ok() && p()->rppm.let_fly->trigger() )
        make_event( sim, 300_ms, [ this ]() { p()->actions.let_fly->execute_on_target( target ); } );
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = hunter_ranged_attack_t::composite_da_multiplier( s );

    m *= 1 + p()->buffs.precise_shots->check_stack_value();

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = hunter_ranged_attack_t::composite_target_multiplier( target );

    return m;
  }

  double cost_pct_multiplier() const override
  {
    double c = hunter_ranged_attack_t::cost_pct_multiplier();

    if ( p()->buffs.precise_shots->check() )
      c *= 1 + p()->talents.precise_shots_buff->effectN( 3 ).percent();

    return c;
  }

  double composite_crit_chance() const override
  {
    double cc = hunter_ranged_attack_t::composite_crit_chance();

    if ( p()->talents.critical_precision.ok() && p()->buffs.precise_shots->up() )
    {
      cc += p()->talents.critical_precision->effectN( 1 ).percent();
    }

    return cc;
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    hunter_ranged_attack_t::snapshot_internal( s, flags, rt );

    debug_cast<state_t*>( s )->empowered_by_precise_shots = p()->buffs.precise_shots->up();
  }
};

// Aimed Shot =========================================================================

struct aimed_shot_base_t : public hunter_ranged_attack_t
{
  const int trick_shots_targets;


  aimed_shot_base_t( util::string_view n, hunter_t* p, spell_data_ptr_t s ) :
    hunter_ranged_attack_t( n, p, s ),
    trick_shots_targets( as<int>( p->talents.trick_shots_data->effectN( 1 ).base_value() ) )
  {
    radius = 8;
    base_aoe_multiplier = p->talents.trick_shots_data->effectN( 4 ).percent();
  }

  double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    return am;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = hunter_ranged_attack_t::composite_da_multiplier( s );

    m *= 1 + p()->buffs.bulletstorm->check_stack_value();

    return m;
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = hunter_ranged_attack_t::composite_target_da_multiplier( t );

    m *= 1 + td( target )->debuffs.spotters_mark->check_value();

    return m;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double cm = hunter_ranged_attack_t::composite_crit_damage_bonus_multiplier();

    if ( p()->talents.incendiary_ammunition.ok() )
      cm *= 1 + p()->buffs.bulletstorm->check() * p()->talents.bulletstorm_buff->effectN( 2 ).percent();

    if ( p()->talents.take_aim_2.ok() )
      cm *= 1 + p()->talents.take_aim_2->effectN( 4 ).percent() * p()->cache.attack_crit_chance();

    return cm;
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = hunter_ranged_attack_t::composite_target_crit_chance( target );

    return c;
  }

  void execute() override
  {
    if ( is_aoe() )
      target_cache.is_valid = false;

    hunter_ranged_attack_t::execute();
  }

  int n_targets() const override
  {
    if ( p()->buffs.trick_shots->check() )
      return 1 + trick_shots_targets;

    return hunter_ranged_attack_t::n_targets();
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    hunter_td_t* target_data = td( s->target );

    if ( p()->talents.precision_detonation.ok() )
    {
      if ( target_data->dots.explosive_shot->is_ticking() )
      {
        p()->buffs.precision_detonation_hidden->trigger();

        // Expire Precision Detonation after other possible impacts.
        if ( !p()->state.precision_detonation_expiry )
          p()->state.precision_detonation_expiry = make_event( p()->sim, [ this ]() {
            p()->buffs.precision_detonation_hidden->expire();
            p()->state.precision_detonation_expiry = nullptr;
          } );

        p()->procs.precision_detonation->occur();
        p()->buffs.precision_detonation_hidden->trigger();
        target_data->dots.explosive_shot->cancel();
      }
    }

    if ( target_data->debuffs.spotters_mark->check() || target_data->debuffs.sentinels_mark->check() )
    {
      target_data->debuffs.spotters_mark->expire();
      target_data->debuffs.sentinels_mark->expire();
      p()->trigger_lunar_storm( s->target );
    }
  }
};

struct aimed_shot_t : public aimed_shot_base_t
{
  struct aimed_shot_aspect_of_the_hydra_t : aimed_shot_base_t
  {
    aimed_shot_aspect_of_the_hydra_t( util::string_view n, hunter_t* p ) : aimed_shot_base_t( n, p, p->talents.aimed_shot )
    {
      background = dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
      base_multiplier *= p->talents.aspect_of_the_hydra->effectN( 1 ).percent();
    }

    void execute() override
    {
      aimed_shot_base_t::execute();

      // Consumes Lock and Load without a benefit
      if ( p()->buffs.lock_and_load->check() )
      {
        p()->buffs.lock_and_load->decrement();
      }
    }
  };

  struct aimed_shot_double_tap_t : aimed_shot_base_t
  {
    aimed_shot_double_tap_t( util::string_view n, hunter_t* p ) : aimed_shot_base_t( n, p, p->talents.aimed_shot )
    {
      background = dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
      base_multiplier *= p->talents.double_tap->effectN( 3 ).percent();
    }

    void execute() override
    {
      aimed_shot_base_t::execute();

      // Consumes Lock and Load without a benefit
      if ( p()->buffs.lock_and_load->check() )
      {
        p()->buffs.lock_and_load->decrement();
      }

      // 2026-04-21: Double Tap Aimed Shots can trigger Blighted Arrow casts
      if ( p()->talents.pact_of_the_hollow.ok() )
        for ( auto pet : p()->pets.dark_minion.active_pets() )
          pet->actions.blighted_arrow->execute();
    }
  };

  struct {
    double chance = 0;
    proc_t* proc;
  } surging_shots;

  struct {
    double chance = 0; 
  } deathblow;

  aimed_shot_aspect_of_the_hydra_t* aspect_of_the_hydra = nullptr;
  aimed_shot_double_tap_t* double_tap = nullptr;
  bool lock_and_loaded = false;

  aimed_shot_t( hunter_t* p, util::string_view options_str ) : 
    aimed_shot_base_t( "aimed_shot", p, p->talents.aimed_shot )
  {
    parse_options( options_str );

    if ( p->talents.aspect_of_the_hydra.ok() )
    {
      aspect_of_the_hydra = p->get_background_action<aimed_shot_aspect_of_the_hydra_t>( "aimed_shot_aspect_of_the_hydra" );
      add_child( aspect_of_the_hydra );
    }

    if ( p->talents.double_tap.ok() )
    {
      double_tap = p->get_background_action<aimed_shot_double_tap_t>( "aimed_shot_double_tap" );
      add_child( double_tap );
    }

    if ( p->talents.surging_shots.ok() )
    {
      surging_shots.chance = p->talents.surging_shots->effectN( 3 ).percent();
      surging_shots.proc = p->get_proc( "Surging Shots Rapid Fire reset" );
    }

    if ( p->talents.deathblow.ok() )
      deathblow.chance = p->talents.deathblow->effectN( 1 ).percent();
  }

  double cost() const override
  {
    const bool casting = p() -> executing && p() -> executing == this;
    if ( casting ? lock_and_loaded : p() -> buffs.lock_and_load -> check() )
      return 0;

    return aimed_shot_base_t::cost();
  }

  double cost_pct_multiplier() const override
  {
    double c = aimed_shot_base_t::cost_pct_multiplier();

    if ( p()->buffs.trueshot->check() )
      c *= 1 - p()->talents.tensile_bowstring->effectN( 2 ).percent();

    return c;
  }

  double execute_time_pct_multiplier() const override
  {
    if ( p() -> buffs.lock_and_load -> check() )
      return 0;

    auto et = aimed_shot_base_t::execute_time_pct_multiplier();

    if ( p()->buffs.trueshot->check() )
      et *= 1 - p()->talents.tensile_bowstring->effectN( 1 ).percent();

    return et;
  }
  
  void schedule_execute( action_state_t* s ) override
  {
    lock_and_loaded = p() -> buffs.lock_and_load -> up();

    aimed_shot_base_t::schedule_execute( s );
  }

  void execute() override
  {
    aimed_shot_base_t::execute();

    if ( rng().roll( surging_shots.chance ) )
    {
      surging_shots.proc->occur();
      p()->cooldowns.rapid_fire->reset( true );
      p()->buffs.focus_fire->trigger();
    }
    
    p()->buffs.trick_shots->up(); // Benefit tracking
    p()->consume_trick_shots();

    int precise_shot_stacks = 1;
    
    p()->buffs.precise_shots->increment( precise_shot_stacks );

    if ( rng().roll( deathblow.chance ) )
      p()->trigger_deathblow();

    auto tl = target_list();

    // Delay these secondary shots since they can consume Moving Target or Lock and Load if either trigger off a queued cast.
    if ( aspect_of_the_hydra && tl.size() > 1 )
      make_event( p()->sim, 10_ms, [ this, tl ]() { aspect_of_the_hydra->execute_on_target( tl[ 1 ] ); } );

    if ( double_tap && p()->buffs.double_tap->up() )
    {
      make_event( p()->sim, 10_ms, [ this ]() { double_tap->execute_on_target( target ); } );
      if ( aspect_of_the_hydra && tl.size() > 1 )
        make_event( p()->sim, 10_ms, [ this, tl ]() { aspect_of_the_hydra->execute_on_target( tl[ 1 ] ); } );

      p()->buffs.double_tap->expire();
    }

    if ( p()->talents.pact_of_the_hollow.ok() )
      for ( auto pet : p()->pets.dark_minion.active_pets() )
        pet->actions.blighted_arrow->execute();

    if ( lock_and_loaded )
    {
      p()->buffs.lock_and_load->decrement();
    }

    lock_and_loaded = false;
  }

  double recharge_rate_multiplier( const cooldown_t& cd ) const override
  {
    double m = aimed_shot_base_t::recharge_rate_multiplier( cd );

    if ( p() -> buffs.trueshot -> check() )
      m /= 1 + p() -> talents.trueshot -> effectN( 2 ).percent();

    return m;
  }

  bool usable_moving() const override
  {
    return false;
  }
};

// Rapid Fire =========================================================================

struct rapid_fire_t: public hunter_ranged_attack_t
{
  struct rapid_fire_tick_t : public hunter_ranged_attack_t
  {
    const int trick_shots_targets;

    sanctified_armaments_t* sanctified_armaments = nullptr;

    rapid_fire_tick_t( util::string_view n, hunter_t* p )
      : hunter_ranged_attack_t( n, p, p->talents.rapid_fire_tick ),
        trick_shots_targets( as<int>( p->talents.trick_shots_data->effectN( 3 ).base_value() ) )
    {
      background = dual = true;
      direct_tick = true;
      radius = 8;
      base_aoe_multiplier = p->talents.trick_shots_data->effectN( 5 ).percent();

      // energize
      parse_effect_data( p->talents.rapid_fire_energize->effectN( 1 ) );

      if ( p->talents.sanctified_armaments.ok() )
        sanctified_armaments = p->get_background_action<sanctified_armaments_t>( "sanctified_armaments" );
    }

    int n_targets() const override
    {
      if ( p()->buffs.trick_shots->check() )
        return 1 + trick_shots_targets;

      return hunter_ranged_attack_t::n_targets();
    }

    void execute() override
    {
      hunter_ranged_attack_t::execute();

      p()->buffs.trick_shots->up(); // Benefit tracking

      if ( p()->talents.take_aim_1.ok() )
        p()->cooldowns.aimed_shot->adjust( -p()->talents.take_aim_1->effectN( 2 ).time_value() );
    }

    void impact( action_state_t* state ) override
    {
      hunter_ranged_attack_t::impact( state );

      p()->buffs.bulletstorm->trigger();

      if ( sanctified_armaments )
      {
        double amount = state->result_amount * p()->talents.sanctified_armaments->effectN( 1 ).percent();
        residual_action::trigger( sanctified_armaments, state->target, amount );
      }
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = hunter_ranged_attack_t::composite_da_multiplier( s );

      if ( p()->buffs.focus_fire->up() )
        m *= 1 + p()->talents.focus_fire_buff->effectN( 1 ).percent();

      return m;
    }
  };

  struct rapid_fire_tick_aspect_of_the_hydra_t : public rapid_fire_tick_t
  {
    rapid_fire_tick_aspect_of_the_hydra_t( util::string_view n, hunter_t* p ) : rapid_fire_tick_t( n, p )
    {
      base_multiplier *= p->talents.aspect_of_the_hydra->effectN( 1 ).percent();
    }
  };

  struct arcane_shot_unload_t : public attacks::arcane_shot_base_t
  {
    arcane_shot_unload_t( util::string_view n, hunter_t* p ) : arcane_shot_base_t( n, p )
    {
      background = dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
      base_dd_multiplier *= p->talents.unload->effectN( 1 ).percent();

      // TODO can't guarantee action exists here, find a better solution
      auto arcane_shot = p->find_action( "arcane_shot" );
      if ( arcane_shot )
        arcane_shot->add_child( this );
    }

    void impact( action_state_t* s ) override
    {
      arcane_shot_base_t::impact( s );

      // Despite not consuming Precise Shots, these Arcanes can trigger Let Fly.
      if ( debug_cast<state_t*>( s )->empowered_by_precise_shots )
      {
        if ( p()->tier_set.mid_s1_mm_4pc.ok() && p()->rppm.let_fly->trigger() )
          make_event( sim, 300_ms, [ this ]() { p()->actions.let_fly->execute_on_target( target ); } );
      }
    }
  };

  struct kill_shot_unload_t : public attacks::kill_shot_base_t
  {
    kill_shot_unload_t( util::string_view n, hunter_t* p ) : kill_shot_base_t( n, p, p->talents.kill_shot )
    {
      background = dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
      base_dd_multiplier *= p->talents.unload->effectN( 1 ).percent();

      // TODO can't guarantee action exists here, find a better solution
      auto kill_shot = p->find_action( "kill_shot" );
      if ( kill_shot )
        kill_shot->add_child( this );
    }
  };

  struct black_arrow_unload_t : public attacks::black_arrow_base_t
  {
    black_arrow_unload_t( util::string_view n, hunter_t* p ) : black_arrow_base_t( n, p, p->talents.black_arrow_spell )
    {
      background = dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
      base_dd_multiplier *= p->talents.unload->effectN( 1 ).percent();

      // TODO can't guarantee action exists here, find a better solution
      auto black_arrow = p->find_action( "black_arrow" );
      if ( black_arrow )
        black_arrow->add_child( this );
    }
  };

  struct
  {
    arcane_shot_unload_t* arcane_shot = nullptr;
    kill_shot_unload_t* kill_shot     = nullptr;
    black_arrow_unload_t* black_arrow = nullptr;
  } unload;

  rapid_fire_tick_t* damage;
  rapid_fire_tick_aspect_of_the_hydra_t* aspect_of_the_hydra = nullptr;
  int base_num_ticks;

  struct {
    double chance = 0; 
  } deathblow;

  rapid_fire_t( hunter_t* p, util::string_view options_str ) : hunter_ranged_attack_t( "rapid_fire", p, p -> talents.rapid_fire ),
    damage( p -> get_background_action<rapid_fire_tick_t>( "rapid_fire_tick" ) ),
    base_num_ticks( as<int>( data().effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );

    may_miss = may_crit = false;
    channeled = true;

    if ( p->talents.deathblow.ok() )
      deathblow.chance = p->talents.deathblow->effectN( 2 ).percent();

    if ( p->talents.aspect_of_the_hydra.ok() )
    {
      aspect_of_the_hydra = p->get_background_action<rapid_fire_tick_aspect_of_the_hydra_t>( "rapid_fire_tick_aspect_of_the_hydra" );
      add_child( aspect_of_the_hydra );
    }

    if ( p->talents.unload.ok() )
    {
      unload.arcane_shot = p->get_background_action<arcane_shot_unload_t>( "arcane_shot_unload" );

      if ( p->talents.black_arrow.ok() )
      {
        unload.black_arrow = p->get_background_action<black_arrow_unload_t>( "black_arrow_unload" );
      }
      // Unload only fires Kill Shots with Kill Shot talented
      else if ( p->talents.kill_shot.ok() )
      {
        unload.kill_shot = p->get_background_action<kill_shot_unload_t>( "kill_shot_unload" );
      }
    }
  }

  void execute_unload()
  {
    if ( !p()->talents.unload.ok() )
      return;

    if ( unload.black_arrow && unload.black_arrow->target_ready( target ) )
    {
      unload.black_arrow->execute_on_target( target );
      return;
    }

    if ( unload.kill_shot && unload.kill_shot->target_ready( target ) )
    {
      unload.kill_shot->execute_on_target( target );
      return;
    }

    unload.arcane_shot->execute_on_target( target );
  }

  void init() override
  {
    hunter_ranged_attack_t::init();

    damage -> gain = gain;
    damage -> stats = stats;
    stats -> action_list.push_back( damage );
  }

  bool ready() override
  {
    return hunter_ranged_attack_t::ready() && cooldown->reset_react <= sim->current_time();
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( rng().roll( deathblow.chance ) )
      p()->trigger_deathblow();

    if ( p()->talents.no_scope.ok() )
      p()->buffs.precise_shots->trigger();

    p()->buffs.bulletstorm->expire();

    execute_unload();
  }

  void tick( dot_t* d ) override
  {
    hunter_ranged_attack_t::tick( d );

    damage -> execute_on_target( d->target );

    auto tl = target_list();
    if ( aspect_of_the_hydra && tl.size() > 1 )
      aspect_of_the_hydra->execute_on_target( tl[ 1 ] );
  }

  void last_tick( dot_t* d ) override
  {
    hunter_ranged_attack_t::last_tick( d );

    p()->consume_trick_shots();
    p()->buffs.double_tap->expire();
    p()->buffs.focus_fire->expire();

    execute_unload();
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    // substract 1 here because RF has a tick at zero
    double num_ticks = base_num_ticks - 1;

    if ( p()->buffs.double_tap->check() )
      num_ticks *= 1 + p()->talents.double_tap_buff->effectN( 3 ).percent();

    timespan_t base_duration = num_ticks * tick_time( s );
    
    return base_duration; 
  }

  double tick_time_pct_multiplier( const action_state_t* s ) const override
  {
    double m = hunter_ranged_attack_t::tick_time_pct_multiplier( s );

    m *= 1 + p()->buffs.double_tap->check_value();

    return m;
  }

  double energize_cast_regen( const action_state_t* ) const override
  {
    return base_num_ticks * damage -> composite_energize_amount( nullptr );
  }

  double recharge_rate_multiplier( const cooldown_t& cd ) const override
  {
    double m = hunter_ranged_attack_t::recharge_rate_multiplier( cd );

    if ( p() -> buffs.trueshot -> check() )
      m /= 1 + p() -> talents.trueshot -> effectN( 1 ).percent();

    return m;
  }
};

//==============================
// Survival attacks
//==============================

// Melee attack ==============================================================

struct melee_t : public auto_attack_base_t<melee_attack_t>
{
  struct wildfire_imbuement_t final : hunter_melee_attack_t
  {
    wildfire_imbuement_t( util::string_view n, hunter_t* p )
      : hunter_melee_attack_t( n, p, p->talents.wildfire_imbuement_dmg )
    {
      background = dual = true;
    }
  };

  wildfire_imbuement_t* wildfire_imbuement = nullptr;

  melee_t( util::string_view n, hunter_t* player ) 
    : auto_attack_base_t( n, player ),
      wildfire_imbuement( player->get_background_action<wildfire_imbuement_t>( "wildfire_imbuement" ) )
  {
    school             = SCHOOL_PHYSICAL;
    weapon_multiplier  = 1;
    may_glance         = true;
    may_crit           = true;

    // Dual wielders have a -19% chance to hit on melee attacks
    if ( player->dual_wield() )
      base_hit -= 0.19;
  }

  void execute() override
  {
    auto_attack_base_t::execute();

    if ( p()->buffs.wildfire_imbuement->up() )
      wildfire_imbuement->execute_on_target( target );
  }

  void impact( action_state_t* s ) override
  {
    auto_attack_base_t::impact( s );

    if ( p()->buffs.wildfire_imbuement->up() && s->result == RESULT_HIT )
      wildfire_imbuement->execute_on_target( s->target );

    if ( p()->talents.lethal_barbs.ok() )
    {
      double amount = p()->talents.lethal_barbs_energize->effectN( 1 ).base_value();

      p()->resource_gain( RESOURCE_FOCUS, amount, p()->gains.lethal_barbs, this );
      p()->trigger_aura_applied_callbacks( p()->proc_data_entries.lethal_barbs_energize, p() );
      p()->pets.main->resource_gain( RESOURCE_FOCUS, amount, p()->gains.lethal_barbs, this );
    }
  }

  double action_multiplier() const override
  {
    double am = auto_attack_base_t::action_multiplier();

    double bonus = p()->cache.mastery() * p()->mastery.spirit_bond -> effectN( 5 ).mastery_value();
    bonus *= 1 + p()->mastery.spirit_bond_buff->effectN( 1 ).percent();
      
    am *= 1 + bonus;

    return am;
  }
};

// Muzzle =============================================================

struct muzzle_t : public hunter_melee_attack_t
{
  muzzle_t( hunter_t* p, util::string_view options_str ) : hunter_melee_attack_t( "muzzle", p, p->talents.muzzle )
  {
    parse_options( options_str );

    may_miss = may_block = may_dodge = may_parry = false;
    is_interrupt = true;
  }

  void impact( action_state_t* s ) override
  {
    if( s->target->debuffs.casting->check() && p()->talents.disruptive_rounds.ok() )
      p()->resource_gain( RESOURCE_FOCUS, p()->talents.disruptive_rounds->effectN( 1 ).base_value(), p()->gains.disruptive_rounds,  this );

    hunter_melee_attack_t::impact( s );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !candidate_target->debuffs.casting || !candidate_target->debuffs.casting->check() ) return false;
    return hunter_melee_attack_t::target_ready( candidate_target );
  }
};

// Harpoon ==================================================================

struct harpoon_t : public hunter_ranged_attack_t
{
  harpoon_t( hunter_t* p, util::string_view options_str ) : hunter_ranged_attack_t( "harpoon", p, p->specs.harpoon )
  {
    parse_options( options_str );

    harmful = false;
    base_teleport_distance = data().max_range();
    movement_directionality = movement_direction_type::OMNI;

    const weapon_e group = p->main_hand_weapon.group();
    if ( group != WEAPON_2H && group != WEAPON_1H && group != WEAPON_SMALL )
      background = true;
  }

  bool ready() override
  {
    // XXX: disable this for now to actually make it usable without explicit apl support for movement
    // if ( p() -> current.distance_to_move < data().min_range() )
    //  return false;

    return hunter_ranged_attack_t::ready();
  }
};

// Hatchet Toss ===============================================================================

struct hatchet_toss_t final : public hunter_ranged_attack_t
{
  hatchet_toss_t( hunter_t* p, util::string_view options_str )
    : hunter_ranged_attack_t( "hatchet_toss", p, p->specs.hatchet_toss )
  {
    parse_options( options_str );
  }
};

// Raptor Strike/Mongoose Bite ================================================================

struct melee_focus_spender_t: hunter_melee_attack_t
{
  melee_focus_spender_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
    hunter_melee_attack_t( n, p, s ) {}

  void execute() override
  {
    hunter_melee_attack_t::execute();

    p()->buffs.howl_of_the_pack_leader_cooldown->extend_duration( -p()->talents.dire_summons->effectN( 4 ).time_value() );
  }

  bool ready() override
  {
    const bool has_eagle = p() -> buffs.aspect_of_the_eagle -> check();
    return ( range > 10 ? has_eagle : !has_eagle ) && hunter_melee_attack_t::ready();
  }
};

// Raptor Strike =====================================================================

// TODO move code to melee_focus_spender_t and rename it to remove unnecessary inheritance
struct raptor_strike_base_t : public melee_focus_spender_t
{
  sanctified_armaments_t* sanctified_armaments = nullptr;

  raptor_strike_base_t( util::string_view n, hunter_t* p, spell_data_ptr_t s ) : melee_focus_spender_t( n, p, s )
  {
    if ( p->talents.sanctified_armaments.ok() )
      sanctified_armaments = p->get_background_action<sanctified_armaments_t>( "sanctified_armaments" );
  }

  void execute() override
  {
    melee_focus_spender_t::execute();

    p()->buffs.mongoose_fury->trigger();
  }

  void impact( action_state_t* state ) override
  {
    melee_focus_spender_t::impact( state );

    if ( sanctified_armaments )
    {
      double amount = state->result_amount * p()->talents.sanctified_armaments->effectN( 2 ).percent();
      residual_action::trigger( sanctified_armaments, state->target, amount );
    }
  }
};

struct raptor_strike_t : public raptor_strike_base_t
{
  struct swipe_t final : raptor_strike_base_t
  {
    swipe_t( hunter_t* p ) : raptor_strike_base_t( "raptor_swipe", p, p->talents.raptor_swipe_spell )
    {
      aoe = -1;
      reduced_aoe_targets = p->talents.raptor_swipe_spell->effectN( 2 ).base_value();
    }

    void execute() override
    {
      // Run before execute() as Tip is decremented in the base class
      if ( p()->talents.raptor_swipe_3.ok() && p()->buffs.tip_of_the_spear->check() )
        if ( auto pet = p()->pets.main )
          pet->actions.strike_as_one_swipe->execute_on_target( target );

      raptor_strike_base_t::execute();

      p()->buffs.raptor_swipe->expire();
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double am = raptor_strike_base_t::composite_da_multiplier( s );

      if ( p()->talents.raptor_swipe_2.ok() && s->chain_target == 0 )
        am *= 1 + p()->talents.raptor_swipe_2->effectN( 2 ).percent();

      return am;
    }
  };

  struct
  {
    double chance = 0;
    swipe_t* action = nullptr;
  } swipe;

  raptor_strike_t( hunter_t* p, util::string_view options_str )
    : raptor_strike_base_t( "raptor_strike", p, p->talents.raptor_strike )
  {
    parse_options( options_str );

    if ( p->talents.raptor_swipe_spell.ok() )
    {
      swipe.action = new swipe_t( p );
      add_child( swipe.action );

      // Raptor Swipe 3 doesn't modify Raptor Swipe 1's spell data
      swipe.chance = p->talents.raptor_swipe_3.ok() 
                   ? p->talents.raptor_swipe_3->effectN( 1 ).percent()
                   : p->talents.raptor_swipe_1->effectN( 1 ).percent();
    }
  }

  void execute() override
  {
    if ( p()->buffs.raptor_swipe->up() )
    {
      swipe.action->execute();
    }
    else
    {
      raptor_strike_base_t::execute();
      
      if ( p()->rng().roll( swipe.chance ) )
        p()->buffs.raptor_swipe->trigger();
    }
  }
};

// 2026-01-23: TODO add swipe to eagle after testing

struct raptor_strike_eagle_t : public raptor_strike_base_t
{
  raptor_strike_eagle_t( hunter_t* p, util::string_view options_str )
    : raptor_strike_base_t( "raptor_strike_eagle", p, p->talents.raptor_strike_eagle )
  {
    parse_options( options_str );
  }
};

// Boomstick ===========================================================================
struct boomstick_t : public hunter_spell_t
{
  struct boomstick_tick_t : public hunter_ranged_attack_t
  {
    boomstick_tick_t( util::string_view n, hunter_t* p ) 
      : hunter_ranged_attack_t( n, p, p->talents.boomstick->effectN( 1 ).trigger() )
    {
      aoe                         = -1;
      background                  = true;
      may_crit                    = true;
      radius                      = data().max_range();
      reduced_aoe_targets         = p->talents.boomstick->effectN( 2 ).base_value();
      decrements_tip_of_the_spear = false;
    }
    
    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double dm = hunter_ranged_attack_t::composite_da_multiplier( s );

      if ( p()->buffs.tip_of_the_spear_boomstick->up() )
        dm *= 1 + p()->talents.tip_of_the_spear_boomstick_buff->effectN( 1 ).percent();

      double shellshock_bonus = p()->talents.shellshock->effectN( 1 ).percent();
      if ( s->n_targets > 1 )
      {
        shellshock_bonus = 
          std::max( 0.0, shellshock_bonus - s->n_targets * p()->talents.shellshock->effectN( 2 ).percent() );
      }
      dm *= 1 + shellshock_bonus;

      dm *= 1 + ( p()->buffs.hogstrider->check() * p()->talents.hogstrider_buff->effectN( 2 ).percent() );

      return dm;
    }
  };

  boomstick_tick_t* boomstick_tick = nullptr;

  boomstick_t( hunter_t* p, util::string_view options_str ) 
    : hunter_spell_t( "boomstick", p, p->talents.boomstick ),
      boomstick_tick( p->get_background_action<boomstick_tick_t>( "boomstick_damage" ) )
  {
    parse_options( options_str );

    channeled                   = true;
    tick_zero                   = true;
    decrements_tip_of_the_spear = false;

    add_child( boomstick_tick );
  }

  void execute() override
  {
    if ( p()->buffs.tip_of_the_spear->up() )
    {
      p()->buffs.tip_of_the_spear->decrement();
      p()->buffs.tip_of_the_spear_boomstick->trigger();

      p()->buffs.stargazer->trigger();

      p()->trigger_eagles_mark( get_random_valid_target( boomstick_tick->aoe ), true );

      if ( p()->cooldowns.strike_as_one->up() )
      {
        auto pet = p()->pets.main;
        if ( pet )
        {
          p()->pets.main->actions.strike_as_one->execute_on_target( target );
          p()->cooldowns.strike_as_one->start();
        }
      }
    }
    
    hunter_spell_t::execute();
  }

  void tick( dot_t* dot ) override
  {
    hunter_spell_t::tick( dot );

    boomstick_tick->execute_on_target( dot->target );

    if ( p()->talents.mongoose_rounds.ok() )
      p()->buffs.mongoose_fury->trigger( as<int>( p()->talents.mongoose_rounds->effectN( 1 ).base_value() ) );

    if ( p()->talents.wildfire_shells.ok() )
      p()->cooldowns.wildfire_bomb->adjust( -p()->talents.wildfire_shells->effectN( 1 ).time_value() );
  }

  void last_tick( dot_t* dot ) override
  {
    hunter_spell_t::last_tick( dot );

    p()->buffs.tip_of_the_spear_boomstick->expire();

    p()->buffs.hogstrider->expire();
  }
};

// Takedown =================================================================

struct takedown_t : public hunter_spell_t
{
  struct damage_t final : hunter_melee_attack_t
  {
    damage_t( util::string_view n, hunter_t* p ) : hunter_melee_attack_t( n, p, p->talents.takedown_dmg )
    {
      background = true;
    }
  };

  damage_t* damage = nullptr;

  takedown_t( hunter_t* p, util::string_view options_str ) : hunter_spell_t( "takedown", p, p->talents.takedown )
  {
    parse_options( options_str );

    base_teleport_distance  = data().max_range();
    movement_directionality = movement_direction_type::OMNI;

    energize_type     = action_energize::ON_CAST;
    energize_resource = RESOURCE_FOCUS;
    energize_amount   = p->talents.takedown_energize->effectN( 1 ).base_value();

    // Only the background damage action consumes a Tip stack.
    decrements_tip_of_the_spear = false;

    damage = p->get_background_action<damage_t>( "takedown_damage" );
    add_child( damage );
  }

  void execute() override
  {
    // With Twin Fangs, Takedown applies 3 Tip stacks and then consumes one for its damage
    if ( p()->talents.twin_fangs.ok() )
      p()->buffs.tip_of_the_spear->trigger( as<int>( p()->talents.twin_fangs->effectN( 1 ).base_value() ) );

    hunter_spell_t::execute();

    // Takedown's Buff is applied before the damage event
    p()->buffs.takedown->trigger();

    if ( p()->talents.lunar_calling.ok() )
      p()->trigger_eagles_mark( target, true, true );

    damage->execute_on_target( target );
    if ( auto pet = p()->pets.main )
      pet->actions.takedown->execute_on_target( target );

    if ( p()->talents.stampede.ok() )
    {
      p()->buffs.stampede_incoming->trigger();
      p()->trigger_howl_of_the_pack_leader();
    }

    if ( p()->talents.moonlight_chakram.ok() )
      p()->buffs.moonlight_chakram->trigger();
  }
};

// Flamefang Pitch ==========================================================

struct flamefang_pitch_t : public hunter_spell_t
{
  struct damage_t final : hunter_ranged_attack_t
  {
    damage_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->talents.flamefang_pitch_dmg )
    {
      background = dual = true;
      aoe = -1;
      reduced_aoe_targets = p->talents.flamefang_pitch->effectN( 3 ).base_value();
    }
  };

  struct aoe_t final : hunter_ranged_attack_t
  {
    aoe_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->talents.flamefang_pitch_aoe )
    {
      background = dual = ground_aoe = true;
      aoe = -1;
      decrements_tip_of_the_spear = false;
    }
  };

  damage_t* damage = nullptr;
  aoe_t* aoe = nullptr;

  flamefang_pitch_t( hunter_t* p, util::string_view options_str )
    : hunter_spell_t( "flamefang_pitch", p, p->talents.flamefang_pitch ),
      damage( p->get_background_action<damage_t>( "flamefang_pitch_damage" ) ),
      aoe( p->get_background_action<aoe_t>( "flamefang_pitch_aoe" ) )
  {
    parse_options( options_str );
    add_child( damage );
    add_child( aoe );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p()->buffs.wildfire_imbuement->trigger();

    damage->execute_on_target( target );
    make_event<ground_aoe_event_t>( 
        *sim, player,
        ground_aoe_params_t()
            .target( execute_state->target )
            .duration( p()->talents.flamefang_pitch_data->duration() )
            // No true pulse time exists in spell data for this spell
            .pulse_time( timespan_t::from_seconds( p()->talents.flamefang_pitch_data->effectN( 1 ).base_value() ) )
            .action( aoe ) );

    // 2026-01-18: Grenade Juggler is refunding the unhasted cooldown of a bomb instead of a charge.
    if ( p()->talents.grenade_juggler.ok() )
      p()->cooldowns.wildfire_bomb->adjust( -p()->cooldowns.wildfire_bomb->base_duration );
  }
};

} // end namespace attacks

// ==========================================================================
// Hunter Spells
// ==========================================================================

namespace spells
{

// Summon Pet ===============================================================

struct summon_pet_t: public hunter_spell_t
{
  bool opt_disabled;
  pet_t* pet;

  summon_pet_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "summon_pet", p, p->specs.call_pet ),
    opt_disabled( false ), pet( nullptr )
  {
    parse_options( options_str );

    harmful = false;
    callbacks = false;
    ignore_false_positive = true;

    opt_disabled = util::str_compare_ci( p -> options.summon_pet_str, "disabled" );

    target = player;
  }

  void init_finished() override
  {
    if ( !pet && !opt_disabled )
      pet = player -> find_pet( p() -> options.summon_pet_str );

    if ( !pet && ( p() -> specialization() != HUNTER_MARKSMANSHIP || p()->talents.unbreakable_bond.ok() ) )
    {
      throw sc_invalid_apl_argument(
        fmt::format( "Unable to find pet '{}' for summons.", p()->options.summon_pet_str ) );
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
    if ( opt_disabled || p() -> pets.main == pet || ( p()->specialization() == HUNTER_MARKSMANSHIP && !p()->talents.unbreakable_bond.ok() ) )
      return false;

    return hunter_spell_t::ready();
  }
};

// Base Trap =========================================================================

struct trap_base_t : hunter_spell_t
{
  timespan_t precast_time;

  trap_base_t( util::string_view n, hunter_t* p, spell_data_ptr_t s ) :
    hunter_spell_t( n, p, s )
  {
    add_option( opt_timespan( "precast_time", precast_time ) );

    harmful = may_miss = false;
  }

  void init() override
  {
    hunter_spell_t::init();

    precast_time = clamp( precast_time, 0_ms, cooldown -> duration );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    adjust_precast_cooldown( precast_time );
  }

  timespan_t travel_time() const override
  {
    timespan_t time_to_travel = hunter_spell_t::travel_time();
    if ( is_precombat )
      return std::max( 0_ms, time_to_travel - precast_time );
    return time_to_travel;
  }
};

// Freezing Trap ============================================================================

struct freezing_trap_t : public trap_base_t
{
  freezing_trap_t( hunter_t* p, util::string_view options_str )
    : trap_base_t( "freezing_trap", p, p->specs.freezing_trap )
  {
    parse_options( options_str );
  }
};

// Tar Trap (Hunter Talent) ==============================================================

struct tar_trap_t : public trap_base_t
{
  timespan_t debuff_duration;

  tar_trap_t( hunter_t* p, util::string_view options_str ) :
    trap_base_t( "tar_trap", p, p -> talents.tar_trap )
  {
    parse_options( options_str );

    debuff_duration = p -> find_spell( 13810 ) -> duration();
  }

  void impact( action_state_t* s ) override
  {
    trap_base_t::impact( s );

    p() -> state.tar_trap_aoe = make_event<events::tar_trap_aoe_t>( *p() -> sim, p(), s -> target, debuff_duration );
  }
};

// Kill Command (Beast Mastery/Survival Talent) =============================================================

struct kill_command_t: public hunter_spell_t
{
  struct {
    double chance = 0; 
  } deathblow;

  struct
  {
    timespan_t extension = 0_s;
    timespan_t cap = 0_s;
  } fury_of_the_wyvern;

  kill_command_t( hunter_t* p, util::string_view options_str, const spell_data_t* s ) : hunter_spell_t( "kill_command", p, s )
  {
    parse_options( options_str );

    if ( p->specialization() == HUNTER_BEAST_MASTERY )
    {
      if ( p->talents.soul_drinker.ok() )
        deathblow.chance = p->talents.soul_drinker->effectN( 1 ).percent();
      
      if ( p->talents.fury_of_the_wyvern.ok() )
      {
        fury_of_the_wyvern.extension = p->talents.fury_of_the_wyvern->effectN( 2 ).time_value();
        fury_of_the_wyvern.cap = timespan_t::from_seconds( p->talents.fury_of_the_wyvern->effectN( 4 ).base_value() );
      }
    }
  }

  void execute() override
  {
    hunter_spell_t::execute();

    for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p()->pets.main, p()->pets.animal_companion, p()->pets.natures_ally_pet.active_pet() ) )
      pet -> actions.kill_command -> execute_on_target( target );

    if ( p()->talents.wildspeaker.ok() )
    {
      for ( auto pet : p()->active_pets )
      {
        pets::dire_critter_t* dire_critter = dynamic_cast<pets::dire_critter_t*>( pet );
        if ( dire_critter )
          dire_critter->actions.kill_command->execute_on_target( target );
      }
    }

    if ( p()->talents.pact_of_the_hollow.ok() )
      for ( auto pet : p()->pets.dark_hound.active_pets() )
        pet->actions.shadow_thrash->execute();

    p()->consume_howl_of_the_pack_leader( target );

    int tip_stacks = 1;
    tip_stacks += as<int>( p()->talents.primal_surge->effectN( 1 ).base_value() );
    p()->buffs.tip_of_the_spear->trigger( tip_stacks );

    if ( p()->talents.dire_command && p()->accumulated_rng.dire_command->trigger() )
    {
      p()->spawn_dire_beast( p()->talents.dire_beast_summon->duration() );
      p()->procs.dire_command->occur();
    }

    if ( p()->talents.soul_drinker.ok() )
    {
      if ( rng().roll( deathblow.chance ) )
        p()->trigger_deathblow();
    }

    p()->cooldowns.wildfire_bomb->adjust( -p()->talents.wildfire_infusion->effectN( 1 ).time_value() );

    p()->buffs.howl_of_the_pack_leader_cooldown->extend_duration( -p()->talents.dire_summons->effectN( p()->specialization() == HUNTER_BEAST_MASTERY ? 1 : 2 ).time_value() );
    
    if ( p()->buffs.wyverns_cry->check() && p()->state.fury_of_the_wyvern_extension < fury_of_the_wyvern.cap )
    {
      p()->buffs.wyverns_cry->extend_duration( fury_of_the_wyvern.extension );
      p()->state.fury_of_the_wyvern_extension += fury_of_the_wyvern.extension;
      p()->state.fury_of_the_wyvern_extendable = p()->state.fury_of_the_wyvern_extension < fury_of_the_wyvern.cap;
    }

    p()->buffs.natures_ally_3->expire();
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( p()->pets.main &&
         p()->pets.main->hunter_main_pet_base_t::actions.kill_command->target_ready( candidate_target ) )
      return hunter_spell_t::target_ready( candidate_target );

    return false;
  }

  bool ready() override
  {
    if ( p()->pets.main &&
         p()->pets.main->hunter_main_pet_base_t::actions.kill_command->ready() ) // Range check from the pet.
    {
        return hunter_spell_t::ready();
    }

    return false;
  }

  std::unique_ptr<expr_t> create_expression(util::string_view expression_str) override
  {
    if ( expression_str == "damage" )
    {
      auto pet = p()->find_pet( p()->options.summon_pet_str );
      if ( pet )
      {
        auto kc = pet->find_action( "kill_command" );
        if ( kc )
          return std::make_unique<pet_amount_expr_t>( expression_str, *this, *kc );
      }
    }

    return hunter_spell_t::create_expression( expression_str );
  }
};

//==============================
// Beast Mastery spells
//==============================

// Bestial Wrath ============================================================

struct bestial_wrath_t: public hunter_ranged_attack_t
{
  timespan_t precast_time = 0_ms;

  bestial_wrath_t( hunter_t* p, util::string_view options_str ) : hunter_ranged_attack_t( "bestial_wrath", p, p -> talents.bestial_wrath )
  {
    add_option( opt_timespan( "precast_time", precast_time ) );
    parse_options( options_str );

    precast_time = clamp( precast_time, 0_ms, data().duration() );
  }

  bool usable_precombat() const override
  {
    return true;
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    trigger_buff( p() -> buffs.bestial_wrath, precast_time );

    if ( p()->talents.natures_ally_1.ok() )
      p()->pets.natures_ally_pet.spawn( p()->talents.natures_ally_1_summon->duration() );

    if ( p()->tier_set.mid_s1_bm_4pc->ok() ) 
      p()->spawn_dire_beast( p()->tier_set.mid_s1_bm_4pc->effectN( 1 ).time_value() );

    for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p()->pets.main, p()->pets.animal_companion, p()->pets.natures_ally_pet.active_pet() ) )
    {
      trigger_buff( pet->buffs.bestial_wrath, precast_time );

      // Assume the pet is out of range / not engaged when precasting.
      if ( !is_precombat )
        pet -> actions.bestial_wrath -> execute_on_target( target );
    }

    if ( p()->talents.wildspeaker.ok() )
    {
      for ( auto pet : p()->active_pets )
      {
        pets::dire_critter_t* dire_critter = dynamic_cast<pets::dire_critter_t*>( pet );
        if ( dire_critter )
          dire_critter->buffs.bestial_wrath->trigger();
      }
    }

    adjust_precast_cooldown( precast_time );

    if ( p()->talents.scent_of_blood.ok() )
      p()->cooldowns.barbed_shot->reset( true, as<int>( p()->talents.scent_of_blood->effectN( 1 ).base_value() ) );

    if ( p()->talents.bloody_frenzy.ok() )
      p()->buffs.bloody_frenzy->trigger();

    if ( p()->talents.bloodshed.ok() )
    {
      for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p()->pets.main, p()->pets.animal_companion, p()->pets.natures_ally_pet.active_pet() ) )
        pet->actions.bloodshed->execute_on_target( target );
    }

    if ( p()->talents.thundering_hooves.ok() )
    {
      // 01/02/2026 - The pet summoned by Nature's Ally does not benefit from Thundering Hooves 
      if( !p()->bugs )
        for ( auto pet : pets::active<pets::stable_pet_t>( p()->pets.main, p()->pets.animal_companion, p()->pets.natures_ally_pet.active_pet() ) )
          pet->actions.thundering_hooves->execute_on_target( target );
      else
        for ( auto pet : pets::active<pets::stable_pet_t>( p()->pets.main, p()->pets.animal_companion ) )
          pet->actions.thundering_hooves->execute_on_target( target );
    }

    if ( p()->talents.withering_fire.ok() )
    {
      p()->buffs.withering_fire->trigger();
      p()->trigger_deathblow( true );
    }
    
    if ( p()->talents.wailing_dead.ok() )
    {
      bool force_hound = true;
      p()->spawn_dire_beast( p()->talents.corpsecaller_hound_summon->duration(), force_hound );
      p()->buffs.wailing_arrow->trigger();
    }
      

    if ( p()->talents.stampede.ok() )
    {
      p()->buffs.stampede_incoming->trigger();
      p()->trigger_howl_of_the_pack_leader();
    }
  }

  bool ready() override
  {
    if ( !p() -> pets.main )
      return false;

    return hunter_ranged_attack_t::ready();
  }
};

// Wild Thrash =============================================================

struct wild_thrash_t : public hunter_spell_t
{
  wild_thrash_t( hunter_t* p, util::string_view options_str ) : hunter_spell_t( "wild_thrash", p, p->talents.wild_thrash_player )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p()->pets.main, p()->pets.animal_companion, p()->pets.natures_ally_pet.active_pet() ) )
      pet->actions.wild_thrash->execute();

    if ( p()->talents.beast_cleave->ok() )
    {
      p()->buffs.beast_cleave->trigger();

      for ( auto pet : pets::active<pets::hunter_pet_t>( p()->pets.main, p()->pets.animal_companion, p()->pets.natures_ally_pet.active_pet() ) )
        pet->buffs.beast_cleave->trigger();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( p()->pets.main &&
         p()->pets.main->hunter_main_pet_base_t::actions.wild_thrash->target_ready( candidate_target ) )
      return hunter_spell_t::target_ready( candidate_target );

    return false;
  }

  bool ready() override
  {
    if ( p()->pets.main &&
         p()->pets.main->hunter_main_pet_base_t::actions.wild_thrash->ready() )  // Range check from the pet.
    {
      return hunter_spell_t::ready();
    }

    return false;
  }
};

//==============================
// Marksmanship spells
//==============================

// Harrier's Cry ===========================================================

struct harriers_cry_t: public hunter_spell_t
{
  harriers_cry_t( hunter_t* p, util::string_view options_str ) : hunter_spell_t( "harriers_cry", p, p->find_spell( 466904 ) )
  {
    parse_options( options_str );

    harmful = false;
    track_cd_waste = false;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    // use indices since it's possible to spawn new actors when bloodlust is triggered
    for ( size_t i = 0; i < sim->player_non_sleeping_list.size(); i++ )
    {
      auto* p = sim->player_non_sleeping_list[ i ];
      if ( p->is_pet() || p->buffs.exhaustion->check() )
        continue;

      p->buffs.bloodlust->trigger();
      p->buffs.exhaustion->trigger();
    }
  }

  bool ready() override
  {
    if ( !sim->overrides.bloodlust )
      return false;

    return hunter_spell_t::ready();
  }
};

// Trueshot =================================================================

struct trueshot_t : public hunter_spell_t
{
  trueshot_t( hunter_t* p, util::string_view options_str ) : hunter_spell_t( "trueshot", p, p -> talents.trueshot )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    // Applying Trueshot directly does not extend an existing Trueshot and resets Unerring Vision stacks.
    p() -> buffs.trueshot -> expire();
    p() -> buffs.trueshot -> trigger();
    
    if ( p()->talents.withering_fire.ok() )
    {
      p()->buffs.withering_fire->trigger( p()->buffs.trueshot->data().duration() );
      p()->trigger_deathblow( true );
    }

    if ( p()->talents.wailing_dead.ok() )
    {
      p()->pets.dark_minion.spawn( p()->talents.corpsecaller_minion_summon->duration() );
      p()->buffs.wailing_arrow->trigger();
    }

    if ( p()->talents.feathered_frenzy.ok() )
      p()->trigger_eagles_mark( target, p()->talents.sentinel.ok(), true );

    if ( p()->talents.moonlight_chakram.ok() )
      p()->buffs.moonlight_chakram->trigger();

    p()->buffs.double_tap->trigger();
  }
};

// Volley ===========================================================================

struct volley_t : public hunter_spell_t
{
  struct damage_t final : hunter_ranged_attack_t
  {
    struct salvo {
      attacks::explosive_shot_background_t* explosive = nullptr;
      int targets = 0;
    } salvo;

    damage_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p -> talents.volley_dmg )
    {
      aoe = -1;
      background = dual = ground_aoe = true;

      if ( p -> talents.salvo.ok() )
      {
        salvo.targets = as<int>( p->talents.salvo->effectN( 1 ).base_value() );
        salvo.explosive = p->get_background_action<attacks::explosive_shot_background_t>( "explosive_shot", p->find_spell( 212431 ) );
      }
    }

    void execute() override
    {
      hunter_ranged_attack_t::execute();

      p()->cooldowns.salvo->start();
    }

    void impact( action_state_t* s ) override
    {
      hunter_ranged_attack_t::impact( s );

      if ( s->chain_target < salvo.targets && p()->cooldowns.salvo->up() )
        salvo.explosive->execute_on_target( s->target );
    }
  };

  damage_t* damage;
  timespan_t tick_duration;

  volley_t( hunter_t* p, util::string_view options_str ) : hunter_spell_t( "volley", p, p->talents.volley ),
    damage( p->get_background_action<damage_t>( "volley_damage" ) ),
    tick_duration( data().duration() )
  {
    parse_options( options_str );

    if ( damage->salvo.explosive )
      add_child( damage->salvo.explosive );

    // disable automatic generation of the dot from spell data
    dot_duration = 0_ms;

    may_hit = false;
    damage -> stats = stats;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.volley -> trigger( tick_duration );
    p() -> buffs.trick_shots -> trigger( tick_duration );

    p() -> state.current_volley = 
      make_event<ground_aoe_event_t>( *sim, player, ground_aoe_params_t()
        .target( execute_state -> target )
        .duration( tick_duration )
        .pulse_time( data().effectN( 2 ).period() )
        .action( damage )
        .state_callback( [ this ]( ground_aoe_params_t::state_type type, ground_aoe_event_t* event ) {
          switch ( type )
            {
              case ground_aoe_params_t::EVENT_CREATED:
                p() -> state.current_volley = event;
                break;
              case ground_aoe_params_t::EVENT_STOPPED:
              {
                p()->state.current_volley = nullptr;
                break;
              }
              default:
                break;
            }
        } )
      );

    p()->buffs.double_tap->trigger();
  }
};

//==============================
// Survival spells
//==============================

// Wildfire Bomb ==============================================================

struct wildfire_bomb_base_t : public hunter_ranged_attack_t
{
  struct bomb_damage_t : public hunter_ranged_attack_t
  {
    struct bomb_dot_t final : public hunter_spell_t
    {
      bomb_dot_t( util::string_view n, hunter_t* p ) :
        hunter_spell_t( n, p, p->talents.shrapnel_bomb.ok() ? p->talents.shrapnel_bomb_bleed : p->talents.wildfire_bomb_dot )
      {
        background = dual = true;
      }

      double composite_ta_multiplier( const action_state_t* s ) const override
      {
        double am = hunter_spell_t::composite_ta_multiplier( s );

        if ( as<double>( s->n_targets ) > reduced_aoe_targets )
          am *= std::sqrt( reduced_aoe_targets / s->n_targets );

        if ( s->chain_target == 0 )
          am *= 1.0 + p()->talents.wildfire_bomb->effectN( 3 ).percent();

        return am;
      }
    };

    bomb_dot_t* bomb_dot;

    bomb_damage_t( util::string_view n, hunter_t* p, wildfire_bomb_base_t* a ) : hunter_ranged_attack_t( n, p, p->talents.wildfire_bomb_dmg ),
      bomb_dot( p->get_background_action<bomb_dot_t>( p->talents.shrapnel_bomb.ok() ? "wildfire_bomb_bleed" : "wildfire_bomb_dot" ) )
    {
      background = dual = true;

      // 2026-02-11: Wildfire Bomb's direct damage is not buffed by Unnatural Causes in game, despite being in spell data
      affected_by.unnatural_causes.direct = as<uint8_t>( 0 );

      aoe = -1;
      reduced_aoe_targets = p -> talents.wildfire_bomb -> effectN( 2 ).base_value();
      radius = 5; // XXX: It's actually a circle + cone, but we sadly can't really model that

      bomb_dot->reduced_aoe_targets = reduced_aoe_targets;
      bomb_dot->aoe                 = aoe;
      bomb_dot->radius              = radius;

      a->add_child( this );
      a->add_child( bomb_dot );
    }

    void execute() override
    {
      hunter_ranged_attack_t::execute();

      if ( num_targets_hit > 0 )
      {
        // Dot applies to all of the same targets hit by the main explosion
        bomb_dot->target                    = target;
        bomb_dot->target_cache.list         = target_cache.list;
        bomb_dot->target_cache.is_valid     = true;
        bomb_dot->execute();
      }

      if ( p()->talents.shrapnel_bomb.ok() )
      {
        p()->buffs.shrapnel_bomb->trigger();
      }
    }

    void impact( action_state_t* s ) override
    {
      hunter_ranged_attack_t::impact( s );

      auto target_data = td( s->target );

      if ( target_data->debuffs.sentinels_mark->check() )
      {
        target_data->debuffs.sentinels_mark->expire();
        p()->trigger_lunar_storm( s->target );
      }

      if ( s->chain_target < p()->talents.lethal_calibration->effectN( 2 ).base_value() )
        p()->cooldowns.boomstick->adjust( -p()->talents.lethal_calibration->effectN( 1 ).time_value() );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double am = hunter_ranged_attack_t::composite_da_multiplier( s );

      if ( s->chain_target == 0 )
      {
        am *= 1.0 + p()->talents.wildfire_bomb->effectN( 3 ).percent();
        am *= 1 + p()->talents.sharpened_fangs->effectN( 2 ).percent();
      }

      return am;
    }
  };

  wildfire_bomb_base_t( hunter_t* p, const spell_data_t* s = spell_data_t::nil() ) : hunter_ranged_attack_t( "wildfire_bomb", p, s )
  {
    may_miss = false;
    school = SCHOOL_FIRE; // for report coloring
    harmful = false;

    impact_action = p->get_background_action<bomb_damage_t>( "wildfire_bomb_damage", this );
  }
};

struct wildfire_bomb_t: public wildfire_bomb_base_t
{
  struct
  {
    timespan_t extension = 0_s;
    timespan_t cap = 0_s;
  } fury_of_the_wyvern;

  wildfire_bomb_t( hunter_t* p, util::string_view options_str ) : wildfire_bomb_base_t( p, p->talents.wildfire_bomb )
  {
    parse_options( options_str );

    if ( p->talents.fury_of_the_wyvern.ok() )
    {
      fury_of_the_wyvern.extension = p->talents.fury_of_the_wyvern->effectN( 3 ).time_value();
      fury_of_the_wyvern.cap = timespan_t::from_seconds( p->talents.fury_of_the_wyvern->effectN( 5 ).base_value() );
    }
  }

  timespan_t travel_time() const override
  {
    if ( is_precombat )
      return timespan_t::from_millis( 0 );

    return wildfire_bomb_base_t::travel_time();
  }

  void execute() override
  {
    // Tip of the Spear is decremented in execute() so run here
    if ( p()->tier_set.mid_s1_sv_4pc.ok() && p()->buffs.tip_of_the_spear->check() )
      if ( auto pet = p()->pets.main )
        pet->actions.strike_as_one->execute_on_target( target );

    wildfire_bomb_base_t::execute();

    if ( p()->buffs.wyverns_cry->check() && p()->state.fury_of_the_wyvern_extension < fury_of_the_wyvern.cap )
    {
      p()->buffs.wyverns_cry->extend_duration( fury_of_the_wyvern.extension );
      p()->state.fury_of_the_wyvern_extension += fury_of_the_wyvern.extension;
      p()->state.fury_of_the_wyvern_extendable = p()->state.fury_of_the_wyvern_extension < fury_of_the_wyvern.cap;
    }
  }
};

// Aspect of the Eagle ======================================================

struct aspect_of_the_eagle_t: public hunter_spell_t
{
  aspect_of_the_eagle_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "aspect_of_the_eagle", p, p->specs.aspect_of_the_eagle )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p()->buffs.aspect_of_the_eagle->trigger();
  }
};

} // end namespace spells

namespace actions {

// Auto attack =======================================================================

struct auto_attack_t: public action_t
{
  auto_attack_t( hunter_t* p, util::string_view options_str ) :
    action_t( ACTION_OTHER, "auto_attack", p )
  {
    parse_options( options_str );

    ignore_false_positive = true;
    trigger_gcd = 0_ms;

  #ifdef NDEBUG
    assert( p->main_hand_weapon.type != WEAPON_NONE );
  #endif

    if ( p->main_hand_weapon.group() == WEAPON_RANGED )
    {
      if ( p->talents.bleak_arrows.ok() )
        p->main_hand_attack = new attacks::bleak_arrows_t( p );
      else
        p->main_hand_attack = new attacks::auto_shot_t( p );
    }
    else
    {
      p->main_hand_attack                    = new attacks::melee_t( "auto_attack_mh", p );
      p->main_hand_attack->weapon            = &( p->main_hand_weapon );
      p->main_hand_attack->base_execute_time = p->main_hand_weapon.swing_time;

      if ( p->off_hand_weapon.type != WEAPON_NONE )
      {
        p->off_hand_attack                    = new attacks::melee_t( "auto_attack_oh", p );
        p->off_hand_attack->weapon            = &( p->off_hand_weapon );
        p->off_hand_attack->base_execute_time = p->off_hand_weapon.swing_time;
        p->off_hand_attack->id                = 1;
      }

      range = 5;
    }
  }

  void execute() override
  {
    player->main_hand_attack->schedule_execute();

    // 2026-01-24: Sync swings by default, with more log data maybe add some delay for accuracy.
    if ( player->off_hand_attack )
      player->off_hand_attack->schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() && !usable_moving() )
      return false;

    return player->main_hand_attack->execute_event == nullptr; // not swinging
  }
};

} // end namespace actions

hunter_td_t::hunter_td_t( player_t* t, hunter_t* p ) : actor_target_data_t( t, p ),
  debuffs(),
  dots()
{
  double outland_venom_value = p->talents.outland_venom_debuff->effectN( 1 ).percent();
  if ( p->bugs )
    outland_venom_value /= 2; // 2026-01-24: Outland Venom is only giving half of its value.
  debuffs.outland_venom = make_buff( *this, "outland_venom", p->talents.outland_venom_debuff )
    ->set_default_value( outland_venom_value )
    ->disable_ticking( true );

  debuffs.spotters_mark = make_buff( *this, "spotters_mark", p->specs.spotters_mark_debuff )
    ->set_default_value( p->specs.spotters_mark_debuff->effectN( 1 ).percent() );

  debuffs.sentinels_mark = make_buff( *this, "sentinels_mark", p->talents.sentinels_mark )
    ->set_default_value_from_effect( p->specialization() == HUNTER_MARKSMANSHIP ? 1 : 2 );

  debuffs.headshot = make_buff( *this, "headshot", p->talents.headshot_debuff )
    -> set_default_value_from_effect( 1 );

  dots.wildfire_bomb = t->get_dot( p->talents.shrapnel_bomb ? "wildfire_bomb_bleed" : "wildfire_bomb_dot", p );
  dots.sanctified_armaments = t->get_dot( "sanctified_armaments", p );
  dots.black_arrow = t -> get_dot( "black_arrow_dot", p );
  dots.barbed_shot = t -> get_dot( "barbed_shot", p );
  dots.explosive_shot = t->get_dot( "explosive_shot", p );

  t -> register_on_demise_callback( p, [this](player_t*) { target_demise(); } );
}

void hunter_td_t::target_demise()
{
  damaged = false;

  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( source -> sim -> event_mgr.canceled )
    return;
}

/**
 * Hunter specific action expression
 *
 * Use this function for expressions which are bound to an action property such as target, cast_time etc.
 * If you need an expression tied to the player itself use the normal hunter_t::create_expression override.
 */
std::unique_ptr<expr_t> hunter_t::create_action_expression ( action_t& action, util::string_view expression_str )
{
  return player_t::create_action_expression( action, expression_str );
}

std::unique_ptr<expr_t> hunter_t::create_expression( util::string_view expression_str )
{
  auto splits = util::string_split<util::string_view>( expression_str, "." );

  if ( splits.size() == 1 && splits[ 0 ] == "max_prio_damage" )
  {
    return std::make_unique<const_expr_t>( expression_str, options.max_prio_damage );
  }
  else if ( splits.size() == 2 && splits[ 0 ] == "tar_trap" )
  {
    if ( splits[ 1 ] == "up" )
      return make_fn_expr( expression_str, [ this ] { return state.tar_trap_aoe != nullptr; } );

    if ( splits[ 1 ] == "remains" )
    {
      return make_fn_expr( expression_str,
        [ this ]() -> timespan_t {
          if ( state.tar_trap_aoe == nullptr )
            return 0_ms;
          return state.tar_trap_aoe -> remains();
        } );
    }
  }
  else if ( splits.size() >= 2 && splits[ 0 ] == "pet" && splits[ 1 ] == "main" &&
            !util::str_compare_ci( options.summon_pet_str, "disabled" ) )
  {
    // fudge the expression to refer to the "main pet"
    splits[ 1 ] = options.summon_pet_str;
    return player_t::create_expression( util::string_join( splits, "." ) );
  }
  else if ( splits.size() > 1 && splits[ 0 ] == "howl_summon" )
  {
    if ( splits.size() == 2 && splits[ 1 ] == "ready" )
    {
      return make_fn_expr( expression_str, [ this ] {
        return buffs.howl_of_the_pack_leader_wyvern->check()
          || buffs.howl_of_the_pack_leader_boar->check()
          || buffs.howl_of_the_pack_leader_bear->check();
      } );
    }
    else if ( splits.size() == 3 )
    {
      if ( splits[ 1 ] == "ready" )
      {
        if ( splits[ 2 ] == "wyvern" )
          return make_fn_expr( expression_str, [ this ] { return buffs.howl_of_the_pack_leader_wyvern->check(); } );
        
        if ( splits[ 2 ] == "boar" )
          return make_fn_expr( expression_str, [ this ] { return buffs.howl_of_the_pack_leader_boar->check(); } );
        
        if ( splits[ 2 ] == "bear" )
          return make_fn_expr( expression_str, [ this ] { return buffs.howl_of_the_pack_leader_bear->check(); } );
      }
      else if ( splits[ 1 ] == "next" )
      {
        if ( splits[ 2 ] == "wyvern" )
          return make_fn_expr( expression_str, [ this ] { return state.howl_of_the_pack_leader_next_beast == howl_of_the_pack_leader_beast::WYVERN; } );
        
        if ( splits[ 2 ] == "boar" )
          return make_fn_expr( expression_str, [ this ] { return state.howl_of_the_pack_leader_next_beast == howl_of_the_pack_leader_beast::BOAR; } );
        
        if ( splits[ 2 ] == "bear" )
          return make_fn_expr( expression_str, [ this ] { return state.howl_of_the_pack_leader_next_beast == howl_of_the_pack_leader_beast::BEAR; } );
      }
    }
  }
  else if ( splits.size() == 1 && splits[ 0 ] == "fury_of_the_wyvern_extendable" )
  {
    return make_fn_expr( expression_str, [ this ] { return state.fury_of_the_wyvern_extendable; } );
  }

  return player_t::create_expression( expression_str );
}

action_t* hunter_t::create_action( util::string_view name, util::string_view options_str )
{
  using namespace attacks;
  using namespace spells;

  if ( name == "aimed_shot"            ) return new             aimed_shot_t( this, options_str );
  if ( name == "aspect_of_the_eagle"   ) return new    aspect_of_the_eagle_t( this, options_str );
  if ( name == "auto_attack"           ) return new   actions::auto_attack_t( this, options_str );
  if ( name == "auto_shot"             ) return new   actions::auto_attack_t( this, options_str );
  if ( name == "barbed_shot"           ) return new            barbed_shot_t( this, options_str );
  if ( name == "bestial_wrath"         ) return new          bestial_wrath_t( this, options_str );
  if ( name == "black_arrow"           ) return new            black_arrow_t( this, options_str );
  if ( name == "cobra_shot"            ) return new             cobra_shot_t( this, options_str );
  if ( name == "counter_shot"          ) return new           counter_shot_t( this, options_str );
  if ( name == "explosive_shot"        ) return new         explosive_shot_t( this, options_str );
  if ( name == "flamefang_pitch"       ) return new        flamefang_pitch_t( this, options_str );
  if ( name == "freezing_trap"         ) return new          freezing_trap_t( this, options_str );
  if ( name == "boomstick"             ) return new              boomstick_t( this, options_str );
  if ( name == "harpoon"               ) return new                harpoon_t( this, options_str );
  if ( name == "hatchet_toss"          ) return new           hatchet_toss_t( this, options_str );
  if ( name == "harriers_cry"          ) return new           harriers_cry_t( this, options_str );
  if ( name == "moonlight_chakram"     ) return new      moonlight_chakram_t( this, options_str );
  if ( name == "multishot"             ) return new              multishot_t( this, options_str );
  if ( name == "muzzle"                ) return new                 muzzle_t( this, options_str );
  if ( name == "rapid_fire"            ) return new             rapid_fire_t( this, options_str );
  if ( name == "raptor_strike"         ) return new           raptor_strike_t( this, options_str );
  if ( name == "raptor_strike_eagle"   ) return new     raptor_strike_eagle_t( this, options_str );
  if ( name == "steady_shot"           ) return new            steady_shot_t( this, options_str );
  if ( name == "summon_pet"            ) return new             summon_pet_t( this, options_str );
  if ( name == "call_pet_1"            ) return new             summon_pet_t( this, options_str );
  if ( name == "takedown"              ) return new               takedown_t( this, options_str );
  if ( name == "tar_trap"              ) return new               tar_trap_t( this, options_str );
  if ( name == "trueshot"              ) return new               trueshot_t( this, options_str );
  if ( name == "volley"                ) return new                 volley_t( this, options_str );
  if ( name == "wailing_arrow"         ) return new          wailing_arrow_t( this, options_str );
  if ( name == "wild_thrash"           ) return new            wild_thrash_t( this, options_str );
  if ( name == "wildfire_bomb"         ) return new          wildfire_bomb_t( this, options_str );

  // Blizzard refers to Cobra Shot as Arcane Shot in their Assisted Combat system. 
  // We should consider if we want to do the same in the APL where we only allow 'arcane_shot' to be used for both spells.
  if ( name == "arcane_shot" )
  {
    if ( talents.cobra_shot.ok() )
      return new cobra_shot_t( this, options_str );
    else
      return new arcane_shot_t( this, options_str );
  }

  if ( name == "kill_shot" )
  {
    if ( !talents.black_arrow.ok() || specialization() == HUNTER_MARKSMANSHIP )
      return new kill_shot_t( this, options_str );
    else
      return new black_arrow_t( this, options_str );
  }

  if ( name == "kill_command" )
  {
    if ( specialization() == HUNTER_BEAST_MASTERY )
      return new kill_command_t( this, options_str, talents.kill_command_bm_player );
    if ( specialization()  == HUNTER_SURVIVAL )
      return new kill_command_t( this, options_str, talents.kill_command_sv_player );
  }

  return player_t::create_action( name, options_str );
}

pet_t* hunter_t::create_pet( util::string_view pet_name, util::string_view pet_type )
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
    throw sc_invalid_player_argument( fmt::format( "Pet '{}' has unknown type '{}'.", pet_name, pet_type ) );
  }

  return nullptr;
}

void hunter_t::create_pets()
{
  if ( !util::str_compare_ci( options.summon_pet_str, "disabled" ) )
    create_pet( options.summon_pet_str, options.summon_pet_str );

  if ( talents.animal_companion.ok() )
    pets.animal_companion = new pets::animal_companion_t( this );
}

void hunter_t::init()
{
  player_t::init();
}

double hunter_t::resource_loss( resource_e resource_type, double amount, gain_t* g, action_t* a )
{
  double loss = player_t::resource_loss(resource_type, amount, g, a);

  return loss;
}

void hunter_t::init_spells()
{
  player_t::init_spells();

  // Hunter Tree
  talents.combat_experience                 = find_talent_spell( talent_tree::CLASS, "Combat Experience" );

  talents.precision_strikes                 = find_talent_spell( talent_tree::CLASS, "Precision Strikes" );
  talents.counter_shot                      = find_talent_spell( talent_tree::CLASS, "Counter Shot" );
  talents.muzzle                            = find_talent_spell( talent_tree::CLASS, "Muzzle" );
  talents.serrated_tips                     = find_talent_spell( talent_tree::CLASS, "Serrated Tips" );

  talents.trigger_finger                    = find_talent_spell( talent_tree::CLASS, "Trigger Finger" );
  talents.tar_trap                          = find_talent_spell( talent_tree::CLASS, "Tar Trap" );

  talents.specialized_arsenal               = find_talent_spell( talent_tree::CLASS, "Specialized Arsenal" );

  talents.keen_eyesight                     = find_talent_spell( talent_tree::CLASS, "Keen Eyesight" );

  talents.unnatural_causes                  = find_talent_spell( talent_tree::CLASS, "Unnatural Causes" );
  talents.unnatural_causes_debuff           = talents.unnatural_causes.ok() ? find_spell( 459529 ) : spell_data_t::not_found();

  talents.blackrock_munitions               = find_talent_spell( talent_tree::CLASS, "Blackrock Munitions" );
  talents.born_to_be_wild                   = find_talent_spell( talent_tree::CLASS, "Born To Be Wild" );
  talents.improved_traps                    = find_talent_spell( talent_tree::CLASS, "Improved Traps" );
  
  // Beast Mastery Tree
  if ( specialization() == HUNTER_BEAST_MASTERY )
  {
    talents.kill_command_bm_player            = find_talent_spell( talent_tree::SPECIALIZATION, "Kill Command", HUNTER_BEAST_MASTERY );
    talents.kill_command_bm_pet               = talents.kill_command_bm_player.ok() ? find_spell( 83381 ) : spell_data_t::not_found();

    talents.animal_companion                  = find_talent_spell( talent_tree::SPECIALIZATION, "Animal Companion", HUNTER_BEAST_MASTERY );
    talents.solitary_companion                = find_talent_spell( talent_tree::SPECIALIZATION, "Solitary Companion", HUNTER_BEAST_MASTERY );
    talents.barbed_shot                       = find_talent_spell( talent_tree::SPECIALIZATION, "Barbed Shot", HUNTER_BEAST_MASTERY );
    talents.barbed_shot_buff                  = talents.barbed_shot.ok() ? find_spell( 246152 ) : spell_data_t::not_found();

    talents.alpha_predator                    = find_talent_spell( talent_tree::SPECIALIZATION, "Alpha Predator", HUNTER_BEAST_MASTERY );
    talents.dire_beast                        = find_talent_spell( talent_tree::SPECIALIZATION, "Dire Beast", HUNTER_BEAST_MASTERY );
    talents.stomp                             = find_talent_spell( talent_tree::SPECIALIZATION, "Stomp", HUNTER_BEAST_MASTERY );
    talents.stomp_dmg                         = find_spell( 201754 );
    talents.war_orders                        = find_talent_spell( talent_tree::SPECIALIZATION, "War Orders", HUNTER_BEAST_MASTERY );

    talents.wild_thrash_player                = find_talent_spell( talent_tree::SPECIALIZATION, "Wild Thrash", HUNTER_BEAST_MASTERY );
    talents.wild_thrash_pet                   = talents.wild_thrash_player.ok() ? find_spell( 1264355 ) : spell_data_t::not_found();
    talents.bestial_wrath                     = find_talent_spell( talent_tree::SPECIALIZATION, "Bestial Wrath", HUNTER_BEAST_MASTERY );
    talents.cobra_shot                        = find_talent_spell( talent_tree::SPECIALIZATION, "Cobra Shot", HUNTER_BEAST_MASTERY );
    talents.cobra_shot_data                   = find_spell( 193455 );

    talents.beast_cleave                      = find_talent_spell( talent_tree::SPECIALIZATION, "Beast Cleave", HUNTER_BEAST_MASTERY );
    talents.scent_of_blood                    = find_talent_spell( talent_tree::SPECIALIZATION, "Scent of Blood", HUNTER_BEAST_MASTERY );
    talents.thundering_hooves                 = find_talent_spell( talent_tree::SPECIALIZATION, "Thundering Hooves", HUNTER_BEAST_MASTERY );
    talents.go_for_the_throat                 = find_talent_spell( talent_tree::SPECIALIZATION, "Go for the Throat", HUNTER_BEAST_MASTERY );

    talents.laceration                        = find_talent_spell( talent_tree::SPECIALIZATION, "Laceration", HUNTER_BEAST_MASTERY );
    talents.laceration_driver                 = talents.laceration.ok() ? find_spell( 459555 ) : spell_data_t::not_found();
    talents.laceration_bleed                  = talents.laceration.ok() ? find_spell( 459560 ) : spell_data_t::not_found();
    talents.kill_cleave                       = find_talent_spell( talent_tree::SPECIALIZATION, "Kill Cleave", HUNTER_BEAST_MASTERY );
    talents.training_expert                   = find_talent_spell( talent_tree::SPECIALIZATION, "Training Expert", HUNTER_BEAST_MASTERY );
    talents.the_beast_within                  = find_talent_spell( talent_tree::SPECIALIZATION, "The Beast Within", HUNTER_BEAST_MASTERY );
    talents.thrill_of_the_hunt                = find_talent_spell( talent_tree::SPECIALIZATION, "Thrill of the Hunt", HUNTER_BEAST_MASTERY );
    talents.pack_tactics                      = find_talent_spell( talent_tree::SPECIALIZATION, "Pack Tactics", HUNTER_BEAST_MASTERY );
    talents.pack_tactics_energize             = talents.pack_tactics.ok() ? find_spell( 1282660 ) : spell_data_t::not_found();
    talents.barbed_scales                     = find_talent_spell( talent_tree::SPECIALIZATION, "Barbed Scales", HUNTER_BEAST_MASTERY );

    talents.aspect_of_the_beast               = find_talent_spell( talent_tree::SPECIALIZATION, "Aspect of the Beast", HUNTER_BEAST_MASTERY );
    talents.dire_cleave                       = find_talent_spell( talent_tree::SPECIALIZATION, "Dire Cleave", HUNTER_BEAST_MASTERY );
    talents.dire_command                      = find_talent_spell( talent_tree::SPECIALIZATION, "Dire Command", HUNTER_BEAST_MASTERY );
    talents.jagged_wounds                     = find_talent_spell( talent_tree::SPECIALIZATION, "Jagged Wounds", HUNTER_BEAST_MASTERY );
    talents.serpentine_strikes                = find_talent_spell( talent_tree::SPECIALIZATION, "Serpentine Strikes", HUNTER_BEAST_MASTERY );
    talents.serpentine_strikes_energize       = talents.serpentine_strikes.ok() ? find_spell( 1282710 ) : spell_data_t::not_found();
    talents.snakeskin_quiver                  = find_talent_spell( talent_tree::SPECIALIZATION, "Snakeskin Quiver", HUNTER_BEAST_MASTERY );
    talents.cobra_senses                      = find_talent_spell( talent_tree::SPECIALIZATION, "Cobra Senses", HUNTER_BEAST_MASTERY );

    talents.dire_frenzy                       = find_talent_spell( talent_tree::SPECIALIZATION, "Dire Frenzy", HUNTER_BEAST_MASTERY );
    talents.frenzy                            = find_talent_spell( talent_tree::SPECIALIZATION, "Frenzy", HUNTER_BEAST_MASTERY );
    talents.killer_instinct                   = find_talent_spell( talent_tree::SPECIALIZATION, "Killer Instinct", HUNTER_BEAST_MASTERY );

    talents.brutal_companion                  = find_talent_spell( talent_tree::SPECIALIZATION, "Brutal Companion", HUNTER_BEAST_MASTERY );
    talents.huntmasters_call                  = find_talent_spell( talent_tree::SPECIALIZATION, "Huntmaster's Call", HUNTER_BEAST_MASTERY );
    talents.heart_of_the_pack                 = find_talent_spell( talent_tree::SPECIALIZATION, "Heart of the Pack", HUNTER_BEAST_MASTERY );
    talents.heart_of_the_pack_buff            = talents.heart_of_the_pack.ok() ? find_spell( 1282747 ) : spell_data_t::not_found();
    talents.bloodshed                         = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodshed", HUNTER_BEAST_MASTERY );
    talents.bloodshed_dot                     = talents.bloodshed.ok() ? find_spell( 321538 ) : spell_data_t::not_found();
    talents.savagery_bm                       = find_talent_spell( talent_tree::SPECIALIZATION, "Savagery", HUNTER_BEAST_MASTERY );
    talents.killer_cobra                      = find_talent_spell( talent_tree::SPECIALIZATION, "Killer Cobra", HUNTER_BEAST_MASTERY );
    talents.master_handler                    = find_talent_spell( talent_tree::SPECIALIZATION, "Master Handler", HUNTER_BEAST_MASTERY );

    talents.wildspeaker                       = find_talent_spell( talent_tree::SPECIALIZATION, "Wildspeaker", HUNTER_BEAST_MASTERY );
    talents.wildspeaker_bestial_wrath         = talents.wildspeaker.ok() ? find_spell( 1235388 ) : spell_data_t::not_found();
    talents.wildspeaker_kill_command          = talents.wildspeaker.ok() ? find_spell( 1232922 ) : spell_data_t::not_found();
    talents.wild_instincts                    = find_talent_spell( talent_tree::SPECIALIZATION, "Wild Instincts", HUNTER_BEAST_MASTERY );
    talents.bloody_frenzy                     = find_talent_spell( talent_tree::SPECIALIZATION, "Bloody Frenzy", HUNTER_BEAST_MASTERY );
    talents.bloody_frenzy_buff                = talents.bloody_frenzy.ok() ? find_spell( 1265063 ) : spell_data_t::not_found();
    talents.piercing_fangs                    = find_talent_spell( talent_tree::SPECIALIZATION, "Piercing Fangs", HUNTER_BEAST_MASTERY );

    talents.natures_ally_1                    = find_talent_spell( talent_tree::SPECIALIZATION, "Nature's Ally", 1 );
    talents.natures_ally_1_summon             = talents.natures_ally_1.ok() ? find_spell( 1282474 ) : spell_data_t::not_found();
    talents.natures_ally_2                    = find_talent_spell( talent_tree::SPECIALIZATION, "Nature's Ally", 2 );
    talents.natures_ally_3                    = find_talent_spell( talent_tree::SPECIALIZATION, "Nature's Ally", 3 );
    talents.natures_ally_3_buff               = talents.natures_ally_3.ok() ? find_spell( 1276720 ) : spell_data_t::not_found();
  }

  // Marksmanship Tree
  if ( specialization() == HUNTER_MARKSMANSHIP )
  {
    specs.multishot                           = find_specialization_spell( "Multi-Shot" );
    specs.spotters_mark_data                  = find_specialization_spell( "Spotter's Mark" );
    specs.spotters_mark_debuff                = specs.spotters_mark_data.ok() ? find_spell( 466872 ) : spell_data_t::not_found();

    talents.aimed_shot                        = find_talent_spell( talent_tree::SPECIALIZATION, "Aimed Shot", HUNTER_MARKSMANSHIP );

    talents.rapid_fire                        = find_talent_spell( talent_tree::SPECIALIZATION, "Rapid Fire", HUNTER_MARKSMANSHIP );
    talents.rapid_fire_tick                   = talents.rapid_fire.ok() ? find_spell( 257045 ) : spell_data_t::not_found();
    talents.rapid_fire_energize               = talents.rapid_fire.ok() ? find_spell( 263585 ) : spell_data_t::not_found();
    talents.precise_shots                     = find_talent_spell( talent_tree::SPECIALIZATION, "Precise Shots", HUNTER_MARKSMANSHIP );
    talents.precise_shots_buff                = talents.precise_shots.ok() ? find_spell( 260242 ) : spell_data_t::not_found();

    talents.quick_draw                        = find_talent_spell( talent_tree::SPECIALIZATION, "Quick Draw", HUNTER_MARKSMANSHIP );
    talents.lock_and_load                     = find_talent_spell( talent_tree::SPECIALIZATION, "Lock and Load", HUNTER_MARKSMANSHIP );
    talents.lock_and_load_buff                = find_spell( 194594 );

    talents.surging_shots                     = find_talent_spell( talent_tree::SPECIALIZATION, "Surging Shots", HUNTER_MARKSMANSHIP );
    talents.avian_specialization              = find_talent_spell( talent_tree::SPECIALIZATION, "Avian Specialization", HUNTER_MARKSMANSHIP );
    talents.unbreakable_bond                  = find_talent_spell( talent_tree::SPECIALIZATION, "Unbreakable Bond", HUNTER_MARKSMANSHIP );
    talents.trick_shots                       = find_talent_spell( talent_tree::SPECIALIZATION, "Trick Shots", HUNTER_MARKSMANSHIP );
    talents.trick_shots_data                  = find_spell( 257621 );
    talents.trick_shots_buff                  = find_spell( 257622 );
    talents.aspect_of_the_hydra               = find_talent_spell( talent_tree::SPECIALIZATION, "Aspect of the Hydra", HUNTER_MARKSMANSHIP );

    talents.penetrating_shots                 = find_talent_spell( talent_tree::SPECIALIZATION, "Penetrating Shots", HUNTER_MARKSMANSHIP );
    talents.tenacious                         = find_talent_spell( talent_tree::SPECIALIZATION, "Tenacious", HUNTER_MARKSMANSHIP );
    talents.cunning                           = find_talent_spell( talent_tree::SPECIALIZATION, "Cunning", HUNTER_MARKSMANSHIP );
    talents.master_marksman                   = find_talent_spell( talent_tree::SPECIALIZATION, "Master Marksman", HUNTER_MARKSMANSHIP );
    talents.master_marksman_bleed             = talents.master_marksman.ok() ? find_spell( 269576 ) : spell_data_t::not_found();
    talents.light_ammo                        = find_talent_spell( talent_tree::SPECIALIZATION, "Light Ammo", HUNTER_MARKSMANSHIP );
    
    talents.obsidian_arrowhead                = find_talent_spell( talent_tree::SPECIALIZATION, "Obsidian Arrowhead", HUNTER_MARKSMANSHIP );
    talents.on_target                         = find_talent_spell( talent_tree::SPECIALIZATION, "On Target", HUNTER_MARKSMANSHIP );
    talents.trueshot                          = find_talent_spell( talent_tree::SPECIALIZATION, "Trueshot", HUNTER_MARKSMANSHIP );
    talents.kill_shot                         = find_talent_spell( talent_tree::SPECIALIZATION, "Kill Shot", HUNTER_MARKSMANSHIP );

    talents.explosive_shot                    = find_talent_spell( talent_tree::SPECIALIZATION, "Explosive Shot", HUNTER_MARKSMANSHIP );
    talents.explosive_shot_damage             = find_spell( 212680 );

    talents.precision_detonation              = find_talent_spell( talent_tree::SPECIALIZATION, "Precision Detonation", HUNTER_MARKSMANSHIP );
    talents.precision_detonation_buff         = talents.precision_detonation.ok() ? find_spell( 474199 ) : spell_data_t::not_found();
   
    talents.critical_precision                = find_talent_spell( talent_tree::SPECIALIZATION, "Critical Precision", HUNTER_MARKSMANSHIP );
    talents.no_scope                          = find_talent_spell( talent_tree::SPECIALIZATION, "No Scope", HUNTER_MARKSMANSHIP );
    talents.feathered_frenzy                  = find_talent_spell( talent_tree::SPECIALIZATION, "Feathered Frenzy", HUNTER_MARKSMANSHIP );
    talents.headshot                          = find_talent_spell( talent_tree::SPECIALIZATION, "Headshot", HUNTER_MARKSMANSHIP );
    talents.headshot_debuff                   = talents.headshot.ok() ? find_spell( 1277558 ) : spell_data_t::not_found();
    talents.deadeye                           = find_talent_spell( talent_tree::SPECIALIZATION, "Deadeye", HUNTER_MARKSMANSHIP );
    talents.deathblow                         = find_talent_spell( talent_tree::SPECIALIZATION, "Deathblow", HUNTER_MARKSMANSHIP );

    talents.unmatched_precision               = find_talent_spell( talent_tree::SPECIALIZATION, "Unmatched Precision", HUNTER_MARKSMANSHIP );
    talents.bullseye                          = find_talent_spell( talent_tree::SPECIALIZATION, "Bullseye", HUNTER_MARKSMANSHIP );
    talents.bullseye_buff                     = talents.bullseye->effectN( 1 ).trigger();
    talents.calling_the_shots                 = find_talent_spell( talent_tree::SPECIALIZATION, "Calling the Shots", HUNTER_MARKSMANSHIP );
    talents.unerring_vision                   = find_talent_spell( talent_tree::SPECIALIZATION, "Unerring Vision", HUNTER_MARKSMANSHIP );
    talents.small_game_hunter                 = find_talent_spell( talent_tree::SPECIALIZATION, "Small Game Hunter", HUNTER_MARKSMANSHIP );
    talents.eagles_accuracy                   = find_talent_spell( talent_tree::SPECIALIZATION, "Eagle's Accuracy", HUNTER_MARKSMANSHIP );

    talents.focused_aim                       = find_talent_spell( talent_tree::SPECIALIZATION, "Focused Aim", HUNTER_MARKSMANSHIP );
    talents.bulletstorm                       = find_talent_spell( talent_tree::SPECIALIZATION, "Bulletstorm", HUNTER_MARKSMANSHIP );
    talents.bulletstorm_buff                  = talents.bulletstorm.ok() ? find_spell( 389020 ) : spell_data_t::not_found();
    talents.tensile_bowstring                 = find_talent_spell( talent_tree::SPECIALIZATION, "Tensile Bowstring", HUNTER_MARKSMANSHIP );
    talents.volley                            = find_talent_spell( talent_tree::SPECIALIZATION, "Volley", HUNTER_MARKSMANSHIP );
    talents.volley_data                       = find_spell( 260243 );
    talents.volley_dmg                        = find_spell( 260247 );
    talents.focus_fire                        = find_talent_spell( talent_tree::SPECIALIZATION, "Focus Fire", HUNTER_MARKSMANSHIP );
    talents.focus_fire_buff                   = talents.focus_fire.ok() ? find_spell( 1277549 ) : spell_data_t::not_found();

    talents.windrunner_quiver                 = find_talent_spell( talent_tree::SPECIALIZATION, "Windrunner Quiver", HUNTER_MARKSMANSHIP );
    talents.incendiary_ammunition             = find_talent_spell( talent_tree::SPECIALIZATION, "Incendiary Ammunition", HUNTER_MARKSMANSHIP );
    talents.double_tap                        = find_talent_spell( talent_tree::SPECIALIZATION, "Double Tap", HUNTER_MARKSMANSHIP );
    talents.double_tap_buff                   = talents.double_tap.ok() ? find_spell( 260402 ) : spell_data_t::not_found();
    talents.salvo                             = find_talent_spell( talent_tree::SPECIALIZATION, "Salvo", HUNTER_MARKSMANSHIP );
    talents.shrapnel_shot                     = find_talent_spell( talent_tree::SPECIALIZATION, "Shrapnel Shot", HUNTER_MARKSMANSHIP );
    talents.unload                            = find_talent_spell( talent_tree::SPECIALIZATION, "Unload", HUNTER_MARKSMANSHIP );

    talents.take_aim_1                        = find_talent_spell( talent_tree::SPECIALIZATION, "Take Aim", 1 );
    talents.take_aim_2                        = find_talent_spell( talent_tree::SPECIALIZATION, "Take Aim", 2 );
    talents.take_aim_3                        = find_talent_spell( talent_tree::SPECIALIZATION, "Take Aim", 3 );
  }

  // Survival Tree
  if ( specialization() == HUNTER_SURVIVAL )
  {
    specs.aspect_of_the_eagle = find_spell( 186289 );
    specs.harpoon = find_spell( 190925 );
    specs.hatchet_toss = find_spell( 193265 );

    talents.kill_command_sv_player            = find_talent_spell( talent_tree::SPECIALIZATION, "Kill Command", HUNTER_SURVIVAL );
    talents.kill_command_sv_pet               = talents.kill_command_sv_player.ok() ? find_spell( 259277 ) : spell_data_t::not_found();

    talents.wildfire_bomb                     = find_talent_spell( talent_tree::SPECIALIZATION, "Wildfire Bomb", HUNTER_SURVIVAL );
    talents.wildfire_bomb_data                = find_spell( 259495 );
    talents.wildfire_bomb_dmg                 = find_spell( 265157 );
    talents.wildfire_bomb_dot                 = find_spell( 269747 );
    talents.raptor_strike                     = find_talent_spell( talent_tree::SPECIALIZATION, "Raptor Strike", HUNTER_SURVIVAL );
    talents.raptor_strike_eagle               = talents.raptor_strike.ok() ? find_spell( 265189 ) : spell_data_t::not_found();

    talents.raptor_swipe_1                    = find_talent_spell( talent_tree::SPECIALIZATION, "Raptor Swipe", 1 );
    talents.raptor_swipe_2                    = find_talent_spell( talent_tree::SPECIALIZATION, "Raptor Swipe", 2 );
    talents.raptor_swipe_3                    = find_talent_spell( talent_tree::SPECIALIZATION, "Raptor Swipe", 3 );
    talents.raptor_swipe_spell                = talents.raptor_swipe_1.ok() ? find_spell( 1262293 ) : spell_data_t::not_found();
    talents.raptor_swipe_buff                 = talents.raptor_swipe_1.ok() ? find_spell( 1273155 ) : spell_data_t::not_found();

    talents.guerrilla_tactics                 = find_talent_spell( talent_tree::SPECIALIZATION, "Guerrilla Tactics", HUNTER_SURVIVAL );
    talents.tip_of_the_spear                  = find_talent_spell( talent_tree::SPECIALIZATION, "Tip of the Spear", HUNTER_SURVIVAL );
    talents.tip_of_the_spear_buff             = talents.tip_of_the_spear.ok() ? find_spell( 260286 ) : spell_data_t::not_found();
    talents.tip_of_the_spear_boomstick_buff   = talents.tip_of_the_spear.ok() ? find_spell( 471536 ) : spell_data_t::not_found();
    talents.tip_of_the_spear_chakram_buff     = talents.tip_of_the_spear.ok() ? find_spell( 1280140 ) : spell_data_t::not_found();

    talents.lunge                             = find_talent_spell( talent_tree::SPECIALIZATION, "Lunge", HUNTER_SURVIVAL );
    talents.boomstick                         = find_talent_spell( talent_tree::SPECIALIZATION, "Boomstick", HUNTER_SURVIVAL );
    talents.strike_as_one                     = find_talent_spell( talent_tree::SPECIALIZATION, "Strike As One", HUNTER_SURVIVAL );
    talents.strike_as_one_dmg                 = talents.strike_as_one.ok() ? find_spell( 1251779 ) : spell_data_t::not_found();

    talents.shrapnel_bomb                     = find_talent_spell( talent_tree::SPECIALIZATION, "Shrapnel Bomb", HUNTER_SURVIVAL );
    talents.shrapnel_bomb_bleed               = talents.shrapnel_bomb.ok() ? find_spell( 1253171 ) : spell_data_t::not_found();
    talents.shrapnel_bomb_buff                = talents.shrapnel_bomb.ok() ? find_spell( 1292687 ) : spell_data_t::not_found();
    talents.flamebreak                        = find_talent_spell( talent_tree::SPECIALIZATION, "Flamebreak", HUNTER_SURVIVAL );
    talents.bloodseeker                       = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodseeker", HUNTER_SURVIVAL );
    talents.quick_reload                      = find_talent_spell( talent_tree::SPECIALIZATION, "Quick Reload", HUNTER_SURVIVAL );
    talents.flankers_advantage                = find_talent_spell( talent_tree::SPECIALIZATION, "Flanker's Advantage", HUNTER_SURVIVAL );
    talents.two_against_many                  = find_talent_spell( talent_tree::SPECIALIZATION, "Two Against Many", HUNTER_SURVIVAL );

    talents.mongoose_fury                     = find_talent_spell( talent_tree::SPECIALIZATION, "Mongoose Fury", HUNTER_SURVIVAL );
    talents.mongoose_fury_buff                = find_spell( 259388 ); // Mongoose Fury can be applied without being talented.
    talents.mongoose_rounds                   = find_talent_spell( talent_tree::SPECIALIZATION, "Mongoose Rounds", HUNTER_SURVIVAL );
    talents.wildfire_shells                   = find_talent_spell( talent_tree::SPECIALIZATION, "Wildfire Shells", HUNTER_SURVIVAL );
    talents.shellshock                        = find_talent_spell( talent_tree::SPECIALIZATION, "Shellshock", HUNTER_SURVIVAL );
    talents.sic_em                            = find_talent_spell( talent_tree::SPECIALIZATION, "Sic 'Em", HUNTER_SURVIVAL );
    talents.sic_em_bleed                      = talents.sic_em.ok() ? find_spell( 1253138 ) : spell_data_t::not_found();

    talents.bloody_claws                      = find_talent_spell( talent_tree::SPECIALIZATION, "Bloody Claws", HUNTER_SURVIVAL );
    talents.wallop                            = find_talent_spell( talent_tree::SPECIALIZATION, "Wallop", HUNTER_SURVIVAL );
    talents.wallop_buff                       = talents.wallop.ok() ? find_spell( 1252741 ) : spell_data_t::not_found();
    talents.improved_wildfire_bomb            = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Wildfire Bomb", HUNTER_SURVIVAL );
    talents.bonding                           = find_talent_spell( talent_tree::SPECIALIZATION, "Bonding", HUNTER_SURVIVAL );
    talents.sweeping_spear                    = find_talent_spell( talent_tree::SPECIALIZATION, "Sweeping Spear", HUNTER_SURVIVAL );
    talents.vulnerability                     = find_talent_spell( talent_tree::SPECIALIZATION, "Vulnerability", HUNTER_SURVIVAL );
    talents.blackrock_munitions               = find_talent_spell( talent_tree::SPECIALIZATION, "Blackrock Munitions", HUNTER_SURVIVAL );
    talents.shower_of_blood                   = find_talent_spell( talent_tree::SPECIALIZATION, "Shower of Blood", HUNTER_SURVIVAL );
    talents.outland_venom                     = find_talent_spell( talent_tree::SPECIALIZATION, "Outland Venom", HUNTER_SURVIVAL );
    talents.outland_venom_debuff              = talents.outland_venom.ok() ? find_spell( 459941 ) : spell_data_t::not_found();

    talents.explosives_expert                 = find_talent_spell( talent_tree::SPECIALIZATION, "Explosives Expert", HUNTER_SURVIVAL );
    talents.takedown                          = find_talent_spell( talent_tree::SPECIALIZATION, "Takedown", HUNTER_SURVIVAL );
    talents.takedown_energize                 = talents.takedown.ok() ? find_spell( 1258571 ) : spell_data_t::not_found();
    talents.takedown_dmg                      = talents.takedown.ok() ? find_spell( 1253859 ) : spell_data_t::not_found();
    talents.takedown_pet                      = talents.takedown.ok() ? find_spell( 1253862 ) : spell_data_t::not_found();
    talents.killer_companion                  = find_talent_spell( talent_tree::SPECIALIZATION, "Killer Companion", HUNTER_SURVIVAL );

    talents.flamefang_pitch                   = find_talent_spell( talent_tree::SPECIALIZATION, 1251592, HUNTER_SURVIVAL );
    talents.flamefang_pitch_data              = talents.flamefang_pitch.ok() ? find_spell( 1251610 ) : spell_data_t::not_found();
    talents.flamefang_pitch_dmg               = talents.flamefang_pitch.ok() ? find_spell( 1251595 ) : spell_data_t::not_found();
    talents.flamefang_pitch_aoe               = talents.flamefang_pitch.ok() ? find_spell( 1251614 ) : spell_data_t::not_found();
    talents.twin_fangs                        = find_talent_spell( talent_tree::SPECIALIZATION, "Twin Fangs", HUNTER_SURVIVAL );
    talents.savagery_sv                       = find_talent_spell( talent_tree::SPECIALIZATION, "Savagery", HUNTER_SURVIVAL );
    talents.wildfire_infusion                 = find_talent_spell( talent_tree::SPECIALIZATION, "Wildfire Infusion", HUNTER_SURVIVAL );

    talents.grenade_juggler                   = find_talent_spell( talent_tree::SPECIALIZATION, "Grenade Juggler", HUNTER_SURVIVAL );
    talents.wildfire_imbuement                = find_talent_spell( talent_tree::SPECIALIZATION, "Wildfire Imbuement", HUNTER_SURVIVAL );
    talents.wildfire_imbuement_dmg            = talents.wildfire_imbuement.ok() ? find_spell( 1252966 ) : spell_data_t::not_found();
    talents.wildfire_imbuement_buff           = talents.wildfire_imbuement.ok() ? find_spell( 1252947 ) : spell_data_t::not_found();
    talents.flanked                           = find_talent_spell( talent_tree::SPECIALIZATION, "Flanked", HUNTER_SURVIVAL );
    talents.lethal_calibration                = find_talent_spell( talent_tree::SPECIALIZATION, "Lethal Calibration", HUNTER_SURVIVAL );
    talents.primal_surge                      = find_talent_spell( talent_tree::SPECIALIZATION, "Primal Surge", HUNTER_SURVIVAL );
  }

  if ( specialization() == HUNTER_MARKSMANSHIP || specialization() == HUNTER_BEAST_MASTERY )
  {
    // Dark Ranger
    talents.black_arrow                 = find_talent_spell( talent_tree::HERO, "Black Arrow" );
    talents.black_arrow_spell           = talents.black_arrow.ok() ? find_spell( 466930 ) : spell_data_t::not_found();
    talents.black_arrow_dot             = talents.black_arrow.ok() ? find_spell( 468572 ) : spell_data_t::not_found();

    talents.bleak_arrows                = find_talent_spell( talent_tree::HERO, "Bleak Arrows" );
    talents.bleak_arrows_spell          = talents.bleak_arrows.ok() ? find_spell( 467718 ) : spell_data_t::not_found();
    talents.soul_drinker                = find_talent_spell( talent_tree::HERO, "Soul Drinker" );
    talents.deathblow_buff              = ( talents.deathblow.ok() || talents.black_arrow.ok() ) ? find_spell( 378770 ) : spell_data_t::not_found();
    talents.bleak_powder                = find_talent_spell( talent_tree::HERO, "Bleak Powder" );
    talents.bleak_powder_spell          = talents.bleak_powder.ok() ? ( specialization() == HUNTER_MARKSMANSHIP ? find_spell( 467914 ) : find_spell( 472084 ) ) : spell_data_t::not_found();
    talents.corpsecaller                = find_talent_spell( talent_tree::HERO, "Corpsecaller" );
    talents.corpsecaller_minion_summon  = specialization() == HUNTER_MARKSMANSHIP && talents.corpsecaller.ok() ? find_spell( 1264345 ) : spell_data_t::not_found();
    talents.corpsecaller_hound_summon   = specialization() == HUNTER_BEAST_MASTERY && talents.corpsecaller.ok() ? find_spell( 442419 ) : spell_data_t::not_found();

    talents.ebon_bowstring              = find_talent_spell( talent_tree::HERO, "Ebon Bowstring" );
    talents.wailing_dead                = find_talent_spell( talent_tree::HERO, "Wailing Dead" );
    talents.wailing_arrow               = talents.wailing_dead.ok() ? find_spell( 392060 ) : spell_data_t::not_found();
    talents.wailing_arrow_buff          = talents.wailing_dead.ok() ? find_spell( 459808 ) : spell_data_t::not_found();
    talents.wailing_arrow_damage        = talents.wailing_dead.ok() ? find_spell( 392058 ) : spell_data_t::not_found();

    talents.blighted_quiver             = find_talent_spell( talent_tree::HERO, "Blighted Quiver" );
    talents.banshees_mark               = find_talent_spell( talent_tree::HERO, "Banshee's Mark" );
    talents.the_bell_tolls              = find_talent_spell( talent_tree::HERO, "The Bell Tolls" );
    talents.umbral_reach                = find_talent_spell( talent_tree::HERO, "Umbral Reach" );
    talents.pact_of_the_hollow          = find_talent_spell( talent_tree::HERO, "Pact of the Hollow" );

    talents.withering_fire              = find_talent_spell( talent_tree::HERO, "Withering Fire" );
    talents.withering_fire_black_arrow  = talents.withering_fire.ok() ? find_spell( 468037 ) : spell_data_t::not_found();
    talents.withering_fire_buff         = talents.withering_fire.ok() ? find_spell( 466991 ) : spell_data_t::not_found();
  }

  if ( specialization() == HUNTER_BEAST_MASTERY || specialization() == HUNTER_SURVIVAL )
  {
    // Pack Leader
    talents.howl_of_the_pack_leader                       = find_talent_spell( talent_tree::HERO, "Howl of the Pack Leader" );
    talents.howl_of_the_pack_leader_wyvern_ready_buff     = talents.howl_of_the_pack_leader.ok() ? find_spell( 471878 ) : spell_data_t::not_found();
    talents.howl_of_the_pack_leader_boar_ready_buff       = talents.howl_of_the_pack_leader.ok() ? find_spell( 472324 ) : spell_data_t::not_found();
    talents.howl_of_the_pack_leader_bear_ready_buff       = talents.howl_of_the_pack_leader.ok() ? find_spell( 472325 ) : spell_data_t::not_found();
    talents.howl_of_the_pack_leader_cooldown_buff         = talents.howl_of_the_pack_leader.ok() ? find_spell( 471877 ) : spell_data_t::not_found();
    talents.howl_of_the_pack_leader_wyvern_summon         = talents.howl_of_the_pack_leader.ok() ? find_spell( 1222271 ) : spell_data_t::not_found();
    talents.howl_of_the_pack_leader_wyvern_buff           = talents.howl_of_the_pack_leader.ok() ? find_spell( 471881 ) : spell_data_t::not_found();
    talents.howl_of_the_pack_leader_boar_charge_trigger   = talents.howl_of_the_pack_leader.ok() ? find_spell( 472020 ) : spell_data_t::not_found();
    talents.howl_of_the_pack_leader_boar_charge_impact    = talents.howl_of_the_pack_leader.ok() ? find_spell( 471936 ) : spell_data_t::not_found();
    talents.howl_of_the_pack_leader_boar_charge_cleave    = talents.howl_of_the_pack_leader.ok() ? find_spell( 471938 ) : spell_data_t::not_found();
    talents.howl_of_the_pack_leader_bear_summon           = talents.howl_of_the_pack_leader.ok() ? find_spell( 471993 ) : spell_data_t::not_found();
    talents.howl_of_the_pack_leader_bear_buff             = talents.howl_of_the_pack_leader.ok() ? find_spell( 1225858 ) : spell_data_t::not_found();
    talents.howl_of_the_pack_leader_bear_bleed            = talents.howl_of_the_pack_leader.ok() ? find_spell( 471999 ) : spell_data_t::not_found();

    talents.pack_mentality                                = find_talent_spell( talent_tree::HERO, "Pack Mentality" );
    talents.dire_summons                                  = find_talent_spell( talent_tree::HERO, "Dire Summons" );
    talents.better_together                               = find_talent_spell( talent_tree::HERO, "Better Together" );

    talents.ursine_fury                                   = find_talent_spell( talent_tree::HERO, "Ursine Fury" );
    talents.dire_beast_summon                             = find_spell( 219199 );
    talents.sharpened_claws                               = find_talent_spell( talent_tree::HERO, "Sharpened Claws" );
    talents.fury_of_the_wyvern                            = find_talent_spell( talent_tree::HERO, "Fury of the Wyvern" );
    talents.hogstrider                                    = find_talent_spell( talent_tree::HERO, "Hogstrider" );
    talents.hogstrider_buff                               = talents.hogstrider.ok() ? find_spell( 472640 ) : spell_data_t::not_found();
    talents.lethal_barbs                                  = find_talent_spell( talent_tree::HERO, "Lethal Barbs" );
    talents.lethal_barbs_energize                         = talents.lethal_barbs.ok() ? find_spell( 1264783 ) : spell_data_t::not_found();
    proc_data_entries.lethal_barbs_energize               = talents.lethal_barbs.ok() ? find_spell( 1264783 ) : spell_data_t::not_found();

    talents.no_mercy                                      = find_talent_spell( talent_tree::HERO, "No Mercy" );
    talents.hoof_and_blade                                = find_talent_spell( talent_tree::HERO, "Hoof and Blade" );
    talents.wyverns_gaze                                  = find_talent_spell( talent_tree::HERO, "Wyvern's Gaze" );
    talents.sharpened_fangs                               = find_talent_spell( talent_tree::HERO, "Sharpened Fangs" );

    talents.stampede                                      = find_talent_spell( talent_tree::HERO, "Stampede!" );
    talents.stampede_incoming_buff                        = talents.stampede.ok() ? find_spell( 1258338 ) : spell_data_t::not_found();
    talents.stampede_trigger                              = talents.stampede.ok() ? find_spell( 1258344 ) : spell_data_t::not_found();
    talents.stampede_dmg                                  = talents.stampede.ok() ? find_spell( 201594 ) : spell_data_t::not_found();
  }

  if ( specialization() == HUNTER_MARKSMANSHIP || specialization() == HUNTER_SURVIVAL )
  {
    // Sentinel
    talents.sentinel                 = find_talent_spell( talent_tree::HERO, "Sentinel" );
    talents.sentinels_mark           = talents.sentinel.ok() ? find_spell( 1253601 ) : spell_data_t::not_found();

    talents.dont_look_back           = find_talent_spell( talent_tree::HERO, "Don't Look Back" );
    talents.moons_blessing           = find_talent_spell( talent_tree::HERO, "Moon's Blessing" );
    talents.sanctified_armaments     = find_talent_spell( talent_tree::HERO, "Sanctified Armaments" );
    talents.sanctified_armaments_dot = talents.sanctified_armaments.ok() ? find_spell( 1253836 ) : spell_data_t::not_found();
    talents.moonlight_chakram        = find_talent_spell( talent_tree::HERO, "Moonlight Chakram" );
    talents.moonlight_chakram_spell  = talents.moonlight_chakram.ok() ? find_spell( 1264949 ) : spell_data_t::not_found();
    talents.moonlight_chakram_damage = talents.moonlight_chakram.ok() ? find_spell( 1266081 ) : spell_data_t::not_found();
    talents.moonlight_chakram_buff   = talents.moonlight_chakram.ok() ? find_spell( 1264946 ) : spell_data_t::not_found();

    talents.stargazer                = find_talent_spell( talent_tree::HERO, "Stargazer" );
    talents.stargazer_buff           = talents.stargazer.ok() ? find_spell( 1253750 ) : spell_data_t::not_found();
    talents.open_fire                = find_talent_spell( talent_tree::HERO, "Open Fire" );
    talents.cant_miss_wont_miss      = find_talent_spell( talent_tree::HERO, "Can't Miss, Won't Miss" );
    talents.invigorating_pulse       = find_talent_spell( talent_tree::HERO, "Invigorating Pulse" );
    talents.twilight_requiem         = find_talent_spell( talent_tree::HERO, "Twilight Requiem" );
    talents.twilight_requiem_damage  = talents.twilight_requiem.ok() ? find_spell( 1266096 ) : spell_data_t::not_found();
    talents.stalk_and_strike         = find_talent_spell( talent_tree::HERO, "Stalk and Strike" );

    talents.arcane_talons            = find_talent_spell( talent_tree::HERO, "Arcane Talons" );
    talents.lunar_calling            = find_talent_spell( talent_tree::HERO, "Lunar Calling" );
    talents.radiant_edge             = find_talent_spell( talent_tree::HERO, "Radiant Edge" );
    
    talents.lunar_storm              = find_talent_spell( talent_tree::HERO, "Lunar Storm" );
    talents.lunar_storm_dmg          = talents.lunar_storm.ok() ? find_spell( 1253733 ) : spell_data_t::not_found();
  }

  // Mastery
  mastery.master_of_beasts     = find_mastery_spell( HUNTER_BEAST_MASTERY );
  mastery.sniper_training      = find_mastery_spell( HUNTER_MARKSMANSHIP );
  mastery.spirit_bond          = find_mastery_spell( HUNTER_SURVIVAL );
  mastery.spirit_bond_buff     = mastery.spirit_bond.ok() ? find_spell( 459722 ) : spell_data_t::not_found();

  // Spec spells
  specs.pet_damage           = find_spell( 1284992 );
  specs.hunter               = find_spell( 137014 );
  specs.beast_mastery_hunter = find_specialization_spell( "Beast Mastery Hunter" );
  specs.marksmanship_hunter  = find_specialization_spell( "Marksmanship Hunter" );
  specs.survival_hunter      = find_specialization_spell( "Survival Hunter" );

  specs.auto_shot            = find_spell( 75 );
  specs.freezing_trap        = find_class_spell( "Freezing Trap" );
  specs.arcane_shot          = talents.cobra_shot.ok() ? spell_data_t::not_found() : find_class_spell( "Arcane Shot" );
  specs.steady_shot          = talents.pack_tactics.ok() ?  spell_data_t::not_found() : find_class_spell( "Steady Shot" );
  specs.steady_shot_energize = find_spell( 77443 );
  specs.flare                = find_class_spell( "Flare" );
  specs.call_pet             = find_spell( 883 );

  // Tier Sets
  tier_set.mid_s1_bm_2pc        = sets->set( HUNTER_BEAST_MASTERY, MID1, B2 );
  tier_set.mid_s1_bm_4pc        = sets->set( HUNTER_BEAST_MASTERY, MID1, B4 );

  tier_set.mid_s1_mm_2pc        = sets->set( HUNTER_MARKSMANSHIP, MID1, B2 );
  tier_set.mid_s1_mm_4pc        = sets->set( HUNTER_MARKSMANSHIP, MID1, B4 );
  tier_set.mid_s1_mm_4pc_damage = tier_set.mid_s1_mm_4pc.ok() ? find_spell( 1271682 ) : spell_data_t::not_found();

  tier_set.mid_s1_sv_2pc        = sets->set( HUNTER_SURVIVAL, MID1, B2 );
  tier_set.mid_s1_sv_4pc        = sets->set( HUNTER_SURVIVAL, MID1, B4 );

  // Cooldowns
  cooldowns.salvo->duration = talents.volley->duration();
  cooldowns.shrapnel_shot->duration = talents.shrapnel_shot->internal_cooldown();

  cooldowns.dire_beast->duration = talents.dire_beast->internal_cooldown();

  cooldowns.strike_as_one->duration = talents.strike_as_one->internal_cooldown();

  cooldowns.bleak_powder->duration = talents.bleak_powder->internal_cooldown();

  // Register passives
  register_passive_effect_mask( talents.precision_strikes, 
                                specialization() == HUNTER_BEAST_MASTERY ||
                                specialization() == HUNTER_MARKSMANSHIP
                                                    ? effect_mask_t( true ).disable( 2 )
                                                    : effect_mask_t( true ).disable( 1 ) );

  register_passive_effect_mask( talents.better_together, 
                                specialization() == HUNTER_BEAST_MASTERY
                                                    ? effect_mask_t( true ).disable( 2, 3 )
                                                    : effect_mask_t( true ).disable( 1, 4 ) );

  register_passive_effect_mask( talents.no_mercy, 
                                specialization() == HUNTER_BEAST_MASTERY 
                                                    ? effect_mask_t( true ).disable( 2 )
                                                    : effect_mask_t( true ).disable( 1 ) );

  register_passive_effect_mask( talents.the_bell_tolls,
                                specialization() == HUNTER_BEAST_MASTERY
                                                    ? effect_mask_t( true ).disable( 1, 3 )
                                                    : effect_mask_t( true ).disable( 2, 4 ) );

  register_passive_effect_mask( talents.hoof_and_blade, effect_mask_t( true ).disable( 3 ) );

  deregister_passive_spell( talents.penetrating_shots );

  parse_all_class_passives();
  parse_all_passive_talents();
  parse_all_passive_sets();
  parse_raid_buffs();
}

void hunter_t::init_base_stats()
{
  if ( base.distance < 1 )
  {
    base.distance = 40;
    if ( specialization() == HUNTER_SURVIVAL )
      base.distance = 5;
  }

  base.attack_power_per_strength = 0;
  base.attack_power_per_agility  = 1;
  base.spell_power_per_intellect = 1;

  player_t::init_base_stats();
}

void hunter_t::create_actions()
{
  player_t::create_actions();

  if ( talents.laceration.ok() )
    actions.laceration = new attacks::laceration_t( this );
  
  if ( talents.howl_of_the_pack_leader.ok() )
    actions.boar_charge = new attacks::boar_charge_t( this );

  if ( talents.lunar_storm.ok() )
    actions.lunar_storm = new attacks::lunar_storm_t( this );

  if ( talents.snakeskin_quiver.ok() )
    actions.snakeskin_quiver = new attacks::cobra_shot_snakeskin_quiver_t( this );

  if ( talents.stampede.ok() )
    actions.stampede = new attacks::stampede_t( this );

  if ( talents.wild_instincts.ok() )
    actions.wild_instincts = new attacks::barbed_shot_wild_instincts_t( this );

  if ( tier_set.mid_s1_mm_4pc.ok() )
    actions.let_fly = new attacks::let_fly_t( this );
}

void hunter_t::create_buffs()
{
  player_t::create_buffs();

  // Hunter Tree

  buffs.deathblow = make_buff( this, "deathblow", talents.deathblow_buff );
  // By default, subject Deathblow to aura delay which allows queued casts to consume an existing Deathblow before a new Deathblow is applied.
  buffs.deathblow->activated = false;
  // By deafult, subject Deathblow to stack reaction, which allows may_react() in the ready().
  buffs.deathblow->reactable = true;

  // Marksmanship Tree

  buffs.precise_shots = 
    make_buff( this, "precise_shots", talents.precise_shots_buff )
      ->set_default_value_from_effect( 1 );

  buffs.trick_shots =
    make_buff( this, "trick_shots", talents.trick_shots_buff );
  
  buffs.lock_and_load =
    make_buff( this, "lock_and_load", talents.lock_and_load_buff )
      ->set_stack_change_callback(
        [ this ]( buff_t*, int _old, int _new ) {
          if ( _new > _old )
            cooldowns.aimed_shot->reset( true );
        } );

  buffs.trueshot =
    make_buff( this, "trueshot", talents.trueshot )
      ->set_cooldown( 0_s )
      ->set_refresh_behavior( buff_refresh_behavior::EXTEND )
      ->add_invalidate( cache_e::CACHE_CRIT_CHANCE )
      ->set_stack_change_callback(
        [ this ]( buff_t*, int, int ) {
          cooldowns.aimed_shot->adjust_recharge_multiplier();
          cooldowns.rapid_fire->adjust_recharge_multiplier();
        } );

  buffs.precision_detonation_hidden = 
    make_buff( this, "precision_detonation", talents.precision_detonation_buff )
      ->set_default_value_from_effect( 1 )
      ->set_quiet( true );

  buffs.bullseye =
    make_buff( this, "bullseye", talents.bullseye_buff )
      ->set_default_value_from_effect( 1 )
      ->set_chance( talents.bullseye.ok() );

  buffs.bulletstorm =
    make_buff( this, "bulletstorm", talents.bulletstorm_buff )
      ->set_default_value_from_effect( 1 )
      ->set_refresh_behavior( buff_refresh_behavior::DISABLED );

  buffs.double_tap =
    make_buff( this, "double_tap", talents.double_tap_buff )
      ->set_default_value_from_effect( 1 );

  buffs.volley =
    make_buff( this, "volley", talents.volley_data )
      -> set_cooldown( 0_ms )
      -> disable_ticking( true ) // disable ticks as an optimization
      -> set_refresh_behavior( buff_refresh_behavior::DURATION );

  buffs.focus_fire = 
    make_buff( this, "focus_fire", talents.focus_fire_buff )
      -> set_default_value_from_effect( 1 );

  // Beast Mastery Tree

  buffs.barbed_shot = 
    make_buff( this, "barbed_shot", talents.barbed_shot_buff )
      ->set_default_value( talents.barbed_shot_buff->effectN( 1 ).resource( RESOURCE_FOCUS ) )
      ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
      ->set_freeze_stacks( true )
      ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
            resource_gain( RESOURCE_FOCUS, b->check_stack_value(), gains.barbed_shot, actions.barbed_shot );
          } );

  buffs.bestial_wrath =
    make_buff( this, "bestial_wrath", talents.bestial_wrath )
      -> set_cooldown( 0_ms )
      -> set_default_value_from_effect( 1 );

  buffs.beast_cleave = 
    make_buff( this, "beast_cleave", find_spell( 268877 ) )
    -> set_duration( talents.beast_cleave -> effectN( 2 ).time_value() );

  buffs.huntmasters_call = 
    make_buff( this, "huntmasters_call", find_spell( 459731 ) );

  buffs.summon_fenryr = 
    make_buff( this, "summon_fenryr", find_spell ( 459735 ) )
    -> set_default_value_from_effect( 2 )
    -> set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buffs.summon_hati = 
    make_buff( this, "summon_hati", find_spell( 459738 ) )
      -> add_invalidate( CACHE_PET_DAMAGE_MULTIPLIER )
      -> set_default_value_from_effect( 2 );

  buffs.heart_of_the_pack = 
    make_buff( this, "heart_of_the_pack", talents.heart_of_the_pack_buff )
      -> set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
      -> set_default_value( talents.heart_of_the_pack->effectN( 1 ).percent() / 10 ) // Spelldata is scuffed as of 2026-01-08
      -> set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buffs.natures_ally_3 = 
    make_buff( this, "natures_ally", talents.natures_ally_3_buff )
      -> set_default_value( talents.natures_ally_3_buff->effectN( 1 ).percent() );

  buffs.bloody_frenzy = 
    make_buff( this, "bloody_frenzy", talents.bloody_frenzy_buff )
      -> set_default_value_from_effect( 1 );

  // Survival Tree

  buffs.tip_of_the_spear =
    make_buff( this, "tip_of_the_spear", talents.tip_of_the_spear_buff )
      ->set_default_value_from_effect( 1 )
      ->set_chance( talents.tip_of_the_spear.ok() );

  buffs.tip_of_the_spear_boomstick =
    make_buff( this, "tip_of_the_spear_boomstick", talents.tip_of_the_spear_boomstick_buff )
      ->set_default_value_from_effect( 1 )
      ->set_chance( talents.tip_of_the_spear.ok() );

  buffs.tip_of_the_spear_chakram = 
    make_buff( this, "tip_of_the_spear_chakram", talents.tip_of_the_spear_chakram_buff )
      ->set_default_value_from_effect( 1 )
      ->set_chance( talents.tip_of_the_spear.ok() );
  
  buffs.mongoose_fury =
    make_buff( this, "mongoose_fury", talents.mongoose_fury_buff )
      ->set_default_value_from_effect( 1 )
      ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
      ->set_stack_change_callback( [ this ]( buff_t*, int old, int cur ) {
        if ( cur > old && rng().roll( talents.wallop->effectN( 1 ).percent() ) )
          buffs.wallop->trigger();
      } );

  buffs.bloodseeker =
    make_buff( this, "bloodseeker", find_spell( 260249 ) )
      -> set_default_value_from_effect( 1 )
      -> add_invalidate( CACHE_AUTO_ATTACK_SPEED );

  buffs.aspect_of_the_eagle =
    make_buff( this, "aspect_of_the_eagle", specs.aspect_of_the_eagle )
      -> set_cooldown( 0_ms );

  buffs.takedown = 
    make_buff( this, "takedown", talents.takedown )
      ->set_default_value_from_effect( 1 )
      ->set_cooldown( 0_ms )
      ->add_invalidate( CACHE_AUTO_ATTACK_SPEED )
      ->add_invalidate( CACHE_PET_DAMAGE_MULTIPLIER );

  buffs.wallop = 
    make_buff( this, "wallop", talents.wallop_buff )
      ->set_default_value_from_effect( 1 );

  buffs.wildfire_imbuement = 
    make_buff( this, "wildfire_imbuement", talents.wildfire_imbuement_buff );

  buffs.raptor_swipe = 
    make_buff( this, "raptor_swipe", talents.raptor_swipe_buff );

  buffs.shrapnel_bomb = 
    make_buff( this, "shrapnel_bomb", talents.shrapnel_bomb_buff )
      ->set_default_value( talents.shrapnel_bomb_buff->effectN( 1 ).resource( RESOURCE_FOCUS ) )
      ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
        resource_gain( RESOURCE_FOCUS, b->check_value(), gains.shrapnel_bomb );
      } );

  // Pet family buffs

  buffs.endurance_training =
    make_buff( this, "endurance_training", find_spell( 264662 ) )
      -> set_default_value_from_effect( 2 )
      -> set_stack_change_callback(
          []( buff_t* b, int old, int cur ) {
            player_t* p = b -> player;
            if ( cur == 0 )
              p -> resources.initial_multiplier[ RESOURCE_HEALTH ] /= 1 + b -> default_value;
            else if ( old == 0 )
              p -> resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1 + b -> default_value;
            p -> recalculate_resource_max( RESOURCE_HEALTH );
          } );

  buffs.pathfinding =
    make_buff( this, "pathfinding", find_spell( 264656 ) )
      -> set_default_value_from_effect( 2 )
      -> add_invalidate( CACHE_RUN_SPEED );

  buffs.predators_thirst =
    make_buff( this, "predators_thirst", find_spell( 264663 ) )
      -> set_default_value_from_effect( 2 )
      -> add_invalidate( CACHE_LEECH );

  // Tier Set Bonuses

  // Hero Talents

  buffs.howl_of_the_pack_leader_wyvern = 
    make_buff( this, "howl_of_the_pack_leader_wyvern", talents.howl_of_the_pack_leader_wyvern_ready_buff )
      ->set_stack_change_callback(
        [ this ]( buff_t*, int, int cur ) {
          if ( cur == 0 && !buffs.howl_of_the_pack_leader_cooldown->check() )
            buffs.howl_of_the_pack_leader_cooldown->trigger();
        } );

  buffs.howl_of_the_pack_leader_boar = 
    make_buff( this, "howl_of_the_pack_leader_boar", talents.howl_of_the_pack_leader_boar_ready_buff )
      ->set_stack_change_callback(
        [ this ]( buff_t*, int, int cur ) {
          if ( cur == 0 && !buffs.howl_of_the_pack_leader_cooldown->check() )
            buffs.howl_of_the_pack_leader_cooldown->trigger();
        } );

  buffs.howl_of_the_pack_leader_bear = 
    make_buff( this, "howl_of_the_pack_leader_bear", talents.howl_of_the_pack_leader_bear_ready_buff )
      ->set_stack_change_callback(
        [ this ]( buff_t*, int, int cur ) {
          if ( cur == 0 && !buffs.howl_of_the_pack_leader_cooldown->check() )
            buffs.howl_of_the_pack_leader_cooldown->trigger();
        } );

  buffs.howl_of_the_pack_leader_cooldown = 
    make_buff( this, "howl_of_the_pack_leader_cooldown", talents.howl_of_the_pack_leader_cooldown_buff )
      ->set_stack_change_callback(
        [ this ]( buff_t*, int, int cur ) {
          if ( cur == 0 )
            trigger_howl_of_the_pack_leader();
        } );

  buffs.wyverns_cry = 
    make_buff( this, "wyverns_cry", talents.howl_of_the_pack_leader_wyvern_buff )
      ->set_default_value_from_effect( 1 )
      ->add_invalidate( CACHE_PET_DAMAGE_MULTIPLIER )
      ->set_stack_change_callback(
        [ this ]( buff_t*, int, int cur ) {
          // Expiration only because...
          if ( cur == 0 ) 
          {
            state.fury_of_the_wyvern_extension = 0_s;
            state.fury_of_the_wyvern_extendable = false;
          }
          // ...Wyvern's Cry starts at a variable stack count
          else if ( cur == as<int>( talents.howl_of_the_pack_leader->effectN( 3 ).base_value() ) ) 
          {
            state.fury_of_the_wyvern_extendable = talents.fury_of_the_wyvern.ok();
          }
        } );

  buffs.hogstrider =
    make_buff( this, "hogstrider", talents.hogstrider_buff )
      ->set_default_value_from_effect( 1 );

  buffs.stampede_incoming = 
    make_buff( this, "stampede_incoming", talents.stampede_incoming_buff )
      ->set_default_value_from_effect( 1 );

  buffs.stampede = 
        make_buff( this, "stampede", talents.stampede_trigger );
  
  buffs.stargazer = 
    make_buff( this, "stargazer", talents.stargazer_buff )
      ->set_default_value_from_effect( 1 )
      ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );

  buffs.moonlight_chakram = 
    make_buff( this, "moonlight_chakram", talents.moonlight_chakram_buff );

  buffs.withering_fire =
    make_buff( this, "withering_fire", talents.withering_fire_buff );

  buffs.wailing_arrow = 
    make_buff( this, "wailing_arrow", talents.wailing_arrow_buff );
}

void hunter_t::init_gains()
{
  player_t::init_gains();

  gains.barbed_shot               = get_gain( "Barbed Shot" );
  gains.pack_tactics              = get_gain( "Pack Tactics" );
  gains.invigorating_pulse        = get_gain( "Invigorating Pulse" );
  gains.serpentine_strikes        = get_gain( "Serpentine Strikes" );
  gains.lethal_barbs              = get_gain( "Lethal Barbs" );
  gains.shrapnel_bomb             = get_gain( "Shrapnel Bomb" );
}

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

void hunter_t::init_procs()
{
  player_t::init_procs();

  if ( talents.dire_command.ok() )
    procs.dire_command = get_proc( "Dire Command" );

  if ( talents.snakeskin_quiver.ok() )
    procs.snakeskin_quiver = get_proc( "Snakeskin Quiver" );

  if ( talents.deathblow_buff.ok() )
    procs.deathblow = get_proc( "Deathblow" );

  if ( talents.sentinels_mark.ok() )
    procs.eagles_mark = get_proc( "Sentinel's Mark" );
  else if ( specs.spotters_mark_data.ok() )
    procs.eagles_mark = get_proc( "Spotter's Mark" );

  procs.windrunner_quiver = get_proc( "Windrunner Quiver" );

  procs.precision_detonation = get_proc( "Precision Detonation" );

  if ( talents.dire_beast_summon.ok() )
    procs.dire_beast_spawn = get_proc( "Dire Beast" );

  if ( talents.corpsecaller_minion_summon.ok() )
    procs.dark_minion_spawn = get_proc( "Dark Minion" );

  if ( talents.corpsecaller_hound_summon.ok() )
    procs.dark_hound_spawn = get_proc( "Dark Hound" );
}

void hunter_t::init_rng()
{
  player_t::init_rng();
  
  rppm.corpsecaller = get_rppm( "Corpsecaller", talents.corpsecaller );
  rppm.let_fly      = get_rppm( "Let Fly", tier_set.mid_s1_mm_4pc );

  accumulated_rng.dire_command = get_accumulated_rng( "Dire Command", prd::find_constant( talents.dire_command->effectN( 1 ).percent() ) );
}

void hunter_t::init_scaling()
{
  player_t::init_scaling();

  scaling -> disable( STAT_STRENGTH );
}

void hunter_t::init_assessors()
{
  player_t::init_assessors();
}

void hunter_t::init_action_list()
{
  if ( main_hand_weapon.group() == WEAPON_RANGED )
  {
    const weapon_e type = main_hand_weapon.type;
    if ( type != WEAPON_BOW && type != WEAPON_CROSSBOW && type != WEAPON_GUN )
    {
      if ( specialization() == HUNTER_SURVIVAL )
      {
        sim->error( "{} does not have a proper weapon type at the Main Hand slot: {}.", *this,
                    util::weapon_subclass_string( items[ main_hand_weapon.slot ].parsed.data.item_subclass ) );
      }
      else
      {
        throw sc_initialization_error(
          fmt::format( "{} does not have a proper weapon type at the Main Hand slot: {}.", *this,
                       util::weapon_subclass_string( items[ main_hand_weapon.slot ].parsed.data.item_subclass ) ) );
      }
    }
  }

  if ( specialization() == HUNTER_SURVIVAL )
  {
    const weapon_e mh_group = main_hand_weapon.group();
    if ( mh_group != WEAPON_2H && mh_group != WEAPON_1H && mh_group != WEAPON_SMALL )
      sim->error( "Player {} does not have a proper weapon at the Main Hand slot: {}.", name(), main_hand_weapon.type );

    if ( off_hand_attack )
    {
      const weapon_e oh_group = off_hand_weapon.group();
      if ( oh_group != WEAPON_1H && oh_group != WEAPON_SMALL )
        sim->error( "Player {} does not have a proper weapon at the Off Hand slot: {}.", name(), off_hand_weapon.type );
    }
  }

  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    switch ( specialization() )
    {
    case HUNTER_BEAST_MASTERY:
      if ( is_ptr() )
        hunter_apl::beast_mastery_ptr( this );
      else
        hunter_apl::beast_mastery( this );
      break;
    case HUNTER_MARKSMANSHIP:
      if ( is_ptr() )
        hunter_apl::marksmanship_ptr( this );
      else
        hunter_apl::marksmanship( this );
      break;
    case HUNTER_SURVIVAL:
      if ( is_ptr() )
        hunter_apl::survival_ptr( this );
      else
        hunter_apl::survival( this );
      break;
    default:
      get_action_priority_list( "default" ) -> add_action( "arcane_shot" );
      break;
    }

    use_default_action_list = true;
  }

  player_t::init_action_list();
}

void hunter_t::init_blizzard_action_list()
{
  action_priority_list_t* default_ = get_action_priority_list( "default" );
  // Added before generating the other action lists so its always the highest priority and gets executed at the start of combat.
  default_->add_action( specialization() == HUNTER_SURVIVAL ? "auto_attack" : "auto_shot" );

  player_t::init_blizzard_action_list();

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  switch ( specialization() )
  {
    case HUNTER_MARKSMANSHIP:
      precombat->add_action( "summon_pet,if=talent.unbreakable_bond" );
      break;
    default:
      precombat->add_action( "summon_pet" );
      break;
  }

  action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );
  switch ( specialization() )
  {
    case HUNTER_BEAST_MASTERY:
      break;
    case HUNTER_MARKSMANSHIP:
      cooldowns->add_action( "trueshot" );
      break;
    case HUNTER_SURVIVAL:
      cooldowns->add_action( "takedown" );
      break;
    default:
      break;
  }
}

parsed_assisted_combat_rule_t hunter_t::parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule,
                                                const assisted_combat_step_data_t& step ) const
{
  return player_t::parse_assisted_combat_rule( rule, step );
}

std::vector<std::string> hunter_t::action_names_from_spell_id( unsigned int spell_id ) const
{
  if ( spell_id == 53351 )
  {
    return { "kill_shot", "black_arrow" };
  }

  if( spell_id == 19574 )
  {
    return { "bestial_wrath", "wailing_arrow" };
  }

  return player_t::action_names_from_spell_id( spell_id );
}

void hunter_t::parse_assisted_combat_step( const assisted_combat_step_data_t& step, action_priority_list_t* assisted_combat )
{
  // Revive Pet is not an action relevant for SimC, so we don't add it to the assisted combat list.
  if ( step.spell_id == 982 )
  {
    return;
  }

  return player_t::parse_assisted_combat_step( step, assisted_combat );
}

void hunter_t::init_special_effects()
{
  player_t::init_special_effects();

  if ( talents.bullseye.ok() )
  {
    struct bullseye_cb_t : public dbc_proc_callback_t
    {
      double threshold;

      bullseye_cb_t( const special_effect_t& e, double threshold ) : dbc_proc_callback_t( e.player, e ),
        threshold( threshold )
      {
      }

      void trigger( const proc_data_t& data, player_t* t, action_state_t* s, proc_trigger_type_e type ) override
      {
        if ( t -> health_percentage() >= threshold )
          return;

        dbc_proc_callback_t::trigger( data, t, s, type );
      }
    };

    auto const effect = new special_effect_t( this );
    effect -> name_str = "bullseye";
    effect -> spell_id = talents.bullseye -> id();
    effect -> custom_buff = buffs.bullseye;
    effect -> proc_flags2_ = PF2_ALL_HIT;
    special_effects.push_back( effect );

    auto cb = new bullseye_cb_t( *effect, talents.bullseye -> effectN( 1 ).base_value() );
    cb -> initialize();
  }

  if ( talents.master_marksman.ok() )
  {
    struct master_marksman_cb_t : public dbc_proc_callback_t
    {
      double bleed_amount;
      action_t* bleed;

      master_marksman_cb_t( const special_effect_t& e, double amount, action_t* bleed ) : dbc_proc_callback_t( e.player, e ),
        bleed_amount( amount ), bleed( bleed )
      {
      }

      void execute( const spell_data_t* spell, player_t* t, action_state_t* s ) override
      {
        dbc_proc_callback_t::execute( spell, t, s );

        double amount = s -> result_amount * bleed_amount;
        if ( amount > 0 )
          residual_action::trigger( bleed, s -> target, amount );
      }
    };

    auto const effect = new special_effect_t( this );
    effect -> name_str = "master_marksman";
    effect -> spell_id = talents.master_marksman -> id();
    effect -> proc_flags2_ = PF2_CRIT;
    special_effects.push_back( effect );

    auto cb = new master_marksman_cb_t( *effect, talents.master_marksman -> effectN( 1 ).percent(), new attacks::master_marksman_t( this ) );
    cb -> initialize();
  }
}

void hunter_t::init_finished()
{
  player_t::init_finished();
}

void hunter_t::reset()
{
  player_t::reset();

  // Active
  pets.main = nullptr;
  state = {};
}

void hunter_t::merge( player_t& other )
{
  player_t::merge( other );

  cd_waste.merge( static_cast<hunter_t&>( other ).cd_waste );
}

void hunter_t::arise()
{
  player_t::arise();
}

void hunter_t::combat_begin()
{
  if ( talents.bloodseeker.ok() && sim -> player_no_pet_list.size() > 1 )
  {
    make_repeating_event( *sim, 1_s, [ this ] { trigger_bloodseeker_update(); } );
  }

  if ( talents.outland_venom.ok() )
    make_repeating_event( *sim, talents.outland_venom_debuff->effectN( 2 ).period(),
                          [ this ] { trigger_outland_venom_update(); } );

  buffs.howl_of_the_pack_leader_cooldown->trigger();

  player_t::combat_begin();
}

bool hunter_t::validate_actor()
{
  return true;
}

void hunter_t::datacollection_begin()
{
  if ( active_during_iteration )
    cd_waste.datacollection_begin();

  player_t::datacollection_begin();
}

void hunter_t::datacollection_end()
{
  if ( requires_data_collection() )
    cd_waste.datacollection_end();

  player_t::datacollection_end();
}

double hunter_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_AGILITY )
  {
    if ( talents.lunge.ok() && dual_wield() )
    {
      // TODO confirm if the dual-wield bonus is additive or multiplicative, assuming the latter for now.
      m *= 1 + talents.lunge->effectN( 2 ).percent();
    }
  }

  return m;
}

double hunter_t::composite_melee_crit_chance() const
{
  double crit = player_t::composite_melee_crit_chance();

  if ( buffs.trueshot->check() )
    crit += talents.trueshot->effectN( 3 ).percent();

  return crit;
}

double hunter_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  if ( buffs.trueshot->check() )
    crit += talents.trueshot->effectN( 3 ).percent();

  return crit;
}

double hunter_t::composite_rating_multiplier( rating_e r ) const
{
  double rm = player_t::composite_rating_multiplier( r );

  return rm;
}

double hunter_t::composite_melee_auto_attack_speed() const
{
  double s = player_t::composite_melee_auto_attack_speed();

  if ( buffs.bloodseeker->check() )
    s /= 1 + buffs.bloodseeker->check_stack_value();

  // Only need to apply here as the pet inherits this bonus.
  if ( buffs.takedown->check() )
    s /= 1 + talents.takedown->effectN( 4 ).percent();

  return s;
}

double hunter_t::composite_player_critical_damage_multiplier( const action_state_t* s, school_e school ) const
{
  double m = player_t::composite_player_critical_damage_multiplier( s, school );

  if ( talents.penetrating_shots -> effectN( 1 ).has_common_school( school ) )
    m *= 1.0 + talents.penetrating_shots -> effectN( 2 ).percent() * cache.attack_crit_chance();

  return m;
}

double hunter_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  return m;
}

double hunter_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double d = player_t::composite_player_target_multiplier( target, school );

  return d;
}

double hunter_t::composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s, guardian );

  if ( mastery.master_of_beasts->ok() )
    m *= 1.0 + cache.mastery_value();

  if ( !guardian )
  {
    if ( mastery.spirit_bond->ok() )
      m *= 1.0 + cache.mastery_value() * ( 1 + mastery.spirit_bond_buff->effectN( 1 ).percent() );

    if ( buffs.takedown->check() )
      m *= 1 + talents.takedown->effectN( 3 ).percent();

    m *= 1 + buffs.summon_hati->check_value();
    m *= 1 + buffs.wyverns_cry->check_stack_value();
  }

  return m;
}

double hunter_t::composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const
{
  double m = player_t::composite_player_target_pet_damage_multiplier( target, guardian );

  return m;
}

double hunter_t::composite_leech() const
{
  double l = player_t::composite_leech();

  l += buffs.predators_thirst -> check_value();

  return l;
}

void hunter_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
  case CACHE_MASTERY:
    if ( sim -> distance_targeting_enabled && mastery.sniper_training.ok() )
    {
      // Marksman is a unique butterfly, since mastery changes the max range of abilities.
      // We need to regenerate every target cache.
      // XXX: Do we? We don't change action range anywhere.
      for ( action_t* action : action_list )
        action -> target_cache.is_valid = false;
    }
    if( specialization() == HUNTER_BEAST_MASTERY && mastery.master_of_beasts.ok() )
    {
      invalidate_cache( CACHE_PET_DAMAGE_MULTIPLIER );
      invalidate_cache( CACHE_GUARDIAN_DAMAGE_MULTIPLIER );
    }
    if( specialization() == HUNTER_SURVIVAL && mastery.spirit_bond.ok() )
    {
      invalidate_cache( CACHE_PET_DAMAGE_MULTIPLIER );
    }
    break;
  default: break;
  }
}

void hunter_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( resources.is_infinite( RESOURCE_FOCUS ) )
    return;
}

double hunter_t::resource_gain( resource_e type, double amount, gain_t* g, action_t* action )
{
  double actual_amount = player_t::resource_gain( type, amount, g, action );

  if ( action && type == RESOURCE_FOCUS && amount > 0 )
  {
    /**
     * If the gain event has an action specified we treat it as an "energize" effect.
     * Focus energize effects are a bit special in that they can grant only integral amounts
     * of focus flooring the total calculated amount.
     * That means we can't just simply multiply stuff and trigger gains in the presence of non-integral
     * mutipliers. Which Trueshot is, at 50%. We have to calculate the fully multiplied value, floor
     * that and distribute the amounts & gains accordingly.
     * To keep gains attribution "fair" we distribute the additional gain to all of the present
     * multipliers according to their "weight".
     */

    assert( g != player_t::gains.resource_regen[ type ] );

    std::array<std::pair<double, gain_t*>, 3> mul_gains;
    size_t mul_gains_count = 0;
    double mul_sum = 0;

    const double initial_amount = floor( amount );
    amount = initial_amount;

    const double additional_amount = floor( amount ) - initial_amount;
    if ( additional_amount > 0 )
    {
      for ( const auto& data : util::make_span( mul_gains ).subspan( 0, mul_gains_count ) )
        actual_amount += player_t::resource_gain( RESOURCE_FOCUS, additional_amount * ( data.first / mul_sum ), data.second, action );
    }
  }

  return actual_amount;
}

double hunter_t::matching_gear_multiplier( attribute_e attr ) const
{
  return player_t::matching_gear_multiplier( attr );
}

double hunter_t::stacking_movement_modifier() const
{
  double ms = player_t::stacking_movement_modifier();

  ms += buffs.pathfinding -> check_value();

  return ms;
}

void hunter_t::create_options()
{
  player_t::create_options();

  add_option( opt_string( "summon_pet", options.summon_pet_str ) );
  add_option( opt_timespan( "hunter.pet_attack_speed", options.pet_attack_speed, 0.5_s, 4_s ) );
  add_option( opt_timespan( "hunter.pet_basic_attack_delay", options.pet_basic_attack_delay, 0_ms, 0.6_s ) );
  add_option( opt_bool( "max_prio_damage", options.max_prio_damage ) );
}

std::string hunter_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  const options_t defaults{};
  auto print_option = [&] ( auto ref, util::string_view name ) {
    if ( std::invoke( ref, options ) != std::invoke( ref, defaults ) )
      fmt::format_to( std::back_inserter( profile_str ), "{}={}\n", name, std::invoke( ref, options ) );
  };

  print_option( &options_t::summon_pet_str, "summon_pet" );
  print_option( &options_t::pet_attack_speed, "hunter.pet_attack_speed" );
  print_option( &options_t::pet_basic_attack_delay, "hunter.pet_basic_attack_delay" );

  return profile_str;
}

void hunter_t::copy_from( player_t* source )
{
  player_t::copy_from( source );
  options = debug_cast<hunter_t*>( source ) -> options;
}

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
  case STAT_STR_INT:
    return STAT_NONE;
  case STAT_SPIRIT:
    return STAT_NONE;
  case STAT_BONUS_ARMOR:
    return STAT_NONE;
  default: return s;
  }
}

void hunter_t::moving()
{
  // Override moving() so that it doesn't suppress auto_shot and only interrupts the few shots that cannot be used while moving.
  if ( ( executing && !executing -> usable_moving() ) || ( channeling && !channeling -> usable_moving() ) )
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

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new hunter_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new hunter_report_t( *p ) );
    return p;
  }

  bool valid() const override { return true; }

  void register_hotfixes() const override
  {
    // 2026-02-02: Radiant Edge is missing 10% from its base value
    hotfix::register_effect( "Hunter", "2026-02-02", "Radiant Edge Bonus", 1276513 )
        .field( "base_value" )
        .operation( hotfix::HOTFIX_SET )
        .modifier( 25 )
        .verification_value( 15 );
  }

  void register_actor_initializers( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::hunter()
{
  static hunter_module_t m;
  return &m;
}
