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
damage_affected_by parse_damage_affecting_aura( action_t* a, spell_data_ptr_t spell )
{
  damage_affected_by affected_by;
  for ( const spelleffect_data_t& effect : spell -> effects() )
  {
    if ( effect.type() != E_APPLY_AURA ||
         effect.subtype() != A_ADD_PCT_MODIFIER ||
         !a -> data().affected_by( effect ) )
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

} // namespace cd_waste

// ==========================================================================
// Hunter
// ==========================================================================

// in-game the buffs are actually 8 distinct spells, so the player can't get more than 8 simultaneously
constexpr unsigned BARBED_SHOT_BUFFS_MAX = 8;

// different types of Wildfire Infusion bombs
enum wildfire_infusion_e {
  WILDFIRE_INFUSION_SHRAPNEL = 0,
  WILDFIRE_INFUSION_PHEROMONE,
  WILDFIRE_INFUSION_VOLATILE,
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

struct hunter_t;

namespace pets
{
struct stable_pet_t;
struct call_of_the_wild_pet_t;
struct hunter_main_pet_t;
struct animal_companion_t;
struct dire_critter_t;
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
    buff_t* latent_poison;
    buff_t* latent_poison_injectors;
    buff_t* death_chakram;
    buff_t* stampede;
    buff_t* shredded_armor;
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

struct hunter_t final : public player_t
{
public:

  struct pets_t
  {
    pets::hunter_main_pet_t* main = nullptr;
    pets::animal_companion_t* animal_companion = nullptr;
    pets::dire_critter_t* dire_beast = nullptr;
    spawner::pet_spawner_t<pets::dire_critter_t, hunter_t> dc_dire_beast;
    spawner::pet_spawner_t<pets::call_of_the_wild_pet_t, hunter_t> cotw_stable_pet;

    pets_t( hunter_t* p ) : dc_dire_beast( "dire_beast_(dc)", p ), cotw_stable_pet( "call_of_the_wild_pet", p ) { }
  } pets;

  struct tier_sets_t {
    // T29 - Vault of the Incarnates
    spell_data_ptr_t t29_mm_2pc;
    spell_data_ptr_t t29_mm_4pc;
    spell_data_ptr_t t29_bm_2pc;
    spell_data_ptr_t t29_bm_4pc;
    spell_data_ptr_t t29_sv_2pc;
    spell_data_ptr_t t29_sv_4pc;
    spell_data_ptr_t t29_sv_4pc_buff;
    // T30 - Aberrus, the Shadowed Crucible
    spell_data_ptr_t t30_bm_2pc; 
    spell_data_ptr_t t30_bm_4pc; 
    spell_data_ptr_t t30_mm_2pc; 
    spell_data_ptr_t t30_mm_4pc; 
    spell_data_ptr_t t30_sv_2pc; 
    spell_data_ptr_t t30_sv_4pc; 
  } tier_set;

  // Buffs
  struct buffs_t
  {
    // Marksmanship Tree
    buff_t* precise_shots;
    buff_t* lone_wolf;
    buff_t* streamline;
    buff_t* in_the_rhythm;
    buff_t* deathblow;
    buff_t* razor_fragments;
    buff_t* trick_shots;
    buff_t* bombardment;
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

    // Beast Mastery Tree
    std::array<buff_t*, BARBED_SHOT_BUFFS_MAX> barbed_shot;
    buff_t* flamewakers_cobra_sting;
    buff_t* cobra_sting;
    buff_t* thrill_of_the_hunt;
    buff_t* dire_beast;
    buff_t* bestial_wrath;
    buff_t* hunters_prey;
    buff_t* aspect_of_the_wild;
    buff_t* call_of_the_wild;
    buff_t* dire_pack;

    // Survival Tree
    buff_t* tip_of_the_spear;
    buff_t* bloodseeker;
    buff_t* aspect_of_the_eagle;
    buff_t* terms_of_engagement;
    buff_t* mongoose_fury;
    buff_t* coordinated_assault;
    buff_t* coordinated_assault_empower;
    buff_t* spearhead;
    buff_t* deadly_duo;

    // Pet family buffs
    buff_t* endurance_training;
    buff_t* pathfinding;
    buff_t* predators_thirst;

    // Tier Set Bonuses
    buff_t* find_the_mark;
    buff_t* focusing_aim;
    buff_t* lethal_command;
    buff_t* bestial_barrage;

    buff_t* exposed_wound; 
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* kill_command;
    cooldown_t* kill_shot;

    cooldown_t* rapid_fire;
    cooldown_t* aimed_shot;
    cooldown_t* trueshot;

    cooldown_t* barbed_shot;
    cooldown_t* a_murder_of_crows;
    cooldown_t* bestial_wrath;
    cooldown_t* aspect_of_the_wild;

    cooldown_t* wildfire_bomb;
    cooldown_t* harpoon;
    cooldown_t* flanking_strike;
    cooldown_t* fury_of_the_eagle;
    cooldown_t* ruthless_marauder;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* trueshot;
    gain_t* legacy_of_the_windrunners;

    gain_t* barbed_shot;

    gain_t* terms_of_engagement;
    gain_t* coordinated_kill;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* calling_the_shots;
    proc_t* windrunners_guidance;

    proc_t* wild_call;
    proc_t* wild_instincts;
    proc_t* dire_command; 

    proc_t* t30_sv_4p; 
  } procs;

  // RPPM
  struct rppms_t
  {
    real_ppm_t* arctic_bola;
  } rppm;

  // Talents
  struct talents_t
  {
    // Hunter Tree
    spell_data_ptr_t kill_command;
    spell_data_ptr_t kill_shot;

    spell_data_ptr_t improved_kill_shot;

    spell_data_ptr_t tar_trap;

    spell_data_ptr_t improved_traps;
    spell_data_ptr_t born_to_be_wild;

    spell_data_ptr_t high_explosive_trap;

    spell_data_ptr_t beast_master;
    spell_data_ptr_t keen_eyesight;
    spell_data_ptr_t master_marksman;

    spell_data_ptr_t improved_kill_command;
    spell_data_ptr_t serrated_shots;
    spell_data_ptr_t arctic_bola;
    spell_data_ptr_t serpent_sting;

    spell_data_ptr_t alpha_predator;
    spell_data_ptr_t killer_instinct;
    spell_data_ptr_t steel_trap;
    spell_data_ptr_t stampede;
    spell_data_ptr_t death_chakram;
    spell_data_ptr_t barrage;
    spell_data_ptr_t explosive_shot;
    spell_data_ptr_t poison_injection;
    spell_data_ptr_t hydras_bite;

    // Shared
    spell_data_ptr_t wailing_arrow;

    // Marksmanship Tree
    spell_data_ptr_t aimed_shot;

    spell_data_ptr_t crack_shot;
    spell_data_ptr_t improved_steady_shot;
    spell_data_ptr_t precise_shots;

    spell_data_ptr_t rapid_fire;
    spell_data_ptr_t lone_wolf;
    spell_data_ptr_t chimaera_shot;

    spell_data_ptr_t streamline;
    spell_data_ptr_t killer_accuracy;
    spell_data_ptr_t hunters_knowledge;
    spell_data_ptr_t careful_aim;

    spell_data_ptr_t in_the_rhythm;
    spell_data_ptr_t surging_shots;
    spell_data_ptr_t deathblow;
    spell_data_ptr_t target_practice;
    spell_data_ptr_t focused_aim;

    spell_data_ptr_t multishot_mm;
    spell_data_ptr_t razor_fragments;
    spell_data_ptr_t tactical_reload;
    spell_data_ptr_t dead_eye;
    spell_data_ptr_t bursting_shot;

    spell_data_ptr_t trick_shots;
    spell_data_ptr_t bombardment;
    spell_data_ptr_t volley;
    spell_data_ptr_t steady_focus;
    spell_data_ptr_t serpentstalkers_trickery;
    spell_data_ptr_t quick_load; //NYI - When you fall below 40% heath, Bursting Shot's cooldown is immediately reset. This can only occur once every 25 sec.

    spell_data_ptr_t light_ammo;
    spell_data_ptr_t heavy_ammo;
    spell_data_ptr_t trueshot;
    spell_data_ptr_t lock_and_load;
    spell_data_ptr_t bullseye;

    spell_data_ptr_t bulletstorm;
    spell_data_ptr_t sharpshooter;
    spell_data_ptr_t eagletalons_true_focus;
    spell_data_ptr_t legacy_of_the_windrunners;

    spell_data_ptr_t salvo;
    spell_data_ptr_t calling_the_shots;
    spell_data_ptr_t unerring_vision;
    spell_data_ptr_t windrunners_barrage;
    spell_data_ptr_t readiness;
    spell_data_ptr_t windrunners_guidance;

    // Beast Mastery Tree
    spell_data_ptr_t cobra_shot;

    spell_data_ptr_t pack_tactics;
    spell_data_ptr_t multishot_bm;
    spell_data_ptr_t barbed_shot;

    spell_data_ptr_t aspect_of_the_beast;
    spell_data_ptr_t kindred_spirits;
    spell_data_ptr_t training_expert;

    spell_data_ptr_t animal_companion;
    spell_data_ptr_t beast_cleave;
    spell_data_ptr_t killer_command;
    spell_data_ptr_t sharp_barbs;

    spell_data_ptr_t cobra_sting;
    spell_data_ptr_t thrill_of_the_hunt;
    spell_data_ptr_t kill_cleave;
    spell_data_ptr_t a_murder_of_crows;
    spell_data_ptr_t bloodshed;
    spell_data_ptr_t cobra_senses;

    spell_data_ptr_t dire_beast;
    spell_data_ptr_t bestial_wrath;
    spell_data_ptr_t war_orders;

    spell_data_ptr_t hunters_prey;
    spell_data_ptr_t stomp;
    spell_data_ptr_t barbed_wrath;
    spell_data_ptr_t wild_call;
    spell_data_ptr_t aspect_of_the_wild;

    spell_data_ptr_t dire_command;
    spell_data_ptr_t scent_of_blood;
    spell_data_ptr_t one_with_the_pack;
    spell_data_ptr_t master_handler;
    spell_data_ptr_t snake_bite;

    spell_data_ptr_t dire_frenzy;
    spell_data_ptr_t brutal_companion;
    spell_data_ptr_t call_of_the_wild;

    spell_data_ptr_t dire_pack;
    spell_data_ptr_t piercing_fangs;
    spell_data_ptr_t killer_cobra;
    spell_data_ptr_t bloody_frenzy;
    spell_data_ptr_t wild_instincts;

    // Survival Tree
    spell_data_ptr_t raptor_strike;

    spell_data_ptr_t wildfire_bomb;
    spell_data_ptr_t tip_of_the_spear;

    spell_data_ptr_t ferocity;
    spell_data_ptr_t flankers_advantage;
    spell_data_ptr_t harpoon;

    spell_data_ptr_t energetic_ally;
    spell_data_ptr_t bloodseeker;
    spell_data_ptr_t aspect_of_the_eagle;
    spell_data_ptr_t terms_of_engagement;

    spell_data_ptr_t guerrilla_tactics;
    spell_data_ptr_t lunge;
    spell_data_ptr_t butchery;
    spell_data_ptr_t carve;
    spell_data_ptr_t mongoose_bite;

    spell_data_ptr_t intense_focus;
    spell_data_ptr_t improved_wildfire_bomb;
    spell_data_ptr_t frenzy_strikes;
    spell_data_ptr_t flanking_strike;
    spell_data_ptr_t spear_focus;

    spell_data_ptr_t vipers_venom;
    spell_data_ptr_t sharp_edges;
    spell_data_ptr_t sweeping_spear;
    spell_data_ptr_t tactical_advantage;
    spell_data_ptr_t bloody_claws;

    spell_data_ptr_t wildfire_infusion;
    spell_data_ptr_t quick_shot;
    spell_data_ptr_t coordinated_assault;
    spell_data_ptr_t killer_companion;

    spell_data_ptr_t fury_of_the_eagle;
    spell_data_ptr_t ranger;
    spell_data_ptr_t coordinated_kill;
    spell_data_ptr_t explosives_expert;
    spell_data_ptr_t spearhead;

    spell_data_ptr_t ruthless_marauder;
    spell_data_ptr_t birds_of_prey;
    spell_data_ptr_t bombardier;
    spell_data_ptr_t deadly_duo;
  } talents;

  // Specialization Spells
  struct specs_t
  {
    spell_data_ptr_t critical_strikes;
    spell_data_ptr_t hunter;
    spell_data_ptr_t beast_mastery_hunter;
    spell_data_ptr_t marksmanship_hunter;
    spell_data_ptr_t survival_hunter;

    // Hunter Tree
    spell_data_ptr_t freezing_trap;
    spell_data_ptr_t arcane_shot;
    spell_data_ptr_t steady_shot;
    spell_data_ptr_t flare;

    // Marksmanship/Beast Mastery Tree
    spell_data_ptr_t wailing_arrow;
  } specs;

  struct mastery_spells_t
  {
    spell_data_ptr_t master_of_beasts;
    spell_data_ptr_t sniper_training;
    spell_data_ptr_t spirit_bond;
  } mastery;

  struct {
    action_t* master_marksman = nullptr;
    action_t* barbed_shot = nullptr;
    action_t* latent_poison = nullptr;
    action_t* arctic_bola = nullptr;
    action_t* dire_command = nullptr; 
    action_t* windrunners_guidance_background = nullptr;
  } actions;

  cdwaste::player_data_t cd_waste;

  struct {
    events::tar_trap_aoe_t* tar_trap_aoe = nullptr;
    wildfire_infusion_e next_wi_bomb = WILDFIRE_INFUSION_SHRAPNEL;
    unsigned steady_focus_counter = 0;
    // Focus used for Calling the Shots (260404)
    double focus_used_CTS = 0;
    // Focus used for T30 Survival 4pc (405530)
    double focus_used_T30_sv_4p = 0; 
    // Last KC target used for T30 Survival 4pc (410167)
    player_t* last_kc_target;
    unsigned dire_pack_counter = 0;
    unsigned bombardment_counter = 0;
    unsigned lotw_counter = 0;
  } state;

  struct options_t {
    std::string summon_pet_str = "duck";
    timespan_t pet_attack_speed = 2_s;
    timespan_t pet_basic_attack_delay = 0.15_s;
    bool separate_wfi_stats = false;
    int dire_pack_start = 0;
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

    cooldowns.aimed_shot            = get_cooldown( "aimed_shot" );
    cooldowns.rapid_fire            = get_cooldown( "rapid_fire" );
    cooldowns.trueshot              = get_cooldown( "trueshot" );

    cooldowns.barbed_shot           = get_cooldown( "barbed_shot" );
    cooldowns.a_murder_of_crows     = get_cooldown( "a_murder_of_crows" );
    cooldowns.bestial_wrath         = get_cooldown( "bestial_wrath" );
    cooldowns.aspect_of_the_wild    = get_cooldown( "aspect_of_the_wild" );

    cooldowns.wildfire_bomb         = get_cooldown( "wildfire_bomb" );
    cooldowns.harpoon               = get_cooldown( "harpoon" );
    cooldowns.flanking_strike       = get_cooldown( "flanking_strike");
    cooldowns.fury_of_the_eagle     = get_cooldown( "fury_of_the_eagle" );
    cooldowns.ruthless_marauder     = get_cooldown( "ruthless_marauder" );

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
  double    composite_melee_speed() const override;
  double    composite_player_target_crit_chance( player_t* target ) const override;
  double    composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  double    composite_player_multiplier( school_e school ) const override;
  double    composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double    composite_player_pet_damage_multiplier( const action_state_t*, bool ) const override;
  double    composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const override;
  double    composite_leech() const override;
  double    matching_gear_multiplier( attribute_e attr ) const override;
  double    passive_movement_modifier() const override;
  void      invalidate_cache( cache_e ) override;
  void      regen( timespan_t periodicity ) override;
  double    resource_gain( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;
  void      reset_auto_attacks( timespan_t delay, proc_t* proc ) override;
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
  void trigger_t30_sv_4p( action_t* action, double cost );
  void trigger_calling_the_shots( action_t* action, double cost );
  void trigger_latent_poison( const action_state_t* s );
  void trigger_bloody_frenzy();
  void consume_trick_shots();
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
  maybe_bool triggers_t30_sv_4p; 

  struct {
    bool serrated_shots = false;
    // bm
    bool thrill_of_the_hunt = false;
    damage_affected_by bestial_wrath;
    damage_affected_by master_of_beasts;
    // mm
    bool bullseye_crit_chance = false;
    damage_affected_by lone_wolf;
    bool precise_shots = false;
    damage_affected_by sniper_training;
    bool t29_mm_4pc = false;
    // surv
    damage_affected_by spirit_bond;
    bool coordinated_assault = false;
    bool t29_sv_4pc_cost = false;
    damage_affected_by t29_sv_4pc_dmg;
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
        affected_by.serrated_shots = true;
    }

    affected_by.bullseye_crit_chance  = check_affected_by( this, p -> talents.bullseye -> effectN( 1 ).trigger() -> effectN( 1 ));
    affected_by.lone_wolf             = parse_damage_affecting_aura( this, p -> talents.lone_wolf );
    affected_by.sniper_training       = parse_damage_affecting_aura( this, p -> mastery.sniper_training );
    affected_by.t29_mm_4pc            = check_affected_by( this, p -> tier_set.t29_mm_4pc -> effectN( 1 ).trigger() -> effectN( 1 ) );

    affected_by.thrill_of_the_hunt    = check_affected_by( this, p -> talents.thrill_of_the_hunt -> effectN( 1 ).trigger() -> effectN( 1 ) );
    affected_by.bestial_wrath         = parse_damage_affecting_aura( this, p -> talents.bestial_wrath );
    affected_by.master_of_beasts      = parse_damage_affecting_aura( this, p -> mastery.master_of_beasts );

    affected_by.spirit_bond           = parse_damage_affecting_aura( this, p -> mastery.spirit_bond );
    affected_by.coordinated_assault   = check_affected_by( this, p -> find_spell( 361738 ) -> effectN( 2 ) );

    affected_by.t29_sv_4pc_cost       = check_affected_by( this, p -> tier_set.t29_sv_4pc_buff -> effectN( 1 ) );
    affected_by.t29_sv_4pc_dmg        = parse_damage_affecting_aura( this, p -> tier_set.t29_sv_4pc_buff );

    // Hunter Tree passives
    ab::apply_affecting_aura( p -> talents.improved_kill_shot );
    ab::apply_affecting_aura( p -> talents.improved_traps );
    ab::apply_affecting_aura( p -> talents.born_to_be_wild );
    ab::apply_affecting_aura( p -> talents.arctic_bola );
    ab::apply_affecting_aura( p -> talents.hydras_bite );

    // Marksmanship Tree passives
    ab::apply_affecting_aura( p -> talents.crack_shot );
    ab::apply_affecting_aura( p -> talents.streamline );
    ab::apply_affecting_aura( p -> talents.killer_accuracy );
    ab::apply_affecting_aura( p -> talents.hunters_knowledge );
    ab::apply_affecting_aura( p -> talents.surging_shots );
    ab::apply_affecting_aura( p -> talents.target_practice );
    ab::apply_affecting_aura( p -> talents.focused_aim );
    ab::apply_affecting_aura( p -> talents.dead_eye );
    ab::apply_affecting_aura( p -> talents.tactical_reload );

    // Beast Mastery Tree Passives
    ab::apply_affecting_aura( p -> talents.killer_command );
    ab::apply_affecting_aura( p -> talents.sharp_barbs );
    ab::apply_affecting_aura( p -> talents.war_orders );

    // Survival Tree Passives
    ab::apply_affecting_aura( p -> talents.terms_of_engagement );
    ab::apply_affecting_aura( p -> talents.guerrilla_tactics );
    ab::apply_affecting_aura( p -> talents.lunge );
    ab::apply_affecting_aura( p -> talents.improved_wildfire_bomb );
    ab::apply_affecting_aura( p -> talents.spear_focus );
    ab::apply_affecting_aura( p -> talents.sweeping_spear );
    ab::apply_affecting_aura( p -> talents.tactical_advantage );
    ab::apply_affecting_aura( p -> talents.ranger );
    ab::apply_affecting_aura( p -> talents.explosives_expert );

    // passive set bonuses
    ab::apply_affecting_aura( p -> tier_set.t29_bm_4pc );
    ab::apply_affecting_aura( p -> tier_set.t29_sv_2pc );

    ab::apply_affecting_aura( p -> tier_set.t30_bm_2pc );
    ab::apply_affecting_aura( p -> tier_set.t30_mm_2pc );
    ab::apply_affecting_aura( p -> tier_set.t30_mm_4pc );
    ab::apply_affecting_aura( p -> tier_set.t30_sv_2pc );
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

    if ( p() -> tier_set.t30_sv_4pc.ok() ) 
    {
      if ( triggers_t30_sv_4p.is_none() )
      {
        triggers_t30_sv_4p = !ab::background && !ab::proc && ab::base_cost() > 0;
      }
    }
    else
    {
      triggers_t30_sv_4p = false; 
    }

    if ( triggers_t30_sv_4p )
      ab::sim -> print_debug( "{} action {} set to proc T30 SV 4P", ab::player -> name(), ab::name() );
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
      p() -> trigger_calling_the_shots( this, ab::cost() );

    if ( affected_by.t29_sv_4pc_cost )
      p() -> buffs.bestial_barrage -> expire();
    
    if ( triggers_t30_sv_4p )
      p() -> trigger_t30_sv_4p( this, ab::cost() );
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
      am *= 1 + p() -> cache.mastery() * p() -> mastery.spirit_bond -> effectN( affected_by.spirit_bond.direct ).mastery_value();

    if ( affected_by.coordinated_assault )
      am *= 1 + p() -> buffs.coordinated_assault_empower -> check_value();

    if ( affected_by.t29_sv_4pc_dmg.direct && p() -> buffs.bestial_barrage -> check() )
      am *= 1 + p() -> tier_set.t29_sv_4pc_buff -> effectN( affected_by.t29_sv_4pc_dmg.direct ).percent();

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
      am *= 1 + p() -> cache.mastery() * p() -> mastery.spirit_bond -> effectN( affected_by.spirit_bond.tick ).mastery_value();

    if ( affected_by.t29_sv_4pc_dmg.tick && p() -> buffs.bestial_barrage -> check() )
      am *= 1 + p() -> tier_set.t29_sv_4pc_buff -> effectN( affected_by.t29_sv_4pc_dmg.tick ).percent();

    if ( affected_by.serrated_shots )
    {
      if ( s -> target -> health_percentage() < p() -> talents.serrated_shots -> effectN( 3 ).base_value() )
        am *= 1 + p() -> talents.serrated_shots -> effectN( 2 ).percent();
      else
        am *= 1 + p() -> talents.serrated_shots -> effectN( 1 ).percent();
    }

    return am;
  }

  double composite_crit_chance() const override
  {
    double cc = ab::composite_crit_chance();

    if ( affected_by.thrill_of_the_hunt )
      cc += p() -> buffs.thrill_of_the_hunt -> check_stack_value();

    if ( affected_by.bullseye_crit_chance )
      cc += p() -> buffs.bullseye -> check_stack_value();

    if ( affected_by.t29_mm_4pc )
      cc += p() -> buffs.focusing_aim -> check_value();

    return cc;
  }

  double cost() const override
  {
    double c = ab::cost();

    if ( affected_by.t29_sv_4pc_cost && p() -> buffs.bestial_barrage -> check() )
    { 
      c += p() -> tier_set.t29_sv_4pc_buff -> effectN( 1 ).base_value();
    }

    if ( c < 0 )
      return 0;

    return ceil( c );
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

struct hunter_ranged_attack_t: public hunter_action_t < ranged_attack_t >
{
  bool breaks_steady_focus = true;

  hunter_ranged_attack_t( util::string_view n, hunter_t* p,
                          const spell_data_t* s = spell_data_t::nil() ):
                          hunter_action_t( n, p, s )
  {
    affected_by.precise_shots = p -> talents.precise_shots.ok() &&
      parse_damage_affecting_aura( this, p -> find_spell( 260242 ) ).direct;
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
};

struct hunter_melee_attack_t: public hunter_action_t < melee_attack_t >
{
  hunter_melee_attack_t( util::string_view n, hunter_t* p,
                         const spell_data_t* s = spell_data_t::nil() ):
                         hunter_action_t( n, p, s )
  {}

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

  double composite_player_target_crit_chance( player_t* target ) const override
  {
    double crit = pet_t::composite_player_target_crit_chance( target );

    crit += o() -> get_target_data( target ) -> debuffs.stampede -> check_value();

    return crit;
  }

  void apply_affecting_auras( action_t& action ) override
  {
    pet_t::apply_affecting_auras(action);

    action.apply_affecting_aura( o() -> specs.hunter );
    action.apply_affecting_aura( o() -> specs.beast_mastery_hunter );
    action.apply_affecting_aura( o() -> specs.marksmanship_hunter );
    action.apply_affecting_aura( o() -> specs.survival_hunter );
  }

  hunter_t* o()             { return static_cast<hunter_t*>( owner ); }
  const hunter_t* o() const { return static_cast<hunter_t*>( owner ); }
};

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

    ab::apply_affecting_aura( o() -> talents.killer_companion );
    ab::apply_affecting_aura( o() -> talents.improved_kill_command );
    ab::apply_affecting_aura( o() -> tier_set.t29_bm_2pc );
    ab::apply_affecting_aura( o() -> talents.killer_command );
    ab::apply_affecting_aura( o() -> tier_set.t30_bm_2pc );
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


struct stable_pet_t : public hunter_pet_t
{
  struct buffs_t
  {
    buff_t* frenzy = nullptr;
    buff_t* beast_cleave = nullptr;
  } buffs;

  struct actives_t
  {
    action_t* beast_cleave = nullptr;
  } active;

  stable_pet_t( hunter_t* owner, util::string_view pet_name, pet_e pet_type ):
    hunter_pet_t( owner, pet_name, pet_type, false /* GUARDIAN */, true /* dynamic */ )
  {
    stamina_per_owner = 0.7;
    owner_coeff.ap_from_ap = 0.6;

    initial.armor_multiplier *= 1.05;

    main_hand_weapon.swing_time = owner -> options.pet_attack_speed;
  }

  void create_buffs() override
  {
    hunter_pet_t::create_buffs();

    buffs.frenzy =
      make_buff( this, "frenzy", o() -> find_spell( 272790 ) )
      -> set_default_value_from_effect( 1 )
      -> add_invalidate( CACHE_ATTACK_SPEED );

    buffs.beast_cleave =
      make_buff( this, "beast_cleave", find_spell( 118455 ) )
      // 3-24-23 Bloody Frenzy cleaves at 100% value when Beast Cleave is not talented.
      -> set_default_value( o() -> talents.beast_cleave.ok() ? o() -> talents.beast_cleave -> effectN( 1 ).percent() : 1.0 )
      -> apply_affecting_effect( o() -> talents.beast_cleave -> effectN( 2 ) );
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    hunter_pet_t::summon( duration );

    if (duration > 0_s)
      o() -> cooldowns.aspect_of_the_wild -> adjust( -o() -> talents.master_handler -> effectN( 1 ).time_value() );
  }

  double composite_melee_speed() const override
  {
    double ah = hunter_pet_t::composite_melee_speed();

    if ( buffs.frenzy -> check() )
      ah /= 1 + buffs.frenzy -> check_stack_value();

    return ah;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = hunter_pet_t::composite_player_multiplier( school );

    m *= 1 + o() -> talents.beast_master -> effectN( 1 ).percent();
    m *= 1 + o() -> talents.animal_companion -> effectN( 2 ).percent();
    m *= 1 + o() -> talents.training_expert -> effectN( 1 ).percent();

    return m;
  }

  void init_spells() override;
};

// Base class for main pet & Animal Companion
struct hunter_main_pet_base_t : public stable_pet_t
{
  struct actives_t
  {
    action_t* basic_attack = nullptr;
    action_t* brutal_companion_ba = nullptr;
    action_t* kill_command = nullptr;
    action_t* kill_cleave = nullptr;
    action_t* bestial_wrath = nullptr;
    action_t* stomp = nullptr;
    action_t* bloodshed = nullptr;

    action_t* flanking_strike = nullptr;
    action_t* coordinated_assault = nullptr;
    action_t* spearhead = nullptr;
  } active;

  struct buffs_t
  {
    buff_t* thrill_of_the_hunt = nullptr;
    buff_t* bestial_wrath = nullptr;
    buff_t* piercing_fangs = nullptr;
    buff_t* lethal_command = nullptr;

    buff_t* bloodseeker = nullptr;
    buff_t* coordinated_assault = nullptr;
    buff_t* exposed_wound = nullptr; 
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

    buffs.thrill_of_the_hunt =
      make_buff( this, "thrill_of_the_hunt", find_spell( 312365 ) )
        -> set_default_value_from_effect( 1 )
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

    buffs.lethal_command =
      make_buff( this, "lethal_command", o() -> tier_set.t29_bm_4pc -> effectN( 3 ).trigger() )
      -> set_default_value_from_effect( 1 );

    buffs.exposed_wound = 
      make_buff( this, "exposed_wound", o() -> tier_set.t30_sv_2pc -> effectN( 2 ).trigger() )
        -> set_default_value_from_effect( 1 ); 
  }

  double composite_melee_speed() const override
  {
    double ah = stable_pet_t::composite_melee_speed();

    if ( buffs.bloodseeker && buffs.bloodseeker -> check() )
      ah /= 1 + buffs.bloodseeker -> check_stack_value();

    return ah;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = stable_pet_t::composite_player_multiplier( school );

    if ( buffs.bestial_wrath -> has_common_school( school ) )
      m *= 1 + buffs.bestial_wrath -> check_value();
    
    m *= 1 + o() -> talents.ferocity -> effectN( 1 ).percent();
    m *= 1 + o() -> buffs.spearhead -> check_value();

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
// Call of the Wild
// ==========================================================================

struct call_of_the_wild_pet_t final : public stable_pet_t
{
  call_of_the_wild_pet_t( hunter_t* owner ):
    stable_pet_t( owner, "call_of_the_wild_pet", PET_HUNTER )
  {
    resource_regeneration = regen_type::DISABLED;
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    stable_pet_t::summon( duration );

    if ( main_hand_attack )
      main_hand_attack -> execute();
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
  } dots;

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
      + o() -> talents.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS )
      + o() -> talents.energetic_ally -> effectN( 1 ).resource( RESOURCE_FOCUS );

    base_gcd = 1.5_s;

    resources.infinite_resource[RESOURCE_FOCUS] = o() -> resources.infinite_resource[RESOURCE_FOCUS];
  }

  void create_buffs() override
  {
    hunter_main_pet_base_t::create_buffs();

    buffs.bloodseeker =
      make_buff( this, "bloodseeker", o() -> find_spell( 260249 ) )
        -> set_default_value_from_effect( 1 )
        -> add_invalidate( CACHE_ATTACK_SPEED );

    buffs.coordinated_assault =
      make_buff( this, "coordinated_assault", o() -> find_spell( 361736 ) );
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
  }

  void demise() override
  {
    hunter_main_pet_base_t::demise();

    if ( o() -> pets.main == this )
    {
      o() -> pets.main = nullptr;

      spec_passive() -> expire();
      if ( ! sim -> event_mgr.canceled )
        o() -> buffs.lone_wolf -> trigger();
    }
    if ( o() -> pets.animal_companion )
      o() -> pets.animal_companion -> demise();
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

  if ( auto main_pet = o() -> pets.main ) // theoretically should always be there /shrug
  {
    const hunter_main_pet_td_t* td = main_pet -> find_target_data( target );
    if ( td && td -> dots.bloodshed -> is_ticking() && spells.bloodshed -> effectN( 2 ).has_common_school( school ) )
      m *= 1 + spells.bloodshed -> effectN( 2 ).percent();
  }

  return m;
}

// ==========================================================================
// Dire Critter
// ==========================================================================

struct dire_critter_t final : public hunter_pet_t
{
  unsigned dire_pack_threshold;

  struct dire_beast_melee_t: public hunter_pet_melee_t<dire_critter_t>
  {
    dire_beast_melee_t( dire_critter_t* p ):
      hunter_pet_melee_t( "dire_beast_melee", p )
    { }
  };

  dire_critter_t( hunter_t* owner, util::string_view n = "dire_beast" ):
    hunter_pet_t( owner, n, PET_HUNTER, true /* GUARDIAN */, true /* dynamic */ )
  {
    owner_coeff.ap_from_ap = 0.15;
    resource_regeneration = regen_type::DISABLED;

    dire_pack_threshold = as<int>( o() -> talents.dire_pack -> effectN( 1 ).base_value() );
  }

  void init_spells() override;

  void summon( timespan_t duration = 0_ms ) override
  {
    hunter_pet_t::summon( duration );

    o() -> buffs.dire_beast -> trigger();

    o() -> state.dire_pack_counter++;
    if ( o() -> state.dire_pack_counter == dire_pack_threshold )
    {
      o() -> state.dire_pack_counter = 0;
      o() -> cooldowns.kill_command -> reset( true );
      o() -> buffs.dire_pack -> trigger();
    }

    if ( main_hand_attack )
      main_hand_attack -> execute();

    o() -> cooldowns.aspect_of_the_wild -> adjust( -o() -> talents.master_handler -> effectN( 1 ).time_value() );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = hunter_pet_t::composite_player_multiplier( school );

    m *= 1 + o() -> talents.dire_frenzy -> effectN( 2 ).percent();

    // 11-10-22 Dire Beast - Damage increased by 400%.
    // 13-10-22 Dire Beast damage increased by 50%.
    m *= 6;

    return m;
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
  const timespan_t base_duration = p -> buffs.dire_beast -> buff_duration();
  const timespan_t swing_time = 2_s * p -> cache.attack_speed();
  double partial_attacks_per_summon = base_duration / swing_time;
  int base_attacks_per_summon = static_cast<int>( partial_attacks_per_summon );
  partial_attacks_per_summon -= static_cast<double>( base_attacks_per_summon );

  if ( p -> rng().roll( partial_attacks_per_summon ) )
    base_attacks_per_summon += 1;

  return { base_attacks_per_summon * swing_time, base_attacks_per_summon };
}

namespace actions
{

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
  } affected_by;

  hunter_main_pet_action_t( util::string_view n, hunter_main_pet_t* p, const spell_data_t* s = spell_data_t::nil() ):
                            ab( n, p, s ), affected_by()
  {
    affected_by.aspect_of_the_beast = parse_damage_affecting_aura( this, ab::o() -> talents.aspect_of_the_beast );
    affected_by.spirit_bond         = parse_damage_affecting_aura( this, ab::o() -> mastery.spirit_bond );
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
      am *= 1 + ab::o() -> cache.mastery() * ab::o() -> mastery.spirit_bond -> effectN( affected_by.spirit_bond.direct ).mastery_value();

    return am;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double am = ab::composite_ta_multiplier( s );

    if ( affected_by.spirit_bond.tick )
      am *= 1 + ab::o() -> cache.mastery() * ab::o() -> mastery.spirit_bond -> effectN( affected_by.spirit_bond.tick ).mastery_value();

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
    double percent = 0;
    double multiplier = 1;
    benefit_t* benefit = nullptr;
  } killer_instinct;

  kill_command_base_t( hunter_main_pet_base_t* p, const spell_data_t* s ):
    hunter_pet_action_t( "kill_command", p, s )
  {
    background = true;
    proc = true;

    base_dd_multiplier *= 1 + o() -> talents.alpha_predator -> effectN( 2 ).percent();

    if ( o() -> talents.killer_instinct.ok() )
    {
      killer_instinct.percent = o() -> talents.killer_instinct -> effectN( 2 ).base_value();
      killer_instinct.multiplier = 1 + o() -> talents.killer_instinct -> effectN( 1 ).percent();
      killer_instinct.benefit = o() -> get_benefit( "killer_instinct" );
    }
  }

  double composite_attack_power() const override
  {
    // Kill Command for both Survival & Beast Mastery uses player AP directly
    return o() -> cache.attack_power() * o() -> composite_attack_power_multiplier();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double am = hunter_pet_action_t::composite_target_multiplier( t );

    if ( killer_instinct.percent )
    {
      const bool active = t -> health_percentage() < killer_instinct.percent;
      killer_instinct.benefit -> update( active );
      if ( active )
        am *= killer_instinct.multiplier;
    }

    return am;
  }

  void impact( action_state_t* s ) override
  {
    hunter_pet_action_t::impact( s );

    if ( o() -> talents.master_marksman.ok() && s -> result == RESULT_CRIT )
    {
      double amount = s -> result_amount * o() -> talents.master_marksman -> effectN( 1 ).percent();
      if ( amount > 0 )
        residual_action::trigger( o() -> actions.master_marksman, s -> target, amount );
    }
  }
};

// Beast Cleave ==============================================================

struct beast_cleave_attack_t: public hunter_pet_action_t<stable_pet_t, melee_attack_t>
{
  beast_cleave_attack_t( stable_pet_t* p ):
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
    tl.erase( std::remove( tl.begin(), tl.end(), target ), tl.end() );

    return tl.size();
  }
};

static void trigger_beast_cleave( const action_state_t* s )
{
  if ( !s -> action -> result_is_hit( s -> result ) )
    return;

  if ( s -> action -> sim -> active_enemies == 1 )
    return;

  auto p = debug_cast<stable_pet_t*>( s -> action -> player );

  if ( !p -> buffs.beast_cleave -> up() )
    return;

  // Target multipliers do not replicate to secondary targets
  const double target_da_multiplier = ( 1.0 / s -> target_da_multiplier );

  const double amount = s -> result_total * p -> buffs.beast_cleave -> check_value() * target_da_multiplier;
  p -> active.beast_cleave -> execute_on_target( s -> target, amount );
}

// Kill Cleave ==============================================================

struct kill_cleave_t: public hunter_pet_action_t<hunter_main_pet_base_t, melee_attack_t>
{
  kill_cleave_t( hunter_main_pet_base_t* p ):
    hunter_pet_action_t( "kill_cleave", p, p -> find_spell( 389448 ) )
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
    tl.erase( std::remove( tl.begin(), tl.end(), target ), tl.end() );

    return tl.size();
  }
};

struct kill_command_bm_mm_t: public kill_command_base_t
{
  struct {
    double chance = 0;
  } dire_command;

  kill_command_bm_mm_t( hunter_main_pet_base_t* p ) :
    kill_command_base_t( p, p -> find_spell( 83381 ) )
  {
    attack_power_mod.direct = o() -> talents.kill_command -> effectN( 2 ).percent();
  }

  void execute() override
  {
    kill_command_base_t::execute();

    if ( rng().roll( dire_command.chance ) )
    {
      o() -> actions.dire_command -> execute(); 
      o() -> procs.dire_command -> occur();
    }

    p() -> buffs.lethal_command -> expire();
  }

  void impact( action_state_t* s ) override
  {
    kill_command_base_t::impact( s );

    if ( o() -> talents.kill_cleave.ok() && s -> action -> result_is_hit( s -> result ) &&
      s -> action -> sim -> active_enemies > 1 && p() -> stable_pet_t::buffs.beast_cleave -> up() )
    {
      const double target_da_multiplier = ( 1.0 / s -> target_da_multiplier );
      const double amount = s -> result_total * o() -> talents.kill_cleave -> effectN( 1 ).percent() * target_da_multiplier;
      p() -> active.kill_cleave -> execute_on_target( s -> target, amount );
    }
  }

  double action_multiplier() const override
  {
    double am = kill_command_base_t::action_multiplier();

    am *= 1 + p() -> buffs.lethal_command -> value();

    return am;
  }
};

struct kill_command_sv_t: public kill_command_base_t
{
  kill_command_sv_t( hunter_main_pet_base_t* p ):
    kill_command_base_t( p, p -> find_spell( 259277 ) )
  {
    attack_power_mod.direct = o() -> talents.kill_command -> effectN( 1 ).percent();
    attack_power_mod.tick = o() -> talents.bloodseeker -> effectN( 1 ).percent();

    if ( ! o() -> talents.bloodseeker.ok() )
      dot_duration = 0_ms;
  }

  void impact( action_state_t* s ) override
  {
    kill_command_base_t::impact( s );

    if( ! o() -> tier_set.t30_sv_4pc.ok() )
      return;

    if ( o() -> state.last_kc_target && o() -> state.last_kc_target != s -> target )
      o() -> get_target_data( o() -> state.last_kc_target ) -> debuffs.shredded_armor -> expire();

    o() -> state.last_kc_target = s -> target;
    o() -> get_target_data( s -> target ) -> debuffs.shredded_armor -> trigger();
  }
  
  void trigger_dot( action_state_t* s ) override
  {
    kill_command_base_t::trigger_dot( s );

    o() -> trigger_bloodseeker_update();
  }

  void last_tick( dot_t* d ) override
  {
    kill_command_base_t::last_tick( d );

    o() -> trigger_bloodseeker_update();
  }

  double action_multiplier() const override
  {
    double am = kill_command_base_t::action_multiplier();

    am *= 1 + o() -> buffs.deadly_duo -> stack_value();

    am *= 1 + o() -> buffs.exposed_wound -> value(); 

    return am;
  }
};

// Pet Melee ================================================================

struct pet_melee_t : public hunter_pet_melee_t<stable_pet_t>
{
  pet_melee_t( util::string_view n, stable_pet_t* p ):
    hunter_pet_melee_t( n, p )
  {
  }

  void impact( action_state_t* s ) override
  {
    hunter_pet_melee_t::impact( s );

    trigger_beast_cleave( s );
  }
};

// Pet Claw/Bite/Smack ======================================================

struct basic_attack_t : public hunter_main_pet_attack_t
{
  struct {
    double cost_pct = 0;
    double multiplier = 1;
    benefit_t* benefit = nullptr;
  } wild_hunt;

  basic_attack_t( hunter_main_pet_t* p, util::string_view n, util::string_view options_str ):
    hunter_main_pet_attack_t( n, p, p -> find_pet_spell( n ) )
  {
    parse_options( options_str );

    school = SCHOOL_PHYSICAL;
    attack_power_mod.direct = 1 / 3.0;

    auto wild_hunt_spell = p -> find_spell( 62762 );
    wild_hunt.cost_pct = 1 + wild_hunt_spell -> effectN( 2 ).percent();
    wild_hunt.multiplier = 1 + wild_hunt_spell -> effectN( 1 ).percent();
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

  void execute() override
  {
    hunter_main_pet_attack_t::execute();

    if ( p() -> buffs.coordinated_assault -> check() )
      o() -> buffs.coordinated_assault_empower -> trigger();
  }
};

struct brutal_companion_ba_t : public basic_attack_t
{
  brutal_companion_ba_t( hunter_main_pet_t* p, util::string_view n ):
    basic_attack_t( p, n, "" )
  {
    background = dual = true;
  }

  double action_multiplier() const override
  {
    double am = hunter_main_pet_attack_t::action_multiplier();

    am *= 1 + o() -> talents.brutal_companion -> effectN( 2 ).percent();

    return am;
  }

  double cost() const override
  {
    return 0;
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
  {
    return o() -> cache.attack_power() * o() -> composite_attack_power_multiplier();
  }
};

// Coordinated Assault ====================================================

struct coordinated_assault_t: public hunter_main_pet_attack_t
{
  coordinated_assault_t( hunter_main_pet_t* p ):
    hunter_main_pet_attack_t( "coordinated_assault", p, p -> find_spell( 360969 ) )
  {
    background = true;
  }
};

// Spearhead ====================================================

struct spearhead_t: public hunter_main_pet_attack_t
{
  spearhead_t( hunter_main_pet_t* p ):
    hunter_main_pet_attack_t( "spearhead", p, p -> find_spell( 378957 ) )
  {
    background = true;
  }
};

// Stomp ===================================================================

struct stomp_t : public hunter_pet_action_t<hunter_pet_t, attack_t>
{
  stomp_t( hunter_pet_t* p ):
    hunter_pet_action_t( "stomp", p, p -> find_spell( 201754 ) )
  {
    aoe = -1;
    base_dd_multiplier *= o() -> talents.stomp -> effectN( 1 ).base_value();
  }

  void impact( action_state_t* s ) override
  {
    hunter_pet_action_t::impact( s );
  }
};

// Bloodshed ===============================================================

struct bloodshed_t : hunter_main_pet_attack_t
{
  bloodshed_t( hunter_main_pet_t* p ):
    hunter_main_pet_attack_t( "bloodshed", p, p -> spells.bloodshed )
  {
    background = true;
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
  bestial_wrath_t( hunter_main_pet_base_t* p ):
    hunter_pet_action_t( "bestial_wrath", p, p -> find_spell( 344572 ) )
  {
    background = true;
  }

  void impact( action_state_t* s ) override
  {
    hunter_pet_action_t::impact( s );
  }
};

} // end namespace pets::actions

hunter_main_pet_td_t::hunter_main_pet_td_t( player_t* target, hunter_main_pet_t* p ):
  actor_target_data_t( target, p )
{
  dots.bloodseeker = target -> get_dot( "kill_command", p );
  dots.bloodshed   = target -> get_dot( "bloodshed", p );
}

// hunter_pet_t::create_action ==============================================

action_t* hunter_main_pet_t::create_action( util::string_view name,
                                            util::string_view options_str )
{
  if ( name == "claw" ) return new        actions::basic_attack_t( this, "Claw", options_str );
  if ( name == "bite" ) return new        actions::basic_attack_t( this, "Bite", options_str );
  if ( name == "smack" ) return new       actions::basic_attack_t( this, "Smack", options_str );

  return hunter_main_pet_base_t::create_action( name, options_str );
}

// hunter_pet_t::init_spells ====================================================

void stable_pet_t::init_spells()
{
  hunter_pet_t::init_spells();

  main_hand_attack = new actions::pet_melee_t( "melee", this );

  if ( o() -> talents.beast_cleave.ok() || o() -> talents.bloody_frenzy.ok() )
    active.beast_cleave = new actions::beast_cleave_attack_t( this );
}

void hunter_main_pet_base_t::init_spells()
{
  stable_pet_t::init_spells();

  if ( o() -> specialization() == HUNTER_BEAST_MASTERY || o() -> specialization() == HUNTER_MARKSMANSHIP )
    active.kill_command = new actions::kill_command_bm_mm_t( this );
  else if ( o() -> specialization() == HUNTER_SURVIVAL )
    active.kill_command = new actions::kill_command_sv_t( this );

  spells.bloodshed = find_spell( 321538 );

  if ( o() -> specialization() == HUNTER_BEAST_MASTERY )
    active.bestial_wrath = new actions::bestial_wrath_t( this );

  if ( o() -> talents.kill_cleave.ok() )
    active.kill_cleave = new actions::kill_cleave_t( this );

  if ( o() -> talents.stomp.ok() )
    active.stomp = new actions::stomp_t( this );
}

void hunter_main_pet_t::init_spells()
{
  hunter_main_pet_base_t::init_spells();

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
}

void dire_critter_t::init_spells()
{
  hunter_pet_t::init_spells();

  main_hand_attack = new dire_beast_melee_t( this );
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

} // namespace events

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

} // namespace buffs

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

void hunter_t::trigger_t30_sv_4p( action_t* action, double cost )
{
  if ( !tier_set.t30_sv_4pc.ok() )
    return;

  state.focus_used_T30_sv_4p += cost; 
  sim -> print_debug( "{} action {} spent {} focus, t30 4p now at {}", name(), action->name(), cost, state.focus_used_T30_sv_4p );

  const double threshold = tier_set.t30_sv_4pc -> effectN( 1 ).base_value();
  if ( state.focus_used_T30_sv_4p >= threshold )
  {
    state.focus_used_T30_sv_4p -= threshold;
    cooldowns.wildfire_bomb -> adjust( -timespan_t::from_millis( tier_set.t30_sv_4pc -> effectN( 2 ).base_value() ) );
    procs.t30_sv_4p -> occur();
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

void hunter_t::trigger_latent_poison( const action_state_t* s )
{
  if ( !actions.latent_poison )
    return;

  auto td = find_target_data( s -> target );
  if ( !td )
    return;

  auto debuff = td -> debuffs.latent_poison;
  if ( ! debuff -> check() )
    return;

  actions.latent_poison -> execute_on_target( s -> target );

  debuff -> expire();
}

// 24-3-23: Currently only applying Beast Cleave with the remaining Call of the Wild duration as the result of a Cobra Shot, Serpent Sting, Arcant Shot, or Multi-Shot.
void hunter_t::trigger_bloody_frenzy()
{
  if ( !talents.bloody_frenzy.ok() || !buffs.call_of_the_wild -> check() )
    return;

  timespan_t duration = buffs.call_of_the_wild -> remains();
  for ( auto pet : pets::active<pets::stable_pet_t>( pets.main, pets.animal_companion ) )
    pet -> buffs.beast_cleave -> trigger( duration );

  for ( auto pet : pets.cotw_stable_pet.active_pets() )
    pet -> buffs.beast_cleave -> trigger( duration );
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

// ==========================================================================
// Hunter Attacks
// ==========================================================================

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

  auto_shot_t( hunter_t* p ) : auto_attack_base_t( "auto_shot", p, p -> find_spell( 75 ) )
  {
    wild_call_chance =
      p -> talents.wild_call -> effectN( 1 ).percent() +
      p -> talents.one_with_the_pack -> effectN( 1 ).percent();
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

    p() -> buffs.focusing_aim -> trigger();
  }

  double action_multiplier() const override
  {
    double am = auto_attack_base_t::action_multiplier();

    if ( player -> buffs.heavens_nemesis && player -> buffs.heavens_nemesis -> data().effectN( 1 ).subtype() != A_MOD_RANGED_AND_MELEE_ATTACK_SPEED )
      am *= 1 + player -> buffs.heavens_nemesis -> stack_value();

    return am;
  }
};

//==============================
// Shared attacks
//==============================

// Barrage ==================================================================

struct barrage_t: public hunter_spell_t
{
  struct damage_t final : public hunter_ranged_attack_t
  {
    damage_t( util::string_view n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> talents.barrage -> effectN( 1 ).trigger() )
    {
      aoe = -1;
      reduced_aoe_targets = data().effectN( 1 ).base_value();
    }

    void impact( action_state_t* s ) override
    {
      hunter_ranged_attack_t::impact( s );
    }
  };

  barrage_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "barrage", p, p -> talents.barrage )
  {
    parse_options( options_str );

    may_miss = may_crit = false;
    channeled = true;
    // 19-9-22 TODO: barrage is the only ability not counting toward cts
    triggers_calling_the_shots = false;

    tick_action = p -> get_background_action<damage_t>( "barrage_damage" );
    starved_proc = p -> get_proc( "starved: barrage" );
  }

  void schedule_execute( action_state_t* state = nullptr ) override
  {
    hunter_spell_t::schedule_execute( state );
  }
};

struct residual_bleed_base_t : public residual_action::residual_periodic_action_t<hunter_ranged_attack_t>
{
  residual_bleed_base_t( util::string_view n, hunter_t* p, const spell_data_t* s )
    : residual_periodic_action_t( n, p, s )
  {
    // Affected by Serrated Shots regardless of modifier attribute flags, so ignore any possible
    // application in hunter_action_t::composite_ta_multiplier since it's handled manually in below.
    affected_by.serrated_shots = false;
  }

  double base_ta(const action_state_t* s) const override
  {
    double ta = residual_periodic_action_t::base_ta( s );

    if ( s -> target -> health_percentage() < p() -> talents.serrated_shots -> effectN( 3 ).base_value() )
      ta *= 1 + p() -> talents.serrated_shots -> effectN( 2 ).percent();
    else
      ta *= 1 + p() -> talents.serrated_shots -> effectN( 1 ).percent();

    return ta;
  }
};

// Kill Shot =========================================================================

struct kill_shot_t : hunter_ranged_attack_t
{
  struct state_data_t
  {
    bool razor_fragments_up = false;
    bool coordinated_assault_empower_up = false;

    friend void sc_format_to( const state_data_t& data, fmt::format_context::iterator out ) {
      fmt::format_to( out, "razor_fragments_up={:d} coordinated_assault_empower_up={:d}", data.razor_fragments_up, data.coordinated_assault_empower_up );
    }
  };
  using state_t = hunter_action_state_t<state_data_t>;

  // Razor Fragments (Talent)
  struct razor_fragments_t : residual_bleed_base_t
  {
    double result_mod;

    razor_fragments_t( util::string_view n, hunter_t* p )
      : residual_bleed_base_t( n, p, p -> find_spell( 385638 ) )
    {
      result_mod = p -> find_spell( 388998 ) -> effectN( 3 ).percent();
      aoe = as<int>( p -> find_spell( 388998 ) -> effectN( 2 ).base_value() );
    }
  };

  // Coordinated Assault
  struct bleeding_gash_t : residual_bleed_base_t
  {
    double result_mod;

    bleeding_gash_t( util::string_view n, hunter_t* p )
      : residual_bleed_base_t( n, p, p -> find_spell( 361049 ) )
    {
      result_mod = p -> find_spell( 361738 ) -> effectN( 1 ).percent();
    }
  };

  double health_threshold_pct;
  razor_fragments_t* razor_fragments = nullptr;
  bleeding_gash_t* bleeding_gash = nullptr;

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

    if ( p -> talents.coordinated_assault.ok() )
    {
      bleeding_gash = p -> get_background_action<bleeding_gash_t>( "bleeding_gash" );
      add_child( bleeding_gash );
    }
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> buffs.deathblow -> decrement();
    p() -> buffs.razor_fragments -> decrement();

    p() -> buffs.hunters_prey -> decrement();

    if ( p() -> tier_set.t30_mm_4pc.ok() )
    {
      p() -> cooldowns.aimed_shot -> adjust( -timespan_t::from_millis( p() -> tier_set.t30_mm_4pc -> effectN( 2 ).base_value() ) );
      p() -> cooldowns.rapid_fire -> adjust( -timespan_t::from_millis( p() -> tier_set.t30_mm_4pc -> effectN( 2 ).base_value() ) );
    }

    if ( p() -> talents.bombardment.ok() )
      p() -> buffs.trick_shots -> trigger();
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

    // Buff is consumed on first impact but all hits (in the case of Birds of Prey) can trigger the bleed.
    p() -> buffs.coordinated_assault_empower -> expire();
    if ( bleeding_gash && debug_cast<state_t*>( s ) -> coordinated_assault_empower_up )
    {
      double amount = s -> result_amount * bleeding_gash -> result_mod;
      if ( amount > 0 )
        residual_action::trigger( bleeding_gash, s -> target, amount );
    }
  }

  int n_targets() const override
  {
    if ( p() -> talents.birds_of_prey.ok() && p() -> buffs.coordinated_assault -> check() )
      return 1 + as<int>( p() -> talents.birds_of_prey -> effectN( 1 ).base_value() );

    return hunter_ranged_attack_t::n_targets();
  }

  double cost() const override
  {
    return hunter_ranged_attack_t::cost();
  }

  bool target_ready( player_t* candidate_target ) override
  {
    return hunter_ranged_attack_t::target_ready( candidate_target ) &&
      ( candidate_target -> health_percentage() <= health_threshold_pct
        || p() -> buffs.deathblow -> check()
        || p() -> buffs.hunters_prey -> check()
        || ( p() -> talents.coordinated_kill.ok() && p() -> buffs.coordinated_assault -> check() ) );
  }

  double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    am *= 1 + p() -> buffs.razor_fragments -> check_value();

    return am;
  }

  double recharge_rate_multiplier( const cooldown_t& cd ) const override
  {
    double m = hunter_spell_t::recharge_rate_multiplier( cd );

    if ( p() -> buffs.coordinated_assault -> check() )
      m *= 1 + p() -> talents.coordinated_kill -> effectN( 3 ).percent();

    return m;
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  void snapshot_state( action_state_t* s, result_amount_type type ) override
  {
    hunter_ranged_attack_t::snapshot_state( s, type );
    debug_cast<state_t*>( s ) -> razor_fragments_up = p() -> buffs.razor_fragments -> check();
    debug_cast<state_t*>( s ) -> coordinated_assault_empower_up = p() -> buffs.coordinated_assault_empower -> check();
  }
};

// Arcane Shot ========================================================================

struct arcane_shot_t: public hunter_ranged_attack_t
{
  arcane_shot_t( hunter_t* p, util::string_view options_str ) :
    hunter_ranged_attack_t( "arcane_shot", p, p -> specs.arcane_shot )
  {
    parse_options( options_str );

    if ( p -> specialization() == HUNTER_MARKSMANSHIP )
      background = p -> talents.chimaera_shot.ok();
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> trigger_bloody_frenzy();

    p() -> buffs.precise_shots -> up(); // benefit tracking
    p() -> buffs.precise_shots -> decrement();

    p() -> buffs.focusing_aim -> expire();

    if ( rng().roll( p() -> tier_set.t30_mm_2pc -> proc_chance() ) ) 
    {
      p() -> buffs.deathblow -> trigger(); 
    }
  }

  double cost() const override
  {
    double c = hunter_ranged_attack_t::cost();

    if ( p() -> buffs.eagletalons_true_focus -> check() )
      c *= 1 + p() -> talents.eagletalons_true_focus -> effectN( 3 ).percent();

    return c;
  }

  void impact( action_state_t* state ) override
  {
    hunter_ranged_attack_t::impact( state );

    if ( state -> result == RESULT_CRIT )
      p() -> buffs.find_the_mark -> trigger();
  }
};

// Hit the Mark =========================================================================

struct hit_the_mark_t : residual_bleed_base_t
{
  double result_mod;

  hit_the_mark_t( util::string_view n, hunter_t* p)
    : residual_bleed_base_t( n, p, p -> find_spell( 394371 ) )
  {
    result_mod = p -> tier_set.t29_mm_2pc -> effectN( 1 ).trigger() -> effectN( 1 ).percent();
  }
};

// Wind Arrow =========================================================================

struct wind_arrow_t final : public hunter_ranged_attack_t
{
  struct {
    double multiplier = 0;
    double high, low;
  } careful_aim;
  hit_the_mark_t* hit_the_mark;
  struct {
    unsigned trigger_threshold;
    unsigned aimed_recharges;
    double focus_gain;
  } lotw;

  wind_arrow_t( util::string_view n, hunter_t* p ):
    hunter_ranged_attack_t( n, p, p -> find_spell( 191043 ) )
  {
    dual = true;
    // LotW arrows behave more like AiS re cast time/speed
    // TODO: RETEST for DL & test its behavior on lnl AiSes
    base_execute_time = p -> talents.aimed_shot -> cast_time();
    travel_speed = p -> talents.aimed_shot -> missile_speed();

    if ( p -> talents.careful_aim.ok() )
    {
      careful_aim.high = p -> talents.careful_aim -> effectN( 1 ).base_value();
      careful_aim.low = p -> talents.careful_aim -> effectN( 2 ).base_value();
      careful_aim.multiplier = p -> talents.careful_aim -> effectN( 3 ).percent();
    }

    if ( p -> tier_set.t29_mm_2pc.ok() )
    {
      hit_the_mark = p -> get_background_action<hit_the_mark_t>( "hit_the_mark" );
    }

    lotw.trigger_threshold = as<int>( p -> talents.legacy_of_the_windrunners -> effectN( 2 ).base_value() );
    lotw.focus_gain = p -> talents.legacy_of_the_windrunners -> effectN( 3 ).base_value();
    lotw.aimed_recharges = as<int>( p -> talents.legacy_of_the_windrunners -> effectN( 4 ).base_value() );
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( p() -> talents.windrunners_guidance.ok() && rng().roll( p() -> talents.windrunners_guidance -> effectN( 1 ).percent() ) ) 
    {
      
      //12/05/2023: Windrunners Guidance can proc the class trinket, but only if Trueshot is not already running
      if(!p() -> buffs.trueshot -> up()) 
      {
        p() -> actions.windrunners_guidance_background -> execute();
      }
      p() -> buffs.trueshot -> trigger( p() -> talents.windrunners_guidance -> effectN( 2 ).time_value() );
      p() -> procs.windrunners_guidance -> occur();
    }

    if ( ++p() -> state.lotw_counter == lotw.trigger_threshold )
    {
      p() -> state.lotw_counter = 0;
      p() -> cooldowns.aimed_shot -> reset( true, lotw.aimed_recharges );
      p() -> resource_gain( RESOURCE_FOCUS, lotw.focus_gain, p() -> gains.legacy_of_the_windrunners, this );
    }
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

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    // 16-10-22 TODO: Wind Arrows are consuming MM 2pc buff
    if ( hit_the_mark )
    {
      double amount = s -> result_amount * p() -> buffs.find_the_mark -> check_value();
      if ( amount > 0 )
        residual_action::trigger( hit_the_mark, s -> target, amount );
      p() -> buffs.find_the_mark -> expire();
    }
  }
};

// Wailing Arrow =====================================================================

struct wailing_arrow_t: public hunter_ranged_attack_t
{
  struct {
    int primary_arrows = 0;
    int secondary_arrows = 0;
    wind_arrow_t* wind_arrow = nullptr;
  } windrunners_barrage;

  struct damage_t final : hunter_ranged_attack_t
  {
    int wind_arrows = 0;
    wind_arrow_t* wind_arrow = nullptr;

    damage_t( util::string_view n, hunter_t* p, wind_arrow_t* wind_arrow, int arrows ):
      hunter_ranged_attack_t( n, p, p -> find_spell( 354831 ) ), wind_arrows( arrows ), wind_arrow( wind_arrow )
    {
      aoe = -1;
      attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
      base_aoe_multiplier = data().effectN( 2 ).ap_coeff() / attack_power_mod.direct;

      dual = true;
    }

    void execute() override
    {
      hunter_ranged_attack_t::execute();

      if ( wind_arrow && execute_state && execute_state -> chain_target > 0)
      {
        int arrows = wind_arrows;
        int target = 1;
        std::vector<player_t*>& tl = target_list();
        while ( arrows-- > 0 )
        {
          wind_arrow -> execute_on_target( tl[ target++ ] );
          if ( target > execute_state -> chain_target )
            target = 1;
        }
      }
    }
  };

  wailing_arrow_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "wailing_arrow", p,
      p -> specs.wailing_arrow.ok() || p -> talents.wailing_arrow.ok() ? p -> find_spell( 355589 ) : spell_data_t::not_found() )
  {
    parse_options( options_str );

    if ( p -> talents.windrunners_barrage.ok() )
    {
      windrunners_barrage.primary_arrows = as<int>( p -> talents.windrunners_barrage -> effectN( 1 ).base_value() );
      windrunners_barrage.secondary_arrows = as<int>( p -> talents.windrunners_barrage -> effectN( 2 ).base_value() );
      windrunners_barrage.wind_arrow = p -> get_background_action<wind_arrow_t>( "windrunners_barrage" );
      add_child(windrunners_barrage.wind_arrow);
    }

    impact_action = p -> get_background_action<damage_t>( "wailing_arrow_damage", windrunners_barrage.wind_arrow, windrunners_barrage.secondary_arrows );
    impact_action -> stats = stats;
    stats -> action_list.push_back( impact_action );
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( windrunners_barrage.wind_arrow )
    {
      for ( int i = 0; i < windrunners_barrage.primary_arrows; i++ )
        windrunners_barrage.wind_arrow -> execute_on_target( target );
    }

    if ( p() -> talents.readiness.ok() ) {
      p() -> cooldowns.rapid_fire -> reset( false );
      p() -> cooldowns.aimed_shot -> reset( p() -> talents.readiness -> effectN( 2 ).base_value() );
    }
  }

  result_e calculate_result( action_state_t* ) const override { return RESULT_NONE; }
  double calculate_direct_amount( action_state_t* ) const override { return 0.0; }
};

// Explosive Shot  ====================================================================

struct explosive_shot_t : public hunter_ranged_attack_t
{
  struct damage_t final : hunter_ranged_attack_t
  {
    damage_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p -> find_spell( 212680 ) )
    {
      background = dual = true;
    }
  };

  explosive_shot_t( hunter_t* p, util::string_view options_str )
    : hunter_ranged_attack_t( "explosive_shot", p, p -> find_spell( 212431 ) )
  {
    parse_options( options_str );

    if ( !p -> talents.explosive_shot -> ok() )
      background = true;

    may_miss = may_crit = false;

    tick_action = p -> get_background_action<damage_t>( "explosive_shot_aoe" );

    tick_action -> aoe = -1;
    tick_action -> reduced_aoe_targets = data().effectN( 2 ).base_value();
  }

  timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const override
  {
    return dot -> time_to_next_tick() + triggered_duration;
  }
};

struct explosive_shot_background_t : public explosive_shot_t
{
  size_t targets = 0;

  explosive_shot_background_t( util::string_view, hunter_t* p ) : explosive_shot_t( p, "" )
  {
    dual = true;
    base_costs[ RESOURCE_FOCUS ] = 0;
  }
};

// Serpent Sting =====================================================================

struct serpent_sting_base_t: public hunter_ranged_attack_t
{
  serpent_sting_base_t( hunter_t* p, util::string_view options_str, const spell_data_t* s ) :
    hunter_ranged_attack_t( "serpent_sting", p, s )
  {
    parse_options( options_str );

    affected_by.serrated_shots = true;

    if ( p -> talents.hydras_bite.ok() )
      aoe = 1 + static_cast<int>( p -> talents.hydras_bite -> effectN( 1 ).base_value() );
  }

  void execute() override
  {
    // have to always reset target_cache because of smart targeting
    if ( is_aoe() )
      target_cache.is_valid = false;

    hunter_ranged_attack_t::execute();
  }

  void init() override
  {
    hunter_ranged_attack_t::init();

    if ( action_t* lpi = p() -> find_action( "latent_poison_injection" ) )
      add_child( lpi );

    if ( action_t* lp = p() -> find_action( "latent_poison" ) )
      add_child( lp );
  }

  void assess_damage( result_amount_type type, action_state_t* s ) override
  {
    hunter_ranged_attack_t::assess_damage( type, s );

    if ( s -> result_amount > 0 && p() -> talents.poison_injection.ok() )
      td( s -> target ) -> debuffs.latent_poison -> trigger();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = hunter_ranged_attack_t::composite_da_multiplier( s );

    if ( s -> target -> health_percentage() < p() -> talents.serrated_shots -> effectN( 3 ).base_value() )
      m *= 1 + p() -> talents.serrated_shots -> effectN( 2 ).percent();
    else
      m *= 1 + p() -> talents.serrated_shots -> effectN( 1 ).percent();

    return m;
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

struct serpent_sting_t final : public serpent_sting_base_t
{
  serpent_sting_t( hunter_t* p, util::string_view options_str ):
    serpent_sting_base_t( p, options_str, p -> talents.serpent_sting )
  {
  }

  void execute() override
  {
    serpent_sting_base_t::execute();

    p() -> trigger_bloody_frenzy();
  }
};

// Arctic Bola ===================================================================

struct arctic_bola_t final : hunter_spell_t
{
  arctic_bola_t( hunter_t* p ):
    hunter_spell_t( "arctic_bola", p, p -> talents.arctic_bola -> effectN( 3 ).trigger() )
  {
    background = true;
    aoe = as<int>( p -> talents.arctic_bola -> effectN( 1 ).base_value() );
  }
};

// Windrunner's Guidance =====================================================
struct windrunners_guidance_background_t : public hunter_spell_t
{
  windrunners_guidance_background_t( hunter_t* p):
    hunter_spell_t( "windrunners_guidance", p, p -> talents.windrunners_guidance )
  {
    background = true;
    dual = true;
    //Only set to harmful for the 10.1 class trinket effect
    harmful = true;
  }
};

// Latent Poison ==================================================

struct latent_poison_t final : hunter_spell_t
{
  latent_poison_t( hunter_t* p ):
    hunter_spell_t( "latent_poison", p, p -> find_spell( 378016 ) )
  {
    background = true;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = hunter_spell_t::composite_da_multiplier( s );

    m *= td( target ) -> debuffs.latent_poison -> check();

    return m;
  }
};

// Master Marksman ====================================================================

struct master_marksman_t : residual_bleed_base_t
{
  master_marksman_t( hunter_t* p ):
    residual_bleed_base_t( "master_marksman", p, p -> find_spell( 269576 ) )
  { }
};

namespace death_chakram
{
/**
 * A whole namespace for a single spell...
 *
 * The spell consists of at least 3 spells: (talent version in parentheses)
 *   325028 (375891) - main spell, does all damage on multiple targets
 *   361756 (375893) - values the main spell tooltip points to
 *   325037 (375893) - additional periodic damage done on single target
 *   325553 (375894) - energize (amount is in 325028 (375891))
 *
 * The behavior is different when it hits multiple or only a single target.
 *
 * Observations when it hits multiple targets:
 *   - only 325028 is in play
 *   - damage events after the initial travel time are not instant
 *   - all energizes happen on cast (7 total 325553 energize events)
 *   - the spell can "bounce" to dead targets
 *
 * Observations when there is a single target involved:
 *   - 325028 is only the initial hit
 *   - 325037 starts hitting ~850ms after the initial hit, doing 6 hits total
 *     every ~630ms [*]
 *   - only 1 325553 energize happens on cast, all others happen when 325037
 *     hit (sometimes with a slight desync)
 *
 * [*] period data for 325037 hitting the main target:
 *        Min    Max  Median    Avg  Stddev
 *       0.59  0.711   0.631  0.635  0.0242
 *
 * 2020-08-11 Additional tests performed by Ghosteld, Putro, Tirrill & Laquan:
 *   https://www.warcraftlogs.com/reports/F9ZKyQf7LWxD4vqJ
 * Observations:
 *   - aoe reach is 8 yards
 *   - bringing targets in / out of range does not affect the outcome (it will
 *     "hit" corpses if the target was alive on cast)
 *   - animation is completely decoupled from damage events (matters only for
 *     the multi-target case)
 *   - damage calculation is dynamic only for 325037 (single-target case), on
 *     multi-target the damage is snapshot on cast (like a regular aoe)
 *   - target list building has a bunch of range-related quirks
 *
 * Current theory is:
 * The spell builds the full target list on cast.
 * If there are at least 2 available targets it builds "some" path between them
 * to get 7 "bounces" total. After that it works almost exactly like a typical
 * aoe spell. The only quirk is that all 7 hits don't always happen at the same
 * time, but there is no sane pattern to the delays (there are 3 mostly discrete
 * delays though: 0, 100 & 200 ms).
 * If there is only 1 available target, it hits it as usual, once, and schedules
 * 6 hits of 325037.
 *
 * Somewhat simplified (in multi-target) modeling of the theory for now:
 *  - in single-target case - let the main spell hit as usual (once), then
 *    schedule 1 "secondary" hit after the initial delay which schedules
 *    another 5 repeating hits
 *  - in multi-target case - just model at as a simple "instant" chained aoe
 */

static constexpr timespan_t ST_FIRST_HIT_DELAY = 850_ms;
static constexpr timespan_t ST_HIT_DELAY = 630_ms;

struct base_t : hunter_ranged_attack_t
{
  base_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
    hunter_ranged_attack_t( n, p, s )
  {
    chain_multiplier = p -> talents.death_chakram -> effectN( 1 ).chain_multiplier();

    energize_type = action_energize::PER_HIT;
    energize_resource = RESOURCE_FOCUS;
    energize_amount = ( p -> talents.death_chakram -> effectN( 4 ).base_value() );
  }
};

struct single_target_t final : base_t
{
  int hit_number = 0;
  int max_hit_number;

  single_target_t( util::string_view n, hunter_t* p, int hits ):
    base_t( n, p, p -> find_spell( 375893 ) ),
    max_hit_number( hits )
  {
    dual = true;
  }

  double action_multiplier() const override
  {
    double am = base_t::action_ta_multiplier();

    am *= std::pow( chain_multiplier, hit_number );

    return am;
  }

  void trigger( player_t* target_, int hit_number_ )
  {
    hit_number = hit_number_;
    execute_on_target( target_ );
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    // Each bounce refreshes the debuff
    td( s -> target ) -> debuffs.death_chakram -> trigger();
  }
};

struct single_target_event_t final : public event_t
{
  single_target_t& action;
  player_t* target;
  int hit_number;

  single_target_event_t( single_target_t& action, player_t* target, timespan_t t, int n = 1 ) :
    event_t( *action.player -> sim , t ), action( action ), target( target ), hit_number ( n )
  { }

  const char* name() const override
  { return "Hunter-DeathChakram-SingleTarget"; }

  void execute() override
  {
    if ( target -> is_sleeping() )
      return;

    action.trigger( target, hit_number );

    hit_number += 1;
    if ( hit_number == action.max_hit_number )
      return;

    make_event<single_target_event_t>( sim(), action, target, ST_HIT_DELAY, hit_number );
  }
};

} // namespace death_chakram

struct death_chakram_t : death_chakram::base_t
{
  death_chakram::single_target_t* single_target;

  death_chakram_t( hunter_t* p, util::string_view options_str ):
    death_chakram::base_t( "death_chakram", p, p -> talents.death_chakram )
  {
    parse_options( options_str );

    radius = 8; // Tested on 2020-08-11
    aoe = data().effectN( 1 ).chain_target();

    single_target = p -> get_background_action<death_chakram::single_target_t>( "death_chakram_st", data().effectN( 1 ).chain_target() );

    may_crit = single_target -> may_crit;
    base_dd_min = single_target -> base_dd_min;
    base_dd_max = single_target -> base_dd_max;
    attack_power_mod.direct = single_target -> attack_power_mod.direct;
    school = single_target -> school;
  }

  void init() override
  {
    base_t::init();

    single_target -> gain = gain;
    single_target -> stats = stats;
    stats -> action_list.push_back( single_target );
  }

  void impact( action_state_t* s ) override
  {
    using namespace death_chakram;

    base_t::impact( s );

    if ( s -> n_targets == 1 )
    {
      // Hit only a single target, schedule the repeating single target hitter
      make_event<single_target_event_t>( *sim, *single_target, s -> target, ST_FIRST_HIT_DELAY );
    }

    td( s -> target ) -> debuffs.death_chakram -> trigger();
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    size_t tl_size = base_t::available_targets( tl );

    // If we have more than 1 target, simply repeat current targets until the
    // target cap. This won't work with distance_targeting_enabled, the list
    // would have to be rebuilt in check_distance_targeting() with range checks.
    if ( tl_size > 1 && tl_size < as<size_t>( aoe ) )
    {
      tl.resize( as<size_t>( aoe ) );
      do {
        std::copy_n( tl.begin(), std::min( tl_size, tl.size() - tl_size ), tl.begin() + tl_size );
        tl_size += tl_size;
      } while ( tl_size < tl.size() );
    }

    return tl.size();
  }
};

//==============================
// Beast Mastery attacks
//==============================

// Multi Shot =================================================================

struct multishot_bm_t: public hunter_ranged_attack_t
{
  multishot_bm_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "multishot", p, p -> talents.multishot_bm )
  {
    parse_options( options_str );

    aoe = -1;
    reduced_aoe_targets = data().effectN( 1 ).base_value();
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( p() -> talents.beast_cleave -> ok() )
      for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p() -> pets.main, p() -> pets.animal_companion ) )
        pet -> stable_pet_t::buffs.beast_cleave -> trigger();

    if ( p() -> tier_set.t30_bm_4pc.ok() )
    {
      p() -> cooldowns.bestial_wrath -> adjust( -timespan_t::from_millis( p() -> tier_set.t30_bm_4pc -> effectN( 1 ).base_value() ) );
    }

    p() -> trigger_bloody_frenzy();
  }
};

// Cobra Shot Attack =================================================================

struct cobra_shot_t: public hunter_ranged_attack_t
{
  struct serpent_sting_cotw_t final : public serpent_sting_base_t
  {
    serpent_sting_cotw_t( util::string_view, hunter_t* p ):
      serpent_sting_base_t( p, "", p -> find_spell( 271788 ) )
    {
      dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
      aoe = 0;
    }
  };

  const timespan_t kill_command_reduction;
  action_t* cotw_serpent_sting = nullptr;

  cobra_shot_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "cobra_shot", p, p -> talents.cobra_shot ),
    kill_command_reduction( -timespan_t::from_seconds( data().effectN( 3 ).base_value() ) + p -> talents.cobra_senses -> effectN( 1 ).time_value() )
  {
    parse_options( options_str );

    if ( p -> talents.bloody_frenzy.ok() )
      cotw_serpent_sting = p -> get_background_action<serpent_sting_cotw_t>( "serpent_sting_cotw" );
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( cotw_serpent_sting && p() -> buffs.call_of_the_wild -> up() )
      cotw_serpent_sting -> execute_on_target( target );

    if ( p() -> talents.killer_cobra.ok() && p() -> buffs.bestial_wrath -> check() )
      p() -> cooldowns.kill_command -> reset( true );

    if ( !dual )
      p() -> trigger_bloody_frenzy();
  }

  void schedule_travel( action_state_t* s ) override
  {
    hunter_ranged_attack_t::schedule_travel( s );

    p() -> cooldowns.kill_command -> adjust( kill_command_reduction );

    p() -> buffs.cobra_sting -> trigger();

    if ( p() -> rppm.arctic_bola -> trigger() )
      p() -> actions.arctic_bola -> execute_on_target( s -> target );

    if ( p() -> tier_set.t30_bm_4pc.ok() )
      p() -> cooldowns.bestial_wrath -> adjust( -p() -> tier_set.t30_bm_4pc -> effectN( 1 ).time_value() );
  }

  double cost() const override
  {
    double c = hunter_ranged_attack_t::cost();

    c += p() -> buffs.aspect_of_the_wild -> value();

    return c;
  }

  int n_targets() const override
  {
    if ( p() -> buffs.aspect_of_the_wild -> check() )
      return 1 + as<int>( p() -> talents.aspect_of_the_wild -> effectN( 1 ).base_value() );

    return hunter_ranged_attack_t::n_targets();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double am = hunter_ranged_attack_t::composite_da_multiplier( s );

    if ( p() -> buffs.aspect_of_the_wild -> check() )
      am *= 1 + p() -> talents.snake_bite -> effectN( 1 ).percent();

    return am;
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

      pet -> stable_pet_t::buffs.frenzy -> trigger();
      pet -> buffs.thrill_of_the_hunt -> trigger();
      pet -> buffs.lethal_command -> trigger();
    }

    auto pet = p() -> pets.main;
    if ( pet && pet -> stable_pet_t::buffs.frenzy -> check() == as<int>( p() -> talents.brutal_companion -> effectN( 1 ).base_value() ) )
    {
      pet -> active.brutal_companion_ba -> execute_on_target( target );
    }

    p() -> buffs.lethal_command -> trigger();
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    p() -> trigger_latent_poison( s );
  }
};

//==============================
// Marksmanship attacks
//==============================

// Chimaera Shot ======================================================================

struct chimaera_shot_t: public hunter_ranged_attack_t
{
  struct impact_t final : public hunter_ranged_attack_t
  {
    impact_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
      hunter_ranged_attack_t( n, p, s )
    {
      dual = true;

      if ( s -> id() == 344121 )
      {
        // "Secondary" frost damage multiplier
        base_multiplier *= p -> talents.chimaera_shot -> effectN( 1 ).percent();
      }
    }

    void impact( action_state_t* state ) override
    {
      hunter_ranged_attack_t::impact( state );

      if ( state -> result == RESULT_CRIT )
        p() -> buffs.find_the_mark -> trigger();
    }
  };

  impact_t* nature;
  impact_t* frost;

  chimaera_shot_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "chimaera_shot", p, p -> talents.chimaera_shot ),
    nature( p -> get_background_action<impact_t>( "chimaera_shot_nature", p -> find_spell( 344120 ) ) ),
    frost( p -> get_background_action<impact_t>( "chimaera_shot_frost", p -> find_spell( 344121 ) ) )
  {
    parse_options( options_str );

    aoe = 2;
    radius = 5;

    add_child( nature );
    add_child( frost );

    school = SCHOOL_FROSTSTRIKE; // Just so the report shows a mixture of the two colors.
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> buffs.precise_shots -> up(); // benefit tracking
    p() -> buffs.precise_shots -> decrement();

    p() -> buffs.focusing_aim -> expire();

    if ( rng().roll( p() -> tier_set.t30_mm_2pc -> proc_chance() ) ) 
    {
      p() -> buffs.deathblow -> trigger();
    }

  }

  double cost() const override
  {
    double c = hunter_ranged_attack_t::cost();

    if ( p() -> buffs.eagletalons_true_focus -> check() )
      c *= 1 + p() -> talents.eagletalons_true_focus -> effectN( 3 ).percent();

    return c;
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

struct aimed_shot_t : public hunter_ranged_attack_t
{
  struct serpent_sting_sst_t final : public serpent_sting_base_t
  {
    serpent_sting_sst_t( util::string_view /*name*/, hunter_t* p ):
      serpent_sting_base_t( p, "", p -> find_spell( 271788 ) )
    {
      dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
    }
  };

  struct {
    double multiplier = 0;
    double high, low;
  } careful_aim;
  const int trick_shots_targets;
  bool lock_and_loaded = false;
  struct {
    double chance = 0;
    proc_t* proc;
  } surging_shots;
  struct {
    int count = 0;
    double chance = 0;
    wind_arrow_t* action = nullptr;
  } legacy_of_the_windrunners;
  hit_the_mark_t* hit_the_mark = nullptr;
  serpent_sting_sst_t* serpentstalkers_trickery = nullptr;

  aimed_shot_t( hunter_t* p, util::string_view options_str ) :
    hunter_ranged_attack_t( "aimed_shot", p, p -> talents.aimed_shot ),
    trick_shots_targets( as<int>( p -> find_spell( 257621 ) -> effectN( 1 ).base_value()
      + p -> talents.light_ammo -> effectN( 1 ).base_value()
      + p -> talents.heavy_ammo -> effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );

    radius = 8;
    base_aoe_multiplier = p -> find_spell( 257621 ) -> effectN( 4 ).percent()
      + p -> talents.heavy_ammo -> effectN( 3 ).percent();

    triggers_calling_the_shots = false;

    if ( p -> talents.careful_aim.ok() )
    {
      careful_aim.high = p -> talents.careful_aim -> effectN( 1 ).base_value();
      careful_aim.low = p -> talents.careful_aim -> effectN( 2 ).base_value();
      careful_aim.multiplier = p -> talents.careful_aim -> effectN( 3 ).percent();
    }

    if ( p -> talents.serpentstalkers_trickery.ok() )
      serpentstalkers_trickery = p -> get_background_action<serpent_sting_sst_t>( "serpent_sting_sst" );

    if ( p -> talents.surging_shots.ok() )
    {
      surging_shots.chance = p -> talents.surging_shots -> proc_chance();
      surging_shots.proc = p -> get_proc( "Surging Shots Rapid Fire reset" );
    }

    if ( p -> talents.legacy_of_the_windrunners.ok() )
    {
      legacy_of_the_windrunners.count = as<int>( p -> talents.legacy_of_the_windrunners -> effectN( 1 ).base_value() );
      legacy_of_the_windrunners.chance = 1.0;
      legacy_of_the_windrunners.action = p -> get_background_action<wind_arrow_t>( "legacy_of_the_windrunners" );
      add_child( legacy_of_the_windrunners.action );
    }

    if ( p -> tier_set.t29_mm_2pc.ok() )
    {
      hit_the_mark = p -> get_background_action<hit_the_mark_t>( "hit_the_mark" );
      add_child( hit_the_mark );
    }
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

    double c = hunter_ranged_attack_t::cost();

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
      serpentstalkers_trickery -> execute_on_target( target );

    p() -> buffs.precise_shots -> trigger( 1 + rng().range( p() -> buffs.precise_shots -> max_stack() ) );
    p() -> buffs.deathblow -> trigger( -1, buff_t::DEFAULT_VALUE(), p() -> talents.deathblow -> proc_chance() );

    p() -> buffs.trick_shots -> up(); // benefit tracking
    p() -> consume_trick_shots();

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

    if ( rng().roll( legacy_of_the_windrunners.chance ) )
    {
      for ( int i = 0; i < legacy_of_the_windrunners.count; i++ )
        legacy_of_the_windrunners.action -> execute_on_target( target );
    }

    if ( p() -> rppm.arctic_bola -> trigger() )
      p() -> actions.arctic_bola -> execute_on_target( target );
  }

  timespan_t execute_time() const override
  {
    if ( p() -> buffs.lock_and_load -> check() )
      return 0_ms;

    auto et = hunter_ranged_attack_t::execute_time();

    if ( p() -> buffs.streamline -> check() )
      et *= 1 + p() -> buffs.streamline -> check_value();

    if ( p() -> buffs.trueshot -> check() )
      et *= 1 + p() -> buffs.trueshot -> check_value();

    return et;
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    // 10-10-22 TODO: only main target hit is triggering Latent Poison and MM 2pc
    if ( s -> chain_target == 0 ) {
      p() -> trigger_latent_poison( s );

      if ( hit_the_mark )
      {
        double amount = s -> result_amount * p() -> buffs.find_the_mark -> check_value();
        if ( amount > 0 )
          residual_action::trigger( hit_the_mark, s -> target, amount );
        p() -> buffs.find_the_mark -> expire();
      }
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

  int n_targets() const override
  {
    if ( p() -> buffs.trick_shots -> check() )
      return 1 + trick_shots_targets;
    return hunter_ranged_attack_t::n_targets();
  }

  bool usable_moving() const override
  {
    return false;
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
  struct damage_t final : public hunter_ranged_attack_t
  {
    const int trick_shots_targets;

    damage_t( util::string_view n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> find_spell( 257045 ) ),
      trick_shots_targets( as<int>( p -> find_spell( 257621 ) -> effectN( 3 ).base_value()
        + p -> talents.light_ammo -> effectN( 2 ).base_value()
        + p -> talents.heavy_ammo -> effectN( 2 ).base_value() ) )
    {
      dual = true;
      direct_tick = true;
      radius = 8;
      base_aoe_multiplier = p -> find_spell( 257621 ) -> effectN( 5 ).percent()
        + p -> talents.heavy_ammo -> effectN( 4 ).percent();

      // energize
      parse_effect_data( p -> find_spell( 263585 ) -> effectN( 1 ) );
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

      /* This is not mentioned anywhere but testing shows that Rapid Fire inside
       * Trueshot has a 50% chance of energizing twice. Presumably to account for the
       * fact that focus is integral and 1 * 1.5 = 1. It's still affected by the
       * generic focus gen increase from Trueshot as each energize gives 3 focus when
       * combined with Nesingwary's Trapping Apparatus buff.
       */
      if ( p() -> buffs.trueshot -> check() && rng().roll( .5 ) )
        p() -> resource_gain( RESOURCE_FOCUS, composite_energize_amount( execute_state ), p() -> gains.trueshot, this );
    }
  };

  damage_t* damage;
  int base_num_ticks;

  rapid_fire_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "rapid_fire", p, p -> talents.rapid_fire ),
    damage( p -> get_background_action<damage_t>( "rapid_fire_damage" ) ),
    base_num_ticks( as<int>( data().effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );

    may_miss = may_crit = false;
    channeled = reset_auto_attack = true;
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

    p() -> buffs.in_the_rhythm -> trigger();
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    // substract 1 here because RF has a tick at zero
    return ( base_num_ticks - 1 ) * tick_time( s );
  }

  double energize_cast_regen( const action_state_t* s ) const override
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

// Multi Shot =================================================================

struct multishot_mm_t: public hunter_ranged_attack_t
{
  explosive_shot_background_t* explosive = nullptr;

  multishot_mm_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "multishot", p, p -> talents.multishot_mm )
  {
    parse_options( options_str );

    aoe = -1;
    reduced_aoe_targets = p -> find_spell( 2643 ) -> effectN( 1 ).base_value();

    if ( p -> talents.salvo.ok() )
    {
      explosive = p -> get_background_action<attacks::explosive_shot_background_t>( "explosive_shot_salvo" );
      explosive -> targets = as<size_t>( p -> talents.salvo -> effectN( 1 ).base_value() );
    }
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> buffs.precise_shots -> up(); // benefit tracking
    p() -> buffs.precise_shots -> decrement();

    if ( ( p() -> talents.trick_shots.ok() && num_targets_hit >= p() -> talents.trick_shots -> effectN( 2 ).base_value() ) || p() -> buffs.bombardment -> up() )
      p() -> buffs.trick_shots -> trigger();

    if ( explosive && p() -> buffs.salvo -> check() )
    {
      std::vector<player_t*>& tl = target_list();
      size_t targets = std::min<size_t>( tl.size(), explosive -> targets );
      for ( size_t t = 0; t < targets; t++ )
        explosive -> execute_on_target( tl[ t ] );

      p() -> buffs.salvo -> expire();
    }

    p() -> buffs.focusing_aim -> expire();

    if ( rng().roll( p() -> tier_set.t30_mm_2pc -> proc_chance() ) ) 
    {
      p() -> buffs.deathblow -> trigger(); 
    }
  }

  double cost() const override
  {
    double c = hunter_ranged_attack_t::cost();

    if ( p() -> buffs.eagletalons_true_focus -> check() )
      c *= 1 + p() -> talents.eagletalons_true_focus -> effectN( 3 ).percent();

    return c;
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

    if ( state -> result == RESULT_CRIT )
      p() -> buffs.find_the_mark -> trigger();
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

    if( p() -> talents.lunge.ok() )
    {
      p() -> cooldowns.wildfire_bomb -> adjust( -timespan_t::from_millis( p() -> talents.lunge -> effectN( 3 ).base_value() ) );
    }
  }
};

// Internal Bleeding (Wildfire Infusion) ===============================================

struct internal_bleeding_t
{
  struct action_t final : hunter_ranged_attack_t
  {
    action_t( util::string_view n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> find_spell( 270343 ) )
    {
      dual = true;
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double am = hunter_spell_t::composite_ta_multiplier( s );

      auto td = p() -> find_target_data( s -> target ); 
      if( td )
      {
        am *= 1.0 + td -> debuffs.shredded_armor -> value();
      }

      return am;
    }    
  };
  action_t* action = nullptr;

  internal_bleeding_t( hunter_t* p )
  {
    if ( p -> talents.wildfire_infusion.ok() )
      action = p -> get_background_action<action_t>( "internal_bleeding" );
  }

  void trigger( const action_state_t* s )
  {
    if ( action && action -> td( s -> target ) -> dots.shrapnel_bomb -> is_ticking() )
      action -> execute_on_target( s -> target );
  }
};

// Raptor Strike (Base) ================================================================
// Shared part between Raptor Strike & Mongoose Bite

struct melee_focus_spender_t: hunter_melee_attack_t
{
  struct spearhead_bleed_t : residual_bleed_base_t
  {
    double result_mod;

    spearhead_bleed_t( util::string_view n, hunter_t* p)
      : residual_bleed_base_t( n, p, p -> find_spell( 389881 ) )
    {
      result_mod = p -> talents.spearhead -> effectN( 2 ).percent();
    }
  };

  struct latent_poison_injectors_t final : hunter_spell_t
  {
    latent_poison_injectors_t( util::string_view n, hunter_t* p ):
      hunter_spell_t( n, p, p -> find_spell( 336904 ) )
    {
    }

    void trigger( player_t* target )
    {
      auto debuff = td( target ) -> debuffs.latent_poison_injectors;
      if ( ! debuff -> check() )
        return;

      execute_on_target( target );

      debuff -> expire();
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = hunter_spell_t::composite_da_multiplier( s );

      m *= td( target ) -> debuffs.latent_poison_injectors -> check();

      return m;
    }
  };

  struct serpent_sting_vv_t final : public serpent_sting_base_t
  {
    serpent_sting_vv_t( util::string_view /*name*/, hunter_t* p ):
      serpent_sting_base_t( p, "", p -> find_spell( 271788 ) )
    {
      dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;

      // Viper's Venom is left out of Hydra's Bite target count modification.
      aoe = 0;
    }
  };

  internal_bleeding_t internal_bleeding;
  latent_poison_injectors_t* latent_poison_injectors = nullptr;
  spearhead_bleed_t* spearhead = nullptr;

  struct {
    double chance = 0;
    serpent_sting_vv_t* action;
  } vipers_venom;

  struct {
    double chance = 0;
    proc_t* proc;
  } rylakstalkers_strikes;

  melee_focus_spender_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
    hunter_melee_attack_t( n, p, s ),
    internal_bleeding( p )
  {
    if ( p -> talents.vipers_venom.ok() )
    {
      vipers_venom.chance = p -> talents.vipers_venom -> effectN( 1 ).percent();
      vipers_venom.action = p -> get_background_action<serpent_sting_vv_t>( "serpent_sting_vv" );
    }

    if ( p -> talents.spearhead.ok() )
      spearhead = p -> get_background_action<spearhead_bleed_t>( "spearhead_bleed" );
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    if ( rng().roll( vipers_venom.chance ) )
      vipers_venom.action -> execute_on_target( target );

    if ( rng().roll( rylakstalkers_strikes.chance ) )
    {
      p() -> cooldowns.wildfire_bomb -> reset( true );
      rylakstalkers_strikes.proc -> occur();
    }

    p() -> buffs.tip_of_the_spear -> expire();

    if ( p() -> rppm.arctic_bola -> trigger() )
      p() -> actions.arctic_bola -> execute_on_target( target );

    p() -> buffs.bestial_barrage -> trigger();

    if ( p() -> buffs.spearhead -> up() )
      p() -> buffs.deadly_duo -> trigger();
  }

  void impact( action_state_t* s ) override
  {
    hunter_melee_attack_t::impact( s );

    internal_bleeding.trigger( s );

    if ( latent_poison_injectors )
      latent_poison_injectors -> trigger( s -> target );

    p() -> trigger_latent_poison( s );

    if ( spearhead && p() -> buffs.spearhead -> up() )
    {
      double amount = s -> result_amount * spearhead -> result_mod;
      if ( amount > 0 )
        residual_action::trigger( spearhead, s -> target, amount );
    }
  }

  double action_multiplier() const override
  {
    double am = hunter_melee_attack_t::action_multiplier();

    am *= 1 + p() -> buffs.tip_of_the_spear -> stack_value();

    return am;
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

  mongoose_bite_base_t( util::string_view n, hunter_t* p, spell_data_ptr_t s ):
    melee_focus_spender_t( n, p, s )
  {
    for ( size_t i = 0; i < stats_.at_fury.size(); i++ )
      stats_.at_fury[ i ] = p -> get_proc( fmt::format( "bite_at_{}_fury", i ) );

    background = !p -> talents.mongoose_bite.ok();

    if ( spearhead && p -> talents.mongoose_bite.ok() )
      add_child( spearhead );
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
    mongoose_bite_base_t( "mongoose_bite_eagle", p, p -> find_spell( 265888 ) )
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
      hunter_melee_attack_t( n, p, p -> find_spell( 269752 ) )
    {
      background = true;
      dual = true;
    }
  };
  damage_t* damage;

  flanking_strike_t( hunter_t* p, util::string_view options_str ):
    hunter_melee_attack_t( "flanking_strike", p, p -> talents.flanking_strike ),
    damage( p -> get_background_action<damage_t>( "flanking_strike_damage" ) )
  {
    parse_options( options_str );

    base_teleport_distance  = data().max_range();
    movement_directionality = movement_direction_type::OMNI;

    add_child( damage );
  }

  void init_finished() override
  {
    for ( auto pet : p() -> pet_list )
      add_pet_stats( pet, { "flanking_strike" } );

    hunter_melee_attack_t::init_finished();
  }

  double energize_cast_regen( const action_state_t* ) const override
  {
    return damage -> composite_energize_amount( nullptr );
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    if ( p() -> main_hand_weapon.group() == WEAPON_2H )
      damage -> execute_on_target( target );

    if ( auto pet = p() -> pets.main )
      pet -> active.flanking_strike -> execute_on_target( target );
  }
};

// Carve (Base) ======================================================================
// Shared part between Carve & Butchery

struct carve_base_t: public hunter_melee_attack_t
{
  timespan_t wildfire_bomb_reduction;
  timespan_t wildfire_bomb_reduction_cap;
  internal_bleeding_t internal_bleeding;

  carve_base_t( util::string_view n, hunter_t* p, const spell_data_t* s, timespan_t wfb_reduction ):
    hunter_melee_attack_t( n, p, s ), internal_bleeding( p )
  {
    if ( p -> talents.frenzy_strikes.ok() )
      wildfire_bomb_reduction = wfb_reduction;
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    timespan_t reduction = std::min( wildfire_bomb_reduction * num_targets_hit, wildfire_bomb_reduction_cap );
    p() -> cooldowns.wildfire_bomb -> adjust( -reduction, true );
    p() -> cooldowns.flanking_strike -> adjust( -reduction, true );

    p() -> buffs.bestial_barrage -> trigger();
  }

  void impact( action_state_t* s ) override
  {
    hunter_melee_attack_t::impact( s );

    internal_bleeding.trigger( s );
  }

  double action_multiplier() const override
  {
    double am = hunter_melee_attack_t::action_multiplier();

    return am;
  }
};

// Carve =============================================================================

struct carve_t: public carve_base_t
{
  carve_t( hunter_t* p, util::string_view options_str ):
    carve_base_t( "carve", p, p -> talents.carve, p -> talents.carve -> effectN( 2 ).time_value() )
  {
    parse_options( options_str );

    if ( p -> talents.butchery.ok() )
      background = true;

    aoe = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();

    wildfire_bomb_reduction_cap = timespan_t::from_seconds( data().effectN( 3 ).base_value() );
  }
};

// Butchery ==========================================================================

struct butchery_t: public carve_base_t
{
  butchery_t( hunter_t* p, util::string_view options_str ):
    carve_base_t( "butchery", p, p -> talents.butchery, p -> talents.butchery -> effectN( 2 ).time_value() )
  {
    parse_options( options_str );

    aoe = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();

    wildfire_bomb_reduction_cap = timespan_t::from_seconds( data().effectN( 3 ).base_value() );
  }
};

// Raptor Strike =====================================================================

struct raptor_strike_base_t: public melee_focus_spender_t
{
  raptor_strike_base_t( util::string_view n, hunter_t* p, spell_data_ptr_t s ):
    melee_focus_spender_t( n, p, s )
  {
    background = p -> talents.mongoose_bite.ok();

    if ( spearhead && !p -> talents.mongoose_bite.ok() )
      add_child( spearhead );
  }
};

struct raptor_strike_t: public raptor_strike_base_t
{
  raptor_strike_t( hunter_t* p, util::string_view options_str ):
    raptor_strike_base_t( "raptor_strike", p, p -> talents.raptor_strike )
  {
    parse_options( options_str );
  }
};

struct raptor_strike_eagle_t: public raptor_strike_base_t
{
  raptor_strike_eagle_t( hunter_t* p, util::string_view options_str ):
    raptor_strike_base_t( "raptor_strike_eagle", p, p -> find_spell( 265189 ) )
  {
    parse_options( options_str );
  }
};

// Harpoon ==================================================================

struct harpoon_t: public hunter_melee_attack_t
{
  struct terms_of_engagement_t final : hunter_ranged_attack_t
  {
    terms_of_engagement_t( util::string_view n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> find_spell( 271625 ) )
    {
      dual = true;
    }

    void impact( action_state_t* s ) override
    {
      hunter_ranged_attack_t::impact( s );

      p() -> buffs.terms_of_engagement -> trigger();
    }
  };
  terms_of_engagement_t* terms_of_engagement = nullptr;

  harpoon_t( hunter_t* p, util::string_view options_str ):
    hunter_melee_attack_t( "harpoon", p, p -> talents.harpoon )
  {
    parse_options( options_str );

    harmful = false;
    base_teleport_distance  = data().max_range();
    movement_directionality = movement_direction_type::OMNI;

    if ( p -> talents.terms_of_engagement.ok() )
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
      terms_of_engagement -> execute_on_target( target );
  }

  bool ready() override
  {
    // XXX: disable this for now to actually make it usable without explicit apl support for movement
    //if ( p() -> current.distance_to_move < data().min_range() )
    //  return false;

    return hunter_melee_attack_t::ready();
  }
};

struct fury_of_the_eagle_t: public hunter_melee_attack_t
{
  struct fury_of_the_eagle_tick_t: public hunter_melee_attack_t
  {
    double health_threshold = 0;
    double crit_chance_bonus = 0;
    timespan_t ruthless_marauder_adjust = 0_ms;

    fury_of_the_eagle_tick_t( hunter_t* p, int aoe_cap ):
      hunter_melee_attack_t( "fury_of_the_eagle_tick", p, p -> talents.fury_of_the_eagle -> effectN( 1 ).trigger() )
    {
      aoe = -1;
      background = true;
      may_crit = true;
      radius = data().max_range();
      reduced_aoe_targets = aoe_cap;
      health_threshold = p -> talents.fury_of_the_eagle -> effectN( 4 ).base_value() + p -> talents.ruthless_marauder -> effectN( 1 ).base_value();
      crit_chance_bonus = p -> talents.fury_of_the_eagle -> effectN( 3 ).percent();

      // TODO 2-12-22 Ruthless Marauder also adds to crit rate.
      if ( p -> bugs )
        crit_chance_bonus += p -> talents.ruthless_marauder -> effectN( 1 ).percent();

      if ( p -> talents.ruthless_marauder )
        ruthless_marauder_adjust = p -> talents.ruthless_marauder -> effectN( 3 ).time_value();
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

  hunter_melee_attack_t* fote_tick;
  fury_of_the_eagle_t( hunter_t* p, util::string_view options_str ):
    hunter_melee_attack_t( "fury_of_the_eagle", p, p -> talents.fury_of_the_eagle ),
    fote_tick( new fury_of_the_eagle_tick_t( p, as<int>( data().effectN( 5 ).base_value() ) ) )
  {
    parse_options( options_str );

    channeled = true;
    tick_zero = true;

    add_child( fote_tick );
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();
  }

  void tick( dot_t* dot ) override
  {
    hunter_melee_attack_t::tick( dot );
    fote_tick -> execute_on_target( dot -> target );
  }
};

// Coordinated Assault ==============================================================

struct coordinated_assault_t: public hunter_melee_attack_t
{
  struct damage_t final : hunter_melee_attack_t
  {
    damage_t( util::string_view n, hunter_t* p ):
      hunter_melee_attack_t( n, p, p -> find_spell( 360969 ) )
    {
      dual = true;
    }
  };
  damage_t* damage;

  coordinated_assault_t( hunter_t* p, util::string_view options_str ):
    hunter_melee_attack_t( "coordinated_assault", p, p -> talents.coordinated_assault )
  {
    parse_options( options_str );

    base_teleport_distance  = data().max_range();
    movement_directionality = movement_direction_type::OMNI;

    damage = p -> get_background_action<damage_t>( "coordinated_assault_damage" );
    add_child( damage );
  }

  void init_finished() override
  {
    for ( auto pet : p() -> pet_list )
      add_pet_stats( pet, { "coordinated_assault" } );

    hunter_melee_attack_t::init_finished();
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    if ( p() -> main_hand_weapon.group() == WEAPON_2H )
      damage -> execute_on_target( target );

    p() -> buffs.coordinated_assault -> trigger();

    if ( auto pet = p() -> pets.main )
    {
      pet -> buffs.coordinated_assault -> trigger();
      pet -> active.coordinated_assault -> execute_on_target( target );
    }
  }
};

// Spearhead ==============================================================

struct spearhead_t: public hunter_melee_attack_t
{
  struct damage_t final : hunter_melee_attack_t
  {
    damage_t( util::string_view n, hunter_t* p ):
      hunter_melee_attack_t( n, p, p -> find_spell( 378957 ) )
    {
      dual = true;
    }
  };
  damage_t* damage;

  spearhead_t( hunter_t* p, util::string_view options_str ):
    hunter_melee_attack_t( "spearhead", p, p -> talents.spearhead )
  {
    parse_options( options_str );

    base_teleport_distance  = data().max_range();
    movement_directionality = movement_direction_type::OMNI;

    damage = p -> get_background_action<damage_t>( "spearhead_damage" );
    add_child( damage );
  }

  void init_finished() override
  {
    for ( auto pet : p() -> pet_list )
      add_pet_stats( pet, { "spearhead" } );

    hunter_melee_attack_t::init_finished();
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    if ( p() -> main_hand_weapon.group() == WEAPON_2H )
      damage -> execute_on_target( target );

    p() -> buffs.spearhead -> trigger();

    if ( auto pet = p() -> pets.main )
      pet -> active.spearhead -> execute_on_target( target );
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
    if ( ! candidate_target -> debuffs.casting || ! candidate_target -> debuffs.casting -> check() ) return false;
    return hunter_spell_t::target_ready( candidate_target );
  }
};

// A Murder of Crows ========================================================

struct a_murder_of_crows_t : public hunter_spell_t
{
  struct peck_t final : public hunter_ranged_attack_t
  {
    peck_t( util::string_view n, hunter_t* p ) :
      hunter_ranged_attack_t( n, p, p -> find_spell( 131900 ) )
    { }

    timespan_t travel_time() const override
    {
      return timespan_t::from_seconds( data().missile_speed() );
    }

    void impact( action_state_t* s ) override
    {
      hunter_ranged_attack_t::impact( s );
    }
  };

  a_murder_of_crows_t( hunter_t* p, util::string_view options_str ) :
    hunter_spell_t( "a_murder_of_crows", p, p -> talents.a_murder_of_crows )
  {
    parse_options( options_str );

    tick_action = p -> get_background_action<peck_t>( "crow_peck" );
    starved_proc = p -> get_proc( "starved: a_murder_of_crows" );
  }

  void execute() override
  {
    hunter_spell_t::execute();
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
    if ( ! pet && ! opt_disabled )
      pet = player -> find_pet( p() -> options.summon_pet_str );

    if ( ! pet && p() -> specialization() != HUNTER_MARKSMANSHIP )
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

// Trap =========================================================================
// Base Trap action

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

// Tar Trap =====================================================================

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

// Freezing Trap =====================================================================

struct freezing_trap_t : public trap_base_t
{
  freezing_trap_t( hunter_t* p, util::string_view options_str ) :
    trap_base_t( "freezing_trap", p, p -> specs.freezing_trap )
  {
    parse_options( options_str );
  }
};

// High Explosive Trap =====================================================================

struct high_explosive_trap_t : public trap_base_t
{
  struct damage_t final : hunter_ranged_attack_t
  {
    damage_t( util::string_view n, hunter_t* p ) : hunter_ranged_attack_t( n, p, p -> find_spell( 236777 ) )
    {
      aoe = -1;
      attack_power_mod.direct = data().effectN( 2 ).ap_coeff();
    }
  };

  high_explosive_trap_t( hunter_t* p, util::string_view options_str )
    : trap_base_t( "high_explosive_trap", p, p -> talents.high_explosive_trap )
  {
    parse_options( options_str );

    impact_action = p -> get_background_action<damage_t>( "high_explosive_trap_damage" );
  }
};

// Counter Shot ======================================================================

struct counter_shot_t: public interrupt_base_t
{
  counter_shot_t( hunter_t* p, util::string_view options_str ):
    interrupt_base_t( "counter_shot", p, p -> find_specialization_spell( "Counter Shot" ) )
  {
    parse_options( options_str );
  }
};

// Kill Command =============================================================

struct kill_command_t: public hunter_spell_t
{
  struct arcane_shot_qs_t final : public attacks::arcane_shot_t
  {
    arcane_shot_qs_t( util::string_view /*name*/, hunter_t* p ):
      arcane_shot_t( p, "" )
    {
      dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
    }
  };

  struct {
    double chance = 0;
    proc_t* proc = nullptr;
  } reset;

  struct {
    double chance = 0;
    arcane_shot_qs_t* action = nullptr;
  } quick_shot;

  struct {
    double chance = 0;
    proc_t* proc;
  } dire_command;

  kill_command_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "kill_command", p, p -> talents.kill_command )
  {
    parse_options( options_str );

    cooldown -> charges += as<int>( p -> talents.alpha_predator -> effectN( 1 ).base_value() );

    if ( p -> specialization() == HUNTER_SURVIVAL )
    {
      reset.chance = data().effectN( 2 ).percent() + p -> talents.flankers_advantage -> effectN( 1 ).percent();
      reset.proc = p -> get_proc( "Kill Command Reset" );

      energize_amount += p -> talents.intense_focus -> effectN( 1 ).base_value();

      if ( p -> talents.quick_shot.ok() )
      {
        quick_shot.chance = p -> talents.quick_shot -> effectN( 1 ).percent();
        quick_shot.action = p -> get_background_action<arcane_shot_qs_t>( "arcane_shot_qs" );
      }
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

    p() -> buffs.tip_of_the_spear -> trigger();

    if ( reset.chance != 0 )
    {
      double chance = reset.chance;

      if ( td( target ) -> dots.pheromone_bomb -> is_ticking() )
        chance += p() -> find_spell( 270323 ) -> effectN( 2 ).percent();

      chance += p() -> talents.bloody_claws -> effectN( 1 ).percent()
        * p() -> buffs.mongoose_fury -> check();

      if ( p() -> buffs.spearhead -> check() )
        chance += p() -> talents.spearhead -> effectN( 3 ).percent();

      if ( p() -> buffs.deadly_duo -> check() )
        chance += p() -> buffs.deadly_duo -> check() * p() -> talents.deadly_duo -> effectN( 3 ).percent();

      if ( rng().roll( chance ) )
      {
        reset.proc -> occur();
        cooldown -> reset( true );

        if ( rng().roll( quick_shot.chance ) )
          quick_shot.action -> execute_on_target( target );

        p() -> cooldowns.fury_of_the_eagle -> adjust( - p() -> talents.fury_of_the_eagle -> effectN( 2 ).time_value() );
        p() -> buffs.spearhead -> extend_duration( p(), p() -> talents.deadly_duo -> effectN( 2 ).time_value() );
      }
    }

    if ( rng().roll( dire_command.chance ) )
    {
      p() -> actions.dire_command -> execute();
      p() -> procs.dire_command -> occur();
    }

    p() -> buffs.flamewakers_cobra_sting -> up(); // benefit tracking
    p() -> buffs.flamewakers_cobra_sting -> decrement();

    p() -> buffs.cobra_sting -> up(); // benefit tracking
    p() -> buffs.cobra_sting -> decrement();

    if ( p() -> buffs.hunters_prey -> trigger() )
      p() -> cooldowns.kill_shot -> reset( true );

    if ( rng().roll( p() -> tier_set.t29_bm_2pc -> proc_chance() ) )
      p() -> cooldowns.barbed_shot -> reset( true );

    p() -> buffs.lethal_command -> expire();
    p() -> buffs.deadly_duo -> expire();
    p() -> buffs.exposed_wound -> expire();

    if ( p() -> tier_set.t30_bm_4pc.ok() )
    {
      p() -> cooldowns.bestial_wrath -> adjust( -timespan_t::from_millis( p() -> tier_set.t30_bm_4pc -> effectN( 1 ).base_value() ) );
    }
  }

  double cost() const override
  {
    double c = hunter_spell_t::cost();

    c *= 1 + p() -> buffs.flamewakers_cobra_sting -> check_value();
    c *= 1 + p() -> buffs.cobra_sting -> check_value();
    c *= 1 + p() -> buffs.dire_pack -> check_value();

    return c;
  }

  double recharge_rate_multiplier( const cooldown_t& cd ) const override
  {
    double m = hunter_spell_t::recharge_rate_multiplier( cd );

    m *= 1 + p() -> buffs.dire_pack -> check_value();

    return m;
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
      if ( ! p() -> talents.bloodseeker.ok() )
        return expr_t::create_constant( expression_str, 0_ms );

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
  dire_beast_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "dire_beast", p, p -> talents.dire_beast )
  {
    parse_options( options_str );

    harmful = false;
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

    sim -> print_debug( "Dire Beast summoned with {} autoattacks", base_attacks_per_summon );

    p() -> pets.dire_beast -> summon( summon_duration );
  }
};

// Dire Command =============================================================
struct dire_command_summon_t final : hunter_spell_t
{
  dire_command_summon_t( hunter_t* p ): 
    hunter_spell_t( "dire_command_summon", p, p -> find_spell( 219199 ) )
    {
      cooldown -> duration = 0_ms;
      track_cd_waste = false;
      background = true;
      harmful = false;
    }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> pets.dc_dire_beast.spawn( pets::dire_beast_duration( p() ).first );
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

    //Specifically set to true for 10.1 class trinket 
    harmful = true;

    precast_time = clamp( precast_time, 0_ms, data().duration() );
  }

  void init_finished() override
  {
    for ( auto pet : p() -> pet_list )
      add_pet_stats( pet, { "bestial_wrath" } );

    hunter_spell_t::init_finished();

    //Used to ensure that during precombat this isn't harmful for the duration of 10.1 due to the class trinket existing
    if ( is_precombat )
      harmful = false;
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
  struct cobra_shot_aotw_t final : public attacks::cobra_shot_t
  {
    cobra_shot_aotw_t( util::string_view /*name*/, hunter_t* p ):
      cobra_shot_t( p, "" )
    {
      dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
    }
  };

  cobra_shot_aotw_t* cobra_shot = nullptr;

  aspect_of_the_wild_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "aspect_of_the_wild", p, p -> talents.aspect_of_the_wild )
  {
    parse_options( options_str );

    cobra_shot = p -> get_background_action<cobra_shot_aotw_t>( "cobra_shot_aotw" );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.aspect_of_the_wild -> trigger();
    cobra_shot -> execute_on_target( target );
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
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.call_of_the_wild -> trigger();
    p() -> pets.cotw_stable_pet.spawn( data().duration(), as<int>( data().effectN( 1 ).base_value() ) );
  }
};

// Stampede =================================================================

struct stampede_t: public hunter_spell_t
{
  struct damage_t : public hunter_spell_t
  {
    damage_t( hunter_t* p ):
      hunter_spell_t( "stampede_tick", p, p -> find_spell( 201594 ) )
    {
      aoe = -1;
      background = true;
    }

    void impact( action_state_t* s ) override
    {
      hunter_spell_t::impact( s );

      p() -> get_target_data( s -> target ) -> debuffs.stampede -> trigger();
    }
  };

  stampede_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "stampede", p, p -> talents.stampede )
  {
    parse_options( options_str );

    base_tick_time = 667_ms;
    dot_duration = data().duration();
    hasted_ticks = false;
    radius = 8;
    school = SCHOOL_PHYSICAL;

    tick_action = new damage_t( p );
  }

  void execute() override
  {
    hunter_spell_t::execute();
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
      pet -> active.bloodshed -> execute_on_target( target );
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

    //Only set to harmful for the 10.1 class trinket effect
    harmful = true;
  }

  //Used to ensure that during precombat this isn't harmful for the duration of 10.1 due to the class trinket existing
  void init_finished() override
  {
    hunter_spell_t::init_finished();

    if ( is_precombat )
      harmful = false;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    // Applying Trueshot directly does not extend an existing Trueshot and resets Unerring Vision stacks.
    p() -> buffs.trueshot -> expire();

    p() -> buffs.trueshot -> trigger();
  }
};

// Volley ===========================================================================

struct volley_t : public hunter_spell_t
{
  struct damage_t final : hunter_ranged_attack_t
  {
    attacks::explosive_shot_background_t* explosive = nullptr;

    damage_t( util::string_view n, hunter_t* p )
      : hunter_ranged_attack_t( n, p, p -> find_spell( 260247 ) )
    {
      aoe = -1;
      background = dual = ground_aoe = true;

      if ( p -> talents.salvo.ok() ) {
        explosive = p -> get_background_action<attacks::explosive_shot_background_t>( "explosive_shot_salvo" );
        explosive -> targets = as<size_t>( p -> talents.salvo -> effectN( 1 ).base_value() );
      }
    }

    void impact( action_state_t* s ) override
    {
      hunter_ranged_attack_t::impact( s );
    }

    void execute() override
    {
      hunter_ranged_attack_t::execute();

      if ( explosive && p() -> buffs.salvo -> check() )
      {
        std::vector<player_t*>& tl = target_list();
        size_t targets = std::min<size_t>( tl.size(), explosive -> targets );
        for ( size_t t = 0; t < targets; t++ )
          explosive -> execute_on_target( tl[ t ] );
        
        p() -> buffs.salvo -> expire();
      }
    }
  };

  damage_t* damage;

  volley_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "volley", p, p -> talents.volley ),
    damage( p -> get_background_action<damage_t>( "volley_damage" ) )
  {
    parse_options( options_str );

    // disable automatic generation of the dot from spell data
    dot_duration = 0_ms;

    may_hit = false;
    damage -> stats = stats;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.volley -> trigger();
    p() -> buffs.trick_shots -> trigger( data().duration() );

    make_event<ground_aoe_event_t>( *sim, player, ground_aoe_params_t()
        .target( execute_state -> target )
        .duration( data().duration() )
        .pulse_time( data().effectN( 2 ).period() )
        .action( damage )
      );

    p() -> player_t::reset_auto_attacks( data().duration(), player -> procs.reset_aa_channel );
  }
};

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

// Steel Trap =======================================================================

struct steel_trap_t: public trap_base_t
{
  struct impact_t final : public hunter_spell_t
  {
    impact_t( util::string_view n, hunter_t* p ):
      hunter_spell_t( n, p, p -> find_spell( 162487 ) )
    {
      background = true;
      dual = true;
    }
  };

  steel_trap_t( hunter_t* p, util::string_view options_str ):
    trap_base_t( "steel_trap", p, p -> talents.steel_trap )
  {
    parse_options( options_str );

    impact_action = p -> get_background_action<impact_t>( "steel_trap_impact" );
    add_child( impact_action );
  }
};

// Wildfire Bomb ==============================================================

struct wildfire_bomb_t: public hunter_spell_t
{
  struct state_data_t {
    wildfire_infusion_e current_bomb = WILDFIRE_INFUSION_SHRAPNEL;
  };
  using state_t = hunter_action_state_t<state_data_t>;

  struct bomb_base_t : public hunter_spell_t
  {
    struct dot_action_t final : public hunter_spell_t
    {
      dot_action_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
        hunter_spell_t( n, p, s )
      {
        dual = true;
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

        return am;
      }
    };
    dot_action_t* dot_action;

    bomb_base_t( util::string_view n, wildfire_bomb_t* a, const spell_data_t* s, util::string_view dot_n, const spell_data_t* dot_s ):
      hunter_spell_t( n, a -> p(), s ),
      dot_action( a -> p() -> get_background_action<dot_action_t>( dot_n, dot_s ) )
    {
      dual = true;

      aoe = -1;
      reduced_aoe_targets = p() -> talents.wildfire_bomb -> effectN( 2 ).base_value();
      radius = 5; // XXX: It's actually a circle + cone, but we sadly can't really model that

      dot_action -> reduced_aoe_targets = reduced_aoe_targets;
      dot_action -> aoe = aoe;
      dot_action -> radius = radius;

      if ( a -> p() -> talents.wildfire_infusion.ok() && a -> p() -> options.separate_wfi_stats )
      {
        add_child( dot_action );
      }
      else
      {
        a -> add_child( this );
        a -> add_child( dot_action );
      }
    }

    void execute() override
    {
      hunter_spell_t::execute();

      if ( p() -> talents.wildfire_infusion.ok() && p() -> options.separate_wfi_stats )
      {
        // fudge trigger_gcd so gcd() returns a non-zero value and we get proper dpet values
        const auto trigger_gcd_ = std::exchange( trigger_gcd, p() -> base_gcd );
        stats -> add_execute( gcd(), target );
        trigger_gcd = trigger_gcd_;
      }

      if ( num_targets_hit > 0 )
      {
        // Dot applies to all of the same targets hit by the main explosion
        dot_action -> target = target;
        dot_action -> target_cache.list = target_cache.list;
        dot_action -> target_cache.is_valid = true;
        dot_action -> execute();
      }

      p() -> buffs.coordinated_assault_empower -> up();
      p() -> buffs.coordinated_assault_empower -> expire();

      if( p() -> tier_set.t30_sv_2pc.ok() )
      {
        p() -> buffs.exposed_wound -> trigger();
      }
    }

    double energize_cast_regen( const action_state_t* s ) const override
    {
      double r = hunter_spell_t::energize_cast_regen( s );

      if ( p() -> talents.coordinated_kill.ok() && p() -> buffs.coordinated_assault -> check() )
        r += p() -> talents.coordinated_kill -> effectN( 2 ).base_value();

      return r;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double am = hunter_spell_t::composite_da_multiplier( s );

      auto td = p() -> find_target_data( s -> target ); 
      if( td )
      {
        am *= 1.0 + td -> debuffs.shredded_armor -> value();
      }

      return am;
    }
  };

  struct wildfire_bomb_damage_t final : public bomb_base_t
  {
    wildfire_bomb_damage_t( util::string_view n, hunter_t* p, wildfire_bomb_t* a ):
      bomb_base_t( n, a, p -> find_spell( 265157 ), "wildfire_bomb_dot", p -> find_spell( 269747 ) )
    {}
  };

  struct shrapnel_bomb_t final : public bomb_base_t
  {
    shrapnel_bomb_t( util::string_view n, hunter_t* p, wildfire_bomb_t* a ):
      bomb_base_t( n, a, p -> find_spell( 270338 ), "shrapnel_bomb", p -> find_spell( 270339 ) )
    {
      attacks::internal_bleeding_t internal_bleeding( p );
      if ( p -> options.separate_wfi_stats )
        add_child( internal_bleeding.action );
      else
        a -> add_child( internal_bleeding.action );
    }
  };

  struct pheromone_bomb_t final : public bomb_base_t
  {
    pheromone_bomb_t( util::string_view n, hunter_t* p, wildfire_bomb_t* a ):
      bomb_base_t( n, a, p -> find_spell( 270329 ), "pheromone_bomb", p -> find_spell( 270332 ) )
    {}
  };

  struct volatile_bomb_t final : public bomb_base_t
  {
    struct serpent_sting_vb_t final : public attacks::serpent_sting_base_t
    {
      serpent_sting_vb_t( util::string_view /*name*/, hunter_t* p ):
        serpent_sting_base_t( p, "", p -> find_spell( 271788 ) )
      {
        dual = true;
        base_costs[ RESOURCE_FOCUS ] = 0;

        aoe = as<int>( p -> talents.wildfire_infusion -> effectN( 2 ).base_value() );
      }

      size_t available_targets( std::vector< player_t* >& tl ) const override
      {
        hunter_ranged_attack_t::available_targets( tl );

        // Don't force primary target to stay at the front.
        std::partition( tl.begin(), tl.end(), [ this ]( player_t* t ) {
          return !( this -> td( t ) -> dots.serpent_sting -> is_ticking() );
          } );

        return tl.size();
      }
    };

    struct violent_reaction_t final : public hunter_spell_t
    {
      violent_reaction_t( util::string_view n, hunter_t* p ):
        hunter_spell_t( n, p, p -> find_spell( 260231 ) )
      {
        dual = true;
        aoe = -1;
      }

      double composite_da_multiplier( const action_state_t* s ) const override
      {
        double am = hunter_spell_t::composite_da_multiplier( s );

        auto td = p() -> find_target_data( s -> target ); 
        if( td )
        {
          am *= 1.0 + td -> debuffs.shredded_armor -> value();
        }

        return am;
      }
    };
    violent_reaction_t* violent_reaction;
    serpent_sting_vb_t* serpent_sting;

    volatile_bomb_t( util::string_view n, hunter_t* p, wildfire_bomb_t* a ):
      bomb_base_t( n, a, p -> find_spell( 271048 ), "volatile_bomb", p -> find_spell( 271049 ) ),
      violent_reaction( p -> get_background_action<violent_reaction_t>( "violent_reaction" ) )
    {
      if ( p -> options.separate_wfi_stats )
        add_child( violent_reaction );
      else
        a -> add_child( violent_reaction );

      serpent_sting = p -> get_background_action<serpent_sting_vb_t>( "serpent_sting_vb" );
    }

    void execute() override
    {
      bomb_base_t::execute();

      if ( td( target ) -> dots.serpent_sting -> is_ticking() )
        violent_reaction -> execute_on_target( target );

      serpent_sting -> execute_on_target( target );
    }
  };

  std::array<bomb_base_t*, 3> bombs;

  wildfire_bomb_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "wildfire_bomb", p, p -> talents.wildfire_bomb )
  {
    parse_options( options_str );

    may_miss = false;
    school = SCHOOL_FIRE; // for report coloring

    if ( p -> talents.wildfire_infusion.ok() )
    {
      bombs[ WILDFIRE_INFUSION_SHRAPNEL ] =  p -> get_background_action<shrapnel_bomb_t>( "shrapnel_bomb_impact", this );
      bombs[ WILDFIRE_INFUSION_PHEROMONE ] = p -> get_background_action<pheromone_bomb_t>( "pheromone_bomb_impact", this );
      bombs[ WILDFIRE_INFUSION_VOLATILE ] =  p -> get_background_action<volatile_bomb_t>( "volatile_bomb_impact", this );
    }
    else
    {
      bombs[ WILDFIRE_INFUSION_SHRAPNEL ] = p -> get_background_action<wildfire_bomb_damage_t>( "wildfire_bomb_impact", this );
    }
  }

  void execute() override
  {
    hunter_spell_t::execute();

    if ( p() -> talents.wildfire_infusion.ok() )
    {
      // assume that we can't get 2 same bombs in a row
      int slot = rng().range( 2 );
      if ( slot == p() -> state.next_wi_bomb )
        slot += 2 - p() -> state.next_wi_bomb;
      p() -> state.next_wi_bomb = static_cast<wildfire_infusion_e>( slot );
    }

    if ( p() -> talents.coordinated_kill.ok() && p() -> buffs.coordinated_assault -> check() )
      p() -> resource_gain( RESOURCE_FOCUS, p() -> talents.coordinated_kill -> effectN( 2 ).base_value(), p() -> gains.coordinated_kill, this);
  }

  void impact( action_state_t* s ) override
  {
    hunter_spell_t::impact( s );

    bombs[ debug_cast<state_t*>( s ) -> current_bomb ] -> execute_on_target( s -> target );
  }

  double recharge_rate_multiplier( const cooldown_t& cd ) const override
  {
    double m = hunter_spell_t::recharge_rate_multiplier( cd );

    if ( p() -> buffs.coordinated_assault -> check() )
      m *= 1 + p() -> talents.coordinated_kill -> effectN( 1 ).percent();

    return m;
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  void snapshot_state( action_state_t* s, result_amount_type type ) override
  {
    hunter_spell_t::snapshot_state( s, type );
    debug_cast<state_t*>( s ) -> current_bomb = p() -> state.next_wi_bomb;
  }
};

// Aspect of the Eagle ======================================================

struct aspect_of_the_eagle_t: public hunter_spell_t
{
  aspect_of_the_eagle_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "aspect_of_the_eagle", p, p -> talents.aspect_of_the_eagle )
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

} // namespace actions

hunter_td_t::hunter_td_t( player_t* target, hunter_t* p ):
  actor_target_data_t( target, p ),
  debuffs(),
  dots()
{
  debuffs.latent_poison =
    make_buff( *this, "latent_poison", p -> talents.poison_injection -> effectN( 1 ).trigger() )
      -> set_trigger_spell( p -> talents.poison_injection );

  debuffs.shredded_armor = 
    make_buff( *this, "shredded_armor", p -> find_spell( 410167 ) )
      -> set_default_value_from_effect( 1 );

  debuffs.death_chakram =
    make_buff( *this, "death_chakram", p -> find_spell( 375893 ) )
      -> set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER )
      -> set_cooldown( 0_s );

  debuffs.stampede = make_buff( *this, "stampede", p -> find_spell( 201594 ) )
    -> set_default_value_from_effect( 3 );

  dots.serpent_sting = target -> get_dot( "serpent_sting", p );
  dots.a_murder_of_crows = target -> get_dot( "a_murder_of_crows", p );
  dots.pheromone_bomb = target -> get_dot( "pheromone_bomb", p );
  dots.shrapnel_bomb = target -> get_dot( "shrapnel_bomb", p );

  target -> register_on_demise_callback( p, [this](player_t*) { target_demise(); } );
}

void hunter_td_t::target_demise()
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( source -> sim -> event_mgr.canceled )
    return;

  hunter_t* p = static_cast<hunter_t*>( source );
  if ( p -> talents.a_murder_of_crows.ok() && dots.a_murder_of_crows -> is_ticking() )
  {
    p -> sim -> print_debug( "{} a_murder_of_crows cooldown reset on target death.", p -> name() );
    p -> cooldowns.a_murder_of_crows -> reset( true );
  }

  if ( p -> talents.terms_of_engagement.ok() && damaged )
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

  if ( splits.size() == 1 && splits[ 0 ] == "dire_pack_count" )
  {
    return make_fn_expr( expression_str, [ this ] { return state.dire_pack_counter; });
  }
  else if ( splits.size() == 1 && splits[ 0 ] == "steady_focus_count" )
  {
    return make_fn_expr( expression_str, [ this ] { return state.steady_focus_counter; } );
  }
  else if ( splits.size() == 2 && splits[ 0 ] == "next_wi_bomb" )
  {
    if ( splits[ 1 ] == "shrapnel" )
      return make_fn_expr( expression_str, [ this ] { return talents.wildfire_infusion.ok() && state.next_wi_bomb == WILDFIRE_INFUSION_SHRAPNEL; } );
    if ( splits[ 1 ] == "pheromone" )
      return make_fn_expr( expression_str, [ this ] { return state.next_wi_bomb == WILDFIRE_INFUSION_PHEROMONE; } );
    if ( splits[ 1 ] == "volatile" )
      return make_fn_expr( expression_str, [ this ] { return state.next_wi_bomb == WILDFIRE_INFUSION_VOLATILE; } );
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

// hunter_t::create_action ==================================================

action_t* hunter_t::create_action( util::string_view name,
                                   util::string_view options_str )
{
  using namespace attacks;
  using namespace spells;

  if ( name == "a_murder_of_crows"     ) return new      a_murder_of_crows_t( this, options_str );
  if ( name == "aimed_shot"            ) return new             aimed_shot_t( this, options_str );
  if ( name == "arcane_shot"           ) return new            arcane_shot_t( this, options_str );
  if ( name == "aspect_of_the_eagle"   ) return new    aspect_of_the_eagle_t( this, options_str );
  if ( name == "aspect_of_the_wild"    ) return new     aspect_of_the_wild_t( this, options_str );
  if ( name == "auto_attack"           ) return new   actions::auto_attack_t( this, options_str );
  if ( name == "auto_shot"             ) return new   actions::auto_attack_t( this, options_str );
  if ( name == "barbed_shot"           ) return new            barbed_shot_t( this, options_str );
  if ( name == "barrage"               ) return new                barrage_t( this, options_str );
  if ( name == "bestial_wrath"         ) return new          bestial_wrath_t( this, options_str );
  if ( name == "bloodshed"             ) return new              bloodshed_t( this, options_str );
  if ( name == "bursting_shot"         ) return new          bursting_shot_t( this, options_str );
  if ( name == "butchery"              ) return new               butchery_t( this, options_str );
  if ( name == "call_of_the_wild"      ) return new       call_of_the_wild_t( this, options_str );
  if ( name == "carve"                 ) return new                  carve_t( this, options_str );
  if ( name == "chimaera_shot"         ) return new          chimaera_shot_t( this, options_str );
  if ( name == "cobra_shot"            ) return new             cobra_shot_t( this, options_str );
  if ( name == "coordinated_assault"   ) return new    coordinated_assault_t( this, options_str );
  if ( name == "counter_shot"          ) return new           counter_shot_t( this, options_str );
  if ( name == "death_chakram"         ) return new          death_chakram_t( this, options_str );
  if ( name == "dire_beast"            ) return new             dire_beast_t( this, options_str );
  if ( name == "explosive_shot"        ) return new         explosive_shot_t( this, options_str );
  if ( name == "flanking_strike"       ) return new        flanking_strike_t( this, options_str );
  if ( name == "freezing_trap"         ) return new          freezing_trap_t( this, options_str );
  if ( name == "fury_of_the_eagle"     ) return new      fury_of_the_eagle_t( this, options_str );
  if ( name == "harpoon"               ) return new                harpoon_t( this, options_str );
  if ( name == "high_explosive_trap"   ) return new    high_explosive_trap_t( this, options_str );
  if ( name == "kill_command"          ) return new           kill_command_t( this, options_str );
  if ( name == "kill_shot"             ) return new              kill_shot_t( this, options_str );
  if ( name == "mongoose_bite"         ) return new          mongoose_bite_t( this, options_str );
  if ( name == "mongoose_bite_eagle"   ) return new    mongoose_bite_eagle_t( this, options_str );
  if ( name == "muzzle"                ) return new                 muzzle_t( this, options_str );
  if ( name == "rapid_fire"            ) return new             rapid_fire_t( this, options_str );
  if ( name == "raptor_strike"         ) return new          raptor_strike_t( this, options_str );
  if ( name == "raptor_strike_eagle"   ) return new    raptor_strike_eagle_t( this, options_str );
  if ( name == "salvo"                 ) return new                  salvo_t( this, options_str );
  if ( name == "serpent_sting"         ) return new          serpent_sting_t( this, options_str );
  if ( name == "spearhead"             ) return new              spearhead_t( this, options_str );
  if ( name == "stampede"              ) return new               stampede_t( this, options_str );
  if ( name == "steady_shot"           ) return new            steady_shot_t( this, options_str );
  if ( name == "steel_trap"            ) return new             steel_trap_t( this, options_str );
  if ( name == "summon_pet"            ) return new             summon_pet_t( this, options_str );
  if ( name == "tar_trap"              ) return new               tar_trap_t( this, options_str );
  if ( name == "trueshot"              ) return new               trueshot_t( this, options_str );
  if ( name == "volley"                ) return new                 volley_t( this, options_str );
  if ( name == "wailing_arrow"         ) return new          wailing_arrow_t( this, options_str );
  if ( name == "wildfire_bomb"         ) return new          wildfire_bomb_t( this, options_str );

  if ( name == "multishot" )
  {
    if ( specialization() == HUNTER_MARKSMANSHIP )
      return new multishot_mm_t( this, options_str );
    if ( specialization() == HUNTER_BEAST_MASTERY )
      return new multishot_bm_t( this, options_str );
  }

  return player_t::create_action( name, options_str );
}

// hunter_t::create_pet =====================================================

pet_t* hunter_t::create_pet( util::string_view pet_name,
                             util::string_view pet_type )
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

  if ( talents.animal_companion.ok() )
    pets.animal_companion = new pets::animal_companion_t( this );

  if ( talents.dire_beast.ok() )
    pets.dire_beast = new pets::dire_critter_t( this );

  if ( talents.dire_command.ok() )
    pets.dc_dire_beast.set_creation_callback(
      []( hunter_t* p ) { return new pets::dire_critter_t( p, "dire_beast_(dc)" ); });
}

void hunter_t::init()
{
  player_t::init();
}

// hunter_t::resource_loss ==================================================

double hunter_t::resource_loss( resource_e resource_type, double amount, gain_t* g, action_t* a )
{
  double loss = player_t::resource_loss(resource_type, amount, g, a);

  if ( loss > 0 && talents.wild_instincts.ok() && buffs.call_of_the_wild -> check() && rng().roll( talents.wild_instincts -> proc_chance() ) )
  {
    procs.wild_instincts -> occur();
    cooldowns.barbed_shot -> reset( true );
  }

  return loss;
}

// hunter_t::init_spells ====================================================

void hunter_t::init_spells()
{
  player_t::init_spells();

  // Hunter Tree
  talents.kill_command                      = find_talent_spell( talent_tree::CLASS, "Kill Command" );
  talents.kill_shot                         = find_talent_spell( talent_tree::CLASS, "Kill Shot" );

  talents.improved_kill_shot                = find_talent_spell( talent_tree::CLASS, "Improved Kill Shot" );

  talents.tar_trap                          = find_talent_spell( talent_tree::CLASS, "Tar Trap" );

  talents.improved_traps                    = find_talent_spell( talent_tree::CLASS, "Improved Traps" );
  talents.born_to_be_wild                   = find_talent_spell( talent_tree::CLASS, "Born To Be Wild" );

  talents.high_explosive_trap               = find_talent_spell( talent_tree::CLASS, "High Explosive Trap" );

  talents.beast_master                      = find_talent_spell( talent_tree::CLASS, "Beast Master" );
  talents.keen_eyesight                     = find_talent_spell( talent_tree::CLASS, "Keen Eyesight" );
  talents.master_marksman                   = find_talent_spell( talent_tree::CLASS, "Master Marksman" );

  talents.improved_kill_command             = find_talent_spell( talent_tree::CLASS, "Improved Kill Command" );
  talents.serrated_shots                    = find_talent_spell( talent_tree::CLASS, "Serrated Shots" );
  talents.arctic_bola                       = find_talent_spell( talent_tree::CLASS, "Arctic Bola" );
  talents.serpent_sting                     = find_talent_spell( talent_tree::CLASS, "Serpent Sting" );

  talents.alpha_predator                    = find_talent_spell( talent_tree::CLASS, "Alpha Predator" );
  talents.killer_instinct                   = find_talent_spell( talent_tree::CLASS, "Killer Instinct" );
  talents.steel_trap                        = find_talent_spell( talent_tree::CLASS, "Steel Trap" );
  talents.stampede                          = find_talent_spell( talent_tree::CLASS, "Stampede" );
  talents.death_chakram                     = find_talent_spell( talent_tree::CLASS, "Death Chakram" );
  talents.explosive_shot                    = find_talent_spell( talent_tree::CLASS, "Explosive Shot" );
  talents.barrage                           = find_talent_spell( talent_tree::CLASS, "Barrage" );
  talents.poison_injection                  = find_talent_spell( talent_tree::CLASS, "Poison Injection" );
  talents.hydras_bite                       = find_talent_spell( talent_tree::CLASS, "Hydra's Bite" );

  // Marksmanship Tree
  if (specialization() == HUNTER_MARKSMANSHIP)
  {
    talents.aimed_shot                        = find_talent_spell( talent_tree::SPECIALIZATION, "Aimed Shot", HUNTER_MARKSMANSHIP );

    talents.crack_shot                        = find_talent_spell( talent_tree::SPECIALIZATION, "Crack Shot", HUNTER_MARKSMANSHIP );
    talents.improved_steady_shot              = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Steady Shot", HUNTER_MARKSMANSHIP );
    talents.precise_shots                     = find_talent_spell( talent_tree::SPECIALIZATION, "Precise Shots", HUNTER_MARKSMANSHIP );

    talents.rapid_fire                        = find_talent_spell( talent_tree::SPECIALIZATION, "Rapid Fire", HUNTER_MARKSMANSHIP );
    talents.lone_wolf                         = find_talent_spell( talent_tree::SPECIALIZATION, "Lone Wolf", HUNTER_MARKSMANSHIP );
    talents.chimaera_shot                     = find_talent_spell( talent_tree::SPECIALIZATION, "Chimaera Shot", HUNTER_MARKSMANSHIP );

    talents.streamline                        = find_talent_spell( talent_tree::SPECIALIZATION, "Streamline", HUNTER_MARKSMANSHIP );
    talents.killer_accuracy                   = find_talent_spell( talent_tree::SPECIALIZATION, "Killer Accuracy", HUNTER_MARKSMANSHIP );
    talents.hunters_knowledge                 = find_talent_spell( talent_tree::SPECIALIZATION, "Hunter's Knowledge", HUNTER_MARKSMANSHIP );
    talents.careful_aim                       = find_talent_spell( talent_tree::SPECIALIZATION, "Careful Aim", HUNTER_MARKSMANSHIP );

    talents.in_the_rhythm                     = find_talent_spell( talent_tree::SPECIALIZATION, "In the Rhythm", HUNTER_MARKSMANSHIP );
    talents.surging_shots                     = find_talent_spell( talent_tree::SPECIALIZATION, "Surging Shots", HUNTER_MARKSMANSHIP );
    talents.deathblow                         = find_talent_spell( talent_tree::SPECIALIZATION, "Deathblow", HUNTER_MARKSMANSHIP );
    talents.target_practice                   = find_talent_spell( talent_tree::SPECIALIZATION, "Target Practice", HUNTER_MARKSMANSHIP );
    talents.focused_aim                       = find_talent_spell( talent_tree::SPECIALIZATION, "Focused Aim", HUNTER_MARKSMANSHIP );

    talents.multishot_mm                      = find_talent_spell( talent_tree::SPECIALIZATION, "Multi-Shot", HUNTER_MARKSMANSHIP );
    talents.razor_fragments                   = find_talent_spell( talent_tree::SPECIALIZATION, "Razor Fragments", HUNTER_MARKSMANSHIP );
    talents.dead_eye                          = find_talent_spell( talent_tree::SPECIALIZATION, "Deadeye", HUNTER_MARKSMANSHIP );
    talents.bursting_shot                     = find_talent_spell( talent_tree::SPECIALIZATION, "Bursting Shot", HUNTER_MARKSMANSHIP );

    talents.trick_shots                       = find_talent_spell( talent_tree::SPECIALIZATION, "Trick Shots", HUNTER_MARKSMANSHIP );
    talents.bombardment                       = find_talent_spell( talent_tree::SPECIALIZATION, "Bombardment", HUNTER_MARKSMANSHIP );
    talents.volley                            = find_talent_spell( talent_tree::SPECIALIZATION, "Volley", HUNTER_MARKSMANSHIP );
    talents.tactical_reload                   = find_talent_spell( talent_tree::SPECIALIZATION, "Tactical Reload", HUNTER_MARKSMANSHIP );
    talents.steady_focus                      = find_talent_spell( talent_tree::SPECIALIZATION, "Steady Focus", HUNTER_MARKSMANSHIP );
    talents.serpentstalkers_trickery          = find_talent_spell( talent_tree::SPECIALIZATION, "Serpentstalker's Trickery", HUNTER_MARKSMANSHIP );

    talents.light_ammo                        = find_talent_spell( talent_tree::SPECIALIZATION, "Light Ammo", HUNTER_MARKSMANSHIP );
    talents.heavy_ammo                        = find_talent_spell( talent_tree::SPECIALIZATION, "Heavy Ammo", HUNTER_MARKSMANSHIP );
    talents.trueshot                          = find_talent_spell( talent_tree::SPECIALIZATION, "Trueshot", HUNTER_MARKSMANSHIP );
    talents.lock_and_load                     = find_talent_spell( talent_tree::SPECIALIZATION, "Lock and Load", HUNTER_MARKSMANSHIP );
    talents.bullseye                          = find_talent_spell( talent_tree::SPECIALIZATION, "Bullseye", HUNTER_MARKSMANSHIP );

    talents.bulletstorm                       = find_talent_spell( talent_tree::SPECIALIZATION, "Bulletstorm", HUNTER_MARKSMANSHIP );
    talents.sharpshooter                      = find_talent_spell( talent_tree::SPECIALIZATION, "Sharpshooter", HUNTER_MARKSMANSHIP );
    talents.eagletalons_true_focus            = find_talent_spell( talent_tree::SPECIALIZATION, "Eagletalon's True Focus", HUNTER_MARKSMANSHIP );
    talents.wailing_arrow                     = find_talent_spell( talent_tree::SPECIALIZATION, "Wailing Arrow", HUNTER_MARKSMANSHIP );
    talents.legacy_of_the_windrunners         = find_talent_spell( talent_tree::SPECIALIZATION, "Legacy of the Windrunners", HUNTER_MARKSMANSHIP );

    talents.salvo                             = find_talent_spell( talent_tree::SPECIALIZATION, "Salvo", HUNTER_MARKSMANSHIP );
    talents.calling_the_shots                 = find_talent_spell( talent_tree::SPECIALIZATION, "Calling the Shots", HUNTER_MARKSMANSHIP );
    talents.unerring_vision                   = find_talent_spell( talent_tree::SPECIALIZATION, "Unerring Vision", HUNTER_MARKSMANSHIP );
    talents.windrunners_barrage               = find_talent_spell( talent_tree::SPECIALIZATION, "Windrunner's Barrage", HUNTER_MARKSMANSHIP );
    talents.readiness                         = find_talent_spell( talent_tree::SPECIALIZATION, "Readiness", HUNTER_MARKSMANSHIP );
    talents.windrunners_guidance              = find_talent_spell( talent_tree::SPECIALIZATION, "Windrunner's Guidance", HUNTER_MARKSMANSHIP );
  }

  // Beast Mastery Tree
  if (specialization() == HUNTER_BEAST_MASTERY)
  {
    talents.cobra_shot                        = find_talent_spell( talent_tree::SPECIALIZATION, "Cobra Shot", HUNTER_BEAST_MASTERY );

    talents.pack_tactics                      = find_talent_spell( talent_tree::SPECIALIZATION, "Pack Tactics", HUNTER_BEAST_MASTERY );
    talents.multishot_bm                      = find_talent_spell( talent_tree::SPECIALIZATION, "Multi-Shot", HUNTER_BEAST_MASTERY );
    talents.barbed_shot                       = find_talent_spell( talent_tree::SPECIALIZATION, "Barbed Shot", HUNTER_BEAST_MASTERY );

    talents.aspect_of_the_beast               = find_talent_spell( talent_tree::SPECIALIZATION, "Aspect of the Beast", HUNTER_BEAST_MASTERY );
    talents.kindred_spirits                   = find_talent_spell( talent_tree::SPECIALIZATION, "Kindred Spirits", HUNTER_BEAST_MASTERY);
    talents.training_expert                   = find_talent_spell( talent_tree::SPECIALIZATION, "Training Expert", HUNTER_BEAST_MASTERY);

    talents.animal_companion                  = find_talent_spell( talent_tree::SPECIALIZATION, "Animal Companion", HUNTER_BEAST_MASTERY);
    talents.beast_cleave                      = find_talent_spell( talent_tree::SPECIALIZATION, "Beast Cleave", HUNTER_BEAST_MASTERY);
    talents.killer_command                    = find_talent_spell( talent_tree::SPECIALIZATION, "Killer Command", HUNTER_BEAST_MASTERY);
    talents.sharp_barbs                       = find_talent_spell( talent_tree::SPECIALIZATION, "Sharp Barbs", HUNTER_BEAST_MASTERY);

    talents.cobra_sting                       = find_talent_spell( talent_tree::SPECIALIZATION, "Cobra Sting", HUNTER_BEAST_MASTERY);
    talents.thrill_of_the_hunt                = find_talent_spell( talent_tree::SPECIALIZATION, "Thrill of the Hunt", HUNTER_BEAST_MASTERY);
    talents.kill_cleave                       = find_talent_spell( talent_tree::SPECIALIZATION, "Kill Cleave", HUNTER_BEAST_MASTERY);
    talents.a_murder_of_crows                 = find_talent_spell( talent_tree::SPECIALIZATION, "A Murder of Crows", HUNTER_BEAST_MASTERY);
    talents.bloodshed                         = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodshed", HUNTER_BEAST_MASTERY);
    talents.cobra_senses                      = find_talent_spell( talent_tree::SPECIALIZATION, "Cobra Senses", HUNTER_BEAST_MASTERY);

    talents.dire_beast                        = find_talent_spell( talent_tree::SPECIALIZATION, "Dire Beast", HUNTER_BEAST_MASTERY);
    talents.bestial_wrath                     = find_talent_spell( talent_tree::SPECIALIZATION, "Bestial Wrath", HUNTER_BEAST_MASTERY);
    talents.war_orders                        = find_talent_spell( talent_tree::SPECIALIZATION, "War Orders", HUNTER_BEAST_MASTERY);

    talents.hunters_prey                      = find_talent_spell( talent_tree::SPECIALIZATION, "Hunter's Prey", HUNTER_BEAST_MASTERY);
    talents.stomp                             = find_talent_spell( talent_tree::SPECIALIZATION, "Stomp", HUNTER_BEAST_MASTERY);
    talents.barbed_wrath                      = find_talent_spell( talent_tree::SPECIALIZATION, "Barbed Wrath", HUNTER_BEAST_MASTERY);
    talents.wild_call                         = find_talent_spell( talent_tree::SPECIALIZATION, "Wild Call", HUNTER_BEAST_MASTERY);
    talents.aspect_of_the_wild                = find_talent_spell( talent_tree::SPECIALIZATION, "Aspect of the Wild", HUNTER_BEAST_MASTERY);


    talents.dire_command                      = find_talent_spell( talent_tree::SPECIALIZATION, "Dire Command", HUNTER_BEAST_MASTERY);
    talents.scent_of_blood                    = find_talent_spell( talent_tree::SPECIALIZATION, "Scent of Blood", HUNTER_BEAST_MASTERY);
    talents.one_with_the_pack                 = find_talent_spell( talent_tree::SPECIALIZATION, "One with the Pack", HUNTER_BEAST_MASTERY);
    talents.master_handler                    = find_talent_spell( talent_tree::SPECIALIZATION, "Master Handler", HUNTER_BEAST_MASTERY);
    talents.snake_bite                        = find_talent_spell( talent_tree::SPECIALIZATION, "Snake Bite", HUNTER_BEAST_MASTERY);

    talents.dire_frenzy                       = find_talent_spell( talent_tree::SPECIALIZATION, "Dire Frenzy", HUNTER_BEAST_MASTERY);
    talents.wailing_arrow                     = find_talent_spell( talent_tree::SPECIALIZATION, "Wailing Arrow", HUNTER_BEAST_MASTERY );
    talents.brutal_companion                  = find_talent_spell( talent_tree::SPECIALIZATION, "Brutal Companion", HUNTER_BEAST_MASTERY );
    talents.call_of_the_wild                  = find_talent_spell( talent_tree::SPECIALIZATION, "Call of the Wild", HUNTER_BEAST_MASTERY );

    talents.dire_pack                         = find_talent_spell( talent_tree::SPECIALIZATION, "Dire Pack", HUNTER_BEAST_MASTERY );
    talents.piercing_fangs                    = find_talent_spell( talent_tree::SPECIALIZATION, "Piercing Fangs", HUNTER_BEAST_MASTERY );
    talents.killer_cobra                      = find_talent_spell( talent_tree::SPECIALIZATION, "Killer Cobra", HUNTER_BEAST_MASTERY );
    talents.bloody_frenzy                     = find_talent_spell( talent_tree::SPECIALIZATION, "Bloody Frenzy", HUNTER_BEAST_MASTERY );
    talents.wild_instincts                    = find_talent_spell( talent_tree::SPECIALIZATION, "Wild Instincts", HUNTER_BEAST_MASTERY );
  }

  // Survival Tree
  if ( specialization() == HUNTER_SURVIVAL )
  {
    talents.raptor_strike                     = find_talent_spell( talent_tree::SPECIALIZATION, "Raptor Strike", HUNTER_SURVIVAL );

    talents.wildfire_bomb                     = find_talent_spell( talent_tree::SPECIALIZATION, "Wildfire Bomb", HUNTER_SURVIVAL );
    talents.tip_of_the_spear                  = find_talent_spell( talent_tree::SPECIALIZATION, "Tip of the Spear", HUNTER_SURVIVAL );

    talents.ferocity                          = find_talent_spell( talent_tree::SPECIALIZATION, "Ferocity", HUNTER_SURVIVAL );
    talents.flankers_advantage                = find_talent_spell( talent_tree::SPECIALIZATION, "Flanker's Advantage", HUNTER_SURVIVAL );
    talents.harpoon                           = find_talent_spell( talent_tree::SPECIALIZATION, "Harpoon", HUNTER_SURVIVAL );

    talents.energetic_ally                    = find_talent_spell( talent_tree::SPECIALIZATION, "Energetic Ally", HUNTER_SURVIVAL );
    talents.bloodseeker                       = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodseeker", HUNTER_SURVIVAL );
    talents.aspect_of_the_eagle               = find_talent_spell( talent_tree::SPECIALIZATION, "Aspect of the Eagle", HUNTER_SURVIVAL );
    talents.terms_of_engagement               = find_talent_spell( talent_tree::SPECIALIZATION, "Terms of Engagement", HUNTER_SURVIVAL );

    talents.guerrilla_tactics                 = find_talent_spell( talent_tree::SPECIALIZATION, "Guerrilla Tactics", HUNTER_SURVIVAL );
    talents.lunge                             = find_talent_spell( talent_tree::SPECIALIZATION, "Lunge", HUNTER_SURVIVAL );
    talents.butchery                          = find_talent_spell( talent_tree::SPECIALIZATION, "Butchery", HUNTER_SURVIVAL );
    talents.carve                             = find_talent_spell( talent_tree::SPECIALIZATION, "Carve", HUNTER_SURVIVAL );
    talents.mongoose_bite                     = find_talent_spell( talent_tree::SPECIALIZATION, "Mongoose Bite", HUNTER_SURVIVAL );

    talents.intense_focus                     = find_talent_spell( talent_tree::SPECIALIZATION, "Intense Focus", HUNTER_SURVIVAL );
    talents.improved_wildfire_bomb            = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Wildfire Bomb", HUNTER_SURVIVAL );
    talents.frenzy_strikes                    = find_talent_spell( talent_tree::SPECIALIZATION, "Frenzy Strikes", HUNTER_SURVIVAL );
    talents.flanking_strike                   = find_talent_spell( talent_tree::SPECIALIZATION, "Flanking Strike", HUNTER_SURVIVAL );
    talents.spear_focus                       = find_talent_spell( talent_tree::SPECIALIZATION, "Spear Focus", HUNTER_SURVIVAL );

    talents.vipers_venom                      = find_talent_spell( talent_tree::SPECIALIZATION, "Viper's Venom", HUNTER_SURVIVAL );
    talents.sharp_edges                       = find_talent_spell( talent_tree::SPECIALIZATION, "Sharp Edges", HUNTER_SURVIVAL );
    talents.sweeping_spear                    = find_talent_spell( talent_tree::SPECIALIZATION, "Sweeping Spear", HUNTER_SURVIVAL );
    talents.tactical_advantage                = find_talent_spell( talent_tree::SPECIALIZATION, "Tactical Advantage", HUNTER_SURVIVAL );
    talents.bloody_claws                      = find_talent_spell( talent_tree::SPECIALIZATION, "Bloody Claws", HUNTER_SURVIVAL );

    talents.wildfire_infusion                 = find_talent_spell( talent_tree::SPECIALIZATION, "Wildfire Infusion", HUNTER_SURVIVAL );
    talents.quick_shot                        = find_talent_spell( talent_tree::SPECIALIZATION, "Quick Shot", HUNTER_SURVIVAL );
    talents.coordinated_assault               = find_talent_spell( talent_tree::SPECIALIZATION, "Coordinated Assault", HUNTER_SURVIVAL );
    talents.killer_companion                  = find_talent_spell( talent_tree::SPECIALIZATION, "Killer Companion", HUNTER_SURVIVAL );

    talents.fury_of_the_eagle                 = find_talent_spell( talent_tree::SPECIALIZATION, "Fury of the Eagle", HUNTER_SURVIVAL );
    talents.ranger                            = find_talent_spell( talent_tree::SPECIALIZATION, "Ranger", HUNTER_SURVIVAL );
    talents.coordinated_kill                  = find_talent_spell( talent_tree::SPECIALIZATION, "Coordinated Kill", HUNTER_SURVIVAL );
    talents.explosives_expert                 = find_talent_spell( talent_tree::SPECIALIZATION, "Explosives Expert", HUNTER_SURVIVAL );
    talents.spearhead                         = find_talent_spell( talent_tree::SPECIALIZATION, "Spearhead", HUNTER_SURVIVAL );

    talents.ruthless_marauder                 = find_talent_spell( talent_tree::SPECIALIZATION, "Ruthless Marauder", HUNTER_SURVIVAL );
    talents.birds_of_prey                     = find_talent_spell( talent_tree::SPECIALIZATION, "Birds of Prey", HUNTER_SURVIVAL );
    talents.bombardier                        = find_talent_spell( talent_tree::SPECIALIZATION, "Bombardier", HUNTER_SURVIVAL );
    talents.deadly_duo                        = find_talent_spell( talent_tree::SPECIALIZATION, "Deadly Duo", HUNTER_SURVIVAL );
  }

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

  specs.freezing_trap        = find_class_spell( "Freezing Trap" );
  specs.arcane_shot          = find_class_spell( "Arcane Shot" );
  specs.steady_shot          = find_class_spell( "Steady Shot" );
  specs.flare                = find_class_spell( "Flare" );

  // Rae'shalare, Death's Whisper spell
  specs.wailing_arrow        = find_item_by_name( "raeshalare_deaths_whisper" ) ? find_spell( 355589 ) : spell_data_t::not_found();

  // Tier Sets
  tier_set.t29_mm_2pc = sets -> set( HUNTER_MARKSMANSHIP, T29, B2 );
  tier_set.t29_mm_4pc = sets -> set( HUNTER_MARKSMANSHIP, T29, B4 );
  tier_set.t29_bm_2pc = sets -> set( HUNTER_BEAST_MASTERY, T29, B2 );
  tier_set.t29_bm_4pc = sets -> set( HUNTER_BEAST_MASTERY, T29, B4 );
  tier_set.t29_sv_2pc = sets -> set( HUNTER_SURVIVAL, T29, B2 );
  tier_set.t29_sv_4pc = sets -> set( HUNTER_SURVIVAL, T29, B4 );
  tier_set.t29_sv_4pc_buff = find_spell( 394388 );

  tier_set.t30_bm_2pc = sets -> set( HUNTER_BEAST_MASTERY, T30, B2 );
  tier_set.t30_bm_4pc = sets -> set( HUNTER_BEAST_MASTERY, T30, B4 );
  tier_set.t30_mm_2pc = sets -> set( HUNTER_MARKSMANSHIP, T30, B2 );
  tier_set.t30_mm_4pc = sets -> set( HUNTER_MARKSMANSHIP, T30, B4 );
  tier_set.t30_sv_2pc = sets -> set( HUNTER_SURVIVAL, T30, B2 );
  tier_set.t30_sv_4pc = sets -> set( HUNTER_SURVIVAL, T30, B4 );

  // Cooldowns
  cooldowns.ruthless_marauder -> duration = talents.ruthless_marauder -> internal_cooldown();
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
    + talents.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS )
    + talents.energetic_ally -> effectN( 1 ).resource( RESOURCE_FOCUS );
}

void hunter_t::create_actions()
{
  player_t::create_actions();

  if ( talents.master_marksman.ok() )
    actions.master_marksman = new attacks::master_marksman_t( this );

  if ( talents.poison_injection.ok() )
    actions.latent_poison = new attacks::latent_poison_t( this );

  if ( talents.arctic_bola.ok() )
    actions.arctic_bola = new attacks::arctic_bola_t( this );
  
  if ( talents.dire_command.ok() )
    actions.dire_command = new spells::dire_command_summon_t( this );
  
  if( talents.windrunners_guidance.ok() )
    actions.windrunners_guidance_background = new attacks::windrunners_guidance_background_t( this );
}

void hunter_t::create_buffs()
{
  player_t::create_buffs();

  // Marksmanship Tree

  buffs.bombardment =
    make_buff( this, "bombardment", find_spell( 386875 ) )
      -> set_chance( talents.bombardment.ok() );

  buffs.precise_shots =
    make_buff( this, "precise_shots", find_spell( 260242 ) )
      -> set_default_value( talents.precise_shots -> effectN( 1 ).percent() )
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

  buffs.volley =
    make_buff( this, "volley", talents.volley )
      -> set_cooldown( 0_ms )
      -> set_period( 0_ms ); // disable ticks as an optimization

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
        } );

  buffs.razor_fragments =
    make_buff( this, "razor_fragments", find_spell( 388998 ) )
    -> set_chance( talents.razor_fragments.ok() )
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

  buffs.flamewakers_cobra_sting =
    make_buff( this, "flamewakers_cobra_sting", find_spell( 336826 ) )
      -> set_default_value_from_effect( 1 );

  buffs.cobra_sting =
    make_buff( this, "cobra_sting", talents.cobra_sting -> effectN( 1 ).trigger() )
      -> set_chance( talents.cobra_sting -> effectN( 2 ).percent() )
      -> set_default_value_from_effect( 1 );

  buffs.thrill_of_the_hunt =
    make_buff( this, "thrill_of_the_hunt", talents.thrill_of_the_hunt -> effectN( 1 ).trigger() )
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
    make_buff( this, "hunters_prey", talents.hunters_prey -> effectN( 1 ).trigger() )
      -> set_chance( talents.hunters_prey -> effectN( 1 ).percent() );

  buffs.aspect_of_the_wild =
    make_buff( this, "aspect_of_the_wild", talents.aspect_of_the_wild )
      -> set_cooldown( 0_ms )
      -> set_default_value_from_effect( 2 );

  buffs.call_of_the_wild =
    make_buff( this, "call_of_the_wild", talents.call_of_the_wild )
      -> set_cooldown( 0_ms )
      -> set_tick_callback(
        [ this ]( buff_t*, int, timespan_t ) {
          pets.cotw_stable_pet.spawn( talents.call_of_the_wild -> effectN( 2 ).trigger() -> duration(), 1 );
        } );

  buffs.dire_pack =
    make_buff( this, "dire_pack", find_spell( 378747 ) )
      -> set_default_value_from_effect( 1 )
      -> set_chance( talents.dire_pack.ok() )
      -> set_stack_change_callback(
        [ this ]( buff_t*, int, int ) {
          cooldowns.kill_command -> adjust_recharge_multiplier();
        } );

  // Survival

  buffs.bloodseeker =
    make_buff( this, "bloodseeker", find_spell( 260249 ) )
      -> set_default_value_from_effect( 1 )
      -> add_invalidate( CACHE_ATTACK_SPEED );

  buffs.tip_of_the_spear =
    make_buff( this, "tip_of_the_spear", find_spell( 260286 ) )
      -> set_default_value( talents.tip_of_the_spear -> effectN( 1 ).percent() )
      -> set_chance( talents.tip_of_the_spear.ok() );

  buffs.terms_of_engagement =
    make_buff( this, "terms_of_engagement", find_spell( 265898 ) )
      -> set_default_value_from_effect( 1, 1 / 5.0 )
      -> set_affects_regen( true );

  buffs.aspect_of_the_eagle =
    make_buff( this, "aspect_of_the_eagle", talents.aspect_of_the_eagle )
      -> set_cooldown( 0_ms );

  buffs.mongoose_fury =
    make_buff( this, "mongoose_fury", find_spell( 259388 ) )
      -> set_default_value_from_effect( 1 )
      -> set_refresh_behavior( buff_refresh_behavior::DISABLED );

  buffs.coordinated_assault =
    make_buff( this, "coordinated_assault", talents.coordinated_assault )
    -> set_cooldown( 0_ms );

  if ( talents.coordinated_kill.ok() ) {
    buffs.coordinated_assault -> set_stack_change_callback(
      [ this ]( buff_t*, int /*old*/, int /*cur*/ ) {
        if ( talents.bombardier.ok() )
          cooldowns.wildfire_bomb -> reset( true );

        cooldowns.wildfire_bomb -> adjust_recharge_multiplier();
        cooldowns.kill_shot -> adjust_recharge_multiplier();
      } );
  }

  buffs.coordinated_assault_empower =
    make_buff( this, "coordinated_assault_empower", find_spell( 361738 ) )
      -> set_default_value_from_effect( 2 );

  buffs.spearhead =
    make_buff( this, "spearhead", talents.spearhead )
    -> set_default_value_from_effect( 1 );

  buffs.deadly_duo =
    make_buff( this, "deadly_duo", find_spell( 397568 ) )
      -> set_chance( talents.deadly_duo.ok() )
      -> set_default_value( talents.deadly_duo -> effectN( 1 ).percent() );

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

  buffs.find_the_mark =
    make_buff( this, "find_the_mark", tier_set.t29_mm_2pc -> effectN( 1 ).trigger() )
      -> set_default_value_from_effect( 1 );

  buffs.focusing_aim =
    make_buff( this, "focusing_aim", tier_set.t29_mm_4pc -> effectN( 1 ).trigger() )
    -> set_chance( tier_set.t29_mm_4pc -> effectN( 1 ).percent() )
    -> set_default_value_from_effect( 1 );

  buffs.lethal_command =
    make_buff( this, "lethal_command", tier_set.t29_bm_4pc -> effectN( 3 ).trigger() )
      -> set_default_value_from_effect( 1 );

  buffs.bestial_barrage =
    make_buff( this, "bestial_barrage", tier_set.t29_sv_4pc_buff )
    -> set_chance( tier_set.t29_sv_4pc -> effectN( 1 ).percent() );

  buffs.exposed_wound = 
    make_buff( this, "exposed_wound", tier_set.t30_sv_2pc -> effectN( 2 ).trigger() ) 
      -> set_default_value_from_effect( 1 ); 
}

// hunter_t::init_gains =====================================================

void hunter_t::init_gains()
{
  player_t::init_gains();

  gains.trueshot               = get_gain( "Trueshot" );
  gains.legacy_of_the_windrunners = get_gain( "Legacy of the Windrunners" );

  gains.barbed_shot            = get_gain( "Barbed Shot" );

  gains.terms_of_engagement    = get_gain( "Terms of Engagement" );
  gains.coordinated_kill       = get_gain( "Coordinated Kill" );
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

  if ( talents.calling_the_shots.ok() )
    procs.calling_the_shots   = get_proc( "Calling the Shots" );

  if ( talents.windrunners_guidance.ok() )
    procs.windrunners_guidance = get_proc( "Windrunner's Guidance" );

  if ( talents.dire_command.ok() )
    procs.dire_command        = get_proc( "Dire Command" );

  if ( talents.wild_call.ok() )
    procs.wild_call           = get_proc( "Wild Call" );
  
  if( talents.wild_instincts.ok() )
    procs.wild_instincts      = get_proc( "Wild Instincts");

  if ( tier_set.t30_sv_4pc.ok() )
    procs.t30_sv_4p          = get_proc( "Wildfire Bomb Reduction T304P" );
}

// hunter_t::init_rng =======================================================

void hunter_t::init_rng()
{
  player_t::init_rng();

  rppm.arctic_bola = get_rppm( "arctic_bola", talents.arctic_bola );
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

  if ( talents.terms_of_engagement.ok() )
  {
    assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, [this]( result_amount_type, action_state_t* s ) {
      if ( s -> result_amount > 0 )
        get_target_data( s -> target ) -> damaged = true;
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
                  name(), main_hand_weapon.type );
  }

  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    switch ( specialization() )
    {
    case HUNTER_BEAST_MASTERY:
      hunter_apl::beast_mastery( this );
      break;
    case HUNTER_MARKSMANSHIP:
      hunter_apl::marksmanship( this );
      break;
    case HUNTER_SURVIVAL:
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

    auto cb = new master_marksman_cb_t( *effect, talents.master_marksman -> effectN( 1 ).percent(), actions.master_marksman);
    cb -> initialize();
  }
}

// hunter_t::reset ==========================================================

void hunter_t::reset()
{
  player_t::reset();

  // Active
  pets.main = nullptr;
  state = {};

  if ( options.dire_pack_start > 0 )
    state.dire_pack_counter = options.dire_pack_start;
  else
    state.dire_pack_counter = rng().range( as<int>( talents.dire_pack -> effectN( 1 ).base_value() ) );
}

// hunter_t::merge ==========================================================

void hunter_t::merge( player_t& other )
{
  player_t::merge( other );

  cd_waste.merge( static_cast<hunter_t&>( other ).cd_waste );
}

void hunter_t::arise()
{
  player_t::arise();

  buffs.lone_wolf -> trigger();
}

// hunter_t::combat_begin ==================================================

void hunter_t::combat_begin()
{
  if ( talents.bloodseeker.ok() && ( talents.wildfire_infusion.ok() || sim -> player_no_pet_list.size() > 1 ) )
  {
    make_repeating_event( *sim, 1_s, [ this ] { trigger_bloodseeker_update(); } );
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

// hunter_t::composite_melee_crit_chance ===========================================

double hunter_t::composite_melee_crit_chance() const
{
  double crit = player_t::composite_melee_crit_chance();

  crit += specs.critical_strikes -> effectN( 1 ).percent();
  crit += talents.keen_eyesight -> effectN( 1 ).percent();
  crit += buffs.unerring_vision -> stack() * buffs.unerring_vision -> data().effectN( 1 ).percent();

  return crit;
}

// hunter_t::composite_spell_crit_chance ===========================================

double hunter_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  crit += specs.critical_strikes->effectN( 1 ).percent();
  crit += talents.keen_eyesight->effectN( 1 ).percent();
  crit += buffs.unerring_vision -> stack() * buffs.unerring_vision -> data().effectN( 1 ).percent();

  return crit;
}

// hunter_t::composite_melee_speed ==========================================

double hunter_t::composite_melee_speed() const
{
  double s = player_t::composite_melee_speed();

  if ( buffs.bloodseeker -> check() )
    s /= 1 + buffs.bloodseeker -> check_stack_value();

  return s;
}

// hunter_t::composite_player_target_crit_chance ============================

double hunter_t::composite_player_target_crit_chance( player_t* target ) const
{
  double crit = player_t::composite_player_target_crit_chance( target );

  crit += get_target_data( target ) -> debuffs.stampede -> check_value();

  return crit;
}

// hunter_t::composite_player_critical_damage_multiplier ====================

double hunter_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_critical_damage_multiplier( s );

  if ( talents.sharpshooter -> effectN( 1 ).has_common_school( s -> action -> school ) )
    m *= 1.0 + talents.sharpshooter -> effectN( 1 ).percent();

  if ( talents.sharp_edges -> effectN( 1 ).has_common_school( s -> action -> school ) )
    m *= 1.0 + talents.sharp_edges -> effectN( 1 ).percent();

  if ( buffs.unerring_vision -> data().effectN( 2 ).has_common_school( s -> action -> school ) )
    m *= 1.0 + buffs.unerring_vision -> stack() * buffs.unerring_vision -> data().effectN( 2 ).percent();

  return m;
}

// hunter_t::composite_player_multiplier ====================================

double hunter_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  return m;
}

// hunter_t::composite_player_target_multiplier ====================================

double hunter_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double d = player_t::composite_player_target_multiplier( target, school );

  auto td = get_target_data( target );
  if ( td -> debuffs.death_chakram -> has_common_school( school ) )
    d *= 1 + td -> debuffs.death_chakram -> value();

  return d;
}

