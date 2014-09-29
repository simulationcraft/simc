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

io::cfile open_file( const std::string& auto_path, const std::string& name )
{
  std::vector<std::string> splits = util::string_split( auto_path, ",;|" );

  for ( size_t i = 0; i < splits.size(); i++ )
  {
    FILE* f = io::fopen( splits[ i ] + "/" + name, "r" );
    if ( f )
      return io::cfile(f);
  }

  return io::cfile( name, "r" );
}

} // UNNAMED NAMESPACE ======================================================

// option_t::print ==========================================================

std::ostream& operator<<( std::ostream& stream, const option_t& opt )
{
  switch ( opt.type )
  {
  case option_t::STRING:
  {
    const std::string& v = *static_cast<std::string*>( opt.data.address );
    stream << opt.name << "=" << v.c_str() << "\n";
  }
  break;
  case option_t::INT_BOOL:
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

// option_t::copy ===========================================================

void option_t::copy( std::vector<option_t>& opt_vector,
                     const option_t*        opt_array )
{
  while ( opt_array -> name )
    opt_vector.push_back( *opt_array++ );
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t*             sim,
                      const std::string& n,
                      const std::string& v )
{
  if ( ! name || n.empty() ) return false;

  if ( n == name )
  {
    switch ( type )
    {
      case STRING: *static_cast<std::string*>( data.address ) = v;                        break;
      case APPEND: *static_cast<std::string*>( data.address ) += v;                       break;
      case INT:    *static_cast<int*>( data.address )      = strtol( v.c_str(), 0, 10 );  break;
      case UINT:   *static_cast<unsigned*>( data.address ) = strtoul( v.c_str(), 0, 10 ); break;
      case FLT:    *static_cast<double*>( data.address )   = atof( v.c_str() );           break;
      case TIMESPAN: *( reinterpret_cast<timespan_t*>( data.address ) ) = timespan_t::from_seconds( atof( v.c_str() ) ); break;
      case INT_BOOL:
        *( ( int* ) data.address ) = atoi( v.c_str() ) ? 1 : 0;
        if ( v != "0" && v != "1" ) sim -> errorf( "Acceptable values for '%s' are '1' or '0'\n", name );
        break;
      case BOOL:
        *static_cast<bool*>( data.address ) = atoi( v.c_str() ) != 0;
        if ( v != "0" && v != "1" ) sim -> errorf( "Acceptable values for '%s' are '1' or '0'\n", name );
        break;
      case LIST:
        ( ( std::vector<std::string>* ) data.address ) -> push_back( v );
        break;
      case FUNC:
        return ( *data.func )( sim, n, v );
      case DEPRECATED:
        sim -> errorf( "Option '%s' has been deprecated.\n", name );
        if ( data.cstr ) sim -> errorf( "Please use option '%s' instead.\n", data.cstr );
        sim -> cancel();
        break;
      default:
        assert( 0 );
        return false;
    }
    return true;
  }
  else if ( type == MAP )
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
      if ( name == n.substr( 0, dot + 1 ) )
      {
        std::map<std::string, std::string>* m = ( std::map<std::string, std::string>* ) data.address;
        std::string& value = ( *m )[ n.substr( dot + 1, last - dot ) ];
        value = append ? ( value + v ) : v;
        return true;
      }
    }
  }

  return false;
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t*                 sim,
                      std::vector<option_t>& options,
                      const std::string&     name,
                      const std::string&     value )
{
  size_t num_options = options.size();

  for ( size_t i = 0; i < num_options; i++ )
    if ( options[ i ].parse( sim, name, value ) )
      return true;

  return false;
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t*                 sim,
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
      sim -> errorf( "%s: Unexpected parameter '%s'. Expected format: name=value\n", context, s.c_str() );
      return false;
    }

    std::string n = s.substr( 0, index );
    std::string v = s.substr( index + 1 );

    if ( ! option_t::parse( sim, options, n, v ) )
    {
      sim -> errorf( "%s: Unexpected parameter '%s'.\n", context, n.c_str() );
      return false;
    }
  }

  return true;
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t*                 sim,
                      const char*            context,
                      std::vector<option_t>& options,
                      const std::string&     options_str )
{
  std::vector<std::string> splits = util::string_split( options_str, "," );
  return option_t::parse( sim, context, options, splits );
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t*             sim,
                      const char*        context,
                      const option_t*    options,
                      const std::string& options_str )
{
  std::vector<option_t> options_vector;
  option_t::copy( options_vector, options );
  return parse( sim, context, options_vector, options_str );
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t*             sim,
                      const char*        context,
                      const option_t*    options,
                      const std::vector<std::string>& strings )
{
  std::vector<option_t> options_vector;
  option_t::copy( options_vector, options );
  return parse( sim, context, options_vector, strings );
}

// option_t::merge ==========================================================

option_t* option_t::merge( std::vector<option_t>& merged_options,
                           const option_t*        options1,
                           const option_t*        options2 )
{
  merged_options.clear();
  if ( options1 ) while ( options1 && options1 -> name ) merged_options.push_back( *options1++ );
  if ( options2 ) while ( options2 && options2 -> name ) merged_options.push_back( *options2++ );
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
    #define SC_SHARED_DATA SC_LINUX_PACKAGING "/profiles/"
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
  auto_path = ".|";

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

    if ( *( prefix.end() - 1 ) != '/' )
      prefix += "/";

    auto_path += prefix + "|";
    auto_path += prefix + "PreRaid|";

    // Add profiles for each tier, except pvp
    for ( int i = 0; i < ( N_TIER - 1 ); i++ )
    {
      auto_path += prefix + "Tier" + util::to_string( MIN_TIER + i ) + "B|";
      auto_path += prefix + "Tier" + util::to_string( MIN_TIER + i ) + "H|";
      auto_path += prefix + "Tier" + util::to_string( MIN_TIER + i ) + "N|";
      auto_path += prefix + "Tier" + util::to_string( MIN_TIER + i ) + "M|";
      auto_path += prefix + "Tier" + util::to_string( MIN_TIER + i ) + "PR|";
    }
  }

  if ( auto_path[ auto_path.size() - 1 ] == '|' )
    auto_path.erase( auto_path.size() - 1 );
}

#undef SC_SHARED_DATA

