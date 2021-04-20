//==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <memory>

#include "simulationcraft.hpp"
#include "player/covenant.hpp"
#include "player/pet_spawner.hpp"
#include "class_modules/apl/hunter.hpp"

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

  fmt::format_to( out, "{} {} is affected by {}", *a->player, *a, spell.name_cstr() );
  if ( spell_text.rank() )
    fmt::format_to( out, " (desc={})", spell_text.rank() );
  fmt::format_to( out, " (id={}) effect#{}", spell.id(), effect.spell_effect_num() + 1 );
  if ( !label.empty() )
    fmt::format_to( out, ": {}", label );

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
  typedef std::pair<std::string, std::unique_ptr<action_data_t>> record_t;
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

struct hunter_t;

namespace pets
{
struct animal_companion_t;
struct hunter_main_pet_t;
struct dire_critter_t;
struct spitting_cobra_t;
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
    buff_t* latent_poison_injection;
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
    pets::spitting_cobra_t* spitting_cobra = nullptr;
    spawner::pet_spawner_t<pets::dire_critter_t, hunter_t> dc_dire_beast;

    pets_t( hunter_t* p ) : dc_dire_beast( "dire_beast_(dc)", p ) {}
  } pets;

  struct {
    spell_data_ptr_t death_chakram;
    spell_data_ptr_t flayed_shot;
    spell_data_ptr_t resonating_arrow;
    spell_data_ptr_t wild_spirits; // VERY WIP
  } covenants;

  struct {
    // Covenant
    conduit_data_t empowered_release;
    conduit_data_t enfeebled_mark;
    conduit_data_t necrotic_barrage;
    conduit_data_t spirit_attunement;
    // Beast Mastery
    conduit_data_t bloodletting;
    conduit_data_t echoing_call;
    conduit_data_t ferocious_appetite;
    conduit_data_t one_with_the_beast;
    // Marksmanship
    conduit_data_t brutal_projectiles;
    conduit_data_t deadly_chain;
    conduit_data_t powerful_precision;
    conduit_data_t sharpshooters_focus;
    // Survival
    conduit_data_t deadly_tandem;
    conduit_data_t flame_infusion;
    conduit_data_t reversal_of_fortune;
    conduit_data_t stinging_strike;
    conduit_data_t strength_of_the_pack;
  } conduits;

  struct {
    spell_data_ptr_t call_of_the_wild;
    spell_data_ptr_t craven_strategem; // NYI (Feign Death Utility)
    spell_data_ptr_t nesingwarys_apparatus;
    spell_data_ptr_t soulforge_embers;
    // Beast Mastery
    spell_data_ptr_t dire_command;
    spell_data_ptr_t flamewakers_cobra_sting;
    spell_data_ptr_t qapla_eredun_war_order;
    spell_data_ptr_t rylakstalkers_fangs;
    // Marksmanship
    spell_data_ptr_t eagletalons_true_focus;
    spell_data_ptr_t secrets_of_the_vigil;
    spell_data_ptr_t serpentstalkers_trickery;
    spell_data_ptr_t surging_shots;
    // Survival
    spell_data_ptr_t butchers_bone_fragments;
    spell_data_ptr_t latent_poison_injectors;
    spell_data_ptr_t rylakstalkers_strikes;
    spell_data_ptr_t wildfire_cluster;
  } legendary;

  // Buffs
  struct buffs_t
  {
    // Pet family buffs
    buff_t* endurance_training;
    buff_t* pathfinding;
    buff_t* predators_thirst;

    // Beast Mastery
    buff_t* aspect_of_the_wild;
    std::array<buff_t*, BARBED_SHOT_BUFFS_MAX> barbed_shot;
    buff_t* bestial_wrath;
    buff_t* dire_beast;
    buff_t* thrill_of_the_hunt;

    // Marksmanship
    buff_t* dead_eye;
    buff_t* double_tap;
    buff_t* lock_and_load;
    buff_t* lone_wolf;
    buff_t* precise_shots;
    buff_t* steady_focus;
    buff_t* streamline;
    buff_t* trick_shots;
    buff_t* trueshot;
    buff_t* volley;

    // Survival
    buff_t* aspect_of_the_eagle;
    buff_t* coordinated_assault;
    buff_t* mongoose_fury;
    buff_t* predator;
    buff_t* terms_of_engagement;
    buff_t* tip_of_the_spear;
    buff_t* vipers_venom;

    // Legendaries
    buff_t* butchers_bone_fragments;
    buff_t* eagletalons_true_focus;
    buff_t* flamewakers_cobra_sting;
    buff_t* nesingwarys_apparatus;
    buff_t* secrets_of_the_vigil;

    // Conduits
    buff_t* brutal_projectiles;
    buff_t* brutal_projectiles_hidden;
    buff_t* empowered_release;
    buff_t* flame_infusion;
    buff_t* strength_of_the_pack;

    // Covenants
    buff_t* flayers_mark;
    buff_t* resonating_arrow;
    buff_t* wild_spirits;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* a_murder_of_crows;
    cooldown_t* aimed_shot;
    cooldown_t* aspect_of_the_wild;
    cooldown_t* barbed_shot;
    cooldown_t* bestial_wrath;
    cooldown_t* harpoon;
    cooldown_t* kill_command;
    cooldown_t* kill_shot;
    cooldown_t* rapid_fire;
    cooldown_t* trueshot;
    cooldown_t* wildfire_bomb;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* aspect_of_the_wild;
    gain_t* barbed_shot;
    gain_t* nesingwarys_apparatus_direct;
    gain_t* nesingwarys_apparatus_buff;
    gain_t* reversal_of_fortune;
    gain_t* terms_of_engagement;
    gain_t* trueshot;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* lethal_shots;
    proc_t* wild_call;
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

    // tier 25
    spell_data_ptr_t scent_of_blood;
    spell_data_ptr_t one_with_the_pack;
    spell_data_ptr_t chimaera_shot;

    spell_data_ptr_t careful_aim;
    spell_data_ptr_t barrage;
    spell_data_ptr_t explosive_shot;

    spell_data_ptr_t guerrilla_tactics;
    spell_data_ptr_t hydras_bite;
    spell_data_ptr_t butchery;

    // tier 30
    spell_data_ptr_t trailblazer;
    spell_data_ptr_t natural_mending;
    spell_data_ptr_t camouflage;

    // tier 35
    spell_data_ptr_t spitting_cobra;
    spell_data_ptr_t thrill_of_the_hunt;
    spell_data_ptr_t a_murder_of_crows;

    spell_data_ptr_t steady_focus;
    spell_data_ptr_t streamline;

    spell_data_ptr_t bloodseeker;
    spell_data_ptr_t steel_trap;

    // tier 40
    spell_data_ptr_t born_to_be_wild;
    spell_data_ptr_t posthaste;
    spell_data_ptr_t binding_shot;

    spell_data_ptr_t binding_shackles;

    // tier 45
    spell_data_ptr_t stomp;
    spell_data_ptr_t stampede;

    spell_data_ptr_t lethal_shots;
    spell_data_ptr_t dead_eye;
    spell_data_ptr_t double_tap;

    spell_data_ptr_t tip_of_the_spear;
    spell_data_ptr_t mongoose_bite;
    spell_data_ptr_t flanking_strike;

    // tier 50
    spell_data_ptr_t aspect_of_the_beast;
    spell_data_ptr_t killer_cobra;
    spell_data_ptr_t bloodshed;

    spell_data_ptr_t calling_the_shots;
    spell_data_ptr_t lock_and_load;
    spell_data_ptr_t volley;

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

    spell_data_ptr_t kill_command;
    spell_data_ptr_t kill_shot;

    // Beast Mastery
    spell_data_ptr_t aspect_of_the_wild;
    spell_data_ptr_t barbed_shot;
    spell_data_ptr_t beast_cleave;
    spell_data_ptr_t bestial_wrath;
    spell_data_ptr_t kindred_spirits;
    spell_data_ptr_t pack_tactics;
    spell_data_ptr_t wild_call;

    // Marksmanship
    spell_data_ptr_t aimed_shot;
    spell_data_ptr_t lone_wolf;
    spell_data_ptr_t precise_shots;
    spell_data_ptr_t rapid_fire;
    spell_data_ptr_t trick_shots;
    spell_data_ptr_t trueshot;

    // Survival
    spell_data_ptr_t aspect_of_the_eagle;
    spell_data_ptr_t carve;
    spell_data_ptr_t coordinated_assault;
    spell_data_ptr_t harpoon;
    spell_data_ptr_t raptor_strike;
    spell_data_ptr_t wildfire_bomb;
  } specs;

  struct mastery_spells_t
  {
    spell_data_ptr_t master_of_beasts;
    spell_data_ptr_t sniper_training;
    spell_data_ptr_t spirit_bond;
  } mastery;

  struct {
    action_t* master_marksman = nullptr;
    action_t* wild_spirits = nullptr;
    // Semi-random actions, needed *ONLY* for properly attributing focus gains
    action_t* aspect_of_the_wild = nullptr;
    action_t* barbed_shot = nullptr;
  } actions;

  cdwaste::player_data_t cd_waste;

  struct {
    events::tar_trap_aoe_t* tar_trap_aoe = nullptr;
    wildfire_infusion_e next_wi_bomb = WILDFIRE_INFUSION_SHRAPNEL;
    unsigned steady_focus_counter = 0;
  } state;

  struct options_t {
    std::string summon_pet_str = "turtle";
    timespan_t pet_attack_speed = 2_s;
    timespan_t pet_basic_attack_delay = 0.15_s;
    // random testing stuff
    bool stomp_triggers_wild_spirits = true;
  } options;

  hunter_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) :
    player_t( sim, HUNTER, name, r ),
    pets( this ),
    buffs(),
    cooldowns(),
    gains(),
    procs()
  {
    // Cooldowns
    cooldowns.a_murder_of_crows   = get_cooldown( "a_murder_of_crows" );
    cooldowns.aimed_shot          = get_cooldown( "aimed_shot" );
    cooldowns.aspect_of_the_wild  = get_cooldown( "aspect_of_the_wild" );
    cooldowns.barbed_shot         = get_cooldown( "barbed_shot" );
    cooldowns.bestial_wrath       = get_cooldown( "bestial_wrath" );
    cooldowns.harpoon             = get_cooldown( "harpoon" );
    cooldowns.kill_command        = get_cooldown( "kill_command" );
    cooldowns.kill_shot           = get_cooldown( "kill_shot" );
    cooldowns.rapid_fire          = get_cooldown( "rapid_fire" );
    cooldowns.trueshot            = get_cooldown( "trueshot" );
    cooldowns.wildfire_bomb       = get_cooldown( "wildfire_bomb" );

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
  double    composite_player_multiplier( school_e school ) const override;
  double    composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double    composite_player_pet_damage_multiplier( const action_state_t* ) const override;
  double    composite_leech() const override;
  double    matching_gear_multiplier( attribute_e attr ) const override;
  double    passive_movement_modifier() const override;
  void      invalidate_cache( cache_e ) override;
  void      regen( timespan_t periodicity ) override;
  double    resource_gain( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;
  void      create_options() override;
  std::unique_ptr<expr_t> create_expression( util::string_view expression_str ) override;
  std::unique_ptr<expr_t> create_action_expression( action_t&, util::string_view expression_str ) override;
  action_t* create_action( util::string_view name, const std::string& options ) override;
  pet_t*    create_pet( util::string_view name, util::string_view type ) override;
  void      create_pets() override;
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

  void trigger_wild_spirits( const action_state_t* s );
  void trigger_birds_of_prey( player_t* t );
  void trigger_bloodseeker_update();
  void trigger_lethal_shots();
  void trigger_calling_the_shots();

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
  maybe_bool triggers_wild_spirits;

  struct {
    // bm
    bool aotw_crit_chance = false;
    bool aotw_gcd_reduce = false;
    damage_affected_by bestial_wrath;
    damage_affected_by master_of_beasts;
    bool thrill_of_the_hunt = false;
    // mm
    damage_affected_by lone_wolf;
    bool precise_shots = false;
    damage_affected_by sniper_training;
    // surv
    damage_affected_by coordinated_assault;
    damage_affected_by spirit_bond;
  } affected_by;

  cdwaste::action_data_t* cd_waste = nullptr;

  hunter_action_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
    ab( n, p, s ),
    track_cd_waste( s -> cooldown() > 0_ms || s -> charge_cooldown() > 0_ms )
  {
    ab::special = true;

    affected_by.aotw_crit_chance    = check_affected_by( this, p -> specs.aspect_of_the_wild -> effectN( 1 ) );
    affected_by.aotw_gcd_reduce     = check_affected_by( this, p -> specs.aspect_of_the_wild -> effectN( 3 ) );
    affected_by.bestial_wrath       = parse_damage_affecting_aura( this, p -> specs.bestial_wrath );
    affected_by.master_of_beasts    = parse_damage_affecting_aura( this, p -> mastery.master_of_beasts );
    affected_by.thrill_of_the_hunt  = check_affected_by( this, p -> talents.thrill_of_the_hunt -> effectN( 1 ).trigger() -> effectN( 1 ) );

    affected_by.lone_wolf           = parse_damage_affecting_aura( this, p -> specs.lone_wolf );
    affected_by.sniper_training     = parse_damage_affecting_aura( this, p -> mastery.sniper_training );

    affected_by.coordinated_assault = parse_damage_affecting_aura( this, p -> specs.coordinated_assault );
    affected_by.spirit_bond         = parse_damage_affecting_aura( this, p -> mastery.spirit_bond );

    // passive talents
    ab::apply_affecting_aura( p -> talents.alpha_predator );
    ab::apply_affecting_aura( p -> talents.born_to_be_wild );
    ab::apply_affecting_aura( p -> talents.dead_eye );
    ab::apply_affecting_aura( p -> talents.guerrilla_tactics );
    ab::apply_affecting_aura( p -> talents.hydras_bite );
    ab::apply_affecting_aura( p -> talents.streamline );

    // "simple" passive rank 2 spells
    ab::apply_affecting_aura( p -> find_rank_spell( "Arcane Shot", "Rank 2" ) );
    ab::apply_affecting_aura( p -> find_rank_spell( "Cobra Shot", "Rank 2" ) );
    ab::apply_affecting_aura( p -> find_rank_spell( "Harpoon", "Rank 2" ) );
    ab::apply_affecting_aura( p -> find_rank_spell( "Improved Traps", "Rank 2" ) );
    ab::apply_affecting_aura( p -> find_rank_spell( "Kill Shot", "Rank 2" ) );
    ab::apply_affecting_aura( p -> find_rank_spell( "Wildfire Bombs", "Rank 2" ) );
    ab::apply_affecting_aura( p -> find_specialization_spell( "True Aim" ) );

    // passive legendary effects
    ab::apply_affecting_aura( p -> legendary.call_of_the_wild );
    ab::apply_affecting_aura( p -> legendary.qapla_eredun_war_order );
    ab::apply_affecting_aura( p -> legendary.surging_shots );

    // passive conduits
    ab::apply_affecting_conduit( p -> conduits.bloodletting );
    ab::apply_affecting_conduit( p -> conduits.necrotic_barrage );
    ab::apply_affecting_conduit( p -> conduits.spirit_attunement );
    ab::apply_affecting_conduit( p -> conduits.stinging_strike );
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

    if ( p() -> covenants.wild_spirits.ok() )
    {
      // By default nothing procs Wild Spirits, if the action did not explicitly set
      // it to non-default try to infer it
      if ( triggers_wild_spirits.is_none() )
      {
        const spell_data_ptr_t trigger = p() -> covenants.wild_spirits -> effectN( 1 ).trigger();
        triggers_wild_spirits = ab::harmful && ab::may_hit && ab::callbacks && !ab::proc &&
                                ( trigger -> proc_flags() & ( 1 << ab::proc_type() ) );
      }
    }
    else
    {
      triggers_wild_spirits = false;
    }

    if ( triggers_wild_spirits )
      ab::sim -> print_debug( "{} action {} set to proc Wild Spirits", ab::player -> name(), ab::name() );
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
      if ( ab::gcd_type != gcd_haste_type::NONE )
        g *= ab::composite_haste();
    }

    if ( g < ab::min_gcd )
      g = ab::min_gcd;

    return g;
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( triggers_wild_spirits )
      p() -> trigger_wild_spirits( s );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double am = ab::composite_da_multiplier( s );

    if ( affected_by.bestial_wrath.direct )
      am *= 1 + p() -> buffs.bestial_wrath -> check_value();

    if ( affected_by.coordinated_assault.direct )
      am *= 1 + p() -> buffs.coordinated_assault -> check_value();

    if ( affected_by.master_of_beasts.direct )
      am *= 1 + p() -> cache.mastery() * p() -> mastery.master_of_beasts -> effectN( affected_by.master_of_beasts.direct ).mastery_value();

    if ( affected_by.sniper_training.direct )
      am *= 1 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( affected_by.sniper_training.direct ).mastery_value();

    if ( affected_by.spirit_bond.direct )
      am *= 1 + p() -> cache.mastery() * p() -> mastery.spirit_bond -> effectN( affected_by.spirit_bond.direct ).mastery_value();

    if ( affected_by.lone_wolf.direct )
      am *= 1 + p() -> buffs.lone_wolf -> check_value();

    return am;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double am = ab::composite_ta_multiplier( s );

    if ( affected_by.bestial_wrath.tick )
      am *= 1 + p() -> buffs.bestial_wrath -> check_value();

    if ( affected_by.coordinated_assault.tick )
      am *= 1 + p() -> buffs.coordinated_assault -> check_value();

    if ( affected_by.sniper_training.tick )
      am *= 1 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( affected_by.sniper_training.tick ).mastery_value();

    if ( affected_by.spirit_bond.tick )
      am *= 1 + p() -> cache.mastery() * p() -> mastery.spirit_bond -> effectN( affected_by.spirit_bond.tick ).mastery_value();

    if ( affected_by.lone_wolf.tick )
      am *= 1 + p() -> buffs.lone_wolf -> check_value();

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
    double c = ab::cost();

    if ( p() -> buffs.eagletalons_true_focus -> check() )
      c *= 1 + p() -> buffs.eagletalons_true_focus -> check_value();

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
                     p() -> specs.trueshot -> effectN( 6 ).percent();
      if ( execute_time < remains )
        total_energize *= 1 + p() -> specs.trueshot -> effectN( 5 ).percent();
    }

    if ( p() -> buffs.nesingwarys_apparatus -> check() &&
         execute_time < p() -> buffs.nesingwarys_apparatus -> remains() )
    {
      total_energize *= 1 + p() -> buffs.nesingwarys_apparatus -> check_value();
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

  bool trigger_buff( buff_t *const buff, timespan_t precast_time ) const
  {
    const bool in_combat = ab::player -> in_combat;
    const bool triggered = buff -> trigger();
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
  maybe_bool triggers_master_marksman;

  hunter_ranged_attack_t( util::string_view n, hunter_t* p,
                          const spell_data_t* s = spell_data_t::nil() ):
                          hunter_action_t( n, p, s )
  {
    affected_by.precise_shots = p -> specs.precise_shots.ok() &&
                                parse_damage_affecting_aura( this, p -> find_spell( 260242 ) ).direct;
  }

  void init() override
  {
    hunter_action_t::init();

    if ( p() -> talents.master_marksman.ok() )
    {
      if ( triggers_master_marksman.is_none() )
        triggers_master_marksman = harmful && special && may_crit;
    }
    else
    {
      triggers_master_marksman = false;
    }

    if ( triggers_master_marksman )
      sim -> print_debug( "{} action {} set to proc Master Marksman", player -> name(), name() );
  }

  bool usable_moving() const override
  { return true; }

  void execute() override
  {
    hunter_action_t::execute();

    if ( !background && breaks_steady_focus )
      p() -> state.steady_focus_counter = 0;
  }

  void impact( action_state_t* s ) override
  {
    hunter_action_t::impact( s );

    if ( triggers_master_marksman && s -> result == RESULT_CRIT )
    {
      double amount = s -> result_amount * p() -> talents.master_marksman -> effectN( 1 ).percent();
      if ( amount > 0 )
        residual_action::trigger( p() -> actions.master_marksman, s -> target, amount );
    }
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

  double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( o() -> mastery.master_of_beasts.ok() )
      m *= 1 + owner -> cache.mastery_value();

    return m;
  }

  double composite_player_target_crit_chance( player_t* target ) const override
  {
    double crit = pet_t::composite_player_target_crit_chance( target );

    crit += o()->buffs.resonating_arrow->check_value();

    return crit;
  }

  double composite_player_target_multiplier( player_t* target, school_e school ) const override
  {
    double m = pet_t::composite_player_target_multiplier( target, school );

    m *= 1 + o() -> buffs.wild_spirits -> check_value();

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

// Base class for main pet & Animal Companion
struct hunter_main_pet_base_t : public hunter_pet_t
{
  struct actives_t
  {
    action_t* basic_attack = nullptr;
    action_t* beast_cleave = nullptr;
    action_t* bestial_wrath = nullptr;
    action_t* bloodshed = nullptr;
    action_t* flanking_strike = nullptr;
    action_t* kill_command = nullptr;
    action_t* stomp = nullptr;
  } active;

  struct buffs_t
  {
    buff_t* beast_cleave = nullptr;
    buff_t* bestial_wrath = nullptr;
    buff_t* frenzy = nullptr;
    buff_t* predator = nullptr;
    buff_t* rylakstalkers_fangs = nullptr;
  } buffs;

  struct {
    spell_data_ptr_t bloodshed;
    spell_data_ptr_t thrill_of_the_hunt;
  } spells;

  hunter_main_pet_base_t( hunter_t* owner, util::string_view pet_name, pet_e pt ):
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
        -> set_default_value( find_spell( 186254 ) -> effectN( 1 ).percent() +
                              o() -> conduits.one_with_the_beast.percent() )
        -> set_cooldown( 0_ms )
        -> set_stack_change_callback( [this]( buff_t*, int old, int cur ) {
            if ( cur == 0 )
              buffs.rylakstalkers_fangs -> expire();
            else if ( old == 0 )
              buffs.rylakstalkers_fangs -> trigger();
          } );

    buffs.beast_cleave =
      make_buff( this, "beast_cleave", find_spell( 118455 ) )
        -> set_default_value( o() -> specs.beast_cleave -> effectN( 1 ).percent() )
        -> set_chance( o() -> specs.beast_cleave.ok() );

    buffs.frenzy =
      make_buff( this, "frenzy", o() -> find_spell( 272790 ) )
        -> set_default_value_from_effect( 1 )
        -> add_invalidate( CACHE_ATTACK_SPEED );

    buffs.rylakstalkers_fangs =
      make_buff( this, "rylakstalkers_piercing_fangs", o() -> find_spell( 336845 ) )
        -> set_default_value_from_effect( 1 )
        -> set_chance( o() -> legendary.rylakstalkers_fangs.ok() );
  }

  double composite_melee_speed() const override
  {
    double ah = hunter_pet_t::composite_melee_speed();

    if ( buffs.frenzy -> check() )
      ah /= 1 + buffs.frenzy -> check_stack_value();

    if ( buffs.predator && buffs.predator -> check() )
      ah /= 1 + buffs.predator -> check_stack_value();

    return ah;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = hunter_pet_t::composite_player_multiplier( school );

    m *= 1 + buffs.bestial_wrath -> check_value();
    m *= 1 + o() -> buffs.coordinated_assault -> check_value();

    return m;
  }

  double composite_player_target_multiplier( player_t* target, school_e school ) const override;

  double composite_melee_crit_chance() const override
  {
    double cc = hunter_pet_t::composite_melee_crit_chance();

    if ( o() -> buffs.thrill_of_the_hunt -> check() )
      cc += o() -> buffs.thrill_of_the_hunt -> check() * spells.thrill_of_the_hunt -> effectN( 1 ).percent();

    return cc;
  }

  double composite_player_critical_damage_multiplier( const action_state_t* s ) const override
  {
    double m = hunter_pet_t::composite_player_critical_damage_multiplier( s );

    m *= 1 + buffs.rylakstalkers_fangs -> check_value();

    return m;
  }

  void init_spells() override;

  void moving() override
  { return; }
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
    dot_t* bloodseeker = nullptr;
    dot_t* bloodshed = nullptr;
  } dots;

  hunter_main_pet_td_t( player_t* target, hunter_main_pet_t* p );
};

struct hunter_main_pet_t final : public hunter_main_pet_base_t
{
  struct gains_t
  {
    gain_t* aspect_of_the_wild = nullptr;
  } gains;

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
    resources.base[RESOURCE_FOCUS] = 100 + o() -> specs.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS );

    base_gcd = 1.5_s;

    resources.infinite_resource[RESOURCE_FOCUS] = o() -> resources.infinite_resource[RESOURCE_FOCUS];
  }

  void create_buffs() override
  {
    hunter_main_pet_base_t::create_buffs();

    buffs.predator =
      make_buff( this, "predator", o() -> find_spell( 260249 ) )
        -> set_default_value_from_effect( 1 )
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
      return owner -> resource_regen_per_second( RESOURCE_FOCUS ) * 1.25;

    return hunter_main_pet_base_t::resource_regen_per_second( r );
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    hunter_main_pet_base_t::summon( duration );

    o() -> pets.main = this;
    if ( o()->pets.animal_companion )
    {
      o()->pets.animal_companion->summon();
      o()->pets.animal_companion->schedule_ready(0_s, false);
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
    // 23/07/2018 - hunter pets seem to have a "generic" lag of about .6s on basic attack usage
    const auto delay_mean = o() -> options.pet_basic_attack_delay;
    const auto delay_stddev = 100_ms;
    const auto lag = o() -> bugs ? rng().gauss( delay_mean, delay_stddev ) : 0_ms;
    return std::max( remains + lag, 100_ms );
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override;

  void init_spells() override;
};

double hunter_main_pet_base_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = hunter_pet_t::composite_player_target_multiplier( target, school );

  if ( auto main_pet = o() -> pets.main ) // theoretically should always be there /shrug
  {
    const hunter_main_pet_td_t* td = main_pet -> find_target_data( target );
    if ( td && td -> dots.bloodshed -> is_ticking() )
      m *= 1 + spells.bloodshed -> effectN( 2 ).percent();
  }

  if ( o() -> conduits.enfeebled_mark.ok() && o() -> buffs.resonating_arrow -> check() )
    m *= 1 + o() -> conduits.enfeebled_mark.percent();

  return m;
}

// ==========================================================================
// Dire Critter
// ==========================================================================

struct dire_critter_t final : public hunter_pet_t
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

  dire_critter_t( hunter_t* owner, util::string_view n = "dire_beast" ):
    hunter_pet_t( owner, n, PET_HUNTER, true /* GUARDIAN */, true /* dynamic */ )
  {
    owner_coeff.ap_from_ap = 0.15;
    resource_regeneration = regen_type::DISABLED;
  }

  void init_spells() override;

  void summon( timespan_t duration = 0_ms ) override
  {
    hunter_pet_t::summon( duration );

    o() -> buffs.dire_beast -> trigger();

    if ( o() -> talents.stomp.ok() )
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
  const timespan_t base_duration = p -> buffs.dire_beast -> buff_duration();
  const timespan_t swing_time = 2_s * p -> cache.attack_speed();
  double partial_attacks_per_summon = base_duration / swing_time;
  int base_attacks_per_summon = static_cast<int>( partial_attacks_per_summon );
  partial_attacks_per_summon -= static_cast<double>( base_attacks_per_summon );

  if ( p -> rng().roll( partial_attacks_per_summon ) )
    base_attacks_per_summon += 1;

  return { base_attacks_per_summon * swing_time, base_attacks_per_summon };
}

// ==========================================================================
// Spitting Cobra
// ==========================================================================

struct spitting_cobra_t final : public hunter_pet_t
{
  struct cobra_spit_t: public hunter_pet_action_t<spitting_cobra_t, spell_t>
  {
    cobra_spit_t( spitting_cobra_t* p, util::string_view options_str ):
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

  unsigned cobra_shot_count = 0;
  double active_damage_multiplier = 0;

  spitting_cobra_t( hunter_t* o ):
    hunter_pet_t( o, "spitting_cobra", PET_HUNTER, true /* GUARDIAN */, true /* dynamic */ )
  {
    owner_coeff.ap_from_ap = 0.15;

    // 25 Feb 2021
    // The patch note for the PTR says increased BY 260%, when in fact it was
    // only increased by 160% or otherwise increased TO 260% of the live value.
    owner_coeff.ap_from_ap *= 2.6;

    resource_regeneration = regen_type::DISABLED;

    action_list_str = "cobra_spit";
  }

  action_t* create_action( util::string_view name,
                           const std::string& options_str ) override
  {
    if ( name == "cobra_spit" )
      return new cobra_spit_t( this, options_str );

    return hunter_pet_t::create_action( name, options_str );
  }

  // for some reason it gets the player's multipliers
  double composite_player_multiplier( school_e school ) const override
  {
    double m = owner -> composite_player_multiplier( school );

    m *= 1 + owner -> cache.mastery_value();
    m *= 1 + active_damage_multiplier;

    return m;
  }

  void summon( timespan_t duration = 0_ms ) override
  {
    active_damage_multiplier =
      o() -> talents.spitting_cobra -> effectN( 1 ).percent() * cobra_shot_count;
    cobra_shot_count = 0;

    // XXX: add some kind of stats/reporting to html report?
    if ( sim -> debug )
      sim -> print_debug( "{} summoning Spitting Cobra: dmg_mult={:.1f}", o() -> name(), active_damage_multiplier );

    hunter_pet_t::summon( duration );
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
  kill_command_base_t( hunter_main_pet_base_t* p, const spell_data_t* s ):
    hunter_pet_action_t( "kill_command", p, s )
  {
    background = true;
    proc = true;
  }

  double composite_attack_power() const override
  {
    // Kill Command for both Survival & Beast Mastery uses player AP directly
    return o() -> cache.attack_power() * o() -> composite_attack_power_multiplier();
  }
};

struct kill_command_bm_t: public kill_command_base_t
{
  struct {
    double percent = 0;
    double multiplier = 1;
    benefit_t* benefit = nullptr;
  } killer_instinct;
  struct {
    double chance = 0;
    proc_t* proc;
  } dire_command;
  timespan_t ferocious_appetite_reduction;

  kill_command_bm_t( hunter_main_pet_base_t* p ):
    kill_command_base_t( p, p -> find_spell( 83381 ) )
  {
    attack_power_mod.direct = o() -> specs.kill_command -> effectN( 2 ).percent();

    if ( o() -> talents.killer_instinct.ok() )
    {
      killer_instinct.percent = o() -> talents.killer_instinct -> effectN( 2 ).base_value();
      killer_instinct.multiplier = 1 + o() -> talents.killer_instinct -> effectN( 1 ).percent();
      killer_instinct.benefit = o() -> get_benefit( "killer_instinct" );
    }

    if ( o() -> conduits.ferocious_appetite.ok() )
      ferocious_appetite_reduction = timespan_t::from_seconds( o() -> conduits.ferocious_appetite.value() / 10 );

    if ( o() -> legendary.dire_command.ok() )
    {
      // assume it's a flat % without any bs /shrug
      dire_command.chance = o() -> legendary.dire_command -> effectN( 1 ).percent();
      dire_command.proc = o() -> get_proc( "Dire Command" );
    }
  }

  void execute() override
  {
    kill_command_base_t::execute();

    if ( rng().roll( dire_command.chance ) )
    {
      o() -> pets.dc_dire_beast.spawn( pets::dire_beast_duration( o() ).first );
      dire_command.proc -> occur();
    }
  }

  void impact( action_state_t* s ) override
  {
    kill_command_base_t::impact( s );

    if ( ferocious_appetite_reduction != 0_s && s -> result == RESULT_CRIT )
      o() -> cooldowns.aspect_of_the_wild -> adjust( -ferocious_appetite_reduction );
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

    if ( ! o() -> talents.bloodseeker.ok() )
      dot_duration = 0_ms;

    base_dd_multiplier *= 1 + o() -> talents.alpha_predator -> effectN( 2 ).percent();
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

  auto p = debug_cast<hunter_main_pet_base_t*>( s -> action -> player );

  if ( !p -> buffs.beast_cleave -> check() )
    return;

  const double multiplier = p -> buffs.beast_cleave -> check_value();

  // Target multipliers do not replicate to secondary targets
  const double target_da_multiplier = ( 1.0 / s -> target_da_multiplier );

  const double amount = s -> result_total * multiplier * target_da_multiplier;
  p -> active.beast_cleave -> set_target( s -> target );
  p -> active.beast_cleave -> base_dd_min = amount;
  p -> active.beast_cleave -> base_dd_max = amount;
  p -> active.beast_cleave -> execute();
}

// Pet Melee ================================================================

struct pet_melee_t : public hunter_pet_melee_t<hunter_main_pet_base_t>
{
  pet_melee_t( util::string_view n, hunter_main_pet_base_t* p ):
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

  void impact( action_state_t* s ) override
  {
    hunter_pet_action_t::impact( s );

    // Animal Companion can't proc Wild Spirits, but Dire Beast can
    if ( player != o() -> pets.animal_companion && o() -> options.stomp_triggers_wild_spirits )
      o() -> trigger_wild_spirits( s );
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

    if ( player == o() -> pets.main )
      o() -> trigger_wild_spirits( s );

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

    if ( player == o() -> pets.main )
      o() -> trigger_wild_spirits( s );
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
                                            const std::string& options_str )
{
  if ( name == "claw" ) return new        actions::basic_attack_t( this, "Claw", options_str );
  if ( name == "bite" ) return new        actions::basic_attack_t( this, "Bite", options_str );
  if ( name == "smack" ) return new       actions::basic_attack_t( this, "Smack", options_str );

  return hunter_main_pet_base_t::create_action( name, options_str );
}

// hunter_t::init_spells ====================================================

void hunter_main_pet_base_t::init_spells()
{
  hunter_pet_t::init_spells();

  spells.bloodshed          = find_spell( 321538 );
  spells.thrill_of_the_hunt = find_spell( 312365 );

  main_hand_attack = new actions::pet_melee_t( "melee", this );

  if ( o() -> specialization() == HUNTER_BEAST_MASTERY )
    active.bestial_wrath = new actions::bestial_wrath_t( this );

  if ( o() -> specialization() == HUNTER_BEAST_MASTERY )
    active.kill_command = new actions::kill_command_bm_t( this );
  else if ( o() -> specialization() == HUNTER_SURVIVAL )
    active.kill_command = new actions::kill_command_sv_t( this );

  if ( o() -> specs.beast_cleave.ok() )
    active.beast_cleave = new actions::beast_cleave_attack_t( this );

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
}

void dire_critter_t::init_spells()
{
  hunter_pet_t::init_spells();

  main_hand_attack = new dire_beast_melee_t( this );

  if ( o() -> talents.stomp.ok() )
    active.stomp = new actions::stomp_t( this );
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

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool ret = buff_t::trigger( stacks, value, chance, duration );

    if ( ret )
    {
      hunter_t* p = debug_cast<hunter_t*>( player );
      if ( p -> buffs.secrets_of_the_vigil -> trigger() )
        p -> cooldowns.aimed_shot -> reset( true );
    }

    return ret;
  }
};

} // namespace buffs

void hunter_t::trigger_wild_spirits( const action_state_t* s )
{
  if ( !covenants.wild_spirits.ok() )
    return;

  if ( s -> chain_target != 0 )
    return;

  if ( !buffs.wild_spirits -> check() )
    return;

  if ( !action_t::result_is_hit( s -> result ) )
    return;

  if ( sim -> debug )
  {
    sim -> print_debug( "{} procs wild_spirits from {} on {}",
                        name(), s -> action -> name(), s -> target -> name() );
  }

  actions.wild_spirits -> set_target( s -> target );
  actions.wild_spirits -> execute();
}

void hunter_t::trigger_birds_of_prey( player_t* t )
{
  if ( !talents.birds_of_prey.ok() )
    return;

  if ( !pets.main )
    return;

  if ( t == pets.main -> target )
    buffs.coordinated_assault -> extend_duration( this, talents.birds_of_prey -> effectN( 1 ).time_value() );
}

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
  bleeding_targets = std::min( bleeding_targets, buffs.predator -> max_stack() );

  const int current = buffs.predator -> check();
  if ( current < bleeding_targets )
  {
    buffs.predator -> trigger( bleeding_targets - current );
    if ( auto pet = pets.main )
      pet -> buffs.predator -> trigger( bleeding_targets - current );
  }
  else if ( current > bleeding_targets )
  {
    buffs.predator -> decrement( current - bleeding_targets );
    if ( auto pet = pets.main )
      pet -> buffs.predator -> decrement( current - bleeding_targets );
  }
}

void hunter_t::trigger_lethal_shots()
{
  if ( !talents.lethal_shots.ok() )
    return;

  if ( rng().roll( talents.lethal_shots -> proc_chance() ) )
  {
    const auto base_value = talents.lethal_shots -> effectN( 1 ).base_value();
    cooldowns.rapid_fire -> adjust( -timespan_t::from_seconds( base_value / 10 ) );
    procs.lethal_shots -> occur();
  }
}

void hunter_t::trigger_calling_the_shots()
{
  if ( !talents.calling_the_shots.ok() )
    return;

  cooldowns.trueshot -> adjust( - talents.calling_the_shots -> effectN( 1 ).time_value() );
}

void hunter_t::consume_trick_shots()
{
  if ( buffs.volley -> up() )
    return;

  buffs.trick_shots -> decrement();
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
// Covenant Abilities
// ==========================================================================

namespace death_chakram
{
/**
 * A whole namespace for a single spell...
 *
 * The spell consists of at least 3 spells:
 *   325028 - main spell, does all damage on multiple targets
 *   325037 - additional periodic damage done on single target
 *   325553 - energize (amount is in 325028)
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
 * 2020.08.11 Additional tests performed by Ghosteld, Putro, Tirrill & Laquan:
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
    chain_multiplier = p -> covenants.death_chakram -> effectN( 1 ).chain_multiplier();

    energize_type = action_energize::PER_HIT;
    energize_resource = RESOURCE_FOCUS;
    energize_amount = p -> covenants.death_chakram -> effectN( 4 ).base_value() +
                      p -> conduits.necrotic_barrage -> effectN( 2 ).base_value();
  }
};

struct single_target_t final : base_t
{
  int hit_number = 0;
  int max_hit_number;

  single_target_t( util::string_view n, hunter_t* p ):
    base_t( n, p, p -> find_spell( 325037 ) ),
    max_hit_number( p -> covenants.death_chakram -> effectN( 1 ).chain_target() )
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
    set_target( target_ );
    execute();
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
    death_chakram::base_t( "death_chakram", p, p -> covenants.death_chakram ),
    single_target( p -> get_background_action<death_chakram::single_target_t>( "death_chakram_st" ) )
  {
    parse_options( options_str );

    radius = 8; // Tested on 2020-08-11
    aoe = data().effectN( 1 ).chain_target();
  }

  void init() override
  {
    base_t::init();

    single_target -> gain  = gain;
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

struct flayed_shot_t : hunter_ranged_attack_t
{
  flayed_shot_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "flayed_shot", p, p -> covenants.flayed_shot )
  {
    parse_options( options_str );
  }

  void tick( dot_t* d ) override
  {
    hunter_ranged_attack_t::tick( d );

    if ( p() -> buffs.flayers_mark -> trigger() )
    {
      p() -> cooldowns.kill_shot -> reset( true );
      p() -> buffs.empowered_release -> trigger();
    }
  }
};

struct resonating_arrow_t : hunter_spell_t
{
  struct damage_t final : hunter_spell_t
  {
    damage_t( util::string_view n, hunter_t* p ):
      hunter_spell_t( n, p, p -> covenants.resonating_arrow -> effectN( 1 ).trigger() )
    {
      dual = true;
      aoe = -1;
      triggers_master_marksman = false;
    }

    void execute() override
    {
      hunter_spell_t::execute();

      p() -> buffs.resonating_arrow -> trigger();
    }
  };

  resonating_arrow_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "resonating_arrow", p, p -> covenants.resonating_arrow )
  {
    parse_options( options_str );

    impact_action = p -> get_background_action<damage_t>( "resonating_arrow_damage" );
    impact_action -> stats = stats;
    stats -> action_list.push_back( impact_action );
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( data().missile_speed() );
  }
};

struct wild_spirits_t : hunter_spell_t
{
  struct damage_t final : hunter_spell_t
  {
    damage_t( util::string_view n, hunter_t* p ):
      hunter_spell_t( n, p, p -> covenants.wild_spirits -> effectN( 1 ).trigger() )
    {
      dual = true;
      aoe = -1;
      triggers_wild_spirits = false;
      triggers_master_marksman = false;
    }

    void execute() override
    {
      hunter_spell_t::execute();

      p() -> buffs.wild_spirits -> trigger();
    }
  };

  struct proc_t final : hunter_spell_t
  {
    proc_t( util::string_view n, hunter_t* p ):
      hunter_spell_t( n, p, p -> find_spell( 328757 ) )
    {
      proc = true;
      callbacks = false;
      triggers_master_marksman = false;

      // 2020-12-07 hotfix:
      //     Damage of Wild Spirits has been increased by 25% for Marksmanship Hunters.
      // A random multiplier out of nowhere not present in the spell data
      if ( p -> specialization() == HUNTER_MARKSMANSHIP )
        base_multiplier *= 1.25;
    }
  };

  wild_spirits_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "wild_spirits", p, p -> covenants.wild_spirits )
  {
    parse_options( options_str );

    triggers_wild_spirits = false;

    impact_action = p -> get_background_action<damage_t>( "wild_spirits_damage" );
    impact_action -> stats = stats;
    stats -> action_list.push_back( impact_action );

    p -> actions.wild_spirits = p -> get_background_action<proc_t>( "wild_spirits_proc" );
    add_child( p -> actions.wild_spirits );
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

  auto_shot_t( hunter_t* p ) :
    auto_attack_base_t( "auto_shot", p, p -> find_spell( 75 ) )
  {
    wild_call_chance = p -> specs.wild_call -> proc_chance() +
                       p -> talents.one_with_the_pack -> effectN( 1 ).percent() +
                       p -> conduits.echoing_call.percent();
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

    if ( s -> result == RESULT_CRIT && p() -> specs.wild_call.ok() && rng().roll( wild_call_chance ) )
    {
      p() -> cooldowns.barbed_shot -> reset( true );
      p() -> procs.wild_call -> occur();
    }

    p() -> buffs.brutal_projectiles -> trigger();
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
      radius = 0; //Barrage attacks all targets in front of the hunter, so setting radius to 0 will prevent distance targeting from using a 40 yard radius around the target.
      // Todo: Add in support to only hit targets in the frontal cone.
    }

    void impact( action_state_t* s ) override
    {
      hunter_ranged_attack_t::impact( s );

      // XXX: Wild Spirits kludge
      if ( s -> chain_target == 0 )
        triggers_wild_spirits = false;
    }
  };

  barrage_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "barrage", p, p -> talents.barrage )
  {
    parse_options( options_str );

    may_miss = may_crit = false;
    channeled = true;
    triggers_wild_spirits = false;

    tick_action = p -> get_background_action<damage_t>( "barrage_damage" );
    starved_proc = p -> get_proc( "starved: barrage" );
  }

  void schedule_execute( action_state_t* state = nullptr ) override
  {
    // XXX: Wild Spirits kludge
    static_cast<damage_t*>( tick_action ) -> triggers_wild_spirits = true;

    hunter_spell_t::schedule_execute( state );
  }
};

// Multi Shot Attack =================================================================

struct multi_shot_t: public hunter_ranged_attack_t
{
  multi_shot_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "multishot", p, p -> find_class_spell( "Multi-Shot" ) )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> buffs.precise_shots -> up(); // benefit tracking
    p() -> buffs.precise_shots -> decrement();

    for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p() -> pets.main, p() -> pets.animal_companion ) )
      pet -> buffs.beast_cleave -> trigger();

    if ( num_targets_hit >= p() -> specs.trick_shots -> effectN( 2 ).base_value() )
      p() -> buffs.trick_shots -> trigger();

    p() -> trigger_calling_the_shots();
    p() -> trigger_lethal_shots();
  }
};

// Kill Shot ! =======================================================================

struct kill_shot_t : hunter_ranged_attack_t
{
  double health_threshold_pct;

  kill_shot_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "kill_shot", p, p -> specs.kill_shot ),
    health_threshold_pct( p -> specs.kill_shot -> effectN( 2 ).base_value() )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> buffs.flayers_mark -> up(); // benefit tracking
    p() -> buffs.flayers_mark -> decrement();
    p() -> buffs.empowered_release -> decrement();
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      p() -> buffs.dead_eye -> trigger();
  }

  double cost() const override
  {
    if ( p() -> buffs.flayers_mark -> check() )
      return 0;

    return hunter_ranged_attack_t::cost();
  }

  bool target_ready( player_t* candidate_target ) override
  {
    return ( candidate_target -> health_percentage() <= health_threshold_pct ||
             p() -> buffs.flayers_mark -> check() ) &&
           hunter_ranged_attack_t::target_ready( candidate_target );
  }

  double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    am *= 1 + p() -> buffs.flayers_mark -> check_value();
    am *= 1 + p() -> buffs.empowered_release -> check_value();

    return am;
  }
};

// Arcane Shot ========================================================================

struct arcane_shot_t: public hunter_ranged_attack_t
{
  arcane_shot_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "arcane_shot", p, p -> find_class_spell( "Arcane Shot" ) )
  {
    parse_options( options_str );

    if ( p -> specialization() == HUNTER_MARKSMANSHIP )
      background = p -> talents.chimaera_shot.ok();
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> trigger_lethal_shots();
    p() -> trigger_calling_the_shots();

    p() -> buffs.precise_shots -> up(); // benefit tracking
    p() -> buffs.precise_shots -> decrement();
  }
};

//==============================
// Beast Mastery attacks
//==============================

// Cobra Shot Attack =================================================================

struct cobra_shot_t: public hunter_ranged_attack_t
{
  const timespan_t kill_command_reduction;

  cobra_shot_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "cobra_shot", p, p -> find_specialization_spell( "Cobra Shot" ) ),
    kill_command_reduction( timespan_t::from_seconds( data().effectN( 3 ).base_value() ) )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> cooldowns.kill_command -> adjust( -kill_command_reduction );

    if ( p() -> talents.killer_cobra.ok() && p() -> buffs.bestial_wrath -> check() )
      p() -> cooldowns.kill_command -> reset( true );

    if ( p() -> talents.spitting_cobra.ok() && p() -> buffs.bestial_wrath -> check() )
      p() -> pets.spitting_cobra -> cobra_shot_count++;

    p() -> buffs.flamewakers_cobra_sting -> trigger();
  }
};

// Chimaera Shot =============================================================

struct chimaera_shot_bm_t: public hunter_ranged_attack_t
{
  struct impact_t final : public hunter_ranged_attack_t
  {
    impact_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
      hunter_ranged_attack_t( n, p, s )
    {
      dual = true;
      // Beast Mastery focus gain
      parse_effect_data( p -> find_spell( 204304 ) -> effectN( 1 ) );
    }
  };

  std::array<impact_t*, 2> damage;
  unsigned current_damage_action = 0;

  chimaera_shot_bm_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "chimaera_shot", p, p -> talents.chimaera_shot )
  {
    parse_options( options_str );

    aoe = 2;
    radius = 5;

    damage[ 0 ] = p -> get_background_action<impact_t>( "chimaera_shot_frost", p -> find_spell( 171454 ) );
    damage[ 1 ] = p -> get_background_action<impact_t>( "chimaera_shot_nature", p -> find_spell( 171457 ) );
    for ( auto a : damage )
      add_child( a );

    school = SCHOOL_FROSTSTRIKE; // Just so the report shows a mixture of the two colors.
  }

  void schedule_travel( action_state_t* s ) override
  {
    damage[ current_damage_action ] -> set_target( s -> target );
    damage[ current_damage_action ] -> execute();
    current_damage_action = ( current_damage_action + 1 ) % damage.size();
    action_state_t::release( s );
  }

  // Don't bother, the results will be discarded anyway.
  result_e calculate_result( action_state_t* ) const override { return RESULT_NONE; }
  double calculate_direct_amount( action_state_t* ) const override { return 0.0; }

  double energize_cast_regen( const action_state_t* ) const override
  {
    const size_t targets_hit = std::min( target_list().size(), as<size_t>( aoe ) );
    return targets_hit * damage[ 0 ] -> composite_energize_amount( nullptr );
  }
};

// Barbed Shot ===============================================================

struct barbed_shot_t: public hunter_ranged_attack_t
{
  timespan_t bestial_wrath_r2_reduction;

  barbed_shot_t( hunter_t* p, util::string_view options_str ) :
    hunter_ranged_attack_t( "barbed_shot", p, p -> specs.barbed_shot )
  {
    parse_options(options_str);

    if ( p -> find_rank_spell( "Bestial Wrath", "Rank 2" ) -> ok() )
      bestial_wrath_r2_reduction = timespan_t::from_seconds( p -> specs.bestial_wrath -> effectN( 3 ).base_value() );

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

    // Bestial Wrath (Rank 2) cooldown reduction
    p() -> cooldowns.bestial_wrath -> adjust( -bestial_wrath_r2_reduction );

    if ( p() -> legendary.qapla_eredun_war_order.ok() )
      p() -> cooldowns.kill_command -> adjust( -p() -> legendary.qapla_eredun_war_order -> effectN( 1 ).time_value() );

    for ( auto pet : pets::active<pets::hunter_main_pet_base_t>( p() -> pets.main, p() -> pets.animal_companion ) )
    {
      if ( p() -> talents.stomp.ok() )
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

// Chimaera Shot ======================================================================

struct chimaera_shot_mm_t: public hunter_ranged_attack_t
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
        /* In-game Wild Spirits procs off the *first* Chimaera Shot damage spell that
         * hits. As we don't support distance targeting anyway just disable procs from
         * the "secondary" frost damage spell.
         */
        triggers_wild_spirits = false;
      }
    }
  };

  impact_t* nature;
  impact_t* frost;

  chimaera_shot_mm_t( hunter_t* p, util::string_view options_str ):
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

    p() -> trigger_calling_the_shots();
    p() -> trigger_lethal_shots();

    p() -> buffs.precise_shots -> up(); // benefit tracking
    p() -> buffs.precise_shots -> decrement();
  }

  void schedule_travel( action_state_t* s ) override
  {
    impact_t* damage = ( s -> chain_target == 0 ) ? nature : frost;
    damage -> set_target( s -> target );
    damage -> execute();
    action_state_t::release( s );
  }

  // Don't bother, the results will be discarded anyway.
  result_e calculate_result( action_state_t* ) const override { return RESULT_NONE; }
  double calculate_direct_amount( action_state_t* ) const override { return 0.0; }
};

