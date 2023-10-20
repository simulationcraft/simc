#pragma once

#include <string>

struct player_t;

namespace demon_hunter_apl
{
std::string potion( const player_t* );
std::string flask_havoc( const player_t* );
std::string flask_vengeance( const player_t* );
std::string flask_vengeance_ptr( const player_t* );
std::string food( const player_t* );
std::string rune( const player_t* );
std::string temporary_enchant_havoc( const player_t* );
std::string temporary_enchant_vengeance( const player_t* );
std::string temporary_enchant_vengeance_ptr( const player_t* );
void havoc( player_t* );
void havoc_ptr( player_t* );
void vengeance( player_t* );
void vengeance_ptr( player_t* );
}  // namespace demon_hunter_apl
