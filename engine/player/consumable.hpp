// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "action/action.hpp"

#include <memory>

struct action_t;
struct player_t;
struct dbc_item_data_t;
struct special_effect_t;
struct actor_pair_t;

struct dbc_consumable_base_t : public action_t
{
  std::string consumable_name;
  std::unique_ptr<item_t> consumable_item;  // Dummy item used for quality adjustment done via ilevel
  const dbc_item_data_t* item_data;
  item_subclass_consumable type;

  action_t* consumable_action;
  buff_t* consumable_buff;
  bool opt_disabled;  // Disabled through a consumable-specific "disabled" keyword

  dbc_consumable_base_t( player_t* p, std::string_view name_str );

  std::unique_ptr<expr_t> create_expression( std::string_view name ) override;

  // Needed to satisfy normal execute conditions
  result_e calculate_result( action_state_t* ) const override
  { return RESULT_HIT; }

  void execute() override;

  // Figure out the default consumable for a given type
  virtual std::string consumable_default() const
  { return {}; }

  // Consumable type is fully disabled; base class just returns the option state (for the consumable
  // type). Consumable type specialized classes take into account other options (i.e., the allow_X
  // sim-wide options)
  virtual bool disabled_consumable() const
  { return opt_disabled; }

  void init() override;

  virtual const spell_data_t* driver() const;

  virtual special_effect_t* create_special_effect();

  virtual void initialize_consumable();

  bool ready() override;
};

struct consumable_buff_item_data_t
{
  const dbc_item_data_t* item_data = nullptr;
};

template <class BASE>
struct consumable_buff_t : public BASE, public consumable_buff_item_data_t
{
  template <typename... Args>
  consumable_buff_t( Args&&... args ) : BASE( std::forward<Args>( args )... ) {}
};

namespace consumable
{
action_t* create_action( player_t*, std::string_view name, std::string_view options );
void create_consumeable_actions( player_t* );
}  // namespace consumable
