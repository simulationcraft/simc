// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"

#include "action/parse_buff_effects.hpp"
#include "class_modules/apl/apl_evoker.hpp"
#include "dbc/trait_data.hpp"
#include "sim/option.hpp"

#include "simulationcraft.hpp"

namespace
{
// ==========================================================================
// Evoker
// ==========================================================================

// Forward declarations
struct evoker_t;

enum empower_e
{
  EMPOWER_NONE = 0,
  EMPOWER_1    = 1,
  EMPOWER_2,
  EMPOWER_3,
  EMPOWER_4,
  EMPOWER_MAX
};

enum spell_color_e
{
  SPELL_COLOR_NONE = 0,
  SPELL_BLACK,
  SPELL_BLUE,
  SPELL_BRONZE,
  SPELL_GREEN,
  SPELL_RED
};

enum proc_spell_type_e : unsigned
{
  NONE          = 0x0,
  DRAGONRAGE    = 0x1,  // 3 pyre proc'd via dragonrage
  SCINTILLATION = 0x2,  // level 1 eternity surge proc'd from disintegrate tick via scintillation
  VOLATILITY    = 0x4   // chained pyre proc'd via volatility
};

struct evoker_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* fire_breath;
    dot_t* disintegrate;
  } dots;

  struct debuffs_t
  {
    buff_t* shattering_star;
    buff_t* in_firestorm;
    buff_t* temporal_wound;
  } debuffs;

  struct buffs_t
  {
    buff_t* blistering_scales;
    buff_t* prescience;
    buff_t* infernos_blessing;
    stat_buff_t* ebon_might;
    buff_t* shifting_sands;

    // Legendary
    buff_t* unbound_surge;
  } buffs;

  evoker_td_t( player_t* target, evoker_t* source );
};

template <typename Data, typename Base = action_state_t>
struct evoker_action_state_t : public Base, public Data
{
  static_assert( std::is_base_of_v<action_state_t, Base> );
  static_assert( std::is_default_constructible_v<Data> );  // required for initialize
  static_assert( std::is_copy_assignable_v<Data> );        // required for copy_state

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
    *static_cast<Data*>( this ) = *static_cast<const Data*>( static_cast<const evoker_action_state_t*>( o ) );
  }
};

struct evoker_t : public player_t
{
  // !!!===========================================================================!!!
  // !!! Runtime variables NOTE: these MUST be properly reset in evoker_t::reset() !!!
  // !!!===========================================================================!!!
  vector_with_callback<player_t*> allies_with_my_ebon;
  vector_with_callback<player_t*> allies_with_my_prescience;
  mutable std::vector<buff_t*> allied_ebons_on_me;
  player_t* last_scales_target;
  bool was_empowering;
  // !!!===========================================================================!!!

  timespan_t precombat_travel = 0_s;

  const special_effect_t* naszuro;

  std::vector<evoker_t*> allied_augmentations;
  std::vector<std::function<void()>> allied_ebon_callbacks;

  struct heartbeat_t
  {
    std::vector<std::function<void()>> callbacks;
    timespan_t initial_time;
  } heartbeat;
  bool close_as_clutchmates;

  // Options
  struct options_t
  {
    // Should every Disintegrate in Dragonrage be clipped after the 3rd tick
    bool use_clipping = true;
    // Should chained Disintegrates( those with 5 ticks ) be chained after the 3rd tick in Dragonrage
    bool use_early_chaining = true;
    double scarlet_overheal = 0.5;
    double heal_eb_chance   = 0.9;
    // How much time should prepulling with Deep Breath delay opener
    timespan_t prepull_deep_breath_delay        = timespan_t::from_seconds( 0.3 );
    timespan_t prepull_deep_breath_delay_stddev = timespan_t::from_seconds( 0.05 );
    bool naszuro_accurate_behaviour             = false;
    double naszuro_bounce_chance                = 0.85;
  } option;

  // Action pointers
  struct actions_t
  {
    action_t* obsidian_shards;  // 2t30
    action_t* volatility;
    action_t* volatility_dragonrage;
  } action;

  // Buffs
  struct buffs_t
  {
    // Baseline Abilities
    propagate_const<buff_t*> essence_burst;
    propagate_const<buff_t*> essence_burst_titanic_wrath_disintegrate;
    propagate_const<buff_t*> hover;
    propagate_const<buff_t*> leaping_flames;
    propagate_const<buff_t*> tailwind;

    // Class Traits
    propagate_const<buff_t*> ancient_flame;
    propagate_const<buff_t*> obsidian_scales;
    propagate_const<buff_t*> scarlet_adaptation;
    propagate_const<buff_t*> tip_the_scales;

    // Devastation
    propagate_const<buff_t*> blazing_shards;  // 4t30
    propagate_const<buff_t*> burnout;
    propagate_const<buff_t*> charged_blast;
    propagate_const<buff_t*> dragonrage;
    propagate_const<buff_t*> fury_of_the_aspects;
    propagate_const<buff_t*> imminent_destruction;
    propagate_const<buff_t*> iridescence_blue;
    propagate_const<buff_t*> iridescence_blue_disintegrate;
    propagate_const<buff_t*> iridescence_red;
    propagate_const<buff_t*> limitless_potential;
    propagate_const<buff_t*> power_swell;
    propagate_const<buff_t*> snapfire;
    propagate_const<buff_t*> feed_the_flames_stacking;
    propagate_const<buff_t*> feed_the_flames_pyre;
    propagate_const<buff_t*> ebon_might_self_buff;
    propagate_const<buff_t*> momentum_shift;

    // Preservation

    // Augmentation
    propagate_const<buff_t*> reactive_hide;
    propagate_const<buff_t*> time_skip;
  } buff;

  // Specialization Spell Data
  struct specializations_t
  {
    // Baseline
    const spell_data_t* evoker;        // evoker class aura
    const spell_data_t* devastation;   // devastation class aura
    const spell_data_t* preservation;  // preservation class aura
    const spell_data_t* augmentation;  // augmentation class aura
    const spell_data_t* mastery;       // Mastery Spell Data

    const spell_data_t* tempered_scales;

    const spell_data_t* living_flame_damage;
    const spell_data_t* living_flame_heal;
    const spell_data_t* energizing_flame;

    const spell_data_t* emerald_blossom;
    const spell_data_t* emerald_blossom_heal;
    const spell_data_t* emerald_blossom_spec;

    // Augmentation
    const spell_data_t* close_as_clutchmates;

    // Devastation

    // Preservation
  } spec;

  // Talents
  struct talents_t
  {
    // Class Traits
    player_talent_t landslide;  // row 1
    player_talent_t obsidian_scales;
    player_talent_t expunge;
    player_talent_t natural_convergence;  // row 2
    player_talent_t permeating_chill;
    player_talent_t verdant_embrace;
    player_talent_t forger_of_mountains;  // row 3
    player_talent_t innate_magic;
    player_talent_t obsidian_bulwark;
    player_talent_t enkindled;
    player_talent_t scarlet_adaptation;
    player_talent_t quell;  // row 4
    player_talent_t recall;
    player_talent_t heavy_wingbeats;
    player_talent_t clobbering_sweep;
    player_talent_t tailwind;
    player_talent_t cauterizing_flame;
    player_talent_t roar_of_exhilaration;  // row 5
    player_talent_t instinctive_arcana;
    player_talent_t tip_the_scales;
    player_talent_t attuned_to_the_dream;
    player_talent_t sleep_walk;
    player_talent_t draconic_legacy;  // row 6
    player_talent_t inherent_resistance;
    player_talent_t extended_flight;
    player_talent_t bountiful_bloom;
    player_talent_t blast_furnace;  // row 7
    player_talent_t panacea;
    const spell_data_t* panacea_spell;
    player_talent_t exuberance;
    player_talent_t source_of_magic;
    player_talent_t ancient_flame;
    player_talent_t unravel;  // row 8
    player_talent_t protracted_talons;
    player_talent_t oppressing_roar;
    player_talent_t rescue;
    player_talent_t lush_growth;
    player_talent_t renewing_blaze;
    player_talent_t leaping_flames;  // row 9
    player_talent_t overawe;
    player_talent_t aerial_mastery;
    player_talent_t twin_guardian;
    player_talent_t foci_of_life;
    player_talent_t fire_within;
    player_talent_t terror_of_the_skies;  // row 10
    player_talent_t time_spiral;
    player_talent_t zephyr;

    // Devastation Traits
    player_talent_t pyre;                // row 1
    player_talent_t ruby_essence_burst;  // row 2
    player_talent_t azure_essence_burst;
    player_talent_t dense_energy;  // row 3
    player_talent_t imposing_presence;
    player_talent_t inner_radiance;
    player_talent_t eternity_surge;
    player_talent_t volatility;  // row 4
    player_talent_t power_nexus;
    player_talent_t dragonrage;
    player_talent_t lay_waste;
    player_talent_t arcane_intensity;
    player_talent_t ruby_embers;  // row 5
    player_talent_t engulfing_blaze;
    player_talent_t animosity;
    player_talent_t essence_attunement;
    player_talent_t firestorm;  // row 6
    player_talent_t heat_wave;
    player_talent_t titanic_wrath;
    player_talent_t honed_aggression;
    player_talent_t eternitys_span;
    player_talent_t eye_of_infinity;
    player_talent_t event_horizon;
    player_talent_t causality;
    player_talent_t catalyze;  // row 7
    player_talent_t tyranny;
    player_talent_t charged_blast;
    player_talent_t shattering_star;
    player_talent_t snapfire;  // row 8
    player_talent_t raging_inferno;
    player_talent_t font_of_magic;
    player_talent_t onyx_legacy;
    player_talent_t spellweavers_dominance;
    player_talent_t focusing_iris;
    player_talent_t arcane_vigor;
    player_talent_t burnout;  // row 9
    player_talent_t imminent_destruction;
    player_talent_t scintillation;
    player_talent_t power_swell;
    player_talent_t feed_the_flames;  // row 10
    const spell_data_t* feed_the_flames_pyre_buff;
    player_talent_t everburning_flame;
    player_talent_t hoarded_power;
    player_talent_t iridescence;

    // Preservation Traits

    // Augmentation Traits
    player_talent_t ebon_might;
    const spell_data_t* ebon_might_self_buff;
    const spell_data_t* sands_of_time;
    player_talent_t eruption;
    player_talent_t essence_burst;
    // Imposing Presence / Inner Radiance - Non DPS
    player_talent_t ricocheting_pyroclast;
    // Essence Attunement - Devastation also has
    player_talent_t pupil_of_alexstrasza;
    player_talent_t echoing_strike;
    player_talent_t upheaval;
    player_talent_t breath_of_eons;
    const spell_data_t* breath_of_eons_damage;
    const spell_data_t* temporal_wound;
    // Defy Fate - Non DPS
    // Timelessness - Non DPS
    // Seismic Slam - Non DPS
    player_talent_t volcanism;
    // Perilous Fate / Chrono Ward - Non DPS
    // Stretch Time - Non DPS
    // Geomancy - Non DPS
    // Bestow Weyrnstone - Non DPS
    player_talent_t blistering_scales;
    const spell_data_t* blistering_scales_damage;
    // Draconic Attunements - Non DPS
    // Spatial Paradox Non DPS - Movement DPS Gain?
    player_talent_t unyielding_domain;
    player_talent_t tectonic_locus;
    player_talent_t regenerative_chitin;
    player_talent_t molten_blood;
    // Power Nexus - Devastation also has
    // Aspects' Favor - Non DPS
    player_talent_t plot_the_future;
    player_talent_t dream_of_spring;
    // Symbiotic Bloom - Non DPS but Scarlet exists. Todo: implement healing
    player_talent_t reactive_hide;
    const spell_data_t* reactive_hide_buff;
    // Hoarded Power - Devas Has
    player_talent_t ignition_rush;
    player_talent_t prescience;
    const spell_data_t* prescience_buff;
    // Prolong Life - Non DPS. Scarlet Exists. Todo: Implement Healing
    player_talent_t momentum_shift;
    player_talent_t infernos_blessing;
    const spell_data_t* infernos_blessing_damage;
    const spell_data_t* infernos_blessing_buff;
    player_talent_t time_skip;
    player_talent_t accretion;
    player_talent_t anachronism;
    player_talent_t motes_of_possibility;
    // Font of Magic - Devastation ha
    player_talent_t tomorrow_today;
    player_talent_t interwoven_threads;
    player_talent_t overlord;
    player_talent_t fate_mirror;
    const spell_data_t* fate_mirror_damage;
  } talent;

  // Benefits
  struct benefits_t
  {
    propagate_const<benefit_t*> supercharged_shards;
  } benefit;

  // Cooldowns
  struct cooldowns_t
  {
    propagate_const<cooldown_t*> eternity_surge;
    propagate_const<cooldown_t*> fire_breath;
    propagate_const<cooldown_t*> firestorm;
    propagate_const<cooldown_t*> upheaval;
    propagate_const<cooldown_t*> breath_of_eons;
  } cooldown;

  // Gains
  struct gains_t
  {
    propagate_const<gain_t*> eye_of_infinity;
    propagate_const<gain_t*> roar_of_exhilaration;
    propagate_const<gain_t*> energizing_flame;
  } gain;

  // Procs
  struct procs_t
  {
    propagate_const<proc_t*> ruby_essence_burst;
    propagate_const<proc_t*> azure_essence_burst;
    propagate_const<proc_t*> anachronism_essence_burst;
    propagate_const<proc_t*> echoing_strike;
  } proc;

  // RPPMs
  struct rppms_t
  {
  } rppm;

  // Uptimes
  struct uptimes_t
  {
  } uptime;

  // Background Actions
  struct background_actions_t
  {
    propagate_const<action_t*> ebon_might;
  } background_actions;

  evoker_t( sim_t* sim, std::string_view name, race_e r = RACE_DRACTHYR_HORDE );

  // Character Definitions
  unsigned int specialization_aura_id();
  void init_action_list() override;
  void init_finished() override;
  void init_base_stats() override;
  void init_background_actions() override;
  // void init_resources( bool ) override;
  void init_benefits() override;
  resource_e primary_resource() const override
  {
    return RESOURCE_ESSENCE;
  }
  role_e primary_role() const override;
  void init_gains() override;
  void init_procs() override;
  // void init_rng() override;
  // void init_uptimes() override;
  void init_spells() override;
  void init_special_effects() override;
  // void init_finished() override;
  void create_actions() override;
  void create_buffs() override;
  void create_options() override;
  // void arise() override;
  void moving() override;
  void schedule_ready( timespan_t, bool ) override;
  // void combat_begin() override;
  // void combat_end() override;
  void analyze( sim_t& ) override;
  void reset() override;
  void copy_from( player_t* ) override;
  void merge( player_t& ) override;
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;

  target_specific_t<evoker_td_t> target_data;
  const evoker_td_t* find_target_data( const player_t* target ) const override;
  evoker_td_t* get_target_data( player_t* target ) const override;

  void apply_affecting_auras( action_t& action ) override;
  action_t* create_action( std::string_view name, std::string_view options_str ) override;
  std::unique_ptr<expr_t> create_expression( std::string_view expr_str ) override;

  // Stat & Multiplier overrides
  double matching_gear_multiplier( attribute_e ) const override;
  double composite_base_armor_multiplier() const override;
  double composite_armor() const override;
  double composite_base_armor() const;
  double composite_attribute_multiplier( attribute_e ) const override;
  double composite_player_multiplier( school_e ) const override;
  double composite_spell_haste() const override;
  double composite_melee_haste() const override;
  stat_e convert_hybrid_stat( stat_e ) const override;
  double passive_movement_modifier() const override;
  double resource_regen_per_second( resource_e ) const override;
  void target_mitigation( school_e, result_amount_type, action_state_t* ) override;
  double temporary_movement_modifier() const override;

  void bounce_naszuro( player_t*, timespan_t );

  // Augmentation Helpers
  void extend_ebon( timespan_t );

  // Utility functions
  const spelleffect_data_t* find_spelleffect( const spell_data_t* spell, effect_subtype_t subtype = A_MAX,
                                              int misc_value               = P_GENERIC,
                                              const spell_data_t* affected = spell_data_t::nil(),
                                              effect_type_t type           = E_APPLY_AURA );
  const spell_data_t* find_spell_override( const spell_data_t* base, const spell_data_t* passive );

  std::vector<action_t*> secondary_action_list;

  template <typename T, typename... Ts>
  T* get_secondary_action( std::string_view n, Ts&&... args )
  {
    auto it = range::find( secondary_action_list, n, &action_t::name_str );
    if ( it != secondary_action_list.cend() )
      return dynamic_cast<T*>( *it );

    auto a        = new T( this, std::forward<Ts>( args )... );
    a->background = true;
    secondary_action_list.push_back( a );
    return a;
  }
};

namespace buffs
{
// Template for base evoker buffs
template <class Base>
struct evoker_buff_t : public Base
{
private:
  using bb = Base;  // buff base, buff_t/stat_buff_t/etc.

protected:
  using base_t = evoker_buff_t<buff_t>;  // shorthand

public:
  evoker_buff_t( evoker_t* player, std::string_view name, const spell_data_t* spell = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : bb( player, name, spell, item )
  {
  }

  evoker_buff_t( evoker_td_t& td, std::string_view name, const spell_data_t* spell = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : bb( td, name, spell, item )
  {
  }

  timespan_t buff_duration() const override
  {
    if ( p()->specialization() != EVOKER_AUGMENTATION || !bb::data().affected_by( p()->spec.mastery->effectN( 2 ) ) )
      return bb::buff_duration();

    auto m = 1 + p()->cache.mastery() * p()->spec.mastery->effectN( 2 ).mastery_value();

    return m * bb::buff_duration();
  }

  evoker_t* p()
  {
    return static_cast<evoker_t*>( bb::source );
  }

  const evoker_t* p() const
  {
    return static_cast<evoker_t*>( bb::source );
  }
};

struct time_skip_t : public buff_t
{
  std::vector<action_t*> affected_actions;

  time_skip_t( evoker_t* p ) : buff_t( p, "time_skip", p->talent.time_skip )
  {
    set_cooldown( 0_ms );
    set_default_value_from_effect( 1 );

    apply_affecting_aura( p->talent.tomorrow_today );

    set_stack_change_callback( [ this ]( buff_t* /* b */, int /* old */, int new_ ) { update_cooldowns( new_ ); } );
  }

  void update_cooldowns( int new_ )
  {
    double recharge_rate_multiplier = 1.0 / ( default_value );
    for ( auto a : affected_actions )
    {
      if ( new_ > 0 )
      {
        a->dynamic_recharge_rate_multiplier *= recharge_rate_multiplier;
      }
      else
      {
        a->dynamic_recharge_rate_multiplier /= recharge_rate_multiplier;
      }
      if ( a->cooldown->action == a )
        a->cooldown->adjust_recharge_multiplier();
      if ( a->internal_cooldown->action == a )
        a->internal_cooldown->adjust_recharge_multiplier();
    }
  }
};
}  // namespace buffs

// Base Classes =============================================================

struct stats_data_t
{
  stats_t* stats;
};

struct augment_t : public spell_base_t
{
private:
  using ab = spell_base_t;

public:
  augment_t( std::string_view name, player_t* player, const spell_data_t* spell = spell_data_t::nil() )
    : ab( ACTION_OTHER, name, player, spell )
  {
    harmful = false;
    if ( !target || target->is_enemy() )
      target = player;
  }

  void activate() override
  {
    sim->player_no_pet_list.register_callback( [ this ]( player_t* ) { target_cache.is_valid = false; } );
  }

  int num_targets() const override
  {
    return as<int>( sim->player_no_pet_list.size() );
  }

  size_t available_targets( std::vector<player_t*>& target_list ) const override
  {
    target_list.clear();
    target_list.push_back( target );

    for ( const auto& t : sim->player_no_pet_list )
    {
      if ( t != target )
        target_list.push_back( t );
    }

    return target_list.size();
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    if ( name_str == "active_allies" )
    {
      return make_fn_expr( "active_allies", [ this ] { return num_targets(); } );
    }

    return spell_base_t::create_expression( name );
  }

  player_t* get_expression_target() override
  {
    return target;
  }
};

struct external_action_data
{
  evoker_t* evoker;
};

// Template for base external evoker action code.
template <class Base>
struct evoker_external_action_t : public Base
{
private:
  using ab = Base;  // action base, spell_t/heal_t/etc.

protected:
  using state_t = evoker_action_state_t<external_action_data>;

public:
  evoker_t* evoker;

  evoker_external_action_t( std::string_view name, player_t* player, const spell_data_t* spell = spell_data_t::nil() )
    : ab( name, player, spell ), evoker( nullptr )
  {
  }

  action_state_t* new_state() override
  {
    return new state_t( this, ab::target );
  }

  state_t* cast_state( action_state_t* s )
  {
    return static_cast<state_t*>( s );
  }

  const state_t* cast_state( const action_state_t* s ) const
  {
    return static_cast<const state_t*>( s );
  }

  double composite_crit_chance( const action_state_t* s ) const
  {
    return action_t::composite_crit_chance() + p( s )->cache.spell_crit_chance();
  }

  double composite_haste( const action_state_t* s ) const
  {
    return action_t::composite_haste() * p( s )->cache.spell_speed();
  }

  double composite_crit_chance_multiplier( const action_state_t* s ) const
  {
    return action_t::composite_crit_chance_multiplier() * p( s )->composite_spell_crit_chance_multiplier();
  }

