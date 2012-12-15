// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_report.hpp"
#include <stack>

namespace { // UNNAMED NAMESPACE ==========================================

class xml_writer_t
{
private:
  FILE* file;
  enum state
  {
    NONE, TAG, TEXT
  };
  std::stack<std::string> current_tags;
  std::string tabulation;
  state current_state;
  std::string indentation;

  struct replacement
  {
    char from;
    const char* to;
  };

public:
  xml_writer_t( const std::string & filename )
    : tabulation( "  " ), current_state( NONE )
  {
    file = io::fopen( filename, "w" );
  }

  ~xml_writer_t()
  {
    if ( file )
    {
      fclose( file );
    }
  }

  bool ready() { return file != NULL; }

  void set_tabulation( const std::string & tabulation_ )
  {
    int n = indentation.size() / tabulation.size();
    tabulation = tabulation_;
    indentation.clear();
    while ( --n >= 0 )
      indentation += tabulation;
  }

  int printf( const char *format, ... ) const PRINTF_ATTRIBUTE( 2,3 )
  {
    va_list fmtargs;
    va_start( fmtargs, format );

    int retcode = vfprintf( file, format, fmtargs );

    va_end( fmtargs );

    return retcode;
  }

  void init_document( const std::string & stylesheet_file )
  {
    assert( current_state == NONE );

    printf( "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
    if ( !stylesheet_file.empty() )
    {
      printf( "<?xml-stylesheet type=\"text/xml\" href=\"%s\"?>", stylesheet_file.c_str() );
    }

    current_state = TEXT;
  }

  void begin_tag( const std::string & tag )
  {
    assert( current_state != NONE );

    if ( current_state != TEXT )
    {
      printf( ">" );
    }

    printf( "\n%s<%s", indentation.c_str(), tag.c_str() );

    current_tags.push( tag );
    indentation += tabulation;

    current_state = TAG;
  }

  void end_tag( const std::string & tag )
  {
    assert( current_state != NONE );
    assert( ! current_tags.empty() );
    assert( indentation.size() == tabulation.size() * current_tags.size() );

    assert( tag == current_tags.top() );
    current_tags.pop();

    indentation.resize( indentation.size() - tabulation.size() );

    if ( current_state == TAG )
    {
      printf( "/>" );
    }
    else if ( current_state == TEXT )
    {
      printf( "\n%s</%s>", indentation.c_str(), tag.c_str() );
    }

    current_state = TEXT;
  }

  void print_attribute( const std::string & name, const std::string & value )
  { print_attribute_unescaped( name, sanitize( value ) ); }

  void print_attribute_unescaped( const std::string & name, const std::string & value )
  {
    assert( current_state != NONE );

    if ( current_state == TAG )
    {
      printf( " %s=\"%s\"", name.c_str(), value.c_str() );
    }
  }

  void print_tag( const std::string & name, const std::string & inner_value )
  {
    assert( current_state != NONE );

    if ( current_state != TEXT )
    {
      printf( ">" );
    }

    printf( "\n%s<%s>%s</%s>", indentation.c_str(), name.c_str(), sanitize( inner_value ).c_str(), name.c_str() );

    current_state = TEXT;
  }

  void print_text( const std::string & input )
  {
    assert( current_state != NONE );

    if ( current_state != TEXT )
    {
      printf( ">" );
    }

    printf( "\n%s", sanitize( input ).c_str() );

    current_state = TEXT;
  }

  static std::string sanitize( std::string v )
  {
    static const replacement replacements[] =
    {
      { '&', "&amp;" },
      { '"', "&quot;" },
      { '<', "&lt;" },
      { '>', "&gt;" },
    };

    for ( unsigned int i = 0; i < sizeof_array( replacements ); ++i )
      util::replace_all( v, replacements[ i ].from, replacements[ i ].to );

    return v;
  }
};

// report::print_xml ======================================================

void print_xml_errors( sim_t* sim, xml_writer_t & writer );
void print_xml_raid_events( sim_t* sim, xml_writer_t & writer );
void print_xml_roster( sim_t* sim, xml_writer_t & writer );
void print_xml_targets( sim_t* sim, xml_writer_t & writer );
void print_xml_buffs( sim_t* sim, xml_writer_t & writer );
void print_xml_hat_donors( sim_t* sim, xml_writer_t & writer );
void print_xml_performance( sim_t* sim, xml_writer_t & writer );
void print_xml_summary( sim_t* sim, xml_writer_t & writer, sim_t::report_information_t& );
void print_xml_player( sim_t* sim, xml_writer_t & writer, player_t * p, player_t * owner );

void print_xml_player_stats( xml_writer_t & writer, player_t * p );
void print_xml_player_attribute( xml_writer_t & writer, const std::string& attribute, double initial, double gear, double buffed );
void print_xml_player_actions( xml_writer_t & writer, player_t* p );
void print_xml_player_action_definitions( xml_writer_t & writer, player_t * p );
void print_xml_player_buffs( xml_writer_t & writer, player_t * p );
void print_xml_player_uptime( xml_writer_t & writer, player_t * p );
void print_xml_player_procs( xml_writer_t & writer, player_t * p );
void print_xml_player_gains( xml_writer_t & writer, player_t * p );
void print_xml_player_scale_factors( xml_writer_t & writer, player_t * p, player_t::report_information_t& );
void print_xml_player_dps_plots( xml_writer_t & writer, player_t * p );
void print_xml_player_charts( xml_writer_t & writer, player_t::report_information_t& );

void print_xml_errors( sim_t* sim, xml_writer_t & writer )
{
  size_t num_errors = sim -> error_list.size();
  if ( num_errors > 0 )
  {
    writer.begin_tag( "errors" );
    for ( size_t i=0; i < num_errors; i++ )
    {
      writer.begin_tag( "error" );
      writer.print_attribute( "message", sim -> error_list[ i ] );
      writer.end_tag( "error" );
    }
    writer.end_tag( "errors" );
  }
}

void print_xml_raid_events( sim_t* sim, xml_writer_t & writer )
{
  if ( ! sim -> raid_events_str.empty() )
  {
    writer.begin_tag( "raid_events" );

    std::vector<std::string> raid_event_names;
    int num_raid_events = util::string_split( raid_event_names, sim -> raid_events_str, "/" );

    for ( int i=0; i < num_raid_events; i++ )
    {
      writer.begin_tag( "raid_event" );
      writer.print_attribute( "index", util::to_string( i ) );
      writer.print_attribute( "name", raid_event_names[i] );
      writer.end_tag( "raid_event" );
    }

    writer.end_tag( "raid_events" );
  }
}

void print_xml_roster( sim_t* sim, xml_writer_t & writer )
{
  writer.begin_tag( "players" );

  int num_players = ( int ) sim -> players_by_dps.size();
  for ( int i = 0; i < num_players; ++i )
  {
    player_t * current_player = sim -> players_by_name[ i ];
    print_xml_player( sim, writer, current_player, NULL );
    std::vector<pet_t*>& pl = sim -> players_by_name[ i ] -> pet_list;
    for ( size_t i = 0; i < pl.size(); ++i )
    {
      pet_t* pet = pl[ i ];
      if ( pet -> summoned )
        print_xml_player( sim, writer, pet, current_player );
    }
  }

  writer.end_tag( "players" );
}

void print_xml_targets( sim_t* sim, xml_writer_t & writer )
{
  writer.begin_tag( "targets" );

  size_t count = sim -> targets_by_name.size();
  for ( size_t i = 0; i < count; ++i )
  {
    player_t * current_player = sim -> targets_by_name[ i ];
    print_xml_player( sim, writer, current_player, NULL );
    for ( size_t i = 0; i < current_player -> pet_list.size(); ++i )
    {
      pet_t* pet = current_player -> pet_list[ i ];
      if ( pet -> summoned )
        print_xml_player( sim, writer, pet, current_player );
    }
  }

  writer.end_tag( "targets" );
}

void print_xml_player( sim_t * sim, xml_writer_t & writer, player_t * p, player_t * owner )
{
  report::generate_player_charts( p, p->report_information );

  writer.begin_tag( "player" );
  writer.print_attribute( "name", p -> name() );
  if ( owner )
    writer.print_attribute( "owner", owner -> name() );
  writer.print_tag( "type", p -> is_enemy() ? "Target" : p -> is_add() ? "Add" : "Player" );
  writer.print_tag( "level", util::to_string( p -> level ) );
  writer.print_tag( "race", p -> race_str.c_str() );
  writer.begin_tag( "class" );
  writer.print_attribute( "type", util::player_type_string( p->type ) );
  if ( p -> is_pet() )
    writer.print_attribute( "subtype", util::pet_type_string( p -> cast_pet() -> pet_type ) );
  writer.end_tag( "class" );
  writer.print_tag( "specialization", util::specialization_string( p -> specialization() ) );
  writer.print_tag( "primary_role", util::role_type_string( p -> primary_role() ) );
  writer.print_tag( "position", p -> position_str );
  writer.begin_tag( "dps" );
  writer.print_attribute( "value", util::to_string( p -> dps.mean ) );
  writer.print_attribute( "effective", util::to_string( p -> dpse.mean ) );
  writer.print_attribute( "error", util::to_string( p -> dps_error ) );
  writer.print_attribute( "range", util::to_string( ( p -> dps.max - p -> dps.min ) / 2.0 ) );
  writer.print_attribute( "convergence", util::to_string( p -> dps_convergence ) );
  writer.end_tag( "dps" );

  if ( p -> rps_loss > 0 )
  {
    writer.begin_tag( "dpr" );
    writer.print_attribute( "value", util::to_string( p -> dpr ) );
    writer.print_attribute( "rps_loss", util::to_string( p -> rps_loss ) );
    writer.print_attribute( "rps_gain", util::to_string( p -> rps_gain ) );
    writer.print_attribute( "resource", util::resource_type_string( p -> primary_resource() ) );
    writer.end_tag( "dpr" );
  }

  writer.begin_tag( "waiting_time" );
  writer.print_attribute( "pct", util::to_string( p -> fight_length.mean ? 100.0 * p -> waiting_time.mean / p -> fight_length.mean : 0 ) );
  writer.end_tag( "waiting_time" );
  writer.begin_tag( "active_time" );
  writer.print_attribute( "pct", util::to_string( sim -> simulation_length.mean ? p -> fight_length.mean / sim -> simulation_length.mean * 100.0 : 0 ) );
  writer.end_tag( "active_time" );
  writer.print_tag( "apm", util::to_string( p -> fight_length.mean ? 60.0 * p -> executed_foreground_actions.mean / p -> fight_length.mean : 0 ) );

  if ( ! p -> origin_str.empty() )
    writer.print_tag( "origin", p -> origin_str );

  if ( ! p -> talents_str.empty() )
    writer.print_tag( "talents_url", p -> talents_str );

  print_xml_player_stats( writer, p );
  print_xml_player_actions( writer, p );

  print_xml_player_buffs( writer, p );
  print_xml_player_uptime( writer, p );
  print_xml_player_procs( writer, p );
  print_xml_player_gains( writer, p );
  print_xml_player_scale_factors( writer, p, p->report_information );
  print_xml_player_dps_plots( writer, p );
  print_xml_player_charts( writer, p->report_information );

  writer.end_tag( "player" );
}

void print_xml_player_stats( xml_writer_t & writer, player_t * p )
{
  for ( attribute_e i = ATTRIBUTE_NONE; ++i < ATTRIBUTE_MAX; )
  {
    print_xml_player_attribute( writer, util::attribute_type_string( i ),
                                p -> get_attribute( i ), p -> stats.attribute[ i ],
                                p -> buffed.attribute[ i ] );
  }
  print_xml_player_attribute( writer, "mastery",
                              p -> composite_mastery(), p -> stats.mastery_rating, p -> buffed.mastery );
  print_xml_player_attribute( writer, "spellpower",
                              p -> composite_spell_power( SCHOOL_MAX ) * p -> composite_spell_power_multiplier(), p -> stats.spell_power, p -> buffed.spell_power );
  print_xml_player_attribute( writer, "spellhit",
                              100 * p -> composite_spell_hit(), p -> stats.hit_rating, 100 * p -> buffed.spell_hit );
  print_xml_player_attribute( writer, "spellcrit",
                              100 * p -> composite_spell_crit(), p -> stats.crit_rating, 100 * p -> buffed.spell_crit );
  print_xml_player_attribute( writer, "spellhaste",
                              100 * ( 1 / p -> spell_haste - 1 ), p -> stats.haste_rating, 100 * ( 1 / p -> buffed.spell_haste - 1 ) );
  print_xml_player_attribute( writer, "manareg_per_second",
                              p -> mana_regen_per_second(), 0, p -> buffed.manareg_per_second );
  print_xml_player_attribute( writer, "attackpower",
                              p -> composite_attack_power() * p -> composite_attack_power_multiplier(), p -> stats.attack_power, p -> buffed.attack_power );
  print_xml_player_attribute( writer, "attackhit",
                              100 * p -> composite_attack_hit(), p -> stats.hit_rating, 100 * p -> buffed.attack_hit );
  print_xml_player_attribute( writer, "attackcrit",
                              100 * p -> composite_attack_crit(), p -> stats.crit_rating, 100 * p -> buffed.attack_crit );
  print_xml_player_attribute( writer, "expertise",
                              100 * p -> composite_attack_expertise( &( p -> main_hand_weapon ) ), p -> stats.expertise_rating, 100 * p -> buffed.mh_attack_expertise );
  print_xml_player_attribute( writer, "expertise_oh",
                              100 * p -> composite_attack_expertise( &( p -> off_hand_weapon ) ), p -> stats.expertise_rating, 100 * p -> buffed.oh_attack_expertise );
  print_xml_player_attribute( writer, "attackhaste",
                              100 * ( 1 / p -> composite_attack_haste() - 1 ), p -> stats.haste_rating, 100 * ( 1 / p -> buffed.attack_haste - 1 ) );
  print_xml_player_attribute( writer, "attackspeed",
                              100 * ( 1 / p -> composite_attack_speed() - 1 ), p -> stats.haste_rating, 100 * ( 1 / p -> buffed.attack_speed - 1 ) );
  print_xml_player_attribute( writer, "armor",
                              p -> composite_armor(), ( p -> stats.armor + p -> stats.bonus_armor ), p -> buffed.armor );
  print_xml_player_attribute( writer, "miss",
                              100 * ( p -> composite_tank_miss( SCHOOL_PHYSICAL ) ), 0, 100 * p -> buffed.miss );
  print_xml_player_attribute( writer, "dodge",
                              100 * ( p -> composite_tank_dodge() ), p -> stats.dodge_rating, 100 * p -> buffed.dodge );
  print_xml_player_attribute( writer, "parry",
                              100 * ( p -> composite_tank_parry() ), p -> stats.parry_rating, 100 * p -> buffed.parry );
  print_xml_player_attribute( writer, "block",
                              100 * p -> composite_tank_block(), p -> stats.block_rating, 100 * p -> buffed.block );
  print_xml_player_attribute( writer, "tank_crit",
                              100 * p -> composite_tank_crit( SCHOOL_PHYSICAL ), 0, 100 * p -> buffed.crit );

  writer.begin_tag( "resource" );
  writer.print_attribute( "name", "health" );
  writer.print_attribute( "base", util::to_string( p -> resources.max[ RESOURCE_HEALTH ], 0 ) );
  writer.print_attribute( "buffed", util::to_string( p -> buffed.attribute[ RESOURCE_HEALTH ], 0 ) );
  writer.end_tag( "resource" );

  writer.begin_tag( "resource" );
  writer.print_attribute( "name", "mana" );
  writer.print_attribute( "base", util::to_string( p -> resources.max[ RESOURCE_MANA ], 0 ) );
  writer.print_attribute( "buffed", util::to_string( p -> buffed.resource[ RESOURCE_MANA ], 0 ) );
  writer.end_tag( "resource" );
}

void print_xml_player_attribute( xml_writer_t & writer, const std::string & attribute, double initial, double gear, double buffed )
{
  writer.begin_tag( "attribute" );
  writer.print_attribute( "name", attribute );
  writer.print_attribute( "base", util::to_string( initial, 0 ) );
  writer.print_attribute( "gear", util::to_string( gear, 0 ) );
  writer.print_attribute( "buffed", util::to_string( buffed, 0 ) );
  writer.end_tag( "attribute" );
}

void print_xml_player_actions( xml_writer_t & writer, player_t* p )
{
  writer.begin_tag( "glyphs" );
  std::vector<std::string> glyph_names;
  int num_glyphs = util::string_split( glyph_names, p -> glyphs_str.c_str(), "/" );
  for ( int i=0; i < num_glyphs; i++ )
  {
    writer.begin_tag( "glyph" );
    writer.print_attribute( "name", glyph_names[i] );
    writer.end_tag( "glyph" );
  }
  writer.end_tag( "glyphs" );

  writer.begin_tag( "priorities" );
  std::vector<std::string> action_list;
  int num_actions = util::string_split( action_list, p -> action_list_str, "/" );
  for ( int i=0; i < num_actions; i++ )
  {
    writer.begin_tag( "action" );
    writer.print_attribute( "index", util::to_string( i ) );
    writer.print_attribute( "value", action_list[ i ] );
    writer.end_tag( "action" );
  }
  writer.end_tag( "priorities" );


  writer.begin_tag( "actions" );

  for ( size_t i = 0; i < p -> stats_list.size(); ++i )
  {
    stats_t* s = p -> stats_list[ i ];
    if ( s -> num_executes.mean > 1 || s -> compound_amount > 0 )
    {
      int id = 0;
      for ( size_t i = 0; i < s -> player -> action_list.size(); ++i )
      {
        action_t* a = s -> player -> action_list[ i ];
        if ( a -> stats != s ) continue;
        id = a -> id;
        if ( ! a -> background ) break;
      }

      writer.begin_tag( "action" );
      writer.print_attribute( "id", util::to_string( id ) );
      writer.print_attribute( "name", s -> name_str );
      writer.print_attribute( "count", util::to_string( s -> num_executes.mean ) );
      writer.print_attribute( "frequency", util::to_string( s -> total_intervals.mean ) );
      writer.print_attribute( "dpe", util::to_string( s -> ape, 0 ) );
      writer.print_attribute( "dpe_pct", util::to_string( s -> portion_amount * 100.0 ) );
      writer.print_attribute( "dpet", util::to_string( s -> apet ) );
      writer.print_attribute( "apr", util::to_string( s -> apr[ p -> primary_resource() ] ) );
      writer.print_attribute( "pdps", util::to_string( s -> portion_aps.mean ) );

      if ( ! s -> timeline_aps_chart.empty() )
      {
        writer.begin_tag( "chart" );
        writer.print_attribute( "type", "timeline_aps" );
        writer.print_attribute( "href", s -> timeline_aps_chart );
        writer.end_tag( "chart" );
      }

      writer.print_tag( "etpe", util::to_string( s -> etpe ) );
      writer.print_tag( "ttpt", util::to_string( s -> ttpt ) );
      writer.print_tag( "actual_amount", util::to_string( s -> actual_amount.mean ) );
      writer.print_tag( "total_amount", util::to_string( s -> total_amount.mean ) );
      writer.print_tag( "overkill_pct", util::to_string( s -> overkill_pct ) );
      writer.print_tag( "aps", util::to_string( s -> aps ) );
      writer.print_tag( "apet", util::to_string( s -> apet ) );

      if ( s -> num_direct_results.mean > 0 )
      {
        writer.begin_tag( "direct_results" );
        writer.print_attribute( "count", util::to_string( s -> num_direct_results.mean ) );

        for ( result_e i=RESULT_MAX; --i >= RESULT_NONE; )
        {
          if ( s -> direct_results[ i ].count.mean )
          {
            writer.begin_tag( "result" );
            writer.print_attribute( "type", util::result_type_string( i ) );
            writer.print_attribute( "count", util::to_string( s -> direct_results[ i ].count.mean ) );
            writer.print_attribute( "pct", util::to_string( s -> direct_results[ i ].pct ) );
            writer.print_attribute( "min", util::to_string( s -> direct_results[ i ].actual_amount.min ) );
            writer.print_attribute( "max", util::to_string( s -> direct_results[ i ].actual_amount.max ) );
            writer.print_attribute( "avg", util::to_string( s -> direct_results[ i ].actual_amount.mean ) );
            writer.print_attribute( "avg_min", util::to_string( s -> direct_results[ i ].avg_actual_amount.min ) );
            writer.print_attribute( "avg_max", util::to_string( s -> direct_results[ i ].avg_actual_amount.max ) );
            writer.print_attribute( "actual", util::to_string( s -> direct_results[ i ].fight_actual_amount.mean ) );
            writer.print_attribute( "total", util::to_string( s -> direct_results[ i ].fight_total_amount.mean ) );
            writer.print_attribute( "overkill_pct", util::to_string( s -> direct_results[ i ].overkill_pct.mean ) );
            writer.end_tag( "result" );
          }
        }

        writer.end_tag( "direct_results" );
      }

      if ( s -> num_ticks.mean > 0 )
      {
        writer.begin_tag( "tick_results" );
        writer.print_attribute( "count", util::to_string( s -> num_tick_results.mean ) );
        writer.print_attribute( "ticks", util::to_string( s -> num_ticks.mean ) );

        for ( result_e i = RESULT_MAX; --i >= RESULT_NONE; )
        {
          if ( s -> tick_results[ i ].count.mean )
          {
            writer.begin_tag( "result" );
            writer.print_attribute( "type", util::result_type_string( i ) );
            writer.print_attribute( "count", util::to_string( s -> tick_results[ i ].count.mean ) );
            writer.print_attribute( "pct", util::to_string( s -> tick_results[ i ].pct ) );
            writer.print_attribute( "min", util::to_string( s -> tick_results[ i ].actual_amount.min ) );
            writer.print_attribute( "max", util::to_string( s -> tick_results[ i ].actual_amount.max ) );
            writer.print_attribute( "avg", util::to_string( s -> tick_results[ i ].actual_amount.mean ) );
            writer.print_attribute( "avg_min", util::to_string( s -> tick_results[ i ].avg_actual_amount.min ) );
            writer.print_attribute( "avg_max", util::to_string( s -> tick_results[ i ].avg_actual_amount.max ) );
            writer.print_attribute( "actual", util::to_string( s -> tick_results[ i ].fight_actual_amount.mean ) );
            writer.print_attribute( "total", util::to_string( s -> tick_results[ i ].fight_total_amount.mean ) );
            writer.print_attribute( "overkill_pct", util::to_string( s -> tick_results[ i ].overkill_pct.mean ) );
            writer.end_tag( "result" );
          }
        }

        writer.end_tag( "tick_results" );
      }


      writer.begin_tag( "chart" );
      writer.print_attribute( "type", "timeline_stat_aps" );
      writer.print_attribute_unescaped( "href", s -> timeline_aps_chart );
      writer.end_tag( "chart" );

      // Action Details
      std::vector<std::string> processed_actions;
      size_t size = s -> action_list.size();
      for ( size_t i = 0; i < size; i++ )
      {
        action_t* a = s -> action_list[ i ];

        bool found = false;
        size_t size_processed = processed_actions.size();
        for ( size_t j = 0; j < size_processed && !found; j++ )
          if ( processed_actions[ j ] == a -> name() )
            found = true;
        if ( found ) continue;
        processed_actions.push_back( a -> name() );

        if ( a -> target )
        {
          writer.print_tag( "target", a -> target -> name() );
        }
      }
      writer.end_tag( "action" );
    }
  }

  writer.end_tag( "actions" );

  print_xml_player_action_definitions( writer, p );
}

void print_xml_player_buffs( xml_writer_t & writer, player_t * p )
{
  writer.begin_tag( "buffs" );

  for ( size_t i = 0; i < p -> buff_list.size(); ++i )
  {
    buff_t* b = p -> buff_list[ i ];
    if ( b -> quiet || ! b -> avg_start.sum )
      continue;

    writer.begin_tag( "buff" );
    writer.print_attribute( "name", b -> name_str.c_str() );
    writer.print_attribute( "type", b -> constant ? "constant" : "dynamic" );

    if ( b -> constant )
    {
      writer.print_attribute( "start", util::to_string( b -> avg_start.mean, 1 ) );
      writer.print_attribute( "refresh", util::to_string( b -> avg_refresh.mean, 1 ) );
      writer.print_attribute( "interval", util::to_string( b -> start_intervals.mean, 1 ) );
      writer.print_attribute( "trigger", util::to_string( b -> trigger_intervals.mean, 1 ) );
      writer.print_attribute( "uptime", util::to_string( b -> uptime_pct.mean, 0 ) );

      if ( b -> benefit_pct.mean > 0 && b -> benefit_pct.mean < 100 )
      {
        writer.print_attribute( "benefit", util::to_string( b -> benefit_pct.mean ) );
      }
    }
    writer.end_tag( "buff" );
  }

  writer.end_tag( "buffs" );
}

void print_xml_player_uptime( xml_writer_t & writer, player_t * p )
{
  writer.begin_tag( "benefits" );

  for ( size_t j = 0; j < p -> benefit_list.size(); ++j )
  {
    benefit_t* u = p -> benefit_list[ j ];
    if ( u -> ratio.mean > 0 )
    {
      writer.begin_tag( "benefit" );
      writer.print_attribute( "name", u -> name() );
      writer.print_attribute( "ratio_pct", util::to_string( u -> ratio.mean, 1 ) );
      writer.end_tag( "benefit" );
    }
  }

  writer.end_tag( "benefits" );


  writer.begin_tag( "uptimes" );

  for ( size_t j = 0; j < p -> uptime_list.size(); ++j )
  {
    uptime_t* u = p -> uptime_list[ j ];
    if ( u -> uptime_sum.mean > 0 )
    {
      writer.begin_tag( "uptime" );
      writer.print_attribute( "name", u -> name_str );
      writer.print_attribute( "pct", util::to_string( u -> uptime_sum.mean * 100.0, 1 ) );
      writer.end_tag( "uptime" );
    }
  }

  writer.end_tag( "uptimes" );
}

void print_xml_player_procs( xml_writer_t & writer, player_t * p )
{
  writer.begin_tag( "procs" );

  for ( size_t i = 0; i < p -> proc_list.size(); ++i )
  {
    proc_t* proc = p -> proc_list[ i ];
    if ( proc -> count.mean > 0 )
    {
      writer.begin_tag( "proc" );
      writer.print_attribute( "name", proc -> name() );
      writer.print_attribute( "count", util::to_string( proc -> count.mean, 1 ) );
      writer.print_attribute( "frequency", util::to_string( proc -> interval_sum.mean, 2 ) );
      writer.end_tag( "proc" );
    }
  }

  writer.end_tag( "procs" );
}

void print_xml_player_gains( xml_writer_t & writer, player_t * p )
{
  writer.begin_tag( "gains" );

  for ( size_t i = 0; i < p -> gain_list.size(); ++i )
  {
    gain_t* g = p -> gain_list[ i ];
    for ( size_t i = 0; i < RESOURCE_MAX; i++ )
    {
      if ( g -> actual[ i ] > 0 || g -> overflow[ i ] > 0 )
      {
        writer.begin_tag( "gain" );
        writer.print_attribute( "name", g -> name() );
        writer.print_attribute( "actual", util::to_string( g -> actual[ i ], 1 ) );
        double overflow_pct = 100.0 * g -> overflow[ i ] / ( g -> actual[ i ] + g -> overflow[ i ] );
        if ( overflow_pct > 1.0 )
          writer.print_attribute( "overflow_pct", util::to_string( overflow_pct, 1 ) );
        writer.end_tag( "gain" );
      }
    }
  }

  writer.end_tag( "gains" );
}

void print_xml_player_scale_factors( xml_writer_t & writer, player_t * p, player_t::report_information_t& ri )
{
  if ( ! p -> sim -> scaling -> has_scale_factors() ) return;

  if ( p -> is_add() || p -> is_enemy() ) return;

  if ( p -> sim -> report_precision < 0 )
    p -> sim -> report_precision = 2;

  writer.begin_tag( "scale_factors" );

  gear_stats_t& sf = p -> scaling;
  gear_stats_t& sf_norm = p -> scaling_normalized;

  writer.begin_tag( "weights" );

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( p -> scales_with[ i ] )
    {
      writer.begin_tag( "stat" );
      writer.print_attribute( "name", util::stat_type_abbrev( i ) );
      writer.print_attribute( "value", util::to_string( sf.get_stat( i ), p -> sim -> report_precision ) );
      writer.print_attribute( "normalized", util::to_string( sf_norm.get_stat( i ), p -> sim -> report_precision ) );
      writer.print_attribute( "scaling_error", util::to_string( p -> scaling_error.get_stat( i ), p -> sim -> report_precision ) );
      writer.print_attribute( "delta", util::to_string( p -> sim -> scaling -> stats.get_stat( i ) ) );

      writer.end_tag( "stat" );
    }
  }

