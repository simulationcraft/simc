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
  if ( player->true_level >= 80 )
    return "tempered_potion_3";
  return "disabled";
}

std::string default_flask( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "flask_of_alchemical_chaos_3";
  return "disabled";
}

std::string default_food( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "feast_of_the_midnight_masquerade";
  return "disabled";
}

std::string default_rune( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "crystallized";
  return "disabled";
}

std::string default_temporary_enchant( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "main_hand:ironclaw_whetstone_3/off_hand:ironclaw_whetstone_3";
  return "disabled";
}

void default_apl( monk_t* player )
{
  action_priority_list_t* precombat_    = player->get_action_priority_list( "precombat" );
  action_priority_list_t* default_      = player->get_action_priority_list( "default" );
  action_priority_list_t* item_actions_ = player->get_action_priority_list( "item_actions" );
  action_priority_list_t* race_actions_ = player->get_action_priority_list( "race_actions" );

  precombat_->add_action( "snapshot_stats" );
  precombat_->add_action( "potion" );

  default_->add_action( "auto_attack" );
  default_->add_action( "potion" );
  default_->add_action( "call_action_list,name=race_actions" );
  default_->add_action( "call_action_list,name=item_actions" );
  default_->add_action(
      "black_ox_brew,if=energy<40&(!talent.aspect_of_harmony.enabled|cooldown.celestial_brew.charges_fractional<1)" );
  default_->add_action(
      "celestial_brew,if=(buff.aspect_of_harmony_accumulator.value>0.3*health.max&buff.weapons_of_order.up&!dot.aspect_"
      "of_harmony_damage.ticking)" );
  default_->add_action(
      "celestial_brew,if=(buff.aspect_of_harmony_accumulator.value>0.3*health.max&!talent.weapons_of_order.enabled&!"
      "dot.aspect_of_harmony_damage.ticking)" );
  default_->add_action(
      "celestial_brew,if=(target.time_to_die<20&target.time_to_die>14&buff.aspect_of_harmony_accumulator.value>0.2*"
      "health.max)" );
  default_->add_action(
      "celestial_brew,if=(buff.aspect_of_harmony_accumulator.value>0.3*health.max&cooldown.weapons_of_order.remains>20&"
      "!dot.aspect_of_harmony_damage.ticking)" );
  default_->add_action(
      "celestial_brew,if=!buff.blackout_combo.up&(cooldown.celestial_brew.charges_fractional>1.8|(cooldown.celestial_"
      "brew.charges_fractional>1.2&cooldown.black_ox_brew.up))" );
  default_->add_action( "blackout_kick" );
  default_->add_action( "chi_burst,if=!talent.aspect_of_harmony.enabled|buff.balanced_stratagem_magic.stack>3" );
  default_->add_action( "weapons_of_order" );
  default_->add_action( "invoke_niuzao,if=!talent.call_to_arms.enabled" );
  default_->add_action(
      "invoke_niuzao,if=talent.call_to_arms.enabled&buff.call_to_arms_invoke_niuzao.down&buff.weapons_of_order.remains<"
      "16" );
  default_->add_action( "rising_sun_kick,if=!talent.fluidity_of_motion.enabled" );
  default_->add_action(
      "keg_smash,if=buff.weapons_of_order.up&(debuff.weapons_of_order_debuff.remains<1.8|debuff.weapons_of_order_"
      "debuff.stack<3-buff.blackout_combo.up|(buff.weapons_of_order.remains<3-buff.blackout_combo.up&buff.weapons_of_"
      "order.remains<1+cooldown.rising_sun_kick.remains))" );
  default_->add_action( "tiger_palm,if=buff.blackout_combo.up" );
  default_->add_action( "keg_smash,if=talent.scalding_brew.enabled" );
  default_->add_action(
      "spinning_crane_kick,if=talent.charred_passions.enabled&talent.scalding_brew.enabled&buff.charred_passions.up&"
      "buff.charred_passions.remains<3&dot.breath_of_fire.remains<9&active_enemies>4" );
  default_->add_action( "rising_sun_kick,if=talent.fluidity_of_motion.enabled" );
  default_->add_action(
      "purifying_brew,if=buff.blackout_combo.down&!(talent.call_to_arms.enabled|talent.invoke_niuzao_the_black_ox."
      "enabled)" );
  default_->add_action(
      "purifying_brew,if=buff.blackout_combo.down&(talent.call_to_arms.enabled|talent.invoke_niuzao_the_black_ox."
      "enabled)&(buff.invoke_niuzao_the_black_ox.up|buff.call_to_arms_invoke_niuzao.up)" );
  default_->add_action(
      "purifying_brew,if=buff.blackout_combo.down&(talent.call_to_arms.enabled|talent.invoke_niuzao_the_black_ox."
      "enabled)&cooldown.weapons_of_order.remains>10&cooldown.invoke_niuzao_the_black_ox.remains>10" );
  default_->add_action(
      "breath_of_fire,if=(buff.charred_passions.down&(!talent.scalding_brew.enabled|active_enemies<5))|!talent.charred_"
      "passions.enabled|(dot.breath_of_fire.remains<3&talent.scalding_brew.enabled)" );
  default_->add_action( "exploding_keg,if=!talent.rushing_jade_wind.enabled|buff.rushing_jade_wind.up" );
  default_->add_action(
      "rushing_jade_wind,if=talent.aspect_of_harmony.enabled&((buff.rushing_jade_wind.remains<2.5&buff.rushing_jade_"
      "wind.up)|!buff.rushing_jade_wind.up)" );
  default_->add_action( "keg_smash" );
  default_->add_action(
      "rushing_jade_wind,if=!talent.aspect_of_harmony.enabled&((buff.rushing_jade_wind.remains<2.5&buff.rushing_jade_"
      "wind.up)|!buff.rushing_jade_wind.up)" );
  default_->add_action( "tiger_palm,if=energy>40-cooldown.keg_smash.remains*energy.regen" );
  default_->add_action( "spinning_crane_kick,if=energy>40-cooldown.keg_smash.remains*energy.regen" );

  item_actions_->add_action( "use_item,name=tome_of_lights_devotion,if=buff.inner_resilience.up" );
  item_actions_->add_action(
      "use_item,name=unyielding_netherprism,if=buff.weapons_of_order.up&debuff.weapons_of_order_debuff.stack=4" );
  item_actions_->add_action( "use_item,name=unyielding_netherprism,if=!talent.weapons_of_order.enabled" );
  item_actions_->add_action(
      "use_item,name=lily_of_the_eternal_weave,if=buff.weapons_of_order.up&debuff.weapons_of_order_debuff.stack=4" );
  item_actions_->add_action( "use_item,name=lily_of_the_eternal_weave,if=!talent.weapons_of_order.enabled" );
  item_actions_->add_action(
      "use_item,name=signet_of_the_priory,if=buff.weapons_of_order.up&debuff.weapons_of_order_debuff.stack=4" );
  item_actions_->add_action( "use_item,name=signet_of_the_priory,if=!talent.weapons_of_order.enabled" );
  item_actions_->add_action( "use_items" );

  race_actions_->add_action( "blood_fury" );
  race_actions_->add_action( "berserking" );
  race_actions_->add_action( "arcane_torrent" );
  race_actions_->add_action( "lights_judgment" );
  race_actions_->add_action( "fireblood" );
  race_actions_->add_action( "ancestral_call" );
  race_actions_->add_action( "bag_of_tricks" );
}
};  // namespace brewmaster

