// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "report_helper.hpp"

#include "buff/sc_buff.hpp"
#include "dbc/dbc.hpp"
#include "dbc/sc_spell_info.hpp"
#include "dbc/spell_query/spell_data_expr.hpp"
#include "item/item.hpp"
#include "player/pet.hpp"
#include "player/sc_player.hpp"
#include "report/gear_weights.hpp"
#include "sim/sc_sim.hpp"
#include "sim/scale_factor_control.hpp"
#include "util/xml.hpp"

// ==========================================================================
// Report
// ==========================================================================

namespace
{  // UNNAMED NAMESPACE

struct buff_is_dynamic
{
  bool operator()( const buff_t* b ) const
  {
    if ( !b->quiet && b->avg_start.sum() && !b->constant )
      return false;

    return true;
  }
};

struct buff_is_constant
{
  bool operator()( const buff_t* b ) const
  {
    if ( !b->quiet && b->avg_start.sum() && b->constant )
      return false;

    return true;
  }
};

struct buff_comp
{
  bool operator()( const buff_t* i, const buff_t* j )
  {
    // Aura&Buff / Pet
    if ( ( !i->player || !i->player->is_pet() ) && j->player && j->player->is_pet() )
      return true;
    // Pet / Aura&Buff
    else if ( i->player && i->player->is_pet() && ( !j->player || !j->player->is_pet() ) )
      return false;
    // Pet / Pet
    else if ( i->player && i->player->is_pet() && j->player && j->player->is_pet() )
    {
      if ( i->player->name_str.compare( j->player->name_str ) == 0 )
        return ( i->name_str.compare( j->name_str ) < 0 );
      else
        return ( i->player->name_str.compare( j->player->name_str ) < 0 );
    }

    return ( i->name_str.compare( j->name_str ) < 0 );
  }
};

class tooltip_parser_t
{
  struct error
  {
  };

  static const bool PARSE_DEBUG = true;

  const spell_data_t& default_spell;
  const dbc_t& dbc;
  const player_t* player;  // For spell query tags (e.g., "$?s9999[Text if you
                           // have spell 9999][Text if you do not.]")
  const int level;

  const std::string& text;
  std::string::const_iterator pos;

  std::string result;

  unsigned parse_unsigned()
  {
    unsigned u = 0;
    while ( pos != text.end() && isdigit( *pos ) )
      u = u * 10 + *pos++ - '0';
    return u;
  }

  unsigned parse_effect_number()
  {
    if ( pos == text.end() || *pos < '1' || *pos > '9' )
      throw error();
    return *pos++ - '0';
  }

  const spell_data_t* parse_spell()
  {
    unsigned id           = parse_unsigned();
    const spell_data_t* s = dbc.spell( id );
    if ( s->id() != id )
      throw error();
    return s;
  }

  std::string parse_scaling( const spell_data_t& spell, double multiplier = 1.0 )
  {
    if ( pos == text.end() || *pos != 's' )
      throw error();
    ++pos;

    unsigned effect_number = parse_effect_number();
    if ( effect_number == 0 || effect_number > spell.effect_count() )
      throw error();

    if ( level > MAX_LEVEL )
      throw error();

    const spelleffect_data_t& effect = spell.effectN( effect_number );
    bool show_scale_factor           = effect.type() != E_APPLY_AURA;
    double s_min                     = dbc.effect_min( effect.id(), level );
    double s_max                     = dbc.effect_max( effect.id(), level );
    if ( s_min < 0 && s_max == s_min )
      s_max = s_min = -s_min;
    else if ( ( player && effect.type() == E_SCHOOL_DAMAGE && ( spell.get_school_type() & SCHOOL_MAGIC_MASK ) != 0 ) ||
              ( player && effect.type() == E_HEAL ) )
    {
      double power = effect.sp_coeff() * player->initial.stats.spell_power;
      s_min += power;
      s_max += power;
      show_scale_factor = false;
    }
    std::string result = util::to_string( util::round( multiplier * s_min ) );
    if ( s_max != s_min )
    {
      result += " to ";
      result += util::to_string( util::round( multiplier * s_max ) );
    }
    if ( show_scale_factor && effect.sp_coeff() )
    {
      result += " + ";
      result += util::to_string( 100 * multiplier * effect.sp_coeff(), 1 );
      result += '%';
    }

    return result;
  }

public:
  tooltip_parser_t( const dbc_t& d, int l, const spell_data_t& s, const std::string& t )
    : default_spell( s ), dbc( d ), player( nullptr ), level( l ), text( t ), pos( t.begin() )
  {
  }

