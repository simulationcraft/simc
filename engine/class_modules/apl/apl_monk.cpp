#include "class_modules/apl/apl_monk.hpp"

#include "class_modules/monk/sc_monk.hpp"
#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace monk_apl
{
std::string potion( const player_t *p )
{
  switch ( p->specialization() )
  {
    case MONK_BREWMASTER:
      if ( p->true_level > 70 )
        return "tempered_potion_3";
      else if ( p->true_level > 60 )
        return "elemental_potion_of_ultimate_power_3";
      else if ( p->true_level > 50 )
        return "phantom_fire";
      else if ( p->true_level > 45 )
        return "unbridled_fury";
      else
        return "disabled";
      break;
    case MONK_MISTWEAVER:
      if ( p->true_level > 70 )
        return "tempered_potion_3";
      else if ( p->true_level > 60 )
        return "elemental_potion_of_ultimate_power_3";
      else if ( p->true_level > 50 )
        return "potion_of_spectral_intellect";
      else if ( p->true_level > 45 )
        return "superior_battle_potion_of_intellect";
      else
        return "disabled";
      break;
    case MONK_WINDWALKER:
      if ( p->true_level > 70 )
        return "tempered_potion_3";
      else if ( p->true_level > 60 )
        return "elemental_potion_of_ultimate_power_3";
      else if ( p->true_level > 50 )
        return "potion_of_spectral_agility";
      else if ( p->true_level > 45 )
        return "unbridled_fury";
      else
        return "disabled";
      break;
    default:
      return "disabled";
      break;
  }
}

std::string flask( const player_t *p )
{
  switch ( p->specialization() )
  {
    case MONK_BREWMASTER:
      if ( p->true_level > 70 )
        return "flask_of_alchemical_chaos_3";
      else if ( p->true_level > 60 )
        return "iced_phial_of_corrupting_rage_3";
      else if ( p->true_level > 50 )
        return "spectral_flask_of_power";
      else if ( p->true_level > 45 )
        return "currents";
      else
        return "disabled";
      break;
    case MONK_MISTWEAVER:
      if ( p->true_level > 70 )
        return "flask_of_alchemical_chaos_3";
      else if ( p->true_level > 60 )
        return "phial_of_elemental_chaos_3";
      else if ( p->true_level > 50 )
        return "spectral_flask_of_power";
      else if ( p->true_level > 45 )
        return "greater_flask_of_endless_fathoms";
      else
        return "disabled";
      break;
    case MONK_WINDWALKER:
      if ( p->true_level > 70 )
        return "flask_of_alchemical_chaos_3";
      else if ( p->true_level > 60 )
        return "iced_phial_of_corrupting_rage_3";
      else if ( p->true_level > 50 )
        return "spectral_flask_of_power";
      else if ( p->true_level > 45 )
        return "greater_flask_of_the_currents";
      else
        return "disabled";
      break;
    default:
      return "disabled";
      break;
  }
}

std::string food( const player_t *p )
{
  switch ( p->specialization() )
  {
    case MONK_BREWMASTER:
      if ( p->true_level > 70 )
        return "feast_of_the_midnight_masquerade";
      else if ( p->true_level > 60 )
        return "fated_fortune_cookie";
      else if ( p->true_level > 50 )
        return "spinefin_souffle_and_fries";
      else if ( p->true_level > 45 )
        return "biltong";
      else
        return "disabled";
      break;
    case MONK_MISTWEAVER:
      if ( p->true_level > 70 )
        return "feast_of_the_midnight_masquerade";
      else if ( p->true_level > 60 )
        return "roast_duck_delight";
      else if ( p->true_level > 50 )
        return "feast_of_gluttonous_hedonism";
      else if ( p->true_level > 45 )
        return "famine_evaluator_and_snack_table";
      else
        return "disabled";
      break;
    case MONK_WINDWALKER:
      if ( p->true_level > 70 )
        return "feast_of_the_midnight_masquerade";
      else if ( p->true_level > 60 )
        return "aromatic_seafood_platter";
      else if ( p->true_level > 50 )
        return "feast_of_gluttonous_hedonism";
      else if ( p->true_level > 45 )
        return "mechdowels_big_mech";
      else
        return "disabled";
      break;
    default:
      return "disabled";
      break;
  }
}

std::string rune( const player_t *p )
{
  if ( p->true_level > 70 )
    return "crystallized";
  else if ( p->true_level > 60 )
    return "draconic";
  else if ( p->true_level > 50 )
    return "veiled";
  else if ( p->true_level > 45 )
    return "battle_scarred";
  else if ( p->true_level > 40 )
    return "defiled";
  return "disabled";
}

std::string temporary_enchant( const player_t *p )
{
  switch ( p->specialization() )
  {
    case MONK_BREWMASTER:
      if ( p->true_level > 70 )
        return "main_hand:ironclaw_whetstone_3/off_hand:ironclaw_whetstone_3";
      else if ( p->true_level > 60 )
        return "main_hand:buzzing_rune_3/off_hand:buzzing_rune_3";
      else if ( p->true_level > 50 )
        return "main_hand:shadowcore_oil/off_hand:shadowcore_oil";
      else
        return "disabled";
      break;
    case MONK_MISTWEAVER:
      if ( p->true_level > 70 )
        return "main_hand:howling_rune_3";
      else if ( p->true_level > 60 )
        return "main_hand:howling_rune_3";
      else if ( p->true_level > 50 )
        return "main_hand:shadowcore_oil";
      else
        return "disabled";
      break;
    case MONK_WINDWALKER:
      if ( p->true_level > 70 )
        return "main_hand:ironclaw_whetstone_3/off_hand:ironclaw_whetstone_3";
      else if ( p->true_level > 60 )
        return "main_hand:howling_rune_3/off_hand:howling_rune_3";
      else if ( p->true_level > 50 )
        return "main_hand:shaded_weightstone/off_hand:shaded_weightstone";
      else
        return "disabled";
      break;
    default:
      return "disabled";
      break;
  }
}

void brewmaster( player_t *p )
{
  std::vector<std::string> racial_actions = p->get_racial_actions();
  action_priority_list_t *precombat       = p->get_action_priority_list( "precombat" );
  action_priority_list_t *default_        = p->get_action_priority_list( "default" );

  action_priority_list_t *item_actions = p->get_action_priority_list( "item_actions" );
  action_priority_list_t *race_actions = p->get_action_priority_list( "race_actions" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "potion" );
  precombat->add_action( "chi_burst" );

  race_actions->add_action( "blood_fury" );
  race_actions->add_action( "berserking" );
  race_actions->add_action( "arcane_torrent" );
  race_actions->add_action( "lights_judgment" );
  race_actions->add_action( "fireblood" );
  race_actions->add_action( "ancestral_call" );
  race_actions->add_action( "bag_of_tricks" );

  item_actions->add_action( "use_item,slot=trinket1" );
  item_actions->add_action( "use_item,slot=trinket2" );

  default_->add_action( "auto_attack" );
  default_->add_action( "potion" );
  default_->add_action( "call_action_list,name=race_actions" );
  default_->add_action( "call_action_list,name=item_actions" );
  default_->add_action( "black_ox_brew,if=energy<40" );
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
  default_->add_action( "blackout_kick" );
  default_->add_action( "chi_burst" );
  default_->add_action( "weapons_of_order" );
  default_->add_action( "rising_sun_kick,if=!talent.fluidity_of_motion.enabled" );
  default_->add_action( "tiger_palm,if=buff.blackout_combo.up" );
  default_->add_action( "keg_smash,if=talent.scalding_brew.enabled" );
  default_->add_action(
      "spinning_crane_kick,if=talent.charred_passions.enabled&talent.scalding_brew.enabled&buff.charred_passions.up&"
      "buff.charred_passions.remains<3&dot.breath_of_fire.remains<9&active_enemies>4" );
  default_->add_action( "rising_sun_kick,if=talent.fluidity_of_motion.enabled" );
  default_->add_action( "purifying_brew,if=buff.blackout_combo.down" );
  default_->add_action(
      "breath_of_fire,if=(buff.charred_passions.down&(!talent.scalding_brew.enabled|active_enemies<5))|!talent.charred_"
      "passions.enabled|(dot.breath_of_fire.remains<3&talent.scalding_brew.enabled)" );
  default_->add_action( "exploding_keg" );
  default_->add_action( "keg_smash" );
  default_->add_action( "rushing_jade_wind" );
  default_->add_action( "invoke_niuzao" );
  default_->add_action( "tiger_palm,if=energy>40-cooldown.keg_smash.remains*energy.regen" );
  default_->add_action( "spinning_crane_kick,if=energy>40-cooldown.keg_smash.remains*energy.regen" );
}

void mistweaver( player_t *p )
{
  action_priority_list_t *pre = p->get_action_priority_list( "precombat" );

  // Snapshot stats
  pre->add_action( "snapshot_stats" );

  pre->add_action( "potion" );

  pre->add_action( p, "chi_burst" );
  pre->add_action( p, "chi_wave" );

  std::vector<std::string> racial_actions = p->get_racial_actions();
  action_priority_list_t *def             = p->get_action_priority_list( "default" );
  action_priority_list_t *st              = p->get_action_priority_list( "st" );
  action_priority_list_t *aoe             = p->get_action_priority_list( "aoe" );

  def->add_action( "auto_attack" );
  def->add_action( "roll,if=movement.distance>5", "Move to target" );
  def->add_action( "chi_torpedo,if=movement.distance>5" );
  def->add_action( "spear_hand_strike,if=target.debuff.casting.react" );

  if ( p->items[ SLOT_MAIN_HAND ].name_str == "jotungeirr_destinys_call" )
    def->add_action( "use_item,name=" + p->items[ SLOT_MAIN_HAND ].name_str );

  for ( const auto &item : p->items )
  {
    std::string name_str;
    if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
    {
      if ( item.name_str == "scars_of_fraternal_strife" )
        def->add_action( "use_item,name=" + item.name_str + ",if=!buff.scars_of_fraternal_strife_4.up&time>1" );
      else if ( item.name_str == "jotungeirr_destinys_call" )
        continue;
      else
        def->add_action( "use_item,name=" + item.name_str );
    }
  }

  for ( const auto &racial_action : racial_actions )
  {
    def->add_action( racial_action + ",if=target.time_to_die<18" );
  }

  def->add_action( "potion" );

  def->add_action( "jadefire_stomp" );
  def->add_action( "bonedust_brew" );

  def->add_action( "call_action_list,name=aoe,if=active_enemies>=3" );
  def->add_action( "call_action_list,name=st,if=active_enemies<3" );

  st->add_action( p, "Thunder Focus Tea" );
  st->add_action( p, "Rising Sun Kick" );
  st->add_action( p, "Blackout Kick",
                  "if=buff.teachings_of_the_monastery.stack=1&cooldown.rising_sun_kick.remains<12" );
  st->add_action( p, "chi_wave" );
  st->add_action( p, "chi_burst" );
  st->add_action( p, "Tiger Palm",
                  "if=buff.teachings_of_the_monastery.stack<3|buff.teachings_of_the_monastery.remains<2" );

  aoe->add_action( p, "Spinning Crane Kick" );
  aoe->add_action( p, "chi_wave" );
  aoe->add_action( p, "chi_burst" );
}

void windwalker( player_t *p )
{
  auto monk = debug_cast<monk::monk_t *>( p );

  //============================================================================
  // On-use Items
  //============================================================================
  auto _WW_ON_USE = []( const item_t &item ) {
    //-------------------------------------------
    // SEF item map
    //-------------------------------------------
    const static std::unordered_map<std::string, std::string> sef_trinkets{
        // name_str -> APL
        // DF Trinkets
        { "algethar_puzzle_box",
          ",if=(pet.xuen_the_white_tiger.active|!talent.invoke_xuen_the_white_tiger)&!buff.storm_earth_"
          "and_fire.up|fight_remains<25" },
        { "erupting_spear_fragment", ",if=buff.storm_earth_and_fire.up" },
        { "manic_grieftorch",
          ",if=!trinket.1.has_use_buff&!trinket.2.has_use_buff&!buff.storm_earth_and_fire.up&!pet.xuen_"
          "the_white_tiger.active|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger."
          "remains>30|fight_remains<5" },
        { "beacon_to_the_beyond",
          ",if=!trinket.1.has_use_buff&!trinket.2.has_use_buff&!buff.storm_earth_and_fire.up&!pet.xuen_"
          "the_white_tiger.active|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger."
          "remains>30|fight_remains<10" },
        { "djaruun_pillar_of_the_elder_flame",
          ",if=cooldown.fists_of_fury.remains<2&cooldown.invoke_xuen_the_white_tiger.remains>10|fight_remains<12" },
        { "dragonfire_bomb_dispenser",
          ",if=!trinket.1.has_use_buff&!trinket.2.has_use_buff|(trinket.1.has_use_buff|trinket.2.has_use_buff)&"
          "cooldown.invoke_xuen_the_white_tiger.remains>10|fight_remains<10" },
        // TWW Trinkets
        { "imperfect_ascendancy_serum", ",use_off_gcd=1,if=pet.xuen_the_white_tiger.active" },
        { "mad_queens_mandate",
          ",target_if=min:time_to_die,if=!trinket.1.has_use_buff&!trinket.2.has_use_buff|(trinket.1.has_use_buff|"
          "trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>30" },
        { "treacherous_transmitter",
          ",if=!fight_style.dungeonslice&(cooldown.invoke_xuen_the_white_tiger.remains<4|talent.xuens_bond&pet.xuen_"
          "the_white_tiger.active)|fight_style.dungeonslice&((fight_style.DungeonSlice&active_enemies=1&(time<10|"
          "talent.xuens_bond&talent.celestial_conduit)|!fight_style.dungeonslice|active_enemies>1)&cooldown.storm_"
          "earth_and_fire.ready&(target.time_to_die>14&!fight_style.dungeonroute|target.time_to_die>22)&(active_"
          "enemies>2|debuff.acclamation.up|!talent.ordered_elements&time<5)&(chi>2&talent.ordered_elements|chi>5|chi>3&"
          "energy<50|energy<50&active_enemies=1|prev.tiger_palm&!talent.ordered_elements&time<5)|fight_remains<30)|"
          "buff.invokers_delight.up" },

        // Defaults:
        { "ITEM_STAT_BUFF", ",if=pet.xuen_the_white_tiger.active" },
        { "ITEM_DMG_BUFF",
          ",if=!trinket.1.has_use_buff&!trinket.2.has_use_buff|(trinket.1.has_use_buff|trinket.2.has_use_buff)&"
          "cooldown.invoke_xuen_the_white_tiger.remains>30" },
    };

    // -----------------------------------------

    std::string concat = "";
    auto talent_map    = sef_trinkets;
    try
    {
      concat = talent_map.at( item.name_str );
    }
    catch ( ... )
    {
      int duration = 0;

      for ( auto e : item.parsed.special_effects )
      {
        duration = (int)floor( e->duration().total_seconds() );

        // Ignore items that have a 30 second or shorter cooldown (or no cooldown)
        // Unless defined in the map above these will be used on cooldown.
        if ( e->type == SPECIAL_EFFECT_USE && e->cooldown() > timespan_t::from_seconds( 30 ) )
        {
          if ( e->is_stat_buff() || e->buff_type() == SPECIAL_EFFECT_BUFF_STAT )
          {
            // This item grants a stat buff on use
            concat = talent_map.at( "ITEM_STAT_BUFF" );

            break;
          }
          else
            // This item has a generic damage effect
            concat = talent_map.at( "ITEM_DMG_BUFF" );
        }
      }

      if ( concat.length() > 0 && duration > 0 )
        concat = concat + "|fight_remains<" + std::to_string( duration );
    }

    return concat;
  };

  //============================================================================

  action_priority_list_t *pre = p->get_action_priority_list( "precombat" );

  // Snapshot stats
  pre->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Add Precombattrinkets
  pre->add_action( "use_item,name=imperfect_ascendancy_serum" );

  std::vector<std::string> racial_actions = p->get_racial_actions();
  action_priority_list_t *def             = p->get_action_priority_list( "default" );
  action_priority_list_t *trinkets        = p->get_action_priority_list( "trinkets" );
  action_priority_list_t *aoe_opener      = p->get_action_priority_list( "aoe_opener" );
  action_priority_list_t *normal_opener   = p->get_action_priority_list( "normal_opener" );
  action_priority_list_t *cooldowns       = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t *default_aoe     = p->get_action_priority_list( "default_aoe" );
  action_priority_list_t *default_cleave  = p->get_action_priority_list( "default_cleave" );
  action_priority_list_t *default_st      = p->get_action_priority_list( "default_st" );

  def->add_action( "auto_attack" );
  def->add_action( "roll,if=movement.distance>5", "Move to target" );
  def->add_action( "chi_torpedo,if=movement.distance>5" );
  def->add_action( "flying_serpent_kick,if=movement.distance>5" );
  def->add_action( "spear_hand_strike,if=target.debuff.casting.react" );

  // Potion
  if ( p->sim->allow_potions )
  {
    if ( monk->talent.windwalker.invoke_xuen_the_white_tiger->ok() )
      def->add_action( "potion,if=buff.storm_earth_and_fire.up&pet.xuen_the_white_tiger.active|fight_remains<=30",
                       "Potion" );
    else
      def->add_action( "potion,if=buff.storm_earth_and_fire.up|fight_remains<=30", "Potion" );
  }

  // Enable PI if available
  def->add_action( "variable,name=has_external_pi,value=cooldown.invoke_power_infusion_0.duration>0",
                   "Enable PI if available" );

  // Use Trinkets
  def->add_action( "call_action_list,name=trinkets", "Use Trinkets" );

  // Openers
  def->add_action( "call_action_list,name=aoe_opener,if=time<3&active_enemies>2", "Openers" );
  def->add_action( "call_action_list,name=normal_opener,if=time<4&active_enemies<3" );

  // Use Cooldowns
  def->add_action( "call_action_list,name=cooldowns,if=talent.storm_earth_and_fire", "Use Cooldowns" );

  // Default priority
  def->add_action( "call_action_list,name=default_aoe,if=active_enemies>=5", "Default Priority" );
  def->add_action(
      "call_action_list,name=default_cleave,if=active_enemies>1&(time>7|!talent.celestial_conduit)&active_enemies<5" );
  def->add_action( "call_action_list,name=default_st,if=active_enemies<2" );

  // Trinkets
  for ( const auto &item : p->items )
  {
    if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
      trinkets->add_action( "use_item,name=" + item.name_str + _WW_ON_USE( item ) );
  }

  trinkets->add_action( "do_treacherous_transmitter_task,if=pet.xuen_the_white_tiger.active|fight_remains<20" );

  // Cooldowns
  cooldowns->add_action(
      "invoke_external_buff,name=power_infusion,if=pet.xuen_the_white_tiger.active&(!buff.bloodlust.up|buff.bloodlust."
      "up&cooldown.strike_of_the_windlord.remains)",
      "Use <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> while <a "
      "href='https://www.wowhead.com/spell=123904/invoke-xuen-the-white-tiger'>Invoke Xuen, the White Tiger</a> is "
      "active." );
  cooldowns->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=(target.time_to_die>14&!fight_style.dungeonroute|"
      "target.time_to_die>22)&!cooldown.invoke_xuen_the_white_tiger.remains&(chi<5&!talent.ordered_elements|chi<3)&("
      "combo_strike|!talent.hit_combo)" );
  cooldowns->add_action(
      "invoke_xuen_the_white_tiger,target_if=max:target.time_to_die,if=(fight_style.DungeonSlice&active_enemies=1&("
      "time<10|talent.xuens_bond&talent.celestial_conduit)|!fight_style.dungeonslice|active_enemies>1)&cooldown.storm_"
      "earth_and_fire.ready&(target.time_to_die>14&!fight_style.dungeonroute|target.time_to_die>22)&(active_enemies>2|"
      "debuff.acclamation.up|!talent.ordered_elements&time<5)&(chi>2&talent.ordered_elements|chi>5|chi>3&energy<50|"
      "energy<50&active_enemies=1|prev.tiger_palm&!talent.ordered_elements&time<5)|fight_remains<30" );
  cooldowns->add_action(
      "storm_earth_and_fire,target_if=max:target.time_to_die,if=(target.time_to_die>14&!fight_style.dungeonroute|"
      "target.time_to_die>22)&(active_enemies>2|cooldown.rising_sun_kick.remains|!talent.ordered_elements)&((buff."
      "invokers_delight.remains>10&!buff.bloodlust.up|buff.bloodlust.up&cooldown.storm_earth_and_fire.full_recharge_"
      "time<1)|cooldown.storm_earth_and_fire.full_recharge_time<cooldown.invoke_xuen_the_white_tiger.remains&!buff."
      "bloodlust.up&(active_enemies>1|cooldown.strike_of_the_windlord.remains<2&(talent.flurry_strikes|buff.heart_of_"
      "the_jade_serpent.up))&(chi>3|chi>1&talent.ordered_elements)|cooldown.storm_earth_and_fire.full_recharge_time<10&"
      "(chi>3|chi>1&talent.ordered_elements))|fight_remains<30|prev.invoke_xuen_the_white_tiger|buff.invokers_delight."
      "remains>10&fight_style.dungeonslice&(cooldown.rising_sun_kick.remains|!talent.ordered_elements|active_enemies>"
      "2)" );
  cooldowns->add_action( "touch_of_karma" );

  // Racials
  for ( const auto &racial_action : racial_actions )
  {
    if ( racial_action != "arcane_torrent" )
    {
      if ( racial_action == "ancestral_call" )
        cooldowns->add_action( racial_action + ",if=cooldown.invoke_xuen_the_white_tiger.remains>30|fight_remains<20" );
      else if ( racial_action == "blood_fury" )
        cooldowns->add_action( racial_action + ",if=cooldown.invoke_xuen_the_white_tiger.remains>30|fight_remains<20" );
      else if ( racial_action == "fireblood" )
        cooldowns->add_action( racial_action + ",if=cooldown.invoke_xuen_the_white_tiger.remains>30|fight_remains<10" );
      else if ( racial_action == "berserking" )
        cooldowns->add_action( racial_action + ",if=cooldown.invoke_xuen_the_white_tiger.remains>60|fight_remains<15" );
      else if ( racial_action == "bag_of_tricks" )
        def->add_action( racial_action + ",if=buff.storm_earth_and_fire.down" );
      else if ( racial_action == "lights_judgment" )
        def->add_action( racial_action + ",if=buff.storm_earth_and_fire.down" );
      else if ( racial_action == "haymaker" )
        def->add_action( racial_action + ",if=buff.storm_earth_and_fire.down" );
      else if ( racial_action == "rocket_barrage" )
        def->add_action( racial_action + ",if=buff.storm_earth_and_fire.down" );
      else if ( racial_action == "azerite_surge" )
        def->add_action( racial_action + ",if=buff.storm_earth_and_fire.down" );
      else if ( racial_action == "arcane_pulse" )
        def->add_action( racial_action + ",if=buff.storm_earth_and_fire.down" );
      else
        def->add_action( racial_action );
    }
  }

  // AoE Opener
  aoe_opener->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=chi<6", "aoe opener" );

  // Normal Opener
  normal_opener->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=chi<6&combo_strike",
                             "normal opener" );
  normal_opener->add_action( "rising_sun_kick,target_if=max:debuff.acclamation.stack,if=talent.ordered_elements" );

  // >=5 Target priority
  default_aoe->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=(energy>55&talent.inner_peace|energy>60&!talent."
      "inner_peace)&combo_strike&chi.max-chi>=2&buff.teachings_of_the_monastery.stack<buff.teachings_of_the_monastery."
      "max_stack&(talent.energy_burst&!buff.bok_proc.up)&!buff.ordered_elements.up|(talent.energy_burst&!buff.bok_proc."
      "up)&!buff.ordered_elements.up&!cooldown.fists_of_fury.remains&chi<3|(prev.strike_of_the_windlord|cooldown."
      "strike_of_the_windlord.remains)&cooldown.celestial_conduit.remains<2&buff.ordered_elements.up&chi<5&combo_"
      "strike",
      ">=5 Targets" );
  default_aoe->add_action( "touch_of_death" );
  default_aoe->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=buff.dance_of_chiji.stack=2&combo_strike" );
  default_aoe->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.chi_energy.stack>29&cooldown.fists_of_"
      "fury.remains<5" );
  default_aoe->add_action(
      "celestial_conduit,target_if=max:debuff.acclamation.stack,if=buff.storm_earth_and_fire.up&cooldown.strike_of_the_"
      "windlord.remains&(talent.xuens_bond|!talent.xuens_bond&buff.invokers_delight.up)|fight_remains<15" );
  default_aoe->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=!talent.xuens_battlegear&!cooldown.whirling_dragon_punch."
      "remains&cooldown.fists_of_fury.remains>1&(!talent.revolving_whirl|talent.revolving_whirl&buff.dance_of_chiji."
      "stack<2&active_enemies>2)|!buff.storm_earth_and_fire.up&buff.pressure_point.up" );
  default_aoe->add_action(
      "whirling_dragon_punch,target_if=max:target.time_to_die,if=!talent.revolving_whirl|talent.revolving_whirl&buff."
      "dance_of_chiji.stack<2&active_enemies>2" );
  default_aoe->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&buff.bok_proc.up&chi<2&talent."
      "energy_burst&energy<55" );
  default_aoe->add_action(
      "strike_of_the_windlord,target_if=max:target.time_to_die,if=time>5&(cooldown.invoke_xuen_the_white_tiger.remains>"
      "15|talent.flurry_strikes)" );
  default_aoe->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=8&talent."
      "shadowboxing_treads" );
  default_aoe->add_action(
      "crackling_jade_lightning,target_if=max:target.time_to_die,if=buff.the_emperors_capacitor.stack>19&combo_strike&"
      "talent.power_of_the_thunder_king" );
  default_aoe->add_action( "fists_of_fury,target_if=max:target.time_to_die" );
  default_aoe->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&energy.time_to_max<=gcd.max*3&talent."
      "flurry_strikes&buff.wisdom_of_the_wall_flurry.up&chi<6" );
  default_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&chi>5" );
  default_aoe->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.dance_of_chiji.up&buff.chi_energy."
      "stack>29&cooldown.fists_of_fury.remains<5" );
  default_aoe->add_action(
      "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.pressure_point.up&cooldown.fists_of_fury."
      "remains>2" );
  default_aoe->add_action("blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=talent.shadowboxing_treads&talent.courageous_impulse&combo_strike&buff.bok_proc.stack=2" );
  default_aoe->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.dance_of_chiji.up&spinning_crane_kick."
      "max" );
  default_aoe->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.ordered_elements.up&talent.crane_"
      "vortex&active_enemies>2&spinning_crane_kick.max" );
  default_aoe->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&energy.time_to_max<=gcd.max*3&talent."
      "flurry_strikes&buff.ordered_elements.up" );
  default_aoe->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&chi.deficit>=2&(!buff.ordered_"
      "elements.up|energy.time_to_max<=gcd.max*3)" );
  default_aoe->add_action(
      "jadefire_stomp,target_if=max:target.time_to_die,if=talent.Singularly_Focused_Jade|talent.jadefire_harmony" );
  default_aoe->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&!buff.ordered_elements.up&talent.crane_"
      "vortex&active_enemies>2&chi>4&spinning_crane_kick.max" );
  default_aoe->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&cooldown.fists_of_fury.remains&("
      "buff.teachings_of_the_monastery.stack>3|buff.ordered_elements.up)&(talent.shadowboxing_treads|buff.bok_proc."
      "up)" );
  default_aoe->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&!cooldown.fists_of_fury.remains&"
      "chi<3" );
  default_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=talent.shadowboxing_treads&talent.courageous_impulse&combo_strike&buff.bok_proc.up" );
  default_aoe->add_action( "spinning_crane_kick,if=combo_strike&(chi>3|energy>55)" );
  default_aoe->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&(buff.ordered_elements.up|buff.bok_"
      "proc.up&chi.deficit>=1&talent.energy_burst)&cooldown.fists_of_fury.remains" );
  default_aoe->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&cooldown.fists_of_fury.remains&("
      "chi>2|energy>60|buff.bok_proc.up)" );
  default_aoe->add_action( "jadefire_stomp,target_if=max:debuff.acclamation.stack" );
  default_aoe->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&buff.ordered_elements.up&chi.deficit>="
      "1" );
  default_aoe->add_action( "chi_burst,if=!buff.ordered_elements.up" );
  default_aoe->add_action( "chi_burst" );
  default_aoe->add_action(
      "spinning_crane_kick,if=combo_strike&buff.ordered_elements.up&talent.hit_combo&spinning_crane_kick.max" );
  default_aoe->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.ordered_elements.up&!talent.hit_combo&"
      "cooldown.fists_of_fury.remains" );
  default_aoe->add_action( "tiger_palm,if=prev.tiger_palm&chi<3&!cooldown.fists_of_fury.remains" );

  // 2-4 targets
  default_cleave->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=buff.pressure_point.up&active_enemies<4&cooldown.fists_of_"
      "fury.remains>4",
      "2-4 targets" );
  default_cleave->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.dance_of_chiji.stack=2&active_enemies>"
      "3" );
  default_cleave->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=(energy>55&talent.inner_peace|energy>60&!talent."
      "inner_peace)&combo_strike&chi.max-chi>=2&buff.teachings_of_the_monastery.stack<buff.teachings_of_the_monastery."
      "max_stack&(talent.energy_burst&!buff.bok_proc.up|!talent.energy_burst)&!buff.ordered_elements.up|(talent.energy_"
      "burst&!buff.bok_proc.up|!talent.energy_burst)&!buff.ordered_elements.up&!cooldown.fists_of_fury.remains&chi<3|("
      "prev.strike_of_the_windlord|cooldown.strike_of_the_windlord.remains)&cooldown.celestial_conduit.remains<2&buff."
      "ordered_elements.up&chi<5&combo_strike|(!buff.heart_of_the_jade_serpent_cdr.up|!buff.heart_of_the_jade_serpent_"
      "cdr_celestial.up)&combo_strike&chi.deficit>=2&!buff.ordered_elements.up" );
  default_cleave->add_action( "touch_of_death" );
  default_cleave->add_action(
      "celestial_conduit,target_if=max:debuff.acclamation.stack,if=buff.storm_earth_and_fire.up&cooldown.strike_of_the_"
      "windlord.remains&(talent.xuens_bond|!talent.xuens_bond&buff.invokers_delight.up)|fight_remains<15" );
  default_cleave->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=!pet.xuen_the_white_tiger.active&prev.tiger_palm&time<5|"
      "buff.heart_of_the_jade_serpent_cdr_celestial.up&buff.pressure_point.up" );
  default_cleave->add_action(
      "fists_of_fury,target_if=max:target.time_to_die,if=buff.heart_of_the_jade_serpent_cdr_celestial.up" );
  default_cleave->add_action(
      "whirling_dragon_punch,target_if=max:target.time_to_die,if=buff.heart_of_the_jade_serpent_cdr_celestial.up" );
  default_cleave->add_action(
      "strike_of_the_windlord,target_if=max:target.time_to_die,if=talent.gale_force&buff.invokers_delight.up&(buff."
      "bloodlust.up|cooldown.celestial_conduit.remains&!buff.heart_of_the_jade_serpent_cdr_celestial.up)" );
  default_cleave->add_action(
      "fists_of_fury,target_if=max:target.time_to_die,if=buff.power_infusion.up&buff.bloodlust.up" );
  default_cleave->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=buff.power_infusion.up&buff.bloodlust.up&active_enemies<3" );
  default_cleave->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=8&(active_"
      "enemies<3|talent.shadowboxing_treads)" );
  default_cleave->add_action(
      "whirling_dragon_punch,target_if=max:target.time_to_die,if=!talent.revolving_whirl|talent.revolving_whirl&buff."
      "dance_of_chiji.stack<2&active_enemies>2|active_enemies<3" );
  default_cleave->add_action(
      "strike_of_the_windlord,target_if=max:debuff.acclamation.stack,if=time>5&(cooldown.invoke_xuen_the_white_tiger."
      "remains>15|talent.flurry_strikes)" );
  default_cleave->add_action(
      "crackling_jade_lightning,target_if=max:target.time_to_die,if=buff.the_emperors_capacitor.stack>19&combo_strike&"
      "talent.power_of_the_thunder_king" );
  default_cleave->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.dance_of_chiji.stack=2" );
  default_cleave->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&energy.time_to_max<=gcd.max*3&talent."
      "flurry_strikes&active_enemies<5&buff.wisdom_of_the_wall_flurry.up&active_enemies<4" );
  default_cleave->add_action( "fists_of_fury,target_if=max:target.time_to_die" );
  default_cleave->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&energy.time_to_max<=gcd.max*3&talent."
      "flurry_strikes&active_enemies<5&buff.wisdom_of_the_wall_flurry.up" );
  default_cleave->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.dance_of_chiji.up&buff.chi_energy."
      "stack>29" );
  default_cleave->add_action(
      "rising_sun_kick,target_if=max:target.time_to_die,if=chi>4&(active_enemies<3|talent.glory_of_the_dawn)|chi>2&"
      "energy>50&(active_enemies<3|talent.glory_of_the_dawn)|cooldown.fists_of_fury.remains>2&(active_enemies<3|talent."
      "glory_of_the_dawn)" );
   default_cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=talent.shadowboxing_treads&talent.courageous_impulse&combo_strike&buff.bok_proc.stack=2" );
  default_cleave->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=4&!talent."
      "knowledge_of_the_broken_temple&talent.shadowboxing_treads&active_enemies<3" );
  default_cleave->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&buff.dance_of_chiji.up" );
  default_cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=talent.shadowboxing_treads&talent.courageous_impulse&combo_strike&buff.bok_proc.up" );
 
  default_cleave->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&energy.time_to_max<=gcd.max*3&talent."
      "flurry_strikes&active_enemies<5" );
  default_cleave->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&chi.deficit>=2&(!buff.ordered_"
      "elements.up|energy.time_to_max<=gcd.max*3)" );
  default_cleave->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&cooldown.fists_of_fury.remains&"
      "buff.teachings_of_the_monastery.stack>3&cooldown.rising_sun_kick.remains" );
  default_cleave->add_action(
      "jadefire_stomp,target_if=max:debuff.acclamation.stack,if=talent.Singularly_Focused_Jade|talent.jadefire_"
      "harmony" );
  default_cleave->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&cooldown.fists_of_fury.remains&("
      "buff.teachings_of_the_monastery.stack>3|buff.ordered_elements.up)&(talent.shadowboxing_treads|buff.bok_proc.up|"
      "buff.ordered_elements.up)" );
  default_cleave->add_action(
      "spinning_crane_kick,target_if=max:target.time_to_die,if=combo_strike&!buff.ordered_elements.up&talent.crane_"
      "vortex&active_enemies>2&chi>4" );
  default_cleave->add_action( "chi_burst,if=!buff.ordered_elements.up" );
  default_cleave->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&(buff.ordered_elements.up|buff.bok_"
      "proc.up&chi.deficit>=1&talent.energy_burst)&cooldown.fists_of_fury.remains" );
  default_cleave->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&cooldown.fists_of_fury.remains&("
      "chi>2|energy>60|buff.bok_proc.up)" );
  default_cleave->add_action( "jadefire_stomp,target_if=max:debuff.acclamation.stack" );
  default_cleave->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&buff.ordered_elements.up&chi.deficit>="
      "1" );
  default_cleave->add_action( "chi_burst" );
  default_cleave->add_action( "spinning_crane_kick,if=combo_strike&buff.ordered_elements.up&talent.hit_combo" );
  default_cleave->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.ordered_elements.up&!talent.hit_combo&"
      "cooldown.fists_of_fury.remains" );
  default_cleave->add_action( "tiger_palm,if=prev.tiger_palm&chi<3&!cooldown.fists_of_fury.remains" );

  // 1 Target priority
  default_st->add_action(
      "rising_sun_kick,if=buff.pressure_point.up|buff.ordered_elements.remains<=gcd.max*3&buff.storm_earth_and_fire.up",
      "1 target" );
  default_st->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=(energy>55&talent.inner_peace|energy>60&!talent."
      "inner_peace)&combo_strike&chi.max-chi>=2&buff.teachings_of_the_monastery.stack<buff.teachings_of_the_monastery."
      "max_stack&(talent.energy_burst&!buff.bok_proc.up|!talent.energy_burst)&!buff.ordered_elements.up|(talent.energy_"
      "burst&!buff.bok_proc.up|!talent.energy_burst)&!buff.ordered_elements.up&!cooldown.fists_of_fury.remains&chi<3|("
      "prev.strike_of_the_windlord|cooldown.strike_of_the_windlord.remains)&cooldown.celestial_conduit.remains<2&buff."
      "ordered_elements.up&chi<5&combo_strike|(!buff.heart_of_the_jade_serpent_cdr.up|!buff.heart_of_the_jade_serpent_"
      "cdr_celestial.up)&combo_strike&chi.deficit>=2&!buff.ordered_elements.up" );
  default_st->add_action( "touch_of_death" );
  default_st->add_action(
      "rising_sun_kick,target_if=max:debuff.acclamation.stack,if=buff.invokers_delight.up&!buff.storm_earth_and_fire."
      "up&talent.ordered_elements" );
  default_st->add_action(
      "celestial_conduit,if=buff.storm_earth_and_fire.up&(!talent.ordered_elements|buff.ordered_elements.up)&cooldown."
      "strike_of_the_windlord.remains&(talent.xuens_bond|!talent.xuens_bond&buff.invokers_delight.up)|fight_remains<"
      "15" );
  default_st->add_action(
      "rising_sun_kick,target_if=max:debuff.acclamation.stack,if=!pet.xuen_the_white_tiger.active&prev.tiger_palm&time<"
      "5|buff.storm_earth_and_fire.up&talent.ordered_elements" );
  default_st->add_action(
      "strike_of_the_windlord,if=talent.gale_force&buff.invokers_delight.up&(buff.bloodlust.up|cooldown.celestial_"
      "conduit.remains&!buff.heart_of_the_jade_serpent_cdr_celestial.up)" );
  default_st->add_action(
      "rising_sun_kick,target_if=max:debuff.acclamation.stack,if=buff.power_infusion.up&buff.bloodlust.up" );
  default_st->add_action(
      "fists_of_fury,target_if=max:debuff.acclamation.stack,if=buff.power_infusion.up&buff.bloodlust.up" );
  default_st->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack>3&buff."
      "ordered_elements.up&cooldown.rising_sun_kick.remains>1&cooldown.fists_of_fury.remains>2" );
  default_st->add_action(
      "spinning_crane_kick,if=buff.dance_of_chiji.stack=2&combo_strike&buff.power_infusion.up&buff.bloodlust.up" );
  default_st->add_action( "whirling_dragon_punch,if=buff.power_infusion.up&buff.bloodlust.up" );
  default_st->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&energy.time_to_max<=gcd.max*3&talent."
      "flurry_strikes&buff.power_infusion.up&buff.bloodlust.up" );
  default_st->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack>4&"
      "cooldown.rising_sun_kick.remains>1&cooldown.fists_of_fury.remains>2" );
  default_st->add_action(
      "whirling_dragon_punch,if=!buff.heart_of_the_jade_serpent_cdr_celestial.up&!buff.dance_of_chiji.stack=2|buff."
      "ordered_elements.up|talent.knowledge_of_the_broken_temple" );
  default_st->add_action(
      "strike_of_the_windlord,if=time>5&(cooldown.invoke_xuen_the_white_tiger.remains>15|talent.flurry_strikes)" );
  default_st->add_action(
      "rising_sun_kick,target_if=max:debuff.acclamation.stack,if=chi>4|chi>2&energy>50|cooldown.fists_of_fury.remains>"
      "2" );
  default_st->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&energy.time_to_max<=gcd.max*3&talent."
      "flurry_strikes&buff.wisdom_of_the_wall_flurry.up" );
  default_st->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&chi.deficit>=2&energy.time_to_max<="
      "gcd.max*3" );
  default_st->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack>7&talent."
      "memory_of_the_monastery&!buff.memory_of_the_monastery.up&cooldown.fists_of_fury.remains" );
  default_st->add_action( "fists_of_fury" );
  default_st->add_action(
      "spinning_crane_kick,if=(buff.dance_of_chiji.stack=2|buff.dance_of_chiji.remains<2&buff.dance_of_chiji.up)&combo_"
      "strike&!buff.ordered_elements.up" );
  default_st->add_action( "whirling_dragon_punch" );
  default_st->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=4&!talent."
      "knowledge_of_the_broken_temple&cooldown.rising_sun_kick.remains>1&cooldown.fists_of_fury.remains>2" );
  default_st->add_action( "spinning_crane_kick,if=buff.dance_of_chiji.stack=2&combo_strike" );
  default_st->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=talent.courageous_impulse&combo_strike&buff.bok_proc.stack=2" );
  default_st->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&buff.ordered_elements.up&cooldown."
      "rising_sun_kick.remains>1&cooldown.fists_of_fury.remains>2" );
  default_st->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&energy.time_to_max<=gcd.max*3&talent."
      "flurry_strikes" );
  default_st->add_action(
      "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up&(buff.ordered_elements.up|energy.time_to_max>=gcd."
      "max*3&talent.sequenced_strikes&talent.energy_burst|!talent.sequenced_strikes|!talent.energy_burst|buff.dance_of_"
      "chiji.remains<=gcd.max*3)" );
  default_st->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&energy.time_to_max<=gcd.max*3&talent."
      "flurry_strikes" );
  default_st->add_action( "jadefire_stomp,if=talent.Singularly_Focused_Jade|talent.jadefire_harmony" );
  default_st->add_action( "chi_burst,if=!buff.ordered_elements.up" );
  default_st->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&(buff.ordered_elements.up|buff.bok_"
      "proc.up&chi.deficit>=1&talent.energy_burst)&cooldown.fists_of_fury.remains" );
  default_st->add_action(
      "crackling_jade_lightning,if=buff.the_emperors_capacitor.stack>19&!buff.ordered_elements.up&combo_strike" );
  default_st->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&cooldown.fists_of_fury.remains&("
      "chi>2|energy>60|buff.bok_proc.up)" );
  default_st->add_action( "jadefire_stomp" );
  default_st->add_action(
      "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&buff.ordered_elements.up&chi.deficit>="
      "1" );
  default_st->add_action( "chi_burst" );
  default_st->add_action( "spinning_crane_kick,if=combo_strike&buff.ordered_elements.up&talent.hit_combo" );
  default_st->add_action(
      "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.ordered_elements.up&!talent.hit_combo&"
      "cooldown.fists_of_fury.remains" );
  default_st->add_action( "tiger_palm,if=prev.tiger_palm&chi<3&!cooldown.fists_of_fury.remains" );
}

void no_spec( player_t *p )
{
  action_priority_list_t *pre = p->get_action_priority_list( "precombat" );
  action_priority_list_t *def = p->get_action_priority_list( "default" );

  pre->add_action( "flask" );
  pre->add_action( "food" );
  pre->add_action( "augmentation" );
  pre->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  pre->add_action( "potion" );

  def->add_action( "roll,if=movement.distance>5", "Move to target" );
  def->add_action( "chi_torpedo,if=movement.distance>5" );
  def->add_action( "Tiger Palm" );
}

}  // namespace monk_apl
