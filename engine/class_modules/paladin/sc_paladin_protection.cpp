#include "simulationcraft.hpp"
#include "sc_paladin.hpp"
#include "class_modules/apl/apl_paladin.hpp"

// TODO :
// Defensive stuff :
// - correctly update sotr's armor bonus on each cast
// - avenger's valor defensive benefit

namespace paladin {


  namespace buffs
  {
  sentinel_buff_t::sentinel_buff_t( paladin_t* p )
    : buff_t( p, "sentinel", p->spells.sentinel ),
      damage_modifier( 0.0 ),
      healing_modifier( 0.0 ),
      crit_bonus( 0.0 ),
      damage_reduction_modifier( 0.0 ),
      health_bonus( 0.0 )
  {
    if ( !p->talents.sentinel->ok() )
    {
      set_chance( 0 );
    }
    set_refresh_behavior( buff_refresh_behavior::DISABLED );
    damage_modifier           = data().effectN( 1 ).percent();
    healing_modifier          = data().effectN( 1 ).percent();
    crit_bonus                = data().effectN( 3 ).percent();
    health_bonus              = data().effectN( 11 ).percent();
    damage_reduction_modifier = data().effectN( 12 ).percent();

    // Sentinel starts at max stacks
    set_initial_stack( max_stack() );

    disable_ticking( true );

    // let the ability handle the cooldown
    cooldown->duration = 0_ms;

    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
    add_invalidate( CACHE_MASTERY );
    add_invalidate( CACHE_STAMINA );
  }

  sentinel_decay_buff_t::sentinel_decay_buff_t( paladin_t* p ) : buff_t( p, "sentinel_decay" )
  {
    if ( !p->talents.sentinel->ok() )
    {
      set_chance( 0 );
    }
    set_refresh_behavior( buff_refresh_behavior::NONE );

    cooldown->duration = p->spells.sentinel->effectN( 14 ).time_value() *
                         ( p->talents.righteous_protector->ok()
                             ? ( 1.0 - ( std::abs( p->talents.righteous_protector->effectN( 2 ).percent() ) ) )
                             : 1.0 );

    add_invalidate( CACHE_STAMINA );
  }
  }  // namespace buffs

// Ardent Defender (Protection) ===============================================

struct ardent_defender_t : public paladin_spell_t
{
  ardent_defender_t( paladin_t* p, util::string_view options_str )
    : paladin_spell_t( "ardent_defender", p, p->find_talent_spell( talent_tree::SPECIALIZATION, "Ardent Defender" ) )
  {
    parse_options( options_str );

    harmful = false;
    use_off_gcd = true;
    trigger_gcd = 0_ms;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p()->buffs.ardent_defender->trigger();
  }
};

// Avengers Shield ==========================================================

// This struct is for all things Avenger's Shield which should occur baseline, disregarding whether it's selfcast, Divine Resonance or Divine Toll.
// Specific interactions with Selfcast, Divine Resonance and Divine Toll should be put under the other structs.
struct avengers_shield_base_t : public paladin_spell_t
{
  struct tyrs_enforcer_t : public paladin_spell_t
  {
    tyrs_enforcer_t( util::string_view n, paladin_t* p )
      : paladin_spell_t( n, p,
                         p->talents.tyrs_enforcer->effectN( 1 ).trigger() )
    {
      background = may_crit = true;
      may_miss              = false;
      base_multiplier *= 1.0 + p->talents.tyrs_enforcer->effectN( 2 ).percent();
    }
  };

  struct glory_of_the_vanguard_t : public paladin_spell_t
  {
    struct glory_of_the_vanguard_aoe_t : public paladin_spell_t
    {
      glory_of_the_vanguard_aoe_t(util::string_view n, paladin_t* p)
        : paladin_spell_t(n, p, p->spells.glory_of_the_vanguard)
      {
        background             = true;
        aoe                    = -1;
        target_filter_callback = secondary_targets_only();
        base_multiplier        = .5;  // Doesn't seem to be in spell data
      }

    };
    glory_of_the_vanguard_aoe_t* gotv_aoe;
    glory_of_the_vanguard_t( util::string_view n, paladin_t* p )
      : paladin_spell_t( n, p, p->spells.glory_of_the_vanguard )
    {
      background = true;
      // Glory of the Vanguard hits every enemy in a line. For now, just assume it hits everything
      // Theoretically, it also has a chance to miss completely, for whatever reasons. Drunk Paladins.
      if ( p->is_ptr() )
      {
        aoe      = 1;
        gotv_aoe = new glory_of_the_vanguard_aoe_t( std::string( n ) + "_aoe", p );
        add_child( gotv_aoe );
      }
      else
      {
        aoe = -1;
      }
    }
    void execute() override
    {
      paladin_spell_t::execute();
      if ( p()->talents.glory_of_the_vanguard_2->ok() )
      {
        p()->resource_gain( RESOURCE_HOLY_POWER, p()->talents.glory_of_the_vanguard_2->effectN( 2 ).base_value(),
                            p()->gains.hp_glory_of_the_vanguard_2 );
      }
      if ( p()->talents.glory_of_the_vanguard_3->ok() )
        p()->buffs.valor->trigger();
    }
    void impact(action_state_t* s) override
    {
      paladin_spell_t::impact( s );
      if ( p()->is_ptr() )
        gotv_aoe->execute_on_target( s->target, s->result_amount );
    }
  };

  struct refining_fire_dot_t : public residual_action::residual_periodic_action_t<paladin_spell_t>
  {
    using base_t = residual_action::residual_periodic_action_t<paladin_spell_t>;
    refining_fire_dot_t( paladin_t* p ) : base_t( "refining_fire", p, p->spells.refining_fire_tick )
    {
      may_miss = may_crit = false;
      dual                = true;
      proc                = true;
      ap_type             = attack_power_type::NO_WEAPON;
    }

