// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "action/action.hpp"
#include "buff/buff.hpp"
#include "player/player.hpp"
#include "player/stats.hpp"
#include "sim/cooldown.hpp"
#include "util/io.hpp"

#include <string_view>

// forward declarations
struct parse_effects_t;

// local aliases
namespace
{
using parse_cb_t = std::function<void( action_state_t* )>;
template <typename T> using detect_simple = decltype( T::simple );
template <typename T> using detect_buff = decltype( T::buff );
template <typename T> using detect_func = decltype( T::func );
template <typename T> using detect_value_func = decltype( T::value_func );
template <typename T> using detect_use_stacks = decltype( T::use_stacks );
template <typename T> using detect_type = decltype( T::type );
template <typename T> using detect_value = decltype( T::value );
template <typename T> using detect_idx = decltype( T::idx );
}

enum parse_flag_e : uint16_t
{
  USE_DATA          = 0x0000,
  USE_DEFAULT       = 0x0001,
  USE_CURRENT       = 0x0002,
  IGNORE_STACKS     = 0x0004,
  ALLOW_ZERO        = 0x0008,
  CONSUME_BUFF      = 0x0010,
  ROUND_VALUE       = 0x0020,  // uses std::round (round to nearest integer, round half away from zero)
  IGNORE_WHITELIST  = 0x0040,
  PARSE_PASSIVE     = 0x0080,  // force parsing of passive effects
  // internal flags that should not be used in parse_effects()
  VALUE_OVERRIDE    = 0x1000,
  AFFECTED_OVERRIDE = 0x2000,
  MANUAL_ENTRY      = 0x4000,
  VALUE_FUNCTION    = 0x8000
};

enum parse_callback_e
{
  PARSE_CALLBACK_POST_EXECUTE,
  PARSE_CALLBACK_POST_IMPACT,
  PARSE_CALLBACK_POST_SNAPSHOT,
  PARSE_CALLBACK_MAX
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
  double base_mastery = 0.0;
  uint32_t idx = 0;  // index of parse_action_base_t::callback_list
  // effect linkback
  const spelleffect_data_t* eff = &spelleffect_data_t::nil();
  // optional enum identifier
  uint32_t opt_enum = UINT32_MAX;
  // note for html report
  std::string_view note;

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

  player_effect_t& set_base_mastery( double v )
  { base_mastery = v; simple = false; return *this; }

  player_effect_t& set_idx( uint32_t i )
  { idx = i; simple = false; return *this; }

  player_effect_t& add_parse_callback( parse_effects_t*, parse_callback_e, parse_cb_t );

  player_effect_t& set_eff( const spelleffect_data_t* e )
  { eff = e; return *this; }

  player_effect_t& set_opt_enum( uint32_t o )
  { opt_enum = o; return *this; }

  player_effect_t& set_note( std::string_view n )
  { note = n; return *this; }

  player_effect_t& print_debug( sim_t*, std::string );
  player_effect_t& print_debug( action_t* );
  player_effect_t& print_debug( player_t* );

