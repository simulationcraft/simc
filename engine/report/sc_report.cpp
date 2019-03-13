// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_report.hpp"
#include "dbc/sc_spell_info.hpp"

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
    : default_spell( s ), dbc( p.dbc ), player( &p ), level( p.true_level ), text( t ), pos( t.begin() )
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
            replacement_text = report::pretty_spell_text( *spell, spell->desc(), *player );
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

const color::rgb& item_quality_color( const item_t& item )
{
  switch ( item.parsed.data.quality )
  {
    case 1:
      return color::COMMON;
    case 2:
      return color::UNCOMMON;
    case 3:
      return color::RARE;
    case 4:
      return color::EPIC;
    case 5:
      return color::LEGENDARY;
    case 6:
      return color::HEIRLOOM;
    case 7:
      return color::HEIRLOOM;
    default:
      return color::POOR;
  }
}
}  // namespace

std::string report::pretty_spell_text( const spell_data_t& default_spell, const std::string& text, const player_t& p )
{
  return tooltip_parser_t( p, default_spell, text ).parse();
}

// report::check_gear =======================================================

bool report::check_gear( player_t& p, sim_t& sim )
{
  int max_ilevel_allowed         = 0;
  int max_azerite_ilevel_allowed = 0;
  int hoa_ilevel                 = 0;
  unsigned hoa_level             = 0;
  std::string tier_name          = "";
  int third_ring_traits          = 2;

  if ( p.report_information.save_str.find( "PR" ) != std::string::npos )
  {
    max_ilevel_allowed         = 340;
    max_azerite_ilevel_allowed = max_ilevel_allowed + 5;
    hoa_ilevel                 = 347;
    hoa_level                  = 27;
    tier_name                  = "PR";
    third_ring_traits          = 1;
  }
  else if ( p.report_information.save_str.find( "T22" ) != std::string::npos )
  {
    max_ilevel_allowed         = 385;
    max_azerite_ilevel_allowed = max_ilevel_allowed + 5;
    hoa_ilevel                 = 389;
    hoa_level                  = 33;
    tier_name                  = "T22";
    third_ring_traits          = 1;
  }
  else if (p.report_information.save_str.find( "T23" ) != std::string::npos )
  {
    max_ilevel_allowed         = 415;
    max_azerite_ilevel_allowed = max_ilevel_allowed + 5;
    hoa_ilevel                 = 429;
    hoa_level                  = 48;
    tier_name                  = "T23";
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
        sim.errorf( "Player %s has HoA of level %s, level for %s should be %s.\n", p.name(),
                    util::to_string( item.parsed.azerite_level ).c_str(), tier_name.c_str(),
                    util::to_string( hoa_level ).c_str() );
      // Check final item level (since it's not only computed from azerite_level)
      if ( item.parsed.data.level != hoa_ilevel )
        sim.errorf( "Player %s has HoA of ilevel %s, ilevel for %s should be %s.\n", p.name(),
                    util::to_string( item.parsed.data.level ).c_str(), tier_name.c_str(),
                    util::to_string( hoa_ilevel ).c_str() );
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
        const auto& power = p.dbc.azerite_power( azerite_id );
        if ( power.id == 0 )
          sim.errorf( "Player %s has %s with azerite power id %s which does not exists.", p.name(),
                      util::slot_type_string( slot ), util::to_string( azerite_id ).c_str() );
      }
      // Check if there is more than one azerite power per tier (two for tier 3) and less than one for all tiers but tier 1
      for ( unsigned i = 0; i < azerite_tiers; i++ )
      {
        int powers = 0;
        for ( auto& azerite_id : item.parsed.azerite_ids )
        {
          const auto& power = p.dbc.azerite_power( azerite_id );
          if ( power.tier == i )
            powers++;
        }
        if ( i != 3 && powers > 1 )
          sim.errorf( "Player %s has %s with %s azerite powers of tier %s, should have 1.", p.name(), util::slot_type_string( slot ),
                      util::to_string( powers ).c_str(), util::to_string( i ).c_str() );
        if ( i == 3 && powers > third_ring_traits )
          sim.errorf( "Player %s has %s with %s azerite powers of tier %s, should have %s.", p.name(), util::slot_type_string( slot ),
                      util::to_string( powers ).c_str(), util::to_string( i ).c_str(), util::to_string( third_ring_traits ) );
        if ( i != 1 && powers == 0 )
          sim.errorf( "Player %s has %s with 0 azerite power of tier %s, should have 1.", p.name(), util::slot_type_string( slot ),
                      util::to_string( i ).c_str() );
      }
      // Check final item level (item level + bonus from azerite)
      if ( item.parsed.data.level > max_azerite_ilevel_allowed )
        sim.errorf( "Player %s has %s of ilevel %s, maximum allowed ilevel for %s is %s.\n", p.name(),
                    util::slot_type_string( slot ), util::to_string( item.parsed.data.level ).c_str(),
                    tier_name.c_str(), util::to_string( max_azerite_ilevel_allowed ).c_str() );
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
        sim.errorf( "Player %s has %s of ilevel %s, maximum allowed ilevel for %s is %s.\n", p.name(),
                    util::slot_type_string( slot ), util::to_string( item.parsed.data.level ).c_str(),
                    tier_name.c_str(), util::to_string( max_ilevel_allowed ).c_str() );
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
        if ( p.dbc.item( unique.parsed.data.id ) == nullptr )
          continue;
        if ( p.dbc.item( item.parsed.data.id ) == nullptr )
          continue;

        if ( unique.parsed.data.id == item.parsed.data.id )
        {
          if ( p.dbc.item( unique.parsed.data.id )->flags_1 == 524288 &&
               p.dbc.item( item.parsed.data.id )->flags_1 == 524288 )
            sim.errorf(
                "Player %s has equipped more than 1 of a unique item in slots %s and %s, please remove one of the "
                "unique items.\n",
                p.name(), util::slot_type_string( slot ), util::slot_type_string( slot2 ) );
        }
      }
    }

    // Check if the item is using ilevel=
    if ( !item.option_ilevel_str.empty() )
      sim.errorf( "Player %s has %s with ilevel=, use bonus_id= instead.\n", p.name(),
                  util::slot_type_string( slot ) );

    // Check if the item is using stats=
    if ( !item.option_stats_str.empty() )
      sim.errorf( "Player %s has %s with stats=, it is not allowed.\n", p.name(),
                  util::slot_type_string( slot ) );

    // Check if the item is using enchant_id=
    if ( !item.option_enchant_id_str.empty() )
      sim.errorf( "Player %s has %s with enchant_id=, use enchant= instead.\n", p.name(),
                  util::slot_type_string( slot ) );

    // Check if the item is using gems=
    if ( !item.option_gems_str.empty() )
      sim.errorf( "Player %s has %s with gems=, use gem_id= instead.\n", p.name(),
                  util::slot_type_string( slot ) );
  }

  return true;
}

