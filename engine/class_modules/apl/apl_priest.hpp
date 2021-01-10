// ==========================================================================
// Priest APL Definitions File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#pragma once

#include <string>

struct player_t;

namespace priest_apl
{
std::string potion( const player_t* );
std::string flask( const player_t* );
std::string food( const player_t* );
std::string rune( const player_t* );
std::string temporary_enchant( const player_t* );
void discipline( player_t* );
void holy( player_t* );
void shadow( player_t* );
void no_spec( player_t* );

}  // namespace priest_apl
