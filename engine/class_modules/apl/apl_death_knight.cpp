#include "class_modules/apl/apl_death_knight.hpp"

#include "player/action_priority_list.hpp"
#include "player/sc_player.hpp"

namespace death_knight_apl {

std::string potion( const player_t* p )
{
  std::string frost_potion = ( p -> true_level >= 60 ) ? "potion_of_spectral_strength" :
                             "disabled";

  std::string unholy_potion = ( p -> true_level >= 60 ) ? "potion_of_spectral_strength" :
                              "disabled";

  std::string blood_potion =  ( p -> true_level >= 60 ) ? "potion_of_phantom_fire" :
                              "disabled";

  switch ( p -> specialization() )
  {
    case DEATH_KNIGHT_BLOOD: return blood_potion;
    case DEATH_KNIGHT_FROST: return frost_potion;
    default:                 return unholy_potion;
  }
}

std::string flask( const player_t* p )
{
  std::string flask_name = ( p -> true_level >= 60 ) ? "spectral_flask_of_power" :
                           "disabled";

  // All specs use a strength flask as default
  return flask_name;
}

std::string food( const player_t* p )
{
  std::string frost_food = ( p -> true_level >= 60 ) ? "feast_of_gluttonous_hedonism" :
                           "disabled";

  std::string unholy_food = ( p ->  true_level >= 60 ) ? "feast_of_gluttonous_hedonism" :
                            "disabled";

  std::string blood_food =  ( p -> true_level >= 60 ) ? "feast_of_gluttonous_hedonism" :
                            "disabled";

  switch ( p -> specialization() )
  {
    case DEATH_KNIGHT_BLOOD: return blood_food;
    case DEATH_KNIGHT_FROST: return frost_food;

    default:                 return unholy_food;
  }
}

std::string rune( const player_t* p )
{
  return ( p -> true_level >= 60 ) ? "veiled" :
         "disabled";
}

std::string temporary_enchant( const player_t* p )
{
  std::string frost_temporary_enchant = ( p -> true_level >= 60 ) ? "main_hand:shaded_sharpening_stone/off_hand:shaded_sharpening_stone" :
                           "disabled";

  std::string unholy_temporary_enchant = ( p -> true_level >= 60 ) ? "main_hand:shaded_sharpening_stone" :
                            "disabled";

  std::string blood_temporary_enchant =  ( p -> true_level >= 60 ) ? "main_hand:shadowcore_oil" :
                            "disabled";

  switch ( p -> specialization() )
  {
    case DEATH_KNIGHT_BLOOD: return blood_temporary_enchant;
    case DEATH_KNIGHT_FROST: return frost_temporary_enchant;

    default:                 return unholy_temporary_enchant;
  }
}

//blood_apl_start
void blood( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list  ( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list ( "precombat" );
  action_priority_list_t* covenants = p->get_action_priority_list ( "covenants" );
  action_priority_list_t* standard = p->get_action_priority_list  ( "standard" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  default_->add_action( "auto_attack" );
  default_->add_action( "blood_fury,if=cooldown.dancing_rune_weapon.ready&(!cooldown.blooddrinker.ready|!talent.blooddrinker.enabled)" );
  default_->add_action( "berserking" );
  default_->add_action( "arcane_pulse,if=active_enemies>=2|rune<1&runic_power.deficit>60" );
  default_->add_action( "lights_judgment,if=buff.unholy_strength.up" );
  default_->add_action( "ancestral_call" );
  default_->add_action( "fireblood" );
  default_->add_action( "bag_of_tricks" );
  default_->add_action( "potion,if=buff.dancing_rune_weapon.up", "Since the potion cooldown has changed, we'll sync with DRW" );
  default_->add_action( "use_items" );
  default_->add_action( "raise_dead" );
  default_->add_action( "blooddrinker,if=!buff.dancing_rune_weapon.up&(!covenant.night_fae|buff.deaths_due.remains>7)" );
  default_->add_action( "blood_boil,if=charges>=2&(covenant.kyrian|buff.dancing_rune_weapon.up)" );
  default_->add_action( "raise_dead" );
  default_->add_action( "death_strike,if=fight_remains<3" );
  default_->add_action( "call_action_list,name=covenants" );
  default_->add_action( "call_action_list,name=standard" );

  covenants->add_action( "death_strike,if=covenant.night_fae&buff.deaths_due.remains>6&runic_power>70", "Burn RP if we have time between DD refreshes" );
  covenants->add_action( "heart_strike,if=covenant.night_fae&death_and_decay.ticking&((buff.deaths_due.up|buff.dancing_rune_weapon.up)&buff.deaths_due.remains<6)", "Make sure we never lose that buff" );
  covenants->add_action( "deaths_due,if=!buff.deaths_due.up|buff.deaths_due.remains<4|buff.crimson_scourge.up", "And that we always cast DD as high prio when we actually need it" );
  covenants->add_action( "sacrificial_pact,if=(!covenant.night_fae|buff.deaths_due.remains>6)&!buff.dancing_rune_weapon.up&(pet.ghoul.remains<10|target.time_to_die<gcd)", "Attempt to sacrifice the ghoul if we predictably will not do much in the near future" );
  covenants->add_action( "death_strike,if=covenant.venthyr&runic_power>70&cooldown.swarming_mist.remains<3", "Burn RP off just before swarming comes back off CD" );
  covenants->add_action( "swarming_mist,if=!buff.dancing_rune_weapon.up", "And swarming as long as we're not < 3s off DRW" );
  covenants->add_action( "marrowrend,if=covenant.necrolord&buff.bone_shield.stack<=0", "Pre-AL marrow on pull in order to guarantee ossuary during the first DRW" );
  covenants->add_action( "abomination_limb,if=!buff.dancing_rune_weapon.up", "And we cast AL" );
  covenants->add_action( "shackle_the_unworthy,if=cooldown.dancing_rune_weapon.remains<3|!buff.dancing_rune_weapon.up", "We just don't cast this during DRW" );

  standard->add_action( "blood_tap,if=rune<=2&rune.time_to_4>gcd&charges_fractional>=1.8", "Use blood tap to prevent overcapping charges if we have space for a rune and a GCD to spare to burn it" );
  standard->add_action( "dancing_rune_weapon,if=!talent.blooddrinker.enabled|!cooldown.blooddrinker.ready" );
  standard->add_action( "tombstone,if=buff.bone_shield.stack>=7&rune>=2" );
  standard->add_action( "marrowrend,if=(!covenant.necrolord|buff.abomination_limb.up)&(buff.bone_shield.remains<=rune.time_to_3|buff.bone_shield.remains<=(gcd+cooldown.blooddrinker.ready*talent.blooddrinker.enabled*2)|buff.bone_shield.stack<3)&runic_power.deficit>=20" );
  standard->add_action( "death_strike,if=runic_power.deficit<=70" );
  standard->add_action( "marrowrend,if=buff.bone_shield.stack<6&runic_power.deficit>=15&(!covenant.night_fae|buff.deaths_due.remains>5)" );
  standard->add_action( "heart_strike,if=!talent.blooddrinker.enabled&death_and_decay.remains<5&runic_power.deficit<=(15+buff.dancing_rune_weapon.up*5+spell_targets.heart_strike*talent.heartbreaker.enabled*2)" );
  standard->add_action( "blood_boil,if=charges_fractional>=1.8&(buff.hemostasis.stack<=(5-spell_targets.blood_boil)|spell_targets.blood_boil>2)" );
  standard->add_action( "death_and_decay,if=(buff.crimson_scourge.up&talent.relish_in_blood.enabled)&runic_power.deficit>10" );
  standard->add_action( "bonestorm,if=runic_power>=100&!buff.dancing_rune_weapon.up" );
  standard->add_action( "death_strike,if=runic_power.deficit<=(15+buff.dancing_rune_weapon.up*5+spell_targets.heart_strike*talent.heartbreaker.enabled*2)|target.1.time_to_die<10" );
  standard->add_action( "death_and_decay,if=spell_targets.death_and_decay>=3" );
  standard->add_action( "heart_strike,if=buff.dancing_rune_weapon.up|rune.time_to_4<gcd" );
  standard->add_action( "blood_boil,if=buff.dancing_rune_weapon.up" );
  standard->add_action( "blood_tap,if=rune.time_to_3>gcd" );
  standard->add_action( "death_and_decay,if=buff.crimson_scourge.up|talent.rapid_decomposition.enabled|spell_targets.death_and_decay>=2" );
  standard->add_action( "consumption" );
  standard->add_action( "blood_boil,if=charges_fractional>=1.1" );
  standard->add_action( "heart_strike,if=(rune>1&(rune.time_to_3<gcd|buff.bone_shield.stack>7))" );
  standard->add_action( "arcane_torrent,if=runic_power.deficit>20" );
}
//blood_apl_end

//frost_apl_start
void frost( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list             ( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list            ( "precombat" );
  action_priority_list_t* aoe = p->get_action_priority_list                  ( "aoe" );
  action_priority_list_t* bos_pooling = p->get_action_priority_list          ( "bos_pooling" );
  action_priority_list_t* bos_ticking = p->get_action_priority_list          ( "bos_ticking" );
  action_priority_list_t* cold_heart = p->get_action_priority_list           ( "cold_heart" );
  action_priority_list_t* cooldowns = p->get_action_priority_list            ( "cooldowns" );
  action_priority_list_t* covenants = p->get_action_priority_list            ( "covenants" );
  action_priority_list_t* obliteration = p->get_action_priority_list         ( "obliteration" );
  action_priority_list_t* obliteration_pooling = p->get_action_priority_list ( "obliteration_pooling" );
  action_priority_list_t* standard = p->get_action_priority_list             ( "standard" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  default_->add_action( "auto_attack" );
  default_->add_action( "howling_blast,if=!dot.frost_fever.ticking&(talent.icecap|cooldown.breath_of_sindragosa.remains>15|talent.obliteration&cooldown.pillar_of_frost.remains&!buff.killing_machine.up)", "Apply Frost Fever and maintain Icy Talons" );
  default_->add_action( "glacial_advance,if=buff.icy_talons.remains<=gcd&buff.icy_talons.up&spell_targets.glacial_advance>=2&(!talent.breath_of_sindragosa|cooldown.breath_of_sindragosa.remains>15)" );
  default_->add_action( "frost_strike,if=buff.icy_talons.remains<=gcd&buff.icy_talons.up&(!talent.breath_of_sindragosa|cooldown.breath_of_sindragosa.remains>15)" );
  default_->add_action( "call_action_list,name=covenants", "Choose Action list to run" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=cold_heart,if=talent.cold_heart&(buff.cold_heart.stack>=10&(debuff.razorice.stack=5|!death_knight.runeforge.razorice)|fight_remains<=gcd)" );
  default_->add_action( "run_action_list,name=bos_ticking,if=buff.breath_of_sindragosa.up" );
  default_->add_action( "run_action_list,name=bos_pooling,if=talent.breath_of_sindragosa&(cooldown.breath_of_sindragosa.remains<10)" );
  default_->add_action( "run_action_list,name=obliteration,if=buff.pillar_of_frost.up&talent.obliteration" );
  default_->add_action( "run_action_list,name=obliteration_pooling,if=talent.obliteration&cooldown.pillar_of_frost.remains<10" );
  default_->add_action( "run_action_list,name=aoe,if=active_enemies>=2" );
  default_->add_action( "call_action_list,name=standard" );

  aoe->add_action( "remorseless_winter", "AoE Rotation" );
  aoe->add_action( "glacial_advance,if=talent.frostscythe" );
  aoe->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=cooldown.remorseless_winter.remains<=2*gcd&talent.gathering_storm" );
  aoe->add_action( "howling_blast,if=buff.rime.up" );
  aoe->add_action( "obliterate,if=death_and_decay.ticking&covenant.night_fae&buff.deaths_due.stack>8" );
  aoe->add_action( "frostscythe,if=buff.killing_machine.react&(!death_and_decay.ticking&covenant.night_fae|!covenant.night_fae)" );
  aoe->add_action( "glacial_advance,if=runic_power.deficit<(15+talent.runic_attenuation*3)" );
  aoe->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=runic_power.deficit<(15+talent.runic_attenuation*3)" );
  aoe->add_action( "frostscythe,if=!death_and_decay.ticking&covenant.night_fae|!covenant.night_fae" );
  aoe->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=runic_power.deficit>(25+talent.runic_attenuation*3)" );
  aoe->add_action( "glacial_advance" );
  aoe->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice" );
  aoe->add_action( "horn_of_winter" );
  aoe->add_action( "arcane_torrent" );

  bos_pooling->add_action( "howling_blast,if=buff.rime.up", "Breath of Sindragosa pooling rotation : starts 10s before BoS is available" );
  bos_pooling->add_action( "remorseless_winter,if=active_enemies>=2|rune.time_to_5<=gcd&(talent.gathering_storm|conduit.everfrost|runeforge.biting_cold)" );
  bos_pooling->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=runic_power.deficit>=25", "'target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice' Repeats a lot, this is intended to target the highest priority enemy with an ability that will apply razorice if runeforged. That being an enemy with 0 stacks, or an enemy that the debuff will soon expire on." );
  bos_pooling->add_action( "glacial_advance,if=runic_power.deficit<20&spell_targets.glacial_advance>=2&cooldown.pillar_of_frost.remains>5" );
  bos_pooling->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=runic_power.deficit<20&cooldown.pillar_of_frost.remains>5" );
  bos_pooling->add_action( "frostscythe,if=buff.killing_machine.react&runic_power.deficit>(15+talent.runic_attenuation*3)&spell_targets.frostscythe>=2&(buff.deaths_due.stack=8|!death_and_decay.ticking|!covenant.night_fae)" );
  bos_pooling->add_action( "frostscythe,if=runic_power.deficit>=(35+talent.runic_attenuation*3)&spell_targets.frostscythe>=2&(buff.deaths_due.stack=8|!death_and_decay.ticking|!covenant.night_fae)" );
  bos_pooling->add_action( "glacial_advance,if=cooldown.pillar_of_frost.remains>rune.time_to_4&runic_power.deficit<40&spell_targets.glacial_advance>=2" );
  bos_pooling->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=cooldown.pillar_of_frost.remains>rune.time_to_4&runic_power.deficit<40" );

  bos_ticking->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=runic_power.deficit>=60", "Breath of Sindragosa Active Rotation" );
  bos_ticking->add_action( "remorseless_winter,if=talent.gathering_storm|conduit.everfrost|runeforge.biting_cold|active_enemies>=2" );
  bos_ticking->add_action( "howling_blast,if=buff.rime.up&(runic_power.deficit<55|rune.time_to_3<=gcd|spell_targets.howling_blast>=2)" );
  bos_ticking->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=rune.time_to_4<gcd|runic_power.deficit>=45" );
  bos_ticking->add_action( "frostscythe,if=buff.killing_machine.up&spell_targets.frostscythe>=2&(!death_and_decay.ticking&covenant.night_fae|!covenant.night_fae)" );
  bos_ticking->add_action( "horn_of_winter,if=runic_power.deficit>=40&rune.time_to_3>gcd" );
  bos_ticking->add_action( "frostscythe,if=spell_targets.frostscythe>=2&(buff.deaths_due.stack=8|!death_and_decay.ticking|!covenant.night_fae)" );
  bos_ticking->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=runic_power.deficit>25&rune>3" );
  bos_ticking->add_action( "howling_blast,if=buff.rime.up" );
  bos_ticking->add_action( "arcane_torrent,if=runic_power.deficit>50" );

  cold_heart->add_action( "chains_of_ice,if=fight_remains<gcd", "Cold Heart Conditions" );
  cold_heart->add_action( "chains_of_ice,if=!talent.obliteration&buff.pillar_of_frost.remains<3&buff.pillar_of_frost.up&buff.cold_heart.stack>=10", "Use during Pillar with Icecap/Breath" );
  cold_heart->add_action( "chains_of_ice,if=!talent.obliteration&death_knight.runeforge.fallen_crusader&!buff.pillar_of_frost.up&(buff.cold_heart.stack>=16&buff.unholy_strength.up|buff.cold_heart.stack>=19&cooldown.pillar_of_frost.remains>10)", "Outside of Pillar useage with Icecap/Breath" );
  cold_heart->add_action( "chains_of_ice,if=!talent.obliteration&!death_knight.runeforge.fallen_crusader&buff.cold_heart.stack>=10&!buff.pillar_of_frost.up&cooldown.pillar_of_frost.remains>20" );
  cold_heart->add_action( "chains_of_ice,if=talent.obliteration&!buff.pillar_of_frost.up&(buff.cold_heart.stack>=16&buff.unholy_strength.up|buff.cold_heart.stack>=19|cooldown.pillar_of_frost.remains<3&buff.cold_heart.stack>=14)", "Prevent Cold Heart overcapping during pillar" );

  cooldowns->add_action( "use_items,if=cooldown.pillar_of_frost.ready|cooldown.pillar_of_frost.remains>20", "On Use Items, Potion and Racials" );
  cooldowns->add_action( "potion,if=buff.pillar_of_frost.up&buff.empower_rune_weapon.up" );
  cooldowns->add_action( "blood_fury,if=buff.pillar_of_frost.up&buff.empower_rune_weapon.up" );
  cooldowns->add_action( "berserking,if=buff.pillar_of_frost.up" );
  cooldowns->add_action( "arcane_pulse,if=(!buff.pillar_of_frost.up&active_enemies>=2)|!buff.pillar_of_frost.up&(rune.deficit>=5&runic_power.deficit>=60)" );
  cooldowns->add_action( "lights_judgment,if=buff.pillar_of_frost.up" );
  cooldowns->add_action( "ancestral_call,if=buff.pillar_of_frost.up&buff.empower_rune_weapon.up" );
  cooldowns->add_action( "fireblood,if=buff.pillar_of_frost.remains<=8&buff.empower_rune_weapon.up" );
  cooldowns->add_action( "bag_of_tricks,if=buff.pillar_of_frost.up&active_enemies=1&(buff.pillar_of_frost.remains<5&talent.cold_heart.enabled|!talent.cold_heart.enabled&buff.pillar_of_frost.remains<3)" );
  cooldowns->add_action( "empower_rune_weapon,if=talent.obliteration&(cooldown.pillar_of_frost.ready&rune.time_to_5>gcd&runic_power.deficit>=10|buff.pillar_of_frost.up&rune.time_to_5>gcd)|fight_remains<20", "Cooldowns" );
  cooldowns->add_action( "empower_rune_weapon,if=talent.breath_of_sindragosa&runic_power.deficit>40&rune.time_to_5>gcd&(buff.breath_of_sindragosa.up|fight_remains<20)" );
  cooldowns->add_action( "empower_rune_weapon,if=talent.icecap&rune<3" );
  cooldowns->add_action( "pillar_of_frost,if=talent.breath_of_sindragosa&(cooldown.breath_of_sindragosa.remains|cooldown.breath_of_sindragosa.ready&runic_power.deficit<60)" );
  cooldowns->add_action( "pillar_of_frost,if=talent.icecap&!buff.pillar_of_frost.up" );
  cooldowns->add_action( "pillar_of_frost,if=talent.obliteration&(talent.gathering_storm.enabled&buff.remorseless_winter.up|!talent.gathering_storm.enabled)" );
  cooldowns->add_action( "breath_of_sindragosa,if=buff.pillar_of_frost.up" );
  cooldowns->add_action( "frostwyrms_fury,if=buff.pillar_of_frost.remains<gcd&buff.pillar_of_frost.up&!talent.obliteration" );
  cooldowns->add_action( "frostwyrms_fury,if=active_enemies>=2&(buff.pillar_of_frost.up&buff.pillar_of_frost.remains<gcd|raid_event.adds.exists&raid_event.adds.remains<gcd|fight_remains<gcd)" );
  cooldowns->add_action( "frostwyrms_fury,if=talent.obliteration&!buff.pillar_of_frost.up&((buff.unholy_strength.up|!death_knight.runeforge.fallen_crusader)&(debuff.razorice.stack=5|!death_knight.runeforge.razorice))" );
  cooldowns->add_action( "hypothermic_presence,if=talent.breath_of_sindragosa&runic_power.deficit>40&rune>=3&buff.pillar_of_frost.up|!talent.breath_of_sindragosa&runic_power.deficit>=25" );
  cooldowns->add_action( "raise_dead,if=buff.pillar_of_frost.up" );
  cooldowns->add_action( "sacrificial_pact,if=active_enemies>=2&(pet.ghoul.remains<gcd|target.time_to_die<gcd)" );
  cooldowns->add_action( "death_and_decay,if=active_enemies>5|runeforge.phearomones" );

  covenants->add_action( "deaths_due,if=raid_event.adds.in>15|!raid_event.adds.exists|active_enemies>=2", "Covenant Abilities" );
  covenants->add_action( "swarming_mist,if=active_enemies=1&runic_power.deficit>3&cooldown.pillar_of_frost.remains<3&!talent.breath_of_sindragosa&(!raid_event.adds.exists|raid_event.adds.in>15)" );
  covenants->add_action( "swarming_mist,if=active_enemies>=2&!talent.breath_of_sindragosa" );
  covenants->add_action( "swarming_mist,if=talent.breath_of_sindragosa&(buff.breath_of_sindragosa.up&(active_enemies=1&runic_power.deficit>40|active_enemies>=2&runic_power.deficit>60)|!buff.breath_of_sindragosa.up&cooldown.breath_of_sindragosa.remains)" );
  covenants->add_action( "abomination_limb,if=active_enemies=1&cooldown.pillar_of_frost.remains<3&(!raid_event.adds.exists|raid_event.adds.in>15)" );
  covenants->add_action( "abomination_limb,if=active_enemies>=2" );
  covenants->add_action( "shackle_the_unworthy,if=active_enemies=1&cooldown.pillar_of_frost.remains<3&(!raid_event.adds.exists|raid_event.adds.in>15)" );
  covenants->add_action( "shackle_the_unworthy,if=active_enemies>=2" );

  obliteration->add_action( "remorseless_winter,if=active_enemies>=3&(talent.gathering_storm|conduit.everfrost|runeforge.biting_cold)", "Obliteration rotation" );
  obliteration->add_action( "howling_blast,if=!dot.frost_fever.ticking&!buff.killing_machine.up" );
  obliteration->add_action( "frostscythe,if=buff.killing_machine.react&spell_targets.frostscythe>=2&(buff.deaths_due.stack=8|!death_and_decay.ticking|!covenant.night_fae)" );
  obliteration->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=buff.killing_machine.react|!buff.rime.up&spell_targets.howling_blast>=3" );
  obliteration->add_action( "glacial_advance,if=spell_targets.glacial_advance>=2&(runic_power.deficit<10|rune.time_to_2>gcd)|(debuff.razorice.stack<5|debuff.razorice.remains<15)" );
  obliteration->add_action( "frost_strike,if=conduit.eradicating_blow&buff.eradicating_blow.stack=2&active_enemies=1" );
  obliteration->add_action( "howling_blast,if=buff.rime.up&spell_targets.howling_blast>=2" );
  obliteration->add_action( "glacial_advance,if=spell_targets.glacial_advance>=2" );
  obliteration->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=!talent.avalanche&!buff.killing_machine.up|talent.avalanche&!buff.rime.up" );
  obliteration->add_action( "howling_blast,if=buff.rime.up" );
  obliteration->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice" );

  obliteration_pooling->add_action( "remorseless_winter,if=talent.gathering_storm|conduit.everfrost|runeforge.biting_cold|active_enemies>=2", "Pooling For Obliteration: Starts 10 seconds before Pillar of Frost comes off CD" );
  obliteration_pooling->add_action( "howling_blast,if=buff.rime.up" );
  obliteration_pooling->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=buff.killing_machine.react" );
  obliteration_pooling->add_action( "glacial_advance,if=spell_targets.glacial_advance>=2&runic_power.deficit<60" );
  obliteration_pooling->add_action( "frost_strike,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=runic_power.deficit<70" );
  obliteration_pooling->add_action( "obliterate,target_if=max:(debuff.razorice.stack+1)%(debuff.razorice.remains+1)*death_knight.runeforge.razorice,if=rune>4" );
  obliteration_pooling->add_action( "frostscythe,if=active_enemies>=4&(!death_and_decay.ticking&covenant.night_fae|!covenant.night_fae)" );

  standard->add_action( "remorseless_winter,if=talent.gathering_storm|conduit.everfrost|runeforge.biting_cold", "Standard single-target rotation" );
  standard->add_action( "glacial_advance,if=!death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<7)" );
  standard->add_action( "frost_strike,if=cooldown.remorseless_winter.remains<=2*gcd&talent.gathering_storm" );
  standard->add_action( "frost_strike,if=conduit.eradicating_blow&buff.eradicating_blow.stack=2|conduit.unleashed_frenzy&buff.unleashed_frenzy.remains<3&buff.unleashed_frenzy.up" );
  standard->add_action( "howling_blast,if=buff.rime.up" );
  standard->add_action( "obliterate,if=!buff.frozen_pulse.up&talent.frozen_pulse|buff.killing_machine.react|death_and_decay.ticking&covenant.night_fae&buff.deaths_due.stack>8|rune.time_to_4<=gcd" );
  standard->add_action( "frost_strike,if=runic_power.deficit<(15+talent.runic_attenuation*3)" );
  standard->add_action( "obliterate,if=runic_power.deficit>(25+talent.runic_attenuation*3)" );
  standard->add_action( "frost_strike" );
  standard->add_action( "horn_of_winter" );
  standard->add_action( "arcane_torrent" );
}
//frost_apl_end

//unholy_apl_start
void unholy( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list    ( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list   ( "precombat" );
  action_priority_list_t* aoe_burst = p->get_action_priority_list   ( "aoe_burst" );
  action_priority_list_t* aoe_setup = p->get_action_priority_list   ( "aoe_setup" );
  action_priority_list_t* cooldowns = p->get_action_priority_list   ( "cooldowns" );
  action_priority_list_t* covenants = p->get_action_priority_list   ( "covenants" );
  action_priority_list_t* generic = p->get_action_priority_list     ( "generic" );
  action_priority_list_t* generic_aoe = p->get_action_priority_list ( "generic_aoe" );
  action_priority_list_t* trinkets = p->get_action_priority_list    ( "trinkets" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "raise_dead" );

  default_->add_action( "auto_attack" );
  default_->add_action( "variable,name=pooling_runic_power,value=cooldown.summon_gargoyle.remains<5&talent.summon_gargoyle", "Variables" );
  default_->add_action( "variable,name=pooling_runes,value=talent.soul_reaper&rune<2&target.time_to_pct_35<5&fight_remains>5" );
  default_->add_action( "variable,name=st_planning,value=active_enemies=1&(!raid_event.adds.exists|raid_event.adds.in>15)" );
  default_->add_action( "variable,name=major_cooldowns_active,value=pet.gargoyle.active|buff.unholy_assault.up|talent.army_of_the_damned&pet.apoc_ghoul.active|buff.dark_transformation.up" );
  default_->add_action( "arcane_torrent,if=runic_power.deficit>65&(pet.gargoyle.active|!talent.summon_gargoyle.enabled)&rune.deficit>=5", "Racials" );
  default_->add_action( "blood_fury,if=variable.major_cooldowns_active|target.time_to_die<=buff.blood_fury.duration" );
  default_->add_action( "berserking,if=variable.major_cooldowns_active|target.time_to_die<=buff.berserking.duration" );
  default_->add_action( "lights_judgment,if=buff.unholy_strength.up" );
  default_->add_action( "ancestral_call,if=variable.major_cooldowns_active|target.time_to_die<=15", "Ancestral Call can trigger 4 potential buffs, each lasting 15 seconds. Utilized hard coded time as a trigger to keep it readable." );
  default_->add_action( "arcane_pulse,if=active_enemies>=2|(rune.deficit>=5&runic_power.deficit>=60)" );
  default_->add_action( "fireblood,if=variable.major_cooldowns_active|target.time_to_die<=buff.fireblood.duration" );
  default_->add_action( "bag_of_tricks,if=buff.unholy_strength.up&active_enemies=1" );
  default_->add_action( "outbreak,if=dot.virulent_plague.refreshable&!talent.unholy_blight&!raid_event.adds.exists", "Maintaining Virulent Plague is a priority" );
  default_->add_action( "outbreak,if=dot.virulent_plague.refreshable&active_enemies>=2&(!talent.unholy_blight|talent.unholy_blight&cooldown.unholy_blight.remains)" );
  default_->add_action( "outbreak,if=runeforge.superstrain&(dot.frost_fever.refreshable|dot.blood_plague.refreshable)" );
  default_->add_action( "wait_for_cooldown,name=soul_reaper,if=talent.soul_reaper&target.time_to_pct_35<5&fight_remains>5&cooldown.soul_reaper.remains<(gcd*0.75)&active_enemies=1" );
  default_->add_action( "call_action_list,name=trinkets", "Action Lists and Openers" );
  default_->add_action( "call_action_list,name=covenants" );
  default_->add_action( "sequence,if=active_enemies=1&!death_knight.disable_aotd,name=opener:army_of_the_dead:festering_strike:festering_strike:potion:unholy_blight:dark_transformation:apocalypse" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "run_action_list,name=aoe_setup,if=active_enemies>=2&(cooldown.death_and_decay.remains<10&!talent.defile|cooldown.defile.remains<10&talent.defile)&!death_and_decay.ticking" );
  default_->add_action( "run_action_list,name=aoe_burst,if=active_enemies>=2&death_and_decay.ticking" );
  default_->add_action( "run_action_list,name=generic_aoe,if=active_enemies>=2&(!death_and_decay.ticking&(cooldown.death_and_decay.remains>10&!talent.defile|cooldown.defile.remains>10&talent.defile))" );
  default_->add_action( "call_action_list,name=generic,if=active_enemies=1" );

  aoe_burst->add_action( "death_coil,if=(buff.sudden_doom.react|!variable.pooling_runic_power)&(buff.dark_transformation.up&runeforge.deadliest_coil&active_enemies<=3|active_enemies=2)", "AoE Burst" );
  aoe_burst->add_action( "epidemic,if=runic_power.deficit<(10+death_knight.fwounded_targets*3)&death_knight.fwounded_targets<6&!variable.pooling_runic_power|buff.swarming_mist.up" );
  aoe_burst->add_action( "epidemic,if=runic_power.deficit<25&death_knight.fwounded_targets>5&!variable.pooling_runic_power" );
  aoe_burst->add_action( "epidemic,if=!death_knight.fwounded_targets&!variable.pooling_runic_power|fight_remains<5|raid_event.adds.exists&raid_event.adds.remains<5" );
  aoe_burst->add_action( "wound_spender" );
  aoe_burst->add_action( "epidemic,if=!variable.pooling_runic_power" );

  aoe_setup->add_action( "any_dnd,if=death_knight.fwounded_targets=active_enemies|raid_event.adds.exists&raid_event.adds.remains<=11", "AoE Setup" );
  aoe_setup->add_action( "any_dnd,if=death_knight.fwounded_targets>=5" );
  aoe_setup->add_action( "death_coil,if=!variable.pooling_runic_power&(buff.dark_transformation.up&runeforge.deadliest_coil&active_enemies<=3|active_enemies=2)" );
  aoe_setup->add_action( "epidemic,if=!variable.pooling_runic_power" );
  aoe_setup->add_action( "festering_strike,target_if=max:debuff.festering_wound.stack,if=debuff.festering_wound.stack<=3&cooldown.apocalypse.remains<3" );
  aoe_setup->add_action( "festering_strike,target_if=debuff.festering_wound.stack<1" );
  aoe_setup->add_action( "festering_strike,target_if=min:debuff.festering_wound.stack,if=rune.time_to_4<(cooldown.death_and_decay.remains&!talent.defile|cooldown.defile.remains&talent.defile)" );

  cooldowns->add_action( "potion,if=variable.major_cooldowns_active|fight_remains<26", "Potion" );
  cooldowns->add_action( "army_of_the_dead,if=cooldown.unholy_blight.remains<5&cooldown.dark_transformation.remains_expected<5&talent.unholy_blight|!talent.unholy_blight|fight_remains<35", "Cooldowns" );
  cooldowns->add_action( "soul_reaper,target_if=target.time_to_pct_35<5&target.time_to_die>5" );
  cooldowns->add_action( "unholy_blight,if=variable.st_planning&(cooldown.apocalypse.remains_expected<5|cooldown.apocalypse.remains_expected>10)&(cooldown.dark_transformation.remains<gcd|buff.dark_transformation.up)", "Holds Blight for up to 5 seconds to sync with Apocalypse, Otherwise, use with Dark Transformation." );
  cooldowns->add_action( "unholy_blight,if=active_enemies>=2|fight_remains<21" );
  cooldowns->add_action( "dark_transformation,if=variable.st_planning&(dot.unholy_blight_dot.remains|!talent.unholy_blight)" );
  cooldowns->add_action( "dark_transformation,if=active_enemies>=2|fight_remains<21" );
  cooldowns->add_action( "apocalypse,if=active_enemies=1&debuff.festering_wound.stack>=4" );
  cooldowns->add_action( "apocalypse,target_if=max:debuff.festering_wound.stack,if=active_enemies>=2&debuff.festering_wound.stack>=4&!death_and_decay.ticking" );
  cooldowns->add_action( "summon_gargoyle,if=runic_power.deficit<14&(cooldown.unholy_blight.remains<10|dot.unholy_blight_dot.remains)" );
  cooldowns->add_action( "unholy_assault,if=variable.st_planning&debuff.festering_wound.stack<2&(pet.apoc_ghoul.active|buff.dark_transformation.up&!pet.army_ghoul.active)" );
  cooldowns->add_action( "unholy_assault,target_if=min:debuff.festering_wound.stack,if=active_enemies>=2&debuff.festering_wound.stack<2" );
  cooldowns->add_action( "raise_dead,if=!pet.ghoul.active" );
  cooldowns->add_action( "sacrificial_pact,if=active_enemies>=2&!buff.dark_transformation.up&!cooldown.dark_transformation.ready|fight_remains<gcd" );

  covenants->add_action( "swarming_mist,if=variable.st_planning&runic_power.deficit>16&(cooldown.apocalypse.remains|!talent.army_of_the_damned&cooldown.dark_transformation.remains)|fight_remains<11", "Covenant Abilities" );
  covenants->add_action( "swarming_mist,if=cooldown.apocalypse.remains&(active_enemies>=2&active_enemies<=5&runic_power.deficit>10+(active_enemies*6)|active_enemies>5&runic_power.deficit>40)", "Set to use after apoc is on CD as to prevent overcapping RP while setting up CD's" );
  covenants->add_action( "abomination_limb,if=variable.st_planning&!soulbind.lead_by_example&(cooldown.apocalypse.remains|!talent.army_of_the_damned&cooldown.dark_transformation.remains)&rune.time_to_4>(3+buff.runic_corruption.remains)|fight_remains<21" );
  covenants->add_action( "abomination_limb,if=variable.st_planning&soulbind.lead_by_example&(dot.unholy_blight_dot.remains>11|!talent.unholy_blight&cooldown.dark_transformation.remains)" );
  covenants->add_action( "abomination_limb,if=active_enemies>=2&rune.time_to_4>(3+buff.runic_corruption.remains)" );
  covenants->add_action( "shackle_the_unworthy,if=variable.st_planning&(cooldown.apocalypse.remains|!talent.army_of_the_damned&cooldown.dark_transformation.remains)|fight_remains<15" );
  covenants->add_action( "shackle_the_unworthy,if=active_enemies>=2&(death_and_decay.ticking|raid_event.adds.remains<=14)" );

  generic->add_action( "death_coil,if=buff.sudden_doom.react&!variable.pooling_runic_power|pet.gargoyle.active", "Single Target" );
  generic->add_action( "death_coil,if=runic_power.deficit<13|fight_remains<5&!debuff.festering_wound.up" );
  generic->add_action( "any_dnd,if=cooldown.apocalypse.remains&(talent.defile.enabled|covenant.night_fae|runeforge.phearomones)&(!variable.pooling_runes|fight_remains<5)" );
  generic->add_action( "wound_spender,if=debuff.festering_wound.stack>4&!variable.pooling_runes" );
  generic->add_action( "wound_spender,if=debuff.festering_wound.up&cooldown.apocalypse.remains_expected>5&!variable.pooling_runes" );
  generic->add_action( "death_coil,if=runic_power.deficit<20&!variable.pooling_runic_power" );
  generic->add_action( "festering_strike,if=debuff.festering_wound.stack<1&!variable.pooling_runes" );
  generic->add_action( "festering_strike,if=debuff.festering_wound.stack<4&cooldown.apocalypse.remains_expected<5&!variable.pooling_runes" );
  generic->add_action( "death_coil,if=!variable.pooling_runic_power" );

  generic_aoe->add_action( "wait_for_cooldown,name=soul_reaper,if=talent.soul_reaper&target.time_to_pct_35<5&fight_remains>5&cooldown.soul_reaper.remains<(gcd*0.75)&active_enemies<=3", "Generic AoE Priority" );
  generic_aoe->add_action( "death_coil,if=(!variable.pooling_runic_power|buff.sudden_doom.react)&(buff.dark_transformation.up&runeforge.deadliest_coil&active_enemies<=3|active_enemies=2)" );
  generic_aoe->add_action( "epidemic,if=buff.sudden_doom.react|!variable.pooling_runic_power" );
  generic_aoe->add_action( "wound_spender,target_if=max:debuff.festering_wound.stack,if=(cooldown.apocalypse.remains>5&debuff.festering_wound.up|debuff.festering_wound.stack>4)&(fight_remains<cooldown.death_and_decay.remains+10|fight_remains>cooldown.apocalypse.remains)" );
  generic_aoe->add_action( "festering_strike,target_if=max:debuff.festering_wound.stack,if=debuff.festering_wound.stack<=3&cooldown.apocalypse.remains<3|debuff.festering_wound.stack<1" );
  generic_aoe->add_action( "festering_strike,target_if=min:debuff.festering_wound.stack,if=cooldown.apocalypse.remains>5&debuff.festering_wound.stack<1" );
  
  trinkets->add_action( "use_item,name=inscrutable_quantum_device,if=(cooldown.unholy_blight.remains|cooldown.dark_transformation.remains)&(pet.army_ghoul.active|pet.apoc_ghoul.active&!talent.army_of_the_damned|target.time_to_pct_20<5)|fight_remains<21", "Trinkets" );
  trinkets->add_action( "use_item,name=macabre_sheet_music,if=cooldown.apocalypse.remains<5&(!equipped.inscrutable_quantum_device|cooldown.inscrutable_quantum_device.remains)|fight_remains<21" );
  trinkets->add_action( "use_item,name=dreadfire_vessel,if=cooldown.apocalypse.remains&(!equipped.inscrutable_quantum_device|cooldown.inscrutable_quantum_device.remains)|fight_remains<3" );
  trinkets->add_action( "use_item,name=darkmoon_deck_voracity,if=cooldown.apocalypse.remains&(!equipped.inscrutable_quantum_device|cooldown.inscrutable_quantum_device.remains)|fight_remains<21" );
  trinkets->add_action( "use_items,if=(cooldown.apocalypse.remains|buff.dark_transformation.up)&(!equipped.inscrutable_quantum_device|cooldown.inscrutable_quantum_device.remains)" );
}
//unholy_apl_end

}  // namespace death_knight_apl
