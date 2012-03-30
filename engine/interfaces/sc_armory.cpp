// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // ANONYMOUS NAMESPACE ==========================================

// is_number ================================================================

static bool is_number( const std::string s )
{
  int size = ( int ) s.size();
  for ( int i=0; i < size; i++ )
    if ( ! isdigit( s[ i ] ) )
      return false;
  return true;
}

// stat_search ==============================================================

static void stat_search( std::string&              encoding_str,
                         std::vector<std::string>& description_tokens,
                         int                       stat_type,
                         const std::string&        stat_str )
{
  std::vector<std::string> stat_tokens;
  int num_stats = util_t::string_split( stat_tokens, stat_str, " " );
  int num_descriptions = ( int ) description_tokens.size();

  for ( int i=0; i < num_descriptions; i++ )
  {
    bool match = true;

    for ( int j=0; j < num_stats && match; j++ )
    {
      if ( ( i + j ) == num_descriptions )
      {
        match = false;
      }
      else
      {
        if ( stat_tokens[ j ][ 0 ] == '!' )
        {
          if ( stat_tokens[ j ].substr( 1 ) == description_tokens[ i + j ] )
          {
            match = false;
          }
        }
        else
        {
          if ( stat_tokens[ j ] != description_tokens[ i + j ] )
          {
            match = false;
          }
        }
      }
    }

    if ( match )
    {
      std::string value_str;

      if ( ( i > 0 ) &&
           ( is_number( description_tokens[ i-1 ] ) ) )
      {
        value_str = description_tokens[ i-1 ];
      }
      if ( ( ( i + num_stats + 1 ) < num_descriptions ) &&
           ( description_tokens[ i + num_stats ] == "by" ) &&
           ( is_number( description_tokens[ i + num_stats + 1 ] ) ) )
      {
        value_str = description_tokens[ i + num_stats + 1 ];
      }

      if ( ! value_str.empty() )
      {
        encoding_str += "_" + value_str + util_t::stat_type_abbrev( stat_type );
      }
    }
  }
}

// is_proc_description ======================================================

static bool is_proc_description( const std::string& description_str )
{
  if ( description_str.find( "chance" ) != std::string::npos ) return true;
  if ( description_str.find( "stack"  ) != std::string::npos ) return true;
  if ( description_str.find( "time"   ) != std::string::npos ) return true;
  if ( ( description_str.find( "_sec"   ) != std::string::npos ) &&
       ! ( ( description_str.find( "restores" ) != std::string::npos ) &&
           ( ( description_str.find( "_per_5_sec" ) != std::string::npos ) ||
             ( description_str.find( "_every_5_sec" ) != std::string::npos ) ) ) )
    return true;

  return false;
}

} // ANONYMOUS NAMESPACE ===================================================

// armory_t::fuzzy_stats ====================================================

void armory_t::fuzzy_stats( std::string&       encoding_str,
                            const std::string& description_str )
{
  if ( description_str.empty() ) return;

  std::string buffer = description_str;
  armory_t::format( buffer );

  if ( is_proc_description( buffer ) )
    return;

  std::vector<std::string> splits;
  util_t::string_split( splits, buffer, "_." );

  stat_search( encoding_str, splits, STAT_MAX,  "all stats" );
  stat_search( encoding_str, splits, STAT_MAX,  "to all stats" );

  stat_search( encoding_str, splits, STAT_STRENGTH,  "strength" );
  stat_search( encoding_str, splits, STAT_AGILITY,   "agility" );
  stat_search( encoding_str, splits, STAT_STAMINA,   "stamina" );
  stat_search( encoding_str, splits, STAT_INTELLECT, "intellect" );
  stat_search( encoding_str, splits, STAT_SPIRIT,    "spirit" );

  stat_search( encoding_str, splits, STAT_SPELL_POWER, "spell power" );
  stat_search( encoding_str, splits, STAT_MP5,         "mana regen" );
  stat_search( encoding_str, splits, STAT_MP5,         "mana every 5" );
  stat_search( encoding_str, splits, STAT_MP5,         "mana per 5" );
  stat_search( encoding_str, splits, STAT_MP5,         "mana restored per 5" );
  stat_search( encoding_str, splits, STAT_MP5,         "mana 5" );

  stat_search( encoding_str, splits, STAT_ATTACK_POWER,             "attack power" );
  stat_search( encoding_str, splits, STAT_EXPERTISE_RATING,         "expertise rating" );

  stat_search( encoding_str, splits, STAT_HASTE_RATING,         "haste rating" );
  stat_search( encoding_str, splits, STAT_HIT_RATING,           "ranged hit rating" );
  stat_search( encoding_str, splits, STAT_HIT_RATING,           "hit rating" );
  stat_search( encoding_str, splits, STAT_CRIT_RATING,          "ranged critical strike" );
  stat_search( encoding_str, splits, STAT_CRIT_RATING,          "critical strike rating" );
  stat_search( encoding_str, splits, STAT_CRIT_RATING,          "crit rating" );
  stat_search( encoding_str, splits, STAT_MASTERY_RATING,       "mastery rating" );

  stat_search( encoding_str, splits, STAT_BONUS_ARMOR,    "armor !penetration" );
  stat_search( encoding_str, splits, STAT_DODGE_RATING,   "dodge rating" );
  stat_search( encoding_str, splits, STAT_PARRY_RATING,   "parry rating" );
  stat_search( encoding_str, splits, STAT_BLOCK_RATING,   "block_rating" );
}

