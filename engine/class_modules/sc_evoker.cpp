// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"

#include "action/parse_buff_effects.hpp"
#include "class_modules/apl/apl_evoker.hpp"
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
  } debuffs;

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
  bool was_empowering;
  // !!!===========================================================================!!!

  // Options
  struct options_t
  {
    // Should every Disintegrate in Dragonrage be clipped after the 3rd tick
    bool use_clipping       = true;
    // Should chained Disintegrates( those with 5 ticks ) be chained after the 3rd tick in Dragonrage
    bool use_early_chaining = true;
    double scarlet_overheal = 0.5;
    double ancient_flame_chance = 0.05;
    double heal_eb_chance = 0.9;
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
    

    // Preservation
  } buff;

  // Specialization Spell Data
  struct specializations_t
  {
    // Baseline
    const spell_data_t* evoker;        // evoker class aura
    const spell_data_t* devastation;   // devastation class aura
    const spell_data_t* preservation;  // preservation class aura

    const spell_data_t* living_flame_damage;
    const spell_data_t* living_flame_heal;
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
    player_talent_t tempered_scales;
    player_talent_t extended_flight;
    player_talent_t bountiful_bloom;
    player_talent_t blast_furnace;  // row 7
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
  } cooldown;

  // Gains
  struct gains_t
  {
    propagate_const<gain_t*> eye_of_infinity;
    propagate_const<gain_t*> roar_of_exhilaration;
  } gain;

  // Procs
  struct procs_t
  {
    propagate_const<proc_t*> ruby_essence_burst;
    propagate_const<proc_t*> azure_essence_burst;
  } proc;

  // RPPMs
  struct rppms_t
  {
  } rppm;

  // Uptimes
  struct uptimes_t
  {
  } uptime;

  evoker_t( sim_t* sim, std::string_view name, race_e r = RACE_DRACTHYR_HORDE );

  // Character Definitions
  void init_action_list() override;
  void init_base_stats() override;
  // void init_resources( bool ) override;
  void init_benefits() override;
  resource_e primary_resource() const override { return RESOURCE_ESSENCE; }
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
  double composite_armor() const override;
  double composite_attribute_multiplier( attribute_e ) const override;
  double composite_player_multiplier( school_e ) const override;
  double composite_spell_haste() const override;
  double composite_melee_haste() const override;
  stat_e convert_hybrid_stat( stat_e ) const override;
  double passive_movement_modifier() const override;
  double resource_regen_per_second( resource_e ) const override;
  void target_mitigation( school_e, result_amount_type, action_state_t* ) override;
  double temporary_movement_modifier() const override;

  // Utility functions
  const spelleffect_data_t* find_spelleffect( const spell_data_t* spell, effect_subtype_t subtype,
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

  evoker_t* p()
  {
    return static_cast<evoker_t*>( bb::source );
  }

  const evoker_t* p() const
  {
    return static_cast<evoker_t*>( bb::source );
  }
};
}  // namespace buffs

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
    parse_buff_effects( p()->buff.essence_burst );
    parse_buff_effects( p()->buff.snapfire );
    parse_buff_effects( p()->buff.tip_the_scales );

    parse_buff_effects( p()->buff.imminent_destruction );
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
      stored += s->result_raw * p()->talent.scarlet_adaptation->effectN( 1 ).percent();
      // TODO: confirm if this always matches living flame SP coeff
      stored = std::min( stored, p()->cache.spell_power( SCHOOL_MAX ) * scarlet_adaptation_sp_cap * ( 1 - p()->option.scarlet_overheal ));
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
}  // namespace heals

namespace spells
{

// Base Classes =============================================================

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
        for ( auto t : sim->target_non_sleeping_list )
        {
          td( t )->dots.fire_breath->adjust_duration(
              timespan_t::from_seconds( as<int>( p()->talent.everburning_flame->effectN( 1 ).base_value() ) ), 0_ms, -1,
              false );
        }
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    trigger_charged_blast( s );
    trigger_everburning_flame( s );

