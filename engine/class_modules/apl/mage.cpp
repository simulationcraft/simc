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
  action_priority_list_t* sunfury = p->get_action_priority_list( "sunfury" );

  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "variable,name=aoe_target_count,op=reset,default=2" );
  precombat->add_action( "variable,name=aoe_target_count,op=set,value=9,if=!talent.arcing_cleave" );
  precombat->add_action( "variable,name=opener,op=set,value=1" );
  precombat->add_action( "variable,name=aoe_list,default=0,op=reset" );
  precombat->add_action( "variable,name=steroid_trinket_equipped,op=set,value=equipped.gladiators_badge|equipped.signet_of_the_priory|equipped.imperfect_ascendancy_serum|equipped.quickwick_candlestick|equipped.soulletting_ruby|equipped.funhouse_lens|equipped.house_of_cards|equipped.flarendos_pilot_light|equipped.neural_synapse_enhancer|equipped.lily_of_the_eternal_weave|equipped.sunblood_amethyst|equipped.arazs_ritual_forge|equipped.incorporeal_essencegorger" );
  precombat->add_action( "variable,name=neural_on_mini,op=set,value=equipped.gladiators_badge|equipped.signet_of_the_priory|equipped.soulletting_ruby|equipped.funhouse_lens|equipped.house_of_cards|equipped.flarendos_pilot_light|equipped.lily_of_the_eternal_weave|equipped.sunblood_amethyst|equipped.arazs_ritual_forge|equipped.incorporeal_essencegorger" );
  precombat->add_action( "variable,name=nonsteroid_trinket_equipped,op=set,value=equipped.blastmaster3000|equipped.ratfang_toxin|equipped.ingenious_mana_battery|equipped.geargrinders_spare_keys|equipped.ringing_ritual_mud|equipped.goo_blin_grenade|equipped.noggenfogger_ultimate_deluxe|equipped.garbagemancers_last_resort|equipped.mad_queens_mandate|equipped.fearbreakers_echo|equipped.mereldars_toll|equipped.gooblin_grenade|equipped.perfidious_projector|equipped.chaotic_nethergate" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "use_item,name=ingenious_mana_battery" );
  precombat->add_action( "variable,name=treacherous_transmitter_precombat_cast,value=11" );
  precombat->add_action( "use_item,name=treacherous_transmitter" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "use_item,name=imperfect_ascendancy_serum" );
  precombat->add_action( "arcane_blast,if=!talent.evocation" );
  precombat->add_action( "evocation,if=talent.evocation" );

  default_->add_action( "counterspell" );
  default_->add_action( "potion,if=(buff.siphon_storm.up|(!talent.evocation&cooldown.arcane_surge.ready))" );
  default_->add_action( "lights_judgment,if=(buff.arcane_surge.down&debuff.touch_of_the_magi.down&active_enemies>=2)" );
  default_->add_action( "berserking,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "blood_fury,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "fireblood,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "ancestral_call,if=prev_gcd.1.arcane_surge" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=prev_gcd.1.arcane_surge", "Invoke Externals with cooldowns except Autumn which should come just after cooldowns" );
  default_->add_action( "invoke_external_buff,name=blessing_of_autumn,if=cooldown.touch_of_the_magi.remains>5" );
  default_->add_action( "use_items,if=(prev_gcd.1.arcane_surge&variable.steroid_trinket_equipped)|(!variable.steroid_trinket_equipped&variable.nonsteroid_trinket_equipped)|(variable.nonsteroid_trinket_equipped&buff.siphon_storm.remains<10&(cooldown.evocation.remains>17|trinket.cooldown.remains>20))|fight_remains<20", "Trinket specific use cases vary, default is just with cooldowns" );
  default_->add_action( "use_item,name=neural_synapse_enhancer,if=(debuff.touch_of_the_magi.remains>8&buff.arcane_surge.up)|(debuff.touch_of_the_magi.remains>8&variable.neural_on_mini)" );
  default_->add_action( "variable,name=opener,op=set,if=debuff.touch_of_the_magi.up&variable.opener,value=0" );
  default_->add_action( "arcane_barrage,if=fight_remains<2" );
  default_->add_action( "call_action_list,name=cd_opener", "Enter cooldowns, then action list depending on your hero talent choices" );
  default_->add_action( "call_action_list,name=sunfury,if=talent.spellfire_spheres" );
  default_->add_action( "call_action_list,name=spellslinger,if=!talent.spellfire_spheres" );
  default_->add_action( "arcane_barrage" );

  cd_opener->add_action( "touch_of_the_magi,use_off_gcd=1,if=prev_gcd.1.arcane_barrage&(action.arcane_barrage.in_flight_remains<=0.5|gcd.remains<=0.5)&(buff.arcane_surge.up|cooldown.arcane_surge.remains>30)|(prev_gcd.1.arcane_surge&(buff.arcane_charge.stack<4|buff.nether_precision.down))|(cooldown.arcane_surge.remains>30&cooldown.touch_of_the_magi.ready&buff.arcane_charge.stack<4&!prev_gcd.1.arcane_barrage)", "Touch of the Magi used when Arcane Barrage is mid-flight or if you just used Arcane Surge and you don't have 4 Arcane Charges, the wait simulates the time it takes to queue another spell after Touch when you Surge into Touch, throws up Touch as soon as possible even without Barraging first if it's ready for miniburn." );
  cd_opener->add_action( "wait,sec=0.05,if=prev_gcd.1.arcane_surge&time-action.touch_of_the_magi.last_used<0.015,line_cd=15" );
  cd_opener->add_action( "arcane_blast,if=buff.presence_of_mind.up" );
  cd_opener->add_action( "arcane_orb,if=talent.high_voltage&variable.opener,line_cd=10", "Use Orb for Charges on the opener if you have High Voltage as the Missiles will generate the remaining Charge you need" );
  cd_opener->add_action( "arcane_barrage,if=buff.arcane_tempo.up&cooldown.evocation.ready&buff.arcane_tempo.remains<gcd.max*5,line_cd=11", "Barrage before Evocation if Tempo will expire" );
  cd_opener->add_action( "evocation,if=cooldown.arcane_surge.remains<(gcd.max*3)&cooldown.touch_of_the_magi.remains<(gcd.max*5)" );
  cd_opener->add_action( "arcane_missiles,if=((prev_gcd.1.evocation|prev_gcd.1.arcane_surge)|variable.opener)&buff.nether_precision.down,interrupt_if=tick_time>gcd.remains&buff.aether_attunement.react=0,interrupt_immediate=1,interrupt_global=1,chain=1,line_cd=30", "Use Missiles to get Nether Precision up for your burst window, clipping logic applies as long as you don't have Aether Attunement." );
  cd_opener->add_action( "arcane_surge,if=cooldown.touch_of_the_magi.remains<(action.arcane_surge.execute_time+(gcd.max*(buff.arcane_charge.stack=4)))" );

  spellslinger->add_action( "shifting_power,if=(((((action.arcane_orb.charges=0)&cooldown.arcane_orb.remains>16)|cooldown.touch_of_the_magi.remains<20)&buff.arcane_surge.down&buff.siphon_storm.down&debuff.touch_of_the_magi.down&(buff.intuition.react=0|(buff.intuition.react&buff.intuition.remains>cast_time))&cooldown.touch_of_the_magi.remains>(12+6*gcd.max))|(prev_gcd.1.arcane_barrage&talent.shifting_shards&(buff.intuition.react=0|(buff.intuition.react&buff.intuition.remains>cast_time))&(buff.arcane_surge.up|debuff.touch_of_the_magi.up|cooldown.evocation.remains<20)))&fight_remains>10&(buff.arcane_tempo.remains>gcd.max*2.5|buff.arcane_tempo.down)", "With Shifting Shards we can use Shifting Power whenever basically favoring cooldowns slightly, without it though we want to use it outside of cooldowns, don't cast if it'll conflict with Intuition expiration." );
  spellslinger->add_action( "cancel_buff,name=presence_of_mind,use_off_gcd=1,if=prev_gcd.1.arcane_blast&buff.presence_of_mind.stack=1", "In single target, use Presence of Mind at the very end of Touch of the Magi, then cancelaura the buff to start the cooldown, wait is to simulate the delay of hitting Presence of Mind after another spell cast." );
  spellslinger->add_action( "presence_of_mind,if=debuff.touch_of_the_magi.remains<=gcd.max&buff.nether_precision.up&active_enemies<variable.aoe_target_count&!talent.unerring_proficiency" );
  spellslinger->add_action( "wait,sec=0.05,if=time-action.presence_of_mind.last_used<0.015,line_cd=15" );
  spellslinger->add_action( "supernova,if=debuff.touch_of_the_magi.remains<=gcd.max&buff.unerring_proficiency.stack=30" );
  spellslinger->add_action( "arcane_orb,if=buff.arcane_charge.stack<4", "Orb if you need charges." );
  spellslinger->add_action( "arcane_barrage,if=(buff.arcane_tempo.up&buff.arcane_tempo.remains<gcd.max)", "Barrage if Tempo is about to expire." );
  spellslinger->add_action( "arcane_missiles,if=buff.aether_attunement.react&cooldown.touch_of_the_magi.remains<gcd.max*3&buff.clearcasting.react&set_bonus.thewarwithin_season_2_4pc", "Use Aether Attunement up before casting Touch if you have S2 4pc equipped to avoid munching." );
  spellslinger->add_action( "arcane_barrage,if=(cooldown.touch_of_the_magi.ready|cooldown.touch_of_the_magi.remains<((travel_time+0.05)>?gcd.max))&(cooldown.arcane_surge.remains>30&cooldown.arcane_surge.remains<75)", "Barrage if Touch is up or will be up while Barrage is in the air." );
  spellslinger->add_action( "arcane_barrage,if=buff.arcane_charge.stack=4&buff.arcane_harmony.stack>=20&set_bonus.thewarwithin_season_3_4pc", "Anticipate the Intuition granted from the Season 3 set bonus." );
  spellslinger->add_action( "arcane_missiles,if=(buff.clearcasting.react&buff.nether_precision.down&((cooldown.touch_of_the_magi.remains>gcd.max*7&cooldown.arcane_surge.remains>gcd.max*7)|buff.clearcasting.react>1|!talent.magis_spark|(cooldown.touch_of_the_magi.remains<gcd.max*4&buff.aether_attunement.react=0)|set_bonus.thewarwithin_season_2_4pc))|(fight_remains<5&buff.clearcasting.react),interrupt_if=tick_time>gcd.remains&(buff.aether_attunement.react=0|(active_enemies>3&(!talent.time_loop|talent.resonance))),interrupt_immediate=1,interrupt_global=1,chain=1", "Use Clearcasting procs to keep Nether Precision up, if you don't have S2 4pc try to pool Aether Attunement for cooldown windows." );
  spellslinger->add_action( "arcane_missiles,if=talent.high_voltage&(buff.clearcasting.react>1|(buff.clearcasting.react&buff.aether_attunement.react))&buff.arcane_charge.stack<3,interrupt_if=tick_time>gcd.remains&(buff.aether_attunement.react=0|(active_enemies>3&(!talent.time_loop|talent.resonance))),interrupt_immediate=1,interrupt_global=1,chain=1", "Missile to refill charges if you have High Voltage and either Aether Attunement or more than one Clearcasting proc. Recheck AOE" );
  spellslinger->add_action( "arcane_barrage,if=buff.intuition.react", "Use Intuition." );
  spellslinger->add_action( "arcane_blast,if=debuff.magis_spark_arcane_blast.up|buff.leydrinker.up,line_cd=2", "Make sure to always activate Spark!" );
  spellslinger->add_action( "arcane_blast,if=buff.nether_precision.up&buff.arcane_harmony.stack<=16&buff.arcane_charge.stack=4&active_enemies=1", "In single target, spending your Nether Precision stacks on Blast is a higher priority in single target." );
  spellslinger->add_action( "arcane_barrage,if=mana.pct<10&buff.arcane_surge.down&(cooldown.arcane_orb.remains<gcd.max)", "Barrage if you're going to run out of mana and have Orb ready." );
  spellslinger->add_action( "arcane_orb,if=active_enemies=1&(cooldown.touch_of_the_magi.remains<6|!talent.charged_orb|buff.arcane_surge.up|cooldown.arcane_orb.charges_fractional>1.5)", "Orb in ST if you don't have Charged Orb, will overcap soon, and before entering cooldowns." );
  spellslinger->add_action( "arcane_barrage,if=active_enemies>=2&buff.arcane_charge.stack=4&cooldown.arcane_orb.remains<gcd.max&(buff.arcane_harmony.stack<=(8+(10*!set_bonus.thewarwithin_season_3_4pc)))&(((prev_gcd.1.arcane_barrage|prev_gcd.1.arcane_orb)&buff.nether_precision.stack=1)|buff.nether_precision.stack=2|buff.nether_precision.down)", "Barrage if you have orb coming off cooldown in AOE and you don't have enough harmony stacks to make it worthwhile to hold for set proc." );
  spellslinger->add_action( "arcane_barrage,if=active_enemies>2&(buff.arcane_charge.stack=4&!set_bonus.thewarwithin_season_3_4pc)" );
  spellslinger->add_action( "arcane_orb,if=active_enemies>1&buff.arcane_harmony.stack<20&(buff.arcane_surge.up|buff.nether_precision.up|active_enemies>=7)&set_bonus.thewarwithin_season_3_4pc", "Orb if you're low on Harmony stacs." );
  spellslinger->add_action( "arcane_barrage,if=talent.high_voltage&active_enemies>=2&buff.arcane_charge.stack=4&buff.aether_attunement.react&buff.clearcasting.react", "Arcane Barrage in AOE if you have Aether Attunement ready and High Voltage" );
  spellslinger->add_action( "arcane_orb,if=active_enemies>1&(active_enemies<3|buff.arcane_surge.up|(buff.nether_precision.up))&set_bonus.thewarwithin_season_3_4pc", "Use Orb more aggressively if cleave and a little less in AOE." );
  spellslinger->add_action( "arcane_barrage,if=active_enemies>1&buff.arcane_charge.stack=4&cooldown.arcane_orb.remains<gcd.max", "Barrage if Orb is available in AOE." );
  spellslinger->add_action( "arcane_barrage,if=talent.high_voltage&buff.arcane_charge.stack=4&buff.clearcasting.react&buff.nether_precision.stack=1", "If you have High Voltage throw out a Barrage before you need to use Clearcasting for NP." );
  spellslinger->add_action( "arcane_barrage,if=(active_enemies=1&(talent.orb_barrage|(target.health.pct<35&talent.arcane_bombardment))&(cooldown.arcane_orb.remains<gcd.max)&buff.arcane_charge.stack=4&(cooldown.touch_of_the_magi.remains>gcd.max*6|!talent.magis_spark)&(buff.nether_precision.down|(buff.nether_precision.stack=1&buff.clearcasting.stack=0)))&!set_bonus.thewarwithin_season_3_4pc", "Barrage with Orb Barrage or execute if you have orb up and no Nether Precision or no way to get another and use Arcane Orb to recover Arcane Charges, hold resources for Touch of the Magi if you have Magi's Spark. Skip this with Season 3 set." );
  spellslinger->add_action( "arcane_explosion,if=active_enemies>1&((buff.arcane_charge.stack<1&!talent.high_voltage)|(buff.arcane_charge.stack<3&(buff.clearcasting.react=0|talent.reverberate)))", "Use Explosion for your first charge or if you have High Voltage you can use it for charge 2 and 3, but at a slightly higher target count." );
  spellslinger->add_action( "arcane_explosion,if=active_enemies=1&buff.arcane_charge.stack<2&buff.clearcasting.react=0", "You can use Arcane Explosion in single target for your first 2 charges when you have no Clearcasting procs and aren't out of mana. This is only a very slight gain for some profiles so don't feel you have to do this." );
  spellslinger->add_action( "arcane_barrage,if=(((target.health.pct<35&(debuff.touch_of_the_magi.remains<(gcd.max*1.25))&(debuff.touch_of_the_magi.remains>action.arcane_barrage.travel_time))|((buff.arcane_surge.remains<gcd.max)&buff.arcane_surge.up))&buff.arcane_charge.stack=4)&!set_bonus.thewarwithin_season_3_4pc", "Barrage in execute if you're at the end of Touch or at the end of Surge windows. Skip this with Season 3 set." );
  spellslinger->add_action( "arcane_blast", "Nothing else to do? Blast. Out of mana? Barrage." );
  spellslinger->add_action( "arcane_barrage" );

  sunfury->add_action( "shifting_power,if=((buff.arcane_surge.down&buff.siphon_storm.down&debuff.touch_of_the_magi.down&cooldown.evocation.remains>15&cooldown.touch_of_the_magi.remains>10)&fight_remains>10)&buff.arcane_soul.down&(buff.intuition.react=0|(buff.intuition.react&buff.intuition.remains>cast_time))", "For Sunfury, Shifting Power only when you're not under the effect of any cooldowns." );
  sunfury->add_action( "cancel_buff,name=presence_of_mind,use_off_gcd=1,if=(prev_gcd.1.arcane_blast&buff.presence_of_mind.stack=1)|active_enemies<4" );
  sunfury->add_action( "presence_of_mind,if=debuff.touch_of_the_magi.remains<=gcd.max&buff.nether_precision.up&active_enemies<4" );
  sunfury->add_action( "wait,sec=0.05,if=time-action.presence_of_mind.last_used<0.015,line_cd=15" );
  sunfury->add_action( "arcane_missiles,if=buff.nether_precision.down&buff.clearcasting.react&buff.arcane_soul.up&buff.arcane_soul.remains>gcd.max*(4-buff.clearcasting.react),interrupt_if=tick_time>gcd.remains,interrupt_immediate=1,interrupt_global=1,chain=1", "When Arcane Soul is up, use Missiles to generate Nether Precision as needed while also ensuring you end Soul with 3 Clearcasting." );
  sunfury->add_action( "arcane_barrage,if=buff.arcane_soul.up" );
  sunfury->add_action( "arcane_barrage,if=(buff.arcane_tempo.up&buff.arcane_tempo.remains<gcd.max)|(buff.intuition.react&buff.intuition.remains<gcd.max)", "Prioritize Tempo and Intuition if they are about to expire, spend Aether Attunement if you have 4pc S2 set before Touch." );
  sunfury->add_action( "arcane_barrage,if=(talent.orb_barrage&active_enemies>1&buff.arcane_harmony.stack>=18&((active_enemies>3&(talent.resonance|talent.high_voltage))|buff.nether_precision.down|buff.nether_precision.stack=1|(buff.nether_precision.stack=2&buff.clearcasting.react=3)))" );
  sunfury->add_action( "arcane_missiles,if=buff.clearcasting.react&set_bonus.thewarwithin_season_2_4pc&buff.aether_attunement.react&cooldown.touch_of_the_magi.remains<gcd.max*(3-(1.5*(active_enemies>3&(!talent.time_loop|talent.resonance)))),interrupt_if=tick_time>gcd.remains&(buff.aether_attunement.react=0|(active_enemies>3&(!talent.time_loop|talent.resonance))),interrupt_immediate=1,interrupt_global=1,chain=1" );
  sunfury->add_action( "arcane_blast,if=((debuff.magis_spark_arcane_blast.up&((debuff.magis_spark_arcane_blast.remains<(cast_time+gcd.max))|active_enemies=1|talent.leydrinker))|buff.leydrinker.up)&buff.arcane_charge.stack=4&(buff.nether_precision.up|buff.clearcasting.react=0),line_cd=2", "Blast whenever you have the bonus from Leydrinker or Magi's Spark up, don't let spark expire in AOE." );
  sunfury->add_action( "arcane_barrage,if=buff.arcane_charge.stack=4&(cooldown.touch_of_the_magi.ready|cooldown.touch_of_the_magi.remains<((travel_time+0.05)>?gcd.max))", "Barrage into Touch if you have charges when it comes up." );
  sunfury->add_action( "arcane_barrage,if=(talent.high_voltage&active_enemies>1&buff.arcane_charge.stack=4&buff.clearcasting.react&buff.nether_precision.stack=1)", "AOE Barrage conditions are optimized for funnel, avoids overcapping Harmony stacks (line below Tempo line above), spending Charges when you have a way to recoup them via High Voltage or Orb while pooling sometimes for Touch with various talent optimizations." );
  sunfury->add_action( "arcane_barrage,if=(active_enemies>1&talent.high_voltage&buff.arcane_charge.stack=4&buff.clearcasting.react&buff.aether_attunement.react&buff.glorious_incandescence.down&buff.intuition.down)" );
  sunfury->add_action( "arcane_barrage,if=(active_enemies>2&talent.orb_barrage&talent.high_voltage&debuff.magis_spark_arcane_blast.down&buff.arcane_charge.stack=4&target.health.pct<35&talent.arcane_bombardment&(buff.nether_precision.up|(buff.nether_precision.down&buff.clearcasting.stack=0)))" );
  sunfury->add_action( "arcane_barrage,if=((active_enemies>2|(active_enemies>1&target.health.pct<35&talent.arcane_bombardment))&cooldown.arcane_orb.remains<gcd.max&buff.arcane_charge.stack=4&cooldown.touch_of_the_magi.remains>gcd.max*6&(debuff.magis_spark_arcane_blast.down|!talent.magis_spark)&buff.nether_precision.up&(talent.high_voltage|buff.nether_precision.stack=2|(buff.nether_precision.stack=1&buff.clearcasting.react=0)))" );
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
  precombat->add_action( "variable,name=cast_remains_time,value=0.2" );
  precombat->add_action( "variable,name=pooling_time,value=15*gcd.max" );
  precombat->add_action( "variable,name=ff_combustion_flamestrike,if=talent.frostfire_bolt,value=100" );
  precombat->add_action( "variable,name=ff_filler_flamestrike,if=talent.frostfire_bolt,value=100" );
  precombat->add_action( "variable,name=sf_combustion_flamestrike,if=talent.spellfire_spheres,value=100-(50*talent.mark_of_the_firelord)-(44*talent.quickflame)" );
  precombat->add_action( "variable,name=sf_filler_flamestrike,if=talent.spellfire_spheres,value=100" );
  precombat->add_action( "variable,name=treacherous_transmitter_precombat_cast,value=12,if=equipped.treacherous_transmitter" );
  precombat->add_action( "use_item,name=treacherous_transmitter" );
  precombat->add_action( "use_item,name=ingenious_mana_battery,target=self" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "frostfire_bolt,if=talent.frostfire_bolt" );
  precombat->add_action( "combustion,if=!talent.firestarter" );
  precombat->add_action( "pyroblast" );

  default_->add_action( "call_action_list,name=cds,if=!(buff.hot_streak.up&prev_gcd.1.scorch)" );
  default_->add_action( "run_action_list,name=ff_combustion,if=talent.frostfire_bolt&!buff.hyperthermia.react&(cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.up)" );
  default_->add_action( "run_action_list,name=sf_combustion,if=!buff.hyperthermia.react&!(buff.hyperthermia.up&buff.lesser_time_warp.up)&(cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.up)" );
  default_->add_action( "run_action_list,name=ff_filler,if=talent.frostfire_bolt" );
  default_->add_action( "run_action_list,name=sf_filler" );

  cds->add_action( "variable,name=combustion_precast_time,value=talent.frostfire_bolt*(action.fireball.cast_time*(!improved_scorch.active|(action.scorch.cast_time<gcd.max))+action.scorch.cast_time*((action.scorch.cast_time>=gcd.max)&improved_scorch.active))+talent.spellfire_spheres*(action.fireball.cast_time*((action.scorch.cast_time<gcd.max)&buff.hot_streak.react|!talent.scorch)+action.scorch.cast_time*((action.scorch.cast_time>=gcd.max)|!buff.hot_streak.react))-variable.cast_remains_time" );
  cds->add_action( "potion,if=time=0|buff.combustion.remains>6|fight_remains<35" );
  cds->add_action( "use_item,name=arazs_ritual_forge,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=neural_synapse_enhancer,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,effect_name=gladiators_badge,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=signet_of_the_priory,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=sunblood_amethyst,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=lily_of_the_eternal_weave,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=funhouse_lens,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=mereldars_toll,if=buff.combustion.remains>6|fight_remains<15" );
  cds->add_action( "use_item,name=flarendos_pilot_light,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=house_of_cards,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=soulletting_ruby,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=quickwick_candlestick,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=hyperthread_wristwraps,if=hyperthread_wristwraps.fire_blast>=2&buff.combustion.remains&action.fire_blast.charges=0" );
  cds->add_action( "use_items" );
  cds->add_action( "ancestral_call,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "berserking,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "blood_fury,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "fireblood,if=buff.combustion.remains>6|fight_remains<10" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down&(buff.combustion.remains>6|fight_remains<25)" );
  cds->add_action( "invoke_external_buff,name=blessing_of_summer,if=buff.blessing_of_summer.down" );

  ff_combustion->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=buff.combustion.down&action.fireball.executing&(action.fireball.execute_remains<variable.cast_remains_time)|action.meteor.in_flight&(action.meteor.in_flight_remains<variable.cast_remains_time)|action.pyroblast.executing&(action.pyroblast.execute_remains<variable.cast_remains_time)|action.scorch.executing&(action.scorch.execute_remains<variable.cast_remains_time)" );
  ff_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.combustion.up&!action.scorch.executing&!action.fireball.executing&!action.pyroblast.executing&!buff.hot_streak.react&gcd.remains&gcd.remains<gcd.max&(hot_streak_spells_in_flight+buff.heating_up.react)<2&!buff.fury_of_the_sun_king.up" );
  ff_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.combustion.up&buff.heating_up.react&action.pyroblast.executing&action.pyroblast.execute_remains<0.5" );
  ff_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.combustion.up&buff.heating_up.react&action.fireball.executing&action.fireball.execute_remains<0.5" );
  ff_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.combustion.up&!buff.heating_up.react&!buff.hot_streak.react&action.scorch.executing&action.scorch.execute_remains<0.5" );
  ff_combustion->add_action( "flamestrike,if=buff.fury_of_the_sun_king.up&active_enemies>=variable.ff_combustion_flamestrike" );
  ff_combustion->add_action( "pyroblast,if=buff.fury_of_the_sun_king.up" );
  ff_combustion->add_action( "meteor,if=buff.combustion.down|buff.combustion.remains>2" );
  ff_combustion->add_action( "scorch,if=buff.combustion.down&!prev_gcd.1.scorch&cast_time>=gcd.max&(buff.heat_shimmer.react&talent.improved_scorch|improved_scorch.active)" );
  ff_combustion->add_action( "fireball,if=buff.combustion.down" );
  ff_combustion->add_action( "flamestrike,if=buff.hot_streak.react&active_enemies>=variable.ff_combustion_flamestrike" );
  ff_combustion->add_action( "flamestrike,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies>=variable.ff_combustion_flamestrike" );
  ff_combustion->add_action( "pyroblast,if=buff.hot_streak.react" );
  ff_combustion->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react" );
  ff_combustion->add_action( "phoenix_flames,if=buff.excess_frost.up&(!action.pyroblast.in_flight|!buff.heating_up.react)" );
  ff_combustion->add_action( "fireball" );

  ff_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.heating_up.react&action.fireball.executing&action.fireball.execute_remains<0.5&((cooldown.combustion.remains>variable.pooling_time)|talent.sun_kings_blessing)" );
  ff_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.heating_up.react&action.pyroblast.executing&action.pyroblast.execute_remains<0.5" );
  ff_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&!buff.heating_up.react&action.scorch.executing&action.scorch.execute_remains<0.5&((cooldown.combustion.remains>variable.pooling_time)|talent.sun_kings_blessing)" );
  ff_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&action.shifting_power.executing&((cooldown.combustion.remains>variable.pooling_time)|talent.sun_kings_blessing)" );
  ff_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&(hot_streak_spells_in_flight+buff.heating_up.react<2)&buff.hyperthermia.react&gcd.remains<gcd.max&((cooldown.combustion.remains>variable.pooling_time)|talent.sun_kings_blessing)" );
  ff_filler->add_action( "meteor,if=((cooldown.combustion.remains>variable.pooling_time)|talent.sun_kings_blessing)" );
  ff_filler->add_action( "scorch,if=(improved_scorch.active|buff.heat_shimmer.react&talent.improved_scorch)&debuff.improved_scorch.remains<3*gcd.max&!prev_gcd.1.scorch" );
  ff_filler->add_action( "flamestrike,if=buff.fury_of_the_sun_king.up&active_enemies>=variable.ff_filler_flamestrike" );
  ff_filler->add_action( "flamestrike,if=buff.hyperthermia.react&active_enemies>=variable.ff_filler_flamestrike" );
  ff_filler->add_action( "flamestrike,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies>=variable.ff_filler_flamestrike" );
  ff_filler->add_action( "flamestrike,if=buff.hot_streak.react&active_enemies>=variable.ff_filler_flamestrike" );
  ff_filler->add_action( "pyroblast,if=buff.fury_of_the_sun_king.up" );
  ff_filler->add_action( "pyroblast,if=buff.hyperthermia.react" );
  ff_filler->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react" );
  ff_filler->add_action( "pyroblast,if=buff.hot_streak.react" );
  ff_filler->add_action( "shifting_power,if=cooldown.combustion.remains>10" );
  ff_filler->add_action( "fireball,if=talent.sun_kings_blessing&buff.frostfire_empowerment.react" );
  ff_filler->add_action( "phoenix_flames,if=(buff.excess_frost.up|talent.sun_kings_blessing)&!(time-action.frostfire_bolt.last_used<0.5)" );
  ff_filler->add_action( "scorch,if=talent.sun_kings_blessing&(scorch_execute.active|buff.heat_shimmer.react)" );
  ff_filler->add_action( "fireball" );

  sf_combustion->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=action.fireball.executing&(action.fireball.execute_remains<variable.cast_remains_time)|action.scorch.executing&(action.scorch.execute_remains<variable.cast_remains_time)" );
  sf_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.combustion.up&!action.scorch.executing&!action.fireball.executing&!action.pyroblast.executing&!buff.hot_streak.react&gcd.remains&gcd.remains<gcd.max&(!talent.glorious_incandescence|buff.glorious_incandescence.up|charges_fractional>1.7|buff.combustion.remains<(gcd.max*charges))&(hot_streak_spells_in_flight+buff.heating_up.react)<2" );
  sf_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.combustion.up&buff.heating_up.react&action.pyroblast.executing&action.pyroblast.execute_remains<0.5" );
  sf_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.combustion.up&buff.heating_up.react&action.fireball.executing&action.fireball.execute_remains<0.5" );
  sf_combustion->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.combustion.up&!buff.heating_up.react&!buff.hot_streak.react&action.scorch.executing&action.scorch.execute_remains<0.5" );
  sf_combustion->add_action( "scorch,if=buff.combustion.down&(cast_time>=gcd.max|!buff.hot_streak.react)" );
  sf_combustion->add_action( "fireball,if=buff.combustion.down" );
  sf_combustion->add_action( "flamestrike,if=buff.hot_streak.react&active_enemies>=variable.sf_combustion_flamestrike" );
  sf_combustion->add_action( "flamestrike,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies>=variable.sf_combustion_flamestrike" );
  sf_combustion->add_action( "pyroblast,if=buff.hot_streak.react" );
  sf_combustion->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react" );
  sf_combustion->add_action( "scorch,if=(improved_scorch.active|buff.heat_shimmer.react)&debuff.improved_scorch.remains<4*gcd.max" );
  sf_combustion->add_action( "phoenix_flames" );
  sf_combustion->add_action( "scorch" );
  sf_combustion->add_action( "fireball" );

  sf_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&buff.heating_up.react&action.fireball.executing&action.fireball.execute_remains<0.5" );
  sf_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&!buff.heating_up.react&action.scorch.executing&action.scorch.execute_remains<0.5&cooldown.combustion.remains>variable.pooling_time" );
  sf_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&action.shifting_power.executing&cooldown.combustion.remains>variable.pooling_time" );
  sf_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&(hot_streak_spells_in_flight+buff.heating_up.react<2)&(buff.hyperthermia.react|buff.hyperthermia.up&buff.lesser_time_warp.up)&gcd.remains<gcd.max&(!talent.glorious_incandescence|buff.glorious_incandescence.up|charges_fractional>1.7|buff.hyperthermia.remains<(gcd.max*charges))&cooldown.combustion.remains>variable.pooling_time" );
  sf_filler->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&active_enemies>=2&!buff.hot_streak.react&(hot_streak_spells_in_flight+buff.heating_up.react<2)&buff.glorious_incandescence.up&cooldown.combustion.remains>variable.pooling_time" );
  sf_filler->add_action( "flamestrike,if=(buff.hyperthermia.react|buff.hyperthermia.up&buff.lesser_time_warp.up)&active_enemies>=variable.sf_filler_flamestrike" );
  sf_filler->add_action( "flamestrike,if=buff.hot_streak.react&active_enemies>=variable.sf_filler_flamestrike" );
  sf_filler->add_action( "flamestrike,if=prev_gcd.1.scorch&buff.heating_up.react&active_enemies>=variable.sf_filler_flamestrike" );
  sf_filler->add_action( "pyroblast,if=buff.hyperthermia.react|buff.hyperthermia.up&buff.lesser_time_warp.up" );
  sf_filler->add_action( "pyroblast,if=buff.hot_streak.react" );
  sf_filler->add_action( "pyroblast,if=prev_gcd.1.scorch&buff.heating_up.react" );
  sf_filler->add_action( "shifting_power" );
  sf_filler->add_action( "scorch,if=buff.heat_shimmer.react" );
  sf_filler->add_action( "meteor,if=active_enemies>=2" );
  sf_filler->add_action( "phoenix_flames,if=buff.heating_up.react|action.fire_blast.cooldown_react|action.phoenix_flames.charges_fractional>1.5|buff.born_of_flame.react" );
  sf_filler->add_action( "scorch,if=scorch_execute.active" );
  sf_filler->add_action( "fireball" );
}
//fire_apl_end

