// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_enums.hpp"
#include "util/timespan.hpp"
#include "util/format.hpp"
#include "dbc/spell_data.hpp"

#include <string>
#include <vector>
#include <functional>

struct absorb_buff_t;
struct action_t;
struct attack_t;
struct buff_t;
class dbc_t;
struct heal_t;
struct item_t;
struct item_enchantment_data_t;
struct player_t;
struct scoped_callback_t;
struct spell_t;
struct spell_data_t;
struct special_effect_t;
struct stat_buff_t;

namespace special_effect
{
void parse_special_effect_encoding( special_effect_t& effect, const std::string& encoding );
bool usable_proc( const special_effect_t& effect );
}

struct special_effect_t
{
  const item_t* item;
  player_t* player;

  special_effect_e type;
  special_effect_source_e source;
  std::string name_str, trigger_str, encoding_str;
  unsigned proc_flags_; /* Proc-by */
  unsigned proc_flags2_; /* Proc-on (hit/damage/...) */
  stat_e stat;
  school_e school;
  int max_stacks;
  double stat_amount, discharge_amount, discharge_scaling;
  double proc_chance_;
  double ppm_;
  unsigned rppm_scale_;
  double rppm_modifier_;
  int rppm_blp_;
  timespan_t duration_, cooldown_, tick;
  bool cost_reduction;
  int refresh;
  bool chance_to_discharge;
  unsigned int override_result_es_mask;
  unsigned result_es_mask;
  bool reverse;
  int reverse_stack_reduction;
  int aoe;
  bool proc_delay;
  bool unique;
  bool weapon_proc;
  /// Expire procced buff on max stack, 0 = never, 1 = always, -1 = autodetect (default)
  int expire_on_max_stack;
  unsigned spell_id, trigger_spell_id;
  action_t* execute_action; // Allows custom action to be executed on use
  buff_t* custom_buff; // Allows custom action

  bool action_disabled, buff_disabled;

  // Old-style function-based custom second phase initializer (callback)
  std::function<void(special_effect_t&)> custom_init;
  // New-style object-based custom second phase initializer
  std::vector<scoped_callback_t*> custom_init_object;
  // Activation callback, if set, called when an actor becomes active
  std::function<void(void)> activation_cb;
  // Link to an SpellItemEnchantment entry, set for various "enchant"-based special effects
  // (Enchants, Gems, Addons)
  const item_enchantment_data_t* enchant_data;

  special_effect_t( player_t* p ) :
    item( nullptr ), player( p ),
    name_str()
  { reset(); }

  special_effect_t( const item_t* item );

  // Uses a custom initialization callback or object
  bool is_custom() const
  { return custom_init || custom_init_object.size() > 0; }

  // Forcefully disable creation of an (autodetected) buff or action. This is necessary in scenarios
  // where the autodetection decides to create an invalid action or buff due to the spell data.
  void disable_action()
  { action_disabled = true; }
  void disable_buff()
  { buff_disabled = true; }

  void reset();

  bool active() { return stat != STAT_NONE || school != SCHOOL_NONE || execute_action; }

  const spell_data_t* driver() const;
  const spell_data_t* trigger() const;
  std::string name() const;
  std::string cooldown_name() const;

  // On-use item cooldown handling accessories
  int cooldown_group() const;
  std::string cooldown_group_name() const;
  timespan_t cooldown_group_duration() const;

  // Buff related functionality
  buff_t* create_buff() const;
  special_effect_buff_e buff_type() const;
  int max_stack() const;

  stat_e stat_buff_type( const spelleffect_data_t& ) const;
  bool is_stat_buff() const;
  stat_e stat_type() const;
  stat_buff_t* initialize_stat_buff() const;

  bool is_absorb_buff() const;
  absorb_buff_t* initialize_absorb_buff() const;

  // Action related functionality
  action_t* create_action() const;
  special_effect_action_e action_type() const;
  bool is_offensive_spell_action() const;
  spell_t* initialize_offensive_spell_action() const;
  bool is_heal_action() const;
  heal_t* initialize_heal_action() const;
  bool is_attack_action() const;
  attack_t* initialize_attack_action() const;
  bool is_resource_action() const;
  spell_t* initialize_resource_action() const;

  /* Accessors for driver specific features of the proc; some are also used for on-use effects */
  unsigned proc_flags() const;
  unsigned proc_flags2() const;
  double ppm() const;
  double rppm() const;
  unsigned rppm_scale() const;
  double rppm_modifier() const;
  double proc_chance() const;
  timespan_t cooldown() const;

  /* Accessors for buff specific features of the proc. */
  timespan_t duration() const;
  timespan_t tick_time() const;

  friend void format_to( const special_effect_t&, fmt::format_context::iterator );
};
