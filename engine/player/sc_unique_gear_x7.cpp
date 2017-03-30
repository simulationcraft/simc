// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

using namespace unique_gear;

/**
 * Forward declarations so we can reorganize the file a bit more sanely.
 */

namespace consumable
{
  void potion_of_the_old_war( special_effect_t& );
  void potion_of_deadly_grace( special_effect_t& );
  void hearty_feast( special_effect_t& );
  void lavish_suramar_feast( special_effect_t& );
  void pepper_breath( special_effect_t& );
  void lemon_herb_filet( special_effect_t& );
}

namespace enchants
{
  void mark_of_the_hidden_satyr( special_effect_t& );
  void mark_of_the_distant_army( special_effect_t& );
  void mark_of_the_ancient_priestess( special_effect_t& ); // NYI
}

namespace item
{
  // 7.0 Dungeon
  void chaos_talisman( special_effect_t& );
  void corrupted_starlight( special_effect_t& );
  void elementium_bomb_squirrel( special_effect_t& );
  void faulty_countermeasures( special_effect_t& );
  void figurehead_of_the_naglfar( special_effect_t& );
  void giant_ornamental_pearl( special_effect_t& );
  void horn_of_valor( special_effect_t& );
  void impact_tremor( special_effect_t& );
  void memento_of_angerboda( special_effect_t& );
  void moonlit_prism( special_effect_t& );
  void obelisk_of_the_void( special_effect_t& );
  void portable_manacracker( special_effect_t& );
  void shivermaws_jawbone( special_effect_t& );
  void spiked_counterweight( special_effect_t& );
  void stabilized_energy_pendant( special_effect_t& );
  void stormsinger_fulmination_charge( special_effect_t& );
  void terrorbound_nexus( special_effect_t& );
  void tiny_oozeling_in_a_jar( special_effect_t& );
  void tirathons_betrayal( special_effect_t& );
  void windscar_whetstone( special_effect_t& );
  void jeweled_signet_of_melandrus( special_effect_t& );
  void caged_horror( special_effect_t& );
  void tempered_egg_of_serpentrix( special_effect_t& );
  void gnawed_thumb_ring( special_effect_t& );
  void naraxas_spiked_tongue( special_effect_t& );
  void choker_of_barbed_reins( special_effect_t& );

  // 7.1 Dungeon
  void arans_relaxing_ruby( special_effect_t& );
  void ring_of_collapsing_futures( special_effect_t& );
  void mrrgrias_favor( special_effect_t& );
  void toe_knees_promise( special_effect_t& );
  void deteriorated_construct_core( special_effect_t& );
  void eye_of_command( special_effect_t& );
  void bloodstained_hankerchief( special_effect_t& );
  void majordomos_dinner_bell( special_effect_t& );

  // 7.0 Misc
  void darkmoon_deck( special_effect_t& );
  void infernal_alchemist_stone( special_effect_t& ); // WIP
  void sixfeather_fan( special_effect_t& );
  void eyasus_mulligan( special_effect_t& );
  void marfisis_giant_censer( special_effect_t& );
  void devilsaurs_bite( special_effect_t& );
  void leyspark(special_effect_t& );

  // 7.0 Raid
  void bloodthirsty_instinct( special_effect_t& );
  void natures_call( special_effect_t& );
  void ravaged_seed_pod( special_effect_t& );
  void spontaneous_appendages( special_effect_t& );
  void twisting_wind( special_effect_t& );
  void unstable_horrorslime( special_effect_t& );
  void wriggling_sinew( special_effect_t& );
  void bough_of_corruption( special_effect_t& );
  void ursocs_rending_paw( special_effect_t& );

  // 7.1.5 Raid
  void draught_of_souls( special_effect_t&        );
  void convergence_of_fates( special_effect_t&    );
  void entwined_elemental_foci( special_effect_t& );
  void fury_of_the_burning_sky( special_effect_t& );
  void icon_of_rot( special_effect_t&             );
  void star_gate( special_effect_t&               );
  void erratic_metronome( special_effect_t&       );
  void whispers_in_the_dark( special_effect_t&    );
  void nightblooming_frond( special_effect_t&     );
  void might_of_krosus( special_effect_t&         );

  // 7.2.0 Dungeon
  void dreadstone_of_endless_shadows( special_effect_t& );

  // Adding this here to check it off the list.
  // The sim builds it automatically.
  void pharameres_forbidden_grimoire( special_effect_t& );

  // Legendary
  void aggramars_stride( special_effect_t& );
  void kiljadens_burning_wish( special_effect_t& );
  void norgannons_foresight( special_effect_t& );




  /* NYI ================================================================
  Nighthold ---------------------------------

  Everything

  Healer trinkets / other rubbish -----------

  cocoon_of_enforced_solitude
  goblet_of_nightmarish_ichor
  grotesque_statuette
  horn_of_cenarius
  phantasmal_echo
  vial_of_nightmare_fog
  heightened_senses
  */
}

namespace util
{
// Return the Karazhan Chest empowerment multiplier (as 1.0 + multiplier) for trinkets, or 1.0 if
// the chest is not found on the actor.
double composite_karazhan_empower_multiplier( const player_t* player )
{
  auto it = range::find_if( player -> items, []( const item_t& item ) {
    return item.name_str ==  "robes_of_the_ancient_chronicle" ||
           item.name_str ==  "harness_of_smoldering_betrayal" ||
           item.name_str ==  "hauberk_of_warped_intuition"    ||
           item.name_str ==  "chestplate_of_impenetrable_darkness";
  } );

  if ( it != player -> items.end() )
  {
    return 1.0 + player -> find_spell( 231626 ) -> effectN( 1 ).percent();
  }

  return 1.0;
}
}

namespace set_bonus
{
  // 7.0 Dungeon
  void march_of_the_legion( special_effect_t& ); 
  void journey_through_time( special_effect_t& ); // NYI

  // Generic passive stat aura adder for set bonuses
  void passive_stat_aura( special_effect_t& );
  // Simple callback creator for set bonuses
  void simple_callback( special_effect_t& );
}

// TODO: Ratings
void set_bonus::passive_stat_aura( special_effect_t& effect )
{
  const spell_data_t* spell = effect.player -> find_spell( effect.spell_id );
  stat_e stat = STAT_NONE;
  // Sanity check for stat-giving aura, either stats or aura type 465 ("bonus armor")
  if ( spell -> effectN( 1 ).subtype() != A_MOD_STAT || spell -> effectN( 1 ).subtype() == A_465 )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  if ( spell -> effectN( 1 ).subtype() == A_MOD_STAT )
  {
    if ( spell -> effectN( 1 ).misc_value1() >= 0 )
    {
      stat = static_cast< stat_e >( spell -> effectN( 1 ).misc_value1() + 1 );
    }
    else if ( spell -> effectN( 1 ).misc_value1() == -1 )
    {
      stat = STAT_ALL;
    }
  }
  else
  {
    stat = STAT_BONUS_ARMOR;
  }

  double amount = util::round( spell -> effectN( 1 ).average( effect.player, std::min( MAX_LEVEL, effect.player -> level() ) ) );

  effect.player -> passive.add_stat( stat, amount );
}

void set_bonus::simple_callback( special_effect_t& effect )
{ new dbc_proc_callback_t( effect.player, effect ); }

// Mark of the Distant Army =================================================

struct mark_of_the_distant_army_t : public proc_spell_t
{
  mark_of_the_distant_army_t( player_t* p ) :
    proc_spell_t( "mark_of_the_distant_army",
      p, p -> find_spell( 191380 ), nullptr )
  {
    // Hardcoded somewhere in the bowels of the server
    attack_power_mod.tick = ( 2.5 / 3.0 );
    spell_power_mod.tick = ( 2.0 / 3.0 );
  }

  double amount_delta_modifier( const action_state_t* ) const override
  { return 0.15; }

  double attack_tick_power_coefficient( const action_state_t* s ) const override
  {
    auto total_ap = attack_power_mod.tick * s -> composite_attack_power();
    auto total_sp = spell_power_mod.tick * s -> composite_spell_power();

    if ( total_ap <= total_sp )
    {
      return 0;
    }

    return proc_spell_t::attack_tick_power_coefficient( s );
  }

  double spell_tick_power_coefficient( const action_state_t* s ) const override
  {
    auto total_ap = attack_power_mod.tick * s -> composite_attack_power();
    auto total_sp = spell_power_mod.tick * s -> composite_spell_power();

    if ( total_sp < total_ap )
      return 0;

    return proc_spell_t::spell_tick_power_coefficient( s );
  }

  // Hack to force defender to mitigate the damage with armor.
  void assess_damage( dmg_e, action_state_t* s ) override
  { proc_spell_t::assess_damage( DMG_DIRECT, s ); }
};

void enchants::mark_of_the_distant_army( special_effect_t& effect )
{
  effect.execute_action = effect.player -> find_action( "mark_of_the_distant_army" );

  if ( ! effect.execute_action )
  {
    effect.execute_action = effect.player -> create_proc_action( "mark_of_the_distant_army", effect );
  }

  if ( ! effect.execute_action )
  {
    action_t* a = new mark_of_the_distant_army_t( effect.player );

    effect.execute_action = a;
  }

  effect.proc_flags_ = PF_ALL_DAMAGE | PF_PERIODIC; // DBC says procs off heals. Let's not.

  new dbc_proc_callback_t( effect.item, effect );
}

// Mark of the Hidden Satyr =================================================

void enchants::mark_of_the_hidden_satyr( special_effect_t& effect )
{
  // Uses max of apcoeff*ap, spcoeff*sp to power the damage
  struct mark_of_the_hidden_satyr_t : public proc_spell_t
  {
    mark_of_the_hidden_satyr_t( const special_effect_t& e ) :
      proc_spell_t( "mark_of_the_hidden_satyr", e.player,
        e.player -> find_spell( 191259 ), nullptr )
    {
      // Hardcoded somewhere in the bowels of the server
      attack_power_mod.direct = 2.5;
      spell_power_mod.direct = 2.0;
    }

    double amount_delta_modifier( const action_state_t* ) const override
    { return 0.15; }

    double attack_direct_power_coefficient( const action_state_t* s ) const override
    {
      auto total_ap = attack_power_mod.direct * s -> composite_attack_power();
      auto total_sp = spell_power_mod.direct * s -> composite_spell_power();

      if ( total_ap <= total_sp )
      {
        return 0;
      }

      return proc_spell_t::attack_direct_power_coefficient( s );
    }

    double spell_direct_power_coefficient( const action_state_t* s ) const override
    {
      auto total_ap = attack_power_mod.direct * s -> composite_attack_power();
      auto total_sp = spell_power_mod.direct * s -> composite_spell_power();

      if ( total_sp < total_ap )
        return 0;

      return proc_spell_t::spell_direct_power_coefficient( s );
    }
  };

  effect.execute_action = effect.player -> find_action( "mark_of_the_hidden_satyr" );

  if ( ! effect.execute_action )
  {
    effect.execute_action = effect.player -> create_proc_action( "mark_of_the_hidden_satyr", effect );
  }

  if ( ! effect.execute_action )
  {
    action_t* a = new mark_of_the_hidden_satyr_t( effect );
    effect.execute_action = a;
  }

  effect.proc_flags_ = PF_ALL_DAMAGE | PF_PERIODIC; // DBC says procs off heals. Let's not.

  new dbc_proc_callback_t( effect.item, effect );
}
// Aran's Relaxing Ruby ============================================================

struct flame_wreath_t : public spell_t
{
  flame_wreath_t( const special_effect_t& effect ) :
    spell_t( "flame_wreath", effect.player, effect.player -> find_spell( 230257 ) )
  {
    background = may_crit = true;
    callbacks = false;
    item = effect.item;
    school = SCHOOL_FIRE;
    base_dd_min = base_dd_max = effect.driver() -> effectN( 1 ).average( effect.item );

    for ( const auto& item : effect.player -> items )
    {
      if ( item.name_str ==  "robes_of_the_ancient_chronicle" ||
           item.name_str ==  "harness_of_smoldering_betrayal" ||
           item.name_str ==  "hauberk_of_warped_intuition"    ||
           item.name_str ==  "chestplate_of_impenetrable_darkness" )
        //FIXME: Don't hardcode the 30% damage bonus
        base_dd_min = base_dd_max = effect.driver() -> effectN( 1 ).average( effect.item ) * 1.3;
    }
    aoe = -1;
  }

};

void item::arans_relaxing_ruby( special_effect_t& effect )
{
  action_t* action = effect.player -> find_action( "flame_wreath" ) ;
  if ( ! action )
  {
    action = effect.player -> create_proc_action( "flame_wreath", effect );
  }

  if ( ! action )
  {
    action = new flame_wreath_t( effect );
  }



  effect.execute_action = action;
  effect.proc_flags2_ = PF2_ALL_HIT;

  new dbc_proc_callback_t( effect.player, effect );
}

void item::ring_of_collapsing_futures( special_effect_t& effect )
{
  struct collapse_t: public proc_spell_t {
    collapse_t( const special_effect_t& effect ):
      proc_spell_t( "collapse", effect.player,
                    effect.player -> find_spell( effect.spell_id ),
                    effect.item )
    {
      base_dd_min = base_dd_max = effect.player ->find_spell( effect.spell_id ) -> effectN( 1 ).base_value(); // Does not scale with ilevel, apparently.
    }
  };

  struct apply_debuff_t: public action_t
  {
    special_effect_t effect_;
    cooldown_t* base_cd;
    action_t* collapse;
    apply_debuff_t( special_effect_t& effect ):
      action_t( ACTION_OTHER, "apply_temptation", effect.player, spell_data_t::nil() ),
      effect_( effect ), base_cd( effect.player -> get_cooldown( effect.cooldown_name() ) ),
      collapse( nullptr )
    {
      background = quiet = true;
      callbacks = false;
      collapse = effect.player -> find_action( "collapse" );
      cooldown = base_cd;
      if ( !collapse ) {
        collapse = effect.player -> create_proc_action( "collapse", effect );
      }

      if ( !collapse ) {
        collapse = new collapse_t( effect );
      }
    }

    // Suppress assertion
    result_e calculate_result( action_state_t* ) const override
    { return RESULT_NONE; }

    void execute() override
    {
      action_t::execute();

      collapse -> target = target;
      collapse -> schedule_execute();
      if ( rng().roll( effect_.custom_buff -> stack_value() ) )
      {
        base_cd -> adjust( timespan_t::from_minutes( 5 ) ); // Ouch.
      }

      effect_.custom_buff -> trigger( 1 );
    }
  };

  effect.custom_buff = effect.player -> buffs.temptation;
  effect.execute_action = new apply_debuff_t( effect );
  effect.buff_disabled = true; // Buff application is handled inside apply_debuff_t, and this will prevent use_item
                               // from putting the item on cooldown but not using the action.
}

// Giant Ornamental Pearl ===================================================

struct gaseous_bubble_t : public absorb_buff_t
{
  action_t* explosion;

  gaseous_bubble_t( special_effect_t& effect, action_t* a ) :
    absorb_buff_t( absorb_buff_creator_t( effect.player, "gaseous_bubble", effect.driver(), effect.item ) ),
    explosion( a )
  {
    // Set correct damage amount for explosion.
    a -> base_dd_min = effect.driver() -> effectN( 2 ).min( effect.item );
    a -> base_dd_max = effect.driver() -> effectN( 2 ).max( effect.item );
  }

  void expire_override( int stacks, timespan_t remaining ) override
  {
    absorb_buff_t::expire_override( stacks, remaining );

    explosion -> schedule_execute();
  }
};

void item::giant_ornamental_pearl( special_effect_t& effect )
{
  effect.trigger_spell_id = 214972;

  effect.custom_buff = new gaseous_bubble_t( effect, effect.create_action() );

  // Reset trigger_spell_id so it does not create an execute action.
  effect.trigger_spell_id = 0;
}

// Gnawed Thumb Ring =======================================================

void item::gnawed_thumb_ring( special_effect_t& effect )
{
  effect.custom_buff = buff_creator_t( effect.player, "taste_of_mana", effect.player -> find_spell( 228461 ), effect.item )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .default_value( effect.player -> find_spell( 228461 ) -> effectN( 1 ).percent() );

  effect.player -> buffs.taste_of_mana = effect.custom_buff;
}

// Naraxa's Spiked Tongue ==================================================

void item::naraxas_spiked_tongue( special_effect_t& effect )
{
  struct rancid_maw_t: public proc_spell_t
  {
    rancid_maw_t( const special_effect_t& e ):
      proc_spell_t( "rancid_maw", e.player, e.player -> find_spell( 215405 ), e.item )
    {
    }

    double action_multiplier() const override
    {
      double am = proc_spell_t::action_multiplier();

      double distance = player -> get_player_distance( *target );
      am *= ( std::min( distance, 20.0 ) / 20.0 ); // Does less damage the closer player is to target.
      return am;
    }
  };

  effect.execute_action = effect.player -> find_action( "rancid_maw" );

  if ( !effect.execute_action )
  {
    effect.execute_action = effect.player -> create_proc_action( "rancid_maw", effect );
  }

  if ( !effect.execute_action )
  {
    action_t* a = new rancid_maw_t( effect );
    effect.execute_action = a;
  }

  new dbc_proc_callback_t( effect.item, effect );
}

// Choker of Barbed Reins ==================================================

void item::choker_of_barbed_reins( special_effect_t& effect )
{
  struct choker_of_barbed_reins_t: public proc_attack_t
  {
    choker_of_barbed_reins_t( const special_effect_t& e ):
      proc_attack_t( "barbed_rebuke", e.player, e.player -> find_spell( 234108 ), e.item )
    {
      may_block = true;
    }
    double target_armor( player_t* ) const override
    { return 0.0; }
  };

  effect.execute_action = effect.player -> find_action( "barbed_rebuke" );

  if ( !effect.execute_action )
  {
    effect.execute_action = effect.player -> create_proc_action( "barbed_rebuke", effect );
  }

  if ( !effect.execute_action )
  {
    action_t* a = new choker_of_barbed_reins_t( effect );
    effect.execute_action = a;
  }

  new dbc_proc_callback_t( effect.item, effect );
}

// Deteriorated Construct Core ==============================================
struct volatile_energy_t : public spell_t
{
  volatile_energy_t( const special_effect_t& effect ) :
    spell_t( "volatile_energy", effect.player, effect.player -> find_spell( 230241 ) )
  {
    background = may_crit = true;
    callbacks = false;
    item = effect.item;
    base_dd_min = base_dd_max = effect.driver() -> effectN( 1 ).average( effect.item ) *
                                util::composite_karazhan_empower_multiplier( effect.player );
    aoe = -1;
  }
};

void item::deteriorated_construct_core( special_effect_t& effect )
{
  action_t* action = effect.player -> find_action( "volatile_energy" );
  if ( ! action )
  {
    action = effect.player -> create_proc_action( "volatile_energy", effect );
  }

  if ( ! action )
  {
    action = new volatile_energy_t( effect );
  }

  effect.execute_action = action;
  effect.proc_flags2_= PF2_ALL_HIT;

  new dbc_proc_callback_t( effect.player, effect );
}

// Eye of Command ===========================================================