    if ( !ab::background && !ab::dual )
    {
      // These happen after any secondary spells are executed, so we schedule as events
      if ( spell_color == SPELL_BLUE )
        make_event( *sim, [ this ]() { p()->buff.iridescence_blue->decrement(); } );
      else if ( spell_color == SPELL_RED )
        make_event( *sim, [ this ]() { p()->buff.iridescence_red->decrement(); } );
    }
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

struct empower_data_t
{
  empower_e empower;

  friend void sc_format_to( const empower_data_t& data, fmt::format_context::iterator out )
  {
    fmt::format_to( out, "empower_level={}", static_cast<int>( data.empower ) );
  }
};

struct empowered_base_t : public evoker_spell_t
{
protected:
  using state_t = evoker_action_state_t<empower_data_t>;

public:
  empower_e max_empower;

  empowered_base_t( std::string_view name, evoker_t* p, const spell_data_t* spell, std::string_view options_str = {} )
    : evoker_spell_t( name, p, spell, options_str ),
      max_empower( p->talent.font_of_magic.ok() ? empower_e::EMPOWER_4 : empower_e::EMPOWER_3 )
  {
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
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

struct empowered_release_spell_t : public empowered_base_t
{
  using base_t = empowered_release_spell_t;

  timespan_t extend_tier29_4pc;

  empowered_release_spell_t( std::string_view name, evoker_t* p, const spell_data_t* spell )
    : empowered_base_t( name, p, spell )
  {
    dual = true;

    // TODO: Continue to check it uses this spell to trigger GCD, as of 28/10/2022 it does. It can still be bypassed via
    // spell queue. Potentally add a better way to model this?
    const spell_data_t* gcd_spell = p->find_spell( 359115 );
    if ( gcd_spell )
      trigger_gcd = gcd_spell->gcd();
    gcd_type = gcd_haste_type::NONE;

    extend_tier29_4pc = timespan_t::from_seconds( p->sets->set( EVOKER_DEVASTATION, T29, B4 )->effectN( 1 ).base_value() );
  }

  empower_e empower_level( const action_state_t* s ) const
  {
    return cast_state( s )->empower;
  }

  int empower_value( const action_state_t* s ) const
  {
    return static_cast<int>( cast_state( s )->empower );
  }

  void execute() override
  {
    p()->was_empowering = false;

    empowered_base_t::execute();

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

struct empowered_charge_spell_t : public empowered_base_t
{
  using base_t = empowered_charge_spell_t;

  action_t* release_spell;
  stats_t* dummy_stat;  // used to hack channel tick time into execute time
  stats_t* orig_stat;
  int empower_to;
  timespan_t base_empower_duration;
  timespan_t lag;

  empowered_charge_spell_t( std::string_view name, evoker_t* p, const spell_data_t* spell,
                            std::string_view options_str )
    : empowered_base_t( name, p, p->find_spell_override( spell, p->talent.font_of_magic ) ),
      release_spell( nullptr ),
      dummy_stat( p->get_stats( "dummy_stat" ) ),
      orig_stat( stats ),
      empower_to( max_empower ),
      base_empower_duration( 0_ms ),
      lag( 0_ms )
  {
    channeled = true;

    // TODO: convert to full empower expression support
    add_option( opt_int( "empower_to", empower_to, EMPOWER_1, EMPOWER_MAX ) );

    parse_options( options_str );

    empower_to = std::min( static_cast<int>( max_empower ), empower_to );

    dot_duration = base_tick_time = base_empower_duration =
        base_time_to_empower( static_cast<empower_e>( empower_to ) );

  }

  template <typename T>
  void create_release_spell( std::string_view n )
  {
    static_assert( std::is_base_of_v<empowered_release_spell_t, T>,
                   "Empowered release spell must be dervied from empowered_release_spell_t." );

    release_spell             = p()->get_secondary_action<T>( n );
    release_spell->stats      = stats;
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
    return base_time_to_empower( max_empower ) + 2_s;
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    // we need to have the tick time match duration.
    // NOTE: composite_dot_duration CANNOT reference parent method as spell_t::composite_dot_duration calls tick_time()
    return composite_dot_duration( s );
  }

  timespan_t base_composite_dot_duration( const action_state_t* s ) const
  {
    return dot_duration * s->haste * get_buff_effects_value( dot_duration_buffeffects );
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

    return std::min( max_empower, emp );
  }

  void init() override
  {
    empowered_base_t::init();
    assert( release_spell && "Empowered charge spell must have a release spell." );
  }

  void execute() override
  {
    // pre-determine lag here per every execute
    lag = rng().gauss( sim->channel_lag, sim->channel_lag_stddev );

    empowered_base_t::execute();
  }

  void tick( dot_t* d ) override
  {
    // For proper DPET analysis, we need to treat charge spells as non-channel, since channelled spells sum up tick
    // times to get the execute time, but this does not work for fire breath which also has a dot component. Instead we
    // hijack the stat obj during action_t:tick() causing the channel's tick to be recorded onto a throwaway stat obj.
    // We then record the corresponding tick time as execute time onto the original real stat obj. See further notes in
    // evoker_t::analyze().
    stats = dummy_stat;
    empowered_base_t::tick( d );
    stats = orig_stat;

    stats->iteration_total_execute_time += d->time_to_tick();
  }

  void last_tick( dot_t* d ) override
  {
    empowered_base_t::last_tick( d );

    if ( empower_level( d ) == empower_e::EMPOWER_NONE )
    {
      p()->was_empowering = false;
      return;
    }

    auto emp_state    = release_spell->get_state();
    emp_state->target = d->state->target;
    release_spell->snapshot_state( emp_state, release_spell->amount_type( emp_state ) );

    if ( p()->buff.tip_the_scales->up() )
    {
      p()->buff.tip_the_scales->expire();
      cast_state( emp_state )->empower = max_empower;
    }
    else
      cast_state( emp_state )->empower = empower_level( d );

    release_spell->schedule_execute( emp_state );

    // hack to prevent dot_t::last_tick() from schedule_ready()'ing the player
    d->current_action = release_spell;
    // hack to prevent channel lag being added when player is schedule_ready()'d after the release spell execution
    p()->last_foreground_action = release_spell;
  }
};

struct essence_spell_t : public evoker_spell_t
{
  timespan_t ftf_dur;
  double hoarded_pct;
  double titanic_mul;
  double obsidian_shards_mul;

  essence_spell_t( std::string_view n, evoker_t* p, const spell_data_t* s, std::string_view o = {} )
    : evoker_spell_t( n, p, s, o ),
      ftf_dur( -timespan_t::from_seconds( p->talent.feed_the_flames->effectN( 1 ).base_value() ) ),
      hoarded_pct( p->talent.hoarded_power->effectN( 1 ).percent() ),
      titanic_mul( p->talent.titanic_wrath->effectN( 1 ).percent() ),
      obsidian_shards_mul( p->sets->set( EVOKER_DEVASTATION, T30, B2 )->effectN( 1 ).percent() )
  {
  }

  void consume_resource() override
  {
    evoker_spell_t::consume_resource();

    if ( !base_cost() || proc )
      return;

    if ( p()->buff.essence_burst->up() )
    {
      if ( !rng().roll( hoarded_pct ) )
        p()->buff.essence_burst->decrement();
    }
  }
};

// Empowered Spells =========================================================

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
};

struct deep_breath_t : public evoker_spell_t
{
  struct deep_breath_dot_t : public evoker_spell_t
  {
    deep_breath_dot_t( evoker_t* p ) : evoker_spell_t( "deep_breath_dot", p, p->find_spell( 353759 ) )
    {
      travel_delay = 0.9;   // guesstimate, TODO: confirm
      travel_speed = 19.5;  // guesstimate, TODO: confirm
      aoe          = -1;
    }

    timespan_t travel_time() const override
    {
      // we set the execute_time of the base action as travel_time of the damage action so they match, but when we
      // actually execute the damage action, we want travel_time to be 0_ms since it is already accounted for in the
      // base action.
      return execute_state && execute_state->target ? 0_ms : evoker_spell_t::travel_time();
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

  deep_breath_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "deep_breath", p, p->find_class_spell( "Deep Breath" ), options_str )
  {
    damage        = p->get_secondary_action<deep_breath_dot_t>( "deep_breath_dot" );
    damage->stats = stats;
  }

  timespan_t execute_time() const override
  {
    // TODO: Work out a better solution for this.
    // return damage->travel_time();
    return 3_s;
  }

  void execute() override
  {
    evoker_spell_t::execute();

    damage->execute_on_target( target );
  }
};

struct disintegrate_t : public essence_spell_t
{
  eternity_surge_t::eternity_surge_damage_t* eternity_surge;
  int num_ticks;

  disintegrate_t( evoker_t* p, std::string_view options_str )
    : essence_spell_t( "disintegrate", p, p->find_class_spell( "Disintegrate" ), options_str ),
      num_ticks( as<int>( dot_duration / base_tick_time ) + 1 )
  {
    channeled = tick_zero = true;

    auto surge = p->get_secondary_action<eternity_surge_t::eternity_surge_damage_t>( "scintillation", "scintillation" );
    surge->s_data_reporting   = p->talent.scintillation;
    surge->name_str_reporting = "scintillation";
    surge->proc_spell_type    = proc_spell_type_e::SCINTILLATION;
    eternity_surge            = surge;

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

    living_flame_base_t( std::string_view n, evoker_t* p, const spell_data_t* s ) : Base( n, p, s )
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

      if ( rng().roll( p()->option.ancient_flame_chance ) )
        p()->buff.ancient_flame->trigger();
    }
  };

  action_t* damage;
  action_t* heal;
  double gcd_mul;
  bool cast_heal;

  living_flame_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "living_flame", p, p->find_class_spell( "Living Flame" ) ),
      gcd_mul( p->find_spelleffect( &p->buff.ancient_flame->data(), A_ADD_PCT_MODIFIER, P_GCD, &data() )->percent() ),
      cast_heal( false )
  {
    damage        = p->get_secondary_action<living_flame_damage_t>( "living_flame_damage" );
    damage->stats = stats;

    // TODO: implement option to cast heal instead
    heal = p->get_secondary_action<living_flame_heal_t>( "living_flame_heal" );

    add_option( opt_bool( "heal", cast_heal ) );
    parse_options( options_str );
  }

  bool has_amount_result() const override
  {
    return damage->has_amount_result();
  }

  timespan_t gcd() const override
  {
    auto g = evoker_spell_t::gcd();

    if ( p()->buff.ancient_flame->check() )
      g *= 1.0 + gcd_mul;

    return std::max( min_gcd, g );
  }

  void execute() override
  {
    evoker_spell_t::execute();

    damage->execute_on_target( target );

    int total_hits = damage->num_targets_hit;

    p()->buff.ancient_flame->expire();
    p()->buff.scarlet_adaptation->expire();

    if ( p()->buff.leaping_flames->up() && damage->num_targets_hit <= p()->buff.leaping_flames->check() )
    {
      p()->buff.leaping_flames->decrement( damage->num_targets_hit - 1 );
      heal->execute_on_target( p() );
      for ( int i = 0; i < 1 + p()->buff.leaping_flames->check(); i++ )
      {
        if ( rng().roll( p()->option.heal_eb_chance ) )
          total_hits += 1;
      }
    }

    p()->buff.leaping_flames->expire();

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
  {}

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
    if ( p()->talent.feed_the_flames.enabled() &&
         ( proc_spell_type & proc_spell_type_e::VOLATILITY ) == 0 )
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

}  // end namespace spells

// ==========================================================================
// Evoker Character Definitions
// ==========================================================================

evoker_td_t::evoker_td_t( player_t* target, evoker_t* evoker )
  : actor_target_data_t( target, evoker ), dots(), debuffs()
{
  dots.fire_breath  = target->get_dot( "fire_breath_damage", evoker );
  dots.disintegrate = target->get_dot( "disintegrate", evoker );

  debuffs.shattering_star = make_buff( *this, "shattering_star_debuff", evoker->talent.shattering_star )
                                ->set_cooldown( 0_ms )
                                ->apply_affecting_aura( evoker->talent.focusing_iris );

  debuffs.in_firestorm = make_buff( *this, "in_firestorm" )->set_max_stack( 20 )->set_duration( timespan_t::zero() );
}

evoker_t::evoker_t( sim_t* sim, std::string_view name, race_e r )
  : player_t( sim, EVOKER, name, r ),
    was_empowering( false ),
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
    default:
      evoker_apl::no_spec( this );
      break;
  }

  use_default_action_list = true;

  player_t::init_action_list();
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
}

void evoker_t::init_procs()
{
  player_t::init_procs();

  proc.ruby_essence_burst  = get_proc( "Ruby Essence Burst" );
  proc.azure_essence_burst = get_proc( "Azure Essence Burst" );
}

void evoker_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = 25;