  double composite_versatility( const action_state_t* s ) const override
  {
    return action_t::composite_versatility( s ) + p( s )->cache.damage_versatility();
  }

  double composite_attack_power( const action_state_t* s ) const
  {
    return p( s )->composite_melee_attack_power_by_type( ab::get_attack_power_type() );
  }

  double composite_spell_power( const action_state_t* s ) const
  {
    double spell_power = 0;
    double tmp;

    auto _player = p( s );

    for ( auto base_school : ab::base_schools )
    {
      tmp = _player->cache.spell_power( base_school );
      if ( tmp > spell_power )
        spell_power = tmp;
    }

    return spell_power;
  }

  void snapshot_internal( action_state_t* state, unsigned flags, result_amount_type rt ) override
  {
    assert( state );

    cast_state( state )->evoker = evoker;

    assert( p( state ) );

    ab::snapshot_internal( state, flags, rt );

    if ( flags & STATE_CRIT )
      state->crit_chance = composite_crit_chance( state ) * composite_crit_chance_multiplier( state );

    if ( flags & STATE_HASTE )
      state->haste = composite_haste( state );

    if ( flags & STATE_AP )
      state->attack_power = composite_attack_power( state ) * p( state )->composite_attack_power_multiplier();

    if ( flags & STATE_SP )
      state->spell_power = composite_spell_power( state ) * p( state )->composite_spell_power_multiplier();

    if ( flags & STATE_VERSATILITY )
      state->versatility = composite_versatility( state );
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    ab::snapshot_state( s, rt );
    cast_state( s )->evoker = evoker;
  }

  evoker_t* p( action_state_t* s )
  {
    return cast_state( s )->evoker;
  }

  const evoker_t* p( const action_state_t* s ) const
  {
    return cast_state( s )->evoker;
  }

  evoker_td_t* td( action_state_t* s, player_t* t ) const
  {
    return p( s )->get_target_data( t );
  }

  const evoker_td_t* find_td( action_state_t* s, const player_t* t ) const
  {
    return p( s )->find_target_data( t );
  }

  double composite_spell_power() const override
  {
    return ab::composite_spell_power();
  }
};

// Template for base evoker action code.
template <class Base>
struct evoker_action_t : public Base, public parse_buff_effects_t<evoker_td_t>
{
private:
  using ab = Base;  // action base, spell_t/heal_t/etc.

public:
  spell_color_e spell_color;
  unsigned proc_spell_type;
  bool move_during_hover;

  evoker_action_t( std::string_view name, evoker_t* player, const spell_data_t* spell = spell_data_t::nil() )
    : ab( name, player, spell ),
      parse_buff_effects_t( this ),
      spell_color( SPELL_COLOR_NONE ),
      proc_spell_type( proc_spell_type_e::NONE ),
      move_during_hover( false )
  {
    // TODO: find out if there is a better data source for the spell color
    if ( ab::data().ok() )
    {
      const auto& desc = player->dbc->spell_text( ab::data().id() ).rank();
      if ( desc )
      {
        if ( util::str_compare_ci( desc, "Black" ) )
          spell_color = SPELL_BLACK;
        else if ( util::str_compare_ci( desc, "Blue" ) )
          spell_color = SPELL_BLUE;
        else if ( util::str_compare_ci( desc, "Bronze" ) )
          spell_color = SPELL_BRONZE;
        else if ( util::str_compare_ci( desc, "Green" ) )
          spell_color = SPELL_GREEN;
        else if ( util::str_compare_ci( desc, "Red" ) )
          spell_color = SPELL_RED;
      }

      apply_buff_effects();
      apply_debuffs_effects();

      move_during_hover =
          player->find_spelleffect( player->find_class_spell( "Hover" ), A_CAST_WHILE_MOVING_WHITELIST, 0, &ab::data() )
              ->ok();

      const auto spell_powers = ab::data().powers();
      if ( spell_powers.size() == 1 && spell_powers.front().aura_id() == 0 )
      {
        ab::resource_current = spell_powers.front().resource();
      }
      else
      {
        // Find the first power entry without a aura id
        auto it = range::find( spell_powers, 0U, &spellpower_data_t::aura_id );
        if ( it != spell_powers.end() )
        {
          ab::resource_current = it->resource();
        }
        else
        {
          auto it = range::find( spell_powers, p()->specialization_aura_id(), &spellpower_data_t::aura_id );
          if ( it != spell_powers.end() )
          {
            ab::resource_current = it->resource();
          }
        }
      }

      for ( const spellpower_data_t& pd : spell_powers )
      {
        if ( pd.aura_id() != 0 && pd.aura_id() != p()->specialization_aura_id() )
          continue;

        if ( pd._cost != 0 )
          ab::base_costs[ pd.resource() ] = pd.cost();
        else
          ab::base_costs[ pd.resource() ] = floor( pd.cost() * p()->resources.base[ pd.resource() ] );

        ab::secondary_costs[ pd.resource() ] = pd.max_cost();

        if ( pd._cost_per_tick != 0 )
          ab::base_costs_per_tick[ pd.resource() ] = pd.cost_per_tick();
        else
          ab::base_costs_per_tick[ pd.resource() ] = floor( pd.cost_per_tick() * p()->resources.base[ pd.resource() ] );
      }
    }
  }

  evoker_t* p()
  {
    return static_cast<evoker_t*>( ab::player );
  }

  const evoker_t* p() const
  {
    return static_cast<evoker_t*>( ab::player );
  }

  void execute() override
  {
    ab::execute();

    // Precombat Hacks Beware
    if ( ab::is_precombat && p()->precombat_travel > 0_s && ab::gcd() > timespan_t::zero() )
    {
      // Start GCD
      ab::start_gcd();
      // Set this as the last Executing Action
      ab::player->last_foreground_action = this;
      // Work out what is longer, execute time or GCD, then subtract off travel time of precast missile spells.
      auto gcd_ready = std::max( 0_s, std::max( ab::player->gcd_ready, ab::execute_time() ) - p()->precombat_travel );
      // Set GCD ready to this, which is when the sim will wake for first action
      ab::player->gcd_ready = gcd_ready;
      // Add time spent to stats, because it cost this much time
      ab::stats->iteration_total_execute_time += gcd_ready;
    }
  }

  evoker_td_t* td( player_t* t ) const
  {
    return p()->get_target_data( t );
  }

  const evoker_td_t* find_td( const player_t* t ) const
  {
    return p()->find_target_data( t );
  }

  bool usable_moving() const override
  {
    return move_during_hover && p()->buff.hover->check();
  }

  
  template <typename... Ts>
  void parse_buff_effects_mods( buff_t* buff, const bfun& f, unsigned ignore_mask, bool use_stacks, bool use_default,
                                Ts... mods )
  {
    if ( !buff )
      return;

    const spell_data_t* spell = &buff->data();

    for ( size_t i = 1; i <= spell->effect_count(); i++ )
    {
      if ( ignore_mask & 1 << ( i - 1 ) )
        continue;

      parse_buff_effect( buff, f, spell, i, use_stacks, use_default, false, mods... );
    }
  }

  // Syntax: parse_buff_effects[<S[,S...]>]( buff[, ignore_mask|use_stacks[, use_default]][, spell1[,spell2...] )
  //  buff = buff to be checked for to see if effect applies
  //  ignore_mask = optional bitmask to skip effect# n corresponding to the n'th bit
  //  use_stacks = optional, default true, whether to multiply value by stacks
  //  use_default = optional, default false, whether to use buff's default value over effect's value
  //  S = optional list of template parameter(s) to indicate spell(s) with redirect effects
  //  spell = optional list of spell(s) with redirect effects that modify the effects on the buff
  void apply_buff_effects()
  {
    // using S = const spell_data_t*;

    parse_buff_effects( p()->buff.ancient_flame );
    parse_buff_effects( p()->buff.burnout );
    parse_buff_effects( p()->buff.essence_burst, p()->talent.ignition_rush );
    parse_buff_effects( p()->buff.snapfire );
    parse_buff_effects( p()->buff.tip_the_scales );

    parse_buff_effects( p()->buff.imminent_destruction );

    if ( p()->specialization() == EVOKER_AUGMENTATION )
    {
      parse_buff_effects_mods(
          p()->buff.ebon_might_self_buff, [ this ] { return p()->close_as_clutchmates; }, 0U, true, false,
          p()->sets->set( EVOKER_AUGMENTATION, T30, B2 ), p()->spec.close_as_clutchmates );

      parse_buff_effects_mods(
          p()->buff.ebon_might_self_buff, [ this ] { return !p()->close_as_clutchmates; }, 0U, true, false,
          p()->sets->set( EVOKER_AUGMENTATION, T30, B2 ) );
    }

  }

  // Syntax: parse_dot_debuffs[<S[,S...]>]( func, spell_data_t* dot[, spell_data_t* spell1[,spell2...] )
  //  func = function returning the dot_t* of the dot
  //  dot = spell data of the dot
  //  S = optional list of template parameter(s) to indicate spell(s)with redirect effects
  //  spell = optional list of spell(s) with redirect effects that modify the effects on the dot
  void apply_debuffs_effects()
  {
    // using S = const spell_data_t*;

    parse_debuff_effects( []( evoker_td_t* t ) { return t->debuffs.shattering_star->check(); },
                          p()->talent.shattering_star );
  }

  double cost() const override
  {
    if ( ab::data().powers().size() > 1 && ab::current_resource() != ab::data().powers()[ 0 ].resource() )
      return ab::cost();

    return std::max( 0.0, ( ab::cost() + get_buff_effects_value( flat_cost_buffeffects, true, false ) ) *
                              get_buff_effects_value( cost_buffeffects, false, false ) );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t ) * get_debuff_effects_value( td( t ) );
    return tm;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double ta = ab::composite_ta_multiplier( s ) * get_buff_effects_value( ta_multiplier_buffeffects );
    return ta;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double da = ab::composite_da_multiplier( s ) * get_buff_effects_value( da_multiplier_buffeffects );
    return da;
  }

  double composite_crit_chance() const override
  {
    double cc = ab::composite_crit_chance() + get_buff_effects_value( crit_chance_buffeffects, true );

    return cc;
  }

  timespan_t execute_time() const override
  {
    timespan_t et = ab::execute_time() * get_buff_effects_value( execute_time_buffeffects );
    return std::max( 0_ms, et );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t dd = ab::composite_dot_duration( s ) * get_buff_effects_value( dot_duration_buffeffects );
    return dd;
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double rm = ab::recharge_multiplier( cd ) * get_buff_effects_value( recharge_multiplier_buffeffects, false, false );
    return rm;
  }

  void init() override
  {
    ab::init();

    if ( p()->specialization() == EVOKER_AUGMENTATION )
    {
      auto time_skip = static_cast<buffs::time_skip_t*>( p()->buff.time_skip.get() );
      if ( p()->find_spelleffect( &time_skip->data(), A_MAX, 0, &ab::data() )->ok() )
      {
        if ( range::find( time_skip->affected_actions, this ) == time_skip->affected_actions.end() )
        {
          time_skip->affected_actions.push_back( this );
        }
      }
    }
  }
};

// Essence base template
template <class BASE>
struct essence_base_t : public BASE
{
  timespan_t ftf_dur;
  double hoarded_pct;
  double titanic_mul;
  double obsidian_shards_mul;

  essence_base_t( std::string_view n, evoker_t* p, const spell_data_t* s, std::string_view o = {} )
    : BASE( n, p, s, o ),
      ftf_dur( -timespan_t::from_seconds( p->talent.feed_the_flames->effectN( 1 ).base_value() ) ),
      hoarded_pct( p->talent.hoarded_power->effectN( 1 ).percent() ),
      titanic_mul( p->talent.titanic_wrath->effectN( 1 ).percent() ),
      obsidian_shards_mul( p->sets->set( EVOKER_DEVASTATION, T30, B2 )->effectN( 1 ).percent() )
  {
  }

  void consume_resource() override
  {
    BASE::consume_resource();

    if ( !BASE::base_cost() || BASE::proc || BASE::current_resource() != RESOURCE_ESSENCE )
      return;

    if ( BASE::p()->buff.essence_burst->up() )
    {
      if ( BASE::p()->talent.momentum_shift.ok() )
      {
        BASE::p()->buff.momentum_shift->trigger();
      }
      if ( !BASE::rng().roll( hoarded_pct ) )
        BASE::p()->buff.essence_burst->decrement();
    }
  }
};

struct evoker_augment_t : public evoker_action_t<augment_t>
{
private:
  using ab = evoker_action_t<augment_t>;

public:
  evoker_augment_t( std::string_view name, evoker_t* player, const spell_data_t* spell = spell_data_t::nil(),
                    std::string_view options_str = {} )
    : ab( name, player, spell )
  {
    parse_options( options_str );
  }
};

// Empowered spell base templates
struct empower_data_t
{
  empower_e empower;

  friend void sc_format_to( const empower_data_t& data, fmt::format_context::iterator out )
  {
    fmt::format_to( out, "empower_level={}", static_cast<int>( data.empower ) );
  }
};

template <class BASE>
struct empowered_base_t : public BASE
{
protected:
  using state_t = evoker_action_state_t<empower_data_t>;

public:
  empower_e max_empower;

  empowered_base_t( std::string_view name, evoker_t* p, const spell_data_t* spell, std::string_view options_str = {} )
    : BASE( name, p, spell, options_str ),
      max_empower( p->talent.font_of_magic.ok() ? empower_e::EMPOWER_4 : empower_e::EMPOWER_3 )
  {
  }

  action_state_t* new_state() override
  {
    return new state_t( this, BASE::target );
  }

  state_t* cast_state( action_state_t* s )
  {
    return static_cast<state_t*>( s );
  }

  const state_t* cast_state( const action_state_t* s ) const
  {
    return static_cast<const state_t*>( s );
  }
};

template <class BASE>
struct empowered_release_t : public empowered_base_t<BASE>
{
  struct shifting_sands_t : public evoker_augment_t
  {
    shifting_sands_t( evoker_t* p )
      : evoker_augment_t( "shifting_sands", p, p->find_spell( 413984 ) )
    {
      background   = true;
      dot_duration = base_tick_time = 0_ms;

      if ( aoe == 0 )
        aoe = 1;
    }

    void impact( action_state_t* s ) override
    {
      evoker_augment_t::impact( s );

      p()->get_target_data( s->target )->buffs.shifting_sands->current_value = p()->cache.mastery_value();
      p()->get_target_data( s->target )->buffs.shifting_sands->trigger();
    }

    size_t available_targets( std::vector<player_t*>& target_list ) const override
    {
      std::vector<player_t*> helper_list;

      target_list.clear();

      if ( sim->player_no_pet_list.size() <= n_targets() )
      {
        for ( const auto& t : sim->player_no_pet_list )
          target_list.push_back( t );

        return target_list.size();
      }

      for ( const auto& t : sim->player_no_pet_list )
      {
        if ( t != player )
        {
          if ( t->role != ROLE_HYBRID && t->role != ROLE_HEAL && t->role != ROLE_TANK &&
               p()->get_target_data( t )->buffs.ebon_might->check() && !p()->get_target_data( t )->buffs.shifting_sands->check() )
          {
            target_list.push_back( t );
          }
          else
          {
            helper_list.push_back( t );
          }
        }
      }

      if ( as<int>( target_list.size() ) >= n_targets() )
      {
        if ( as<int>( target_list.size() ) > n_targets() )
        {
          rng().shuffle( target_list.begin(), target_list.end() );
        }
        return target_list.size();
      }
      
      std::vector<std::function<bool( player_t* )>> lambdas = {
          [ this ]( player_t* t ) {
            return p()->get_target_data( t )->buffs.ebon_might->check() &&
                   !p()->get_target_data( t )->buffs.shifting_sands->check();
          },
          [ this ]( player_t* t ) {
            return t->role != ROLE_HYBRID && t->role != ROLE_HEAL && t->role != ROLE_TANK &&
                   !p()->get_target_data( t )->buffs.shifting_sands->check();
          },
          [ this ]( player_t* t ) { return !p()->get_target_data( t )->buffs.shifting_sands->check(); },
          [ this ]( player_t* t ) { return t->role != ROLE_HYBRID && t->role != ROLE_HEAL && t->role != ROLE_TANK; },
          [ this ]( player_t* t ) { return true; } };

      for ( auto& fn : lambdas )
      {
        auto pos = target_list.size();

        for ( size_t i = 0; i < helper_list.size(); )
        {
          if ( fn( helper_list[ i ] ) )
          {
            target_list.push_back( helper_list[ i ] );
            erase_unordered( helper_list, helper_list.begin() + i );
          }
          else
          {
            i++;
          }
        }

        if ( as<int>( target_list.size() ) >= n_targets() )
        {
          if ( target_list.size() - pos > 1 )
          {
            rng().shuffle( target_list.begin() + pos + 1, target_list.end() );
          }

          return target_list.size();
        }
      }

      return target_list.size();
    }

    // No point caching using basic cache, cache would be ruined by every cast.
    // TODO: If noticable lag caused by this target list implement custom cache.
    std::vector<player_t*>& target_list() const override
    {
      available_targets( target_cache.list );

      return target_cache.list;
    }
  };

  using ab = empowered_base_t<BASE>;

  timespan_t extend_tier29_4pc;
  timespan_t extend_ebon;
  action_t* sands;

  empowered_release_t( std::string_view name, evoker_t* p, const spell_data_t* spell )
    : ab( name, p, spell ),
      extend_ebon( p->talent.ebon_might.ok() ? p->talent.sands_of_time->effectN( 2 ).time_value() : 0_s ),
      sands( p->specialization() == EVOKER_AUGMENTATION ? p->get_secondary_action<shifting_sands_t>( "shifting_sands" )
                                                        : nullptr )
  {
    ab::dual = true;

    // TODO: Continue to check it uses this spell to trigger GCD, as of 28/10/2022 it does. It can still be bypassed via
    // spell queue. Potentally add a better way to model this?
    const spell_data_t* gcd_spell = p->find_spell( 359115 );
    if ( gcd_spell )
      ab::trigger_gcd = gcd_spell->gcd();
    ab::gcd_type = gcd_haste_type::NONE;

    extend_tier29_4pc =
        timespan_t::from_seconds( p->sets->set( EVOKER_DEVASTATION, T29, B4 )->effectN( 1 ).base_value() );
  }

  empower_e empower_level( const action_state_t* s ) const
  {
    return ab::cast_state( s )->empower;
  }

  int empower_value( const action_state_t* s ) const
  {
    return static_cast<int>( ab::cast_state( s )->empower );
  }

  void execute() override
  {
    ab::p()->was_empowering = false;

    ab::execute();

    ab::p()->extend_ebon( extend_ebon );

    if ( sands )
      sands->execute();
  }
};

template <class BASE>
struct empowered_charge_t : public empowered_base_t<BASE>
{
  using ab = empowered_base_t<BASE>;

  action_t* release_spell;
  stats_t* dummy_stat;  // used to hack channel tick time into execute time
  stats_t* orig_stat;
  int empower_to;
  timespan_t base_empower_duration;
  timespan_t lag;

  empowered_charge_t( std::string_view name, evoker_t* p, const spell_data_t* spell, std::string_view options_str )
    : ab( name, p, p->find_spell_override( spell, p->talent.font_of_magic ) ),
      release_spell( nullptr ),
      dummy_stat( p->get_stats( "dummy_stat" ) ),
      orig_stat( ab::stats ),
      empower_to( ab::max_empower ),
      base_empower_duration( 0_ms ),
      lag( 0_ms )
  {
    ab::channeled = true;

    // TODO: convert to full empower expression support
    ab::add_option( opt_int( "empower_to", empower_to, EMPOWER_1, EMPOWER_MAX ) );

    ab::parse_options( options_str );

    empower_to = std::min( static_cast<int>( ab::max_empower ), empower_to );

    ab::dot_duration = ab::base_tick_time = base_empower_duration =
        base_time_to_empower( static_cast<empower_e>( empower_to ) );

    ab::apply_affecting_aura( p->talent.font_of_magic );

    ab::gcd_type = gcd_haste_type::NONE;
    if ( ab::trigger_gcd > timespan_t::zero() )
      ab::min_gcd = ab::trigger_gcd;
  }

  template <typename T>
  void create_release_spell( std::string_view n )
  {
    static_assert( std::is_base_of_v<empowered_release_t<BASE>, T>,
                   "Empowered release spell must be dervied from empowered_release_spell_t." );

    release_spell             = ab::p()->template get_secondary_action<T>( n );
    release_spell->stats      = ab::stats;
    release_spell->background = false;
  }

  timespan_t base_time_to_empower( empower_e emp ) const
  {
    // TODO: confirm these values and determine if they're set values or adjust based on a formula
    // Currently all empowered spells are 2.5s base and 3.25s with empower 4
    switch ( emp )
    {
      case empower_e::EMPOWER_1:
        return 1000_ms;
      case empower_e::EMPOWER_2:
        return 1750_ms;
      case empower_e::EMPOWER_3:
        return 2500_ms;
      case empower_e::EMPOWER_4:
        return 3250_ms;
      default:
        break;
    }

    return 0_ms;
  }

  timespan_t max_hold_time() const
  {
    // TODO: confirm if this is affected by duration mods/haste
    return base_time_to_empower( ab::max_empower ) + 2_s;
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    // we need to have the tick time match duration.
    // NOTE: composite_dot_duration CANNOT reference parent method as spell_t::composite_dot_duration calls tick_time()
    return composite_dot_duration( s );
  }

