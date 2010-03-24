// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace   // ANONYMOUS NAMESPACE ==========================================
{

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
  for ( int i=0; i < num_splits; i++ )
  {
    token_t& t = tokens[ i ];
    t.full = splits[ i ];
    int index=0;
    while ( t.full[ index ] != '\0' &&
            t.full[ index ] != '%'  &&
            ! isalpha( t.full[ index ] ) ) index++;
    if ( index == 0 )
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
  for ( int i=0; i < META_GEM_MAX; i++ )
  {
    const char* meta_gem_name = util_t::meta_gem_type_string( i );

    for ( int j=0; tolower( meta_gem_name[ j ] ) == tolower( option_name[ j ] ); j++ )
      if ( option_name[ j+1 ] == '\0' )
        return true;
  }

  return false;
}

// is_meta_suffix ===========================================================

static bool is_meta_suffix( const std::string& option_name )
{
  for ( int i=0; i < META_GEM_MAX; i++ )
  {
    const char* meta_gem_name = util_t::meta_gem_type_string( i );

    const char* s = strstr( meta_gem_name, "_" );
    if ( ! s ) continue;
    s++;

    for ( int j=0; tolower( s[ j ] ) == tolower( option_name[ j ] ); j++ )
      if ( option_name[ j ] == '\0' )
        return true;
  }

  return false;
}

// parse_meta_gem ===========================================================

static int parse_meta_gem( const std::string& prefix,
                           const std::string& suffix )
{
  if ( prefix.empty() || suffix.empty() ) return META_GEM_NONE;

  std::string name = prefix + "_" + suffix;

  return util_t::parse_meta_gem_type( name );
}

} // ANONYMOUS NAMESPACE ====================================================

// item_t::item_t ===========================================================

item_t::item_t( player_t* p, const std::string& o ) :
    sim(p->sim), player(p), slot(SLOT_NONE), unique(false), unique_enchant(false), is_heroic( false ), options_str(o)
{
}

// item_t::active ===========================================================

bool item_t::active() SC_CONST
{
  if ( slot == SLOT_NONE ) return false;
  if ( ! encoded_name_str.empty() ) return true;
  return false;
}

// item_t::heroic ===========================================================

bool item_t::heroic() SC_CONST
{
  if ( slot == SLOT_NONE ) return false;
  return is_heroic;
}

// item_t::name =============================================================

const char* item_t::name() SC_CONST
{
  if ( ! encoded_name_str.empty() ) return encoded_name_str.c_str();
  if ( !  armory_name_str.empty() ) return  armory_name_str.c_str();
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
  if ( slot == SLOT_MAIN_HAND ) return &( player -> main_hand_weapon );
  if ( slot == SLOT_OFF_HAND  ) return &( player ->  off_hand_weapon );
  if ( slot == SLOT_RANGED    ) return &( player ->    ranged_weapon );
  return 0;
}

// item_t::parse_options ====================================================

bool item_t::parse_options()
{
  if ( options_str.empty() ) return true;

  option_name_str = options_str;
  std::string remainder = "";

  std::string::size_type cut_pt = options_str.find_first_of( "," );

  if ( cut_pt != options_str.npos )
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
    { "heroic",  OPT_STRING, &option_heroic_str  },
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
  armory_t::format( option_heroic_str  );

  return true;
}

// item_t::encode_options ===================================================

void item_t::encode_options()
{
  // Re-build options_str for use in saved profiles

  std::string& o = options_str;

  o = encoded_name_str;

  if ( heroic() )                      { o += ",heroic=1";                           }
  if ( ! encoded_stats_str.empty()   ) { o += ",stats=";   o += encoded_stats_str;   }
  if ( ! encoded_gems_str.empty()    ) { o += ",gems=";    o += encoded_gems_str;    }
  if ( ! encoded_enchant_str.empty() ) { o += ",enchant="; o += encoded_enchant_str; }
  if ( ! encoded_equip_str.empty()   ) { o += ",equip=";   o += encoded_equip_str;   }
  if ( ! encoded_use_str.empty()     ) { o += ",use=";     o += encoded_use_str;     }
  if ( ! encoded_weapon_str.empty()  ) { o += ",weapon=";  o += encoded_weapon_str;  }
}

// item_t::init =============================================================

