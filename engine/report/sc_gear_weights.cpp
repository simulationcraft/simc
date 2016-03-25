// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

std::array<std::string, SCALE_METRIC_MAX> gear_weights::lootrank(
    const player_t& p )
{
  std::array<std::string, SCALE_METRIC_MAX> sa;

  for ( scale_metric_e sm = SCALE_METRIC_NONE; sm < SCALE_METRIC_MAX; sm++ )
  {
    std::string s = "http://www.guildox.com/go/wr.asp?";

    switch ( p.type )
    {
      case DEATH_KNIGHT:
        s += "Cla=2048";
        break;
      case DRUID:
        s += "Cla=1024";
        break;
      case HUNTER:
        s += "Cla=4";
        break;
      case MAGE:
        s += "Cla=128";
        break;
      case MONK:
        s += "Cla=4096";
        break;
      case PALADIN:
        s += "Cla=2";
        break;
      case PRIEST:
        s += "Cla=16";
        break;
      case ROGUE:
        s += "Cla=8";
        break;
      case SHAMAN:
        s += "Cla=64";
        break;
      case WARLOCK:
        s += "Cla=256";
        break;
      case WARRIOR:
        s += "Cla=1";
        break;
      default:
        assert( false );
        break;
    }

    switch ( p.type )
    {
      case WARRIOR:
      case PALADIN:
      case DEATH_KNIGHT:
        s += "&Art=1";
        break;
      case HUNTER:
      case SHAMAN:
        s += "&Art=2";
        break;
      case DRUID:
      case ROGUE:
      case MONK:
        s += "&Art=4";
        break;
      case MAGE:
      case PRIEST:
      case WARLOCK:
        s += "&Art=8";
        break;
      default:
        break;
    }

    /* FIXME: Commenting this out since this won't currently work the way we
    handle pandaren, and we don't currently care what faction people are anyway
    switch ( p -> race )
    {
    case RACE_PANDAREN_ALLIANCE:
    case RACE_NIGHT_ELF:
    case RACE_HUMAN:
    case RACE_GNOME:
    case RACE_DWARF:
    case RACE_WORGEN:
    case RACE_DRAENEI: s += "&F=A"; break;

    case RACE_PANDAREN_HORDE:
    case RACE_ORC:
    case RACE_TROLL:
    case RACE_UNDEAD:
    case RACE_BLOOD_ELF:
    case RACE_GOBLIN:
    case RACE_TAUREN: s += "&F=H"; break;

    case RACE_PANDAREN:
    default: break;
    }
    */
    bool positive_normalizing_value =
        p.scaling[ sm ].get_stat( p.normalize_by() ) >= 0;
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      double value = positive_normalizing_value
                         ? p.scaling[ sm ].get_stat( i )
                         : -p.scaling[ sm ].get_stat( i );
      if ( value == 0 )
        continue;

      const char* name;
      switch ( i )
      {
        case STAT_STRENGTH:
          name = "Str";
          break;
        case STAT_AGILITY:
          name = "Agi";
          break;
        case STAT_STAMINA:
          name = "Sta";
          break;
        case STAT_INTELLECT:
          name = "Int";
          break;
        case STAT_SPIRIT:
          name = "Spi";
          break;
        case STAT_SPELL_POWER:
          name = "spd";
          break;
        case STAT_ATTACK_POWER:
          name = "map";
          break;
        case STAT_EXPERTISE_RATING:
          name = "Exp";
          break;
        case STAT_HIT_RATING:
          name = "mhit";
          break;
        case STAT_CRIT_RATING:
          name = "mcr";
          break;
        case STAT_HASTE_RATING:
          name = "mh";
          break;
        case STAT_MASTERY_RATING:
          name = "Mr";
          break;
        case STAT_ARMOR:
          name = "Arm";
          break;
        case STAT_BONUS_ARMOR:
          name = "bar";
          break;
        case STAT_WEAPON_DPS:
          if ( HUNTER == p.type )
            name = "rdps";
          else
            name = "dps";
          break;
        case STAT_WEAPON_OFFHAND_DPS:
          name = "odps";
          break;
        default:
          name = nullptr;
          break;
      }

      if ( name )
      {
        str::format( s, "&%s=%.*f", name, p.sim->report_precision, value );
      }
    }

    // Set the trinket style choice
    switch ( p.specialization() )
    {
      case DEATH_KNIGHT_BLOOD:
      case DRUID_GUARDIAN:
      case MONK_BREWMASTER:
      case PALADIN_PROTECTION:
      case WARRIOR_PROTECTION:
        // Tank
        s += "&TF=1";
        break;

      case DEATH_KNIGHT_FROST:
      case DEATH_KNIGHT_UNHOLY:
      case DRUID_FERAL:
      case MONK_WINDWALKER:
      case PALADIN_RETRIBUTION:
      case ROGUE_ASSASSINATION:
      case ROGUE_OUTLAW:
      case ROGUE_SUBTLETY:
      case SHAMAN_ENHANCEMENT:
      case WARRIOR_ARMS:
      case WARRIOR_FURY:
        // Melee DPS
        s += "&TF=2";
        break;

      case HUNTER_BEAST_MASTERY:
      case HUNTER_MARKSMANSHIP:
      case HUNTER_SURVIVAL:
        // Ranged DPS
        s += "&TF=4";
        break;

      case DRUID_BALANCE:
      case MAGE_ARCANE:
      case MAGE_FIRE:
      case MAGE_FROST:
      case PRIEST_SHADOW:
      case SHAMAN_ELEMENTAL:
      case WARLOCK_AFFLICTION:
      case WARLOCK_DEMONOLOGY:
      case WARLOCK_DESTRUCTION:
        // Caster DPS
        s += "&TF=8";
        break;

      // Healer
      case DRUID_RESTORATION:
      case MONK_MISTWEAVER:
      case PALADIN_HOLY:
      case PRIEST_DISCIPLINE:
      case PRIEST_HOLY:
      case SHAMAN_RESTORATION:
        s += "&TF=16";
        break;

      default:
        break;
    }

    s += "&Gem=3";  // FIXME: Remove this when epic gems become available
    s += "&Ver=7";
    str::format( s, "&maxlv=%d", p.true_level );

    if ( p.items[ 0 ].parsed.data.id )
      s += "&t1=" + util::to_string( p.items[ 0 ].parsed.data.id );
    if ( p.items[ 1 ].parsed.data.id )
      s += "&t2=" + util::to_string( p.items[ 1 ].parsed.data.id );
    if ( p.items[ 2 ].parsed.data.id )
      s += "&t3=" + util::to_string( p.items[ 2 ].parsed.data.id );
    if ( p.items[ 4 ].parsed.data.id )
      s += "&t5=" + util::to_string( p.items[ 4 ].parsed.data.id );
    if ( p.items[ 5 ].parsed.data.id )
      s += "&t8=" + util::to_string( p.items[ 5 ].parsed.data.id );
    if ( p.items[ 6 ].parsed.data.id )
      s += "&t9=" + util::to_string( p.items[ 6 ].parsed.data.id );
    if ( p.items[ 7 ].parsed.data.id )
      s += "&t10=" + util::to_string( p.items[ 7 ].parsed.data.id );
    if ( p.items[ 8 ].parsed.data.id )
      s += "&t6=" + util::to_string( p.items[ 8 ].parsed.data.id );
    if ( p.items[ 9 ].parsed.data.id )
      s += "&t7=" + util::to_string( p.items[ 9 ].parsed.data.id );
    if ( p.items[ 10 ].parsed.data.id )
      s += "&t11=" + util::to_string( p.items[ 10 ].parsed.data.id );
    if ( p.items[ 11 ].parsed.data.id )
      s += "&t31=" + util::to_string( p.items[ 11 ].parsed.data.id );
    if ( p.items[ 12 ].parsed.data.id )
      s += "&t12=" + util::to_string( p.items[ 12 ].parsed.data.id );
    if ( p.items[ 13 ].parsed.data.id )
      s += "&t32=" + util::to_string( p.items[ 13 ].parsed.data.id );
    if ( p.items[ 14 ].parsed.data.id )
      s += "&t4=" + util::to_string( p.items[ 14 ].parsed.data.id );
    if ( p.items[ 15 ].parsed.data.id )
      s += "&t14=" + util::to_string( p.items[ 15 ].parsed.data.id );
    if ( p.items[ 16 ].parsed.data.id )
      s += "&t15=" + util::to_string( p.items[ 16 ].parsed.data.id );

    util::urlencode( s );

    sa[ sm ] = s;
  }
  return sa;
}

