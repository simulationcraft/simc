// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "reports.hpp"

#include "dbc/dbc.hpp"
#include "dbc/sc_spell_info.hpp"
#include "dbc/spell_query/spell_data_expr.hpp"
#include "player/sc_player.hpp"
#include "report/report_helper.hpp"
#include "sim/sc_sim.hpp"
#include "util/xml.hpp"

namespace
{
struct report_version_settings_t
{
  int minimum_version;
  int minimum_non_deprecated_version;
  int maximum_version;
};

report_version_settings_t get_report_settings( report::report_type type )
{
  switch ( type )
  {
    case report::report_type::HTML:
      return report_version_settings_t{1, 1, 1};
    case report::report_type::JSON:
      return report_version_settings_t{2, 2, 3};
    case report::report_type::TEXT:
      return report_version_settings_t{1, 1, 1};
  }
  throw std::invalid_argument( "Unknown report type" );
}

void print_report( sim_t& sim, const report::report_configuration_t& entry )
{
  switch ( entry.report_type() )
  {
    case report::report_type::HTML:
      report::print_html( sim );
      break;
    case report::report_type::JSON:
      report::print_json( sim, entry );
      break;
    case report::report_type::TEXT:
      report::print_text( &sim, sim.report_details );
      break;
    default:
      throw std::invalid_argument( "Unknown report type" );
  }
}
}  // namespace

// report::print_profiles ===================================================
namespace report
{
report_configuration_t::report_configuration_t( enum report_type type, int version, std::string destination )
  : _type( type ), _version( version ), _destination( std::move( destination ) )
{
}

bool report_configuration_t::is_version_greater_than( int min_version ) const
{
  return _version > min_version;
}

bool report_configuration_t::is_version_between( int min_version, int max_version ) const
{
  return _version > min_version && _version < max_version;
}

bool report_configuration_t::is_version_less_than( int max_version ) const
{
  return _version < max_version;
}

int report_configuration_t::version() const
{
  return _version;
}

std::string report_configuration_t::report_type_string() const
{
  return get_report_type_string( _type );
}

report_type report_configuration_t::report_type() const
{
  return _type;
}

std::string report_configuration_t::destination() const
{
  return _destination;
}

std::string get_report_type_string( report_type type )
{
  switch ( type )
  {
    case report_type::HTML:
      return "html";
    case report_type::JSON:
      return "json";
    case report_type::TEXT:
      return "text";
  }
  throw std::invalid_argument( "Unknown report type" );
}

report_configuration_t create_report_entry( sim_t& sim, report_type type, int version, std::string destination )
{
  auto settings = get_report_settings( type );

  if ( version < settings.minimum_version )
  {
    throw std::invalid_argument(
        fmt::format( "Cannot generate report {} with version {}, which is less than the supported minimum version {}",
                     get_report_type_string( type ), version, settings.minimum_version ) );
  }
  if ( version < settings.minimum_non_deprecated_version )
  {
    sim.error( "Report {} with version {} is deprecated. Minimum non-deprecated version is {}.",
               get_report_type_string( type ), version, settings.minimum_non_deprecated_version );
  }
  if ( version > settings.maximum_version )
  {
    throw std::invalid_argument(
        fmt::format( "Cannot generate report {} with version {}, which greater than the supported maximum version {}",
                     get_report_type_string( type ), version, settings.maximum_version ) );
  }

  return report_configuration_t( type, version, destination );
}

void print_profiles(sim_t* sim)
{
  int k = 0;
  for (unsigned int i = 0; i < sim->actor_list.size(); i++)
  {
    player_t* p = sim->actor_list[i];
    if (p->is_pet())
      continue;

    if (!report_helper::check_gear(*p, *sim))
    {
      continue;
    }
    k++;

    if (!p->report_information.save_gear_str.empty())  // Save gear
    {
      io::cfile file(p->report_information.save_gear_str, "w");
      if (!file)
      {
        sim->errorf("Unable to save gear profile %s for player %s\n", p->report_information.save_gear_str.c_str(),
          p->name());
      }
      else
      {
        std::string profile_str = p->create_profile(SAVE_GEAR);
        fprintf(file, "%s", profile_str.c_str());
      }
    }

    if (!p->report_information.save_talents_str.empty())  // Save talents
    {
      io::cfile file(p->report_information.save_talents_str, "w");
      if (!file)
      {
        sim->errorf("Unable to save talents profile %s for player %s\n",
          p->report_information.save_talents_str.c_str(), p->name());
      }
      else
      {
        std::string profile_str = p->create_profile(SAVE_TALENTS);
        fprintf(file, "%s", profile_str.c_str());
      }
    }

    if (!p->report_information.save_actions_str.empty())  // Save actions
    {
      io::cfile file(p->report_information.save_actions_str, "w");
      if (!file)
      {
        sim->errorf("Unable to save actions profile %s for player %s\n",
          p->report_information.save_actions_str.c_str(), p->name());
      }
      else
      {
        std::string profile_str = p->create_profile(SAVE_ACTIONS);
        fprintf(file, "%s", profile_str.c_str());
      }
    }

    std::string file_name = p->report_information.save_str;

    if (file_name.empty() && sim->save_profiles)
    {
      file_name = sim->save_prefix_str;
      file_name += p->name_str;
      if (sim->save_talent_str != 0)
      {
        file_name += "_";
        file_name += p->primary_tree_name();
      }
      file_name += sim->save_suffix_str;
      file_name += ".simc";
    }

    if (file_name.empty())
      continue;

    io::cfile file(file_name, "w");
    if (!file)
    {
      sim->errorf("Unable to save profile %s for player %s\n", file_name.c_str(), p->name());
      continue;
    }

    unsigned save_type = SAVE_ALL;
    if (!sim->save_profile_with_actions)
    {
      save_type &= ~(SAVE_ACTIONS);
    }
    std::string profile_str = p->create_profile(static_cast<save_e>(save_type));
    fprintf(file, "%s", profile_str.c_str());
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

          if (sim->save_talent_str != 0)
            fprintf(file, "-%s", p->primary_tree_name());

          fprintf(file, "%s.simc\n\n", sim->save_suffix_str.c_str());
        }
      }
    }
  }
}

