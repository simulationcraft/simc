// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "report_helper.hpp"

#include "buff/buff.hpp"
#include "dbc/dbc.hpp"
#include "dbc/sc_spell_info.hpp"
#include "dbc/spell_query/spell_data_expr.hpp"
#include "item/item.hpp"
#include "player/pet.hpp"
#include "player/player.hpp"
#include "player/covenant.hpp"
#include "report/gear_weights.hpp"
#include "report/charts.hpp"
#include "report/highchart.hpp"
#include "sim/sim.hpp"
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
    return !(!b->quiet && b->avg_start.sum() && !b->constant);
  }
};

struct buff_is_constant
{
  bool operator()( const buff_t* b ) const
  {
    return !(!b->quiet && b->avg_start.sum() && b->constant);
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

    const spelleffect_data_t& effect = spell.effectN( effect_number );
    bool show_scale_factor           = effect.type() != E_APPLY_AURA;
    double s_min                     = dbc.effect_min( effect.id(), std::min( MAX_SCALING_LEVEL, level ) );
    double s_max                     = dbc.effect_max( effect.id(), std::min( MAX_SCALING_LEVEL, level ) );
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
          auto _num = parse_effect_number();
          if ( _num <= spell->effect_count() )
            replacement_text = util::to_string( spell->effectN( _num ).base_value() );
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
          auto _num = parse_effect_number();
          if ( _num <= spell->effect_count() )
            replacement_text = util::to_string( spell->effectN( _num ).period().total_seconds() );
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
          if ( *pos == 's' || *pos == 'a' )
          {
            ++pos;
            spell = parse_spell();
            bool has_spell = false;
            if ( player )
            {
              auto n = spell->name_cstr();

              has_spell = player->find_class_spell( n )->ok();
              if ( !has_spell )
                has_spell = player->find_specialization_spell( n )->ok();
              if ( !has_spell )
                has_spell = player->find_talent_spell( talent_tree::CLASS, n )->ok();
              if ( !has_spell )
                has_spell = player->find_talent_spell( talent_tree::SPECIALIZATION, n )->ok();
            }
            replacement_text = has_spell ? "true" : "false";
          }
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
          if ( text.compare( pos - text.begin(), 9, "spelldesc" ) == 0 )
          {
            pos += 9;

            spell = parse_spell();
            if ( !spell )
              throw error();
            assert( player );

            // References in tooltips/descriptions don't seem to form DAG, check for cycles of length 1 here (which
            // should hopefully be enough).
            if ( spell->id() != default_spell.id() )
            {
              const auto& spell_text = dbc.spell_text( spell->id() );
              replacement_text = report_helper::pretty_spell_text( *spell, spell_text.desc(), *player );
            }
          }
          break;
        }

        default:
          break;  // throw error();
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
  std::string tier_name;
  unsigned int max_ilevel_allowed = 0;
  unsigned int legendary_ilevel   = 0;
  int max_legendary_items         = 0;
  int equipped_legendaries        = 0;  // counter

  if ( p.report_information.save_str.find( "PR" ) != std::string::npos )
  {
    tier_name          = "PR";
    max_ilevel_allowed = 372;
  }
  else if ( p.report_information.save_str.find( "DS" ) != std::string::npos )
  {
    tier_name          = "DS";
    max_ilevel_allowed = 431;
  }
  else if ( p.report_information.save_str.find( "T29" ) != std::string::npos )
  {
    tier_name          = "T29";
    max_ilevel_allowed = 431;
  }
  else if ( p.report_information.save_str.find( "T30" ) != std::string::npos )
  {
    tier_name          = "T30";
    max_ilevel_allowed = 457;
  }
  else
  {
    return true;
  }

