#include "class_modules/apl/apl_warrior.hpp"

#include "simulationcraft.hpp"

namespace warrior_apl
{
//fury_apl_start
void fury( player_t* p )
{
  std::vector<std::string> racial_actions = p->get_racial_actions();

  action_priority_list_t* default_list = p->get_action_priority_list( "default" );
  // action_priority_list_t* movement      = get_action_priority_list( "movement" );
  action_priority_list_t* multi_target  = p->get_action_priority_list( "multi_target" );
  action_priority_list_t* single_target = p->get_action_priority_list( "single_target" );
  action_priority_list_t* precombat     = p->get_action_priority_list( "precombat" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );

  precombat->add_action( "berserker_stance,toggle=on" );
  precombat->add_action( "avatar,if=!talent.titans_torment" );
  precombat->add_action( "recklessness,if=!talent.reckless_abandon" );

  default_list->add_action( "auto_attack" );
  default_list->add_action( "charge,if=time<=0.5|movement.distance>5" );

  // default_list->add_action( "run_action_list,name=movement,if=movement.distance>5",
  //"This is mostly to prevent cooldowns from being accidentally used during movement." );
  default_list->add_action( "heroic_leap,if=(raid_event.movement.distance>25&raid_event.movement.in>45)" );

  if ( p->sim->allow_potions && p->true_level >= 40 )
  {
    default_list->add_action( "potion" );
  }

  default_list->add_action( "pummel,if=target.debuff.casting.react" );

  //   default_list->add_action( this, covenant.conquerors_banner, "conquerors_banner" );

  default_list->add_action( "use_item,name=algethar_puzzle_box,if=cooldown.recklessness.remains<3|(talent.anger_management&cooldown.avatar.remains<3)" );
  default_list->add_action( "use_item,name=vial_of_animated_blood,if=buff.avatar.up" );
  default_list->add_action( "use_item,name=elementium_pocket_anvil,use_off_gcd=1,if=gcd.remains>0.7" );
  default_list->add_action( "use_item,name=beacon_to_the_beyond,use_off_gcd=1,if=gcd.remains>0.7" );
  default_list->add_action( "use_item,name=irideus_fragment,if=buff.avatar.up" );
  default_list->add_action( "use_item,name=manic_grieftorch,if=buff.avatar.down" );
  default_list->add_action( "use_item,name=gladiators_badge,if=cooldown.recklessness.remains>10&(buff.recklessness.up|target.time_to_die<11|target.time_to_die>65)" );
  default_list->add_action( "use_items");

  default_list->add_action( "ravager,if=cooldown.recklessness.remains<3|buff.recklessness.up" );

  // default_list->add_action( "arcane_torrent" ); // While it's on the GCD, arcane torrent wasting a global is a dps decrease.
  default_list->add_action( "lights_judgment,if=buff.recklessness.down" );
  // default_list->add_action( "bag_of_tricks" ); // currently better to ignore entirely due to eating a GCD
  default_list->add_action( "berserking,if=buff.recklessness.up" );
  default_list->add_action( "blood_fury" );
  default_list->add_action( "fireblood" );
  default_list->add_action( "ancestral_call" );

  default_list->add_action( "avatar,if=talent.titans_torment&buff.enrage.up&raid_event.adds.in>15|talent.berserkers_torment&buff.enrage.up&!buff.avatar.up&raid_event.adds.in>15|!talent.titans_torment&!talent.berserkers_torment&(buff.recklessness.up|target.time_to_die<20)" );
  default_list->add_action( "recklessness,if=!raid_event.adds.exists&(talent.annihilator&cooldown.avatar.remains<1|cooldown.avatar.remains>40|!talent.avatar|target.time_to_die<12)" );
  default_list->add_action( "recklessness,if=!raid_event.adds.exists&!talent.annihilator|target.time_to_die<12" );
  default_list->add_action( "spear_of_bastion,if=buff.enrage.up&(buff.recklessness.up|buff.avatar.up|target.time_to_die<20|active_enemies>1)&raid_event.adds.in>15" );

  // default_list->add_action(
  // "whirlwind,if=spell_targets.whirlwind>1&talent.improved_whirlwind&!buff.meat_cleaver.up|raid_event.adds.in<2&talent.improved_whirlwind&!buff.meat_cleaver.up"
  // );

  default_list->add_action( "call_action_list,name=multi_target,if=raid_event.adds.exists|active_enemies>2" );
  default_list->add_action( "call_action_list,name=single_target,if=!raid_event.adds.exists" );

  // movement->add_action( this, "Heroic Leap" );

  multi_target->add_action( "recklessness,if=raid_event.adds.in>15|active_enemies>1|target.time_to_die<12" );
  multi_target->add_action( "odyns_fury,if=active_enemies>1&talent.titanic_rage&(!buff.meat_cleaver.up|buff.avatar.up|buff.recklessness.up)" );
  multi_target->add_action( "whirlwind,if=spell_targets.whirlwind>1&talent.improved_whirlwind&!buff.meat_cleaver.up|raid_event.adds.in<2&talent.improved_whirlwind&!buff.meat_cleaver.up" );
  multi_target->add_action( "execute,if=buff.ashen_juggernaut.up&buff.ashen_juggernaut.remains<gcd" );
  multi_target->add_action( "thunderous_roar,if=buff.enrage.up&(spell_targets.whirlwind>1|raid_event.adds.in>15)" );
  multi_target->add_action( "odyns_fury,if=active_enemies>1&buff.enrage.up&raid_event.adds.in>15" );
  multi_target->add_action( "bloodbath,if=set_bonus.tier30_4pc&action.bloodthirst.crit_pct_current>=95" );
  multi_target->add_action( "bloodthirst,if=set_bonus.tier30_4pc&action.bloodthirst.crit_pct_current>=95" );
  multi_target->add_action( "crushing_blow,if=talent.wrath_and_fury&buff.enrage.up" );
  multi_target->add_action( "execute,if=buff.enrage.up" );
  multi_target->add_action( "odyns_fury,if=buff.enrage.up&raid_event.adds.in>15" );
  multi_target->add_action( "rampage,if=buff.recklessness.up|buff.enrage.remains<gcd|(rage>110&talent.overwhelming_rage)|(rage>80&!talent.overwhelming_rage)" );
  multi_target->add_action( "execute" );
  multi_target->add_action( "bloodbath,if=buff.enrage.up&talent.reckless_abandon&!talent.wrath_and_fury" );
  multi_target->add_action( "bloodthirst,if=buff.enrage.down|(talent.annihilator&!buff.recklessness.up)" );
  multi_target->add_action( "onslaught,if=!talent.annihilator&buff.enrage.up|talent.tenderize" );
  multi_target->add_action( "raging_blow,if=charges>1&talent.wrath_and_fury" );
  multi_target->add_action( "crushing_blow,if=charges>1&talent.wrath_and_fury" );
  multi_target->add_action( "bloodbath,if=buff.enrage.down|!talent.wrath_and_fury" );
  multi_target->add_action( "crushing_blow,if=buff.enrage.up&talent.reckless_abandon" );
  multi_target->add_action( "bloodthirst,if=!talent.wrath_and_fury" );
  multi_target->add_action( "raging_blow,if=charges>=1" );
  multi_target->add_action( "rampage" );
  multi_target->add_action( "slam,if=talent.annihilator" );
  multi_target->add_action( "bloodbath" );
  multi_target->add_action( "raging_blow" );
  multi_target->add_action( "crushing_blow" );
  multi_target->add_action( "whirlwind" );

  single_target->add_action( "whirlwind,if=spell_targets.whirlwind>1&talent.improved_whirlwind&!buff.meat_cleaver.up|raid_event.adds.in<2&talent.improved_whirlwind&!buff.meat_cleaver.up" );
  single_target->add_action( "execute,if=buff.ashen_juggernaut.up&buff.ashen_juggernaut.remains<gcd" );
  single_target->add_action( "thunderous_roar,if=buff.enrage.up&(spell_targets.whirlwind>1|raid_event.adds.in>15)" );
  single_target->add_action( "odyns_fury,if=buff.enrage.up&(spell_targets.whirlwind>1|raid_event.adds.in>15)&(talent.dancing_blades&buff.dancing_blades.remains<5|!talent.dancing_blades)" );
  single_target->add_action( "rampage,if=talent.anger_management&(buff.recklessness.up|buff.enrage.remains<gcd|rage.pct>85)" );
  single_target->add_action( "bloodbath,if=set_bonus.tier30_4pc&action.bloodthirst.crit_pct_current>=95" );
  single_target->add_action( "bloodthirst,if=set_bonus.tier30_4pc&action.bloodthirst.crit_pct_current>=95" );
  single_target->add_action( "execute,if=buff.enrage.up" );
  single_target->add_action( "onslaught,if=buff.enrage.up|talent.tenderize" );
  single_target->add_action( "crushing_blow,if=talent.wrath_and_fury&buff.enrage.up" );
  single_target->add_action( "rampage,if=talent.reckless_abandon&(buff.recklessness.up|buff.enrage.remains<gcd|rage.pct>85)" );
  single_target->add_action( "rampage,if=talent.anger_management" );
  single_target->add_action( "execute" );
  single_target->add_action( "bloodbath,if=buff.enrage.up&talent.reckless_abandon&!talent.wrath_and_fury" );
  single_target->add_action( "bloodthirst,if=buff.enrage.down|(talent.annihilator&!buff.recklessness.up)" );
  single_target->add_action( "raging_blow,if=charges>1&talent.wrath_and_fury" );
  single_target->add_action( "crushing_blow,if=charges>1&talent.wrath_and_fury" );
  single_target->add_action( "bloodbath,if=buff.enrage.down|!talent.wrath_and_fury" );
  single_target->add_action( "crushing_blow,if=buff.enrage.up&talent.reckless_abandon" );
  single_target->add_action( "bloodthirst,if=!talent.wrath_and_fury" );
  single_target->add_action( "raging_blow,if=charges>1" );
  single_target->add_action( "rampage" );
  single_target->add_action( "slam,if=talent.annihilator" );
  single_target->add_action( "bloodbath" );
  single_target->add_action( "raging_blow" );
  single_target->add_action( "crushing_blow" );
  single_target->add_action( "bloodthirst" );
  single_target->add_action( "whirlwind" );
  single_target->add_action( "wrecking_throw" );
  single_target->add_action( "storm_bolt" );
}
//fury_apl_end

//arms_apl_start
void arms( player_t* p )
{
  std::vector<std::string> racial_actions = p->get_racial_actions();

  action_priority_list_t* default_list = p->get_action_priority_list( "default" );
  action_priority_list_t* hac          = p->get_action_priority_list( "hac" );
  // action_priority_list_t* five_target   = get_action_priority_list( "five_target" );
  action_priority_list_t* execute       = p->get_action_priority_list( "execute" );
  action_priority_list_t* single_target = p->get_action_priority_list( "single_target" );
  action_priority_list_t* precombat     = p->get_action_priority_list( "precombat" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );
  precombat->add_action( "battle_stance,toggle=on" );

  default_list->add_action( "charge,if=time<=0.5|movement.distance>5" );
  default_list->add_action( "auto_attack" );

  if ( p->sim->allow_potions && p->true_level >= 40 )
  {
    default_list->add_action( "potion,if=gcd.remains=0&debuff.colossus_smash.remains>8|target.time_to_die<25" );
  }

  default_list->add_action( "pummel,if=target.debuff.casting.react" );


  default_list->add_action( "use_item,name=algethar_puzzle_box,if=cooldown.avatar.remains<3" );
  default_list->add_action( "use_item,name=vial_of_animated_blood,if=buff.avatar.up" );
  default_list->add_action( "use_item,name=elementium_pocket_anvil,use_off_gcd=1,if=gcd.remains>0.7" );
  default_list->add_action( "use_item,name=beacon_to_the_beyond,use_off_gcd=1,if=gcd.remains>0.7" );
  default_list->add_action( "use_item,name=irideus_fragment,if=buff.avatar.up" );
  default_list->add_action( "use_item,name=manic_grieftorch,if=!buff.avatar.up&!debuff.colossus_smash.up" );
  default_list->add_action( "use_item,name=gladiators_badge,if=gcd.remains=0&debuff.colossus_smash.remains>8|target.time_to_die<25" );
  default_list->add_action( "use_items");
  
  default_list->add_action( "arcane_torrent,if=cooldown.mortal_strike.remains>1.5&rage<50" );
  default_list->add_action( "lights_judgment,if=debuff.colossus_smash.down&cooldown.mortal_strike.remains" );
  default_list->add_action( "bag_of_tricks,if=debuff.colossus_smash.down&cooldown.mortal_strike.remains" );
  default_list->add_action( "berserking,if=debuff.colossus_smash.remains>6" );
  default_list->add_action( "blood_fury,if=debuff.colossus_smash.up" );
  default_list->add_action( "fireblood,if=debuff.colossus_smash.up" );
  default_list->add_action( "ancestral_call,if=debuff.colossus_smash.up" );

  // default_list->add_action( "sweeping_strikes,if=spell_targets.whirlwind>1&(cooldown.bladestorm.remains>15)" );

  default_list->add_action( "run_action_list,name=hac,if=raid_event.adds.exists|active_enemies>2" );
  default_list->add_action( "call_action_list,name=execute,target_if=min:target.health.pct,if=(talent.massacre.enabled&target.health.pct<35)|target.health.pct<20" );
  default_list->add_action( "run_action_list,name=single_target,if=!raid_event.adds.exists" );

  execute->add_action( "sweeping_strikes,if=spell_targets.whirlwind>1" );
  execute->add_action( "rend,if=remains<=gcd&(!talent.warbreaker&cooldown.colossus_smash.remains<4|talent.warbreaker&cooldown.warbreaker.remains<4)&target.time_to_die>12" );
  execute->add_action( "avatar,if=cooldown.colossus_smash.ready|debuff.colossus_smash.up|target.time_to_die<20" );
  execute->add_action( "warbreaker" );
  execute->add_action( "colossus_smash" );
  execute->add_action( "thunderous_roar,if=buff.test_of_might.up|!talent.test_of_might&debuff.colossus_smash.up" );
  execute->add_action( "spear_of_bastion,if=debuff.colossus_smash.up|buff.test_of_might.up" );
  execute->add_action( "skullsplitter,if=rage<40" );
  execute->add_action( "cleave,if=spell_targets.whirlwind>2&dot.deep_wounds.remains<gcd" );
  execute->add_action( "overpower,if=rage<40&buff.martial_prowess.stack<2" );
  execute->add_action( "mortal_strike,if=debuff.executioners_precision.stack=2|dot.deep_wounds.remains<=gcd" );
  execute->add_action( "execute" );
  execute->add_action( "shockwave,if=talent.sonic_boom" );
  execute->add_action( "overpower" );
  execute->add_action( "bladestorm" );

  hac->add_action( "execute,if=buff.juggernaut.up&buff.juggernaut.remains<gcd" );
  hac->add_action( "thunder_clap,if=active_enemies>2&talent.thunder_clap&talent.blood_and_thunder&talent.rend&dot.rend.remains<=dot.rend.duration*0.3" );
  hac->add_action( "sweeping_strikes,if=active_enemies>=2&(cooldown.bladestorm.remains>15|!talent.bladestorm)" );
  hac->add_action( "rend,if=active_enemies=1&remains<=gcd&(target.health.pct>20|talent.massacre&target.health.pct>35)|talent.tide_of_blood&cooldown.skullsplitter.remains<=gcd&(cooldown.colossus_smash.remains<=gcd|debuff.colossus_smash.up)&dot.rend.remains<dot.rend.duration*0.85" );
  hac->add_action( "avatar,if=raid_event.adds.in>15|talent.blademasters_torment&active_enemies>1|target.time_to_die<20" );
  hac->add_action( "warbreaker,if=raid_event.adds.in>22|active_enemies>1" );
  hac->add_action( "colossus_smash,cycle_targets=1,if=(target.health.pct<20|talent.massacre&target.health.pct<35)" );
  hac->add_action( "colossus_smash" );
  hac->add_action( "thunderous_roar,if=(buff.test_of_might.up|!talent.test_of_might&debuff.colossus_smash.up)&raid_event.adds.in>15|active_enemies>1&dot.deep_wounds.remains" );
  hac->add_action( "spear_of_bastion,if=(buff.test_of_might.up|!talent.test_of_might&debuff.colossus_smash.up)&raid_event.adds.in>15" );
  hac->add_action( "bladestorm,if=talent.unhinged&(buff.test_of_might.up|!talent.test_of_might&debuff.colossus_smash.up)" );
  hac->add_action( "bladestorm,if=active_enemies>1&(buff.test_of_might.up|!talent.test_of_might&debuff.colossus_smash.up)&raid_event.adds.in>30|active_enemies>1&dot.deep_wounds.remains" );
  // hac->add_action( "execute,if=active_enemies<=2&buff.hurricane.up" );
  hac->add_action( "cleave,if=active_enemies>2|!talent.battlelord&buff.merciless_bonegrinder.up&cooldown.mortal_strike.remains>gcd" );
  hac->add_action( "whirlwind,if=active_enemies>2|talent.storm_of_swords&(buff.merciless_bonegrinder.up|buff.hurricane.up)" );
  hac->add_action( "skullsplitter,if=rage<40|talent.tide_of_blood&dot.rend.remains&(buff.sweeping_strikes.up&active_enemies>=2|debuff.colossus_smash.up|buff.test_of_might.up)" );
  hac->add_action( "mortal_strike,if=buff.sweeping_strikes.up&buff.crushing_advance.stack=3,if=set_bonus.tier30_4pc" );
  hac->add_action( "overpower,if=buff.sweeping_strikes.up&talent.dreadnaught" );
  hac->add_action( "mortal_strike,cycle_targets=1,if=debuff.executioners_precision.stack=2|dot.deep_wounds.remains<=gcd|talent.dreadnaught&talent.battlelord&active_enemies<=2" );
  hac->add_action( "execute,cycle_targets=1,if=buff.sudden_death.react|active_enemies<=2&(target.health.pct<20|talent.massacre&target.health.pct<35)|buff.sweeping_strikes.up" );
  hac->add_action( "thunderous_roar,if=raid_event.adds.in>15" );
  hac->add_action( "shockwave,if=active_enemies>2&talent.sonic_boom" );
  hac->add_action( "overpower,if=active_enemies=1&(charges=2&!talent.battlelord&(debuff.colossus_smash.down|rage.pct<25)|talent.battlelord)" );
  hac->add_action( "slam,if=active_enemies=1&!talent.battlelord&rage.pct>70" );
  hac->add_action( "overpower,if=charges=2&(!talent.test_of_might|talent.test_of_might&debuff.colossus_smash.down|talent.battlelord)|rage<70" );
  hac->add_action( "thunder_clap,if=active_enemies>2" );
  hac->add_action( "mortal_strike" );
  hac->add_action( "rend,if=active_enemies=1&dot.rend.remains<duration*0.3" );
  hac->add_action( "whirlwind,if=talent.storm_of_swords|talent.fervor_of_battle&active_enemies>1" );
  hac->add_action( "cleave,if=!talent.crushing_force" );
  hac->add_action( "ignore_pain,if=talent.battlelord&talent.anger_management&rage>30&(target.health.pct>20|talent.massacre&target.health.pct>35)" );
  hac->add_action( "slam,if=talent.crushing_force&rage>30&(talent.fervor_of_battle&active_enemies=1|!talent.fervor_of_battle)" );
  hac->add_action( "shockwave,if=talent.sonic_boom" );
  hac->add_action( "bladestorm,if=raid_event.adds.in>30" );
  hac->add_action( "wrecking_throw" );

  single_target->add_action( "sweeping_strikes,if=spell_targets.whirlwind>1" );
  single_target->add_action( "mortal_strike" );
  single_target->add_action( "rend,if=remains<=gcd|talent.tide_of_blood&cooldown.skullsplitter.remains<=gcd&(cooldown.colossus_smash.remains<=gcd|debuff.colossus_smash.up)&dot.rend.remains<dot.rend.duration*0.85" );
  single_target->add_action( "avatar,if=talent.warlords_torment&rage.pct<33&(cooldown.colossus_smash.ready|debuff.colossus_smash.up|buff.test_of_might.up)|!talent.warlords_torment&(cooldown.colossus_smash.ready|debuff.colossus_smash.up)" );
  single_target->add_action( "spear_of_bastion,if=cooldown.colossus_smash.remains<=gcd|cooldown.warbreaker.remains<=gcd" );
  single_target->add_action( "warbreaker" );
  single_target->add_action( "colossus_smash" );
  single_target->add_action( "thunderous_roar,if=buff.test_of_might.up|talent.test_of_might&debuff.colossus_smash.up&rage.pct<33|!talent.test_of_might&debuff.colossus_smash.up" );
  // single_target->add_action(
  // "spear_of_bastion,if=buff.test_of_might.up|!talent.test_of_might&debuff.colossus_smash.up" );
  single_target->add_action( "bladestorm,if=talent.hurricane&(buff.test_of_might.up|!talent.test_of_might&debuff.colossus_smash.up)|talent.unhinged&(buff.test_of_might.up|!talent.test_of_might&debuff.colossus_smash.up)" );
  single_target->add_action( "skullsplitter,if=talent.tide_of_blood&dot.rend.remains&(debuff.colossus_smash.up|cooldown.colossus_smash.remains>gcd*4&buff.test_of_might.up|!talent.test_of_might&cooldown.colossus_smash.remains>gcd*4)|rage<30" );
  single_target->add_action( "execute,if=buff.sudden_death.react" );
  single_target->add_action( "shockwave,if=talent.sonic_boom.enabled" );
  single_target->add_action( "ignore_pain,if=talent.anger_management|talent.test_of_might&debuff.colossus_smash.up" );
  single_target->add_action( "whirlwind,if=talent.storm_of_swords&talent.battlelord&rage.pct>80&debuff.colossus_smash.up" );
  single_target->add_action( "overpower,if=charges=2&!talent.battlelord&(debuff.colossus_smash.down|rage.pct<25)|talent.battlelord" );
  single_target->add_action( "whirlwind,if=talent.storm_of_swords|talent.fervor_of_battle&active_enemies>1" );
  single_target->add_action( "thunder_clap,if=talent.battlelord&talent.blood_and_thunder" );
  single_target->add_action( "overpower,if=debuff.colossus_smash.down&rage.pct<50&!talent.battlelord|rage.pct<25" );
  single_target->add_action( "whirlwind,if=buff.merciless_bonegrinder.up" );
  single_target->add_action( "cleave,if=set_bonus.tier29_2pc&!talent.crushing_force" );
  single_target->add_action( "slam,if=rage>30&(!talent.fervor_of_battle|talent.fervor_of_battle&active_enemies=1)" );
  single_target->add_action( "bladestorm" );
  single_target->add_action( "cleave" );
  single_target->add_action( "wrecking_throw" );
  single_target->add_action( "rend,if=remains<duration*0.3" );
}
//arms_apl_end

//protection_apl_start
void protection( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* generic = p->get_action_priority_list( "generic" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "battle_stance,toggle=on" );

  default_->add_action( "auto_attack" );
  default_->add_action( "charge,if=time=0" );
  default_->add_action( "use_items" );
  default_->add_action( "avatar" );
  default_->add_action( "shield_wall,if=talent.immovable_object.enabled&buff.avatar.down" );
  default_->add_action( "blood_fury" );
  default_->add_action( "berserking" );
  default_->add_action( "arcane_torrent" );
  default_->add_action( "lights_judgment" );
  default_->add_action( "fireblood" );
  default_->add_action( "ancestral_call" );
  default_->add_action( "bag_of_tricks" );
  default_->add_action( "potion,if=buff.avatar.up|buff.avatar.up&target.health.pct<=20" );
  default_->add_action( "ignore_pain,if=target.health.pct>=20&(rage.deficit<=15&cooldown.shield_slam.ready|rage.deficit<=40&cooldown.shield_charge.ready&talent.champions_bulwark.enabled|rage.deficit<=20&cooldown.shield_charge.ready|rage.deficit<=30&cooldown.demoralizing_shout.ready&talent.booming_voice.enabled|rage.deficit<=20&cooldown.avatar.ready|rage.deficit<=45&cooldown.demoralizing_shout.ready&talent.booming_voice.enabled&buff.last_stand.up&talent.unnerving_focus.enabled|rage.deficit<=30&cooldown.avatar.ready&buff.last_stand.up&talent.unnerving_focus.enabled|rage.deficit<=20|rage.deficit<=40&cooldown.shield_slam.ready&buff.violent_outburst.up&talent.heavy_repercussions.enabled&talent.impenetrable_wall.enabled|rage.deficit<=55&cooldown.shield_slam.ready&buff.violent_outburst.up&buff.last_stand.up&talent.unnerving_focus.enabled&talent.heavy_repercussions.enabled&talent.impenetrable_wall.enabled|rage.deficit<=17&cooldown.shield_slam.ready&talent.heavy_repercussions.enabled|rage.deficit<=18&cooldown.shield_slam.ready&talent.impenetrable_wall.enabled),use_off_gcd=1" );
  default_->add_action( "last_stand,if=(target.health.pct>=90&talent.unnerving_focus.enabled|target.health.pct<=20&talent.unnerving_focus.enabled)|talent.bolster.enabled|set_bonus.tier30_2pc|set_bonus.tier30_4pc" );
  default_->add_action( "ravager" );
  default_->add_action( "demoralizing_shout,if=talent.booming_voice.enabled" );
  default_->add_action( "spear_of_bastion" );
  default_->add_action( "thunderous_roar" );
  default_->add_action( "shockwave,if=talent.sonic_boom.enabled&buff.avatar.up&talent.unstoppable_force.enabled&!talent.rumbling_earth.enabled" );
  default_->add_action( "shield_charge" );
  default_->add_action( "shield_block,if=buff.shield_block.duration<=18&talent.enduring_defenses.enabled|buff.shield_block.duration<=12" );
  default_->add_action( "run_action_list,name=aoe,if=spell_targets.thunder_clap>=3" );
  default_->add_action( "call_action_list,name=generic" );

  aoe->add_action( "thunder_clap,if=dot.rend.remains<=1" );
  aoe->add_action( "shield_slam,if=(set_bonus.tier30_2pc|set_bonus.tier30_4pc)&spell_targets.thunder_clap<=7|buff.earthen_tenacity.up" );
  aoe->add_action( "thunder_clap,if=buff.violent_outburst.up&spell_targets.thunderclap>5&buff.avatar.up&talent.unstoppable_force.enabled" );
  aoe->add_action( "revenge,if=rage>=70&talent.seismic_reverberation.enabled&spell_targets.revenge>=3" );
  aoe->add_action( "shield_slam,if=rage<=60|buff.violent_outburst.up&spell_targets.thunderclap<=7" );
  aoe->add_action( "thunder_clap" );
  aoe->add_action( "revenge,if=rage>=30|rage>=40&talent.barbaric_training.enabled" );

  generic->add_action( "shield_slam" );
  generic->add_action( "thunder_clap,if=dot.rend.remains<=1&buff.violent_outburst.down" );
  generic->add_action( "execute,if=buff.sudden_death.up&talent.sudden_death.enabled" );
  generic->add_action( "execute,if=spell_targets.revenge=1&rage>=50" );
  generic->add_action( "thunder_clap,if=(spell_targets.thunder_clap>1|cooldown.shield_slam.remains&!buff.violent_outburst.up)" );
  generic->add_action( "revenge,if=(rage>=60&target.health.pct>20|buff.revenge.up&target.health.pct<=20&rage<=18&cooldown.shield_slam.remains|buff.revenge.up&target.health.pct>20)|(rage>=60&target.health.pct>35|buff.revenge.up&target.health.pct<=35&rage<=18&cooldown.shield_slam.remains|buff.revenge.up&target.health.pct>35)&talent.massacre.enabled" );
  generic->add_action( "execute,if=spell_targets.revenge=1" );
  generic->add_action( "revenge" );
  generic->add_action( "thunder_clap,if=(spell_targets.thunder_clap>=1|cooldown.shield_slam.remains&buff.violent_outburst.up)" );
  generic->add_action( "devastate" );
}
//protection_apl_end
}  // namespace warrior_apl
