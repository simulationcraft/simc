// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE ==========================================

// is_white_space ===========================================================

static bool is_white_space( char c )
{
  return( c == ' ' || c == '\t' || c == '\n' || c == '\r' );
}

// only_white_space =========================================================

static bool only_white_space( char* s )
{
  while ( *s )
  {
    if ( ! is_white_space( *s ) )
      return false;
    s++;
  }
  return true;
}

} // UNNAMED NAMESPACE ====================================================

// option_t::print ==========================================================

void option_t::print( FILE* file )
{
  if ( type == OPT_STRING )
  {
    std::string& v = *( static_cast<std::string*>(address) );
    util::fprintf( file, "%s=%s\n", name, v.c_str() );
  }
  else if ( type == OPT_BOOL )
  {
    int v = *( static_cast< int* >( address ) );
    util::fprintf( file, "%s=%d\n", name, v ? 1 : 0 );
  }
  else if ( type == OPT_INT )
  {
    int v = *( ( int* ) address );
    util::fprintf( file, "%s=%d\n", name, v );
  }
  else if ( type == OPT_FLT )
  {
    double v = *( ( double* ) address );
    util::fprintf( file, "%s=%.2f\n", name, v );
  }
  else if ( type == OPT_TIMESPAN )
  {
    timespan_t v = *( reinterpret_cast<timespan_t*>(address) );
    util::fprintf( file, "%s=%.2f\n", name, v.total_seconds() );
  }
  else if ( type == OPT_COOLDOWN )
  {
    cooldown_t& v = *( ( cooldown_t* ) address );
    util::fprintf( file, "%s=%.2f\n", name, v.duration.total_seconds() );
  }
  else if ( type == OPT_LIST )
  {
    std::vector<std::string>& v = *( ( std::vector<std::string>* ) address );
    util::fprintf( file, "%s=", name );
    for ( unsigned i=0; i < v.size(); i++ ) util::fprintf( file, "%s%s", ( i?" ":"" ), v[ i ].c_str() );
    util::fprintf( file, "\n" );
  }
  else if ( type == OPT_MAP )
  {
    std::map<std::string,std::string>& m = *( ( std::map<std::string,std::string>* ) address );
    for ( std::map<std::string,std::string>::iterator it = m.begin(), end = m.end(); it != end; ++it )
      util::fprintf( file, "%s%s=%s\n", name, it->first.c_str(), it->second.c_str() );
  }
}

// option_t::save ===========================================================

void option_t::save( FILE* file )
{
  if ( type == OPT_STRING )
  {
    std::string& v = *( ( std::string* ) address );
    if ( ! v.empty() ) util::fprintf( file, "%s=%s\n", name, v.c_str() );
  }
  else if ( type == OPT_BOOL )
  {
    int v = *( ( int* ) address );
    if ( v != 0 ) util::fprintf( file, "%s=1\n", name );
  }
  else if ( type == OPT_INT )
  {
    int v = *( ( int* ) address );
    if ( v != 0 ) util::fprintf( file, "%s=%d\n", name, v );
  }
  else if ( type == OPT_FLT )
  {
    double v = *( ( double* ) address );
    if ( v != 0 ) util::fprintf( file, "%s=%.2f\n", name, v );
  }
  else if ( type == OPT_TIMESPAN )
  {
    timespan_t v = *( reinterpret_cast<timespan_t*>(address) );
    if ( v != timespan_t::zero() ) util::fprintf( file, "%s=%.2f\n", name, v.total_seconds() );
  }
  else if ( type == OPT_COOLDOWN )
  {
    cooldown_t& v = *( ( cooldown_t* ) address );
    if ( v.duration != timespan_t::zero() ) util::fprintf( file, "%s=%.2f\n", name, v.duration.total_seconds() );
  }
  else if ( type == OPT_LIST )
  {
    std::vector<std::string>& v = *( ( std::vector<std::string>* ) address );
    if ( ! v.empty() )
    {
      util::fprintf( file, "%s=", name );
      for ( size_t i = 0; i < v.size(); i++ )
        util::fprintf( file, "%s%s", ( i?" ":"" ), v[ i ].c_str() );
      util::fprintf( file, "\n" );
    }
  }
  else if ( type == OPT_MAP )
  {
    std::map<std::string,std::string>& m = *( ( std::map<std::string,std::string>* ) address );
    for ( std::map<std::string,std::string>::iterator it = m.begin(), end = m.end(); it != end; ++it )
      util::fprintf( file, "%s%s=%s\n", name, it->first.c_str(), it->second.c_str() );
  }
}

