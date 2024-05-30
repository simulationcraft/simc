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
#include "report/decorators.hpp"
#include "sim/sim.hpp"
#include "util/io.hpp"

#include <functional>

enum parse_flag_e
{
  USE_DATA,
  USE_DEFAULT,
  USE_CURRENT,
  IGNORE_STACKS,
  ALLOW_ZERO
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
  uint32_t opt_enum = UINT32_MAX;

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

  bool operator==( const player_effect_t& other )
  {
    return buff == other.buff && value == other.value && type == other.type && use_stacks == other.use_stacks &&
           mastery == other.mastery && eff == other.eff && opt_enum == other.opt_enum;
  }

  std::string value_type_name( parse_flag_e ) const;

  void print_parsed_line( report::sc_html_stream&, const sim_t&, bool, std::function<std::string( uint32_t )>,
                          std::function<std::string( double )> ) const;
};

// effects dependent on target state
// TODO: add value type to debuffs if it becomes necessary in the future
struct target_effect_t
{
  std::function<int( actor_target_data_t* )> func = nullptr;
  double value = 0.0;
  bool mastery = false;
  const spelleffect_data_t* eff = &spelleffect_data_t::nil();
  uint32_t opt_enum = UINT32_MAX;

  target_effect_t& set_func( std::function<int( actor_target_data_t* )> f )
  { func = std::move( f ); return *this; }

  target_effect_t& set_value( double v )
  { value = v; return *this; }

  target_effect_t& set_mastery( bool m )
  { mastery = m; return *this; }

  target_effect_t& set_eff( const spelleffect_data_t* e )
  { eff = e; return *this; }

  target_effect_t& set_opt_enum( uint32_t e )
  { opt_enum = e; return *this; }

  bool operator==( const target_effect_t& other )
  {
    return value == other.value && mastery == other.mastery && eff == other.eff && opt_enum == other.opt_enum;
  }

  void print_parsed_line( report::sc_html_stream&, const sim_t&, bool, std::function<std::string( uint32_t )>,
                          std::function<std::string( double )> ) const;
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

// handy wrapper to create ignore masks with verbose effect index enable/disable methods
struct effect_mask_t
{
  uint32_t mask;

  effect_mask_t( bool b ) : mask( b ? 0 : UINT32_MAX ) {}

  effect_mask_t& disable( uint32_t i )
  { mask |= 1 << ( i - 1 ); return *this; }

  template <typename... Ts>
  effect_mask_t& disable( uint32_t i, Ts... idxs )
  { disable( i ); return disable( idxs... ); }

  effect_mask_t& enable( uint32_t i )
  { mask &= ~( 1 << ( i - 1 ) ); return *this; }

  template <typename... Ts>
  effect_mask_t& enable( uint32_t i, Ts... idxs )
  { enable( i ); return enable( idxs... ); }

  operator uint32_t() const
  { return mask; }
};

// used to store values from parameter pack recursion of parse_effect/parse_target_effects
template <typename U, typename = std::enable_if_t<std::is_default_constructible_v<U>>>
struct pack_t
{
  U data;
  std::vector<const spell_data_t*> list;
  uint32_t mask = 0U;
  std::vector<U>* copy = nullptr;
};

template <typename U>
static inline U& add_parse_entry( std::vector<U>& vec ) { return vec.emplace_back(); }

// input interface framework
struct parse_base_t
{
  parse_base_t() = default;
  virtual ~parse_base_t() = default;

  // detectors for is_detected_v<>
  template <typename T> using detect_buff = decltype( T::buff );
  template <typename T> using detect_func = decltype( T::func );
  template <typename T> using detect_use_stacks = decltype( T::use_stacks );
  template <typename T> using detect_type = decltype( T::type );
  template <typename T> using detect_value = decltype( T::value );