  size_t num_scaling_stats = p -> scaling_stats.size();
  for ( size_t i=0; i < num_scaling_stats; i++ )
  {
    writer.begin_tag( "scaling_stat" );
    writer.print_attribute( "name", util::stat_type_abbrev( p -> scaling_stats[ i ] ) );
    writer.print_attribute( "index", util::to_string( ( int64_t )i ) );

    if ( i > 0 )
    {
      if ( ( ( p -> scaling.get_stat( p -> scaling_stats[ i - 1 ] ) - p -> scaling.get_stat( p -> scaling_stats[ i ] ) )
             > sqrt ( p -> scaling_compare_error.get_stat( p -> scaling_stats[ i - 1 ] ) * p -> scaling_compare_error.get_stat( p -> scaling_stats[ i - 1 ] ) / 4 + p -> scaling_compare_error.get_stat( p -> scaling_stats[ i ] ) * p -> scaling_compare_error.get_stat( p -> scaling_stats[ i ] ) / 4 ) * 2 ) )
        writer.print_attribute( "relative_to_previous", ">" );
      else
        writer.print_attribute( "relative_to_previous", "=" );
    }

    writer.end_tag( "scaling_stat" );
  }

  writer.end_tag( "weights" );

  if ( p -> sim -> scaling -> normalize_scale_factors )
  {
    writer.begin_tag( "dps_per_point" );
    writer.print_attribute( "stat", util::stat_type_abbrev( p -> normalize_by() ) );
    writer.print_attribute( "value", util::to_string( p -> scaling.get_stat( p -> normalize_by() ), p -> sim -> report_precision ) );
    writer.end_tag( "dps_per_point" );
  }
  if ( p -> sim -> scaling -> scale_lag )
  {
    writer.begin_tag( "scale_lag_ms" );
    writer.print_attribute( "value", util::to_string( p -> scaling_lag, p -> sim -> report_precision ) );
    writer.print_attribute( "error", util::to_string( p -> scaling_lag_error, p -> sim -> report_precision ) );
    writer.end_tag( "scale_lag_ms" );
  }