  tooltip_parser_t( const player_t& p, const spell_data_t& s, const std::string& t )
    : default_spell( s ), dbc( *p.dbc ), player( &p ), level( p.true_level ), text( t ), pos( t.begin() )
  {
  }

  std::string parse();
};

std::string tooltip_parser_t::parse()
{
  while ( pos != text.end() )
  {
    while ( pos != text.end() && *pos != '$' )
      result += *pos++;
    if ( pos == text.end() )
      break;
    std::string::const_iterator lastpos = pos++;

    try
    {
      if ( pos == text.end() )
        throw error();

      const spell_data_t* spell = &default_spell;
      if ( isdigit( *pos ) )
      {
        spell = parse_spell();
        if ( pos == text.end() )
          throw error();
      }

      std::string replacement_text;
      switch ( *pos )
      {
        case 'd':
        {
          ++pos;
          timespan_t d = spell->duration();
          if ( d < timespan_t::from_seconds( 1 ) )
          {
            replacement_text = util::to_string( d.total_millis() );
            replacement_text += " milliseconds";
          }
          else if ( d > timespan_t::from_seconds( 1 ) )
          {
            replacement_text = util::to_string( d.total_seconds() );
            replacement_text += " seconds";
          }
          else
            replacement_text = "1 second";
          break;
        }

        case 'h':
        {
          ++pos;
          replacement_text = util::to_string( 100 * spell->proc_chance() );
          break;
        }

        case 'm':
        {
          ++pos;
          if ( parse_effect_number() <= spell->effect_count() )
            replacement_text = util::to_string( spell->effectN( parse_effect_number() ).base_value() );
          else
            replacement_text = util::to_string( 0 );
          break;
        }

        case 's':
          replacement_text = parse_scaling( *spell );
          break;

        case 't':
        {
          ++pos;
          if ( parse_effect_number() <= spell->effect_count() )
            replacement_text = util::to_string( spell->effectN( parse_effect_number() ).period().total_seconds() );
          else
            replacement_text = util::to_string( 0 );
          break;
        }

        case 'u':
        {
          ++pos;
          replacement_text = util::to_string( spell->max_stacks() );
          break;
        }

        case '?':
        {
          ++pos;
          if ( pos == text.end() || *pos != 's' )
            throw error();
          ++pos;
          spell          = parse_spell();
          bool has_spell = false;
          if ( player )
          {
            has_spell = player->find_class_spell( spell->name_cstr() )->ok();
          }
          replacement_text = has_spell ? "true" : "false";
          break;
        }

        case '*':
        {
          ++pos;
          unsigned m = parse_unsigned();

          if ( pos == text.end() || *pos != ';' )
            throw error();
          ++pos;

          replacement_text = parse_scaling( *spell, m );
          break;
        }

        case '/':
        {
          ++pos;
          unsigned m = parse_unsigned();

          if ( pos == text.end() || *pos != ';' )
            throw error();
          ++pos;

          replacement_text = parse_scaling( *spell, 1.0 / m );
          break;
        }

        case '@':
        {
          ++pos;
          if ( text.compare( pos - text.begin(), 9, "spelldesc" ) )
            throw error();
          pos += 9;

          spell = parse_spell();
          if ( !spell )
            throw error();
          assert( player );

          // References in tooltips/descriptions don't seem to form DAG, check
          // for cycles of length 1 here (which should hopefully be enough).
          if ( spell->id() != default_spell.id() )
          {
            const auto& spell_text = dbc.spell_text( spell->id() );
            replacement_text = report_helper::pretty_spell_text( *spell, spell_text.desc(), *player );
          }
          break;
        }

        default:
          throw error();
      }

      if ( PARSE_DEBUG )
      {
        result += '{';
        result.append( lastpos, pos );
        result += '=';
      }

      result += replacement_text;

      if ( PARSE_DEBUG )
        result += '}';
    }
    catch ( error& )
    {
      result.append( lastpos, pos );
    }
  }

  return result;
}

}  // namespace

