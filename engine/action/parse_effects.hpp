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
  IGNORE_STACKS,
  ALLOW_ZERO
};

static std::string value_type_name( parse_flag_e t )
{
  switch ( t )
  {
    case USE_DEFAULT: return "Default Value";
    case USE_CURRENT: return "Current Value";
    default:          return "Spell Data";
  }
}

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

  bool operator==( const player_effect_t&  other )
  {
    return buff == other.buff && value == other.value && type == other.type && use_stacks == other.use_stacks &&
           mastery == other.mastery && eff == other.eff && opt_enum == other.opt_enum;
  }

  void print_parsed_line( report::sc_html_stream& os, const sim_t& sim, bool decorate,
                          std::function<std::string( uint32_t )> note_fn,
                          std::function<std::string( double )> val_str_fn ) const
  {
    std::vector<std::string> notes;

    if ( !use_stacks )
      notes.emplace_back( "No-stacks" );

    if ( mastery )
      notes.emplace_back( "Mastery" );

    if ( func )
      notes.emplace_back( "Conditional" );

    if ( note_fn )
      notes.emplace_back( note_fn( opt_enum ) );

    range::for_each( notes, []( auto& s ) { s[ 0 ] = std::toupper( s[ 0 ] ); } );

    std::string val_str = val_str_fn ? val_str_fn( value )
                          : mastery  ? fmt::format( "{:.5f}", value * 100 )
                                     : fmt::format( "{:.1f}%", value * 100 );

    os.format(
      "<td class=\"left\">{}</td>"
      "<td class=\"right\">{}</td>"
      "<td class=\"right\">{}</td>"
      "<td class=\"right\">{}</td>"
      "<td>{}</td>"
      "<td>{}</td>""</tr>\n",
      decorate ? report_decorators::decorated_spell_data( sim, eff->spell() ) : eff->spell()->name_cstr(),
      eff->spell()->id(),
      eff->index() + 1,
      val_str,
      value_type_name( type ),
      util::string_join( notes ) );
  }
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
  uint32_t opt_enum = UINT32_MAX;

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

  bool operator==( const target_effect_t<TD>&  other )
  {
    return value == other.value && mastery == other.mastery && eff == other.eff && opt_enum == other.opt_enum;
  }

  void print_parsed_line( report::sc_html_stream& os, const sim_t& sim, bool decorate,
                          std::function<std::string( uint32_t )> note_fn,
                          std::function<std::string( double )> val_str_fn ) const
  {
    std::vector<std::string> notes;

    if ( mastery )
      notes.emplace_back( "Mastery" );

    if ( note_fn )
      notes.emplace_back( note_fn( opt_enum ) );

    range::for_each( notes, []( auto& s ) { s[ 0 ] = std::toupper( s[ 0 ] ); } );

    std::string val_str = val_str_fn ? val_str_fn( value )
                          : mastery  ? fmt::format( "{:.5f}", value * 100 )
                                     : fmt::format( "{:.1f}%", value * 100 );

    os.format(
      "<td class=\"left\">{}</td>"
      "<td class=\"right\">{}</td>"
      "<td class=\"right\">{}</td>"
      "<td class=\"right\">{}</td>"
      "<td>{}</td>"
      "<td>{}</td></tr>\n",
      decorate ? report_decorators::decorated_spell_data( sim, eff->spell() ) : eff->spell()->name_cstr(),
      eff->spell()->id(),
      eff->index() + 1,
      val_str,
      "",
      util::string_join( notes ) );
  }
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

// input interface framework
struct parse_base_t
{
  parse_base_t() = default;
  virtual ~parse_base_t() = default;

  template <typename U>
  U& add_parse_entry( std::vector<U>& vec )
  { return vec.emplace_back(); }

  // detectors for is_detected<>
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

  // returns the value of the effect. overload if different value access method is needed for other spell_data_t*
  // castable types, such as conduits
  double mod_spell_effects_value( const spell_data_t*, const spelleffect_data_t& e ) { return e.base_value(); }