  std::string lootrank   = ri.gear_weights_lootrank_link;
  std::string wowhead    = ri.gear_weights_wowhead_link;
  std::string wowreforge = ri.gear_weights_wowreforge_link;
  //std::string pawn_std   = p -> gear_weights_pawn_std_string;
  //std::string pawn_alt   = p -> gear_weights_pawn_alt_string;

  writer.begin_tag( "link" );
  writer.print_attribute( "name", "wowhead" );
  writer.print_attribute( "type", "ranking" );
  writer.print_attribute_unescaped( "href", wowhead );
  writer.end_tag( "link" );

  writer.begin_tag( "link" );
  writer.print_attribute( "name", "lootrank" );
  writer.print_attribute( "type", "ranking" );
  writer.print_attribute( "href", lootrank );
  writer.end_tag( "link" );

  writer.begin_tag( "link" );
  writer.print_attribute( "name", "wowreforge" );
  writer.print_attribute( "type", "optimizer" );
  writer.print_attribute( "href", wowreforge );
  writer.end_tag( "link" );

  writer.end_tag( "scale_factors" );
}

void print_xml_player_dps_plots( xml_writer_t & writer, player_t * p )
{
  sim_t* sim = p -> sim;

  if ( sim -> plot -> dps_plot_stat_str.empty() ) return;

  int range = sim -> plot -> dps_plot_points / 2;

  double min = -range * sim -> plot -> dps_plot_step;
  double max = +range * sim -> plot -> dps_plot_step;

  int points = 1 + range * 2;

  writer.begin_tag( "dps_plot_data" );
  writer.print_attribute( "min", util::to_string( min, 1 ) );
  writer.print_attribute( "max", util::to_string( max, 1 ) );
  writer.print_attribute( "points", util::to_string( points ) );

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    std::vector<plot_data_t>& pd = p -> dps_plot_data[ i ];

    if ( ! pd.empty() )
    {
      writer.begin_tag( "dps" );
      writer.print_attribute( "stat", util::stat_type_abbrev( i ) );
      size_t num_points = pd.size();
      for ( size_t j=0; j < num_points; j++ )
      {
        writer.print_tag( "value", util::to_string( pd[ j ].value, 0 ) );
      }
      writer.end_tag( "dps" );
    }
  }