// hunter_t::composite_player_pet_damage_multiplier ======================

double hunter_t::composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s, guardian );

  if ( mastery.master_of_beasts->ok() )
    m *= 1.0 + cache.mastery_value();

  m *= 1 + specs.beast_mastery_hunter -> effectN( 3 ).percent();
  m *= 1 + specs.survival_hunter -> effectN( 3 ).percent();
  m *= 1 + specs.marksmanship_hunter -> effectN( 3 ).percent();

  return m;
}

// hunter_t::composite_player_target_pet_damage_multiplier ======================

double hunter_t::composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const
{
  double m = player_t::composite_player_target_pet_damage_multiplier( target, guardian );

  m *= 1 + get_target_data( target ) -> debuffs.death_chakram -> value();

  return m;
}

double hunter_t::composite_leech() const
{
  double l = player_t::composite_leech();

  l += buffs.predators_thirst -> check_value();

  return l;
}

// hunter_t::invalidate_cache ==============================================

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

// hunter_t::regen =========================================================

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

// hunter_t::resource_gain =================================================

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

void hunter_t::reset_auto_attacks( timespan_t delay, proc_t* proc )
{
  // Keep Auto Shot suppressed while Volley is "channeled"
  if ( !buffs.volley -> check() || delay > buffs.volley -> remains() )
  {
    player_t::reset_auto_attacks( delay, proc );
  }
}

// hunter_t::matching_gear_multiplier =======================================

double hunter_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_AGILITY )
    return 0.05;

  return 0;
}

double hunter_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  ms += buffs.pathfinding -> check_value();

  return ms;
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
  add_option( opt_bool( "hunter.separate_wfi_stats", options.separate_wfi_stats ) );
  add_option( opt_int( "hunter.dire_pack_start", options.dire_pack_start, 0, 4 ) );
}

// hunter_t::create_profile =================================================

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
  print_option( &options_t::dire_pack_start, "hunter.dire_pack_start" );

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
