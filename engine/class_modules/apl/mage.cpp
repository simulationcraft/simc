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
  return p->true_level >= 80 ? "feast_of_the_midnight_masquerade"
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
  action_priority_list_t* spellslinger = p->get_action_priority_list( "spellslinger" );
  action_priority_list_t* spellslinger_aoe = p->get_action_priority_list( "spellslinger_aoe" );
  action_priority_list_t* sunfury = p->get_action_priority_list( "sunfury" );

  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "variable,name=aoe_target_count,op=reset,default=2" );
  precombat->add_action( "variable,name=aoe_target_count,op=set,value=9,if=!talent.arcing_cleave" );
  precombat->add_action( "variable,name=opener,op=set,value=1" );
  precombat->add_action( "variable,name=aoe_list,default=0,op=reset" );
  precombat->add_action( "variable,name=steroid_trinket_equipped,op=set,value=equipped.gladiators_badge|equipped.signet_of_the_priory|equipped.high_speakers_accretion|equipped.spymasters_web|equipped.treacherous_transmitter|equipped.imperfect_ascendancy_serum|equipped.quickwick_candlestick|equipped.soulletting_ruby|equipped.funhouse_lens|equipped.house_of_cards|equipped.flarendos_pilot_light|equipped.signet_of_the_priory|equipped.neural_synapse_enhancer" );
  precombat->add_action( "variable,name=neural_on_mini,op=set,value=equipped.gladiators_badge|equipped.signet_of_the_priory|equipped.high_speakers_accretion|equipped.spymasters_web|equipped.treacherous_transmitter|equipped.imperfect_ascendancy_serum|equipped.quickwick_candlestick|equipped.soulletting_ruby|equipped.funhouse_lens|equipped.house_of_cards|equipped.flarendos_pilot_light|equipped.signet_of_the_priory" );
  precombat->add_action( "variable,name=nonsteroid_trinket_equipped,op=set,value=equipped.blastmaster3000|equipped.ratfang_toxin|equipped.ingenious_mana_battery|equipped.geargrinders_spare_keys|equipped.ringing_ritual_mud|equipped.goo_blin_grenade|equipped.noggenfogger_ultimate_deluxe|equipped.garbagemancers_last_resort|equipped.mad_queens_mandate|equipped.fearbreakers_echo|equipped.mereldars_toll|equipped.gooblin_grenade" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "use_item,name=ingenious_mana_battery" );
  precombat->add_action( "variable,name=treacherous_transmitter_precombat_cast,value=11" );
  precombat->add_action( "use_item,name=treacherous_transmitter" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "use_item,name=imperfect_ascendancy_serum" );
  precombat->add_action( "arcane_blast,if=!talent.evocation" );
  precombat->add_action( "evocation,if=talent.evocation" );

  default_->add_action( "counterspell" );
  default_->add_action( "potion,if=!equipped.spymasters_web&(buff.siphon_storm.up|(!talent.evocation&cooldown.arcane_surge.ready))|equipped.spymasters_web&(buff.spymasters_web.up|(fight_remains>330&buff.siphon_storm.up))" );
  default_->add_action( "lights_judgment,if=(buff.arcane_surge.down&debuff.touch_of_the_magi.down&active_enemies>=2)" );
  default_->add_action( "berserking,if=(prev_gcd.1.arcane_surge&variable.opener)|((prev_gcd.1.arcane_surge&(fight_remains<80|target.health.pct<35|!talent.arcane_bombardment|buff.spymasters_web.up))|(prev_gcd.1.arcane_surge&!equipped.spymasters_web))" );
  default_->add_action( "blood_fury,if=(prev_gcd.1.arcane_surge&variable.opener)|((prev_gcd.1.arcane_surge&(fight_remains<80|target.health.pct<35|!talent.arcane_bombardment|buff.spymasters_web.up))|(prev_gcd.1.arcane_surge&!equipped.spymasters_web))" );
  default_->add_action( "fireblood,if=(prev_gcd.1.arcane_surge&variable.opener)|((prev_gcd.1.arcane_surge&(fight_remains<80|target.health.pct<35|!talent.arcane_bombardment|buff.spymasters_web.up))|(prev_gcd.1.arcane_surge&!equipped.spymasters_web))" );
  default_->add_action( "ancestral_call,if=(prev_gcd.1.arcane_surge&variable.opener)|((prev_gcd.1.arcane_surge&(fight_remains<80|target.health.pct<35|!talent.arcane_bombardment|buff.spymasters_web.up))|(prev_gcd.1.arcane_surge&!equipped.spymasters_web))" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=(!equipped.spymasters_web&prev_gcd.1.arcane_surge)|(equipped.spymasters_web&prev_gcd.1.evocation)", "Invoke Externals with cooldowns except Autumn which should come just after cooldowns" );
  default_->add_action( "invoke_external_buff,name=blessing_of_summer,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "invoke_external_buff,name=blessing_of_autumn,if=cooldown.touch_of_the_magi.remains>5" );
  default_->add_action( "variable,name=spymasters_double_on_use,op=set,value=(equipped.gladiators_badge|equipped.signet_of_the_priory|equipped.high_speakers_accretion|equipped.treacherous_transmitter|equipped.imperfect_ascendancy_serum|equipped.quickwick_candlestick|equipped.soulletting_ruby|equipped.funhouse_lens|equipped.house_of_cards|equipped.flarendos_pilot_light|equipped.signet_of_the_priory)&equipped.spymasters_web&cooldown.evocation.remains<17&(buff.spymasters_report.stack>35|(fight_remains<90&buff.spymasters_report.stack>25))" );
  default_->add_action( "use_items,if=((prev_gcd.1.arcane_surge&variable.steroid_trinket_equipped)|(cooldown.arcane_surge.ready&variable.steroid_trinket_equipped)|!variable.steroid_trinket_equipped&variable.nonsteroid_trinket_equipped|(variable.nonsteroid_trinket_equipped&buff.siphon_storm.remains<10&(cooldown.evocation.remains>17|trinket.cooldown.remains>20)))&!variable.spymasters_double_on_use|(fight_remains<20)", "Trinket specific use cases vary, default is just with cooldowns" );
  default_->add_action( "use_item,name=treacherous_transmitter,if=buff.spymasters_report.stack<40" );
  default_->add_action( "do_treacherous_transmitter_task,use_off_gcd=1,if=buff.siphon_storm.up|fight_remains<20|(buff.cryptic_instructions.remains<?buff.realigning_nexus_convergence_divergence.remains<?buff.errant_manaforge_emission.remains)<3" );
  default_->add_action( "use_item,name=spymasters_web,if=((prev_gcd.1.arcane_surge|prev_gcd.1.evocation)&(fight_remains<80|target.health.pct<35|!talent.arcane_bombardment|(buff.spymasters_report.stack=40&fight_remains>240))|fight_remains<20)" );
  default_->add_action( "use_item,name=high_speakers_accretion,if=(prev_gcd.1.arcane_surge|prev_gcd.1.evocation|(buff.siphon_storm.up&variable.opener)|cooldown.evocation.remains<4|fight_remains<20)&!variable.spymasters_double_on_use" );
  default_->add_action( "use_item,name=imperfect_ascendancy_serum,if=(cooldown.evocation.ready|cooldown.arcane_surge.ready|fight_remains<21)&!variable.spymasters_double_on_use" );
  default_->add_action( "use_item,name=neural_synapse_enhancer,if=(debuff.touch_of_the_magi.remains>8&buff.arcane_surge.up)|(debuff.touch_of_the_magi.remains>8&variable.neural_on_mini)" );
  default_->add_action( "variable,name=opener,op=set,if=debuff.touch_of_the_magi.up&variable.opener,value=0" );
  default_->add_action( "arcane_barrage,if=fight_remains<2" );
  default_->add_action( "call_action_list,name=cd_opener", "Enter cooldowns, then action list depending on your hero talent choices" );
  default_->add_action( "call_action_list,name=spellslinger_aoe,if=!talent.spellfire_spheres&variable.aoe_list" );
  default_->add_action( "call_action_list,name=sunfury,if=talent.spellfire_spheres" );
  default_->add_action( "call_action_list,name=spellslinger,if=!talent.spellfire_spheres" );
  default_->add_action( "arcane_barrage" );

  cd_opener->add_action( "touch_of_the_magi,use_off_gcd=1,if=prev_gcd.1.arcane_barrage&(action.arcane_barrage.in_flight_remains<=0.5|gcd.remains<=0.5)&(buff.arcane_surge.up|cooldown.arcane_surge.remains>30)|(prev_gcd.1.arcane_surge&(buff.arcane_charge.stack<4|buff.nether_precision.down))|(cooldown.arcane_surge.remains>30&cooldown.touch_of_the_magi.ready&buff.arcane_charge.stack<4&!prev_gcd.1.arcane_barrage)", "Touch of the Magi used when Arcane Barrage is mid-flight or if you just used Arcane Surge and you don't have 4 Arcane Charges, the wait simulates the time it takes to queue another spell after Touch when you Surge into Touch, throws up Touch as soon as possible even without Barraging first if it's ready for miniburn." );
  cd_opener->add_action( "wait,sec=0.05,if=prev_gcd.1.arcane_surge&time-action.touch_of_the_magi.last_used<0.015,line_cd=15" );
  cd_opener->add_action( "arcane_blast,if=buff.presence_of_mind.up" );
  cd_opener->add_action( "arcane_orb,if=talent.high_voltage&variable.opener,line_cd=10", "Use Orb for Charges on the opener if you have High Voltage as the Missiles will generate the remaining Charge you need" );
  cd_opener->add_action( "arcane_barrage,if=buff.arcane_tempo.up&cooldown.evocation.ready&buff.arcane_tempo.remains<gcd.max*5,line_cd=11", "Barrage before Evocation if Tempo will expire" );
  cd_opener->add_action( "evocation,if=cooldown.arcane_surge.remains<(gcd.max*3)&cooldown.touch_of_the_magi.remains<(gcd.max*5)" );
  cd_opener->add_action( "arcane_missiles,if=((prev_gcd.1.evocation|prev_gcd.1.arcane_surge)|variable.opener)&buff.nether_precision.down&(buff.aether_attunement.react=0|set_bonus.thewarwithin_season_2_4pc),interrupt_if=tick_time>gcd.remains&(buff.aether_attunement.react=0|(active_enemies>3&(!talent.time_loop|talent.resonance))),interrupt_immediate=1,interrupt_global=1,chain=1,line_cd=30", "Use Missiles to get Nether Precision up for your opener and to spend Aether Attunement if you have 4pc S2 set before Surging, clipping logic now applies to Aether Attunement in AOE when you have Time Loop talented and not Resonance." );
  cd_opener->add_action( "arcane_surge,if=cooldown.touch_of_the_magi.remains<(action.arcane_surge.execute_time+(gcd.max*(buff.arcane_charge.stack=4)))" );

  spellslinger->add_action( "shifting_power,if=(((((action.arcane_orb.charges=talent.charged_orb)&cooldown.arcane_orb.remains)|cooldown.touch_of_the_magi.remains<23)&buff.arcane_surge.down&buff.siphon_storm.down&debuff.touch_of_the_magi.down&(buff.intuition.react=0|(buff.intuition.react&buff.intuition.remains>cast_time))&cooldown.touch_of_the_magi.remains>(12+6*gcd.max))|(prev_gcd.1.arcane_barrage&talent.shifting_shards&(buff.intuition.react=0|(buff.intuition.react&buff.intuition.remains>cast_time))&(buff.arcane_surge.up|debuff.touch_of_the_magi.up|cooldown.evocation.remains<20)))&fight_remains>10&(buff.arcane_tempo.remains>gcd.max*2.5|buff.arcane_tempo.down)", "With Shifting Shards we can use Shifting Power whenever basically favoring cooldowns slightly, without it though we want to use it outside of cooldowns, don't cast if it'll conflict with Intuition expiration." );
  spellslinger->add_action( "cancel_buff,name=presence_of_mind,use_off_gcd=1,if=prev_gcd.1.arcane_blast&buff.presence_of_mind.stack=1", "In single target, use Presence of Mind at the very end of Touch of the Magi, then cancelaura the buff to start the cooldown, wait is to simulate the delay of hitting Presence of Mind after another spell cast." );
  spellslinger->add_action( "presence_of_mind,if=debuff.touch_of_the_magi.remains<=gcd.max&buff.nether_precision.up&active_enemies<variable.aoe_target_count&!talent.unerring_proficiency" );
  spellslinger->add_action( "wait,sec=0.05,if=time-action.presence_of_mind.last_used<0.015,line_cd=15" );
  spellslinger->add_action( "supernova,if=debuff.touch_of_the_magi.remains<=gcd.max&buff.unerring_proficiency.stack=30" );
  spellslinger->add_action( "arcane_barrage,if=(buff.arcane_tempo.up&buff.arcane_tempo.remains<gcd.max)|(buff.intuition.react&buff.intuition.remains<gcd.max)", "Barrage if Tempo or Intuition are about to expire." );
  spellslinger->add_action( "arcane_barrage,if=buff.arcane_harmony.stack>=(18-(6*talent.high_voltage))&(buff.nether_precision.down|buff.nether_precision.stack=1)", "Barrage if Harmony is over 18 stacks, or 12 with High Voltage and either no Nether Precision or your last stack of it." );
  spellslinger->add_action( "arcane_missiles,if=buff.aether_attunement.react&cooldown.touch_of_the_magi.remains<gcd.max*3&buff.clearcasting.react&set_bonus.thewarwithin_season_2_4pc", "Use Aether Attunement up before casting Touch if you have S2 4pc equipped to avoid munching." );
  spellslinger->add_action( "arcane_barrage,if=(cooldown.touch_of_the_magi.ready|cooldown.touch_of_the_magi.remains<((travel_time+50)>?gcd.max))", "Barrage if Touch is up or will be up while Barrage is in the air." );
  spellslinger->add_action( "arcane_missiles,if=(buff.clearcasting.react&buff.nether_precision.down&((cooldown.touch_of_the_magi.remains>gcd.max*7&cooldown.arcane_surge.remains>gcd.max*7)|buff.clearcasting.react>1|!talent.magis_spark|(cooldown.touch_of_the_magi.remains<gcd.max*4&buff.aether_attunement.react=0)|set_bonus.thewarwithin_season_2_4pc))|(fight_remains<5&buff.clearcasting.react),interrupt_if=tick_time>gcd.remains&(buff.aether_attunement.react=0|(active_enemies>3&(!talent.time_loop|talent.resonance))),interrupt_immediate=1,interrupt_global=1,chain=1", "Use Clearcasting procs to keep Nether Precision up, if you don't have S2 4pc try to pool Aether Attunement for cooldown windows." );
  spellslinger->add_action( "arcane_blast,if=((debuff.magis_spark_arcane_blast.up&((debuff.magis_spark_arcane_blast.remains<(cast_time+gcd.max))|active_enemies=1|talent.leydrinker))|buff.leydrinker.up)&buff.arcane_charge.stack=4&!talent.charged_orb&active_enemies<3,line_cd=2", "Blast whenever you have the bonus from Leydrinker or Magi's Spark up, don't let spark expire in AOE." );
  spellslinger->add_action( "arcane_barrage,if=talent.high_voltage&active_enemies>1&buff.arcane_charge.stack=4&buff.clearcasting.react&(buff.aether_attunement.react|!set_bonus.thewarwithin_season_2_4pc)&buff.nether_precision.up", "Barrage in AOE if you can refund Charges through High Voltage as soon as possible if you have Aether Attunement and Nether Precision up." );
  spellslinger->add_action( "arcane_barrage,if=talent.orb_barrage&active_enemies>1&(debuff.magis_spark_arcane_blast.down|!talent.magis_spark)&buff.arcane_charge.stack=4&((talent.high_voltage&active_enemies>2)|((cooldown.touch_of_the_magi.remains>gcd.max*6|!talent.magis_spark)|(talent.charged_orb&cooldown.arcane_orb.charges_fractional>1.8)))", "Barrage in AOE with Orb Barrage under some minor restrictions if you can recoup Charges, pooling for Spark as Touch comes off cooldown." );
  spellslinger->add_action( "arcane_barrage,if=active_enemies>1&(debuff.magis_spark_arcane_blast.down|!talent.magis_spark)&buff.arcane_charge.stack=4&(cooldown.arcane_orb.remains<gcd.max|(target.health.pct<35&talent.arcane_bombardment))&(buff.nether_precision.stack=1|(buff.nether_precision.down&talent.high_voltage)|(buff.nether_precision.stack=2&target.health.pct<35&talent.arcane_bombardment&talent.high_voltage))&(cooldown.touch_of_the_magi.remains>gcd.max*6|(talent.charged_orb&cooldown.arcane_orb.charges_fractional>1.8))", "Barrage in AOE if Orb is up or enemy is in execute range." );
  spellslinger->add_action( "arcane_missiles,if=talent.high_voltage&(buff.clearcasting.react>1|(buff.clearcasting.react&buff.aether_attunement.react))&buff.arcane_charge.stack<3,interrupt_if=tick_time>gcd.remains&(buff.aether_attunement.react=0|(active_enemies>3&(!talent.time_loop|talent.resonance))),interrupt_immediate=1,interrupt_global=1,chain=1", "Missile to refill charges if you have High Voltage and either Aether Attunement or more than one Clearcasting proc." );
  spellslinger->add_action( "arcane_orb,if=(active_enemies=1&buff.arcane_charge.stack<3)|(buff.arcane_charge.stack<1|(buff.arcane_charge.stack<2&talent.high_voltage))", "Orb below 3 charges in single target, at 0 charges, or 1 or 0 charge with High Voltage." );
  spellslinger->add_action( "arcane_barrage,if=buff.intuition.react" );
  spellslinger->add_action( "arcane_barrage,if=active_enemies=1&talent.high_voltage&buff.arcane_charge.stack=4&buff.clearcasting.react&buff.nether_precision.stack=1&(buff.aether_attunement.react|(target.health.pct<35&talent.arcane_bombardment))", "Barrage in single target if you have High Voltage, last Nether Precision stack, Clearcasting and either Aether or Execute." );
  spellslinger->add_action( "arcane_barrage,if=cooldown.arcane_orb.remains<gcd.max&buff.arcane_charge.stack=4&buff.nether_precision.down&talent.orb_barrage&(cooldown.touch_of_the_magi.remains>gcd.max*6|!talent.magis_spark)", "Barrage if you have orb ready and either Orb Barrage or High Voltage, pool for Spark." );
  spellslinger->add_action( "arcane_barrage,if=active_enemies=1&(talent.orb_barrage|(target.health.pct<35&talent.arcane_bombardment))&(cooldown.arcane_orb.remains<gcd.max)&buff.arcane_charge.stack=4&(cooldown.touch_of_the_magi.remains>gcd.max*6|!talent.magis_spark)&(buff.nether_precision.down|(buff.nether_precision.stack=1&buff.clearcasting.stack=0))", "Barrage with Orb Barrage or execute if you have orb up and no Nether Precision or no way to get another." );
  spellslinger->add_action( "arcane_explosion,if=active_enemies>1&((buff.arcane_charge.stack<1&!talent.high_voltage)|(buff.arcane_charge.stack<3&(buff.clearcasting.react=0|talent.reverberate)))", "Use Explosion for your first charge or if you have High Voltage you can use it for charge 2 and 3, but at a slightly higher target count." );
  spellslinger->add_action( "arcane_blast", "Nothing else to do? Blast. Out of mana? Barrage." );
  spellslinger->add_action( "arcane_barrage" );

  spellslinger_aoe->add_action( "supernova,if=buff.unerring_proficiency.stack=30", "This section is only called with a variable to aggressively AOE instead of focus funnel into one target, the overall dps is slightly higher but the priority dps is much longer" );
  spellslinger_aoe->add_action( "shifting_power,if=((buff.arcane_surge.down&buff.siphon_storm.down&debuff.touch_of_the_magi.down&cooldown.evocation.remains>15&cooldown.touch_of_the_magi.remains>10)&(cooldown.arcane_orb.remains&action.arcane_orb.charges=0)&fight_remains>10)|(prev_gcd.1.arcane_barrage&(buff.arcane_surge.up|debuff.touch_of_the_magi.up|cooldown.evocation.remains<20)&talent.shifting_shards)" );
  spellslinger_aoe->add_action( "arcane_orb,if=buff.arcane_charge.stack<3" );
  spellslinger_aoe->add_action( "arcane_blast,if=((debuff.magis_spark_arcane_blast.up|buff.leydrinker.up)&!prev_gcd.1.arcane_blast)" );
  spellslinger_aoe->add_action( "arcane_barrage,if=buff.aether_attunement.react&talent.high_voltage&buff.clearcasting.react&buff.arcane_charge.stack>1", "Clearcasting is exclusively spent on Arcane Missiles in AOE and always interrupted after the global cooldown ends except for Aether Attunement" );
  spellslinger_aoe->add_action( "arcane_missiles,if=buff.clearcasting.react&((talent.high_voltage&buff.arcane_charge.stack<4)|buff.nether_precision.down),interrupt_if=tick_time>gcd.remains,interrupt_immediate=1,interrupt_global=1,chain=1" );
  spellslinger_aoe->add_action( "presence_of_mind,if=buff.arcane_charge.stack=3|buff.arcane_charge.stack=2", "Only use Presence of Mind at low charges, use these to get to 4 Charges quicker" );
  spellslinger_aoe->add_action( "arcane_barrage,if=buff.arcane_charge.stack=4" );
  spellslinger_aoe->add_action( "arcane_explosion,if=(talent.reverberate|buff.arcane_charge.stack<2)" );
  spellslinger_aoe->add_action( "arcane_blast" );
  spellslinger_aoe->add_action( "arcane_barrage" );

  sunfury->add_action( "shifting_power,if=((buff.arcane_surge.down&buff.siphon_storm.down&debuff.touch_of_the_magi.down&cooldown.evocation.remains>15&cooldown.touch_of_the_magi.remains>10)&fight_remains>10)&buff.arcane_soul.down&(buff.intuition.react=0|(buff.intuition.react&buff.intuition.remains>cast_time))", "For Sunfury, Shifting Power only when you're not under the effect of any cooldowns." );
  sunfury->add_action( "cancel_buff,name=presence_of_mind,use_off_gcd=1,if=(prev_gcd.1.arcane_blast&buff.presence_of_mind.stack=1)|active_enemies<4" );
  sunfury->add_action( "presence_of_mind,if=debuff.touch_of_the_magi.remains<=gcd.max&buff.nether_precision.up&active_enemies<4" );
  sunfury->add_action( "wait,sec=0.05,if=time-action.presence_of_mind.last_used<0.015,line_cd=15" );
  sunfury->add_action( "arcane_missiles,if=buff.nether_precision.down&buff.clearcasting.react&buff.arcane_soul.up&buff.arcane_soul.remains>gcd.max*(4-buff.clearcasting.react),interrupt_if=tick_time>gcd.remains,interrupt_immediate=1,interrupt_global=1,chain=1", "When Arcane Soul is up, use Missiles to generate Nether Precision as needed while also ensuring you end Soul with 3 Clearcasting." );
  sunfury->add_action( "arcane_barrage,if=buff.arcane_soul.up" );
  sunfury->add_action( "arcane_barrage,if=(buff.arcane_tempo.up&buff.arcane_tempo.remains<gcd.max)|(buff.intuition.react&buff.intuition.remains<gcd.max)", "Prioritize Tempo and Intuition if they are about to expire, spend Aether Attunement if you have 4pc S2 set before Touch." );
  sunfury->add_action( "arcane_missiles,if=buff.aether_attunement.react&cooldown.touch_of_the_magi.remains<gcd.max*3&buff.clearcasting.react&set_bonus.thewarwithin_season_2_4pc" );
  sunfury->add_action( "arcane_blast,if=((debuff.magis_spark_arcane_blast.up&((debuff.magis_spark_arcane_blast.remains<(cast_time+gcd.max))|active_enemies=1|talent.leydrinker))|buff.leydrinker.up)&buff.arcane_charge.stack=4&(buff.nether_precision.up|buff.clearcasting.react=0),line_cd=2", "Blast whenever you have the bonus from Leydrinker or Magi's Spark up, don't let spark expire in AOE." );
  sunfury->add_action( "arcane_barrage,if=(talent.orb_barrage&!talent.high_voltage&active_enemies>2&buff.arcane_harmony.stack>=18&(buff.nether_precision.down|buff.nether_precision.stack=1|(buff.nether_precision.stack=2&buff.clearcasting.react=3)))|(talent.high_voltage&active_enemies>1&buff.arcane_charge.stack=4&buff.clearcasting.react&buff.nether_precision.stack=1)|(active_enemies>1&talent.high_voltage&buff.arcane_charge.stack=4&buff.clearcasting.react&buff.aether_attunement.react&buff.glorious_incandescence.down&buff.intuition.down)|(active_enemies>2&talent.orb_barrage&talent.high_voltage&cooldown.touch_of_the_magi.remains>gcd.max*6&(debuff.magis_spark_arcane_blast.down|!talent.magis_spark)&buff.arcane_charge.stack=4&target.health.pct<35&talent.arcane_bombardment&(buff.nether_precision.up|(buff.nether_precision.down&buff.clearcasting.stack=0)))|((active_enemies>2|(active_enemies>1&target.health.pct<35&talent.arcane_bombardment))&cooldown.arcane_orb.remains<gcd.max&buff.arcane_charge.stack=4&cooldown.touch_of_the_magi.remains>gcd.max*6&(debuff.magis_spark_arcane_blast.down|!talent.magis_spark)&buff.nether_precision.up&(talent.high_voltage|buff.nether_precision.stack=2|(buff.nether_precision.stack=1&buff.clearcasting.react=0)))", "AOE Barrage is optimized for funnel, avoids overcapping Harmony stacks, spending Charges when you have a way to recoup them via High Voltage or Orb while pooling sometimes for Touch with various talent optimizations." );
  sunfury->add_action( "arcane_barrage,if=buff.arcane_charge.stack=4&(cooldown.touch_of_the_magi.ready|cooldown.touch_of_the_magi.remains<((travel_time+50)>?gcd.max))", "Barrage into Touch if you have charges when it comes up." );
  sunfury->add_action( "arcane_missiles,if=buff.clearcasting.react&((talent.high_voltage&buff.arcane_charge.stack<4)|buff.nether_precision.down|(buff.clearcasting.react=3&(!talent.high_voltage|active_enemies=1))),interrupt_if=tick_time>gcd.remains&(buff.aether_attunement.react=0|(active_enemies>3&(!talent.time_loop|talent.resonance))),interrupt_immediate=1,interrupt_global=1,chain=1", "Missiles to recoup Charges, maintain Nether Precisioin, or keep from overcapping Clearcasting with High Voltage or in single target." );
  sunfury->add_action( "arcane_barrage,if=(buff.arcane_charge.stack=4&active_enemies>1&active_enemies<5&buff.burden_of_power.up&((talent.high_voltage&buff.clearcasting.react)|buff.glorious_incandescence.up|buff.intuition.react|(cooldown.arcane_orb.remains<gcd.max|action.arcane_orb.charges>0)))&(!talent.consortiums_bauble|talent.high_voltage)", "Barrage with Burden if 2-4 targets and you have a way to recoup Charges, however skip this is you have Bauble and don't have High Voltage." );
  sunfury->add_action( "arcane_orb,if=buff.arcane_charge.stack<3", "Arcane Orb to recover Charges quickly if below 3." );
  sunfury->add_action( "arcane_barrage,if=(buff.glorious_incandescence.up&(cooldown.touch_of_the_magi.remains>6|!talent.magis_spark))|buff.intuition.react", "Barrage with Intuition or Incandescence unless Touch is almost up or you don't have Magi's Spark talented." );
  sunfury->add_action( "presence_of_mind,if=(buff.arcane_charge.stack=3|buff.arcane_charge.stack=2)&active_enemies>=3", "In AOE, Presence of Mind is used to build Charges. Arcane Explosion can be used to build your first Charge." );
  sunfury->add_action( "arcane_explosion,if=buff.arcane_charge.stack<2&active_enemies>1" );
  sunfury->add_action( "arcane_blast" );
  sunfury->add_action( "arcane_barrage" );
}
//arcane_apl_end

//fire_apl_start
void fire( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* ff_combustion = p->get_action_priority_list( "ff_combustion" );
  action_priority_list_t* ff_filler = p->get_action_priority_list( "ff_filler" );
  action_priority_list_t* sf_combustion = p->get_action_priority_list( "sf_combustion" );
  action_priority_list_t* sf_filler = p->get_action_priority_list( "sf_filler" );

  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "variable,name=cast_remains_time,value=0.3" );
  precombat->add_action( "variable,name=pooling_time,value=10+10*talent.frostfire_bolt" );
  precombat->add_action( "variable,name=ff_combustion_flamestrike,if=talent.frostfire_bolt,value=4+talent.firefall*99-(talent.mark_of_the_firelord&talent.quickflame&talent.firefall)*96+(!talent.mark_of_the_firelord&!talent.quickflame&!talent.flame_patch)*99+(talent.mark_of_the_firelord&!(talent.quickflame|talent.flame_patch))*2+(!talent.mark_of_the_firelord&(talent.quickflame|talent.flame_patch))*3" );
  precombat->add_action( "variable,name=ff_filler_flamestrike,if=talent.frostfire_bolt,value=4+talent.firefall*99-(talent.mark_of_the_firelord&talent.flame_patch&talent.firefall)*96+(!talent.mark_of_the_firelord&!talent.quickflame&!talent.flame_patch)*99+(talent.mark_of_the_firelord&!(talent.quickflame|talent.flame_patch))*2+(!talent.mark_of_the_firelord&(talent.quickflame|talent.flame_patch))*3" );
  precombat->add_action( "variable,name=sf_combustion_flamestrike,if=talent.spellfire_spheres,value=5+(!talent.mark_of_the_firelord)*99+(!(talent.flame_patch|talent.quickflame)&talent.firefall)*99+talent.firefall+(!(talent.flame_patch|talent.quickflame))*3" );
  precombat->add_action( "variable,name=sf_filler_flamestrike,if=talent.spellfire_spheres,value=4+talent.firefall+!talent.mark_of_the_firelord+!(talent.flame_patch|talent.quickflame)+(!talent.mark_of_the_firelord&!(talent.flame_patch|talent.quickflame))*2+(!talent.mark_of_the_firelord&!(talent.flame_patch|talent.quickflame)&talent.firefall)*99" );
  precombat->add_action( "variable,name=treacherous_transmitter_precombat_cast,value=12,if=equipped.treacherous_transmitter" );
  precombat->add_action( "use_item,name=treacherous_transmitter" );
  precombat->add_action( "use_item,name=ingenious_mana_battery" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "frostfire_bolt,if=talent.frostfire_bolt" );
  precombat->add_action( "pyroblast" );

  default_->add_action( "call_action_list,name=cds,if=!(buff.hot_streak.up&prev_gcd.1.scorch)" );
  default_->add_action( "run_action_list,name=ff_combustion,if=talent.frostfire_bolt&(cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.react|buff.combustion.remains>5)" );
  default_->add_action( "run_action_list,name=sf_combustion,if=cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.react|buff.combustion.remains>5" );
  default_->add_action( "run_action_list,name=ff_filler,if=talent.frostfire_bolt" );
  default_->add_action( "run_action_list,name=sf_filler" );

  cds->add_action( "phoenix_flames,if=time=0&!talent.firestarter", "Opener" );
  cds->add_action( "variable,name=combustion_precast_time,value=(talent.frostfire_bolt*(cooldown.meteor.ready+action.fireball.cast_time*!improved_scorch.active+action.scorch.cast_time*improved_scorch.active)+talent.spellfire_spheres*action.scorch.cast_time)-variable.cast_remains_time", "Determines when the combustion rotation should be used right before the combustion cooldown is coming up again" );
  cds->add_action( "potion,if=!talent.sun_kings_blessing&cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.remains>7|fight_remains<35", "Items, Racials and Externals" );
  cds->add_action( "use_item,name=neural_synapse_enhancer,if=!talent.sun_kings_blessing&cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.remains>7|fight_remains<20" );
  cds->add_action( "use_item,effect_name=gladiators_badge,if=!talent.sun_kings_blessing&cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.remains>7|fight_remains<20" );
  cds->add_action( "use_item,name=flarendos_pilot_light,if=!talent.sun_kings_blessing&cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.remains>7|fight_remains<20" );
  cds->add_action( "use_item,name=house_of_cards,if=!talent.sun_kings_blessing&cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.remains>7|fight_remains<20" );
  cds->add_action( "use_item,name=signet_of_the_priory,if=!talent.sun_kings_blessing&cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.remains>7|fight_remains<20" );
  cds->add_action( "use_item,name=funhouse_lens,if=!talent.sun_kings_blessing&cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.remains>7|fight_remains<20" );
  cds->add_action( "use_item,name=soulletting_ruby,if=!talent.sun_kings_blessing&cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.remains>7|fight_remains<20" );
  cds->add_action( "use_item,name=quickwick_candlestick,if=!talent.sun_kings_blessing&cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.remains>7|fight_remains<20" );
  cds->add_action( "use_item,name=hyperthread_wristwraps,if=hyperthread_wristwraps.fire_blast>=2&buff.combustion.remains&action.fire_blast.charges=0" );
  cds->add_action( "use_items" );
  cds->add_action( "blood_fury,if=!talent.sun_kings_blessing&cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.remains>7|fight_remains<20" );
  cds->add_action( "berserking,if=buff.combustion.remains>7|fight_remains<20" );
  cds->add_action( "fireblood,if=buff.combustion.remains>7|fight_remains<10" );
  cds->add_action( "ancestral_call,if=!talent.sun_kings_blessing&cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.remains>7|fight_remains<20" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down&(cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.remains>7|fight_remains<20)" );
  cds->add_action( "invoke_external_buff,name=blessing_of_summer,if=buff.blessing_of_summer.down" );

  ff_combustion->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=buff.combustion.down&(action.fireball.executing&action.fireball.execute_remains<variable.cast_remains_time|action.meteor.in_flight&action.meteor.in_flight_remains<variable.cast_remains_time|action.pyroblast.executing&action.pyroblast.execute_remains<variable.cast_remains_time|action.scorch.executing&action.scorch.execute_remains<variable.cast_remains_time)" );
  ff_combustion->add_action( "meteor,if=buff.combustion.down|buff.combustion.remains>2" );
  ff_combustion->add_action( "scorch,if=buff.combustion.down&(buff.heat_shimmer.react&talent.improved_scorch|improved_scorch.active)&!prev_gcd.1.scorch" );
  ff_combustion->add_action( "flamestrike,if=buff.fury_of_the_sun_king.up&active_enemies>=variable.ff_combustion_flamestrike" );
  ff_combustion->add_action( "pyroblast,if=buff.fury_of_the_sun_king.up" );
  ff_combustion->add_action( "fireball,if=buff.combustion.down" );
  ff_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&gcd.remains<gcd.max&buff.combustion.up&!buff.hot_streak.react&hot_streak_spells_in_flight+buff.heating_up.react*(gcd.remains>0)<2&(buff.fury_of_the_sun_king.down|action.pyroblast.executing)" );
  ff_combustion->add_action( "flamestrike,if=buff.hyperthermia.react&active_enemies>=variable.ff_combustion_flamestrike" );
  ff_combustion->add_action( "flamestrike,if=buff.hot_streak.react&active_enemies>=variable.ff_combustion_flamestrike" );
  ff_combustion->add_action( "pyroblast,if=buff.hyperthermia.react" );
  ff_combustion->add_action( "pyroblast,if=buff.hot_streak.react" );
  ff_combustion->add_action( "phoenix_flames,if=buff.excess_frost.up&(!action.pyroblast.in_flight|!buff.heating_up.react)" );
  ff_combustion->add_action( "fireball" );

  ff_filler->add_action( "meteor,if=cooldown.combustion.remains>variable.pooling_time" );
  ff_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.heating_up.react&action.fireball.executing&action.fireball.execute_remains<0.5&(cooldown.combustion.remains>variable.pooling_time|talent.sun_kings_blessing)" );
  ff_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.heating_up.react&!buff.hot_streak.react&action.scorch.executing&action.scorch.execute_remains<0.5&(cooldown.combustion.remains>variable.pooling_time|talent.sun_kings_blessing)" );
  ff_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&charges=3" );
  ff_filler->add_action( "scorch,if=(improved_scorch.active|buff.heat_shimmer.react&talent.improved_scorch)&debuff.improved_scorch.remains<3*gcd.max&!prev_gcd.1.scorch" );
  ff_filler->add_action( "flamestrike,if=buff.fury_of_the_sun_king.up&active_enemies>=variable.ff_filler_flamestrike" );
  ff_filler->add_action( "flamestrike,if=buff.hyperthermia.react&active_enemies>=variable.ff_filler_flamestrike" );
  ff_filler->add_action( "flamestrike,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies>=variable.ff_filler_flamestrike" );
  ff_filler->add_action( "flamestrike,if=buff.hot_streak.react&active_enemies>=variable.ff_filler_flamestrike" );
  ff_filler->add_action( "pyroblast,if=buff.fury_of_the_sun_king.up" );
  ff_filler->add_action( "pyroblast,if=buff.hyperthermia.react" );
  ff_filler->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react" );
  ff_filler->add_action( "pyroblast,if=buff.hot_streak.react" );
  ff_filler->add_action( "shifting_power,if=cooldown.combustion.remains>10&!firestarter.active" );
  ff_filler->add_action( "fireball,if=talent.sun_kings_blessing&buff.frostfire_empowerment.react" );
  ff_filler->add_action( "phoenix_flames,if=buff.excess_frost.up|talent.sun_kings_blessing" );
  ff_filler->add_action( "scorch,if=talent.sun_kings_blessing&(scorch_execute.active|buff.heat_shimmer.react)" );
  ff_filler->add_action( "fireball" );

  sf_combustion->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=buff.combustion.down&(action.fireball.executing&action.fireball.execute_remains<variable.cast_remains_time|action.scorch.executing&action.scorch.execute_remains<variable.cast_remains_time)" );
  sf_combustion->add_action( "scorch,if=buff.combustion.down" );
  sf_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&gcd.remains<gcd.max&buff.combustion.up&!buff.hot_streak.react&hot_streak_spells_in_flight+buff.heating_up.react*(gcd.remains>0)<2" );
  sf_combustion->add_action( "flamestrike,if=buff.hyperthermia.react&active_enemies>=variable.sf_combustion_flamestrike" );
  sf_combustion->add_action( "flamestrike,if=buff.hot_streak.react&active_enemies>=variable.sf_combustion_flamestrike" );
  sf_combustion->add_action( "flamestrike,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies>=variable.sf_combustion_flamestrike" );
  sf_combustion->add_action( "pyroblast,if=buff.hyperthermia.react" );
  sf_combustion->add_action( "pyroblast,if=buff.hot_streak.react" );
  sf_combustion->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react" );
  sf_combustion->add_action( "scorch,if=buff.heat_shimmer.react&!scorch_execute.active" );
  sf_combustion->add_action( "phoenix_flames" );
  sf_combustion->add_action( "scorch" );

  sf_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.heating_up.react&buff.hyperthermia.react&cooldown.combustion.remains>variable.pooling_time" );
  sf_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.heating_up.react&action.fireball.executing&action.fireball.execute_remains<0.5&cooldown.combustion.remains>variable.pooling_time" );
  sf_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.heating_up.react&!buff.hot_streak.react&action.scorch.executing&action.scorch.execute_remains<0.5&cooldown.combustion.remains>variable.pooling_time" );
  sf_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&charges=3&cooldown.combustion.remains>variable.pooling_time*0.3" );
  sf_filler->add_action( "fire_blast,use_off_gcd=1,if=active_enemies>=2&cooldown_react&buff.glorious_incandescence.react&!buff.heating_up.react&!buff.hot_streak.react&cooldown.combustion.remains>variable.pooling_time" );
  sf_filler->add_action( "flamestrike,if=buff.hyperthermia.react&active_enemies>=variable.sf_filler_flamestrike" );
  sf_filler->add_action( "flamestrike,if=buff.hot_streak.react&active_enemies>=variable.sf_filler_flamestrike" );
  sf_filler->add_action( "flamestrike,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies>=variable.sf_filler_flamestrike" );
  sf_filler->add_action( "pyroblast,if=buff.hyperthermia.react" );
  sf_filler->add_action( "pyroblast,if=buff.hot_streak.react&cooldown.combustion.remains>variable.pooling_time*0.3" );
  sf_filler->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react&cooldown.combustion.remains>variable.pooling_time*0.3" );
  sf_filler->add_action( "shifting_power,if=cooldown.combustion.remains>8" );
  sf_filler->add_action( "scorch,if=buff.heat_shimmer.react" );
  sf_filler->add_action( "meteor,if=active_enemies>=2" );
  sf_filler->add_action( "phoenix_flames" );
  sf_filler->add_action( "scorch,if=scorch_execute.active" );
  sf_filler->add_action( "fireball" );
}
//fire_apl_end

//frost_apl_start
void frost( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe_ff = p->get_action_priority_list( "aoe_ff" );
  action_priority_list_t* aoe_ss = p->get_action_priority_list( "aoe_ss" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* cleave_ff = p->get_action_priority_list( "cleave_ff" );
  action_priority_list_t* cleave_ss = p->get_action_priority_list( "cleave_ss" );
  action_priority_list_t* movement = p->get_action_priority_list( "movement" );
  action_priority_list_t* st_ff = p->get_action_priority_list( "st_ff" );
  action_priority_list_t* st_ss = p->get_action_priority_list( "st_ss" );

  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=treacherous_transmitter_precombat_cast,value=12,if=equipped.treacherous_transmitter" );
  precombat->add_action( "use_item,name=treacherous_transmitter" );
  precombat->add_action( "use_item,name=ingenious_mana_battery" );
  precombat->add_action( "blizzard,if=active_enemies>=3" );
  precombat->add_action( "frostbolt,if=active_enemies<=2" );

  default_->add_action( "counterspell" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "run_action_list,name=aoe_ff,if=talent.frostfire_bolt&active_enemies>=3" );
  default_->add_action( "run_action_list,name=aoe_ss,if=active_enemies>=3" );
  default_->add_action( "run_action_list,name=cleave_ff,if=talent.frostfire_bolt&active_enemies=2" );
  default_->add_action( "run_action_list,name=cleave_ss,if=active_enemies=2" );
  default_->add_action( "run_action_list,name=st_ff,if=talent.frostfire_bolt" );
  default_->add_action( "run_action_list,name=st_ss" );

  aoe_ff->add_action( "frostfire_bolt,if=talent.deaths_chill&buff.icy_veins.remains>9&(buff.deaths_chill.stack<9|buff.deaths_chill.stack=9&!action.frostfire_bolt.in_flight)" );
  aoe_ff->add_action( "cone_of_cold,if=talent.coldest_snap&prev_gcd.1.comet_storm" );
  aoe_ff->add_action( "freeze,if=freezable&(prev_gcd.1.glacial_spike|prev_gcd.1.comet_storm&time-action.cone_of_cold.last_used>8)" );
  aoe_ff->add_action( "ice_nova,if=freezable&!prev_off_gcd.freeze&(prev_gcd.1.glacial_spike&remaining_winters_chill=0&debuff.winters_chill.down|prev_gcd.1.comet_storm&time-action.cone_of_cold.last_used>8)" );
  aoe_ff->add_action( "frozen_orb" );
  aoe_ff->add_action( "ice_lance,if=buff.excess_fire.stack=2&action.comet_storm.cooldown_react" );
  aoe_ff->add_action( "blizzard,if=talent.ice_caller|talent.freezing_rain" );
  aoe_ff->add_action( "comet_storm,if=cooldown.cone_of_cold.remains>10|cooldown.cone_of_cold.ready" );
  aoe_ff->add_action( "ray_of_frost,if=talent.splintering_ray&remaining_winters_chill" );
  aoe_ff->add_action( "glacial_spike,if=buff.icicles.react=5" );
  aoe_ff->add_action( "flurry,if=cooldown_react&buff.excess_fire.up&buff.excess_frost.up" );
  aoe_ff->add_action( "flurry,if=cooldown_react&remaining_winters_chill=0&debuff.winters_chill.down" );
  aoe_ff->add_action( "frostfire_bolt,if=buff.frostfire_empowerment.react&!buff.excess_fire.up" );
  aoe_ff->add_action( "shifting_power,if=cooldown.icy_veins.remains>10&cooldown.frozen_orb.remains>10&(!talent.comet_storm|cooldown.comet_storm.remains>10)" );
  aoe_ff->add_action( "ice_lance,if=buff.fingers_of_frost.react|remaining_winters_chill" );
  aoe_ff->add_action( "frostfire_bolt" );
  aoe_ff->add_action( "call_action_list,name=movement" );

  aoe_ss->add_action( "cone_of_cold,if=talent.coldest_snap&!action.frozen_orb.cooldown_react&(prev_gcd.1.comet_storm|prev_gcd.1.frozen_orb&cooldown.comet_storm.remains>5)&(!talent.deaths_chill|buff.icy_veins.remains<9|buff.deaths_chill.stack>=15)" );
  aoe_ss->add_action( "freeze,if=freezable&(prev_gcd.1.glacial_spike|!talent.glacial_spike)" );
  aoe_ss->add_action( "flurry,if=cooldown_react&remaining_winters_chill=0&debuff.winters_chill.down&prev_gcd.1.glacial_spike" );
  aoe_ss->add_action( "ice_nova,if=freezable&!prev_off_gcd.freeze&prev_gcd.1.glacial_spike&remaining_winters_chill=0&debuff.winters_chill.down" );
  aoe_ss->add_action( "ice_nova,if=talent.unerring_proficiency&time-action.cone_of_cold.last_used<10&time-action.cone_of_cold.last_used>7" );
  aoe_ss->add_action( "frozen_orb,if=cooldown_react" );
  aoe_ss->add_action( "blizzard,if=talent.ice_caller|talent.freezing_rain" );
  aoe_ss->add_action( "frostbolt,if=talent.deaths_chill&buff.icy_veins.remains>9&(buff.deaths_chill.stack<12|buff.deaths_chill.stack=12&!action.frostbolt.in_flight)" );
  aoe_ss->add_action( "comet_storm" );
  aoe_ss->add_action( "ray_of_frost,if=talent.splintering_ray&remaining_winters_chill&buff.icy_veins.down" );
  aoe_ss->add_action( "glacial_spike,if=buff.icicles.react=5&(action.flurry.cooldown_react|remaining_winters_chill|freezable&cooldown.ice_nova.ready)" );
  aoe_ss->add_action( "shifting_power,if=cooldown.icy_veins.remains>10&(fight_remains+15>cooldown.icy_veins.remains)" );
  aoe_ss->add_action( "ice_lance,if=buff.fingers_of_frost.react|remaining_winters_chill" );
  aoe_ss->add_action( "flurry,if=cooldown_react&remaining_winters_chill=0&debuff.winters_chill.down" );
  aoe_ss->add_action( "frostbolt" );
  aoe_ss->add_action( "call_action_list,name=movement" );

  cds->add_action( "use_item,name=treacherous_transmitter,if=fight_remains<32+20*equipped.spymasters_web|prev_off_gcd.icy_veins|(cooldown.icy_veins.remains<12|cooldown.icy_veins.remains<22&cooldown.shifting_power.remains<10)" );
  cds->add_action( "do_treacherous_transmitter_task,if=fight_remains<18|(buff.cryptic_instructions.remains<?buff.realigning_nexus_convergence_divergence.remains<?buff.errant_manaforge_emission.remains)<(action.shifting_power.execute_time+1*talent.ray_of_frost)" );
  cds->add_action( "use_item,name=spymasters_web,if=fight_remains<20|buff.icy_veins.remains<19&(fight_remains<105|buff.spymasters_report.stack>=32)&(buff.icy_veins.remains>15|trinket.treacherous_transmitter.cooldown.remains>50)" );
  cds->add_action( "use_item,name=house_of_cards,if=buff.icy_veins.remains>9|fight_remains<20" );
  cds->add_action( "use_item,name=imperfect_ascendancy_serum,if=buff.icy_veins.remains>15|fight_remains<20" );
  cds->add_action( "use_item,name=burst_of_knowledge,if=buff.icy_veins.remains>15|fight_remains<20" );
  cds->add_action( "use_item,name=ratfang_toxin,if=time>10" );
  cds->add_action( "potion,if=fight_remains<35|buff.icy_veins.remains>15" );
  cds->add_action( "icy_veins,if=buff.icy_veins.remains<1.5&(talent.frostfire_bolt|active_enemies>=3)" );
  cds->add_action( "frozen_orb,if=time=0&active_enemies>=3" );
  cds->add_action( "flurry,if=time=0&active_enemies<=2" );
  cds->add_action( "icy_veins,if=buff.icy_veins.remains<1.5&talent.splinterstorm" );
  cds->add_action( "use_item,name=neural_synapse_enhancer,if=active_enemies<=2|prev_gcd.1.comet_storm|fight_remains<20" );
  cds->add_action( "use_items" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down" );
  cds->add_action( "invoke_external_buff,name=blessing_of_summer,if=buff.blessing_of_summer.down" );
  cds->add_action( "blood_fury" );
  cds->add_action( "berserking,if=buff.icy_veins.remains>9&buff.icy_veins.remains<15|fight_remains<15" );
  cds->add_action( "fireblood" );
  cds->add_action( "ancestral_call" );

  cleave_ff->add_action( "frostfire_bolt,if=talent.deaths_chill&buff.icy_veins.remains>9&(buff.deaths_chill.stack<4|buff.deaths_chill.stack=4&!action.frostfire_bolt.in_flight)" );
  cleave_ff->add_action( "freeze,if=freezable&prev_gcd.1.glacial_spike" );
  cleave_ff->add_action( "ice_nova,if=freezable&prev_gcd.1.glacial_spike&remaining_winters_chill=0&debuff.winters_chill.down&!prev_off_gcd.freeze" );
  cleave_ff->add_action( "flurry,target_if=min:debuff.winters_chill.stack,if=cooldown_react&prev_gcd.1.glacial_spike&!prev_off_gcd.freeze" );
  cleave_ff->add_action( "flurry,if=cooldown_react&(buff.icicles.react<5|!talent.glacial_spike)&remaining_winters_chill=0&debuff.winters_chill.down&(prev_gcd.1.frostfire_bolt|prev_gcd.1.comet_storm)" );
  cleave_ff->add_action( "flurry,if=cooldown_react&(buff.icicles.react<5|!talent.glacial_spike)&buff.excess_fire.up&buff.excess_frost.up" );
  cleave_ff->add_action( "comet_storm" );
  cleave_ff->add_action( "frozen_orb" );
  cleave_ff->add_action( "blizzard,if=buff.freezing_rain.up&talent.ice_caller" );
  cleave_ff->add_action( "glacial_spike,if=buff.icicles.react=5" );
  cleave_ff->add_action( "ray_of_frost,target_if=max:debuff.winters_chill.stack,if=remaining_winters_chill=1" );
  cleave_ff->add_action( "frostfire_bolt,if=buff.frostfire_empowerment.react&!buff.excess_fire.up" );
  cleave_ff->add_action( "shifting_power,if=cooldown.icy_veins.remains>10&cooldown.frozen_orb.remains>10&(!talent.comet_storm|cooldown.comet_storm.remains>10)&(!talent.ray_of_frost|cooldown.ray_of_frost.remains>10)" );
  cleave_ff->add_action( "ice_lance,target_if=max:debuff.winters_chill.stack,if=buff.fingers_of_frost.react|remaining_winters_chill" );
  cleave_ff->add_action( "frostfire_bolt" );
  cleave_ff->add_action( "call_action_list,name=movement" );

  cleave_ss->add_action( "flurry,target_if=min:debuff.winters_chill.stack,if=cooldown_react&prev_gcd.1.glacial_spike&!prev_off_gcd.freeze" );
  cleave_ss->add_action( "freeze,if=freezable&prev_gcd.1.glacial_spike" );
  cleave_ss->add_action( "ice_nova,if=freezable&!prev_off_gcd.freeze&remaining_winters_chill=0&debuff.winters_chill.down&prev_gcd.1.glacial_spike" );
  cleave_ss->add_action( "flurry,if=cooldown_react&debuff.winters_chill.down&remaining_winters_chill=0&prev_gcd.1.frostbolt" );
  cleave_ss->add_action( "ice_lance,if=buff.fingers_of_frost.react=2" );
  cleave_ss->add_action( "comet_storm,if=remaining_winters_chill&buff.icy_veins.down" );
  cleave_ss->add_action( "frozen_orb,if=cooldown_react&(cooldown.icy_veins.remains>30|buff.icy_veins.react)" );
  cleave_ss->add_action( "ray_of_frost,target_if=max:debuff.winters_chill.stack,if=prev_gcd.1.flurry&buff.icy_veins.down" );
  cleave_ss->add_action( "glacial_spike,if=buff.icicles.react=5&(action.flurry.cooldown_react|remaining_winters_chill|freezable&cooldown.ice_nova.ready)" );
  cleave_ss->add_action( "shifting_power,if=cooldown.icy_veins.remains>10&!action.flurry.cooldown_react&(fight_remains+15>cooldown.icy_veins.remains)" );
  cleave_ss->add_action( "frostbolt,if=talent.deaths_chill&buff.icy_veins.remains>9&(buff.deaths_chill.stack<6|buff.deaths_chill.stack=6&!action.frostbolt.in_flight)" );
  cleave_ss->add_action( "blizzard,if=talent.freezing_rain&talent.ice_caller" );
  cleave_ss->add_action( "ice_lance,target_if=max:debuff.winters_chill.stack,if=buff.fingers_of_frost.react|remaining_winters_chill" );
  cleave_ss->add_action( "frostbolt" );
  cleave_ss->add_action( "call_action_list,name=movement" );

  movement->add_action( "any_blink,if=movement.distance>10" );
  movement->add_action( "ice_floes,if=buff.ice_floes.down" );
  movement->add_action( "ice_nova" );
  movement->add_action( "cone_of_cold,if=!talent.coldest_snap&active_enemies>=2" );
  movement->add_action( "arcane_explosion,if=mana.pct>30&active_enemies>=2" );
  movement->add_action( "fire_blast" );
  movement->add_action( "ice_lance" );

  st_ff->add_action( "flurry,if=cooldown_react&(buff.icicles.react<5|!talent.glacial_spike)&remaining_winters_chill=0&debuff.winters_chill.down&(prev_gcd.1.glacial_spike|prev_gcd.1.frostfire_bolt|prev_gcd.1.comet_storm)" );
  st_ff->add_action( "flurry,if=cooldown_react&(buff.icicles.react<5|!talent.glacial_spike)&buff.excess_fire.up&buff.excess_frost.up" );
  st_ff->add_action( "comet_storm" );
  st_ff->add_action( "glacial_spike,if=buff.icicles.react=5" );
  st_ff->add_action( "ray_of_frost,if=remaining_winters_chill=1" );
  st_ff->add_action( "frozen_orb" );
  st_ff->add_action( "shifting_power,if=cooldown.icy_veins.remains>10&cooldown.frozen_orb.remains>10&(!talent.comet_storm|cooldown.comet_storm.remains>10)&(!talent.ray_of_frost|cooldown.ray_of_frost.remains>10)" );
  st_ff->add_action( "ice_lance,if=buff.fingers_of_frost.react|remaining_winters_chill" );
  st_ff->add_action( "frostfire_bolt" );
  st_ff->add_action( "call_action_list,name=movement" );

  st_ss->add_action( "flurry,if=cooldown_react&debuff.winters_chill.down&remaining_winters_chill=0&(prev_gcd.1.glacial_spike|prev_gcd.1.frostbolt)" );
  st_ss->add_action( "comet_storm,if=remaining_winters_chill&buff.icy_veins.down" );
  st_ss->add_action( "frozen_orb,if=cooldown_react&(cooldown.icy_veins.remains>30|buff.icy_veins.react)" );
  st_ss->add_action( "ray_of_frost,if=prev_gcd.1.flurry" );
  st_ss->add_action( "glacial_spike,if=buff.icicles.react=5&(action.flurry.cooldown_react|remaining_winters_chill)" );
  st_ss->add_action( "shifting_power,if=cooldown.icy_veins.remains>10&!action.flurry.cooldown_react&(fight_remains+15>cooldown.icy_veins.remains)" );
  st_ss->add_action( "ice_lance,if=buff.fingers_of_frost.react|remaining_winters_chill" );
  st_ss->add_action( "frostbolt" );
  st_ss->add_action( "call_action_list,name=movement" );
}
//frost_apl_end

}  // namespace mage_apl
