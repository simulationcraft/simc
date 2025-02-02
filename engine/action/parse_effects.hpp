// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "action/action.hpp"
#include "buff/buff.hpp"
#include "player/player.hpp"
#include "player/stats.hpp"
#include "util/io.hpp"

enum parse_flag_e : uint16_t
{
  USE_DATA          = 0x0000,
  USE_DEFAULT       = 0x0001,
  USE_CURRENT       = 0x0002,
  IGNORE_STACKS     = 0x0004,
  ALLOW_ZERO        = 0x0008,
  EXPIRE_BUFF       = 0x0010,
  DECREMENT_BUFF    = 0x0020,
  // internal flags that should not be used in parse_effects()
  VALUE_OVERRIDE    = 0x0100,
  AFFECTED_OVERRIDE = 0x0200,
  MANUAL_ENTRY      = 0x0400
};

enum parse_callback_e
{
  PARSE_CALLBACK_PRE_IMPACT,
  PARSE_CALLBACK_POST_IMPACT,
  PARSE_CALLBACK_POST_EXECUTE,
};

// effects dependent on player state
struct player_effect_t
{
  // simple processing
  bool simple = true;
  buff_t* buff = nullptr;
  double value = 0.0;
  bool use_stacks = true;
  // full processing
  std::function<bool()> func = nullptr;
  std::function<double( double )> value_func = nullptr;
  uint16_t type = USE_DATA;
  bool mastery = false;
  uint32_t idx = 0;  // index of parse_action_base_t::callback_list
  // effect linkback
  const spelleffect_data_t* eff = &spelleffect_data_t::nil();
  // optional enum identifier
  uint32_t opt_enum = UINT32_MAX;

  player_effect_t& set_buff( buff_t* b )
  { buff = b; return *this; }

  player_effect_t& set_value( double v )
  { value = v; return *this; }

  player_effect_t& set_use_stacks( bool s )
  { use_stacks = s; simple = false; return *this; }

  player_effect_t& set_func( std::function<bool()> f )
  { func = std::move( f ); simple = false; return *this; }

  player_effect_t& set_value_func( std::function<double( double )> f )
  { value_func = std::move( f ); simple = false; return *this; }

  player_effect_t& set_type( uint8_t t )
  { type = t; simple = false; return *this; }

  player_effect_t& set_mastery( bool m )
  { mastery = m; simple = false; return *this; }

  player_effect_t& set_idx( uint32_t i )
  { idx = i; simple = false; return *this; }

  player_effect_t& set_eff( const spelleffect_data_t* e )
  { eff = e; return *this; }

  player_effect_t& set_opt_enum( uint32_t o )
  { opt_enum = o; return *this; }

  bool operator==( const player_effect_t& other )
  {
    return simple == other.simple && buff == other.buff && value == other.value && use_stacks == other.use_stacks &&
           type == other.type && mastery == other.mastery && idx == other.idx && eff == other.eff &&
           opt_enum == other.opt_enum;
  }

  std::string value_type_name( uint16_t ) const;

  void print_parsed_line( report::sc_html_stream&, const sim_t&, bool,
                          const std::function<std::string( uint32_t )>&,
                          const std::function<std::string( double )>& ) const;
};

// effects dependent on target state
struct target_effect_t
{
  std::function<double( actor_target_data_t* )> func = nullptr;
  double value = 0.0;
  uint16_t type = USE_DATA;  // for internal flags only
  bool mastery = false;
  const spelleffect_data_t* eff = &spelleffect_data_t::nil();
  uint32_t opt_enum = UINT32_MAX;

  target_effect_t& set_func( std::function<double( actor_target_data_t* )> f )
  { func = std::move( f ); return *this; }

  target_effect_t& set_value( double v )
  { value = v; return *this; }

  target_effect_t& set_mastery( bool m )
  { mastery = m; return *this; }

  target_effect_t& set_eff( const spelleffect_data_t* e )
  { eff = e; return *this; }

