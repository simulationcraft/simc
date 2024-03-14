// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "action/action.hpp"
#include "buff/buff.hpp"
#include "player/player.hpp"
#include "player/pet.hpp"
#include "sim/sim.hpp"
#include "util/io.hpp"

#include <functional>

// Mixin to action base class to allow auto parsing and dynamic application of whitelist based buffs & auras.
//
// Make `parse_action_effects_t` the parent of module action classe with the following parameters:
//  1) BASE: The base class of the action class. This is usually spell_t, melee_attack_t, or a template parameter
//  2) PLAYER: The player class
//  3) TD: The target data class
//  4) OWNER: Optional owner class if the player class is a pet

enum parse_flag_e
{
  USE_DATA,
  USE_DEFAULT,
  USE_CURRENT,
  IGNORE_STACKS
};

struct action_effect_t
{
  buff_t* buff;                   // nullptr
  double value;                   // 0.0
  parse_flag_e type;              // USE_DATA
  bool use_stacks;                // true
  bool mastery;                   // false
  std::function<bool()> func;     // nullptr
  const spelleffect_data_t* eff;  // &spelleffect_data_t::nil()

  action_effect_t( buff_t* b = nullptr, double v = 0.0, parse_flag_e t = USE_DATA, bool s = true, bool m = false,
                   std::function<bool()> f = nullptr, const spelleffect_data_t* e = &spelleffect_data_t::nil() )
    : buff( b ), value( v ), type( t ), use_stacks( s ), mastery( m ), func( std::move( f ) ), eff( e )
  {}

  action_effect_t& set_buff( buff_t* b )
  { buff = b; return *this; }

  action_effect_t& set_value( double v )
  { value = v; return *this; }

  action_effect_t& set_type( parse_flag_e t )
  { type = t; return *this; }

  action_effect_t& set_use_stacks( bool s )
  { use_stacks = s; return *this; }

  action_effect_t& set_mastery( bool m )
  { mastery = m; return *this; }

  action_effect_t& set_func( std::function<bool()> f )
  { func = std::move( f ); return *this; }

  action_effect_t& set_eff( const spelleffect_data_t* e )
  { eff = e; return *this; }
};

// TODO: add value type to debuffs if it becomes necessary in the future
template <typename TD>
struct target_effect_t
{
  std::function<int( TD* )> func;  // nullptr
  double value;                    // 0.0
  bool mastery;                    // false
  const spelleffect_data_t* eff;   // &spelleffect_data_t::nil()

  target_effect_t( std::function<int( TD* )> f = nullptr, double v = 0.0, bool m = false,
                   const spelleffect_data_t* e = &spelleffect_data_t::nil() )
    : func( std::move( f ) ), value( v ), mastery( m ), eff( e )
  {}

  target_effect_t& set_func( std::function<int( TD* )> f )
  { func = std::move( f ); return *this; }

  target_effect_t& set_value( double v )
  { value = v; return *this; }

  target_effect_t& set_mastery( bool m )
  { mastery = m; return *this; }

  target_effect_t& set_eff( const spelleffect_data_t* e )
  { eff = e; return *this; }
};

struct modify_effect_t
{
  buff_t* buff;                   // nullptr
  std::function<bool()> func;     // nullptr
  double value;                   // 0.0
  bool use_stacks;                // true
  bool flat;                      // false

  modify_effect_t( buff_t* b = nullptr, std::function<bool()> f = nullptr, double v = 0.0, bool s = true,
                   bool fl = false )
    : buff( b ), func( std::move( f ) ), value( v ), use_stacks( s ), flat( fl )
  {}

  modify_effect_t& set_buff( buff_t* b )
  { buff = b; return *this; }

  modify_effect_t& set_func( std::function<bool()> f )
  { func = std::move( f ); return *this; }

  modify_effect_t& set_value( double v )
  { value = v; return *this; }

  modify_effect_t& set_use_stacks( bool s )
  { use_stacks = s; return *this; }

  modify_effect_t& set_flat( bool fl )
  { flat = fl; return *this; }
};

// used to store values from parameter pack recursion of parse_effect/parse_target_effects
template <typename U, typename = std::enable_if_t<std::is_default_constructible_v<U>>>
struct pack_t
{
  U data;
  std::vector<const spell_data_t*> list;
  unsigned mask = 0U;
};

struct parse_effects_t
{
  parse_effects_t() = default;
  virtual ~parse_effects_t() = default;