// report::print_profiles ===================================================

void report::print_profiles( sim_t* sim )
{
  int k = 0;
  for ( unsigned int i = 0; i < sim->actor_list.size(); i++ )
  {
    player_t* p = sim->actor_list[ i ];
    if ( p->is_pet() )
      continue;

    if ( !check_gear( *p, *sim ) )
    {
      continue;
    }
    k++;

    if ( !p->report_information.save_gear_str.empty() )  // Save gear
    {
      io::cfile file( p->report_information.save_gear_str, "w" );
      if ( !file )
      {
        sim->errorf( "Unable to save gear profile %s for player %s\n", p->report_information.save_gear_str.c_str(),
                     p->name() );
      }
      else
      {
        std::string profile_str = p->create_profile( SAVE_GEAR );
        fprintf( file, "%s", profile_str.c_str() );
      }
    }

    if ( !p->report_information.save_talents_str.empty() )  // Save talents
    {
      io::cfile file( p->report_information.save_talents_str, "w" );
      if ( !file )
      {
        sim->errorf( "Unable to save talents profile %s for player %s\n",
                     p->report_information.save_talents_str.c_str(), p->name() );
      }
      else
      {
        std::string profile_str = p->create_profile( SAVE_TALENTS );
        fprintf( file, "%s", profile_str.c_str() );
      }
    }

    if ( !p->report_information.save_actions_str.empty() )  // Save actions
    {
      io::cfile file( p->report_information.save_actions_str, "w" );
      if ( !file )
      {
        sim->errorf( "Unable to save actions profile %s for player %s\n",
                     p->report_information.save_actions_str.c_str(), p->name() );
      }
      else
      {
        std::string profile_str = p->create_profile( SAVE_ACTIONS );
        fprintf( file, "%s", profile_str.c_str() );
      }
    }

    std::string file_name = p->report_information.save_str;

    if ( file_name.empty() && sim->save_profiles )
    {
      file_name = sim->save_prefix_str;
      file_name += p->name_str;
      if ( sim->save_talent_str != 0 )
      {
        file_name += "_";
        file_name += p->primary_tree_name();
      }
      file_name += sim->save_suffix_str;
      file_name += ".simc";
    }

    if ( file_name.empty() )
      continue;

    io::cfile file( file_name, "w" );
    if ( !file )
    {
      sim->errorf( "Unable to save profile %s for player %s\n", file_name.c_str(), p->name() );
      continue;
    }

    unsigned save_type = SAVE_ALL;
    if ( !sim->save_profile_with_actions )
    {
      save_type &= ~( SAVE_ACTIONS );
    }
    std::string profile_str = p->create_profile( static_cast<save_e>( save_type ) );
    fprintf( file, "%s", profile_str.c_str() );
  }

  // Save overview file for Guild downloads
  // if ( /* guild parse */ )
  if ( sim->save_raid_summary )
  {
    static const char* const filename = "Raid_Summary.simc";
    io::cfile file( filename, "w" );
    if ( !file )
    {
      sim->errorf( "Unable to save overview profile %s\n", filename );
    }
    else
    {
      fprintf( file,
               "#Raid Summary\n"
               "# Contains %d Players.\n\n",
               k );

      for ( unsigned int i = 0; i < sim->actor_list.size(); ++i )
      {
        player_t* p = sim->actor_list[ i ];
        if ( p->is_pet() )
          continue;

        if ( !p->report_information.save_str.empty() )
          fprintf( file, "%s\n", p->report_information.save_str.c_str() );
        else if ( sim->save_profiles )
        {
          fprintf( file,
                   "# Player: %s Spec: %s Role: %s\n"
                   "%s%s",
                   p->name(), p->primary_tree_name(), util::role_type_string( p->primary_role() ),
                   sim->save_prefix_str.c_str(), p->name() );

          if ( sim->save_talent_str != 0 )
            fprintf( file, "-%s", p->primary_tree_name() );

          fprintf( file, "%s.simc\n\n", sim->save_suffix_str.c_str() );
        }
      }
    }
  }
}