// Master Marksman ====================================================================

struct master_marksman_t : public residual_action::residual_periodic_action_t<hunter_ranged_attack_t>
{
  master_marksman_t( hunter_t* p ):
    residual_periodic_action_t( "master_marksman", p, p -> find_spell( 269576 ) )
  { }

  void init() override
  {
    residual_periodic_action_t::init();

    snapshot_flags |= STATE_TGT_MUL_TA;
    update_flags   |= STATE_TGT_MUL_TA;
  }
};

// Bursting Shot ======================================================================

struct bursting_shot_t : public hunter_ranged_attack_t
{
  bursting_shot_t( hunter_t* p, util::string_view options_str ) :
    hunter_ranged_attack_t( "bursting_shot", p, p -> find_specialization_spell( "Bursting Shot" ) )
  {
    parse_options( options_str );
  }
};

// Aimed Shot base class ==============================================================

struct aimed_shot_base_t: public hunter_ranged_attack_t
{
  struct {
    /* This is required *only* for Double Tap
     * In-game the second Aimed Shot is performed with a slight delay, which means it can get
     * affected by stuff happening after the main AiS cast.
     * This is typically not a problem, but it has a tricky interaction with Trick Shots:
     *  - if you do DT -> AiS -> MS the second AiS from DT will be affected by Trick Shots
     *    from MS and will cleave
     *  - if you do MS -> DT -> AiS the second AiS from DT will *also* cleave even though Trick
     *    Shots is consumed by the main AiS
     * This flag is the way to handle that for Double Tap Aimed Shots: we save Tricks Shots
     * state to it during the main AiS cast and then check it along with Trick Shots on execute.
     * It should be always false for the "main" Aimed Shot.
     */
    bool up = false;
    int target_count = 0;
  } trick_shots;
  struct {
    double multiplier = 0;
    double high, low;
  } careful_aim;

