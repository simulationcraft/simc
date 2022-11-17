#include "class_modules/apl/apl_evoker.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace evoker_apl
{

std::string potion( const player_t* p )
{
  return ( p->true_level > 60 ) ? "elemental_potion_of_ultimate_power_3" : "potion_of_spectral_intellect" ;
}

std::string flask( const player_t* p )
{
  return ( p->true_level > 60 ) ? "phial_of_static_empowerment_3" : "greater_flask_of_endless_fathoms";
}

std::string food( const player_t* p )
{
  return ( p->true_level > 60 ) ? "fated_fortune_cookie" : "feast_of_gluttonous_hedonism";
}

std::string rune( const player_t* p )
{
  return ( p->true_level > 60 ) ? "draconic_augment_rune" : "veiled_augment_rune";
}

std::string temporary_enchant( const player_t* p )
{
  return ( p->true_level > 60 ) ? "main_hand:howling_rune" : "main_hand:shadowcore_oil";
}

//devastation_apl_start
void devastation( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "use_item,name=shadowed_orb_of_torment" );
  precombat->add_action( "firestorm,if=talent.firestorm" );
  precombat->add_action( "living_flame,if=!talent.firestorm" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=trinket.1.has_use_buff&(trinket.1.cooldown.duration%%cooldown.dragonrage.duration=0)", "Evaluates both trinkets cooldowns to see if they can be evenly divided by the cooldown of Dragonrage, prioritizes trinkets that will sync with this cooldown" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=trinket.2.has_use_buff&(trinket.2.cooldown.duration%%cooldown.dragonrage.duration=0)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!trinket.1.has_use_buff&trinket.2.has_use_buff|trinket.2.has_use_buff&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.intellect)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.intellect)*(variable.trinket_1_sync))", "Estimates a trinkets value by comparing the cooldown of the trinket, divided by the duration of the buff it provides. Has a intellect modifier (currently 1.5x) to give a higher priority to intellect trinkets. The intellect modifier should be changed as intellect priority increases or decreases. As well as a modifier for if a trinket will or will not sync with cooldowns." );
 
  default_->add_action( "potion,if=buff.dragonrage.up|time>=300&fight_remains<35" );
  default_->add_action( "use_item,name=shadowed_orb_of_torment" );
  default_->add_action( "use_item,name=crimson_aspirants_badge_of_ferocity,if=cooldown.dragonrage.remains>=55" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "deep_breath,if=spell_targets.deep_breath>1&buff.dragonrage.down" );
  default_->add_action( "dragonrage,if=cooldown.eternity_surge.remains<=(buff.dragonrage.duration+6)&(cooldown.fire_breath.remains<=2*gcd.max|!talent.feed_the_flames)" );
  default_->add_action( "tip_the_scales,if=buff.dragonrage.up&(cooldown.eternity_surge.up|cooldown.fire_breath.up)&buff.dragonrage.remains<=gcd.max" );
  default_->add_action( "eternity_surge,empower_to=1,if=buff.dragonrage.up&(buff.bloodlust.up|buff.power_infusion.up)&talent.feed_the_flames" );
  default_->add_action( "tip_the_scales,if=buff.dragonrage.up&cooldown.fire_breath.up&talent.everburning_flame&talent.firestorm" );
  default_->add_action( "fire_breath,empower_to=1,if=talent.everburning_flame&(cooldown.firestorm.remains>=2*gcd.max|!dot.firestorm.ticking)|cooldown.dragonrage.remains>=10&talent.feed_the_flames|!talent.everburning_flame&!talent.feed_the_flames" );
  default_->add_action( "fire_breath,empower_to=2,if=talent.everburning_flame" );
  default_->add_action( "firestorm,if=talent.everburning_flame&(cooldown.fire_breath.up|dot.fire_breath_damage.remains>=cast_time&dot.fire_breath_damage.remains<cooldown.fire_breath.remains)|buff.snapfire.up|spell_targets.firestorm>1" );
  default_->add_action( "eternity_surge,empower_to=4,if=spell_targets.pyre>3*(1+talent.eternitys_span)" );
  default_->add_action( "eternity_surge,empower_to=3,if=spell_targets.pyre>2*(1+talent.eternitys_span)" );
  default_->add_action( "eternity_surge,empower_to=2,if=spell_targets.pyre>(1+talent.eternitys_span)" );
  default_->add_action( "eternity_surge,empower_to=1,if=(cooldown.eternity_surge.duration-cooldown.dragonrage.remains)<(buff.dragonrage.duration+6-gcd.max)" );
  default_->add_action( "shattering_star,if=!talent.arcane_vigor|essence+1<essence.max|buff.dragonrage.up" );
  default_->add_action( "azure_strike,if=essence<essence.max&!buff.burnout.up&spell_targets.azure_strike>(2-buff.dragonrage.up)&buff.essence_burst.stack<buff.essence_burst.max_stack&(!talent.ruby_embers|spell_targets.azure_strike>2)" );
  default_->add_action( "pyre,if=spell_targets.pyre>(2+talent.scintillation*talent.eternitys_span)|buff.charged_blast.stack=buff.charged_blast.max_stack&cooldown.dragonrage.remains>20&spell_targets.pyre>2" );
  default_->add_action( "living_flame,if=essence<essence.max&buff.essence_burst.stack<buff.essence_burst.max_stack&(buff.burnout.up|!talent.engulfing_blaze&!talent.shattering_star&buff.dragonrage.up&target.health.pct>80)" );
  default_->add_action( "disintegrate,chain=1,if=buff.dragonrage.up,interrupt_if=buff.dragonrage.up&ticks>=2,interrupt_immediate=1" );
  default_->add_action( "disintegrate,chain=1,if=essence=essence.max|buff.essence_burst.stack=buff.essence_burst.max_stack|debuff.shattering_star_debuff.up|cooldown.shattering_star.remains>=3*gcd.max|!talent.shattering_star" );
  default_->add_action( "use_item,name=kharnalex_the_first_light,if=!debuff.shattering_star_debuff.up&!buff.dragonrage.up&spell_targets.pyre=1" );
  default_->add_action( "azure_strike,if=spell_targets.azure_strike>2|(talent.engulfing_blaze|talent.feed_the_flames)&buff.dragonrage.up" );
  default_->add_action( "living_flame" );
  
  trinkets->add_action( "use_item,slot=trinket1,if=buff.dragonrage.up&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains", "The trinket with the highest estimated value, will be used first and paired with Dragonrage." );
  trinkets->add_action( "use_item,slot=trinket2,if=buff.dragonrage.up&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains" );
  trinkets->add_action( "use_item,slot=trinket1,if=(!trinket.1.has_use_buff&(trinket.2.cooldown.remains|!trinket.2.has_use_buff)|cooldown.dragonrage.remains>20|!talent.dragonrage)", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  trinkets->add_action( "use_item,slot=trinket2,if=(!trinket.2.has_use_buff&(trinket.1.cooldown.remains|!trinket.1.has_use_buff)|cooldown.dragonrage.remains>20|!talent.dragonrage)" );
}
//devastation_apl_end

void preservation( player_t* /*p*/ )
{
}

void no_spec( player_t* /*p*/ )
{
}

}  // namespace evoker_apl