//frost_apl_start
void frost( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* ff_aoe = p->get_action_priority_list( "ff_aoe" );
  action_priority_list_t* ff_cleave = p->get_action_priority_list( "ff_cleave" );
  action_priority_list_t* ff_st = p->get_action_priority_list( "ff_st" );
  action_priority_list_t* ff_st_boltspam = p->get_action_priority_list( "ff_st_boltspam" );
  action_priority_list_t* movement = p->get_action_priority_list( "movement" );
  action_priority_list_t* ss_aoe = p->get_action_priority_list( "ss_aoe" );
  action_priority_list_t* ss_cleave = p->get_action_priority_list( "ss_cleave" );
  action_priority_list_t* ss_st = p->get_action_priority_list( "ss_st" );

  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=treacherous_transmitter_precombat_cast,value=12,if=equipped.treacherous_transmitter" );
  precombat->add_action( "use_item,name=treacherous_transmitter" );
  precombat->add_action( "use_item,name=ingenious_mana_battery,target=self" );
  precombat->add_action( "blizzard,if=active_enemies>=3" );
  precombat->add_action( "frostbolt,if=active_enemies<=2" );

  default_->add_action( "counterspell" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "run_action_list,name=ff_aoe,if=talent.frostfire_bolt&active_enemies>=3" );
  default_->add_action( "run_action_list,name=ss_aoe,if=active_enemies>=3" );
  default_->add_action( "run_action_list,name=ff_cleave,if=talent.frostfire_bolt&active_enemies=2" );
  default_->add_action( "run_action_list,name=ss_cleave,if=active_enemies=2" );
  default_->add_action( "run_action_list,name=ff_st_boltspam,if=talent.frostfire_bolt&(talent.glacial_spike&talent.slick_ice&talent.cold_front&talent.deaths_chill&talent.deep_shatter)" );
  default_->add_action( "run_action_list,name=ff_st,if=talent.frostfire_bolt" );
  default_->add_action( "run_action_list,name=ss_st" );

  cds->add_action( "flurry,if=time=0&active_enemies<=2&talent.splinterstorm", "Spellslinger uses Icy Veins after the initial Flurry in the ST/Cleave opener to guarantee being in combat for Augury Abounds." );
  cds->add_action( "icy_veins" );
  cds->add_action( "potion,if=buff.icy_veins.remains>15|fight_remains<35", "Potion" );
  cds->add_action( "use_item,name=treacherous_transmitter,if=fight_remains<32+20*equipped.spymasters_web|prev_off_gcd.icy_veins|(cooldown.icy_veins.remains<12|cooldown.icy_veins.remains<22&cooldown.shifting_power.remains<10)", "Trinkets" );
  cds->add_action( "do_treacherous_transmitter_task,if=fight_remains<18|(buff.cryptic_instructions.remains<?buff.realigning_nexus_convergence_divergence.remains<?buff.errant_manaforge_emission.remains)<(action.shifting_power.execute_time+1*talent.ray_of_frost)" );
  cds->add_action( "use_item,name=spymasters_web,if=fight_remains<20|buff.icy_veins.remains<19&(fight_remains<105|buff.spymasters_report.stack>=32)&(buff.icy_veins.remains>15|trinket.treacherous_transmitter.cooldown.remains>50)" );
  cds->add_action( "use_item,name=arazs_ritual_forge" );
  cds->add_action( "use_item,name=signet_of_the_priory" );
  cds->add_action( "use_item,name=sunblood_amethyst,if=buff.icy_veins.remains>10|fight_remains<20" );
  cds->add_action( "use_item,name=lily_of_the_eternal_weave,if=buff.icy_veins.remains>10|fight_remains<20" );
  cds->add_action( "use_item,name=funhouse_lens,if=buff.icy_veins.remains>10|fight_remains<20" );
  cds->add_action( "use_item,name=mereldars_toll,if=buff.icy_veins.remains>10|fight_remains<15" );
  cds->add_action( "use_item,name=house_of_cards,if=buff.icy_veins.remains>10|fight_remains<20" );
  cds->add_action( "use_item,name=flarendos_pilot_light" );
  cds->add_action( "use_item,name=soulletting_ruby" );
  cds->add_action( "use_item,name=quickwick_candlestick,if=buff.icy_veins.remains>10|fight_remains<20" );
  cds->add_action( "use_item,name=imperfect_ascendancy_serum,if=buff.icy_veins.remains>10|fight_remains<20" );
  cds->add_action( "use_item,name=burst_of_knowledge,if=buff.icy_veins.remains>10|fight_remains<20" );
  cds->add_action( "use_item,name=ratfang_toxin,if=time>10" );
  cds->add_action( "use_item,name=neural_synapse_enhancer,if=active_enemies<=2|prev_gcd.1.comet_storm|fight_remains<20" );
  cds->add_action( "use_items" );
  cds->add_action( "flurry,if=time=0&active_enemies<=2", "Opener for AoE and Frostfire ST/Cleave" );
  cds->add_action( "frozen_orb,if=time=0&active_enemies>=3" );
  cds->add_action( "blood_fury", "Racials" );
  cds->add_action( "berserking,if=buff.icy_veins.remains>10|fight_remains<15" );
  cds->add_action( "fireblood" );
  cds->add_action( "ancestral_call" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down", "Externals" );
  cds->add_action( "invoke_external_buff,name=blessing_of_summer,if=buff.blessing_of_summer.down" );

  ff_aoe->add_action( "cone_of_cold,if=talent.coldest_snap&prev_gcd.1.comet_storm" );
  ff_aoe->add_action( "freeze,if=freezable&time-action.cone_of_cold.last_used>8&(prev_gcd.1.glacial_spike&remaining_winters_chill=0&debuff.winters_chill.down|prev_gcd.1.comet_storm)", "Against freezable targets prioritize Pet Freeze and Ice Nova to shatter Glacial Spike and Comet Storm." );
  ff_aoe->add_action( "ice_nova,if=!prev_off_gcd.freeze&freezable&time-action.cone_of_cold.last_used>8&(prev_gcd.1.glacial_spike&remaining_winters_chill=0&debuff.winters_chill.down|prev_gcd.1.comet_storm)" );
  ff_aoe->add_action( "flurry,if=cooldown_react&!prev_off_gcd.freeze&remaining_winters_chill=0&debuff.winters_chill.down&prev_gcd.1.glacial_spike" );
  ff_aoe->add_action( "frozen_orb" );
  ff_aoe->add_action( "blizzard,if=talent.ice_caller|talent.freezing_rain" );
  ff_aoe->add_action( "frostfire_bolt,if=talent.deaths_chill&buff.icy_veins.up&(buff.deaths_chill.stack<9|buff.deaths_chill.stack=9&!action.frostfire_bolt.in_flight)", "During Icy Veins stack Death's Chill to 12 while keeping Blizzard and Frozen Orb on cooldown." );
  ff_aoe->add_action( "ice_lance,if=talent.deaths_chill&buff.excess_fire.stack=2&cooldown.comet_storm.ready", "Don't munch Excess Fire before pressing the 2nd Comet Storm in the Cone of Cold sequence. Only relevant for Deaths Chill builds." );
  ff_aoe->add_action( "comet_storm,if=cooldown.cone_of_cold.remains>12|cooldown.cone_of_cold.ready", "Hold Comet Storm for up to 12 seconds for Cone of Cold." );
  ff_aoe->add_action( "ray_of_frost,if=talent.splintering_ray&remaining_winters_chill=2" );
  ff_aoe->add_action( "glacial_spike,if=buff.icicles.react=5" );
  ff_aoe->add_action( "flurry,if=cooldown_react&buff.excess_frost.up", "Cast Flurry to spend Excess Frost." );
  ff_aoe->add_action( "shifting_power,if=(!equipped.arazs_ritual_forge|buff.icy_veins.down)&cooldown.icy_veins.remains>8&(cooldown.comet_storm.remains>8|!talent.comet_storm)&cooldown.blizzard.remains>6*gcd.max", "With Araz's Ritual Forge equipped only cast Shifting Power outside of Icy Veins to create more overlap with subsequent Icy Veins." );
  ff_aoe->add_action( "frostfire_bolt,if=buff.frostfire_empowerment.react&!buff.excess_fire.up" );
  ff_aoe->add_action( "ice_lance,if=buff.fingers_of_frost.react" );
  ff_aoe->add_action( "ice_lance,if=remaining_winters_chill" );
  ff_aoe->add_action( "frostfire_bolt" );
  ff_aoe->add_action( "call_action_list,name=movement" );

  ff_cleave->add_action( "flurry,target_if=debuff.winters_chill.down,if=cooldown_react&(prev_gcd.1.glacial_spike|prev_gcd.1.frostfire_bolt|prev_gcd.1.comet_storm)", "If one of your targets doesnt have Winter's Chill up, target-swap and queue a Flurry after casting Glacial Spike, Frostfire Bolt or Comet Storm." );
  ff_cleave->add_action( "comet_storm" );
  ff_cleave->add_action( "glacial_spike,if=buff.icicles.react=5" );
  ff_cleave->add_action( "frozen_orb" );
  ff_cleave->add_action( "blizzard,if=buff.icy_veins.down&buff.freezing_rain.up" );
  ff_cleave->add_action( "shifting_power,if=(!equipped.arazs_ritual_forge|buff.icy_veins.down)&cooldown.icy_veins.remains>8&(cooldown.comet_storm.remains>8|!talent.comet_storm)", "With Araz's Ritual Forge equipped only cast Shifting Power outside of Icy Veins to create more overlap with subsequent Icy Veins." );
  ff_cleave->add_action( "ice_lance,if=buff.fingers_of_frost.react" );
  ff_cleave->add_action( "ice_lance,target_if=max:debuff.winters_chill.stack,if=remaining_winters_chill=2&!(talent.slick_ice&talent.cold_front&talent.deaths_chill)", "Cast Ice Lance into 2 Winter's Chill stacks, unless you have all of Death's Chill, Cold Front and Slick Ice talented." );
  ff_cleave->add_action( "frostfire_bolt,target_if=min:debuff.winters_chill.stack", "Always cast Frostfire Bolt at the target with the lowest number of Winter's Chill stacks." );
  ff_cleave->add_action( "call_action_list,name=movement" );

  ff_st->add_action( "flurry,if=cooldown_react&remaining_winters_chill=0&debuff.winters_chill.down&prev_gcd.1.glacial_spike" );
  ff_st->add_action( "flurry,if=cooldown_react&remaining_winters_chill=0&debuff.winters_chill.down&prev_gcd.1.frostfire_bolt&(buff.icicles.react>=3|!talent.glacial_spike)", "Cast Flurry only with 3+ Icicles to guarantee shattering the Glacial Spike + Pyroblast into Winter's Chill. This also means queueing Flurry when casting Frostfire Bolt with 2+ Icicles and overcapping freely." );
  ff_st->add_action( "comet_storm,if=remaining_winters_chill" );
  ff_st->add_action( "ray_of_frost,if=remaining_winters_chill=2" );
  ff_st->add_action( "glacial_spike,if=buff.icicles.react=5" );
  ff_st->add_action( "frozen_orb" );
  ff_st->add_action( "shifting_power,if=(!equipped.arazs_ritual_forge|buff.icy_veins.down)&cooldown.icy_veins.remains>8&(cooldown.comet_storm.remains>8|!talent.comet_storm)", "With Araz's Ritual Forge equipped only cast Shifting Power outside of Icy Veins to create more overlap with subsequent Icy Veins." );
  ff_st->add_action( "ice_lance,if=buff.fingers_of_frost.react" );
  ff_st->add_action( "ice_lance,if=remaining_winters_chill" );
  ff_st->add_action( "frostfire_bolt" );
  ff_st->add_action( "call_action_list,name=movement" );

  ff_st_boltspam->add_action( "flurry,if=cooldown_react&remaining_winters_chill=0&debuff.winters_chill.down&prev_gcd.1.glacial_spike", "The Boltspam roation is used whenever all of Death's Chill, Cold Front, Slick Ice and at least one point of Deep Shatter is talented." );
  ff_st_boltspam->add_action( "flurry,if=cooldown_react&prev_gcd.1.frostfire_bolt&buff.icicles.react>=3", "Queue Flurry whenever casting Frostfire Bolt with 2+ Icicles." );
  ff_st_boltspam->add_action( "comet_storm,if=remaining_winters_chill" );
  ff_st_boltspam->add_action( "glacial_spike,if=buff.icicles.react=5" );
  ff_st_boltspam->add_action( "shifting_power,if=buff.icy_veins.down&cooldown.comet_storm.remains>8&cooldown.icy_veins.remains>8" );
  ff_st_boltspam->add_action( "ice_lance,if=remaining_winters_chill=2" );
  ff_st_boltspam->add_action( "ice_lance,if=buff.fingers_of_frost.react&buff.icicles.react=0" );
  ff_st_boltspam->add_action( "frostfire_bolt" );
  ff_st_boltspam->add_action( "call_action_list,name=movement" );

  movement->add_action( "ice_floes,if=buff.ice_floes.down" );
  movement->add_action( "any_blink,if=movement.distance>5" );
  movement->add_action( "flurry" );
  movement->add_action( "frozen_orb" );
  movement->add_action( "comet_storm,if=talent.splinterstorm" );
  movement->add_action( "ice_nova" );
  movement->add_action( "ice_lance" );

  ss_aoe->add_action( "cone_of_cold,if=talent.coldest_snap&(prev_gcd.1.frozen_orb|cooldown.frozen_orb.remains>30)" );
  ss_aoe->add_action( "ice_nova,if=(freezable|talent.unerring_proficiency)&active_enemies>=5&time-action.cone_of_cold.last_used<8&time-action.cone_of_cold.last_used>7", "Cast Ice Nova against 5+ freezable targets to consume the Winter's Chill debuff applied by Cone of Cold in the very last moment of its duration." );
  ss_aoe->add_action( "freeze,if=freezable&(prev_gcd.1.glacial_spike|!talent.glacial_spike&time-action.cone_of_cold.last_used>8)", "Cast Pet Freeze whenever you cast Glacial Spike against freezable targets to shatter the second Glacial Spike." );
  ss_aoe->add_action( "flurry,if=cooldown_react&remaining_winters_chill=0&debuff.winters_chill.down&prev_gcd.1.glacial_spike" );
  ss_aoe->add_action( "flurry,if=cooldown_react&remaining_winters_chill=0&debuff.winters_chill.down&prev_gcd.1.frostbolt" );
  ss_aoe->add_action( "flurry,if=cooldown_react&buff.cold_front_ready.react", "Cast Flurry regardless of Winter's Chill to spend the Cold Front buff." );
  ss_aoe->add_action( "frozen_orb,if=cooldown_react" );
  ss_aoe->add_action( "blizzard,if=talent.ice_caller|talent.freezing_rain" );
  ss_aoe->add_action( "comet_storm,if=talent.glacial_assault|buff.icy_veins.down" );
  ss_aoe->add_action( "ray_of_frost,if=talent.splintering_ray&buff.icy_veins.down&remaining_winters_chill" );
  ss_aoe->add_action( "shifting_power,if=talent.shifting_shards" );
  ss_aoe->add_action( "ice_lance,if=buff.fingers_of_frost.react=2&talent.glacial_spike", "Not munching Fingers of Frost is more important than munching Icicles." );
  ss_aoe->add_action( "glacial_spike,if=buff.icicles.react=5&(action.flurry.cooldown_react|remaining_winters_chill)", "Cast Glacial Spike if you can shatter it into Winter's Chill or with a followup Flurry." );
  ss_aoe->add_action( "frostbolt,if=talent.deaths_chill&buff.icy_veins.up&(buff.deaths_chill.stack<6|buff.deaths_chill.stack=6&!action.frostbolt.in_flight)", "During Icy Veins stack Deaths Chill to 9 before using regular Ice Lances." );
  ss_aoe->add_action( "ice_lance,if=buff.fingers_of_frost.react" );
  ss_aoe->add_action( "ice_lance,if=remaining_winters_chill" );
  ss_aoe->add_action( "shifting_power,if=buff.icy_veins.down&cooldown.icy_veins.remains>8" );
  ss_aoe->add_action( "frostbolt" );
  ss_aoe->add_action( "call_action_list,name=movement" );

  ss_cleave->add_action( "flurry,target_if=min:debuff.winters_chill.stack,if=cooldown_react&prev_gcd.1.glacial_spike", "Flurry the off-target after Glacial Spike to make it shatter on both targets." );
  ss_cleave->add_action( "flurry,if=cooldown_react&debuff.winters_chill.down&remaining_winters_chill=0&prev_gcd.1.frostbolt" );
  ss_cleave->add_action( "flurry,if=cooldown_react&debuff.winters_chill.down&remaining_winters_chill=0&talent.shifting_shards&buff.cold_front_ready.react", "With Shifting Shards talented cast Flurry with or without precast to spend the Cold Front buff." );
  ss_cleave->add_action( "ice_lance,if=buff.fingers_of_frost.react=2&talent.glacial_spike", "Not munching Fingers of Frost is more important than munching Icicles." );
  ss_cleave->add_action( "frozen_orb,if=cooldown_react" );
  ss_cleave->add_action( "comet_storm,if=buff.icy_veins.down&remaining_winters_chill&talent.shifting_shards" );
  ss_cleave->add_action( "shifting_power,if=!equipped.arazs_ritual_forge&cooldown.flurry.charges<2&cooldown.icy_veins.remains>8|talent.shifting_shards" );
  ss_cleave->add_action( "glacial_spike,if=buff.icicles.react=5&(action.flurry.cooldown_react|remaining_winters_chill)", "Cast Glacial Spike if you can shatter it into Winter's Chill or with a followup Flurry." );
  ss_cleave->add_action( "blizzard,if=buff.freezing_rain.up&talent.ice_caller" );
  ss_cleave->add_action( "ice_lance,if=buff.fingers_of_frost.react" );
  ss_cleave->add_action( "frostbolt,if=talent.deaths_chill&buff.icy_veins.up&(buff.deaths_chill.stack<8|buff.deaths_chill.stack=8&!action.frostbolt.in_flight)", "In Icy Veins only ever cast Ice Lance with Fingers of Frost until you have at least 10 Deaths Chill stacks." );
  ss_cleave->add_action( "ice_lance,target_if=max:debuff.winters_chill.stack,if=remaining_winters_chill" );
  ss_cleave->add_action( "shifting_power,if=equipped.arazs_ritual_forge&buff.icy_veins.down&cooldown.flurry.charges<2&cooldown.icy_veins.remains>8" );
  ss_cleave->add_action( "frostbolt" );
  ss_cleave->add_action( "call_action_list,name=movement" );

  ss_st->add_action( "flurry,if=cooldown_react&debuff.winters_chill.down&remaining_winters_chill=0&prev_gcd.1.glacial_spike" );
  ss_st->add_action( "flurry,if=cooldown_react&debuff.winters_chill.down&remaining_winters_chill=0&(buff.icicles.react<5|!talent.glacial_spike)&prev_gcd.1.frostbolt" );
  ss_st->add_action( "frozen_orb,if=cooldown_react" );
  ss_st->add_action( "comet_storm,if=buff.icy_veins.down&remaining_winters_chill&talent.shifting_shards" );
  ss_st->add_action( "ray_of_frost,if=buff.icy_veins.down&remaining_winters_chill=1" );
  ss_st->add_action( "shifting_power,if=!equipped.arazs_ritual_forge&cooldown.flurry.charges<2&cooldown.icy_veins.remains>8|talent.shifting_shards" );
  ss_st->add_action( "glacial_spike,if=buff.icicles.react=5&(action.flurry.cooldown_react|remaining_winters_chill)", "Cast Glacial Spike if you can shatter it into Winter's Chill or with a followup Flurry." );
  ss_st->add_action( "blizzard,if=buff.icy_veins.down&buff.freezing_rain.up&talent.ice_caller" );
  ss_st->add_action( "ice_lance,if=buff.fingers_of_frost.react" );
  ss_st->add_action( "ice_lance,if=remaining_winters_chill" );
  ss_st->add_action( "shifting_power,if=equipped.arazs_ritual_forge&buff.icy_veins.down&cooldown.flurry.charges<2&cooldown.icy_veins.remains>8" );
  ss_st->add_action( "frostbolt" );
  ss_st->add_action( "call_action_list,name=movement" );
}
//frost_apl_end

}  // namespace mage_apl