  target_effect_t& set_opt_enum( uint32_t o )
  { opt_enum = o; return *this; }

  bool operator==( const target_effect_t& other )
  {
    return value == other.value && mastery == other.mastery && eff == other.eff && opt_enum == other.opt_enum;
  }

  std::string value_type_name( uint16_t ) const;

  void print_parsed_line( report::sc_html_stream&, const sim_t&, bool,
                          const std::function<std::string( uint32_t )>&,
                          const std::function<std::string( double )>& ) const;
};

struct modify_effect_t
{
  buff_t* buff = nullptr;
  std::function<bool( const action_t*, const action_state_t* )> func = nullptr;
  double value = 0.0;
  bool use_stacks = true;
  bool flat = false;
  const spelleffect_data_t* eff = &spelleffect_data_t::nil();

  modify_effect_t& set_buff( buff_t* b )
  { buff = b; return *this; }

  modify_effect_t& set_func( std::function<bool( const action_t*, const action_state_t* )> f )
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

/*
 * Wrapper to create ignore masks effects for parse_effects and parse_target_effects.
 * effect_mask_t( bool b ): default state for effects (true = all enabled, false = all disabled)
 * enable( ints... ): enable effects by index
 * disable( ints... ): disable effects by index
 *
 * e.g.
 * effect_mask_t( true ).disable( 2, 4 ); enables all effects, then disables 2 and 4
 */
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

/*
 * Modifies whitelists of effects.
 * affect_list_t( ints... ): selects effects to modify
 * add/remove_family( ints... ): add/remove by family flag
 * add/remove_label( ints... ): add/remove by label id
 * add/remove_spell( ints... ): add/remove by spell id
 *
 * affect_list_t( 1, 3 ).add_spell( 1, 2 ).remove_spell( 3 );
 * modifies effect 1 and 3 whitelist. adds spell id 1 & 2, removes spell id 3
 */
struct affect_list_t
{
  std::vector<uint8_t> idx;
  std::vector<int8_t> family;
  std::vector<int16_t> label;
  std::vector<int32_t> spell;

  affect_list_t() = default;

  affect_list_t( uint8_t i ) { idx.push_back( i ); }

  template <typename... Ts>
  affect_list_t( uint8_t i, Ts... is ) : affect_list_t( is... ) { idx.push_back( i ); }

  affect_list_t& add_family( int8_t f )
  { family.push_back( f ); return *this; }

  affect_list_t& remove_family( int8_t f )
  { family.push_back( -f ); return *this; }

  template <typename... Ts>
  affect_list_t& add_family( int8_t f, Ts... fs )
  { add_family( f ); return add_family( fs... ); }

  template <typename... Ts>
  affect_list_t& remove_family( int8_t f, Ts... fs )
  { remove_family( f ); return remove_family( fs... ); }

  affect_list_t& add_label( int16_t l )
  { label.push_back( l ); return *this; }

  affect_list_t& remove_label( int16_t l )
  { label.push_back( -l ); return *this; }

  template <typename... Ts>
  affect_list_t& add_label( int16_t l, Ts... ls )
  { add_label( l ); return add_label( ls... ); }

  template <typename... Ts>
  affect_list_t& remove_label( int16_t l, Ts... ls )
  { remove_label( l ); return remove_label( ls... ); }

  affect_list_t& add_spell( int32_t s )
  { spell.push_back( s ); return *this; }

  affect_list_t& remove_spell( int32_t s )
  { spell.push_back( -s ); return *this; }

  template <typename... Ts>
  affect_list_t& add_spell( int32_t s, Ts... ss )
  { add_spell( s ); return add_spell( ss... ); }