    void init() override
    {
      base_t::init();
      // disable the snapshot_flags for all multipliers
      snapshot_flags = update_flags = 0;
    }
  };

  tyrs_enforcer_t* tyrs_enforcer;
  refining_fire_dot_t* refining_fire_dot;
  consecration_tick_t* consecration_tick;
  glory_of_the_vanguard_t* glory_of_the_vanguard;
  bool triggers_apex;
  avengers_shield_base_t( util::string_view n, paladin_t* p, util::string_view options_str, double mul = 1.0 )
    : paladin_spell_t( n, p, p->find_talent_spell( talent_tree::SPECIALIZATION, "Avenger's Shield" ) ),
      tyrs_enforcer( nullptr ),
      refining_fire_dot( nullptr ),
      consecration_tick( nullptr ),
      glory_of_the_vanguard( nullptr ),
      triggers_apex( true )
  {
    parse_options( options_str );
    if ( !p->has_shield_equipped() )
    {
      sim->errorf( "%s: %s only usable with shield equipped in offhand\n", p->name(), name() );
      background = true;
    }
    may_crit = true;
    base_multiplier *= mul;

    std::string fullname = std::string( n );
    std::string abbrev   = fullname != "avengers_shield" ? fullname.substr( fullname.size() - 3 ) : "";

    if ( p->talents.tyrs_enforcer->ok() )
    {
      tyrs_enforcer = new tyrs_enforcer_t( "tyrs_enforcer_as" + abbrev, p );
      add_child( tyrs_enforcer );
    }
    if ( p->talents.refining_fire->ok() )
    {
      refining_fire_dot = new refining_fire_dot_t( p );
    }
    if ( p->talents.searing_sunlight->ok() )
    {
      consecration_tick = new consecration_tick_t( "ss_as" + abbrev, p );
      add_child( consecration_tick );
    }
    if (p->talents.glory_of_the_vanguard_1->ok())
    {
      glory_of_the_vanguard = new glory_of_the_vanguard_t( "glory_of_the_vanguard_as" + abbrev, p );
      add_child( glory_of_the_vanguard );
    }
  }

  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );
    if ( p()->talents.tyrs_enforcer->ok() )
    {
      tyrs_enforcer->execute_on_target( s->target );
    }

    //Bulwark of Order absorb shield. Amount is additive per hit.
    if ( p()->talents.bulwark_of_order->ok() )
    {
      double max_absorb = p()->talents.bulwark_of_order->effectN( 2 ).percent() * p()->resources.max[ RESOURCE_HEALTH ];
      double new_absorb = s->result_amount * p()->talents.bulwark_of_order->effectN( 1 ).percent();
      p()->buffs.bulwark_of_order_absorb->trigger(
          1, std::min( p()->buffs.bulwark_of_order_absorb->value() + new_absorb, max_absorb ) );
    }

    if ( p()->talents.bulwark_of_righteous_fury->ok() )
      p()->buffs.bulwark_of_righteous_fury->trigger();

    if ( p() ->talents.gift_of_the_golden_valkyr->ok())
    {
      p()->cooldowns.guardian_of_ancient_kings->adjust(
          timespan_t::from_millis( -1 ) * p()->talents.gift_of_the_golden_valkyr->effectN( 1 ).base_value() );
    }
    if ( p()->talents.crusaders_resolve->ok() )
    {
      td( s->target )->debuff.crusaders_resolve->trigger();
    }

    if ( p()->talents.refining_fire->ok() )
    {
      double damage = s-> result_amount;
      damage *= p()->talents.refining_fire->effectN( 1 ).percent();
      residual_action::trigger( refining_fire_dot, s->target, damage );
    }

    // Technically this should be in execute(), but we only know on impact if Avenger's Shield critted.
    if ( s->chain_target == 0 )
    {
      if ( p()->is_ptr() )
        make_event<delayed_execute_on_target_event_t>(
            *sim, p(), glory_of_the_vanguard, s->target,
            s->result_amount * p()->talents.glory_of_the_vanguard_1->effectN( 1 ).percent(), 300_ms );
      else
        make_event<delayed_execute_event_t>( *sim, p(), glory_of_the_vanguard, s->target, 300_ms );
    }
  }

  double action_multiplier() const override
  {
    double m = paladin_spell_t::action_multiplier();
    if ( p()->talents.focused_enmity->ok() )
    {
      if ( paladin_spell_t::num_targets() == 1 )
        m *= 1.0 + p()->talents.focused_enmity->effectN( 1 ).percent();
    }
    return m;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = paladin_spell_t::composite_da_multiplier( state );
    if ( state->chain_target == 0 )
    {
      m *= 1.0 + p()->talents.ferren_marcuss_fervor->effectN( 1 ).percent();
    }
    return m;
  }

  void execute() override
  {
    paladin_spell_t::execute();

     if ( p()->talents.strength_in_adversity->ok() )
    {
      // Buff overwrites previous buff, even if it was stronger
      p()->buffs.strength_in_adversity->expire();
      p()->buffs.strength_in_adversity->trigger( execute_state->n_targets );
    }
    if ( p()->talents.searing_sunlight->ok() && p()->all_active_consecrations.size() > 0 )
    {
      consecration_tick->execute_on_target( target );
    }
    bool isApex3 = p()->wings_up() && p()->talents.glory_of_the_vanguard_3->ok();
    if ( triggers_apex && ( ( p()->talents.glory_of_the_vanguard_1->ok() && p()->buffs.vanguard->up() ) || isApex3 ) )
    {
      if (!isApex3)
        p()->buffs.vanguard->decrement();
    }
  }
};

