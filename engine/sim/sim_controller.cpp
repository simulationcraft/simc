#include "sim_controller.hpp"

// struct min_player_stat_t : sim_controller_t
// {
//   // no using as this is a unary sim controller with no paired data

//   player_t* target_player;
//   rating_e rating;
//   double min_rating;

//   min_player_stat_t( sim_t*, player_t*, rating_e, double );
//   const std::string name() const override { return { "min_player_stat" }; };
//   bool evaluate_post_init() override;
//   bool evaluate_post_iter() override { return true; }
//   void report_json_profileset( js::JsonOutput& ) override;
//   void report_json_options( js::JsonOutput& ) override;
//   void report_html( std::ostream& ) override;
// };

min_player_stat_t::min_player_stat_t( sim_t* sim, player_t* target_player, stat_e rating, double amount )
  : sim_controller_t( sim ), target_player( target_player ), rating( rating ), min_rating( amount )
{}

bool min_player_stat_t::evaluate_post_iter()
{
  return false;
}

bool min_player_stat_t::evaluate_post_init()
{
  // return false;
  return target_player->get_stat_value( rating ) < min_rating;
}

void min_player_stat_t::report_json_profileset( js::JsonOutput& ){}
void min_player_stat_t::report_json_options( js::JsonOutput& ){}
void min_player_stat_t::report_html( std::ostream& ){}