struct eye_of_command_cb_t : public dbc_proc_callback_t
{
  player_t* current_target;
  buff_t*   buff;

  eye_of_command_cb_t( const special_effect_t& effect, buff_t* b ) :
    dbc_proc_callback_t( effect.item, effect ),
    current_target( nullptr ), buff( b )
  { }

  void execute( action_t* /* a */, action_state_t* state ) override
  {
    if ( current_target != state -> target )
    {
      if ( current_target != nullptr && listener -> sim -> debug )
      {
        listener -> sim -> out_debug.printf( "%s eye_of_command target reset, old=%s new=%s",
          listener -> name(), current_target -> name(), state -> target -> name() );
      }
      buff -> expire();
      current_target = state -> target;
    }

    buff -> trigger();
  }
};

// TODO: Autoattacks don't really change targets currently in simc, so this code is for future
// reference.
// TODO: For PTR purposes is CR_MULTIPLIER_TRINKET correct, or should it be CR_MULTIPLIER_ARMOR?
void item::eye_of_command( special_effect_t& effect )
{
  auto amount = effect.trigger() -> effectN( 1 ).average( effect.item ) *
                effect.player -> dbc.combat_rating_multiplier( effect.item -> item_level(), CR_MULTIPLIER_TRINKET ) *
                util::composite_karazhan_empower_multiplier( effect.player );

  stat_buff_t* b = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "legions_gaze" ) );
  if ( ! b )
  {
    b = stat_buff_creator_t( effect.player, "legions_gaze", effect.trigger(), effect.item )
        .add_stat( STAT_CRIT_RATING, amount );
  }

  effect.custom_buff = b;

  new eye_of_command_cb_t( effect, b );
}

// Bloodstained Hankerchief =================================================

// Note, custom implementations are going to have to apply the empower multiplier independent of
// this function.

struct cruel_garrote_t: public spell_t
{
  cruel_garrote_t( const special_effect_t& effect ):
    spell_t( "cruel_garrote", effect.player, effect.driver() )
  {
    background = hasted_ticks = tick_may_crit = may_crit = true;
    base_td *= util::composite_karazhan_empower_multiplier( effect.player );
  }
};

void item::bloodstained_hankerchief( special_effect_t& effect )
{
  action_t* a = new cruel_garrote_t( effect );

  effect.execute_action = a;
}
// Erratic Metronome ========================================================
void item::erratic_metronome( special_effect_t& effect )
{
  struct erratic_metronome_cb_t : public dbc_proc_callback_t
  {
    erratic_metronome_cb_t ( const special_effect_t& effect, const double amount ) :
      dbc_proc_callback_t( effect.item, effect )
    {
      // TODO: FIX HARDCODED VALUES
      proc_buff = stat_buff_creator_t( effect.player, "accelerando", effect.player -> find_spell( 225719 ), effect.item )
        .cd( timespan_t::zero() )
        .add_stat( STAT_HASTE_RATING, amount )
        .max_stack( 5 )
        .refresh_behavior( BUFF_REFRESH_DISABLED )
        .duration( timespan_t::from_seconds( 12.0 ) );
    }

    void execute(action_t*, action_state_t*) override
    {
      int stack = proc_buff -> check();

      if (stack == 0)
      {
        proc_buff -> trigger();
      }
      else
      {
        proc_buff -> bump( 1 );
      }
    }
  };
  // the amount is stored in another spell
  double amount = item_database::apply_combat_rating_multiplier(
                                 *effect.item, effect.player -> find_spell( 225719 ) ->
                                 effectN( 1 ).average( effect.item ) );
  new erratic_metronome_cb_t( effect, amount );

}

// Icon of Rot ==============================================================

struct carrion_swarm_t : public spell_t
{
  carrion_swarm_t( const special_effect_t& effect ) :
    spell_t( "carrion_swarm", effect.player, effect.driver() -> effectN( 1 ).trigger() )
  {
    background = true;
    hasted_ticks = may_miss = may_dodge = may_parry = may_block = may_crit = false;
    callbacks = false;
    base_td = effect.driver() -> effectN( 1 ).average( effect.item );
  }
};

struct icon_of_rot_driver_t : public dbc_proc_callback_t
{
  carrion_swarm_t* carrion_swarm;
  icon_of_rot_driver_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.player, effect ),
    carrion_swarm( new carrion_swarm_t( effect ) )
  { }

  void initialize() override
  {
    dbc_proc_callback_t::initialize();

    action_t* damage_spell = listener -> find_action( "carrion_swarm" );

    if( ! damage_spell )
    {
      damage_spell = listener -> create_proc_action( "carrion_swarm", effect );
    }

    if ( ! damage_spell )
    {
      damage_spell = new carrion_swarm_t( effect );
    }
  }

  void execute( action_t*  /*a*/ , action_state_t* trigger_state ) override
  {
    actor_target_data_t* td = listener -> get_target_data( trigger_state -> target );
    assert( td );

    auto& tl = listener -> sim -> target_non_sleeping_list;

    for ( size_t i = 0, targets = tl.size(); i < targets; i++ )
    {
      carrion_swarm -> target = tl[ i ];
      carrion_swarm -> execute();
    }
  }
};


void item::icon_of_rot( special_effect_t& effect )
{
  effect.proc_flags_ = effect.driver() -> proc_flags();
  effect.proc_flags2_ = PF2_ALL_HIT;

  new icon_of_rot_driver_t( effect );
}
// Impact Tremor ============================================================

void item::impact_tremor( special_effect_t& effect )
{
  // Use automagic action creation
  effect.trigger_spell_id = effect.trigger() -> effectN( 1 ).trigger() -> id();
  action_t* stampede = effect.create_action();
  effect.trigger_spell_id = 0;

  effect.custom_buff = buff_creator_t( effect.player, "devilsaurs_stampede", effect.driver() -> effectN( 1 ).trigger(), effect.item )
    .tick_zero( true )
    .tick_callback( [ stampede ]( buff_t*, int, const timespan_t& ) {
      stampede -> schedule_execute();
    } );

  // Disable automatic creation of a trigger spell.
  effect.action_disabled = true;

  new dbc_proc_callback_t( effect.item, effect );
}


// Fury of the Burning Sky ==================================================
struct solar_collapse_impact_t : public spell_t
{
  solar_collapse_impact_t( const special_effect_t& effect ) :
    spell_t( "solar_collapse_damage", effect.player, effect.player -> find_spell( 229737 ) )
  {
    background = may_crit = true;
    callbacks = false;
    aoe = -1;
    // FIXME: This value is returning a slightly lower damage amount vs. in game for the base ilvl trinket. (147k vs. 145k).
    // Maybe hotfix things will fix.
    base_dd_min = base_dd_max = data().effectN( 2 ).average( effect.item );
  }
};
struct fury_of_the_burning_sky_driver_t : public dbc_proc_callback_t
{
  cooldown_t* icd;
  fury_of_the_burning_sky_driver_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.player, effect )
  {
    icd = effect.player -> get_cooldown( "solar_collapse_icd" );
    icd -> duration = timespan_t::from_seconds( 3.0 );
  }

  void initialize() override
  {
    dbc_proc_callback_t::initialize();

    action_t* damage_spell = listener -> find_action( "solar_collapse_damage" );

    if( ! damage_spell )
    {
      damage_spell = listener -> create_proc_action( "solar_collapse_damage", effect );
    }

    if ( ! damage_spell )
    {
      damage_spell = new solar_collapse_impact_t( effect );
    }
  }

  void execute( action_t*  /*a*/ , action_state_t* trigger_state ) override
  {
    actor_target_data_t* td = listener -> get_target_data( trigger_state -> target );
    assert( td );

    // FIXME: This might be in the wrong place - the ICD is on the RPPM trigger event, this
    // might only be preventing debuff application, not the actual RPPM being rolled/firing.
    if ( icd -> up() )
    {
      td -> debuff.solar_collapse -> trigger();
    }
    icd -> start();
  }
};

struct solar_collapse_t : public buff_t
{
  action_t* damage_event;

  solar_collapse_t( const actor_pair_t& p, const special_effect_t& source_effect ) :
    buff_t( buff_creator_t( p, "solar_collapse", source_effect.driver() -> effectN( 1 ).trigger(), source_effect.item )
                                  .duration( timespan_t::from_seconds( 3.0 ) ) ),
                                  damage_event( source -> find_action( "solar_collapse_damage" ) )
  { }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
    damage_event -> target = player;
    damage_event -> execute();
  }
};
struct fury_of_the_burning_sun_constructor_t : public item_targetdata_initializer_t
{
  fury_of_the_burning_sun_constructor_t( unsigned iid, const std::vector< slot_e >& s ) :
    item_targetdata_initializer_t( iid, s )
  { }

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td -> source );
    if( effect == 0 )
    {
      td -> debuff.solar_collapse = buff_creator_t( *td, "solar_collapse" );
    }
    else
    {
      assert( ! td -> debuff.solar_collapse );

      td -> debuff.solar_collapse = new solar_collapse_t( *td, *effect );
      td -> debuff.solar_collapse -> reset();
    }
  }
};

void item::fury_of_the_burning_sky( special_effect_t& effect )
{
  effect.proc_flags_ = effect.driver() -> proc_flags();
  effect.proc_flags2_ = PF2_ALL_HIT;

  new fury_of_the_burning_sky_driver_t( effect );
}

// Mrrgria's Favor ==========================================================

struct thunder_ritual_impact_t : public spell_t
{
  //TODO: Are these multipliers multiplicative with one another or should they be added together then applied?
  // Right now we assume they are independant multipliers.

  // Note: Mrrgria's Favor and Toe Knee's Promise interact to buff eachother by 30% when the other is present.
  // However, their cooldowns are not equal, so we cannot simply treat this as a 30% permanant modifier.
  // Instead, if we are modeling having the trinkets paired, we give mrrgria's favor an ICD event that does
  // not effect ready() state, but instead controls the application of the 30% multiplier.

  bool pair_multiplied;
  double chest_multiplier;
  bool pair_buffed;
  cooldown_t* pair_icd;

  thunder_ritual_impact_t( const special_effect_t& effect ) :
    spell_t( "thunder_ritual_damage", effect.player, effect.driver() -> effectN( 1 ).trigger() ),
    pair_multiplied( false ),
    chest_multiplier( util::composite_karazhan_empower_multiplier( effect.player ) ),
    pair_buffed( false )
  {
    background = may_crit = true;
    callbacks = false;
    pair_icd = effect.player -> get_cooldown( "paired_trinket_icd" );
    pair_icd -> duration = timespan_t::from_seconds( 60.0 );
    if ( player -> karazhan_trinkets_paired )
    {

      pair_multiplied = true;
    }
    base_dd_min = base_dd_max = data().effectN( 1 ).average( effect.item ) * chest_multiplier;
  }

  virtual void execute() override
  {
    // Reset pair checking
    pair_buffed = false;

    if( pair_multiplied == true && pair_icd -> up() )
    {
      pair_buffed = true;
      pair_icd -> start();
    }
    spell_t::execute();
  }

  double action_multiplier() const override
  {
    double am = spell_t::action_multiplier();
    if ( pair_buffed )
    {
      am *= 1.3;
    }
    return am;
  }
};

struct thunder_ritual_t : public buff_t
{
  action_t* damage_event;

  thunder_ritual_t( const actor_pair_t& p, const special_effect_t& source_effect ) :
    buff_t( buff_creator_t( p, "thunder_ritual", source_effect.driver() -> effectN( 1 ).trigger(), source_effect.item )
                                  .duration( timespan_t::from_seconds( 3.0 ) ) ),
                                  damage_event( source -> find_action( "thunder_ritual_damage" ) )
  {}

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
    damage_event -> target = player;
    damage_event -> execute();
  }
};

struct thunder_ritual_driver_t : public proc_spell_t
{
  thunder_ritual_driver_t( special_effect_t& effect ) :
    proc_spell_t( "thunder_ritual_driver", effect.player, effect.driver() -> effectN( 1 ).trigger(), effect.item )
  {
    harmful = false;
    base_dd_min = base_dd_max = 0;
  }

  void impact( action_state_t* s ) override
  {
    proc_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      player -> get_target_data( s -> target ) -> debuff.thunder_ritual -> trigger();
    }
  }
};
struct mrrgrias_favor_constructor_t : public item_targetdata_initializer_t
{
  mrrgrias_favor_constructor_t( unsigned iid, const std::vector< slot_e >& s ) :
    item_targetdata_initializer_t( iid, s )
  { }

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td -> source );
    if( effect == 0 )
    {
      td -> debuff.thunder_ritual = buff_creator_t( *td, "thunder_ritual" );
    }
    else
    {
      assert( ! td -> debuff.thunder_ritual );

      td -> debuff.thunder_ritual = new thunder_ritual_t( *td, *effect );
      td -> debuff.thunder_ritual -> reset();
    }
  }
};

void item::mrrgrias_favor( special_effect_t& effect )
{
  effect.execute_action = new thunder_ritual_driver_t( effect );
  effect.execute_action -> add_child( new thunder_ritual_impact_t( effect ) );
}


// Toe Knee's Promise ======================================================

struct flame_gale_pulse_t : spell_t
{
  //TODO: Are these multipliers multiplicative with one another or should they be added together then applied?
  // Right now we assume they are independant multipliers.
  double chest_multiplier;
  double paired_multiplier;
  flame_gale_pulse_t( special_effect_t& effect ) :
    spell_t( "flame_gale_pulse", effect.player, effect.player -> find_spell( 230213 ) ),
    chest_multiplier( util::composite_karazhan_empower_multiplier( effect.player ) ),
    paired_multiplier( 1.0 )
  {
    background = true;
    callbacks = false;
    school = SCHOOL_FIRE;
    aoe = -1;
    if ( player -> karazhan_trinkets_paired )
      paired_multiplier += 0.3;
    base_dd_min = base_dd_max = data().effectN( 2 ).average( effect.item ) * paired_multiplier * chest_multiplier;
  }
};
struct flame_gale_driver_t : spell_t
{
  flame_gale_pulse_t* flame_pulse;
  flame_gale_driver_t( special_effect_t& effect ) :
    spell_t( "flame_gale_driver", effect.player, effect.player -> find_spell( 230213 ) ),
    flame_pulse( new flame_gale_pulse_t( effect ) )
  {
  background = true;
  }

  virtual void impact( action_state_t* s )
  {
    spell_t::impact( s );
      make_event<ground_aoe_event_t>( *sim, player, ground_aoe_params_t()
        .pulse_time( timespan_t::from_seconds( 1.0 ) )
        .target( execute_state -> target )
        .duration( timespan_t::from_seconds( 8.0 ) )
        .hasted( ground_aoe_params_t::ATTACK_HASTE )
        .action( flame_pulse ) );
  }
};
void item::toe_knees_promise( special_effect_t& effect )
{
  effect.execute_action = new flame_gale_driver_t( effect );
}

// Majordomo's Dinner Bell ==================================================

struct majordomos_dinner_bell_t : proc_spell_t
{
  std::vector<stat_buff_t*> buffs;

  majordomos_dinner_bell_t(const special_effect_t& effect) :
    proc_spell_t("dinner_bell", effect.player, effect.player -> find_spell(230101), effect.item)
  {
    // Spell used for tooltips, game uses buffs 230102-230105 based on a script but not all are in DBC spell data
    const spell_data_t* buff_spell = effect.player->find_spell(230102);
    const double buff_amount = item_database::apply_combat_rating_multiplier(*effect.item,
      buff_spell->effectN(1).average(effect.item)) * util::composite_karazhan_empower_multiplier(effect.player);

    buffs =
    {
      stat_buff_creator_t(effect.player, "quite_satisfied_crit", buff_spell, effect.item)
        .add_stat(STAT_CRIT_RATING, buff_amount),
      stat_buff_creator_t(effect.player, "quite_satisfied_haste", buff_spell, effect.item)
        .add_stat(STAT_HASTE_RATING, buff_amount),
      stat_buff_creator_t(effect.player, "quite_satisfied_mastery", buff_spell, effect.item)
        .add_stat(STAT_MASTERY_RATING, buff_amount),
      stat_buff_creator_t(effect.player, "quite_satisfied_vers", buff_spell, effect.item)
        .add_stat(STAT_VERSATILITY_RATING, buff_amount),
    };
  }

  void execute() override
  {
    // The way this works, despite the tooltip, is that the buff matches your current food buff
    // If you don't have a food buff, it is random
    if(player->consumables.food)
    {
      const stat_buff_t* food_buff = dynamic_cast<stat_buff_t*>(player->consumables.food);
      if (food_buff && food_buff->stats.size() > 0)
      {
        const stat_e food_stat = food_buff->stats.front().stat;
        const auto it = range::find_if(buffs, [food_stat](const stat_buff_t* buff) {
          if (buff->stats.size() > 0)
            return buff->stats.front().stat == food_stat;
          else
            return false;
        });

        if (it != buffs.end())
        {
          (*it)->trigger();
          return;
        }
      }
    }

    // We didn't find a matching food buff, so pick randomly
    const int selected_buff = (int)(player->sim->rng().real() * buffs.size());
    buffs[selected_buff]->trigger();
  }
};

void item::majordomos_dinner_bell(special_effect_t& effect)
{
  effect.execute_action = new majordomos_dinner_bell_t( effect );
}


// Memento of Angerboda =====================================================

struct memento_callback_t : public dbc_proc_callback_t
{
  std::vector<buff_t*> buffs;

  memento_callback_t( const special_effect_t& effect, std::vector<buff_t*> b ) :
    dbc_proc_callback_t( effect.item, effect ), buffs( b )
  {}

  void execute( action_t* /* a */, action_state_t* /* call_data */ ) override
  {
    // Memento prefers to proc inactive buffs over active ones.
    // Make a vector with only the inactive buffs.
    std::vector<buff_t*> inactive_buffs;

    for ( unsigned i = 0; i < buffs.size(); i++ )
    {
      if ( ! buffs[ i ] -> check() )
      {
        inactive_buffs.push_back( buffs[ i ] );
      }
    }

    // If the vector is empty, we can roll any of the buffs.
    if ( inactive_buffs.empty() )
    {
      inactive_buffs = buffs;
    }

    // Roll it!
    int roll = ( int ) ( listener -> sim -> rng().real() * inactive_buffs.size() );
    inactive_buffs[ roll ] -> trigger();
  }
};

void item::memento_of_angerboda( special_effect_t& effect )
{
  double rating_amount = item_database::apply_combat_rating_multiplier( *effect.item,
      effect.driver() -> effectN( 2 ).average( effect.item ) );

  std::vector<buff_t*> buffs;

  buffs.push_back( stat_buff_creator_t( effect.player, "howl_of_ingvar", effect.player -> find_spell( 214802 ) )
      .activated( false )
      .add_stat( STAT_CRIT_RATING, rating_amount ) );
  buffs.push_back( stat_buff_creator_t( effect.player, "wail_of_svala", effect.player -> find_spell( 214803 ) )
     .activated( false )
      .add_stat( STAT_HASTE_RATING, rating_amount ) );
  buffs.push_back( stat_buff_creator_t( effect.player, "dirge_of_angerboda", effect.player -> find_spell( 214807 ) )
      .activated( false )
      .add_stat( STAT_MASTERY_RATING, rating_amount ) );

  new memento_callback_t( effect, buffs );
}