  aimed_shot_base_t( util::string_view name, hunter_t* p ):
    hunter_ranged_attack_t( name, p, p -> specs.aimed_shot )
  {
    radius = 8;
    base_aoe_multiplier = p -> specs.trick_shots -> effectN( 4 ).percent() +
                          p -> conduits.deadly_chain.percent();

    trick_shots.target_count = 1 + static_cast<int>( p -> specs.trick_shots -> effectN( 1 ).base_value() );

    if ( p -> talents.careful_aim.ok() )
    {
      careful_aim.high = p -> talents.careful_aim -> effectN( 1 ).base_value();
      careful_aim.low = p -> talents.careful_aim -> effectN( 2 ).base_value();
      careful_aim.multiplier = p -> talents.careful_aim -> effectN( 3 ).percent();
    }
  }

  bool trick_shots_up() const
  {
    return trick_shots.up || p() -> buffs.trick_shots -> check();
  }

  int n_targets() const override
  {
    if ( trick_shots_up() )
      return trick_shots.target_count;
    return hunter_ranged_attack_t::n_targets();
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
};

// Aimed Shot =========================================================================

struct aimed_shot_t : public aimed_shot_base_t
{
  struct double_tap_t final : public aimed_shot_base_t
  {
    double_tap_t( util::string_view n, hunter_t* p ):
      aimed_shot_base_t( n, p )
    {
      dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
      triggers_wild_spirits = false;
    }

