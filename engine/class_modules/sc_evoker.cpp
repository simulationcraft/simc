// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"
#include "simulationcraft.hpp"
#include "class_modules/apl/apl_evoker.hpp"

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
  EMPOWER_1 = 1,
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

struct empowered_state_t : public action_state_t
{
  empower_e empower;

  empowered_state_t( action_t* a, player_t* t ) : action_state_t( a, t ), empower( empower_e::EMPOWER_NONE ) {}

  void initialize() override
  {
    action_state_t::initialize();
    empower = empower_e::EMPOWER_NONE;
  }

  void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );
    empower = debug_cast<const empowered_state_t*>( s )->empower;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s ) << " empower_level=" << static_cast<int>( empower );
    return s;
  }
};

struct evoker_td_t : public actor_target_data_t
{
  struct dots_t
  {

  } dots;

  struct debuffs_t
  {
    buff_t* shattering_star;
  } debuffs;

  evoker_td_t( player_t* target, evoker_t* source );
};

struct evoker_t : public player_t
{
  // Options
  struct options_t
  {

  } option;

  // Action pointers
  struct actions_t
  {

  } action;

  // Buffs
  struct buffs_t
  {
    // Baseline Abilities
    propagate_const<buff_t*> essence_burst;
    // Class Traits
    // Devastation Traits
    // Preservation Traits
  } buff;

  // Specialization Spell Data
  struct specializations_t
  {
    // Baseline
    const spell_data_t* evoker;        // evoker class aura
    const spell_data_t* devastation;   // devastation class aura
    const spell_data_t* preservation;  // preservation class aura
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
    player_talent_t rescue;
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
    player_talent_t suffused_with_power;
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
    player_talent_t fly_with_me;
    player_talent_t lush_growth;
    player_talent_t renewing_blaze;
    player_talent_t leaping_flames;  // row 9
    player_talent_t overawe;
    player_talent_t aerial_mastery;
    player_talent_t twin_guardian;
    player_talent_t pyrexia;
    player_talent_t fire_within;
    player_talent_t terror_of_the_skies;  // row 10
    player_talent_t time_spiral;
    player_talent_t zephyr;

    // Devastation Traits
    player_talent_t pyre;  // row 1
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
    player_talent_t arcane_instability;
    player_talent_t ruby_embers;  // row 5
    player_talent_t engulfing_blaze;
    player_talent_t animosity;
    player_talent_t essence_attunement;
    player_talent_t firestorm;  // row 6
    player_talent_t heat_wave;
    player_talent_t might_of_the_aspects;
    player_talent_t honed_aggression;
    player_talent_t eternitys_span;
    player_talent_t continuum;
    player_talent_t casuality;
    player_talent_t catalyze;  // row 7
    player_talent_t ruin;
    player_talent_t charged_blast;
    player_talent_t shattering_star;
    player_talent_t snapfire;  // row 8
    player_talent_t font_of_magic;
    player_talent_t onyx_legacy;
    player_talent_t tyranny;
    player_talent_t focusing_iris;
    player_talent_t arcane_vigor;
    player_talent_t burnout;  // row 9
    player_talent_t imminent_destruction;
    player_talent_t scintillation;
    player_talent_t power_swell;
    player_talent_t feed_the_flames;  // row 10
    player_talent_t everburning_flame;
    player_talent_t cascading_power;
    player_talent_t iridescence;

    // Preservation Traits
  } talent;

  // Benefits
  struct benefits_t
  {

  } benefit;

  // Cooldowns
  struct cooldowns_t
  {

  } cooldown;

  // Gains
  struct gains_t
  {

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

  evoker_t( sim_t * sim, std::string_view name, race_e r = RACE_DRACTHYR_HORDE );

  // Character Definitions
  void init_action_list() override;
  void init_base_stats() override;
  //void init_resources( bool ) override;
  //void init_benefits() override;
  //void init_gains() override;
  void init_procs() override;
  //void init_rng() override;
  //void init_uptimes() override;
  void init_spells() override;
  //void init_finished() override;
  void create_actions() override;
  void create_buffs() override;
  void create_options() override;
  //void arise() override;
  //void combat_begin() override;
  //void combat_end() override;
  //void reset() override;
  void copy_from( player_t* ) override;
  void merge( player_t& ) override;

