#pragma once

#include <string>

struct player_t;

namespace death_knight_apl {

std::string potion( const player_t* );
std::string flask( const player_t* );
std::string food( const player_t* );
std::string rune( const player_t* );
std::string temporary_enchant( const player_t* );
void blood( player_t* );
void frost( player_t* );
void unholy( player_t* );

}  // namespace death_knight_apl