  writer.end_tag( "dps_plot_data" );
}

void print_xml_player_charts( xml_writer_t & writer, player_t::report_information_t& ri )
{
  writer.begin_tag( "charts" );

  if ( ! ri.action_dpet_chart.empty() )
  {
    writer.begin_tag( "chart" );
    writer.print_attribute( "type", "dpet" );
    writer.print_attribute_unescaped( "href", ri.action_dmg_chart );
    writer.end_tag( "chart" );
  }

  if ( ! ri.action_dmg_chart.empty() )
  {
    writer.begin_tag( "chart" );
    writer.print_attribute( "type", "dmg" );
    writer.print_attribute_unescaped( "href", ri.action_dmg_chart );
    writer.end_tag( "chart" );
  }

  if ( ! ri.scaling_dps_chart.empty() )
  {
    writer.begin_tag( "chart" );
    writer.print_attribute( "type", "scaling_dps" );
    writer.print_attribute_unescaped( "href", ri.scaling_dps_chart );
    writer.end_tag( "chart" );
  }

  if ( ! ri.reforge_dps_chart.empty() )
  {
    writer.begin_tag( "chart" );
    writer.print_attribute( "type", "reforge_dps" );
    writer.print_attribute_unescaped( "href", ri.reforge_dps_chart );
    writer.end_tag( "chart" );
  }

  if ( ! ri.scale_factors_chart.empty() )
  {
    writer.begin_tag( "chart" );
    writer.print_attribute( "type", "scale_factors" );
    writer.print_attribute_unescaped( "href", ri.scale_factors_chart );
    writer.end_tag( "chart" );
  }

  if ( ! ri.timeline_dps_chart.empty() )
  {
    writer.begin_tag( "chart" );
    writer.print_attribute( "type", "timeline_dps" );
    writer.print_attribute_unescaped( "href", ri.timeline_dps_chart );
    writer.end_tag( "chart" );
  }

  if ( ! ri.distribution_dps_chart.empty() )
  {
    writer.begin_tag( "chart" );
    writer.print_attribute( "type", "distribution_dps" );
    writer.print_attribute_unescaped( "href", ri.distribution_dps_chart );
    writer.end_tag( "chart" );
  }

  if ( ! ri.time_spent_chart.empty() )
  {
    writer.begin_tag( "chart" );
    writer.print_attribute( "type", "time_spent" );
    writer.print_attribute_unescaped( "href", ri.time_spent_chart );
    writer.end_tag( "chart" );
  }

  writer.end_tag( "charts" );
}

