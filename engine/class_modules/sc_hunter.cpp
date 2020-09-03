//==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <memory>

#include "simulationcraft.hpp"
#include "player/covenant.hpp"
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

// Wild Spirits proc type
enum class wild_spirits_proc_e : uint8_t {
  DEFAULT = 0, ST, MT, NONE
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
    buff_t* hunters_mark;
    buff_t* latent_poison;
    buff_t* latent_poison_injection;
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

  struct {
    spell_data_ptr_t death_chakram; // VERY WIP
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
    spell_data_ptr_t nessingwarys_apparatus;
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

  struct {
    azerite_essence_t memory_of_lucid_dreams; // Memory of Lucid Dreams Minor
    double memory_of_lucid_dreams_major_mult = 0;
    double memory_of_lucid_dreams_minor_mult = 0;
    azerite_essence_t vision_of_perfection;
    double vision_of_perfection_major_mult = 0;
  } azerite_essence;

  // Buffs
  struct buffs_t
  {
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
    buff_t* precise_shots;
    buff_t* steady_focus;
    buff_t* streamline;
    buff_t* trick_shots;
    buff_t* trueshot;
    buff_t* volley;

    // Survival
    buff_t* aspect_of_the_eagle;
    buff_t* coordinated_assault;
    buff_t* coordinated_assault_vision;
    buff_t* mongoose_fury;
    buff_t* predator;
    buff_t* terms_of_engagement;
    buff_t* tip_of_the_spear;
    buff_t* vipers_venom;

    // Legendaries
    buff_t* butchers_bone_fragments;
    buff_t* eagletalons_true_focus;
    buff_t* flamewakers_cobra_sting;
    buff_t* secrets_of_the_vigil;

    // Conduits
    buff_t* brutal_projectiles;
    buff_t* brutal_projectiles_hidden;
    buff_t* flame_infusion;
    buff_t* strength_of_the_pack;

    // Covenants
    buff_t* flayers_mark;
    buff_t* resonating_arrow;
    buff_t* wild_spirits;

    // azerite
    buff_t* blur_of_talons;
    buff_t* blur_of_talons_vision;
    buff_t* dance_of_death;
    buff_t* haze_of_rage;
    buff_t* in_the_rhythm;
    buff_t* primal_instincts;
    buff_t* primeval_intuition;
    buff_t* unerring_vision;
    buff_t* unerring_vision_driver;
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
    cooldown_t* rapid_fire;
    cooldown_t* trueshot;
    cooldown_t* wildfire_bomb;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* aspect_of_the_wild;
    gain_t* barbed_shot;
    gain_t* memory_of_lucid_dreams_major;
    gain_t* memory_of_lucid_dreams_minor;
    gain_t* nessingwarys_apparatus;
    gain_t* reversal_of_fortune;
    gain_t* terms_of_engagement;
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

    spell_data_ptr_t master_marksman; // NYI
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

    spell_data_ptr_t hunters_mark;
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
    struct { // Wild Spirits Night Fae Covenant ability
      action_t* st = nullptr;
      action_t* mt = nullptr;
      cooldown_t* icd = nullptr; // XXX: assume the icd is shared
    } wild_spirits;
  } actions;

  cdwaste::player_data_t cd_waste;

  struct {
    player_t* current_hunters_mark_target = nullptr;
    events::tar_trap_aoe_t* tar_trap_aoe = nullptr;
    wildfire_infusion_e next_wi_bomb = WILDFIRE_INFUSION_SHRAPNEL;
    unsigned steady_focus_counter = 0;
  } var;

  struct options_t {
    std::string summon_pet_str = "turtle";
    timespan_t pet_attack_speed = 2_s;
    timespan_t pet_basic_attack_delay = 0.15_s;
    double memory_of_lucid_dreams_proc_chance = 0.15;
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
    cooldowns.rapid_fire          = get_cooldown( "rapid_fire" );
    cooldowns.trueshot            = get_cooldown( "trueshot" );
    cooldowns.wildfire_bomb       = get_cooldown( "wildfire_bomb" );

    base_gcd = 1.5_s;

    resource_regeneration = regen_type::DYNAMIC;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }

  // Character Definition
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
  void      combat_begin() override;

  void datacollection_begin() override;
  void datacollection_end() override;

  double    composite_attack_power_multiplier() const override;
  double    composite_melee_crit_chance() const override;
  double    composite_spell_crit_chance() const override;
  double    composite_melee_haste() const override;
  double    composite_melee_speed() const override;
  double    composite_spell_haste() const override;
  double    composite_player_target_crit_chance( player_t* target ) const override;
  double    composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  double    composite_player_multiplier( school_e school ) const override;
  double    composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double    composite_player_pet_damage_multiplier( const action_state_t* ) const override;
  double    matching_gear_multiplier( attribute_e attr ) const override;
  void      invalidate_cache( cache_e ) override;
  void      regen( timespan_t periodicity ) override;
  double    resource_gain( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;
  void      create_options() override;
  std::unique_ptr<expr_t>   create_expression( util::string_view name ) override;
  std::unique_ptr<expr_t>   create_action_expression( action_t&, util::string_view name ) override;
  action_t* create_action( util::string_view name, const std::string& options ) override;
  pet_t*    create_pet( util::string_view name, util::string_view type ) override;
  void      create_pets() override;
  resource_e primary_resource() const override { return RESOURCE_FOCUS; }
  role_e    primary_role() const override { return ROLE_ATTACK; }
  stat_e    convert_hybrid_stat( stat_e s ) const override;
  std::string      create_profile( save_e ) override;
  void      copy_from( player_t* source ) override;
  void      moving( ) override;
  void      vision_of_perfection_proc() override;

  void              apl_default();
  void              apl_surv();
  void              apl_bm();
  void              apl_mm();
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  void apply_affecting_auras( action_t& ) override;

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

  void trigger_wild_spirits( const action_state_t* s, wild_spirits_proc_e type );
  void trigger_birds_of_prey( player_t* t );
  void trigger_bloodseeker_update();
  void trigger_lethal_shots();
  void trigger_calling_the_shots();
  void trigger_nessingwarys_apparatus( action_t* a );

  void consume_trick_shots();
};

wild_spirits_proc_e wild_spirits_proc_type( const hunter_t& p, const action_t& a )
{
  if ( a.proc || a.background )
    return wild_spirits_proc_e::NONE;
  if ( !( a.callbacks && a.may_hit && a.harmful ) )
    return wild_spirits_proc_e::NONE;
  auto trigger = p.covenants.wild_spirits -> effectN( 1 ).trigger();
  if ( ( trigger -> proc_flags() & ( 1 << a.proc_type() ) ) == 0 )
    return wild_spirits_proc_e::NONE;
  return ( a.aoe == -1 || a.aoe > 0 ) ? wild_spirits_proc_e::MT : wild_spirits_proc_e::ST;
}

// Template for common hunter action code.
template <class Base>
struct hunter_action_t: public Base
{
private:
  using ab = Base;
public:

  bool precombat = false;
  bool track_cd_waste;
  wild_spirits_proc_e wild_spirits_proc = wild_spirits_proc_e::DEFAULT;

  struct {
    // bm
    bool aotw_crit_chance = false;
    bool aotw_gcd_reduce = false;
    damage_affected_by bestial_wrath;
    damage_affected_by master_of_beasts;
    bool thrill_of_the_hunt = false;
    // mm
    damage_affected_by lone_wolf;
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
      if ( wild_spirits_proc == wild_spirits_proc_e::DEFAULT )
        wild_spirits_proc = wild_spirits_proc_type( *p(), *this );
    }
    else
    {
      wild_spirits_proc = wild_spirits_proc_e::NONE;
    }

    assert( wild_spirits_proc != wild_spirits_proc_e::DEFAULT );
    if ( wild_spirits_proc != wild_spirits_proc_e::NONE )
      ab::sim -> print_debug( "{} action {} set to proc Wild Spirits", ab::player -> name(), ab::name() );
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

    if ( wild_spirits_proc != wild_spirits_proc_e::NONE )
      p() -> trigger_wild_spirits( s, wild_spirits_proc );
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

    if ( affected_by.lone_wolf.direct && p() -> pets.main == nullptr )
      am *= 1 + p() -> specs.lone_wolf -> effectN( affected_by.lone_wolf.direct ).percent();

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

    if ( affected_by.lone_wolf.tick && p() -> pets.main == nullptr )
      am *= 1 + p() -> specs.lone_wolf -> effectN( affected_by.lone_wolf.tick ).percent();

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

    return c;
  }

  void update_ready( timespan_t cd ) override
  {
    if ( cd_waste )
      cd_waste -> update_ready( this, cd );

    ab::update_ready( cd );
  }

  void consume_resource() override
  {
    ab::consume_resource();

    if ( !p() -> azerite_essence.memory_of_lucid_dreams.enabled() )
      return;
    if ( ab::last_resource_cost <= 0 )
      return;

    if ( !ab::rng().roll( p() -> options.memory_of_lucid_dreams_proc_chance ) )
      return;

    const double gain = ab::last_resource_cost * p() -> azerite_essence.memory_of_lucid_dreams_minor_mult;
    p() -> resource_gain( RESOURCE_FOCUS, gain, p() -> gains.memory_of_lucid_dreams_minor );

    if ( p() -> azerite_essence.memory_of_lucid_dreams.rank() >= 3 )
      ab::player -> buffs.lucid_dreams -> trigger();
  }

