#include "class_modules/apl/apl_death_knight.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"
#include "dbc/dbc.hpp"
#include "sim/sim.hpp"

namespace death_knight_apl
{

std::string potion( const player_t* p )
{
  std::string frost_potion = ( p->true_level >= 81 ) ? "potion_of_recklessness_2" : "tempered_potion_3";

  std::string unholy_potion = ( p->true_level >= 81 ) ? "potion_of_recklessness_2" : "tempered_potion_3";

  std::string blood_potion = ( p->true_level >= 81 ) ? "draught_of_rampant_abandon_2" : "tempered_potion_3";

  switch ( p->specialization() )
  {
    case DEATH_KNIGHT_BLOOD:
      return blood_potion;
    case DEATH_KNIGHT_FROST:
      return frost_potion;
    default:
      return unholy_potion;
  }
}

std::string flask( const player_t* p )
{
  std::string frost_flask = ( p->true_level >= 81 ) ? "flask_of_the_shattered_sun_2" : "flask_of_alchemical_chaos_3";

  std::string unholy_flask = ( p->true_level >= 81 ) ? "flask_of_the_magisters_2" : "flask_of_alchemical_chaos_3";

  std::string blood_flask = ( p->true_level >= 81 ) ? "flask_of_the_shattered_sun_2" : "flask_of_alchemical_chaos_3";

  switch ( p->specialization() )
  {
    case DEATH_KNIGHT_BLOOD:
      return blood_flask;
    case DEATH_KNIGHT_FROST:
      return frost_flask;
    default:
      return unholy_flask;
  }
}

std::string food( const player_t* p )
{
  std::string frost_food;
  std::string unholy_food;
  std::string blood_food;

  if ( p->true_level >= 81 )
  {
    frost_food = "silvermoon_parade";
    unholy_food = "silvermoon_parade";
    blood_food = "silvermoon_parade";
  }
  else
  {
    frost_food = "feast_of_the_divine_day";
    unholy_food = "chippy_tea";
    blood_food = "beledars_bounty";
  }

  switch ( p->specialization() )
  {
    case DEATH_KNIGHT_BLOOD:
      return blood_food;
    case DEATH_KNIGHT_FROST:
      return frost_food;

    default:
      return unholy_food;
  }
}

std::string rune( const player_t* p )
{
  return ( p->true_level >= 81 ) ? "void_touched" : "crystallized";
}

std::string temporary_enchant( const player_t* p )
{
  std::string frost_temporary_enchant = ( p->true_level >= 81 )
                                            ? "main_hand:thalassian_phoenix_oil_2/off_hand:thalassian_phoenix_oil_2"
                                            : "main_hand:algari_mana_oil_3/off_hand:algari_mana_oil_3";

  std::string unholy_temporary_enchant =
      ( p->true_level >= 81 ) ? "main_hand:thalassian_phoenix_oil_2" : "main_hand:algari_mana_oil_3";

  std::string blood_temporary_enchant =
      ( p->true_level >= 81 ) ? "main_hand:thalassian_phoenix_oil_2" : "main_hand:ironclaw_whetstone_3";

  switch ( p->specialization() )
  {
    case DEATH_KNIGHT_BLOOD:
      return blood_temporary_enchant;
    case DEATH_KNIGHT_FROST:
      return frost_temporary_enchant;

    default:
      return unholy_temporary_enchant;
  }
}

//blood_apl_start
void blood( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* high_prio_actions = p->get_action_priority_list( "high_prio_actions" );
  action_priority_list_t* deathbringer = p->get_action_priority_list( "deathbringer" );
  action_priority_list_t* san_gift = p->get_action_priority_list( "san_gift" );
  action_priority_list_t* sanlayn = p->get_action_priority_list( "sanlayn" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "deaths_caress" );

  default_->add_action( "auto_attack" );
  default_->add_action( "use_items" );
  default_->add_action( "use_item,name=light_company_guidon,use_off_gcd=1,if=cooldown.dancing_rune_weapon.remains>78|fight_remains<15" );
  default_->add_action( "use_item,name=algethar_puzzle_box,if=fight_remains>122|cooldown.dancing_rune_weapon.remains>78|fight_remains<25" );
  default_->add_action( "fireblood,if=fight_remains>120|cooldown.dancing_rune_weapon.remains>78|fight_remains<8" );
  default_->add_action( "blood_fury,if=fight_remains>120|cooldown.dancing_rune_weapon.remains>78|fight_remains<12" );
  default_->add_action( "berserking,if=cooldown.dancing_rune_weapon.remains>78|fight_remains<=15" );
  default_->add_action( "ancestral_call,if=fight_remains>120|cooldown.dancing_rune_weapon.remains>78|fight_remains<15" );
  default_->add_action( "potion,if=cooldown.dancing_rune_weapon.remains>78|fight_remains<=30" );
  default_->add_action( "vampiric_blood,if=!buff.vampiric_blood.up" );
  default_->add_action( "call_action_list,name=high_prio_actions" );
  default_->add_action( "run_action_list,name=deathbringer,if=hero_tree.deathbringer" );
  default_->add_action( "run_action_list,name=san_gift,if=hero_tree.sanlayn&buff.gift_of_the_sanlayn.up" );
  default_->add_action( "run_action_list,name=sanlayn,if=hero_tree.sanlayn" );

  high_prio_actions->add_action( "raise_dead,use_off_gcd=1" );
  high_prio_actions->add_action( "death_strike,if=buff.coagulopathy.up&buff.coagulopathy.remains<=gcd" );
  high_prio_actions->add_action( "dancing_rune_weapon,if=!buff.exterminate.up&!debuff.reapers_mark_debuff.up&!buff.dancing_rune_weapon.up&(fight_remains>95|fight_remains<25|time>300)" );

  deathbringer->add_action( "death_strike,if=(runic_power.deficit<20|(runic_power.deficit<26&buff.dancing_rune_weapon.up))" );
  deathbringer->add_action( "death_and_decay,if=!buff.death_and_decay.up" );
  deathbringer->add_action( "reapers_mark" );
  deathbringer->add_action( "marrowrend,if=buff.exterminate.up" );
  deathbringer->add_action( "deaths_caress,if=(!buff.bone_shield.up|buff.bone_shield.remains<3|buff.bone_shield.stack<6)&rune<4" );
  deathbringer->add_action( "marrowrend,if=!buff.bone_shield.up|buff.bone_shield.remains<3|buff.bone_shield.stack<6" );
  deathbringer->add_action( "death_strike" );
  deathbringer->add_action( "blood_boil" );
  deathbringer->add_action( "consumption,empower_to=1,if=!buff.dancing_rune_weapon.up" );
  deathbringer->add_action( "heart_strike" );
  deathbringer->add_action( "consumption,empower_to=1" );
  deathbringer->add_action( "arcane_torrent,if=runic_power.deficit>20" );

  san_gift->add_action( "heart_strike,if=buff.essence_of_the_blood_queen.remains<1.5&buff.essence_of_the_blood_queen.remains" );
  san_gift->add_action( "death_strike,if=runic_power.deficit<36" );
  san_gift->add_action( "blood_boil,if=!drw.bp_ticking" );
  san_gift->add_action( "any_dnd,if=buff.crimson_scourge.remains" );
  san_gift->add_action( "heart_strike,if=buff.essence_of_the_blood_queen.stack<7" );
  san_gift->add_action( "death_strike" );
  san_gift->add_action( "blood_boil,if=buff.boiling_point.up&!buff.boiling_point_echo.up" );
  san_gift->add_action( "heart_strike" );
  san_gift->add_action( "blood_boil" );

  sanlayn->add_action( "deaths_caress,if=!buff.bone_shield.up|buff.bone_shield.remains<1.5|buff.bone_shield.stack<=1" );
  sanlayn->add_action( "blood_boil,if=dot.blood_plague.remains<3" );
  sanlayn->add_action( "heart_strike,if=(buff.essence_of_the_blood_queen.remains<1.5&buff.essence_of_the_blood_queen.remains&buff.vampiric_strike.remains)" );
  sanlayn->add_action( "death_strike,if=runic_power.deficit<20" );
  sanlayn->add_action( "deaths_caress,if=buff.bone_shield.stack<6" );
  sanlayn->add_action( "marrowrend,if=buff.bone_shield.stack<6" );
  sanlayn->add_action( "any_dnd,if=buff.crimson_scourge.remains" );
  sanlayn->add_action( "heart_strike,if=buff.vampiric_strike.up" );
  sanlayn->add_action( "death_strike" );
  sanlayn->add_action( "blood_boil,if=buff.boiling_point.up&!buff.boiling_point_echo.up" );
  sanlayn->add_action( "consumption,empower_to=1" );
  sanlayn->add_action( "heart_strike,if=rune>=2" );
  sanlayn->add_action( "blood_boil" );
  sanlayn->add_action( "heart_strike" );
}
//blood_apl_end

//frost_apl_start
void frost( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* high_prio_actions = p->get_action_priority_list( "high_prio_actions" );
  action_priority_list_t* racials = p->get_action_priority_list( "racials" );
  action_priority_list_t* single_target = p->get_action_priority_list( "single_target" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=trinket.1.has_use_buff&(talent.pillar_of_frost&!talent.breath_of_sindragosa&(trinket.1.cooldown.duration%%cooldown.pillar_of_frost.duration=0)|talent.breath_of_sindragosa&(cooldown.breath_of_sindragosa.duration%%trinket.1.cooldown.duration=0))", "Evaluates a trinkets cooldown, divided by pillar of frost, empower rune weapon, or breath of sindragosa's cooldown. If it's value has no remainder return 1, else return 0.5." );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=trinket.2.has_use_buff&(talent.pillar_of_frost&!talent.breath_of_sindragosa&(trinket.2.cooldown.duration%%cooldown.pillar_of_frost.duration=0)|talent.breath_of_sindragosa&(cooldown.breath_of_sindragosa.duration%%trinket.2.cooldown.duration=0))" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_cooldown&!trinket.1.is.improvised_seaforium_pacemaker&(trinket.1.has_use_buff|trinket.1.has_buff.strength|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit)" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_cooldown&!trinket.2.is.improvised_seaforium_pacemaker&(trinket.2.has_use_buff|trinket.2.has_buff.strength|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit)" );
  precombat->add_action( "variable,name=trinket_1_duration,value=trinket.1.proc.any_dps.duration" );
  precombat->add_action( "variable,name=trinket_2_duration,value=trinket.2.proc.any_dps.duration," );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs&(trinket.2.has_cooldown|!trinket.1.has_cooldown)|variable.trinket_2_buffs&((trinket.2.cooldown.duration%variable.trinket_2_duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync)*(1+((trinket.2.ilvl-trinket.1.ilvl)%100)))>((trinket.1.cooldown.duration%variable.trinket_1_duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync)*(1+((trinket.1.ilvl-trinket.2.ilvl)%100)))" );
  precombat->add_action( "variable,name=damage_trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs&trinket.2.ilvl>=trinket.1.ilvl" );
  precombat->add_action( "variable,name=trinket_1_manual,value=trinket.1.is.unyielding_netherprism" );
  precombat->add_action( "variable,name=trinket_2_manual,value=trinket.2.is.unyielding_netherprism" );

  default_->add_action( "auto_attack" );
  default_->add_action( "call_action_list,name=variables", "Choose Action list to run" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=high_prio_actions" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=racials" );
  default_->add_action( "run_action_list,name=aoe,if=active_enemies>=3" );
  default_->add_action( "run_action_list,name=single_target" );

  aoe->add_action( "frostscythe,if=buff.killing_machine.react=2&active_enemies>=variable.frostscythe_priority", "Aoe Rotation" );
  aoe->add_action( "frost_strike,target_if=max:(talent.shattering_blade&debuff.razorice.react=5),if=debuff.razorice.react=5&buff.frostbane.react" );
  aoe->add_action( "frostscythe,if=buff.killing_machine.react&rune>=3&active_enemies>=variable.frostscythe_priority" );
  aoe->add_action( "obliterate,target_if=max:(hero_tree.rider_of_the_apocalypse&debuff.chains_of_ice_trollbane_slow.react),if=buff.killing_machine.react=2|(buff.killing_machine.react&rune>=3)" );
  aoe->add_action( "howling_blast,if=buff.rime.react&talent.frostbound_will|!dot.frost_fever.ticking" );
  aoe->add_action( "frost_strike,target_if=max:(talent.shattering_blade&debuff.razorice.react=5),if=debuff.razorice.react=5&talent.shattering_blade&active_enemies<5&!variable.rp_pooling&!talent.frostbane" );
  aoe->add_action( "frostscythe,if=buff.killing_machine.react&!variable.rune_pooling&active_enemies>=variable.frostscythe_priority" );
  aoe->add_action( "obliterate,target_if=max:(hero_tree.rider_of_the_apocalypse&debuff.chains_of_ice_trollbane_slow.react),if=buff.killing_machine.react&!variable.rune_pooling" );
  aoe->add_action( "howling_blast,if=buff.rime.react" );
  aoe->add_action( "glacial_advance,if=!variable.rp_pooling" );
  aoe->add_action( "frostscythe,if=!variable.rune_pooling&!(talent.obliteration&buff.pillar_of_frost.up)&active_enemies>=variable.frostscythe_priority" );
  aoe->add_action( "obliterate,target_if=max:(hero_tree.rider_of_the_apocalypse&debuff.chains_of_ice_trollbane_slow.react),if=!variable.rune_pooling&!(talent.obliteration&buff.pillar_of_frost.up)" );
  aoe->add_action( "howling_blast,if=!buff.killing_machine.react&(talent.obliteration&buff.pillar_of_frost.up)" );

  cooldowns->add_action( "remorseless_winter,if=variable.sending_cds&(active_enemies>1|talent.gathering_storm)|(buff.gathering_storm.stack=10&buff.remorseless_winter.remains<gcd.max)&fight_remains>10", "Cooldowns" );
  cooldowns->add_action( "reapers_mark,target_if=first:debuff.reapers_mark_debuff.down,if=cooldown.pillar_of_frost.remains<=gcd.max&(!talent.breath_of_sindragosa|cooldown.breath_of_sindragosa.remains>20|cooldown.breath_of_sindragosa.remains<gcd.max&runic_power>=40)|fight_remains<20" );
  cooldowns->add_action( "pillar_of_frost,if=variable.sending_cds&(!hero_tree.deathbringer|cooldown.reapers_mark.remains>10)&(!talent.breath_of_sindragosa|cooldown.breath_of_sindragosa.remains>20|cooldown.breath_of_sindragosa.up&runic_power>=60)|fight_remains<20" );
  cooldowns->add_action( "breath_of_sindragosa,use_off_gcd=1,if=!buff.breath_of_sindragosa.up&(buff.pillar_of_frost.up|fight_remains<20)" );
  cooldowns->add_action( "frostwyrms_fury,if=((talent.apocalypse_now|talent.chosen_of_frostbrood)&!buff.chosen_of_frostbrood_fwf.up)&variable.sending_cds&(!talent.breath_of_sindragosa&buff.pillar_of_frost.up|buff.breath_of_sindragosa.up)&!debuff.reapers_mark_debuff.up&!buff.exterminate.up|(fight_remains<20&!buff.chosen_of_frostbrood_haste.up)" );
  cooldowns->add_action( "frostwyrms_fury,if=buff.chosen_of_frostbrood_fwf.up&variable.fwf_buffs" );
  cooldowns->add_action( "frostwyrms_fury,if=!(talent.apocalypse_now|talent.chosen_of_frostbrood)&active_enemies=1&(talent.pillar_of_frost&buff.pillar_of_frost.up&!talent.obliteration|!talent.pillar_of_frost)&(!raid_event.adds.exists|raid_event.adds.in>cooldown.frostwyrms_fury.duration+raid_event.adds.duration)&variable.fwf_buffs|fight_remains<3" );
  cooldowns->add_action( "frostwyrms_fury,if=!(talent.apocalypse_now|talent.chosen_of_frostbrood)&active_enemies>=2&(talent.pillar_of_frost&buff.pillar_of_frost.up|raid_event.adds.exists&raid_event.adds.up&raid_event.adds.in<cooldown.pillar_of_frost.remains-raid_event.adds.in-raid_event.adds.duration)&variable.fwf_buffs" );
  cooldowns->add_action( "frostwyrms_fury,if=!(talent.apocalypse_now|talent.chosen_of_frostbrood)&talent.obliteration&(talent.pillar_of_frost&buff.pillar_of_frost.up&!main_hand.2h|!buff.pillar_of_frost.up&main_hand.2h&cooldown.pillar_of_frost.remains|!talent.pillar_of_frost)&variable.fwf_buffs&(!raid_event.adds.exists|raid_event.adds.in>cooldown.frostwyrms_fury.duration+raid_event.adds.duration)" );
  cooldowns->add_action( "raise_dead,use_off_gcd=1" );
  cooldowns->add_action( "empower_rune_weapon,if=(rune<2|!buff.killing_machine.react)&runic_power<35+(talent.icy_onslaught*buff.icy_onslaught.stack*5)" );
  cooldowns->add_action( "empower_rune_weapon,if=cooldown.empower_rune_weapon.full_recharge_time<=6&buff.killing_machine.react<2-(talent.killing_streak)" );
  cooldowns->add_action( "empower_rune_weapon,if=talent.breath_of_sindragosa&(cooldown.empower_rune_weapon.full_recharge_time-30<=cooldown.breath_of_sindragosa.remains+6)&(cooldown.breath_of_sindragosa.remains<=6)&(buff.killing_machine.react<2-(talent.killing_streak))" );
  cooldowns->add_action( "empower_rune_weapon,if=talent.obliteration&buff.pillar_of_frost.remains>4*gcd.max&rune<=2&buff.killing_machine.react=1" );

  high_prio_actions->add_action( "mind_freeze,if=target.debuff.casting.react", "High Priority Actions" );
  high_prio_actions->add_action( "invoke_external_buff,name=power_infusion,if=variable.cooldown_check", "Use <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> while <a href='https://www.wowhead.com/spell=51271/pillar-of-frost'>Pillar of Frost</a> is up" );
  high_prio_actions->add_action( "antimagic_shell,if=runic_power.deficit>40&death_knight.first_ams_cast<time" );

  racials->add_action( "blood_fury,use_off_gcd=1,if=variable.cooldown_check", "Obliteration Active Rotation  Racial Abilities" );
  racials->add_action( "berserking,use_off_gcd=1,if=variable.cooldown_check" );
  racials->add_action( "arcane_pulse,if=variable.cooldown_check" );
  racials->add_action( "lights_judgment,if=variable.cooldown_check" );
  racials->add_action( "ancestral_call,use_off_gcd=1,if=variable.cooldown_check" );
  racials->add_action( "fireblood,use_off_gcd=1,if=variable.cooldown_check" );
  racials->add_action( "bag_of_tricks,if=talent.obliteration&!buff.pillar_of_frost.up&buff.unholy_strength.up" );
  racials->add_action( "bag_of_tricks,if=!talent.obliteration&buff.pillar_of_frost.up&(buff.unholy_strength.up&buff.unholy_strength.remains<gcd*3|buff.pillar_of_frost.remains<gcd*3)" );

  single_target->add_action( "obliterate,if=buff.killing_machine.react=2|(buff.killing_machine.react&rune>=3)", "Single Target Rotation" );
  single_target->add_action( "howling_blast,if=buff.rime.react&talent.frostbound_will" );
  single_target->add_action( "frost_strike,target_if=max:(talent.shattering_blade&debuff.razorice.react=5),if=debuff.razorice.react=5&talent.shattering_blade&!variable.rp_pooling" );
  single_target->add_action( "howling_blast,if=buff.rime.react" );
  single_target->add_action( "frost_strike,if=!talent.shattering_blade&!variable.rp_pooling&runic_power.deficit<30" );
  single_target->add_action( "obliterate,if=buff.killing_machine.react&!variable.rune_pooling" );
  single_target->add_action( "frost_strike,if=!variable.rp_pooling" );
  single_target->add_action( "obliterate,if=!variable.rune_pooling&!(talent.obliteration&buff.pillar_of_frost.up)" );
  single_target->add_action( "howling_blast,if=!buff.killing_machine.react&(talent.obliteration&buff.pillar_of_frost.up)" );

  trinkets->add_action( "potion,use_off_gcd=1,if=variable.cooldown_check|fight_remains<25", "Trinkets  Trinkets The trinket with the highest estimated value, will be used first and paired with Pillar of Frost." );
  trinkets->add_action( "use_item,slot=trinket1,if=!trinket.1.cast_time>0&variable.trinket_1_buffs&!variable.trinket_1_manual&buff.pillar_of_frost.remains&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)" );
  trinkets->add_action( "use_item,slot=trinket2,if=!trinket.2.cast_time>0&variable.trinket_2_buffs&!variable.trinket_2_manual&buff.pillar_of_frost.remains&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)" );
  trinkets->add_action( "use_item,slot=trinket1,if=trinket.1.cast_time>0&(!hero_tree.rider_of_the_apocalypse|cooldown.frostwyrms_fury.remains)&variable.trinket_1_buffs&!variable.trinket_1_manual&cooldown.pillar_of_frost.remains<trinket.1.cast_time&(!talent.breath_of_sindragosa|variable.breath_of_sindragosa_check)&variable.sending_cds&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|variable.trinket_1_duration>=fight_remains", "Channeled buff trinkets will be used before cooldowns" );
  trinkets->add_action( "use_item,slot=trinket2,if=trinket.2.cast_time>0&(!hero_tree.rider_of_the_apocalypse|cooldown.frostwyrms_fury.remains)&variable.trinket_2_buffs&!variable.trinket_2_manual&cooldown.pillar_of_frost.remains<trinket.2.cast_time&(!talent.breath_of_sindragosa|variable.breath_of_sindragosa_check)&variable.sending_cds&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|variable.trinket_2_duration>=fight_remains" );
  trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&!variable.trinket_1_manual&(variable.damage_trinket_priority=1|(!trinket.2.has_cooldown|trinket.2.cooldown.remains))&((trinket.1.cast_time>0&(!talent.breath_of_sindragosa|!buff.breath_of_sindragosa.up)&!buff.pillar_of_frost.up|!trinket.1.cast_time>0)&(!variable.trinket_2_buffs|cooldown.pillar_of_frost.remains>20)|!talent.pillar_of_frost)|fight_remains<15", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&!variable.trinket_2_manual&(variable.damage_trinket_priority=2|(!trinket.1.has_cooldown|trinket.1.cooldown.remains))&((trinket.2.cast_time>0&(!talent.breath_of_sindragosa|!buff.breath_of_sindragosa.up)&!buff.pillar_of_frost.up|!trinket.2.cast_time>0)&(!variable.trinket_1_buffs|cooldown.pillar_of_frost.remains>20)|!talent.pillar_of_frost)|fight_remains<15" );
  trinkets->add_action( "use_item,slot=main_hand,if=buff.pillar_of_frost.up|(buff.breath_of_sindragosa.up&cooldown.pillar_of_frost.remains)|(variable.trinket_1_buffs&variable.trinket_2_buffs&(trinket.1.cooldown.remains<cooldown.pillar_of_frost.remains|trinket.2.cooldown.remains<cooldown.pillar_of_frost.remains)&cooldown.pillar_of_frost.remains>20)|fight_remains<15" );

