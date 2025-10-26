#include "sim_controller.hpp"

#include "player/set_bonus.hpp"

min_player_stat_t::min_player_stat_t( sim_t* sim, player_t* target_player, stat_e rating, double amount )
  : sim_controller_t( sim ), target_player( target_player ), rating( rating ), min_rating( amount )
{
}

bool min_player_stat_t::evaluate_post_init()
{
  return target_player->get_stat_value( rating ) >= min_rating;
}

tier_set_count_t::tier_set_count_t( sim_t* sim, player_t* target_player, set_bonus_type_e tier, set_bonus_e count )
  : sim_controller_t( sim ), target_player( target_player ), tier( tier ), count( count )
{
}

bool tier_set_count_t::evaluate_post_init()
{
  return target_player->sets->has_set_bonus( target_player->specialization(), tier, count );
}

const std::string tier_set_count_t::reason() const
{
  // no to string for set bonus tier or count...
  // that should definitely exist :)
  return fmt::format( "player {} does not have tier {} {} active", target_player->name(), static_cast<int>( tier ),
                      static_cast<int>( count ) );
}
