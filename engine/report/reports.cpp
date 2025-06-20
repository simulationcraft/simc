// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "reports.hpp"

#include "dbc/dbc.hpp"
#include "dbc/sc_spell_info.hpp"
#include "dbc/spell_query/spell_data_expr.hpp"
#include "player/player.hpp"
#include "report/report_helper.hpp"
#include "sim/sim.hpp"
#include "util/xml.hpp"

#include <ostream>
#include <sstream>

// report::print_profiles ===================================================
namespace report
{


void print_profiles(sim_t* sim)
{
  if ( sim->save_profile_pre_init )
    fmt::print( "Profiles generated with the save_profile_pre_init option may exhibit strange behaviour!\n" );

  int k = 0;
  for ( unsigned int i = 0; i < sim->actor_list.size(); i++ )
  {
    player_t* p = sim->actor_list[ i ];
    if ( p->is_pet() )
      continue;

    if ( !report_helper::check_gear( *p, *sim ) )
      continue;

    k++;

    auto profile_writer = [ & ]( std::string filename, std::string type_str, save_e save_type ) {
      if ( filename.empty() )
        return;

      io::cfile file( filename, "w" );
      if ( file )
      {
        std::string contents = p->create_profile( save_type );
        if ( sim->save_profile_pre_init )
          contents.insert(
              0,
              "# Warning! Profiles generated with the save_profile_pre_init option may exhibit strange behaviour!\n" );
        fprintf( file, "%s", contents.c_str() );
        return;
      }

      sim->error( "Unable to save {} profile {} for player {}s\n", type_str, filename, p->name() );
    };

    profile_writer( p->report_information.save_gear_str, "gear", SAVE_GEAR );
    profile_writer( p->report_information.save_talents_str, "talents", SAVE_TALENTS );
    profile_writer( p->report_information.save_actions_str, "actions", SAVE_ACTIONS );

    std::string file_name = p->report_information.save_str;

    if ( file_name.empty() && sim->save_profiles && sim->save_full_profile )
    {
      file_name = sim->save_prefix_str;
      file_name += p->name_str;

      if ( sim->save_talent_str )
      {
        file_name += "_";
        file_name += p->primary_tree_name();
      }

      file_name += sim->save_suffix_str;
      file_name += ".simc";
    }

    unsigned save_type = SAVE_ALL;
    if ( !sim->save_profile_with_actions )
      save_type &= ~( SAVE_ACTIONS );

    profile_writer( file_name, "", static_cast<save_e>( save_type ) );
  }

  // Save overview file for Guild downloads
  // if ( /* guild parse */ )
  if (sim->save_raid_summary)
  {
    static const char* const filename = "Raid_Summary.simc";
    io::cfile file(filename, "w");
    if (!file)
    {
      sim->errorf("Unable to save overview profile %s\n", filename);
    }
    else
    {
      fprintf(file,
        "#Raid Summary\n"
        "# Contains %d Players.\n\n",
        k);

      for (unsigned int i = 0; i < sim->actor_list.size(); ++i)
      {
        player_t* p = sim->actor_list[i];
        if (p->is_pet())
          continue;

        if (!p->report_information.save_str.empty())
          fprintf(file, "%s\n", p->report_information.save_str.c_str());
        else if (sim->save_profiles)
        {
          fprintf(file,
            "# Player: %s Spec: %s Role: %s\n"
            "%s%s",
            p->name(), p->primary_tree_name(), util::role_type_string(p->primary_role()),
            sim->save_prefix_str.c_str(), p->name());

          if (sim->save_talent_str)
            fprintf(file, "-%s", p->primary_tree_name());

          fprintf(file, "%s.simc\n\n", sim->save_suffix_str.c_str());
        }
      }
    }
  }
}

// report::print_spell_query ================================================

void print_spell_query( std::ostream& out, const sim_t& sim, const spell_data_expr_t& sq, unsigned level )
{
  expr_data_e data_type = sq.data_type;
  for ( unsigned int id : sq.result_spell_list )
  {
    switch ( data_type )
    {
      case DATA_TALENT:
        out << spell_info::talent_to_str( *sim.dbc, trait_data_t::find( id, sim.dbc->ptr ), level );
        break;
      case DATA_EFFECT:
      {
        std::ostringstream sqs;
        const spelleffect_data_t* base_effect = sim.dbc->effect( id );
        if ( const spell_data_t* spell = dbc::find_spell( &( sim ), base_effect->spell() ) )
        {
          const auto spell_effects = spell->effects();
          auto effect = range::find( spell_effects, base_effect->id(), &spelleffect_data_t::id );
          if ( effect != spell_effects.end() )
          {
            spell_info::effect_to_str( *sim.dbc, spell, &( *effect ), sqs, level, sim.spell_query_wrap );
            out << sqs.str();
          }
        }
      }
      break;
      default:
      {
        const spell_data_t* spell = dbc::find_spell( &( sim ), sim.dbc->spell( id ) );
        out << spell_info::to_str( *sim.dbc, spell, level, sim.spell_query_wrap );
      }
    }
  }
}

void print_spell_query( xml_node_t* out, FILE* file, const sim_t& sim, const spell_data_expr_t& sq, unsigned level )
{
  expr_data_e data_type = sq.data_type;
  for ( unsigned int id : sq.result_spell_list )
  {
    switch (data_type)
    {
    case DATA_TALENT:
      spell_info::talent_to_xml( *sim.dbc, trait_data_t::find( id, sim.dbc->ptr ), out, level );
      break;
    case DATA_EFFECT:
    {
      std::ostringstream sqs;
      const spelleffect_data_t* dbc_effect = sim.dbc->effect(id);
      if ( const spell_data_t* spell = dbc::find_spell( &(sim), dbc_effect->spell() ) )
      {
        const auto spell_effects = spell->effects();
        auto effect = range::find( spell_effects, dbc_effect->id(), &spelleffect_data_t::id );
        if ( effect != spell_effects.end() )
          spell_info::effect_to_xml( *sim.dbc, spell, &( *effect ), out, level );
      }
    }
    break;
    default:
    {
      const spell_data_t* spell = dbc::find_spell(&(sim), sim.dbc->spell(id));
      spell_info::to_xml( *sim.dbc, spell, out, level );
    }
    }
  }

  fmt::print(file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  out->print_xml( file );
}
// report::print_suite ======================================================

void print_suite( sim_t* sim )
{
  if (!sim->profileset_enabled)
  {
    fmt::print( "\nGenerating reports...\n" );
  }

  report::print_text(sim, sim->report_details != 0);
  report::print_json(*sim);
  report::print_html(*sim);
  report::print_profiles(sim);
}
}  // namespace report