  player_t::init_base_stats();

  base.spell_power_per_intellect = 1.0;

  resources.base[ RESOURCE_ESSENCE ] = 5 + find_spelleffect( talent.power_nexus, A_MOD_MAX_RESOURCE )->base_value();
  // TODO: confirm base essence regen. currently estimated at 1 per 5s base
  resources.base_regen_per_second[ RESOURCE_ESSENCE ] = 0.2 * ( 1.0 + talent.innate_magic->effectN( 1 ).percent() );
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
  talent.tempered_scales      = CT( "Tempered Scales" );
  talent.extended_flight      = CT( "Extended Flight" );
  talent.bountiful_bloom      = CT( "Bountiful Bloom" );
  talent.blast_furnace        = CT( "Blast Furnace" );  // Row 7
  talent.exuberance           = CT( "Exuberance" );
  talent.ancient_flame        = CT( "Ancient Flame" );
  talent.protracted_talons    = CT( "Protracted Talons" );  // Row 8
  talent.lush_growth          = CT( "Lush Growth" );
  talent.leaping_flames       = CT( "Leaping Flames" );  // Row 9
  talent.aerial_mastery       = CT( "Aerial Mastery" );
  // Devastation Traits
  talent.pyre                        = ST( "Pyre" );                // Row 1
  talent.ruby_essence_burst          = ST( "Ruby Essence Burst" );  // Row 2
  talent.azure_essence_burst         = ST( "Azure Essence Burst" );
  talent.dense_energy                = ST( "Dense Energy" );  // Row 3
  talent.imposing_presence           = ST( "Imposing Presence" );
  talent.eternity_surge              = ST( "Eternity Surge" );
  talent.volatility                  = ST( "Volatility" );  // Row 4
  talent.power_nexus                 = ST( "Power Nexus" );
  talent.dragonrage                  = ST( "Dragonrage" );
  talent.lay_waste                   = ST( "Lay Waste" );
  talent.arcane_intensity            = ST( "Arcane Intensity" );
  talent.ruby_embers                 = ST( "Ruby Embers" );  // Row 5
  talent.engulfing_blaze             = ST( "Engulfing Blaze" );
  talent.animosity                   = ST( "Animosity" );
  talent.essence_attunement          = ST( "Essence Attunement" );
  talent.firestorm                   = ST( "Firestorm" );  // Row 6
  talent.heat_wave                   = ST( "Heat Wave" );
  talent.titanic_wrath               = ST( "Titanic Wrath" );
  talent.honed_aggression            = ST( "Honed Aggression" );
  talent.eternitys_span              = ST( "Eternity's Span" );
  talent.eye_of_infinity             = ST( "Eye of Infinity" );
  talent.event_horizon               = ST( "Event Horizon" );
  talent.causality                   = ST( "Causality" );
  talent.catalyze                    = ST( "Catalyze" );  // Row 7
  talent.tyranny                     = ST( "Tyranny" );
  talent.charged_blast               = ST( "Charged Blast" );
  talent.shattering_star             = ST( "Shattering Star" );
  talent.snapfire                    = ST( "Snapfire" );  // Row 8
  talent.raging_inferno              = ST( "Raging Inferno" );
  talent.font_of_magic               = ST( "Font of Magic" );
  talent.onyx_legacy                 = ST( "Onyx Legacy" );
  talent.spellweavers_dominance      = ST( "Spellweaver's Dominance" );
  talent.focusing_iris               = ST( "Focusing Iris" );
  talent.arcane_vigor                = ST( "Arcane Vigor" );
  talent.burnout                     = ST( "Burnout" );  // Row 9
  talent.imminent_destruction        = ST( "Imminent Destruction" );
  talent.scintillation               = ST( "Scintillation" );
  talent.power_swell                 = ST( "Power Swell" );
  talent.feed_the_flames             = ST( "Feed the Flames" );  // Row 10
  talent.feed_the_flames_pyre_buff   = find_spell( 411288 );
  talent.everburning_flame           = ST( "Everburning Flame" );
  talent.hoarded_power               = ST( "Hoarded Power" );
  talent.iridescence                 = ST( "Iridescence" );
  // Preservation Traits