  template <typename... Ts>
  affect_list_t& remove_spell( int32_t s, Ts... ss )
  { remove_spell( s ); return remove_spell( ss... ); }
};

// local aliases
namespace
{
using parse_cb_t = std::function<void( parse_callback_e )>;
template <typename T> using detect_simple = decltype( T::simple );
template <typename T> using detect_buff = decltype( T::buff );
template <typename T> using detect_func = decltype( T::func );
template <typename T> using detect_value_func = decltype( T::value_func );
template <typename T> using detect_use_stacks = decltype( T::use_stacks );
template <typename T> using detect_type = decltype( T::type );
template <typename T> using detect_value = decltype( T::value );
template <typename T> using detect_idx = decltype( T::idx );
}

// used to store values from parameter pack recursion of parse_effect/parse_target_effects
template <typename U, typename = std::enable_if_t<std::is_default_constructible_v<U>>>
struct pack_t
{
  U data;
  const spell_data_t* spell;  // uninitalized
  std::vector<const spell_data_t*> list;
  uint32_t mask = 0U;
  std::vector<U>* copy = nullptr;
  std::vector<affect_list_t> affect_lists;
  parse_cb_t callback = nullptr;

  pack_t( const spell_data_t* s_data ) : spell( s_data ) {}

  pack_t( buff_t* buff )
  {
    if ( buff )
    {
      spell = &buff->data();

      if constexpr ( is_detected_v<detect_buff, U> )
      {
        data.buff = buff;
      }
    }
    else
    {
      spell = nullptr;
    }
  }
};

template <typename U>
static inline bool has_parse_entry( std::vector<U>& vec, const spelleffect_data_t* eff )
{ return !eff->ok() || range::contains( vec, eff, &U::eff ); }

template <typename U>
static inline U& add_parse_entry( std::vector<U>& vec )
{ U& tmp = vec.emplace_back(); tmp.type = MANUAL_ENTRY; return tmp; }

// input interface framework
struct parse_base_t
{
  parse_base_t() = default;
  virtual ~parse_base_t() = default;

  // returns the value of the effect. overload if different value access method is needed for other spell_data_t*
  // castable types, such as conduits
  double mod_spell_effects_value( const spell_data_t*, const spelleffect_data_t& e ) { return e.base_value(); }

  template <typename T>
  void apply_affecting_mod( double&, bool&, const spell_data_t*, size_t, T );

  template <typename U>
  void apply_affecting_mods( const pack_t<U>& pack, double& val, bool& mastery, size_t idx )
  {
    // Apply effect modifying effects from mod list. Blizz only currently supports modifying effects 1-5
    if ( idx > 5 )
      return;

    for ( size_t j = 0; j < pack.list.size(); j++ )
      apply_affecting_mod( val, mastery, pack.spell, idx, pack.list[ j ] );
  }

  virtual void parse_callback_function( pack_t<player_effect_t>&, parse_cb_t )
  { assert( false && "cannot register parse callback on this base" ); }
  virtual void parse_callback_function( pack_t<player_effect_t>&, parse_flag_e )
  { assert( false && "cannot register parse callback on this base" ); }
  virtual void register_callback_function( pack_t<player_effect_t>& )
  { assert( false && "cannot register parse callback on this base" ); }

