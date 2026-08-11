#include "class_modules/apl/mage.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace mage_apl {

std::string potion( const player_t* p )
{
  std::string lvl90_potion = "disabled";

  switch ( p->specialization() )
  {
    case MAGE_ARCANE: lvl90_potion = "lights_potential_2"; break;
    case MAGE_FIRE: lvl90_potion = "lights_potential_2"; break;
    case MAGE_FROST: lvl90_potion = "potion_of_recklessness_2"; break;
    default: break;
  }

  return p->true_level >= 90 ? lvl90_potion
       : p->true_level >= 80 ? "tempered_potion_3"
       : p->true_level >= 70 ? "elemental_potion_of_ultimate_power_3"
       : p->true_level >= 60 ? "spectral_intellect"
       : p->true_level >= 50 ? "superior_battle_potion_of_intellect"
       :                       "disabled";
}

std::string flask( const player_t* p )
{
  std::string lvl90_flask = "disabled";

  switch ( p->specialization() )
  {
    case MAGE_ARCANE: lvl90_flask = "flask_of_the_blood_knights_2"; break;
    case MAGE_FIRE: lvl90_flask = "flask_of_thalassian_resistance_2"; break;
    case MAGE_FROST: lvl90_flask = "flask_of_the_shattered_sun_2"; break;
    default: break;
  }

  return p->true_level >= 90 ? lvl90_flask
       : p->true_level >= 80 ? "flask_of_alchemical_chaos_3"
       : p->true_level >= 70 ? "phial_of_tepid_versatility_3"
       : p->true_level >= 60 ? "spectral_flask_of_power"
       : p->true_level >= 50 ? "greater_flask_of_endless_fathoms"
       :                       "disabled";
}

std::string food( const player_t* p )
{
  return p->true_level >= 90 ? "silvermoon_parade"
       : p->true_level >= 80 ? "feast_of_the_midnight_masquerade"
       : p->true_level >= 70 ? "fated_fortune_cookie"
       : p->true_level >= 60 ? "feast_of_gluttonous_hedonism"
       : p->true_level >= 50 ? "famine_evaluator_and_snack_table"
       :                       "disabled";
}

std::string rune( const player_t* p )
{
  return p->true_level >= 90 ? "void_touched"
       : p->true_level >= 80 ? "crystallized"
       : p->true_level >= 70 ? "draconic"
       : p->true_level >= 60 ? "veiled"
       : p->true_level >= 50 ? "battle_scarred"
       :                       "disabled";
}

std::string temporary_enchant( const player_t* p )
{
  return p->true_level >= 90 ? "main_hand:thalassian_phoenix_oil_2"
       : p->true_level >= 80 ? "main_hand:algari_mana_oil_3"
       : p->true_level >= 70 ? "main_hand:buzzing_rune_3"
       : p->true_level >= 60 ? "main_hand:shadowcore_oil"
       :                       "disabled";
}

