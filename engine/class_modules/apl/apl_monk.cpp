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

  std::string flask( const player_t* p )
  {
    switch ( p->specialization() )
    {
      case MONK_BREWMASTER:
        if ( p->true_level > 60 )
          return "phial_of_static_empowerment_3";
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
          return "phial_of_static_empowerment_3";
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

  std::string food( const player_t* p )
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

  std::string rune( const player_t* p )
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

  std::string temporary_enchant( const player_t* p )
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
          return "main_hand:primal_whetstone_3/off_hand:primal_whetstone_3";
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

  void brewmaster( player_t* p )
  {
    std::vector<std::string> racial_actions = p->get_racial_actions();
    action_priority_list_t* pre = p->get_action_priority_list( "precombat" );
    action_priority_list_t* def = p->get_action_priority_list( "default" );
    action_priority_list_t* cooldowns_improved_niuzao_woo = p->get_action_priority_list( "cooldowns_improved_niuzao_woo" );
    action_priority_list_t* cooldowns_improved_niuzao_cta = p->get_action_priority_list( "cooldowns_improved_niuzao_cta" );
    action_priority_list_t* cooldowns_niuzao_woo = p->get_action_priority_list( "cooldowns_niuzao_woo" );

    action_priority_list_t* rotation_boc      = p->get_action_priority_list( "rotation_boc" );
    action_priority_list_t* rotation_fom_boc  = p->get_action_priority_list( "rotation_fom_boc" );
    action_priority_list_t* rotation_chp_dfb  = p->get_action_priority_list( "rotation_chp_dfb" );
    action_priority_list_t* rotation_fallback = p->get_action_priority_list( "rotation_fallback" );


    // Flask
    pre->add_action( "flask" );

    // Food
    pre->add_action( "food" );

    // Rune
    pre->add_action( "augmentation" );

    // Snapshot stats
    pre->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

    pre->add_action( "potion" );

    pre->add_action( "chi_burst,if=talent.chi_burst.enabled" );
    pre->add_action( "chi_wave,if=talent.chi_wave.enabled" );

    // ---------------------------------
    // PRE-COMBAT VARIABLE CONFIGURATION
    // ---------------------------------
    // Blackout Combo
    pre->add_action( "variable,name=boc_count,op=set,value=0", "Blackout Combo" );
    // To account for some strange behaviour, these are in combat, and not strictly pre-combat.
    def->add_action( "variable,op=set,name=rotation_selection,value=0", "Rotation Selection" );
    def->add_action( "variable,op=set,name=rotation_selection,value=1,if=(talent.charred_passions.enabled|talent.dragonfire_brew.enabled)&talent.salsalabims_strength.enabled" );
    def->add_action( "variable,op=set,name=rotation_selection,value=2,if=(talent.charred_passions.enabled|talent.dragonfire_brew.enabled)&talent.salsalabims_strength.enabled&talent.blackout_combo.enabled" );
    def->add_action( "variable,op=set,name=rotation_selection,value=3-variable.rotation_selection" );

    // ---------------------------------
    // BEGIN ACTIONS
    // ---------------------------------

    def->add_action( "auto_attack" );
    def->add_action( "spear_hand_strike,if=target.debuff.casting.react" );

    // Base DPS Cooldowns
    def->add_action( "summon_white_tiger_statue,if=talent.summon_white_tiger_statue.enabled", "Base DPS Cooldowns" );
    def->add_action( "touch_of_death" );
    def->add_action( "bonedust_brew,if=!debuff.bonedust_brew_debuff.up&talent.bonedust_brew.enabled" );

    // On-Use Effects
    if ( p->items[SLOT_MAIN_HAND].name_str == "jotungeirr_destinys_call" )
      def->add_action( "use_item,name=" + p->items[SLOT_MAIN_HAND].name_str );

    for ( const auto& item : p->items )
    {
      if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
      {
        if ( item.name_str == "scars_of_fraternal_strife" )
          def->add_action( "use_item,name=" + item.name_str + ",if=!buff.scars_of_fraternal_strife_4.up&time>1" );
        else if ( item.name_str == "cache_of_acquired_treasures" )
          def->add_action( "use_item,name=" + item.name_str + ",if=buff.acquired_axe.up|fight_remains<25" );
        else if ( item.name_str == "jotungeirr_destinys_call" )
          continue;
        else
          def->add_action( "use_item,name=" + item.name_str );
      }
    }

    def->add_action( "potion" );

    for ( const auto& racial_action : racial_actions )
    {
      if ( racial_action != "arcane_torrent" )
        def->add_action( racial_action );
    }

    // Cooldown Action Lists
    def->add_action( "call_action_list,name=cooldowns_improved_niuzao_woo,if=(talent.invoke_niuzao_the_black_ox.enabled+talent.improved_invoke_niuzao_the_black_ox.enabled)=2&(talent.weapons_of_order.enabled+talent.call_to_arms.enabled)<=1",
      "Cooldown Action Lists" );
    def->add_action( "call_action_list,name=cooldowns_improved_niuzao_cta,if=(talent.invoke_niuzao_the_black_ox.enabled+talent.improved_invoke_niuzao_the_black_ox.enabled)=2&(talent.weapons_of_order.enabled+talent.call_to_arms.enabled)=2" );
    def->add_action( "call_action_list,name=cooldowns_niuzao_woo,if=(talent.invoke_niuzao_the_black_ox.enabled+talent.improved_invoke_niuzao_the_black_ox.enabled)<=1" );

    // Rotation Action Lists
    def->add_action("call_action_list,name=rotation_boc,if=variable.rotation_selection=1&(((1%spell_haste-1)*100>=1%3&talent.fluidity_of_motion.enabled)|!talent.fluidity_of_motion.enabled)", "Rotation Action Lists" );
    def->add_action("call_action_list,name=rotation_fom_boc,if=variable.rotation_selection=1&((1%spell_haste-1)*100<1%3&talent.fluidity_of_motion.enabled)" );
    def->add_action("call_action_list,name=rotation_chp_dfb,if=variable.rotation_selection=2" );
    def->add_action("call_action_list,name=rotation_fallback,if=variable.rotation_selection=3", "Fallback Rotation" );

    // ---------------------------------
    // DPS COOLDOWNS
    // ---------------------------------

    // Name: Niuzao + Weapons of Order / Niuzao + Call to Arms
    cooldowns_niuzao_woo->add_action( "weapons_of_order,if=talent.weapons_of_order.enabled",
      "Name: Niuzao + Weapons of Order / Niuzao + Call to Arms" );
    cooldowns_niuzao_woo->add_action( "invoke_external_buff,name=power_infusion,if=buff.weapons_of_order.remains<=20" );
    cooldowns_niuzao_woo->add_action( "invoke_niuzao_the_black_ox,if=talent.invoke_niuzao_the_black_ox.enabled&buff.weapons_of_order.remains<=16&talent.weapons_of_order.enabled" );
    cooldowns_niuzao_woo->add_action( "invoke_niuzao_the_black_ox,if=talent.invoke_niuzao_the_black_ox.enabled&!talent.weapons_of_order.enabled" );
    cooldowns_niuzao_woo->add_action( "purifying_brew,if=talent.purifying_brew.enabled&stagger.amounttototalpct>=0.7&!buff.blackout_combo.up" );
    cooldowns_niuzao_woo->add_action( "purifying_brew,if=talent.purifying_brew.enabled&cooldown.purifying_brew.remains_expected<5&!buff.blackout_combo.up" );

    /*
     * Process Overview:
     * snapshot purifies in cta, or niuzao into pb_in_window
     * decrement pb_in_window if a pb is cast in window
     * evenly cast pb across window
     * dump excess pbs at end of niuzao window if available(excess at end of CtA will be consumed by niuzao, as they tend to be pretty well synced with wwto)
     * pre - purify if WoO is up in 4 or fewer seconds, and the last purification was longer ago than half a gcd after next woo is expected to be usable(a lil latency fudge factor)
     * WoO if pre - purify buff exists
     * pre - purify if last CtA stomp has occurred, and niuzao will be cast in <= $( pre_pb_wiggle )
     * niuzao if enough time has passed, and most recent stomp is older than most recent purification(indirectly via woo last used + small error.pet last used isn't a thing?)
     * consume excess pbs
     */

     // Name: Improved Niuzao + Weapons of Order
    cooldowns_improved_niuzao_woo->add_action( "variable,name=pb_in_window,op=set,value=floor(cooldown.purifying_brew.charges_fractional+(20+4*0.05)%cooldown.purifying_brew.duration%0.65),if=prev.invoke_niuzao_the_black_ox",
      "Name: Improved Niuzao + Weapons of Order" );
    cooldowns_improved_niuzao_woo->add_action( "variable,name=pb_in_window,op=set,value=floor(cooldown.purifying_brew.charges_fractional+(20+4*0.05)%cooldown.purifying_brew.duration%0.65),if=prev.invoke_niuzao_the_black_ox" );
    cooldowns_improved_niuzao_woo->add_action( "purifying_brew,if=talent.purifying_brew.enabled&(time-action.purifying_brew.last_used>=20+4*0.05-time+action.invoke_niuzao_the_black_ox.last_used%variable.pb_in_window&time-action.invoke_niuzao_the_black_ox.last_used<=20+4*0.05)" );
    cooldowns_improved_niuzao_woo->add_action( "purifying_brew,use_off_gcd=1,if=talent.purifying_brew.enabled&(variable.pb_in_window=0&20+4*0.05-time+action.invoke_niuzao_the_black_ox.last_used<1&20+4*0.05-time+action.invoke_niuzao_the_black_ox.last_used>0)" );
    cooldowns_improved_niuzao_woo->add_action( "weapons_of_order,if=talent.weapons_of_order.enabled" );
    cooldowns_improved_niuzao_woo->add_action( "invoke_external_buff,name=power_infusion,if=buff.weapons_of_order.remains<=20" );
    cooldowns_improved_niuzao_woo->add_action( "purifying_brew,if=talent.purifying_brew.enabled&cooldown.invoke_niuzao_the_black_ox.remains<=3.5&time-action.purifying_brew.last_used>=3.5+cooldown.invoke_niuzao_the_black_ox.remains" );
    cooldowns_improved_niuzao_woo->add_action( "invoke_niuzao_the_black_ox,if=talent.invoke_niuzao_the_black_ox.enabled&time-action.purifying_brew.last_used<=5" );
    cooldowns_improved_niuzao_woo->add_action( "purifying_brew,if=talent.purifying_brew.enabled&cooldown.purifying_brew.full_recharge_time*2<=cooldown.invoke_niuzao_the_black_ox.remains-3.5" );

    // Name: Improved Niuzao + Call to Arms
    cooldowns_improved_niuzao_cta->add_action( "variable,name=pb_in_window,op=set,value=floor(cooldown.purifying_brew.charges_fractional+(10+2*0.05)%cooldown.purifying_brew.duration%0.65),if=prev.weapons_of_order",
      "Name: Improved Niuzao + Call to Arms" );
    cooldowns_improved_niuzao_cta->add_action( "variable,name=pb_in_window,op=set,value=floor(cooldown.purifying_brew.charges_fractional+(20+4*0.05)%cooldown.purifying_brew.duration%0.65),if=prev.invoke_niuzao_the_black_ox" );
    cooldowns_improved_niuzao_cta->add_action( "variable,name=pb_in_window,op=sub,value=1,if=prev.purifying_brew&(time-action.weapons_of_order.last_used<=10+2*0.05|time-action.invoke_niuzao_the_black_ox.last_used<=20+4*0.05)" );
    cooldowns_improved_niuzao_cta->add_action( "purifying_brew,if=talent.purifying_brew.enabled&(time-action.purifying_brew.last_used>=10+2*0.05-time+action.weapons_of_order.last_used%variable.pb_in_window&time-action.weapons_of_order.last_used<=10+2*0.05)" );
    cooldowns_improved_niuzao_cta->add_action( "purifying_brew,if=talent.purifying_brew.enabled&(time-action.purifying_brew.last_used>=20+4*0.05-time+action.invoke_niuzao_the_black_ox.last_used%variable.pb_in_window&time-action.invoke_niuzao_the_black_ox.last_used<=20+4*0.05)" );
    cooldowns_improved_niuzao_cta->add_action( "purifying_brew,use_off_gcd=1,if=talent.purifying_brew.enabled&(variable.pb_in_window=0&20+4*0.05-time+action.invoke_niuzao_the_black_ox.last_used<1&20+4*0.05-time+action.invoke_niuzao_the_black_ox.last_used>0)" );
    cooldowns_improved_niuzao_cta->add_action( "purifying_brew,if=talent.purifying_brew.enabled&cooldown.weapons_of_order.remains<=3.5&time-action.purifying_brew.last_used>=3.5+cooldown.weapons_of_order.remains" );
    cooldowns_improved_niuzao_cta->add_action( "weapons_of_order,if=talent.weapons_of_order.enabled&time-action.purifying_brew.last_used<=5" );
    cooldowns_improved_niuzao_cta->add_action( "invoke_external_buff,name=power_infusion,if=buff.weapons_of_order.remains<=20" );
    cooldowns_improved_niuzao_cta->add_action( "purifying_brew,if=talent.purifying_brew.enabled&cooldown.invoke_niuzao_the_black_ox.remains<=3.5&time-action.purifying_brew.last_used>=3.5+cooldown.invoke_niuzao_the_black_ox.remains&buff.weapons_of_order.remains<=30-17" );
    cooldowns_improved_niuzao_cta->add_action( "invoke_niuzao_the_black_ox,if=talent.invoke_niuzao_the_black_ox.enabled&buff.weapons_of_order.remains<=30-17&action.purifying_brew.last_used>action.weapons_of_order.last_used+10+2*0.05" );
    cooldowns_improved_niuzao_cta->add_action( "purifying_brew,if=talent.purifying_brew.enabled&cooldown.purifying_brew.full_recharge_time*2<=cooldown.weapons_of_order.remains-3.5&cooldown.purifying_brew.full_recharge_time*2<=cooldown.invoke_niuzao_the_black_ox.remains-3.5" );

    // ---------------------------------
    // ROTATIONS
    // ---------------------------------

    // Name: Blackout Combo Salsalabim's Strength Charred Passions [Shadowboxing Treads or high haste Fluidity of Motion]
    // Basic Sequence: Blackout Kick Breath of Fire x x Blackout Kick Keg Smash x x
    // Fluidity of Motion Sequence: Blackout Kick Breath of Fire x Blackout Kick Keg Smash x
    rotation_boc->add_action( "blackout_kick",
      "Name: Blackout Combo Salsalabim's Strength Charred Passions [Shadowboxing Treads or high haste Fluidity of Motion]" );
    rotation_boc->add_action( "rising_sun_kick,if=talent.rising_sun_kick.enabled" );
    rotation_boc->add_action( "breath_of_fire,if=buff.blackout_combo.up" );
    rotation_boc->add_action( "keg_smash,if=buff.blackout_combo.up" );
    rotation_boc->add_action( "exploding_keg,if=talent.exploding_keg.enabled" );
    rotation_boc->add_action( "rushing_jade_wind,if=buff.rushing_jade_wind.down&talent.rushing_jade_wind.enabled" );
    rotation_boc->add_action( "black_ox_brew,if=energy+energy.regen*(variable.time_to_scheduled_ks+execute_time)>=65&talent.black_ox_brew.enabled" );
    rotation_boc->add_action( "keg_smash,if=cooldown.keg_smash.charges_fractional>1.5" );
    rotation_boc->add_action( "spinning_crane_kick,if=energy+energy.regen*(variable.time_to_scheduled_ks+execute_time)>=65&active_enemies>1" );
    rotation_boc->add_action( "tiger_palm,if=energy+energy.regen*(variable.time_to_scheduled_ks+execute_time)>=65&active_enemies=1&!buff.blackout_combo.up" );
    rotation_boc->add_action( "celestial_brew,if=talent.celestial_brew.enabled&!buff.blackout_combo.up" );
    rotation_boc->add_action( "chi_wave,if=talent.chi_wave.enabled" );
    rotation_boc->add_action( "chi_burst,if=talent.chi_burst.enabled" );

    // Name: Blackout Combo Salsalabim's Strength Chared Passions Fluidity of Motion Not High Haste
    // Basic Sequence: Blackout Kick Breath of Fire x Blackout Kick (Keg Smash / x) x Blackout Kick Keg Smash x
    rotation_fom_boc->add_action( "variable,name=boc_count,op=add,value=1,if=prev.blackout_kick",
      "Name: Blackout Combo Salsalabim's Strength Chared Passions Fluidity of Motion Not High Haste" );
    rotation_fom_boc->add_action( "variable,name=time_to_scheduled_ks,op=set,value=cooldown.blackout_kick.duration_expected*(1-(variable.boc_count)%%3)+cooldown.blackout_kick.remains+1" );
    rotation_fom_boc->add_action( "blackout_kick" );
    rotation_fom_boc->add_action( "rising_sun_kick,if=variable.boc_count%%3=1&talent.rising_sun_kick.enabled" );
    rotation_fom_boc->add_action( "breath_of_fire,if=buff.blackout_combo.up&variable.boc_count%%3=1" );
    rotation_fom_boc->add_action( "keg_smash,if=buff.blackout_combo.up&variable.boc_count%%3=2" );
    rotation_fom_boc->add_action( "keg_smash,if=buff.blackout_combo.up&variable.boc_count%%3=0&cooldown.keg_smash.charges_fractional>1&cooldown.keg_smash.full_recharge_time<=variable.time_to_scheduled_ks&energy+energy.regen*(variable.time_to_scheduled_ks+execute_time)>=80" );
    rotation_fom_boc->add_action( "cancel_buff,name=blackout_combo,if=variable.boc_count%%3=0" );
    rotation_fom_boc->add_action( "exploding_keg,if=talent.exploding_keg.enabled" );
    rotation_fom_boc->add_action( "rushing_jade_wind,if=buff.rushing_jade_wind.down&talent.rushing_jade_wind.enabled" );
    rotation_fom_boc->add_action( "black_ox_brew,if=energy+energy.regen*(variable.time_to_scheduled_ks+execute_time)>=65&talent.black_ox_brew.enabled" );
    rotation_fom_boc->add_action( "rising_sun_kick,if=talent.rising_sun_kick.enabled" );
    rotation_fom_boc->add_action( "spinning_crane_kick,if=energy+energy.regen*(variable.time_to_scheduled_ks+execute_time)>=65&buff.charred_passions.up&active_enemies>1" );
    rotation_fom_boc->add_action( "tiger_palm,if=energy+energy.regen*(variable.time_to_scheduled_ks+execute_time)>=65&active_enemies=1&!buff.blackout_combo.up" );
    rotation_fom_boc->add_action( "celestial_brew,if=!buff.blackout_combo.up" );
    rotation_fom_boc->add_action( "chi_wave,if=talent.chi_wave.enabled" );
    rotation_fom_boc->add_action( "chi_burst,if=talent.chi_burst.enabled" );

    // Name: Salsalabim's Strength Charred Passions / Dragonfire Brew
    // Basic Sequence: Keg Smash Breath of Fire x x x x x
    rotation_chp_dfb->add_action( "breath_of_fire,if=talent.charred_passions.enabled&buff.charred_passions.remains<1.5|talent.dragonfire_brew.enabled",
      "Name: Salsalabim's Strength Charred Passions / Dragonfire Brew" );
    rotation_chp_dfb->add_action( "rushing_jade_wind,if=buff.rushing_jade_wind.down&talent.rushing_jade_wind.enabled" );
    rotation_chp_dfb->add_action( "blackout_kick" );
    rotation_chp_dfb->add_action( "keg_smash" );
    rotation_chp_dfb->add_action( "exploding_keg,if=talent.exploding_keg.enabled" );
    rotation_chp_dfb->add_action( "black_ox_brew,if=energy+energy.regen*(variable.time_to_scheduled_ks+execute_time)>=65&talent.black_ox_brew.enabled" );
    rotation_chp_dfb->add_action( "rising_sun_kick" );
    rotation_chp_dfb->add_action( "spinning_crane_kick,if=energy+energy.regen*(cooldown.keg_smash.remains+execute_time)>=65&active_enemies>1" );
    rotation_chp_dfb->add_action( "tiger_palm,if=energy+energy.regen*(cooldown.keg_smash.remains+execute_time)>=65&active_enemies=1" );
    rotation_chp_dfb->add_action( "chi_wave,if=talent.chi_wave.enabled" );
    rotation_chp_dfb->add_action( "chi_burst,if=talent.chi_burst.enabled" );
    rotation_chp_dfb->add_action( "celestial_brew" );

    // Name: Fallback
    rotation_fallback->add_action( "rising_sun_kick,if=talent.rising_sun_kick.enabled",
      "Name: Fallback" );
    rotation_fallback->add_action( "keg_smash" );
    rotation_fallback->add_action( "breath_of_fire,if=talent.breath_of_fire.enabled" );
    rotation_fallback->add_action( "blackout_kick" );
    rotation_fallback->add_action( "exploding_keg,if=talent.exploding_keg.enabled" );
    rotation_fallback->add_action( "black_ox_brew,if=energy+energy.regen*(cooldown.keg_smash.remains+execute_time)>=65&talent.black_ox_brew.enabled" );
    rotation_fallback->add_action( "rushing_jade_wind,if=talent.rushing_jade_wind.enabled" );
    rotation_fallback->add_action( "spinning_crane_kick,if=energy+energy.regen*(cooldown.keg_smash.remains+execute_time)>=65" );
    rotation_fallback->add_action( "celestial_brew,if=!buff.blackout_combo.up&talent.celestial_brew.enabled" );
    rotation_fallback->add_action( "chi_wave,if=talent.chi_wave.enabled" );
    rotation_fallback->add_action( "chi_burst,if=talent.chi_burst.enabled" );
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

    pre->add_action( p, "chi_burst" );
    pre->add_action( p, "chi_wave" );

    std::vector<std::string> racial_actions = p->get_racial_actions();
    action_priority_list_t* def = p->get_action_priority_list( "default" );
    action_priority_list_t* st = p->get_action_priority_list( "st" );
    action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );

    def->add_action( "auto_attack" );

    if ( p->items[SLOT_MAIN_HAND].name_str == "jotungeirr_destinys_call" )
      def->add_action( "use_item,name=" + p->items[SLOT_MAIN_HAND].name_str );

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

  void windwalker( player_t* p )
  {
    auto monk = debug_cast<monk::monk_t*>( p );

    //============================================================================
    // On-use Items
    //============================================================================
    auto _WW_ON_USE = [ monk ] ( const item_t& item )
    {
      //-------------------------------------------
      // Serenity item map
      //-------------------------------------------
      const static std::unordered_map<std::string, std::string> serenity_trinkets {
        // name_str -> APL
        { "horn_of_valor",",if=pet.xuen_the_white_tiger.active|!talent.invoke_xuen_the_white_tiger&buff.serenity.up|fight_remains<30" },

        // Defaults:
        { "ITEM_STAT_BUFF", ",if=buff.serenity.remains>10" },
        { "ITEM_DMG_BUFF", ",if=cooldown.invoke_xuen_the_white_tiger.remains>cooldown%%120|cooldown<=60&variable.hold_xuen|!talent.invoke_xuen_the_white_tiger" },
      };

      //-------------------------------------------
      // SEF item map
      //-------------------------------------------
      const static std::unordered_map<std::string, std::string> sef_trinkets {
        // name_str -> APL
        { "horn_of_valor",",if=pet.xuen_the_white_tiger.active|!talent.invoke_xuen_the_white_tiger&buff.storm_earth_and_fire.up|fight_remains<30" },

        // Defaults:
        { "ITEM_STAT_BUFF", ",if=cooldown.invoke_xuen_the_white_tiger.remains>cooldown%%120|cooldown<=60&variable.hold_xuen|cooldown<=60&buff.storm_earth_and_fire.remains>10|!talent.invoke_xuen_the_white_tiger" },
        { "ITEM_DMG_BUFF", ",if=cooldown.invoke_xuen_the_white_tiger.remains>cooldown%%120|cooldown<=60&variable.hold_xuen|!talent.invoke_xuen_the_white_tiger" },
      };

      // -----------------------------------------

      std::string concat = "";
      auto talent_map = monk->talent.windwalker.serenity->ok() ? serenity_trinkets : sef_trinkets;
      try { concat = talent_map.at( item.name_str ); }
      catch ( ... )
      {

        int duration = 0;

        for ( auto e : item.parsed.special_effects )
        {

          int duration = (int) floor( e->duration().total_seconds() );

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

    action_priority_list_t* pre = p->get_action_priority_list( "precombat" );

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
    action_priority_list_t* def = p->get_action_priority_list( "default" );
    action_priority_list_t* opener = p->get_action_priority_list( "opener" );
    action_priority_list_t* bdb_setup = p->get_action_priority_list( "bdb_setup" );
    action_priority_list_t* cd_sef = p->get_action_priority_list( "cd_sef" );
    action_priority_list_t* cd_serenity = p->get_action_priority_list( "cd_serenity" );
    action_priority_list_t* serenity = p->get_action_priority_list( "serenity" );
    action_priority_list_t* st_cleave = p->get_action_priority_list( "st_cleave" );
    action_priority_list_t* aoe = p->get_action_priority_list( "aoe" );
    action_priority_list_t* fallthru = p->get_action_priority_list( "fallthru" );

    def->add_action( "auto_attack" );
    def->add_action( p, "Spear Hand Strike", "if=target.debuff.casting.react" );
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

    // Prioritize Faeline Stomp if playing with Faeline Harmony
    def->add_action( "faeline_stomp,target_if=min:debuff.fae_exposure_damage.remains,if=combo_strike&talent.faeline_harmony&debuff.fae_exposure_damage.remains<1", "Prioritize Faeline Stomp if playing with Faeline Harmony" );
    // Spend excess energy
    def->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=!buff.serenity.up&buff.teachings_of_the_monastery.stack<3&combo_strike&chi.max-chi>=(2+buff.power_strikes.up)&(!talent.invoke_xuen_the_white_tiger&!talent.serenity|(!talent.skyreach|time>5|pet.xuen_the_white_tiger.active))", "TP if not overcapping Chi or TotM" );
    // Use Chi Burst to reset Faeline Stomp
    def->add_action( "chi_burst,if=talent.faeline_stomp&cooldown.faeline_stomp.remains&(chi.max-chi>=1&active_enemies=1|chi.max-chi>=2&active_enemies>=2)", "Use Chi Burst to reset Faeline Stomp" );

    // Use Cooldowns
    def->add_action( "call_action_list,name=cd_sef,if=!talent.serenity", "Cooldowns" );
    def->add_action( "call_action_list,name=cd_serenity,if=talent.serenity" );

    // Serenity / Default priority
    def->add_action( "call_action_list,name=serenity,if=buff.serenity.up", "Serenity / Default Priority" );
    def->add_action( "call_action_list,name=aoe,if=active_enemies>=3" );
    def->add_action( "call_action_list,name=st_cleave,if=active_enemies<3" );
    def->add_action( "call_action_list,name=fallthru" );

    // Storm, Earth and Fire Cooldowns
    cd_sef->add_action( "summon_white_tiger_statue,if=pet.xuen_the_white_tiger.active", "Storm, Earth and Fire Cooldowns" );
    cd_sef->add_action( "invoke_external_buff,name=power_infusion,if=pet.xuen_the_white_tiger.active" );
    cd_sef->add_action( "invoke_xuen_the_white_tiger,if=!variable.hold_xuen&talent.bonedust_brew&cooldown.bonedust_brew.remains<=5&(active_enemies<3&chi>=3|active_enemies>=3&chi>=2)|fight_remains<25" );
    cd_sef->add_action( "invoke_xuen_the_white_tiger,if=!variable.hold_xuen&!talent.bonedust_brew&(cooldown.rising_sun_kick.remains<2)&chi>=3" );
    cd_sef->add_action( "storm_earth_and_fire,if=talent.bonedust_brew&(fight_remains<30&cooldown.bonedust_brew.remains<4&chi>=4|buff.bonedust_brew.up|!spinning_crane_kick.max&active_enemies>=3&cooldown.bonedust_brew.remains<=2&chi>=2)&(pet.xuen_the_white_tiger.active|cooldown.invoke_xuen_the_white_tiger.remains>cooldown.storm_earth_and_fire.full_recharge_time)" );
    cd_sef->add_action( "bonedust_brew,if=(!buff.bonedust_brew.up&buff.storm_earth_and_fire.up&buff.storm_earth_and_fire.remains<11&spinning_crane_kick.max)|(!buff.bonedust_brew.up&fight_remains<30&fight_remains>10&spinning_crane_kick.max&chi>=4)|fight_remains<10" );
    cd_sef->add_action(
      "call_action_list,name=bdb_setup,if=!buff.bonedust_brew.up&talent.bonedust_brew&cooldown.bonedust_brew.remains<=2&(fight_remains>60&(cooldown.storm_earth_and_fire.charges>0|cooldown.storm_earth_and_fire.remains>10)&(pet.xuen_the_white_tiger.active|cooldown.invoke_xuen_the_white_tiger.remains>10|variable.hold_xuen)|((pet.xuen_the_white_tiger.active|cooldown.invoke_xuen_the_white_tiger.remains>13)&(cooldown.storm_earth_and_fire.charges>0|cooldown.storm_earth_and_fire.remains>13|buff.storm_earth_and_fire.up)))" );

    cd_sef->add_action( "storm_earth_and_fire,if=fight_remains<20|(cooldown.storm_earth_and_fire.charges=2&cooldown.invoke_xuen_the_white_tiger.remains>cooldown.storm_earth_and_fire.full_recharge_time)&cooldown.fists_of_fury.remains<=9&chi>=2&cooldown.whirling_dragon_punch.remains<=12" );

    if ( monk->sim->fight_style == FIGHT_STYLE_DUNGEON_ROUTE )
    {
      cd_sef->add_action( "touch_of_death,target_if=max:target.health,if=combo_strike&target.health<health" );
      cd_sef->add_action( "touch_of_death,cycle_targets=1,if=combo_strike&(target.time_to_die>60|debuff.bonedust_brew_debuff.up|fight_remains<10)" );
    }
    else
      cd_sef->add_action( "touch_of_death,cycle_targets=1,if=combo_strike" );


    for ( const auto& item : p->items )
    {
      if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
        cd_sef->add_action( "use_item,name=" + item.name_str + _WW_ON_USE( item ) );
    }


    if ( monk->talent.windwalker.invoke_xuen_the_white_tiger->ok() )
      cd_sef->add_action( "touch_of_karma,target_if=max:target.time_to_die,if=fight_remains>90|pet.xuen_the_white_tiger.active|variable.hold_xuen|fight_remains<16" );
    else
      cd_sef->add_action( "touch_of_karma,if=fight_remains>159|variable.hold_xuen" );

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

    // Serenity Cooldowns
    cd_serenity->add_action( "summon_white_tiger_statue,if=pet.xuen_the_white_tiger.active", "Serenity Cooldowns" );
    cd_serenity->add_action( "invoke_external_buff,name=power_infusion,if=pet.xuen_the_white_tiger.active" );
    cd_serenity->add_action( "invoke_xuen_the_white_tiger,if=!variable.hold_xuen&talent.bonedust_brew&cooldown.bonedust_brew.remains<=5|fight_remains<25" );
    cd_serenity->add_action( "invoke_xuen_the_white_tiger,if=!variable.hold_xuen&!talent.bonedust_brew&(cooldown.rising_sun_kick.remains<2)|fight_remains<25" );

    cd_serenity->add_action(
      "bonedust_brew,if=!buff.bonedust_brew.up&(cooldown.serenity.up|cooldown.serenity.remains>15|fight_remains<30&fight_remains>10)|fight_remains<10" );

    cd_serenity->add_action(
      "serenity,if=pet.xuen_the_white_tiger.active|cooldown.invoke_xuen_the_white_tiger.remains>10|!talent.invoke_xuen_the_white_tiger|fight_remains<15" );

    if ( monk->sim->fight_style == FIGHT_STYLE_DUNGEON_ROUTE )
    {
      cd_serenity->add_action( "touch_of_death,target_if=max:target.health,if=combo_strike&target.health<health" );
      cd_serenity->add_action( "touch_of_death,cycle_targets=1,if=combo_strike&(target.time_to_die>60|debuff.bonedust_brew_debuff.up|fight_remains<10)" );
    }
    else
      cd_serenity->add_action( "touch_of_death,cycle_targets=1,if=combo_strike" );

    cd_serenity->add_action( "touch_of_karma,if=fight_remains>90|fight_remains<10" );

    // Serenity Racials
    for ( const auto& racial_action : racial_actions )
    {
      if ( racial_action == "ancestral_call" )
        cd_serenity->add_action( racial_action + ",if=buff.serenity.up|fight_remains<20" );
      else if ( racial_action == "blood_fury" )
        cd_serenity->add_action( racial_action + ",if=buff.serenity.up|fight_remains<20" );
      else if ( racial_action == "fireblood" )
        cd_serenity->add_action( racial_action + ",if=buff.serenity.up|fight_remains<20" );
      else if ( racial_action == "berserking" )
        cd_serenity->add_action( racial_action + ",if=buff.serenity.up|fight_remains<20" );
      else if ( racial_action == "bag_of_tricks" )
        cd_serenity->add_action( racial_action + ",if=buff.serenity.up|fight_remains<20" );
      else if ( racial_action != "arcane_torrent" )
        cd_serenity->add_action( racial_action );
    }

    for ( const auto& item : p->items )
    {
      if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
        cd_serenity->add_action( "use_item,name=" + item.name_str + _WW_ON_USE( item ) );
    }

    // AoE priority
    aoe->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up&active_enemies>3", "AoE Priority (3+ Targets)" );
    aoe->add_action( "strike_of_the_windlord,if=talent.thunderfist&active_enemies>3" );
    aoe->add_action( "whirling_dragon_punch,if=active_enemies>8" );
    aoe->add_action( "spinning_crane_kick,if=buff.bonedust_brew.up&combo_strike&active_enemies>5&spinning_crane_kick.modifier>=3.2" );
    aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=3&talent.shadowboxing_treads" );
    aoe->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up" );
    aoe->add_action( "strike_of_the_windlord,if=talent.thunderfist" );
    aoe->add_action( "whirling_dragon_punch,if=active_enemies>5" );
    aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.up&(buff.teachings_of_the_monastery.stack=2|active_enemies<5)&talent.shadowboxing_treads" );
    aoe->add_action( "whirling_dragon_punch" );
    aoe->add_action( "spinning_crane_kick,if=buff.bonedust_brew.up&combo_strike" );
    aoe->add_action( "fists_of_fury,if=active_enemies>3" );
    aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=3" );
    aoe->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up&active_enemies>3" );
    aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.up&active_enemies>=5&talent.shadowboxing_treads" );
    aoe->add_action( "spinning_crane_kick,if=combo_strike&(active_enemies>=7|active_enemies=6&spinning_crane_kick.modifier>=2.7|active_enemies=5&spinning_crane_kick.modifier>=2.9)" );
    aoe->add_action( "strike_of_the_windlord" );
    aoe->add_action( "spinning_crane_kick,if=combo_strike&active_enemies>=5|active_enemies=4&spinning_crane_kick.modifier>=2.5|!talent.shadowboxing_treads" );
    aoe->add_action( "fists_of_fury" );
    aoe->add_action( "faeline_stomp,if=combo_strike");
    aoe->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike" );
    aoe->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    aoe->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains" );
    aoe->add_action( "whirling_dragon_punch" );

    // ST / Cleave
    st_cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=3&talent.shadowboxing_treads", "ST Priority (<3 Targets)" );
    st_cleave->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up" );
    st_cleave->add_action( "strike_of_the_windlord,if=talent.thunderfist" );
    st_cleave->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=active_enemies=1&buff.kicks_of_flowing_momentum.up|buff.pressure_point.up" );
    st_cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=2&talent.shadowboxing_treads" );
    st_cleave->add_action( "strike_of_the_windlord" );
    st_cleave->add_action( "fists_of_fury" );
    st_cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.up&(talent.shadowboxing_treads&active_enemies>1|cooldown.rising_sun_kick.remains>1)" );
    st_cleave->add_action( "whirling_dragon_punch,if=active_enemies=2" );
    st_cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=buff.teachings_of_the_monastery.stack=3" );
    st_cleave->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=(active_enemies=1|!talent.shadowboxing_treads)&cooldown.fists_of_fury.remains>4&talent.xuens_battlegear" );
    st_cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&active_enemies=2&cooldown.rising_sun_kick.remains&cooldown.fists_of_fury.remains" );
    st_cleave->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up&active_enemies=2" );
    st_cleave->add_action( "spinning_crane_kick,if=buff.bonedust_brew.up&combo_strike&(active_enemies>1|spinning_crane_kick.modifier>=2.7)" );
    st_cleave->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains" );
    st_cleave->add_action( "whirling_dragon_punch" );
    st_cleave->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up" );
    st_cleave->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike" );

    // Fallthru
    fallthru->add_action( "crackling_jade_lightning,if=buff.the_emperors_capacitor.stack>19&energy.time_to_max>execute_time-1&cooldown.rising_sun_kick.remains>execute_time|buff.the_emperors_capacitor.stack>14&(cooldown.serenity.remains<5&talent.serenity|fight_remains<5)", "Fallthru" );
    fallthru->add_action( "faeline_stomp,if=combo_strike" );
    fallthru->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&chi.max-chi>=(2+buff.power_strikes.up)" );
    fallthru->add_action( "expel_harm,if=chi.max-chi>=1&active_enemies>2" );
    fallthru->add_action( "chi_burst,if=chi.max-chi>=1&active_enemies=1&raid_event.adds.in>20|chi.max-chi>=2&active_enemies>=2" );
    fallthru->add_action( "chi_wave" );
    fallthru->add_action( "expel_harm,if=chi.max-chi>=1" );
    fallthru->add_action( "spinning_crane_kick,if=combo_strike&buff.chi_energy.stack>30-5*active_enemies&buff.storm_earth_and_fire.down&(cooldown.rising_sun_kick.remains>2&cooldown.fists_of_fury.remains>2|cooldown.rising_sun_kick.remains<3&cooldown.fists_of_fury.remains>3&chi>3|cooldown.rising_sun_kick.remains>3&cooldown.fists_of_fury.remains<3&chi>4|chi.max-chi<=1&energy.time_to_max<2)|buff.chi_energy.stack>10&fight_remains<7" );
    fallthru->add_action( "arcane_torrent,if=chi.max-chi>=1" );
    fallthru->add_action( "flying_serpent_kick,interrupt=1" );
    fallthru->add_action( "tiger_palm" );

    // Serenity Priority
    serenity->add_action( "strike_of_the_windlord,if=active_enemies<3", "Serenity Priority" );
    serenity->add_action( "fists_of_fury,if=buff.serenity.remains<1" );
    serenity->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&buff.teachings_of_the_monastery.stack=3&buff.teachings_of_the_monastery.remains<1" );
    serenity->add_action( "fists_of_fury_cancel" );
    serenity->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&active_enemies=3&buff.teachings_of_the_monastery.stack=2" );
    serenity->add_action( "spinning_crane_kick,if=combo_strike&(active_enemies>3|active_enemies>2&spinning_crane_kick.modifier>=2.3)" );
    serenity->add_action( "strike_of_the_windlord,if=active_enemies>=3" );
    serenity->add_action( "spinning_crane_kick,if=combo_strike&active_enemies>1" );
    serenity->add_action( "whirling_dragon_punch,if=active_enemies>1" );
    serenity->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=active_enemies>=3&cooldown.fists_of_fury.remains&talent.shadowboxing_treads" );
    serenity->add_action( "rushing_jade_wind,if=!buff.rushing_jade_wind.up&active_enemies>=3" );
    serenity->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains" );
    serenity->add_action( "spinning_crane_kick,if=combo_strike&buff.dance_of_chiji.up" );
    serenity->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike" );
    serenity->add_action( "whirling_dragon_punch" );
    serenity->add_action( "tiger_palm,target_if=min:debuff.mark_of_the_crane.remains,if=talent.teachings_of_the_monastery&buff.teachings_of_the_monastery.stack<3" );


    // Opener
    opener->add_action( p, "Expel Harm", "if=talent.chi_burst.enabled&chi.max-chi>=3", "Opener" );
    opener->add_action( p, "Tiger Palm",
      "target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&chi.max-chi>=(2+buff.power_strikes.up)" );
    opener->add_action( "chi_wave,if=chi.max-chi=2" );
    opener->add_action( p, "Expel Harm" );
    opener->add_action( p, "Tiger Palm",
      "target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=chi.max-chi>=(2+buff.power_strikes.up)" );

    // Bonedust Brew Setup
    bdb_setup->add_action( "bonedust_brew,if=spinning_crane_kick.max&chi>=4", "Bonedust Brew Setup" );
    bdb_setup->add_action( "blackout_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&!talent.whirling_dragon_punch" );
    bdb_setup->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&chi>=5" );
    bdb_setup->add_action( p, "Tiger Palm",
      "target_if=min:debuff.mark_of_the_crane.remains+(debuff.skyreach_exhaustion.up*20),if=combo_strike&chi.max-chi>=2" );
    bdb_setup->add_action( "rising_sun_kick,target_if=min:debuff.mark_of_the_crane.remains,if=combo_strike&active_enemies>=2" );
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