  // populate pack with optional arguments to parse_effects().
  // parsing order of precedence is:
  // 1) spell_data_t* added to list of spells to check for effect modifying effects
  // 2) bool F() functor to set condition for effect to apply.
  // 3) parse flags such as IGNORE_STACKS, USE_DEFAULT, USE_CURRENT
  // 4) floating point value to directly set the effect value and override all parsing
  // 5) integral bitmask to ignore effect# n corresponding to the n'th bit
  template <typename U, typename T>
  void parse_spell_effect_mod( pack_t<U>& pack, T mod )
  {
    if constexpr ( std::is_invocable_v<decltype( &spell_data_t::ok ), T> )
    {
      pack.list.push_back( mod );
    }
    else if constexpr ( std::is_convertible_v<T, std::function<double( double )>> &&
                        is_detected_v<detect_value_func, U> )
    {
      pack.data.value_func = std::move( mod );
    }
    else if constexpr ( ( std::is_convertible_v<T, std::function<bool()>> ||
                          std::is_convertible_v<T, std::function<bool( const action_t*, const action_state_t* )>> ) &&
                        is_detected_v<detect_func, U> )
    {
      pack.data.func = std::move( mod );
    }
    else if constexpr ( std::is_convertible_v<T, parse_cb_t> && std::is_same_v<U, player_effect_t> )
    {
      parse_callback_function( pack, std::move( mod ) );
    }
    else if constexpr ( std::is_same_v<T, parse_flag_e> )
    {
      if constexpr ( is_detected_v<detect_use_stacks, U> )
      {
        if ( mod == IGNORE_STACKS )
        {
          pack.data.use_stacks = false;
          return;
        }
      }

      if constexpr ( is_detected_v<detect_type, U> )
      {
        if ( ( mod == USE_DEFAULT || mod == USE_CURRENT ) && !( pack.data.type & VALUE_OVERRIDE ) )
        {
          pack.data.type &= ~( USE_DEFAULT | USE_CURRENT );
          pack.data.type |= mod;
          return;
        }
      }

      if constexpr ( std::is_same_v<U, player_effect_t> )
      {
        if ( mod == EXPIRE_BUFF || mod == DECREMENT_BUFF )
        {
          parse_callback_function( pack, mod );
          return;
        }
      }
    }
    else if constexpr ( std::is_floating_point_v<T> && is_detected_v<detect_value, U> )
    {
      pack.data.value = mod;

      if constexpr ( is_detected_v<detect_type, U> )
      {
        pack.data.type &= ~( USE_DEFAULT | USE_CURRENT );
        pack.data.type |= VALUE_OVERRIDE;
      }
    }
    else if constexpr ( std::is_same_v<T, effect_mask_t> || ( std::is_integral_v<T> && !std::is_same_v<T, bool> ) )
    {
      pack.mask = mod;
    }
    else if constexpr ( std::is_same_v<T, affect_list_t> )
    {
      pack.affect_lists.push_back( std::move( mod ) );
    }
    else if constexpr ( std::is_convertible_v<decltype( *std::declval<T>() ), const std::vector<U>> )
    {
      pack.copy = &( *mod );
    }
    else
    {
      static_assert( static_false<T>, "Invalid mod type for parse_spell_effect_mods" );
    }
  }

  template <typename U, typename... Ts>
  void parse_spell_effect_mods( pack_t<U>& pack, Ts... mods )
  {
    ( parse_spell_effect_mod( pack, mods ), ... );
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
  double base_value( const action_t* = nullptr, const action_state_t* = nullptr ) const;

  double percent( const action_t* a = nullptr, const action_state_t* s = nullptr ) const
  { return base_value( a, s ) * 0.01; }

  double resource( resource_e r, const action_t* a = nullptr, const action_state_t* s = nullptr ) const
  { return base_value( a, s ) * _eff.resource_multiplier( r ); }

  double resource( const action_t* a = nullptr, const action_state_t* s = nullptr ) const
  { return resource( _eff.resource_gain_type(), a, s ); }

  timespan_t time_value( const action_t* a = nullptr, const action_state_t* s = nullptr ) const
  { return timespan_t::from_millis( base_value( a, s ) ); }

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

    pack_t<modify_effect_t> pack( data );

    if ( !pack.spell || !pack.spell->ok() )
      return this;

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    for ( size_t i = 1; i <= pack.spell->effect_count(); i++ )
    {
      if ( pack.mask & 1 << ( i - 1 ) )
        continue;

      parse_effect( pack, i );
    }

    return this;
  }

  void parse_effect( const pack_t<modify_effect_t>&, size_t );

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
  std::vector<parse_cb_t> callback_list;
  mutable uint32_t callback_idx = 0;

public:
  parse_effects_t( player_t* p ) : _player( p ) {}

  template <typename U>
  bool parse_effect( pack_t<U>&, size_t, bool );

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
    pack_t<player_effect_t> pack( data );