// Obelisk of the Void ======================================================

void item::obelisk_of_the_void( special_effect_t& effect )
{
  effect.custom_buff = stat_buff_creator_t( effect.player, "collapsing_shadow", effect.player -> find_spell( 215476 ), effect.item )
    .add_stat( effect.player -> convert_hybrid_stat( STAT_AGI_INT ), effect.driver() -> effectN( 2 ).average( effect.item ) );
}

// Shivermaws Jawbone =======================================================

struct ice_bomb_t : public proc_spell_t
{
  buff_t* buff;

  ice_bomb_t( special_effect_t& effect ) :
    proc_spell_t( "ice_bomb", effect.player, effect.driver(), effect.item )
  {
    buff = stat_buff_creator_t( effect.player, "frigid_armor", effect.player -> find_spell( 214589 ), effect.item );
  }

  void execute() override
  {
    proc_spell_t::execute();

    buff -> trigger( num_targets_hit );
  }
};

void item::shivermaws_jawbone( special_effect_t& effect )
{
  effect.execute_action = new ice_bomb_t( effect );
}

// Spiked Counterweight =====================================================

struct haymaker_damage_t : public proc_spell_t
{
  haymaker_damage_t( const special_effect_t& effect ) :
    proc_spell_t( "brutal_haymaker_vulnerability", effect.player, effect.driver() -> effectN( 2 ).trigger(), effect.item )
  {
    may_crit = may_miss = false;
    dot_duration = timespan_t::zero();
  }

  void init() override
  {
    proc_spell_t::init();

    snapshot_flags = update_flags = 0;
  }
};

struct haymaker_driver_t : public dbc_proc_callback_t
{
  struct haymaker_event_t;

  buff_t* debuff;
  const special_effect_t& effect;
  double multiplier;
  haymaker_event_t* accumulator;
  haymaker_damage_t* action;

  struct haymaker_event_t : public event_t
  {
    haymaker_damage_t* action;
    haymaker_driver_t* callback;
    buff_t* debuff;
    double damage;

    haymaker_event_t( haymaker_driver_t* cb, haymaker_damage_t* a, buff_t* d ) :
      event_t( *a -> player, a -> data().effectN( 1 ).period() ), action( a ), callback( cb ), debuff( d ), damage( 0 )
    {
    }

    const char* name() const override
    { return "brutal_haymaker_damage"; }

    void execute() override
    {
      if ( damage > 0 )
      {
        action -> target = debuff -> player;
        damage = std::min( damage, debuff -> current_value );
        action -> base_dd_min = action -> base_dd_max = damage;
        action -> schedule_execute();
        // 2016-10-11 - Damage increases from target multiplier debuffs do not count towards the damage cap.
        damage /= action -> player -> composite_player_target_multiplier( action -> target, SCHOOL_PHYSICAL );
        debuff -> current_value -= damage;
      }

      if ( debuff -> current_value > 0 && debuff -> check() )
        callback -> accumulator = make_event<haymaker_event_t>( *action -> player -> sim, callback, action, debuff );
      else
      {
        callback -> accumulator = nullptr;
        debuff -> expire();
      }
    }
  };

  haymaker_driver_t( const special_effect_t& e, double m ) :
    dbc_proc_callback_t( e.player, e ), debuff(nullptr), effect( e ), multiplier( m ), accumulator(nullptr),
    action( debug_cast<haymaker_damage_t*>( e.player -> find_action( "brutal_haymaker_vulnerability" ) ) )
  {}

  void activate() override
  {
    dbc_proc_callback_t::activate();

    accumulator = make_event<haymaker_event_t>( *effect.player -> sim, this, action, debuff );
  }

  void execute( action_t* /* a */, action_state_t* trigger_state ) override
  {
    if ( trigger_state -> result_amount <= 0 )
      return;

    actor_target_data_t* td = effect.player -> get_target_data( trigger_state -> target );

    if ( td && td -> debuff.brutal_haymaker -> check() )
      accumulator -> damage += trigger_state -> result_amount * multiplier;
  }
};

struct spiked_counterweight_constructor_t : public item_targetdata_initializer_t
{
  spiked_counterweight_constructor_t( unsigned iid, const std::vector< slot_e >& s ) :
    item_targetdata_initializer_t( iid, s )
  {}

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td -> source );
    if ( effect == 0 )
    {
      td -> debuff.brutal_haymaker = buff_creator_t( *td, "brutal_haymaker" );
    }
    else
    {
      assert( ! td -> debuff.brutal_haymaker );

      // Vulnerability: Deal x% of the damage dealt to the debuffed target, up to the limit.

      // Create effect for the callback.
      special_effect_t* effect2 = new special_effect_t( effect -> item );
      effect2 -> source = SPECIAL_EFFECT_SOURCE_ITEM;
      effect2 -> name_str = "brutal_haymaker_accumulator";
      effect2 -> proc_chance_ = 1.0;
      effect2 -> proc_flags_ = PF_ALL_DAMAGE | PF_PERIODIC;
      effect2 -> proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
      effect -> player -> special_effects.push_back( effect2 );

      // Create callback. We'll enable this callback whenever the debuff is active.
      haymaker_driver_t* callback =
        new haymaker_driver_t( *effect2, effect -> trigger() -> effectN( 2 ).percent() );
      callback -> initialize();
      callback -> deactivate();

      // Create debuff with stack callback.
      td -> debuff.brutal_haymaker = buff_creator_t( *td, "brutal_haymaker", effect -> trigger() )
        .stack_change_callback( [ callback ]( buff_t*, int old, int new_ )
        {
          if ( old == 0 ) {
            assert( ! callback -> active );
            callback -> activate();
          } else if ( new_ == 0 )
            callback -> deactivate();
        } )
        .default_value( effect -> driver() -> effectN( 1 ).trigger() -> effectN( 3 ).average( effect -> item ) );
      td -> debuff.brutal_haymaker -> reset();

      // Set pointer to debuff so the callback can do its thing.
      callback -> debuff = td -> debuff.brutal_haymaker;
    }
  }
};

struct brutal_haymaker_initial_t : public proc_spell_t
{
  brutal_haymaker_initial_t( special_effect_t& effect ) :
    proc_spell_t( "brutal_haymaker", effect.player, effect.driver() -> effectN( 1 ).trigger(), effect.item )
  {}

  void impact( action_state_t* s ) override
  {
    proc_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      player -> get_target_data( s -> target ) -> debuff.brutal_haymaker -> trigger();
    }
  }
};

void item::spiked_counterweight( special_effect_t& effect )
{
  effect.execute_action = new brutal_haymaker_initial_t( effect );
  effect.execute_action -> add_child( new haymaker_damage_t( effect ) );

  new dbc_proc_callback_t( effect.item, effect );
}

// Star Gate ================================================================

struct nether_meteor_t : public spell_t
{
  nether_meteor_t( const special_effect_t& effect ) :
    spell_t( "nether_meteor", effect.player, effect.driver() )
  {
    background = may_crit = true;
    callbacks = false;
    school = SCHOOL_SPELLFIRE;
    base_dd_min = base_dd_max = effect.player -> find_spell( 225764 ) -> effectN( 1 ).average( effect.item );
    aoe = -1;
  }

   timespan_t travel_time() const override
  {
    // Hardcode this to override the 1y/s velocity in the spelldata and keep
    // impact timings consistent with in game results.
    return timespan_t::from_seconds( 1.0 );
  }
};

void item::star_gate( special_effect_t& effect )
{

  action_t* action = effect.player -> find_action( "nether_meteor" ) ;
  if ( ! action )
  {
    action = effect.player -> create_proc_action( "nether_meteor", effect );
  }

  if ( ! action )
  {
    action = new nether_meteor_t( effect );
  }

  effect.execute_action = action;
  effect.proc_flags2_ = PF2_ALL_HIT;

  new dbc_proc_callback_t( effect.player, effect );

}

// Windscar Whetstone =======================================================

void item::windscar_whetstone( special_effect_t& effect )
{
  action_t* maelstrom = effect.create_action();
  maelstrom -> cooldown -> duration = timespan_t::zero(); // damage spell has erroneous cooldown

  effect.custom_buff = buff_creator_t( effect.player, "slicing_maelstrom", effect.driver(), effect.item )
    .tick_zero( true )
    .tick_callback( [ maelstrom ]( buff_t*, int, const timespan_t& ) {
      maelstrom -> schedule_execute();
    } );

  // Disable automatic creation of a trigger spell.
  effect.trigger_spell_id = 1;
}

// Whispers in the Dark =====================================================

void item::whispers_in_the_dark( special_effect_t& effect )
{
  auto good_buff_data = effect.player -> find_spell( 225774 );
  auto good_amount = good_buff_data -> effectN( 1 ).average( effect.item ) / 100.0;

  auto bad_buff_data = effect.player -> find_spell( 225776 );
  auto bad_amount = bad_buff_data -> effectN( 1 ).average( effect.item ) / 100.0;

  haste_buff_t* bad_buff = haste_buff_creator_t( effect.player, "devils_due", bad_buff_data, effect.item )
    .add_invalidate( CACHE_SPELL_SPEED )
    .default_value( bad_amount );

  haste_buff_t* good_buff = haste_buff_creator_t( effect.player, "nefarious_pact", good_buff_data, effect.item )
    .add_invalidate( CACHE_SPELL_SPEED )
    .default_value( good_amount )
    .stack_change_callback( [ bad_buff ]( buff_t*, int old_, int ) {
      if ( old_ == 1 )
      {
        bad_buff -> trigger();
      }
    } );

  effect.custom_buff = good_buff;
  effect.player -> buffs.nefarious_pact = good_buff;
  effect.player -> buffs.devils_due = bad_buff;

  new dbc_proc_callback_t( effect.item, effect );
}

// Nightblooming Frond ======================================================

void item::nightblooming_frond( special_effect_t& effect )
{
  struct recursive_strikes_attack_t : public proc_attack_t
  {
    buff_t* recursive_strikes_buff;

    recursive_strikes_attack_t( const special_effect_t& e ) :
      proc_attack_t( "recursive_strikes", e.player, e.trigger(), e.item ),
      recursive_strikes_buff( buff_t::find( e.player, "recursive_strikes" ) )
    { }

    double action_multiplier() const override
    {
      double m = proc_attack_t::action_multiplier();

      m *= recursive_strikes_buff -> stack();

      return m;
    }
  };

  struct recursive_strikes_dmg_cb_t : public dbc_proc_callback_t
  {
    recursive_strikes_dmg_cb_t( const special_effect_t* effect ) :
      dbc_proc_callback_t( effect->item, *effect )
    { }

    // Have to override default dbc_proc_callback_t behavior here, as it would expire the buff upon
    // reaching max stack.
    void execute( action_t*, action_state_t* state ) override
    {
      proc_buff -> trigger();
      if ( proc_buff -> check() > 1 )
      {
        proc_action -> target = target( state );
        proc_action -> execute();
      }
    }
  };

  struct recursive_strikes_driver_t : public dbc_proc_callback_t
  {
    recursive_strikes_driver_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.item, effect )
    { }

    // Since the basic refresh behavior of Recursive Strikes is to never refresh the duration,
    // temporarily make the driver change the refresh behavior to "refresh to full duration", so if
    // you get a lucky driver proc while recursive strikes is up, you get +1 stack, and a full 15
    // seconds of additional buff time.
    void execute( action_t*, action_state_t* ) override
    {
      proc_buff -> refresh_behavior = BUFF_REFRESH_DURATION;
      proc_buff -> trigger();
      proc_buff -> refresh_behavior = BUFF_REFRESH_DISABLED;
    }
  };

  special_effect_t* effect2 = new special_effect_t( effect.item );
  effect2 -> name_str = "recursive_strikes_intensity";
  effect2 -> spell_id = effect.trigger() -> id();
  effect2 -> item = effect.item;
  effect.player -> special_effects.push_back( effect2 );

  // The auto-attack callback is instantiated here but initialized below, because the secondary
  // special effect struct needs to have the buff and action pointers initialized to their
  // respective objects, before the initialization is run.
  auto cb = new recursive_strikes_dmg_cb_t( effect2 );

  buff_t* b = buff_t::find( effect.player, "recursive_strikes" );
  if ( b == nullptr )
  {
    b = buff_creator_t( effect.player, "recursive_strikes", effect.trigger() )
      .refresh_behavior( BUFF_REFRESH_DISABLED ) // Don't refresh duration when the buff gains stacks
      .stack_change_callback( [ cb ]( buff_t*, int old_, int new_ ) {
        if ( old_ == 0 ) // Buff goes up the first time
        {
          cb -> activate();
        }
        else if ( new_ == 0 ) // Buff expires
        {
          cb -> deactivate();
        }
      } );
  }

  action_t* a = effect.player -> find_action( "recursive_strikes" );
  if ( a == nullptr )
  {
    a = effect.player -> create_proc_action( "recursive_strikes", *effect2 );
  }

  if ( a == nullptr )
  {
    a = new recursive_strikes_attack_t( *effect2 );
  }

  effect.custom_buff = b;
  effect2 -> custom_buff = b;
  effect2 -> execute_action = a;

  // Manually initialize and deactivate the auto-attack proc callback that generates damage and
  // procs more stacks on the Recursive Strikes buff.
  cb -> initialize();
  cb -> deactivate();

  // Base Driver, responsible for triggering the Recursive Strikes buff. Recursive Strikes buff in
  // turn will activate the secondary proc callback that generates stacks and triggers damage.
  new recursive_strikes_driver_t( effect );
}

// Might of Krosus ==========================================================

void item::might_of_krosus( special_effect_t& effect )
{
  struct colossal_slam_t : public proc_attack_t
  {
    const special_effect_t& effect;
    cooldown_t* use_cooldown;

    colossal_slam_t( const special_effect_t& effect_ ) :
      proc_attack_t( "colossal_slam", effect_.player, effect_.trigger(), effect_.item ),
      effect( effect_ ),
      use_cooldown( effect_.player -> get_cooldown( effect_.cooldown_name() ) )
    {
      split_aoe_damage = true;
    }

    double composite_crit_chance() const override
    {
      return 1.0; // Always crits
    }

    void execute() override
    {
      proc_attack_t::execute();

      if ( as<int>( execute_state -> n_targets ) < effect.driver() -> effectN( 2 ).base_value() )
      {
        use_cooldown -> adjust( -timespan_t::from_seconds( effect.driver() -> effectN( 3 ).base_value() ) );
      }
    }
  };

  action_t* a = effect.player -> find_action( "colossal_slam" );
  if ( a == nullptr )
  {
    a = effect.player -> create_proc_action( "colossal_slam", effect );
  }

  if ( a == nullptr )
  {
    a = new colossal_slam_t( effect );
  }

  effect.execute_action = a;
}

void item::pharameres_forbidden_grimoire( special_effect_t& effect )
{
  struct orb_of_destruction_impact_t : public spell_t
  {
    orb_of_destruction_impact_t( const special_effect_t& effect ) :
      spell_t( "orb_of_destruction_impact", effect.player, effect.driver() ->effectN( 1 ).trigger() )
    {
      background = may_crit = true;
      aoe = -1;
      base_dd_min = base_dd_max = data().effectN( 1 ).average( effect.item );
      if ( effect.player -> type == ( WARRIOR || ROGUE || PALADIN || DEMON_HUNTER || DEATH_KNIGHT || MONK ) ||
           effect.player -> specialization() == ( HUNTER_SURVIVAL || SHAMAN_ENHANCEMENT || DRUID_FERAL || DRUID_GUARDIAN ) ) // Melee users always deal half damage even if they run 20 yards out to use this trinket. 
        base_multiplier = 0.5;
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double am = spell_t::composite_target_multiplier( t );
      //Formula for the damage reduction due to distance from the original target seems to fit
      // damage_reduction = 1 - ( distance / 30 ) ^ 2
      // Data used to find this approximation:
      /* Distance, Damage Done
         0.0,      506675
         7.32e-5,  506669
         2.3087,   503726
         3.2249,   500923
         5.8694,   487644
         6.3789,   483542
         7.7104,   470934
         8.2765,   467706
         10.2108,  448322
         11.6349,  430007
         12.1807,  423206
         13.2231,  408371
         16.2364,  358321
         18.071,   323028
         19.9642,  283293
         20.0721,  0
      */
      am *= 1.0 - std::pow( std::min( target -> get_player_distance( *t ), 20.0 ) / 30.0, 2 );
      return am;
    }
  };

  struct orb_of_destruction_t : public spell_t
  {
    orb_of_destruction_impact_t* impact;
    orb_of_destruction_t( const special_effect_t& effect ) :
      spell_t( "orb_of_destruction", effect.player, effect.driver() ),
      impact( nullptr )
    {
      callbacks = false;
      background = true;
      impact = new orb_of_destruction_impact_t( effect );
      add_child( impact );
    }

    bool ready() override
    {
      if ( player -> get_player_distance( *target ) < 14 )
      {
        return false;
      }
      return spell_t::ready();
    }

    void execute() override
    {
      impact -> target = target;
      impact -> execute();
    }
  };

  action_t* a = effect.player -> find_action( "orb_of_destruction" );
  if ( a == nullptr )
  {
    a = effect.player -> create_proc_action( "orb_of_destruction", effect );
  }

  if ( a == nullptr )
  {
    a = new orb_of_destruction_t( effect );
  }

  effect.execute_action = a;
}

// Dreadstone of Endless Shadows ============================================

struct dreadstone_proc_cb_t : public dbc_proc_callback_t
{
  const std::vector<stat_buff_t*> buffs;

  dreadstone_proc_cb_t( const special_effect_t& effect, const std::vector<stat_buff_t*>& buffs_ ) :
    dbc_proc_callback_t( effect.item, effect ), buffs( buffs_ )
  { }

  void execute( action_t* /* a */, action_state_t* /* state */ ) override
  {
    size_t buff_index = static_cast<size_t>( rng().range( 0, buffs.size() ) );
    buffs[ buff_index ] -> trigger();
  }
};

void item::dreadstone_of_endless_shadows( special_effect_t& effect )
{
  auto p = effect.player;

  auto amount = item_database::apply_combat_rating_multiplier( *effect.item,
      effect.driver() -> effectN( 2 ).average( effect.item ) );

  // Buffs
  auto crit = debug_cast<stat_buff_t*>( buff_t::find( p, "sign_of_the_hippo" ) );
  if ( crit == nullptr )
  {
    crit = stat_buff_creator_t( p, "sign_of_the_hippo", p -> find_spell( 225749 ), effect.item )
      .add_stat( STAT_CRIT_RATING, amount );
  }

  auto mastery = debug_cast<stat_buff_t*>( buff_t::find( p, "sign_of_the_hare" ) );
  if ( mastery == nullptr )
  {
    mastery = stat_buff_creator_t( p, "sign_of_the_hare", p -> find_spell( 225752 ), effect.item )
      .add_stat( STAT_MASTERY_RATING, amount );
  }

  auto haste = debug_cast<stat_buff_t*>( buff_t::find( p, "sign_of_the_dragon" ) );
  if ( haste == nullptr )
  {
    haste = stat_buff_creator_t( p, "sign_of_the_dragon", p -> find_spell( 225753 ), effect.item )
      .add_stat( STAT_HASTE_RATING, amount );
  }

  new dreadstone_proc_cb_t( effect, { crit, mastery, haste } );
}

