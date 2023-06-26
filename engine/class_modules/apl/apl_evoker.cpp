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
  return ( p->true_level > 60 ) ? "main_hand:buzzing_rune_3" : "main_hand:shadowcore_oil";
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
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_buff.intellect|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_buff.intellect|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(trinket.1.cooldown.duration%%cooldown.dragonrage.duration=0|cooldown.dragonrage.duration%%trinket.1.cooldown.duration=0)", "Decide which trinket to pair with Dragonrage, prefer 2 minute and 1 minute trinkets" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(trinket.2.cooldown.duration%%cooldown.dragonrage.duration=0|cooldown.dragonrage.duration%%trinket.2.cooldown.duration=0)" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.spoils_of_neltharus", "Estimates a trinkets value by comparing the cooldown of the trinket, divided by the duration of the buff it provides. Has a intellect modifier (currently 1.5x) to give a higher priority to intellect trinkets. The intellect modifier should be changed as intellect priority increases or decreases. As well as a modifier for if a trinket will or will not sync with cooldowns." );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.spoils_of_neltharus" );
  precombat->add_action( "variable,name=trinket_1_exclude,value=trinket.1.is.ruby_whelp_shell|trinket.1.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_2_exclude,value=trinket.2.is.ruby_whelp_shell|trinket.2.is.whispering_incarnate_icon" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs|variable.trinket_2_buffs&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.intellect)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.intellect)*(variable.trinket_1_sync))" );
  precombat->add_action( "variable,name=r1_cast_time,value=1.0*spell_haste", "Rank 1 empower spell cast time" );
  precombat->add_action( "variable,name=dr_prep_time_aoe,default=4,op=reset", "Variable for when to start holding empowers for upcoming DR in AoE. - From my testing 4sec seems like the sweetspot, but it's very minor diff so far - Holding for more than 6 seconds it begins to become a loss." );
  precombat->add_action( "variable,name=dr_prep_time_st,default=13,op=reset", "Variable for when to start holding empowers for upcoming DR in ST." );
  precombat->add_action( "variable,name=has_external_pi,value=cooldown.invoke_power_infusion_0.duration>0" );
  precombat->add_action( "verdant_embrace,if=talent.scarlet_adaptation&talent.firestorm" );
  precombat->add_action( "use_item,name=shadowed_orb_of_torment" );
  precombat->add_action( "firestorm,if=talent.firestorm" );
  precombat->add_action( "living_flame,if=!talent.firestorm" );

  default_->add_action( "potion,if=buff.dragonrage.up&(!cooldown.shattering_star.up|active_enemies>=2)|fight_remains<35", "Delay pot in ST if you are about to SS - mostly relevant for opener where you want DR->FB->SS->rotation" );
  default_->add_action( "variable,name=next_dragonrage,value=cooldown.dragonrage.remains<?(cooldown.eternity_surge.remains-2*gcd.max)<?(cooldown.fire_breath.remains-gcd.max)", "Variable that evaluates when next dragonrage is by working out the maximum between the dragonrage cd and your empowers, ignoring CDR effect estimates." );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=buff.dragonrage.up&!buff.power_infusion.up&!cooldown.fire_breath.up&!cooldown.shattering_star.up", "Invoke External Power Infusions if they're available during dragonrage" );
  default_->add_action( "quell,use_off_gcd=1,if=target.debuff.casting.react", "Rupt to make the raidleader happy" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "run_action_list,name=aoe,if=active_enemies>=3" );
  default_->add_action( "run_action_list,name=st" );

  aoe->add_action( "shattering_star,target_if=max:target.health.pct,if=cooldown.dragonrage.up", "AOE action list; Actually kinda clean now  Open with star before DR to save a global and start with a free EB" );
  aoe->add_action( "dragonrage,if=target.time_to_die>=32|fight_remains<30", "Send DR on CD with no regard to the safety of the mobs - DPS loss to hold DR for empowers, in real world scenario maybe you hold if you are going for longer DRs  DS optimization: Only cast if current fight will last 32s+ or encounter ends in less than 30s" );
  aoe->add_action( "tip_the_scales,if=buff.dragonrage.up&(active_enemies<=3+3*talent.eternitys_span|!cooldown.fire_breath.up)", "Use tip to get that sweet aggro" );
  aoe->add_action( "call_action_list,name=fb,if=(!talent.dragonrage|variable.next_dragonrage>variable.dr_prep_time_aoe|!talent.animosity)&(buff.power_swell.remains<variable.r1_cast_time&buff.blazing_shards.remains<variable.r1_cast_time|buff.dragonrage.up)&(target.time_to_die>=8|fight_remains<30)", "Cast Fire Breath - stagger for swell/blazing shards outside DR  DS optimization: Only cast if current fight will last 8s+ or encounter ends in less than 30s" );
  aoe->add_action( "call_action_list,name=es,if=buff.dragonrage.up|!talent.dragonrage|(cooldown.dragonrage.remains>variable.dr_prep_time_aoe&buff.power_swell.remains<variable.r1_cast_time&buff.blazing_shards.remains<variable.r1_cast_time)&(target.time_to_die>=8|fight_remains<30)", "Cast Eternity Surge - stagger for swell/blazing shards outside DR  DS optimization: Only cast if current fight will last 8s+ or encounter ends in less than 30s" );
  aoe->add_action( "deep_breath,if=!buff.dragonrage.up", "Cast DB if not in DR and not running imminent destruction. - I'm assuming that casting DB inside of DR with imminent before you start spending should be an increase, but might be worth to save it for outside of DR for more value?" );
  aoe->add_action( "shattering_star,target_if=max:target.health.pct,if=buff.essence_burst.stack<buff.essence_burst.max_stack|!talent.arcane_vigor", "Send SS when it doesn't overflow EB, without vigor send on CD" );
  aoe->add_action( "firestorm", "Cast Firestorm before spending ressources" );
  aoe->add_action( "pyre,target_if=max:target.health.pct,if=talent.volatility&(active_enemies>=4|(talent.charged_blast&!buff.essence_burst.up&!buff.iridescence_blue.up)|(!talent.charged_blast&(!buff.essence_burst.up|!buff.iridescence_blue.up))|(buff.charged_blast.stack>=15)|(talent.raging_inferno&debuff.in_firestorm.up))", "Pyre logic with volatility.  Pyre 4T+  Pyre on 3T if playing CB and neither EB or blue buff is up  Pyre on 3T if not playing CB aslong as both EB and blue buff isn't up at the same time.  Pyre with 15 stacks of CB  Pyre with raging debuff active" );
  aoe->add_action( "pyre,target_if=max:target.health.pct,if=(talent.raging_inferno&debuff.in_firestorm.up)|(active_enemies==3&buff.charged_blast.stack>=15)|active_enemies>=4", "Pyre logic without volatility.  Without Vola use pyre if raging debuff is active, or if on 3T with 15 stacks of CB, or if 4T+" );
  aoe->add_action( "living_flame,target_if=max:target.health.pct,if=(!talent.burnout|buff.burnout.up|active_enemies>=4|buff.scarlet_adaptation.up)&buff.leaping_flames.up&!buff.essence_burst.up&essence<essence.max-1", "Cast LF with leaping flames up if: not playing burnout, burnout is up, more than 4 enemies, or scarlet adaptation is up." );
  aoe->add_action( "use_item,name=kharnalex_the_first_light,if=!buff.dragonrage.up&debuff.shattering_star_debuff.down&active_enemies<=5", "Use staff on 5T or below" );
  aoe->add_action( "disintegrate,target_if=max:target.health.pct,chain=1,early_chain_if=evoker.use_early_chaining&(buff.dragonrage.up|essence.deficit<=1)&ticks>=2&(raid_event.movement.in>2|buff.hover.up),interrupt_if=evoker.use_clipping&buff.dragonrage.up&ticks>=2&(raid_event.movement.in>2|buff.hover.up),if=raid_event.movement.in>2|buff.hover.up", "Yoinked the disintegrate logic from ST" );
  aoe->add_action( "living_flame,target_if=max:target.health.pct,if=talent.snapfire&buff.burnout.up", "Cast LF with burnout and snapfire(kekW) proc for those juicy insta firestorms" );
  aoe->add_action( "azure_strike,target_if=max:target.health.pct", "Fallback filler" );

  es->add_action( "eternity_surge,empower_to=1,target_if=max:target.health.pct,if=active_enemies<=1+talent.eternitys_span|buff.dragonrage.remains<1.75*spell_haste&buff.dragonrage.remains>=1*spell_haste|buff.dragonrage.up&(active_enemies==5|!talent.eternitys_span&active_enemies>=6|talent.eternitys_span&active_enemies>=8)", "Eternity Surge, use rank most applicable to targets." );
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
  st->add_action( "dragonrage,if=cooldown.fire_breath.remains<4&cooldown.eternity_surge.remains<10&target.time_to_die>=32|fight_remains<30", "Relaxed Dragonrage Entry Requirements, cannot reliably reach third extend under normal conditions (Bloodlust + Power Infusion/Very high haste gear)  DS optimization: Only cast if current fight will last 32s+ or encounter ends in less than 30s" );
  st->add_action( "tip_the_scales,if=buff.dragonrage.up&(((!talent.font_of_magic|talent.everburning_flame)&cooldown.fire_breath.remains<cooldown.eternity_surge.remains&buff.dragonrage.remains<14)|(cooldown.eternity_surge.remains<cooldown.fire_breath.remains&!talent.everburning_flame&talent.font_of_magic))", "Tip second FB if not playing font of magic or if playing EBF, otherwise tip ES." );
  st->add_action( "call_action_list,name=fb,if=(!talent.dragonrage|variable.next_dragonrage>variable.dr_prep_time_st|!talent.animosity)&((buff.power_swell.remains<variable.r1_cast_time|buff.bloodlust.up|buff.power_infusion.up|buff.dragonrage.up)&(buff.blazing_shards.remains<variable.r1_cast_time|buff.dragonrage.up))&(!cooldown.eternity_surge.up|!talent.event_horizon|!buff.dragonrage.up)&(target.time_to_die>=8|fight_remains<30)", "Fire breath logic  Play around power swell if you don't have pi or lust up. Play around blazing shards if outside of DR.  DS optimization: Only cast if current fight will last 8s+ or encounter ends in less than 30s" );
  st->add_action( "disintegrate,if=buff.dragonrage.remains>19&cooldown.fire_breath.remains>28&talent.eye_of_infinity", "Use Disintegrate after FB opener in DR." );
  st->add_action( "shattering_star,if=(buff.essence_burst.stack<buff.essence_burst.max_stack|!talent.arcane_vigor)&(!cooldown.fire_breath.up|!talent.event_horizon)", "Throw Star on CD, Don't overcap with Arcane Vigor." );
  st->add_action( "call_action_list,name=es,if=(!talent.dragonrage|variable.next_dragonrage>variable.dr_prep_time_st|!talent.animosity)&((buff.power_swell.remains<variable.r1_cast_time|buff.bloodlust.up|buff.power_infusion.up)&(buff.blazing_shards.remains<variable.r1_cast_time|buff.dragonrage.up))&(target.time_to_die>=8|fight_remains<30)", "Eternity Surge logic  Play around power swell if you don't have pi or lust up. Play around blazing shards if outside of DR.  DS optimization: Only cast if current fight will last 8s+ or encounter ends in less than 30s" );
  st->add_action( "wait,sec=cooldown.fire_breath.remains,if=talent.animosity&buff.dragonrage.up&buff.dragonrage.remains<gcd.max+variable.r1_cast_time*buff.tip_the_scales.down&buff.dragonrage.remains-cooldown.fire_breath.remains>=variable.r1_cast_time*buff.tip_the_scales.down", "Wait for FB/ES to be ready if spending another GCD would result in the cast no longer fitting inside of DR" );
  st->add_action( "wait,sec=cooldown.eternity_surge.remains,if=talent.animosity&buff.dragonrage.up&buff.dragonrage.remains<gcd.max+variable.r1_cast_time&buff.dragonrage.remains-cooldown.eternity_surge.remains>variable.r1_cast_time*buff.tip_the_scales.down" );
  st->add_action( "living_flame,if=buff.dragonrage.up&buff.dragonrage.remains<(buff.essence_burst.max_stack-buff.essence_burst.stack)*gcd.max&buff.burnout.up", "Spend the last 1 or 2 GCDs of DR on fillers to exit with 2 EBs" );
  st->add_action( "azure_strike,if=buff.dragonrage.up&buff.dragonrage.remains<(buff.essence_burst.max_stack-buff.essence_burst.stack)*gcd.max" );
  st->add_action( "living_flame,if=buff.burnout.up&(buff.leaping_flames.up&!buff.essence_burst.up|!buff.leaping_flames.up&buff.essence_burst.stack<buff.essence_burst.max_stack)&essence.deficit>=2", "Spend burnout procs without overcapping resources" );
  st->add_action( "pyre,if=debuff.in_firestorm.up&talent.raging_inferno&buff.charged_blast.stack==20&active_enemies>=2", "Spend pyre if raging inferno debuff is active and you have 20 stacks of CB on 2T" );
  st->add_action( "disintegrate,chain=1,early_chain_if=evoker.use_early_chaining&ticks>=2&buff.dragonrage.up&!(buff.power_infusion.up&buff.bloodlust.up)&(raid_event.movement.in>2|buff.hover.up),interrupt_if=evoker.use_clipping&buff.dragonrage.up&ticks>=2&(!(buff.power_infusion.up&buff.bloodlust.up)|cooldown.fire_breath.up|cooldown.eternity_surge.up)&(raid_event.movement.in>2|buff.hover.up),if=raid_event.movement.in>2|buff.hover.up", "Dis logic  Early Chain in DR after third tick if both lust & pi isn't up.  Clip after in DR after third tick for more important buttions, atm that is: empowers, burnout & SS. burnout and SS you only clip for if both lust & pi isn't up." );
  st->add_action( "firestorm,if=!buff.dragonrage.up&debuff.shattering_star_debuff.down", "Hard cast only outside of SS and DR windows" );
  st->add_action( "deep_breath,if=!buff.dragonrage.up&active_enemies>=2&((raid_event.adds.in>=120&!talent.onyx_legacy)|(raid_event.adds.in>=60&talent.onyx_legacy))", "Use Deep Breath on 2T, unless adds will come before it'll be ready again or if talented ID." );
  st->add_action( "deep_breath,if=!buff.dragonrage.up&talent.imminent_destruction&!debuff.shattering_star_debuff.up" );
  st->add_action( "call_action_list,name=green,if=talent.ancient_flame&!buff.ancient_flame.up&!buff.shattering_star_debuff.up&talent.scarlet_adaptation&!buff.dragonrage.up", "Get Ancient Flame as Filler" );
  st->add_action( "living_flame,if=!buff.dragonrage.up|(buff.iridescence_red.remains>execute_time|buff.iridescence_blue.up)&active_enemies==1", "Cast LF outside of DR, In DR only cast with Iridescence." );
  st->add_action( "azure_strike", "Fallback for movement" );

  trinkets->add_action( "use_item,name=spoils_of_neltharus,if=buff.dragonrage.up&(active_enemies>=3|!buff.spoils_of_neltharus_vers.up&!cooldown.fire_breath.up&!cooldown.shattering_star.up|buff.dragonrage.remains+4*(cooldown.eternity_surge.remains<=gcd.max*2+cooldown.fire_breath.remains<=gcd.max*2)<=18)|fight_remains<=20", "With spoils try to fish for non vers buff before you using it on <=2T, use regardless of buff when 18s is left on DR. Don't fish when >=3T." );
  trinkets->add_action( "use_item,slot=trinket1,if=buff.dragonrage.up&((!cooldown.fire_breath.up&!cooldown.shattering_star.up)|active_enemies>=3|(buff.dragonrage.remains+4*(cooldown.eternity_surge.remains<=gcd.max*2+cooldown.fire_breath.remains<=gcd.max*2)<=18))&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1|variable.trinket_2_exclude)&!variable.trinket_1_manual|trinket.1.proc.any_dps.duration>=fight_remains|trinket.1.cooldown.duration<=60&(variable.next_dragonrage>20|!talent.dragonrage)&(!buff.dragonrage.up|variable.trinket_priority=1)", "The trinket with the highest estimated value, will be used first and paired with Dragonrage." );
  trinkets->add_action( "use_item,slot=trinket2,if=buff.dragonrage.up&((!cooldown.fire_breath.up&!cooldown.shattering_star.up)|active_enemies>=3|(buff.dragonrage.remains+4*(cooldown.eternity_surge.remains<=gcd.max*2+cooldown.fire_breath.remains<=gcd.max*2)<=18))&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2|variable.trinket_1_exclude)&!variable.trinket_2_manual|trinket.2.proc.any_dps.duration>=fight_remains|trinket.2.cooldown.duration<=60&(variable.next_dragonrage>20|!talent.dragonrage)&(!buff.dragonrage.up|variable.trinket_priority=2)" );
  trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs)&(variable.next_dragonrage>20|!talent.dragonrage)&!variable.trinket_1_manual", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs)&(variable.next_dragonrage>20|!talent.dragonrage)&!variable.trinket_2_manual" );
}
//devastation_apl_end

void preservation( player_t* /*p*/ )
{
}

void augmentation( player_t* /*p*/ )
{
}

void no_spec( player_t* /*p*/ )
{
}

}  // namespace evoker_apl