  timespan_t base_composite_dot_duration( const action_state_t* s ) const
  {
    return ab::dot_duration * s->haste * ab::get_buff_effects_value( ab::dot_duration_buffeffects );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    // NOTE: DO NOT reference parent method as spell_t::composite_dot_duration calls tick_time()
    auto dur = base_composite_dot_duration( s );

    // hack so we always have a non-zero duration in order to trigger last_tick()
    if ( dur == 0_ms )
      return 1_ms;

    return dur + lag;
  }

  timespan_t composite_time_to_empower( const action_state_t* s, empower_e emp ) const
  {
    auto base = base_time_to_empower( emp );
    auto mult = base_composite_dot_duration( s ) / base_empower_duration;

    return base * mult;
  }

  empower_e empower_level( const dot_t* d ) const
  {
    auto emp = empower_e::EMPOWER_NONE;

    if ( !d->is_ticking() )
      return emp;

    auto s       = d->state;
    auto elapsed = tick_time( s ) - d->time_to_next_full_tick();

    if ( elapsed >= composite_time_to_empower( s, empower_e::EMPOWER_4 ) )
      emp = empower_e::EMPOWER_4;
    else if ( elapsed >= composite_time_to_empower( s, empower_e::EMPOWER_3 ) )
      emp = empower_e::EMPOWER_3;
    else if ( elapsed >= composite_time_to_empower( s, empower_e::EMPOWER_2 ) )
      emp = empower_e::EMPOWER_2;
    else if ( elapsed >= composite_time_to_empower( s, empower_e::EMPOWER_1 ) )
      emp = empower_e::EMPOWER_1;

    return std::min( ab::max_empower, emp );
  }

  void init() override
  {
    ab::init();
    assert( release_spell && "Empowered charge spell must have a release spell." );
  }

  void execute() override
  {
    // pre-determine lag here per every execute
    lag = ab::rng().gauss( ab::sim->channel_lag, ab::sim->channel_lag_stddev );

    ab::execute();
  }

  void tick( dot_t* d ) override
  {
    // For proper DPET analysis, we need to treat charge spells as non-channel, since channelled spells sum up tick
    // times to get the execute time, but this does not work for fire breath which also has a dot component. Instead we
    // hijack the stat obj during action_t:tick() causing the channel's tick to be recorded onto a throwaway stat obj.
    // We then record the corresponding tick time as execute time onto the original real stat obj. See further notes in
    // evoker_t::analyze().
    ab::stats = dummy_stat;
    ab::tick( d );
    ab::stats = orig_stat;

    ab::stats->iteration_total_execute_time += d->time_to_tick();
  }

  virtual bool validate_release_target( dot_t* )
  {
    return true;
  }

  void last_tick( dot_t* d ) override
  {
    ab::last_tick( d );

    // being stunned ends the empower without triggering the release spell
    if ( ab::p()->buffs.stunned->check() )
    {
      ab::p()->was_empowering = false;
      return;
    }

    if ( empower_level( d ) == empower_e::EMPOWER_NONE || !validate_release_target( d ) )
    {
      ab::p()->was_empowering = false;
      return;
    }

    if ( ab::p()->naszuro )
    {
      if ( ab::p()->get_target_data( ab::p() )->buffs.unbound_surge->check() &&
           ab::p()->option.naszuro_accurate_behaviour )
      {
        ab::p()->bounce_naszuro( ab::p() );
      }
      else
      {
        ab::p()->get_target_data( ab::p() )->buffs.unbound_surge->trigger();
      }
    }

    auto emp_state        = release_spell->get_state();
    emp_state->target     = ab::target;
    release_spell->target = ab::target;
    release_spell->snapshot_state( emp_state, release_spell->amount_type( emp_state ) );

    if ( ab::p()->buff.tip_the_scales->up() )
    {
      ab::p()->buff.tip_the_scales->expire();
      ab::cast_state( emp_state )->empower = ab::max_empower;
    }
    else
      ab::cast_state( emp_state )->empower = empower_level( d );

    release_spell->schedule_execute( emp_state );

    // hack to prevent dot_t::last_tick() from schedule_ready()'ing the player
    d->current_action = release_spell;
    // hack to prevent channel lag being added when player is schedule_ready()'d after the release spell execution
    ab::p()->last_foreground_action = release_spell;
    // Start GCD - All Empowerw have a GCD of 0.5s after completion.
    ab::start_gcd();
  }
};

namespace heals
{
struct evoker_heal_t : public evoker_action_t<heal_t>
{
private:
  using ab = evoker_action_t<heal_t>;

public:
  double scarlet_adaptation_sp_cap;

  evoker_heal_t( std::string_view name, evoker_t* player, const spell_data_t* spell = spell_data_t::nil(),
                 std::string_view options_str = {} )
    : ab( name, player, spell ), scarlet_adaptation_sp_cap( player->spec.living_flame_damage->effectN( 1 ).sp_coeff() )
  {
    parse_options( options_str );
  }

  void assess_damage( result_amount_type rt, action_state_t* s ) override
  {
    ab::assess_damage( rt, s );

    if ( p()->talent.scarlet_adaptation.ok() )
    {
      if ( !p()->buff.scarlet_adaptation->check() )
        p()->buff.scarlet_adaptation->trigger();

      auto& stored = p()->buff.scarlet_adaptation->current_value;
      // TODO: raw_amount for used for testing
      // stored += s->result_amount * p()->talent.scarlet_adaptation->effectN( 1 ).percent();
      stored +=
          s->result_raw * p()->talent.scarlet_adaptation->effectN( 1 ).percent() * ( 1 - p()->option.scarlet_overheal );
      // TODO: confirm if this always matches living flame SP coeff
      stored = std::min( stored, p()->cache.spell_power( SCHOOL_MAX ) * scarlet_adaptation_sp_cap );
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t );

    if ( p()->specialization() == EVOKER_PRESERVATION && t->health_percentage() < p()->health_percentage() )
      tm *= 1.0 + p()->cache.mastery_value();

    return tm;
  }
};

using essence_heal_t = essence_base_t<evoker_heal_t>;

// Empowered Heals ==========================================================
using empowered_charge_heal_t  = empowered_charge_t<evoker_heal_t>;
using empowered_release_heal_t = empowered_release_t<evoker_heal_t>;

// Heals ====================================================================

struct emerald_blossom_t : public essence_heal_t
{
  struct emerald_blossom_heal_t : public evoker_heal_t
  {
    emerald_blossom_heal_t( evoker_t* p, bool do_aoe = true )
      : evoker_heal_t( do_aoe ? "emerald_blossom_heal" : "emerald_blossom_virtual_heal", p,
                       p->spec.emerald_blossom_heal )
    {
      harmful = false;
      dual    = true;
      if ( do_aoe )
        aoe = as<int>( data().effectN( 2 ).base_value() ) +
              as<int>( p->talent.bountiful_bloom->effectN( 1 ).base_value() );
    }
  };

  struct panacea_t : public evoker_heal_t
  {
    panacea_t( evoker_t* p ) : evoker_heal_t( "panacea", p, p->talent.panacea_spell )
    {
      harmful = false;
      dual    = true;
      target  = p;
    }
  };

  action_t *heal, *panacea, *virtual_heal;

  timespan_t extend_ebon;

  emerald_blossom_t( evoker_t* p, std::string_view options_str )
    : essence_heal_t( "emerald_blossom", p, p->spec.emerald_blossom, options_str )
  {
    harmful      = false;
    heal         = p->get_secondary_action<emerald_blossom_heal_t>( "emerald_blossom_heal" );
    virtual_heal = p->get_secondary_action<emerald_blossom_heal_t>( "emerald_blossom_virtual_heal", false );
    panacea      = p->get_secondary_action<panacea_t>( "panacea" );

    min_travel_time = data().duration().total_seconds();

    extend_ebon = timespan_t::from_seconds( p->talent.dream_of_spring->effectN( 2 ).base_value() );

    add_child( heal );
    add_child( panacea );
  }

  void impact( action_state_t* s ) override
  {
    essence_heal_t::impact( s );

    heal->execute_on_target( s->target );

    auto tl_size = heal->target_list().size();
    if ( heal->aoe > tl_size )
    {
      tl_size = heal->aoe - tl_size;
      for ( size_t i = 0; i < tl_size; i++ )
      {
        virtual_heal->execute_on_target( s->target );
      }
    }
  }

  void consume_resource() override
  {
    essence_heal_t::consume_resource();

    resource_e prev_cr = current_resource();

    if ( prev_cr == RESOURCE_MANA )
      return;

    resource_e cr = resource_current = RESOURCE_MANA;

    if ( base_cost() == 0 || proc )
      return;

    last_resource_cost = cost();

    player->resource_loss( cr, last_resource_cost, nullptr, this );

    sim->print_log( "{} consumes {} {} for {} ({})", *player, last_resource_cost, cr, *this,
                    player->resources.current[ cr ] );

    stats->consume_resource( cr, last_resource_cost );

    resource_current = prev_cr;
  }

  bool ready() override
  {
    if ( !essence_heal_t::ready() )
      return false;

    if ( resource_current == RESOURCE_MANA )
      return true;

    auto prev_resource = resource_current;
    resource_current   = RESOURCE_MANA;

    if ( !player->resource_available( RESOURCE_MANA, cost() ) )
    {
      if ( starved_proc )
        starved_proc->occur();

      resource_current = prev_resource;
      return false;
    }

    resource_current = prev_resource;
    return true;
  }

  void execute() override
  {
    essence_heal_t::execute();

    if ( extend_ebon > timespan_t::zero() )
      p()->extend_ebon( extend_ebon );

    if ( p()->talent.ancient_flame->ok() )
      p()->buff.ancient_flame->trigger();

    if ( p()->talent.panacea.ok() )
      panacea->execute_on_target( p() );
  }
};

struct verdant_embrace_t : public evoker_heal_t
{
  struct verdant_embrace_heal_t : public evoker_heal_t
  {
    verdant_embrace_heal_t( evoker_t* p ) : evoker_heal_t( "verdant_embrace_heal", p, p->find_spell( 361195 ) )
    {
      harmful = false;
      dual    = true;
    }
  };

  verdant_embrace_t( evoker_t* p, std::string_view options_str )
    : evoker_heal_t( "verdant_embrace", p, p->talent.verdant_embrace, options_str )
  {
    harmful       = false;
    impact_action = p->get_secondary_action<verdant_embrace_heal_t>( "verdant_embrace_heal" );

    add_child( impact_action );
  }

  void execute() override
  {
    evoker_heal_t::execute();

    if ( p()->talent.ancient_flame->ok() )
      p()->buff.ancient_flame->trigger();
  }
};

}  // namespace heals

namespace spells
{
struct evoker_spell_t : public evoker_action_t<spell_t>
{
private:
  using ab = evoker_action_t<spell_t>;

public:
  evoker_spell_t( std::string_view name, evoker_t* player, const spell_data_t* spell = spell_data_t::nil(),
                  std::string_view options_str = {} )
    : ab( name, player, spell )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    ab::execute();

    if ( !ab::background && !ab::dual )
    {
      // These happen after any secondary spells are executed, so we schedule as events
      if ( spell_color == SPELL_BLUE )
        make_event( *sim, [ this ]() { p()->buff.iridescence_blue->decrement(); } );
      else if ( spell_color == SPELL_RED )
        make_event( *sim, [ this ]() { p()->buff.iridescence_red->decrement(); } );
    }
  }

  virtual void trigger_charged_blast( action_state_t* s )
  {
    if ( spell_color == SPELL_BLUE && has_amount_result() && result_is_hit( s->result ) )
      p()->buff.charged_blast->trigger();
  }

  virtual void trigger_everburning_flame( action_state_t* s )
  {
    if ( s->chain_target == 0 && s->result_amount > 0 && s->result_type == result_amount_type::DMG_DIRECT &&
         spell_color == SPELL_RED )
    {
      if ( p()->talent.everburning_flame.ok() )
      {
        auto ext = timespan_t::from_seconds( as<int>( p()->talent.everburning_flame->effectN( 1 ).base_value() ) );

        for ( auto t : sim->target_non_sleeping_list )
          td( t )->dots.fire_breath->adjust_duration( ext );
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    trigger_charged_blast( s );
    trigger_everburning_flame( s );
  }

  void tick( dot_t* d ) override
  {
    ab::tick( d );

    trigger_charged_blast( d->state );
  }

  virtual bool use_full_mastery() const
  {
    return p()->talent.tyranny.ok() && p()->buff.dragonrage->check();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t );

    // Preliminary testing shows this is linear with target hp %.
    // TODO: confirm this applies only to all evoker offensive spells
    if ( p()->specialization() == EVOKER_DEVASTATION )
    {
      if ( use_full_mastery() )
        tm *= 1.0 + p()->cache.mastery_value();
      else
        tm *= 1.0 + p()->cache.mastery_value() * t->health_percentage() / 100;
    }

    return tm;
  }
};

using essence_spell_t = essence_base_t<evoker_spell_t>;

// Empowered Spells =========================================================
struct empowered_release_spell_t : public empowered_release_t<evoker_spell_t>
{
  using base_t = empowered_release_spell_t;

  empowered_release_spell_t( std::string_view n, evoker_t* p, const spell_data_t* s ) : empowered_release_t( n, p, s )
  {
  }

  void execute() override
  {
    empowered_release_t::execute();

    if ( background )
      return;

    p()->buff.limitless_potential->trigger();

    if ( p()->talent.animosity.ok() )
    {
      p()->buff.dragonrage->extend_duration( p(), p()->talent.animosity->effectN( 1 ).time_value() );
    }

    p()->buff.power_swell->trigger();

    if ( spell_color == SPELL_BLUE )
      p()->buff.iridescence_blue->trigger();
    else if ( spell_color == SPELL_RED )
      p()->buff.iridescence_red->trigger();

    if ( rng().roll( p()->sets->set( EVOKER_DEVASTATION, T29, B4 )->effectN( 2 ).percent() ) )
    {
      if ( p()->buffs.bloodlust->check() )
        p()->buffs.bloodlust->extend_duration( p(), extend_tier29_4pc );
      else if ( p()->buff.fury_of_the_aspects->check() )
        p()->buff.fury_of_the_aspects->extend_duration( p(), extend_tier29_4pc );
      else
        p()->buff.fury_of_the_aspects->trigger( extend_tier29_4pc );
    }

    p()->buff.blazing_shards->trigger();
  }
};

struct empowered_charge_spell_t : public empowered_charge_t<evoker_spell_t>
{
  using base_t = empowered_charge_spell_t;

  empowered_charge_spell_t( std::string_view n, evoker_t* p, const spell_data_t* s, std::string_view o )
    : empowered_charge_t( n, p, s, o )
  {
  }

  bool validate_release_target( dot_t* d ) override
  {
    auto t = d->state->target;

    if ( t->is_sleeping() )
    {
      t = nullptr;

      for ( auto enemy : p()->sim->target_non_sleeping_list )
      {
        if ( enemy->is_sleeping() || ( enemy->debuffs.invulnerable && enemy->debuffs.invulnerable->check() ) )
          continue;

        t = enemy;
        break;
      }
    }

    if ( !t )
      return false;
    else
      return true;
  }
};

struct sands_of_time_state_t
{
  bool sands_crit;
};

struct ebon_might_t : public evoker_augment_t
{
protected:
  using state_t = evoker_action_state_t<sands_of_time_state_t>;

public:
  double base_ebon_value        = 0.0;
  double clutchmates_ebon_value = 0.0;
  timespan_t ebon_time          = timespan_t::min();
  mutable std::vector<player_t*> secondary_list, tertiary_list;

  ebon_might_t( evoker_t* p, std::string_view options_str )
    : ebon_might_t( p, "ebon_might", options_str, timespan_t::min() )
  {
  }

  ebon_might_t( evoker_t* p, timespan_t ebon, std::string_view name ) : ebon_might_t( p, name, {}, ebon )
  {
  }

  ebon_might_t( evoker_t* p, std::string_view name, std::string_view options_str, timespan_t ebon )
    : evoker_augment_t( name, p, p->talent.ebon_might, options_str ),
      secondary_list(),
      tertiary_list(),
      ebon_time( ebon )
  {
    // Add a target so you always hit yourself.
    aoe += 1;
    dot_duration = base_tick_time = 0_ms;

    cooldown->base_duration = 0_s;

    parse_effect_modifiers( p->sets->set( EVOKER_AUGMENTATION, T30, B4 ) );

    base_ebon_value = modified_effect( 1 ).percent();

    parse_effect_modifiers( p->spec.close_as_clutchmates );

    clutchmates_ebon_value = modified_effect( 1 ).percent();
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  state_t* cast_state( action_state_t* s )
  {
    return static_cast<state_t*>( s );
  }

  double ebon_value() const
  {
    return p()->close_as_clutchmates ? clutchmates_ebon_value : base_ebon_value;
  }

  const state_t* cast_state( const action_state_t* s ) const
  {
    return static_cast<const state_t*>( s );
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    evoker_augment_t::snapshot_state( s, rt );
    cast_state( s )->sands_crit = rng().roll( p()->cache.spell_crit_chance() );
  }

  double ebon_int()
  {
    if ( p()->allied_ebons_on_me.empty() )
      return p()->cache.intellect() * ebon_value();

    double ignore_int = 0;

    for ( auto b : p()->allied_ebons_on_me )
    {
      ignore_int += debug_cast<stat_buff_t*>( b )->stats[ 0 ].amount;
    }

    ignore_int *= p()->composite_attribute_multiplier( ATTR_INTELLECT );

    return ( p()->cache.intellect() - ignore_int ) * ebon_value();
  }

  void extend_ebon( timespan_t extend )
  {
    if ( extend <= 0_s )
      return;

    if ( rng().roll( p()->cache.spell_crit_chance() ) )
      extend *= 1 + p()->talent.sands_of_time->effectN( 4 ).percent();

    if ( p()->buff.ebon_might_self_buff->check() )
    {
      p()->buff.ebon_might_self_buff->extend_duration( p(), extend );
    }

    for ( auto ally : p()->allies_with_my_ebon )
    {
      auto ebon = p()->get_target_data( ally )->buffs.ebon_might;

      ebon->extend_duration( p(), extend );
    }
  }

  void update_stat( stat_buff_t* ebon, double _ebon_int )
  {
    if ( ebon->check() && ebon->stats[ 0 ].amount != _ebon_int )
    {
      ebon->stats[ 0 ].amount = _ebon_int;
      adjust_int( ebon );
    }
  }

  void update_stats()
  {
    auto _ebon_int = ebon_int();

    for ( auto ally : p()->allies_with_my_ebon )
    {
      auto ebon = p()->get_target_data( ally )->buffs.ebon_might;
      update_stat( ebon, _ebon_int );
    }
  }

  void adjust_int( stat_buff_t* b )
  {
    for ( auto& buff_stat : b->stats )
    {
      if ( buff_stat.check_func && !buff_stat.check_func( *b ) )
        continue;

      double delta = buff_stat.stack_amount( b->current_stack ) - buff_stat.current_value;
      if ( delta > 0 )
      {
        b->player->stat_gain( buff_stat.stat, delta, b->stat_gain, nullptr, b->buff_duration() > timespan_t::zero() );
      }
      else if ( delta < 0 )
      {
        b->player->stat_loss( buff_stat.stat, std::fabs( delta ), b->stat_gain, nullptr,
                              b->buff_duration() > timespan_t::zero() );
      }

      buff_stat.current_value += delta;
    }
  }

  void ebon_on_target( player_t* t, bool crit )
  {
    if ( t->is_enemy() )
    {
      sim->error( "{} Attempted to cast Ebon Might on {}, an enemy.", *p(), *t );
      return;
    }

    buff_t* buff;

    if ( t == p() )
      buff = p()->buff.ebon_might_self_buff;
    else
    {
      buff = p()->get_target_data( t )->buffs.ebon_might;
    }

    bool new_cast = !buff->check();
    if ( ebon_time <= timespan_t::zero() || new_cast )
    {
      buff->trigger( ebon_time );
      if ( t != p() && new_cast )
        update_stat( debug_cast<stat_buff_t*>( buff ), ebon_int() );
    }
    else
    {
      auto time = ebon_time;
      if ( crit )
        time *= 1 + p()->talent.sands_of_time->effectN( 4 ).percent();
      buff->extend_duration( p(), time );
    }
  }

  void execute() override
  {
    sim->print_debug( "{} before cast {} allies with prescience: {} allies with ebon: {} n_targets: {}", *p(), *this,
                      p()->allies_with_my_prescience.size(), p()->allies_with_my_ebon.size(), n_targets() );

    // Debug for Ebon Might - Need to check if there's a bug
    if ( sim->debug )
    {
      std::string _str = "";

      for ( auto& _p : sim->player_no_pet_list )
      {
        auto td = p()->get_target_data( _p );

        if ( td->buffs.ebon_might->up() )
        {
          if ( td->buffs.prescience->up() )
          {
            _str += "B ";
          }
          else
          {
            _str += "E ";
          }
        }
        else if ( td->buffs.prescience->up() )
        {
          _str += "P ";
        }
        else
        {
          _str += "X ";
        }
      }

      sim->print_debug( "{} {}", *p(), _str );

      for ( auto& _e : p()->allied_augmentations )
      {
        _str = "";

        for ( auto& _p : sim->player_no_pet_list )
        {
          auto td = _e->get_target_data( _p );

          if ( td->buffs.ebon_might->up() )
          {
            if ( td->buffs.prescience->up() )
            {
              _str += "B ";
            }
            else
            {
              _str += "E ";
            }
          }
          else if ( td->buffs.prescience->up() )
          {
            _str += "P ";
          }
          else
          {
            _str += "X ";
          }
        }
        sim->print_debug( "{} {}", *_e, _str );
      }
    }

    evoker_augment_t::execute();

    sim->print_debug( "{} casts {} allies with prescience: {} allies with ebon: {} n_targets: {}", *p(), *this,
                      p()->allies_with_my_prescience.size(), p()->allies_with_my_ebon.size(), n_targets() );
  }

  void impact( action_state_t* s ) override
  {
    evoker_augment_t::impact( s );

    if ( s->chain_target == 0 )
    {
      for ( auto t : p()->allies_with_my_ebon )
      {
        ebon_on_target( t, cast_state( s )->sands_crit );
      }
    }

    ebon_on_target( s->target, cast_state( s )->sands_crit );
  }

  int n_targets() const override
  {
    return aoe - as<int>( p()->allies_with_my_ebon.size() );
  }

  void activate() override
  {
    evoker_augment_t::activate();
    p()->allies_with_my_ebon.register_callback( [ this ]( player_t* ) { target_cache.is_valid = false; } );
    p()->allies_with_my_prescience.register_callback( [ this ]( player_t* ) { target_cache.is_valid = false; } );

    for ( auto e : p()->allied_augmentations )
    {
      e->allied_ebon_callbacks.push_back( [ this ]() { target_cache.is_valid = false; } );
    }
  }

  size_t available_targets( std::vector<player_t*>& target_list ) const override
  {
    target_list.clear();
    // Player must always be the first target.
    target_list.push_back( player );

    for ( auto& p : p()->allies_with_my_prescience )
    {
      if ( p != player )
        target_list.push_back( p );
    }

    // Clear helper vectors used to process in a single pass.
    secondary_list.clear();
    tertiary_list.clear();

    for ( const auto& t : sim->player_no_pet_list )
    {
      if ( t != player && !p()->get_target_data( t )->buffs.ebon_might->up() )
      {
        if ( range::find( p()->allies_with_my_prescience, t ) == p()->allies_with_my_prescience.end() )
        {
          if ( std::none_of( p()->allied_augmentations.begin(), p()->allied_augmentations.end(),
                             [ t ]( evoker_t* e ) { return e->get_target_data( t )->buffs.ebon_might->up(); } ) )
          {
            if ( t->role != ROLE_HYBRID && t->role != ROLE_HEAL && t->role != ROLE_TANK )
              target_list.push_back( t );
            else
              secondary_list.push_back( t );
          }
          else
          {
            tertiary_list.push_back( t );
          }
        }
      }
    }

    if ( target_list.size() < n_targets() )
    {
      for ( auto& t : secondary_list )
      {
        target_list.push_back( t );
        if ( target_list.size() >= n_targets() )
          break;
      }
    }

    if ( target_list.size() < n_targets() )
    {
      for ( auto& t : tertiary_list )
      {
        target_list.push_back( t );
        if ( target_list.size() >= n_targets() )
          break;
      }
    }

    return target_list.size();
  }
};

struct fire_breath_t : public empowered_charge_spell_t
{
  struct fire_breath_damage_t : public empowered_release_spell_t
  {
    timespan_t dot_dur_per_emp;