// Tirathon's Betrayal ======================================================

struct darkstrikes_absorb_t : public absorb_t
{
  darkstrikes_absorb_t( const special_effect_t& effect ) :
    absorb_t( "darkstrikes_absorb", effect.player, effect.player -> find_spell( 215659 ) )
  {
    background = true;
    callbacks = false;
    item = effect.item;
    target = effect.player;

    // Set correct absorb amount.
    parse_effect_data( data().effectN( 2 ) );
  }
};

struct darkstrikes_t : public proc_spell_t
{
  action_t* absorb;

  darkstrikes_t( const special_effect_t& effect ) :
    proc_spell_t( "darkstrikes", effect.player, effect.player -> find_spell( 215659 ), effect.item )
  {
    // Set correct damage amount (can accidentally pick up absorb amount instead).
    parse_effect_data( data().effectN( 1 ) );
  }

  void init() override
  {
    proc_spell_t::init();

    absorb = player -> find_action( "darkstrikes_absorb" );
  }

  void execute() override
  {
    proc_spell_t::execute();

    absorb -> schedule_execute();
  }
};

struct darkstrikes_driver_t : public dbc_proc_callback_t
{
  action_t* damage;

  darkstrikes_driver_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.player, effect ),
    damage( effect.player -> find_action( "darkstrikes" ) )
  { }

  void execute( action_t* /* a */, action_state_t* trigger_state ) override
  {
    damage -> target = trigger_state -> target;
    damage -> schedule_execute();
  }
};

void item::tirathons_betrayal( special_effect_t& effect )
{
  action_t* darkstrikes = effect.player -> find_action( "darkstrikes" );
  if ( ! darkstrikes )
  {
    darkstrikes = effect.player -> create_proc_action( "darkstrikes", effect );
  }

  if ( ! darkstrikes )
  {
    darkstrikes = new darkstrikes_t( effect );
  }

  action_t* absorb = effect.player -> find_action( "darkstrikes_absorb" );
  if ( ! absorb )
  {
    absorb = effect.player -> create_proc_action( "darkstrikes_absorb", effect );
  }

  if ( ! absorb )
  {
    absorb = new darkstrikes_absorb_t( effect );
  }

  // Special effect to drive the AOE damage callback
  special_effect_t* dmg_effect = new special_effect_t( effect.item );
  dmg_effect -> source = SPECIAL_EFFECT_SOURCE_ITEM;
  dmg_effect -> name_str = "darkstrikes_driver";
  dmg_effect -> spell_id = effect.driver() -> id();
  dmg_effect -> cooldown_ = timespan_t::zero();
  effect.player -> special_effects.push_back( dmg_effect );

  // And create, initialized and deactivate the callback
  dbc_proc_callback_t* callback = new darkstrikes_driver_t( *dmg_effect );
  callback -> initialize();
  callback -> deactivate();

  effect.custom_buff = buff_creator_t( effect.player, "darkstrikes", effect.driver(), effect.item )
    .chance( 1 ) // overrride RPPM
    .activated( false )
    .cd( timespan_t::zero() )
    .stack_change_callback( [ callback ]( buff_t*, int old, int new_ )
    {
      if ( old == 0 ) {
        assert( ! callback -> active );
        callback -> activate();
      } else if ( new_ == 0 )
        callback -> deactivate();
    } );
}

// Bough of Corruption ===============================================================

// Damage event for the poisoned dreams impact, comes from the Posioned Dreams debuff being
// impacted by a spell.
struct poisoned_dreams_impact_t : public spell_t
{
  // Poisoned Dreams has some ICD that prevents the damage event from
  // being triggered multiple times in quick succession.

  cooldown_t* icd;
  double stack_multiplier;
  poisoned_dreams_impact_t( const special_effect_t& effect ) :
    spell_t( "poisoned_dreams_damage", effect.player, effect.player -> find_spell( 222705 ) ),
    stack_multiplier( 1.0 )
  {
    background = may_crit = true;
    callbacks = false;
    base_dd_min = base_dd_max = data().effectN( 1 ).average( effect.item );
    cooldown -> duration = timespan_t::zero();
    item = effect.item;
    icd = effect.player -> get_cooldown( "poisoned_dreams_icd" );
    icd -> duration = timespan_t::from_seconds( 1.0 );
  }
  virtual bool ready() override
  {
    if ( icd -> down() )
    {
      return false;
    }

    return spell_t::ready();
  }

  virtual void execute() override
  {    // TODO: Verify exact ICD from in game data. Currently based off spelldata ICD on the damage event.
       // Also fix hardcoding
    if ( icd -> up() )
    {
     spell_t::execute();
     icd -> start();
    }

  }
};
// Callback driver that executes the Poisoned Dreams_impact on the enemy when the Poisoned Dreams
// debuff is up.
struct poisoned_dreams_damage_driver_t : public dbc_proc_callback_t
{
  action_t* damage;
  player_t* target;

  poisoned_dreams_damage_driver_t( const special_effect_t& effect, action_t* d, player_t* t ) :
    dbc_proc_callback_t( effect.player, effect ), damage( d ), target( t )
  {

  }

  void trigger( action_t* a, void* call_data ) override
  {
    const action_state_t* s = static_cast<const action_state_t*>( call_data );
    if ( s -> target != target )
    {
      return;
    }

    dbc_proc_callback_t::trigger( a, call_data );
  }

  void execute( action_t* /* a */ , action_state_t* trigger_state ) override
  {
    actor_target_data_t* td = listener -> get_target_data( trigger_state -> target );
    damage -> base_multiplier = 1.0; // Reset base multiplier before each trigger so we scale linearly with #stacks, not exponentially.
    damage -> target = trigger_state -> target;
    damage -> base_multiplier *= td -> debuff.poisoned_dreams -> current_stack;
    damage -> execute();
  }

};
// Poisoned Dreams debuff to control poisoned_dreams_driver_t
struct poisoned_dreams_t : public buff_t
{
  poisoned_dreams_damage_driver_t* driver_cb;
  action_t* damage_spell;
  special_effect_t* effect;

  poisoned_dreams_t( const actor_pair_t& p, const special_effect_t& source_effect ) :
    buff_t( buff_creator_t( p, "poisoned_dreams", source_effect.driver() -> effectN( 1 ).trigger(), source_effect.item )
                              .activated( false )
                              .period( timespan_t::from_seconds( 2.0 ) ) ),
                              damage_spell( p.source -> find_action( "poisoned_dreams_damage" ) )
  {
    effect = new special_effect_t( p.source );
    effect -> name_str = "poisoned_dreams_damage_driver";
    effect -> proc_chance_ = 1.0;
    effect -> proc_flags_ = PF_SPELL | PF_AOE_SPELL | PF_PERIODIC;
    effect -> proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
    p.source -> special_effects.push_back( effect );

    //TODO: Fix hardcoded ICD on the debuff proc application from spelldata.
    cooldown -> duration = timespan_t::from_seconds( 20.0 );
    driver_cb = new poisoned_dreams_damage_driver_t( *effect, damage_spell, p.target );
    driver_cb -> initialize();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    driver_cb -> deactivate();
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    driver_cb -> activate();
  }

  void reset() override
  {
    buff_t::reset();

    driver_cb -> deactivate();
  }
};

// Prophecy of Fear base driver, handles the proccing ( triggering ) of Mark of Doom on targets
struct bough_of_corruption_driver_t : public dbc_proc_callback_t
{
  bough_of_corruption_driver_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.player, effect )
  {
  }

  void initialize() override
  {
    dbc_proc_callback_t::initialize();

    action_t* damage_spell = listener -> find_action( "poisoned_dreams_damage" );

    if( ! damage_spell )
    {
      damage_spell = listener -> create_proc_action( "poisoned_dreams_damage", effect );
    }

    if ( ! damage_spell )
    {
      damage_spell = new poisoned_dreams_impact_t( effect );
    }
  }

  void execute( action_t*  /*a*/ , action_state_t* trigger_state ) override
  {
    actor_target_data_t* td = listener -> get_target_data( trigger_state -> target );
    assert( td );
    td -> debuff.poisoned_dreams -> trigger();
  }
};

struct bough_of_corruption_constructor_t : public item_targetdata_initializer_t
{
  bough_of_corruption_constructor_t( unsigned iid, const std::vector< slot_e >& s ) :
    item_targetdata_initializer_t( iid, s )
  { }

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td -> source );
    if( effect == 0 )
    {
      td -> debuff.poisoned_dreams = buff_creator_t( *td, "poisoned_dreams" );
    }
    else
    {
      assert( ! td -> debuff.poisoned_dreams );

      td -> debuff.poisoned_dreams = new poisoned_dreams_t( *td, *effect );
      td -> debuff.poisoned_dreams -> reset();
    }
  }

};
void item::bough_of_corruption( special_effect_t& effect )
{
  effect.proc_flags_ = effect.driver() -> proc_flags()  | PF_AOE_SPELL;
  effect.proc_flags2_ = PF2_ALL_HIT;

  new bough_of_corruption_driver_t( effect );
}

void item::ursocs_rending_paw( special_effect_t& effect )
{
  struct rending_flesh_t: public residual_action::residual_periodic_action_t < attack_t > {
    double crit_chance_of_last_impact;
    player_t* player_;
    rending_flesh_t( const special_effect_t& effect ):
      residual_action::residual_periodic_action_t<attack_t>( "rend_flesh", effect.player, effect.player -> find_spell( effect.trigger_spell_id ) ),
      crit_chance_of_last_impact( 0 ), player_( effect.player )
    {
      base_td = base_dd_max = base_dd_min = 0;
      attack_power_mod.tick = 0;
      tick_may_crit = true;
    }

    double calculate_tick_amount( action_state_t* state, double dmg_multiplier ) const override
    {
      double amount = 0.0;

      if ( dot_t* d = find_dot( state->target ) )
      {
        residual_action::residual_periodic_state_t* dot_state = debug_cast<residual_action::residual_periodic_state_t*>( d->state );
        amount += dot_state->tick_amount;
      }

      state->result_raw = amount;

      state->result = RESULT_HIT; // Reset result to hit, as it has already been rolled inside tick().
      if ( rng().roll( composite_crit_chance() ) )
        state->result = RESULT_CRIT;

      if ( state->result == RESULT_CRIT )
        amount *= 1.0 + total_crit_bonus( state );

      amount *= dmg_multiplier;

      state->result_total = amount;

      return amount;
    }

    double composite_crit_chance() const override
    {
      return crit_chance_of_last_impact;
    }

    void impact( action_state_t* s ) override
    {
      residual_periodic_action_t::impact( s );
      crit_chance_of_last_impact = player_ -> composite_melee_crit_chance(); // Uses crit chance of the last autoattack to proc it.
    }
  };

  struct rending_flesh_execute_t: spell_t {
    rending_flesh_t* rend;
    rending_flesh_execute_t( const special_effect_t& effect ):
      spell_t( "rending_flesh_trigger", effect.player, effect.player -> find_spell( effect.trigger_spell_id ) ),
      rend( nullptr )
    {
      rend = new rending_flesh_t( effect );
      background = true;
      base_dd_min = base_dd_max = data().effectN( 1 ).average( effect.item ) * 4; //We're tossing the entire dot into the ignite.
      dot_duration = timespan_t::zero();
    }

    void impact( action_state_t* s ) override
    {
      residual_action::trigger( rend, s -> target, s -> result_amount );
    }
  };

  effect.proc_flags2_ = PF2_CRIT;
  effect.trigger_spell_id = 221770;

  auto rend = new rending_flesh_execute_t( effect );
  effect.execute_action = rend;

  new dbc_proc_callback_t( effect.item, effect );
}

// Draught of Souls =========================================================

void item::draught_of_souls( special_effect_t& effect )
{
  struct felcrazed_rage_t : public proc_spell_t
  {
    felcrazed_rage_t( const special_effect_t& effect ) :
      proc_spell_t( "felcrazed_rage", effect.player, effect.trigger(), effect.item )
    {
      aoe = 0; // This does not actually AOE
      dual = true;
      base_multiplier *= 1.0 + effect.player -> find_specialization_spell( "Unholy Death Knight" ) -> effectN( 4 ).percent();
    }
  };

  struct draught_of_souls_driver_t : public proc_spell_t
  {
    const special_effect_t& effect;
    action_t* damage;

    draught_of_souls_driver_t( const special_effect_t& effect_ ):
      proc_spell_t( "draught_of_souls", effect_.player, effect_.driver(), effect_.item ),
      effect( effect_ ), damage( nullptr )
    {
      channeled = tick_zero = true;
      interrupt_auto_attack = false;
      cooldown -> duration = timespan_t::zero();
      hasted_ticks = false;

      damage = player -> find_action( "felcrazed_rage" );
      if ( damage == nullptr )
      {
        damage = player -> create_proc_action( "felcrazed_rage", effect );
      }

      if ( damage == nullptr )
      {
        damage = new felcrazed_rage_t( effect );
        add_child( damage );
      }
    }

    double composite_haste() const override
    { return 1.0; } // Not hasted.

    void execute() override
    {
      proc_spell_t::execute();

      // Use_item_t (that executes this action) will trigger a player-ready event after execution.
      // Since this action is a "background channel", we'll need to cancel the player ready event to
      // prevent the player from picking something to do while channeling.
      event_t::cancel( player -> readying );
    }

    player_t* select_random_target() const
    {
      if ( sim -> distance_targeting_enabled )
      {
        std::vector<player_t*> targets;
        range::for_each( sim -> target_non_sleeping_list, [ &targets, this ]( player_t* t ) {
          if ( player -> get_player_distance( *t ) <= radius + t -> combat_reach )
          {
            targets.push_back( t );
          }
        } );

        auto random_idx = static_cast<size_t>( rng().range( 0, targets.size() ) );
        return targets.size() ? targets[ random_idx ] : nullptr;
      }
      else
      {
        auto random_idx = static_cast<size_t>( rng().range( 0, sim -> target_non_sleeping_list.size() ) );
        return sim -> target_non_sleeping_list[ random_idx ];
      }
    }

    void tick( dot_t* d ) override
    {
      proc_spell_t::tick( d );

      auto t = select_random_target();

      if ( t )
      {
        damage -> target = t;
        damage -> execute();
      }
    }

    void last_tick( dot_t* d ) override
    {
      // Last_tick() will zero player_t::channeling if this action is being channeled, so check it
      // before calling the parent.
      auto was_channeling = player -> channeling == this;

      proc_spell_t::last_tick( d );

      // Since Draught of Souls must be modeled as a channel (player cannot be allowed to perform
      // any actions for 3 seconds), we need to manually restart the player-ready event immediately
      // after the channel ends. This is because the channel is flagged as a background action,
      // which by default prohibits player-ready generation.
      if ( was_channeling && player -> readying == nullptr )
      {
        // Due to the client not allowing the ability queue here, we have to wait
        // the amount of lag + how often the key is spammed until the next ability is used.
        // Modeling this as 2 * lag for now. Might increase to 3 * lag after looking at logs of people using the trinket.
        timespan_t time = ( player -> world_lag_override ? player -> world_lag : sim -> world_lag ) * 2.0;
        player -> schedule_ready( time );
      }
    }
  };

  effect.execute_action = new draught_of_souls_driver_t( effect );
}

// Entwined Elemental Foci ==================================================

// TODO: How do refresh procs for this trinket work?
void item::entwined_elemental_foci( special_effect_t& effect )
{
  struct entwined_elemental_foci_cb_t : public dbc_proc_callback_t
  {
    std::vector<stat_buff_t*> buffs;

    entwined_elemental_foci_cb_t( const special_effect_t& effect, double amount ) :
      dbc_proc_callback_t( effect.item, effect ),
      buffs( {
        stat_buff_creator_t( effect.player, "fiery_enchant", effect.player -> find_spell( 225726 ), effect.item )
          .cd( timespan_t::zero() )
          .add_stat( STAT_CRIT_RATING, amount ),
        stat_buff_creator_t( effect.player, "frost_enchant", effect.player -> find_spell( 225729 ), effect.item )
          .cd( timespan_t::zero() )
          .add_stat( STAT_MASTERY_RATING, amount ),
        stat_buff_creator_t( effect.player, "arcane_enchant", effect.player -> find_spell( 225730 ), effect.item )
          .cd( timespan_t::zero() )
          .add_stat( STAT_HASTE_RATING, amount )
      } )
    { }

    void execute( action_t*, action_state_t* )
    {
      size_t selected_buff = static_cast<size_t>( rng().range( 0, buffs.size() ) );

      buffs[ selected_buff ] -> trigger();
    }
  };

  double rating_amount = item_database::apply_combat_rating_multiplier( *effect.item,
      effect.driver() -> effectN( 2 ).average( effect.item ) );
  new entwined_elemental_foci_cb_t( effect, rating_amount );
}

// Horn of Valor ============================================================

void item::horn_of_valor( special_effect_t& effect )
{
  effect.custom_buff = stat_buff_creator_t( effect.player, "valarjars_path", effect.driver(), effect.item )
    .add_stat( effect.player -> convert_hybrid_stat( STAT_STR_AGI_INT ), effect.driver() -> effectN( 1 ).average( effect.item ) );
}

// Tiny Oozeling in a Jar ===================================================

struct fetid_regurgitation_t : public proc_spell_t
{
  buff_t* driver_buff;

  fetid_regurgitation_t( special_effect_t& effect, buff_t* b ) :
    proc_spell_t( "fetid_regurgitation", effect.player, effect.driver() -> effectN( 1 ).trigger(), effect.item ),
    driver_buff( b )
  {
    // Set damage amount, since the scaled amount is not in the damage spell.
    base_dd_min = base_dd_max = effect.driver() -> effectN( 1 ).average( item );
  }

  double action_multiplier() const override
  {
    double am = proc_spell_t::action_multiplier();

    am *= driver_buff -> check_value();

    return am;
  }
};

// TOCHECK: Does this hit 6 or 7 times? Tooltip suggests 6 but it the buff lasts long enough for 7 ticks.