namespace mistweaver
{
std::string default_potion( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "tempered_potion_3";
  return "disabled";
}

std::string default_flask( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "flask_of_alchemical_chaos_3";
  return "disabled";
}

std::string default_food( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "feast_of_the_midnight_masquerade";
  return "disabled";
}

std::string default_rune( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "crystallized";
  return "disabled";
}

std::string default_temporary_enchant( const monk_t* )
{
  return "disabled";
}

void default_apl( monk_t* player )
{
  action_priority_list_t* pre     = player->get_action_priority_list( "precombat" );
  action_priority_list_t* def     = player->get_action_priority_list( "default" );
  action_priority_list_t* racials = player->get_action_priority_list( "race_actions" );

  pre->add_action( "snapshot_stats" );
  pre->add_action( "potion" );

  for ( const auto& racial_action : player->get_racial_actions() )
    racials->add_action( racial_action );

  def->add_action( "auto_attack" );
  def->add_action( "potion" );
  def->add_action( "use_item,slot=trinket1" );
  def->add_action( "use_item,slot=trinket2" );
  def->add_action( "call_action_list,name=race_actions" );

  def->add_action( "touch_of_death" );
  def->add_action( "thunder_focus_tea" );
  def->add_action( "invoke_chiji,if=talent.invokers_delight" );
  def->add_action( "invoke_yulon,if=talent.invokers_delight" );
  def->add_action(
      "sheiluns_gift,if=talent.shaohaos_lessons&(buff.sheiluns_gift.stack>=10|(buff.sheiluns_gift.stack*4>=fight_"
      "remains&buff.sheiluns_gift.stack>=3)|(fight_style.dungeonslice&buff.sheiluns_gift.stack>=5&active_enemies>=4)"
      ")" );
  def->add_action( "celestial_conduit" );
  def->add_action( "rising_sun_kick,if=talent.secret_infusion&buff.thunder_focus_tea.up" );
  def->add_action( "spinning_crane_kick,if=buff.dance_of_chiji.up" );
  def->add_action( "chi_burst,if=active_enemies>=2" );
  def->add_action( "crackling_jade_lightning,if=buff.jade_empowerment.up" );

  def->add_action( "jadefire_stomp,if=active_enemies>=4&active_enemies<=10" );
  def->add_action( "spinning_crane_kick,if=active_enemies>=4" );

  def->add_action( "jadefire_stomp,if=buff.jadefire_stomp.down" );
  def->add_action( "rising_sun_kick,if=active_enemies<=2" );
  def->add_action(
      "blackout_kick,if=buff.teachings_of_the_monastery.stack>=3&(active_enemies>=2|cooldown.rising_sun_kick.remains>"
      "gcd)" );
  def->add_action( "tiger_palm" );
}
};  // namespace mistweaver