    if ( !pack.spell || !pack.spell->ok() )
      return;

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    bool has_entry = false;

    for ( size_t i = 1; i <= pack.spell->effect_count(); i++ )
    {
      if ( pack.mask & 1 << ( i - 1 ) )
        continue;

      has_entry = parse_effect( pack, i, false ) || has_entry;
    }

    if ( has_entry && pack.callback )
      register_callback_function( pack );
  }

  template <typename T, typename... Ts>
  void force_effect( T data, unsigned idx, Ts... mods )
  {
    pack_t<player_effect_t> pack( data );

    if ( !pack.spell || !pack.spell->ok() || !can_force( pack.spell->effectN( idx ) ) )
      return;

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    if ( parse_effect( pack, idx, true ) && pack.callback )
      register_callback_function( pack );
  }

  // Syntax: parse_target_effects( func, debuff[, spells|ignore_mask][,...] )
  //   (int F(TD*))            func: Function taking the target_data as argument and returning an integer mutiplier
  //   (const spell_data_t*) debuff: Spell data of the debuff
  //
  // The following optional arguments can be used in any order:
  //   (const spell_data_t*) spells: List of spells with redirect effects that modify the effects on the debuff
  //   (unsigned)       ignore_mask: Bitmask to skip effect# n corresponding to the n'th bit
  template <typename... Ts>
  void parse_target_effects( const std::function<double( actor_target_data_t* )>& fn, const spell_data_t* spell,
                             Ts... mods )
  {
    if ( !spell || !spell->ok() )
      return;

    pack_t<target_effect_t> pack( spell );
    pack.data.func = std::move( fn );

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    for ( size_t i = 1; i <= pack.spell->effect_count(); i++ )
    {
      if ( pack.mask & 1 << ( i - 1 ) )
        continue;

      parse_effect( pack, i, false );
    }
  }

  template <typename... Ts>
  void force_target_effect( const std::function<double( actor_target_data_t* )>& fn, const spell_data_t* spell,
                            unsigned idx, Ts... mods )
  {
    if ( !spell || !spell->ok() || !can_force( spell->effectN( idx ) ) )
      return;

    pack_t<target_effect_t> pack( spell );
    pack.data.func = std::move( fn );

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    parse_effect( pack, idx, true );
  }

  virtual bool is_valid_aura( const spelleffect_data_t& ) const { return false; }
  virtual bool is_valid_target_aura( const spelleffect_data_t& ) const { return false; }

  virtual std::vector<player_effect_t>* get_effect_vector( const spelleffect_data_t& eff, player_effect_t& tmp,
                                                           double& val_mul, std::string& str, bool& flat, bool force,
                                                           const pack_t<player_effect_t>& pack ) = 0;
  virtual std::vector<target_effect_t>* get_effect_vector( const spelleffect_data_t& eff, target_effect_t& tmp,
                                                           double& val_mul, std::string& str, bool& flat, bool force,
                                                           const pack_t<target_effect_t>& pack ) = 0;

  virtual void debug_message( const player_effect_t& data, std::string_view type_str, std::string_view val_str,
                              bool mastery, const spell_data_t* s_data, size_t i ) = 0;
  virtual void debug_message( const target_effect_t& /* data */, std::string_view type_str, std::string_view val_str,
                              bool /* mastery */, const spell_data_t* s_data, size_t i ) = 0;

  double get_effect_value( const player_effect_t&, bool benefit = false ) const;
  double get_effect_value_full( const player_effect_t&, bool benefit ) const;
  double get_effect_value( const target_effect_t&, actor_target_data_t* ) const;

  virtual bool can_force( const spelleffect_data_t& ) const { return true; }
};

