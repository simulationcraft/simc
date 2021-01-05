#pragma once

#include <string>

struct player_t;

namespace monk_apl
{
std::string potion( const player_t* );
std::string flask( const player_t* );
std::string food( const player_t* );
std::string rune( const player_t* );
std::string temporary_enchant( const player_t* );
void brewmaster( player_t* );
void mistweaver( player_t* );
void windwalker( player_t* );
void no_spec( player_t* );

}  // namespace monk_apl
