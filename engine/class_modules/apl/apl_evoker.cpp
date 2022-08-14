#include "class_modules/apl/apl_evoker.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace evoker_apl
{

std::string potion( const player_t* p )
{
  std::string lvl50_potion = ( p->specialization() == EVOKER_DEVASTATION ) ? "unbridled_fury" : "battle_potion_of_intellect";

  return ( p->true_level > 50 ) ? "potion_of_spectral_intellect" : lvl50_potion;
}

std::string flask( const player_t* p )
{
  return ( p->true_level > 50 ) ? "spectral_flask_of_power" : "greater_flask_of_endless_fathoms";
}

std::string food( const player_t* p )
{
  return ( p->true_level > 50 ) ? "feast_of_gluttonous_hedonism" : "baked_port_tato";
}

std::string rune( const player_t* p )
{
  return ( p->true_level > 50 ) ? "veiled_augment_rune" : "battle_scarred";
}

std::string temporary_enchant( const player_t* p )
{
  return ( p->true_level >= 60 ) ? "main_hand:shadowcore_oil" : "disabled";
}

void devastation( player_t* p )
{
  action_priority_list_t* precombat    = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_list = p->get_action_priority_list( "default" );

  // Snapshot stats
  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Professions
  for ( const auto& profession_action : p->get_profession_actions() )
  {
    default_list->add_action( profession_action );
  }

  // Potions
  default_list->add_action( "potion" );
  default_list->add_action( "disintegrate,if=cooldown.dragonrage.remains<=3*gcd.max" );
  default_list->add_action( "dragonrage" );
  default_list->add_action( "shattering_star" );
  default_list->add_action( "eternity_surge,empower_to=1" );
  default_list->add_action( "tip_the_scales,if=buff.dragonrage.up" );
  default_list->add_action( "fire_breath" );
  default_list->add_action( "pyre,if=spell_targets.pyre>2" );
  default_list->add_action( "living_flame,if=buff.burnout.up&buff.essence_burst.stack<buff.essence_burst.max_stack" );
  default_list->add_action( "disintegrate" );
  default_list->add_action( "azure_strike,if=spell_targets.azure_strike>2" );
  default_list->add_action( "living_flame" );
  
}

void preservation( player_t* p )
{
}

void no_spec( player_t* p )
{
}

}  // namespace evoker_apl
