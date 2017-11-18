// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_option.hpp"

#include <sstream>
#include <iostream>

#include "util/io.hpp"
#include "sc_util.hpp"

namespace { // UNNAMED NAMESPACE ============================================

bool is_white_space( char c )
{
  return ( c == ' ' || c == '\t' || c == '\n' || c == '\r' );
}

char* skip_white_space( char* s )
{
  while ( is_white_space( *s ) )
    ++s;
  return s;
}

io::cfile open_file( const std::vector<std::string>& splits, const std::string& name, std::string& actual_name )
{
  for ( auto& split : splits )
  {
    auto file_path = split + "/" + name;
    FILE* f = io::fopen( file_path, "r" );
    if ( f )
    {
      actual_name = file_path;
      return io::cfile(f);
    }
  }

  actual_name = name;
  return io::cfile( name, "r" );
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
    std::stringstream s;
    s << "Nesting depth exceeded for: '" << str << "' (max: " << max_depth << ")";
    throw std::invalid_argument( s.str() );
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

  auto end = str.find( ")", begin + 2 );
  if ( end == std::string::npos )
  {
    std::stringstream s;
    s << "Unbalanced parenthesis in template variable for: '" << str << "'";
    throw std::invalid_argument( s.str() );
  }

  auto var = str.substr( begin + 2, ( end - begin ) - 2 );
  if ( opts.var_map.find( var ) == opts.var_map.end() )
  {
    std::stringstream s;
    s << "Missing template variable: '" << var << "'";
    throw std::invalid_argument( s.str() );
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
  static uint64_t convert( const std::string& v )
  {
    return std::stoull( v );
  }
};

struct converter_int_t
{
  static int convert( const std::string& v )
  {
    return std::stoi( v );
  }
};

struct converter_uint_t
{
  static unsigned int convert( const std::string& v )
  {
    return std::stoul( v );
  }
};

struct converter_double_t
{
  static double convert( const std::string& v )
  {
    return std::stod( v );
  }
};

struct converter_timespan_t
{
  static timespan_t convert( const std::string& v )
  {
    return timespan_t::from_seconds( std::stod( v ) );
  }
};

/* Option helper class
 * Stores a reference of given type T
 */
template<class T>
struct opts_helper_t : public option_t
{
  typedef opts_helper_t<T> base_t;

  opts_helper_t( const std::string& name, T& ref ) :
    option_t( name ),
    _ref( ref )
  { }
protected:
  T& _ref;
};

struct opt_string_t : public option_t
{
  opt_string_t( const std::string& name, std::string& addr ) :
    option_t( name ),
    _ref( addr )
  { }
protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( n != name() )
      return false;

    _ref = v;
    return true;
  }

  std::ostream& print( std::ostream& stream ) const override
  {
     stream << name() << "="  <<  _ref << "\n";
     return stream;
  }
private:
  std::string& _ref;
};

struct opt_append_t : public option_t
{
  opt_append_t( const std::string& name, std::string& addr ) :
    option_t( name ),
    _ref( addr )
  { }

protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( n != name() )
      return false;

    _ref += v;
    return true;
  }

  std::ostream& print( std::ostream& stream ) const override
  {
     stream << name() << "+="  <<  _ref << "\n";
     return stream;
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
  opt_numeric_t( const std::string& name, T& addr ) :
    option_t( name ),
    _ref( addr )
  { }
protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( n != name() )
      return false;

    if ( v.empty() )
      _ref = {};
    else
      _ref = Converter::convert( v );

    return true;
  }

  std::ostream& print( std::ostream& stream ) const override
  {
     stream << name() << "="  <<  _ref << "\n";
     return stream;
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
  opt_numeric_mm_t( const std::string& name, T& addr, T min, T max ) :
    option_t( name ),
    _ref( addr ),
    _min( min ),
    _max( max )
  { }

protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( n != name() )
      return false;

    T tmp;
    if ( v.empty() )
      tmp = {};
    else
      tmp = Converter::convert( v );

    // Range checking
    if ( tmp < _min || tmp > _max ) {
      std::stringstream s;
      s << "Option '" << n << "' with value '" << v
          << "' not within valid boundaries [" << _min << " - " << _max << "]";
      throw std::invalid_argument(s.str());
    }
    _ref = tmp;
    return true;
  }

  std::ostream& print( std::ostream& stream ) const override
  {
     stream << name() << "="  <<  _ref << "\n";
     return stream;
  }
private:
  T& _ref;
  T _min, _max;
};