  double composite_player_multiplier( school_e ) const override;

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
  stat_e convert_hybrid_stat( stat_e ) const override;
  double resource_regen_per_second( resource_e ) const override;

  // Utility functions
  const spelleffect_data_t* find_spelleffect( const spell_data_t* spell,
                                              effect_subtype_t subtype,
                                              int misc_value = P_GENERIC,
                                              const spell_data_t* affected = spell_data_t::nil(),
                                              effect_type_t type = E_APPLY_AURA );
  const spell_data_t* find_spell_override( const spell_data_t* base, const spell_data_t* passive );

  std::vector<action_t*> secondary_action_list;

  template <typename T, typename... Ts>
  T* get_secondary_action( std::string_view n, Ts&&... args )
  {
    auto it = range::find( secondary_action_list, n, &action_t::name_str );
    if ( it != secondary_action_list.cend() )
      return dynamic_cast<T*>( *it );

    auto a = new T( this, std::forward<Ts>( args )... );
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
  {}

  evoker_buff_t( evoker_td_t& td, std::string_view name, const spell_data_t* spell = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : bb( td, name, spell, item )
  {}

  evoker_t* p()
  { return static_cast<evoker_t*>( bb::source ); }

  const evoker_t* p() const
  { return static_cast<evoker_t*>( bb::source ); }
};
}  // namespace buffs

// Template for base evoker action code.
template <class Base>
struct evoker_action_t : public Base
{
private:
  using ab = Base;  // action base, spell_t/heal_t/etc.

public:
  // auto parsed dynamic effects
  using bfun = std::function<bool()>;
  struct buff_effect_t
  {
    buff_t* buff;
    double value;
    bool use_stacks;
    bfun func;

    buff_effect_t( buff_t* b, double v, bool s = true, bfun f = nullptr )
      : buff( b ), value( v ), use_stacks( s ), func( std::move( f ) )
    {}
  };

  using dfun = std::function<buff_t*( evoker_td_t* )>;
  struct debuff_effect_t
  {
    dfun func;
    double value;
    bool use_stacks;

    debuff_effect_t( dfun f, double v, bool b )
      : func( std::move( f ) ), value( v ), use_stacks( b )
    {}
  };

  std::vector<buff_effect_t> ta_multiplier_buffeffects;
  std::vector<buff_effect_t> da_multiplier_buffeffects;
  std::vector<buff_effect_t> execute_time_buffeffects;
  std::vector<buff_effect_t> recharge_multiplier_buffeffects;
  std::vector<buff_effect_t> cost_buffeffects;
  std::vector<buff_effect_t> crit_chance_buffeffects;
  std::vector<debuff_effect_t> target_multiplier_debuffeffects;

  spell_color_e spell_color;

  evoker_action_t( std::string_view name, evoker_t* player, const spell_data_t* spell = spell_data_t::nil() )
    : ab( name, player, spell ), spell_color( SPELL_COLOR_NONE )
  {
    // TODO: find out if there is a better data source for the spell color
    if ( ab::data().ok() )
    {
      std::string_view desc = player->dbc->spell_text( ab::data().id() ).rank();
      if ( !desc.empty() )
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
    }
  }

  evoker_t* p()
  { return static_cast<evoker_t*>( ab::player ); }

  const evoker_t* p() const
  { return static_cast<evoker_t*>( ab::player ); }

  evoker_td_t* td( player_t* t ) const
  { return p()->get_target_data( t ); }

  const evoker_td_t* find_td( const player_t* t ) const
  { return p()->find_target_data( t ); }