    fire_breath_damage_t( evoker_t* p )
      : base_t( "fire_breath_damage", p, p->find_spell( 357209 ) ), dot_dur_per_emp( 6_s )
    {
      aoe                 = -1;  // TODO: actually a cone so we need to model it if possible
      reduced_aoe_targets = 5.0;

      dot_duration = 20_s;  // base * 10? or hardcoded to 20s?
      dot_duration += timespan_t::from_seconds( p->talent.blast_furnace->effectN( 1 ).base_value() );
    }

    timespan_t reduction_from_empower( const action_state_t* s ) const
    {
      return std::max( 0, empower_value( s ) - 1 ) * dot_dur_per_emp;
    }

    timespan_t composite_dot_duration( const action_state_t* s ) const override
    {
      return base_t::composite_dot_duration( s ) - reduction_from_empower( s );
    }

    double bonus_da( const action_state_t* s ) const override
    {
      auto da          = base_t::bonus_da( s );
      auto ticks       = reduction_from_empower( s ) / tick_time( s );
      auto tick_damage = s->composite_spell_power() * spell_tick_power_coefficient( s );

      return da + ticks * tick_damage;
    }

    void trigger_everburning_flame( action_state_t* ) override
    {
      return;  // flame breath can't extend itself. TODO: confirm if this ever becomes a possiblity.
    }

    void execute() override
    {
      base_t::execute();

      p()->buff.leaping_flames->trigger( empower_value( execute_state ) );

      if ( p()->talent.infernos_blessing.ok() )
      {
        if ( p()->buff.ebon_might_self_buff->check() )
        {
          p()->get_target_data( p() )->buffs.infernos_blessing->trigger();

          for ( auto a : p()->allies_with_my_ebon )
          {
            p()->get_target_data( a )->buffs.infernos_blessing->trigger();
          }
        }
      }
    }

    timespan_t tick_time( const action_state_t* state ) const override
    {
      timespan_t t = base_t::tick_time( state );

      if ( p()->talent.catalyze.ok() && p()->get_target_data( state->target )->dots.disintegrate->is_ticking() )
      {
        t /= ( 1 + p()->talent.catalyze->effectN( 1 ).percent() );
      }

      return t;
    }

    double calculate_tick_amount( action_state_t* s, double m ) const override
    {
      auto n = std::clamp( as<double>( s->n_targets ), reduced_aoe_targets, 20.0 );

      m *= std::sqrt( reduced_aoe_targets / n );

      return base_t::calculate_tick_amount( s, m );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      auto m = base_t::composite_da_multiplier( s );

      if ( p()->talent.eye_of_infinity.enabled() )
      {
        m *= 1 + p()->talent.eye_of_infinity->effectN( 1 ).percent();
      }

      return m;
    }

    void tick( dot_t* d ) override
    {
      empowered_release_spell_t::tick( d );

      // TODO: confirm this doesn't have a target # based DR, or exhibit previously bugged behavior where icd is
      // triggered on check, not success
      p()->buff.burnout->trigger();
    }
  };

  fire_breath_t( evoker_t* p, std::string_view options_str )
    : base_t( "fire_breath", p, p->find_class_spell( "Fire Breath" ), options_str )
  {
    create_release_spell<fire_breath_damage_t>( "fire_breath_damage" );
  }
};

struct eternity_surge_t : public empowered_charge_spell_t
{
  struct eternity_surge_damage_t : public empowered_release_spell_t
  {
    double eoi_ess;  // essence gain on crit from Eye of Infinity

    eternity_surge_damage_t( evoker_t* p, std::string_view name )
      : base_t( name, p, p->find_spell( 359077 ) ),
        eoi_ess( p->talent.eye_of_infinity->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_ESSENCE ) )
    {
    }

    eternity_surge_damage_t( evoker_t* p ) : eternity_surge_damage_t( p, "eternity_surge_damage" )
    {
    }

    int n_targets() const override
    {
      return n_targets( pre_execute_state );
    }

    int n_targets( action_state_t* s ) const
    {
      int n = s ? empower_value( s ) : max_empower;

      n *= as<int>( 1 + p()->talent.eternitys_span->effectN( 2 ).percent() );

      return n == 1 ? 0 : n;
    }

    void impact( action_state_t* s ) override
    {
      base_t::impact( s );

      if ( eoi_ess && s->result == RESULT_CRIT )
        p()->resource_gain( RESOURCE_ESSENCE, eoi_ess, p()->gain.eye_of_infinity );
    }
  };

  eternity_surge_t( evoker_t* p, std::string_view options_str )
    : base_t( "eternity_surge", p, p->talent.eternity_surge, options_str )
  {
    create_release_spell<eternity_surge_damage_t>( "eternity_surge_damage" );
  }
};

// Spells ===================================================================
struct azure_strike_t : public evoker_spell_t
{
  azure_strike_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "azure_strike", p, p->find_class_spell( "Azure Strike" ), options_str )
  {
    aoe = as<int>( data().effectN( 1 ).base_value() + p->talent.protracted_talons->effectN( 1 ).base_value() );
  }

  void execute() override
  {
    evoker_spell_t::execute();

    if ( p()->talent.azure_essence_burst.ok() &&
         ( p()->buff.dragonrage->up() || rng().roll( p()->talent.azure_essence_burst->effectN( 1 ).percent() ) ) )
    {
      p()->buff.essence_burst->trigger();
      p()->proc.azure_essence_burst->occur();
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    auto da = evoker_spell_t::composite_da_multiplier( s );

    da *= 1.0 + p()->buff.iridescence_blue->check_value();

    return da;
  }

  void impact( action_state_t* s ) override
  {
    evoker_spell_t::impact( s );

    if ( p()->talent.echoing_strike.ok() && s->chain_target == 0 &&
         rng().roll( p()->talent.echoing_strike->effectN( 1 ).percent() * s->n_targets ) )
    {
      execute_on_target( s->target );
      p()->proc.echoing_strike->occur();
    }
  }
};

struct deep_breath_t : public evoker_spell_t
{
  struct deep_breath_dot_t : public evoker_spell_t
  {
    deep_breath_dot_t( evoker_t* p ) : evoker_spell_t( "deep_breath_dot", p, p->find_spell( 353759 ) )
    {
      aoe = -1;
    }

    bool use_full_mastery() const override
    {
      return p()->talent.tyranny.ok();
    }

    void impact( action_state_t* s ) override
    {
      evoker_spell_t::impact( s );
    }

    void execute() override
    {
      evoker_spell_t::execute();

      if ( p()->talent.imminent_destruction->ok() )
      {
        p()->buff.imminent_destruction->trigger();
      }
    }
  };

  action_t* damage;
  action_t* ebon;

  deep_breath_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "deep_breath", p,
                      p->talent.breath_of_eons.ok() ? spell_data_t::not_found() : p->find_class_spell( "Deep Breath" ),
                      options_str ),
      damage( nullptr ),
      ebon( nullptr )
  {
    damage        = p->get_secondary_action<deep_breath_dot_t>( "deep_breath_dot" );
    damage->stats = stats;

    travel_delay = 0.9;   // guesstimate, TODO: confirm
    travel_speed = 19.5;  // guesstimate, TODO: confirm

    trigger_gcd = 3_s;
    gcd_type    = gcd_haste_type::NONE;

    if ( p->specialization() == EVOKER_AUGMENTATION )
      ebon = p->get_secondary_action<ebon_might_t>(
          "ebon_might_deep_breath", p->talent.sands_of_time->effectN( 3 ).time_value(), "ebon_might_deep_breath" );
  }

  void impact( action_state_t* s ) override
  {
    evoker_spell_t::impact( s );

    if ( s->chain_target == 0 )
      damage->execute_on_target( s->target );
  }

  void execute() override
  {
    evoker_spell_t::execute();

    if ( ebon )
      ebon->execute();

    if ( is_precombat )
    {
      start_gcd();
      player->last_foreground_action = this;
      auto delay                     = std::max(
          0_s, rng().gauss( p()->option.prepull_deep_breath_delay, p()->option.prepull_deep_breath_delay_stddev ) );
      player->gcd_ready = delay;
      stats->iteration_total_execute_time += delay;
    }
  }
};

struct disintegrate_t : public essence_spell_t
{
  eternity_surge_t::eternity_surge_damage_t* eternity_surge;
  int num_ticks;

  disintegrate_t( evoker_t* p, std::string_view options_str )
    : essence_spell_t( "disintegrate", p,
                       p->talent.eruption.ok() ? spell_data_t::not_found() : p->find_class_spell( "Disintegrate" ),
                       options_str ),
      num_ticks( as<int>( dot_duration / base_tick_time ) + 1 )
  {
    channeled = tick_zero = true;

    auto surge = p->get_secondary_action<eternity_surge_t::eternity_surge_damage_t>( "scintillation", "scintillation" );
    surge->s_data_reporting   = p->talent.scintillation;
    surge->name_str_reporting = "scintillation";
    surge->proc_spell_type    = proc_spell_type_e::SCINTILLATION;
    eternity_surge            = surge;

    // Not in Spell Data, However it would appear to proc class ability related effects. Primarily made to Fix Irideus
    // Fragment Bug - TODO: Review for other class based procs.
    allow_class_ability_procs = true;

    // 25/11/2022 - Override the lag handling for Disintegrate so that it doesn't use channeled ready behavior
    //              In-game tests have shown it is possible to cast after faster than the 250ms channel_lag using a
    //              nochannel macro
    ability_lag        = p->world_lag;
    ability_lag_stddev = p->world_lag_stddev;

    add_child( eternity_surge );
  }

  void execute() override
  {
    // trigger the buffs first so tick-zero can get buffed
    if ( p()->buff.essence_burst->check() )
      p()->buff.essence_burst_titanic_wrath_disintegrate->trigger( num_ticks );

    if ( p()->buff.iridescence_blue->check() )
      p()->buff.iridescence_blue_disintegrate->trigger( num_ticks );

    essence_spell_t::execute();
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    auto ta = essence_spell_t::composite_ta_multiplier( s );

    if ( p()->buff.essence_burst_titanic_wrath_disintegrate->check() )
      ta *= 1.0 + titanic_mul;

    ta *= 1.0 + p()->buff.iridescence_blue_disintegrate->check_value();

    return ta;
  }

  void tick( dot_t* d ) override
  {
    essence_spell_t::tick( d );

    p()->buff.essence_burst_titanic_wrath_disintegrate->decrement();
    p()->buff.iridescence_blue_disintegrate->decrement();

    if ( p()->talent.scintillation.ok() && rng().roll( p()->talent.scintillation->effectN( 2 ).percent() ) )
    {
      auto emp_state    = eternity_surge->get_state();
      emp_state->target = d->state->target;
      eternity_surge->snapshot_state( emp_state, eternity_surge->amount_type( emp_state ) );
      emp_state->persistent_multiplier *= p()->talent.scintillation->effectN( 1 ).percent();
      eternity_surge->cast_state( emp_state )->empower = EMPOWER_1;

      eternity_surge->schedule_execute( emp_state );
    }

    if ( p()->action.obsidian_shards )
    {
      residual_action::trigger( p()->action.obsidian_shards, d->state->target,
                                d->state->result_amount * obsidian_shards_mul );
    }

    if ( p()->talent.causality.ok() )
    {
      auto cdr = p()->talent.causality->effectN( 1 ).time_value();
      p()->cooldown.eternity_surge->adjust( cdr );
      p()->cooldown.fire_breath->adjust( cdr );
    }
  }
};

struct expunge_t : public evoker_spell_t
{
  expunge_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "expunge", p, p->talent.expunge, options_str )
  {
  }
};

struct firestorm_t : public evoker_spell_t
{
  struct firestorm_tick_t : public evoker_spell_t
  {
    firestorm_tick_t( evoker_t* p, std::string_view n ) : evoker_spell_t( n, p, p->find_spell( 369374 ) )
    {
      dual = ground_aoe = true;
      aoe               = -1;

      apply_affecting_aura( p->talent.raging_inferno );
    }

    double composite_persistent_multiplier( const action_state_t* s ) const override
    {
      auto pm = evoker_spell_t::composite_persistent_multiplier( s );

      pm *= 1.0 + p()->buff.snapfire->check_value();
      pm *= 1.0 + p()->buff.iridescence_red->check_value();

      return pm;
    }
  };

  action_t* damage;

  firestorm_t( evoker_t* p, std::string_view options_str ) : firestorm_t( p, "firestorm", false, options_str )
  {
  }

  firestorm_t( evoker_t* p, std::string_view n, bool ftf, std::string_view options_str = {} )
    : evoker_spell_t( n, p, ftf ? p->find_spell( 368847 ) : p->talent.firestorm, options_str )
  {
    damage        = p->get_secondary_action<firestorm_tick_t>( name_str + "_tick", name_str + "_tick" );
    damage->stats = stats;
  }

  void impact( action_state_t* s ) override
  {
    evoker_spell_t::impact( s );

    make_event<ground_aoe_event_t>(
        *sim, p(),
        ground_aoe_params_t()
            .target( s->target )
            .pulse_time( 2_s )  // from text description, not in spell data. TODO: confirm
            .duration( data().effectN( 1 ).trigger()->duration() )
            .action( damage )
            .state_callback( [ this ]( ground_aoe_params_t::state_type s, ground_aoe_event_t* e ) {
              if ( s == ground_aoe_params_t::state_type::EVENT_STARTED )
              {
                for ( player_t* t : e->params->action()->target_list() )
                {
                  auto td = p()->get_target_data( t );
                  td->debuffs.in_firestorm->increment();
                }
              }
              else if ( s == ground_aoe_params_t::state_type::EVENT_STOPPED )
              {
                for ( player_t* t : e->params->action()->target_list() )
                {
                  auto td = p()->get_target_data( t );
                  td->debuffs.in_firestorm->decrement();
                }
              }
            } ),
        true );

    p()->buff.snapfire->expire();
  }
};

struct hover_t : public evoker_spell_t
{
  hover_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "hover", p, p->find_class_spell( "Hover" ), options_str )
  {
  }

  void execute() override
  {
    evoker_spell_t::execute();

    p()->buff.hover->trigger();
  }
};

struct landslide_t : public evoker_spell_t
{
  landslide_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "landslide", p, p->talent.landslide, options_str )
  {
    // TODO: root NYI
  }
};

struct living_flame_t : public evoker_spell_t
{
  template <class Base>

  struct living_flame_base_t : public Base
  {
    using base_t = living_flame_base_t<Base>;

    timespan_t prepull_timespent;

    living_flame_base_t( std::string_view n, evoker_t* p, const spell_data_t* s )
      : Base( n, p, s ), prepull_timespent( timespan_t::zero() )
    {
      base_t::dual         = true;
      base_t::dot_duration = p->talent.ruby_embers.ok() ? base_t::dot_duration : 0_ms;
    }

    int n_targets() const override
    {
      if ( auto n = base_t::p()->buff.leaping_flames->check() )
        return 1 + n;
      else
        return Base::n_targets();
    }

    timespan_t travel_time() const override
    {
      if ( prepull_timespent == timespan_t::zero() )
        return Base::travel_time();

      // for each additional spell in precombat apl, reduce the travel time by the cast time
      base_t::player->invalidate_cache( CACHE_SPELL_HASTE );
      return std::max( 1_ms, Base::travel_time() - prepull_timespent * base_t::composite_haste() );
    }

    std::vector<player_t*>& target_list() const override
    {
      auto& tl = Base::target_list();

      if ( base_t::is_aoe() && as<int>( tl.size() ) > base_t::n_targets() )
      {
        // always hit the target, so if it exists make sure it's first
        auto start_it = tl.begin() + ( tl[ 0 ] == base_t::target ? 1 : 0 );

        // randomize remaining targets
        base_t::rng().shuffle( start_it, tl.end() );
      }

      return tl;
    }

    void impact( action_state_t* s ) override
    {
      Base::impact( s );

      base_t::p()->buff.snapfire->trigger();
    }
  };

  struct living_flame_damage_t : public living_flame_base_t<evoker_spell_t>
  {
    living_flame_damage_t( evoker_t* p ) : base_t( "living_flame_damage", p, p->spec.living_flame_damage )
    {
    }

    double bonus_da( const action_state_t* s ) const override
    {
      auto da = base_t::bonus_da( s );

      da += p()->buff.scarlet_adaptation->check_value();

      return da;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      auto da = base_t::composite_da_multiplier( s );

      da *= 1.0 + p()->buff.iridescence_red->check_value();

      return da;
    }
  };

  struct living_flame_heal_t : public living_flame_base_t<heals::evoker_heal_t>
  {
    living_flame_heal_t( evoker_t* p ) : base_t( "living_flame_heal", p, p->spec.living_flame_heal )
    {
    }

    void execute() override
    {
      base_t::execute();
    }
  };

  action_t* damage;
  action_t* heal;
  double gcd_mul;
  bool cast_heal;
  timespan_t prepull_timespent;
  double mana_return;

