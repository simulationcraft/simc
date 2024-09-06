//==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <memory>

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
  bool affected = a -> data().affected_by( effect );
  if ( affected && a -> sim -> debug )
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
      print_affected_by( a, effect, "damage increase" );
      return affected_by;
    }
    
    if ( effect.subtype() != A_ADD_PCT_MODIFIER || !a->data().affected_by( effect ) )
      continue;

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

// in-game the buffs are actually 8 distinct spells, so the player can't get more than 8 simultaneously
constexpr unsigned BARBED_SHOT_BUFFS_MAX = 8;

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
struct dire_critter_t;
struct dark_hound_t;
struct fenryr_t;
struct hati_t;
struct stable_pet_t;
struct call_of_the_wild_pet_t;
struct beast_of_opportunity_pet_t;
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
  bool sentinel_imploding = false;
  bool crescent_steel_damaged = false;

  struct debuffs_t
  {
    buff_t* shredded_armor;
    buff_t* wild_instincts;
    buff_t* basilisk_collar;
    buff_t* outland_venom;
    buff_t* kill_zone;
    buff_t* sentinel;
    buff_t* crescent_steel;
    buff_t* lunar_storm;
  } debuffs;

  struct dots_t
  {
    dot_t* serpent_sting;
    dot_t* a_murder_of_crows;
    dot_t* wildfire_bomb;
    dot_t* black_arrow;
    dot_t* barbed_shot;
    dot_t* cull_the_herd;
    dot_t* explosive_shot;
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
    spawner::pet_spawner_t<pets::dire_critter_t, hunter_t> dire_beast;
    spawner::pet_spawner_t<pets::dark_hound_t, hunter_t> dark_hound;
    spawner::pet_spawner_t<pets::fenryr_t, hunter_t> fenryr;
    spawner::pet_spawner_t<pets::hati_t, hunter_t> hati;
    spawner::pet_spawner_t<pets::call_of_the_wild_pet_t, hunter_t> cotw_stable_pet;
    spawner::pet_spawner_t<pets::beast_of_opportunity_pet_t, hunter_t> boo_stable_pet;

    pets_t( hunter_t* p ) : 
      dire_beast( "dire_beast", p ),
      dark_hound( "dark_hound", p ),
      fenryr( "fenryr", p ),
      hati( "hati", p ),
      cotw_stable_pet( "call_of_the_wild_pet", p ), 
      boo_stable_pet( "beast_of_opportunity_pet", p )
    {
    }
  } pets;

  struct tier_sets_t
  {
    // TWW Season 1 - Nerub'ar Palace
    spell_data_ptr_t tww_s1_bm_2pc;
    spell_data_ptr_t tww_s1_bm_4pc;
    spell_data_ptr_t tww_s1_mm_2pc;
    spell_data_ptr_t tww_s1_mm_4pc;
    spell_data_ptr_t tww_s1_mm_4pc_buff;
    spell_data_ptr_t tww_s1_sv_2pc;
    spell_data_ptr_t tww_s1_sv_4pc;
  } tier_set;

  // Buffs
  struct buffs_t
  {
    // Hunter Tree
    buff_t* trigger_finger;

    // Marksmanship Tree
    buff_t* precise_shots;
    buff_t* lone_wolf;
    buff_t* streamline;
    buff_t* in_the_rhythm;
    buff_t* deathblow;
    buff_t* razor_fragments;
    buff_t* trick_shots;
    buff_t* volley;
    buff_t* steady_focus;
    buff_t* trueshot;
    buff_t* lock_and_load;
    buff_t* bullseye;
    buff_t* bulletstorm;
    buff_t* eagletalons_true_focus;
    buff_t* salvo;
    buff_t* unerring_vision_hidden;
    buff_t* unerring_vision;
    buff_t* wailing_arrow_counter;
    buff_t* wailing_arrow_override;

    // Beast Mastery Tree
    std::array<buff_t*, BARBED_SHOT_BUFFS_MAX> barbed_shot;
    buff_t* thrill_of_the_hunt;
    buff_t* dire_beast;
    buff_t* bestial_wrath;
    buff_t* hunters_prey;
    buff_t* call_of_the_wild;
    buff_t* beast_cleave; 
    buff_t* explosive_venom;
    buff_t* a_murder_of_crows;
    buff_t* huntmasters_call; 
    buff_t* summon_fenryr;
    buff_t* summon_hati;  

    // Survival Tree
    buff_t* tip_of_the_spear;
    buff_t* tip_of_the_spear_explosive;
    buff_t* sulfur_lined_pockets;
    buff_t* sulfur_lined_pockets_explosive;
    buff_t* merciless_blows;
    buff_t* bloodseeker;
    buff_t* aspect_of_the_eagle;
    buff_t* terms_of_engagement;
    buff_t* mongoose_fury;
    buff_t* coordinated_assault;
    buff_t* exposed_flank;
    buff_t* sic_em;
    buff_t* relentless_primal_ferocity;
    buff_t* bombardier;

    // Pet family buffs
    buff_t* endurance_training;
    buff_t* pathfinding;
    buff_t* predators_thirst;

    // Tier Set Bonuses
    //TWW - S1
    buff_t* harmonize; //BM 4pc
    buff_t* moving_target; //MM 4pc

    // Hero Talents 

    // Pack Leader
    buff_t* vicious_hunt;
    buff_t* howl_of_the_pack;
    buff_t* frenzied_tear; 
    buff_t* scattered_prey;
    buff_t* furious_assault;
    buff_t* beast_of_opportunity;

    // Sentinel
    buff_t* eyes_closed;

    // Dark Ranger
    buff_t* black_arrow;
    buff_t* withering_fire;

  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* kill_command;
    cooldown_t* kill_shot;
    cooldown_t* explosive_shot;

    cooldown_t* rapid_fire;
    cooldown_t* aimed_shot;
    cooldown_t* trueshot;
    cooldown_t* legacy_of_the_windrunners;

    cooldown_t* barbed_shot;
    cooldown_t* bestial_wrath;

    cooldown_t* wildfire_bomb;
    cooldown_t* butchery;
    cooldown_t* harpoon;
    cooldown_t* flanking_strike;
    cooldown_t* fury_of_the_eagle;
    cooldown_t* ruthless_marauder;
    cooldown_t* coordinated_assault;

    cooldown_t* black_arrow;
    cooldown_t* shadow_surge;
    cooldown_t* lunar_storm;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* trueshot;

    gain_t* barbed_shot;
    gain_t* dire_beast;

    gain_t* terms_of_engagement;

    gain_t* dark_empowerment;
    gain_t* invigorating_pulse;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* calling_the_shots;

    proc_t* wild_call;
    proc_t* wild_instincts;
    proc_t* dire_command;

    proc_t* sentinel_stacks;
    proc_t* sentinel_implosions;
    proc_t* extrapolated_shots_stacks;
    proc_t* release_and_reload_stacks;
    proc_t* crescent_steel_stacks;
    proc_t* overwatch_implosions;
  } procs;

  // Talents
  struct talents_t
  {
    // Hunter Tree
    spell_data_ptr_t kill_shot;

    spell_data_ptr_t improved_kill_shot;

    spell_data_ptr_t tar_trap;

    spell_data_ptr_t counter_shot;
    spell_data_ptr_t muzzle;

    spell_data_ptr_t lone_survivor;
    spell_data_ptr_t specialized_arsenal;
    spell_data_ptr_t disruptive_rounds; //NYI - When Counter Shot interrupts a cast, gain 10 focus. 

    spell_data_ptr_t explosive_shot;
    spell_data_ptr_t explosive_shot_cast;
    spell_data_ptr_t explosive_shot_damage;

    spell_data_ptr_t bursting_shot; 
    spell_data_ptr_t scatter_shot; // NYI
    spell_data_ptr_t trigger_finger;
    spell_data_ptr_t blackrock_munitions; 
    spell_data_ptr_t keen_eyesight;

    spell_data_ptr_t quick_load; //NYI - When you fall below 40% heath, Bursting Shot's cooldown is immediately reset. This can only occur once every 25 sec.

    spell_data_ptr_t serrated_tips;
    spell_data_ptr_t born_to_be_wild;
    spell_data_ptr_t improved_traps;

    spell_data_ptr_t high_explosive_trap;
    spell_data_ptr_t implosive_trap;
    spell_data_ptr_t explosive_trap_damage;

    spell_data_ptr_t unnatural_causes;
    spell_data_ptr_t unnatural_causes_debuff;
    
    // Shared
    // BM + SV
    spell_data_ptr_t alpha_predator; 
    spell_data_ptr_t kill_command;

    // MM + BM
    spell_data_ptr_t barrage;

    // MM + SV

    // Marksmanship Tree
    spell_data_ptr_t aimed_shot;

    spell_data_ptr_t rapid_fire;
    spell_data_ptr_t rapid_fire_tick;
    spell_data_ptr_t multishot_mm;
    spell_data_ptr_t precise_shots;
    spell_data_ptr_t precise_shots_buff;

    spell_data_ptr_t surging_shots;
    spell_data_ptr_t streamline;
    spell_data_ptr_t improved_steady_shot;
    spell_data_ptr_t crack_shot;

    spell_data_ptr_t penetrating_shots;
    spell_data_ptr_t trick_shots;
    spell_data_ptr_t trick_shots_data;
    spell_data_ptr_t master_marksman;
    spell_data_ptr_t master_marksman_bleed;

    spell_data_ptr_t fan_the_hammer;
    spell_data_ptr_t careful_aim;
    spell_data_ptr_t light_ammo;
    spell_data_ptr_t heavy_ammo;
    spell_data_ptr_t bulletstorm;
    spell_data_ptr_t lock_and_load;
    spell_data_ptr_t steady_focus;

    spell_data_ptr_t deathblow;
    spell_data_ptr_t night_hunter;
    spell_data_ptr_t tactical_reload;
    spell_data_ptr_t serpentstalkers_trickery;
    spell_data_ptr_t chimaera_shot;
    spell_data_ptr_t chimaera_shot_nature;
    spell_data_ptr_t chimaera_shot_frost;

    spell_data_ptr_t killer_accuracy;
    spell_data_ptr_t rapid_fire_barrage;
    spell_data_ptr_t rapid_fire_barrage_override;
    spell_data_ptr_t in_the_rhythm;
    spell_data_ptr_t lone_wolf;
    spell_data_ptr_t bullseye;
    spell_data_ptr_t hydras_bite;
    spell_data_ptr_t volley;
    spell_data_ptr_t volley_data;
    spell_data_ptr_t volley_dmg;

    spell_data_ptr_t legacy_of_the_windrunners;
    spell_data_ptr_t legacy_of_the_windrunners_wind_arrow;
    spell_data_ptr_t trueshot;
    spell_data_ptr_t focused_aim;

    spell_data_ptr_t razor_fragments;
    spell_data_ptr_t razor_fragments_bleed;
    spell_data_ptr_t razor_fragments_buff;
    spell_data_ptr_t wailing_arrow;
    spell_data_ptr_t wailing_arrow_counter_buff;
    spell_data_ptr_t wailing_arrow_override_buff;
    spell_data_ptr_t wailing_arrow_override;
    spell_data_ptr_t wailing_arrow_damage;

    spell_data_ptr_t eagletalons_true_focus;
    spell_data_ptr_t calling_the_shots;
    spell_data_ptr_t small_game_hunter;
    spell_data_ptr_t kill_zone;
    spell_data_ptr_t kill_zone_debuff;

    spell_data_ptr_t readiness;
    spell_data_ptr_t unerring_vision;
    spell_data_ptr_t salvo;

    // Beast Mastery Tree
    spell_data_ptr_t cobra_shot;
    spell_data_ptr_t animal_companion;
    spell_data_ptr_t barbed_shot;

    spell_data_ptr_t pack_tactics;
    spell_data_ptr_t aspect_of_the_beast;
    spell_data_ptr_t war_orders;
    spell_data_ptr_t thrill_of_the_hunt;

    spell_data_ptr_t go_for_the_throat;
    spell_data_ptr_t multishot_bm;
    spell_data_ptr_t laceration;

    spell_data_ptr_t cobra_senses;
    spell_data_ptr_t improved_kill_command;
    spell_data_ptr_t beast_cleave;
    spell_data_ptr_t wild_call;
    spell_data_ptr_t hunters_prey;
    spell_data_ptr_t venoms_bite;

    spell_data_ptr_t stomp;
    spell_data_ptr_t kindred_spirits;
    spell_data_ptr_t kill_cleave;
    spell_data_ptr_t training_expert;
    spell_data_ptr_t dire_beast;

    spell_data_ptr_t a_murder_of_crows;
    spell_data_ptr_t savagery;
    spell_data_ptr_t bestial_wrath;
    spell_data_ptr_t dire_command;
    spell_data_ptr_t huntmasters_call;
    spell_data_ptr_t dire_frenzy;

    spell_data_ptr_t killer_instinct;
    spell_data_ptr_t master_handler;
    spell_data_ptr_t barbed_wrath;
    spell_data_ptr_t explosive_venom;
    spell_data_ptr_t basilisk_collar;
    
    spell_data_ptr_t call_of_the_wild;
    spell_data_ptr_t killer_cobra;
    spell_data_ptr_t scent_of_blood;
    spell_data_ptr_t brutal_companion;
    spell_data_ptr_t bloodshed;

    spell_data_ptr_t wild_instincts;
    spell_data_ptr_t bloody_frenzy;
    spell_data_ptr_t piercing_fangs;
    spell_data_ptr_t venomous_bite;
    spell_data_ptr_t shower_of_blood;

    // Survival Tree
    spell_data_ptr_t wildfire_bomb;
    spell_data_ptr_t wildfire_bomb_data;
    spell_data_ptr_t wildfire_bomb_dmg;
    spell_data_ptr_t wildfire_bomb_dot;
    spell_data_ptr_t raptor_strike;
    spell_data_ptr_t raptor_strike_eagle;

    spell_data_ptr_t guerrilla_tactics;
    spell_data_ptr_t tip_of_the_spear;
    spell_data_ptr_t tip_of_the_spear_buff;
    spell_data_ptr_t tip_of_the_spear_explosive_buff;

    spell_data_ptr_t lunge;
    spell_data_ptr_t quick_shot;
    spell_data_ptr_t mongoose_bite;
    spell_data_ptr_t mongoose_bite_eagle;
    spell_data_ptr_t flankers_advantage;

    spell_data_ptr_t wildfire_infusion;
    spell_data_ptr_t improved_wildfire_bomb;
    spell_data_ptr_t sulfur_lined_pockets;
    spell_data_ptr_t butchery;
    spell_data_ptr_t bloody_claws;
    spell_data_ptr_t terms_of_engagement;
    spell_data_ptr_t terms_of_engagement_dmg;
    spell_data_ptr_t terms_of_engagement_buff;

    spell_data_ptr_t grenade_juggler;
    spell_data_ptr_t flanking_strike;
    spell_data_ptr_t flanking_strike_player;
    spell_data_ptr_t frenzy_strikes;
    spell_data_ptr_t merciless_blows;
    spell_data_ptr_t vipers_venom;
    spell_data_ptr_t bloodseeker;

    spell_data_ptr_t ranger;
    spell_data_ptr_t exposed_flank;
    spell_data_ptr_t tactical_advantage;
    spell_data_ptr_t sic_em;
    spell_data_ptr_t contagious_reagents;
    spell_data_ptr_t outland_venom;
    spell_data_ptr_t outland_venom_debuff;

    spell_data_ptr_t explosives_expert;
    spell_data_ptr_t sweeping_spear;
    spell_data_ptr_t killer_companion;

    spell_data_ptr_t fury_of_the_eagle;
    spell_data_ptr_t coordinated_assault;
    spell_data_ptr_t coordinated_assault_dmg;
    spell_data_ptr_t spearhead;
    spell_data_ptr_t spearhead_attack;

    spell_data_ptr_t ruthless_marauder;
    spell_data_ptr_t symbiotic_adrenaline;
    spell_data_ptr_t relentless_primal_ferocity;
    spell_data_ptr_t relentless_primal_ferocity_buff;
    spell_data_ptr_t bombardier;
    spell_data_ptr_t bombardier_buff;
    spell_data_ptr_t deadly_duo;

    // Dark Ranger
    spell_data_ptr_t black_arrow;
    spell_data_ptr_t black_arrow_buff;

    spell_data_ptr_t overshadow;
    spell_data_ptr_t shadow_hounds;
    spell_data_ptr_t death_shade;

    spell_data_ptr_t dark_empowerment;
    spell_data_ptr_t grave_reaper;
    spell_data_ptr_t embrace_the_shadows;  // TODO defensive
    spell_data_ptr_t smoke_screen;         // TODO defensive
    spell_data_ptr_t dark_chains;          // TODO defensive

    spell_data_ptr_t shadow_lash;
    spell_data_ptr_t shadow_surge;
    spell_data_ptr_t shadow_surge_dmg;
    spell_data_ptr_t darkness_calls;
    spell_data_ptr_t shadow_erasure;

    spell_data_ptr_t withering_fire;
    spell_data_ptr_t withering_fire_dmg;
    spell_data_ptr_t withering_fire_buff;


    // Pack Leader
    spell_data_ptr_t vicious_hunt;

    spell_data_ptr_t pack_coordination;
    spell_data_ptr_t howl_of_the_pack;
    spell_data_ptr_t wild_attacks;

    spell_data_ptr_t den_recovery;  // TODO defensive
    spell_data_ptr_t tireless_hunt; // TODO utility
    spell_data_ptr_t cornered_prey; // TODO utility
    spell_data_ptr_t frenzied_tear;

    spell_data_ptr_t scattered_prey;
    spell_data_ptr_t covering_fire;
    spell_data_ptr_t cull_the_herd;
    spell_data_ptr_t furious_assault;
    spell_data_ptr_t beast_of_opportunity;

    spell_data_ptr_t pack_assault;

    // Sentinel
    spell_data_ptr_t sentinel;
    spell_data_ptr_t sentinel_debuff;
    spell_data_ptr_t sentinel_tick;

    spell_data_ptr_t dont_look_back;
    spell_data_ptr_t extrapolated_shots;
    spell_data_ptr_t sentinel_precision;

    spell_data_ptr_t release_and_reload;
    spell_data_ptr_t catch_out;
    spell_data_ptr_t sideline;
    spell_data_ptr_t invigorating_pulse;

    spell_data_ptr_t sentinel_watch;
    spell_data_ptr_t eyes_closed;
    spell_data_ptr_t symphonic_arsenal;
    spell_data_ptr_t symphonic_arsenal_dmg;
    spell_data_ptr_t overwatch;
    spell_data_ptr_t crescent_steel;
    spell_data_ptr_t crescent_steel_debuff;

    spell_data_ptr_t lunar_storm;
    spell_data_ptr_t lunar_storm_ground;
    spell_data_ptr_t lunar_storm_dmg;
  } talents;

  // Specialization Spells
  struct specs_t
  {
    spell_data_ptr_t critical_strikes;
    spell_data_ptr_t hunter;
    spell_data_ptr_t beast_mastery_hunter;
    spell_data_ptr_t marksmanship_hunter;
    spell_data_ptr_t survival_hunter;

    spell_data_ptr_t auto_shot;
    spell_data_ptr_t freezing_trap;
    spell_data_ptr_t arcane_shot;
    spell_data_ptr_t steady_shot;
    spell_data_ptr_t flare;
    spell_data_ptr_t serpent_sting;
    spell_data_ptr_t harpoon;
  } specs;

  struct mastery_spells_t
  {
    spell_data_ptr_t master_of_beasts;
    spell_data_ptr_t sniper_training;
    spell_data_ptr_t spirit_bond;
    spell_data_ptr_t spirit_bond_buff;
  } mastery;

  struct {
    action_t* barbed_shot = nullptr;
    action_t* dire_command = nullptr;
    action_t* a_murder_of_crows = nullptr;

    action_t* vicious_hunt = nullptr;
    action_t* cull_the_herd = nullptr;

    action_t* sentinel = nullptr;
    action_t* symphonic_arsenal = nullptr;
    action_t* lunar_storm = nullptr;

    action_t* shadow_surge = nullptr;
    action_t* withering_fire = nullptr;
  } actions;

  cdwaste::player_data_t cd_waste;

  struct {
    events::tar_trap_aoe_t* tar_trap_aoe = nullptr;
    unsigned steady_focus_counter = 0;
    // Focus used for Calling the Shots (260404)
    double focus_used_CTS = 0;
    unsigned bombardment_counter = 0;
    unsigned windrunners_guidance_counter = 0;
    event_t* current_volley = nullptr;
    timespan_t sentinel_watch_reduction = 0_s;
  } state;

  struct options_t {
    std::string summon_pet_str = "duck";
    timespan_t pet_attack_speed = 2_s;
    timespan_t pet_basic_attack_delay = 0.15_s;
  } options;

  hunter_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) :
    player_t( sim, HUNTER, name, r ),
    pets( this ),
    buffs(),
    cooldowns(),
    gains(),
    procs()
  {
    cooldowns.kill_command          = get_cooldown( "kill_command" );
    cooldowns.kill_shot             = get_cooldown( "kill_shot" );
    cooldowns.explosive_shot        = get_cooldown( "explosive_shot" );

    cooldowns.aimed_shot            = get_cooldown( "aimed_shot" );
    cooldowns.rapid_fire            = get_cooldown( "rapid_fire" );
    cooldowns.trueshot              = get_cooldown( "trueshot" );
    cooldowns.legacy_of_the_windrunners = get_cooldown( "legacy_of_the_windrunners" );

    cooldowns.barbed_shot           = get_cooldown( "barbed_shot" );
    cooldowns.bestial_wrath         = get_cooldown( "bestial_wrath" );

    cooldowns.wildfire_bomb         = get_cooldown( "wildfire_bomb" );
    cooldowns.butchery              = get_cooldown( "butchery" );
    cooldowns.harpoon               = get_cooldown( "harpoon" );
    cooldowns.flanking_strike       = get_cooldown( "flanking_strike");
    cooldowns.fury_of_the_eagle     = get_cooldown( "fury_of_the_eagle" );
    cooldowns.ruthless_marauder     = get_cooldown( "ruthless_marauder" );
    cooldowns.coordinated_assault   = get_cooldown( "coordinated_assault" );

    cooldowns.black_arrow = get_cooldown( "black_arrow" );
    cooldowns.shadow_surge = get_cooldown( "shadow_surge" );
    cooldowns.lunar_storm = get_cooldown( "lunar_storm" );

    base_gcd = 1.5_s;

    resource_regeneration = regen_type::DYNAMIC;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }

  // Character Definition
  void      init() override;
  void      init_spells() override;
  void      init_base_stats() override;
  void      create_actions() override;
  void      create_buffs() override;
  void      init_gains() override;
  void      init_position() override;
  void      init_procs() override;
  void      init_rng() override;
  void      init_scaling() override;
  void      init_assessors() override;
  void      init_action_list() override;
  void      init_special_effects() override;
  void      reset() override;
  void      merge( player_t& other ) override;
  void      arise() override;
  void      combat_begin() override;

  void datacollection_begin() override;
  void datacollection_end() override;

  double    composite_melee_crit_chance() const override;
  double    composite_spell_crit_chance() const override;
  double    composite_rating_multiplier( rating_e ) const override;
  double    composite_melee_auto_attack_speed() const override;
  double    composite_player_target_crit_chance( player_t* target ) const override;
  double    composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  double    composite_player_multiplier( school_e school ) const override;
  double    composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double    composite_player_pet_damage_multiplier( const action_state_t*, bool ) const override;
  double    composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const override;
  double    composite_leech() const override;
  double    matching_gear_multiplier( attribute_e attr ) const override;
  double    stacking_movement_modifier() const override;
  void      invalidate_cache( cache_e ) override;
  void      regen( timespan_t periodicity ) override;
  double    resource_gain( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;
  void      create_options() override;
  std::unique_ptr<expr_t> create_expression( util::string_view expression_str ) override;
  std::unique_ptr<expr_t> create_action_expression( action_t&, util::string_view expression_str ) override;
  action_t* create_action( util::string_view name, util::string_view options ) override;
  pet_t*    create_pet( util::string_view name, util::string_view type ) override;
  void      create_pets() override;
  double resource_loss( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;
  resource_e primary_resource() const override { return RESOURCE_FOCUS; }
  role_e    primary_role() const override { return ROLE_ATTACK; }
  stat_e    convert_hybrid_stat( stat_e s ) const override;
  std::string      create_profile( save_e ) override;
  void      copy_from( player_t* source ) override;
  void      moving( ) override;

  std::string default_potion() const override { return hunter_apl::potion( this ); }
  std::string default_flask() const override { return hunter_apl::flask( this ); }
  std::string default_food() const override { return hunter_apl::food( this ); }
  std::string default_rune() const override { return hunter_apl::rune( this ); }
  std::string default_temporary_enchant() const override { return hunter_apl::temporary_enchant( this ); }

  void apply_affecting_auras( action_t& ) override;

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
  void trigger_basilisk_collar_update();
  void trigger_outland_venom_update();
  void trigger_calling_the_shots( action_t* action, double cost );
  void consume_trick_shots();
  void trigger_sentinel( player_t* target, bool force = false, proc_t* proc = nullptr );
  void trigger_sentinel_implosion( hunter_td_t* td );
  void trigger_symphonic_arsenal();
  void trigger_lunar_storm( player_t* target );
};

// Template for common hunter action code.
template <class Base>
struct hunter_action_t: public Base
{
private:
  using ab = Base;
public:

  bool track_cd_waste;
  maybe_bool triggers_calling_the_shots;
  maybe_bool decrements_tip_of_the_spear;

  struct {
    bool cull_the_herd = false;
    damage_affected_by unnatural_causes;

    // bm
    bool thrill_of_the_hunt = false;
    damage_affected_by bestial_wrath;
    damage_affected_by master_of_beasts;

    // mm
    bool bullseye_crit_chance = false;
    damage_affected_by lone_wolf;
    bool precise_shots = false;
    bool precise_shots_cost = false;
    damage_affected_by sniper_training;

    // surv
    damage_affected_by spirit_bond;
    damage_affected_by coordinated_assault;
    damage_affected_by tip_of_the_spear;
    damage_affected_by tip_of_the_spear_explosive;
    bool spearhead_crit_chance = false;
    bool spearhead_crit_damage = false;

    // Tier Set
    damage_affected_by tww_s1_mm_4pc;
    damage_affected_by withering_fire;
  } affected_by;

  cdwaste::action_data_t* cd_waste = nullptr;

  hunter_action_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
    ab( n, p, s ),
    track_cd_waste( s -> cooldown() > 0_ms || s -> charge_cooldown() > 0_ms )
  {
    ab::special = true;

    for ( size_t i = 1; i <= ab::data().effect_count(); i++ )
    {
      if ( ab::data().mechanic() == MECHANIC_BLEED || ab::data().effectN( i ).mechanic() == MECHANIC_BLEED )
        affected_by.cull_the_herd = true;
    }

    if ( p->talents.unnatural_causes.ok() )
      affected_by.unnatural_causes      = parse_damage_affecting_aura( this, p->talents.unnatural_causes_debuff );

    affected_by.bullseye_crit_chance  = check_affected_by( this, p -> talents.bullseye -> effectN( 1 ).trigger() -> effectN( 1 ) );
    affected_by.lone_wolf             = parse_damage_affecting_aura( this, p -> talents.lone_wolf );
    affected_by.sniper_training       = parse_damage_affecting_aura( this, p -> mastery.sniper_training );

    affected_by.thrill_of_the_hunt    = check_affected_by( this, p -> talents.thrill_of_the_hunt -> effectN( 1 ).trigger() -> effectN( 1 ) );
    affected_by.bestial_wrath         = parse_damage_affecting_aura( this, p -> talents.bestial_wrath );
    affected_by.master_of_beasts      = parse_damage_affecting_aura( this, p -> mastery.master_of_beasts );

    affected_by.spirit_bond           = parse_damage_affecting_aura( this, p -> mastery.spirit_bond );
    affected_by.coordinated_assault   = parse_damage_affecting_aura( this, p->talents.coordinated_assault );
    affected_by.tip_of_the_spear      = parse_damage_affecting_aura( this, p->talents.tip_of_the_spear_buff );
    affected_by.tip_of_the_spear_explosive = parse_damage_affecting_aura( this, p->talents.tip_of_the_spear_explosive_buff );
    affected_by.spearhead_crit_chance = check_affected_by( this, p->talents.spearhead_attack->effectN( 2 ) );
    affected_by.spearhead_crit_damage = check_affected_by( this, p->talents.spearhead_attack->effectN( 3 ) );

    affected_by.tww_s1_mm_4pc = parse_damage_affecting_aura( this, p -> tier_set.tww_s1_mm_4pc_buff );
    affected_by.withering_fire = parse_damage_affecting_aura( this, p->talents.withering_fire_buff );

    // Hunter Tree passives
    ab::apply_affecting_aura( p -> talents.improved_kill_shot );
    ab::apply_affecting_aura( p -> talents.specialized_arsenal );
    ab::apply_affecting_aura( p -> talents.improved_traps );
    ab::apply_affecting_aura( p -> talents.born_to_be_wild );
    ab::apply_affecting_aura( p -> talents.hydras_bite );
    ab::apply_affecting_aura( p -> talents.blackrock_munitions );
    ab::apply_affecting_aura( p -> talents.lone_survivor );

    // Marksmanship Tree passives
    ab::apply_affecting_aura( p -> talents.crack_shot );
    ab::apply_affecting_aura( p -> talents.streamline );
    ab::apply_affecting_aura( p -> talents.killer_accuracy );
    ab::apply_affecting_aura( p -> talents.surging_shots );
    ab::apply_affecting_aura( p -> talents.focused_aim );
    ab::apply_affecting_aura( p -> talents.tactical_reload );
    ab::apply_affecting_aura( p -> talents.night_hunter );
    ab::apply_affecting_aura( p -> talents.small_game_hunter );
    ab::apply_affecting_aura( p -> talents.fan_the_hammer );
    ab::apply_affecting_aura( p -> talents.rapid_fire_barrage );
    ab::apply_affecting_aura( p -> talents.eagletalons_true_focus );

    // Beast Mastery Tree passives
    ab::apply_affecting_aura( p -> talents.war_orders );
    ab::apply_affecting_aura( p -> talents.savagery );

    // Survival Tree passives
    ab::apply_affecting_aura( p -> talents.terms_of_engagement );
    ab::apply_affecting_aura( p -> talents.guerrilla_tactics );
    ab::apply_affecting_aura( p -> talents.improved_wildfire_bomb );
    ab::apply_affecting_aura( p -> talents.sweeping_spear );
    ab::apply_affecting_aura( p -> talents.tactical_advantage );
    ab::apply_affecting_aura( p -> talents.ranger );
    ab::apply_affecting_aura( p -> talents.explosives_expert );
    ab::apply_affecting_aura( p -> talents.symbiotic_adrenaline );
    ab::apply_affecting_aura( p -> talents.grenade_juggler );
    ab::apply_affecting_aura( p -> talents.deadly_duo );

    // Set Bonus passives
    ab::apply_affecting_aura( p -> tier_set.tww_s1_mm_2pc );
    ab::apply_affecting_aura( p -> tier_set.tww_s1_sv_2pc );

    // Hero Tree passives
    ab::apply_affecting_aura( p->talents.overshadow );
    ab::apply_affecting_aura( p->talents.sentinel_precision );
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

    if ( p() -> talents.calling_the_shots.ok() )
    {
      if ( triggers_calling_the_shots.is_none() )
        triggers_calling_the_shots = !ab::background && !ab::proc && ab::base_cost() > 0;
    }
    else
    {
      triggers_calling_the_shots = false;
    }

    if ( triggers_calling_the_shots )
      ab::sim -> print_debug( "{} action {} set to proc Calling the Shots", ab::player -> name(), ab::name() );

    if ( p()->talents.tip_of_the_spear.ok() )
    {
      if ( decrements_tip_of_the_spear.is_none() )
        decrements_tip_of_the_spear = affected_by.tip_of_the_spear.direct > 0;
    }
    else
    {
      decrements_tip_of_the_spear = false;
    }

    if ( decrements_tip_of_the_spear )
      ab::sim->print_debug( "{} action {} set to decrement Tip of the Spear", ab::player->name(), ab::name() );
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

    if ( triggers_calling_the_shots )
      p() -> trigger_calling_the_shots( this, this -> cost() );

    if ( decrements_tip_of_the_spear )
      p()->buffs.tip_of_the_spear->decrement();

    if ( affected_by.tip_of_the_spear_explosive.direct )
      p()->buffs.tip_of_the_spear_explosive->decrement();
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double am = ab::composite_da_multiplier( s );

    if ( affected_by.lone_wolf.direct )
      am *= 1 + p() -> buffs.lone_wolf -> check_value();

    if ( affected_by.bestial_wrath.direct )
      am *= 1 + p() -> buffs.bestial_wrath -> check_value();

    if ( affected_by.master_of_beasts.direct )
      am *= 1 + p() -> cache.mastery() * p() -> mastery.master_of_beasts -> effectN( affected_by.master_of_beasts.direct ).mastery_value();

    if ( affected_by.sniper_training.direct )
      am *= 1 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( affected_by.sniper_training.direct ).mastery_value();

    if ( affected_by.spirit_bond.direct )
    {
      double bonus = p() -> cache.mastery() * p() -> mastery.spirit_bond -> effectN( affected_by.spirit_bond.direct ).mastery_value();
      // TODO implement range
      bonus *= 1 + p()->mastery.spirit_bond_buff->effectN( 1 ).percent();
      
      am *= 1 + bonus;
    }

    if ( affected_by.coordinated_assault.direct && p()->buffs.coordinated_assault->check() )
      am *= 1 + p()->talents.coordinated_assault->effectN( affected_by.coordinated_assault.direct ).percent();

    if ( affected_by.unnatural_causes.direct )
    {
      double amount = p()->talents.unnatural_causes_debuff->effectN( affected_by.unnatural_causes.direct ).percent();
      if ( s->target->health_percentage() < p()->talents.unnatural_causes->effectN( 3 ).base_value() )
        amount *= 1 + p()->talents.unnatural_causes->effectN( 2 ).percent();

      am *= 1 + amount;
    }

    int affected_by_tip = std::max( affected_by.tip_of_the_spear.direct, affected_by.tip_of_the_spear_explosive.direct );
    if ( affected_by_tip && p()->buffs.tip_of_the_spear->check() )
    {
      double tip_bonus = p()->talents.tip_of_the_spear->effectN( affected_by_tip ).percent();

      if ( p()->talents.flankers_advantage.ok() )
      {
        double max_bonus = p()->talents.flankers_advantage->effectN( 6 ).percent() - tip_bonus;

        // Seems that the amount of the 15% bonus given is based on the ratio of player crit % out of a cap of 50% from effect 5.
        double crit_chance =
            std::min( p()->cache.attack_crit_chance(), p()->talents.flankers_advantage->effectN( 5 ).percent() );

        tip_bonus += max_bonus * crit_chance / p()->talents.flankers_advantage->effectN( 5 ).percent();
      }

      if ( p()->buffs.relentless_primal_ferocity->check() )
        tip_bonus += p()->talents.relentless_primal_ferocity_buff->effectN( 2 ).percent();

      am *= 1 + tip_bonus;
    }

    if ( affected_by.tww_s1_mm_4pc.direct && p()->buffs.moving_target->check() )
    {
      am *= 1 + p()->buffs.moving_target->value();
    }

    if ( affected_by.withering_fire.direct )
      am *= 1 + p()->buffs.withering_fire->check_value();

    return am;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double am = ab::composite_ta_multiplier( s );

    if ( affected_by.lone_wolf.tick )
      am *= 1 + p() -> buffs.lone_wolf -> check_value();

    if ( affected_by.bestial_wrath.tick )
      am *= 1 + p() -> buffs.bestial_wrath -> check_value();

    if ( affected_by.sniper_training.tick )
      am *= 1 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( affected_by.sniper_training.tick ).mastery_value();

    if ( affected_by.spirit_bond.tick )
      {
      double bonus = p() -> cache.mastery() * p() -> mastery.spirit_bond -> effectN( affected_by.spirit_bond.tick ).mastery_value();
      // TODO implement range
      bonus *= 1 + p()->mastery.spirit_bond_buff->effectN( 3 ).percent();
      
      am *= 1 + bonus;
    }

    if ( affected_by.unnatural_causes.tick )
    {
      double amount = p()->talents.unnatural_causes_debuff->effectN( affected_by.unnatural_causes.tick ).percent();
      if ( s->target->health_percentage() < p()->talents.unnatural_causes->effectN( 3 ).base_value() )
        amount *= 1 + p()->talents.unnatural_causes->effectN( 2 ).percent();

      am *= 1 + amount;
    }

    if ( affected_by.coordinated_assault.tick && p()->buffs.coordinated_assault->check() )
      am *= 1 + p()->talents.coordinated_assault->effectN( affected_by.coordinated_assault.tick ).percent();

    return am;
  }

  double composite_crit_chance() const override
  {
    double cc = ab::composite_crit_chance();

    if ( affected_by.thrill_of_the_hunt )
      cc += p() -> buffs.thrill_of_the_hunt -> check_stack_value();

    if ( affected_by.bullseye_crit_chance )
      cc += p() -> buffs.bullseye -> check_stack_value();

    return cc;
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = ab::composite_target_crit_chance( target );

    if ( affected_by.spearhead_crit_chance && p()->pets.main && p()->pets.main->get_target_data( target )->dots.spearhead->is_ticking() )
      c += p()->talents.spearhead_attack->effectN( 2 ).percent();

    return c;
  }

  double composite_target_crit_damage_bonus_multiplier( player_t* target ) const override
  {
    double cm = ab::composite_target_crit_damage_bonus_multiplier( target );

    if ( affected_by.spearhead_crit_damage && p()->talents.deadly_duo.ok() && p()->pets.main && p()->pets.main->get_target_data( target )->dots.spearhead->is_ticking() )
    {
      cm *= 1.0 + p()->talents.deadly_duo->effectN( 2 ).percent();
    }

    return cm;
  }

  double composite_target_ta_multiplier( player_t* target ) const override
  {
    double ta = ab::composite_target_ta_multiplier( target );

    if ( affected_by.cull_the_herd && p() -> get_target_data( target ) -> dots.cull_the_herd -> is_ticking() )
    {
      ta *= 1 + p() -> talents.cull_the_herd -> effectN( 3 ).percent();
    }

    return ta;
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

    if ( p() -> buffs.trueshot -> check() )
    {
      const timespan_t remains = p() -> buffs.trueshot -> remains();

      total_regen += regen * std::min( cast_time, remains ).total_seconds() *
                     p() -> talents.trueshot -> effectN( 6 ).percent();

      if ( execute_time < remains )
        total_energize *= 1 + p() -> talents.trueshot -> effectN( 5 ).percent();
    }

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
      buff -> extend_duration( ab::player, -std::min( precast_time, buff -> buff_duration() ) );
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
};

struct hunter_ranged_attack_t : public hunter_action_t<ranged_attack_t>
{
  bool breaks_steady_focus = true;

  hunter_ranged_attack_t( util::string_view n, hunter_t* p, const spell_data_t* s = spell_data_t::nil() ) : hunter_action_t( n, p, s )
  {
    affected_by.precise_shots = p -> talents.precise_shots.ok() && parse_damage_affecting_aura( this, p -> talents.precise_shots_buff ).direct;
    affected_by.precise_shots_cost = check_affected_by( this, p->talents.precise_shots_buff->effectN( 6 ) ); 
  }

  bool usable_moving() const override
  { return true; }

  void execute() override
  {
    hunter_action_t::execute();

    if ( !background && breaks_steady_focus )
      p() -> state.steady_focus_counter = 0;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double am = hunter_action_t::composite_da_multiplier( s );

    if ( affected_by.precise_shots )
      am *= 1 + p() -> buffs.precise_shots -> check_value();

    return am;
  }

  double cost_pct_multiplier() const override
  {
    double c = hunter_action_t::cost_pct_multiplier();

    if ( affected_by.precise_shots_cost && p()->buffs.precise_shots->check() )
      c *= 1 + p()->talents.precise_shots_buff->effectN( 6 ).percent();

    return c;
  }
};

struct hunter_melee_attack_t : public hunter_action_t<melee_attack_t>
{
  hunter_melee_attack_t( util::string_view n, hunter_t* p, const spell_data_t* s = spell_data_t::nil() ) : hunter_action_t( n, p, s )
  {
  }

  void init() override
  {
    hunter_action_t::init();

    if ( weapon && weapon -> group() != WEAPON_2H )
      background = true;
  }
};

using hunter_spell_t = hunter_ranged_attack_t;

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

  struct actives_t
  {
    action_t* beast_cleave = nullptr;
    action_t* laceration = nullptr; 
  } active;

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

  double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( o()->talents.darkness_calls->effectN( 1 ).has_common_school( school ) )
      m *= 1 + o()->talents.darkness_calls->effectN( 1 ).percent();

    return m;
  }

  double composite_player_critical_damage_multiplier( const action_state_t* s ) const override
  {
    double m = pet_t::composite_player_critical_damage_multiplier( s );

    if( o() -> buffs.howl_of_the_pack -> check() )
      m *= 1 + o() -> buffs.howl_of_the_pack -> check_value();

    return m;
  }

  double composite_player_target_multiplier( player_t* target, school_e school ) const override
  {
    double m = pet_t::composite_player_target_multiplier( target, school );

    auto td = o()->get_target_data( target );
    bool guardian = type == PLAYER_GUARDIAN;

    if ( o()->talents.basilisk_collar->ok() )
    {   
      double bonus = guardian ? o()->talents.basilisk_collar->effectN( 2 ).percent() : o()->talents.basilisk_collar->effectN( 1 ).percent();
      int stacks = td->debuffs.basilisk_collar->stack();
      m *= 1 + ( bonus * stacks );
    }

    // TODO should these go in composite_player_target_pet_damage_multiplier (non-hunter pets)

    if ( td->debuffs.kill_zone->check() )
      m *= 1 + o()->talents.kill_zone_debuff->effectN( guardian ? 4 : 3 ).percent();

    if ( td->debuffs.lunar_storm->check() )
      m *= 1 + o()->talents.lunar_storm_dmg->effectN( 2 ).trigger()->effectN( guardian ? 3 : 2 ).percent();
    
    return m;
  }

  void apply_affecting_auras( action_t& action ) override
  {
    pet_t::apply_affecting_auras(action);

    action.apply_affecting_aura( o() -> specs.hunter );
    action.apply_affecting_aura( o() -> specs.beast_mastery_hunter );
    action.apply_affecting_aura( o() -> specs.marksmanship_hunter );
    action.apply_affecting_aura( o() -> specs.survival_hunter );
  }

  void create_buffs() override
  {
    pet_t::create_buffs();

    buffs.beast_cleave =
      make_buff( this, "beast_cleave", find_spell( 118455 ) )
      -> set_default_value( o() -> talents.beast_cleave -> effectN( 1 ).percent() )
      -> apply_affecting_effect( o() -> talents.beast_cleave -> effectN( 2 ) );
  }

  hunter_t* o()             { return static_cast<hunter_t*>( owner ); }
  const hunter_t* o() const { return static_cast<hunter_t*>( owner ); }

  void init_spells() override;
};