// armory_t::parse_meta_gem =================================================

int armory_t::parse_meta_gem( const std::string& description )
{
  if ( description == "+54 Agility and 3% Increased Critical Damage"                      ) return META_AGILE_SHADOWSPIRIT;
  if ( description == "+32 Stamina and 2% Increased Armor Value from Items"               ) return META_AUSTERE_EARTHSIEGE;
  if ( description == "+81 Stamina and 2% Increased Armor Value from Items"               ) return META_AUSTERE_SHADOWSPIRIT;
  if ( description == "+21 Critical Strike Rating and +2% Mana"                           ) return META_BEAMING_EARTHSIEGE;
  if ( description == "+21 Intellect and +2% Reduced Threat"                              ) return META_BRACING_EARTHSIEGE;
  if ( description == "+12 Intellect and +2% Reduced Threat"                              ) return META_BRACING_EARTHSTORM;
  if ( description == "+54 Intellect and +2% Reduced Threat"                              ) return META_BRACING_SHADOWSPIRIT;
  if ( description == "+54 Intellect and 3% Increased Critical Damage"                    ) return META_BURNING_SHADOWSPIRIT;
  if ( description == "+54 Critical Strike Rating and 3% Increased Critical Damage"       ) return META_CHAOTIC_SHADOWSPIRIT;
  if ( description == "+21 Critical Strike Rating and 3% Increased Critical Damage"       ) return META_CHAOTIC_SKYFLARE;
  if ( description == "+12 Critical Strike Rating and 3% Increased Critical Damage"       ) return META_CHAOTIC_SKYFIRE;
  if ( description == "+54 Critical Strike Rating and 1% Spell Reflect"                   ) return META_DESTRUCTIVE_SHADOWSPIRIT;
  if ( description == "+14 Critical Strike Rating and 1% Spell Reflect"                   ) return META_DESTRUCTIVE_SKYFIRE;
  if ( description == "+25 Critical Strike Rating and 1% Spell Reflect"                   ) return META_DESTRUCTIVE_SKYFLARE;
  if ( description == "+81 Stamina and Reduce Spell Damage Taken by 2%"                   ) return META_EFFULGENT_SHADOWSPIRIT;
  if ( description == "+54 Intellect and +2% Maximum Mana"                                ) return META_EMBER_SHADOWSPIRIT;
  if ( description == "+12 Intellect and +2% Maximum Mana"                                ) return META_EMBER_SKYFIRE;
  if ( description == "+21 Intellect and +2% Maximum Mana"                                ) return META_EMBER_SKYFLARE;
  if ( description == "+54 Critical Strike Rating and Reduces Snare/Root Duration by 10%" ) return META_ENIGMATIC_SHADOWSPIRIT;
  if ( description == "+21 Critical Strike Rating and Reduces Snare/Root Duration by 10%" ) return META_ENIGMATIC_SKYFLARE;
  if ( description == "+17 Critical Strike Rating and Reduces Snare/Root Duration by 10%" ) return META_ENIGMATIC_STARFLARE;
  if ( description == "+12 Critical Strike Rating and Reduces Snare/Root Duration by 10%" ) return META_ENIGMATIC_SKYFIRE;
  if ( description == "+21 Dodge Rating and +1% Shield Block Value"                       ) return META_ETERNAL_EARTHSIEGE;
  if ( description == "+81 Stamina and +5% Shield Block Value"                            ) return META_ETERNAL_SHADOWSPIRIT;
  if ( description == "+54 Mastery Rating and Minor Run Speed Increase"                   ) return META_FLEET_SHADOWSPIRIT;
  if ( description == "+54 Intellect and Silence Duration Reduced by 10%"                 ) return META_FORLORN_SHADOWSPIRIT;
  if ( description == "+21 Intellect and Silence Duration Reduced by 10%"                 ) return META_FORLORN_SKYFLARE;
  if ( description == "+17 Intellect and Silence Duration Reduced by 10%"                 ) return META_FORLORN_STARFLARE;
  if ( description == "+54 Critical Strike Rating and Fear Duration Reduced by 10%"       ) return META_IMPASSIVE_SHADOWSPIRIT;
  if ( description == "+21 Critical Strike Rating and Fear Duration Reduced by 10%"       ) return META_IMPASSIVE_SKYFLARE;
  if ( description == "+17 Critical Strike Rating and Fear Duration Reduced by 10%"       ) return META_IMPASSIVE_STARFLARE;
  if ( description == "+21 Intellect and Chance to restore mana on spellcast"             ) return META_INSIGHTFUL_EARTHSIEGE;
  if ( description == "+12 Intellect and Chance to restore mana on spellcast"             ) return META_INSIGHTFUL_EARTHSTORM;
  if ( description == "+21 Haste Rating and Sometimes Heal on Your Crits"                 ) return META_INVIGORATING_EARTHSIEGE;
  if ( description == "+21 Agility and 3% Increased Critical Damage"                      ) return META_RELENTLESS_EARTHSIEGE;
  if ( description == "+12 Agility and 3% Increased Critical Damage"                      ) return META_RELENTLESS_EARTHSTORM;
  if ( description == "+21 Critical Strike Rating and Stun Duration Reduced by 10%"       ) return META_PERSISTENT_EARTHSIEGE;
  if ( description == "+17 Critical Strike Rating and Stun Duration Reduced by 10%"       ) return META_PERSISTENT_EARTHSHATTER;
  if ( description == "+32 Stamina and Stun Duration Reduced by 10%"                      ) return META_POWERFUL_EARTHSIEGE;
  if ( description == "+26 Stamina and Stun Duration Reduced by 10%"                      ) return META_POWERFUL_EARTHSHATTER;
  if ( description == "+18 Stamina and Stun Duration Reduced by 10%"                      ) return META_POWERFUL_EARTHSTORM;
  if ( description == "+81 Stamina and Stun Duration Reduced by 10%"                      ) return META_POWERFUL_SHADOWSPIRIT;
  if ( description == "+81 Spirit and 3% Increased Critical Healing Effect"               ) return META_REVITALIZING_SHADOWSPIRIT;
  if ( description == "+54 Strength and 3% Increased Critical Damage"                     ) return META_REVERBERATING_SHADOWSPIRIT;
  if ( description == "+22 Spirit and 3% Increased Critical Healing Effect"               ) return META_REVITALIZING_SKYFLARE;
  if ( description == "+12 Critical Strike Rating and Minor Run Speed Increase"           ) return META_SWIFT_SKYFIRE;
  if ( description == "+21 Critical Strike Rating and Minor Run Speed Increase"           ) return META_SWIFT_SKYFLARE;
  if ( description == "+17 Critical Strike Rating and Minor Run Speed Increase"           ) return META_SWIFT_STARFLARE;
  if ( description == "Chance to Increase Melee/Ranged Attack Speed"                      ) return META_THUNDERING_SKYFLARE; //FIXME: Assuming the best one for now, since there's no way to tell by just the description
  if ( description == "+17 Intellect and Minor Run Speed Increase"                        ) return META_TIRELESS_STARFLARE;
  if ( description == "+21 Intellect and Minor Run Speed Increase"                        ) return META_TIRELESS_SKYFLARE;
  if ( description == "+21 Intellect and Stun Duration Reduced by 10%"                    ) return META_POWERFUL_EARTHSIEGE;
  if ( description == "+17 Intellect and Stun Duration Reduced by 10%"                    ) return META_POWERFUL_EARTHSHATTER;

  return META_GEM_NONE;
}

