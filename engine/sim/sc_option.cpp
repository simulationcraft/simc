// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_option.hpp"

#include <iostream>

#include "fmt/format.h"
#include "util/io.hpp"
#include "util/util.hpp"
#include "util/generic.hpp"
#include "lib/utf8-cpp/utf8.h"

namespace { // UNNAMED NAMESPACE ============================================

template <typename Range>
inline bool is_valid_utf8(const Range& r)
{
  return utf8::is_valid(range::begin(r), range::end(r));
}

bool is_white_space( char c )
{
  return ( c == ' ' || c == '\t' || c == '\n' || c == '\r' );
}

void open_file( io::ifstream& f, util::span<const std::string> splits, const std::string& name, std::string& actual_name )
{
  for ( auto& split : splits )
  {
    auto file_path = split + "/" + name;
    f.open( file_path );
    if ( f.is_open() )
    {
      actual_name = file_path;
      return;
    }
  }

  actual_name = name;
  f.open( name );
}

std::string base_name( const std::string& file_path )
{
#if defined( SC_WINDOWS )
  auto base_it = file_path.rfind('\\');
#else
  auto base_it = file_path.rfind('/');
#endif
  std::string file_name;
  if ( base_it != std::string::npos )
  {
    file_name = file_path.substr( base_it + 1 );
  }
  else
  {
    file_name = file_path;
  }

  auto extension_it = file_name.find('.');
  std::string base_name;
  if ( extension_it != std::string::npos )
  {
    base_name = file_name.substr( 0, extension_it );
  }
  else
  {
    base_name = file_name;
  }

  return base_name;
}

void do_replace( const option_db_t& opts, std::string& str, std::string::size_type begin, int depth )
{
  static const int max_depth = 10;
  if ( depth > max_depth )
  {
    throw std::invalid_argument( fmt::format( "Nesting depth exceeded for: '{}' (max: {})", str, max_depth ) );
  }

  if ( begin == std::string::npos )
  {
    return;
  }

  auto next = str.find( "$(", begin + 2 );
  if ( next != std::string::npos )
  {
    do_replace( opts, str, next, ++depth );
  }

  auto end = str.find( ')', begin + 2 );
  if ( end == std::string::npos )
  {
    throw std::invalid_argument( fmt::format( "Unbalanced parenthesis in template variable for: '{}'", str ) );
  }

  auto var = str.substr( begin + 2, ( end - begin ) - 2 );
  if ( opts.var_map.find( var ) == opts.var_map.end() )
  {
    throw std::invalid_argument( fmt::format( "Missing template variable: '{}'", var ) );
  }

  str.replace( begin, end - begin + 1, opts.var_map.at( var ) );
}


// Shared data base path
#ifndef SC_SHARED_DATA
  #if defined( SC_LINUX_PACKAGING )
    const char* SC_SHARED_DATA SC_LINUX_PACKAGING = "/profiles";
  #else
    const char* SC_SHARED_DATA = "";
  #endif
#endif

struct converter_uint64_t
{
  static uint64_t convert( util::string_view v )
  {
    return std::stoull( std::string( v ) );
  }
};

struct converter_int_t
{
  static int convert( util::string_view v )
  {
    return util::to_int( v );
  }
};

struct converter_uint_t
{
  static unsigned int convert( util::string_view v )
  {
    return util::to_unsigned( v );
  }
};

struct converter_double_t
{
  static double convert( util::string_view v )
  {
    return util::to_double( v );
  }
};

struct converter_timespan_t
{
  static timespan_t convert( util::string_view v )
  {
    return timespan_t::from_seconds( util::to_double( v ) );
  }
};

/* Option helper class
 * Stores a reference of given type T
 */
template<class T>
struct opts_helper_t : public option_t
{
  using base_t = opts_helper_t<T>;

  opts_helper_t( util::string_view name, T& ref ) :
    option_t( name ),
    _ref( ref )
  { }
protected:
  T& _ref;
};

struct opt_string_t : public option_t
{
  opt_string_t( util::string_view name, std::string& addr ) :
    option_t( name ),
    _ref( addr )
  { }
protected:
  opts::parse_status do_parse( sim_t*, util::string_view n, util::string_view v ) const override
  {
    if ( n != name() )
      return opts::parse_status::CONTINUE;

    _ref = std::string( v );
    return opts::parse_status::OK;
  }

