#include "class_modules/apl/mage.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace mage_apl {

std::string potion( const player_t* p )
{
  return p->true_level >= 70 ? "elemental_potion_of_ultimate_power_3"
       : p->true_level >= 60 ? "spectral_intellect"
       : p->true_level >= 50 ? "superior_battle_potion_of_intellect"
       :                       "disabled";
}

std::string flask( const player_t* p )
{
  return p->true_level >= 70 ? "phial_of_tepid_versatility_3"
       : p->true_level >= 60 ? "spectral_flask_of_power"
       : p->true_level >= 50 ? "greater_flask_of_endless_fathoms"
       :                       "disabled";
}

std::string food( const player_t* p )
{
  return p->true_level >= 70 ? "fated_fortune_cookie"
       : p->true_level >= 60 ? "feast_of_gluttonous_hedonism"
       : p->true_level >= 50 ? "famine_evaluator_and_snack_table"
       :                       "disabled";
}

std::string rune( const player_t* p )
{
  return p->true_level >= 70 ? "draconic"
       : p->true_level >= 60 ? "veiled"
       : p->true_level >= 50 ? "battle_scarred"
       :                       "disabled";
}

std::string temporary_enchant( const player_t* p )
{
  std::string lvl70_temp_enchant = p->specialization() == MAGE_FIRE ? "main_hand:howling_rune_3" : "main_hand:buzzing_rune_3";

  return p->true_level >= 70 ? lvl70_temp_enchant
       : p->true_level >= 60 ? "main_hand:shadowcore_oil"
       :                       "disabled";
}

