// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

struct token_t
{
  std::string full;
  std::string name;
  double value;
};

// parse_tokens =============================================================

static int parse_tokens( std::vector<token_t>& tokens,
			 const std::string&    encoded_str )
{
  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, encoded_str, "_" );

  tokens.resize( num_splits );
  for( int i=0; i < num_splits; i++ )
  {
    token_t& t = tokens[ i ];
    t.full = splits[ i ];
    int index=0;
    while( t.full[ index ] != '\0' && 
	   t.full[ index ] != '%'  && 
	   ! isalpha( t.full[ index ] ) ) index++;
    if( index == 0 )
    {
      t.name = t.full;
      t.value = 0;
    }
    else
    {
      t.name = t.full.substr( index );
      t.value = atof( t.full.substr( 0, index ).c_str() );
    }
  }

  return num_splits;
}

// is_meta_prefix ===========================================================

static bool is_meta_prefix( const std::string& option_name )
{
  for( int i=0; i < META_GEM_MAX; i++ )
  {
    const char* meta_gem_name = util_t::meta_gem_type_string( i );

    for( int j=0; tolower( meta_gem_name[ j ] ) == tolower( option_name[ j ] ); j++ )
      if( option_name[ j+1 ] == '\0' )
	return true;
  }

  return false;
}

// is_meta_suffix ===========================================================

static bool is_meta_suffix( const std::string& option_name )
{
  for( int i=0; i < META_GEM_MAX; i++ )
  {
    const char* meta_gem_name = util_t::meta_gem_type_string( i );

    const char* s = strstr( meta_gem_name, "_" );
    if( ! s ) continue;
    s++;

    for( int j=0; tolower( s[ j ] ) == tolower( option_name[ j ] ); j++ )
      if( option_name[ j ] == '\0' )
	return true;
  }

  return false;
}

// parse_meta_gem ===========================================================

static int parse_meta_gem( const std::string& prefix,
			   const std::string& suffix )
{
  if( prefix.empty() || suffix.empty() ) return META_GEM_NONE;

  std::string name = prefix + "_" + suffix;
  
  for( int i=0; i < META_GEM_MAX; i++ )
    if( name == util_t::meta_gem_type_string( i ) )
      return i;

  return META_GEM_NONE;
}

} // ANONYMOUS NAMESPACE ====================================================

// item_t::item_t ===========================================================

item_t::item_t( player_t* p, const std::string& o ) : 
  sim(p->sim), player(p), slot(SLOT_NONE), enchant(ENCHANT_NONE), unique(false), options_str(o)
{ 
}

// item_t::active ===========================================================

bool item_t::active() SC_CONST
{
  if( slot == SLOT_NONE ) return false;
  if( ! encoded_name_str.empty() ) return true;
  return false;
}

// item_t::name =============================================================

const char* item_t::name() SC_CONST
{
  if( ! encoded_name_str.empty() ) return encoded_name_str.c_str();
  if( !  armory_name_str.empty() ) return  armory_name_str.c_str();
  return "inactive";
}

// item_t::slot_name ========================================================

const char* item_t::slot_name() SC_CONST
{
  return util_t::slot_type_string( slot );
}

// item_t::weapon ===========================================================

weapon_t* item_t::weapon() SC_CONST
{
  if( slot == SLOT_MAIN_HAND ) return &( player -> main_hand_weapon );
  if( slot == SLOT_OFF_HAND  ) return &( player ->  off_hand_weapon );
  if( slot == SLOT_RANGED    ) return &( player ->    ranged_weapon );
  return 0;
}

// item_t::parse_options ====================================================