struct opt_bool_t : public option_t
{
  opt_bool_t( const std::string& name, bool& addr ) :
    option_t( name ),
    _ref( addr )
  { }
protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( n != name() )
      return false;

    if ( v != "0" && v != "1" )
    {
      std::stringstream s;
      s << "Acceptable values for '" << n << "' are '1' or '0'";
      throw std::invalid_argument( s.str() );
    }

    _ref = std::stoi( v ) != 0;
    return true;
  }
  std::ostream& print( std::ostream& stream ) const override
  {
     stream << name() << "="  <<  _ref << "\n";
     return stream;
  }
private:
  bool& _ref;
};

struct opt_bool_int_t : public option_t
{
  opt_bool_int_t( const std::string& name, int& addr ) :
    option_t( name ),
    _ref( addr )
  { }
protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( n != name() )
      return false;

    if ( v != "0" && v != "1" )
    {
      std::stringstream s;
      s << "Acceptable values for '" << n << "' are '1' or '0'";
      throw std::invalid_argument( s.str() );
    }

    _ref = std::stoi( v );
    return true;
  }
  std::ostream& print( std::ostream& stream ) const override
  {
     stream << name() << "="  <<  _ref << "\n";
     return stream;
  }
private:
  int& _ref;
};

struct opts_sim_func_t : public option_t
{
  opts_sim_func_t( const std::string& name, const opts::function_t& ref ) :
    option_t( name ),
    _fun( ref )
  { }
protected:
  bool parse( sim_t* sim, const std::string& n, const std::string& value ) const override
  {
    if ( name() != n )
      return false;

     return _fun( sim, n, value );
  }
  std::ostream& print( std::ostream& stream ) const override
  { return stream << "function option: " << name() << "\n"; }
private:
  opts::function_t _fun;
};

struct opts_map_t : public option_t
{
  opts_map_t( const std::string& name, opts::map_t& ref ) :
    option_t( name ),
    _ref( ref )
  { }
protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    std::string::size_type last = n.size() - 1;
    bool append = false;
    if ( n[ last ] == '+' )
    {
      append = true;
      --last;
    }
    std::string::size_type dot = n.rfind( ".", last );
    if ( dot != std::string::npos )
    {
      if ( name() == n.substr( 0, dot + 1 ) )
      {
        std::string& value = _ref[ n.substr( dot + 1, last - dot ) ];
        value = append ? ( value + v ) : v;
        return true;
      }
    }
    return false;
  }
  std::ostream& print( std::ostream& stream ) const override
  {
    for ( const auto& entry : _ref )
          stream << name() << entry.first << "="<< entry.second << "\n";
     return stream;
  }
  opts::map_t& _ref;
};

struct opts_map_list_t : public option_t
{
  opts_map_list_t( const std::string& name, opts::map_list_t& ref ) :
    option_t( name ), _ref( ref )
  { }

protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( v.empty() )
    {
      return false;
    }

    std::string::size_type last = n.size() - 1;
    bool append = false;
    if ( n[ last ] == '+' )
    {
      append = true;
      --last;
    }
    std::string::size_type dot = n.rfind( ".", last );
    if ( dot != std::string::npos )
    {
      if ( name() == n.substr( 0, dot + 1 ) )
      {
        auto listname = n.substr( dot + 1, last - dot );
        auto& vec = _ref[ listname ];
        if ( ! append )
        {
          vec.clear();
        }

        vec.push_back( v );
        return true;
      }
    }
    return false;
  }

  std::ostream& print( std::ostream& stream ) const override
  {
    for ( auto& entry : _ref )
    {
      for ( auto i = 0u; i < entry.second.size(); ++i )
      {
        stream << name() << entry.first;

        if ( i == 0 )
        {
          stream << "=";
        }
        else
        {
          stream << "+=";
        }

        stream << entry.second[ i ] << "\n";
      }
    }
    return stream;
  }

  opts::map_list_t& _ref;
};

struct opts_list_t : public option_t
{
  opts_list_t( const std::string& name, opts::list_t& ref ) :
    option_t( name ),
    _ref( ref )
  { }
protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( name() != n )
      return false;

    _ref.push_back( v );

    return false;
  }
  std::ostream& print( std::ostream& stream ) const override
  {
    stream << name() << "=";
    for ( auto& entry : _ref )
    {
      stream << name() << entry << " ";
    }
    stream << "\n";
    return stream;
  }
private:
  opts::list_t& _ref;
};