// report::print_spell_query ================================================

void print_spell_query(std::ostream& out, const sim_t& sim, const spell_data_expr_t& sq, unsigned level)
{
  expr_data_e data_type = sq.data_type;
  for (auto i = sq.result_spell_list.begin(); i != sq.result_spell_list.end(); ++i)
  {
    switch (data_type)
    {
    case DATA_TALENT:
      out << spell_info::talent_to_str(*sim.dbc, sim.dbc->talent(*i), level);
      break;
    case DATA_EFFECT:
    {
      std::ostringstream sqs;
      const spelleffect_data_t* base_effect = sim.dbc->effect(*i);
      if (const spell_data_t* spell = dbc::find_spell(&(sim), base_effect->spell()))
      {
        spell_info::effect_to_str(*sim.dbc, spell, dbc::find_effect(&(sim), base_effect), sqs, level);
        out << sqs.str();
      }
    }
    break;
    default:
    {
      const spell_data_t* spell = dbc::find_spell(&(sim), sim.dbc->spell(*i));
      out << spell_info::to_str(*sim.dbc, spell, level);
    }
    }
  }
}

void print_spell_query(xml_node_t* root, FILE* file, const sim_t& sim, const spell_data_expr_t& sq,
  unsigned level)
{
  expr_data_e data_type = sq.data_type;
  for (auto i = sq.result_spell_list.begin(); i != sq.result_spell_list.end(); ++i)
  {
    switch (data_type)
    {
    case DATA_TALENT:
      spell_info::talent_to_xml(*sim.dbc, sim.dbc->talent(*i), root, level);
      break;
    case DATA_EFFECT:
    {
      std::ostringstream sqs;
      const spelleffect_data_t* dbc_effect = sim.dbc->effect(*i);
      if (const spell_data_t* spell = dbc::find_spell(&(sim), dbc_effect->spell()))
      {
        spell_info::effect_to_xml(*sim.dbc, spell, dbc::find_effect(&(sim), dbc_effect), root, level);
      }
    }
    break;
    default:
    {
      const spell_data_t* spell = dbc::find_spell(&(sim), sim.dbc->spell(*i));
      spell_info::to_xml(*sim.dbc, spell, root, level);
    }
    }
  }

  util::fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  root->print_xml(file);
}
// report::print_suite ======================================================

void print_suite(sim_t* sim, const std::vector<report_configuration_t>& report_entries)
{
  if (!sim->profileset_enabled)
  {
    std::cout << "\nGenerating reports...\n";
  }

  report::print_text(sim, sim->report_details != 0);

  report::print_html(*sim);
  report::print_profiles(sim);

  for(const auto& entry : report_entries)
  {
    print_report(*sim, entry);
  }
}
}  // namespace report