std::string report_helper::pretty_spell_text( const spell_data_t& default_spell, const std::string& text,
                                              const player_t& p )
{
  return tooltip_parser_t( p, default_spell, text ).parse();
}

// report::check_gear =======================================================

bool report_helper::check_gear( player_t& p, sim_t& sim )
{
  int max_ilevel_allowed         = 0;
  int max_azerite_ilevel_allowed = 0;
  int hoa_ilevel                 = 0;
  unsigned hoa_level             = 0;
  std::string tier_name          = "";
  int third_ring_traits          = 2;
  int max_cloak_ilevel           = 0;

  if ( p.report_information.save_str.find( "PR" ) != std::string::npos )
  {
    max_ilevel_allowed         = 340;
    max_azerite_ilevel_allowed = max_ilevel_allowed + 5;
    hoa_ilevel                 = 347;
    hoa_level                  = 27;
    tier_name                  = "PR";
    third_ring_traits          = 1;
  }
  else if ( p.report_information.save_str.find( "DS" ) != std::string::npos )
  {
    max_ilevel_allowed         = 485;
    max_azerite_ilevel_allowed = max_ilevel_allowed + 5;
    hoa_ilevel                 = 493;
    hoa_level                  = 80;
    tier_name                  = "DS";
    max_cloak_ilevel           = 500;
  }
  else if ( p.report_information.save_str.find( "T25" ) != std::string::npos )
  {
    max_ilevel_allowed         = 485;
    max_azerite_ilevel_allowed = max_ilevel_allowed + 5;
    hoa_ilevel                 = 493;
    hoa_level                  = 80;
    tier_name                  = "T25";
    max_cloak_ilevel           = 500;
  }
  else
  {
    return true;
  }

  const unsigned azerite_tiers = 4;

  unsigned whitelisted_items[] = {
      152636,         // Endless Tincture of Fractional Power
      152632,         // Surging Alchemist Stone
      159125, 159126  // Darkmoon Deck: Fathoms, Darkmoon Deck: Squalls
  };

  const slot_e SLOT_OUT_ORDER[] = {
      SLOT_HEAD,      SLOT_NECK,      SLOT_SHOULDERS, SLOT_BACK,     SLOT_CHEST,  SLOT_SHIRT,    SLOT_TABARD,
      SLOT_WRISTS,    SLOT_HANDS,     SLOT_WAIST,     SLOT_LEGS,     SLOT_FEET,   SLOT_FINGER_1, SLOT_FINGER_2,
      SLOT_TRINKET_1, SLOT_TRINKET_2, SLOT_MAIN_HAND, SLOT_OFF_HAND, SLOT_RANGED,
  };

  const slot_e SLOT_DUPLICATES[] = {
      SLOT_FINGER_1,
      SLOT_FINGER_2,
      SLOT_TRINKET_1,
      SLOT_TRINKET_2,
  };

  for ( auto& slot : SLOT_OUT_ORDER )
  {
    item_t& item = p.items[ slot ];

    // Heart of Azeroth
    if ( slot == SLOT_NECK )
    {
      // Check azerite_level
      if ( item.parsed.azerite_level != hoa_level )
        sim.error( "Player {} has HoA of level {}, level for {} should be {}.\n", p.name(), item.parsed.azerite_level,
                   tier_name, hoa_level );
      // Check final item level (since it's not only computed from azerite_level)
      if ( item.parsed.data.level != hoa_ilevel )
        sim.error( "Player {} has HoA of ilevel {}, ilevel for {} should be {}.\n", p.name(), item.parsed.data.level,
                   tier_name, hoa_ilevel );
    }
    // Azerite gear
    else if ( slot == SLOT_HEAD || slot == SLOT_SHOULDERS || slot == SLOT_CHEST )
    {
      // Check if there is at least one azerite power declared
      if ( item.parsed.azerite_ids.empty() )
        sim.errorf( "Player %s has %s with no azerite added.", p.name(), util::slot_type_string( slot ) );
      // Check if the azerite power declared does exists
      for ( auto& azerite_id : item.parsed.azerite_ids )
      {
        const auto& power = p.dbc->azerite_power( azerite_id );
        if ( power.id == 0 )
          sim.error( "Player {} has {} with azerite power id {} which does not exists.", p.name(),
                     util::slot_type_string( slot ), azerite_id );
      }
      // Check if there is more than one azerite power per tier (two for tier 3) and less than one for all tiers but
      // tier 1
      for ( unsigned i = 0; i < azerite_tiers; i++ )
      {
        int powers = 0;
        for ( auto& azerite_id : item.parsed.azerite_ids )
        {
          const auto& power = p.dbc->azerite_power( azerite_id );
          if ( power.tier == i )
            powers++;
        }
        if ( i != 3 && powers > 1 )
          sim.error( "Player {} has {} with {} azerite powers of tier {}, should have 1.", p.name(),
                     util::slot_type_string( slot ), powers, i );
        if ( i == 3 && powers > third_ring_traits )
          sim.error( "Player {} has {} with {} azerite powers of tier {}, should have {}.", p.name(),
                     util::slot_type_string( slot ), powers, i, third_ring_traits );
        if ( i != 1 && powers == 0 )
          sim.error( "Player {} has {} with 0 azerite power of tier {}, should have 1.", p.name(),
                     util::slot_type_string( slot ), i );
      }
      // Check final item level (item level + bonus from azerite)
      if ( item.parsed.data.level > max_azerite_ilevel_allowed )
        sim.error( "Player {} has {} of ilevel {}, maximum allowed ilevel for {} is {}.\n", p.name(),
                   util::slot_type_string( slot ), item.parsed.data.level, tier_name, max_azerite_ilevel_allowed );
    }
    // Legendary Cloak for T25 profile
    else if ( slot == SLOT_BACK && ( tier_name == "T25" || tier_name == "DS" ) )
    {
      // Check item level
      if ( item.parsed.data.level != max_cloak_ilevel )
        sim.error( "Player {} has cloak of ilevel {}, ilevel for {} should be {}.\n", p.name(), item.parsed.data.level,
                   tier_name, max_cloak_ilevel );
    }
    // Normal gear
    else
    {
      // Check if the item is not whitelisted
      bool is_whitelisted = false;
      for ( auto& item_id : whitelisted_items )
      {
        if ( item.parsed.data.id == item_id )
        {
          is_whitelisted = true;
          break;
        }
      }
      // Check item level
      if ( !is_whitelisted && item.parsed.data.level > max_ilevel_allowed )
        sim.error( "Player {} has {} of ilevel {}, maximum allowed ilevel for {} is {}.\n", p.name(),
                   util::slot_type_string( slot ), item.parsed.data.level, tier_name, max_ilevel_allowed );
    }

    size_t num_gems = 0;
    for ( size_t jj = 0; jj < item.parsed.gem_id.size(); ++jj )
    {
      if ( item.parsed.data.stat_alloc[ 0 ] == 7889 && item.parsed.gem_id[ jj ] > 0 && num_gems < 1 )
      {
        num_gems++;
        continue;  // 7889 seems to be the stat value for an item that comes with a socket by default,
                   // so we will allow 1 gem there.
      }
      if ( item.parsed.gem_id[ jj ] > 0 )
      {
        sim.errorf(
            "Player %s has gems equipped in slot %s, there are no gems allowed in default profiles even if they have a "
            "slot by default, this is to ensure that all default profiles within %s are as equal as possible.\n",
            p.name(), util::slot_type_string( slot ), tier_name.c_str() );
        break;
      }
    }

    if ( slot == SLOT_FINGER_1 || slot == SLOT_FINGER_2 || slot == SLOT_TRINKET_1 || slot == SLOT_TRINKET_2 )
    {
      for ( auto& slot2 : SLOT_DUPLICATES )
      {
        item_t& unique = p.items[ slot2 ];
        if ( slot2 == slot )
          continue;
        if ( p.dbc->item( unique.parsed.data.id ).id == 0 )
          continue;
        if ( p.dbc->item( item.parsed.data.id ).id == 0 )
          continue;

        if ( unique.parsed.data.id == item.parsed.data.id )
        {
          if ( p.dbc->item( unique.parsed.data.id ).flags_1 == 524288 &&
               p.dbc->item( item.parsed.data.id ).flags_1 == 524288 )
            sim.errorf(
                "Player %s has equipped more than 1 of a unique item in slots %s and %s, please remove one of the "
                "unique items.\n",
                p.name(), util::slot_type_string( slot ), util::slot_type_string( slot2 ) );
        }
      }
    }

    // Check if the item is using ilevel=
    if ( !item.option_ilevel_str.empty() )
      sim.errorf( "Player %s has %s with ilevel=, use bonus_id= instead.\n", p.name(), util::slot_type_string( slot ) );

    // Check if the item is using stats=
    if ( !item.option_stats_str.empty() )
      sim.errorf( "Player %s has %s with stats=, it is not allowed.\n", p.name(), util::slot_type_string( slot ) );

    // Check if the item is using enchant_id=
    if ( !item.option_enchant_id_str.empty() )
      sim.errorf( "Player %s has %s with enchant_id=, use enchant= instead.\n", p.name(),
                  util::slot_type_string( slot ) );

    // Check if the item is using gems=
    if ( !item.option_gems_str.empty() )
      sim.errorf( "Player %s has %s with gems=, use gem_id= instead.\n", p.name(), util::slot_type_string( slot ) );
  }

  return true;
}