    timespan_t execute_time() const override
    {
      return rng().gauss( 125_ms, 25_ms );
    }

    void schedule_execute( action_state_t* s = nullptr ) override
    {
      trick_shots.up = p() -> buffs.trick_shots -> check();

      aimed_shot_base_t::schedule_execute( s );
    }

    void execute() override
    {
      aimed_shot_base_t::execute();

      // XXX: Wild Spirits from Double Tap AiS at "close" range
      triggers_wild_spirits = p() -> get_player_distance( *target ) <= 20;
    }
  };

  struct serpent_sting_sst_t final : public hunter_ranged_attack_t
  {
    serpent_sting_sst_t( util::string_view n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> find_spell( 271788 ) )
    {
      dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
      triggers_wild_spirits = false;
    }
  };

  bool lock_and_loaded = false;
  struct {
    double_tap_t* action = nullptr;
    proc_t* proc;
  } double_tap;
  struct {
    serpent_sting_sst_t* action = nullptr;
  } serpentstalkers_trickery;
  struct {
    double chance = 0;
    proc_t* proc;
  } surging_shots;

  aimed_shot_t( hunter_t* p, util::string_view options_str ) :
    aimed_shot_base_t( "aimed_shot", p )
  {
    parse_options( options_str );

    if ( p -> talents.double_tap.ok() )
    {
      double_tap.action = p -> get_background_action<double_tap_t>( "aimed_shot_double_tap" );
      add_child( double_tap.action );
      double_tap.proc = p -> get_proc( "double_tap_aimed" );
    }

    if ( p -> legendary.serpentstalkers_trickery.ok() )
      serpentstalkers_trickery.action = p -> get_background_action<serpent_sting_sst_t>( "serpent_sting" );

    if ( p -> legendary.surging_shots.ok() )
    {
      surging_shots.chance = p -> legendary.surging_shots -> proc_chance();
      surging_shots.proc = p -> get_proc( "Surging Shots Rapid Fire reset" );
    }
  }

  double cost() const override
  {
    const bool casting = p() -> executing && p() -> executing == this;
    if ( casting ? lock_and_loaded : p() -> buffs.lock_and_load -> check() )
      return 0;

    double c = aimed_shot_base_t::cost();

    c *= 1 + p() -> buffs.secrets_of_the_vigil -> check_value();

    return c;
  }