void item::tiny_oozeling_in_a_jar( special_effect_t& effect )
{
  struct congealing_goo_callback_t : public dbc_proc_callback_t
  {
    buff_t* goo;

    congealing_goo_callback_t( const special_effect_t* effect, buff_t* cg ) :
      dbc_proc_callback_t( effect -> item, *effect ), goo( cg )
    {}

    void execute( action_t* /* action */, action_state_t* /* state */ ) override
    {
      goo -> trigger();
    }
  };

  buff_t* charges = buff_creator_t( effect.player, "congealing_goo", effect.player -> find_spell( 215126 ), effect.item );

  special_effect_t* goo_effect = new special_effect_t( effect.item );
  goo_effect -> source = SPECIAL_EFFECT_SOURCE_ITEM;
  goo_effect -> spell_id = 215120;
  goo_effect -> proc_flags_ = PROC1_MELEE | PROC1_MELEE_ABILITY;
  goo_effect -> proc_flags2_ = PF2_ALL_HIT;
  effect.player -> special_effects.push_back( goo_effect );

  new congealing_goo_callback_t( goo_effect, charges );

  struct fetid_regurgitation_buff_t : public buff_t
  {
    buff_t* congealing_goo;
    action_t* damage;

    fetid_regurgitation_buff_t( special_effect_t& effect, buff_t* cg ) :
      buff_t( buff_creator_t( effect.player, "fetid_regurgitation", effect.driver(), effect.item )
        .activated( false )
        .tick_zero( true )
        .tick_callback( [ this ] ( buff_t*, int, const timespan_t& ) {
          damage -> schedule_execute();
        } ) ), congealing_goo( cg )
    {
      damage = effect.player -> find_action( "fetid_regurgitation" );
      if ( ! damage )
      {
        damage = effect.player -> create_proc_action( "fetid_regurgitation", effect );
      }

      if ( ! damage )
      {
        damage = new fetid_regurgitation_t( effect, this );
      }
    }

    bool trigger( int stack, double, double chance, timespan_t duration ) override
    {
      if ( congealing_goo -> stack() > 0 )
      {
        bool s = buff_t::trigger( stack, congealing_goo -> stack(), chance, duration );

        congealing_goo -> expire();

        return s;
      }

      return false;
    }
  };

  effect.custom_buff = new fetid_regurgitation_buff_t( effect, charges );
}

// Figurehead of the Naglfar ================================================

struct taint_of_the_sea_t : public proc_spell_t
{
  taint_of_the_sea_t( const special_effect_t& effect ) :
    proc_spell_t( "taint_of_the_sea", effect.player, effect.player -> find_spell( 215695 ), effect.item )
  {
    base_dd_min = base_dd_max = 0;
    base_multiplier = effect.driver() -> effectN( 1 ).percent();
  }

  void init() override
  {
    proc_spell_t::init();

    // Allow DA multipliers so base_multiplier may take effect.
    snapshot_flags = STATE_MUL_DA;
    update_flags = 0;
  }

  // Override so ONLY base_multiplier takes effect.
  double composite_da_multiplier( const action_state_t* ) const override
  { return base_multiplier; }

  void execute() override
  {
    buff_t* d = player -> get_target_data( target ) -> debuff.taint_of_the_sea;

    assert( d && d -> check() );

    base_dd_min = base_dd_max = std::min( base_dd_min, d -> current_value / base_multiplier );

    proc_spell_t::execute();

    d -> current_value -= execute_state -> result_amount;

    // can't assert on any negative number because precision reasons
    assert( d -> current_value >= -1.0 );

    if ( d -> current_value <= 0.0 )
      d -> expire();
  }
};

struct taint_of_the_sea_driver_t : public dbc_proc_callback_t
{
  action_t* damage;
  player_t* player, *active_target;

  taint_of_the_sea_driver_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.player, effect ),
    damage( effect.player -> find_action( "taint_of_the_sea" ) ),
    player( effect.player ), active_target( nullptr )
  {}

  void execute( action_t* /* a */, action_state_t* trigger_state ) override
  {
    assert( active_target );

    if ( trigger_state -> target == active_target )
      return;
    if ( trigger_state -> result_amount <= 0 )
      return;

    assert( player -> get_target_data( active_target ) -> debuff.taint_of_the_sea -> check() );

    damage -> target = active_target;
    damage -> base_dd_min = damage -> base_dd_max = trigger_state -> result_amount;
    damage -> execute();
  }
};

struct figurehead_of_the_naglfar_constructor_t : public item_targetdata_initializer_t
{
  figurehead_of_the_naglfar_constructor_t( unsigned iid, const std::vector< slot_e >& s ) :
    item_targetdata_initializer_t( iid, s )
  {}

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td -> source );
    if ( effect == 0 )
    {
      td -> debuff.taint_of_the_sea = buff_creator_t( *td, "taint_of_the_sea" );
    }
    else
    {
      assert( ! td -> debuff.taint_of_the_sea );

      // Create special effect for the damage transferral.
      special_effect_t* effect2 = new special_effect_t( effect -> item );
      effect2 -> source = SPECIAL_EFFECT_SOURCE_ITEM;
      effect2 -> name_str = "taint_of_the_sea_driver";
      effect2 -> proc_chance_ = 1.0;
      effect2 -> proc_flags_ = PF_ALL_DAMAGE;
      effect2 -> proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
      effect -> player -> special_effects.push_back( effect2 );

      // Create callback; we'll enable this callback whenever the debuff is active.
      taint_of_the_sea_driver_t* callback = new taint_of_the_sea_driver_t( *effect2 );
      callback -> initialize();
      callback -> deactivate();

      td -> debuff.taint_of_the_sea = buff_creator_t( *td, "taint_of_the_sea", effect -> driver() )
      .default_value( effect -> driver() -> effectN( 2 ).trigger() -> effectN( 2 ).average( effect -> item ) )
      .stack_change_callback( [ callback ]( buff_t* b, int old, int new_ )
      {
        if ( old == 0 )
        {
          assert( ! callback -> active );
          callback -> active_target = b -> player;
          callback -> activate();
        }
        else if ( new_ == 0 )
        {
          callback -> active_target = nullptr;
          callback -> deactivate();
        }
      } );
      td -> debuff.taint_of_the_sea -> reset();
    }
  }
};

void item::figurehead_of_the_naglfar( special_effect_t& effect )
{
  action_t* damage_spell = effect.player -> find_action( "taint_of_the_sea" );
  if ( ! damage_spell )
  {
    damage_spell = effect.player -> create_proc_action( "taint_of_the_sea", effect );
  }

  if ( ! damage_spell )
  {
    damage_spell = new taint_of_the_sea_t( effect );
  }

  struct apply_debuff_t : public action_t
  {
    apply_debuff_t( special_effect_t& effect ) :
      action_t( ACTION_OTHER, "apply_taint_of_the_sea", effect.player, spell_data_t::nil() )
    {
      background = quiet = true;
      callbacks = false;
    }

    // Suppress assertion
    result_e calculate_result( action_state_t* ) const override
    { return RESULT_NONE; }

    void execute() override
    {
      action_t::execute();

      player -> get_target_data( target ) -> debuff.taint_of_the_sea -> trigger();
    }
  };

  effect.execute_action = new apply_debuff_t( effect );
}

// Chaos Talisman ===========================================================
// TODO: Stack decay

void item::chaos_talisman( special_effect_t& effect )
{
  effect.proc_flags_ = PF_MELEE;
  effect.proc_flags2_ = PF_ALL_DAMAGE;
  const spell_data_t* buff_spell = effect.driver() -> effectN( 1 ).trigger();
  effect.custom_buff = stat_buff_creator_t( effect.player, "chaotic_energy", buff_spell, effect.item )
    .add_stat( effect.player -> convert_hybrid_stat( STAT_STR_AGI ), buff_spell -> effectN( 1 ).average( effect.item ) )
    .period( timespan_t::zero() ); // disable ticking

  new dbc_proc_callback_t( effect.item, effect );
}

// Corrupted Starlight ======================================================
// TOCHECK: Do these void zones stack?

struct nightfall_t : public proc_spell_t
{
  proc_spell_t* damage_spell;

  nightfall_t( special_effect_t& effect ) :
    proc_spell_t( "nightfall", effect.player, effect.player -> find_spell( 213785 ), effect.item )
  {
    const spell_data_t* tick_spell = effect.player -> find_spell( 213786 );
    proc_spell_t* t = new proc_spell_t( "nightfall_tick", effect.player, tick_spell, effect.item );
    t -> dual = t -> ground_aoe = true;
    t -> stats = stats;
    damage_spell = t;
  }

  void execute() override
  {
    proc_spell_t::execute();

    make_event<ground_aoe_event_t>( *sim, player, ground_aoe_params_t()
      .target( execute_state -> target )
      .x( execute_state -> target -> x_position )
      .y( execute_state -> target -> y_position )
      .duration( data().duration() )
      .start_time( sim -> current_time() )
      .action( damage_spell )
      .pulse_time( timespan_t::from_seconds( 1.0 ) ) );
  }
};

void item::corrupted_starlight( special_effect_t& effect )
{
  effect.execute_action = effect.player -> find_action( "nightfall" );

  if ( ! effect.execute_action )
  {
    effect.execute_action = effect.player -> create_proc_action( "nightfall", effect );
  }

  if ( ! effect.execute_action )
  {
    effect.execute_action = new nightfall_t( effect );
  }

  new dbc_proc_callback_t( effect.item, effect );
}

// Darkmoon Decks ===========================================================

struct darkmoon_deck_t
{
  player_t* player;
  std::array<stat_buff_t*, 8> cards;
  buff_t* top_card;
  timespan_t shuffle_period;

  darkmoon_deck_t( special_effect_t& effect, std::array<unsigned, 8> c ) :
    player( effect.player ), top_card( nullptr ),
    shuffle_period( effect.driver() -> effectN( 1 ).period() )
  {
    for ( unsigned i = 0; i < 8; i++ )
    {
      const spell_data_t* s = player -> find_spell( c[ i ] );
      assert( s -> found() );

      std::string n = s -> name_cstr();
      util::tokenize( n );

      cards[ i ] = stat_buff_creator_t( player, n, s, effect.item );
    }
  }

  void shuffle()
  {
    if ( top_card )
      top_card -> expire();

    top_card = cards[ ( int ) player -> rng().range( 0, 8 ) ];

    top_card -> trigger();
  }
};

struct shuffle_event_t : public event_t
{
  darkmoon_deck_t* deck;

  static timespan_t delta_time( sim_t& sim, bool initial, darkmoon_deck_t* deck )
  {
    if ( initial )
    {
      return deck->shuffle_period * sim.rng().real();
    }
    else
    {
      return deck->shuffle_period;
    }
  }

  shuffle_event_t( darkmoon_deck_t* d, bool initial = false )
    : event_t( *d->player, delta_time( *d -> player -> sim, initial, d ) ), deck( d )
  {
    /* Shuffle when we schedule an event instead of when it executes.
    This will assure the deck starts shuffled */
    deck->shuffle();
  }

  const char* name() const override
  { return "shuffle_event"; }

  void execute() override
  {
    make_event<shuffle_event_t>( sim(), deck );
  }
};

// TODO: The sim could use an "arise" and "demise" callback, it's kinda wasteful to call these
// things per every player actor shifting in the non-sleeping list. Another option would be to make
// (yet another list) that holds active, non-pet players.
// TODO: Also, the darkmoon_deck_t objects are not cleaned up at the end
void item::darkmoon_deck( special_effect_t& effect )
{
  std::array<unsigned, 8> cards;
  switch( effect.spell_id )
  {
  case 191632: // Immortality
    cards = { { 191624, 191625, 191626, 191627, 191628, 191629, 191630, 191631 } };
    break;
  case 191611: // Hellfire
    cards = { { 191603, 191604, 191605, 191606, 191607, 191608, 191609, 191610 } };
    break;
  case 191563: // Dominion
    cards = { { 191545, 191548, 191549, 191550, 191551, 191552, 191553, 191554 } };
    break;
  default:
    assert( false );
  }

  darkmoon_deck_t* d = new darkmoon_deck_t( effect, cards );

  effect.player -> sim -> player_non_sleeping_list.register_callback([ d, &effect ]( player_t* player ) {
    // Arise time gets set to timespan_t::min() in demise, before the actor is removed from the
    // non-sleeping list. In arise, the arise_time is set to current time before the actor is added
    // to the non-sleeping list.
    if ( player != effect.player || player -> arise_time < timespan_t::zero() )
    {
      return;
    }

    make_event<shuffle_event_t>( *effect.player -> sim, d, true );
  });
}

// Elementium Bomb Squirrel =================================================

struct aw_nuts_t : public proc_spell_t
{
  aw_nuts_t( special_effect_t& effect ) :
    proc_spell_t( "aw_nuts", effect.player, effect.player -> find_spell( 216099 ), effect.item )
  {
    travel_speed = 7.0; // "Charge"!

    // Set damage amount, as the scaled value isn't stored in the damage spell.
    base_dd_min = base_dd_max = effect.driver() -> effectN( 1 ).average( item );
  }

  void init() override
  {
    proc_spell_t::init();

    // Don't benefit from player multipliers because, in game, the squirrel is dealing the damage, not you.
    snapshot_flags &= ~( STATE_MUL_DA | STATE_MUL_PERSISTENT | STATE_TGT_MUL_DA );
  }
};

void item::elementium_bomb_squirrel( special_effect_t& effect )
{
  effect.execute_action = effect.player -> find_action( "aw_nuts" );

  if ( ! effect.execute_action )
  {
    effect.execute_action = effect.player -> create_proc_action( "aw_nuts", effect );
  }

  if ( ! effect.execute_action )
  {
    effect.execute_action = new aw_nuts_t( effect );
  }

  new dbc_proc_callback_t( effect.item, effect );
}


// Kil'jaeden's Burning Wish ================================================

struct kiljaedens_burning_wish_t : public spell_t
{
  kiljaedens_burning_wish_t( const special_effect_t& effect ) :
    spell_t( "kiljaedens_burning_wish", effect.player, effect.player -> find_spell( 235999 ) )
  {
    background = may_crit = true;
    aoe = -1;
    item = effect.item;
    school = SCHOOL_FIRE;
    base_dd_min = base_dd_max = data().effectN( 1 ).average( effect.item );

    // Projectile is very slow, combat log show 8 seconds to travel the maximum 80 yard range of the item
    travel_speed = 10;
  }

  virtual void init() override
  {
    spell_t::init();
    // Through testing with Rune of Power, Incanter's Flow, Arcane Power,
    // and Enhacement multipliers we conclude this ignores all standard %dmg
    // multipliers. It still gains crit damage multipliers.
    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_TGT_MUL_DA;
  }


  // This always crits.
  virtual double composite_crit_chance() const override
  { return 1.0; }

};

void item::kiljadens_burning_wish( special_effect_t& effect )
{
  effect.execute_action = new kiljaedens_burning_wish_t( effect );
}
// Nature's Call ============================================================

// Helper class so we can handle all of the procs as 1 object.

struct natures_call_proc_t
{
  action_t* action;
  buff_t* buff;

  natures_call_proc_t( action_t* a ) : action( a ), buff( nullptr )
  {
  }

  natures_call_proc_t( buff_t* b ) : action( nullptr ), buff( b )
  {
  }

  void execute( player_t* t )
  {
    if ( action )
    {
      action -> target = t;
      action -> schedule_execute();
    }
    else
    {
      buff -> trigger();
    }
  }

  bool active()
  { return buff && buff -> check(); };
};

struct natures_call_callback_t : public dbc_proc_callback_t
{
  std::vector<natures_call_proc_t*> procs;

  natures_call_callback_t( const special_effect_t& effect, std::vector<natures_call_proc_t*> p ) :
    dbc_proc_callback_t( effect.item, effect ), procs( p )
  {}

  void execute( action_t* /* a */, action_state_t* call_data ) override
  {
    // Nature's Call prefers to proc inactive buffs over active ones.
    // Make a vector with only the inactive buffs.
    std::vector<natures_call_proc_t*> inactive_procs;

    for ( unsigned i = 0; i < procs.size(); i++ )
    {
      if ( ! procs[ i ] -> active() )
      {
        inactive_procs.push_back( procs[ i ] );
      }
    }

    // Roll it!
    int roll = ( int ) ( listener -> sim -> rng().real() * inactive_procs.size() );
    inactive_procs[ roll ] -> execute( call_data -> target );
  }
};

void item::natures_call( special_effect_t& effect )
{
  double rating_amount = item_database::apply_combat_rating_multiplier( *effect.item,
      effect.driver() -> effectN( 2 ).average( effect.item ) );

  std::vector<natures_call_proc_t*> procs;

  procs.push_back( new natures_call_proc_t(
    stat_buff_creator_t( effect.player, "cleansed_ancients_blessing", effect.player -> find_spell( 222517 ) )
    .activated( false )
    .add_stat( STAT_CRIT_RATING, rating_amount ) ) );
  procs.push_back( new natures_call_proc_t(
    stat_buff_creator_t( effect.player, "cleansed_sisters_blessing", effect.player -> find_spell( 222519 ) )
    .activated( false )
    .add_stat( STAT_HASTE_RATING, rating_amount ) ) );
  procs.push_back( new natures_call_proc_t(
    stat_buff_creator_t( effect.player, "cleansed_wisps_blessing", effect.player -> find_spell( 222518 ) )
    .activated( false )
    .add_stat( STAT_MASTERY_RATING, rating_amount ) ) );

  // Set trigger spell so we can automatically create the breath action.
  effect.trigger_spell_id = 222520;
  procs.push_back( new natures_call_proc_t( effect.create_action() ) );

  // Disable trigger spell again
  effect.trigger_spell_id = 0;

  new natures_call_callback_t( effect, procs );
}

// Moonlit Prism ============================================================
// TOCHECK: Proc mechanics

void item::moonlit_prism( special_effect_t& effect )
{
  // Create stack gain driver
  special_effect_t* effect2 = new special_effect_t( effect.item );
  effect2 -> source = SPECIAL_EFFECT_SOURCE_ITEM;
  effect2 -> name_str     = "moonlit_prism_driver";
  effect2 -> proc_chance_ = 1.0;
  effect2 -> spell_id = effect.driver() -> id();
  effect2 -> cooldown_ = timespan_t::zero();
  effect.player -> special_effects.push_back( effect2 );

  // Create callback; it will be enabled when the buff is active.
  dbc_proc_callback_t* callback = new dbc_proc_callback_t( effect.player, *effect2 );

  // Create buff.
  effect.custom_buff = stat_buff_creator_t( effect.player, "elunes_light", effect.driver(), effect.item )
    .cd( timespan_t::zero() )
    .refresh_behavior( BUFF_REFRESH_DISABLED )
    .stack_change_callback( [ callback ]( buff_t*, int old, int new_ )
    {
      if ( old == 0 ) {
        assert( ! callback -> active );
        callback -> activate();
      } else if ( new_ == 0 )
        callback -> deactivate();
    } );

  // Assign buff to our stack gain driver.
  effect2 -> custom_buff = effect.custom_buff;

  callback -> initialize();
  callback -> deactivate();
}

// Faulty Countermeasures ===================================================

void item::faulty_countermeasures( special_effect_t& effect )
{
  // Create effect & callback for the damage proc
  special_effect_t* effect2 = new special_effect_t( effect.item );
  effect2 -> source         = SPECIAL_EFFECT_SOURCE_ITEM;
  effect2 -> name_str       = "brittle";
  effect2 -> spell_id       = effect.driver() -> id();
  effect2 -> cooldown_      = timespan_t::zero();

  effect.player -> special_effects.push_back( effect2 );

  dbc_proc_callback_t* callback = new dbc_proc_callback_t( effect.player, *effect2 );
  callback -> initialize();
  callback -> deactivate();

  effect.custom_buff = buff_creator_t( effect.player, "sheathed_in_frost", effect.driver(), effect.item )
    .cd( timespan_t::zero() )
    .activated( false )
    .chance( 1 )
    .stack_change_callback( [ callback ]( buff_t*, int old, int new_ )
    {
      if ( old == 0 ) {
        assert( ! callback -> active );
        callback -> activate();
      } else if ( new_ == 0 )
        callback -> deactivate();
    } );
}