struct parse_player_effects_t : public player_t, public parse_effects_t
{
  std::vector<player_effect_t> auto_attack_speed_effects;
  std::vector<player_effect_t> attribute_multiplier_effects;
  std::vector<player_effect_t> matching_armor_attribute_multiplier_effects;
  std::vector<player_effect_t> rating_multiplier_effects;
  std::vector<player_effect_t> versatility_effects;
  std::vector<player_effect_t> player_multiplier_effects;
  std::vector<player_effect_t> pet_multiplier_effects;
  std::vector<player_effect_t> attack_power_multiplier_effects;
  std::vector<player_effect_t> crit_chance_effects;
  std::vector<player_effect_t> leech_effects;
  std::vector<player_effect_t> expertise_effects;
  std::vector<player_effect_t> crit_avoidance_effects;
  std::vector<player_effect_t> parry_effects;
  std::vector<player_effect_t> base_armor_multiplier_effects;
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
  double composite_rating_multiplier( rating_e ) const override;
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
  double composite_base_armor_multiplier() const override;
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
  bool is_valid_target_aura( const spelleffect_data_t& ) const override;

  std::vector<player_effect_t>* get_effect_vector( const spelleffect_data_t&, player_effect_t&, double&, std::string&,
                                                   bool&, bool, const pack_t<player_effect_t>& ) override;
  std::vector<target_effect_t>* get_effect_vector( const spelleffect_data_t&, target_effect_t&, double&, std::string&,
                                                   bool&, bool, const pack_t<target_effect_t>& ) override;

  void debug_message( const player_effect_t&, std::string_view, std::string_view, bool, const spell_data_t*,
                      size_t ) override;
  void debug_message( const target_effect_t&, std::string_view, std::string_view, bool, const spell_data_t*,
                      size_t ) override;

  void parsed_effects_html( report::sc_html_stream& );

  virtual size_t total_effects_count();

  virtual void print_parsed_custom_type( report::sc_html_stream& ) {}

  template <typename U>
  void print_parsed_type( report::sc_html_stream& os, const std::vector<U>& entries, std::string_view n,
                          const std::function<std::string( uint32_t )>& note_fn = nullptr,
                          const std::function<std::string( double )>& val_str_fn = nullptr )
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

struct parse_action_base_t : public parse_effects_t
{
  std::vector<player_effect_t> ta_multiplier_effects;
  std::vector<player_effect_t> da_multiplier_effects;
  std::vector<player_effect_t> execute_time_effects;
  std::vector<player_effect_t> flat_execute_time_effects;
  std::vector<player_effect_t> gcd_effects;
  std::vector<player_effect_t> dot_duration_effects;
  std::vector<player_effect_t> flat_dot_duration_effects;
  std::vector<player_effect_t> tick_time_effects;
  std::vector<player_effect_t> flat_tick_time_effects;
  std::vector<player_effect_t> recharge_multiplier_effects;
  std::vector<player_effect_t> recharge_rate_effects;
  std::vector<player_effect_t> cost_effects;
  std::vector<player_effect_t> flat_cost_effects;
  std::vector<player_effect_t> crit_chance_effects;
  std::vector<player_effect_t> crit_chance_multiplier_effects;
  std::vector<player_effect_t> crit_bonus_effects;
  std::vector<player_effect_t> spell_school_effects;
  std::vector<target_effect_t> target_multiplier_effects;
  std::vector<target_effect_t> target_crit_chance_effects;
  std::vector<target_effect_t> target_crit_bonus_effects;

private:
  action_t* _action;

public:
  parse_action_base_t( player_t* p, action_t* a ) : parse_effects_t( p ), _action( a ) {}

  void parse_callback_function( pack_t<player_effect_t>& pack, parse_cb_t cb ) override;
  void parse_callback_function( pack_t<player_effect_t>& pack, parse_flag_e type ) override;
  void register_callback_function( pack_t<player_effect_t>& pack ) override;

  void trigger_callbacks( parse_callback_e );

  bool is_valid_aura( const spelleffect_data_t& ) const override;
  bool is_valid_target_aura( const spelleffect_data_t& ) const override;