  void schedule_execute( action_state_t* s ) override
  {
    lock_and_loaded = p() -> buffs.lock_and_load -> up();

    aimed_shot_base_t::schedule_execute( s );
  }

  void execute() override
  {
    aimed_shot_base_t::execute();

    p() -> buffs.precise_shots -> trigger( 1 + rng().range( p() -> buffs.precise_shots -> max_stack() ) );

    if ( double_tap.action && p() -> buffs.double_tap -> check() )
    {
      double_tap.action -> set_target( target );
      double_tap.action -> schedule_execute();
      p() -> buffs.double_tap -> decrement();
      double_tap.proc -> occur();
    }

    if ( serpentstalkers_trickery.action )
    {
      serpentstalkers_trickery.action -> set_target( target );
      serpentstalkers_trickery.action -> execute();
    }

    p() -> buffs.trick_shots -> up(); // benefit tracking
    p() -> consume_trick_shots();

    p() -> buffs.secrets_of_the_vigil -> up(); // benefit tracking
    // XXX: 2020-12-02 Be on the safe side and assume the buff doesn't get consumed
    // only if the AiS *benefits* from LnL. It may work as Streamline though.
    if ( ! lock_and_loaded )
      p() -> buffs.secrets_of_the_vigil -> decrement();

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
  }