static std::pair<timespan_t, int> dire_beast_duration( hunter_t* p )
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
  const timespan_t base_duration    = p->buffs.dire_beast->buff_duration();
  const timespan_t swing_time       = 2_s * p->cache.auto_attack_speed();
  double partial_attacks_per_summon = base_duration / swing_time;
  int base_attacks_per_summon       = static_cast<int>( partial_attacks_per_summon );
  partial_attacks_per_summon -= static_cast<double>( base_attacks_per_summon );

  if ( p->rng().roll( partial_attacks_per_summon ) )
    base_attacks_per_summon += 1;

  return { base_attacks_per_summon * swing_time, base_attacks_per_summon };
}

// ==========================================================================
// Dire Critter
// ==========================================================================

struct dire_critter_t : public hunter_pet_t
{
  struct actives_t
  {
    action_t* kill_command = nullptr;
    action_t* kill_cleave  = nullptr;
  } active;

  const spell_data_t* energize = find_spell( 281036 );

  dire_critter_t( hunter_t* owner, util::string_view n = "dire_beast" )
    : hunter_pet_t( owner, n, PET_HUNTER, true /* GUARDIAN */, true /* dynamic */ )
  {
    // 11-10-22 Dire Beast - Damage increased by 400%. (15% -> 60%)
    // 13-10-22 Dire Beast damage increased by 50%. (60% -> 90%)
    // 22-7-24 Dire Beast damage increased by 10% (90% -> 100%)
    owner_coeff.ap_from_ap = 1;

    resource_regeneration = regen_type::DISABLED;
  }

  void init_spells() override;

  void summon( timespan_t duration = 0_ms ) override
  {
    hunter_pet_t::summon( duration );

    o()->buffs.dire_beast->trigger( duration );
    o()->resource_gain( RESOURCE_FOCUS, energize->effectN( 2 ).base_value(), o()->gains.dire_beast );

    if ( main_hand_attack )
      main_hand_attack->execute();

    if ( o()->talents.huntmasters_call.ok() )
    {
      o()->buffs.huntmasters_call->trigger();
      if ( o()->buffs.huntmasters_call->at_max_stacks() )
      {
        if ( rng().roll( 0.5 ) )
        {
          o()->buffs.summon_fenryr->trigger();
          o()->pets.fenryr.spawn( o()->buffs.summon_fenryr->buff_duration() );
        }
        else
        {
          o()->buffs.summon_hati->trigger();
          o()->pets.hati.spawn( o()->buffs.summon_hati->buff_duration() );
        }
        o()->buffs.huntmasters_call->expire();
      }
    }
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = hunter_pet_t::composite_player_multiplier( school );

    m *= 1 + o()->talents.dire_frenzy->effectN( 2 ).percent();

    return m;
  }
};

// ==========================================================================
// Shadow Hounds
// ==========================================================================

struct dark_hound_t final : public hunter_pet_t
{
  dark_hound_t( hunter_t* owner, util::string_view n = "dark_hound" )
    : hunter_pet_t( owner, n, PET_HUNTER, true /* GUARDIAN */, true /* dynamic */ )
  {
    resource_regeneration  = regen_type::DISABLED;
    owner_coeff.ap_from_ap = 3;
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    hunter_pet_t::summon( duration );

    if ( main_hand_attack )
      main_hand_attack->execute();
  }

  void init_spells() override;
};

// =========================================================================
// Fenryr
// =========================================================================

struct fenryr_t final : public dire_critter_t
{
  struct actives_t
  {
    action_t* ravenous_leap = nullptr;
  } active;

  fenryr_t( hunter_t* owner, util::string_view n = "fenryr" ) : dire_critter_t( owner, n )
  {
    owner_coeff.ap_from_ap = 0.4;
    auto_attack_multiplier = 4.85;
  }

  void init_spells() override;

  void summon( timespan_t duration = 0_ms ) override
  {
    // Don't run the base summon function as we don't want to trigger the dire beast buff or energize
    hunter_pet_t::summon( duration );

    if ( main_hand_attack )
      main_hand_attack->execute();

    active.ravenous_leap->execute_on_target( target );
  }
};

// ==========================================================================
// Hati
// ==========================================================================

struct hati_t final : public dire_critter_t
{
  hati_t( hunter_t* owner, util::string_view n = "hati" ) : dire_critter_t( owner, n )
  {
    owner_coeff.ap_from_ap = 0.4;
    auto_attack_multiplier = 4.85;
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    // Don't run the base summon function as we don't want to trigger the dire beast buff or energize
    hunter_pet_t::summon( duration );

    if ( main_hand_attack )
      main_hand_attack->execute();
  }
};

// Base class for pets from player stable
struct stable_pet_t : public hunter_pet_t
{
  struct actives_t
  {
    action_t* stomp = nullptr; 
  } active;

  stable_pet_t( hunter_t* owner, util::string_view pet_name, pet_e pet_type ):
    hunter_pet_t( owner, pet_name, pet_type, false /* GUARDIAN */, true /* dynamic */ )
  {
    stamina_per_owner = 0.7;
    owner_coeff.ap_from_ap = 0.6;

    initial.armor_multiplier *= 1.05;

    main_hand_weapon.swing_time = owner -> options.pet_attack_speed;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = hunter_pet_t::composite_player_multiplier( school );

    m *= 1 + o() -> talents.animal_companion -> effectN( 2 ).percent();
    m *= 1 + o() -> talents.training_expert -> effectN( 1 ).percent();
    m *= 1 + o()->buffs.harmonize->check_value();

    return m;
  }

  void init_spells() override;
};

// ==========================================================================
// Call of the Wild
// ==========================================================================

struct call_of_the_wild_pet_t final : public stable_pet_t
{
  call_of_the_wild_pet_t( hunter_t* owner ) : stable_pet_t( owner, "call_of_the_wild_pet", PET_HUNTER )
  {
    resource_regeneration = regen_type::DISABLED;
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    stable_pet_t::summon( duration );

    if ( main_hand_attack )
      main_hand_attack->execute();
  }
};

// ==========================================================================
// Beast of Opportunity
// ==========================================================================

struct beast_of_opportunity_pet_t final : public stable_pet_t
{
  beast_of_opportunity_pet_t( hunter_t* owner ) : stable_pet_t( owner, "beast_of_opportunity_pet", PET_HUNTER )
  {
    resource_regeneration = regen_type::DISABLED;
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    stable_pet_t::summon( duration );

    if ( main_hand_attack )
      main_hand_attack->execute();
  }
};

// Base class for main pet & Animal Companion
struct hunter_main_pet_base_t : public stable_pet_t
{
  struct actives_t
  {
    action_t* basic_attack = nullptr;
    action_t* brutal_companion_ba = nullptr;
    action_t* pack_coordination_ba = nullptr;
    action_t* kill_command = nullptr;
    action_t* kill_cleave = nullptr;
    action_t* bestial_wrath = nullptr;
    action_t* stomp = nullptr;
    action_t* bloodshed = nullptr;

    action_t* flanking_strike = nullptr;
    action_t* coordinated_assault = nullptr;
    action_t* spearhead = nullptr;

    action_t* frenzied_tear = nullptr;
  } active;

  struct buffs_t
  {
    buff_t* frenzy = nullptr;
    buff_t* thrill_of_the_hunt = nullptr;
    buff_t* bestial_wrath = nullptr;
    buff_t* piercing_fangs = nullptr;

    buff_t* bloodseeker = nullptr;

    buff_t* pack_coordination = nullptr;
    buff_t* wild_attacks = nullptr;
    buff_t* wild_attacks_dmg_amp = nullptr;
  } buffs;

  struct {
    spell_data_ptr_t bloodshed;
  } spells;

  hunter_main_pet_base_t( hunter_t* owner, util::string_view pet_name, pet_e pet_type ):
    stable_pet_t( owner, pet_name, pet_type )
  {
  }

  void create_buffs() override
  {
    stable_pet_t::create_buffs();

    buffs.frenzy =
      make_buff( this, "frenzy", o() -> find_spell( 272790 ) )
      -> set_default_value_from_effect( 1 )
      -> modify_default_value( o() -> tier_set.tww_s1_bm_2pc -> effectN( 1 ).percent() )
      -> apply_affecting_aura( o() -> talents.savagery )
      -> add_invalidate( CACHE_AUTO_ATTACK_SPEED );

    buffs.thrill_of_the_hunt =
      make_buff( this, "thrill_of_the_hunt", find_spell( 312365 ) )
        -> set_default_value_from_effect( 1 )
        -> apply_affecting_aura( o() -> talents.savagery )
        -> set_max_stack( std::max( 1, as<int>( o() -> talents.thrill_of_the_hunt -> effectN( 2 ).base_value() ) ) )
        -> set_chance( o() -> talents.thrill_of_the_hunt.ok() );

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
    double ah = stable_pet_t::composite_melee_auto_attack_speed();

    if ( buffs.bloodseeker && buffs.bloodseeker -> check() )
      ah /= 1 + buffs.bloodseeker -> check_stack_value();

    if ( buffs.frenzy -> check() )
      ah /= 1 + buffs.frenzy -> check_stack_value();

    return ah;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = stable_pet_t::composite_player_multiplier( school );

    if ( buffs.bestial_wrath -> has_common_school( school ) )
      m *= 1 + buffs.bestial_wrath -> check_value();
    
    return m;
  }

  double composite_player_target_multiplier( player_t* target, school_e school ) const override;

  double composite_melee_crit_chance() const override
  {
    double cc = stable_pet_t::composite_melee_crit_chance();

    cc += buffs.thrill_of_the_hunt -> check_stack_value();

    return cc;
  }

  double composite_player_critical_damage_multiplier( const action_state_t* s ) const override
  {
    double m = stable_pet_t::composite_player_critical_damage_multiplier( s );

    if ( buffs.piercing_fangs -> data().effectN( 1 ).has_common_school( s -> action -> school ) )
      m *= 1 + buffs.piercing_fangs -> check_value();

    return m;
  }

  void init_spells() override;
  void init_special_effects() override;

  void moving() override { return; }
};

// ==========================================================================
// Animal Companion
// ==========================================================================

struct animal_companion_t final : public hunter_main_pet_base_t
{
  animal_companion_t( hunter_t* owner ):
    hunter_main_pet_base_t( owner, "animal_companion", PET_HUNTER )
  {
    resource_regeneration = regen_type::DISABLED;
  }
};

// ==========================================================================
// Hunter Main Pet
// ==========================================================================

struct hunter_main_pet_td_t: public actor_target_data_t
{
public:
  struct dots_t
  {
    dot_t* bloodshed = nullptr;
    dot_t* bloodseeker = nullptr;
    dot_t* laceration = nullptr;
    dot_t* ravenous_leap = nullptr;
    dot_t* spearhead     = nullptr;
  } dots;

  struct debuffs_t
  {
    buff_t* venomous_bite = nullptr;
  } debuffs; 

  hunter_main_pet_td_t( player_t* target, hunter_main_pet_t* p );
};

struct hunter_main_pet_t final : public hunter_main_pet_base_t
{
  hunter_main_pet_t( hunter_t* owner, util::string_view pet_name, pet_e pt ):
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
    resources.base[RESOURCE_FOCUS] = 100
      + o() -> talents.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS );

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
    
    buffs.pack_coordination
      = make_buff( this, "pack_coordination", o() -> find_spell( 445695 ) )
        -> set_initial_stack( 1 + as<int>( o() -> talents.pack_assault -> effectN( 2 ).base_value() ) )
        -> set_default_value_from_effect( 1 );

    buffs.wild_attacks = 
      make_buff( this, "wild_attacks", o() -> talents.wild_attacks )
        -> set_max_stack( 3 ); 

    buffs.wild_attacks_dmg_amp = 
      make_buff( this, "wild_attacks_dmg_amp", o() -> talents.wild_attacks )
        -> set_default_value_from_effect( 1 );
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
    if ( o() -> pets.animal_companion )
    {
      o() -> pets.animal_companion -> summon();
      o() -> pets.animal_companion -> schedule_ready(0_s, false);
    }

    spec_passive() -> trigger();
    o() -> buffs.lone_wolf -> expire();
    o()->buffs.trigger_finger->trigger( 1, o()->talents.trigger_finger->effectN( 1 ).percent() );
  }

  void demise() override
  {
    hunter_main_pet_base_t::demise();

    if ( o() -> pets.main == this )
    {
      o() -> pets.main = nullptr;

      spec_passive() -> expire();
      if ( !sim->event_mgr.canceled )
      {
        o()->buffs.lone_wolf->trigger();
        o()->buffs.trigger_finger->trigger();
      }
    }
    if ( o() -> pets.animal_companion )
      o() -> pets.animal_companion -> demise();
  }

  double composite_melee_auto_attack_speed() const override
  {
    double ah = hunter_main_pet_base_t::composite_melee_auto_attack_speed();

    if ( o()->talents.trigger_finger.ok() )
      ah /= 1 + o()->talents.trigger_finger->effectN( 4 ).percent();

    return ah;
  }

  target_specific_t<hunter_main_pet_td_t> target_data;

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
    if ( ! active.basic_attack )
      return hunter_main_pet_base_t::available();

    const auto time_to_fc = timespan_t::from_seconds( ( active.basic_attack -> base_cost() - resources.current[ RESOURCE_FOCUS ] ) /
                                                        resource_regen_per_second( RESOURCE_FOCUS ) );
    const auto time_to_cd = active.basic_attack -> cooldown -> remains();
    const auto remains = std::max( time_to_cd, time_to_fc );
    // 2018-07-23 - hunter pets seem to have a "generic" lag of about .6s on basic attack usage
    const auto delay_mean = o() -> options.pet_basic_attack_delay;
    const auto delay_stddev = 100_ms;
    const auto lag = o() -> bugs ? rng().gauss( delay_mean, delay_stddev ) : 0_ms;
    return std::max( remains + lag, 100_ms );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;

  void init_spells() override;
};

double hunter_main_pet_base_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = stable_pet_t::composite_player_target_multiplier( target, school );

  if ( auto main_pet = o()->pets.main )  // theoretically should always be there /shrug
  {
    const hunter_main_pet_td_t* td = main_pet->find_target_data( target );
    if ( td && td->dots.bloodshed->is_ticking() && spells.bloodshed->effectN( 2 ).has_common_school( school ) )
    {
      double bonus = spells.bloodshed->effectN( 2 ).percent();
      if ( td->debuffs.venomous_bite->check() )
      {
        bonus *= 1 + o()->talents.venomous_bite->effectN( 1 ).percent();
      }

      m *= 1 + bonus;
    }
  }

  return m;
}

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
  // TODO check target_pet_multiplier
  const double target_da_multiplier = ( 1.0 / s->target_da_multiplier );

  const double amount = s->result_total * p->buffs.beast_cleave->check_value() * target_da_multiplier;
  p->active.beast_cleave->execute_on_target( s->target, amount );
}

// Template for common hunter pet action code.
template <class T_PET, class Base>
struct hunter_pet_action_t: public Base
{
private:
  using ab = Base;
public:

  hunter_pet_action_t( util::string_view n, T_PET* p, const spell_data_t* s = spell_data_t::nil() ):
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

    //Shared
    ab::apply_affecting_aura( o() -> talents.specialized_arsenal );

    //Beast Mastery 
    ab::apply_affecting_aura( o() -> talents.savagery );
    ab::apply_affecting_aura( o() -> talents.improved_kill_command );

    //Marksmanship

    //Survival
    ab::apply_affecting_aura( o() -> talents.killer_companion );

    //Hero Trees
    ab::apply_affecting_aura( o() -> talents.overshadow );
  }

  T_PET* p()             { return static_cast<T_PET*>( ab::player ); }
  const T_PET* p() const { return static_cast<T_PET*>( ab::player ); }

  hunter_t* o()             { return p() -> o(); }
  const hunter_t* o() const { return p() -> o(); }

  bool usable_moving() const override { return true; }
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

    ab::weapon = &( p -> main_hand_weapon );
    ab::weapon_multiplier = 1;

    ab::base_execute_time = ab::weapon -> swing_time;
    ab::school = SCHOOL_PHYSICAL;
    ab::may_crit = true;
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

// Template for common hunter main pet action code.
template <class Base>
struct hunter_main_pet_action_t: public hunter_pet_action_t < hunter_main_pet_t, Base >
{
private:
  using ab = hunter_pet_action_t<hunter_main_pet_t, Base>;
public:

  struct {
    damage_affected_by aspect_of_the_beast;
    damage_affected_by spirit_bond;
    damage_affected_by tip_of_the_spear;
  } affected_by;

  hunter_main_pet_action_t( util::string_view n, hunter_main_pet_t* p, const spell_data_t* s = spell_data_t::nil() ) : ab( n, p, s ), affected_by()
  {
    affected_by.aspect_of_the_beast = parse_damage_affecting_aura( this, ab::o() -> talents.aspect_of_the_beast );
    affected_by.spirit_bond         = parse_damage_affecting_aura( this, ab::o() -> mastery.spirit_bond );
    affected_by.tip_of_the_spear         = parse_damage_affecting_aura( this, ab::o()->talents.tip_of_the_spear_buff );
  }

  void init() override
  {
    ab::init();

    if ( affected_by.aspect_of_the_beast.direct )
      ab::base_dd_multiplier *= 1 + ab::o() -> talents.aspect_of_the_beast -> effectN( affected_by.aspect_of_the_beast.direct ).percent();
    if ( affected_by.aspect_of_the_beast.tick )
      ab::base_td_multiplier *= 1 + ab::o() -> talents.aspect_of_the_beast -> effectN( affected_by.aspect_of_the_beast.tick ).percent();
  }

  hunter_main_pet_td_t* td( player_t* t = nullptr ) const
  { return ab::p() -> get_target_data( t ? t : ab::target ); }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double am = ab::composite_da_multiplier( s );

    if ( affected_by.spirit_bond.direct )
    {
      double bonus = ab::o() -> cache.mastery() * ab::o() -> mastery.spirit_bond -> effectN( affected_by.spirit_bond.direct ).mastery_value();
      // TODO implement range
      bonus *= 1 + ab::o()->mastery.spirit_bond_buff->effectN( 1 ).percent();

      am *= 1 + bonus;
    }

    if ( affected_by.tip_of_the_spear.direct && ab::o()->buffs.tip_of_the_spear->check() )
    {
      double tip_bonus = ab::o()->talents.tip_of_the_spear->effectN( affected_by.tip_of_the_spear.direct ).percent();

      if ( ab::o()->talents.flankers_advantage.ok() )
      {
        double max_bonus = ab::o()->talents.flankers_advantage->effectN( 6 ).percent() - tip_bonus;

        // Seems that the amount of the 15% bonus given is based on the ratio of player crit % out of a cap of 50% from effect 5.
        double crit_chance = std::min( ab::o()->cache.attack_crit_chance(), ab::o()->talents.flankers_advantage->effectN( 5 ).percent() );

        tip_bonus += max_bonus * crit_chance / ab::o()->talents.flankers_advantage->effectN( 5 ).percent();
      }

      if ( ab::o()->buffs.relentless_primal_ferocity->check() )
        tip_bonus += ab::o()->talents.relentless_primal_ferocity_buff->effectN( 2 ).percent();

      am *= 1 + tip_bonus;
    }

    return am;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double am = ab::composite_ta_multiplier( s );

    if ( affected_by.spirit_bond.tick )
    {
      double bonus = ab::o() -> cache.mastery() * ab::o() -> mastery.spirit_bond -> effectN( affected_by.spirit_bond.tick ).mastery_value();
      // TODO implement range
      bonus *= 1 + ab::o()->mastery.spirit_bond_buff->effectN( 3 ).percent();

      am *= 1 + bonus;
    }

    return am;
  }
};

using hunter_main_pet_attack_t = hunter_main_pet_action_t< melee_attack_t >;

// ==========================================================================
// Hunter Pet Attacks
// ==========================================================================

// Kill Command ============================================================

template <class Pet>
struct kill_command_base_t : public hunter_pet_action_t<Pet, melee_attack_t>
{
private:
  using ab = hunter_pet_action_t<Pet, melee_attack_t>;
public:

  struct {
    double percent = 0;
    double multiplier = 1;
    benefit_t* benefit = nullptr;
  } killer_instinct;

  kill_command_base_t( Pet* p, const spell_data_t* s ) :
    ab( "kill_command", p, s )
  {
    ab::background = true;
    ab::proc = true;
    
    ab::base_dd_multiplier *= 1 + ab::o() -> talents.alpha_predator -> effectN( 2 ).percent();

    if ( ab::o() -> talents.killer_instinct.ok() )
    {
      killer_instinct.percent = ab::o() -> talents.killer_instinct -> effectN( 2 ).base_value();
      killer_instinct.multiplier = 1 + ab::o() -> talents.killer_instinct -> effectN( 1 ).percent();
      killer_instinct.benefit = ab::o() -> get_benefit( "killer_instinct" );
    }
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double cm = ab::composite_crit_damage_bonus_multiplier();

    if ( ab::o() -> talents.go_for_the_throat.ok() )
    {
      cm *= 1 + ab::o() -> talents.go_for_the_throat -> effectN( 2 ).percent() * ab::o() -> cache.attack_crit_chance();
    }

    return cm;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double am = ab::composite_da_multiplier( s );

    if ( killer_instinct.percent )
    {
      const bool active = s->target->health_percentage() < killer_instinct.percent;
      killer_instinct.benefit->update( active );
      if ( active )
        am *= killer_instinct.multiplier;
    }

    return am;
  }
};

struct kill_command_db_t: public kill_command_base_t<dire_critter_t>
{
  kill_command_db_t( dire_critter_t* p ) :
    kill_command_base_t( p, p -> find_spell( 426703 ) )
  {
  }

  void impact( action_state_t* s ) override
  {
    kill_command_base_t::impact( s );

    if ( o() -> talents.kill_cleave.ok() && s -> action -> result_is_hit( s -> result ) &&
      s -> action -> sim -> active_enemies > 1 && p() -> buffs.beast_cleave -> up() )
    {
      const double target_da_multiplier = ( 1.0 / s -> target_da_multiplier );
      const double amount = s -> result_total * o() -> talents.kill_cleave -> effectN( 1 ).percent() * target_da_multiplier;
      p() -> active.kill_cleave -> execute_on_target( s -> target, amount );
    }
  }
};