  void do_format_to( fmt::format_context::iterator out ) const override
  {
    fmt::format_to( out, "{}={}\n", name(), _ref );
  }
private:
  std::string& _ref;
};

struct opt_append_t : public option_t
{
  opt_append_t( util::string_view name, std::string& addr ) :
    option_t( name ),
    _ref( addr )
  { }

protected:
  opts::parse_status do_parse( sim_t*, util::string_view n, util::string_view v ) const override
  {
    if ( n != name() )
      return opts::parse_status::CONTINUE;

    _ref += std::string( v );
    return opts::parse_status::OK;
  }

  void do_format_to( fmt::format_context::iterator out ) const override
  {
    fmt::format_to( out, "{}+={}\n", name(), _ref );
  }
private:
  std::string& _ref;
};

/**
 * Option template for converting numeric type (unsigned, int, double, etc.)
 *
 * Empty input string is silently interpreted as 0, allowing for options with empty value.
 */
template<typename T, typename Converter>
struct opt_numeric_t : public option_t
{
  opt_numeric_t( util::string_view name, T& addr ) :
    option_t( name ),
    _ref( addr )
  { }
protected:
  opts::parse_status do_parse( sim_t*, util::string_view n, util::string_view v ) const override
  {
    if ( n != name() )
      return opts::parse_status::CONTINUE;

    if ( v.empty() )
      _ref = {};
    else
      _ref = Converter::convert( v );

    return opts::parse_status::OK;
  }

  void do_format_to( fmt::format_context::iterator out ) const override
  {
    fmt::format_to( out, "{}={}\n", name(), _ref );
  }
private:
  T& _ref;
};


/**
 * Option template for converting numeric type (unsigned, int, double, etc.) with min/max check
 *
 * Throws std::invalid_argument if value is out of min/max bounds.
 * Empty input string is silently interpreted as 0, allowing for options with empty value.
 */
template<typename T, typename Converter>
struct opt_numeric_mm_t : public option_t
{
  opt_numeric_mm_t( util::string_view name, T& addr, T min, T max ) :
    option_t( name ),
    _ref( addr ),
    _min( min ),
    _max( max )
  { }

protected:
  opts::parse_status do_parse( sim_t*, util::string_view n, util::string_view v ) const override
  {
    if ( n != name() )
      return opts::parse_status::CONTINUE;

    T tmp;
    if ( v.empty() )
      tmp = {};
    else
      tmp = Converter::convert( v );

    // Range checking
    if ( tmp < _min || tmp > _max ) {
      throw std::invalid_argument( fmt::format(
        "Option '{}' with value '{}' not within valid boundaries [{}..{}]", n, v, _min, _max ) );
    }
    _ref = tmp;
    return opts::parse_status::OK;
  }

  void do_format_to( fmt::format_context::iterator out ) const override
  {
    fmt::format_to( out, "{}={}\n", name(), _ref );
  }
private:
  T& _ref;
  T _min, _max;
};

struct opt_bool_t : public option_t
{
  opt_bool_t( util::string_view name, bool& addr ) :
    option_t( name ),
    _ref( addr )
  { }
protected:
  opts::parse_status do_parse( sim_t*, util::string_view n, util::string_view v ) const override
  {
    if ( n != name() )
      return opts::parse_status::CONTINUE;

    if ( v != "0" && v != "1" )
    {
      throw std::invalid_argument( fmt::format( "Acceptable values for '{}' are '1' or '0'", n ) );
    }

    _ref = util::to_int( v ) != 0;
    return opts::parse_status::OK;
  }

  void do_format_to( fmt::format_context::iterator out ) const override
  {
    fmt::format_to( out, "{}={:d}\n", name(), _ref );
  }
private:
  bool& _ref;
};

struct opt_bool_int_t : public option_t
{
  opt_bool_int_t( util::string_view name, int& addr ) :
    option_t( name ),
    _ref( addr )
  { }
protected:
  opts::parse_status do_parse( sim_t*, util::string_view n, util::string_view v ) const override
  {
    if ( n != name() )
      return opts::parse_status::CONTINUE;

    if ( v != "0" && v != "1" )
    {
      throw std::invalid_argument( fmt::format( "Acceptable values for '{}' are '1' or '0'", n ) );
    }

    _ref = util::to_int( v );
    return opts::parse_status::OK;
  }
  