  template <typename U>
  U& add_parse_entry( std::vector<U>& vec )
  { return vec.emplace_back(); }

  template <typename T> using detect_buff = decltype( T::buff );

  template <typename T, typename U>
  const spell_data_t* resolve_parse_data( T data, pack_t<U>& tmp )
  {
    if constexpr ( std::is_invocable_v<decltype( &buff_t::data ), T> )
    {
      if ( !data )
        return nullptr;

      if constexpr ( is_detected<detect_buff, U>::value )
        tmp.data.buff = data;

      return &data->data();
    }
    else if constexpr ( std::is_invocable_v<decltype( &spell_data_t::ok ), T> )
    {
      return data;
    }
    else
    {
      static_assert( static_false<T>, "Invalid data type for resolve_parse_data" );
      return nullptr;
    }
  }

  double mod_spell_effects_value( const spell_data_t*, const spelleffect_data_t& e ) { return e.base_value(); }

  template <typename T>
  void apply_affecting_mod( double& val, bool& mastery, const spell_data_t* base, size_t idx, T mod )
  {
    bool mod_is_mastery = false;

    if ( mod->effect_count() && mod->flags( SX_MASTERY_AFFECTS_POINTS ) )
    {
      mastery = true;
      mod_is_mastery = true;
    }

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
        double pct = mod_is_mastery ? eff.mastery_value() : mod_spell_effects_value( mod, eff );

        if ( eff.subtype() == A_ADD_FLAT_MODIFIER || eff.subtype() == A_ADD_FLAT_LABEL_MODIFIER )
          val += pct;
        else if ( eff.subtype() == A_ADD_PCT_MODIFIER || eff.subtype() == A_ADD_PCT_LABEL_MODIFIER )
          val *= 1.0 + pct / 100;
        else if ( eff.subtype() == A_PROC_TRIGGER_SPELL_WITH_VALUE )
          val = pct;
      }
    }
  }

  template <typename U>
  void apply_affecting_mods( const pack_t<U>& tmp, double& val, bool& mastery, const spell_data_t* base, size_t idx )
  {
    // Apply effect modifying effects from mod list. Blizz only currently supports modifying effects 1-5
    if ( idx > 5 )
      return;

    for ( size_t j = 0; j < tmp.list.size(); j++ )
      apply_affecting_mod( val, mastery, base, idx, tmp.list[ j ] );
  }

  template <typename T> using detect_func = decltype( T::func );
  template <typename T> using detect_use_stacks = decltype( T::use_stacks );
  template <typename T> using detect_type = decltype( T::type );
  template <typename T> using detect_value = decltype( T::value );

  template <typename U, typename T>
  void parse_spell_effect_mod( pack_t<U>& tmp, T mod )
  {
    if constexpr ( std::is_invocable_v<decltype( &spell_data_t::ok ), T> )
    {
      tmp.list.push_back( mod );
    }
    else if constexpr ( std::is_invocable_v<T> && is_detected<detect_func, U>::value )
    {
      tmp.data.func = std::move( mod );
    }
    else if constexpr ( std::is_same_v<T, parse_flag_e> )
    {
      if constexpr ( is_detected<detect_use_stacks, U>::value )
      {
        if ( mod == IGNORE_STACKS )
          tmp.data.use_stacks = false;
      }

      if constexpr ( is_detected<detect_type, U>::value )
      {
        if ( mod == USE_DEFAULT || mod == USE_CURRENT )
          tmp.data.type = mod;
      }
    }
    else if constexpr ( std::is_floating_point_v<T> && is_detected<detect_value, U>::value )
    {
      tmp.data.value = mod;
    }
    else if constexpr ( std::is_integral_v<T> && !std::is_same_v<T, bool> )
    {
      tmp.mask = mod;
    }
    else
    {
      static_assert( static_false<T>, "Invalid mod type for parse_spell_effect_mods" );
    }
  }

  template <typename U, typename... Ts>
  void parse_spell_effect_mods( pack_t<U>& tmp, Ts... mods )
  {
    ( parse_spell_effect_mod( tmp, mods ), ... );
  }
};