// This struct is solely for all Avenger's Shields which are cast by Divine Toll.
struct avengers_shield_dt_t : public avengers_shield_base_t
{
  hammer_and_anvil_t* haa;
  avengers_shield_dt_t( paladin_t* p ) : avengers_shield_base_t( "avengers_shield_dt", p, "" ), haa( nullptr )
  {
    background = true;
  }
  void execute() override
  {
    avengers_shield_base_t::execute();

    // Gain 1 Holy Power for each target hit (Protection only) - Not sure if Effect 5 with a value of 1 belongs to this
    p()->resource_gain( RESOURCE_HOLY_POWER, as<int>( p()->talents.divine_toll->effectN( 5 ).base_value() ),
                        p()->gains.hp_divine_toll );
  }
};

// This struct is solely for all Avenger's Shields which are cast by Divine Resonance.
struct avengers_shield_dr_t : public avengers_shield_base_t
{
  avengers_shield_dr_t( paladin_t* p ):
    avengers_shield_base_t( "avengers_shield_dr", p, "" )
  {
    background = true;
    triggers_apex = false;
  }
};

// This struct is solely for all Avenger's Shield which are self cast.
struct avengers_shield_t : public avengers_shield_base_t
{
  avengers_shield_t( paladin_t* p, util::string_view options_str ) :
    avengers_shield_base_t( "avengers_shield", p, options_str )
  {
    cooldown = p->cooldowns.avengers_shield;
  }
};

struct avengers_shield_divine_exaction_t :public avengers_shield_base_t
{
  avengers_shield_divine_exaction_t(paladin_t* p)
    : avengers_shield_base_t( "avengers_shield_de", p, "",
                              p->talents.templar.divine_exaction->effectN( 2 ).percent() )
  {
    background = true;
    if ( p->is_ptr() )
      base_multiplier = 1.5;  // Not sure where this comes from
    else
      base_multiplier += 1.0;
  }
};

struct blessed_hammer_data_t
{
  double blessed_assurance_mult;
};

struct blessed_hammer_t : public paladin_spell_t
{
  using state_t = paladin_action_state_t<blessed_hammer_data_t>;
  // Blessed Hammer (Protection) ================================================
  struct blessed_hammer_tick_t : public paladin_spell_t
  {
    unrelenting_edict_t* ue;
    blessed_hammer_tick_t( paladin_t* p ) : paladin_spell_t( "blessed_hammer_tick", p, p->find_spell( 204301 ) )
    {
      aoe        = -1;
      background = dual = direct_tick = true;
      callbacks                       = false;
      radius                          = 9.0;  // Guess, must be > 8 (cons) but < 10 (HoJ)
      may_crit                        = true;
      if ( p->sets->has_set_bonus( PALADIN_PROTECTION, MID2, B4 ) )
      {
        ue = new unrelenting_edict_t( p, "blessed_hammer" );
        add_child( ue );
      }
    }
    action_state_t* new_state() override
    {
      return new state_t( this, target );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      auto da = paladin_spell_t::composite_da_multiplier( s );
      auto s_ = static_cast<const state_t*>( s );

      da *= 1.0 + s_->blessed_assurance_mult;
      return da;
    }

    void impact( action_state_t* s ) override
    {
      paladin_spell_t::impact( s );
      // apply BH debuff to target_data structure
      // Does not interact with vers.
      // To Do: Investigate refresh behaviour
      td( s->target )
          ->debuff.blessed_hammer->trigger( 1, s->attack_power * p()->talents.blessed_hammer->effectN( 1 ).percent() );
      if ( p()->is_ptr() && p()->talents.seal_of_reprisal->ok() )
        p()->get_target_data( s->target )->debuff.seal_of_reprisal->execute();
      if ( p()->sets->has_set_bonus( PALADIN_PROTECTION, MID2, B4 ) )
        ue->do_execute( s );
    }
  };

  blessed_hammer_tick_t* hammer;
  double num_strikes;

  blessed_hammer_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "blessed_hammer", p, p->talents.blessed_hammer ),
    hammer( new blessed_hammer_tick_t( p ) ), num_strikes( 2 )
  {
    parse_options( options_str );
    if ( p->options.blessed_hammer_strikes )
      num_strikes = p->options.blessed_hammer_strikes;

    // Sanity check for num_strikes
    if ( num_strikes <= 0 || num_strikes > 10)
    {
      num_strikes = 2;
      sim->error( "{} invalid blessed_hammer strikes, value changed to 2", p->name() );
    }

    dot_duration = 0_ms; // The periodic event is handled by ground_aoe_event_t
    base_tick_time = 0_ms;

    may_miss = false;
    cooldown->hasted = true;

    tick_may_crit = true;

    add_child( hammer );

    triggers_higher_calling = true;
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  void snapshot_state(action_state_t* s, result_amount_type rt) override
  {
    paladin_spell_t::snapshot_state( s, rt );

    auto s_ = static_cast<state_t*>( s );

    s_->blessed_assurance_mult = p()->buffs.lightsmith.blessed_assurance->stack_value();
  }

  void execute() override
  {
    paladin_spell_t::execute();
    // Grand Crusader can proc on cast, but not on impact
    p()->trigger_grand_crusader();
    if ( p()->is_ptr() )
    {
      if ( p()->buffs.lightsmith.masterwork_weapon->up() )
      {
        p()->buffs.lightsmith.masterwork_weapon->decrement();
        p()->cast_lesser_armament( 1, LESSER_WEAPON );
      }
      if ( p()->buffs.lightsmith.masterwork_bulwark->up() )
      {
        p()->buffs.lightsmith.masterwork_bulwark->decrement();
        p()->cast_lesser_armament( 1, LESSER_BULWARK );
      }
    }
  }
  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );
    auto state = hammer->get_state();
    state->copy_state( s );
    timespan_t initial_delay = num_strikes < 3 ? data().duration() * 0.25 : 0_ms;
    // Let strikes be a decimal rather than int, and roll a random number to decide
    // hits each time.
    int roll_strikes = static_cast<int>( floor( num_strikes ) );
    if ( num_strikes - roll_strikes != 0 && rng().roll( num_strikes - roll_strikes ) )
      roll_strikes += 1;
    if ( roll_strikes > 0 )
    {
      make_event<ground_aoe_event_t>( *sim, p(),
                                      ground_aoe_params_t()
                                          .target( execute_state->target )
                                          // spawn at feet of player
                                          .x( execute_state->action->player->x_position )
                                          .y( execute_state->action->player->y_position )
                                          .pulse_time( data().duration() / roll_strikes )
                                          .n_pulses( roll_strikes )
                                          .start_time( sim->current_time() + initial_delay )
                                          .action( hammer ),
                                      state, true );
    }
    p()->buffs.lightsmith.blessed_assurance->expire();
  }
};