// option_t::add ============================================================

void option_t::add( std::vector<option_t>& options,
                    const                  char* name,
                    option_e          type,
                    void*                  address )
{
  size_t size = options.size();
  options.resize( size+1 );
  options[ size ].name = name;
  options[ size ].type = type;
  options[ size ].address = address;
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
    case OPT_STRING: *( ( std::string* ) address ) = v;                         break;
    case OPT_APPEND: *( ( std::string* ) address ) += v;                        break;
    case OPT_INT:    *( ( int* )         address ) = atoi( v.c_str() );         break;
    case OPT_FLT:    *( ( double* )      address ) = atof( v.c_str() );         break;
    case OPT_TIMESPAN:*( reinterpret_cast<timespan_t*>(address) ) = timespan_t::from_seconds( atof( v.c_str() ) ); break;
    case OPT_COOLDOWN:( *( ( cooldown_t** ) address ) ) -> duration = timespan_t::from_seconds( atof( v.c_str() ) ); break;
    case OPT_BOOL:
      *( ( int* ) address ) = atoi( v.c_str() ) ? 1 : 0;
      if ( v != "0" && v != "1" ) sim -> errorf( "Acceptable values for '%s' are '1' or '0'\n", name );
      break;
    case OPT_LIST:
      ( ( std::vector<std::string>* ) address ) -> push_back( v );
      break;
    case OPT_FUNC:
      return ( ( option_function_t ) address )( sim, n, v );
#if 0
// TO-DO: Re-enable at some point
    case OPT_TALENT_RANK:
      return ( ( struct talent_t * ) address )->set_rank( atoi( v.c_str() ), true );
    case OPT_SPELL_ENABLED:
      return ( ( struct spell_id_t * ) address )->enable( atoi( v.c_str() ) != 0 );
#endif
    case OPT_DEPRECATED:
      sim -> errorf( "Option '%s' has been deprecated.\n", name );
      if ( address ) sim -> errorf( "Please use option '%s' instead.\n", ( char* ) address );
      sim -> cancel();
      break;
    default:
      assert( 0 );
      return false;
    }
    return true;
  }
  else if ( type == OPT_MAP )
  {
    std::string::size_type last = n.size() - 1;
    bool append=false;
    if ( n[ last ] == '+' )
    {
      append=true;
      --last;
    }
    std::string::size_type dot = n.rfind( ".", last );
    if ( dot != std::string::npos )
    {
      if ( name == n.substr( 0, dot+1 ) )
      {
        std::map<std::string,std::string>* m = ( std::map<std::string,std::string>* ) address;
        std::string& value = ( *m )[ n.substr( dot+1, last-dot ) ];
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

  for ( size_t i=0; i < num_options; i++ )
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
  std::vector<std::string> splits;
  util::string_split( splits, options_str, "," );
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
  merged_options.push_back( option_t() );
  return &merged_options[ 0 ];
}

// option_db_t::open_file ======================================================

FILE* option_db_t::open_file( const std::string& name )
{
  std::vector<std::string> splits;
  util::string_split( splits, auto_path, ",;|" );

  for ( size_t i = 0; i < splits.size(); i++ )
  {
    FILE* f = fopen( ( splits[ i ] + DIRECTORY_DELIMITER + name ).c_str(), "r" );
    if ( f )
      return f;
  }

  return fopen( name.c_str(), "r" );
}

// option_db_t::parse_file =====================================================

bool option_db_t::parse_file( FILE* file )
{
  char buffer[ 1024 ];
  bool first = true;
  while ( fgets( buffer, sizeof( buffer ), file ) )
  {
    char *b = buffer;
    if ( first )
    {
      // Skip the Windows UTF-8 magic cookie.
      size_t len = strlen( b );
      if ( ( len >= 3 ) && ( ( unsigned char ) b[ 0 ] == 0xEF ) && ( ( unsigned char ) b[ 1 ] == 0xBB ) && ( ( unsigned char ) b[ 2 ] == 0xBF ) )
      {
        b += 3;
      }
      first = false;
    }
    if ( *b == '#' ) continue;
    if ( only_white_space( b ) ) continue;
    parse_line( b );
  }
  return true;
}

// option_db_t::parse_line =====================================================

bool option_db_t::parse_line( const std::string& line )
{
  if ( line[ 0 ] == '#' ) return true;

  std::vector<std::string> tokens;
  size_t num_tokens = util::string_split( tokens, line, " \t\n\r", true );

  for ( size_t i = 0; i < num_tokens; ++i )
  {
    if ( ! parse_token( tokens[ i ] ) )
      return false;
  }

  return true;
}

// option_db_t::parse_token ====================================================

bool option_db_t::parse_token( const std::string& token )
{
  if ( token == "-" )
  {
    parse_file( stdin );
    return true;
  }

  std::string::size_type cut_pt = token.find( '=' );

  if ( cut_pt == token.npos )
  {
    FILE* file = open_file( token );
    if ( ! file )
    {
      printf( "Unexpected parameter '%s'. Expected format: name=value\n", token.c_str() );
      return false;
    }
    parse_file( file );
    fclose( file );
    return true;
  }

  std::string name( token, 0, cut_pt ), value( token, cut_pt + 1, token.npos );

  std::string::size_type start=0;
  while ( ( start = value.find( "$(", start ) ) != std::string::npos )
  {
    std::string::size_type end = value.find( ')', start );
    if ( end == std::string::npos )
    {
      printf( "Variable syntax error: %s\n", token.c_str() );
      return false;
    }

    value.replace( start, ( end - start ) + 1,
                   var_map[ value.substr( start+2, ( end - start ) - 2 ) ] );
  }

  if ( name.size() >= 1 && name[ 0 ] == '$' )
  {
    if ( name.size() < 3 || name[ 1 ] != '(' || name[ name.size()-1 ] != ')' )
    {
      printf( "Variable syntax error: %s\n", token.c_str() );
      return false;
    }
    std::string var_name( name, 2, name.size() - 3 );
    var_map[ var_name ] = value;
    if ( false ) printf( "%s = %s", var_name.c_str(), value.c_str() );
  }
  else if ( name == "input" )
  {
    FILE* file = open_file( value );
    if ( ! file )
    {
      printf( "Unable to open input parameter file '%s'\n", value.c_str() );
      return false;
    }
    parse_file( file );
    fclose( file );
  }
  else
  {
    add( "global", name, value );
  }

  return true;
}

// option_db_t::parse_args =====================================================

bool option_db_t::parse_args( const std::vector<std::string>& args )
{
  for ( size_t i = 0; i < args.size(); ++i )
    if ( ! parse_line( args[ i ] ) )
      return false;

  return true;
}

// option_db_t::option_db_t ====================================================

option_db_t::option_db_t()
{
  // This makes baby pandas cry.

  auto_path = ".";
  auto_path += "|profiles";
  auto_path += "|profiles_heal";
  auto_path += "|profiles";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "Tier14H";
  auto_path += "|profiles";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "Tier14N";
  auto_path += "|profiles";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "Tier13H";
  auto_path += "|profiles";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "Tier13N";
  auto_path += "|profiles";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "mop_test";
  auto_path += "|profiles";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "PreRaid";
  auto_path += "|profiles";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "RaidDummy";


  auto_path += "|..";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "profiles";
  auto_path += "|..";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "profiles_heal";
  auto_path += "|..";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "profiles";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "mop_test";
  auto_path += "|..";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "profiles";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "Tier14H";
  auto_path += "|..";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "profiles";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "Tier14N";
  auto_path += "|..";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "profiles";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "Tier13H";
  auto_path += "|..";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "profiles";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "Tier13N";
  auto_path += "|..";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "profiles";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "PreRaid";
  auto_path += "|..";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "profiles";
  auto_path += DIRECTORY_DELIMITER;
  auto_path += "RaidDummy";
}
