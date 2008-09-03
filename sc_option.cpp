// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

namespace { // ANONYMOUS NAMESPACE ==========================================

// is_white_space ===========================================================

static bool is_white_space( char c )
{
  return( c == ' ' || c == '\t' || c == '\n' );
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

void option_t::print( sim_t* sim, option_t* options )
{
  FILE* f = sim -> output_file;

  for( int i=0; options[ i ].name; i++ )
  {
    option_t& o = options[ i ];

    fprintf( sim -> output_file, "\t%s : ", o.name );
    
    switch( o.type )
    {
    case OPT_STRING: fprintf( f, "%s\n",    ( (std::string*) o.address ) -> c_str()         ); break;
    case OPT_CHAR_P: fprintf( f, "%s\n",   *( (char**)       o.address )                    ); break;
    case OPT_INT8:   fprintf( f, "%d\n",   *( (int8_t*)      o.address )                    ); break;
    case OPT_INT16:  fprintf( f, "%d\n",   *( (int16_t*)     o.address )                    ); break;
    case OPT_INT32:  fprintf( f, "%d\n",   *( (int32_t*)     o.address )                    ); break;
    case OPT_FLT:    fprintf( f, "%.2f\n", *( (double*)      o.address )                    ); break;
    default: assert(0);
    }
  }
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t*             sim,
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
      case OPT_STRING: *( (std::string*) o.address ) = value;                             break;
      case OPT_CHAR_P: *( (char**)       o.address ) = util_t::dup( value.c_str() );      break;
      case OPT_INT8:   *( (int8_t*)      o.address ) = atoi( value.c_str() );             break;
      case OPT_INT16:  *( (int16_t*)     o.address ) = atoi( value.c_str() );             break;
      case OPT_INT32:  *( (int32_t*)     o.address ) = atoi( value.c_str() );             break;
      case OPT_FLT:    *( (double*)      o.address ) = atof( value.c_str() );             break;
      default: assert(0);
      }
      return true;
    }	 
  }

  return false;
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t* sim,
		      char*  line )
{
  if( sim -> debug ) report_t::log( sim, "option_t::parse: %s", line );

  static std::string buffer, name, value;

  remove_white_space( buffer, line );

  std::string::size_type cut_pt = buffer.find_first_of( "=" );

  if( cut_pt == buffer.npos )
  {
    fprintf( sim -> output_file, "simcraft: Unexpected parameter '%s'.  Expected format: name=value\n", line );
    return false;
  }
  
  name  = buffer.substr( 0, cut_pt );
  value = buffer.substr( cut_pt + 1 );

  if( name == "output" )
  {
    if( sim -> output_file != stdout ) fclose( sim -> output_file );

    sim -> output_file = fopen( value.c_str(), "w" );
    if( ! sim -> output_file )
    {
      fprintf( stderr, "simcraft: Unable to open output file '%s'\n", value.c_str() );
      exit(0);
    }
  }
  else if( name == "input" )
  {
    FILE* file = fopen( value.c_str(), "r" );
    if( ! file )
    {
      fprintf( sim -> output_file, "simcraft: Unable to open input parameter file '%s'\n", value.c_str() );
      exit(0);
    }

    char buffer[ 1024 ];
    while( fgets( buffer, 1024, file ) )
    {
      if( *buffer == '#' ) continue;
      if( only_white_space( buffer ) ) continue;
      option_t::parse( sim, buffer );
    }
    fclose( file );      
  }
  else if( name == "html" )
  {
    if( sim -> html_file != stdout ) fclose( sim -> html_file );

    sim -> html_file = fopen( value.c_str(), "w" );
    if( ! sim -> html_file )
    {
      fprintf( stderr, "simcraft: Unable to open html file '%s'\n", value.c_str() );
      exit(0);
    }
  }
  else if( name == "druid" ) 
  { 
    sim -> active_player = player_t::create_druid( sim, value ); 
    assert( sim -> active_player );
  }
  else if( name == "mage" ) 
  { 
    sim -> active_player = player_t::create_mage( sim, value ); 
  }
  else if( name == "priest" ) 
  { 
    sim -> active_player = player_t::create_priest( sim, value );
    assert( sim -> active_player );
  }
  else if( name == "shaman" )
  {
    sim -> active_player = player_t::create_shaman( sim, value );
    assert( sim -> active_player );
  }
  else if( name == "warlock" )
  {
    //sim -> active_player = player_t::create_warlock( sim, value );
    assert( 0 );
  }
  else if( name == "pet" ) 
  { 
    sim -> active_player = sim -> active_player -> create_pet( value );
    assert( sim -> active_player );
  }
  else if( name == "active"  ) 
  { 
    if( value == "owner" )
    {
      sim -> active_player = sim -> active_player -> cast_pet() -> owner;
    }
    else
    {
      for( sim -> active_player = sim -> player_list; sim -> active_player; sim -> active_player = sim -> active_player -> next )
      {
	if( value == sim -> active_player -> name() )
	  break;
      }
      assert( sim -> active_player );
    }
  }
  else if( name == "talents" )
  {
    static std::string talent_string;
    std::string::size_type cut_pt; 
    if( ( cut_pt = value.find_first_of( "=" ) ) != value.npos ) 
    {
      talent_string = value.substr( cut_pt + 1 );
    }
    else
    {
      talent_string = value;
    }
    sim -> active_player -> parse_talents( talent_string );
  }
  else
  {
    if( ! sim -> parse_option( name, value ) )
    {
      fprintf( sim -> output_file, "simcraft: Unknown option/value pair: '%s' : '%s'\n", name.c_str(), value.c_str() );
      return false;
    }
  }
  
  return true;
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t* sim,
		      int    argc, 
		      char** argv )
{
  if( argc <= 1 ) return false;

   for( int i=1; i < argc; i++ )
   {
      if( ! option_t::parse( sim, argv[ i ] ) )
	 return false;
   }

   if( sim -> max_time <= 0 && sim -> target -> initial_health <= 0 )
   {
     fprintf( sim -> output_file, "simcraft: One of -max_time or -target_health must be specified.\n" );
     return false;
   }

   if( sim -> debug ) sim -> print_options();

   return true;
}