// Blessing of Spellwarding =====================================================

struct blessing_of_spellwarding_t : public paladin_spell_t
{
  blessing_of_spellwarding_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "blessing_of_spellwarding", p, p->talents.blessing_of_spellwarding )
  {
    parse_options( options_str );
    harmful = false;
    may_miss = false;
    cooldown = p->cooldowns.blessing_of_spellwarding; // Needed for shared cooldown with Blessing of Protection
  }

  void execute() override
  {
    paladin_spell_t::execute();

    // TODO: Check if target is self, because it's castable on anyone
    p()->buffs.blessing_of_spellwarding->trigger();

    p()->cooldowns.blessing_of_protection->start(); // Shared cooldown
    // apply forbearance, track locally for forbearant faithful & force recharge recalculation
    p()->trigger_forbearance( execute_state->target );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( candidate_target->debuffs.forbearance->check() )
      return false;

    if ( candidate_target->is_enemy() )
      return false;

    return paladin_spell_t::target_ready( candidate_target );
  }
};

// Guardian of Ancient Kings ============================================

struct guardian_of_ancient_kings_t : public paladin_spell_t
{
  guardian_of_ancient_kings_t( paladin_t* p, util::string_view options_str )
    : paladin_spell_t( "guardian_of_ancient_kings", p,
                       p->find_talent_spell( talent_tree::SPECIALIZATION, "Guardian of Ancient Kings" ) )
  {
    parse_options( options_str );
    use_off_gcd = true;
    trigger_gcd = 0_ms;
    cooldown = p->cooldowns.guardian_of_ancient_kings;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p()->buffs.guardian_of_ancient_kings->trigger();
  }
};

// Hammer of the Righteous ==================================================

struct hammer_of_the_righteous_data_t
{
  double blessed_assurance_mult;
};

struct hammer_of_the_righteous_t : public paladin_melee_attack_t
{
  using state_t = paladin_action_state_t<hammer_of_the_righteous_data_t>;
  struct hammer_of_the_righteous_aoe_t : public paladin_melee_attack_t
  {
    unrelenting_edict_t* ue;
    hammer_of_the_righteous_aoe_t( paladin_t* p )
      : paladin_melee_attack_t( "hammer_of_the_righteous_aoe", p, p->find_spell( 88263 ) )
    {
      // AoE effect always hits if single-target attack succeeds
      // Doesn't proc Grand Crusader
      may_dodge = may_parry = may_miss = false;
      background                       = true;
      aoe                              = -1;
      trigger_gcd                      = 0_ms;  // doesn't incur GCD (HotR does that already)
      target_filter_callback           = secondary_targets_only();
      if ( p->sets->has_set_bonus( PALADIN_PROTECTION, MID2, B4 ) )
      {
        ue = new unrelenting_edict_t( p, "hammer_of_the_righteous_aoe" );
        add_child( ue );
      }
    }

    action_state_t* new_state() override
    {
      return new state_t( this, target );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      auto da = paladin_melee_attack_t::composite_da_multiplier( s );
      auto s_ = static_cast<const state_t*>( s );

      da *= 1.0 + s_->blessed_assurance_mult;
      return da;
    }
    void impact(action_state_t* s) override
    {
      paladin_melee_attack_t::impact( s );
      if ( p()->sets->has_set_bonus( PALADIN_PROTECTION, MID2, B4 ) )
        ue->do_execute( s );
      if ( p()->is_ptr() && p()->talents.seal_of_reprisal->ok() )
        p()->get_target_data( s->target )->debuff.seal_of_reprisal->execute();
    }
  };