  unsigned whitelisted_items[] = {
      171323, // Spiritual Alchemy Stone
      173096, 173069, 173087, 173078  // Darkmoon Decks: Indomitable, Putrescence, Voracity, Repose
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

  const slot_e SLOT_GEMS[] = {
    SLOT_HEAD,
    SLOT_NECK,
    SLOT_WRISTS,
    SLOT_WAIST,
    SLOT_FINGER_1,
    SLOT_FINGER_2,
  };

  for ( auto& slot : SLOT_OUT_ORDER )
  {
    item_t& item = p.items[ slot ];

    // Legendary item count
    if ( item.parsed.data.quality == ITEM_QUALITY_LEGENDARY )
    {
      equipped_legendaries++;
      if ( item.item_level() != legendary_ilevel )
      {
        sim.error( "Player {} has legendary item at slot {} of ilevel {}, legendary ilevel for {} is {}.\n", p.name(),
                   util::slot_type_string( slot ), item.item_level(), tier_name, legendary_ilevel );
      }
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
      if ( !is_whitelisted && item.item_level() > max_ilevel_allowed )
        sim.error( "Player {} has {} of ilevel {}, maximum allowed ilevel for {} is {}.\n", p.name(),
                   util::slot_type_string( slot ), item.item_level(), tier_name, max_ilevel_allowed );
    }

    // Check gem sockets
    int gem_count = 0;

    for ( size_t jj = 0; jj < item.parsed.gem_id.size(); ++jj )
    {
      const auto& gem                     = item.player->dbc->item( item.parsed.gem_id[ jj ] );
      if ( gem.id > 0 )
      {
        gem_count++;
      }
    }

    // Check gem count and gem slot usage
    if ( gem_count > 1 && item.slot != SLOT_NECK )
    {
      sim.error( "Player {} has {} with {} gems slotted. Only one gem per item is allowed.\n",
                 p.name(), util::slot_type_string( slot ), gem_count );
    }
    
    // Jewelcrafting item lets you add 3 sockets on a Neck item
    if ( gem_count > 3 && item.slot == SLOT_NECK )
    {
      sim.error( "Player {} has {} with {} gems slotted. Only three gems per neck is allowed.\n",
                 p.name(), util::slot_type_string( slot ), gem_count );
    }

    // Prismatic sockets can only proc (or be added) on head, neck, wrists, belt and rings
    if ( gem_count )
    {
      bool valid_gem_slot = false;
      for ( auto& gem_slot : SLOT_GEMS )
      {
        if ( item.slot == gem_slot )
        {
          valid_gem_slot = true;
          break;
        }
      }
      if ( !valid_gem_slot )
        sim.error( "Player {} has prismatic socket on {}, prismatic sockets are only valid on head, neck, wrists, belts and rings.\n",
                    p.name(), util::slot_type_string( slot ) );
    }

    // Check if an unique equipped item is equipped multiple times
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

    // Check allowed enchant slots in Dragonflight: chest/back/fingers/weapons/legs
    // Warns about invalid slots and kindly notices if an item is missing an enchant
    if ( slot == SLOT_CHEST || slot == SLOT_FINGER_1 || slot == SLOT_FINGER_2 || slot == SLOT_MAIN_HAND || slot == SLOT_LEGS ||
         // Make sure offhand enchants are only on regular weapons, not shields or off-hand stat sticks
         ( slot == SLOT_OFF_HAND && item.weapon()->type != weapon_e::WEAPON_NONE ))
    {
      if ( item.option_enchant_str.empty() && item.option_enchant_id_str.empty() )
        sim.errorf( "Player %s is missing an enchantment on %s, please add one.\n", p.name(),
                    util::slot_type_string( slot ) );
    }
    // Don't enforce non-throughput cloak, feet, or wrist enchants
    else if ( !item.option_enchant_str.empty() && slot != SLOT_BACK && slot != SLOT_WRISTS && slot != SLOT_FEET )
      sim.errorf( "Player %s has an invalid enchantment equipped on %s, please remove it. \n", p.name(),
                  util::slot_type_string( slot ) );

    // Check if the item is using enchant_id=
    if ( !item.option_enchant_id_str.empty() )
      sim.errorf( "Player %s has %s with enchant_id=, use enchant= instead.\n", p.name(),
                  util::slot_type_string( slot ) );

    // Check if the item is using gems=
    if ( !item.option_gems_str.empty() )
      sim.errorf( "Player %s has %s with gems=, use gem_id= instead.\n", p.name(), util::slot_type_string( slot ) );
  }

  // Ensures that the player doesn't have too many legendary items equipped
  if ( equipped_legendaries != max_legendary_items )
  {
    sim.error( "Player {} has {} legendary items equipped, legendary item count for {} is {}, at item level {}.\n",
               p.name(), equipped_legendaries, tier_name, max_legendary_items, legendary_ilevel );
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
                                 "Constructive feedback regarding our output will shorten the Beta phase dramatically.",
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