  timespan_t execute_time() const override
  {
    if ( p() -> buffs.lock_and_load -> check() )
      return 0_ms;

    auto et = aimed_shot_base_t::execute_time();

    if ( p() -> buffs.streamline -> check() )
      et *= 1 + p() -> buffs.streamline -> check_value();

    if ( p() -> buffs.trueshot -> check() )
      et *= 1 + p() -> buffs.trueshot -> check_value();

    return et;
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double m = aimed_shot_base_t::recharge_multiplier( cd );

    // XXX [8.1]: Spell Data indicates that it's reducing Aimed Shot recharge rate by 225% (12s/3.25 = 3.69s)
    // m /= 1 + .6;  // The information from the bluepost
    // m /= 1 + 2.25; // The bugged (in spelldata) value for Aimed Shot.
    if ( p() -> buffs.trueshot -> check() )
      m /= 1 + p() -> specs.trueshot -> effectN( 3 ).percent();

    // XXX: [9.0 Beta]: has the same effect as Trueshot above, implement as such
    if ( p() -> buffs.dead_eye -> up() )
      m /= 1 + p() -> buffs.dead_eye -> check_value();

    return m;
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
    hunter_ranged_attack_t( "steady_shot", p, p -> find_class_spell( "Steady Shot" ) )
  {
    parse_options( options_str );

    breaks_steady_focus = false;

    spell_data_ptr_t rank2 = p -> find_rank_spell( "Steady Shot", "Rank 2" );
    if ( rank2.ok() )
    {
      energize_type = action_energize::ON_CAST;
      energize_resource = RESOURCE_FOCUS;
      energize_amount = rank2 -> effectN( 1 ).base_value();
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
  // this is required because Double Tap 'snapshots' on channel start
  struct state_t : public action_state_t
  {
    bool double_tapped = false;

    using action_state_t::action_state_t;

    void initialize() override
    {
      action_state_t::initialize();
      double_tapped = false;
    }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    {
      action_state_t::debug_str( s );
      fmt::print( s, " double_tapped={:d}", double_tapped );
      return s;
    }

    void copy_state( const action_state_t* o ) override
    {
      action_state_t::copy_state( o );
      double_tapped = debug_cast<const state_t*>( o ) -> double_tapped;
    }
  };

  struct damage_t final : public hunter_ranged_attack_t
  {
    const int trick_shots_targets;

    damage_t( util::string_view n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> find_spell( 257045 ) ),
      trick_shots_targets( as<int>( p -> specs.trick_shots -> effectN( 3 ).base_value() ) )
    {
      dual = true;
      direct_tick = true;
      radius = 8;
      base_aoe_multiplier = p -> specs.trick_shots -> effectN( 5 ).percent() +
                            p -> conduits.deadly_chain.percent();

      if ( p -> find_rank_spell( "Rapid Fire", "Rank 2" ) -> ok() )
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
       * fact that focus is integral and 1 * 1.5 = 1. It's still affected be the
       * generic focus gen increase from Trueshot as each energize gives 3 focus when
       * combined with Nesingwary's Trapping Apparatus buff.
       */
      if ( p() -> buffs.trueshot -> check() && rng().roll( .5 ) )
        p() -> resource_gain( RESOURCE_FOCUS, composite_energize_amount( execute_state ), p() -> gains.trueshot, this );

      // TODO
      // As of 2021-03-09 this seems to miss 1 trigger under Double Tap stacking only to 12. Need more testing etc.
      trigger_brutal_projectiles();
    }

    void impact( action_state_t* s ) override
    {
      hunter_ranged_attack_t::impact( s );

      // XXX: Wild Spirits kludge
      if ( s -> chain_target == 0 )
        triggers_wild_spirits = false;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = hunter_ranged_attack_t::composite_da_multiplier( s );

      m *= 1 + p() -> buffs.brutal_projectiles_hidden -> check_stack_value();

      return m;
    }

    void trigger_brutal_projectiles() const
    {
      if ( p() -> buffs.brutal_projectiles -> check() )
      {
        p() -> buffs.brutal_projectiles_hidden -> trigger();
        p() -> buffs.brutal_projectiles -> expire();
      }
      else if ( p() -> buffs.brutal_projectiles_hidden -> check() )
      {
        p() -> buffs.brutal_projectiles_hidden -> trigger();
      }
    }
  };

  damage_t* damage;
  int base_num_ticks;
  struct {
    proc_t* double_tap;
  } procs;

  rapid_fire_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "rapid_fire", p, p -> specs.rapid_fire ),
    damage( p -> get_background_action<damage_t>( "rapid_fire_damage" ) ),
    base_num_ticks( as<int>( data().effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );

    may_miss = may_crit = false;
    channeled = reset_auto_attack = true;
    triggers_wild_spirits = false;

    procs.double_tap = p -> get_proc( "double_tap_rapid_fire" );
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
    // XXX: Wild Spirits kludge
    damage -> triggers_wild_spirits = true;

    hunter_spell_t::execute();

    p() -> buffs.streamline -> trigger();
  }

  void tick( dot_t* d ) override
  {
    hunter_spell_t::tick( d );

    damage -> parent_dot = d; // BfA Surging Shots shenanigans
    damage -> set_target( d -> target );
    damage -> execute();
  }

  void last_tick( dot_t* d ) override
  {
    hunter_spell_t::last_tick( d );

    p() -> consume_trick_shots();

    if ( p() -> buffs.double_tap -> check() )
      procs.double_tap -> occur();
    p() -> buffs.double_tap -> decrement();

    p() -> buffs.brutal_projectiles_hidden -> expire();
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    timespan_t t = hunter_spell_t::tick_time( s );

    if ( debug_cast<const state_t*>( s ) -> double_tapped )
      t *= 1 + p() -> talents.double_tap -> effectN( 1 ).percent();

    return t;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    // substract 1 here because RF has a tick at zero
    return ( num_ticks( s ) - 1 ) * tick_time( s );
  }

  double energize_cast_regen( const action_state_t* s ) const override
  {
    // XXX: Not exactly true for Nesingwary's / Trueshot because the buff can fall off mid-channel. Meh
    return num_ticks( s ) * damage -> composite_energize_amount( nullptr );
  }

  int num_ticks( const action_state_t* s ) const
  {
    int num_ticks_ = base_num_ticks;
    if ( debug_cast<const state_t*>( s ) -> double_tapped )
      num_ticks_ = as<int>( num_ticks_ * ( 1 + p() -> talents.double_tap -> effectN( 3 ).percent() ) );
    return num_ticks_;
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double m = hunter_spell_t::recharge_multiplier( cd );

    // XXX [8.1]: Spell Data indicates that it's reducing Rapid Fire by 240% (20s/3.4 = 5.88s)
    // m /= 1 + .6;  // The information from the bluepost
    // m /= 1 + 2.4; // The bugged (in spelldata) value for Rapid Fire.
    if ( p() -> buffs.trueshot -> check() )
      m /= 1 + p() -> specs.trueshot -> effectN( 1 ).percent();

    return m;
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  void snapshot_state( action_state_t* s, result_amount_type type ) override
  {
    hunter_spell_t::snapshot_state( s, type );
    debug_cast<state_t*>( s ) -> double_tapped = p() -> buffs.double_tap -> check();
  }
};

// Explosive Shot  ====================================================================

struct explosive_shot_t: public hunter_ranged_attack_t
{
  struct damage_t final : hunter_ranged_attack_t
  {
    damage_t( util::string_view n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> find_spell( 212680 ) )
    {
      background = true;
      dual = true;
    }
  };

  explosive_shot_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "explosive_shot", p, p -> talents.explosive_shot )
  {
    parse_options( options_str );

    may_miss = may_crit = false;
    triggers_wild_spirits = false;

    tick_action = p -> get_background_action<damage_t>( "explosive_shot_aoe" );
  }
};

// Serpent Sting (Marksmanship) ==============================================

struct serpent_sting_mm_t: public hunter_ranged_attack_t
{
  serpent_sting_mm_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "serpent_sting", p, p -> talents.serpent_sting )
  {
    parse_options( options_str );
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

    // technically there is a separate effect for auto attacks, but meh
    affected_by.coordinated_assault.direct = true;
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
  struct latent_poison_injection_t final : hunter_spell_t
  {
    latent_poison_injection_t( util::string_view n, hunter_t* p ):
      hunter_spell_t( n, p, p -> find_spell( 336904 ) )
    {
      triggers_wild_spirits = false;
    }

    void trigger( player_t* target )
    {
      auto debuff = td( target ) -> debuffs.latent_poison_injection;
      if ( ! debuff -> check() )
        return;

      set_target( target );
      execute();

      debuff -> expire();
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = hunter_spell_t::composite_da_multiplier( s );

      m *= td( target ) -> debuffs.latent_poison_injection -> check();

      return m;
    }
  };

  internal_bleeding_t internal_bleeding;
  latent_poison_injection_t* latent_poison_injection = nullptr;
  struct {
    double chance = 0;
    proc_t* proc;
  } rylakstalkers_strikes;

  melee_focus_spender_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
    hunter_melee_attack_t( n, p, s ),
    internal_bleeding( p )
  {
    if ( p -> legendary.latent_poison_injectors.ok() )
      latent_poison_injection = p -> get_background_action<latent_poison_injection_t>( "latent_poison_injection" );

    if ( p -> legendary.rylakstalkers_strikes.ok() )
    {
      rylakstalkers_strikes.chance = p -> legendary.rylakstalkers_strikes -> proc_chance();
      rylakstalkers_strikes.proc = p -> get_proc( "Rylakstalker's Confounding Strikes" );
    }
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    p() -> buffs.vipers_venom -> trigger();
    p() -> buffs.butchers_bone_fragments -> trigger();

    p() -> trigger_birds_of_prey( target );

    if ( rng().roll( rylakstalkers_strikes.chance ) )
    {
      p() -> cooldowns.wildfire_bomb -> reset( true );
      rylakstalkers_strikes.proc -> occur();
    }

    p() -> buffs.tip_of_the_spear -> expire();
  }

  void impact( action_state_t* s ) override
  {
    hunter_melee_attack_t::impact( s );

    internal_bleeding.trigger( s );

    if ( latent_poison_injection )
      latent_poison_injection -> trigger( s -> target );
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

    background = ! p -> talents.mongoose_bite.ok();
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
    triggers_wild_spirits = false; // the trigger is on the player damage spell

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
  timespan_t wildfire_bomb_reduction;
  internal_bleeding_t internal_bleeding;

  carve_base_t( util::string_view n, hunter_t* p, const spell_data_t* s,
                timespan_t wfb_reduction ):
    hunter_melee_attack_t( n, p, s ), internal_bleeding( p )
  {
    if ( p -> find_rank_spell( "Carve", "Rank 2" ) -> ok() )
      wildfire_bomb_reduction = wfb_reduction;
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    const auto reduction = wildfire_bomb_reduction * num_targets_hit;
    p() -> cooldowns.wildfire_bomb -> adjust( -reduction, true );

    p() -> buffs.butchers_bone_fragments -> up(); // benefit tracking
    p() -> buffs.butchers_bone_fragments -> expire();
  }

  void impact( action_state_t* s ) override
  {
    hunter_melee_attack_t::impact( s );

    p() -> trigger_birds_of_prey( s -> target );
    p() -> buffs.flame_infusion -> trigger();
    internal_bleeding.trigger( s );
  }

  double action_multiplier() const override
  {
    double am = hunter_melee_attack_t::action_multiplier();

    if ( p() -> buffs.butchers_bone_fragments -> check() )
      am *= 1 + p() -> buffs.butchers_bone_fragments -> check_stack_value();

    return am;
  }
};

// Carve =============================================================================

struct carve_t: public carve_base_t
{
  carve_t( hunter_t* p, util::string_view options_str ):
    carve_base_t( "carve", p, p -> specs.carve ,
                  p -> specs.carve -> effectN( 2 ).time_value() )
  {
    parse_options( options_str );

    if ( p -> talents.butchery.ok() )
      background = true;
  }
};

// Butchery ==========================================================================

struct butchery_t: public carve_base_t
{
  butchery_t( hunter_t* p, util::string_view options_str ):
    carve_base_t( "butchery", p, p -> talents.butchery,
                  p -> talents.butchery -> effectN( 2 ).time_value() )
  {
    parse_options( options_str );
  }
};

// Raptor Strike =====================================================================

struct raptor_strike_base_t: public melee_focus_spender_t
{
  raptor_strike_base_t( util::string_view n, hunter_t* p, spell_data_ptr_t s ):
    melee_focus_spender_t( n, p, s )
  {
    background = p -> talents.mongoose_bite.ok();
  }
};

struct raptor_strike_t: public raptor_strike_base_t
{
  raptor_strike_t( hunter_t* p, util::string_view options_str ):
    raptor_strike_base_t( "raptor_strike", p, p -> specs.raptor_strike )
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
    hunter_melee_attack_t( "harpoon", p, p -> specs.harpoon )
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
  serpent_sting_sv_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "serpent_sting", p, p -> find_specialization_spell( "Serpent Sting" ) )
  {
    parse_options( options_str );

    if ( p -> talents.hydras_bite.ok() )
      aoe = 1 + static_cast<int>( p -> talents.hydras_bite -> effectN( 1 ).base_value() );
  }

  void init() override
  {
    hunter_ranged_attack_t::init();

    if ( action_t* lpi = p() -> find_action( "latent_poison_injection" ) )
      add_child( lpi );

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

    m *= 1 + p() -> buffs.vipers_venom -> check_value();

    return m;
  }

  void assess_damage( result_amount_type type, action_state_t* s ) override
  {
    hunter_ranged_attack_t::assess_damage( type, s );

    if ( s -> result_amount > 0 && p() -> legendary.latent_poison_injectors.ok() )
      td( s -> target ) -> debuffs.latent_poison_injection -> trigger();
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

  struct damage_t final : public hunter_ranged_attack_t
  {
    damage_t( util::string_view n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> talents.chakrams -> effectN( 1 ).trigger() )
    {
      dual = true;

      // Chakrams hits all targets in it's path, as we don't have support for this
      // just make it hit everything.
      aoe = -1;
      radius = 0;

      base_multiplier *= 2;
      base_aoe_multiplier = 0.5;
    }
  };
  damage_t* damage = nullptr;

  chakrams_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "chakrams", p, p -> talents.chakrams )
  {
    parse_options( options_str );

    damage = p -> get_background_action<damage_t>( "chakrams_damage" );
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
  interrupt_base_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
    hunter_spell_t( n, p, s )
  {
    may_miss = may_block = may_dodge = may_parry = false;
    is_interrupt = true;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    if ( p() -> conduits.reversal_of_fortune.ok() )
      p() -> resource_gain( RESOURCE_FOCUS, p() -> conduits.reversal_of_fortune.value(), p() -> gains.reversal_of_fortune, this );
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

      // XXX: Wild Spirits kludge
      if ( p() -> buffs.wild_spirits -> check() )
        triggers_wild_spirits = false;
    }
  };

  a_murder_of_crows_t( hunter_t* p, util::string_view options_str ) :
    hunter_spell_t( "a_murder_of_crows", p, p -> talents.a_murder_of_crows )
  {
    parse_options( options_str );

    triggers_wild_spirits = false;

    tick_action = p -> get_background_action<peck_t>( "crow_peck" );
    starved_proc = p -> get_proc( "starved: a_murder_of_crows" );
  }

  void execute() override
  {
    // XXX: Wild Spirits kludge
    static_cast<peck_t*>( tick_action ) -> triggers_wild_spirits = true;

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

  void impact( action_state_t* s ) override
  {
    hunter_spell_t::impact( s );

    if ( p() -> legendary.nesingwarys_apparatus.ok() )
    {
      double amount = p() -> buffs.nesingwarys_apparatus -> data().effectN( 1 ).resource( RESOURCE_FOCUS );
      p() -> resource_gain( RESOURCE_FOCUS, amount, p() -> gains.nesingwarys_apparatus_direct, this );
      p() -> buffs.nesingwarys_apparatus -> trigger();
    }
  }

  timespan_t travel_time() const override
  {
    timespan_t time_to_travel = hunter_spell_t::travel_time();
    if ( is_precombat )
      return std::max( 0_ms, time_to_travel - precast_time );
    return time_to_travel;
  }

  double energize_cast_regen( const action_state_t* ) const override
  {
    if ( p() -> legendary.nesingwarys_apparatus.ok() )
      return p() -> buffs.nesingwarys_apparatus -> data().effectN( 1 ).resource( RESOURCE_FOCUS );
    return 0;
  }
};

// Tar Trap =====================================================================

struct tar_trap_t : public trap_base_t
{
  timespan_t debuff_duration;

  tar_trap_t( hunter_t* p, util::string_view options_str ) :
    trap_base_t( "tar_trap", p, p -> find_class_spell( "Tar Trap" ) )
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
    trap_base_t( "freezing_trap", p, p -> find_class_spell( "Freezing Trap" ) )
  {
    parse_options( options_str );
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

// Flare ====================================================================

struct flare_t : hunter_spell_t
{
  struct soulforge_embers_t final : hunter_spell_t
  {
    soulforge_embers_t( util::string_view n, hunter_t* p )
      : hunter_spell_t( n, p, p -> find_spell( 336746 ) )
    {
      aoe = as<int>( p -> legendary.soulforge_embers -> effectN( 1 ).base_value() );
      radius = p -> find_class_spell( "Tar Trap" ) -> effectN( 1 ).trigger() -> effectN( 1 ).base_value();
      triggers_wild_spirits = false;
    }
  };

  soulforge_embers_t* soulforge_embers = nullptr;

  flare_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "flare", p, p -> find_class_spell( "Flare" ) )
  {
    parse_options( options_str );

    harmful = false;

    if ( p -> legendary.soulforge_embers.ok() )
      soulforge_embers = p -> get_background_action<soulforge_embers_t>( "soulforge_embers" );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    if ( soulforge_embers && p() -> state.tar_trap_aoe )
    {
      make_event<ground_aoe_event_t>( *sim, player, ground_aoe_params_t()
        .target( execute_state -> target )
        .x( p() -> state.tar_trap_aoe -> x_position )
        .y( p() -> state.tar_trap_aoe -> y_position )
        .pulse_time( 1_ms )
        .n_pulses( 1 )
        .action( soulforge_embers )
      );
    }
  }
};

// Kill Command =============================================================

struct kill_command_t: public hunter_spell_t
{
  struct {
    double chance = 0;
    proc_t* proc = nullptr;
  } flankers_advantage;

  kill_command_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "kill_command", p, p -> specs.kill_command )
  {
    parse_options( options_str );

    if ( p -> find_rank_spell( "Kill Command", "Rank 2" ) -> ok() )
    {
      flankers_advantage.chance = data().effectN( 2 ).percent();
      flankers_advantage.proc = p -> get_proc( "flankers_advantage" );
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

    if ( flankers_advantage.chance != 0 )
    {
      double chance = flankers_advantage.chance;
      if ( p() -> buffs.coordinated_assault -> check() )
        chance += p() -> specs.coordinated_assault -> effectN( 4 ).percent();
      if ( td( target ) -> dots.pheromone_bomb -> is_ticking() )
        chance += p() -> find_spell( 270323 ) -> effectN( 2 ).percent();
      if ( rng().roll( chance ) )
      {
        flankers_advantage.proc -> occur();
        cooldown -> reset( true );
        p() -> buffs.strength_of_the_pack -> trigger();
      }
    }

    p() -> buffs.flamewakers_cobra_sting -> up(); // benefit tracking
    p() -> buffs.flamewakers_cobra_sting -> decrement();
  }

  double cost() const override
  {
    double c = hunter_spell_t::cost();

    if ( p() -> buffs.flamewakers_cobra_sting -> check() )
      c *= 1 + p() -> buffs.flamewakers_cobra_sting -> check_value();

    return c;
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

// Bestial Wrath ============================================================

struct bestial_wrath_t: public hunter_spell_t
{
  timespan_t precast_time = 0_ms;

  bestial_wrath_t( hunter_t* player, util::string_view options_str ):
    hunter_spell_t( "bestial_wrath", player, player -> specs.bestial_wrath )
  {
    add_option( opt_timespan( "precast_time", precast_time ) );
    parse_options( options_str );

    harmful = false;

    precast_time = clamp( precast_time, 0_ms, data().duration() );
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
      {
        pet -> active.bestial_wrath -> set_target( target );
        pet -> active.bestial_wrath -> execute();
      }
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
  timespan_t precast_time = 0_ms;

  aspect_of_the_wild_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "aspect_of_the_wild", p, p -> specs.aspect_of_the_wild )
  {
    add_option( opt_timespan( "precast_time", precast_time ) );
    parse_options( options_str );

    harmful = false;
    dot_duration = 0_ms;

    precast_time = clamp( precast_time, 0_ms, data().duration() );

    p -> actions.aspect_of_the_wild = this;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    trigger_buff( p() -> buffs.aspect_of_the_wild, precast_time );

    adjust_precast_cooldown( precast_time );
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

      // XXX: Wild Spirits kludge
      if ( s -> chain_target == 0 && p() -> buffs.wild_spirits -> check() )
        triggers_wild_spirits = false;
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
    triggers_wild_spirits = false;

    tick_action = new damage_t( p );
  }

  void execute() override
  {
    // XXX: Wild Spirits kludge
    static_cast<damage_t*>( tick_action ) -> triggers_wild_spirits = true;

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
    {
      pet -> active.bloodshed -> set_target( target );
      pet -> active.bloodshed -> execute();
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

//==============================
// Marksmanship spells
//==============================

// Trueshot =================================================================

struct trueshot_t: public hunter_spell_t
{
  timespan_t precast_time = 0_ms;

  trueshot_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "trueshot", p, p -> specs.trueshot )
  {
    add_option( opt_timespan( "precast_time", precast_time ) );
    parse_options( options_str );

    harmful = false;

    precast_time = clamp( precast_time, 0_ms, data().duration() );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    trigger_buff( p() -> buffs.trueshot, precast_time );

    adjust_precast_cooldown( precast_time );
  }
};

// Double Tap ===================================================================

struct double_tap_t: public hunter_spell_t
{
  timespan_t precast_time = 0_ms;

  double_tap_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "double_tap", p, p -> talents.double_tap )
  {
    add_option( opt_timespan( "precast_time", precast_time ) );
    parse_options( options_str );

    harmful = false;
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

// Volley ===========================================================================

struct volley_t : hunter_spell_t
{
  struct damage_t final : hunter_ranged_attack_t
  {
    damage_t( util::string_view n, hunter_t* p )
      : hunter_ranged_attack_t( n, p, p -> find_spell( 260247 ) )
    {
      aoe = -1;
      background = dual = ground_aoe = true;
    }

    void impact( action_state_t* s ) override
    {
      hunter_ranged_attack_t::impact( s );

      // XXX: Wild Spirits kludge
      if ( s -> chain_target == 0 && p() -> buffs.wild_spirits -> check() )
        triggers_wild_spirits = false;
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

    // XXX: Wild Spirits kludge
    damage -> triggers_wild_spirits = true;

    make_event<ground_aoe_event_t>( *sim, player, ground_aoe_params_t()
        .target( execute_state -> target )
        .duration( data().duration() )
        .pulse_time( data().effectN( 2 ).period() )
        .action( damage )
      );
  }
};

//==============================
// Survival spells
//==============================

// Coordinated Assault ==============================================================

struct coordinated_assault_t: public hunter_spell_t
{
  coordinated_assault_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "coordinated_assault", p, p -> specs.coordinated_assault )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.coordinated_assault -> trigger();
  }
};

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
  struct wildfire_cluster_t final : hunter_spell_t
  {
    wildfire_cluster_t( util::string_view n, hunter_t* p ):
      hunter_spell_t( n, p, p -> find_spell( 336899 ) )
    {
      aoe = -1;
      triggers_wild_spirits = false;
    }
  };

  struct bomb_base_t : public hunter_spell_t
  {
    struct dot_action_t final : public hunter_spell_t
    {
      dot_action_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
        hunter_spell_t( n, p, s )
      {
        dual = true;
        triggers_wild_spirits = false;
      }

      // does not pandemic
      timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const override
      {
        return dot -> time_to_next_tick() + triggered_duration;
      }
    };
    dot_action_t* dot_action;

    bomb_base_t( util::string_view n, wildfire_bomb_t* a, const spell_data_t* s, util::string_view dot_n, const spell_data_t* dot_s ):
      hunter_spell_t( n, a -> p(), s ),
      dot_action( a -> p() -> get_background_action<dot_action_t>( dot_n, dot_s ) )
    {
      dual = true;

      aoe = -1;
      radius = 5; // XXX: It's actually a circle + cone, but we sadly can't really model that

      dot_action -> aoe = aoe;
      dot_action -> radius = radius;

      a -> add_child( this );
      a -> add_child( dot_action );
    }

    void execute() override
    {
      hunter_spell_t::execute();

      if ( num_targets_hit > 0 )
      {
        // Dot applies to all of the same targets hit by the main explosion
        dot_action -> target = target;
        dot_action -> target_cache.list = target_cache.list;
        dot_action -> target_cache.is_valid = true;
        dot_action -> execute();
      }

      p() -> buffs.flame_infusion -> up(); // benefit tracking
      p() -> buffs.flame_infusion -> expire();
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double am = hunter_spell_t::composite_da_multiplier( s );

      am *= 1 + p() -> buffs.flame_infusion -> check_stack_value();

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
    struct violent_reaction_t final : public hunter_spell_t
    {
      violent_reaction_t( util::string_view n, hunter_t* p ):
        hunter_spell_t( n, p, p -> find_spell( 260231 ) )
      {
        dual = true;
        aoe = -1;
        triggers_wild_spirits = false;
      }
    };
    violent_reaction_t* violent_reaction;

    volatile_bomb_t( util::string_view n, hunter_t* p, wildfire_bomb_t* a ):
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
  hunter_spell_t* wildfire_cluster = nullptr;

  wildfire_bomb_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "wildfire_bomb", p, p -> specs.wildfire_bomb )
  {
    parse_options( options_str );

    may_miss = false;
    school = SCHOOL_FIRE; // for report coloring
    triggers_wild_spirits = false;

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

    if ( p -> legendary.wildfire_cluster.ok() )
    {
      wildfire_cluster = p -> get_background_action<wildfire_cluster_t>( "wildfire_cluster" );
      add_child( wildfire_cluster );
    }
  }

  void execute() override
  {
    current_bomb = bombs[ p() -> state.next_wi_bomb ];

    hunter_spell_t::execute();

    if ( p() -> talents.wildfire_infusion.ok() )
    {
      // assume that we can't get 2 same bombs in a row
      int slot = rng().range( 2 );
      if ( slot == p() -> state.next_wi_bomb )
        slot += 2 - p() -> state.next_wi_bomb;
      p() -> state.next_wi_bomb = static_cast<wildfire_infusion_e>( slot );
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
  aspect_of_the_eagle_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "aspect_of_the_eagle", p, p -> specs.aspect_of_the_eagle )
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
  debuffs.latent_poison_injection =
    make_buff( *this, "latent_poison_injection", p -> legendary.latent_poison_injectors -> effectN( 1 ).trigger() )
      -> set_trigger_spell( p -> legendary.latent_poison_injectors );

  dots.serpent_sting = target -> get_dot( "serpent_sting", p );
  dots.a_murder_of_crows = target -> get_dot( "a_murder_of_crows", p );
  dots.pheromone_bomb = target -> get_dot( "pheromone_bomb", p );
  dots.shrapnel_bomb = target -> get_dot( "shrapnel_bomb", p );

  target -> register_on_demise_callback( p, std::bind( &hunter_td_t::target_demise, this ) );
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
      [ &action, high_pct = talents.careful_aim->effectN( 1 ).base_value() ] {
        return action.target->health_percentage() > high_pct;
      } );
  }

  return player_t::create_action_expression( action, expression_str );
}

std::unique_ptr<expr_t> hunter_t::create_expression( util::string_view expression_str )
{
  auto splits = util::string_split<util::string_view>( expression_str, "." );

  if ( splits.size() == 2 && splits[ 0 ] == "next_wi_bomb" )
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
                                   const std::string& options_str )
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
  if ( name == "carve"                 ) return new                  carve_t( this, options_str );
  if ( name == "chakrams"              ) return new               chakrams_t( this, options_str );
  if ( name == "cobra_shot"            ) return new             cobra_shot_t( this, options_str );
  if ( name == "coordinated_assault"   ) return new    coordinated_assault_t( this, options_str );
  if ( name == "counter_shot"          ) return new           counter_shot_t( this, options_str );
  if ( name == "death_chakram"         ) return new          death_chakram_t( this, options_str );
  if ( name == "dire_beast"            ) return new             dire_beast_t( this, options_str );
  if ( name == "double_tap"            ) return new             double_tap_t( this, options_str );
  if ( name == "explosive_shot"        ) return new         explosive_shot_t( this, options_str );
  if ( name == "flanking_strike"       ) return new        flanking_strike_t( this, options_str );
  if ( name == "flare"                 ) return new                  flare_t( this, options_str );
  if ( name == "flayed_shot"           ) return new            flayed_shot_t( this, options_str );
  if ( name == "freezing_trap"         ) return new          freezing_trap_t( this, options_str );
  if ( name == "harpoon"               ) return new                harpoon_t( this, options_str );
  if ( name == "kill_command"          ) return new           kill_command_t( this, options_str );
  if ( name == "kill_shot"             ) return new              kill_shot_t( this, options_str );
  if ( name == "mongoose_bite"         ) return new          mongoose_bite_t( this, options_str );
  if ( name == "mongoose_bite_eagle"   ) return new    mongoose_bite_eagle_t( this, options_str );
  if ( name == "multi_shot"            ) return new             multi_shot_t( this, options_str );
  if ( name == "multishot"             ) return new             multi_shot_t( this, options_str );
  if ( name == "muzzle"                ) return new                 muzzle_t( this, options_str );
  if ( name == "rapid_fire"            ) return new             rapid_fire_t( this, options_str );
  if ( name == "raptor_strike"         ) return new          raptor_strike_t( this, options_str );
  if ( name == "raptor_strike_eagle"   ) return new    raptor_strike_eagle_t( this, options_str );
  if ( name == "resonating_arrow"      ) return new       resonating_arrow_t( this, options_str );
  if ( name == "stampede"              ) return new               stampede_t( this, options_str );
  if ( name == "steady_shot"           ) return new            steady_shot_t( this, options_str );
  if ( name == "steel_trap"            ) return new             steel_trap_t( this, options_str );
  if ( name == "summon_pet"            ) return new             summon_pet_t( this, options_str );
  if ( name == "tar_trap"              ) return new               tar_trap_t( this, options_str );
  if ( name == "trueshot"              ) return new               trueshot_t( this, options_str );
  if ( name == "volley"                ) return new                 volley_t( this, options_str );
  if ( name == "wild_spirits"          ) return new           wild_spirits_t( this, options_str );
  if ( name == "wildfire_bomb"         ) return new          wildfire_bomb_t( this, options_str );

  if ( name == "chimaera_shot" )
  {
    if ( specialization() == HUNTER_MARKSMANSHIP )
      return new chimaera_shot_mm_t( this, options_str );
    if ( specialization() == HUNTER_BEAST_MASTERY )
      return new chimaera_shot_bm_t( this, options_str );
  }
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

  if ( talents.spitting_cobra.ok() )
    pets.spitting_cobra = new pets::spitting_cobra_t( this );

  if ( legendary.dire_command.ok() )
  {
    pets.dc_dire_beast.set_creation_callback(
      [] ( hunter_t* p ) { return new pets::dire_critter_t( p, "dire_beast_(dc)" ); });
  }
}

void hunter_t::init()
{
  player_t::init();
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

  // tier 25
  talents.scent_of_blood                    = find_talent_spell( "Scent of Blood" );
  talents.one_with_the_pack                 = find_talent_spell( "One with the Pack" );
  talents.chimaera_shot                     = find_talent_spell( "Chimaera Shot" );

  talents.careful_aim                       = find_talent_spell( "Careful Aim" );
  talents.barrage                           = find_talent_spell( "Barrage" );
  talents.explosive_shot                    = find_talent_spell( "Explosive Shot" );

  talents.guerrilla_tactics                 = find_talent_spell( "Guerrilla Tactics" );
  talents.hydras_bite                       = find_talent_spell( "Hydra's Bite" );
  talents.butchery                          = find_talent_spell( "Butchery" );

  // tier 30
  talents.trailblazer                       = find_talent_spell( "Trailblazer" );
  talents.natural_mending                   = find_talent_spell( "Natural Mending" );
  talents.camouflage                        = find_talent_spell( "Camouflage" );

  // tier 35
  talents.spitting_cobra                    = find_talent_spell( "Spitting Cobra" );
  talents.thrill_of_the_hunt                = find_talent_spell( "Thrill of the Hunt" );
  talents.a_murder_of_crows                 = find_talent_spell( "A Murder of Crows" );

  talents.steady_focus                      = find_talent_spell( "Steady Focus" );
  talents.streamline                        = find_talent_spell( "Streamline" );

  talents.bloodseeker                       = find_talent_spell( "Bloodseeker" );
  talents.steel_trap                        = find_talent_spell( "Steel Trap" );

  // tier 40
  talents.born_to_be_wild                   = find_talent_spell( "Born To Be Wild" );
  talents.posthaste                         = find_talent_spell( "Posthaste" );
  talents.binding_shot                      = find_talent_spell( "Binding Shot" );

  // tier 45
  talents.stomp                             = find_talent_spell( "Stomp" );
  talents.stampede                          = find_talent_spell( "Stampede" );

  talents.lethal_shots                      = find_talent_spell( "Lethal Shots" );
  talents.dead_eye                          = find_talent_spell( "Dead Eye" );
  talents.double_tap                        = find_talent_spell( "Double Tap" );

  talents.tip_of_the_spear                  = find_talent_spell( "Tip of the Spear" );
  talents.mongoose_bite                     = find_talent_spell( "Mongoose Bite" );
  talents.flanking_strike                   = find_talent_spell( "Flanking Strike" );

  // tier 50
  talents.aspect_of_the_beast               = find_talent_spell( "Aspect of the Beast" );
  talents.killer_cobra                      = find_talent_spell( "Killer Cobra" );
  talents.bloodshed                         = find_talent_spell( "Bloodshed" );

  talents.calling_the_shots                 = find_talent_spell( "Calling the Shots" );
  talents.lock_and_load                     = find_talent_spell( "Lock and Load" );
  talents.volley                            = find_talent_spell( "Volley" );

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

  specs.kill_command         = find_class_spell( "Kill Command" );
  specs.kill_shot            = find_class_spell( "Kill Shot" );

  // Beast Mastery
  specs.aspect_of_the_wild   = find_specialization_spell( "Aspect of the Wild" );
  specs.barbed_shot          = find_specialization_spell( "Barbed Shot" );
  specs.beast_cleave         = find_specialization_spell( "Beast Cleave" );
  specs.bestial_wrath        = find_specialization_spell( "Bestial Wrath" );
  specs.kindred_spirits      = find_specialization_spell( "Kindred Spirits" );
  specs.pack_tactics         = find_specialization_spell( "Pack Tactics" );
  specs.wild_call            = find_specialization_spell( "Wild Call" );

  // Marksmanship
  specs.aimed_shot           = find_specialization_spell( "Aimed Shot" );
  specs.lone_wolf            = find_specialization_spell( "Lone Wolf" );
  specs.precise_shots        = find_specialization_spell( "Precise Shots" );
  specs.rapid_fire           = find_specialization_spell( "Rapid Fire" );
  specs.trick_shots          = find_specialization_spell( "Trick Shots" );
  specs.trueshot             = find_specialization_spell( "Trueshot" );

  // Survival
  specs.aspect_of_the_eagle  = find_specialization_spell( "Aspect of the Eagle" );
  specs.carve                = find_specialization_spell( "Carve" );
  specs.coordinated_assault  = find_specialization_spell( "Coordinated Assault" );
  specs.harpoon              = find_specialization_spell( "Harpoon" );
  specs.raptor_strike        = find_specialization_spell( "Raptor Strike" );
  specs.wildfire_bomb        = find_specialization_spell( "Wildfire Bomb" );

  // Covenants

  covenants.death_chakram    = find_covenant_spell( "Death Chakram" );
  covenants.flayed_shot      = find_covenant_spell( "Flayed Shot" );
  covenants.resonating_arrow = find_covenant_spell( "Resonating Arrow" );
  covenants.wild_spirits     = find_covenant_spell( "Wild Spirits" );

  // Soulbind Conduits

  conduits.empowered_release    = find_conduit_spell( "Empowered Release" );
  conduits.enfeebled_mark       = find_conduit_spell( "Enfeebled Mark" );
  conduits.necrotic_barrage     = find_conduit_spell( "Necrotic Barrage" );
  conduits.spirit_attunement    = find_conduit_spell( "Spirit Attunement" );

  conduits.bloodletting         = find_conduit_spell( "Bloodletting" );
  conduits.echoing_call         = find_conduit_spell( "Echoing Call" );
  conduits.ferocious_appetite   = find_conduit_spell( "Ferocious Appetite" );
  conduits.one_with_the_beast   = find_conduit_spell( "One With the Beast" );

  conduits.brutal_projectiles   = find_conduit_spell( "Brutal Projectiles" );
  conduits.deadly_chain         = find_conduit_spell( "Deadly Chain" );
  conduits.powerful_precision   = find_conduit_spell( "Powerful Precision" );
  conduits.sharpshooters_focus  = find_conduit_spell( "Sharpshooter's Focus" );

  conduits.deadly_tandem        = find_conduit_spell( "Deadly Tandem" );
  conduits.flame_infusion       = find_conduit_spell( "Flame Infusion" );
  conduits.reversal_of_fortune  = find_conduit_spell( "Reversal of Fortune" );
  conduits.stinging_strike      = find_conduit_spell( "Stinging Strike" );
  conduits.strength_of_the_pack = find_conduit_spell( "Strength of the Pack" );

  // Runeforge Legendaries

  legendary.call_of_the_wild         = find_runeforge_legendary( "Call of the Wild" );
  legendary.craven_strategem         = find_runeforge_legendary( "Craven Strategem" );
  legendary.nesingwarys_apparatus    = find_runeforge_legendary( "Nessingwary's Trapping Apparatus" );
  legendary.soulforge_embers         = find_runeforge_legendary( "Soulforge Embers" );

  legendary.dire_command             = find_runeforge_legendary( "Dire Command" );
  legendary.flamewakers_cobra_sting  = find_runeforge_legendary( "Flamewaker's Cobra Sting" );
  legendary.qapla_eredun_war_order   = find_runeforge_legendary( "Qa'pla, Eredun War Order" );
  legendary.rylakstalkers_fangs      = find_runeforge_legendary( "Rylakstalker's Piercing Fangs" );

  legendary.eagletalons_true_focus   = find_runeforge_legendary( "Eagletalon's True Focus" );
  legendary.secrets_of_the_vigil     = find_runeforge_legendary( "Secrets of the Unblinking Vigil" );
  legendary.serpentstalkers_trickery = find_runeforge_legendary( "Serpentstalker's Trickery" );
  legendary.surging_shots            = find_runeforge_legendary( "Surging Shots" );

  legendary.butchers_bone_fragments  = find_runeforge_legendary( "Butcher's Bone Fragments" );
  legendary.latent_poison_injectors  = find_runeforge_legendary( "Latent Poison Injectors" );
  legendary.rylakstalkers_strikes    = find_runeforge_legendary( "Rylakstalker's Confounding Strikes" );
  legendary.wildfire_cluster         = find_runeforge_legendary( "Wildfire Cluster" );
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
  for ( auto spell : { specs.marksmanship_hunter, specs.survival_hunter, specs.pack_tactics } )
  {
    for ( const spelleffect_data_t& effect : spell -> effects() )
    {
      if ( effect.ok() && effect.type() == E_APPLY_AURA && effect.subtype() == A_MOD_POWER_REGEN_PERCENT )
        resources.base_regen_per_second[ RESOURCE_FOCUS ] *= 1 + effect.percent();
    }
  }

  resources.base[RESOURCE_FOCUS] = 100 + specs.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS );
}

void hunter_t::create_actions()
{
  player_t::create_actions();

  if ( talents.master_marksman.ok() )
    actions.master_marksman = new attacks::master_marksman_t( this );
}

void hunter_t::create_buffs()
{
  player_t::create_buffs();

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

  // Beast Mastery

  buffs.aspect_of_the_wild =
    make_buff( this, "aspect_of_the_wild", specs.aspect_of_the_wild )
      -> set_cooldown( 0_ms )
      -> set_activated( true )
      -> set_default_value_from_effect( 1 )
      -> set_tick_callback( [ this ]( buff_t *b, int, timespan_t ){
          resource_gain( RESOURCE_FOCUS, b -> data().effectN( 2 ).resource( RESOURCE_FOCUS ), gains.aspect_of_the_wild, actions.aspect_of_the_wild );
          if ( auto pet = pets.main )
            pet -> resource_gain( RESOURCE_FOCUS, b -> data().effectN( 5 ).resource( RESOURCE_FOCUS ), pet -> gains.aspect_of_the_wild );
        } );

  buffs.bestial_wrath =
    make_buff( this, "bestial_wrath", specs.bestial_wrath )
      -> set_cooldown( 0_ms )
      -> set_activated( true )
      -> set_default_value_from_effect( 1 )
      -> apply_affecting_conduit( conduits.one_with_the_beast );
  if ( talents.spitting_cobra.ok() )
  {
    timespan_t duration = find_spell( 194407 ) -> duration();
    buffs.bestial_wrath -> set_stack_change_callback(
      [this, duration]( buff_t*, int, int cur ) {
        if ( cur == 0 )
          pets.spitting_cobra -> summon( duration );
      } );
  }

  const spell_data_t* barbed_shot = find_spell( 246152 );
  for ( size_t i = 0; i < buffs.barbed_shot.size(); i++ )
  {
    buffs.barbed_shot[ i ] =
      make_buff( this, fmt::format( "barbed_shot_{}", i + 1 ), barbed_shot )
        -> set_default_value( barbed_shot -> effectN( 1 ).resource( RESOURCE_FOCUS ) )
        -> set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
            resource_gain( RESOURCE_FOCUS, b -> default_value, gains.barbed_shot, actions.barbed_shot );
          } );
  }

  buffs.dire_beast =
    make_buff( this, "dire_beast", find_spell( 120679 ) -> effectN( 2 ).trigger() )
      -> set_default_value_from_effect( 1 )
      -> set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buffs.thrill_of_the_hunt =
    make_buff( this, "thrill_of_the_hunt", talents.thrill_of_the_hunt -> effectN( 1 ).trigger() )
      -> set_default_value_from_effect( 1 )
      -> set_trigger_spell( talents.thrill_of_the_hunt );

  // Marksmanship

  buffs.dead_eye =
    make_buff( this, "dead_eye", talents.dead_eye -> effectN( 2 ).trigger() )
      -> set_default_value_from_effect( 1 )
      -> set_stack_change_callback( [this]( buff_t*, int, int ) {
          cooldowns.aimed_shot -> adjust_recharge_multiplier();
        } );

  buffs.double_tap =
    make_buff( this, "double_tap", talents.double_tap )
      -> set_default_value_from_effect( 1 )
      -> set_cooldown( 0_ms )
      -> set_activated( true );

  buffs.lock_and_load =
    make_buff( this, "lock_and_load", talents.lock_and_load -> effectN( 1 ).trigger() )
      -> set_trigger_spell( talents.lock_and_load );

  buffs.lone_wolf =
    make_buff( this, "lone_wolf", find_spell( 164273 ) )
      -> set_default_value( specs.lone_wolf -> effectN( 1 ).percent() )
      -> set_period( 0_ms ) // disable ticks as an optimization
      -> set_chance( specs.lone_wolf.ok() );

  buffs.precise_shots =
    make_buff( this, "precise_shots", find_spell( 260242 ) )
      -> set_default_value_from_effect( 1 )
      -> apply_affecting_conduit( conduits.powerful_precision );

  buffs.steady_focus =
    make_buff( this, "steady_focus", find_spell( 193534 ) )
      -> set_default_value_from_effect( 1 )
      -> set_pct_buff_type( STAT_PCT_BUFF_HASTE )
      -> set_trigger_spell( talents.steady_focus );

  buffs.streamline =
    make_buff( this, "streamline", find_spell( 342076 ) )
      -> set_default_value_from_effect( 1 )
      -> set_chance( talents.streamline.ok() );

  buffs.trick_shots =
    make_buff<buffs::trick_shots_t>( this, "trick_shots", find_spell( 257622 ) )
      -> set_chance( specs.trick_shots.ok() );

  buffs.trueshot =
    make_buff( this, "trueshot", specs.trueshot )
      -> set_cooldown( 0_ms )
      -> set_activated( true )
      -> set_default_value_from_effect( 4 )
      -> set_affects_regen( true )
      -> set_stack_change_callback( [this]( buff_t*, int old, int cur ) {
          cooldowns.aimed_shot -> adjust_recharge_multiplier();
          cooldowns.rapid_fire -> adjust_recharge_multiplier();
          if ( cur == 0 )
            buffs.eagletalons_true_focus -> expire();
          else if ( old == 0 )
            buffs.eagletalons_true_focus -> trigger();
        } )
      -> apply_affecting_aura( legendary.eagletalons_true_focus )
      -> apply_affecting_conduit( conduits.sharpshooters_focus );

  buffs.volley =
    make_buff( this, "volley", talents.volley )
      -> set_period( 0_ms ) // disable ticks as an optimization
      -> set_activated( true );

  // Survival

  buffs.coordinated_assault =
    make_buff( this, "coordinated_assault", specs.coordinated_assault )
      -> set_cooldown( 0_ms )
      -> set_activated( true )
      -> set_default_value_from_effect( 1 )
      -> apply_affecting_conduit( conduits.deadly_tandem );

  buffs.vipers_venom =
    make_buff( this, "vipers_venom", talents.vipers_venom -> effectN( 1 ).trigger() )
      -> set_default_value_from_effect( 1 )
      -> set_trigger_spell( talents.vipers_venom );

  buffs.tip_of_the_spear =
    make_buff( this, "tip_of_the_spear", find_spell( 260286 ) )
      -> set_default_value_from_effect( 1 )
      -> set_chance( talents.tip_of_the_spear.ok() );

  buffs.mongoose_fury =
    make_buff( this, "mongoose_fury", find_spell( 259388 ) )
      -> set_default_value_from_effect( 1 )
      -> set_refresh_behavior( buff_refresh_behavior::DISABLED );

  buffs.predator =
    make_buff( this, "predator", find_spell( 260249 ) )
      -> set_default_value_from_effect( 1 )
      -> add_invalidate( CACHE_ATTACK_SPEED );

  buffs.terms_of_engagement =
    make_buff( this, "terms_of_engagement", find_spell( 265898 ) )
      -> set_default_value_from_effect( 1, 1 / 5.0 )
      -> set_affects_regen( true );

  buffs.aspect_of_the_eagle =
    make_buff( this, "aspect_of_the_eagle", specs.aspect_of_the_eagle )
      -> set_cooldown( 0_ms );

  // Conduits

  buffs.brutal_projectiles =
    make_buff( this, "brutal_projectiles", find_spell( 339928 ) )
      -> set_chance( conduits.brutal_projectiles -> effectN( 2 ).percent() );

  buffs.brutal_projectiles_hidden =
    make_buff( this, "brutal_projectiles_hidden", find_spell( 339929 ) )
      -> set_default_value( conduits.brutal_projectiles.percent() );

  buffs.empowered_release =
    make_buff( this, "empowered_release", find_spell( 339061 ) )
      -> set_default_value( conduits.empowered_release.percent() )
      -> set_chance( conduits.empowered_release.ok() );

  buffs.flame_infusion =
    make_buff( this, "flame_infusion", find_spell( 341401 ) )
      -> set_default_value( conduits.flame_infusion.percent() )
      -> set_trigger_spell( conduits.flame_infusion );

  buffs.strength_of_the_pack =
    make_buff( this, "strength_of_the_pack", find_spell( 341223 ) )
      -> set_default_value( conduits.strength_of_the_pack.percent() )
      -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
      -> set_chance( conduits.strength_of_the_pack.ok() );

  // Covenants

  buffs.flayers_mark =
    make_buff( this, "flayers_mark", find_spell( 324156 ) )
      -> set_default_value_from_effect( 3 )
      -> set_chance( covenants.flayed_shot -> effectN( 2 ).percent() +
                     conduits.empowered_release -> effectN( 1 ).percent() );

  buffs.resonating_arrow =
    make_buff( this, "resonating_arrow", covenants.resonating_arrow -> effectN( 1 ).trigger() )
      -> set_default_value( find_spell( 308498 ) -> effectN( 1 ).percent() )
      -> set_activated( true );

  buffs.wild_spirits =
      make_buff( this, "wild_spirits", covenants.wild_spirits -> effectN( 1 ).trigger() )
        -> set_default_value( find_spell( 328275 ) -> effectN( 2 ).percent() )
        -> set_activated( true )
        -> apply_affecting_conduit( conduits.spirit_attunement );

  // Legendaries

  buffs.butchers_bone_fragments =
    make_buff( this, "butchers_bone_fragments", legendary.butchers_bone_fragments -> effectN( 1 ).trigger() )
      -> set_default_value_from_effect( 1 )
      -> set_trigger_spell( legendary.butchers_bone_fragments );

  buffs.eagletalons_true_focus =
    make_buff( this, "eagletalons_true_focus", legendary.eagletalons_true_focus -> effectN( 1 ).trigger() )
      -> set_default_value_from_effect( 1 )
      -> set_trigger_spell( legendary.eagletalons_true_focus );

  buffs.flamewakers_cobra_sting =
    make_buff( this, "flamewakers_cobra_sting", legendary.flamewakers_cobra_sting -> effectN( 1 ).trigger() )
      -> set_default_value_from_effect( 1 )
      -> set_trigger_spell( legendary.flamewakers_cobra_sting );

  buffs.nesingwarys_apparatus =
    make_buff( this, "nesingwarys_trapping_apparatus", find_spell( 336744 ) )
      -> set_default_value_from_effect( 2 )
      -> set_chance( legendary.nesingwarys_apparatus.ok() );

  buffs.secrets_of_the_vigil =
    make_buff( this, "secrets_of_the_unblinking_vigil", legendary.secrets_of_the_vigil -> effectN( 1 ).trigger() )
      -> set_default_value_from_effect( 1 )
      -> set_trigger_spell( legendary.secrets_of_the_vigil );
}

// hunter_t::init_gains =====================================================

void hunter_t::init_gains()
{
  player_t::init_gains();

  gains.aspect_of_the_wild     = get_gain( "aspect_of_the_wild" );
  gains.barbed_shot            = get_gain( "barbed_shot" );
  gains.nesingwarys_apparatus_direct = get_gain( "Nesingwary's Trapping Apparatus (Direct)" );
  gains.nesingwarys_apparatus_buff   = get_gain( "Nesingwary's Trapping Apparatus (Buff)" );
  gains.reversal_of_fortune    = get_gain( "Reversal of Fortune" );
  gains.terms_of_engagement    = get_gain( "terms_of_engagement" );
  gains.trueshot               = get_gain( "Trueshot" );
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

// hunter_t::reset ==========================================================

void hunter_t::reset()
{
  player_t::reset();

  // Active
  pets.main = nullptr;
  state = {};
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

  return crit;
}

// hunter_t::composite_spell_crit_chance ===========================================

double hunter_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  crit += specs.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// hunter_t::composite_melee_speed ==========================================

double hunter_t::composite_melee_speed() const
{
  double s = player_t::composite_melee_speed();

  if ( buffs.predator -> check() )
    s /= 1 + buffs.predator -> check_stack_value();

  return s;
}

// hunter_t::composite_player_target_crit_chance ============================

double hunter_t::composite_player_target_crit_chance( player_t* target ) const
{
  double crit = player_t::composite_player_target_crit_chance( target );

  crit += buffs.resonating_arrow -> check_value();

  return crit;
}

// hunter_t::composite_player_multiplier ====================================

double hunter_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  m *= 1 + buffs.strength_of_the_pack -> check_value();

  return m;
}

// composite_player_target_multiplier ====================================

double hunter_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double d = player_t::composite_player_target_multiplier( target, school );

  d *= 1 + buffs.wild_spirits -> check_value();

  if ( conduits.enfeebled_mark.ok() && buffs.resonating_arrow -> check() )
    d *= 1 + conduits.enfeebled_mark.percent();

  return d;
}

// hunter_t::composite_player_pet_damage_multiplier ======================

double hunter_t::composite_player_pet_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s );

