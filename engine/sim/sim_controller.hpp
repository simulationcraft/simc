#pragma once

#include "player/player.hpp"
#include "player/rating.hpp"
#include "sc_enums.hpp"
#include "sim.hpp"

template <typename TBase, typename... Args, typename>
bool sim_controller_t::register_sim_controller( sim_t* sim, Args&&... args )
{
  if ( sim && sim->profileset_enabled && sim->parent )
  {
    sim->sim_controllers.emplace_back( std::make_unique<TBase>( sim, std::forward<Args>( args )... ) );
    return sim->parent->sim_controller_data
        .emplace( sim->sim_controllers.back()->name(), std::make_unique<typename TBase::data_t>() )
        .second;
  }
  return false;
}

template <typename T>
data_wrapper_t<T> sim_controller_t::get_data()
{
  auto& data = parent->sim_controller_data.at( name() );
  return { *std::static_pointer_cast<T>( data.data ), data.mutex };
}

template <typename T>
void sim_controller_t::set_data( T&& data )
{
  auto& scd = parent->sim_controller_data;
  assert( scd.find( name() ) != scd.end() );
  scd[ name() ].data = std::make_unique<T>( data );
}

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
  const std::string name() const override
  {
    return "min_player_stat";
  }
  bool evaluate_post_init() override;
  const std::string reason() const override;
};

struct tier_set_count_t : sim_controller_t
{
  using data_t = sim_controller_data_t;

  player_t* target_player;
  set_bonus_type_e tier;
  set_bonus_e count;

  tier_set_count_t( sim_t*, player_t*, set_bonus_type_e, set_bonus_e );
  const std::string name() const override
  {
    return "tier_set_count";
  }
  bool evaluate_post_init() override;
  const std::string reason() const override;
};