template <typename BASE, typename PLAYER, typename TD, typename OWNER = PLAYER>
struct parse_action_effects_t : public BASE, public parse_effects_t
{
private:
  PLAYER* player_;
  std::array<std::pair<double, std::vector<modify_effect_t>>, 5> effect_modifiers;

public:
  // auto parsed dynamic effects
  std::vector<action_effect_t> ta_multiplier_effects;
  std::vector<action_effect_t> da_multiplier_effects;
  std::vector<action_effect_t> execute_time_effects;
  std::vector<action_effect_t> dot_duration_effects;
  std::vector<action_effect_t> tick_time_effects;
  std::vector<action_effect_t> recharge_multiplier_effects;
  std::vector<action_effect_t> cost_effects;
  std::vector<action_effect_t> flat_cost_effects;
  std::vector<action_effect_t> crit_chance_effects;
  std::vector<target_effect_t<TD>> target_multiplier_effects;
  std::vector<target_effect_t<TD>> target_crit_damage_effects;
  std::vector<target_effect_t<TD>> target_crit_chance_effects;

  parse_action_effects_t( std::string_view name, PLAYER* player, const spell_data_t* spell )
    : BASE( name, player, spell ), parse_effects_t(), player_( player )
  {
    for ( size_t i = 0; i < 5 && i < spell->effect_count(); i++ )
      effect_modifiers[ i ].first = spell->effectN( i + 1 ).base_value();
  }

  double cost() const override
  {
    return std::max( 0.0, ( BASE::cost() * get_effects_value( cost_effects, false, false ) ) );
  }

  double cost_flat_modifier() const override
  {
    return BASE::cost_flat_modifier() + get_effects_value( flat_cost_effects, true, false );
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    return BASE::composite_ta_multiplier( s ) * get_effects_value( ta_multiplier_effects );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    return BASE::composite_da_multiplier( s ) * get_effects_value( da_multiplier_effects );
  }

  double composite_crit_chance() const override
  {
    return BASE::composite_crit_chance() + get_effects_value( crit_chance_effects, true );
  }

  timespan_t execute_time() const override
  {
    return std::max( 0_ms, BASE::execute_time() * get_effects_value( execute_time_effects ) );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return BASE::composite_dot_duration( s ) * get_effects_value( dot_duration_effects );
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    return std::max( 1_ms, BASE::tick_time( s ) * get_effects_value( tick_time_effects ) );
  }

  timespan_t cooldown_duration() const override
  {
    return BASE::cooldown_duration() * get_effects_value( recharge_multiplier_effects );
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    return BASE::recharge_multiplier( cd ) * get_effects_value( recharge_multiplier_effects );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    return BASE::composite_target_multiplier( t ) *
           get_target_effects_value( target_multiplier_effects, _get_td( t ) );
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    return BASE::composite_target_crit_chance( t ) +
           get_target_effects_value( target_crit_chance_effects, _get_td( t ), true );
  }

  double composite_target_crit_damage_bonus_multiplier( player_t* t ) const override
  {
    return BASE::composite_target_crit_damage_bonus_multiplier( t ) *
           get_target_effects_value( target_crit_damage_effects, _get_td( t ) );
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    parsed_html_report( os );
  }

  // Syntax: parse_effects( data[, spells|condition|ignore_mask|value|flags][,...] )
  //   (buff_t*) or
  //   (const spell_data_t*)   data: Buff or spell to be checked for to see if effect applies. If buff is used, effect
  //                                 will require the buff to be active. If spell is used, effect will always apply
  //                                 unless an optional condition function is provided.
  //
  // The following optional arguments can be used in any order:
  //   (const spell_data_t*) spells: List of spells with redirect effects that modify the effects on the buff
  //   (bool F())         condition: Function that takes no arguments and returns true if the effect should apply
  //   (unsigned)       ignore_mask: Bitmask to skip effect# n corresponding to the n'th bit
  //   (double)               value: Directly set the value, this overrides all other parsed values
  //   (parse_flag_e)         flags: Various flags to control how the value is calculated when the action executes
  //                    USE_DEFAULT: Use the buff's default value instead of spell effect data value
  //                    USE_CURRENT: Use the buff's current value instead of spell effect data value
  //                  IGNORE_STACKS: Ignore stacks of the buff and don't multiply the value
  //
  // Example 1: Parse buff1, ignore effects #1 #3 #5, modify by talent1, modify by tier1:
  //   parse_effects( buff1, 0b10101U, talent1, tier1 );
  //
  // Example 2: Parse buff2, don't multiply by stacks, use the default value set on the buff instead of effect value:
  //   parse_effects( buff2, false, USE_DEFAULT );
  //
  // Example 3: Parse spell1, modify by talent1, only apply if my_player_t::check1() returns true:
  //   parse_effects( spell1, talent1, &my_player_t::check1 );
  //
  // Example 4: Parse buff3, only apply if my_player_t::check2() and my_player_t::check3() returns true:
  //   parse_effects( buff3, [ this ] { return p()->check2() && p()->check3(); } );
  template <typename T, typename... Ts>
  void parse_effects( T data, Ts... mods )
  {
    pack_t<action_effect_t> pack;
    const spell_data_t* spell = resolve_parse_data( data, pack );

    if ( !spell || !spell->ok() )
      return;

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    for ( size_t i = 1; i <= spell->effect_count(); i++ )
    {
      if ( pack.mask & 1 << ( i - 1 ) )
        continue;

      // local copy of pack per effect
      pack_t<action_effect_t> tmp = pack;

      parse_action_effect( tmp, spell, i, false );
    }
  }