void print_xml_buffs( sim_t* sim, xml_writer_t & writer )
{
  writer.begin_tag( "buffs" );

  for ( size_t i = 0; i < sim -> buff_list.size(); ++i )
  {
    buff_t* b = sim -> buff_list[ i ];
    if ( b -> quiet || ! b -> avg_start.sum )
      continue;

    writer.begin_tag( "buff" );
    writer.print_attribute( "name", b -> name_str.c_str() );
    writer.print_attribute( "type", b -> constant ? "constant" : "dynamic" );

    if ( b -> constant )
    {
      writer.print_attribute( "start", util::to_string( b -> avg_start.mean, 1 ) );
      writer.print_attribute( "refresh", util::to_string( b -> avg_refresh.mean, 1 ) );
      writer.print_attribute( "interval", util::to_string( b -> start_intervals.mean, 1 ) );
      writer.print_attribute( "trigger", util::to_string( b -> trigger_intervals.mean, 1 ) );
      writer.print_attribute( "uptime", util::to_string( b -> uptime_pct.mean, 0 ) );

      if ( b -> benefit_pct.mean > 0 && b -> benefit_pct.mean < 100 )
      {
        writer.print_attribute( "benefit", util::to_string( b -> benefit_pct.mean ) );
      }

      if ( b -> trigger_pct.mean > 0 && b -> trigger_pct.mean < 100 )
      {
        writer.print_attribute( "trigger_pct", util::to_string( b -> trigger_pct.mean ) );
      }
    }
    writer.end_tag( "buff" );
  }

  writer.end_tag( "buffs" );
}



