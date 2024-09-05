#include "class_modules/apl/mage.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace mage_apl {

std::string potion( const player_t* p )
{
  return p->true_level >= 80 ? "tempered_potion_3"
       : p->true_level >= 70 ? "elemental_potion_of_ultimate_power_3"
       : p->true_level >= 60 ? "spectral_intellect"
       : p->true_level >= 50 ? "superior_battle_potion_of_intellect"
       :                       "disabled";
}

std::string flask( const player_t* p )
{
  return p->true_level >= 80 ? "flask_of_alchemical_chaos_3"
       : p->true_level >= 70 ? "phial_of_tepid_versatility_3"
       : p->true_level >= 60 ? "spectral_flask_of_power"
       : p->true_level >= 50 ? "greater_flask_of_endless_fathoms"
       :                       "disabled";
}

std::string food( const player_t* p )
{
  return p->true_level >= 80 ? "everything_stew"
       : p->true_level >= 70 ? "fated_fortune_cookie"
       : p->true_level >= 60 ? "feast_of_gluttonous_hedonism"
       : p->true_level >= 50 ? "famine_evaluator_and_snack_table"
       :                       "disabled";
}

std::string rune( const player_t* p )
{
  return p->true_level >= 80 ? "crystallized"
       : p->true_level >= 70 ? "draconic"
       : p->true_level >= 60 ? "veiled"
       : p->true_level >= 50 ? "battle_scarred"
       :                       "disabled";
}

std::string temporary_enchant( const player_t* p )
{
  return p->true_level >= 80 ? "main_hand:algari_mana_oil_3"
       : p->true_level >= 70 ? "main_hand:buzzing_rune_3"
       : p->true_level >= 60 ? "main_hand:shadowcore_oil"
       :                       "disabled";
}

