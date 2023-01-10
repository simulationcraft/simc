#pragma once

#include <string>

struct player_t;

namespace warlock_apl
{
	std::string potion( const player_t* );
  std:: string flask( const player_t* );
  std::string food( const player_t* );
  std::string rune( const player_t* );
  std::string temporary_enchant( const player_t* );

	void affliction( player_t* );
	void demonology( player_t* );
  void destruction( player_t* );
}  // namespace warlock_apl
