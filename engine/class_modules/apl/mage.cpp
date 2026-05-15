#include "class_modules/apl/mage.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace mage_apl {

std::string potion( const player_t* p )
{
  std::string lvl90_potion = "disabled";

  switch ( p->specialization() )
  {
    case MAGE_ARCANE: lvl90_potion = "lights_potential_2"; break;
    case MAGE_FIRE: lvl90_potion = "lights_potential_2"; break;
    case MAGE_FROST: lvl90_potion = "potion_of_recklessness_2"; break;
    default: break;
  }
  
  return p->true_level >= 90 ? lvl90_potion
       : p->true_level >= 80 ? "tempered_potion_3"    
       : p->true_level >= 70 ? "elemental_potion_of_ultimate_power_3"
       : p->true_level >= 60 ? "spectral_intellect"
       : p->true_level >= 50 ? "superior_battle_potion_of_intellect"
       :                       "disabled";
}

std::string flask( const player_t* p )
{
  std::string lvl90_flask = "disabled";

  switch ( p->specialization() )
  {
    case MAGE_ARCANE: lvl90_flask = "flask_of_thalassian_resistance_2"; break;
    case MAGE_FIRE: lvl90_flask = "flask_of_the_magisters_2"; break;
    case MAGE_FROST: lvl90_flask = "flask_of_the_shattered_sun_2"; break;
    default: break;
  }
  
  return p->true_level >= 90 ? lvl90_flask
       : p->true_level >= 80 ? "flask_of_alchemical_chaos_3"    
       : p->true_level >= 70 ? "phial_of_tepid_versatility_3"
       : p->true_level >= 60 ? "spectral_flask_of_power"
       : p->true_level >= 50 ? "greater_flask_of_endless_fathoms"
       :                       "disabled";
}

std::string food( const player_t* p )
{
  return p->true_level >= 90 ? "silvermoon_parade"
       : p->true_level >= 80 ? "feast_of_the_midnight_masquerade"    
       : p->true_level >= 70 ? "fated_fortune_cookie"
       : p->true_level >= 60 ? "feast_of_gluttonous_hedonism"
       : p->true_level >= 50 ? "famine_evaluator_and_snack_table"
       :                       "disabled";
}

std::string rune( const player_t* p )
{
  return p->true_level >= 90 ? "void_touched"
       : p->true_level >= 80 ? "crystallized"    
       : p->true_level >= 70 ? "draconic"
       : p->true_level >= 60 ? "veiled"
       : p->true_level >= 50 ? "battle_scarred"
       :                       "disabled";
}

std::string temporary_enchant( const player_t* p )
{
  return p->true_level >= 90 ? "main_hand:thalassian_phoenix_oil_2"
       : p->true_level >= 80 ? "main_hand:algari_mana_oil_3"    
       : p->true_level >= 70 ? "main_hand:buzzing_rune_3"
       : p->true_level >= 60 ? "main_hand:shadowcore_oil"
       :                       "disabled";
}