struct opts_deperecated_t : public option_t
{
  opts_deperecated_t( const std::string& name, const std::string& new_option ) :
    option_t( name ),
    _new_option( new_option )
  { }
protected:
  bool parse( sim_t*, const std::string& name, const std::string& ) const override
  {
    if ( name != this -> name() )
      return false;
    std::stringstream s;
    s << "Option '" << name << "' has been deprecated.";
    s << " Please use option '" << _new_option << "' instead.";
    throw std::invalid_argument( s.str() );
    return false;
  }
  std::ostream& print( std::ostream& stream ) const override
  {
    stream << "Option '" << name() << "' has been deprecated. Please use '" << _new_option << "'.\n";
    return stream;
  }
private:
  std::string _new_option;
};

} // opts

// option_t::parse ==========================================================

bool opts::parse( sim_t*                 sim,
                  const std::vector<std::unique_ptr<option_t>>& options,
                      const std::string&     name,
                      const std::string&     value )
{
  for ( auto& option : options )
  {
    if ( option -> parse_option( sim, name, value ) )
    {
      return true;
    }
  }

  return false;
}

// option_t::parse ==========================================================

void opts::parse( sim_t*                 sim,
    const std::string&            context,
    const std::vector<std::unique_ptr<option_t>>& options,
                      const std::vector<std::string>& splits )
{
  for (auto & s : splits)
  {
    
    auto index = s.find_first_of( '=' );

    if ( index == std::string::npos )
    {
      std::stringstream stream;
      stream << context << ": Unexpected parameter '" << s << "'. Expected format: name=value";
      throw std::invalid_argument( stream.str() );
    }

    std::string n = s.substr( 0, index );
    std::string v = s.substr( index + 1 );

    if ( ! opts::parse( sim, options, n, v ) )
    {
      std::stringstream stream;
      stream << context << ": Unexpected parameter '" << n << "'.";
      throw std::invalid_argument( stream.str() );
    }
  }
}

// option_t::parse ==========================================================

void opts::parse( sim_t*                 sim,
    const std::string&            context,
    const std::vector<std::unique_ptr<option_t>>& options,
                      const std::string&     options_str )
{
  opts::parse( sim, context, options, util::string_split( options_str, "," ) );
}


// option_db_t::parse_file ==================================================

bool option_db_t::parse_file( FILE* file )
{
  std::array<char, 1024> buffer;
  bool first = true;
  while ( fgets( buffer.data(), buffer.size(), file ) )
  {
    char* b = buffer.data();
    if ( first )
    {
      first = false;

      // Skip the UTF-8 BOM, if any.
      size_t len = strlen( b );
      if ( len >= 3 && utf8::is_bom( b ) )
      {
        b += 3;
      }
    }

    b = skip_white_space( b );
    if ( *b == '#' || *b == '\0' )
    {
      continue;
    }

    parse_line( io::maybe_latin1_to_utf8( b ) );
  }
  return true;
}

// option_db_t::parse_text ==================================================

void option_db_t::parse_text( const std::string& text )
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
    if ( false )
    {
      std::cerr << "first = " << first << ", last = " << last << " ["
                << text.substr( first, last - first ) << ']' << std::endl;
    }
    if ( text[ first ] != '#' )
    {
      parse_line( text.substr( first, last - first ) );
    }

    first = last;
  }

}

// option_db_t::parse_line ==================================================

void option_db_t::parse_line( const std::string& line )
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

