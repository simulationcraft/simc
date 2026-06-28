#include "class_modules/monk/sc_monk.hpp"
#include "player/action_priority_list.hpp"
#include "player/player.hpp"

#include <string>

using monk::monk_t;

// Per Specialization Defaults
namespace
{
namespace brewmaster
{
std::string default_potion( const monk_t* player )
{
  if ( player->true_level >= 90 )
    return "draught_of_rampant_abandon_2";
  return "disabled";
}

std::string default_flask( const monk_t* player )
{
  if ( player->true_level >= 90 )
    return "flask_of_the_magisters_2";
  return "disabled";
}

std::string default_food( const monk_t* player )
{
  if ( player->true_level >= 90 )
    return "harandar_celebration";
  return "disabled";
}

std::string default_rune( const monk_t* player )
{
  if ( player->true_level >= 90 )
    return "void_touched";
  return "disabled";
}

std::string default_temporary_enchant( const monk_t* player )
{
  if ( player->true_level >= 90 )
    return "main_hand:thalassian_phoenix_oil_2/off_hand:thalassian_phoenix_oil_2";
  return "disabled";
}

void default_apl( monk_t* player )
{
  action_priority_list_t* pre = player->get_action_priority_list( "precombat" );
  action_priority_list_t* def = player->get_action_priority_list( "default" );
  action_priority_list_t* ite = player->get_action_priority_list( "item_actions" );
  action_priority_list_t* rac = player->get_action_priority_list( "race_actions" );
  action_priority_list_t* moh = player->get_action_priority_list( "master_of_harmony" );
  action_priority_list_t* spm = player->get_action_priority_list( "shado_pan" );

  pre->add_action( "snapshot_stats", "Precombat" );
  pre->add_action( "potion" );

  def->add_action( "auto_attack", "Default List" );
  def->add_action( "potion" );
  def->add_action( "call_action_list,name=race_actions" );
  def->add_action( "call_action_list,name=item_actions" );
  def->add_action( "run_action_list,name=master_of_harmony,if=hero_tree.master_of_harmony" );
  def->add_action( "run_action_list,name=shado_pan,if=hero_tree.shadopan" );

  // Master of Harmony
  moh->add_action( "black_ox_brew,if=cooldown.celestial_brew.charges_fractional<1" );
  moh->add_action( "celestial_brew,if=buff.aspect_of_harmony_spender.up&!buff.empty_barrel.up" );
  moh->add_action( "keg_smash,if=buff.aspect_of_harmony_spender.up&buff.empty_barrel.up" );
  moh->add_action( "blackout_kick,if=talent.blackout_combo.enabled&!buff.blackout_combo.up" );
  moh->add_action( "celestial_brew,if=!(apex.3&buff.empty_barrel.up)&buff.aspect_of_harmony_accumulator.value>0.3*health.max&cooldown.celestial_brew.charges_fractional>1.9" );
  moh->add_action( "celestial_brew,if=!(apex.3&buff.empty_barrel.up)&target.time_to_die<15&buff.aspect_of_harmony_accumulator.value>0.2*health.max" );
  moh->add_action( "purifying_brew,if=!(apex.1&buff.empty_barrel.up)" );
  moh->add_action( "fortifying_brew,if=!(apex.3&buff.empty_barrel.up)" );
  moh->add_action( "chi_burst" );
  moh->add_action( "invoke_niuzao" );
  moh->add_action( "tiger_palm,if=buff.blackout_combo.up&cooldown.blackout_kick.remains<1.3" );
  moh->add_action( "exploding_keg,if=cooldown.keg_smash.charges_fractional<1" );
  moh->add_action( "empty_the_cellar,if=cooldown.celestial_brew.remains>15" );
  moh->add_action( "breath_of_fire,if=cooldown.blackout_kick.remains>1.5&!buff.empty_barrel.up&cooldown.keg_smash.charges<1+talent.stormstouts_last_keg.enabled" );
  moh->add_action( "tiger_palm,if=buff.blackout_combo.up" );
  moh->add_action( "keg_smash,if=talent.scalding_brew.enabled" );
  moh->add_action( "keg_smash,if=buff.empty_barrel.up" );
  moh->add_action( "keg_smash,if=cooldown.keg_smash.charges=1+talent.stormstouts_last_keg.enabled" );
  moh->add_action( "breath_of_fire" );
  moh->add_action( "empty_the_cellar" );
  moh->add_action( "rushing_jade_wind" );
  moh->add_action( "keg_smash" );
  moh->add_action( "blackout_kick" );
  moh->add_action( "tiger_palm,if=energy>50-energy.regen*2" );
  moh->add_action( "expel_harm" );

  // Shado-Pan
  spm->add_action( "black_ox_brew,if=!(apex.1&buff.empty_barrel.up)&cooldown.celestial_brew.charges_fractional<0.5" );
  spm->add_action( "breath_of_fire,if=talent.salsalabims_strength.enabled&buff.invoke_niuzao_the_black_ox.up" );
  spm->add_action( "keg_smash,if=talent.salsalabims_strength.enabled&buff.invoke_niuzao_the_black_ox.up" );
  spm->add_action( "blackout_kick,if=talent.blackout_combo.enabled&!buff.blackout_combo.up" );
  spm->add_action( "purifying_brew,if=!(apex.1&buff.empty_barrel.up)" );
  spm->add_action( "fortifying_brew,if=!(apex.3&buff.empty_barrel.up)" );
  spm->add_action( "chi_burst" );
  spm->add_action( "invoke_niuzao" );
  spm->add_action( "tiger_palm,if=buff.blackout_combo.up&cooldown.blackout_kick.remains<1.3" );
  spm->add_action( "exploding_keg,if=cooldown.keg_smash.charges_fractional<1" );
  spm->add_action( "empty_the_cellar,if=buff.empty_the_cellar.remains<1.5" );
  spm->add_action( "tiger_palm,if=buff.blackout_combo.up" );
  spm->add_action( "celestial_brew,if=!(apex.3&buff.empty_barrel.up)" );
  spm->add_action( "breath_of_fire,if=active_enemies>2" );
  spm->add_action( "keg_smash" );
  spm->add_action( "empty_the_cellar" );
  spm->add_action( "breath_of_fire" );
  spm->add_action( "rushing_jade_wind" );
  spm->add_action( "blackout_kick" );
  spm->add_action( "tiger_palm,if=energy>65-energy.regen" );
  spm->add_action( "expel_harm" );

  ite->add_action( "use_items", "Items" );

  rac->add_action( "blood_fury", "Racials" );
  rac->add_action( "berserking" );
  rac->add_action( "arcane_torrent" );
  rac->add_action( "lights_judgment" );
  rac->add_action( "fireblood" );
  rac->add_action( "ancestral_call" );
  rac->add_action( "bag_of_tricks" );
}
};  // namespace brewmaster

namespace windwalker
{
std::string default_potion( const monk_t* player )
{
  if ( player->true_level >= 90 )
    return "potion_of_recklessness_2";
  return "disabled";
}

std::string default_flask( const monk_t* player )
{
  if ( player->true_level >= 90 )
    return "flask_of_the_shattered_sun_2";
  return "disabled";
}

std::string default_food( const monk_t* player )
{
  if ( player->true_level >= 90 )
    return "harandar_celebration";
  return "disabled";
}

std::string default_rune( const monk_t* player )
{
  if ( player->true_level >= 90 )
    return "void_touched";
  return "disabled";
}

std::string default_temporary_enchant( const monk_t* player )
{
  if ( player->true_level >= 90 )
    return "main_hand:thalassian_phoenix_oil_2/off_hand:thalassian_phoenix_oil_2";
  return "disabled";
}

void live_apl( monk_t* player )
{
  action_priority_list_t* def      = player->get_action_priority_list( "default" );
  action_priority_list_t* pre      = player->get_action_priority_list( "precombat" );
  action_priority_list_t* opener   = player->get_action_priority_list( "opener" );
  action_priority_list_t* trinket  = player->get_action_priority_list( "trinket" );
  action_priority_list_t* coc      = player->get_action_priority_list( "big_coc" );
  action_priority_list_t* zen      = player->get_action_priority_list( "zenith" );
  action_priority_list_t* racials  = player->get_action_priority_list( "racials" );
  action_priority_list_t* st       = player->get_action_priority_list( "default_st" );
  action_priority_list_t* multi    = player->get_action_priority_list( "multitarget" );
  action_priority_list_t* fallback = player->get_action_priority_list( "fallback" );

  // Precombat
  pre->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins." );
  pre->add_action( "use_item,name=algethar_puzzle_box,if=!talent.flurry_strikes&(trinket.1.is.algethar_puzzle_box|trinket.2.is.algethar_puzzle_box)" );
  // Simplify fightstyle expressions for bloodmallet
  pre->add_action( "variable,name=patchwerk,value=fight_style.patchwerk|fight_style.castingpatchwerk" );

  // Default List
  def->add_action( "auto_attack,target_if=max:target.time_to_die", "Default List" );
  def->add_action( "touch_of_karma,target_if=max:target.time_to_die" );
  def->add_action( "roll,if=movement.distance>5", "Move to target" );
  def->add_action( "chi_torpedo,if=movement.distance>5" );
  def->add_action( "flying_serpent_kick,if=movement.distance>5" );
  def->add_action( "spear_hand_strike,if=target.debuff.casting.react" );
  def->add_action( "potion,if=buff.invoke_xuen_the_white_tiger.remains>15|fight_remains<=30" );
  def->add_action( "potion,if=talent.flurry_strikes&chi>2&(time<5|cooldown.zenith.up&time<5|time>300&((trinket.1.is.algethar_puzzle_box&trinket.1.cooldown.remains>100|trinket.2.is.algethar_puzzle_box&trinket.2.cooldown.remains>100)|!trinket.1.has_use_buff&!trinket.2.has_use_buff)&talent.flurry_strikes|time>300&buff.zenith.up)" );
  def->add_action( "variable,name=has_external_pi,value=cooldown.invoke_power_infusion_0.duration>0", "Enable PI if available" );
  def->add_action( "call_action_list,name=opener,if=time<2" );
  def->add_action( "call_action_list,name=trinket" );
  def->add_action( "invoke_external_buff,name=power_infusion,if=buff.zenith.up&(buff.invoke_xuen_the_white_tiger.up|talent.flurry_strikes)" );
  def->add_action( "call_action_list,name=big_coc,if=talent.celestial_conduit" );
  def->add_action( "call_action_list,name=zenith" );
  def->add_action( "call_action_list,name=racials" );
  def->add_action( "call_action_list,name=default_st,if=active_enemies=1" );
  def->add_action( "call_action_list,name=multitarget,if=active_enemies>1" );
  def->add_action( "call_action_list,name=fallback" );
  def->add_action( "arcane_torrent,if=chi<chi.max&energy<55" );
  def->add_action( "thorn_bloom" );
  def->add_action( "haymaker" );
  def->add_action( "bag_of_tricks" );
  def->add_action( "arcane_pulse" );
  def->add_action( "rocket_barrage" );
  def->add_action( "lights_judgment" );

  // Opener
  opener->add_action( "tiger_palm,if=combo_strike&chi<4", "Opener" );
  opener->add_action( "use_item,name=algethar_puzzle_box,if=target.time_to_die>25&(cooldown.invoke_xuen_the_white_tiger.remains<2|talent.flurry_strikes&cooldown.zenith.up)|fight_remains<25" );

  // Trinkets and Weapons
  trinket->add_action( "use_item,slot=main_hand", "Use Weapon" );
  trinket->add_action( "use_item,name=algethar_puzzle_box,if=fight_remains>5&(!buff.zenith.up&!talent.flurry_strikes&(target.time_to_die>35&fight_style.dungeonroute|target.time_to_die>25)&(cooldown.potion.remains>30|fight_remains<45|fight_remains>80)&(cooldown.invoke_xuen_the_white_tiger.remains<2|talent.flurry_strikes&cooldown.zenith.up)|fight_remains<25|talent.flurry_strikes&(target.time_to_die>35&fight_style.dungeonroute|target.time_to_die>25)&!buff.zenith.up|fight_style.dungeonslice&(time<5&chi>3|active_enemies>3&target.time_to_die>15))", "Use Algethar" );
  trinket->add_action( "use_item,slot=trinket1,if=trinket.1.has_use_buff&!trinket.2.has_use_buff&(pet.xuen_the_white_tiger.active&talent.invoke_xuen_the_white_tiger|talent.flurry_strikes&buff.zenith.remains>14)", "Stat on use with passive or DMG on use" );
  trinket->add_action( "use_item,slot=trinket2,if=trinket.2.has_use_buff&!trinket.1.has_use_buff&(pet.xuen_the_white_tiger.active&talent.invoke_xuen_the_white_tiger|talent.flurry_strikes&buff.zenith.remains>14)" );
  trinket->add_action( "use_item,slot=trinket1,if=trinket.1.has_use_buff&trinket.2.has_use_buff&(pet.xuen_the_white_tiger.active&talent.invoke_xuen_the_white_tiger|talent.flurry_strikes&buff.zenith.remains>14)", "Stat on use with Stat on use" );
  trinket->add_action( "use_item,slot=trinket2,if=trinket.1.has_use_buff&trinket.2.has_use_buff&(cooldown.invoke_xuen_the_white_tiger.remains>30&(buff.zenith.up|(cooldown.strike_of_the_windlord.remains<2&talent.strike_of_the_windlord|cooldown.whirling_dragon_punch.remains<2&talent.whirling_dragon_punch))|talent.flurry_strikes&buff.zenith.remains>10)" );
  trinket->add_action( "use_item,slot=trinket1,if=!trinket.1.has_use_buff&trinket.2.has_use_buff&trinket.2.cooldown.remains>30", "DMG on use with stat on use" );
  trinket->add_action( "use_item,slot=trinket2,if=!trinket.2.has_use_buff&trinket.1.has_use_buff&trinket.1.cooldown.remains>30" );
  trinket->add_action( "use_item,slot=trinket1,if=!trinket.1.has_use_buff&!trinket.2.has_use_buff", "DMG on use without stat on use" );
  trinket->add_action( "use_item,slot=trinket2,if=!trinket.1.has_use_buff&!trinket.2.has_use_buff" );

  // Celestial of the Conduit
  coc->add_action( "invoke_xuen_the_white_tiger,target_if=max:target.time_to_die,if=(target.time_to_die>35&fight_style.dungeonroute|target.time_to_die>25&!fight_style.dungeonroute)&((cooldown.zenith.up|buff.zenith.remains>13)&!buff.heart_of_the_jade_serpent.up)&(!fight_style.dungeonslice|active_enemies>1|time<60)","Celestial of the Conduit Burst Windows" );
  coc->add_action( "invoke_xuen_the_white_tiger,target_if=max:target.time_to_die,if=(target.time_to_die>35&fight_style.dungeonroute|target.time_to_die>25&!fight_style.dungeonroute)&(trinket.1.is.algethar_puzzle_box&trinket.1.cooldown.remains>100|trinket.2.is.algethar_puzzle_box&trinket.2.cooldown.remains>100)&(!fight_style.dungeonslice|active_enemies>1|time<60)" );
  coc->add_action( "invoke_xuen_the_white_tiger,target_if=max:target.time_to_die,if=fight_style.dungeonslice&target.time_to_die>15&active_enemies>4|fight_remains<=25" );
  coc->add_action( "celestial_conduit,target_if=max:target.time_to_die,if=buff.zenith.remains<12&buff.zenith.up&(!buff.bloodlust.up|buff.power_infusion.up)|fight_remains<4" );
  coc->add_action( "whirling_dragon_punch,if=buff.power_infusion.up&(!buff.heart_of_the_jade_serpent_unity_within.up|buff.heart_of_the_jade_serpent_unity_within.remains<2)" );
  coc->add_action( "blackout_kick,target_if=max:target.time_to_die,if=combo_strike&talent.celestial_conduit&buff.zenith.remains>11&chi<=2&cooldown.rising_sun_kick.remains&!buff.rushing_wind_kick.up&talent.obsidian_spiral&buff.combo_breaker.up" );
  coc->add_action( "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&talent.celestial_conduit&buff.zenith.remains>11&chi<=2&cooldown.rising_sun_kick.remains&!buff.rushing_wind_kick.up&(!talent.obsidian_spiral|!buff.combo_breaker.up|prev.blackout_kick)" );
  coc->add_action( "celestial_conduit,target_if=max:target.time_to_die,if=buff.zenith.up&(cooldown.rising_sun_kick.remains|active_enemies>2)&cooldown.fists_of_fury.remains&(cooldown.strike_of_the_windlord.remains|talent.whirling_dragon_punch)&(cooldown.whirling_dragon_punch.remains|talent.strike_of_the_windlord)&!buff.rushing_wind_kick.up&!buff.combo_breaker.up&chi>1&(!buff.heart_of_the_jade_serpent.up|buff.heart_of_the_jade_serpent.remains<4)" );
  coc->add_action( "celestial_conduit,target_if=max:target.time_to_die,if=buff.zenith.up&!buff.heart_of_the_jade_serpent.up&!buff.heart_of_the_jade_serpent_yulons_avatar.up&chi>1&(cooldown.rising_sun_kick.remains|active_enemies>2)&(cooldown.strike_of_the_windlord.remains|(cooldown.whirling_dragon_punch.remains|cooldown.fists_of_fury.remains))" );
  coc->add_action( "celestial_conduit,target_if=max:target.time_to_die,if=buff.zenith.up&buff.heart_of_the_jade_serpent.remains<2&prev.rising_sun_kick&cooldown.rising_sun_kick.remains&cooldown.fists_of_fury.remains&buff.heart_of_the_jade_serpent.up&chi>1" );

  // Zenith usage
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=buff.invoke_xuen_the_white_tiger.up&(!buff.zenith.up|talent.flurry_strikes)", "Zenith Usage" );
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=buff.bloodlust.remains>30&(active_enemies>2|cooldown.rising_sun_kick.remains)&!buff.zenith.up" );
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=(target.time_to_die>30&fight_style.dungeonroute|target.time_to_die>25&!fight_style.dungeonroute)&(buff.bloodlust.up&cooldown.celestial_conduit.remains&(cooldown.rising_sun_kick.remains|active_enemies>2)&!buff.zenith.up&talent.celestial_conduit)" );
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=(target.time_to_die>30&fight_style.dungeonroute|target.time_to_die>25&!fight_style.dungeonroute)&(talent.flurry_strikes&(buff.bloodlust.up|cooldown.potion.remains>295))&!buff.zenith.up&(buff.bloodlust.remains>30|talent.spiritual_focus)" );
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=time>250&cooldown.potion.remains>295&(!trinket.1.has_use_buff&!trinket.2.has_use_buff|trinket.1.has_use_buff&trinket.1.cooldown.remains>30|trinket.2.has_use_buff&trinket.2.cooldown.remains>30)&(fight_remains>120|fight_remains<50&fight_remains>cooldown.zenith.full_recharge_time)" );
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=(target.time_to_die>30&fight_style.dungeonroute|target.time_to_die>25&!fight_style.dungeonroute)&talent.flurry_strikes&!trinket.1.has_use_buff&!trinket.2.has_use_buff&cooldown.rising_sun_kick.remains&cooldown.fists_of_fury.remains<5&(cooldown.whirling_dragon_punch.remains<10|cooldown.strike_of_the_windlord.remains<10)&cooldown.zenith.full_recharge_time<40&!fight_style.dungeonslice&!buff.zenith.up" );
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=(target.time_to_die>30&fight_style.dungeonroute|target.time_to_die>25&!fight_style.dungeonroute)&(!buff.bloodlust.up&(trinket.1.is.algethar_puzzle_box&trinket.1.cooldown.remains>100|trinket.2.is.algethar_puzzle_box&trinket.2.cooldown.remains>100)&(cooldown.rising_sun_kick.remains|active_enemies>2|talent.drinking_horn_cover&chi<2))&!buff.zenith.up" );
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=(variable.patchwerk|fight_style.dungeonroute&target.time_to_die>27+5*talent.drinking_horn_cover)&talent.flurry_strikes&(buff.tigereye_brew_1.stack>19-2*talent.echo_technique|buff.tigereye_brew_1.stack>11&talent.spiritual_focus-2*talent.echo_technique)&(target.time_to_die>30&fight_style.dungeonroute|target.time_to_die>25&!fight_style.dungeonroute)&(cooldown.rising_sun_kick.remains|active_enemies>1)&(!trinket.1.has_use_buff&!trinket.2.has_use_buff|trinket.1.has_use_buff&(trinket.1.cooldown.remains>40|trinket.1.cooldown.remains>30&talent.spiritual_focus)|trinket.2.has_use_buff&(trinket.2.cooldown.remains>40|trinket.2.cooldown.remains>30&talent.spiritual_focus))&(talent.strike_of_the_windlord&cooldown.strike_of_the_windlord.remains<15-5*talent.revolving_whirl&talent.drinking_horn_cover|talent.whirling_dragon_punch&cooldown.whirling_dragon_punch.remains<15-7*talent.revolving_whirl&talent.drinking_horn_cover|talent.strike_of_the_windlord&cooldown.strike_of_the_windlord.remains<10|talent.whirling_dragon_punch&cooldown.whirling_dragon_punch.remains<10)&cooldown.fists_of_fury.remains<9+4*talent.spiritual_focus" );
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=(cooldown.rising_sun_kick.remains|active_enemies>2)&fight_style.dungeonslice&time>130&time<150&active_enemies>1&talent.flurry_strikes&!buff.zenith.up" );
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=fight_style.dungeonslice&target.time_to_die>15&active_enemies>4&(talent.flurry_strikes|talent.celestial_conduit&talent.restore_balance&cooldown.invoke_xuen_the_white_tiger.remains<cooldown.zenith.full_recharge_time)&!variable.patchwerk&!buff.zenith.up" );
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=!buff.zenith.up&(talent.celestial_conduit&fight_remains<cooldown.invoke_xuen_the_white_tiger.remains&(cooldown.rising_sun_kick.remains|active_enemies>2)&(target.time_to_die>30&fight_style.dungeonroute|target.time_to_die>25&!fight_style.dungeonroute|target.time_to_die>15&active_enemies>4)&!variable.patchwerk)" );
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=!buff.zenith.up&talent.flurry_strikes&fight_style.dungeonroute&cooldown.zenith.full_recharge_time<30&target.time_to_die>25" );
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=variable.patchwerk&!buff.zenith.up&cooldown.fists_of_fury.remains<10&(cooldown.whirling_dragon.remains<10|cooldown.strike_of_the_windlord.remains<10)&(cooldown.rising_sun_kick.remains|chi<2&energy<50|active_enemies>1)&cooldown.zenith.full_recharge_time<30&(!trinket.1.has_use_buff&!trinket.2.has_use_buff|trinket.1.has_use_buff&trinket.1.cooldown.remains>30|trinket.2.has_use_buff&trinket.2.cooldown.remains>30)&(fight_remains>120|fight_remains<50&fight_remains>cooldown.zenith.full_recharge_time)" );
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=fight_remains<=24&(cooldown.rising_sun_kick.remains|active_enemies>2)" );
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=fight_remains<45&cooldown.zenith.full_recharge_time<5&(cooldown.rising_sun_kick.remains|active_enemies>1)" );
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=!buff.zenith.up&(variable.patchwerk&!trinket.1.is.algethar_puzzle_box&!trinket.2.is.algethar_puzzle_box&trinket.1.has_use_buff&(trinket.1.cooldown.ready|cooldown.zenith.full_recharge_time<5))" );
  zen->add_action( "zenith,target_if=max:target.time_to_die,if=!buff.zenith.up&(variable.patchwerk&!trinket.1.is.algethar_puzzle_box&!trinket.2.is.algethar_puzzle_box&trinket.2.has_use_buff&(trinket.2.cooldown.ready|cooldown.zenith.full_recharge_time<5))" );

  // Racials (Good)
  racials->add_action( "berserking,if=buff.invoke_xuen_the_white_tiger.remains>15|!talent.invoke_xuen_the_white_tiger&buff.zenith.remains>14|fight_remains<20", "Racials (Good)" );
  racials->add_action( "ancestral_call,if=buff.invoke_xuen_the_white_tiger.remains>15|!talent.invoke_xuen_the_white_tiger&buff.zenith.remains>14|fight_remains<20" );
  racials->add_action( "blood_fury,if=buff.invoke_xuen_the_white_tiger.remains>15|!talent.invoke_xuen_the_white_tiger&buff.zenith.remains>14|fight_remains<20" );
  racials->add_action( "fireblood,if=buff.invoke_xuen_the_white_tiger.remains>15|!talent.invoke_xuen_the_white_tiger&buff.zenith.remains>14|fight_remains<20" );

  // Single Target
  st->add_action( "whirling_dragon_punch,if=!buff.heart_of_the_jade_serpent_unity_within.up&buff.whirling_dragon_punch.remains<1&(buff.zenith.up|cooldown.invoke_xuen_the_white_tiger.remains>5|talent.flurry_strikes|!variable.patchwerk)","Single Target" );
  st->add_action( "zenith_stomp,if=buff.zenith.up&(buff.zenith.remains<5&buff.zenith_stomp.stack=2|buff.zenith.remains<4)|talent.celestial_conduit&chi<5&!buff.heart_of_the_jade_serpent_unity_within.up" );
  st->add_action( "whirling_dragon_punch,if=buff.power_infusion.up&(!buff.heart_of_the_jade_serpent_unity_within.up|buff.heart_of_the_jade_serpent_unity_within.remains<2)" );
  st->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.remains<1&buff.combo_breaker.stack<2&talent.sequenced_strikes&buff.dance_of_chiji.up&talent.celestial_conduit" );
  st->add_action( "fists_of_fury,if=buff.heart_of_the_jade_serpent.remains<1&buff.heart_of_the_jade_serpent.up|buff.flurry_charge.stack=30&!buff.zenith.up" );
  st->add_action( "whirling_dragon_punch,if=talent.celestial_conduit&buff.heart_of_the_jade_serpent_unity_within.remains<2&(buff.zenith.up|cooldown.invoke_xuen_the_white_tiger.remains>5|!variable.patchwerk)|talent.flurry_strikes" );
  st->add_action( "tiger_palm,if=chi<3-1*!talent.ascension+1*talent.celestial_conduit+1*(buff.tigereye_brew_1.stack<15&time>60&time<120)&combo_strike&energy.time_to_max<=gcd.max*3&!buff.zenith.up&(!buff.bloodlust.up|chi<2)&buff.combo_breaker.stack<2" );
  st->add_action( "strike_of_the_windlord,if=talent.celestial_conduit&buff.heart_of_the_jade_serpent_unity_within.remains<2&(buff.zenith.up|cooldown.invoke_xuen_the_white_tiger.remains>5|!variable.patchwerk)|talent.flurry_strikes" );
  st->add_action( "rising_sun_kick,if=!buff.bloodlust.up&!buff.zenith.up&(chi>4|energy>50|cooldown.fists_of_fury.remains>5)|buff.zenith.up&buff.zenith.remains<2&combo_strike" );
  st->add_action( "fists_of_fury,if=combo_strike&(buff.heart_of_the_jade_serpent.up|buff.heart_of_the_jade_serpent_yulons_avatar.up|buff.heart_of_the_jade_serpent_unity_within.up)&buff.bloodlust.up|buff.bloodlust.up&talent.flurry_strikes|!buff.zenith.up&(talent.flurry_strikes|cooldown.invoke_xuen_the_white_tiger.remains>3|!variable.patchwerk)|buff.zenith.up&(talent.flurry_strikes|!buff.bloodlust.up)&(variable.patchwerk|target.time_to_die>5)" );
  st->add_action( "rushing_wind_kick" );
  st->add_action( "rising_sun_kick,if=combo_strike&buff.bloodlust.up|combo_strike&(buff.heart_of_the_jade_serpent.up|buff.heart_of_the_jade_serpent_yulons_avatar.up|buff.heart_of_the_jade_serpent_unity_within.up)" );
  st->add_action( "fists_of_fury,if=buff.bloodlust.up|combo_strike&(buff.heart_of_the_jade_serpent.up|buff.heart_of_the_jade_serpent_yulons_avatar.up|buff.heart_of_the_jade_serpent_unity_within.up)" );
  st->add_action( "tiger_palm,if=buff.zenith.up&chi<2&talent.celestial_conduit&(buff.heart_of_the_jade_serpent.up|buff.heart_of_the_jade_serpent_unity_within.up)&!cooldown.fists_of_fury.remains&combo_strike" );
  st->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.remains<5&buff.combo_breaker.stack<2&talent.sequenced_strikes&buff.dance_of_chiji.up|combo_strike&buff.dance_of_chiji.stack=2&buff.combo_breaker.stack<2&talent.sequenced_strikes&(talent.flurry_strikes|!buff.bloodlust.up)" );
  st->add_action( "rising_sun_kick,if=buff.zenith.up&talent.flurry_strikes&!cooldown.fists_of_fury.remains" );
  st->add_action( "rising_sun_kick,if=combo_strike" );
  st->add_action( "fists_of_fury,if=talent.flurry_strikes|!buff.zenith.up&(talent.flurry_strikes|cooldown.invoke_xuen_the_white_tiger.remains>3|!variable.patchwerk)|buff.bloodlust.up&talent.jadefire_stomp&cooldown.celestial_conduit.remains" );
  st->add_action( "rising_sun_kick,if=buff.heart_of_the_jade_serpent.up|buff.heart_of_the_jade_serpent_unity_within.up|buff.heart_of_the_jade_serpent_yulons_avatar.up" );
  st->add_action( "touch_of_death,if=!buff.zenith.up|fight_remains<5|((trinket.1.is.algethar_puzzle_box&trinket.1.cooldown.remains>100|trinket.2.is.algethar_puzzle_box&trinket.2.cooldown.remains>100)|!trinket.1.has_use_buff&!trinket.2.has_use_buff)" );
  st->add_action( "strike_of_the_windlord,if=buff.heart_of_the_jade_serpent_unity_within.remains<2&(buff.zenith.up|cooldown.invoke_xuen_the_white_tiger.remains>5|!variable.patchwerk)|talent.flurry_strikes" );
  st->add_action( "rising_sun_kick,if=combo_strike&(buff.flurry_charge.stack<30|chi>3|buff.zenith.up|buff.bloodlust.up|energy>50&chi>2)|combo_strike&buff.heart_of_the_jade_serpent.up" );
  st->add_action( "tiger_palm,if=combo_strike&buff.zenith.up&(chi<1|chi<2&!buff.combo_breaker.up)&talent.celestial_conduit" );
  st->add_action( "zenith_stomp,if=buff.zenith.up&chi<5-1*!talent.ascension&(talent.flurry_strikes|chi<3|buff.zenith.remains<5)&buff.combo_breaker.stack<2&buff.dance_of_chiji.stack<2&(!buff.combo_breaker.up|talent.echo_technique)" );
  st->add_action( "blackout_kick,if=combo_strike&buff.zenith.up&chi>1&(talent.obsidian_spiral|cooldown.fists_of_fury.remains|buff.combo_breaker.up)&(chi<6|buff.combo_breaker.up|cooldown.rising_sun_kick.remains<3)" );
  st->add_action( "blackout_kick,if=combo_strike&buff.combo_breaker.up" );
  st->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up&talent.sequenced_strikes" );
  st->add_action( "spinning_crane_kick,if=combo_strike&buff.zenith.up&talent.flurry_strikes&chi>3+1*!talent.ascension" );
  st->add_action( "slicing_winds" );
  st->add_action( "spinning_crane_kick,if=talent.flurry_strikes&buff.zenith.up&chi>5-1*!talent.ascension&combo_strike|combo_strike&buff.bloodlust.up&buff.dance_of_chiji.up&buff.combo_breaker.stack<2" );
  st->add_action( "tiger_palm,if=combo_strike&((energy>55&talent.inner_peace|energy>60&!talent.inner_peace)&chi.max-chi>=3-1*talent.celestial_conduit&(talent.energy_burst&!buff.combo_breaker.up|!talent.energy_burst)&!buff.zenith.up|(talent.energy_burst&!buff.combo_breaker.up|!talent.energy_burst)&!buff.zenith.up&!cooldown.fists_of_fury.remains&chi<3)" );

  //Multitarget
  multi->add_action( "fists_of_fury,target_if=max:target.time_to_die,if=buff.heart_of_the_jade_serpent.remains<1&buff.heart_of_the_jade_serpent.up", "Multi Target" );
  multi->add_action( "zenith_stomp,target_if=max:target.time_to_die,if=buff.zenith.up&(buff.zenith.remains<5&buff.zenith_stomp.stack=2|buff.zenith.remains<4)|talent.celestial_conduit&chi<5&!buff.heart_of_the_jade_serpent_unity_within.up" );
  multi->add_action( "whirling_dragon_punch,if=talent.celestial_conduit&buff.heart_of_the_jade_serpent_unity_within.remains<2" );
  multi->add_action( "whirling_dragon_punch,target_if=max:target.time_to_die,if=!buff.heart_of_the_jade_serpent_unity_within.up&buff.whirling_dragon_punch.remains<1" );
  multi->add_action( "tiger_palm,target_if=max:target.time_to_die,if=buff.zenith.up&chi<2&talent.celestial_conduit&(buff.heart_of_the_jade_serpent.up|buff.heart_of_the_jade_serpent_unity_within.up)&!cooldown.fists_of_fury.remains&combo_strike" );
  multi->add_action( "tiger_palm,target_if=max:target.time_to_die,if=chi<5&combo_strike&energy.time_to_max<=gcd.max*3&!buff.zenith.up&!buff.bloodlust.up&buff.combo_breaker.stack<2|combo_strike&chi<3-1*buff.zenith.up&!cooldown.fists_of_fury.remains&(!buff.combo_breaker.up|!talent.energy_burst)&!buff.zenith_stomp.up" );
  multi->add_action( "strike_of_the_windlord,if=talent.celestial_conduit&buff.heart_of_the_jade_serpent_unity_within.remains<2" );
  multi->add_action( "fists_of_fury,target_if=max:target.time_to_die,if=buff.flurry_charge.stack=30&!buff.zenith.up|buff.heart_of_the_jade_serpent.up|buff.heart_of_the_jade_serpent_unity_within.up|buff.heart_of_the_jade_serpent_yulons_avatar.up|talent.flurry_strikes" );
  multi->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up&buff.combo_breaker.stack<2&talent.sequenced_strikes&buff.dance_of_chiji.remains<3" );
  multi->add_action( "rushing_wind_kick,target_if=max:target.time_to_die" );
  multi->add_action( "rising_sun_kick,target_if=max:target.time_to_die,if=(active_enemies<5|cooldown.fists_of_fury.remains>1|buff.zenith.up)&(buff.rushing_wind_kick.up|buff.heart_of_the_jade_serpent.up|buff.heart_of_the_jade_serpent_unity_within.up|buff.heart_of_the_jade_serpent_yulons_avatar.up)" );
  multi->add_action( "zenith_stomp,target_if=max:target.time_to_die,if=buff.zenith.up&chi<5-1*!talent.ascension&(talent.flurry_strikes|chi<3|buff.zenith.remains<5)&buff.combo_breaker.stack<2&buff.dance_of_chiji.stack<2&(!buff.combo_breaker.up|talent.echo_technique)" );
  multi->add_action( "touch_of_death,target_if=min:target.time_to_die,if=!buff.zenith.up|fight_remains<5|((trinket.1.is.algethar_puzzle_box&trinket.1.cooldown.remains>100|trinket.2.is.algethar_puzzle_box&trinket.2.cooldown.remains>100)|!trinket.1.has_use_buff&!trinket.2.has_use_buff)" );
  multi->add_action( "strike_of_the_windlord,if=buff.zenith.up|cooldown.zenith.remains>5&buff.heart_of_the_jade_serpent_unity_within.remains<2" );
  multi->add_action( "whirling_dragon_punch,if=buff.zenith.up|cooldown.zenith.remains>5&buff.heart_of_the_jade_serpent_unity_within.remains<2" );
  multi->add_action( "fists_of_fury,target_if=max:target.time_to_die,if=talent.flurry_strikes|!buff.zenith.up|buff.bloodlust.up&talent.jadefire_stomp&cooldown.celestial_conduit.remains" );
  multi->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.stack=2&buff.combo_breaker.stack<2&talent.sequenced_strikes" );
  multi->add_action( "rising_sun_kick,target_if=max:target.time_to_die,if=(active_enemies<5|cooldown.fists_of_fury.remains>1|buff.zenith.up)&(combo_strike&(buff.flurry_charge.stack<30|chi>3|buff.zenith.up|buff.bloodlust.up|energy>50&chi>2)|combo_strike&buff.heart_of_the_jade_serpent.up)" );
  multi->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=talent.flurry_strikes&buff.zenith.up&chi>3&combo_strike&(!talent.shadowboxing_treads|active_enemies>3)" );
  multi->add_action( "blackout_kick,target_if=max:target.time_to_die,if=combo_strike&buff.zenith.up&chi>1&(talent.obsidian_spiral|buff.combo_breaker.up|cooldown.rising_sun_kick.remains<3&cooldown.rising_sun_kick.remains|talent.shadowboxing_treads&cooldown.rising_sun_kick.remains)&chi<6" );
  multi->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up&buff.combo_breaker.stack<2&talent.sequenced_strikes&buff.dance_of_chiji.remains<4" );
  multi->add_action( "slicing_winds" );
  multi->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=talent.flurry_strikes&buff.zenith.up&chi>3&combo_strike" );
  multi->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&(buff.dance_of_chiji.up|(chi>2|energy>55))&cooldown.rising_sun_kick.remains&cooldown.fists_of_fury.remains&!talent.shadowboxing_treads&!buff.zenith.up" );
  multi->add_action( "blackout_kick,target_if=max:target.time_to_die,if=combo_strike&buff.combo_breaker.up&(buff.heart_of_the_jade_serpent.up|buff.heart_of_the_jade_serpent_unity_within.up)" );
  multi->add_action( "blackout_kick,target_if=max:target.time_to_die,if=combo_strike&buff.combo_breaker.up" );
  multi->add_action( "tiger_palm,target_if=max:target.time_to_die,if=chi<5&combo_strike&energy.time_to_max<=gcd.max*3&!buff.zenith.up&!buff.bloodlust.up" );
  multi->add_action( "blackout_kick,target_if=max:target.time_to_die,if=combo_strike&buff.combo_breaker.stack=2" );
  multi->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.dance_of_chiji.stack=2" );
  multi->add_action( "spinning_crane_kick,if=combo_strike&!buff.zenith.up&chi>5&buff.combo_breaker.up&cooldown.rising_sun_kick.remains&cooldown.fists_of_fury.remains" );
  multi->add_action( "blackout_kick,target_if=max:target.time_to_die,if=combo_strike&buff.combo_breaker.up" );
  multi->add_action( "tiger_palm,target_if=max:target.time_to_die,if=chi<5&combo_strike&energy.time_to_max<=gcd.max*3&!buff.zenith.up&active_enemies<3" );
  multi->add_action( "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&((energy>55&talent.inner_peace|energy>60&!talent.inner_peace)&chi.max-chi>=2&(talent.energy_burst&!buff.combo_breaker.up|!talent.energy_burst)&!buff.zenith.up|(talent.energy_burst&!buff.combo_breaker.up|!talent.energy_burst)&!buff.zenith.up&!cooldown.fists_of_fury.remains&chi<3)" );
  multi->add_action( "blackout_kick,target_if=max:target.time_to_die,if=combo_strike&talent.shadowboxing_treads" );
  multi->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&(chi>3|energy>55)&(!talent.shadowboxing_treads&active_enemies>2|active_enemies>5)&cooldown.rising_sun_kick.remains&cooldown.fists_of_fury.remains" );
  multi->add_action( "rising_sun_kick,target_if=max:target.time_to_die,if=combo_strike" );
  multi->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&chi>2" );

  // Fallback
  fallback->add_action( "blackout_kick,if=combo_strike", "Fallback" );
  fallback->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up" );
  fallback->add_action( "spinning_crane_kick,if=chi>5&combo_strike&talent.flurry_strikes" );
  fallback->add_action( "tiger_palm,if=combo_strike" );
  fallback->add_action( "spinning_crane_kick,if=chi>5&combo_strike" );
}

void ptr_apl( monk_t* player )
{
  live_apl( player );
}

void default_apl( monk_t* player )
{
  if ( !player->is_ptr() )
    live_apl( player );
  else
    ptr_apl( player );
}
};  // namespace windwalker
};  // namespace