  variables->add_action( "variable,name=st_planning,op=setif,value=1,value_else=0,condition=active_enemies=1&(!raid_event.adds.exists|!raid_event.adds.in|raid_event.adds.in>15)", "Variables" );
  variables->add_action( "variable,name=adds_remain,value=active_enemies>=2&(!raid_event.adds.exists|!raid_event.pull.exists&raid_event.adds.remains>5|raid_event.pull.exists&raid_event.adds.in>20)" );
  variables->add_action( "variable,name=sending_cds,value=(variable.st_planning|variable.adds_remain)" );
  variables->add_action( "variable,name=cooldown_check,value=(talent.pillar_of_frost&buff.pillar_of_frost.up)|!talent.pillar_of_frost|fight_remains<20" );
  variables->add_action( "variable,name=fwf_buffs,value=(buff.pillar_of_frost.remains<gcd.max|(buff.unholy_strength.up&buff.unholy_strength.remains<gcd.max)|(talent.bonegrinder.rank=2&buff.bonegrinder_frost.up&buff.bonegrinder_frost.remains<gcd.max))&(active_enemies>1|debuff.razorice.stack=5|talent.shattering_blade)" );
  variables->add_action( "variable,name=rune_pooling,value=hero_tree.deathbringer&cooldown.reapers_mark.remains<6&rune<3&variable.sending_cds" );
  variables->add_action( "variable,name=rp_pooling,value=talent.breath_of_sindragosa&cooldown.breath_of_sindragosa.remains<4*gcd.max&runic_power<60+(35+5*buff.icy_onslaught.up)-(10*rune)&variable.sending_cds" );
  variables->add_action( "variable,name=frostscythe_priority,value=3", "Frostscythe is equal at 3 targets" );
  variables->add_action( "variable,name=breath_of_sindragosa_check,value=!talent.breath_of_sindragosa|(cooldown.breath_of_sindragosa.remains>20|(cooldown.breath_of_sindragosa.remains<1*gcd.max&runic_power>=(60-20*hero_tree.deathbringer)))" );
}
//frost_apl_end