  template <typename T, typename... Ts>
  void force_effect( T data, unsigned idx, Ts... mods )
  {
    pack_t<action_effect_t> pack;
    const spell_data_t* spell = resolve_parse_data( data, pack );

    if ( !spell || !spell->ok() )
      return;

    if ( BASE::data().affected_by_all( spell->effectN( idx ) ) )
    {
      assert( false && "Effect already affects action, no need to force" );
      return;
    }

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    parse_action_effect( pack, spell, idx, true );
  }

  void parse_action_effect( pack_t<action_effect_t>& tmp, const spell_data_t* s_data, size_t i, bool force )
  {
    const auto& eff = s_data->effectN( i );
    bool mastery = s_data->flags( SX_MASTERY_AFFECTS_POINTS );
    double val = 0.0;
    double val_mul = 0.01;

    if ( tmp.data.buff && tmp.data.type == USE_DEFAULT )
      val = tmp.data.buff->default_value * 100;
    else if ( mastery )
      val = eff.mastery_value();
    else
      val = eff.base_value();

    // TODO: more robust logic around 'party' buffs with radius
    if ( eff.radius() )
      return;

    // Only parse apply aura effects
    switch ( eff.type() )
    {
      case E_APPLY_AURA:
      case E_APPLY_AREA_AURA_PARTY:
      case E_APPLY_AURA_PET:
        break;
      default:
        return;
    }

    if ( tmp.data.value != 0.0 )
    {
      val = tmp.data.value;
      mastery = false;
    }
    else
    {
      apply_affecting_mods( tmp, val, mastery, s_data, i );

      if ( mastery )
        val_mul = 1.0;
    }

    if ( !val )
      return;

    std::vector<action_effect_t>* vec = nullptr;
    std::string str;
    bool flat = false;

    auto debug_message = [ & ]( std::string_view type ) {
      std::string val_str = mastery ? fmt::format( "{}*mastery", val * 100 )
                            : flat  ? fmt::format( "{}", val )
                                    : fmt::format( "{}%", val * 100 );

      if ( tmp.data.buff )
      {
        if ( tmp.data.value != 0.0 )
          val_str = val_str + " (overridden)";
        else if ( tmp.data.type == parse_flag_e::USE_CURRENT )
          val_str = flat ? "current value" : "current value percent";
        else if ( tmp.data.type == parse_flag_e::USE_DEFAULT )
          val_str = val_str + " (default value)";

        BASE::sim->print_debug( "action-effects: {} ({}) {} modified by {} {} buff {} ({}#{})", BASE::name(), BASE::id,
                                type, val_str, tmp.data.use_stacks ? "per stack of" : "with", tmp.data.buff->name(),
                                tmp.data.buff->data().id(), i );
      }
      else if ( mastery && !tmp.data.func )
      {
        BASE::sim->print_debug( "action-effects: {} ({}) {} modified by {} from {} ({}#{})", BASE::name(), BASE::id,
                                type, val_str, s_data->name_cstr(), s_data->id(), i );
      }
      else if ( tmp.data.func )
      {
        BASE::sim->print_debug( "action-effects: {} ({}) {} modified by {} with condition from {} ({}#{})",
                                BASE::name(), BASE::id, type, val_str, s_data->name_cstr(), s_data->id(), i );
      }
      else
      {
        BASE::sim->print_debug( "action-effects: {} ({}) {} modified by {} from {} ({}#{})", BASE::name(), BASE::id,
                                type, val_str, s_data->name_cstr(), s_data->id(), i );
      }
    };

    if ( !BASE::special && eff.subtype() == A_MOD_AUTO_ATTACK_PCT )
    {
      vec = &da_multiplier_effects;
      str = "auto attack";
    }
    else if ( !BASE::data().affected_by_all( eff ) && !force )
    {
      return;
    }
    else if ( eff.subtype() == A_ADD_PCT_MODIFIER || eff.subtype() == A_ADD_PCT_LABEL_MODIFIER )
    {
      switch ( eff.misc_value1() )
      {
        case P_GENERIC:
          vec = &da_multiplier_effects;
          str = "direct damage";
          break;
        case P_DURATION:
          vec = &dot_duration_effects;
          str = "duration";
          break;
        case P_TICK_DAMAGE:
          vec = &ta_multiplier_effects;
          str = "tick damage";
          break;
        case P_CAST_TIME:
          vec = &execute_time_effects;
          str = "cast time";
          break;
        case P_TICK_TIME:
          vec = &tick_time_effects;
          str = "tick time";
          break;
        case P_COOLDOWN:
          vec = &recharge_multiplier_effects;
          str = "cooldown";
          break;
        case P_RESOURCE_COST:
          vec = &cost_effects;
          str = "cost percent";
          break;
        default:
          return;
      }
    }
    else if ( eff.subtype() == A_ADD_FLAT_MODIFIER || eff.subtype() == A_ADD_FLAT_LABEL_MODIFIER )
    {
      flat = true;

      switch ( eff.misc_value1() )
      {
        case P_CRIT:
          vec = &crit_chance_effects;
          str = "crit chance";
          break;
        case P_RESOURCE_COST:
          val_mul = spelleffect_data_t::resource_multiplier( BASE::current_resource() );
          vec = &flat_cost_effects;
          str = "flat cost";
          break;
        default:
          return;
      }
    }

    if ( !vec )
      return;

    val *= val_mul;
    debug_message( str );
    tmp.data.value = val;
    tmp.data.mastery = mastery;
    tmp.data.eff = &eff;
    vec->push_back( tmp.data );
  }

