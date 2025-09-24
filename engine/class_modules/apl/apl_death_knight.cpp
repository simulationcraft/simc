#include "class_modules/apl/apl_death_knight.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"
#include "dbc/dbc.hpp"
#include "sim/sim.hpp"

namespace death_knight_apl
{

std::string potion( const player_t* p )
{
  std::string frost_potion = ( p->true_level >= 71 ) ? "tempered_potion_3" : "elemental_potion_of_ultimate_power_3";

  std::string unholy_potion = ( p->true_level >= 71 ) ? "tempered_potion_3" : "elemental_potion_of_ultimate_power_3";

  std::string blood_potion = ( p->true_level >= 71 ) ? "tempered_potion_3" : "elemental_potion_of_ultimate_power_3";

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
  std::string frost_flask = ( p->true_level >= 71 ) ? "flask_of_alchemical_chaos_3" : "iced_phial_of_corrupting_rage_3";

  std::string unholy_flask = ( p->true_level >= 71 ) ? "flask_of_alchemical_chaos_3" : "iced_phial_of_corrupting_rage_3";

  std::string blood_flask = ( p->true_level >= 71 ) ? "flask_of_alchemical_chaos_3" : "iced_phial_of_corrupting_rage_3";

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

  if ( p->true_level >= 71 )
  {
    frost_food = "feast_of_the_divine_day";
    unholy_food = "chippy_tea";
    blood_food = "beledars_bounty";
  }
  else
  {
    frost_food = "sizzling_seafood_medley";
    unholy_food = "sizzling_seafood_medley";
    blood_food = "great_cerulean_sea";
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
  return ( p->true_level >= 71 ) ? "crystallized" : "draconic";
}

std::string temporary_enchant( const player_t* p )
{
  std::string frost_temporary_enchant = ( p->true_level >= 71 )
                                            ? "main_hand:algari_mana_oil_3/off_hand:algari_mana_oil_3"
                                            : "main_hand:buzzing_rune_3/off_hand:buzzing_rune_3";

  std::string unholy_temporary_enchant =
      ( p->true_level >= 71 ) ? "main_hand:algari_mana_oil_3" : "main_hand:howling_rune_3";

  std::string blood_temporary_enchant =
      ( p->true_level >= 71 ) ? "main_hand:ironclaw_whetstone_3" : "main_hand:howling_rune_3";

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
  action_priority_list_t* san_drw = p->get_action_priority_list( "san_drw" );
  action_priority_list_t* sanlayn = p->get_action_priority_list( "sanlayn" );

  precombat->add_action( "snapshot_stats", "Default consumables otion=tempered_potion_ lask=flask_of_alchemical_chaos_ ood=beledars_bount ugmentation=crystallize emporary_enchant=main_hand:algari_mana_oil_" );
  precombat->add_action( "deaths_caress" );

  default_->add_action( "auto_attack" );
  default_->add_action( "use_item,name=tome_of_lights_devotion,if=buff.inner_resilience.up,use_off_gcd=1" );
  default_->add_action( "use_item,name=unyielding_netherprism,if=cooldown.dancing_rune_weapon.remains<1|target.time_to_die<=20,use_off_gcd=1" );
  default_->add_action( "use_items" );
  default_->add_action( "use_item,name=bestinslots,use_off_gcd=1" );
  default_->add_action( "blood_fury,if=buff.dancing_rune_weapon.up" );
  default_->add_action( "berserking,if=buff.dancing_rune_weapon.up" );
  default_->add_action( "ancestral_call,if=buff.dancing_rune_weapon.up" );
  default_->add_action( "fireblood,if=buff.dancing_rune_weapon.up" );
  default_->add_action( "potion,if=buff.dancing_rune_weapon.up" );
  default_->add_action( "vampiric_blood,if=!buff.vampiric_blood.up" );
  default_->add_action( "call_action_list,name=high_prio_actions" );
  default_->add_action( "run_action_list,name=deathbringer,if=hero_tree.deathbringer" );
  default_->add_action( "run_action_list,name=san_drw,if=hero_tree.sanlayn&buff.dancing_rune_weapon.up" );
  default_->add_action( "run_action_list,name=sanlayn,if=hero_tree.sanlayn" );

  high_prio_actions->add_action( "raise_dead,use_off_gcd=1" );
  high_prio_actions->add_action( "blood_tap,if=(rune<=2&rune.time_to_3>gcd&charges_fractional>=1.8)" );
  high_prio_actions->add_action( "blood_tap,if=(rune<=1&rune.time_to_3>gcd)" );
  high_prio_actions->add_action( "death_strike,if=buff.coagulopathy.up&buff.coagulopathy.remains<=gcd" );
  high_prio_actions->add_action( "dancing_rune_weapon" );

  deathbringer->add_action( "rune_tap,if=rune>4" );
  deathbringer->add_action( "bonestorm,if=buff.bone_shield.stack>=5&buff.death_and_decay.remains" );
  deathbringer->add_action( "death_strike,if=(runic_power.deficit<20|(runic_power.deficit<26&buff.dancing_rune_weapon.up))" );
  deathbringer->add_action( "soul_reaper,if=active_enemies<=2&buff.reaper_of_souls.up&target.time_to_die>(dot.soul_reaper.remains+5)" );
  deathbringer->add_action( "soul_reaper,if=active_enemies<=2&target.time_to_pct_35<5&target.time_to_die>(dot.soul_reaper.remains+5)" );
  deathbringer->add_action( "reapers_mark" );
  deathbringer->add_action( "blood_boil,if=buff.dancing_rune_weapon.up&!drw.bp_ticking" );
  deathbringer->add_action( "death_and_decay,if=!buff.death_and_decay.up" );
  deathbringer->add_action( "marrowrend,if=buff.exterminate.up|(buff.bone_shield.stack<5&!dot.bonestorm.ticking)" );
  deathbringer->add_action( "death_strike" );
  deathbringer->add_action( "tombstone,if=buff.bone_shield.stack>=8&buff.death_and_decay.remains&cooldown.dancing_rune_weapon.remains>=25" );
  deathbringer->add_action( "blooddrinker,if=!buff.dancing_rune_weapon.up&active_enemies<=2&buff.coagulopathy.remains>3" );
  deathbringer->add_action( "consumption" );
  deathbringer->add_action( "blood_boil" );
  deathbringer->add_action( "heart_strike,if=buff.coagulopathy.stack<5" );
  deathbringer->add_action( "heart_strike" );
  deathbringer->add_action( "soul_reaper,if=buff.reaper_of_souls.up" );
  deathbringer->add_action( "arcane_torrent,if=runic_power.deficit>20" );

  san_drw->add_action( "heart_strike,if=buff.essence_of_the_blood_queen.remains<1.5&buff.essence_of_the_blood_queen.remains" );
  san_drw->add_action( "bonestorm,if=buff.bone_shield.stack>=5" );
  san_drw->add_action( "death_strike,if=runic_power.deficit<36" );
  san_drw->add_action( "blood_boil,if=!drw.bp_ticking" );
  san_drw->add_action( "any_dnd,if=(active_enemies<=3&buff.crimson_scourge.remains)|(active_enemies>3&!buff.death_and_decay.remains)" );
  san_drw->add_action( "heart_strike" );
  san_drw->add_action( "death_strike" );
  san_drw->add_action( "consumption" );
  san_drw->add_action( "blood_boil" );

  sanlayn->add_action( "blood_boil,if=(!buff.bone_shield.up|buff.bone_shield.remains<1.5|buff.bone_shield.stack<=1)&active_enemies>=2" );
  sanlayn->add_action( "deaths_caress,if=!buff.bone_shield.up|buff.bone_shield.remains<1.5|buff.bone_shield.stack<=1" );
  sanlayn->add_action( "blood_boil,if=dot.blood_plague.remains<3" );
  sanlayn->add_action( "heart_strike,if=(buff.essence_of_the_blood_queen.remains<1.5&buff.essence_of_the_blood_queen.remains&buff.vampiric_strike.remains)" );
  sanlayn->add_action( "bonestorm,if=buff.bone_shield.stack>=5&(buff.death_and_decay.remains|active_enemies<=3)" );
  sanlayn->add_action( "death_strike,if=runic_power.deficit<20" );
  sanlayn->add_action( "consumption,if=buff.infliction_of_sorrow.up&buff.death_and_decay.up" );
  sanlayn->add_action( "heart_strike,if=(buff.infliction_of_sorrow.up|buff.vampiric_strike.up)&buff.death_and_decay.up" );
  sanlayn->add_action( "soul_reaper,if=active_enemies<=2&target.time_to_pct_35<5&target.time_to_die>(dot.soul_reaper.remains+5)" );
  sanlayn->add_action( "blood_boil,if=buff.bone_shield.stack<6&!dot.bonestorm.ticking&active_enemies>=2" );
  sanlayn->add_action( "deaths_caress,if=buff.bone_shield.stack<6&!dot.bonestorm.ticking" );
  sanlayn->add_action( "marrowrend,if=buff.bone_shield.stack<6&!dot.bonestorm.ticking" );
  sanlayn->add_action( "tombstone,if=buff.bone_shield.stack>=6&(buff.death_and_decay.remains|active_enemies<=3)&cooldown.dancing_rune_weapon.remains>=25" );
  sanlayn->add_action( "any_dnd,if=(active_enemies<=3&buff.crimson_scourge.remains)|(active_enemies>3&!buff.death_and_decay.remains)" );
  sanlayn->add_action( "blooddrinker,if=active_enemies<=2&buff.coagulopathy.remains>3" );
  sanlayn->add_action( "heart_strike,if=buff.vampiric_strike.up" );
  sanlayn->add_action( "death_strike" );
  sanlayn->add_action( "heart_strike,if=rune>=2" );
  sanlayn->add_action( "consumption" );
  sanlayn->add_action( "blood_boil" );
  sanlayn->add_action( "heart_strike" );
}
//blood_apl_end

//frost_apl_start
void frost( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* high_prio_actions = p->get_action_priority_list( "high_prio_actions" );
  action_priority_list_t* racials = p->get_action_priority_list( "racials" );
  action_priority_list_t* single_target = p->get_action_priority_list( "single_target" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
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

  cooldowns->add_action( "potion,use_off_gcd=1,if=variable.cooldown_check|fight_remains<25", "target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice+((hero_tree.deathbringer&debuff.reapers_mark_debuff.up)*5)  Cooldowns" );
  cooldowns->add_action( "remorseless_winter,if=variable.sending_cds&(active_enemies>1|talent.gathering_storm)|(buff.gathering_storm.stack=10&buff.remorseless_winter.remains<gcd.max)&fight_remains>10" );
  cooldowns->add_action( "frostwyrms_fury,if=hero_tree.rider_of_the_apocalypse&talent.apocalypse_now&variable.sending_cds&(cooldown.pillar_of_frost.remains<gcd.max|fight_remains<20)&!talent.breath_of_sindragosa" );
  cooldowns->add_action( "frostwyrms_fury,if=hero_tree.rider_of_the_apocalypse&talent.apocalypse_now&variable.sending_cds&(cooldown.pillar_of_frost.remains<gcd.max|fight_remains<20)&talent.breath_of_sindragosa&runic_power>=60" );
  cooldowns->add_action( "pillar_of_frost,if=!talent.breath_of_sindragosa&variable.sending_cds&(!hero_tree.deathbringer|rune>=2)|fight_remains<20" );
  cooldowns->add_action( "pillar_of_frost,if=talent.breath_of_sindragosa&variable.sending_cds&variable.breath_of_sindragosa_check&(!hero_tree.deathbringer|rune>=2)" );
  cooldowns->add_action( "breath_of_sindragosa,use_off_gcd=1,if=!buff.breath_of_sindragosa.up&(buff.pillar_of_frost.up|fight_remains<20)" );
  cooldowns->add_action( "reapers_mark,target_if=first:debuff.reapers_mark_debuff.down,if=buff.pillar_of_frost.up|cooldown.pillar_of_frost.remains>5|fight_remains<20" );
  cooldowns->add_action( "frostwyrms_fury,if=!talent.apocalypse_now&active_enemies=1&(talent.pillar_of_frost&buff.pillar_of_frost.up&!talent.obliteration|!talent.pillar_of_frost)&(!raid_event.adds.exists|raid_event.adds.in>cooldown.frostwyrms_fury.duration+raid_event.adds.duration)&variable.fwf_buffs|fight_remains<3" );
  cooldowns->add_action( "frostwyrms_fury,if=!talent.apocalypse_now&active_enemies>=2&(talent.pillar_of_frost&buff.pillar_of_frost.up|raid_event.adds.exists&raid_event.adds.up&raid_event.adds.in<cooldown.pillar_of_frost.remains-raid_event.adds.in-raid_event.adds.duration)&variable.fwf_buffs" );
  cooldowns->add_action( "frostwyrms_fury,if=!talent.apocalypse_now&talent.obliteration&(talent.pillar_of_frost&buff.pillar_of_frost.up&!main_hand.2h|!buff.pillar_of_frost.up&main_hand.2h&cooldown.pillar_of_frost.remains|!talent.pillar_of_frost)&variable.fwf_buffs&(!raid_event.adds.exists|raid_event.adds.in>cooldown.frostwyrms_fury.duration+raid_event.adds.duration)" );
  cooldowns->add_action( "raise_dead,use_off_gcd=1" );
  cooldowns->add_action( "soul_reaper,if=talent.reaper_of_souls&buff.reaper_of_souls.up&buff.killing_machine.react<2" );
  cooldowns->add_action( "empower_rune_weapon,use_off_gcd=1,if=(rune<2|!buff.killing_machine.react)&runic_power<35+(talent.icy_onslaught*buff.icy_onslaught.stack*5)&gcd.remains<0.5" );
  cooldowns->add_action( "empower_rune_weapon,use_off_gcd=1,if=cooldown.empower_rune_weapon.full_recharge_time<=6&buff.killing_machine.react<1+(1*talent.killing_streak)&gcd.remains<0.5" );

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

  aoe->add_action( "frostscythe,if=(buff.killing_machine.react=2|(buff.killing_machine.react&rune>=3))&active_enemies>=variable.frostscythe_prio", "Aoe Rotation" );
  aoe->add_action( "obliterate,target_if=max:(hero_tree.rider_of_the_apocalypse&debuff.chains_of_ice_trollbane_slow.react),if=buff.killing_machine.react=2|(buff.killing_machine.react&rune>=3)" );
  aoe->add_action( "howling_blast,if=buff.rime.react&talent.frostbound_will|!dot.frost_fever.ticking" );
  aoe->add_action( "frost_strike,target_if=max:(talent.shattering_blade&debuff.razorice.react=5),if=debuff.razorice.react=5&buff.frostbane.react" );
  aoe->add_action( "frost_strike,target_if=max:(talent.shattering_blade&debuff.razorice.react=5),if=debuff.razorice.react=5&talent.shattering_blade&active_enemies<5&!variable.rp_pooling&!talent.frostbane" );
  aoe->add_action( "frostscythe,if=buff.killing_machine.react&!variable.rune_pooling&active_enemies>=variable.frostscythe_prio" );
  aoe->add_action( "obliterate,target_if=max:(hero_tree.rider_of_the_apocalypse&debuff.chains_of_ice_trollbane_slow.react),if=buff.killing_machine.react&!variable.rune_pooling" );
  aoe->add_action( "howling_blast,if=buff.rime.react" );
  aoe->add_action( "glacial_advance,if=!variable.rp_pooling" );
  aoe->add_action( "frostscythe,if=!variable.rune_pooling&!(talent.obliteration&buff.pillar_of_frost.up)&active_enemies>=variable.frostscythe_prio" );
  aoe->add_action( "obliterate,target_if=max:(hero_tree.rider_of_the_apocalypse&debuff.chains_of_ice_trollbane_slow.react),if=!variable.rune_pooling&!(talent.obliteration&buff.pillar_of_frost.up)" );
  aoe->add_action( "howling_blast,if=!buff.killing_machine.react&(talent.obliteration&buff.pillar_of_frost.up)" );

  trinkets->add_action( "use_item,name=unyielding_netherprism,if=buff.latent_energy.stack>8&buff.pillar_of_frost.remains&(!talent.breath_of_sindragosa|buff.breath_of_sindragosa.remains)", "Trinkets  Trinkets The trinket with the highest estimated value, will be used first and paired with Pillar of Frost." );
  trinkets->add_action( "use_item,slot=trinket1,if=!trinket.1.cast_time>0&variable.trinket_1_buffs&!variable.trinket_1_manual&buff.pillar_of_frost.remains&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)" );
  trinkets->add_action( "use_item,slot=trinket2,if=!trinket.2.cast_time>0&variable.trinket_2_buffs&!variable.trinket_2_manual&buff.pillar_of_frost.remains&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)" );
  trinkets->add_action( "use_item,slot=trinket1,if=trinket.1.cast_time>0&(!trinket.2.is.unyielding_netherprism|buff.latent_energy.stack<8)&(!hero_tree.rider_of_the_apocalypse|cooldown.frostwyrms_fury.remains)&variable.trinket_1_buffs&!variable.trinket_1_manual&cooldown.pillar_of_frost.remains<trinket.1.cast_time&(!talent.breath_of_sindragosa|variable.breath_of_sindragosa_check)&variable.sending_cds&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|variable.trinket_1_duration>=fight_remains", "Channeled buff trinkets will be used before cooldowns" );
  trinkets->add_action( "use_item,slot=trinket2,if=trinket.2.cast_time>0&(!trinket.1.is.unyielding_netherprism|buff.latent_energy.stack<8)&(!hero_tree.rider_of_the_apocalypse|cooldown.frostwyrms_fury.remains)&variable.trinket_2_buffs&!variable.trinket_2_manual&cooldown.pillar_of_frost.remains<trinket.2.cast_time&(!talent.breath_of_sindragosa|variable.breath_of_sindragosa_check)&variable.sending_cds&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|variable.trinket_2_duration>=fight_remains" );
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
  variables->add_action( "variable,name=frostscythe_prio,value=3+(1*(set_bonus.tww3_rider_of_the_apocalypse_4pc&!(talent.cleaving_strikes&buff.remorseless_winter.up)))", "Frostscythe is equal at 3 targets, except with Rider 4pc which brings Obliterate higher at 3, unless cleaving strikes is up" );
  variables->add_action( "variable,name=breath_of_sindragosa_check,value=talent.breath_of_sindragosa&(cooldown.breath_of_sindragosa.remains>20|(cooldown.breath_of_sindragosa.up&runic_power>=(60-20*hero_tree.deathbringer)))" );
}
//frost_apl_end

//unholy_apl_start
void unholy( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* aoe_burst = p->get_action_priority_list( "aoe_burst" );
  action_priority_list_t* aoe_setup = p->get_action_priority_list( "aoe_setup" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* cds_aoe = p->get_action_priority_list( "cds_aoe" );
  action_priority_list_t* cds_aoe_san = p->get_action_priority_list( "cds_aoe_san" );
  action_priority_list_t* cds_cleave_san = p->get_action_priority_list( "cds_cleave_san" );
  action_priority_list_t* cds_san = p->get_action_priority_list( "cds_san" );
  action_priority_list_t* cds_shared = p->get_action_priority_list( "cds_shared" );
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* racials = p->get_action_priority_list( "racials" );
  action_priority_list_t* san_fishing = p->get_action_priority_list( "san_fishing" );
  action_priority_list_t* san_st = p->get_action_priority_list( "san_st" );
  action_priority_list_t* san_trinkets = p->get_action_priority_list( "san_trinkets" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );
  action_priority_list_t* variables = p->get_action_priority_list( "variables" );

  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "raise_dead" );
  precombat->add_action( "army_of_the_dead,precombat_time=2" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_use_buff|trinket.1.is.treacherous_transmitter|trinket.1.is.unyielding_netherprism" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_use_buff|trinket.2.is.treacherous_transmitter|trinket.2.is.unyielding_netherprism" );
  precombat->add_action( "variable,name=trinket_1_duration,op=setif,value=trinket.1.is.treacherous_transmitter*15+trinket.1.is.funhouse_lens*15+trinket.1.is.signet_of_the_priory*20+trinket.1.is.unyielding_netherprism*20+trinket.1.is.cursed_stone_idol*16,value_else=trinket.1.proc.any_dps.duration,condition=trinket.1.is.treacherous_transmitter|trinket.1.is.funhouse_lens|trinket.1.is.signet_of_the_priory|trinket.1.is.unyielding_netherprism|trinket.1.is.cursed_stone_idol" );
  precombat->add_action( "variable,name=trinket_2_duration,op=setif,value=trinket.2.is.treacherous_transmitter*15+trinket.2.is.funhouse_lens*15+trinket.2.is.signet_of_the_priory*20+trinket.2.is.unyielding_netherprism*20+trinket.2.is.cursed_stone_idol*16,value_else=trinket.2.proc.any_dps.duration,condition=trinket.2.is.treacherous_transmitter|trinket.2.is.funhouse_lens|trinket.2.is.signet_of_the_priory|trinket.2.is.unyielding_netherprism|trinket.2.is.cursed_stone_idol" );
  precombat->add_action( "variable,name=trinket_1_high_value,op=setif,value=2,value_else=1,condition=trinket.1.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_2_high_value,op=setif,value=2,value_else=1,condition=trinket.2.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_1_buffs&(talent.apocalypse&trinket.1.cooldown.duration%%cooldown.apocalypse.duration=0|talent.dark_transformation&trinket.1.cooldown.duration%%cooldown.dark_transformation.duration=0)|trinket.1.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=variable.trinket_2_buffs&(talent.apocalypse&trinket.2.cooldown.duration%%cooldown.apocalypse.duration=0|talent.dark_transformation&trinket.2.cooldown.duration%%cooldown.dark_transformation.duration=0)|trinket.2.is.treacherous_transmitter" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&variable.trinket_2_buffs&(trinket.2.has_cooldown|!trinket.1.has_cooldown)|variable.trinket_2_buffs&((trinket.2.cooldown.duration%variable.trinket_2_duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync)*(variable.trinket_2_high_value)*(1+((trinket.2.ilvl-trinket.1.ilvl)%100)))>((trinket.1.cooldown.duration%variable.trinket_1_duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync)*(variable.trinket_1_high_value)*(1+((trinket.1.ilvl-trinket.2.ilvl)%100)))" );
  precombat->add_action( "variable,name=damage_trinket_priority,op=setif,value=2,value_else=1,condition=!variable.trinket_1_buffs&!variable.trinket_2_buffs&trinket.2.ilvl>=trinket.1.ilvl" );

  default_->add_action( "auto_attack" );
  default_->add_action( "call_action_list,name=variables", "Call Action Lists" );
  default_->add_action( "call_action_list,name=san_trinkets,if=talent.vampiric_strike" );
  default_->add_action( "call_action_list,name=trinkets,if=!talent.vampiric_strike" );
  default_->add_action( "call_action_list,name=racials" );
  default_->add_action( "call_action_list,name=cds_shared" );
  default_->add_action( "call_action_list,name=cds_aoe_san,if=talent.vampiric_strike&active_enemies>=3" );
  default_->add_action( "call_action_list,name=cds_aoe,if=!talent.vampiric_strike&active_enemies>=2" );
  default_->add_action( "call_action_list,name=cds_cleave_san,if=talent.vampiric_strike&active_enemies=2" );
  default_->add_action( "call_action_list,name=cds_san,if=talent.vampiric_strike&active_enemies=1" );
  default_->add_action( "call_action_list,name=cds,if=!talent.vampiric_strike&active_enemies=1" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies=2" );
  default_->add_action( "call_action_list,name=aoe_setup,if=active_enemies>=3&cooldown.any_dnd.remains<7&!buff.legion_of_souls.up&(buff.death_and_decay.remains<3|death_knight.fwounded_targets<(active_enemies*0.5))" );
  default_->add_action( "call_action_list,name=aoe_burst,if=active_enemies>=3&(death_and_decay.ticking|buff.death_and_decay.up&(death_knight.fwounded_targets>=(active_enemies*0.5)|talent.vampiric_strike&active_enemies<16|talent.desecrate&death_knight.fwounded_targets=active_enemies&talent.bursting_sores))" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>=3&!buff.death_and_decay.up" );
  default_->add_action( "run_action_list,name=san_fishing,if=active_enemies=1&talent.gift_of_the_sanlayn&!cooldown.dark_transformation.ready&!buff.gift_of_the_sanlayn.up&buff.essence_of_the_blood_queen.remains<cooldown.dark_transformation.remains+3" );
  default_->add_action( "call_action_list,name=san_st,if=active_enemies=1&talent.vampiric_strike" );
  default_->add_action( "call_action_list,name=st,if=active_enemies=1&!talent.vampiric_strike" );

  aoe->add_action( "festering_strike,if=buff.festering_scythe.react", "AoE" );
  aoe->add_action( "death_coil,target_if=min:debuff.rotten_touch.remains*(buff.sudden_doom.react&talent.rotten_touch),if=rune<4&active_enemies<variable.epidemic_targets&buff.gift_of_the_sanlayn.up&gcd<=1.0&(!raid_event.adds.exists&fight_remains>buff.dark_transformation.remains*2|raid_event.adds.exists&raid_event.adds.remains>buff.dark_transformation.remains*2)" );
  aoe->add_action( "epidemic,if=rune<4&active_enemies>variable.epidemic_targets&buff.gift_of_the_sanlayn.up&gcd<=1.0&(!raid_event.adds.exists&fight_remains>buff.dark_transformation.remains*2|raid_event.adds.exists&raid_event.adds.remains>buff.dark_transformation.remains*2)" );
  aoe->add_action( "scourge_strike,target_if=max:debuff.festering_wound.stack,if=debuff.festering_wound.stack>=1&buff.death_and_decay.up&talent.bursting_sores&cooldown.apocalypse.remains>variable.apoc_timing" );
  aoe->add_action( "death_coil,target_if=min:debuff.rotten_touch.remains*(buff.sudden_doom.react&talent.rotten_touch),if=!variable.pooling_runic_power&active_enemies<variable.epidemic_targets" );
  aoe->add_action( "epidemic,if=!variable.pooling_runic_power" );
  aoe->add_action( "scourge_strike,target_if=debuff.chains_of_ice_trollbane_slow.up" );
  aoe->add_action( "festering_strike,target_if=max:debuff.festering_wound.stack,if=cooldown.apocalypse.remains<variable.apoc_timing|buff.festering_scythe.react" );
  aoe->add_action( "festering_strike,target_if=min:debuff.festering_wound.stack,if=debuff.festering_wound.stack<2" );
  aoe->add_action( "scourge_strike,target_if=max:debuff.festering_wound.stack,if=debuff.festering_wound.stack>=1&cooldown.apocalypse.remains>gcd|buff.vampiric_strike.react&dot.virulent_plague.ticking" );

  aoe_burst->add_action( "festering_strike,if=buff.festering_scythe.react", "AoE Burst" );
  aoe_burst->add_action( "death_and_decay,if=death_knight.fwounded_targets=active_enemies&talent.desecrate&(talent.festering_scythe&death_knight.fwounded_targets=0&buff.festering_scythe_stacks.stack<10&!buff.festering_scythe.up|!talent.festering_scythe)&(raid_event.adds.exists&raid_event.adds.remains>6&(raid_event.adds.remains<10|!buff.death_and_decay.up)|!raid_event.adds.exists|fight_remains>6&fight_remains<10)" );
  aoe_burst->add_action( "death_coil,target_if=min:debuff.rotten_touch.remains*(buff.sudden_doom.react&talent.rotten_touch),if=!buff.vampiric_strike.react&active_enemies<variable.epidemic_targets&(!talent.bursting_sores|talent.bursting_sores&buff.sudden_doom.react&death_knight.fwounded_targets<active_enemies*0.4|buff.sudden_doom.react&(talent.doomed_bidding&talent.menacing_magus|talent.rotten_touch|debuff.death_rot.remains<gcd)|rune<2)|(rune<4|active_enemies<4|raid_event.pull.has_boss)&active_enemies<variable.epidemic_targets&buff.gift_of_the_sanlayn.up&gcd<=1.0&(!raid_event.adds.exists&fight_remains>buff.dark_transformation.remains*2|raid_event.adds.exists&raid_event.adds.remains>buff.dark_transformation.remains*2)" );
  aoe_burst->add_action( "epidemic,if=!buff.vampiric_strike.react&(!talent.bursting_sores|talent.bursting_sores&buff.sudden_doom.react&death_knight.fwounded_targets<active_enemies*0.4|buff.sudden_doom.react&(buff.a_feast_of_souls.up|debuff.death_rot.remains<gcd|debuff.death_rot.stack<10)|rune<2)|(rune<4|raid_event.pull.has_boss)&active_enemies>=variable.epidemic_targets&buff.gift_of_the_sanlayn.up&gcd<=1.0&(!raid_event.adds.exists&fight_remains>buff.dark_transformation.remains*2|raid_event.adds.exists&raid_event.adds.remains>buff.dark_transformation.remains*2)" );
  aoe_burst->add_action( "scourge_strike,target_if=debuff.chains_of_ice_trollbane_slow.up" );
  aoe_burst->add_action( "scourge_strike,target_if=max:debuff.festering_wound.stack,if=debuff.festering_wound.stack>=1|buff.vampiric_strike.react|buff.death_and_decay.up" );
  aoe_burst->add_action( "death_coil,target_if=min:debuff.rotten_touch.remains*(buff.sudden_doom.react&talent.rotten_touch),if=active_enemies<variable.epidemic_targets" );
  aoe_burst->add_action( "epidemic,if=variable.epidemic_targets<=active_enemies" );
  aoe_burst->add_action( "festering_strike,target_if=min:debuff.festering_wound.stack,if=debuff.festering_wound.stack<=2" );
  aoe_burst->add_action( "scourge_strike,target_if=max:debuff.festering_wound.stack" );

  aoe_setup->add_action( "festering_strike,if=buff.festering_scythe.react", "AoE Setup" );
  aoe_setup->add_action( "any_dnd,if=!death_and_decay.ticking&(death_knight.fwounded_targets=active_enemies&(rune>3|runic_power<30)|talent.desecrate&(talent.festering_scythe&death_knight.fwounded_targets=0&buff.festering_scythe_stacks.stack<10&!buff.festering_scythe.up|!talent.festering_scythe))" );
  aoe_setup->add_action( "festering_strike,target_if=min:debuff.festering_wound.stack,if=death_knight.fwounded_targets=0&cooldown.apocalypse.remains<gcd&(cooldown.dark_transformation.remains&cooldown.unholy_assault.remains|cooldown.unholy_assault.remains|!talent.unholy_assault)" );
  aoe_setup->add_action( "scourge_strike,target_if=debuff.chains_of_ice_trollbane_slow.up" );
  aoe_setup->add_action( "death_coil,target_if=min:debuff.rotten_touch.remains*(buff.sudden_doom.react&talent.rotten_touch),if=!variable.pooling_runic_power&active_enemies<variable.epidemic_targets&rune<4" );
  aoe_setup->add_action( "epidemic,if=!variable.pooling_runic_power&variable.epidemic_targets<=active_enemies&rune<4" );
  aoe_setup->add_action( "any_dnd,if=!buff.death_and_decay.up&(!talent.bursting_sores|death_knight.fwounded_targets=active_enemies|death_knight.fwounded_targets>=8|raid_event.adds.exists&raid_event.adds.remains<=11&raid_event.adds.remains>5|!buff.death_and_decay.up&talent.defile)" );
  aoe_setup->add_action( "death_coil,target_if=min:debuff.rotten_touch.remains*(buff.sudden_doom.react&talent.rotten_touch),if=!variable.pooling_runic_power&active_enemies<variable.epidemic_targets&(buff.sudden_doom.react|death_knight.fwounded_targets=active_enemies|death_knight.fwounded_targets>=8)" );
  aoe_setup->add_action( "epidemic,if=!variable.pooling_runic_power&variable.epidemic_targets<=active_enemies&(buff.sudden_doom.react|death_knight.fwounded_targets=active_enemies|death_knight.fwounded_targets>=8)" );
  aoe_setup->add_action( "death_coil,target_if=min:debuff.rotten_touch.remains*(buff.sudden_doom.react&talent.rotten_touch),if=!variable.pooling_runic_power&active_enemies<variable.epidemic_targets" );
  aoe_setup->add_action( "epidemic,if=!variable.pooling_runic_power" );
  aoe_setup->add_action( "festering_strike,target_if=min:debuff.festering_wound.stack,if=death_knight.fwounded_targets<8&!death_knight.fwounded_targets=active_enemies" );
  aoe_setup->add_action( "scourge_strike,target_if=max:debuff.festering_wound.stack,if=buff.vampiric_strike.react" );

  cds->add_action( "dark_transformation,if=variable.st_planning|fight_remains<20", "Non-Sanlayn CDs" );
  cds->add_action( "unholy_assault,if=variable.st_planning&(cooldown.apocalypse.remains<gcd*1.5|!talent.apocalypse|active_enemies>=2&buff.dark_transformation.up)|fight_remains<20" );
  cds->add_action( "apocalypse,if=variable.st_planning|fight_remains<20" );
  cds->add_action( "outbreak,target_if=target.time_to_die>dot.virulent_plague.remains&dot.virulent_plague.ticks_remain<5,if=(dot.virulent_plague.refreshable|talent.superstrain&(dot.frost_fever.refreshable|dot.blood_plague.refreshable))&(!talent.unholy_blight|talent.plaguebringer)&(!talent.raise_abomination|!pet.abomination.active&talent.raise_abomination&cooldown.raise_abomination.remains>dot.virulent_plague.ticks_remain*3)" );

  cds_aoe->add_action( "unholy_assault,target_if=min:debuff.festering_wound.stack,if=variable.adds_remain", "Non-Sanlayn CDs AoE" );
  cds_aoe->add_action( "dark_transformation,if=variable.adds_remain&(death_and_decay.ticking|cooldown.death_and_decay.remains<3)" );
  cds_aoe->add_action( "apocalypse,target_if=max:debuff.festering_wound.stack,if=variable.adds_remain&(death_and_decay.ticking|cooldown.death_and_decay.remains<3|rune<3|set_bonus.tww3_rider_of_the_apocalypse_2pc)" );
  cds_aoe->add_action( "outbreak,if=dot.virulent_plague.ticks_remain<5&dot.virulent_plague.refreshable&(!talent.unholy_blight|talent.unholy_blight&cooldown.dark_transformation.remains)&(!talent.raise_abomination|talent.raise_abomination&cooldown.raise_abomination.remains)" );

  cds_aoe_san->add_action( "dark_transformation,if=variable.adds_remain&(buff.death_and_decay.up|active_enemies<=3)", "Sanlayn CDs AoE" );
  cds_aoe_san->add_action( "unholy_assault,target_if=min:debuff.festering_wound.stack,if=variable.adds_remain&buff.dark_transformation.up&buff.dark_transformation.remains<12" );
  cds_aoe_san->add_action( "apocalypse,target_if=min:debuff.festering_wound.stack,if=variable.adds_remain&(buff.death_and_decay.up|active_enemies<=3|rune<3)" );
  cds_aoe_san->add_action( "outbreak,if=(dot.virulent_plague.ticks_remain<5|set_bonus.tww2_4pc&talent.superstrain&dot.frost_fever.ticks_remain<5&!pet.abomination.active)&(talent.unholy_blight&!cooldown.dark_transformation.ready|!talent.unholy_blight)&(dot.virulent_plague.refreshable|talent.morbidity&!buff.gift_of_the_sanlayn.up&talent.superstrain&dot.frost_fever.refreshable&dot.blood_plague.refreshable)&(!dot.virulent_plague.ticking&variable.epidemic_targets<active_enemies|(!talent.unholy_blight|talent.unholy_blight&cooldown.dark_transformation.remains>5)&(!talent.raise_abomination|talent.raise_abomination&cooldown.raise_abomination.remains>5))|buff.visceral_strength_unholy.up" );

  cds_cleave_san->add_action( "dark_transformation", "Sanlayn CDs Cleave" );
  cds_cleave_san->add_action( "apocalypse,target_if=max:debuff.festering_wound.stack" );
  cds_cleave_san->add_action( "unholy_assault,target_if=min:debuff.festering_wound.stack,if=buff.dark_transformation.up&buff.dark_transformation.remains<12|fight_remains<20|raid_event.adds.exists&raid_event.adds.remains<20" );
  cds_cleave_san->add_action( "outbreak,target_if=target.time_to_die>dot.virulent_plague.remains&dot.virulent_plague.ticks_remain<5,if=(dot.virulent_plague.refreshable|talent.morbidity&buff.infliction_of_sorrow.up&talent.superstrain&dot.frost_fever.refreshable&dot.blood_plague.refreshable)&(!talent.unholy_blight|talent.unholy_blight&cooldown.dark_transformation.remains>6)&(!talent.raise_abomination|talent.raise_abomination&cooldown.raise_abomination.remains>6)|buff.visceral_strength_unholy.up" );

  cds_san->add_action( "dark_transformation,if=variable.st_planning|fight_remains<20", "Sanlayn CDs ST" );
  cds_san->add_action( "apocalypse,if=variable.st_planning|fight_remains<20" );
  cds_san->add_action( "unholy_assault,if=variable.st_planning&(buff.dark_transformation.up&buff.dark_transformation.remains<12)|fight_remains<20" );
  cds_san->add_action( "outbreak,target_if=target.time_to_die>dot.virulent_plague.remains&dot.virulent_plague.ticks_remain<5,if=(dot.virulent_plague.refreshable|talent.morbidity&buff.infliction_of_sorrow.up&talent.superstrain&dot.frost_fever.refreshable&dot.blood_plague.refreshable)&(!talent.unholy_blight|talent.unholy_blight&cooldown.dark_transformation.remains>6)&(!talent.raise_abomination|talent.raise_abomination&cooldown.raise_abomination.remains>6)|buff.visceral_strength_unholy.up" );
  cds_san->add_action( "outbreak,target_if=target.time_to_die>dot.frost_fever.remains&dot.frost_fever.ticks_remain<5,if=talent.superstrain&set_bonus.tww2_4pc&dot.frost_fever.refreshable&(!talent.unholy_blight|talent.unholy_blight&cooldown.dark_transformation.remains>6)&(!talent.raise_abomination|talent.raise_abomination&cooldown.raise_abomination.remains>6)" );

  cds_shared->add_action( "potion,if=(variable.st_planning|variable.adds_remain)&(!talent.summon_gargoyle|cooldown.summon_gargoyle.remains>60)&(buff.dark_transformation.up&30>=buff.dark_transformation.remains|!talent.vampiric_strike&pet.army_ghoul.active&pet.army_ghoul.remains<=30|!talent.vampiric_strike&pet.apoc_ghoul.active&pet.apoc_ghoul.remains<=30|!talent.vampiric_strike&pet.abomination.active&pet.abomination.remains<=30)|fight_remains<=30", "Shared CDs" );
  cds_shared->add_action( "invoke_external_buff,name=power_infusion,if=active_enemies>=1&(variable.st_planning|variable.adds_remain)&(pet.gargoyle.active&pet.gargoyle.remains<=22|!talent.summon_gargoyle&talent.army_of_the_dead&(talent.raise_abomination&pet.abomination.active&pet.abomination.remains<18|!talent.raise_abomination&pet.army_ghoul.active&pet.army_ghoul.remains<=18)|!talent.summon_gargoyle&!talent.army_of_the_dead&buff.dark_transformation.up|!talent.summon_gargoyle&buff.dark_transformation.up|!pet.gargoyle.active&cooldown.summon_gargoyle.remains+10>cooldown.invoke_external_buff_power_infusion.duration|active_enemies>=3&(buff.dark_transformation.up|death_and_decay.ticking))" );
  cds_shared->add_action( "army_of_the_dead,if=(variable.st_planning|variable.adds_remain)&(talent.commander_of_the_dead&cooldown.dark_transformation.remains<5|!talent.commander_of_the_dead&active_enemies>=1)|fight_remains<35" );
  cds_shared->add_action( "raise_abomination,if=(variable.st_planning|variable.adds_remain)&(!talent.vampiric_strike|(pet.apoc_ghoul.active|!talent.apocalypse))|fight_remains<30" );
  cds_shared->add_action( "legion_of_souls,if=(variable.st_planning|variable.adds_remain)&(death_knight.fwounded_targets<active_enemies|(cooldown.apocalypse.remains<3|cooldown.dark_transformation.remains<3))" );
  cds_shared->add_action( "summon_gargoyle,use_off_gcd=1,if=(variable.st_planning|variable.adds_remain)&(buff.commander_of_the_dead.up|!talent.commander_of_the_dead&active_enemies>=1)|fight_remains<25" );
  cds_shared->add_action( "antimagic_shell,if=death_knight.ams_absorb_percent>0&runic_power<30&rune<2" );
  cds_shared->add_action( "desecrate,if=active_enemies>=2&((!raid_event.adds.exists&fight_remains<6|raid_event.adds.exists&raid_event.adds.remains<6)|(!talent.festering_scythe|buff.festering_scythe_stacks.stack<active_enemies&!buff.festering_scythe.up)&(active_enemies>1&death_knight.fwounded_targets<active_enemies|death_knight.fwounded_targets=active_enemies|death_knight.fwounded_targets=0&talent.festering_scythe&!buff.festering_scythe.up&buff.festering_scythe_stacks.stack<10))" );

  cleave->add_action( "any_dnd,if=!death_and_decay.ticking&variable.adds_remain|talent.gift_of_the_sanlayn", "Cleave" );
  cleave->add_action( "death_coil,if=!variable.pooling_runic_power&talent.improved_death_coil" );
  cleave->add_action( "scourge_strike,if=buff.vampiric_strike.react" );
  cleave->add_action( "death_coil,if=!variable.pooling_runic_power&!talent.improved_death_coil" );
  cleave->add_action( "festering_strike,target_if=min:debuff.festering_wound.stack,if=!buff.vampiric_strike.react&!variable.pop_wounds&debuff.festering_wound.stack<2|buff.festering_scythe.react" );
  cleave->add_action( "festering_strike,target_if=max:debuff.festering_wound.stack,if=!buff.vampiric_strike.react&cooldown.apocalypse.remains<variable.apoc_timing&debuff.festering_wound.stack<1" );
  cleave->add_action( "scourge_strike,if=variable.pop_wounds" );

  racials->add_action( "arcane_torrent,if=runic_power<20&rune<2", "Racials" );
  racials->add_action( "blood_fury,if=(buff.blood_fury.duration+3>=pet.gargoyle.remains&pet.gargoyle.active)|(!talent.summon_gargoyle|cooldown.summon_gargoyle.remains>60)&(pet.army_ghoul.active&pet.army_ghoul.remains<=buff.blood_fury.duration+3|pet.apoc_ghoul.active&pet.apoc_ghoul.remains<=buff.blood_fury.duration+3|active_enemies>=2&death_and_decay.ticking)|fight_remains<=buff.blood_fury.duration+3" );
  racials->add_action( "berserking,if=(buff.berserking.duration+3>=pet.gargoyle.remains&pet.gargoyle.active)|(!talent.summon_gargoyle|cooldown.summon_gargoyle.remains>60)&(pet.army_ghoul.active&pet.army_ghoul.remains<=buff.berserking.duration+3|pet.apoc_ghoul.active&pet.apoc_ghoul.remains<=buff.berserking.duration+3|active_enemies>=2&death_and_decay.ticking)|fight_remains<=buff.berserking.duration+3" );
  racials->add_action( "lights_judgment,if=buff.unholy_strength.up&(!talent.festermight|buff.festermight.remains<target.time_to_die|buff.unholy_strength.remains<target.time_to_die)" );
  racials->add_action( "ancestral_call,if=(18>=pet.gargoyle.remains&pet.gargoyle.active)|(!talent.summon_gargoyle|cooldown.summon_gargoyle.remains>60)&(pet.army_ghoul.active&pet.army_ghoul.remains<=18|pet.apoc_ghoul.active&pet.apoc_ghoul.remains<=18|active_enemies>=2&death_and_decay.ticking)|fight_remains<=18" );
  racials->add_action( "arcane_pulse,if=active_enemies>=2|(rune.deficit>=5&runic_power.deficit>=60)" );
  racials->add_action( "fireblood,if=(buff.fireblood.duration+3>=pet.gargoyle.remains&pet.gargoyle.active)|(!talent.summon_gargoyle|cooldown.summon_gargoyle.remains>60)&(pet.army_ghoul.active&pet.army_ghoul.remains<=buff.fireblood.duration+3|pet.apoc_ghoul.active&pet.apoc_ghoul.remains<=buff.fireblood.duration+3|active_enemies>=2&death_and_decay.ticking)|fight_remains<=buff.fireblood.duration+3" );
  racials->add_action( "bag_of_tricks,if=active_enemies=1&(buff.unholy_strength.up|fight_remains<5)" );

  san_fishing->add_action( "antimagic_shell,if=death_knight.ams_absorb_percent>0&runic_power<40", "San'layn Fishing" );
  san_fishing->add_action( "scourge_strike,if=buff.infliction_of_sorrow.up" );
  san_fishing->add_action( "any_dnd,if=!buff.death_and_decay.up&!buff.vampiric_strike.react" );
  san_fishing->add_action( "death_coil,if=buff.sudden_doom.react&talent.doomed_bidding|set_bonus.tww2_4pc&buff.essence_of_the_blood_queen.at_max_stacks&talent.frenzied_bloodthirst&!buff.vampiric_strike.react" );
  san_fishing->add_action( "soul_reaper,if=target.health.pct<=35&fight_remains>5" );
  san_fishing->add_action( "death_coil,if=!buff.vampiric_strike.react" );
  san_fishing->add_action( "scourge_strike,if=(debuff.festering_wound.stack>=3-pet.abomination.active&cooldown.apocalypse.remains>variable.apoc_timing)|buff.vampiric_strike.react" );
  san_fishing->add_action( "festering_strike,if=debuff.festering_wound.stack<3-pet.abomination.active" );

  san_st->add_action( "scourge_strike,if=buff.infliction_of_sorrow.up", "San'layn Single Target" );
  san_st->add_action( "festering_strike,if=buff.festering_scythe.react&(!raid_event.adds.exists|!raid_event.adds.in|raid_event.adds.in>11|raid_event.pull.has_boss&raid_event.adds.in>11)" );
  san_st->add_action( "death_coil,if=buff.sudden_doom.react&buff.gift_of_the_sanlayn.remains&(talent.doomed_bidding|talent.rotten_touch)|rune<3&!buff.runic_corruption.up|set_bonus.tww2_4pc&runic_power>80|buff.gift_of_the_sanlayn.up&buff.essence_of_the_blood_queen.at_max_stacks&talent.frenzied_bloodthirst&set_bonus.tww2_4pc&buff.winning_streak_unholy.at_max_stacks&rune<=3&buff.essence_of_the_blood_queen.remains>3" );
  san_st->add_action( "scourge_strike,if=buff.vampiric_strike.react&debuff.festering_wound.stack>=1|buff.gift_of_the_sanlayn.up|talent.gift_of_the_sanlayn&buff.dark_transformation.up&buff.dark_transformation.remains<gcd" );
  san_st->add_action( "soul_reaper,if=target.health.pct<=35&!buff.gift_of_the_sanlayn.up&fight_remains>5" );
  san_st->add_action( "festering_strike,if=(debuff.festering_wound.stack=0&cooldown.apocalypse.remains<variable.apoc_timing)|!buff.dark_transformation.up&cooldown.dark_transformation.remains<10&debuff.festering_wound.stack<=3&(rune>4|runic_power<80)|(talent.gift_of_the_sanlayn&!buff.gift_of_the_sanlayn.up|!talent.gift_of_the_sanlayn)&debuff.festering_wound.stack<=1" );
  san_st->add_action( "scourge_strike,if=(!talent.apocalypse|cooldown.apocalypse.remains>variable.apoc_timing)&(cooldown.dark_transformation.remains>5&debuff.festering_wound.stack>=3-pet.abomination.active|buff.vampiric_strike.react)" );
  san_st->add_action( "death_coil,if=!variable.pooling_runic_power&debuff.death_rot.remains<gcd|(buff.sudden_doom.react&debuff.festering_wound.stack>=1|rune<2)" );
  san_st->add_action( "scourge_strike,if=debuff.festering_wound.stack>4" );
  san_st->add_action( "death_coil,if=!variable.pooling_runic_power" );
  san_st->add_action( "scourge_strike,if=(!talent.apocalypse|cooldown.apocalypse.remains>variable.apoc_timing)&rune>=4" );

  san_trinkets->add_action( "do_treacherous_transmitter_task,use_off_gcd=1,if=buff.errant_manaforge_emission.up&buff.dark_transformation.up&buff.errant_manaforge_emission.remains<2|buff.cryptic_instructions.up&buff.dark_transformation.up&buff.cryptic_instructions.remains<2|buff.realigning_nexus_convergence_divergence.up&buff.dark_transformation.up&buff.realigning_nexus_convergence_divergence.remains<2", "San'layn Trinkets" );
  san_trinkets->add_action( "use_item,name=treacherous_transmitter,if=(variable.adds_remain|variable.st_planning)&cooldown.dark_transformation.remains<10" );
  san_trinkets->add_action( "use_item,name=cursed_stone_idol,if=pet.apoc_ghoul.active|!talent.apocalypse&buff.dark_transformation.up|cooldown.apocalypse.ready" );
  san_trinkets->add_action( "use_item,slot=trinket1,if=(buff.latent_energy.stack>=8|!trinket.1.is.unyielding_netherprism)&variable.trinket_1_buffs&(buff.dark_transformation.up&buff.dark_transformation.remains<variable.trinket_1_duration*0.73&(variable.trinket_priority=1|trinket.2.cooldown.remains|!trinket.2.has_cooldown))|variable.trinket_1_duration>=fight_remains" );
  san_trinkets->add_action( "use_item,slot=trinket2,if=(buff.latent_energy.stack>=8|!trinket.2.is.unyielding_netherprism)&variable.trinket_2_buffs&(buff.dark_transformation.up&buff.dark_transformation.remains<variable.trinket_2_duration*0.73&(variable.trinket_priority=2|trinket.1.cooldown.remains|!trinket.1.has_cooldown))|variable.trinket_2_duration>=fight_remains" );
  san_trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(trinket.1.cast_time>0&!buff.gift_of_the_sanlayn.up|!trinket.1.cast_time>0)&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains|!trinket.2.has_cooldown|!talent.summon_gargoyle&!talent.army_of_the_dead&!talent.raise_abomination|!talent.summon_gargoyle&talent.army_of_the_dead&(!talent.raise_abomination&cooldown.army_of_the_dead.remains>20|talent.raise_abomination&cooldown.raise_abomination.remains>20)|!talent.summon_gargoyle&!talent.army_of_the_dead&!talent.raise_abomination&cooldown.dark_transformation.remains>20|talent.summon_gargoyle&cooldown.summon_gargoyle.remains>20&!pet.gargoyle.active)|fight_remains<15" );
  san_trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(trinket.2.cast_time>0&!buff.gift_of_the_sanlayn.up|!trinket.2.cast_time>0)&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains|!trinket.1.has_cooldown|!talent.summon_gargoyle&!talent.army_of_the_dead&!talent.raise_abomination|!talent.summon_gargoyle&talent.army_of_the_dead&(!talent.raise_abomination&cooldown.army_of_the_dead.remains>20|talent.raise_abomination&cooldown.raise_abomination.remains>20)|!talent.summon_gargoyle&!talent.army_of_the_dead&!talent.raise_abomination&cooldown.dark_transformation.remains>20|talent.summon_gargoyle&cooldown.summon_gargoyle.remains>20&!pet.gargoyle.active)|fight_remains<15" );
  san_trinkets->add_action( "use_item,slot=main_hand,if=(!variable.trinket_1_buffs&!variable.trinket_2_buffs|trinket.1.cooldown.remains>20&!variable.trinket_2_buffs|trinket.2.cooldown.remains>20&!variable.trinket_1_buffs|trinket.1.cooldown.remains>20&trinket.2.cooldown.remains>20)&(buff.dark_transformation.up&buff.dark_transformation.remains>10)&(!talent.raise_abomination&!talent.army_of_the_dead|!talent.raise_abomination&talent.army_of_the_dead&pet.army_ghoul.active|talent.raise_abomination&pet.abomination.active|(variable.trinket_1_buffs|variable.trinket_2_buffs|fight_remains<15))" );

  st->add_action( "soul_reaper,if=target.health.pct<=35&fight_remains>5", "Non-San'layn Single Target" );
  st->add_action( "scourge_strike,if=debuff.chains_of_ice_trollbane_slow.up" );
  st->add_action( "death_coil,if=!variable.pooling_runic_power&variable.spend_rp|fight_remains<10" );
  st->add_action( "festering_strike,if=debuff.festering_wound.stack<4&(!variable.pop_wounds|buff.festering_scythe.react)" );
  st->add_action( "scourge_strike,if=variable.pop_wounds" );
  st->add_action( "death_coil,if=!variable.pooling_runic_power" );
  st->add_action( "scourge_strike,if=!variable.pop_wounds&debuff.festering_wound.stack>=4" );

  trinkets->add_action( "do_treacherous_transmitter_task,use_off_gcd=1,if=buff.errant_manaforge_emission.up&(pet.apoc_ghoul.active|!talent.apocalypse&buff.dark_transformation.up)|buff.cryptic_instructions.up&(pet.apoc_ghoul.active|!talent.apocalypse&buff.dark_transformation.up)|buff.realigning_nexus_convergence_divergence.up&(pet.apoc_ghoul.active|!talent.apocalypse&buff.dark_transformation.up)", "Non-San'layn Trinkets" );
  trinkets->add_action( "use_item,name=treacherous_transmitter,if=(variable.adds_remain|variable.st_planning)&cooldown.dark_transformation.remains<10" );
  trinkets->add_action( "use_item,slot=trinket1,if=(buff.latent_energy.stack>=8|!trinket.1.is.unyielding_netherprism)&variable.trinket_1_buffs&(variable.trinket_priority=1|!trinket.2.has_cooldown|trinket.2.cooldown.remains>20)&(!talent.apocalypse&buff.dark_transformation.up|pet.apoc_ghoul.active&pet.apoc_ghoul.remains<=variable.trinket_1_duration&pet.apoc_ghoul.remains>5)&(talent.army_of_the_dead&!talent.raise_abomination&pet.army_ghoul.active&pet.army_ghoul.remains>10|talent.raise_abomination&pet.abomination.active&pet.abomination.remains>10|talent.legion_of_souls|!talent.raise_abomination&!talent.apocalypse&buff.dark_transformation.up|variable.trinket_2_buffs&trinket.2.cooldown.remains)|fight_remains<=variable.trinket_1_duration" );
  trinkets->add_action( "use_item,slot=trinket2,if=(buff.latent_energy.stack>=8|!trinket.2.is.unyielding_netherprism)&variable.trinket_2_buffs&(variable.trinket_priority=2|!trinket.1.has_cooldown|trinket.1.cooldown.remains>20)&(!talent.apocalypse&buff.dark_transformation.up|pet.apoc_ghoul.active&pet.apoc_ghoul.remains<=variable.trinket_2_duration&pet.apoc_ghoul.remains>5)&(talent.army_of_the_dead&!talent.raise_abomination&pet.army_ghoul.active&pet.army_ghoul.remains>10|talent.raise_abomination&pet.abomination.active&pet.abomination.remains>10|talent.legion_of_souls|!talent.raise_abomination&!talent.apocalypse&buff.dark_transformation.up|variable.trinket_1_buffs&trinket.1.cooldown.remains)|fight_remains<=variable.trinket_2_duration" );
  trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(variable.damage_trinket_priority=1|trinket.2.cooldown.remains|!trinket.2.has_cooldown|!talent.summon_gargoyle&!talent.army_of_the_dead&!talent.raise_abomination|!talent.summon_gargoyle&talent.army_of_the_dead&(!talent.raise_abomination&cooldown.army_of_the_dead.remains>20|talent.raise_abomination&cooldown.raise_abomination.remains>20)|!talent.summon_gargoyle&!talent.army_of_the_dead&!talent.raise_abomination&cooldown.dark_transformation.remains>20|talent.summon_gargoyle&cooldown.summon_gargoyle.remains>20&!pet.gargoyle.active)|fight_remains<15" );
  trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(variable.damage_trinket_priority=2|trinket.1.cooldown.remains|!trinket.1.has_cooldown|!talent.summon_gargoyle&!talent.army_of_the_dead&!talent.raise_abomination|!talent.summon_gargoyle&talent.army_of_the_dead&(!talent.raise_abomination&cooldown.army_of_the_dead.remains>20|talent.raise_abomination&cooldown.raise_abomination.remains>20)|!talent.summon_gargoyle&!talent.army_of_the_dead&!talent.raise_abomination&cooldown.dark_transformation.remains>20|talent.summon_gargoyle&cooldown.summon_gargoyle.remains>20&!pet.gargoyle.active)|fight_remains<15" );
  trinkets->add_action( "use_item,slot=main_hand,if=(!variable.trinket_1_buffs&!variable.trinket_2_buffs|trinket.1.cooldown.remains&!variable.trinket_2_buffs|trinket.2.cooldown.remains&!variable.trinket_1_buffs|trinket.1.cooldown.remains&trinket.2.cooldown.remains)&(pet.apoc_ghoul.active&pet.apoc_ghoul.remains<=18|!talent.apocalypse&buff.dark_transformation.up)&((trinket.1.cooldown.duration=90|trinket.2.cooldown.duration=90)|!talent.raise_abomination&!talent.army_of_the_dead|!talent.raise_abomination&talent.army_of_the_dead&pet.army_ghoul.active|talent.raise_abomination&pet.abomination.active)" );

  variables->add_action( "variable,name=st_planning,op=setif,value=1,value_else=0,condition=active_enemies=1&(!raid_event.adds.exists|!raid_event.adds.in|raid_event.adds.in>15|raid_event.pull.has_boss&raid_event.adds.in>15)", "Variables" );
  variables->add_action( "variable,name=adds_remain,op=setif,value=1,value_else=0,condition=active_enemies>=2&(!raid_event.adds.exists&fight_remains>6|raid_event.adds.exists&raid_event.adds.remains>6)" );
  variables->add_action( "variable,name=apoc_timing,op=setif,value=5,value_else=2,condition=cooldown.apocalypse.remains<5&debuff.festering_wound.stack<4&cooldown.unholy_assault.remains>5" );
  variables->add_action( "variable,name=pop_wounds,op=setif,value=1,value_else=0,condition=(cooldown.apocalypse.remains>variable.apoc_timing|!talent.apocalypse)&(debuff.festering_wound.stack>=1&cooldown.unholy_assault.remains<20&talent.unholy_assault&variable.st_planning|debuff.rotten_touch.up&debuff.festering_wound.stack>=1|debuff.festering_wound.stack>=4-pet.abomination.active)|fight_remains<5&debuff.festering_wound.stack>=1" );
  variables->add_action( "variable,name=pooling_runic_power,op=setif,value=1,value_else=0,condition=cooldown.summon_gargoyle.remains>5&runic_power<40" );
  variables->add_action( "variable,name=spend_rp,op=setif,value=1,value_else=0,condition=(!talent.rotten_touch|talent.rotten_touch&!debuff.rotten_touch.up|runic_power.deficit<20)&((talent.improved_death_coil&(active_enemies=2|talent.coil_of_devastation)|rune<3|pet.gargoyle.active|buff.sudden_doom.react|!variable.pop_wounds&debuff.festering_wound.stack>=4))" );
  variables->add_action( "variable,name=san_coil_mult,op=setif,value=2,value_else=1,condition=buff.essence_of_the_blood_queen.stack>=4" );
  variables->add_action( "variable,name=epidemic_targets,value=3+talent.improved_death_coil+(talent.frenzied_bloodthirst*variable.san_coil_mult)+(talent.hungering_thirst&talent.harbinger_of_doom&buff.sudden_doom.up)" );
}
//unholy_apl_end

}  // namespace death_knight_apl