  template <typename T>
  void parse_spell_effects_mods( double& val, const spell_data_t* base, size_t idx, T mod )
  {
    for ( size_t i = 1; i <= mod->effect_count(); i++ )
    {
      const auto& eff = mod->effectN( i );

      if ( eff.type() != E_APPLY_AURA )
        continue;

      if ( ( base->affected_by_all( eff ) &&
             ( ( eff.misc_value1() == P_EFFECT_1 && idx == 1 ) || ( eff.misc_value1() == P_EFFECT_2 && idx == 2 ) ||
               ( eff.misc_value1() == P_EFFECT_3 && idx == 3 ) || ( eff.misc_value1() == P_EFFECT_4 && idx == 4 ) ||
               ( eff.misc_value1() == P_EFFECT_5 && idx == 5 ) ) ) ||
           ( eff.subtype() == A_PROC_TRIGGER_SPELL_WITH_VALUE && eff.trigger_spell_id() == base->id() && idx == 1 ) )
      {
        double pct = eff.percent();

        if ( eff.subtype() == A_ADD_FLAT_MODIFIER || eff.subtype() == A_ADD_FLAT_LABEL_MODIFIER )
          val += pct;
        else if ( eff.subtype() == A_ADD_PCT_MODIFIER || eff.subtype() == A_ADD_PCT_LABEL_MODIFIER )
          val *= 1.0 + pct;
        else if ( eff.subtype() == A_PROC_TRIGGER_SPELL_WITH_VALUE )
          val = pct;
        else
          continue;
      }
    }
  }

  void parse_spell_effects_mods( double&, const spell_data_t*, size_t ) {}

  template <typename T, typename... Ts>
  void parse_spell_effects_mods( double& val, const spell_data_t* base, size_t idx, T mod, Ts... mods )
  {
    parse_spell_effects_mods( val, base, idx, mod );
    parse_spell_effects_mods( val, base, idx, mods... );
  }

