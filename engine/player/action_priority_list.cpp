// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "action_priority_list.hpp"
#include "util/util.hpp"
#include "action/sc_action.hpp"
#include "player/sc_player.hpp"
#include "dbc/dbc.hpp"


/**
 * @brief add to action list without restriction
 *
 * Anything goes to action priority list.
 */
action_priority_t* action_priority_list_t::add_action(const std::string& action_priority_str,
  const std::string& comment)
{
  if (action_priority_str.empty())
    return nullptr;
  action_list.emplace_back(action_priority_str, comment);
  return &(action_list.back());
}

/**
 * @brief add to action list & check spelldata
 *
 * Check the validity of spell data before anything goes to action priority list
 */
action_priority_t* action_priority_list_t::add_action(const player_t* p, const spell_data_t* s,
  const std::string& action_name,
  const std::string& action_options, const std::string& comment)
{
  if (!s || !s->ok() || !s->is_level(p->true_level))
    return nullptr;

  std::string str = action_name;
  if (!action_options.empty())
    str += "," + action_options;

  return add_action(str, comment);
}

/**
 * @brief add to action list & check class spell with given name
 *
 * Check the availability of a class spell of "name" and the validity of it's
 * spell data before anything goes to action priority list
 */
action_priority_t* action_priority_list_t::add_action(const player_t* p, const std::string& name,
  const std::string& action_options, const std::string& comment)
{
  const spell_data_t* s = p->find_class_spell(name);
  if (s == spell_data_t::not_found())
    s = p->find_specialization_spell(name);
  std::string tokenized_name = s->name_cstr();
  util::tokenize(tokenized_name);
  return add_action(p, s, tokenized_name, action_options, comment);
}

/**
 * @brief add talent action to action list & check talent availability
 *
 * Check the availability of a talent spell of "name" and the validity of it's
 * spell data before anything goes to action priority list. Note that this will
 * ignore the actual talent check so we can make action list entries for
 * talents, even though the profile does not have a talent.
 *
 * In addition, the method automatically checks for the presence of an if
 * expression that includes the "talent guard", i.e., "talent.X.enabled" in it.
 * If omitted, it will be automatically added to the if expression (or
 * if expression will be created if it is missing).
 */
action_priority_t* action_priority_list_t::add_talent(const player_t* p, const std::string& name,
  const std::string& action_options, const std::string& comment)
{
  const spell_data_t* s = p->find_talent_spell(name, SPEC_NONE, false, false);
  std::string tokenized_name = s->name_cstr();
  util::tokenize(tokenized_name);
  return add_action(p, s, tokenized_name, action_options, comment);
}
