#include "class_modules/apl/apl_monk.hpp"

#include "class_modules/monk/sc_monk.hpp"
#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace monk_apl
{
std::string potion( const player_t* p )
{
  switch ( p->specialization() )
  {
    case MONK_BREWMASTER:
      if ( p->true_level >= 60 )
        return "phantom_fire";
      else if ( p->true_level >= 50 )
        return "unbridled_fury";
      else
        return "disabled";
      break;
    case MONK_MISTWEAVER:
      if ( p->true_level >= 60 )
        return "potion_of_spectral_intellect";
      else if ( p->true_level >= 50 )
        return "superior_battle_potion_of_intellect";
      else
        return "disabled";
      break;
    case MONK_WINDWALKER:
      if ( p->true_level >= 60 )
        return "potion_of_spectral_agility";
      else if ( p->true_level >= 50 )
        return "unbridled_fury";
      else
        return "disabled";
      break;
    default:
      return "disabled";
      break;
  }
}

std::string flask( const player_t* p )
{
  switch ( p->specialization() )
  {
    case MONK_BREWMASTER:
      if ( p->true_level >= 60 )
        return "spectral_flask_of_power";
      else if ( p->true_level >= 50 )
        return "currents";
      else
        return "disabled";
      break;
    case MONK_MISTWEAVER:
      if ( p->true_level >= 60 )
        return "spectral_flask_of_power";
      else if ( p->true_level >= 50 )
        return "greater_flask_of_endless_fathoms";
      else
        return "disabled";
      break;
    case MONK_WINDWALKER:
      if ( p->true_level >= 60 )
        return "spectral_flask_of_power";
      else if ( p->true_level >= 50 )
        return "greater_flask_of_the_currents";
      else
        return "disabled";
      break;
    default:
      return "disabled";
      break;
  }
}

std::string food( const player_t* p )
{
  switch ( p->specialization() )
  {
    case MONK_BREWMASTER:
      if ( p->true_level >= 60 )
        return "spinefin_souffle_and_fries";
      else if ( p->true_level >= 50 )
        return "biltong";
      else
        return "disabled";
      break;
    case MONK_MISTWEAVER:
      if ( p->true_level >= 60 )
        return "feast_of_gluttonous_hedonism";
      else if ( p->true_level >= 50 )
        return "famine_evaluator_and_snack_table";
      else
        return "disabled";
      break;
    case MONK_WINDWALKER:
      if ( p->true_level >= 60 )
        return "feast_of_gluttonous_hedonism";
      else if ( p->true_level >= 50 )
        return "mechdowels_big_mech";
      else
        return "disabled";
      break;
    default:
      return "disabled";
      break;
  }
}

std::string rune( const player_t* p )
{
  if ( p->true_level >= 60 )
    return "veiled";
  else if ( p->true_level >= 50 )
    return "battle_scarred";
  else if ( p->true_level >= 45 )
    return "defiled";
  return "disabled";
}

std::string temporary_enchant( const player_t* p )
{
  switch ( p->specialization() )
  {
    case MONK_BREWMASTER:
      if ( p->true_level >= 60 )
        return "main_hand:shadowcore_oil/off_hand:shadowcore_oil";
      else
        return "disabled";
      break;
    case MONK_MISTWEAVER:
      if ( p->true_level >= 60 )
        return "main_hand:shadowcore_oil";
      else
        return "disabled";
      break;
    case MONK_WINDWALKER:
      if ( p->true_level >= 60 )
        return "main_hand:shaded_weightstone/off_hand:shaded_weightstone";
      else
        return "disabled";
      break;
    default:
      return "disabled";
      break;
  }
}

void brewmaster( player_t* p )
{
  auto monk                   = debug_cast<monk::monk_t*>( p );
  action_priority_list_t* pre = p->get_action_priority_list( "precombat" );

  // Flask
  pre->add_action( "flask" );

  // Food
  pre->add_action( "food" );

  // Rune
  pre->add_action( "augmentation" );

  // Snapshot stats
  pre->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  pre->add_action( "potion" );

  pre->add_action( "fleshcraft" );

  pre->add_talent( p, "Chi Burst", "if=!covenant.night_fae" );
  pre->add_talent( p, "Chi Wave" );

  std::vector<std::string> racial_actions = p->get_racial_actions();
  action_priority_list_t* def             = p->get_action_priority_list( "default" );
  def->add_action( "auto_attack" );
  def->add_action( p, "Spear Hand Strike", "if=target.debuff.casting.react" );

  if ( p->items[ SLOT_MAIN_HAND ].name_str == "jotungeirr_destinys_call" )
    def->add_action( "use_item,name=" + p->items[ SLOT_MAIN_HAND ].name_str );

  for ( const auto& item : p->items )
  {
    if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
    {
      if ( item.name_str == "scars_of_fraternal_strife" )
        def->add_action( "use_item,name=" + item.name_str + 
            ",if=!buff.scars_of_fraternal_strife_4.up&time>1" );
      else if( item.name_str == "cache_of_acquired_treasures" )
          def->add_action( "use_item,name=" + item.name_str + ",if=buff.acquired_axe.up|fight_remains<25" );
      else if ( item.name_str == "jotungeirr_destinys_call" )
        continue;
      else
        def->add_action( "use_item,name=" + item.name_str );
    }
  }

  def->add_action( p, "Gift of the Ox", "if=health<health.max*0.65" );
  def->add_talent( p, "Dampen Harm", "if=incoming_damage_1500ms&buff.fortifying_brew.down" );
  def->add_action( p, "Fortifying Brew", "if=incoming_damage_1500ms&(buff.dampen_harm.down|buff.diffuse_magic.down)" );

  def->add_action( "potion" );

  for ( const auto& racial_action : racial_actions )
  {
    if ( racial_action != "arcane_torrent" )
      def->add_action( racial_action );
  }

  def->add_action(
      p, monk->talent.brewmaster.invoke_niuzao_the_black_ox, "invoke_niuzao_the_black_ox",
      "if=buff.recent_purifies.value>=health.max*0.05&(target.cooldown.pause_action.remains>=20|time<=10|target.cooldown.pause_action.duration=0)&(!runeforge.call_to_arms.equipped|cooldown.weapons_of_order.remains>25)&!buff.invoke_niuzao_the_black_ox.up",
      "Cast Niuzao when we'll get at least 20 seconds of uptime. This is specific to the default enemy APL and will "
      "need adjustments for other enemies." );
  def->add_action(
      p, monk->talent.brewmaster.invoke_niuzao_the_black_ox, "invoke_niuzao_the_black_ox",
      "if=buff.weapons_of_order.up&runeforge.call_to_arms.equipped&!buff.invoke_niuzao_the_black_ox.up",
      "Cast Niuzao if we just lost the Niuzao from CtA"
      );
  def->add_action( p, "Touch of Death", "if=target.health.pct<=15" );

  // Covenant Abilities
  def->add_action( "weapons_of_order,if=!runeforge.call_to_arms.equipped|(buff.recent_purifies.value>=health.max*0.05&(target.cooldown.pause_action.remains>=20|time<=10|target.cooldown.pause_action.duration=0)&!buff.invoke_niuzao_the_black_ox.up)", "Use WoO on CD unless we have CtA equipped, in which case we treat it as mini-Niuzao not WoO." );
  def->add_action( "fallen_order" );
  def->add_action( "bonedust_brew,if=!debuff.bonedust_brew_debuff.up" );

  // Purifying Brew
  def->add_action( p, "Purifying Brew",
                   "if=stagger.amounttototalpct>=0.7&(((target.cooldown.pause_action.remains>=20|time<=10|target.cooldown.pause_action.duration=0)&cooldown."
                   "invoke_niuzao_the_black_ox.remains<5)|buff.invoke_niuzao_the_black_ox.up)",
                   "Cast PB during the Niuzao window, but only if recently hit." );
  def->add_action( p, "Purifying Brew", "if=buff.invoke_niuzao_the_black_ox.up&buff.invoke_niuzao_the_black_ox.remains<8", "Dump PB charges towards the end of Niuzao: anything is better than nothing." );
  def->add_action( p, "Purifying Brew", "if=cooldown.purifying_brew.charges_fractional>=1.8&(cooldown.invoke_niuzao_the_black_ox.remains>10|buff.invoke_niuzao_the_black_ox.up)", "Avoid capping charges, but pool charges shortly before Niuzao comes up and allow dumping to avoid capping during Niuzao." );

  // Black Ox Brew
  def->add_talent( p, "Black Ox Brew", "if=cooldown.purifying_brew.charges_fractional<0.5",
                   "Black Ox Brew is currently used to either replenish brews based on less than half a brew charge "
                   "available, or low energy to enable Keg Smash" );
  def->add_talent(
      p, "Black Ox Brew",
      "if=(energy+(energy.regen*cooldown.keg_smash.remains))<40&buff.blackout_combo.down&cooldown.keg_smash.up" );

  def->add_action( "fleshcraft,if=cooldown.bonedust_brew.remains<4&soulbind.pustule_eruption.enabled" );

  def->add_action(
      p, "Keg Smash", "if=spell_targets>=2",
      "Offensively, the APL prioritizes KS on cleave, BoS else, with energy spenders and cds sorted below" );

  // Covenant Faeline Stomp
  def->add_action( "faeline_stomp,if=spell_targets>=2" );

  def->add_action( p, "Keg Smash", "if=buff.weapons_of_order.up", "cast KS at top prio during WoO buff" );

  // Celestial Brew
  def->add_action( p, "Celestial Brew",
                   "if=buff.blackout_combo.down&incoming_damage_1999ms>(health.max*0.1+stagger.last_tick_damage_4)&buff.elusive_brawler.stack<2",
                   "Celestial Brew priority whenever it took significant damage (adjust the health.max coefficient "
                   "according to intensity of damage taken), and to dump excess charges before BoB." );
  
  def->add_action( "exploding_keg" );

  def->add_action( p, "Tiger Palm",
                   "if=talent.rushing_jade_wind.enabled&buff.blackout_combo.up&buff.rushing_jade_wind.up" );
  def->add_action( p, "Breath of Fire", "if=buff.charred_passions.down&runeforge.charred_passions.equipped" );
  def->add_action( p, "Blackout Kick" );
  def->add_action( p, "Keg Smash" );

  // Covenant Faeline Stomp
  def->add_talent( p, "Chi Burst", "if=cooldown.faeline_stomp.remains>2&spell_targets>=2" );
  def->add_action( "faeline_stomp" );

  def->add_action( p, "Touch of Death" );
  def->add_talent( p, "Rushing Jade Wind", "if=buff.rushing_jade_wind.down" );
  def->add_action( p, "Spinning Crane Kick", "if=buff.charred_passions.up" );
  def->add_action( p, "Breath of Fire",
      "if=buff.blackout_combo.down&(buff.bloodlust.down|(buff.bloodlust.up&dot.breath_of_fire_dot.refreshable))" );
  def->add_talent( p, "Chi Burst" );
  def->add_talent( p, "Chi Wave" );
  def->add_action( p, "Spinning Crane Kick",
      "if=!runeforge.shaohaos_might.equipped&active_enemies>=3&cooldown.keg_smash.remains>gcd&(energy+(energy.regen*("
      "cooldown.keg_smash.remains+execute_time)))>=65&(!talent.spitfire.enabled|!runeforge.charred_passions.equipped)",
      "Cast SCK if enough enemies are around, or if WWWTO is enabled. This is a slight defensive loss over using TP "
      "but generally reduces sim variance more than anything else." );
  def->add_action( p, "Tiger Palm",
       "if=!talent.blackout_combo&cooldown.keg_smash.remains>gcd&(energy+(energy.regen*(cooldown.keg_smash.remains+gcd)))>=65" );

  for ( const auto& racial_action : racial_actions )
  {
    if ( racial_action == "arcane_torrent" )
      def->add_action( racial_action + ",if=energy<31" );
  }

  def->add_action("fleshcraft,if=soulbind.volatile_solvent.enabled");

  def->add_talent( p, "Rushing Jade Wind" );
}

void mistweaver( player_t* p )
{
  action_priority_list_t* pre = p->get_action_priority_list( "precombat" );

  // Flask
  pre->add_action( "flask" );

  // Food
  pre->add_action( "food" );

  // Rune
  pre->add_action( "augmentation" );

  // Snapshot stats
  pre->add_action( "snapshot_stats" );

  pre->add_action( "potion" );

  pre->add_action( "fleshcraft" );

  pre->add_talent( p, "Chi Burst" );
  pre->add_talent( p, "Chi Wave" );

  std::vector<std::string> racial_actions = p->get_racial_actions();
  action_priority_list_t* def             = p->get_action_priority_list( "default" );
  action_priority_list_t* st              = p->get_action_priority_list( "st" );
  action_priority_list_t* aoe             = p->get_action_priority_list( "aoe" );

  def->add_action( "auto_attack" );
  
  if ( p->items[ SLOT_MAIN_HAND ].name_str == "jotungeirr_destinys_call" )
    def->add_action( "use_item,name=" + p->items[ SLOT_MAIN_HAND ].name_str );

  for ( const auto& item : p->items )
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

  for ( const auto& racial_action : racial_actions )
  {
    def->add_action( racial_action + ",if=target.time_to_die<18" );
  }

  def->add_action( "potion" );

  // Covenant Abilities
  def->add_action( "weapons_of_order" );
  def->add_action( "faeline_stomp" );
  def->add_action( "fallen_order" );
  def->add_action( "bonedust_brew" );

  def->add_action( "fleshcraft,if=soulbind.lead_by_example.enabled" );

  def->add_action( "call_action_list,name=aoe,if=active_enemies>=3" );
  def->add_action( "call_action_list,name=st,if=active_enemies<3" );

  st->add_action( p, "Thunder Focus Tea" );
  st->add_action( p, "Rising Sun Kick" );
  st->add_action( p, "Blackout Kick",
                  "if=buff.teachings_of_the_monastery.stack=1&cooldown.rising_sun_kick.remains<12" );
  st->add_talent( p, "Chi Wave" );
  st->add_talent( p, "Chi Burst" );
  st->add_action( p, "Tiger Palm",
                  "if=buff.teachings_of_the_monastery.stack<3|buff.teachings_of_the_monastery.remains<2" );

  aoe->add_action( p, "Spinning Crane Kick" );
  aoe->add_talent( p, "Chi Wave" );
  aoe->add_talent( p, "Chi Burst" );
}

void windwalker( player_t* p )
{
  auto monk = debug_cast<monk::monk_t*>( p );

  action_priority_list_t* pre = p->get_action_priority_list( "precombat" );

  // Flask
  pre->add_action( "flask" );
  // Food
  pre->add_action( "food" );
  // Rune
  pre->add_action( "augmentation" );

  // Snapshot stats
  pre->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  pre->add_action( "variable,name=xuen_on_use_trinket,op=set,value=equipped.inscrutable_quantum_device|equipped.gladiators_badge|equipped.wrathstone|equipped.overcharged_anima_battery|equipped.shadowgrasp_totem|equipped.the_first_sigil|equipped.cache_of_acquired_treasures" );
  pre->add_action( "fleshcraft" );
  pre->add_action( p, "Expel Harm", "if=chi<chi.max" );
  pre->add_talent( p, "Chi Burst", "if=!covenant.night_fae&!talent.faeline_stomp" );
  pre->add_talent( p, "Chi Wave" );

  std::vector<std::string> racial_actions  = p->get_racial_actions();
  action_priority_list_t* def              = p->get_action_priority_list( "default" );
  action_priority_list_t* opener           = p->get_action_priority_list( "opener" );
  action_priority_list_t* spend_energy     = p->get_action_priority_list( "spend_energy" );
  action_priority_list_t* bdb_setup        = p->get_action_priority_list( "bdb_setup" );
  action_priority_list_t* cd_sef           = p->get_action_priority_list( "cd_sef" );
  action_priority_list_t* cd_serenity      = p->get_action_priority_list( "cd_serenity" );
  action_priority_list_t* serenity         = p->get_action_priority_list( "serenity" );
  action_priority_list_t* def_actions      = p->get_action_priority_list( "def_actions" );

  def->add_action( "auto_attack" );
  def->add_action( p, "Spear Hand Strike", "if=target.debuff.casting.react" );
  def->add_action(
      "variable,name=hold_xuen,op=set,value=cooldown.invoke_xuen_the_white_tiger.remains>fight_remains|fight_remains-cooldown.invoke_xuen_the_white_tiger.remains<120&((talent.serenity&fight_remains>cooldown.serenity.remains&cooldown.serenity.remains>10)|(cooldown.storm_earth_and_fire.full_recharge_time<fight_remains&cooldown.storm_earth_and_fire.full_recharge_time>15)|(cooldown.storm_earth_and_fire.charges=0&cooldown.storm_earth_and_fire.remains<fight_remains))" );
  def->add_action(
      "variable,name=hold_sef,op=set,value=cooldown.bonedust_brew.up&cooldown.storm_earth_and_fire.charges<2&chi<3|buff.bonedust_brew.remains<8" );

  // Fixate if using Coordinated Offensive conduit
  def->add_action( "storm_earth_and_fire_fixate,if=conduit.coordinated_offensive.enabled&spinning_crane_kick.max", "Fixate if using Coordinated Offensive conduit" );

  // Potion
  if ( p->sim->allow_potions )
  {
    if ( monk->talent.windwalker.invoke_xuen_the_white_tiger->ok() )
      def->add_action(
          "potion,if=(buff.serenity.up|buff.storm_earth_and_fire.up)&pet.xuen_the_white_tiger.active|fight_remains<=60", "Potion" );
    else
      def->add_action( "potion,if=(buff.serenity.up|buff.storm_earth_and_fire.up)&fight_remains<=60", "Potion");
  }

  // Build Chi at the start of combat
  if ( monk->talent.windwalker.invoke_xuen_the_white_tiger->ok() )
    def->add_action( "call_action_list,name=opener,if=time<4&chi<5&!pet.xuen_the_white_tiger.active", "Build Chi at the start of combat" );
  else
    def->add_action( "call_action_list,name=opener,if=time<4&chi<5", "Build Chi at the start of combat" );

  // Prioritize Faeline Stomp if playing with Faeline Harmony or Niya Soulbind
  def->add_action( "faeline_stomp,if=combo_strike&fight_remains>5&!buff.bonedust_brew.up&(runeforge.faeline_harmony|talent.faeline_harmony|soulbind.grove_invigoration|active_enemies<3&buff.storm_earth_and_fire.down)", "Prioritize Faeline Stomp if playing with Faeline Harmony or Niya Soulbind" );
  // Spend excess energy
  def->add_action( "call_action_list,name=spend_energy,if=!buff.bonedust_brew.up&!buff.first_strike.up", "Spend excess energy" );
  // Use Chi Burst to reset Faeline Stomp
  def->add_talent( p, "Chi Burst", "if=(covenant.night_fae|talent.faeline_stomp)&cooldown.faeline_stomp.remains&(chi.max-chi>=1&active_enemies=1|chi.max-chi>=2&active_enemies>=2)&!buff.first_strike.up", "Use Chi Burst to reset Faeline Stomp" );

  // Use Cooldowns
  def->add_action( "call_action_list,name=cd_sef,if=!talent.serenity", "Cooldowns" );
  def->add_action( "call_action_list,name=cd_serenity,if=talent.serenity" );
  
  // Serenity / Default priority
  def->add_action( "call_action_list,name=serenity,if=buff.serenity.up", "Serenity / Default Priority");
  def->add_action( "call_action_list,name=def_actions" );

  // Storm, Earth and Fire Cooldowns
  cd_sef->add_action( "summon_white_tiger_statue,if=pet.xuen_the_white_tiger.active", "Storm, Earth and Fire Cooldowns" );
  cd_sef->add_action( "invoke_xuen_the_white_tiger,if=!variable.hold_xuen&(covenant.necrolord|talent.bonedust_brew)&cooldown.bonedust_brew.remains<=5&(active_enemies<3&chi>=3|active_enemies>=3&chi>=2)|fight_remains<25" );
  cd_sef->add_action( "invoke_xuen_the_white_tiger,if=!variable.hold_xuen&!(covenant.necrolord|talent.bonedust_brew)&(cooldown.rising_sun_kick.remains<2|!covenant.kyrian)&chi>=3" );
  cd_sef->add_action(
    "weapons_of_order,if=(raid_event.adds.in>45|raid_event.adds.up)&cooldown.rising_sun_kick.remains<execute_time&cooldown.invoke_xuen_the_white_tiger.remains>(20+20*(runeforge.invokers_delight|talent.invokers_delight))&(!runeforge.xuens_treasure&!talent.xuens_battlegear|cooldown.fists_of_fury.remains)|fight_remains<35" );
  cd_sef->add_action( "storm_earth_and_fire,if=(covenant.necrolord|talent.bonedust_brew)&(fight_remains<30&cooldown.bonedust_brew.remains<4&chi>=4|buff.bonedust_brew.up&!variable.hold_sef|!spinning_crane_kick.max&active_enemies>=3&cooldown.bonedust_brew.remains<=2&chi>=2)&(pet.xuen_the_white_tiger.active|cooldown.invoke_xuen_the_white_tiger.remains>cooldown.storm_earth_and_fire.full_recharge_time)" );
  cd_sef->add_action( "bonedust_brew,if=(!buff.bonedust_brew.up&buff.storm_earth_and_fire.up&buff.storm_earth_and_fire.remains<11&spinning_crane_kick.max)|(!buff.bonedust_brew.up&fight_remains<30&fight_remains>10&spinning_crane_kick.max&chi>=4)|fight_remains<10&soulbind.lead_by_example" );
  cd_sef->add_action(
      "call_action_list,name=bdb_setup,if=!buff.bonedust_brew.up&(talent.bonedust_brew|covenant.necrolord)&cooldown.bonedust_brew.remains<=2&(fight_remains>60&(cooldown.storm_earth_and_fire.charges>0|cooldown.storm_earth_and_fire.remains>10)&(pet.xuen_the_white_tiger.active|cooldown.invoke_xuen_the_white_tiger.remains>10|variable.hold_xuen)|((pet.xuen_the_white_tiger.active|cooldown.invoke_xuen_the_white_tiger.remains>13)&(cooldown.storm_earth_and_fire.charges>0|cooldown.storm_earth_and_fire.remains>13|buff.storm_earth_and_fire.up)))" );

  cd_sef->add_action( "storm_earth_and_fire,if=fight_remains<20|!covenant.necrolord&(cooldown.storm_earth_and_fire.charges=2|buff.weapons_of_order.up|covenant.kyrian&cooldown.weapons_of_order.remains>cooldown.storm_earth_and_fire.full_recharge_time|cooldown.invoke_xuen_the_white_tiger.remains>cooldown.storm_earth_and_fire.full_recharge_time)&cooldown.fists_of_fury.remains<=9&chi>=2&cooldown.whirling_dragon_punch.remains<=12" );

  if ( monk->talent.windwalker.invoke_xuen_the_white_tiger->ok() )
    cd_sef->add_action( p, "Touch of Death",
                        "if=fight_remains>(180-runeforge.fatal_touch*120)|buff.storm_earth_and_fire.down&pet.xuen_the_white_tiger.active&(!covenant.necrolord|buff.bonedust_brew.up)|(cooldown.invoke_xuen_the_white_tiger.remains>fight_remains)&buff.bonedust_brew.up|fight_remains<10" );
  else
    cd_sef->add_action(
        p, "Touch of Death",
        "if=fight_remains>(180-runeforge.fatal_touch*120)|buff.storm_earth_and_fire.down&(!covenant.necrolord|buff.bonedust_brew.up)|fight_remains<10" );


  cd_sef->add_action( "fallen_order,if=raid_event.adds.in>30|raid_event.adds.up" );

  // Storm, Earth, and Fire on-use trinkets
  
  // Scars of Fraternal Strife 1st-4th Rune, always used first
  if ( p->items[SLOT_TRINKET_1].name_str == "scars_of_fraternal_strife" || p->items[SLOT_TRINKET_2].name_str == "scars_of_fraternal_strife" )
    cd_sef->add_action( "use_item,name=scars_of_fraternal_strife,if=!buff.scars_of_fraternal_strife_4.up" );

  if ( p->items[SLOT_MAIN_HAND].name_str == "jotungeirr_destinys_call" )
    cd_sef->add_action( "use_item,name=" + p->items[SLOT_MAIN_HAND].name_str + ",if=pet.xuen_the_white_tiger.active|cooldown.invoke_xuen_the_white_tiger.remains>60&fight_remains>180|fight_remains<20" );

  for ( const auto& item : p->items )
  {
    if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
    {
      if ( item.name_str == "inscrutable_quantum_device" )
        cd_sef->add_action( "use_item,name=" + item.name_str +
          ",if=pet.xuen_the_white_tiger.active|cooldown.invoke_xuen_the_white_tiger.remains>60&fight_remains>180|fight_remains<20" );
      else if ( item.name_str == "wrathstone" )
        cd_sef->add_action( "use_item,name=" + item.name_str +
          ",if=pet.xuen_the_white_tiger.active|fight_remains<20" );
      else if ( item.name_str == "shadowgrasp_totem" )
        cd_sef->add_action( "use_item,name=" + item.name_str +
          ",if=pet.xuen_the_white_tiger.active|fight_remains<20|!runeforge.invokers_delight&!talent.invokers_delight" );
      else if ( item.name_str == "overcharged_anima_battery" )
        cd_sef->add_action( "use_item,name=" + item.name_str +
          ",if=pet.xuen_the_white_tiger.active|cooldown.invoke_xuen_the_white_tiger.remains>90|fight_remains<20" );
      else if ( (int)item.name_str.find( "gladiators_badge" ) != -1 )
        cd_sef->add_action( "use_item,name=" + item.name_str +
          ",if=covenant.necrolord&(buff.storm_earth_and_fire.remains>10|buff.bonedust_brew.up&cooldown.bonedust_brew.remains>30)|!covenant.necrolord&(cooldown.invoke_xuen_the_white_tiger.remains>55|variable.hold_xuen)|fight_remains<15" );
      else if ( item.name_str == "the_first_sigil" )
        cd_sef->add_action( "use_item,name=" + item.name_str +
          ",if=pet.xuen_the_white_tiger.remains>15|cooldown.invoke_xuen_the_white_tiger.remains>60&fight_remains>300|fight_remains<20" );
      else if ( item.name_str == "cache_of_acquired_treasures" )
        cd_sef->add_action( "use_item,name=" + item.name_str +
          ",if=active_enemies<2&buff.acquired_wand.up|active_enemies>1&buff.acquired_axe.up|fight_remains<20" );
      else if ( item.name_str == "scars_of_fraternal_strife" )
        // Scars of Fraternal Strife Final Rune, use in order of trinket slots
        cd_sef->add_action( "use_item,name=" + item.name_str +
          ",if=(buff.scars_of_fraternal_strife_4.up&(active_enemies>1|raid_event.adds.in<20)&(buff.weapons_of_order.up|(debuff.bonedust_brew_debuff.up&pet.xuen_the_white_tiger.active)))|fight_remains<35" );
      else if ( item.name_str == "enforcers_stun_grenade" )
        cd_sef->add_action( "use_item,name=" + item.name_str +
          ",if=!covenant.necrolord&pet.xuen_the_white_tiger.active|buff.storm_earth_and_fire.remains>10|buff.bonedust_brew.up&cooldown.bonedust_brew.remains>30|fight_remains<20" );
      else if ( item.name_str == "kihras_adrenaline_injector" )
        cd_sef->add_action( "use_item,name=" + item.name_str +
          ",if=!covenant.necrolord&pet.xuen_the_white_tiger.active|buff.storm_earth_and_fire.remains>10|buff.bonedust_brew.up&cooldown.bonedust_brew.remains>30|fight_remains<20" );
      else if ( item.name_str == "wraps_of_electrostatic_potential" )
        cd_sef->add_action( "use_item,name=" + item.name_str );
      else if ( item.name_str == "ring_of_collapsing_futures" )
        cd_sef->add_action( "use_item,name=" + item.name_str +
          ",if=buff.temptation.down|fight_remains<30" );
      else if ( item.name_str == "jotungeirr_destinys_call" )
        continue;
      else
        cd_sef->add_action( "use_item,name=" + item.name_str +
          ",if=!variable.xuen_on_use_trinket|cooldown.invoke_xuen_the_white_tiger.remains>20&pet.xuen_the_white_tiger.remains<20|variable.hold_xuen" );
    }
  }


  if ( monk->talent.windwalker.invoke_xuen_the_white_tiger->ok() )
    cd_sef->add_action( p, "Touch of Karma",
                        "target_if=max:target.time_to_die,if=fight_remains>90|pet.xuen_the_white_tiger.active|variable.hold_xuen|fight_remains<16" );
  else
    cd_sef->add_action( p, "Touch of Karma", "if=fight_remains>159|variable.hold_xuen" );

  // Storm, Earth and Fire Racials
  for ( const auto& racial_action : racial_actions )
  {
    if ( racial_action != "arcane_torrent" )
    {
      if ( racial_action == "ancestral_call" )
        cd_sef->add_action( racial_action +
                            ",if=cooldown.invoke_xuen_the_white_tiger.remains>30|variable.hold_xuen|fight_remains<20" );
      else if ( racial_action == "blood_fury" )
        cd_sef->add_action( racial_action +
                            ",if=cooldown.invoke_xuen_the_white_tiger.remains>30|variable.hold_xuen|fight_remains<20" );
      else if ( racial_action == "fireblood" )
        cd_sef->add_action( racial_action +
                            ",if=cooldown.invoke_xuen_the_white_tiger.remains>30|variable.hold_xuen|fight_remains<10" );
      else if ( racial_action == "berserking" )
        cd_sef->add_action( racial_action +
                            ",if=cooldown.invoke_xuen_the_white_tiger.remains>30|variable.hold_xuen|fight_remains<15" );
      else if ( racial_action == "bag_of_tricks" )
        cd_sef->add_action( racial_action + ",if=buff.storm_earth_and_fire.down" );
      else
        cd_sef->add_action( racial_action );
    }
  }

  cd_sef->add_action( "fleshcraft,if=soulbind.pustule_eruption&!pet.xuen_the_white_tiger.active&buff.storm_earth_and_fire.down&buff.bonedust_brew.down" );
 
  // Serenity Cooldowns

  if ( monk->talent.windwalker.invoke_xuen_the_white_tiger->ok() )
    cd_serenity->add_action(
      "variable,name=serenity_burst,op=set,value=cooldown.serenity.remains<1|pet.xuen_the_white_tiger.active&cooldown.serenity.remains>30|fight_remains<20", "Serenity Cooldowns");
  else
    cd_serenity->add_action(
      "variable,name=serenity_burst,op=set,value=cooldown.serenity.remains<1|cooldown.serenity.remains>30|fight_"
      "remains<20", "Serenity Cooldowns");

  cd_serenity->add_action( "invoke_xuen_the_white_tiger,if=!variable.hold_xuen&(covenant.necrolord|talent.bonedust_brew)&cooldown.bonedust_brew.remains<=5|fight_remains<25" );
  cd_serenity->add_action( "invoke_xuen_the_white_tiger,if=!variable.hold_xuen&!(covenant.necrolord|talent.bonedust_brew)&(cooldown.rising_sun_kick.remains<2|!covenant.kyrian)|fight_remains<25" );

  cd_serenity->add_action(
    "weapons_of_order,if=(raid_event.adds.in>45|raid_event.adds.up)&cooldown.rising_sun_kick.remains<execute_time&cooldown.invoke_xuen_the_white_tiger.remains>(20+20*(runeforge.invokers_delight|talent.invokers_delight))&(!runeforge.xuens_treasure&!talent.xuens_battlegear|cooldown.fists_of_fury.remains)|fight_remains<35" );
  cd_serenity->add_action(
    "bonedust_brew,if=(!buff.bonedust_brew.up&cooldown.serenity.up&spinning_crane_kick.max)|(!buff.bonedust_brew.up&fight_remains<30&fight_remains>10&spinning_crane_kick.max)|fight_remains<10&soulbind.lead_by_example" );
 
  cd_serenity->add_action(
    "serenity,if=(covenant.necrolord|talent.bonedust_brew)&(fight_remains<30&cooldown.bonedust_brew.remains<2|buff.bonedust_brew.up&buff.bonedust_brew.remains>=8)&(pet.xuen_the_white_tiger.active|cooldown.invoke_xuen_the_white_tiger.remains>10)|fight_remains<15" );
  cd_serenity->add_action(
    "serenity,if=!(covenant.necrolord|talent.bonedust_brew)&(pet.xuen_the_white_tiger.active|cooldown.invoke_xuen_the_white_tiger.remains>10)");
 

  if ( monk->talent.windwalker.invoke_xuen_the_white_tiger->ok() )
  {
    cd_serenity->add_action( p, "Touch of Death",
      "if=fight_remains>(180-runeforge.fatal_touch*120)|pet.xuen_the_white_tiger.active&(!covenant.necrolord|buff.bonedust_brew.up)|(cooldown.invoke_xuen_the_white_tiger.remains>fight_remains)&buff.bonedust_brew.up|fight_remains<10" );
    cd_serenity->add_action( "touch_of_karma,if=fight_remains>90|pet.xuen_the_white_tiger.active|fight_remains<10" );
  }
  else
  {
    cd_serenity->add_action( p, "Touch of Death", "if=fight_remains>(180-runeforge.fatal_touch*120)|buff.bonedust_brew.up|fight_remains<10" );
    cd_serenity->add_action( "touch_of_karma,if=fight_remains>90|fight_remains<16" );
  }

  cd_serenity->add_action(
    "fallen_order" );
  
  // Serenity Racials
  for ( const auto& racial_action : racial_actions )
  {
    if ( racial_action == "ancestral_call" )
      cd_serenity->add_action( racial_action + ",if=variable.serenity_burst" );
    else if ( racial_action == "blood_fury" )
      cd_serenity->add_action( racial_action + ",if=variable.serenity_burst" );
    else if ( racial_action == "fireblood" )
      cd_serenity->add_action( racial_action + ",if=variable.serenity_burst" );
    else if ( racial_action == "berserking" )
      cd_serenity->add_action( racial_action + ",if=variable.serenity_burst" );
    else if ( racial_action == "bag_of_tricks" )
      cd_serenity->add_action( racial_action + ",if=variable.serenity_burst" );
    else if ( racial_action != "arcane_torrent" )
      cd_serenity->add_action( racial_action );
  }

  cd_serenity->add_action(
    "fleshcraft,if=soulbind.pustule_eruption&!pet.xuen_the_white_tiger.active&buff.serenity.down&buff.bonedust_brew.down" );

  // Serenity On-use items
  if ( p->items[SLOT_MAIN_HAND].name_str == "jotungeirr_destinys_call" )
    cd_serenity->add_action( "use_item,name=" + p->items[SLOT_MAIN_HAND].name_str + ",if=variable.serenity_burst|fight_remains<20" );

  for ( const auto& item : p->items )
  {
    std::string name_str;
    if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
    {
      if ( item.name_str == "inscrutable_quantum_device" )
        cd_serenity->add_action( "use_item,name=" + item.name_str + ",if=variable.serenity_burst|fight_remains<20" );
      else if ( item.name_str == "wrathstone" )
        cd_serenity->add_action( "use_item,name=" + item.name_str + ",if=variable.serenity_burst|fight_remains<20" );
      else if ( item.name_str == "overcharged_anima_battery" )
        cd_serenity->add_action( "use_item,name=" + item.name_str + ",if=variable.serenity_burst|fight_remains<20" );
      else if ( item.name_str == "shadowgrasp_totem" )
        cd_serenity->add_action( "use_item,name=" + item.name_str + ",if=pet.xuen_the_white_tiger.active|fight_remains<20|!runeforge.invokers_delight" );
      else if ( (int)item.name_str.find( "gladiators_badge" ) != -1 )
        cd_serenity->add_action( "use_item,name=" + item.name_str + ",if=variable.serenity_burst|fight_remains<20" );
      else if ( item.name_str == "the_first_sigil" )
        cd_serenity->add_action( "use_item,name=" + item.name_str + ",if=variable.serenity_burst|fight_remains<20" );
      else if ( item.name_str == "wraps_of_electrostatic_potential" )
        cd_serenity->add_action( "use_item,name=" + item.name_str );
      else if ( item.name_str == "ring_of_collapsing_futures" )
        cd_serenity->add_action( "use_item,name=" + item.name_str +
          ",if=buff.temptation.down|fight_remains<30" );
      else if ( item.name_str == "jotungeirr_destinys_call" )
        continue;
      else
        cd_serenity->add_action(
          "use_item,name=" + item.name_str +
          ",if=!variable.xuen_on_use_trinket|cooldown.invoke_xuen_the_white_tiger.remains>20|variable.hold_xuen" );
    }
  }


  // Default Actions
  def_actions->add_action( "whirling_dragon_punch", "Default Actions" );
  def_actions->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up" );
  def_actions->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&buff.weapons_of_order.up" );
  def_actions->add_action( "strike_of_the_windlord,if=active_enemies>1" );
  def_actions->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack>=2" );
  def_actions->add_action( "strike_of_the_windlord" );
  def_actions->add_action( "spinning_crane_kick,if=combo_strike&(buff.bonedust_brew.up|buff.weapons_of_order_ww.up)&active_enemies>1&spinning_crane_kick.modifier>2.8" );
  def_actions->add_action( "spinning_crane_kick,if=buff.bonedust_brew.up&spinning_crane_kick.modifier>3.9&(!talent.teachings_of_the_monastery|!talent.shadowboxing_treads|active_enemies>=9)" );
  def_actions->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&(!talent.xuens_battlegear&!runeforge.xuens_treasure|cooldown.fists_of_fury.remains)" );
  def_actions->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=1&cooldown.rising_sun_kick.remains&(cooldown.fists_of_fury.remains|!talent.whirling_dragon_punch)" );
  def_actions->add_action( "fists_of_fury,target_if=max:target.time_to_die,if=(raid_event.adds.in>cooldown.fists_of_fury.duration*0.8|active_enemies>1)&(energy.time_to_max>execute_time-1|chi.max-chi<=1|buff.storm_earth_and_fire.remains<execute_time+1)|fight_remains<execute_time+1|debuff.bonedust_brew_debuff.up|buff.primordial_power.up" );
  def_actions->add_action( "spinning_crane_kick,if=combo_strike&(active_enemies>1&!talent.shadowboxing_treads|active_enemies>=6&spinning_crane_kick.modifier>=3.1)" );
  // ..
  def_actions->add_action( "faeline_stomp,if=combo_strike&!buff.bonedust_brew.up" );
  def_actions->add_action( "crackling_jade_lightning,if=buff.the_emperors_capacitor.stack>19&energy.time_to_max>execute_time-1&cooldown.rising_sun_kick.remains>execute_time|buff.the_emperors_capacitor.stack>14&(cooldown.serenity.remains<5&talent.serenity|cooldown.weapons_of_order.remains<5&covenant.kyrian|fight_remains<5)" );
  def_actions->add_action( "rushing_jade_wind,if=buff.rushing_jade_wind.down&active_enemies>1" );
  def_actions->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&buff.bonedust_brew.up&chi.max-chi>=(2+buff.power_strikes.up)" );
  def_actions->add_action( "expel_harm,if=chi.max-chi>=1" );
  def_actions->add_action( "chi_burst,if=chi.max-chi>=1&active_enemies=1&raid_event.adds.in>20|chi.max-chi>=2&active_enemies>=2" );
  def_actions->add_action( "chi_wave,if=!buff.primordial_power.up" );
  def_actions->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&chi.max-chi>=(2+buff.power_strikes.up)&buff.storm_earth_and_fire.down" );
  def_actions->add_action( "spinning_crane_kick,if=combo_strike&buff.chi_energy.stack>30-5*active_enemies&buff.storm_earth_and_fire.down&(cooldown.rising_sun_kick.remains>2&cooldown.fists_of_fury.remains>2|cooldown.rising_sun_kick.remains<3&cooldown.fists_of_fury.remains>3&chi>3|cooldown.rising_sun_kick.remains>3&cooldown.fists_of_fury.remains<3&chi>4|chi.max-chi<=1&energy.time_to_max<2)|buff.chi_energy.stack>10&fight_remains<7" );
  def_actions->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&(talent.serenity&cooldown.serenity.remains<3|cooldown.rising_sun_kick.remains>1&cooldown.fists_of_fury.remains>1|cooldown.rising_sun_kick.remains<3&cooldown.fists_of_fury.remains>3&chi>2|cooldown.rising_sun_kick.remains>3&cooldown.fists_of_fury.remains<3&chi>3|chi>5|buff.bok_proc.up)" );
  def_actions->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&chi.max-chi>=(2+buff.power_strikes.up)" );
  def_actions->add_action( "arcane_torrent,if=chi.max-chi>=1" );
  def_actions->add_action( "flying_serpent_kick,interrupt=1,if=!covenant.necrolord|buff.primordial_potential.up" );
  def_actions->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&energy.time_to_max<2&(chi.max-chi<=1|prev_gcd.1.tiger_palm)" );


  // Serenity Priority
  serenity->add_action( "whirling_dragon_punch", "Serenity Priority" );
  serenity->add_action( "strike_of_the_windlord" );
  serenity->add_action( "fists_of_fury" );
  serenity->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=3&talent.shadowboxing_treads&active_enemies>1" );
  serenity->add_action( "spinning_crane_kick,if=combo_strike&active_enemies>1&spinning_crane_kick.modifier>3.1" );
  serenity->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike");
  serenity->add_action( "spinning_crane_kick,if=combo_strike&(buff.dance_of_chiji.up|active_enemies>=3)" );
  serenity->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike" );

  // Opener
  opener->add_action( p, "Expel Harm", "if=talent.chi_burst.enabled&chi.max-chi>=3", "Opener" );
  opener->add_action( p, "Tiger Palm",
    "target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&chi.max-chi>=(2+buff.power_strikes.up)" );
  opener->add_talent( p, "Chi Wave", "if=chi.max-chi=2" );
  opener->add_action( p, "Expel Harm" );
  opener->add_action( p, "Tiger Palm",
    "target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=chi.max-chi>=(2+buff.power_strikes.up)" );

  // Excess Energy
  spend_energy->add_action( p, "Expel Harm", "if=chi.max-chi>=1&(energy.time_to_max<1|cooldown.serenity.remains<2|energy.time_to_max<4&cooldown.fists_of_fury.remains<1.5|cooldown.weapons_of_order.remains<2)&(!buff.bonedust_brew.up|buff.bloodlust.up|buff.invokers_delight.up)", "Excess Energy" );
  spend_energy->add_action( p, "Tiger Palm",
    "target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=buff.teachings_of_the_monastery.stack<3&combo_strike&chi.max-chi>=(2+buff.power_strikes.up)&(energy.time_to_max<1|cooldown.serenity.remains<2|energy.time_to_max<4&cooldown.fists_of_fury.remains<1.5|cooldown.weapons_of_order.remains<2)&!buff.primordial_power.up" );

  // Bonedust Brew Setup
  bdb_setup->add_action( "bonedust_brew,if=spinning_crane_kick.max&chi>=4", "Bonedust Brew Setup" );
  bdb_setup->add_action("rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&chi>=5" );
  bdb_setup->add_action( p, "Tiger Palm",
    "target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&chi.max-chi>=2" );
  bdb_setup->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&cooldown.fists_of_fury.remains&active_enemies>=2" );

}

void no_spec( player_t* p )
{
  action_priority_list_t* pre = p->get_action_priority_list( "precombat" );
  action_priority_list_t* def = p->get_action_priority_list( "default" );

  pre->add_action( "flask" );
  pre->add_action( "food" );
  pre->add_action( "augmentation" );
  pre->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  pre->add_action( "potion" );

  def->add_action( "Tiger Palm" );
}

}  // namespace monk_apl