void print_xml_hat_donors( sim_t* sim, xml_writer_t & writer )
{
  std::vector<player_t*> hat_donors;

  int num_players = ( int ) sim -> players_by_name.size();
  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];
    if ( p -> procs.hat_donor -> count.mean )
      hat_donors.push_back( p );
  }

  int num_donors = ( int ) hat_donors.size();
  if ( num_donors )
  {
    range::sort( hat_donors, report::compare_hat_donor_interval()  );

    writer.begin_tag( "honor_among_thieves" );

    for ( int i=0; i < num_donors; i++ )
    {
      writer.begin_tag( "donors" );
      player_t* p = hat_donors[ i ];
      proc_t* proc = p -> procs.hat_donor;
      writer.print_attribute( "name", p -> name() );
      writer.print_attribute( "frequency_sec", util::to_string( proc -> interval_sum.mean, 2 ) );
      writer.print_attribute( "frequency_pct", util::to_string( ( 1.0 / proc -> interval_sum.mean ), 3 ) );
      writer.end_tag( "donors" );
    }

    writer.end_tag( "honor_among_thieves" );
  }
}

void print_xml_performance( sim_t* sim, xml_writer_t & writer )
{
  writer.begin_tag( "performance" );

  writer.print_tag( "total_events", util::to_string( sim -> total_events_processed ) );
  writer.print_tag( "max_event_queue", util::to_string( sim -> max_events_remaining ) );
  writer.print_tag( "target_health", util::to_string( sim -> target -> resources.base[ RESOURCE_HEALTH ], 0 ) );
  writer.print_tag( "sim_seconds", util::to_string( sim -> iterations * sim -> simulation_length.mean, 0 ) );
  writer.print_tag( "cpu_seconds", util::to_string( sim -> elapsed_cpu.total_seconds(), 3 ) );
  writer.print_tag( "speed_up", util::to_string( sim -> iterations * sim -> simulation_length.mean / sim -> elapsed_cpu.total_seconds(), 0 ) );

  writer.end_tag( "performance" );
}

void print_xml_config( sim_t* sim, xml_writer_t & writer )
{
  writer.begin_tag( "config" );

  writer.print_tag( "style", sim -> hosted_html ? "hosted" : ( sim -> print_styles ? "print" : "not-specified" ) );
  writer.print_tag( "has_scale_factors", sim -> scaling -> has_scale_factors() ? "true" : "false" );
  writer.print_tag( "report_pets_separately", sim -> report_pets_separately ? "true" : "false" );
  writer.print_tag( "report_details", sim -> report_details ? "true" : "false" );
  writer.print_tag( "report_targets", sim -> report_targets ? "true" : "false" );
  writer.print_tag( "report_raw_abilities", sim -> report_raw_abilities ? "true" : "false" );
  writer.print_tag( "normalize_scale_factors", sim -> scaling ->normalize_scale_factors ? "true" : "false" );

  writer.end_tag( "config" );
}

