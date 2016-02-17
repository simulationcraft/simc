// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
 
#include "simulationcraft.hpp"
#include "sc_report.hpp"

// ==========================================================================
// Report
// ==========================================================================

namespace { // UNNAMED NAMESPACE

struct buff_is_dynamic
{
  bool operator() ( const buff_t* b ) const
  {
    if ( ! b -> quiet && b -> avg_start.sum() && ! b -> constant )
      return false;

    return true;
  }
};

struct buff_is_constant
{
  bool operator() ( const buff_t* b ) const
  {
    if ( ! b -> quiet && b -> avg_start.sum() && b -> constant )
      return false;

    return true;
  }
};

struct buff_comp
{
  bool operator()( const buff_t* i, const buff_t* j )
  {
    // Aura&Buff / Pet
    if ( ( ! i -> player || ! i -> player -> is_pet() ) && j -> player && j -> player -> is_pet() )
      return true;
    // Pet / Aura&Buff
    else if ( i -> player && i -> player -> is_pet() && ( ! j -> player || ! j -> player -> is_pet() ) )
      return false;
    // Pet / Pet
    else if ( i -> player && i -> player -> is_pet() && j -> player && j -> player -> is_pet() )
    {
      if ( i -> player -> name_str.compare( j -> player -> name_str ) == 0 )
        return ( i -> name_str.compare( j -> name_str ) < 0 );
      else
        return ( i -> player -> name_str.compare( j -> player -> name_str ) < 0 );
    }

    return ( i -> name_str.compare( j -> name_str ) < 0 );
  }
};

size_t player_chart_length( const player_t& p )
{
  const player_t* a = p.get_owner_or_self();
  assert( a );

  if ( ! a ) return 0; // For release builds.

  return static_cast<size_t>( a -> collected_data.fight_length.max() );
}

char stat_type_letter( stats_e type )
{
  switch ( type )
  {
    case STATS_ABSORB:
      return 'A';
    case STATS_DMG:
      return 'D';
    case STATS_HEAL:
      return 'H';
    case STATS_NEUTRAL:
    default:
      return 'X';
  }
}

class tooltip_parser_t
{
  struct error {};

  static const bool PARSE_DEBUG = true;

  const spell_data_t& default_spell;
  const dbc_t& dbc;
  const player_t* player; // For spell query tags (e.g., "$?s9999[Text if you have spell 9999][Text if you do not.]")
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
    unsigned id = parse_unsigned();
    const spell_data_t* s = dbc.spell( id );
    if ( s -> id() != id )
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
    bool show_scale_factor = effect.type() != E_APPLY_AURA;
    double s_min = dbc.effect_min( effect.id(), level );
    double s_max = dbc.effect_max( effect.id(), level );
    if ( s_min < 0 && s_max == s_min )
      s_max = s_min = -s_min;
    else if ( ( player && effect.type() == E_SCHOOL_DAMAGE && ( spell.get_school_type() & SCHOOL_MAGIC_MASK ) != 0 ) ||
              ( player && effect.type() == E_HEAL ) )
    {
      double power = effect.sp_coeff() * player -> initial.stats.spell_power;
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
  tooltip_parser_t( const dbc_t& d, int l, const spell_data_t& s, const std::string& t ) :
    default_spell( s ), dbc( d ), player( nullptr ), level( l ), text( t ), pos( t.begin() ) {}

  tooltip_parser_t( const player_t& p, const spell_data_t& s, const std::string& t ) :
    default_spell( s ), dbc( p.dbc ), player( &p ), level( p.true_level ), text( t ), pos( t.begin() ) {}

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
          timespan_t d = spell -> duration();
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
          replacement_text = util::to_string( 100 * spell -> proc_chance() );
          break;
        }

        case 'm':
        {
          ++pos;
          if ( parse_effect_number() <= spell -> effect_count() )
            replacement_text = util::to_string( spell -> effectN( parse_effect_number() ).base_value() );
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
          if ( parse_effect_number() <= spell -> effect_count() )
            replacement_text = util::to_string( spell -> effectN( parse_effect_number() ).period().total_seconds() );
          else
            replacement_text = util::to_string( 0 );
          break;
        }

        case 'u':
        {
          ++pos;
          replacement_text = util::to_string( spell -> max_stacks() );
          break;
        }