void report_helper::generate_player_buff_lists( player_t& p, player_processed_report_information_t& ri )
{
  if ( ri.buff_lists_generated )
    return;

  // Append p.buff_list to ri.buff_list
  ri.buff_list.insert( ri.buff_list.end(), p.buff_list.begin(), p.buff_list.end() );

  for ( const auto& pet : p.pet_list )
  {
    // Append pet -> buff_list to ri.buff_list
    ri.buff_list.insert( ri.buff_list.end(), pet->buff_list.begin(), pet->buff_list.end() );
  }

  // Append p.sim -> buff_list to ri.buff_list
  ri.buff_list.insert( ri.buff_list.end(), p.sim->buff_list.begin(), p.sim->buff_list.end() );

  // Filter out non-dynamic buffs, copy them into ri.dynamic_buffs and sort
  // range::remove_copy_if( ri.buff_list, back_inserter( ri.dynamic_buffs ),
  // buff_is_dynamic );
  range::remove_copy_if( ri.buff_list, back_inserter( ri.dynamic_buffs ), buff_is_dynamic() );
  range::sort( ri.dynamic_buffs, buff_comp() );

  // Filter out non-constant buffs, copy them into ri.constant_buffs and sort
  range::remove_copy_if( ri.buff_list, back_inserter( ri.constant_buffs ), buff_is_constant() );
  range::sort( ri.constant_buffs, buff_comp() );

  ri.buff_lists_generated = true;
}