//arcane_apl_start
void arcane( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* spellslinger = p->get_action_priority_list( "spellslinger" );
  action_priority_list_t* sunfury = p->get_action_priority_list( "sunfury" );

  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "variable,name=aoe_count,op=set,value=2+(3*talent.spellfire_spheres)" );
  precombat->add_action( "variable,name=20ssteroid_trinket_equipped,op=set,value=equipped.signet_of_the_priory|equipped.incorporeal_essencegorger|equipped.sealed_chaos_urn|equipped.hex_lords_dooming_idol" );
  precombat->add_action( "variable,name=15ssteroid_trinket_equipped,op=set,value=equipped.lily_of_the_eternal_weave|equipped.sunblood_amethyst|equipped.astral_gladiators_badge_of_ferocity|equipped.arazs_ritual_forge|equipped.freightrunners_flask|equipped.emberwing_feather|equipped.vaelgors_final_stare|equipped.galactic_gladiators_badge_of_ferocity|equipped.vile_vial_of_volatile_venom|equipped.venomous_gladiators_badge_of_ferocity" );
  precombat->add_action( "variable,name=12ssteroid_trinket_equipped,op=set,value=equipped.nevermelting_ice_crystal|equipped.ever_collapsing_void_fissure" );
  precombat->add_action( "variable,name=steroid_trinket_equipped,op=set,value=equipped.lily_of_the_eternal_weave|equipped.sunblood_amethyst|equipped.astral_gladiators_badge_of_ferocity|equipped.arazs_ritual_forge|equipped.freightrunners_flask|equipped.emberwing_feather|equipped.vaelgors_final_stare|equipped.galactic_gladiators_badge_of_ferocity|equipped.nevermelting_ice_crystal|equipped.ever_collapsing_void_fissure|equipped.signet_of_the_priory|equipped.incorporeal_essencegorger|equipped.sealed_chaos_urn|equipped.venomous_gladiators_badge_of_ferocity|equipped.vile_vial_of_volatile_venom|equipped.stormbound_emblem_of_dazar" );
  precombat->add_action( "variable,name=nonsteroid_trinket_equipped,op=set,value=equipped.mereldars_toll|equipped.perfidious_projector|equipped.chaotic_nethergate|equipped.wraps_of_cosmic_madness|equipped.astalors_anguish_agitator|equipped.sethraliss_defiled_relic|equipped.vexhuls_everflowing_gland|equipped.font_of_venomous_rage|equipped.ophidian_bone_whistle|equipped.spiritrending_poison" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "arcane_blast" );

  default_->add_action( "counterspell" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=(buff.arcane_surge.up&debuff.touch_of_the_magi.up)|(fight_remains<16)" );
  default_->add_action( "potion,if=cooldown.arcane_surge.ready|fight_remains<35" );
  default_->add_action( "berserking,if=(buff.arcane_surge.up&debuff.touch_of_the_magi.up)|(fight_remains<13)" );
  default_->add_action( "blood_fury,if=(buff.arcane_surge.up&debuff.touch_of_the_magi.up)|(fight_remains<9)" );
  default_->add_action( "fireblood,if=(buff.arcane_surge.up&debuff.touch_of_the_magi.up)|(fight_remains<16)" );
  default_->add_action( "ancestral_call,if=(buff.arcane_surge.up&debuff.touch_of_the_magi.up)|(fight_remains<16)" );
  default_->add_action( "use_item,name=stormbound_emblem_of_dazar,if=(cooldown.arcane_surge.remains<2)|cooldown.arcane_surge.ready|(fight_remains<22)" );
  default_->add_action( "use_items,if=((buff.arcane_surge.up&((variable.12ssteroid_trinket_equipped&debuff.touch_of_the_magi.up)|variable.15ssteroid_trinket_equipped))|(cooldown.arcane_surge.ready&variable.20ssteroid_trinket_equipped))|(fight_remains<13&variable.12ssteroid_trinket_equipped)|(fight_remains<16&variable.15ssteroid_trinket_equipped)|(fight_remains<21&variable.20ssteroid_trinket_equipped)|(variable.nonsteroid_trinket_equipped&((buff.arcane_surge.down&cooldown.arcane_surge.remains>20)|!variable.steroid_trinket_equipped))" );
  default_->add_action( "arcane_barrage,if=fight_remains<gcd.max*2&buff.arcane_charge.stack=4" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=spellslinger,if=talent.splintering_sorcery" );
  default_->add_action( "call_action_list,name=sunfury,if=!talent.splintering_sorcery" );
  default_->add_action( "arcane_barrage,if=talent.spellfire_spheres" );

  cooldowns->add_action( "arcane_orb,line_cd=999" );
  cooldowns->add_action( "touch_of_the_magi,use_off_gcd=1,if=(prev_gcd.1.prismatic_bolt|prev_gcd.1.arcane_barrage|(buff.arcane_surge.up&buff.arcane_surge.remains<12))&(buff.arcane_surge.up|cooldown.arcane_surge.remains>30)" );
  cooldowns->add_action( "arcane_surge,if=buff.lustrous_gleam.stack>=2|buff.lustrous_gleam.down" );
  cooldowns->add_action( "cancel_action,if=action.evocation.channeling&mana.pct>=95" );
  cooldowns->add_action( "evocation,if=mana.pct<10&buff.arcane_surge.down&debuff.touch_of_the_magi.down&cooldown.arcane_surge.remains>10" );
  cooldowns->add_action( "presence_of_mind,use_off_gcd=1,if=buff.arcane_charge.stack<1&!buff.prismatic_bolt.up&!cooldown.arcane_orb.ready&!cooldown.arcane_pulse.ready&!cooldown.touch_of_the_magi.ready&(buff.clearcasting.react=0|!talent.high_voltage)&prev_gcd.1.arcane_barrage" );

  spellslinger->add_action( "prismatic_bolt,if=(!set_bonus.midnight_season_2_4pc|buff.cumulative_power.stack>=6|active_enemies>=variable.aoe_count&buff.clearcasting.react=0)&(active_enemies>=variable.aoe_count&buff.clearcasting.react<3|buff.arcane_salvo.react>13)" );
  spellslinger->add_action( "arcane_orb,if=cooldown.arcane_orb.charges_fractional>=1+(0.95*talent.charged_orb)&talent.orb_mastery&active_enemies>=variable.aoe_count" );
  spellslinger->add_action( "arcane_barrage,if=buff.arcane_salvo.react>=(20-talent.orb_barrage)&(active_enemies<variable.aoe_count|buff.arcane_charge.stack=4)|prev_gcd.1.arcane_surge&buff.arcane_salvo.stack>=(10+5*talent.orb_barrage)" );
  spellslinger->add_action( "arcane_missiles,if=buff.clearcasting.react&(buff.arcane_salvo.stack<15|active_enemies<variable.aoe_count&buff.clearcasting.react=3),interrupt_if=tick_time>gcd.remains&(buff.overpowered_missiles.react=0|active_enemies>=variable.aoe_count|!talent.high_voltage),interrupt_global=1,interrupt_immediate=1" );
  spellslinger->add_action( "prismatic_bolt" );
  spellslinger->add_action( "arcane_orb,if=buff.arcane_charge.stack<(3+(active_enemies>=variable.aoe_count))|(talent.orb_mastery|active_enemies>=(variable.aoe_count+2))&cooldown.arcane_orb.charges_fractional>=(1+0.5*(talent.orb_mastery&active_enemies<variable.aoe_count&buff.arcane_surge.down))" );
  spellslinger->add_action( "arcane_pulse,if=buff.arcane_charge.stack<3&buff.arcane_surge.down" );
  spellslinger->add_action( "arcane_blast" );

  sunfury->add_action( "arcane_missiles,if=buff.clearcasting.react&buff.arcane_salvo.stack<12,chain=1" );
  sunfury->add_action( "prismatic_bolt,if=((!set_bonus.midnight_season_2_4pc)|buff.cumulative_power.stack=8)&buff.arcane_soul.down" );
  sunfury->add_action( "arcane_barrage,if=(buff.arcane_charge.stack=4&(((((cooldown.arcane_orb.charges_fractional>0.95|cooldown.arcane_pulse.charges_fractional>0.95)&active_enemies>=variable.aoe_count)|buff.clearcasting.react)&buff.arcane_salvo.react>=12)|((buff.arcane_surge.remains>gcd.max|buff.arcane_surge.down)&buff.arcane_salvo.react=25)))|buff.arcane_soul.up|(buff.arcane_salvo.react>8&cooldown.touch_of_the_magi.ready)" );
  sunfury->add_action( "prismatic_bolt" );
  sunfury->add_action( "arcane_orb,if=buff.arcane_charge.stack<1" );
  sunfury->add_action( "arcane_pulse,if=active_enemies>=(variable.aoe_count-1)|buff.arcane_charge.stack<1" );
  sunfury->add_action( "arcane_blast" );
}
//arcane_apl_end

//fire_apl_start
void fire( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* fireblast = p->get_action_priority_list( "fireblast" );
  action_priority_list_t* ff_combustion = p->get_action_priority_list( "ff_combustion" );
  action_priority_list_t* ff_filler = p->get_action_priority_list( "ff_filler" );
  action_priority_list_t* sf_combustion = p->get_action_priority_list( "sf_combustion" );
  action_priority_list_t* sf_filler = p->get_action_priority_list( "sf_filler" );

  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "variable,name=cast_remains_time,value=0.2" );
  precombat->add_action( "variable,name=flamestriking,op=reset,default=1", "flamestriking is the on/off switch, overridable via apl_variable.flamestriking=0" );
  precombat->add_action( "variable,name=combustion_delay,value=(18*talent.firestarter)-(10*(expected_combat_length<60)+10*(expected_combat_length<30))-10*(((expected_combat_length%%60)>=25)&((expected_combat_length%%60)<=40))", "Delay Combustion if playing Firestarter until the target is >=90% HP unless it means losing casts of Combustion. Do not do so if fight length is short." );
  precombat->add_action( "variable,name=steroid_trinket_equipped,op=set,value=equipped.freightrunners_flask|equipped.emberwing_feather|equipped.vaelgors_final_stare|equipped.galactic_gladiators_badge_of_ferocity|equipped.nevermelting_ice_crystal|equipped.ever_collapsing_void_fissure|equipped.signet_of_the_priory|equipped.sealed_chaos_urn|equipped.stormbound_emblem_of_dazar|equipped.hex_lords_dooming_idol|equipped.vile_vial_of_volatile_venom|equipped.venomous_gladiators_badge_of_ferocity" );
  precombat->add_action( "variable,name=nonsteroid_trinket_equipped,op=set,value=equipped.wraps_of_cosmic_madness|equipped.astalors_anguish_agitator|equipped.font_of_venomous_rage|equipped.vexhuls_everflowing_gland|equipped.spiritrending_poison|equipped.ophidian_bone_whistle" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "pyroblast" );

  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "variable,name=flamestriking_active,op=set,value=variable.flamestriking&(time>variable.combustion_delay|talent.firestarter&buff.pyroclasm.up&buff.hot_streak.react=0)", "Turns off flamestriking during Firestarter, but allows Flamestrike with Pyroclasm." );
  default_->add_action( "variable,name=ff_combustion_flamestrike,op=set,if=!talent.spellfire_spheres,value=3+(999*!talent.fuel_the_fire+999*!variable.flamestriking_active)", "Modifiable target count cutoffs for when to cast Flamestrike. Always turned off if missing Fuel the Fire or during Firestarter." );
  default_->add_action( "variable,name=ff_filler_flamestrike,op=set,if=!talent.spellfire_spheres,value=3+(999*!talent.fuel_the_fire+999*!variable.flamestriking_active)" );
  default_->add_action( "variable,name=sf_combustion_flamestrike,op=set,if=talent.spellfire_spheres,value=3+(999*!talent.fuel_the_fire+999*!variable.flamestriking_active)" );
  default_->add_action( "variable,name=sf_filler_flamestrike,op=set,if=talent.spellfire_spheres,value=3+(999*!talent.fuel_the_fire+999*!variable.flamestriking_active)" );
  default_->add_action( "run_action_list,name=ff_combustion,if=talent.frostfire_bolt&((time>=variable.combustion_delay)&(cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.up|cooldown.combustion.ready))", "Combustion is delayed 18 seconds on pull for all Firestarter builds to simulate realistic timings for when a boss drops below 90% HP." );
  default_->add_action( "run_action_list,name=sf_combustion,if=!talent.frostfire_bolt&((time>=variable.combustion_delay)&(cooldown.combustion.remains<=variable.combustion_precast_time+gcd.max*(talent.sunfury_execution&cooldown.meteor.remains<gcd.max)|buff.combustion.up|cooldown.combustion.ready))" );
  default_->add_action( "run_action_list,name=ff_filler,if=talent.frostfire_bolt" );
  default_->add_action( "run_action_list,name=sf_filler" );

  cds->add_action( "variable,name=combustion_precast_time,value=(action.scorch.cast_time*!buff.pyroclasm.up*scorch_execute.active)+(action.fireball.cast_time*!buff.pyroclasm.up*!scorch_execute.active)+(action.pyroblast.cast_time*buff.pyroclasm.up)-variable.cast_remains_time" );
  cds->add_action( "potion,if=time>=(8*(talent.firestarter&talent.spellfire_spheres))|buff.combustion.remains>6|fight_remains<35", "Use Potion on pull. Delay by about 8 seconds if playing with Firestarter as Sunfury." );
  cds->add_action( "use_item,name=vile_vial_of_volatile_venom,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=stormbound_emblem_of_dazar,if=time>=variable.combustion_delay&cooldown.combustion.remains<2&buff.combustion.down&(buff.bloodlust.down|gcd.max>0.76&!equipped.hex_lords_dooming_idol&(!equipped.vile_vial_of_volatile_venom|trinket.1.is.vile_vial_of_volatile_venom&trinket.1.cooldown.remains>20|trinket.2.is.vile_vial_of_volatile_venom&trinket.2.cooldown.remains>20))|fight_remains<25" );
  cds->add_action( "use_item,name=sethraliss_defiled_relic,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=hex_lords_dooming_idol,if=buff.combustion.remains>6|fight_remains<30" );
  cds->add_action( "use_item,name=vaelgors_final_stare,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=emberwing_feather,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=venomous_gladiators_badge_of_ferocity,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=galactic_gladiators_badge_of_ferocity,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=ever_collapsing_void_fissure,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=freightrunners_flask,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_items,if=variable.nonsteroid_trinket_equipped&(time>variable.combustion_delay&cooldown.combustion.remains>20|time<=variable.combustion_delay&(variable.combustion_delay>=18|!variable.steroid_trinket_equipped))&buff.combustion.down&buff.hyperthermia.down", "Non-steroid trinkets are used outside cooldowns. They can be used on pull if Combustion is delayed an ample amount of time or you don't have a steroid trinket equipped." );
  cds->add_action( "ancestral_call,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "berserking,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "blood_fury,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "fireblood,if=buff.combustion.remains>6|fight_remains<10" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down&(buff.combustion.remains>6|fight_remains<25)" );

  fireblast->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&cooldown.fire_blast.charges_fractional>=2.9&talent.firestarter", "Cast Fire Blast if close to overcapping charges. This can result in casting Fire Blast twice during one cast, if you have ample time left of your cast." );
  fireblast->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&(buff.combustion.up|buff.hyperthermia.up)&(hot_streak_spells_in_flight+buff.heating_up.react=1)&gcd.remains<gcd.max", "During Combustion/Hyperthermia, spend Fire Blasts with Heating Up." );
  fireblast->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&(action.fireball.executing&action.fireball.execute_remains>0.1|buff.pyroclasm.react&action.pyroblast.executing&action.pyroblast.execute_remains>0.1|action.flamestrike.executing&action.flamestrike.execute_remains>0.1)&(!scorch_execute.active|!talent.scorch)&buff.heating_up.react&(hot_streak_spells_in_flight+buff.heating_up.react=1)&gcd.remains<gcd.max", "During non-execute filler, use Fire Blast with Heating Up while hardcasting Fireball/Frostfire Bolt/Pyroblast/Flamestrike." );
  fireblast->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&scorch_execute.active&(hot_streak_spells_in_flight+buff.heating_up.react=0)&(action.scorch.executing&buff.heat_shimmer.down|action.pyroblast.executing&buff.pyroclasm.up&set_bonus.midnight_season_2_2pc)&gcd.remains<gcd.max", "During execute, spend Fire Blasts while casting Scorch, or consuming Pyroclasm with 12.1 2pc, if you don't have Heating Up." );
  fireblast->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&time<variable.combustion_delay&(talent.firestarter|action.fireball.executing&action.fireball.execute_remains>0.1|buff.pyroclasm.react&action.pyroblast.executing&action.pyroblast.execute_remains>0.1)&(hot_streak_spells_in_flight+buff.heating_up.react=1)&gcd.remains<gcd.max&cooldown.combustion.ready", "While delaying Combustion on pull (Firestarter or not), spend Fire Blasts with Heating Up freely. If not playing Firestarter, only do so during hardcasts." );
  fireblast->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&(time>=variable.combustion_delay&(cooldown.combustion.remains<=variable.combustion_precast_time))&buff.combustion.down&talent.spontaneous_combustion&(action.scorch.executing|action.fireball.executing|action.pyroblast.executing|action.flamestrike.executing)", "When talented into Spontaneous Combustion, spend all Fire Blasts during the pre-cast going into Combustion regardless of Heating Up / Hot Streak status." );
  fireblast->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=fight_remains<1", "Spend all available Fire Blasts if fight is ending." );

  ff_combustion->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=buff.combustion.down&action.fireball.executing&(action.fireball.execute_remains<variable.cast_remains_time)|action.meteor.in_flight&(action.meteor.in_flight_remains<0.3)|action.pyroblast.executing&(action.pyroblast.execute_remains<variable.cast_remains_time)|prev_gcd.1.meteor" );
  ff_combustion->add_action( "flamestrike,if=buff.pyroclasm.up&!buff.hot_streak.react&buff.combustion.down&active_enemies>=variable.ff_combustion_flamestrike", "Precast into Combustion. Prioritize Pyroclasm if available." );
  ff_combustion->add_action( "pyroblast,if=buff.pyroclasm.up&!buff.hot_streak.react&buff.combustion.down" );
  ff_combustion->add_action( "fireball,if=buff.combustion.down" );
  ff_combustion->add_action( "meteor,if=talent.burnout&buff.combustion.remains<8|!talent.burnout&buff.combustion.remains>2", "Meteor is used towards the end of Combustion to maximize the Ignite bank for Burnout. If not playing Burnout, just make sure the Meteor lands during Combustion at any time." );
  ff_combustion->add_action( "flamestrike,if=buff.hot_streak.react&active_enemies>=variable.ff_combustion_flamestrike", "Spend Hot Streaks on Pyroblast in ST or Flamestrike in AoE." );
  ff_combustion->add_action( "pyroblast,if=buff.hot_streak.react" );
  ff_combustion->add_action( "flamestrike,if=buff.pyroclasm.up&cast_time<buff.combustion.remains&active_enemies>=(variable.ff_filler_flamestrike-1)", "Make sure Pyroclasm FINISHES its cast before Combustion ends." );
  ff_combustion->add_action( "pyroblast,if=buff.pyroclasm.up&cast_time<buff.combustion.remains" );
  ff_combustion->add_action( "scorch,if=buff.heat_shimmer.react|talent.scald&scorch_execute.active&buff.frostfire_empowerment.down" );
  ff_combustion->add_action( "fireball" );
  ff_combustion->add_action( "call_action_list,name=fireblast,if=!talent.pyroclasm|(buff.pyroclasm.stack<2|action.pyroblast.executing&action.pyroblast.execute_remains>0.2&buff.pyroclasm.stack=2|cooldown.fire_blast.charges_fractional>=2.9|buff.combustion.remains<action.pyroblast.cast_time)&(active_enemies<variable.sf_combustion_flamestrike|buff.pyroclasm.down|!action.flamestrike.executing)" );

  ff_filler->add_action( "meteor,if=time>=(variable.combustion_delay-gcd.max)", "Cast Meteor on CD starting from the precast of your first Combustion." );
  ff_filler->add_action( "flamestrike,if=(buff.hot_streak.react&(cooldown.combustion.remains>=5|time<variable.combustion_delay))&active_enemies>=variable.ff_filler_flamestrike", "Hold Hot Streak if Combustion is coming up soon. Do not hold if intentionally delaying Combustion." );
  ff_filler->add_action( "pyroblast,if=buff.hot_streak.react&(cooldown.combustion.remains>=(5-5*buff.pyroclasm.up)|time<variable.combustion_delay)" );
  ff_filler->add_action( "flamestrike,if=(buff.pyroclasm.up&cooldown.combustion.remains>12|buff.pyroclasm.stack=2)&active_enemies>=variable.ff_filler_flamestrike", "Spend Pyroclasm immediately if you have 2 stacks available. Otherwise, hold one stack if it lasts until Combustion comes up." );
  ff_filler->add_action( "pyroblast,if=buff.pyroclasm.up&cooldown.combustion.remains>12|buff.pyroclasm.stack=2" );
  ff_filler->add_action( "scorch,if=buff.heat_shimmer.react" );
  ff_filler->add_action( "fireball" );
  ff_filler->add_action( "call_action_list,name=fireblast" );

  sf_combustion->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=action.scorch.executing&(action.scorch.execute_remains<variable.cast_remains_time)|action.fireball.executing&(action.fireball.execute_remains<variable.cast_remains_time)|action.pyroblast.executing&(action.pyroblast.execute_remains<variable.cast_remains_time)&!cooldown.meteor.ready|action.flamestrike.executing&(action.flamestrike.execute_remains<variable.cast_remains_time)|action.meteor.in_flight&action.meteor.in_flight_remains<0.3&(action.pyroblast.executing|action.flamestrike.executing|buff.pyroclasm.down)" );
  sf_combustion->add_action( "flamestrike,if=buff.combustion.down&buff.hot_streak.react&active_enemies>=variable.sf_combustion_flamestrike", "Spend Hot Streak before going into your precast." );
  sf_combustion->add_action( "pyroblast,if=buff.combustion.down&buff.hot_streak.react" );
  sf_combustion->add_action( "meteor,if=buff.combustion.down&talent.sunfury_execution", "Precast one of these into Combustion." );
  sf_combustion->add_action( "flamestrike,if=active_enemies>=variable.sf_combustion_flamestrike&(buff.combustion.down&!buff.hot_streak.react&buff.pyroclasm.up)" );
  sf_combustion->add_action( "pyroblast,if=buff.combustion.down&!buff.hot_streak.react&buff.pyroclasm.up" );
  sf_combustion->add_action( "scorch,if=buff.combustion.down&(scorch_execute.active|active_enemies>=4)" );
  sf_combustion->add_action( "fireball,if=buff.combustion.down&(!prev_gcd.1.meteor|buff.bloodlust.down)" );
  sf_combustion->add_action( "meteor,if=buff.combustion.remains>2", "Make sure Meteor lands during Combustion." );
  sf_combustion->add_action( "flamestrike,if=(buff.hot_streak.react|prev_gcd.1.scorch&buff.heating_up.react&time-action.scorch.last_used<0.2)&active_enemies>=variable.sf_combustion_flamestrike", "Spend Hot Streaks on Pyroblast in ST or Flamestrike in AoE. The Scorch condition is simply to simulate predictable guaranteed crits during Combustion." );
  sf_combustion->add_action( "pyroblast,if=buff.hot_streak.react|prev_gcd.1.scorch&buff.heating_up.react&time-action.scorch.last_used<0.2" );
  sf_combustion->add_action( "flamestrike,if=buff.pyroclasm.react&cast_time<buff.combustion.remains&active_enemies>=variable.sf_combustion_flamestrike", "Make sure Pyroclasm FINISHES its cast before Combustion ends." );
  sf_combustion->add_action( "pyroblast,if=buff.pyroclasm.react&cast_time<buff.combustion.remains" );
  sf_combustion->add_action( "scorch" );
  sf_combustion->add_action( "call_action_list,name=fireblast,if=!talent.pyroclasm|(buff.pyroclasm.stack<2|action.pyroblast.executing&action.pyroblast.execute_remains>0.2&buff.pyroclasm.stack=2|cooldown.fire_blast.charges_fractional>=2.9|buff.combustion.remains<action.pyroblast.cast_time)&(active_enemies<variable.sf_combustion_flamestrike|buff.pyroclasm.down|!action.flamestrike.executing)", "We prioritize starting to cast Pyroclasm (on 2 stacks) over Fire Blast if there's no risk of overcapping. In AoE, we also do not want to cast Fire Blast during a hardcast Flamestrike." );

  sf_filler->add_action( "meteor,if=talent.blast_zone&time>variable.combustion_delay", "Meteor is used immediately when ready after the first Combustion, if you have Blast Zone." );
  sf_filler->add_action( "flamestrike,if=(buff.hot_streak.react|prev_gcd.1.scorch&buff.heating_up.react&time-action.scorch.last_used<0.2|buff.hyperthermia.up)&active_enemies>=variable.sf_filler_flamestrike", "Spend Hot Streaks on Pyroblast in ST or Flamestrike in AoE. The Scorch condition is simply to simulate predictable guaranteed crits during execute." );
  sf_filler->add_action( "pyroblast,if=buff.hot_streak.react|prev_gcd.1.scorch&buff.heating_up.react&time-action.scorch.last_used<0.2|buff.hyperthermia.up" );
  sf_filler->add_action( "flamestrike,if=buff.pyroclasm.up&active_enemies>=variable.sf_filler_flamestrike", "Spend Pyroclasm immediately." );
  sf_filler->add_action( "pyroblast,if=buff.pyroclasm.up" );
  sf_filler->add_action( "scorch,if=talent.scald&scorch_execute.active|buff.heat_shimmer.react&(firestarter.active|prev_gcd.1.pyroblast|prev_gcd.1.flamestrike)", "Cast Scorch in execute or with a Heat Shimmer proc." );
  sf_filler->add_action( "fireball" );
  sf_filler->add_action( "call_action_list,name=fireblast" );
}
//fire_apl_end