  living_flame_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "living_flame", p, p->find_class_spell( "Living Flame" ) ),
      gcd_mul( p->find_spelleffect( &p->buff.ancient_flame->data(), A_ADD_PCT_MODIFIER, P_GCD, &data() )->percent() ),
      cast_heal( false ),
      prepull_timespent( timespan_t::zero() ),
      mana_return( p->spec.energizing_flame->effectN( 1 ).percent() )
  {
    damage        = p->get_secondary_action<living_flame_damage_t>( "living_flame_damage" );
    damage->stats = stats;

    // TODO: implement option to cast heal instead
    heal = p->get_secondary_action<living_flame_heal_t>( "living_flame_heal" );

    add_option( opt_bool( "heal", cast_heal ) );
    parse_options( options_str );

    switch ( p->specialization() )
    {
      case EVOKER_DEVASTATION:
        mana_return += p->spec.devastation->effectN( 9 ).percent();
        break;
      case EVOKER_AUGMENTATION:
        mana_return += p->spec.augmentation->effectN( 9 ).percent();
        break;
      case EVOKER_PRESERVATION:
      default:
        break;
    }
  }

  bool has_amount_result() const override
  {
    return damage->has_amount_result();
  }

  timespan_t travel_time() const override
  {
    if ( is_precombat )
      return 1_ms;

    return evoker_spell_t::travel_time();
  }

  timespan_t gcd() const override
  {
    auto g = evoker_spell_t::gcd();

    if ( p()->buff.ancient_flame->check() && !p()->buff.burnout->check() )
      g *= 1.0 + gcd_mul;

    return std::max( min_gcd, g );
  }

  void execute() override
  {
    evoker_spell_t::execute();

    
    auto cr = current_resource();

    // Single child, update children to parent action on each precombat execute

    if ( is_precombat )
    {
      debug_cast<living_flame_damage_t*>( damage )->prepull_timespent = prepull_timespent;
      debug_cast<living_flame_heal_t*>( heal )->prepull_timespent     = prepull_timespent;
    }

    if ( !cast_heal )
    {
      p()->resource_gain( RESOURCE_MANA, cost() * mana_return, p()->gain.energizing_flame, this );
    }

    damage->execute_on_target( target );

    int total_hits = damage->num_targets_hit;

    if ( !p()->buff.burnout->up() )
      p()->buff.ancient_flame->expire();
    p()->buff.scarlet_adaptation->expire();

    if ( p()->buff.leaping_flames->up() && damage->num_targets_hit <= p()->buff.leaping_flames->check() )
    {
      if ( damage->num_targets_hit > 1 )
        p()->buff.leaping_flames->decrement( damage->num_targets_hit - 1 );

      while ( p()->buff.leaping_flames->check() )
      {
        // Leaping flames are hitting extra targets, the primary target is already a leaping flame used.
        p()->buff.leaping_flames->decrement( 1 );
        heal->execute_on_target( p() );
        p()->buff.leaping_flames->decrement( heal->num_targets_hit );
        for ( int i = 0; i < heal->num_targets_hit; i++ )
          if ( rng().roll( p()->option.heal_eb_chance ) )
            total_hits++;
      }
    }

    // Make sure the buff is definitely gone.
    p()->buff.leaping_flames->expire();

    if ( p()->talent.pupil_of_alexstrasza->ok() )
    {
      // TODO: Auto handle dummy cleave values and damage effectiveness
      if ( !damage->target_cache.is_valid )
      {
        damage->available_targets( damage->target_cache.list );
        damage->target_cache.is_valid = true;
      }

      if ( damage->target_cache.list.size() > 1 )
      {
        damage->execute_on_target( damage->target_list()[ 1 ] );
        total_hits++;
      }
    }

    if ( p()->talent.ruby_essence_burst.ok() )
    {
      for ( int i = 0; i < total_hits; i++ )
      {
        if ( p()->buff.dragonrage->up() || rng().roll( p()->talent.ruby_essence_burst->effectN( 1 ).percent() ) )
        {
          p()->buff.essence_burst->trigger();
          p()->proc.ruby_essence_burst->occur();
        }
      }
    }

    if ( p()->buff.burnout->up() )
      p()->buff.burnout->decrement();

    // Reset them after execute
    if ( is_precombat )
    {
      debug_cast<living_flame_damage_t*>( damage )->prepull_timespent = timespan_t::zero();
      debug_cast<living_flame_heal_t*>( heal )->prepull_timespent     = timespan_t::zero();
    }
  }
};

struct obsidian_scales_t : public evoker_spell_t
{
  obsidian_scales_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "obsidian_scales", p, p->talent.obsidian_scales, options_str )
  {
  }
};

struct obsidian_shards_t : public residual_action::residual_periodic_action_t<evoker_spell_t>
{
  double blazing_shards_mul;

  obsidian_shards_t( evoker_t* p )
    : residual_action_t( "obsidian_shards", p, p->find_spell( 409776 ) ),
      blazing_shards_mul( p->sets->set( EVOKER_DEVASTATION, T30, B4 )->effectN( 3 ).percent() )
  {
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    auto ta = residual_action_t::composite_ta_multiplier( s );

    if ( p()->buff.dragonrage->check() || p()->buff.blazing_shards->check() )
      ta *= 1.0 + blazing_shards_mul;

    return ta;
  }

  // Return Spell_t's multiplier as evoker's contains our mastery amp.
  double composite_target_multiplier( player_t* t ) const override
  {
    return spell_t::composite_target_multiplier( t );
  }

  void tick( dot_t* d ) override
  {
    residual_action_t::tick( d );

    p()->benefit.supercharged_shards->update( p()->buff.dragonrage->check() || p()->buff.blazing_shards->check() );
  }
};

struct quell_t : public evoker_spell_t
{
  quell_t( evoker_t* p, std::string_view options_str ) : evoker_spell_t( "quell", p, p->talent.quell, options_str )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = harmful = false;
    ignore_false_positive = use_off_gcd = is_interrupt = true;
  }

  bool target_ready( player_t* t ) override
  {
    if ( !t->debuffs.casting->check() )
      return false;

    return evoker_spell_t::target_ready( t );
  }

  void execute() override
  {
    evoker_spell_t::execute();

    // always assume succes since action cannot be used unless target is casting
    if ( p()->talent.roar_of_exhilaration.ok() )
    {
      p()->resource_gain(
          RESOURCE_ESSENCE,
          p()->talent.roar_of_exhilaration->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_ESSENCE ),
          p()->gain.roar_of_exhilaration );
    }
  }
};

struct shattering_star_t : public evoker_spell_t
{
  shattering_star_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "shattering_star", p, p->talent.shattering_star, options_str )
  {
    aoe = as<int>( data().effectN( 1 ).base_value() * ( 1.0 + p->talent.eternitys_span->effectN( 2 ).percent() ) );
    aoe = ( aoe == 1 ) ? 0 : aoe;
  }

  void execute() override
  {
    evoker_spell_t::execute();

    if ( p()->talent.arcane_vigor.ok() )
    {
      p()->buff.essence_burst->trigger();
    }
  }

  void impact( action_state_t* s ) override
  {
    evoker_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
      td( s->target )->debuffs.shattering_star->trigger();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    auto da = evoker_spell_t::composite_da_multiplier( s );

    da *= 1.0 + p()->buff.iridescence_blue->check_value();

    return da;
  }
};

struct tip_the_scales_t : public evoker_spell_t
{
  tip_the_scales_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "tip_the_scales", p, p->talent.tip_the_scales, options_str )
  {
  }

  void execute() override
  {
    evoker_spell_t::execute();

    p()->buff.tip_the_scales->trigger();
  }
};

struct pyre_data_t
{
  double charged_blast;
  double essence_burst;
  double iridescence;

  friend void sc_format_to( const pyre_data_t& data, fmt::format_context::iterator out )
  {
    fmt::format_to( out, "charged_blast={}, essence_burst={}, iridescence={}", data.charged_blast, data.essence_burst,
                    data.iridescence );
  }
};

struct pyre_t : public essence_spell_t
{
  using state_t = evoker_action_state_t<pyre_data_t>;

  struct pyre_damage_t : public essence_spell_t
  {
    pyre_damage_t( evoker_t* p, std::string_view name_str ) : essence_spell_t( name_str, p, p->find_spell( 357212 ) )
    {
      dual = true;
      aoe  = -1;

      if ( p->talent.raging_inferno->ok() )
        target_multiplier_dotdebuffs.emplace_back(
            []( evoker_td_t* t ) { return t->debuffs.in_firestorm->check() > 0; },
            p->talent.raging_inferno->effectN( 2 ).percent(), false );
    }

    action_state_t* new_state() override
    {
      return new state_t( this, target );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      auto da = essence_spell_t::composite_da_multiplier( s );
      auto s_ = static_cast<const state_t*>( s );

      da *= 1.0 + s_->charged_blast;
      da *= 1.0 + s_->essence_burst;
      da *= 1.0 + s_->iridescence;

      return da;
    }

    void impact( action_state_t* s ) override
    {
      essence_spell_t::impact( s );

      if ( p()->talent.causality.ok() && s->chain_target == 0 )
      {
        auto cdr = std::min( 5u, s->n_targets ) * p()->talent.causality->effectN( 2 ).time_value();
        p()->cooldown.eternity_surge->adjust( cdr );
        p()->cooldown.fire_breath->adjust( cdr );
      }

      if ( p()->action.obsidian_shards )
        residual_action::trigger( p()->action.obsidian_shards, s->target, s->result_amount * obsidian_shards_mul );
    }
  };

  action_t* volatility;
  action_t* damage;
  action_t* firestorm;

  pyre_t( evoker_t* p, std::string_view options_str ) : pyre_t( p, "pyre", p->talent.pyre, options_str )
  {
  }

  pyre_t( evoker_t* p, std::string_view n, const spell_data_t* s, std::string_view o = {} )
    : essence_spell_t( n, p, s, o ), volatility( nullptr )
  {
    damage          = p->get_secondary_action<pyre_damage_t>( name_str + "_damage", name_str + "_damage" );
    firestorm       = p->get_secondary_action<firestorm_t>( "firestorm_ftf", "firestorm_ftf", true );
    firestorm->proc = true;
    damage->stats   = stats;
    damage->proc    = true;
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  void init() override
  {
    essence_spell_t::init();

    if ( p()->talent.volatility->ok() )
    {
      if ( proc_spell_type & proc_spell_type_e::DRAGONRAGE )
        volatility = p()->action.volatility_dragonrage;
      else
        volatility = p()->action.volatility;

      if ( ( proc_spell_type & proc_spell_type_e::VOLATILITY ) == 0 )
        add_child( volatility );
    }
  }

  bool has_amount_result() const override
  {
    return damage->has_amount_result();
  }

  void execute() override
  {
    essence_spell_t::execute();
    p()->buff.charged_blast->expire();
    if ( p()->talent.feed_the_flames.enabled() && ( proc_spell_type & proc_spell_type_e::VOLATILITY ) == 0 )
    {
      p()->buff.feed_the_flames_stacking->trigger();
    }
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    essence_spell_t::snapshot_state( s, rt );

    auto s_ = static_cast<state_t*>( s );

    if ( ( proc_spell_type & proc_spell_type_e::VOLATILITY ) != 0 )
      return;

    s_->charged_blast = p()->buff.charged_blast->check_stack_value();
    s_->essence_burst = p()->buff.essence_burst->check() ? titanic_mul : 0.0;
    s_->iridescence   = p()->buff.iridescence_red->check_value();
  }

  void impact( action_state_t* s ) override
  {
    essence_spell_t::impact( s );

    if ( p()->buff.feed_the_flames_pyre->up() )
    {
      firestorm->execute_on_target( s->target );
      p()->buff.feed_the_flames_pyre->expire();
    }

    // The captured da mul is passed along to the damage action state.
    auto state = damage->get_state();
    state->copy_state( s );
    damage->snapshot_state( state, damage->amount_type( state ) );
    damage->schedule_execute( state );

    // TODO: How many times can volatility chain?
    if ( volatility && rng().roll( p()->talent.volatility->effectN( 1 ).percent() ) )
    {
      const auto& tl = volatility->target_list();
      volatility->execute_on_target( tl[ rng().range( tl.size() ) ] );
    }
  }
};

struct dragonrage_t : public evoker_spell_t
{
  struct dragonrage_damage_t : public pyre_t
  {
    dragonrage_damage_t( evoker_t* p ) : pyre_t( p, "dragonrage_pyre", p->talent.dragonrage->effectN( 2 ).trigger() )
    {
      name_str_reporting = "pyre";
      s_data_reporting   = p->talent.pyre;

      // spell has 30yd range but the effect to launch pyre only has 25yd
      range = data().effectN( 1 ).radius();
      aoe   = as<int>( data().effectN( 1 ).base_value() );

      proc_spell_type = proc_spell_type_e::DRAGONRAGE;
      proc            = true;
    }

    std::vector<player_t*>& target_list() const override
    {
      auto& tl = pyre_t::target_list();

      if ( as<int>( tl.size() ) > n_targets() )
        rng().shuffle( tl.begin(), tl.end() );

      return tl;
    }
  };

  action_t* damage;
  unsigned max_targets;

  dragonrage_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "dragonrage", p, p->talent.dragonrage, options_str ),
      max_targets( as<unsigned>( data().effectN( 2 ).trigger()->effectN( 1 ).base_value() ) )
  {
    if ( !data().ok() )
      return;

    damage = p->get_secondary_action<dragonrage_damage_t>( "dragonrage_pyre" );
    add_child( damage );

    school = damage->school;
  }

  void execute() override
  {
    evoker_spell_t::execute();

    p()->buff.dragonrage->trigger();
    damage->execute_on_target( target );
  }
};

struct eruption_t : public essence_spell_t
{
  timespan_t extend_ebon;
  timespan_t upheaval_cdr;

  eruption_t( evoker_t* p, std::string_view name ) : eruption_t( p, name, {} )
  {
  }

  eruption_t( evoker_t* p, std::string_view name, std::string_view options_str )
    : essence_spell_t( name, p, p->talent.eruption, options_str ),
      extend_ebon( p->talent.sands_of_time->effectN( 1 ).time_value() ),
      upheaval_cdr( p->talent.accretion->effectN( 1 ).trigger()->effectN( 1 ).time_value() )
  {
    split_aoe_damage = true;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double da = essence_spell_t::composite_da_multiplier( s );

    if ( p()->talent.ricocheting_pyroclast->ok() )
    {
      da *= 1 + std::min( static_cast<double>( s->n_targets ),
                          p()->talent.ricocheting_pyroclast->effectN( 2 ).base_value() ) *
                    p()->talent.ricocheting_pyroclast->effectN( 1 ).percent();
    }

    return da;
  }

  void execute() override
  {
    essence_spell_t::execute();

    p()->extend_ebon( extend_ebon );

    if ( p()->talent.accretion->ok() )
    {
      p()->cooldown.upheaval->adjust( upheaval_cdr );
    }

    if ( p()->talent.motes_of_possibility->ok() && rng().roll( p()->talent.motes_of_possibility->proc_chance() ) )
    {
      p()->cooldown.breath_of_eons->adjust(
          -timespan_t::from_seconds( p()->talent.motes_of_possibility->effectN( 1 ).base_value() ) );
    }

    if ( p()->talent.regenerative_chitin->ok() && p()->last_scales_target &&
         p()->get_target_data( p()->last_scales_target )->buffs.blistering_scales->check() )
    {
      p()->get_target_data( p()->last_scales_target )
          ->buffs.blistering_scales->bump( as<int>( p()->talent.regenerative_chitin->effectN( 3 ).base_value() ) );
    }
  }
};

struct upheaval_t : public empowered_charge_spell_t
{
  struct upheaval_damage_t : public empowered_release_spell_t
  {
    upheaval_damage_t( evoker_t* p, std::string_view name ) : base_t( name, p, p->find_spell( 396288 ) )
    {
    }

    upheaval_damage_t( evoker_t* p ) : upheaval_damage_t( p, "upheaval_damage" )
    {
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double da = empowered_release_spell_t::composite_da_multiplier( s );

      if ( p()->talent.tectonic_locus->ok() && s->chain_target == 0 )
      {
        da *= 1 + p()->talent.tectonic_locus->effectN( 1 ).percent();
      }

      return da;
    }
  };

  upheaval_t( evoker_t* p, std::string_view options_str ) : base_t( "upheaval", p, p->talent.upheaval, options_str )
  {
    create_release_spell<upheaval_damage_t>( "upheaval_damage" );
  }
};

struct fate_mirror_damage_t : public evoker_external_action_t<spell_t>
{
protected:
  using base = evoker_external_action_t<spell_t>;

public:
  fate_mirror_damage_t( player_t* p ) : base( "fate_mirror", p, p->find_spell( 404908 ) )
  {
    may_dodge = may_parry = may_block = may_crit = false;
    background                                   = true;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    return cast_state( s )->evoker->talent.fate_mirror->effectN( 1 ).percent();
  }

  void init() override
  {
    spell_t::init();
    snapshot_flags &= STATE_NO_MULTIPLIER & ~STATE_TARGET;
    snapshot_flags |= STATE_MUL_SPELL_DA;
  }
};


struct infernos_blessing_t : public evoker_external_action_t<spell_t>
{
protected:
  using base    = evoker_external_action_t<spell_t>;

public:
  infernos_blessing_t( player_t* p )
    : base( "infernos_blessing", p, p->find_spell( 410265 ) )
  {
    may_dodge = may_parry = may_block = false;
    background                        = true;
    spell_power_mod.direct            = 0.88;  // Hardcoded for some reason, 24/05/2023
  }
};

struct breath_of_eons_damage_t : public evoker_external_action_t<spell_t>
{
protected:
  using base = evoker_external_action_t<spell_t>;

public:
  breath_of_eons_damage_t( player_t* p ) : base( "breath_of_eons_damage", p, p->find_spell( 409632 ) )
  {
    may_dodge = may_parry = may_block = false;
    background                        = true;
    base_crit += 1.0;
    crit_bonus = 0.0;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = p( s )->talent.temporal_wound->effectN( 1 ).percent();

    if ( p( s )->close_as_clutchmates )
      m *= 1 + p( s )->spec.close_as_clutchmates->effectN( 1 ).percent();

    return m;
  }

  void init() override
  {
    spell_t::init();
    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_MUL_SPELL_DA | STATE_CRIT | STATE_TGT_MUL_DA;
  }
};

struct prescience_t : public evoker_augment_t
{
  double anachronism_chance;

  prescience_t( evoker_t* p, std::string_view options_str )
    : evoker_augment_t( "prescience", p, p->talent.prescience, options_str )
  {
    anachronism_chance = p->talent.anachronism->effectN( 1 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    evoker_augment_t::impact( s );

    p()->get_target_data( s->target )->buffs.prescience->trigger();

    if ( is_precombat )
    {
      for ( auto ally : p()->allies_with_my_prescience )
      {
        p()->get_target_data( ally )->buffs.prescience->extend_duration(
            p(), ally == s->target ? -gcd() : -cooldown->cooldown_duration( cooldown ) );
      }
    }
  }

  bool ready() override
  {
    // Do not ever do this out of precombat, the order of entries in allies with my prescience is not preserved. During
    // precombat we can guarantee the first entry is the first buff because we do not allow the sim to cast prescience
    // if it would expire a prescience before the next cast, factoring in a gcds since you need time to make use of your
    // spell.
    if ( is_precombat )
      return p()->allies_with_my_prescience.empty() ||
             p()->get_target_data( p()->allies_with_my_prescience[ 0 ] )->buffs.prescience->remains() >
                 cooldown->cooldown_duration( cooldown ) + gcd() + 100_ms;

    return evoker_augment_t::ready();
  }

  void execute() override
  {
    evoker_augment_t::execute();

    if ( p()->talent.anachronism.ok() && rng().roll( anachronism_chance ) )
    {
      p()->buff.essence_burst->trigger();
      p()->proc.anachronism_essence_burst->occur();
    }
  }
};

struct blistering_scales_damage_t : public evoker_external_action_t<spell_t>
{
private:
  using base = evoker_external_action_t<spell_t>;
protected:
  using state_t = evoker_action_state_t<stats_data_t>;

public:
  blistering_scales_damage_t( player_t* p ) : base( "blistering_scales_damage", p, p->find_spell( 360828 ) )
  {
    may_dodge = may_parry = may_block = false;
    background                        = true;
    aoe                               = -1;
    spell_power_mod.direct            = 0.3;  // Hardcoded for some reason, 19/05/2023
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double da = base::composite_da_multiplier( s );

    da *= 1 + p( s )->buff.reactive_hide->check_stack_value();
    return da;
  }

  void execute() override
  {
    base::execute();

    if ( p( execute_state )->talent.reactive_hide.ok() )
      p( execute_state )->buff.reactive_hide->trigger();
  }
};

struct breath_of_eons_t : public evoker_spell_t
{
  action_t* ebon;
  action_t* eruption;
  timespan_t plot_duration;

  breath_of_eons_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "breath_of_eons", p, p->talent.breath_of_eons, options_str ), ebon( nullptr ), eruption( nullptr )
  {
    travel_delay = 0.9;   // guesstimate, TODO: confirm
    travel_speed = 19.5;  // guesstimate, TODO: confirm

    trigger_gcd = 3_s;
    gcd_type    = gcd_haste_type::NONE;

    aoe = -1;

    if ( p->specialization() == EVOKER_AUGMENTATION )
      ebon = p->get_secondary_action<ebon_might_t>(
          "ebon_might_eons", p->talent.sands_of_time->effectN( 3 ).time_value(), "ebon_might_eons" );

    if ( p->talent.overlord->ok() )
    {
      eruption = p->get_secondary_action<eruption_t>( "eruption_overlord", "eruption_overlord" );
      add_child( eruption );
    }

    plot_duration = timespan_t::from_seconds( p->talent.plot_the_future->effectN( 1 ).base_value() );
  }

  void impact( action_state_t* s ) override
  {
    evoker_spell_t::impact( s );

    p()->get_target_data( s->target )->debuffs.temporal_wound->trigger();

    if ( eruption && s->chain_target < p()->talent.overlord->effectN( 1 ).base_value() )
    {
      eruption->execute_on_target( s->target );
    }
  }