bool item_t::init()
{
  parse_options();

  encoded_name_str = armory_name_str;

  if ( ! option_name_str.empty() ) encoded_name_str = option_name_str;

  if ( ! option_id_str.empty() )
  {
    if ( ! item_t::download_item( *this, option_id_str ) )
    {
      return false;
    }

    if ( encoded_name_str != armory_name_str )
    {
      sim -> errorf( "Player %s at slot %s has inconsistency between name '%s' and '%s' for id '%s'\n",
		     player -> name(), slot_name(), option_name_str.c_str(), armory_name_str.c_str(), option_id_str.c_str() );

      encoded_name_str = armory_name_str;
    }
  }

  if( encoded_name_str != "empty" &&
      encoded_name_str != "none" )
  {
    id_str              = armory_id_str;
    encoded_stats_str   = armory_stats_str;
    encoded_gems_str    = armory_gems_str;
    encoded_enchant_str = armory_enchant_str;
    encoded_weapon_str  = armory_weapon_str;
    encoded_heroic_str  = armory_heroic_str;
  }

  if ( ! option_heroic_str.empty()  ) encoded_heroic_str  = option_heroic_str;

  if ( ! decode_heroic()  ) return false;

  unique_gear_t::get_equip_encoding( encoded_equip_str, encoded_name_str, heroic(), id_str );
  unique_gear_t::get_use_encoding  ( encoded_use_str,   encoded_name_str, heroic(), id_str );

  if ( ! option_stats_str.empty()   ) encoded_stats_str   = option_stats_str;
  if ( ! option_gems_str.empty()    ) encoded_gems_str    = option_gems_str;
  if ( ! option_enchant_str.empty() ) encoded_enchant_str = option_enchant_str;
  if ( ! option_weapon_str.empty()  ) encoded_weapon_str  = option_weapon_str;


  if ( ! decode_stats()   ) return false;
  if ( ! decode_gems()    ) return false;
  if ( ! decode_enchant() ) return false;
  if ( ! decode_weapon()  ) return false;
  if ( ! decode_heroic()  ) return false;

  if ( ! option_equip_str.empty() ) encoded_equip_str = option_equip_str;
  if ( ! option_use_str.empty()   ) encoded_use_str   = option_use_str;

  if ( ! decode_special(   use, encoded_use_str   ) ) return false;
  if ( ! decode_special( equip, encoded_equip_str ) ) return false;

  encode_options();

  return true;
}

// item_t::decode_heroic ====================================================

bool item_t::decode_heroic()
{
  is_heroic = ! ( encoded_heroic_str.empty() || ( encoded_heroic_str == "0" ) || ( encoded_heroic_str == "no" ) );

  return true;
}

// item_t::decode_stats =====================================================

bool item_t::decode_stats()
{
  if ( encoded_stats_str == "none" ) return true;

  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoded_stats_str );

  for ( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];

    int s = util_t::parse_stat_type( t.name );

    if ( s != STAT_NONE )
    {
      if ( s == STAT_ARMOR )
      {
        if ( slot != SLOT_HEAD      &&
             slot != SLOT_SHOULDERS &&
             slot != SLOT_CHEST     &&
             slot != SLOT_WAIST     &&
             slot != SLOT_LEGS      &&
             slot != SLOT_FEET      &&
             slot != SLOT_WRISTS    &&
             slot != SLOT_HANDS     &&
             slot != SLOT_BACK      &&
             slot != SLOT_OFF_HAND )
        {
          s = STAT_BONUS_ARMOR;
        }
      }
      stats.add_stat( s, t.value );
    }
    else
    {
      sim -> errorf( "Player %s has unknown 'stats=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
      return false;
    }
  }

  return true;
}

// item_t::decode_gems ======================================================

bool item_t::decode_gems()
{
  if ( encoded_gems_str == "none" ) return true;

  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoded_gems_str );

  std::string meta_prefix, meta_suffix;

  for ( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];
    int s;

    if ( ( s = util_t::parse_stat_type( t.name ) ) != STAT_NONE )
    {
      stats.add_stat( s, t.value );
    }
    else if ( is_meta_prefix( t.name ) )
    {
      meta_prefix = t.name;
    }
    else if ( is_meta_suffix( t.name ) )
    {
      meta_suffix = t.name;
    }
    else
    {
      sim -> errorf( "Player %s has unknown 'gems=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
      //return false;
    }
  }

  int meta_gem = parse_meta_gem( meta_prefix, meta_suffix );

  if ( meta_gem != META_GEM_NONE )
  {
    player -> meta_gem = meta_gem;
  }

  return true;
}

