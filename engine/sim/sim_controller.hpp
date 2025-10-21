#pragma once

#include "sim.hpp"
#include "player/rating.hpp"
#include "player/player.hpp"

struct min_player_stat_t : sim_controller_t
{
  // no using as this is a unary sim controller with no paired data
  using data_t = sim_controller_data_t;

  player_t* target_player;
  stat_e rating;
  double min_rating;

  min_player_stat_t( sim_t*, player_t*, stat_e, double );
  const std::string name() const override { return "min_player_stat"; };
  bool evaluate_post_init() override;// { return false; }
  bool evaluate_post_iter() override;// { return true; }
  void report_json_profileset( js::JsonOutput& ) override;
  void report_json_options( js::JsonOutput& ) override;
  void report_html( std::ostream& ) override;
};
