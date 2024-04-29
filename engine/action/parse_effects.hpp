// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "action/action.hpp"
#include "buff/buff.hpp"
#include "dbc/dbc.hpp"
#include "player/pet.hpp"
#include "player/player.hpp"
#include "sim/sim.hpp"
#include "util/io.hpp"

#include <functional>

// Mixin to action base class to allow auto parsing and dynamic application of whitelist based buffs & auras.
//
// Make `parse_action_effects_t` the parent of module action classe with the following parameters:
//  1) BASE: The base class of the action class. This is usually spell_t, melee_attack_t, or a template parameter
//  2) PLAYER: The player class
//  3) TD: The target data class

enum parse_flag_e
{
  USE_DATA,
  USE_DEFAULT,
  USE_CURRENT,
  IGNORE_STACKS
};

// effects dependent on player state
struct player_effect_t
{
  buff_t* buff = nullptr;
  double value = 0.0;
  parse_flag_e type = USE_DATA;
  bool use_stacks = true;
  bool mastery = false;
  std::function<bool()> func = nullptr;
  const spelleffect_data_t* eff = &spelleffect_data_t::nil();
  uint32_t opt_enum = 0xFFFFFFFF;

  player_effect_t& set_buff( buff_t* b )
  { buff = b; return *this; }

  player_effect_t& set_value( double v )
  { value = v; return *this; }

  player_effect_t& set_type( parse_flag_e t )
  { type = t; return *this; }

  player_effect_t& set_use_stacks( bool s )
  { use_stacks = s; return *this; }

  player_effect_t& set_mastery( bool m )
  { mastery = m; return *this; }

  player_effect_t& set_func( std::function<bool()> f )
  { func = std::move( f ); return *this; }

  player_effect_t& set_eff( const spelleffect_data_t* e )
  { eff = e; return *this; }

  player_effect_t& set_opt_enum( uint32_t e )
  { opt_enum = e; return *this; }
};

// effects dependent on target state
// TODO: add value type to debuffs if it becomes necessary in the future
template <typename TD>
struct target_effect_t
{
  std::function<int( TD* )> func = nullptr;
  double value = 0.0;
  bool mastery = false;
  const spelleffect_data_t* eff = &spelleffect_data_t::nil();
  uint32_t opt_enum = 0xFFFFFFFF;

  target_effect_t& set_func( std::function<int( TD* )> f )
  { func = std::move( f ); return *this; }

  target_effect_t& set_value( double v )
  { value = v; return *this; }

  target_effect_t& set_mastery( bool m )
  { mastery = m; return *this; }

  target_effect_t& set_eff( const spelleffect_data_t* e )
  { eff = e; return *this; }

  target_effect_t& set_opt_enum( uint32_t e )
  { opt_enum = e; return *this; }
};

struct modify_effect_t
{
  buff_t* buff = nullptr;
  std::function<bool()> func = nullptr;
  double value = 0.0;
  bool use_stacks = true;
  bool flat = false;
  const spelleffect_data_t* eff = &spelleffect_data_t::nil();

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

  modify_effect_t& set_eff( const spelleffect_data_t* e )
  { eff = e; return *this; }
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
protected:
  player_t* _player;

public:
  parse_effects_t( player_t* p ) : _player( p ) {}
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
  void parse_effect( pack_t<player_effect_t>& tmp, const spell_data_t* s_data, size_t i, bool force )
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

    if ( !is_valid_aura( eff ) )
      return;

    if ( tmp.data.value != 0.0 )
    {
      val = tmp.data.value;
      val_mul = 1.0;
      mastery = false;
    }
    else
    {
      apply_affecting_mods( tmp, val, mastery, s_data, i );

      if ( mastery )
        val_mul = 1.0;
    }

    if ( !val && tmp.data.type == parse_flag_e::USE_DATA )
      return;

    std::string type_str;
    bool flat = false;

    auto vec = get_effect_vector( eff, tmp.data.opt_enum, val_mul, type_str, flat, force );

    if ( !vec )
      return;

    val *= val_mul;

    std::string val_str = mastery ? fmt::format( "{}*mastery", val * 100 )
                          : flat  ? fmt::format( "{}", val )
                                  : fmt::format( "{}%", val * ( 1 / val_mul ) );

    if ( tmp.data.value != 0.0 )
    {
      val_str = val_str + " (overridden)";
    }
    else if ( tmp.data.buff )
    {
      if ( tmp.data.type == parse_flag_e::USE_CURRENT )
        val_str = flat ? "current value" : "current value percent";
      else if ( tmp.data.type == parse_flag_e::USE_DEFAULT )
        val_str = val_str + " (default value)";
    }

    debug_message( tmp.data, type_str, val_str, mastery, s_data, i );

