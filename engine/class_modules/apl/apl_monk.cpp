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
        if ( p->true_level > 60 )
          return "elemental_potion_of_ultimate_power_3";
        else if ( p->true_level > 50 )
          return "phantom_fire";
        else if ( p->true_level > 45 )
          return "unbridled_fury";
        else
          return "disabled";
        break;
      case MONK_MISTWEAVER:
        if ( p->true_level > 60 )
          return "elemental_potion_of_ultimate_power_3";
        else if ( p->true_level > 50 )
          return "potion_of_spectral_intellect";
        else if ( p->true_level > 45 )
          return "superior_battle_potion_of_intellect";
        else
          return "disabled";
        break;
      case MONK_WINDWALKER:
        if ( p->true_level > 60 )
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
        if ( p->true_level > 60 )
          return "iced_phial_of_corrupting_rage_3";
        else if ( p->true_level > 50 )
          return "spectral_flask_of_power";
        else if ( p->true_level > 45 )
          return "currents";
        else
          return "disabled";
        break;
      case MONK_MISTWEAVER:
        if ( p->true_level > 60 )
          return "phial_of_elemental_chaos_3";
        else if ( p->true_level > 50 )
          return "spectral_flask_of_power";
        else if ( p->true_level > 45 )
          return "greater_flask_of_endless_fathoms";
        else
          return "disabled";
        break;
      case MONK_WINDWALKER:
        if ( p->true_level > 60 )
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
        if ( p->true_level > 60 )
          return "fated_fortune_cookie";
        else if ( p->true_level > 50 )
          return "spinefin_souffle_and_fries";
        else if ( p->true_level > 45 )
          return "biltong";
        else
          return "disabled";
        break;
      case MONK_MISTWEAVER:
        if ( p->true_level > 60 )
          return "roast_duck_delight";
        else if ( p->true_level > 50 )
          return "feast_of_gluttonous_hedonism";
        else if ( p->true_level > 45 )
          return "famine_evaluator_and_snack_table";
        else
          return "disabled";
        break;
      case MONK_WINDWALKER:
        if ( p->true_level > 60 )
          return "fated_fortune_cookie";
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
    if ( p->true_level > 60 )
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
        if ( p->true_level > 60 )
          return "main_hand:buzzing_rune_3/off_hand:buzzing_rune_3";
        else if ( p->true_level > 50 )
          return "main_hand:shadowcore_oil/off_hand:shadowcore_oil";
        else
          return "disabled";
        break;
      case MONK_MISTWEAVER:
        if ( p->true_level > 60 )
          return "main_hand:howling_rune_3";
        else if ( p->true_level > 50 )
          return "main_hand:shadowcore_oil";
        else
          return "disabled";
        break;
      case MONK_WINDWALKER:
        if ( p->true_level > 60 )
          return "main_hand:buzzing_rune_3/off_hand:buzzing_rune_3";
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
    const static std::unordered_map<std::string, std::string> trinkets {
      // trinket_name_slot_name -> trailing fragment
      // slot_names util/util.cpp @ slot_type_string: head, neck, shoulders, shirt, chest, waist, legs, feet, wrists, hands, finger1, finger2, trinket1, trinket2, back, main_hand, off_hand, tabard

      // 10.0
      // { "manic_grieftorch", ",use_off_gcd=1,if=debuff.weapons_of_order_debuff.stacks>3|fight_remains<5" },
      // { "windscarred_whetstone", ",if=debuff.weapons_of_order_debuff.stacks>3|fight_remains<25" },

      // Defaults by slot:
      { "trinket1", "" },
      { "trinket2", "" },
      { "main_hand", "" },
    };
    const static std::unordered_map<std::string, std::string> racials {
      // name_str -> tail
      // Default:
      { "DEFAULT", "" },
    };

    auto _BRM_ON_USE = [] ( const item_t &item )
    {
      std::string concat = "";
      try
      {
        concat = trinkets.at( item.name_str + '_' + item.slot_name() );
      }
      catch ( ... )
      {
        int duration = 0;

        for ( auto e : item.parsed.special_effects )
        {
          duration = ( int )floor( e->duration().total_seconds() );

          // Ignore items that have a 30 second or shorter cooldown (or no cooldown)
          // Unless defined in the map above these will be used on cooldown.
          if ( e->type == SPECIAL_EFFECT_USE && e->cooldown() > timespan_t::from_seconds( 30 ) )
            try {
              concat = trinkets.at( item.slot_name() );
            }
            catch ( ... )
            {
              concat = ",if=";
            }
        }

        if ( concat.length() > 0 && duration > 0 )
        {
          if ( concat[concat.length()] != '|' )
            concat = concat + '|';
          concat = concat + "fight_remains<" + std::to_string( duration );
        }
      }

      return concat;
    };

    auto _BRM_RACIALS = [] ( const std::string &racial_action )
    {
      std::string concat = "";
      try
      {
        concat = racials.at( racial_action );
      }
      catch ( ... )
      {
        concat = racials.at( "DEFAULT" );
      }

      return concat;
    };

    std::vector<std::string> racial_actions = p->get_racial_actions();
    action_priority_list_t *pre = p->get_action_priority_list( "precombat" );
    action_priority_list_t *def = p->get_action_priority_list( "default" );

    action_priority_list_t *item_actions = p->get_action_priority_list( "item_actions" );
    action_priority_list_t *race_actions = p->get_action_priority_list( "race_actions" );

    action_priority_list_t *rotation_pta = p->get_action_priority_list( "rotation_pta" );
    action_priority_list_t *rotation_boc = p->get_action_priority_list( "rotation_boc" );

    pre->add_action( "flask" );
    pre->add_action( "food" );
    pre->add_action( "augmentation" );
    pre->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
    pre->add_action( "potion" );
    pre->add_action( "chi_burst,if=talent.chi_burst.enabled" );
    pre->add_action( "chi_wave,if=talent.chi_wave.enabled" );
    pre->add_action( "summon_white_tiger_statue,if=talent.summon_white_tiger_statue.enabled");

    def->add_action( "auto_attack" );
    def->add_action( "roll,if=movement.distance>5", "Move to target" );
    def->add_action( "chi_torpedo,if=movement.distance>5" );
    def->add_action( "spear_hand_strike,if=target.debuff.casting.react" );
    def->add_action( "potion" );
    def->add_action( "invoke_external_buff,name=power_infusion,if=buff.weapons_of_order.remains<=20&talent.weapons_of_order.enabled",
                     "Use <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> when <a href='https://www.wowhead.com/spell=387184/weapons-of-order'>Weapons of Order</a> reaches 4 stacks." );
    def->add_action( "invoke_external_buff,name=power_infusion,if=!talent.weapons_of_order.enabled",
                     "Use <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> when <a href='https://www.wowhead.com/spell=387184/weapons-of-order'>Weapons of Order</a> reaches 4 stacks." );
    def->add_action( "call_action_list,name=item_actions" );
    def->add_action( "call_action_list,name=race_actions" );
    def->add_action( "call_action_list,name=rotation_pta,if=talent.press_the_advantage.enabled" );
    def->add_action( "call_action_list,name=rotation_boc,if=!talent.press_the_advantage.enabled" );

    for ( const auto &item : p->items )
    {
      if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
        item_actions->add_action( "use_item,name=" + item.name_str + _BRM_ON_USE( item ) );
    }

    for ( const auto &racial_action : racial_actions )
      race_actions->add_action( racial_action + _BRM_RACIALS( racial_action ));

    rotation_pta->add_action( "invoke_niuzao_the_black_ox" );
    rotation_pta->add_action( "rising_sun_kick,if=buff.press_the_advantage.stack<(7+main_hand.2h)" );
    rotation_pta->add_action( "rising_sun_kick,if=buff.press_the_advantage.stack>9&active_enemies<=3&(buff.blackout_combo.up|!talent.blackout_combo.enabled)" );
    rotation_pta->add_action( "keg_smash,if=(buff.press_the_advantage.stack>9)&active_enemies>3" );
    rotation_pta->add_action( "spinning_crane_kick,if=active_enemies>5&buff.exploding_keg.up&buff.charred_passions.up" );
    rotation_pta->add_action( "blackout_kick" );
    rotation_pta->add_action( "purifying_brew,if=(!buff.blackout_combo.up)" );
    rotation_pta->add_action( "black_ox_brew,if=energy+energy.regen<=40" );
    rotation_pta->add_action( "breath_of_fire,if=buff.charred_passions.remains<cooldown.blackout_kick.remains&(buff.blackout_combo.up|!talent.blackout_combo.enabled)" );
    rotation_pta->add_action( "summon_white_tiger_statue" );
    rotation_pta->add_action( "bonedust_brew" );
    rotation_pta->add_action( "exploding_keg,if=((buff.bonedust_brew.up)|(cooldown.bonedust_brew.remains>=20))" );
    rotation_pta->add_action( "exploding_keg,if=(!talent.bonedust_brew.enabled)" );
    rotation_pta->add_action( "breath_of_fire,if=(buff.blackout_combo.up|!talent.blackout_combo.enabled)" );
    rotation_pta->add_action( "keg_smash,if=buff.press_the_advantage.stack<10" );
    rotation_pta->add_action( "rushing_jade_wind,if=talent.rushing_jade_wind.enabled" );
    rotation_pta->add_action( "spinning_crane_kick,if=active_enemies>2" );
    rotation_pta->add_action( "spinning_crane_kick,if=(1.1>(time-action.melee_main_hand.last_used)*(1+spell_haste)-main_hand.2h)" );
    rotation_pta->add_action( "expel_harm" );
    rotation_pta->add_action( "chi_wave" );
    rotation_pta->add_action( "chi_burst" );

    rotation_boc->add_action( "blackout_kick" );
    rotation_boc->add_action( "purifying_brew,if=(buff.blackout_combo.down&(buff.recent_purifies.down|cooldown.purifying_brew.charges_fractional>(1+talent.improved_purifying_brew.enabled-0.1)))&talent.improved_invoke_niuzao_the_black_ox.enabled&(cooldown.weapons_of_order.remains>40|cooldown.weapons_of_order.remains<5)" );
    rotation_boc->add_action( "weapons_of_order,if=(buff.recent_purifies.up)&talent.improved_invoke_niuzao_the_black_ox.enabled" );
    rotation_boc->add_action( "invoke_niuzao_the_black_ox,if=(buff.invoke_niuzao_the_black_ox.down&buff.recent_purifies.up&buff.weapons_of_order.remains<14)&talent.improved_invoke_niuzao_the_black_ox.enabled" );
    rotation_boc->add_action( "invoke_niuzao_the_black_ox,if=(debuff.weapons_of_order_debuff.stack>3)&!talent.improved_invoke_niuzao_the_black_ox.enabled" );
    rotation_boc->add_action( "invoke_niuzao_the_black_ox,if=(!talent.weapons_of_order.enabled)" );
    rotation_boc->add_action( "weapons_of_order,if=(talent.weapons_of_order.enabled)&!talent.improved_invoke_niuzao_the_black_ox.enabled" );
    rotation_boc->add_action( "keg_smash,if=(time-action.weapons_of_order.last_used<2)" );
    rotation_boc->add_action( "keg_smash,if=(buff.weapons_of_order.remains<gcd*2&buff.weapons_of_order.up)&!talent.improved_invoke_niuzao_the_black_ox.enabled" );
    rotation_boc->add_action( "keg_smash,if=(buff.weapons_of_order.remains<gcd*2)&talent.improved_invoke_niuzao_the_black_ox.enabled" );
    rotation_boc->add_action( "purifying_brew,if=(!buff.blackout_combo.up)&!talent.improved_invoke_niuzao_the_black_ox.enabled" );
    rotation_boc->add_action( "rising_sun_kick" );
    rotation_boc->add_action( "black_ox_brew,if=(energy+energy.regen<=40)" );
    rotation_boc->add_action( "tiger_palm,if=(buff.blackout_combo.up&active_enemies=1)" );
    rotation_boc->add_action( "breath_of_fire,if=(buff.charred_passions.remains<cooldown.blackout_kick.remains)" );
    rotation_boc->add_action( "keg_smash,if=(buff.weapons_of_order.up&debuff.weapons_of_order_debuff.stack<=3)" );
    rotation_boc->add_action( "summon_white_tiger_statue,if=(debuff.weapons_of_order_debuff.stack>3)" );
    rotation_boc->add_action( "summon_white_tiger_statue,if=(!talent.weapons_of_order.enabled)" );
    rotation_boc->add_action( "bonedust_brew,if=(time<10&debuff.weapons_of_order_debuff.stack>3)|(time>10&talent.weapons_of_order.enabled)" );
    rotation_boc->add_action( "bonedust_brew,if=(!talent.weapons_of_order.enabled)" );
    rotation_boc->add_action( "exploding_keg,if=(buff.bonedust_brew.up)" );
    rotation_boc->add_action( "exploding_keg,if=(cooldown.bonedust_brew.remains>=20)" );
    rotation_boc->add_action( "exploding_keg,if=(!talent.bonedust_brew.enabled)" );
    rotation_boc->add_action( "keg_smash" );
    rotation_boc->add_action( "rushing_jade_wind,if=talent.rushing_jade_wind.enabled" );
    rotation_boc->add_action( "breath_of_fire" );
    rotation_boc->add_action( "tiger_palm,if=active_enemies=1&!talent.blackout_combo.enabled" );
    rotation_boc->add_action( "spinning_crane_kick,if=active_enemies>1" );
    rotation_boc->add_action( "expel_harm" );
    rotation_boc->add_action( "chi_wave" );
    rotation_boc->add_action( "chi_burst" );
  }

  void mistweaver( player_t *p )
  {
    action_priority_list_t *pre = p->get_action_priority_list( "precombat" );

    // Flask
    pre->add_action( "flask" );

    // Food
    pre->add_action( "food" );

    // Rune
    pre->add_action( "augmentation" );

    // Snapshot stats
    pre->add_action( "snapshot_stats" );

    pre->add_action( "potion" );

    pre->add_action( p, "chi_burst" );
    pre->add_action( p, "chi_wave" );

    std::vector<std::string> racial_actions = p->get_racial_actions();
    action_priority_list_t *def = p->get_action_priority_list( "default" );
    action_priority_list_t *st = p->get_action_priority_list( "st" );
    action_priority_list_t *aoe = p->get_action_priority_list( "aoe" );

    def->add_action( "auto_attack" );
    def->add_action( "roll,if=movement.distance>5", "Move to target" );
    def->add_action( "chi_torpedo,if=movement.distance>5" );
    def->add_action( "spear_hand_strike,if=target.debuff.casting.react" );

    if ( p->items[SLOT_MAIN_HAND].name_str == "jotungeirr_destinys_call" )
      def->add_action( "use_item,name=" + p->items[SLOT_MAIN_HAND].name_str );

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
    auto monk = debug_cast< monk::monk_t * >( p );

    //============================================================================
    // On-use Items
    //============================================================================
    auto _WW_ON_USE = [ monk ] ( const item_t &item )
    {
      //-------------------------------------------
      // Serenity item map
      //-------------------------------------------
      const static std::unordered_map<std::string, std::string> serenity_trinkets {
        // name_str -> APL
        { "algethar_puzzle_box", ",use_off_gcd=1,if=(pet.xuen_the_white_tiger.active|!talent.invoke_xuen_the_white_tiger)&!buff.serenity.up|fight_remains<25" },
        { "erupting_spear_fragment", ",if=buff.serenity.up|(buff.invokers_delight.up&!talent.serenity)" },
        { "manic_grieftorch", ",use_off_gcd=1,if=!trinket.1.has_use_buff&!trinket.2.has_use_buff&!buff.serenity.up&!pet.xuen_the_white_tiger.active|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>30&cooldown.serenity.remains|fight_remains<5" },
        { "beacon_to_the_beyond", ",use_off_gcd=1,if=!trinket.1.has_use_buff&!trinket.2.has_use_buff&!buff.serenity.up&!pet.xuen_the_white_tiger.active|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>30&cooldown.serenity.remains|fight_remains<10" },
        { "djaruun_pillar_of_the_elder_flame", ",if=cooldown.fists_of_fury.remains<2&cooldown.invoke_xuen_the_white_tiger.remains>10|fight_remains<12" },
        { "dragonfire_bomb_dispenser", ",if=!trinket.1.has_use_buff&!trinket.2.has_use_buff|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>10&cooldown.serenity.remains|fight_remains<10" },

        // Defaults:
        { "ITEM_STAT_BUFF", ",if=(pet.xuen_the_white_tiger.active|!talent.invoke_xuen_the_white_tiger)&buff.serenity.up|fight_remains<25" },
        { "ITEM_DMG_BUFF", ",if=!trinket.1.has_use_buff&!trinket.2.has_use_buff|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>30&cooldown.serenity.remains" },
      };

      //-------------------------------------------
      // SEF item map
      //-------------------------------------------
      const static std::unordered_map<std::string, std::string> sef_trinkets {
        // name_str -> APL
        { "algethar_puzzle_box", ",use_off_gcd=1,if=(pet.xuen_the_white_tiger.active|!talent.invoke_xuen_the_white_tiger)&!buff.storm_earth_and_fire.up|fight_remains<25" },
        { "erupting_spear_fragment", ",if=buff.serenity.up|(buff.invokers_delight.up&!talent.serenity)" },
        { "manic_grieftorch", ",use_off_gcd=1,if=!trinket.1.has_use_buff&!trinket.2.has_use_buff&!buff.storm_earth_and_fire.up&!pet.xuen_the_white_tiger.active|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>30|fight_remains<5" },
        { "beacon_to_the_beyond", ",use_off_gcd=1,if=!trinket.1.has_use_buff&!trinket.2.has_use_buff&!buff.storm_earth_and_fire.up&!pet.xuen_the_white_tiger.active|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>30|fight_remains<10" },
        { "djaruun_pillar_of_the_elder_flame", ",if=cooldown.fists_of_fury.remains<2&cooldown.invoke_xuen_the_white_tiger.remains>10|fight_remains<12" },
        { "dragonfire_bomb_dispenser", ",if=!trinket.1.has_use_buff&!trinket.2.has_use_buff|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>10|fight_remains<10" },

        // Defaults:
        { "ITEM_STAT_BUFF", ",if=(pet.xuen_the_white_tiger.active|!talent.invoke_xuen_the_white_tiger)&buff.storm_earth_and_fire.up|fight_remains<25" },
        { "ITEM_DMG_BUFF", ",if=!trinket.1.has_use_buff&!trinket.2.has_use_buff|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>30" },
      };

      // -----------------------------------------

      std::string concat = "";
      auto talent_map = monk->talent.windwalker.serenity->ok() ? serenity_trinkets : sef_trinkets;
      try
      {
        concat = talent_map.at( item.name_str );
      }
      catch ( ... )
      {
        int duration = 0;

        for ( auto e : item.parsed.special_effects )
        {
          duration = ( int )floor( e->duration().total_seconds() );

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

    // Flask
    pre->add_action( "flask" );
    // Food
    pre->add_action( "food" );
    // Rune
    pre->add_action( "augmentation" );

    // Snapshot stats
    pre->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
    pre->add_action( "variable,name=sync_serenity,default=0,value=1,if=(equipped.neltharions_call_to_dominance|equipped.ashes_of_the_embersoul|equipped.mirror_of_fractured_tomorrows|equipped.witherbarks_branch)&!(fight_style.dungeonslice|fight_style.dungeonroute)" );

    pre->add_action( "summon_white_tiger_statue" );
    pre->add_action( "expel_harm,if=chi<chi.max" );
    pre->add_action( "chi_burst,if=!talent.jadefire_stomp" );
    pre->add_action( "chi_wave" );

    std::vector<std::string> racial_actions = p->get_racial_actions();
    action_priority_list_t *def = p->get_action_priority_list( "default" );
    action_priority_list_t *opener = p->get_action_priority_list( "opener" );
    action_priority_list_t *bdb_setup = p->get_action_priority_list( "bdb_setup" );
    action_priority_list_t *trinkets = p->get_action_priority_list( "trinkets" );
    action_priority_list_t *cd_sef = p->get_action_priority_list( "cd_sef" );
    action_priority_list_t *cd_serenity = p->get_action_priority_list( "cd_serenity" );
    action_priority_list_t *serenity_aoelust = p->get_action_priority_list( "serenity_aoelust" );
    action_priority_list_t *serenity_lust = p->get_action_priority_list( "serenity_lust" );
    action_priority_list_t *serenity_aoe = p->get_action_priority_list( "serenity_aoe" );
    action_priority_list_t *serenity_4t = p->get_action_priority_list( "serenity_4t" );
    action_priority_list_t *serenity_3t = p->get_action_priority_list( "serenity_3t" );
    action_priority_list_t *serenity_2t = p->get_action_priority_list( "serenity_2t" );
    action_priority_list_t *serenity_st = p->get_action_priority_list( "serenity_st" );
    action_priority_list_t *default_aoe = p->get_action_priority_list( "default_aoe" );
    action_priority_list_t *default_4t = p->get_action_priority_list( "default_4t" );
    action_priority_list_t *default_3t = p->get_action_priority_list( "default_3t" );
    action_priority_list_t *default_2t = p->get_action_priority_list( "default_2t" );
    action_priority_list_t *default_st = p->get_action_priority_list( "default_st" );
    action_priority_list_t *fallthru = p->get_action_priority_list( "fallthru" );

    def->add_action( "auto_attack" );
    def->add_action( "roll,if=movement.distance>5", "Move to target" );
    def->add_action( "chi_torpedo,if=movement.distance>5" );
    def->add_action( "flying_serpent_kick,if=movement.distance>5" );
    def->add_action( "spear_hand_strike,if=target.debuff.casting.react" );
    def->add_action( "variable,name=hold_xuen,op=set,value=!talent.invoke_xuen_the_white_tiger|cooldown.invoke_xuen_the_white_tiger.duration>fight_remains" );
    def->add_action( "variable,name=hold_tp_rsk,op=set,value=!debuff.skyreach_exhaustion.remains<1&cooldown.rising_sun_kick.remains<1&(set_bonus.tier30_2pc|active_enemies<5)" );
    def->add_action( "variable,name=hold_tp_bdb,op=set,value=!debuff.skyreach_exhaustion.remains<1&cooldown.bonedust_brew.remains<1&active_enemies=1" );

    // Potion
    if ( p->sim->allow_potions )
    {
      if ( monk->talent.windwalker.invoke_xuen_the_white_tiger->ok() )
        def->add_action(
        "potion,if=buff.serenity.up|buff.storm_earth_and_fire.up&pet.xuen_the_white_tiger.active|fight_remains<=30", "Potion" );
      else
        def->add_action( "potion,if=(buff.serenity.up|buff.storm_earth_and_fire.up)&fight_remains<=30", "Potion" );
    }

    // Build Chi at the start of combat
    if ( monk->talent.windwalker.invoke_xuen_the_white_tiger->ok() )
      def->add_action( "call_action_list,name=opener,if=time<4&chi<5&!pet.xuen_the_white_tiger.active&!talent.serenity", "Build Chi at the start of combat" );
    else
      def->add_action( "call_action_list,name=opener,if=time<4&chi<5&!talent.serenity", "Build Chi at the start of combat" );
    // Use Trinkets
    def->add_action( "call_action_list,name=trinkets", "Use Trinkets" );
    // Prioritize jadefire Stomp if playing with jadefire Harmony
    def->add_action( "jadefire_stomp,target_if=min:debuff.jadefire_brand_damage.remains,if=combo_strike&talent.jadefire_harmony&debuff.jadefire_brand_damage.remains<1", "Prioritize jadefire Stomp if playing with jadefire Harmony" );
    // BDB before skyreach in ST
    def->add_action( "bonedust_brew,if=active_enemies=1&!debuff.skyreach_exhaustion.remains&(pet.xuen_the_white_tiger.active|cooldown.xuen_the_white_tiger.remains)", "Use bdb before skyreach in st" );
    // Spend excess energy
    def->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=!buff.serenity.up&energy>50&buff.teachings_of_the_monastery.stack<3&combo_strike&chi.max-chi>=(2+buff.power_strikes.up)&(!talent.invoke_xuen_the_white_tiger&!talent.serenity|((!talent.skyreach&!talent.skytouch)|time>5|pet.xuen_the_white_tiger.active))&!variable.hold_tp_rsk&(active_enemies>1|!talent.bonedust_brew|talent.bonedust_brew&active_enemies=1&cooldown.bonedust_brew.remains)", "TP to not waste energy" );
    def->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=!buff.serenity.up&buff.teachings_of_the_monastery.stack<3&combo_strike&chi.max-chi>=(2+buff.power_strikes.up)&(!talent.invoke_xuen_the_white_tiger&!talent.serenity|((!talent.skyreach&!talent.skytouch)|time>5|pet.xuen_the_white_tiger.active))&!variable.hold_tp_rsk&(active_enemies>1|!talent.bonedust_brew|talent.bonedust_brew&active_enemies=1&cooldown.bonedust_brew.remains)", "TP if not overcapping Chi or TotM" );
    // Use Chi Burst to reset jadefire Stomp
    def->add_action( "chi_burst,if=talent.jadefire_stomp&cooldown.jadefire_stomp.remains&(chi.max-chi>=1&active_enemies=1|chi.max-chi>=2&active_enemies>=2)&!talent.jadefire_harmony", "Use Chi Burst to reset jadefire Stomp" );
    // Use Cooldowns
    def->add_action( "call_action_list,name=cd_sef,if=!talent.serenity", "Use Cooldowns" );
    def->add_action( "call_action_list,name=cd_serenity,if=talent.serenity" );
    // Serenity priority
    def->add_action( "call_action_list,name=serenity_aoelust,if=buff.serenity.up&((buff.bloodlust.up&(buff.invokers_delight.up|buff.power_infusion.up))|buff.invokers_delight.up&buff.power_infusion.up)&active_enemies>4", "Serenity Priority" );
    def->add_action( "call_action_list,name=serenity_lust,if=buff.serenity.up&((buff.bloodlust.up&(buff.invokers_delight.up|buff.power_infusion.up))|buff.invokers_delight.up&buff.power_infusion.up)&active_enemies<4" );
    def->add_action( "call_action_list,name=serenity_aoe,if=buff.serenity.up&active_enemies>4" );
    def->add_action( "call_action_list,name=serenity_4t,if=buff.serenity.up&active_enemies=4" );
    def->add_action( "call_action_list,name=serenity_3t,if=buff.serenity.up&active_enemies=3" );
    def->add_action( "call_action_list,name=serenity_2t,if=buff.serenity.up&active_enemies=2" );
    def->add_action( "call_action_list,name=serenity_st,if=buff.serenity.up&active_enemies=1" );
    // Default priority
    def->add_action( "call_action_list,name=default_aoe,if=active_enemies>4", "Default Priority" );
    def->add_action( "call_action_list,name=default_4t,if=active_enemies=4" );
    def->add_action( "call_action_list,name=default_3t,if=active_enemies=3" );
    def->add_action( "call_action_list,name=default_2t,if=active_enemies=2" );
    def->add_action( "call_action_list,name=default_st,if=active_enemies=1" );
    def->add_action( "summon_white_tiger_statue" );
    def->add_action( "call_action_list,name=fallthru" );


    // Trinkets
    for ( const auto &item : p->items )
    {
      if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
        trinkets->add_action( "use_item,name=" + item.name_str + _WW_ON_USE( item ) );
    }

    // Storm, Earth and Fire Cooldowns
    cd_sef->add_action( "invoke_external_buff,name=power_infusion,if=pet.xuen_the_white_tiger.active",
      "Use <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> while <a href='https://www.wowhead.com/spell=123904/invoke-xuen-the-white-tiger'>Invoke Xuen, the White Tiger</a> is active." );
    cd_sef->add_action( "invoke_xuen_the_white_tiger,if=!variable.hold_xuen&target.time_to_die>25&talent.bonedust_brew&cooldown.bonedust_brew.remains<=5&(active_enemies<3&chi>=3|active_enemies>=3&chi>=2)|fight_remains<25" );
    cd_sef->add_action( "invoke_xuen_the_white_tiger,if=target.time_to_die>25&fight_remains>120&(!trinket.1.is.ashes_of_the_embersoul&!trinket.1.is.witherbarks_branch&!trinket.2.is.ashes_of_the_embersoul&!trinket.2.is.witherbarks_branch|(trinket.1.is.ashes_of_the_embersoul|trinket.1.is.witherbarks_branch)&!trinket.1.cooldown.remains|(trinket.2.is.ashes_of_the_embersoul|trinket.2.is.witherbarks_branch)&!trinket.2.cooldown.remains)" );
    cd_sef->add_action( "invoke_xuen_the_white_tiger,if=fight_remains<60&(debuff.skyreach_exhaustion.remains<2|debuff.skyreach_exhaustion.remains>55)&!cooldown.serenity.remains&active_enemies<3" );
    cd_sef->add_action( "storm_earth_and_fire,if=talent.bonedust_brew&(fight_remains<30&cooldown.bonedust_brew.remains<4&chi>=4|buff.bonedust_brew.up|!spinning_crane_kick.max&active_enemies>=3&cooldown.bonedust_brew.remains<=2&chi>=2)&(pet.xuen_the_white_tiger.active|cooldown.invoke_xuen_the_white_tiger.remains>cooldown.storm_earth_and_fire.full_recharge_time)" );
    cd_sef->add_action( "storm_earth_and_fire,if=!talent.bonedust_brew&(pet.xuen_the_white_tiger.active|target.time_to_die>15&cooldown.storm_earth_and_fire.full_recharge_time<cooldown.invoke_xuen_the_white_tiger.remains)" );
    cd_sef->add_action( "bonedust_brew,if=(!buff.bonedust_brew.up&buff.storm_earth_and_fire.up&buff.storm_earth_and_fire.remains<11&spinning_crane_kick.max)|(!buff.bonedust_brew.up&fight_remains<30&fight_remains>10&spinning_crane_kick.max&chi>=4)|fight_remains<10|(!debuff.skyreach_exhaustion.up&active_enemies>=4&spinning_crane_kick.modifier>=2)|(pet.xuen_the_white_tiger.active&spinning_crane_kick.max&active_enemies>=4)" );
    cd_sef->add_action( "call_action_list,name=bdb_setup,if=!buff.bonedust_brew.up&talent.bonedust_brew&cooldown.bonedust_brew.remains<=2&(fight_remains>60&(cooldown.storm_earth_and_fire.charges>0|cooldown.storm_earth_and_fire.remains>10)&(pet.xuen_the_white_tiger.active|cooldown.invoke_xuen_the_white_tiger.remains>10|variable.hold_xuen)|((pet.xuen_the_white_tiger.active|cooldown.invoke_xuen_the_white_tiger.remains>13)&(cooldown.storm_earth_and_fire.charges>0|cooldown.storm_earth_and_fire.remains>13|buff.storm_earth_and_fire.up)))" );
    cd_sef->add_action( "storm_earth_and_fire,if=fight_remains<20|(cooldown.storm_earth_and_fire.charges=2&cooldown.invoke_xuen_the_white_tiger.remains>cooldown.storm_earth_and_fire.full_recharge_time)&cooldown.fists_of_fury.remains<=9&chi>=2&cooldown.whirling_dragon_punch.remains<=12" );
    cd_sef->add_action( "touch_of_death,target_if=max:target.health,if=fight_style.dungeonroute&!buff.serenity.up&(combo_strike&target.health<health)|(buff.hidden_masters_forbidden_touch.remains<2)|(buff.hidden_masters_forbidden_touch.remains>target.time_to_die)" );
    cd_sef->add_action( "touch_of_death,cycle_targets=1,if=fight_style.dungeonroute&combo_strike&(target.time_to_die>60|debuff.bonedust_brew_debuff.up|fight_remains<10)" );
    cd_sef->add_action( "touch_of_death,cycle_targets=1,if=!fight_style.dungeonroute&combo_strike" );

    if ( monk->talent.windwalker.invoke_xuen_the_white_tiger->ok() )
      cd_sef->add_action( "touch_of_karma,target_if=max:target.time_to_die,if=fight_remains>90|pet.xuen_the_white_tiger.active|variable.hold_xuen|fight_remains<16" );
    else
      cd_sef->add_action( "touch_of_karma,if=fight_remains>159|variable.hold_xuen" );

    // Storm, Earth and Fire Racials
    for ( const auto &racial_action : racial_actions )
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

    // Serenity Cooldowns
    cd_serenity->add_action( "invoke_external_buff,name=power_infusion,if=pet.xuen_the_white_tiger.active",
      "Use <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> while <a href='https://www.wowhead.com/spell=123904/invoke-xuen-the-white-tiger'>Invoke Xuen, the White Tiger</a> is active." );
    cd_serenity->add_action( "invoke_xuen_the_white_tiger,if=target.time_to_die>16&!variable.hold_xuen&talent.bonedust_brew&cooldown.bonedust_brew.remains<=1|buff.bloodlust.up|fight_remains<25" );
    cd_serenity->add_action( "invoke_xuen_the_white_tiger,if=(target.time_to_die>16|(fight_style.dungeonslice|fight_style.dungeonroute)&cooldown.serenity.remains<2)&fight_remains>120&(!trinket.1.is.ashes_of_the_embersoul&!trinket.1.is.witherbarks_branch&!trinket.2.is.ashes_of_the_embersoul&!trinket.2.is.witherbarks_branch|(trinket.1.is.ashes_of_the_embersoul|trinket.1.is.witherbarks_branch)&!trinket.1.cooldown.remains|(trinket.2.is.ashes_of_the_embersoul|trinket.2.is.witherbarks_branch)&!trinket.2.cooldown.remains)" );
    cd_serenity->add_action( "invoke_xuen_the_white_tiger,if=target.time_to_die>16&fight_remains<60&(debuff.skyreach_exhaustion.remains<2|debuff.skyreach_exhaustion.remains>55)&!cooldown.serenity.remains&active_enemies<3" );
    cd_serenity->add_action( "invoke_xuen_the_white_tiger,if=fight_style.dungeonslice&talent.bonedust_brew&target.time_to_die>16&!cooldown.serenity.remains&cooldown.bonedust_brew.remains<2");
    cd_serenity->add_action( "bonedust_brew,if=buff.invokers_delight.up|!buff.bonedust_brew.up&cooldown.xuen_the_white_tiger.remains&!pet.xuen_the_white_tiger.active|cooldown.serenity.remains>15|fight_remains<30&fight_remains>10|fight_remains<10" );
    cd_serenity->add_action( "serenity,if=variable.sync_serenity&(buff.invokers_delight.up|variable.hold_xuen&(talent.drinking_horn_cover&fight_remains>110|!talent.drinking_horn_cover&fight_remains>105))|!talent.invoke_xuen_the_white_tiger|fight_remains<15" );
    cd_serenity->add_action( "serenity,if=!variable.sync_serenity&(buff.invokers_delight.up|cooldown.invoke_xuen_the_white_tiger.remains>fight_remains|fight_remains>(cooldown.invoke_xuen_the_white_tiger.remains+10)&fight_remains>90)" );
    cd_serenity->add_action( "touch_of_death,target_if=max:target.health,if=fight_style.dungeonroute&!buff.serenity.up&(combo_strike&target.health<health)|(buff.hidden_masters_forbidden_touch.remains<2)|(buff.hidden_masters_forbidden_touch.remains>target.time_to_die)" );
    cd_serenity->add_action( "touch_of_death,cycle_targets=1,if=fight_style.dungeonroute&combo_strike&(target.time_to_die>60|debuff.bonedust_brew_debuff.up|fight_remains<10)&!buff.serenity.up" );
    cd_serenity->add_action( "touch_of_death,cycle_targets=1,if=!fight_style.dungeonroute&combo_strike&!buff.serenity.up" );

    cd_serenity->add_action( "touch_of_karma,if=fight_remains>90|fight_remains<10" );

    // Serenity Racials
    for ( const auto &racial_action : racial_actions )
    {
      if ( racial_action == "ancestral_call" )
        cd_serenity->add_action( racial_action + ",if=buff.serenity.up|fight_remains<20" );
      else if ( racial_action == "blood_fury" )
        cd_serenity->add_action( racial_action + ",if=buff.serenity.up|fight_remains<20" );
      else if ( racial_action == "fireblood" )
        cd_serenity->add_action( racial_action + ",if=buff.serenity.up|fight_remains<20" );
      else if ( racial_action == "berserking" )
        cd_serenity->add_action( racial_action + ",if=!buff.serenity.up|fight_remains<20" );
      else if ( racial_action == "bag_of_tricks" )
        cd_serenity->add_action( racial_action + ",if=buff.serenity.up|fight_remains<20" );
      else if ( racial_action != "arcane_torrent" )
        cd_serenity->add_action( racial_action );
    }

    // Opener
    opener->add_action( "summon_white_tiger_statue", "Opener" );
    opener->add_action( "expel_harm,if=talent.chi_burst.enabled&chi.max-chi>=3" );
    opener->add_action( "jadefire_stomp,if=debuff.jadefire_brand_damage.remains<2&!debuff.skyreach_exhaustion.remains<2&!debuff.skyreach_exhaustion.remains" );
    opener->add_action( "expel_harm,if=talent.chi_burst.enabled&chi=3" );
    opener->add_action( "chi_wave,if=chi.max-chi=2" );
    opener->add_action( "expel_harm" );
    opener->add_action( "chi_burst,if=chi>1&chi.max-chi>=2" );

    // Bonedust Brew Setup
    bdb_setup->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist&active_enemies>3", "Bonedust Brew Setup" );
    bdb_setup->add_action( "bonedust_brew,if=spinning_crane_kick.max&chi>=4" );
    bdb_setup->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&chi.max-chi>=2&buff.storm_earth_and_fire.up" );
    bdb_setup->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&!talent.whirling_dragon_punch&!spinning_crane_kick.max" );
    bdb_setup->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&chi>=5&talent.whirling_dragon_punch" );
    bdb_setup->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&active_enemies>=2&talent.whirling_dragon_punch" );

    // Serenity AoE Lust Priority
    serenity_aoelust->add_action( "jadefire_stomp,if=debuff.jadefire_brand_damage.remains<1", "Serenity AoE Lust" );
    serenity_aoelust->add_action( "strike_of_the_windlord,if=set_bonus.tier31_4pc&talent.thunderfist" );
    serenity_aoelust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&set_bonus.tier31_2pc" );
    serenity_aoelust->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains,if=talent.jade_ignition,interrupt_if=buff.chi_energy.stack=30" );
    serenity_aoelust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&!buff.blackout_reinforcement.up&set_bonus.tier31_2pc&buff.bonedust_brew.up&active_enemies>4" );
    serenity_aoelust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&set_bonus.tier31_2pc&combo_strike&!buff.blackout_reinforcement.up" );
    serenity_aoelust->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains,if=buff.invokers_delight.up&set_bonus.tier31_2pc&(active_enemies>5&buff.transfer_the_power.stack>5|active_enemies>6|active_enemies>4&!talent.crane_vortex&buff.transfer_the_power.stack>5),interrupt=1" );
    serenity_aoelust->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains,if=!set_bonus.tier30_2pc&!buff.invokers_delight.up&set_bonus.tier31_2pc&(!buff.bonedust_brew.up|active_enemies>10)&(buff.transfer_the_power.stack=10&!talent.crane_vortex|active_enemies>5&talent.crane_vortex&buff.transfer_the_power.stack=10|active_enemies>14|active_enemies>12&!talent.crane_vortex),interrupt=1" );
    serenity_aoelust->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&!spinning_crane_kick.max&talent.shadowboxing_treads" );
    serenity_aoelust->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&buff.teachings_of_the_monastery.stack=3&buff.teachings_of_the_monastery.remains<1" );
    serenity_aoelust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&!buff.blackout_reinforcement.up" );
    serenity_aoelust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&set_bonus.tier31_2pc&combo_strike&buff.blackout_reinforcement.up&prev.blackout_kick&buff.dance_of_chiji.up" );
    serenity_aoelust->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=set_bonus.tier31_2pc&combo_strike&buff.blackout_reinforcement.up" );
    serenity_aoelust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&set_bonus.tier31_2pc&combo_strike&!buff.blackout_reinforcement.up" );
    serenity_aoelust->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=active_enemies<6&combo_strike&set_bonus.tier31_2pc" );
    serenity_aoelust->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.pressure_point.up&set_bonus.tier30_2pc" );
    serenity_aoelust->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=set_bonus.tier30_2pc" );
    serenity_aoelust->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist&buff.call_to_dominance.up&debuff.skyreach_exhaustion.remains>buff.call_to_dominance.remains&active_enemies<10" );
    serenity_aoelust->add_action( "jadefire_stomp,if=debuff.jadefire_brand_damage.remains<2" );
    serenity_aoelust->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist" );
    serenity_aoelust->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains,if=buff.invokers_delight.up,interrupt=1" );
    serenity_aoelust->add_action( "fists_of_fury_cancel,target_if=max:debuff.keefers_skyreach.remains" );
    serenity_aoelust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up" );
    serenity_aoelust->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=active_enemies<6&combo_strike&set_bonus.tier30_2pc" );
    serenity_aoelust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&spinning_crane_kick.max" );
    serenity_aoelust->add_action( "tiger_palm,,target_if=min:debuff.mark_of_the_crane.remains,if=!debuff.skyreach_exhaustion.up*20&combo_strike&active_enemies=5" );
    serenity_aoelust->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    serenity_aoelust->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=talent.shadowboxing_treads&active_enemies>=3&combo_strike" );
    serenity_aoelust->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains" );
    serenity_aoelust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike" );
    serenity_aoelust->add_action( "whirling_dragon_punch" );
    serenity_aoelust->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    serenity_aoelust->add_action( "blackout_kick,target_if=max:debuff.keefers_skyreach.remains,if=combo_strike" );
    serenity_aoelust->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=talent.teachings_of_the_monastery&buff.teachings_of_the_monastery.stack<3" );

    // Serenity Lust Priority
    serenity_lust->add_action( "jadefire_stomp,if=debuff.jadefire_brand_damage.remains<1", "Serenity Lust" );
    serenity_lust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&buff.serenity.remains<1.5&combo_strike&!buff.blackout_reinforcement.remains&set_bonus.tier31_2pc" );
    serenity_lust->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&buff.teachings_of_the_monastery.stack=3&buff.teachings_of_the_monastery.remains<1" );
    serenity_lust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&!buff.blackout_reinforcement.up&set_bonus.tier31_2pc&active_enemies>2" );
    serenity_lust->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist" );
    serenity_lust->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&buff.blackout_reinforcement.up&active_enemies>2" );
    serenity_lust->add_action( "rising_sun_kick,target_if=max:debuff.keefers_skyreach.remains,if=combo_strike&(active_enemies<3|!set_bonus.tier31_2pc)" );
    serenity_lust->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&buff.blackout_reinforcement.up&prev.fists_of_fury&talent.shadowboxing_treads&set_bonus.tier31_2pc&!talent.dance_of_chiji" );
    serenity_lust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&!buff.blackout_reinforcement.up&prev.fists_of_fury&debuff.skyreach_exhaustion.remains>55&set_bonus.tier31_2pc" );
    serenity_lust->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains,if=buff.invokers_delight.up&(active_enemies<3|!set_bonus.tier31_2pc),interrupt=1" );
    serenity_lust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&spinning_crane_kick.max&!buff.blackout_reinforcement.up&active_enemies>2&set_bonus.tier31_2pc");
    serenity_lust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&spinning_crane_kick.max&buff.blackout_reinforcement.up&active_enemies>2&prev.blackout_kick&set_bonus.tier31_2pc");
    serenity_lust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&spinning_crane_kick.max&buff.bonedust_brew.up&active_enemies>2&set_bonus.tier31_2pc" );
    serenity_lust->add_action( "fists_of_fury_cancel,target_if=max:debuff.keefers_skyreach.remains,if=active_enemies<3|!set_bonus.tier31_2pc" );
    
    serenity_lust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&!buff.blackout_reinforcement.up" );
    serenity_lust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&!buff.blackout_reinforcement.up&buff.bonedust_brew.up&set_bonus.tier31_2pc" );
    serenity_lust->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike" );
    serenity_lust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up" );
    serenity_lust->add_action( "blackout_kick,target_if=max:debuff.keefers_skyreach.remains,if=combo_strike" );
    serenity_lust->add_action( "whirling_dragon_punch" );
    serenity_lust->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=talent.teachings_of_the_monastery&buff.teachings_of_the_monastery.stack<3" );
    serenity_lust->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike");

    // Serenity >4 Target Priority
    serenity_aoe->add_action( "jadefire_stomp,if=debuff.jadefire_brand_damage.remains<1", "Serenity >4 Targets" );
    serenity_aoe->add_action( "strike_of_the_windlord,if=set_bonus.tier31_4pc&talent.thunderfist" );
    serenity_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&set_bonus.tier31_2pc" );
    serenity_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&!buff.blackout_reinforcement.up&set_bonus.tier31_2pc&buff.bonedust_brew.up&active_enemies>4" );
    serenity_aoe->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains,if=talent.jade_ignition,interrupt_if=buff.chi_energy.stack=30" );
    serenity_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&set_bonus.tier31_2pc&combo_strike&!buff.blackout_reinforcement.up" );
    serenity_aoe->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains,if=buff.invokers_delight.up&set_bonus.tier31_2pc&(!buff.bonedust_brew.up|active_enemies>10)&(buff.transfer_the_power.stack=10&!talent.crane_vortex|active_enemies>5&talent.crane_vortex&buff.transfer_the_power.stack=10|active_enemies>14|active_enemies>12&!talent.crane_vortex),interrupt=1" ); 
    serenity_aoe->add_action( "fists_of_fury_cancel,target_if=max:debuff.keefers_skyreach.remains,if=!set_bonus.tier30_2pc&set_bonus.tier31_2pc&(!buff.bonedust_brew.up|active_enemies>10)&(buff.transfer_the_power.stack=10&!talent.crane_vortex|active_enemies>5&talent.crane_vortex&buff.transfer_the_power.stack=10|active_enemies>14|active_enemies>12&!talent.crane_vortex)&!buff.bonedust_brew.up|buff.fury_of_xuen_stacks.stack>90" );
    serenity_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&!spinning_crane_kick.max&talent.shadowboxing_treads" );
    serenity_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&buff.teachings_of_the_monastery.stack=3&buff.teachings_of_the_monastery.remains<1" );
    serenity_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&!buff.blackout_reinforcement.up" );
    serenity_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&set_bonus.tier31_2pc&combo_strike&buff.blackout_reinforcement.up&prev.blackout_kick&buff.dance_of_chiji.up" );
    serenity_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=set_bonus.tier31_2pc&combo_strike&buff.blackout_reinforcement.up" );
    serenity_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&set_bonus.tier31_2pc&combo_strike&buff.blackout_reinforcement.up&prev.blackout_kick" );
    serenity_aoe->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.pressure_point.up&set_bonus.tier30_2pc" );
    serenity_aoe->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=set_bonus.tier30_2pc" );
    serenity_aoe->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist&buff.call_to_dominance.up&debuff.skyreach_exhaustion.remains>buff.call_to_dominance.remains&active_enemies<10" );
    serenity_aoe->add_action( "jadefire_stomp,if=debuff.jadefire_brand_damage.remains<2" );
    serenity_aoe->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains,if=buff.invokers_delight.up,interrupt=1" );
    serenity_aoe->add_action( "fists_of_fury_cancel,target_if=max:debuff.keefers_skyreach.remains" );
    serenity_aoe->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist" );
    serenity_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up" );
    serenity_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=active_enemies<6&combo_strike&set_bonus.tier30_2pc" );
    serenity_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&spinning_crane_kick.max" );
    serenity_aoe->add_action( "tiger_palm,,target_if=min:debuff.mark_of_the_crane.remains,if=!debuff.skyreach_exhaustion.up*20&combo_strike&active_enemies=5" );
    serenity_aoe->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    serenity_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=talent.shadowboxing_treads&active_enemies>=3&combo_strike" );
    serenity_aoe->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains" );
    serenity_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike" );
    serenity_aoe->add_action( "whirling_dragon_punch" );
    serenity_aoe->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    serenity_aoe->add_action( "blackout_kick,target_if=max:debuff.keefers_skyreach.remains,if=combo_strike" );
    serenity_aoe->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=talent.teachings_of_the_monastery&buff.teachings_of_the_monastery.stack<3" );

    // Serenity 4 Target Priority
    serenity_4t->add_action( "jadefire_stomp,if=debuff.jadefire_brand_damage.remains<1", "Serenity 4 Targets" );
    serenity_4t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&buff.serenity.remains<1.5&combo_strike&!buff.blackout_reinforcement.remains&set_bonus.tier31_2pc" );
    serenity_4t->add_action( "strike_of_the_windlord,if=set_bonus.tier31_4pc&talent.thunderfist" );
    serenity_4t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&!buff.blackout_reinforcement.up&set_bonus.tier31_2pc" );
    serenity_4t->add_action( "fists_of_fury_cancel,target_if=max:debuff.keefers_skyreach.remains,if=!set_bonus.tier30_2pc&buff.fury_of_xuen_stacks.stack>90" );
    serenity_4t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&set_bonus.tier31_2pc&!buff.blackout_reinforcement.up" );
    serenity_4t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&buff.teachings_of_the_monastery.stack=3&buff.teachings_of_the_monastery.remains<1" );
    serenity_4t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=set_bonus.tier31_2pc&combo_strike&buff.blackout_reinforcement.up" );
    serenity_4t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&set_bonus.tier31_2pc&combo_strike&!buff.blackout_reinforcement.up&talent.crane_vortex" );
    serenity_4t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.pressure_point.up&!talent.bonedust_brew" );
    serenity_4t->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains,if=buff.invokers_delight.up&set_bonus.tier31_2pc&buff.transfer_the_power.stack>5&!talent.crane_vortex&buff.bloodlust.up,interrupt=1" );
    serenity_4t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.pressure_point.up&set_bonus.tier30_2pc" );
    serenity_4t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=set_bonus.tier30_2pc" );
    serenity_4t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&set_bonus.tier31_2pc&combo_strike&!buff.blackout_reinforcement.up" );
    serenity_4t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist&buff.call_to_dominance.up&debuff.skyreach_exhaustion.remains>buff.call_to_dominance.remains" );
    serenity_4t->add_action( "jadefire_stomp,if=debuff.jadefire_brand_damage.remains<2" );
    serenity_4t->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains,if=buff.invokers_delight.up,interrupt=1" );
    serenity_4t->add_action( "fists_of_fury_cancel,target_if=max:debuff.keefers_skyreach.remains" );
    serenity_4t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist" );
    serenity_4t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&!buff.blackout_reinforcement.up" );
    serenity_4t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.pressure_point.up" );
    serenity_4t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&set_bonus.tier30_2pc" );
    serenity_4t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&spinning_crane_kick.max" );
    serenity_4t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=talent.shadowboxing_treads&combo_strike" );
    serenity_4t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains" );
    serenity_4t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&!buff.blackout_reinforcement.up" );
    serenity_4t->add_action( "whirling_dragon_punch" );
    serenity_4t->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    serenity_4t->add_action( "blackout_kick,target_if=max:debuff.keefers_skyreach.remains,if=combo_strike" );
    serenity_4t->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=talent.teachings_of_the_monastery&buff.teachings_of_the_monastery.stack<3" );

    // Serenity 3 Target Priority
    serenity_3t->add_action( "jadefire_stomp,if=debuff.jadefire_brand_damage.remains<1", "Serenity 3 Targets" );
    serenity_3t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=set_bonus.tier31_2pc&combo_strike&buff.blackout_reinforcement.up" );
    serenity_3t->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=!debuff.skyreach_exhaustion.up*20&combo_strike" );
    serenity_3t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&spinning_crane_kick.max&!buff.blackout_reinforcement.up&set_bonus.tier31_2pc" );
    serenity_3t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist&buff.call_to_dominance.up&debuff.skyreach_exhaustion.remains>buff.call_to_dominance.remains" );
    serenity_3t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist&debuff.skyreach_exhaustion.remains>55" );
    serenity_3t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&buff.blackout_reinforcement.up" );
    serenity_3t->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains,if=buff.invokers_delight.up&!set_bonus.tier31_2pc,interrupt=1" );
    serenity_3t->add_action( "fists_of_fury_cancel,target_if=max:debuff.keefers_skyreach.remains,if=!set_bonus.tier31_2pc|buff.fury_of_xuen_stacks.stack>90" );
    serenity_3t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&spinning_crane_kick.max&!buff.blackout_reinforcement.up&set_bonus.tier31_2pc" );
    serenity_3t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&spinning_crane_kick.max&buff.blackout_reinforcement.up&set_bonus.tier31_2pc&prev.blackout_kick&talent.crane_vortex" );
    serenity_3t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&!buff.pressure_point.up" );
    serenity_3t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.pressure_point.up" );
    serenity_3t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.pressure_point.up&set_bonus.tier30_2pc" );
    serenity_3t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=set_bonus.tier30_2pc" );
    serenity_3t->add_action( "jadefire_stomp,if=debuff.jadefire_brand_damage.remains<2" );
    serenity_3t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist" );
    serenity_3t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&spinning_crane_kick.max&!buff.blackout_reinforcement.up" );
    serenity_3t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&set_bonus.tier30_2pc" );
    serenity_3t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&!buff.blackout_reinforcement.up" );
    serenity_3t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&buff.teachings_of_the_monastery.stack=2" );
    serenity_3t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=talent.shadowboxing_treads&combo_strike" );
    serenity_3t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains" );
    serenity_3t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike" );
    serenity_3t->add_action( "whirling_dragon_punch" );
    serenity_3t->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    serenity_3t->add_action( "blackout_kick,target_if=max:debuff.keefers_skyreach.remains,if=combo_strike" );
    serenity_3t->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=talent.teachings_of_the_monastery&buff.teachings_of_the_monastery.stack<3" );

    // Serenity 2 Target Priority
    serenity_2t->add_action( "jadefire_stomp,if=debuff.jadefire_brand_damage.remains<2&!debuff.skyreach_exhaustion.remains<2&!debuff.skyreach_exhaustion.remains", "Serenity 2 Targets" );
    serenity_2t->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=!debuff.skyreach_exhaustion.up*20&combo_strike" );
    serenity_2t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&buff.teachings_of_the_monastery.stack=3&buff.teachings_of_the_monastery.remains<1" );
    serenity_2t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&!buff.blackout_reinforcement.up&prev.fists_of_fury&set_bonus.tier31_2pc" );
    serenity_2t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.pressure_point.up|debuff.skyreach_exhaustion.remains>55" );
    serenity_2t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.pressure_point.up&set_bonus.tier30_2pc" );
    serenity_2t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=set_bonus.tier30_2pc" );
    serenity_2t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist&buff.call_to_dominance.up&debuff.skyreach_exhaustion.remains>buff.call_to_dominance.remains" );
    serenity_2t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist&debuff.skyreach_exhaustion.remains>55" );
    serenity_2t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&buff.blackout_reinforcement.up" );
    serenity_2t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&!buff.blackout_reinforcement.up&set_bonus.tier31_2pc" );
    serenity_2t->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains,if=buff.invokers_delight.up,interrupt=1" );
    serenity_2t->add_action( "fists_of_fury_cancel,target_if=max:debuff.keefers_skyreach.remains" );
    serenity_2t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist" );
    serenity_2t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&!buff.blackout_reinforcement.up&set_bonus.tier31_2pc" );
    serenity_2t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&!buff.blackout_reinforcement.up&set_bonus.tier31_2pc" );
    serenity_2t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&set_bonus.tier30_2pc" );
    serenity_2t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&buff.teachings_of_the_monastery.stack=2" );
    serenity_2t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=cooldown.fists_of_fury.remains>5&talent.shadowboxing_treads&buff.teachings_of_the_monastery.stack=1&combo_strike" );
    serenity_2t->add_action( "whirling_dragon_punch" );
    serenity_2t->add_action( "blackout_kick,target_if=max:debuff.keefers_skyreach.remains,if=combo_strike" );
    serenity_2t->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=talent.teachings_of_the_monastery&buff.teachings_of_the_monastery.stack<3" );

    // Serenity 1 Target Priority
    serenity_st->add_action( "jadefire_stomp,if=debuff.jadefire_brand_damage.remains<2&!debuff.skyreach_exhaustion.remains", "Serenity 1 Target" );
    serenity_st->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&buff.serenity.remains<1.5&combo_strike&!buff.blackout_reinforcement.remains&set_bonus.tier31_4pc" );
    serenity_st->add_action( "tiger_palm,if=!debuff.skyreach_exhaustion.up*20&combo_strike" );
    serenity_st->add_action( "blackout_kick,if=combo_strike&buff.teachings_of_the_monastery.stack=3&buff.teachings_of_the_monastery.remains<1" );
    serenity_st->add_action( "strike_of_the_windlord,if=talent.thunderfist&set_bonus.tier31_4pc" );
    serenity_st->add_action( "rising_sun_kick,if=combo_strike" );
    serenity_st->add_action( "jadefire_stomp,if=debuff.jadefire_brand_damage.remains<2" );
    serenity_st->add_action( "strike_of_the_windlord,if=talent.thunderfist&buff.call_to_dominance.up&debuff.skyreach_exhaustion.remains>buff.call_to_dominance.remains" );
    serenity_st->add_action( "strike_of_the_windlord,if=talent.thunderfist&debuff.skyreach_exhaustion.remains>55" );
    serenity_st->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up&!buff.blackout_reinforcement.up&prev.rising_sun_kick&set_bonus.tier31_2pc" );
    serenity_st->add_action( "blackout_kick,if=combo_strike&set_bonus.tier31_2pc&buff.blackout_reinforcement.up&prev.rising_sun_kick" );
    serenity_st->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&!buff.blackout_reinforcement.up&prev.fists_of_fury&set_bonus.tier31_2pc" );
    serenity_st->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&buff.blackout_reinforcement.up&prev.fists_of_fury&set_bonus.tier31_2pc" );
    serenity_st->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&!buff.blackout_reinforcement.up&set_bonus.tier31_2pc&prev.fists_of_fury");
    serenity_st->add_action( "fists_of_fury,if=buff.invokers_delight.up,interrupt=1" );
    serenity_st->add_action( "fists_of_fury_cancel" );
    serenity_st->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&!buff.blackout_reinforcement.up&set_bonus.tier31_2pc" );
    serenity_st->add_action( "strike_of_the_windlord,if=debuff.skyreach_exhaustion.remains>buff.call_to_dominance.remains" );
    serenity_st->add_action( "blackout_kick,if=combo_strike&set_bonus.tier30_2pc" );
    serenity_st->add_action( "blackout_kick,if=combo_strike" );
    serenity_st->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up" );
    serenity_st->add_action( "whirling_dragon_punch" );
    serenity_st->add_action( "tiger_palm,if=talent.teachings_of_the_monastery&buff.teachings_of_the_monastery.stack<3" );

    // >4 Target priority
    default_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&spinning_crane_kick.max&!buff.blackout_reinforcement.up", ">4 Targets" );
    default_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&!talent.hit_combo&spinning_crane_kick.max&buff.bonedust_brew.up" );
    default_aoe->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist" );
    default_aoe->add_action( "whirling_dragon_punch,if=active_enemies>8" );
    default_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&buff.bonedust_brew.up&combo_strike&spinning_crane_kick.max" );
    default_aoe->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains" );
    default_aoe->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.bonedust_brew.up&buff.pressure_point.up&set_bonus.tier30_2pc" );
    default_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.teachings_of_the_monastery.stack=3&talent.shadowboxing_treads" );
    default_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=talent.shadowboxing_treads&combo_strike&buff.blackout_reinforcement.up" );
    default_aoe->add_action( "whirling_dragon_punch,if=active_enemies>=5" );
    default_aoe->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    default_aoe->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.pressure_point.up&set_bonus.tier30_2pc" );
    default_aoe->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=set_bonus.tier30_2pc" );
    default_aoe->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=talent.whirling_dragon_punch&cooldown.whirling_dragon_punch.remains<3&cooldown.fists_of_fury.remains>3&!buff.kicks_of_flowing_momentum.up" );
    default_aoe->add_action( "expel_harm,if=chi=1&(!cooldown.rising_sun_kick.remains|!cooldown.strike_of_the_windlord.remains)|chi=2&!cooldown.fists_of_fury.remains" );
    default_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&cooldown.fists_of_fury.remains<5&buff.chi_energy.stack>10" );
    default_aoe->add_action( "chi_burst,if=buff.bloodlust.up&chi<5" );
    default_aoe->add_action( "chi_burst,if=chi<5&energy<50" );
    default_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&(cooldown.fists_of_fury.remains>3|chi>2)&spinning_crane_kick.max&buff.bloodlust.up&!buff.blackout_reinforcement.up" );
    default_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&(cooldown.fists_of_fury.remains>3|chi>2)&spinning_crane_kick.max&buff.invokers_delight.up&!buff.blackout_reinforcement.up" );
    default_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=talent.shadowboxing_treads&combo_strike&set_bonus.tier30_2pc&!buff.bonedust_brew.up&active_enemies<15&!talent.crane_vortex" );
    default_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=talent.shadowboxing_treads&combo_strike&set_bonus.tier30_2pc&!buff.bonedust_brew.up&active_enemies<8" );
    default_aoe->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&(cooldown.fists_of_fury.remains>3|chi>4)&spinning_crane_kick.max" );
    default_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.teachings_of_the_monastery.stack=3" );
    default_aoe->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains" );
    default_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=talent.shadowboxing_treads&combo_strike&!spinning_crane_kick.max" );
    default_aoe->add_action( "chi_burst,if=chi.max-chi>=1&active_enemies=1&raid_event.adds.in>20|chi.max-chi>=2" );

    // 4 Target priority
    default_4t->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&chi<2&(cooldown.fists_of_fury.remains<1|cooldown.strike_of_the_windlord.remains<1)&buff.teachings_of_the_monastery.stack<3", "4 Targets" );
    default_4t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&spinning_crane_kick.max&!buff.blackout_reinforcement.up" );
    default_4t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist" );
    default_4t->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains" );
    default_4t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.bonedust_brew.up&buff.pressure_point.up&set_bonus.tier30_2pc" );
    default_4t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=talent.shadowboxing_treads&combo_strike&buff.blackout_reinforcement.up" );
    default_4t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&buff.bonedust_brew.up&combo_strike&spinning_crane_kick.max" );
    default_4t->add_action( "whirling_dragon_punch" );
    default_4t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=!buff.bonedust_brew.up&buff.pressure_point.up&cooldown.fists_of_fury.remains>5" );
    default_4t->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    default_4t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.teachings_of_the_monastery.stack=3&talent.shadowboxing_treads" );
    default_4t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=set_bonus.tier30_2pc" );
    default_4t->add_action( "expel_harm,if=chi=1&(!cooldown.rising_sun_kick.remains|!cooldown.strike_of_the_windlord.remains)|chi=2&!cooldown.fists_of_fury.remains" );
    default_4t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&cooldown.fists_of_fury.remains>3&buff.chi_energy.stack>10" );
    default_4t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&set_bonus.tier30_2pc" );
    default_4t->add_action( "chi_burst,if=buff.bloodlust.up&chi<5" );
    default_4t->add_action( "chi_burst,if=chi<5&energy<50" );
    default_4t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&(cooldown.fists_of_fury.remains>3|chi>4)&spinning_crane_kick.max" );
    default_4t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.teachings_of_the_monastery.stack=3" );
    default_4t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains" );
    default_4t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&(cooldown.fists_of_fury.remains>3|chi>4)" );
    default_4t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike" );

    // 3 Target priority
    default_3t->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&chi<2&(cooldown.rising_sun_kick.remains<1|cooldown.fists_of_fury.remains<1|cooldown.strike_of_the_windlord.remains<1)&buff.teachings_of_the_monastery.stack<3", "3 Targets" );
    default_3t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&!buff.blackout_reinforcement.up" );
    default_3t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist&set_bonus.tier31_4pc" );
    default_3t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist&(cooldown.invoke_xuen_the_white_tiger.remains>20|fight_remains<5)" );
    default_3t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.teachings_of_the_monastery.stack=3&talent.shadowboxing_treads" );
    default_3t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=talent.shadowboxing_treads&combo_strike&buff.blackout_reinforcement.up" );
    default_3t->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains" );
    default_3t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.bonedust_brew.up&buff.pressure_point.up&set_bonus.tier30_2pc" );
    default_3t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&buff.bonedust_brew.up&combo_strike" );
    default_3t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=!buff.bonedust_brew.up&buff.pressure_point.up" );
    default_3t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=set_bonus.tier30_2pc" );
    default_3t->add_action( "expel_harm,if=chi=1&(!cooldown.rising_sun_kick.remains|!cooldown.strike_of_the_windlord.remains)|chi=2&!cooldown.fists_of_fury.remains" );
    default_3t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.teachings_of_the_monastery.stack=2" );
    default_3t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains" );
    default_3t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.teachings_of_the_monastery.up&(talent.shadowboxing_treads|cooldown.rising_sun_kick.remains>1)" );
    default_3t->add_action( "whirling_dragon_punch" );
    default_3t->add_action( "chi_burst,if=buff.bloodlust.up&chi<5" );
    default_3t->add_action( "chi_burst,if=chi<5&energy<50" );
    default_3t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.teachings_of_the_monastery.stack=3" );
    default_3t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&cooldown.fists_of_fury.remains<3&buff.chi_energy.stack>15" );
    default_3t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=cooldown.fists_of_fury.remains>4&chi>3" );
    default_3t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&cooldown.rising_sun_kick.remains&cooldown.fists_of_fury.remains&chi>4&((talent.storm_earth_and_fire&!talent.bonedust_brew)|(talent.serenity))" );
    default_3t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&cooldown.fists_of_fury.remains" );
    default_3t->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    default_3t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&talent.shadowboxing_treads&!spinning_crane_kick.max" );
    default_3t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&(combo_strike&chi>5&talent.storm_earth_and_fire|combo_strike&chi>4&talent.serenity)" );

    // 2 Target priority
    default_2t->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&chi<2&(cooldown.rising_sun_kick.remains<1|cooldown.fists_of_fury.remains<1|cooldown.strike_of_the_windlord.remains<1)&buff.teachings_of_the_monastery.stack<3", "2 Targets" );
    default_2t->add_action( "expel_harm,if=chi=1&(!cooldown.rising_sun_kick.remains|!cooldown.strike_of_the_windlord.remains)|chi=2&!cooldown.fists_of_fury.remains" );
    default_2t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.teachings_of_the_monastery.stack=3&talent.shadowboxing_treads" );
    default_2t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist&set_bonus.tier31_4pc" );
    default_2t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains,if=talent.thunderfist&(cooldown.invoke_xuen_the_white_tiger.remains>20|fight_remains<5)" );
    default_2t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&!buff.blackout_reinforcement.up&set_bonus.tier31_2pc" );
    default_2t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=talent.shadowboxing_treads&combo_strike&buff.blackout_reinforcement.up" );
    default_2t->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains,if=!set_bonus.tier30_2pc" );
    default_2t->add_action( "fists_of_fury,target_if=max:debuff.keefers_skyreach.remains" );
    default_2t->add_action( "rising_sun_kick,if=!cooldown.fists_of_fury.remains" );
    default_2t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=set_bonus.tier30_2pc" );
    default_2t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.kicks_of_flowing_momentum.up|buff.pressure_point.up|debuff.skyreach_exhaustion.remains>55" );
    default_2t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&!buff.blackout_reinforcement.up" );
    default_2t->add_action( "chi_burst,if=buff.bloodlust.up&chi<5" );
    default_2t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.teachings_of_the_monastery.stack=2" );
    default_2t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.pressure_point.remains&chi>2&prev.rising_sun_kick" );
    default_2t->add_action( "chi_burst,if=chi<5&energy<50" );
    default_2t->add_action( "strike_of_the_windlord,target_if=max:debuff.keefers_skyreach.remains" );
    default_2t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.teachings_of_the_monastery.up&(talent.shadowboxing_treads|cooldown.rising_sun_kick.remains>1)" );
    default_2t->add_action( "whirling_dragon_punch" );
    default_2t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=buff.teachings_of_the_monastery.stack=3" );
    default_2t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=!talent.shadowboxing_treads&cooldown.fists_of_fury.remains>4&talent.xuens_battlegear" );
    default_2t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&cooldown.rising_sun_kick.remains&cooldown.fists_of_fury.remains&(!buff.bonedust_brew.up|spinning_crane_kick.modifier<1.5)" );
    default_2t->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    default_2t->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&buff.bonedust_brew.up&combo_strike&spinning_crane_kick.modifier>=2.7" );
    default_2t->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains" );
    default_2t->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike" );
    default_2t->add_action( "jadefire_stomp,if=combo_strike" );

    // 1 Target priority
    default_st->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&chi<2&(cooldown.rising_sun_kick.remains<1|cooldown.fists_of_fury.remains<1|cooldown.strike_of_the_windlord.remains<1)&buff.teachings_of_the_monastery.stack<3", "1 Target");
    default_st->add_action( "expel_harm,if=chi=1&(!cooldown.rising_sun_kick.remains|!cooldown.strike_of_the_windlord.remains)|chi=2&!cooldown.fists_of_fury.remains&cooldown.rising_sun_kick.remains" );
    default_st->add_action( "strike_of_the_windlord,if=buff.domineering_arrogance.up&talent.thunderfist&talent.serenity&cooldown.invoke_xuen_the_white_tiger.remains>20|fight_remains<5|talent.thunderfist&debuff.skyreach_exhaustion.remains>10&!buff.domineering_arrogance.up|talent.thunderfist&debuff.skyreach_exhaustion.remains>35&!talent.serenity" );
    default_st->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&set_bonus.tier31_2pc&!buff.blackout_reinforcement.up" );
    default_st->add_action( "rising_sun_kick,if=!cooldown.fists_of_fury.remains" );
    default_st->add_action( "fists_of_fury,if=!buff.pressure_point.up&debuff.skyreach_exhaustion.remains<55&(debuff.jadefire_brand_damage.remains>2|cooldown.jadefire_stomp.remains)");
    default_st->add_action( "jadefire_stomp,if=debuff.skyreach_exhaustion.remains<1&debuff.jadefire_brand_damage.remains<3" );
    default_st->add_action( "rising_sun_kick,if=buff.pressure_point.up|debuff.skyreach_exhaustion.remains>55" );
    default_st->add_action( "blackout_kick,if=buff.pressure_point.remains&chi>2&prev.rising_sun_kick" );
    default_st->add_action( "blackout_kick,if=buff.teachings_of_the_monastery.stack=3");
    default_st->add_action( "blackout_kick,if=buff.blackout_reinforcement.up&cooldown.rising_sun_kick.remains&combo_strike&buff.dance_of_chiji.up" );
    default_st->add_action( "rising_sun_kick" );
    default_st->add_action( "blackout_kick,if=buff.blackout_reinforcement.up&combo_strike" );
    default_st->add_action( "fists_of_fury" );
    default_st->add_action( "whirling_dragon_punch,if=!buff.pressure_point.up" );
    default_st->add_action( "chi_burst,if=buff.bloodlust.up&chi<5" );
    default_st->add_action( "blackout_kick,if=buff.teachings_of_the_monastery.stack=2&debuff.skyreach_exhaustion.remains>1" );
    default_st->add_action( "chi_burst,if=chi<5&energy<50" );
    default_st->add_action( "strike_of_the_windlord,if=debuff.skyreach_exhaustion.remains>30|fight_remains<5" );
    default_st->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.dance_of_chiji.up&!set_bonus.tier31_2pc" );
    default_st->add_action( "blackout_kick,if=buff.teachings_of_the_monastery.up&cooldown.rising_sun_kick.remains>1" );
    default_st->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&buff.bonedust_brew.up&combo_strike&spinning_crane_kick.modifier>=2.7" );
    default_st->add_action( "whirling_dragon_punch" );
    default_st->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    default_st->add_action( "blackout_kick,if=combo_strike" );


    // Fallthru
    fallthru->add_action( "crackling_jade_lightning,if=buff.the_emperors_capacitor.stack>19&energy.time_to_max>execute_time-1&cooldown.rising_sun_kick.remains>execute_time|buff.the_emperors_capacitor.stack>14&(cooldown.serenity.remains<5&talent.serenity|fight_remains<5)", "Fallthru" );
    fallthru->add_action( "jadefire_stomp,if=combo_strike" );
    fallthru->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&chi.max-chi>=(2+buff.power_strikes.up)" );
    fallthru->add_action( "expel_harm,if=chi.max-chi>=1&active_enemies>2" );
    fallthru->add_action( "chi_burst,if=chi.max-chi>=1&active_enemies=1&raid_event.adds.in>20|chi.max-chi>=2&active_enemies>=2" );
    fallthru->add_action( "chi_wave" );
    fallthru->add_action( "expel_harm,if=chi.max-chi>=1" );
    fallthru->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains-spinning_crane_kick.max*(target.time_to_die+debuff.keefers_skyreach.remains*20),if=combo_strike&active_enemies>=5" );
    fallthru->add_action( "spinning_crane_kick,target_if=max:target.time_to_die,if=target.time_to_die>duration&combo_strike&buff.chi_energy.stack>30-5*active_enemies&buff.storm_earth_and_fire.down&(cooldown.rising_sun_kick.remains>2&cooldown.fists_of_fury.remains>2|cooldown.rising_sun_kick.remains<3&cooldown.fists_of_fury.remains>3&chi>3|cooldown.rising_sun_kick.remains>3&cooldown.fists_of_fury.remains<3&chi>4|chi.max-chi<=1&energy.time_to_max<2)|buff.chi_energy.stack>10&fight_remains<7" );
    fallthru->add_action( "arcane_torrent,if=chi.max-chi>=1" );
    fallthru->add_action( "flying_serpent_kick,interrupt=1" );
    fallthru->add_action( "tiger_palm" );
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