struct kill_command_bm_t: public kill_command_base_t<hunter_main_pet_base_t>
{
  struct {
    double chance = 0;
  } dire_command;

  kill_command_bm_t( hunter_main_pet_base_t* p ) :
    kill_command_base_t( p, p -> find_spell( 83381 ) )
  {
  }

  void execute() override
  {
    kill_command_base_t::execute();

    if ( rng().roll( dire_command.chance ) )
    {
      o() -> actions.dire_command -> execute(); 
      o() -> procs.dire_command -> occur();
    }
  }

  void impact( action_state_t* s ) override
  {
    kill_command_base_t::impact( s );

    if ( o() -> talents.kill_cleave.ok() && s -> action -> result_is_hit( s -> result ) &&
      s -> action -> sim -> active_enemies > 1 && p() -> hunter_pet_t::buffs.beast_cleave -> up() )
    {
      const double target_da_multiplier = ( 1.0 / s -> target_da_multiplier );
      const double amount = s -> result_total * o() -> talents.kill_cleave -> effectN( 1 ).percent() * target_da_multiplier;
      p() -> active.kill_cleave -> execute_on_target( s -> target, amount );
    }

    auto pet = o() -> pets.main;
    if ( pet == p() && o() -> talents.wild_instincts.ok() && o() -> buffs.call_of_the_wild -> check() )
    {
      o() -> get_target_data( s -> target ) -> debuffs.wild_instincts -> trigger();
    }

    if( o()->buffs.frenzied_tear->check() && p()==o()->pets.main )
    {
      double amount = s -> result_mitigated * o() -> talents.frenzied_tear -> effectN( 2 ).percent() / (1 + s->result_crit_bonus);
      p()->active.frenzied_tear->execute_on_target( s -> target, amount );
      o()->buffs.frenzied_tear->decrement();
      if( o() -> talents.furious_assault.ok() && o() -> rng().roll( o() -> talents.furious_assault -> effectN( 1 ).percent() ) )
      {
        o()->buffs.furious_assault->trigger();
        o()->cooldowns.barbed_shot->reset( true );
      }
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double am = kill_command_base_t::composite_target_multiplier( t );

    auto pet = o()->pets.main;
    if ( p() == pet || p() == o()->pets.animal_companion )
    {
      const hunter_main_pet_td_t* td = pet -> find_target_data( target );
      if ( td && td -> debuffs.venomous_bite -> check() )
      {
        am *= 1 + td -> debuffs.venomous_bite -> data().effectN( 1 ).percent();
      }
    }

    return am;
  }
};

struct kill_command_sv_t : public hunter_main_pet_attack_t
{
  kill_command_sv_t( hunter_main_pet_t* p ) : hunter_main_pet_attack_t( "kill_command", p, p->find_spell( 259277 ) )
  {
    if ( ! o() -> talents.bloodseeker.ok() )
      dot_duration = 0_ms;

    base_multiplier *= 1 + o()->talents.alpha_predator->effectN( 2 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    hunter_main_pet_attack_t::impact( s );

    if( o()->buffs.frenzied_tear->check() && p()==o()->pets.main )
    {
      double amount = s->result_mitigated * o()->talents.frenzied_tear->effectN( 2 ).percent() / (1 + s->result_crit_bonus);
      p()->active.frenzied_tear->execute_on_target( s->target, amount );
      o()->buffs.frenzied_tear->decrement();
      if( o()->talents.furious_assault.ok() && o()->rng().roll( o()->talents.furious_assault->effectN( 1 ).percent() ) )
      {
        o()->buffs.furious_assault->trigger();
      }
    }

    if ( o()->talents.sentinel.ok() )
      o()->trigger_sentinel( s->target, false, o()->procs.sentinel_stacks );
  }
  
  void trigger_dot( action_state_t* s ) override
  {
    hunter_main_pet_attack_t::trigger_dot( s );

    o() -> trigger_bloodseeker_update();
  }

  void last_tick( dot_t* d ) override
  {
    hunter_main_pet_attack_t::last_tick( d );

    o() -> trigger_bloodseeker_update();
  }

  int n_targets() const override
  {
    if ( o()->buffs.exposed_flank->up() )
      return as<int>( o()->buffs.exposed_flank->check_value() );

    return hunter_main_pet_attack_t::n_targets();
  }
};

// Beast Cleave ==============================================================

struct beast_cleave_attack_t: public hunter_pet_action_t<hunter_pet_t, melee_attack_t>
{
  beast_cleave_attack_t( hunter_pet_t* p ) :
    hunter_pet_action_t( "beast_cleave", p, p -> find_spell( 118459 ) )
  {
    background = true;
    callbacks = false;
    may_miss = false;
    may_crit = false;
    proc = false;
    // The starting damage includes all the buffs
    base_dd_min = base_dd_max = 0;
    spell_power_mod.direct = attack_power_mod.direct = 0;
    weapon_multiplier = 0;

    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();
  }

  void init() override
  {
    hunter_pet_action_t::init();
    snapshot_flags |= STATE_TGT_MUL_DA;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    hunter_pet_action_t::available_targets( tl );

    // Cannot hit the original target.
    range::erase_remove( tl, target );

    return tl.size();
  }
};

// Kill Cleave ==============================================================

struct kill_cleave_t: public hunter_pet_action_t<hunter_pet_t, melee_attack_t>
{
  kill_cleave_t( hunter_pet_t* p ) : hunter_pet_action_t( "kill_cleave", p, p -> find_spell( 389448 ) )
  {
    background = true;
    callbacks = false;
    may_miss = false;
    may_crit = false;
    proc = false;
    // The starting damage includes all the buffs
    base_dd_min = base_dd_max = 0;
    spell_power_mod.direct = attack_power_mod.direct = 0;
    weapon_multiplier = 0;

    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();
  }

  void init() override
  {
    hunter_pet_action_t::init();
    snapshot_flags |= STATE_TGT_MUL_DA;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    hunter_pet_action_t::available_targets( tl );

    // Cannot hit the original target.
    range::erase_remove( tl, target );

    return tl.size();
  }
};

// Laceration ===============================================================

struct laceration_t : public residual_action::residual_periodic_action_t<hunter_pet_action_t<hunter_pet_t, attack_t>>
{
  laceration_t( hunter_pet_t* p ) : 
    residual_action::residual_periodic_action_t<hunter_pet_action_t<hunter_pet_t, attack_t>>( "laceration", p, p -> find_spell( 459560 ) )
  { }
};

// Frenzied Tear ==================================================================

struct frenzied_tear_t: public hunter_pet_action_t<hunter_pet_t, melee_attack_t>
{
  frenzied_tear_t( hunter_pet_t* p ) : hunter_pet_action_t( "frenzied_tear", p, p -> find_spell( 83381 ) )
  {
    background = true;
    // The starting damage includes all the buffs
    base_dd_min = base_dd_max = 0;
    spell_power_mod.direct = attack_power_mod.direct = 0;
    weapon_multiplier = 0;
  }

  void init() override
  {
    hunter_pet_action_t::init();

    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_TGT_MUL_DA;
  }
};

// Melee ================================================================

struct pet_melee_t : public hunter_pet_melee_t<hunter_pet_t>
{
  pet_melee_t( util::string_view n, hunter_pet_t* p ) :
    hunter_pet_melee_t( n, p )
  {
  }

  void impact( action_state_t* s ) override
  {
    hunter_pet_melee_t::impact( s );

    trigger_beast_cleave( s );

    if( ( p()==o()->pets.main || p()==o()->pets.animal_companion ) && o()->rng().roll( o()->tier_set.tww_s1_bm_4pc->effectN( 1 ).percent() ) )
    {
      o()->buffs.harmonize->trigger();
    }
  }
};

// Claw/Bite/Smack ======================================================

struct basic_attack_base_t : public hunter_main_pet_attack_t
{
  basic_attack_base_t( hunter_main_pet_t* p, util::string_view n, util::string_view suffix ):
    hunter_main_pet_attack_t( fmt::format("{}{}", n, suffix), p, p -> find_pet_spell( n ) )
  {
    school = SCHOOL_PHYSICAL;
  }

  void execute() override
  {
    if ( o() -> talents.wild_attacks.ok() )
    {
      p() -> buffs.wild_attacks -> trigger();
    }
    
    hunter_main_pet_attack_t::execute();    
  }

  void impact( action_state_t* s ) override
  {
    hunter_main_pet_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      trigger_beast_cleave( s );

    if ( o() -> talents.howl_of_the_pack.ok() && s -> result == RESULT_CRIT )
    {
      o() -> buffs.howl_of_the_pack -> trigger();
    }

    if ( rng().roll( o() -> talents.frenzied_tear -> effectN( 1 ).percent() ) )
    {
      o()->cooldowns.kill_command->reset( true ); 
      o()->buffs.frenzied_tear->trigger(); 
    }

    if ( p()->buffs.wild_attacks->at_max_stacks() )
    {
      p()->buffs.wild_attacks->expire();
      p()->buffs.wild_attacks_dmg_amp->trigger();
    }
  }