    tmp.data.value = val;
    tmp.data.mastery = mastery;
    tmp.data.eff = &eff;
    vec->push_back( tmp.data );
  }

  virtual bool is_valid_aura( const spelleffect_data_t& eff ) const { return false; }

  virtual std::vector<player_effect_t>* get_effect_vector( const spelleffect_data_t& eff, uint32_t& opt_enum,
                                                           double& val_mul, std::string& str, bool& flat,
                                                           bool force ) = 0;

  virtual void debug_message( const player_effect_t& data, std::string_view type_str, std::string_view val_str,
                              bool mastery, const spell_data_t* s_data, size_t i ) = 0;

  template <typename T, typename... Ts>
  void parse_effects( T data, Ts... mods )
  {
    pack_t<player_effect_t> pack;
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
      pack_t<player_effect_t> tmp = pack;

      parse_effect( tmp, spell, i, false );
    }
  }

  template <typename T, typename... Ts>
  void force_effect( T data, unsigned idx, Ts... mods )
  {
    pack_t<player_effect_t> pack;
    const spell_data_t* spell = resolve_parse_data( data, pack );

    if ( !spell || !spell->ok() )
      return;

    if ( spell->affected_by_all( spell->effectN( idx ) ) )
    {
      assert( false && "Effect already affects player, no need to force" );
      return;
    }

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    parse_effect( pack, spell, idx, true );
  }

  template <typename U>
  double get_effect_value( const U& i, bool benefit = true ) const
  {
    if ( i.func && !i.func() )
      return 0.0;

    double eff_val = i.value;
    int mod = 1;

    if ( i.buff )
    {
      auto stack = benefit ? i.buff->stack() : i.buff->check();
      if ( !stack )
        return 0.0;

      if ( i.use_stacks )
        mod = stack;

      if ( i.type == USE_CURRENT )
        eff_val = i.buff->check_value();
    }

    if ( i.mastery )
      eff_val *= _player->cache.mastery();

    return mod * eff_val;
  }

  // Syntax: parse_target_effects( func, debuff[, spells|ignore_mask][,...] )
  //   (int F(TD*))            func: Function taking the target_data as argument and returning an integer mutiplier
  //   (const spell_data_t*) debuff: Spell data of the debuff
  //
  // The following optional arguments can be used in any order:
  //   (const spell_data_t*) spells: List of spells with redirect effects that modify the effects on the debuff
  //   (unsigned)       ignore_mask: Bitmask to skip effect# n corresponding to the n'th bit
  template <typename TD>
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

    if ( !is_valid_target_aura( eff ) )
      return;

    if ( tmp.data.value != 0.0 )
    {
      val = tmp.data.value;
      val_mul = 1.0;
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

    std::string type_str;
    bool flat = false;

    // Cast with TD parameter passed from wrappers to _parse_target_effects & _force_target_effect
    auto vec = static_cast<std::vector<target_effect_t<TD>>*>(
        get_target_effect_vector( eff, tmp.data.opt_enum, val_mul, type_str, flat, force ) );

    if ( !vec )
      return;

    val *= val_mul;

    std::string val_str = mastery ? fmt::format( "{}*mastery", val * 100 )
                          : flat  ? fmt::format( "{}", val )
                                  : fmt::format( "{}%", val * 100 );

    if ( tmp.data.value != 0.0 )
      val_str = val_str + " (overridden)";

    target_debug_message( type_str, val_str, s_data, i );

    tmp.data.value = val;
    tmp.data.mastery = mastery;
    tmp.data.eff = &eff;
    vec->push_back( tmp.data );
  }

  virtual bool is_valid_target_aura( const spelleffect_data_t& eff ) const { return false; }

  // Return void* as parse_effect_t is not templated and we don't know what TD is
  virtual void* get_target_effect_vector( const spelleffect_data_t& eff, uint32_t& opt_enum, double& val_mul,
                                          std::string& str, bool& flat, bool force ) = 0;

  virtual void target_debug_message( std::string_view type_str, std::string_view val_str, const spell_data_t* s_data,
                                     size_t i ) = 0;

  // Needs a wrapper to set TD parameter, as parse_effect_t is not templated
  template <typename TD, typename... Ts>
  void _parse_target_effects( const std::function<int( TD* )>& fn, const spell_data_t* spell, Ts... mods )
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

  // Needs a wrapper to set TD parameter, as parse_effect_t is not templated
  template <typename TD, typename... Ts>
  void _force_target_effect( const std::function<int( TD* )>& fn, const spell_data_t* spell, unsigned idx, Ts... mods )
  {
    pack_t<target_effect_t<TD>> pack;

    if ( !spell || !spell->ok() )
      return;

    if ( spell->affected_by_all( spell->effectN( idx ) ) )
    {
      assert( false && "Effect already affects player, no need to force" );
      return;
    }

    pack.data.func = std::move( fn );

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    parse_target_effect( pack, spell, idx, true );
  }

  template <typename U, typename TD>
  double get_target_effect_value( const U& i, TD* td ) const
  {
    if ( auto check = i.func( td ) )
    {
      auto eff_val = i.value;

      if ( i.mastery )
        eff_val *= _player->cache.mastery();

      return check * eff_val;
    }

    return 0.0;
  }
};