  // handle first argument to parse_effects(), set buff if necessary and return spell_data_t*
  template <typename T, typename U>
  const spell_data_t* resolve_parse_data( T data, pack_t<U>& tmp )
  {
    if constexpr ( std::is_invocable_v<decltype( &buff_t::data ), T> )
    {
      if ( !data )
        return nullptr;

      if constexpr ( is_detected_v<detect_buff, U> )
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

  // returns the value of the effect. overload if different value access method is needed for other spell_data_t*
  // castable types, such as conduits
  double mod_spell_effects_value( const spell_data_t*, const spelleffect_data_t& e ) { return e.base_value(); }

  template <typename T>
  void apply_affecting_mod( double&, bool&, const spell_data_t*, size_t, T );

  template <typename U>
  void apply_affecting_mods( const pack_t<U>& tmp, double& val, bool& mastery, const spell_data_t* base, size_t idx )
  {
    // Apply effect modifying effects from mod list. Blizz only currently supports modifying effects 1-5
    if ( idx > 5 )
      return;

    for ( size_t j = 0; j < tmp.list.size(); j++ )
      apply_affecting_mod( val, mastery, base, idx, tmp.list[ j ] );
  }

  // populate pack with optional arguments to parse_effects().
  // parsing order of precedence is:
  // 1) spell_data_t* added to list of spells to check for effect modifying effects
  // 2) bool F() functor to set condition for effect to apply.
  // 3) parse flags such as IGNORE_STACKS, USE_DEFAULT, USE_CURRENT
  // 4) floating point value to directly set the effect value and override all parsing
  // 5) integral bitmask to ignore effect# n corresponding to the n'th bit
  template <typename U, typename T>
  void parse_spell_effect_mod( pack_t<U>& tmp, T mod )
  {
    if constexpr ( std::is_invocable_v<decltype( &spell_data_t::ok ), T> )
    {
      tmp.list.push_back( mod );
    }
    else if constexpr ( std::is_convertible_v<T, std::function<bool()>> && is_detected_v<detect_func, U> )
    {
      tmp.data.func = std::move( mod );
    }
    else if constexpr ( std::is_same_v<T, parse_flag_e> )
    {
      if constexpr ( is_detected_v<detect_use_stacks, U> )
      {
        if ( mod == IGNORE_STACKS )
          tmp.data.use_stacks = false;
      }

      if constexpr ( is_detected_v<detect_type, U> )
      {
        if ( mod == USE_DEFAULT || mod == USE_CURRENT )
          tmp.data.type = mod;
      }
    }
    else if constexpr ( std::is_floating_point_v<T> && is_detected_v<detect_value, U> )
    {
      tmp.data.value = mod;
    }
    else if constexpr ( std::is_same_v<T, effect_mask_t> || ( std::is_integral_v<T> && !std::is_same_v<T, bool> ) )
    {
      tmp.mask = mod;
    }
    else if constexpr ( std::is_convertible_v<decltype( *std::declval<T>() ), const std::vector<U>> )
    {
      tmp.copy = &( *mod );
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

struct modified_spelleffect_t
{
  const spelleffect_data_t& _eff;
  double value;
  mutable std::vector<modify_effect_t> conditional;
  std::vector<const spelleffect_data_t*> permanent;

  modified_spelleffect_t( const spelleffect_data_t& eff ) : _eff( eff ), value( eff.base_value() ) {}

  modified_spelleffect_t() : _eff( spelleffect_data_t::nil() ), value( 0.0 ) {}

  // return base value after modifiers
  double base_value() const;

  double percent() const
  { return base_value() * 0.01; }

  double resource( resource_e r ) const
  { return base_value() * _eff.resource_multiplier( r ); }

  double resource() const
  { return resource( _eff.resource_gain_type() ); }

  operator const spelleffect_data_t&() const
  { return _eff; }

  modify_effect_t& add_parse_entry() const
  { return conditional.emplace_back(); }

  size_t size() const
  { return conditional.size() + permanent.size(); }

  bool modified_by( const spelleffect_data_t& ) const;

  void print_parsed_line( report::sc_html_stream&, const sim_t&, const modify_effect_t& ) const;

  void print_parsed_line( report::sc_html_stream&, const sim_t&, const spelleffect_data_t& ) const;

  void print_parsed_effect( report::sc_html_stream&, const sim_t&, size_t& ) const;

  static const modified_spelleffect_t& nil();
};

inline const modified_spelleffect_t modified_spelleffect_nil_v = modified_spelleffect_t();
inline const modified_spelleffect_t& modified_spelleffect_t::nil() { return modified_spelleffect_nil_v; }

// Modifiable spell_data_t analogue that can be used to parse and apply effect affecting effects (P_EFFECT_1-5)
//
// obj->parse_effects( <parse_effects arguments> ) to parse and apply permanent and conditional effects, controlled via
//   arguments in the same manner as action & player scoped parse_effects().
//
// obj->effectN( # ) to access the modified_spelleffect_t which is analogous to spelleffect_data_t.
//
// obj->effectN( # ).base_value(), .percent(), .resource(), .resource( r ) to get the dynamically modified values.
struct modified_spell_data_t : public parse_base_t
{
  std::vector<modified_spelleffect_t> effects;
  const spell_data_t& _spell;

  modified_spell_data_t( const spell_data_t* s ) : modified_spell_data_t( *s ) {}

  modified_spell_data_t( const spell_data_t& s = *spell_data_t::nil() ) : _spell( s )
  {
    for ( const auto& eff : s.effects() )
      effects.emplace_back( eff );
  }

  const modified_spelleffect_t& effectN( size_t ) const;

  operator const spell_data_t&() const
  { return _spell; }

  operator const spell_data_t*() const
  { return &_spell; }

  modify_effect_t& add_parse_entry( size_t idx )
  { return effects[ idx - 1 ].add_parse_entry(); }

  template <typename T, typename... Ts>
  modified_spell_data_t* parse_effects( T data, Ts... mods )
  {
    if ( !_spell.ok() )
      return this;

    pack_t<modify_effect_t> pack;
    const spell_data_t* spell = resolve_parse_data( data, pack );

    if ( !spell || !spell->ok() )
      return this;

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    for ( size_t i = 1; i <= spell->effect_count(); i++ )
    {
      if ( pack.mask & 1 << ( i - 1 ) )
        continue;

      // local copy of pack per effect
      pack_t<modify_effect_t> tmp = pack;

      parse_effect( tmp, spell, i );
    }

    return this;
  }

  void parse_effect( pack_t<modify_effect_t>&, const spell_data_t*, size_t );

  void print_parsed_spell( report::sc_html_stream&, const sim_t& );

  static void parsed_effects_html( report::sc_html_stream&, const sim_t&, std::vector<modified_spell_data_t*> );

  static modified_spell_data_t* nil();
};

inline modified_spell_data_t modified_spell_data_nil_v = modified_spell_data_t();
inline modified_spell_data_t* modified_spell_data_t::nil() { return &modified_spell_data_nil_v; }

struct parse_effects_t : public parse_base_t
{
protected:
  player_t* _player;

public:
  parse_effects_t( player_t* p ) : _player( p ) {}
  virtual ~parse_effects_t() = default;

  template <typename U>
  void parse_effect( pack_t<U>&, const spell_data_t*, size_t, bool );

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
  virtual bool is_valid_aura( const spelleffect_data_t& ) const { return false; }

  virtual std::vector<player_effect_t>* get_effect_vector( const spelleffect_data_t& eff, player_effect_t& data,
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

  double get_effect_value( const player_effect_t&, bool benefit = false ) const;

  // Syntax: parse_target_effects( func, debuff[, spells|ignore_mask][,...] )
  //   (int F(TD*))            func: Function taking the target_data as argument and returning an integer mutiplier
  //   (const spell_data_t*) debuff: Spell data of the debuff
  //
  // The following optional arguments can be used in any order:
  //   (const spell_data_t*) spells: List of spells with redirect effects that modify the effects on the debuff
  //   (unsigned)       ignore_mask: Bitmask to skip effect# n corresponding to the n'th bit
  virtual bool is_valid_target_aura( const spelleffect_data_t& ) const { return false; }

  virtual std::vector<target_effect_t>* get_target_effect_vector( const spelleffect_data_t& eff, target_effect_t& data,
                                                                  double& val_mul, std::string& str, bool& flat,
                                                                  bool force ) = 0;

  virtual void target_debug_message( std::string_view type_str, std::string_view val_str, const spell_data_t* s_data,
                                     size_t i ) = 0;

  template <typename... Ts>
  void parse_target_effects( const std::function<int( actor_target_data_t* )>& fn, const spell_data_t* spell,
                             Ts... mods )
  {
    pack_t<target_effect_t> pack;

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
      pack_t<target_effect_t> tmp = pack;

      parse_effect( tmp, spell, i, false );
    }
  }

  template <typename... Ts>
  void force_target_effect( const std::function<int( actor_target_data_t* )>& fn, const spell_data_t* spell,
                            unsigned idx, Ts... mods )
  {
    pack_t<target_effect_t> pack;

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

    parse_effect( pack, spell, idx, true );
  }

  double get_target_effect_value( const target_effect_t&, actor_target_data_t* ) const;
};

struct parse_player_effects_t : public player_t, public parse_effects_t
{
  std::vector<player_effect_t> auto_attack_speed_effects;
  std::vector<player_effect_t> attribute_multiplier_effects;
  std::vector<player_effect_t> matching_armor_attribute_multiplier_effects;
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
  std::vector<player_effect_t> parry_rating_from_crit_effects;
  std::vector<player_effect_t> dodge_effects;
  std::vector<target_effect_t> target_multiplier_effects;
  std::vector<target_effect_t> target_pet_multiplier_effects;

  // Cache Pairing, invalidate first of the pair when the second is invalidated
  std::vector<std::pair<cache_e, cache_e>> invalidate_with_parent;

  parse_player_effects_t( sim_t* sim, player_e type, std::string_view name, race_e race )
    : player_t( sim, type, name, race ), parse_effects_t( this )
  {}

  double composite_melee_auto_attack_speed() const override;
  double composite_attribute_multiplier( attribute_e ) const override;
  double composite_damage_versatility() const override;
  double composite_heal_versatility() const override;
  double composite_mitigation_versatility() const override;
  double composite_player_multiplier( school_e ) const override;
  double composite_player_pet_damage_multiplier( const action_state_t*, bool ) const override;
  double composite_attack_power_multiplier() const override;
  double composite_melee_crit_chance() const override;
  double composite_spell_crit_chance() const override;
  double composite_leech() const override;
  double composite_melee_expertise( const weapon_t* ) const override;
  double composite_crit_avoidance() const override;
  double composite_parry() const override;
  double composite_armor_multiplier() const override;
  double composite_melee_haste() const override;
  double composite_spell_haste() const override;
  double composite_mastery() const override;
  double composite_parry_rating() const override;
  double composite_dodge() const override;
  double matching_gear_multiplier( attribute_e ) const override;
  double composite_player_target_multiplier( player_t*, school_e ) const override;
  double composite_player_target_pet_damage_multiplier( player_t*, bool ) const override;

  void invalidate_cache( cache_e c ) override;

  bool is_valid_aura( const spelleffect_data_t& ) const override;

  std::vector<player_effect_t>* get_effect_vector( const spelleffect_data_t&, player_effect_t&, double&, std::string&,
                                                   bool&, bool ) override;

  void debug_message( const player_effect_t&, std::string_view, std::string_view, bool, const spell_data_t*,
                      size_t ) override;

  bool is_valid_target_aura( const spelleffect_data_t& ) const override;

  std::vector<target_effect_t>* get_target_effect_vector( const spelleffect_data_t&, target_effect_t&, double&,
                                                          std::string&, bool&, bool ) override;

  void target_debug_message( std::string_view, std::string_view, const spell_data_t*, size_t ) override;

  void parsed_effects_html( report::sc_html_stream& );

  virtual size_t total_effects_count();

  virtual void print_parsed_custom_type( report::sc_html_stream& ) {}

  template <typename U>
  void print_parsed_type( report::sc_html_stream& os, const std::vector<U>& entries, std::string_view n,
                          std::function<std::string( uint32_t )> note_fn = nullptr,
                          std::function<std::string( double )> val_str_fn = nullptr )
  {
    auto c = entries.size();
    if ( !c )
      return;

    os.format( "<tr><td rowspan=\"{}\" class=\"dark\">{}</td>", c, n );

    for ( size_t i = 0; i < c; i++ )
    {
      if ( i > 0 )
        os << "<tr>";

      entries[ i ].print_parsed_line( os, *sim, true, note_fn, val_str_fn );
    }
  }
};

template <typename BASE>
struct parse_action_effects_t : public BASE, public parse_effects_t
{
private:
  using base_t = parse_action_effects_t<BASE>;

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
  std::vector<player_effect_t> crit_chance_multiplier_effects;
  std::vector<player_effect_t> crit_damage_effects;
  std::vector<target_effect_t> target_multiplier_effects;
  std::vector<target_effect_t> target_crit_damage_effects;
  std::vector<target_effect_t> target_crit_chance_effects;

  parse_action_effects_t( std::string_view name, player_t* player, const spell_data_t* spell )
    : BASE( name, player, spell ), parse_effects_t( player )
  {}

  double cost_flat_modifier() const override
  {
    auto c = BASE::cost_flat_modifier();

    for ( const auto& i : flat_cost_effects )
      c += get_effect_value( i );

    return c;
  }

  double cost_pct_multiplier() const override
  {
    auto c = BASE::cost_pct_multiplier();

    for ( const auto& i : cost_effects )
      c *= 1.0 + get_effect_value( i );

    return c;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    auto ta = BASE::composite_ta_multiplier( s );

    for ( const auto& i : ta_multiplier_effects )
      ta *= 1.0 + get_effect_value( i, true );

    return ta;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    auto da = BASE::composite_da_multiplier( s );

    for ( const auto& i : da_multiplier_effects )
      da *= 1.0 + get_effect_value( i, true );

    return da;
  }

  double composite_crit_chance() const override
  {
    auto cc = BASE::composite_crit_chance();

    for ( const auto& i : crit_chance_effects )
      cc += get_effect_value( i );

    return cc;
  }

  double composite_crit_chance_multiplier() const override
  {
    auto ccm = BASE::composite_crit_chance_multiplier();

    for ( const auto& i : crit_chance_multiplier_effects )
      ccm *= 1.0 + get_effect_value( i );

    return ccm;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    auto cd = BASE::composite_crit_damage_bonus_multiplier();

    for ( const auto& i : crit_damage_effects )
      cd *= 1.0 + get_effect_value( i, true );

    return cd;
  }

  timespan_t execute_time() const override
  {
    auto et = BASE::execute_time();

    for ( const auto& i : execute_time_effects )
      et *= 1.0 + get_effect_value( i, true );

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

  double composite_target_multiplier( player_t* t ) const override
  {
    auto tm = BASE::composite_target_multiplier( t );
    auto td = _player->get_target_data( t );

    for ( const auto& i : target_multiplier_effects )
      tm *= 1.0 + get_target_effect_value( i, td );

    return tm;
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    auto cc = BASE::composite_target_crit_chance( t );
    auto td = _player->get_target_data( t );

    for ( const auto& i : target_crit_chance_effects )
      cc += get_target_effect_value( i, td );

    return cc;
  }

  double composite_target_crit_damage_bonus_multiplier( player_t* t ) const override
  {
    auto cd = BASE::composite_target_crit_damage_bonus_multiplier( t );
    auto td = _player->get_target_data( t );

    for ( const auto& i : target_crit_damage_effects )
      cd *= 1.0 + get_target_effect_value( i, td );

    return cd;
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    os << "<div>\n";

    BASE::html_customsection( os );

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

  std::vector<player_effect_t>* get_effect_vector( const spelleffect_data_t& eff, player_effect_t& data,
                                                   double& val_mul, std::string& str, bool& flat, bool force ) override
  {
    auto adjust_recharge_multiplier_warning = [ this, &data ] {
      if ( BASE::sim->debug && data.buff && !data.buff->stack_change_callback )
      {
        BASE::sim->error( "WARNING: {} adjusts cooldown of {} but does not have a stack change callback.\n\r"
                          "Make sure adjust_recharge_multiplier() is properly called.", *data.buff, *this );
      }
    };

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
        case P_GENERIC:       str = "direct damage";          return &da_multiplier_effects;
        case P_DURATION:      str = "duration";               return &dot_duration_effects;
        case P_TICK_DAMAGE:   str = "tick damage";            return &ta_multiplier_effects;
        case P_CAST_TIME:     str = "cast time";              return &execute_time_effects;
        case P_GCD:           str = "gcd";                    return &gcd_effects;
        case P_TICK_TIME:     str = "tick time";              return &tick_time_effects;
        case P_RESOURCE_COST: str = "cost percent";           return &cost_effects;
        case P_CRIT:          str = "crit chance multiplier"; return &crit_chance_multiplier_effects;
        case P_CRIT_DAMAGE:   str = "crit damage";            return &crit_damage_effects;
        case P_COOLDOWN:      adjust_recharge_multiplier_warning();
                              str = "cooldown";               return &recharge_multiplier_effects;
        default:              return nullptr;
      }
    }
    else if ( eff.subtype() == A_ADD_FLAT_MODIFIER || eff.subtype() == A_ADD_FLAT_LABEL_MODIFIER )
    {
      flat = true;

      switch ( eff.misc_value1() )
      {
        case P_CRIT:          str = "crit chance"; return &crit_chance_effects;
        case P_RESOURCE_COST: val_mul = spelleffect_data_t::resource_multiplier( BASE::current_resource() );
                              str = "flat cost";   return &flat_cost_effects;
        default:              return nullptr;
      }
    }
    else if ( eff.subtype() == A_MOD_RECHARGE_RATE_LABEL || eff.subtype() == A_MOD_RECHARGE_RATE_CATEGORY )
    {
      str = "cooldown";
      adjust_recharge_multiplier_warning();
      return &recharge_multiplier_effects;
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

  std::vector<target_effect_t>* get_target_effect_vector( const spelleffect_data_t& eff, target_effect_t& /* data */,
                                                          double& /* val_mul */, std::string& str, bool& flat,
                                                          bool force ) override
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
        case A_MOD_DAMAGE_FROM_CASTER_SPELLS_LABEL:    str = "damage";      return &target_multiplier_effects;
        case A_MOD_CRIT_CHANCE_FROM_CASTER_SPELLS:     flat = true;
                                                       str = "crit chance"; return &target_crit_chance_effects;
        case A_MOD_CRIT_DAMAGE_PCT_FROM_CASTER_SPELLS: str = "crit damage"; return &target_crit_damage_effects;
        default:                                       return nullptr;
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

  void parsed_effects_html( report::sc_html_stream& os )
  {
    if ( total_effects_count() )
    {
      os << "<div>"
         << "<h4>Affected By (Dynamic)</h4>"
         << "<table class=\"details nowrap\" style=\"width:min-content\">\n";

      os << "<tr>"
         << "<th class=\"small\">Type</th>"
         << "<th class=\"small\">Spell</th>"
         << "<th class=\"small\">ID</th>"
         << "<th class=\"small\">#</th>"
         << "<th class=\"small\">Value</th>"
         << "<th class=\"small\">Source</th>"
         << "<th class=\"small\">Notes</th>"
         << "</tr>\n";

      print_parsed_type( os, &base_t::da_multiplier_effects, "Direct Damage" );
      print_parsed_type( os, &base_t::ta_multiplier_effects, "Periodic Damage" );
      print_parsed_type( os, &base_t::crit_chance_effects, "Critical Strike Chance" );
      print_parsed_type( os, &base_t::crit_damage_effects, "Critical Strike Damage" );
      print_parsed_type( os, &base_t::execute_time_effects, "Execute Time" );
      print_parsed_type( os, &base_t::gcd_effects, "GCD" );
      print_parsed_type( os, &base_t::dot_duration_effects, "Dot Duration" );
      print_parsed_type( os, &base_t::tick_time_effects, "Tick Time" );
      print_parsed_type( os, &base_t::recharge_multiplier_effects, "Recharge Multiplier" );
      print_parsed_type( os, &base_t::flat_cost_effects, "Flat Cost", []( double v ) { return fmt::to_string( v ); } );
      print_parsed_type( os, &base_t::cost_effects, "Percent Cost" );
      print_parsed_type( os, &base_t::target_multiplier_effects, "Damage on Debuff" );
      print_parsed_type( os, &base_t::target_crit_chance_effects, "Crit Chance on Debuff" );
      print_parsed_type( os, &base_t::target_crit_damage_effects, "Crit Damage on Debuff" );
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

  template <typename W = base_t, typename V>
  void print_parsed_type( report::sc_html_stream& os, V vector_ptr, std::string_view n,
                          std::function<std::string( double )> val_str_fn = nullptr )
  {
    auto entries = std::invoke( vector_ptr, static_cast<W*>( this ) );

    // assuming the stats obj being processed isn't orphaned (as can happen in debug=1 with child actions with stats
    // replaced by parent's stats), go through all the actions assigned to the stats obj and populate with all unique
    // entries
    if ( range::contains( BASE::stats->action_list, this ) )
      for ( auto a : BASE::stats->action_list )
        if ( a != this )
          for ( const auto& entry : std::invoke( vector_ptr, static_cast<W*>( a ) ) )
            if ( !range::contains( entries, entry ) )
              entries.push_back( entry );

    auto c = entries.size();
    if ( !c )
      return;

    os.format( "<tr><td class=\"label\" rowspan=\"{}\">{}</td>\n", c, n );

    for ( size_t i = 0; i < c; i++ )
    {
      if ( i > 0 )
        os << "<tr>";

      entries[ i ].print_parsed_line( os, *BASE::sim, false, nullptr, val_str_fn );
    }
  }
};