  // adjust value based on effect modifying effects from mod list.
  // currently supports P_EFFECT_1-5 and A_PROC_TRIGGER_SPELL_WITH_VALUE
  // TODO: add support for P_EFFECTS to modify all effects
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
  double base_value() const
  {
    auto return_value = value;

    for ( const auto& i : conditional )
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

  void print_parsed_line( report::sc_html_stream& os, const sim_t& sim, const modify_effect_t& i ) const
  {
    std::string op_str = i.flat ? "ADD" : "PCT";
    std::string val_str = fmt::to_string( i.value );
    if ( !i.flat )
      val_str += '%';

    std::vector<std::string> notes;

    if ( !i.use_stacks )
      notes.emplace_back( "No-stacks" );

    if ( i.buff )
      notes.emplace_back( "w/ Buff" );

    if ( i.func )
      notes.emplace_back( "Conditional" );

    os.format(
      "<td>{}</td>"
      "<td class=\"right\">{}</td>"
      "<td class=\"right\">{}</td>"
      "<td>{}</td>"
      "<td class=\"right\">{}</td>"
      "<td>{}</td></tr>\n",
      report_decorators::decorated_spell_data( sim, i.eff->spell() ),
      i.eff->spell()->id(),
      i.eff->index() + 1,
      op_str,
      val_str,
      util::string_join( notes ) );
  }

  void print_parsed_line( report::sc_html_stream& os, const sim_t& sim, const spelleffect_data_t& eff ) const
  {
    std::string op_str;
    std::string val_str = fmt::to_string( eff.base_value() );

    switch ( eff.subtype() )
    {
      case A_ADD_PCT_MODIFIER:
      case A_ADD_PCT_LABEL_MODIFIER:
        op_str = "PCT";
        val_str += '%';
        break;
      case A_ADD_FLAT_MODIFIER:
      case A_ADD_FLAT_LABEL_MODIFIER:
        op_str = "ADD";
        break;
      default:
        op_str = "UNK";
        break;
    }

    os.format(
      "<td>{}</td>"
      "<td class=\"right\">{}</td>"
      "<td class=\"right\">{}</td>"
      "<td>{}</td>"
      "<td class=\"right\">{}</td>"
      "<td>Passive</td></tr>\n",
      report_decorators::decorated_spell_data( sim, eff.spell() ),
      eff.spell()->id(),
      eff.index() + 1,
      op_str,
      val_str );
  }

  void print_parsed_effect( report::sc_html_stream& os, const sim_t& sim, bool &first ) const
  {
    auto p = permanent.size();
    auto c = conditional.size();
    if ( p + c == 0 )
      return;

    os.format( "<td rowspan=\"{}\" style=\"background: #111\">#{}</td>\n", p + c, _eff.index() + 1 );

    for ( size_t i = 0; i < p; i++ )
    {
      if ( first )
        first = false;
      else
        os << "<tr>";

      print_parsed_line( os, sim, *permanent[ i ] );
    }

    for ( size_t i = 0; i < c; i++ )
    {
      if ( first )
        first = false;
      else
        os << "<tr>";

      print_parsed_line( os, sim, conditional[ i ] );
    }
  }

  static const modified_spelleffect_t& nil();
};

inline const modified_spelleffect_t modified_spelleffect_nil_v = modified_spelleffect_t();
inline const modified_spelleffect_t& modified_spelleffect_t::nil() { return modified_spelleffect_nil_v; }

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

  const modified_spelleffect_t& effectN( size_t idx ) const
  {
    assert( idx > 0 && "effect index must not be zero or less" );

    if ( idx > effects.size() )
      return modified_spelleffect_t::nil();

    return effects[ idx - 1 ];
  }

  operator const spell_data_t&() const
  { return _spell; }

  operator const spell_data_t*() const
  { return &_spell; }

  modify_effect_t& add_parse_entry( size_t idx )
  { return effects[ idx - 1 ].add_parse_entry(); }

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

  void parse_effect( pack_t<modify_effect_t>& tmp, const spell_data_t* s_data, size_t i )
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

    if ( !_spell.affected_by_all( eff ) )
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
    auto &effN = effects[ idx - 1 ];

