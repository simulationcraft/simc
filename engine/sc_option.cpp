// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace   // ANONYMOUS NAMESPACE ==========================================
{

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

// open_file ================================================================

static FILE* open_file( sim_t* sim, const std::string& name )
{
  if ( sim -> path_str.empty() )
  {
    return fopen( name.c_str(), "r" );
  }

  std::string buffer;
  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, sim -> path_str, ",;|" );

  for ( int i=0; i < num_splits; i++ )
  {
    buffer = splits[ i ];
    buffer += DIRECTORY_DELIMITER;
    buffer += name;

    FILE* f = fopen( buffer.c_str(), "r" );
    if ( f ) return f;
  }

  return fopen( name.c_str(), "r" );
}

} // ANONYMOUS NAMESPACE ====================================================

// option_t::print ==========================================================

void option_t::print( FILE* file )
{
  if ( type == OPT_STRING )
  {
    std::string& v = *( ( std::string* ) address );
    util_t::fprintf( file, "%s=%s\n", name, v.empty() ? "" : v.c_str() );
  }
  else if ( type == OPT_CHARP )
  {
    const char* v = *( ( char** ) address );
    util_t::fprintf( file, "%s=%s\n", name, v ? v : "" );
  }
  else if ( type == OPT_BOOL )
  {
    int v = *( ( int* ) address );
    util_t::fprintf( file, "%s=%d\n", name, ( v>0 ) ? 1 : 0 );
  }
  else if ( type == OPT_INT )
  {
    int v = *( ( int* ) address );
    util_t::fprintf( file, "%s=%d\n", name, v );
  }
  else if ( type == OPT_FLT )
  {
    double v = *( ( double* ) address );
    util_t::fprintf( file, "%s=%.2f\n", name, v );
  }
  else if ( type == OPT_TIMESPAN )
  {
    timespan_t v = *( ( timespan_t* ) address );
    util_t::fprintf( file, "%s=%.2f\n", name, v.total_seconds() );
  }
  else if ( type == OPT_LIST )
  {
    std::vector<std::string>& v = *( ( std::vector<std::string>* ) address );
    util_t::fprintf( file, "%s=", name );
    for ( unsigned i=0; i < v.size(); i++ ) util_t::fprintf( file, "%s%s", ( i?" ":"" ), v[ i ].c_str() );
    util_t::fprintf( file, "\n" );
  }
}

// option_t::save ===========================================================

void option_t::save( FILE* file )
{
  if ( type == OPT_STRING )
  {
    std::string& v = *( ( std::string* ) address );
    if ( ! v.empty() ) util_t::fprintf( file, "%s=%s\n", name, v.c_str() );
  }
  else if ( type == OPT_CHARP )
  {
    const char* v = *( ( char** ) address );
    if ( v ) util_t::fprintf( file, "%s=%s\n", name, v );
  }
  else if ( type == OPT_BOOL )
  {
    int v = *( ( int* ) address );
    if ( v > 0 ) util_t::fprintf( file, "%s=1\n", name );
  }
  else if ( type == OPT_INT )
  {
    int v = *( ( int* ) address );
    if ( v != 0 ) util_t::fprintf( file, "%s=%d\n", name, v );
  }
  else if ( type == OPT_FLT )
  {
    double v = *( ( double* ) address );
    if ( v != 0 ) util_t::fprintf( file, "%s=%.2f\n", name, v );
  }
  else if ( type == OPT_TIMESPAN )
  {
    timespan_t v = *( ( timespan_t* ) address );
    if ( v != timespan_t::zero ) util_t::fprintf( file, "%s=%.2f\n", name, v.total_seconds() );
  }
  else if ( type == OPT_LIST )
  {
    std::vector<std::string>& v = *( ( std::vector<std::string>* ) address );
    if ( ! v.empty() )
    {
      util_t::fprintf( file, "%s=", name );
      for ( unsigned i=0; i < v.size(); i++ ) util_t::fprintf( file, "%s%s", ( i?" ":"" ), v[ i ].c_str() );
      util_t::fprintf( file, "\n" );
    }
  }
}

// option_t::add ============================================================