  void do_format_to( fmt::format_context::iterator out ) const override
  {
    fmt::format_to( out, "{}={}\n", name(), _ref );
  }
private:
  int& _ref;
};

struct opts_sim_func_t : public option_t
{
  opts_sim_func_t( util::string_view name, const opts::function_t& ref ) :
    option_t( name ),
    _fun( ref )
  { }
protected:
  opts::parse_status do_parse( sim_t* sim, util::string_view n, util::string_view value ) const override
  {
    if ( name() != n )
      return opts::parse_status::CONTINUE;

     return _fun( sim, n, value ) ? opts::parse_status::OK : opts::parse_status::FAILURE;
  }

  void do_format_to( fmt::format_context::iterator out ) const override
  {
    fmt::format_to( out, "function option: {}\n", name() );
  }
private:
  opts::function_t _fun;
};

struct opts_map_t : public option_t
{
  opts_map_t( util::string_view name, opts::map_t& ref ) :
    option_t( name ),
    _ref( ref )
  { }
protected:
  opts::parse_status do_parse( sim_t*, util::string_view n, util::string_view v ) const override
  {
    std::string::size_type last = n.size() - 1;
    bool append = false;
    if ( n[ last ] == '+' )
    {
      append = true;
      --last;
    }
    auto dot = n.rfind( ".", last );
    if ( dot != std::string::npos )
    {
      if ( name() == n.substr( 0, dot + 1 ) )
      {
        auto key = n.substr( dot + 1, last - dot );
        if ( key.empty() )
        {
          return opts::parse_status::FAILURE;
        }

        std::string& value = _ref[ std::string(key) ];

        auto new_value = std::string( v );
        value = append ? ( value + new_value ) : new_value;
        return opts::parse_status::OK;
      }
    }

    return opts::parse_status::CONTINUE;
  }
  void do_format_to( fmt::format_context::iterator out ) const override
  {
    for ( const auto& entry : _ref )
    {
      fmt::format_to( out, "{}{}={}", name(), entry.first, entry.second );
    }
  }
  opts::map_t& _ref;
};

struct opts_map_list_t : public option_t
{
  opts_map_list_t( util::string_view name, opts::map_list_t& ref ) :
    option_t( name ), _ref( ref )
  { }

protected:
  opts::parse_status do_parse( sim_t*, util::string_view n, util::string_view v ) const override
  {
    std::string::size_type last = n.size() - 1;
    bool append = false;
    if ( n[ last ] == '+' )
    {
      append = true;
      --last;
    }
    auto dot = n.rfind( ".", last );
    if ( dot != std::string::npos )
    {
      if ( name() == n.substr( 0, dot + 1 ) )
      {
        auto listname = n.substr( dot + 1, last - dot );
        if ( listname.empty() )
        {
          return opts::parse_status::FAILURE;
        }

        auto& vec = _ref[ std::string(listname) ];
        if ( ! append )
        {
          vec.clear();
        }

        if ( !v.empty() )
        {
          vec.emplace_back( v );
        }

        return opts::parse_status::OK;
      }
    }
    return opts::parse_status::CONTINUE;
  }

  void do_format_to( fmt::format_context::iterator out ) const override
  {
    for ( auto& entry : _ref )
    {
      for ( auto i = 0U; i < entry.second.size(); ++i )
      {
        fmt::format_to( out, "{}{}{}{}\n", name(), entry.first, i == 0 ? "=" : "+=", entry.second[ i ] );
      }
    }
  }

  opts::map_list_t& _ref;
};

struct opts_list_t : public option_t
{
  opts_list_t( util::string_view name, opts::list_t& ref ) :
    option_t( name ),
    _ref( ref )
  { }
protected:
  opts::parse_status do_parse( sim_t*, util::string_view n, util::string_view v ) const override
  {
    if ( name() != n )
      return opts::parse_status::CONTINUE;

    _ref.emplace_back( v );

    return opts::parse_status::OK;
  }
  void do_format_to( fmt::format_context::iterator out ) const override
  {
    fmt::format_to( out, "{}=", name() );
    for ( auto& entry : _ref )
      fmt::format_to( out, "{}{} ", name(), entry );
    fmt::format_to( out, "\n" );
  }
private:
  opts::list_t& _ref;
};

struct opts_deperecated_t : public option_t
{
  opts_deperecated_t( util::string_view name, util::string_view new_option ) :
    option_t( name ),
    _new_option( new_option )
  { }
protected:
  opts::parse_status do_parse( sim_t*, util::string_view name, util::string_view ) const override
  {
    if ( name != this -> name() )
      return opts::parse_status::CONTINUE;
    throw std::invalid_argument( fmt::format(
      "Option '{}' has been deprecated. Please use option '{}' instead.", name, _new_option ) );
    return opts::parse_status::DEPRECATED;
  }
  void do_format_to( fmt::format_context::iterator out ) const override
  {
    fmt::format_to( out, "Option '{}' has been deprecated. Please use '{}'.\n", name(), _new_option );
  }
private:
  std::string _new_option;
};

struct opts_obsoleted_t : public option_t
{
  opts_obsoleted_t( util::string_view name ) :
    option_t( name )
  { }
protected:
  opts::parse_status do_parse( sim_t*, util::string_view name, util::string_view ) const override
  {
    if ( name != this -> name() )
      return opts::parse_status::CONTINUE;

    fmt::print( std::cerr, "Option '{}' has been obsoleted and will be removed in the future.", name );
    std::cerr << std::endl;
    return opts::parse_status::OK;
  }

