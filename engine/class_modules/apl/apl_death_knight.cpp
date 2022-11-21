#include "class_modules/apl/apl_death_knight.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"
#include "dbc/dbc.hpp"

namespace death_knight_apl {

std::string potion( const player_t* p )
{
  std::string frost_potion = ( p->true_level >= 61 ) ? "elemental_potion_of_ultimate_power_3" : "potion_of_spectral_strength";

  std::string unholy_potion = ( p->true_level >= 61 ) ? "elemental_potion_of_ultimate_power_3" : "potion_of_spectral_strength";

  std::string blood_potion = ( p->true_level >= 61 ) ? "elemental_potion_of_ultimate_power_3" : "potion_of_spectral_strength";

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
  std::string flask_name = ( p->true_level >= 61 ) ? "phial_of_static_empowerment_3" : "spectral_flask_of_power";

  // All specs use a strength flask as default
  return flask_name;
}

std::string food( const player_t* p )
{
  std::string frost_food = ( p->true_level >= 61 ) ? "fated_fortune_cookie" : "feast_of_gluttonous_hedonism";

  std::string unholy_food = ( p->true_level >= 61 ) ? "fated_fortune_cookie" : "feast_of_gluttonous_hedonism";

  std::string blood_food = ( p->true_level >= 60 ) ? "feast_of_gluttonous_hedonism" : "disabled";

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
  return ( p->true_level >= 61 ) ? "draconic" : "veiled";
}

std::string temporary_enchant( const player_t* p )
{
  std::string frost_temporary_enchant =
      ( p->true_level >= 61 ) ? "main_hand:buzzing_rune_3/off_hand:buzzing_rune_3" : "main_hand:shaded_sharpening_stone/off_hand:shaded_sharpening_stone";

  std::string unholy_temporary_enchant = ( p->true_level >= 61 ) ? "main_hand:howling_rune_3" : "main_hand:shaded_sharpening_stone";

  std::string blood_temporary_enchant = ( p->true_level >= 60 ) ? "main_hand:shaded_weightstone" : "disabled";

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
  action_priority_list_t* drw_up = p->get_action_priority_list( "drw_up" );
  action_priority_list_t* racials = p->get_action_priority_list( "racials" );
  action_priority_list_t* standard = p->get_action_priority_list( "standard" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "fleshcraft" );

  default_->add_action( "auto_attack" );
  default_->add_action( "variable,name=death_strike_dump_amount,value=70" );
  default_->add_action( "mind_freeze,if=target.debuff.casting.react", "Interrupt" );
  default_->add_action( "potion,if=buff.dancing_rune_weapon.up", "Since the potion cooldown has changed, we'll sync with DRW" );
  default_->add_action( "use_items" );
  default_->add_action( "raise_dead" );
  default_->add_action( "deaths_caress,if=!buff.bone_shield.up" );
  default_->add_action( "blooddrinker,if=!buff.dancing_rune_weapon.up" );
  default_->add_action( "call_action_list,name=racials" );
  default_->add_action( "sacrificial_pact,if=!buff.dancing_rune_weapon.up&(pet.ghoul.remains<2|target.time_to_die<gcd)", "Attempt to sacrifice the ghoul if we predictably will not do much in the near future" );
  default_->add_action( "blood_tap,if=(rune<=2&rune.time_to_4>gcd&charges_fractional>=1.8)|rune.time_to_3>gcd" );
  default_->add_action( "marrowrend,if=buff.bone_shield.remains<gcd" );
  default_->add_action( "deaths_caress,if=buff.bone_shield.remains<gcd|!buff.bone_shield.up" );
  default_->add_action( "gorefiends_grasp,if=talent.tightening_grasp.enabled" );
  default_->add_action( "deaths_due,if=covenant.night_fae&!death_and_decay.ticking" );
  default_->add_action( "death_and_decay,if=(talent.unholy_ground|talent.sanguine_ground)&cooldown.dancing_rune_weapon.remains<gcd" );
  default_->add_action( "dancing_rune_weapon,if=!buff.dancing_rune_weapon.up" );
  default_->add_action( "run_action_list,name=drw_up,if=buff.dancing_rune_weapon.up" );
  default_->add_action( "call_action_list,name=standard" );

  drw_up->add_action( "tombstone,if=buff.bone_shield.stack>5&rune>=2&runic_power.deficit>=30&runeforge.crimson_rune_weapon" );
  drw_up->add_action( "empower_rune_weapon,if=rune<6&runic_power.deficit>5" );
  drw_up->add_action( "marrowrend,if=(buff.bone_shield.remains<=rune.time_to_3|(buff.bone_shield.stack<2&buff.abomination_limb_talent.up))&runic_power.deficit>20" );
  drw_up->add_action( "deaths_caress,if=buff.bone_shield.remains<=rune.time_to_3&rune<=1" );
  drw_up->add_action( "death_strike,if=buff.coagulopathy.remains<=gcd|buff.icy_talons.remains<=gcd" );
  drw_up->add_action( "soul_reaper,if=active_enemies=1&target.time_to_pct_35<5&target.time_to_die>(dot.soul_reaper.remains+5)" );
  drw_up->add_action( "soul_reaper,target_if=min:dot.soul_reaper.remains,if=target.time_to_pct_35<5&active_enemies>=2&target.time_to_die>(dot.soul_reaper.remains+5)" );
  drw_up->add_action( "deaths_due,if=covenant.night_fae&!death_and_decay.ticking" );
  drw_up->add_action( "death_and_decay,if=!death_and_decay.ticking&(talent.sanguine_ground|talent.unholy_ground)" );
  drw_up->add_action( "blood_boil,if=((charges>=2&rune<=1)|dot.blood_plague.remains<=2)|(spell_targets.blood_boil>5&charges_fractional>=1.1)" );
  drw_up->add_action( "variable,name=heart_strike_rp_drw,value=(25+spell_targets.heart_strike*talent.heartbreaker.enabled*2)" );
  drw_up->add_action( "death_strike,if=runic_power.deficit<=variable.heart_strike_rp_drw" );
  drw_up->add_action( "consumption" );
  drw_up->add_action( "death_and_decay,if=(spell_targets.death_and_decay==3&buff.crimson_scourge.up)|spell_targets.death_and_decay>=4" );
  drw_up->add_action( "heart_strike,if=rune.time_to_2<gcd|runic_power.deficit>=variable.heart_strike_rp_drw" );

  racials->add_action( "blood_fury,if=cooldown.dancing_rune_weapon.ready&(!cooldown.blooddrinker.ready|!talent.blooddrinker.enabled)" );
  racials->add_action( "berserking" );
  racials->add_action( "arcane_pulse,if=active_enemies>=2|rune<1&runic_power.deficit>60" );
  racials->add_action( "lights_judgment,if=buff.unholy_strength.up" );
  racials->add_action( "ancestral_call" );
  racials->add_action( "fireblood" );
  racials->add_action( "bag_of_tricks" );
  racials->add_action( "arcane_torrent,if=runic_power.deficit>20" );

  standard->add_action( "tombstone,if=buff.bone_shield.stack>5&rune>=2&runic_power.deficit>=30" );
  standard->add_action( "abomination_limb_talent,if=buff.bone_shield.stack<6", "Consider adding empower_rune_weapon here, but as it looks, DRW may end up aligning every 2nd drw" );
  standard->add_action( "marrowrend,if=buff.bone_shield.remains<=rune.time_to_3|buff.bone_shield.remains<=(gcd+cooldown.blooddrinker.ready*talent.blooddrinker.enabled*4)|buff.bone_shield.stack<6&runic_power.deficit>20&!(talent.insatiable_blade&cooldown.dancing_rune_weapon.remains<buff.bone_shield.remains)" );
  standard->add_action( "deaths_caress,if=buff.bone_shield.remains<=rune.time_to_3&rune<=1" );
  standard->add_action( "death_strike,if=buff.coagulopathy.remains<=gcd|buff.icy_talons.remains<=gcd" );
  standard->add_action( "deaths_due,if=covenant.night_fae&!death_and_decay.ticking" );
  standard->add_action( "death_and_decay,if=!death_and_decay.ticking&(talent.sanguine_ground|talent.unholy_ground)" );
  standard->add_action( "bonestorm,if=runic_power>=100" );
  standard->add_action( "soul_reaper,if=active_enemies=1&target.time_to_pct_35<5&target.time_to_die>(dot.soul_reaper.remains+5)" );
  standard->add_action( "soul_reaper,target_if=min:dot.soul_reaper.remains,if=target.time_to_pct_35<5&active_enemies>=2&target.time_to_die>(dot.soul_reaper.remains+5)" );
  standard->add_action( "death_strike,if=runic_power.deficit<=variable.death_strike_dump_amount&!(talent.bonestorm.enabled&cooldown.bonestorm.remains<2)" );
  standard->add_action( "blood_boil,if=charges_fractional>=1.8&(buff.hemostasis.stack<=(5-spell_targets.blood_boil)|spell_targets.blood_boil>2)" );
  standard->add_action( "death_and_decay,if=buff.crimson_scourge.up&talent.relish_in_blood.enabled&runic_power.deficit>10" );
  standard->add_action( "variable,name=heart_strike_rp,value=(15+spell_targets.heart_strike*talent.heartbreaker.enabled*2)" );
  standard->add_action( "death_strike,if=(runic_power.deficit<=variable.heart_strike_rp)|target.time_to_die<10" );
  standard->add_action( "death_and_decay,if=spell_targets.death_and_decay>=3" );
  standard->add_action( "heart_strike,if=rune.time_to_4<gcd" );
  standard->add_action( "consumption" );
  standard->add_action( "death_and_decay,if=buff.crimson_scourge.up|talent.rapid_decomposition.enabled" );
  standard->add_action( "blood_boil,if=charges_fractional>=1.1" );
  standard->add_action( "heart_strike,if=(rune>1&(rune.time_to_3<gcd|buff.bone_shield.stack>7))" );
}
//blood_apl_end

//frost_apl_start
void frost( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* breath = p->get_action_priority_list( "breath" );
  action_priority_list_t* breath_oblit = p->get_action_priority_list( "breath_oblit" );
  action_priority_list_t* cold_heart = p->get_action_priority_list( "cold_heart" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* covenants = p->get_action_priority_list( "covenants" );
  action_priority_list_t* obliteration = p->get_action_priority_list( "obliteration" );
  action_priority_list_t* racials = p->get_action_priority_list( "racials" );
  action_priority_list_t* single_target = p->get_action_priority_list( "single_target" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=trinket.1.has_use_buff&(talent.pillar_of_frost&!talent.breath_of_sindragosa&(trinket.1.cooldown.duration%%cooldown.pillar_of_frost.duration=0)|talent.breath_of_sindragosa&(cooldown.breath_of_sindragosa.duration%%trinket.1.cooldown.duration=0))", "Evaluates a trinkets cooldown, divided by pillar of frost, empower rune weapon, or breath of sindragosa's cooldown. If it's value has no remainder return 1, else return 0.5." );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=trinket.2.has_use_buff&(talent.pillar_of_frost&!talent.breath_of_sindragosa&(trinket.2.cooldown.duration%%cooldown.pillar_of_frost.duration=0)|talent.breath_of_sindragosa&(cooldown.breath_of_sindragosa.duration%%trinket.2.cooldown.duration=0))" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!trinket.1.has_use_buff&trinket.2.has_use_buff|trinket.2.has_use_buff&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync))", "Estimates a trinkets value by comparing the cooldown of the trinket, divided by the duration of the buff it provides. Has a strength modifier to give a higher priority to strength trinkets, as well as a modifier for if a trinket will or will not sync with cooldowns." );
  precombat->add_action( "variable,name=rw_buffs,value=talent.gathering_storm|talent.everfrost" );
  precombat->add_action( "variable,name=2h_check,value=main_hand.2h&talent.might_of_the_frozen_wastes" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_buff.strength|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_buff.strength|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit" );

  default_->add_action( "auto_attack" );
  default_->add_action( "variable,name=st_planning,value=active_enemies=1&(raid_event.adds.in>15|!raid_event.adds.exists)", "Prevent specified trinkets being used with automatic lines actions+=/variable,name=specified_trinket,value=" );
  default_->add_action( "variable,name=adds_remain,value=active_enemies>=2&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.remains>5)" );
  default_->add_action( "variable,name=rime_buffs,value=buff.rime.react&(talent.rage_of_the_frozen_champion|talent.avalanche|talent.icebreaker)" );
  default_->add_action( "variable,name=rp_buffs,value=talent.unleashed_frenzy&(buff.unleashed_frenzy.remains<gcd*3|buff.unleashed_frenzy.stack<3)|talent.icy_talons&(buff.icy_talons.remains<gcd*3|buff.icy_talons.stack<3)" );
  default_->add_action( "variable,name=cooldown_check,value=talent.pillar_of_frost&buff.pillar_of_frost.up|!talent.pillar_of_frost&buff.empower_rune_weapon.up|!talent.pillar_of_frost&!talent.empower_rune_weapon" );
  default_->add_action( "variable,name=frostscythe_priority,value=talent.frostscythe&(buff.killing_machine.react|active_enemies>=3)&(!talent.improved_obliterate&!talent.frigid_executioner&!talent.frostreaper&!talent.might_of_the_frozen_wastes|!talent.cleaving_strikes|talent.cleaving_strikes&(active_enemies>6|!death_and_decay.ticking&active_enemies>3))" );
  default_->add_action( "variable,name=oblit_pooling_time,op=setif,value=((cooldown.pillar_of_frost.remains_expected+1)%gcd)%((rune+3)*(runic_power+5))*100,value_else=gcd*2,condition=runic_power<35&rune<2&cooldown.pillar_of_frost.remains_expected<10", "Formulaic approach to determine the time before these abilities come off cooldown that the simulation should star to pool resources. Capped at 15s in the run_action_list call." );
  default_->add_action( "variable,name=breath_pooling_time,op=setif,value=((cooldown.breath_of_sindragosa.remains+1)%gcd)%((rune+1)*(runic_power+20))*100,value_else=gcd*2,condition=runic_power.deficit>10&cooldown.breath_of_sindragosa.remains<10" );
  default_->add_action( "variable,name=pooling_runes,value=talent.obliteration&cooldown.pillar_of_frost.remains_expected<variable.oblit_pooling_time" );
  default_->add_action( "variable,name=pooling_runic_power,value=talent.breath_of_sindragosa&cooldown.breath_of_sindragosa.remains<variable.breath_pooling_time|talent.obliteration&runic_power<35&cooldown.pillar_of_frost.remains_expected<variable.oblit_pooling_time" );
  default_->add_action( "mind_freeze,if=target.debuff.casting.react", "Interrupt" );
  default_->add_action( "howling_blast,if=!dot.frost_fever.ticking&active_enemies>=2&(!talent.obliteration|talent.obliteration&(!buff.pillar_of_frost.up|buff.pillar_of_frost.up&!buff.killing_machine.react))", "Maintain Frost Fever, Icy Talons and Unleashed Frenzy" );
  default_->add_action( "glacial_advance,if=active_enemies>=2&variable.rp_buffs&talent.obliteration&talent.breath_of_sindragosa&!buff.pillar_of_frost.up&!buff.breath_of_sindragosa.up&cooldown.breath_of_sindragosa.remains>variable.breath_pooling_time" );
  default_->add_action( "glacial_advance,if=active_enemies>=2&variable.rp_buffs&talent.breath_of_sindragosa&!buff.breath_of_sindragosa.up&cooldown.breath_of_sindragosa.remains>variable.breath_pooling_time" );
  default_->add_action( "glacial_advance,if=active_enemies>=2&variable.rp_buffs&!talent.breath_of_sindragosa&talent.obliteration&!buff.pillar_of_frost.up" );
  default_->add_action( "frost_strike,if=active_enemies=1&variable.rp_buffs&talent.obliteration&talent.breath_of_sindragosa&!buff.pillar_of_frost.up&!buff.breath_of_sindragosa.up&cooldown.breath_of_sindragosa.remains>variable.breath_pooling_time" );
  default_->add_action( "frost_strike,if=active_enemies=1&variable.rp_buffs&talent.breath_of_sindragosa&!buff.breath_of_sindragosa.up&cooldown.breath_of_sindragosa.remains>variable.breath_pooling_time" );
  default_->add_action( "frost_strike,if=active_enemies=1&variable.rp_buffs&!talent.breath_of_sindragosa&talent.obliteration&!buff.pillar_of_frost.up" );
  default_->add_action( "remorseless_winter,if=!talent.breath_of_sindragosa&!talent.obliteration&variable.rw_buffs" );
  default_->add_action( "remorseless_winter,if=talent.obliteration&active_enemies>=3&variable.adds_remain" );
  default_->add_action( "call_action_list,name=trinkets", "Choose Action list to run" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=racials" );
  default_->add_action( "call_action_list,name=covenants" );
  default_->add_action( "call_action_list,name=cold_heart,if=talent.cold_heart&(!buff.killing_machine.up|talent.breath_of_sindragosa)&((debuff.razorice.stack=5|!death_knight.runeforge.razorice&!talent.glacial_advance&!talent.avalanche)|fight_remains<=gcd)" );
  default_->add_action( "run_action_list,name=breath_oblit,if=buff.breath_of_sindragosa.up&talent.obliteration&buff.pillar_of_frost.up" );
  default_->add_action( "run_action_list,name=breath,if=buff.breath_of_sindragosa.up&(!talent.obliteration|talent.obliteration&!buff.pillar_of_frost.up)" );
  default_->add_action( "run_action_list,name=obliteration,if=talent.obliteration&buff.pillar_of_frost.up&!buff.breath_of_sindragosa.up" );
  default_->add_action( "call_action_list,name=aoe,if=active_enemies>=2" );
  default_->add_action( "call_action_list,name=single_target,if=active_enemies=1" );

  aoe->add_action( "remorseless_winter", "AoE Action List" );
  aoe->add_action( "howling_blast,if=buff.rime.react|!dot.frost_fever.ticking" );
  aoe->add_action( "glacial_advance,if=!variable.pooling_runic_power&variable.rp_buffs" );
  aoe->add_action( "obliterate,if=buff.killing_machine.react&talent.cleaving_strikes&death_and_decay.ticking&!variable.frostscythe_priority" );
  aoe->add_action( "glacial_advance,if=!variable.pooling_runic_power" );
  aoe->add_action( "frostscythe,if=variable.frostscythe_priority" );
  aoe->add_action( "obliterate,if=!variable.frostscythe_priority" );
  aoe->add_action( "frost_strike,if=!variable.pooling_runic_power&!talent.glacial_advance" );
  aoe->add_action( "horn_of_winter,if=rune<2&runic_power.deficit>25" );
  aoe->add_action( "arcane_torrent,if=runic_power.deficit>25" );

  breath->add_action( "remorseless_winter,if=variable.rw_buffs|variable.adds_remain", "Breath Active Rotation" );
  breath->add_action( "howling_blast,if=variable.rime_buffs&runic_power>(45-talent.rage_of_the_frozen_champion*8)" );
  breath->add_action( "horn_of_winter,if=rune<2&runic_power.deficit>25" );
  breath->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=buff.killing_machine.react&!variable.frostscythe_priority" );
  breath->add_action( "frostscythe,if=buff.killing_machine.react&variable.frostscythe_priority" );
  breath->add_action( "frostscythe,if=variable.frostscythe_priority&runic_power>45" );
  breath->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=runic_power.deficit>40|buff.pillar_of_frost.up&runic_power.deficit>15" );
  breath->add_action( "death_and_decay,if=runic_power<32&rune.time_to_2>runic_power%16" );
  breath->add_action( "remorseless_winter,if=runic_power<32&rune.time_to_2>runic_power%16" );
  breath->add_action( "howling_blast,if=runic_power<32&rune.time_to_2>runic_power%16" );
  breath->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=runic_power.deficit>25" );
  breath->add_action( "howling_blast,if=buff.rime.react" );
  breath->add_action( "arcane_torrent,if=runic_power<60" );

  breath_oblit->add_action( "frostscythe,if=buff.killing_machine.up&variable.frostscythe_priority", "Breath & Obliteration Active Rotation" );
  breath_oblit->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=buff.killing_machine.up" );
  breath_oblit->add_action( "howling_blast,if=buff.rime.react" );
  breath_oblit->add_action( "howling_blast,if=!buff.killing_machine.up" );
  breath_oblit->add_action( "horn_of_winter,if=runic_power.deficit>25" );
  breath_oblit->add_action( "arcane_torrent,if=runic_power.deficit>20" );

  cold_heart->add_action( "chains_of_ice,if=fight_remains<gcd&(rune<2|!buff.killing_machine.up&(!variable.2h_check&buff.cold_heart.stack>=4|variable.2h_check&buff.cold_heart.stack>8)|buff.killing_machine.up&(!variable.2h_check&buff.cold_heart.stack>8|variable.2h_check&buff.cold_heart.stack>10))", "Cold Heart" );
  cold_heart->add_action( "chains_of_ice,if=!talent.obliteration&buff.pillar_of_frost.up&buff.cold_heart.stack>=10&(buff.pillar_of_frost.remains<gcd*(1+(talent.frostwyrms_fury&cooldown.frostwyrms_fury.ready))|buff.unholy_strength.up&buff.unholy_strength.remains<gcd)" );
  cold_heart->add_action( "chains_of_ice,if=!talent.obliteration&death_knight.runeforge.fallen_crusader&!buff.pillar_of_frost.up&cooldown.pillar_of_frost.remains>15&(buff.cold_heart.stack>=10&buff.unholy_strength.up|buff.cold_heart.stack>=13)" );
  cold_heart->add_action( "chains_of_ice,if=!talent.obliteration&!death_knight.runeforge.fallen_crusader&buff.cold_heart.stack>=10&!buff.pillar_of_frost.up&cooldown.pillar_of_frost.remains>20" );
  cold_heart->add_action( "chains_of_ice,if=talent.obliteration&!buff.pillar_of_frost.up&(buff.cold_heart.stack>=14&(buff.unholy_strength.up|buff.chaos_bane.up)|buff.cold_heart.stack>=19|cooldown.pillar_of_frost.remains<3&buff.cold_heart.stack>=14)" );

  cooldowns->add_action( "potion,if=variable.cooldown_check|fight_remains<25", "Cooldowns" );
  cooldowns->add_action( "empower_rune_weapon,if=talent.obliteration&!buff.empower_rune_weapon.up&rune<6&(cooldown.pillar_of_frost.remains<7&(variable.adds_remain|variable.st_planning)|buff.pillar_of_frost.up)|fight_remains<20" );
  cooldowns->add_action( "empower_rune_weapon,use_off_gcd=1,if=buff.breath_of_sindragosa.up&talent.breath_of_sindragosa&!buff.empower_rune_weapon.up&(runic_power<70&rune<3|time<10)" );
  cooldowns->add_action( "empower_rune_weapon,use_off_gcd=1,if=!talent.breath_of_sindragosa&!talent.obliteration&!buff.empower_rune_weapon.up&rune<5&(cooldown.pillar_of_frost.remains_expected<7|buff.pillar_of_frost.up|!talent.pillar_of_frost)" );
  cooldowns->add_action( "abomination_limb_talent,if=talent.obliteration&!buff.pillar_of_frost.up&(variable.adds_remain|variable.st_planning)|fight_remains<12" );
  cooldowns->add_action( "abomination_limb_talent,if=talent.breath_of_sindragosa&(variable.adds_remain|variable.st_planning)" );
  cooldowns->add_action( "abomination_limb_talent,if=!talent.breath_of_sindragosa&!talent.obliteration&(variable.adds_remain|variable.st_planning)" );
  cooldowns->add_action( "chill_streak,if=active_enemies>=2&(!death_and_decay.ticking&talent.cleaving_strikes|!talent.cleaving_strikes|active_enemies<=5)" );
  cooldowns->add_action( "pillar_of_frost,if=talent.obliteration&(variable.adds_remain|variable.st_planning)&(buff.empower_rune_weapon.up|cooldown.empower_rune_weapon.remains)|fight_remains<12" );
  cooldowns->add_action( "pillar_of_frost,if=talent.breath_of_sindragosa&(variable.adds_remain|variable.st_planning)&(!talent.icecap&(runic_power>70|cooldown.breath_of_sindragosa.remains>40)|talent.icecap&(cooldown.breath_of_sindragosa.remains>10|buff.breath_of_sindragosa.up))" );
  cooldowns->add_action( "pillar_of_frost,if=talent.icecap&!talent.obliteration&!talent.breath_of_sindragosa&(variable.adds_remain|variable.st_planning)" );
  cooldowns->add_action( "breath_of_sindragosa,if=runic_power>60&(variable.adds_remain|variable.st_planning)|fight_remains<30" );
  cooldowns->add_action( "frostwyrms_fury,if=active_enemies=1&(talent.pillar_of_frost&buff.pillar_of_frost.remains<gcd*2&buff.pillar_of_frost.up&!talent.obliteration|!talent.pillar_of_frost)&(!raid_event.adds.exists|(raid_event.adds.in>15+raid_event.adds.duration|talent.absolute_zero&raid_event.adds.in>15+raid_event.adds.duration))|fight_remains<3" );
  cooldowns->add_action( "frostwyrms_fury,if=active_enemies>=2&(talent.pillar_of_frost&buff.pillar_of_frost.up|raid_event.adds.exists&raid_event.adds.up&raid_event.adds.in>cooldown.pillar_of_frost.remains_expected-raid_event.adds.in-raid_event.adds.duration)&(buff.pillar_of_frost.remains<gcd*2|raid_event.adds.exists&raid_event.adds.remains<gcd*2)" );
  cooldowns->add_action( "frostwyrms_fury,if=talent.obliteration&(talent.pillar_of_frost&buff.pillar_of_frost.up&!variable.2h_check|!buff.pillar_of_frost.up&variable.2h_check&cooldown.pillar_of_frost.remains|!talent.pillar_of_frost)&((buff.pillar_of_frost.remains<gcd|buff.unholy_strength.up&buff.unholy_strength.remains<gcd)&(debuff.razorice.stack=5|!death_knight.runeforge.razorice&!talent.glacial_advance))" );
  cooldowns->add_action( "raise_dead" );
  cooldowns->add_action( "soul_reaper,if=fight_remains>5&target.time_to_pct_35<5&active_enemies<=2&(buff.breath_of_sindragosa.up&runic_power>40|!buff.breath_of_sindragosa.up&!talent.obliteration|talent.obliteration&!buff.pillar_of_frost.up)" );
  cooldowns->add_action( "sacrificial_pact,if=!talent.glacial_advance&!buff.breath_of_sindragosa.up&pet.ghoul.remains<gcd*2&active_enemies>3" );
  cooldowns->add_action( "any_dnd,if=!death_and_decay.ticking&variable.adds_remain&(buff.pillar_of_frost.up&buff.pillar_of_frost.remains>5|!buff.pillar_of_frost.up)&(active_enemies>5|talent.cleaving_strikes&active_enemies>=2)" );

  covenants->add_action( "deaths_due,if=(variable.rw_buffs&cooldown.remorseless_winter.remains|!variable.rw_buffs)&(!talent.obliteration|talent.obliteration&active_enemies>=2&cooldown.pillar_of_frost.remains|active_enemies=1)&(variable.st_planning|variable.adds_remain)", "Covenant Abilities" );
  covenants->add_action( "swarming_mist,if=runic_power.deficit>13&cooldown.pillar_of_frost.remains<3&!talent.breath_of_sindragosa&variable.st_planning" );
  covenants->add_action( "swarming_mist,if=!talent.breath_of_sindragosa&variable.adds_remain" );
  covenants->add_action( "swarming_mist,if=talent.breath_of_sindragosa&(buff.breath_of_sindragosa.up&(variable.st_planning&runic_power.deficit>40|variable.adds_remain&runic_power.deficit>60|variable.adds_remain&raid_event.adds.remains<9&raid_event.adds.exists)|!buff.breath_of_sindragosa.up&cooldown.breath_of_sindragosa.remains)" );
  covenants->add_action( "abomination_limb_covenant,if=cooldown.pillar_of_frost.remains<gcd*2&variable.st_planning&(talent.breath_of_sindragosa&runic_power>65&cooldown.breath_of_sindragosa.remains<2|!talent.breath_of_sindragosa)" );
  covenants->add_action( "abomination_limb_covenant,if=variable.adds_remain" );
  covenants->add_action( "shackle_the_unworthy,if=variable.st_planning&(cooldown.pillar_of_frost.remains<3|talent.icecap)" );
  covenants->add_action( "shackle_the_unworthy,if=variable.adds_remain" );
  covenants->add_action( "fleshcraft,if=!buff.pillar_of_frost.up&(soulbind.pustule_eruption|soulbind.volatile_solvent&!buff.volatile_solvent_humanoid.up),interrupt_immediate=1,interrupt_global=1,interrupt_if=soulbind.volatile_solvent" );

  obliteration->add_action( "remorseless_winter,if=active_enemies>3", "Obliteration Active Rotation" );
  obliteration->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=buff.killing_machine.react&!variable.frostscythe_priority" );
  obliteration->add_action( "frostscythe,if=buff.killing_machine.react&variable.frostscythe_priority" );
  obliteration->add_action( "howling_blast,if=!dot.frost_fever.ticking&!buff.killing_machine.react|!buff.killing_machine.react&buff.rime.react" );
  obliteration->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=!buff.killing_machine.react&variable.rp_buffs&!variable.pooling_runic_power&(!talent.glacial_advance|active_enemies=1)" );
  obliteration->add_action( "howling_blast,if=buff.rime.react&buff.killing_machine.react" );
  obliteration->add_action( "glacial_advance,if=!variable.pooling_runic_power&variable.rp_buffs&!buff.killing_machine.react&active_enemies>=2" );
  obliteration->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=!buff.killing_machine.react&!variable.pooling_runic_power&(!talent.glacial_advance|active_enemies=1)" );
  obliteration->add_action( "howling_blast,if=!buff.killing_machine.react&runic_power<25" );
  obliteration->add_action( "arcane_torrent,if=rune<1&runic_power<25" );
  obliteration->add_action( "glacial_advance,if=!variable.pooling_runic_power&active_enemies>=2" );
  obliteration->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=!variable.pooling_runic_power&(!talent.glacial_advance|active_enemies=1)" );
  obliteration->add_action( "howling_blast,if=buff.rime.react" );
  obliteration->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice" );

  racials->add_action( "blood_fury,if=variable.cooldown_check", "Racial Abilities" );
  racials->add_action( "berserking,if=variable.cooldown_check" );
  racials->add_action( "arcane_pulse,if=variable.cooldown_check" );
  racials->add_action( "lights_judgment,if=variable.cooldown_check" );
  racials->add_action( "ancestral_call,if=variable.cooldown_check" );
  racials->add_action( "fireblood,if=variable.cooldown_check" );
  racials->add_action( "bag_of_tricks,if=talent.obliteration&!buff.pillar_of_frost.up&buff.unholy_strength.up" );
  racials->add_action( "bag_of_tricks,if=!talent.obliteration&buff.pillar_of_frost.up&(buff.unholy_strength.up&buff.unholy_strength.remains<gcd*3|buff.pillar_of_frost.remains<gcd*3)" );

  single_target->add_action( "remorseless_winter,if=variable.rw_buffs|variable.adds_remain", "Single Target Rotation" );
  single_target->add_action( "howling_blast,if=buff.rime.react&talent.icebreaker.rank=2" );
  single_target->add_action( "frostscythe,if=!variable.pooling_runes&buff.killing_machine.react&variable.frostscythe_priority" );
  single_target->add_action( "obliterate,if=!variable.pooling_runes&buff.killing_machine.react" );
  single_target->add_action( "horn_of_winter,if=rune<4&runic_power.deficit>25&talent.obliteration&talent.breath_of_sindragosa" );
  single_target->add_action( "frost_strike,if=!variable.pooling_runic_power&(variable.rp_buffs|runic_power.deficit<25)" );
  single_target->add_action( "howling_blast,if=variable.rime_buffs" );
  single_target->add_action( "glacial_advance,if=!variable.pooling_runic_power&!death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<gcd*3)" );
  single_target->add_action( "obliterate,if=!variable.pooling_runes" );
  single_target->add_action( "horn_of_winter,if=rune<4&runic_power.deficit>25&(!talent.breath_of_sindragosa|cooldown.breath_of_sindragosa.remains>cooldown.horn_of_winter.duration)" );
  single_target->add_action( "arcane_torrent,if=runic_power.deficit>20" );
  single_target->add_action( "frost_strike,if=!variable.pooling_runic_power" );

  trinkets->add_action( "use_item,name=gavel_of_the_first_arbiter" );
  trinkets->add_action( "use_item,slot=trinket1,if=(buff.pillar_of_frost.up|buff.breath_of_sindragosa.up)&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains", "Trinkets The trinket with the highest estimated value, will be used first and paired with Pillar of Frost." );
  trinkets->add_action( "use_item,slot=trinket2,if=(buff.pillar_of_frost.up|buff.breath_of_sindragosa.up)&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains" );
  trinkets->add_action( "use_item,slot=trinket1,if=(!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs)|talent.pillar_of_frost&cooldown.pillar_of_frost.remains>20|!talent.pillar_of_frost)", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  trinkets->add_action( "use_item,slot=trinket2,if=(!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs)|talent.pillar_of_frost&cooldown.pillar_of_frost.remains>20|!talent.pillar_of_frost)" );
}
//frost_apl_end
  
//unholy_apl_start
void unholy( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* covenants = p->get_action_priority_list( "covenants" );
  action_priority_list_t* generic = p->get_action_priority_list( "generic" );
  action_priority_list_t* racials = p->get_action_priority_list( "racials" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "raise_dead" );
  precombat->add_action( "army_of_the_dead,precombat_time=1.5*gcd" );
  precombat->add_action( "fleshcraft" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=trinket.1.has_use_buff&(trinket.1.cooldown.duration%%45=0)" );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=trinket.2.has_use_buff&(trinket.2.cooldown.duration%%45=0)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!trinket.1.has_use_buff&trinket.2.has_use_buff|trinket.2.has_use_buff&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync))" );
  precombat->add_action( "variable,name=trinket_1_buffs,value=trinket.1.has_buff.strength|trinket.1.has_buff.mastery|trinket.1.has_buff.versatility|trinket.1.has_buff.haste|trinket.1.has_buff.crit" );
  precombat->add_action( "variable,name=trinket_2_buffs,value=trinket.2.has_buff.strength|trinket.2.has_buff.mastery|trinket.2.has_buff.versatility|trinket.2.has_buff.haste|trinket.2.has_buff.crit" );

  default_->add_action( "auto_attack" );
  default_->add_action( "mind_freeze,if=target.debuff.casting.react" );
  default_->add_action( "variable,name=apoc_timing,op=setif,value=((rune.time_to_3)%((debuff.festering_wound.stack+1)%4))+gcd+(talent.unholy_assault*4),value_else=gcd*5,condition=cooldown.apocalypse.remains<12", "Variables" );
  default_->add_action( "variable,name=garg_pooling,op=setif,value=(((cooldown.summon_gargoyle.remains+1)%gcd)%((rune+1)*(runic_power+20)))*100,value_else=gcd*2,condition=runic_power.deficit<40&cooldown.summon_gargoyle.remains<10" );
  default_->add_action( "variable,name=festermight_tracker,op=setif,value=debuff.festering_wound.stack>=2,value_else=debuff.festering_wound.stack>=4,condition=talent.festermight&(buff.festermight.remains%(4*gcd))>=1&cooldown.apocalypse.remains>variable.apoc_timing" );
  default_->add_action( "variable,name=build_wounds,value=debuff.festering_wound.stack<4" );
  default_->add_action( "variable,name=pop_wounds,value=(!cooldown.apocalypse.ready|!talent.apocalypse)&(variable.festermight_tracker|debuff.festering_wound.stack>=1&!talent.apocalypse|debuff.festering_wound.up&cooldown.unholy_assault.remains<20&talent.unholy_assault&variable.st_planning|debuff.festering_wound.stack>4)" );
  default_->add_action( "variable,name=pooling_runic_power,value=cooldown.summon_gargoyle.remains<variable.garg_pooling&talent.summon_gargoyle|talent.eternal_agony&cooldown.dark_transformation.remains<3&!active_enemies>=3|talent.vile_contagion&cooldown.vile_contagion.remains<3&runic_power<60&!variable.st_planning" );
  default_->add_action( "variable,name=pooling_runes,value=talent.soul_reaper&rune<2&target.time_to_pct_35<5&fight_remains>(dot.soul_reaper.remains+5)" );
  default_->add_action( "variable,name=st_planning,value=active_enemies<=3&(!raid_event.adds.exists|raid_event.adds.in>15)" );
  default_->add_action( "variable,name=adds_remain,value=active_enemies>=4&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.remains>6)" );
  default_->add_action( "wait_for_cooldown,name=soul_reaper,if=talent.soul_reaper&target.time_to_pct_35<5&fight_remains>5&cooldown.soul_reaper.remains<(gcd*0.75)&active_enemies=1", "Prioritize Soul Reaper, Outbreak and Maintaining Plaguebringer" );
  default_->add_action( "outbreak,target_if=(dot.virulent_plague.refreshable|dot.frost_fever_superstrain.refreshable|dot.blood_plague_superstrain.refreshable)&(!talent.unholy_blight|talent.unholy_blight&cooldown.unholy_blight.remains>15%((talent.superstrain*3)+(talent.plaguebringer*2)))" );
  default_->add_action( "wound_spender,if=cooldown.apocalypse.remains>variable.apoc_timing&talent.plaguebringer&talent.superstrain&buff.plaguebringer.remains<gcd" );
  default_->add_action( "call_action_list,name=trinkets", "Call Action Lists" );
  default_->add_action( "call_action_list,name=racials" );
  default_->add_action( "sequence,if=active_enemies=1&talent.summon_gargoyle&talent.unholy_assault,name=garg_ua_opener:unholy_blight:festering_strike:potion:dark_transformation:summon_gargoyle:unholy_assault:death_coil:empower_rune_weapon:apocalypse" );
  default_->add_action( "sequence,if=active_enemies=1&talent.summon_gargoyle&!talent.unholy_assault,name=garg_opener:unholy_blight:festering_strike:festering_strike:potion:dark_transformation:summon_gargoyle:death_coil:apocalypse:death_coil:death_coil:empower_rune_weapon" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=covenants" );
  default_->add_action( "run_action_list,name=aoe,if=active_enemies>=4" );
  default_->add_action( "run_action_list,name=generic,if=active_enemies<=3" );

  aoe->add_action( "any_dnd,if=!death_and_decay.ticking&variable.adds_remain&(talent.festermight&buff.festermight.remains<3|!talent.festermight)&(death_knight.fwounded_targets=active_enemies|death_knight.fwounded_targets=8|!talent.bursting_sores&!talent.vile_contagion|raid_event.adds.exists&raid_event.adds.remains<=11&raid_event.adds.remains>5|(cooldown.vile_contagion.remains|!talent.vile_contagion)&buff.dark_transformation.up&talent.infected_claws&(buff.empower_rune_weapon.up|buff.unholy_assault.up))|fight_remains<10", "AoE Action List" );
  aoe->add_action( "abomination_limb_talent,if=rune=0&variable.adds_remain" );
  aoe->add_action( "apocalypse,target_if=min:debuff.festering_wound.stack,if=debuff.festering_wound.up&variable.adds_remain&!death_and_decay.ticking&cooldown.death_and_decay.remains&rune<3|death_and_decay.ticking&rune=0" );
  aoe->add_action( "festering_strike,target_if=max:debuff.festering_wound.stack,if=!death_and_decay.ticking&debuff.festering_wound.stack<4&(cooldown.vile_contagion.remains<5|cooldown.apocalypse.ready&cooldown.any_dnd.remains)" );
  aoe->add_action( "festering_strike,target_if=min:debuff.festering_wound.stack,if=!death_and_decay.ticking&(cooldown.vile_contagion.remains>5|!talent.vile_contagion)" );
  aoe->add_action( "wound_spender,target_if=max:debuff.festering_wound.stack,if=death_and_decay.ticking" );
  aoe->add_action( "death_coil,if=!variable.pooling_runic_power&!talent.epidemic" );
  aoe->add_action( "epidemic,if=!variable.pooling_runic_power" );
  aoe->add_action( "wound_spender,target_if=max:debuff.festering_wound.stack,if=cooldown.death_and_decay.remains>10" );

  cooldowns->add_action( "potion,if=(30>=pet.gargoyle.remains&pet.gargoyle.active)|(!talent.summon_gargoyle|cooldown.summon_gargoyle.remains>60)&(buff.dark_transformation.up&30>=buff.dark_transformation.remains|pet.army_ghoul.active&pet.army_ghoul.remains<=30|pet.apoc_ghoul.active&pet.apoc_ghoul.remains<=30)|fight_remains<=30", "Potion" );
  cooldowns->add_action( "army_of_the_dead,if=talent.commander_of_the_dead&(cooldown.dark_transformation.remains<4|buff.commander_of_the_dead_window.up)|!talent.commander_of_the_dead&talent.unholy_assault&cooldown.unholy_assault.remains<10|!talent.unholy_assault&!talent.commander_of_the_dead|fight_remains<=30", "Cooldowns" );
  cooldowns->add_action( "vile_contagion,target_if=max:debuff.festering_wound.stack,if=active_enemies>=2&debuff.festering_wound.stack>=4&cooldown.any_dnd.remains<3" );
  cooldowns->add_action( "raise_dead,if=!pet.ghoul.active" );
  cooldowns->add_action( "summon_gargoyle,if=buff.commander_of_the_dead_window.up|!talent.commander_of_the_dead&runic_power>=40" );
  cooldowns->add_action( "unholy_assault,if=variable.st_planning" );
  cooldowns->add_action( "unholy_assault,target_if=min:debuff.festering_wound.stack,if=active_enemies>=2&debuff.festering_wound.stack<=2&(talent.vile_contagion&cooldown.vile_contagion.remains<gcd&cooldown.any_dnd.remains<3|buff.dark_transformation.up|cooldown.death_and_decay.remains<gcd)" );
  cooldowns->add_action( "apocalypse,target_if=max:debuff.festering_wound.stack,if=active_enemies<=3" );
  cooldowns->add_action( "dark_transformation,if=variable.st_planning&(pet.apoc_ghoul.active|cooldown.apocalypse.remains<gcd*2|!talent.commander_of_the_dead)" );
  cooldowns->add_action( "dark_transformation,if=variable.adds_remain&(cooldown.any_dnd.remains<10&talent.infected_claws&((cooldown.vile_contagion.remains|raid_event.adds.exists&raid_event.adds.in>10)&death_knight.fwounded_targets<active_enemies|!talent.vile_contagion)&(raid_event.adds.remains>5|!raid_event.adds.exists)|!talent.infected_claws)|fight_remains<25" );
  cooldowns->add_action( "soul_reaper,if=active_enemies=1&target.time_to_pct_35<5&target.time_to_die>(dot.soul_reaper.remains+5)" );
  cooldowns->add_action( "soul_reaper,target_if=min:dot.soul_reaper.remains,if=target.time_to_pct_35<5&active_enemies>=2&target.time_to_die>(dot.soul_reaper.remains+5)" );
  cooldowns->add_action( "unholy_blight,if=variable.st_planning&((!talent.apocalypse|cooldown.apocalypse.remains)&talent.morbidity|!talent.morbidity)" );
  cooldowns->add_action( "unholy_blight,if=variable.adds_remain|fight_remains<21" );
  cooldowns->add_action( "empower_rune_weapon,if=variable.st_planning&runic_power.deficit>20&(pet.gargoyle.active&pet.apoc_ghoul.active|!talent.summon_gargoyle&talent.army_of_the_damned&pet.army_ghoul.active&pet.apoc_ghoul.active|!talent.summon_gargoyle&!talent.army_of_the_damned&buff.dark_transformation.up|!talent.summon_gargoyle&!talent.summon_gargoyle&buff.dark_transformation.up)|fight_remains<=21" );
  cooldowns->add_action( "empower_rune_weapon,if=variable.adds_remain&buff.dark_transformation.up" );
  cooldowns->add_action( "abomination_limb_talent,if=variable.st_planning&rune<3" );
  cooldowns->add_action( "sacrificial_pact,if=active_enemies>=2&!buff.dark_transformation.up&cooldown.dark_transformation.remains>6|fight_remains<gcd" );

  covenants->add_action( "swarming_mist,if=variable.st_planning&runic_power.deficit>16&(cooldown.apocalypse.remains|!talent.army_of_the_damned&cooldown.dark_transformation.remains)|fight_remains<11", "Covenant Abilities" );
  covenants->add_action( "swarming_mist,if=cooldown.apocalypse.remains&(active_enemies>=2&active_enemies<=5&runic_power.deficit>10+(active_enemies*6)&variable.adds_remain|active_enemies>5&runic_power.deficit>40)", "Set to use after apoc is on CD as to prevent overcapping RP while setting up CD's" );
  covenants->add_action( "abomination_limb_covenant,if=variable.st_planning&!soulbind.lead_by_example&(cooldown.apocalypse.remains|!talent.army_of_the_damned&cooldown.dark_transformation.remains)&rune.time_to_4>buff.runic_corruption.remains|fight_remains<12+(soulbind.kevins_oozeling*28)" );
  covenants->add_action( "abomination_limb_covenant,if=variable.st_planning&soulbind.lead_by_example&(dot.unholy_blight_dot.remains>11|!talent.unholy_blight&cooldown.dark_transformation.remains)" );
  covenants->add_action( "abomination_limb_covenant,if=variable.st_planning&soulbind.kevins_oozeling&(debuff.festering_wound.stack>=4&!runeforge.abominations_frenzy|runeforge.abominations_frenzy&cooldown.apocalypse.remains)" );
  covenants->add_action( "abomination_limb_covenant,if=variable.adds_remain&rune.time_to_4>buff.runic_corruption.remains" );
  covenants->add_action( "shackle_the_unworthy,if=variable.st_planning&(cooldown.apocalypse.remains>10|!talent.army_of_the_damned&cooldown.dark_transformation.remains)|fight_remains<15" );
  covenants->add_action( "shackle_the_unworthy,if=variable.adds_remain&(death_and_decay.ticking|raid_event.adds.remains<=14)" );
  covenants->add_action( "fleshcraft,if=soulbind.pustule_eruption|soulbind.volatile_solvent&!buff.volatile_solvent_humanoid.up,interrupt_immediate=1,interrupt_global=1,interrupt_if=soulbind.volatile_solvent" );

  generic->add_action( "death_coil,if=!variable.pooling_runic_power&(buff.sudden_doom.react|runic_power.deficit<=40|rune<3)|pet.gargoyle.active&(cooldown.apocalypse.remains>gcd|debuff.festering_wound.stack>=4)|fight_remains<(30%gcd)", "Generic" );
  generic->add_action( "any_dnd,if=!death_and_decay.ticking&(active_enemies>=2&death_knight.fwounded_targets=active_enemies|covenant.night_fae|runeforge.phearomones)" );
  generic->add_action( "wound_spender,target_if=max:debuff.festering_wound.stack,if=variable.pop_wounds|active_enemies>=2&death_and_decay.ticking" );
  generic->add_action( "festering_strike,target_if=max:debuff.festering_wound.stack,if=debuff.festering_wound.stack<4&cooldown.apocalypse.remains<variable.apoc_timing" );
  generic->add_action( "festering_strike,target_if=min:debuff.festering_wound.stack,if=variable.build_wounds" );

  racials->add_action( "arcane_torrent,if=runic_power.deficit>20&(cooldown.summon_gargoyle.remains<gcd|!talent.summon_gargoyle.enabled|pet.gargoyle.active&rune<2&debuff.festering_wound.stack<1)", "Racials" );
  racials->add_action( "blood_fury,if=(buff.blood_fury.duration>=pet.gargoyle.remains&pet.gargoyle.active)|(!talent.summon_gargoyle|cooldown.summon_gargoyle.remains>60)&(buff.dark_transformation.up&buff.blood_fury.duration>=buff.dark_transformation.remains|pet.army_ghoul.active&pet.army_ghoul.remains<=buff.blood_fury.duration|pet.apoc_ghoul.active&pet.apoc_ghoul.remains<=buff.blood_fury.duration|active_enemies>=2&death_and_decay.ticking)|fight_remains<=buff.blood_fury.duration" );
  racials->add_action( "berserking,if=(buff.berserking.duration>=pet.gargoyle.remains&pet.gargoyle.active)|(!talent.summon_gargoyle|cooldown.summon_gargoyle.remains>60)&(buff.dark_transformation.up&buff.berserking.duration>=buff.dark_transformation.remains|pet.army_ghoul.active&pet.army_ghoul.remains<=buff.berserking.duration|pet.apoc_ghoul.active&pet.apoc_ghoul.remains<=buff.berserking.duration|active_enemies>=2&death_and_decay.ticking)|fight_remains<=buff.berserking.duration" );
  racials->add_action( "lights_judgment,if=buff.unholy_strength.up&(!talent.festermight|buff.festermight.remains<target.time_to_die|buff.unholy_strength.remains<target.time_to_die)" );
  racials->add_action( "ancestral_call,if=(15>=pet.gargoyle.remains&pet.gargoyle.active)|(!talent.summon_gargoyle|cooldown.summon_gargoyle.remains>60)&(buff.dark_transformation.up&15>=buff.dark_transformation.remains|pet.army_ghoul.active&pet.army_ghoul.remains<=15|pet.apoc_ghoul.active&pet.apoc_ghoul.remains<=15|active_enemies>=2&death_and_decay.ticking)|fight_remains<=15" );
  racials->add_action( "arcane_pulse,if=active_enemies>=2|(rune.deficit>=5&runic_power.deficit>=60)" );
  racials->add_action( "fireblood,if=(buff.fireblood.duration>=pet.gargoyle.remains&pet.gargoyle.active)|(!talent.summon_gargoyle|cooldown.summon_gargoyle.remains>60)&(buff.dark_transformation.up&buff.fireblood.duration>=buff.dark_transformation.remains|pet.army_ghoul.active&pet.army_ghoul.remains<=buff.fireblood.duration|pet.apoc_ghoul.active&pet.apoc_ghoul.remains<=buff.fireblood.duration|active_enemies>=2&death_and_decay.ticking)|fight_remains<=buff.fireblood.duration" );
  racials->add_action( "bag_of_tricks,if=active_enemies=1&(buff.unholy_strength.up|fight_remains<5)" );

  trinkets->add_action( "use_item,name=gavel_of_the_first_arbiter" );
  trinkets->add_action( "use_item,slot=trinket1,if=((!talent.summon_gargoyle|talent.summon_gargoyle&pet.gargoyle.active|cooldown.summon_gargoyle.remains>90)&(pet.apoc_ghoul.active|buff.dark_transformation.up)&variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains", "Trinkets" );
  trinkets->add_action( "use_item,slot=trinket2,if=((!talent.summon_gargoyle|talent.summon_gargoyle&pet.gargoyle.active|cooldown.summon_gargoyle.remains>90)&(pet.apoc_ghoul.active|buff.dark_transformation.up)&variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains" );
  trinkets->add_action( "use_item,slot=trinket1,if=!variable.trinket_1_buffs&(trinket.2.cooldown.remains|!variable.trinket_2_buffs|!talent.summon_gargoyle|cooldown.summon_gargoyle.remains>20&!pet.gargoyle.active)|fight_remains<15" );
  trinkets->add_action( "use_item,slot=trinket2,if=!variable.trinket_2_buffs&(trinket.1.cooldown.remains|!variable.trinket_1_buffs|!talent.summon_gargoyle|cooldown.summon_gargoyle.remains>20&!pet.gargoyle.active)|fight_remains<15" );
}
//unholy_apl_end

}  // namespace death_knight_apl
