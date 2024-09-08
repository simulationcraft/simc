#pragma once

#include <string>

struct player_t;

namespace shaman_apl
{
  std::string flask_elemental( const player_t* );
  std::string food_elemental( const player_t* );
  std::string potion_elemental( const player_t* );
  std::string temporary_enchant_elemental( const player_t* );

  std::string rune( const player_t* );

  void elemental( player_t* );
  void elemental_ptr( player_t* );

}  // namespace shaman_apl
