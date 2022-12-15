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
  return ( p->true_level > 60 ) ? "main_hand:howling_rune_3" : "main_hand:shadowcore_oil";
}

//devastation_apl_start
void devastation( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* fb = p->get_action_priority_list( "fb" );
  action_priority_list_t* es = p->get_action_priority_list( "es" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_buff.intellect|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_buff.intellect|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%cooldown.dragonrage.duration=0|cooldown.dragonrage.duration%%trinket.1.cooldown.duration=0)", "Decide which trinket to pair with Dragonrage, prefer 2 minute and 1 minute trinkets" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%cooldown.dragonrage.duration=0|cooldown.dragonrage.duration%%trinket.2.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.intellect)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.intellect)*(variable.trinket_1_sync))", "Estimates a trinkets value by comparing the cooldown of the trinket, divided by the duration of the buff it provides. Has a intellect modifier (currently 1.5x) to give a higher priority to intellect trinkets. The intellect modifier should be changed as intellect priority increases or decreases. As well as a modifier for if a trinket will or will not sync with cooldowns." );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.spoils_of_neltharus" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.spoils_of_neltharus" );
  precombat->add_action( "use_item,name=shadowed_orb_of_torment" );
  precombat->add_action( "firestorm,if=talent.firestorm" );
  precombat->add_action( "living_flame,if=!talent.firestorm" );

  default_->add_action( "potion,if=buff.dragonrage.up|fight_remains<35" );
  default_->add_action( "variable,name=next_dragonrage,value=cooldown.dragonrage.remains<?(cooldown.eternity_surge.remains-2*gcd.max)<?(cooldown.fire_breath.remains-gcd.max)", "Variable that evaluates when next dragonrage is by working out the maximum between the dragonrage cd and your empowers, ignoring CDR effect estimates." );
  default_->add_action( "variable,name=r1_cast_time,value=1.3*spell_haste", "Rank 1 empower spell cast time TODO: multiplier should be 1.0 but 1.3 results in more dps for EBF builds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "run_action_list,name=aoe,if=spell_targets.pyre>=3" );
  default_->add_action( "run_action_list,name=st" );

  st->add_action( "use_item,name=kharnalex_the_first_light,if=!buff.dragonrage.up&debuff.shattering_star_debuff.down", "ST Action List, it's a mess" );
  st->add_action( "dragonrage,if=cooldown.fire_breath.remains<gcd.max&cooldown.eternity_surge.remains<2*gcd.max|fight_remains<30" );
  st->add_action( "tip_the_scales,if=buff.dragonrage.up&(buff.dragonrage.remains<variable.r1_cast_time&(buff.dragonrage.remains>cooldown.fire_breath.remains|buff.dragonrage.remains>cooldown.eternity_surge.remains)|talent.feed_the_flames&!cooldown.fire_breath.up)", "Use to extend DR when an empower cast won't fit inside the DR window anymore. When running FTF use on ES at the start of DR to maximize uses" );
  st->add_action( "call_action_list,name=fb,if=!talent.dragonrage|variable.next_dragonrage>15|!talent.animosity" );
  st->add_action( "call_action_list,name=es,if=!talent.dragonrage|variable.next_dragonrage>15|!talent.animosity" );
  st->add_action( "wait,sec=cooldown.fire_breath.remains,if=talent.animosity&buff.dragonrage.up&buff.dragonrage.remains<gcd.max+variable.r1_cast_time*buff.tip_the_scales.down&buff.dragonrage.remains-cooldown.fire_breath.remains>=variable.r1_cast_time*buff.tip_the_scales.down", "Wait for FB/ES to be ready if spending another GCD would result in the cast no longer fitting inside of DR" );
  st->add_action( "wait,sec=cooldown.eternity_surge.remains,if=talent.animosity&buff.dragonrage.up&buff.dragonrage.remains<gcd.max+variable.r1_cast_time&buff.dragonrage.remains-cooldown.eternity_surge.remains>variable.r1_cast_time*buff.tip_the_scales.down" );
  st->add_action( "shattering_star,if=!buff.dragonrage.up|buff.essence_burst.stack==buff.essence_burst.max_stack|talent.eye_of_infinity", "Wait for 2 EBs to use SS while inside DR, otherwise use on CD" );
  st->add_action( "living_flame,if=buff.dragonrage.up&buff.dragonrage.remains<(buff.essence_burst.max_stack-buff.essence_burst.stack)*gcd.max&buff.burnout.up", "Spend the last 1 or 2 GCDs of DR on fillers to exit with 2 EBs" );
  st->add_action( "azure_strike,if=buff.dragonrage.up&buff.dragonrage.remains<(buff.essence_burst.max_stack-buff.essence_burst.stack)*gcd.max" );
  st->add_action( "firestorm,if=!buff.dragonrage.up&debuff.shattering_star_debuff.down|buff.snapfire.up", "Hard cast only outside of SS and DR windows, always spend snapfire procs" );
  st->add_action( "living_flame,if=buff.burnout.up&buff.essence_burst.stack<buff.essence_burst.max_stack&essence<essence.max-1", "Spend burnout procs without overcapping resources" );
  st->add_action( "azure_strike,if=buff.dragonrage.up&(essence<3&!buff.essence_burst.up|(talent.shattering_star&cooldown.shattering_star.remains<=(buff.essence_burst.max_stack-buff.essence_burst.stack)*gcd.max))", "Ensure we clip Disintegrate inside DR even with our fillers, Pool 1-2 GCDs before SS is up inside DR" );
  st->add_action( "disintegrate,chain=1,early_chain_if=buff.dragonrage.up&ticks>=2,interrupt_if=buff.dragonrage.up&ticks>=2,if=buff.dragonrage.up|(!talent.shattering_star|cooldown.shattering_star.remains>6|essence>essence.max-1|buff.essence_burst.stack==buff.essence_burst.max_stack)", "In DR chain/clip after the 3rd damage tick, Outside of DR pool 6 seconds before SS unless it would result in overcapping resources TODO: revisit pooling conditions" );
  st->add_action( "deep_breath,if=!buff.dragonrage.up&spell_targets.deep_breath>1" );
  st->add_action( "living_flame" );

  aoe->add_action( "dragonrage,if=cooldown.fire_breath.remains<=gcd.max&cooldown.eternity_surge.remains<3*gcd.max", "AOE action list, also a mess." );
  aoe->add_action( "tip_the_scales,if=buff.dragonrage.up&(spell_targets.pyre<=6|!cooldown.fire_breath.up)" );
  aoe->add_action( "call_action_list,name=fb,if=buff.dragonrage.up|!talent.dragonrage|cooldown.dragonrage.remains>10&talent.everburning_flame" );
  aoe->add_action( "fire_breath,empower_to=1,if=cooldown.dragonrage.remains>10&spell_targets.pyre>=7" );
  aoe->add_action( "fire_breath,empower_to=2,if=cooldown.dragonrage.remains>10&spell_targets.pyre>=6" );
  aoe->add_action( "fire_breath,empower_to=3,if=cooldown.dragonrage.remains>10&spell_targets.pyre>=4" );
  aoe->add_action( "fire_breath,empower_to=2,if=cooldown.dragonrage.remains>10" );
  aoe->add_action( "call_action_list,name=es,if=buff.dragonrage.up|!talent.dragonrage|cooldown.dragonrage.remains>15" );
  aoe->add_action( "azure_strike,if=buff.dragonrage.up&buff.dragonrage.remains<(buff.essence_burst.max_stack-buff.essence_burst.stack)*gcd.max" );
  aoe->add_action( "deep_breath,if=!buff.dragonrage.up" );
  aoe->add_action( "firestorm" );
  aoe->add_action( "shattering_star" );
  aoe->add_action( "azure_strike,if=cooldown.dragonrage.remains<gcd.max*6&cooldown.fire_breath.remains<6*gcd.max&cooldown.eternity_surge.remains<6*gcd.max" );
  aoe->add_action( "pyre,if=talent.volatility" );
  aoe->add_action( "living_flame,if=buff.burnout.up&buff.leaping_flames.up&!buff.essence_burst.up" );
  aoe->add_action( "pyre,if=cooldown.dragonrage.remains>=10&spell_targets.pyre>=4" );
  aoe->add_action( "pyre,if=cooldown.dragonrage.remains>=10&spell_targets.pyre=3&buff.charged_blast.stack>=10" );
  aoe->add_action( "disintegrate,chain=1,if=!talent.shattering_star|cooldown.shattering_star.remains>5|essence>essence.max-1|buff.essence_burst.stack==buff.essence_burst.max_stack" );
  aoe->add_action( "living_flame,if=talent.snapfire&buff.burnout.up" );
  aoe->add_action( "azure_strike" );

  fb->add_action( "fire_breath,empower_to=1,if=(20+2*talent.blast_furnace.rank)+dot.fire_breath_damage.remains<(20+2*talent.blast_furnace.rank)*1.3|buff.dragonrage.remains<1.75*spell_haste&buff.dragonrage.remains>=1*spell_haste|active_enemies<=2", "Use Firebreath with some really weird criteria. Override it for st because its not actually useful? Idk. TODO: Someone look at this it's a mess." );
  fb->add_action( "fire_breath,empower_to=2,if=(14+2*talent.blast_furnace.rank)+dot.fire_breath_damage.remains<(20+2*talent.blast_furnace.rank)*1.3|buff.dragonrage.remains<2.5*spell_haste&buff.dragonrage.remains>=1.75*spell_haste" );
  fb->add_action( "fire_breath,empower_to=3,if=(8+2*talent.blast_furnace.rank)+dot.fire_breath_damage.remains<(20+2*talent.blast_furnace.rank)*1.3|!talent.font_of_magic|buff.dragonrage.remains<=3.25*spell_haste&buff.dragonrage.remains>=2.5*spell_haste" );
  fb->add_action( "fire_breath,empower_to=4" );

  es->add_action( "eternity_surge,empower_to=1,if=spell_targets.pyre<=1+talent.eternitys_span|buff.dragonrage.remains<1.75*spell_haste&buff.dragonrage.remains>=1*spell_haste", "Eternity Surge, use rank most applicable to targets." );
  es->add_action( "eternity_surge,empower_to=2,if=spell_targets.pyre<=2+2*talent.eternitys_span|buff.dragonrage.remains<2.5*spell_haste&buff.dragonrage.remains>=1.75*spell_haste" );
  es->add_action( "eternity_surge,empower_to=3,if=spell_targets.pyre<=3+3*talent.eternitys_span|!talent.font_of_magic|buff.dragonrage.remains<=3.25*spell_haste&buff.dragonrage.remains>=2.5*spell_haste" );
  es->add_action( "eternity_surge,empower_to=4" );

  trinkets->add_action( "use_item,name=spoils_of_neltharus,if=buff.dragonrage.up&(buff.spoils_of_neltharus_mastery.up|buff.spoils_of_neltharus_haste.up|buff.dragonrage.remains+6*(cooldown.eternity_surge.remains<=gcd.max*2+cooldown.fire_breath.remains<=gcd.max*2)<=18)|fight_remains<=20", "Try and get spoils to prvide haste or mastery stats, but if dragonrage gets too short just use it anyway." );
  trinkets->add_action( "use_item,slot=trinket1,if=buff.dragonrage.up&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)&!variable.trinket_1_manual|trinket.1.proc.any_dps.duration>=fight_remains|trinket.1.cooldown.duration<=60&(variable.next_dragonrage>20|!talent.dragonrage)&(!buff.dragonrage.up|variable.trinket_priority=1)", "The trinket with the highest estimated value, will be used first and paired with Dragonrage." );
  trinkets->add_action( "use_item,slot=trinket2,if=buff.dragonrage.up&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)&!variable.trinket_2_manual|trinket.2.proc.any_dps.duration>=fight_remains|trinket.2.cooldown.duration<=60&(variable.next_dragonrage>20|!talent.dragonrage)&(!buff.dragonrage.up|variable.trinket_priority=2)" );
  trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs)&(variable.next_dragonrage>20|!talent.dragonrage)&!variable.trinket_1_manual", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs)&(variable.next_dragonrage>20|!talent.dragonrage)&!variable.trinket_2_manual" );
}
//devastation_apl_end

void preservation( player_t* /*p*/ )
{
}

void no_spec( player_t* /*p*/ )
{
}

}  // namespace evoker_apl
