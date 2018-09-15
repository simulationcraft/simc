// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace
{
const int wowhead_min_item_level = 600;

const char* wowhead_domain_name( bool ptr )
{
  if ( maybe_ptr( ptr ) )
  {
    return "ptr.wowhead.com";
  }
  return "www.wowhead.com";
}

int wowhead_role( player_e type, role_e role )
{
  // {0: "N/A", 1: "Agility DPS", 2: "Intellect DPS", 3: "Strength DPS", 4:
  // "Healer", 5: "Tank", 6: "Unknown"}
  switch ( role )
  {
    case ROLE_ATTACK:
      if ( type == DEATH_KNIGHT || type == PALADIN || type == WARRIOR )
        return 3;
      else
        return 1;
    case ROLE_SPELL:
      return 2;
    case ROLE_HEAL:
      return 4;
    case ROLE_TANK:
      return 5;
    default:
      break;
  }
  return 6;
}

int wowhead_stat_id( stat_e i, player_e type )
{
  switch ( i )
  {
    case STAT_STRENGTH:
      return 20;
    case STAT_AGILITY:
      return 21;
    case STAT_STAMINA:
      return 22;
    case STAT_INTELLECT:
      return 23;
    case STAT_SPIRIT:
      return 24;
    case STAT_SPELL_POWER:
      return 123;
    case STAT_ATTACK_POWER:
      return 77;
    case STAT_CRIT_RATING:
      return 96;
    case STAT_HASTE_RATING:
      return 103;
    case STAT_ARMOR:
      return 41;
    case STAT_BONUS_ARMOR:
      return 109;
    case STAT_MASTERY_RATING:
      return 170;
    case STAT_VERSATILITY_RATING:
      return 215;
    case STAT_WEAPON_DPS:
      if ( type == HUNTER )
        return 138;
      else
        return 32;
    default:
      break;
  }
  return 0;
}

}  // unnamed namespace

std::array<std::string, SCALE_METRIC_MAX> gear_weights::wowhead(
    const player_t& p )
{
  std::array<std::string, SCALE_METRIC_MAX> sa;

  for ( scale_metric_e sm = SCALE_METRIC_NONE; sm < SCALE_METRIC_MAX; sm++ )
  {
    bool first = true;

    std::stringstream s;
    s << "https://" << wowhead_domain_name( p.dbc.ptr ) << "/?items&amp;filter=";
    s << "ub=" << util::class_id( p.type ) << ";";

    // Restrict wowhead to rare gems. When epic gems become
    // available:"gm=4;gb=1;"
    s << "gm=3;gb=1;";

    s << "minle=" << wowhead_min_item_level << ";";

    // Filter to the appropriately flagged loot for the specialization.
    s << "ro=" << wowhead_role( p.type, p.primary_role() ) << ";";

    std::string id_string    = "";
    std::string value_string = "";

    bool positive_normalizing_value =
        p.scaling->scaling[ sm ].get_stat( p.normalize_by() ) >= 0;

    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      double value = positive_normalizing_value
                         ? p.scaling->scaling[ sm ].get_stat( i )
                         : -p.scaling->scaling[ sm ].get_stat( i );
      if ( value == 0 )
        continue;

      int id = wowhead_stat_id( i, p.type );

      if ( id )
      {
        if ( !first )
        {
          id_string += ":";
          value_string += ":";
        }
        first = false;

        id_string += fmt::format( "{:d}", id );
        value_string += fmt::format( "{:.{}f}", value, p.sim->report_precision );
      }
    }

    s << "wt=" + id_string << ";";
    s << "wtv=" + value_string << ";";

    sa[ sm ] = s.str();
  }
  return sa;
}