bool item_t::parse_options()
{
  if( options_str.empty() ) return true;

  option_name_str = options_str;
  std::string remainder = "";

  std::string::size_type cut_pt = options_str.find_first_of( "," );

  if( cut_pt != options_str.npos )
  {
    remainder       = options_str.substr( cut_pt + 1 );
    option_name_str = options_str.substr( 0, cut_pt );
  }

  option_t options[] =
  {
    { "id",      OPT_STRING, &option_id_str      },
    { "stats",   OPT_STRING, &option_stats_str   },
    { "gems",    OPT_STRING, &option_gems_str    },
    { "enchant", OPT_STRING, &option_enchant_str },
    { "equip",   OPT_STRING, &option_equip_str   },
    { "use",     OPT_STRING, &option_use_str     },
    { "weapon",  OPT_STRING, &option_weapon_str  },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::parse( sim, option_name_str.c_str(), options, remainder );

  armory_t::format( option_name_str    );
  armory_t::format( option_id_str      );
  armory_t::format( option_stats_str   );
  armory_t::format( option_gems_str    );
  armory_t::format( option_enchant_str );
  armory_t::format( option_equip_str   );
  armory_t::format( option_use_str     );
  armory_t::format( option_weapon_str  );

  return true;
}

// item_t::encode_options ===================================================

void item_t::encode_options()
{
  // Re-build options_str for use in saved profiles

  std::string& o = options_str;

  o = encoded_name_str;

  if( ! encoded_stats_str.empty()   ) { o += ",stats=";   o += encoded_stats_str;   }
  if( ! encoded_gems_str.empty()    ) { o += ",gems=";    o += encoded_gems_str;    }
  if( ! encoded_enchant_str.empty() ) { o += ",enchant="; o += encoded_enchant_str; }
  if( ! encoded_equip_str.empty()   ) { o += ",equip=";   o += encoded_equip_str;   }
  if( ! encoded_use_str.empty()     ) { o += ",use=";     o += encoded_use_str;     }
  if( ! encoded_weapon_str.empty()  ) { o += ",weapon=";  o += encoded_weapon_str;  }
}

// item_t::init =============================================================

bool item_t::init()
{
  parse_options();

  encoded_name_str = armory_name_str;

  if( ! option_name_str.empty() ) encoded_name_str = option_name_str;

  if( ! option_id_str.empty() ) 
  {
    if( ! armory_t::download_item( *this, option_id_str ) )
      return false;

    if( encoded_name_str != armory_name_str ) 
    {
      printf( "simcraft: Warning! Player %s at slot %s has inconsistency between name '%s' and id '%s'\n",
	      player -> name(), slot_name(), option_name_str.c_str(), option_id_str.c_str() );

      encoded_name_str = armory_name_str;
    }
  }

  encoded_stats_str   = armory_stats_str;
  encoded_gems_str    = armory_gems_str;
  encoded_enchant_str = armory_enchant_str;
  encoded_weapon_str  = armory_weapon_str;

  unique_gear_t::get_equip_encoding( encoded_equip_str, encoded_name_str );
  unique_gear_t::get_use_encoding  ( encoded_use_str,   encoded_name_str );

  if( ! option_stats_str.empty()   ) encoded_stats_str   = option_stats_str;
  if( ! option_gems_str.empty()    ) encoded_gems_str    = option_gems_str;
  if( ! option_enchant_str.empty() ) encoded_enchant_str = option_enchant_str;
  if( ! option_equip_str.empty()   ) encoded_equip_str   = option_equip_str;
  if( ! option_use_str.empty()     ) encoded_use_str     = option_use_str;
  if( ! option_weapon_str.empty()  ) encoded_weapon_str  = option_weapon_str;

  if( ! decode_stats()   ) return false;
  if( ! decode_gems()    ) return false;
  if( ! decode_enchant() ) return false;
  if( ! decode_equip()   ) return false;
  if( ! decode_use()     ) return false;
  if( ! decode_weapon()  ) return false;

  encode_options();

  return true;
}

// item_t::decode_stats =====================================================

bool item_t::decode_stats()
{
  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoded_stats_str );

  for( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];

    int s = util_t::parse_stat_type( t.name );

    if( s != STAT_NONE )
    {
      stats.add_stat( s, t.value );
    }
    else
    {
      printf( "simcraft: %s has unknown 'stats=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
      return false;
    }
  }

  return true;
}

// item_t::decode_gems ======================================================

bool item_t::decode_gems()
{
  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoded_gems_str );

  std::string meta_prefix, meta_suffix;

  for( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];
    int s;

    if( ( s = util_t::parse_stat_type( t.name ) ) != STAT_NONE )
    {
      stats.add_stat( s, t.value );
    }
    else if( is_meta_prefix( t.name ) )
    {
      meta_prefix = t.name;
    }
    else if( is_meta_suffix( t.name ) )
    {
      meta_suffix = t.name;
    }
    else
    {
      printf( "simcraft: %s has unknown 'gems=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
      return false;
    }
  }

  int meta_gem = parse_meta_gem( meta_prefix, meta_suffix );

  if( meta_gem != META_GEM_NONE )
  {
    player -> meta_gem = meta_gem;
  }

  return true;
}

