// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "util/fmt/format.h"
#include "util/fmt/ostream.h"
#include "util/fmt/printf.h"
#include "sc_enums.hpp"
#include "dbc/data_enums.hh"
#include "dbc/specialization.hpp"
#include "sc_timespan.hpp"
#include <sstream>
#include <vector>
#include <string>
#include <cstdarg>
#include <chrono>

// Forward declarations
struct player_t;


/**
 * Defines various utility, string and enum <-> string translation functions.
 */
namespace util
{
double wall_time();
double cpu_time();

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
const char* full_result_type_string   ( full_result_e fulltype );
const char* amount_type_string        ( dmg_e type );
const char* school_type_string        ( school_e school );
const char* armor_type_string         ( int type );
const char* armor_type_string         ( item_subclass_armor type );
const char* cache_type_string         ( cache_e );
const char* proc_type_string          ( proc_types type );
const char* proc_type2_string         ( proc_types2 type );
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
const char* movement_direction_string( movement_direction_e );
const char* class_id_string( player_e type );
const char* spec_string_no_class( const player_t&p );
const char* retarget_event_string     ( retarget_event_e );
const char* buff_refresh_behavior_string   ( buff_refresh_behavior );
const char* buff_stack_behavior_string   ( buff_stack_behavior );
const char* buff_tick_behavior_string   ( buff_tick_behavior );
const char* buff_tick_time_behavior_string   ( buff_tick_time_behavior );
std::string rppm_scaling_string       ( unsigned );
std::string profile_source_string( profile_source );

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
profile_source parse_profile_source( const std::string& name );
specialization_e parse_specialization_type( const std::string &name );
movement_direction_e parse_movement_direction( const std::string& name );
item_subclass_armor parse_armor_type( const std::string& name );
weapon_e parse_weapon_type       ( const std::string& name );

int parse_item_quality                ( const std::string& quality );
bool parse_origin( std::string& region, std::string& server, std::string& name, const std::string& origin );
int class_id_mask( player_e type );
int class_id( player_e type );
unsigned race_mask( race_e race );
unsigned race_id( race_e race );
unsigned pet_mask( pet_e type );
unsigned pet_id( pet_e type );
player_e pet_class_type( pet_e type );
player_e translate_class_id( int cid );
player_e translate_class_str( const std::string& s );
race_e translate_race_id( int rid );
bool is_alliance( race_e );
bool is_horde( race_e );
stat_e translate_item_mod( int item_mod );
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
std::vector<std::string> string_split_allow_quotes( std::string str, const char* delim );
template <typename T>
std::string string_join( const T& container, const std::string& delim = ", " );
std::string string_strip_quotes( std::string str );
void replace_all( std::string& s, const std::string&, const std::string& );
void erase_all( std::string& s, const std::string& from );

template <typename T>
std::string to_string( const T& t );
std::string to_string( double f );
std::string to_string( double f, int precision );

unsigned to_unsigned( const std::string& str );
unsigned to_unsigned( const char* str );
int to_int( const std::string& str );

int64_t parse_date( const std::string& month_day_year );

template<typename Format, typename... Args>
int printf(Format&& format, Args&& ... args)
{
  return fmt::printf(std::forward<Format>(format), std::forward<Args>(args)... );
}
template<typename Format, typename... Args>
int fprintf(std::FILE* stream, Format&& format, Args&& ... args)
{
  return fmt::fprintf(stream, std::forward<Format>(format), std::forward<Args>(args)... );
}

std::string encode_html( const std::string& );
std::string decode_html( const std::string& );
void urlencode( std::string& str );
void urldecode( std::string& str );
std::string uchar_to_hex( unsigned char );
std::string create_blizzard_talent_url( const player_t& p );
std::string create_wowhead_artifact_url( const player_t& p );

bool str_compare_ci( const std::string& l, const std::string& r );
bool str_in_str_ci ( const std::string& l, const std::string& r );
bool str_prefix_ci ( const std::string& str, const std::string& prefix );

bool str_begins_with( const std::string& str, const std::string& beginsWith );
bool str_begins_with_ci( const std::string& str, const std::string& beginsWith );

double floor( double X, unsigned int decplaces = 0 );
double ceil( double X, unsigned int decplaces = 0 );
double round( double X, unsigned int decplaces = 0 );
double approx_sqrt( double X );

void tolower( std::string& str );

void tokenize( std::string& name );
std::string tokenize_fn( std::string name );
std::string inverse_tokenize( const std::string& name );

bool is_number( const std::string& s );

void fuzzy_stats( std::string& encoding, const std::string& description );

template <class T>
int numDigits( T number );

bool contains_non_ascii( const std::string& );

template <class T>
double duration_fp_seconds(const T& chrono_time)
{
  auto now = std::chrono::high_resolution_clock::now();

  return std::chrono::duration<double, std::chrono::seconds::period>( now - chrono_time ).count();
}

// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
inline unsigned next_power_of_two( unsigned v )
{
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;

  return v;
}

void print_chained_exception( const std::exception& e, std::ostream& out = std::cerr, int level =  0);
void print_chained_exception( std::exception_ptr eptr, std::ostream& out = std::cerr, int level =  0);

} // namespace util

template <typename T>
std::string util::to_string( const T& t )
{ std::stringstream s; s << t; return s.str(); }

template <typename T>
std::string util::string_join( const T& container, const std::string& delim )
{
  std::stringstream s;

  for ( auto i = container.cbegin(); i < container.cend(); ++i )
  {
    if ( i != container.cbegin() && ! delim.empty() )
    {
      s << delim;
    }

    s << util::to_string( *i );
  }

  return s.str();
}
