#include "class_modules/apl/apl_death_knight.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"
#include "dbc/dbc.hpp"

namespace death_knight_apl {

std::string potion( const player_t* p )
{
  std::string frost_potion = ( p->true_level >= 60 ) ? "potion_of_spectral_strength" : "disabled";

  std::string unholy_potion = ( p->true_level >= 60 ) ? "potion_of_spectral_strength" : "disabled";

  std::string blood_potion = ( p->true_level >= 60 ) ? "potion_of_spectral_strength" : "disabled";

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
  std::string flask_name = ( p->true_level >= 60 ) ? "spectral_flask_of_power" : "disabled";

  // All specs use a strength flask as default
  return flask_name;
}

std::string food( const player_t* p )
{
  std::string frost_food = ( p->true_level >= 60 ) ? "feast_of_gluttonous_hedonism" : "disabled";

  std::string unholy_food = ( p->true_level >= 60 ) ? "feast_of_gluttonous_hedonism" : "disabled";

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
  return ( p->true_level >= 60 ) ? "veiled" : "disabled";
}

std::string temporary_enchant( const player_t* p )
{
  std::string frost_temporary_enchant =
      ( p->true_level >= 60 ) ? "main_hand:shaded_sharpening_stone/off_hand:shaded_sharpening_stone" : "disabled";

  std::string unholy_temporary_enchant = ( p->true_level >= 60 ) ? "main_hand:shaded_sharpening_stone" : "disabled";

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

// blood_apl_start
void blood( player_t* p )
{
  action_priority_list_t* default_  = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* covenants = p->get_action_priority_list( "covenants" );
  action_priority_list_t* standard  = p->get_action_priority_list( "standard" );
  action_priority_list_t* racials   = p->get_action_priority_list( "racials" );
  action_priority_list_t* drw_up    = p->get_action_priority_list( "drw_up" );


  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "fleshcraft" );

  default_->add_action( "auto_attack" );
  default_->add_action( "variable,name=death_strike_dump_amount,if=!covenant.night_fae,value=70" );
  default_->add_action( "variable,name=death_strike_dump_amount,if=covenant.night_fae,value=55" );
  default_->add_action( "mind_freeze,if=target.debuff.casting.react", "Interrupt" );
  default_->add_action( "potion,if=buff.dancing_rune_weapon.up","Since the potion cooldown has changed, we'll sync with DRW" );
  default_->add_action( "use_items" );
  default_->add_action( "use_item,name=gavel_of_the_first_arbiter" );
  default_->add_action( "raise_dead" );
  default_->add_action( "blooddrinker,if=!buff.dancing_rune_weapon.up&(!covenant.night_fae|buff.deaths_due.remains>7)" );
  default_->add_action( "call_action_list,name=racials" );
  default_->add_action( "sacrificial_pact,if=(!covenant.night_fae|buff.deaths_due.remains>6)&buff.dancing_rune_weapon.remains>4&(pet.ghoul.remains<2|target.time_to_die<gcd)","Attempt to sacrifice the ghoul if we predictably will not do much in the near future" );
  default_->add_action( "call_action_list,name=covenants" );
  default_->add_action( "blood_tap,if=(rune<=2&rune.time_to_4>gcd&charges_fractional>=1.8)|rune.time_to_3>gcd" );
  default_->add_action( "dancing_rune_weapon,if=!buff.dancing_rune_weapon.up" );
  default_->add_action( "run_action_list,name=drw_up,if=buff.dancing_rune_weapon.up" );
  default_->add_action( "call_action_list,name=standard" );

  racials->add_action( "blood_fury,if=cooldown.dancing_rune_weapon.ready&(!cooldown.blooddrinker.ready|!talent.blooddrinker.enabled)" );
  racials->add_action( "berserking" );
  racials->add_action( "arcane_pulse,if=active_enemies>=2|rune<1&runic_power.deficit>60" );
  racials->add_action( "lights_judgment,if=buff.unholy_strength.up" );
  racials->add_action( "ancestral_call" );
  racials->add_action( "fireblood" );
  racials->add_action( "bag_of_tricks" );
  racials->add_action( "arcane_torrent,if=runic_power.deficit>20" );

  covenants->add_action( "deaths_due,if=!buff.deaths_due.up|buff.deaths_due.remains<4|buff.crimson_scourge.up" );
  covenants->add_action( "swarming_mist,if=cooldown.dancing_rune_weapon.remains>3&runic_power>=(90-(spell_targets.swarming_mist*3))" );
  covenants->add_action( "abomination_limb" );
  covenants->add_action( "fleshcraft,if=soulbind.pustule_eruption|soulbind.volatile_solvent&!buff.volatile_solvent_humanoid.up,interrupt_immediate=1,interrupt_global=1,interrupt_if=soulbind.volatile_solvent" );
  covenants->add_action( "shackle_the_unworthy,if=rune<3&runic_power<100" );

  drw_up->add_action( "tombstone,if=buff.bone_shield.stack>5&rune>=2&runic_power.deficit>=30&runeforge.crimson_rune_weapon" );
  drw_up->add_action( "marrowrend,if=(buff.bone_shield.remains<=rune.time_to_3|(buff.bone_shield.stack<2&(!covenant.necrolord|buff.abomination_limb.up)))&runic_power.deficit>20" );
  drw_up->add_action( "blood_boil,if=((charges>=2&rune<=1)|dot.blood_plague.remains<=2)|(spell_targets.blood_boil>5&charges_fractional>=1.1)&!(covenant.venthyr&buff.swarming_mist.up)" );
  drw_up->add_action( "variable,name=heart_strike_rp_drw,value=(25+spell_targets.heart_strike*talent.heartbreaker.enabled*2)" );
  drw_up->add_action( "death_strike,if=((runic_power.deficit<=variable.heart_strike_rp_drw)|(runic_power.deficit<=variable.death_strike_dump_amount&covenant.venthyr))&!(talent.bonestorm.enabled&cooldown.bonestorm.remains<2)" );
  drw_up->add_action( "death_and_decay,if=(spell_targets.death_and_decay==3&buff.crimson_scourge.up)|spell_targets.death_and_decay>=4" );
  drw_up->add_action( "bonestorm,if=runic_power>=100&buff.endless_rune_waltz.stack>4&!(covenant.venthyr&cooldown.swarming_mist.remains<3)" );
  drw_up->add_action( "heart_strike,if=rune.time_to_2<gcd|runic_power.deficit>=variable.heart_strike_rp_drw" );
  drw_up->add_action( "consumption" );

  standard->add_action( "heart_strike,if=covenant.night_fae&death_and_decay.ticking&(buff.deaths_due.up&buff.deaths_due.remains<6)" );
  standard->add_action( "tombstone,if=buff.bone_shield.stack>5&rune>=2&runic_power.deficit>=30&!(covenant.venthyr&cooldown.swarming_mist.remains<3)" );
  standard->add_action( "marrowrend,if=(buff.bone_shield.remains<=rune.time_to_3|buff.bone_shield.remains<=(gcd+cooldown.blooddrinker.ready*talent.blooddrinker.enabled*4)|buff.bone_shield.stack<6|((!covenant.night_fae|buff.deaths_due.remains>5)&buff.bone_shield.remains<7))&runic_power.deficit>20&!(runeforge.crimson_rune_weapon&cooldown.dancing_rune_weapon.remains<buff.bone_shield.remains)" );
  standard->add_action( "death_strike,if=runic_power.deficit<=variable.death_strike_dump_amount&!(talent.bonestorm.enabled&cooldown.bonestorm.remains<2)&!(covenant.venthyr&cooldown.swarming_mist.remains<3)" );
  standard->add_action( "blood_boil,if=charges_fractional>=1.8&(buff.hemostasis.stack<=(5-spell_targets.blood_boil)|spell_targets.blood_boil>2)" );
  standard->add_action( "death_and_decay,if=buff.crimson_scourge.up&talent.relish_in_blood.enabled&runic_power.deficit>10" );
  standard->add_action( "bonestorm,if=runic_power>=100&!(covenant.venthyr&cooldown.swarming_mist.remains<3)" );
  standard->add_action( "variable,name=heart_strike_rp,value=(15+spell_targets.heart_strike*talent.heartbreaker.enabled*2),op=setif,condition=covenant.night_fae&death_and_decay.ticking,value_else=(15+spell_targets.heart_strike*talent.heartbreaker.enabled*2)*1.2" );
  standard->add_action( "death_strike,if=(runic_power.deficit<=variable.heart_strike_rp)|target.time_to_die<10" );
  standard->add_action( "death_and_decay,if=spell_targets.death_and_decay>=3" );
  standard->add_action( "heart_strike,if=rune.time_to_4<gcd" );
  standard->add_action( "death_and_decay,if=buff.crimson_scourge.up|talent.rapid_decomposition.enabled" );
  standard->add_action( "consumption" );
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
  action_priority_list_t* bos_pooling = p->get_action_priority_list( "bos_pooling" );
  action_priority_list_t* bos_ticking = p->get_action_priority_list( "bos_ticking" );
  action_priority_list_t* cold_heart = p->get_action_priority_list( "cold_heart" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* obliteration = p->get_action_priority_list( "obliteration" );
  action_priority_list_t* obliteration_pooling = p->get_action_priority_list( "obliteration_pooling" );
  action_priority_list_t* racials = p->get_action_priority_list( "racials" );
  action_priority_list_t* standard = p->get_action_priority_list( "standard" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "variable,name=rw_buffs,value=talent.gathering_storm|talent.everfrost|talent.biting_cold", "Evaluates a trinkets cooldown, divided by pillar of frost, empower rune weapon, or breath of sindragosa's cooldown. If it's value has no remainder return 1, else return 0.5. actions.precombat+=/variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=trinket.1.has_use_buff&(talent.pillar_of_frost&!talent.breath_of_sindragosa&(trinket.1.cooldown.duration%%cooldown.pillar_of_frost.duration=0)|talent.breath_of_sindragosa&(cooldown.breath_of_sindragosa.duration%%trinket.1.cooldown.duration=0)|talent.icecap) actions.precombat+=/variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=trinket.2.has_use_buff&(talent.pillar_of_frost&!talent.breath_of_sindragosa&(trinket.2.cooldown.duration%%cooldown.pillar_of_frost.duration=0)|talent.breath_of_sindragosa&(cooldown.breath_of_sindragosa.duration%%trinket.2.cooldown.duration=0)|talent.icecap)  Estimates a trinkets value by comparing the cooldown of the trinket, divided by the duration of the buff it provides. Has a strength modifier to give a higher priority to strength trinkets, as well as a modifier for if a trinket will or will not sync with cooldowns. actions.precombat+=/variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!trinket.1.has_use_buff&trinket.2.has_use_buff|trinket.2.has_use_buff&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync))" );
  precombat->add_action( "variable,name=2h_check,value=main_hand.2h&talent.might_of_the_frozen_wastes" );

  default_->add_action( "auto_attack" );
  default_->add_action( "variable,name=st_planning,value=active_enemies=1&(raid_event.adds.in>15|!raid_event.adds.exists)", "Prevent specified trinkets being used with automatic lines  actions+=/variable,name=specified_trinket,value=" );
  default_->add_action( "variable,name=adds_remain,value=active_enemies>=2&(!raid_event.adds.exists|raid_event.adds.exists&(raid_event.adds.remains>5|target.1.time_to_die>10))" );
  default_->add_action( "variable,name=rotfc_rime,value=buff.rime.up&(!talent.rage_of_the_frozen_champion|talent.rage_of_the_frozen_champion&runic_power.deficit>8)" );
  default_->add_action( "variable,name=frost_strike_buffs,value=talent.unleashed_frenzy&buff.unleashed_frenzy.remains<(gcd*2)" );
  default_->add_action( "variable,name=cooldown_check,value=talent.pillar_of_frost&buff.pillar_of_frost.up|!talent.pillar_of_frost&buff.empower_rune_weapon.up|!talent.pillar_of_frost" );
  default_->add_action( "remorseless_winter,if=talent.gathering_storm&!talent.obliteration&cooldown.pillar_of_frost.remains&(talent.everfrost|talent.biting_cold)", "Apply Frost Fever, maintain Icy Talons and keep Remorseless Winter rolling" );
  default_->add_action( "howling_blast,target_if=!dot.frost_fever.remains&(talent.icecap|!buff.breath_of_sindragosa.up&talent.breath_of_sindragosa|talent.obliteration&cooldown.pillar_of_frost.remains&!buff.killing_machine.up)" );
  default_->add_action( "glacial_advance,if=buff.icy_talons.remains<=gcd*2&talent.icy_talons&spell_targets.glacial_advance>=2&(talent.icecap|talent.breath_of_sindragosa&!buff.breath_of_sindragosa.up|talent.obliteration&!buff.pillar_of_frost.up)" );
  default_->add_action( "frost_strike,if=buff.icy_talons.remains<=gcd*2&talent.icy_talons&(talent.icecap|talent.breath_of_sindragosa&!buff.breath_of_sindragosa.up&cooldown.breath_of_sindragosa.remains>10|talent.obliteration&!buff.pillar_of_frost.up)" );
  default_->add_action( "mind_freeze,if=target.debuff.casting.react", "Interrupt" );
  default_->add_action( "call_action_list,name=racials", "Choose Action list to run" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=cold_heart,if=talent.cold_heart&(!buff.killing_machine.up|talent.breath_of_sindragosa)&((debuff.razorice.stack=5|!death_knight.runeforge.razorice&!talent.glacial_advance)|fight_remains<=gcd)" );
  default_->add_action( "run_action_list,name=bos_ticking,if=buff.breath_of_sindragosa.up" );
  default_->add_action( "run_action_list,name=bos_pooling,if=talent.breath_of_sindragosa&!buff.breath_of_sindragosa.up&(cooldown.breath_of_sindragosa.remains<10)&(raid_event.adds.in>25|!raid_event.adds.exists|talent.pillar_of_frost&cooldown.pillar_of_frost.remains<10&raid_event.adds.exists&raid_event.adds.in<10|!talent.pillar_of_frost)" );
  default_->add_action( "run_action_list,name=obliteration,if=buff.pillar_of_frost.up&talent.obliteration" );
  default_->add_action( "run_action_list,name=obliteration_pooling,if=talent.obliteration&talent.pillar_of_frost&cooldown.pillar_of_frost.remains<10&(variable.st_planning|raid_event.adds.exists&raid_event.adds.in<10|!raid_event.adds.exists)" );
  default_->add_action( "run_action_list,name=aoe,if=active_enemies>=2" );
  default_->add_action( "call_action_list,name=standard" );

  aoe->add_action( "remorseless_winter", "AoE Rotation" );
  aoe->add_action( "glacial_advance,if=!death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<gcd*4)" );
  aoe->add_action( "chill_streak,if=active_enemies>=2" );
  aoe->add_action( "frostscythe,if=buff.killing_machine.react" );
  aoe->add_action( "howling_blast,if=variable.rotfc_rime&talent.avalanche" );
  aoe->add_action( "glacial_advance,if=!buff.rime.up&active_enemies<=3|active_enemies>3" );
  aoe->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=cooldown.remorseless_winter.remains<=2*gcd&talent.gathering_storm", "Formulaic approach to create a pseudo priority target list for applying razorice in aoe" );
  aoe->add_action( "howling_blast,if=variable.rotfc_rime" );
  aoe->add_action( "frostscythe,if=talent.gathering_storm&buff.remorseless_winter.up&active_enemies>2" );
  aoe->add_action( "obliterate,if=talent.gathering_storm&buff.remorseless_winter.up" );
  aoe->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=runic_power.deficit<(15+talent.runic_attenuation*5)" );
  aoe->add_action( "frostscythe" );
  aoe->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=runic_power.deficit>(25+talent.runic_attenuation*5)" );
  aoe->add_action( "glacial_advance" );
  aoe->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice" );
  // aoe->add_action( "horn_of_winter" );
  aoe->add_action( "arcane_torrent" );

  bos_pooling->add_action( "remorseless_winter,if=active_enemies>=2|variable.rw_buffs", "Breath of Sindragosa pooling rotation : starts 10s before BoS is available" );
  bos_pooling->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=buff.killing_machine.react&cooldown.pillar_of_frost.remains>3" );
  bos_pooling->add_action( "howling_blast,if=variable.rotfc_rime" );
  bos_pooling->add_action( "frostscythe,if=buff.killing_machine.react&runic_power.deficit>(15+talent.runic_attenuation*5)&spell_targets.frostscythe>2" );
  bos_pooling->add_action( "frostscythe,if=runic_power.deficit>=(35+talent.runic_attenuation*5)&spell_targets.frostscythe>2" );
  bos_pooling->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=runic_power.deficit>=25" );
  bos_pooling->add_action( "chill_streak,if=active_enemies>=2&runic_power.deficit<20&(cooldown.pillar_of_frost.remains>5|!talent.pillar_of_frost&cooldown.breath_of_sindragosa.remains>5)" );
  bos_pooling->add_action( "glacial_advance,if=runic_power.deficit<20&spell_targets.glacial_advance>=2&(cooldown.pillar_of_frost.remains>5|!talent.pillar_of_frost&cooldown.breath_of_sindragosa.remains>5)" );
  bos_pooling->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=runic_power.deficit<20&(cooldown.pillar_of_frost.remains>5|!talent.pillar_of_frost&cooldown.breath_of_sindragosa.remains>5)" );
  bos_pooling->add_action( "glacial_advance,if=cooldown.pillar_of_frost.remains>rune.time_to_4&runic_power.deficit<40&spell_targets.glacial_advance>=2" );
  bos_pooling->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=talent.pillar_of_frost&cooldown.pillar_of_frost.remains>rune.time_to_4&runic_power.deficit<40|!talent.pillar_of_frost&cooldown.breath_of_sindragosa.remains>rune.time_to_4&runic_power.deficit<40" );

  bos_ticking->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=runic_power<=(45+talent.runic_attenuation*5)", "Breath of Sindragosa Active Rotation" );
  bos_ticking->add_action( "remorseless_winter,if=variable.rw_buffs|active_enemies>=2|runic_power<32&rune.time_to_2<runic_power%16" );
  bos_ticking->add_action( "death_and_decay,if=runic_power<32&rune.time_to_2<runic_power%16" );
  bos_ticking->add_action( "howling_blast,if=variable.rotfc_rime&(runic_power>=45|rune.time_to_3<=gcd|talent.rage_of_the_frozen_champion|spell_targets.howling_blast>=2|buff.rime.remains<3)|runic_power<32&rune.time_to_2<runic_power%16" );
  bos_ticking->add_action( "frostscythe,if=buff.killing_machine.up&spell_targets.frostscythe>2" );
  bos_ticking->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=buff.killing_machine.react" );
  // bos_ticking->add_action( "horn_of_winter,if=runic_power<=60&rune.time_to_3>gcd" );
  bos_ticking->add_action( "frostscythe,if=spell_targets.frostscythe>2" );
  bos_ticking->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=runic_power.deficit>25|rune.time_to_4<gcd" );
  bos_ticking->add_action( "howling_blast,if=variable.rotfc_rime" );
  bos_ticking->add_action( "arcane_torrent,if=runic_power<50" );

  cold_heart->add_action( "chains_of_ice,if=fight_remains<gcd&(rune<2|!buff.killing_machine.up&(!variable.2h_check&buff.cold_heart.stack>=4|variable.2h_check&buff.cold_heart.stack>8)|buff.killing_machine.up&(!variable.2h_check&buff.cold_heart.stack>8|variable.2h_check&buff.cold_heart.stack>10))", "Cold Heart Conditions" );
  cold_heart->add_action( "chains_of_ice,if=!talent.obliteration&buff.pillar_of_frost.up&buff.cold_heart.stack>=10&(buff.pillar_of_frost.remains<gcd*(1+cooldown.frostwyrms_fury.ready)|buff.unholy_strength.up&buff.unholy_strength.remains<gcd)", "Use during Pillar with Icecap/Breath" );
  cold_heart->add_action( "chains_of_ice,if=!talent.obliteration&death_knight.runeforge.fallen_crusader&!buff.pillar_of_frost.up&cooldown.pillar_of_frost.remains>15&(buff.cold_heart.stack>=10&buff.unholy_strength.up|buff.cold_heart.stack>=13)", "Outside of Pillar useage with Icecap/Breath" );
  cold_heart->add_action( "chains_of_ice,if=!talent.obliteration&!death_knight.runeforge.fallen_crusader&buff.cold_heart.stack>=10&!buff.pillar_of_frost.up&cooldown.pillar_of_frost.remains>20" );
  cold_heart->add_action( "chains_of_ice,if=talent.obliteration&!buff.pillar_of_frost.up&(buff.cold_heart.stack>=14&(buff.unholy_strength.up|buff.chaos_bane.up)|buff.cold_heart.stack>=19|cooldown.pillar_of_frost.remains<3&buff.cold_heart.stack>=14)", "Prevent Cold Heart overcapping during pillar" );

  cooldowns->add_action( "potion,if=variable.cooldown_check", "Potion" );
  cooldowns->add_action( "empower_rune_weapon,if=talent.obliteration&rune<6&!buff.empower_rune_weapon.up&(variable.st_planning|variable.adds_remain)&(cooldown.pillar_of_frost.remains<5|buff.pillar_of_frost.up)|fight_remains<20|!talent.pillar_of_frost", "Cooldowns" );
  cooldowns->add_action( "empower_rune_weapon,if=talent.breath_of_sindragosa&!buff.empower_rune_weapon.up&rune<5&runic_power<(60-(death_knight.runeforge.hysteria*5))&(buff.breath_of_sindragosa.up|fight_remains<20)" );
  cooldowns->add_action( "empower_rune_weapon,if=talent.icecap&!buff.empower_rune_weapon.up" );
  cooldowns->add_action( "pillar_of_frost,if=talent.breath_of_sindragosa&(variable.st_planning|variable.adds_remain)&(cooldown.breath_of_sindragosa.remains|buff.breath_of_sindragosa.up&runic_power>45|cooldown.breath_of_sindragosa.ready&runic_power>65)" );
  cooldowns->add_action( "pillar_of_frost,if=talent.icecap&!buff.pillar_of_frost.up" );
  cooldowns->add_action( "pillar_of_frost,if=talent.obliteration&(runic_power>=35&!buff.abomination_limb_talent.up|buff.abomination_limb_talent.up|talent.rage_of_the_frozen_champion)&(variable.st_planning|variable.adds_remain)&(talent.gathering_storm&buff.remorseless_winter.up|!talent.gathering_storm)" );
  cooldowns->add_action( "breath_of_sindragosa,if=!buff.breath_of_sindragosa.up&runic_power>60&(buff.pillar_of_frost.up|cooldown.pillar_of_frost.remains>15|!talent.pillar_of_frost)" );
  cooldowns->add_action( "soul_reaper,target_if=target.time_to_pct_35<5&target.time_to_die>(dot.soul_reaper.remains+5)&(talent.obliteration&!buff.pillar_of_frost.up|talent.icecap|talent.breath_of_sindragosa&(!buff.breath_of_sindragosa.up|runic_power>50|runic_power<32&rune.time_to_2<runic_power%16))" );
  cooldowns->add_action( "frostwyrms_fury,if=active_enemies=1&(talent.pillar_of_frost&buff.pillar_of_frost.remains<gcd&buff.pillar_of_frost.up&!talent.obliteration|!talent.pillar_of_frost)&(!raid_event.adds.exists|raid_event.adds.in>30)|fight_remains<3" );
  cooldowns->add_action( "frostwyrms_fury,if=active_enemies>=2&(talent.pillar_of_frost&buff.pillar_of_frost.up&buff.pillar_of_frost.remains<gcd*2|raid_event.adds.exists&!raid_event.adds.up&raid_event.adds.in>cooldown.pillar_of_frost.remains+7)&(buff.pillar_of_frost.remains<gcd|raid_event.adds.exists&raid_event.adds.remains<gcd|!talent.pillar_of_frost)" );
  cooldowns->add_action( "frostwyrms_fury,if=talent.obliteration&(talent.pillar_of_frost&buff.pillar_of_frost.up&!variable.2h_check|!buff.pillar_of_frost.up&variable.2h_check&cooldown.pillar_of_frost.remains|!talent.pillar_of_frost)&((buff.pillar_of_frost.remains<gcd|buff.unholy_strength.up&buff.unholy_strength.remains<gcd)&(debuff.razorice.stack=5|!death_knight.runeforge.razorice&!talent.glacial_advance))" );
  cooldowns->add_action( "abomination_limb_talent,if=variable.st_planning&(talent.pillar_of_frost&cooldown.pillar_of_frost.remains<gcd*2|!talent.pillar_of_frost)&(talent.breath_of_sindragosa&runic_power>65&cooldown.breath_of_sindragosa.remains<2|!talent.breath_of_sindragosa)" );
  cooldowns->add_action( "abomination_limb_talent,if=variable.adds_remain" );
  cooldowns->add_action( "raise_dead,if=cooldown.pillar_of_frost.remains<=5|!talent.pillar_of_frost" );
  cooldowns->add_action( "sacrificial_pact,if=active_enemies>=2&(fight_remains<3|!buff.breath_of_sindragosa.up&(pet.ghoul.remains<gcd|raid_event.adds.exists&raid_event.adds.remains<3&raid_event.adds.in>pet.ghoul.remains))" );
  cooldowns->add_action( "death_and_decay,if=active_enemies>5|talent.improved_death_and_decay&active_enemies>=2" );

  obliteration->add_action( "remorseless_winter,if=active_enemies>=3&variable.rw_buffs", "Obliteration rotation" );
  obliteration->add_action( "frost_strike,if=!buff.killing_machine.up&(rune<2|talent.icy_talons&buff.icy_talons.remains<gcd*2)" );
  obliteration->add_action( "howling_blast,target_if=!buff.killing_machine.up&rune>=3&(buff.rime.remains<3&buff.rime.up|!dot.frost_fever.ticking)" );
  obliteration->add_action( "glacial_advance,if=!buff.killing_machine.up&(debuff.razorice.stack<5|debuff.razorice.remains<gcd*4|spell_targets.glacial_advance>=2)" );
  obliteration->add_action( "frostscythe,if=buff.killing_machine.react&spell_targets.frostscythe>2" );
  obliteration->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=buff.killing_machine.react" );
  obliteration->add_action( "frost_strike,if=active_enemies=1&variable.frost_strike_buffs" );
  obliteration->add_action( "howling_blast,if=variable.rotfc_rime&spell_targets.howling_blast>=2" );
  obliteration->add_action( "glacial_advance,if=spell_targets.glacial_advance>=2" );
  obliteration->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=!talent.avalanche&!buff.killing_machine.up|talent.avalanche&!variable.rotfc_rime|variable.rotfc_rime&rune.time_to_2>=gcd" );
  obliteration->add_action( "howling_blast,if=variable.rotfc_rime" );
  obliteration->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice" );

  obliteration_pooling->add_action( "remorseless_winter,if=variable.rw_buffs|active_enemies>=2", "Pooling For Obliteration: Starts 10 seconds before Pillar of Frost comes off CD" );
  // obliteration_pooling->add_action( "horn_of_winter,if=cooldown.pillar_of_frost.remains<gcd" );
  obliteration_pooling->add_action( "glacial_advance,if=!death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<gcd*4)" );
  obliteration_pooling->add_action( "frostscythe,if=buff.killing_machine.react&active_enemies>2" );
  obliteration_pooling->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=buff.killing_machine.react" );
  obliteration_pooling->add_action( "frost_strike,if=active_enemies=1&variable.frost_strike_buffs" );
  obliteration_pooling->add_action( "howling_blast,if=variable.rotfc_rime" );
  obliteration_pooling->add_action( "glacial_advance,if=spell_targets.glacial_advance>=2&runic_power.deficit<60" );
  obliteration_pooling->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=runic_power.deficit<70" );
  obliteration_pooling->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=rune>=3&(!main_hand.2h|covenant.necrolord|covenant.kyrian)|rune>=4&main_hand.2h" );
  obliteration_pooling->add_action( "frostscythe,if=active_enemies>=4" );

  racials->add_action( "blood_fury,if=variable.cooldown_check", "Racial Abilities" );
  racials->add_action( "berserking,if=variable.cooldown_check" );
  racials->add_action( "arcane_pulse,if=(!buff.pillar_of_frost.up&active_enemies>=2)|!buff.pillar_of_frost.up&(rune.deficit>=5&runic_power.deficit>=60)" );
  racials->add_action( "lights_judgment,if=variable.cooldown_check" );
  racials->add_action( "ancestral_call,if=variable.cooldown_check" );
  racials->add_action( "fireblood,if=buff.pillar_of_frost.remains<=8&variable.cooldown_check" );
  racials->add_action( "bag_of_tricks,if=variable.cooldown_check&active_enemies=1&(buff.pillar_of_frost.remains<5&talent.cold_heart|!talent.cold_heart&buff.pillar_of_frost.remains<3|!talent.pillar_of_frost)" );

  standard->add_action( "remorseless_winter,if=variable.rw_buffs", "Standard single-target rotation" );
  standard->add_action( "obliterate,if=buff.killing_machine.react" );
  standard->add_action( "howling_blast,if=variable.rotfc_rime&buff.rime.remains<3" );
  standard->add_action( "frost_strike,if=variable.frost_strike_buffs" );
  standard->add_action( "glacial_advance,if=!death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<gcd*4)" );
  standard->add_action( "frost_strike,if=cooldown.remorseless_winter.remains<=2*gcd&talent.gathering_storm" );
  standard->add_action( "howling_blast,if=variable.rotfc_rime" );
  standard->add_action( "frost_strike,if=runic_power.deficit<(15+talent.runic_attenuation*5)" );
  standard->add_action( "obliterate,if=!buff.frozen_pulse.up&talent.frozen_pulse|talent.gathering_storm&buff.remorseless_winter.up|runic_power.deficit>(25+talent.runic_attenuation*5)" );
  standard->add_action( "frost_strike" );
  // standard->add_action( "horn_of_winter,if=talent.obliteration&cooldown.pillar_of_frost.remains>30|talent.breath_of_sindragosa&cooldown.breath_of_sindragosa.remains>20|talent.icecap" );
  standard->add_action( "arcane_torrent" );

  trinkets->add_action( "use_items", "Trinkets  The trinket with the highest estimated value, will be used first and paired with Pillar of Frost. actions.trinkets+=/use_item,slot=trinket1,if=buff.pillar_of_frost.up&(!talent.icecap|talent.icecap&(buff.pillar_of_frost.remains>=10|!talent.pillar_of_frost))&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1)|trinket.1.proc.any_dps.duration>=fight_remains actions.trinkets+=/use_item,slot=trinket2,if=buff.pillar_of_frost.up&(!talent.icecap|talent.icecap&(buff.pillar_of_frost.remains>=10|!talent.pillar_of_frost))&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2)|trinket.2.proc.any_dps.duration>=fight_remains  If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown. actions.trinkets+=/use_item,slot=trinket1,if=(!trinket.1.has_use_buff&(trinket.2.cooldown.remains|!trinket.2.has_use_buff)|talent.pillar_of_frost&cooldown.pillar_of_frost.remains>20|!talent.pillar_of_frost) actions.trinkets+=/use_item,slot=trinket2,if=(!trinket.2.has_use_buff&(trinket.1.cooldown.remains|!trinket.1.has_use_buff)|talent.pillar_of_frost&cooldown.pillar_of_frost.remains>20|!talent.pillar_of_frost)" );
}
//frost_apl_end

//unholy_apl_start
void unholy( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* aoe_burst = p->get_action_priority_list( "aoe_burst" );
  action_priority_list_t* aoe_setup = p->get_action_priority_list( "aoe_setup" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* generic = p->get_action_priority_list( "generic" );
  action_priority_list_t* generic_aoe = p->get_action_priority_list( "generic_aoe" );
  action_priority_list_t* racials = p->get_action_priority_list( "racials" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "raise_dead" );
  precombat->add_action( "army_of_the_dead,precombat_time=1.5" );
  precombat->add_action( "variable,name=trinket_1_sync,op=setif,value=1,value_else=0.5,condition=trinket.1.has_use_buff&(trinket.1.cooldown.duration%%45=0)", "Evaluates a trinkets cooldown, divided by 45. This was chosen as unholy works on 45 second burst cycles, but has too many cdr effects to give a cooldown.x.duration divisor instead. If it's value has no remainder return 1, else return 0.5." );
  precombat->add_action( "variable,name=trinket_2_sync,op=setif,value=1,value_else=0.5,condition=trinket.2.has_use_buff&(trinket.2.cooldown.duration%%45=0)" );
  precombat->add_action( "variable,name=trinket_priority,op=setif,value=2,value_else=1,condition=!trinket.1.has_use_buff&trinket.2.has_use_buff|trinket.2.has_use_buff&((trinket.2.cooldown.duration%trinket.2.proc.any_dps.duration)*(1.5+trinket.2.has_buff.strength)*(variable.trinket_2_sync))>((trinket.1.cooldown.duration%trinket.1.proc.any_dps.duration)*(1.5+trinket.1.has_buff.strength)*(variable.trinket_1_sync))", "Estimates a trinkets value by comparing the cooldown of the trinket, divided by the duration of the buff it provides. Has a strength modifier to give a higher priority to strength trinkets, as well as a modifier for if a trinket will or will not sync with cooldowns." );

  default_->add_action( "auto_attack", "Evaluates current setup for the quantity of Apocalypse CDR effects" );
  default_->add_action( "mind_freeze,if=target.debuff.casting.react", "Interrupt" );
  default_->add_action( "variable,name=apoc_timing,value=(rune.time_to_3%(debuff.festering_wound.stack%4))+2+(talent.unholy_assault*3)", "Prevent specified trinkets being used with automatic lines Variables" );
  default_->add_action( "variable,name=build_wounds,value=debuff.festering_wound.stack<4" );
  default_->add_action( "variable,name=pop_wounds,value=(debuff.festering_wound.stack>=4&cooldown.apocalypse.remains>variable.apoc_timing|debuff.festering_wound.up&cooldown.unholy_assault.remains<30&talent.unholy_assault|cooldown.apocalypse.remains>10&talent.plaguebringer&talent.superstrain&buff.plaguebringer.remains<gcd*2|debuff.festering_wound.stack>4)" );
  default_->add_action( "variable,name=pooling_runic_power,value=cooldown.summon_gargoyle.remains<6&talent.summon_gargoyle" );
  default_->add_action( "variable,name=pooling_runes,value=talent.soul_reaper&rune<2&target.time_to_pct_35<5&fight_remains>(dot.soul_reaper.remains+5)|talent.eternal_agony&talent.ghoulish_frenzy&cooldown.dark_transformation.remains<4" );
  default_->add_action( "variable,name=st_planning,value=active_enemies<=3&(!raid_event.adds.exists|raid_event.adds.in>15)" );
  default_->add_action( "variable,name=adds_remain,value=active_enemies>=4&(!raid_event.adds.exists|raid_event.adds.exists&(raid_event.adds.remains>5|target.1.time_to_die>10))" );
  default_->add_action( "variable,name=major_cooldowns_active,value=(talent.summon_gargoyle&pet.gargoyle.active|!talent.summon_gargoyle)&(buff.unholy_assault.up|talent.army_of_the_damned&pet.apoc_ghoul.active|buff.dark_transformation.up&buff.dark_transformation.remains>5|active_enemies>=2&death_and_decay.ticking)" );
  default_->add_action( "outbreak,target_if=(dot.virulent_plague.refreshable|dot.frost_fever_superstrain.refreshable|dot.blood_plague_superstrain.refreshable)&(!talent.unholy_blight|talent.unholy_blight&cooldown.unholy_blight.remains>15%((talent.superstrain*3)+(talent.plaguebringer*2)))", "Maintaining Virulent Plague is a priority" );
  default_->add_action( "wait_for_cooldown,name=soul_reaper,if=talent.soul_reaper&target.time_to_pct_35<5&fight_remains>5&cooldown.soul_reaper.remains<(gcd*0.75)&active_enemies=1", "Refreshes Deaths Due's buff just before deaths due ends" );
  default_->add_action( "call_action_list,name=trinkets", "Action Lists and Openers" );
  default_->add_action( "call_action_list,name=racials" );
  default_->add_action( "sequence,if=active_enemies>=3&talent.plaguebearer&talent.unholy_assault,name=aoe_opener:unholy_blight:unholy_assault:festering_strike:plaguebearer:dark_transformation:any_dnd:empower_rune_weapon" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "run_action_list,name=aoe_setup,if=variable.adds_remain&(cooldown.death_and_decay.remains<10&!talent.defile|cooldown.defile.remains<10&talent.defile|talent.plaguebearer&cooldown.plaguebearer.remains<3*gcd)&!death_and_decay.ticking" );
  default_->add_action( "run_action_list,name=aoe_burst,if=variable.adds_remain&death_and_decay.ticking" );
  default_->add_action( "run_action_list,name=generic_aoe,if=variable.adds_remain&(!death_and_decay.ticking&(cooldown.death_and_decay.remains>10&!talent.defile|cooldown.defile.remains>10&talent.defile))" );
  default_->add_action( "call_action_list,name=generic,if=variable.st_planning" );

  aoe_burst->add_action( "clawing_shadows,if=active_enemies<=5", "AoE Burst" );
  aoe_burst->add_action( "clawing_shadows,if=active_enemies=6&death_knight.fwounded_targets>=3" );
  aoe_burst->add_action( "wound_spender,target_if=max:debuff.festering_wound.stack,if=talent.bursting_sores" );
  aoe_burst->add_action( "death_coil,if=(buff.sudden_doom.react|!variable.pooling_runic_power)&(active_enemies<=3+(talent.eternal_agony*2)+talent.death_rot+talent.improved_death_coil+talent.rotten_touch+talent.coil_of_devastation|!talent.epidemic)" );
  aoe_burst->add_action( "epidemic,if=runic_power.deficit<(10+death_knight.fwounded_targets*3)&death_knight.fwounded_targets<6&!variable.pooling_runic_power|buff.swarming_mist.up" );
  aoe_burst->add_action( "epidemic,if=runic_power.deficit<25&death_knight.fwounded_targets>5&!variable.pooling_runic_power" );
  aoe_burst->add_action( "epidemic,if=!death_knight.fwounded_targets&!variable.pooling_runic_power|fight_remains<5|raid_event.adds.exists&raid_event.adds.remains<5" );
  aoe_burst->add_action( "wound_spender" );
  aoe_burst->add_action( "epidemic,if=!variable.pooling_runic_power" );
  aoe_burst->add_action( "death_coil,if=!variable.pooling_runic_power" );

  aoe_setup->add_action( "any_dnd,if=death_knight.fwounded_targets=active_enemies|death_knight.fwounded_targets>=5|!talent.bursting_sores|raid_event.adds.exists&raid_event.adds.remains<=11|fight_remains<=11", "AoE Setup" );
  aoe_setup->add_action( "abomination_limb_talent,if=variable.adds_remain&rune<=2|fight_remains<13" );
  aoe_setup->add_action( "festering_strike,target_if=max:debuff.festering_wound.stack,if=debuff.festering_wound.stack<=3&(cooldown.apocalypse.remains<rune.time_to_3|cooldown.plaguebearer.remains<gcd*3)" );
  aoe_setup->add_action( "festering_strike,target_if=debuff.festering_wound.stack<1" );
  aoe_setup->add_action( "festering_strike,target_if=min:debuff.festering_wound.stack,if=rune.time_to_4<(cooldown.death_and_decay.remains&!talent.defile|cooldown.defile.remains&talent.defile)" );
  aoe_setup->add_action( "death_coil,if=(buff.sudden_doom.react|!variable.pooling_runic_power)&(active_enemies<=3+(talent.eternal_agony*2)+talent.death_rot+talent.improved_death_coil+talent.rotten_touch+talent.coil_of_devastation|!talent.epidemic)" );
  aoe_setup->add_action( "epidemic,if=!variable.pooling_runic_power" );

  cooldowns->add_action( "potion,if=variable.major_cooldowns_active|pet.gargoyle.active&pet.gargoyle.remains<=26|fight_remains<26", "Potion" );
  cooldowns->add_action( "army_of_the_dead,if=talent.commander_of_the_dead&cooldown.dark_transformation.remains_expected<8|!talent.commander_of_the_dead&talent.unholy_assault&cooldown.unholy_assault.remains<10|!talent.unholy_assault&!talent.commander_of_the_dead", "Cooldowns" );
  cooldowns->add_action( "army_of_the_dead,if=fight_remains<30+gcd" );
  cooldowns->add_action( "raise_dead,if=!pet.ghoul.active" );
  cooldowns->add_action( "soul_reaper,if=active_enemies=1&target.time_to_pct_35<5&target.time_to_die>(dot.soul_reaper.remains+5)" );
  cooldowns->add_action( "soul_reaper,target_if=min:dot.soul_reaper.remains,if=target.time_to_pct_35<5&active_enemies>=2&target.time_to_die>(dot.soul_reaper.remains+5)" );
  cooldowns->add_action( "unholy_blight,if=variable.st_planning&((!talent.apocalypse|cooldown.apocalypse.remains)&talent.morbidity|!talent.morbidity)" );
  cooldowns->add_action( "unholy_blight,if=variable.adds_remain|fight_remains<21" );
  cooldowns->add_action( "plaguebearer,target_if=max:debuff.festering_wound.stack,if=active_enemies>=2&debuff.festering_wound.stack>=4" );
  cooldowns->add_action( "unholy_assault,if=variable.st_planning&(!talent.apocalypse|cooldown.apocalypse.remains<gcd|talent.summon_gargoyle&pet.gargoyle.remains<=buff.unholy_assault.duration)" );
  cooldowns->add_action( "unholy_assault,target_if=min:debuff.festering_wound.stack,if=active_enemies>=2&debuff.festering_wound.stack<=2&(talent.plaguebearer&cooldown.plaguebearer.remains<gcd|buff.dark_transformation.up|cooldown.death_and_decay.remains<gcd)" );
  cooldowns->add_action( "apocalypse,if=active_enemies=1&debuff.festering_wound.stack>=4" );
  cooldowns->add_action( "apocalypse,target_if=max:debuff.festering_wound.stack,if=active_enemies>=2&debuff.festering_wound.stack>=4&cooldown.death_and_decay.remains&!death_and_decay.ticking" );
  cooldowns->add_action( "summon_gargoyle,if=runic_power.deficit<60" );
  cooldowns->add_action( "dark_transformation,if=variable.st_planning&pet.apoc_ghoul.active" );
  cooldowns->add_action( "dark_transformation,if=variable.adds_remain|fight_remains<30" );
  cooldowns->add_action( "empower_rune_weapon,if=variable.st_planning&(pet.gargoyle.active&pet.gargoyle.remains<=21|!talent.summon_gargoyle&talent.army_of_the_damned&pet.army_ghoul.active&pet.apoc_ghoul.active|!talent.summon_gargoyle&!talent.army_of_the_damned&buff.dark_transformation.up)|fight_remains<=21" );
  cooldowns->add_action( "empower_rune_weapon,if=variable.adds_remain&buff.dark_transformation.up" );
  cooldowns->add_action( "abomination_limb_talent,if=variable.st_planning&rune<3" );
  cooldowns->add_action( "sacrificial_pact,if=active_enemies>=2&!buff.dark_transformation.up&cooldown.dark_transformation.remains>6|fight_remains<gcd" );

  generic->add_action( "death_coil,if=!variable.pooling_runic_power&(buff.sudden_doom.react|runic_power.deficit<=40|rune<3)|pet.gargoyle.active|fight_remains<10", "Single Target" );
  generic->add_action( "wound_spender,if=variable.pop_wounds" );
  generic->add_action( "festering_strike,if=variable.build_wounds" );

  generic_aoe->add_action( "wait_for_cooldown,name=soul_reaper,if=talent.soul_reaper&target.time_to_pct_35<5&fight_remains>5&cooldown.soul_reaper.remains<(gcd*0.75)&active_enemies<=3", "Generic AoE Priority" );
  generic_aoe->add_action( "festering_strike,target_if=max:debuff.festering_wound.stack,if=debuff.festering_wound.stack<=3&cooldown.apocalypse.remains<5|debuff.festering_wound.stack<1" );
  generic_aoe->add_action( "festering_strike,target_if=min:debuff.festering_wound.stack,if=cooldown.apocalypse.remains>5&debuff.festering_wound.stack<1" );
  generic_aoe->add_action( "death_coil,if=(buff.sudden_doom.react|!variable.pooling_runic_power)&(active_enemies<=3+(talent.eternal_agony*2)+talent.death_rot+talent.improved_death_coil+talent.rotten_touch+talent.coil_of_devastation|!talent.epidemic)" );
  generic_aoe->add_action( "epidemic,if=buff.sudden_doom.react|!variable.pooling_runic_power" );
  generic_aoe->add_action( "wound_spender,target_if=max:debuff.festering_wound.stack,if=(cooldown.apocalypse.remains>5&debuff.festering_wound.up|debuff.festering_wound.stack>4)&(fight_remains<cooldown.death_and_decay.remains+10|fight_remains>cooldown.apocalypse.remains)" );

  racials->add_action( "arcane_torrent,if=runic_power.deficit>65&(pet.gargoyle.active|!talent.summon_gargoyle.enabled)&rune.deficit>=5", "Racials" );
  racials->add_action( "blood_fury,if=variable.major_cooldowns_active|pet.gargoyle.active&pet.gargoyle.remains<=buff.blood_fury.duration|fight_remains<=buff.blood_fury.duration" );
  racials->add_action( "berserking,if=variable.major_cooldowns_active&pet.army_ghoul.active|pet.gargoyle.active&pet.gargoyle.remains<=buff.berserking.duration|fight_remains<=buff.berserking.duration" );
  racials->add_action( "lights_judgment,if=buff.unholy_strength.up" );
  racials->add_action( "ancestral_call,if=variable.major_cooldowns_active|pet.gargoyle.active&pet.gargoyle.remains<=15|fight_remains<=15", "Ancestral Call can trigger 4 potential buffs, each lasting 15 seconds. Utilized hard coded time as a trigger to keep it readable." );
  racials->add_action( "arcane_pulse,if=active_enemies>=2|(rune.deficit>=5&runic_power.deficit>=60)" );
  racials->add_action( "fireblood,if=variable.major_cooldowns_active|pet.gargoyle.active&pet.gargoyle.remains<=buff.fireblood.duration|fight_remains<=buff.fireblood.duration" );
  racials->add_action( "bag_of_tricks,if=active_enemies=1&(buff.unholy_strength.up|fight_remains<5)" );

  trinkets->add_action( "use_item,slot=trinket1,if=((trinket.1.proc.any_dps.duration<=15&cooldown.apocalypse.remains>20|trinket.1.proc.any_dps.duration>15&(cooldown.unholy_blight.remains>20|cooldown.dark_transformation.remains_expected>20)|active_enemies>=2&buff.dark_transformation.up)&(!trinket.2.has_cooldown|trinket.2.cooldown.remains|variable.trinket_priority=1))|trinket.1.proc.any_dps.duration>=fight_remains", "Trinkets The trinket with the highest estimated value, will be used first and paired with Apocalypse (if buff is 15 seconds or less) or Blight/DT (if greater than 15 seconds)" );
  trinkets->add_action( "use_item,slot=trinket2,if=((trinket.2.proc.any_dps.duration<=15&cooldown.apocalypse.remains>20|trinket.2.proc.any_dps.duration>15&(cooldown.unholy_blight.remains>20|cooldown.dark_transformation.remains_expected>20)|active_enemies>=2&buff.dark_transformation.up)&(!trinket.1.has_cooldown|trinket.1.cooldown.remains|variable.trinket_priority=2))|trinket.2.proc.any_dps.duration>=fight_remains" );
  trinkets->add_action( "use_item,slot=trinket1,if=!trinket.1.has_use_buff&(trinket.2.cooldown.remains|!trinket.2.has_use_buff)", "If only one on use trinket provides a buff, use the other on cooldown. Or if neither trinket provides a buff, use both on cooldown." );
  trinkets->add_action( "use_item,slot=trinket2,if=!trinket.2.has_use_buff&(trinket.1.cooldown.remains|!trinket.1.has_use_buff)" );
}
//unholy_apl_end

}  // namespace death_knight_apl