//unholy_apl_start
void unholy( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* racials = p->get_action_priority_list( "racials" );
  action_priority_list_t* single_target = p->get_action_priority_list( "single_target" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "raise_dead" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff" );
  precombat->add_action( "variable,name=trinket_1_duration,op=setif,value=0,value_else=trinket.1.proc.any_dps.duration,condition=0" );
  precombat->add_action( "variable,name=trinket_2_duration,op=setif,value=0,value_else=trinket.2.proc.any_dps.duration,condition=0" );
  precombat->add_action( "variable,name=trinket_1_high_value,op=setif,value=2,value_else=1,condition=trinket.1.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_2_high_value,op=setif,value=2,value_else=1,condition=trinket.2.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&talent.dark_transformation&trinket.1.cooldown.duration%%cooldown.dark_transformation.duration=0" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&talent.dark_transformation&trinket.2.cooldown.duration%%cooldown.dark_transformation.duration=0" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs&(trinket.2.has_cooldown|!trinket.1.has_cooldown)|variable.trinket_2_buffs&((trinket.2.cooldown.duration%variable.trinket_2_duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync)*(variable.trinket_2_high_value)*(1+((trinket.2.ilvl-trinket.1.ilvl)%100)))>((trinket.1.cooldown.duration%variable.trinket_1_duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync)*(variable.trinket_1_high_value)*(1+((trinket.1.ilvl-trinket.2.ilvl)%100)))" );
  precombat->add_action( "variable,name=damage_trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs&trinket.2.ilvl>=trinket.1.ilvl" );

  default_->add_action( "auto_attack" );
  default_->add_action( "call_action_list,name=variables", "Choose Action list to run" );
  default_->add_action( "call_action_list,name=racials" );
  default_->add_action( "potion,if=(variable.st_planning|variable.adds_remain)&variable.cds_active&(variable.trinket_priority=1&trinket.1.has_use_buff&!trinket.1.proc.mastery.duration>0|variable.trinket_priority=2&trinket.2.has_use_buff&!trinket.2.proc.mastery.duration>0)" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>=3" );
  default_->add_action( "call_action_list,name=single_target,if=active_enemies<3" );

  aoe->add_action( "death_and_decay,if=!death_and_decay.ticking&talent.desecrate|talent.cycle_of_death&cooldown.putrefy.charges<cooldown.putrefy.max_charges&(raid_event.adds.exists&raid_event.adds.remains>duration%2|!raid_event.adds.exists&fight_remains>duration%2)&(raid_event.adds.exists&raid_event.adds.count<active_enemies|!raid_event.adds.exists|charges=max_charges|raid_event.adds.remains>cooldown.any_dnd.duration)", "Aoe Rotation" );
  aoe->add_action( "festering_strike,target_if=min:health.pct,if=talent.festering_scythe&(fight_remains>3|raid_event.adds.exists&raid_event.adds.remains>3)&(buff.festering_scythe.up&(buff.festering_scythe.remains<=3|buff.festering_scythe_tt.remains<3)|!buff.festering_scythe.up&buff.festering_scythe_tt.remains<3)" );
  aoe->add_action( "epidemic,if=variable.spending_rp&variable.epidemic_prio" );
  aoe->add_action( "death_coil,target_if=min:health.pct,if=variable.spending_rp&!variable.epidemic_prio" );
  aoe->add_action( "festering_strike,target_if=min:health.pct,if=buff.lesser_ghoul_ready.stack=0" );
  aoe->add_action( "scourge_strike,target_if=min:health.pct,if=buff.lesser_ghoul_ready.stack>=1" );
  aoe->add_action( "epidemic,if=variable.epidemic_prio" );
  aoe->add_action( "death_coil,target_if=min:health.pct,if=!variable.epidemic_prio" );

  cooldowns->add_action( "potion,if=(variable.st_planning|variable.adds_remain)&variable.cds_active", "Cooldowns" );
  cooldowns->add_action( "invoke_external_buff,name=power_infusion,if=pet.lesser_ghoul_army.active|buff.forbidden_knowledge.up|buff.dark_transformation.up", "Use<a href = 'https://www.wowhead.com/spell=10060/power-infusion'> Power Infusion</ a> while<a href = 'https://www.wowhead.com/spell=1233448/dark-transformation'> Dark Transformation</ a> is up" );
  cooldowns->add_action( "outbreak,if=(!talent.blightburst|talent.blightburst&(cooldown.putrefy.remains>gcd.max*2|time<5))&(dot.dread_plague.active_dots=0|dot.virulent_plague.active_dots=0)&(fight_remains>gcd.max*2&!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.remains>gcd.max*2)" );
  cooldowns->add_action( "army_of_the_dead,if=(variable.st_planning|variable.adds_remain)&(buff.festering_scythe_tt.up|!talent.festering_scythe)" );
  cooldowns->add_action( "soul_reaper,target_if=min:health.pct,if=!talent.blightfall&(!debuff.soul_reaper_debuff.up|!variable.cds_active&cooldown.dark_transformation.remains>cooldown.soul_reaper.duration-1|cooldown.dark_transformation.remains<gcd.max&talent.reaping)|talent.blightfall&talent.infliction_of_sorrow&(buff.dark_transformation.remains<5|buff.reaping.remains<=gcd.max)" );
  cooldowns->add_action( "putrefy,if=(variable.st_planning|variable.adds_remain)*(target.health.pct>35|!talent.soul_reaper)&(charges=max_charges&!buff.sudden_doom.react&(cooldown.dark_transformation.remains>9|!talent.reaping|!talent.soul_reaper)|buff.dark_transformation.up)|fight_remains<cooldown.soul_reaper.remains|raid_event.adds.exists&raid_event.adds.remains<3" );
  cooldowns->add_action( "dark_transformation,if=(variable.st_planning|variable.adds_remain)&!buff.blightfall.up&(pet.lesser_ghoul_army.active|cooldown.army_of_the_dead.remains>30|!talent.army_of_the_dead)|buff.blightfall.up&buff.dark_transformation.remains<4" );

  racials->add_action( "ancestral_call,if=variable.cds_active", "Racials" );
  racials->add_action( "arcane_pulse,if=runic_power<20&rune<2" );
  racials->add_action( "arcane_torrent,if=runic_power<20&rune<2" );
  racials->add_action( "bag_of_tricks,if=runic_power<20&rune<2" );
  racials->add_action( "blood_fury,if=buff.dark_transformation.up" );
  racials->add_action( "berserking,if=variable.cds_active" );
  racials->add_action( "fireblood,if=variable.cds_active" );
  racials->add_action( "lights_judgment,if=runic_power<20&rune<2" );

  single_target->add_action( "festering_strike,if=talent.festering_scythe&fight_remains>3&(buff.festering_scythe.up&(buff.festering_scythe.remains<=3|buff.festering_scythe_tt.remains<3)|!buff.festering_scythe.up&buff.festering_scythe_tt.remains<3)", "Single Target Rotation" );
  single_target->add_action( "scourge_strike,if=buff.lesser_ghoul_ready.stack>=1&talent.gift_of_the_sanlayn&buff.essence_of_the_blood_queen.stack<5&buff.vampiric_strike.up" );
  single_target->add_action( "death_coil,if=buff.sudden_doom.react" );
  single_target->add_action( "scourge_strike,if=buff.lesser_ghoul_ready.stack>=1&buff.blighted.up" );
  single_target->add_action( "death_coil,if=variable.spending_rp" );
  single_target->add_action( "putrefy,if=(target.health.pct>35|!talent.soul_reaper)&(buff.commander_of_the_dead.remains>9|!talent.commander_of_the_dead)" );
  single_target->add_action( "scourge_strike,if=buff.lesser_ghoul_ready.stack>=1" );
  single_target->add_action( "festering_strike" );
  single_target->add_action( "death_coil" );

  trinkets->add_action( "use_item,slot=trinket1,if=variable.trinket_1_buffs&(variable.trinket_priority=1|!variable.trinket_2_buffs|!trinket.2.has_cooldown)&(trinket.1.cast_time>0&trinket.1.cast_time>cooldown.army_of_the_dead.remains&(!talent.festering_scythe|buff.festering_scythe_tt.up)|trinket.1.cast_time=0&variable.cds_active)", "Trinkets" );
  trinkets->add_action( "use_item,slot=trinket2,if=variable.trinket_2_buffs&(variable.trinket_priority=2|!variable.trinket_1_buffs|!trinket.1.has_cooldown)&(trinket.2.cast_time>0&trinket.2.cast_time>cooldown.army_of_the_dead.remains&(!talent.festering_scythe|buff.festering_scythe_tt.up)|trinket.2.cast_time=0&variable.cds_active)" );
  trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(variable.damage_trinket_priority=1|!variable.trinket_2_buffs|!trinket.2.has_cooldown)" );
  trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(variable.damage_trinket_priority=2|!variable.trinket_1_buffs|!trinket.1.has_cooldown)" );

  variables->add_action( "variable,name=spending_rp,value=rune<2|buff.forbidden_knowledge.up&(rune<4|pet.gargoyle.active)|buff.sudden_doom.react", "Variables" );
  variables->add_action( "variable,name=st_planning,op=setif,value=1,value_else=0,condition=active_enemies=1&(!raid_event.adds.exists|!raid_event.adds.in|raid_event.adds.in>15|!raid_event.pull.exists|raid_event.pull.exists&raid_event.pull.in>15)" );
  variables->add_action( "variable,name=adds_remain,value=active_enemies>=2&((!raid_event.adds.exists|!raid_event.pull.exists)|raid_event.adds.remains>5|raid_event.pull.remains>5)" );
  variables->add_action( "variable,name=cds_active,value=pet.lesser_ghoul_army.active|buff.forbidden_knowledge.up|buff.dark_transformation.up&buff.dark_transformation.remains>5" );
  variables->add_action( "variable,name=epidemic_prio,value=active_enemies>=4&!buff.forbidden_knowledge.up|active_enemies>=6&buff.forbidden_knowledge.up" );
}
//unholy_apl_end

}  // namespace death_knight_apl
