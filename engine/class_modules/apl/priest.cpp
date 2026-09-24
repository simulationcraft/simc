#include "class_modules/apl/priest.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace priest_apl
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
}  // namespace priest_apl