  virtual double cast_regen( const action_state_t* s ) const
  {
    const timespan_t execute_time = this -> execute_time();
    const timespan_t cast_time = std::max( execute_time, this -> gcd() );
    const double regen = p() -> resource_regen_per_second( RESOURCE_FOCUS );
    const int num_targets = this -> n_targets();
    size_t targets_hit = 1;
    if ( ab::energize_type == action_energize::PER_HIT && ( num_targets == -1 || num_targets > 0 ) )
    {
      size_t tl_size = this -> target_list().size();
      targets_hit = ( num_targets < 0 ) ? tl_size : std::min( tl_size, as<size_t>( num_targets ) );
    }
    double total_regen = regen * cast_time.total_seconds();
    double total_energize = targets_hit * this -> composite_energize_amount( s );

    if ( ab::player -> buffs.memory_of_lucid_dreams -> check() )
    {
      const timespan_t remains = ab::player -> buffs.memory_of_lucid_dreams -> remains();

      total_regen += regen * std::min( cast_time, remains ).total_seconds() *
                     p() -> azerite_essence.memory_of_lucid_dreams_major_mult;
      if ( execute_time < remains )
        total_energize *= 1 + p() -> azerite_essence.memory_of_lucid_dreams_major_mult;
    }

    return total_regen + total_energize;
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
    if ( triggered && precombat && !in_combat && precast_time > 0_ms )
    {
      buff -> extend_duration( ab::player, -std::min( precast_time, buff -> buff_duration() ) );
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

struct hunter_ranged_attack_t: public hunter_action_t < ranged_attack_t >
{
  bool breaks_steady_focus = true;
  maybe_bool triggers_master_marksman;

  hunter_ranged_attack_t( util::string_view n, hunter_t* player,
                          const spell_data_t* s = spell_data_t::nil() ):
                          hunter_action_t( n, player, s )
  {}

  void init() override
  {
    hunter_action_t::init();

    if ( triggers_master_marksman.is_none() )
      triggers_master_marksman = special;
  }

  bool usable_moving() const override
  { return true; }

  void execute() override
  {
    hunter_action_t::execute();

    if ( !background && breaks_steady_focus )
      p() -> var.steady_focus_counter = 0;
  }

  void impact( action_state_t* s ) override
  {
    hunter_action_t::impact( s );

    if ( p() -> talents.master_marksman.ok() && triggers_master_marksman && s -> result == RESULT_CRIT )
    {
      double amount = s -> result_amount * p() -> talents.master_marksman -> effectN( 1 ).percent();
      if ( amount > 0 )
        residual_action::trigger( p() -> actions.master_marksman, s -> target, amount );
    }
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

    if ( auto td = o() -> find_target_data( target ) )
      m *= 1 + td -> debuffs.hunters_mark -> check_value();

    m *= 1 + o() -> buffs.wild_spirits -> check_value();

    // TODO: check Resonating Arrow w/ Enfeebled Mark

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
    action_t* kill_command = nullptr;
    attack_t* beast_cleave = nullptr;
    action_t* stomp = nullptr;
    action_t* flanking_strike = nullptr;
    action_t* bloodshed = nullptr;
  } active;

  struct buffs_t
  {
    buff_t* bestial_wrath = nullptr;
    buff_t* beast_cleave = nullptr;
    buff_t* frenzy = nullptr;
    buff_t* predator = nullptr;
    buff_t* rylakstalkers_fangs = nullptr;
  } buffs;

  struct {
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
        -> add_invalidate( CACHE_ATTACK_SPEED )
        -> apply_affecting_aura( o() -> azerite.feeding_frenzy.spell() -> effectN( 1 ).trigger() );

    buffs.rylakstalkers_fangs =
      make_buff( this, "rylakstalkers_piercing_fangs", o() -> find_spell( 336845 ) )
        -> set_default_value_from_effect( 1 )
        -> set_chance( o() -> legendary.rylakstalkers_fangs.ok() );
  }

  double composite_melee_speed() const override
  {
    double ah = hunter_pet_t::composite_melee_speed();

    if ( buffs.frenzy -> check() )
      ah *= 1.0 / ( 1 + buffs.frenzy -> check_stack_value() );

    if ( buffs.predator && buffs.predator -> check() )
      ah *= 1.0 / ( 1 + buffs.predator -> check_stack_value() );

    return ah;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = hunter_pet_t::composite_player_multiplier( school );

    m *= 1 + buffs.bestial_wrath -> check_value();
    m *= 1 + o() -> buffs.coordinated_assault -> check_value();

    return m;
  }

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

  action_t* create_action( util::string_view name, const std::string& options_str ) override;

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

  void init_action_list() override
  {
    if ( action_list_str.empty() )
    {
      action_list_str += "/auto_attack";
      use_default_action_list = true;
    }

    hunter_main_pet_base_t::init_action_list();
  }

  timespan_t available() const override
  {
    return sim -> expected_iteration_time * 2;
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

  struct {
    spell_data_ptr_t bloodshed;
  } spells;

  double owner_hp_mult = 1;

  hunter_main_pet_t( hunter_t* owner, util::string_view pet_name, pet_e pt ):
    hunter_main_pet_base_t( owner, pet_name, pt )
  {
    // FIXME work around assert in pet specs
    // Set default specs
    _spec = default_spec();

    if ( _spec == PET_TENACITY )
    {
      spell_data_ptr_t endurance_training = find_spell( 264662 );
      owner_hp_mult = 1 + endurance_training->effectN( 2 ).percent() * ( 1 + o() -> talents.aspect_of_the_beast->effectN( 4 ).percent() );
    }
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

  double composite_player_target_multiplier( player_t* target, school_e school ) const override
  {
    double m = hunter_pet_t::composite_player_target_multiplier( target, school );

    const hunter_main_pet_td_t* td = find_target_data( target );
    if ( td && td -> dots.bloodshed -> is_ticking() )
      m *= 1 + spells.bloodshed -> effectN( 2 ).percent();

    return m;
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
      o() -> pets.animal_companion -> summon();

    o() -> resources.initial_multiplier[ RESOURCE_HEALTH ] *= owner_hp_mult;
    o() -> recalculate_resource_max( RESOURCE_HEALTH );
  }

  void demise() override
  {
    hunter_main_pet_base_t::demise();

    if ( o() -> pets.main == this )
    {
      o() -> pets.main = nullptr;

      o() -> resources.initial_multiplier[ RESOURCE_HEALTH ] /= owner_hp_mult;
      o() -> recalculate_resource_max( RESOURCE_HEALTH );
    }
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

  const hunter_main_pet_td_t* find_target_data( const player_t* target ) const
  {
    return target_data[ target ];
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
    hunter_pet_t( owner, n, PET_HUNTER, true /*GUARDIAN*/ )
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
  const timespan_t base_duration = p -> talents.dire_beast -> duration();
  const timespan_t swing_time = 2_s * p -> cache.attack_speed();
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
    hunter_pet_t( o, "spitting_cobra", PET_HUNTER, true )
  {
    owner_coeff.ap_from_ap = 0.15;
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
      serrated_jaws.gain = p ->resource_regeneration != regen_type::DISABLED ? p -> get_gain( "serrated_jaws" ) : nullptr;
    }
  }

  void execute() override
  {
    serrated_jaws.procced = rng().roll( serrated_jaws.chance );

    hunter_pet_action_t::execute();

    if ( serrated_jaws.procced && serrated_jaws.gain )
      p() -> resource_gain( RESOURCE_FOCUS, serrated_jaws.energize_amount, serrated_jaws.gain, this );
  }

  double composite_attack_power() const override
  {
    // Kill Command for both Survival & Beast Mastery uses player AP directly
    return o() -> cache.attack_power() * o() -> composite_attack_power_multiplier();
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
    double multiplier = 1;
    benefit_t* benefit = nullptr;
  } killer_instinct;
  timespan_t ferocious_appetite_reduction;

  kill_command_bm_t( hunter_main_pet_base_t* p ):
    kill_command_base_t( p, p -> find_spell( 83381 ) )
  {
    attack_power_mod.direct = o() -> specs.kill_command -> effectN( 2 ).percent();
    base_dd_adder += o() -> azerite.dire_consequences.value( 1 );

    if ( o() -> talents.killer_instinct.ok() )
    {
      killer_instinct.percent = o() -> talents.killer_instinct -> effectN( 2 ).base_value();
      killer_instinct.multiplier = 1 + o() -> talents.killer_instinct -> effectN( 1 ).percent();
      killer_instinct.benefit = o() -> get_benefit( "killer_instinct" );
    }

    if ( o() -> conduits.ferocious_appetite.ok() )
      ferocious_appetite_reduction = timespan_t::from_seconds( o() -> conduits.ferocious_appetite.value() / 10 );
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
    base_dd_adder += o() -> azerite.dire_consequences.value( 2 );

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

// Pet Auto Attack ==========================================================

struct pet_auto_attack_t: public action_t
{
  pet_auto_attack_t( hunter_main_pet_base_t* player, util::string_view options_str ):
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
    double multiplier = 1;
    benefit_t* benefit = nullptr;
  } wild_hunt;
  const double venomous_fangs_bonus_da;

  basic_attack_t( hunter_main_pet_t* p, util::string_view n, util::string_view options_str ):
    hunter_main_pet_attack_t( n, p, p -> find_pet_spell( n ) ),
    venomous_fangs_bonus_da( p -> o() -> azerite.venomous_fangs.value( 1 ) )
  {
    parse_options( options_str );

    school = SCHOOL_PHYSICAL;

    attack_power_mod.direct = 1 / 3.0;
    // 28-06-2018: While spell data says it has a base damage in-game testing shows that it doesn't use it.
    base_dd_min = base_dd_max = 0;

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

// Bloodshed ===============================================================

struct bloodshed_t : hunter_main_pet_attack_t
{
  bloodshed_t( hunter_main_pet_t* p ):
    hunter_main_pet_attack_t( "bloodshed", p, p -> spells.bloodshed )
  {
    background = true;
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

action_t* hunter_main_pet_base_t::create_action( util::string_view name, const std::string& options_str )
{
  if ( name == "auto_attack" )
    return new actions::pet_auto_attack_t( this, options_str );

  return hunter_pet_t::create_action( name, options_str );
}

action_t* hunter_main_pet_t::create_action( util::string_view name,
                                            const std::string& options_str )
{
  using namespace actions;

  if ( name == "claw" ) return new                 basic_attack_t( this, "Claw", options_str );
  if ( name == "bite" ) return new                 basic_attack_t( this, "Bite", options_str );
  if ( name == "smack" ) return new                basic_attack_t( this, "Smack", options_str );

  return hunter_main_pet_base_t::create_action( name, options_str );
}

// hunter_t::init_spells ====================================================

void hunter_main_pet_base_t::init_spells()
{
  hunter_pet_t::init_spells();

  spells.thrill_of_the_hunt = find_spell( 312365 );

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

  spells.bloodshed = find_spell( 321538 );

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
    if ( p -> var.tar_trap_aoe == this )
      p -> var.tar_trap_aoe = nullptr;
    p -> sim -> print_debug( "{} Tar Trap at {:.3f}:{:.3f} expired ({})", p -> name(), x_position, y_position, *this );
  }
};

} // namespace events

void hunter_t::trigger_wild_spirits( const action_state_t* s, wild_spirits_proc_e type )
{
  assert( type == wild_spirits_proc_e::MT || type == wild_spirits_proc_e::ST );

  if ( !buffs.wild_spirits -> check() )
    return;

  if ( !action_t::result_is_hit( s -> result ) )
    return;

  if ( actions.wild_spirits.icd -> down() )
    return;

  action_t* action = type == wild_spirits_proc_e::MT ? actions.wild_spirits.mt : actions.wild_spirits.st;
  if ( sim -> debug )
  {
    sim -> print_debug( "{} procs {} from {} on {}",
      name(), action -> name(), s -> action -> name(), s -> target -> name() );
  }

  action -> set_target( s -> target );
  action -> execute();
  actions.wild_spirits.icd -> start();
}

void hunter_t::trigger_birds_of_prey( player_t* t )
{
  if ( !talents.birds_of_prey.ok() )
    return;

  if ( !pets.main )
    return;

  if ( t == pets.main -> target )
  {
    buffs.coordinated_assault -> extend_duration( this, talents.birds_of_prey -> effectN( 1 ).time_value() );
    buffs.coordinated_assault_vision -> extend_duration( this, talents.birds_of_prey -> effectN( 1 ).time_value() );
  }
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

void hunter_t::trigger_nessingwarys_apparatus( action_t* a )
{
  if ( !legendary.nessingwarys_apparatus.ok() )
    return;

  double amount = legendary.nessingwarys_apparatus -> effectN( 1 ).trigger()
                  -> effectN( 1 ).resource( RESOURCE_FOCUS );
  resource_gain( RESOURCE_FOCUS, amount, gains.nessingwarys_apparatus, a );
}

void hunter_t::consume_trick_shots()
{
  if ( buffs.volley -> up() )
    return;

  if ( buffs.secrets_of_the_vigil -> up() )
  {
    buffs.secrets_of_the_vigil -> decrement();
    return;
  }

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
 * 2020.08.10 9.0.1.35432
 * YUGE DISCLAIMER! VERY WIP!
 * The spell consists of at least 3 spells:
 *   325028 - main spell, does all damage on multiple targets
 *   325037 - additional damage done on single target
 *   325553 - energize (amount is in 325028)
 *
 * Below analysis is done based on this log from beta:
 *   https://www.warcraftlogs.com/reports/rFh9gLHqZya2YmRv
 *
 * The spell behaves very differently when it can hit multiple or only a
 * single target.
 *
 * Observations when it hits multiple targets:
 *   - only 325028 is in play
 *   - damage events after the initial travel time are not instant
 *   - all energizes happen on cast (7 total 325553 energize events)
 *   - the spell can "bounce" to dead targets
 *
 * Observations when there is a single target involved:
 *   - 325028 is only the initial hit
 *   - 325037 starts hitting ~1 after the initial hit, doing 6 hits total
 *     every ~745ms [*]
 *   - only 1 325553 energize happens on cast, all others happen when 325037
 *     hit (sometimes with a slight desync)
 *
 * [*] period data for 325037 hitting the main target:
 *        Min    Max  Median    Avg  Stddev
 *      0.643  0.809   0.745  0.744  0.0357
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
 * 6 hits of 325037, first hit in ~1s, 745ms period after (it may actually even
 * be driven by a hidden dot internally)
 *
 * Somewhat simplified (in multi-target) modeling of the theory for now:
 *  - in single-target case - let the main spell hit as usual (once), then
 *    schedule 1 "secondary" hit after 1s which schedules another 5 repeating
 *    hits with a 745ms period
 *  - in multi-target case - just model at as a simple "instant" chained aoe
 */

struct base_t : hunter_ranged_attack_t
{
  base_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
    hunter_ranged_attack_t( n, p, s )
  {
    chain_multiplier = p -> covenants.death_chakram -> effectN( 1 ).chain_multiplier();

    energize_type = action_energize::ON_HIT;
    energize_resource = RESOURCE_FOCUS;
    energize_amount = p -> covenants.death_chakram -> effectN( 4 ).base_value() +
                      p -> conduits.necrotic_barrage -> effectN( 2 ).base_value();
  }
};

struct single_target_t final : base_t
{
  int hit_number = 0;

  single_target_t( util::string_view n, hunter_t* p ):
    base_t( n, p, p -> find_spell( 325037 ) )
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
    if ( hit_number == action.p() -> covenants.death_chakram -> effectN( 1 ).chain_target() )
      return;

    make_event<single_target_event_t>( sim(), action, target, 745_ms, hit_number );
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
    base_t::impact( s );

    if ( s -> n_targets == 1 )
    {
      // Hit only a single target, schedule the repeating single target hitter
      make_event<death_chakram::single_target_event_t>( *sim, *single_target, s -> target, 1_s );
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

    p() -> buffs.flayers_mark -> trigger();
  }
};

struct resonating_arrow_t : hunter_spell_t
{
  resonating_arrow_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "resonating_arrow", p, p -> covenants.resonating_arrow )
  {
    parse_options( options_str );

    harmful = may_hit = may_miss = false;
  }

  void execute()
  {
    hunter_spell_t::execute();

    p() -> buffs.resonating_arrow -> trigger();
  }
};

struct wild_spirits_t : hunter_spell_t
{
  struct wild_spirits_proc_t final : hunter_spell_t
  {
    wild_spirits_proc_t( util::string_view n, hunter_t* p, unsigned spell_id ):
      hunter_spell_t( n, p, p -> find_spell( spell_id ) )
    {
      proc = true;
      callbacks = false;
    }
  };

  wild_spirits_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "wild_spirits", p, p -> covenants.wild_spirits )
  {
    parse_options( options_str );

    harmful = may_hit = may_miss = false;

    p -> actions.wild_spirits.st = p -> get_background_action<wild_spirits_proc_t>( "wild_spirits_st_proc", 328523 );
    p -> actions.wild_spirits.mt = p -> get_background_action<wild_spirits_proc_t>( "wild_spirits_mt_proc", 328757 );
    p -> actions.wild_spirits.icd = p -> get_cooldown( "wild_spirits_proc" );
    p -> actions.wild_spirits.icd -> duration = p -> covenants.wild_spirits -> effectN( 1 ).trigger() -> internal_cooldown();
    add_child( p -> actions.wild_spirits.st );
    add_child( p -> actions.wild_spirits.mt );
  }

  void execute()
  {
    hunter_spell_t::execute();

    p() -> buffs.wild_spirits -> trigger();
  }
};

// ==========================================================================
// Hunter Attacks
// ==========================================================================

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
    return new auto_shot_state_t( this, target );
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
  struct barrage_damage_t final : public hunter_ranged_attack_t
  {
    barrage_damage_t( util::string_view n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> talents.barrage -> effectN( 1 ).trigger() )
    {
      radius = 0; //Barrage attacks all targets in front of the hunter, so setting radius to 0 will prevent distance targeting from using a 40 yard radius around the target.
      // Todo: Add in support to only hit targets in the frontal cone.
    }
  };

  barrage_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "barrage", p, p -> talents.barrage )
  {
    parse_options( options_str );

    may_miss = may_crit = false;
    channeled = true;

    tick_zero = true;
    travel_speed = 0;

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
  struct rapid_reload_t final : public hunter_ranged_attack_t
  {
    rapid_reload_t( util::string_view n, hunter_t* p ):
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

  multi_shot_t( hunter_t* p, util::string_view options_str ):
    hunter_ranged_attack_t( "multishot", p, p -> find_class_spell( "Multi-Shot" ) )
  {
    parse_options( options_str );

    if ( p -> azerite.rapid_reload.ok() )
    {
      rapid_reload.action = p -> get_background_action<rapid_reload_t>( "multishot_rapid_reload" );
      rapid_reload.min_targets = as<int>(p -> azerite.rapid_reload.spell() -> effectN( 2 ).base_value());
      rapid_reload.reduction = ( timespan_t::from_seconds( p -> azerite.rapid_reload.spell() -> effectN ( 3 ).base_value() ) );
      add_child( rapid_reload.action );
    }
  }

  double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    am *= 1 + p() -> buffs.precise_shots -> check_value();

    return am;
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

    if ( rapid_reload.action && num_targets_hit > rapid_reload.min_targets )
    {
      rapid_reload.action -> set_target( target );
      p() -> cooldowns.aspect_of_the_wild -> adjust( - ( rapid_reload.reduction * num_targets_hit ) );
      rapid_reload.action -> execute();
    }
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
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      p() -> buffs.dead_eye -> trigger();
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

    return am;
  }
};

// Arcane Shot (Base) =================================================================

struct arcane_shot_base_t: public hunter_ranged_attack_t
{
  arcane_shot_base_t( util::string_view name, hunter_t* p ):
    hunter_ranged_attack_t( name, p, p -> find_class_spell( "Arcane Shot" ) )
  { }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> trigger_lethal_shots();
    p() -> trigger_calling_the_shots();
  }

  double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    am *= 1 + p() -> buffs.precise_shots -> check_value();

    return am;
  }
};

// Arcane Shot ========================================================================

struct arcane_shot_t: public arcane_shot_base_t
{
  arcane_shot_t( hunter_t* p, util::string_view options_str ):
    arcane_shot_base_t( "arcane_shot", p )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    arcane_shot_base_t::execute();

    p() -> buffs.precise_shots -> up(); // benefit tracking
    p() -> buffs.precise_shots -> decrement();
  }
};

// Chimaera Shot (Base) ===============================================================

struct chimaera_shot_base_t: public hunter_ranged_attack_t
{
  struct impact_t final : public hunter_ranged_attack_t
  {
    impact_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
      hunter_ranged_attack_t( n, p, s )
    {
      dual = true;

      // Beast Mastery focus gain
      if ( p -> specs.beast_mastery_hunter.ok() )
        parse_effect_data( p -> find_spell( 204304 ) -> effectN( 1 ) );
    }

    void execute() override
    {
      hunter_ranged_attack_t::execute();

      p() -> trigger_lethal_shots();
    }

    double action_multiplier() const override
    {
      double am = hunter_ranged_attack_t::action_multiplier();

      am *= 1 + p() -> buffs.precise_shots -> check_value();

      return am;
    }
  };

  std::array<impact_t*, 2> damage;
  unsigned current_damage_action = 0;

  chimaera_shot_base_t( util::string_view n, hunter_t* p ):
    hunter_ranged_attack_t( n, p, p -> talents.chimaera_shot )
  {
    aoe = 2;
    radius = 5;

    constexpr std::array<unsigned, 2> bm_spells { { 171454, 171457 } };
    constexpr std::array<unsigned, 2> mm_spells { { 344121, 344120 } };
    const auto spells = p -> specialization() == HUNTER_MARKSMANSHIP ? mm_spells : bm_spells;
    damage[ 0 ] = p -> get_background_action<impact_t>( fmt::format( "{}_frost", n ), p -> find_spell( spells[ 0 ] ) );
    damage[ 1 ] = p -> get_background_action<impact_t>( fmt::format( "{}_nature", n ), p -> find_spell( spells[ 1 ] ) );
    for ( auto a : damage )
      add_child( a );

    school = SCHOOL_FROSTSTRIKE; // Just so the report shows a mixture of the two colors.
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    p() -> trigger_calling_the_shots();
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
};

// Chimaera Shot ======================================================================

struct chimaera_shot_t : chimaera_shot_base_t
{
  chimaera_shot_t( hunter_t* p, util::string_view options_str ):
    chimaera_shot_base_t( "chimaera_shot", p )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    chimaera_shot_base_t::execute();

    p() -> buffs.precise_shots -> up(); // benefit tracking
    p() -> buffs.precise_shots -> decrement();
  }

  double cast_regen( const action_state_t* s ) const override
  {
    const size_t targets_hit = std::min( target_list().size(), as<size_t>( aoe ) );
    return chimaera_shot_base_t::cast_regen( s ) +
           ( targets_hit * damage[ 0 ] -> composite_energize_amount( nullptr ) );
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

// Barbed Shot ===============================================================

struct barbed_shot_t: public hunter_ranged_attack_t
{
  timespan_t bestial_wrath_r2_reduction;

  barbed_shot_t( hunter_t* p, util::string_view options_str ) :
    hunter_ranged_attack_t( "barbed_shot", p, p -> specs.barbed_shot )
  {
    parse_options(options_str);

    base_ta_adder += p -> azerite.feeding_frenzy.value( 2 );

    if ( p -> find_rank_spell( "Bestial Wrath", "Rank 2" ) -> ok() )
      bestial_wrath_r2_reduction = timespan_t::from_seconds( p -> specs.bestial_wrath -> effectN( 3 ).base_value() );
  }

  void init_finished() override
  {
    for ( auto pet : p()->pet_list )
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

    if ( p() -> azerite.dance_of_death.ok() && rng().roll( composite_crit_chance() ) )
      p() -> buffs.dance_of_death -> trigger();

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

// Master Marksman ====================================================================

struct master_marksman_t : public residual_action::residual_periodic_action_t<hunter_ranged_attack_t>
{
  master_marksman_t( hunter_t* p ):
    residual_periodic_action_t( "master_marksman", p, p -> find_spell( 269576 ) )
  { }
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
  struct arcane_shot_sst_t final : public arcane_shot_base_t
  {
    arcane_shot_sst_t( util::string_view n, hunter_t* p ):
      arcane_shot_base_t( n, p )
    {
      dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
    }
  };
  struct chimaera_shot_sst_t final : public chimaera_shot_base_t
  {
    chimaera_shot_sst_t( util::string_view n, hunter_t* p ):
      chimaera_shot_base_t( n, p )
    {
      dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
    }
  };

  struct {
    double multiplier = 0;
    double high, low;
  } careful_aim;
  struct {
    hunter_ranged_attack_t* action = nullptr;
    int target_count = 0;
  } serpentstalkers_trickery;
  const int trick_shots_targets;

  aimed_shot_base_t( util::string_view name, hunter_t* p ):
    hunter_ranged_attack_t( name, p, p -> specs.aimed_shot ),
    trick_shots_targets( static_cast<int>( p -> specs.trick_shots -> effectN( 1 ).base_value() ) )
  {
    radius = 8;
    base_aoe_multiplier = p -> specs.trick_shots -> effectN( 4 ).percent() +
                          p -> conduits.deadly_chain.percent();

    if ( p -> talents.careful_aim.ok() )
    {
      careful_aim.high = p -> talents.careful_aim -> effectN( 1 ).base_value();
      careful_aim.low = p -> talents.careful_aim -> effectN( 2 ).base_value();
      careful_aim.multiplier = p -> talents.careful_aim -> effectN( 3 ).percent();
    }

    if ( p -> legendary.serpentstalkers_trickery.ok() )
    {
      if ( p -> talents.chimaera_shot.ok() )
        serpentstalkers_trickery.action = p -> get_background_action<chimaera_shot_sst_t>( "chimaera_shot_sst" );
      else
        serpentstalkers_trickery.action = p -> get_background_action<arcane_shot_sst_t>( "arcane_shot_sst" );
      serpentstalkers_trickery.target_count =
          as<int>( p -> legendary.serpentstalkers_trickery -> effectN( 1 ).base_value() );
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

    if ( careful_aim.multiplier )
    {
      const double target_health_pct = t -> health_percentage();
      if ( target_health_pct > careful_aim.high || target_health_pct < careful_aim.low )
        m *= 1 + careful_aim.multiplier;
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

  void schedule_travel( action_state_t* s ) override
  {
    hunter_ranged_attack_t::schedule_travel( s );

    if ( p() -> buffs.trick_shots -> check() && s -> chain_target < serpentstalkers_trickery.target_count )
    {
      serpentstalkers_trickery.action -> set_target( s -> target );
      serpentstalkers_trickery.action -> execute();
    }
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
  struct double_tap_t final : public aimed_shot_base_t
  {
    double_tap_t( util::string_view n, hunter_t* p ):
      aimed_shot_base_t( n, p )
    {
      dual = true;
      base_costs[ RESOURCE_FOCUS ] = 0;
    }
  };

  bool lock_and_loaded = false;
  struct {
    double_tap_t* action = nullptr;
    proc_t* proc;
  } double_tap;
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

    if ( p -> legendary.surging_shots.ok() )
    {
      surging_shots.chance = p -> legendary.surging_shots -> proc_chance();
      surging_shots.proc = p -> get_proc( "Surging Shots Rapid Fire reset" );
    }
    else if ( p -> azerite.surging_shots.ok() )
    {
      surging_shots.chance = p -> azerite.surging_shots.spell_ref().effectN( 1 ).percent();
      surging_shots.proc = p -> get_proc( "Surging Shots Rapid Fire reset" );
    }

    if ( serpentstalkers_trickery.action )
      add_child( serpentstalkers_trickery.action );
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
    // trigger Precise Shots before execute so that Arcane Shots from
    // Serpenstalker's Trickery get affected by it
    p() -> buffs.precise_shots -> trigger( 1 + rng().range( p() -> buffs.precise_shots -> max_stack() ) );

    aimed_shot_base_t::execute();

    if ( double_tap.action && p() -> buffs.double_tap -> check() )
    {
      double_tap.action -> set_target( target );
      double_tap.action -> execute();
      p() -> buffs.double_tap -> decrement();
      double_tap.proc -> occur();
    }

    p() -> buffs.trick_shots -> up(); // benefit tracking
    p() -> consume_trick_shots();

    if ( lock_and_loaded )
      p() -> buffs.lock_and_load -> decrement();
    lock_and_loaded = false;

    p() -> buffs.streamline -> decrement();

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

    p() -> var.steady_focus_counter += 1;
    if ( p() -> var.steady_focus_counter >= 2 )
      p() -> buffs.steady_focus -> trigger();
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( p() -> azerite.steady_aim.ok() )
      td( s -> target ) -> debuffs.steady_aim -> trigger();
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
      fmt::print( s, " double_tapped={:d}", double_tapped );
      return s;
    }

    void copy_state( const action_state_t* o ) override
    {
      action_state_t::copy_state( o );
      double_tapped = debug_cast<const rapid_fire_state_t*>( o ) -> double_tapped;
    }
  };

  struct rapid_fire_damage_t final : public hunter_ranged_attack_t
  {
    const int trick_shots_targets;
    struct {
      double chance = 0;
      double amount = 0;
      gain_t* gain = nullptr;
    } focused_fire;
    struct {
      double value = 0;
      int max_num_ticks = 0;
    } surging_shots;

    rapid_fire_damage_t( util::string_view n, hunter_t* p ):
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

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = hunter_ranged_attack_t::composite_da_multiplier( s );

      m *= 1 + p() -> buffs.brutal_projectiles_hidden -> check_stack_value();

      return m;
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

  rapid_fire_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "rapid_fire", p, p -> specs.rapid_fire ),
    damage( p -> get_background_action<rapid_fire_damage_t>( "rapid_fire_damage" ) ),
    base_num_ticks( as<int>(data().effectN( 1 ).base_value()) )
  {
    parse_options( options_str );

    may_miss = may_crit = false;
    channeled = true;

    procs.double_tap = p -> get_proc( "double_tap_rapid_fire" );
  }

  void init() override
  {
    hunter_spell_t::init();

    damage -> gain       = gain;
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

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.streamline -> trigger();
  }

  void tick( dot_t* d ) override
  {
    hunter_spell_t::tick( d );
    damage -> set_target( d -> target );
    damage -> execute();

    if ( p() -> buffs.brutal_projectiles -> up() )
      p() -> buffs.brutal_projectiles_hidden -> trigger();
  }

  void last_tick( dot_t* d ) override
  {
    hunter_spell_t::last_tick( d );

    p() -> consume_trick_shots();

    if ( p() -> buffs.double_tap -> check() )
      procs.double_tap -> occur();
    p() -> buffs.double_tap -> decrement();

    p() -> buffs.brutal_projectiles -> expire();
    p() -> buffs.brutal_projectiles_hidden -> expire();

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
      t *= 1 + p() -> talents.double_tap -> effectN( 1 ).percent();

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
    return new rapid_fire_state_t( this, target );
  }

  void snapshot_state( action_state_t* s, result_amount_type type ) override
  {
    hunter_spell_t::snapshot_state( s, type );
    debug_cast<rapid_fire_state_t*>( s ) -> double_tapped = p() -> buffs.double_tap -> check();
  }
};

// Explosive Shot  ====================================================================

struct explosive_shot_t: public hunter_ranged_attack_t
{
  struct explosive_shot_aoe_t final : hunter_ranged_attack_t
  {
    explosive_shot_aoe_t( util::string_view n, hunter_t* p ):
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

    may_miss = false;

    tick_action = p -> get_background_action<explosive_shot_aoe_t>( "explosive_shot_aoe" );
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
  struct internal_bleeding_action_t final : hunter_ranged_attack_t
  {
    internal_bleeding_action_t( util::string_view n, hunter_t* p ):
      hunter_ranged_attack_t( n, p, p -> find_spell( 270343 ) )
    {
      dual = true;
    }
  };
  internal_bleeding_action_t* action = nullptr;

  internal_bleeding_t( hunter_t* p )
  {
    if ( p -> talents.wildfire_infusion.ok() )
      action = p -> get_background_action<internal_bleeding_action_t>( "internal_bleeding" );
  }

  void trigger( const action_state_t* s )
  {
    if ( action )
    {
      auto td = action -> find_td( s -> target );
      if ( td && td -> dots.shrapnel_bomb -> is_ticking() )
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
  struct latent_poison_t final : hunter_spell_t
  {
    latent_poison_t( util::string_view n, hunter_t* p ):
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

  struct latent_poison_injection_t final : hunter_spell_t
  {
    latent_poison_injection_t( util::string_view n, hunter_t* p ):
      hunter_spell_t( n, p, p -> find_spell( 336904 ) )
    {}

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
  latent_poison_t* latent_poison = nullptr;
  latent_poison_injection_t* latent_poison_injection = nullptr;
  timespan_t wilderness_survival_reduction;
  struct {
    double chance = 0;
    proc_t* proc;
  } rylakstalkers_strikes;

  melee_focus_spender_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
    hunter_melee_attack_t( n, p, s ),
    internal_bleeding( p ),
    wilderness_survival_reduction( p -> azerite.wilderness_survival.spell() -> effectN( 1 ).time_value() )
  {
    if ( p -> legendary.latent_poison_injectors.ok() )
      latent_poison_injection = p -> get_background_action<latent_poison_injection_t>( "latent_poison_injection" );

    if ( p -> legendary.rylakstalkers_strikes.ok() )
    {
      rylakstalkers_strikes.chance = p -> legendary.rylakstalkers_strikes -> proc_chance();
      rylakstalkers_strikes.proc = p -> get_proc( "Rylakstalker's Confounding Strikes" );
    }

    if ( p -> azerite.latent_poison.ok() )
      latent_poison = p -> get_background_action<latent_poison_t>( "latent_poison" );
  }

  void execute() override
  {
    hunter_melee_attack_t::execute();

    p() -> buffs.vipers_venom -> trigger();

    if ( p() -> buffs.coordinated_assault -> check() )
    {
      if ( p()->buffs.coordinated_assault_vision->check() )
      {
        p()->buffs.blur_of_talons_vision->trigger();
      }
      else
      {
        if ( p()->buffs.blur_of_talons_vision->check() )
        {
          p()->buffs.blur_of_talons->trigger( p()->buffs.blur_of_talons_vision->check() + 1 );
          p()->buffs.blur_of_talons_vision->expire();
        }
        else
        {
          p()->buffs.blur_of_talons->trigger();
        }
      }
    }

    p() -> buffs.primeval_intuition -> trigger();

    p() -> trigger_birds_of_prey( target );

    if ( wilderness_survival_reduction != 0_ms )
      p() -> cooldowns.wildfire_bomb -> adjust( -wilderness_survival_reduction );

    p() -> buffs.butchers_bone_fragments -> trigger();

    if ( rng().roll( rylakstalkers_strikes.chance ) )
    {
      p() -> cooldowns.wildfire_bomb -> reset( true );
      rylakstalkers_strikes.proc -> occur();
    }
  }

  void impact( action_state_t* s ) override
  {
    hunter_melee_attack_t::impact( s );

    internal_bleeding.trigger( s );

    if ( latent_poison )
      latent_poison -> trigger( s -> target );

    if ( latent_poison_injection )
      latent_poison_injection -> trigger( s -> target );
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
    base_dd_adder += p -> azerite.wilderness_survival.value( 2 );

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
  struct flanking_strike_damage_t final : hunter_melee_attack_t
  {
    flanking_strike_damage_t( util::string_view n, hunter_t* p ):
      hunter_melee_attack_t( n, p, p -> find_spell( 269752 ) )
    {
      background = true;
      dual = true;
    }
  };
  flanking_strike_damage_t* damage;

  flanking_strike_t( hunter_t* p, util::string_view options_str ):
    hunter_melee_attack_t( "flanking_strike", p, p -> talents.flanking_strike ),
    damage( p -> get_background_action<flanking_strike_damage_t>( "flanking_strike_damage" ) )
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

    p() -> buffs.flame_infusion -> trigger();
  }

  void impact( action_state_t* s ) override
  {
    hunter_melee_attack_t::impact( s );

    p() -> trigger_birds_of_prey( s -> target );
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
    base_dd_adder += p -> azerite.wilderness_survival.value( 3 );

    background = p -> talents.mongoose_bite.ok();
  }

  void execute() override
  {
    melee_focus_spender_t::execute();

    p() -> buffs.tip_of_the_spear -> expire();
  }

  double action_multiplier() const override
  {
    double am = melee_focus_spender_t::action_multiplier();

    am *= 1 + p() -> buffs.tip_of_the_spear -> stack_value();

    return am;
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

    update_flags &= ~STATE_HASTE;

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

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }

  void assess_damage( result_amount_type type, action_state_t* s ) override
  {
    hunter_ranged_attack_t::assess_damage( type, s );

    if ( s -> result_amount > 0 && p() -> legendary.latent_poison_injectors.ok() )
      td( s -> target ) -> debuffs.latent_poison_injection -> trigger();

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

  struct chakrams_damage_t final : public hunter_ranged_attack_t
  {
    chakrams_damage_t( util::string_view n, hunter_t* p ):
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
  chakrams_damage_t* damage = nullptr;

  chakrams_t( hunter_t* p, util::string_view options_str ):
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
  interrupt_base_t( util::string_view n, hunter_t* p, const spell_data_t* s ):
    hunter_spell_t( n, p, s )
  {
    may_miss = may_block = may_dodge = may_parry = false;
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
    {
      travel_speed = 0;
    }
  };

  a_murder_of_crows_t( hunter_t* p, util::string_view options_str ) :
    hunter_spell_t( "a_murder_of_crows", p, p -> talents.a_murder_of_crows )
  {
    parse_options( options_str );

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
  bool opt_disabled;
  pet_t* pet;

  summon_pet_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "summon_pet", p, p -> find_spell( 883 ) ),
    opt_disabled( false ), pet( nullptr )
  {
    add_option( opt_obsoleted( "name" ) );
    parse_options( options_str );

    harmful = may_hit = false;
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

// Tar Trap =====================================================================

struct tar_trap_t : public hunter_spell_t
{
  tar_trap_t( hunter_t* p, util::string_view options_str ) :
    hunter_spell_t( "tar_trap", p, p -> find_class_spell( "Tar Trap" ) )
  {
    parse_options( options_str );

    harmful = may_miss = false;
    cooldown -> duration = data().cooldown();
  }

  void impact( action_state_t* s ) override
  {
    hunter_spell_t::impact( s );

    p() -> trigger_nessingwarys_apparatus( this );

    p() -> var.tar_trap_aoe = make_event<events::tar_trap_aoe_t>( *p() -> sim, p(), s -> target, 30_s );
  }
};

// Freezing Trap =====================================================================

struct freezing_trap_t : public hunter_spell_t
{
  freezing_trap_t( hunter_t* p, util::string_view options_str ) :
    hunter_spell_t( "freezing_trap", p, p -> find_class_spell( "Freezing Trap" ) )
  {
    parse_options( options_str );

    harmful = may_miss = false;
    cooldown -> duration = data().cooldown();
  }

  void impact( action_state_t* s ) override
  {
    hunter_spell_t::impact( s );

    p() -> trigger_nessingwarys_apparatus( this );
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
      aoe = -1;
      radius = p -> find_class_spell( "Tar Trap" ) -> effectN( 1 ).trigger() -> effectN( 1 ).base_value();
    }
  };

  soulforge_embers_t* soulforge_embers = nullptr;

  flare_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "flare", p, p -> find_class_spell( "Flare" ) )
  {
    parse_options( options_str );

    harmful = may_hit = may_miss = false;

    if ( p -> legendary.soulforge_embers.ok() )
      soulforge_embers = p -> get_background_action<soulforge_embers_t>( "soulforge_embers" );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    if ( p() -> var.tar_trap_aoe )
    {
      make_event<ground_aoe_event_t>( *sim, player, ground_aoe_params_t()
        .target( execute_state -> target )
        .x( p() -> var.tar_trap_aoe -> x_position )
        .y( p() -> var.tar_trap_aoe -> y_position )
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
  struct {
    real_ppm_t* rppm = nullptr;
    proc_t* proc;
  } dire_consequences;
  struct {
    double chance = 0;
    proc_t* proc;
  } dire_command;

  kill_command_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "kill_command", p, p -> specs.kill_command )
  {
    parse_options( options_str );

    if ( p -> find_rank_spell( "Kill Command", "Rank 2" ) -> ok() )
    {
      flankers_advantage.chance = data().effectN( 2 ).percent();
      flankers_advantage.proc = p -> get_proc( "flankers_advantage" );
    }

    if ( p -> legendary.dire_command.ok() )
    {
      // assume it's a flat % without any bs /shrug
      dire_command.chance = p -> legendary.dire_command -> effectN( 1 ).percent();
      dire_command.proc = p -> get_proc( "Dire Command" );
    }

    if ( p -> azerite.dire_consequences.ok() )
    {
      dire_consequences.rppm = p -> get_rppm( "dire_consequences", 1, 1, RPPM_ATTACK_SPEED );
      if ( p -> specialization() == HUNTER_BEAST_MASTERY )
        dire_consequences.rppm -> set_modifier( .65 );
      dire_consequences.proc = p -> get_proc( "Dire Consequences" );
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

    if ( rng().roll( dire_command.chance ) )
    {
      p() -> pets.dc_dire_beast.spawn( pets::dire_beast_duration( p() ).first );
      dire_command.proc -> occur();
    }

    p() -> buffs.flamewakers_cobra_sting -> up(); // benefit tracking
    p() -> buffs.flamewakers_cobra_sting -> decrement();

    if ( dire_consequences.rppm && dire_consequences.rppm -> trigger() )
    {
      p() -> pets.dc_dire_beast.spawn( pets::dire_beast_duration( p() ).first );
      dire_consequences.proc -> occur();
    }
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
      return make_fn_expr( expression_str, [ this ] () {
          if ( auto pet = p() -> pets.main ) {
            if ( auto td = pet -> find_target_data( target ) )
              return td -> dots.bloodseeker -> remains();
          }
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

    harmful = may_hit = false;
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

    harmful = may_hit = false;
    dot_duration = 0_ms;

    cooldown->duration *= 1 + azerite::vision_of_perfection_cdr( p->azerite_essence.vision_of_perfection );

    precast_time = clamp( precast_time, 0_ms, data().duration() );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.aspect_of_the_wild -> expire();
    trigger_buff( p() -> buffs.aspect_of_the_wild, precast_time );

    p()->buffs.primal_instincts->expire();
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

    tick_action = new stampede_tick_t( p );
  }
};

// Bloodshed ================================================================

struct bloodshed_t : hunter_spell_t
{
  bloodshed_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "bloodshed", p, p -> talents.bloodshed )
  {
    parse_options( options_str );
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
    harmful = may_hit = false;

    cooldown->duration *= 1 + azerite::vision_of_perfection_cdr( p->azerite_essence.vision_of_perfection );

    precast_time = clamp( precast_time, 0_ms, data().duration() );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p()->buffs.trueshot->expire();

    trigger_buff( p() -> buffs.trueshot, precast_time );
    trigger_buff( p() -> buffs.unerring_vision_driver, precast_time );

    adjust_precast_cooldown( precast_time );
  }
};

// Hunter's Mark ============================================================

struct hunters_mark_t: public hunter_spell_t
{
  hunters_mark_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "hunters_mark", p, p -> specs.hunters_mark )
  {
    parse_options( options_str );

    harmful = may_hit = false;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    if ( p() -> var.current_hunters_mark_target != nullptr )
      td( p() -> var.current_hunters_mark_target ) -> debuffs.hunters_mark -> expire();

    p() -> var.current_hunters_mark_target = target;
    td( target ) -> debuffs.hunters_mark -> trigger();
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
  };

  damage_t* damage;

  volley_t( hunter_t* p, util::string_view options_str ):
    hunter_spell_t( "volley", p, p -> talents.volley ),
    damage( p -> get_background_action<damage_t>( "volley_damage" ) )
  {
    parse_options( options_str );

    // disable automatic generation of the dot from spell data
    dot_duration = 0_ms;

    may_miss = may_hit = false;
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

    harmful = may_hit = false;

    cooldown->duration *= 1 + azerite::vision_of_perfection_cdr( p->azerite_essence.vision_of_perfection );
  }

  void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.coordinated_assault->expire();
    p() -> buffs.coordinated_assault_vision->expire();
    p() -> buffs.coordinated_assault -> trigger();
  }
};

// Steel Trap =======================================================================

struct steel_trap_t: public hunter_spell_t
{
  struct steel_trap_impact_t final : public hunter_spell_t
  {
    steel_trap_impact_t( util::string_view n, hunter_t* p ):
      hunter_spell_t( n, p, p -> find_spell( 162487 ) )
    {
      background = true;
      dual = true;
    }

    void execute() override
    {
      hunter_spell_t::execute();

      p() -> trigger_nessingwarys_apparatus( this );
    }
  };

  steel_trap_t( hunter_t* p, util::string_view options_str ):
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
  struct wildfire_cluster_t final : hunter_spell_t
  {
    wildfire_cluster_t( util::string_view n, hunter_t* p ):
      hunter_spell_t( n, p, p -> find_spell( 336899 ) )
    {
      aoe = -1;
    }
  };

  struct azerite_wildfire_cluster_t final : public hunter_spell_t
  {
    azerite_wildfire_cluster_t( util::string_view n, hunter_t* p ):
      hunter_spell_t( n, p, p -> find_spell( 272745 ) )
    {
      aoe = -1;
      base_dd_min = base_dd_max = p -> azerite.wildfire_cluster.value( 1 );
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
    else if ( p -> azerite.wildfire_cluster.ok() )
    {
      wildfire_cluster = p -> get_background_action<azerite_wildfire_cluster_t>( "wildfire_cluster" );
      add_child( wildfire_cluster );
    }
  }

  void execute() override
  {
    current_bomb = bombs[ p() -> var.next_wi_bomb ];

    hunter_spell_t::execute();

    if ( p() -> talents.wildfire_infusion.ok() )
    {
      // assume that we can't get 2 same bombs in a row
      int slot = rng().range( 2 );
      if ( slot == p() -> var.next_wi_bomb )
        slot += 2 - p() -> var.next_wi_bomb;
      p() -> var.next_wi_bomb = static_cast<wildfire_infusion_e>( slot );
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

    harmful = may_hit = false;
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
  debuffs.hunters_mark =
    make_buff( *this, "hunters_mark", p -> specs.hunters_mark )
      -> set_default_value_from_effect( 1 );

  debuffs.latent_poison =
    make_buff( *this, "latent_poison", p -> find_spell( 273286 ) )
      -> set_default_value( p -> azerite.latent_poison.value( 1 ) )
      -> set_trigger_spell( p -> azerite.latent_poison );

  debuffs.latent_poison_injection =
    make_buff( *this, "latent_poison_injection", p -> legendary.latent_poison_injectors -> effectN( 1 ).trigger() )
      -> set_trigger_spell( p -> legendary.latent_poison_injectors );

  debuffs.steady_aim =
    make_buff( *this, "steady_aim", p -> find_spell( 277959 ) )
      -> set_default_value( p -> azerite.steady_aim.value( 1 ) )
      -> set_trigger_spell( p -> azerite.steady_aim );

  dots.serpent_sting = target -> get_dot( "serpent_sting", p );
  dots.a_murder_of_crows = target -> get_dot( "a_murder_of_crows", p );
  dots.pheromone_bomb = target -> get_dot( "pheromone_bomb", p );
  dots.shrapnel_bomb = target -> get_dot( "shrapnel_bomb", p );

  target -> callbacks_on_demise.emplace_back(std::bind( &hunter_td_t::target_demise, this ) );
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

  if ( debuffs.hunters_mark -> check() )
  {
    p -> var.current_hunters_mark_target = nullptr;
  }

  if ( p -> talents.terms_of_engagement.ok() && damaged )
  {
    p -> sim -> print_debug( "{} harpoon cooldown reset on damaged target death.", p -> name() );
    p -> cooldowns.harpoon -> reset( true );
  }

  damaged = false;
}

void hunter_t::vision_of_perfection_proc()
{
  switch ( specialization() )
  {
  case HUNTER_BEAST_MASTERY:
  {
    if ( azerite.primal_instincts.ok() )
    {
      timespan_t recharge = cooldown_t::cooldown_duration( cooldowns.barbed_shot ) * azerite_essence.vision_of_perfection_major_mult;
      cooldowns.barbed_shot->adjust( -recharge );
    }

    timespan_t dur = buffs.aspect_of_the_wild->buff_duration() * azerite_essence.vision_of_perfection_major_mult;
    if ( buffs.aspect_of_the_wild->check() )
    {
      buffs.aspect_of_the_wild->extend_duration( this, dur );
      buffs.primal_instincts->extend_duration( this, dur );
    }
    else
    {
      buffs.aspect_of_the_wild->trigger( dur );
      buffs.primal_instincts->trigger( dur );
    }
    break;
  }
  case HUNTER_MARKSMANSHIP:
  {
    timespan_t ts_dur = buffs.trueshot->buff_duration() * azerite_essence.vision_of_perfection_major_mult;
    timespan_t uv_dur = buffs.unerring_vision_driver->buff_duration() * azerite_essence.vision_of_perfection_major_mult;

    if ( buffs.trueshot->check() )
    {
      buffs.trueshot->extend_duration( this, ts_dur );
      if ( buffs.unerring_vision_driver->check() )
        buffs.unerring_vision_driver->extend_duration( this, uv_dur );
    }
    else
    {
      buffs.trueshot->trigger( ts_dur );
      buffs.unerring_vision_driver->trigger( uv_dur );
    }
    break;
  }
  case HUNTER_SURVIVAL:
  {
    // the vision proc on its own starts at 35% effectiveness, upgraded to 100% if refreshed by the active cooldown
    timespan_t dur = buffs.coordinated_assault->buff_duration() * azerite_essence.vision_of_perfection_major_mult;
    if ( buffs.coordinated_assault->check() )
    {
      buffs.coordinated_assault->extend_duration( this, dur );
      if ( buffs.coordinated_assault_vision->check() )
      {
        buffs.coordinated_assault_vision->extend_duration( this, dur );
      }
    }
    else
    {
      buffs.coordinated_assault->trigger( dur );
      buffs.coordinated_assault_vision->trigger( dur );
    }
    break;
  }
  default:
    break;
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
  if ( splits.size() == 1 && splits[ 0 ] == "ca_execute" )
  {
    if ( !talents.careful_aim.ok() )
      return expr_t::create_constant( "ca_execute", false );

    return make_fn_expr( "ca_execute",
      [ &action,
        high_pct = talents.careful_aim->effectN( 1 ).base_value(),
        low_pct  = talents.careful_aim->effectN( 2 ).base_value() ]
      {
        const double target_health_pct = action.target->health_percentage();
        return target_health_pct > high_pct || target_health_pct < low_pct;
      } );
  }

  return player_t::create_action_expression( action, expression_str );
}

std::unique_ptr<expr_t> hunter_t::create_expression( util::string_view expression_str )
{
  auto splits = util::string_split<util::string_view>( expression_str, "." );

  if ( splits.size() == 3 && splits[ 0 ] == "cooldown")
  {
    if ( splits[ 2 ] == "remains_guess" )
    {
      if ( cooldown_t* cooldown = get_cooldown( splits[ 1 ] ) )
      {
        return make_fn_expr( expression_str,
          [ cooldown ] {
            if ( cooldown -> remains() == cooldown -> duration )
              return cooldown -> duration;

            if ( cooldown -> up() )
              return 0_ms;

            double reduction = ( cooldown -> sim.current_time() - cooldown -> last_start ) /
                               ( cooldown -> duration - cooldown -> remains() );
            return cooldown -> remains() * reduction;
          } );
      }
    }
    else if ( splits[ 2 ] == "duration_guess" )
    {
      if ( cooldown_t* cooldown = get_cooldown( splits[ 1 ] ) )
      {
        return make_fn_expr( expression_str,
          [ cooldown ] {
            if ( cooldown -> last_charged == 0_ms || cooldown -> remains() == cooldown -> duration )
              return cooldown -> duration;

            if ( cooldown -> up() )
              return ( cooldown -> last_charged - cooldown -> last_start );

            double reduction = ( cooldown -> sim.current_time() - cooldown -> last_start ) /
                               ( cooldown -> duration - cooldown -> remains() );
            return cooldown -> duration * reduction;
          } );
      }
    }
  }
  else if ( splits.size() == 2 && splits[ 0 ] == "next_wi_bomb" )
  {
    if ( splits[ 1 ] == "shrapnel" )
      return make_fn_expr( expression_str, [ this ] { return talents.wildfire_infusion.ok() && var.next_wi_bomb == WILDFIRE_INFUSION_SHRAPNEL; } );
    if ( splits[ 1 ] == "pheromone" )
      return make_fn_expr( expression_str, [ this ] { return var.next_wi_bomb == WILDFIRE_INFUSION_PHEROMONE; } );
    if ( splits[ 1 ] == "volatile" )
      return make_fn_expr( expression_str, [ this ] { return var.next_wi_bomb == WILDFIRE_INFUSION_VOLATILE; } );
  }
  else if ( splits.size() == 2 && splits[ 0 ] == "tar_trap" )
  {
    if ( splits[ 1 ] == "up" )
      return make_fn_expr( expression_str, [ this ] { return var.tar_trap_aoe != nullptr; } );

    if ( splits[ 1 ] == "remains" )
    {
      return make_fn_expr( expression_str,
        [ this ]() -> timespan_t {
          if ( var.tar_trap_aoe == nullptr )
            return 0_ms;
          return var.tar_trap_aoe -> remains();
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
  if ( name == "chimaera_shot"         ) return new          chimaera_shot_t( this, options_str );
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
  if ( name == "hunters_mark"          ) return new           hunters_mark_t( this, options_str );
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

  specs.hunters_mark         = find_class_spell( "Hunter's Mark" );
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
  legendary.nessingwarys_apparatus   = find_runeforge_legendary( "Nessingwary's Trapping Apparatus" );
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

  azerite_essence.memory_of_lucid_dreams = find_azerite_essence( "Memory of Lucid Dreams" );
  azerite_essence.memory_of_lucid_dreams_major_mult = find_spell( 298357 ) -> effectN( 1 ).percent();
  azerite_essence.memory_of_lucid_dreams_minor_mult =
    azerite_essence.memory_of_lucid_dreams.spell( 1u, essence_type::MINOR ) -> effectN( 1 ).percent();

  azerite_essence.vision_of_perfection = find_azerite_essence( "Vision of Perfection" );
  azerite_essence.vision_of_perfection_major_mult =
    azerite_essence.vision_of_perfection.spell( 1u )->effectN( 1 ).percent() +
    azerite_essence.vision_of_perfection.spell( 2u, essence_spell::UPGRADE )->effectN( 1 ).percent();
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
  if ( azerite.primeval_intuition.ok() )
    resources.base[RESOURCE_FOCUS] += find_spell( 288571 ) -> effectN( 2 ).base_value();
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

  // Beast Mastery

  buffs.aspect_of_the_wild =
    make_buff( this, "aspect_of_the_wild", specs.aspect_of_the_wild )
      -> set_cooldown( 0_ms )
      -> set_activated( true )
      -> set_default_value_from_effect( 1 )
      -> set_tick_callback( [ this ]( buff_t *b, int, timespan_t ){
          resource_gain( RESOURCE_FOCUS, b -> data().effectN( 2 ).resource( RESOURCE_FOCUS ), gains.aspect_of_the_wild );
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
            resource_gain( RESOURCE_FOCUS, b -> default_value, gains.barbed_shot );
          } );
  }

  buffs.dire_beast =
    make_buff( this, "dire_beast", find_spell( 120679 ) -> effectN( 2 ).trigger() )
      -> set_default_value_from_effect( 1 )
      -> add_invalidate( CACHE_HASTE );

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

  buffs.precise_shots =
    make_buff( this, "precise_shots", find_spell( 260242 ) )
      -> set_default_value_from_effect( 1 )
      -> apply_affecting_conduit( conduits.powerful_precision );

  buffs.steady_focus =
    make_buff( this, "steady_focus", find_spell( 193534 ) )
      -> set_default_value_from_effect( 1 )
      -> set_max_stack( 1 )
      -> add_invalidate( CACHE_HASTE )
      -> set_trigger_spell( talents.steady_focus );

  buffs.streamline =
    make_buff( this, "streamline", find_spell( 342076 ) )
      -> set_default_value_from_effect( 1 )
      -> set_chance( talents.streamline.ok() );

  buffs.trick_shots =
    make_buff( this, "trick_shots", find_spell( 257622 ) )
      -> set_chance( specs.trick_shots.ok() )
      -> set_stack_change_callback( [this]( buff_t*, int old, int cur ) {
          if ( cur == 0 )
            buffs.secrets_of_the_vigil -> expire();
          else if ( old == 0 )
            buffs.secrets_of_the_vigil -> trigger();
        } );

  buffs.trueshot =
    make_buff( this, "trueshot", specs.trueshot )
      -> set_cooldown( 0_ms )
      -> set_activated( true )
      -> set_default_value_from_effect( 4 )
      -> set_stack_change_callback( [this]( buff_t*, int old, int cur ) {
          cooldowns.aimed_shot -> adjust_recharge_multiplier();
          cooldowns.rapid_fire -> adjust_recharge_multiplier();
          if ( cur == 0 )
          {
            buffs.unerring_vision_driver -> expire();
            buffs.unerring_vision -> expire();
            buffs.eagletalons_true_focus -> expire();
          }
          else if ( old == 0 )
          {
            buffs.eagletalons_true_focus -> trigger();
          }
        } )
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

  buffs.coordinated_assault_vision =
    make_buff( this, "coordinated_assault_vision", specs.coordinated_assault )
      -> set_cooldown( 0_ms )
      -> set_default_value_from_effect( 1 )
      -> set_quiet( true );

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
      -> set_default_value( conduits.empowered_release.percent() )
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

  buffs.secrets_of_the_vigil =
    make_buff( this, "secrets_of_the_unblinking_vigil", legendary.secrets_of_the_vigil -> effectN( 1 ).trigger() )
      -> set_trigger_spell( legendary.secrets_of_the_vigil );

  // Azerite

  buffs.blur_of_talons =
    make_buff<stat_buff_t>( this, "blur_of_talons", find_spell( 277969 ) )
      -> add_stat( STAT_AGILITY, azerite.blur_of_talons.value( 1 ) )
      -> add_stat( STAT_SPEED_RATING, azerite.blur_of_talons.value( 2 ) )
      -> set_trigger_spell( azerite.blur_of_talons );

  buffs.blur_of_talons_vision =
    make_buff<stat_buff_t>( this, "blur_of_talons_vision", find_spell( 277969 ) )
      -> add_stat( STAT_AGILITY, azerite.blur_of_talons.value( 1 ) * azerite_essence.vision_of_perfection_major_mult )
      -> add_stat( STAT_SPEED_RATING, azerite.blur_of_talons.value( 2 ) * azerite_essence.vision_of_perfection_major_mult )
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
      -> set_tick_zero( true )
      -> set_tick_callback( [ this ]( buff_t*, int, timespan_t ) { buffs.unerring_vision -> trigger(); } )
      -> set_trigger_spell( azerite.unerring_vision )
      -> set_stack_change_callback( [ this ]( buff_t*, int, int cur ) {
          if ( cur == 0 && buffs.unerring_vision->check() <= buffs.unerring_vision->max_stack() )
            buffs.unerring_vision->trigger();
        } );

  buffs.unerring_vision =
    make_buff<stat_buff_t>( this, "unerring_vision", find_spell( 274447 ) )
      -> add_stat( STAT_CRIT_RATING, azerite.unerring_vision.value( 1 ) );

  player_t::buffs.memory_of_lucid_dreams -> set_affects_regen( true );
}

// hunter_t::init_gains =====================================================

void hunter_t::init_gains()
{
  player_t::init_gains();

  gains.aspect_of_the_wild     = get_gain( "aspect_of_the_wild" );
  gains.barbed_shot            = get_gain( "barbed_shot" );
  gains.memory_of_lucid_dreams_major = get_gain( "Lucid Dreams (Major)" );
  gains.memory_of_lucid_dreams_minor = get_gain( "Lucid Dreams (Minor)" );
  gains.nessingwarys_apparatus = get_gain( "Nessingwary's Trapping Apparatus" );
  gains.reversal_of_fortune    = get_gain( "Reversal of Fortune" );
  gains.terms_of_engagement    = get_gain( "terms_of_engagement" );
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

// hunter_t::default_potion =================================================

std::string hunter_t::default_potion() const
{
  return ( true_level >  110 ) ? "unbridled_fury" :
         ( true_level >= 100 ) ? "prolonged_power" :
         ( true_level >= 90  ) ? "draenic_agility" :
         ( true_level >= 85  ) ? "virmens_bite":
         "disabled";
}

// hunter_t::default_flask ==================================================

std::string hunter_t::default_flask() const
{
  return ( true_level >  110 ) ? "greater_flask_of_the_currents" :
         ( true_level >  100 ) ? "seventh_demon" :
         ( true_level >= 90  ) ? "greater_draenic_agility_flask" :
         ( true_level >= 85  ) ? "spring_blossoms" :
         ( true_level >= 80  ) ? "winds" :
         "disabled";
}

// hunter_t::default_food ===================================================

std::string hunter_t::default_food() const
{

  std::string bfa_food = ( specialization() == HUNTER_SURVIVAL ) ? "baked_port_tato" :
    ( specialization() == HUNTER_MARKSMANSHIP ) ? "abyssalfried_rissole" : "mechdowels_big_mech";

  return ( true_level >  110 ) ? bfa_food :
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

    action_priority_list_t* precombat = get_action_priority_list( "precombat" );

    // Flask, Rune, Food
    precombat -> add_action( "flask" );
    precombat -> add_action( "augmentation" );
    precombat -> add_action( "food" );

    if ( specialization() != HUNTER_MARKSMANSHIP )
      precombat -> add_action( "summon_pet" );

    precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

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

// Beastmastery Action List =============================================================

void hunter_t::apl_bm()
{
  if ( true_level > 50 )
  {
    action_priority_list_t* default_list = get_action_priority_list( "default" );
    action_priority_list_t* precombat    = get_action_priority_list( "precombat" );
    action_priority_list_t* cds          = get_action_priority_list( "cds" );
    action_priority_list_t* st           = get_action_priority_list( "st" );
    action_priority_list_t* cleave       = get_action_priority_list( "cleave" );

    precombat -> add_action( "hunters_mark" );
    precombat -> add_action( "tar_trap,if=runeforge.soulforge_embers.equipped" );
    precombat -> add_action( "aspect_of_the_wild,precast_time=1.3" );
    precombat -> add_action( "wild_spirits" );
    precombat -> add_action( "bestial_wrath,precast_time=1.5,if=!talent.scent_of_blood.enabled&!runeforge.soulforge_embers.equipped" );
    precombat -> add_action( "potion,dynamic_prepot=1" );

    default_list -> add_action( "auto_shot" );
    default_list -> add_action( "use_items,if=prev_gcd.1.aspect_of_the_wild|target.time_to_die<20" );
    default_list -> add_action( "call_action_list,name=cds" );
    default_list -> add_action( "call_action_list,name=st,if=active_enemies<2" );
    default_list -> add_action( "call_action_list,name=cleave,if=active_enemies>1" );

    cds -> add_action( "ancestral_call,if=cooldown.bestial_wrath.remains>30" );
    cds -> add_action( "fireblood,if=cooldown.bestial_wrath.remains>30" );
    cds -> add_action( "berserking,if=buff.aspect_of_the_wild.up&(target.time_to_die>cooldown.berserking.duration+duration|(target.health.pct<35|!talent.killer_instinct.enabled))|target.time_to_die<13" );
    cds -> add_action( "blood_fury,if=buff.aspect_of_the_wild.up&(target.time_to_die>cooldown.blood_fury.duration+duration|(target.health.pct<35|!talent.killer_instinct.enabled))|target.time_to_die<16" );
    cds -> add_action( "lights_judgment,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains>gcd.max|!pet.main.buff.frenzy.up" );
    cds -> add_action( "potion,if=buff.bestial_wrath.up&buff.aspect_of_the_wild.up&target.health.pct<35|((consumable.potion_of_unbridled_fury|consumable.unbridled_fury)&target.time_to_die<61|target.time_to_die<26)" );

    cleave -> add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd" );
    cleave -> add_action( "multishot,if=gcd-pet.main.buff.beast_cleave.remains>0.25" );
    cleave -> add_action( "tar_trap,if=runeforge.soulforge_embers.equipped&tar_trap.remains<gcd&cooldown.flare.remains<gcd" );
    cleave -> add_action( "flare,if=tar_trap.up" );
    cleave -> add_action( "death_chakram,if=focus+cast_regen<focus.max" );
    cleave -> add_action( "wild_spirits" );
    cleave -> add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=full_recharge_time<gcd&cooldown.bestial_wrath.remains|cooldown.bestial_wrath.remains<12+gcd&talent.scent_of_blood.enabled" );
    cleave -> add_action( "aspect_of_the_wild" );
    cleave -> add_action( "bestial_wrath" );
    cleave -> add_action( "resonating_arrow" );
    cleave -> add_action( "stampede,if=buff.aspect_of_the_wild.up|target.time_to_die<15" );
    cleave -> add_action( "flayed_shot" );
    cleave -> add_action( "kill_shot" );
    cleave -> add_action( "chimaera_shot" );
    cleave -> add_action( "bloodshed" );
    cleave -> add_action( "a_murder_of_crows" );
    cleave -> add_action( "barrage,if=pet.main.buff.frenzy.remains>execute_time" );
    cleave -> add_action( "kill_command,if=focus>cost+action.multishot.cost" );
    cleave -> add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
    cleave -> add_action( "dire_beast" );
    cleave -> add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=target.time_to_die<9" );
    cleave -> add_action( "arcane_shot,if=focus.time_to_max<gcd*2" );

    st -> add_action( "barbed_shot,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd|full_recharge_time<gcd&cooldown.bestial_wrath.remains|cooldown.bestial_wrath.remains<12+gcd&talent.scent_of_blood.enabled" );
    st -> add_action( "tar_trap,if=runeforge.soulforge_embers.equipped&tar_trap.remains<gcd&cooldown.flare.remains<gcd" );
    st -> add_action( "flare,if=tar_trap.up" );
    st -> add_action( "wild_spirits" );
    st -> add_action( "kill_shot" );
    st -> add_action( "flayed_shot" );
    st -> add_action( "death_chakram,if=focus+cast_regen<focus.max" );
    st -> add_action( "bloodshed" );
    st -> add_action( "aspect_of_the_wild" );
    st -> add_action( "stampede,if=buff.aspect_of_the_wild.up|target.time_to_die<15" );
    st -> add_action( "a_murder_of_crows" );
    st -> add_action( "bestial_wrath" );
    st -> add_action( "resonating_arrow" );
    st -> add_action( "chimaera_shot" );
    st -> add_action( "kill_command" );
    st -> add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
    st -> add_action( "dire_beast" );
    st -> add_action( "barbed_shot,if=target.time_to_die<9" );
    st -> add_action( "barrage" );
    st -> add_action( "arcane_shot" );
    // st -> add_action( "cobra_shot,if=cooldown.kill_command.remains>1+gcd|focus.time_to_max<cooldown.kill_command.remains|buff.bestial_wrath.up|target.time_to_die<5" );
  }
  else
  {
    action_priority_list_t* default_list = get_action_priority_list( "default" );
    action_priority_list_t* precombat    = get_action_priority_list( "precombat" );
    action_priority_list_t* cds          = get_action_priority_list( "cds" );
    action_priority_list_t* st           = get_action_priority_list( "st" );
    action_priority_list_t* cleave       = get_action_priority_list( "cleave" );

    precombat -> add_action( "use_item,name=azsharas_font_of_power" );
    precombat -> add_action( "worldvein_resonance" );
    precombat -> add_action( "guardian_of_azeroth" );
    precombat -> add_action( "memory_of_lucid_dreams" );
    precombat -> add_action( "use_item,effect_name=cyclotronic_blast,if=!raid_event.invulnerable.exists&(trinket.1.has_cooldown+trinket.2.has_cooldown<2|equipped.variable_intensity_gigavolt_oscillating_reactor)" );
    precombat -> add_action( "focused_azerite_beam,if=!raid_event.invulnerable.exists" );
    precombat -> add_action( this, "Aspect of the Wild", "precast_time=1.3,if=!azerite.primal_instincts.enabled&!essence.essence_of_the_focusing_iris.major&(equipped.azsharas_font_of_power|!equipped.cyclotronic_blast)",
            "Adjusts the duration and cooldown of Aspect of the Wild and Primal Instincts by the duration of an unhasted GCD when they're used precombat. Because Aspect of the Wild reduces GCD by 200ms, this is 1.3 seconds. ");
    precombat -> add_action( this, "Bestial Wrath", "precast_time=1.5,if=azerite.primal_instincts.enabled&!essence.essence_of_the_focusing_iris.major&(equipped.azsharas_font_of_power|!equipped.cyclotronic_blast)",
            "Adjusts the duration and cooldown of Bestial Wrath and Haze of Rage by the duration of an unhasted GCD when they're used precombat." );
    precombat -> add_action( "potion,dynamic_prepot=1" );

    default_list -> add_action( "auto_shot" );
    default_list -> add_action( "use_items,if=prev_gcd.1.aspect_of_the_wild|target.time_to_die<20" );
    default_list -> add_action( "use_item,name=azsharas_font_of_power,if=cooldown.aspect_of_the_wild.remains_guess<15&target.time_to_die>10" );
    default_list -> add_action( "use_item,name=ashvanes_razor_coral,if=debuff.razor_coral_debuff.up&(!equipped.azsharas_font_of_power|trinket.azsharas_font_of_power.cooldown.remains>86|essence.blood_of_the_enemy.major)&(prev_gcd.1.aspect_of_the_wild|!equipped.cyclotronic_blast&buff.aspect_of_the_wild.remains>9)&(!essence.condensed_lifeforce.major|buff.guardian_of_azeroth.up)&(target.health.pct<35|!essence.condensed_lifeforce.major|!talent.killer_instinct.enabled)|(debuff.razor_coral_debuff.down|target.time_to_die<26)&target.time_to_die>(24*(cooldown.cyclotronic_blast.remains+4<target.time_to_die))" );
    default_list -> add_action( "use_item,effect_name=cyclotronic_blast,if=buff.bestial_wrath.down|target.time_to_die<5" );
    default_list -> add_action( "call_action_list,name=cds" );
    default_list -> add_action( "call_action_list,name=st,if=active_enemies<2" );
    default_list -> add_action( "call_action_list,name=cleave,if=active_enemies>1" );

    cds -> add_action( "ancestral_call,if=cooldown.bestial_wrath.remains>30" );
    cds -> add_action( "fireblood,if=cooldown.bestial_wrath.remains>30" );
    cds -> add_action( "berserking,if=buff.aspect_of_the_wild.up&(target.time_to_die>cooldown.berserking.duration+duration|(target.health.pct<35|!talent.killer_instinct.enabled))|target.time_to_die<13" );
    cds -> add_action( "blood_fury,if=buff.aspect_of_the_wild.up&(target.time_to_die>cooldown.blood_fury.duration+duration|(target.health.pct<35|!talent.killer_instinct.enabled))|target.time_to_die<16" );
    cds -> add_action( "lights_judgment,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains>gcd.max|!pet.main.buff.frenzy.up" );
    cds -> add_action( "potion,if=buff.bestial_wrath.up&buff.aspect_of_the_wild.up&target.health.pct<35|((consumable.potion_of_unbridled_fury|consumable.unbridled_fury)&target.time_to_die<61|target.time_to_die<26)" );
    cds -> add_action( "worldvein_resonance,if=(prev_gcd.1.aspect_of_the_wild|cooldown.aspect_of_the_wild.remains<gcd|target.time_to_die<20)|!essence.vision_of_perfection.minor" );
    cds -> add_action( "guardian_of_azeroth,if=cooldown.aspect_of_the_wild.remains<10|target.time_to_die>cooldown+duration|target.time_to_die<30" );
    cds -> add_action( "ripple_in_space" );
    cds -> add_action( "memory_of_lucid_dreams" );
    cds -> add_action( "reaping_flames,if=target.health.pct>80|target.health.pct<=20|target.time_to_pct_20>30" );

    st -> add_action( this, "Kill Shot" );
    st -> add_talent( this, "Bloodshed" );
    st -> add_action( this, "Barbed Shot", "if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<gcd|cooldown.bestial_wrath.remains&(full_recharge_time<gcd|azerite.primal_instincts.enabled&cooldown.aspect_of_the_wild.remains<gcd)" );
    st -> add_action( "concentrated_flame,if=focus+focus.regen*gcd<focus.max&buff.bestial_wrath.down&(!dot.concentrated_flame_burn.remains&!action.concentrated_flame.in_flight)|full_recharge_time<gcd|target.time_to_die<5" );
    st -> add_action( this, "Aspect of the Wild", "if=buff.aspect_of_the_wild.down&(cooldown.barbed_shot.charges<1|!azerite.primal_instincts.enabled)" );
    st -> add_talent( this, "Stampede", "if=buff.aspect_of_the_wild.up&buff.bestial_wrath.up|target.time_to_die<15" );
    st -> add_talent( this, "A Murder of Crows" );
    st -> add_action( "focused_azerite_beam,if=buff.bestial_wrath.down|target.time_to_die<5" );
    st -> add_action( "the_unbound_force,if=buff.reckless_force.up|buff.reckless_force_counter.stack<10|target.time_to_die<5" );
    st -> add_action( this, "Bestial Wrath", "if=talent.one_with_the_pack.enabled&buff.bestial_wrath.remains<gcd|buff.bestial_wrath.down&cooldown.aspect_of_the_wild.remains>15|target.time_to_die<15+gcd" );
    st -> add_action( this, "Barbed Shot", "if=azerite.dance_of_death.rank>1&buff.dance_of_death.remains<gcd" );
    st -> add_action( "blood_of_the_enemy,if=buff.aspect_of_the_wild.remains>10+gcd|target.time_to_die<10+gcd" );
    st -> add_action( this, "Kill Command" );
    st -> add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
    st -> add_talent( this, "Chimaera Shot" );
    st -> add_talent( this, "Dire Beast" );
    st -> add_action( this, "Barbed Shot", "if=talent.one_with_the_pack.enabled&charges_fractional>1.5|charges_fractional>1.8|cooldown.aspect_of_the_wild.remains<pet.main.buff.frenzy.duration-gcd&azerite.primal_instincts.enabled|target.time_to_die<9" );
    st -> add_action( "purifying_blast,if=buff.bestial_wrath.down|target.time_to_die<8" );
    st -> add_talent( this, "Barrage" );
    st -> add_action( this, "Cobra Shot", "if=(focus-cost+focus.regen*(cooldown.kill_command.remains-1)>action.kill_command.cost|cooldown.kill_command.remains>1+gcd&cooldown.bestial_wrath.remains_guess>focus.time_to_max|buff.memory_of_lucid_dreams.up)&cooldown.kill_command.remains>1|target.time_to_die<3" );
    st -> add_action( this, "Barbed Shot", "if=pet.main.buff.frenzy.duration-gcd>full_recharge_time" );

    cleave -> add_action( this, "Barbed Shot", "target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd.max" );
    cleave -> add_action( this, "Multi-Shot", "if=gcd.max-pet.main.buff.beast_cleave.remains>0.25" );
    cleave -> add_action( this, "Barbed Shot", "target_if=min:dot.barbed_shot.remains,if=full_recharge_time<gcd.max&cooldown.bestial_wrath.remains" );
    cleave -> add_action( this, "Aspect of the Wild" );
    cleave -> add_talent( this, "Stampede", "if=buff.aspect_of_the_wild.up&buff.bestial_wrath.up|target.time_to_die<15" );
    cleave -> add_action( this, "Bestial Wrath", "if=cooldown.aspect_of_the_wild.remains_guess>20|talent.one_with_the_pack.enabled|target.time_to_die<15" );
    cleave -> add_talent( this, "Chimaera Shot" );
    cleave -> add_talent( this, "A Murder of Crows" );
    cleave -> add_talent( this, "Barrage" );
    cleave -> add_action( this, "Kill Command", "if=active_enemies<4|!azerite.rapid_reload.enabled" );
    cleave -> add_talent( this, "Dire Beast" );
    cleave -> add_action( this, "Barbed Shot", "target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.down&(charges_fractional>1.8|buff.bestial_wrath.up)|cooldown.aspect_of_the_wild.remains<pet.main.buff.frenzy.duration-gcd&azerite.primal_instincts.enabled|charges_fractional>1.4|target.time_to_die<9" );
    cleave -> add_action( "focused_azerite_beam" );
    cleave -> add_action( "purifying_blast" );
    cleave -> add_action( "concentrated_flame" );
    cleave -> add_action( "blood_of_the_enemy" );
    cleave -> add_action( "the_unbound_force,if=buff.reckless_force.up|buff.reckless_force_counter.stack<10" );
    cleave -> add_action( this, "Multi-Shot", "if=azerite.rapid_reload.enabled&active_enemies>2");
    cleave -> add_action( this, "Cobra Shot", "if=cooldown.kill_command.remains>focus.time_to_max&(active_enemies<3|!azerite.rapid_reload.enabled)" );
  }
}

// Marksman Action List ======================================================================

void hunter_t::apl_mm()
{
  if ( true_level > 50 )
  {
    action_priority_list_t* default_list = get_action_priority_list( "default" );
    action_priority_list_t* precombat    = get_action_priority_list( "precombat" );
    action_priority_list_t* cds          = get_action_priority_list( "cds" );
    action_priority_list_t* st           = get_action_priority_list( "st" );
    action_priority_list_t* trickshots   = get_action_priority_list( "trickshots" );

    precombat -> add_action( "hunters_mark" );
    precombat -> add_action( "tar_trap,if=runeforge.soulforge_embers.equipped" );
    precombat -> add_action( "double_tap,precast_time=10" );
    precombat -> add_action( "wild_spirits" );
    precombat -> add_action( "trueshot,precast_time=1.5,if=active_enemies>2" );
    precombat -> add_action( "potion,dynamic_prepot=1" );
    precombat -> add_action( "aimed_shot,if=active_enemies<3" );

    default_list -> add_action( "auto_shot" );
    default_list -> add_action( "use_items,if=prev_gcd.1.trueshot|!talent.calling_the_shots.enabled|target.time_to_die<20" );
    default_list -> add_action( "call_action_list,name=cds" );
    default_list -> add_action( "call_action_list,name=st,if=active_enemies<3" );
    default_list -> add_action( "call_action_list,name=trickshots,if=active_enemies>2" );

    cds -> add_action( "hunters_mark,if=debuff.hunters_mark.down&!buff.trueshot.up" );
    cds -> add_action( "berserking,if=prev_gcd.1.trueshot&(target.time_to_die>cooldown.berserking.duration+duration|(target.health.pct<20|!talent.careful_aim.enabled))|target.time_to_die<13" );
    cds -> add_action( "blood_fury,if=prev_gcd.1.trueshot&(target.time_to_die>cooldown.blood_fury.duration+duration|(target.health.pct<20|!talent.careful_aim.enabled))|target.time_to_die<16" );
    cds -> add_action( "ancestral_call,if=prev_gcd.1.trueshot&(target.time_to_die>cooldown.ancestral_call.duration+duration|(target.health.pct<20|!talent.careful_aim.enabled))|target.time_to_die<16" );
    cds -> add_action( "fireblood,if=prev_gcd.1.trueshot&(target.time_to_die>cooldown.fireblood.duration+duration|(target.health.pct<20|!talent.careful_aim.enabled))|target.time_to_die<9" );
    cds -> add_action( "lights_judgment,if=buff.trueshot.down" );
    cds -> add_action( "bag_of_tricks,if=buff.trueshot.down" );
    cds -> add_action( "potion,if=buff.trueshot.react&buff.bloodlust.react|prev_gcd.1.trueshot&target.health.pct<20|((consumable.potion_of_unbridled_fury|consumable.unbridled_fury)&target.time_to_die<61|target.time_to_die<26)" );

    trickshots -> add_action( "double_tap,if=cooldown.aimed_shot.up|cooldown.rapid_fire.remains>cooldown.aimed_shot.remains" );
    trickshots -> add_action( "tar_trap,if=runeforge.soulforge_embers.equipped&tar_trap.remains<gcd&cooldown.flare.remains<gcd" );
    trickshots -> add_action( "flare,if=tar_trap.up" );
    trickshots -> add_action( "wild_spirits" );
    trickshots -> add_action( "volley" );
    trickshots -> add_action( "resonating_arrow" );
    trickshots -> add_action( "barrage" );
    trickshots -> add_action( "explosive_shot" );
    trickshots -> add_action( "trueshot,if=cooldown.rapid_fire.remains|focus+action.rapid_fire.cast_regen>focus.max|target.time_to_die<15" );
    trickshots -> add_action( "aimed_shot,if=buff.trick_shots.up&(buff.precise_shots.down|full_recharge_time<cast_time+gcd|buff.trueshot.up)" );
    trickshots -> add_action( "death_chakram,if=focus+cast_regen<focus.max" );
    trickshots -> add_action( "rapid_fire,if=buff.trick_shots.up&buff.double_tap.down" );
    trickshots -> add_action( "multishot,if=buff.trick_shots.down|buff.precise_shots.up|focus-cost+cast_regen>action.aimed_shot.cost" );
    trickshots -> add_action( "kill_shot,if=buff.dead_eye.down" );
    trickshots -> add_action( "a_murder_of_crows" );
    trickshots -> add_action( "flayed_shot" );
    trickshots -> add_action( "serpent_sting,target_if=min:dot.serpent_sting.remains,if=refreshable" );
    trickshots -> add_action( "steady_shot" );

    st -> add_action( "double_tap,if=cooldown.aimed_shot.up|cooldown.rapid_fire.remains>cooldown.aimed_shot.remains" );
    st -> add_action( "tar_trap,if=runeforge.soulforge_embers.equipped&tar_trap.remains<gcd&cooldown.flare.remains<gcd" );
    st -> add_action( "flare,if=tar_trap.up" );
    st -> add_action( "wild_spirits" );
    st -> add_action( "volley,if=active_enemies<2|cooldown.trueshot.remains" );
    st -> add_action( "explosive_shot" );
    st -> add_action( "a_murder_of_crows" );
    st -> add_action( "flayed_shot" );
    st -> add_action( "resonating_arrow" );
    st -> add_action( "death_chakram,if=focus+cast_regen<focus.max" );
    st -> add_action( "trueshot,if=(buff.precise_shots.down|!talent.chimaera_shot.enabled)&cooldown.aimed_shot.remains%3.4<gcd|target.time_to_die<15" );
    st -> add_action( "aimed_shot,if=(full_recharge_time<cast_time+gcd|target.time_to_pct_20<5&talent.dead_eye.enabled|buff.trueshot.up)&(buff.precise_shots.down|!talent.chimaera_shot.enabled)" );
    st -> add_action( "kill_shot,if=buff.dead_eye.down&cooldown.aimed_shot.full_recharge_time%3>gcd" );
    st -> add_action( "rapid_fire,if=buff.double_tap.down&focus+cast_regen<focus.max" );
    st -> add_action( "chimaera_shot,if=buff.precise_shots.up&(buff.trueshot.down|active_enemies>1)&(!talent.dead_eye.enabled|target.time_to_pct_20>5|target.time_to_die<10)" );
    st -> add_action( "serpent_sting,target_if=min:remains,if=refreshable&target.time_to_die>duration" );
    st -> add_action( "barrage,if=active_enemies>1" );
    st -> add_action( "arcane_shot,if=buff.precise_shots.up&buff.trueshot.down&(!talent.dead_eye.enabled|target.time_to_pct_20>5|target.time_to_die<10)" );
    st -> add_action( "aimed_shot,if=buff.precise_shots.down" );
    st -> add_action( "chimaera_shot,if=(focus>cost+action.aimed_shot.cost)&(!talent.dead_eye.enabled|target.time_to_pct_20>5)" );
    st -> add_action( "arcane_shot,if=(focus>cost+action.aimed_shot.cost)&(!talent.dead_eye.enabled|target.time_to_pct_20>5)" );
    st -> add_action( "steady_shot,if=focus+cast_regen<focus.max" );
    st -> add_action( "chimaera_shot" );
    st -> add_action( "arcane_shot" );
  }
  else
  {
    action_priority_list_t* default_list = get_action_priority_list( "default" );
    action_priority_list_t* precombat    = get_action_priority_list( "precombat" );
    action_priority_list_t* cds          = get_action_priority_list( "cds" );
    action_priority_list_t* st           = get_action_priority_list( "st" );
    action_priority_list_t* trickshots   = get_action_priority_list( "trickshots" );

    precombat -> add_talent( this, "Hunter's Mark" );
    precombat -> add_talent( this, "Double Tap", "precast_time=10",
          "Precast this as early as possible to potentially gain another cast during the fight." );
    precombat -> add_action( "use_item,name=azsharas_font_of_power" );
    precombat -> add_action( "worldvein_resonance" );
    precombat -> add_action( "guardian_of_azeroth" );
    precombat -> add_action( "memory_of_lucid_dreams" );
    precombat -> add_action( this, "Trueshot", "precast_time=1.5,if=active_enemies>2" );
    precombat -> add_action( "potion,dynamic_prepot=1" );
    precombat -> add_action( this, "Aimed Shot", "if=active_enemies<3" );

    default_list -> add_action( "auto_shot" );
    default_list -> add_action( "use_item,name=lurkers_insidious_gift,if=cooldown.trueshot.remains_guess<15|target.time_to_die<30" );
    default_list -> add_action( "use_item,name=azsharas_font_of_power,if=(target.time_to_die>cooldown+34|target.health.pct<20|target.time_to_pct_20<15)&cooldown.trueshot.remains_guess<15|target.time_to_die<35" );
    default_list -> add_action( "use_item,name=lustrous_golden_plumage,if=cooldown.trueshot.remains_guess<5|target.time_to_die<20" );
    default_list -> add_action( "use_item,name=galecallers_boon,if=prev_gcd.1.trueshot|!talent.calling_the_shots.enabled|target.time_to_die<10" );
    default_list -> add_action( "use_item,name=ashvanes_razor_coral,if=prev_gcd.1.trueshot&(buff.guardian_of_azeroth.up|!essence.condensed_lifeforce.major&ca_execute)|debuff.razor_coral_debuff.down|target.time_to_die<20" );
    default_list -> add_action( "use_item,name=pocketsized_computation_device,if=!buff.trueshot.up&!essence.blood_of_the_enemy.major|debuff.blood_of_the_enemy.up|target.time_to_die<5" );
    default_list -> add_action( "use_items,if=prev_gcd.1.trueshot|!talent.calling_the_shots.enabled|target.time_to_die<20",
          "Try to line up activated trinkets with Trueshot" );
    default_list -> add_action( "call_action_list,name=cds" );
    default_list -> add_action( "call_action_list,name=st,if=active_enemies<3" );
    default_list -> add_action( "call_action_list,name=trickshots,if=active_enemies>2" );

    cds -> add_action( this, "Hunter's Mark", "if=debuff.hunters_mark.down&!buff.trueshot.up" );
    cds -> add_talent( this, "Double Tap", "if=cooldown.rapid_fire.remains<gcd|cooldown.rapid_fire.remains<cooldown.aimed_shot.remains|target.time_to_die<20" );
    cds -> add_action( "berserking,if=prev_gcd.1.trueshot&(target.time_to_die>cooldown.berserking.duration+duration|(target.health.pct<20|!talent.careful_aim.enabled))|target.time_to_die<13" );
    cds -> add_action( "blood_fury,if=prev_gcd.1.trueshot&(target.time_to_die>cooldown.blood_fury.duration+duration|(target.health.pct<20|!talent.careful_aim.enabled))|target.time_to_die<16" );
    cds -> add_action( "ancestral_call,if=prev_gcd.1.trueshot&(target.time_to_die>cooldown.ancestral_call.duration+duration|(target.health.pct<20|!talent.careful_aim.enabled))|target.time_to_die<16" );
    cds -> add_action( "fireblood,if=prev_gcd.1.trueshot&(target.time_to_die>cooldown.fireblood.duration+duration|(target.health.pct<20|!talent.careful_aim.enabled))|target.time_to_die<9" );
    cds -> add_action( "lights_judgment,if=buff.trueshot.down" );
    cds -> add_action( "bag_of_tricks,if=buff.trueshot.down" );
    cds -> add_action( "reaping_flames,if=buff.trueshot.down&(target.health.pct>80|target.health.pct<=20|target.time_to_pct_20>30)" );
    cds -> add_action( "worldvein_resonance,if=(trinket.azsharas_font_of_power.cooldown.remains>20|!equipped.azsharas_font_of_power|target.time_to_die<trinket.azsharas_font_of_power.cooldown.duration+34&target.health.pct>20)&(cooldown.trueshot.remains_guess<3|(essence.vision_of_perfection.minor&target.time_to_die>cooldown+buff.worldvein_resonance.duration))|target.time_to_die<20" );
    cds -> add_action( "guardian_of_azeroth,if=(ca_execute|target.time_to_die>cooldown+30)&(buff.trueshot.up|cooldown.trueshot.remains<16)|target.time_to_die<31" );
    cds -> add_action( "ripple_in_space,if=cooldown.trueshot.remains<7" );
    cds -> add_action( "memory_of_lucid_dreams,if=!buff.trueshot.up" );
    cds -> add_action( "potion,if=buff.trueshot.react&buff.bloodlust.react|prev_gcd.1.trueshot&target.health.pct<20|((consumable.potion_of_unbridled_fury|consumable.unbridled_fury)&target.time_to_die<61|target.time_to_die<26)" );
    cds -> add_action( this, "Trueshot", "if=buff.trueshot.down&cooldown.rapid_fire.remains|target.time_to_die<15" );

    st -> add_action( this, "Kill Shot" );
    st -> add_talent( this, "Explosive Shot" );
    st -> add_talent( this, "Barrage", "if=active_enemies>1" );
    st -> add_talent( this, "A Murder of Crows" );
    st -> add_talent( this, "Volley" );
    st -> add_talent( this, "Serpent Sting", "if=refreshable&!action.serpent_sting.in_flight" );
    st -> add_action( this, "Rapid Fire", "if=buff.trueshot.down|focus<35|focus<60&!talent.lethal_shots.enabled|buff.in_the_rhythm.remains<execute_time");
    st -> add_action( "blood_of_the_enemy,if=buff.trueshot.up&(buff.unerring_vision.stack>4|!azerite.unerring_vision.enabled)|target.time_to_die<11" );
    st -> add_action( "focused_azerite_beam,if=!buff.trueshot.up|target.time_to_die<5" );
    st -> add_action( this, "Arcane Shot", "if=buff.trueshot.up&!buff.memory_of_lucid_dreams.up" );
    st -> add_talent( this, "Chimaera Shot", "if=buff.trueshot.up&!buff.memory_of_lucid_dreams.up" );
    st -> add_action( this, "Aimed Shot", "if=buff.trueshot.up|(buff.double_tap.down|ca_execute)&buff.precise_shots.down|full_recharge_time<cast_time&cooldown.trueshot.remains" );
    st -> add_action( this, "Arcane Shot", "if=buff.trueshot.up&buff.memory_of_lucid_dreams.up" );
    st -> add_talent( this, "Chimaera Shot", "if=buff.trueshot.up&buff.memory_of_lucid_dreams.up" );
    st -> add_action( "purifying_blast,if=!buff.trueshot.up|target.time_to_die<8" );
    st -> add_action( "concentrated_flame,if=focus+focus.regen*gcd<focus.max&buff.trueshot.down&(!dot.concentrated_flame_burn.remains&!action.concentrated_flame.in_flight)|full_recharge_time<gcd|target.time_to_die<5" );
    st -> add_action( "the_unbound_force,if=buff.reckless_force.up|buff.reckless_force_counter.stack<10|target.time_to_die<5" );
    st -> add_action( this, "Arcane Shot", "if=buff.trueshot.down&(buff.precise_shots.up&(focus>55)|focus>75|target.time_to_die<5)" );
    st -> add_talent( this, "Chimaera Shot", "if=buff.trueshot.down&(buff.precise_shots.up&(focus>55)|focus>75|target.time_to_die<5)" );
    st -> add_action( this, "Steady Shot" );

    trickshots -> add_action( this, "Kill Shot" );
    trickshots -> add_talent( this, "Volley" );
    trickshots -> add_talent( this, "Barrage" );
    trickshots -> add_talent( this, "Explosive Shot" );
    trickshots -> add_action( this, "Aimed Shot", "if=buff.trick_shots.up&ca_execute&buff.double_tap.up");
    trickshots -> add_action( this, "Rapid Fire", "if=buff.trick_shots.up&(azerite.focused_fire.enabled|azerite.in_the_rhythm.rank>1|azerite.surging_shots.enabled|talent.streamline.enabled)" );
    trickshots -> add_action( this, "Aimed Shot", "if=buff.trick_shots.up&(buff.precise_shots.down|cooldown.aimed_shot.full_recharge_time<action.aimed_shot.cast_time|buff.trueshot.up)" );
    trickshots -> add_action( this, "Rapid Fire", "if=buff.trick_shots.up" );
    trickshots -> add_action( this, "Multi-Shot", "if=buff.trick_shots.down|buff.precise_shots.up&!buff.trueshot.up|focus>70" );
    trickshots -> add_action( "focused_azerite_beam" );
    trickshots -> add_action( "purifying_blast" );
    trickshots -> add_action( "concentrated_flame" );
    trickshots -> add_action( "blood_of_the_enemy" );
    trickshots -> add_action( "the_unbound_force,if=buff.reckless_force.up|buff.reckless_force_counter.stack<10" );
    trickshots -> add_talent( this, "A Murder of Crows" );
    trickshots -> add_talent( this, "Serpent Sting", "if=refreshable&!action.serpent_sting.in_flight" );
    trickshots -> add_action( this, "Steady Shot" );
  }
}

// Survival Action List ===================================================================

void hunter_t::apl_surv()
{
  if ( true_level > 50 )
  {
    action_priority_list_t* default_list = get_action_priority_list( "default" );
    action_priority_list_t* precombat    = get_action_priority_list( "precombat" );
    action_priority_list_t* cds          = get_action_priority_list( "cds" );
    action_priority_list_t* st           = get_action_priority_list( "st" );
    action_priority_list_t* cleave       = get_action_priority_list( "cleave" );

    precombat -> add_action( "hunters_mark" );
    precombat -> add_action( "tar_trap,if=runeforge.soulforge_embers.equipped" );
    precombat -> add_action( "steel_trap" );
    precombat -> add_action( "coordinated_assault" );
    precombat -> add_action( "wild_spirits" );
    precombat -> add_action( "potion,dynamic_prepot=1" );

    cds -> add_action( "blood_fury,if=cooldown.coordinated_assault.remains>30" );
    cds -> add_action( "ancestral_call,if=cooldown.coordinated_assault.remains>30" );
    cds -> add_action( "fireblood,if=cooldown.coordinated_assault.remains>30" );
    cds -> add_action( "lights_judgment" );
    cds -> add_action( "berserking,if=cooldown.coordinated_assault.remains>60|time_to_die<13" );
    cds -> add_action( "potion,if=target.time_to_die<26|buff.coordinated_assault.up" );
    cds -> add_action( "aspect_of_the_eagle,if=target.distance>=6" );

    default_list -> add_action( "auto_attack" );
    default_list -> add_action( "use_items" );
    default_list -> add_action( "call_action_list,name=cds" );
    default_list -> add_action( "call_action_list,name=st,if=active_enemies<2" );
    default_list -> add_action( "call_action_list,name=cleave,if=active_enemies>1" );
    default_list -> add_action( "arcane_torrent" );
    default_list -> add_action( "bag_of_tricks" );

    st -> add_action( "harpoon,if=talent.terms_of_engagement.enabled&focus<focus.max" );
    st -> add_action( "kill_command,if=focus+cast_regen<focus.max&buff.tip_of_the_spear.stack<3&!talent.alpha_predator.enabled" );
    st -> add_action( "mongoose_bite,if=dot.shrapnel_bomb.ticking&(dot.internal_bleeding.stack<2|dot.shrapnel_bomb.remains<gcd)|buff.mongoose_fury.up&buff.mongoose_fury.remains<focus%30*gcd|talent.birds_of_prey.enabled&buff.coordinated_assault.up&buff.coordinated_assault.remains<gcd" );
    st -> add_action( "raptor_strike,if=dot.shrapnel_bomb.ticking&(dot.internal_bleeding.stack<2|dot.shrapnel_bomb.remains<gcd)|talent.birds_of_prey.enabled&buff.coordinated_assault.up&buff.coordinated_assault.remains<gcd" );
    st -> add_action( "wild_spirits" );
    st -> add_action( "tar_trap,if=runeforge.soulforge_embers.equipped&tar_trap.remains<gcd&cooldown.flare.remains<gcd" );
    st -> add_action( "flare,if=runeforge.soulforge_embers.equipped&tar_trap.up" );
    st -> add_action( "coordinated_assault" );
    st -> add_action( "resonating_arrow" );
    st -> add_action( "flayed_shot" );
    st -> add_action( "death_chakram,if=focus+cast_regen<focus.max" );
    st -> add_action( "kill_shot" );
    st -> add_action( "flanking_strike,if=focus+cast_regen<focus.max" );
    st -> add_action( "kill_command,if=focus+cast_regen<focus.max&buff.tip_of_the_spear.stack<3&full_recharge_time<2*gcd" );
    st -> add_action( "chakrams" );
    st -> add_action( "a_murder_of_crows" );
    st -> add_action( "wildfire_bomb,if=next_wi_bomb.volatile&dot.serpent_sting.ticking&dot.serpent_sting.refreshable" );
    st -> add_action( "serpent_sting,if=refreshable|buff.vipers_venom.up" );
    st -> add_action( "wildfire_bomb,if=next_wi_bomb.shrapnel&focus>60|next_wi_bomb.pheromone&(focus+cast_regen+action.kill_command.cast_regen*3<focus.max|talent.mongoose_bite.enabled&!buff.mongoose_fury.up)|!dot.wildfire_bomb.ticking&full_recharge_time<gcd" );
    st -> add_action( "kill_command,if=focus+cast_regen<focus.max&buff.tip_of_the_spear.stack<3" );
    st -> add_action( "steel_trap,if=!runeforge.nessingwarys_trapping_apparatus.equipped|focus+cast_regen+25<focus.max" );
    st -> add_action( "tar_trap,if=runeforge.nessingwarys_trapping_apparatus.equipped&focus+cast_regen+25<focus.max" );
    st -> add_action( "freezing_trap,if=runeforge.nessingwarys_trapping_apparatus.equipped&focus+cast_regen+25<focus.max" );
    st -> add_action( "mongoose_bite,if=buff.mongoose_fury.up|focus+action.kill_command.cast_regen>focus.max" );
    st -> add_action( "raptor_strike,if=!next_wi_bomb.shrapnel|buff.tip_of_the_spear.stack>2|focus+action.kill_command.cast_regen>focus.max" );
    st -> add_action( "kill_command,if=buff.tip_of_the_spear.stack<3" );
    st -> add_action( "wildfire_bomb,if=!dot.wildfire_bomb.ticking&!talent.wildfire_infusion.enabled" );

    cleave -> add_action( "harpoon,if=talent.terms_of_engagement.enabled&focus<focus.max" );
    cleave -> add_action( "wild_spirits" );
    cleave -> add_action( "tar_trap,if=runeforge.soulforge_embers.equipped&tar_trap.remains<gcd&cooldown.flare.remains<gcd" );
    cleave -> add_action( "flare,if=runeforge.soulforge_embers.equipped&tar_trap.up" );
    cleave -> add_action( "resonating_arrow" );
    cleave -> add_action( "wildfire_bomb,if=full_recharge_time<gcd" );
    cleave -> add_action( "chakrams" );
    cleave -> add_action( "butchery,if=dot.shrapnel_bomb.ticking&(dot.internal_bleeding.stack<2|dot.shrapnel_bomb.remains<gcd)" );
    cleave -> add_action( "carve,if=dot.shrapnel_bomb.ticking" );
    cleave -> add_action( "death_chakram,if=focus+cast_regen<focus.max" );
    cleave -> add_action( "coordinated_assault" );
    cleave -> add_action( "butchery,if=charges_fractional>2.5" );
    cleave -> add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max&full_recharge_time<gcd" );
    cleave -> add_action( "flanking_strike,if=focus+cast_regen<focus.max" );
    cleave -> add_action( "wildfire_bomb,if=!dot.wildfire_bomb.ticking" );
    cleave -> add_action( "butchery,if=!talent.wildfire_infusion.enabled" );
    cleave -> add_action( "kill_shot" );
    cleave -> add_action( "flayed_shot" );
    cleave -> add_action( "a_murder_of_crows" );
    cleave -> add_action( "steel_trap,if=!runeforge.nessingwarys_trapping_apparatus.equipped|focus+cast_regen+25<focus.max" );
    cleave -> add_action( "serpent_sting,target_if=min:remains,if=refreshable&talent.flanking_strike.enabled|buff.vipers_venom.up" );
    cleave -> add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max" );
    cleave -> add_action( "mongoose_bite,target_if=max:debuff.latent_poison_injection.stack,if=debuff.latent_poison_injection.stack>9" );
    cleave -> add_action( "raptor_strike,target_if=max:debuff.latent_poison_injection.stack,if=debuff.latent_poison_injection.stack>9" );
    cleave -> add_action( "carve" );
    cleave -> add_action( "serpent_sting,target_if=min:remains,if=!talent.vipers_venom.enabled&!runeforge.latent_poison_injectors.equipped&!runeforge.rylakstalkers_confounding_strikes.equipped&!runeforge.butchers_bone_fragments.equipped" );
    cleave -> add_action( "mongoose_bite,target_if=max:debuff.latent_poison_injection.stack" );
    cleave -> add_action( "raptor_strike,target_if=max:debuff.latent_poison_injection.stack" );
  }
  else
  {
    action_priority_list_t* default_list   = get_action_priority_list( "default" );
    action_priority_list_t* precombat      = get_action_priority_list( "precombat" );
    action_priority_list_t* cds            = get_action_priority_list( "cds" );
    action_priority_list_t* apwfi          = get_action_priority_list( "apwfi" );
    action_priority_list_t* wfi            = get_action_priority_list( "wfi" );
    action_priority_list_t* apst           = get_action_priority_list( "apst" );
    action_priority_list_t* st             = get_action_priority_list( "st" );
    action_priority_list_t* cleave         = get_action_priority_list( "cleave" );

    precombat -> add_action( "use_item,name=azsharas_font_of_power" );
    precombat -> add_action( "guardian_of_azeroth" );
    precombat -> add_action( this, "Coordinated Assault" );
    precombat -> add_action( "worldvein_resonance" );
    precombat -> add_action( "potion,dynamic_prepot=1" );
    precombat -> add_talent( this, "Steel Trap" );
    precombat -> add_action( this, "Harpoon" );

    default_list -> add_action( "auto_attack" );
    default_list -> add_action( "use_items" );
    default_list -> add_action( "call_action_list,name=cds" );
    default_list -> add_action( "mongoose_bite,if=active_enemies=1&target.time_to_die<focus%(action.mongoose_bite.cost-cast_regen)*gcd" );
    default_list -> add_action( "call_action_list,name=apwfi,if=active_enemies<3&talent.chakrams.enabled&talent.alpha_predator.enabled" );
    default_list -> add_action( "call_action_list,name=wfi,if=active_enemies<3&talent.chakrams.enabled");
    default_list -> add_action( "call_action_list,name=st,if=active_enemies<3&!talent.alpha_predator.enabled&!talent.wildfire_infusion.enabled" );
    default_list -> add_action( "call_action_list,name=apst,if=active_enemies<3&talent.alpha_predator.enabled&!talent.wildfire_infusion.enabled" );
    default_list -> add_action( "call_action_list,name=apwfi,if=active_enemies<3&talent.alpha_predator.enabled&talent.wildfire_infusion.enabled" );
    default_list -> add_action( "call_action_list,name=wfi,if=active_enemies<3&!talent.alpha_predator.enabled&talent.wildfire_infusion.enabled" );
    default_list -> add_action( "call_action_list,name=cleave,if=active_enemies>1&!talent.birds_of_prey.enabled|active_enemies>2" );

    // Fillers, cast if nothing else is available.
    default_list -> add_action( "concentrated_flame" );
    default_list -> add_action( "arcane_torrent" );
    default_list -> add_action( "bag_of_tricks" );

    for ( std::string racial : { "blood_fury", "ancestral_call", "fireblood" } )
      cds -> add_action( racial + ",if=cooldown.coordinated_assault.remains>30" );

    cds -> add_action( "lights_judgment" );
    cds -> add_action( "berserking,if=cooldown.coordinated_assault.remains>60|time_to_die<13");
    cds -> add_action( "potion,if=buff.guardian_of_azeroth.up&(buff.berserking.up|buff.blood_fury.up|!race.troll)|(consumable.potion_of_unbridled_fury&target.time_to_die<61|target.time_to_die<26)|!essence.condensed_lifeforce.major&buff.coordinated_assault.up" );
    cds -> add_action( this, "Aspect of the Eagle", "if=target.distance>=6" );
    cds -> add_action( "use_item,name=ashvanes_razor_coral,if=buff.memory_of_lucid_dreams.up&target.time_to_die<cooldown.memory_of_lucid_dreams.remains+15|buff.guardian_of_azeroth.stack=5&target.time_to_die<cooldown.guardian_of_azeroth.remains+20|debuff.razor_coral_debuff.down|target.time_to_die<21|buff.worldvein_resonance.remains&target.time_to_die<cooldown.worldvein_resonance.remains+18|!talent.birds_of_prey.enabled&target.time_to_die<cooldown.coordinated_assault.remains+20&buff.coordinated_assault.remains" );
    cds -> add_action( "use_item,name=galecallers_boon,if=cooldown.memory_of_lucid_dreams.remains|talent.wildfire_infusion.enabled&cooldown.coordinated_assault.remains|!essence.memory_of_lucid_dreams.major&cooldown.coordinated_assault.remains" );
    cds -> add_action( "use_item,name=azsharas_font_of_power" );

    // Essences
    cds->add_action( "focused_azerite_beam,if=raid_event.adds.in>90&focus<focus.max-25|(active_enemies>1&!talent.birds_of_prey.enabled|active_enemies>2)&(buff.blur_of_talons.up&buff.blur_of_talons.remains>3*gcd|!buff.blur_of_talons.up)" );
    cds->add_action( "blood_of_the_enemy,if=((raid_event.adds.remains>90|!raid_event.adds.exists)|(active_enemies>1&!talent.birds_of_prey.enabled|active_enemies>2))&focus<focus.max" );
    cds->add_action( "purifying_blast,if=((raid_event.adds.remains>60|!raid_event.adds.exists)|(active_enemies>1&!talent.birds_of_prey.enabled|active_enemies>2))&focus<focus.max" );
    cds->add_action( "guardian_of_azeroth" );
    cds->add_action( "ripple_in_space" );
    cds->add_action( "concentrated_flame,if=full_recharge_time<1*gcd" );
    cds->add_action( "the_unbound_force,if=buff.reckless_force.up" );
    cds->add_action( "worldvein_resonance" );
    cds->add_action( "reaping_flames,if=target.health.pct>80|target.health.pct<=20|target.time_to_pct_20>30" );
    // lucid Major Focusdump phase
    cds->add_action( this, "Serpent Sting", "if=essence.memory_of_lucid_dreams.major&refreshable&buff.vipers_venom.up&!cooldown.memory_of_lucid_dreams.remains" );
    cds->add_talent( this, "Mongoose Bite", "if=essence.memory_of_lucid_dreams.major&!cooldown.memory_of_lucid_dreams.remains" );
    cds->add_action( this, "Wildfire Bomb", "if=essence.memory_of_lucid_dreams.major&full_recharge_time<1.5*gcd&focus<action.mongoose_bite.cost&!cooldown.memory_of_lucid_dreams.remains" );
    cds->add_action( "memory_of_lucid_dreams,if=focus<action.mongoose_bite.cost&buff.coordinated_assault.up" );

    st -> add_action( this, "Kill Shot" );
    st -> add_action( this, "Harpoon", "if=talent.terms_of_engagement.enabled" );
    st -> add_talent( this, "Flanking Strike", "if=focus+cast_regen<focus.max" );
    st -> add_action( this, "Raptor Strike", "if=buff.coordinated_assault.up&(buff.coordinated_assault.remains<1.5*gcd|buff.blur_of_talons.up&buff.blur_of_talons.remains<1.5*gcd)" );
    st -> add_talent( this, "Mongoose Bite", "if=buff.coordinated_assault.up&(buff.coordinated_assault.remains<1.5*gcd|buff.blur_of_talons.up&buff.blur_of_talons.remains<1.5*gcd)",
                          "To simulate usage for Mongoose Bite or Raptor Strike during Aspect of the Eagle, copy each occurrence of the action and append _eagle to the action name." );
    st -> add_action( this, "Kill Command", "target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max" );
    st -> add_action( this, "Serpent Sting", "if=buff.vipers_venom.up&buff.vipers_venom.remains<1*gcd" );
    st -> add_talent( this, "Steel Trap", "if=focus+cast_regen<focus.max" );
    st -> add_action( this, "Wildfire Bomb", "if=focus+cast_regen<focus.max&refreshable&full_recharge_time<gcd&!buff.memory_of_lucid_dreams.up|focus+cast_regen<focus.max&(!dot.wildfire_bomb.ticking&(!buff.coordinated_assault.up|buff.mongoose_fury.stack<1|time_to_die<18|!dot.wildfire_bomb.ticking&azerite.wilderness_survival.rank>0))&!buff.memory_of_lucid_dreams.up" );
    st -> add_action( this, "Serpent Sting", "if=buff.vipers_venom.up&dot.serpent_sting.remains<4*gcd|dot.serpent_sting.refreshable&!buff.coordinated_assault.up" );
    st -> add_talent( this, "A Murder of Crows", "if=!buff.coordinated_assault.up" );
    st -> add_action( this, "Coordinated Assault", "if=!buff.coordinated_assault.up" );
    st -> add_talent( this, "Mongoose Bite", "if=buff.mongoose_fury.up|focus+cast_regen>focus.max-20&talent.vipers_venom.enabled|focus+cast_regen>focus.max-1&talent.terms_of_engagement.enabled|buff.coordinated_assault.up" );
    st -> add_action( this, "Raptor Strike" );
    st -> add_action( this, "Wildfire Bomb", "if=dot.wildfire_bomb.refreshable" );
    st -> add_action( this, "Serpent Sting", "if=buff.vipers_venom.up" );

    // Alpha Predator
    apst -> add_action( this, "Kill Shot" );
    apst -> add_talent( this, "Mongoose Bite", "if=buff.coordinated_assault.up&(buff.coordinated_assault.remains<1.5*gcd|buff.blur_of_talons.up&buff.blur_of_talons.remains<1.5*gcd)" );
    apst -> add_action( this, "Raptor Strike", "if=buff.coordinated_assault.up&(buff.coordinated_assault.remains<1.5*gcd|buff.blur_of_talons.up&buff.blur_of_talons.remains<1.5*gcd)" );
    apst -> add_talent( this, "Flanking Strike", "if=focus+cast_regen<focus.max" );
    apst -> add_action( this, "Kill Command", "target_if=min:bloodseeker.remains,if=full_recharge_time<1.5*gcd&focus+cast_regen<focus.max" );
    apst -> add_talent( this, "Steel Trap", "if=focus+cast_regen<focus.max" );
    apst -> add_action( this, "Wildfire Bomb", "if=focus+cast_regen<focus.max&!ticking&!buff.memory_of_lucid_dreams.up&(full_recharge_time<1.5*gcd|!dot.wildfire_bomb.ticking&!buff.coordinated_assault.up|!dot.wildfire_bomb.ticking&buff.mongoose_fury.stack<1)|time_to_die<18&!dot.wildfire_bomb.ticking" );
    apst -> add_action( this, "Serpent Sting", "if=!dot.serpent_sting.ticking&!buff.coordinated_assault.up" );
    apst -> add_action( this, "Kill Command", "target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max&(buff.mongoose_fury.stack<5|focus<action.mongoose_bite.cost)" );
    apst -> add_action( this, "Serpent Sting", "if=refreshable&!buff.coordinated_assault.up&buff.mongoose_fury.stack<5" );
    apst -> add_talent( this, "A Murder of Crows", "if=!buff.coordinated_assault.up" );
    apst -> add_action( this, "Coordinated Assault", "if=!buff.coordinated_assault.up" );
    apst -> add_talent( this, "Mongoose Bite", "if=buff.mongoose_fury.up|focus+cast_regen>focus.max-10|buff.coordinated_assault.up" );
    apst -> add_action( this, "Raptor Strike");
    apst -> add_action( this, "Wildfire Bomb", "if=!ticking" );

    // Wildfire Infusion
    wfi -> add_action( this, "Kill Shot" );
    wfi -> add_action( this, "Harpoon", "if=focus+cast_regen<focus.max&talent.terms_of_engagement.enabled" );
    wfi -> add_talent( this, "Mongoose Bite", "if=buff.blur_of_talons.up&buff.blur_of_talons.remains<gcd" );
    wfi -> add_action( this, "Raptor Strike", "if=buff.blur_of_talons.up&buff.blur_of_talons.remains<gcd" );
    wfi -> add_action( this, "Serpent Sting", "if=buff.vipers_venom.up&buff.vipers_venom.remains<1.5*gcd|!dot.serpent_sting.ticking" );
    wfi -> add_action( this, "Wildfire Bomb", "if=full_recharge_time<1.5*gcd&focus+cast_regen<focus.max|(next_wi_bomb.volatile&dot.serpent_sting.ticking&dot.serpent_sting.refreshable|next_wi_bomb.pheromone&!buff.mongoose_fury.up&focus+cast_regen<focus.max-action.kill_command.cast_regen*3)" );
    wfi -> add_action( this, "Kill Command", "target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max-focus.regen" );
    wfi -> add_talent( this, "A Murder of Crows" );
    wfi -> add_talent( this, "Steel Trap", "if=focus+cast_regen<focus.max" );
    wfi -> add_action( this, "Wildfire Bomb", "if=full_recharge_time<1.5*gcd" );
    wfi -> add_action( this, "Coordinated Assault");
    wfi -> add_action( this, "Serpent Sting", "if=buff.vipers_venom.up&dot.serpent_sting.remains<4*gcd" );
    wfi -> add_talent( this, "Mongoose Bite", "if=dot.shrapnel_bomb.ticking|buff.mongoose_fury.stack=5" );
    wfi -> add_action( this, "Wildfire Bomb", "if=next_wi_bomb.shrapnel&dot.serpent_sting.remains>5*gcd" );
    wfi -> add_action( this, "Serpent Sting", "if=refreshable" );
    wfi -> add_talent( this, "Chakrams", "if=!buff.mongoose_fury.remains" );
    wfi -> add_talent( this, "Mongoose Bite" );
    wfi -> add_action( this, "Raptor Strike" );
    wfi -> add_action( this, "Serpent Sting", "if=buff.vipers_venom.up" );
    wfi -> add_action( this, "Wildfire Bomb", "if=next_wi_bomb.volatile&dot.serpent_sting.ticking|next_wi_bomb.pheromone|next_wi_bomb.shrapnel" );

    // Alpha Predator + Wildfire Infusion
    apwfi -> add_action( this, "Kill Shot" );
    apwfi -> add_talent( this, "Mongoose Bite", "if=buff.blur_of_talons.up&buff.blur_of_talons.remains<gcd" );
    apwfi -> add_action( this, "Raptor Strike", "if=buff.blur_of_talons.up&buff.blur_of_talons.remains<gcd" );
    apwfi -> add_action( this, "Serpent Sting", "if=!dot.serpent_sting.ticking" );
    apwfi -> add_talent( this, "A Murder of Crows" );
    apwfi -> add_action( this, "Wildfire Bomb", "if=full_recharge_time<1.5*gcd|focus+cast_regen<focus.max&(next_wi_bomb.volatile&dot.serpent_sting.ticking&dot.serpent_sting.refreshable|next_wi_bomb.pheromone&!buff.mongoose_fury.up&focus+cast_regen<focus.max-action.kill_command.cast_regen*3)" );
    apwfi -> add_action( this, "Coordinated Assault");
    apwfi -> add_talent( this, "Mongoose Bite", "if=buff.mongoose_fury.remains&next_wi_bomb.pheromone" );
    apwfi -> add_action( this, "Kill Command", "target_if=min:bloodseeker.remains,if=full_recharge_time<1.5*gcd&focus+cast_regen<focus.max-20" );
    apwfi -> add_talent( this, "Steel Trap", "if=focus+cast_regen<focus.max" );
    apwfi -> add_action( this, "Raptor Strike", "if=buff.tip_of_the_spear.stack=3|dot.shrapnel_bomb.ticking" );
    apwfi -> add_talent( this, "Mongoose Bite", "if=dot.shrapnel_bomb.ticking" );
    apwfi -> add_action( this, "Wildfire Bomb", "if=next_wi_bomb.shrapnel&focus>30&dot.serpent_sting.remains>5*gcd" );
    apwfi -> add_talent( this, "Chakrams", "if=!buff.mongoose_fury.remains" );
    apwfi -> add_action( this, "Serpent Sting", "if=refreshable" );
    apwfi -> add_action( this, "Kill Command", "target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max&(buff.mongoose_fury.stack<5|focus<action.mongoose_bite.cost)" );
    apwfi -> add_action( this, "Raptor Strike" );
    apwfi -> add_talent( this, "Mongoose Bite", "if=buff.mongoose_fury.up|focus>40|dot.shrapnel_bomb.ticking" );
    apwfi -> add_action( this, "Wildfire Bomb", "if=next_wi_bomb.volatile&dot.serpent_sting.ticking|next_wi_bomb.pheromone|next_wi_bomb.shrapnel&focus>50" );

    cleave -> add_action( "variable,name=carve_cdr,op=setif,value=active_enemies,value_else=5,condition=active_enemies<5" );
    cleave -> add_talent( this, "Mongoose Bite", "if=azerite.blur_of_talons.rank>0&(buff.coordinated_assault.up&(buff.coordinated_assault.remains<1.5*gcd|buff.blur_of_talons.up&buff.blur_of_talons.remains<1.5*gcd|buff.coordinated_assault.remains&!buff.blur_of_talons.remains))" );
    cleave -> add_talent( this, "Mongoose Bite", "target_if=min:time_to_die,if=debuff.latent_poison.stack>(active_enemies|9)&target.time_to_die<active_enemies*gcd" );
    cleave -> add_talent( this, "A Murder of Crows" );
    cleave -> add_action( this, "Coordinated Assault" );
    cleave -> add_action( this, "Carve", "if=dot.shrapnel_bomb.ticking&!talent.hydras_bite.enabled|dot.shrapnel_bomb.ticking&active_enemies>5" );
    cleave -> add_action( this, "Wildfire Bomb", "if=!talent.guerrilla_tactics.enabled|full_recharge_time<gcd|raid_event.adds.remains<6&raid_event.adds.exists" );
    cleave -> add_talent( this, "Butchery", "if=charges_fractional>2.5|dot.shrapnel_bomb.ticking|cooldown.wildfire_bomb.remains>active_enemies-gcd|debuff.blood_of_the_enemy.remains|raid_event.adds.remains<5&raid_event.adds.exists" );
    cleave -> add_talent( this, "Mongoose Bite", "target_if=max:debuff.latent_poison.stack,if=debuff.latent_poison.stack>8" );
    cleave -> add_talent( this, "Chakrams" );
    cleave -> add_action( this, "Kill Command", "target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max" );
    cleave -> add_action( this, "Harpoon", "if=talent.terms_of_engagement.enabled" );
    cleave -> add_action( this, "Carve", "if=talent.guerrilla_tactics.enabled" );
    cleave -> add_talent( this, "Butchery", "if=cooldown.wildfire_bomb.remains>(active_enemies|5)" );
    cleave -> add_talent( this, "Flanking Strike", "if=focus+cast_regen<focus.max" );
    cleave -> add_action( this, "Wildfire Bomb", "if=dot.wildfire_bomb.refreshable|talent.wildfire_infusion.enabled" );
    cleave -> add_action( this, "Serpent Sting", "target_if=min:remains,if=buff.vipers_venom.react" );
    cleave -> add_action( this, "Carve", "if=cooldown.wildfire_bomb.remains>variable.carve_cdr%2" );
    cleave -> add_talent( this, "Steel Trap" );
    cleave -> add_action( this, "Serpent Sting", "target_if=min:remains,if=refreshable&buff.tip_of_the_spear.stack<3&next_wi_bomb.volatile|refreshable&azerite.latent_poison.rank>0" );
    cleave -> add_talent( this, "Mongoose Bite", "target_if=max:debuff.latent_poison.stack",
                          "To simulate usage for Mongoose Bite or Raptor Strike during Aspect of the Eagle, copy each occurrence of the action and append _eagle to the action name." );
    cleave -> add_action( this, "Raptor Strike", "target_if=max:debuff.latent_poison.stack" );
  }
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
  var = {};
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
    h *= 1.0 / ( 1 + buffs.dire_beast -> check_value() );

  if ( buffs.steady_focus -> check() )
    h *= 1.0 / ( 1 + buffs.steady_focus -> check_value() );

  return h;
}

// hunter_t::composite_melee_speed ==========================================

double hunter_t::composite_melee_speed() const
{
  double s = player_t::composite_melee_speed();

  if ( buffs.predator -> check() )
    s *= 1.0 / ( 1 + buffs.predator -> check_stack_value() );

  return s;
}

// hunter_t::composite_spell_haste ===========================================

double hunter_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.dire_beast -> check() )
    h *= 1.0 / ( 1 + buffs.dire_beast -> check_value() );

  if ( buffs.steady_focus -> check() )
    h *= 1.0 / ( 1 + buffs.steady_focus -> check_value() );

  return h;
}

// hunter_t::composite_player_target_crit_chance ============================

double hunter_t::composite_player_target_crit_chance( player_t* target ) const
{
  double crit = player_t::composite_player_target_crit_chance( target );

  crit += buffs.resonating_arrow -> check_value();

  return crit;
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

  m *= 1 + buffs.strength_of_the_pack -> check_value();

  return m;
}

// composite_player_target_multiplier ====================================

double hunter_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double d = player_t::composite_player_target_multiplier( target, school );

  if ( auto td = find_target_data( target ) )
    d *= 1 + td -> debuffs.hunters_mark -> check_value();

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

  return m;
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

  if ( buffs.terms_of_engagement -> check() )
    resource_gain( RESOURCE_FOCUS, buffs.terms_of_engagement -> check_value() * periodicity.total_seconds(), gains.terms_of_engagement );
}

// hunter_t::resource_gain =================================================

double hunter_t::resource_gain( resource_e type, double amount, gain_t* g, action_t* a )
{
  double actual_amount = player_t::resource_gain( type, amount, g, a );

  if ( type == RESOURCE_FOCUS && amount > 0 && player_t::buffs.memory_of_lucid_dreams -> check() )
  {
    actual_amount += player_t::resource_gain( RESOURCE_FOCUS,
                                              amount * azerite_essence.memory_of_lucid_dreams_major_mult,
                                              gains.memory_of_lucid_dreams_major, a );
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

// hunter_t::create_options =================================================

void hunter_t::create_options()
{
  player_t::create_options();

  add_option( opt_string( "summon_pet", options.summon_pet_str ) );
  add_option( opt_timespan( "hunter.pet_attack_speed", options.pet_attack_speed,
                            0.5_s, 4_s ) );
  add_option( opt_timespan( "hunter.pet_basic_attack_delay", options.pet_basic_attack_delay,
                            0_ms, 0.6_s ) );
  add_option( opt_float( "hunter.memory_of_lucid_dreams_proc_chance", options.memory_of_lucid_dreams_proc_chance,
                            0, 1 ) );
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
    fmt::format_to( std::back_inserter( profile_str ), "hunter.pet_attack_speed={}\n", options.pet_attack_speed );
  if ( options.pet_basic_attack_delay != defaults.pet_basic_attack_delay )
    fmt::format_to( std::back_inserter( profile_str ), "hunter.pet_basic_attack_delay={}\n", options.pet_basic_attack_delay );

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
