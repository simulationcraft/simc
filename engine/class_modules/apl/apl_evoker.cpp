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
  return ( p->true_level > 60 ) ? "iced_phial_of_corrupting_rage_3" : "greater_flask_of_endless_fathoms";
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
  switch ( p->specialization() )
  {
    case EVOKER_AUGMENTATION:
      return ( p->true_level > 60 ) ? "main_hand:hissing_rune_3" : "main_hand:shadowcore_oil";
    default:
      return ( p->true_level > 60 ) ? "main_hand:buzzing_rune_3" : "main_hand:shadowcore_oil";
  }
}

//devastation_apl_start
void devastation( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* es = p->get_action_priority_list( "es" );
  action_priority_list_t* fb = p->get_action_priority_list( "fb" );
  action_priority_list_t* green = p->get_action_priority_list( "green" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_buff.intellect|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit|trinket.1.is.mirror_of_fractured_tomorrows" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_buff.intellect|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit|trinket.2.is.mirror_of_fractured_tomorrows" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%cooldown.dragonrage.duration=0|cooldown.dragonrage.duration%%trinket.1.cooldown.duration=0)", "Decide which trinket to pair with Dragonrage, prefer 2 minute and 1 minute trinkets" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%cooldown.dragonrage.duration=0|cooldown.dragonrage.duration%%trinket.2.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.belorrelos_the_suncaller|trinket.1.is.nymues_unraveling_spindle", "Estimates a trinkets value by comparing the cooldown of the trinket, divided by the duration of the buff it provides. Has a intellect modifier (currently 1.5x) to give a higher priority to intellect trinkets. The intellect modifier should be changed as intellect priority increases or decreases. As well as a modifier for if a trinket will or will not sync with cooldowns." );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.belorrelos_the_suncaller|trinket.2.is.nymues_unraveling_spindle" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell|trinket.1.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell|trinket.2.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.intellect)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.intellect)*(variable.trinket_1_sync))" );
  precombat->add_action( "variable,name=r1_cast_time,value=1.0*spell_haste", "Rank 1 empower spell cast time" );
  precombat->add_action( "variable,name=dr_prep_time_aoe,default=4,op=reset", "Variable for when to start holding empowers for upcoming DR in AoE. - From my testing 4sec seems like the sweetspot, but it's very minor diff so far - Holding for more than 6 seconds it begins to become a loss." );
  precombat->add_action( "variable,name=dr_prep_time_st,default=13,op=reset", "Variable for when to start holding empowers for upcoming DR in ST." );
  precombat->add_action( "variable,name=has_external_pi,value=cooldown.invoke_power_infusion_0.duration>0" );
  precombat->add_action( "verdant_embrace,if=talent.scarlet_adaptation", "Get Some Scarlet Adaptation Prepull" );
  precombat->add_action( "firestorm,if=talent.firestorm" );
  precombat->add_action( "living_flame,if=!talent.firestorm" );

  default_->add_action( "potion,if=buff.dragonrage.up&(!cooldown.shattering_star.up|active_enemies>=2)|fight_remains<35", "Delay pot in ST if you are about to SS - mostly relevant for opener where you want DR->FB->SS->rotation" );
  default_->add_action( "variable,name=next_dragonrage,value=cooldown.dragonrage.remains<?(cooldown.eternity_surge.remains-2*gcd.max)<?(cooldown.fire_breath.remains-gcd.max)", "Variable that evaluates when next dragonrage is by working out the maximum between the dragonrage cd and your empowers, ignoring CDR effect estimates." );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=buff.dragonrage.up&(buff.emerald_trance_stacking.stack>=3&set_bonus.tier31_2pc|!cooldown.fire_breath.up&!cooldown.shattering_star.up&(!set_bonus.tier31_2pc|!talent.event_horizon))", "Invoke External Power Infusions if they're available during dragonrage" );
  default_->add_action( "quell,use_off_gcd=1,if=target.debuff.casting.react", "Rupt to make the raidleader happy" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "run_action_list,name=aoe,if=active_enemies>=3" );
  default_->add_action( "run_action_list,name=st" );

  aoe->add_action( "shattering_star,target_if=max:target.health.pct,if=cooldown.dragonrage.up", "AOE action list; This is kind of a mess again and should prolly be rewritten completely Open with star before DR to save a global and start with a free EB" );
  aoe->add_action( "firestorm,if=talent.raging_inferno&cooldown.dragonrage.remains<=gcd&(target.time_to_die>=32|fight_remains<30)", "Send DR on CD with no regard to the safety of the mobs - DPS loss to hold DR for empowers, in real world scenario maybe you hold if you are going for longer DRs DS optimization: Only cast if current fight will last 32s+ or encounter ends in less than 30s" );
  aoe->add_action( "dragonrage,if=target.time_to_die>=32|fight_remains<30", "actions.aoe+=/call_action_list,name=fb,if=talent.dragonrage&talent.iridescence&(target.time_to_die>=32|fight_remains<30)&cooldown.dragonrage.remains<=gcd" );
  aoe->add_action( "tip_the_scales,if=buff.dragonrage.up&(active_enemies<=3+3*talent.eternitys_span|!cooldown.fire_breath.up)", "Use tip to get that sweet aggro" );
  aoe->add_action( "call_action_list,name=fb,if=(!talent.dragonrage|variable.next_dragonrage>variable.dr_prep_time_aoe|!talent.animosity)&((buff.power_swell.remains<variable.r1_cast_time|(!talent.volatility&active_enemies=3))&buff.blazing_shards.remains<variable.r1_cast_time|buff.dragonrage.up)&(target.time_to_die>=8|fight_remains<30)", "Cast Fire Breath - stagger for swell/blazing shards outside DR DS optimization: Only cast if current fight will last 8s+ or encounter ends in less than 30s" );
  aoe->add_action( "call_action_list,name=es,if=buff.dragonrage.up|!talent.dragonrage|(cooldown.dragonrage.remains>variable.dr_prep_time_aoe&(buff.power_swell.remains<variable.r1_cast_time|(!talent.volatility&active_enemies=3))&buff.blazing_shards.remains<variable.r1_cast_time)&(target.time_to_die>=8|fight_remains<30)", "Cast Eternity Surge - stagger for swell/blazing shards outside DR DS optimization: Only cast if current fight will last 8s+ or encounter ends in less than 30s" );
  aoe->add_action( "deep_breath,if=!buff.dragonrage.up&essence.deficit>3", "Cast DB if not in DR and not going to overflow essence." );
  aoe->add_action( "shattering_star,target_if=max:target.health.pct,if=buff.essence_burst.stack<buff.essence_burst.max_stack|!talent.arcane_vigor", "Send SS when it doesn't overflow EB, without vigor send on CD" );
  aoe->add_action( "firestorm,if=talent.raging_inferno&(cooldown.dragonrage.remains>=20|cooldown.dragonrage.remains<=10)&(buff.essence_burst.up|essence>=2|cooldown.dragonrage.remains<=10)|buff.snapfire.up", "Cast Firestorm before spending ressources" );
  aoe->add_action( "pyre,target_if=max:target.health.pct,if=active_enemies>=4", "Pyre logic Pyre 4T+ Pyre 3T+ if playing Volatility Pyre with 15 stacks of CB" );
  aoe->add_action( "pyre,target_if=max:target.health.pct,if=active_enemies>=3&talent.volatility" );
  aoe->add_action( "pyre,target_if=max:target.health.pct,if=buff.charged_blast.stack>=15" );
  aoe->add_action( "living_flame,target_if=max:target.health.pct,if=(!talent.burnout|buff.burnout.up|active_enemies>=4&cooldown.fire_breath.remains<=gcd.max*3|buff.scarlet_adaptation.up)&buff.leaping_flames.up&!buff.essence_burst.up&essence<essence.max-1", "Cast LF with leaping flames up if: not playing burnout, burnout is up, more than 4 enemies, or scarlet adaptation is up." );
  aoe->add_action( "disintegrate,target_if=max:target.health.pct,chain=1,early_chain_if=evoker.use_early_chaining&ticks>=2&buff.dragonrage.up&(raid_event.movement.in>2|buff.hover.up),interrupt_if=evoker.use_clipping&buff.dragonrage.up&ticks>=2&(raid_event.movement.in>2|buff.hover.up),if=raid_event.movement.in>2|buff.hover.up", "Yoinked the disintegrate logic from ST" );
  aoe->add_action( "living_flame,target_if=max:target.health.pct,if=talent.snapfire&buff.burnout.up", "Cast LF with burnout and snapfire proc for those juicy insta firestorms" );
  aoe->add_action( "firestorm" );
  aoe->add_action( "call_action_list,name=green,if=talent.ancient_flame&!buff.ancient_flame.up&!buff.dragonrage.up", "Get Ancient Flame as Filler" );
  aoe->add_action( "azure_strike,target_if=max:target.health.pct", "Fallback filler" );

  es->add_action( "eternity_surge,empower_to=1,target_if=max:target.health.pct,if=active_enemies<=1+talent.eternitys_span|buff.dragonrage.remains<1.75*spell_haste&buff.dragonrage.remains>=1*spell_haste|buff.dragonrage.up&(active_enemies==5&!talent.font_of_magic|active_enemies>(3+talent.font_of_magic)*(1+talent.eternitys_span))|active_enemies>=6&!talent.eternitys_span", "Eternity Surge, use rank most applicable to targets." );
  es->add_action( "eternity_surge,empower_to=2,target_if=max:target.health.pct,if=active_enemies<=2+2*talent.eternitys_span|buff.dragonrage.remains<2.5*spell_haste&buff.dragonrage.remains>=1.75*spell_haste" );
  es->add_action( "eternity_surge,empower_to=3,target_if=max:target.health.pct,if=active_enemies<=3+3*talent.eternitys_span|!talent.font_of_magic|buff.dragonrage.remains<=3.25*spell_haste&buff.dragonrage.remains>=2.5*spell_haste" );
  es->add_action( "eternity_surge,empower_to=4,target_if=max:target.health.pct" );

  fb->add_action( "fire_breath,empower_to=1,target_if=max:target.health.pct,if=(buff.dragonrage.up&active_enemies<=2)|(active_enemies=1&!talent.everburning_flame)|(buff.dragonrage.remains<1.75*spell_haste&buff.dragonrage.remains>=1*spell_haste)", "Fire Breath, use rank appropriate to target count/talents." );
  fb->add_action( "fire_breath,empower_to=2,target_if=max:target.health.pct,if=(!debuff.in_firestorm.up&talent.everburning_flame&active_enemies<=3)|(active_enemies=2&!talent.everburning_flame)|(buff.dragonrage.remains<2.5*spell_haste&buff.dragonrage.remains>=1.75*spell_haste)" );
  fb->add_action( "fire_breath,empower_to=3,target_if=max:target.health.pct,if=(talent.everburning_flame&buff.dragonrage.up&active_enemies>=5)|!talent.font_of_magic|(debuff.in_firestorm.up&talent.everburning_flame&active_enemies<=3)|(buff.dragonrage.remains<=3.25*spell_haste&buff.dragonrage.remains>=2.5*spell_haste)" );
  fb->add_action( "fire_breath,empower_to=4,target_if=max:target.health.pct" );

  green->add_action( "emerald_blossom", "Green Spells used to trigger Ancient Flame" );
  green->add_action( "verdant_embrace" );

  st->add_action( "use_item,name=kharnalex_the_first_light,if=!buff.dragonrage.up&debuff.shattering_star_debuff.down&raid_event.movement.in>6", "ST Action List, it's a mess, but it's getting better!" );
  st->add_action( "hover,use_off_gcd=1,if=raid_event.movement.in<2&!buff.hover.up", "Movement Logic, Time spiral logic might need some tweaking actions.st+=/time_spiral,if=raid_event.movement.in<3&cooldown.hover.remains>=3&!buff.hover.up" );
  st->add_action( "firestorm,if=buff.snapfire.up", "Spend firestorm procs ASAP" );
  st->add_action( "dragonrage,if=cooldown.fire_breath.remains<4&cooldown.eternity_surge.remains<10&target.time_to_die>=32|fight_remains<32", "Relaxed Dragonrage Entry Requirements, cannot reliably reach third extend under normal conditions (Bloodlust + Power Infusion/Very high haste gear) DS optimization: Only cast if current fight will last 32s+ or encounter ends in less than 30s" );
  st->add_action( "tip_the_scales,if=buff.dragonrage.up&(((!talent.font_of_magic|talent.everburning_flame)&cooldown.fire_breath.remains<cooldown.eternity_surge.remains&buff.dragonrage.remains<14)|(cooldown.eternity_surge.remains<cooldown.fire_breath.remains&!talent.everburning_flame&talent.font_of_magic))", "Tip second FB if not playing font of magic or if playing EBF, otherwise tip ES." );
  st->add_action( "call_action_list,name=fb,if=(!talent.dragonrage|variable.next_dragonrage>variable.dr_prep_time_st|!talent.animosity)&(buff.blazing_shards.remains<variable.r1_cast_time|buff.dragonrage.up)&(!cooldown.eternity_surge.up|!talent.event_horizon|!buff.dragonrage.up)&(target.time_to_die>=8|fight_remains<30)", "Fire breath logic. Play around blazing shards if outside of DR. DS optimization: Only cast if current fight will last 8s+ or encounter ends in less than 30s" );
  st->add_action( "shattering_star,if=(buff.essence_burst.stack<buff.essence_burst.max_stack|!talent.arcane_vigor)&(!cooldown.eternity_surge.up|!buff.dragonrage.up|!talent.event_horizon)", "Throw Star on CD, Don't overcap with Arcane Vigor." );
  st->add_action( "call_action_list,name=es,if=(!talent.dragonrage|variable.next_dragonrage>variable.dr_prep_time_st|!talent.animosity)&(buff.blazing_shards.remains<variable.r1_cast_time|buff.dragonrage.up)&(target.time_to_die>=8|fight_remains<30)", "Eternity Surge logic. Play around blazing shards if outside of DR. DS optimization: Only cast if current fight will last 8s+ or encounter ends in less than 30s" );
  st->add_action( "wait,sec=cooldown.fire_breath.remains,if=talent.animosity&buff.dragonrage.up&buff.dragonrage.remains<gcd.max+variable.r1_cast_time*buff.tip_the_scales.down&buff.dragonrage.remains-cooldown.fire_breath.remains>=variable.r1_cast_time*buff.tip_the_scales.down", "Wait for FB/ES to be ready if spending another GCD would result in the cast no longer fitting inside of DR" );
  st->add_action( "wait,sec=cooldown.eternity_surge.remains,if=talent.animosity&buff.dragonrage.up&buff.dragonrage.remains<gcd.max+variable.r1_cast_time&buff.dragonrage.remains-cooldown.eternity_surge.remains>variable.r1_cast_time*buff.tip_the_scales.down" );
  st->add_action( "living_flame,if=buff.dragonrage.up&buff.dragonrage.remains<(buff.essence_burst.max_stack-buff.essence_burst.stack)*gcd.max&buff.burnout.up", "Spend the last 1 or 2 GCDs of DR on fillers to exit with 2 EBs" );
  st->add_action( "azure_strike,if=buff.dragonrage.up&buff.dragonrage.remains<(buff.essence_burst.max_stack-buff.essence_burst.stack)*gcd.max" );
  st->add_action( "living_flame,if=buff.burnout.up&(buff.leaping_flames.up&!buff.essence_burst.up|!buff.leaping_flames.up&buff.essence_burst.stack<buff.essence_burst.max_stack)&essence.deficit>=2", "Spend burnout procs without overcapping resources" );
  st->add_action( "pyre,if=debuff.in_firestorm.up&talent.raging_inferno&buff.charged_blast.stack==20&active_enemies>=2", "Spend pyre if raging inferno debuff is active and you have 20 stacks of CB on 2T" );
  st->add_action( "disintegrate,chain=1,early_chain_if=evoker.use_early_chaining&ticks>=2&buff.dragonrage.up&(raid_event.movement.in>2|buff.hover.up),interrupt_if=evoker.use_clipping&buff.dragonrage.up&ticks>=2&(raid_event.movement.in>2|buff.hover.up),if=raid_event.movement.in>2|buff.hover.up", "Dis logic Early Chain if needed for resources management. Clip after in DR after third tick for more important buttons." );
  st->add_action( "firestorm,if=!buff.dragonrage.up&debuff.shattering_star_debuff.down", "Hard cast only outside of SS and DR windows" );
  st->add_action( "deep_breath,if=!buff.dragonrage.up&active_enemies>=2&((raid_event.adds.in>=120&!talent.onyx_legacy)|(raid_event.adds.in>=60&talent.onyx_legacy))", "Use Deep Breath on 2T, unless adds will come before it'll be ready again or if talented ID." );
  st->add_action( "deep_breath,if=!buff.dragonrage.up&talent.imminent_destruction&!debuff.shattering_star_debuff.up" );
  st->add_action( "call_action_list,name=green,if=talent.ancient_flame&!buff.ancient_flame.up&!buff.shattering_star_debuff.up&talent.scarlet_adaptation&!buff.dragonrage.up", "Get Ancient Flame as Filler" );
  st->add_action( "living_flame,if=!buff.dragonrage.up|(buff.iridescence_red.remains>execute_time|buff.iridescence_blue.up)&active_enemies==1", "Cast LF outside of DR, In DR only cast with Iridescence." );
  st->add_action( "azure_strike", "Fallback for movement" );

  trinkets->add_action( "use_item,name=dreambinder_loom_of_the_great_cycle,use_off_gcd=1,if=gcd.remains>0.5", "Use Dreambinder on CD" );
  trinkets->add_action( "use_item,target_if=min:target.health.pct,name=iridal_the_earths_master,use_off_gcd=1,if=gcd.remains>0.5" );
  trinkets->add_action( "use_item,name=nymues_unraveling_spindle,if=(buff.emerald_trance_stacking.stack>=2&variable.has_external_pi&talent.event_horizon|cooldown.dragonrage.remains<=3&cooldown.fire_breath.remains<7&cooldown.eternity_surge.remains<13&target.time_to_die>=35&(!variable.has_external_pi&active_enemies<=2|!talent.event_horizon|!set_bonus.tier31_2pc))|cooldown.dragonrage.remains<=3&active_enemies>=3|fight_remains<=20", "Nymues is used before Dragonrage unless you have PI and Event Horizon, then it is used on 2 Stacks of Emerald Trance. In AoE it is used before DR." );
  trinkets->add_action( "use_item,name=belorrelos_the_suncaller,if=(((trinket.2.cooldown.remains|!variable.trinket_2_buffs)&(trinket.1.cooldown.remains|!variable.trinket_1_buffs)|active_enemies<=2)&(trinket.nymues_unraveling_spindle.cooldown.remains|!equipped.nymues_unraveling_spindle))|fight_remains<=20", "Belorrelos is used on CD if not playing Nymues, it is used after Nymues if it is played. On AoE use after other use trinket." );
  trinkets->add_action( "use_item,slot=trinket1,if=buff.dragonrage.up&((buff.emerald_trance_stacking.stack>=4&set_bonus.tier31_2pc)|(variable.trinket_2_buffs&!cooldown.fire_breath.up&!cooldown.shattering_star.up&!equipped.nymues_unraveling_spindle&trinket.2.cooldown.remains)|(!cooldown.fire_breath.up&!cooldown.shattering_star.up&!set_bonus.tier31_2pc)|active_enemies>=3)&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1|variable.trinket_2_exclude)&!variable.trinket_1_manual|trinket.1.proc.any_dps.duration>=fight_remains|trinket.1.cooldown.duration<=60&(variable.next_dragonrage>20|!talent.dragonrage)&(!buff.dragonrage.up|variable.trinket_priority=1)", "The trinket with the highest estimated value, will be used first and paired with Dragonrage. Trinkets are used on 4 stacks of Emerald Trance, unless playing double buff trinket, then one is used after SS/FB and the next on CD. Or with DR in AoE" );
  trinkets->add_action( "use_item,slot=trinket2,if=buff.dragonrage.up&((buff.emerald_trance_stacking.stack>=4&set_bonus.tier31_2pc)|(variable.trinket_1_buffs&!cooldown.fire_breath.up&!cooldown.shattering_star.up&!equipped.nymues_unraveling_spindle&trinket.1.cooldown.remains)|(!cooldown.fire_breath.up&!cooldown.shattering_star.up&!set_bonus.tier31_2pc)|active_enemies>=3)&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2|variable.trinket_1_exclude)&!variable.trinket_2_manual|trinket.2.proc.any_dps.duration>=fight_remains|trinket.2.cooldown.duration<=60&(variable.next_dragonrage>20|!talent.dragonrage)&(!buff.dragonrage.up|variable.trinket_priority=2)" );
  trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs)&(variable.next_dragonrage>20|!talent.dragonrage)&!variable.trinket_1_manual", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs)&(variable.next_dragonrage>20|!talent.dragonrage)&!variable.trinket_2_manual" );
}
//devastation_apl_end