template <typename TD>
struct parse_player_effects_t : public player_t, public parse_effects_t
{
  std::vector<player_effect_t> melee_speed_effects;
  std::vector<player_effect_t> attribute_multiplier_effects;
  std::vector<player_effect_t> versatility_effects;
  std::vector<player_effect_t> player_multiplier_effects;
  std::vector<player_effect_t> pet_multiplier_effects;
  std::vector<player_effect_t> attack_power_multiplier_effects;
  std::vector<player_effect_t> crit_chance_effects;
  std::vector<player_effect_t> leech_effects;
  std::vector<player_effect_t> expertise_effects;
  std::vector<player_effect_t> crit_avoidance_effects;
  std::vector<player_effect_t> parry_effects;
  std::vector<player_effect_t> armor_multiplier_effects;
  std::vector<player_effect_t> haste_effects;
  std::vector<player_effect_t> mastery_effects;
  std::vector<target_effect_t<TD>> target_multiplier_effects;
  std::vector<target_effect_t<TD>> target_pet_multiplier_effects;

  parse_player_effects_t( sim_t* sim, player_e type, std::string_view name, race_e race )
    : player_t( sim, type, name, race ), parse_effects_t( this )
  {}

  double composite_melee_speed() const override
  {
    auto ms = player_t::composite_melee_speed();

    for ( const auto& i : melee_speed_effects )
      ms *= 1.0 / ( 1.0 + get_effect_value( i ) );

    return ms;
  }

  double composite_attribute_multiplier( attribute_e attr ) const override
  {
    auto am = player_t::composite_attribute_multiplier( attr );

    for ( const auto& i : attribute_multiplier_effects )
      if ( i.opt_enum & ( 1 << ( attr - 1 ) ) )
        am *= 1.0 + get_effect_value( i );

    return am;
  }

  double composite_damage_versatility() const override
  {
    auto v = player_t::composite_damage_versatility();

    for ( const auto& i : versatility_effects )
      v *= 1.0 + get_effect_value( i );

    return v;
  }

  double composite_heal_versatility() const override
  {
    auto v = player_t::composite_heal_versatility();

    for ( const auto& i : versatility_effects )
      v *= 1.0 + get_effect_value( i );

    return v;
  }

  double composite_mitigation_versatility() const override
  {
    auto v = player_t::composite_mitigation_versatility();

    for ( const auto& i : versatility_effects )
      v *= 1.0 + get_effect_value( i ) * 0.5;

    return v;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    auto m = player_t::composite_player_multiplier( school );

    for ( const auto& i : player_multiplier_effects )
      if ( i.opt_enum & dbc::get_school_mask( school ) )
        m *= 1.0 + get_effect_value( i );

    return m;
  }

  double composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const override
  {
    auto dm = player_t::composite_player_pet_damage_multiplier( s, guardian );

    for ( const auto& i : pet_multiplier_effects )
      if ( static_cast<bool>( i.opt_enum ) == guardian )
        dm *= 1.0 + get_effect_value( i );

    return dm;
  }

  double composite_attack_power_multiplier() const override
  {
    auto apm = player_t::composite_attack_power_multiplier();

    for ( const auto& i : attack_power_multiplier_effects )
      apm *= 1.0 + get_effect_value( i );

    return apm;
  }

  double composite_melee_crit_chance() const override
  {
    auto mcc = player_t::composite_melee_crit_chance();

    for ( const auto& i : crit_chance_effects )
      mcc += get_effect_value( i );

    return mcc;
  }

  double composite_spell_crit_chance() const override
  {
    auto scc = player_t::composite_spell_crit_chance();

    for ( const auto& i : crit_chance_effects )
      scc += get_effect_value( i );

    return scc;
  }

  double composite_leech() const override
  {
    auto leech = player_t::composite_leech();

    for ( const auto& i : leech_effects )
      leech += get_effect_value( i );

    return leech;
  }

  double composite_melee_expertise( const weapon_t* ) const override
  {
    auto me = player_t::composite_melee_expertise( nullptr );

    for ( const auto& i : expertise_effects )
      me += get_effect_value( i );

    return me;
  }

  double composite_crit_avoidance() const override
  {
    auto ca = player_t::composite_crit_avoidance();

    for ( const auto& i : crit_avoidance_effects )
      ca += get_effect_value( i );

    return ca;
  }

  double composite_parry() const override
  {
    auto parry = player_t::composite_parry();

    for ( const auto& i : parry_effects )
      parry += get_effect_value( i );

    return parry;
  }

  double composite_armor_multiplier() const override
  {
    auto am = player_t::composite_armor_multiplier();

    for ( const auto& i : armor_multiplier_effects )
      am *= 1.0 + get_effect_value( i );

    return am;
  }