  bool operator==( const player_effect_t& other )
  {
    return simple == other.simple && buff == other.buff && value == other.value && use_stacks == other.use_stacks &&
           type == other.type && mastery == other.mastery && base_mastery == other.base_mastery && idx == other.idx &&
           eff == other.eff && opt_enum == other.opt_enum;
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
  double base_mastery = 0.0;
  const spelleffect_data_t* eff = &spelleffect_data_t::nil();
  uint32_t opt_enum = UINT32_MAX;
  // note for html report
  std::string_view note;

  target_effect_t& set_func( std::function<double( actor_target_data_t* )> f )
  { func = std::move( f ); return *this; }

  target_effect_t& set_value( double v )
  { value = v; return *this; }

  target_effect_t& set_mastery( bool m )
  { mastery = m; return *this; }

  target_effect_t& set_base_mastery( double v )
  { base_mastery = v; return *this; }

  target_effect_t& set_eff( const spelleffect_data_t* e )
  { eff = e; return *this; }

  target_effect_t& set_opt_enum( uint32_t o )
  { opt_enum = o; return *this; }

  target_effect_t& set_note( std::string_view n )
  { note = n; return *this; }

  bool operator==( const target_effect_t& other )
  {
    return value == other.value && mastery == other.mastery && base_mastery == other.base_mastery && eff == other.eff &&
           opt_enum == other.opt_enum;
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
  // note for html report
  std::string_view note;

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

  modify_effect_t& set_note( std::string_view n )
  { note = n; return *this; }
};

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
  std::array<parse_cb_t, PARSE_CALLBACK_MAX> callback;
  parse_callback_e callback_type = PARSE_CALLBACK_POST_EXECUTE;
  bool ignore_whitelist = false;

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

  size_t num_callbacks() const
  {
    return callback.size() - std::count( callback.begin(), callback.end(), nullptr );
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
      pack.data.type |= VALUE_FUNCTION;
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
    else if constexpr ( std::is_same_v<T, parse_callback_e> )
    {
      pack.callback_type = mod;
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

        if ( mod == ROUND_VALUE || mod == PARSE_PASSIVE )
        {
          pack.data.type |= mod;
          return;
        }
      }

      if constexpr ( std::is_same_v<U, player_effect_t> )
      {
        if ( mod == CONSUME_BUFF )
        {
          parse_callback_function( pack, mod );
          return;
        }
      }

      if ( mod == IGNORE_WHITELIST )
      {
        pack.ignore_whitelist = true;
        return;
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
    else if constexpr ( std::is_constructible_v<std::string_view, std::decay_t<T>> )
    {
      pack.data.note = mod;
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
  {
  #if SC_USE_PTR == 1
    assert( power_type_data_t::multiplier( r, false ) ==
            power_type_data_t::multiplier( r, true ) );
  #endif
    return base_value( a, s ) * power_type_data_t::multiplier( r );
  }

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
  // Internal player pointer used to access target data and mastery, can differ from the player of the action
  player_t* _player;
  std::array<std::vector<parse_cb_t>, PARSE_CALLBACK_MAX> callback_list;
  std::array<uint32_t, PARSE_CALLBACK_MAX> callback_mask{};
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
  bool parse_effects( T data, Ts... mods )
  {
    pack_t<player_effect_t> pack( data );

    if ( !pack.spell || !pack.spell->ok() )
      return false;

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    bool has_entry = false;

    for ( size_t i = 1; i <= pack.spell->effect_count(); i++ )
    {
      if ( pack.mask & 1 << ( i - 1 ) )
        continue;

      has_entry = parse_effect( pack, i, false ) || has_entry;
    }

    if ( has_entry && pack.num_callbacks() )
      register_callback_function( pack );

    return has_entry;
  }

  template <typename T, typename... Ts>
  bool force_effect( T data, unsigned idx, Ts... mods )
  {
    pack_t<player_effect_t> pack( data );

    if ( !pack.spell || !pack.spell->ok() || !can_force( pack.spell->effectN( idx ) ) )
      return false;

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    bool has_entry = parse_effect( pack, idx, true );

    if ( has_entry && pack.num_callbacks() )
      register_callback_function( pack );

    return has_entry;
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

  // These methods are performance sensitive, make sure you profile when changing them.
  double get_effect_value( const player_effect_t&, bool benefit = false ) const;
  double get_effect_value_full( const player_effect_t&, bool benefit ) const;
  double get_effect_value( const target_effect_t&, actor_target_data_t* ) const;

  // virtual methods
  virtual bool is_valid_aura( const spelleffect_data_t& ) const { return false; }
  virtual bool is_valid_target_aura( const spelleffect_data_t& ) const { return false; }

  virtual std::vector<player_effect_t>* get_effect_vector( const spelleffect_data_t& eff, player_effect_t& tmp,
                                                           double& val_mul, std::string& str, bool& flat, bool force,
                                                           const pack_t<player_effect_t>& pack ) = 0;
  virtual std::vector<target_effect_t>* get_effect_vector( const spelleffect_data_t& eff, target_effect_t& tmp,
                                                           double& val_mul, std::string& str, bool& flat, bool force,
                                                           const pack_t<target_effect_t>& pack ) = 0;

  virtual void debug_message( const player_effect_t& data, std::string_view type_str, std::string_view val_str,
                              const spelleffect_data_t& eff ) = 0;
  virtual void debug_message( const target_effect_t& /* data */, std::string_view type_str, std::string_view val_str,
                              const spelleffect_data_t& eff ) = 0;

  virtual void throw_passive_error( const spell_data_t* s ) = 0;

  virtual bool can_force( const spelleffect_data_t& ) const { return true; }

  friend player_effect_t& player_effect_t::add_parse_callback( parse_effects_t*, parse_callback_e, parse_cb_t );
};

struct parse_player_effects_t : public player_t, public parse_effects_t
{
  std::vector<player_effect_t> auto_attack_speed_effects;
  std::vector<player_effect_t> attribute_multiplier_effects;
  std::vector<player_effect_t> rating_multiplier_effects;
  std::vector<player_effect_t> versatility_effects;
  std::vector<player_effect_t> player_multiplier_effects;
  std::vector<player_effect_t> pet_multiplier_effects;
  std::vector<player_effect_t> attack_power_multiplier_effects;
  std::vector<player_effect_t> crit_chance_effects;
  std::vector<player_effect_t> crit_bonus_effects;
  std::vector<player_effect_t> spell_crit_chance_effects;
  std::vector<player_effect_t> leech_effects;
  std::vector<player_effect_t> expertise_effects;
  std::vector<player_effect_t> crit_avoidance_effects;
  std::vector<player_effect_t> parry_effects;
  std::vector<player_effect_t> base_armor_multiplier_effects;
  std::vector<player_effect_t> armor_multiplier_effects;
  std::vector<player_effect_t> haste_effects;
  std::vector<player_effect_t> melee_haste_effects;
  std::vector<player_effect_t> spell_haste_effects;
  std::vector<player_effect_t> mastery_effects;
  std::vector<player_effect_t> parry_rating_from_crit_effects;
  std::vector<player_effect_t> dodge_effects;
  std::vector<player_effect_t> mitigation_multiplier_effects;
  std::vector<target_effect_t> mitigation_from_target_multiplier_effects;
  std::vector<player_effect_t> absorb_multiplier_effects;
  std::vector<player_effect_t> absorb_received_mult_effects;
  std::vector<player_effect_t> healing_received_effects;
  std::vector<target_effect_t> target_multiplier_effects;
  std::vector<target_effect_t> target_pet_multiplier_effects;
  std::vector<player_effect_t> non_stacking_movement_effects;
  std::vector<player_effect_t> stacking_movement_effects;

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
  double composite_player_critical_damage_multiplier( const action_state_t*, school_e ) const override;
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
  double composite_player_absorb_multiplier( const action_state_t* ) const override;
  double composite_player_healing_received_multiplier() const override;
  double composite_player_absorb_received_multiplier() const override;
  double composite_player_target_multiplier( player_t*, school_e ) const override;
  double composite_player_target_pet_damage_multiplier( player_t*, bool ) const override;
  double composite_mitigation_multiplier( const action_state_t*, school_e, bool direct ) const override;
  double composite_mitigation_from_player_multiplier( player_t*, const action_state_t*, school_e,
                                                      bool direct ) const override;
  double non_stacking_movement_modifier() const override;
  double stacking_movement_modifier() const override;

  void invalidate_cache( cache_e c ) override;

  bool is_valid_aura( const spelleffect_data_t& ) const override;
  bool is_valid_target_aura( const spelleffect_data_t& ) const override;

  std::vector<player_effect_t>* get_effect_vector( const spelleffect_data_t&, player_effect_t&, double&, std::string&,
                                                   bool&, bool, const pack_t<player_effect_t>& ) override;
  std::vector<target_effect_t>* get_effect_vector( const spelleffect_data_t&, target_effect_t&, double&, std::string&,
                                                   bool&, bool, const pack_t<target_effect_t>& ) override;

  void debug_message( const player_effect_t&, std::string_view, std::string_view, const spelleffect_data_t& ) override;
  void debug_message( const target_effect_t&, std::string_view, std::string_view, const spelleffect_data_t& ) override;

  void throw_passive_error( const spell_data_t* s ) override;

  void print_custom_parsed_effects( report::sc_html_stream& ) const override;

  virtual size_t total_effects_count() const;

  virtual void print_parsed_custom_type( report::sc_html_stream& ) const {}

  template <typename U>
  void print_parsed_type( report::sc_html_stream& os, const std::vector<U>& entries, std::string_view n,
                          const std::function<std::string( uint32_t )>& note_fn = nullptr,
                          const std::function<std::string( double )>& val_str_fn = nullptr ) const
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
  // TODO: currently gcd is NOT split into flat vs percent effects via parsed_value_t, and only percent multipliers are
  // parsed. If flat gcd effects become more prevalent, they may need to be added to parsing.
  std::vector<player_effect_t> gcd_effects;
  // std::vector<player_effect_t> flat_gcd_effects;
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
  std::vector<buff_t*> _cd_buffs;  // buffs that affect the cooldown of this action
  action_t* _action;

public:
  parse_action_base_t( player_t* p, action_t* a ) : parse_effects_t( p ), _action( a ) {}

  void parse_callback_function( pack_t<player_effect_t>& pack, parse_cb_t cb ) override;
  void parse_callback_function( pack_t<player_effect_t>& pack, parse_flag_e type ) override;
  void register_callback_function( pack_t<player_effect_t>& pack ) override;

  void trigger_callbacks( parse_callback_e, action_state_t* );

  bool is_valid_aura( const spelleffect_data_t& ) const override;
  bool is_valid_target_aura( const spelleffect_data_t& ) const override;

  std::vector<player_effect_t>* get_effect_vector( const spelleffect_data_t&, player_effect_t&, double&, std::string&,
                                                   bool&, bool, const pack_t<player_effect_t>& ) override;
  std::vector<target_effect_t>* get_effect_vector( const spelleffect_data_t&, target_effect_t&, double&, std::string&,
                                                   bool&, bool, const pack_t<target_effect_t>& ) override;

  void debug_message( const player_effect_t&, std::string_view, std::string_view, const spelleffect_data_t& ) override;
  void debug_message( const target_effect_t&, std::string_view, std::string_view, const spelleffect_data_t& ) override;

  void throw_passive_error( const spell_data_t* s ) override;

  bool can_force( const spelleffect_data_t& ) const override;

  bool check_affected_list( const std::vector<affect_list_t>&, const spelleffect_data_t&, bool& );

  void process_cooldown_buffs( bool dynamic, std::vector<player_effect_t>& vec );
  void initialize_cooldown_buffs();

  void parsed_effects_html( report::sc_html_stream& ) const;

  virtual void print_parsed_custom_type( report::sc_html_stream& ) const {}

  virtual size_t total_effects_count() const;


  template <typename W = parse_action_base_t, typename V>
  void print_parsed_type( report::sc_html_stream& os, V vector_ptr, std::string_view n,
                          const std::function<std::string( uint32_t )>& note_fn = nullptr,
                          const std::function<std::string( double )>& val_str_fn = nullptr ) const
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
  parse_action_effects_t( std::string_view name, player_t* player, const spell_data_t* spell )
    : BASE( name, player, spell ), parse_action_base_t( player, this )
  {}

  template <typename U>
  void remove_damage_entries( std::vector<U>& vec, std::string_view vec_name )
  {
    for ( const auto& data : vec )
    {
      BASE::sim->print_debug( "action-effects: non-damage action {} removing {} entry from {}", *this, vec_name,
                              *data.eff );
    }

    vec.clear();
  }

  void init_finished() override
  {
    BASE::init_finished();

    // We do this in action_t::init_finished() instead of at parsing so that we can account for any damage values set in
    // the final derived constructor.
    if ( !BASE::does_direct_damage() && !BASE::does_periodic_damage() )
    {
      remove_damage_entries( ta_multiplier_effects, "tick damage" );
      remove_damage_entries( da_multiplier_effects, "direct damage" );
      remove_damage_entries( crit_bonus_effects, "crit bonus multiplier" );
      remove_damage_entries( target_multiplier_effects, "damage to target" );
    }

    initialize_cooldown_buffs();
  }

  void snapshot_internal( action_state_t* s, unsigned fl, result_amount_type rt ) override
  {
    // "fake snapshots" can happens to states created during expression evaluation, and since the action never executes
    // callback_idx is not cleared. as only post_snapshot callbacks can pollute callback_idx this way, we clear all
    // post_snapshot callbacks every snapshot.
    callback_idx &= ~callback_mask[ PARSE_CALLBACK_POST_SNAPSHOT ];
    BASE::snapshot_internal( s, fl, rt );
    if ( rt != result_amount_type::NONE )
      trigger_callbacks( PARSE_CALLBACK_POST_SNAPSHOT, s );
  }

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );
    trigger_callbacks( PARSE_CALLBACK_POST_IMPACT, s );
  }


  void execute() override
  {
    BASE::execute();
    trigger_callbacks( PARSE_CALLBACK_POST_EXECUTE, BASE::execute_state );
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

    if ( BASE::sim->debug )
    {
      for ( const auto& i : ta_multiplier_effects )
      {
        auto val = get_effect_value( i, true );
        BASE::sim->print_debug( "{} ta_multiplier_effects: {} from {}{}", *this, val, *i.eff,
                                i.func ? " (Conditional)" : "" );
        ta *= 1.0 + val;
      }
    }
    else
    {
      for ( const auto& i : ta_multiplier_effects )
        ta *= 1.0 + get_effect_value( i, true );
    }

    return ta;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    auto da = BASE::composite_da_multiplier( s );

    if ( BASE::sim->debug )
    {
      for ( const auto& i : da_multiplier_effects )
      {
        auto val = get_effect_value( i, true );
        BASE::sim->print_debug( "{} da_multiplier_effects: {} from {}{}", *this, val, *i.eff,
                                i.func ? " (Conditional)" : "" );
        da *= 1.0 + val;
      }
    }
    else
    {
      for ( const auto& i : da_multiplier_effects )
        da *= 1.0 + get_effect_value( i, true );
    }

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

    return g <= 0_ms ? 0_ms : std::max( BASE::min_gcd, g );
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

  void print_custom_parsed_effects( report::sc_html_stream& os ) const override
  {
    parsed_effects_html( os );
  }
};
