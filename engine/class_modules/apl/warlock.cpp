#include "class_modules/apl/warlock.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace warlock_apl{
  std::string potion( player_t* p )
  {
    return ( p->true_level >= 60 ) ? "spectral_intellect" : "disabled";
  }

  std::string flask( player_t* p )
  {
    return ( p->true_level >= 60 ) ? "spectral_flask_of_power" : "disabled";
  }

  std::string food( player_t* p )
  {
    return ( p->true_level >= 60 ) ? "feast_of_gluttonous_hedonism" : "disabled";
  }

  std::string rune( player_t* p )
  {
    return ( p->true_level >= 60 ) ? "veiled" : "disabled";
  }

  std::string temporary_enchant( player_t* p )
  {
    return ( p->true_level >= 60 ) ? "main_hand:shadowcore_oil" : "disabled";
  }

  //affliction_apl_start
  void affliction( player_t* p )
  {
  }
  //affliction_apl_end

  //demonology_apl_start
  void demonology( player_t* p )
  {
  }
  //demonology_apl_end

  //destruction_apl_start
  void destruction( player_t* p )
  {
  }
  //destruction_apl_end

} // namespace warlock_apl