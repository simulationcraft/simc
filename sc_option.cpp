// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

namespace { // ANONYMOUS NAMESPACE ==========================================

// only_white_space =========================================================

static bool only_white_space( char* s )
{
  while( *s )
  {
    if( *s != ' '  && 
	*s != '\t' && 
	*s != '\n' )
      return false;

    s++;
  }
  return true;
}

} // ANONYMOUS NAMESPACE ====================================================

// option_t::print ==========================================================

void option_t::print( option_t* options )
{
  for( int i=0; options[ i ].name; i++ )
  {
    option_t& o = options[ i ];

    printf( "\t%s : ", o.name );
    
    switch( o.type )
    {
    case OPT_STRING: printf( "%s\n",    ( (std::string*) o.address ) -> c_str()         ); break;
    case OPT_CHAR_P: printf( "%s\n",   *( (char**)       o.address )                    ); break;
    case OPT_INT8:   printf( "%d\n",   *( (int8_t*)      o.address )                    ); break;
    case OPT_INT16:  printf( "%d\n",   *( (int16_t*)     o.address )                    ); break;
    case OPT_INT32:  printf( "%d\n",   *( (int32_t*)     o.address )                    ); break;
    case OPT_FLT:    printf( "%.2f\n", *( (double*)       o.address )                    ); break;
    default: assert(0);
    }
  }
}

// option_t::parse ==========================================================

bool option_t::parse( option_t* options,
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
      case OPT_CHAR_P: *( (char**)       o.address ) = wow_dup( value.c_str() );          break;
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

bool option_t::parse( sim_t*    sim,
		      char*     str )
{
  report_t::debug( sim, "option_t::parse: %s", str );
   
  static std::string name;
  static std::string value;

  if( 2 != wow_string_split( str, "=\n ", "S S", &name, &value ) )
  {
    printf( "wow_sim: Unexpected parameter '%s'.  Expected format: name=value\n", str );
    return false;
  }
  
  if( name == "file" )
  {
    FILE* file = fopen( value.c_str(), "r" );
    if( ! file )
    {
      printf( "wow_sim: Unable to open profile file '%s'\n", value.c_str() );
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
//else if( name == "druid"   ) { player_t::create_druid  ( sim, value ); }
//else if( name == "mage"    ) { player_t::create_mage   ( sim, value ); }
  else if( name == "priest"  ) { player_t::create_priest ( sim, value ); }
//else if( name == "shaman"  ) { player_t::create_shaman ( sim, value ); }
//else if( name == "warlock" ) { player_t::create_warlock( sim, value ); }
  else if( name == "pet"     ) 
  { 
    sim -> player_list -> create_pet( value );
  }
  else if( name == "talents" )
  {
    static std::string talent_string;
    std::string::size_type cut_pt = value.find_first_of( "?" );
    if( cut_pt != value.npos ) 
    {
      talent_string = value.substr( cut_pt + 1 );
    }
    else
    {
      talent_string = value;
    }
    sim -> player_list -> parse_talents( talent_string );
  }
  else
  {
    if( ! sim -> parse_option( name, value ) )
    {
      printf( "wow_sim: Unknown option/value pair: '%s' : '%s'\n", name.c_str(), value.c_str() );
      return false;
    }
  }
  
  return true;
}

// option_t::parse ==========================================================

bool option_t::parse( int    argc, 
		      char** argv,
		      sim_t* sim )
{
  if( argc <= 1 ) return false;

   for( int i=1; i < argc; i++ )
   {
      if( ! option_t::parse( sim, argv[ i ] ) )
	 return false;
   }

   if( sim -> max_time <= 0 && sim -> target -> initial_health <= 0 )
   {
     printf( "wow_sim: One of -max_time or -target_health must be specified.\n" );
     return false;
   }

   if( sim -> debug ) sim -> print_options();

   return true;
}