// item_t::decode_enchant ===================================================

bool item_t::decode_enchant()
{
  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoded_enchant_str );

  for( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];
    int s, e;

    if( ( s = util_t::parse_stat_type( t.name ) ) != STAT_NONE )
    {
      stats.add_stat( s, t.value );
    }
    else if( ( e = util_t::parse_enchant_type( t.name ) ) != ENCHANT_NONE )
    {
      enchant = e;
    }
    else if( t.full == "pyrorocket" )
    {
      use.school = SCHOOL_FIRE;
      use.amount = 1600;
      use.cooldown = 45;
    }
    else
    {
      printf( "simcraft: %s has unknown 'enchant=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
      return false;
    }
  }

  return true;
}

// item_t::decode_equip =====================================================

bool item_t::decode_equip()
{
  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoded_equip_str );

  for( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];
    int s;

    if( ( s = util_t::parse_stat_type( t.name ) ) != STAT_NONE )
    {
      equip.stat = s;
      equip.amount = t.value;
    }
    else if( ( s = util_t::parse_school_type( t.name ) ) != SCHOOL_NONE )
    {
      equip.school = s;
      equip.amount = t.value;
    }
    else if( t.name == "stacks" || t.name == "stack" )
    {
      equip.max_stacks = (int) t.value;
    }
    else if( t.name == "%" )
    {
      equip.proc_chance = t.value / 100.0;
    }
    else if( t.name == "duration" || t.name == "dur" )
    {
      equip.duration = t.value;
    }
    else if( t.name == "cooldown" || t.name == "cd" )
    {
      equip.cooldown = t.value;
    }
    else if( t.full == "ondamage"     ||
	     t.full == "ontick"       ||
	     t.full == "onspellhit"   ||
	     t.full == "onspellcrit"  ||
	     t.full == "onspellmiss"  ||
	     t.full == "onattackhit"  ||
	     t.full == "onattackcrit" ||
	     t.full == "onattackmiss" )
    {
      equip.trigger = t.full;
    }
    else
    {
      printf( "simcraft: %s has unknown 'equip=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
      return false;
    }
  }

  return true;
}

// item_t::decode_use =======================================================

bool item_t::decode_use()
{
  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoded_use_str );

  for( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];
    int s;

    if( ( s = util_t::parse_stat_type( t.name ) ) != STAT_NONE )
    {
      use.stat = s;
      use.amount = t.value;
    }
    else if( ( s = util_t::parse_school_type( t.name ) ) != SCHOOL_NONE )
    {
      use.school = s;
      use.amount = t.value;
    }
    else if( t.name == "duration" || t.name == "dur" )
    {
      use.duration = t.value;
    }
    else if( t.name == "cooldown" || t.name == "cd" )
    {
      use.cooldown = t.value;
    }
    else
    {
      printf( "simcraft: %s has unknown 'use=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
      return false;
    }
  }

  return true;
}

// item_t::decode_weapon ====================================================

bool item_t::decode_weapon()
{
  weapon_t* w = weapon();
  if( ! w ) return true;

  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoded_weapon_str );

  bool dps_set=false, dmg_set=false;
  
  for( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];
    int type, school;

    if( ( type = util_t::parse_weapon_type( t.name ) ) != WEAPON_NONE )
    {
      w -> type = type;
    }
    else if( ( school = util_t::parse_school_type( t.name ) ) != SCHOOL_NONE )
    {
      w -> school = school;
    }
    else if( t.name == "dps" )
    {
      w -> dps = t.value;
      dps_set = true;
    }
    else if( t.name == "damage" || t.name == "dmg" )
    {
      w -> damage = t.value;
      dmg_set = true;
    }
    else if( t.name == "speed" || t.name == "spd" )
    {
      w -> swing_time = t.value;
    }
    else
    {
      printf( "simcraft: %s has unknown 'weapon=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
      return false;
    }
  }

  if( dps_set ) w -> damage = w -> dps    * w -> swing_time;
  if( dmg_set ) w -> dps    = w -> damage / w -> swing_time;

  return true;
}

