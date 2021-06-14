// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "dbc/data_enums.hh"
#include "dbc/specialization.hpp"
#include "sc_enums.hpp"
#include "util/timespan.hpp"
#include "util/span.hpp"
#include "util/string_view.hpp"

#include "fmt/format.h"
#include "fmt/ostream.h"

#include <exception>
#include <iosfwd>
#include <string>
#include <vector>

// Forward declarations
struct player_t;
class dbc_t;

/**
 * Defines various utility, string and enum <-> string translation functions.
 */
namespace util
{
double stat_value( const player_t* p, stat_e stat );
stat_e highest_stat( const player_t* p, util::span<const stat_e> stat );

std::string version_info_str( const dbc_t* dbc );

const char* attribute_type_string     ( attribute_e type );
const char* dot_behavior_type_string  ( dot_behavior_e t );
const char* meta_gem_type_string      ( meta_gem_e type );
const char* player_type_string        ( player_e );
const char* pet_type_string           ( pet_e type );
const char* position_type_string      ( position_e );
const char* profession_type_string    ( profession_e );
const char* race_type_string          ( race_e );
const char* stats_type_string         ( stats_e );
const char* role_type_string          ( role_e );
const char* gcd_haste_type_string     (gcd_haste_type);
const char* resource_type_string      ( resource_e );
const char* result_type_string        ( result_e type );
const char* block_result_type_string  ( block_result_e type );
const char* full_result_type_string   ( full_result_e fulltype );
const char* amount_type_string        ( result_amount_type type );
const char* school_type_string        ( school_e school );
const char* armor_type_string         ( int type );
const char* armor_type_string         ( item_subclass_armor type );
const char* cache_type_string         ( cache_e );
const char* proc_type_string          ( proc_types type );
const char* proc_type2_string         ( proc_types2 type );
const char* item_spell_trigger_string ( item_spell_trigger_type type );
const char* special_effect_string     ( special_effect_e type );
const char* special_effect_source_string( special_effect_source_e type );
const char* scale_metric_type_abbrev  ( scale_metric_e );
const char* scale_metric_type_string  ( scale_metric_e );
const char* slot_type_string          ( slot_e slot );
const char* stat_type_string          ( stat_e stat );
const char* stat_type_abbrev          ( stat_e stat );
const char* stat_type_wowhead         ( stat_e stat );
const char* stat_type_gem             ( stat_e stat );
const char* weapon_type_string        ( weapon_e type );
const char* weapon_class_string       ( int weapon_class );
const char* weapon_subclass_string    ( int subclass );
const char* item_quality_string       ( int quality );
const char* specialization_string     ( specialization_e spec );
const char* movement_direction_string( movement_direction_type );
const char* spec_string_no_class( const player_t&p );
const char* retarget_event_string     ( retarget_source );
const char* buff_refresh_behavior_string   ( buff_refresh_behavior );
const char* buff_stack_behavior_string   ( buff_stack_behavior );
const char* buff_tick_behavior_string   ( buff_tick_behavior );
const char* buff_tick_time_behavior_string   ( buff_tick_time_behavior );
const char* action_energize_type_string( action_energize energize_type );
std::string rppm_scaling_string       ( unsigned );
std::string profile_source_string( profile_source );

uint32_t    school_type_component     ( school_e s_type, school_e c_type );
bool is_match_slot( slot_e slot );
item_subclass_armor matching_armor_type ( player_e ptype );
resource_e  translate_power_type      ( power_e );
stat_e      power_type_to_stat        ( power_e );

attribute_e parse_attribute_type ( util::string_view name );
result_amount_type parse_dmg_type ( util::string_view name );
meta_gem_e parse_meta_gem_type   ( util::string_view name );
player_e parse_player_type       ( util::string_view name );
pet_e parse_pet_type             ( util::string_view name );
profession_e parse_profession_type( util::string_view name );
position_e parse_position_type   ( util::string_view name );
race_e parse_race_type           ( util::string_view name );
role_e parse_role_type           ( util::string_view name );
resource_e parse_resource_type   ( util::string_view name );
result_e parse_result_type       ( util::string_view name );
school_e parse_school_type       ( util::string_view name );
slot_e parse_slot_type           ( util::string_view name );
stat_e parse_stat_type           ( util::string_view name );
scale_metric_e parse_scale_metric( util::string_view name );
profile_source parse_profile_source( util::string_view name );
specialization_e parse_specialization_type( util::string_view name );
movement_direction_type parse_movement_direction( util::string_view name );
item_subclass_armor parse_armor_type( util::string_view name );
weapon_e parse_weapon_type       ( util::string_view name );

int parse_item_quality                ( util::string_view quality );
bool parse_origin( std::string& region, std::string& server, std::string& name, util::string_view origin );
int class_id_mask( player_e type );
int class_id( player_e type );
unsigned race_mask( race_e race );
unsigned race_id( race_e race );
unsigned pet_mask( pet_e type );
unsigned pet_id( pet_e type );
player_e pet_class_type( pet_e type );
player_e translate_class_id( int cid );
player_e translate_class_str( util::string_view s );
race_e translate_race_id( int rid );
bool is_alliance( race_e );
bool is_horde( race_e );
stat_e translate_item_mod( int item_mod );
rating_e stat_to_rating( stat_e );
bool is_combat_rating( item_mod_type t );
bool is_combat_rating( stat_e t );
bool is_primary_stat( stat_e );
int translate_stat( stat_e stat );
stat_e translate_attribute( attribute_e attribute );
stat_e translate_rating_mod( unsigned ratings );
std::vector<stat_e> translate_all_rating_mod( unsigned ratings );
slot_e translate_invtype( inventory_type inv_type );
weapon_e translate_weapon_subclass( int weapon_subclass );
item_subclass_weapon translate_weapon( weapon_e weapon );
profession_e translate_profession_id( int skill_id );
bool socket_gem_match( item_socket_color socket, item_socket_color gem );
double crit_multiplier( meta_gem_e gem );


template<typename StringType = std::string>
inline std::vector<StringType> string_split( util::string_view str, util::string_view delim, bool skip_empty_entries = true )
{
  std::vector<StringType> results;
  if ( str.empty() )
    return results;

  typename StringType::size_type cut_pt, start = 0;

  while ( ( cut_pt = str.find_first_of( delim, start ) ) != StringType::npos )
  {
    if ( !skip_empty_entries || cut_pt > start ) // Found something, push to the vector
      results.emplace_back( str.substr( start, cut_pt - start ) );

    start = cut_pt + 1; // skip the found delimeter
  }

  if ( start < str.size() )
  {
    // Push the tail
    results.emplace_back( str.substr( start, str.size() - start ) );
  }

  return results;
}

std::vector<std::string> string_split_allow_quotes( util::string_view str, util::string_view delim );
template <typename T>
std::string string_join( const T& container, util::string_view delim = ", " );
void replace_all( std::string& s, util::string_view, util::string_view );
void erase_all( std::string& s, util::string_view from );

template <typename T>
std::string to_string( const T& t );
std::string to_string( double f );
std::string to_string( double f, int precision );

unsigned to_unsigned( const std::string& str );
unsigned to_unsigned( util::string_view str );


unsigned to_unsigned_ignore_error( const std::string& str, unsigned on_error_value );
unsigned to_unsigned_ignore_error( util::string_view str, unsigned on_error_value );

int to_int( const std::string& str );
int to_int( util::string_view str );

double to_double( const std::string& str );
double to_double( util::string_view str );

int64_t parse_date( util::string_view month_day_year );

std::string encode_html( util::string_view );
std::string decode_html( const std::string& );
// Strips away all non-underscore, non-alphanumeric ASCII characters from the string.
std::string remove_special_chars( util::string_view );
void urlencode( std::string& str );
void urldecode( std::string& str );
std::string create_blizzard_talent_url( const player_t& p );

bool str_compare_ci( util::string_view l, util::string_view r );
bool str_in_str_ci ( util::string_view l, util::string_view r );
bool str_prefix_ci ( util::string_view str, util::string_view prefix );

double floor( double arg, unsigned int decplaces = 0 );
double ceil( double arg, unsigned int decplaces = 0 );
double round( double arg, unsigned int decplaces = 0 );
double approx_sqrt( double arg );

void tolower( std::string& str );

void tokenize( std::string& name );
std::string tokenize_fn( util::string_view );
std::string inverse_tokenize( util::string_view name );

bool is_number( util::string_view s );

void fuzzy_stats( std::string& encoding, util::string_view description );

template <class T>
int numDigits( T number );

bool contains_non_ascii( util::string_view );

void print_chained_exception( const std::exception& e, std::ostream& out, int level =  0);
void print_chained_exception( const std::exception_ptr& eptr, std::ostream& out, int level =  0);

} // namespace util

