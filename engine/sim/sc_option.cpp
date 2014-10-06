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

template<typename T>
inline void check_boundaries( bool clamp, std::string name, T v, double min, double max) {

  if (clamp) {
    if (v < min || v > max) {
      std::stringstream s;
      s << "Option '" << name << "' with value '" << v
          << "' not within valid boundaries [" << min << " - " << max << "]";
      throw std::invalid_argument(s.str());
    }
  }
}
} // UNNAMED NAMESPACE ======================================================

// option_t::print ==========================================================
/*
std::ostream& operator<<( std::ostream& stream, const option_base_t& opt )
{
  switch ( opt -> type )
  {
  case STRING:
  {
    const std::string& v = *static_cast<std::string*>( opt.data.address );
    stream << opt.name << "=" << v.c_str() << "\n";
  }
  break;
  case INT_BOOL:
  {
    int v = *static_cast<int*>( opt.data.address );
    stream << opt.name << "=" << (v ? "true" : "false") << "\n";
  }
  break;
  case option_t::BOOL:
  {
    bool v = *static_cast<bool*>( opt.data.address );
    stream << opt.name << "=" << (v ? "true" : "false") << "\n";
  }
  break;
  case option_t::INT:
  {
    int v = *static_cast<int*>( opt.data.address );
    stream << opt.name << "=" << v << "\n";
  }
  break;
  case option_t::UINT:
  {
    unsigned v = *static_cast<unsigned*>( opt.data.address );
    stream << opt.name << "=" << v << "\n";
  }
  break;
  case option_t::FLT:
  {
    double v = *static_cast<double*>( opt.data.address );
    stream << opt.name << "=" << v << "\n";
  }
  break;
  case option_t::TIMESPAN:
  {
    timespan_t v = *static_cast<timespan_t*>( opt.data.address );
    stream << opt.name << "=" << v.total_seconds() << "\n";
  }
  break;
  case option_t::LIST:
  {
    const std::vector<std::string>& v = *static_cast<std::vector<std::string>*>( opt.data.address );
    stream << opt.name << "=";
    for ( size_t i = 0; i < v.size(); ++i )
    {
      if ( i > 0 )
        stream << " ";
      stream << v[ i ];
    }
    stream << "\n";
  }
  break;
  case option_t::MAP:
  {
    const std::map<std::string, std::string>& m = *static_cast<std::map<std::string, std::string>*>( opt.data.address );
    for ( std::map<std::string, std::string>::const_iterator it = m.begin(), end = m.end(); it != end; ++it )
      stream << opt.name << it->first << "="<< it->second << "\n";
  }
  break;
  default: break;
  }
  return stream;
}
*/


namespace opts {

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
enum option_assignement_e {
  ASSIGN,
  APPEND,
};
template<class T>
struct opts_generic_t : public opts_helper_t<T>
{
  opts_generic_t( const std::string& name, T& ref, option_assignement_e ass = ASSIGN ) :
    opts_helper_t<T>( name, ref ),
    _ass( ass )
  { }
protected:
  bool parse( sim_t*, const std::string& n, const std::string& value ) const override
  {
    if ( n != this -> name() )
      return false;

    T v = util::from_string<T>( value );
    switch ( _ass )
    {
    case ASSIGN:
      this -> _ref = v;
      break;
    case APPEND:
      this -> _ref += v;
      break;
    }
    return true;
  }
private:
  option_assignement_e _ass;
};


template<class T>
struct opts_bool_t : public opts_generic_t<T>
{
  opts_bool_t( const std::string& name, T& ref ) :
    opts_generic_t<T>( name, ref )
  { }
protected:
  bool parse( sim_t* sim, const std::string& name, const std::string& value ) const override
  {
    if ( name != this -> name() )
      return false;

    if ( value != "0" && value != "1" )
    {
      std::stringstream s;
      s << "Acceptable values for '" << name << "' are '1' or '0'";
      throw std::invalid_argument( s.str() );
    }
    return opts_generic_t<T>::parse( sim, name, value );
  }
};

struct opts_sim_func_t : public opts_helper_t<function_t>
{
  opts_sim_func_t( const std::string& name, function_t& ref ) :
    base_t( name, ref )
  { }
protected:
  bool parse( sim_t* sim, const std::string& n, const std::string& value ) const override
  {
    if ( name() != n )
      return false;

     return _ref( sim, n, value );
  }
};

struct opts_map_t : public opts_helper_t<map_t>
{
  opts_map_t( const std::string& name, map_t& ref ) :
    base_t( name, ref )
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
    if ( !_new_option.empty() ) {
      s << " Please use option '" << _new_option << "' instead.";
    }
    throw std::invalid_argument( s.str() );
    return false;
  }
private:
  std::string _new_option;
};

struct opts_null_t : public option_base_t
{
  opts_null_t() :
    option_base_t( "" )
  { }
protected:
  bool parse( sim_t*, const std::string&, const std::string& ) const override
  { assert( false ); return false; };
};

} // opts