// Shared Defaults
namespace monk
{
void monk_t::init_blizzard_action_list()
{
  action_priority_list_t* default_ = get_action_priority_list( "default" );

  switch ( specialization() )
  {
    case MONK_BREWMASTER:
    case MONK_WINDWALKER:
      default_->add_action( "auto_attack", "Overridden" );
      break;
    default:
      assert( false );
      break;
  }

  base_t::init_blizzard_action_list();

  action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );

  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      cooldowns->add_action( "invoke_niuzao_the_black_ox" );
      break;
    case MONK_WINDWALKER:
      cooldowns->add_action( "invoke_xuen_the_white_tiger" );
      cooldowns->add_action( "touch_of_karma" );
      break;
    default:
      assert( false );
      break;
  }
}

void monk_t::parse_assisted_combat_step( const assisted_combat_step_data_t& step,
                                         action_priority_list_t* assisted_combat )
{
  if ( step.spell_id == 388193 )
    return;

  base_t::parse_assisted_combat_step( step, assisted_combat );
}

parsed_assisted_combat_rule_t monk_t::parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule,
                                                                  const assisted_combat_step_data_t& step ) const
{
  // Assisted Combat APL is partially updated and still includes references to Emperor's Capacitor
  if ( step.spell_id == 117952 && rule.condition_type == AC_PLAYER_AURA_APPLICATION_GREATER &&
       rule.condition_value_1 == 393039 && rule.condition_value_2 == 20 && rule.condition_value_3 == 0 )
    return {
        "0",
        "Condition `buff.emperors_capacitor.stack>=20` is not resolvable, as the source talent no longer exists." };

  if ( step.spell_id == 152175 && rule.condition_type == AC_TARGET_DISTANCE_LESS )
  {
    assisted_combat_rule_data_t rule_copy = rule;
    rule_copy.condition_value_1           = 10;

    return { base_t::parse_assisted_combat_rule( rule_copy, step ), "Extended range check to 10 yards (from 5)." };
  }

  if ( step.spell_id == 100784 && rule.condition_type == AC_AURA_ON_PLAYER && rule.condition_value_1 == 116768 &&
       rule.condition_value_2 == 0 && rule.condition_value_3 == 0 )
    return {
        "buff.combo_breaker.up",
        "The name `Combo Breaker` is used instead of `Blackout Kick` due to standard tokenization rules and clarity.",
        true };

  if ( step.spell_id == 100780 && rule.condition_type == AC_AURA_MISSING_PLAYER && rule.condition_value_1 == 261916 &&
       rule.condition_value_2 == 0 && rule.condition_value_3 == 0 )
    return { "level<17", "Checks for Blackout Kick Rank 2 not being known, which is learned at level 17.", true };

  if ( rule.condition_type == AC_AURA_MISSING_PLAYER && rule.condition_value_2 == 0 && rule.condition_value_3 == 0 )
  {
    switch ( rule.condition_value_1 )
    {
      case 1249753:
      case 1249754:
      case 1249756:
      case 1249757:
      case 1249758:
      case 1249764:
      case 1249765:
      case 1249766:
        return { "combo_strike",
                 fmt::format( "Rule contains a helper buff to avoid breaking Combo Strikes for {}.",
                              rule.condition_value_1, find_spell( step.spell_id )->name_cstr() ),
                 false };
    }
  }

  if ( rule.condition_type == AC_TARGET_COUNT_NEAR_PLAYER_GREATER && rule.condition_value_1 == 1 &&
       rule.condition_value_2 == 15 && rule.condition_value_3 == 0 )
    return { "1", "Counts valid targets for action, including player." };

  return base_t::parse_assisted_combat_rule( rule, step );
}