// Stabilized Energy Pendant ================================================

void item::stabilized_energy_pendant( special_effect_t& effect )
{
  double value = 1.0 + effect.driver() -> effectN( 1 ).percent();
  switch ( effect.player -> type )
  {
    case DEATH_KNIGHT:
      effect.player -> resources.initial_multiplier[ RESOURCE_RUNIC_POWER ] *= value;
      break;
    case DEMON_HUNTER:
      effect.player -> resources.initial_multiplier[ RESOURCE_FURY ] *= value;
      effect.player -> resources.initial_multiplier[ RESOURCE_PAIN ] *= value;
      break;
    case DRUID:
      effect.player -> resources.initial_multiplier[ RESOURCE_MANA ] *= value;
      effect.player -> resources.initial_multiplier[ RESOURCE_ENERGY ] *= value;
      effect.player -> resources.initial_multiplier[ RESOURCE_RAGE ] *= value;
      effect.player -> resources.initial_multiplier[ RESOURCE_ASTRAL_POWER ] *= value;
      break;
    case HUNTER:
      effect.player -> resources.initial_multiplier[ RESOURCE_FOCUS ] *= value;
      break;
    case MONK:
      effect.player -> resources.initial_multiplier[ RESOURCE_MANA ] *= value;
      effect.player -> resources.initial_multiplier[ RESOURCE_ENERGY ] *= value;
      break;
    case PRIEST:
      effect.player -> resources.initial_multiplier[ RESOURCE_MANA ] *= value;
      effect.player -> resources.initial_multiplier[ RESOURCE_INSANITY ] *= value;
      break;
    case ROGUE:
      effect.player -> resources.initial_multiplier[ RESOURCE_ENERGY ] *= value;
      break;
    case SHAMAN:
      effect.player -> resources.initial_multiplier[ RESOURCE_MANA ] *= value;
      effect.player -> resources.initial_multiplier[ RESOURCE_MAELSTROM ] *= value;
      break;
    case WARRIOR:
      effect.player -> resources.initial_multiplier[ RESOURCE_RAGE ] *= value;
      break;
    default:
      effect.player -> resources.initial_multiplier[ RESOURCE_MANA ] *= value;
      break;
  }
}
// Stormsinger Fulmination Charge ===========================================

// 9 seconds ascending, 2 seconds at max stacks, 9 seconds descending. TOCHECK
struct focused_lightning_t : public stat_buff_t
{
  focused_lightning_t( const special_effect_t& effect ) :
    stat_buff_t( stat_buff_creator_t( effect.player, "focused_lightning", effect.trigger() -> effectN( 1 ).trigger(), effect.item )
      .duration( timespan_t::from_seconds( 20.0 ) ) )
  { }

  void bump( int stacks, double value ) override
  {
    bool waste = current_stack == max_stack();

    stat_buff_t::bump( stacks, value );

    if ( waste )
    {
      if ( sim -> debug )
        sim -> out_debug.printf( "%s buff %s changes to reverse.", player -> name(), name() );

      reverse = true;
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    stat_buff_t::expire_override( expiration_stacks, remaining_duration );

    reverse = false;
  }

  void reset() override
  {
    stat_buff_t::reset();

    reverse = false;
  }
};

void item::stormsinger_fulmination_charge( special_effect_t& effect )
{
  effect.custom_buff = new focused_lightning_t( effect );

  new dbc_proc_callback_t( effect.item, effect );
}

// Terrorbound Nexus ========================================================

struct shadow_wave_callback_t : public dbc_proc_callback_t
{
  action_t* shadow_wave;

  shadow_wave_callback_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.item, effect ),
    shadow_wave( effect.player -> find_action( "shadow_wave" ) )
  {}

  void execute( action_t* /* a */, action_state_t* s ) override
  {
    // 2 second return time, from in-game combat logs.
    make_event<ground_aoe_event_t>( *effect.player -> sim, effect.player, ground_aoe_params_t()
      .target( s -> target )
      .x( effect.player -> x_position )
      .y( effect.player -> y_position )
      .duration( timespan_t::from_seconds( 2.0 ) )
      .start_time( effect.player -> sim -> current_time() )
      .action( shadow_wave )
      .pulse_time( timespan_t::from_seconds( 2.0 ) ), true );
  }
};

void item::terrorbound_nexus( special_effect_t& effect )
{
  effect.trigger_spell_id = 215047;
  action_t* a = effect.initialize_offensive_spell_action();
  a -> aoe = -1;
  a -> radius = 15;
  a -> base_dd_min = a -> base_dd_max = effect.driver() -> effectN( 1 ).average( effect.item );

  new shadow_wave_callback_t( effect );
}

// Unstable Horrorslime =====================================================

struct volatile_ichor_t : public spell_t
{
  volatile_ichor_t( const special_effect_t& effect ) :
    spell_t( "volatile_ichor", effect.player, effect.player -> find_spell( 222187 ) )
  {
    background = may_crit = true;
    //TODO: Is this true?
    callbacks = false;
    item = effect.item;
    school = SCHOOL_NATURE; // spell 222197 has this information, but doesn't have the damage.

    base_dd_min = base_dd_max = effect.driver() -> effectN( 1 ).average( effect.item );

    aoe = -1;
    //FIXME: Assume this is kind of slow from wording.
    //       Get real velocity from in game data after raids open.
    travel_speed = 25;
  }

};

void item::unstable_horrorslime( special_effect_t& effect )
{
  action_t* action = effect.player -> find_action( "volatile_ichor" ) ;
  if ( ! action )
  {
    action = effect.player -> create_proc_action( "volatile_ichor", effect );
  }

  if ( ! action )
  {
    action = new volatile_ichor_t( effect );
  }

  effect.execute_action = action;
  effect.proc_flags2_ = PF2_ALL_HIT;

  new dbc_proc_callback_t( effect.player, effect );
}

// Portable Manacracker =====================================================

struct volatile_magic_debuff_t : public buff_t
{
  action_t* damage;

  volatile_magic_debuff_t( const special_effect_t& effect, actor_target_data_t& td ) :
    buff_t( buff_creator_t( td, "volatile_magic", effect.trigger() ) ),
    damage( effect.player -> find_action( "withering_consumption" ) )
  {}

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool s = buff_t::trigger( stacks, value, chance, duration );

    if ( current_stack == max_stack() )
    {
      damage -> target = player;
      damage -> schedule_execute();
      expire();
    }

    return s;
  }
};

struct portable_manacracker_constructor_t : public item_targetdata_initializer_t
{
  portable_manacracker_constructor_t( unsigned iid, const std::vector< slot_e >& s ) :
    item_targetdata_initializer_t( iid, s )
  {}

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td -> source );
    if ( effect == 0 )
    {
      td -> debuff.volatile_magic = buff_creator_t( *td, "volatile_magic" );
    }
    else
    {
      assert( ! td -> debuff.volatile_magic );

      td -> debuff.volatile_magic = new volatile_magic_debuff_t( *effect, *td );
      td -> debuff.volatile_magic -> reset();
    }
  }
};

struct volatile_magic_callback_t : public dbc_proc_callback_t
{
  volatile_magic_callback_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.item, effect )
  {}

  void execute( action_t* a, action_state_t* s ) override
  {
    actor_target_data_t* td = a -> player -> get_target_data( s -> target );

    if ( ! td ) return;

    td -> debuff.volatile_magic -> trigger();
  }
};

void item::portable_manacracker( special_effect_t& effect )
{
  // Set spell so we can create the DoT action.
  effect.trigger_spell_id = 215884;
  effect.proc_flags2_ = PF2_CRIT;
  action_t* dot = effect.create_action();
  // Disable trigger spell again.
  effect.trigger_spell_id = 0;
  dot -> base_td = effect.trigger() -> effectN( 1 ).average( effect.item );
  dot -> tick_zero = true;

  new volatile_magic_callback_t( effect );
}

// Infernal Alchemist Stone =================================================

void item::infernal_alchemist_stone( special_effect_t& effect )
{
  const spell_data_t* stat_spell = effect.player -> find_spell( 60229 );

  effect.custom_buff = stat_buff_creator_t( effect.player, "infernal_alchemist_stone", effect.driver(), effect.item )
    .duration( stat_spell -> duration() )
    .add_stat( effect.player -> convert_hybrid_stat( STAT_STR_AGI_INT ), effect.driver() -> effectN( 1 ).average( effect.item ) )
    .chance( 1 ); // RPPM is handled by the special effect, so make the buff always go up

  new dbc_proc_callback_t( effect.item, effect );
}

// Marfisis's Giant Censer ==================================================

void item::marfisis_giant_censer( special_effect_t& effect )
{
  const spell_data_t* driver = effect.player -> find_spell( effect.spell_id );
  effect.player -> buffs.incensed = buff_creator_t( effect.player, "incensed", driver -> effectN( 1 ).trigger() )
    .default_value( driver -> effectN( 1 ).trigger() -> effectN( 1 ).average( effect.item ) / 100 )
    .chance ( 1 );
  effect.custom_buff = effect.player -> buffs.incensed;

  new dbc_proc_callback_t( effect.player, effect );
}

// Devilsaur's Bite =========================================================

void item::devilsaurs_bite( special_effect_t& effect )
{
  auto a = effect.create_action();
  // Devilsaur's bite ignores armor
  a -> snapshot_flags &= ~STATE_TGT_ARMOR;

  effect.execute_action = a;

  new dbc_proc_callback_t( effect.item, effect );
}

// Ley Spark =========================================================

void item::leyspark( special_effect_t& effect )
{
  struct sparking_driver_t : public buff_t
  {
    stat_buff_t* sparking_;
    sparking_driver_t( const special_effect_t& effect, stat_buff_t* sparking ) :
      buff_t( buff_creator_t( effect.player, "sparking_driver", effect.driver() -> effectN( 1 ).trigger() )
              .tick_callback( [sparking]( buff_t*, int, const timespan_t& ) { sparking -> trigger( 1 ); } ) ),
      sparking_( sparking )
    {
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      buff_t::expire_override( expiration_stacks, remaining_duration );

      sparking_ -> expire();
    }
  };

  stat_buff_t* sparking = stat_buff_creator_t( effect.player, "sparking", effect.driver() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
    .default_value( effect.driver() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() -> effectN( 1 ).average( effect.item ) )
    .add_invalidate( CACHE_AGILITY );

  effect.custom_buff = new sparking_driver_t( effect, sparking );

  new dbc_proc_callback_t( effect.item, effect );
}

// Spontaneous Appendages ===================================================
struct spontaneous_appendages_t: public proc_spell_t
{
  spontaneous_appendages_t( const special_effect_t& effect ):
    proc_spell_t( "horrific_slam", effect.player,
      effect.player -> find_spell( effect.trigger() -> effectN( 1 ).trigger() -> id() ),
      effect.item )
  {}

  double target_armor( player_t* ) const override
  { return 0.0; }
};

void item::spontaneous_appendages( special_effect_t& effect )
{
  // Set trigger spell so we can create an action from it.
  action_t* slam = effect.player -> find_action( "horrific_slam" );
  if ( ! slam )
  {
    slam = effect.player -> create_proc_action( "horrific_slam", effect );
  }

  if ( ! slam )
  {
    slam = new spontaneous_appendages_t( effect );
  }


  effect.custom_buff = buff_creator_t( effect.player, "horrific_appendages", effect.trigger(), effect.item )
    .tick_callback( [ slam ]( buff_t*, int, const timespan_t& ) {
      slam -> schedule_execute();
    } );

  new dbc_proc_callback_t( effect.item, effect );
}

// Wriggling Sinew ==========================================================

struct wriggling_sinew_constructor_t : public item_targetdata_initializer_t
{
  struct maddening_whispers_debuff_t : public buff_t
  {
    action_t* action;

    maddening_whispers_debuff_t( const special_effect_t& effect, actor_target_data_t& td ) :
      buff_t( buff_creator_t( td, "maddening_whispers", effect.trigger(), effect.item ) ),
      action( effect.player -> find_action( "maddening_whispers" ) )
    {}

    void expire_override( int stacks, timespan_t dur ) override
    {
      buff_t::expire_override( stacks, dur );

      // Schedule an execute, but snapshot state right now so we can apply the stack multiplier.
      action_state_t* s   = action -> get_state();
      s -> target         = player;
      action -> snapshot_state( s, DMG_DIRECT );
      s -> target_da_multiplier *= stacks;
      action -> schedule_execute( s );
    }
  };

  wriggling_sinew_constructor_t( unsigned iid, const std::vector< slot_e >& s ) :
    item_targetdata_initializer_t( iid, s )
  {}

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td -> source );
    if ( effect == 0 )
    {
      td -> debuff.maddening_whispers = buff_creator_t( *td, "maddening_whispers" );
    }
    else
    {
      assert( ! td -> debuff.maddening_whispers );

      td -> debuff.maddening_whispers = new maddening_whispers_debuff_t( *effect, *td );
      td -> debuff.maddening_whispers -> reset();
    }
  }
};

struct maddening_whispers_cb_t : public dbc_proc_callback_t
{
  buff_t* buff;

  maddening_whispers_cb_t( const special_effect_t& effect, buff_t* b ) :
    dbc_proc_callback_t( effect.player, effect ), buff( b )
  {}

  void execute( action_t*, action_state_t* s ) override
  {
    if ( s -> target == s -> action -> player ) return;
    if ( s -> result_amount <= 0 ) return;

    actor_target_data_t* td = s -> action -> player -> get_target_data( s -> target );

    if ( ! td ) return;

    td -> debuff.maddening_whispers -> trigger();
    buff -> decrement();
  }
};

struct maddening_whispers_t : public buff_t
{
  dbc_proc_callback_t* callback;

  maddening_whispers_t( const special_effect_t& effect ) :
    buff_t( buff_creator_t( effect.player, "maddening_whispers", effect.driver(), effect.item ).cd( timespan_t::zero() ) )
  {
    // Stack gain effect
    special_effect_t* effect2 = new special_effect_t( effect.item );
    effect2 -> source       = SPECIAL_EFFECT_SOURCE_ITEM;
    effect2 -> name_str     = "maddening_whispers_driver";
    effect2 -> proc_chance_ = 1.0;
    effect2 -> proc_flags_  = PF_SPELL | PF_AOE_SPELL;
    effect2 -> proc_flags2_  = PF2_ALL_HIT;
    effect.player -> special_effects.push_back( effect2 );

    callback = new maddening_whispers_cb_t( *effect2, this );
    callback -> initialize();
    callback -> deactivate();
  }

  void start( int, double value, timespan_t duration ) override
  {
    // Always start at max stacks.
    buff_t::start( max_stack(), value, duration );

    callback -> activate();
  }

  void expire_override( int stacks, timespan_t remaining ) override
  {
    buff_t::expire_override( stacks, remaining );

    callback -> deactivate();

    for ( size_t i = 0; i < sim -> target_non_sleeping_list.size(); i++ ) {
      player_t* t = sim -> target_non_sleeping_list[ i ];
      player -> get_target_data( t ) -> debuff.maddening_whispers -> expire();
    }
  }
};

void item::wriggling_sinew( special_effect_t& effect )
{
  // Set triggered spell so we can create an action from it.
  effect.trigger_spell_id = 222050;
  action_t* a = effect.initialize_offensive_spell_action();
  a -> base_dd_min = a -> base_dd_max = effect.driver() -> effectN( 1 ).average( effect.item );
  // Reset triggered spell; we don't want to trigger a spell on use.
  effect.trigger_spell_id = 0;

  effect.custom_buff = new maddening_whispers_t( effect );
}

// Convergence of Fates =====================================================

struct convergence_cd_t
{
  specialization_e spec;
  const char* cooldowns[3];
};

static const convergence_cd_t convergence_cds[] =
{
  /* !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!!

    Spells that trigger buffs must have the cooldown of their buffs removed
    if they have one, or this trinket may cause undesirable results.

  !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! */
  { DEATH_KNIGHT_FROST,   { "empower_rune_weapon", "hungering_rune_weapon" } },
  { DEATH_KNIGHT_UNHOLY,  { "summon_gargoyle", "dark_arbiter" } },
  { DRUID_FERAL,          { "berserk", "incarnation" } },
  { HUNTER_BEAST_MASTERY, { "aspect_of_the_wild" } },
  { HUNTER_MARKSMANSHIP,  { "trueshot" } },
  { HUNTER_SURVIVAL,      { "aspect_of_the_eagle" } },
  { MONK_WINDWALKER,      { "storm_earth_and_fire", "serenity" } },
  { PALADIN_RETRIBUTION,  { "avenging_wrath", "crusade" } },
  { ROGUE_SUBTLETY,       { "shadow_blades" } },
  { ROGUE_OUTLAW,         { "adrenaline_rush" } },
  { ROGUE_ASSASSINATION,  { "vendetta" } },
  { SHAMAN_ENHANCEMENT,   { "feral_spirit" } },
  { WARRIOR_ARMS,         { "battle_cry" } },
  { WARRIOR_FURY,         { "battle_cry" } },
  { DEMON_HUNTER_HAVOC,   { "metamorphosis" } },
  { SPEC_NONE,            { 0 } }
};

struct convergence_of_fates_callback_t : public dbc_proc_callback_t
{
  std::vector<cooldown_t*> cooldowns;
  timespan_t amount;

  convergence_of_fates_callback_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.item, effect ),
    amount( timespan_t::from_seconds( -effect.driver() -> effectN( 1 ).base_value() ) )
  {
    const convergence_cd_t* cd = &( convergence_cds[ 0 ] );
    do
    {
      if ( effect.player -> specialization() != cd -> spec )
      {
        cd++;
        continue;
      }

      for ( size_t i = 0; i < 3; i++ )
      {
        if ( ! cd -> cooldowns[ i ] )
          continue;

        cooldown_t* c = effect.player -> get_cooldown( cd -> cooldowns[ i ] );

        if ( c )
          cooldowns.push_back( c );
      }

      break;
    } while ( cd -> spec != SPEC_NONE );
  }

  void execute( action_t*, action_state_t* ) override
  {
    assert( cooldowns.size() > 0 );

    for ( size_t i = 0; i < cooldowns.size(); i++ )
    {
      cooldowns[ i ] -> adjust( amount );
    }
  }
};

bool player_talent( player_t* player_, const std::string talent )
{
  return player_ -> find_talent_spell( talent ) -> ok();
}

void item::convergence_of_fates( special_effect_t& effect )
{
  switch ( effect.player -> specialization() )
  {
  case PALADIN_RETRIBUTION:
    if ( player_talent( effect.player, "Crusade" ) )
    {
      effect.ppm_ = -1.5;
      effect.rppm_modifier_ = 1.0;
    }
    else
    {
      effect.ppm_ = -4.2; //Blizz fudged up the spelldata for ret, 4.2 is the baseline, not 3. 
      effect.rppm_modifier_ = 1.0;
    }
    break;
  case MONK_WINDWALKER:
    if ( player_talent( effect.player, "Serenity" ) )
    {
      effect.ppm_ = -1.6;
      effect.rppm_modifier_ = 1.0;
    }
    break;
  case DEATH_KNIGHT_FROST:
    if ( player_talent( effect.player, "Hungering Rune Weapon" ) )
    {
      effect.ppm_ = -4.62;
      effect.rppm_modifier_ = 1.0;
    }
    break;
  case DEATH_KNIGHT_UNHOLY:
    if ( ! player_talent( effect.player, "Dark Arbiter" ) )
    {
      effect.ppm_ = -4.98;
      effect.rppm_modifier_ = 1.0;
    }
    break;
  case DRUID_FERAL:
    if ( player_talent( effect.player, "Incarnation: King of the Jungle" ) )
    {
      effect.ppm_ = -3.7;
      effect.rppm_modifier_ = 1.0;
    }
    break;
  default: break;
  }

  new convergence_of_fates_callback_t( effect );
}