//arcane_apl_start
void arcane( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cd_opener = p->get_action_priority_list( "cd_opener" );
  action_priority_list_t* rotation_aoe = p->get_action_priority_list( "rotation_aoe" );
  action_priority_list_t* rotation_default = p->get_action_priority_list( "rotation_default" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "variable,name=aoe_target_count,op=reset,default=2" );
  precombat->add_action( "variable,name=aoe_target_count,op=set,value=9,if=!talent.arcing_cleave" );
  precombat->add_action( "variable,name=opener,op=set,value=1" );
  precombat->add_action( "variable,name=alt_rotation,op=set,if=talent.high_voltage,value=1" );
  precombat->add_action( "variable,name=steroid_trinket_equipped,op=set,value=equipped.gladiators_badge|equipped.irideus_fragment|equipped.spoils_of_neltharus|equipped.timebreaching_talon|equipped.ashes_of_the_embersoul|equipped.nymues_unraveling_spindle|equipped.signet_of_the_priory|equipped.high_speakers_accretion|equipped.spymasters_web|equipped.treacherous_transmitter", "Variable indicates use of a trinket that boosts stats during burst" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "arcane_blast,if=!talent.evocation" );
  precombat->add_action( "evocation,if=talent.evocation" );

  default_->add_action( "counterspell" );
  default_->add_action( "potion,if=buff.siphon_storm.up|(!talent.evocation&cooldown.arcane_surge.ready)" );
  default_->add_action( "lights_judgment,if=buff.arcane_surge.down&debuff.touch_of_the_magi.down&active_enemies>=2" );
  default_->add_action( "berserking,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "blood_fury,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "fireblood,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "ancestral_call,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=prev_gcd.1.arcane_surge", "PI/Summer after Radiant Spark when cooldowns are coming up, Autumn after Touch of the Magi cd starts" );
  default_->add_action( "invoke_external_buff,name=blessing_of_summer,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "invoke_external_buff,name=blessing_of_autumn,if=cooldown.touch_of_the_magi.remains>5" );
  default_->add_action( "use_items,if=prev_gcd.1.arcane_surge|prev_gcd.1.evocation|fight_remains<20|!variable.steroid_trinket_equipped" );
  default_->add_action( "use_item,name=high_speakers_accretion,if=(prev_gcd.1.arcane_surge|prev_gcd.1.evocation)|cooldown.evocation.remains<7|fight_remains<20" );
  default_->add_action( "use_item,name=treacherous_transmitter,if=((prev_gcd.1.arcane_surge|prev_gcd.1.evocation)&variable.opener)|cooldown.evocation.remains<6|fight_remains<20" );
  default_->add_action( "do_treacherous_transmitter_task,use_off_gcd=1,if=buff.siphon_storm.up|fight_remains<20" );
  default_->add_action( "use_item,name=spymasters_web,if=(prev_gcd.1.arcane_surge|prev_gcd.1.evocation)&(fight_remains<80|target.health.pct<35|!talent.arcane_bombardment)|fight_remains<20" );
  default_->add_action( "use_item,name=aberrant_spellforge,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down|(equipped.spymasters_web&target.health.pct>35)" );
  default_->add_action( "use_item,name=mad_queens_mandate,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down" );
  default_->add_action( "use_item,name=mereldars_toll,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down" );
  default_->add_action( "use_item,name=nymues_unraveling_spindle,if=cooldown.arcane_surge.remains<=(gcd.max*4)|cooldown.arcane_surge.ready|fight_remains<=24" );
  default_->add_action( "use_item,name=belorrelos_the_suncaller,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down" );
  default_->add_action( "use_item,name=beacon_to_the_beyond,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down" );
  default_->add_action( "use_item,name=iceblood_deathsnare,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down" );
  default_->add_action( "use_item,name=desperate_invokers_codex,if=!variable.steroid_trinket_equipped|buff.siphon_storm.down" );
  default_->add_action( "use_item,name=conjured_chillglobe,if=mana.pct>65&!variable.steroid_trinket_equipped|buff.siphon_storm.down" );
  default_->add_action( "use_item,name=dreambinder_loom_of_the_great_cycle" );
  default_->add_action( "use_item,name=iridal_the_earths_master,use_off_gcd=1,if=gcd.remains" );
  default_->add_action( "variable,name=opener,op=set,if=debuff.touch_of_the_magi.up&variable.opener,value=0" );
  default_->add_action( "arcane_barrage,if=fight_remains<2" );
  default_->add_action( "call_action_list,name=cd_opener", "Enter cooldown phase when cds are available or coming off cooldown otherwise default to rotation priority" );
  default_->add_action( "call_action_list,name=rotation_aoe,if=active_enemies>=(variable.aoe_target_count+talent.impetus+talent.splintering_sorcery)" );
  default_->add_action( "call_action_list,name=rotation_default" );
  default_->add_action( "arcane_barrage" );

  cd_opener->add_action( "touch_of_the_magi,use_off_gcd=1,if=prev_gcd.1.arcane_barrage&(action.arcane_barrage.in_flight_remains<=0.5|gcd.remains<=0.5)" );
  cd_opener->add_action( "supernova,if=debuff.touch_of_the_magi.remains<=gcd.max&buff.unerring_proficiency.stack=30", "Use PoM or Supernova if you have Unerring Proficiency to end Touch of the Magi windows - if PoM then cancel it after to trigger the cooldown to align with future Touch windows" );
  cd_opener->add_action( "cancel_buff,name=presence_of_mind,use_off_gcd=1,if=prev_gcd.1.arcane_blast&buff.presence_of_mind.stack=1" );
  cd_opener->add_action( "presence_of_mind,if=debuff.touch_of_the_magi.remains<=gcd.max&buff.nether_precision.up&active_enemies<variable.aoe_target_count&!talent.unerring_proficiency" );
  cd_opener->add_action( "wait,sec=0.05,if=buff.presence_of_mind.up,line_cd=15" );
  cd_opener->add_action( "arcane_blast,if=buff.presence_of_mind.up" );
  cd_opener->add_action( "arcane_orb,if=variable.opener,line_cd=10" );
  cd_opener->add_action( "evocation,if=cooldown.arcane_surge.remains<gcd.max*2" );
  cd_opener->add_action( "arcane_missiles,if=variable.opener,interrupt_if=!gcd.remains,interrupt_immediate=1,interrupt_global=1,line_cd=10", "Use the Clearcasting from Evocation to trigger Nether Precision, Harmony, charges from High Voltage, and increment Aether" );
  cd_opener->add_action( "arcane_surge" );
  cd_opener->add_action( "shifting_power,if=((buff.arcane_surge.down&buff.siphon_storm.down&debuff.touch_of_the_magi.down&cooldown.evocation.remains>15&cooldown.touch_of_the_magi.remains>15)&(cooldown.arcane_orb.remains&action.arcane_orb.charges=0)&fight_remains>10)|(prev_gcd.1.arcane_barrage&(buff.arcane_surge.up|debuff.touch_of_the_magi.up|cooldown.evocation.remains<20)&talent.shifting_shards),interrupt_if=(cooldown.evocation.ready&cooldown.arcane_surge.remains<3),interrupt_immediate=1,interrupt_global=1", "Use Shifting Power whenever all major cooldowns will fully benefit, add Arcane Orb to the list if its AOE, as Spellslinger you can use this in cooldowns with no damage lost" );
  cd_opener->add_action( "arcane_orb,if=buff.arcane_charge.stack<2&(cooldown.touch_of_the_magi.remains>18|!active_enemies>=variable.aoe_target_count)", "Pool Arcane Orb for Touch of the Magi in AOE, otherwise just use to recover charges when you're low" );

  rotation_aoe->add_action( "arcane_blast,if=active_enemies>=variable.aoe_target_count&debuff.touch_of_the_magi.up&talent.magis_spark,line_cd=15", "Cast Blast in AOE if you have Magi's Spark" );
  rotation_aoe->add_action( "arcane_barrage,if=(talent.arcane_tempo&buff.arcane_tempo.remains<gcd.max)|((buff.intuition.up&(buff.arcane_charge.stack=buff.arcane_charge.max_stack|!variable.alt_rotation))&buff.nether_precision.up)|(buff.nether_precision.up&action.arcane_blast.executing)", "Use Barrage to maintain Arcane Tempo, to double dip Nether Precision when you use the above line, or if you get a 4pc proc" );
  rotation_aoe->add_action( "arcane_missiles,if=buff.clearcasting.react&((variable.alt_rotation&buff.arcane_charge.stack<buff.arcane_charge.max_stack)|buff.aether_attunement.up|talent.arcane_harmony)&((variable.alt_rotation&buff.arcane_charge.stack<buff.arcane_charge.max_stack)|!buff.nether_precision.up),interrupt_if=!gcd.remains,interrupt_immediate=1,interrupt_global=1,chain=1", "Use Missiles to regenerate charges, generate Nether Precision, and stack Harmony/Aether, but we always interrupt it as soon as possible." );
  rotation_aoe->add_action( "arcane_barrage,if=buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  rotation_aoe->add_action( "arcane_explosion" );

  rotation_default->add_action( "arcane_missiles,if=buff.clearcasting.react&(buff.nether_precision.down|(buff.clearcasting.stack=3&!talent.splintering_sorcery)|(variable.alt_rotation&buff.nether_precision.stack=1&buff.arcane_charge.stack<4)),interrupt_if=!gcd.remains&(!variable.alt_rotation|buff.arcane_charge.stack=buff.arcane_charge.max_stack),interrupt_immediate=1,interrupt_global=1,chain=1", "Use Missiles to generate charges and Nether Precision, interrupt if you get 4 charges" );
  rotation_default->add_action( "arcane_barrage,if=(buff.arcane_charge.stack=buff.arcane_charge.max_stack&((buff.nether_precision.stack=1&((buff.clearcasting.up|action.arcane_orb.charges>0)&time-action.arcane_blast.last_used<0.015)&buff.arcane_harmony.stack>12)|(cooldown.touch_of_the_magi.ready&(buff.nether_precision.up|!talent.magis_spark))))|(talent.arcane_tempo&buff.arcane_tempo.remains<(gcd.max*2))|buff.intuition.up", "Queue Barrage on the second Nether Precision stack under certain conditions, ensure you have nether precision before doing this to start Touch window, maintain Tempo and use 4pc as soon as possible" );
  rotation_default->add_action( "arcane_blast,if=buff.nether_precision.stack=2|(buff.nether_precision.stack=1&!prev_gcd.1.arcane_blast)" );
  rotation_default->add_action( "arcane_barrage,if=buff.arcane_surge.down&(mana.pct<70&cooldown.arcane_surge.remains>45&cooldown.touch_of_the_magi.remains>6)|(mana.deficit>(mana.max-action.arcane_blast.cost))|cooldown.touch_of_the_magi.ready|(cooldown.shifting_power.ready&cooldown.arcane_orb.ready)", "Conserve mana above 70% when Evocation is further than your next Touch" );
  rotation_default->add_action( "arcane_blast,if=!talent.splintering_sorcery|(buff.arcane_charge.stack>2&buff.nether_precision.down)", "Blast for filler if you're not in execute or you already have some charges from another effect" );
  rotation_default->add_action( "arcane_barrage" );
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
  precombat->add_action( "variable,name=steroid_trinket_equipped,op=set,value=equipped.gladiators_badge|equipped.irideus_fragment|equipped.erupting_spear_fragment|equipped.spoils_of_neltharus|equipped.timebreaching_talon|equipped.horn_of_valor|equipped.mirror_of_fractured_tomorrows|equipped.ashes_of_the_embersoul|equipped.balefire_branch|equipped.time_theifs_gambit|equipped.nymues_unraveling_spindle", "steroid trinkets" );
  precombat->add_action( "variable,name=firestarter_combustion,default=-1,value=talent.sun_kings_blessing,if=variable.firestarter_combustion<0", "APL Variable Option: This variable specifies whether Combustion should be used during Firestarter." );
  precombat->add_action( "variable,name=hot_streak_flamestrike,if=variable.hot_streak_flamestrike=0,value=4*talent.flame_patch+999*(!talent.flame_patch|!talent.quickflame)", "APL Variable Option: This variable specifies the number of targets at which Hot Streak Flamestrikes outside of Combustion should be used." );
  precombat->add_action( "variable,name=hard_cast_flamestrike,if=variable.hard_cast_flamestrike=0,value=999", "APL Variable Option: This variable specifies the number of targets at which Hard Cast Flamestrikes outside of Combustion should be used as filler." );
  precombat->add_action( "variable,name=combustion_flamestrike,if=variable.combustion_flamestrike=0,value=4*talent.flame_patch+999*(!talent.flame_patch|!talent.quickflame)", "APL Variable Option: This variable specifies the number of targets at which Hot Streak Flamestrikes are used during Combustion." );
  precombat->add_action( "variable,name=skb_flamestrike,if=variable.skb_flamestrike=0,value=3*(talent.quickflame|talent.flame_patch)+999*(!talent.flame_patch|!talent.quickflame)", "APL Variable Option: This variable specifies the number of targets at which Flamestrikes should be used to consume Fury of the Sun King.  Restricting this variable to be true only if Fuel the Fire is talented." );
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
  default_->add_action( "time_warp" );
  default_->add_action( "call_action_list,name=combustion_timing", "The combustion_timing action list schedules the approximate time when Combustion should be used and stores the number of seconds until then in variable.time_to_combustion." );
  default_->add_action( "potion,if=buff.potion.duration>variable.time_to_combustion+buff.combustion.duration" );
  default_->add_action( "variable,name=shifting_power_before_combustion,value=variable.time_to_combustion>cooldown.shifting_power.remains", "Variable that estimates whether Shifting Power will be used before the next Combustion." );
  default_->add_action( "variable,name=item_cutoff_active,value=(variable.time_to_combustion<variable.on_use_cutoff|buff.combustion.remains>variable.skb_duration&!cooldown.item_cd_1141.remains)&((trinket.1.has_cooldown&trinket.1.cooldown.remains<variable.on_use_cutoff)+(trinket.2.has_cooldown&trinket.2.cooldown.remains<variable.on_use_cutoff)>1)" );
  default_->add_action( "use_item,name=irideus_fragment,if=(variable.time_to_combustion<=3&buff.fury_of_the_sun_king.up)|(buff.combustion.up&buff.combustion.remains>11)" );
  default_->add_action( "use_item,name=belorrelos_the_suncaller,if=(!variable.steroid_trinket_equipped&buff.combustion.down)|(variable.steroid_trinket_equipped&trinket.1.has_cooldown&trinket.1.cooldown.remains>20&buff.combustion.down)|(variable.steroid_trinket_equipped&trinket.2.has_cooldown&trinket.2.cooldown.remains>20&buff.combustion.down)" );
  default_->add_action( "use_item,effect_name=gladiators_badge,if=variable.time_to_combustion>cooldown-5" );
  default_->add_action( "use_items,if=!variable.item_cutoff_active" );
  default_->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=fire_blast_pooling,value=buff.combustion.down&action.fire_blast.charges_fractional+(variable.time_to_combustion+action.shifting_power.full_reduction*variable.shifting_power_before_combustion)%cooldown.fire_blast.duration-1<cooldown.fire_blast.max_charges+variable.overpool_fire_blasts%cooldown.fire_blast.duration-(buff.combustion.duration%cooldown.fire_blast.duration)%%1&variable.time_to_combustion<fight_remains", "Pool as many Fire Blasts as possible for Combustion." );
  default_->add_action( "call_action_list,name=combustion_phase,if=variable.time_to_combustion<=0|buff.combustion.up|variable.time_to_combustion<variable.combustion_precast_time&cooldown.combustion.remains<variable.combustion_precast_time" );
  default_->add_action( "variable,use_off_gcd=1,use_while_casting=1,name=fire_blast_pooling,value=scorch_execute.active&action.fire_blast.full_recharge_time>3*gcd.max,if=!variable.fire_blast_pooling&talent.sun_kings_blessing", "Adjust the variable that controls Fire Blast usage to save Fire Blasts while Searing Touch is active with Sun King's Blessing." );
  default_->add_action( "shifting_power,if=buff.combustion.down&(action.fire_blast.charges=0|variable.fire_blast_pooling)&(!improved_scorch.active|debuff.improved_scorch.remains>cast_time+action.scorch.cast_time&!buff.fury_of_the_sun_king.up)&!buff.hot_streak.react&variable.shifting_power_before_combustion" );
  default_->add_action( "variable,name=phoenix_pooling,if=active_enemies<variable.combustion_flamestrike,value=(variable.time_to_combustion+buff.combustion.duration-5<action.phoenix_flames.full_recharge_time+cooldown.phoenix_flames.duration-action.shifting_power.full_reduction*variable.shifting_power_before_combustion&variable.time_to_combustion<fight_remains|talent.sun_kings_blessing)&!talent.call_of_the_sun_king", "Variable that controls Phoenix Flames usage to ensure its charges are pooled for Combustion when needed. Only use Phoenix Flames outside of Combustion when full charges can be obtained during the next Combustion." );
  default_->add_action( "variable,name=phoenix_pooling,if=active_enemies>=variable.combustion_flamestrike,value=(variable.time_to_combustion<action.phoenix_flames.full_recharge_time-action.shifting_power.full_reduction*variable.shifting_power_before_combustion&variable.time_to_combustion<fight_remains|talent.sun_kings_blessing)&!talent.call_of_the_sun_king", "When using Flamestrike in Combustion, save as many charges as possible for Combustion without capping." );
  default_->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!variable.fire_blast_pooling&variable.time_to_combustion>0&active_enemies>=variable.hard_cast_flamestrike&!firestarter.active&!buff.hot_streak.react&(buff.heating_up.react&action.flamestrike.execute_remains<0.5|charges_fractional>=2)", "When Hardcasting Flamestrike, Fire Blasts should be used to generate Hot Streaks and to extend Feel the Burn." );
  default_->add_action( "call_action_list,name=firestarter_fire_blasts,if=buff.combustion.down&firestarter.active&variable.time_to_combustion>0" );
  default_->add_action( "fire_blast,use_while_casting=1,if=action.shifting_power.executing&full_recharge_time<action.shifting_power.tick_reduction", "Avoid capping Fire Blast charges while channeling Shifting Power" );
  default_->add_action( "call_action_list,name=standard_rotation,if=variable.time_to_combustion>0&buff.combustion.down" );
  default_->add_action( "ice_nova,if=!scorch_execute.active", "Ice Nova can be used during movement when Searing Touch is not active." );
  default_->add_action( "scorch" );

  active_talents->add_action( "meteor,if=variable.time_to_combustion<=0|buff.combustion.remains>travel_time|!talent.sun_kings_blessing&(cooldown.meteor.duration<variable.time_to_combustion|fight_remains<variable.time_to_combustion)" );
  active_talents->add_action( "dragons_breath,if=talent.alexstraszas_fury&(buff.combustion.down&!buff.hot_streak.react)&(buff.feel_the_burn.up|time>15)&(!improved_scorch.active)", "With Alexstrasza's Fury when Combustion is not active, Dragon's Breath should be used to convert Heating Up to a Hot Streak." );

  combustion_cooldowns->add_action( "potion" );
  combustion_cooldowns->add_action( "blood_fury" );
  combustion_cooldowns->add_action( "berserking,if=buff.combustion.up" );
  combustion_cooldowns->add_action( "fireblood" );
  combustion_cooldowns->add_action( "ancestral_call" );
  combustion_cooldowns->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down" );
  combustion_cooldowns->add_action( "invoke_external_buff,name=blessing_of_summer,if=buff.blessing_of_summer.down" );
  combustion_cooldowns->add_action( "use_item,effect_name=gladiators_badge" );

  combustion_phase->add_action( "lights_judgment,if=buff.combustion.down" );
  combustion_phase->add_action( "bag_of_tricks,if=buff.combustion.down" );
  combustion_phase->add_action( "call_action_list,name=combustion_cooldowns,if=buff.combustion.remains>variable.skb_duration|fight_remains<20", "Other cooldowns that should be used with Combustion should only be used with an actual Combustion cast and not with a Sun King's Blessing proc." );
  combustion_phase->add_action( "call_action_list,name=active_talents" );
  combustion_phase->add_action( "flamestrike,if=buff.combustion.down&buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.remains>cast_time&buff.fury_of_the_sun_king.expiration_delay_remains=0&cooldown.combustion.remains<cast_time&active_enemies>=variable.skb_flamestrike", "If Combustion is down, precast something before activating it." );
  combustion_phase->add_action( "pyroblast,if=buff.combustion.down&buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.remains>cast_time&buff.fury_of_the_sun_king.expiration_delay_remains=0" );
  combustion_phase->add_action( "fireball,if=buff.combustion.down&cooldown.combustion.remains<cast_time&active_enemies<2&!improved_scorch.active" );
  combustion_phase->add_action( "scorch,if=buff.combustion.down&cooldown.combustion.remains<cast_time" );
  combustion_phase->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=hot_streak_spells_in_flight=0&buff.combustion.down&variable.time_to_combustion<=0&(action.scorch.executing&action.scorch.execute_remains<variable.combustion_cast_remains|action.fireball.executing&action.fireball.execute_remains<variable.combustion_cast_remains|action.pyroblast.executing&action.pyroblast.execute_remains<variable.combustion_cast_remains|action.flamestrike.executing&action.flamestrike.execute_remains<variable.combustion_cast_remains|action.meteor.in_flight&action.meteor.in_flight_remains<variable.combustion_cast_remains)", "Combustion should be used when the precast is almost finished or when Meteor is about to land." );
  combustion_phase->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!variable.fire_blast_pooling&(!improved_scorch.active|action.scorch.executing|debuff.improved_scorch.remains>4*gcd.max)&(buff.fury_of_the_sun_king.down|action.pyroblast.executing)&buff.combustion.up&!buff.hyperthermia.react&!buff.hot_streak.react&hot_streak_spells_in_flight+buff.heating_up.react*(gcd.remains>0)<2", "Without Feel the Burn, just use Fire Blasts when they won't munch crits and when Hyperthermia is down." );
  combustion_phase->add_action( "flamestrike,if=(buff.hot_streak.react&active_enemies>=variable.combustion_flamestrike)|(buff.hyperthermia.react&active_enemies>=variable.combustion_flamestrike-talent.hyperthermia)", "Spend Hot Streaks during Combustion at high priority." );
  combustion_phase->add_action( "pyroblast,if=buff.hyperthermia.react" );
  combustion_phase->add_action( "pyroblast,if=buff.hot_streak.react&buff.combustion.up" );
  combustion_phase->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies<variable.combustion_flamestrike&buff.combustion.up" );
  combustion_phase->add_action( "shifting_power,if=buff.combustion.up&!action.fire_blast.charges&(action.phoenix_flames.charges<action.phoenix_flames.max_charges|talent.call_of_the_sun_king)&variable.combustion_shifting_power<=active_enemies", "Using Shifting Power during Combustion to restore Fire Blast and Phoenix Flame charges can be beneficial, but usually only on AoE." );
  combustion_phase->add_action( "flamestrike,if=buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.remains>cast_time&active_enemies>=variable.skb_flamestrike&buff.fury_of_the_sun_king.expiration_delay_remains=0", "Spend Fury of the Sun King procs." );
  combustion_phase->add_action( "pyroblast,if=buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.remains>cast_time&buff.fury_of_the_sun_king.expiration_delay_remains=0" );
  combustion_phase->add_action( "scorch,if=improved_scorch.active&(debuff.improved_scorch.remains<4*gcd.max)&active_enemies<variable.combustion_flamestrike", "Maintain Improved Scorch when not casting Flamestrikes in Combustion." );
  combustion_phase->add_action( "phoenix_flames,if=talent.phoenix_reborn&buff.heating_up.react+hot_streak_spells_in_flight<2&buff.flames_fury.up", "With the T30 set, Phoenix Flames should be used to maintain Charring Embers during Combustion and Flame's Fury procs should be spent." );
  combustion_phase->add_action( "scorch,if=buff.heat_shimmer.react" );
  combustion_phase->add_action( "phoenix_flames,if=(!talent.call_of_the_sun_king&travel_time<buff.combustion.remains|talent.call_of_the_sun_king)&buff.heating_up.react+hot_streak_spells_in_flight<2", "Use Phoenix Flames and Scorch in Combustion to help generate Hot Streaks when Fire Blasts are not available or need to be conserved." );
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
  firestarter_fire_blasts->add_action( "fire_blast,use_off_gcd=1,if=!variable.fire_blast_pooling&buff.heating_up.react+hot_streak_spells_in_flight=1&(talent.feel_the_burn&buff.feel_the_burn.remains<gcd.remains|cooldown.shifting_power.ready)", "If not casting anything, use Fire Blast to trigger Hot Streak! only if Feel the Burn is talented and would expire before the GCD ends or if Shifting Power is available." );

  standard_rotation->add_action( "flamestrike,if=active_enemies>=variable.hot_streak_flamestrike&(buff.hot_streak.react|buff.hyperthermia.react)" );
  standard_rotation->add_action( "pyroblast,if=(buff.hyperthermia.react|buff.hot_streak.react&(buff.hot_streak.remains<action.fireball.execute_time)|buff.hot_streak.react&(hot_streak_spells_in_flight|firestarter.active|talent.call_of_the_sun_king&action.phoenix_flames.charges)|buff.hot_streak.react&scorch_execute.active)" );
  standard_rotation->add_action( "flamestrike,if=active_enemies>=variable.skb_flamestrike&buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.expiration_delay_remains=0" );
  standard_rotation->add_action( "scorch,if=improved_scorch.active&debuff.improved_scorch.remains<action.pyroblast.cast_time+5*gcd.max&buff.fury_of_the_sun_king.up&!action.scorch.in_flight" );
  standard_rotation->add_action( "pyroblast,if=buff.fury_of_the_sun_king.up&buff.fury_of_the_sun_king.expiration_delay_remains=0" );
  standard_rotation->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=!firestarter.active&!variable.fire_blast_pooling&buff.fury_of_the_sun_king.down&(((action.fireball.executing&(action.fireball.execute_remains<0.5|!talent.hyperthermia)|action.pyroblast.executing&(action.pyroblast.execute_remains<0.5|!talent.hyperthermia))&buff.heating_up.react)|(scorch_execute.active&(!improved_scorch.active|debuff.improved_scorch.stack=debuff.improved_scorch.max_stack|full_recharge_time<3)&(buff.heating_up.react&!action.scorch.executing|!buff.hot_streak.react&!buff.heating_up.react&action.scorch.executing&!hot_streak_spells_in_flight)))", "During the standard rotation, only use Fire Blasts when they are not being pooled for RoP or Combustion. Use Fire Blast either during a Fireball/Pyroblast cast when Heating Up is active or during execute with Searing Touch." );
  standard_rotation->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&scorch_execute.active&active_enemies<variable.hot_streak_flamestrike" );
  standard_rotation->add_action( "scorch,if=improved_scorch.active&debuff.improved_scorch.remains<4*gcd.max" );
  standard_rotation->add_action( "phoenix_flames,if=talent.call_of_the_sun_king&(!talent.feel_the_burn|buff.feel_the_burn.remains<2*gcd.max)" );
  standard_rotation->add_action( "scorch,if=improved_scorch.active&debuff.improved_scorch.stack<debuff.improved_scorch.max_stack" );
  standard_rotation->add_action( "phoenix_flames,if=!talent.call_of_the_sun_king&!buff.hot_streak.react&!variable.phoenix_pooling&buff.flames_fury.up" );
  standard_rotation->add_action( "phoenix_flames,if=talent.call_of_the_sun_king&!buff.hot_streak.react&hot_streak_spells_in_flight=0&(!variable.phoenix_pooling&buff.flames_fury.up|charges_fractional>2.5|charges_fractional>1.5)" );
  standard_rotation->add_action( "call_action_list,name=active_talents" );
  standard_rotation->add_action( "dragons_breath,if=active_enemies>1&talent.alexstraszas_fury", "Dragon's Breath is no longer a gain to be cast unless Alexstrazas is talented (need to check cutoff, was true on 10t)" );
  standard_rotation->add_action( "scorch,if=(scorch_execute.active|buff.heat_shimmer.up)" );
  standard_rotation->add_action( "arcane_explosion,if=active_enemies>=variable.arcane_explosion&mana.pct>=variable.arcane_explosion_mana" );
  standard_rotation->add_action( "flamestrike,if=active_enemies>=variable.hard_cast_flamestrike", "With enough targets, it is a gain to cast Flamestrike as filler instead of Fireball." );
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
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* movement = p->get_action_priority_list( "movement" );
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
  default_->add_action( "run_action_list,name=cleave,if=active_enemies>=2&active_enemies<=3" );
  default_->add_action( "run_action_list,name=st" );

  aoe->add_action( "cone_of_cold,if=talent.coldest_snap&(prev_gcd.1.comet_storm|prev_gcd.1.frozen_orb&!talent.comet_storm)" );
  aoe->add_action( "frozen_orb,if=!prev_gcd.1.glacial_spike|!freezable" );
  aoe->add_action( "blizzard,if=!prev_gcd.1.glacial_spike|!freezable" );
  aoe->add_action( "frostbolt,if=buff.icy_veins.up&(buff.deaths_chill.stack<9|buff.deaths_chill.stack=9&!action.frostbolt.in_flight)&buff.icy_veins.remains>8&talent.deaths_chill" );
  aoe->add_action( "comet_storm,if=!prev_gcd.1.glacial_spike&(!talent.coldest_snap|cooldown.cone_of_cold.ready&cooldown.frozen_orb.remains>25|cooldown.cone_of_cold.remains>20)" );
  aoe->add_action( "freeze,if=freezable&debuff.frozen.down&(!talent.glacial_spike|prev_gcd.1.glacial_spike)" );
  aoe->add_action( "ice_nova,if=freezable&!prev_off_gcd.freeze&(prev_gcd.1.glacial_spike)" );
  aoe->add_action( "frost_nova,if=freezable&!prev_off_gcd.freeze&(prev_gcd.1.glacial_spike&!remaining_winters_chill)" );
  aoe->add_action( "shifting_power,if=cooldown.comet_storm.remains>10" );
  aoe->add_action( "flurry,if=cooldown_react&!debuff.winters_chill.remains&buff.icicles.react=4&talent.glacial_spike&!freezable" );
  aoe->add_action( "glacial_spike,if=buff.icicles.react=5&cooldown.blizzard.remains>gcd.max" );
  aoe->add_action( "flurry,if=(freezable|!talent.glacial_spike)&cooldown_react&!debuff.winters_chill.remains&(buff.brain_freeze.react|!buff.fingers_of_frost.react)" );
  aoe->add_action( "ice_lance,if=buff.fingers_of_frost.react|debuff.frozen.remains>travel_time|remaining_winters_chill" );
  aoe->add_action( "ice_nova,if=active_enemies>=4&(!talent.glacial_spike|!freezable)" );
  aoe->add_action( "dragons_breath,if=active_enemies>=7" );
  aoe->add_action( "arcane_explosion,if=mana.pct>30&active_enemies>=7" );
  aoe->add_action( "frostbolt" );
  aoe->add_action( "call_action_list,name=movement" );

  cds->add_action( "use_item,name=spoils_of_neltharus,if=buff.spoils_of_neltharus_mastery.up|buff.spoils_of_neltharus_haste.up&buff.bloodlust.down|buff.spoils_of_neltharus_vers.up&(buff.bloodlust.up)" );
  cds->add_action( "potion,if=prev_off_gcd.icy_veins|fight_remains<60" );
  cds->add_action( "use_item,name=dreambinder_loom_of_the_great_cycle,if=(equipped.nymues_unraveling_spindle&prev_gcd.1.nymues_unraveling_spindle)|fight_remains>2" );
  cds->add_action( "use_item,name=belorrelos_the_suncaller,if=time>5&!prev_gcd.1.flurry" );
  cds->add_action( "flurry,if=time=0&active_enemies<=2" );
  cds->add_action( "icy_veins" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down" );
  cds->add_action( "invoke_external_buff,name=blessing_of_summer,if=buff.blessing_of_summer.down" );
  cds->add_action( "blood_fury" );
  cds->add_action( "berserking" );
  cds->add_action( "lights_judgment" );
  cds->add_action( "fireblood" );
  cds->add_action( "ancestral_call" );

  cleave->add_action( "comet_storm,if=prev_gcd.1.flurry|prev_gcd.1.cone_of_cold" );
  cleave->add_action( "flurry,target_if=min:debuff.winters_chill.stack,if=cooldown_react&((prev_gcd.1.frostbolt&buff.icicles.react>=3)|prev_gcd.1.glacial_spike|(buff.icicles.react>=3&buff.icicles.react<5&charges_fractional=2))" );
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
  movement->add_action( "arcane_explosion,if=mana.pct>30&active_enemies>=2" );
  movement->add_action( "fire_blast" );
  movement->add_action( "ice_lance" );

  st->add_action( "comet_storm,if=prev_gcd.1.flurry|prev_gcd.1.cone_of_cold" );
  st->add_action( "flurry,if=cooldown_react&remaining_winters_chill=0&debuff.winters_chill.down&((prev_gcd.1.frostbolt&buff.icicles.react>=3|prev_gcd.1.frostbolt&buff.brain_freeze.react)|prev_gcd.1.glacial_spike|talent.glacial_spike&buff.icicles.react=4&!buff.fingers_of_frost.react)" );
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
