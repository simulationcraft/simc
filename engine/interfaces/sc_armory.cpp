// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // ANONYMOUS NAMESPACE ==========================================

// stat_search ==============================================================

static void stat_search( std::string&              encoding_str,
                         std::vector<std::string>& description_tokens,
                         stat_type_e               type,
                         const std::string&        stat_str )
{
  std::vector<std::string> stat_tokens;
  size_t num_stats = util_t::string_split( stat_tokens, stat_str, " " );
  size_t num_descriptions = description_tokens.size();

  for ( size_t i = 0; i < num_descriptions; i++ )
  {
    bool match = true;

    for ( size_t j = 0; j < num_stats && match; j++ )
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
           ( util_t::is_number( description_tokens[ i-1 ] ) ) )
      {
        value_str = description_tokens[ i-1 ];
      }
      if ( ( ( i + num_stats + 1 ) < num_descriptions ) &&
           ( description_tokens[ i + num_stats ] == "by" ) &&
           ( util_t::is_number( description_tokens[ i + num_stats + 1 ] ) ) )
      {
        value_str = description_tokens[ i + num_stats + 1 ];
      }

      if ( ! value_str.empty() )
      {
        encoding_str += '_' + value_str + util_t::stat_type_abbrev( type );
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
  util_t::armory_format( buffer );

  if ( is_proc_description( buffer ) )
    return;

  std::vector<std::string> splits;
  util_t::string_split( splits, buffer, "_." );

  stat_search( encoding_str, splits, STAT_ALL,  "all stats" );
  stat_search( encoding_str, splits, STAT_ALL,  "to all stats" );

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