//frost_apl_start
void frost( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* frostfire = p->get_action_priority_list( "frostfire" );
  action_priority_list_t* movement = p->get_action_priority_list( "movement" );
  action_priority_list_t* spellslinger = p->get_action_priority_list( "spellslinger" );

  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "summon_water_elemental" );
  precombat->add_action( "blizzard,if=talent.frostfire_bolt&active_enemies>=(8-3*talent.freezing_winds-2*talent.freezing_rain)|!talent.frostfire_bolt&active_enemies>=(7-2*talent.freezing_winds-2*talent.freezing_rain)", "Blizzard is only used as a precast when it is also hardcasted in the regular rotation at the given number of enemies." );
  precombat->add_action( "glacial_spike" );
  precombat->add_action( "frostbolt" );

  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "run_action_list,name=frostfire,if=talent.frostfire_bolt" );
  default_->add_action( "run_action_list,name=spellslinger" );

  cds->add_action( "variable,name=ff_trinket_timing,value=talent.frostfire_bolt&(fight_remains<15|prev_gcd.1.frozen_orb|prev_gcd.1.comet_storm|prev_gcd.1.glacial_spike|cooldown.ray_of_frost.charges>=1&debuff.freezing.react<12&buff.fingers_of_frost.react<2&buff.icicles.react<5&(!buff.frostfire_empowerment.react|active_enemies<=2)&(!buff.brain_freeze.react|buff.thermal_void.up))", "Frostfire uses potions, items and racials after Frozen Orb, Comet Storm or Glacial Spike, or together with Ray of Frost." );
  cds->add_action( "variable,name=ss_trinket_timing,value=!talent.frostfire_bolt&(fight_remains<15|prev_gcd.1.frozen_orb|time>1.5&cooldown.ray_of_frost.charges>=1&debuff.freezing.react<6&!buff.fingers_of_frost.react&buff.icicles.react<=3&(!buff.brain_freeze.react|buff.thermal_void.up))", "Spellslinger uses potions, items and racials after Frozen Orb or together with Ray of Frost." );
  cds->add_action( "use_item,name=nevermelting_ice_crystal,if=variable.ff_trinket_timing|variable.ss_trinket_timing", "Haste trinkets are used after using potion (of recklessness). Crit trinkets are used before using potion. Mastery trinkets are used after using potion if Crit is your highest secondary stat, and before otherwise." );
  cds->add_action( "use_item,name=freightrunners_flask,if=variable.ff_trinket_timing|variable.ss_trinket_timing" );
  cds->add_action( "use_item,name=vaelgors_final_stare,if=(variable.ff_trinket_timing|variable.ss_trinket_timing)&(stat.haste_rating>stat.crit_rating|stat.versatility_rating>stat.crit_rating)" );
  cds->add_action( "potion,if=variable.ff_trinket_timing|variable.ss_trinket_timing|fight_remains<35" );
  cds->add_action( "use_item,name=vaelgors_final_stare,if=variable.ff_trinket_timing|variable.ss_trinket_timing" );
  cds->add_action( "use_item,name=vile_vial_of_volatile_venom,if=variable.ff_trinket_timing|variable.ss_trinket_timing" );
  cds->add_action( "use_items" );
  cds->add_action( "blood_fury,if=variable.ff_trinket_timing|variable.ss_trinket_timing" );
  cds->add_action( "berserking,if=variable.ff_trinket_timing|variable.ss_trinket_timing" );
  cds->add_action( "fireblood,if=variable.ff_trinket_timing|variable.ss_trinket_timing" );
  cds->add_action( "ancestral_call,if=variable.ff_trinket_timing|variable.ss_trinket_timing" );
  cds->add_action( "ray_of_frost,if=talent.frostfire_bolt,interrupt_if=!talent.hand_of_frost_4&active_enemies>=2&tick_time>gcd.remains&(!talent.crystalline_refraction|talent.crystalline_refraction&buff.fingers_of_frost.react=2),interrupt_global=1,interrupt_immediate=1,line_cd=9999", "Frostfire Opener: precast Frostfire Bolt/Blizzard --> Ray of Frost --> Flurry --> Frozen Orb." );
  cds->add_action( "flurry,if=talent.frostfire_bolt&talent.wintertide,line_cd=9999" );
  cds->add_action( "frozen_orb,if=talent.frostfire_bolt,line_cd=9999" );
  cds->add_action( "flurry,if=!talent.frostfire_bolt&talent.wintertide,line_cd=9999", "Spellslinger Opener: precast Frostbolt/Blizzard --> Flurry --> Frozen Orb --> Ray of Frost." );
  cds->add_action( "frozen_orb,if=!talent.frostfire_bolt,line_cd=9999" );
  cds->add_action( "ray_of_frost,if=!talent.frostfire_bolt,line_cd=9999" );
  cds->add_action( "ray_of_frost,if=fight_remains<12|charges=2,interrupt_if=talent.frostfire_bolt&!talent.hand_of_frost_4&active_enemies>=2&tick_time>gcd.remains&(!talent.crystalline_refraction|talent.crystalline_refraction&buff.fingers_of_frost.react=2),interrupt_global=1,interrupt_immediate=1", "End-Of-Fight actions and overcap protection." );
  cds->add_action( "comet_storm,if=fight_remains<8" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down", "Externals." );

  frostfire->add_action( "glacial_spike,if=buff.glacial_spike.react|time-buff.rapid_refreezing.last_trigger<1.5", "These Glacial Spike conditions prevent super-human behaviour. Chaincasting 4p proccs as GS --> X --> GS --> X is reasonable, as long as the cast time/gcd of X is longer than 1 second." );
  frostfire->add_action( "comet_storm,if=active_enemies<=2|prev_gcd.2.glacial_spike&(time-buff.rapid_refreezing.last_trigger>1.5|!set_bonus.midnight_season_2_4pc)", "In AoE, Comet Storm can quickly refill Icicles through the 2pc bonus. Cast it second-to-last after Glacial Spike, allowing time to react to a 4pc proc first." );
  frostfire->add_action( "flurry,if=buff.brain_freeze.react&buff.thermal_void.down" );
  frostfire->add_action( "ice_lance,if=buff.fingers_of_frost.react=2" );
  frostfire->add_action( "ray_of_frost,if=!talent.hand_of_frost_4,interrupt_if=active_enemies>=2&tick_time>gcd.remains&(!talent.crystalline_refraction|talent.crystalline_refraction&buff.fingers_of_frost.react=2),interrupt_global=1,interrupt_immediate=1", "Without the final Apex Talent, Ray of Frost is cast at higher priority. Against 2+ targets, cancel Ray of Frost on your next gcd. With Crystalline Refraction, wait until 2 Fingers of Frost before cancelling it." );
  frostfire->add_action( "frozen_orb" );
  frostfire->add_action( "blizzard,if=active_enemies>=(8-3*talent.freezing_winds-2*talent.freezing_rain)|active_enemies>=3&buff.freezing_rain.up" );
  frostfire->add_action( "ice_lance,if=buff.fingers_of_frost.react&buff.thermal_void.up", "Single Finger of Frost procs are held unless Thermal Void is active. They may still get spent passively when the enemy has 12+ stacks of Freezing." );
  frostfire->add_action( "ice_lance,if=debuff.freezing.stack>=12" );
  frostfire->add_action( "flurry,if=cooldown_react" );
  frostfire->add_action( "ice_nova,if=active_enemies>=5&talent.cone_of_frost&!buff.frostfire_empowerment.react&cooldown.ray_of_frost.charges>=1" );
  frostfire->add_action( "cone_of_cold,if=active_enemies>=5&talent.cone_of_frost&!buff.frostfire_empowerment.react&cooldown.ray_of_frost.charges>=1" );
  frostfire->add_action( "ray_of_frost,if=active_enemies<=2|!buff.frostfire_empowerment.react", "With the final Apex Talent, spend Frostfire Empowerment in AoE before casting Ray of Frost. This builds Freezing stacks before the following Comet Storm." );
  frostfire->add_action( "glacial_spike" );
  frostfire->add_action( "frostbolt" );
  frostfire->add_action( "call_action_list,name=movement" );

  movement->add_action( "any_blink,if=movement.distance>5" );
  movement->add_action( "blizzard,if=buff.freezing_rain.up" );
  movement->add_action( "ice_nova,if=talent.cone_of_frost" );
  movement->add_action( "cone_of_cold,if=talent.cone_of_frost" );
  movement->add_action( "ice_lance" );

  spellslinger->add_action( "comet_storm" );
  spellslinger->add_action( "flurry,if=buff.brain_freeze.react&buff.thermal_void.down" );
  spellslinger->add_action( "ice_lance,if=buff.fingers_of_frost.react=2" );
  spellslinger->add_action( "frozen_orb" );
  spellslinger->add_action( "glacial_spike,if=buff.glacial_spike.react|time-buff.rapid_refreezing.last_trigger<1.5", "These Glacial Spike conditions prevent super-human behaviour. Chaincasting 4p proccs as GS --> X --> GS --> X is reasonable, as long as the cast time/gcd of X is longer than 1 second." );
  spellslinger->add_action( "blizzard,if=active_enemies>=(5-2*talent.freezing_winds)&buff.freezing_rain.up" );
  spellslinger->add_action( "ice_lance,if=buff.fingers_of_frost.react" );
  spellslinger->add_action( "ice_lance,if=debuff.freezing.react>=6" );
  spellslinger->add_action( "ray_of_frost,if=buff.icicles.react<=3|active_enemies>=3", "This Ray of Frost optimization improves ressource overflow slightly. It is neutral in AoE." );
  spellslinger->add_action( "flurry,if=cooldown_react" );
  spellslinger->add_action( "ice_nova,if=active_enemies>=4&talent.cone_of_frost" );
  spellslinger->add_action( "cone_of_cold,if=active_enemies>=4&talent.cone_of_frost" );
  spellslinger->add_action( "blizzard,if=active_enemies>=(7-2*talent.freezing_winds-2*talent.freezing_rain)" );
  spellslinger->add_action( "glacial_spike" );
  spellslinger->add_action( "frostbolt" );
  spellslinger->add_action( "call_action_list,name=movement" );
}
//frost_apl_end

}  // namespace mage_apl
