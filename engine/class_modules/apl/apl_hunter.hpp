#pragma once

#include <string>

struct player_t;

namespace hunter_apl {

std::string potion( const player_t* );
std::string flask( const player_t* );
std::string food( const player_t* );
std::string rune( const player_t* );
std::string temporary_enchant( const player_t* );
void beast_mastery( player_t* );
void marksmanship( player_t* );
void survival( player_t* );

} // namespace hunter_apl