// item_t::decode_enchant ===================================================

bool item_t::decode_enchant()
{
  if ( encoded_enchant_str == "none" ) return true;

  if( encoded_enchant_str == "berserking"  ||
      encoded_enchant_str == "executioner" ||
      encoded_enchant_str == "mongoose"    ||
      encoded_enchant_str == "spellsurge"  ) 
  {
    unique_enchant = true;
    return true;
  }

  std::string hidden_str;
  if( unique_gear_t::get_hidden_encoding( hidden_str, encoded_enchant_str, heroic() ) )
  {
    std::vector<token_t> tokens;
    int num_tokens = parse_tokens( tokens, hidden_str );

    for ( int i=0; i < num_tokens; i++ )
    {
      token_t& t = tokens[ i ];
      int s;

      if ( ( s = util_t::parse_stat_type( t.name ) ) != STAT_NONE )
      {
        stats.add_stat( s, t.value );
      }
      else
      {
        sim -> errorf( "Player %s has unknown 'enchant=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
        return false;
      }
    }
    unique_enchant = true;
    enchant.name_str = encoded_enchant_str;
  }

  std::string use_str;
  if( unique_gear_t::get_use_encoding( use_str, encoded_enchant_str, heroic() ) )
  {
    unique_enchant = true;
    use.name_str = encoded_enchant_str;
    return decode_special( use, use_str );
  }

  std::string equip_str;
  if( unique_gear_t::get_equip_encoding( equip_str, encoded_enchant_str, heroic() ) )
  {
    unique_enchant = true;
    enchant.name_str = encoded_enchant_str;
    return decode_special( enchant, equip_str );
  }

  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoded_enchant_str );

  for ( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];
    int s;

    if ( ( s = util_t::parse_stat_type( t.name ) ) != STAT_NONE )
    {
      stats.add_stat( s, t.value );
    }
    else
    {
      sim -> errorf( "Player %s has unknown 'enchant=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
      return false;
    }
  }

  return true;
}

// item_t::decode_special ===================================================

