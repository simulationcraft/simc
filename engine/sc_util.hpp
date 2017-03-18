// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "sc_enums.hpp"
#include "dbc/data_enums.hh"
#include "dbc/specialization.hpp"
#include "sc_timespan.hpp"
#include <sstream>
#include <vector>
#include <string>
#include <cstdarg>

// Forward declarations
struct player_t;


/**
 * Defines various utility, string and enum <-> string translation functions.
 */
namespace util
{
double wall_time();
double cpu_time();

template <typename T>
T ability_rank( int player_level, T ability_value, int ability_level, ... );
double interpolate( int level, double val_60, double val_70, double val_80, double val_85 = -1 );

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
const char* resource_type_string      ( resource_e );
const char* result_type_string        ( result_e type );
const char* block_result_type_string  ( block_result_e type );
const char* full_result_type_string   ( full_result_e type );
const char* amount_type_string        ( dmg_e type );
const char* school_type_string        ( school_e type );
const char* armor_type_string         ( int type );
const char* armor_type_string         ( item_subclass_armor type );
const char* cache_type_string         ( cache_e type );
const char* proc_type_string          ( proc_types type );
const char* proc_type2_string         ( proc_types2 type );
const char* special_effect_string     ( special_effect_e type );
const char* special_effect_source_string( special_effect_source_e type );
const char* scale_metric_type_abbrev  ( scale_metric_e );
const char* scale_metric_type_string  ( scale_metric_e );
const char* slot_type_string          ( slot_e type );
const char* stat_type_string          ( stat_e type );
const char* stat_type_abbrev          ( stat_e type );
const char* stat_type_wowhead         ( stat_e type );
const char* stat_type_gem             ( stat_e type );
const char* weapon_type_string        ( weapon_e type );
const char* weapon_class_string       ( int class_ );
const char* weapon_subclass_string    ( int subclass );
const char* item_quality_string       ( int item_quality );
const char* specialization_string     ( specialization_e spec );
const char* movement_direction_string( movement_direction_e );
const char* class_id_string( player_e type );
const char* spec_string_no_class( const player_t&p );
const char* retarget_event_string     ( retarget_event_e );

uint32_t    school_type_component     ( school_e s_type, school_e c_type );
bool is_match_slot( slot_e slot );
item_subclass_armor matching_armor_type ( player_e ptype );
resource_e  translate_power_type      ( power_e );
stat_e      power_type_to_stat        ( power_e );

attribute_e parse_attribute_type ( const std::string& name );
dmg_e parse_dmg_type             ( const std::string& name );
meta_gem_e parse_meta_gem_type   ( const std::string& name );
player_e parse_player_type       ( const std::string& name );
pet_e parse_pet_type             ( const std::string& name );
profession_e parse_profession_type( const std::string& name );
position_e parse_position_type   ( const std::string& name );
race_e parse_race_type           ( const std::string& name );
role_e parse_role_type           ( const std::string& name );
resource_e parse_resource_type   ( const std::string& name );
result_e parse_result_type       ( const std::string& name );
school_e parse_school_type       ( const std::string& name );
slot_e parse_slot_type           ( const std::string& name );
stat_e parse_stat_type           ( const std::string& name );
scale_metric_e parse_scale_metric( const std::string& name );
specialization_e parse_specialization_type( const std::string &name );
movement_direction_e parse_movement_direction( const std::string& name );
item_subclass_armor parse_armor_type( const std::string& name );
weapon_e parse_weapon_type       ( const std::string& name );

int parse_item_quality                ( const std::string& quality );
bool parse_origin( std::string& region, std::string& server, std::string& name, const std::string& origin );
int class_id_mask( player_e type );
int class_id( player_e type );
unsigned race_mask( race_e type );
unsigned race_id( race_e type );
unsigned pet_mask( pet_e type );
unsigned pet_id( pet_e type );
player_e pet_class_type( pet_e type );
player_e translate_class_id( int cid );
player_e translate_class_str( const std::string& s );
race_e translate_race_id( int rid );
stat_e translate_item_mod( int stat_mod );
bool is_combat_rating( item_mod_type t );
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

std::vector<std::string> string_split( const std::string& str, const std::string& delim );
size_t string_split_allow_quotes( std::vector<std::string>& results, const std::string& str, const char* delim );
size_t string_split( const std::string& str, const char* delim, const char* format, ... );
void string_strip_quotes( std::string& str );
void replace_all( std::string& s, const std::string&, const std::string& );
void erase_all( std::string& s, const std::string& from );

template <typename T>
std::string to_string( const T& t );
std::string to_string( double f );
std::string to_string( double f, int precision );

unsigned to_unsigned( const std::string& str );
unsigned to_unsigned( const char* str );
int to_int( const std::string& str );
int to_int( const char* str );

int64_t parse_date( const std::string& month_day_year );

int printf( const char *format, ... ) PRINTF_ATTRIBUTE( 1, 2 );
int fprintf( FILE *stream, const char *format, ... ) PRINTF_ATTRIBUTE( 2, 3 );
int vfprintf( FILE *stream, const char *format, va_list fmtargs ) PRINTF_ATTRIBUTE( 2, 0 );
int vprintf( const char *format, va_list fmtargs ) PRINTF_ATTRIBUTE( 1, 0 );

std::string encode_html( const std::string& );
std::string decode_html( const std::string& str );
void urlencode( std::string& str );
void urldecode( std::string& str );
std::string uchar_to_hex( unsigned char );
std::string google_image_chart_encode( const std::string& str );
std::string create_blizzard_talent_url( const player_t& p );
std::string create_wowhead_artifact_url( const player_t& p );

bool str_compare_ci( const std::string& l, const std::string& r );
bool str_in_str_ci ( const std::string& l, const std::string& r );
bool str_prefix_ci ( const std::string& str, const std::string& prefix );

double floor( double X, unsigned int decplaces = 0 );
double ceil( double X, unsigned int decplaces = 0 );
double round( double X, unsigned int decplaces = 0 );
double approx_sqrt( double X );

void tolower( std::string& str );

void tokenize( std::string& name );
std::string tokenize_fn( std::string name );
std::string inverse_tokenize( const std::string& name );

bool is_number( const std::string& s );

int snformat( char* buf, size_t size, const char* fmt, ... );
void fuzzy_stats( std::string& encoding, const std::string& description );

template <class T>
int numDigits( T number );

template <typename T>
T str_to_num( const std::string& );

bool contains_non_ascii( const std::string& );

std::ostream& stream_printf( std::ostream&, const char* format, ... );

template<class T>
T from_string( const std::string& );

template<>
inline int from_string( const std::string& v )
{
  return strtol( v.c_str(), nullptr, 10 );
}
template<>
inline bool from_string( const std::string& v )
{
  return from_string<int>( v ) != 0;
}

template<>
inline unsigned from_string( const std::string& v )
{
  return strtoul( v.c_str(), nullptr, 10 );
}

template<>
inline double from_string( const std::string& v )
{
  return strtod( v.c_str(), nullptr );
}
template<>
inline timespan_t from_string( const std::string& v )
{
  return timespan_t::from_seconds( util::from_string<double>( v ) );
}
template<>
inline std::string from_string( const std::string& v )
{
  return v;
}

} // namespace util


/* Simple String to Number function, using stringstream
 * This will NOT translate all numbers in the string to a number,
 * but stops at the first non-numeric character.
 */
template <typename T>
T util::str_to_num ( const std::string& text )
{
  std::istringstream ss( text );
  T result;
  return ss >> result ? result : T();
}

// ability_rank =====================================================
template <typename T>
T util::ability_rank( int player_level,
                      T   ability_value,
                      int ability_level, ... )
{
  va_list vap;
  va_start( vap, ability_level );

  while ( player_level < ability_level )
  {
    ability_value = va_arg( vap, T );
    ability_level = va_arg( vap, int );
  }

  va_end( vap );

  return ability_value;
}

template <typename T>
std::string util::to_string( const T& t )
{ std::stringstream s; s << t; return s.str(); }