typedef opts::option_base_t* option_t;
namespace opts {
void copy( std::vector<option_t>& opt_vector, const option_t* opt_array );
bool parse( sim_t*, std::vector<option_t>&, const std::string& name, const std::string& value );
void parse( sim_t*, const char* context, std::vector<option_t>&, const std::string& options_str );
void parse( sim_t*, const char* context, std::vector<option_t>&, const std::vector<std::string>& strings );
void parse( sim_t*, const char* context, const option_t*,        const std::vector<std::string>& strings );
void parse( sim_t*, const char* context, const option_t*,        const std::string& options_str );
bool parse_file( sim_t*, FILE* file );
bool parse_line( sim_t*, const char* line );
bool parse_token( sim_t*, const std::string& token );
option_t* merge( std::vector<option_t>& out, const option_t* in1, const option_t* in2 );

} // opts

// option_t::copy ===========================================================

void opts::copy( std::vector<option_t>& opt_vector,
                     const option_t*        opt_array )
{
  if ( !opt_array )
    return;

  while ( !(*opt_array) -> name().empty() )
    opt_vector.push_back( *opt_array++ );
}

// option_t::parse ==========================================================

bool opts::parse( sim_t*                 sim,
                      std::vector<option_t>& options,
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
                      const char*            context,
                      std::vector<option_t>& options,
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
                      const char*            context,
                      std::vector<option_t>& options,
                      const std::string&     options_str )
{
  opts::parse( sim, context, options, util::string_split( options_str, "," ) );
}

// option_t::parse ==========================================================

void opts::parse( sim_t*             sim,
                      const char*        context,
                      const option_t*    options,
                      const std::string& options_str )
{
  std::vector<option_t> options_vector;
  opts::copy( options_vector, options );
  parse( sim, context, options_vector, options_str );
}

// option_t::parse ==========================================================

void opts::parse( sim_t*             sim,
                      const char*        context,
                      const option_t*    options,
                      const std::vector<std::string>& strings )
{
  std::vector<option_t> options_vector;
  opts::copy( options_vector, options );
  parse( sim, context, options_vector, strings );
}

// option_t::merge ==========================================================

option_t* opts::merge( std::vector<option_t>& merged_options,
                           const option_t*        options1,
                           const option_t*        options2 )
{
  merged_options.clear();
  if ( options1 ) while ( options1 && (!(*options1) -> name().empty()) ) merged_options.push_back( *options1++ );
  if ( options2 ) while ( options2 && (!(*options2) -> name().empty()) ) merged_options.push_back( *options2++ );
  merged_options.push_back( opt_null() );
  return &merged_options[ 0 ];
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
    io::cfile file = io::cfile( open_file( auto_path, token ) );
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
{ return new opts::opts_generic_t<std::string>( n, v ); }

option_t opt_append( const std::string& n, std::string& v )
{ return new opts::opts_generic_t<std::string>( n, v, opts::APPEND ); }

option_t opt_bool( const std::string& n, int& v )
{ return new opts::opts_bool_t<int>( n, v ); }

option_t opt_bool( const std::string& n, bool& v )
{ return new opts::opts_bool_t<bool>( n, v ); }

option_t opt_int( const std::string& n, int& v )
{ return new opts::opts_generic_t<int>( n, v ); }

option_t opt_int( const std::string& n, int& v, int , int )
{ return new opts::opts_generic_t<int>( n, v ); }

option_t opt_uint( const std::string& n, unsigned& v )
{ return new opts::opts_generic_t<unsigned>( n, v ); }

option_t opt_uint( const std::string& n, unsigned& v, unsigned , unsigned  )
{ return new opts::opts_generic_t<unsigned>( n, v ); }

option_t opt_float( const std::string& n, double& v )
{ return new opts::opts_generic_t<double>( n, v ); }

option_t opt_float( const std::string& n, double& v, double , double  )
{ return new opts::opts_generic_t<double>( n, v ); }

option_t opt_timespan( const std::string& n, timespan_t& v )
{ return new opts::opts_generic_t<timespan_t>( n, v ); }

option_t opt_timespan( const std::string& n, timespan_t& v, timespan_t , timespan_t  )
{ return new opts::opts_generic_t<timespan_t>( n, v ); }

/*option_t opt_list( const std::string& n, option_base_t::list_t& v )
{ return new option_base_t( n, option_base_t::LIST, &v ); } */

option_t opt_map( const std::string& n, opts::map_t& v )
{ return new opts::opts_map_t( n, v ); }

option_t opt_func( const std::string& n, opts::function_t& f )
{ return new opts::opts_sim_func_t( n, f ); }

option_t opt_deprecated( const std::string& n, const std::string& new_option )
{ return new opts::opts_deperecated_t( n, new_option ); }

option_t opt_null()
{ return new opts::opts_null_t(); }

#undef SC_SHARED_DATA