  double get_effects_value( const std::vector<action_effect_t>& effects, bool flat = false, bool benefit = true ) const
  {
    double return_value = flat ? 0.0 : 1.0;

    for ( const auto& i : effects )
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

        if ( i.use_stacks )
          mod = stack;

        if ( i.type == USE_CURRENT )
          eff_val = i.buff->check_value();
      }

      if ( i.mastery )
        eff_val *= BASE::player->cache.mastery();

      if ( flat )
        return_value += eff_val * mod;
      else
        return_value *= 1.0 + eff_val * mod;
    }

    return return_value;
  }

  // Syntax: parse_target_effects( func, debuff[, spells|ignore_mask][,...] )
  //   (int F(TD*))            func: Function taking the target_data as argument and returning an integer mutiplier
  //   (const spell_data_t*) debuff: Spell data of the debuff
  //
  // The following optional arguments can be used in any order:
  //   (const spell_data_t*) spells: List of spells with redirect effects that modify the effects on the debuff
  //   (unsigned)       ignore_mask: Bitmask to skip effect# n corresponding to the n'th bit
  template <typename... Ts>
  void parse_target_effects( const std::function<int( TD* )>& fn, const spell_data_t* spell, Ts... mods )
  {
    pack_t<target_effect_t<TD>> pack;

    if ( !spell || !spell->ok() )
      return;

    pack.data.func = std::move( fn );

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    for ( size_t i = 1; i <= spell->effect_count(); i++ )
    {
      if ( pack.mask & 1 << ( i - 1 ) )
        continue;

      // local copy of pack per effect
      pack_t<target_effect_t<TD>> tmp = pack;

      parse_target_effect( tmp, spell, i, false );
    }
  }

  template <typename... Ts>
  void force_target_effect( const std::function<int( TD* )>& fn, const spell_data_t* spell, unsigned idx, Ts... mods )
  {
    pack_t<target_effect_t<TD>> pack;

    if ( !spell || !spell->ok() )
      return;

    if ( BASE::data().affected_by_all( spell->effectN( idx ) ) )
    {
      assert( false && "Effect already affects action, no need to force" );
      return;
    }

    pack.data.func = std::move( fn );

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    parse_target_effect( pack, spell, idx, true );
  }

  template <typename... Ts>
  void parse_target_effect( pack_t<target_effect_t<TD>>& tmp, const spell_data_t* s_data, size_t i, bool force )
  {
    const auto& eff = s_data->effectN( i );
    bool mastery = s_data->flags( SX_MASTERY_AFFECTS_POINTS );
    double val = 0.0;
    double val_mul = 0.01;

    if ( mastery )
      val = eff.mastery_value();
    else
      val = eff.base_value();

    if ( eff.type() != E_APPLY_AURA )
      return;

    if ( tmp.data.value != 0.0 )
    {
      val = tmp.data.value;
      mastery = false;
    }
    else
    {
      apply_affecting_mods( tmp, val, mastery, s_data, i );

      if ( mastery )
        val_mul = 1.0;
    }

    if ( !val )
      return;

    std::vector<target_effect_t<TD>>* vec = nullptr;
    std::string str;
    bool flat = false;

    auto debug_message = [ & ]( std::string_view type ) {
      std::string val_str = mastery ? fmt::format( "{}*mastery", val * 100 )
                            : flat  ? fmt::format( "{}", val )
                                    : fmt::format( "{}%", val * 100 );

      if ( tmp.data.value != 0.0 )
        val_str = val_str + " (overridden)";

      BASE::sim->print_debug( "target-effects: {} ({}) {} modified by {} on targets with dot {} ({}#{})", BASE::name(),
                              BASE::id, type, val_str, s_data->name_cstr(), s_data->id(), i );
    };


    if ( !BASE::special && eff.subtype() == A_MOD_AUTO_ATTACK_FROM_CASTER )
    {
      vec = &target_multiplier_effects;
      str = "auto attack";
    }
    else if ( !BASE::data().affected_by_all( eff ) && !force )
    {
      return;
    }
    else
    {
      switch ( eff.subtype() )
      {
        case A_MOD_DAMAGE_FROM_CASTER_SPELLS:
        case A_MOD_DAMAGE_FROM_CASTER_SPELLS_LABEL:
          vec = &target_multiplier_effects;
          str = "damage";
          break;
        case A_MOD_CRIT_CHANCE_FROM_CASTER_SPELLS:
          flat = true;
          vec = &target_crit_chance_effects;
          str = "crit chance";
          break;
        case A_MOD_CRIT_DAMAGE_PCT_FROM_CASTER_SPELLS:
          vec = &target_crit_damage_effects;
          str = "crit damage";
          break;
        default:
          return;
      }
    }

    if ( !vec )
      return;

    val *= val_mul;
    debug_message( str );
    tmp.data.value = val;
    tmp.data.mastery = mastery;
    tmp.data.eff = &eff;
    vec->push_back( tmp.data );
  }