  double composite_melee_haste() const override
  {
    auto mh = player_t::composite_melee_haste();

    for ( const auto& i : haste_effects )
      mh *= 1.0 / ( 1.0 + get_effect_value( i ) );

    return mh;
  }

  double composite_spell_haste() const override
  {
    auto sh = player_t::composite_spell_haste();

    for ( const auto& i : haste_effects )
      sh *= 1.0 / ( 1.0 + get_effect_value( i ) );

    return sh;
  }

  double composite_mastery() const override
  {
    auto m = player_t::composite_mastery();

    for ( const auto& i : mastery_effects )
      m *= 1.0 + get_effect_value( i );

    return m;
  }

private:
  TD* _get_td( player_t* t ) const
  {
    return static_cast<TD*>( get_target_data( t ) );
  }

public:
  double composite_player_target_multiplier( player_t* target, school_e school ) const override
  {
    auto tm = player_t::composite_player_target_multiplier( target, school );
    auto td = _get_td( target );

    for ( const auto& i : target_multiplier_effects )
      if ( i.opt_enum & dbc::get_school_mask( school ) )
        tm *= 1.0 + get_target_effect_value( i, td );

    return tm;
  }

  double composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const override
  {
    auto tm = player_t::composite_player_target_pet_damage_multiplier( target, guardian );
    auto td = _get_td( target );

    for ( const auto& i : target_pet_multiplier_effects )
      if ( static_cast<bool>( i.opt_enum ) == guardian )
        tm *= 1.0 + get_target_effect_value( i, td );

    return tm;
  }

  bool is_valid_aura( const spelleffect_data_t& eff ) const override
  {
    switch ( eff.type() )
    {
      case E_APPLY_AURA:
      case E_APPLY_AURA_PET:
        // TODO: more robust logic around 'party' buffs with radius
        if ( eff.radius() )
          return false;
        break;
      case E_APPLY_AREA_AURA_PARTY:
      case E_APPLY_AREA_AURA_PET:
        break;
      default:
        return false;
    }

    return true;
  }

  std::vector<player_effect_t>* get_effect_vector( const spelleffect_data_t& eff, uint32_t& opt_enum, double& val_mul,
                                                   std::string& str, bool& flat, bool force ) override
  {
    switch ( eff.subtype() )
    {
      case A_MOD_MELEE_SPEED_PCT:
      case A_MOD_RANGED_AND_MELEE_ATTACK_SPEED:
        str = "auto attack speed";
        return &melee_speed_effects;

      case A_MOD_TOTAL_STAT_PERCENTAGE:
        opt_enum = eff.misc_value2();
        {
          std::vector<std::string> str_list;
          for ( auto stat : { STAT_STRENGTH, STAT_AGILITY, STAT_STAMINA, STAT_INTELLECT, STAT_SPIRIT } )
            if ( opt_enum & ( 1 << ( stat - 1 ) ) )
              str_list.push_back( util::stat_type_string( stat ) );

          str = util::string_join( str_list );
        }
        return &attribute_multiplier_effects;

      case A_MOD_VERSATILITY_PCT:
        str = "versatility";
        return &versatility_effects;

      case A_MOD_DAMAGE_PERCENT_DONE:
        opt_enum = eff.misc_value1();
        str = opt_enum == 0x7f ? "all" : util::school_type_string( dbc::get_school_type( opt_enum ) );
        str += " damage";
        return &player_multiplier_effects;

      case A_MOD_PET_DAMAGE_DONE:
        opt_enum = 0;
        str = "pet damage";
        return &pet_multiplier_effects;

      case A_MOD_GUARDIAN_DAMAGE_DONE:
        opt_enum = 1;
        str = "guardian damage";
        return &pet_multiplier_effects;

      case A_MOD_ATTACK_POWER_PCT:
        str = "attack power";
        return &attack_power_multiplier_effects;

      case A_MOD_ALL_CRIT_CHANCE:
        str = "all crit chance";
        return &crit_chance_effects;

      case A_MOD_LEECH_PERCENT:
        str = "leech";
        return &leech_effects;

      case A_MOD_EXPERTISE:
        str = "expertise";
        return &expertise_effects;

      case A_MOD_ATTACKER_MELEE_CRIT_CHANCE:
        str = "crit avoidance";
        return &crit_avoidance_effects;

      case A_MOD_PARRY_PERCENT:
        str = "parry";
        return &parry_effects;

      case A_MOD_BASE_RESISTANCE_PCT:
        str = "armor multiplier";
        return &armor_multiplier_effects;

      case A_HASTE_ALL:
        str = "haste";
        return &haste_effects;

      case A_MOD_MASTERY_PCT:
        str = "mastery";
        val_mul = 1.0;
        return &mastery_effects;

      default:
        return nullptr;
    }

    return nullptr;
  }