  double composite_crit_chance() const override
  {
    double cc = hunter_main_pet_attack_t::composite_crit_chance();

    if ( p()->buffs.wild_attacks->at_max_stacks() )
      cc = 1.0;

    return cc;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double am = hunter_main_pet_attack_t::composite_da_multiplier( s );

    //2024-08-31: Wild Attacks seems to amp the damage of the basic attack after the guaranteed crit, regardless of it being a crit or not.  
    if ( p()->buffs.wild_attacks_dmg_amp->up() )
    {
      am *= 1 + o()->cache.attack_crit_chance();
      p()->buffs.wild_attacks_dmg_amp->expire();
    }

    return am;
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

    p -> active.basic_attack = this;
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

struct pack_coordination_ba_t : public basic_attack_base_t
{
  pack_coordination_ba_t( hunter_main_pet_t* p, util::string_view n ) : basic_attack_base_t( p, n, "_pack_coordination" )
  {
    background = true;
  }

  double cost() const override
  {
    return 0;
  }
};

struct brutal_companion_ba_t : public basic_attack_base_t
{
  brutal_companion_ba_t( hunter_main_pet_t* p, util::string_view n ) : basic_attack_base_t( p, n, "_brutal_companion" )
  {
    background = true;
    base_multiplier *= 1 + o()->talents.brutal_companion->effectN( 2 ).percent();
  }

  double cost() const override
  {
    return 0;
  }
};

// Flanking Strike ===================================================

struct flanking_strike_t: public hunter_main_pet_attack_t
{
  flanking_strike_t( hunter_main_pet_t* p ) : hunter_main_pet_attack_t( "flanking_strike", p, p->find_spell( 259516 ) )
  {
    background = true;
    
    if ( o()->talents.exposed_flank.ok() )
    {
      aoe = as<int>( o()->talents.exposed_flank->effectN( 3 ).base_value() );
      base_aoe_multiplier = o()->talents.exposed_flank->effectN( 1 ).percent();
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double bonus = hunter_main_pet_attack_t::bonus_da( s );

    // Adding raw player ap (* effect 1 from talent spell tooltip).
    bonus += o()->cache.attack_power() * o()->composite_attack_power_multiplier() * o()->talents.flanking_strike->effectN( 1 ).percent();

    return bonus;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double am = hunter_main_pet_attack_t::composite_da_multiplier( s );

    // Flanking Strike is getting mastery applied twice.
    double bonus = o() -> cache.mastery() * o() -> mastery.spirit_bond -> effectN( affected_by.spirit_bond.direct ).mastery_value();
    // TODO implement range
    bonus *= 1 + o()->mastery.spirit_bond_buff->effectN( 1 ).percent();

    am *= 1 + bonus;

    return am;
  }
};

// Coordinated Assault ====================================================

struct coordinated_assault_t: public hunter_main_pet_attack_t
{
  coordinated_assault_t( hunter_main_pet_t* p ) : hunter_main_pet_attack_t( "coordinated_assault", p, p->find_spell( 360969 ) )
  {
    background = true;
  }
};

// Spearhead ====================================================

struct spearhead_t: public hunter_main_pet_attack_t
{
  spearhead_t( hunter_main_pet_t* p ):
    hunter_main_pet_attack_t( "spearhead", p, p->o()->talents.spearhead_attack )
  {
    background = true;
  }
};

// Stomp ===================================================================

struct stomp_t : public hunter_pet_action_t<hunter_pet_t, attack_t>
{
  stomp_t( hunter_pet_t* p ) : hunter_pet_action_t( "stomp", p, p -> find_spell( 201754 ) )
  {
    aoe = -1;
    base_dd_multiplier *= o() -> talents.stomp -> effectN( 1 ).base_value();
  }

  double bleed_amount = o() -> find_spell( 459555 ) -> effectN( 1 ).percent(); 

  void impact( action_state_t* s ) override
  {
    hunter_pet_action_t::impact( s );

    //Only the main pet or animal companion can trigger laceration
    auto pet = o() -> pets.main;
    auto animal_companion = o() -> pets.animal_companion;
    if ( !( pet == p() || animal_companion == p() ) )
      return;

    if( p() -> active.laceration && s -> result == RESULT_CRIT )
    {
      double amount = s -> result_amount * bleed_amount; 
      residual_action::trigger( p() -> active.laceration, s -> target, amount );
    }
  }
};

// Bloodshed ===============================================================

struct bloodshed_t : hunter_main_pet_attack_t
{
  bloodshed_t( hunter_main_pet_t* p ) : hunter_main_pet_attack_t( "bloodshed", p, p -> spells.bloodshed )
  {
    background = true;
    aoe = o() -> talents.shower_of_blood.ok() ? as<int>( o() -> talents.shower_of_blood -> effectN( 1 ).base_value() ) : 1;
  }

  void impact( action_state_t* s ) override
  {
    hunter_main_pet_attack_t::impact( s );

    (void) td( s -> target ); // force target_data creation for damage amp handling
  }
};

// Bestial Wrath ===========================================================

struct bestial_wrath_t : hunter_pet_action_t<hunter_main_pet_base_t, melee_attack_t>
{
  bestial_wrath_t( hunter_main_pet_base_t* p ) : hunter_pet_action_t( "bestial_wrath", p, p -> find_spell( 344572 ) )
  {
    background = true;
  }

  void impact( action_state_t* s ) override
  {
    hunter_pet_action_t::impact( s );
  }
};

// Ravenous Leap (Fenryr) ===================================================

struct ravenous_leap_t : public hunter_pet_action_t<fenryr_t, attack_t>
{
  ravenous_leap_t( fenryr_t* p ) : hunter_pet_action_t( "ravenous_leap", p, p -> find_spell( 459753 ) )
  {
    background = true;
  }

  void impact( action_state_t* s ) override
  {
    hunter_pet_action_t::impact( s );
  }
};

} // end namespace pets::actions

hunter_main_pet_td_t::hunter_main_pet_td_t( player_t* target, hunter_main_pet_t* p ) : actor_target_data_t( target, p )
{
  dots.bloodseeker    = target -> get_dot( "kill_command", p );
  dots.bloodshed      = target -> get_dot( "bloodshed", p );
  dots.laceration     = target -> get_dot( "laceration", p );
  dots.ravenous_leap  = target -> get_dot( "ravenous_leap", p );
  dots.spearhead      = target -> get_dot( "spearhead", p );

  debuffs.venomous_bite = 
    make_buff( *this, "venomous_bite", p -> find_spell( 459668 ) )
      -> set_default_value_from_effect( 1 )
      //Grab duration from bloodshed as they're interlinked and venomous bite has no duration in data
      -> set_duration( p -> find_spell( 346396 ) -> duration() );
}

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

  main_hand_attack = new actions::pet_melee_t( "melee", this );
  
  active.beast_cleave = new actions::beast_cleave_attack_t( this );
  
  if ( o() -> talents.laceration.ok() )
    active.laceration = new actions::laceration_t( this );
}

void stable_pet_t::init_spells()
{
  hunter_pet_t::init_spells();

  if( o() -> talents.bloody_frenzy.ok() )
    active.stomp = new actions::stomp_t( this );
}

void hunter_main_pet_base_t::init_spells()
{
  stable_pet_t::init_spells();

  if ( o() -> specialization() == HUNTER_BEAST_MASTERY )
    active.kill_command = new actions::kill_command_bm_t( this );

  spells.bloodshed = find_spell( 321538 );

  if ( o() -> specialization() == HUNTER_BEAST_MASTERY )
    active.bestial_wrath = new actions::bestial_wrath_t( this );

  if ( o() -> talents.kill_cleave.ok() )
    active.kill_cleave = new actions::kill_cleave_t( this );

  if ( o() -> talents.stomp.ok() || o() -> talents.bloody_frenzy.ok() )
    active.stomp = new actions::stomp_t( this );

  if( o() -> talents.frenzied_tear.ok() )
    active.frenzied_tear = new actions::frenzied_tear_t( this );
}

void hunter_main_pet_t::init_spells()
{
  hunter_main_pet_base_t::init_spells();

  if ( o()->specialization() == HUNTER_SURVIVAL )
    active.kill_command = new actions::kill_command_sv_t( this );

  if ( o() -> talents.flanking_strike.ok() )
    active.flanking_strike = new actions::flanking_strike_t( this );

  if ( o() -> talents.bloodshed.ok() )
    active.bloodshed = new actions::bloodshed_t( this );

  if ( o() -> talents.coordinated_assault.ok() )
    active.coordinated_assault = new actions::coordinated_assault_t( this );

  if ( o() -> talents.spearhead.ok() )
    active.spearhead = new actions::spearhead_t( this );

  if ( o() -> talents.brutal_companion.ok() )
    active.brutal_companion_ba = new actions::brutal_companion_ba_t( this, "Claw" );

  if ( o() -> talents.pack_coordination.ok())
    active.pack_coordination_ba = new actions::pack_coordination_ba_t( this, "Claw" );
}

void dire_critter_t::init_spells()
{
  hunter_pet_t::init_spells();
}

void dark_hound_t::init_spells()
{
  hunter_pet_t::init_spells();

  main_hand_attack->school = SCHOOL_SHADOW;
}

void fenryr_t::init_spells()
{
  hunter_pet_t::init_spells();

  active.ravenous_leap = new actions::ravenous_leap_t( this );
}

void hunter_main_pet_base_t::init_special_effects()
{
  stable_pet_t::init_special_effects();

  if( o() -> talents.laceration.ok() )
  {
    struct laceration_cb_t : public dbc_proc_callback_t
    {
      double bleed_amount; 
      action_t* bleed; 

      laceration_cb_t( const special_effect_t& e, double amount, action_t* bleed ) : dbc_proc_callback_t( e.player, e ),
        bleed_amount( amount ), bleed( bleed )
      {
      }

      void execute( action_t*, action_state_t* s ) override
      {
        if ( s && s -> target -> is_sleeping() )
        {
          return;
        }

        double amount = s -> result_amount * bleed_amount;
        if ( amount > 0 )
          residual_action::trigger( bleed, s -> target, amount );
      }  
    };

    auto const effect = new special_effect_t( this );
    effect -> name_str = "laceration";
    effect -> spell_id =  459555;
    effect -> proc_flags2_ = PF2_CRIT;
    //Pet melee, bestial wrath on demand damage and Kill Command are procs in simc implemenation
    effect -> set_can_proc_from_procs(true);
    special_effects.push_back( effect );

    auto cb = new laceration_cb_t( *effect, find_spell( 459555 ) -> effectN( 1 ).percent(), hunter_pet_t::active.laceration );
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

struct trick_shots_t : public buff_t
{
  using buff_t::buff_t;

  void expire_override( int remaining_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( remaining_stacks, remaining_duration );

    hunter_t* p = debug_cast<hunter_t*>( player );
    p -> buffs.razor_fragments -> trigger();
  }
};

} // end namespace buffs

void hunter_t::trigger_bloodseeker_update()
{
  if ( !talents.bloodseeker.ok() )
    return;

  int bleeding_targets = 0;
  for ( const player_t* t : sim -> target_non_sleeping_list )
  {
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

int hunter_t::ticking_dots( hunter_td_t* td )
{
  int dots = 0;

  auto hunter_dots = td->dots;
  dots += hunter_dots.a_murder_of_crows->is_ticking();
  dots += hunter_dots.barbed_shot->is_ticking();
  dots += hunter_dots.black_arrow->is_ticking();
  dots += hunter_dots.serpent_sting->is_ticking();
  dots += hunter_dots.wildfire_bomb->is_ticking();
  dots += hunter_dots.cull_the_herd->is_ticking();

  auto pet_dots = pets.main->get_target_data( td->target )->dots;
  dots += pet_dots.bloodshed->is_ticking();
  dots += pet_dots.laceration->is_ticking();
  dots += pet_dots.ravenous_leap->is_ticking();
  dots += pet_dots.bloodseeker->is_ticking();
  dots += pet_dots.spearhead->is_ticking();

  return dots;
}

void hunter_t::trigger_basilisk_collar_update()
{
  if ( !talents.basilisk_collar.ok() )
    return;

  for ( player_t* t : sim -> target_non_sleeping_list )
  {
    if ( t -> is_enemy() )
    {
      auto td = get_target_data( t );
      int current = td -> debuffs.basilisk_collar -> check(); 
      int new_stacks = ticking_dots( td );

      new_stacks = std::min( new_stacks, td -> debuffs.basilisk_collar -> max_stack() );

      if ( current < new_stacks )
      {
        td -> debuffs.basilisk_collar -> trigger( new_stacks - current );
      }
      else if ( current > new_stacks )
      {
        td -> debuffs.basilisk_collar -> decrement( current - new_stacks ); 
      }
    }
  }
}

void hunter_t::trigger_outland_venom_update()
{
  if ( !talents.outland_venom.ok() )
    return;

  for ( player_t* t : sim->target_non_sleeping_list )
  {
    if ( t->is_enemy() )
    {
      auto td        = get_target_data( t );
      int current    = td->debuffs.outland_venom->check();
      int new_stacks = ticking_dots( td );

      new_stacks = std::min( new_stacks, td->debuffs.outland_venom->max_stack() );

      if ( current < new_stacks )
        td->debuffs.outland_venom->trigger( new_stacks - current );
      else if ( current > new_stacks )
        td->debuffs.outland_venom->decrement( current - new_stacks );
    }
  }
}

void hunter_t::trigger_calling_the_shots( action_t* action, double cost )
{
  if ( !talents.calling_the_shots.ok() )
    return;

  state.focus_used_CTS += cost;
  sim -> print_debug( "{} action {} spent {} focus, calling the shots now at {}", name(), action->name(), cost, state.focus_used_CTS );

  const double calling_the_shots_value = talents.calling_the_shots -> effectN( 2 ).base_value();
  while ( state.focus_used_CTS >= calling_the_shots_value )
  {
    state.focus_used_CTS -= calling_the_shots_value;
    cooldowns.trueshot -> adjust( - talents.calling_the_shots -> effectN( 1 ).time_value() );
    procs.calling_the_shots -> occur();
  }
}

void hunter_t::consume_trick_shots()
{
  if ( buffs.volley -> up() )
    return;

  buffs.trick_shots -> decrement();
}

void hunter_t::trigger_sentinel( player_t* target, bool force, proc_t* proc )
{
  if ( force || buffs.eyes_closed->check() || rng().roll( 0.22 ) )
  {
    hunter_td_t* td = get_target_data( target );
    buff_t* sentinel = td->debuffs.sentinel;

    int stacks = sentinel->check();

    if ( proc )
      proc->occur();

    sentinel->trigger();
    if ( rng().roll( talents.release_and_reload->effectN( 1 ).percent() ) )
    {
      procs.release_and_reload_stacks->occur();
      sentinel->trigger();
    }

    if ( !stacks && talents.extrapolated_shots.ok() )
    {
      procs.extrapolated_shots_stacks->occur();
      sentinel->trigger( as<int>( talents.extrapolated_shots->effectN( 1 ).base_value() ) );
      if ( rng().roll( talents.release_and_reload->effectN( 1 ).percent() ) )
      {
        procs.release_and_reload_stacks->occur();
        sentinel->trigger();
      }
    }

    // TODO: Seen strange behavior with multiple implosions triggering, ticks desyncing from the 2 second period by possibly 
    // overwriting or ticking in parallel, but for now model as just allowing one to tick at a time.
    if ( !td->sentinel_imploding && sentinel->check() > talents.sentinel->effectN( 1 ).base_value() && rng().roll( 0.32 ) )
    {
      procs.sentinel_implosions->occur();
      trigger_sentinel_implosion( td );
    }
  }
}

void hunter_t::trigger_sentinel_implosion( hunter_td_t* td )
{
  // Seems to tick one last time after it consumes the last Sentinel stack, resulting in a Sentinel 
  // re-application shortly after expiration but before the next tick to continue being consumed.
  if ( td->debuffs.sentinel->check() )
  {
    td->sentinel_imploding = true;
    actions.sentinel->execute_on_target( td->target );
    make_event( sim, 2_s, [ this, td ]() {
      if ( td->sentinel_imploding )
        trigger_sentinel_implosion( td );
    } );
  }
  else
  {
    td->sentinel_imploding = false;
  }
}

void hunter_t::trigger_symphonic_arsenal()
{
  if ( actions.symphonic_arsenal )
    for ( player_t* t : sim->target_non_sleeping_list )
      if ( t->is_enemy() && get_target_data( t )->debuffs.sentinel->check() )
        actions.symphonic_arsenal->execute_on_target( t );
}

void hunter_t::trigger_lunar_storm( player_t* /* target */ )
{
  if ( actions.lunar_storm )
  {
    cooldowns.lunar_storm->start();
    make_repeating_event(
        sim, talents.lunar_storm_ground->effectN( 2 ).period(),
        [ this ] {
          auto& tl = actions.lunar_storm->target_list();
          if ( tl.size() )
          {
            rng().shuffle( tl.begin(), tl.end() );
            actions.lunar_storm->execute_on_target( tl.front() );
          }
        },
        as<int>( talents.lunar_storm_ground->duration() / talents.lunar_storm_ground->effectN( 2 ).period() ) );
  }
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

struct auto_shot_t : public auto_attack_base_t<ranged_attack_t>
{
  struct state_t : public action_state_t
  {
    using action_state_t::action_state_t;

    proc_types2 cast_proc_type2() const override
    {
      // Auto shot seems to trigger Meticulous Scheming (and possibly other
      // effects that care about casts).
      return PROC2_CAST_DAMAGE;
    }
  };

  double wild_call_chance = 0;

  auto_shot_t( hunter_t* p ) : auto_attack_base_t( "auto_shot", p, p->specs.auto_shot )
  {
    wild_call_chance = p -> talents.wild_call -> effectN( 1 ).percent();
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  void impact( action_state_t* s ) override
  {
    auto_attack_base_t::impact( s );

    if ( p() -> buffs.lock_and_load -> trigger() )
      p() -> cooldowns.aimed_shot -> reset( true );

    if ( s -> result == RESULT_CRIT && p() -> talents.wild_call.ok() && rng().roll( wild_call_chance ) )
    {
      p() -> cooldowns.barbed_shot -> reset( true );
      p() -> procs.wild_call -> occur();
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

//==============================
// Shared attacks
//==============================

struct residual_bleed_base_t : public residual_action::residual_periodic_action_t<hunter_ranged_attack_t>
{
  residual_bleed_base_t( util::string_view n, hunter_t* p, const spell_data_t* s )
    : residual_periodic_action_t( n, p, s )
  {
  }
};

// Legacy of the Windrunners - Wind Arrow (Marksmanship Talent) ==========================================

struct wind_arrow_t final : public hunter_ranged_attack_t
{
  struct
  {
    double multiplier = 0;
    double high, low;
  } careful_aim;

  wind_arrow_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->talents.legacy_of_the_windrunners_wind_arrow )
  {
    background = dual = true;

    if ( p->talents.careful_aim.ok() )
    {
      careful_aim.high       = p->talents.careful_aim->effectN( 1 ).base_value();
      careful_aim.low        = p->talents.careful_aim->effectN( 2 ).base_value();
      careful_aim.multiplier = p->talents.careful_aim->effectN( 3 ).percent();
    }
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( p()->talents.wailing_arrow.ok() )
    {
      p()->buffs.wailing_arrow_counter->trigger();
      if ( p()->buffs.wailing_arrow_counter->at_max_stacks() )
      {
        p()->buffs.wailing_arrow_override->trigger();
        p()->buffs.wailing_arrow_counter->expire();
      }
    }
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = hunter_ranged_attack_t::composite_target_da_multiplier( t );

    if ( careful_aim.multiplier )
    {
      const double target_health_pct = t->health_percentage();
      if ( target_health_pct > careful_aim.high || target_health_pct < careful_aim.low )
        m *= 1 + careful_aim.multiplier;
    }

    return m;
  }
};

// Rapid Fire/Rapid Fire Barrage (Marksmanship Talent) ========================================================

struct rapid_fire_tick_t final : public hunter_ranged_attack_t
{
  const int trick_shots_targets;

  struct
  {
    double chance = 0;
    wind_arrow_t* wind_arrow = nullptr;
  } legacy_of_the_windrunners;

  rapid_fire_tick_t( util::string_view n, hunter_t* p )
    : hunter_ranged_attack_t( n, p, p->talents.rapid_fire_tick ),
      trick_shots_targets( as<int>( p->talents.trick_shots_data->effectN( 3 ).base_value() +
                                    p->talents.light_ammo->effectN( 2 ).base_value() +
                                    p->talents.heavy_ammo->effectN( 2 ).base_value() ) )
  {
    background = dual = true;
    direct_tick = true;
    radius = 8;
    base_aoe_multiplier = p->talents.trick_shots_data->effectN( 5 ).percent() + p->talents.heavy_ammo->effectN( 4 ).percent();

    // energize
    parse_effect_data( p->find_spell( 263585 )->effectN( 1 ) );

    if ( p->talents.legacy_of_the_windrunners.ok() )
    {
      legacy_of_the_windrunners.chance = p->talents.legacy_of_the_windrunners->proc_chance();
      legacy_of_the_windrunners.wind_arrow = p->get_background_action<wind_arrow_t>( "wind_arrow" );
    }
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

    p()->buffs.trick_shots->up();  // Benefit tracking

    /* This is not mentioned anywhere but testing shows that Rapid Fire inside
     * Trueshot has a 50% chance of energizing twice. Presumably to account for the
     * fact that focus is integral and 1 * 1.5 = 1. It's still affected by the
     * generic focus gen increase from Trueshot as each energize gives 3 focus when
     * combined with Nesingwary's Trapping Apparatus buff.
     */
    if ( p()->buffs.trueshot->check() && rng().roll( .5 ) )
      p()->resource_gain( RESOURCE_FOCUS, composite_energize_amount( execute_state ), p()->gains.trueshot, this );

    if ( p()->cooldowns.lunar_storm->up() )
      p()->trigger_lunar_storm( target );
  }

  void impact( action_state_t* state ) override
  {
    hunter_ranged_attack_t::impact( state );

    if ( legacy_of_the_windrunners.wind_arrow && p()->cooldowns.legacy_of_the_windrunners->up() &&
      rng().roll( legacy_of_the_windrunners.chance ) )
    {
      legacy_of_the_windrunners.wind_arrow->execute_on_target( state->target );
      p()->cooldowns.legacy_of_the_windrunners->start();
    }
  }
};

// Barrage (Marksmanship/Beast Mastery Talent) ===========================================

struct barrage_t: public hunter_spell_t
{
  struct barrage_damage_t final : public hunter_ranged_attack_t
  {
    barrage_damage_t( util::string_view n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> talents.barrage -> effectN( 1 ).trigger() )
    {
      background = dual = true;
      aoe = -1;
      reduced_aoe_targets = data().effectN( 1 ).base_value();
    }
  };

  struct rapid_fire_barrage_tick_t final : public hunter_ranged_attack_t
  {
    rapid_fire_tick_t* tick;

    rapid_fire_barrage_tick_t( util::string_view n, hunter_t* p )
      : hunter_ranged_attack_t( n, p, p->talents.rapid_fire_barrage )
    {
      aoe = 1 + as<int>( p->talents.rapid_fire_barrage_override->effectN( 3 ).base_value() );
      tick = p->get_background_action<rapid_fire_tick_t>( "rapid_fire_tick" );
      tick->base_multiplier *= p->talents.rapid_fire_barrage->effectN( 4 ).percent();
    }

    void impact( action_state_t* state ) override
    {
      hunter_ranged_attack_t::impact( state );

      tick->execute_on_target( state->target );
    }
  };

  action_t* barrage_tick = nullptr;

  barrage_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "barrage", p, p->talents.rapid_fire_barrage.ok() ? p -> talents.rapid_fire_barrage_override : p -> talents.barrage )
  {
    parse_options( options_str );

    may_miss = may_crit = false;
    channeled = true;
    // 06/08/2024: Barrage is the only ability not counting toward Calling the Shots
    triggers_calling_the_shots = false;

    if ( p->talents.rapid_fire_barrage.ok() )
    {
      rapid_fire_barrage_tick_t* rapid_fire_tick = p->get_background_action<rapid_fire_barrage_tick_t>( "rapid_fire_barrage" );
      tick_action = rapid_fire_tick;
      add_child( rapid_fire_tick->tick );
    }
    else
      tick_action = p->get_background_action<barrage_damage_t>( "barrage_damage" );

    starved_proc = p -> get_proc( "starved: barrage" );
  }

  void tick( dot_t* d ) override
  {
    hunter_spell_t::tick( d );

    if ( p()->talents.rapid_fire_barrage.ok() && p()->talents.bulletstorm->ok() && d->current_tick == 1 && 
      tick_action->execute_state && tick_action->execute_state->chain_target > 0 )

      p()->buffs.bulletstorm->increment( tick_action->execute_state->chain_target );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    if ( p()->specialization() == HUNTER_BEAST_MASTERY &&
         p()->buffs.beast_cleave->buff_duration() > p()->buffs.beast_cleave->remains() )
    {
      p() -> buffs.beast_cleave -> trigger(); 
      for ( auto pet : pets::active<pets::hunter_pet_t>( p() -> pets.main, p() -> pets.animal_companion ) )
        pet -> buffs.beast_cleave -> trigger();
    }
  }
};

// Arcane Shot ========================================================================

struct arcane_shot_base_t: public hunter_ranged_attack_t
{
  arcane_shot_base_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->specs.arcane_shot )
  {
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if( p()->tier_set.tww_s1_mm_4pc.ok() && p()->buffs.precise_shots->check() )
      p()->buffs.moving_target->trigger();

    p() -> buffs.precise_shots -> up(); // Benefit tracking
    p() -> buffs.precise_shots -> decrement();

    p()->buffs.sulfur_lined_pockets->trigger();
    if ( p()->buffs.sulfur_lined_pockets->at_max_stacks() )
    {
      p()->buffs.sulfur_lined_pockets_explosive->trigger();
      p()->buffs.sulfur_lined_pockets->expire();
    }
  }
};

struct arcane_shot_t : public arcane_shot_base_t
{
  struct arcane_shot_eagletalons_true_focus_t : public arcane_shot_base_t
  {
    arcane_shot_eagletalons_true_focus_t( util::string_view n, hunter_t* p ) : arcane_shot_base_t( n, p )
    {
      background = dual = true;
      base_multiplier *= p->talents.eagletalons_true_focus->effectN( 3 ).percent();
      base_costs[ RESOURCE_FOCUS ] = 0;
    }
  };

  arcane_shot_eagletalons_true_focus_t* eagletalons_true_focus = nullptr;

  arcane_shot_t( hunter_t* p, util::string_view options_str ) : arcane_shot_base_t( "arcane_shot", p )
  {
    parse_options( options_str );

    if ( p->specialization() == HUNTER_MARKSMANSHIP && p->talents.chimaera_shot.ok() )
      background = true;

    if ( p->talents.eagletalons_true_focus.ok() )
    {
      eagletalons_true_focus = p->get_background_action<arcane_shot_eagletalons_true_focus_t>( "arcane_shot_eagletalons_true_focus" );
      add_child( eagletalons_true_focus );
    }
  }

  void execute() override
  {
    arcane_shot_base_t::execute();

    if ( eagletalons_true_focus && p()->buffs.eagletalons_true_focus->up() )
      eagletalons_true_focus->execute_on_target( target );
  }
};

// Serpent Sting =====================================================================

struct serpent_sting_t: public hunter_ranged_attack_t
{
  serpent_sting_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->specs.serpent_sting )
  {
    background = dual = true;
  }

  // We have a whole lot of Serpent Sting variations that all need to work with the same dot.
  dot_t* get_dot( player_t* t )
  {
    if ( !t )
      t = target;
    if ( !t )
      return nullptr;

    return td( t )->dots.serpent_sting;
  }
};

// Explosive Shot (Hunter Talent)  ====================================================================

struct explosive_shot_base_t : public hunter_ranged_attack_t
{
  static const snapshot_state_e STATE_EXPLOSIVE_VENOM = STATE_USER_1;
  static const snapshot_state_e STATE_EFFECTIVENESS = STATE_USER_2;

  struct state_data_t
  {
    bool explosive_venom_ready = false;
    double effectiveness = 1.0;

    friend void sc_format_to( const state_data_t& data, fmt::format_context::iterator out )
    {
      fmt::format_to( out, "explosive_venom_ready={}, effectiveness={}", data.explosive_venom_ready, data.effectiveness );
    }
  };
  using state_t = hunter_action_state_t<state_data_t>;

  struct damage_t final : hunter_ranged_attack_t
  {
    serpent_sting_t* explosive_venom = nullptr;

    damage_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->talents.explosive_shot_damage )
    {
      background = dual = true;
      reduced_aoe_targets = p->talents.explosive_shot_cast->effectN( 2 ).base_value();
      aoe = -1;
      
      if ( p->talents.explosive_venom.ok() )
        explosive_venom = p->get_background_action<serpent_sting_t>( "serpent_sting" );
    }

    void execute() override
    {
      if ( pre_execute_state )
        update_state( pre_execute_state, result_amount_type::DMG_DIRECT );

      hunter_ranged_attack_t::execute();
    }

    void impact( action_state_t* s ) override
    {
      hunter_ranged_attack_t::impact( s );

      if ( explosive_venom && debug_cast<state_t*>( s )->explosive_venom_ready )
        explosive_venom->execute_on_target( s->target );
    }

    action_state_t* new_state() override
    {
      return new state_t( this, target );
    }

    void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
    {
      hunter_ranged_attack_t::snapshot_internal( s, flags, rt );

      if ( flags & STATE_MUL_SPELL_DA )
        s->da_multiplier *= debug_cast<state_t*>( s )->effectiveness;
    }
  };

  timespan_t grenade_juggler_reduction = 0_s;
  damage_t* explosion = nullptr;

  explosive_shot_base_t( util::string_view n, hunter_t* p, const spell_data_t* s )
    : hunter_ranged_attack_t( n, p, s )
  {
    may_miss = may_crit = false;

    explosion = p->get_background_action<damage_t>( "explosive_shot_damage" );

    grenade_juggler_reduction = p->talents.grenade_juggler->effectN( 3 ).time_value();
  }

  void init() override
  {
    hunter_ranged_attack_t::init();

    // Don't let dot ticks lose our cast state.
    snapshot_flags = STATE_EXPLOSIVE_VENOM | STATE_EFFECTIVENESS;
  }

  // We have a whole lot of Explosive Shot variations that all need to work with the same dot.
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

    if ( dot->is_ticking() )
    {
      if ( !explosion->pre_execute_state )
        explosion->pre_execute_state = explosion->get_state();
      
      // The dot should have the state from the cast that triggered it, so forward it to the explosion.
      explosion->pre_execute_state->copy_state( dot->state );
      explosion->execute_on_target( s->target );
    }
        
    hunter_ranged_attack_t::impact( s );
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

    if ( p() -> talents.explosive_venom.ok() ) 
    {
      p() -> buffs.explosive_venom -> up(); // Benefit tracking
      if( p() -> buffs.explosive_venom -> at_max_stacks() )
      {
        p() -> buffs.explosive_venom -> expire();
        p() -> buffs.explosive_venom -> increment();
      }
      else 
      {
        p() -> buffs.explosive_venom -> increment();
      }
    }
    
    p()->cooldowns.wildfire_bomb->adjust( -grenade_juggler_reduction );
    p()->buffs.bombardier->decrement();
  }

  double cost_pct_multiplier() const override
  {
    double c = hunter_ranged_attack_t::cost_pct_multiplier();

    if ( p()->buffs.bombardier->check() )
      c *= 1 + p()->talents.bombardier_buff->effectN( 1 ).percent();

    return c;
  }

  void update_ready( timespan_t ) override
  {
    timespan_t d = cooldown->duration;

    if ( p()->buffs.bombardier->check() )
    {
      d = timespan_t::zero();
    }

    hunter_ranged_attack_t::update_ready( d );
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    hunter_ranged_attack_t::snapshot_internal( s, flags, rt );

    if ( flags & STATE_EXPLOSIVE_VENOM )
      debug_cast<state_t*>( s )->explosive_venom_ready = p()->buffs.explosive_venom->at_max_stacks();
  }
};

struct explosive_shot_t : public explosive_shot_base_t
{
  explosive_shot_t( hunter_t* p, util::string_view options_str )
    : explosive_shot_base_t( "explosive_shot", p, p->talents.explosive_shot )
  {
    parse_options( options_str );
  }
  
  void execute() override
  {
    explosive_shot_base_t::execute();

    if ( p()->buffs.tip_of_the_spear->up() )
    {
      p()->buffs.tip_of_the_spear->decrement();
      p()->buffs.tip_of_the_spear_explosive->trigger();
    }
  }
};

struct explosive_shot_background_t : public explosive_shot_base_t
{
  explosive_shot_background_t( util::string_view n, hunter_t* p )
    : explosive_shot_base_t( n, p, p->talents.explosive_shot_cast )
  {
    background = dual = true;
    base_costs[ RESOURCE_FOCUS ] = 0;
  }
};

// Kill Shot (Hunter Talent) ====================================================================

struct kill_shot_t : hunter_ranged_attack_t
{
  struct state_data_t
  {
    bool razor_fragments_up = false;

    friend void sc_format_to( const state_data_t& data, fmt::format_context::iterator out ) {
      fmt::format_to( out, "razor_fragments_up={:d}", data.razor_fragments_up );
    }
  };
  using state_t = hunter_action_state_t<state_data_t>;

  // Razor Fragments (Marksmanship Talent)
  struct razor_fragments_t : residual_bleed_base_t
  {
    double result_mod;

    razor_fragments_t( util::string_view n, hunter_t* p )
      : residual_bleed_base_t( n, p, p -> talents.razor_fragments_bleed )
    {
      result_mod = p -> talents.razor_fragments_buff -> effectN( 3 ).percent();
      aoe = as<int>( p -> talents.razor_fragments_buff -> effectN( 2 ).base_value() );
    }
  };

  double health_threshold_pct;
  serpent_sting_t* venoms_bite = nullptr;
  razor_fragments_t* razor_fragments = nullptr;

  cooldown_t* se_recharge_cooldown = nullptr;

  kill_shot_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "kill_shot", p, p -> talents.kill_shot ),
    health_threshold_pct( p -> talents.kill_shot -> effectN( 2 ).base_value() )
  {
    parse_options( options_str );

    if ( p -> talents.razor_fragments.ok() )
    {
      razor_fragments = p -> get_background_action<razor_fragments_t>( "razor_fragments" );
      add_child( razor_fragments );
    }

    if ( p->specialization() == HUNTER_MARKSMANSHIP )
      se_recharge_cooldown = p->cooldowns.aimed_shot;

    if ( p->specialization() == HUNTER_BEAST_MASTERY )
      se_recharge_cooldown = p->cooldowns.barbed_shot;

    if ( p->talents.venoms_bite.ok() )
      venoms_bite = p->get_background_action<serpent_sting_t>( "serpent_sting" );
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> buffs.deathblow -> decrement();
    p() -> buffs.razor_fragments -> decrement();

    p() -> buffs.hunters_prey -> decrement();

    if ( p()->talents.shadow_erasure.ok() && td( target )->dots.black_arrow->is_ticking() &&
         rng().roll( p()->talents.shadow_erasure->proc_chance() ) )
      se_recharge_cooldown->reset( true );
    
    if ( venoms_bite )
      venoms_bite->execute_on_target( target );

    p()->buffs.sic_em->expire();
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( razor_fragments && debug_cast<state_t*>( s ) -> razor_fragments_up && s -> chain_target < 1 )
    {
      double amount = s -> result_amount * razor_fragments -> result_mod;
      if ( amount > 0 )
      {
        std::vector<player_t*>& tl = target_list();
        for ( player_t* t : util::make_span( tl ).first( std::min( tl.size(), size_t( razor_fragments -> aoe ) ) ) )
          residual_action::trigger( razor_fragments, t, amount );
      }
    }

    if ( p()->talents.cull_the_herd.ok() )
    {
      double amount = s -> result_amount * p()->talents.cull_the_herd->effectN( 1 ).percent();
      if ( amount > 0 )
        residual_action::trigger( p()->actions.cull_the_herd, s -> target, amount );
    }
  }

  int n_targets() const override
  {
    if ( p()->buffs.sic_em->up() )
      return as<int>( p()->buffs.sic_em->check_value() );

    return hunter_ranged_attack_t::n_targets();
  }

  bool target_ready( player_t* candidate_target ) override
  {
    return hunter_ranged_attack_t::target_ready( candidate_target ) &&
      ( candidate_target -> health_percentage() <= health_threshold_pct
        || p() -> buffs.deathblow -> check()
        || p() -> buffs.hunters_prey -> check()
        || p() -> buffs.sic_em -> check() );
  }

  double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    am *= 1 + p() -> buffs.razor_fragments -> check_value();

    return am;
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  void snapshot_state( action_state_t* s, result_amount_type type ) override
  {
    hunter_ranged_attack_t::snapshot_state( s, type );
    debug_cast<state_t*>( s ) -> razor_fragments_up = p() -> buffs.razor_fragments -> check();
  }
};

// Black Arrow (Dark Ranger) =========================================================

struct black_arrow_t : public hunter_ranged_attack_t
{
  double tick_recharge_chance = 0.0;
  cooldown_t* tick_recharge_cooldown = nullptr;

  struct
  {
    double chance = 0;
    timespan_t duration = 0_s;
  } shadow_hounds;

  buff_t* death_shade_cast_buff = nullptr;

  double dark_empowerment_gain = 0;

  struct
  {
    buff_t* cooldown_buff = nullptr;
    double tick_time_mult = 0;
  } shadow_lash;

  black_arrow_t( hunter_t* p, util::string_view options_str )
    : hunter_ranged_attack_t( "black_arrow", p, p->talents.black_arrow )
  {
    parse_options( options_str );

    tick_recharge_chance = data().effectN( 2 ).percent();

    if ( p->specialization() == HUNTER_MARKSMANSHIP )
    {
      tick_recharge_cooldown = p->cooldowns.aimed_shot;
      death_shade_cast_buff = p->buffs.deathblow;
      shadow_lash.cooldown_buff = p->buffs.trueshot;
    }

    if ( p->specialization() == HUNTER_BEAST_MASTERY )
    {
      tick_recharge_cooldown = p->cooldowns.barbed_shot;
      death_shade_cast_buff = p->buffs.hunters_prey;
      shadow_lash.cooldown_buff = p->buffs.call_of_the_wild;
    }

    if ( p->talents.shadow_hounds.ok() )
    {
      shadow_hounds.chance = p->talents.shadow_hounds->effectN( 1 ).percent();
      shadow_hounds.duration = p->find_spell( 442419 )->duration();
    }

    if ( p->talents.dark_empowerment.ok() )
      dark_empowerment_gain = p->find_spell( 442511 )->effectN( 1 ).base_value();

    if ( p->talents.shadow_lash.ok() )
      shadow_lash.tick_time_mult = p->find_spell( 444354 )->effectN( 1 ).percent();

    tick_zero = true;
  }

  double tick_time_pct_multiplier( const action_state_t* state ) const override
  {
    auto mul = hunter_ranged_attack_t::tick_time_pct_multiplier( state );

    if ( shadow_lash.tick_time_mult && shadow_lash.cooldown_buff->up() )
      mul *= 1.0 + shadow_lash.tick_time_mult;

    return mul;
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( p()->talents.death_shade.ok() )
      death_shade_cast_buff->trigger();
  }

  void tick( dot_t* d ) override
  {
    hunter_ranged_attack_t::tick( d );

    if ( rng().roll( tick_recharge_chance ) )
    {
      tick_recharge_cooldown->reset( true );
      p()->buffs.black_arrow->trigger();

      if ( dark_empowerment_gain )
        p()->resource_gain( RESOURCE_FOCUS, dark_empowerment_gain, p()->gains.dark_empowerment, this );

      if ( p()->actions.withering_fire )
      {
        p()->buffs.withering_fire->trigger();
        for ( int i = 0; i < 4; i++ )
          p()->actions.withering_fire->execute_on_target( d->target );
      }
    }

    if ( rng().roll( shadow_hounds.chance ) )
      p()->pets.dark_hound.spawn( shadow_hounds.duration );
  }
};

// Shadow Surge (Dark Ranger) =========================================================

struct shadow_surge_t final : hunter_ranged_attack_t
{
  shadow_surge_t( hunter_t* p ) : hunter_ranged_attack_t( "shadow_surge", p, p->talents.shadow_surge_dmg )
  {
    aoe = -1;
    background = dual = true;
  }
};

// Withering Fire (Dark Ranger) =========================================================

struct withering_fire_t final : hunter_ranged_attack_t
{
  withering_fire_t(hunter_t* p) : hunter_ranged_attack_t( "withering_fire", p, p->talents.withering_fire_dmg )
  {
    background = dual = true;
  }
};

// Vicious Hunt (Pack Leader) ============================================================

struct vicious_hunt_t final : hunter_ranged_attack_t
{
  vicious_hunt_t( hunter_t* p ) : hunter_ranged_attack_t( "vicious_hunt", p, p->find_spell( 445431 ) )
  {
    background = true;
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( p()->talents.pack_coordination.ok() && p()->pets.main )
      p()->pets.main->buffs.pack_coordination->trigger();
  }
};

// Cull The Herd (Pack Leader) ============================================================

struct cull_the_herd_t : residual_bleed_base_t
{
  cull_the_herd_t( hunter_t* p ):
    residual_bleed_base_t( "cull_the_herd", p, p -> find_spell( 449233 ) )
  { }
};

// Sentinel (Sentinel) ==================================================================

struct sentinel_t : hunter_ranged_attack_t
{
  struct {
    double chance = 0.0;
    double gain = 0.0;
  } invigorating_pulse;

  struct {
    timespan_t reduction = 0_s;
    timespan_t limit = 0_s;
    cooldown_t* cooldown = nullptr;
  } sentinel_watch;

  sentinel_t( hunter_t* p ) : hunter_ranged_attack_t( "sentinel", p, p->talents.sentinel_tick )
  {
    background = dual = true;

    if ( p->talents.invigorating_pulse.ok() )
    {
      invigorating_pulse.chance = p->talents.invigorating_pulse->effectN( 2 ).percent();
      invigorating_pulse.gain   = p->talents.invigorating_pulse->effectN( 1 ).base_value();
    }

    if ( p->talents.sentinel_watch.ok() )
    {
      sentinel_watch.reduction = timespan_t::from_seconds( p->talents.sentinel_watch->effectN( 1 ).base_value() );
      sentinel_watch.limit     = timespan_t::from_seconds( p->talents.sentinel_watch->effectN( 2 ).base_value() );

      if ( p->specialization() == HUNTER_SURVIVAL )
        sentinel_watch.cooldown = p->cooldowns.coordinated_assault;
      else if ( p->specialization() == HUNTER_MARKSMANSHIP )
        sentinel_watch.cooldown = p->cooldowns.trueshot;
    }
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    td( target )->debuffs.sentinel->decrement();
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    // TODO test for lower chance when multiple implosions are active
    if ( rng().roll( invigorating_pulse.chance ) )
      p()->resource_gain( RESOURCE_FOCUS, invigorating_pulse.gain, p()->gains.invigorating_pulse, this );

    if ( sentinel_watch.cooldown && p()->state.sentinel_watch_reduction < sentinel_watch.limit )
    {
      p()->state.sentinel_watch_reduction += sentinel_watch.reduction;
      sentinel_watch.cooldown->adjust( -sentinel_watch.reduction, true );
    }
  }
};

// Symphonic Arsenal (Sentinel) ============================================================

struct symphonic_arsenal_t : hunter_ranged_attack_t
{
  symphonic_arsenal_t( hunter_t* p ) : hunter_ranged_attack_t( "symphonic_arsenal", p, p->talents.symphonic_arsenal_dmg )
  {
    background = dual = true;
    aoe = as<int>( p->talents.symphonic_arsenal->effectN( 1 ).base_value() );
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    // Can still proc Sentinel on original target.
    p()->trigger_sentinel( target, false, p()->procs.sentinel_stacks );
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    hunter_ranged_attack_t::available_targets( tl );

    // Cannot hit the original target.
    range::erase_remove( tl, target );

    return tl.size();
  }
};

// Lunar Storm (Sentinel) ============================================================

struct lunar_storm_t : hunter_ranged_attack_t
{
  lunar_storm_t( hunter_t* p ) : hunter_ranged_attack_t( "lunar_storm_dmg", p, p->talents.lunar_storm_dmg )
  {
    background = dual = true;
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    td( s->target )->debuffs.lunar_storm->trigger();
  }

  std::vector<player_t*>& target_list() const override
  {
    target_cache.is_valid = false;
    return hunter_ranged_attack_t::target_list();
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    hunter_ranged_attack_t::available_targets( tl );

    range::erase_remove( tl, [ this ]( player_t* t ) { return !td( t )->debuffs.sentinel->check(); } );

    return tl.size();
  }
};

//==============================
// Beast Mastery attacks
//==============================

// Multi-Shot =================================================================

struct multishot_bm_t: public hunter_ranged_attack_t
{
  struct state_data_t
  {
    bool explosive_venom_ready = false;

    friend void sc_format_to( const state_data_t& data, fmt::format_context::iterator out ) {
      fmt::format_to( out, "explosive_venom_ready={}", data.explosive_venom_ready );
    }
  };
  using state_t = hunter_action_state_t<state_data_t>;

  serpent_sting_t* explosive_venom = nullptr;

  multishot_bm_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "multishot", p, p -> talents.multishot_bm )
  {
    parse_options( options_str );

    aoe = -1;
    reduced_aoe_targets = data().effectN( 1 ).base_value();

    if ( p -> talents.explosive_venom.ok() )
      explosive_venom = p->get_background_action<serpent_sting_t>( "serpent_sting" );
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( p()->talents.beast_cleave->ok() && p()->buffs.beast_cleave->buff_duration() > p()->buffs.beast_cleave->remains() ) {

      p() -> buffs.beast_cleave -> trigger();
      
      for ( auto pet : pets::active<pets::hunter_pet_t>( p() -> pets.main, p() -> pets.animal_companion ) )
        pet -> buffs.beast_cleave -> trigger();
    }

    if ( p() -> talents.explosive_venom.ok() ) 
    {
      p() -> buffs.explosive_venom -> up(); // Benefit tracking
      if( p() -> buffs.explosive_venom -> at_max_stacks() ) 
      {
        p() -> buffs.explosive_venom -> expire();
        p() -> buffs.explosive_venom -> increment();
      }
      else 
      {
        p() -> buffs.explosive_venom -> increment();
      }
    }

    if ( p() -> talents.scattered_prey.ok() ) 
    {
      if( p() -> buffs.scattered_prey -> up() ) 
      {
        p() -> buffs.scattered_prey -> decrement();
      }
      else
      {
        p() -> buffs.scattered_prey -> trigger();
      }
    }
  }

  void impact(action_state_t* s) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( p()->actions.shadow_surge && td( s->target )->dots.black_arrow->is_ticking() && p()->cooldowns.shadow_surge->up() )
    {
      p()->actions.shadow_surge->execute_on_target( s->target );
      p()->cooldowns.shadow_surge->start();
    }

    if ( explosive_venom && debug_cast<state_t*>( s )->explosive_venom_ready )
      explosive_venom->execute_on_target( s->target );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = hunter_ranged_attack_t::composite_da_multiplier( s );
    
    m *= 1.0 + p() -> buffs.scattered_prey -> value();

    return m;
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  void snapshot_state( action_state_t* s, result_amount_type type ) override
  {
    hunter_ranged_attack_t::snapshot_state( s, type );
    debug_cast<state_t*>( s ) -> explosive_venom_ready = p() -> buffs.explosive_venom -> at_max_stacks();
  }
};

// Cobra Shot =================================================================

struct cobra_shot_t: public hunter_ranged_attack_t
{
  const timespan_t kill_command_reduction;

  cobra_shot_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "cobra_shot", p, p -> talents.cobra_shot ),
    kill_command_reduction( -timespan_t::from_seconds( data().effectN( 3 ).base_value() ) + p -> talents.cobra_senses -> effectN( 1 ).time_value() )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( p() -> talents.killer_cobra.ok() && p() -> buffs.bestial_wrath -> check() )
      p() -> cooldowns.kill_command -> reset( true );
  }

  void schedule_travel( action_state_t* s ) override
  {
    hunter_ranged_attack_t::schedule_travel( s );

    p() -> cooldowns.kill_command -> adjust( kill_command_reduction );
  }
};

// Barbed Shot ===============================================================

struct barbed_shot_t: public hunter_ranged_attack_t
{
  timespan_t bestial_wrath_reduction;

  barbed_shot_t( hunter_t* p, util::string_view options_str ) :
    hunter_ranged_attack_t( "barbed_shot", p, p -> talents.barbed_shot )
  {
    parse_options(options_str);

    bestial_wrath_reduction = p -> talents.barbed_wrath -> effectN( 1 ).time_value();

    tick_zero = true; 

    p -> actions.barbed_shot = this;
  }

  void init_finished() override
  {
    pet_t* pets[] = { p() -> find_pet( p() -> options.summon_pet_str ), p() -> pets.animal_companion };
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
      ( *it ) -> trigger();
    else if ( sim -> debug )
      sim -> out_debug.print( "{} {} unable to trigger excess Barbed Shot buff", player -> name(), name() );

    p() -> buffs.thrill_of_the_hunt -> trigger();

    p() -> cooldowns.bestial_wrath -> adjust( -bestial_wrath_reduction );

    if ( rng().roll( p() -> talents.war_orders -> effectN( 3 ).percent() ) )
      p() -> cooldowns.kill_command -> reset( true );

    for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p() -> pets.main, p() -> pets.animal_companion ) )
    {
      if ( p() -> talents.stomp.ok() )
        pet -> active.stomp -> execute();

      pet -> buffs.frenzy -> trigger();
      pet -> buffs.thrill_of_the_hunt -> trigger();
    }

    auto pet = p() -> pets.main;
    if ( pet && pet -> buffs.frenzy -> check() == as<int>( p() -> talents.brutal_companion -> effectN( 1 ).base_value() ) )
    {
      pet -> active.brutal_companion_ba -> execute_on_target( target );
    }

    if( p()->talents.pack_coordination.ok() && p()->pets.main->buffs.pack_coordination->check() )
    {
      p()->pets.main->active.pack_coordination_ba->execute_on_target( target );
      if( !p()->talents.pack_assault.ok() || !p()->buffs.call_of_the_wild->check() )
        p()->pets.main->buffs.pack_coordination->decrement();
    }

    if( p()->buffs.furious_assault->check() )
    {
      p()->buffs.furious_assault->decrement();
    }
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = hunter_ranged_attack_t::composite_ta_multiplier( s );

    m *= 1 + p()->buffs.furious_assault->value();

    return m;
  }

  void tick( dot_t* d ) override
  {
    hunter_ranged_attack_t::tick( d );

    if( p() -> talents.master_handler -> ok() )
    {
      p() -> cooldowns.kill_command -> adjust( -p() -> talents.master_handler -> effectN( 1 ).time_value() );
    }
  }
};

//==============================
// Marksmanship attacks
//==============================

// Master Marksman ====================================================================

struct master_marksman_t : residual_bleed_base_t
{
  master_marksman_t( hunter_t* p ) : residual_bleed_base_t( "master_marksman", p, p->talents.master_marksman_bleed )
  {
  }
};

// Chimaera Shot ======================================================================

struct chimaera_shot_base_t : public hunter_ranged_attack_t
{
  struct impact_t final : public hunter_ranged_attack_t
  {
    impact_t( util::string_view n, hunter_t* p, const spell_data_t* s, double effectiveness = 1 ) :
      hunter_ranged_attack_t( n, p, s )
    {
      background = dual = true;

      // "Secondary" frost damage multiplier
      if ( s -> id() == 344121 )
        base_multiplier *= p -> talents.chimaera_shot -> effectN( 1 ).percent();

      base_multiplier *= effectiveness;
    }
  };

  impact_t* nature = nullptr;
  impact_t* frost = nullptr;

  chimaera_shot_base_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p -> talents.chimaera_shot )
  {
    aoe = 2;
    radius = 5;
    school = SCHOOL_FROSTSTRIKE; // Just so the report shows a mixture of the two colors.
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if( p()->tier_set.tww_s1_mm_4pc.ok() && p()->buffs.precise_shots->check() )
    {
      p()->buffs.moving_target->trigger();
    }

    p() -> buffs.precise_shots -> up(); // Benefit tracking
    p() -> buffs.precise_shots -> decrement();
  }

  void schedule_travel( action_state_t* s ) override
  {
    impact_t* damage = ( s -> chain_target == 0 ) ? nature : frost;
    damage -> execute_on_target( s -> target );
    action_state_t::release( s );
  }

  // Don't bother, the results will be discarded anyway.
  result_e calculate_result( action_state_t* ) const override { return RESULT_NONE; }
  double calculate_direct_amount( action_state_t* ) const override { return 0.0; }
};

struct chimaera_shot_t : public chimaera_shot_base_t
{
  struct chimaera_shot_eagletalons_true_focus_t : public chimaera_shot_base_t
  {
    chimaera_shot_eagletalons_true_focus_t( util::string_view n, hunter_t* p ) : chimaera_shot_base_t( n, p )
    {
      background = dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;

      nature = p->get_background_action<impact_t>( "chimaera_shot_eagletalons_true_focus_nature",
                                                   p->talents.chimaera_shot_nature,
                                                   p->talents.eagletalons_true_focus->effectN( 3 ).percent() );

      frost  = p->get_background_action<impact_t>( "chimaera_shot_eagletalons_true_focus_frost",
                                                  p->talents.chimaera_shot_frost,
                                                  p->talents.eagletalons_true_focus->effectN( 3 ).percent() );

      add_child( nature );
      add_child( frost );
    }
  };

  chimaera_shot_eagletalons_true_focus_t* eagletalons_true_focus = nullptr;

  chimaera_shot_t( hunter_t* p, util::string_view options_str ) : chimaera_shot_base_t( "chimaera_shot", p )
  {
    parse_options( options_str );

    nature = p->get_background_action<impact_t>( "chimaera_shot_nature", p->talents.chimaera_shot_nature );
    frost = p->get_background_action<impact_t>( "chimaera_shot_frost", p->talents.chimaera_shot_frost );

    add_child( nature );
    add_child( frost );

    if ( p->talents.eagletalons_true_focus.ok() )
    {
      eagletalons_true_focus = p->get_background_action<chimaera_shot_eagletalons_true_focus_t>( "chimaera_shot_eagletalons_true_focus" );
      add_child( eagletalons_true_focus );
    }
  }

  void execute() override
  {
    chimaera_shot_base_t::execute();

    if ( eagletalons_true_focus && p()->buffs.eagletalons_true_focus->up() )
      eagletalons_true_focus->execute_on_target( target );
  }
};

// Bursting Shot ======================================================================

struct bursting_shot_t : public hunter_ranged_attack_t
{
  bursting_shot_t( hunter_t* p, util::string_view options_str ) :
    hunter_ranged_attack_t( "bursting_shot", p, p -> talents.bursting_shot )
  {
    parse_options( options_str );
  }
};

// Aimed Shot =========================================================================

struct aimed_shot_base_t : public hunter_ranged_attack_t
{
  struct serpent_sting_hydras_bite_t final : public serpent_sting_t
  {
    serpent_sting_hydras_bite_t( util::string_view n, hunter_t* p ) : serpent_sting_t( n, p )
    {
      aoe = as<int>( p->talents.hydras_bite->effectN( 3 ).base_value() );
    }

    timespan_t travel_time() const override
    {
      return 0_s;
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      serpent_sting_t::available_targets( tl );

      if ( is_aoe() && tl.size() > 1 )
      {
        // Prefer targets without Serpent Sting ticking.
        auto start = tl.begin();
        std::partition( *start == target ? std::next( start ) : start, tl.end(),
                        [ this ]( player_t* t ) { return !( this->td( t )->dots.serpent_sting->is_ticking() ); } );

        // Remove the cast target if it is already ticking, otherwise it remains as the first target.
        if ( p()->get_target_data( target )->dots.serpent_sting->is_ticking() )
          range::erase_remove( tl, target );
      }

      return tl.size();
    }
  };