  hammer_of_the_righteous_aoe_t* hotr_aoe;
  unrelenting_edict_t* ue;
  hammer_of_the_righteous_t( paladin_t* p, util::string_view options_str )
      : paladin_melee_attack_t( "hammer_of_the_righteous", p, p->find_talent_spell( talent_tree::SPECIALIZATION, "Hammer of the Righteous" ) )
  {
    parse_options( options_str );

    if ( p->talents.blessed_hammer->ok() )
      background = true;

    hotr_aoe = new hammer_of_the_righteous_aoe_t( p );
    // Attach AoE proc as a child
    add_child( hotr_aoe );
    // 2022-11-09 Old HotR Rank 2 doesn't seem to exist anymore. New talent only has 1 charge, but it has 2 charges.
    cooldown->charges = 2;
    cooldown->hasted        = true;
    triggers_higher_calling = true;
    if ( p->sets->has_set_bonus( PALADIN_PROTECTION, MID2, B4 ) )
    {
      ue = new unrelenting_edict_t( p, "hammer_of_the_righteous" );
      add_child( ue );
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    auto da = paladin_melee_attack_t::composite_da_multiplier( s );
    da *= 1.0 + p()->buffs.lightsmith.blessed_assurance->stack_value();
    return da;
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();

    // Special things that happen when HotR connects
    if ( result_is_hit( execute_state->result ) )
    {
      // Grand Crusader
      p()->trigger_grand_crusader();

      if ( hotr_aoe->target != execute_state->target )
        hotr_aoe->target_cache.is_valid = false;
    }
    if ( p()->is_ptr() )
    {
      if ( p()->buffs.lightsmith.masterwork_weapon->up() )
      {
        p()->buffs.lightsmith.masterwork_weapon->decrement();
        p()->cast_lesser_armament( 1, LESSER_WEAPON );
      }
      if ( p()->buffs.lightsmith.masterwork_bulwark->up() )
      {
        p()->buffs.lightsmith.masterwork_bulwark->decrement();
        p()->cast_lesser_armament( 1, LESSER_BULWARK );
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    paladin_melee_attack_t::impact( s );
    if ( p()->standing_in_consecration() )
    {
      auto state = hotr_aoe->get_state();
      state->copy_state( s );
      hotr_aoe->snapshot_state( state, hotr_aoe->amount_type( state ) );
      hotr_aoe->target = s->target;
      hotr_aoe->schedule_execute( state );
    }
    p()->buffs.lightsmith.blessed_assurance->expire();
    if ( p()->sets->has_set_bonus( PALADIN_PROTECTION, MID2, B4 ) )
      ue->do_execute( s );
    if ( p()->is_ptr() && p()->talents.seal_of_reprisal->ok() )
      p()->get_target_data( s->target )->debuff.seal_of_reprisal->execute();
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    paladin_melee_attack_t::snapshot_state( s, rt );

    auto s_ = static_cast<state_t*>( s );

    s_->blessed_assurance_mult = p()->buffs.lightsmith.blessed_assurance->stack_value();
  }
};

// Judgment - Protection =================================================================

struct judgment_prot_t : public judgment_t
{
  int judge_holy_power, sw_holy_power;
  hammer_and_anvil_t* hammer_and_anvil;
  judgment_prot_t( paladin_t* p, util::string_view name, util::string_view options_str )
    : judgment_t( p, name, options_str ),
      judge_holy_power( as<int>( p->find_spell( 220637 )->effectN( 1 ).base_value() ) ),
      sw_holy_power( as<int>( p->talents.sanctified_wrath->effectN( 3 ).base_value() ) ),
      hammer_and_anvil( nullptr )
  {
  }

  void execute() override
  {
    judgment_t::execute();
  }

  // Special things that happen when Judgment damages target
  void impact( action_state_t* s ) override
  {
    judgment_t::impact( s );
  }
};

// Sentinel
struct sentinel_t : public paladin_spell_t
{
  sentinel_t( paladin_t* p, util::string_view options_str ) : paladin_spell_t( "sentinel", p, p->spells.sentinel)
  {
    parse_options( options_str );

    if ( !( p->talents.sentinel->ok() ) )
      background = true;

    harmful = false;
    dot_duration   = 0_ms;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    if ( p()->buffs.sentinel->up() )
      p()->buffs.sentinel->expire();
    if ( p()->buffs.sentinel_decay->up() )
      p()->buffs.sentinel_decay->expire();

    p()->buffs.sentinel->trigger();
    p()->adjust_health_percent();

    // Those 15 seconds may be the total stack count, but I won't risk it.
    // First expire is after buff length minus 15 seconds, but at least 1 second (E.g., Retribution Aura-procced Sentinel decays instantly)
    timespan_t firstExpireDuration = std::max(p()->buffs.sentinel->buff_duration() - timespan_t::from_seconds(15), timespan_t::from_seconds(1));
    p()->buffs.sentinel_decay->trigger( firstExpireDuration );

    if ( p()->talents.lightsmith.blessing_of_the_forge->ok() )
      p()->buffs.lightsmith.blessing_of_the_forge->execute();

    p()->buffs.hammer_of_wrath->trigger();
  }
};

void buffs::sentinel_buff_t::expire_override( int expiration_stacks, timespan_t remaining_duration )
{
  buff_t::expire_override( expiration_stacks, remaining_duration );

  auto* p = debug_cast<paladin_t*>( player );
  if (p-> buffs.sentinel_decay->up())
  {
    p->buffs.sentinel_decay->expire();
  }
  p->adjust_health_percent();
  p->buffs.hammer_of_wrath->expire();
}


void buffs::sentinel_decay_buff_t::expire_override( int expiration_stacks, timespan_t remaining_duration )
{
  buff_t::expire_override( expiration_stacks, remaining_duration );
  auto* p = static_cast<paladin_t*>( player );
  if ( p->buffs.sentinel->up() )
  {
    p->buffs.sentinel->decrement();
    if ( p->buffs.sentinel->current_stack == 0 )
    {
      p->buffs.sentinel->expire();
    }
    else
    {
      p->buffs.sentinel_decay->trigger(timespan_t::from_seconds(1));
    }
    p->adjust_health_percent();
  }
}

// paladin_t::target_mitigation ===============================================

void paladin_t::target_mitigation( school_e school, result_amount_type dt, action_state_t* s )
{
  player_t::target_mitigation( school, dt, s );

  // Mastery 'block' of periodic damage (absorbed)
  if ( s->block_result == BLOCK_RESULT_BLOCKED && dt == result_amount_type::DMG_OVER_TIME )
  {
    auto block_value = s->target_block_value;
    auto block_resist = util::calculate_armor_resist( block_value, s->action->player->current.armor_coeff );
    auto block_amount = s->result_amount * block_resist;

    // update the relevant counters
    iteration_absorb_taken += block_amount;
    s->self_absorb_amount += block_amount;
    s->result_amount -= block_amount;
    s->result_absorbed = s->result_amount;

    if ( sim->debug )
    {
      sim->print_debug( "{} Divine Bulwark absorbs {} damage from block on DOT {}.", *this, block_amount, *s->action );
    }
  }

  // Blessed Hammer
  if ( talents.blessed_hammer->ok() && s->action )
  {
    buff_t* b = get_target_data( s->action->player )->debuff.blessed_hammer;

    // BH now reduces all damage; not just melees.
    if ( b->up() )
    {
      // Absorb value collected from (de)buff value
      // Calculate actual amount absorbed.
      double amount_absorbed = std::min( s->result_amount, b->value() );
      b->expire();

      sim->print_debug( "{} Blessed Hammer absorbs {} out of {} damage",
        name(),
        amount_absorbed,
        s->result_amount
      );

      // update the relevant counters
      iteration_absorb_taken += amount_absorbed;
      s->self_absorb_amount += amount_absorbed;
      s->result_amount -= amount_absorbed;
      s->result_absorbed = s->result_amount;

      // hack to register this on the abilities table
      buffs.blessed_hammer_absorb->trigger( 1, amount_absorbed );
      buffs.blessed_hammer_absorb->consume( amount_absorbed );
    }
  }

  // Ardent Defender
  if ( buffs.ardent_defender->check() )
  {
    if ( s->result_amount > 0 && s->result_amount >= resources.current[ RESOURCE_HEALTH ] )
    {
      // Ardent defender is a little odd - it doesn't heal you *for* 20%, it heals you *to* 12%.
      // It does this by either absorbing all damage and healing you for the difference between 20% and your current health (if current < 20%)
      // or absorbing any damage that would take you below 20% (if current > 20%).
      // To avoid complications with absorb modeling, we're just going to kludge it by adjusting the amount gained or lost accordingly.
      s->result_amount = 0.0;
      double AD_health_threshold = resources.max[ RESOURCE_HEALTH ] * buffs.ardent_defender->data().effectN( 2 ).percent();
      if ( resources.current[ RESOURCE_HEALTH ] >= AD_health_threshold )
      {
        resource_loss( RESOURCE_HEALTH,
                       resources.current[ RESOURCE_HEALTH ] - AD_health_threshold,
                       nullptr,
                       s->action );
      }
      else
      {
        // Ardent Defender, like most cheat death effects, is capped at a 200% max health overkill situation
        resource_gain( RESOURCE_HEALTH,
                       std::min( AD_health_threshold - resources.current[ RESOURCE_HEALTH ], 2 * resources.max[ RESOURCE_HEALTH ] ),
                       nullptr,
                       s->action );
      }
      buffs.ardent_defender->expire();
    }

    if ( sim->debug && s->action && ! s->target->is_enemy() && ! s->target->is_add() )
      sim->print_debug( "Damage to {} after Ardent Defender (death-save) is {}", s->target->name(), s->result_amount );
  }
}

block_result_e paladin_t::target_block_resolution( const action_state_t* s ) const
{
  double block_chance = 0.0;

  // Normal block for direct physical attacks
  if ( s->result_type == result_amount_type::DMG_DIRECT && s->action->get_school() == SCHOOL_PHYSICAL &&
       s->action->type == ACTION_ATTACK )
  {
    block_chance = cache.block();
  }
  // Spell block
  else if ( s->action->get_school() != SCHOOL_PHYSICAL && s->action->type == ACTION_SPELL )
  {
    block_chance = cache.mastery() * mastery.divine_bulwark_2->effectN( 1 ).mastery_value();
  }

  if ( rng().roll( block_chance ) )
    return BLOCK_RESULT_BLOCKED;
  else
    return BLOCK_RESULT_UNBLOCKED;
}

void paladin_t::trigger_grand_crusader( grand_crusader_source /* source */ )
{
  // escape if we don't have Grand Crusader
  if ( ! talents.grand_crusader->ok() )
    return;

  double gc_proc_chance = talents.grand_crusader->effectN( 1 ).percent();

  // The bonus from First Avenger is added after Inspiring Vanguard
  bool success = rng().roll( gc_proc_chance );
  if ( ! success )
    return;

  // reset AS cooldown and count procs
  if ( ! cooldowns.avengers_shield->is_ready() )
  {
    procs.as_grand_crusader->occur();
    cooldowns.avengers_shield->reset( true );
  }
  else
    procs.as_grand_crusader_wasted->occur();

  if ( cooldowns.judgment != nullptr && talents.crusaders_judgment->ok() && cooldowns.judgment->current_charge < cooldowns.judgment->charges )
  {
    cooldowns.judgment->adjust( -( talents.crusaders_judgment->effectN( 2 ).time_value() ), true );
  }
}

void paladin_t::adjust_health_percent( )
{
  double oh             = resources.current[ RESOURCE_HEALTH ];
  double omh            = resources.max[ RESOURCE_HEALTH ];
  double currentPercent = oh / omh;
  recalculate_resource_max( RESOURCE_HEALTH );
  resources.current[ RESOURCE_HEALTH ] = currentPercent * resources.max[ RESOURCE_HEALTH ];
}



// Initialization
void paladin_t::create_prot_actions()
{
  active.divine_toll = new avengers_shield_dt_t( this );
  active.divine_resonance      = new avengers_shield_dr_t( this );
  active.divine_exaction_prot  = new avengers_shield_divine_exaction_t( this );
}

action_t* paladin_t::create_action_protection( util::string_view name, util::string_view options_str )
{
  if ( name == "ardent_defender"           ) return new ardent_defender_t          ( this, options_str );
  if ( name == "sentinel"                  ) return new sentinel_t                 ( this, options_str );
  if ( name == "avengers_shield"           ) return new avengers_shield_t          ( this, options_str );
  if ( name == "blessed_hammer"            ) return new blessed_hammer_t           ( this, options_str );
  if ( name == "blessing_of_spellwarding"  ) return new blessing_of_spellwarding_t ( this, options_str );
  if ( name == "guardian_of_ancient_kings" ) return new guardian_of_ancient_kings_t( this, options_str );
  if ( name == "hammer_of_the_righteous"   ) return new hammer_of_the_righteous_t  ( this, options_str );
  if ( name == "avenging_wrath" && talents.sentinel->ok() ) // Normal wings in base function
    return new sentinel_t( this, options_str );
  return nullptr;
}

void paladin_t::create_buffs_protection()
{
  buffs.ardent_defender = make_buff( this, "ardent_defender", find_spell( 31850 ) )
        ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN )
        ->set_cooldown( 0_ms );  // handled by the ability
  buffs.guardian_of_ancient_kings = make_buff( this, "guardian_of_ancient_kings", find_spell( 86659 ) )
        ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN )
        ->set_cooldown( 0_ms );
//HS and BH fake absorbs
  buffs.divine_bulwark_absorb = make_buff<absorb_buff_t>( this, "divine_bulwark", mastery.divine_bulwark );
  buffs.divine_bulwark_absorb->set_absorb_school( SCHOOL_MAGIC )
        ->set_absorb_source( get_stats( "divine_bulwark_absorb" ) )
        ->set_absorb_gain( get_gain( "divine_bulwark_absorb" ) );
  buffs.blessed_hammer_absorb = make_buff<absorb_buff_t>( this, "blessed_hammer_absorb", find_spell( 204301 ) );
  buffs.blessed_hammer_absorb->set_absorb_source( get_stats( "blessed_hammer_absorb" ) )
        ->set_absorb_gain( get_gain( "blessed_hammer_absorb" ) );
  buffs.bulwark_of_order_absorb = make_buff<absorb_buff_t>( this, "bulwark_of_order", find_spell( 209389 ) )
        ->set_absorb_source( get_stats( "bulwark_of_order_absorb" ) );
  buffs.redoubt                 = make_buff( this, "redoubt", find_spell( 280373 ) );
  buffs.shield_of_the_righteous = new shield_of_the_righteous_buff_t( this );
  buffs.bulwark_of_righteous_fury = make_buff( this, "bulwark_of_righteous_fury", find_spell( 386652 ) )
                                        ->set_default_value( find_spell( 386652 )->effectN( 1 ).percent() );
  buffs.shining_light_stacks = make_buff( this, "shining_light_stacks", find_spell( 182104 ) )
  // Kind of lazy way to make sure that SL only triggers for prot. That spelldata doesn't have to be used anywhere else so /shrug
    ->set_trigger_spell( find_specialization_spell( "Shining Light" ) )
  // Chance was 0% for whatever reasons, max stacks were also 5. Set to 2, because it changes to free when ShoR is used at 2 stacks
                                   ->set_chance(1)
                                   ->set_max_stack(2);
  buffs.shining_light_free = make_buff( this, "shining_light_free", find_spell( 327510 ) );
  buffs.sentinel = new buffs::sentinel_buff_t( this );
  buffs.sentinel_decay = new buffs::sentinel_decay_buff_t( this );
}

void paladin_t::init_spells_protection()
{
  // Talents
//0
  talents.avengers_shield                = find_talent_spell( talent_tree::SPECIALIZATION, "Avenger's Shield" );

  talents.shining_light                  = find_talent_spell( talent_tree::SPECIALIZATION, "Shining Light" );
  talents.hammer_of_the_righteous        = find_talent_spell( talent_tree::SPECIALIZATION, "Hammer of the Righteous" );
  talents.blessed_hammer                 = find_talent_spell( talent_tree::SPECIALIZATION, "Blessed Hammer" );

  talents.imbued_shield                  = find_talent_spell( talent_tree::SPECIALIZATION, "Imbued Shield" );
  talents.redoubt                        = find_talent_spell( talent_tree::SPECIALIZATION, "Redoubt" );
  talents.grand_crusader                 = find_talent_spell( talent_tree::SPECIALIZATION, "Grand Crusader" );
  talents.seal_of_charity                = find_talent_spell( talent_tree::SPECIALIZATION, "Seal of Charity" );


  talents.refining_fire                  = find_talent_spell( talent_tree::SPECIALIZATION, "Refining Fire" );
  talents.valiant_crusade                = find_talent_spell( talent_tree::SPECIALIZATION, "Valiant Crusade" );
  talents.ardent_defender                = find_talent_spell( talent_tree::SPECIALIZATION, "Ardent Defender" );
  talents.searing_sunlight               = find_talent_spell( talent_tree::SPECIALIZATION, "Searing Sunlight" );
  talents.solace                         = find_talent_spell( talent_tree::SPECIALIZATION, "Solace" );

  //8
  talents.undying_embers                 = find_talent_spell( talent_tree::SPECIALIZATION, "Undying Embers" );
  talents.bulwark_of_order               = find_talent_spell( talent_tree::SPECIALIZATION, "Bulwark of Order" );
  talents.improved_ardent_defender       = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Ardent Defender" );
  talents.blessing_of_spellwarding       = find_talent_spell( talent_tree::SPECIALIZATION, "Blessing of Spellwarding" );
  talents.light_of_the_titans            = find_talent_spell( talent_tree::SPECIALIZATION, "Light of the Titans" );
  talents.tirions_devotion               = find_talent_spell( talent_tree::SPECIALIZATION, "Tirion's Devotion" );
  talents.vision_of_sanctity             = find_talent_spell( talent_tree::SPECIALIZATION, "Vision of Sanctity" );

  talents.tyrs_enforcer                  = find_talent_spell( talent_tree::SPECIALIZATION, "Tyr's Enforcer" );
  talents.relentless_inquisitor          = find_talent_spell( talent_tree::SPECIALIZATION, "Relentless Inquisitor" );
  talents.avenging_wrath_might           = find_talent_spell( talent_tree::SPECIALIZATION, "Avenging Wrath: Might" );
  talents.sentinel                       = find_talent_spell( talent_tree::SPECIALIZATION, "Sentinel" );
  talents.crusaders_judgment             = find_talent_spell( talent_tree::SPECIALIZATION, "Crusader's Judgment" );
  talents.consecration_in_flame          = find_talent_spell( talent_tree::SPECIALIZATION, "Consecration in Flame" );

  talents.soaring_shield                 = find_talent_spell( talent_tree::SPECIALIZATION, "Soaring Shield" );
  talents.seal_of_reprisal               = find_talent_spell( talent_tree::SPECIALIZATION, "Seal of Reprisal" );
  talents.guardian_of_ancient_kings      = find_talent_spell( talent_tree::SPECIALIZATION, "Guardian of Ancient Kings" );
  talents.hand_of_the_protector          = find_talent_spell( talent_tree::SPECIALIZATION, "Hand of the Protector" );
  talents.sanctuary                      = find_talent_spell( talent_tree::SPECIALIZATION, "Sanctuary" );


  //20
  talents.focused_enmity                 = find_talent_spell( talent_tree::SPECIALIZATION, "Focused Enmity" );
  talents.gift_of_the_golden_valkyr      = find_talent_spell( talent_tree::SPECIALIZATION, "Gift of the Golden Val'kyr" );
  talents.sanctified_wrath               = find_talent_spell( talent_tree::SPECIALIZATION, "Sanctified Wrath" );
  talents.uthers_counsel                 = find_talent_spell( talent_tree::SPECIALIZATION, "Uther's Counsel" );

  talents.strength_in_adversity          = find_talent_spell( talent_tree::SPECIALIZATION, "Strength in Adversity" );
  talents.crusaders_resolve              = find_talent_spell( talent_tree::SPECIALIZATION, "Crusader's Resolve" );
  talents.ferren_marcuss_fervor          = find_talent_spell( talent_tree::SPECIALIZATION, "Ferren Marcus's Fervor" );
  talents.empyrean_authority             = find_talent_spell( talent_tree::SPECIALIZATION, "Empyrean Authority" );
  talents.zealots_paragon                = find_talent_spell( talent_tree::SPECIALIZATION, "Zealot's Paragon" );
  talents.instrument_of_the_divine       = find_talent_spell( talent_tree::SPECIALIZATION, "Instrument of the Divine" );

  talents.sweeping_verdict               = find_talent_spell( talent_tree::SPECIALIZATION, "Sweeping Verdict" );
  talents.adjudication                   = find_talent_spell( talent_tree::SPECIALIZATION, "Adjudication" );
  talents.bulwark_of_righteous_fury      = find_talent_spell( talent_tree::SPECIALIZATION, "Bulwark of Righteous Fury" );
  talents.final_stand                    = find_talent_spell( talent_tree::SPECIALIZATION, "Final Stand" );
  talents.righteous_protector            = find_talent_spell( talent_tree::SPECIALIZATION, "Righteous Protector" );

  talents.glory_of_the_vanguard_1 = find_talent_spell( talent_tree::SPECIALIZATION, 1267203 );
  talents.glory_of_the_vanguard_2 = find_talent_spell( talent_tree::SPECIALIZATION, 1267211 );
  talents.glory_of_the_vanguard_3 = find_talent_spell( talent_tree::SPECIALIZATION, 1267215 );


  // Spec passives and useful spells
  spec.protection_paladin = find_specialization_spell( "Protection Paladin" );
  spec.protection_paladin_2 = find_spell( 1305063 );
  mastery.divine_bulwark = find_mastery_spell( PALADIN_PROTECTION );
  mastery.divine_bulwark_2 = find_specialization_spell( "Mastery: Divine Bulwark", "Rank 2" );


  if ( specialization() == PALADIN_PROTECTION )
  {
    spec.consecration_3 = find_rank_spell( "Consecration", "Rank 3" );
    spec.consecration_2 = find_rank_spell( "Consecration", "Rank 2" );
    spec.judgment_3 = find_rank_spell( "Judgment", "Rank 3" );
    spec.judgment_4 = find_rank_spell( "Judgment", "Rank 4" );

    spells.judgment_debuff = find_spell( 197277 );
    spells.standing_in_consecration_buff = find_spell( 188370 );
  }

  spec.shield_of_the_righteous = find_class_spell( "Shield of the Righteous" );
  spells.sotr_buff = find_spell( 132403 );

  passives.riposte             = find_specialization_spell( "Riposte" );
  passives.sanctuary           = find_specialization_spell( "Sanctuary" );
  passives.aegis_of_light      = find_specialization_spell( "Aegis of Light" );

  spells.sentinel = find_spell( 389539 );
  spells.refining_fire_tick = find_spell( 469882 );

  spells.glory_of_the_vanguard = find_spell( 1269175 );
  spells.blaze_of_glory        = find_spell( 1269224 );
}

// Action Priority List Generation
void paladin_t::generate_action_prio_list_prot()
{
  paladin_apl::protection( this );
}
}
