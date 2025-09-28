// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"

#include "util/timespan.hpp"

#include <vector>

#pragma once

// Container to hold base value and all permanent/passive modifiers for values that are automatically parsed from spell
// data. We need to hold these separate because flat modifiers, including those from dynamic sources, are calculated
// before percent multipliers. The final value should be properly reconstituted in the relevant method. Also includes
// operator overloads for ease of use/compatibility with operations to type previously used to store the value.

template <typename T>
struct parsed_value_t
{
  T base;
  T flat_add;
  double pct_mul;

  parsed_value_t( T value = T() ) : base( value ), flat_add(), pct_mul( 1.0 ) {}

  T value() const
  {
    if constexpr( std::is_same_v<T, timespan_t> )
    {
      // timespan_t truncates to the nearest second, but in-game testing suggests aura periods & durations are rounded instead.
      // if this holds for all time values, timespan_t should be adjusted instead.
      return timespan_t::from_native( std::round( timespan_t::to_native( base + flat_add ) * pct_mul ) );
    }
    else
    {
      return ( base + flat_add ) * pct_mul;
    }
  }

  operator T() const
  { return value(); }

  parsed_value_t& operator=( T v )
  { base = v; flat_add = T(); pct_mul = 1.0; return *this; }

  parsed_value_t& operator+=( T v )
  { flat_add += v; return *this; }

  parsed_value_t& operator-=( T v )
  { flat_add -= v; return *this; }

  template <typename U>
  parsed_value_t& operator*=( U v )
  { pct_mul *= v; return *this; }

  template <typename U>
  parsed_value_t& operator/=( U v )
  { pct_mul /= v; return *this; }

  friend void sc_format_to( const parsed_value_t<T>& v, fmt::format_context::iterator out )
  { fmt::format_to( out, "{}", v.value() ); }

  parsed_value_t& operator=( std::array<double, 3> v )
  {
    // special handling for timespan_t as timespan_t() ctor is private
    if constexpr ( std::is_same_v<T, timespan_t> )
    {
      base = timespan_t::from_millis( v[ 0 ] );
      flat_add = timespan_t::from_millis( v[ 1 ] );
    }
    else if constexpr( std::is_arithmetic_v<T> )
    {
      base = v[ 0 ];
      flat_add = v[ 1 ];
    }
    else
    {
      base = T( v[ 0 ] );
      flat_add = T( v[ 1 ] );
    }
    pct_mul = v[ 2 ];
    return *this;
  }

  // additional operator overrides for timespan_t, as it is used quite often. these are unnecessary for POD
  template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, timespan_t>>>
  bool operator==( const timespan_t& t ) const
  { return value() == t; }

  template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, timespan_t>>>
  bool operator<( const timespan_t& t ) const
  { return value() < t; }

  template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, timespan_t>>>
  bool operator>( const timespan_t& t ) const
  { return value() > t; }

  template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, timespan_t>>>
  bool operator<=( const timespan_t& t ) const
  { return value() <= t; }

  template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, timespan_t>>>
  bool operator>=( const timespan_t& t ) const
  { return value() >= t; }

  template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, timespan_t>>>
  double total_seconds() const
  { return value().total_seconds(); }

  template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, timespan_t>>>
  timespan_t::native_t total_millis() const
  { return value().total_millis(); }
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