  void debug_message( const player_effect_t& data, std::string_view type_str, std::string_view val_str, bool mastery,
                      const spell_data_t* s_data, size_t i ) override
  {
    if ( data.buff )
    {
      sim->print_debug( "player-effects: Player {} modified by {} {} buff {} ({}#{})", type_str, val_str,
                        data.use_stacks ? "per stack of" : "with", data.buff->name(), data.buff->data().id(), i );
    }
    else if ( mastery && !data.func )
    {
      sim->print_debug( "player-effects: Player {} modified by {} from {} ({}#{})", type_str, val_str,
                        s_data->name_cstr(), s_data->id(), i );
    }
    else if ( data.func )
    {
      sim->print_debug( "player-effects: Player {} modified by {} with condition from {} ({}#{})", type_str, val_str,
                        s_data->name_cstr(), s_data->id(), i );
    }
    else
    {
      sim->print_debug( "player-effects: Player {} modified by {} from {} ({}#{})", type_str, val_str,
                        s_data->name_cstr(), s_data->id(), i );
    }
  }

  bool is_valid_target_aura( const spelleffect_data_t& eff ) const override
  {
    if ( eff.type() == E_APPLY_AURA )
      return true;

    return false;
  }

  void* get_target_effect_vector( const spelleffect_data_t& eff, uint32_t& opt_enum, double& val_mul, std::string& str,
                                  bool& flat, bool force ) override
  {
    switch ( eff.subtype() )
    {
      case A_MOD_DAMAGE_FROM_CASTER:
        opt_enum = eff.misc_value1();
        str = opt_enum == 0x7f ? "all" : util::school_type_string( dbc::get_school_type( opt_enum ) );
        return &target_multiplier_effects;

      case A_MOD_DAMAGE_FROM_CASTER_PET:
        opt_enum = 0;
        str = "pet";
        return &target_pet_multiplier_effects;

      case A_MOD_DAMAGE_FROM_CASTER_GUARDIAN:
        opt_enum = 1;
        str = "guardian";
        return &target_pet_multiplier_effects;

      default:
        return nullptr;
    }

    return nullptr;
  }

  void target_debug_message( std::string_view type_str, std::string_view val_str, const spell_data_t* s_data,
                             size_t i ) override
  {
    sim->print_debug( "target-effects: Target {} damage taken modified by {} from {} ({}#{})", type_str, val_str,
                      s_data->name_cstr(), s_data->id(), i );
  }

  template <typename... Ts>
  void parse_target_effects( const std::function<int( TD* )>& fn, const spell_data_t* debuff, Ts&&... mods )
  {
    _parse_target_effects<TD>( fn, debuff, std::forward<Ts>( mods )... );
  }

  template <typename... Ts>
  void force_target_effect( const std::function<int( TD* )>& fn, const spell_data_t* debuff, unsigned idx,
                            Ts&&... mods )
  {
    _force_target_effect<TD>( fn, debuff, idx, std::forward<Ts>( mods )... );
  }
};

template <typename BASE, typename PLAYER, typename TD>
struct parse_action_effects_t : public BASE, public parse_effects_t
{
private:
  struct effect_pack_t
  {
    std::vector<const spelleffect_data_t*> permanent;
    double value = 0.0;
    std::vector<modify_effect_t> conditional;
  };

  std::array<effect_pack_t, 5> effect_modifiers;

public:
  // auto parsed dynamic effects
  std::vector<player_effect_t> ta_multiplier_effects;
  std::vector<player_effect_t> da_multiplier_effects;
  std::vector<player_effect_t> execute_time_effects;
  std::vector<player_effect_t> gcd_effects;
  std::vector<player_effect_t> dot_duration_effects;
  std::vector<player_effect_t> tick_time_effects;
  std::vector<player_effect_t> recharge_multiplier_effects;
  std::vector<player_effect_t> cost_effects;
  std::vector<player_effect_t> flat_cost_effects;
  std::vector<player_effect_t> crit_chance_effects;
  std::vector<player_effect_t> crit_damage_effects;
  std::vector<target_effect_t<TD>> target_multiplier_effects;
  std::vector<target_effect_t<TD>> target_crit_damage_effects;
  std::vector<target_effect_t<TD>> target_crit_chance_effects;

  parse_action_effects_t( std::string_view name, PLAYER* player, const spell_data_t* spell )
    : BASE( name, player, spell ), parse_effects_t( player )
  {
    for ( size_t i = 0; i < 5 && i < spell->effect_count(); i++ )
      effect_modifiers[ i ].value = spell->effectN( i + 1 ).base_value();
  }

  double cost_flat_modifier() const override
  {
    auto c = BASE::cost_flat_modifier();

    for ( const auto& i : flat_cost_effects )
      c += get_effect_value( i, false );

    return c;
  }

  double cost_pct_multiplier() const override
  {
    auto c = BASE::cost_pct_multiplier();

    for ( const auto& i : cost_effects )
      c *= 1.0 + get_effect_value( i, false );

    return c;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    auto ta = BASE::composite_ta_multiplier( s );

    for ( const auto& i : ta_multiplier_effects )
      ta *= 1.0 + get_effect_value( i );

    return ta;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    auto da = BASE::composite_da_multiplier( s );

    for ( const auto& i : da_multiplier_effects )
      da *= 1.0 + get_effect_value( i );

    return da;
  }

