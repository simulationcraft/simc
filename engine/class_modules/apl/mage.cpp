#include "class_modules/apl/mage.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace mage_apl
{
std::string potion( const player_t* )
{
  return "disabled";
}

std::string flask( const player_t* )
{
  return "disabled";
}

std::string food( const player_t* )
{
  return "disabled";
}

std::string rune( const player_t* )
{
  return "disabled";
}

std::string temporary_enchant( const player_t* )
{
  return "disabled";
}

//mage_apl_start
// shared mana management: armor before combat, gems and Evocation when low
static void mana( action_priority_list_t* precombat, action_priority_list_t* default_ )
{
  precombat->add_action( "mage_armor" );
  precombat->add_action( "ice_armor,if=!buff.mage_armor.up" );
  precombat->add_action( "frost_armor,if=!buff.mage_armor.up&!buff.ice_armor.up" );

  default_->add_action( "mana_gem,if=mana.deficit>1200|mana.pct<40" );
  default_->add_action( "evocation,if=mana.pct<15" );
}

void arcane( player_t* p )
{
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_  = p->get_action_priority_list( "default" );

  mana( precombat, default_ );
  default_->add_action( "arcane_power" );
  default_->add_action( "presence_of_mind" );
  default_->add_action( "arcane_wrath" );
  default_->add_action( "arcane_blast" );
  default_->add_action( "arcane_missiles" );
  default_->add_action( "frostbolt" );
}

void fire( player_t* p )
{
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_  = p->get_action_priority_list( "default" );

  mana( precombat, default_ );
  default_->add_action( "combustion" );
  default_->add_action( "pyroblast,if=buff.presence_of_mind.up" );
  default_->add_action( "fire_blast" );
  default_->add_action( "fireball" );
}

void frost( player_t* p )
{
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_  = p->get_action_priority_list( "default" );

  mana( precombat, default_ );
  default_->add_action( "frostbolt" );
}
//mage_apl_end
}  // namespace mage_apl