namespace windwalker
{
std::string default_potion( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "tempered_potion_3";
  return "disabled";
}

std::string default_flask( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "flask_of_alchemical_chaos_3";
  return "disabled";
}

std::string default_food( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "authentic_undermine_clam_chowder";
  return "disabled";
}

std::string default_rune( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "crystallized";
  return "disabled";
}

std::string default_temporary_enchant( const monk_t* player )
{
  if ( player->true_level >= 80 )
    return "main_hand:algari_mana_oil_3/off_hand:algari_mana_oil_3";
  return "disabled";
}

void live_apl( monk_t* player )
{
  action_priority_list_t* pre              = player->get_action_priority_list( "precombat" );
  action_priority_list_t* def              = player->get_action_priority_list( "default" );
  action_priority_list_t* trinkets         = player->get_action_priority_list( "trinkets" );
  action_priority_list_t* special_trinkets = player->get_action_priority_list( "special_trinkets" );
  action_priority_list_t* normal_opener    = player->get_action_priority_list( "normal_opener" );
  action_priority_list_t* cooldowns        = player->get_action_priority_list( "cooldowns" );
  action_priority_list_t* default_aoe      = player->get_action_priority_list( "default_aoe" );
  action_priority_list_t* default_cleave   = player->get_action_priority_list( "default_cleave" );
  action_priority_list_t* default_st       = player->get_action_priority_list( "default_st" );
  action_priority_list_t* fallback         = player->get_action_priority_list( "fallback" );

  pre->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  pre->add_action( "use_item,name=imperfect_ascendancy_serum" );

  // Define variable for double on use trinket situations
  pre->add_action( "variable,name=invoke_xuen_count,op=set,value=0" );

  def->add_action( "auto_attack,target_if=max:target.time_to_die" );
  def->add_action( "roll,if=movement.distance>5", "Move to target" );
  def->add_action( "chi_torpedo,if=movement.distance>5" );
  def->add_action( "flying_serpent_kick,if=movement.distance>5" );
  def->add_action( "spear_hand_strike,if=target.debuff.casting.react" );

  def->add_action( "potion,if=buff.invoke_xuen_the_white_tiger.up&buff.storm_earth_and_fire.up", "Potion" );
  def->add_action( "potion,if=!talent.invoke_xuen_the_white_tiger&buff.storm_earth_and_fire.up" );
  def->add_action( "potion,if=fight_remains<=30" );

  // Enable PI if available
  def->add_action( "variable,name=has_external_pi,value=cooldown.invoke_power_infusion_0.duration>0",
                   "Enable PI if available" );

  // Define special trinket situation S3 TWW
  def->add_action(
      "variable,name=special_case_trinket,value=talent.flurry_strikes&(trinket.1.cooldown.duration=120&trinket.1.has_"
      "use_buff|trinket.2.cooldown.duration=120&trinket.2.has_use_buff)&equipped.unyielding_netherprism&!talent.xuens_"
      "bond" );

  // Combine small HotJS into one condition
  def->add_action(
      "variable,name=small_hotjs_active,value=buff.heart_of_the_jade_serpent_cdr.up|buff.heart_of_the_jade_serpent_"
      "tww3_tier.up" );

  // Count Xuen usages
  def->add_action( "variable,name=invoke_xuen_count,op=add,value=1,if=prev.invoke_xuen_the_white_tiger" );

  // Define variables for CD Usage
  def->add_action(
      "variable,name=sef_condition,value=target.time_to_die>6&(cooldown.rising_sun_kick.remains|active_enemies>2|!"
      "talent.ordered_elements)&(prev.invoke_xuen_the_white_tiger|(talent.celestial_conduit|!talent.last_emperors_"
      "capacitor)&buff.bloodlust.up&(cooldown.strike_of_the_windlord.remains<5|!talent.strike_of_the_windlord)&talent."
      "sequenced_strikes|buff.invokers_delight.remains>15|(cooldown.strike_of_the_windlord.remains<5|!talent.strike_of_"
      "the_windlord)&cooldown.storm_earth_and_fire.full_recharge_time<cooldown.invoke_xuen_the_white_tiger.remains&"
      "cooldown.fists_of_fury.remains<5&(!talent.last_emperors_capacitor|talent.celestial_conduit)|talent.last_"
      "emperors_capacitor&buff.the_emperors_capacitor.stack>17&cooldown.invoke_xuen_the_white_tiger.remains>cooldown."
      "storm_earth_and_fire.full_recharge_time)|fight_remains<30&cooldown.invoke_xuen_the_white_tiger.remains>10&"
      "(cooldown.rising_sun_kick.remains|active_enemies>1)|buff.invokers_delight.remains>15&(cooldown.rising_sun_"
      "kick.remains|active_enemies>2|!talent.ordered_elements)|fight_style.patchwerk&buff.bloodlust.up&(cooldown."
      "rising_sun_kick.remains|active_enemies>2|!talent.ordered_elements)&talent.celestial_conduit&time>10",
      "Define Variables for CD Management" );
  def->add_action(
      "variable,name=xuen_dungeonslice_condition,value=active_enemies=1&(time<10|talent.xuens_bond&talent.celestial_"
      "conduit&target.time_to_die>14)|active_enemies>1&cooldown.storm_earth_and_fire.ready&target.time_to_die>14&("
      "active_enemies>2|debuff.acclamation.up|!talent.ordered_elements&time<5)&((chi>2&!talent.ordered_elements|talent."
      "ordered_elements|!talent.ordered_elements&energy<50)|talent.sequenced_strikes&talent.energy_burst&talent."
      "revolving_whirl)|fight_remains<30|active_enemies>3&target.time_to_die>5|fight_style.dungeonslice&time>50&target."
      "time_to_die>1&talent.xuens_bond" );
  def->add_action(
      "variable,name=xuen_condition,value=(fight_style.DungeonSlice&active_enemies=1&(time<10|talent.xuens_bond&talent."
      "celestial_conduit)|!fight_style.dungeonslice|active_enemies>1)&cooldown.storm_earth_and_fire.ready&(target.time_"
      "to_die>14&!fight_style.dungeonroute|target.time_to_die>22)&(active_enemies>2|debuff.acclamation.up|!talent."
      "ordered_elements&time<5)&(chi>2&talent.ordered_elements|chi>5|chi>3&energy<50|energy<50&active_enemies=1|prev."
      "tiger_palm&!talent.ordered_elements&time<5)|fight_remains<30|fight_style.dungeonroute&talent.celestial_conduit&"
      "target.time_to_die>14" );
  def->add_action(
      "variable,name=xuen_dungeonroute_condition,value=cooldown.storm_earth_and_fire.ready&(active_enemies>1&cooldown."
      "storm_earth_and_fire.ready&target.time_to_die>22&(active_enemies>2|debuff.acclamation.up|!talent.ordered_"
      "elements&time<5)&((chi>2&!talent.ordered_elements|talent.ordered_elements|!talent.ordered_elements&energy<50)|"
      "talent.sequenced_strikes&talent.energy_burst&talent.revolving_whirl)|fight_remains<30|active_enemies>3&target."
      "time_to_die>15|time>50&(target.time_to_die>10&talent.xuens_bond|target.time_to_die>20))|buff.storm_earth_and_"
      "fire.remains>5" );
  def->add_action(
      "variable,name=sef_dungeonroute_condition,value=time<50&target.time_to_die>10&(buff.bloodlust.up|active_enemies>"
      "2|cooldown.strike_of_the_windlord.remains<2|talent.last_emperors_capacitor&buff.the_emperors_capacitor.stack>17)"
      "|target.time_to_die>10&(cooldown.storm_earth_and_fire.full_recharge_time<cooldown.invoke_xuen_the_white_tiger."
      "remains|cooldown.invoke_xuen_the_white_tiger.remains<30&(cooldown.storm_earth_and_fire.full_recharge_time<30|"
      "cooldown.storm_earth_and_fire.full_recharge_time<40&talent.flurry_strikes))&(talent.sequenced_strikes&talent."
      "energy_burst&talent.revolving_whirl|talent.flurry_strikes|chi>3|energy<50)&(active_enemies>2|!talent.ordered_"
      "elements|cooldown.rising_sun_kick.remains)&!talent.flurry_strikes|target.time_to_die>10&talent.flurry_strikes&("
      "active_enemies>2|!talent.ordered_elements|cooldown.rising_sun_kick.remains)&(talent.last_emperors_capacitor&"
      "buff.the_emperors_capacitor.stack>17&cooldown.storm_earth_and_fire.full_recharge_time<cooldown.invoke_xuen_the_"
      "white_tiger.remains&cooldown.invoke_xuen_the_white_tiger.remains>15|!talent.last_emperors_capacitor&cooldown."
      "storm_earth_and_fire.full_recharge_time<cooldown.invoke_xuen_the_white_tiger.remains&cooldown.invoke_xuen_the_"
      "white_tiger.remains>15)" );

  // Use Trinkets
  def->add_action( "call_action_list,name=trinkets,if=!variable.special_case_trinket", "Use Trinkets" );
  def->add_action( "call_action_list,name=special_trinkets,if=variable.special_case_trinket" );

  // Openers
  def->add_action( "call_action_list,name=normal_opener,if=time<4&active_enemies<3", "Opener" );

  // Use Cooldowns
  def->add_action( "call_action_list,name=cooldowns,if=talent.storm_earth_and_fire", "Use Cooldowns" );

  // Default priority (aoe>=5; cleave=3-4; st<3)
  def->add_action( "call_action_list,name=default_aoe,if=active_enemies>=5", "Default Priority" );
  def->add_action( "call_action_list,name=default_cleave,if=active_enemies>2&active_enemies<5" );
  def->add_action( "call_action_list,name=default_st,if=active_enemies<3" );

  // Fallback
  def->add_action( "call_action_list,name=fallback" );

  // irrelevant racials
  def->add_action( "arcane_torrent,if=chi<chi.max&energy<55" );
  def->add_action( "bag_of_tricks,if=buff.storm_earth_and_fire.down" );
  def->add_action( "lights_judgment,if=buff.storm_earth_and_fire.down" );
  def->add_action( "haymaker,if=buff.storm_earth_and_fire.down" );
  def->add_action( "rocket_barrage,if=buff.storm_earth_and_fire.down" );
  // earthen racial not implemented yet
  // def->add_action( "azerite_surge,if=buff.storm_earth_and_fire.down" );
  def->add_action( "arcane_pulse,if=buff.storm_earth_and_fire.down" );

  // Trinkets
  trinkets->add_action(
      "use_item,slot=trinket1,if=trinket.1.has_use_buff&trinket.2.has_use_buff&buff.invoke_xuen_the_white_tiger.up&"
      "variable.invoke_xuen_count%%2|fight_remains<20",
      "Double on Use Stats" );
  trinkets->add_action(
      "use_item,slot=trinket2,if=trinket.1.has_use_buff&trinket.2.has_use_buff&buff.invoke_xuen_the_white_tiger.up|"
      "fight_"
      "remains<20" );
  trinkets->add_action(
      "use_item,slot=trinket1,if=trinket.1.has_use_buff&!trinket.2.has_use_buff&buff.invoke_xuen_the_white_tiger.up|"
      "fight_"
      "remains<20",
      "Trinket 1 On use Stats" );
  trinkets->add_action(
      "use_item,slot=trinket2,if=trinket.1.has_use_buff&!trinket.2.has_use_buff&cooldown.invoke_xuen_the_white_tiger."
      "remains>30|fight_remains<20" );
  trinkets->add_action(
      "use_item,slot=trinket1,if=!trinket.1.has_use_buff&trinket.2.has_use_buff&cooldown.invoke_xuen_the_white_tiger."
      "remains>30|fight_remains<20",
      "Trinket 2 On use Stats" );
  trinkets->add_action(
      "use_item,slot=trinket2,if=!trinket.1.has_use_buff&trinket.2.has_use_buff&buff.invoke_xuen_the_white_tiger.up|"
      "fight_"
      "remains<20" );
  trinkets->add_action( "use_item,slot=trinket1,if=!trinket.1.has_use_buff&!trinket.2.has_use_buff", "No Stat on Use" );
  trinkets->add_action( "use_item,slot=trinket2,if=!trinket.1.has_use_buff&!trinket.2.has_use_buff" );
  trinkets->add_action( "use_item,slot=main_hand", "Use Weapon" );

  // Special Case Trinkets
  special_trinkets->add_action(
      "use_item,slot=trinket1,if=trinket.1.cooldown.duration=120&buff.invoke_xuen_the_white_tiger.up|fight_remains<30",
      "Prism + 2 Min on use without XB talented" );
  special_trinkets->add_action(
      "use_item,slot=trinket2,if=trinket.2.cooldown.duration=120&buff.invoke_xuen_the_white_tiger.up|fight_remains<"
      "30" );
  special_trinkets->add_action(
      "use_item,name=unyielding_netherprism,if=cooldown.invoke_xuen_the_white_tiger.remains&buff.storm_earth_and_fire."
      "remains>10&buff.latent_energy.stack>2" );
  special_trinkets->add_action( "use_item,slot=main_hand", "Use Weapon" );

  // Cooldowns
  cooldowns->add_action(
      "invoke_external_buff,name=power_infusion,if=buff.invoke_xuen_the_white_tiger.up&(!buff.bloodlust.up|buff."
      "bloodlust."
      "up&cooldown.strike_of_the_windlord.remains)",
      "Use <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> while <a "
      "href='https://www.wowhead.com/spell=123904/invoke-xuen-the-white-tiger'>Invoke Xuen, the White Tiger</a> is "
      "active." );
  cooldowns->add_action(
      "storm_earth_and_fire,target_if=max:target.time_to_die,if=fight_style.dungeonroute&buff.invokers_delight.remains>"
      "15&(active_enemies>2|!talent.ordered_elements|cooldown.rising_sun_kick.remains)" );
  cooldowns->add_action(
      "tiger_palm,if=(target.time_to_die>14&!fight_style.dungeonroute|"
      "target.time_to_die>22)&!cooldown.invoke_xuen_the_white_tiger.remains&(chi<5&!talent.ordered_elements|chi<3)&("
      "combo_strike|!talent.hit_combo)" );
  cooldowns->add_action(
      "invoke_xuen_the_white_tiger,target_if=max:target.time_to_die,if=!fight_style.dungeonroute&(target.time_to_die>"
      "12|!talent.xuens_bond&target.time_to_die>8)&set_bonus.tww3_2pc&talent.celestial_conduit&cooldown.strike_of_the_"
      "windlord.remains<3&(chi>2&talent.ordered_elements|chi>5|chi>3&energy<50|energy<50&active_enemies=1|prev.tiger_"
      "palm&!talent.ordered_elements&time<5)|fight_remains<20" );
  cooldowns->add_action(
      "invoke_xuen_the_white_tiger,target_if=max:target.time_to_die,if=(!set_bonus.tww3_2pc|!talent.celestial_conduit|!"
      "fight_style.patchwerk)&(variable.xuen_condition&!fight_style.dungeonslice&!fight_style.dungeonroute|variable."
      "xuen_dungeonslice_condition&fight_style.Dungeonslice|variable.xuen_dungeonroute_condition&fight_style."
      "dungeonroute)" );
  cooldowns->add_action(
      "storm_earth_and_fire,target_if=max:target.time_to_die,if=talent.flurry_strikes&cooldown.invoke_xuen_the_white_"
      "tiger.remains&buff.bloodlust.up&cooldown.rising_sun_kick.remains|variable.sef_condition&!fight_style."
      "dungeonroute|variable.sef_dungeonroute_condition&fight_style.dungeonroute|fight_style.patchwerk&active_enemies="
      "1&talent.flurry_strikes&fight_remains<60&cooldown.invoke_xuen_the_white_tiger.remains>fight_remains&cooldown."
      "rising_sun_kick.remains&buff.the_emperors_capacitor.stack>15" );
  cooldowns->add_action( "touch_of_karma,target_if=max:target.time_to_die" );
  // CD relevant racials
  cooldowns->add_action(
      "ancestral_call,if=buff.invoke_xuen_the_white_tiger.remains>15|!talent.invoke_xuen_the_white_tiger&(!talent."
      "storm_earth_and_fire&(cooldown.strike_of_the_windlord.ready|!talent.strike_of_the_windlord&cooldown.fists_of_"
      "fury.ready)|buff.storm_earth_and_fire.remains>10)|fight_remains<20" );
  cooldowns->add_action(
      "blood_fury,if=buff.invoke_xuen_the_white_tiger.remains>15|!talent.invoke_xuen_the_white_tiger&(!talent.storm_"
      "earth_and_fire&(cooldown.strike_of_the_windlord.ready|!talent.strike_of_the_windlord&cooldown.fists_of_fury."
      "ready)|buff.storm_earth_and_fire.remains>10)|fight_remains<20" );
  cooldowns->add_action(
      "fireblood,if=buff.invoke_xuen_the_white_tiger.remains>15|!talent.invoke_xuen_the_white_tiger&(!talent.storm_"
      "earth_and_fire&(cooldown.strike_of_the_windlord.ready|!talent.strike_of_the_windlord&cooldown.fists_of_fury."
      "ready)|buff.storm_earth_and_fire.remains>10)|fight_remains<20" );
  cooldowns->add_action(
      "berserking,if=buff.invoke_xuen_the_white_tiger.remains>15|!talent.invoke_xuen_the_white_tiger&(!talent.storm_"
      "earth_and_fire&(cooldown.strike_of_the_windlord.ready|!talent.strike_of_the_windlord&cooldown.fists_of_fury."
      "ready)|buff.storm_earth_and_fire.remains>10)|fight_remains<20" );

  // Normal Opener
  normal_opener->add_action( "tiger_palm,if=chi<6&combo_strike", "normal opener" );
  normal_opener->add_action( "rising_sun_kick" );

  // >=5 Target priority
  default_aoe->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=(energy>55&talent.inner_peace|energy>60&!talent."
      "inner_peace)&combo_strike&chi.max-chi>=2&buff.teachings_of_the_monastery.stack<buff.teachings_of_the_monastery."
      "max_stack&(talent.energy_burst&!buff.bok_proc.up)&!buff.ordered_elements.up|(talent.energy_burst&!buff.bok_proc."
      "up)&!buff.ordered_elements.up&!cooldown.fists_of_fury.remains&chi<3|(prev.strike_of_the_windlord|cooldown."
      "strike_of_the_windlord.remains)&cooldown.celestial_conduit.remains<2&buff.ordered_elements.up&chi<5&combo_"
      "strike",
      ">=5 Targets" );
  default_aoe->add_action(
      "touch_of_death,target_if=min:target.time_to_die,if=!variable.small_hotjs_active&!buff.heart_of_the_jade_serpent_"
      "cdr_celestial.up|fight_remains<10" );
  default_aoe->add_action( "whirling_dragon_punch,target_if=max:target.time_to_die,if=buff.dance_of_chiji.stack<2" );
  default_aoe->add_action(
      "strike_of_the_windlord,target_if=max:target.time_to_die,if=talent.gale_force&cooldown.invoke_xuen_the_white_"
      "tiger.remains>10&set_bonus.tww3_2pc&!talent.flurry_strikes" );
  default_aoe->add_action(
      "slicing_winds,if=set_bonus.tww3_2pc&talent.celestial_conduit&variable.small_hotjs_active&!talent.flurry_"
      "strikes" );
  default_aoe->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&buff.storm_earth_and_fire.remains>2&talent.flurry_"
      "strikes&energy.time_to_max<=gcd.max*3&set_bonus.tww3_4pc" );
  default_aoe->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=buff.dance_of_chiji.stack=2&combo_strike" );
  default_aoe->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.chi_energy.stack>29&cooldown.fists_of_"
      "fury.remains<5" );
  default_aoe->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&buff.storm_earth_and_fire.remains>2&talent.flurry_"
      "strikes&energy.time_to_max<=gcd."
      "max*3&cooldown.fists_of_fury.remains&(!talent.xuens_battlegear|chi<6)&set_bonus.tww3_4pc" );
  default_aoe->add_action(
      "slicing_winds,if=variable.small_hotjs_active|buff.heart_of_the_jade_serpent_cdr_celestial.up" );
  default_aoe->add_action(
      "celestial_conduit,if=buff.storm_earth_and_fire.up&cooldown.strike_of_the_windlord.remains&(!variable.small_"
      "hotjs_active|debuff.gale_force.remains<5)&(talent.xuens_bond|!talent.xuens_bond&buff.invokers_delight.up)|fight_"
      "remains<15|fight_style.dungeonroute&buff.invokers_delight.up&cooldown.strike_of_the_windlord.remains&buff.storm_"
      "earth_and_fire.remains<8" );
  default_aoe->add_action(
      "fists_of_fury,target_if=max:target.time_to_die,if=combo_strike&buff.heart_of_the_jade_serpent_cdr_celestial.up|"
      "variable.small_hotjs_active" );
  default_aoe->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=cooldown.whirling_dragon_punch.remains<2&cooldown.fists_of_"
      "fury.remains>1&buff.dance_of_chiji.stack<2|!buff.storm_earth_and_fire.up&buff.pressure_point.up" );
  default_aoe->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&buff.storm_earth_and_fire.remains>2&talent.flurry_"
      "strikes&energy.time_to_max<=gcd."
      "max*3&set_bonus.tww3_4pc" );
  default_aoe->add_action(
      "whirling_dragon_punch,target_if=max:target.time_to_die,if=!talent.revolving_whirl|talent.revolving_whirl&buff."
      "dance_of_chiji.stack<2&active_enemies>2" );
  default_aoe->add_action(
      "blackout_kick,target_if=max:target.time_to_die,if=combo_strike&buff.bok_proc.up&chi<2&talent."
      "energy_burst&energy<55" );
  default_aoe->add_action(
      "strike_of_the_windlord,target_if=max:target.time_to_die,if=(time>5|buff.invokers_delight.up&buff.storm_earth_"
      "and_fire.up)&(cooldown.invoke_xuen_the_white_tiger.remains>"
      "15|talent.flurry_strikes)" );
  default_aoe->add_action( "slicing_winds" );
  default_aoe->add_action(
      "blackout_kick,target_if=max:target.time_to_die,if=buff.teachings_of_the_monastery.stack=8&talent."
      "shadowboxing_treads" );
  default_aoe->add_action(
      "crackling_jade_lightning,target_if=max:target.time_to_die,if=buff.the_emperors_capacitor.stack>19&combo_strike&"
      "talent.power_of_the_thunder_king&cooldown.invoke_xuen_the_white_tiger.remains>10" );
  default_aoe->add_action(
      "fists_of_fury,target_if=max:target.time_to_die,if=(talent.flurry_strikes|talent.xuens_battlegear&(cooldown."
      "invoke_xuen_the_white_tiger.remains>5&fight_style.patchwerk|cooldown.invoke_xuen_the_white_tiger.remains>9)|"
      "cooldown.invoke_xuen_the_white_tiger.remains>10)" );
  default_aoe->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&energy.time_to_max<=gcd.max*3&talent."
      "flurry_strikes&buff.wisdom_of_the_wall_flurry.up&chi<6" );
  default_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&chi>5" );
  default_aoe->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.dance_of_chiji.up&buff.chi_energy."
      "stack>29&cooldown.fists_of_fury.remains<5" );
  default_aoe->add_action(
      "rising_sun_kick,if=buff.pressure_point.up&cooldown.fists_of_fury."
      "remains>2" );
  default_aoe->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&buff.storm_earth_and_fire.remains>2&talent.flurry_"
      "strikes&active_enemies>5&set_bonus."
      "tww3_4pc" );
  default_aoe->add_action(
      "blackout_kick,target_if=max:target.time_to_die,if=talent.shadowboxing_treads&talent.courageous_"
      "impulse&combo_strike&buff.bok_proc.stack=2" );
  default_aoe->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.dance_of_chiji.up" );
  default_aoe->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.ordered_elements.up&talent.crane_"
      "vortex&active_enemies>2" );
  default_aoe->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&energy.time_to_max<=gcd.max*3&talent."
      "flurry_strikes&buff.ordered_elements.up" );
  default_aoe->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&chi.deficit>=2&(!buff.ordered_"
      "elements.up|energy.time_to_max<=gcd.max*3)" );
  default_aoe->add_action(
      "jadefire_stomp,target_if=max:target.time_to_die,if=talent.Singularly_Focused_Jade|talent.jadefire_harmony" );
  default_aoe->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&!buff.ordered_elements.up&talent.crane_"
      "vortex&active_enemies>2&chi>4" );
  default_aoe->add_action(
      "blackout_kick,target_if=max:target.time_to_die,if=combo_strike&cooldown.fists_of_fury.remains&("
      "buff.teachings_of_the_monastery.stack>3|buff.ordered_elements.up)&(talent.shadowboxing_treads|buff.bok_proc."
      "up)" );
  default_aoe->add_action(
      "blackout_kick,target_if=max:target.time_to_die,if=combo_strike&!cooldown.fists_of_fury.remains&"
      "chi<3" );
  default_aoe->add_action(
      "blackout_kick,target_if=max:target.time_to_die,if=talent.shadowboxing_treads&talent.courageous_"
      "impulse&combo_strike&buff.bok_proc.up" );
  default_aoe->add_action( "spinning_crane_kick,if=combo_strike&(chi>3|energy>55)" );
  default_aoe->add_action(
      "blackout_kick,target_if=max:target.time_to_die,if=combo_strike&(buff.ordered_elements.up|buff.bok_"
      "proc.up&chi.deficit>=1&talent.energy_burst)&cooldown.fists_of_fury.remains" );
  default_aoe->add_action(
      "blackout_kick,target_if=max:target.time_to_die,if=combo_strike&cooldown.fists_of_fury.remains&("
      "chi>2|energy>60|buff.bok_proc.up)" );
  default_aoe->add_action( "jadefire_stomp,target_if=max:debuff.acclamation.stack" );
  default_aoe->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&buff.ordered_elements.up&chi.deficit>="
      "1" );
  default_aoe->add_action( "chi_burst,if=!buff.ordered_elements.up" );
  default_aoe->add_action( "chi_burst" );
  default_aoe->add_action( "spinning_crane_kick,if=combo_strike&buff.ordered_elements.up&talent.hit_combo" );
  default_aoe->add_action(
      "blackout_kick,target_if=max:target.time_to_die,if=buff.ordered_elements.up&!talent.hit_combo&"
      "cooldown.fists_of_fury.remains" );
  default_aoe->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=prev.tiger_palm&chi<3&!cooldown.fists_of_fury.remains" );

  // 3-4 targets
  default_cleave->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=buff.storm_earth_and_fire.remains>13&combo_strike&talent."
      "glory_of_the_dawn",
      "2-4 targets" );
  default_cleave->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=combo_strike&!cooldown.whirling_dragon_punch.remains&("
      "cooldown.fists_of_fury.remains>1&!talent.xuens_battlegear|cooldown.fists_of_fury.remains>4)&!buff.invokers_"
      "delight.up&!talent.flurry_strikes&buff.dance_of_chiji.stack<2&!talent.glory_of_the_dawn" );
  default_cleave->add_action(
      "whirling_dragon_punch,target_if=max:target.time_to_die,if=variable.small_hotjs_active&buff.dance_of_chiji.stack<"
      "2&prev.rising_sun_kick" );
  default_cleave->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&buff.storm_earth_and_fire.remains>2&talent.flurry_"
      "strikes&energy.time_to_max<=gcd.max*3&set_bonus.tww3_4pc" );
  default_cleave->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&buff.storm_earth_and_fire.remains>2&talent.flurry_"
      "strikes&!talent.xuens_battlegear&set_bonus.tww3_4pc" );
  default_cleave->add_action(
      "strike_of_the_windlord,target_if=max:target.time_to_die,if=talent.gale_force&cooldown.invoke_xuen_the_white_"
      "tiger.remains>10&set_bonus.tww3_2pc&!"
      "talent.flurry_strikes" );
  default_cleave->add_action(
      "slicing_winds,if=set_bonus.tww3_2pc&talent.celestial_conduit&variable.small_hotjs_active&!talent.flurry_"
      "strikes" );
  default_cleave->add_action( "spinning_crane_kick,if=buff.dance_of_chiji.stack=2&combo_strike" );
  default_cleave->add_action(
      "whirling_dragon_punch,target_if=max:target.time_to_die,if=variable.small_hotjs_active&buff.dance_of_chiji.stack<"
      "2" );
  default_cleave->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=buff.pressure_point.up&active_enemies<4&cooldown.fists_of_"
      "fury.remains>4" );
  default_cleave->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=cooldown.whirling_dragon_punch.remains<2&cooldown.fists_of_"
      "fury.remains<3&buff.dance_of_chiji.stack<2&cooldown.fists_of_fury.remains" );
  default_cleave->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.dance_of_chiji.stack=2&active_enemies>"
      "3" );
  default_cleave->add_action(
      "celestial_conduit,target_if=max:target.time_to_die,if=buff.storm_earth_and_fire.up&debuff.gale_force.remains<5&"
      "cooldown.strike_of_the_windlord.remains&(!buff.heart_of_the_jade_serpent_cdr.up|debuff.gale_force.remains<5)&("
      "talent.xuens_bond|!talent.xuens_bond&buff.invokers_delight.up)|fight_remains<15|fight_style.dungeonroute&buff."
      "invokers_delight.up&cooldown.strike_of_the_windlord.remains&buff.storm_earth_and_fire.remains<8|fight_remains<"
      "10" );
  default_cleave->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&(energy>55&talent.inner_peace|energy>60&!talent."
      "inner_peace)&combo_strike&chi.max-chi>=2&buff.teachings_of_the_monastery.stack<buff.teachings_of_the_monastery."
      "max_stack&(talent.energy_burst&!buff.bok_proc.up|!talent.energy_burst)&!buff.ordered_elements.up|(talent.energy_"
      "burst&!buff.bok_proc.up|!talent.energy_burst)&!buff.ordered_elements.up&!cooldown.fists_of_fury.remains&chi<3|("
      "prev.strike_of_the_windlord|cooldown.strike_of_the_windlord.remains)&cooldown.celestial_conduit.remains<2&buff."
      "ordered_elements.up&chi<5&combo_strike|(!variable.small_hotjs_active|!buff.heart_of_the_jade_serpent_"
      "cdr_celestial.up)&combo_strike&chi.deficit>=2&!buff.ordered_elements.up" );
  default_cleave->add_action(
      "touch_of_death,target_if=max:target.time_to_die,if=!variable.small_hotjs_active&!buff.heart_of_the_jade_serpent_"
      "cdr_celestial.up|fight_remains<10" );
  default_cleave->add_action(
      "whirling_dragon_punch,target_if=max:target.time_to_die,if=variable.small_hotjs_active&buff.dance_of_"
      "chiji.stack<2" );
  default_cleave->add_action( "whirling_dragon_punch,target_if=max:target.time_to_die,if=buff.dance_of_chiji.stack<2" );
  default_cleave->add_action(
      "slicing_winds,if=variable.small_hotjs_active|buff.heart_of_the_jade_serpent_cdr_celestial.up" );
  default_cleave->add_action(
      "blackout_kick,if=combo_strike&talent.courageous_impulse&talent.shadowboxing_treads&buff.bok_proc.stack=2&"
      "cooldown.fists_of_fury.remains&active_enemies=3" );
  default_cleave->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=buff.invoke_xuen_the_white_tiger.down&prev.tiger_palm&time<"
      "5&talent.ordered_elements&(talent.glory_of_the_dawn|active_enemies<3)|variable.small_hotjs_active&buff.pressure_"
      "point.up&cooldown.fists_of_fury.remains&(talent.glory_of_the_dawn|active_enemies<3)" );
  default_cleave->add_action(
      "fists_of_fury,target_if=max:target.time_to_die,if=combo_strike&buff.heart_of_the_jade_serpent_cdr_celestial.up|"
      "variable.small_hotjs_active" );
  default_cleave->add_action(
      "whirling_dragon_punch,target_if=max:target.time_to_die,if=buff.heart_of_the_jade_serpent_cdr_celestial.up" );
  default_cleave->add_action(
      "strike_of_the_windlord,target_if=max:target.time_to_die,if=talent.gale_force&buff.invokers_delight.up&(buff."
      "bloodlust.up|!buff.heart_of_the_jade_serpent_cdr_celestial.up)" );
  default_cleave->add_action(
      "fists_of_fury,target_if=max:target.time_to_die,if=buff.power_infusion.up&buff.bloodlust.up" );
  default_cleave->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=buff.power_infusion.up&buff.bloodlust.up&active_enemies<3" );
  default_cleave->add_action(
      "blackout_kick,target_if=max:target.time_to_die,if=buff.teachings_of_the_monastery.stack=8&(active_"
      "enemies<3|talent.shadowboxing_treads)" );
  default_cleave->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&buff.storm_earth_and_fire.remains>2&talent.flurry_"
      "strikes&!talent.xuens_battlegear&"
      "set_bonus.tww3_4pc" );
  default_cleave->add_action(
      "whirling_dragon_punch,target_if=max:target.time_to_die,if=!talent.revolving_whirl|talent.revolving_whirl&buff."
      "dance_of_chiji.stack<2&active_enemies>2|active_enemies<3" );
  default_cleave->add_action(
      "strike_of_the_windlord,target_if=max:target.time_to_die,if=time>5&(cooldown.invoke_xuen_the_white_tiger."
      "remains>15|talent.flurry_strikes)&(cooldown.fists_of_fury.remains<2|cooldown.celestial_conduit.remains<10)" );
  default_cleave->add_action( "slicing_winds" );
  default_cleave->add_action(
      "crackling_jade_lightning,target_if=max:target.time_to_die,if=buff.the_emperors_capacitor.stack>19&combo_strike&"
      "talent.power_of_the_thunder_king&cooldown.invoke_xuen_the_white_tiger.remains>10" );
  default_cleave->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.dance_of_chiji.stack=2" );
  default_cleave->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&energy.time_to_max<=gcd.max*3&talent."
      "flurry_strikes&active_enemies<5&buff.wisdom_of_the_wall_flurry.up&active_enemies<4" );
  default_cleave->add_action(
      "fists_of_fury,target_if=max:target.time_to_die,if=(talent.flurry_strikes|talent.xuens_battlegear|!talent.xuens_"
      "battlegear&(cooldown.strike_of_the_windlord.remains>1|variable.small_hotjs_active|buff.heart_of_the_"
      "jade_serpent_cdr_celestial.up))&(talent.flurry_strikes|talent.xuens_battlegear&(cooldown.invoke_xuen_the_white_"
      "tiger.remains>5&fight_style.patchwerk|cooldown.invoke_xuen_the_white_tiger.remains>9)|cooldown.invoke_xuen_the_"
      "white_tiger.remains>10)" );
  default_cleave->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&energy.time_to_max<=gcd.max*3&talent."
      "flurry_strikes&active_enemies<5&buff.wisdom_of_the_wall_flurry.up" );
  default_cleave->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.dance_of_chiji.up&buff.chi_energy."
      "stack>29" );
  default_cleave->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=chi>4&(active_enemies<3|talent.glory_of_the_dawn)|chi>2&"
      "energy>50&(active_enemies<3|talent.glory_of_the_dawn)|cooldown.fists_of_fury.remains>2&(active_enemies<3|talent."
      "glory_of_the_dawn)" );
  default_cleave->add_action(
      "blackout_kick,target_if=max:target.time_to_die,if=talent.shadowboxing_treads&talent.courageous_"
      "impulse&combo_strike&buff.bok_proc.stack=2" );
  default_cleave->add_action(
      "blackout_kick,target_if=max:target.time_to_die,if=buff.teachings_of_the_monastery.stack=4&!talent."
      "knowledge_of_the_broken_temple&talent.shadowboxing_treads&active_enemies<3" );
  default_cleave->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.dance_of_chiji.up" );
  default_cleave->add_action(
      "blackout_kick,target_if=max:target.time_to_die,if=talent.shadowboxing_treads&talent.courageous_"
      "impulse&combo_strike&buff.bok_proc.up" );
  default_cleave->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&energy.time_to_max<=gcd.max*3&talent."
      "flurry_strikes&active_enemies<5" );
  default_cleave->add_action(
      "tiger_palm,target_if=max:target.time_to_die,if=combo_strike&chi.deficit>=2&(!buff.ordered_"
      "elements.up|energy.time_to_max<=gcd.max*3)" );
  default_cleave->add_action(
      "blackout_kick,target_if=max:target.time_to_die,if=combo_strike&cooldown.fists_of_fury.remains&"
      "buff.teachings_of_the_monastery.stack>3&cooldown.rising_sun_kick.remains" );
  default_cleave->add_action(
      "jadefire_stomp,if=talent.Singularly_Focused_Jade|talent.jadefire_"
      "harmony" );
  default_cleave->add_action(
      "blackout_kick,if=combo_strike&cooldown.fists_of_fury.remains&("
      "buff.teachings_of_the_monastery.stack>3|buff.ordered_elements.up)&(talent.shadowboxing_treads|buff.bok_proc.up|"
      "buff.ordered_elements.up)" );
  default_cleave->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&!buff.ordered_elements.up&talent.crane_"
      "vortex&active_enemies>2&chi>4" );
  default_cleave->add_action( "chi_burst,if=!buff.ordered_elements.up" );
  default_cleave->add_action(
      "blackout_kick,if=combo_strike&(buff.ordered_elements.up|buff.bok_"
      "proc.up&chi.deficit>=1&talent.energy_burst)&cooldown.fists_of_fury.remains" );
  default_cleave->add_action(
      "blackout_kick,if=combo_strike&cooldown.fists_of_fury.remains&("
      "chi>2|energy>60|buff.bok_proc.up)" );
  default_cleave->add_action( "jadefire_stomp,target_if=max:debuff.acclamation.stack" );
  default_cleave->add_action(
      "tiger_palm,if=combo_strike&buff.ordered_elements.up&chi.deficit>="
      "1" );
  default_cleave->add_action( "chi_burst" );
  default_cleave->add_action( "spinning_crane_kick,if=combo_strike&buff.ordered_elements.up&talent.hit_combo" );
  default_cleave->add_action(
      "blackout_kick,if=buff.ordered_elements.up&!talent.hit_combo&"
      "cooldown.fists_of_fury.remains" );
  default_cleave->add_action( "blackout_kick,if=combo_strike&!cooldown.fists_of_fury.remains&prev.tiger_palm" );
  default_cleave->add_action( "tiger_palm,if=prev.tiger_palm&chi<3&!cooldown.fists_of_fury.remains" );

  // 1-2 Target priority
  default_st->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=combo_strike&buff.pressure_point.up&variable.small_hotjs_"
      "active",
      "1 target" );
  default_st->add_action( "slicing_winds,if=set_bonus.tww3_2pc&talent.celestial_conduit&variable.small_hotjs_active" );
  default_st->add_action(
      "tiger_palm,if=combo_strike&!cooldown.celestial_conduit.remains&buff.pressure_point.up&chi<5&time<10" );
  default_st->add_action(
      "fists_of_fury,target_if=max:debuff.acclamation.stack,if=active_enemies>1&(combo_strike&buff.heart_of_the_jade_"
      "serpent_cdr_celestial.up|variable.small_hotjs_active)" );
  default_st->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=combo_strike&(buff.pressure_point.up&!variable.small_hotjs_"
      "active&buff.heart_of_the_jade_"
      "serpent_cdr_celestial.up|buff.invokers_delight.up|buff.bloodlust.up|buff.pressure_point.up&cooldown.fists_of_"
      "fury.remains|talent.flurry_strikes)" );
  default_st->add_action(
      "tiger_palm,if=chi<5&combo_strike&!variable.small_hotjs_active&!"
      "buff.heart_of_the_jade_serpent_cdr_celestial.up&energy.time_to_max<=gcd.max*3" );
  default_st->add_action(
      "tiger_palm,if=combo_strike&buff.storm_earth_and_fire.remains>2&talent.flurry_strikes&set_bonus.tww3_4pc" );
  default_st->add_action(
      "tiger_palm,if=combo_strike&buff.storm_earth_and_fire.remains&talent.flurry_strikes&set_bonus.tww3_4pc&!buff."
      "bloodlust.up" );
  default_st->add_action(
      "whirling_dragon_punch,target_if=max:debuff.acclamation.stack,if=!buff.heart_of_the_jade_serpent_cdr_celestial."
      "up&!buff.dance_of_chiji.stack=2&!set_bonus.tww3_2pc|active_enemies>1&buff.dance_of_chiji.stack<2&cooldown."
      "celestial_conduit.remains" );
  default_st->add_action(
      "celestial_conduit,if=buff.storm_earth_and_fire.up&(!buff.heart_of_the_jade_serpent_cdr.up|debuff.gale_force."
      "remains<5)&cooldown.strike_of_the_windlord.remains&(talent.xuens_bond|!talent.xuens_bond&buff.invokers_delight."
      "up)|fight_remains<15|fight_style.dungeonroute&buff.invokers_delight.up&cooldown.strike_of_the_windlord.remains&"
      "buff.storm_earth_and_fire.remains<8|fight_remains<10" );
  default_st->add_action(
      "tiger_palm,if=chi<5&combo_strike&variable.small_hotjs_active&!"
      "buff.heart_of_the_jade_serpent_cdr_celestial.up&energy.time_to_max<=gcd.max*3&!buff.invokers_delight.up&!buff."
      "bloodlust.up&!buff.storm_earth_and_fire.up" );
  default_st->add_action(
      "tiger_palm,if=chi<2&combo_strike&cooldown.strike_of_the_windlord.remains<5&talent.celestial_conduit" );
  default_st->add_action( "tiger_palm,if=chi<2&combo_strike&talent.celestial_conduit" );
  default_st->add_action(
      "fists_of_fury,target_if=max:debuff.acclamation.stack,if=combo_strike&buff.heart_of_the_jade_serpent_cdr_"
      "celestial.up|variable.small_hotjs_active" );
  default_st->add_action(
      "spinning_crane_kick,if=buff.dance_of_chiji.stack=2&combo_strike&(!set_bonus.tww3_2pc|!buff.bloodlust.up)&!"
      "talent.flurry_strikes" );
  default_st->add_action(
      "tiger_palm,target_if=max:debuff.acclamation.stack,if=(energy>55&talent.inner_peace|energy>60&!talent."
      "inner_peace)&combo_strike&chi.max-chi>=2&buff.teachings_of_the_monastery.stack<buff.teachings_of_the_monastery."
      "max_stack&(talent.energy_burst&!buff.bok_proc.up|!talent.energy_burst)&!buff.ordered_elements.up|(talent.energy_"
      "burst&!buff.bok_proc.up|!talent.energy_burst)&!buff.ordered_elements.up&!cooldown.fists_of_fury.remains&chi<3|("
      "prev.strike_of_the_windlord|!buff.heart_of_the_jade_serpent_cdr_celestial.up)&combo_strike&chi.deficit>=2&!buff."
      "ordered_elements.up" );
  default_st->add_action( "touch_of_death,target_if=min:target.time_to_die" );
  default_st->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=combo_strike&(buff.invoke_xuen_the_white_tiger.down&prev."
      "tiger_palm&time<5|buff.storm_earth_and_"
      "fire.up&talent.ordered_elements)" );
  default_st->add_action(
      "strike_of_the_windlord,target_if=max:debuff.acclamation.stack,if=!buff.heart_of_the_jade_serpent_cdr_celestial."
      "up&talent.celestial_conduit&!buff."
      "invokers_delight.up&!buff.heart_of_the_jade_serpent_cdr_celestial.up&cooldown.fists_of_fury.remains<5&cooldown."
      "invoke_xuen_the_white_tiger.remains>15&(cooldown.slicing_winds.remains<23|!set_bonus.tww3_2pc)|fight_remains<"
      "12" );
  default_st->add_action(
      "strike_of_the_windlord,target_if=max:debuff.acclamation.stack,if=talent.gale_force&cooldown.invoke_xuen_the_"
      "white_tiger.remains>13|fight_remains<cooldown.invoke_xuen_the_white_tiger.remains" );
  default_st->add_action(
      "strike_of_the_windlord,target_if=max:debuff.acclamation.stack,if=time>5&talent.flurry_strikes" );
  default_st->add_action(
      "strike_of_the_windlord,target_if=max:debuff.acclamation.stack,if=time>15&set_bonus.tww3_2pc&cooldown.invoke_"
      "xuen_the_white_tiger.remains>15&(cooldown."
      "slicing_winds.remains<23|!set_bonus.tww3_2pc)" );
  default_st->add_action( "spinning_crane_kick,if=buff.dance_of_chiji.stack=2&combo_strike&!talent.flurry_strikes" );
  default_st->add_action(
      "blackout_kick,target_if=max:debuff.acclamation.stack,if=buff.teachings_of_the_monastery.stack>3&buff."
      "ordered_elements.up&cooldown.rising_sun_kick.remains>1&cooldown.fists_of_fury.remains>2&!set_bonus.tww3_2pc" );
  default_st->add_action(
      "blackout_kick,target_if=max:debuff.acclamation.stack,if=buff.teachings_of_the_monastery.stack>4&"
      "cooldown.rising_sun_kick.remains>1&cooldown.fists_of_fury.remains>2&!set_bonus.tww3_2pc" );
  default_st->add_action( "whirling_dragon_punch,target_if=max:debuff.acclamation.stack,if=set_bonus.tww3_2pc" );
  default_st->add_action(
      "whirling_dragon_punch,target_if=max:debuff.acclamation.stack,if=!buff.heart_of_the_jade_serpent_cdr_celestial."
      "up&!buff.dance_of_chiji.stack=2|buff."
      "ordered_elements.up|talent.knowledge_of_the_broken_temple" );
  default_st->add_action(
      "crackling_jade_lightning,target_if=max:debuff.acclamation.stack,if=buff.the_emperors_capacitor.stack>19&!"
      "variable.small_hotjs_active&!buff.heart_of_"
      "the_jade_serpent_cdr_celestial.up&combo_strike&(!fight_style.dungeonslice|target.time_to_die>20)&cooldown."
      "invoke_xuen_the_white_tiger.remains>15|buff.the_emperors_capacitor.stack>20&!variable.small_hotjs_active&!buff."
      "heart_of_the_jade_serpent_cdr_celestial.up&combo_strike&(!fight_style.dungeonslice|target.time_to_die>20)&"
      "cooldown.invoke_xuen_the_white_tiger.remains<20&cooldown.invoke_xuen_the_white_tiger.remains>2|buff.the_"
      "emperors_capacitor.stack>10&fight_remains<5|buff.storm_earth_and_fire.remains<2&buff.the_emperors_capacitor."
      "stack>15&buff.storm_earth_and_fire.up" );
  default_st->add_action( "slicing_winds,if=target.time_to_die>10&!set_bonus.tww3_4pc|talent.flurry_strikes" );
  default_st->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=combo_strike&(chi>4|chi>2&energy>50|cooldown.fists_of_fury."
      "remains>2)" );
  default_st->add_action(
      "fists_of_fury,target_if=max:debuff.acclamation.stack,if=(talent.xuens_battlegear|!talent.xuens_battlegear&("
      "cooldown.strike_of_the_windlord.remains>1|"
      "variable.small_hotjs_active|buff.heart_of_the_jade_serpent_cdr_celestial.up))&(talent.xuens_battlegear&cooldown."
      "invoke_xuen_the_white_tiger.remains>5|cooldown.invoke_xuen_the_white_tiger.remains>10)&(!buff.invokers_delight."
      "up|buff.invokers_delight.up&cooldown.strike_of_the_windlord.remains>4&cooldown.celestial_conduit.remains)|fight_"
      "remains<5|talent.flurry_strikes" );
  default_st->add_action(
      "tiger_palm,target_if=max:debuff.acclamation.stack,if=combo_strike&energy.time_to_max<=gcd.max*3&talent.flurry_"
      "strikes&buff.wisdom_of_the_wall_flurry."
      "up" );
  default_st->add_action(
      "tiger_palm,target_if=max:debuff.acclamation.stack,if=combo_strike&chi<5&set_bonus.tww3_2pc" );
  default_st->add_action( "spinning_crane_kick,if=buff.dance_of_chiji.stack=2&combo_strike" );
  default_st->add_action(
      "blackout_kick,target_if=max:debuff.acclamation.stack,if=combo_strike&talent.energy_burst&buff.bok_proc.up&chi<5&"
      "(variable.small_hotjs_active|buff."
      "heart_of_the_jade_serpent_cdr_celestial.up)" );
  default_st->add_action(
      "spinning_crane_kick,if=combo_strike&buff.bloodlust.up&variable.small_hotjs_active&buff.dance_of_chiji."
      "up" );
  default_st->add_action(
      "tiger_palm,target_if=max:debuff.acclamation.stack,if=combo_strike&chi.deficit>=2&energy.time_to_max<="
      "gcd.max*3" );
  default_st->add_action(
      "blackout_kick,target_if=max:debuff.acclamation.stack,if=buff.teachings_of_the_monastery.stack>7&talent."
      "memory_of_the_monastery&!buff.memory_of_the_monastery.up&cooldown.fists_of_fury.remains" );
  default_st->add_action(
      "spinning_crane_kick,if=(buff.dance_of_chiji.stack=2|buff.dance_of_chiji.remains<2&buff.dance_of_chiji.up)&combo_"
      "strike&!buff.ordered_elements.up" );
  default_st->add_action( "whirling_dragon_punch,target_if=max:debuff.acclamation.stack" );
  default_st->add_action( "spinning_crane_kick,if=buff.dance_of_chiji.stack=2&combo_strike" );
  default_st->add_action(
      "blackout_kick,target_if=max:debuff.acclamation.stack,if=talent.courageous_impulse&combo_strike&buff.bok_"
      "proc.stack=2" );
  default_st->add_action( "spinning_crane_kick,if=buff.dance_of_chiji.up&set_bonus.tww3_2pc&combo_strike" );
  default_st->add_action(
      "blackout_kick,target_if=max:debuff.acclamation.stack,if=combo_strike&buff.ordered_elements.up&cooldown."
      "rising_sun_kick.remains>1&cooldown.fists_of_fury.remains>2" );
  default_st->add_action(
      "tiger_palm,target_if=max:debuff.acclamation.stack,if=combo_strike&energy.time_to_max<=gcd.max*3&talent."
      "flurry_strikes" );
  default_st->add_action(
      "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up&(buff.ordered_elements.up|energy.time_to_max>=gcd."
      "max*3&talent.sequenced_strikes&talent.energy_burst|!talent.sequenced_strikes|!talent.energy_burst|buff.dance_of_"
      "chiji.remains<=gcd.max*3)" );
  default_st->add_action(
      "tiger_palm,target_if=max:debuff.acclamation.stack,if=combo_strike&energy.time_to_max<=gcd.max*3&talent."
      "flurry_strikes" );
  default_st->add_action( "jadefire_stomp,if=talent.Singularly_Focused_Jade|talent.jadefire_harmony" );
  default_st->add_action( "chi_burst,if=!buff.ordered_elements.up" );
  default_st->add_action(
      "blackout_kick,target_if=max:debuff.acclamation.stack,if=combo_strike&(buff.ordered_elements.up|buff.bok_"
      "proc.up&chi.deficit>=1&talent.energy_burst)&cooldown.fists_of_fury.remains" );
  default_st->add_action(
      "blackout_kick,target_if=max:debuff.acclamation.stack,if=combo_strike&cooldown.fists_of_fury.remains&("
      "chi>2|energy>60|buff.bok_proc.up)" );
  default_st->add_action( "jadefire_stomp" );
  default_st->add_action(
      "tiger_palm,target_if=max:debuff.acclamation.stack,if=combo_strike&buff.ordered_elements.up&chi.deficit>="
      "1" );
  default_st->add_action( "chi_burst" );
  default_st->add_action( "spinning_crane_kick,if=combo_strike&buff.ordered_elements.up&talent.hit_combo" );
  default_st->add_action(
      "blackout_kick,if=buff.ordered_elements.up&!talent.hit_combo&"
      "cooldown.fists_of_fury.remains" );
  default_st->add_action( "tiger_palm,if=prev.tiger_palm&chi<3&!cooldown.fists_of_fury.remains" );

  // fallback
  fallback->add_action( "spinning_crane_kick,if=chi>5&combo_strike", "Fallback" );
  fallback->add_action( "blackout_kick,if=combo_strike&chi>3" );
  fallback->add_action( "tiger_palm,if=combo_strike&chi>5" );
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
    case MONK_MISTWEAVER:
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
      cooldowns->add_action( "weapons_of_order" );
      cooldowns->add_action( "invoke_niuzao_the_black_ox" );
      break;
    case MONK_MISTWEAVER:
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
  base_t::parse_assisted_combat_step( step, assisted_combat );
}

