#pragma once

#include <string>

struct player_t;

namespace mage_apl {

std::string potion( const player_t* );
std::string flask( const player_t* );
std::string food( const player_t* );
std::string rune( const player_t* );
void arcane( player_t* );
void fire( player_t* );
void frost( player_t* );

}  // namespace mage_apl