  void execute() override
  {
    evoker_spell_t::execute();

    if ( ebon )
      ebon->execute();

    if ( is_precombat )
    {
      start_gcd();
      player->last_foreground_action = this;
      auto delay                     = std::max(
          0_s, rng().gauss( p()->option.prepull_deep_breath_delay, p()->option.prepull_deep_breath_delay_stddev ) );
      player->gcd_ready = delay;
      stats->iteration_total_execute_time += delay;
    }

    if ( p()->talent.plot_the_future.ok() )
    {
      make_event( sim, player->gcd_ready - sim->current_time(), [ this ] {
        if ( p()->buffs.bloodlust->check() )
          p()->buffs.bloodlust->extend_duration( p(), plot_duration );
        else if ( p()->buff.fury_of_the_aspects->check() )
          p()->buff.fury_of_the_aspects->extend_duration( p(), plot_duration );
        else
          p()->buff.fury_of_the_aspects->trigger( plot_duration );
      } );
    }
  }
};

struct time_skip_t : public evoker_spell_t
{
  time_skip_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "time_skip", p,
                      p->talent.interwoven_threads.ok() ? spell_data_t::not_found() : p->talent.time_skip, options_str )
  {
    target            = player;
    channeled         = true;
    hasted_ticks      = false;
    base_execute_time = 0_s;
    harmful           = false;
    dot_duration = base_tick_time = data().duration() + p->talent.tomorrow_today->effectN( 1 ).time_value();

    // Adjust base tick time to allow for reasonably timed interrupt statements.
    base_tick_time = 250_ms;
  }

  void execute() override
  {
    evoker_spell_t::execute();
    p()->buff.time_skip->trigger();
  }

  void last_tick( dot_t* d ) override
  {
    evoker_spell_t::last_tick( d );
    p()->buff.time_skip->expire();
  }
};

}  // end namespace spells

// Namespace buffs post spells
namespace buffs
{

struct fate_mirror_cb_t : public dbc_proc_callback_t
{
  evoker_t* source;
  spells::fate_mirror_damage_t* fate_mirror;

  fate_mirror_cb_t( player_t* p, const special_effect_t& e, evoker_t* source )
    : dbc_proc_callback_t( p, e ), source( source )
  {
    allow_pet_procs = true;
    deactivate();
    initialize();

    fate_mirror = debug_cast<spells::fate_mirror_damage_t*>( p->find_action( "fate_mirror" ) );
  }

  evoker_t* p()
  {
    return source;
  }

  void execute( action_t*, action_state_t* s ) override
  {
    if ( s->target->is_sleeping() )
      return;

    double da = s->result_amount;
    if ( da > 0 )
    {
      fate_mirror->evoker = source;
      fate_mirror->execute_on_target( s->target, da );
    }
  }
};

struct infernos_blessing_cb_t : public dbc_proc_callback_t
{
  evoker_t* source;
  spells::infernos_blessing_t* infernos_blessing;

  infernos_blessing_cb_t( player_t* p, const special_effect_t& e, evoker_t* source )
    : dbc_proc_callback_t( p, e ), source( source )
  {
    allow_pet_procs = true;
    deactivate();
    initialize();

    infernos_blessing = debug_cast<spells::infernos_blessing_t*>( p->find_action( "infernos_blessing" ) );
  }

  void execute( action_t*, action_state_t* s ) override
  {
    if ( s->target->is_sleeping() || !infernos_blessing )
      return;

    infernos_blessing->evoker = source;
    infernos_blessing->execute_on_target( s->target );
  }
};


struct blistering_scales_buff_t : public evoker_buff_t<buff_t>
{
private:
  using bb = evoker_buff_t<buff_t>;

  dbc_proc_callback_t* cb;
  stat_buff_t* armour;

  struct blistering_scales_cb_t : public dbc_proc_callback_t
  {
    evoker_t* source;
    spells::blistering_scales_damage_t* blistering_scales;
    stats_t* stats;

    blistering_scales_cb_t( player_t* p, const special_effect_t& e, evoker_t* source )
      : dbc_proc_callback_t( p, e ), source( source )
    {
      allow_pet_procs = true;
      deactivate();
      initialize();

      blistering_scales =
          debug_cast<spells::blistering_scales_damage_t*>( p->find_action( "blistering_scales_damage" ) );
    }

    evoker_t* p()
    {
      return source;
    }

    void execute( action_t*, action_state_t* s ) override
    {
      if ( s->target->is_sleeping() )
        return;

      p()->get_target_data( s->target )->buffs.blistering_scales->decrement();

      p()->sim->print_debug( "{}'s blistering scales detonates for action {} from {} targeting {}", *p(), *s->action,
                             *s->action->player, *s->target );

      blistering_scales->evoker = source;
      blistering_scales->execute_on_target( s->action->player );
    }
  };

  struct blistering_scales_armour_buff_t : evoker_buff_t<stat_buff_t>
  {
    blistering_scales_armour_buff_t( evoker_td_t& td, util::string_view name, const spell_data_t* s ) 
        : evoker_buff_t<stat_buff_t>( td, std::string( name ) + "_armour", s )
    {
      add_stat( STAT_BONUS_ARMOR, p()->composite_base_armor() * p()->talent.blistering_scales->effectN( 2 ).percent() );
      set_duration( 0_s );
      set_cooldown( 0_s );
      set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
      set_max_stack( 1 );
    }
  };

public:

  blistering_scales_buff_t( evoker_td_t& td, util::string_view name, const spell_data_t* s )
    : bb( td, name, s )
  {
    auto blistering_scales_effect          = new special_effect_t( td.target );
    blistering_scales_effect->name_str     = "blistering_scales_" + p()->name_str;
    blistering_scales_effect->type         = SPECIAL_EFFECT_EQUIP;
    blistering_scales_effect->spell_id     = p()->talent.blistering_scales->id();
    blistering_scales_effect->proc_flags2_ = PF2_ALL_HIT;
    blistering_scales_effect->cooldown_    = 2_s;  // Tested 27 / 05 / 2023
    td.target->special_effects.push_back( blistering_scales_effect );

    cb = new blistering_scales_cb_t( td.target, *blistering_scales_effect, p() );

    armour = new blistering_scales_armour_buff_t( td, name, s );

    apply_affecting_aura( p()->talent.regenerative_chitin );
    set_cooldown( 0_s );
    set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
    set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
      if ( new_ )
      {
        cb->activate();
        armour->trigger();
      }
      else if ( !new_ )
      {
        cb->deactivate();
        armour->expire();
      }
    } );
  }
    
  void update_stat( stat_buff_t* b, double amount )
  {
    if ( b->check() && b->stats[ 0 ].amount != amount )
    {
      b->stats[ 0 ].amount = amount;
      adjust_stats( b );
    }
  }

  double calc_armour()
  {
    return p()->composite_base_armor() * p()->talent.blistering_scales->effectN( 2 ).percent();
  }

  void adjust_stats( stat_buff_t* b )
  {
    for ( auto& buff_stat : b->stats )
    {
      if ( buff_stat.check_func && !buff_stat.check_func( *b ) )
        continue;

      double delta = buff_stat.stack_amount( b->current_stack ) - buff_stat.current_value;
      if ( delta > 0 )
      {
        b->player->stat_gain( buff_stat.stat, delta, b->stat_gain, nullptr, b->buff_duration() > timespan_t::zero() );
      }
      else if ( delta < 0 )
      {
        b->player->stat_loss( buff_stat.stat, std::fabs( delta ), b->stat_gain, nullptr,
                              b->buff_duration() > timespan_t::zero() );
      }

      buff_stat.current_value += delta;
    }
  }

  void update_armour_buff()
  {
    update_stat( armour, calc_armour() );
  }

};

struct temporal_wound_buff_t : public evoker_buff_t<buff_t>
{
  target_specific_t<spells::breath_of_eons_damage_t> eon_actions;
  std::map<size_t, double> eon_stored;
  dbc_proc_callback_t* cb;

  struct temporal_wound_cb_t : public dbc_proc_callback_t
  {
    evoker_t* source;
    std::unique_ptr<dbc_proc_callback_t::trigger_fn_t> trig;

    temporal_wound_cb_t( player_t* p, const special_effect_t& e, evoker_t* source )
      : dbc_proc_callback_t( p, e ), source( source )
    {
      allow_pet_procs = true;
      deactivate();
      initialize();

      trigger_type = trigger_fn_type::CONDITION;

      const trigger_fn_t lambda = [ this ]( const dbc_proc_callback_t*, action_t*, action_state_t* s ) {
        if ( s->result_amount <= 0 )
          return false;

        if ( this->p() == s->action->player )
          return static_cast<bool>( this->p()->buff.ebon_might_self_buff->check() );

        return static_cast<bool>(
            this->p()->get_target_data( s->action->player->get_owner_or_self() )->buffs.ebon_might->check() );
      };

      trig = std::make_unique<dbc_proc_callback_t::trigger_fn_t>( lambda );

      trigger_fn = trig.get();
    }

    evoker_t* p()
    {
      return source;
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      if ( s->target->is_sleeping() )
        return;

      double da = s->result_amount;
      p()->sim->print_debug( "{} triggers {}s temporal wound on {} with {} dealing {}",
                             *s->action->player->get_owner_or_self(), *p(), *s->target, *a, da );
      if ( da > 0 )
      {
        buffs::temporal_wound_buff_t* buff =
            debug_cast<buffs::temporal_wound_buff_t*>( p()->get_target_data( s->target )->debuffs.temporal_wound );

        if ( buff && buff->up() )
        {
          buff->eon_stored[ buff->player_id( s->action->player->get_owner_or_self() ) ] += da;
          p()->sim->print_debug(
              "{} triggers {}s temporal wound on {} with {} dealing {} increasing stored damage to {} from {}",
              *s->action->player->get_owner_or_self(), *p(), *s->target, *a, da,
              buff->eon_stored[ buff->player_id( s->action->player->get_owner_or_self() ) ],
              buff->eon_stored[ buff->player_id( s->action->player->get_owner_or_self() ) ] - da );
        }
      }
    }
  };

  temporal_wound_buff_t( evoker_td_t& td, util::string_view name, const spell_data_t* s )
    : evoker_buff_t<buff_t>( td, name, s ), eon_actions{ false }
  {
    buff_period = 0_s;

    auto temporal_wound_effect      = new special_effect_t( p() );
    temporal_wound_effect->name_str = "temporal_wound_" + p()->name_str;
    temporal_wound_effect->type     = SPECIAL_EFFECT_EQUIP;
    temporal_wound_effect->spell_id = p()->talent.temporal_wound->id();
    player->special_effects.push_back( temporal_wound_effect );

    cb = new temporal_wound_cb_t( player, *temporal_wound_effect, p() );
  }

  size_t player_id( player_t* p )
  {
    return p->actor_index;
  }

  // TODO: Stop using PID and just use the pointers.
  player_t* player_from_id( size_t i )
  {
    return sim->actor_list[ i ];
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    if ( !evoker_buff_t::trigger( stacks, value, chance, duration ) )
      return false;

    if ( cb )
      cb->activate();

    return true;
  }

  spells::breath_of_eons_damage_t* get_eon_action( const player_t* target )
  {
    if ( eon_actions[ target ] != nullptr )
      return eon_actions[ target ];

    eon_actions[ target ] = debug_cast<spells::breath_of_eons_damage_t*>( target->find_action( "breath_of_eons_damage" ) );

    return eon_actions[ target ];
  }


  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( cb )
      cb->deactivate();

    for ( auto& [ pid, damage ] : eon_stored )
    {
      auto pp            = player_from_id( pid );
      auto eon_damage    = get_eon_action( pp );
      eon_damage->evoker = p();
      sim->print_debug( "{} eon helper expiry, player {}s, size {}, damage execution inc, executing on {}", *source,
                        *pp, damage, *player );
      eon_damage->execute_on_target( player, damage );
    }
    eon_stored.clear();
  }
};
}  // namespace buffs

// Namespace spells again
namespace spells
{

struct blistering_scales_t : public evoker_augment_t
{
  blistering_scales_t( evoker_t* p, std::string_view options_str )
    : evoker_augment_t( "blistering_scales", p, p->talent.blistering_scales, options_str )
  {
  }

  void impact( action_state_t* s ) override
  {
    evoker_augment_t::impact( s );

    if ( p()->last_scales_target )
      p()->get_target_data( p()->last_scales_target )->buffs.blistering_scales->expire();

    p()->last_scales_target = s->target;

    p()->get_target_data( s->target )->buffs.blistering_scales->trigger();

    debug_cast<buffs::blistering_scales_buff_t*>( p()->get_target_data( s->target )->buffs.blistering_scales )
        ->update_armour_buff();
  }
};

}  // namespace spells

// ==========================================================================
// Evoker Character Definitions
// ==========================================================================

evoker_td_t::evoker_td_t( player_t* target, evoker_t* evoker )
  : actor_target_data_t( target, evoker ), dots(), debuffs(), buffs()
{
  dots.fire_breath  = target->get_dot( "fire_breath_damage", evoker );
  dots.disintegrate = target->get_dot( "disintegrate", evoker );

  debuffs.shattering_star = make_buff( *this, "shattering_star_debuff", evoker->talent.shattering_star )
                                ->set_cooldown( 0_ms )
                                ->apply_affecting_aura( evoker->talent.focusing_iris );

  debuffs.in_firestorm = make_buff( *this, "in_firestorm" )->set_max_stack( 20 )->set_duration( timespan_t::zero() );

  if ( evoker->naszuro && !target->is_enemy() && !target->is_pet() )
  {
    buffs.unbound_surge = make_buff<stat_buff_t>( target, "unbound_surge_" + evoker->name_str,
                                                  evoker->find_spell( 403275 ), evoker->naszuro->item );
    buffs.unbound_surge->set_period( 0_s );

    switch ( evoker->specialization() )
    {
      case EVOKER_DEVASTATION:
        buffs.unbound_surge->set_duration(
            timespan_t::from_seconds( evoker->naszuro->driver()->effectN( 2 ).base_value() ) );
        break;
      case EVOKER_PRESERVATION:
        buffs.unbound_surge->set_duration(
            timespan_t::from_seconds( evoker->naszuro->driver()->effectN( 3 ).base_value() ) );
        break;
      case EVOKER_AUGMENTATION:
        buffs.unbound_surge->set_duration(
            timespan_t::from_seconds( evoker->naszuro->driver()->effectN( 4 ).base_value() ) );
        break;
      default:
        break;
    }

    if ( evoker->option.naszuro_bounce_chance > 0 && evoker->option.naszuro_accurate_behaviour )
    {
      buffs.unbound_surge->set_period( 3_s );
      if ( auto* _target = dynamic_cast<evoker_t*>( target ) )
      {
        buffs.unbound_surge->set_tick_callback( [ _target, evoker ]( buff_t* b, int s, timespan_t t ) {
          {
            if ( b->remains() > 0_s && !_target->buff.dragonrage->check() &&
                 _target->rng().roll( evoker->option.naszuro_bounce_chance ) )
              make_event( _target->sim, [ _target, evoker, b, t ] {
                evoker->bounce_naszuro( _target, b->remains() );
                b->expire();
              } );
          }
        } );
      }
      else
      {
        buffs.unbound_surge->set_tick_callback( [ target, evoker ]( buff_t* b, int s, timespan_t t ) {
          {
            if ( b->remains() > 0_s && target->rng().roll( evoker->option.naszuro_bounce_chance ) )
              make_event( target->sim, [ target, evoker, b, t ] {
                evoker->bounce_naszuro( target, b->remains() );
                b->expire();
              } );
          }
        } );
      }
    }
  }

  if ( evoker->specialization() == EVOKER_AUGMENTATION )
  {
    using e_buff_t = buffs::evoker_buff_t<buff_t>;

    if ( evoker->talent.breath_of_eons.ok() )
    {
      if ( target->is_enemy() )
      {
        debuffs.temporal_wound =
            make_buff<buffs::temporal_wound_buff_t>( *this, "temporal_wound", evoker->talent.temporal_wound );
      }
    }

    if ( !target->is_enemy() && !target->is_pet() )
    {
      buffs.shifting_sands =
          make_buff<e_buff_t>( *this, "shifting_sands_" + evoker->name_str, evoker->find_spell( 413984 ) )
              ->set_default_value( evoker->cache.mastery_value() )
              ->set_pct_buff_type( STAT_PCT_BUFF_VERSATILITY )
              ->set_period( timespan_t::zero() );

      buffs.ebon_might = make_buff<buffs::evoker_buff_t<stat_buff_t>>( *this, "ebon_might_" + evoker->name_str,
                                                                       evoker->find_spell( 395152 ) )
                             ->set_stat_from_effect( 2, 0 );

      buffs.ebon_might->set_cooldown( 0_ms )
          ->set_period( timespan_t::zero() )
          ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
          ->set_stack_change_callback( [ target, evoker ]( buff_t* b, int, int new_ ) {
            if ( new_ )
            {
              evoker->allies_with_my_ebon.push_back( target );
              if ( auto e = dynamic_cast<evoker_t*>( target ) )
              {
                e->allied_ebons_on_me.push_back( b );
              }
            }
            else
            {
              evoker->allies_with_my_ebon.find_and_erase_unordered( target );
              if ( auto e = dynamic_cast<evoker_t*>( target ) )
              {
                auto vec = e->allied_ebons_on_me;
                vec.erase( std::remove( vec.begin(), vec.end(), b ), vec.end() );
              }
            }
            for ( auto& c : evoker->allied_ebon_callbacks )
            {
              c();
            }
          } );

      buffs.prescience = make_buff<e_buff_t>( *this, "prescience", evoker->talent.prescience_buff )
                             ->set_default_value( evoker->talent.prescience_buff->effectN( 1 ).percent() )
                             ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
                             ->set_chance( 1.0 );

      if ( evoker->talent.fate_mirror.ok() )
      {
        auto fate_mirror_effect      = new special_effect_t( target );
        fate_mirror_effect->name_str = "fate_mirror_" + evoker->name_str;
        fate_mirror_effect->type     = SPECIAL_EFFECT_EQUIP;
        fate_mirror_effect->spell_id = evoker->talent.prescience_buff->id();
        target->special_effects.push_back( fate_mirror_effect );

        auto fate_mirror_cb = new buffs::fate_mirror_cb_t( target, *fate_mirror_effect, evoker );

        buffs.prescience->set_stack_change_callback( [ fate_mirror_cb, target, evoker ]( buff_t*, int, int new_ ) {
          if ( new_ )
          {
            fate_mirror_cb->activate();
            evoker->allies_with_my_prescience.push_back( target );
          }
          else
          {
            fate_mirror_cb->deactivate();
            evoker->allies_with_my_prescience.find_and_erase_unordered( target );
          }
        } );
      }
      else
      {
        buffs.prescience->set_stack_change_callback( [ target, evoker ]( buff_t* b, int, int new_ ) {
          if ( new_ )
          {
            evoker->allies_with_my_prescience.push_back( target );
          }
          else
          {
            evoker->allies_with_my_prescience.find_and_erase_unordered( target );
          }
        } );
      }

      if ( evoker->talent.blistering_scales )
      {
        buffs.blistering_scales = make_buff<buffs::blistering_scales_buff_t>(
            *this, "blistering_scales_" + evoker->name_str, evoker->talent.blistering_scales );
      }

      if ( evoker->talent.infernos_blessing.ok() )
      {
        buffs.infernos_blessing =
            make_buff<e_buff_t>( *this, "infernos_blessing", evoker->talent.infernos_blessing_buff );

        auto infernos_blessing_effect      = new special_effect_t( target );
        infernos_blessing_effect->name_str = "infernos_blessing_" + target->name_str;
        infernos_blessing_effect->type     = SPECIAL_EFFECT_EQUIP;
        infernos_blessing_effect->spell_id = evoker->talent.infernos_blessing_buff->id();
        target->special_effects.push_back( infernos_blessing_effect );

        auto infernos_blessing_cb =
            new buffs::infernos_blessing_cb_t( target, *infernos_blessing_effect, evoker );

        buffs.infernos_blessing->set_stack_change_callback( [ infernos_blessing_cb ]( buff_t*, int, int new_ ) {
          if ( new_ )
            infernos_blessing_cb->activate();
          else
            infernos_blessing_cb->deactivate();
        } );
      }
    }
  }
}

evoker_t::evoker_t( sim_t* sim, std::string_view name, race_e r )
  : player_t( sim, EVOKER, name, r ),
    allies_with_my_ebon(),
    allies_with_my_prescience(),
    allied_ebons_on_me(),
    last_scales_target( nullptr ),
    was_empowering( false ),
    naszuro(),
    allied_augmentations(),
    allied_ebon_callbacks(),
    heartbeat(),
    close_as_clutchmates( false ),
    option(),
    action(),
    buff(),
    spec(),
    talent(),
    benefit(),
    cooldown(),
    gain(),
    proc(),
    rppm(),
    uptime()
{
  cooldown.eternity_surge = get_cooldown( "eternity_surge" );
  cooldown.fire_breath    = get_cooldown( "fire_breath" );
  cooldown.firestorm      = get_cooldown( "firestorm" );
  cooldown.upheaval       = get_cooldown( "upheaval" );
  cooldown.breath_of_eons = get_cooldown( "breath_of_eons" );

  resource_regeneration             = regen_type::DYNAMIC;
  regen_caches[ CACHE_HASTE ]       = true;
  regen_caches[ CACHE_SPELL_HASTE ] = true;
}

// Kharnalex, The First Light ======================================================

