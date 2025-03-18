#include "class_modules/apl/apl_evoker.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace evoker_apl
{

std::string potion( const player_t* p )
{
  return ( p->true_level > 79 ) ? "tempered_potion_3" : "elemental_potion_of_ultimate_power_3";
}

std::string flask( const player_t* p )
{
  switch ( p->specialization() )
  {
    case EVOKER_AUGMENTATION:
      return ( p->true_level > 79 ) ? "flask_of_tempered_mastery_3" : "iced_phial_of_corrupting_rage_3";
    default:
      return ( p->true_level > 79 ) ? "flask_of_alchemical_chaos_3" : "iced_phial_of_corrupting_rage_3";
  }
}

std::string food( const player_t* p )
{
  return ( p->true_level > 79 ) ? "feast_of_the_divine_day" : "fated_fortune_cookie";
}

std::string rune( const player_t* p )
{
  return ( p->true_level > 79 ) ? "crystallized" : "draconic";
}

std::string temporary_enchant( const player_t* p )
{
  switch ( p->specialization() )
  {
    case EVOKER_AUGMENTATION:
      return ( p->true_level > 79 ) ? "main_hand:algari_mana_oil_3" : "main_hand:hissing_rune_3";
    default:
      return ( p->true_level > 79 ) ? "main_hand:algari_mana_oil_3" : "main_hand:buzzing_rune_3";
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

  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_buff.intellect|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit|trinket.1.is.mirror_of_fractured_tomorrows" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_buff.intellect|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit|trinket.2.is.mirror_of_fractured_tomorrows" );
  precombat->add_action( "variable,name=weapon_buffs,value=equipped.bestinslots" );
  precombat->add_action( "variable,name=weapon_sync,op=setif,value=1,value_else=0.5,condition=equipped.bestinslots" );
  precombat->add_action( "variable,name=weapon_stat_value,value=equipped.bestinslots*5142*15", "Mythic one hardcoded - To Do implement something in simc to just get the value." );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%cooldown.dragonrage.duration=0|cooldown.dragonrage.duration%%trinket.1.cooldown.duration=0|trinket.1.is.house_of_cards)", "Decide which trinket to pair with Dragonrage, prefer 2 minute and 1 minute trinkets" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%cooldown.dragonrage.duration=0|cooldown.dragonrage.duration%%trinket.2.cooldown.duration=0|trinket.2.is.house_of_cards)" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.belorrelos_the_suncaller|trinket.1.is.nymues_unraveling_spindle|trinket.1.is.spymasters_web", "Estimates a trinkets value by comparing the cooldown of the trinket, divided by the duration of the buff it provides. Has a intellect modifier (currently 1.5x) to give a higher priority to intellect trinkets. The intellect modifier should be changed as intellect priority increases or decreases. As well as a modifier for if a trinket will or will not sync with cooldowns." );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.belorrelos_the_suncaller|trinket.2.is.nymues_unraveling_spindle|trinket.2.is.spymasters_web" );
  precombat->add_action( "variable,name=trinket_1_ogcd_cast,value=0" );
  precombat->add_action( "variable,name=trinket_2_ogcd_cast,value=0" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell|trinket.1.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell|trinket.2.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.proc.any_dps.duration)*(variable.trinket_2_sync)*trinket.2.proc.any_dps.default_value)>((trinket.1.proc.any_dps.duration)*(variable.trinket_1_sync)*trinket.1.proc.any_dps.default_value)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,if=variable.weapon_buffs,value=3,value_else=variable.trinket_priority,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs|variable.weapon_stat_value*variable.weapon_sync>(((trinket.2.proc.any_dps.duration)*(variable.trinket_2_sync)*trinket.2.proc.any_dps.default_value)<?((trinket.1.proc.any_dps.duration)*(variable.trinket_1_sync)*trinket.1.proc.any_dps.default_value))" );
  precombat->add_action( "variable,name=damage_trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs&trinket.2.ilvl>=trinket.1.ilvl" );
  precombat->add_action( "variable,name=r1_cast_time,value=1.0*spell_haste", "Rank 1 empower spell cast time" );
  precombat->add_action( "variable,name=dr_prep_time_aoe,default=4,op=reset", "Variable for when to start holding empowers for upcoming DR in AoE. - From my testing 4sec seems like the sweetspot, but it's very minor diff so far - Holding for more than 6 seconds it begins to become a loss." );
  precombat->add_action( "variable,name=pool_for_cb,default=0,op=reset" );
  precombat->add_action( "variable,name=dr_prep_time_st,default=8,op=reset", "Variable for when to start holding empowers for upcoming DR in ST." );
  precombat->add_action( "variable,name=can_extend_dr,default=0,op=reset" );
  precombat->add_action( "variable,name=has_external_pi,value=cooldown.invoke_power_infusion_0.duration>0" );
  precombat->add_action( "variable,name=pyre_st,default=0,op=reset" );
  precombat->add_action( "verdant_embrace,if=talent.scarlet_adaptation", "Get Some Scarlet Adaptation Prepull" );
  precombat->add_action( "firestorm,if=talent.firestorm&(!talent.engulf|!talent.ruby_embers)" );
  precombat->add_action( "living_flame,if=!talent.firestorm|talent.engulf&talent.ruby_embers" );

  default_->add_action( "potion,if=(!talent.dragonrage|buff.dragonrage.up)&(!cooldown.shattering_star.up|debuff.shattering_star_debuff.up|active_enemies>=2)|fight_remains<35", "Delay pot in ST if you are about to SS - mostly relevant for opener where you want DR->FB->SS->rotation" );
  default_->add_action( "variable,name=next_dragonrage,value=cooldown.dragonrage.remains<?((cooldown.eternity_surge.remains-8)>?(cooldown.fire_breath.remains-8))", "Variable that evaluates when next dragonrage is by working out the maximum between the dragonrage cd and your empowers, ignoring CDR effect estimates." );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=buff.dragonrage.up&(!cooldown.shattering_star.up|debuff.shattering_star_debuff.up|active_enemies>=2)|fight_remains<35", "Invoke External Power Infusions if they're available during dragonrage" );
  default_->add_action( "variable,name=pool_for_id,if=talent.imminent_destruction,default=0,op=set,value=cooldown.deep_breath.remains<7&essence.deficit>=1&!buff.essence_burst.up&(raid_event.adds.in>=action.deep_breath.cooldown*0.4|talent.melt_armor&talent.maneuverability|active_enemies>=3)" );
  default_->add_action( "variable,name=can_extend_dr,if=talent.animosity,op=set,value=buff.dragonrage.up&(buff.dragonrage.duration+dbc.effect.1160688.base_value%1000-buff.dragonrage.elapsed-buff.dragonrage.remains)>0" );
  default_->add_action( "quell,use_off_gcd=1,if=target.debuff.casting.react", "actions+=/variable,name=pool_for_cb,if=!talent.blast_furnace&talent.scorching_embers&talent.charged_blast&talent.iridescence&talent.engulf,default=0,op=set,value=cooldown.fire_breath.remains<=3&action.engulf.usable_in>5&essence.deficit>=1&buff.essence_burst.stack<buff.essence_burst.max_stack&buff.charged_blast.stack=20  Rupt to make the raidleader happy" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "run_action_list,name=aoe,if=active_enemies>=3" );
  default_->add_action( "run_action_list,name=st" );

  aoe->add_action( "shattering_star,target_if=max:target.health.pct,if=cooldown.dragonrage.up&talent.arcane_vigor|talent.eternitys_span&active_enemies<=3", "AOE action list; Open with star before DR to save a global and start with a free EB" );
  aoe->add_action( "hover,use_off_gcd=1,if=raid_event.movement.in<6&!buff.hover.up&gcd.remains>=0.5&(buff.mass_disintegrate_stacks.up&talent.mass_disintegrate|active_enemies<=4)" );
  aoe->add_action( "firestorm,if=buff.snapfire.up&!talent.feed_the_flames", "Spend firestorm procs ASAP" );
  aoe->add_action( "deep_breath,if=talent.maneuverability&talent.melt_armor&!cooldown.fire_breath.up&!cooldown.eternity_surge.up" );
  aoe->add_action( "firestorm,if=talent.feed_the_flames", "Acquire the buff" );
  aoe->add_action( "call_action_list,name=fb,if=talent.dragonrage&cooldown.dragonrage.up&(talent.iridescence|talent.scorching_embers)&!talent.engulf", "Grab Irid Red before Dragonrage without griefing extension" );
  aoe->add_action( "tip_the_scales,if=(!talent.dragonrage|buff.dragonrage.up)&(cooldown.fire_breath.remains<=cooldown.eternity_surge.remains|(cooldown.eternity_surge.remains<=cooldown.fire_breath.remains&talent.font_of_magic)&!talent.engulf)", "Tip ES at appropiate target count or when playing Flameshaper otherwise Tip FB" );
  aoe->add_action( "dragonrage,target_if=max:target.time_to_die,if=target.time_to_die>=32|active_enemies>=3&target.time_to_die>=15|fight_remains<30" );
  aoe->add_action( "call_action_list,name=fb,if=(!talent.dragonrage|buff.dragonrage.up|cooldown.dragonrage.remains>variable.dr_prep_time_aoe|!talent.animosity|talent.flame_siphon)&(target.time_to_die>=8|talent.mass_disintegrate)", "Cast Fire Breath DS optimization: Only cast if current fight will last 8s+ or encounter ends in less than 30s" );
  aoe->add_action( "call_action_list,name=es,if=(!talent.dragonrage|buff.dragonrage.up|cooldown.dragonrage.remains>variable.dr_prep_time_aoe|!talent.animosity)&(!buff.jackpot.up|!set_bonus.tww2_4pc|talent.mass_disintegrate)", "Cast Eternity Surge DS optimization: Only cast if current fight will last 8s+ or encounter ends in less than 30s" );
  aoe->add_action( "deep_breath,if=!buff.dragonrage.up&essence.deficit>3", "Cast DB if not in DR and not going to overflow essence." );
  aoe->add_action( "shattering_star,target_if=max:target.health.pct,if=buff.essence_burst.stack<buff.essence_burst.max_stack&talent.arcane_vigor|talent.eternitys_span&active_enemies<=3|set_bonus.tww2_4pc&buff.jackpot.stack<2", "Send SS when it doesn't overflow EB, without vigor send on CD" );
  aoe->add_action( "engulf,target_if=max:(((dot.fire_breath_damage.remains-dbc.effect.1140380.base_value*action.engulf_damage.in_flight_to_target-action.engulf_damage.travel_time)>0)*3+dot.living_flame_damage.ticking+dot.enkindle.ticking),if=(dot.fire_breath_damage.remains>=action.engulf_damage.travel_time+dbc.effect.1140380.base_value*action.engulf_damage.in_flight_to_target)&(variable.next_dragonrage>=cooldown*1.2|!talent.dragonrage)" );
  aoe->add_action( "pyre,target_if=max:target.health.pct,if=buff.charged_blast.stack>=12&(cooldown.dragonrage.remains>gcd.max*4|!talent.dragonrage)" );
  aoe->add_action( "disintegrate,target_if=min:debuff.bombardments.remains,if=buff.mass_disintegrate_stacks.up&talent.mass_disintegrate&(!variable.pool_for_id|buff.mass_disintegrate_stacks.remains<=buff.mass_disintegrate_stacks.stack*(duration+0.1))", "Use Mass Disintegrate if CB wont't overcap" );
  aoe->add_action( "deep_breath,if=talent.imminent_destruction&!buff.essence_burst.up", "Use Deep Breath on 2T, unless adds will come before it'll be ready again or if talented ID." );
  aoe->add_action( "pyre,target_if=max:target.health.pct,if=(active_enemies>=4-(buff.imminent_destruction.up)|talent.volatility|talent.scorching_embers&active_dot.fire_breath_damage>=active_enemies*0.75)&(cooldown.dragonrage.remains>gcd.max*4|!talent.dragonrage|!talent.charged_blast)&!variable.pool_for_id&(!buff.mass_disintegrate_stacks.up|buff.essence_burst.stack=2|buff.essence_burst.stack=1&essence>=(3-buff.imminent_destruction.up)|essence>=(5-buff.imminent_destruction.up*2))", "Pyre 4T+ - 3T+ with Volatility - 12 stacks of CB - Pool CB for DR" );
  aoe->add_action( "living_flame,target_if=max:target.health.pct,if=(!talent.burnout|buff.burnout.up|cooldown.fire_breath.remains<=gcd.max*5|buff.scarlet_adaptation.up|buff.ancient_flame.up)&buff.leaping_flames.up&(!buff.essence_burst.up&essence.deficit>1|cooldown.fire_breath.remains<=gcd.max*3&buff.essence_burst.stack<buff.essence_burst.max_stack)", "Cast LF with leaping flames up if: not playing burnout, burnout is up or the next firebreath is soon." );
  aoe->add_action( "disintegrate,target_if=max:target.health.pct,chain=1,early_chain_if=evoker.use_early_chaining&ticks>=2&(raid_event.movement.in>2|buff.hover.up),interrupt_if=evoker.use_clipping&buff.dragonrage.up&ticks>=2&(raid_event.movement.in>2|buff.hover.up),if=(raid_event.movement.in>2|buff.hover.up)&!variable.pool_for_id&(active_enemies<=4|buff.mass_disintegrate_stacks.up)", "Yoinked the disintegrate logic from ST" );
  aoe->add_action( "living_flame,target_if=max:target.health.pct,if=talent.snapfire&buff.burnout.up", "Cast LF with burnout to fish for snapfire procs" );
  aoe->add_action( "firestorm" );
  aoe->add_action( "living_flame,if=talent.snapfire&!talent.engulfing_blaze", "Get Ancient Flame as Filler  actions.aoe+=/call_action_list,name=green,if=talent.ancient_flame&!buff.ancient_flame.up&!buff.dragonrage.up" );
  aoe->add_action( "azure_strike,target_if=max:target.health.pct", "Fallback filler" );

  es->add_action( "eternity_surge,empower_to=1,target_if=max:target.health.pct,if=active_enemies<=1+talent.eternitys_span|(variable.can_extend_dr&talent.animosity|talent.mass_disintegrate)&active_enemies>(3+talent.font_of_magic+4*talent.eternitys_span)|buff.dragonrage.remains<1.75*spell_haste&buff.dragonrage.remains>=1*spell_haste&talent.animosity&variable.can_extend_dr", "Eternity Surge, use rank most applicable to targets." );
  es->add_action( "eternity_surge,empower_to=2,target_if=max:target.health.pct,if=active_enemies<=2+2*talent.eternitys_span|buff.dragonrage.remains<2.5*spell_haste&buff.dragonrage.remains>=1.75*spell_haste&talent.animosity&variable.can_extend_dr" );
  es->add_action( "eternity_surge,empower_to=3,target_if=max:target.health.pct,if=active_enemies<=3+3*talent.eternitys_span|!talent.font_of_magic&talent.mass_disintegrate|buff.dragonrage.remains<=3.25*spell_haste&buff.dragonrage.remains>=2.5*spell_haste&talent.animosity&variable.can_extend_dr" );
  es->add_action( "eternity_surge,empower_to=4,target_if=max:target.health.pct,if=talent.mass_disintegrate|active_enemies<=4+4*talent.eternitys_span" );

  fb->add_action( "fire_breath,empower_to=1,target_if=max:target.health.pct,if=(buff.dragonrage.remains<1.75*spell_haste&buff.dragonrage.remains>=1*spell_haste)&talent.animosity&variable.can_extend_dr|active_enemies=1", "Fire Breath, use rank appropriate to target count/talents." );
  fb->add_action( "fire_breath,empower_to=2,target_if=max:target.health.pct,if=(buff.dragonrage.remains<2.5*spell_haste&buff.dragonrage.remains>=1.75*spell_haste)&talent.animosity&variable.can_extend_dr|talent.scorching_embers|active_enemies>=2" );
  fb->add_action( "fire_breath,empower_to=3,target_if=max:target.health.pct,if=!talent.font_of_magic|(buff.dragonrage.remains<=3.25*spell_haste&buff.dragonrage.remains>=2.5*spell_haste)&talent.animosity&variable.can_extend_dr|talent.scorching_embers" );
  fb->add_action( "fire_breath,empower_to=4,target_if=max:target.health.pct" );

  green->add_action( "emerald_blossom", "Green Spells used to trigger Ancient Flame" );
  green->add_action( "verdant_embrace" );

  st->add_action( "use_item,name=kharnalex_the_first_light,if=!buff.dragonrage.up&debuff.shattering_star_debuff.down&raid_event.movement.in>6", "ST Action List, it's a mess, but it's getting better!" );
  st->add_action( "hover,use_off_gcd=1,if=raid_event.movement.in<6&!buff.hover.up&gcd.remains>=0.5", "Movement Logic, Time spiral logic might need some tweaking" );
  st->add_action( "tip_the_scales,if=(!talent.dragonrage|buff.dragonrage.up)&(cooldown.fire_breath.remains<=cooldown.eternity_surge.remains|(cooldown.eternity_surge.remains<=cooldown.fire_breath.remains&talent.font_of_magic)&!talent.engulf)", "Tip second FB if not playing font of magic, otherwise tip ES." );
  st->add_action( "deep_breath,if=talent.maneuverability&talent.melt_armor" );
  st->add_action( "dragonrage,if=(cooldown.fire_breath.remains<4|talent.flame_siphon&talent.causality|cooldown.eternity_surge.remains<4&(!set_bonus.tww1_4pc|!talent.mass_disintegrate))&((cooldown.fire_breath.remains<8|talent.flame_siphon&talent.causality|!talent.animosity)&(cooldown.eternity_surge.remains<8|set_bonus.tww1_4pc&talent.mass_disintegrate|!talent.animosity))&target.time_to_die>=32|fight_remains<32" );
  st->add_action( "call_action_list,name=es,if=buff.dragonrage.up&talent.animosity&talent.engulf&set_bonus.tww2_4pc&!buff.jackpot.up&variable.can_extend_dr&!cooldown.engulf.up" );
  st->add_action( "shattering_star,if=(buff.essence_burst.stack<buff.essence_burst.max_stack|!talent.arcane_vigor)", "Throw Star on CD, Don't overcap with Arcane Vigor." );
  st->add_action( "engulf,target_if=max:(dot.fire_breath_damage.remains-dbc.effect.1140380.base_value*action.engulf_damage.in_flight_to_target),if=(dot.fire_breath_damage.remains>=action.engulf_damage.travel_time+dbc.effect.1140380.base_value*action.engulf_damage.in_flight_to_target)&(!talent.enkindle|dot.enkindle.ticking)&(!talent.ruby_embers|dot.living_flame_damage.ticking)&(!talent.shattering_star&!talent.iridescence|debuff.shattering_star_debuff.up&(!talent.iridescence|full_recharge_time<=cooldown.fire_breath.remains+4|buff.dragonrage.up)|buff.iridescence_red.up&(debuff.shattering_star_debuff.up|!talent.shattering_star|full_recharge_time<=cooldown.shattering_star.remains)|talent.scorching_embers&dot.fire_breath_damage.duration<=10&dot.fire_breath_damage.remains<=5)&(variable.next_dragonrage>=cooldown*1.2|!talent.dragonrage|full_recharge_time<=variable.next_dragonrage)&(cooldown.tip_the_scales.remains>=4|cooldown.fire_breath.remains>=4|!talent.scorching_embers|!talent.tip_the_scales)|fight_remains<=10" );
  st->add_action( "call_action_list,name=fb,if=(!talent.dragonrage|variable.next_dragonrage>variable.dr_prep_time_st|!talent.animosity)&(!cooldown.eternity_surge.up|!talent.event_horizon|talent.mass_disintegrate|!buff.dragonrage.up|talent.flame_siphon&talent.causality|set_bonus.tww2_4pc)&(target.time_to_die>=8|!talent.mass_disintegrate)&((debuff.shattering_star_debuff.up|!cooldown.shattering_star.up)&buff.dragonrage.up&buff.tip_the_scales.up|!buff.tip_the_scales.up|!talent.dragonrage|!talent.animosity|!buff.dragonrage.up|!talent.engulf)", "Fire breath logic. Play around blazing shards if outside of DR. DS optimization: Only cast if current fight will last 8s+ or encounter ends in less than 30s" );
  st->add_action( "deep_breath,if=(talent.imminent_destruction&!debuff.shattering_star_debuff.up|talent.melt_armor&talent.maneuverability)&(talent.melt_armor&talent.maneuverability|!buff.dragonrage.up)" );
  st->add_action( "call_action_list,name=es,if=(!talent.dragonrage|variable.next_dragonrage>variable.dr_prep_time_st|!talent.animosity|set_bonus.tww1_4pc&talent.mass_disintegrate)&(!set_bonus.tww2_4pc|!buff.jackpot.up|talent.mass_disintegrate)&(!talent.power_swell|buff.power_swell.remains<=gcd.max)&(!buff.jackpot.up|talent.mass_disintegrate)", "Eternity Surge logic. Play around blazing shards if outside of DR. DS optimization: Only cast if current fight will last 8s+ or encounter ends in less than 30s" );
  st->add_action( "wait,sec=cooldown.fire_breath.remains,if=variable.can_extend_dr&talent.animosity&buff.dragonrage.up&buff.dragonrage.remains<gcd.max+variable.r1_cast_time*buff.tip_the_scales.down&buff.dragonrage.remains-cooldown.fire_breath.remains>=variable.r1_cast_time*buff.tip_the_scales.down", "Wait for FB/ES to be ready if spending another GCD would result in the cast no longer fitting inside of DR" );
  st->add_action( "wait,sec=cooldown.eternity_surge.remains,if=variable.can_extend_dr&talent.animosity&buff.dragonrage.up&buff.dragonrage.remains<gcd.max+variable.r1_cast_time&buff.dragonrage.remains-cooldown.eternity_surge.remains>variable.r1_cast_time*buff.tip_the_scales.down" );
  st->add_action( "living_flame,if=buff.dragonrage.up&buff.dragonrage.remains<(buff.essence_burst.max_stack-buff.essence_burst.stack)*gcd.max&buff.burnout.up", "Spend the last 1 or 2 GCDs of DR on fillers to exit with 2 EBs" );
  st->add_action( "azure_strike,if=buff.dragonrage.up&buff.dragonrage.remains<(buff.essence_burst.max_stack-buff.essence_burst.stack)*gcd.max" );
  st->add_action( "firestorm,if=buff.snapfire.up" );
  st->add_action( "living_flame,if=(buff.burnout.up|talent.flame_siphon&cooldown.fire_breath.remains<=gcd.max*3)&buff.leaping_flames.up&!buff.essence_burst.up&(essence.deficit>=1|cooldown.fire_breath.remains<=gcd.max*3)", "Spend burnout procs without overcapping resources" );
  st->add_action( "living_flame,if=talent.ruby_embers&talent.engulf&(buff.burnout.up&dot.living_flame_damage.remains<=gcd.max*3|dot.living_flame_damage.remains<=gcd.max)&!action.living_flame_damage.in_flight_to_target&(cooldown.engulf.up|cooldown.engulf.recharge_time<gcd.max*3)" );
  st->add_action( "pyre,if=debuff.in_firestorm.up&talent.feed_the_flames&buff.charged_blast.stack=20&active_enemies>=2", "Spend pyre if raging inferno debuff is active and you have 20 stacks of CB on 2T" );
  st->add_action( "call_action_list,name=es,if=(!talent.dragonrage|variable.next_dragonrage>variable.dr_prep_time_st|!talent.animosity|set_bonus.tww1_4pc&talent.mass_disintegrate)&(!set_bonus.tww2_4pc|!buff.jackpot.up|talent.mass_disintegrate)" );
  st->add_action( "disintegrate,target_if=min:debuff.bombardments.remains,early_chain_if=ticks_remain<=1&buff.mass_disintegrate_stacks.up,if=(raid_event.movement.in>2|buff.hover.up)&buff.mass_disintegrate_stacks.up&talent.mass_disintegrate&!variable.pool_for_id", "Mass Disintegrates" );
  st->add_action( "deep_breath,if=!buff.dragonrage.up&active_enemies>=2&((raid_event.adds.in>=120&!talent.onyx_legacy)|(raid_event.adds.in>=60&talent.onyx_legacy))", "Use Deep Breath on 2T, unless adds will come before it'll be ready again or if talented ID." );
  st->add_action( "deep_breath,if=!buff.dragonrage.up&(talent.imminent_destruction&!debuff.shattering_star_debuff.up|talent.melt_armor|talent.maneuverability)" );
  st->add_action( "deep_breath,if=talent.imminent_destruction&raid_event.adds.in>=cooldown*0.4&!buff.essence_burst.up" );
  st->add_action( "pyre,if=(variable.pyre_st|active_enemies>1&talent.snapfire)&!variable.pool_for_id" );
  st->add_action( "disintegrate,target_if=min:buff.bombardments.remains,chain=1,early_chain_if=evoker.use_early_chaining&ticks>=2&(raid_event.movement.in>2|buff.hover.up)&(buff.dragonrage.up|set_bonus.tww1_4pc),interrupt_if=evoker.use_clipping&ticks>=2&(raid_event.movement.in>2|buff.hover.up)&(buff.dragonrage.up|set_bonus.tww1_4pc),if=(raid_event.movement.in>2|buff.hover.up)&!variable.pool_for_id&!variable.pool_for_cb&!variable.pyre_st", "Dis logic Early Chain if needed for resources management. Clip after in DR after third tick for more important buttons." );
  st->add_action( "firestorm,if=active_enemies>1", "Spend firestorm procs ASAP" );
  st->add_action( "call_action_list,name=green,if=talent.ancient_flame&!buff.ancient_flame.up&!buff.shattering_star_debuff.up&talent.scarlet_adaptation&!buff.dragonrage.up&!buff.burnout.up&talent.engulfing_blaze", "Get Ancient Flame as Filler" );
  st->add_action( "living_flame,if=!buff.dragonrage.up|(buff.iridescence_red.remains>execute_time|!talent.engulfing_blaze|buff.iridescence_blue.up|buff.burnout.up|buff.leaping_flames.up&cooldown.fire_breath.remains<=5)&active_enemies=1", "Cast LF outside of DR, In DR only cast with Iridescence." );
  st->add_action( "azure_strike", "Fallback for movement" );

  trinkets->add_action( "use_item,name=spymasters_web,if=(buff.dragonrage.up|!talent.dragonrage&(talent.imminent_destruction&buff.imminent_destruction.up|!talent.imminent_destruction&!talent.melt_armor|talent.melt_armor&debuff.melt_armor.up))&(fight_remains<130|buff.bloodlust.react)&buff.spymasters_report.stack>=15|(fight_remains<=20|cooldown.engulf.up&talent.engulf&fight_remains<=40&cooldown.dragonrage.remains>=40)" );
  trinkets->add_action( "use_item,name=neural_synapse_enhancer,if=buff.dragonrage.up|!talent.dragonrage|cooldown.dragonrage.remains>=5" );
  trinkets->add_action( "use_item,slot=trinket1,if=buff.dragonrage.up&((variable.trinket_2_buffs&!cooldown.fire_breath.up&!cooldown.shattering_star.up&trinket.2.cooldown.remains)|buff.tip_the_scales.up&(!cooldown.shattering_star.up|talent.engulf)&variable.trinket_priority=1|(!cooldown.fire_breath.up&!cooldown.shattering_star.up)|active_enemies>=3)&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1|variable.trinket_2_exclude)&!variable.trinket_1_manual|trinket.1.proc.any_dps.duration>=fight_remains|trinket.1.cooldown.duration<=60&(variable.next_dragonrage>20|!talent.dragonrage)&(!buff.dragonrage.up|variable.trinket_priority=1)&!variable.trinket_1_manual", "The trinket with the highest estimated value, will be used first and paired with Dragonrage. Trinkets are used on 4 stacks of Emerald Trance, unless playing double buff trinket, then one is used after SS/FB and the next on CD. Or with DR in AoE" );
  trinkets->add_action( "use_item,slot=trinket2,if=buff.dragonrage.up&((variable.trinket_1_buffs&!cooldown.fire_breath.up&!cooldown.shattering_star.up&trinket.1.cooldown.remains)|buff.tip_the_scales.up&(!cooldown.shattering_star.up|talent.engulf)&variable.trinket_priority=2|(!cooldown.fire_breath.up&!cooldown.shattering_star.up)|active_enemies>=3)&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2|variable.trinket_1_exclude)&!variable.trinket_2_manual|trinket.2.proc.any_dps.duration>=fight_remains|trinket.2.cooldown.duration<=60&(variable.next_dragonrage>20|!talent.dragonrage)&(!buff.dragonrage.up|variable.trinket_priority=2)&!variable.trinket_2_manual" );
  trinkets->add_action( "use_item,slot=main_hand,if=variable.weapon_buffs&((variable.trinket_2_buffs&(trinket.2.cooldown.remains|trinket.2.cooldown.duration<=20)|!variable.trinket_2_buffs|variable.trinket_2_exclude|variable.trinket_priority=3)&(variable.trinket_1_buffs&(trinket.1.cooldown.remains|trinket.1.cooldown.duration<=20)|!variable.trinket_1_buffs|variable.trinket_1_exclude|variable.trinket_priority=3)&(!cooldown.fire_breath.up&!cooldown.shattering_star.up|buff.tip_the_scales.up&(!cooldown.shattering_star.up|talent.engulf)|(!cooldown.fire_breath.up&!cooldown.shattering_star.up)|active_enemies>=3))&(variable.next_dragonrage>20|!talent.dragonrage)&(!buff.dragonrage.up|variable.trinket_priority=3|variable.trinket_priority=1&trinket.1.cooldown.remains|variable.trinket_priority=2&trinket.2.cooldown.remains)" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains|trinket.2.is.spymasters_web|trinket.2.cooldown.duration=0)&(gcd.remains>0.1&!prev_gcd.1.deep_breath)&(variable.next_dragonrage>20|!talent.dragonrage|!variable.trinket_2_buffs|trinket.2.is.spymasters_web&(buff.spymasters_report.stack<5|fight_remains>=130+variable.next_dragonrage))", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains|trinket.1.is.spymasters_web|trinket.1.cooldown.duration=0)&(gcd.remains>0.1&!prev_gcd.1.deep_breath)&(variable.next_dragonrage>20|!talent.dragonrage|!variable.trinket_1_buffs|trinket.1.is.spymasters_web&(buff.spymasters_report.stack<5|fight_remains>=130+variable.next_dragonrage))" );
  trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains|trinket.2.is.spymasters_web|trinket.2.cooldown.duration=0)&(!variable.trinket_1_ogcd_cast)&(variable.next_dragonrage>20|!talent.dragonrage|!variable.trinket_2_buffs|trinket.2.is.spymasters_web&(buff.spymasters_report.stack<5|fight_remains>=130+variable.next_dragonrage))" );
  trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains|trinket.1.is.spymasters_web|trinket.1.cooldown.duration=0)&(!variable.trinket_2_ogcd_cast)&(variable.next_dragonrage>20|!talent.dragonrage|!variable.trinket_1_buffs|trinket.1.is.spymasters_web&(buff.spymasters_report.stack<5|fight_remains>=130+variable.next_dragonrage))" );
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
  action_priority_list_t* fb = p->get_action_priority_list( "fb" );
  action_priority_list_t* filler = p->get_action_priority_list( "filler" );
  action_priority_list_t* items = p->get_action_priority_list( "items" );
  action_priority_list_t* opener_filler = p->get_action_priority_list( "opener_filler" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=spam_heal,default=1,op=reset" );
  precombat->add_action( "variable,name=minimum_opener_delay,op=reset,default=0" );
  precombat->add_action( "variable,name=opener_delay,value=variable.minimum_opener_delay,if=!talent.interwoven_threads" );
  precombat->add_action( "variable,name=opener_delay,value=variable.minimum_opener_delay+variable.opener_delay,if=talent.interwoven_threads" );
  precombat->add_action( "variable,name=opener_cds_detected,op=reset,default=0" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell|trinket.1.is.whispering_incarnate_icon|trinket.1.is.ovinaxs_mercurial_egg|trinket.1.is.aberrant_spellforge" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell|trinket.2.is.whispering_incarnate_icon|trinket.2.is.ovinaxs_mercurial_egg|trinket.2.is.aberrant_spellforge" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.nymues_unraveling_spindle|trinket.1.is.spymasters_web|trinket.1.is.treacherous_transmitter", "Nymues is complicated, Manual Handle" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.nymues_unraveling_spindle|trinket.2.is.spymasters_web|trinket.2.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_1_ogcd_cast,value=trinket.1.is.beacon_to_the_beyond" );
  precombat->add_action( "variable,name=trinket_2_ogcd_cast,value=trinket.2.is.beacon_to_the_beyond" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=(trinket.1.has_use_buff|(trinket.1.has_buff.intellect|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit)&!variable.trinket_1_exclude)&(!trinket.1.is.flarendos_pilot_light)" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=(trinket.2.has_use_buff|(trinket.2.has_buff.intellect|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit)&!variable.trinket_2_exclude)&(!trinket.2.is.flarendos_pilot_light)" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%120=0)" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%120=0)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs&(trinket.2.has_cooldown&!variable.trinket_2_exclude|!trinket.1.has_cooldown)|variable.trinket_2_buffs&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(0.5+trinket.2.has_buff.intellect*3+trinket.2.has_buff.mastery)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(0.5+trinket.1.has_buff.intellect*3+trinket.1.has_buff.mastery)*(variable.trinket_1_sync)*(1+((trinket.1.ilvl-trinket.2.ilvl)%100)))" );
  precombat->add_action( "variable,name=damage_trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs&trinket.2.ilvl>=trinket.1.ilvl" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=trinket.1.is.nymues_unraveling_spindle&trinket.2.has_buff.intellect|trinket.2.is.nymues_unraveling_spindle&!trinket.1.has_buff.intellect,if=(trinket.1.is.nymues_unraveling_spindle|trinket.2.is.nymues_unraveling_spindle)&(variable.trinket_1_buffs&variable.trinket_2_buffs)", "Double on use - Priotize Intellect on use trinkets over Nymues, force overwriting the normal logic to guarantee it is correct." );
  precombat->add_action( "variable,name=hold_empower_for,op=reset,default=6" );
  precombat->add_action( "variable,name=ebon_might_pandemic_threshold,op=reset,default=0.4" );
  precombat->add_action( "variable,name=wingleader_force_timings,op=reset,default=0" );
  precombat->add_action( "use_item,name=aberrant_spellforge" );
  precombat->add_action( "blistering_scales,target_if=target.role.tank" );
  precombat->add_action( "living_flame" );

  default_->add_action( "variable,name=temp_wound,value=debuff.temporal_wound.remains,target_if=max:debuff.temporal_wound.remains" );
  default_->add_action( "variable,name=eons_remains,op=setif,value=cooldown.allied_virtual_cd_time.remains,value_else=cooldown.breath_of_eons.remains,condition=!talent.wingleader|variable.wingleader_force_timings" );
  default_->add_action( "variable,name=pool_for_id,if=talent.imminent_destruction,default=0,op=set,value=(variable.eons_remains<8)&essence.deficit>=1&!buff.essence_burst.react" );
  default_->add_action( "prescience,target_if=min:(debuff.prescience.remains-200*(target.role.attack|target.role.spell|target.role.dps)+50*target.spec.augmentation),if=((full_recharge_time<=gcd.max*3|cooldown.ebon_might.remains<=gcd.max*3&(buff.ebon_might_self.remains-gcd.max*3)<=buff.ebon_might_self.duration*variable.ebon_might_pandemic_threshold|fight_remains<=30)|variable.eons_remains<=8|talent.anachronism&buff.imminent_destruction.up&essence<1&!cooldown.fire_breath.up&!cooldown.upheaval.up)&debuff.prescience.remains<gcd.max*2&(!talent.anachronism|buff.essence_burst.stack<buff.essence_burst.max_stack|time<=5+5*talent.time_skip)" );
  default_->add_action( "prescience,target_if=min:(debuff.prescience.remains-200*(target.spec.augmentation|target.role.tank)),if=full_recharge_time<=gcd.max*3&debuff.prescience.remains<gcd.max*2&(target.spec.augmentation|target.role.tank)&(!talent.anachronism|buff.essence_burst.stack<buff.essence_burst.max_stack|time<=5)" );
  default_->add_action( "hover,use_off_gcd=1,if=gcd.remains>=0.5&(!raid_event.movement.exists&(trinket.1.is.ovinaxs_mercurial_egg|trinket.2.is.ovinaxs_mercurial_egg)|raid_event.movement.in<=6)" );
  default_->add_action( "potion,if=variable.eons_remains<=0|fight_remains<=30" );
  default_->add_action( "call_action_list,name=ebon_logic,if=(buff.ebon_might_self.remains-cast_time)<=buff.ebon_might_self.duration*variable.ebon_might_pandemic_threshold&(active_enemies>0|raid_event.adds.in<=3)&variable.eons_remains>0&(!buff.imminent_destruction.up|buff.ebon_might_self.remains<=gcd.max)" );
  default_->add_action( "call_action_list,name=items" );
  default_->add_action( "run_action_list,name=opener_filler,if=variable.opener_delay>0&!fight_style.dungeonroute" );
  default_->add_action( "fury_of_the_aspects,if=talent.time_convergence&!buff.time_convergence_intellect.up&(essence>=2|buff.essence_burst.react)" );
  default_->add_action( "deep_breath" );
  default_->add_action( "tip_the_scales,if=talent.threads_of_fate&(prev_gcd.1.breath_of_eons|fight_remains<=30)" );
  default_->add_action( "call_action_list,name=fb,if=(raid_event.adds.remains>13|raid_event.adds.in>20|evoker.allied_cds_up>0|!raid_event.adds.exists)&(variable.eons_remains>=variable.hold_empower_for|!talent.breath_of_eons|variable.eons_remains=0)" );
  default_->add_action( "upheaval,target_if=target.time_to_die>duration+0.2,empower_to=1,if=buff.ebon_might_self.remains>duration&(raid_event.adds.remains>10|evoker.allied_cds_up>0|!raid_event.adds.exists|raid_event.adds.in>20)&(!talent.molten_embers|dot.fire_breath_damage.ticking|cooldown.fire_breath.remains>=10)&(cooldown.allied_virtual_cd_time.remains>=variable.hold_empower_for|!talent.breath_of_eons|talent.wingleader&cooldown.breath_of_eons.remains>=variable.hold_empower_for)&(buff.essence_burst.stack<buff.essence_burst.max_stack|!set_bonus.tww2_4pc&!talent.rockfall|!buff.essence_burst.react)" );
  default_->add_action( "breath_of_eons,if=talent.wingleader&(target.time_to_die>=15&(raid_event.adds.in>=20|raid_event.adds.remains>=15))&!variable.wingleader_force_timings&(time%%240<=190&time%%240>=3)|fight_remains<=30" );
  default_->add_action( "breath_of_eons,if=((cooldown.ebon_might.remains<=4|buff.ebon_might_self.up)&target.time_to_die>15&raid_event.adds.in>15|fight_remains<30)&!fight_style.dungeonroute&cooldown.allied_virtual_cd_time.up|fight_remains<=15&(talent.imminent_destruction|talent.melt_armor)" );
  default_->add_action( "breath_of_eons,if=evoker.allied_cds_up>0&((cooldown.ebon_might.remains<=4|buff.ebon_might_self.up)&target.time_to_die>15|fight_remains<30)&fight_style.dungeonroute" );
  default_->add_action( "time_skip,if=(((cooldown.fire_breath.remains>?20)+(cooldown.upheaval.remains>?20)))>=30&cooldown.breath_of_eons.remains>=20" );
  default_->add_action( "emerald_blossom,if=talent.dream_of_spring&buff.essence_burst.react&(variable.spam_heal=2|variable.spam_heal=1&!buff.ancient_flame.up&talent.ancient_flame)&(buff.ebon_might_self.up|essence.deficit=0|buff.essence_burst.stack=buff.essence_burst.max_stack&cooldown.ebon_might.remains>4)" );
  default_->add_action( "living_flame,target_if=max:debuff.bombardments.remains,if=talent.mass_eruption&buff.mass_eruption_stacks.up&!buff.imminent_destruction.up&buff.essence_burst.stack<buff.essence_burst.max_stack&essence.deficit>1&(buff.ebon_might_self.remains>=6|cooldown.ebon_might.remains<=6)&debuff.bombardments.remains<action.eruption.execute_time&(talent.pupil_of_alexstrasza|active_enemies=1)" );
  default_->add_action( "azure_strike,target_if=max:debuff.bombardments.remains,if=talent.mass_eruption&buff.mass_eruption_stacks.up&!buff.imminent_destruction.up&buff.essence_burst.stack<buff.essence_burst.max_stack&essence.deficit>1&(buff.ebon_might_self.remains>=6|cooldown.ebon_might.remains<=6)&debuff.bombardments.remains<action.eruption.execute_time&(talent.echoing_strike&active_enemies>1)" );
  default_->add_action( "eruption,target_if=min:debuff.bombardments.remains,if=(buff.ebon_might_self.remains>execute_time|essence.deficit=0|buff.essence_burst.stack=buff.essence_burst.max_stack&cooldown.ebon_might.remains>4|buff.essence_burst.react&set_bonus.tww2_4pc)&!variable.pool_for_id&(buff.imminent_destruction.up|essence.deficit<=2|buff.essence_burst.up|variable.ebon_might_pandemic_threshold>0)", "actions+=/time_spiral,if=talent.time_convergence&!buff.time_convergence_intellect.up&(essence>=2|buff.essence_burst.react)  actions+=/oppressing_roar,if=talent.time_convergence&!buff.time_convergence_intellect.up&(essence>=2|buff.essence_burst.react)" );
  default_->add_action( "blistering_scales,target_if=target.role.tank,if=!evoker.scales_up&buff.ebon_might_self.down" );
  default_->add_action( "run_action_list,name=filler" );

  ebon_logic->add_action( "ebon_might" );

  fb->add_action( "tip_the_scales,if=cooldown.fire_breath.ready&buff.ebon_might_self.up" );
  fb->add_action( "fire_breath,empower_to=4,target_if=target.time_to_die>4,if=talent.font_of_magic&(buff.ebon_might_self.remains>duration&(!talent.molten_embers|cooldown.upheaval.remains<=(20+4*talent.blast_furnace-6*3))|buff.tip_the_scales.up)" );
  fb->add_action( "fire_breath,empower_to=3,target_if=target.time_to_die>8,if=(buff.ebon_might_self.remains>duration&(!talent.molten_embers|cooldown.upheaval.remains<=(20+4*talent.blast_furnace-6*2))|buff.tip_the_scales.up)" );
  fb->add_action( "fire_breath,empower_to=2,target_if=target.time_to_die>12,if=buff.ebon_might_self.remains>duration&(!talent.molten_embers|cooldown.upheaval.remains<=(20+4*talent.blast_furnace-6*1))" );
  fb->add_action( "fire_breath,empower_to=1,target_if=target.time_to_die>16,if=buff.ebon_might_self.remains>duration&(!talent.molten_embers|cooldown.upheaval.remains<=(20+4*talent.blast_furnace-6*0))" );
  fb->add_action( "fire_breath,empower_to=4,target_if=target.time_to_die>4,if=talent.font_of_magic&(buff.ebon_might_self.remains>duration)" );
  fb->add_action( "fire_breath,empower_to=3,target_if=target.time_to_die>8,if=talent.font_of_magic&set_bonus.tww2_2pc&talent.molten_embers" );

  filler->add_action( "living_flame,if=(buff.ancient_flame.up|mana>=200000|!talent.dream_of_spring|variable.spam_heal=0)&(active_enemies=1|talent.pupil_of_alexstrasza)" );
  filler->add_action( "emerald_blossom,if=!buff.ebon_might_self.up&talent.ancient_flame&talent.scarlet_adaptation&!talent.dream_of_spring&!buff.ancient_flame.up&active_enemies=1" );
  filler->add_action( "verdant_embrace,if=!buff.ebon_might_self.up&talent.ancient_flame&talent.scarlet_adaptation&!buff.ancient_flame.up&(!talent.dream_of_spring|mana>=200000)&active_enemies=1" );
  filler->add_action( "azure_strike" );

  items->add_action( "use_item,name=nymues_unraveling_spindle,if=cooldown.breath_of_eons.remains<=3&(trinket.1.is.nymues_unraveling_spindle&variable.trinket_priority=1|trinket.2.is.nymues_unraveling_spindle&variable.trinket_priority=2)|(cooldown.fire_breath.remains<=4|cooldown.upheaval.remains<=4)&cooldown.breath_of_eons.remains>10&!(debuff.temporal_wound.up|prev_gcd.1.breath_of_eons)&(trinket.1.is.nymues_unraveling_spindle&variable.trinket_priority=2|trinket.2.is.nymues_unraveling_spindle&variable.trinket_priority=1)" );
  items->add_action( "use_item,name=aberrant_spellforge" );
  items->add_action( "use_item,name=flarendos_pilot_light,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&trinket.2.is.flarendos_pilot_light|!variable.trinket_2_buffs&!variable.trinket_2_manual&trinket.1.is.flarendos_pilot_light" );
  items->add_action( "use_item,name=treacherous_transmitter,if=cooldown.allied_virtual_cd_time.remains<=10|cooldown.breath_of_eons.remains<=10&talent.wingleader|fight_remains<=15" );
  items->add_action( "do_treacherous_transmitter_task,use_off_gcd=1,if=(debuff.temporal_wound.up|prev_gcd.1.breath_of_eons|fight_remains<=15)" );
  items->add_action( "use_item,name=spymasters_web,if=(debuff.temporal_wound.up|prev_gcd.1.breath_of_eons)&(fight_remains<=130-(30+12*talent.interwoven_threads)*talent.wingleader-20*talent.time_skip*(cooldown.time_skip.remains<=90)*!talent.interwoven_threads)|(fight_remains<=20|evoker.allied_cds_up>0&fight_remains<=60)&(trinket.1.is.spymasters_web&(trinket.2.cooldown.duration=0|trinket.2.cooldown.remains>=10|variable.trinket_2_exclude)|trinket.2.is.spymasters_web&(trinket.1.cooldown.duration=0|trinket.1.cooldown.remains>=10|variable.trinket_1_exclude))&!buff.spymasters_web.up" );
  items->add_action( "use_item,slot=trinket1,if=variable.trinket_1_buffs&!variable.trinket_1_manual&!variable.trinket_1_exclude&((debuff.temporal_wound.up|prev_gcd.1.breath_of_eons)|variable.trinket_2_buffs&!trinket.2.cooldown.up&(prev_gcd.1.fire_breath|prev_gcd.1.upheaval)&buff.ebon_might_self.up)&(variable.trinket_2_exclude|!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains" );
  items->add_action( "use_item,slot=trinket2,if=variable.trinket_2_buffs&!variable.trinket_2_manual&!variable.trinket_2_exclude&((debuff.temporal_wound.up|prev_gcd.1.breath_of_eons)|variable.trinket_1_buffs&!trinket.1.cooldown.up&(prev_gcd.1.fire_breath|prev_gcd.1.upheaval)&buff.ebon_might_self.up)&(variable.trinket_1_exclude|!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains" );
  items->add_action( "azure_strike,if=cooldown.item_cd_1141.up&(variable.trinket_1_ogcd_cast&trinket.1.cooldown.up&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains)|variable.trinket_2_ogcd_cast&trinket.2.cooldown.up&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains))", "Azure Strike for OGCD trinkets. Ideally this would be Prescience casts in reality but this is simpler and seems to have no noticeable diferrence in DPS." );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&!variable.trinket_1_exclude&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains|trinket.2.is.spymasters_web&buff.spymasters_report.stack<30|variable.eons_remains>=20|trinket.2.cooldown.duration=0|variable.trinket_2_exclude)&(gcd.remains>0.1&variable.trinket_1_ogcd_cast)", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  items->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&!variable.trinket_2_exclude&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains|trinket.1.is.spymasters_web&buff.spymasters_report.stack<30|variable.eons_remains>=20|trinket.1.cooldown.duration=0|variable.trinket_1_exclude)&(gcd.remains>0.1&variable.trinket_2_ogcd_cast)" );
  items->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&!variable.trinket_1_exclude&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains|trinket.2.is.spymasters_web&buff.spymasters_report.stack<30|variable.eons_remains>=20|trinket.2.cooldown.duration=0|variable.trinket_2_exclude)&(!variable.trinket_1_ogcd_cast)" );
  items->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&!variable.trinket_2_exclude&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains|trinket.1.is.spymasters_web&buff.spymasters_report.stack<30|variable.eons_remains>=20|trinket.1.cooldown.duration=0|variable.trinket_1_exclude)&(!variable.trinket_2_ogcd_cast)" );
  items->add_action( "use_item,name=bestinslots,use_off_gcd=1,if=buff.ebon_might_self.up" );
  items->add_action( "use_item,slot=main_hand,use_off_gcd=1,if=gcd.remains>=gcd.max*0.6&!equipped.bestinslots", "Use on use weapons" );

  opener_filler->add_action( "variable,name=opener_delay,value=variable.opener_delay>?variable.minimum_opener_delay,if=!variable.opener_cds_detected&evoker.allied_cds_up>0" );
  opener_filler->add_action( "variable,name=opener_delay,value=variable.opener_delay-1" );
  opener_filler->add_action( "variable,name=opener_cds_detected,value=1,if=!variable.opener_cds_detected&evoker.allied_cds_up>0" );
  opener_filler->add_action( "eruption,if=variable.opener_delay>=3" );
  opener_filler->add_action( "living_flame,if=active_enemies=1|talent.pupil_of_alexstrasza" );
  opener_filler->add_action( "azure_strike" );
}
//augmentation_apl_end

void no_spec( player_t* /*p*/ )
{
}

}  // namespace evoker_apl