  void do_format_to( fmt::format_context::iterator ) const override
  { 
    // do nothing
  }
};

} // opts

opts::parse_status option_t::parse( sim_t* sim, util::string_view name, util::string_view value ) const
{
  try {
    return do_parse( sim, name, value );
  }
  catch ( const std::exception& ) {
    std::throw_with_nested( std::runtime_error( fmt::format( "Option '{}' with value '{}'", name, value ) ) );
  }
}

void format_to( const option_t& option, fmt::format_context::iterator out )
{
  option.do_format_to( out );
}

// option_t::parse ==========================================================

opts::parse_status opts::parse( sim_t*                                      sim,
                                util::span<const std::unique_ptr<option_t>> options,
                                util::string_view                           name,
                                util::string_view                          value,
                                const parse_status_fn_t&                    status_fn )
{
  for ( auto& option : options )
  {
    auto ret = option->parse( sim, name, value );
    if ( ret != parse_status::CONTINUE )
    {
      if ( status_fn )
      {
        ret = status_fn( ret, name, value );
      }
      return ret;
    }
  }

  auto ret = parse_status::NOT_FOUND;
  if ( status_fn )
  {
    ret = status_fn( parse_status::NOT_FOUND, name, value );
  }

  return ret;
}

// option_t::parse ==========================================================

void opts::parse( sim_t* sim, util::string_view /* context */, util::span<const std::unique_ptr<option_t>> options,
                  util::string_view options_str, const parse_status_fn_t& status_fn )
{
  for ( auto& split : util::string_split<util::string_view>( options_str, "," ) )
  {
    auto index = split.find_first_of( '=' );

    if ( index == std::string::npos )
    {
      throw std::invalid_argument( fmt::format( "Unexpected parameter '{}'. Expected format: name=value", split ) );
    }

    auto name  = split.substr( 0, index );
    auto value = split.substr( index + 1 );

    auto status = opts::parse( sim, options, name, value, status_fn );

    if ( status == parse_status::FAILURE )
    {
      throw std::invalid_argument( fmt::format( "Unexpected parameter '{}'.", name ) );
    }
  }
}

// option_db_t::parse_file ==================================================

bool option_db_t::parse_file( std::istream& input )
{
  std::string buffer;
  bool first = true;
  while ( input.good() )
  {
    std::getline( input, buffer );
    auto it = buffer.begin();
    auto end = buffer.end();
    if ( first )
    {
      first = false;

      // Skip the UTF-8 BOM, if any.
      if ( utf8::starts_with_bom( it, end ) )
      {
        it += range::size( utf8::bom );
      }
    }

    while ( it != end && is_white_space( *it ) )
    {
      ++it;
    }

    if ( it == end || *it == '#' )
    {
      continue;
    }

    auto len = std::distance( buffer.begin(), it );
    assert(len >= 0 && as<size_t>(len) <= buffer.size());
    auto substring = buffer.substr(len);
    parse_line( io::maybe_latin1_to_utf8( substring ) );
  }
  return true;
}

// option_db_t::parse_text ==================================================

void option_db_t::parse_text( util::string_view text )
{
  // Split a chunk of text into lines to parse.
  std::string::size_type first = 0;

  while ( true )
  {
    while ( first < text.size() && is_white_space( text[ first ] ) )
    {
      ++first;
    }

    if ( first >= text.size() )
    {
      break;
    }

    auto last = text.find( '\n', first );
    if ( text[ first ] != '#' )
    {
      parse_line( text.substr( first, last - first ) );
    }

    first = last;
  }

}

// option_db_t::parse_line ==================================================