std::array<std::string, SCALE_METRIC_MAX> gear_weights::wowhead(
    const player_t& p )
{
  std::array<std::string, SCALE_METRIC_MAX> sa;

  for ( scale_metric_e sm = SCALE_METRIC_NONE; sm < SCALE_METRIC_MAX; sm++ )
  {
    bool first = true;

    std::string s = "http://";

    if ( maybe_ptr( p.dbc.ptr ) )
      s += "ptr.";
    else
      s += "www.";

    s += "wowhead.com/?items&amp;filter=";

    switch ( p.type )
    {
      case DEATH_KNIGHT:
        s += "ub=6;";
        break;
      case DRUID:
        s += "ub=11;";
        break;
      case HUNTER:
        s += "ub=3;";
        break;
      case MAGE:
        s += "ub=8;";
        break;
      case PALADIN:
        s += "ub=2;";
        break;
      case PRIEST:
        s += "ub=5;";
        break;
      case ROGUE:
        s += "ub=4;";
        break;
      case SHAMAN:
        s += "ub=7;";
        break;
      case WARLOCK:
        s += "ub=9;";
        break;
      case WARRIOR:
        s += "ub=1;";
        break;
      case MONK:
        s += "ub=10;";
        break;
      default:
        assert( 0 );
        break;
    }

    // Restrict wowhead to rare gems. When epic gems become
    // available:"gm=4;gb=1;"
    s += "gm=3;gb=1;";

    // Min ilvl of 600 (sensible for current raid tier).
    s += "minle=600;";

    // Filter to the appropriately flagged loot for the specialization.
    switch ( p.role )
    {
      case ROLE_ATTACK:
        if ( p.type == DEATH_KNIGHT || p.type == PALADIN || p.type == WARRIOR )
          s += "ro=3;";
        else
          s += "ro=1;";
        break;
      case ROLE_SPELL:
        s += "ro=2;";
        break;
      case ROLE_HEAL:
        s += "ro=4;";
        break;
      case ROLE_TANK:
        s += "ro=5;";
        break;
      default:
        break;
    }

    std::string id_string    = "";
    std::string value_string = "";

    bool positive_normalizing_value =
        p.scaling[ sm ].get_stat( p.normalize_by() ) >= 0;

    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      double value = positive_normalizing_value
                         ? p.scaling[ sm ].get_stat( i )
                         : -p.scaling[ sm ].get_stat( i );
      if ( value == 0 )
        continue;

      int id = 0;
      switch ( i )
      {
        case STAT_STRENGTH:
          id = 20;
          break;
        case STAT_AGILITY:
          id = 21;
          break;
        case STAT_STAMINA:
          id = 22;
          break;
        case STAT_INTELLECT:
          id = 23;
          break;
        case STAT_SPIRIT:
          id = 24;
          break;
        case STAT_SPELL_POWER:
          id = 123;
          break;
        case STAT_ATTACK_POWER:
          id = 77;
          break;
        case STAT_CRIT_RATING:
          id = 96;
          break;
        case STAT_HASTE_RATING:
          id = 103;
          break;
        case STAT_ARMOR:
          id = 41;
          break;
        case STAT_BONUS_ARMOR:
          id = 109;
          break;
        case STAT_MASTERY_RATING:
          id = 170;
          break;
        case STAT_VERSATILITY_RATING:
          id = 215;
          break;
        case STAT_WEAPON_DPS:
          if ( HUNTER == p.type )
            id = 138;
          else
            id = 32;
          break;
        default:
          break;
      }

      if ( id )
      {
        if ( !first )
        {
          id_string += ":";
          value_string += ":";
        }
        first = false;

        str::format( id_string, "%d", id );
        str::format( value_string, "%.*f", p.sim->report_precision, value );
      }
    }

    s += "wt=" + id_string + ";";
    s += "wtv=" + value_string + ";";

    sa[ sm ] = s;
  }
  return sa;
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
    s += "\": ";

    bool positive_normalizing_value =
        p.scaling[ sm ].get_stat( p.normalize_by() ) >= 0;
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      double value = positive_normalizing_value
                         ? p.scaling[ sm ].get_stat( i )
                         : -p.scaling[ sm ].get_stat( i );
      if ( value == 0 )
        continue;

      const char* name = nullptr;
      switch ( i )
      {
        case STAT_STRENGTH:
          name = "Strength";
          break;
        case STAT_AGILITY:
          name = "Agility";
          break;
        case STAT_STAMINA:
          name = "Stamina";
          break;
        case STAT_INTELLECT:
          name = "Intellect";
          break;
        case STAT_SPIRIT:
          name = "Spirit";
          break;
        case STAT_SPELL_POWER:
          name = "SpellPower";
          break;
        case STAT_ATTACK_POWER:
          name = "Ap";
          break;
        case STAT_CRIT_RATING:
          name = "CritRating";
          break;
        case STAT_HASTE_RATING:
          name = "HasteRating";
          break;
        case STAT_ARMOR:
          name = "Armor";
          break;
        case STAT_BONUS_ARMOR:
          name = "BonusArmor";
          break;
        case STAT_MASTERY_RATING:
          name = "MasteryRating";
          break;
        case STAT_VERSATILITY_RATING:
          name = "Versatility";
          break;
        case STAT_WEAPON_DPS:
          name = "Dps";
          break;
        default:
          break;
      }

      if ( name )
      {
        if ( !first )
        {
          s += ",";
        }
        first = false;
        str::format( s, " %s=%.*f", name, p.sim->report_precision, value );
      }
    }
    s += " )";
    sa[ sm ] = s;
  }

  return sa;
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

    // This next section is sort of unwieldly, I may move this to external
    // functions
    std::string spec;

    // Player type
    switch ( p.type )
    {
      case DEATH_KNIGHT:
        spec += "DeathKnight";
        break;
      case DRUID:
        spec += "Druid";
        break;
      case HUNTER:
        spec += "Hunter";
        break;
      case MAGE:
        spec += "Mage";
        break;
      case PALADIN:
        spec += "Paladin";
        break;
      case PRIEST:
        spec += "Priest";
        break;
      case ROGUE:
        spec += "Rogue";
        break;
      case SHAMAN:
        spec += "Shaman";
        break;
      case WARLOCK:
        spec += "Warlock";
        break;
      case WARRIOR:
        spec += "Warrior";
        break;
      case MONK:
        spec += "Monk";
        break;
      // if this isn't a player, the AMR link is useless
      default:
        assert( 0 );
        break;
    }
    // Player spec
    switch ( p.specialization() )
    {
      case DEATH_KNIGHT_BLOOD:
        spec += "Blood";
        break;
      case DEATH_KNIGHT_FROST:
      {
        if ( p.main_hand_weapon.type == WEAPON_2H )
        {
          spec += "Frost2H";
          break;
        }
        else
        {
          spec += "FrostDw";
          break;
        }
      }
      case DEATH_KNIGHT_UNHOLY:
        spec += "Unholy";
        break;
      case DRUID_BALANCE:
        spec += "Balance";
        break;
      case DRUID_FERAL:
        spec += "Feral";
        break;
      case DRUID_GUARDIAN:
        spec += "Guardian";
        break;
      case DRUID_RESTORATION:
        spec += "Restoration";
        break;
      case HUNTER_BEAST_MASTERY:
        spec += "BeastMastery";
        break;
      case HUNTER_MARKSMANSHIP:
        spec += "Marksmanship";
        break;
      case HUNTER_SURVIVAL:
        spec += "Survival";
        break;
      case MAGE_ARCANE:
        spec += "Arcane";
        break;
      case MAGE_FIRE:
        spec += "Fire";
        break;
      case MAGE_FROST:
        spec += "Frost";
        break;
      case MONK_BREWMASTER:
      {
        if ( p.main_hand_weapon.type == WEAPON_STAFF ||
             p.main_hand_weapon.type == WEAPON_POLEARM )
        {
          spec += "Brewmaster2h";
          break;
        }
        else
        {
          spec += "BrewmasterDw";
          break;
        }
      }
      case MONK_MISTWEAVER:
        spec += "Mistweaver";
        break;
      case MONK_WINDWALKER:
      {
        if ( p.main_hand_weapon.type == WEAPON_STAFF ||
             p.main_hand_weapon.type == WEAPON_POLEARM )
        {
          spec += "Windwalker2h";
          break;
        }
        else
        {
          spec += "WindwalkerDw";
          break;
        }
      }
      case PALADIN_HOLY:
        spec += "Holy";
        break;
      case PALADIN_PROTECTION:
        spec += "Protection";
        break;
      case PALADIN_RETRIBUTION:
        spec += "Retribution";
        break;
      case PRIEST_DISCIPLINE:
        spec += "Discipline";
        break;
      case PRIEST_HOLY:
        spec += "Holy";
        break;
      case PRIEST_SHADOW:
        spec += "Shadow";
        break;
      case ROGUE_ASSASSINATION:
        spec += "Assassination";
        break;
      case ROGUE_OUTLAW:
        spec += "Combat";
        break;
      case ROGUE_SUBTLETY:
        spec += "Subtlety";
        break;
      case SHAMAN_ELEMENTAL:
        spec += "Elemental";
        break;
      case SHAMAN_ENHANCEMENT:
        spec += "Enhancement";
        break;
      case SHAMAN_RESTORATION:
        spec += "Restoration";
        break;
      case WARLOCK_AFFLICTION:
        spec += "Affliction";
        break;
      case WARLOCK_DEMONOLOGY:
        spec += "Demonology";
        break;
      case WARLOCK_DESTRUCTION:
        spec += "Destruction";
        break;
      case WARRIOR_ARMS:
        spec += "Arms";
        break;
      case WARRIOR_FURY:
      {
        if ( p.main_hand_weapon.type == WEAPON_SWORD_2H ||
             p.main_hand_weapon.type == WEAPON_AXE_2H ||
             p.main_hand_weapon.type == WEAPON_MACE_2H ||
             p.main_hand_weapon.type == WEAPON_POLEARM )
        {
          spec += "Fury2H";
          break;
        }
        else
        {
          spec += "Fury";
          break;
        }
      }
      case WARRIOR_PROTECTION:
        spec += "Protection";
        break;

      // if this is a pet or an unknown spec, the AMR link is pointless anyway
      default:
        assert( 0 );
        break;
    }

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
        p.scaling_normalized[ sm ].get_stat( p.normalize_by() ) >= 0;

    // AMR accepts a max precision of 2 decimal places
    ss.precision( std::min( p.sim->report_precision + 1, 2 ) );

    // flag for skipping the first comma
    bool skipFirstComma = false;

    // loop through stats and append the relevant ones to the URL
    for ( stat_e i = STAT_NONE; i < STAT_MAX; ++i )
    {
      // get stat weight value
      double value = positive_normalizing_value
                         ? p.scaling_normalized[ sm ].get_stat( i )
                         : -p.scaling_normalized[ sm ].get_stat( i );

      // if the weight is negative or AMR won't recognize the stat type string,
      // skip this stat
      if ( value <= 0 ||
           util::str_compare_ci( util::stat_type_askmrrobot( i ), "unknown" ) )
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
      ss << util::stat_type_askmrrobot( i ) << ':' << std::fixed << value;
    }

    // softweights, softcaps, hardcaps would go here if we supported them

    sa[ sm ] = util::encode_html( ss.str() );
  }
  return sa;
}