bool item_t::decode_special( special_effect_t& effect,
                             const std::string& encoding )
{
  if ( encoding == "custom" || encoding == "none" ) return true;

  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoding );

  for ( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];
    int s;

    if ( ( s = util_t::parse_stat_type( t.name ) ) != STAT_NONE )
    {
      effect.stat = s;
      effect.amount = t.value;
    }
    else if ( ( s = util_t::parse_school_type( t.name ) ) != SCHOOL_NONE )
    {
      effect.school = s;
      effect.amount = t.value;
    }
    else if ( t.name == "stacks" || t.name == "stack" )
    {
      effect.max_stacks = ( int ) t.value;
    }
    else if ( t.name == "%" )
    {
      effect.proc_chance = t.value / 100.0;
    }
    else if ( t.name == "ppm" )
    {
      effect.proc_chance = -( t.value );
    }
    else if ( t.name == "duration" || t.name == "dur" )
    {
      effect.duration = t.value;
    }
    else if ( t.name == "cooldown" || t.name == "cd" )
    {
      effect.cooldown = t.value;
    }
    else if ( t.name == "tick" )
    {
      effect.tick = t.value;
    }
    else if ( t.full == "reverse" )
    {
      effect.reverse = true;
    }
    else if ( t.full == "ondamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "ontickdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_TICK_DAMAGE;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "ondirectdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DIRECT_DAMAGE;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "onspelldamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = SCHOOL_SPELL_MASK;
    }
    else if ( t.full == "onspelltickdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_TICK_DAMAGE;
      effect.trigger_mask = SCHOOL_SPELL_MASK;
    }
    else if ( t.full == "onspelldirectdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DIRECT_DAMAGE;
      effect.trigger_mask = SCHOOL_SPELL_MASK;
    }
    else if ( t.full == "onattackdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = SCHOOL_ATTACK_MASK;
    }
    else if ( t.full == "onattacktickdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_TICK_DAMAGE;
      effect.trigger_mask = SCHOOL_ATTACK_MASK;
    }
    else if ( t.full == "onattackdirectdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DIRECT_DAMAGE;
      effect.trigger_mask = SCHOOL_ATTACK_MASK;
    }
    else if ( t.full == "onarcanedamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = (1 << SCHOOL_ARCANE);
    }
    else if ( t.full == "onbleeddamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = (1 << SCHOOL_BLEED);
    }
    else if ( t.full == "onchaosdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = (1 << SCHOOL_CHAOS);
    }
    else if ( t.full == "onfiredamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = (1 << SCHOOL_FIRE);
    }
    else if ( t.full == "onfrostdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = (1 << SCHOOL_FROST);
    }
    else if ( t.full == "onfrostfiredamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = (1 << SCHOOL_FROSTFIRE);
    }
    else if ( t.full == "onholydamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = (1 << SCHOOL_HOLY);
    }
    else if ( t.full == "onnaturedamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = (1 << SCHOOL_NATURE);
    }
    else if ( t.full == "onphysicaldamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = (1 << SCHOOL_PHYSICAL);
    }
    else if ( t.full == "onshadowdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = (1 << SCHOOL_SHADOW);
    }
    else if ( t.full == "ondraindamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = (1 << SCHOOL_DRAIN);
    }
    else if ( t.full == "ontick" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_TICK;
    }
    else if ( t.full == "onspellcast" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_SPELL_CAST;
      effect.trigger_mask = RESULT_ALL_MASK;
    }
    else if ( t.full == "onspellcasthit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_SPELL_CAST;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "onspellcastcrit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_SPELL_CAST;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onspellcastmiss" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_SPELL_CAST;
      effect.trigger_mask = RESULT_MISS_MASK;
    }
    else if ( t.full == "onspellhit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_SPELL;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "onspellcrit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_SPELL;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onspellmiss" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_SPELL;
      effect.trigger_mask = RESULT_MISS_MASK;
    }
    else if ( t.full == "onspelldirecthit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_SPELL_DIRECT;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "onspelldirectcrit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_SPELL_DIRECT;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onspelldirectmiss" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_SPELL_DIRECT;
      effect.trigger_mask = RESULT_MISS_MASK;
    }
    else if ( t.full == "onattackcast" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_ATTACK;
      effect.trigger_mask = RESULT_ALL_MASK;
    }
    else if ( t.full == "onattackhit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_ATTACK;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "onattackcrit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_ATTACK;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onattackmiss" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_ATTACK;
      effect.trigger_mask = RESULT_MISS_MASK;
    }
    else if ( t.full == "onattackdirecthit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_ATTACK_DIRECT;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "onattackdirectcrit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_ATTACK_DIRECT;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onattackdirectmiss" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_ATTACK_DIRECT;
      effect.trigger_mask = RESULT_MISS_MASK;
    }
    else
    {
      sim -> errorf( "Player %s has unknown 'use/equip=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
      return false;
    }
  }

  return true;
}

// item_t::decode_weapon ====================================================

bool item_t::decode_weapon()
{
  weapon_t* w = weapon();
  if ( ! w ) return true;

  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoded_weapon_str );

  bool dps_set=false, dmg_set=false, min_set=false, max_set=false;

  for ( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];
    int type, school;

    if ( ( type = util_t::parse_weapon_type( t.name ) ) != WEAPON_NONE )
    {
      w -> type = type;
    }
    else if ( ( school = util_t::parse_school_type( t.name ) ) != SCHOOL_NONE )
    {
      w -> school = school;
    }
    else if ( t.name == "dps" )
    {
      if ( ! dmg_set )
      {
        w -> dps = t.value;
        dps_set = true;
      }
    }
    else if ( t.name == "damage" || t.name == "dmg" )
    {
      w -> damage  = t.value;
      w -> min_dmg = t.value;
      w -> max_dmg = t.value;
      dmg_set = true;
    }
    else if ( t.name == "speed" || t.name == "spd" )
    {
      w -> swing_time = t.value;
    }
    else if ( t.name == "min" )
    {
      w -> min_dmg = t.value;
      min_set = true;
      
      if ( max_set )
      {
        dmg_set = true;
        dps_set = false;
        w -> damage = ( w -> min_dmg + w -> max_dmg ) / 2;
      }
      else
      {
        w -> max_dmg = w -> min_dmg;
      }
    }
    else if ( t.name == "max" )
    {
      w -> max_dmg = t.value;
      max_set = true;
      
      if ( min_set )
      {
        dmg_set = true;
        dps_set = false;
        w -> damage = ( w -> min_dmg + w -> max_dmg ) / 2;
      }
      else
      {
        w -> min_dmg = w -> max_dmg;
      }
    }
    else
    {
      sim -> errorf( "Player %s has unknown 'weapon=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
      return false;
    }
  }

  if ( dps_set ) w -> damage = w -> dps    * w -> swing_time;
  if ( dmg_set ) w -> dps    = w -> damage / w -> swing_time;

  if ( ! max_set || ! min_set )
  {    
    w -> max_dmg = w -> damage;
    w -> min_dmg = w -> damage;
  }

  return true;
}