void report_helper::generate_player_charts( player_t& p, player_processed_report_information_t& ri )
{
  if ( ri.generated )
    return;

  // Scaling charts
  if ( !( ( p.sim->scaling->num_scaling_stats <= 0 ) || p.quiet || p.is_pet() || p.is_enemy() || p.is_add() ||
          p.type == HEALING_ENEMY ) )
  {
    ri.gear_weights_wowhead_std_link = gear_weights::wowhead( p );
    ri.gear_weights_pawn_string      = gear_weights::pawn( p );
    // ri.gear_weights_askmrrobot_link  = gear_weights::askmrrobot( p ); AMR has changed their web api drastically, I
    // doubt we'll be able to interface them anymore.
  }

  // Create html profile str
  ri.html_profile_str = p.create_profile( SAVE_ALL );

  ri.generated = true;
}

bool report_helper::output_scale_factors( const player_t* p )
{
  if ( !p->sim->scaling->has_scale_factors() || p->quiet || p->is_pet() || p->is_enemy() || p->type == HEALING_ENEMY )
  {
    return false;
  }

  return true;
}

std::vector<std::string> report_helper::beta_warnings()
{
  std::vector<std::string> s = { "Beta! Beta! Beta! Beta! Beta! Beta!",
                                 "Not All classes are yet supported.",
                                 "Some class models still need tweaking.",
                                 "Some class action lists need tweaking.",
                                 "Some class BiS gear setups need tweaking.",
                                 "Some trinkets not yet implemented.",
                                 "Constructive feedback regarding our output will shorten the Beta phase "
                                 "dramatically.",
                                 "Beta! Beta! Beta! Beta! Beta! Beta!" };
  return s;
}