std::vector<std::string> monk_t::action_names_from_spell_id( unsigned int spell_id ) const
{
  if ( spell_id == 107428 && specialization() == MONK_WINDWALKER )
    return { "rising_sun_kick", "rushing_wind_kick" };

  return base_t::action_names_from_spell_id( spell_id );
}

std::string monk_t::aura_expr_from_spell_id( unsigned int spell_id, bool on_self ) const
{
  if ( on_self )
  {
    switch ( spell_id )
    {
      case 443616:
        return "buff.heart_of_the_jade_serpent_unity_within";
      case 1238904:
        return "buff.heart_of_the_jade_serpent_yulons_avatar";
    }
  }

  return base_t::aura_expr_from_spell_id( spell_id, on_self );
}
};  // namespace monk

namespace monk
{
std::string monk_t::default_potion() const
{
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      return brewmaster::default_potion( this );
    case MONK_WINDWALKER:
      return windwalker::default_potion( this );
    default:
      return "disabled";
  }
}

std::string monk_t::default_flask() const
{
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      return brewmaster::default_flask( this );
    case MONK_WINDWALKER:
      return windwalker::default_flask( this );
    default:
      return "disabled";
  }
}

std::string monk_t::default_food() const
{
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      return brewmaster::default_food( this );
    case MONK_WINDWALKER:
      return windwalker::default_food( this );
    default:
      return "disabled";
  }
}

std::string monk_t::default_rune() const
{
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      return brewmaster::default_rune( this );
    case MONK_WINDWALKER:
      return windwalker::default_rune( this );
    default:
      return "disabled";
  }
}

std::string monk_t::default_temporary_enchant() const
{
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      return brewmaster::default_temporary_enchant( this );
    case MONK_WINDWALKER:
      return windwalker::default_temporary_enchant( this );
    default:
      return "disabled";
  }
}

void monk_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    switch ( specialization() )
    {
      case MONK_BREWMASTER:
        brewmaster::default_apl( this );
        break;
      case MONK_WINDWALKER:
        windwalker::default_apl( this );
        break;
      default:
        assert( false );
        break;
    }

    use_default_action_list = true;
  }

  base_t::init_action_list();
}
};  // namespace monk