// Twisting Wind ============================================================
/* FIXME: How many times does this really hit? It has a small radius and
  moves around so it doesn't hit every tick. A: Large mobs will get >80-90% of ticks. Normal mobs will get 30-50% ticks.
   TOCHECK: Does this AoE stack? A: Sort of, yes. */

struct tormenting_cyclone_t : public proc_spell_t
{
  proc_spell_t* damage_spell;

  tormenting_cyclone_t( special_effect_t& effect ) :
    proc_spell_t( "tormenting_cyclone", effect.player, effect.trigger(), effect.item )
  {
    damage_spell = new proc_spell_t( "tormenting_cyclone_tick", effect.player, effect.player -> find_spell( 221865 ), effect.item );
    damage_spell -> dual = true;
    damage_spell -> stats = stats;
    damage_spell -> base_dd_min = damage_spell -> base_dd_max =
      effect.driver() -> effectN( 1 ).average( effect.item );
  }

  void execute() override
  {
    proc_spell_t::execute();

    make_event<ground_aoe_event_t>( *sim, player, ground_aoe_params_t()
      .target( execute_state -> target )
      .x( execute_state -> target -> x_position )
      .y( execute_state -> target -> y_position )
      .duration( data().duration() )
      .start_time( sim -> current_time() )
      .action( damage_spell )
      .pulse_time( timespan_t::from_seconds( 1.42 ) ) ); //NOTE: We change the pulse time here to better model #ticks
  }
};

void item::twisting_wind( special_effect_t& effect )
{
  effect.execute_action = effect.player -> find_action( "tormenting_cyclone" );

  if ( ! effect.execute_action )
  {
    effect.execute_action = effect.player -> create_proc_action( "tormenting_cyclone", effect );
  }

  if ( ! effect.execute_action )
  {
    effect.execute_action = new tormenting_cyclone_t( effect );
  }

  new dbc_proc_callback_t( effect.item, effect );
}

// Bloodthirsty Instinct ====================================================

// Oct 6 2016: Proc rate on training dummies is not appreciably higher than
//  in raid logs, so don't need a custom implementation for now until there's
//  some insight on how this thing actually works.

/* struct bloodthirsty_instinct_cb_t : public dbc_proc_callback_t
{
  bloodthirsty_instinct_cb_t( special_effect_t& effect ) :
    dbc_proc_callback_t( effect.item, effect )
  {}

  void trigger( action_t* a, void* call_data ) override
  {
    assert( rppm );

    action_state_t* s = static_cast<action_state_t*>( call_data );

    assert( s -> target );

    // Sep 29 2016: From 33 Heroic Ursoc logs, actors averaged 40.75% uptime.
    // 3 RPPM (from spell data) translates to ~45% uptime. Assuming that is the
    // peak RPPM, I adjusted the "base" proc rate down until the average uptime
    // fit the observed uptime in the logs

    // End result here is: 2.25 RPPM scaling linearly with the target's health
    // deficit up to a maximum of 3.00 RPPM
    double mod = 0.75 + 0.25 * ( 100.0 - s -> target -> health_percentage() ) / 100.0;

    if ( effect.player -> sim -> debug )
      effect.player -> sim -> out_debug.printf( "Player %s adjusts %s rppm modifier: old=%.3f new=%.3f",
        effect.player -> name(), effect.name().c_str(), rppm -> get_modifier(), mod );

    rppm -> set_modifier( mod );

    dbc_proc_callback_t::trigger( a, call_data );
  }
};

void item::bloodthirsty_instinct( special_effect_t& effect )
{
  new bloodthirsty_instinct_cb_t( effect );
} */

// Ravaged Seed Pod =========================================================

struct infested_ground_t : public proc_spell_t
{
  proc_spell_t* damage_spell;

  infested_ground_t( special_effect_t& effect ) :
    proc_spell_t( "infested_ground", effect.player, effect.driver(), effect.item )
  {
    damage_spell = new proc_spell_t( "infested_ground_tick", effect.player, effect.player -> find_spell( 221804 ), effect.item );
    damage_spell -> dual = damage_spell -> ground_aoe = true;
    damage_spell -> stats = stats;
    damage_spell -> base_dd_min = damage_spell -> base_dd_max =
      effect.driver() -> effectN( 2 ).average( effect.item );
  }

  void execute() override
  {
    proc_spell_t::execute();

    make_event<ground_aoe_event_t>( *sim, player, ground_aoe_params_t()
      .target( execute_state -> target )
      .x( player -> x_position )
      .y( player -> y_position )
      .duration( data().duration() )
      .start_time( sim -> current_time() )
      .action( damage_spell )
      .pulse_time( timespan_t::from_seconds( 1.0 ) ) );
  }
};

void item::ravaged_seed_pod( special_effect_t& effect )
{
  effect.execute_action = effect.player -> find_action( "infested_ground" );

  if ( ! effect.execute_action )
  {
    effect.execute_action = effect.player -> create_proc_action( "infested_ground", effect );
  }

  if ( ! effect.execute_action )
  {
    effect.execute_action = new infested_ground_t( effect );
  }

  // FIXME: Only while in the area of effect.
  effect.custom_buff = stat_buff_creator_t( effect.player, "leeching_pestilence", effect.player -> find_spell( 221805 ), effect.item )
    .add_stat( STAT_LEECH_RATING, effect.driver() -> effectN( 3 ).average( effect.item ) );
}

// March of the Legion ======================================================

void set_bonus::march_of_the_legion( special_effect_t&  effect ) {
    const spell_data_t* spell = effect.player->find_spell( 228445 );

    std::string spell_name = spell->name_cstr();
    util::tokenize( spell_name );

    struct march_t : public spell_t
    {
      march_t(player_t* player) :
        spell_t("march_of_the_legion", player, player -> find_spell( 228446 ) )
      {
        background = proc = may_crit = true;
        callbacks = false;
        if (player->target->race != RACE_DEMON) {
          base_dd_min = 0;
          base_dd_max = 0;
        }
      }
    };

    effect.execute_action = new march_t( effect.player );

    new dbc_proc_callback_t(effect.player, effect);
}

// Cinidaria, the Symbiote ==================================================

struct cinidaria_the_symbiote_damage_t : public attack_t
{
  cinidaria_the_symbiote_damage_t( player_t* p ) :
    attack_t( "symbiote_strike", p, p -> find_spell( 207694 ) )
  {
    callbacks = may_crit = may_miss = false;
    background = true;
  }

  // TODO: Any multipliers for this?
  void init() override
  {
    attack_t::init();

    snapshot_flags = update_flags = 0;
  }
};

struct cinidaria_the_symbiote_t : public class_scoped_callback_t
{
  cinidaria_the_symbiote_t() : class_scoped_callback_t( { DEMON_HUNTER, DRUID, MONK, ROGUE } )
  { }

  // Cinidaria needs special handling on the modeling. Since it is allowed to proc from nearly any
  // damage, we need to circumvent the normal "special effect" system in Simulationcraft. Instead,
  // hook Cinidaria into the outgoing damage assessor pipeline, since all outgoing damage resolution
  // of a source actor is done through it.
  //
  // NOTE NOTE NOTE: This also requires that if there is some (direct or periodic) damage that is
  // not allowed to generate Cinidaria damage, the spell id of such spells is listed below in the
  // "spell_blacklist" map.
  //
  void initialize( special_effect_t& effect ) override
  {
    // Vector of blacklisted spell ids that cannot proc Cinidaria.
    const std::unordered_map<unsigned, bool> spell_blacklist = {
      { 207694, true }, // Symbiote Strike (Cinidaria itself)
      { 191259, true }, // Mark of the Hidden Satyr
      { 191380, true }, // Mark of the Distant Army
      { 220893, true }, // Soul Rip (Subtlety Rogue Akaari pet)
    };

    // Health percentage threshold and damage multiplier
    const double threshold = effect.driver() -> effectN( 2 ).base_value();
    const double multiplier = effect.driver() -> effectN( 1 ).percent();

    // Damage spell
    const auto spell = new cinidaria_the_symbiote_damage_t( effect.player );

    // Install an outgoing damage assessor that triggers Cinidaria after the target damage has been
    // resolved. This ensures most of the esoteric mitigation mechanisms that the sim might have are
    // also resolved, and that state -> result_amount will hold the "final actual damage" of the
    // ability.
    effect.player -> assessor_out_damage.add( assessor::TARGET_DAMAGE + 1,
      [ spell_blacklist, threshold, multiplier, spell ]( dmg_e, action_state_t* state )
      {
        const auto source_action = state -> action;
        const auto target = state -> target;

        // Must be a reasonably valid action (a harmful spell or attack)
        if ( ! source_action -> harmful ||
             ( source_action -> type != ACTION_SPELL && source_action -> type != ACTION_ATTACK ) )
        {
          return assessor::CONTINUE;
        }

        // Must do damage
        if ( state -> result_amount == 0 )
        {
          return assessor::CONTINUE;
        }

        // Target must be equal or above threshold health%
        if ( target -> health_percentage() < threshold )
        {
          return assessor::CONTINUE;
        }

        // Spell cannot be blacklisted
        if ( spell_blacklist.find( source_action -> id ) != spell_blacklist.end() )
        {
          return assessor::CONTINUE;
        }

        if ( source_action -> sim -> debug )
        {
          source_action -> sim -> out_debug.printf( "%s cinidaria_the_symbiote proc from %s",
            spell -> player -> name(), source_action -> name() );
        }

        // All ok, trigger the damage
        spell -> target = target;
        spell -> base_dd_min = spell -> base_dd_max = state -> result_amount * multiplier;
        spell -> execute();

        return assessor::CONTINUE;
      }
    );
  }
};

// Combined legion potion action ============================================

template <typename T>
struct legion_potion_damage_t : public T
{
  legion_potion_damage_t( const special_effect_t& effect, const std::string& name_str, const spell_data_t* spell ) :
    T( name_str, effect.player, spell )
  {
    this -> background = this -> may_crit = this -> special = true;
    this -> callbacks = false;
    this -> base_dd_min = spell -> effectN( 1 ).min( this -> player );
    this -> base_dd_max = spell -> effectN( 1 ).max( this -> player );
    // Currently 0, but future proof if they decide to make it scale ..
    this -> attack_power_mod.direct = spell -> effectN( 1 ).ap_coeff();
    this -> spell_power_mod.direct = spell -> effectN( 1 ).sp_coeff();
  }
};

// Potion of the Old War ====================================================

void consumable::potion_of_the_old_war( special_effect_t& effect )
{
  // Instantiate a new action if we cannot find a suitable one already
  auto action = effect.player -> find_action( effect.name() );
  if ( ! action )
  {
    action = effect.player -> create_proc_action( effect.name(), effect );
  }

  if ( ! action )
  {
    action = new legion_potion_damage_t<melee_attack_t>( effect, effect.name(), effect.driver() );
  }

  // Explicitly disable action creation for the potion on the base special effect
  effect.disable_action();

  // Make a bog standard damage proc for the buff
  auto secondary = new special_effect_t( effect.player );
  secondary -> type = SPECIAL_EFFECT_EQUIP;
  secondary -> spell_id = effect.spell_id;
  secondary -> cooldown_ = timespan_t::zero();
  secondary -> execute_action = action;
  effect.player -> special_effects.push_back( secondary );

  auto proc = new dbc_proc_callback_t( effect.player, *secondary );
  proc -> initialize();
  proc -> deactivate();

  effect.custom_buff = buff_creator_t( effect.player, effect.name(), effect.driver() )
    .stack_change_callback( [ proc ]( buff_t*, int, int new_ ) {
      if ( new_ == 1 ) proc -> activate();
      else             proc -> deactivate();
    } )
    .cd( timespan_t::zero() ) // Handled by the action
    .chance( 1.0 ); // Override chance, the 20RPPM thing is in the secondary proc above
}

// Potion of Deadly Grace ===================================================

void consumable::potion_of_deadly_grace( special_effect_t& effect )
{
  std::string action_name = effect.driver() -> effectN( 1 ).trigger() -> name_cstr();
  util::tokenize( action_name );

  // Instantiate a new action if we cannot find a suitable one already
  auto action = effect.player -> find_action( action_name );
  if ( ! action )
  {
    action = effect.player -> create_proc_action( action_name, effect );
  }

  if ( ! action )
  {
    action = new legion_potion_damage_t<spell_t>( effect, action_name, effect.driver() -> effectN( 1 ).trigger() );
  }

  // Explicitly disable action creation for the potion on the base special effect
  effect.disable_action();

  // Make a bog standard damage proc for the buff
  auto secondary = new special_effect_t( effect.player );
  secondary -> type = SPECIAL_EFFECT_EQUIP;
  secondary -> spell_id = effect.spell_id;
  secondary -> cooldown_ = timespan_t::zero();
  secondary -> execute_action = action;
  effect.player -> special_effects.push_back( secondary );

  auto proc = new dbc_proc_callback_t( effect.player, *secondary );
  proc -> initialize();
  proc -> deactivate();

  timespan_t duration = effect.driver() -> duration();
  // Allow hunters and all spellcasters to have +5 seconds. Technically this should probably be
  // handled fancifully with movement and all, but this should be a reasonable compromise for now.
  if ( effect.player -> type == HUNTER || effect.player -> role == ROLE_SPELL )
  {
    duration += timespan_t::from_seconds( 5 );
  }

  effect.custom_buff = buff_creator_t( effect.player, effect.name(), effect.driver() )
    .stack_change_callback( [ proc ]( buff_t*, int, int new_ ) {
      if ( new_ == 1 ) proc -> activate();
      else             proc -> deactivate();
    } )
    .cd( timespan_t::zero() ) // Handled by the action
    .duration( duration )
    .chance( 1.0 ); // Override chance, the 20RPPM thing is in the secondary proc above
}

// Hearty Feast =============================================================

void consumable::hearty_feast( special_effect_t& effect )
{
  effect.stat = effect.player -> convert_hybrid_stat( STAT_STR_AGI_INT );
  switch ( effect.stat )
  {
    case STAT_STRENGTH:
      effect.trigger_spell_id = 201634;
      break;
    case STAT_AGILITY:
      effect.trigger_spell_id = 201635;
      break;
    case STAT_INTELLECT:
      effect.trigger_spell_id = 201646;
      break;
    default:
      break;
  }

  // TODO: Is this actually spec specific?
  if ( effect.player -> role == ROLE_TANK )
  {
    effect.stat = STAT_STAMINA;
    effect.trigger_spell_id = 201647;
  }
  effect.stat_amount = effect.player -> find_spell( effect.trigger_spell_id ) -> effectN( 1 ).average( effect.player );
}

// Lavish Suramar Feast =====================================================

void consumable::lavish_suramar_feast( special_effect_t& effect )
{
  effect.stat = effect.player -> convert_hybrid_stat( STAT_STR_AGI_INT );
  switch ( effect.stat )
  {
    case STAT_STRENGTH:
      effect.trigger_spell_id = 201638;
      break;
    case STAT_AGILITY:
      effect.trigger_spell_id = 201639;
      break;
    case STAT_INTELLECT:
      effect.trigger_spell_id = 201640;
      break;
    default:
      break;
  }

  // TODO: Is this actually spec specific?
  if ( effect.player -> role == ROLE_TANK )
  {
    effect.stat = STAT_STAMINA;
    effect.trigger_spell_id = 201641;
  }

  effect.stat_amount = effect.player -> find_spell( effect.trigger_spell_id ) -> effectN( 1 ).average( effect.player );
}

// Lemon Herb Filet =========================================================

void consumable::lemon_herb_filet( special_effect_t& effect )
{
  double value = effect.driver() -> effectN( 1 ).percent();

  buff_t* dmf_well_fed = buff_creator_t( effect.player, "lemon_herb_filet", effect.driver() )
    .default_value( effect.player -> race == race_e::RACE_PANDAREN ? 2 * value : value )
    .add_invalidate( CACHE_VERSATILITY );

  effect.custom_buff = dmf_well_fed;
  effect.player -> buffs.dmf_well_fed = dmf_well_fed;
}

// Pepper Breath (generic) ==================================================

struct pepper_breath_damage_t : public spell_t
{
  pepper_breath_damage_t( const special_effect_t& effect, unsigned spell_id ) :
    spell_t( "pepper_breath_damage", effect.player, effect.player -> find_spell( spell_id ) )
  {
    background = true;
    callbacks = false;
    travel_speed = 17.5; // Spelldata says 1.75, but from in-game testing it's definitely closer to 17.5
  }

  void init() override
  {
    spell_t::init();
    snapshot_flags = update_flags = 0;
    snapshot_flags |= STATE_TGT_MUL_TA | STATE_TGT_MUL_DA;
    update_flags |= STATE_TGT_MUL_TA | STATE_TGT_MUL_DA;
  }
};


struct pepper_breath_driver_t : public spell_t
{
  size_t balls_min, balls_max;

  pepper_breath_driver_t( const special_effect_t& effect, unsigned trigger_id ) :
    spell_t( "pepper_breath", effect.player, effect.trigger() ),
    balls_min( effect.trigger() -> effectN( 1 ).min( effect.player ) ),
    balls_max( effect.trigger() -> effectN( 1 ).max( effect.player ) )
  {
    assert( balls_min > 0 && balls_max > 0 );
    background = true;
    callbacks = may_crit = hasted_ticks = tick_may_crit = may_miss = false;

    // TODO: Need to look at logs
    base_tick_time = timespan_t::from_millis( 250 );
    dot_behavior = DOT_EXTEND;
    travel_speed = 0;

    tick_action = effect.player -> find_action( "pepper_breath_damage" );
    if ( ! tick_action )
    {
      tick_action = effect.player -> create_proc_action( "pepper_breath_damage", effect );
    }

    if ( ! tick_action )
    {
      tick_action = new pepper_breath_damage_t( effect, trigger_id );
    }
  }
  virtual void init() override
  {
    spell_t::init();
    snapshot_flags = update_flags = 0;
    snapshot_flags |= STATE_TGT_MUL_TA | STATE_TGT_MUL_DA;
    update_flags |= STATE_TGT_MUL_TA | STATE_TGT_MUL_DA;
  }
  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    auto n_ticks = static_cast<size_t>( rng().range( balls_min, balls_max + 1 ) );
    assert( n_ticks >= 4 && n_ticks <= 6 );
    return base_tick_time * n_ticks;
  }
};