  // Will parse simple buffs that ONLY target the caster and DO NOT have multiple ranks
  // 1: Add Percent Modifier to Spell Direct Amount
  // 2: Add Percent Modifier to Spell Periodic Amount
  // 3: Add Percent Modifier to Spell Cast Time
  // 4: Add Percent Modifier to Spell Cooldown
  // 5: Add Percent Modifier to Spell Resource Cost
  // 6: Add Flat Modifier to Spell Critical Chance
  template <typename... Ts>
  void parse_buff_effect( buff_t* buff, bfun f, const spell_data_t* s_data, size_t i, bool use_stacks, bool use_default,
                          Ts... mods )
  {
    const auto& eff = s_data->effectN( i );
    double val      = eff.percent();

    auto debug_message = [ & ]( std::string_view type ) {
      if ( buff )
      {
        p()->sim->print_debug( "buff-effects: {} ({}) {} modified by {}% with buff {} ({}#{})", ab::name(), ab::id,
                               type, val * 100.0, buff->name(), buff->data().id(), i );
      }
      else if ( f )
      {
        p()->sim->print_debug( "conditional-effects: {} ({}) {} modified by {}% with condition from {} ({}#{})",
                               ab::name(), ab::id, type, val * 100.0, s_data->name_cstr(), s_data->id(), i );
      }
      else
      {
        p()->sim->print_debug( "passive-effects: {} ({}) {} modified by {}% from {} ({}#{})", ab::name(), ab::id, type,
                               val * 100.0, s_data->name_cstr(), s_data->id(), i );
      }
    };

    // TODO: more robust logic around 'party' buffs with radius
    if ( !( eff.type() == E_APPLY_AURA || eff.type() == E_APPLY_AREA_AURA_PARTY ) || eff.radius() ) return;

    if ( i <= 5 )
      parse_spell_effects_mods( val, s_data, i, mods... );

    if ( !ab::data().affected_by_all( eff ) )
      return;

    if ( use_default && buff)
      val = buff->default_value;

    if ( !val )
      return;

    if ( eff.subtype() == A_ADD_PCT_MODIFIER || eff.subtype() == A_ADD_PCT_LABEL_MODIFIER )
    {
      switch ( eff.misc_value1() )
      {
        case P_GENERIC:
          da_multiplier_buffeffects.emplace_back( buff, val, use_stacks, f );
          debug_message( "direct damage" );
          break;
        case P_TICK_DAMAGE:
          ta_multiplier_buffeffects.emplace_back( buff, val, use_stacks, f );
          debug_message( "tick damage" );
          break;
        case P_CAST_TIME:
          execute_time_buffeffects.emplace_back( buff, val, use_stacks, f );
          debug_message( "cast time" );
          break;
        case P_COOLDOWN:
          recharge_multiplier_buffeffects.emplace_back( buff, val, use_stacks, f );
          debug_message( "cooldown" );
          break;
        case P_RESOURCE_COST:
          cost_buffeffects.emplace_back( buff, val, use_stacks, f );
          debug_message( "cost" );
          break;
        default:
          return;
      }
    }
    else if ( eff.subtype() == A_ADD_FLAT_MODIFIER && eff.misc_value1() == P_CRIT )
    {
      crit_chance_buffeffects.emplace_back( buff, val, use_stacks, f );
      debug_message( "crit chance" );
    }
    else
    {
      return;
    }
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, unsigned ignore_mask, bool use_stacks, bool use_default, Ts... mods )
  {
    if ( !buff )
      return;

    const spell_data_t* s_data = &buff->data();
    for ( size_t i = 1; i <= s_data->effect_count(); i++ )
    {
      if ( ignore_mask & 1 << ( i - 1 ) )
        continue;

      parse_buff_effect( buff, nullptr, s_data, i, use_stacks, use_default, mods... );
    }
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, unsigned ignore_mask, Ts... mods )
  { parse_buff_effects<Ts...>( buff, ignore_mask, true, false, mods... ); }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, bool stack, bool use_default, Ts... mods )
  { parse_buff_effects<Ts...>( buff, 0U, stack, use_default, mods... ); }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, bool stack, Ts... mods )
  { parse_buff_effects<Ts...>( buff, 0U, stack, false, mods... ); }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, Ts... mods )
  { parse_buff_effects<Ts...>( buff, 0U, true, false, mods... ); }

  void parse_conditional_effects( const spell_data_t* spell, bfun f, unsigned ignore_mask = 0U )
  {
    if ( !spell || !spell->ok() )
      return;

    for ( size_t i = 1 ; i <= spell->effect_count(); i++ )
    {
      if ( ignore_mask & 1 << ( i - 1 ) )
        continue;

      parse_buff_effect( nullptr, f, spell, i, false, false );
    }
  }

  void parse_passive_effects( const spell_data_t* spell, unsigned ignore_mask = 0U )
  { parse_conditional_effects( spell, nullptr, ignore_mask ); }

  double get_buff_effects_value( const std::vector<buff_effect_t>& buffeffects, bool flat = false,
                                 bool benefit = true ) const
  {
    double return_value = flat ? 0.0 : 1.0;

    for ( const auto& i : buffeffects )
    {
      double eff_val = i.value;
      int mod = 1;

      if ( i.func && !i.func() )
          continue;  // continue to next effect if conditional effect function is false

      if ( i.buff )
      {
        auto stack = benefit ? i.buff->stack() : i.buff->check();

        if ( !stack )
          continue;  // continue to next effect if stacks == 0 (buff is down)

        mod = i.use_stacks ? stack : 1;
      }

      if ( flat )
        return_value += eff_val * mod;
      else
        return_value *= 1.0 + eff_val * mod;
    }

    return return_value;
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
    using S = const spell_data_t*;

    parse_buff_effects( p()->buff.essence_burst );
  }

  template <typename... Ts>
  void parse_debuff_effects( const dfun& func, bool use_stacks, const spell_data_t* s_data, Ts... mods )
  {
    if ( !s_data->ok() )
      return;

    for ( size_t i = 1; i <= s_data->effect_count(); i++ )
    {
      const auto& eff = s_data->effectN( i );
      double val      = eff.percent();

      if ( eff.type() != E_APPLY_AURA )
        continue;

      if ( eff.subtype() != A_MOD_DAMAGE_FROM_CASTER_SPELLS || !ab::data().affected_by_all( eff ) )
        continue;

      if ( i <= 5 )
        parse_spell_effects_mods( val, s_data, i, mods... );

      if ( !val )
        continue;

      p()->sim->print_debug( "debuff-effects: {} ({}) damage modified by {}% on targets with debuff {} ({}#{})", ab::name(),
                             ab::id, val * 100.0, s_data->name_cstr(), s_data->id(), i );
      target_multiplier_debuffeffects.emplace_back( func, val, use_stacks  );
    }
  }

  template <typename... Ts>
  void parse_debuff_effects( dfun func, const spell_data_t* s_data, Ts... mods )
  { parse_debuff_effects( func, true, s_data, mods... ); }

  double get_debuff_effect_values( evoker_td_t* t ) const
  {
    double return_value = 1.0;

    for ( const auto& i : target_multiplier_debuffeffects )
    {
      auto debuff = i.func( t );

      if ( debuff->check() )
        return_value *= 1.0 + i.value * ( i.use_stacks ? debuff->check() : 1.0 );
    }

    return return_value;
  }

  // Syntax: parse_dot_debuffs[<S[,S...]>]( func, spell_data_t* dot[, spell_data_t* spell1[,spell2...] )
  //  func = function returning the dot_t* of the dot
  //  dot = spell data of the dot
  //  S = optional list of template parameter(s) to indicate spell(s)with redirect effects
  //  spell = optional list of spell(s) with redirect effects that modify the effects on the dot
  void apply_debuffs_effects()
  {
    using S = const spell_data_t*;

    parse_debuff_effects( []( evoker_td_t* t ) -> buff_t* { return t->debuffs.shattering_star; },
        p()->talent.shattering_star );
  }

  double cost() const override
  {
    double c = ab::cost() * std::max( 0.0, get_buff_effects_value( cost_buffeffects, false, false ) );
    return c;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t ) * get_debuff_effect_values( td( t ) );
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
    return et;
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
  evoker_heal_t( std::string_view name, evoker_t* player, const spell_data_t* spell = spell_data_t::nil(),
                 std::string_view options_str = {} )
    : ab( name, player, spell )
  {
    parse_options( options_str );
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

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t );

    // Preliminary testing shows this is linear with target hp %.
    // TODO: confirm this applies only to all evoker offensive spells
    if ( p()->specialization() == EVOKER_DEVASTATION )
      tm *= 1.0 + ( p()->cache.mastery_value() * t->health_percentage() / 100 );

    return tm;
  }

  double composite_persistent_multiplier( const action_state_t* s ) const
  {
    auto mult = ab::composite_persistent_multiplier( s );
    
    if ( p()->talent.might_of_the_aspects.ok() && p()->buff.essence_burst->check())
    {
      mult *= 1.0 + p()->talent.might_of_the_aspects->effectN( 1 ).percent();
    }

    return mult;
  }

  void consume_resource()
  {
    ab::consume_resource();

    resource_e cr = current_resource();

    if ( cr != RESOURCE_ESSENCE || base_cost() == 0 || proc )
      return;

    if ( p()->buff.essence_burst->up() )
    {
      p()->buff.essence_burst->decrement();
    }
  }
};