    // always active
    if ( !tmp.data.buff && !tmp.data.func )
    {
      if ( flat )
        effN.value += val;
      else
        effN.value *= 1.0 + val;

      effN.permanent.push_back( &eff );
    }
    // conditionally active
    else
    {
      tmp.data.value = val;
      tmp.data.flat = flat;
      tmp.data.eff = &eff;

      effN.conditional.push_back( tmp.data );
    }
  }

  void print_parsed_spell( report::sc_html_stream& os, const sim_t& sim )
  {
    auto c = range::accumulate( effects, 0, &modified_spelleffect_t::size );
    if ( !c )
      return;

    bool first = true;

    os.format( "<tr><td rowspan=\"{}\" style=\"background: #111\">{}</td>\n", c,
               report_decorators::decorated_spell_data( sim, &_spell ) );

    for ( const auto& eff : effects )
      eff.print_parsed_effect( os, sim, first );
  }

  static void parsed_effects_html( report::sc_html_stream& os, const sim_t& sim,
                                   std::vector<modified_spell_data_t*> entries )
  {
    if ( entries.size() )
    {
      os << "<h3 class=\"toggle\">Modified Spell Data</h3>"
         << "<div class=\"toggle-content hide\">"
         << "<table class=\"sc even left\">\n";

      os << "<thead><tr>"
         << "<th colspan=\"2\">Spell</th>"
         << "<th>Modified By</th>"
         << "<th>ID</th>"
         << "<th>#</th>"
         << "<th>+/%</th>"
         << "<th>Value</th>"
         << "<th>Notes</th>"
         << "</tr></thead>\n";

      for ( auto m_data : entries )
        m_data->print_parsed_spell( os, sim );

      os << "</table>\n"
         << "</div>\n";
    }
  }

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

  // main effect parsing function with constexpr checks to handle player_effect_t vs target_effect_t
  template <typename U>
  void parse_effect( pack_t<U>& tmp, const spell_data_t* s_data, size_t i, bool force )
  {
    const auto& eff = s_data->effectN( i );
    bool mastery = s_data->flags( SX_MASTERY_AFFECTS_POINTS );
    double val = 0.0;
    double val_mul = 0.01;

    if ( mastery )
      val = eff.mastery_value();
    else
      val = eff.base_value();

    if constexpr ( is_detected<detect_buff, U>::value && is_detected<detect_type, U>::value )
    {
      if ( tmp.data.buff && tmp.data.type == USE_DEFAULT )
        val = tmp.data.buff->default_value * 100;

      if ( !is_valid_aura( eff ) )
        return;
    }
    else
    {
      if ( !is_valid_target_aura( eff ) )
        return;
    }

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

    std::string type_str;
    bool flat = false;
    std::vector<U>* vec;

    // NOTE: get_target_effect_vector is virtual and not templated, which means:
    // 1) only opt_enum reference is passed instead of the full data reference
    // 2) void* is returned and needs to be re-cast to the correct vector<U>*
    if constexpr ( is_detected<detect_buff, U>::value )
    {
      vec = get_effect_vector( eff, tmp.data, val_mul, type_str, flat, force );
    }
    else
    {
      vec = static_cast<std::vector<U>*>(
          get_target_effect_vector( eff, tmp.data.opt_enum, val_mul, type_str, flat, force ) );
    }

    if ( !vec )
      return;

    if constexpr ( is_detected<detect_type, U>::value )
    {
      if ( !val && tmp.data.type == parse_flag_e::USE_DATA )
        return;
    }
    else
    {
      if ( !val )
        return;
    }

    val *= val_mul;

    std::string val_str = mastery ? fmt::format( "{:.5f}*mastery", val * 100 )
                          : flat  ? fmt::format( "{}", val )
                                  : fmt::format( "{:.1f}%", val * ( 1 / val_mul ) );

    if ( tmp.data.value != 0.0 )
      val_str = val_str + " (overridden)";

    if constexpr ( is_detected<detect_buff, U>::value )
    {
      if ( tmp.data.buff )
      {
        if ( tmp.data.type == parse_flag_e::USE_CURRENT )
          val_str = flat ? "current value" : "current value percent";
        else if ( tmp.data.type == parse_flag_e::USE_DEFAULT )
          val_str = val_str + " (default value)";
      }

      debug_message( tmp.data, type_str, val_str, mastery, s_data, i );
    }
    else
    {
      target_debug_message( type_str, val_str, s_data, i );
    }

    tmp.data.value = val;
    tmp.data.mastery = mastery;
    tmp.data.eff = &eff;
    vec->push_back( tmp.data );

    if ( tmp.copy )
      tmp.copy->push_back( tmp.data );
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
  virtual bool is_valid_aura( const spelleffect_data_t& /* eff */ ) const { return false; }

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

  double get_effect_value( const player_effect_t& i, bool benefit = false ) const
  {
    if ( i.func && !i.func() )
      return 0.0;

    double eff_val = i.value;

    if ( i.buff )
    {
      auto stack = benefit ? i.buff->stack() : i.buff->check();
      if ( !stack )
        return 0.0;

      if ( i.type == USE_CURRENT )
        eff_val = i.buff->check_value();

      if ( i.use_stacks )
        eff_val *= stack;
    }

    if ( i.mastery )
      eff_val *= _player->cache.mastery();

    return eff_val;
  }

  // Syntax: parse_target_effects( func, debuff[, spells|ignore_mask][,...] )
  //   (int F(TD*))            func: Function taking the target_data as argument and returning an integer mutiplier
  //   (const spell_data_t*) debuff: Spell data of the debuff
  //
  // The following optional arguments can be used in any order:
  //   (const spell_data_t*) spells: List of spells with redirect effects that modify the effects on the debuff
  //   (unsigned)       ignore_mask: Bitmask to skip effect# n corresponding to the n'th bit
  virtual bool is_valid_target_aura( const spelleffect_data_t& /* eff */ ) const { return false; }

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

      parse_effect( tmp, spell, i, false );
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

    parse_effect( pack, spell, idx, true );
  }

  template <typename TD>
  double get_target_effect_value( const target_effect_t<TD>& i, TD* td ) const
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
  std::vector<target_effect_t<TD>> target_multiplier_effects;
  std::vector<target_effect_t<TD>> target_pet_multiplier_effects;

  // Cache Pairing, invalidate first of the pair when the second is invalidated
  std::vector<std::pair<cache_e, cache_e>> invalidate_with_parent;

  parse_player_effects_t( sim_t* sim, player_e type, std::string_view name, race_e race )
    : player_t( sim, type, name, race ), parse_effects_t( this )
  {}

  double composite_melee_auto_attack_speed() const override
  {
    auto ms = player_t::composite_melee_auto_attack_speed();

    for ( const auto& i : auto_attack_speed_effects )
      ms *= 1.0 / ( 1.0 + get_effect_value( i ) );

    return ms;
  }

  double composite_attribute_multiplier( attribute_e attr ) const override
  {
    auto am = player_t::composite_attribute_multiplier( attr );

    assert( attr != ATTRIBUTE_NONE && "ATTRIBUTE_NONE will be out of index" );

    for ( const auto& i : attribute_multiplier_effects )
      if ( i.opt_enum & ( 1 << ( attr - 1 ) ) )
        am *= 1.0 + get_effect_value( i );

    return am;
  }

  double composite_damage_versatility() const override
  {
    auto v = player_t::composite_damage_versatility();

    for ( const auto& i : versatility_effects )
      v += get_effect_value( i );

    return v;
  }

  double composite_heal_versatility() const override
  {
    auto v = player_t::composite_heal_versatility();

    for ( const auto& i : versatility_effects )
      v += get_effect_value( i );

    return v;
  }

  double composite_mitigation_versatility() const override
  {
    auto v = player_t::composite_mitigation_versatility();

    for ( const auto& i : versatility_effects )
      v += get_effect_value( i ) * 0.5;

    return v;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    auto m = player_t::composite_player_multiplier( school );

    for ( const auto& i : player_multiplier_effects )
      if ( i.opt_enum & dbc::get_school_mask( school ) )
        m *= 1.0 + get_effect_value( i, true );

    return m;
  }

  double composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const override
  {
    auto dm = player_t::composite_player_pet_damage_multiplier( s, guardian );

    for ( const auto& i : pet_multiplier_effects )
      if ( static_cast<bool>( i.opt_enum ) == guardian )
        dm *= 1.0 + get_effect_value( i, true );

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
      m += get_effect_value( i );

    return m;
  }

  double composite_parry_rating() const override
  {
    auto pr = player_t::composite_parry_rating();

    for ( const auto& i : parry_rating_from_crit_effects )
      pr += player_t::composite_melee_crit_rating() * get_effect_value( i );

    return pr;
  }

  double composite_dodge() const override
  {
    auto dodge = player_t::composite_dodge();

    for ( const auto& i : dodge_effects )
      dodge += get_effect_value( i );

    return dodge;
  }

  double matching_gear_multiplier( attribute_e attr ) const override
  {
    double mg = player_t::matching_gear_multiplier( attr );

    assert( attr != ATTRIBUTE_NONE && "ATTRIBUTE_NONE will be out of index" );

    for ( const auto& i : matching_armor_attribute_multiplier_effects )
      if ( i.opt_enum & ( 1 << ( attr - 1 ) ) )
        mg += get_effect_value( i );

    return mg;
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

  void invalidate_cache( cache_e c ) override
  {
    player_t::invalidate_cache( c );

    for ( const auto& i : invalidate_with_parent )
    {
      if ( c == i.second )
        player_t::invalidate_cache( i.first );
    }
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

  std::vector<player_effect_t>* get_effect_vector( const spelleffect_data_t& eff, player_effect_t& data, double& val_mul,
                                                   std::string& str, bool& /* flat */, bool /* force */ ) override
  {
    auto invalidate = [ &data ]( cache_e c ) {
      if ( data.buff )
        data.buff->add_invalidate( c );
    };

    switch ( eff.subtype() )
    {
      case A_MOD_MELEE_AUTO_ATTACK_SPEED:
      case A_MOD_RANGED_AND_MELEE_AUTO_ATTACK_SPEED:
        str = "auto attack speed";
        invalidate( CACHE_AUTO_ATTACK_SPEED );
        return &auto_attack_speed_effects;

      case A_MOD_TOTAL_STAT_PERCENTAGE:
        data.opt_enum = eff.misc_value2();
        {
          std::vector<std::string> str_list;
          for ( auto stat : { STAT_STRENGTH, STAT_AGILITY, STAT_STAMINA, STAT_INTELLECT, STAT_SPIRIT } )
          {
            if ( data.opt_enum & ( 1 << ( stat - 1 ) ) )
            {
              str_list.emplace_back( util::stat_type_string( stat ) );
              invalidate( cache_from_stat( stat ) );
            }
          }
          str = util::string_join( str_list );
        }

        if ( eff.spell()->equipped_class() == ITEM_CLASS_ARMOR && eff.spell()->flags( SX_REQUIRES_EQUIPPED_ARMOR_TYPE ) )
        {
          auto type_bit = 1U << static_cast<unsigned>( util::matching_armor_type( type ) );
          if ( eff.spell()->equipped_subclass_mask() == type_bit )
          {
            str += "|with matching armor";
            return &matching_armor_attribute_multiplier_effects;
          }
        }

        return &attribute_multiplier_effects;

      case A_MOD_VERSATILITY_PCT:
        str = "versatility";
        invalidate( CACHE_VERSATILITY );
        return &versatility_effects;

      case A_HASTE_ALL:
        str = "haste";
        invalidate( CACHE_HASTE );
        return &haste_effects;

      case A_MOD_MASTERY_PCT:
        str = "mastery";
        val_mul = 1.0;
        invalidate( CACHE_MASTERY );
        return &mastery_effects;

      case A_MOD_ALL_CRIT_CHANCE:
        str = "all crit chance";
        invalidate( CACHE_CRIT_CHANCE );
        return &crit_chance_effects;

      case A_MOD_DAMAGE_PERCENT_DONE:
        data.opt_enum = eff.misc_value1();
        str = data.opt_enum == 0x7f ? "all" : util::school_type_string( dbc::get_school_type( data.opt_enum ) );
        str += " damage";
        invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
        return &player_multiplier_effects;

      case A_MOD_PET_DAMAGE_DONE:
        data.opt_enum = 0;
        str = "pet damage";
        return &pet_multiplier_effects;

      case A_MOD_GUARDIAN_DAMAGE_DONE:
        data.opt_enum = 1;
        str = "guardian damage";
        return &pet_multiplier_effects;

      case A_MOD_ATTACK_POWER_PCT:
        str = "attack power";
        invalidate( CACHE_ATTACK_POWER );
        return &attack_power_multiplier_effects;

      case A_MOD_LEECH_PERCENT:
        str = "leech";
        invalidate( CACHE_LEECH );
        return &leech_effects;

      case A_MOD_EXPERTISE:
        str = "expertise";
        invalidate( CACHE_EXP );
        return &expertise_effects;

      case A_MOD_ATTACKER_MELEE_CRIT_CHANCE:
        str = "crit avoidance";
        invalidate( CACHE_CRIT_AVOIDANCE );
        return &crit_avoidance_effects;

      case A_MOD_PARRY_PERCENT:
        str = "parry";
        invalidate( CACHE_PARRY );
        return &parry_effects;

      case A_MOD_RESISTANCE_PCT:
      case A_MOD_BASE_RESISTANCE_PCT:
        str = "armor multiplier";
        invalidate( CACHE_ARMOR );
        return &armor_multiplier_effects;

      case A_MOD_PARRY_FROM_CRIT_RATING:
        str = "parry rating|of crit rating";
        invalidate_with_parent.emplace_back( CACHE_PARRY, CACHE_CRIT_CHANCE );
        return &parry_rating_from_crit_effects;

      case A_MOD_DODGE_PERCENT:
        str = "dodge";
        invalidate( CACHE_DODGE );
        return &dodge_effects;

      default:
        return nullptr;
    }

    return nullptr;
  }

  void debug_message( const player_effect_t& data, std::string_view type_str, std::string_view val_str, bool mastery,
                      const spell_data_t* s_data, size_t i ) override
  {
    auto splits = util::string_split<std::string_view>( type_str, "|" );
    auto tok1 = splits[ 0 ];
    auto tok2 = std::string( val_str );

    if ( splits.size() > 1 )
      tok2 += fmt::format( " {}", splits[ 1 ] );

    if ( data.buff )
    {
      sim->print_debug( "player-effects: {} {} modified by {} {} buff {} ({}#{})", *this, tok1, tok2,
                        data.use_stacks ? "per stack of" : "with", data.buff->name(), data.buff->data().id(), i );
    }
    else if ( mastery && !data.func )
    {
      sim->print_debug( "player-effects: {} {} modified by {} from {} ({}#{})", *this, tok1, tok2, s_data->name_cstr(),
                        s_data->id(), i );
    }
    else if ( data.func )
    {
      sim->print_debug( "player-effects: {} {} modified by {} with condition from {} ({}#{})", *this, tok1, tok2,
                        s_data->name_cstr(), s_data->id(), i );
    }
    else
    {
      sim->print_debug( "player-effects: {} {} modified by {} from {} ({}#{})", *this, tok1, tok2, s_data->name_cstr(),
                        s_data->id(), i );
    }
  }

  bool is_valid_target_aura( const spelleffect_data_t& eff ) const override
  {
    if ( eff.type() == E_APPLY_AURA )
      return true;

    return false;
  }

  void* get_target_effect_vector( const spelleffect_data_t& eff, uint32_t& opt_enum, double& /* val_mul */, std::string& str,
                                  bool& /* flat */, bool /* force */ ) override
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

  void parsed_effects_html( report::sc_html_stream& os )
  {
    if ( total_effects_count() )
    {
      os << "<h3 class=\"toggle\">Player Effects</h3>"
         << "<div class=\"toggle-content hide\">"
         << "<table class=\"sc left even\">\n";

      os << "<thead><tr>"
         << "<th>Type</th>"
         << "<th>Spell</th>"
         << "<th>ID</th>"
         << "<th>#</th>"
         << "<th>Value</th>"
         << "<th>Source</th>"
         << "<th>Notes</th>"
         << "</tr></thead>\n";

      auto attr_note = []( uint32_t opt ) {
        std::vector<std::string> str_list;

        for ( auto stat : { STAT_STRENGTH, STAT_AGILITY, STAT_STAMINA, STAT_INTELLECT, STAT_SPIRIT } )
          if ( opt & ( 1 << ( stat - 1 ) ) )
            str_list.emplace_back( util::stat_type_string( stat ) );

        return util::string_join( str_list );
      };

      auto mult_note = []( uint32_t opt ) {
        return opt == 0x7f ? "All" : util::school_type_string( dbc::get_school_type( opt ) );
      };

      auto pet_note = []( uint32_t opt ) {
        return opt ? "Guardian" : "Pet";
      };

      auto mastery_val = [ this ]( double v ) {
        return fmt::format( "{:.1f}%", v * mastery_coefficient() * 100 );
      };

      print_parsed_type( os, auto_attack_speed_effects, "Auto Attack Speed" );
      print_parsed_type( os, attribute_multiplier_effects, "Attribute Multiplier", attr_note );
      print_parsed_type( os, matching_armor_attribute_multiplier_effects, "Matching Armor", attr_note );
      print_parsed_type( os, versatility_effects, "Versatility" );
      print_parsed_type( os, player_multiplier_effects, "Player Multiplier", mult_note );
      print_parsed_type( os, pet_multiplier_effects, "Pet Multiplier", pet_note );
      print_parsed_type( os, attack_power_multiplier_effects, "Attack Power Multiplier" );
      print_parsed_type( os, crit_chance_effects, "Crit Chance" );
      print_parsed_type( os, leech_effects, "Leech" );
      print_parsed_type( os, expertise_effects, "Expertise" );
      print_parsed_type( os, crit_avoidance_effects, "Crit Avoidance" );
      print_parsed_type( os, parry_effects, "Parry" );
      print_parsed_type( os, armor_multiplier_effects, "Armor Multiplier" );
      print_parsed_type( os, haste_effects, "Haste" );
      print_parsed_type( os, mastery_effects, "Mastery", nullptr, mastery_val );
      print_parsed_type( os, parry_rating_from_crit_effects, "Parry Rating from Crit" );
      print_parsed_type( os, dodge_effects, "Dodge" );
      print_parsed_type( os, target_multiplier_effects, "Target Multiplier", mult_note );
      print_parsed_type( os, target_pet_multiplier_effects, "Target Pet Multiplier", pet_note );
      print_parsed_custom_type( os );

      os << "</table>\n"
         << "</div>\n";
    }
  }

  virtual size_t total_effects_count()
  {
    return auto_attack_speed_effects.size() +
           attribute_multiplier_effects.size() +
           matching_armor_attribute_multiplier_effects.size() +
           versatility_effects.size() +
           player_multiplier_effects.size() +
           pet_multiplier_effects.size() +
           attack_power_multiplier_effects.size() +
           crit_chance_effects.size() +
           leech_effects.size() +
           expertise_effects.size() +
           crit_avoidance_effects.size() +
           parry_effects.size() +
           armor_multiplier_effects.size() +
           haste_effects.size() +
           mastery_effects.size() +
           parry_rating_from_crit_effects.size() +
           dodge_effects.size() +
           target_multiplier_effects.size() +
           target_pet_multiplier_effects.size();
  }

  virtual void print_parsed_custom_type( report::sc_html_stream& ) {}

  template <typename U>
  void print_parsed_type( report::sc_html_stream& os, const std::vector<U>& entries, std::string_view n,
                          std::function<std::string( uint32_t )> note_fn = nullptr,
                          std::function<std::string( double )> val_str_fn = nullptr )
  {
    auto c = entries.size();
    if( !c )
      return;

    os.format( "<tr><td rowspan=\"{}\" style=\"background: #111\">{}</td>", c, n );

    for ( size_t i = 0; i < c; i++ )
    {
      if ( i > 0 )
        os << "<tr>";

      entries[ i ].print_parsed_line( os, *sim, true, note_fn, val_str_fn );
    }
  }
};

template <typename BASE, typename PLAYER, typename TD>
struct parse_action_effects_t : public BASE, public parse_effects_t
{
private:
  using base_t = parse_action_effects_t<BASE, PLAYER, TD>;

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
  std::vector<target_effect_t<TD>> target_multiplier_effects;
  std::vector<target_effect_t<TD>> target_crit_damage_effects;
  std::vector<target_effect_t<TD>> target_crit_chance_effects;

  parse_action_effects_t( std::string_view name, PLAYER* player, const spell_data_t* spell )
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

  std::vector<player_effect_t>* get_effect_vector( const spelleffect_data_t& eff, player_effect_t& /*data*/,
                                                   double& val_mul, std::string& str, bool& flat, bool force ) override
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

        case P_CRIT:
          str = "crit chance multiplier";
          return &crit_chance_multiplier_effects;

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
    else if ( eff.subtype() == A_MOD_RECHARGE_RATE_LABEL || eff.subtype() == A_MOD_RECHARGE_RATE_CATEGORY )
    {
      str = "cooldown";
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

  void* get_target_effect_vector( const spelleffect_data_t& eff, uint32_t& /* opt_enum */, double& /* val_mul */,
                                          std::string& str, bool& flat, bool force ) override
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