  const int trick_shots_targets;

  struct {
    double multiplier = 0;
    double high, low;
  } careful_aim;

  bool lock_and_loaded = false;
  
  struct {
    double chance = 0;
    proc_t* proc;
  } surging_shots;
  
  struct {
    int count = 0;
    wind_arrow_t* wind_arrow = nullptr;
  } legacy_of_the_windrunners;

  serpent_sting_t* serpentstalkers_trickery = nullptr;
  serpent_sting_hydras_bite_t* hydras_bite = nullptr;

  aimed_shot_base_t( util::string_view n, hunter_t* p, spell_data_ptr_t s ) :
    hunter_ranged_attack_t( n, p, s ),
    trick_shots_targets( as<int>( p->talents.trick_shots_data->effectN( 1 ).base_value()
      + p -> talents.light_ammo -> effectN( 1 ).base_value()
      + p -> talents.heavy_ammo -> effectN( 1 ).base_value() ) )
  {
    radius = 8;
    base_aoe_multiplier = p->talents.trick_shots_data->effectN( 4 ).percent() + p->talents.heavy_ammo->effectN( 3 ).percent();

    triggers_calling_the_shots = false;

    if ( p -> talents.careful_aim.ok() )
    {
      careful_aim.high = p -> talents.careful_aim -> effectN( 1 ).base_value();
      careful_aim.low = p -> talents.careful_aim -> effectN( 2 ).base_value();
      careful_aim.multiplier = p -> talents.careful_aim -> effectN( 3 ).percent();
    }

    if ( p -> talents.serpentstalkers_trickery.ok() )
      serpentstalkers_trickery = p->get_background_action<serpent_sting_t>( "serpent_sting" );

    if ( p -> talents.surging_shots.ok() )
    {
      surging_shots.chance = p -> talents.surging_shots -> proc_chance();
      surging_shots.proc = p -> get_proc( "Surging Shots Rapid Fire reset" );
    }

    if ( p -> talents.legacy_of_the_windrunners.ok() )
    {
      legacy_of_the_windrunners.count = as<int>( p->talents.legacy_of_the_windrunners->effectN( 1 ).base_value() );
      legacy_of_the_windrunners.wind_arrow = p->get_background_action<wind_arrow_t>( "wind_arrow" );
    }

    if ( p->talents.hydras_bite.ok() )
      hydras_bite = p->get_background_action<serpent_sting_hydras_bite_t>( "serpent_sting_hydras_bite" );
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = hunter_ranged_attack_t::composite_target_da_multiplier( t );

    if ( careful_aim.multiplier )
    {
      const double target_health_pct = t -> health_percentage();
      if ( target_health_pct > careful_aim.high || target_health_pct < careful_aim.low )
        m *= 1 + careful_aim.multiplier;
    }

    return m;
  }

  double cost() const override
  {
    const bool casting = p() -> executing && p() -> executing == this;
    if ( casting ? lock_and_loaded : p() -> buffs.lock_and_load -> check() )
      return 0;

    return hunter_ranged_attack_t::cost();
  }

  double cost_pct_multiplier() const override
  {
    double c = hunter_ranged_attack_t::cost_pct_multiplier();

    if ( p() -> buffs.eagletalons_true_focus -> check() )
      c *= 1 + p() -> talents.eagletalons_true_focus -> effectN( 1 ).percent();

    return c;
  }

  void schedule_execute( action_state_t* s ) override
  {
    lock_and_loaded = p() -> buffs.lock_and_load -> up();

    hunter_ranged_attack_t::schedule_execute( s );
  }

  void execute() override
  {
    p() -> trigger_calling_the_shots( this, cost() );

    if ( is_aoe() )
      target_cache.is_valid = false;

    hunter_ranged_attack_t::execute();

    if ( p() -> talents.bulletstorm -> ok() && execute_state && execute_state -> chain_target > 0 ) {
      p() -> buffs.bulletstorm -> increment( execute_state -> chain_target );
    }

    if ( serpentstalkers_trickery )
      serpentstalkers_trickery->execute_on_target( target );

    p() -> buffs.precise_shots -> trigger();
    p() -> buffs.deathblow -> trigger( -1, buff_t::DEFAULT_VALUE(), p() -> talents.deathblow -> proc_chance() );

    // TODO: Wailing Arrow consumes Trick Shots despite gaining nothing from it,
    // but check for it here if that's ever fixed.
    p()->buffs.trick_shots->up(); // Benefit tracking
    p()->consume_trick_shots();

    // XXX: 2020-10-22 Lock and Load completely supresses consumption of Streamline
    if ( ! p() -> buffs.lock_and_load -> check() )
      p() -> buffs.streamline -> decrement();

    if ( lock_and_loaded )
      p() -> buffs.lock_and_load -> decrement();
    lock_and_loaded = false;

    if ( rng().roll( surging_shots.chance ) )
    {
      surging_shots.proc -> occur();
      p() -> cooldowns.rapid_fire -> reset( true );
    }

    if ( legacy_of_the_windrunners.count )
    {
      int count = legacy_of_the_windrunners.count;
      if ( p()->buffs.trueshot->check() )
        count += as<int>( p()->talents.readiness->effectN( 3 ).base_value() );
      for ( int i = 0; i < count; i++ )
        legacy_of_the_windrunners.wind_arrow->execute_on_target( target );
    }

    p()->buffs.black_arrow->decrement();
  }

  int n_targets() const override
  {
    if ( p()->buffs.trick_shots->check() )
      return 1 + trick_shots_targets;
    return hunter_ranged_attack_t::n_targets();
  }

  double execute_time_pct_multiplier() const override
  {
    if ( p() -> buffs.lock_and_load -> check() )
      return 0;

    auto et = hunter_ranged_attack_t::execute_time_pct_multiplier();

    if ( p() -> buffs.streamline -> check() )
      et *= 1 + p() -> buffs.streamline -> check_value();

    if ( p() -> buffs.trueshot -> check() )
      et *= 1 + p() -> buffs.trueshot -> check_value();

    et *= 1 + p()->buffs.black_arrow->value();

    return et;
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( s -> chain_target == 0 ) {
      if ( hydras_bite )
        hydras_bite->execute_on_target( s->target );
    }
  }

  double recharge_rate_multiplier( const cooldown_t& cd ) const override
  {
    double m = hunter_ranged_attack_t::recharge_rate_multiplier( cd );

    // XXX [8.1]: Spell Data indicates that it's reducing Aimed Shot recharge rate by 225% (12s/3.25 = 3.69s)
    // m /= 1 + .6;  // The information from the bluepost
    // m /= 1 + 2.25; // The bugged (in spelldata) value for Aimed Shot.
    if ( p() -> buffs.trueshot -> check() )
      m /= 1 + p() -> talents.trueshot -> effectN( 3 ).percent();

    return m;
  }

  bool usable_moving() const override
  {
    return false;
  }
};

struct aimed_shot_t : public aimed_shot_base_t
{
  aimed_shot_t( hunter_t* p, util::string_view options_str ) : 
    aimed_shot_base_t( "aimed_shot", p, p->talents.aimed_shot )
  {
    parse_options( options_str );
  }

  bool ready() override
  {
    return !p()->buffs.wailing_arrow_override->check() && aimed_shot_base_t::ready();
  }
};

// Wailing Arrow =====================================================================

struct wailing_arrow_t : public aimed_shot_base_t
{
  struct splash_damage_t final : hunter_ranged_attack_t
  {
    splash_damage_t( util::string_view n, hunter_t* p )
      : hunter_ranged_attack_t( n, p, p->talents.wailing_arrow_damage )
    {
      aoe = -1;
      attack_power_mod.direct = data().effectN( 2 ).ap_coeff();
      dual = true;
    }
  };

  struct primary_damage_t final : hunter_ranged_attack_t
  {
    primary_damage_t( util::string_view n, hunter_t* p )
      : hunter_ranged_attack_t( n, p, p->talents.wailing_arrow_damage )
    {
      attack_power_mod.direct = data().effectN( 1 ).ap_coeff();

      dual = true;
    }
  };

  primary_damage_t* primary_damage;
  splash_damage_t* splash_damage;

  wailing_arrow_t( hunter_t* p, util::string_view options_str )
    : aimed_shot_base_t( "wailing_arrow", p, p->talents.wailing_arrow_override )
  {
    parse_options( options_str );

    aoe = 1;

    primary_damage = p->get_background_action<primary_damage_t>( "wailing_arrow_primary" );
    primary_damage->stats = stats;
    primary_damage->base_aoe_multiplier = base_aoe_multiplier;

    splash_damage = p->get_background_action<splash_damage_t>( "wailing_arrow_splash" );
    add_child( splash_damage );

    stats->action_list.push_back( primary_damage );
  }

  void execute() override
  {
    aimed_shot_base_t::execute();

    if ( p()->talents.readiness.ok() )
    {
      p()->cooldowns.rapid_fire->reset( false );
      p()->cooldowns.aimed_shot->reset( p()->talents.readiness->effectN( 2 ).base_value() );
    }

    p()->buffs.wailing_arrow_override->expire();
  }

  void impact( action_state_t* state ) override
  {
    aimed_shot_base_t::impact( state );

    primary_damage->execute_on_target( state->target );
    splash_damage->execute_on_target( state->target );
  }

  int n_targets() const override
  {
    return hunter_ranged_attack_t::n_targets();
  }

  bool ready() override
  {
    return p()->buffs.wailing_arrow_override->check() && aimed_shot_base_t::ready();
  }

  result_e calculate_result( action_state_t* ) const override
  {
    return RESULT_NONE;
  }

  double calculate_direct_amount( action_state_t* ) const override
  {
    return 0.0;
  }
};

// Steady Shot ========================================================================

struct steady_shot_t: public hunter_ranged_attack_t
{
  steady_shot_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "steady_shot", p, p -> specs.steady_shot )
  {
    parse_options( options_str );

    breaks_steady_focus = false;

    if ( p -> talents.improved_steady_shot.ok() )
    {
      energize_type = action_energize::ON_CAST;
      energize_resource = RESOURCE_FOCUS;
      energize_amount = p -> talents.improved_steady_shot -> effectN( 1 ).base_value();
    }
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> state.steady_focus_counter += 1;
    if ( p() -> state.steady_focus_counter == 2 )
    {
      p() -> buffs.steady_focus -> trigger();
      p() -> state.steady_focus_counter = 0;
    }
  }
};

// Rapid Fire =========================================================================

struct rapid_fire_t: public hunter_spell_t
{
  rapid_fire_tick_t* damage;
  int base_num_ticks;

  rapid_fire_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "rapid_fire", p, p -> talents.rapid_fire ),
    damage( p -> get_background_action<rapid_fire_tick_t>( "rapid_fire_tick" ) ),
    base_num_ticks( as<int>( data().effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );

    may_miss = may_crit = false;
    channeled = reset_auto_attack = true;

    base_num_ticks += p -> talents.fan_the_hammer.ok() ? as<int>( p -> talents.fan_the_hammer -> effectN( 2 ).base_value() ) : 0;
  }

  void init() override
  {
    hunter_spell_t::init();

    damage -> gain  = gain;
    damage -> stats = stats;
    stats -> action_list.push_back( damage );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.streamline -> trigger();
    p() -> buffs.deathblow -> trigger( -1, buff_t::DEFAULT_VALUE(), p() -> talents.deathblow -> effectN( 1 ).percent() );
  }

  void tick( dot_t* d ) override
  {
    hunter_spell_t::tick( d );

    damage -> parent_dot = d; // BfA Surging Shots shenanigans
    damage -> execute_on_target( d -> target );

    if ( p() -> talents.bulletstorm -> ok() && d -> current_tick == 1 && damage -> execute_state && damage -> execute_state -> chain_target > 0 )
      p() -> buffs.bulletstorm -> increment( damage -> execute_state -> chain_target );
  }

  void last_tick( dot_t* d ) override
  {
    hunter_spell_t::last_tick( d );

    p() -> consume_trick_shots();

    //2024-07-16: When talented into Fan The Hammer, In The Rhythm will trigger on the old last tick (7th instead of 10th).
    p() -> buffs.in_the_rhythm -> trigger();
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    // substract 1 here because RF has a tick at zero
    timespan_t base_duration = ( base_num_ticks - 1 ) * tick_time( s ); 
    
    return base_duration; 
  }

  double energize_cast_regen( const action_state_t* ) const override
  {
    // XXX: Not exactly true for Nesingwary's / Trueshot because the buff can fall off mid-channel. Meh
    return base_num_ticks * damage -> composite_energize_amount( nullptr );
  }

  double recharge_rate_multiplier( const cooldown_t& cd ) const override
  {
    double m = hunter_spell_t::recharge_rate_multiplier( cd );

    // XXX [8.1]: Spell Data indicates that it's reducing Rapid Fire by 240% (20s/3.4 = 5.88s)
    // m /= 1 + .6;  // The information from the bluepost
    // m /= 1 + 2.4; // The bugged (in spelldata) value for Rapid Fire.
    if ( p() -> buffs.trueshot -> check() )
      m /= 1 + p() -> talents.trueshot -> effectN( 1 ).percent();

    return m;
  }
};

// Multi-Shot =================================================================

struct multishot_mm_base_t: public hunter_ranged_attack_t
{
  struct salvo {
    explosive_shot_background_t* explosive = nullptr;
    int targets = 0;
  } salvo;

  multishot_mm_base_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p -> talents.multishot_mm )
  {
    aoe = -1;
    reduced_aoe_targets = p -> find_spell( 2643 ) -> effectN( 1 ).base_value();

    if ( p->talents.salvo.ok() )
    {
      salvo.targets = as<int>( p->talents.salvo->effectN( 1 ).base_value() );
      salvo.explosive = p->get_background_action<explosive_shot_background_t>( "explosive_shot" );
    }
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if( p()->tier_set.tww_s1_mm_4pc.ok() && p()->buffs.precise_shots->check() )
    {
      p()->buffs.moving_target->trigger();
    }

    p() -> buffs.precise_shots -> up(); // Benefit tracking
    p() -> buffs.precise_shots -> decrement();

    if ( ( p() -> talents.trick_shots.ok() && num_targets_hit >= p() -> talents.trick_shots -> effectN( 2 ).base_value() ) )
      p() -> buffs.trick_shots -> trigger();

    p()->buffs.salvo->expire();
  }

  void schedule_travel( action_state_t* s ) override
  {
    hunter_ranged_attack_t::schedule_travel( s );

    if ( s->chain_target < salvo.targets && p()->buffs.salvo->check() )
      salvo.explosive->execute_on_target( s->target );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = hunter_ranged_attack_t::composite_da_multiplier( s );

    m *= 1.0 + p() -> buffs.bulletstorm -> check_stack_value();

    return m;
  }

  void impact( action_state_t* state ) override
  {
    hunter_ranged_attack_t::impact( state );

    if ( p()->actions.shadow_surge && td( state->target )->dots.black_arrow->is_ticking() &&
         p()->cooldowns.shadow_surge->up() )
    {
      p()->actions.shadow_surge->execute_on_target( state->target );
      p()->cooldowns.shadow_surge->start();
    }
  }
};

struct multishot_mm_t : public multishot_mm_base_t
{
  struct multishot_mm_eagletalons_true_focus_t : public multishot_mm_base_t
  {
    multishot_mm_eagletalons_true_focus_t( util::string_view n, hunter_t* p ) : multishot_mm_base_t( n, p )
    {
      background = dual = true;
      base_multiplier *= p->talents.eagletalons_true_focus->effectN( 3 ).percent();
      base_costs[ RESOURCE_FOCUS ] = 0;
    }
  };

  multishot_mm_eagletalons_true_focus_t* eagletalons_true_focus = nullptr;

  multishot_mm_t( hunter_t* p, util::string_view options_str ) : multishot_mm_base_t( "multishot", p )
  {
    parse_options( options_str );

    if ( p->talents.eagletalons_true_focus.ok() )
    {
      eagletalons_true_focus = p->get_background_action<multishot_mm_eagletalons_true_focus_t>( "multishot_eagletalons_true_focus" );
      add_child( eagletalons_true_focus );
    }
  }

  void execute() override
  {
    multishot_mm_base_t::execute();

    p()->trigger_symphonic_arsenal();

    if ( eagletalons_true_focus && p()->buffs.eagletalons_true_focus->up() )
      eagletalons_true_focus->execute_on_target( target );
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
    weapon_multiplier  = 1;
    may_glance         = true;
    may_crit           = true;
  }

  void impact( action_state_t* s ) override
  {
    auto_attack_base_t::impact( s );

    p()->cooldowns.wildfire_bomb->adjust( -p()->talents.lunge->effectN( 2 ).time_value() );
  }
};

// Harpoon ==================================================================

struct harpoon_t : public hunter_melee_attack_t
{
  struct terms_of_engagement_t final : hunter_ranged_attack_t
  {
    terms_of_engagement_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->talents.terms_of_engagement_dmg )
    {
      background = dual = true;
    }

    void impact( action_state_t* s ) override
    {
      hunter_ranged_attack_t::impact( s );

      p()->buffs.terms_of_engagement->trigger();
    }
  };
  terms_of_engagement_t* terms_of_engagement = nullptr;

  harpoon_t( hunter_t* p, util::string_view options_str )
    : hunter_melee_attack_t( "harpoon", p, p->specs.harpoon )
  {
    parse_options( options_str );

    harmful = false;
    base_teleport_distance = data().max_range();
    movement_directionality = movement_direction_type::OMNI;

    if ( p->talents.terms_of_engagement.ok() )
    {
      terms_of_engagement = p->get_background_action<terms_of_engagement_t>( "harpoon_terms_of_engagement" );
      add_child( terms_of_engagement );
    }

    if ( p->main_hand_weapon.group() != WEAPON_2H )
      background = true;
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    if ( terms_of_engagement )
      terms_of_engagement->execute_on_target( target );
  }

  bool ready() override
  {
    // XXX: disable this for now to actually make it usable without explicit apl support for movement
    // if ( p() -> current.distance_to_move < data().min_range() )
    //  return false;

    return hunter_melee_attack_t::ready();
  }
};

// Raptor Strike/Mongoose Bite ================================================================

struct melee_focus_spender_t: hunter_melee_attack_t
{
  struct serpent_sting_vipers_venom_t final : public serpent_sting_t
  {
    serpent_sting_vipers_venom_t( util::string_view n, hunter_t* p ) : serpent_sting_t( n, p )
    {
    }

    int n_targets() const override
    {
      if ( p()->talents.contagious_reagents.ok() && p()->get_target_data( target )->dots.serpent_sting->is_ticking() )
        return 1 + as<int>( p()->talents.contagious_reagents->effectN( 1 ).base_value() );

      return serpent_sting_t::n_targets();
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      serpent_sting_t::available_targets( tl );

      if ( is_aoe() && tl.size() > 1 )
      {
        // Prefer targets without Serpent Sting ticking.
        auto start = tl.begin();
        std::partition( *start == target ? std::next( start ) : start, tl.end(),
                        [ this ]( player_t* t ) { return !( this->td( t )->dots.serpent_sting->is_ticking() ); } );
      }

      return tl.size();
    }
  };

  serpent_sting_vipers_venom_t* vipers_venom = nullptr;
  
  struct {
    double chance = 0;
    proc_t* proc;
  } rylakstalkers_strikes;

  double wildfire_infusion_chance = 0;

  melee_focus_spender_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
    hunter_melee_attack_t( n, p, s )
  {
    if ( p -> talents.vipers_venom.ok() )
      vipers_venom = p->get_background_action<serpent_sting_vipers_venom_t>( "serpent_sting_vipers_venom" );

    wildfire_infusion_chance = p->talents.wildfire_infusion->effectN( 1 ).percent();
  }

  double cost() const override
  {
    double cost = hunter_melee_attack_t::cost();

    if( p()->buffs.furious_assault->up() )
      cost *= 1 + p()->buffs.furious_assault->data().effectN( 3 ).percent();

    return cost; 
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = hunter_melee_attack_t::composite_da_multiplier( s );

    m *= 1 + p()->buffs.furious_assault->value();

    return m;
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = hunter_melee_attack_t::composite_target_da_multiplier( t );

    hunter_td_t *td = p()->get_target_data( t );
    if ( p()->tier_set.tww_s1_sv_4pc.ok() && td->dots.wildfire_bomb->is_ticking() )
      m *= 1 + p()->tier_set.tww_s1_sv_4pc->effectN( 1 ).percent();

    return m;
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    if ( rng().roll( rylakstalkers_strikes.chance ) )
    {
      p() -> cooldowns.wildfire_bomb -> reset( true );
      rylakstalkers_strikes.proc -> occur();
    }

    if ( rng().roll( wildfire_infusion_chance ) )
      p()->cooldowns.kill_command->reset( true );

    p()->buffs.merciless_blows->expire();

    if( p()->talents.pack_coordination.ok() && p()->pets.main->buffs.pack_coordination->check() )
    {
      p()->pets.main->active.pack_coordination_ba->execute_on_target( target );
      if( !p()->talents.pack_assault.ok() || !p()->buffs.coordinated_assault->check() )
        p()->pets.main->buffs.pack_coordination->decrement();
    }

    if( p()->buffs.furious_assault->check() )
    {
      p()->buffs.furious_assault->decrement();
    }
  }

  void impact( action_state_t* s ) override
  {
    hunter_melee_attack_t::impact( s );

    if ( vipers_venom )
      vipers_venom->execute_on_target( s->target );
  }

  bool ready() override
  {
    const bool has_eagle = p() -> buffs.aspect_of_the_eagle -> check();
    return ( range > 10 ? has_eagle : !has_eagle ) && hunter_melee_attack_t::ready();
  }

  int n_targets() const override
  {
    if ( p()->buffs.merciless_blows->up() )
      return as<int>( p()->buffs.merciless_blows->check_value() );

    return hunter_melee_attack_t::n_targets();
  }
};

// Raptor Strike =====================================================================

struct raptor_strike_base_t : public melee_focus_spender_t
{
  raptor_strike_base_t( util::string_view n, hunter_t* p, spell_data_ptr_t s ) : melee_focus_spender_t( n, p, s )
  {
  }
};

struct raptor_strike_t : public raptor_strike_base_t
{
  raptor_strike_t( hunter_t* p, util::string_view options_str )
    : raptor_strike_base_t( "raptor_strike", p, p->talents.raptor_strike )
  {
    parse_options( options_str );
  }
};

struct raptor_strike_eagle_t : public raptor_strike_base_t
{
  raptor_strike_eagle_t( hunter_t* p, util::string_view options_str )
    : raptor_strike_base_t( "raptor_strike_eagle", p, p->talents.raptor_strike_eagle )
  {
    parse_options( options_str );
  }
};

// Mongoose Bite =======================================================================

struct mongoose_bite_base_t: melee_focus_spender_t
{
  struct {
    std::array<proc_t*, 7> at_fury;
  } stats_;

  mongoose_bite_base_t( util::string_view n, hunter_t* p, spell_data_ptr_t s ):
    melee_focus_spender_t( n, p, s )
  {
    for ( size_t i = 0; i < stats_.at_fury.size(); i++ )
      stats_.at_fury[ i ] = p -> get_proc( fmt::format( "bite_at_{}_fury", i ) );

    if( !p->talents.mongoose_bite.ok() )
      background = true;
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

    am *= 1 + p() -> buffs.mongoose_fury -> stack_value();

    return am;
  }
};

struct mongoose_bite_t : mongoose_bite_base_t
{
  mongoose_bite_t( hunter_t* p, util::string_view options_str ):
    mongoose_bite_base_t( "mongoose_bite", p, p -> talents.mongoose_bite )
  {
    parse_options( options_str );
  }
};

struct mongoose_bite_eagle_t : mongoose_bite_base_t
{
  mongoose_bite_eagle_t( hunter_t* p, util::string_view options_str ):
    mongoose_bite_base_t( "mongoose_bite_eagle", p, p -> talents.mongoose_bite_eagle )
  {
    parse_options( options_str );
  }
};

// Flanking Strike =====================================================================

struct flanking_strike_t: hunter_melee_attack_t
{
  struct damage_t final : hunter_melee_attack_t
  {
    damage_t( util::string_view n, hunter_t* p ):
      hunter_melee_attack_t( n, p, p -> talents.flanking_strike_player )
    {
      background = dual = true;

      // Decrement after player and pet damage.
      decrements_tip_of_the_spear = false;

      if ( p->talents.exposed_flank.ok() )
      {
        aoe = as<int>( p->talents.exposed_flank->effectN( 3 ).base_value() );
        base_aoe_multiplier = p->talents.exposed_flank->effectN( 1 ).percent();
      }
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double am = hunter_melee_attack_t::composite_da_multiplier( s );

      double bonus = p() -> cache.mastery() * p() -> mastery.spirit_bond -> effectN( affected_by.spirit_bond.direct ).mastery_value();
      // TODO implement range
      bonus *= 1 + p()->mastery.spirit_bond_buff->effectN( 1 ).percent();
      
      am *= 1 + bonus;

      return am;
    }
  };

  damage_t* damage;

  flanking_strike_t( hunter_t* p, util::string_view options_str ):
    hunter_melee_attack_t( "flanking_strike", p, p -> talents.flanking_strike ),
    damage( p -> get_background_action<damage_t>( "flanking_strike_player" ) )
  {
    parse_options( options_str );

    base_teleport_distance  = data().max_range();
    movement_directionality = movement_direction_type::OMNI;

    add_child( damage );

    // Decrement after player and pet damage.
    decrements_tip_of_the_spear = false;
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

    if ( p() -> main_hand_weapon.group() == WEAPON_2H )
      damage -> execute_on_target( target );

    if ( auto pet = p() -> pets.main )
      pet -> active.flanking_strike -> execute_on_target( target );

    p()->buffs.tip_of_the_spear->decrement();

    p()->buffs.tip_of_the_spear->trigger( as<int>( p()->talents.flanking_strike->effectN( 2 ).base_value() ) );

    p()->buffs.exposed_flank->trigger();
  }
};

// Butchery ==========================================================================

struct butchery_t : public hunter_melee_attack_t
{
  struct
  {
    timespan_t reduction = 0_s;
    int cap = 0;
  } frenzy_strikes;

  butchery_t( hunter_t* p, util::string_view options_str ):
    hunter_melee_attack_t( "butchery", p, p -> talents.butchery )
  {
    parse_options( options_str );

    aoe = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();

    if ( p->talents.frenzy_strikes.ok() )
    {
      frenzy_strikes.reduction = data().effectN( 2 ).time_value();
      frenzy_strikes.cap = as<int>( data().effectN( 3 ).base_value() );
    }
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    p()->trigger_symphonic_arsenal();

    if ( p()->talents.frenzy_strikes.ok() )
      p()->cooldowns.wildfire_bomb->adjust( -frenzy_strikes.reduction * std::min( num_targets_hit, frenzy_strikes.cap ) );

    p()->buffs.merciless_blows->trigger();

    if ( p() -> talents.scattered_prey.ok() ) 
    {
      if( p() -> buffs.scattered_prey -> up() ) 
      {
        p() -> buffs.scattered_prey -> decrement();
      }
      else
      {
        p() -> buffs.scattered_prey -> trigger();
      }
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = hunter_melee_attack_t::composite_da_multiplier( s );
    
    m *= 1.0 + p() -> buffs.scattered_prey -> value();

    return m;
  }
};

// Fury of the Eagle ==============================================================

struct fury_of_the_eagle_t : public hunter_melee_attack_t
{
  struct fury_of_the_eagle_tick_t: public hunter_melee_attack_t
  {
    double health_threshold = 0;
    double crit_chance_bonus = 0;
    timespan_t ruthless_marauder_adjust = 0_ms;

    fury_of_the_eagle_tick_t( util::string_view n, hunter_t* p ) : hunter_melee_attack_t( n, p, p->talents.fury_of_the_eagle->effectN( 1 ).trigger() )
    {
      aoe = -1;
      background = true;
      may_crit = true;
      radius = data().max_range();
      reduced_aoe_targets = p->talents.fury_of_the_eagle->effectN( 5 ).base_value();
      health_threshold = p -> talents.fury_of_the_eagle -> effectN( 4 ).base_value() + p -> talents.ruthless_marauder -> effectN( 1 ).base_value();
      crit_chance_bonus = p -> talents.fury_of_the_eagle -> effectN( 3 ).percent();

      // 2-12-22: Ruthless Marauder also adds to crit rate.
      if ( p -> bugs )
        crit_chance_bonus += p -> talents.ruthless_marauder -> effectN( 1 ).percent();

      if ( p -> talents.ruthless_marauder )
        ruthless_marauder_adjust = p -> talents.ruthless_marauder -> effectN( 3 ).time_value();

      // Fury of the Eagle ticks should not decrement Tip of the Spear on execute, but rather on the last tick.
      decrements_tip_of_the_spear = false;
    }

    double composite_target_crit_chance( player_t* target ) const override
    {
      double c = hunter_melee_attack_t::composite_target_crit_chance( target );

      if ( target -> health_percentage() < health_threshold )
        c += crit_chance_bonus;

      return c;
    }

    void impact( action_state_t* state ) override
    {
      hunter_melee_attack_t::impact( state );

      if ( ruthless_marauder_adjust > 0_ms && state -> result == RESULT_CRIT && p() -> cooldowns.ruthless_marauder -> is_ready() )
      {
        p() -> cooldowns.wildfire_bomb -> adjust( -ruthless_marauder_adjust );
        p() -> cooldowns.flanking_strike -> adjust( -ruthless_marauder_adjust );
        p() -> cooldowns.ruthless_marauder -> start();
      }
    }
  };

  fury_of_the_eagle_tick_t* fote_tick;

  fury_of_the_eagle_t( hunter_t* p, util::string_view options_str ):
    hunter_melee_attack_t( "fury_of_the_eagle", p, p -> talents.fury_of_the_eagle ),
      fote_tick( p->get_background_action<fury_of_the_eagle_tick_t>( "fury_of_the_eagle_damage" ) )
  {
    parse_options( options_str );

    channeled = true;
    tick_zero = true;

    add_child( fote_tick );

    decrements_tip_of_the_spear = false;
  }

  void tick( dot_t* dot ) override
  {
    hunter_melee_attack_t::tick( dot );

    fote_tick -> execute_on_target( dot -> target );
  }

  void last_tick( dot_t* dot ) override
  {
    hunter_melee_attack_t::last_tick( dot );

    if ( p()->talents.tip_of_the_spear.ok() )
      p()->buffs.tip_of_the_spear->decrement();
  }
};

// Coordinated Assault ==============================================================

struct coordinated_assault_t: public hunter_melee_attack_t
{
  struct damage_t final : hunter_melee_attack_t
  {
    damage_t( util::string_view n, hunter_t* p ):
      hunter_melee_attack_t( n, p, p -> talents.coordinated_assault_dmg )
    {
      background = dual = true;

      // Despite damage being affected, does not consume.
      decrements_tip_of_the_spear = false;
    }
  };
  damage_t* damage;

  coordinated_assault_t( hunter_t* p, util::string_view options_str ):
    hunter_melee_attack_t( "coordinated_assault", p, p -> talents.coordinated_assault )
  {
    parse_options( options_str );

    base_teleport_distance = data().max_range();
    movement_directionality = movement_direction_type::OMNI;

    damage = p -> get_background_action<damage_t>( "coordinated_assault_player" );
    add_child( damage );

    decrements_tip_of_the_spear = false;
  }

  void init_finished() override
  {
    for ( auto pet : p() -> pet_list )
      add_pet_stats( pet, { "coordinated_assault" } );

    hunter_melee_attack_t::init_finished();
  }

  void execute() override
  {
    // Can be affected by its own generated stacks.
    if ( p()->talents.symbiotic_adrenaline.ok() )
      p()->buffs.tip_of_the_spear->trigger( as<int>( p()->talents.symbiotic_adrenaline->effectN( 2 ).base_value() ) );

    hunter_melee_attack_t::execute();

    p()->state.sentinel_watch_reduction = 0_s;
    p()->buffs.eyes_closed->trigger();

    if ( p() -> main_hand_weapon.group() == WEAPON_2H )
      damage -> execute_on_target( target );

    p()->buffs.coordinated_assault->trigger();
    p()->buffs.relentless_primal_ferocity->trigger();

    if ( auto pet = p() -> pets.main )
      pet -> active.coordinated_assault -> execute_on_target( target );
    
    if ( p()->talents.bombardier.ok() )
      p()->cooldowns.wildfire_bomb->reset( false, as<int>( p()->talents.bombardier->effectN( 3 ).base_value() ) );

    if( p()->talents.beast_of_opportunity.ok() )
      p()->pets.boo_stable_pet.spawn( p()->buffs.beast_of_opportunity->buff_duration(), as<int>( p()->buffs.beast_of_opportunity->data().effectN( 1 ).base_value() ) );

    if( p()->talents.pack_assault.ok() )
      p()->buffs.vicious_hunt->trigger();
  }
};

} // end namespace attacks