  double composite_crit_chance() const override
  {
    auto cc = BASE::composite_crit_chance();

    for ( const auto& i : crit_chance_effects )
      cc += get_effect_value( i );

    return cc;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    auto cd = BASE::composite_crit_damage_bonus_multiplier();

    for ( const auto& i : crit_damage_effects )
      cd *= get_effect_value( i );

    return cd;
  }

  timespan_t execute_time() const override
  {
    auto et = BASE::execute_time();

    for ( const auto& i : execute_time_effects )
      et *= 1.0 + get_effect_value( i );

    return std::max( 0_ms, et );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    auto dur = BASE::composite_dot_duration( s );

    for ( const auto& i : dot_duration_effects )
      dur *= 1.0 + get_effect_value( i );

    return std::max( 0_ms, dur );
  }

  timespan_t gcd() const override
  {
    auto g = BASE::gcd();

    for ( const auto& i : gcd_effects )
      g *= 1.0 + get_effect_value( i );

    return std::max( BASE::min_gcd, g );
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    auto tt = BASE::tick_time( s );

    for ( const auto& i : tick_time_effects )
      tt *= 1.0 + get_effect_value( i );

    return std::max( 1_ms, tt );
  }

  timespan_t cooldown_duration() const override
  {
    auto dur = BASE::cooldown_duration();

    for ( const auto& i : recharge_multiplier_effects )
      dur *= 1.0 + get_effect_value( i );

    return std::max( 0_ms, dur );
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    auto rm = BASE::recharge_multiplier( cd );

    for ( const auto& i : recharge_multiplier_effects )
      rm *= 1.0 + get_effect_value( i );

    return rm;
  }

private:
  // method for getting target data as each module may have different action scoped method
  TD* _get_td( player_t* t ) const
  {
    if constexpr ( std::is_invocable_v<decltype( &pet_t::owner ), PLAYER> )
      return static_cast<TD*>( static_cast<PLAYER*>( _player )->owner->get_target_data( t ) );
    else
      return static_cast<TD*>( _player->get_target_data( t ) );
  }

public:
  double composite_target_multiplier( player_t* t ) const override
  {
    auto tm = BASE::composite_target_multiplier( t );
    auto td = _get_td( t );

    for ( const auto& i : target_multiplier_effects )
      tm *= 1.0 + get_target_effect_value( i, td );

    return tm;
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    auto cc = BASE::composite_target_crit_chance( t );
    auto td = _get_td( t );

    for ( const auto& i : target_crit_chance_effects )
      cc += get_target_effect_value( i, td );

    return cc;
  }

  double composite_target_crit_damage_bonus_multiplier( player_t* t ) const override
  {
    auto cd = BASE::composite_target_crit_damage_bonus_multiplier( t );
    auto td = _get_td( t );

    for ( const auto& i : target_crit_damage_effects )
      cd *= 1.0 + get_target_effect_value( i, td );

    return cd;
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    os << "<div>\n";

    BASE::html_customsection( os );
    modified_effects_html( os );

    os << "</div>\n"
       << "<div>\n";

    parsed_effects_html( os );

    os << "</div>\n";
  }

  bool is_valid_aura( const spelleffect_data_t& eff ) const override
  {
    // Only parse apply aura effects
    switch ( eff.type() )
    {
      case E_APPLY_AURA:
      case E_APPLY_AURA_PET:
        // TODO: more robust logic around 'party' buffs with radius
        if ( eff.radius() )
          return false;
        break;
      case E_APPLY_AREA_AURA_PARTY:
      case E_APPLY_AREA_AURA_PET:
        break;
      default:
        return false;
    }

    return true;
  }

  std::vector<player_effect_t>* get_effect_vector( const spelleffect_data_t& eff, uint32_t& opt_enum, double& val_mul,
                                                   std::string& str, bool& flat, bool force ) override
  {
    if ( !BASE::special && eff.subtype() == A_MOD_AUTO_ATTACK_PCT )
    {
      str = "auto attack";
      return &da_multiplier_effects;
    }
    else if ( !BASE::data().affected_by_all( eff ) && !force )
    {
      return nullptr;
    }
    else if ( eff.subtype() == A_ADD_PCT_MODIFIER || eff.subtype() == A_ADD_PCT_LABEL_MODIFIER )
    {
      switch ( eff.misc_value1() )
      {
        case P_GENERIC:
          str = "direct damage";
          return &da_multiplier_effects;

        case P_DURATION:
          str = "duration";
          return &dot_duration_effects;

        case P_TICK_DAMAGE:
          str = "tick damage";
          return &ta_multiplier_effects;

        case P_CAST_TIME:
          str = "cast time";
          return &execute_time_effects;

        case P_GCD:
          str = "gcd";
          return &gcd_effects;

        case P_TICK_TIME:
          str = "tick time";
          return &tick_time_effects;

        case P_COOLDOWN:
          str = "cooldown";
          return &recharge_multiplier_effects;

        case P_RESOURCE_COST:
          str = "cost percent";
          return &cost_effects;

        case P_CRIT_DAMAGE:
          str = "crit damage";
          return &crit_damage_effects;

        default:
          return nullptr;
      }
    }
    else if ( eff.subtype() == A_ADD_FLAT_MODIFIER || eff.subtype() == A_ADD_FLAT_LABEL_MODIFIER )
    {
      flat = true;

      switch ( eff.misc_value1() )
      {
        case P_CRIT:
          str = "crit chance";
          return &crit_chance_effects;

        case P_RESOURCE_COST:
          val_mul = spelleffect_data_t::resource_multiplier( BASE::current_resource() );
          str = "flat cost";
          return &flat_cost_effects;

        default:
          return nullptr;
      }
    }

    return nullptr;
  }