  std::vector<player_effect_t>* get_effect_vector( const spelleffect_data_t&, player_effect_t&, double&, std::string&,
                                                   bool&, bool, const pack_t<player_effect_t>& ) override;
  std::vector<target_effect_t>* get_effect_vector( const spelleffect_data_t&, target_effect_t&, double&, std::string&,
                                                   bool&, bool, const pack_t<target_effect_t>& ) override;

  void debug_message( const player_effect_t&, std::string_view, std::string_view, bool, const spell_data_t*,
                      size_t ) override;
  void debug_message( const target_effect_t&, std::string_view, std::string_view, bool, const spell_data_t*,
                      size_t ) override;

  bool can_force( const spelleffect_data_t& ) const override;

  bool check_affected_list( const std::vector<affect_list_t>&, const spelleffect_data_t&, bool& );

  void parsed_effects_html( report::sc_html_stream& );

  virtual void print_parsed_custom_type( report::sc_html_stream& ) {}

  virtual size_t total_effects_count();

  template <typename W = parse_action_base_t, typename V>
  void print_parsed_type( report::sc_html_stream& os, V vector_ptr, std::string_view n,
                          const std::function<std::string( uint32_t )>& note_fn = nullptr,
                          const std::function<std::string( double )>& val_str_fn = nullptr )
  {
    auto _this = dynamic_cast<W*>( _action );
    assert( _this );
    auto entries = std::invoke( vector_ptr, _this );

    // assuming the stats obj being processed isn't orphaned (as can happen in debug=1 with child actions with stats
    // replaced by parent's stats), go through all the actions assigned to the stats obj and populate with all unique
    // entries
    if ( range::contains( _action->stats->action_list, _action ) )
      for ( auto a : _action->stats->action_list )
        if ( auto tmp = dynamic_cast<W*>( a ); tmp && a != _action )
          for ( const auto& entry : std::invoke( vector_ptr, tmp ) )
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

      entries[ i ].print_parsed_line( os, *_action->sim, false, note_fn, val_str_fn );
    }
  }
};

template <typename BASE>
struct parse_action_effects_t : public BASE, public parse_action_base_t
{
private:
  std::vector<buff_t*> _cd_buffs;  // buffs that affect the cooldown of this action

public:
  parse_action_effects_t( std::string_view name, player_t* player, const spell_data_t* spell )
    : BASE( name, player, spell ), parse_action_base_t( player, this )
  {}

  void initialize_cooldown_buffs()
  {
    if ( !BASE::cooldown )
      return;

    bool dynamic = range::contains( BASE::player->dynamic_cooldown_list, BASE::cooldown );

    auto vec = recharge_multiplier_effects;  // make a copy
    vec.insert( vec.end(), recharge_rate_effects.begin(), recharge_rate_effects.end() );

    for ( const auto& i : vec )
    {
      if ( i.buff && ( i.buff->gcd_type == gcd_haste_type::NONE || !dynamic ) &&
           !range::contains( _cd_buffs, i.buff ) )
      {
        BASE::sim->print_debug( "action-effects: cooldown {} recharge multiplier adjusted by buff {} ({})",
                                BASE::cooldown->name(), i.buff->name(), i.buff->data().id() );

        if ( BASE::internal_cooldown->charges )
        {
          i.buff->add_stack_change_callback( [ this ]( buff_t*, int, int ) {
            BASE::cooldown->adjust_recharge_multiplier();
            BASE::internal_cooldown->adjust_recharge_multiplier();
          } );
        }
        else
        {
          i.buff->add_stack_change_callback( [ this ]( buff_t*, int, int ) {
            BASE::cooldown->adjust_recharge_multiplier();
          } );
        }

        // actions do not necessarily have unique cooldowns, so we need to go through every action and check to see if
        // the cooldown matches, then add the buff to _cd_buffs so that we don't add duplicate stack_change_callbacks.
        for ( auto a : BASE::player->action_list )
          if ( a->cooldown == BASE::cooldown )
            if ( auto tmp = dynamic_cast<parse_action_effects_t<BASE>*>( a ) )
              tmp->_cd_buffs.push_back( i.buff );
      }
    }
  }

