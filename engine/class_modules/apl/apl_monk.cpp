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
          return "potion_of_shocking_disclosure_3";
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
          return "phial_of_tepid_versatility_3";
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
    auto monk = debug_cast< monk::monk_t * >( p );
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

    auto _BRM_ON_USE = [ monk ] ( const item_t &item )
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

    auto _BRM_RACIALS = [ monk ] ( const std::string &racial_action )
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

    action_priority_list_t *cooldowns_niuzao_woo = p->get_action_priority_list( "cooldowns_niuzao_woo" );
    action_priority_list_t *cooldowns_improved_niuzao_woo = p->get_action_priority_list( "cooldowns_improved_niuzao_woo" );
    action_priority_list_t *cooldowns_improved_niuzao_cta = p->get_action_priority_list( "cooldowns_improved_niuzao_cta" );

    action_priority_list_t *rotation_boc_dfb = p->get_action_priority_list( "rotation_boc_dfb" );
    action_priority_list_t *rotation_dfb = p->get_action_priority_list( "rotation_dfb" );
    action_priority_list_t *rotation_chp = p->get_action_priority_list( "rotation_chp" );
    action_priority_list_t *rotation_fallback = p->get_action_priority_list( "rotation_fallback" );

    pre->add_action( "flask" );
    pre->add_action( "food" );
    pre->add_action( "augmentation" );
    pre->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
    pre->add_action( "potion" );
    pre->add_action( "chi_burst,if=talent.chi_burst.enabled" );
    pre->add_action( "chi_wave,if=talent.chi_wave.enabled" );

    // this maybe works but probably has to be done during combat
    pre->add_action( "variable,op=set,name=fallback_detection,value=1" );
    // to detect if fallback is appropriate, add the following line to the top of each rotation apl
    // apl_name->add_action( "variable,op=set,name=fallback_detection,value=0" );

    def->add_action( "auto_attack" );
    def->add_action( "roll,if=movement.distance>5", "Move to target" );
    def->add_action( "chi_torpedo,if=movement.distance>5" );
    def->add_action( "spear_hand_strike,if=target.debuff.casting.react" );
    def->add_action( "potion" ); // TODO: potion sync with cds
    def->add_action( "summon_white_tiger_statue,if=talent.summon_white_tiger_statue.enabled" );

    def->add_action( "call_action_list,name=item_actions" );
    def->add_action( "call_action_list,name=race_actions" );

    def->add_action( "call_action_list,name=cooldowns_improved_niuzao_woo,if=(talent.invoke_niuzao_the_black_ox.enabled+talent.improved_invoke_niuzao_the_black_ox.enabled)=2&(talent.weapons_of_order.enabled+talent.call_to_arms.enabled)<=1" );
    def->add_action( "call_action_list,name=cooldowns_improved_niuzao_cta,if=(talent.invoke_niuzao_the_black_ox.enabled+talent.improved_invoke_niuzao_the_black_ox.enabled)=2&(talent.weapons_of_order.enabled+talent.call_to_arms.enabled)=2" );
    def->add_action( "call_action_list,name=cooldowns_niuzao_woo,if=(talent.invoke_niuzao_the_black_ox.enabled+talent.improved_invoke_niuzao_the_black_ox.enabled)<=1" );

    def->add_action( "call_action_list,name=rotation_boc_dfb,if=talent.blackout_combo.enabled&talent.dragonfire_brew.enabled&talent.salsalabims_strength.enabled" );
    def->add_action( "call_action_list,name=rotation_dfb,if=talent.dragonfire_brew.enabled&talent.salsalabims_strength.enabled" );
    def->add_action( "call_action_list,name=rotation_chp,if=talent.charred_passions.enabled&talent.salsalabims_strength.enabled" );
    def->add_action( "call_action_list,name=rotation_fallback,if=variable.fallback_detection" );

    // trinket_actions->add_action( "use_item,name=" +_p->items[SLOT_TRINKET_1].name_str)

    for ( const auto &item : p->items )
    {
      if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
        item_actions->add_action( "use_item,name=" + item.name_str + _BRM_ON_USE( item ) );
    }

    for ( const auto &racial_action : racial_actions )
      race_actions->add_action( racial_action + _BRM_RACIALS( racial_action ));

    // TODO
    // cds
    // bdb:
    //  - bb consistency for ek alignment (?) probably ok


    cooldowns_niuzao_woo->add_action( "invoke_external_buff,name=power_infusion,if=buff.weapons_of_order.remains<=20",
      "Use <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> when <a href='https://www.wowhead.com/spell=387184/weapons-of-order'>Weapons of Order</a> reaches 4 stacks." );
    cooldowns_niuzao_woo->add_action( "weapons_of_order,if="
                                      "(talent.weapons_of_order.enabled)");
    cooldowns_niuzao_woo->add_action( "bonedust_brew,if=!buff.bonedust_brew.up&debuff.weapons_of_order_debuff.stack>3" );
    cooldowns_niuzao_woo->add_action( "bonedust_brew,if=!buff.bonedust_brew.up&!buff.weapons_of_order.up&cooldown.weapons_of_order.remains>10" );
    cooldowns_niuzao_woo->add_action( "exploding_keg,if=buff.bonedust_brew.up" );
    cooldowns_niuzao_woo->add_action( "invoke_niuzao_the_black_ox,if=buff.weapons_of_order.remains<=16&talent.weapons_of_order.enabled" );
    cooldowns_niuzao_woo->add_action( "invoke_niuzao_the_black_ox,if=!talent.weapons_of_order.enabled" );
    cooldowns_niuzao_woo->add_action( "purifying_brew,if=stagger.amounttototalpct>=0.7&!buff.blackout_combo.up" );
    cooldowns_niuzao_woo->add_action( "purifying_brew,if=cooldown.purifying_brew.remains_expected<5&!buff.blackout_combo.up" );

    cooldowns_improved_niuzao_woo->add_action( "invoke_external_buff,name=power_infusion,if=buff.weapons_of_order.remains<=20",
      "Use <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> when <a href='https://www.wowhead.com/spell=387184/weapons-of-order'>Weapons of Order</a> reaches 4 stacks." );
    cooldowns_improved_niuzao_woo->add_action( "variable,name=pb_in_window,op=set,value=floor(cooldown.purifying_brew.charges_fractional+(20+4*0.05)%cooldown.purifying_brew.duration%0.65),if=prev.invoke_niuzao_the_black_ox" );
    cooldowns_improved_niuzao_woo->add_action( "variable,name=pb_in_window,op=sub,value=1,if=prev.purifying_brew&time-action.invoke_niuzao_the_black_ox.last_used<=20+4*0.05" );
    cooldowns_improved_niuzao_woo->add_action( "purifying_brew,if=(time-action.purifying_brew.last_used>=20+4*0.05-time+action.invoke_niuzao_the_black_ox.last_used%variable.pb_in_window&time-action.invoke_niuzao_the_black_ox.last_used<=20+4*0.05)" );
    cooldowns_improved_niuzao_woo->add_action( "purifying_brew,use_off_gcd=1,if=(variable.pb_in_window=0&20+4*0.05-time+action.invoke_niuzao_the_black_ox.last_used<1&20+4*0.05-time+action.invoke_niuzao_the_black_ox.last_used>0)" );
    cooldowns_improved_niuzao_woo->add_action( "weapons_of_order,if="
                                               "(talent.weapons_of_order.enabled)");
    cooldowns_improved_niuzao_woo->add_action( "bonedust_brew,if=!buff.bonedust_brew.up&debuff.weapons_of_order_debuff.stack>3" );
    cooldowns_improved_niuzao_woo->add_action( "bonedust_brew,if=!buff.bonedust_brew.up&!buff.weapons_of_order.up&cooldown.weapons_of_order.remains>10" );
    cooldowns_improved_niuzao_woo->add_action( "exploding_keg,if=buff.bonedust_brew.up" );
    cooldowns_improved_niuzao_woo->add_action( "purifying_brew,if=cooldown.invoke_niuzao_the_black_ox.remains<=3.5&time-action.purifying_brew.last_used>=3.5+cooldown.invoke_niuzao_the_black_ox.remains" );
    cooldowns_improved_niuzao_woo->add_action( "invoke_niuzao_the_black_ox,if=time-action.purifying_brew.last_used<=5" );
    cooldowns_improved_niuzao_woo->add_action( "purifying_brew,if=cooldown.purifying_brew.full_recharge_time*2<=cooldown.invoke_niuzao_the_black_ox.remains-3.5" );

    cooldowns_improved_niuzao_cta->add_action( "invoke_external_buff,name=power_infusion,if=buff.weapons_of_order.remains<=20",
      "Use <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> when <a href='https://www.wowhead.com/spell=387184/weapons-of-order'>Weapons of Order</a> reaches 4 stacks." );
    cooldowns_improved_niuzao_cta->add_action( "variable,name=pb_in_window,op=set,value=floor(cooldown.purifying_brew.charges_fractional+(10+2*0.05)%cooldown.purifying_brew.duration%0.65),if=prev.weapons_of_order" );
    cooldowns_improved_niuzao_cta->add_action( "variable,name=pb_in_window,op=set,value=floor(cooldown.purifying_brew.charges_fractional+(20+4*0.05)%cooldown.purifying_brew.duration%0.65),if=prev.invoke_niuzao_the_black_ox" );
    cooldowns_improved_niuzao_cta->add_action( "variable,name=pb_in_window,op=sub,value=1,if=prev.purifying_brew&(time-action.weapons_of_order.last_used<=10+2*0.05|time-action.invoke_niuzao_the_black_ox.last_used<=20+4*0.05)" );
    cooldowns_improved_niuzao_cta->add_action( "purifying_brew,if=(time-action.purifying_brew.last_used>=10+2*0.05-time+action.weapons_of_order.last_used%variable.pb_in_window&time-action.weapons_of_order.last_used<=10+2*0.05)" );
    cooldowns_improved_niuzao_cta->add_action( "purifying_brew,if=(time-action.purifying_brew.last_used>=20+4*0.05-time+action.invoke_niuzao_the_black_ox.last_used%variable.pb_in_window&time-action.invoke_niuzao_the_black_ox.last_used<=20+4*0.05)" );
    cooldowns_improved_niuzao_cta->add_action( "purifying_brew,use_off_gcd=1,if=(variable.pb_in_window=0&20+4*0.05-time+action.invoke_niuzao_the_black_ox.last_used<1&20+4*0.05-time+action.invoke_niuzao_the_black_ox.last_used>0)" );
    cooldowns_improved_niuzao_cta->add_action( "purifying_brew,if=cooldown.weapons_of_order.remains<=3.5&time-action.purifying_brew.last_used>=3.5+cooldown.weapons_of_order.remains" );
    cooldowns_improved_niuzao_cta->add_action( "weapons_of_order,if="
                                               "(time-action.purifying_brew.last_used<=5)");
    cooldowns_improved_niuzao_cta->add_action( "bonedust_brew,if=!buff.bonedust_brew.up&debuff.weapons_of_order_debuff.stack>3" );
    cooldowns_improved_niuzao_cta->add_action( "bonedust_brew,if=!buff.bonedust_brew.up&!buff.weapons_of_order.up&cooldown.weapons_of_order.remains>10" );
    cooldowns_improved_niuzao_cta->add_action( "exploding_keg,if=buff.bonedust_brew.up" );
    cooldowns_improved_niuzao_cta->add_action( "purifying_brew,if=cooldown.invoke_niuzao_the_black_ox.remains<=3.5&time-action.purifying_brew.last_used>=3.5+cooldown.invoke_niuzao_the_black_ox.remains&buff.weapons_of_order.remains<=30-17" );
    cooldowns_improved_niuzao_cta->add_action( "invoke_niuzao_the_black_ox,if=buff.weapons_of_order.remains<=30-17&action.purifying_brew.last_used>action.weapons_of_order.last_used+10+2*0.05" );
    cooldowns_improved_niuzao_cta->add_action( "purifying_brew,if=cooldown.purifying_brew.full_recharge_time*2<=cooldown.weapons_of_order.remains-3.5&cooldown.purifying_brew.full_recharge_time*2<=cooldown.invoke_niuzao_the_black_ox.remains-3.5" );

    // rotations
    // sck:
    //  - remove energy condition from tp and sck?
    //  - sck if wwto + chp talented?
    //  - 2x sck after ek?

    rotation_boc_dfb->add_action( "variable,op=set,name=fallback_detection,value=0");
    rotation_boc_dfb->add_action( "blackout_kick" );
    rotation_boc_dfb->add_action( "rising_sun_kick,if=talent.rising_sun_kick.enabled" );
    rotation_boc_dfb->add_action( "rushing_jade_wind,if=talent.rushing_jade_wind.enabled&buff.rushing_jade_wind.remains<1" );
    rotation_boc_dfb->add_action( "breath_of_fire,if=buff.blackout_combo.up" );
    rotation_boc_dfb->add_action( "keg_smash" );
    rotation_boc_dfb->add_action( "black_ox_brew,if=energy+energy.regen*(cooldown.keg_smash.full_recharge_time*(1-cooldown.keg_smash.charges_fractional))>=65&talent.black_ox_brew.enabled" );
    rotation_boc_dfb->add_action( "rushing_jade_wind,if=talent.rushing_jade_wind.enabled" );
    // rotation_boc_dfb->add_action( "spinning_crane_kick,if=active_enemies>1&energy+energy.regen*(cooldown.keg_smash.full_recharge_time*(1-cooldown.keg_smash.charges_fractional))>=65" );
    // rotation_boc_dfb->add_action( "tiger_palm,if=active_enemies=1&energy+energy.regen*(cooldown.keg_smash.full_recharge_time*(1-cooldown.keg_smash.charges_fractional))>=65" );
    rotation_boc_dfb->add_action( "spinning_crane_kick,if=active_enemies>1|(talent.walk_with_the_ox.enabled&talent.charred_passions.enabled)" );
    rotation_boc_dfb->add_action( "tiger_palm,if=active_enemies=1" );
    rotation_boc_dfb->add_action( "celestial_brew,if=talent.celestial_brew.enabled&!buff.blackout_combo.up" );
    rotation_boc_dfb->add_action( "chi_wave,if=talent.chi_wave.enabled" );
    rotation_boc_dfb->add_action( "chi_burst,if=talent.chi_burst.enabled" );

    rotation_dfb->add_action( "variable,op=set,name=fallback_detection,value=0");
    rotation_dfb->add_action( "blackout_kick" );
    rotation_dfb->add_action( "rising_sun_kick,if=talent.rising_sun_kick.enabled" );
    rotation_dfb->add_action( "rushing_jade_wind,if=talent.rushing_jade_wind.enabled&buff.rushing_jade_wind.remains<1" );
    rotation_dfb->add_action( "breath_of_fire" );
    rotation_dfb->add_action( "keg_smash" );
    rotation_dfb->add_action( "black_ox_brew,if=energy+energy.regen*(cooldown.keg_smash.full_recharge_time*(1-cooldown.keg_smash.charges_fractional))>=65&talent.black_ox_brew.enabled" );
    rotation_dfb->add_action( "rushing_jade_wind,if=talent.rushing_jade_wind.enabled" );
    // rotation_dfb->add_action( "spinning_crane_kick,if=active_enemies>1&energy+energy.regen*(cooldown.keg_smash.full_recharge_time*(1-cooldown.keg_smash.charges_fractional))>=65" );
    // rotation_dfb->add_action( "tiger_palm,if=active_enemies=1&energy+energy.regen*(cooldown.keg_smash.full_recharge_time*(1-cooldown.keg_smash.charges_fractional))>=65" );
    rotation_dfb->add_action( "spinning_crane_kick,if=active_enemies>1|(talent.walk_with_the_ox.enabled&talent.charred_passions.enabled)" );
    rotation_dfb->add_action( "tiger_palm,if=active_enemies=1" );
    rotation_dfb->add_action( "celestial_brew,if=talent.celestial_brew.enabled" );
    rotation_dfb->add_action( "chi_wave,if=talent.chi_wave.enabled" );
    rotation_dfb->add_action( "chi_burst,if=talent.chi_burst.enabled" );

    rotation_chp->add_action( "variable,op=set,name=fallback_detection,value=0");
    rotation_chp->add_action( "breath_of_fire,if=!buff.charred_passions.up" );
    rotation_chp->add_action( "blackout_kick" );
    rotation_chp->add_action( "rising_sun_kick,if=talent.rising_sun_kick.enabled" );
    rotation_chp->add_action( "rushing_jade_wind,if=talent.rushing_jade_wind.enabled&buff.rushing_jade_wind.remains<1" );
    rotation_chp->add_action( "keg_smash" );
    rotation_chp->add_action( "black_ox_brew,if=energy+energy.regen*(cooldown.keg_smash.full_recharge_time*(1-cooldown.keg_smash.charges_fractional))>=65&talent.black_ox_brew.enabled" );
    rotation_chp->add_action( "rushing_jade_wind,if=talent.rushing_jade_wind.enabled" );
    // rotation_chp->add_action( "spinning_crane_kick,if=active_enemies>1&energy+energy.regen*(cooldown.keg_smash.full_recharge_time*(1-cooldown.keg_smash.charges_fractional))>=65" );
    // rotation_chp->add_action( "tiger_palm,if=active_enemies=1&energy+energy.regen*(cooldown.keg_smash.full_recharge_time*(1-cooldown.keg_smash.charges_fractional))>=65" );
    rotation_chp->add_action( "spinning_crane_kick,if=active_enemies>1|(talent.walk_with_the_ox.enabled&talent.charred_passions.enabled)" );
    rotation_chp->add_action( "tiger_palm,if=active_enemies=1" );
    rotation_chp->add_action( "celestial_brew,if=talent.celestial_brew.enabled&!buff.blackout_combo.up" );
    rotation_chp->add_action( "chi_wave,if=talent.chi_wave.enabled" );
    rotation_chp->add_action( "chi_burst,if=talent.chi_burst.enabled" );

    rotation_fallback->add_action( "rising_sun_kick,if=talent.rising_sun_kick.enabled" );
    rotation_fallback->add_action( "keg_smash" );
    rotation_fallback->add_action( "breath_of_fire,if=talent.breath_of_fire.enabled" );
    rotation_fallback->add_action( "blackout_kick" );
    rotation_fallback->add_action( "black_ox_brew,if=energy+energy.regen*(cooldown.keg_smash.remains+execute_time)>=65&talent.black_ox_brew.enabled" );
    rotation_fallback->add_action( "rushing_jade_wind,if=talent.rushing_jade_wind.enabled" );
    // rotation_fallback->add_action( "spinning_crane_kick,if=active_enemies>1" );
    // rotation_fallback->add_action( "tiger_palm,if=active_enemies=1" );
    rotation_fallback->add_action( "spinning_crane_kick,if=active_enemies>1|(talent.walk_with_the_ox.enabled&talent.charred_passions.enabled)" );
    rotation_fallback->add_action( "tiger_palm,if=active_enemies=1" );
    rotation_fallback->add_action( "celestial_brew,if=!buff.blackout_combo.up&talent.celestial_brew.enabled" );
    rotation_fallback->add_action( "chi_wave,if=talent.chi_wave.enabled" );
    rotation_fallback->add_action( "chi_burst,if=talent.chi_burst.enabled" );
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

    def->add_action( "faeline_stomp" );
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
        { "manic_grieftorch", ",use_off_gcd=1,if=!trinket.1.has_use_buff&!trinket.2.has_use_buff&!buff.serenity.up&!pet.xuen_the_white_tiger.active&(debuff.fae_exposure_damage.remains>3|!talent.faeline_harmony)|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>30&cooldown.serenity.remains&(debuff.fae_exposure_damage.remains>2|!talent.faeline_harmony)|fight_remains<5" },
        { "beacon_to_the_beyond", ",use_off_gcd=1,if=!trinket.1.has_use_buff&!trinket.2.has_use_buff&!buff.serenity.up&!pet.xuen_the_white_tiger.active&(debuff.fae_exposure_damage.remains>2|!talent.faeline_harmony)|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>30&cooldown.serenity.remains&(debuff.fae_exposure_damage.remains>2|!talent.faeline_harmony)|fight_remains<10" },
        { "djaruun_pillar_of_the_elder_flame", ",if=cooldown.fists_of_fury.remains<2&cooldown.invoke_xuen_the_white_tiger.remains>10|fight_remains<12" },
        { "dragonfire_bomb_dispenser", ",if=!trinket.1.has_use_buff&!trinket.2.has_use_buff&(debuff.fae_exposure_damage.remains|!talent.faeline_harmony)|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>10&cooldown.serenity.remains&(debuff.fae_exposure_damage.remains|!talent.faeline_harmony)|fight_remains<10" },
        { "irideus_fragment", ",if=(pet.xuen_the_white_tiger.active|!talent.invoke_xuen_the_white_tiger)&buff.serenity.up|fight_remains<25" },

        // Defaults:
        { "ITEM_STAT_BUFF", ",if=(pet.xuen_the_white_tiger.active|!talent.invoke_xuen_the_white_tiger)&buff.serenity.up" },
        { "ITEM_DMG_BUFF", ",if=!trinket.1.has_use_buff&!trinket.2.has_use_buff&(debuff.fae_exposure_damage.remains>5|!talent.faeline_harmony)|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>30&cooldown.serenity.remains&(debuff.fae_exposure_damage.remains>5|!talent.faeline_harmony)" },
      };

      //-------------------------------------------
      // SEF item map
      //-------------------------------------------
      const static std::unordered_map<std::string, std::string> sef_trinkets {
        // name_str -> APL
        { "algethar_puzzle_box", ",use_off_gcd=1,if=(pet.xuen_the_white_tiger.active|!talent.invoke_xuen_the_white_tiger)&!buff.storm_earth_and_fire.up|fight_remains<25" },
        { "erupting_spear_fragment", ",if=buff.serenity.up|(buff.invokers_delight.up&!talent.serenity)" },
        { "manic_grieftorch", ",use_off_gcd=1,if=!trinket.1.has_use_buff&!trinket.2.has_use_buff&!buff.storm_earth_and_fire.up&!pet.xuen_the_white_tiger.active&(debuff.fae_exposure_damage.remains>3|!talent.faeline_harmony)|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>30&(debuff.fae_exposure_damage.remains>2|!talent.faeline_harmony)|fight_remains<5" },
        { "beacon_to_the_beyond", ",use_off_gcd=1,if=!trinket.1.has_use_buff&!trinket.2.has_use_buff&!buff.storm_earth_and_fire.up&!pet.xuen_the_white_tiger.active&(debuff.fae_exposure_damage.remains>2|!talent.faeline_harmony)|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>30&(debuff.fae_exposure_damage.remains>2|!talent.faeline_harmony)|fight_remains<10" },
        { "djaruun_pillar_of_the_elder_flame", ",if=cooldown.fists_of_fury.remains<2&cooldown.invoke_xuen_the_white_tiger.remains>10|fight_remains<12" },
        { "dragonfire_bomb_dispenser", ",if=!trinket.1.has_use_buff&!trinket.2.has_use_buff&(debuff.fae_exposure_damage.remains|!talent.faeline_harmony)|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>10&(debuff.fae_exposure_damage.remains|!talent.faeline_harmony)|fight_remains<10" },
        { "irideus_fragment", ",if=(pet.xuen_the_white_tiger.active|!talent.invoke_xuen_the_white_tiger)&buff.storm_earth_and_fire.up|fight_remains<25" },

        // Defaults:
        { "ITEM_STAT_BUFF", ",if=(pet.xuen_the_white_tiger.active|!talent.invoke_xuen_the_white_tiger)&buff.storm_earth_and_fire.up" },
        { "ITEM_DMG_BUFF", ",if=!trinket.1.has_use_buff&!trinket.2.has_use_buff&(debuff.fae_exposure_damage.remains>5|!talent.faeline_harmony)|(trinket.1.has_use_buff|trinket.2.has_use_buff)&cooldown.invoke_xuen_the_white_tiger.remains>30&(debuff.fae_exposure_damage.remains>5|!talent.faeline_harmony)" },
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

    pre->add_action( "summon_white_tiger_statue" );
    pre->add_action( "expel_harm,if=chi<chi.max" );
    pre->add_action( "chi_burst,if=!talent.faeline_stomp" );
    pre->add_action( "chi_wave" );

    std::vector<std::string> racial_actions = p->get_racial_actions();
    action_priority_list_t *def = p->get_action_priority_list( "default" );
    action_priority_list_t *opener = p->get_action_priority_list( "opener" );
    action_priority_list_t *bdb_setup = p->get_action_priority_list( "bdb_setup" );
    action_priority_list_t *trinkets = p->get_action_priority_list( "trinkets" );
    action_priority_list_t *cd_sef = p->get_action_priority_list( "cd_sef" );
    action_priority_list_t *cd_serenity = p->get_action_priority_list( "cd_serenity" );
    action_priority_list_t *serenity = p->get_action_priority_list( "serenity" );
    action_priority_list_t *heavy_aoe = p->get_action_priority_list( "heavy_aoe" );
    action_priority_list_t *aoe = p->get_action_priority_list( "aoe" );
    action_priority_list_t *cleave = p->get_action_priority_list( "cleave" );
    action_priority_list_t *st_cleave = p->get_action_priority_list( "st_cleave" );
    action_priority_list_t *st = p->get_action_priority_list( "st" );
    action_priority_list_t *fallthru = p->get_action_priority_list( "fallthru" );

    def->add_action( "auto_attack" );
    def->add_action( "roll,if=movement.distance>5", "Move to target" );
    def->add_action( "chi_torpedo,if=movement.distance>5" );
    def->add_action( "flying_serpent_kick,if=movement.distance>5" );
    def->add_action( "spear_hand_strike,if=target.debuff.casting.react" );
    def->add_action(
      "variable,name=hold_xuen,op=set,value=!talent.invoke_xuen_the_white_tiger|cooldown.invoke_xuen_the_white_tiger.remains>fight_remains|fight_remains-cooldown.invoke_xuen_the_white_tiger.remains<120&((talent.serenity&fight_remains>cooldown.serenity.remains&cooldown.serenity.remains>10)|(cooldown.storm_earth_and_fire.full_recharge_time<fight_remains&cooldown.storm_earth_and_fire.full_recharge_time>15)|(cooldown.storm_earth_and_fire.charges=0&cooldown.storm_earth_and_fire.remains<fight_remains))" );

    // Potion
    if ( p->sim->allow_potions )
    {
      if ( monk->talent.windwalker.invoke_xuen_the_white_tiger->ok() )
        def->add_action(
        "potion,if=(buff.serenity.up|buff.storm_earth_and_fire.up)&pet.xuen_the_white_tiger.active|fight_remains<=60", "Potion" );
      else
        def->add_action( "potion,if=(buff.serenity.up|buff.storm_earth_and_fire.up)&fight_remains<=60", "Potion" );
    }

    // Build Chi at the start of combat
    if ( monk->talent.windwalker.invoke_xuen_the_white_tiger->ok() )
      def->add_action( "call_action_list,name=opener,if=time<4&chi<5&!pet.xuen_the_white_tiger.active&!talent.serenity", "Build Chi at the start of combat" );
    else
      def->add_action( "call_action_list,name=opener,if=time<4&chi<5&!talent.serenity", "Build Chi at the start of combat" );
    // Use Trinkets
    def->add_action( "call_action_list,name=trinkets", "Use Trinkets" );
    // Prioritize Faeline Stomp if playing with Faeline Harmony
    def->add_action( "faeline_stomp,target_if=min:debuff.fae_exposure_damage.remains,if=combo_strike&talent.faeline_harmony&debuff.fae_exposure_damage.remains<1", "Prioritize Faeline Stomp if playing with Faeline Harmony" );
    // Spend excess energy
    def->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=!buff.serenity.up&buff.teachings_of_the_monastery.stack<3&combo_strike&chi.max-chi>=(2+buff.power_strikes.up)&(!talent.invoke_xuen_the_white_tiger&!talent.serenity|((!talent.skyreach&!talent.skytouch)|time>5|pet.xuen_the_white_tiger.active))", "TP if not overcapping Chi or TotM" );
    //TP during serenity to activate skyreach
    def->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=active_enemies=1&buff.serenity.up&pet.xuen_the_white_tiger.active&!debuff.skyreach_exhaustion.up*20&combo_strike", "TP during serenity to activate skyreach" );
    // Use Chi Burst to reset Faeline Stomp
    def->add_action( "chi_burst,if=talent.faeline_stomp&cooldown.faeline_stomp.remains&(chi.max-chi>=1&active_enemies=1|chi.max-chi>=2&active_enemies>=2)&!talent.faeline_harmony", "Use Chi Burst to reset Faeline Stomp" );
    // Use Cooldowns
    def->add_action( "call_action_list,name=cd_sef,if=!talent.serenity", "Use Cooldowns" );
    def->add_action( "call_action_list,name=cd_serenity,if=talent.serenity" );

    // Serenity / Default priority
    def->add_action( "call_action_list,name=serenity,if=buff.serenity.up", "Serenity / Default Priority" );
    def->add_action( "call_action_list,name=heavy_aoe,if=active_enemies>4" );
    def->add_action( "call_action_list,name=aoe,if=active_enemies=4" );
    def->add_action( "call_action_list,name=cleave,if=active_enemies=3" );
    def->add_action( "call_action_list,name=st_cleave,if=active_enemies=2" );
    def->add_action( "call_action_list,name=st,if=active_enemies=1" );
    def->add_action( "call_action_list,name=fallthru" );


    // Trinkets
    for ( const auto &item : p->items )
    {
      if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
        trinkets->add_action( "use_item,name=" + item.name_str + _WW_ON_USE( item ) );
    }

    // Storm, Earth and Fire Cooldowns
    cd_sef->add_action( "summon_white_tiger_statue,if=!cooldown.invoke_xuen_the_white_tiger.remains|active_enemies>4|cooldown.invoke_xuen_the_white_tiger.remains>50|fight_remains<=30", "Storm, Earth and Fire Cooldowns" );
    cd_sef->add_action( "invoke_external_buff,name=power_infusion,if=pet.xuen_the_white_tiger.active",
      "Use <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> while <a href='https://www.wowhead.com/spell=123904/invoke-xuen-the-white-tiger'>Invoke Xuen, the White Tiger</a> is active." );
    cd_sef->add_action( "invoke_xuen_the_white_tiger,if=!variable.hold_xuen&target.time_to_die>25&talent.bonedust_brew&cooldown.bonedust_brew.remains<=5&(active_enemies<3&chi>=3|active_enemies>=3&chi>=2)|fight_remains<25" );
    cd_sef->add_action( "invoke_xuen_the_white_tiger,if=!variable.hold_xuen&target.time_to_die>25&!talent.bonedust_brew&(cooldown.rising_sun_kick.remains<2)&chi>=3" );
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
    cd_serenity->add_action( "summon_white_tiger_statue,if=!cooldown.invoke_xuen_the_white_tiger.remains|active_enemies>4|cooldown.invoke_xuen_the_white_tiger.remains>50|fight_remains<=30", "Serenity Cooldowns" );
    cd_serenity->add_action( "invoke_external_buff,name=power_infusion,if=pet.xuen_the_white_tiger.active",
      "Use <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> while <a href='https://www.wowhead.com/spell=123904/invoke-xuen-the-white-tiger'>Invoke Xuen, the White Tiger</a> is active." );
    cd_serenity->add_action( "invoke_xuen_the_white_tiger,if=!variable.hold_xuen&talent.bonedust_brew&cooldown.bonedust_brew.remains<=5&target.time_to_die>25|fight_remains<25" );
    cd_serenity->add_action( "invoke_xuen_the_white_tiger,if=!variable.hold_xuen&!talent.bonedust_brew&(cooldown.rising_sun_kick.remains<2)&target.time_to_die>25|fight_remains<25" );
    cd_serenity->add_action( "bonedust_brew,if=!buff.bonedust_brew.up&(cooldown.serenity.up|cooldown.serenity.remains>15|fight_remains<30&fight_remains>10)|fight_remains<10" );
    cd_serenity->add_action( "serenity,if=pet.xuen_the_white_tiger.active|!talent.invoke_xuen_the_white_tiger|fight_remains<15" );
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
    opener->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&chi.max-chi>=(2+buff.power_strikes.up)" );
    opener->add_action( "expel_harm,if=talent.chi_burst.enabled&chi=3" );
    opener->add_action( "chi_wave,if=chi.max-chi=2" );
    opener->add_action( "expel_harm" );
    opener->add_action( "chi_burst,if=chi>1&chi.max-chi>=2" );

    // Bonedust Brew Setup
    bdb_setup->add_action( "strike_of_the_windlord,if=talent.thunderfist&active_enemies>3", "Bonedust Brew Setup" );
    bdb_setup->add_action( "bonedust_brew,if=spinning_crane_kick.max&chi>=4" );
    bdb_setup->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&chi.max-chi>=2&buff.storm_earth_and_fire.up" );
    bdb_setup->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&!talent.whirling_dragon_punch&!spinning_crane_kick.max" );
    bdb_setup->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&chi>=5&talent.whirling_dragon_punch" );
    bdb_setup->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&active_enemies>=2&talent.whirling_dragon_punch" );
    
    // Serenity Priority
    serenity->add_action( "fists_of_fury,if=buff.serenity.remains<1", "Serenity Priority" );
    serenity->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&!spinning_crane_kick.max&active_enemies>4&talent.shdaowboxing_treads" );
    serenity->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&buff.teachings_of_the_monastery.stack=3&buff.teachings_of_the_monastery.remains<1" );
    serenity->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=active_enemies=4&buff.pressure_point.up&!talent.bonedust_brew" );
    serenity->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=active_enemies=1" );
    serenity->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=active_enemies<=3&buff.pressure_point.up" );
    serenity->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.pressure_point.up&set_bonus.tier30_2pc" );
    serenity->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=set_bonus.tier30_2pc" );
    serenity->add_action( "fists_of_fury,if=buff.invokers_delight.up&active_enemies<3&talent.Jade_Ignition,interrupt=1" );
    serenity->add_action( "fists_of_fury,if=buff.bloodlust.up,interrupt=1" );
    serenity->add_action( "fists_of_fury,if=buff.invokers_delight.up&active_enemies>4,interrupt=1" );
    serenity->add_action( "fists_of_fury,if=active_enemies=2,interrupt=1" );
    serenity->add_action( "fists_of_fury_cancel,target_if=max:target.time_to_die" );
    serenity->add_action( "strike_of_the_windlord,if=talent.thunderfist" );
    serenity->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up&active_enemies>=2" );
    serenity->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=active_enemies=4&buff.pressure_point.up" );
    serenity->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=active_enemies<6&combo_strike&set_bonus.tier30_2pc" );
    serenity->add_action( "spinning_crane_kick,if=combo_strike&active_enemies>=3&spinning_crane_kick.max" );
    serenity->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&active_enemies>1&active_enemies<4&buff.teachings_of_the_monastery.stack=2" );
    serenity->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up&active_enemies>=5" );
    serenity->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=talent.shadowboxing_treads&active_enemies>=3&combo_strike" );
    serenity->add_action( "spinning_crane_kick,if=combo_strike&(active_enemies>3|active_enemies>2&spinning_crane_kick.modifier>=2.3)" );
    serenity->add_action( "strike_of_the_windlord,if=active_enemies>=3" );
    serenity->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=active_enemies=2&cooldown.fists_of_fury.remains>5" );
    serenity->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=active_enemies=2&cooldown.fists_of_fury.remains>5&talent.shadowboxing_treads&buff.teachings_of_the_monastery.stack=1&combo_strike" );
    serenity->add_action( "spinning_crane_kick,if=combo_strike&active_enemies>1" );
    serenity->add_action( "whirling_dragon_punch,if=active_enemies>1" );
    serenity->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up&active_enemies>=3" );
    serenity->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains" );
    serenity->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up" );
    serenity->add_action( "blackout_kick,if=combo_strike" );
    serenity->add_action( "whirling_dragon_punch" );
    serenity->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=talent.teachings_of_the_monastery&buff.teachings_of_the_monastery.stack<3" );
    
    // >4 Target priority
    heavy_aoe->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up&spinning_crane_kick.max", ">4 Targets" );
    heavy_aoe->add_action( "strike_of_the_windlord,if=talent.thunderfist" );
    heavy_aoe->add_action( "whirling_dragon_punch,if=active_enemies>8" );
    heavy_aoe->add_action( "fists_of_fury" );
    heavy_aoe->add_action( "spinning_crane_kick,if=buff.bonedust_brew.up&combo_strike&spinning_crane_kick.max" );
    heavy_aoe->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.bonedust_brew.up&buff.pressure_point.up&set_bonus.tier30_2pc" );
    heavy_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=3&talent.shadowboxing_treads" );
    heavy_aoe->add_action( "whirling_dragon_punch,if=active_enemies>=5" );
    heavy_aoe->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.pressure_point.up&set_bonus.tier30_2pc" );
    heavy_aoe->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=set_bonus.tier30_2pc" );
    heavy_aoe->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=talent.whirling_dragon_punch&cooldown.whirling_dragon_punch.remains<3&cooldown.fists_of_fury.remains>3&!buff.kicks_of_flowing_momentum.up" );
    heavy_aoe->add_action( "spinning_crane_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&cooldown.fists_of_fury.remains<5&buff.chi_energy.stack>10" );
    heavy_aoe->add_action( "spinning_crane_kick,if=combo_strike&(cooldown.fists_of_fury.remains>3|chi>2)&spinning_crane_kick.max&buff.bloodlust.up" );
    heavy_aoe->add_action( "spinning_crane_kick,if=combo_strike&(cooldown.fists_of_fury.remains>3|chi>2)&spinning_crane_kick.max&buff.invokers_delight.up" );
    heavy_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=talent.shadowboxing_treads&combo_strike&set_bonus.tier30_2pc&!buff.bonedust_brew.up&active_enemies<15&!talent.crane_vortex" );
    heavy_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=talent.shadowboxing_treads&combo_strike&set_bonus.tier30_2pc&!buff.bonedust_brew.up&active_enemies<8" );
    heavy_aoe->add_action( "spinning_crane_kick,if=combo_strike&(cooldown.fists_of_fury.remains>3|chi>4)&spinning_crane_kick.max" );
    heavy_aoe->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    heavy_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=3" );
    heavy_aoe->add_action( "strike_of_the_windlord" );
    heavy_aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=talent.shadowboxing_treads&combo_strike&!spinning_crane_kick.max" );
    heavy_aoe->add_action( "chi_burst,if=chi.max-chi>=1&active_enemies=1&raid_event.adds.in>20|chi.max-chi>=2" );

    // 4 Target priority
    aoe->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up&spinning_crane_kick.max", "4 Targets" );
    aoe->add_action( "strike_of_the_windlord,if=talent.thunderfist" );
    aoe->add_action( "fists_of_fury" );
    aoe->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.bonedust_brew.up&buff.pressure_point.up&set_bonus.tier30_2pc" );
    aoe->add_action( "spinning_crane_kick,if=buff.bonedust_brew.up&combo_strike&spinning_crane_kick.max" );
    aoe->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=!buff.bonedust_brew.up&buff.pressure_point.up&cooldown.fists_of_fury.remains>5" );
    aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=3&talent.shadowboxing_treads" );
    aoe->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=set_bonus.tier30_2pc" );
    aoe->add_action( "spinning_crane_kick,if=combo_strike&cooldown.fists_of_fury.remains>3&buff.chi_energy.stack>10" );
    aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&set_bonus.tier30_2pc" );
    aoe->add_action( "spinning_crane_kick,if=combo_strike&(cooldown.fists_of_fury.remains>3|chi>4)&spinning_crane_kick.max" );
    aoe->add_action( "whirling_dragon_punch" );
    aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=3" );
    aoe->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    aoe->add_action( "strike_of_the_windlord" );
    aoe->add_action( "spinning_crane_kick,if=combo_strike&(cooldown.fists_of_fury.remains>3|chi>4)" );
    aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike" );

    // 3 Target priority
    cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=3&talent.shadowboxing_treads", "3 Targets" );
    cleave->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up" );
    cleave->add_action( "strike_of_the_windlord,if=talent.thunderfist" );
    cleave->add_action( "fists_of_fury" );
    cleave->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.bonedust_brew.up&buff.pressure_point.up&set_bonus.tier30_2pc" );
    cleave->add_action( "spinning_crane_kick,if=buff.bonedust_brew.up&combo_strike" );
    cleave->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=!buff.bonedust_brew.up&buff.pressure_point.up" );
    cleave->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=set_bonus.tier30_2pc" );
    cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=2" );
    cleave->add_action( "strike_of_the_windlord" );
    cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.up&(talent.shadowboxing_treads|cooldown.rising_sun_kick.remains>1)" );
    cleave->add_action( "whirling_dragon_punch" );
    cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=3" );
    cleave->add_action( "spinning_crane_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&cooldown.fists_of_fury.remains<3&buff.chi_energy.stack>15" );
    cleave->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=cooldown.fists_of_fury.remains>4&chi>3" );
    cleave->add_action( "spinning_crane_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&cooldown.rising_sun_kick.remains&cooldown.fists_of_fury.remains&chi>4&((talent.storm_earth_and_fire&!talent.bonedust_brew)|(talent.serenity))" );
    cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&cooldown.fists_of_fury.remains" );
    cleave->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&talent.shadowboxing_treads&!spinning_crane_kick.max" );
    cleave->add_action( "spinning_crane_kick,target_if=min:debuff.mark_of_the_crane.remains,if=(combo_strike&chi>5&talent.storm_earth_and_fire|combo_strike&chi>4&talent.serenity)" );

    // 2 Target priority
    st_cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=3&talent.shadowboxing_treads", "2 Targets" );
    st_cleave->add_action( "strike_of_the_windlord,if=talent.thunderfist" );
    st_cleave->add_action( "fists_of_fury" );
    st_cleave->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=set_bonus.tier30_2pc" );
    st_cleave->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.kicks_of_flowing_momentum.up|buff.pressure_point.up" );
    st_cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=2" );
    st_cleave->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up" );
    st_cleave->add_action( "strike_of_the_windlord" );
    st_cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.up&(talent.shadowboxing_treads|cooldown.rising_sun_kick.remains>1)" );
    st_cleave->add_action( "whirling_dragon_punch" );
    st_cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=3" );
    st_cleave->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=!talent.shadowboxing_treads&cooldown.fists_of_fury.remains>4&talent.xuens_battlegear" );
    st_cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&cooldown.rising_sun_kick.remains&cooldown.fists_of_fury.remains&(!buff.bonedust_brew.up|spinning_crane_kick.modifier<1.5)" );
    st_cleave->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    st_cleave->add_action( "spinning_crane_kick,if=buff.bonedust_brew.up&combo_strike&spinning_crane_kick.modifier>=2.7" );
    st_cleave->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains" );
    st_cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike" );
    st_cleave->add_action( "spinning_crane_kick,target_if=min:debuff.mark_of_the_crane.remains,if=(combo_strike&chi>5&talent.storm_earth_and_fire|combo_strike&chi>4&talent.serenity)" );

    // 1 Target priority
    st->add_action( "fists_of_fury,if=!buff.pressure_point.up&!cooldown.rising_sun_kick.remains", "1 Target");
    st->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.pressure_point.up" );
    st->add_action( "strike_of_the_windlord,if=talent.thunderfist&(cooldown.invoke_xuen_the_white_tiger.remains>10|fight_remains<5)" );
    st->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.kicks_of_flowing_momentum.up|buff.pressure_point.up" );
    st->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=3");
    st->add_action( "fists_of_fury" );
    st->add_action( "rising_sun_kick" );
    st->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=2" );
    st->add_action( "strike_of_the_windlord,if=debuff.skyreach_exhaustion.remains>30|fight_remains<5" );
    st->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up" );
    st->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.up&cooldown.rising_sun_kick.remains>1" );
    st->add_action( "spinning_crane_kick,if=buff.bonedust_brew.up&combo_strike&spinning_crane_kick.modifier>=2.7" );
    st->add_action( "whirling_dragon_punch" );
    st->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    st->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike" );


    // Fallthru
    fallthru->add_action( "crackling_jade_lightning,if=buff.the_emperors_capacitor.stack>19&energy.time_to_max>execute_time-1&cooldown.rising_sun_kick.remains>execute_time|buff.the_emperors_capacitor.stack>14&(cooldown.serenity.remains<5&talent.serenity|fight_remains<5)", "Fallthru" );
    fallthru->add_action( "faeline_stomp,if=combo_strike" );
    fallthru->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&chi.max-chi>=(2+buff.power_strikes.up)" );
    fallthru->add_action( "expel_harm,if=chi.max-chi>=1&active_enemies>2" );
    fallthru->add_action( "chi_burst,if=chi.max-chi>=1&active_enemies=1&raid_event.adds.in>20|chi.max-chi>=2&active_enemies>=2" );
    fallthru->add_action( "chi_wave" );
    fallthru->add_action( "expel_harm,if=chi.max-chi>=1" );
    fallthru->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&active_enemies>=5" );
    fallthru->add_action( "spinning_crane_kick,if=combo_strike&buff.chi_energy.stack>30-5*active_enemies&buff.storm_earth_and_fire.down&(cooldown.rising_sun_kick.remains>2&cooldown.fists_of_fury.remains>2|cooldown.rising_sun_kick.remains<3&cooldown.fists_of_fury.remains>3&chi>3|cooldown.rising_sun_kick.remains>3&cooldown.fists_of_fury.remains<3&chi>4|chi.max-chi<=1&energy.time_to_max<2)|buff.chi_energy.stack>10&fight_remains<7" );
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
