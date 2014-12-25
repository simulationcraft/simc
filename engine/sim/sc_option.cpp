// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE ============================================

// is_white_space ===========================================================

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

// option_db_t::open_file ===================================================

io::cfile open_file( const std::vector<std::string>& splits, const std::string& name )
{
  for ( size_t i = 0; i < splits.size(); i++ )
  {
    FILE* f = io::fopen( splits[ i ] + "/" + name, "r" );
    if ( f )
      return io::cfile(f);
  }

  return io::cfile( name, "r" );
}

} // UNNAMED NAMESPACE ======================================================

namespace opts {

/* Option helper class
 * Stores a reference of given type T
 */
template<class T>
struct opts_helper_t : public option_base_t
{
  typedef opts_helper_t<T> base_t;
  opts_helper_t( const std::string& name, T& ref ) :
    option_base_t( name ),
    _ref( ref )
  { }
protected:
  T& _ref;
};

struct opt_string_t : public option_base_t
{
  opt_string_t( const std::string& name, std::string& addr ) :
    option_base_t( name ),
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

struct opt_append_t : public option_base_t
{
  opt_append_t( const std::string& name, std::string& addr ) :
    option_base_t( name ),
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

struct opt_uint64_t : public option_base_t
{
  opt_uint64_t( const std::string& name, uint64_t& addr ) :
    option_base_t( name ),
    _ref( addr )
  { }
protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( n != name() )
      return false;
#if defined( SC_WINDOWS ) && defined( SC_VS )
    _ref = _strtoui64( v.c_str(), nullptr, 10 );
#else
    _ref = strtoull( v.c_str(), nullptr, 10 );
#endif
    return true;
  }
  std::ostream& print( std::ostream& stream ) const override
  {
     stream << name() << "="  <<  _ref << "\n";
     return stream;
  }
private:
  uint64_t& _ref;
};

struct opt_int_t : public option_base_t
{
  opt_int_t( const std::string& name, int& addr ) :
    option_base_t( name ),
    _ref( addr )
  { }
protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( n != name() )
      return false;

    _ref = strtol( v.c_str(), nullptr, 10 );
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

struct opt_int_mm_t : public option_base_t
{
  opt_int_mm_t( const std::string& name, int& addr, int min, int max ) :
    option_base_t( name ),
    _ref( addr ),
    _min( min ),
    _max( max )
  { }
protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( n != name() )
      return false;

    int tmp = strtol( v.c_str(), nullptr, 10 );
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
  int& _ref;
  int _min, _max;
};

struct opt_uint_t : public option_base_t
{
  opt_uint_t( const std::string& name, unsigned int& addr ) :
    option_base_t( name ),
    _ref( addr )
  { }
protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( n != name() )
      return false;

    _ref = strtoul( v.c_str(), nullptr, 10 );
    return true;
  }
  std::ostream& print( std::ostream& stream ) const override
  {
     stream << name() << "="  <<  _ref << "\n";
     return stream;
  }
private:
  unsigned int& _ref;
};

struct opt_uint_mm_t : public option_base_t
{
  opt_uint_mm_t( const std::string& name, unsigned int& addr, unsigned int min, unsigned int max ) :
    option_base_t( name ),
    _ref( addr ),
    _min( min ),
    _max( max )
  { }
protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( n != name() )
      return false;

    unsigned int tmp = strtoul( v.c_str(), nullptr, 10 );
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
  unsigned int& _ref;
  unsigned int _min, _max;
};

struct opt_double_t : public option_base_t
{
  opt_double_t( const std::string& name, double& addr ) :
    option_base_t( name ),
    _ref( addr )
  { }
protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( n != name() )
      return false;

    _ref = strtod( v.c_str(), nullptr );
    return true;
  }
  std::ostream& print( std::ostream& stream ) const override
  {
     stream << name() << "="  <<  _ref << "\n";
     return stream;
  }
private:
  double& _ref;
};

struct opt_double_mm_t : public option_base_t
{
  opt_double_mm_t( const std::string& name, double& addr, double min, double max ) :
    option_base_t( name ),
    _ref( addr ),
    _min( min ),
    _max( max )
  { }
protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( n != name() )
      return false;

    double tmp = strtod( v.c_str(), nullptr );
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
  double& _ref;
  double _min, _max;
};

struct opt_timespan_t : public option_base_t
{
  opt_timespan_t( const std::string& name, timespan_t& addr ) :
    option_base_t( name ),
    _ref( addr )
  { }
protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( n != name() )
      return false;

    _ref = timespan_t::from_seconds( strtod( v.c_str(), nullptr ) );
    return true;
  }
  std::ostream& print( std::ostream& stream ) const override
  {
     stream << name() << "="  <<  _ref << "\n";
     return stream;
  }
private:
  timespan_t& _ref;
};