  void init_finished() override
  {
    BASE::init_finished();
    initialize_cooldown_buffs();
  }

  void impact( action_state_t* s ) override
  {
    trigger_callbacks( PARSE_CALLBACK_PRE_IMPACT );
    BASE::impact( s );
    trigger_callbacks( PARSE_CALLBACK_POST_IMPACT );
  }


  void execute() override
  {
    BASE::execute();
    trigger_callbacks( PARSE_CALLBACK_POST_EXECUTE );
    callback_idx = 0;
  }

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

    for ( const auto& i : crit_bonus_effects )
      cd *= 1.0 + get_effect_value( i, true );

    return cd;
  }

  double execute_time_pct_multiplier() const override
  {
    auto mul = BASE::execute_time_pct_multiplier();

    for ( const auto& i : execute_time_effects )
      mul *= 1.0 + get_effect_value( i, true );

    return mul;
  }

  timespan_t execute_time_flat_modifier() const override
  {
    double add = 0.0;

    for ( const auto& i : flat_execute_time_effects )
      add += get_effect_value( i, true );

    return BASE::execute_time_flat_modifier() + timespan_t::from_millis( add );
  }

  double dot_duration_pct_multiplier( const action_state_t* s ) const override
  {
    auto mul = BASE::dot_duration_pct_multiplier( s );

    for ( const auto& i : dot_duration_effects )
      mul *= 1.0 + get_effect_value( i );

    return mul;
  }

  timespan_t dot_duration_flat_modifier( const action_state_t* s ) const override
  {
    double add = 0.0;

    for ( const auto& i : flat_dot_duration_effects )
      add += get_effect_value( i );

    return BASE::dot_duration_flat_modifier( s ) + timespan_t::from_millis( add );
  }

  timespan_t gcd() const override
  {
    auto g = BASE::gcd();
    if ( g <= 0_ms )
      return 0_ms;

    for ( const auto& i : gcd_effects )
      g *= 1.0 + get_effect_value( i );

    return std::max( BASE::min_gcd, g );
  }

  double tick_time_pct_multiplier( const action_state_t* s ) const override
  {
    auto mul = BASE::tick_time_pct_multiplier( s );

    for ( const auto& i : tick_time_effects )
      mul *= 1.0 + get_effect_value( i );

    return mul;
  }

  timespan_t tick_time_flat_modifier( const action_state_t* s ) const override
  {
    double add = 0.0;

    for ( const auto& i : flat_tick_time_effects )
      add += get_effect_value( i );

    return BASE::tick_time_flat_modifier( s ) + timespan_t::from_millis( add );
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    auto rm = BASE::recharge_multiplier( cd );

    for ( const auto& i : recharge_multiplier_effects )
      rm *= 1.0 + get_effect_value( i );

    return rm;
  }

  double recharge_rate_multiplier( const cooldown_t& cd ) const override
  {
    auto rm = BASE::recharge_rate_multiplier( cd );

    for ( const auto& i : recharge_rate_effects )
      rm /= 1.0 + get_effect_value( i );

    return rm;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    auto tm = BASE::composite_target_multiplier( t );
    auto td = _player->get_target_data( t );

    for ( const auto& i : target_multiplier_effects )
      tm *= 1.0 + get_effect_value( i, td );

    return tm;
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    auto cc = BASE::composite_target_crit_chance( t );
    auto td = _player->get_target_data( t );

    for ( const auto& i : target_crit_chance_effects )
      cc += get_effect_value( i, td );

    return cc;
  }

  double composite_target_crit_damage_bonus_multiplier( player_t* t ) const override
  {
    auto cd = BASE::composite_target_crit_damage_bonus_multiplier( t );
    auto td = _player->get_target_data( t );

    for ( const auto& i : target_crit_bonus_effects )
      cd *= 1.0 + get_effect_value( i, td );

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
};