void karnalex_the_first_light( special_effect_t& effect )
{
  using generic_proc_t = unique_gear::generic_proc_t;
  struct light_of_creation_t : public generic_proc_t
  {
    light_of_creation_t( const special_effect_t& e ) : generic_proc_t( e, "light_of_creation", e.driver() )
    {
      channeled = true;
    }

    bool usable_moving() const override
    {
      return false;
    }

    void execute() override
    {
      generic_proc_t::execute();
      event_t::cancel( player->readying );
      player->reset_auto_attacks( composite_dot_duration( execute_state ) );
    }

    evoker_t* p()
    {
      return static_cast<evoker_t*>( generic_proc_t::player );
    }

    evoker_t* p() const
    {
      return static_cast<evoker_t*>( generic_proc_t::player );
    }

    void tick( dot_t* d ) override
    {
      generic_proc_t::tick( d );

      // TODO: Do a better job of this. It's close enough for now but it still is awful, and the item doesn't have the
      // time properly added.
      stats->iteration_total_execute_time += d->time_to_tick();
    }

    void last_tick( dot_t* d ) override
    {
      bool was_channeling = player->channeling == this;
      generic_proc_t::last_tick( d );

      if ( was_channeling && !player->readying )
      {
        player->schedule_ready( rng().gauss( sim->channel_lag, sim->channel_lag_stddev ) );
      }
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double tm = generic_proc_t::composite_target_multiplier( t );

      if ( p()->specialization() == EVOKER_DEVASTATION )
      {
        if ( !p()->buff.dragonrage->check() || !p()->talent.tyranny.ok() )
          tm *= 1.0 + p()->cache.mastery_value() * t->health_percentage() / 100;
        else
          tm *= 1.0 + p()->cache.mastery_value();
      }

      return tm;
    }
  };

  effect.execute_action = unique_gear::create_proc_action<light_of_creation_t>( "light_of_creation", effect );

  action_t* action = effect.player->find_action( "use_item_" + effect.item->name_str );
  if ( action )
    action->base_execute_time = effect.execute_action->base_execute_time;
}

// Evoker Legendary Weapon Nasz'uro, the Unbound Legacy
void insight_of_naszuro( special_effect_t& effect )
{
  if ( auto e = dynamic_cast<evoker_t*>( effect.player ) )
  {
    e->naszuro = &effect;
  }
}

unsigned int evoker_t::specialization_aura_id()
{
  switch ( specialization() )
  {
    case EVOKER_DEVASTATION:
      return spec.devastation->id();
      break;
    case EVOKER_PRESERVATION:
      return spec.preservation->id();
      break;
    case EVOKER_AUGMENTATION:
      return spec.augmentation->id();
      break;
    default:
      return 356816;
      break;
  }
}

void evoker_t::init_action_list()
{
  // 2022-08-07: Healing is not supported
  if ( !sim->allow_experimental_specializations && primary_role() == ROLE_HEAL )
  {
    if ( !quiet )
      sim->error( "Role heal for evoker '{}' is currently not supported.", name() );

    quiet = true;
    return;
  }

  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  switch ( specialization() )
  {
    case EVOKER_DEVASTATION:
      evoker_apl::devastation( this );
      break;
    case EVOKER_PRESERVATION:
      evoker_apl::preservation( this );
      break;
    case EVOKER_AUGMENTATION:
      evoker_apl::augmentation( this );
      break;
    default:
      evoker_apl::no_spec( this );
      break;
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

void evoker_t::init_finished()
{
  for ( auto p : sim->player_no_pet_list )
  {
    if ( p != this && p->specialization() == EVOKER_AUGMENTATION )
    {
      allied_augmentations.push_back( static_cast<evoker_t*>( p ) );
    }
  }

  if ( sim->player_no_pet_list.size() <= 5 )
  {
    close_as_clutchmates = true;
    sim->print_debug( "{} Activating Close As Clutchmates", *this );
  }

  player_t::init_finished();

  /*PRECOMBAT SHENANIGANS
  we do this here so all precombat actions have gone throught init() and init_finished() so if-expr are properly
  parsed and we can adjust wrath travel times accordingly based on subsequent precombat actions that will sucessfully
  cast*/

  for ( auto pre = precombat_action_list.begin(); pre != precombat_action_list.end(); pre++ )
  {
    if ( auto lf = dynamic_cast<spells::living_flame_t*>( *pre ) )
    {
      int actions           = 0;
      timespan_t time_spent = timespan_t::zero();

      std::for_each( pre + 1, precombat_action_list.end(), [ &actions, &time_spent ]( action_t* a ) {
        if ( a->gcd() > timespan_t::zero() && ( !a->if_expr || a->if_expr->success() ) && a->action_ready() )
        {
          actions++;
          time_spent += std::max( a->base_execute_time, a->trigger_gcd );
        }
      } );

      // Only allow precast Living Flame if there's only one GCD action following it - It doesn't have a very long
      // travel time.
      if ( actions == 1 )
      {
        lf->harmful = false;
        // Child contains the travel time
        precombat_travel      = lf->damage->travel_time();
        lf->prepull_timespent = time_spent;
        break;
      }
    }
  }

  if ( heartbeat.callbacks.size() > 0 )
  {
    heartbeat.initial_time = timespan_t::from_millis( rng().range( 0, 5250 ) );
    register_combat_begin( [ this ]( player_t* ) {
      make_event( sim, heartbeat.initial_time, [ this ] {
        for ( auto& cb : heartbeat.callbacks )
        {
          cb();
        }
        make_repeating_event( sim, 5.25_s, [ this ]() {
          for ( auto& cb : heartbeat.callbacks )
          {
            cb();
          }
        } );
      } );
    } );
  }
}

role_e evoker_t::primary_role() const
{
  switch ( player_t::primary_role() )
  {
    case ROLE_HEAL:
      return ROLE_HEAL;
    case ROLE_DPS:
    case ROLE_SPELL:
      return ROLE_SPELL;
    case ROLE_ATTACK:
      return ROLE_SPELL;
    default:
      if ( specialization() == EVOKER_PRESERVATION )
      {
        return ROLE_HEAL;
      }
      break;
  }

  return ROLE_SPELL;
}

void evoker_t::init_benefits()
{
  player_t::init_benefits();

  benefit.supercharged_shards = get_benefit( "Supercharged Shards" );
}

void evoker_t::init_gains()
{
  player_t::init_gains();

  gain.eye_of_infinity      = get_gain( "Eye of Infinity" );
  gain.roar_of_exhilaration = get_gain( "Roar of Exhilaration" );
  gain.energizing_flame = get_gain( "Energizing Flame" );
}

void evoker_t::init_procs()
{
  player_t::init_procs();

  proc.ruby_essence_burst        = get_proc( "Ruby Essence Burst" );
  proc.azure_essence_burst       = get_proc( "Azure Essence Burst" );
  proc.anachronism_essence_burst = get_proc( "Anachronism" );
  proc.echoing_strike            = get_proc( "Echoing Strike" );
}

void evoker_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = 25;

  player_t::init_base_stats();

  // We need to check travel time during init_finished... So we have to initialise current.distance ourselves. Note to
  // self: Never play with prepull actions ever again.
  current.distance = base.distance;

  base.spell_power_per_intellect = 1.0;

  resources.base[ RESOURCE_ESSENCE ] = 5 + find_spelleffect( talent.power_nexus, A_MOD_MAX_RESOURCE )->base_value();
  // TODO: confirm base essence regen. currently estimated at 1 per 5s base
  resources.base_regen_per_second[ RESOURCE_ESSENCE ] = 0.2 * ( 1.0 + talent.innate_magic->effectN( 1 ).percent() );
}

void evoker_t::init_background_actions()
{
  player_t::init_background_actions();

  if ( talent.ebon_might.ok() )
  {
    background_actions.ebon_might =
        get_secondary_action<spells::ebon_might_t>( "ebon_might_helper", timespan_t::min(), "ebon_might_helper" );

    heartbeat.callbacks.push_back( [ ebon = background_actions.ebon_might.get() ]() {
      static_cast<spells::ebon_might_t*>( ebon )->update_stats();
    } );
  }
}

void evoker_t::init_spells()
{
  player_t::init_spells();

  // Evoker Talents
  auto CT = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::CLASS, n ); };
  auto ST = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::SPECIALIZATION, n ); };

  // Class Traits
  talent.landslide            = CT( "Landslide" );  // Row 1
  talent.obsidian_scales      = CT( "Obsidian Scales" );
  talent.expunge              = CT( "Expunge" );
  talent.natural_convergence  = CT( "Natural Convergence" );  // Row 2
  talent.verdant_embrace      = CT( "Verdant Embrace" );
  talent.forger_of_mountains  = CT( "Forger of Mountains" );  // Row 3
  talent.innate_magic         = CT( "Innate Magic" );
  talent.obsidian_bulwark     = CT( "Obsidian Bulwark" );
  talent.enkindled            = CT( "Enkindled" );
  talent.scarlet_adaptation   = CT( "Scarlet Adaptation" );
  talent.quell                = CT( "Quell" );  // Row 4
  talent.tailwind             = CT( "Tailwind" );
  talent.roar_of_exhilaration = CT( "Roar of Exhilaration" );  // Row 5
  talent.instinctive_arcana   = CT( "Instinctive Arcana" );
  talent.tip_the_scales       = CT( "Tip the Scales" );
  talent.attuned_to_the_dream = CT( "Attuned to the Dream" );  // healing received NYI
  talent.draconic_legacy      = CT( "Draconic Legacy" );       // Row 6
  talent.inherent_resistance  = CT( "Inherent Resistance" );
  talent.extended_flight      = CT( "Extended Flight" );
  talent.bountiful_bloom      = CT( "Bountiful Bloom" );
  talent.blast_furnace        = CT( "Blast Furnace" );  // Row 7
  talent.panacea              = CT( "Panacea" );
  talent.panacea_spell        = find_spell( 387763 );
  talent.exuberance           = CT( "Exuberance" );
  talent.ancient_flame        = CT( "Ancient Flame" );
  talent.protracted_talons    = CT( "Protracted Talons" );  // Row 8
  talent.lush_growth          = CT( "Lush Growth" );
  talent.leaping_flames       = CT( "Leaping Flames" );  // Row 9
  talent.aerial_mastery       = CT( "Aerial Mastery" );
  // Devastation Traits
  talent.pyre                      = ST( "Pyre" );                // Row 1
  talent.ruby_essence_burst        = ST( "Ruby Essence Burst" );  // Row 2
  talent.azure_essence_burst       = ST( "Azure Essence Burst" );
  talent.dense_energy              = ST( "Dense Energy" );  // Row 3
  talent.imposing_presence         = ST( "Imposing Presence" );
  talent.eternity_surge            = ST( "Eternity Surge" );
  talent.volatility                = ST( "Volatility" );  // Row 4
  talent.power_nexus               = ST( "Power Nexus" );
  talent.dragonrage                = ST( "Dragonrage" );
  talent.lay_waste                 = ST( "Lay Waste" );
  talent.arcane_intensity          = ST( "Arcane Intensity" );
  talent.ruby_embers               = ST( "Ruby Embers" );  // Row 5
  talent.engulfing_blaze           = ST( "Engulfing Blaze" );
  talent.animosity                 = ST( "Animosity" );
  talent.essence_attunement        = ST( "Essence Attunement" );
  talent.firestorm                 = ST( "Firestorm" );  // Row 6
  talent.heat_wave                 = ST( "Heat Wave" );
  talent.titanic_wrath             = ST( "Titanic Wrath" );
  talent.honed_aggression          = ST( "Honed Aggression" );
  talent.eternitys_span            = ST( "Eternity's Span" );
  talent.eye_of_infinity           = ST( "Eye of Infinity" );
  talent.event_horizon             = ST( "Event Horizon" );
  talent.causality                 = ST( "Causality" );
  talent.catalyze                  = ST( "Catalyze" );  // Row 7
  talent.tyranny                   = ST( "Tyranny" );
  talent.charged_blast             = ST( "Charged Blast" );
  talent.shattering_star           = ST( "Shattering Star" );
  talent.snapfire                  = ST( "Snapfire" );  // Row 8
  talent.raging_inferno            = ST( "Raging Inferno" );
  talent.font_of_magic             = ST( "Font of Magic" );
  talent.onyx_legacy               = ST( "Onyx Legacy" );
  talent.spellweavers_dominance    = ST( "Spellweaver's Dominance" );
  talent.focusing_iris             = ST( "Focusing Iris" );
  talent.arcane_vigor              = ST( "Arcane Vigor" );
  talent.burnout                   = ST( "Burnout" );  // Row 9
  talent.imminent_destruction      = ST( "Imminent Destruction" );
  talent.scintillation             = ST( "Scintillation" );
  talent.power_swell               = ST( "Power Swell" );
  talent.feed_the_flames           = ST( "Feed the Flames" );  // Row 10
  talent.feed_the_flames_pyre_buff = find_spell( 411288 );
  talent.everburning_flame         = ST( "Everburning Flame" );
  talent.hoarded_power             = ST( "Hoarded Power" );
  talent.iridescence               = ST( "Iridescence" );
  // Preservation Traits

  // Augmentation Traits
  talent.ebon_might           = ST( "Ebon Might" );
  talent.ebon_might_self_buff = find_spell( 395296 );
  talent.sands_of_time        = find_spell( 395153 );
  talent.eruption             = ST( "Eruption" );
  talent.essence_burst        = ST( "Essence Burst" );
  // Imposing Presence / Inner Radiance - Non DPS
  talent.ricocheting_pyroclast = ST( "Ricocheting Pyroclast" );
  // Essence Attunement - Devastation also has
  talent.pupil_of_alexstrasza  = ST( "Pupil of Alexstrasza" );
  talent.echoing_strike        = ST( "Echoing Strike" );
  talent.upheaval              = ST( "upheaval" );
  talent.breath_of_eons        = ST( "Breath of Eons" );
  talent.breath_of_eons_damage = find_spell( 409632 );
  talent.temporal_wound        = find_spell( 409560 );
  // Defy Fate - Non DPS
  // Timelessness - Non DPS
  // Seismic Slam - Non DPS
  talent.volcanism = ST( "Volcanism" );
  // Perilous Fate / Chrono Ward - Non DPS
  // Stretch Time - Non DPS
  // Geomancy - Non DPS
  // Bestow Weyrnstone - Non DPS
  talent.blistering_scales        = ST( "Blistering Scales" );
  talent.blistering_scales_damage = find_spell( 360828 );
  // Draconic Attunements - Non DPS
  // Spatial Paradox Non DPS - Movement DPS Gain?
  talent.unyielding_domain   = ST( "Unyielding Domain" );
  talent.tectonic_locus      = ST( "Tectonic Locus" );
  talent.regenerative_chitin = ST( "Regenerative Chitin" );
  talent.molten_blood        = ST( "Molten Blood" );
  // Power Nexus - Devastation also has
  // Aspects' Favor - Non DPS
  talent.plot_the_future = ST( "Plot the Future" );
  talent.dream_of_spring = ST( "Dream of Spring" );
  // Symbiotic Bloom - Non DPS but Scarlet exists. Todo: implement healing
  talent.reactive_hide      = ST( "Reactive Hide" );
  talent.reactive_hide_buff = find_spell( 410256 );
  // Hoarded Power - Devas Has
  talent.ignition_rush   = ST( "Ignition Rush" );
  talent.prescience      = ST( "Prescience" );
  talent.prescience_buff = find_spell( 410089 );
  // Prolong Life - Non DPS. Scarlet Exists. Todo: Implement Healing
  talent.momentum_shift           = ST( "Momentum Shift" );
  talent.infernos_blessing        = ST( "Inferno's Blessing" );
  talent.infernos_blessing_damage = find_spell( 410265 );
  talent.infernos_blessing_buff   = find_spell( 410263 );
  talent.time_skip                = ST( "Time Skip" );
  talent.accretion                = ST( "Accretion" );
  talent.anachronism              = ST( "Anachronism" );
  talent.motes_of_possibility     = ST( "Motes of Possibility" );
  // Font of Magic - Devastation ha
  talent.tomorrow_today     = ST( "Tomorrow, Today" );
  talent.interwoven_threads = ST( "Interwoven Threads" );
  talent.overlord           = ST( "Overlord" );
  talent.fate_mirror        = ST( "Fate Mirror" );
  talent.fate_mirror_damage = find_spell( 404908 );

  // Set up Essence Bursts for Preservation and Augmentation
  if ( talent.essence_burst.ok() )
  {
    const trait_data_t* trait;
    uint32_t class_idx, spec_idx;

    dbc->spec_idx( EVOKER_DEVASTATION, class_idx, spec_idx );

    trait = trait_data_t::find( talent_tree::SPECIALIZATION, "Ruby Essence Burst", class_idx, EVOKER_DEVASTATION,
                                dbc->ptr );

    if ( trait )
    {
      talent.ruby_essence_burst = player_talent_t( this, trait, trait->max_ranks );
    }

    if ( specialization() == EVOKER_AUGMENTATION )
    {
      trait = trait_data_t::find( talent_tree::SPECIALIZATION, "Azure Essence Burst", class_idx, EVOKER_DEVASTATION,
                                  dbc->ptr );

      if ( trait )
      {
        talent.azure_essence_burst = player_talent_t( this, trait, trait->max_ranks );
      }
    }
  }

  // Evoker Specialization Spells
  spec.evoker               = find_spell( 353167 );  // TODO: confirm this is the class aura
  spec.devastation          = find_specialization_spell( "Devastation Evoker" );
  spec.preservation         = find_specialization_spell( "Preservation Evoker" );
  spec.augmentation         = find_specialization_spell( "Augmentation Evoker" );
  spec.mastery              = find_mastery_spell( specialization() );
  spec.living_flame_damage  = find_spell( 361500 );
  spec.living_flame_heal    = find_spell( 361509 );
  spec.energizing_flame     = find_spell( 400006 );
  spec.tempered_scales      = find_spell( 396571 );
  spec.emerald_blossom      = find_spell( 355913 );
  spec.emerald_blossom_heal = find_spell( 355916 );
  spec.emerald_blossom_spec = find_specialization_spell( 365261, specialization() );
  spec.close_as_clutchmates = find_specialization_spell( 396043, specialization() );
}

void evoker_t::init_special_effects()
{
  player_t::init_special_effects();
}

void evoker_t::create_actions()
{
  using namespace spells;
  using namespace heals;

  if ( sets->has_set_bonus( EVOKER_DEVASTATION, T30, B2 ) )
    action.obsidian_shards = get_secondary_action<obsidian_shards_t>( "obsidian_shards" );

  if ( talent.volatility.ok() )
  {
    auto vol                = get_secondary_action<pyre_t>( "pyre_volatility", "pyre_volatility", talent.pyre );
    vol->s_data_reporting   = talent.volatility;
    vol->name_str_reporting = "volatility";
    vol->proc_spell_type    = proc_spell_type_e::VOLATILITY;
    vol->proc               = true;
    action.volatility       = vol;

    if ( talent.dragonrage.ok() )
    {
      auto vol_dr =
          get_secondary_action<pyre_t>( "dragonrage_pyre_volatility", "dragonrage_pyre_volatility", talent.pyre );
      vol_dr->s_data_reporting     = talent.volatility;
      vol_dr->name_str_reporting   = "volatility";
      vol_dr->proc_spell_type      = proc_spell_type_e::VOLATILITY | proc_spell_type_e::DRAGONRAGE;
      vol_dr->proc                 = true;
      action.volatility_dragonrage = vol_dr;
    }
  }

  player_t::create_actions();
}

