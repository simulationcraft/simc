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

  for( int i=0; i < num_splits; i++ )
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

// option_t::copy ===========================================================

void option_t::copy( std::vector<option_t>& opt_vector,
                     option_t*              opt_array )
{
  int vector_size = ( int ) opt_vector.size();
  int  array_size = 0;

  for ( int i=0; opt_array[ i ].name; i++ ) array_size++;
  opt_vector.resize( vector_size + array_size );

  for ( int i=0; i < array_size; i++ )
  {
    opt_vector[ vector_size + i ]  = opt_array[ i ];
  }
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t*             sim,
                      const std::string& n,
                      const std::string& v )
{
  if ( ! name ) return false;

  if ( n == name )
  {
    switch ( type )
    {
    case OPT_STRING: *( ( std::string* ) address ) = v;                         break;
    case OPT_APPEND: *( ( std::string* ) address ) += v;                        break;
    case OPT_CHARP:  *( ( char** )       address ) = util_t::dup( v.c_str() );  break;
    case OPT_INT:    *( ( int* )         address ) = atoi( v.c_str() );         break;
    case OPT_FLT:    *( ( double* )      address ) = atof( v.c_str() );         break;
    case OPT_BOOL:
      *( ( int* ) address ) = atoi( v.c_str() ) ? 1 : 0;
      if ( v != "0" && v != "1" ) sim -> errorf( "Acceptable values for '%s' are '1' or '0'\n", name );
      break;
    case OPT_FUNC: return ( ( option_function_t ) address )( sim, n, v );
    case OPT_LIST:   ( ( std::vector<std::string>* ) address ) -> push_back( v ); break;
    case OPT_DEPRECATED:
      sim -> errorf( "Option '%s' has been deprecated.\n", name );
      if ( address ) sim -> errorf( "Please use option '%s' instead.\n", ( char* ) address );
      exit( 0 );
    default: assert( 0 );
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
                      option_t*          options,
                      const std::string& options_str )
{
  int num_options=0;
  while ( options[ num_options ].name ) num_options++;

  std::vector<option_t> options_vector;
  options_vector.resize( num_options );

  for ( int i=0; i < num_options; i++ )
  {
    options_vector[ i ] = options[ i ];
  }

  return parse( sim, context, options_vector, options_str );
}

// option_t::parse_file =====================================================

bool option_t::parse_file( sim_t* sim,
                           FILE*  file )
{
  char buffer[ 1024 ];
  while ( fgets( buffer, 1024, file ) )
  {
    if ( *buffer == '#' ) continue;
    if ( only_white_space( buffer ) ) continue;
    option_t::parse_line( sim, buffer );
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

  std::string::size_type cut_pt = token.find_first_of( "=" );

  if ( cut_pt == token.npos )
  {
    FILE* file = open_file( sim, token );
    if ( ! file )
    {
      sim -> errorf( "Unexpected parameter '%s'.  Expected format: name=value\n", token.c_str() );
      return false;
    }
    sim -> active_files.push_back( token );
    parse_file( sim, file );
    fclose( file );
    sim -> active_files.pop_back();
    return true;
  }

  std::string name, value;

  name  = token.substr( 0, cut_pt );
  value = token.substr( cut_pt + 1 );

  if ( name == "input" )
  {
    FILE* file = open_file( sim, value );
    if ( ! file )
    {
      sim -> errorf( "Unable to open input parameter file '%s'\n", value.c_str() );
    }
    sim -> active_files.push_back( token );
    parse_file( sim, file );
    fclose( file );
    sim -> active_files.pop_back();
  }
  else if ( ! sim -> parse_option( name, value ) )
  {
    sim -> errorf( "Unknown option/value pair: '%s' : '%s'\n", name.c_str(), value.c_str() );
    return false;
  }

  return true;
}
