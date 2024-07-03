#include "class_modules/apl/warlock.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace warlock_apl{
  std::string potion( const player_t* p )
  {
    if ( p->true_level >= 70 ) return "elemental_potion_of_ultimate_power_3";
    return ( p->true_level >= 60 ) ? "spectral_intellect" : "disabled";
  }

  std::string flask( const player_t* p )
  {
    if ( p->true_level >= 70 ) return "iced_phial_of_corrupting_rage_3";
    return ( p->true_level >= 60 ) ? "spectral_flask_of_power" : "disabled";
  }

  std::string food( const player_t* p )
  {
    if ( p->true_level >= 70 ) return "fated_fortune_cookie";
    return ( p->true_level >= 60 ) ? "feast_of_gluttonous_hedonism" : "disabled";
  }

  std::string rune( const player_t* p )
  {
    if ( p->true_level >= 70 ) return "draconic_augment_rune";
    return ( p->true_level >= 60 ) ? "veiled" : "disabled";
  }

  std::string temporary_enchant( const player_t* p )
  {
    if ( p->true_level >= 70 ) return "main_hand:howling_rune_3";
    return ( p->true_level >= 60 ) ? "main_hand:shadowcore_oil" : "disabled";
  }

//affliction_apl_start
void affliction( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );

  default_->add_action( "shadow_bolt" );
}
//affliction_apl_end

//demonology_apl_start
void demonology( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );

  default_->add_action( "shadow_bolt" );
}
//demonology_apl_end

//destruction_apl_start
void destruction( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );

  default_->add_action( "incinerate" );
}
//destruction_apl_end

} // namespace warlock_apl