void option_db_t::parse_token( const std::string& token )
{
  if ( token == "-" )
  {
    parse_file( stdin );
    return;
  }

  std::string parsed_token = token;
  std::string::size_type cut_pt = parsed_token.find( '=' );

  // Expand the token with template variables, and try to find '=' again
  if ( cut_pt == std::string::npos )
  {
    do_replace( *this, parsed_token, parsed_token.find( "$(" ), 1 );

    cut_pt = parsed_token.find( "=" );
  }

  if ( cut_pt == token.npos )
  {
    std::string actual_name;
    io::cfile file = open_file( auto_path, parsed_token, actual_name );
    if ( ! file )
    {
      std::stringstream s;
      s << "Unexpected parameter '" << parsed_token << "'. Expected format: name=value";
      throw std::invalid_argument( s.str() );
    }
    parse_file( file );
    return;
  }

  std::string name( parsed_token, 0, cut_pt ), value( parsed_token, cut_pt + 1, std::string::npos );

  do_replace( *this, value, value.find( "$(" ), 1 );

  if ( name.size() >= 1 && name[ 0 ] == '$' )
  {
    if ( name.size() < 3 || name[ 1 ] != '(' || name[ name.size() - 1 ] != ')' )
    {
      std::stringstream s;
      s << "Variable syntax error: '" << parsed_token << "'";
      throw std::invalid_argument( s.str() );
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
    io::cfile file( open_file( auto_path, value, actual_name ) );
    if ( ! file )
    {
      std::stringstream s;
      s << "Unable to open input parameter file '" << value << "'";
      throw std::invalid_argument( s.str() );
    }
    else
    {
      var_map[ "current_base_name" ] = base_name( actual_name );
    }
    parse_file( file );

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

void option_db_t::parse_args( const std::vector<std::string>& args )
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
  auto_path.push_back( "." );

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

    // Add profiles for each tier
    for ( unsigned i = 0; i < N_TIER; ++i )
    {
      auto_path.push_back( path + "generators/Tier" + util::to_string( MIN_TIER + i ) );
      auto_path.push_back( path + "Tier" + util::to_string( MIN_TIER + i ) );
    }
  }

  // Make sure we only have unique entries
  auto it = std::unique(auto_path.begin(), auto_path.end());
  auto_path.resize( std::distance(auto_path.begin(), it) );
}

std::unique_ptr<option_t> opt_string( const std::string& n, std::string& v )
{ return std::unique_ptr<option_t>(new opt_string_t( n, v )); }

std::unique_ptr<option_t> opt_append( const std::string& n, std::string& v )
{ return std::unique_ptr<option_t>(new opt_append_t( n, v )); }

std::unique_ptr<option_t> opt_bool( const std::string& n, int& v )
{ return std::unique_ptr<option_t>(new opt_bool_int_t( n, v )); }

std::unique_ptr<option_t> opt_bool( const std::string& n, bool& v )
{ return std::unique_ptr<option_t>(new opt_bool_t( n, v )); }

std::unique_ptr<option_t> opt_uint64( const std::string& n, uint64_t& v )
{ return std::unique_ptr<option_t>(new opt_numeric_t<uint64_t, converter_uint64_t>( n, v )); }

std::unique_ptr<option_t> opt_int( const std::string& n, int& v )
{ return std::unique_ptr<option_t>(new opt_numeric_t<int, converter_int_t>( n, v )); }

std::unique_ptr<option_t> opt_int( const std::string& n, int& v, int min, int max )
{ return std::unique_ptr<option_t>(new opt_numeric_mm_t<int, converter_int_t>( n, v, min, max )); }

std::unique_ptr<option_t> opt_uint( const std::string& n, unsigned& v )
{ return std::unique_ptr<option_t>(new opt_numeric_t<unsigned, converter_uint_t>( n, v )); }

std::unique_ptr<option_t> opt_uint( const std::string& n, unsigned& v, unsigned min, unsigned max )
{ return std::unique_ptr<option_t>(new opt_numeric_mm_t<unsigned, converter_uint_t>( n, v, min, max )); }

std::unique_ptr<option_t> opt_float( const std::string& n, double& v )
{ return std::unique_ptr<option_t>(new opt_numeric_t<double, converter_double_t>( n, v )); }

std::unique_ptr<option_t> opt_float( const std::string& n, double& v, double min, double max )
{ return std::unique_ptr<option_t>(new opt_numeric_mm_t<double, converter_double_t>( n, v, min, max )); }

std::unique_ptr<option_t> opt_timespan( const std::string& n, timespan_t& v )
{ return std::unique_ptr<option_t>(new opt_numeric_t<timespan_t, converter_timespan_t>( n, v )); }

std::unique_ptr<option_t> opt_timespan( const std::string& n, timespan_t& v, timespan_t min, timespan_t max )
{ return std::unique_ptr<option_t>(new opt_numeric_mm_t<timespan_t, converter_timespan_t>( n, v, min, max )); }

std::unique_ptr<option_t> opt_list( const std::string& n, opts::list_t& v )
{ return std::unique_ptr<option_t>(new opts_list_t( n, v )); }

std::unique_ptr<option_t> opt_map( const std::string& n, opts::map_t& v )
{ return std::unique_ptr<option_t>(new opts_map_t( n, v )); }

std::unique_ptr<option_t> opt_map_list( const std::string& n, opts::map_list_t& v )
{ return std::unique_ptr<option_t>(new opts_map_list_t( n, v )); }

std::unique_ptr<option_t> opt_func( const std::string& n, const opts::function_t& f )
{ return std::unique_ptr<option_t>(new opts_sim_func_t( n, f )); }

std::unique_ptr<option_t> opt_deprecated( const std::string& n, const std::string& new_option )
{ return std::unique_ptr<option_t>(new opts_deperecated_t( n, new_option )); }