struct empowered_spell_t : public evoker_spell_t
{
  int empower_to;
  empower_e max_empower;

  empowered_spell_t( std::string_view name, evoker_t* p, const spell_data_t* spell, std::string_view options_str = {} )
    : evoker_spell_t( name, p, p->find_spell_override( spell, p->talent.font_of_magic ) ),
      max_empower( p->talent.font_of_magic.ok() ? empower_e::EMPOWER_4 : empower_e::EMPOWER_3 ),
      empower_to( empower_e::EMPOWER_MAX )
  {
    // TODO: convert to full empower expression support
    add_option( opt_int( "empower_to", empower_to ) );
    parse_options( options_str );

    base_execute_time = time_to_empower( (empower_e) empower_to );
    trigger_gcd       = base_execute_time + 1.5_s;
  }

  action_state_t* new_state() override
  { return new empowered_state_t( this, target ); }

  empower_e empower_level( const action_state_t* s ) const
  { return debug_cast<const empowered_state_t*>( s )->empower; }

  int empower_value( const action_state_t* s ) const
  { return static_cast<int>( debug_cast<const empowered_state_t*>( s )->empower ); }

  virtual empower_e empower_level() const
  {
    // TODO: return the current empowerment level based on elapsed cast/channel time
    return std::min( (empower_e) empower_to, max_empower );
  }