void report_helper::print_html_sample_data( report::sc_html_stream& os, const player_t& p,
                                            const extended_sample_data_t& data, const std::string& name, int columns )
{
  // Print Statistics of a Sample Data Container
  os << "<tbody>\n"
     << "<tr>\n"
     << "<td class=\"left small\" colspan=\"" << columns << "\">";

  auto tokenized_name = util::remove_special_chars( util::tokenize_fn( data.name_str ) );
  os.printf( "<a id=\"actor%d_%s_stats_toggle\" class=\"toggle-details\">%s</a></td>\n</tr>\n", p.index,
             tokenized_name.c_str(), name.c_str() );

  os << "<tr class=\"details hide\">\n"
     << "<td class=\"filler\" colspan=\"" << columns << "\">\n"
     << "<table class=\"details\">\n"
     << "<tr>\n"
     << "<th class=\"left\" colspan=\"2\">" << util::encode_html( data.name_str ) << "</th>\n"
     << "</tr>\n";

  os.printf( "<tr>\n<td class=\"left\">Count</td>\n<td class=\"right\">%d</td>\n</tr>\n", (int)data.size() );
  os.printf( "<tr>\n<td class=\"left\">Mean</td>\n<td class=\"right\">%.2f</td>\n</tr>\n", data.mean() );
  os.printf( "<tr>\n<td class=\"left\">Minimum</td>\n<td class=\"right\">%.2f</td>\n</tr>\n", data.min() );
  os.printf( "<tr>\n<td class=\"left\">Maximum</td>\n<td class=\"right\">%.2f</td>\n</tr>\n", data.max() );
  os.printf( "<tr>\n<td class=\"left\">Spread ( max - min )</td>\n<td class=\"right\">%.2f</td>\n</tr>\n",
             data.max() - data.min() );
  os.printf(
      "<tr>\n<td class=\"left\">Range [ ( max - min ) / 2 * 100%% ]</td>\n"
      "<td class=\"right\">%.2f%%</td>\n</tr>\n",
      data.mean() ? ( ( data.max() - data.min() ) / 2 ) * 100 / data.mean() : 0 );
  if ( !data.simple )
  {
    os.printf( "<tr>\n<td class=\"left\">Standard Deviation</td>\n<td class=\"right\">%.4f</td>\n</tr>\n",
               data.std_dev );
    os.printf( "<tr>\n<td class=\"left\">5th Percentile</td>\n<td class=\"right\">%.2f</td>\n</tr>\n",
               data.percentile( 0.05 ) );
    os.printf( "<tr>\n<td class=\"left\">95th Percentile</td>\n<td class=\"right\">%.2f</td>\n</tr>\n",
               data.percentile( 0.95 ) );
    os.printf(
        "<tr>\n<td class=\"left\">( 95th Percentile - 5th Percentile )</td>\n"
        "<td class=\"right\">%.2f</td>\n</tr>\n",
        data.percentile( 0.95 ) - data.percentile( 0.05 ) );

    os << "<tr>\n"
       << "<th class=\"left\" colspan=\"2\">Mean Distribution</th>\n"
       << "</tr>\n";

    os.printf( "<tr>\n<td class=\"left\">Standard Deviation</td>\n<td class=\"right\">%.4f</td>\n</tr>\n",
               data.mean_std_dev );

    double mean_error = data.mean_std_dev * p.sim->confidence_estimator;
    os.printf(
        "<tr>\n<td class=\"left\">%.2f%% Confidence Interval</td>\n"
        "<td class=\"right\">( %.2f - %.2f )</td>\n</tr>\n",
        p.sim->confidence * 100.0, data.mean() - mean_error, data.mean() + mean_error );
    os.printf(
        "<tr>\n<td class=\"left\">Normalized %.2f%% Confidence Interval</td>\n"
        "<td class=\"right\">( %.2f%% - %.2f%% )</td>\n</tr>\n",
        p.sim->confidence * 100.0, data.mean() ? 100 - mean_error * 100 / data.mean() : 0,
        data.mean() ? 100 + mean_error * 100 / data.mean() : 0 );

    os << "<tr>\n"
       << "<th class=\"left\" colspan=\"2\">Approx. Iterations needed for ( always use n>=50 )</th>\n"
       << "</tr>\n";

    os.printf(
        "<tr>\n<td class=\"left\">1%% Error</td>\n<td class=\"right\">%.0f</td>\n</tr>\n",
        std::ceil( data.mean() ? ( mean_error * mean_error * data.size() / ( 0.01 * data.mean() * 0.01 * data.mean() ) )
                               : 0 ) );
    os.printf( "<tr>\n<td class=\"left\">0.1%% Error</td>\n<td class=\"right\">%.0f</td>\n</tr>\n",
               std::ceil( data.mean() ? ( mean_error * mean_error * data.size() /
                                          ( 0.001 * data.mean() * 0.001 * data.mean() ) )
                                      : 0 ) );
    os.printf(
        "<tr>\n<td class=\"left\">0.1 Scale Factor Error with Delta=300</td>\n"
        "<td class=\"right\">%.0f</td>\n</tr>\n",
        std::ceil( 2.0 * mean_error * mean_error * data.size() / ( 30.0 * 30.0 ) ) );
    os.printf(
        "<tr>\n<td class=\"left\">0.05 Scale Factor Error with Delta=300</td>\n"
        "<td class=\"right\">%.0f</td>\n</tr>\n",
        std::ceil( 2.0 * mean_error * mean_error * data.size() / ( 15 * 15 ) ) );
    os.printf(
        "<tr>\n<td class=\"left\">0.01 Scale Factor Error with Delta=300</td>\n"
        "<td class=\"right\">%.0f</td>\n</tr>\n",
        std::ceil( 2.0 * mean_error * mean_error * data.size() / ( 3 * 3 ) ) );
  }

  os << "</table>\n";

  if ( !data.simple )
  {
    highchart::histogram_chart_t chart( tokenized_name + "_dist", *p.sim );
    chart.set_toggle_id( "actor" + util::to_string( p.index ) + "_" + tokenized_name + "_stats_toggle" );
    if ( chart::generate_distribution( chart, nullptr, data.distribution, name, data.mean(), data.min(), data.max() ) )
    {
      os << chart.to_target_div();
      p.sim->add_chart_data( chart );
    }
  }

  os << "</td>\n"
     << "</tr>\n"
     << "</tbody>\n";
}