void print_xml_summary( sim_t* sim, xml_writer_t & writer, sim_t::report_information_t& ri )
{
  writer.begin_tag( "summary" );

  time_t rawtime;
  time ( &rawtime );

  writer.print_tag( "timestamp", ctime( &rawtime ) );
  writer.print_tag( "iterations", util::to_string( sim -> iterations ) );

  writer.print_tag( "threads", util::to_string( sim -> threads < 1 ? 1 : sim -> threads ) );

  writer.print_tag( "confidence", util::to_string( sim -> confidence * 100.0 ) );

  writer.begin_tag( "simulation_length" );
  writer.print_attribute( "mean", util::to_string( sim -> simulation_length.mean, 0 ) );
  if ( !sim -> fixed_time )
  {
    writer.print_attribute( "min", util::to_string( sim -> simulation_length.min, 0 ) );
    writer.print_attribute( "max", util::to_string( sim -> simulation_length.max, 0 ) );
  }
  writer.print_attribute( "total", util::to_string( sim -> simulation_length.sum, 0 ) );
  writer.end_tag( "simulation_length" );

  writer.begin_tag( "events" );
  writer.print_attribute( "processed", util::to_string( sim -> total_events_processed ) );
  writer.print_attribute( "max_remaining", util::to_string( sim -> max_events_remaining ) );
  writer.end_tag( "events" );

  writer.print_tag( "fight_style", sim -> fight_style );

  writer.print_tag( "elapsed_cpu_sec", util::to_string( sim -> elapsed_cpu.total_seconds() ) );

  writer.begin_tag( "lag" );
  writer.print_attribute( "type", "world" );
  writer.print_attribute( "value", util::to_string( sim -> world_lag.total_millis() ) );
  writer.print_attribute( "stddev", util::to_string( sim -> world_lag_stddev.total_millis() ) );
  writer.end_tag( "lag" );

  writer.begin_tag( "lag" );
  writer.print_attribute( "type", "queue" );
  writer.print_attribute( "value", util::to_string( sim -> queue_lag.total_millis() ) );
  writer.print_attribute( "stddev", util::to_string( sim -> queue_lag_stddev.total_millis() ) );
  writer.end_tag( "lag" );

  if ( sim -> strict_gcd_queue )
  {
    writer.begin_tag( "lag" );
    writer.print_attribute( "type", "gcd" );
    writer.print_attribute( "value", util::to_string( sim -> gcd_lag.total_millis() ) );
    writer.print_attribute( "stddev", util::to_string( sim -> gcd_lag_stddev.total_millis() ) );
    writer.end_tag( "lag" );

    writer.begin_tag( "lag" );
    writer.print_attribute( "type", "channel" );
    writer.print_attribute( "value", util::to_string( sim -> channel_lag.total_millis() ) );
    writer.print_attribute( "stddev", util::to_string( sim -> channel_lag_stddev.total_millis() ) );
    writer.end_tag( "lag" );

    writer.begin_tag( "lag" );
    writer.print_attribute( "type", "queue_gcd" );
    writer.print_attribute( "value", util::to_string( sim -> queue_gcd_reduction.total_millis() ) );
    writer.end_tag( "lag" );
  }

  size_t count = ri.dps_charts.size();
  writer.begin_tag( "charts" );
  writer.print_attribute( "max_players_per_chart", util::to_string( MAX_PLAYERS_PER_CHART ) );
  for ( size_t i=0; i < count; i++ )
  {
    writer.begin_tag( "chart" );
    writer.print_attribute( "type", "dps" );
    writer.print_attribute_unescaped( "img_src", ri.dps_charts[ i ] );
    writer.end_tag( "chart" );
  }
  count = ri.hps_charts.size();
  for ( size_t i=0; i < count; i++ )
  {
    writer.begin_tag( "chart" );
    writer.print_attribute( "type", "hps" );
    writer.print_attribute_unescaped( "img_src", ri.hps_charts[ i ] );
    writer.end_tag( "chart" );
  }
  count = ri.gear_charts.size();
  for ( size_t i=0; i < count; i++ )
  {
    writer.begin_tag( "chart" );
    writer.print_attribute( "type", "gear" );
    writer.print_attribute_unescaped( "img_src", ri.gear_charts[ i ] );
    writer.end_tag( "chart" );
  }
  count = ri.dpet_charts.size();
  for ( size_t i=0; i < count; i++ )
  {
    writer.begin_tag( "chart" );
    writer.print_attribute( "type", "dpet" );
    writer.print_attribute_unescaped( "img_src", ri.dpet_charts[ i ] );
    writer.end_tag( "chart" );
  }
  writer.end_tag( "charts" );

  writer.begin_tag( "dmg" );
  writer.print_attribute( "total", util::to_string( sim -> total_dmg.mean, 0 ) );
  writer.print_attribute( "dps", util::to_string( sim -> raid_dps.mean, 0 ) );
  writer.end_tag( "dmg" );

  writer.begin_tag( "heal" );
  writer.print_attribute( "total", util::to_string( sim -> total_heal.mean, 0 ) );
  writer.print_attribute( "hps", util::to_string( sim -> raid_hps.mean, 0 ) );
  writer.end_tag( "heal" );

  writer.begin_tag( "player_by_dps" );
  count = sim -> players_by_dps.size();
  for ( size_t i = 0; i < count; i++ )
  {
    player_t* p = sim -> players_by_dps[ i ];
    writer.begin_tag( "player" );
    writer.print_attribute( "name", p -> name() );
    writer.print_attribute( "index", util::to_string( i ) );
    writer.print_attribute( "dps", util::to_string( p -> dps.mean ) );
    writer.end_tag( "player" );
  }
  writer.end_tag( "player_by_dps" );

  writer.begin_tag( "player_by_hps" );
  count = sim -> players_by_dps.size();
  for ( size_t i = 0; i < count; i++ )
  {
    player_t* p = sim -> players_by_dps[ i ];
    writer.begin_tag( "player" );
    writer.print_attribute( "name", p -> name() );
    writer.print_attribute( "index", util::to_string( i ) );
    writer.print_attribute( "hps", util::to_string( p -> hps.mean ) );
    writer.end_tag( "player" );
  }
  writer.end_tag( "player_by_hps" );

  print_xml_raid_events( sim, writer );

  writer.end_tag( "summary" );
}

void print_xml_get_action_list( sim_t* sim, player_t* p, std::map<int, action_t*> & all_actions )
{
  for ( size_t i = 0; i < p -> stats_list.size(); ++i )
  {
    stats_t* s = p -> stats_list[ i ];
    if ( s -> num_executes.mean > 1 || s -> compound_amount > 0 || sim -> debug )
    {
      // Action Details
      std::vector<std::string> processed_actions;
      size_t size = s -> action_list.size();
      for ( size_t i = 0; i < size; i++ )
      {
        action_t* a = s -> action_list[ i ];

        bool found = false;
        size_t size_processed = processed_actions.size();
        for ( size_t j = 0; j < size_processed && !found; j++ )
          if ( processed_actions[ j ] == a -> name() )
            found = true;
        if ( found ) continue;
        processed_actions.push_back( a -> name() );
        if ( all_actions.find( a -> id ) == all_actions.end() ) // Not found.
        {
          all_actions[ a -> id ] = a;
        }
      }
    }
  }
}