namespace
{
const char* pawn_stat_name( stat_e stat )
{
  switch ( stat )
  {
    case STAT_STRENGTH:
      return "Strength";
    case STAT_AGILITY:
      return "Agility";
    case STAT_STAMINA:
      return "Stamina";
    case STAT_INTELLECT:
      return "Intellect";
    case STAT_SPIRIT:
      return "Spirit";
    case STAT_SPELL_POWER:
      return "SpellPower";
    case STAT_ATTACK_POWER:
      return "Ap";
    case STAT_CRIT_RATING:
      return "CritRating";
    case STAT_HASTE_RATING:
      return "HasteRating";
    case STAT_ARMOR:
      return "Armor";
    case STAT_BONUS_ARMOR:
      return "BonusArmor";
    case STAT_MASTERY_RATING:
      return "MasteryRating";
    case STAT_VERSATILITY_RATING:
      return "Versatility";
    case STAT_WEAPON_DPS:
      return "Dps";
    case STAT_WEAPON_OFFHAND_DPS:
      return "OffHandDps";
    case STAT_LEECH_RATING:
      return "Leech";
    default:
      break;
  }
  return nullptr;
}
}

std::array<std::string, SCALE_METRIC_MAX> gear_weights::pawn(
    const player_t& p )
{
  std::array<std::string, SCALE_METRIC_MAX> sa;

  for ( scale_metric_e sm = SCALE_METRIC_NONE; sm < SCALE_METRIC_MAX; sm++ )
  {
    bool first    = true;
    std::string s = "( Pawn: v1: \"";
    s += p.name();
    s += "-";
    s += util::spec_string_no_class(p);
    s += "\": ";
    s += "Class=";
    std::string name = util::player_type_string( p.type );
    name[0] = toupper( name[0] );
    s += name;
    s += ", Spec=";
    s += util::spec_string_no_class( p );
    s += ", ";

    bool positive_normalizing_value =
        p.scaling->scaling[ sm ].get_stat( p.normalize_by() ) >= 0;
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      double value = positive_normalizing_value
                         ? p.scaling->scaling[ sm ].get_stat( i )
                         : -p.scaling->scaling[ sm ].get_stat( i );
      if ( value == 0 )
        continue;

      const char* name = pawn_stat_name( i );
      if ( name )
      {
        if ( !first )
        {
          s += ",";
        }
        first = false;
        s += fmt::format(" {}={:.{}f}", name, value, p.sim->report_precision );
      }
    }
    s += " )";
    sa[ sm ] = s;
  }

  return sa;
}

namespace
{
const char* askmrrobot_player_type_name( player_e type )
{
  // Player type
  switch ( type )
  {
    case DEATH_KNIGHT:
      return "DeathKnight";
    case DEMON_HUNTER:
      return "DemonHunter";
    case DRUID:
      return "Druid";
    case HUNTER:
      return "Hunter";
    case MAGE:
      return "Mage";
    case PALADIN:
      return "Paladin";
    case PRIEST:
      return "Priest";
    case ROGUE:
      return "Rogue";
    case SHAMAN:
      return "Shaman";
    case WARLOCK:
      return "Warlock";
    case WARRIOR:
      return "Warrior";
    case MONK:
      return "Monk";
    default:
      // if this isn't a player, the AMR link is useless
      assert( false );
      break;
  }
  return nullptr;
}

const char* askmrrobot_stat_type( stat_e stat )
{
  switch ( stat )
  {
    case STAT_STRENGTH:
      return "Strength";
    case STAT_AGILITY:
      return "Agility";
    case STAT_STAMINA:
      return "Stamina";
    case STAT_INTELLECT:
      return "Intellect";
    case STAT_SPIRIT:
      return "Spirit";

    case STAT_ATTACK_POWER:
      return "AttackPower";
    case STAT_SPELL_POWER:
      return "SpellPower";
    case STAT_CRIT_RATING:
      return "CriticalStrike";
    case STAT_HASTE_RATING:
      return "Haste";
    case STAT_ARMOR:
      return "Armor";
    case STAT_RESILIENCE_RATING:
      return "PvpResilience";
    case STAT_MASTERY_RATING:
      return "Mastery";
    case STAT_VERSATILITY_RATING:
      return "Versatility";
    case STAT_BONUS_ARMOR:
      return "BonusArmor";
    case STAT_PVP_POWER:
      return "PvpPower";
    case STAT_WEAPON_DPS:
      return "MainHandDps";
    case STAT_WEAPON_OFFHAND_DPS:
      return "OffHandDps";
    case STAT_AVOIDANCE_RATING:
      return "Avoidance";
    case STAT_LEECH_RATING:
      return "Leech";
    case STAT_SPEED_RATING:
      return "MovementSpeed";

    default:
      return "unknown";
  }
}
}

