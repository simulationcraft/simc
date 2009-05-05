// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

// is_white_space ===========================================================

static bool is_white_space( char c )
{
  return( c == ' ' || c == '\t' || c == '\n' || c == '\r' );
}

// only_white_space =========================================================

static bool only_white_space( char* s )
{
  while( *s )
  {
    if( ! is_white_space( *s ) )
      return false;
    s++;
  }
  return true;
}

// remove_space =============================================================

static void remove_white_space( std::string& buffer,
                                char*        line )
{
  while( is_white_space( *line ) ) line++;

  char* endl = line + strlen( line ) - 1;

  while( endl > line && is_white_space( *endl ) ) 
  {
    *endl = '\0';
    endl--;
  }

  buffer = line;
}

} // ANONYMOUS NAMESPACE ====================================================

// option_t::print ==========================================================

void option_t::print( FILE* file, option_t* options )
{
  for( int i=0; options[ i ].name; i++ )
  {
    option_t& o = options[ i ];

    if( o.type != OPT_APPEND &&
        o.type != OPT_DEPRECATED ) 
    {
      fprintf( file, "\t%s : ", o.name );
    }
    
    switch( o.type )
    {
    case OPT_STRING: fprintf( file, "%s\n",    ( (std::string*) o.address ) -> c_str()         ); break;
    case OPT_CHAR_P: fprintf( file, "%s\n",   *( (char**)       o.address )                    ); break;
    case OPT_BOOL:   fprintf( file, "%d\n",   *( (int*)         o.address ) ? 1 : 0            ); break;
    case OPT_INT:    fprintf( file, "%d\n",   *( (int*)         o.address )                    ); break;
    case OPT_FLT:    fprintf( file, "%.2f\n", *( (double*)      o.address )                    ); break;
    case OPT_LIST:     
      {
        std::vector<std::string>& v = *( (std::vector<std::string>*) o.address );
        for( unsigned i=0; i < v.size(); i++ ) fprintf( file, "%s%s", (i?" ":""), v[ i ].c_str() );
        fprintf( file, "\n" );
      }
      break;
    case OPT_APPEND:     break;
    case OPT_DEPRECATED: break;
    default: assert(0);
    }
  }
}

// option_t::parse ==========================================================

bool option_t::parse( app_t*             app,
                      option_t*          options,
                      const std::string& name,
                      const std::string& value )
{
  for( int i=0; options[ i ].name; i++ )
  {
    option_t& o = options[ i ];
      
    if( name == o.name )
    {
      switch( o.type )
      {
      case OPT_STRING: *( (std::string*) o.address ) = value;                                break;
      case OPT_APPEND: *( (std::string*) o.address ) += value;                               break;
      case OPT_CHAR_P: *( (char**)       o.address ) = util_t::dup( value.c_str() );         break;
      case OPT_INT:    *( (int*)         o.address ) = atoi( value.c_str() );                break;
      case OPT_FLT:    *( (double*)      o.address ) = atof( value.c_str() );                break;
      case OPT_LIST:   ( (std::vector<std::string>*) o.address ) -> push_back( value );      break;
      case OPT_BOOL:   
	*( (int*) o.address ) = atoi( value.c_str() ) ? 1 : 0; 
	if( value != "0" && value != "1" ) printf( "%s: Acceptable values for '%s' are '1' or '0'\n", app -> name(), name.c_str() );
	break;
      case OPT_DEPRECATED: 
        printf( "%s: option '%s' has been deprecated.\n", app -> name(), o.name );
        if( o.address ) printf( "%s: please use '%s' instead.\n", app -> name(), (char*) o.address ); 
        exit(0);
      default: assert(0);
      }
      return true;
    }    
  }

  return false;
}

// option_t::parse_file =====================================================

bool option_t::parse_file( app_t* app,
			   FILE*  file )
{
  char buffer[ 1024 ];
  while( fgets( buffer, 1024, file ) )
  {
    if( *buffer == '#' ) continue;
    if( only_white_space( buffer ) ) continue;
    option_t::parse_line( app, buffer );
  }
  return true;
}

// option_t::parse_line =====================================================

bool option_t::parse_line( app_t* app,
			   char*  line )
{
  std::string buffer;

  remove_white_space( buffer, line );

  std::vector<std::string> tokens;

  int num_tokens = util_t::string_split( tokens, buffer, " \t\n" );

  for( int i=0; i < num_tokens; i++ )
    if( ! parse_token( app, tokens[ i ] ) )
      return false;

  return true;
}

// option_t::parse_token ====================================================

bool option_t::parse_token( app_t*       app,
			    std::string& token )
{
  if( token == "-" )
  {
    parse_file( app, stdin );
    return true;
  }

  std::string::size_type cut_pt = token.find_first_of( "=" );

  if( cut_pt == token.npos )
  {
    FILE* file = fopen( token.c_str(), "r" );
    if( ! file )
    {
      printf( "%s: Unexpected parameter '%s'.  Expected format: name=value\n", app -> name(), token.c_str() );
    }
    parse_file( app, file );
    fclose( file );      
    return true;
  }
  
  std::string name, value;

  name  = token.substr( 0, cut_pt );
  value = token.substr( cut_pt + 1 );

  if( name == "input" )
  {
    FILE* file = fopen( value.c_str(), "r" );
    if( ! file )
    {
      printf( "%s: Unable to open input parameter file '%s'\n", app -> name(), value.c_str() );
    }
    parse_file( app, file );
    fclose( file );      
  }
  else if( ! app -> parse_option( name, value ) )
  {
    printf( "%s: Unknown option/value pair: '%s' : '%s'\n", app -> name(), name.c_str(), value.c_str() );
    return false;
  }
  
  return true;
}