// ==========================================================================
// Hunter Spells
// ==========================================================================

namespace spells
{

// Base Interrupt ===========================================================

struct interrupt_base_t: public hunter_spell_t
{
  interrupt_base_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
    hunter_spell_t( n, p, s )
  {
    may_miss = may_block = may_dodge = may_parry = false;
    is_interrupt = true;
  }

  void execute() override
  {
    hunter_spell_t::execute();
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !candidate_target -> debuffs.casting || !candidate_target -> debuffs.casting -> check() ) return false;
    return hunter_spell_t::target_ready( candidate_target );
  }
};

//==============================
// Shared spells
//==============================

// Summon Pet ===============================================================

struct summon_pet_t: public hunter_spell_t
{
  bool opt_disabled;
  pet_t* pet;

  summon_pet_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "summon_pet", p, p -> find_spell( 883 ) ),
    opt_disabled( false ), pet( nullptr )
  {
    add_option( opt_obsoleted( "name" ) );
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

    if ( !pet && p() -> specialization() != HUNTER_MARKSMANSHIP )
    {
      throw std::invalid_argument(fmt::format("Unable to find pet '{}' for summons.", p() -> options.summon_pet_str));
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

// High Explosive/Implosive Trap (Hunter Talent) ======================================================

struct explosive_trap_damage_t final : hunter_ranged_attack_t
{
  explosive_trap_damage_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->talents.explosive_trap_damage )
  {
    aoe = -1;
    attack_power_mod.direct = data().effectN( 2 ).ap_coeff();
  }
};

struct high_explosive_trap_t : public trap_base_t
{
  high_explosive_trap_t( hunter_t* p, util::string_view options_str )
    : trap_base_t( "high_explosive_trap", p, p -> talents.high_explosive_trap )
  {
    parse_options( options_str );

    impact_action = p->get_background_action<explosive_trap_damage_t>( "high_explosive_trap_damage" );
  }
};

struct implosive_trap_t : public trap_base_t
{
  implosive_trap_t( hunter_t* p, util::string_view options_str )
    : trap_base_t( "implosive_trap", p, p->talents.implosive_trap )
  {
    parse_options( options_str );

    impact_action = p->get_background_action<explosive_trap_damage_t>( "implosive_trap_damage" );
  }
};

// Counter Shot (Marksmanship/Beast Mastery Talent) ===========================================================

struct counter_shot_t: public interrupt_base_t
{
  counter_shot_t( hunter_t* p, util::string_view options_str ):
    interrupt_base_t( "counter_shot", p, p -> talents.counter_shot )
  {
    parse_options( options_str );
  }
};

// Kill Command (Beast Mastery/Survival Talent) =============================================================

struct kill_command_t: public hunter_spell_t
{
  struct arcane_shot_quick_shot_t final : public attacks::arcane_shot_base_t
  {
    arcane_shot_quick_shot_t( util::string_view n, hunter_t* p ) : arcane_shot_base_t( n, p )
    {
      background = dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
      base_dd_multiplier *= p->talents.quick_shot->effectN( 2 ).percent();

      // Don't consume a stack or buff the damage.
      affected_by.tip_of_the_spear.direct = 0;
      decrements_tip_of_the_spear = false;
    }
  };

  struct explosive_shot_quick_shot_t : public attacks::explosive_shot_background_t
  {
    explosive_shot_quick_shot_t( util::string_view n, hunter_t* p ) : explosive_shot_background_t( n, p )
    {
    }

    void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
    {
      explosive_shot_background_t::snapshot_internal( s, flags, rt );

      if ( flags & STATE_EFFECTIVENESS )
        debug_cast<state_t*>( s )->effectiveness = p()->talents.sulfur_lined_pockets->effectN( 2 ).percent();
    }
  };

  struct {
    double chance = 0;
    proc_t* proc = nullptr;
  } reset;

  struct {
    double chance = 0;
    arcane_shot_quick_shot_t* arcane_shot = nullptr;
    explosive_shot_quick_shot_t* explosive_shot = nullptr;
  } quick_shot;

  struct {
    double chance = 0;
    proc_t* proc;
  } dire_command;

  timespan_t wildfire_infusion_reduction = 0_s;
  timespan_t bloody_claws_extension = 0_s;
  double sic_em_chance = 0;

  kill_command_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "kill_command", p, p -> talents.kill_command )
  {
    parse_options( options_str );

    cooldown -> charges += as<int>( p -> talents.alpha_predator -> effectN( 1 ).base_value() );

    if ( p -> specialization() == HUNTER_SURVIVAL )
    {
      reset.chance = data().effectN( 2 ).percent() + p -> talents.flankers_advantage -> effectN( 1 ).percent();
      reset.proc = p -> get_proc( "Kill Command Reset" );

      if ( p -> talents.quick_shot.ok() )
      {
        quick_shot.chance = p -> talents.quick_shot -> effectN( 1 ).percent();
        quick_shot.arcane_shot = p->get_background_action<arcane_shot_quick_shot_t>( "arcane_shot_quick_shot" );
        add_child( quick_shot.arcane_shot );

        if ( p->talents.sulfur_lined_pockets.ok() )
          quick_shot.explosive_shot = p->get_background_action<explosive_shot_quick_shot_t>( "explosive_shot_quick_shot" );
      }

      wildfire_infusion_reduction = p->talents.wildfire_infusion->effectN( 2 ).time_value();
      bloody_claws_extension      = p->talents.bloody_claws->effectN( 2 ).time_value();
      sic_em_chance               = p->talents.sic_em->effectN( 1 ).percent();
    }

    if ( p -> talents.dire_command.ok() )
    {
      dire_command.chance = p -> talents.dire_command -> effectN( 1 ).percent();
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
      pet -> active.kill_command -> execute_on_target( target );

    p()->buffs.tip_of_the_spear->trigger( 1 + ( p()->buffs.relentless_primal_ferocity->check() ? as<int>( p()->talents.relentless_primal_ferocity_buff->effectN( 4 ).base_value() ) : 0 ) );

    if ( rng().roll( quick_shot.chance ) )
    {
      if ( p()->buffs.sulfur_lined_pockets_explosive->up() )
      {
        p()->buffs.sulfur_lined_pockets_explosive->expire();
        quick_shot.explosive_shot->execute_on_target( target );
      }
      else
        quick_shot.arcane_shot->execute_on_target( target );
    }

    if ( reset.chance != 0 )
    {
      double chance = reset.chance;

      chance += p() -> talents.bloody_claws -> effectN( 1 ).percent()
        * p() -> buffs.mongoose_fury -> check();

      chance += p()->buffs.coordinated_assault->check_value();

      if ( rng().roll( chance ) )
      {
        reset.proc -> occur();
        cooldown -> reset( true );

        p() -> cooldowns.fury_of_the_eagle -> adjust( - p() -> talents.fury_of_the_eagle -> effectN( 2 ).time_value() );
      }
    }

    if ( rng().roll( dire_command.chance ) )
    {
      p() -> actions.dire_command -> execute();
      p() -> procs.dire_command -> occur();
    }

    if ( rng().roll( p()->talents.hunters_prey->effectN( 1 ).percent() ) )
      p()->buffs.hunters_prey->trigger();

    if( p() -> talents.a_murder_of_crows.ok() )
    {
      p() -> buffs.a_murder_of_crows -> trigger();
      if ( p() -> buffs.a_murder_of_crows -> at_max_stacks() )
      {
        p() -> actions.a_murder_of_crows -> execute_on_target( target );
        p() -> buffs.a_murder_of_crows -> expire();
      }
    }

    p()->cooldowns.wildfire_bomb->adjust( -wildfire_infusion_reduction );
    p()->buffs.mongoose_fury->extend_duration( p(), bloody_claws_extension );

    if ( rng().roll( sic_em_chance ) )
    {
      p()->buffs.sic_em->trigger();
      p()->cooldowns.kill_shot->reset( true );
    }

    if( p()->talents.vicious_hunt.ok() )
    {
      if( p()->buffs.vicious_hunt->up() )
      {
        p()->actions.vicious_hunt->execute_on_target( target ); 
        if( !p()->talents.pack_assault.ok() || !p()->buffs.call_of_the_wild->check() )
          p()->buffs.vicious_hunt->decrement(); 
      }
      else
      {
        p()->buffs.vicious_hunt->trigger();
      }
    }

    if( p()->talents.covering_fire.ok() )
    {
      timespan_t duration = timespan_t::from_seconds( p()->talents.covering_fire->effectN( 1 ).base_value() );
      if( p()->buffs.beast_cleave->check() )
        p()->buffs.beast_cleave->extend_duration( p(), duration );

      for ( auto pet : pets::active<pets::hunter_pet_t>( p() -> pets.main, p() -> pets.animal_companion ) )
      {
        if ( pet->buffs.beast_cleave->check() ) 
          pet->buffs.beast_cleave->extend_duration( p(), duration );
      }
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

  std::unique_ptr<expr_t> create_expression(util::string_view expression_str) override
  {
    // this is somewhat unfortunate but we can't get at the pets dot in any other way
    auto splits = util::string_split<util::string_view>( expression_str, "." );
    if ( splits.size() == 2 && splits[ 0 ] == "bloodseeker" && splits[ 1 ] == "remains" )
    {
      if ( !p() -> talents.bloodseeker.ok() )
        return expr_t::create_constant( expression_str, 0_ms );

      return make_fn_expr( expression_str, [ this ] () {
          if ( auto pet = p() -> pets.main )
            return pet -> get_target_data( target ) -> dots.bloodseeker -> remains();
          return 0_ms;
        } );
    }

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

// Dire Beast ===============================================================

struct dire_beast_t: public hunter_spell_t
{
  dire_beast_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "dire_beast", p, p -> talents.dire_beast )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    timespan_t summon_duration;
    int base_attacks_per_summon;
    std::tie( summon_duration, base_attacks_per_summon ) = pets::dire_beast_duration( p() );

    sim -> print_debug( "Dire Beast summoned with {} autoattacks", base_attacks_per_summon );

    p() -> pets.dire_beast.spawn( summon_duration );
  }
};

// Dire Command =============================================================

struct dire_command_summon_t final : hunter_spell_t
{
  dire_command_summon_t( hunter_t* p ) : hunter_spell_t( "dire_command_summon", p, p -> find_spell( 219199 ) )
  {
    cooldown -> duration = 0_ms;
    track_cd_waste = false;
    background = true;
    harmful = false;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> pets.dire_beast.spawn( pets::dire_beast_duration( p() ).first );
  }
};

// Bestial Wrath ============================================================

struct bestial_wrath_t: public hunter_spell_t
{
  timespan_t precast_time = 0_ms;

  bestial_wrath_t( hunter_t* player, util::string_view options_str ):
    hunter_spell_t( "bestial_wrath", player, player -> talents.bestial_wrath )
  {
    add_option( opt_timespan( "precast_time", precast_time ) );
    parse_options( options_str );

    precast_time = clamp( precast_time, 0_ms, data().duration() );
  }

  bool usable_precombat() const override
  {
    return true;
  }

  void init_finished() override
  {
    for ( auto pet : p() -> pet_list )
      add_pet_stats( pet, { "bestial_wrath" } );

    hunter_spell_t::init_finished();
  }

  void execute() override
  {
    hunter_spell_t::execute();

    trigger_buff( p() -> buffs.bestial_wrath, precast_time );

    for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p() -> pets.main, p() -> pets.animal_companion ) )
    {
      // Assume the pet is out of range / not engaged when precasting.
      if ( !is_precombat )
        pet -> active.bestial_wrath -> execute_on_target( target );
      trigger_buff( pet -> buffs.bestial_wrath, precast_time );
    }

    adjust_precast_cooldown( precast_time );

    if ( p() -> talents.scent_of_blood.ok() )
      p() -> cooldowns.barbed_shot -> reset( true, as<int>( p() -> talents.scent_of_blood -> effectN( 1 ).base_value() ) );

    if( p()->talents.beast_of_opportunity.ok() )
      p()->pets.boo_stable_pet.spawn( p()->buffs.beast_of_opportunity->buff_duration(), as<int>( p()->buffs.beast_of_opportunity->data().effectN( 1 ).base_value() ) );
  }

  bool ready() override
  {
    if ( !p() -> pets.main )
      return false;

    return hunter_spell_t::ready();
  }
};

// Call of the Wild =======================================================

struct call_of_the_wild_t: public hunter_spell_t
{
  call_of_the_wild_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "call_of_the_wild", p, p -> talents.call_of_the_wild )
  {
    parse_options( options_str );

    harmful = false;
    // disable automatic generation of the dot from spell data
    dot_duration = 0_ms;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.call_of_the_wild -> trigger();
    p() -> pets.cotw_stable_pet.spawn( data().duration(), as<int>( data().effectN( 1 ).base_value() ) );

    double percent_reduction = p() -> talents.call_of_the_wild -> effectN( 3 ).base_value() / 100.0; 
    double on_cast_reduction = percent_reduction * as<int>( data().effectN( 1 ).base_value() );
    p() -> cooldowns.kill_command -> adjust( -( p() -> cooldowns.kill_command -> duration * on_cast_reduction ) );
    p() -> cooldowns.barbed_shot -> adjust( -( p() -> cooldowns.barbed_shot -> duration * on_cast_reduction ) );

    //2023-11-14 
    //When casting Call of the Wild with Bloody Frenzy talented it will apply beast cleave to the player, the main pet, AC pet and all call of the wild pets
    if ( p() -> talents.bloody_frenzy -> ok() )
    {
      timespan_t duration = p() -> buffs.call_of_the_wild -> remains();
      p() -> buffs.beast_cleave -> trigger( duration ); 
      for ( auto pet : pets::active<pets::hunter_pet_t>( p() -> pets.main, p() -> pets.animal_companion ) )
        pet -> buffs.beast_cleave -> trigger( duration );

      for ( auto pet : p() -> pets.cotw_stable_pet.active_pets() )
        pet -> hunter_pet_t::buffs.beast_cleave -> trigger( duration );
    }

    if( p()->talents.pack_assault.ok() )
      p()->buffs.vicious_hunt->trigger();
  }
};

// Bloodshed ================================================================

struct bloodshed_t : hunter_spell_t
{
  bloodshed_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "bloodshed", p, p -> talents.bloodshed )
  {
    parse_options( options_str );

    may_hit = false;
  }

  void init_finished() override
  {
    for ( auto pet : p() -> pet_list )
      add_pet_stats( pet, { "bloodshed" } );

    hunter_spell_t::init_finished();
  }

  void execute() override
  {
    hunter_spell_t::execute();

    if ( auto pet = p() -> pets.main )
    {
      pet -> active.bloodshed -> execute_on_target( target );
      if ( p() -> talents.venomous_bite.ok() ) 
        pet -> get_target_data( target ) -> debuffs.venomous_bite -> trigger();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    return p() -> pets.main &&
           p() -> pets.main -> active.bloodshed -> target_ready( candidate_target ) &&
           hunter_spell_t::target_ready( candidate_target );
  }

  bool ready() override
  {
    return p() -> pets.main &&
           p() -> pets.main -> active.bloodshed -> ready() &&
           hunter_spell_t::ready();
  }
};

// A Murder of Crows ========================================================

struct a_murder_of_crows_t : public hunter_spell_t
{
  struct peck_t final : public hunter_ranged_attack_t
  {
    peck_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p->find_spell( 131900 ) )
    {
      background = dual = true;
    }

    timespan_t travel_time() const override
    {
      return timespan_t::from_seconds( data().missile_speed() );
    }
  };

  a_murder_of_crows_t( hunter_t* p ) : hunter_spell_t( "a_murder_of_crows", p, p->find_spell( 131894 ) )
  {
    background = dual = true;
    tick_action = p->get_background_action<peck_t>( "crow_peck" );
  }

  // Spell data for A Murder of Crows still has it listed as costing focus
  double cost() const override
  {
    return 0;
  }
};

//==============================
// Marksmanship spells
//==============================

// Trueshot =================================================================

struct trueshot_t: public hunter_spell_t
{
  trueshot_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "trueshot", p, p -> talents.trueshot )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p()->state.sentinel_watch_reduction = 0_s;
    p()->buffs.eyes_closed->trigger();

    // Applying Trueshot directly does not extend an existing Trueshot and resets Unerring Vision stacks.
    p() -> buffs.trueshot -> expire();
    p() -> buffs.trueshot -> trigger();

    if ( p()->talents.readiness.ok() )
      p()->buffs.wailing_arrow_override->trigger();
  }
};

// Volley ===========================================================================

struct volley_base_t : public hunter_spell_t
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
        salvo.explosive = p -> get_background_action<attacks::explosive_shot_background_t>( "explosive_shot" );
      }
    }

    void execute() override
    {
      hunter_ranged_attack_t::execute();

      p()->buffs.salvo->expire();
    }

    void impact( action_state_t* s ) override
    {
      hunter_ranged_attack_t::impact( s );

      if ( s->chain_target < salvo.targets && p()->buffs.salvo->check() )
        salvo.explosive->execute_on_target( s->target );

      if ( p()->talents.kill_zone.ok() )
        p()->get_target_data( s->target )->debuffs.kill_zone->trigger();
    }
  };

  damage_t* damage;
  timespan_t tick_duration;

  volley_base_t( hunter_t* p, const spell_data_t* s = spell_data_t::nil() ) : hunter_spell_t( "volley", p, s ),
    damage( p->get_background_action<damage_t>( "volley_damage" ) ),
    tick_duration( data().duration() )
  {
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
                if ( p()->talents.kill_zone.ok() )
                  // Scheduled after next Volley tick.
                  make_event( *sim, 0_ms, [ this ]() { 
                    for ( player_t* t : sim->target_non_sleeping_list )
                      if ( t->is_enemy() )
                        p()->get_target_data( t )->debuffs.kill_zone->expire();
                  } );
                  
                break;
              }
              default:
                break;
            }
        } )
      );
  }
};

struct volley_t : public volley_base_t
{
  volley_t( hunter_t* p, util::string_view options_str ) : volley_base_t( p, p->talents.volley )
  {
    parse_options( options_str );
  }
};

// Salvo =====================================================================

struct salvo_t: public hunter_spell_t
{
  timespan_t precast_time = 0_ms;

  salvo_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "salvo", p, p -> talents.salvo )
  {
    add_option( opt_timespan( "precast_time", precast_time ) );
    parse_options( options_str );

    harmful = false;

    precast_time = clamp( precast_time, 0_ms, data().duration() );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    trigger_buff( p() -> buffs.salvo, precast_time );

    adjust_precast_cooldown( precast_time );
  }
};

//==============================
// Survival spells
//==============================

// Spearhead ==============================================================

struct spearhead_t : public hunter_spell_t
{
  spearhead_t( hunter_t* p, util::string_view options_str ) : 
    hunter_spell_t( "spearhead", p, p->talents.spearhead )
  {
    parse_options( options_str );

    decrements_tip_of_the_spear = false;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    if ( auto pet = p()->pets.main )
      pet->active.spearhead->execute_on_target( target );
  }
};

// Wildfire Bomb ==============================================================

struct wildfire_bomb_base_t: public hunter_spell_t
{
  struct bomb_damage_t : public hunter_spell_t
  {
    struct bomb_dot_t final : public hunter_spell_t
    {
      bomb_dot_t( util::string_view n, hunter_t* p ) :
        hunter_spell_t( n, p, p->talents.wildfire_bomb_dot )
      {
        background = dual = true;
      }

      double composite_ta_multiplier( const action_state_t* s ) const override
      {
        double am = hunter_spell_t::composite_ta_multiplier( s );

        auto td = p() -> find_target_data( s -> target ); 
        if( td )
        {
          am *= 1.0 + td -> debuffs.shredded_armor -> value();
        }

        if ( as<double>( s -> n_targets ) > reduced_aoe_targets )
        {
          am *= std::sqrt( reduced_aoe_targets / s -> n_targets );
        }

        if ( s -> chain_target == 0 )
        {
          am *= 1.0 + p() -> talents.wildfire_bomb -> effectN( 3 ).percent();
        }

        return am;
      }
    };

    struct explosive_shot_grenade_juggler_t : public attacks::explosive_shot_background_t
    {
      explosive_shot_grenade_juggler_t( util::string_view n, hunter_t* p ) : explosive_shot_background_t( n, p )
      {
      }

      void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
      {
        explosive_shot_background_t::snapshot_internal( s, flags, rt );

        if ( flags & STATE_EFFECTIVENESS )
          debug_cast<state_t*>( s )->effectiveness = p()->talents.grenade_juggler->effectN( 5 ).percent();
      }
    };

    bomb_dot_t* bomb_dot;

    struct {
      double chance = 0;
      explosive_shot_grenade_juggler_t* explosive_shot = nullptr;
    } grenade_juggler;

    bomb_damage_t( util::string_view n, hunter_t* p, wildfire_bomb_base_t* a ) : 
      hunter_spell_t( n, p, p->talents.wildfire_bomb_dmg ),
      bomb_dot( p->get_background_action<bomb_dot_t>( "wildfire_bomb_dot" ) )
    {
      background = dual = true;

      aoe = -1;
      reduced_aoe_targets = p -> talents.wildfire_bomb -> effectN( 2 ).base_value();
      radius = 5; // XXX: It's actually a circle + cone, but we sadly can't really model that

      bomb_dot->reduced_aoe_targets = reduced_aoe_targets;
      bomb_dot->aoe                 = aoe;
      bomb_dot->radius              = radius;

      a->add_child( this );
      a->add_child( bomb_dot );

      if ( p->talents.grenade_juggler.ok() )
      {
        grenade_juggler.chance = p->talents.grenade_juggler->effectN( 2 ).percent();
        grenade_juggler.explosive_shot = p->get_background_action<explosive_shot_grenade_juggler_t>( "explosive_shot_grenade_juggler" );
      }
    }

    void execute() override
    {
      hunter_spell_t::execute();

      if ( num_targets_hit > 0 )
      {
        // Dot applies to all of the same targets hit by the main explosion
        bomb_dot->target                    = target;
        bomb_dot->target_cache.list         = target_cache.list;
        bomb_dot->target_cache.is_valid     = true;
        bomb_dot->execute();
      }

      if ( rng().roll( grenade_juggler.chance ) )
        grenade_juggler.explosive_shot->execute_on_target( target );

      if ( p()->cooldowns.lunar_storm->up() )
        p()->trigger_lunar_storm( target );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double am = hunter_spell_t::composite_da_multiplier( s );

      auto td = p() -> find_target_data( s -> target ); 
      if( td )
      {
        am *= 1.0 + td -> debuffs.shredded_armor -> value();
      }

      if ( s -> chain_target == 0 )
      {
        am *= 1.0 + p() -> talents.wildfire_bomb -> effectN( 3 ).percent();
      }

      return am;
    }
  };

  wildfire_bomb_base_t( hunter_t* p, const spell_data_t* s = spell_data_t::nil() ) : hunter_spell_t( "wildfire_bomb", p, s )
  {
    may_miss = false;
    school = SCHOOL_FIRE; // for report coloring

    impact_action = p->get_background_action<bomb_damage_t>( "wildfire_bomb_damage", this );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    if ( p()->talents.covering_fire.ok() )
    {
      p()->cooldowns.butchery->adjust( -timespan_t::from_seconds( p()->talents.covering_fire->effectN( 2 ).base_value() ) );
    }
  }
};

struct wildfire_bomb_t: public wildfire_bomb_base_t
{
  wildfire_bomb_t( hunter_t* p, util::string_view options_str ) : wildfire_bomb_base_t( p, p->talents.wildfire_bomb )
  {
    parse_options( options_str );
  }
};

// Aspect of the Eagle ======================================================