// item_t::download_slot =============================================================

bool item_t::download_slot( item_t& item, const std::string& item_id, const std::string& enchant_id, const std::string gem_ids[ 3 ] )
{
  player_t* p = item.player;
  bool success = false;

  // Check URL caches
  success =      wowhead_t::download_slot( item, item_id, enchant_id, gem_ids, 1 ) ||
            mmo_champion_t::download_slot( item, item_id, enchant_id, gem_ids, 1 ) ||
                  armory_t::download_slot( item, item_id, 1 );

  if ( success )
    return true;

  success = wowhead_t::download_slot( item, item_id, enchant_id, gem_ids, 0 );

  if ( ! success )
  {
    item.sim -> errorf( "Player %s unable to download slot '%s' info from wowhead.  Trying mmo-champion....\n", 
			p -> name(), item.slot_name() );

    success = mmo_champion_t::download_slot( item, item_id, enchant_id, gem_ids, 0 );
  }

  if ( ! success )
  {
    item.sim -> errorf( "Player %s unable to download slot '%s' info from mmo-champion.  Trying wowarmory....\n", 
			p -> name(), item.slot_name() );

    success = armory_t::download_slot( item, item_id, 0 );
  }

  return success;
}

// item_t::download_item ================================================================

bool item_t::download_item( item_t& item, const std::string& item_id )
{
  player_t* p = item.player;
  bool success = false;

  // Check URL caches
  success = ( mmo_champion_t::download_item( item, item_id, 1 ) ||
              wowhead_t     ::download_item( item, item_id, 1 ) ||
              armory_t      ::download_item( item, item_id, 1 ) );

  if ( ! success )
  {
    success = mmo_champion_t::download_item( item, item_id, 0 );
  }
  if ( ! success )
  {
    item.sim -> errorf( "Player %s unable to download item '%s' info from mmo-champion.  Trying wowhead....\n", 
			p -> name(), item.name() );

    success = wowhead_t::download_item( item, item_id, 0 );
  }
  if ( ! success )
  {
    item.sim -> errorf( "Player %s unable to download item '%s' info from mmo-champion.  Trying wowarmory....\n", 
			p -> name(), item.name() );

    success = armory_t::download_item( item, item_id, 0 );
  }

  return success;
}

// item_t::download_glyph ================================================================

bool item_t::download_glyph( sim_t* sim, std::string& glyph_name, const std::string& glyph_id )
{
  bool success = false;

  success =      wowhead_t::download_glyph( sim, glyph_name, glyph_id, 1 ) ||
            mmo_champion_t::download_glyph( sim, glyph_name, glyph_id, 1 );

  if ( success )
    return true;

  success = wowhead_t::download_glyph( sim, glyph_name, glyph_id, 0 );

  if ( ! success )
  {
    sim -> errorf( "Unable to download glyph id '%s' info from wowhead.  Trying mmo-champion....\n", glyph_id.c_str() );

    success = mmo_champion_t::download_glyph( sim, glyph_name, glyph_id, 0 );
  }

  return success;
}

// item_t::parse_gem ================================================================

int item_t::parse_gem( item_t&            item,
                       const std::string& gem_id )
{
  int gem_type = GEM_NONE;

  if ( gem_id.empty() || gem_id == "" || gem_id == "0" )
    return GEM_NONE;

  // Check URL caches first
  gem_type = wowhead_t::parse_gem( item, gem_id, 1 );
  if ( gem_type != GEM_NONE )
    return gem_type;

  gem_type = mmo_champion_t::parse_gem( item, gem_id, 1 );
  if ( gem_type != GEM_NONE )
    return gem_type;

  // Not in cache. Try to download.
  gem_type = wowhead_t::parse_gem( item, gem_id, 0 );
  if ( gem_type != GEM_NONE )
    return gem_type;

  item.sim -> errorf( "Unable to download gem id '%s' info from wowhead.  Trying mmo-champion....\n", gem_id.c_str() );

  gem_type = mmo_champion_t::parse_gem( item, gem_id, 0 );

  return gem_type;
}