template <typename T>
std::string util::to_string( const T& t )
{
  return fmt::to_string( t );
}

template <typename T>
std::string util::string_join( const T& container, util::string_view delim )
{
  return fmt::format( "{}", fmt::join( container, delim ) );
}

// fmtlib formatters for enums
namespace fmt {
#define SC_ENUM_FORMATTER( EnumType, ToStringFn )                          \
  template <> struct formatter<EnumType> : formatter<string_view> {        \
    template <typename FormatContext>                                      \
    auto format(EnumType val, FormatContext& ctx) -> decltype(ctx.out()) { \
      return formatter<string_view>::format(ToStringFn(val), ctx);         \
    }                                                                      \
  }

SC_ENUM_FORMATTER( attribute_e,             util::attribute_type_string );
SC_ENUM_FORMATTER( dot_behavior_e,          util::dot_behavior_type_string );
SC_ENUM_FORMATTER( meta_gem_e,              util::meta_gem_type_string );
SC_ENUM_FORMATTER( player_e,                util::player_type_string );
SC_ENUM_FORMATTER( pet_e,                   util::pet_type_string );
SC_ENUM_FORMATTER( position_e,              util::position_type_string );
SC_ENUM_FORMATTER( profession_e,            util::profession_type_string );
SC_ENUM_FORMATTER( race_e,                  util::race_type_string );
SC_ENUM_FORMATTER( stats_e,                 util::stats_type_string );
SC_ENUM_FORMATTER( role_e,                  util::role_type_string );
SC_ENUM_FORMATTER( gcd_haste_type,          util::gcd_haste_type_string );
SC_ENUM_FORMATTER( resource_e,              util::resource_type_string );
SC_ENUM_FORMATTER( result_e,                util::result_type_string );
SC_ENUM_FORMATTER( block_result_e,          util::block_result_type_string );
SC_ENUM_FORMATTER( full_result_e,           util::full_result_type_string );
SC_ENUM_FORMATTER( result_amount_type,      util::amount_type_string );
SC_ENUM_FORMATTER( school_e,                util::school_type_string );
SC_ENUM_FORMATTER( cache_e,                 util::cache_type_string );
SC_ENUM_FORMATTER( proc_types,              util::proc_type_string );
SC_ENUM_FORMATTER( proc_types2,             util::proc_type2_string );
SC_ENUM_FORMATTER( item_spell_trigger_type, util::item_spell_trigger_string );
SC_ENUM_FORMATTER( special_effect_e,        util::special_effect_string );
SC_ENUM_FORMATTER( special_effect_source_e, util::special_effect_source_string );
SC_ENUM_FORMATTER( scale_metric_e,          util::scale_metric_type_string );
SC_ENUM_FORMATTER( slot_e,                  util::slot_type_string );
SC_ENUM_FORMATTER( stat_e,                  util::stat_type_string );
SC_ENUM_FORMATTER( weapon_e,                util::weapon_type_string );
SC_ENUM_FORMATTER( specialization_e,        util::specialization_string );
SC_ENUM_FORMATTER( movement_direction_type, util::movement_direction_string );
SC_ENUM_FORMATTER( retarget_source,         util::retarget_event_string );
SC_ENUM_FORMATTER( buff_refresh_behavior,   util::buff_refresh_behavior_string );
SC_ENUM_FORMATTER( buff_stack_behavior,     util::buff_stack_behavior_string );
SC_ENUM_FORMATTER( buff_tick_behavior,      util::buff_tick_behavior_string );
SC_ENUM_FORMATTER( buff_tick_time_behavior, util::buff_tick_time_behavior_string );
SC_ENUM_FORMATTER( action_energize,       util::action_energize_type_string );

#undef SC_ENUM_FORMATTER
} // namespace fmt