void option_t::add( std::vector<option_t>& options,
                    const                  char* name,
                    int                    type,
                    void*                  address )
{
  int size = ( int ) options.size();
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
  if ( name && n == name )
  {
    switch ( type )
    {
    case OPT_STRING: *( ( std::string* ) address ) = v;                         break;
    case OPT_APPEND: *( ( std::string* ) address ) += v;                        break;
    case OPT_CHARP:  *( ( char** )       address ) = util_t::dup( v.c_str() );  break;
    case OPT_INT:    *( ( int* )         address ) = atoi( v.c_str() );         break;
    case OPT_FLT:    *( ( double* )      address ) = atof( v.c_str() );         break;
    case OPT_TIMESPAN:*( ( timespan_t* ) address ) = timespan_t::from_seconds( atof( v.c_str() ) ); break;
    case OPT_BOOL:
      *( ( int* ) address ) = atoi( v.c_str() ) ? 1 : 0;
      if ( v != "0" && v != "1" ) sim -> errorf( "Acceptable values for '%s' are '1' or '0'\n", name );
      break;
    case OPT_LIST:
      ( ( std::vector<std::string>* ) address ) -> push_back( v );
      break;
    case OPT_FUNC:
      return ( ( option_function_t ) address )( sim, n, v );
    case OPT_TALENT_RANK:
      return ( ( struct talent_t * ) address )->set_rank( atoi( v.c_str() ), true );
    case OPT_SPELL_ENABLED:
      return ( ( struct spell_id_t * ) address )->enable( atoi( v.c_str() ) != 0 );
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

  return false;
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t*                 sim,
                      std::vector<option_t>& options,
                      const std::string&     name,
                      const std::string&     value )
{
  int num_options = ( int ) options.size();

  for ( int i=0; i < num_options; i++ )
    if ( options[ i ].parse( sim, name, value ) )
      return true;

  return false;
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t*                 sim,
                      const char*            context,
                      std::vector<option_t>& options,
                      const std::string&     options_str )
{
  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, options_str, "," );

  for ( int i=0; i < num_splits; i++ )
  {
    std::string& s = splits[ i ];

    std::string::size_type index = s.find( "=" );

    if ( index == std::string::npos )
    {
      sim -> errorf( "%s: Unexpected parameter '%s'.  Expected format: name=value\n", context, s.c_str() );
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

bool option_t::parse( sim_t*             sim,
                      const char*        context,
                      const option_t*    options,
                      const std::string& options_str )
{
  std::vector<option_t> options_vector;
  option_t::copy( options_vector, options );
  return parse( sim, context, options_vector, options_str );
}

// option_t::parse_file =====================================================

bool option_t::parse_file( sim_t* sim,
                           FILE*  file )
{
  char buffer[ 1024 ];
  bool first = true;
  while ( fgets( buffer, 1024, file ) )
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
    option_t::parse_line( sim, b );
  }
  return true;
}

// option_t::parse_line =====================================================

bool option_t::parse_line( sim_t* sim,
                           char*  line )
{
  if ( *line == '#' ) return true;

  std::string buffer = line;

  std::vector<std::string> tokens;

  int num_tokens = util_t::string_split( tokens, buffer, " \t\n\r", true );

  for ( int i=0; i < num_tokens; i++ )
    if ( ! parse_token( sim, tokens[ i ] ) )
      return false;

  return true;
}

// option_t::parse_token ====================================================

bool option_t::parse_token( sim_t*       sim,
                            std::string& token )
{
  if ( token == "-" )
  {
    parse_file( sim, stdin );
    return true;
  }

  std::string::size_type cut_pt = token.find( '=' );

  if ( cut_pt == token.npos )
  {
    FILE* file = open_file( sim, token );
    if ( ! file )
    {
      sim -> errorf( "Unexpected parameter '%s'.  Expected format: name=value\n", token.c_str() );
      return false;
    }
    sim -> active_files.push( token );
    parse_file( sim, file );
    fclose( file );
    sim -> active_files.pop();
    return true;
  }

  std::string name( token, 0, cut_pt ), value( token, cut_pt + 1, token.npos );

  std::string::size_type start=0;
  while ( ( start = value.find( "$(", start ) ) != std::string::npos )
  {
    std::string::size_type end = value.find( ')', start );
    if ( end == std::string::npos )
    {
      sim -> errorf( "Variable syntax error: %s\n", token.c_str() );
      return false;
    }

    value.replace( start, ( end - start ) + 1,
                   sim -> var_map[ value.substr( start+2, ( end - start ) - 2 ) ] );
  }

  if ( name.size() >= 1 && name[ 0 ] == '$' )
  {
    if ( name.size() < 3 || name[ 1 ] != '(' || name[ name.size()-1 ] != ')' )
    {
      sim -> errorf( "Variable syntax error: %s\n", token.c_str() );
      return false;
    }
    std::string var_name( name, 2, name.size() - 3 );
    sim -> var_map[ var_name ] = value;
    if ( sim -> debug ) log_t::output( sim, "%s = %s", var_name.c_str(), value.c_str() );
    return true;
  }

  if ( name == "input" )
  {
    FILE* file = open_file( sim, value );
    if ( ! file )
    {
      sim -> errorf( "Unable to open input parameter file '%s'\n", value.c_str() );
      return false;
    }
    sim -> active_files.push( value );
    parse_file( sim, file );
    fclose( file );
    sim -> active_files.pop();
  }
  else if ( ! sim -> parse_option( name, value ) )
  {
    sim -> errorf( "Unknown option/value pair: '%s' : '%s'\n", name.c_str(), value.c_str() );
    return false;
  }

  return true;
}

// option_t::merge ==========================================================

namespace {
inline void merge_some( std::vector<option_t>& out, const option_t* in )
{
  if ( ! in ) return;
  while ( in -> name )
    out.push_back( *in++ );
}
}

option_t* option_t::merge( std::vector<option_t>& merged_options,
                           const option_t*        options1,
                           const option_t*        options2 )
{
  merged_options.clear();
  merge_some( merged_options, options1 );
  merge_some( merged_options, options2 );
  merged_options.push_back( option_t() );
  return &merged_options[ 0 ];
}