  // Evoker Specialization Spells
  spec.evoker              = find_spell( 353167 );  // TODO: confirm this is the class aura
  spec.devastation         = find_specialization_spell( "Devastation Evoker" );
  spec.preservation        = find_specialization_spell( "Preservation Evoker" );
  spec.living_flame_damage = find_spell( 361500 );
  spec.living_flame_heal   = find_spell( 361509 );
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

  // Baseline Abilities
  buff.essence_burst =
      make_buff( this, "essence_burst", find_spell( specialization() == EVOKER_DEVASTATION ? 359618 : 369299 ) )
          ->apply_affecting_aura( talent.essence_attunement );

  buff.essence_burst_titanic_wrath_disintegrate =
      make_buff( this, "essence_burst_titanic_wrath_disintegrate", find_spell( 397870 ) )
          ->set_quiet( true )
          ->set_trigger_spell( talent.titanic_wrath );

  buff.hover = make_buff( this, "hover", find_class_spell( "Hover" ) )
                   ->set_cooldown( 0_ms )
                   ->set_default_value_from_effect_type( A_MOD_INCREASE_SPEED );

  buff.tailwind = make_buff( this, "tailwind", find_spelleffect( talent.tailwind, A_PROC_TRIGGER_SPELL )->trigger() )
                      ->set_default_value_from_effect( 1 );

