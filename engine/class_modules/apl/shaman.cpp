#include "class_modules/apl/shaman.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace shaman_apl
{
std::string potion( const player_t* )
{
  return "disabled";
}

std::string flask( const player_t* )
{
  return "disabled";
}

std::string food( const player_t* )
{
  return "disabled";
}

std::string rune( const player_t* )
{
  return "disabled";
}

std::string temporary_enchant( const player_t* )
{
  return "disabled";
}

//shaman_apl_start
void shaman( player_t* p )
{
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_  = p->get_action_priority_list( "default" );

  precombat->add_action( "flametongue_weapon" );
  precombat->add_action( "strength_of_earth_totem" );

  default_->add_action( "auto_attack" );
  default_->add_action( "strength_of_earth_totem,if=!buff.strength_of_earth.up" );
  default_->add_action( "flame_shock,if=!ticking|remains<1" );
  default_->add_action( "earth_shock,if=dot.flame_shock.ticking|!action.flame_shock.enabled" );
}
//shaman_apl_end
}  // namespace shaman_apl