  timespan_t time_to_empower( empower_e empower ) const
  {
    // TODO: confirm these values and determine if they're set values or adjust based on a formula
    // Currently all empowered spells are 2.5s base and 3.25s with empower 4
    switch ( std::min( empower, max_empower ) )
    {
      case empower_e::EMPOWER_1: return 1000_ms;
      case empower_e::EMPOWER_2: return 1750_ms;
      case empower_e::EMPOWER_3: return 2500_ms;
      case empower_e::EMPOWER_4: return 3250_ms;
      default: break;
    }

    return 0_ms;
  }

  timespan_t max_hold_time() const
  { return time_to_empower( max_empower ) + 2_s; }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    evoker_spell_t::snapshot_internal( s, flags, rt );
    debug_cast<empowered_state_t*>( s )->empower = empower_level();
  }
};

// Spells ===================================================================

struct disintegrate_t : public evoker_spell_t
{
  disintegrate_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "disintegrate", p, p->find_class_spell( "Disintegrate" ), options_str )
  {
    channeled = true;
  }
};

struct azure_strike_t : public evoker_spell_t
{
  azure_strike_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "azure_strike", p, p->find_class_spell( "Azure Strike" ), options_str )
  {
    aoe = data().effectN( 1 ).base_value() + p->talent.protracted_talons->effectN( 1 ).base_value();
  }

  void execute() override
  {
    evoker_spell_t::execute();

    if ( p()->talent.azure_essence_burst.ok() && rng().roll( p()->talent.azure_essence_burst->effectN( 1 ).percent() ) )
    {
      p()->buff.essence_burst->trigger();
      p()->proc.azure_essence_burst->occur();
    }
  }

};

struct living_flame_t : public evoker_spell_t
{
  struct living_flame_damage_t : public evoker_spell_t
  {
    living_flame_damage_t( evoker_t* p ) : evoker_spell_t( "living_flame_damage", p, p->find_spell( 361500 ) )
    {
      dual = true;

      dot_duration = p->talent.ruby_embers.ok() ? dot_duration : 0_ms;
    }
  };

  evoker_spell_t* damage;

  living_flame_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "living_flame", p, p->find_class_spell( "Living Flame" ), options_str )
  {

    damage        = p->get_secondary_action<living_flame_damage_t>( "living_flame_damage" );
    damage->stats = stats;
  }

  void execute() override
  {
    evoker_spell_t::execute();

    if ( p()->talent.ruby_essence_burst.ok() && rng().roll( p()->talent.ruby_essence_burst->effectN(1).percent() ) )
    {
      p()->buff.essence_burst->trigger();
      p()->proc.ruby_essence_burst->occur();
    }
  }

  void impact( action_state_t* s ) override
  {
    evoker_spell_t::impact( s );

    damage->schedule_execute();
  }
};

struct shattering_star_t : public evoker_spell_t
{
  shattering_star_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "shattering_star", p, p->talent.shattering_star, options_str )
  {
    aoe = ( 1 + p->talent.eternitys_span->effectN( 2 ).percent() );
  }

  void impact( action_state_t* s ) override
  {
    evoker_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
      td( s->target )->debuffs.shattering_star->trigger();
  }
};

// Empowered Spells =========================================================

struct fire_breath_t : public empowered_spell_t
{
  struct fire_breath_damage_t : public empowered_spell_t
  {
    fire_breath_damage_t( evoker_t* p ) : empowered_spell_t( "fire_breath_damage", p, p->find_spell( 357209 ) )
    {
      dual = true;
    }

    timespan_t composite_dot_duration( const action_state_t* s ) const override
    {
      return empowered_spell_t::composite_dot_duration( s ) * empower_value( s ) / 3.0;
    }
  };

  empowered_spell_t* damage;

  fire_breath_t( evoker_t* p, std::string_view options_str )
    : empowered_spell_t( "fire_breath", p, p->find_class_spell( "Fire Breath" ), options_str )
  {
    damage = p->get_secondary_action<fire_breath_damage_t>( "fire_breath_damage" );
    damage->stats = stats;
  }