void preservation( player_t* /*p*/ )
{
}

//augmentation_apl_start
void augmentation( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* ebon_logic = p->get_action_priority_list( "ebon_logic" );
  action_priority_list_t* opener_filler = p->get_action_priority_list( "opener_filler" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* fb = p->get_action_priority_list( "fb" );
  action_priority_list_t* filler = p->get_action_priority_list( "filler" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=spam_heal,default=1,op=reset" );
  precombat->add_action( "variable,name=minimum_opener_delay,op=reset,default=0" );
  precombat->add_action( "variable,name=opener_delay,value=variable.minimum_opener_delay,if=!talent.interwoven_threads" );
  precombat->add_action( "variable,name=opener_delay,value=variable.minimum_opener_delay+variable.opener_delay,if=talent.interwoven_threads" );
  precombat->add_action( "variable,name=opener_cds_detected,op=reset,default=0" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell|trinket.1.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell|trinket.2.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.nymues_unraveling_spindle", "Nymues is complicated, Manual Handle" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.nymues_unraveling_spindle" );
  precombat->add_action( "variable,name=trinket_1_ogcd_cast,value=trinket.1.is.beacon_to_the_beyond|trinket.1.is.belorrelos_the_suncaller" );
  precombat->add_action( "variable,name=trinket_2_ogcd_cast,value=trinket.2.is.beacon_to_the_beyond|trinket.2.is.belorrelos_the_suncaller" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff|(trinket.1.has_buff.intellect|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit)&!variable.trinket_1_exclude" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff|(trinket.2.has_buff.intellect|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit)&!variable.trinket_2_exclude" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%120=0)" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%120=0)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs&(trinket.2.has_cooldown&!variable.trinket_2_exclude|!trinket.1.has_cooldown)|variable.trinket_2_buffs&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(0.5+trinket.2.has_buff.intellect*3+trinket.2.has_buff.mastery)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(0.5+trinket.1.has_buff.intellect*3+trinket.1.has_buff.mastery)*(variable.trinket_1_sync)*(1+((trinket.1.ilvl-trinket.2.ilvl)%100)))" );
  precombat->add_action( "variable,name=damage_trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs&trinket.2.ilvl>=trinket.1.ilvl" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=trinket.1.is.nymues_unraveling_spindle&trinket.2.has_buff.intellect|trinket.2.is.nymues_unraveling_spindle&!trinket.1.has_buff.intellect,if=(trinket.1.is.nymues_unraveling_spindle|trinket.2.is.nymues_unraveling_spindle)&(variable.trinket_1_buffs&variable.trinket_2_buffs)", "Double on use - Priotize Intellect on use trinkets over Nymues, force overwriting the normal logic to guarantee it is correct." );
  precombat->add_action( "blistering_scales,target_if=target.role.tank" );
  precombat->add_action( "living_flame" );

  ebon_logic->add_action( "ebon_might" );

  opener_filler->add_action( "variable,name=opener_delay,value=variable.opener_delay>?variable.minimum_opener_delay,if=!variable.opener_cds_detected&evoker.allied_cds_up>0" );
  opener_filler->add_action( "variable,name=opener_delay,value=variable.opener_delay-1" );
  opener_filler->add_action( "variable,name=opener_cds_detected,value=1,if=!variable.opener_cds_detected&evoker.allied_cds_up>0" );
  opener_filler->add_action( "living_flame,if=active_enemies=1|talent.pupil_of_alexstrasza" );
  opener_filler->add_action( "azure_strike" );

  items->add_action( "use_item,name=nymues_unraveling_spindle,if=cooldown.breath_of_eons.remains<=3&(trinket.1.is.nymues_unraveling_spindle&variable.trinket_priority=1|trinket.2.is.nymues_unraveling_spindle&variable.trinket_priority=2)|(cooldown.fire_breath.remains<=4|cooldown.upheaval.remains<=4)&cooldown.breath_of_eons.remains>10&!debuff.temporal_wound.up&(trinket.1.is.nymues_unraveling_spindle&variable.trinket_priority=2|trinket.2.is.nymues_unraveling_spindle&variable.trinket_priority=1)" );
  items->add_action( "use_item,slot=trinket1,if=variable.trinket_1_buffs&!variable.trinket_1_manual&(debuff.temporal_wound.up|variable.trinket_2_buffs&!trinket.2.cooldown.up&(prev_gcd.1.fire_breath|prev_gcd.1.upheaval)&buff.ebon_might_self.up)&(variable.trinket_2_exclude|!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains" );
  items->add_action( "use_item,slot=trinket2,if=variable.trinket_2_buffs&!variable.trinket_2_manual&(debuff.temporal_wound.up|variable.trinket_1_buffs&!trinket.1.cooldown.up&(prev_gcd.1.fire_breath|prev_gcd.1.upheaval)&buff.ebon_might_self.up)&(variable.trinket_1_exclude|!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains" );
  items->add_action( "azure_strike,if=cooldown.item_cd_1141.up&(variable.trinket_1_ogcd_cast&trinket.1.cooldown.up&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains)|variable.trinket_2_ogcd_cast&trinket.2.cooldown.up&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains))", "Azure Strike for OGCD trinkets. Ideally this would be Prescience casts in reality but this is simpler and seems to have no noticeable diferrence in DPS." );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains)&(gcd.remains>0.1|!variable.trinket_1_ogcd_cast)", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains)&(gcd.remains>0.1|!variable.trinket_2_ogcd_cast)" );
  items->add_action( "use_item,slot=main_hand,use_off_gcd=1,if=gcd.remains>=gcd.max*0.6", "Use on use weapons" );

  default_->add_action( "variable,name=temp_wound,value=debuff.temporal_wound.remains,target_if=max:debuff.temporal_wound.remains" );
  default_->add_action( "prescience,target_if=min:debuff.prescience.remains+1000*(target=self&active_allies>2)+1000*target.spec.augmentation,if=(full_recharge_time<=gcd.max*3|cooldown.ebon_might.remains<=gcd.max*3&(buff.ebon_might_self.remains-gcd.max*3)<=buff.ebon_might_self.duration*0.4|variable.temp_wound>=(gcd.max+action.eruption.cast_time)|fight_remains<=30)&(buff.trembling_earth.stack+evoker.prescience_buffs)<=(5+(full_recharge_time<=gcd.max*3))" );
  default_->add_action( "call_action_list,name=ebon_logic,if=(buff.ebon_might_self.remains-cast_time)<=buff.ebon_might_self.duration*0.4&(active_enemies>0|raid_event.adds.in<=3)&(evoker.prescience_buffs>=2&time<=10|evoker.prescience_buffs>=3|fight_style.dungeonroute|fight_style.dungeonslice|buff.ebon_might_self.remains>=action.ebon_might.cast_time|active_allies<=2)" );
  default_->add_action( "run_action_list,name=opener_filler,if=variable.opener_delay>0&!fight_style.dungeonroute" );
  default_->add_action( "potion,if=debuff.temporal_wound.up&buff.ebon_might_self.up" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "deep_breath" );
  default_->add_action( "call_action_list,name=fb,if=cooldown.time_skip.up&talent.time_skip&!talent.interwoven_threads" );
  default_->add_action( "upheaval,target_if=target.time_to_die>duration+0.2,empower_to=1,if=buff.ebon_might_self.remains>duration&cooldown.time_skip.up&talent.time_skip&!talent.interwoven_threads" );
  default_->add_action( "breath_of_eons,if=((cooldown.ebon_might.remains<=4|buff.ebon_might_self.up)&target.time_to_die>15&raid_event.adds.in>15&(!equipped.nymues_unraveling_spindle|trinket.nymues_unraveling_spindle.cooldown.remains>=10|fight_remains<30|trinket.1.is.nymues_unraveling_spindle&variable.trinket_priority=2|trinket.2.is.nymues_unraveling_spindle&variable.trinket_priority=1)|fight_remains<30)&!fight_style.dungeonroute,line_cd=117" );
  default_->add_action( "breath_of_eons,if=evoker.allied_cds_up>0&((cooldown.ebon_might.remains<=4|buff.ebon_might_self.up)&target.time_to_die>15&(!equipped.nymues_unraveling_spindle|trinket.nymues_unraveling_spindle.cooldown.remains>=10|fight_remains<30|trinket.1.is.nymues_unraveling_spindle&variable.trinket_priority=2|trinket.2.is.nymues_unraveling_spindle&variable.trinket_priority=1)|fight_remains<30)&fight_style.dungeonroute" );
  default_->add_action( "living_flame,if=buff.leaping_flames.up&cooldown.fire_breath.up&fight_style.dungeonroute" );
  default_->add_action( "living_flame,if=cooldown.breath_of_eons.up&evoker.allied_cds_up=0&target.time_to_die>15&fight_style.dungeonroute" );
  default_->add_action( "call_action_list,name=fb,if=(raid_event.adds.remains>13|raid_event.adds.in>20|evoker.allied_cds_up>0|!raid_event.adds.exists)" );
  default_->add_action( "upheaval,target_if=target.time_to_die>duration+0.2,empower_to=1,if=buff.ebon_might_self.remains>duration&(raid_event.adds.remains>13|!raid_event.adds.exists|raid_event.adds.in>20)" );
  default_->add_action( "time_skip,if=(cooldown.fire_breath.remains+cooldown.upheaval.remains+cooldown.prescience.full_recharge_time)>=35" );
  default_->add_action( "emerald_blossom,if=talent.dream_of_spring&buff.essence_burst.up&(variable.spam_heal=2|variable.spam_heal=1&!buff.ancient_flame.up)&(buff.ebon_might_self.up|essence.deficit=0|buff.essence_burst.stack=buff.essence_burst.max_stack&cooldown.ebon_might.remains>4)" );
  default_->add_action( "eruption,if=buff.ebon_might_self.remains>execute_time|essence.deficit=0|buff.essence_burst.stack=buff.essence_burst.max_stack&cooldown.ebon_might.remains>4" );
  default_->add_action( "blistering_scales,target_if=target.role.tank,if=!evoker.scales_up&buff.ebon_might_self.down" );
  default_->add_action( "emerald_blossom,if=!buff.ebon_might_self.up&talent.ancient_flame&talent.scarlet_adaptation&!talent.dream_of_spring&!buff.ancient_flame.up&active_enemies=1" );
  default_->add_action( "verdant_embrace,if=!buff.ebon_might_self.up&talent.ancient_flame&talent.scarlet_adaptation&!buff.ancient_flame.up&(!talent.dream_of_spring|mana>=200000)&active_enemies=1" );
  default_->add_action( "run_action_list,name=filler" );

  fb->add_action( "tip_the_scales,if=cooldown.fire_breath.ready&buff.ebon_might_self.up" );
  fb->add_action( "fire_breath,empower_to=1,target_if=target.time_to_die>16,if=buff.ebon_might_self.remains>duration&equipped.neltharions_call_to_chaos" );
  fb->add_action( "fire_breath,empower_to=2,target_if=target.time_to_die>12,if=buff.ebon_might_self.remains>duration&equipped.neltharions_call_to_chaos" );
  fb->add_action( "fire_breath,empower_to=3,target_if=target.time_to_die>8,if=buff.ebon_might_self.remains>duration&equipped.neltharions_call_to_chaos" );
  fb->add_action( "fire_breath,empower_to=4,target_if=target.time_to_die>4,if=talent.font_of_magic&(buff.ebon_might_self.remains>duration|buff.tip_the_scales.up)" );
  fb->add_action( "fire_breath,empower_to=3,target_if=target.time_to_die>8,if=(buff.ebon_might_self.remains>duration|buff.tip_the_scales.up)&!equipped.neltharions_call_to_chaos" );
  fb->add_action( "fire_breath,empower_to=2,target_if=target.time_to_die>12,if=buff.ebon_might_self.remains>duration&!equipped.neltharions_call_to_chaos" );
  fb->add_action( "fire_breath,empower_to=1,target_if=target.time_to_die>16,if=buff.ebon_might_self.remains>duration&!equipped.neltharions_call_to_chaos" );

  filler->add_action( "living_flame,if=(buff.ancient_flame.up|mana>=200000|!talent.dream_of_spring|variable.spam_heal=0)&(active_enemies=1|talent.pupil_of_alexstrasza)" );
  filler->add_action( "azure_strike" );
}
//augmentation_apl_end

void no_spec( player_t* /*p*/ )
{
}

}  // namespace evoker_apl