private:
  // method for getting target data as each module may have different action scoped method
  TD* _get_td( player_t* t ) const
  {
    if constexpr ( std::is_invocable_v<decltype( &pet_t::owner ), PLAYER> )
      return static_cast<OWNER*>( player_->owner )->get_target_data( t );
    else
      return player_->get_target_data( t );
  }

public:
  double get_target_effects_value( const std::vector<target_effect_t<TD>>& effects, TD* td, bool flat = false ) const
  {
    double return_value = flat ? 0.0 : 1.0;

    for ( const auto& i : effects )
    {
      if ( auto check = i.func( td ) )
      {
        auto eff_val = i.value;

        if ( i.mastery )
          eff_val *= BASE::player->cache.mastery();

        if ( flat )
          return_value += eff_val * check;
        else
          return_value *= 1.0 + eff_val * check;
      }
    }

    return return_value;
  }

  // Syntax: parse_effect_modifiers( data[, spells|condition|ignore_mask|value|flags][,...])
  //   (buff_t*) or
  //   (const spell_data_t*)   data: Buff or spell to be checked for to see if effect applies. If buff is used,
  //                                 modification will only apply if the buff is active. If spell is used, modification
  //                                 will always apply unless an optional condition function is provided.
  //
  // The following optional arguments can be used in any order:
  //   (const spell_data_t*) spells: List of spells with redirect effects that modify the effects on the buff
  //   (bool F())         condition: Function that takes no arguments and returns true if the effect should apply
  //   (unsigned)       ignore_mask: Bitmask to skip effect# n corresponding to the n'th bit
  //   (double)               value: Directly set the value, this overrides all other parsed values
  //   (parse_flag_e)         flags: Various flags to control how the value is calculated when the action executes
  //                  IGNORE_STACKS: Ignore stacks of the buff and don't multiply the value
  //
  // To access effect values of the action's spell data modified by parsed modifiers:
  //   modified_effectN( idx )            : equivalent to effectN( idx ).base_value()
  //   modified_effectN_percent( idx )    : equivalent to effectN( idx ).percent()
  //   modified_effectN_resource( idx, r ): equivalent to effectN( idx ).resource( r )

  template <typename T, typename... Ts>
  void parse_effect_modifiers( T data, Ts... mods )
  {
    pack_t<modify_effect_t> pack;
    const spell_data_t* spell = resolve_parse_data( data, pack );

    if ( !spell || !spell->ok() )
      return;

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    for ( size_t i = 1; i <= spell->effect_count(); i++ )
    {
      if ( pack.mask & 1 << ( i - 1 ) )
        continue;

      // local copy of pack per effect
      pack_t<modify_effect_t> tmp = pack;

      parse_effect_modifier( tmp, spell, i );
    }
  }

  void parse_effect_modifier( pack_t<modify_effect_t>& tmp, const spell_data_t* s_data, size_t i )
  {
    const auto& eff = s_data->effectN( i );
    bool m;  // dummy throwaway
    double val = eff.base_value();
    double val_mul = 0.01;
    int idx = -1;
    bool flat = false;

    if ( eff.type() != E_APPLY_AURA )
      return;

    switch ( eff.subtype() )
    {
      case A_ADD_FLAT_MODIFIER:
      case A_ADD_FLAT_LABEL_MODIFIER:
        val_mul = 1.0;
        flat = true;
        break;
      case A_ADD_PCT_MODIFIER:
      case A_ADD_PCT_LABEL_MODIFIER:
        break;
      default:
        return;
    }

    switch ( eff.property_type() )
    {
      case P_EFFECT_1: idx = 0; break;
      case P_EFFECT_2: idx = 1; break;
      case P_EFFECT_3: idx = 2; break;
      case P_EFFECT_4: idx = 3; break;
      case P_EFFECT_5: idx = 4; break;
      default: return;
    }

    if ( !BASE::data().affected_by_all( eff ) )
      return;

    if ( tmp.data.value != 0.0 )
    {
      val = tmp.data.value;
    }
    else
    {
      apply_affecting_mods( tmp, val, m, s_data, i );
      val *= val_mul;
    }

    if ( !val )
      return;

    std::string val_str = flat ? fmt::format( "{}", val ) : fmt::format( "{}%", val * 100 );

    // always active
    if ( !tmp.data.buff && !tmp.data.func )
    {
      BASE::sim->print_debug( "modify-effects: {} ({}#{}) permanently modified by {} from {} ({}#{})", BASE::name(),
                              BASE::id, idx + 1, val_str, s_data->name_cstr(), s_data->id(), i );

      if ( flat )
        effect_modifiers[ idx ].first += val;
      else
        effect_modifiers[ idx ].first *= 1.0 + val;
    }
    // conditionally active
    else
    {
      BASE::sim->print_debug( "modify-effects: {} ({}#{}) conditionally modified by {} from {} ({}#{})", BASE::name(),
                              BASE::id, idx + 1, val_str, s_data->name_cstr(), s_data->id(), i );

      tmp.data.value = val;
      tmp.data.flat = flat;

      effect_modifiers[ idx ].second.push_back( tmp.data );
    }
  }

  // return base value after modifiers
  double modified_effectN( size_t idx ) const
  {
    if ( !idx )
      return 0.0;

    auto return_value = effect_modifiers[ idx - 1 ].first;

    for ( const auto& i : effect_modifiers[ idx - 1 ].second )
    {
      double eff_val = i.value;

      if ( i.func && !i.func() )
        continue;  // continue to next effect if conditional effect function is false

      if ( i.buff )
      {
        auto stack = i.buff->check();

        if ( !stack )
          continue;  // continue to next effect if stacks == 0 (buff is down)

        if ( i.use_stacks )
          eff_val *= stack;
      }

      if ( i.flat )
        return_value += eff_val;
      else
        return_value *= 1.0 + eff_val;
    }

    return return_value;
  }

  double modified_effectN_percent( size_t idx ) const
  {
    return modified_effectN( idx ) * 0.01;
  }

  double modified_effectN_resource( size_t idx, resource_e r ) const
  {
    return modified_effectN( idx ) * spelleffect_data_t::resource_multiplier( r );
  }

  void parsed_html_report( report::sc_html_stream& os )
  {
    if ( !total_effects_count() )
      return;

    os << "<div>\n"
       << "<h4>Affected By (Dynamic)</h4>\n"
       << "<table class=\"details nowrap\" style=\"width:min-content\">\n";

    os << "<tr>\n"
       << "<th class=\"small\">Type</th>\n"
       << "<th class=\"small\">Spell</th>\n"
       << "<th class=\"small\">ID</th>\n"
       << "<th class=\"small\">#</th>\n"
       << "<th class=\"small\">Value</th>\n"
       << "<th class=\"small\">Source</th>\n"
       << "<th class=\"small\">Notes</th>\n"
       << "</tr>\n";

    print_parsed_type( os, da_multiplier_effects, "Direct Damage" );
    print_parsed_type( os, ta_multiplier_effects, "Periodic Damage" );
    print_parsed_type( os, crit_chance_effects, "Critical Strike Chance" );
    print_parsed_type( os, execute_time_effects, "Execute Time" );
    print_parsed_type( os, dot_duration_effects, "Dot Duration" );
    print_parsed_type( os, tick_time_effects, "Tick Time" );
    print_parsed_type( os, recharge_multiplier_effects, "Recharge Multiplier" );
    print_parsed_type( os, flat_cost_effects, "Flat Cost" );
    print_parsed_type( os, cost_effects, "Percent Cost" );
    print_parsed_type( os, target_multiplier_effects, "Damage on Debuff" );
    print_parsed_type( os, target_crit_chance_effects, "Crit Chance on Debuff" );
    print_parsed_type( os, target_crit_damage_effects, "Crit Damage on Debuff" );
    print_parsed_custom_type( os );

    os << "</table>\n"
       << "</div>\n";
  }

  virtual size_t total_effects_count()
  {
    return ta_multiplier_effects.size() +
           da_multiplier_effects.size() +
           execute_time_effects.size() +
           dot_duration_effects.size() +
           tick_time_effects.size() +
           recharge_multiplier_effects.size() +
           cost_effects.size() +
           flat_cost_effects.size() +
           crit_chance_effects.size() +
           target_multiplier_effects.size();
  }

  virtual void print_parsed_custom_type( report::sc_html_stream& ) {}

  template <typename V>
  void print_parsed_type( report::sc_html_stream& os, const V& entries, std::string_view n )
  {
    auto c = entries.size();
    if ( !c )
      return;

    os.format( "<tr><td class=\"label\" rowspan=\"{}\">{}</td>\n", c, n );

    for ( size_t i = 0; i < c; i++ )
    {
      if ( i > 0 )
        os << "<tr>";

      print_parsed_line( os, entries[ i ] );
    }
  }

  std::string value_type_name( parse_flag_e t )
  {
    switch ( t )
    {
      case USE_DEFAULT: return "Default Value";
      case USE_CURRENT: return "Current Value";
      default:          return "Spell Data";
    }
  }

  void print_parsed_line( report::sc_html_stream& os, const action_effect_t& entry )
  {
    std::vector<std::string> notes;

    if ( !entry.use_stacks )
      notes.push_back( "No-stacks" );

    if ( entry.mastery )
      notes.push_back( "Mastery" );

    if ( entry.func )
      notes.push_back( "Conditional" );

    os.format( "<td>{}</td><td>{}</td><td>{}</td><td>{:.3f}</td><td>{}</td><td>{}</td></tr>\n",
               entry.eff->spell()->name_cstr(),
               entry.eff->spell()->id(),
               entry.eff->index() + 1,
               entry.value * ( entry.mastery ? 100 : 1 ),
               value_type_name( entry.type ),
               util::string_join( notes ) );
  }

  void print_parsed_line( report::sc_html_stream& os, const target_effect_t<TD>& entry )
  {
    os.format( "<td>{}</td><td>{}</td><td>{}</td><td>{:.3f}</td><td>{}</td><td>{}</td></tr>\n",
               entry.eff->spell()->name_cstr(),
               entry.eff->spell()->id(),
               entry.eff->index() + 1,
               entry.value * ( entry.mastery ? 100 : 1 ),
               "",
               entry.mastery ? "Mastery" : "" );
  }
};