// armory_t::format =========================================================

std::string& armory_t::format( std::string& name, int format_type )
{
  if ( name.empty() ) return name;

  std::string buffer;

  switch ( format_type & FORMAT_CONVERT_MASK )
  {
  case FORMAT_UTF8_MASK:
    util_t::urlencode( name );
    break;
  case FORMAT_ASCII_MASK:
    util_t::str_to_utf8( name );
    break;
  }

  int size = ( int ) name.size();
  for ( int i=0; i < size; i++ )
  {
    unsigned char c = name[ i ];

    if ( c >= 0x80 )
    {
      continue;
    }
    else if ( isalpha( c ) )
    {
      switch ( format_type & FORMAT_ALL_NAME_MASK )
      {
      case FORMAT_GUILD_NAME_MASK:
        break;
      case FORMAT_CHAR_NAME_MASK:
        if ( i != 0 )
        {
          c = tolower( ( unsigned ) c );
        }
        break;
      default:
        c = tolower( ( unsigned ) c );
        break;
      }
    }
    else if ( c == ' ' )
    {
      c = '_';
    }
    else if ( ( c == '_' || c == '+' ) && i == 0 )
    {
      continue;
    }
    else if ( c != '_' &&
              c != '+' &&
              c != '.' &&
              c != '%' &&
              ! isdigit( c ) )
    {
      continue;
    }
    buffer += c;
  }
  name.swap( buffer );

  return name;
}