  void debug_message( const player_effect_t& data, std::string_view type_str, std::string_view val_str, bool mastery,
                      const spell_data_t* s_data, size_t i ) override
  {
    if ( data.buff )
    {
      BASE::sim->print_debug( "action-effects: {} ({}) {} modified by {} {} buff {} ({}#{})", BASE::name(), BASE::id,
                              type_str, val_str, data.use_stacks ? "per stack of" : "with", data.buff->name(),
                              data.buff->data().id(), i );
    }
    else if ( mastery && !data.func )
    {
      BASE::sim->print_debug( "action-effects: {} ({}) {} modified by {} from {} ({}#{})", BASE::name(), BASE::id,
                              type_str, val_str, s_data->name_cstr(), s_data->id(), i );
    }
    else if ( data.func )
    {
      BASE::sim->print_debug( "action-effects: {} ({}) {} modified by {} with condition from {} ({}#{})", BASE::name(),
                              BASE::id, type_str, val_str, s_data->name_cstr(), s_data->id(), i );
    }
    else
    {
      BASE::sim->print_debug( "action-effects: {} ({}) {} modified by {} from {} ({}#{})", BASE::name(), BASE::id,
                              type_str, val_str, s_data->name_cstr(), s_data->id(), i );
    }
  }

  bool is_valid_target_aura( const spelleffect_data_t& eff ) const override
  {
    if ( eff.type() == E_APPLY_AURA )
      return true;

    return false;
  }

  virtual void* get_target_effect_vector( const spelleffect_data_t& eff, uint32_t& opt_enum, double& val_mul,
                                          std::string& str, bool& flat, bool force )
  {
    if ( !BASE::special && eff.subtype() == A_MOD_AUTO_ATTACK_FROM_CASTER )
    {
      str = "auto attack";
      return &target_multiplier_effects;
    }
    else if ( !BASE::data().affected_by_all( eff ) && !force )
    {
      return nullptr;
    }
    else
    {
      switch ( eff.subtype() )
      {
        case A_MOD_DAMAGE_FROM_CASTER_SPELLS:
        case A_MOD_DAMAGE_FROM_CASTER_SPELLS_LABEL:
          str = "damage";
          return &target_multiplier_effects;
        case A_MOD_CRIT_CHANCE_FROM_CASTER_SPELLS:
          flat = true;
          str = "crit chance";
          return &target_crit_chance_effects;
        case A_MOD_CRIT_DAMAGE_PCT_FROM_CASTER_SPELLS:
          str = "crit damage";
          return &target_crit_damage_effects;
        default:
          return nullptr;
      }
    }

    return nullptr;
  }

  void target_debug_message( std::string_view type_str, std::string_view val_str, const spell_data_t* s_data,
                             size_t i ) override

  {
    BASE::sim->print_debug( "target-effects: {} ({}) {} modified by {} on targets with debuff {} ({}#{})", BASE::name(),
                            BASE::id, type_str, val_str, s_data->name_cstr(), s_data->id(), i );
  }

  template <typename... Ts>
  void parse_target_effects( const std::function<int( TD* )>& fn, const spell_data_t* debuff, Ts&&... mods )
  {
    _parse_target_effects<TD>( fn, debuff, std::forward<Ts>( mods )... );
  }

  template <typename... Ts>
  void force_target_effect( const std::function<int( TD* )>& fn, const spell_data_t* debuff, unsigned idx,
                            Ts&&... mods )
  {
    _force_target_effect<TD>( fn, debuff, idx, std::forward<Ts>( mods )... );
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
      val_mul = 1.0;
    }
    else
    {
      apply_affecting_mods( tmp, val, m, s_data, i );
      val *= val_mul;
    }

    if ( !val )
      return;

    std::string val_str = flat ? fmt::format( "{}", val ) : fmt::format( "{}%", val * 100 );
    auto &effN = effect_modifiers[ idx ];