  // Class Traits
  buff.ancient_flame =
      make_buff( this, "ancient_flame", find_spell( 375583 ) )->set_trigger_spell( talent.ancient_flame );

  buff.leaping_flames =
      make_buff( this, "leaping_flames", find_spell( 370901 ) )->set_trigger_spell( talent.leaping_flames );

  buff.obsidian_scales = make_buff( this, "obsidian_scales", talent.obsidian_scales )->set_cooldown( 0_ms );

  buff.scarlet_adaptation =
      make_buff( this, "scarlet_adaptation", find_spell( 372470 ) )->set_trigger_spell( talent.scarlet_adaptation );

  buff.tip_the_scales = make_buff( this, "tip_the_scales", talent.tip_the_scales )->set_cooldown( 0_ms );

  // Devastation
  buff.blazing_shards = make_buff( this, "blazing_shards", find_spell( 409848 ) )
                            ->set_trigger_spell( sets->set( EVOKER_DEVASTATION, T30, B4 ) );

  buff.burnout = make_buff( this, "burnout", find_spell( 375802 ) )
                     ->set_trigger_spell( talent.burnout )
                     ->set_cooldown( talent.burnout->internal_cooldown() )
                     ->set_chance( talent.burnout->effectN( 1 ).percent() );

  buff.charged_blast = make_buff( this, "charged_blast", talent.charged_blast->effectN( 1 ).trigger() )
                           ->set_default_value_from_effect( 1 );

