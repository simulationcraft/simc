#pragma once

#include <string>

struct player_t;

namespace demon_hunter_apl
{
	std::string potion( const player_t* );
	std::string flask( const player_t* );
	std::string food( const player_t* );
	std::string rune( const player_t* );
	std::string temporary_enchant( const player_t* );
	void havoc( player_t* );
	void vengeance( player_t* );
}  // namespace demon_hunter_apl