void option_db_t::parse_line( util::string_view line )
{
  if ( line[ 0 ] == '#' )
  {
    return;
  }

  auto tokens = util::string_split_allow_quotes( line, " \t\n\r" );

  for( const auto& token : tokens )
  {
    parse_token( token );
  }

}

// option_db_t::parse_token =================================================

void option_db_t::parse_token( util::string_view token )
{
  if ( token == "-" )
  {
    parse_file( std::cin );
    return;
  }

  std::string parsed_token = std::string( token );
  std::string::size_type cut_pt = parsed_token.find( '=' );

  // Expand the token with template variables, and try to find '=' again
  if ( cut_pt == std::string::npos )
  {
    do_replace( *this, parsed_token, parsed_token.find( "$(" ), 1 );

    cut_pt = parsed_token.find( '=' );
  }

  if ( cut_pt == token.npos )
  {
    std::string actual_name;
    io::ifstream input;
    open_file( input, auto_path, parsed_token, actual_name );
    if ( ! input.is_open() )
    {
      throw std::invalid_argument( fmt::format("Unexpected parameter '{}'. Expected format: name=value", parsed_token) );
    }
    parse_file( input );
    return;
  }

  std::string name( parsed_token, 0, cut_pt );
  std::string value( parsed_token, cut_pt + 1, std::string::npos );

  do_replace( *this, value, value.find( "$(" ), 1 );

  if ( !name.empty() && name[ 0 ] == '$' )
  {
    if ( name.size() < 3 || name[ 1 ] != '(' || name[ name.size() - 1 ] != ')' )
    {
      throw std::invalid_argument( fmt::format( "Variable syntax error: '{}", parsed_token ) );
    }
    auto var_name = name.substr( 2, name.size() - 3 );
    do_replace( *this, var_name, var_name.find( "$(" ), 1 );
    var_map[ var_name ] = value;
  }
  else if ( name == "input" )
  {
    std::string current_base_name;
    auto base_name_it = var_map.find( "current_base_name" );
    if ( base_name_it != var_map.end() )
    {
      current_base_name = base_name_it -> second;
    }

    std::string actual_name;
    io::ifstream input;
    open_file( input, auto_path, value, actual_name );
    if ( ! input.is_open() )
    {
      throw std::invalid_argument( fmt::format("Unable to open input parameter file '{}'.", value) );
    }
    else
    {
      var_map[ "current_base_name" ] = base_name( actual_name );
    }
    parse_file( input );

    if ( base_name_it != var_map.end() )
    {
      var_map[ "current_base_name" ] = current_base_name;
    }
  }
  else
  {
    // Replace any template variable in the name portion of the option, if found
    do_replace( *this, name, name.find( "$(" ), 1 );

    add( "global", name, value );
  }
}

// option_db_t::parse_args ==================================================

void option_db_t::parse_args( util::span<const std::string> args )
{
  for ( auto& arg : args )
  {
    parse_token( arg );
  }
}

// option_db_t::option_db_t =================================================

option_db_t::option_db_t()
{
  std::vector<std::string> paths = { "..", "./profiles", "../profiles", SC_SHARED_DATA };

  // This makes baby pandas cry a bit less, but still makes them weep.

  // Add current directory automagically.
  auto_path.emplace_back("." );

  // Automatically add "./profiles" and "../profiles", because the command line
  // client is ran both from the engine/ subdirectory, as well as the source
  // root directory, depending on whether the user issues make install or not.
  // In addition, if SC_SHARED_DATA is given, search our profile directory
  // structure directly from there as well.
  for ( auto path : paths )
  {
    // Skip empty SHARED_DATA define, as the default ("..") is already
    // included.
    if ( path.empty() )
      continue;

    // Skip current path, we arleady have that
    if ( path == "." )
      continue;

    auto_path.push_back( path );

    path += "/";

    // Add profiles that doesn't match the tier pattern
    auto_path.push_back( path + "generators" );
    auto_path.push_back( path + "generators/PreRaids" );
    auto_path.push_back( path + "PreRaids" );
    auto_path.push_back( path + "generators/DungeonSlice" );
    auto_path.push_back( path + "DungeonSlice" );

    // Add profiles for each tier
    for ( unsigned i = 0; i < N_TIER; ++i )
    {
      auto_path.push_back( fmt::format( "{}generators/Tier{}", path, MIN_TIER + i ) );
      auto_path.push_back( fmt::format( "{}Tier{}", path, MIN_TIER + i ) );
    }
  }

  // Bossevents
  for ( auto path : paths )
    {
      // Skip empty SHARED_DATA define, as the default ("..") is already
      // included.
      if ( path.empty() )
        continue;

      // Skip current path, we arleady have that
      if ( path == "." )
        continue;

      path += "/Bossevents";
      auto_path.push_back( path );

      path += "/";


      // Add bossevents for each tier
      for ( unsigned i = 0; i < N_TIER; ++i )
      {
        std::string base_tier_path = fmt::format( "{}T{}", path, MIN_TIER + i );
        auto_path.push_back( base_tier_path);
        auto_path.push_back( base_tier_path + "/Heroic" );
        auto_path.push_back( base_tier_path + "/Mythic" );
      }
    }

  // Make sure we only have unique entries
  auto it = std::unique(auto_path.begin(), auto_path.end());
  auto_path.resize( std::distance(auto_path.begin(), it) );
}