void print_xml_player_action_definitions( xml_writer_t & writer, player_t * p )
{
  writer.begin_tag( "action_definitions" );

  for ( size_t i = 0; i < p -> stats_list.size(); ++i )
  {
    stats_t* s = p -> stats_list[ i ];
    if ( s -> num_executes.mean > 1 || s -> compound_amount > 0 )
    {
      // Action Details
      std::vector<std::string> processed_actions;
      size_t size = s -> action_list.size();
      for ( size_t i = 0; i < size; i++ )
      {
        action_t* a = s -> action_list[ i ];

        bool found = false;
        size_t size_processed = processed_actions.size();
        for ( size_t j = 0; j < size_processed && !found; j++ )
          if ( processed_actions[ j ] == a -> name() )
            found = true;
        if ( found ) continue;
        processed_actions.push_back( a -> name() );

        writer.begin_tag( "action_detail" );
        writer.print_attribute( "id", util::to_string( a -> id ) );
        writer.print_attribute( "name", a -> name() );
        writer.print_tag( "school", util::school_type_string( a-> school ) );
        writer.print_tag( "resource", util::resource_type_string( a -> current_resource() ) );
        writer.print_tag( "range", util::to_string( a -> range ) );
        writer.print_tag( "travel_speed", util::to_string( a -> travel_speed ) );
        writer.print_tag( "trigger_gcd", util::to_string( a -> trigger_gcd.total_seconds() ) );
        writer.print_tag( "base_cost", util::to_string( a -> base_costs[ a -> current_resource() ] ) );
        writer.begin_tag( "cooldown" );
        writer.print_attribute( "duration", util::to_string( a -> cooldown -> duration.total_seconds() ) );
        writer.end_tag( "cooldown" );
        writer.print_tag( "base_execute_time", util::to_string( a -> base_execute_time.total_seconds() ) );
        writer.print_tag( "base_crit", util::to_string( a -> base_crit ) );
        if ( a -> target )
        {
          writer.print_tag( "target", a -> target -> name() );
        }
        if ( a -> data().tooltip() )
        {
          writer.print_tag( "tooltip", a -> data().tooltip() );
        }
        if ( a -> data().desc() )
        {
          writer.print_tag( "description", util::encode_html( a -> data().desc() ).c_str() );
        }
        if ( a -> direct_power_mod || a -> base_dd_min || a -> base_dd_max )
        {
          writer.begin_tag( "direct_damage" );
          writer.print_tag( "may_crit", a -> may_crit ? "true" : "false" );
          writer.print_tag( "direct_power_mod", util::to_string( a -> direct_power_mod ) );
          writer.begin_tag( "base" );
          writer.print_attribute( "min", util::to_string( a -> base_dd_min ) );
          writer.print_attribute( "max", util::to_string( a -> base_dd_max ) );
          writer.end_tag( "base" );
          writer.end_tag( "direct_damage" );
        }

        if ( a -> num_ticks )
        {
          writer.begin_tag( "damage_over_time" );
          writer.print_tag( "tick_may_crit", a -> tick_may_crit ? "true" : "false" );
          writer.print_tag( "tick_zero", a -> tick_zero ? "true" : "false" );
          writer.print_tag( "tick_power_mod", util::to_string( a -> tick_power_mod ) );
          writer.print_tag( "base", util::to_string( a -> base_td ) );
          writer.print_tag( "num_ticks", util::to_string( a -> num_ticks ) );
          writer.print_tag( "base_tick_time", util::to_string( a -> base_tick_time.total_seconds() ) );
          writer.print_tag( "hasted_ticks", util::to_string( a -> hasted_ticks ) );
          writer.print_tag( "dot_behavior", util::dot_behavior_type_string( a -> dot_behavior ) );
          writer.end_tag( "damage_over_time" );
        }
        // Extra Reporting for DKs
        if ( a -> player -> type == DEATH_KNIGHT )
        {
          writer.begin_tag( "runes" );
          writer.print_tag( "blood", util::to_string( a -> data().rune_cost() & 0x1 ) );
          writer.print_tag( "frost", util::to_string( ( a -> data().rune_cost() >> 4 ) & 0x1 ) );
          writer.print_tag( "unholy", util::to_string( ( a -> data().rune_cost() >> 2 ) & 0x1 ) );
          writer.print_tag( "runic_power_gain", util::to_string( a -> rp_gain ) );
          writer.end_tag( "runes" );
        }
        if ( a -> weapon )
        {
          writer.begin_tag( "weapon" );
          writer.print_tag( "normalize_speed", a -> normalize_weapon_speed ? "true" : "false" );
          writer.print_tag( "power_mod", util::to_string( a -> weapon_power_mod ) );
          writer.print_tag( "multiplier", util::to_string( a -> weapon_multiplier ) );
          writer.end_tag( "weapon" );
        }

        writer.end_tag( "action_detail" );
      }
    }
  }

  writer.end_tag( "action_definitions" );
}

} // UNNAMED NAMESPACE ====================================================


namespace report {

void print_xml( sim_t* sim )
{
  int num_players = ( int ) sim -> players_by_name.size();

  if ( num_players == 0 ) return;
  if ( sim -> simulation_length.mean == 0 ) return;
  if ( sim -> xml_file_str.empty() ) return;

  xml_writer_t writer( sim -> xml_file_str.c_str() );
  if ( !writer.ready() )
  {
    sim -> errorf( "Unable to open xml file '%s'\n", sim -> xml_file_str.c_str() );
    return;
  }

  report::generate_sim_report_information( sim, sim->report_information );

  writer.init_document( sim -> xml_stylesheet_file_str );
  writer.begin_tag( "simulationcraft" );

  writer.print_attribute( "major_version", SC_MAJOR_VERSION );
  writer.print_attribute( "minor_version", SC_MINOR_VERSION );
  writer.print_attribute( "wow_version", dbc_t::wow_version( sim -> dbc.ptr ) );
  writer.print_attribute( "ptr", sim -> dbc.ptr ? "true" : "false" );
  writer.print_attribute( "wow_build", util::to_string( dbc_t::build_level( sim -> dbc.ptr ) ) );

#if SC_BETA

  writer.print_attribute( "beta", "true" );

#endif

  print_xml_config( sim, writer );
  print_xml_summary( sim, writer, sim->report_information );

  print_xml_raid_events( sim, writer );
  print_xml_roster( sim, writer );
  print_xml_targets( sim, writer );

  print_xml_buffs( sim, writer );
  print_xml_hat_donors( sim, writer );
  print_xml_performance( sim, writer );

  print_xml_errors( sim, writer );

  writer.end_tag( "simulationcraft" );
}

} // END report NAMESPACE