    // always active
    if ( !tmp.data.buff && !tmp.data.func )
    {
      BASE::sim->print_debug( "modify-effects: {} ({}#{}) permanently modified by {} from {} ({}#{})", BASE::name(),
                              BASE::id, idx + 1, val_str, s_data->name_cstr(), s_data->id(), i );

      if ( flat )
        effN.value += val;
      else
        effN.value *= 1.0 + val;

      effN.permanent.push_back( &eff );
    }
    // conditionally active
    else
    {
      BASE::sim->print_debug( "modify-effects: {} ({}#{}) conditionally modified by {} from {} ({}#{})", BASE::name(),
                              BASE::id, idx + 1, val_str, s_data->name_cstr(), s_data->id(), i );

      tmp.data.value = val;
      tmp.data.flat = flat;
      tmp.data.eff = &eff;

      effN.conditional.push_back( tmp.data );
    }
  }

  // return base value after modifiers
  double modified_effectN( size_t idx ) const
  {
    if ( !idx )
      return 0.0;

    auto return_value = effect_modifiers[ idx - 1 ].value;

    for ( const auto& i : effect_modifiers[ idx - 1 ].conditional )
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

  std::vector<modify_effect_t>& modified_effect_vec( size_t idx )
  {
    return effect_modifiers[ idx - 1 ].conditional;
  }

  void modified_effects_html( report::sc_html_stream& os )
  {
    if ( range::count_if( effect_modifiers, []( const effect_pack_t& e ) {
           return e.permanent.size() + e.conditional.size();
         } ) )
    {
      os << "<div>\n"
         << "<h4>Modified Effects</h4>\n"
         << "<table class=\"details nowrap\" style=\"width:min-content\">\n";

      os << "<tr>\n"
         << "<th class=\"small\">Effect</th>\n"
         << "<th class=\"small\">Type</th>\n"
         << "<th class=\"small\">Spell</th>\n"
         << "<th class=\"small\">ID</th>\n"
         << "<th class=\"small\">#</th>\n"
         << "<th class=\"small\">+/%</th>\n"
         << "<th class=\"small\">Value</th>\n"
         << "</tr>\n";

      print_parsed_effect( os, effect_modifiers[ 0 ], "Effect 1" );
      print_parsed_effect( os, effect_modifiers[ 1 ], "Effect 2" );
      print_parsed_effect( os, effect_modifiers[ 2 ], "Effect 3" );
      print_parsed_effect( os, effect_modifiers[ 3 ], "Effect 4" );
      print_parsed_effect( os, effect_modifiers[ 4 ], "Effect 5" );

      os << "</table>\n"
         << "</div>\n";
    }
  }

  void parsed_effects_html( report::sc_html_stream& os )
  {
    if ( total_effects_count() )
    {
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
      print_parsed_type( os, crit_damage_effects, "Critical Strike Damage" );
      print_parsed_type( os, execute_time_effects, "Execute Time" );
      print_parsed_type( os, gcd_effects, "GCD" );
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
  }

  virtual size_t total_effects_count()
  {
    return ta_multiplier_effects.size() +
           da_multiplier_effects.size() +
           execute_time_effects.size() +
           gcd_effects.size() +
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

  void print_parsed_line( report::sc_html_stream& os, const player_effect_t& entry )
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

  void print_parsed_effect( report::sc_html_stream& os, const effect_pack_t& effN, std::string_view n )
  {
    auto p = effN.permanent.size();
    auto c = effN.conditional.size();

    if ( p + c == 0 )
      return;

    bool first_row = true;

    os.format( "<tr><td rowspan=\"{}\">{}</td>\n", p + c, n );

    if ( p )
    {
      first_row = false;

      os.format( "<td rowspan=\"{}\">Permanent</td>", p );
      for ( size_t i = 0; i < p; i++ )
      {
        if ( i > 0 )
          os << "<tr>";

        print_parsed_effect_line( os, *effN.permanent[ i ] );
      }
    }

    if ( c )
    {
      if ( !first_row )
        os << "<tr>";

      os.format( "<td rowspan=\"{}\">Conditional</td>", c );
      for ( size_t i = 0; i < c; i++ )
      {
        if ( i > 0 )
          os << "<tr>";

        print_parsed_effect_line( os, *effN.conditional[ i ].eff );
      }
    }
  }

  void print_parsed_effect_line( report::sc_html_stream& os, const spelleffect_data_t& eff )
  {
    std::string op_str;

    switch ( eff.subtype() )
    {
      case A_ADD_PCT_MODIFIER:
      case A_ADD_PCT_LABEL_MODIFIER:
        op_str = "PCT";
        break;
      case A_ADD_FLAT_MODIFIER:
      case A_ADD_FLAT_LABEL_MODIFIER:
        op_str = "ADD";
        break;
      default:
        op_str = "UNK";
        break;
    }

    os.format( "<td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td></tr>\n",
                eff.spell()->name_cstr(),
                eff.spell()->id(),
                eff.index() + 1,
                op_str,
                eff.base_value() );
  }
};
