#include "class_modules/apl/apl_paladin.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace paladin_apl
{

//retribution_apl_start
void retribution( player_t* p )
{
  action_priority_list_t* default_      = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat     = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cooldowns     = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* es_fr_active  = p->get_action_priority_list( "es_fr_active" );
  action_priority_list_t* es_fr_pooling = p->get_action_priority_list( "es_fr_pooling" );
  action_priority_list_t* finishers     = p->get_action_priority_list( "finishers" );
  action_priority_list_t* generators    = p->get_action_priority_list( "generators" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "retribution_aura" );
  precombat->add_action( "fleshcraft,if=soulbind.pustule_eruption|soulbind.volatile_solvent" );
  precombat->add_action( "arcane_torrent,if=talent.final_reckoning&talent.seraphim" );
  precombat->add_action( "blessing_of_the_seasons" );
  precombat->add_action( "shield_of_vengeance" );

  default_->add_action( "auto_attack" );
  default_->add_action( "rebuke" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action(
      "call_action_list,name=es_fr_pooling,if=(!raid_event.adds.exists|raid_event.adds.up|raid_event.adds.in<9|raid_"
      "event.adds.in>30)&(talent.execution_sentence&cooldown.execution_sentence.remains<9&spell_targets.divine_storm<5|"
      "talent.final_reckoning&cooldown.final_reckoning.remains<9)&target.time_to_die>8" );
  default_->add_action(
      "call_action_list,name=es_fr_active,if=debuff.execution_sentence.up|debuff.final_reckoning.up" );
  default_->add_action( "call_action_list,name=generators" );

  cooldowns->add_action( "potion,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10|fight_remains<25" );
  cooldowns->add_action(
      "lights_judgment,if=spell_targets.lights_judgment>=2|!raid_event.adds.exists|raid_event.adds.in>75|raid_event."
      "adds.up" );
  cooldowns->add_action(
      "fireblood,if=(buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10)&!talent.execution_sentence" );
  cooldowns->add_action(
      "fleshcraft,if=soulbind.pustule_eruption|soulbind.volatile_solvent,interrupt_immediate=1,interrupt_global=1,"
      "interrupt_if=soulbind.volatile_solvent" );
  cooldowns->add_action(
      "shield_of_vengeance,if=(!talent.execution_sentence|cooldown.execution_sentence.remains<52)&fight_remains>15" );
  cooldowns->add_action( "blessing_of_the_seasons" );
  cooldowns->add_action( "use_item,name=gavel_of_the_first_arbiter" );
  cooldowns->add_action( "use_item,name=ring_of_collapsing_futures,if=!buff.temptation.up|fight_remains<15" );
  cooldowns->add_action( "use_item,name=anodized_deflectors" );
  cooldowns->add_action(
      "use_item,name=the_first_sigil,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10|fight_remains<"
      "20" );
  cooldowns->add_action(
      "use_item,name=enforcers_stun_grenade,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10|fight_"
      "remains<20" );
  cooldowns->add_action(
      "use_item,name=inscrutable_quantum_device,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10|fight_"
      "remains<30" );
  cooldowns->add_action(
      "use_item,name=overwhelming_power_crystal,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10|fight_"
      "remains<15" );
  cooldowns->add_action(
      "use_item,name=darkmoon_deck_voracity,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10|fight_"
      "remains<20" );
  cooldowns->add_action(
      "use_item,name=macabre_sheet_music,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10|fight_remains<"
      "20" );
  cooldowns->add_action( "use_item,name=faulty_countermeasure,if=!talent.crusade|buff.crusade.up|fight_remains<30" );
  cooldowns->add_action( "use_item,name=dreadfire_vessel" );
  cooldowns->add_action( "use_item,name=skulkers_wing" );
  cooldowns->add_action( "use_item,name=grim_codex" );
  cooldowns->add_action( "use_item,name=memory_of_past_sins" );
  cooldowns->add_action( "use_item,name=spare_meat_hook" );
  cooldowns->add_action( "use_item,name=salvaged_fusion_amplifier" );
  cooldowns->add_action( "use_item,name=giant_ornamental_pearl" );
  cooldowns->add_action( "use_item,name=windscar_whetstone" );
  cooldowns->add_action(
      "use_item,name=bloodstained_handkerchief,target_if=max:target.time_to_die*(!dot.cruel_garrote.ticking),if=!dot."
      "cruel_garrote.ticking" );
  cooldowns->add_action( "use_item,name=toe_knees_promise" );
  cooldowns->add_action( "use_item,name=remote_guidance_device" );
  cooldowns->add_action(
      "use_item,name=scars_of_fraternal_strife,if=!buff.scars_of_fraternal_strife_4.up|fight_remains<35" );
  cooldowns->add_action( "use_item,name=chains_of_domination" );
  cooldowns->add_action( "use_item,name=earthbreakers_impact" );
  cooldowns->add_action( "use_item,name=heart_of_the_swarm,if=!buff.avenging_wrath.up&!buff.crusade.up" );
  cooldowns->add_action(
      "use_item,name=cosmic_gladiators_badge_of_ferocity,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>="
      "10|cooldown.avenging_wrath.remains>45|cooldown.crusade.remains>45" );
  cooldowns->add_action(
      "use_item,name=cosmic_aspirants_badge_of_ferocity,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>="
      "10|cooldown.avenging_wrath.remains>45|cooldown.crusade.remains>45" );
  cooldowns->add_action(
      "use_item,name=unchained_gladiators_badge_of_ferocity,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade."
      "stack>=10|cooldown.avenging_wrath.remains>45|cooldown.crusade.remains>45" );
  cooldowns->add_action(
      "use_item,name=unchained_aspirants_badge_of_ferocity,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade."
      "stack>=10|cooldown.avenging_wrath.remains>45|cooldown.crusade.remains>45" );
  cooldowns->add_action(
      "use_item,name=sinful_gladiators_badge_of_ferocity,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>="
      "10|cooldown.avenging_wrath.remains>45|cooldown.crusade.remains>45" );
  cooldowns->add_action(
      "use_item,name=sinful_aspirants_badge_of_ferocity,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>="
      "10|cooldown.avenging_wrath.remains>45|cooldown.crusade.remains>45" );
  cooldowns->add_action(
      "avenging_wrath,if=(holy_power>=4&time<5|holy_power>=3&(time>5|runeforge.the_magistrates_judgment)|holy_power>=2&"
      "runeforge.vanguards_momentum&talent.final_reckoning|talent.holy_avenger&cooldown.holy_avenger.remains=0)&(!"
      "talent.seraphim|!talent.final_reckoning|cooldown.seraphim.remains>0)" );
  cooldowns->add_action( "crusade,if=holy_power>=5&time<5|holy_power>=3&time>5" );
  cooldowns->add_action( "ashen_hallow" );
  cooldowns->add_action(
      "holy_avenger,if=time_to_hpg=0&holy_power<=2&(buff.avenging_wrath.up|talent.crusade&(cooldown.crusade.remains=0|"
      "buff.crusade.up)|fight_remains<20)" );
  cooldowns->add_action(
      "final_reckoning,if=(holy_power>=4&time<8|holy_power>=3&(time>=8|spell_targets.divine_storm>=2&covenant.kyrian))&"
      "(cooldown.avenging_wrath.remains>gcd|cooldown.crusade.remains&(!buff.crusade.up|buff.crusade.stack>=10))&(time_"
      "to_hpg>0|holy_power=5)&(!talent.seraphim|buff.seraphim.up)&(!raid_event.adds.exists|raid_event.adds.up|raid_"
      "event.adds.in>40)&(!buff.avenging_wrath.up|holy_power=5|cooldown.hammer_of_wrath.remains|spell_targets.divine_"
      "storm>=2&covenant.kyrian)" );

  es_fr_active->add_action( "fireblood" );
  es_fr_active->add_action(
      "call_action_list,name=finishers,if=holy_power=5|debuff.judgment.up|debuff.final_reckoning.up&(debuff.final_"
      "reckoning.remains<gcd.max|spell_targets.divine_storm>=2&!talent.execution_sentence)|debuff.execution_sentence."
      "up&debuff.execution_sentence.remains<gcd.max" );
  es_fr_active->add_action( "divine_toll,if=holy_power<=2" );
  es_fr_active->add_action( "vanquishers_hammer" );
  es_fr_active->add_action(
      "wake_of_ashes,if=holy_power<=2&(debuff.final_reckoning.up&debuff.final_reckoning.remains<gcd*2&!runeforge."
      "divine_resonance|debuff.execution_sentence.up&debuff.execution_sentence.remains<gcd|spell_targets.divine_storm>="
      "5&runeforge.divine_resonance&talent.execution_sentence)" );
  es_fr_active->add_action(
      "blade_of_justice,if=conduit.expurgation&(!runeforge.divine_resonance&holy_power<=3|holy_power<=2)" );
  es_fr_active->add_action(
      "judgment,if=!debuff.judgment.up&(holy_power>=1&runeforge.the_magistrates_judgment|holy_power>=2)" );
  es_fr_active->add_action( "call_action_list,name=finishers" );
  es_fr_active->add_action( "wake_of_ashes,if=holy_power<=2" );
  es_fr_active->add_action( "blade_of_justice,if=holy_power<=3" );
  es_fr_active->add_action( "judgment,if=!debuff.judgment.up" );
  es_fr_active->add_action( "hammer_of_wrath" );
  es_fr_active->add_action( "crusader_strike" );
  es_fr_active->add_action( "arcane_torrent" );
  es_fr_active->add_action( "exorcism" );
  es_fr_active->add_action( "consecration" );

  es_fr_pooling->add_action(
      "seraphim,if=holy_power=5&(!talent.final_reckoning|cooldown.final_reckoning.remains<=gcd*3)&(!talent.execution_"
      "sentence|cooldown.execution_sentence.remains<=gcd*3|talent.final_reckoning)&(!covenant.kyrian|cooldown.divine_"
      "toll.remains<9)" );
  es_fr_pooling->add_action(
      "call_action_list,name=finishers,if=holy_power=5|debuff.final_reckoning.up|buff.crusade.up&buff.crusade.stack<"
      "10" );
  es_fr_pooling->add_action( "vanquishers_hammer,if=buff.seraphim.up" );
  es_fr_pooling->add_action( "hammer_of_wrath,if=runeforge.vanguards_momentum" );
  es_fr_pooling->add_action(
      "wake_of_ashes,if=holy_power<=2&talent.ashes_to_dust&(cooldown.crusade.remains|cooldown.avenging_wrath."
      "remains)" );
  es_fr_pooling->add_action( "blade_of_justice,if=holy_power<=3" );
  es_fr_pooling->add_action( "judgment,if=!debuff.judgment.up" );
  es_fr_pooling->add_action( "hammer_of_wrath" );
  es_fr_pooling->add_action(
      "crusader_strike,if=cooldown.crusader_strike.charges_fractional>=1.75&(holy_power<=2|holy_power<=3&cooldown."
      "blade_of_justice.remains>gcd*2|holy_power=4&cooldown.blade_of_justice.remains>gcd*2&cooldown.judgment.remains>"
      "gcd*2)" );
  es_fr_pooling->add_action(
      "seraphim,if=!talent.final_reckoning&cooldown.execution_sentence.remains<=gcd*3&(!covenant.kyrian|cooldown."
      "divine_toll.remains<9)" );
  es_fr_pooling->add_action( "call_action_list,name=finishers" );
  es_fr_pooling->add_action( "crusader_strike" );
  es_fr_pooling->add_action( "arcane_torrent,if=holy_power<=4" );
  es_fr_pooling->add_action( "exorcism" );
  es_fr_pooling->add_action(
      "seraphim,if=(!talent.final_reckoning|cooldown.final_reckoning.remains<=gcd*3)&(!talent.execution_sentence|"
      "cooldown.execution_sentence.remains<=gcd*3|talent.final_reckoning)&(!covenant.kyrian|cooldown.divine_toll."
      "remains<9)" );
  es_fr_pooling->add_action( "consecration" );

  finishers->add_action(
      "variable,name=ds_castable,value=spell_targets.divine_storm>=2|buff.empyrean_power.up&!debuff.judgment.up&!buff."
      "divine_purpose.up" );
  finishers->add_action(
      "seraphim,if=(cooldown.avenging_wrath.remains>15|cooldown.crusade.remains>15)&!talent.final_reckoning&(!talent."
      "execution_sentence|spell_targets.divine_storm>=5)&(!raid_event.adds.exists|raid_event.adds.in>40|raid_event."
      "adds.in<gcd|raid_event.adds.up)&(!covenant.kyrian|cooldown.divine_toll.remains<9)|fight_remains<15&fight_"
      "remains>5|buff.crusade.up&buff.crusade.stack<10" );
  finishers->add_action(
      "execution_sentence,if=(buff.crusade.down&cooldown.crusade.remains>10|buff.crusade.stack>=3|cooldown.avenging_"
      "wrath.remains>10)&(!talent.final_reckoning|cooldown.final_reckoning.remains>10)&target.time_to_die>8&(spell_"
      "targets.divine_storm<5|talent.executioners_wrath)" );
  finishers->add_action(
      "radiant_decree,if=(buff.crusade.down&cooldown.crusade.remains>5|buff.crusade.stack>=3|cooldown.avenging_wrath."
      "remains>5)&(!talent.final_reckoning|cooldown.final_reckoning.remains>5)" );
  finishers->add_action(
      "divine_storm,if=variable.ds_castable&!buff.vanquishers_hammer.up&((!talent.crusade|cooldown.crusade.remains>gcd*"
      "3)&(!talent.execution_sentence|cooldown.execution_sentence.remains>gcd*6|cooldown.execution_sentence.remains>"
      "gcd*4&holy_power>=4|target.time_to_die<8|spell_targets.divine_storm>=5|!talent.seraphim&cooldown.execution_"
      "sentence.remains>gcd*2)&(!talent.final_reckoning|cooldown.final_reckoning.remains>gcd*6|cooldown.final_"
      "reckoning.remains>gcd*4&holy_power>=4|!talent.seraphim&cooldown.final_reckoning.remains>gcd*2)|talent.holy_"
      "avenger&cooldown.holy_avenger.remains<gcd*3|buff.holy_avenger.up|buff.crusade.up&buff.crusade.stack<10)" );
  finishers->add_action(
      "templars_verdict,if=(!talent.crusade|cooldown.crusade.remains>gcd*3)&(!talent.execution_sentence|cooldown."
      "execution_sentence.remains>gcd*6|cooldown.execution_sentence.remains>gcd*4&holy_power>=4|target.time_to_die<8|!"
      "talent.seraphim&cooldown.execution_sentence.remains>gcd*2)&(!talent.final_reckoning|cooldown.final_reckoning."
      "remains>gcd*6|cooldown.final_reckoning.remains>gcd*4&holy_power>=4|!talent.seraphim&cooldown.final_reckoning."
      "remains>gcd*2)|talent.holy_avenger&cooldown.holy_avenger.remains<gcd*3|buff.holy_avenger.up|buff.crusade.up&"
      "buff.crusade.stack<10" );

  generators->add_action(
      "call_action_list,name=finishers,if=holy_power=5|(debuff.judgment.up|holy_power=4)&buff.divine_resonance.up|buff."
      "holy_avenger.up" );
  generators->add_action(
      "vanquishers_hammer,if=!runeforge.dutybound_gavel|!talent.final_reckoning&!talent.execution_sentence|fight_"
      "remains<8" );
  generators->add_action(
      "hammer_of_wrath,if=runeforge.the_mad_paragon|covenant.venthyr&cooldown.ashen_hallow.remains>210" );
  generators->add_action(
      "wake_of_ashes,if=holy_power<=2&talent.ashes_to_dust&(cooldown.avenging_wrath.remains|cooldown.crusade."
      "remains)" );
  generators->add_action(
      "divine_toll,if=holy_power<=2&!debuff.judgment.up&(!talent.seraphim|buff.seraphim.up)&(!raid_event.adds.exists|"
      "raid_event.adds.in>30|raid_event.adds.up)&!talent.final_reckoning&(!talent.execution_sentence|fight_remains<8|"
      "spell_targets.divine_storm>=5)&(cooldown.avenging_wrath.remains>15|cooldown.crusade.remains>15|fight_remains<"
      "8)" );
  generators->add_action(
      "judgment,if=!debuff.judgment.up&(holy_power>=1&runeforge.the_magistrates_judgment|holy_power>=2)" );
  generators->add_action(
      "wake_of_ashes,if=(holy_power=0|holy_power<=2&cooldown.blade_of_justice.remains>gcd*2)&(!raid_event.adds.exists|"
      "raid_event.adds.in>20|raid_event.adds.up)&(!talent.seraphim|cooldown.seraphim.remains>5|covenant.kyrian)&(!"
      "talent.execution_sentence|cooldown.execution_sentence.remains>15|target.time_to_die<8|spell_targets.divine_"
      "storm>=5)&(!talent.final_reckoning|cooldown.final_reckoning.remains>15|fight_remains<8)&(cooldown.avenging_"
      "wrath.remains|cooldown.crusade.remains)" );
  generators->add_action( "call_action_list,name=finishers,if=holy_power>=3&buff.crusade.up&buff.crusade.stack<10" );
  generators->add_action( "exorcism" );
  generators->add_action( "blade_of_justice,if=conduit.expurgation&holy_power<=3" );
  generators->add_action( "judgment,if=!debuff.judgment.up" );
  generators->add_action( "hammer_of_wrath" );
  generators->add_action( "blade_of_justice,if=holy_power<=3" );
  generators->add_action(
      "call_action_list,name=finishers,if=(target.health.pct<=20|buff.avenging_wrath.up|buff.crusade.up|buff.empyrean_"
      "power.up)" );
  generators->add_action( "consecration,if=!consecration.up&spell_targets.divine_storm>=2" );
  generators->add_action(
      "crusader_strike,if=cooldown.crusader_strike.charges_fractional>=1.75&(holy_power<=2|holy_power<=3&cooldown."
      "blade_of_justice.remains>gcd*2|holy_power=4&cooldown.blade_of_justice.remains>gcd*2&cooldown.judgment.remains>"
      "gcd*2)" );
  generators->add_action( "call_action_list,name=finishers" );
  generators->add_action( "consecration,if=!consecration.up" );
  generators->add_action( "crusader_strike" );
  generators->add_action( "arcane_torrent" );
  generators->add_action( "consecration" );
}
//retribution_apl_end

//protection_apl_start
