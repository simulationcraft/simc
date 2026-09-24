#include "class_modules/apl/paladin.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace paladin_apl
{
std::string potion( const player_t* p )
{
  return "disabled";
}

std::string flask( const player_t* p )
{
  return "disabled";
}

std::string food( const player_t* p )
{
  return "disabled";
}

std::string rune( const player_t* p )
{
  return "disabled";
}

std::string temporary_enchant( const player_t* p )
{
  return "disabled";
}
}  // namespace paladin_apl