  void impact( action_state_t* s ) override
  {
    empowered_spell_t::impact( s );

    auto emp_state = damage->get_state();
    emp_state->target = s->target;
    damage->snapshot_state( emp_state, damage->amount_type( emp_state ) );
    debug_cast<empowered_state_t*>( emp_state )->empower = empower_level();

    damage->schedule_execute( emp_state );
  }
};


struct eternity_surge_t : public empowered_spell_t
{
  struct eternity_surge_damage_t : public empowered_spell_t
  {
    eternity_surge_damage_t( evoker_t* p ) : empowered_spell_t( "eternity_surge_damage", p, p->find_spell( 359077 ) )
    {
      dual = true;
      background = true;
      aoe        = 1;
    }

    int n_targets() const override
    {
      if ( pre_execute_state )
        return empower_level( pre_execute_state ) * ( 1 + p()->talent.eternitys_span->effectN( 2 ).percent() );
      else
        return empower_e::EMPOWER_MAX * ( 1 + p()->talent.eternitys_span->effectN( 2 ).percent() );
    }
  };

  empowered_spell_t* damage;

  eternity_surge_t( evoker_t* p, std::string_view options_str )
    : empowered_spell_t( "eternity_surge", p, p->talent.eternity_surge, options_str )
  {
    damage        = p->get_secondary_action<eternity_surge_damage_t>( "eternity_surge_damage" );
    damage->stats = stats;
  }

  void impact( action_state_t* s ) override
  {
    empowered_spell_t::impact( s );

    auto emp_state    = damage->get_state();
    emp_state->target = s->target;
    damage->snapshot_state( emp_state, damage->amount_type( emp_state ) );
    debug_cast<empowered_state_t*>( emp_state )->empower = empower_level();

    damage->schedule_execute( emp_state );
  }
};
}  // end namespace spells

// ==========================================================================
// Evoker Character Definitions
// ==========================================================================

evoker_td_t::evoker_td_t( player_t* target, evoker_t* evoker )
  : actor_target_data_t( target, evoker ),
    dots(),
    debuffs()
{
  debuffs.shattering_star = make_buff( *this, "shattering_star_debuff", evoker->talent.shattering_star )
    ->set_cooldown( 0_ms );
}

evoker_t::evoker_t( sim_t* sim, std::string_view name, race_e r )
  : player_t( sim, EVOKER, name, r ),
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
  resource_regeneration = regen_type::DYNAMIC;
  regen_caches[ CACHE_HASTE ] = true;
  regen_caches[ CACHE_SPELL_HASTE ] = true;
}

void evoker_t::init_base_stats()
{
  player_t::init_base_stats();

  base.spell_power_per_intellect = 1.0;

  resources.base[ RESOURCE_ESSENCE ] = 5;
  // TODO: confirm base essence regen. currently estimated at 1 per 5s base
  resources.base_regen_per_second[ RESOURCE_ESSENCE ] = 0.2 * ( 1.0 + talent.innate_magic->effectN( 1 ).percent() );
}

void evoker_t::init_spells()
{
  player_t::init_spells();

  auto CT = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::CLASS, n ); };
  auto ST = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::SPECIALIZATION, n ); };
  // Evoker Talents
  // Class Traits
  talent.natural_convergence = CT( "Natural Convergence" );
  talent.innate_magic        = CT( "Innate Magic" );
  talent.enkindled           = CT( "Enkindled" );
  talent.suffused_with_power = CT( "Suffused With Power" );
  talent.protracted_talons   = CT( "Protracted Talons" );
  // Devastation Traits
  talent.ruby_essence_burst   = ST( "Ruby Essence Burst" );
  talent.azure_essence_burst  = ST( "Azure Essence Burst" );
  talent.eternity_surge       = ST( "Eternity Surge" );
  talent.ruby_embers          = ST( "Ruby Embers" );
  talent.engulfing_blaze      = ST( "Engulfing Blaze" );
  talent.essence_attunement   = ST( "Essence Attunement" );
  talent.shattering_star      = ST( "Shattering Star" );
  talent.might_of_the_aspects = ST( "Might of the Aspects" );
  talent.eternitys_span       = ST( "Eternity's Span" );
  talent.font_of_magic        = ST( "Font of Magic" );
  // Preservation Traits

  // Evoker Specialization Spells
  // Baseline
  spec.evoker       = find_spell( 353167 );  // TODO: confirm this is the class aura
  spec.devastation  = find_specialization_spell( "Devastation Evoker" );
  spec.preservation = find_specialization_spell( "Preservation Evoker" );
  // Devastation
  // Preservation
}