  buff.dragonrage = make_buff( this, "dragonrage", talent.dragonrage )->set_cooldown( 0_ms );


  buff.fury_of_the_aspects = make_buff( this, "fury_of_the_aspects", find_class_spell( "Fury of the Aspects" ) )
                                 ->set_default_value_from_effect( 1 )
                                 ->set_cooldown( 0_s )
                                 ->add_invalidate( CACHE_HASTE );

  buff.imminent_destruction = make_buff( this, "imminent_destruction", find_spell( 411055 ) );

  buff.iridescence_blue = make_buff( this, "iridescence_blue", find_spell( 386399 ) )
                              ->set_trigger_spell( talent.iridescence )
                              ->set_default_value_from_effect( 1 );
  buff.iridescence_blue->set_initial_stack( buff.iridescence_blue->max_stack() );

  buff.iridescence_blue_disintegrate = make_buff( this, "iridescence_blue_disintegrate", find_spell( 399370 ) )
                                           ->set_quiet( true )
                                           ->set_default_value( buff.iridescence_blue->default_value )
                                           ->set_trigger_spell( talent.iridescence );

  buff.iridescence_red = make_buff( this, "iridescence_red", find_spell( 386353 ) )
                             ->set_trigger_spell( talent.iridescence )
                             ->set_default_value_from_effect( 1 );
  buff.iridescence_red->set_initial_stack( buff.iridescence_red->max_stack() );

