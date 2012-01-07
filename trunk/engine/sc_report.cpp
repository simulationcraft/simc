// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

#if SC_BETA
// beta warning messages
static const char* beta_warnings[] =
{
  "Beta! Beta! Beta! Beta! Beta! Beta!",
  "All classes are supported.",
  "Some class models still need tweaking.",
  "Some class action lists need tweaking.",
  "Some class BiS gear setups need tweaking.",
  "Some trinkets not yet implemented.",
  "Constructive feedback regarding our output will shorten the Beta phase dramatically.",
  "Beta! Beta! Beta! Beta! Beta! Beta!",
  0
};
#endif

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Report
// ==========================================================================

// report_t::encode_html ====================================================

void report_t::encode_html( std::string& buffer )
{
  util_t::replace_all( buffer, '&', "&amp;" );
  util_t::replace_all( buffer, '<', "&lt;" );
  util_t::replace_all( buffer, '>', "&gt;" );
}

std::string report_t::encode_html( const char* str )
{
  std::string nstr;

  if ( str )
  {
    nstr = str;
    encode_html( nstr );
  }

  return nstr;
}

// report_t::print_profiles =================================================

void report_t::print_profiles( sim_t* sim )
{
  int k = 0;
  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    if ( p -> is_pet() ) continue;

    k++;
    FILE* file = NULL;

    if ( !p -> save_gear_str.empty() ) // Save gear
    {
      file = fopen( p -> save_gear_str.c_str(), "w" );
      if ( ! file )
      {
        sim -> errorf( "Unable to save gear profile %s for player %s\n", p -> save_gear_str.c_str(), p -> name() );
      }
      else
      {
        std::string profile_str = "";
        p -> create_profile( profile_str, SAVE_GEAR );
        fprintf( file, "%s", profile_str.c_str() );
        fclose( file );
      }
    }

    if ( !p -> save_talents_str.empty() ) // Save talents
    {
      file = fopen( p -> save_talents_str.c_str(), "w" );
      if ( ! file )
      {
        sim -> errorf( "Unable to save talents profile %s for player %s\n", p -> save_talents_str.c_str(), p -> name() );
      }
      else
      {
        std::string profile_str = "";
        p -> create_profile( profile_str, SAVE_TALENTS );
        fprintf( file, "%s", profile_str.c_str() );
        fclose( file );
      }
    }

    if ( !p -> save_actions_str.empty() ) // Save actions
    {
      file = fopen( p -> save_actions_str.c_str(), "w" );
      if ( ! file )
      {
        sim -> errorf( "Unable to save actions profile %s for player %s\n", p -> save_actions_str.c_str(), p -> name() );
      }
      else
      {
        std::string profile_str = "";
        p -> create_profile( profile_str, SAVE_ACTIONS );
        fprintf( file, "%s", profile_str.c_str() );
        fclose( file );
      }
    }

    std::string file_name = p -> save_str;

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
      util_t::urlencode( util_t::format_text( file_name, sim -> input_is_utf8 ) );
    }

    if ( file_name.empty() ) continue;

    file = fopen( file_name.c_str(), "w" );
    if ( ! file )
    {
      sim -> errorf( "Unable to save profile %s for player %s\n", file_name.c_str(), p -> name() );
      continue;
    }

    std::string profile_str = "";
    p -> create_profile( profile_str );
    fprintf( file, "%s", profile_str.c_str() );
    fclose( file );
  }

  // Save overview file for Guild downloads
  //if ( /* guild parse */ )
  if ( sim -> save_raid_summary )
  {
    FILE* file = NULL;

    std::string filename = "Raid_Summary.simc";
    std::string player_str = "#Raid Summary\n";
    player_str += "# Contains ";
    player_str += util_t::to_string( k );
    player_str += " Players.\n\n";

    for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[ i ];
      if ( p -> is_pet() ) continue;

      std::string file_name = p -> save_str;
      std::string profile_name;

      if ( file_name.empty() && sim -> save_profiles )
      {
        file_name  = "# Player: ";
        file_name += p -> name_str;
        file_name += " Spec: ";
        file_name += p -> primary_tree_name();
        file_name += " Role: ";
        file_name += util_t::role_type_string( p -> primary_role() );
        file_name += "\n";
        profile_name += sim -> save_prefix_str;
        profile_name += p -> name_str;
        if ( sim -> save_talent_str != 0 )
        {
          profile_name += "_";
          profile_name += p -> primary_tree_name();
        }
        profile_name += sim -> save_suffix_str;
        profile_name += ".simc";
        util_t::urlencode( util_t::format_text( profile_name, sim -> input_is_utf8 ) );
        file_name += profile_name;
        file_name += "\n\n";
      }
      player_str += file_name;
    }


    file = fopen( filename.c_str(), "w" );
    if ( ! file )
    {
      sim -> errorf( "Unable to save overview profile %s\n", filename.c_str() );
    }
    else
    {
      fprintf( file, "%s", player_str.c_str() );
      fclose( file );
    }
  }
}

// report_t::print_spell_query ==============================================

void report_t::print_spell_query( sim_t* sim )
{
  spell_data_expr_t* sq = sim -> spell_query;
  assert( sq );

  for ( std::vector<uint32_t>::const_iterator i = sq -> result_spell_list.begin(); i != sq -> result_spell_list.end(); i++ )
  {
    if ( sq -> data_type == DATA_TALENT )
    {
      util_t::fprintf( sim -> output_file, "%s", spell_info_t::talent_to_str( sim, sim -> dbc.talent( *i ) ).c_str() );
    }
    else if ( sq -> data_type == DATA_EFFECT )
    {
      std::ostringstream sqs;
      const spell_data_t* spell = sim -> dbc.spell( sim -> dbc.effect( *i ) -> spell_id() );
      if ( spell )
      {
        spell_info_t::effect_to_str( sim,
                                     spell,
                                     sim -> dbc.effect( *i ),
                                     sqs );
      }
      util_t::fprintf( sim -> output_file, "%s", sqs.str().c_str() );
    }
    else
    {
      const spell_data_t* spell = sim -> dbc.spell( *i );
      util_t::fprintf( sim -> output_file, "%s", spell_info_t::to_str( sim, spell ).c_str() );
    }
  }
}

// report_t::print_suite ====================================================

void report_t::print_suite( sim_t* sim )
{
  report_t::print_text( sim -> output_file, sim, sim -> report_details != 0 );
  report_t::print_html( sim );
  report_t::print_xml( sim );
  report_t::print_profiles( sim );
}