//arcane_apl_start
void arcane( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* spellslinger = p->get_action_priority_list( "spellslinger" );
  action_priority_list_t* spellslinger_orbm = p->get_action_priority_list( "spellslinger_orbm" );
  action_priority_list_t* sunfury = p->get_action_priority_list( "sunfury" );

  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "variable,name=opener,op=set,value=1" );
  precombat->add_action( "variable,name=pulse_aoe_count,op=set,value=2+talent.orb_mastery" );
  precombat->add_action( "variable,name=funnel,op=reset,default=0" );
  precombat->add_action( "variable,name=sf_touch_surge,op=reset,default=0" );
  precombat->add_action( "variable,name=pooling,op=reset,default=1" );
  precombat->add_action( "variable,name=time_for_pooling,op=set,value=(((fight_remains%%95)<(7+(2*talent.arcane_pulse)))|((fight_remains%%95)>(20+(2*talent.arcane_pulse))))&variable.pooling" );
  precombat->add_action( "variable,name=did_not_pool,op=set,value=((fight_remains%%95)<(7+(2*talent.arcane_pulse)))|((fight_remains%%95)>(20+(2*talent.arcane_pulse)))&variable.pooling" );
  precombat->add_action( "variable,name=20ssteroid_trinket_equipped,op=set,value=equipped.signet_of_the_priory|equipped.incorporeal_essencegorger|equipped.sealed_chaos_urn" );
  precombat->add_action( "variable,name=15ssteroid_trinket_equipped,op=set,value=equipped.lily_of_the_eternal_weave|equipped.sunblood_amethyst|equipped.astral_gladiators_badge_of_ferocity|equipped.arazs_ritual_forge|equipped.freightrunners_flask|equipped.emberwing_feather|equipped.vaelgors_final_stare|equipped.galactic_gladiators_badge_of_ferocity" );
  precombat->add_action( "variable,name=12ssteroid_trinket_equipped,op=set,value=equipped.nevermelting_ice_crystal|equipped.ever_collapsing_void_fissure" );
  precombat->add_action( "variable,name=steroid_trinket_equipped,op=set,value=equipped.lily_of_the_eternal_weave|equipped.sunblood_amethyst|equipped.astral_gladiators_badge_of_ferocity|equipped.arazs_ritual_forge|equipped.freightrunners_flask|equipped.emberwing_feather|equipped.vaelgors_final_stare|equipped.galactic_gladiators_badge_of_ferocity|equipped.nevermelting_ice_crystal|equipped.ever_collapsing_void_fissure|equipped.signet_of_the_priory|equipped.incorporeal_essencegorger|equipped.sealed_chaos_urn" );
  precombat->add_action( "variable,name=nonsteroid_trinket_equipped,op=set,value=equipped.mereldars_toll|equipped.perfidious_projector|equipped.chaotic_nethergate|equipped.wraps_of_cosmic_madness|equipped.astalors_anguish_agitator" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "potion,if=talent.spellfire_spheres&!variable.sf_touch_surge" );
  precombat->add_action( "arcane_surge,if=(talent.spellfire_spheres&!variable.sf_touch_surge)|(!variable.time_for_pooling&talent.splintering_sorcery)" );
  precombat->add_action( "arcane_pulse,if=(talent.splintering_sorcery|variable.sf_touch_surge)&talent.arcane_pulse&(active_enemies>=variable.pulse_aoe_count)" );
  precombat->add_action( "arcane_blast" );

  default_->add_action( "counterspell" );
  default_->add_action( "invoke_external_buff,name=power_infusion,if=(buff.arcane_surge.up&(debuff.touch_of_the_magi.up&(talent.splintering_sorcery|variable.sf_touch_surge)|(talent.spellfire_spheres&buff.arcane_surge.up&buff.arcane_surge.remains<(9+gcd.remains))))|fight_remains<16", "Steroid racials and potions are used with cds basically based on cooldown and overlap with the most effects. Non-steroid racials are not worth using under any conditions currently and would need substantial buffs to become useable over our baseline spells and abilities." );
  default_->add_action( "potion,if=(cooldown.arcane_surge.ready&buff.arcane_salvo.react=20+(5*talent.spellfire_spheres))|(buff.arcane_surge.up&(prev_gcd.1.arcane_surge|fight_remains<90))|fight_remains<30|(fight_remains>320&fight_remains<330)" );
  default_->add_action( "berserking,if=(buff.arcane_surge.up&debuff.touch_of_the_magi.up)|fight_remains<13" );
  default_->add_action( "blood_fury,if=(buff.arcane_surge.up&(debuff.touch_of_the_magi.up&(talent.splintering_sorcery|variable.sf_touch_surge)|(talent.spellfire_spheres&buff.arcane_surge.up&buff.arcane_surge.remains<(9+gcd.remains))))|fight_remains<16" );
  default_->add_action( "fireblood,if=(buff.arcane_surge.up&((debuff.touch_of_the_magi.up&(talent.splintering_sorcery|variable.sf_touch_surge))|(talent.spellfire_spheres&buff.arcane_surge.up&buff.arcane_surge.remains<(2+gcd.remains))))|fight_remains<9" );
  default_->add_action( "ancestral_call,if=(buff.arcane_surge.up&(debuff.touch_of_the_magi.up&(talent.splintering_sorcery|variable.sf_touch_surge)|(talent.spellfire_spheres&buff.arcane_surge.up&buff.arcane_surge.remains<(9+gcd.remains))))|fight_remains<16" );
  default_->add_action( "use_items,if=((talent.splintering_sorcery|variable.sf_touch_surge)&((buff.arcane_surge.up&((variable.12ssteroid_trinket_equipped&debuff.touch_of_the_magi.up)|variable.15ssteroid_trinket_equipped))|(cooldown.arcane_surge.ready&variable.20ssteroid_trinket_equipped)))|(talent.spellfire_spheres&buff.arcane_surge.up&((variable.12ssteroid_trinket_equipped&debuff.touch_of_the_magi.up)|(variable.15ssteroid_trinket_equipped&buff.arcane_surge.remains<(9+gcd.remains)))|(buff.arcane_surge.remains<(14+gcd.remains)&variable.20ssteroid_trinket_equipped))|(fight_remains<13&variable.12ssteroid_trinket_equipped)|(fight_remains<16&variable.15ssteroid_trinket_equipped)|(fight_remains<21&variable.20ssteroid_trinket_equipped)|(variable.nonsteroid_trinket_equipped&((buff.arcane_surge.down&cooldown.arcane_surge.remains>20)|!variable.steroid_trinket_equipped))", "Use trinkets condition essentially favors using steroid trinkets during cds, avoids using non-steroids in ways that would conflict with using steroids in cds, otherwise just sends if you don't have a steroid trinket. TODO: Recheck after all trinkets are implemented" );
  default_->add_action( "arcane_barrage,if=fight_remains<gcd.max*2", "End of fight conditions for spending your last bit of resources." );
  default_->add_action( "arcane_missiles,if=fight_remains<execute_time*(1+buff.clearcasting.react)&buff.clearcasting.react&buff.arcane_salvo.stack>=13+(5*talent.spellfire_salvo)&!talent.orb_mastery,chain=1" );
  default_->add_action( "arcane_orb,if=fight_remains<execute_time*(1+buff.clearcasting.react)&buff.clearcasting.react&buff.arcane_salvo.stack>=13+(5*talent.spellfire_salvo)&talent.orb_mastery" );
  default_->add_action( "variable,name=opener,op=set,if=debuff.touch_of_the_magi.up&variable.opener,value=0" );
  default_->add_action( "variable,name=time_for_pooling,op=set,if=!variable.opener,value=1" );
  default_->add_action( "variable,name=sunfury_hold_for_cds,op=set,value=((buff.arcane_surge.down&cooldown.touch_of_the_magi.remains>gcd.max*(4-(active_enemies>=3)-((2*(buff.overpowered_missiles.react&buff.clearcasting.react))<?((cooldown.arcane_orb.charges_fractional>0.95|buff.clearcasting.react)&active_enemies>=3)))&cooldown.arcane_surge.remains>gcd.max*(4-(active_enemies>=3)-((2*(buff.overpowered_missiles.react&buff.clearcasting.react))<?((cooldown.arcane_orb.charges_fractional>0.95|buff.clearcasting.react)&active_enemies>=3))))|((buff.clearcasting.react|((buff.arcane_salvo.react=25|cooldown.arcane_orb.charges_fractional>0.95)&active_enemies>=3))&buff.arcane_surge.remains>gcd.max*(6-(2*(buff.overpowered_missiles.react<?(active_enemies>=3))))))", "This line dictates pooling logic around Touch, Surge, and Soul, the line is daunting but the basic idea is that you don't spend Barrage near your cooldowns unless you have a reliable way to get them back; in AOE this is a little more relaxed. TODO: look into simplifying as well as a similar conditional for Spellslinger if it would help." );
  default_->add_action( "call_action_list,name=cooldowns", "cooldowns section dictates actions that only happen around cooldowns, spellslinger_orbm is for Orb Mastery builds, spellslinger is for non-Orb Mastery builds, sunfury supports only missile builds. TODO: Add Orb Mastery support for Sunfury, much of Sunfury likely needs some reassessment. Look into Charged Missiles tailored sequences for both hero trees." );
  default_->add_action( "call_action_list,name=spellslinger_orbm,if=talent.splintering_sorcery&talent.orb_mastery" );
  default_->add_action( "call_action_list,name=spellslinger,if=talent.splintering_sorcery&!talent.orb_mastery" );
  default_->add_action( "call_action_list,name=sunfury,if=!talent.splintering_sorcery" );
  default_->add_action( "arcane_barrage,if=(time>5&!prev_gcd.1.arcane_surge)|(prev_off_gcd.touch_of_the_magi&buff.arcane_salvo.react=(20+(5*talent.spellfire_salvo)))" );

  cooldowns->add_action( "arcane_orb,if=(talent.splintering_sorcery|variable.sf_touch_surge)&variable.opener&variable.time_for_pooling,line_cd=30", "Orb Mastery Slinger builds throw an Orb right after Blasting on pull, other Spellslinger builds will just go for Touch, and Sunfury opens by spending the Clearcasting from Surge on pull." );
  cooldowns->add_action( "arcane_orb,if=talent.splintering_sorcery&prev_off_gcd.touch_of_the_magi&time<5&buff.arcane_salvo.react<=14,line_cd=999" );
  cooldowns->add_action( "arcane_orb,if=!variable.did_not_pool,line_cd=999" );
  cooldowns->add_action( "arcane_missiles,if=talent.spellfire_spheres&!variable.sf_touch_surge&variable.opener,line_cd=30" );
  cooldowns->add_action( "arcane_pulse,if=(talent.splintering_sorcery|variable.sf_touch_surge)&buff.arcane_salvo.react<20&(variable.opener|(talent.orb_mastery&cooldown.arcane_surge.remains<(gcd.max*(mana.pct%(8+(8*(active_enemies>variable.pulse_aoe_count)))))))&(active_enemies>=variable.pulse_aoe_count)", "Spellslinger builds Salvo before going into cds the first time." );
  cooldowns->add_action( "arcane_blast,if=(talent.splintering_sorcery|variable.sf_touch_surge)&buff.arcane_salvo.react<20&((variable.opener&variable.time_for_pooling)|(!variable.opener&talent.orb_mastery&cooldown.arcane_surge.remains<(gcd.max*(mana.pct%(8+(8*(active_enemies>=2)))))))" );
  cooldowns->add_action( "wait,sec=0.05,if=(prev_gcd.1.arcane_surge&gcd.remains=0)|(prev_off_gcd.touch_of_the_magi&gcd.remains=0)|(prev_off_gcd.presence_of_mind&gcd.remains=0),line_cd=1" );
  cooldowns->add_action( "touch_of_the_magi,use_off_gcd=1,if=((talent.splintering_sorcery|variable.sf_touch_surge)&buff.arcane_surge.up)|(talent.spellfire_spheres&!variable.sf_touch_surge&buff.arcane_surge.up&buff.arcane_surge.remains<(5+gcd.remains))|(cooldown.touch_of_the_magi.ready&cooldown.arcane_surge.remains>30&buff.arcane_surge.down)", "Spellslinger uses Touch after Surge, Sunfury holds touch for the end of Surge to capture Soul and the run-off of resources after Soul." );
  cooldowns->add_action( "arcane_surge" );
  cooldowns->add_action( "cancel_action,if=action.evocation.channeling&mana.pct>=95" );
  cooldowns->add_action( "evocation,if=mana.pct<10&buff.arcane_surge.down&debuff.touch_of_the_magi.down&cooldown.arcane_surge.remains>10" );

  spellslinger->add_action( "arcane_orb,if=buff.arcane_charge.stack<(3+(active_enemies>=2))&(((buff.clearcasting.react=0&talent.high_voltage)|(buff.clearcasting.react&buff.arcane_salvo.react>=12))|(active_enemies>=2))&cooldown.touch_of_the_magi.remains>gcd.max*4", "Orb when you need charges, if you have Clearcasting skip this and get your Charges from Missiles." );
  spellslinger->add_action( "arcane_barrage,if=buff.arcane_salvo.react>=20&(buff.arcane_charge.stack=4|talent.orb_barrage)&cooldown.touch_of_the_magi.remains>gcd.max*(4-(2*(active_enemies>=2)))", "Barrage at 20 Salvo or 18+ with Orb Barrage, Charges are also optional with Orb Barrage. Hold for CDs if near." );
  spellslinger->add_action( "arcane_barrage,if=active_enemies>=2&buff.arcane_charge.stack=4&buff.clearcasting.react&buff.overpowered_missiles.react&talent.high_voltage&buff.arcane_salvo.react>5&buff.arcane_salvo.react<14&cooldown.touch_of_the_magi.remains>gcd.max*4", "Barrage in AOE when you can recoup Charges with Missiles or Orb." );
  spellslinger->add_action( "arcane_missiles,if=buff.clearcasting.react&((buff.arcane_salvo.stack<(10+(5*(buff.overpowered_missiles.react=0))))|(buff.arcane_charge.stack<2&talent.high_voltage&active_enemies>=2)),chain=1", "Missiles for Charges with HV and Salvo stacks." );
  spellslinger->add_action( "presence_of_mind,use_off_gcd=1,if=buff.arcane_charge.stack<2&(buff.clearcasting.react=0|!talent.high_voltage&cooldown.arcane_orb.charges_fractional<0.95)&!prev_gcd.1.arcane_orb&!prev_gcd.1.arcane_missiles" );
  spellslinger->add_action( "arcane_blast,if=buff.presence_of_mind.up" );
  spellslinger->add_action( "arcane_pulse,if=((active_enemies>=variable.pulse_aoe_count)&!variable.funnel)|((buff.arcane_charge.stack<3)&mana.pct>30)" );
  spellslinger->add_action( "arcane_blast" );
  spellslinger->add_action( "arcane_barrage,if=!prev_gcd.1.arcane_surge|prev_off_gcd.touch_of_the_magi&buff.arcane_salvo.react=20" );

  spellslinger_orbm->add_action( "arcane_orb,if=(prev_gcd.1.arcane_barrage|active_enemies>=4)&((buff.clearcasting.react&buff.arcane_salvo.react<=14)|(buff.clearcasting.react=0&(cooldown.arcane_orb.charges_fractional>1.9)&buff.arcane_salvo.react<=18))", "Orb after Barraging with Clearcasting to recoup Charges and Salvo, in AOE just send as long as you won't overcap Salvo. If you don't have CC, only Orb if you'll overcap Orb and need Charges." );
  spellslinger_orbm->add_action( "arcane_barrage,if=(buff.arcane_charge.stack=4|talent.orb_barrage)&buff.arcane_salvo.react>=20&cooldown.touch_of_the_magi.remains>gcd.max*(4-(2*(active_enemies>=2)))|(((buff.arcane_surge.remains<gcd.max&buff.arcane_surge.up)|(debuff.touch_of_the_magi.remains<gcd.max&debuff.touch_of_the_magi.up))&buff.arcane_salvo.react>=15)", "Barrage at 20 stacks, save for Touch, Barrage the end of Touch or Surge for Splinters." );
  spellslinger_orbm->add_action( "arcane_missiles,if=(talent.high_voltage|talent.overpowered_missiles|(buff.clearcasting.react=3))&buff.clearcasting.react&buff.arcane_salvo.react<=(10+(5*(buff.overpowered_missiles.react=0)))&!prev_gcd.1.arcane_orb&(buff.arcane_surge.down|(talent.high_voltage&active_enemies=1))&(active_enemies<2|talent.overpowered_missiles),chain=1", "Missiles only if you have HV or OPM specced and in minimal situations." );
  spellslinger_orbm->add_action( "arcane_barrage,if=buff.arcane_salvo.react<7&buff.arcane_surge.down&buff.touch_of_the_magi.down&buff.arcane_charge.stack=4&talent.resonance&talent.arcane_pulse", "Small benefit when playing with Pulse, due to its mana consumption, its a gain for most profiles to Barrage a little bit more often outside of cds when you lack Orbs." );
  spellslinger_orbm->add_action( "presence_of_mind,use_off_gcd=1,if=buff.arcane_charge.stack<2&(buff.clearcasting.react=0|!talent.high_voltage&cooldown.arcane_orb.charges_fractional<0.95)&!prev_gcd.1.arcane_orb&!prev_gcd.1.arcane_missiles" );
  spellslinger_orbm->add_action( "arcane_blast,if=buff.presence_of_mind.up" );
  spellslinger_orbm->add_action( "arcane_pulse,if=((active_enemies>=variable.pulse_aoe_count)&!variable.funnel)|((buff.arcane_charge.stack<3)&mana.pct>30)" );
  spellslinger_orbm->add_action( "arcane_blast" );
  spellslinger_orbm->add_action( "arcane_barrage,if=(time>5&!prev_gcd.1.arcane_surge)|(prev_off_gcd.touch_of_the_magi&buff.arcane_salvo.react=20)" );

  sunfury->add_action( "arcane_barrage,if=(buff.arcane_charge.stack=4&variable.sunfury_hold_for_cds&((((buff.clearcasting.react&talent.high_voltage)|(cooldown.arcane_orb.charges_fractional>0.95&active_enemies>=3))&((buff.arcane_salvo.react>=6&buff.arcane_salvo.react<7)|(buff.arcane_salvo.react>=12&buff.arcane_salvo.react<13)|(buff.arcane_salvo.react>=18&buff.arcane_salvo.react<19)|((buff.arcane_salvo.react<19)&!talent.resonance&active_enemies>=3)))|buff.arcane_salvo.stack=25))|prev_off_gcd.touch_of_the_magi|(debuff.touch_of_the_magi.remains<gcd.max&debuff.touch_of_the_magi.up&buff.arcane_charge.stack=4)|buff.arcane_soul.up", "Basic idea is simple, Barrage to spend Salvo in increments of 6 to optimize around Meteorite generation when possible with Clearcasting when you run High Voltage, or Orb CD is up in AOE, until you get to the point where 25 isn't far away, for a little more dps you can pool for Touch, Surge, and Soul, pooling logic is above. Extra conditions beyond that are to Barrage at the start and end of Touch and during Soul." );
  sunfury->add_action( "arcane_missiles,if=buff.clearcasting.react&((((cooldown.touch_of_the_magi.remains>gcd.max*(8-(4*variable.sf_touch_surge))&buff.overpowered_missiles.react=0)|buff.arcane_surge.up|buff.arcane_charge.stack<3|buff.clearcasting.react>1)&buff.arcane_salvo.react<(15-(5*(buff.overpowered_missiles.react&buff.arcane_surge.down))))|(debuff.touch_of_the_magi.up&buff.arcane_surge.up)),chain=1", "Missile if you have less than 15 Salvo or 10 with OPM proc except when Surge is up; send Missiles if you have both Surge and Touch going." );
  sunfury->add_action( "arcane_orb,if=buff.arcane_charge.stack<2" );
  sunfury->add_action( "arcane_pulse,if=((active_enemies>=variable.pulse_aoe_count)&!variable.funnel)|((buff.arcane_charge.stack<3)&mana.pct>30)" );
  sunfury->add_action( "arcane_explosion,if=active_enemies>3&buff.arcane_charge.stack<2&!talent.impetus" );
  sunfury->add_action( "arcane_blast", "Barrage can be used if you didn't have any of the charge generators above to get over 1 stacks. This is also not default behavior but is interestingly neutral. actions.sunfury+=/arcane_barrage,if=buff.arcane_charge.stack<2" );
  sunfury->add_action( "arcane_barrage,if=(variable.sf_touch_surge&(!prev_gcd.1.arcane_surge|prev_off_gcd.touch_of_the_magi&buff.arcane_salvo.react=25))|!variable.sf_touch_surge" );
}
//arcane_apl_end