void evoker_t::create_actions()
{
  player_t::create_actions();
}

void evoker_t::create_buffs()
{
  player_t::create_buffs();

  using namespace buffs;

  // Baseline Abilities
  if ( specialization() == EVOKER_DEVASTATION)
  {
    buff.essence_burst = make_buff( this, "essence_burst", find_spell( 359618 ) )
      ->apply_affecting_aura( talent.essence_attunement );
  }
  else
  {
    buff.essence_burst = make_buff( this, "essence_burst", find_spell( 369299 ) )
      ->apply_affecting_aura( find_specialization_spell( "Essence Attunement") );
  }
  // Class Traits
  // Devastation Traits
  // Preservation Traits
}

void evoker_t::create_options()
{
  player_t::create_options();
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
  action.apply_affecting_aura( talent.natural_convergence );
  action.apply_affecting_aura( talent.enkindled );
  // Devastaion Traits
  // TODO: Coonfirm if this works properly with Scarlet Adaptation
  action.apply_affecting_aura( talent.engulfing_blaze );
  // Preservation Traits
}

double evoker_t::composite_player_multiplier( school_e s ) const
{
  double m = player_t::composite_player_multiplier( s );

  if ( talent.suffused_with_power.ok() && talent.suffused_with_power->effectN( 1 ).has_common_school( s ) )
    m *= 1 + talent.suffused_with_power->effectN( 1 ).percent();

  return m;
}

action_t* evoker_t::create_action( std::string_view name, std::string_view options_str )
{
  using namespace spells;

  if ( name == "disintegrate" ) return new disintegrate_t( this, options_str );
  if ( name == "fire_breath" ) return new fire_breath_t( this, options_str );
  if ( name == "azure_strike" ) return new azure_strike_t( this, options_str );
  if ( name == "living_flame" ) return new living_flame_t( this, options_str );
  if ( name == "shattering_star" ) return new shattering_star_t( this, options_str );
  if ( name == "eternity_surge" ) return new eternity_surge_t( this, options_str );

  return player_t::create_action( name, options_str );
}

std::unique_ptr<expr_t> evoker_t::create_expression( std::string_view expr_str )
{
  auto splits = util::string_split<std::string_view>( expr_str, "." );

  if ( util::str_compare_ci( expr_str, "essense" ) || util::str_compare_ci( expr_str, "essences" ) )
    return make_ref_expr( expr_str, resources.current[ RESOURCE_ESSENCE ] );

  return player_t::create_expression( expr_str );
}

// Stat & Multiplier overrides ==============================================

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

void evoker_t::init_procs()
{
  proc.ruby_essence_burst = get_proc( "Ruby Essence Burst" );
  proc.azure_essence_burst = get_proc( "Azure Essence Burst" );

  player_t::init_procs();
}

double evoker_t::resource_regen_per_second( resource_e resource ) const
{
  auto rrps = player_t::resource_regen_per_second( resource );

  return rrps;
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
  if ( !passive->ok() )
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
  evoker_report_t( evoker_t& player ) : p( player ) {}

  void html_customsection( report::sc_html_stream& os ) override {}

private:
  evoker_t& p;
};

// EVOKER MODULE INTERFACE ==================================================
struct evoker_module_t : public module_t
{
  evoker_module_t() : module_t( EVOKER ) {}

  player_t* create_player( sim_t* sim, std::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new evoker_t( sim, name, r );
    p->report_extension = std::make_unique<evoker_report_t>( *p );
    return p;
  }
  bool valid() const override { return true; }
  void init( player_t* p ) const override {}
  void static_init() const override {}
  void register_hotfixes() const override {}
  void combat_begin( sim_t* ) const override {}
  void combat_end( sim_t* ) const override {}
};
}

const module_t* module_t::evoker()
{
  static evoker_module_t m;
  return &m;
}
