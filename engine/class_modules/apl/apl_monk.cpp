#include "class_modules/apl/apl_monk.hpp"

#include "class_modules/monk/sc_monk.hpp"
#include "player/action_priority_list.hpp"
#include "player/sc_player.hpp"

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

  pre->add_talent( p, "Chi Burst" );
  pre->add_talent( p, "Chi Wave" );

  std::vector<std::string> racial_actions = p->get_racial_actions();
  action_priority_list_t* def             = p->get_action_priority_list( "default" );
  def->add_action( "auto_attack" );
  def->add_action( p, "Spear Hand Strike", "if=target.debuff.casting.react" );
  def->add_action( p, "Gift of the Ox", "if=health<health.max*0.65" );
  def->add_talent( p, "Dampen Harm", "if=incoming_damage_1500ms&buff.fortifying_brew.down" );
  def->add_action( p, "Fortifying Brew", "if=incoming_damage_1500ms&(buff.dampen_harm.down|buff.diffuse_magic.down)" );

  for ( size_t i = 0; i < p->items.size(); i++ )
  {
    std::string name_str;
    if ( p->items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
    {
      def->add_action( "use_item,name=" + p->items[ i ].name_str );
    }
  }

  def->add_action( "potion" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] != "arcane_torrent" )
      def->add_action( racial_actions[ i ] );
  }

  def->add_action( p, monk->spec.invoke_niuzao, "invoke_niuzao_the_black_ox", "if=target.time_to_die>6&cooldown.purifying_brew.charges_fractional<2" );
  def->add_action( p, "Touch of Death", "if=target.health.pct<=15" );

  // Covenant Abilities
  def->add_action( "weapons_of_order" );
  def->add_action( "fallen_order" );
  def->add_action( "bonedust_brew" );

  // Purifying Brew
  def->add_action( p, "Purifying Brew", "if=stagger.amounttototalpct>=0.7&(cooldown.invoke_niuzao_the_black_ox.remains<5|buff.invoke_niuzao_the_black_ox.up)", "Cast PB during the Niuzao window, but only if recently hit." );
  def->add_action( p, "Purifying Brew", "if=buff.invoke_niuzao_the_black_ox.up&buff.invoke_niuzao_the_black_ox.remains<8", "Dump PB charges towards the end of Niuzao: anything is better than nothing." );
  def->add_action( p, "Purifying Brew", "if=cooldown.purifying_brew.charges_fractional>=1.8&(cooldown.invoke_niuzao_the_black_ox.remains>10|buff.invoke_niuzao_the_black_ox.up)", "Avoid capping charges, but pool charges shortly before Niuzao comes up and allow dumping to avoid capping during Niuzao." );

  // Black Ox Brew
  def->add_talent( p, "Black Ox Brew", "if=cooldown.purifying_brew.charges_fractional<0.5",
                   "Black Ox Brew is currently used to either replenish brews based on less than half a brew charge "
                   "available, or low energy to enable Keg Smash" );
  def->add_talent(
      p, "Black Ox Brew",
      "if=(energy+(energy.regen*cooldown.keg_smash.remains))<40&buff.blackout_combo.down&cooldown.keg_smash.up" );

  def->add_action(
      p, "Keg Smash", "if=spell_targets>=2",
      "Offensively, the APL prioritizes KS on cleave, BoS else, with energy spenders and cds sorted below" );

  // Covenant Faeline Stomp
  def->add_action( "faeline_stomp,if=spell_targets>=2" );

  def->add_action( p, "Keg Smash", "if=buff.weapons_of_order.up", "cast KS at top prio during WoO buff" );

  // Celestial Brew
  def->add_action( p, "Celestial Brew",
                   "if=buff.blackout_combo.down&incoming_damage_1999ms>(health.max*0.1+stagger.last_tick_damage_4)&"
                   "buff.elusive_brawler.stack<2",
                   "Celestial Brew priority whenever it took significant damage (adjust the health.max coefficient "
                   "according to intensity of damage taken), and to dump excess charges before BoB." );

  def->add_action( p, "Tiger Palm",
                   "if=talent.rushing_jade_wind.enabled&buff.blackout_combo.up&buff.rushing_jade_wind.up" );
  def->add_action( p, "Breath of Fire", "if=buff.charred_passions.down&runeforge.charred_passions.equipped" );
  def->add_action( p, "Blackout Kick" );
  def->add_action( p, "Keg Smash" );

  // Covenant Faeline Stomp
  def->add_action( "faeline_stomp" );

  def->add_action( p, "Expel Harm", "if=buff.gift_of_the_ox.stack>=3" );
  def->add_action( p, "Touch of Death" );
  def->add_talent( p, "Rushing Jade Wind", "if=buff.rushing_jade_wind.down" );
  def->add_action( p, "Spinning Crane Kick", "if=buff.charred_passions.up" );
  def->add_action(
      p, "Breath of Fire",
      "if=buff.blackout_combo.down&(buff.bloodlust.down|(buff.bloodlust.up&dot.breath_of_fire_dot.refreshable))" );
  def->add_talent( p, "Chi Burst" );
  def->add_talent( p, "Chi Wave" );
  def->add_action( p, "Spinning Crane Kick",
                   "if=active_enemies>=3&cooldown.keg_smash.remains>gcd&(energy+(energy.regen*(cooldown.keg_smash."
                   "remains+execute_time)))>=65&(!talent.spitfire.enabled|!runeforge.charred_passions.equipped)" );
  def->add_action( p, "Tiger Palm",
                   "if=!talent.blackout_combo&cooldown.keg_smash.remains>gcd&(energy+(energy.regen*(cooldown.keg_smash."
                   "remains+gcd)))>=65" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] == "arcane_torrent" )
      def->add_action( racial_actions[ i ] + ",if=energy<31" );
  }

  def->add_talent( p, "Rushing Jade Wind" );

  //  def->add_action( p, "Expel Harm", "if=buff.gift_of_the_ox.stack>4" );
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

  pre->add_talent( p, "Chi Burst" );
  pre->add_talent( p, "Chi Wave" );

  std::vector<std::string> racial_actions = p->get_racial_actions();
  action_priority_list_t* def             = p->get_action_priority_list( "default" );
  action_priority_list_t* st              = p->get_action_priority_list( "st" );
  action_priority_list_t* aoe             = p->get_action_priority_list( "aoe" );

  def->add_action( "auto_attack" );
  int num_items = (int)p->items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( p->items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
      def->add_action( "use_item,name=" + p->items[ i ].name_str );
  }
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] == "arcane_torrent" )
      def->add_action( racial_actions[ i ] + ",if=chi.max-chi>=1&target.time_to_die<18" );
    else
      def->add_action( racial_actions[ i ] + ",if=target.time_to_die<18" );
  }

  def->add_action( "potion" );

  def->add_action( "run_action_list,name=aoe,if=active_enemies>=3" );
  def->add_action( "call_action_list,name=st,if=active_enemies<3" );

  // Covenant Abilities
  def->add_action( "weapons_of_order" );
  def->add_action( "faeline_stomp" );
  def->add_action( "fallen_order" );
  def->add_action( "bonedust_brew" );

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

  pre->add_action( "variable,name=xuen_on_use_trinket,op=set,value=equipped.inscrutable_quantum_device|equipped.gladiators_badge|equipped.wrathstone" );
  pre->add_talent( p, "Chi Burst" );
  pre->add_talent( p, "Chi Wave", "if=!talent.energizing_elixir.enabled" );

  std::vector<std::string> racial_actions  = p->get_racial_actions();
  action_priority_list_t* def              = p->get_action_priority_list( "default" );
  action_priority_list_t* opener           = p->get_action_priority_list( "opener" );
  action_priority_list_t* cd_sef           = p->get_action_priority_list( "cd_sef" );
  action_priority_list_t* cd_serenity      = p->get_action_priority_list( "cd_serenity" );
  action_priority_list_t* serenity         = p->get_action_priority_list( "serenity" );
  action_priority_list_t* weapons_of_order = p->get_action_priority_list( "weapons_of_order" );
  action_priority_list_t* aoe              = p->get_action_priority_list( "aoe" );
  action_priority_list_t* st               = p->get_action_priority_list( "st" );

  def->add_action( "auto_attack" );
  def->add_action( p, "Spear Hand Strike", "if=target.debuff.casting.react" );
  def->add_action(
      "variable,name=hold_xuen,op=set,value=cooldown.invoke_xuen_the_white_tiger.remains>fight_remains|fight_remains<"
      "120&fight_remains>cooldown.serenity.remains&cooldown.serenity.remains>10" );
  if ( p->sim->allow_potions )
  {
    if ( monk->spec.invoke_xuen->ok() )
      def->add_action(
          "potion,if=(buff.serenity.up|buff.storm_earth_and_fire.up)&pet.xuen_the_white_tiger.active|fight_remains<="
          "60" );
    else
      def->add_action( "potion,if=(buff.serenity.up|buff.storm_earth_and_fire.up)&fight_remains<=60" );
  }

  def->add_action( "call_action_list,name=serenity,if=buff.serenity.up" );
  def->add_action( "call_action_list,name=weapons_of_order,if=buff.weapons_of_order.up" );
  if ( monk->spec.invoke_xuen->ok() )
    def->add_action( "call_action_list,name=opener,if=time<4&chi<5&!pet.xuen_the_white_tiger.active" );
  else
    def->add_action( "call_action_list,name=opener,if=time<4&chi<5" );
  def->add_talent( p, "Fist of the White Tiger",
                   "target_if=min:debuff.mark_of_the_crane.remains,if=chi.max-chi>=3&(energy.time_to_max<1|energy.time_"
                   "to_max<4&cooldown.fists_of_fury.remains<1.5|cooldown.weapons_of_order.remains<2)" );
  def->add_action( p, "Expel Harm",
                   "if=chi.max-chi>=1&(energy.time_to_max<1|cooldown.serenity.remains<2|energy.time_to_max<4&cooldown."
                   "fists_of_fury.remains<1.5|cooldown.weapons_of_order.remains<2)" );
  def->add_action( p, "Tiger Palm",
                   "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&chi.max-chi>=2&(energy.time_to_max<"
                   "1|cooldown.serenity.remains<2|energy.time_to_max<4&cooldown.fists_of_fury.remains<1.5|cooldown."
                   "weapons_of_order.remains<2)" );
  def->add_action( "call_action_list,name=cd_sef,if=!talent.serenity" );
  def->add_action( "call_action_list,name=cd_serenity,if=talent.serenity" );
  def->add_action( "call_action_list,name=st,if=active_enemies<3" );
  def->add_action( "call_action_list,name=aoe,if=active_enemies>=3" );

  // Opener
  opener->add_talent( p, "Fist of the White Tiger",
                      "target_if=min:debuff.mark_of_the_crane.remains,if=chi.max-chi>=3" );
  opener->add_action( p, "Expel Harm", "if=talent.chi_burst.enabled&chi.max-chi>=3" );
  opener->add_action( p, "Tiger Palm",
                      "target_if=min:debuff.mark_of_the_crane.remains+(debuff.recently_rushing_tiger_palm.up*20),if="
                      "combo_strike&chi.max-chi>=2" );
  opener->add_talent( p, "Chi Wave", "if=chi.max-chi=2" );
  opener->add_action( p, "Expel Harm" );
  opener->add_action( p, "Tiger Palm",
      "target_if=min:debuff.mark_of_the_crane.remains+(debuff.recently_rushing_tiger_palm.up*20),if=chi.max-chi>=2" );

  // AoE
  aoe->add_talent( p, "Whirling Dragon Punch" );
  aoe->add_talent( p, "Energizing Elixir", "if=chi.max-chi>=2&energy.time_to_max>2|chi.max-chi>=4" );
  aoe->add_action( p, "Spinning Crane Kick", "if=combo_strike&(buff.dance_of_chiji.up|debuff.bonedust_brew.up)" );
  aoe->add_action( p, "Fists of Fury", "if=energy.time_to_max>execute_time|chi.max-chi<=1" );
  aoe->add_action( p, "Rising Sun Kick",
      "target_if=min:debuff.mark_of_the_crane.remains,if=(talent.whirling_dragon_punch&cooldown.rising_sun_kick."
      "duration>cooldown.whirling_dragon_punch.remains+4)&(cooldown.fists_of_fury.remains>3|chi>=5)" );
  aoe->add_talent( p, "Rushing Jade Wind", "if=buff.rushing_jade_wind.down" );
  aoe->add_action( p, "Expel Harm", "if=chi.max-chi>=1" );
  aoe->add_talent( p, "Fist of the White Tiger", "target_if=min:debuff.mark_of_the_crane.remains,if=chi.max-chi>=3" );
  aoe->add_talent( p, "Chi Burst", "if=chi.max-chi>=2" );
  aoe->add_action( p, "Crackling Jade Lightning",
                   "if=buff.the_emperors_capacitor.stack>19&energy.time_to_max>execute_time-1&cooldown.fists_of_fury."
                   "remains>execute_time" );
  aoe->add_action( p, "Tiger Palm",
                   "target_if=min:debuff.mark_of_the_crane.remains+(debuff.recently_rushing_tiger_palm.up*20),if=chi."
                   "max-chi>=2&(!talent.hit_combo|combo_strike)" );
  aoe->add_action( "arcane_torrent,if=chi.max-chi>=1" );
  aoe->add_action( p, "Spinning Crane Kick",
                   "if=combo_strike&(cooldown.bonedust_brew.remains>2|!covenant.necrolord)&(chi>=5|cooldown.fists_of_fury."
                   "remains>6|cooldown.fists_of_fury.remains>3&chi>=3&energy.time_to_50<1|energy.time_to_max<="
                   "(3+3*cooldown.fists_of_fury.remains<5)|buff.storm_earth_and_fire.up)" );
  aoe->add_talent( p, "Chi Wave", "if=combo_strike" );
  aoe->add_action( p, "Flying Serpent Kick", "if=buff.bok_proc.down,interrupt=1" );
  aoe->add_action( p, "Blackout Kick",
                   "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&(buff.bok_proc.up|talent.hit_combo&"
                   "prev_gcd.1.tiger_palm&chi=2&cooldown.fists_of_fury.remains<3|chi.max-chi<=1&prev_gcd.1.spinning_"
                   "crane_kick&energy.time_to_max<3)" );

  // Serenity Cooldowns
  if ( monk->spec.invoke_xuen->ok() )
    cd_serenity->add_action(
        "variable,name=serenity_burst,op=set,value=cooldown.serenity.remains<1|pet.xuen_the_white_tiger.active&"
        "cooldown.serenity.remains>30|fight_remains<20" );
  else
    cd_serenity->add_action(
        "variable,name=serenity_burst,op=set,value=cooldown.serenity.remains<1|cooldown.serenity.remains>30|fight_"
        "remains<20" );
  cd_serenity->add_action( p, "Invoke Xuen, the White Tiger", "if=!variable.hold_xuen|fight_remains<25" );

  // Serenity Racials
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] == "ancestral_call" )
      cd_serenity->add_action( racial_actions[ i ] + ",if=variable.serenity_burst" );
    else if ( racial_actions[ i ] == "blood_fury" )
      cd_serenity->add_action( racial_actions[ i ] + ",if=variable.serenity_burst" );
    else if ( racial_actions[ i ] == "fireblood" )
      cd_serenity->add_action( racial_actions[ i ] + ",if=variable.serenity_burst" );
    else if ( racial_actions[ i ] == "berserking" )
      cd_serenity->add_action( racial_actions[ i ] + ",if=variable.serenity_burst" );
    else if ( racial_actions[ i ] == "bag_of_tricks" )
      cd_serenity->add_action( racial_actions[ i ] + ",if=variable.serenity_burst" );
    else if ( racial_actions[ i ] != "arcane_torrent" )
      cd_serenity->add_action( racial_actions[ i ] );
  }

  if ( monk->spec.invoke_xuen->ok() )
  {
    cd_serenity->add_action(
        p, "Touch of Death",
        "if=fight_remains>(180-runeforge.fatal_touch*120)|pet.xuen_the_white_tiger.active|fight_remains<10" );
    cd_serenity->add_action( p, "Touch of Karma",
                             "if=fight_remains>90|pet.xuen_the_white_tiger.active|fight_remains<10" );
  }
  else
  {
    cd_serenity->add_action( p, "Touch of Death", "if=fight_remains>(180-runeforge.fatal_touch*120)|fight_remains<10" );
    cd_serenity->add_action( p, "Touch of Karma", "if=fight_remains>90|fight_remains<10" );
  }

  // Serenity Covenant Abilities
  cd_serenity->add_action( "weapons_of_order,if=cooldown.rising_sun_kick.remains<execute_time" );

  // Serenity On-use items
  for ( size_t i = 0; i < p->items.size(); i++ )
  {
    std::string name_str;
    if ( p->items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
    {
      if ( p->items[ i ].name_str == "inscrutable_quantum_device" )
        cd_serenity->add_action( "use_item,name=" + p->items[ i ].name_str + ",if=variable.serenity_burst" );
      else if ( p->items[ i ].name_str == "wrathstone" )
        cd_serenity->add_action( "use_item,name=" + p->items[ i ].name_str + ",if=variable.serenity_burst" );
      else if ( p->items[ i ].name_str == "gladiators_badge" )
        cd_serenity->add_action( "use_item,name=" + p->items[ i ].name_str + ",if=variable.serenity_burst" );
      else
        cd_serenity->add_action(
            "use_item,name=" + p->items[ i ].name_str +
            ",if=!variable.xuen_on_use_trinket|cooldown.invoke_xuen_the_white_tiger.remains>20|variable.hold_xuen" );
    }
  }

  cd_serenity->add_action( "faeline_stomp" );
  cd_serenity->add_action( "fallen_order" );
  cd_serenity->add_action( "bonedust_brew" );

  cd_serenity->add_talent( p, "Serenity", "if=cooldown.rising_sun_kick.remains<2|fight_remains<15" );
  cd_serenity->add_action( "bag_of_tricks" );

  // Storm, Earth and Fire Cooldowns
  cd_sef->add_action( p, "Invoke Xuen, the White Tiger", "if=!variable.hold_xuen|fight_remains<25" );

  if ( monk->spec.invoke_xuen->ok() )
    cd_sef->add_action( p, "Touch of Death",
                        "if=fight_remains>(180-runeforge.fatal_touch*120)|buff.storm_earth_and_fire.down&pet.xuen_the_"
                        "white_tiger.active|fight_remains<10" );
  else
    cd_sef->add_action(
        p, "Touch of Death",
        "if=fight_remains>(180-runeforge.fatal_touch*120)|buff.storm_earth_and_fire.down|fight_remains<10" );

  // Storm, Earth, and Fire Covenant Abilities
  cd_sef->add_action(
      "weapons_of_order,if=(raid_event.adds.in>45|raid_event.adds.up)&cooldown.rising_sun_kick.remains<execute_time" );
  cd_sef->add_action( "faeline_stomp,if=combo_strike&(raid_event.adds.in>10|raid_event.adds.up)" );
  cd_sef->add_action( "fallen_order,if=raid_event.adds.in>30|raid_event.adds.up" );
  cd_sef->add_action( "bonedust_brew,if=raid_event.adds.in>50|raid_event.adds.up,line_cd=60" );

  cd_sef->add_action( "storm_earth_and_fire_fixate,if=conduit.coordinated_offensive.enabled" );
  cd_sef->add_action(
      p, "Storm, Earth, and Fire",
      "if=cooldown.storm_earth_and_fire.charges=2|fight_remains<20|(raid_event.adds.remains>15|!covenant.kyrian&"
      "((raid_event.adds.in>cooldown.storm_earth_and_fire.full_recharge_time|!raid_event.adds.exists)&"
      "(cooldown.invoke_xuen_the_white_tiger.remains>cooldown.storm_earth_and_fire.full_recharge_time|variable.hold_"
      "xuen))&"
      "cooldown.fists_of_fury.remains<=9&chi>=2&cooldown.whirling_dragon_punch.remains<=12)" );
  cd_sef->add_action( p, "Storm, Earth, and Fire",
                      "if=covenant.kyrian&(buff.weapons_of_order.up|(fight_remains<cooldown.weapons_of_order.remains|"
                      "cooldown.weapons_of_order.remains>cooldown.storm_earth_and_fire.full_recharge_time)&cooldown."
                      "fists_of_fury.remains<=9&chi>=2&cooldown.whirling_dragon_punch.remains<=12)" );

  // Storm, Earth, and Fire on-use trinkets
  for ( size_t i = 0; i < p->items.size(); i++ )
  {
    if ( p->items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
    {
      if ( p->items[ i ].name_str == "inscrutable_quantum_device" )
        cd_sef->add_action( "use_item,name=" + p->items[ i ].name_str + 
                            ",if=pet.xuen_the_white_tiger.active|fight_remains<20" );
      else if ( p->items[ i ].name_str == "wrathstone" )
        cd_sef->add_action( "use_item,name=" + p->items[ i ].name_str +
                            ",if=pet.xuen_the_white_tiger.active|fight_remains<20" );
      else if ( p->items[ i ].name_str == "gladiators_badge" )
        cd_sef->add_action( "use_item,name=" + p->items[ i ].name_str +
                            ",if=cooldown.invoke_xuen_the_white_tiger.remains>30|variable.hold_xuen|fight_remains<15" );
      else
        cd_sef->add_action( "use_item,name=" + p->items[ i ].name_str +
               ",if=!variable.xuen_on_use_trinket|cooldown.invoke_xuen_the_white_tiger.remains>20|variable.hold_xuen" );
    }
  }

  if ( monk->spec.invoke_xuen->ok() )
    cd_sef->add_action( p, "Touch of Karma",
                        "if=fight_remains>159|pet.xuen_the_white_tiger.active|variable.hold_xuen" );
  else
    cd_sef->add_action( p, "Touch of Karma", "if=fight_remains>159|variable.hold_xuen" );

  // Storm, Earth and Fire Racials
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] != "arcane_torrent" )
    {
      if ( racial_actions[ i ] == "ancestral_call" )
        cd_sef->add_action( racial_actions[ i ] +
                            ",if=cooldown.invoke_xuen_the_white_tiger.remains>30|variable.hold_xuen|fight_remains<20" );
      else if ( racial_actions[ i ] == "blood_fury" )
        cd_sef->add_action( racial_actions[ i ] +
                            ",if=cooldown.invoke_xuen_the_white_tiger.remains>30|variable.hold_xuen|fight_remains<20" );
      else if ( racial_actions[ i ] == "fireblood" )
        cd_sef->add_action( racial_actions[ i ] +
                            ",if=cooldown.invoke_xuen_the_white_tiger.remains>30|variable.hold_xuen|fight_remains<10" );
      else if ( racial_actions[ i ] == "berserking" )
        cd_sef->add_action( racial_actions[ i ] +
                            ",if=cooldown.invoke_xuen_the_white_tiger.remains>30|variable.hold_xuen|fight_remains<15" );
      else if ( racial_actions[ i ] == "bag_of_tricks" )
        cd_sef->add_action( racial_actions[ i ] + ",if=buff.storm_earth_and_fire.down" );
      else
        cd_sef->add_action( racial_actions[ i ] );
    }
  }

  // Serenity
  serenity->add_action( p, "Fists of Fury", "if=buff.serenity.remains<1" );
  serenity->add_action( p, "Spinning Crane Kick",
                        "if=combo_strike&(active_enemies>=3|active_enemies>1&!cooldown.rising_sun_kick.up)" );
  serenity->add_action( p, "Rising Sun Kick", "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike" );
  serenity->add_action( p, "Fists of Fury", "if=active_enemies>=3" );
  serenity->add_action( p, "Spinning Crane Kick", "if=combo_strike&buff.dance_of_chiji.up" );
  serenity->add_action( p, "Blackout Kick",
                        "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&buff.weapons_of_order.up&cooldown.rising_sun_kick.remains>2" );
  serenity->add_action( p, "Fists of Fury", "interrupt_if=!cooldown.rising_sun_kick.up" );
  serenity->add_action( p, "Spinning Crane Kick", "if=combo_strike&debuff.bonedust_brew.up" );
  serenity->add_talent( p, "Fist of the White Tiger", "target_if=min:debuff.mark_of_the_crane.remains,if=chi<3" );
  serenity->add_action( p, "Blackout Kick",
                        "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike|!talent.hit_combo" );
  serenity->add_action( p, "Spinning Crane Kick" );

  // Weapons of Order
  weapons_of_order->add_action( "variable,name=blackout_kick_needed,op=set,value=buff.weapons_of_order_ww.remains&"
      "(cooldown.rising_sun_kick.remains>buff.weapons_of_order_ww.remains&buff.weapons_of_order_ww.remains<2|"
      "cooldown.rising_sun_kick.remains-buff.weapons_of_order_ww.remains>2&buff.weapons_of_order_ww.remains<4)" );
  weapons_of_order->add_action( "call_action_list,name=cd_sef,if=!talent.serenity" );
  weapons_of_order->add_action( "call_action_list,name=cd_serenity,if=talent.serenity" );
  weapons_of_order->add_talent( p, "Energizing Elixir", "if=chi.max-chi>=2&energy.time_to_max>3" );
  weapons_of_order->add_action( p, "Rising Sun Kick", "target_if=min:debuff.mark_of_the_crane.remains" );
  weapons_of_order->add_action( p, "Fists of Fury", "if=active_enemies>=2&buff.weapons_of_order_ww.remains<1" );
  weapons_of_order->add_talent( p, "Whirling Dragon Punch", "if=active_enemies>=2" );
  weapons_of_order->add_action( p, "Spinning Crane Kick",
                                "if=combo_strike&active_enemies>=3&buff.weapons_of_order_ww.up" );
  weapons_of_order->add_action( p, "Blackout Kick",
                                "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&active_enemies<=2&variable.blackout_kick_needed" );
  weapons_of_order->add_action( p, "Spinning Crane Kick",
                                "if=combo_strike&buff.dance_of_chiji.up" );
  weapons_of_order->add_talent( p, "Whirling Dragon Punch" );
  weapons_of_order->add_action( p, "Fists of Fury",
      "interrupt=1,if=buff.storm_earth_and_fire.up&raid_event.adds.in>cooldown.fists_of_fury.duration*0.6" );
  weapons_of_order->add_action( p, "Spinning Crane Kick", "if=buff.chi_energy.stack>30-5*active_enemies" );
  weapons_of_order->add_talent( p, "Fist of the White Tiger",
                                "target_if=min:debuff.mark_of_the_crane.remains,if=chi<3" );
  weapons_of_order->add_action( p, "Expel Harm", "if=chi.max-chi>=1" );
  weapons_of_order->add_talent( p, "Chi Burst", "if=chi.max-chi>=(1+active_enemies>1)" );
  weapons_of_order->add_action( p, "Tiger Palm",
                                "target_if=min:debuff.mark_of_the_crane.remains+(debuff.recently_rushing_tiger_palm.up*"
                                "20),if=(!talent.hit_combo|combo_strike)&chi.max-chi>=2" );
  weapons_of_order->add_action( p, "Blackout Kick",
                                "target_if=min:debuff.mark_of_the_crane.remains,if=active_enemies<=3&chi>=3|buff.weapons_of_order_ww.up" );
  weapons_of_order->add_talent( p, "Chi Wave" );
  weapons_of_order->add_action( p, "Flying Serpent Kick", "interrupt=1" );

  // Single Target
  st->add_talent( p, "Whirling Dragon Punch",
                  "if=raid_event.adds.in>cooldown.whirling_dragon_punch.duration*0.8|raid_event.adds.up" );
  st->add_talent(
      p, "Energizing Elixir",
      "if=chi.max-chi>=2&energy.time_to_max>3|chi.max-chi>=4&(energy.time_to_max>2|!prev_gcd.1.tiger_palm)" );
  st->add_action(
      p, "Spinning Crane Kick",
      "if=combo_strike&buff.dance_of_chiji.up&(raid_event.adds.in>buff.dance_of_chiji.remains-2|raid_event.adds.up)" );
  st->add_action( p, "Rising Sun Kick",
                  "target_if=min:debuff.mark_of_the_crane.remains,if=cooldown.serenity.remains>1|!talent.serenity" );
  st->add_action(
      p, "Fists of Fury",
      "if=(raid_event.adds.in>cooldown.fists_of_fury.duration*0.8|raid_event.adds.up)&(energy.time_to_max>execute_time-"
      "1|chi.max-chi<=1|buff.storm_earth_and_fire.remains<execute_time+1)|fight_remains<execute_time+1" );
  st->add_action( p, "Crackling Jade Lightning",
                  "if=buff.the_emperors_capacitor.stack>19&energy.time_to_max>execute_time-1&cooldown.rising_sun_kick."
                  "remains>execute_time|buff.the_emperors_capacitor.stack>14&(cooldown.serenity.remains<5&talent."
                  "serenity|cooldown.weapons_of_order.remains<5&covenant.kyrian|fight_remains<5)" );
  st->add_talent( p, "Rushing Jade Wind", "if=buff.rushing_jade_wind.down&active_enemies>1" );
  st->add_talent( p, "Fist of the White Tiger", "target_if=min:debuff.mark_of_the_crane.remains,if=chi<3" );
  st->add_action( p, "Expel Harm", "if=chi.max-chi>=1" );
  st->add_talent( p, "Chi Burst",
                  "if=chi.max-chi>=1&active_enemies=1&raid_event.adds.in>20|chi.max-chi>=2&active_enemies>=2" );
  st->add_talent( p, "Chi Wave" );
  st->add_action( p, "Tiger Palm",
                  "target_if=min:debuff.mark_of_the_crane.remains+(debuff.recently_rushing_tiger_palm.up*20),if=combo_"
                  "strike&chi.max-chi>=2&buff.storm_earth_and_fire.down" );
  st->add_action( p, "Spinning Crane Kick",
                  "if=buff.chi_energy.stack>30-5*active_enemies&buff.storm_earth_and_fire.down&(cooldown.rising_sun_"
                  "kick.remains>2&cooldown.fists_of_fury.remains>2|cooldown.rising_sun_kick.remains<3&cooldown.fists_"
                  "of_fury.remains>3&chi>3|cooldown.rising_sun_kick.remains>3&cooldown.fists_of_fury.remains<3&chi>4|"
                  "chi.max-chi<=1&energy.time_to_max<2)|buff.chi_energy.stack>10&fight_remains<7" );
  st->add_action( p, "Blackout Kick",
                  "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&(talent.serenity&cooldown.serenity."
                  "remains<3|cooldown.rising_sun_kick.remains>1&cooldown.fists_of_fury.remains>1|cooldown.rising_sun_"
                  "kick.remains<3&cooldown.fists_of_fury.remains>3&chi>2|cooldown.rising_sun_kick.remains>3&cooldown."
                  "fists_of_fury.remains<3&chi>3|chi>5|buff.bok_proc.up)" );
  st->add_action( p, "Tiger Palm",
                  "target_if=min:debuff.mark_of_the_crane.remains+(debuff.recently_rushing_tiger_palm.up*20),if=combo_"
                  "strike&chi.max-chi>=2" );
  st->add_action( "arcane_torrent,if=chi.max-chi>=1" );
  st->add_action( p, "Flying Serpent Kick", "interrupt=1" );
  st->add_action( p, "Blackout Kick",
                  "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&cooldown.fists_of_fury.remains<3&chi="
                  "2&prev_gcd.1.tiger_palm&energy.time_to_50<1" );
  st->add_action( p, "Blackout Kick",
                  "target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&energy.time_to_max<2&(chi.max-chi<=1|"
                  "prev_gcd.1.tiger_palm)" );
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
