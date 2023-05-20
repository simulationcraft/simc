#pragma once

#include <string>

struct player_t;

namespace evoker_apl
{
std::string potion( const player_t* );
std::string flask( const player_t* );
std::string food( const player_t* );
std::string rune( const player_t* );
std::string temporary_enchant( const player_t* );
void devastation( player_t* );
void preservation( player_t* );
void augmentation( player_t* );
void no_spec( player_t* );

}  // namespace evoker_apl