void consumable::pepper_breath( special_effect_t& effect )
{
  unsigned trigger_id = 0;
  switch ( effect.driver() -> id() )
  {
    case 225601:
      trigger_id = 225623;
      break;
    case 225606:
      trigger_id = 225624;
      break;
    case 201336:
      trigger_id = 201573;
    default:
      break;
  }

  if ( trigger_id == 0 )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  action_t* a = effect.player -> find_action( "pepper_breath" );
  if ( ! a )
  {
    a = effect.player -> create_proc_action( "pepper_breath", effect );
  }

  if ( ! a )
  {
    a = new pepper_breath_driver_t( effect, trigger_id );
  }

  effect.execute_action = a;
  auto proc = new dbc_proc_callback_t( effect.player, effect );
  proc -> initialize();
}

// Journey Through Time =====================================================

// TODO: Run speed
void set_bonus::journey_through_time( special_effect_t& effect )
{
  // The stat buff will not be created at this point, but the special effect for the (trinket) will
  // be there. So, in an effort to confuse everyone, fiddle with that special effect and compute the
  // actual stats in this method, hardcoding them.
  auto e = unique_gear::find_special_effect( effect.player, 214120, SPECIAL_EFFECT_EQUIP );
  if ( e == nullptr )
  {
    return;
  }

  // Apply legion Combat rating penalty to the effect
  auto base_value = item_database::apply_combat_rating_multiplier( *e -> item,
    e -> trigger() -> effectN( 1 ).average( e -> item ) );
  // And presume that the extra 1k haste is not penalized as it's not strictly sourced from an item
  base_value += effect.driver() -> effectN( 2 ).average( effect.player );

  // Manually apply stats here to the trinket effect
  e -> stat = STAT_HASTE_RATING; // Really should not be hardcoded but it'll do
  e -> stat_amount = base_value;
}

// Jeweled Signet of Melandrus ==============================================

void item::jeweled_signet_of_melandrus( special_effect_t& effect )
{
  double value = 1.0 + effect.driver() -> effectN( 1 ).percent();
  effect.player -> auto_attack_multiplier *= value;
}

// Caged Horror =============================================================

void item::caged_horror( special_effect_t& effect )
{
  double amount = effect.driver() -> effectN( 1 ).average( effect.item );
  auto nuke = new unique_gear::proc_spell_t( "dark_blast", effect.player,
    effect.player -> find_spell( 215407 ), effect.item );
  // Need to manually adjust the damage and properties, since it's not available in the nuke
  nuke -> aoe = -1; // TODO: Fancier targeting, this should probably not hit all targets
  nuke -> radius = 5; // TODO: How wide is the "line" ?
  nuke -> base_dd_min = nuke -> base_dd_max = amount;

  effect.execute_action = nuke;

  new dbc_proc_callback_t( effect.player, effect );
}

// Tempered Egg of Serpentrix ===============================================

struct spawn_of_serpentrix_t : public pet_t
{
  struct magma_spit_t : public spell_t
  {
    magma_spit_t( player_t* player, double damage ) :
      spell_t( "magma_spit", player, player -> find_spell( 215754 ) )
    {
      may_crit = true;
      base_dd_min = base_dd_max = damage;
    }

    // Not really true, but lets at least make haste not work on this, until Blizzard takes a better
    // look at the whole pet.
    double composite_haste() const override
    { return 1.0; }

    void init() override
    {
      spell_t::init();

      if ( ! sim -> report_pets_separately )
      {
        player_t* owner = player -> cast_pet() -> owner;

        auto it = range::find_if( owner -> pet_list, [ this ]( pet_t* pet ) {
          return this -> player -> name_str == pet -> name_str;
        } );

        if ( it != owner -> pet_list.end() && player != *it )
        {
          stats = ( *it ) -> get_stats( name(), this );
        }
      }
    }
  };

  double damage_amount;

  spawn_of_serpentrix_t( const special_effect_t& effect ) :
    pet_t( effect.player -> sim, effect.player, "spawn_of_serpentrix", true, true ),
    damage_amount( effect.driver() -> effectN( 1 ).average( effect.item ) )
  { }

  void init_action_list() override
  {
    pet_t::init_action_list();

    if ( action_list_str.empty() )
    {
      action_priority_list_t* def = get_action_priority_list( "default" );
      def -> add_action( "magma_spit" );
    }
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "magma_spit" ) return new magma_spit_t( this, damage_amount );

    return pet_t::create_action( name, options_str );
  }
};

struct spawn_of_serpentrix_cb_t : public dbc_proc_callback_t
{
  const spell_data_t* summon;
  std::array<spawn_of_serpentrix_t*, 7> pets;

  spawn_of_serpentrix_cb_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.item, effect ),
    summon( effect.player -> find_spell( 215750 ) )
  {
    for ( size_t i = 0; i < pets.size(); ++i )
    {
      pets[ i ] = new spawn_of_serpentrix_t( effect );
    }
  }

  void execute( action_t* /* a */, action_state_t* /* state */ ) override
  {
    bool spawned = false;

    for ( size_t i = 0; i < pets.size(); ++i )
    {
      if ( pets[ i ] -> is_sleeping() )
      {
        pets[ i ] -> summon( summon -> duration() );
        spawned = true;
        break;
      }
    }

    if ( ! spawned )
    {
      listener -> sim -> errorf( "%s spawn_of_serpentrix could not spawn a pet, increase the count",
        listener -> name() );
    }
  }
};

void item::tempered_egg_of_serpentrix( special_effect_t& effect )
{
  new spawn_of_serpentrix_cb_t( effect );
}

// Six-Feather Fan ==========================================================

struct wind_bolt_callback_t : public dbc_proc_callback_t
{
  action_t* wind_bolt;

  wind_bolt_callback_t( const item_t* i, const special_effect_t& effect, action_t* a ) :
    dbc_proc_callback_t( i, effect ), wind_bolt( a )
  {}

  void execute( action_t*, action_state_t* s ) override
  {
    wind_bolt -> target = s -> target;
    effect.custom_buff -> trigger();
  }
};

void item::sixfeather_fan( special_effect_t& effect )
{
  action_t* bolt = effect.player -> find_action( "wind_bolt" );
  if ( ! bolt )
  {
    bolt = effect.player -> create_proc_action( "wind_bolt", effect );
  }

  if ( ! bolt )
  {
    // Set trigger spell so we can magically create the proc action.
    effect.trigger_spell_id = effect.trigger() -> effectN( 1 ).trigger() -> id();

    bolt = effect.initialize_offensive_spell_action();

    // Reset trigger spell ID so it does not trigger an action on proc.
    effect.trigger_spell_id = 0;
  }

  effect.custom_buff = buff_creator_t( effect.player, "sixfeather_fan", effect.trigger(), effect.item )
    .tick_callback( [ = ]( buff_t*, int, const timespan_t& ) {
      bolt -> schedule_execute();
    } )
    .tick_zero( true )
    .tick_behavior( BUFF_TICK_CLIP ); // TOCHECK

  new wind_bolt_callback_t( effect.item, effect, bolt );
}

// Aggramars Speed Boots ====================================================

void item::aggramars_stride( special_effect_t& effect )
{
  effect.custom_buff = buff_creator_t( effect.player, "aggramars_stride", effect.driver(), effect.item )
    .default_value( effect.driver() -> effectN( 1 ).percent() );

  effect.player -> buffs.aggramars_stride = effect.custom_buff;
}

// Aggramars Speed Boots ====================================================

void item::norgannons_foresight( special_effect_t& effect )
{
  effect.player -> buffs.norgannons_foresight -> default_chance = 1;
  effect.player -> buffs.norgannons_foresight_ready -> default_chance = 1;
}


// Eyasu's Mulligan =========================================================

struct eyasus_driver_t : public spell_t
{
  std::array<stat_buff_t*, 4> stat_buffs;
  std::array<buff_t*, 4> mulligan_buffs;
  unsigned current_roll;
  cooldown_t* base_cd;
  bool first_roll, used_mulligan;

  eyasus_driver_t( const special_effect_t& effect ) :
    spell_t( "eyasus_mulligan", effect.player, effect.player -> find_spell( 227389 ) ),
    current_roll( 0 ), base_cd( effect.player -> get_cooldown( effect.cooldown_name() ) ),
    first_roll( true ), used_mulligan( false )
  {
    callbacks = harmful = hasted_ticks = false;
    background = quiet = true;

    base_tick_time = data().duration();
    dot_duration = data().duration();

    double amount = item_database::apply_combat_rating_multiplier( *effect.item,
      effect.driver() -> effectN( 1 ).average( effect.item ) );

    // Initialize stat buffs
    stat_buffs[ 0 ] = stat_buff_creator_t( effect.player, "lethal_on_board",
      effect.player -> find_spell( 227390 ), effect.item )
      .add_stat( STAT_CRIT_RATING, amount );
    stat_buffs[ 1 ] = stat_buff_creator_t( effect.player, "the_coin",
      effect.player -> find_spell( 227392 ), effect.item )
      .add_stat( STAT_HASTE_RATING, amount );
    stat_buffs[ 2 ] = stat_buff_creator_t( effect.player, "top_decking",
      effect.player -> find_spell( 227393 ), effect.item )
      .add_stat( STAT_MASTERY_RATING, amount );
    stat_buffs[ 3 ] = stat_buff_creator_t( effect.player, "full_hand",
      effect.player -> find_spell( 227394 ), effect.item )
      .add_stat( STAT_VERSATILITY_RATING, amount );

    // Initialize mulligan buffs
    mulligan_buffs[ 0 ] = buff_creator_t( effect.player, "lethal_on_board_mulligan",
      effect.player -> find_spell( 227389 ), effect.item );
    mulligan_buffs[ 1 ] = buff_creator_t( effect.player, "the_coin_mulligan",
      effect.player -> find_spell( 227395 ), effect.item );
    mulligan_buffs[ 2 ] = buff_creator_t( effect.player, "top_decking_mulligan",
      effect.player -> find_spell( 227396 ), effect.item );
    mulligan_buffs[ 3 ] = buff_creator_t( effect.player, "full_hand_mulligan",
      effect.player -> find_spell( 227397 ), effect.item );
  }

  void trigger_dot( action_state_t* state ) override
  {
    if ( ! first_roll )
    {
      return;
    }

    spell_t::trigger_dot( state );
  }

  void execute() override
  {
    spell_t::execute();

    // Choose a buff and trigger the mulligan one
    range::for_each( mulligan_buffs, [ this ]( buff_t* b ) { b -> expire(); } );
    current_roll = static_cast<unsigned>( rng().range( 0, mulligan_buffs.size() ) );
    mulligan_buffs[ current_roll ] -> trigger();
    if ( ! first_roll )
    {
      used_mulligan = true;
    }

    first_roll = false;
  }

  void last_tick( dot_t* d ) override
  {
    spell_t::last_tick( d );

    mulligan_buffs[ current_roll ] -> expire();
    stat_buffs[ current_roll ] -> trigger();
    first_roll = true;
    used_mulligan = false;

    // Hardcoded .. for now, need to dig into spell data to see where this is hidden
    base_cd -> start( timespan_t::from_seconds( 120 ) );
  }

  void reset() override
  {
    spell_t::reset();

    first_roll = true;
    used_mulligan = false;
  }

  bool ready() override
  {
    if ( used_mulligan )
    {
      return false;
    }

    return spell_t::ready();
  }
};

void item::eyasus_mulligan( special_effect_t& effect )
{
  effect.execute_action = new eyasus_driver_t( effect );
}

void unique_gear::register_special_effects_x7()
{
  /* Legion 7.0 Dungeon */
  register_special_effect( 214829, item::chaos_talisman                 );
  register_special_effect( 213782, item::corrupted_starlight            );
  register_special_effect( 216085, item::elementium_bomb_squirrel       );
  register_special_effect( 214962, item::faulty_countermeasures         );
  register_special_effect( 215670, item::figurehead_of_the_naglfar      );
  register_special_effect( 214971, item::giant_ornamental_pearl         );
  register_special_effect( 215956, item::horn_of_valor                  );
  register_special_effect( 224059, item::impact_tremor                  );
  register_special_effect( 214798, item::memento_of_angerboda           );
  register_special_effect( 215648, item::moonlit_prism                  );
  register_special_effect( 215467, item::obelisk_of_the_void            );
  register_special_effect( 215857, item::portable_manacracker           );
  register_special_effect( 214584, item::shivermaws_jawbone             );
  register_special_effect( 214168, item::spiked_counterweight           );
  register_special_effect( 228450, item::stabilized_energy_pendant      );
  register_special_effect( 215630, item::stormsinger_fulmination_charge );
  register_special_effect( 215089, item::terrorbound_nexus              );
  register_special_effect( 215127, item::tiny_oozeling_in_a_jar         );
  register_special_effect( 215658, item::tirathons_betrayal             );
  register_special_effect( 214980, item::windscar_whetstone             );
  register_special_effect( 214054, "214052Trigger"                      );
  register_special_effect( 215444, item::caged_horror                   );
  register_special_effect( 215813, "ProcOn/Hit_1Tick_215816Trigger"     );
  register_special_effect( 214492, "ProcOn/Hit_1Tick_214494Trigger"     );
  register_special_effect( 214340, "ProcOn/Hit_1Tick_214342Trigger"     );
  register_special_effect( 228462, item::jeweled_signet_of_melandrus    );
  register_special_effect( 215745, item::tempered_egg_of_serpentrix     );
  register_special_effect( 228461, item::gnawed_thumb_ring              );
  register_special_effect( 215404, item::naraxas_spiked_tongue          );
  register_special_effect( 234106, item::choker_of_barbed_reins         );

  /* Legion 7.1 Dungeon */
  register_special_effect( 230257, item::arans_relaxing_ruby            );
  register_special_effect( 234142, item::ring_of_collapsing_futures     );
  register_special_effect( 230222, item::mrrgrias_favor                 );
  register_special_effect( 231952, item::toe_knees_promise              );
  register_special_effect( 230236, item::deteriorated_construct_core    );
  register_special_effect( 230150, item::eye_of_command                 );
  register_special_effect( 230011, item::bloodstained_hankerchief       );
  register_special_effect( 230101, item::majordomos_dinner_bell         );

  /* Legion 7.0 Raid */
  // register_special_effect( 221786, item::bloodthirsty_instinct  );
  register_special_effect( 222512, item::natures_call           );
  register_special_effect( 222167, item::spontaneous_appendages );
  register_special_effect( 221803, item::ravaged_seed_pod       );
  register_special_effect( 221845, item::twisting_wind          );
  register_special_effect( 222187, item::unstable_horrorslime   );
  register_special_effect( 222705, item::bough_of_corruption    );
  register_special_effect( 221767, item::ursocs_rending_paw     );
  register_special_effect( 222046, item::wriggling_sinew        );

  /* Legion 7.1.5 Raid */
  register_special_effect( 225141, item::draught_of_souls        );
  register_special_effect( 225139, item::convergence_of_fates    );
  register_special_effect( 225129, item::entwined_elemental_foci );
  register_special_effect( 225134, item::fury_of_the_burning_sky );
  register_special_effect( 225131, item::icon_of_rot             );
  register_special_effect( 225137, item::star_gate               );
  register_special_effect( 225125, item::erratic_metronome       );
  register_special_effect( 225142, item::whispers_in_the_dark    );
  register_special_effect( 225135, item::nightblooming_frond     );
  register_special_effect( 225132, item::might_of_krosus         );
  register_special_effect( 225133, item::pharameres_forbidden_grimoire );

  /* Legion 7.2.0 Dungeon */
  register_special_effect( 238498, item::dreadstone_of_endless_shadows );

  /* Legion 7.0 Misc */
  register_special_effect( 188026, item::infernal_alchemist_stone       );
  register_special_effect( 191563, item::darkmoon_deck                  );
  register_special_effect( 191611, item::darkmoon_deck                  );
  register_special_effect( 191632, item::darkmoon_deck                  );
  register_special_effect( 227868, item::sixfeather_fan                 );
  register_special_effect( 227388, item::eyasus_mulligan                );
  register_special_effect( 228141, item::marfisis_giant_censer          );
  register_special_effect( 224073, item::devilsaurs_bite                );
  register_special_effect( 231941, item::leyspark                       );

  /* Legion Enchants */
  register_special_effect( 190888, "190909trigger" );
  register_special_effect( 190889, enchants::mark_of_the_distant_army );
  register_special_effect( 190890, enchants::mark_of_the_hidden_satyr );
  register_special_effect( 228398, "228399trigger" );
  register_special_effect( 228400, "228401trigger" );

  /* T19 Generic Order Hall set bonuses */
  register_special_effect( 221533, set_bonus::passive_stat_aura     );
  register_special_effect( 221534, set_bonus::passive_stat_aura     );
  register_special_effect( 221535, set_bonus::passive_stat_aura     );

  /* 7.0 Dungeon 2 Set Bonuses */
  register_special_effect( 228445, set_bonus::march_of_the_legion );
  register_special_effect( 228447, set_bonus::journey_through_time );
  register_special_effect( 224146, set_bonus::simple_callback );
  register_special_effect( 224148, set_bonus::simple_callback );
  register_special_effect( 224150, set_bonus::simple_callback );
  register_special_effect( 228448, set_bonus::simple_callback );

  /* Legendaries */
  register_special_effect( 207692, cinidaria_the_symbiote_t() );
  register_special_effect( 207438, item::aggramars_stride );
  register_special_effect( 235991, item::kiljadens_burning_wish );
  register_special_effect( 236373, item::norgannons_foresight );

  /* Consumables */
  register_special_effect( 188028, consumable::potion_of_the_old_war );
  register_special_effect( 188027, consumable::potion_of_deadly_grace );
  register_special_effect( 201351, consumable::hearty_feast );
  register_special_effect( 201352, consumable::lavish_suramar_feast );
  register_special_effect( 225606, consumable::pepper_breath );
  register_special_effect( 225601, consumable::pepper_breath );
  register_special_effect( 201336, consumable::pepper_breath );
  register_special_effect( 185736, consumable::lemon_herb_filet );
}

void unique_gear::register_hotfixes_x7()
{
  hotfix::register_spell( "Mark of the Distant Army", "2017-01-10-4", "Set Velocity to a reasonable value.", 191380 )
    .field( "prj_speed" )
    .operation( hotfix::HOTFIX_SET )
    .modifier( 40 )
    .verification_value( 1 );
}

void unique_gear::register_target_data_initializers_x7( sim_t* sim )
{
  std::vector< slot_e > trinkets;
  trinkets.push_back( SLOT_TRINKET_1 );
  trinkets.push_back( SLOT_TRINKET_2 );

  sim -> register_target_data_initializer( spiked_counterweight_constructor_t( 136715, trinkets ) );
  sim -> register_target_data_initializer( figurehead_of_the_naglfar_constructor_t( 137329, trinkets ) );
  sim -> register_target_data_initializer( portable_manacracker_constructor_t( 137398, trinkets ) );
  sim -> register_target_data_initializer( wriggling_sinew_constructor_t( 139326, trinkets ) );
  sim -> register_target_data_initializer( bough_of_corruption_constructor_t( 139336, trinkets ) );
  sim -> register_target_data_initializer( mrrgrias_favor_constructor_t( 142160, trinkets ) ) ;
  sim -> register_target_data_initializer( fury_of_the_burning_sun_constructor_t( 140801, trinkets ) );
}