struct aspect_of_the_eagle_t: public hunter_spell_t
{
  aspect_of_the_eagle_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "aspect_of_the_eagle", p, p -> find_spell( 186289 ) )
  {
    parse_options( options_str );

    harmful = false;
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
  muzzle_t( hunter_t* p, util::string_view options_str ):
    interrupt_base_t( "muzzle", p, p -> talents.muzzle )
  {
    parse_options( options_str );
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

} // end namespace actions

hunter_td_t::hunter_td_t( player_t* t, hunter_t* p ) : actor_target_data_t( t, p ),
  debuffs(),
  dots()
{
  debuffs.shredded_armor = 
    make_buff( *this, "shredded_armor", p -> find_spell( 410167 ) )
      -> set_default_value_from_effect( 1 );

  debuffs.wild_instincts = make_buff( *this, "wild_instincts", p -> find_spell( 424567 ) )
    -> set_default_value_from_effect( 1 );

  debuffs.basilisk_collar = make_buff( *this, "basilisk_collar", p -> find_spell( 459575 ) )
    -> set_default_value( p -> talents.basilisk_collar -> effectN( 1 ).base_value() )
    -> set_period( 0_s );

  debuffs.outland_venom = make_buff( *this, "outland_venom", p->talents.outland_venom_debuff )
    -> set_default_value( p->talents.outland_venom_debuff->effectN( 1 ).percent() )
    -> set_period( 0_s );

  debuffs.kill_zone = make_buff( *this, "kill_zone", p->talents.kill_zone_debuff )
    -> set_default_value_from_effect( 2 )
    -> set_schools_from_effect( 2 )
    -> set_chance( p->talents.kill_zone.ok() );

  debuffs.sentinel = make_buff( *this, "sentinel", p->talents.sentinel_debuff );

  debuffs.crescent_steel = make_buff( *this, "crescent_steel", p->talents.crescent_steel_debuff )
    -> set_tick_callback(
      [ this, p ]( buff_t* b, int, const timespan_t& ) {
        if ( crescent_steel_damaged )
        {
          crescent_steel_damaged = false;
          p->trigger_sentinel( target, true, p->procs.crescent_steel_stacks );
        }
        else
        {
          b->expire();
        }
      } );

  debuffs.lunar_storm = make_buff( *this, "lunar_storm", p->talents.lunar_storm_dmg->effectN( 2 ).trigger() )
    -> set_default_value_from_effect( 1 )
    -> set_schools_from_effect( 1 )
    -> set_chance( p->talents.lunar_storm.ok() );

  dots.serpent_sting = t -> get_dot( "serpent_sting", p );
  dots.a_murder_of_crows = t -> get_dot( "a_murder_of_crows", p );
  dots.wildfire_bomb = t -> get_dot( "wildfire_bomb_dot", p );
  dots.black_arrow = t -> get_dot( "black_arrow", p );
  dots.barbed_shot = t -> get_dot( "barbed_shot", p );
  dots.cull_the_herd = t -> get_dot( "cull_the_herd", p );
  dots.explosive_shot = t->get_dot( "explosive_shot", p );

  t -> register_on_demise_callback( p, [this](player_t*) { target_demise(); } );
}

void hunter_td_t::target_demise()
{
  damaged = false;
  sentinel_imploding = false;
  crescent_steel_damaged = false;

  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( source -> sim -> event_mgr.canceled )
    return;

  hunter_t* p = static_cast<hunter_t*>( source );
  if ( p -> talents.terms_of_engagement.ok() && damaged )
  {
    p -> sim -> print_debug( "{} harpoon cooldown reset on damaged target death.", p -> name() );
    p -> cooldowns.harpoon -> reset( true );
  }

  if ( p->talents.grave_reaper.ok() && dots.black_arrow->is_ticking() )
  {
    p->sim->print_debug( "{} black_arrow cooldown reduces on target death.", p->name() );
    p->cooldowns.black_arrow->adjust( -timespan_t::from_seconds( p->talents.grave_reaper->effectN( 1 ).base_value() ) );
  }
}

/**
 * Hunter specific action expression
 *
 * Use this function for expressions which are bound to an action property such as target, cast_time etc.
 * If you need an expression tied to the player itself use the normal hunter_t::create_expression override.
 */
std::unique_ptr<expr_t> hunter_t::create_action_expression ( action_t& action, util::string_view expression_str )
{
  auto splits = util::string_split<util::string_view>( expression_str, "." );

  // Careful Aim expression
  if ( splits.size() == 1 && splits[ 0 ] == "ca_active" )
  {
    if ( !talents.careful_aim.ok() )
      return expr_t::create_constant( "ca_active", false );

    return make_fn_expr( "ca_active",
      [ &action, high_pct = talents.careful_aim -> effectN( 1 ).base_value() ] {
        return action.get_expression_target()->health_percentage() > high_pct;
      } );
  }

  return player_t::create_action_expression( action, expression_str );
}

std::unique_ptr<expr_t> hunter_t::create_expression( util::string_view expression_str )
{
  auto splits = util::string_split<util::string_view>( expression_str, "." );

  if ( splits.size() == 1 && splits[ 0 ] == "steady_focus_count" )
  {
    return make_fn_expr( expression_str, [ this ] { return state.steady_focus_counter; } );
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

  return player_t::create_expression( expression_str );
}

action_t* hunter_t::create_action( util::string_view name, util::string_view options_str )
{
  using namespace attacks;
  using namespace spells;

  if ( name == "aimed_shot"            ) return new             aimed_shot_t( this, options_str );
  if ( name == "arcane_shot"           ) return new            arcane_shot_t( this, options_str );
  if ( name == "aspect_of_the_eagle"   ) return new    aspect_of_the_eagle_t( this, options_str );
  if ( name == "auto_attack"           ) return new   actions::auto_attack_t( this, options_str );
  if ( name == "auto_shot"             ) return new   actions::auto_attack_t( this, options_str );
  if ( name == "barbed_shot"           ) return new            barbed_shot_t( this, options_str );
  if ( name == "barrage"               ) return new                barrage_t( this, options_str );
  if ( name == "bestial_wrath"         ) return new          bestial_wrath_t( this, options_str );
  if ( name == "black_arrow"           ) return new            black_arrow_t( this, options_str );
  if ( name == "bloodshed"             ) return new              bloodshed_t( this, options_str );
  if ( name == "bursting_shot"         ) return new          bursting_shot_t( this, options_str );
  if ( name == "butchery"              ) return new               butchery_t( this, options_str );
  if ( name == "call_of_the_wild"      ) return new       call_of_the_wild_t( this, options_str );
  if ( name == "chimaera_shot"         ) return new          chimaera_shot_t( this, options_str );
  if ( name == "cobra_shot"            ) return new             cobra_shot_t( this, options_str );
  if ( name == "coordinated_assault"   ) return new    coordinated_assault_t( this, options_str );
  if ( name == "counter_shot"          ) return new           counter_shot_t( this, options_str );
  if ( name == "dire_beast"            ) return new             dire_beast_t( this, options_str );
  if ( name == "explosive_shot"        ) return new         explosive_shot_t( this, options_str );
  if ( name == "flanking_strike"       ) return new        flanking_strike_t( this, options_str );
  if ( name == "freezing_trap"         ) return new          freezing_trap_t( this, options_str );
  if ( name == "fury_of_the_eagle"     ) return new      fury_of_the_eagle_t( this, options_str );
  if ( name == "harpoon"               ) return new                harpoon_t( this, options_str );
  if ( name == "high_explosive_trap"   ) return new    high_explosive_trap_t( this, options_str );
  if ( name == "implosive_trap"        ) return new         implosive_trap_t( this, options_str );
  if ( name == "kill_command"          ) return new           kill_command_t( this, options_str );
  if ( name == "kill_shot"             ) return new              kill_shot_t( this, options_str );
  if ( name == "muzzle"                ) return new                 muzzle_t( this, options_str );
  if ( name == "rapid_fire"            ) return new             rapid_fire_t( this, options_str );
  if ( name == "salvo"                 ) return new                  salvo_t( this, options_str );
  if ( name == "spearhead"             ) return new              spearhead_t( this, options_str );
  if ( name == "steady_shot"           ) return new            steady_shot_t( this, options_str );
  if ( name == "summon_pet"            ) return new             summon_pet_t( this, options_str );
  if ( name == "tar_trap"              ) return new               tar_trap_t( this, options_str );
  if ( name == "trueshot"              ) return new               trueshot_t( this, options_str );
  if ( name == "volley"                ) return new                 volley_t( this, options_str );
  if ( name == "wailing_arrow"         ) return new          wailing_arrow_t( this, options_str );
  if ( name == "wildfire_bomb"         ) return new          wildfire_bomb_t( this, options_str );

  if ( name == "raptor_strike" || name == "mongoose_bite" || name == "raptor_bite" || name == "mongoose_strike" )
  {
    if ( talents.mongoose_bite.ok() )
      return new mongoose_bite_t( this, options_str );
    else
      return new raptor_strike_t( this, options_str );
  }

  if ( name == "raptor_strike_eagle" || name == "mongoose_bite_eagle" || name == "raptor_bite_eagle" || name == "mongoose_strike_eagle" )
  {
    if ( talents.mongoose_bite.ok() )
      return new mongoose_bite_eagle_t( this, options_str );
    else
      return new raptor_strike_eagle_t( this, options_str );
  }

  if ( name == "multishot" )
  {
    if ( specialization() == HUNTER_MARKSMANSHIP )
      return new multishot_mm_t( this, options_str );
    if ( specialization() == HUNTER_BEAST_MASTERY )
      return new multishot_bm_t( this, options_str );
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
    throw std::invalid_argument(fmt::format("Pet '{}' has unknown type '{}'.", pet_name, pet_type ));
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
  talents.kill_shot                         = find_talent_spell( talent_tree::CLASS, "Kill Shot" );

  talents.improved_kill_shot                = find_talent_spell( talent_tree::CLASS, "Improved Kill Shot" );

  talents.tar_trap                          = find_talent_spell( talent_tree::CLASS, "Tar Trap" );

  talents.counter_shot                      = find_talent_spell( talent_tree::CLASS, "Counter Shot" );
  talents.muzzle                            = find_talent_spell( talent_tree::CLASS, "Muzzle" );

  talents.lone_survivor                     = find_talent_spell( talent_tree::CLASS, "Lone Survivor" );
  talents.specialized_arsenal               = find_talent_spell( talent_tree::CLASS, "Specialized Arsenal" );
  talents.disruptive_rounds                 = find_talent_spell( talent_tree::CLASS, "Disruptive Rounds" );

  talents.explosive_shot                    = find_talent_spell( talent_tree::CLASS, "Explosive Shot" );
  talents.explosive_shot_cast               = find_spell( 212431 );
  talents.explosive_shot_damage             = find_spell( 212680 );

  talents.bursting_shot                     = find_talent_spell( talent_tree::CLASS, "Bursting Shot" );
  talents.scatter_shot                      = find_talent_spell( talent_tree::CLASS, "Scatter Shot" );  
  talents.trigger_finger                    = find_talent_spell( talent_tree::CLASS, "Trigger Finger" );
  talents.blackrock_munitions               = find_talent_spell( talent_tree::CLASS, "Blackrock Munitions" );
  talents.keen_eyesight                     = find_talent_spell( talent_tree::CLASS, "Keen Eyesight" );

  talents.quick_load                        = find_talent_spell( talent_tree::CLASS, "Quick Load" );

  talents.serrated_tips                     = find_talent_spell( talent_tree::CLASS, "Serrated Tips" );
  talents.born_to_be_wild                   = find_talent_spell( talent_tree::CLASS, "Born To Be Wild" );
  talents.improved_traps                    = find_talent_spell( talent_tree::CLASS, "Improved Traps" );

  talents.high_explosive_trap               = find_talent_spell( talent_tree::CLASS, "High Explosive Trap" );
  talents.implosive_trap                    = find_talent_spell( talent_tree::CLASS, "Implosive Trap" );
  talents.explosive_trap_damage             = find_spell( 236777 );
  talents.unnatural_causes                  = find_talent_spell( talent_tree::CLASS, "Unnatural Causes" );
  talents.unnatural_causes_debuff           = talents.unnatural_causes.ok() ? find_spell( 459529 ) : spell_data_t::not_found();

  // Marksmanship Tree
  if ( specialization() == HUNTER_MARKSMANSHIP )
  {
    talents.aimed_shot                        = find_talent_spell( talent_tree::SPECIALIZATION, "Aimed Shot", HUNTER_MARKSMANSHIP );

    talents.rapid_fire                        = find_talent_spell( talent_tree::SPECIALIZATION, "Rapid Fire", HUNTER_MARKSMANSHIP );
    talents.rapid_fire_tick                   = find_spell( 257045 );
    talents.multishot_mm                      = find_talent_spell( talent_tree::SPECIALIZATION, "Multi-Shot", HUNTER_MARKSMANSHIP );
    talents.precise_shots                     = find_talent_spell( talent_tree::SPECIALIZATION, "Precise Shots", HUNTER_MARKSMANSHIP );
    talents.precise_shots_buff                = find_spell( 260242 );

    talents.surging_shots                     = find_talent_spell( talent_tree::SPECIALIZATION, "Surging Shots", HUNTER_MARKSMANSHIP );
    talents.streamline                        = find_talent_spell( talent_tree::SPECIALIZATION, "Streamline", HUNTER_MARKSMANSHIP );
    talents.improved_steady_shot              = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Steady Shot", HUNTER_MARKSMANSHIP );
    talents.crack_shot                        = find_talent_spell( talent_tree::SPECIALIZATION, "Crack Shot", HUNTER_MARKSMANSHIP );

    talents.penetrating_shots                 = find_talent_spell( talent_tree::SPECIALIZATION, "Penetrating Shots", HUNTER_MARKSMANSHIP );
    talents.trick_shots                       = find_talent_spell( talent_tree::SPECIALIZATION, "Trick Shots", HUNTER_MARKSMANSHIP );
    talents.trick_shots_data                  = find_spell( 257621 );
    talents.master_marksman                   = find_talent_spell( talent_tree::SPECIALIZATION, "Master Marksman", HUNTER_MARKSMANSHIP );
    talents.master_marksman_bleed             = talents.master_marksman.ok() ? find_spell( 269576 ) : spell_data_t::not_found();

    talents.fan_the_hammer                    = find_talent_spell( talent_tree::SPECIALIZATION, "Fan the Hammer", HUNTER_MARKSMANSHIP );
    talents.careful_aim                       = find_talent_spell( talent_tree::SPECIALIZATION, "Careful Aim", HUNTER_MARKSMANSHIP );
    talents.light_ammo                        = find_talent_spell( talent_tree::SPECIALIZATION, "Light Ammo", HUNTER_MARKSMANSHIP );
    talents.heavy_ammo                        = find_talent_spell( talent_tree::SPECIALIZATION, "Heavy Ammo", HUNTER_MARKSMANSHIP );
    talents.bulletstorm                       = find_talent_spell( talent_tree::SPECIALIZATION, "Bulletstorm", HUNTER_MARKSMANSHIP );
    talents.lock_and_load                     = find_talent_spell( talent_tree::SPECIALIZATION, "Lock and Load", HUNTER_MARKSMANSHIP );
    talents.steady_focus                      = find_talent_spell( talent_tree::SPECIALIZATION, "Steady Focus", HUNTER_MARKSMANSHIP );

    talents.deathblow                         = find_talent_spell( talent_tree::SPECIALIZATION, "Deathblow", HUNTER_MARKSMANSHIP );
    talents.barrage                           = find_talent_spell( talent_tree::SPECIALIZATION, "Barrage", HUNTER_MARKSMANSHIP );
    talents.night_hunter                      = find_talent_spell( talent_tree::SPECIALIZATION, "Night Hunter", HUNTER_MARKSMANSHIP );
    talents.tactical_reload                   = find_talent_spell( talent_tree::SPECIALIZATION, "Tactical Reload", HUNTER_MARKSMANSHIP );
    talents.serpentstalkers_trickery          = find_talent_spell( talent_tree::SPECIALIZATION, "Serpentstalker's Trickery", HUNTER_MARKSMANSHIP );
    talents.chimaera_shot                     = find_talent_spell( talent_tree::SPECIALIZATION, "Chimaera Shot", HUNTER_MARKSMANSHIP );
    talents.chimaera_shot_nature              = talents.chimaera_shot.ok() ? find_spell( 344120 ) : spell_data_t::not_found();
    talents.chimaera_shot_frost               = talents.chimaera_shot.ok() ? find_spell( 344121 ) : spell_data_t::not_found();
  
    talents.killer_accuracy                   = find_talent_spell( talent_tree::SPECIALIZATION, "Killer Accuracy", HUNTER_MARKSMANSHIP );
    talents.rapid_fire_barrage                = find_talent_spell( talent_tree::SPECIALIZATION, "Rapid Fire Barrage", HUNTER_MARKSMANSHIP );
    talents.rapid_fire_barrage_override       = talents.rapid_fire_barrage.ok() ? find_spell( 459796 ) : spell_data_t::not_found();
    talents.in_the_rhythm                     = find_talent_spell( talent_tree::SPECIALIZATION, "In the Rhythm", HUNTER_MARKSMANSHIP );
    talents.lone_wolf                         = find_talent_spell( talent_tree::SPECIALIZATION, "Lone Wolf", HUNTER_MARKSMANSHIP );
    talents.bullseye                          = find_talent_spell( talent_tree::SPECIALIZATION, "Bullseye", HUNTER_MARKSMANSHIP );
    talents.hydras_bite                       = find_talent_spell( talent_tree::SPECIALIZATION, "Hydra's Bite", HUNTER_MARKSMANSHIP );
    talents.volley                            = find_talent_spell( talent_tree::SPECIALIZATION, "Volley", HUNTER_MARKSMANSHIP );
    talents.volley_data                       = find_spell( 260243 );
    talents.volley_dmg                        = find_spell( 260247 );

    talents.legacy_of_the_windrunners         = find_talent_spell( talent_tree::SPECIALIZATION, "Legacy of the Windrunners", HUNTER_MARKSMANSHIP );
    talents.legacy_of_the_windrunners_wind_arrow = talents.legacy_of_the_windrunners.ok() ? find_spell( 191043 ) : spell_data_t::not_found();
    talents.trueshot                          = find_talent_spell( talent_tree::SPECIALIZATION, "Trueshot", HUNTER_MARKSMANSHIP );
    talents.focused_aim                       = find_talent_spell( talent_tree::SPECIALIZATION, "Focused Aim", HUNTER_MARKSMANSHIP );

    talents.razor_fragments                   = find_talent_spell( talent_tree::SPECIALIZATION, "Razor Fragments", HUNTER_MARKSMANSHIP );
    talents.razor_fragments_bleed             = talents.razor_fragments.ok() ? find_spell( 385638 ) : spell_data_t::not_found();
    talents.razor_fragments_buff              = talents.razor_fragments.ok() ? find_spell( 388998 ) : spell_data_t::not_found();
    talents.wailing_arrow                     = find_talent_spell( talent_tree::SPECIALIZATION, "Wailing Arrow", HUNTER_MARKSMANSHIP );
    talents.wailing_arrow_counter_buff        = talents.wailing_arrow.ok() ? find_spell( 459805 ) : spell_data_t::not_found();
    talents.wailing_arrow_override_buff       = talents.wailing_arrow.ok() ? find_spell( 459808 ) : spell_data_t::not_found();
    talents.wailing_arrow_override            = talents.wailing_arrow.ok() ? find_spell( 392060 ) : spell_data_t::not_found();
    talents.wailing_arrow_damage              = talents.wailing_arrow.ok() ? find_spell( 392058 ) : spell_data_t::not_found();
    talents.eagletalons_true_focus            = find_talent_spell( talent_tree::SPECIALIZATION, "Eagletalon's True Focus", HUNTER_MARKSMANSHIP );
    talents.calling_the_shots                 = find_talent_spell( talent_tree::SPECIALIZATION, "Calling the Shots", HUNTER_MARKSMANSHIP );
    talents.small_game_hunter                 = find_talent_spell( talent_tree::SPECIALIZATION, "Small Game Hunter", HUNTER_MARKSMANSHIP );
    talents.kill_zone                         = find_talent_spell( talent_tree::SPECIALIZATION, "Kill Zone", HUNTER_MARKSMANSHIP );
    talents.kill_zone_debuff                  = talents.kill_zone.ok() ? find_spell( 393480 ) : spell_data_t::not_found();

    talents.readiness                         = find_talent_spell( talent_tree::SPECIALIZATION, "Readiness", HUNTER_MARKSMANSHIP );
    talents.unerring_vision                   = find_talent_spell( talent_tree::SPECIALIZATION, "Unerring Vision", HUNTER_MARKSMANSHIP );
    talents.salvo                             = find_talent_spell( talent_tree::SPECIALIZATION, "Salvo", HUNTER_MARKSMANSHIP );
  }

  // Beast Mastery Tree
  if ( specialization() == HUNTER_BEAST_MASTERY )
  {
    talents.kill_command                      = find_talent_spell( talent_tree::SPECIALIZATION, "Kill Command", HUNTER_BEAST_MASTERY );

    talents.cobra_shot                        = find_talent_spell( talent_tree::SPECIALIZATION, "Cobra Shot", HUNTER_BEAST_MASTERY );
    talents.animal_companion                  = find_talent_spell( talent_tree::SPECIALIZATION, "Animal Companion", HUNTER_BEAST_MASTERY );
    talents.barbed_shot                       = find_talent_spell( talent_tree::SPECIALIZATION, "Barbed Shot", HUNTER_BEAST_MASTERY );

    talents.pack_tactics                      = find_talent_spell( talent_tree::SPECIALIZATION, "Pack Tactics", HUNTER_BEAST_MASTERY );
    talents.aspect_of_the_beast               = find_talent_spell( talent_tree::SPECIALIZATION, "Aspect of the Beast", HUNTER_BEAST_MASTERY );
    talents.war_orders                        = find_talent_spell( talent_tree::SPECIALIZATION, "War Orders", HUNTER_BEAST_MASTERY );
    talents.thrill_of_the_hunt                = find_talent_spell( talent_tree::SPECIALIZATION, "Thrill of the Hunt", HUNTER_BEAST_MASTERY );

    talents.go_for_the_throat                 = find_talent_spell( talent_tree::SPECIALIZATION, "Go for the Throat", HUNTER_BEAST_MASTERY );
    talents.multishot_bm                      = find_talent_spell( talent_tree::SPECIALIZATION, "Multi-Shot", HUNTER_BEAST_MASTERY );
    talents.laceration                        = find_talent_spell( talent_tree::SPECIALIZATION, "Laceration", HUNTER_BEAST_MASTERY );

    talents.cobra_senses                      = find_talent_spell( talent_tree::SPECIALIZATION, "Cobra Senses", HUNTER_BEAST_MASTERY );
    talents.alpha_predator                    = find_talent_spell( talent_tree::SPECIALIZATION, "Alpha Predator", HUNTER_BEAST_MASTERY );
    talents.improved_kill_command             = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Kill Command", HUNTER_BEAST_MASTERY );
    talents.beast_cleave                      = find_talent_spell( talent_tree::SPECIALIZATION, "Beast Cleave", HUNTER_BEAST_MASTERY );
    talents.wild_call                         = find_talent_spell( talent_tree::SPECIALIZATION, "Wild Call", HUNTER_BEAST_MASTERY );
    talents.hunters_prey                      = find_talent_spell( talent_tree::SPECIALIZATION, "Hunter's Prey", HUNTER_BEAST_MASTERY );
    talents.venoms_bite                       = find_talent_spell( talent_tree::SPECIALIZATION, "Venom's Bite", HUNTER_BEAST_MASTERY );

    talents.stomp                             = find_talent_spell( talent_tree::SPECIALIZATION, "Stomp", HUNTER_BEAST_MASTERY );
    talents.kindred_spirits                   = find_talent_spell( talent_tree::SPECIALIZATION, "Kindred Spirits", HUNTER_BEAST_MASTERY );
    talents.kill_cleave                       = find_talent_spell( talent_tree::SPECIALIZATION, "Kill Cleave", HUNTER_BEAST_MASTERY );
    talents.training_expert                   = find_talent_spell( talent_tree::SPECIALIZATION, "Training Expert", HUNTER_BEAST_MASTERY );
    talents.dire_beast                        = find_talent_spell( talent_tree::SPECIALIZATION, "Dire Beast", HUNTER_BEAST_MASTERY );

    talents.a_murder_of_crows                 = find_talent_spell( talent_tree::SPECIALIZATION, "A Murder of Crows", HUNTER_BEAST_MASTERY );
    talents.barrage                           = find_talent_spell( talent_tree::SPECIALIZATION, "Barrage", HUNTER_BEAST_MASTERY );
    talents.savagery                          = find_talent_spell( talent_tree::SPECIALIZATION, "Savagery", HUNTER_BEAST_MASTERY );
    talents.bestial_wrath                     = find_talent_spell( talent_tree::SPECIALIZATION, "Bestial Wrath", HUNTER_BEAST_MASTERY );
    talents.dire_command                      = find_talent_spell( talent_tree::SPECIALIZATION, "Dire Command", HUNTER_BEAST_MASTERY );
    talents.huntmasters_call                  = find_talent_spell( talent_tree::SPECIALIZATION, "Huntmaster's Call", HUNTER_BEAST_MASTERY );
    talents.dire_frenzy                       = find_talent_spell( talent_tree::SPECIALIZATION, "Dire Frenzy", HUNTER_BEAST_MASTERY );

    talents.killer_instinct                   = find_talent_spell( talent_tree::SPECIALIZATION, "Killer Instinct", HUNTER_BEAST_MASTERY );
    talents.master_handler                    = find_talent_spell( talent_tree::SPECIALIZATION, "Master Handler", HUNTER_BEAST_MASTERY );
    talents.barbed_wrath                      = find_talent_spell( talent_tree::SPECIALIZATION, "Barbed Wrath", HUNTER_BEAST_MASTERY );
    talents.explosive_venom                   = find_talent_spell( talent_tree::SPECIALIZATION, "Explosive Venom", HUNTER_BEAST_MASTERY );
    talents.basilisk_collar                   = find_talent_spell( talent_tree::SPECIALIZATION, "Basilisk Collar", HUNTER_BEAST_MASTERY );

    talents.call_of_the_wild                  = find_talent_spell( talent_tree::SPECIALIZATION, "Call of the Wild", HUNTER_BEAST_MASTERY );
    talents.killer_cobra                      = find_talent_spell( talent_tree::SPECIALIZATION, "Killer Cobra", HUNTER_BEAST_MASTERY );
    talents.scent_of_blood                    = find_talent_spell( talent_tree::SPECIALIZATION, "Scent of Blood", HUNTER_BEAST_MASTERY );
    talents.brutal_companion                  = find_talent_spell( talent_tree::SPECIALIZATION, "Brutal Companion", HUNTER_BEAST_MASTERY );
    talents.bloodshed                         = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodshed", HUNTER_BEAST_MASTERY );

    talents.wild_instincts                    = find_talent_spell( talent_tree::SPECIALIZATION, "Wild Instincts", HUNTER_BEAST_MASTERY );
    talents.bloody_frenzy                     = find_talent_spell( talent_tree::SPECIALIZATION, "Bloody Frenzy", HUNTER_BEAST_MASTERY );
    talents.piercing_fangs                    = find_talent_spell( talent_tree::SPECIALIZATION, "Piercing Fangs", HUNTER_BEAST_MASTERY );
    talents.venomous_bite                     = find_talent_spell( talent_tree::SPECIALIZATION, "Venomous Bite", HUNTER_BEAST_MASTERY );
    talents.shower_of_blood                   = find_talent_spell( talent_tree::SPECIALIZATION, "Shower of Blood", HUNTER_BEAST_MASTERY );
  }

  // Survival Tree
  if ( specialization() == HUNTER_SURVIVAL )
  {
    talents.kill_command                      = find_talent_spell( talent_tree::SPECIALIZATION, "Kill Command", HUNTER_SURVIVAL );

    talents.wildfire_bomb                     = find_talent_spell( talent_tree::SPECIALIZATION, "Wildfire Bomb", HUNTER_SURVIVAL );
    talents.wildfire_bomb_data                = find_spell( 259495 );
    talents.wildfire_bomb_dmg                 = find_spell( 265157 );
    talents.wildfire_bomb_dot                 = find_spell( 269747 );
    talents.raptor_strike                     = find_talent_spell( talent_tree::SPECIALIZATION, "Raptor Strike", HUNTER_SURVIVAL );
    talents.raptor_strike_eagle               = talents.raptor_strike.ok() ? find_spell( 265189 ) : spell_data_t::not_found();

    talents.guerrilla_tactics                 = find_talent_spell( talent_tree::SPECIALIZATION, "Guerrilla Tactics", HUNTER_SURVIVAL );
    talents.tip_of_the_spear                  = find_talent_spell( talent_tree::SPECIALIZATION, "Tip of the Spear", HUNTER_SURVIVAL );
    talents.tip_of_the_spear_buff             = talents.tip_of_the_spear.ok() ? find_spell( 260286 ) : spell_data_t::not_found();
    talents.tip_of_the_spear_explosive_buff   = talents.tip_of_the_spear.ok() ? find_spell( 460852 ) : spell_data_t::not_found();

    talents.lunge                             = find_talent_spell( talent_tree::SPECIALIZATION, "Lunge", HUNTER_SURVIVAL );
    talents.quick_shot                        = find_talent_spell( talent_tree::SPECIALIZATION, "Quick Shot", HUNTER_SURVIVAL );
    talents.mongoose_bite                     = find_talent_spell( talent_tree::SPECIALIZATION, "Mongoose Bite", HUNTER_SURVIVAL );
    talents.mongoose_bite_eagle               = talents.mongoose_bite.ok() ? find_spell( 265888 ) : spell_data_t::not_found();
    talents.flankers_advantage                = find_talent_spell( talent_tree::SPECIALIZATION, "Flanker's Advantage", HUNTER_SURVIVAL );

    talents.wildfire_infusion                 = find_talent_spell( talent_tree::SPECIALIZATION, "Wildfire Infusion", HUNTER_SURVIVAL );
    talents.improved_wildfire_bomb            = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Wildfire Bomb", HUNTER_SURVIVAL );
    talents.sulfur_lined_pockets              = find_talent_spell( talent_tree::SPECIALIZATION, "Sulfur-Lined Pockets", HUNTER_SURVIVAL );
    talents.butchery                          = find_talent_spell( talent_tree::SPECIALIZATION, "Butchery", HUNTER_SURVIVAL );
    talents.bloody_claws                      = find_talent_spell( talent_tree::SPECIALIZATION, "Bloody Claws", HUNTER_SURVIVAL );
    talents.alpha_predator                    = find_talent_spell( talent_tree::SPECIALIZATION, "Alpha Predator", HUNTER_SURVIVAL );
    talents.terms_of_engagement               = find_talent_spell( talent_tree::SPECIALIZATION, "Terms of Engagement", HUNTER_SURVIVAL );
    talents.terms_of_engagement_dmg           = talents.terms_of_engagement.ok() ? find_spell( 271625 ) : spell_data_t::not_found();
    talents.terms_of_engagement_buff          = talents.terms_of_engagement.ok() ? find_spell( 265898 ) : spell_data_t::not_found();

    talents.grenade_juggler                   = find_talent_spell( talent_tree::SPECIALIZATION, "Grenade Juggler", HUNTER_SURVIVAL );
    talents.flanking_strike                   = find_talent_spell( talent_tree::SPECIALIZATION, "Flanking Strike", HUNTER_SURVIVAL );
    talents.flanking_strike_player            = talents.flanking_strike.ok() ? find_spell( 269752 ) : spell_data_t::not_found();
    talents.frenzy_strikes                    = find_talent_spell( talent_tree::SPECIALIZATION, "Frenzy Strikes", HUNTER_SURVIVAL );
    talents.merciless_blows                   = find_talent_spell( talent_tree::SPECIALIZATION, "Merciless Blows", HUNTER_SURVIVAL );
    talents.vipers_venom                      = find_talent_spell( talent_tree::SPECIALIZATION, "Viper's Venom", HUNTER_SURVIVAL );
    talents.bloodseeker                       = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodseeker", HUNTER_SURVIVAL );

    talents.ranger                            = find_talent_spell( talent_tree::SPECIALIZATION, "Ranger", HUNTER_SURVIVAL );
    talents.exposed_flank                     = find_talent_spell( talent_tree::SPECIALIZATION, "Exposed Flank", HUNTER_SURVIVAL );
    talents.tactical_advantage                = find_talent_spell( talent_tree::SPECIALIZATION, "Tactical Advantage", HUNTER_SURVIVAL );
    talents.sic_em                            = find_talent_spell( talent_tree::SPECIALIZATION, "Sic 'Em", HUNTER_SURVIVAL );
    talents.contagious_reagents               = find_talent_spell( talent_tree::SPECIALIZATION, "Contagious Reagents", HUNTER_SURVIVAL );
    talents.outland_venom                     = find_talent_spell( talent_tree::SPECIALIZATION, "Outland Venom", HUNTER_SURVIVAL );
    talents.outland_venom_debuff              = talents.outland_venom.ok() ? find_spell( 459941 ) : spell_data_t::not_found();

    talents.explosives_expert                 = find_talent_spell( talent_tree::SPECIALIZATION, "Explosives Expert", HUNTER_SURVIVAL );
    talents.sweeping_spear                    = find_talent_spell( talent_tree::SPECIALIZATION, "Sweeping Spear", HUNTER_SURVIVAL );
    talents.killer_companion                  = find_talent_spell( talent_tree::SPECIALIZATION, "Killer Companion", HUNTER_SURVIVAL );

    talents.fury_of_the_eagle                 = find_talent_spell( talent_tree::SPECIALIZATION, "Fury of the Eagle", HUNTER_SURVIVAL );
    talents.coordinated_assault               = find_talent_spell( talent_tree::SPECIALIZATION, "Coordinated Assault", HUNTER_SURVIVAL );
    talents.coordinated_assault_dmg           = talents.coordinated_assault.ok() ? find_spell( 360969 ) : spell_data_t::not_found();

    talents.spearhead                         = find_talent_spell( talent_tree::SPECIALIZATION, "Spearhead", HUNTER_SURVIVAL );
    talents.spearhead_attack                  = talents.spearhead.ok() ? find_spell( 378957 ) : spell_data_t::not_found();

    talents.ruthless_marauder                 = find_talent_spell( talent_tree::SPECIALIZATION, "Ruthless Marauder", HUNTER_SURVIVAL );
    talents.symbiotic_adrenaline              = find_talent_spell( talent_tree::SPECIALIZATION, "Symbiotic Adrenaline", HUNTER_SURVIVAL );
    talents.relentless_primal_ferocity        = find_talent_spell( talent_tree::SPECIALIZATION, "Relentless Primal Ferocity", HUNTER_SURVIVAL );
    talents.relentless_primal_ferocity_buff   = talents.relentless_primal_ferocity.ok() ? find_spell( 459962 ) : spell_data_t::not_found();
    talents.bombardier                        = find_talent_spell( talent_tree::SPECIALIZATION, "Bombardier", HUNTER_SURVIVAL );
    talents.bombardier_buff                   = talents.bombardier.ok() ? find_spell( 459859 ) : spell_data_t::not_found();
    talents.deadly_duo                        = find_talent_spell( talent_tree::SPECIALIZATION, "Deadly Duo", HUNTER_SURVIVAL );

    specs.serpent_sting = find_spell( 259491 );
    specs.harpoon = find_spell( 190925 );
  }

  if ( specialization() == HUNTER_MARKSMANSHIP || specialization() == HUNTER_BEAST_MASTERY )
  {
    // Dark Ranger
    talents.black_arrow = find_talent_spell( talent_tree::HERO, "Black Arrow" );
    talents.black_arrow_buff = talents.black_arrow.ok() ? find_spell( 439659 ) : spell_data_t::not_found();

    talents.overshadow    = find_talent_spell( talent_tree::HERO, "Overshadow" );
    talents.shadow_hounds = find_talent_spell( talent_tree::HERO, "Shadow Hounds" );
    talents.death_shade   = find_talent_spell( talent_tree::HERO, "Death Shade" );

    talents.dark_empowerment    = find_talent_spell( talent_tree::HERO, "Dark Empowerment" );
    talents.grave_reaper        = find_talent_spell( talent_tree::HERO, "Grave Reaper" );
    talents.embrace_the_shadows = find_talent_spell( talent_tree::HERO, "Embrace the Shadows" );
    talents.smoke_screen        = find_talent_spell( talent_tree::HERO, "Smoke Screen" );
    talents.dark_chains         = find_talent_spell( talent_tree::HERO, "Dark Chains" );

    talents.shadow_lash    = find_talent_spell( talent_tree::HERO, "Shadow Lash" );
    talents.shadow_surge   = find_talent_spell( talent_tree::HERO, "Shadow Surge" );
    talents.shadow_surge_dmg = talents.shadow_surge.ok() ? find_spell( 444269 ) : spell_data_t::not_found();
    talents.darkness_calls = find_talent_spell( talent_tree::HERO, "Darkness Calls" );
    talents.shadow_erasure = find_talent_spell( talent_tree::HERO, "Shadow Erasure" );

    talents.withering_fire = find_talent_spell( talent_tree::HERO, "Withering Fire" );
    talents.withering_fire_dmg  = talents.withering_fire.ok() ? find_spell( 461490 ) : spell_data_t::not_found();
    talents.withering_fire_buff = talents.withering_fire.ok() ? find_spell( 461762 ) : spell_data_t::not_found();

    specs.serpent_sting = find_spell( 271788 );
  }

  if ( specialization() == HUNTER_BEAST_MASTERY || specialization() == HUNTER_SURVIVAL )
  {
    // Pack Leader
    talents.vicious_hunt = find_talent_spell( talent_tree::HERO, "Vicious Hunt" );

    talents.pack_coordination = find_talent_spell( talent_tree::HERO, "Pack Coordination" );
    talents.howl_of_the_pack  = find_talent_spell( talent_tree::HERO, "Howl of the Pack" );
    talents.wild_attacks      = find_talent_spell( talent_tree::HERO, "Wild Attacks" );

    talents.den_recovery  = find_talent_spell( talent_tree::HERO, "Den Recovery" );
    talents.tireless_hunt = find_talent_spell( talent_tree::HERO, "Tireless Hunt" );
    talents.cornered_prey = find_talent_spell( talent_tree::HERO, "Cornered Prey" );
    talents.frenzied_tear = find_talent_spell( talent_tree::HERO, "Frenzied Tear" );

    talents.scattered_prey       = find_talent_spell( talent_tree::HERO, "Scattered Prey" );
    talents.covering_fire        = find_talent_spell( talent_tree::HERO, "Covering Fire" );
    talents.cull_the_herd        = find_talent_spell( talent_tree::HERO, "Cull the Herd" );
    talents.furious_assault      = find_talent_spell( talent_tree::HERO, "Furious Assault" );
    talents.beast_of_opportunity = find_talent_spell( talent_tree::HERO, "Beast of Opportunity" );

    talents.pack_assault = find_talent_spell( talent_tree::HERO, "Pack Assault" );
  }

  if ( specialization() == HUNTER_MARKSMANSHIP || specialization() == HUNTER_SURVIVAL )
  {
    // Sentinel
    talents.sentinel = find_talent_spell( talent_tree::HERO, "Sentinel" );
    talents.sentinel_debuff = talents.sentinel.ok() ? find_spell( 450387 ) : spell_data_t::not_found();
    talents.sentinel_tick   = talents.sentinel.ok() ? find_spell( 450412 ) : spell_data_t::not_found();

    talents.dont_look_back     = find_talent_spell( talent_tree::HERO, "Don't Look Back" );
    talents.extrapolated_shots = find_talent_spell( talent_tree::HERO, "Extrapolated Shots" );
    talents.sentinel_precision = find_talent_spell( talent_tree::HERO, "Sentinel Precision" );

    talents.release_and_reload = find_talent_spell( talent_tree::HERO, "Release and Reload" );
    talents.catch_out          = find_talent_spell( talent_tree::HERO, "Catch Out" );
    talents.sideline           = find_talent_spell( talent_tree::HERO, "Sideline" );
    talents.invigorating_pulse = find_talent_spell( talent_tree::HERO, "Invigorating Pulse" );

    talents.sentinel_watch    = find_talent_spell( talent_tree::HERO, "Sentinel Watch" );
    talents.eyes_closed       = find_talent_spell( talent_tree::HERO, "Eyes Closed" );
    talents.symphonic_arsenal = find_talent_spell( talent_tree::HERO, "Symphonic Arsenal" );
    talents.symphonic_arsenal_dmg = talents.symphonic_arsenal.ok() ? find_spell( 451194 ) : spell_data_t::not_found();
    talents.overwatch         = find_talent_spell( talent_tree::HERO, "Overwatch" );
    talents.crescent_steel    = find_talent_spell( talent_tree::HERO, "Crescent Steel" );
    talents.crescent_steel_debuff = talents.crescent_steel.ok() ? find_spell( 451531 ) : spell_data_t::not_found();

    talents.lunar_storm = find_talent_spell( talent_tree::HERO, "Lunar Storm" );
    talents.lunar_storm_ground = talents.lunar_storm.ok() ? find_spell( 450978 ) : spell_data_t::not_found();
    talents.lunar_storm_dmg    = talents.lunar_storm.ok() ? find_spell( 450883 ) : spell_data_t::not_found();
  }

  // Mastery
  mastery.master_of_beasts     = find_mastery_spell( HUNTER_BEAST_MASTERY );
  mastery.sniper_training      = find_mastery_spell( HUNTER_MARKSMANSHIP );
  mastery.spirit_bond          = find_mastery_spell( HUNTER_SURVIVAL );
  mastery.spirit_bond_buff     = mastery.spirit_bond.ok() ? find_spell( 459722 ) : spell_data_t::not_found();

  // Spec spells
  specs.critical_strikes     = find_spell( 157443 );
  specs.hunter               = find_spell( 137014 );
  specs.beast_mastery_hunter = find_specialization_spell( "Beast Mastery Hunter" );
  specs.marksmanship_hunter  = find_specialization_spell( "Marksmanship Hunter" );
  specs.survival_hunter      = find_specialization_spell( "Survival Hunter" );

  specs.auto_shot            = find_spell( 75 );
  specs.freezing_trap        = find_class_spell( "Freezing Trap" );
  specs.arcane_shot          = find_class_spell( "Arcane Shot" );
  specs.steady_shot          = find_class_spell( "Steady Shot" );
  specs.flare                = find_class_spell( "Flare" );

  // Tier Sets
  tier_set.tww_s1_bm_2pc = sets -> set( HUNTER_BEAST_MASTERY, TWW1, B2 );
  tier_set.tww_s1_bm_4pc = sets -> set( HUNTER_BEAST_MASTERY, TWW1, B4 );
  tier_set.tww_s1_mm_2pc = sets -> set( HUNTER_MARKSMANSHIP, TWW1, B2 );
  tier_set.tww_s1_mm_4pc = sets -> set( HUNTER_MARKSMANSHIP, TWW1, B4 );
  tier_set.tww_s1_mm_4pc_buff = tier_set.tww_s1_mm_4pc.ok() ? find_spell( 457116 ) : spell_data_t::not_found();
  tier_set.tww_s1_sv_2pc = sets -> set( HUNTER_SURVIVAL, TWW1, B2 );
  tier_set.tww_s1_sv_4pc = sets -> set( HUNTER_SURVIVAL, TWW1, B4 );

  // Cooldowns
  cooldowns.ruthless_marauder -> duration = talents.ruthless_marauder -> internal_cooldown();
  cooldowns.shadow_surge->duration = talents.shadow_surge->internal_cooldown();
  cooldowns.legacy_of_the_windrunners->duration = talents.legacy_of_the_windrunners->internal_cooldown();
  cooldowns.lunar_storm->duration = talents.lunar_storm->internal_cooldown();
}

void hunter_t::init_base_stats()
{
  if ( base.distance < 1 )
  {
    base.distance = 40;
    if ( specialization() == HUNTER_SURVIVAL )
      base.distance = 5;
  }

  player_t::init_base_stats();

  base.attack_power_per_strength = 0;
  base.attack_power_per_agility  = 1;
  base.spell_power_per_intellect = 1;

  resources.base_regen_per_second[ RESOURCE_FOCUS ] = 5;
  for ( auto spell : { specs.marksmanship_hunter, specs.survival_hunter, talents.pack_tactics } )
  {
    for ( const spelleffect_data_t& effect : spell -> effects() )
    {
      if ( effect.ok() && effect.type() == E_APPLY_AURA && effect.subtype() == A_MOD_POWER_REGEN_PERCENT )
        resources.base_regen_per_second[ RESOURCE_FOCUS ] *= 1 + effect.percent();
    }
  }

  resources.base[RESOURCE_FOCUS] = 100
    + talents.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS );
}

void hunter_t::create_actions()
{
  // Since without an apl defined use of Serpent Sting we can end up in a situation where no "serpent_sting" action is created 
  // for the "active_dot" expression to look up the dot by, force an action to be made here that may or may not go unused.
  get_background_action<attacks::serpent_sting_t>( "serpent_sting" );

  player_t::create_actions();

  if ( talents.dire_command.ok() )
    actions.dire_command = new spells::dire_command_summon_t( this );
  
  if ( talents.shadow_surge.ok() )
    actions.shadow_surge = new attacks::shadow_surge_t( this );

  if ( talents.withering_fire.ok() )
    actions.withering_fire = new attacks::withering_fire_t( this );
  
  if ( talents.a_murder_of_crows.ok() )
    actions.a_murder_of_crows = new spells::a_murder_of_crows_t( this );

  if( talents.vicious_hunt.ok() )
    actions.vicious_hunt = new attacks::vicious_hunt_t( this ); 

  if( talents.cull_the_herd.ok() )
    actions.cull_the_herd = new attacks::cull_the_herd_t( this );

  if ( talents.sentinel.ok() )
    actions.sentinel = new attacks::sentinel_t( this );

  if ( talents.symphonic_arsenal.ok() )
    actions.symphonic_arsenal = new attacks::symphonic_arsenal_t( this );

  if ( talents.lunar_storm.ok() )
    actions.lunar_storm = new attacks::lunar_storm_t( this );
}

void hunter_t::create_buffs()
{
  player_t::create_buffs();

  // Hunter Tree

  buffs.trigger_finger =
    make_buff( this, "trigger_finger", talents.trigger_finger )
      ->set_default_value( talents.trigger_finger->effectN( 1 ).percent() * ( 1 + talents.trigger_finger->effectN( 3 ).percent() ) )
      ->add_invalidate( CACHE_AUTO_ATTACK_SPEED )
      ->set_quiet(true);

  // Marksmanship Tree

  buffs.precise_shots =
    make_buff( this, "precise_shots", talents.precise_shots_buff )
      -> set_default_value( talents.precise_shots -> effectN( 1 ).percent() )
      -> set_reverse( true )
      -> set_chance( talents.precise_shots.ok() );

  buffs.streamline =
    make_buff( this, "streamline", find_spell( 342076 ) )
      -> set_default_value( talents.streamline -> effectN( 3 ).percent() )
      -> set_chance( talents.streamline.ok() );

  buffs.in_the_rhythm = 
    make_buff( this, "in_the_rhythm", find_spell( 407405 ) )
      -> set_default_value_from_effect( 1 )
      -> set_pct_buff_type( STAT_PCT_BUFF_HASTE )
      -> set_chance( talents.in_the_rhythm.ok() );

  buffs.bullseye =
    make_buff( this, "bullseye", talents.bullseye -> effectN( 1 ).trigger() )
      -> set_default_value_from_effect( 1 )
      -> set_max_stack( std::max( as<int>( talents.bullseye -> effectN( 2 ).base_value() ), 1 ) )
      -> set_chance( talents.bullseye.ok() );

  buffs.volley = make_buff( this, "volley", talents.volley_data )
      -> set_cooldown( 0_ms )
      -> set_period( 0_ms ) // disable ticks as an optimization
      -> set_refresh_behavior( buff_refresh_behavior::DURATION );

  buffs.trick_shots =
    make_buff<buffs::trick_shots_t>( this, "trick_shots", find_spell( 257622 ) );

  buffs.steady_focus =
    make_buff( this, "steady_focus", find_spell( 193534 ) )
    -> set_default_value( talents.steady_focus -> effectN( 1 ).percent() )
    -> set_pct_buff_type( STAT_PCT_BUFF_HASTE )
    -> set_chance( talents.steady_focus.ok() );

  buffs.lone_wolf =
    make_buff( this, "lone_wolf", find_spell( 164273 ) )
    -> set_default_value( talents.lone_wolf -> effectN( 1 ).percent() )
    -> set_period( 0_ms ) // disable ticks as an optimization
    -> set_chance( talents.lone_wolf.ok() );

  buffs.trueshot =
    make_buff( this, "trueshot", find_spell( 288613 ) )
      -> set_cooldown( 0_ms )
      -> set_default_value_from_effect( 4 )
      -> set_refresh_behavior( buff_refresh_behavior::EXTEND )
      -> set_affects_regen( true )
      -> set_stack_change_callback(
        [ this ]( buff_t*, int /*ol*/, int cur ) {
          cooldowns.aimed_shot -> adjust_recharge_multiplier();
          cooldowns.rapid_fire -> adjust_recharge_multiplier();
          if ( cur == 0 ) 
          {
            buffs.eagletalons_true_focus -> expire();
            buffs.unerring_vision_hidden -> expire();
            buffs.unerring_vision -> expire();
          }
          else if ( cur == 1 ) 
          {
            buffs.unerring_vision_hidden -> trigger();
            buffs.eagletalons_true_focus -> trigger();
          }
        } )
      -> apply_affecting_aura( talents.eagletalons_true_focus );

  buffs.lock_and_load =
    make_buff( this, "lock_and_load", talents.lock_and_load -> effectN( 1 ).trigger() )
      -> set_trigger_spell( talents.lock_and_load );

  buffs.deathblow =
    make_buff( this, "deathblow", find_spell( 378770 ) )
      -> set_stack_change_callback(
        [ this ]( buff_t*, int old, int ) {
          // XXX: check refreshes
          if ( old == 0 ) {
            cooldowns.kill_shot -> reset( true );
            buffs.razor_fragments -> trigger();
          }
        } )
      -> set_activated( false );

  buffs.razor_fragments =
    make_buff( this, "razor_fragments", talents.razor_fragments_buff )
    -> set_default_value_from_effect( 1 );

  buffs.unerring_vision_hidden =
    make_buff( this, "unerring_vision_hidden", talents.unerring_vision -> effectN( 1 ).trigger() )
      -> set_quiet( true )
      -> set_tick_zero( true )
      -> set_tick_callback(
        [ this ]( buff_t*, int, const timespan_t& ) {
          buffs.unerring_vision -> trigger();
        } );

  buffs.unerring_vision =
    make_buff<stat_buff_t>(this, "unerring_vision", find_spell( 386877 ) )
      -> add_invalidate( CACHE_CRIT_CHANCE );

  buffs.eagletalons_true_focus =
    make_buff( this, "eagletalons_true_focus", talents.eagletalons_true_focus -> effectN( 4 ).trigger() )
      -> set_trigger_spell( talents.eagletalons_true_focus );

  buffs.bulletstorm =
    make_buff( this, "bulletstorm", find_spell( 389020 ) )
      -> set_default_value_from_effect( 1 )
      -> set_refresh_behavior( buff_refresh_behavior::DISABLED );

  buffs.salvo =
    make_buff( this, "salvo", find_spell( 400456 ) );

  buffs.wailing_arrow_counter = 
    make_buff( this, "wailing_arrow", talents.wailing_arrow_counter_buff )
      ->set_chance( talents.wailing_arrow.ok() );

  buffs.wailing_arrow_override = 
    make_buff( this, "wailing_arrow_override", talents.wailing_arrow_override_buff )
      ->set_quiet( true )
      ->set_chance( talents.wailing_arrow.ok() );

  // Beast Mastery Tree
  const spell_data_t* barbed_shot = find_spell( 246152 );
  for ( size_t i = 0; i < buffs.barbed_shot.size(); i++ )
  {
    buffs.barbed_shot[ i ] =
      make_buff( this, fmt::format( "barbed_shot_{}", i + 1 ), barbed_shot )
        -> set_default_value( barbed_shot -> effectN( 1 ).resource( RESOURCE_FOCUS ) )
        -> set_tick_callback(
          [ this ]( buff_t* b, int, timespan_t ) {
            resource_gain( RESOURCE_FOCUS, b -> default_value, gains.barbed_shot, actions.barbed_shot );
          } );
  }

  buffs.thrill_of_the_hunt =
    make_buff( this, "thrill_of_the_hunt", talents.thrill_of_the_hunt -> effectN( 1 ).trigger() )
      -> apply_affecting_aura( talents.savagery )
      -> set_default_value_from_effect( 1 )
      -> set_max_stack( std::max( 1, as<int>( talents.thrill_of_the_hunt -> effectN( 2 ).base_value() ) ) )
      -> set_trigger_spell( talents.thrill_of_the_hunt );

  buffs.dire_beast =
    make_buff( this, "dire_beast", find_spell( 120679 ) -> effectN( 2 ).trigger() )
      -> modify_duration( talents.dire_frenzy -> effectN( 1 ).time_value() )
      -> set_default_value_from_effect( 1 )
      -> set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buffs.bestial_wrath =
    make_buff( this, "bestial_wrath", talents.bestial_wrath )
      -> set_cooldown( 0_ms )
      -> set_default_value_from_effect( 1 );

  buffs.hunters_prey =
    make_buff( this, "hunters_prey", find_spell( 378215 ) )
      -> set_stack_change_callback(
        [ this ]( buff_t*, int old, int ) {
          if ( old == 0 ) {
            cooldowns.kill_shot -> reset( true );
          }
        } )
      -> set_activated( false );

  buffs.call_of_the_wild =
    make_buff( this, "call_of_the_wild", talents.call_of_the_wild )
      -> set_cooldown( 0_ms )
      -> set_tick_callback(
        [ this ]( buff_t*, int, timespan_t ) {
          pets.cotw_stable_pet.spawn( talents.call_of_the_wild -> effectN( 2 ).trigger() -> duration(), 1 );

          double percent_reduction = talents.call_of_the_wild -> effectN( 3 ).base_value() / 100.0; 
          cooldowns.kill_command -> adjust( -( cooldowns.kill_command -> duration * percent_reduction ) );
          cooldowns.barbed_shot -> adjust( -( cooldowns.barbed_shot -> duration * percent_reduction ) );

          if( talents.bloody_frenzy.ok() )
          {
            //In-game this (re)application of beast_cleave happens multiple times a second, since the regular Beast Cleave buff is longer than the time between ticks, we can get by with just refreshing once per tick
            //In 11.0 it has been changed to use the player's remaining duration of beast_cleave, instead of the remaining duration of call_of_the_wild - this change is to support covering_fire functionality
            timespan_t duration = buffs.beast_cleave -> remains();
            for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( pets.main, pets.animal_companion ) )
            {
              pet -> active.stomp -> execute();
              if ( duration > 0_ms )
              {
                pet -> pets::hunter_pet_t::buffs.beast_cleave -> trigger( duration );
              }
            }
            for ( auto pet : ( pets.cotw_stable_pet.active_pets() ) )
            {
              pet -> active.stomp -> execute();
              if ( duration > 0_ms )
              {
                pet -> pets::hunter_pet_t::buffs.beast_cleave -> trigger( duration );
              }
            }
          }
        } );

  buffs.beast_cleave = 
    make_buff( this, "beast_cleave", find_spell( 268877 ) )
    -> apply_affecting_effect( talents.beast_cleave -> effectN( 2 ) );

  buffs.explosive_venom = 
    make_buff( this, "explosive_venom", find_spell( 459689 ) )
    -> set_default_value_from_effect( 1 );

  buffs.a_murder_of_crows = 
    make_buff( this, "a_murder_of_crows", talents.a_murder_of_crows->effectN( 1 ).trigger() );

  buffs.huntmasters_call = 
    make_buff( this, "huntmasters_call", find_spell( 459731 ) );

  buffs.summon_fenryr = 
    make_buff( this, "summon_fenryr", find_spell ( 459735 ) )
    -> modify_duration( talents.dire_frenzy -> effectN( 1 ).time_value() )
    -> set_default_value_from_effect( 2 )
    -> set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buffs.summon_hati = 
    make_buff( this, "summon_hati", find_spell( 459738 ) )
      -> modify_duration( talents.dire_frenzy -> effectN( 1 ).time_value() )
      -> set_default_value_from_effect( 2 );

  // Survival

  buffs.bloodseeker =
    make_buff( this, "bloodseeker", find_spell( 260249 ) )
      -> set_default_value_from_effect( 1 )
      -> add_invalidate( CACHE_AUTO_ATTACK_SPEED );

  buffs.tip_of_the_spear =
    make_buff( this, "tip_of_the_spear", talents.tip_of_the_spear_buff )
      -> set_chance( talents.tip_of_the_spear.ok() );

  buffs.tip_of_the_spear_explosive =
    make_buff( this, "tip_of_the_spear_explosive", talents.tip_of_the_spear_explosive_buff )
      -> set_chance( talents.tip_of_the_spear.ok() );

  buffs.sulfur_lined_pockets =
    make_buff( this, "sulfur_lined_pockets", find_spell( 459830 ) )
      ->set_chance( talents.sulfur_lined_pockets.ok() );

  buffs.sulfur_lined_pockets_explosive =
    make_buff( this, "sulfur_lined_pockets_explosive", find_spell( 459834 ) )
      ->set_chance( talents.sulfur_lined_pockets.ok() );

  buffs.terms_of_engagement =
    make_buff( this, "terms_of_engagement", find_spell( 265898 ) )
      -> set_default_value_from_effect( 1, 1 / 5.0 )
      -> set_affects_regen( true );

  buffs.merciless_blows = make_buff( this, "merciless_blows", find_spell( 459870 ) )
      ->set_default_value_from_effect( 1 )
      ->set_chance( talents.merciless_blows.ok() );

  buffs.aspect_of_the_eagle =
    make_buff( this, "aspect_of_the_eagle", find_spell( 186289 ) )
      -> set_cooldown( 0_ms );

  buffs.mongoose_fury =
    make_buff( this, "mongoose_fury", find_spell( 259388 ) )
      -> set_default_value_from_effect( 1 )
      -> set_refresh_behavior( buff_refresh_behavior::DISABLED );

  buffs.coordinated_assault =
    make_buff( this, "coordinated_assault", talents.coordinated_assault )
      ->set_default_value( talents.coordinated_assault->effectN( 1 ).percent() )
      ->set_cooldown( 0_ms );

  if ( talents.bombardier.ok() )
    buffs.coordinated_assault->set_stack_change_callback( [ this ]( buff_t*, int /*old*/, int cur ) {
      if ( cur == 0 )
      {
        buffs.bombardier->trigger();
        cooldowns.explosive_shot->reset( true );
      }
    } );

  buffs.relentless_primal_ferocity =
    make_buff( this, "relentless_primal_ferocity", talents.relentless_primal_ferocity_buff )
      ->set_default_value_from_effect( 1 )
      ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
      ->set_duration( talents.coordinated_assault->duration() )
      ->set_chance( talents.relentless_primal_ferocity.ok() )
      ->set_quiet( true );

  buffs.exposed_flank = 
    make_buff( this, "exposed_flank", find_spell( 459864 ) )
      ->set_default_value( find_spell( 459864 )->effectN( 1 ).base_value() )
      ->set_chance( talents.exposed_flank.ok() );

  buffs.sic_em = 
    make_buff( this, "sic_em", find_spell( 461409 ) )
      ->set_default_value( find_spell( 461409 )->effectN( 2 ).base_value() )
      ->set_chance( talents.sic_em.ok() );

  buffs.bombardier = 
    make_buff( this, "bombardier", talents.bombardier_buff )
      ->set_reverse( true );

  // Pet family buffs

  buffs.endurance_training =
    make_buff( this, "endurance_training", find_spell( 264662 ) )
      -> set_default_value_from_effect( 2 )
      -> apply_affecting_aura( talents.aspect_of_the_beast )
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
      -> apply_affecting_aura( talents.aspect_of_the_beast )
      -> add_invalidate( CACHE_RUN_SPEED );

  buffs.predators_thirst =
    make_buff( this, "predators_thirst", find_spell( 264663 ) )
      -> set_default_value_from_effect( 2 )
      -> apply_affecting_aura( talents.aspect_of_the_beast )
      -> add_invalidate( CACHE_LEECH );

  // Tier Set Bonuses
  buffs.harmonize =
    make_buff( this, "harmonize", find_spell( 457072 ) )
      -> set_default_value_from_effect( 1 );

  buffs.moving_target
    = make_buff( this, "moving_target", tier_set.tww_s1_mm_4pc_buff )
      -> set_default_value_from_effect( 1 );

  // Hero Talents
  buffs.vicious_hunt = 
    make_buff( this, "vicious_hunt", find_spell( 431917 ) )
      -> apply_affecting_aura( talents.pack_assault )
      -> set_initial_stack( 1 + as<int>( talents.pack_assault -> effectN( 1 ).base_value() ) )
      -> set_default_value_from_effect( 1 );

  buffs.howl_of_the_pack
    = make_buff( this, "howl_of_the_pack", find_spell( 462515 ) )
      -> set_default_value_from_effect( 1 );

  buffs.frenzied_tear 
    = make_buff( this, "frenzied_tear", find_spell( 447262 ) )
      -> set_default_value_from_effect( 1 );

  buffs.scattered_prey
    = make_buff( this, "scattered_prey", find_spell( 461866 ) )
      -> set_default_value_from_effect( 1 );

  buffs.furious_assault
    = make_buff( this, "furious_assault", find_spell( 448814 ) )
      -> set_default_value_from_effect( 4 );

  buffs.beast_of_opportunity
    = make_buff( this, "beast_of_opportunity", find_spell( 450143 ) )
      -> set_default_value_from_effect( 1 );

  buffs.eyes_closed = make_buff( this, "eyes_closed", talents.eyes_closed->effectN( 1 ).trigger() );

  buffs.black_arrow =
    make_buff( this, "black_arrow", talents.black_arrow_buff )
      ->set_default_value_from_effect( 2 )
      ->set_chance( talents.black_arrow.ok() && specialization() == HUNTER_MARKSMANSHIP );

  buffs.withering_fire =
    make_buff( this, "withering_fire", talents.withering_fire_buff )
      ->set_default_value_from_effect( 1 )
      ->set_chance( talents.withering_fire.ok() );
}

void hunter_t::init_gains()
{
  player_t::init_gains();

  gains.trueshot                  = get_gain( "Trueshot" );

  gains.barbed_shot               = get_gain( "Barbed Shot" );
  gains.dire_beast                = get_gain( "Dire Beast" );

  gains.terms_of_engagement       = get_gain( "Terms of Engagement" );

  gains.dark_empowerment          = get_gain( "Dark Empowerment" );
  gains.invigorating_pulse        = get_gain( "Invigorating Pulse" );
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

  if ( talents.calling_the_shots.ok() )
    procs.calling_the_shots = get_proc( "Calling the Shots" );

  if ( talents.dire_command.ok() )
    procs.dire_command = get_proc( "Dire Command" );

  if ( talents.wild_call.ok() )
    procs.wild_call = get_proc( "Wild Call" );

  if ( talents.sentinel.ok() )
  {
    procs.sentinel_stacks = get_proc( "Sentinel Stacks" );
    procs.sentinel_implosions = get_proc( "Sentinel Implosions" );
  }

  if ( talents.extrapolated_shots.ok() )
    procs.extrapolated_shots_stacks = get_proc( "Extrapolated Shots Stacks" );

  if ( talents.release_and_reload.ok() )
    procs.release_and_reload_stacks = get_proc( "Release and Reload Stacks" );

  if ( talents.crescent_steel.ok() )
    procs.crescent_steel_stacks = get_proc( "Crescent Steel Stacks" );

  if ( talents.overwatch.ok() )
    procs.overwatch_implosions = get_proc( "Overwatch Implosion" );
}

void hunter_t::init_rng()
{
  player_t::init_rng();
}

void hunter_t::init_scaling()
{
  player_t::init_scaling();

  scaling -> disable( STAT_STRENGTH );
}

void hunter_t::init_assessors()
{
  player_t::init_assessors();

  if ( talents.terms_of_engagement.ok() )
  {
    assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, [this]( result_amount_type, action_state_t* s ) {
      if ( s -> result_amount > 0 )
        get_target_data( s -> target ) -> damaged = true;
      return assessor::CONTINUE;
    } );
  }

  if ( talents.overwatch.ok() )
  {
    assessor_out_damage.add( assessor::TARGET_DAMAGE + 1, [ this ]( result_amount_type, action_state_t* s ) {
      hunter_td_t* target_data = get_target_data( s->target );
      if ( !target_data->sentinel_imploding && target_data->debuffs.sentinel->check() > 3 && s->target->health_percentage() < talents.overwatch->effectN( 1 ).base_value() )
      {
        sim->print_debug( "Damage to {} with {} Sentinel stacks at {}% triggers Overwatch", s->target->name(),
                          target_data->debuffs.sentinel->check(), s->target->health_percentage() );

        for ( player_t* t : sim->target_non_sleeping_list )
        {
          if ( t->is_enemy() && !t->demise_event )
          {
            hunter_td_t* td = get_target_data( t );
            if ( !td->sentinel_imploding )
            {
              procs.overwatch_implosions->occur();
              trigger_sentinel_implosion( td );
            }
          }
        }
      }
      return assessor::CONTINUE;
    } );
  }

  if ( talents.crescent_steel.ok() )
  {
    assessor_out_damage.add( assessor::TARGET_DAMAGE + 1, [ this ]( result_amount_type, action_state_t* s ) {
      hunter_td_t* target_data = get_target_data( s->target );
      if ( target_data->debuffs.crescent_steel->check() )
      {
        target_data->crescent_steel_damaged = true;
      }
      else if ( target_data->debuffs.sentinel->check() && s->target->health_percentage() < talents.crescent_steel->effectN( 1 ).base_value() )
      {
        target_data->crescent_steel_damaged = true;
        target_data->debuffs.crescent_steel->trigger();
      }
      return assessor::CONTINUE;
    } );
  }
}

