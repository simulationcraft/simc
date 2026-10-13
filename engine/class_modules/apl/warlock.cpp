#include "class_modules/apl/warlock.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace warlock_apl
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
//warlock_apl_start
void warlock( player_t* p )
{
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_  = p->get_action_priority_list( "default" );

  precombat->add_action( "summon_imp" );

  default_->add_action( "immolate,if=!ticking|remains<2" );
  default_->add_action( "corruption,if=!ticking|remains<2" );
  default_->add_action( "shadow_bolt" );
}
//warlock_apl_end
}  // namespace warlock_apl
