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
action_priority_t* action_priority_list_t::add_action(util::string_view action_priority_str,
  util::string_view comment)
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
  util::string_view action_name,
  util::string_view action_options, util::string_view comment)
{
  if (!s || !s->ok() || !s->is_level(p->true_level))
    return nullptr;

  if (action_options.empty())
    return add_action(action_name, comment);

  return add_action(fmt::format("{},{}", action_name, action_options), comment);
}

/**
 * @brief add to action list & check class spell with given name
 *
 * Check the availability of a class spell of "name" and the validity of it's
 * spell data before anything goes to action priority list
 */
action_priority_t* action_priority_list_t::add_action(const player_t* p, util::string_view name,
  util::string_view action_options, util::string_view comment)
{
  const spell_data_t* s = p->find_class_spell(name);
  if (s == spell_data_t::not_found())
    s = p->find_specialization_spell(name);
  return add_action(p, s, util::tokenize_fn( s->name_cstr() ), action_options, comment);
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
action_priority_t* action_priority_list_t::add_talent(const player_t* p, util::string_view name,
  util::string_view action_options, util::string_view comment)
{
  const spell_data_t* s = p->find_talent_spell(name, SPEC_NONE, false, false);
  return add_action(p, s, util::tokenize_fn( s->name_cstr() ), action_options, comment);
}