//arcane_apl_start
void arcane( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cd_opener = p->get_action_priority_list( "cd_opener" );
  action_priority_list_t* spellslinger_aoe = p->get_action_priority_list( "spellslinger_aoe" );
  action_priority_list_t* spellslinger = p->get_action_priority_list( "spellslinger" );
  action_priority_list_t* sunfury_aoe = p->get_action_priority_list( "sunfury_aoe" );
  action_priority_list_t* sunfury = p->get_action_priority_list( "sunfury" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "variable,name=aoe_target_count,op=reset,default=2" );
  precombat->add_action( "variable,name=aoe_target_count,op=set,value=9,if=!talent.arcing_cleave" );
  precombat->add_action( "variable,name=opener,op=set,value=1" );
  precombat->add_action( "variable,name=sunfury_aoe_list,default=0,op=reset" );
  precombat->add_action( "variable,name=steroid_trinket_equipped,op=set,value=equipped.gladiators_badge|equipped.signet_of_the_priory|equipped.high_speakers_accretion|equipped.spymasters_web|equipped.treacherous_transmitter|equipped.imperfect_ascendancy_serum" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "arcane_blast,if=!talent.evocation" );
  precombat->add_action( "evocation,if=talent.evocation" );

  default_->add_action( "counterspell" );
  default_->add_action( "potion,if=buff.siphon_storm.up|(!talent.evocation&cooldown.arcane_surge.ready)" );
  default_->add_action( "lights_judgment,if=(buff.arcane_surge.down&debuff.touch_of_the_magi.down&active_enemies>=2)" );
  default_->add_action( "berserking,if=(prev_gcd.1.arcane_surge&variable.opener)|((prev_gcd.1.arcane_surge&(fight_remains<80|target.health.pct<35|!talent.arcane_bombardment))|(prev_gcd.1.arcane_surge&!equipped.spymasters_web))" );
  default_->add_action( "blood_fury,if=(prev_gcd.1.arcane_surge&variable.opener)|((prev_gcd.1.arcane_surge&(fight_remains<80|target.health.pct<35|!talent.arcane_bombardment))|(prev_gcd.1.arcane_surge&!equipped.spymasters_web))" );
  default_->add_action( "fireblood,if=(prev_gcd.1.arcane_surge&variable.opener)|((prev_gcd.1.arcane_surge&(fight_remains<80|target.health.pct<35|!talent.arcane_bombardment))|(prev_gcd.1.arcane_surge&!equipped.spymasters_web))" );
  default_->add_action( "ancestral_call,if=(prev_gcd.1.arcane_surge&variable.opener)|((prev_gcd.1.arcane_surge&(fight_remains<80|target.health.pct<35|!talent.arcane_bombardment))|(prev_gcd.1.arcane_surge&!equipped.spymasters_web))" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=prev_gcd.1.arcane_surge", "Invoke Externals with cooldowns except Autumn which should come just after cooldowns" );
  default_->add_action( "invoke_external_buff,name=blessing_of_summer,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "invoke_external_buff,name=blessing_of_autumn,if=cooldown.touch_of_the_magi.remains>5" );
  default_->add_action( "use_items,if=prev_gcd.1.arcane_surge|prev_gcd.1.evocation|fight_remains<20|!variable.steroid_trinket_equipped", "Trinket specific use cases vary, default is just with cooldowns" );
  default_->add_action( "use_item,name=spymasters_web,if=(prev_gcd.1.arcane_surge|prev_gcd.1.evocation)&(fight_remains<80|target.health.pct<35|!talent.arcane_bombardment)|fight_remains<20" );
  default_->add_action( "use_item,name=high_speakers_accretion,if=(prev_gcd.1.arcane_surge|prev_gcd.1.evocation)|cooldown.evocation.remains<4|fight_remains<20" );
  default_->add_action( "use_item,name=imperfect_ascendancy_serum,if=cooldown.evocation.ready|cooldown.arcane_surge.ready|fight_remains<20" );
  default_->add_action( "use_item,name=treacherous_transmitter,if=buff.arcane_surge.remains>13|prev_gcd.1.evocation|cooldown.arcane_surge.remains<10|fight_remains<20" );
  default_->add_action( "do_treacherous_transmitter_task,use_off_gcd=1,if=buff.arcane_surge.remains>13|fight_remains<20" );
  default_->add_action( "use_item,name=aberrant_spellforge,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down|(equipped.spymasters_web&target.health.pct>35)" );
  default_->add_action( "use_item,name=mad_queens_mandate,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down" );
  default_->add_action( "use_item,name=fearbreakers_echo,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down" );
  default_->add_action( "use_item,name=mereldars_toll,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down" );
  default_->add_action( "variable,name=opener,op=set,if=debuff.touch_of_the_magi.up&variable.opener,value=0" );
  default_->add_action( "arcane_barrage,if=fight_remains<2" );
  default_->add_action( "call_action_list,name=cd_opener", "Enter cooldowns, then action list depending on your hero talent choices" );
  default_->add_action( "call_action_list,name=sunfury_aoe,if=talent.spellfire_spheres&variable.sunfury_aoe_list" );
  default_->add_action( "call_action_list,name=spellslinger_aoe,if=active_enemies>=(variable.aoe_target_count+talent.impetus)&!talent.spellfire_spheres" );
  default_->add_action( "call_action_list,name=sunfury,if=talent.spellfire_spheres" );
  default_->add_action( "call_action_list,name=spellslinger,if=!talent.spellfire_spheres" );
  default_->add_action( "arcane_barrage" );

  cd_opener->add_action( "touch_of_the_magi,use_off_gcd=1,if=prev_gcd.1.arcane_barrage&(action.arcane_barrage.in_flight_remains<=0.5|gcd.remains<=0.5)&(buff.arcane_surge.up|cooldown.arcane_surge.remains>30)|(prev_gcd.1.arcane_surge&buff.arcane_charge.stack<4)", "Touch of the Magi used when Arcane Barrage is mid-flight or if you just used Arcane Surge and you don't have 4 Arcane Charges, the wait simulates the time it takes to queue another spell after Touch when you Surge into Touch" );
  cd_opener->add_action( "wait,sec=0.05,if=prev_gcd.1.arcane_surge&time-action.touch_of_the_magi.last_used<0.015,line_cd=15" );
  cd_opener->add_action( "arcane_blast,if=buff.presence_of_mind.up" );
  cd_opener->add_action( "arcane_orb,if=talent.high_voltage&variable.opener,line_cd=10", "Use Orb for charges if you have High Voltage, then evocation, then Missiles for Nether Precision, then Arcane Surge" );
  cd_opener->add_action( "evocation,if=cooldown.arcane_surge.remains<(gcd.max*3)&cooldown.touch_of_the_magi.remains<(gcd.max*6)" );
  cd_opener->add_action( "arcane_missiles,if=variable.opener,interrupt_if=tick_time>gcd.remains,interrupt_immediate=1,interrupt_global=1,chain=1,line_cd=10" );
  cd_opener->add_action( "arcane_surge,if=cooldown.touch_of_the_magi.remains<gcd.max*3" );

  spellslinger_aoe->add_action( "supernova,if=buff.unerring_proficiency.stack=30" );
  spellslinger_aoe->add_action( "cancel_buff,name=presence_of_mind,use_off_gcd=1,if=(debuff.magis_spark_arcane_blast.up&time-action.arcane_blast.last_used>0.015)" );
  spellslinger_aoe->add_action( "shifting_power,if=(prev_gcd.1.arcane_barrage&(buff.arcane_surge.up|debuff.touch_of_the_magi.up|cooldown.evocation.remains<20)&talent.shifting_shards)", "Use Shifting Power whenever as long as you'll get some cooldown reduction on your cds, especially if you get a Time Anomaly proc, this usually works out to just using it off cooldown" );
  spellslinger_aoe->add_action( "arcane_orb,if=buff.arcane_charge.stack<2" );
  spellslinger_aoe->add_action( "arcane_blast,if=(debuff.magis_spark_arcane_blast.up&time-action.arcane_blast.last_used>0.015)", "Blast in AOE for Magi's Spark" );
  spellslinger_aoe->add_action( "arcane_barrage,if=(talent.arcane_tempo&buff.arcane_tempo.remains<gcd.max)|((buff.intuition.react&(buff.arcane_charge.stack=4|!talent.high_voltage))&buff.nether_precision.up)|(buff.nether_precision.up&action.arcane_blast.executing)" );
  spellslinger_aoe->add_action( "arcane_missiles,if=buff.clearcasting.react&((talent.high_voltage&buff.arcane_charge.stack<4)|buff.nether_precision.down),interrupt_if=tick_time>gcd.remains&buff.aether_attunement.down,interrupt_immediate=1,interrupt_global=1,chain=1", "Clearcasting is exclusively spent on Arcane Missiles in AOE and always interrupted after the global cooldown ends except for Aether Attunement" );
  spellslinger_aoe->add_action( "presence_of_mind,if=buff.arcane_charge.stack=3|buff.arcane_charge.stack=2", "Only use Presence of Mind at low charges, use these to get to 4 Charges, but cancelaura the buff if you need to queue Arcane Barrage" );
  spellslinger_aoe->add_action( "arcane_blast,if=buff.presence_of_mind.up" );
  spellslinger_aoe->add_action( "arcane_barrage,if=(buff.arcane_charge.stack=4)" );
  spellslinger_aoe->add_action( "arcane_explosion" );

  spellslinger->add_action( "shifting_power,if=((buff.arcane_surge.down&buff.siphon_storm.down&debuff.touch_of_the_magi.down&cooldown.evocation.remains>15&cooldown.touch_of_the_magi.remains>10)&(cooldown.arcane_orb.remains&action.arcane_orb.charges=0)&fight_remains>10)|(prev_gcd.1.arcane_barrage&(buff.arcane_surge.up|debuff.touch_of_the_magi.up|cooldown.evocation.remains<20))" );
  spellslinger->add_action( "cancel_buff,name=presence_of_mind,use_off_gcd=1,if=prev_gcd.1.arcane_blast&buff.presence_of_mind.stack=1", "In single target, use Presence of Mind at the very end of Touch of the Magi, then cancelaura the buff to start the cooldown, wait is to simulate the delay of hitting Presence of Mind after another spell cast" );
  spellslinger->add_action( "presence_of_mind,if=debuff.touch_of_the_magi.remains<=gcd.max&buff.nether_precision.up&active_enemies<variable.aoe_target_count&!talent.unerring_proficiency" );
  spellslinger->add_action( "wait,sec=0.05,if=buff.presence_of_mind.up&prev_gcd.1.arcane_blast,line_cd=15" );
  spellslinger->add_action( "supernova,if=debuff.touch_of_the_magi.remains<=gcd.max&buff.unerring_proficiency.stack=30" );
  spellslinger->add_action( "arcane_barrage,if=(buff.nether_precision.stack=1&time-action.arcane_blast.last_used<0.015)|(cooldown.touch_of_the_magi.ready)|(talent.arcane_tempo&buff.arcane_tempo.remains<gcd.max)", "Always queue Arcane Barrage on the second stack of Nether Precision" );
  spellslinger->add_action( "arcane_missiles,if=(buff.clearcasting.react&buff.nether_precision.down)|buff.clearcasting.react=3,interrupt_if=tick_time>gcd.remains&buff.aether_attunement.down,interrupt_immediate=1,interrupt_global=1,chain=1", "Missiles if you dont have Nether Precision or if you have 3 stacks to prevent munching, always clip off GCD unless you have Aether Attunement" );
  spellslinger->add_action( "arcane_orb,if=buff.arcane_charge.stack<2" );
  spellslinger->add_action( "arcane_blast" );
  spellslinger->add_action( "arcane_barrage" );

  sunfury_aoe->add_action( "arcane_barrage,if=(buff.arcane_soul.up&((buff.clearcasting.react<3)|buff.arcane_soul.remains<gcd.max))", "This list is only used with a variable for extra information, it is not a default list called." );
  sunfury_aoe->add_action( "arcane_missiles,if=buff.arcane_soul.up,interrupt_if=tick_time>gcd.remains&buff.aether_attunement.down,interrupt_immediate=1,interrupt_global=1,chain=1" );
  sunfury_aoe->add_action( "cancel_buff,name=presence_of_mind,use_off_gcd=1,if=(debuff.magis_spark_arcane_blast.up&time-action.arcane_blast.last_used>0.015)" );
  sunfury_aoe->add_action( "shifting_power,if=(buff.arcane_surge.down&buff.siphon_storm.down&debuff.touch_of_the_magi.down&cooldown.evocation.remains>15&cooldown.touch_of_the_magi.remains>15)&(cooldown.arcane_orb.remains&action.arcane_orb.charges=0)&fight_remains>10" );
  sunfury_aoe->add_action( "arcane_orb,if=buff.arcane_charge.stack<2&(!talent.high_voltage|!buff.clearcasting.react)" );
  sunfury_aoe->add_action( "arcane_blast,if=(debuff.magis_spark_arcane_blast.up&time-action.arcane_blast.last_used>0.015)" );
  sunfury_aoe->add_action( "arcane_barrage,if=(buff.arcane_charge.stack=buff.arcane_charge.max_stack)" );
  sunfury_aoe->add_action( "arcane_missiles,if=buff.clearcasting.react&(buff.aether_attunement.up|talent.arcane_harmony),interrupt_if=tick_time>gcd.remains&buff.aether_attunement.down,interrupt_immediate=1,interrupt_global=1,chain=1" );
  sunfury_aoe->add_action( "presence_of_mind,if=buff.arcane_charge.stack=3|buff.arcane_charge.stack=2" );
  sunfury_aoe->add_action( "arcane_explosion,if=talent.reverberate|buff.arcane_charge.stack<1" );
  sunfury_aoe->add_action( "arcane_blast" );
  sunfury_aoe->add_action( "arcane_barrage" );

  sunfury->add_action( "shifting_power,if=((buff.arcane_surge.down&buff.siphon_storm.down&debuff.touch_of_the_magi.down&cooldown.evocation.remains>15&cooldown.touch_of_the_magi.remains>10)&fight_remains>10)&buff.arcane_soul.down", "For Sunfury, Shifting Power only when you're not under the effect of any cooldowns" );
  sunfury->add_action( "cancel_buff,name=presence_of_mind,use_off_gcd=1,if=(debuff.magis_spark_arcane_blast.up&time-action.arcane_blast.last_used>0.015)|((prev_gcd.1.arcane_blast&buff.presence_of_mind.stack=1)|active_enemies<4)" );
  sunfury->add_action( "presence_of_mind,if=debuff.touch_of_the_magi.remains<=gcd.max&buff.nether_precision.up&active_enemies<4" );
  sunfury->add_action( "wait,sec=0.05,if=buff.presence_of_mind.up&prev_gcd.1.arcane_blast,line_cd=15" );
  sunfury->add_action( "arcane_barrage,if=((buff.arcane_charge.stack=4&(time-action.arcane_blast.last_used<0.015&buff.nether_precision.stack=1)&active_enemies>=(5-(2*(talent.arcane_bombardment&target.health.pct<35)))&talent.arcing_cleave&((talent.high_voltage&buff.clearcasting.react)|(cooldown.arcane_orb.remains<gcd.max|action.arcane_orb.charges>0))))|(buff.aether_attunement.up&talent.high_voltage&buff.clearcasting.react&buff.arcane_charge.stack>1&active_enemies>1)", "AOE Barrage is optimized for funnel at the cost of some overall AOE, tries to make sure you have Clearcasting if you have High Voltage or an Orb charge ready" );
  sunfury->add_action( "arcane_orb,if=buff.arcane_charge.stack<2&buff.arcane_soul.down&(!talent.high_voltage|buff.clearcasting.react=0)", "Orb if you don't have High Voltage and a Clearcasting in AOE" );
  sunfury->add_action( "arcane_barrage,if=((buff.glorious_incandescence.up|buff.intuition.react)&((time-action.arcane_blast.last_used<0.015&buff.nether_precision.stack=1)|(buff.nether_precision.down&buff.clearcasting.react=0)))|(buff.arcane_soul.up&((buff.clearcasting.react<3)|buff.arcane_soul.remains<gcd.max))|(buff.arcane_charge.stack=4&cooldown.touch_of_the_magi.ready)", "Barrage whenever Intuition/Incandescence and NP is up, double dipping if 1 stack, or just sending it if you dont have Clearcasting; also Barrage during Arcane Soul as long as you don't cap on Clearcasting procs, or if Touch is ready" );
  sunfury->add_action( "arcane_missiles,if=buff.clearcasting.react&((buff.nether_precision.down|(buff.clearcasting.react=3)|(talent.high_voltage&buff.arcane_charge.stack<3)|(buff.nether_precision.stack=1&time-action.arcane_blast.last_used<0.015))),interrupt_if=tick_time>gcd.remains&buff.aether_attunement.down,interrupt_immediate=1,interrupt_global=1,chain=1", "Missiles when it won't impact various Barrage conditions, interrupt the channel immediately after the GCD but not if you have Aether Attunement" );
  sunfury->add_action( "presence_of_mind,if=(buff.arcane_charge.stack=3|buff.arcane_charge.stack=2)&active_enemies>=3" );
  sunfury->add_action( "arcane_explosion,if=(talent.reverberate|buff.arcane_charge.stack<1)&active_enemies>=4", "Explosion to build the first charge if you have 0" );
  sunfury->add_action( "arcane_blast" );
  sunfury->add_action( "arcane_barrage" );
}
//arcane_apl_end

//fire_apl_start
void fire( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* active_talents = p->get_action_priority_list( "active_talents" );
  action_priority_list_t* combustion_cooldowns = p->get_action_priority_list( "combustion_cooldowns" );
  action_priority_list_t* combustion_phase = p->get_action_priority_list( "combustion_phase" );
  action_priority_list_t* combustion_timing = p->get_action_priority_list( "combustion_timing" );
  action_priority_list_t* firestarter_fire_blasts = p->get_action_priority_list( "firestarter_fire_blasts" );
  action_priority_list_t* standard_rotation = p->get_action_priority_list( "standard_rotation" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "variable,name=firestarter_combustion,default=-1,value=talent.sun_kings_blessing,if=variable.firestarter_combustion<0", "APL Variable Option: This variable specifies whether Combustion should be used during Firestarter." );
  precombat->add_action( "variable,name=hot_streak_flamestrike,if=variable.hot_streak_flamestrike=0,value=4*(talent.quickflame|talent.flame_patch)+999*(!talent.flame_patch&!talent.quickflame)", "APL Variable Option: This variable specifies the number of targets at which Hot Streak Flamestrikes outside of Combustion should be used." );
  precombat->add_action( "variable,name=hard_cast_flamestrike,if=variable.hard_cast_flamestrike=0,value=999", "APL Variable Option: This variable specifies the number of targets at which Hard Cast Flamestrikes outside of Combustion should be used as filler." );
  precombat->add_action( "variable,name=combustion_flamestrike,if=variable.combustion_flamestrike=0,value=4*(talent.quickflame|talent.flame_patch)+999*(!talent.flame_patch&!talent.quickflame)", "APL Variable Option: This variable specifies the number of targets at which Hot Streak Flamestrikes are used during Combustion." );
  precombat->add_action( "variable,name=skb_flamestrike,if=variable.skb_flamestrike=0,value=3*(talent.quickflame|talent.flame_patch)+999*(!talent.flame_patch&!talent.quickflame)", "APL Variable Option: This variable specifies the number of targets at which Flamestrikes should be used to consume Fury of the Sun King." );
  precombat->add_action( "variable,name=arcane_explosion,if=variable.arcane_explosion=0,value=999", "APL Variable Option: This variable specifies the number of targets at which Arcane Explosion outside of Combustion should be used." );
  precombat->add_action( "variable,name=arcane_explosion_mana,default=40,op=reset", "APL Variable Option: This variable specifies the percentage of mana below which Arcane Explosion will not be used." );
  precombat->add_action( "variable,name=combustion_shifting_power,if=variable.combustion_shifting_power=0,value=999", "APL Variable Option: The number of targets at which Shifting Power can used during Combustion." );
  precombat->add_action( "variable,name=combustion_cast_remains,default=0.3,op=reset", "APL Variable Option: The time remaining on a cast when Combustion can be used in seconds." );
  precombat->add_action( "variable,name=overpool_fire_blasts,default=0,op=reset", "APL Variable Option: This variable specifies the number of seconds of Fire Blast that should be pooled past the default amount." );
  precombat->add_action( "variable,name=skb_duration,value=dbc.effect.1016075.base_value", "The duration of a Sun King's Blessing Combustion." );
  precombat->add_action( "variable,name=combustion_on_use,value=equipped.gladiators_badge|equipped.moonlit_prism|equipped.irideus_fragment|equipped.spoils_of_neltharus|equipped.timebreaching_talon|equipped.horn_of_valor", "Whether a usable item used to buff Combustion is equipped." );
  precombat->add_action( "variable,name=on_use_cutoff,value=20,if=variable.combustion_on_use", "How long before Combustion should trinkets that trigger a shared category cooldown on other trinkets not be used?" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "flamestrike,if=active_enemies>=variable.hot_streak_flamestrike" );
  precombat->add_action( "pyroblast" );

  default_->add_action( "counterspell" );
  default_->add_action( "phoenix_flames,if=time=0" );
  default_->add_action( "call_action_list,name=combustion_timing", "The combustion_timing action list schedules the approximate time when Combustion should be used and stores the number of seconds until then in variable.time_to_combustion." );
  default_->add_action( "potion,if=buff.potion.duration>variable.time_to_combustion+buff.combustion.duration" );
  default_->add_action( "variable,name=shifting_power_before_combustion,value=variable.time_to_combustion>cooldown.shifting_power.remains", "Variable that estimates whether Shifting Power will be used before the next Combustion." );
  default_->add_action( "variable,name=item_cutoff_active,value=(variable.time_to_combustion<variable.on_use_cutoff|buff.combustion.remains>variable.skb_duration&!cooldown.item_cd_1141.remains)&((trinket.1.has_cooldown&trinket.1.cooldown.remains<variable.on_use_cutoff)+(trinket.2.has_cooldown&trinket.2.cooldown.remains<variable.on_use_cutoff)>1)" );
  default_->add_action( "use_item,effect_name=treacherous_transmitter,if=buff.combustion.remains>10|fight_remains<25", "The War Within S1 On-Use items with special use timings" );
  default_->add_action( "use_item,name=imperfect_ascendancy_serum,if=variable.time_to_combustion<3" );
  default_->add_action( "use_item,effect_name=spymasters_web,if=(buff.combustion.remains>10&fight_remains<60)|fight_remains<25" );
  default_->add_action( "use_item,effect_name=gladiators_badge,if=variable.time_to_combustion>cooldown-5" );
  default_->add_action( "use_items,if=!variable.item_cutoff_active" );
  default_->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=fire_blast_pooling,value=buff.combustion.down&action.fire_blast.charges_fractional+(variable.time_to_combustion+action.shifting_power.full_reduction*variable.shifting_power_before_combustion)%cooldown.fire_blast.duration-1<cooldown.fire_blast.max_charges+variable.overpool_fire_blasts%cooldown.fire_blast.duration-(buff.combustion.duration%cooldown.fire_blast.duration)%%1&variable.time_to_combustion<fight_remains", "Pool as many Fire Blasts as possible for Combustion." );
  default_->add_action( "call_action_list,name=combustion_phase,if=variable.time_to_combustion<=0|buff.combustion.up|variable.time_to_combustion<variable.combustion_precast_time&cooldown.combustion.remains<variable.combustion_precast_time" );
  default_->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=fire_blast_pooling,value=scorch_execute.active&action.fire_blast.full_recharge_time>3*gcd.max,if=!variable.fire_blast_pooling&talent.sun_kings_blessing", "Adjust the variable that controls Fire Blast usage to save Fire Blasts while Searing Touch is active with Sun King's Blessing." );
  default_->add_action( "shifting_power,if=buff.combustion.down&(!improved_scorch.active|debuff.improved_scorch.remains>cast_time+action.scorch.cast_time&!buff.fury_of_the_sun_king.up)&!buff.hot_streak.react&buff.hyperthermia.down&(cooldown.phoenix_flames.charges<=1|cooldown.combustion.remains<20)" );
  default_->add_action( "variable,name=phoenix_pooling,if=!talent.sun_kings_blessing,value=(variable.time_to_combustion+buff.combustion.duration-5<action.phoenix_flames.full_recharge_time+cooldown.phoenix_flames.duration-action.shifting_power.full_reduction*variable.shifting_power_before_combustion&variable.time_to_combustion<fight_remains|talent.sun_kings_blessing)&!talent.alexstraszas_fury", "Variable that controls Phoenix Flames usage to ensure its charges are pooled for Combustion when needed. Only use Phoenix Flames outside of Combustion when full charges can be obtained during the next Combustion." );
  default_->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!variable.fire_blast_pooling&variable.time_to_combustion>0&active_enemies>=variable.hard_cast_flamestrike&!firestarter.active&!buff.hot_streak.react&(buff.heating_up.react&action.flamestrike.execute_remains<0.5|charges_fractional>=2)", "When Hardcasting Flamestrike, Fire Blasts should be used to generate Hot Streaks and to extend Feel the Burn." );
  default_->add_action( "call_action_list,name=firestarter_fire_blasts,if=buff.combustion.down&firestarter.active&variable.time_to_combustion>0" );
  default_->add_action( "fire_blast,use_while_casting=1,if=action.shifting_power.executing&(full_recharge_time<action.shifting_power.tick_reduction|talent.sun_kings_blessing&buff.heating_up.react)", "Avoid capping Fire Blast charges while channeling Shifting Power" );
  default_->add_action( "call_action_list,name=standard_rotation,if=variable.time_to_combustion>0&buff.combustion.down" );
  default_->add_action( "ice_nova,if=!scorch_execute.active" );
  default_->add_action( "scorch,if=buff.combustion.down" );

  active_talents->add_action( "meteor,if=buff.combustion.up|(buff.sun_kings_blessing.max_stack-buff.sun_kings_blessing.stack>4|variable.time_to_combustion<=0|buff.combustion.remains>travel_time|!talent.sun_kings_blessing&(cooldown.meteor.duration<variable.time_to_combustion&fight_remains<variable.time_to_combustion))" );
  active_talents->add_action( "dragons_breath,if=talent.alexstraszas_fury&(buff.combustion.down&!buff.hot_streak.react)&(buff.feel_the_burn.up|time>15)&(!improved_scorch.active)", "With Alexstrasza's Fury when Combustion is not active, Dragon's Breath should be used to convert Heating Up to a Hot Streak." );

  combustion_cooldowns->add_action( "potion" );
  combustion_cooldowns->add_action( "blood_fury" );
  combustion_cooldowns->add_action( "berserking,if=buff.combustion.up" );
  combustion_cooldowns->add_action( "fireblood" );
  combustion_cooldowns->add_action( "ancestral_call" );
  combustion_cooldowns->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down" );
  combustion_cooldowns->add_action( "invoke_external_buff,name=blessing_of_summer,if=buff.blessing_of_summer.down" );
  combustion_cooldowns->add_action( "use_item,effect_name=gladiators_badge" );

  combustion_phase->add_action( "call_action_list,name=combustion_cooldowns,if=buff.combustion.remains>variable.skb_duration|fight_remains<20", "Other cooldowns that should be used with Combustion should only be used with an actual Combustion cast and not with a Sun King's Blessing proc." );
  combustion_phase->add_action( "call_action_list,name=active_talents" );
  combustion_phase->add_action( "flamestrike,if=buff.combustion.down&buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.remains>cast_time&buff.fury_of_the_sun_king.expiration_delay_remains=0&cooldown.combustion.remains<cast_time&active_enemies>=variable.skb_flamestrike", "If Combustion is down, precast something before activating it." );
  combustion_phase->add_action( "pyroblast,if=buff.combustion.down&buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.remains>cast_time&(buff.fury_of_the_sun_king.expiration_delay_remains=0|buff.flame_accelerant.up)" );
  combustion_phase->add_action( "meteor,if=talent.isothermic_core&buff.combustion.down&cooldown.combustion.remains<cast_time" );
  combustion_phase->add_action( "fireball,if=buff.combustion.down&cooldown.combustion.remains<cast_time&active_enemies<2&!improved_scorch.active&!(talent.sun_kings_blessing&talent.flame_accelerant)" );
  combustion_phase->add_action( "scorch,if=buff.combustion.down&cooldown.combustion.remains<cast_time" );
  combustion_phase->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=hot_streak_spells_in_flight=0&buff.combustion.down&variable.time_to_combustion<=0&(action.scorch.executing&action.scorch.execute_remains<variable.combustion_cast_remains|action.fireball.executing&action.fireball.execute_remains<variable.combustion_cast_remains|action.pyroblast.executing&action.pyroblast.execute_remains<variable.combustion_cast_remains|action.flamestrike.executing&action.flamestrike.execute_remains<variable.combustion_cast_remains|action.meteor.in_flight&action.meteor.in_flight_remains<variable.combustion_cast_remains)", "Combustion should be used when the precast is almost finished or when Meteor is about to land." );
  combustion_phase->add_action( "variable,name=TA_combust,value=cooldown.combustion.remains<10&buff.combustion.up", "Variable to determine which Fire Blast conditions are used." );
  combustion_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=(!variable.TA_combust|talent.sun_kings_blessing)&!variable.fire_blast_pooling&(!improved_scorch.active|action.scorch.executing|debuff.improved_scorch.remains>4*gcd.max)&(buff.fury_of_the_sun_king.down|action.pyroblast.executing)&buff.combustion.up&!buff.hot_streak.react&hot_streak_spells_in_flight+buff.heating_up.react*(gcd.remains>0)<2", "Fire Blast usage for a standard combustion" );
  combustion_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=variable.TA_combust&!variable.fire_blast_pooling&charges_fractional>2.5&(!improved_scorch.active|action.scorch.executing|debuff.improved_scorch.remains>4*gcd.max)&(buff.fury_of_the_sun_king.down|action.pyroblast.executing)&buff.combustion.up&!buff.hot_streak.react&hot_streak_spells_in_flight+buff.heating_up.react*(gcd.remains>0)<2", "Fire Blast usage for a TA combustion, when a regular combustion is soon to follow." );
  combustion_phase->add_action( "cancel_buff,name=hyperthermia,if=buff.fury_of_the_sun_king.react", "Cancelaura HT if SKB is ready" );
  combustion_phase->add_action( "flamestrike,if=(buff.hot_streak.react&active_enemies>=variable.combustion_flamestrike)|(buff.hyperthermia.react&active_enemies>=variable.combustion_flamestrike-talent.hyperthermia)", "Spend Hot Streaks during Combustion at high priority." );
  combustion_phase->add_action( "pyroblast,if=buff.hyperthermia.react" );
  combustion_phase->add_action( "pyroblast,if=buff.hot_streak.react&buff.combustion.up" );
  combustion_phase->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies<variable.combustion_flamestrike&buff.combustion.up" );
  combustion_phase->add_action( "flamestrike,if=buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.remains>cast_time&active_enemies>=variable.skb_flamestrike&buff.fury_of_the_sun_king.expiration_delay_remains=0", "Spend Fury of the Sun King procs inside of combustion." );
  combustion_phase->add_action( "pyroblast,if=buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.remains>cast_time&buff.fury_of_the_sun_king.expiration_delay_remains=0" );
  combustion_phase->add_action( "phoenix_flames,if=talent.phoenix_reborn&buff.heating_up.react+hot_streak_spells_in_flight<2&buff.flames_fury.up" );
  combustion_phase->add_action( "fireball,if=buff.frostfire_empowerment.up&!buff.hot_streak.react&!buff.excess_frost.up" );
  combustion_phase->add_action( "scorch,if=improved_scorch.active&(debuff.improved_scorch.remains<4*gcd.max)&active_enemies<variable.combustion_flamestrike" );
  combustion_phase->add_action( "scorch,if=buff.heat_shimmer.react&(talent.scald|talent.improved_scorch)&active_enemies<variable.combustion_flamestrike" );
  combustion_phase->add_action( "phoenix_flames,if=(!talent.call_of_the_sun_king&travel_time<buff.combustion.remains|(talent.call_of_the_sun_king&buff.combustion.remains<4|buff.sun_kings_blessing.stack<8))&buff.heating_up.react+hot_streak_spells_in_flight<2", "Use Phoenix Flames and Scorch in Combustion to help generate Hot Streaks when Fire Blasts are not available or need to be conserved." );
  combustion_phase->add_action( "fireball,if=buff.frostfire_empowerment.up&!buff.hot_streak.react" );
  combustion_phase->add_action( "scorch,if=buff.combustion.remains>cast_time&cast_time>=gcd.max" );
  combustion_phase->add_action( "fireball,if=buff.combustion.remains>cast_time" );

  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=combustion_ready_time,value=cooldown.combustion.remains*expected_kindling_reduction", "Helper variable that contains the actual estimated time that the next Combustion will be ready." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=combustion_precast_time,value=action.fireball.cast_time*(active_enemies<variable.combustion_flamestrike)+action.flamestrike.cast_time*(active_enemies>=variable.combustion_flamestrike)-variable.combustion_cast_remains", "The cast time of the spell that will be precast into Combustion." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,value=variable.combustion_ready_time" );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=firestarter.remains,if=talent.firestarter&!variable.firestarter_combustion", "Delay Combustion for after Firestarter unless variable.firestarter_combustion is set." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=(buff.sun_kings_blessing.max_stack-buff.sun_kings_blessing.stack)*(3*gcd.max),if=talent.sun_kings_blessing&firestarter.active&buff.fury_of_the_sun_king.down", "Delay Combustion until SKB is ready during Firestarter" );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=cooldown.gladiators_badge_345228.remains,if=equipped.gladiators_badge&cooldown.gladiators_badge_345228.remains-20<variable.time_to_combustion", "Delay Combustion for Gladiators Badge, unless it would be delayed longer than 20 seconds." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=buff.combustion.remains", "Delay Combustion until Combustion expires if it's up." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,op=max,value=raid_event.adds.in,if=raid_event.adds.exists&raid_event.adds.count>=3&raid_event.adds.duration>15", "Raid Events: Delay Combustion for add spawns of 3 or more adds that will last longer than 15 seconds. These values aren't necessarily optimal in all cases." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,value=raid_event.vulnerable.in*!raid_event.vulnerable.up,if=raid_event.vulnerable.exists&variable.combustion_ready_time<raid_event.vulnerable.in", "Raid Events: Always use Combustion with vulnerability raid events, override any delays listed above to make sure it gets used here." );
  combustion_timing->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=time_to_combustion,value=variable.combustion_ready_time,if=variable.combustion_ready_time+cooldown.combustion.duration*(1-(0.4+0.2*talent.firestarter)*talent.kindling)<=variable.time_to_combustion|variable.time_to_combustion>fight_remains-20", "Use the next Combustion on cooldown if it would not be expected to delay the scheduled one or the scheduled one would happen less than 20 seconds before the fight ends." );

  firestarter_fire_blasts->add_action( "fire_blast,use_while_casting=1,if=!variable.fire_blast_pooling&!buff.hot_streak.react&(action.fireball.execute_remains>gcd.remains|action.pyroblast.executing)&buff.heating_up.react+hot_streak_spells_in_flight=1&(cooldown.shifting_power.ready|charges>1|buff.feel_the_burn.remains<2*gcd.max)", "While casting Fireball or Pyroblast, convert Heating Up to a Hot Streak!" );
  firestarter_fire_blasts->add_action( "fire_blast,use_off_gcd=1,if=!variable.fire_blast_pooling&buff.heating_up.react+hot_streak_spells_in_flight=1&(talent.feel_the_burn&buff.feel_the_burn.remains<gcd.remains|cooldown.shifting_power.ready)&time>0", "If not casting anything, use Fire Blast to trigger Hot Streak! only if Feel the Burn is talented and would expire before the GCD ends or if Shifting Power is available." );

  standard_rotation->add_action( "flamestrike,if=active_enemies>=variable.hot_streak_flamestrike&(buff.hot_streak.react|buff.hyperthermia.react)" );
  standard_rotation->add_action( "fireball,if=buff.hot_streak.up&buff.hyperthermia.down&!cooldown.shifting_power.ready&cooldown.phoenix_flames.charges<1&!scorch_execute.active&!prev_gcd.1.fireball,line_cd=2*gcd.max", "When resources are low, fish for Hot Streaks." );
  standard_rotation->add_action( "pyroblast,if=(buff.hyperthermia.react|buff.hot_streak.react&(buff.hot_streak.remains<action.fireball.execute_time)|buff.hot_streak.react&(hot_streak_spells_in_flight|firestarter.active|talent.call_of_the_sun_king&action.phoenix_flames.charges)|buff.hot_streak.react&scorch_execute.active)" );
  standard_rotation->add_action( "flamestrike,if=active_enemies>=variable.skb_flamestrike&buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.expiration_delay_remains=0" );
  standard_rotation->add_action( "scorch,if=improved_scorch.active&debuff.improved_scorch.remains<action.pyroblast.cast_time+5*gcd.max&buff.fury_of_the_sun_king.up&!action.scorch.in_flight" );
  standard_rotation->add_action( "pyroblast,if=buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.expiration_delay_remains=0" );
  standard_rotation->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!firestarter.active&(!variable.fire_blast_pooling|talent.spontaneous_combustion)&buff.fury_of_the_sun_king.down&(((action.fireball.executing&(action.fireball.execute_remains<0.5|!talent.hyperthermia)|action.pyroblast.executing&(action.pyroblast.execute_remains<0.5))&buff.heating_up.react)|(scorch_execute.active&(!improved_scorch.active|debuff.improved_scorch.stack=debuff.improved_scorch.max_stack|full_recharge_time<3)&(buff.heating_up.react&!action.scorch.executing|!buff.hot_streak.react&!buff.heating_up.react&action.scorch.executing&!hot_streak_spells_in_flight)))", "During the standard rotation, only use Fire Blasts when they are not being pooled for  Combustion. Use Fire Blast either during a Fireball/Pyroblast cast when Heating Up is active or during execute with Searing Touch." );
  standard_rotation->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!firestarter.active&(!variable.fire_blast_pooling|talent.spontaneous_combustion)&buff.fury_of_the_sun_king.down&(buff.heating_up.up&hot_streak_spells_in_flight<1&(prev_gcd.1.phoenix_flames|prev_gcd.1.scorch))|(((buff.bloodlust.up&charges_fractional>1.5)|charges_fractional>2.5|buff.feel_the_burn.remains<0.5|full_recharge_time*1-(0.5*cooldown.shifting_power.ready)<buff.hyperthermia.duration)&buff.heating_up.react)", "We will munch Fireblasts during Hyperthermia, and use them after instant casts in filler." );
  standard_rotation->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&scorch_execute.active&active_enemies<variable.hot_streak_flamestrike" );
  standard_rotation->add_action( "scorch,if=improved_scorch.active&debuff.improved_scorch.remains<gcd.max" );
  standard_rotation->add_action( "fireball,if=buff.frostfire_empowerment.up&!buff.hot_streak.react&!buff.excess_frost.up" );
  standard_rotation->add_action( "scorch,if=buff.heat_shimmer.react&(talent.scald|talent.improved_scorch)&active_enemies<variable.combustion_flamestrike" );
  standard_rotation->add_action( "phoenix_flames,if=!buff.hot_streak.up&(hot_streak_spells_in_flight<1&(!prev_gcd.1.fireball|(buff.heating_up.down&buff.hot_streak.down)))|(hot_streak_spells_in_flight<2&buff.flames_fury.react)" );
  standard_rotation->add_action( "call_action_list,name=active_talents" );
  standard_rotation->add_action( "dragons_breath,if=active_enemies>1&talent.alexstraszas_fury" );
  standard_rotation->add_action( "scorch,if=(scorch_execute.active|buff.heat_shimmer.react)" );
  standard_rotation->add_action( "arcane_explosion,if=active_enemies>=variable.arcane_explosion&mana.pct>=variable.arcane_explosion_mana" );
  standard_rotation->add_action( "flamestrike,if=active_enemies>=variable.hard_cast_flamestrike", "With enough targets, it is a gain to cast Flamestrike as filler instead of Fireball. This is currently never true up to 10t." );
  standard_rotation->add_action( "fireball" );
}
//fire_apl_end

//frost_apl_start
void frost( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* ss_cleave = p->get_action_priority_list( "ss_cleave" );
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* movement = p->get_action_priority_list( "movement" );
  action_priority_list_t* ss_st = p->get_action_priority_list( "ss_st" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "blizzard,if=active_enemies>=2&talent.ice_caller&!talent.fractured_frost|active_enemies>=3" );
  precombat->add_action( "frostbolt,if=active_enemies<=2" );

  default_->add_action( "counterspell" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "run_action_list,name=aoe,if=active_enemies>=7&!set_bonus.tier30_2pc|active_enemies>=4&talent.ice_caller" );
  default_->add_action( "run_action_list,name=ss_cleave,if=active_enemies>=2&active_enemies<=3&talent.splinterstorm" );
  default_->add_action( "run_action_list,name=cleave,if=active_enemies>=2&active_enemies<=3" );
  default_->add_action( "run_action_list,name=ss_st,if=talent.splinterstorm" );
  default_->add_action( "run_action_list,name=st" );

  aoe->add_action( "cone_of_cold,if=talent.coldest_snap&(prev_gcd.1.comet_storm|prev_gcd.1.frozen_orb&!talent.comet_storm)" );
  aoe->add_action( "frozen_orb,if=(!prev_gcd.1.cone_of_cold|!talent.isothermic_core)&(!prev_gcd.1.glacial_spike|!freezable)" );
  aoe->add_action( "blizzard,if=!prev_gcd.1.glacial_spike|!freezable" );
  aoe->add_action( "frostbolt,if=buff.icy_veins.up&(buff.deaths_chill.stack<9|buff.deaths_chill.stack=9&!action.frostbolt.in_flight)&buff.icy_veins.remains>8&talent.deaths_chill" );
  aoe->add_action( "comet_storm,if=!prev_gcd.1.glacial_spike&(!talent.coldest_snap|cooldown.cone_of_cold.ready&cooldown.frozen_orb.remains>25|(cooldown.cone_of_cold.remains>10&talent.frostfire_bolt|cooldown.cone_of_cold.remains>20&!talent.frostfire_bolt))" );
  aoe->add_action( "freeze,if=freezable&debuff.frozen.down&(!talent.glacial_spike|prev_gcd.1.glacial_spike)" );
  aoe->add_action( "ice_nova,if=freezable&!prev_off_gcd.freeze&(prev_gcd.1.glacial_spike)" );
  aoe->add_action( "frost_nova,if=freezable&!prev_off_gcd.freeze&(prev_gcd.1.glacial_spike&!remaining_winters_chill)" );
  aoe->add_action( "shifting_power,if=cooldown.comet_storm.remains>10" );
  aoe->add_action( "frostbolt,if=buff.frostfire_empowerment.react&!buff.excess_frost.react&!buff.excess_fire.react" );
  aoe->add_action( "flurry,if=cooldown_react&!remaining_winters_chill&(buff.brain_freeze.react&!talent.excess_frost|buff.excess_frost.react)" );
  aoe->add_action( "ice_lance,if=buff.fingers_of_frost.react|debuff.frozen.remains>travel_time|remaining_winters_chill" );
  aoe->add_action( "flurry,if=cooldown_react&!remaining_winters_chill" );
  aoe->add_action( "ice_nova,if=active_enemies>=4&(!talent.glacial_spike|!freezable)&!talent.frostfire_bolt" );
  aoe->add_action( "cone_of_cold,if=!talent.coldest_snap&active_enemies>=7" );
  aoe->add_action( "frostbolt" );
  aoe->add_action( "call_action_list,name=movement" );

  cds->add_action( "use_item,name=imperfect_ascendancy_serum,if=buff.icy_veins.remains>19|fight_remains<25" );
  cds->add_action( "use_item,name=spymasters_web,if=(buff.icy_veins.remains>19&fight_remains<100)|fight_remains<25" );
  cds->add_action( "potion,if=prev_off_gcd.icy_veins|fight_remains<60" );
  cds->add_action( "use_item,name=dreambinder_loom_of_the_great_cycle,if=(equipped.nymues_unraveling_spindle&prev_gcd.1.nymues_unraveling_spindle)|fight_remains>2" );
  cds->add_action( "use_item,name=belorrelos_the_suncaller,if=time>5&!prev_gcd.1.flurry" );
  cds->add_action( "flurry,if=time=0&active_enemies<=2" );
  cds->add_action( "icy_veins" );
  cds->add_action( "use_items" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down" );
  cds->add_action( "invoke_external_buff,name=blessing_of_summer,if=buff.blessing_of_summer.down" );
  cds->add_action( "blood_fury" );
  cds->add_action( "berserking" );
  cds->add_action( "lights_judgment" );
  cds->add_action( "fireblood" );
  cds->add_action( "ancestral_call" );

  ss_cleave->add_action( "flurry,target_if=min:debuff.winters_chill.stack,if=cooldown_react&remaining_winters_chill=0&debuff.winters_chill.down&(prev_gcd.1.frostbolt|prev_gcd.1.glacial_spike)" );
  ss_cleave->add_action( "ice_lance,target_if=max:debuff.winters_chill.stack,if=buff.icy_veins.up&debuff.winters_chill.stack=2" );
  ss_cleave->add_action( "ray_of_frost,if=buff.icy_veins.down&buff.freezing_winds.down&remaining_winters_chill=1" );
  ss_cleave->add_action( "frozen_orb" );
  ss_cleave->add_action( "shifting_power" );
  ss_cleave->add_action( "ice_lance,target_if=max:debuff.winters_chill.stack,if=remaining_winters_chill|buff.fingers_of_frost.react" );
  ss_cleave->add_action( "comet_storm,if=prev_gcd.1.flurry|prev_gcd.1.cone_of_cold|action.splinterstorm.in_flight" );
  ss_cleave->add_action( "glacial_spike,if=buff.icicles.react=5" );
  ss_cleave->add_action( "flurry,target_if=min:debuff.winters_chill.stack,if=cooldown_react&buff.icy_veins.up" );
  ss_cleave->add_action( "frostbolt" );
  ss_cleave->add_action( "call_action_list,name=movement" );
  
  cleave->add_action( "comet_storm,if=prev_gcd.1.flurry|prev_gcd.1.cone_of_cold" );
  cleave->add_action( "flurry,target_if=min:debuff.winters_chill.stack,if=cooldown_react&(((prev_gcd.1.frostbolt|prev_gcd.1.frostfire_bolt)&buff.icicles.react>=3)|prev_gcd.1.glacial_spike|(buff.icicles.react>=3&buff.icicles.react<5&charges_fractional=2))" );
  cleave->add_action( "ice_lance,target_if=max:debuff.winters_chill.stack,if=talent.glacial_spike&debuff.winters_chill.down&buff.icicles.react=4&buff.fingers_of_frost.react" );
  cleave->add_action( "ray_of_frost,target_if=max:debuff.winters_chill.stack,if=remaining_winters_chill=1" );
  cleave->add_action( "glacial_spike,if=buff.icicles.react=5&(action.flurry.cooldown_react|remaining_winters_chill)" );
  cleave->add_action( "frozen_orb,if=buff.fingers_of_frost.react<2&(!talent.ray_of_frost|cooldown.ray_of_frost.remains)" );
  cleave->add_action( "cone_of_cold,if=talent.coldest_snap&cooldown.comet_storm.remains>10&cooldown.frozen_orb.remains>10&remaining_winters_chill=0&active_enemies>=3" );
  cleave->add_action( "shifting_power,if=cooldown.frozen_orb.remains>10&(!talent.comet_storm|cooldown.comet_storm.remains>10)&(!talent.ray_of_frost|cooldown.ray_of_frost.remains>10)|cooldown.icy_veins.remains<20" );
  cleave->add_action( "glacial_spike,if=buff.icicles.react=5" );
  cleave->add_action( "ice_lance,target_if=max:debuff.winters_chill.stack,if=buff.fingers_of_frost.react&!prev_gcd.1.glacial_spike|remaining_winters_chill" );
  cleave->add_action( "ice_nova,if=active_enemies>=4" );
  cleave->add_action( "frostbolt" );
  cleave->add_action( "call_action_list,name=movement" );

  movement->add_action( "any_blink,if=movement.distance>10" );
  movement->add_action( "ice_floes,if=buff.ice_floes.down" );
  movement->add_action( "ice_nova" );
  movement->add_action( "cone_of_cold,if=!talent.coldest_snap&active_enemies>=2" );
  movement->add_action( "arcane_explosion,if=mana.pct>30&active_enemies>=2" );
  movement->add_action( "fire_blast" );
  movement->add_action( "ice_lance" );

  ss_st->add_action( "flurry,if=cooldown_react&remaining_winters_chill=0&debuff.winters_chill.down&(prev_gcd.1.frostbolt|prev_gcd.1.glacial_spike)" );
  ss_st->add_action( "ice_lance,if=buff.icy_veins.up&debuff.winters_chill.stack=2" );
  ss_st->add_action( "ray_of_frost,if=buff.icy_veins.down&buff.freezing_winds.down&remaining_winters_chill=1" );
  ss_st->add_action( "frozen_orb" );
  ss_st->add_action( "shifting_power" );
  ss_st->add_action( "ice_lance,if=remaining_winters_chill|buff.fingers_of_frost.react" );
  ss_st->add_action( "comet_storm,if=prev_gcd.1.flurry|prev_gcd.1.cone_of_cold|action.splinterstorm.in_flight" );
  ss_st->add_action( "glacial_spike,if=buff.icicles.react=5" );
  ss_st->add_action( "flurry,if=cooldown_react&buff.icy_veins.up" );
  ss_st->add_action( "frostbolt" );
  ss_st->add_action( "call_action_list,name=movement" );

  st->add_action( "comet_storm,if=prev_gcd.1.flurry|prev_gcd.1.cone_of_cold" );
  st->add_action( "flurry,if=cooldown_react&remaining_winters_chill=0&debuff.winters_chill.down&(((prev_gcd.1.frostbolt|prev_gcd.1.frostfire_bolt)&buff.icicles.react>=3|(prev_gcd.1.frostbolt|prev_gcd.1.frostfire_bolt)&buff.brain_freeze.react)|prev_gcd.1.glacial_spike|talent.glacial_spike&buff.icicles.react=4&!buff.fingers_of_frost.react)|buff.excess_frost.up&buff.frostfire_empowerment.up" );
  st->add_action( "ice_lance,if=talent.glacial_spike&debuff.winters_chill.down&buff.icicles.react=4&buff.fingers_of_frost.react" );
  st->add_action( "ray_of_frost,if=remaining_winters_chill=1" );
  st->add_action( "glacial_spike,if=buff.icicles.react=5&(action.flurry.cooldown_react|remaining_winters_chill)" );
  st->add_action( "frozen_orb,if=buff.fingers_of_frost.react<2&(!talent.ray_of_frost|cooldown.ray_of_frost.remains)" );
  st->add_action( "cone_of_cold,if=talent.coldest_snap&cooldown.comet_storm.remains>10&cooldown.frozen_orb.remains>10&remaining_winters_chill=0&active_enemies>=3" );
  st->add_action( "blizzard,if=active_enemies>=2&talent.ice_caller&talent.freezing_rain&(!talent.splintering_cold&!talent.ray_of_frost|buff.freezing_rain.up|active_enemies>=3)" );
  st->add_action( "shifting_power,if=(buff.icy_veins.down|!talent.deaths_chill)&cooldown.frozen_orb.remains>10&(!talent.comet_storm|cooldown.comet_storm.remains>10)&(!talent.ray_of_frost|cooldown.ray_of_frost.remains>10)|cooldown.icy_veins.remains<20" );
  st->add_action( "glacial_spike,if=buff.icicles.react=5" );
  st->add_action( "ice_lance,if=buff.fingers_of_frost.react&!prev_gcd.1.glacial_spike|remaining_winters_chill" );
  st->add_action( "ice_nova,if=active_enemies>=4" );
  st->add_action( "frostbolt" );
  st->add_action( "call_action_list,name=movement" );
}
//frost_apl_end

}  // namespace mage_apl