std::array<std::string, SCALE_METRIC_MAX> gear_weights::askmrrobot(
    const player_t& p )
{
  std::array<std::string, SCALE_METRIC_MAX> sa;

  for ( scale_metric_e sm = SCALE_METRIC_NONE; sm < SCALE_METRIC_MAX; sm++ )
  {
    bool use_generic = false;
    std::stringstream ss;
    // AMR's origin_str provided from their SimC export is a valid base for
    // appending stat weights.
    // If the origin has askmrrobot in it, just use that, but replace
    // wow/player/ with wow/optimize/
    if ( util::str_in_str_ci( p.origin_str, "askmrrobot" ) )
    {
      std::string origin = p.origin_str;
      util::replace_all( origin, "wow/player", "wow/optimize" );
      ss << origin;
    }
    // otherwise, we need to construct it from whatever information we have
    // available
    else
    {
      // format is base (below) /region/server/name#spec=[spec];[weightsHash]
      ss << "http://www.askmrrobot.com/wow";

      // Use valid names if we are provided those
      if ( !p.region_str.empty() && !p.server_str.empty() &&
           !p.name_str.empty() )
        ss << "/optimize/" << p.region_str << '/' << p.server_str << '/'
           << p.name_str;

      // otherwise try to reconstruct it from the origin string
      else
      {
        std::string region_str, server_str, name_str;
        if ( util::parse_origin( region_str, server_str, name_str,
                                 p.origin_str ) )
          ss << "/optimize/" << region_str << '/' << server_str << '/'
             << name_str;
        // if we can't reconstruct, default to a generic character
        // this uses the base followed by /[spec]#[weightsHash]
        else
        {
          use_generic = true;
          ss << "/best-in-slot/generic/";
        }
      }
    }

    std::string spec;
    spec += askmrrobot_player_type_name( p.type );
    spec += util::spec_string_no_class( p );

    // if we're using a generic character, need spec
    if ( use_generic )
    {
      util::tolower( spec );
      ss << spec;
    }

    ss << "#spec=" << spec << ";";

    // add weights
    ss << "weights=";

    // check for negative normalizer
    bool positive_normalizing_value =
        p.scaling->scaling_normalized[ sm ].get_stat( p.normalize_by() ) >= 0;

    // AMR accepts a max precision of 2 decimal places
    ss.precision( std::min( p.sim->report_precision + 1, 2 ) );

    // flag for skipping the first comma
    bool skipFirstComma = false;

    // loop through stats and append the relevant ones to the URL
    for ( stat_e i = STAT_NONE; i < STAT_MAX; ++i )
    {
      // get stat weight value
      double value = positive_normalizing_value
                         ? p.scaling->scaling_normalized[ sm ].get_stat( i )
                         : -p.scaling->scaling_normalized[ sm ].get_stat( i );

      // if the weight is negative or AMR won't recognize the stat type string,
      // skip this stat
      if ( value <= 0 ||
           util::str_compare_ci( askmrrobot_stat_type( i ), "unknown" ) )
        continue;

      // skip the first comma
      if ( skipFirstComma )
        ss << ',';
      skipFirstComma = true;

      // AMR enforces certain bounds on stats, cap at 9.99 for regular and 99.99
      // for weapon DPS
      if ( ( i == STAT_WEAPON_DPS || i == STAT_WEAPON_OFFHAND_DPS ) &&
           value > 99.99 )
        value = 99.99;
      else if ( value > 9.99 )
        value = 9.99;

      // append the stat weight to the URL
      ss << askmrrobot_stat_type( i ) << ':' << std::fixed << value;
    }

    // softweights, softcaps, hardcaps would go here if we supported them

    sa[ sm ] = util::encode_html( ss.str() );
  }
  return sa;
}