void evoker_t::create_buffs()
{
  player_t::create_buffs();

  using namespace buffs;
  using e_buff_t = evoker_buff_t<buff_t>;

  // Baseline Abilities

  switch ( specialization() )
  {
    case EVOKER_PRESERVATION:
      buff.essence_burst = make_buff<e_buff_t>( this, "essence_burst", find_spell( 369299 ) )
                               ->apply_affecting_aura( talent.essence_attunement );
      break;
    case EVOKER_AUGMENTATION:
      buff.essence_burst = make_buff<e_buff_t>( this, "essence_burst", find_spell( 392268 ) )
                               ->apply_affecting_aura( talent.essence_attunement );
      break;
    case EVOKER_DEVASTATION:
    default:
      buff.essence_burst = make_buff<e_buff_t>( this, "essence_burst", find_spell( 359618 ) )
                               ->apply_affecting_aura( talent.essence_attunement );
      break;
  }

  buff.essence_burst_titanic_wrath_disintegrate =
      make_buff<e_buff_t>( this, "essence_burst_titanic_wrath_disintegrate", find_spell( 397870 ) )
          ->set_quiet( true )
          ->set_trigger_spell( talent.titanic_wrath );

  buff.hover = make_buff<e_buff_t>( this, "hover", find_class_spell( "Hover" ) )
                   ->set_cooldown( 0_ms )
                   ->set_default_value_from_effect_type( A_MOD_INCREASE_SPEED );

  buff.tailwind =
      make_buff<e_buff_t>( this, "tailwind", find_spelleffect( talent.tailwind, A_PROC_TRIGGER_SPELL )->trigger() )
          ->set_default_value_from_effect( 1 );

  // Class Traits
  buff.ancient_flame = make_buff<e_buff_t>( this, "ancient_flame", find_spell( 375583 ) )
                           ->set_trigger_spell( talent.ancient_flame )
                           ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  buff.leaping_flames =
      make_buff<e_buff_t>( this, "leaping_flames", find_spell( 370901 ) )->set_trigger_spell( talent.leaping_flames );

  buff.obsidian_scales = make_buff<e_buff_t>( this, "obsidian_scales", talent.obsidian_scales )->set_cooldown( 0_ms );

  buff.scarlet_adaptation = make_buff<e_buff_t>( this, "scarlet_adaptation", find_spell( 372470 ) )
                                ->set_trigger_spell( talent.scarlet_adaptation )
                                ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  buff.tip_the_scales = make_buff<e_buff_t>( this, "tip_the_scales", talent.tip_the_scales )->set_cooldown( 0_ms );

  // Devastation
  buff.blazing_shards = make_buff<e_buff_t>( this, "blazing_shards", find_spell( 409848 ) )
                            ->set_trigger_spell( sets->set( EVOKER_DEVASTATION, T30, B4 ) );

  buff.burnout = make_buff<e_buff_t>( this, "burnout", find_spell( 375802 ) )
                     ->set_trigger_spell( talent.burnout )
                     ->set_cooldown( talent.burnout->internal_cooldown() )
                     ->set_chance( talent.burnout->effectN( 1 ).percent() );

  buff.charged_blast = make_buff<e_buff_t>( this, "charged_blast", talent.charged_blast->effectN( 1 ).trigger() )
                           ->set_default_value_from_effect( 1 );

  buff.dragonrage = make_buff<e_buff_t>( this, "dragonrage", talent.dragonrage )->set_cooldown( 0_ms );

  buff.fury_of_the_aspects =
      make_buff<e_buff_t>( this, "fury_of_the_aspects", find_class_spell( "Fury of the Aspects" ) )
          ->set_default_value_from_effect( 1 )
          ->set_cooldown( 0_s )
          ->add_invalidate( CACHE_HASTE );

  buff.imminent_destruction = make_buff<e_buff_t>( this, "imminent_destruction", find_spell( 411055 ) );

  buff.iridescence_blue = make_buff<e_buff_t>( this, "iridescence_blue", find_spell( 386399 ) )
                              ->set_trigger_spell( talent.iridescence )
                              ->set_default_value_from_effect( 1 );
  buff.iridescence_blue->set_initial_stack( buff.iridescence_blue->max_stack() );

  buff.iridescence_blue_disintegrate =
      make_buff<e_buff_t>( this, "iridescence_blue_disintegrate", find_spell( 399370 ) )
          ->set_quiet( true )
          ->set_default_value( buff.iridescence_blue->default_value )
          ->set_trigger_spell( talent.iridescence );

  buff.iridescence_red = make_buff<e_buff_t>( this, "iridescence_red", find_spell( 386353 ) )
                             ->set_trigger_spell( talent.iridescence )
                             ->set_default_value_from_effect( 1 );
  buff.iridescence_red->set_initial_stack( buff.iridescence_red->max_stack() );

  buff.limitless_potential = make_buff<e_buff_t>( this, "limitless_potential", find_spell( 394402 ) )
                                 ->set_trigger_spell( sets->set( EVOKER_DEVASTATION, T29, B2 ) )
                                 ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
                                 ->set_pct_buff_type( STAT_PCT_BUFF_CRIT );

  buff.power_swell = make_buff<e_buff_t>( this, "power_swell", find_spell( 376850 ) )
                         ->set_trigger_spell( talent.power_swell )
                         ->set_affects_regen( true )
                         ->set_default_value_from_effect_type( A_MOD_POWER_REGEN_PERCENT );

  buff.snapfire = make_buff<e_buff_t>( this, "snapfire", talent.snapfire->effectN( 1 ).trigger() )
                      ->set_chance( talent.snapfire->effectN( 1 ).percent() )
                      ->set_default_value_from_effect( 2 )
                      ->set_stack_change_callback( [ this ]( buff_t* b, int, int new_ ) {
                        if ( new_ )
                          cooldown.firestorm->adjust( b->data().effectN( 3 ).time_value() );
                      } );

  buff.reactive_hide =
      make_buff<e_buff_t>( this, "reactive_hide", talent.reactive_hide_buff )->set_default_value_from_effect( 1, 0.01 );

  buff.time_skip = make_buff<buffs::time_skip_t>( this );

  buff.feed_the_flames_stacking = make_buff<e_buff_t>( this, "feed_the_flames", find_spell( 405874 ) );
  buff.feed_the_flames_pyre     = make_buff<e_buff_t>( this, "feed_the_flames_pyre", talent.feed_the_flames_pyre_buff );

  if ( talent.feed_the_flames.enabled() )
  {
    buff.feed_the_flames_stacking
        ->set_max_stack( as<int>( -talent.feed_the_flames_pyre_buff->effectN( 2 ).base_value() ) )
        ->set_expire_at_max_stack( true )
        ->set_stack_change_callback( [ this ]( buff_t* b, int old, int ) {
          if ( old == b->max_stack() )
          {
            make_event( *sim, [ this ]() { buff.feed_the_flames_pyre->trigger(); } );
          }
        } );
  }

  // Preservation

  // Augmentation
  buff.ebon_might_self_buff = make_buff<e_buff_t>( this, "ebon_might_self", talent.ebon_might_self_buff )
                                  ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
  buff.ebon_might_self_buff->buff_period = timespan_t::zero();

  buff.momentum_shift = make_buff<e_buff_t>( this, "momentum_shift", find_spell( 408005 ) )
                            ->set_default_value_from_effect( 1 )
                            ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT );
}

void evoker_t::create_options()
{
  player_t::create_options();

  add_option( opt_bool( "evoker.use_clipping", option.use_clipping ) );
  add_option( opt_bool( "evoker.use_early_chaining", option.use_early_chaining ) );
  add_option( opt_float( "evoker.scarlet_overheal", option.scarlet_overheal, 0.0, 1.0 ) );
  add_option( opt_float( "evoker.heal_eb_chance", option.heal_eb_chance, 0.0, 1.0 ) );
  add_option( opt_timespan( "evoker.prepull_deep_breath_delay", option.prepull_deep_breath_delay, 0_s, 3_s ) );
  add_option(
      opt_timespan( "evoker.prepull_deep_breath_delay_stddev", option.prepull_deep_breath_delay_stddev, 0_s, 1.5_s ) );
  add_option( opt_float( "evoker.naszuro_bounce_chance", option.naszuro_bounce_chance, 0.0, 1.0 ) );
  add_option( opt_bool( "evoker.naszuro_accurate_behaviour", option.naszuro_accurate_behaviour ) );
}

void evoker_t::analyze( sim_t& sim )
{
  // For proper DPET analysis, we need to treat empowered spell stat objs as non-channelled so the dot ticks from fire
  // breath do not get summed up into total execute time. All empowered spells have a release spell that is pushed onto
  // secondary_action_list, which we can use to find stat objs for empowered actions without having to go through the
  // entire action_list. See empower_charge_spell_t::tick() for more notes.
  for ( auto a : secondary_action_list )
  {
    if ( auto emp = dynamic_cast<spells::empowered_charge_spell_t*>( a->stats->action_list[ 0 ] ) )
      range::for_each( emp->stats->action_list, []( action_t* a ) { a->channeled = false; } );
  }

  player_t::analyze( sim );
}

void evoker_t::moving()
{
  // If we are mid-empower and forced to move, we don't want player_t::interrupt() to schedule_ready as the release
  // action will handle that for us. We set the bool here and override player_t::schedule_ready to return if bool is
  // set.
  if ( channeling && dynamic_cast<spells::empowered_charge_spell_t*>( channeling ) )
    was_empowering = true;

  player_t::moving();
}

void evoker_t::schedule_ready( timespan_t delta_time, bool waiting )
{
  if ( was_empowering )
    return;

  player_t::schedule_ready( delta_time, waiting );
}

void evoker_t::reset()
{
  player_t::reset();

  // clear runtime variables
  allies_with_my_ebon.clear_without_callbacks();
  allies_with_my_prescience.clear_without_callbacks();
  allied_ebons_on_me.clear();
  last_scales_target = nullptr;
  was_empowering = false;
}

void evoker_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  option = debug_cast<evoker_t*>( source )->option;
}

void evoker_t::merge( player_t& other )
{
  player_t::merge( other );
}

std::string evoker_t::default_potion() const
{
  return evoker_apl::potion( this );
}

std::string evoker_t::default_flask() const
{
  return evoker_apl::flask( this );
}

std::string evoker_t::default_food() const
{
  return evoker_apl::food( this );
}

std::string evoker_t::default_rune() const
{
  return evoker_apl::rune( this );
}

std::string evoker_t::default_temporary_enchant() const
{
  return evoker_apl::temporary_enchant( this );
}

const evoker_td_t* evoker_t::find_target_data( const player_t* target ) const
{
  return target_data[ target ];
}

evoker_td_t* evoker_t::get_target_data( player_t* target ) const
{
  evoker_td_t*& td = target_data[ target ];
  if ( !td )
    td = new evoker_td_t( target, const_cast<evoker_t*>( this ) );
  return td;
}

void evoker_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  // Baseline Auras
  action.apply_affecting_aura( spec.evoker );
  action.apply_affecting_aura( spec.devastation );
  action.apply_affecting_aura( spec.preservation );
  action.apply_affecting_aura( spec.augmentation );

  // Class Baselines
  action.apply_affecting_aura( spec.emerald_blossom_spec );

  // Class Traits
  action.apply_affecting_aura( talent.aerial_mastery );
  action.apply_affecting_aura( talent.attuned_to_the_dream );
  action.apply_affecting_aura( talent.bountiful_bloom );
  action.apply_affecting_aura( talent.enkindled );
  action.apply_affecting_aura( talent.extended_flight );
  action.apply_affecting_aura( talent.forger_of_mountains );
  action.apply_affecting_aura( talent.lush_growth );
  action.apply_affecting_aura( talent.natural_convergence );
  action.apply_affecting_aura( talent.obsidian_bulwark );

  // Augmentation
  action.apply_affecting_aura( talent.dream_of_spring );
  action.apply_affecting_aura( talent.unyielding_domain );
  action.apply_affecting_aura( talent.volcanism );
  action.apply_affecting_aura( talent.interwoven_threads );

  // Devastaion
  action.apply_affecting_aura( talent.arcane_intensity );
  action.apply_affecting_aura( talent.dense_energy );
  action.apply_affecting_aura( talent.engulfing_blaze );
  action.apply_affecting_aura( talent.heat_wave );
  action.apply_affecting_aura( talent.honed_aggression );
  action.apply_affecting_aura( talent.imposing_presence );
  action.apply_affecting_aura( talent.lay_waste );
  action.apply_affecting_aura( talent.onyx_legacy );
  action.apply_affecting_aura( talent.spellweavers_dominance );
  action.apply_affecting_aura( talent.eye_of_infinity );
  action.apply_affecting_aura( talent.event_horizon );
  action.apply_affecting_aura( sets->set( EVOKER_DEVASTATION, T29, B2 ) );
  action.apply_affecting_aura( sets->set( EVOKER_DEVASTATION, T30, B4 ) );

  // Preservation
}

action_t* evoker_t::create_action( std::string_view name, std::string_view options_str )
{
  using namespace spells;

  if ( name == "azure_strike" )
    return new azure_strike_t( this, options_str );
  if ( name == "deep_breath" )
    return new deep_breath_t( this, options_str );
  if ( name == "disintegrate" )
    return new disintegrate_t( this, options_str );
  if ( name == "dragonrage" )
    return new dragonrage_t( this, options_str );
  if ( name == "eternity_surge" )
    return new eternity_surge_t( this, options_str );
  if ( name == "expunge" )
    return new expunge_t( this, options_str );
  if ( name == "fire_breath" )
    return new fire_breath_t( this, options_str );
  if ( name == "firestorm" )
    return new firestorm_t( this, options_str );
  if ( name == "hover" )
    return new hover_t( this, options_str );
  if ( name == "landslide" )
    return new landslide_t( this, options_str );
  if ( name == "living_flame" )
    return new living_flame_t( this, options_str );
  if ( name == "obsidian_scales" )
    return new obsidian_scales_t( this, options_str );
  if ( name == "pyre" )
    return new pyre_t( this, options_str );
  if ( name == "quell" )
    return new quell_t( this, options_str );
  if ( name == "shattering_star" )
    return new shattering_star_t( this, options_str );
  if ( name == "tip_the_scales" )
    return new tip_the_scales_t( this, options_str );
  if ( name == "verdant_embrace" )
    return new heals::verdant_embrace_t( this, options_str );
  if ( name == "emerald_blossom" )
    return new heals::emerald_blossom_t( this, options_str );
  if ( name == "ebon_might" )
    return new ebon_might_t( this, options_str );
  if ( name == "eruption" )
    return new eruption_t( this, "eruption", options_str );
  if ( name == "upheaval" )
    return new upheaval_t( this, options_str );
  if ( name == "prescience" )
    return new prescience_t( this, options_str );
  if ( name == "breath_of_eons" )
    return new breath_of_eons_t( this, options_str );
  if ( name == "blistering_scales" )
    return new blistering_scales_t( this, options_str );
  if ( name == "time_skip" )
    return new time_skip_t( this, options_str );

  return player_t::create_action( name, options_str );
}

std::unique_ptr<expr_t> evoker_t::create_expression( std::string_view expr_str )
{
  auto splits = util::string_split<std::string_view>( expr_str, "." );

  if ( util::str_compare_ci( expr_str, "essense" ) || util::str_compare_ci( expr_str, "essences" ) )
    return make_ref_expr( expr_str, resources.current[ RESOURCE_ESSENCE ] );

  if ( splits.size() >= 2 )
  {
    if ( util::str_compare_ci( splits[ 0 ], "evoker" ) )
    {
      if ( util::str_compare_ci( splits[ 1 ], "prescience_buffs" ) )
        return make_fn_expr( "prescience_buffs", [ this ] { return allies_with_my_prescience.size(); } );
      if ( util::str_compare_ci( splits[ 1 ], "ebon_buffs" ) )
        return make_fn_expr( "ebon_buffs", [ this ] { return allies_with_my_ebon.size(); } );
      if ( util::str_compare_ci( splits[ 1 ], "scales_up" ) )
        return make_fn_expr( "scales_up", [ this ] {
          return last_scales_target != nullptr &&
                 get_target_data( last_scales_target )->buffs.blistering_scales->check();
        } );
      if ( util::str_compare_ci( splits[ 1 ], "use_clipping" ) )
        return expr_t::create_constant( "use_clipping", option.use_clipping );
      if ( util::str_compare_ci( splits[ 1 ], "use_early_chaining" ) )
        return expr_t::create_constant( "use_early_chaining", option.use_early_chaining );
      throw std::invalid_argument( fmt::format( "Unsupported evoker expression '{}'.", splits[ 1 ] ) );
    }
  }

  return player_t::create_expression( expr_str );
}

// Stat & Multiplier overrides ==============================================
double evoker_t::matching_gear_multiplier( attribute_e attr ) const
{
  return attr == ATTR_INTELLECT ? 0.05 : 0.0;
}

double evoker_t::composite_base_armor_multiplier() const
{
  double a = player_t::composite_base_armor_multiplier();

  if ( spec.tempered_scales )
    a *= 1.0 + spec.tempered_scales->effectN( 1 ).percent();

  return a;
}

double evoker_t::composite_armor() const
{
  double a = player_t::composite_armor();

  return a;
}

double evoker_t::composite_base_armor() const
{
  double a = current.stats.armor;

  a *= composite_base_armor_multiplier();
  a *= composite_armor_multiplier();

  return a;
}

double evoker_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double am = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STAMINA )
    am *= 1.0 + talent.draconic_legacy->effectN( 1 ).percent();

  return am;
}

double evoker_t::composite_player_multiplier( school_e s ) const
{
  double m = player_t::composite_player_multiplier( s );

  if ( talent.instinctive_arcana.ok() && talent.instinctive_arcana->effectN( 1 ).has_common_school( s ) )
    m *= 1 + talent.instinctive_arcana->effectN( 1 ).percent();

  return m;
}

double evoker_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buff.fury_of_the_aspects )
    h *= 1.0 / ( 1.0 + buff.fury_of_the_aspects->check_value() );

  return h;
}

double evoker_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buff.fury_of_the_aspects )
    h *= 1.0 / ( 1.0 + buff.fury_of_the_aspects->check_value() );

  return h;
}

stat_e evoker_t::convert_hybrid_stat( stat_e stat ) const
{
  switch ( stat )
  {
    case STAT_STR_AGI_INT:
    case STAT_AGI_INT:
    case STAT_STR_INT:
      return STAT_INTELLECT;
    case STAT_STR_AGI:
    case STAT_SPIRIT:
    case STAT_BONUS_ARMOR:
      return STAT_NONE;
    default:
      return stat;
  }
}

void evoker_t::bounce_naszuro( player_t* s, timespan_t remains = timespan_t::min() )
{
  if ( !naszuro )
    return;

  if ( remains <= 0_s && remains != timespan_t::min() )
    return;

  player_t* p = sim->player_no_pet_list[ rng().range( sim->player_no_pet_list.size() ) ];

  // TODO: Improve target selection (CD Based)
  if ( sim->player_no_pet_list.size() > 1 )
  {
    while ( p == s )
    {
      p = sim->player_no_pet_list[ rng().range( sim->player_no_pet_list.size() ) ];
    }
  }

  get_target_data( p )->buffs.unbound_surge->trigger( remains );
}

void evoker_t::extend_ebon( timespan_t extend )
{
  if ( background_actions.ebon_might )
  {
    static_cast<spells::ebon_might_t*>( background_actions.ebon_might.get() )->extend_ebon( extend );
  }
}

double evoker_t::passive_movement_modifier() const
{
  double pmm = player_t::passive_movement_modifier();

  // hardcode 75% from spell desc; not found in spell data
  if ( talent.exuberance.ok() && health_percentage() > 75 )
    pmm += talent.exuberance->effectN( 1 ).percent();

  return pmm;
}

double evoker_t::resource_regen_per_second( resource_e resource ) const
{
  auto rrps = player_t::resource_regen_per_second( resource );

  if ( resource == RESOURCE_ESSENCE )
    rrps *= 1.0 + buff.power_swell->check_value();

  return rrps;
}

void evoker_t::target_mitigation( school_e school, result_amount_type rt, action_state_t* s )
{
  if ( buff.obsidian_scales->check() )
  {
    auto eff = buff.obsidian_scales->data().effectN( 2 );
    if ( eff.has_common_school( school ) )
      s->result_amount *= 1.0 + eff.percent();
  }

  if ( talent.inherent_resistance.ok() )
  {
    auto eff = talent.inherent_resistance->effectN( 1 );
    if ( eff.has_common_school( school ) )
      s->result_amount *= 1.0 + eff.percent();
  }

  player_t::target_mitigation( school, rt, s );
}

double evoker_t::temporary_movement_modifier() const
{
  auto tmm = player_t::temporary_movement_modifier();

  // TODO: confirm hover is a non-stacking temporary movement mod
  if ( buff.hover->check() )
    tmm = std::max( tmm, buff.hover->check_value() + buff.tailwind->check_value() );

  return tmm;
}

// Utility functions ========================================================

// Utility function to search spell data for matching effect.
// NOTE: This will return the FIRST effect that matches parameters.
const spelleffect_data_t* evoker_t::find_spelleffect( const spell_data_t* spell, effect_subtype_t subtype,
                                                      int misc_value, const spell_data_t* affected, effect_type_t type )
{
  for ( size_t i = 1; i <= spell->effect_count(); i++ )
  {
    const auto& eff = spell->effectN( i );

    if ( affected->ok() && !affected->affected_by_all( eff ) )
      continue;

    if ( eff.type() == type && ( eff.subtype() == subtype || subtype == A_MAX ) &&
         ( eff.misc_value1() == misc_value || misc_value == 0 ) )
    {
      return &eff;
    }
  }

  return &spelleffect_data_t::nil();
}

// Return the appropriate spell when `base` is overriden to another spell by `passive`
const spell_data_t* evoker_t::find_spell_override( const spell_data_t* base, const spell_data_t* passive )
{
  if ( !passive->ok() || !base->ok() )
    return base;

  auto id = as<unsigned>( find_spelleffect( passive, A_OVERRIDE_ACTION_SPELL, base->id() )->base_value() );
  if ( !id )
    return base;

  return find_spell( id );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class evoker_report_t : public player_report_extension_t
{
public:
  evoker_report_t( evoker_t& player ) : p( player )
  {
  }

  void html_customsection( report::sc_html_stream& /*os*/ ) override
  {
  }

private:
  evoker_t& p;
};

// EVOKER MODULE INTERFACE ==================================================
struct evoker_module_t : public module_t
{
  evoker_module_t() : module_t( EVOKER )
  {
  }

  player_t* create_player( sim_t* sim, std::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p              = new evoker_t( sim, name, r );
    p->report_extension = std::make_unique<evoker_report_t>( *p );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void init( player_t* /* p */ ) const override
  {
  }

  void static_init() const override
  {
    unique_gear::register_special_effect( 394927, karnalex_the_first_light );
    unique_gear::register_special_effect( 405061, insight_of_naszuro );
  }

  void register_hotfixes() const override
  {
  }

  void combat_begin( sim_t* ) const override
  {
  }

  void create_actions( player_t* p ) const override
  {
    if ( p->is_enemy() || p->type == HEALING_ENEMY || p->is_pet() )
      return;

    new spells::infernos_blessing_t( p );
    new spells::blistering_scales_damage_t( p );
    new spells::fate_mirror_damage_t( p );
    new spells::breath_of_eons_damage_t( p );
  }


  void combat_end( sim_t* ) const override
  {
  }
};
}  // namespace

const module_t* module_t::evoker()
{
  static evoker_module_t m;
  return &m;
}
