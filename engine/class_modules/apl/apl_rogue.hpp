#pragma once

#include <string>

struct player_t;

namespace rogue_apl {

std::string potion( const player_t* );
std::string flask( const player_t* );
std::string food( const player_t* );
std::string rune( const player_t* );
std::string temporary_enchant( const player_t* );
void assassination( player_t* );
void outlaw( player_t* );
void subtlety( player_t* );
void assassination_df( player_t* );
void outlaw_df( player_t* );
void subtlety_df( player_t* );

} // namespace hunter_apl