        case '?':
        {
          ++pos;
          if ( pos == text.end() || *pos != 's' )
            throw error();
          ++pos;
          spell = parse_spell();
          bool has_spell = false;
          if ( player )
          {
            has_spell = player -> find_class_spell( spell -> name_cstr() ) -> ok();
            if ( ! has_spell )
              has_spell = player -> find_glyph_spell( spell -> name_cstr() ) -> ok();
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
          if ( ! spell )
            throw error();
          assert( player );
          replacement_text = pretty_spell_text( *spell, spell -> desc(), *player );
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

} // UNNAMED NAMESPACE ======================================================

std::string pretty_spell_text( const spell_data_t& default_spell, const std::string& text, const player_t& p )
{ return tooltip_parser_t( p, default_spell, text ).parse(); }

// report::print_profiles ===================================================

void report::print_profiles( sim_t* sim )
{
  int k = 0;
  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    if ( p -> is_pet() ) continue;

    k++;

    if ( !p -> report_information.save_gear_str.empty() ) // Save gear
    {
      io::cfile file( p -> report_information.save_gear_str, "w" );
      if ( ! file )
      {
        sim -> errorf( "Unable to save gear profile %s for player %s\n", p -> report_information.save_gear_str.c_str(), p -> name() );
      }
      else
      {
        std::string profile_str = p -> create_profile( SAVE_GEAR );
        fprintf( file, "%s", profile_str.c_str() );
      }
    }

    if ( !p -> report_information.save_talents_str.empty() ) // Save talents
    {
      io::cfile file( p -> report_information.save_talents_str, "w" );
      if ( ! file )
      {
        sim -> errorf( "Unable to save talents profile %s for player %s\n", p -> report_information.save_talents_str.c_str(), p -> name() );
      }
      else
      {
        std::string profile_str = p -> create_profile( SAVE_TALENTS );
        fprintf( file, "%s", profile_str.c_str() );
      }
    }

    if ( !p -> report_information.save_actions_str.empty() ) // Save actions
    {
      io::cfile file( p -> report_information.save_actions_str, "w" );
      if ( ! file )
      {
        sim -> errorf( "Unable to save actions profile %s for player %s\n", p -> report_information.save_actions_str.c_str(), p -> name() );
      }
      else
      {
        std::string profile_str = p -> create_profile( SAVE_ACTIONS );
        fprintf( file, "%s", profile_str.c_str() );
      }
    }

    std::string file_name = p -> report_information.save_str;

    if ( file_name.empty() && sim -> save_profiles )
    {
      file_name  = sim -> save_prefix_str;
      file_name += p -> name_str;
      if ( sim -> save_talent_str != 0 )
      {
        file_name += "_";
        file_name += p -> primary_tree_name();
      }
      file_name += sim -> save_suffix_str;
      file_name += ".simc";
    }

    if ( file_name.empty() ) continue;

    io::cfile file( file_name, "w" );
    if ( ! file )
    {
      sim -> errorf( "Unable to save profile %s for player %s\n", file_name.c_str(), p -> name() );
      continue;
    }

    std::string profile_str = p -> create_profile();
    fprintf( file, "%s", profile_str.c_str() );
  }

  // Save overview file for Guild downloads
  //if ( /* guild parse */ )
  if ( sim -> save_raid_summary )
  {
    static const char* const filename = "Raid_Summary.simc";
    io::cfile file( filename, "w" );
    if ( ! file )
    {
      sim -> errorf( "Unable to save overview profile %s\n", filename );
    }
    else
    {
      fprintf( file, "#Raid Summary\n"
                     "# Contains %d Players.\n\n", k );

      for ( unsigned int i = 0; i < sim -> actor_list.size(); ++i )
      {
        player_t* p = sim -> actor_list[ i ];
        if ( p -> is_pet() ) continue;

        if ( ! p -> report_information.save_str.empty() )
          fprintf( file, "%s\n", p -> report_information.save_str.c_str() );
        else if ( sim -> save_profiles )
        {
          fprintf( file,
                   "# Player: %s Spec: %s Role: %s\n"
                   "%s%s",
                   p -> name(), p -> primary_tree_name(),
                   util::role_type_string( p -> primary_role() ),
                   sim -> save_prefix_str.c_str(), p -> name() );

          if ( sim -> save_talent_str != 0 )
            fprintf( file, "-%s", p -> primary_tree_name() );

          fprintf( file, "%s.simc\n\n", sim -> save_suffix_str.c_str() );
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
      out << spell_info::talent_to_str( sim.dbc, sim.dbc.talent( *i ) );
      break;
    case DATA_EFFECT:
      {
        std::ostringstream sqs;
        const spelleffect_data_t* base_effect = sim.dbc.effect( *i );
        if ( const spell_data_t* spell = dbc::find_spell( &( sim ), base_effect -> spell() ) )
        {
          spell_info::effect_to_str( sim.dbc, spell, dbc::find_effect( &( sim ), base_effect ), sqs );
          out << sqs.str();
        }
      }
      break;
    case DATA_SET_BONUS:
      out << spell_info::set_bonus_to_str( sim.dbc, dbc::set_bonus( sim.dbc.ptr ) + *i );
      break;
    default:
      {
        const spell_data_t* spell = dbc::find_spell( &( sim ), sim.dbc.spell( *i ) );
        out << spell_info::to_str( sim.dbc, spell, level );
      }
    }
  }
}

void report::print_spell_query( xml_node_t* root, FILE* file, const sim_t& sim, const spell_data_expr_t& sq, unsigned level )
{

  expr_data_e data_type = sq.data_type;
  for ( auto i = sq.result_spell_list.begin(); i != sq.result_spell_list.end(); ++i )
  {
    switch ( data_type )
    {
    case DATA_TALENT:
      spell_info::talent_to_xml( sim.dbc, sim.dbc.talent( *i ), root );
      break;
    case DATA_EFFECT:
      {
        std::ostringstream sqs;
        const spelleffect_data_t* dbc_effect = sim.dbc.effect( *i );
        if ( const spell_data_t* spell = dbc::find_spell( &( sim ), dbc_effect -> spell() ) )
        {
          spell_info::effect_to_xml( sim.dbc, spell, dbc::find_effect( &( sim ), dbc_effect ), root );
        }
      }
      break;
    case DATA_SET_BONUS:
      spell_info::talent_to_xml( sim.dbc, sim.dbc.talent( *i ), root );
      break;
    default:
      {
        const spell_data_t* spell = dbc::find_spell( &( sim ), sim.dbc.spell( *i ) );
        spell_info::to_xml( sim.dbc, spell, root, level );
      }
    }
  }

  util::fprintf( file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
  root -> print_xml( file );
}
// report::print_suite ======================================================

void report::print_suite( sim_t* sim )
{
  std::cout << "\nGenerating reports..." << std::endl;

  report::print_text( sim, sim -> report_details != 0 );

  report::print_html( *sim );
  report::print_xml( sim );
  report::print_json( *sim );
  report::print_profiles( sim );
}

void report::print_html_sample_data( report::sc_html_stream& os, const player_t& p, const extended_sample_data_t& data, const std::string& name, int& td_counter, int columns )
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
  if ( ! p.sim -> enable_highcharts )
  {
    os.format( "<a class=\"toggle-details\">%s</a></td>\n",
               name.c_str() );
  }
  else
  {
    std::string tokenized_name = name;
    util::tokenize( tokenized_name );
    os.format( "<a id=\"actor%d_%s_stats_toggle\" class=\"toggle-details\">%s</a></td>\n",
               p.index, tokenized_name.c_str(), name.c_str() );
  }

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
  os.format(
    "\t\t\t\t\t\t\t\t\t<td class=\"left\">Count</td>\n"
    "\t\t\t\t\t\t\t\t\t<td class=\"right\">%d</td>\n"
    "\t\t\t\t\t\t\t\t</tr>\n",
    (int) data.size() );

  ++i;
  os << "\t\t\t\t\t\t\t\t<tr";
  if ( !( i & 1 ) )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  os.format(
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

  os.format(
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
  os.format(
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
  os.format(
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
  os.format(
    "\t\t\t\t\t\t\t\t\t<td class=\"left\">Range [ ( max - min ) / 2 * 100%% ]</td>\n"
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
    os.format(
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
    os.format(
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
    os.format(
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
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">( 95th Percentile - 5th Percentile )</td>\n"
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
    os.format(
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
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">Standard Deviation</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.4f</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      data.mean_std_dev );

    ++i;
    double mean_error = data.mean_std_dev * p.sim -> confidence_estimator;
    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">%.2f%% Confidence Intervall</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">( %.2f - %.2f )</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      p.sim -> confidence * 100.0,
      data.mean() - mean_error,
      data.mean() + mean_error );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">Normalized %.2f%% Confidence Intervall</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">( %.2f%% - %.2f%% )</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      p.sim -> confidence * 100.0,
      data.mean() ? 100 - mean_error * 100 / data.mean() : 0,
      data.mean() ? 100 + mean_error * 100 / data.mean() : 0 );



    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"left\"><b>Approx. Iterations needed for ( always use n>=50 )</b></td>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"right\"></td>\n"
       << "\t\t\t\t\t\t\t\t</tr>\n";

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">1%% Error</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      ( int ) ( data.mean() ? ( ( mean_error * mean_error * ( ( float ) data.size() ) / ( 0.01 * data.mean() * 0.01 * data.mean() ) ) ) : 0 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.1%% Error</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      ( int ) ( data.mean() ? ( ( mean_error * mean_error * ( ( float ) data.size() ) / ( 0.001 * data.mean() * 0.001 * data.mean() ) ) ) : 0 ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.1 Scale Factor Error with Delta=300</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      ( int ) ( 2.0 * mean_error * mean_error * ( ( float ) data.size() ) / ( 30 * 30 ) ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.05 Scale Factor Error with Delta=300</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      ( int ) ( 2.0 * mean_error * mean_error * ( ( float ) data.size() ) / ( 15 * 15 ) ) );

    ++i;
    os << "\t\t\t\t\t\t\t\t<tr";
    if ( !( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.01 Scale Factor Error with Delta=300</td>\n"
      "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
      "\t\t\t\t\t\t\t\t</tr>\n",
      ( int ) (  2.0 * mean_error * mean_error * ( ( float ) data.size() ) / ( 3 * 3 ) ) );

  }

  os << "\t\t\t\t\t\t\t\t</table>\n";

  if ( ! data.simple )
  {
    if ( ! p.sim -> enable_highcharts )
    {
      std::string dist_chart = chart::distribution( data.distribution, name, data.mean(), data.min(), data.max() );

      os.format(
        "\t\t\t\t\t<img src=\"%s\" alt=\"Distribution Chart\" />\n",
        dist_chart.c_str() );
    }
    else
    {
      std::string tokenized_div_name = data.name_str + "_dist";
      util::tokenize( tokenized_div_name );

      highchart::histogram_chart_t chart( tokenized_div_name, *p.sim );
      chart.set_toggle_id( "actor" + util::to_string( p.index ) + "_" + tokenized_div_name + "_stats_toggle" );
      if ( chart::generate_distribution( chart, nullptr, data.distribution, name, data.mean(), data.min(), data.max() ) )
      {
        os << chart.to_target_div();
        p.sim -> add_chart_data( chart );
      }
    }
  }


  os << "\t\t\t\t\t\t\t\t</td>\n"
     << "\t\t\t\t\t\t\t</tr>\n";
}

void report::generate_player_buff_lists( player_t&  p, player_processed_report_information_t& ri )
{
  if ( ri.buff_lists_generated )
    return;

  // Append p.buff_list to ri.buff_list
  ri.buff_list.insert( ri.buff_list.end(), p.buff_list.begin(), p.buff_list.end() );

  for ( const auto& pet : p.pet_list )
  {
    // Append pet -> buff_list to ri.buff_list
    ri.buff_list.insert( ri.buff_list.end(), pet -> buff_list.begin(), pet -> buff_list.end() );
  }

  // Append p.sim -> buff_list to ri.buff_list
  ri.buff_list.insert( ri.buff_list.end(), p.sim -> buff_list.begin(), p.sim -> buff_list.end() );

  // Filter out non-dynamic buffs, copy them into ri.dynamic_buffs and sort
  //range::remove_copy_if( ri.buff_list, back_inserter( ri.dynamic_buffs ), buff_is_dynamic );
  range::remove_copy_if( ri.buff_list, back_inserter( ri.dynamic_buffs ), buff_is_dynamic() );
  range::sort( ri.dynamic_buffs, buff_comp() );

  // Filter out non-constant buffs, copy them into ri.constant_buffs and sort
  range::remove_copy_if( ri.buff_list, back_inserter( ri.constant_buffs ), buff_is_constant() );
  range::sort( ri.constant_buffs, buff_comp() );

  ri.buff_lists_generated = true;
}

void report::generate_player_charts( player_t& p, player_processed_report_information_t& ri )
{
  if ( ri.charts_generated )
    return;

  if ( ! p.sim -> enable_highcharts )
  {
    const player_collected_data_t& cd = p.collected_data;

    // Pet Chart Adjustment ===================================================
    size_t max_buckets = player_chart_length( p );

    // Stats Charts
    std::vector<stats_t*> stats_list;

    // Append p.stats_list to stats_list
    stats_list.insert( stats_list.end(), p.stats_list.begin(), p.stats_list.end() );

    for ( size_t i = 0; i < p.pet_list.size(); ++i )
    {
      pet_t* pet = p.pet_list[ i ];
      // Append pet -> stats_list to stats_list
      stats_list.insert( stats_list.end(), pet -> stats_list.begin(), pet -> stats_list.end() );
    }

    if ( ! p.is_pet() )
    {
      for ( size_t i = 0; i < stats_list.size(); i++ )
      {
        stats_t* s = stats_list[ i ];

        // Create Stats Timeline Chart
        sc_timeline_t timeline_aps;
        s -> timeline_amount.build_derivative_timeline( timeline_aps );
        s -> timeline_aps_chart = chart::timeline( timeline_aps.data(), s -> name_str + ' ' + stat_type_letter( s -> type ) + "PS", timeline_aps.mean() );
        s -> aps_distribution_chart = chart::distribution( s -> portion_aps.distribution, s -> name_str + ( s -> type == STATS_DMG ? " DPS" : " HPS" ),
                                                           s -> portion_aps.mean(), s -> portion_aps.min(), s -> portion_aps.max() );
      }
    }
    // End Stats Charts

    // Player Charts
    ri.action_dpet_chart    = chart::action_dpet  ( p );
    ri.action_dmg_chart     = chart::aps_portion  ( p );
    ri.time_spent_chart     = chart::time_spent   ( p );
    ri.scaling_dps_chart    = chart::scaling_dps  ( p );
    ri.reforge_dps_charts    = chart::reforge_dps  ( p );
    ri.scale_factors_chart  = chart::scale_factors( p );

    std::string encoded_name = util::google_image_chart_encode( p.name_str );
    util::urlencode( encoded_name );

    {
      sc_timeline_t timeline_dps;
      p.collected_data.timeline_dmg.build_derivative_timeline( timeline_dps );
      ri.timeline_dps_chart = chart::timeline( timeline_dps.data(), encoded_name + " DPS", cd.dps.mean() );
    }

    ri.timeline_dps_error_chart = chart::timeline_dps_error( p );
    ri.dps_error_chart = chart::dps_error( p );

    if ( p.primary_role() == ROLE_HEAL )
    {
      ri.distribution_dps_chart = chart::distribution( cd.hps.distribution, encoded_name + " HPS",
                                                       cd.hps.mean(),
                                                       cd.hps.min(),
                                                       cd.hps.max() );
    }
    else
    {
      ri.distribution_dps_chart = chart::distribution( cd.dps.distribution, encoded_name + " DPS",
                                                       cd.dps.mean(),
                                                       cd.dps.min(),
                                                       cd.dps.max() );
    }

    ri.distribution_deaths_chart = chart::distribution( cd.deaths.distribution, encoded_name + " Death",
                                                        cd.deaths.mean(),
                                                        cd.deaths.min(),
                                                        cd.deaths.max() );

    // Resource Charts
    for ( size_t i = 0; i < cd.resource_timelines.size(); ++i )
    {
      resource_e rt = cd.resource_timelines[ i ].type;
      ri.timeline_resource_chart[ rt ] =
        chart::timeline( cd.resource_timelines[ i ].timeline.data(),
                         encoded_name + ' ' + util::inverse_tokenize( util::resource_type_string( rt ) ),
                         cd.resource_timelines[ i ].timeline.mean(),
                         color::resource_color( rt ).hex_str(),
                         max_buckets );
      ri.gains_chart[ rt ] = chart::gains( p, rt );
    }

    // Stat Charts
    for ( size_t i = 0; i < cd.stat_timelines.size(); ++i )
    {
      stat_e st = cd.stat_timelines[ i ].type;
      if ( cd.stat_timelines[ i ].timeline.mean() >  0 )
      {
        ri.timeline_stat_chart[ st ] =
          chart::timeline( cd.stat_timelines[ i ].timeline.data(),
                           encoded_name + ' ' + util::inverse_tokenize( util::stat_type_string( st ) ),
                           cd.stat_timelines[ i ].timeline.mean(),
                           color::stat_color( st ).hex_str(),
                           max_buckets );
      }
    }

    if ( ! p.is_pet() && p.primary_role() == ROLE_TANK )
    {
      ri.health_change_chart =
        chart::timeline( cd.health_changes.merged_timeline.data(),
                         encoded_name + ' ' + "Health Change",
                         cd.health_changes.merged_timeline.mean(),
                         color::resource_color( RESOURCE_HEALTH ).hex_str(),
                         max_buckets );

      sc_timeline_t sliding_average_tl;
      cd.health_changes.merged_timeline.build_sliding_average_timeline( sliding_average_tl, (int)p.tmi_window );
      ri.health_change_sliding_chart =
        chart::timeline( sliding_average_tl.data(),
                         encoded_name + ' ' + "Health Change (" + util::to_string( p.tmi_window, 2 ) + "s moving avg.)",
                         sliding_average_tl.mean(),
                         color::resource_color( RESOURCE_HEALTH ).hex_str(),
                         max_buckets );
    }
  }

  // Scaling charts
  if ( ! ( ( p.sim -> scaling -> num_scaling_stats <= 0 ) || p.quiet || p.is_pet() || p.is_enemy() || p.is_add() || p.type == HEALING_ENEMY ) )
  {
    ri.gear_weights_lootrank_link        = chart::gear_weights_lootrank   ( p );
    ri.gear_weights_wowhead_std_link     = chart::gear_weights_wowhead    ( p );
    ri.gear_weights_pawn_string          = chart::gear_weights_pawn       ( p );
    ri.gear_weights_askmrrobot_link      = chart::gear_weights_askmrrobot ( p );
  }

  // Create html profile str
  ri.html_profile_str = p.create_profile( SAVE_ALL );

  ri.charts_generated = true;
}

void report::generate_sim_report_information( const sim_t& sim , sim_report_information_t& ri )
{
  if ( ri.charts_generated )
    return;

  if ( sim.enable_highcharts )
    return;

  ri.downtime_chart = chart::raid_downtime( sim.players_by_name );
  
  chart::raid_aps     ( ri.dps_charts, sim, sim.players_by_dps, "dps" );
  chart::raid_aps     ( ri.priority_dps_charts, sim, sim.players_by_priority_dps, "prioritydps" );
  chart::raid_aps     ( ri.hps_charts, sim, sim.players_by_hps_plus_aps, "hps" );
  chart::raid_aps     ( ri.dtps_charts, sim, sim.players_by_dtps, "dtps" );
  chart::raid_aps     ( ri.tmi_charts, sim, sim.players_by_tmi, "tmi" );
  chart::raid_aps     ( ri.apm_charts, sim, sim.players_by_apm, "apm" );
  chart::raid_dpet    ( ri.dpet_charts, sim );
  ri.timeline_chart = chart::distribution( sim.simulation_length.distribution, "Timeline",
                                           sim.simulation_length.mean(),
                                           sim.simulation_length.min(),
                                           sim.simulation_length.max() );

  ri.charts_generated = true;
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
  if ( ! p -> sim -> scaling -> has_scale_factors() ||
       p -> quiet || p -> is_pet() || p -> is_enemy() || p -> type == HEALING_ENEMY )
  {
    return false;
  }

  return true;
}

static bool find_affix( const std::string&  name,
                        const std::string&  data_name,
                        std::string&        prefix,
                        std::string&        suffix )
{
  std::string tokenized_name = data_name;
  util::tokenize( tokenized_name );

  std::string::size_type affix_offset = name.find( tokenized_name );

  // Add an affix to the name, if the name does not match the 
  // spell name. Affix is either the prefix- or suffix portion of the 
  // non matching parts of the stats name.
  if ( affix_offset != std::string::npos && tokenized_name != name )
  {
    // Suffix
    if ( affix_offset == 0 )
      suffix = "&#160;(" + name.substr( tokenized_name.size() ) + ")";
    // Prefix
    else if ( affix_offset > 0 )
      prefix = "(" + name.substr( 0, affix_offset ) + ")&#160;";
  }
  else if ( affix_offset == std::string::npos )
    suffix = "&#160;(" + name + ")";

  return ! prefix.empty() || ! suffix.empty();
}

const color::rgb& report::item_quality_color( const item_t& item )
{
  switch ( item.parsed.data.quality )
  {
    case 1:  return color::COMMON;
    case 2:  return color::UNCOMMON;
    case 3:  return color::RARE;
    case 4:  return color::EPIC;
    case 5:  return color::LEGENDARY;
    case 6:  return color::HEIRLOOM;
    case 7:  return color::HEIRLOOM;
    default: return color::POOR;
  }
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
#endif
}

std::string report::decorated_buff_name( const buff_t* buff )
{
  std::stringstream s;

  if ( buff -> sim -> decorated_tooltips == false || buff -> data().id() == 0 )
  {
    s << "<a href=\"#\">" << buff -> name_str << "</a>";
  }
  else
  {
    std::string prefix, suffix;
    find_affix( buff -> name_str, buff -> data().name_cstr(), prefix, suffix );

    s << prefix << "<a href=\"http://" << decoration_domain( *buff -> sim ) << ".wowdb.com/spells/"
      << buff -> data().id();

    if ( buff -> item )
    {
      s << "?itemLevel=" << buff -> item -> item_level();
    }
    s << "\">" << buff -> data().name_cstr() << "</a>" << suffix;
  }

  return s.str();
}

std::string report::decorated_spell_name( const sim_t& sim, const spell_data_t& spell )
{
  std::stringstream s;

  if ( sim.decorated_tooltips == false )
  {
    s << "<a href=\"#\">" << spell.name_cstr() << "</a>";
  }
  else
  {
    s << "<a href=\"http://" << decoration_domain( sim ) << ".wowdb.com/spells/" << spell.id()
      << "\">" << spell.name_cstr() << "</a>";
  }

  return s.str();
}

std::string report::decorated_item_name( const item_t* item )
{
  std::stringstream s;

  if ( item -> sim -> decorated_tooltips == false || item -> parsed.data.id == 0 )
  {
    s << "<a style=\"color:" << item_quality_color( *item ) << ";\" href=\"#\">" << item -> full_name() << "</a>";
  }
  else
  {

    std::vector<std::string> params;
    if ( item -> parsed.enchant_id > 0 )
    {
      params.push_back( "enchantment=" + util::to_string( item -> parsed.enchant_id ) );
    }

    if ( item -> parsed.upgrade_level > 0 )
    {
      params.push_back( "upgradeNum=" + util::to_string( item -> parsed.upgrade_level ) );
    }

    std::string gem_str = "";
    for ( size_t i = 0; i < item -> parsed.gem_id.size(); ++i )
    {
      if ( item -> parsed.gem_id[ i ] == 0 )
      {
        continue;
      }

      if ( ! gem_str.empty() )
      {
        gem_str += ",";
      }

      gem_str += util::to_string( item -> parsed.gem_id[ i ] );
    }

    if ( ! gem_str.empty() )
    {
      params.push_back( "gems=" + gem_str );
    }

    std::string bonus_str = "";
    for ( size_t i = 0; i < item -> parsed.bonus_id.size(); ++i )
    {
      bonus_str += util::to_string( item -> parsed.bonus_id[ i ] );
      if ( i < item -> parsed.bonus_id.size() - 1 )
      {
        bonus_str += ",";
      }
    }

    if ( ! bonus_str.empty() )
    {
      params.push_back( "bonusIDs=" + bonus_str );
    }

    s << "<a style=\"color:" << item_quality_color( *item ) << ";\" href=\"http://"
      << decoration_domain( *item -> sim ) << ".wowdb.com/items/" << item -> parsed.data.id;

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

    s << "\">" << item -> full_name() << "</a>";
  }

  return s.str();
}

std::string report::decorated_action_name( const action_t* action )
{
  std::stringstream s;

  if ( action -> sim -> decorated_tooltips == false || action -> data().id() == 0 )
  {
    s << "<a href=\"#\">" << action -> name_str << "</a>";
  }
  else
  {
    std::string prefix, suffix;
    find_affix( action -> name_str, action -> data().name_cstr(), prefix, suffix );

    s << prefix << "<a href=\"http://" << decoration_domain( *action -> sim ) << ".wowdb.com/spells/"
      << action -> data().id();
    if ( action -> item )
    {
      s << "?itemLevel=" << action -> item -> item_level();
    }
    s << "\">" << action -> data().name_cstr() << "</a>" << suffix;
  }

  return s.str();
}

namespace color
{
rgb::rgb() : r_( 0 ), g_( 0 ), b_( 0 ), a_( 255 )
{ }

rgb::rgb( unsigned char r, unsigned char g, unsigned char b ) :
  r_( r ), g_( g ), b_( b ), a_( 255 )
{ }

rgb::rgb( double r, double g, double b ) :
  r_( static_cast<unsigned char>( r * 255 ) ),
  g_( static_cast<unsigned char>( g * 255 ) ),
  b_( static_cast<unsigned char>( b * 255 ) ),
  a_( 1 )
{ }

rgb::rgb( const std::string& color ) :
  r_( 0 ), g_( 0 ), b_( 0 ), a_( 255 )
{
  parse_color( color );
}

rgb::rgb( const char* color ) :
  r_( 0 ), g_( 0 ), b_( 0 ), a_( 255 )
{
  parse_color( color );
}

std::string rgb::rgb_str() const
{
  std::stringstream s;

  s << "rgba(" << static_cast<unsigned>( r_ ) << ", "
               << static_cast<unsigned>( g_ ) << ", "
               << static_cast<unsigned>( b_ ) << ", "
               << a_ << ")";

  return s.str();
}

std::string rgb::str() const
{ return *this; }

std::string rgb::hex_str() const
{ return str().substr( 1 ); }

rgb& rgb::adjust( double v )
{
  if ( v < 0 || v > 1 )
  {
    return *this;
  }

  r_ *= v; g_ *= v; b_ *= v;
  return *this;
}

rgb rgb::adjust( double v ) const
{ return rgb( *this ).adjust( v ); }

rgb rgb::dark( double pct ) const
{ return rgb( *this ).adjust( 1.0 - pct ); }

rgb rgb::light( double pct ) const
{ return rgb( *this ).adjust( 1.0 + pct ); }

rgb rgb::opacity( double pct ) const
{
  rgb r( *this );
  r.a_ = pct;

  return r;
}

rgb& rgb::operator=( const std::string& color_str )
{
  parse_color( color_str );
  return *this;
}

rgb& rgb::operator+=( const rgb& other )
{
  if ( this == &( other ) )
  {
    return *this;
  }

  unsigned mix_r = ( r_ + other.r_ ) / 2;
  unsigned mix_g = ( g_ + other.g_ ) / 2;
  unsigned mix_b = ( b_ + other.b_ ) / 2;

  r_ = static_cast<unsigned char>( mix_r );
  g_ = static_cast<unsigned char>( mix_g );
  b_ = static_cast<unsigned char>( mix_b );

  return *this;
}

rgb rgb::operator+( const rgb& other ) const
{
  rgb new_color( *this );
  new_color += other;
  return new_color;
}

rgb::operator std::string() const
{
  std::stringstream s;
  operator<<( s, *this );
  return s.str();
}

bool rgb::parse_color( const std::string& color_str )
{
  std::stringstream i( color_str );

  if ( color_str.size() < 6 || color_str.size() > 7 )
  {
    return false;
  }

  if ( i.peek() == '#' )
  {
    i.get();
  }

  unsigned v = 0;
  i >> std::hex >> v;
  if ( i.fail() )
  {
    return false;
  }

  r_ = static_cast<unsigned char>( v / 0x10000 );
  g_ = static_cast<unsigned char>( ( v / 0x100 ) % 0x100 );
  b_ = static_cast<unsigned char>( v % 0x100 );

  return true;
}

std::ostream& operator<<( std::ostream& s, const rgb& r )
{
  s << '#';
  s << std::setfill('0') << std::internal << std::uppercase << std::hex;
  s << std::setw( 2 ) << static_cast<unsigned>( r.r_ )
    << std::setw( 2 ) << static_cast<unsigned>( r.g_ )
    << std::setw( 2 ) << static_cast<unsigned>( r.b_ );
  s << std::dec;
  return s;
}

rgb class_color( player_e type )
{
  switch ( type )
  {
    case PLAYER_NONE:     return color::GREY;
    case PLAYER_GUARDIAN: return color::GREY;
    case DEATH_KNIGHT:    return color::COLOR_DEATH_KNIGHT;
    case DRUID:           return color::COLOR_DRUID;
    case HUNTER:          return color::COLOR_HUNTER;
    case MAGE:            return color::COLOR_MAGE;
    case MONK:            return color::COLOR_MONK;
    case PALADIN:         return color::COLOR_PALADIN;
    case PRIEST:          return color::COLOR_PRIEST;
    case ROGUE:           return color::COLOR_ROGUE;
    case SHAMAN:          return color::COLOR_SHAMAN;
    case WARLOCK:         return color::COLOR_WARLOCK;
    case WARRIOR:         return color::COLOR_WARRIOR;
    case ENEMY:           return color::GREY;
    case ENEMY_ADD:       return color::GREY;
    case HEALING_ENEMY:   return color::GREY;
    case TMI_BOSS:        return color::GREY;
    case TANK_DUMMY:      return color::GREY;
    default:              return color::GREY2;
  }
}

rgb resource_color( resource_e type )
{
  switch ( type )
  {
    case RESOURCE_HEALTH:
    case RESOURCE_RUNE_UNHOLY:   return class_color( HUNTER );

    case RESOURCE_RUNE_FROST:
    case RESOURCE_MANA:          return class_color( SHAMAN );

    case RESOURCE_ENERGY:
    case RESOURCE_FOCUS:
    case RESOURCE_COMBO_POINT:   return class_color( ROGUE );

    case RESOURCE_RAGE:
    case RESOURCE_RUNIC_POWER:
    case RESOURCE_RUNE:
    case RESOURCE_RUNE_BLOOD:    return class_color( DEATH_KNIGHT );

    case RESOURCE_HOLY_POWER:    return class_color( PALADIN );

    case RESOURCE_SOUL_SHARD:
    case RESOURCE_BURNING_EMBER:
    case RESOURCE_DEMONIC_FURY:  return class_color( WARLOCK );

    case RESOURCE_ECLIPSE:       return class_color( DRUID );

    case RESOURCE_CHI:           return class_color( MONK );

    case RESOURCE_NONE:
    default:                     return GREY2;
  }
}

rgb stat_color( stat_e type )
{
  switch ( type )
  {
    case STAT_STRENGTH:                 return COLOR_WARRIOR;
    case STAT_AGILITY:                  return COLOR_HUNTER;
    case STAT_INTELLECT:                return COLOR_MAGE;
    case STAT_SPIRIT:                   return GREY3;
    case STAT_ATTACK_POWER:             return COLOR_ROGUE;
    case STAT_SPELL_POWER:              return COLOR_WARLOCK;
    case STAT_READINESS_RATING:         return COLOR_DEATH_KNIGHT;
    case STAT_CRIT_RATING:              return COLOR_PALADIN;
    case STAT_HASTE_RATING:             return COLOR_SHAMAN;
    case STAT_MASTERY_RATING:           return COLOR_ROGUE.dark();
    case STAT_MULTISTRIKE_RATING:       return COLOR_DEATH_KNIGHT + COLOR_WARRIOR;
    case STAT_DODGE_RATING:             return COLOR_MONK;
    case STAT_PARRY_RATING:             return TEAL;
    case STAT_ARMOR:                    return COLOR_PRIEST;
    case STAT_BONUS_ARMOR:              return COLOR_PRIEST;
    case STAT_VERSATILITY_RATING:       return PURPLE.dark();
    default:                            return GREY2;
  }
}

/* Blizzard shool colors:
 * http://wowprogramming.com/utils/xmlbrowser/live/AddOns/Blizzard_CombatLog/Blizzard_CombatLog.lua
 * search for: SchoolStringTable
 */
// These colors are picked to sort of line up with classes, but match the "feel" of the spell class' color
rgb school_color( school_e type )
{
  switch ( type )
  {
    // -- Single Schools
    case SCHOOL_NONE:         return color::COLOR_NONE;
    case SCHOOL_PHYSICAL:     return color::PHYSICAL;
    case SCHOOL_HOLY:         return color::HOLY;
    case SCHOOL_FIRE:         return color::FIRE;
    case SCHOOL_NATURE:       return color::NATURE;
    case SCHOOL_FROST:        return color::FROST;
    case SCHOOL_SHADOW:       return color::SHADOW;
    case SCHOOL_ARCANE:       return color::ARCANE;
    // -- Physical and a Magical
    case SCHOOL_FLAMESTRIKE:  return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_FIRE );
    case SCHOOL_FROSTSTRIKE:  return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_FROST );
    case SCHOOL_SPELLSTRIKE:  return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_ARCANE );
    case SCHOOL_STORMSTRIKE:  return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_NATURE );
    case SCHOOL_SHADOWSTRIKE: return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_SHADOW );
    case SCHOOL_HOLYSTRIKE:   return school_color( SCHOOL_PHYSICAL ) + school_color( SCHOOL_HOLY );
      // -- Two Magical Schools
    case SCHOOL_FROSTFIRE:    return color::FROSTFIRE;
    case SCHOOL_SPELLFIRE:    return school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_FIRE );
    case SCHOOL_FIRESTORM:    return school_color( SCHOOL_FIRE ) + school_color( SCHOOL_NATURE );
    case SCHOOL_SHADOWFLAME:  return school_color( SCHOOL_SHADOW ) + school_color( SCHOOL_FIRE );
    case SCHOOL_HOLYFIRE:     return school_color( SCHOOL_HOLY ) + school_color( SCHOOL_FIRE );
    case SCHOOL_SPELLFROST:   return school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_FROST );
    case SCHOOL_FROSTSTORM:   return school_color( SCHOOL_FROST ) + school_color( SCHOOL_NATURE );
    case SCHOOL_SHADOWFROST:  return school_color( SCHOOL_SHADOW ) + school_color( SCHOOL_FROST );
    case SCHOOL_HOLYFROST:    return school_color( SCHOOL_HOLY ) + school_color( SCHOOL_FROST );
    case SCHOOL_SPELLSTORM:   return school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_NATURE );
    case SCHOOL_SPELLSHADOW:  return school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_SHADOW );
    case SCHOOL_DIVINE:       return school_color( SCHOOL_ARCANE ) + school_color( SCHOOL_HOLY );
    case SCHOOL_SHADOWSTORM:  return school_color( SCHOOL_SHADOW ) + school_color( SCHOOL_NATURE );
    case SCHOOL_HOLYSTORM:    return school_color( SCHOOL_HOLY ) + school_color( SCHOOL_NATURE );
    case SCHOOL_SHADOWLIGHT:  return school_color( SCHOOL_SHADOW ) + school_color( SCHOOL_HOLY );
      //-- Three or more schools
    case SCHOOL_ELEMENTAL:    return color::ELEMENTAL;
    case SCHOOL_CHROMATIC:    return school_color( SCHOOL_FIRE ) +
                                       school_color( SCHOOL_FROST ) +
                                       school_color( SCHOOL_ARCANE ) +
                                       school_color( SCHOOL_NATURE ) +
                                       school_color( SCHOOL_SHADOW );
    case SCHOOL_MAGIC:    return school_color( SCHOOL_FIRE ) +
                                   school_color( SCHOOL_FROST ) +
                                   school_color( SCHOOL_ARCANE ) +
                                   school_color( SCHOOL_NATURE ) +
                                   school_color( SCHOOL_SHADOW ) +
                                   school_color( SCHOOL_HOLY );
    case SCHOOL_CHAOS:    return school_color( SCHOOL_PHYSICAL ) +
                                   school_color( SCHOOL_FIRE ) +
                                   school_color( SCHOOL_FROST ) +
                                   school_color( SCHOOL_ARCANE ) +
                                   school_color( SCHOOL_NATURE ) +
                                   school_color( SCHOOL_SHADOW ) +
                                   school_color( SCHOOL_HOLY );

    default:
                          return GREY2;
  }
}
} /* namespace color */