struct opt_timespan_mm_t : public option_base_t
{
  opt_timespan_mm_t( const std::string& name, timespan_t& addr, timespan_t min, timespan_t max ) :
    option_base_t( name ),
    _ref( addr ),
    _min( min ),
    _max( max )
  { }
protected:
  bool parse( sim_t*, const std::string& n, const std::string& v ) const override
  {
    if ( n != name() )
      return false;

    timespan_t tmp = timespan_t::from_seconds( strtod( v.c_str(), nullptr ) );
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
  timespan_t& _ref;
  timespan_t _min, _max;
};

struct opt_bool_t : public option_base_t
{
  opt_bool_t( const std::string& name, bool& addr ) :
    option_base_t( name ),
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

    _ref = strtol( v.c_str(), nullptr, 10 ) != 0;
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

struct opt_bool_int_t : public option_base_t
{
  opt_bool_int_t( const std::string& name, int& addr ) :
    option_base_t( name ),
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

    _ref = strtol( v.c_str(), nullptr, 10 );
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

struct opts_sim_func_t : public option_base_t
{
  opts_sim_func_t( const std::string& name, const function_t& ref ) :
    option_base_t( name ),
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
  function_t _fun;
};

struct opts_map_t : public option_base_t
{
  opts_map_t( const std::string& name, map_t& ref ) :
    option_base_t( name ),
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
    for ( map_t::const_iterator it = _ref.begin(), end = _ref.end(); it != end; ++it )
          stream << name() << it->first << "="<< it->second << "\n";
     return stream;
  }
  map_t& _ref;
};

struct opts_list_t : public option_base_t
{
  opts_list_t( const std::string& name, list_t& ref ) :
    option_base_t( name ),
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
    for ( list_t::const_iterator it = _ref.begin(), end = _ref.end(); it != end; ++it )
          stream << name() << (*it) << " ";
    stream << "\n";
    return stream;
  }
private:
  list_t& _ref;
};

struct opts_deperecated_t : public option_base_t
{
  opts_deperecated_t( const std::string& name, const std::string& new_option ) :
    option_base_t( name ),
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
                  const std::vector<option_t>& options,
                      const std::string&     name,
                      const std::string&     value )
{
  size_t num_options = options.size();

  for ( size_t i = 0; i < num_options; i++ )
    if ( options[ i ] -> parse_option( sim, name, value ) )
      return true;

  return false;
}

// option_t::parse ==========================================================

void opts::parse( sim_t*                 sim,
    const std::string&            context,
    const std::vector<option_t>& options,
                      const std::vector<std::string>& splits )
{
  for ( size_t i = 0, size = splits.size(); i < size; ++i )
  {
    const std::string& s = splits[ i ];
    std::string::size_type index = s.find( '=' );

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
    const std::vector<option_t>& options,
                      const std::string&     options_str )
{
  opts::parse( sim, context, options, util::string_split( options_str, "," ) );
}


// option_db_t::parse_file ==================================================

bool option_db_t::parse_file( FILE* file )
{
  char buffer[ 1024 ];
  bool first = true;
  while ( fgets( buffer, sizeof( buffer ), file ) )
  {
    char *b = buffer;
    if ( first )
    {
      first = false;

      // Skip the UTF-8 BOM, if any.
      size_t len = strlen( b );
      if ( len >= 3 && utf8::is_bom( b ) )
        b += 3;
    }

    b = skip_white_space( b );
    if ( *b == '#' || *b == '\0' )
      continue;

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
      ++first;

    if ( first >= text.size() )
      break;

    std::string::size_type last = text.find( '\n', first );
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
    return;

  std::vector<std::string> tokens;
  size_t num_tokens = util::string_split_allow_quotes( tokens, line, " \t\n\r" );

  for ( size_t i = 0; i < num_tokens; ++i )
  {
    parse_token( tokens[ i ] );
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

  std::string::size_type cut_pt = token.find( '=' );

  if ( cut_pt == token.npos )
  {
    io::cfile file = open_file( auto_path, token );
    if ( ! file )
    {
      std::stringstream s;
      s << "Unexpected parameter '" << token << "'. Expected format: name=value";
      throw std::invalid_argument( s.str() );
    }
    parse_file( file );
    return;
  }

  std::string name( token, 0, cut_pt ), value( token, cut_pt + 1, token.npos );

  std::string::size_type start = 0;
  while ( ( start = value.find( "$(", start ) ) != std::string::npos )
  {
    std::string::size_type end = value.find( ')', start );
    if ( end == std::string::npos )
    {
      std::stringstream s;
      s << "Variable syntax error: '" << token << "'";
      throw std::invalid_argument( s.str() );
    }

    value.replace( start, ( end - start ) + 1,
                   var_map[ value.substr( start + 2, ( end - start ) - 2 ) ] );
  }

  if ( name.size() >= 1 && name[ 0 ] == '$' )
  {
    if ( name.size() < 3 || name[ 1 ] != '(' || name[ name.size() - 1 ] != ')' )
    {
      std::stringstream s;
      s << "Variable syntax error: '" << token << "'";
      throw std::invalid_argument( s.str() );
    }
    std::string var_name( name, 2, name.size() - 3 );
    var_map[ var_name ] = value;
  }
  else if ( name == "input" )
  {
    io::cfile file( open_file( auto_path, value ) );
    if ( ! file )
    {
      std::stringstream s;
      s << "Unable to open input parameter file '" << value << "'";
      throw std::invalid_argument( s.str() );
    }
    parse_file( file );
  }
  else
  {
    add( "global", name, value );
  }
}

// option_db_t::parse_args ==================================================

void option_db_t::parse_args( const std::vector<std::string>& args )
{
  for ( size_t i = 0; i < args.size(); ++i )
    parse_token( args[ i ] );
}

// option_db_t::option_db_t =================================================

#ifndef SC_SHARED_DATA
  #if defined( SC_LINUX_PACKAGING )
    #define SC_SHARED_DATA SC_LINUX_PACKAGING "/profiles"
  #else
    #define SC_SHARED_DATA ".."
  #endif
#endif

option_db_t::option_db_t()
{
  const char* paths[] = { "./profiles", "../profiles", SC_SHARED_DATA };
  int n_paths = 2;
  if ( ! util::str_compare_ci( SC_SHARED_DATA, ".." ) )
    n_paths++;

  // This makes baby pandas cry a bit less, but still makes them weep.

  // Add current directory automagically.
  auto_path.push_back( "." );

  // Automatically add "./profiles" and "../profiles", because the command line
  // client is ran both from the engine/ subdirectory, as well as the source
  // root directory, depending on whether the user issues make install or not.
  // In addition, if SC_SHARED_DATA is given, search our profile directory
  // structure directly from there as well.
  for ( int j = 0; j < n_paths; j++ )
  {
    std::string prefix = paths[ j ];
    // Skip empty SHARED_DATA define, as the default ("..") is already
    // included.
    if ( prefix.empty() )
      continue;

    auto_path.push_back( prefix );

    prefix += "/";

    // Add profiles for each tier, except pvp
    for ( int i = 0; i < ( N_TIER - 1 ); i++ )
    {
      auto_path.push_back( prefix + "Tier" + util::to_string( MIN_TIER + i ) + "B" );
      auto_path.push_back( prefix + "Tier" + util::to_string( MIN_TIER + i ) + "H" );
      auto_path.push_back( prefix + "Tier" + util::to_string( MIN_TIER + i ) + "N" );
      auto_path.push_back( prefix + "Tier" + util::to_string( MIN_TIER + i ) + "M" );
      auto_path.push_back( prefix + "Tier" + util::to_string( MIN_TIER + i ) + "P" );
    }
  }
}

option_t opt_string( const std::string& n, std::string& v )
{ return new opts::opt_string_t( n, v ); }

option_t opt_append( const std::string& n, std::string& v )
{ return new opts::opt_append_t( n, v ); }

option_t opt_bool( const std::string& n, int& v )
{ return new opts::opt_bool_int_t( n, v ); }

option_t opt_bool( const std::string& n, bool& v )
{ return new opts::opt_bool_t( n, v ); }

option_t opt_uint64( const std::string& n, uint64_t& v )
{ return new opts::opt_uint64_t( n, v ); }

option_t opt_int( const std::string& n, int& v )
{ return new opts::opt_int_t( n, v ); }

option_t opt_int( const std::string& n, int& v, int min, int max )
{ return new opts::opt_int_mm_t( n, v, min, max ); }

option_t opt_uint( const std::string& n, unsigned& v )
{ return new opts::opt_uint_t( n, v ); }

option_t opt_uint( const std::string& n, unsigned& v, unsigned min, unsigned max )
{ return new opts::opt_uint_mm_t( n, v, min, max ); }

option_t opt_float( const std::string& n, double& v )
{ return new opts::opt_double_t( n, v ); }

option_t opt_float( const std::string& n, double& v, double min, double max )
{ return new opts::opt_double_mm_t( n, v, min, max ); }

option_t opt_timespan( const std::string& n, timespan_t& v )
{ return new opts::opt_timespan_t( n, v ); }

option_t opt_timespan( const std::string& n, timespan_t& v, timespan_t min, timespan_t max )
{ return new opts::opt_timespan_mm_t( n, v, min, max ); }

option_t opt_list( const std::string& n, opts::list_t& v )
{ return new opts::opts_list_t( n, v ); }

option_t opt_map( const std::string& n, opts::map_t& v )
{ return new opts::opts_map_t( n, v ); }

option_t opt_func( const std::string& n, const opts::function_t& f )
{ return new opts::opts_sim_func_t( n, f ); }

option_t opt_deprecated( const std::string& n, const std::string& new_option )
{ return new opts::opts_deperecated_t( n, new_option ); }

#undef SC_SHARED_DATA