//fire_apl_start
void fire( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* fireblast = p->get_action_priority_list( "fireblast" );
  action_priority_list_t* ff_combustion = p->get_action_priority_list( "ff_combustion" );
  action_priority_list_t* ff_filler = p->get_action_priority_list( "ff_filler" );
  action_priority_list_t* sf_combustion = p->get_action_priority_list( "sf_combustion" );
  action_priority_list_t* sf_filler = p->get_action_priority_list( "sf_filler" );

  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "variable,name=cast_remains_time,value=0.2" );
  precombat->add_action( "variable,name=pooling_time,value=10*gcd.max" );
  precombat->add_action( "variable,name=flamestriking,op=reset,default=1" );
  precombat->add_action( "variable,name=ff_combustion_flamestrike,if=!talent.spellfire_spheres,value=4+(999*!talent.fuel_the_fire)", "Flamestrike at 4 targets during Combustion." );
  precombat->add_action( "variable,name=ff_filler_flamestrike,if=!talent.spellfire_spheres,value=8+(999*!talent.fuel_the_fire)", "Flamestrike at 8 targets." );
  precombat->add_action( "variable,name=sf_combustion_flamestrike,if=talent.spellfire_spheres,value=4+(999*!talent.fuel_the_fire)", "Flamestrike at 4 targets during Combustion. Do at 3 targets if you don't care about prio dmg." );
  precombat->add_action( "variable,name=sf_filler_flamestrike,if=talent.spellfire_spheres,value=4+(999*!talent.fuel_the_fire)", "Flamestrike at 4 targets." );
  precombat->add_action( "variable,name=combustion_delay,value=(18*talent.firestarter)-(10*(expected_combat_length<60)+10*(expected_combat_length<30))-10*(((expected_combat_length%%60)>=25)&((expected_combat_length%%60)<=40))", "Delay Combustion if playing Firestarter until the target is >=90% HP unless it means losing casts of Combustion. Do not do so if fight length is short." );
  precombat->add_action( "variable,name=15ssteroid_trinket_equipped,op=set,value=equipped.nevermelting_ice_crystal|equipped.lily_of_the_eternal_weave|equipped.sunblood_amethyst|equipped.astral_gladiators_badge_of_ferocity|equipped.arazs_ritual_forge|equipped.freightrunners_flask|equipped.emberwing_feather|equipped.vaelgors_final_stare|equipped.galactic_gladiators_badge_of_ferocity" );
  precombat->add_action( "variable,name=10ssteroid_trinket_equipped,op=set,value=equipped.ever_collapsing_void_fissure" );
  precombat->add_action( "variable,name=nonsteroid_trinket_equipped,op=set,value=equipped.mereldars_toll|equipped.perfidious_projector|equipped.chaotic_nethergate|equipped.wraps_of_cosmic_madness|equipped.astalors_anguish_agitator" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "mirror_image" );
  precombat->add_action( "frostfire_bolt,if=talent.frostfire_bolt", "Precast one of these." );
  precombat->add_action( "meteor,if=!talent.firestarter&talent.sunfury_execution" );
  precombat->add_action( "pyroblast" );

  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "run_action_list,name=ff_combustion,if=talent.frostfire_bolt&((time>=variable.combustion_delay)&(cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.up|cooldown.combustion.ready))", "Combustion is delayed on pull 18 seconds for all Firestarter builds to simulate realistic timings for when a boss drops below 90% HP." );
  default_->add_action( "run_action_list,name=sf_combustion,if=!talent.frostfire_bolt&((time>=variable.combustion_delay)&(cooldown.combustion.remains<=variable.combustion_precast_time|buff.combustion.up|cooldown.combustion.ready))" );
  default_->add_action( "run_action_list,name=ff_filler,if=talent.frostfire_bolt" );
  default_->add_action( "run_action_list,name=sf_filler" );

  cds->add_action( "variable,name=combustion_precast_time,value=(action.scorch.cast_time*!buff.pyroclasm.up*scorch_execute.active)+(action.fireball.cast_time*!buff.pyroclasm.up*!scorch_execute.active)+(action.pyroblast.cast_time*buff.pyroclasm.up)-variable.cast_remains_time" );
  cds->add_action( "potion,if=time>=(8*(talent.firestarter&talent.spellfire_spheres))|buff.combustion.remains>6|fight_remains<35", "Use Potion on pull. Delay by about 8 seconds if playing with Firestarter as Sunfury." );
  cds->add_action( "use_item,name=vaelgors_final_stare,if=buff.combustion.remains>6|fight_remains<20", "Force Vaelgor as highest priority on-use trinket, if potentially two on-use trinkets are equipped." );
  cds->add_action( "use_item,name=emberwing_feather,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=nevermelting_ice_crystal,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=freightrunners_flask,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=astral_gladiators_badge_of_ferocity,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=galactic_gladiators_badge_of_ferocity,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_item,name=ever_collapsing_void_fissure,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "use_items,if=variable.nonsteroid_trinket_equipped&time>variable.combustion_delay&buff.combustion.down&buff.hyperthermia.down&cooldown.combustion.remains>20", "Non-steriod trinkets are used outside cooldowns." );
  cds->add_action( "use_items,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "ancestral_call,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "berserking,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "blood_fury,if=buff.combustion.remains>6|fight_remains<20" );
  cds->add_action( "fireblood,if=buff.combustion.remains>6|fight_remains<10" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down&(buff.combustion.remains>6|fight_remains<25)" );

  fireblast->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&(buff.combustion.up|buff.hyperthermia.up)&(hot_streak_spells_in_flight+buff.heating_up.react=1)&gcd.remains<gcd.max", "During Combustion/Hyperthermia, spend Fire Blasts with Heating Up." );
  fireblast->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&(action.fireball.executing&action.fireball.execute_remains>0.1|buff.pyroclasm.react&action.pyroblast.executing&action.pyroblast.execute_remains>0.1)&((target.health.pct>=30|!talent.scorch)&buff.heating_up.react)&(hot_streak_spells_in_flight+buff.heating_up.react=1)&gcd.remains<gcd.max", "During non-execute filler, use Fire Blast with Heating Up while hardcasting Fireball/Frostfire Bolt/Pyroblast." );
  fireblast->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&(target.health.pct<30&talent.scorch)&(hot_streak_spells_in_flight+buff.heating_up.react=0)&action.scorch.executing&buff.heat_shimmer.down&gcd.remains<gcd.max", "During execute, spend Fire Blasts while casting Scorch if you don't have Heating Up." );
  fireblast->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&!buff.hot_streak.react&time<variable.combustion_delay&(talent.firestarter|action.fireball.executing&action.fireball.execute_remains>0.1|buff.pyroclasm.react&action.pyroblast.executing&action.pyroblast.execute_remains>0.1)&(hot_streak_spells_in_flight+buff.heating_up.react=1)&gcd.remains<gcd.max&cooldown.combustion.ready", "While delaying Combustion on pull (Firestarter or not), spend Fire Blasts with Heating Up freely. If not playing Firestarter, only do so during hardcasts." );
  fireblast->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=cooldown_react&(time>=variable.combustion_delay&(cooldown.combustion.remains<=variable.combustion_precast_time))&buff.combustion.down&talent.spontaneous_combustion&(action.scorch.executing|action.fireball.executing|action.pyroblast.executing|action.flamestrike.executing)", "When talented into Spontaneous Combustion, spend all Fire Blasts during the pre-cast going into Combustion regardless of Heating Up / Hot Streak status." );
  fireblast->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=talent.frostfire_bolt&target.health.pct<30&buff.combustion.down&cooldown.combustion.remains>5", "As Frostfire in execute, since we ignore Hot Streak Pyroblast, send Fire Blasts freely." );
  fireblast->add_action( "fire_blast,use_off_gcd=1,use_while_casting=1,if=fight_remains<1", "Spend all available Fire Blasts if fight is ending." );

  ff_combustion->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=buff.combustion.down&action.fireball.executing&(action.fireball.execute_remains<variable.cast_remains_time)|action.meteor.in_flight&(action.meteor.in_flight_remains<0.3)|action.pyroblast.executing&(action.pyroblast.execute_remains<variable.cast_remains_time)|prev_gcd.1.meteor" );
  ff_combustion->add_action( "flamestrike,if=talent.fuel_the_fire&active_enemies>=variable.ff_combustion_flamestrike&variable.flamestriking&(buff.pyroclasm.up&!buff.hot_streak.react&buff.combustion.down)", "Precast into Combustion. Prioritize Pyroclasm if available." );
  ff_combustion->add_action( "pyroblast,if=buff.pyroclasm.up&!buff.hot_streak.react&buff.combustion.down" );
  ff_combustion->add_action( "fireball,if=buff.combustion.down" );
  ff_combustion->add_action( "meteor,if=(talent.burnout&buff.combustion.remains<8)|(!talent.burnout&buff.combustion.remains>2)", "Meteor is used towards the end of Combustion to maximize the Ignite bank for Burnout. If not playing Burnout, just make sure the Meteor lands during Combustion at any time." );
  ff_combustion->add_action( "flamestrike,if=talent.fuel_the_fire&active_enemies>=variable.ff_combustion_flamestrike&variable.flamestriking&(buff.hot_streak.react)", "Spend Hot Streaks on Pyroblast in ST or Flamestrike in AoE." );
  ff_combustion->add_action( "pyroblast,if=buff.hot_streak.react" );
  ff_combustion->add_action( "flamestrike,if=talent.fuel_the_fire&active_enemies>=variable.ff_combustion_flamestrike&variable.flamestriking&(buff.pyroclasm.up&cast_time<buff.combustion.remains)", "Make sure Pyroclasm FINISHES its cast before Combustion ends." );
  ff_combustion->add_action( "pyroblast,if=buff.pyroclasm.up&cast_time<buff.combustion.remains" );
  ff_combustion->add_action( "scorch,if=buff.heat_shimmer.react|talent.scald&target.health.pct<30&buff.frostfire_empowerment.down" );
  ff_combustion->add_action( "fireball" );
  ff_combustion->add_action( "call_action_list,name=fireblast,if=!talent.pyroclasm|(buff.pyroclasm.stack<2|action.pyroblast.executing&action.pyroblast.execute_remains>0.2&buff.pyroclasm.stack=2|cooldown.fire_blast.charges_fractional>=2|buff.combustion.remains<action.pyroblast.cast_time)&(active_enemies<variable.sf_combustion_flamestrike&variable.flamestriking|buff.pyroclasm.down|!action.flamestrike.executing)" );

  ff_filler->add_action( "meteor,if=time>=(variable.combustion_delay-gcd.max)", "Cast Meteor on CD starting from the precast of your first Combustion." );
  ff_filler->add_action( "pyroblast,if=buff.hot_streak.up&talent.firestarter&time<variable.combustion_delay", "During Firestarter, only use Pyroblast as your spender, even for AoE." );
  ff_filler->add_action( "flamestrike,if=talent.fuel_the_fire&active_enemies>=variable.ff_filler_flamestrike&variable.flamestriking&(buff.hot_streak.react&(cooldown.combustion.remains>=5|time<variable.combustion_delay))", "Hold Hot Streak if Combustion is coming up soon. Do not hold if intentionally delaying Combustion." );
  ff_filler->add_action( "pyroblast,if=buff.hot_streak.react&(cooldown.combustion.remains>=(5-(5*buff.pyroclasm.up))|time<variable.combustion_delay)&target.health.pct>30" );
  ff_filler->add_action( "flamestrike,if=talent.fuel_the_fire&active_enemies>=variable.ff_filler_flamestrike&variable.flamestriking&(buff.pyroclasm.up&cooldown.combustion.remains>12|buff.pyroclasm.stack=2)", "Spend Pyroclasm immediately if you have 2 stacks available. Otherwise, hold one stack if it lasts until Combustion comes up." );
  ff_filler->add_action( "pyroblast,if=buff.pyroclasm.up&cooldown.combustion.remains>12|buff.pyroclasm.stack=2" );
  ff_filler->add_action( "scorch,if=buff.heat_shimmer.react" );
  ff_filler->add_action( "fireball" );
  ff_filler->add_action( "call_action_list,name=fireblast" );

  sf_combustion->add_action( "combustion,use_off_gcd=1,use_while_casting=1,if=action.scorch.executing&(action.scorch.execute_remains<variable.cast_remains_time)|action.fireball.executing&(action.fireball.execute_remains<variable.cast_remains_time)|action.pyroblast.executing&(action.pyroblast.execute_remains<variable.cast_remains_time)|action.flamestrike.executing&(action.flamestrike.execute_remains<variable.cast_remains_time)|action.meteor.in_flight&(action.meteor.in_flight_remains<0.3|buff.bloodlust.up)&!talent.sunfury_execution" );
  sf_combustion->add_action( "meteor,if=buff.bloodlust.up&buff.combustion.down", "Precast one of these into Combustion." );
  sf_combustion->add_action( "flamestrike,if=talent.fuel_the_fire&active_enemies>=variable.sf_combustion_flamestrike&variable.flamestriking&(buff.combustion.down&!buff.hot_streak.react&buff.pyroclasm.up)" );
  sf_combustion->add_action( "pyroblast,if=buff.combustion.down&!buff.hot_streak.react&buff.pyroclasm.up" );
  sf_combustion->add_action( "scorch,if=buff.combustion.down&(target.health.pct<30|active_enemies>=4)" );
  sf_combustion->add_action( "fireball,if=buff.combustion.down&(!prev_gcd.1.meteor|buff.bloodlust.down)", "If precasting Meteor into Combustion, can fit a Fireball unless Bloodlust is active." );
  sf_combustion->add_action( "meteor,if=buff.combustion.remains>2", "Make sure Meteor lands during Combustion." );
  sf_combustion->add_action( "flamestrike,if=talent.fuel_the_fire&active_enemies>=variable.sf_combustion_flamestrike&variable.flamestriking&(buff.hot_streak.react|prev_gcd.1.scorch&buff.heating_up.react&time-action.scorch.last_used<0.2)", "Spend Hot Streaks on Pyroblast in ST or Flamestrike in AoE. The Scorch condition is simply to simulate predictable guaranteed crits during Combustion." );
  sf_combustion->add_action( "pyroblast,if=buff.hot_streak.react|prev_gcd.1.scorch&buff.heating_up.react&time-action.scorch.last_used<0.2" );
  sf_combustion->add_action( "flamestrike,if=talent.fuel_the_fire&active_enemies>=variable.sf_combustion_flamestrike&variable.flamestriking&(buff.pyroclasm.up&!buff.hot_streak.up&cast_time<buff.combustion.remains)", "Make sure Pyroclasm FINISHES its cast before Combustion ends." );
  sf_combustion->add_action( "pyroblast,if=buff.pyroclasm.up&!buff.hot_streak.up&cast_time<buff.combustion.remains" );
  sf_combustion->add_action( "scorch" );
  sf_combustion->add_action( "fireball" );
  sf_combustion->add_action( "call_action_list,name=fireblast,if=!talent.pyroclasm|(buff.pyroclasm.stack<2|action.pyroblast.executing&action.pyroblast.execute_remains>0.2&buff.pyroclasm.stack=2|cooldown.fire_blast.charges_fractional>=2|buff.combustion.remains<action.pyroblast.cast_time)&(active_enemies<variable.sf_combustion_flamestrike&variable.flamestriking|buff.pyroclasm.down|!action.flamestrike.executing)", "We prioritize starting to cast Pyroclasm (on 2 stacks) over Fire Blast if there's no risk of overcapping. In AoE, we also do not want to cast Fire Blast during a hardcast Flamestrike." );

  sf_filler->add_action( "meteor,if=active_enemies>=4&time>variable.combustion_delay&cooldown.combustion.remains<=gcd.max+variable.combustion_precast_time&buff.bloodlust.down", "In AoE, outside of Bloodlust, we can use Meteor into a Hardcast as a precast for Combustion." );
  sf_filler->add_action( "pyroblast,if=buff.hot_streak.up&talent.firestarter&time<variable.combustion_delay", "During Firestarter, only use Pyroblast as your spender, even for AoE" );
  sf_filler->add_action( "flamestrike,if=talent.fuel_the_fire&active_enemies>=variable.sf_filler_flamestrike&variable.flamestriking&(buff.hot_streak.react|prev_gcd.1.scorch&buff.heating_up.react&time-action.scorch.last_used<0.2|buff.hyperthermia.up)", "Spend Hot Streaks on Pyroblast in ST or Flamestrike in AoE. The Scorch condition is simply to simulate predictable guaranteed crits during execute." );
  sf_filler->add_action( "pyroblast,if=buff.hot_streak.react|prev_gcd.1.scorch&buff.heating_up.react&time-action.scorch.last_used<0.2|buff.hyperthermia.up" );
  sf_filler->add_action( "flamestrike,if=talent.fuel_the_fire&active_enemies>=variable.sf_filler_flamestrike&variable.flamestriking&buff.pyroclasm.up&((cooldown.combustion.remains>=12|time<variable.combustion_delay&(talent.firestarter|time>(variable.combustion_delay-action.flamestrike.cast_time)))|buff.pyroclasm.stack=2)", "Spend Pyroclasm immediately if you have 2 stacks available or if Firestarter is active. Otherwise, hold one stack if it lasts until Combustion comes up." );
  sf_filler->add_action( "pyroblast,if=buff.pyroclasm.up&(cooldown.combustion.remains>=12|time<variable.combustion_delay&(talent.firestarter|time>(variable.combustion_delay-action.pyroblast.cast_time)))|buff.pyroclasm.stack=2" );
  sf_filler->add_action( "meteor,if=(!talent.blast_zone&talent.sunfury_execution&cooldown.combustion.remains<12&buff.pyroclasm.stack<2)|(talent.blast_zone&time>variable.combustion_delay)", "Meteor is used on CD with Blast Zone starting from the first Combustion. Without Blast Zone, it's used either purely during Combustion or within 12 seconds before if talented into Sunfury Execution." );
  sf_filler->add_action( "scorch,if=talent.scald&target.health.pct<30|buff.heat_shimmer.react&(target.health.pct>=90|prev_gcd.1.pyroblast|prev_gcd.1.flamestrike)", "Cast Scorch in execute or with a Heat Shimmer proc." );
  sf_filler->add_action( "fireball" );
  sf_filler->add_action( "call_action_list,name=fireblast" );
}
//fire_apl_end

//frost_apl_start
void frost( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* ff_aoe = p->get_action_priority_list( "ff_aoe" );
  action_priority_list_t* ff_st = p->get_action_priority_list( "ff_st" );
  action_priority_list_t* movement = p->get_action_priority_list( "movement" );
  action_priority_list_t* ss_aoe = p->get_action_priority_list( "ss_aoe" );
  action_priority_list_t* ss_st = p->get_action_priority_list( "ss_st" );
  action_priority_list_t* ss_tarswap = p->get_action_priority_list( "ss_tarswap" );

  precombat->add_action( "arcane_intellect" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "variable,name=target_swapping,op=reset,default=0" );
  precombat->add_action( "summon_water_elemental" );
  precombat->add_action( "blizzard,if=active_enemies>=(4-talent.frostfire_bolt)|talent.freezing_rain|talent.freezing_winds", "Precast Blizzard against all targets if you have any of the Blizzard talents. In AoE (3+ for FF, 4+ for SS) Blizzard is always the precast." );
  precombat->add_action( "glacial_spike" );
  precombat->add_action( "frostbolt" );

  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "run_action_list,name=ff_aoe,if=talent.frostfire_bolt&active_enemies>=3", "Frostfire AoE starts at 3+ targets." );
  default_->add_action( "run_action_list,name=ff_st,if=talent.frostfire_bolt" );
  default_->add_action( "run_action_list,name=ss_tarswap,if=variable.target_swapping" );
  default_->add_action( "run_action_list,name=ss_aoe,if=active_enemies>=4", "Spellslinger AoE starts at 4+ targets" );
  default_->add_action( "run_action_list,name=ss_st" );

  cds->add_action( "variable,name=ff_trinket_timing,value=talent.frostfire_bolt", "Potion, Items and Racials are used on cd for Frostfire and paired with either Orb or Ray as Spellslinger." );
  cds->add_action( "variable,name=ss_trinket_timing,value=talent.splinterstorm&(time=0|fight_remains<15|prev_gcd.1.frozen_orb|cooldown.ray_of_frost.charges>=1&debuff.freezing.react<6&!buff.fingers_of_frost.react&(icicles<3|time-action.potion.last_used<25))" );
  cds->add_action( "use_item,name=nevermelting_ice_crystal,if=variable.ff_trinket_timing|variable.ss_trinket_timing", "Use Haste trinkets always after pot, Crit trinkets always before pot, and Mastery trinkets after pot if Crit is your highest stat and before pot otherwise." );
  cds->add_action( "use_item,name=freightrunners_flask,if=variable.ff_trinket_timing|variable.ss_trinket_timing" );
  cds->add_action( "use_item,name=vaelgors_final_stare,if=(variable.ff_trinket_timing|variable.ss_trinket_timing)&(stat.haste_rating>stat.crit_rating|stat.versatility_rating>stat.crit_rating)" );
  cds->add_action( "potion,if=variable.ff_trinket_timing|variable.ss_trinket_timing|fight_remains<35" );
  cds->add_action( "use_item,name=vaelgors_final_stare,if=variable.ff_trinket_timing|variable.ss_trinket_timing" );
  cds->add_action( "use_items" );
  cds->add_action( "blood_fury,if=variable.ff_trinket_timing|variable.ss_trinket_timing" );
  cds->add_action( "berserking,if=variable.ff_trinket_timing|variable.ss_trinket_timing" );
  cds->add_action( "fireblood,if=variable.ff_trinket_timing|variable.ss_trinket_timing" );
  cds->add_action( "ancestral_call,if=variable.ff_trinket_timing|variable.ss_trinket_timing" );
  cds->add_action( "flurry,if=active_enemies>=3&talent.wintertide&talent.frostfire_bolt,line_cd=9999", "Opener Frostfire" );
  cds->add_action( "ray_of_frost,if=talent.frostfire_bolt,line_cd=9999" );
  cds->add_action( "flurry,if=active_enemies>=4&talent.wintertide&talent.splinterstorm&!variable.target_swapping,line_cd=9999", "Opener Spellslinger" );
  cds->add_action( "flurry,target_if=min:debuff.freezing.react,if=active_enemies>=4&talent.wintertide&talent.splinterstorm&variable.target_swapping,line_cd=9999" );
  cds->add_action( "frozen_orb,if=active_enemies>=4&talent.splinterstorm,line_cd=9999" );
  cds->add_action( "ray_of_frost,if=talent.splinterstorm&!variable.target_swapping,line_cd=9999" );
  cds->add_action( "ray_of_frost,target_if=min:debuff.freezing.react,if=talent.splinterstorm&variable.target_swapping,line_cd=9999" );
  cds->add_action( "ray_of_frost,if=fight_remains<12&(!variable.target_swapping|talent.frostfire_bolt)", "End-Of-Fight Actions" );
  cds->add_action( "ray_of_frost,target_if=min:debuff.freezing.react,if=fight_remains<12&variable.target_swapping" );
  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.power_infusion.down", "Externals" );

  ff_aoe->add_action( "blizzard,if=buff.freezing_rain.up" );
  ff_aoe->add_action( "flurry,if=buff.brain_freeze.react&buff.thermal_void.down" );
  ff_aoe->add_action( "frozen_orb" );
  ff_aoe->add_action( "glacial_spike" );
  ff_aoe->add_action( "comet_storm" );
  ff_aoe->add_action( "blizzard,if=active_enemies>=(5-talent.freezing_rain-talent.freezing_winds)" );
  ff_aoe->add_action( "ice_lance,if=buff.fingers_of_frost.react" );
  ff_aoe->add_action( "ice_lance,if=debuff.freezing.stack>=10" );
  ff_aoe->add_action( "flurry,if=cooldown_react" );
  ff_aoe->add_action( "ray_of_frost,if=!buff.frostfire_empowerment.react" );
  ff_aoe->add_action( "frostbolt" );
  ff_aoe->add_action( "call_action_list,name=movement" );

  ff_st->add_action( "flurry,if=buff.brain_freeze.react&buff.thermal_void.down" );
  ff_st->add_action( "frozen_orb" );
  ff_st->add_action( "glacial_spike" );
  ff_st->add_action( "comet_storm" );
  ff_st->add_action( "ice_lance,if=buff.fingers_of_frost.react" );
  ff_st->add_action( "ice_lance,if=debuff.freezing.stack>=10" );
  ff_st->add_action( "flurry,if=cooldown_react" );
  ff_st->add_action( "ray_of_frost,if=active_enemies=1|!buff.frostfire_empowerment.react" );
  ff_st->add_action( "frostbolt" );
  ff_st->add_action( "call_action_list,name=movement" );

  movement->add_action( "any_blink,if=movement.distance>5" );
  movement->add_action( "blizzard,if=buff.freezing_rain.up" );
  movement->add_action( "ice_nova,if=talent.cone_of_frost" );
  movement->add_action( "cone_of_cold,if=talent.cone_of_frost" );
  movement->add_action( "ice_lance" );

  ss_aoe->add_action( "comet_storm" );
  ss_aoe->add_action( "blizzard,if=buff.freezing_rain.up" );
  ss_aoe->add_action( "flurry,if=buff.brain_freeze.react&buff.thermal_void.down" );
  ss_aoe->add_action( "ice_lance,if=buff.fingers_of_frost.react=2" );
  ss_aoe->add_action( "frozen_orb" );
  ss_aoe->add_action( "glacial_spike" );
  ss_aoe->add_action( "ice_lance,if=buff.fingers_of_frost.react" );
  ss_aoe->add_action( "ice_lance,if=debuff.freezing.react>=6" );
  ss_aoe->add_action( "ice_nova,if=talent.cone_of_frost" );
  ss_aoe->add_action( "cone_of_cold,if=talent.cone_of_frost" );
  ss_aoe->add_action( "blizzard,if=talent.freezing_winds" );
  ss_aoe->add_action( "ray_of_frost,if=icicles<3|time-action.potion.last_used<25" );
  ss_aoe->add_action( "flurry,if=cooldown_react" );
  ss_aoe->add_action( "frostbolt" );
  ss_aoe->add_action( "call_action_list,name=movement" );

  ss_st->add_action( "comet_storm" );
  ss_st->add_action( "flurry,if=buff.brain_freeze.react&buff.thermal_void.down" );
  ss_st->add_action( "ice_lance,if=buff.fingers_of_frost.react=2" );
  ss_st->add_action( "frozen_orb" );
  ss_st->add_action( "glacial_spike" );
  ss_st->add_action( "ice_lance,if=buff.fingers_of_frost.react" );
  ss_st->add_action( "ice_lance,if=debuff.freezing.react>=6" );
  ss_st->add_action( "ray_of_frost,if=icicles<3|time-action.potion.last_used<25" );
  ss_st->add_action( "flurry,if=cooldown_react" );
  ss_st->add_action( "frostbolt" );
  ss_st->add_action( "call_action_list,name=movement" );

  ss_tarswap->add_action( "comet_storm", "Played when the variable target_swapping=1. It's the ST/AoE rotation but always targets the enemy with the lowest Freezing stacks when casting a spell that generates Freezing." );
  ss_tarswap->add_action( "blizzard,target_if=active_enemies>=4&buff.freezing_rain.up" );
  ss_tarswap->add_action( "flurry,target_if=min:debuff.freezing.react,if=buff.brain_freeze.react&buff.thermal_void.down" );
  ss_tarswap->add_action( "ice_lance,if=buff.fingers_of_frost.react=2" );
  ss_tarswap->add_action( "frozen_orb" );
  ss_tarswap->add_action( "glacial_spike,target_if=min:debuff.freezing.react" );
  ss_tarswap->add_action( "ice_lance,if=buff.fingers_of_frost.react" );
  ss_tarswap->add_action( "ice_lance,target_if=min:debuff.freezing.react>=6,if=active_enemies<=2&debuff.freezing.react>=6", "Against 2 targets, wait for both to have 6+ freezing stacks before casting IL. Against 3+ targets cast IL as soon as any one target has 6+ stacks." );
  ss_tarswap->add_action( "ice_lance,target_if=debuff.freezing.react>=6,if=active_enemies>=3" );
  ss_tarswap->add_action( "ice_nova,if=active_enemies>=4&talent.cone_of_frost" );
  ss_tarswap->add_action( "cone_of_cold,if=active_enemies>=4&talent.cone_of_frost" );
  ss_tarswap->add_action( "blizzard,if=active_enemies>=4&talent.freezing_winds" );
  ss_tarswap->add_action( "ray_of_frost,target_if=min:debuff.freezing.react,if=icicles<3|time-action.potion.last_used<25" );
  ss_tarswap->add_action( "flurry,target_if=min:debuff.freezing.react,if=cooldown_react" );
  ss_tarswap->add_action( "frostbolt,target_if=min:debuff.freezing.react" );
  ss_tarswap->add_action( "call_action_list,name=movement" );
}
//frost_apl_end

}  // namespace mage_apl
