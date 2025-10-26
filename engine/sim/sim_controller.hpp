#pragma once

#include "sim.hpp"
#include "player/rating.hpp"
#include "player/player.hpp"
#include "sc_enums.hpp"

struct min_player_stat_t : sim_controller_t
{
  /*
   * This sim controller doesn't work, as at all controller evaluation points
   * only have base rating provided by the class/spec. If gear stats were to
   * be set once on actor init and preserved between iterations, this would be
   * fixed.
   */
  using data_t = sim_controller_data_t;

  player_t* target_player;
  stat_e rating;
  double min_rating;

  min_player_stat_t( sim_t*, player_t*, stat_e, double );
  const std::string name() const override { return "min_player_stat"; }
  bool evaluate_post_init() override;
};

struct tier_set_count_t : sim_controller_t
{
  using data_t = sim_controller_data_t;

  player_t* target_player;
  set_bonus_type_e tier;
  set_bonus_e count;

  tier_set_count_t( sim_t*, player_t*, set_bonus_type_e, set_bonus_e );
  const std::string name() const override { return "tier_set_count"; }
  bool evaluate_post_init() override;
  const std::string reason() const override;
};