  buff.limitless_potential = make_buff( this, "limitless_potential", find_spell( 394402 ) )
                                 ->set_trigger_spell( sets->set( EVOKER_DEVASTATION, T29, B2 ) )
                                 ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
                                 ->set_pct_buff_type( STAT_PCT_BUFF_CRIT );

  buff.power_swell = make_buff( this, "power_swell", find_spell( 376850 ) )
                         ->set_trigger_spell( talent.power_swell )
                         ->set_affects_regen( true )
                         ->set_default_value_from_effect_type( A_MOD_POWER_REGEN_PERCENT );

  buff.snapfire = make_buff( this, "snapfire", talent.snapfire->effectN( 1 ).trigger() )
                      ->set_chance( talent.snapfire->effectN( 1 ).percent() )
                      ->set_default_value_from_effect( 2 )
                      ->set_stack_change_callback( [ this ]( buff_t* b, int, int new_ ) {
                        if ( new_ )
                          cooldown.firestorm->adjust( b->data().effectN( 3 ).time_value() );
                      } );


  buff.feed_the_flames_stacking = make_buff( this, "feed_the_flames", find_spell( 405874 ) );
  buff.feed_the_flames_pyre     = make_buff( this, "feed_the_flames_pyre", talent.feed_the_flames_pyre_buff );

  if ( talent.feed_the_flames.enabled() )
  {
    buff.feed_the_flames_stacking->set_max_stack( -talent.feed_the_flames_pyre_buff->effectN( 2 ).base_value() )
        ->set_expire_at_max_stack( true )
        ->set_stack_change_callback( [ this ]( buff_t* b, int old, int ) {
          if ( old == b->max_stack() )
          {
            make_event( *sim, [ this ]() { buff.feed_the_flames_pyre->trigger(); } );
          }
        } );
  }

  // Preservation
}

void evoker_t::create_options()
{
  player_t::create_options();

  add_option( opt_bool( "evoker.use_clipping", option.use_clipping ) );
  add_option( opt_bool( "evoker.use_early_chaining", option.use_early_chaining ) );
  add_option( opt_float( "evoker.scarlet_overheal", option.scarlet_overheal, 0.0, 1.0 ) );
  add_option( opt_float( "evoker.ancient_flame_chance", option.ancient_flame_chance, 0.0, 1.0 ) );
  add_option( opt_float( "evoker.heal_eb_chance", option.heal_eb_chance, 0.0, 1.0 ) );
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
  action.apply_affecting_aura( talent.font_of_magic );
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

double evoker_t::composite_armor() const
{
  double a = player_t::composite_armor();

  if ( buff.obsidian_scales->check() )
    a *= 1.0 + buff.obsidian_scales->data().effectN( 1 ).percent();

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

  if ( talent.tempered_scales.ok() )
  {
    auto eff = talent.tempered_scales->effectN( 1 );
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

    if ( eff.type() == type && eff.subtype() == subtype )
    {
      if ( misc_value != 0 )
      {
        if ( eff.misc_value1() == misc_value )
          return &eff;
      }
      else
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
  void init( player_t* ) const override
  {
  }
  void static_init() const override
  {
    unique_gear::register_special_effect( 394927, karnalex_the_first_light );
  }
  void register_hotfixes() const override
  {
  }
  void combat_begin( sim_t* ) const override
  {
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