std::unique_ptr<option_t> opt_string( util::string_view n, std::string& v )
{ return std::unique_ptr<option_t>(new opt_string_t( n, v )); }

std::unique_ptr<option_t> opt_append( util::string_view n, std::string& v )
{ return std::unique_ptr<option_t>(new opt_append_t( n, v )); }

std::unique_ptr<option_t> opt_bool( util::string_view n, int& v )
{ return std::unique_ptr<option_t>(new opt_bool_int_t( n, v )); }

std::unique_ptr<option_t> opt_bool( util::string_view n, bool& v )
{ return std::unique_ptr<option_t>(new opt_bool_t( n, v )); }

std::unique_ptr<option_t> opt_uint64( util::string_view n, uint64_t& v )
{ return std::unique_ptr<option_t>(new opt_numeric_t<uint64_t, converter_uint64_t>( n, v )); }

std::unique_ptr<option_t> opt_int( util::string_view n, int& v )
{ return std::unique_ptr<option_t>(new opt_numeric_t<int, converter_int_t>( n, v )); }

std::unique_ptr<option_t> opt_int( util::string_view n, int& v, int min, int max )
{ return std::unique_ptr<option_t>(new opt_numeric_mm_t<int, converter_int_t>( n, v, min, max )); }

std::unique_ptr<option_t> opt_uint( util::string_view n, unsigned& v )
{ return std::unique_ptr<option_t>(new opt_numeric_t<unsigned, converter_uint_t>( n, v )); }

std::unique_ptr<option_t> opt_uint( util::string_view n, unsigned& v, unsigned min, unsigned max )
{ return std::unique_ptr<option_t>(new opt_numeric_mm_t<unsigned, converter_uint_t>( n, v, min, max )); }

std::unique_ptr<option_t> opt_float( util::string_view n, double& v )
{ return std::unique_ptr<option_t>(new opt_numeric_t<double, converter_double_t>( n, v )); }

std::unique_ptr<option_t> opt_float( util::string_view n, double& v, double min, double max )
{ return std::unique_ptr<option_t>(new opt_numeric_mm_t<double, converter_double_t>( n, v, min, max )); }

std::unique_ptr<option_t> opt_timespan( util::string_view n, timespan_t& v )
{ return std::unique_ptr<option_t>(new opt_numeric_t<timespan_t, converter_timespan_t>( n, v )); }

std::unique_ptr<option_t> opt_timespan( util::string_view n, timespan_t& v, timespan_t min, timespan_t max )
{ return std::unique_ptr<option_t>(new opt_numeric_mm_t<timespan_t, converter_timespan_t>( n, v, min, max )); }

std::unique_ptr<option_t> opt_list( util::string_view n, opts::list_t& v )
{ return std::unique_ptr<option_t>(new opts_list_t( n, v )); }

std::unique_ptr<option_t> opt_map( util::string_view n, opts::map_t& v )
{ return std::unique_ptr<option_t>(new opts_map_t( n, v )); }

std::unique_ptr<option_t> opt_map_list( util::string_view n, opts::map_list_t& v )
{ return std::unique_ptr<option_t>(new opts_map_list_t( n, v )); }

std::unique_ptr<option_t> opt_func( util::string_view n, const opts::function_t& f )
{ return std::unique_ptr<option_t>(new opts_sim_func_t( n, f )); }

std::unique_ptr<option_t> opt_deprecated( util::string_view n, util::string_view new_option )
{ return std::unique_ptr<option_t>(new opts_deperecated_t( n, new_option )); }

std::unique_ptr<option_t> opt_obsoleted( util::string_view n )
{ return std::unique_ptr<option_t>(new opts_obsoleted_t( n )); }
