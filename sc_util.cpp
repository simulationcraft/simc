// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

static const double TIME_UNIT  = 0.1;

void wow_random_init( int32_t seed )
{
  srand( seed );
}

double wow_random()
{
   static int32_t RANDOM_GRANULARITY = 10000;

   return ( rand() % RANDOM_GRANULARITY ) / (double) RANDOM_GRANULARITY;
}

int8_t wow_random( double chance )
{
  return wow_random() <= chance ? 1 : 0;
}

char* wow_dup( const char *value )
{
   return strcpy( (char*) malloc( strlen( value ) + 1 ), value );
}

const char* wow_player_type_string( int8_t type )
{
  switch( type )
  {
  case DRUID:      return "druid";
  case PRIEST:     return "priest";
  case MAGE:       return "mage";
  case SHAMAN:     return "shaman";
  case WARLOCK:    return "warlock";
  case PLAYER_PET: return "pet";
  }
  return "unknown";
}

const char* wow_dmg_type_string( int8_t type )
{
  switch( type )
  {
  case DMG_DIRECT:    return "hit";
  case DMG_OVER_TIME: return "tick";
  }
  return "unknown";
}

const char* wow_result_type_string( int8_t type )
{
  switch( type )
  {
  case RESULT_NONE:   return "none";
  case RESULT_MISS:   return "miss";
  case RESULT_RESIST: return "resist";
  case RESULT_DODGE:  return "dodge";
  case RESULT_PARRY:  return "parry";
  case RESULT_BLOCK:  return "block";
  case RESULT_GLANCE: return "glance";
  case RESULT_CRUSH:  return "crush";
  case RESULT_CRIT:   return "crit";
  case RESULT_HIT:    return "hit";
  }
  return "unknown";
}

const char* wow_resource_type_string( int8_t type )
{
  switch( type )
  {
  case RESOURCE_NONE:   return "none";
  case RESOURCE_HEALTH: return "health";
  case RESOURCE_MANA:   return "mana";
  case RESOURCE_RAGE:   return "rage";
  case RESOURCE_ENERGY: return "energy";
  case RESOURCE_FOCUS:  return "focus";
  case RESOURCE_RUNIC:  return "runic_power";
  }
  return "unknown";
}

const char* wow_school_type_string( int8_t school )
{
  switch( school )
  {
  case SCHOOL_HOLY:     return "holy";
  case SCHOOL_SHADOW:   return "shadow";
  case SCHOOL_ARCANE:   return "arcane";
  case SCHOOL_FROST:    return "frost";
  case SCHOOL_FIRE:     return "fire";
  case SCHOOL_NATURE:   return "nature";
  case SCHOOL_PHYSICAL: return "physical";
  }
  return "unknown";
}

const char* wow_talent_tree_string( int8_t tree )
{
  switch( tree )
  {
  case TREE_BALANCE:     return "balance";
  case TREE_FERAL:       return "feral";
  case TREE_RESTORATION: return "restoration";
  case TREE_ARCANE:      return "arcane";
  case TREE_FIRE:        return "fire";
  case TREE_FROST:       return "frost";
  case TREE_DISCIPLINE:  return "discipline";
  case TREE_HOLY:        return "holy";
  case TREE_SHADOW:      return "shadow";
  case TREE_ELEMENTAL:   return "elemental";
  case TREE_ENHANCEMENT: return "enhancement";
  case TREE_AFFLICTION:  return "affliction";
  case TREE_DEMONOLOGY:  return "demonology";
  case TREE_DESTRUCTION: return "destruction";
  }
  return "unknown";
}

const char* wow_weapon_type_string( int8_t weapon )
{
  switch( weapon )
  {
  case WEAPON_NONE:     return "none";
  case WEAPON_DAGGER:   return "dagger";
  case WEAPON_FIST:     return "fist";
  case WEAPON_BEAST:    return "beast";
  case WEAPON_SWORD:    return "sword";
  case WEAPON_MACE:     return "mace";
  case WEAPON_AXE:      return "axe";
  case WEAPON_BEAST_2H: return "beast_2h";
  case WEAPON_SWORD_2H: return "sword_2h";
  case WEAPON_MACE_2H:  return "mace_2h";
  case WEAPON_AXE_2H:   return "axe_2h";
  case WEAPON_STAFF:    return "staff";
  case WEAPON_POLEARM:  return "polearm";
  case WEAPON_BOW:      return "bow";
  case WEAPON_CROSSBOW: return "crossbow";
  case WEAPON_GUN:      return "gun";
  case WEAPON_THROWN:   return "thrown";
  }
  return "unknown";
}

const char* wow_weapon_enchant_type_string( int8_t enchant )
{
  switch( enchant )
  {
  case WEAPON_ENCHANT_NONE: return "none";
  case DEATH_FROST:         return "deathfrost";
  case EXECUTIONER:         return "executioner";
  case MONGOOSE:            return "mongoose";
  }
  return "unknown";
}

const char* wow_weapon_buff_type_string( int8_t buff )
{
  switch( buff )
  {
  case WEAPON_BUFF_NONE:   return "none";
  case FLAMETONGUE_WEAPON: return "flametongue_weapon";
  case FLAMETONGUE_TOTEM:  return "flametongue_totem";
  case POISON:             return "poison";
  case SHARPENING_STONE:   return "sharpening_stone";
  case WINDFURY_WEAPON:    return "windfury_weapon";
  case WINDFURY_TOTEM:     return "windfury_totem";
  }
  return "unknown";
}

int wow_string_split( std::vector<std::string>& results, 
		      const std::string&        str,
		      const char*               delim )
{
  std::string buffer = str;
  std::string::size_type cut_pt;
  
  while( ( cut_pt = buffer.find_first_of( delim ) ) != buffer.npos )
  {
    if( cut_pt > 0 )
    {
      results.push_back( buffer.substr( 0, cut_pt ) );
    }
    buffer = buffer.substr( cut_pt + 1 );
  }
  if( buffer.length() > 0 )
  {
    results.push_back( buffer );
  }

  return results.size();
}

int wow_string_split( const std::string& str,
		      const char*        delim, 
		      const char*        format, ... )
{
  std::vector<std::string>    str_splits;
  std::vector<std::string> format_splits;

  int    str_size = wow_string_split(    str_splits, str,    delim );
  int format_size = wow_string_split( format_splits, format, " "   );

  if( str_size == format_size )
  {
    va_list vap;
    va_start( vap, format );

    for( int i=0; i < str_size; i++ )
    {
      std::string& f = format_splits[ i ];
      const char*  s =    str_splits[ i ].c_str();

      if     ( f == "i8"  ) *( (int8_t*)      va_arg( vap, int8_t*  ) ) = atoi( s );
      else if( f == "i16" ) *( (int16_t*)     va_arg( vap, int16_t* ) ) = atoi( s );
      else if( f == "i32" ) *( (int32_t*)     va_arg( vap, int32_t* ) ) = atoi( s );
      else if( f == "f" )   *( (double*)      va_arg( vap, double*  ) ) = atof( s );
      else if( f == "d" )   *( (double*)      va_arg( vap, double*  ) ) = atof( s );
      else if( f == "s" )   strcpy( (char*)   va_arg( vap, char* ), s );
      else if( f == "S" )   *( (std::string*) va_arg( vap, std::string* ) ) = s;
      else assert( 0 );
    }
    va_end( vap );
  }

  return str_size;
}