void hunter_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras(action);

  action.apply_affecting_aura( specs.hunter );
  action.apply_affecting_aura( specs.beast_mastery_hunter );
  action.apply_affecting_aura( specs.marksmanship_hunter );
  action.apply_affecting_aura( specs.survival_hunter );
}

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
                  name(), main_hand_weapon.type );
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

      void trigger( action_t* a, action_state_t* state ) override
      {
        if ( state -> target -> health_percentage() >= threshold )
          return;

        dbc_proc_callback_t::trigger( a, state );
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

      void execute( action_t* a, action_state_t* s ) override
      {
        dbc_proc_callback_t::execute( a, s );

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

  if ( talents.sentinel.ok() )
  {
    struct sentinel_cb_t : public dbc_proc_callback_t
    {
      hunter_t* player;

      sentinel_cb_t( const special_effect_t& e, hunter_t* p )
        : dbc_proc_callback_t( p, e ), player( p )
      {
      }

      void execute( action_t* a, action_state_t* s ) override
      {
        dbc_proc_callback_t::execute( a, s );

        player->trigger_sentinel( s->target, false, player->procs.sentinel_stacks );
      }
    };

    auto const effect    = new special_effect_t( this );
    effect->name_str     = "sentinel";
    effect->spell_id     = talents.sentinel->id();
    effect->proc_flags2_ = PF2_ALL_HIT;
    special_effects.push_back( effect );

    auto cb = new sentinel_cb_t( *effect, this );
    cb->initialize();
  }
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

  buffs.lone_wolf->trigger();
  buffs.trigger_finger->trigger();
}

void hunter_t::combat_begin()
{
  if ( talents.bloodseeker.ok() && sim -> player_no_pet_list.size() > 1 )
  {
    make_repeating_event( *sim, 1_s, [ this ] { trigger_bloodseeker_update(); } );
  }

  if ( talents.basilisk_collar.ok() )
  {
    make_repeating_event( *sim, 1_s, [ this ] { trigger_basilisk_collar_update(); } );
  }

  if ( talents.outland_venom.ok() )
    make_repeating_event( *sim, talents.outland_venom_debuff->effectN( 2 ).period(),
                          [ this ] { trigger_outland_venom_update(); } );

  player_t::combat_begin();
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

double hunter_t::composite_melee_crit_chance() const
{
  double crit = player_t::composite_melee_crit_chance();

  crit += specs.critical_strikes -> effectN( 1 ).percent();
  crit += talents.keen_eyesight -> effectN( 1 ).percent();
  crit += buffs.unerring_vision -> stack() * buffs.unerring_vision -> data().effectN( 1 ).percent();

  return crit;
}

double hunter_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  crit += specs.critical_strikes->effectN( 1 ).percent();
  crit += talents.keen_eyesight->effectN( 1 ).percent();
  crit += buffs.unerring_vision -> stack() * buffs.unerring_vision -> data().effectN( 1 ).percent();

  return crit;
}

double hunter_t::composite_rating_multiplier( rating_e r ) const
{
  double rm = player_t::composite_rating_multiplier( r );

  switch ( r )
  {
    case RATING_MELEE_CRIT:
    case RATING_RANGED_CRIT:
    case RATING_SPELL_CRIT:
      rm *= 1.0 + talents.serrated_tips -> effectN( 1 ).percent();
      break;
    default:
      break;
  }

  return rm;
}

double hunter_t::composite_melee_auto_attack_speed() const
{
  double s = player_t::composite_melee_auto_attack_speed();

  if ( buffs.bloodseeker -> check() )
    s /= 1 + buffs.bloodseeker -> check_stack_value();

  if ( buffs.trigger_finger->check() )
    s /= 1 + buffs.trigger_finger->check_value();

  return s;
}

double hunter_t::composite_player_target_crit_chance( player_t* target ) const
{
  double crit = player_t::composite_player_target_crit_chance( target );

  auto td = get_target_data( target );
  crit += td->debuffs.outland_venom->check_stack_value();

  return crit;
}

double hunter_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_critical_damage_multiplier( s );

  if ( buffs.unerring_vision -> data().effectN( 2 ).has_common_school( s -> action -> school ) )
    m *= 1.0 + buffs.unerring_vision -> stack() * buffs.unerring_vision -> data().effectN( 2 ).percent();

  if ( talents.penetrating_shots -> effectN( 1 ).has_common_school( s -> action -> school ) )
    m *= 1.0 + talents.penetrating_shots -> effectN( 2 ).percent() * cache.attack_crit_chance();

  if ( buffs.howl_of_the_pack -> check() )
  {
    m *= 1.0 + buffs.howl_of_the_pack -> check_stack_value();
  }

  return m;
}

double hunter_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( talents.darkness_calls->effectN( 1 ).has_common_school( school ) )
    m *= 1.0 + talents.darkness_calls->effectN( 1 ).percent();

  return m;
}

double hunter_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double d = player_t::composite_player_target_multiplier( target, school );

  auto td = get_target_data( target );

  if ( td->debuffs.kill_zone->has_common_school( school ) )
    d *= 1 + td->debuffs.kill_zone->check_value();

  if ( td->debuffs.lunar_storm->has_common_school( school ) )
    d *= 1 + td->debuffs.lunar_storm->check_value();

  return d;
}

double hunter_t::composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s, guardian );

  if ( mastery.master_of_beasts->ok() )
    m *= 1.0 + cache.mastery_value();

  m *= 1 + specs.beast_mastery_hunter -> effectN( 3 ).percent();
  m *= 1 + specs.survival_hunter -> effectN( 3 ).percent();
  m *= 1 + specs.marksmanship_hunter -> effectN( 3 ).percent();

  m *= 1 + buffs.withering_fire->check_value();

  if ( !guardian )
  {
    if ( buffs.coordinated_assault->check() )
      m *= 1 + talents.coordinated_assault->effectN( 4 ).percent();

    m *= 1 + buffs.summon_hati -> check_value();
  }

  return m;
}

double hunter_t::composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const
{
  double m = player_t::composite_player_target_pet_damage_multiplier( target, guardian );

  if ( !guardian )
  {
    auto td = get_target_data( target ); 
    auto wi_debuff = td -> debuffs.wild_instincts;
    int stacks = wi_debuff -> stack();
    double amp_per_stack = wi_debuff -> data().effectN( 1 ).percent();
    m *= 1 + stacks * amp_per_stack;
  }

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
    break;
  default: break;
  }
}

void hunter_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( resources.is_infinite( RESOURCE_FOCUS ) )
    return;

  double total_regen = periodicity.total_seconds() * resource_regen_per_second( RESOURCE_FOCUS );
  if ( buffs.trueshot -> check() )
  {
    double regen = total_regen * talents.trueshot -> effectN( 6 ).percent();
    resource_gain( RESOURCE_FOCUS, regen, gains.trueshot );
    total_regen += regen;
  }

  if ( buffs.terms_of_engagement -> check() )
    resource_gain( RESOURCE_FOCUS, buffs.terms_of_engagement -> check_value() * periodicity.total_seconds(), gains.terms_of_engagement );
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

    auto add_gain = [&]( double mul, gain_t* gain ) {
      mul_sum += mul;
      amount *= 1.0 + mul;
      mul_gains[ mul_gains_count++ ] = { mul, gain };
    };

    if ( buffs.trueshot -> check() )
      add_gain( talents.trueshot -> effectN( 5 ).percent(), gains.trueshot );

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
  if ( attr == ATTR_AGILITY )
    return 0.05;

  return 0;
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
  add_option( opt_timespan( "hunter.pet_attack_speed", options.pet_attack_speed,
                            0.5_s, 4_s ) );
  add_option( opt_timespan( "hunter.pet_basic_attack_delay", options.pet_basic_attack_delay,
                            0_ms, 0.6_s ) );
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