// report::print_spell_query ================================================

void report::print_spell_query( std::ostream& out, const sim_t& sim, const spell_data_expr_t& sq, unsigned level )
{
  expr_data_e data_type = sq.data_type;
  for ( auto i = sq.result_spell_list.begin(); i != sq.result_spell_list.end(); ++i )
  {
    switch ( data_type )
    {
      case DATA_TALENT:
        out << spell_info::talent_to_str( sim.dbc, sim.dbc.talent( *i ), level );
        break;
      case DATA_EFFECT:
      {
        std::ostringstream sqs;
        const spelleffect_data_t* base_effect = sim.dbc.effect( *i );
        if ( const spell_data_t* spell = dbc::find_spell( &( sim ), base_effect->spell() ) )
        {
          spell_info::effect_to_str( sim.dbc, spell, dbc::find_effect( &( sim ), base_effect ), sqs, level );
          out << sqs.str();
        }
      }
      break;
      default:
      {
        const spell_data_t* spell = dbc::find_spell( &( sim ), sim.dbc.spell( *i ) );
        out << spell_info::to_str( sim.dbc, spell, level );
      }
    }
  }
}

void report::print_spell_query( xml_node_t* root, FILE* file, const sim_t& sim, const spell_data_expr_t& sq,
                                unsigned level )
{
  expr_data_e data_type = sq.data_type;
  for ( auto i = sq.result_spell_list.begin(); i != sq.result_spell_list.end(); ++i )
  {
    switch ( data_type )
    {
      case DATA_TALENT:
        spell_info::talent_to_xml( sim.dbc, sim.dbc.talent( *i ), root, level );
        break;
      case DATA_EFFECT:
      {
        std::ostringstream sqs;
        const spelleffect_data_t* dbc_effect = sim.dbc.effect( *i );
        if ( const spell_data_t* spell = dbc::find_spell( &( sim ), dbc_effect->spell() ) )
        {
          spell_info::effect_to_xml( sim.dbc, spell, dbc::find_effect( &( sim ), dbc_effect ), root, level );
        }
      }
      break;
      default:
      {
        const spell_data_t* spell = dbc::find_spell( &( sim ), sim.dbc.spell( *i ) );
        spell_info::to_xml( sim.dbc, spell, root, level );
      }
    }
  }

  util::fprintf( file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
  root->print_xml( file );
}
// report::print_suite ======================================================

void report::print_suite( sim_t* sim )
{
  if ( !sim->profileset_enabled )
  {
    std::cout << "\nGenerating reports...\n";
  }

  report::print_text( sim, sim->report_details != 0 );

  report::print_html( *sim );
  report::print_json( *sim );
  report::print_profiles( sim );
}

void report::print_html_sample_data( report::sc_html_stream& os, const player_t& p, const extended_sample_data_t& data,
                                     const std::string& name, int& td_counter, int columns )
{
  // Print Statistics of a Sample Data Container
  os << "\t\t\t\t\t\t\t<tr";
  if ( td_counter & 1 )
  {
    os << " class=\"odd\"";
  }
  td_counter++;
  os << ">\n";
  os << "\t\t\t\t\t\t\t\t<td class=\"left small\" colspan=\"" << columns << "\">";

  std::string tokenized_name = name;
  util::tokenize( tokenized_name );
  os.printf(
      "<a id=\"actor%d_%s_stats_toggle\" "
      "class=\"toggle-details\">%s</a></td>\n",
      p.index, tokenized_name.c_str(), name.c_str() );

  os << "\t\t\t\t\t\t\t\t</tr>\n";

  os << "\t\t\t\t\t\t\t<tr class=\"details hide\">\n";

  os << "\t\t\t\t\t\t\t\t<td class=\"filler\" colspan=\"" << columns << "\">\n";
  int i = 0;

  os << "\t\t\t\t\t\t\t<table class=\"details\">\n";

  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os << "\t\t\t\t\t\t\t\t\t<th class=\"left\"><b>Sample Data</b></th>\n"
     << "\t\t\t\t\t\t\t\t\t<th class=\"right\">" << data.name_str << "</th>\n"
     << "\t\t\t\t\t\t\t\t</tr>\n";

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os.printf(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">Count</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%d</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      (int)data.size() );

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os.printf(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">Mean</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      data.mean() );

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";

  os.printf(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">Minimum</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      data.min() );

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os.printf(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">Maximum</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      data.max() );

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os.printf(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">Spread ( max - min )</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      data.max() - data.min() );

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os.printf(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">Range [ ( max - min ) / 2 * 100%% "
      "]</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      data.mean() ? ( ( data.max() - data.min() ) / 2 ) * 100 / data.mean() : 0 );

  if ( !data.simple )
  {
    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.printf(
        "\t\t\t\t\t\t\t\t\t<td class=\"left\">Standard Deviation</td>\n"
        "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.4f</td>\n"
        "\t\t\t\t\t\t\t\t</tr>\n",
        data.std_dev );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.printf(
        "\t\t\t\t\t\t\t\t\t<td class=\"left\">5th Percentile</td>\n"
        "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
        "\t\t\t\t\t\t\t\t</tr>\n",
        data.percentile( 0.05 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.printf(
        "\t\t\t\t\t\t\t\t\t<td class=\"left\">95th Percentile</td>\n"
        "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
        "\t\t\t\t\t\t\t\t</tr>\n",
        data.percentile( 0.95 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.printf(
        "\t\t\t\t\t\t\t\t\t<td class=\"left\">( 95th Percentile - 5th "
        "Percentile )</td>\n"
        "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
        "\t\t\t\t\t\t\t\t</tr>\n",
        data.percentile( 0.95 ) - data.percentile( 0.05 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.printf(
        "\t\t\t\t\t\t\t\t\t<td class=\"left\"><b>Mean Distribution</b></td>\n"
        "\t\t\t\t\t\t\t\t\t<td class=\"right\"></td>\n"
        "\t\t\t\t\t\t\t\t</tr>\n" );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.printf(
        "\t\t\t\t\t\t\t\t\t<td class=\"left\">Standard Deviation</td>\n"
        "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.4f</td>\n"
        "\t\t\t\t\t\t\t\t</tr>\n",
        data.mean_std_dev );

    ++i;
    double mean_error = data.mean_std_dev * p.sim->confidence_estimator;
    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.printf(
        "\t\t\t\t\t\t\t\t\t<td class=\"left\">%.2f%% Confidence "
        "Intervall</td>\n"
        "\t\t\t\t\t\t\t\t\t<td class=\"right\">( %.2f - %.2f )</td>\n"
        "\t\t\t\t\t\t\t\t</tr>\n",
        p.sim->confidence * 100.0, data.mean() - mean_error, data.mean() + mean_error );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.printf(
        "\t\t\t\t\t\t\t\t\t<td class=\"left\">Normalized %.2f%% Confidence "
        "Intervall</td>\n"
        "\t\t\t\t\t\t\t\t\t<td class=\"right\">( %.2f%% - %.2f%% )</td>\n"
        "\t\t\t\t\t\t\t\t</tr>\n",
        p.sim->confidence * 100.0, data.mean() ? 100 - mean_error * 100 / data.mean() : 0,
        data.mean() ? 100 + mean_error * 100 / data.mean() : 0 );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"left\"><b>Approx. Iterations needed "
          "for ( always use n>=50 )</b></td>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"right\"></td>\n"
       << "\t\t\t\t\t\t\t\t</tr>\n";

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.printf(
        "\t\t\t\t\t\t\t\t\t<td class=\"left\">1%% Error</td>\n"
        "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
        "\t\t\t\t\t\t\t\t</tr>\n",
        std::ceil( data.mean() ? ( mean_error * mean_error * data.size() / ( 0.01 * data.mean() * 0.01 * data.mean() ) )
                               : 0 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.printf(
        "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.1%% Error</td>\n"
        "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
        "\t\t\t\t\t\t\t\t</tr>\n",
        std::ceil( data.mean()
                       ? ( mean_error * mean_error * data.size() / ( 0.001 * data.mean() * 0.001 * data.mean() ) )
                       : 0 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.printf(
        "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.1 Scale Factor Error with "
        "Delta=300</td>\n"
        "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
        "\t\t\t\t\t\t\t\t</tr>\n",
        std::ceil( 2.0 * mean_error * mean_error * data.size() / ( 30.0 * 30.0 ) ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.printf(
        "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.05 Scale Factor Error with "
        "Delta=300</td>\n"
        "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
        "\t\t\t\t\t\t\t\t</tr>\n",
        std::ceil( 2.0 * mean_error * mean_error * data.size() / ( 15 * 15 ) ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.printf(
        "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.01 Scale Factor Error with "
        "Delta=300</td>\n"
        "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
        "\t\t\t\t\t\t\t\t</tr>\n",
        std::ceil( 2.0 * mean_error * mean_error * data.size() / ( 3 * 3 ) ) );
  }

  os << "\t\t\t\t\t\t\t\t</table>\n";

  if ( !data.simple )
  {
    std::string tokenized_div_name = data.name_str + "_dist";
    util::tokenize( tokenized_div_name );

    highchart::histogram_chart_t chart( tokenized_div_name, *p.sim );
    chart.set_toggle_id( "actor" + util::to_string( p.index ) + "_" + tokenized_div_name + "_stats_toggle" );
    if ( chart::generate_distribution( chart, nullptr, data.distribution, name, data.mean(), data.min(), data.max() ) )
    {
      os << chart.to_target_div();
      p.sim->add_chart_data( chart );
    }
  }

  os << "\t\t\t\t\t\t\t\t</td>\n"
     << "\t\t\t\t\t\t\t</tr>\n";
}

void report::generate_player_buff_lists( player_t& p, player_processed_report_information_t& ri )
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

void report::generate_player_charts( player_t& p, player_processed_report_information_t& ri )
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

std::string report::decorate_html_string( const std::string& value, const color::rgb& color )
{
  std::stringstream s;

  s << "<span style=\"color:" << color;
  s << "\">" << value << "</span>";

  return s.str();
}

bool report::output_scale_factors( const player_t* p )
{
  if ( !p->sim->scaling->has_scale_factors() || p->quiet || p->is_pet() || p->is_enemy() || p->type == HEALING_ENEMY )
  {
    return false;
  }

  return true;
}

std::string report::decoration_domain( const sim_t& sim )
{
#if SC_BETA == 0
  if ( maybe_ptr( sim.dbc.ptr ) )
  {
    return "ptr";
  }
  else
  {
    return "www";
  }
#else
  return "beta";
  (void)sim;
#endif
}

std::string report::decorated_spell_name( const sim_t& sim, const spell_data_t& spell, const std::string& parms_str )
{
  std::stringstream s;

  if ( sim.decorated_tooltips == false )
  {
    s << "<a href=\"#\">" << spell.name_cstr() << "</a>";
  }
  else
  {
    s << "<a href=\"https://" << decoration_domain( sim ) << ".wowhead.com/spell=" << spell.id()
      << ( !parms_str.empty() ? "?" + parms_str : "" ) << "\">" << spell.name_cstr() << "</a>";
  }

  return s.str();
}

std::string report::decorated_item_name( const item_t* item )
{
  std::stringstream s;

  if ( item->sim->decorated_tooltips == false || item->parsed.data.id == 0 )
  {
    s << "<a style=\"color:" << item_quality_color( *item ) << ";\" href=\"#\">" << item->full_name() << "</a>";
  }
  else
  {
    std::vector<std::string> params;
    if ( item->parsed.enchant_id > 0 )
    {
      params.push_back( "enchantment=" + util::to_string( item->parsed.enchant_id ) );
    }

    if ( item->parsed.upgrade_level > 0 )
    {
      params.push_back( "upgradeNum=" + util::to_string( item->parsed.upgrade_level ) );
    }

    std::string gem_str = "";
    for ( size_t i = 0; i < item->parsed.gem_id.size(); ++i )
    {
      if ( item->parsed.gem_id[ i ] == 0 )
      {
        continue;
      }

      if ( !gem_str.empty() )
      {
        gem_str += ",";
      }

      gem_str += util::to_string( item->parsed.gem_id[ i ] );
    }

    if ( !gem_str.empty() )
    {
      params.push_back( "gems=" + gem_str );
    }

    std::string bonus_str = "";
    for ( size_t i = 0; i < item->parsed.bonus_id.size(); ++i )
    {
      bonus_str += util::to_string( item->parsed.bonus_id[ i ] );
      if ( i < item->parsed.bonus_id.size() - 1 )
      {
        bonus_str += ",";
      }
    }

    if ( !bonus_str.empty() )
    {
      params.push_back( "bonusIDs=" + bonus_str );
    }

    s << "<a style=\"color:" << item_quality_color( *item ) << ";\" href=\"https://" << decoration_domain( *item->sim )
      << ".wowhead.com/item=" << item->parsed.data.id;

    if ( params.size() > 0 )
    {
      s << "?";
      for ( size_t i = 0; i < params.size(); ++i )
      {
        s << params[ i ];

        if ( i < params.size() - 1 )
        {
          s << "&";
        }
      }
    }

    s << "\">" << item->full_name() << "</a>";
  }

  return s.str();
}

std::vector<std::string> report::beta_warnings()
{
  std::vector<std::string> s = {"Beta! Beta! Beta! Beta! Beta! Beta!",
                                "Not All classes are yet supported.",
                                "Some class models still need tweaking.",
                                "Some class action lists need tweaking.",
                                "Some class BiS gear setups need tweaking.",
                                "Some trinkets not yet implemented.",
                                "Constructive feedback regarding our output will shorten the Beta phase "
                                "dramatically.",
                                "Beta! Beta! Beta! Beta! Beta! Beta!"};
  return s;
}

std::string report::buff_decorator_t::url_name_prefix() const
{
  if ( m_obj->source && m_obj->source->is_pet() )
  {
    return m_obj->source->name_str + ":&#160;";
  }

  return std::string();
}

std::vector<std::string> report::buff_decorator_t::parms() const
{
  std::vector<std::string> parms = super::parms();

  // if ( m_obj -> source && m_obj -> source -> specialization() != SPEC_NONE )
  //{
  //  parms.push_back( "spec=" + util::to_string( m_obj -> source -> specialization() ) );
  //}

  return parms;
}

std::vector<std::string> report::action_decorator_t::parms() const
{
  std::vector<std::string> parms = super::parms();

  // if ( m_obj -> player && m_obj -> player -> specialization() != SPEC_NONE )
  //{
  //  parms.push_back( "spec=" + util::to_string( m_obj -> player -> specialization() ) );
  //}

  return parms;
}

report::spell_data_decorator_t::spell_data_decorator_t( const player_t* obj, const spell_data_t* spell )
  : html_decorator_t(), m_sim( obj->sim ), m_player( obj ), m_spell( spell ), m_item( nullptr ), m_power( nullptr )
{
}

report::spell_data_decorator_t::spell_data_decorator_t( const player_t* obj, const artifact_power_t& power )
  : spell_data_decorator_t( obj, power.data() )
{
  artifact_power( power );
}

bool report::spell_data_decorator_t::can_decorate() const
{
  return m_sim->decorated_tooltips && m_spell->id() > 0;
}

std::string report::spell_data_decorator_t::url_name() const
{
  return m_spell->name_cstr();
}

std::string report::spell_data_decorator_t::token() const
{
  std::string token = m_spell->name_cstr();
  util::tokenize( token );
  return token;
}

std::vector<std::string> report::spell_data_decorator_t::parms() const
{
  auto params = html_decorator_t::parms();

  // if ( m_player && m_player -> specialization() != SPEC_NONE )
  //{
  //  params.push_back( "spec=" + util::to_string( m_player -> specialization() ) );
  //}

  if ( m_item )
  {
    params.push_back( "ilvl=" + util::to_string( m_item->item_level() ) );
  }

  if ( m_power )
  {
    params.push_back( "artifactRank=" + util::to_string( m_power->rank() ) );
  }

  return params;
}

std::string report::spell_data_decorator_t::base_url() const
{
  std::stringstream s;

  s << "<a href=\"https://" << decoration_domain( *m_sim ) << ".wowhead.com/spell=" << m_spell->id();

  return s.str();
}

bool report::item_decorator_t::can_decorate() const
{
  return m_item->sim->decorated_tooltips && m_item->parsed.data.id > 0;
}

std::string report::item_decorator_t::base_url() const
{
  std::stringstream s;

  s << "<a "
    << "style=\"color:" << item_quality_color( *m_item ) << ";\" "
    << "href=\"https://" << decoration_domain( *m_item->sim ) << ".wowhead.com/item=" << m_item->parsed.data.id;

  return s.str();
}

std::string report::item_decorator_t::token() const
{
  return m_item->name();
}

std::string report::item_decorator_t::url_name() const
{
  return m_item->full_name();
}

std::vector<std::string> report::item_decorator_t::parms() const
{
  auto params = html_decorator_t::parms();

  if ( m_item->parsed.enchant_id > 0 )
  {
    params.push_back( "ench=" + util::to_string( m_item->parsed.enchant_id ) );
  }

  if ( m_item->parsed.upgrade_level > 0 )
  {
    params.push_back( "upgd=" + util::to_string( m_item->parsed.upgrade_level ) );
  }

  std::stringstream gem_str;
  for ( size_t i = 0, end = m_item->parsed.gem_id.size(); i < end; ++i )
  {
    if ( m_item->parsed.gem_id[ i ] == 0 )
    {
      continue;
    }

    if ( gem_str.tellp() > 0 )
    {
      gem_str << ":";
    }

    gem_str << util::to_string( m_item->parsed.gem_id[ i ] );

    // Include relic bonus ids
    if ( i < m_item->parsed.relic_data.size() )
    {
      range::for_each( m_item->parsed.relic_data[ i ],
                       [&gem_str]( unsigned bonus_id ) { gem_str << ":" << bonus_id; } );
    }
  }

  if ( gem_str.tellp() > 0 )
  {
    params.push_back( "gems=" + gem_str.str() );
  }

  std::stringstream bonus_str;
  for ( size_t i = 0, end = m_item->parsed.bonus_id.size(); i < end; ++i )
  {
    bonus_str << util::to_string( m_item->parsed.bonus_id[ i ] );

    if ( i < end - 1 )
    {
      bonus_str << ":";
    }
  }

  if ( bonus_str.tellp() > 0 )
  {
    params.push_back( "bonus=" + bonus_str.str() );
  }

  std::stringstream azerite_str;
  if ( !m_item->parsed.azerite_ids.empty() )
  {
    azerite_str << util::class_id( m_item->player->type ) << ":";
  }
  for ( size_t i = 0, end = m_item->parsed.azerite_ids.size(); i < end; ++i )
  {
    azerite_str << util::to_string( m_item->parsed.azerite_ids[ i ] );

    if ( i < end - 1 )
    {
      azerite_str << ":";
    }
  }

  if ( azerite_str.tellp() > 0 )
  {
    params.push_back( "azerite-powers=" + azerite_str.str() );
  }

  params.push_back( "ilvl=" + util::to_string( m_item->item_level() ) );

  return params;
}