parsed_assisted_combat_rule_t monk_t::parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule,
                                                                  const assisted_combat_step_data_t& step ) const
{
  if ( step.spell_id == 152175 && rule.condition_type == TARGET_DISTANCE_LESS )
  {
    assisted_combat_rule_data_t rule_copy = rule;
    rule_copy.condition_value_1           = 10;

    return { base_t::parse_assisted_combat_rule( rule_copy, step ), "Extended range check to 10 yards (from 5)." };
  }

  if ( is_ptr() && step.spell_id == 205523 && rule.condition_type == AURA_ON_PLAYER &&
       rule.condition_value_1 == 196736 )
    return { "", "Remove talent.blackout_combo check." };

  if ( rule.condition_type == AURA_MISSING_PLAYER )
  {
    switch ( rule.condition_value_1 )
    {
      case 1245502:
      case 1245503:
      case 1245504:
      case 1245506:
        return { "combo_strike",
                 fmt::format( "Spell id {} is a helper buff to avoid breaking combo for {}.", rule.condition_value_1,
                              find_spell( step.spell_id )->name_cstr() ),
                 true };
    }
  }

  if ( rule.condition_type == TARGET_COUNT_NEAR_PLAYER_GREATER && rule.condition_value_1 == 1 &&
       rule.condition_value_2 == 15 )
    return { "1", "Counts valid targets for action, including player." };

  return base_t::parse_assisted_combat_rule( rule, step );
}

std::string monk_t::aura_expr_from_spell_id( unsigned int spell_id, bool on_self ) const
{
  if ( on_self )
  {
    switch ( spell_id )
    {
      case 261916:  // Blackout Kick! proc
        return "buff.bok_proc";
      case 443421:  // Heart of the Jade Serpent Strike of the Windlord Cooldown Reduction
        return "buff.heart_of_the_jade_serpent_cdr";
      case 443616:  // Heart of the Jade Serpent Conduit of the Celestial Cooldown Reduction
        return "buff.heart_of_the_jade_serpent_cdr_celestial";
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
    case MONK_MISTWEAVER:
      return mistweaver::default_potion( this );
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
    case MONK_MISTWEAVER:
      return mistweaver::default_flask( this );
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
    case MONK_MISTWEAVER:
      return mistweaver::default_food( this );
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
    case MONK_MISTWEAVER:
      return mistweaver::default_rune( this );
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
    case MONK_MISTWEAVER:
      return mistweaver::default_temporary_enchant( this );
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
      case MONK_MISTWEAVER:
        mistweaver::default_apl( this );
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