  m *= 1 + specs.beast_mastery_hunter -> effectN( 3 ).percent();
  m *= 1 + specs.survival_hunter -> effectN( 3 ).percent();
  m *= 1 + specs.marksmanship_hunter -> effectN( 3 ).percent();

  m *= 1 + talents.animal_companion -> effectN( 2 ).percent();

  m *= 1 + buffs.strength_of_the_pack -> check_value();

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
    double regen = total_regen * specs.trueshot -> effectN( 6 ).percent();
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
      add_gain( specs.trueshot -> effectN( 5 ).percent(), gains.trueshot );

    if ( buffs.nesingwarys_apparatus -> check() )
      add_gain( buffs.nesingwarys_apparatus -> check_value(), gains.nesingwarys_apparatus_buff );

    const double additional_amount = floor( amount ) - initial_amount;
    if ( additional_amount > 0 )
    {
      for ( const auto& data : util::make_span( mul_gains ).subspan( 0, mul_gains_count ) )
        actual_amount += player_t::resource_gain( RESOURCE_FOCUS, additional_amount * ( data.first / mul_sum ), data.second, action );
    }
  }

  return actual_amount;
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

  add_option( opt_bool( "hunter.stomp_triggers_wild_spirits", options.stomp_triggers_wild_spirits ) );

  add_option( opt_obsoleted( "hunter_fixed_time" ) );
  add_option( opt_obsoleted( "hunter.brutal_projectiles_on_execute" ) );
  add_option( opt_obsoleted( "hunter.memory_of_lucid_dreams_proc_chance" ) );
  add_option( opt_obsoleted( "hunter.serpenstalkers_triggers_wild_spirits" ) );
}

// hunter_t::create_profile =================================================

std::string hunter_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  const options_t defaults{};
  auto print_option = [&] ( auto ref, util::string_view name ) {
    if ( range::invoke( ref, options ) != range::invoke( ref, defaults ) )
      fmt::format_to( std::back_inserter( profile_str ), "{}={}\n", name, range::invoke( ref, options ) );
  };

  print_option( &options_t::summon_pet_str, "summon_pet" );
  print_option( &options_t::pet_attack_speed, "hunter.pet_attack_speed" );
  print_option( &options_t::pet_basic_attack_delay, "hunter.pet_basic_attack_delay" );

  print_option( &options_t::stomp_triggers_wild_spirits, "hunter.stomp_triggers_wild_spirits" );

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
