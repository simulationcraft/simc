// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_monk.hpp"

#include "simulationcraft.hpp"

namespace monk
{
// ==========================================================================
// Monk Pets & Statues
// ==========================================================================

namespace pets
{
// ==========================================================================
// Base Monk Pet Action
// ==========================================================================

struct monk_pet_t : public pet_t
{
  monk_pet_t( monk_t* owner, util::string_view name, pet_e pet_type, bool guardian, bool dynamic )
    : pet_t( owner->sim, owner, name, pet_type, guardian, dynamic )
  {
  }

  monk_t* o()
  {
    return static_cast<monk_t*>( owner );
  }

  const monk_t* o() const
  {
    return static_cast<monk_t*>( owner );
  }
};

template <typename BASE, typename PET_TYPE = monk_pet_t>
struct pet_action_base_t : public BASE
{
  using super_t = BASE;
  using base_t  = pet_action_base_t<BASE>;

  pet_action_base_t( util::string_view n, PET_TYPE* p, const spell_data_t* data = spell_data_t::nil() )
    : BASE( n, p, data )
  {
    // No costs are needed either
    this->base_costs[ RESOURCE_ENERGY ] = 0;
    this->base_costs[ RESOURCE_CHI ]    = 0;
    this->base_costs[ RESOURCE_MANA ]   = 0;
  }

  void init() override
  {
    super_t::init();

    if ( !this->player->sim->report_pets_separately )
    {
      auto it =
          range::find_if( o()->pet_list, [ this ]( pet_t* pet ) { return this->player->name_str == pet->name_str; } );

      if ( it != o()->pet_list.end() && this->player != *it )
      {
        this->stats = ( *it )->get_stats( this->name(), this );
      }
    }
  }

  monk_t* o()
  {
    return p()->o();
  }

  const monk_t* o() const
  {
    return p()->o();
  }

  const PET_TYPE* p() const
  {
    return debug_cast<const PET_TYPE*>( this->player );
  }

  PET_TYPE* p()
  {
    return debug_cast<PET_TYPE*>( this->player );
  }

  void execute() override
  {
    this->target = this->player->target;

    super_t::execute();
  }
};

// ==========================================================================
// Base Monk Pet Melee Attack
// ==========================================================================

struct pet_melee_attack_t : public pet_action_base_t<melee_attack_t>
{
  pet_melee_attack_t( util::string_view n, monk_pet_t* p, const spell_data_t* data = spell_data_t::nil() )
    : base_t( n, p, data )
  {
  }

  // Physical tick_action abilities need amount_type() override, so the
  // tick_action multistrikes are properly physically mitigated.
  result_amount_type amount_type( const action_state_t* state, bool periodic ) const override
  {
    if ( tick_action && tick_action->school == SCHOOL_PHYSICAL )
    {
      return result_amount_type::DMG_DIRECT;
    }
    else
    {
      return base_t::amount_type( state, periodic );
    }
  }
};

struct pet_melee_t : pet_melee_attack_t
{
  pet_melee_t( util::string_view name, monk_pet_t* player, weapon_t* weapon )
    : pet_melee_attack_t( name, player, spell_data_t::nil() )
  {
    background = repeating = may_crit = may_glance = true;
    school                                         = SCHOOL_PHYSICAL;
    weapon_multiplier                              = 1.0;
    this->weapon                                   = weapon;
    // Use damage numbers from the level-scaled weapon
    base_execute_time = weapon->swing_time;
    trigger_gcd       = timespan_t::zero();
    special           = false;

    // TODO: check if there should be a dual wield hit malus here.
  }

  void execute() override
  {
    if ( time_to_execute > timespan_t::zero() && player->executing )
    {
      sim->print_debug( "Executing {} during melee ({}).", *player->executing, weapon->slot );
      schedule_execute();
    }
    else
      pet_melee_attack_t::execute();
  }
};

// ==========================================================================
// Generalized Auto Attack Action
// ==========================================================================

struct pet_auto_attack_t : public melee_attack_t
{
  pet_auto_attack_t( monk_pet_t* player ) : melee_attack_t( "auto_attack", player )
  {
    assert( player->main_hand_weapon.type != WEAPON_NONE );
    player->main_hand_attack = nullptr;
    trigger_gcd              = 0_ms;
  }

  void init() override
  {
    melee_attack_t::init();

    assert( player->main_hand_attack && "Pet auto attack created without main hand attack" );
  }

  void execute() override
  {
    player->main_hand_attack->schedule_execute();

    if ( player->off_hand_attack )
      player->off_hand_attack->schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() )
      return false;
    return ( player->main_hand_attack->execute_event == nullptr );
  }
};

// ==========================================================================
// Base Monk Pet Spell
// ==========================================================================

struct pet_spell_t : public pet_action_base_t<spell_t>
{
  pet_spell_t( util::string_view n, monk_pet_t* p, const spell_data_t* data = spell_data_t::nil() )
    : base_t( n, p, data )
  {
  }
};

// ==========================================================================
// Base Monk Heal Spell
// ==========================================================================

struct pet_heal_t : public pet_action_base_t<heal_t>
{
  pet_heal_t( util::string_view n, monk_pet_t* p, const spell_data_t* data = spell_data_t::nil() )
    : base_t( n, p, data )
  {
  }
};

// ==========================================================================
// Storm Earth and Fire (SEF)
// ==========================================================================

struct storm_earth_and_fire_pet_t : public monk_pet_t
{
  // Storm, Earth, and Fire abilities begin =================================

  template <typename BASE>
  struct sef_action_base_t : public pet_action_base_t<BASE, storm_earth_and_fire_pet_t>
  {
    using super_t = pet_action_base_t<BASE, storm_earth_and_fire_pet_t>;
    using base_t  = sef_action_base_t<BASE>;

    const action_t* source_action;

    sef_action_base_t( util::string_view n, storm_earth_and_fire_pet_t* p,
                       const spell_data_t* data = spell_data_t::nil() )
      : super_t( n, p, data ), source_action( nullptr )
    {
      // Make SEF attacks always background, so they do not consume resources
      // or do anything associated with "foreground actions".
      this->background = this->may_crit = true;
      this->callbacks                   = false;

      // Cooldowns are handled automatically by the mirror abilities, the SEF specific ones need none.
      this->cooldown->duration = timespan_t::zero();

      // No costs are needed either
      this->base_costs[ RESOURCE_ENERGY ] = 0;
      this->base_costs[ RESOURCE_CHI ]    = 0;
    }

    void init() override
    {
      super_t::init();

      // Find source_action from the owner by matching the action name and
      // spell id with eachother. This basically means that by default, any
      // spell-data driven ability with 1:1 mapping of name/spell id will
      // always be chosen as the source action. In some cases this needs to be
      // overridden (see sef_zen_sphere_t for example).
      for ( const action_t* a : this->o()->action_list )
      {
        if ( ( this->id > 0 && this->id == a->id ) || util::str_compare_ci( this->name_str, a->name_str ) )
        {
          source_action = a;
          break;
        }
      }

      if ( source_action )
      {
        this->update_flags   = source_action->update_flags;
        this->snapshot_flags = source_action->snapshot_flags;
      }
    }

    // Use SEF-specific override methods for target related multipliers as the
    // pets seem to have their own functionality relating to it. The rest of
    // the state-related stuff is actually mapped to the source (owner) action
    // below.

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = super_t::composite_target_multiplier( t );

      return m;
    }

    // Map the rest of the relevant state-related stuff into the source
    // action's methods. In other words, use the owner's data. Note that attack
    // power is not included here, as we will want to (just in case) snapshot
    // AP through the pet's own AP system. This allows us to override the
    // inheritance coefficient if need be in an easy way.

    double attack_direct_power_coefficient( const action_state_t* state ) const override
    {
      return source_action->attack_direct_power_coefficient( state );
    }

    double attack_tick_power_coefficient( const action_state_t* state ) const override
    {
      return source_action->attack_tick_power_coefficient( state );
    }

    timespan_t composite_dot_duration( const action_state_t* s ) const override
    {
      return source_action->composite_dot_duration( s );
    }

    timespan_t tick_time( const action_state_t* s ) const override
    {
      return source_action->tick_time( s );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      return source_action->composite_da_multiplier( s );
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      return source_action->composite_ta_multiplier( s );
    }

    double composite_persistent_multiplier( const action_state_t* s ) const override
    {
      return source_action->composite_persistent_multiplier( s );
    }

    double composite_versatility( const action_state_t* s ) const override
    {
      return source_action->composite_versatility( s );
    }

    double composite_haste() const override
    {
      return source_action->composite_haste();
    }

    timespan_t travel_time() const override
    {
      return source_action->travel_time();
    }

    int n_targets() const override
    {
      return source_action ? source_action->n_targets() : super_t::n_targets();
    }

    void execute() override
    {
      // Target always follows the SEF clone's target, which is assigned during
      // summon time
      this->target = this->player->target;

      super_t::execute();
    }

    void impact( action_state_t* s ) override
    {
      auto owner = this->o();

      owner->trigger_empowered_tiger_lightning( s );
      owner->trigger_bonedust_brew( s );

      super_t::impact( s );
    }

    void snapshot_internal( action_state_t* state, uint32_t flags, result_amount_type rt ) override
    {
      super_t::snapshot_internal( state, flags, rt );

      if ( this->o()->conduit.coordinated_offensive->ok() && this->p()->sticky_target )
      {
        if ( rt == result_amount_type::DMG_DIRECT && ( flags & STATE_MUL_DA ) )
          state->da_multiplier += this->o()->conduit.coordinated_offensive.percent();

        if ( rt == result_amount_type::DMG_OVER_TIME && ( flags & STATE_MUL_TA ) )
          state->ta_multiplier += this->o()->conduit.coordinated_offensive.percent();
      }
    }
  };

  struct sef_melee_attack_t : public sef_action_base_t<melee_attack_t>
  {
    sef_melee_attack_t( util::string_view n, storm_earth_and_fire_pet_t* p,
                        const spell_data_t* data = spell_data_t::nil() )
      : base_t( n, p, data )
    {
      school = SCHOOL_PHYSICAL;
    }

    // Physical tick_action abilities need amount_type() override, so the
    // tick_action multistrikes are properly physically mitigated.
    result_amount_type amount_type( const action_state_t* state, bool periodic ) const override
    {
      if ( tick_action && tick_action->school == SCHOOL_PHYSICAL )
      {
        return result_amount_type::DMG_DIRECT;
      }
      else
      {
        return base_t::amount_type( state, periodic );
      }
    }
  };

  struct sef_spell_t : public sef_action_base_t<spell_t>
  {
    sef_spell_t( util::string_view n, storm_earth_and_fire_pet_t* p, const spell_data_t* data = spell_data_t::nil() )
      : base_t( n, p, data )
    {
    }
  };

  // Auto attack ============================================================

  struct melee_t : public sef_melee_attack_t
  {
    melee_t( util::string_view n, storm_earth_and_fire_pet_t* player, weapon_t* w )
      : sef_melee_attack_t( n, player, spell_data_t::nil() )
    {
      background = repeating = may_crit = may_glance = true;
      weapon                                         = w;
      school                                         = SCHOOL_PHYSICAL;
      weapon_multiplier                              = 1.0;
      base_execute_time                              = w->swing_time;
      trigger_gcd                                    = timespan_t::zero();
      special                                        = false;

      if ( player->dual_wield() )
      {
        base_hit -= 0.19;
      }

      if ( w == &( player->main_hand_weapon ) )
      {
        source_action = player->owner->find_action( "melee_main_hand" );
      }
      else
      {
        source_action = player->owner->find_action( "melee_off_hand" );
        // If owner is using a 2handed weapon, there's not going to be an
        // off-hand action for autoattacks, thus just use main hand one then.
        if ( !source_action )
        {
          source_action = player->owner->find_action( "melee_main_hand" );
        }
      }

      // TODO: Can't really assert here, need to figure out a fallback if the
      // windwalker does not use autoattacks (how likely is that?)
      if ( !source_action && sim->debug )
      {
        sim->error( "{} has no auto_attack in APL, Storm, Earth, and Fire pets cannot auto-attack.", *o() );
      }
    }

    // A wild equation appears
    double composite_attack_power() const override
    {
      double ap = sef_melee_attack_t::composite_attack_power();

      if ( o()->main_hand_weapon.group() == WEAPON_2H )
      {
        ap += o()->main_hand_weapon.dps * 3.5;
      }
      else
      {
        // 1h/dual wield equation. Note, this formula is slightly off (~3%) for
        // owner dw/pet dw variation.
        double total_dps = o()->main_hand_weapon.dps;
        double dw_mul    = 1.0;
        if ( o()->off_hand_weapon.group() != WEAPON_NONE )
        {
          total_dps += o()->off_hand_weapon.dps * 0.5;
          dw_mul = 0.898882275;
        }

        ap += total_dps * 3.5 * dw_mul;
      }

      return ap;
    }

    void execute() override
    {
      if ( time_to_execute > timespan_t::zero() && ( player->channeling || player->executing ) )
      {
        sim->print_debug( "{} Executing {} during melee ({}).", *player,
                          player->executing ? *player->executing : *player->channeling, weapon->slot );

        schedule_execute();
      }
      else
      {
        sef_melee_attack_t::execute();
      }
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( storm_earth_and_fire_pet_t* player, util::string_view options_str ) : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      melee_t* mh = new melee_t( "auto_attack_mh", player, &( player->main_hand_weapon ) );
      if ( !mh->source_action )
      {
        background = true;
        return;
      }
      player->main_hand_attack = mh;

      if ( player->dual_wield() )
      {
        player->off_hand_attack = new melee_t( "auto_attack_oh", player, &( player->off_hand_weapon ) );
      }
    }
  };

  // Special attacks ========================================================
  //
  // Note, these automatically use the owner's multipliers, so there's no need
  // to adjust anything here.

  struct sef_tiger_palm_t : public sef_melee_attack_t
  {
    sef_tiger_palm_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "tiger_palm", player, player->o()->spec.tiger_palm )
    {
    }

    void impact( action_state_t* state ) override
    {
      sef_melee_attack_t::impact( state );

      if ( result_is_hit( state->result ) && o()->spec.spinning_crane_kick_2_ww->ok() )
        o()->trigger_mark_of_the_crane( state );
    }
  };

  struct sef_blackout_kick_t : public sef_melee_attack_t
  {
    sef_blackout_kick_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "blackout_kick", player, player->o()->spec.blackout_kick )
    {
      // Hard Code the divider
      base_dd_min = base_dd_max = 1;
    }

    void impact( action_state_t* state ) override
    {
      sef_melee_attack_t::impact( state );

      if ( result_is_hit( state->result ) )
      {
        if ( o()->spec.spinning_crane_kick_2_ww->ok() )
          o()->trigger_mark_of_the_crane( state );

        if ( p()->buff.bok_proc_sef->up() )
          p()->buff.bok_proc_sef->expire();
      }
    }
  };

  struct sef_rising_sun_kick_dmg_t : public sef_melee_attack_t
  {
    sef_rising_sun_kick_dmg_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "rising_sun_kick_dmg", player, player->o()->spec.rising_sun_kick->effectN( 1 ).trigger() )
    {
      background = true;
    }

    double composite_crit_chance() const override
    {
      double c = sef_melee_attack_t::composite_crit_chance();

      if ( o()->buff.pressure_point->up() )
        c += o()->buff.pressure_point->value();

      return c;
    }

    void impact( action_state_t* state ) override
    {
      sef_melee_attack_t::impact( state );

      if ( result_is_hit( state->result ) )
      {
        if ( o()->spec.combat_conditioning->ok() )
          state->target->debuffs.mortal_wounds->trigger();

        if ( o()->spec.spinning_crane_kick_2_ww->ok() )
          o()->trigger_mark_of_the_crane( state );
      }
    }
  };

  struct sef_rising_sun_kick_t : public sef_melee_attack_t
  {
    sef_rising_sun_kick_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "rising_sun_kick", player, player->o()->spec.rising_sun_kick )
    {
      execute_action = new sef_rising_sun_kick_dmg_t( player );
    }
  };

  struct sef_tick_action_t : public sef_melee_attack_t
  {
    sef_tick_action_t( util::string_view name, storm_earth_and_fire_pet_t* p, const spell_data_t* data )
      : sef_melee_attack_t( name, p, data )
    {
      aoe = -1;

      // Reset some variables to ensure proper execution
      dot_duration = timespan_t::zero();
      school       = SCHOOL_PHYSICAL;
    }
  };

  struct sef_fists_of_fury_tick_t : public sef_tick_action_t
  {
    sef_fists_of_fury_tick_t( storm_earth_and_fire_pet_t* p )
      : sef_tick_action_t( "fists_of_fury_tick", p, p->o()->passives.fists_of_fury_tick )
    {
      aoe = 1 + as<int>( p->o()->spec.fists_of_fury->effectN( 1 ).base_value() );
    }
  };

  struct sef_fists_of_fury_t : public sef_melee_attack_t
  {
    sef_fists_of_fury_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "fists_of_fury", player, player->o()->spec.fists_of_fury )
    {
      channeled = tick_zero = interrupt_auto_attack = true;
      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;
      // Hard code a 25% reduced cast time to not cause any clipping issues
      // https://us.battle.net/forums/en/wow/topic/20752377961?page=29#post-573
      dot_duration = data().duration() / 1.25;
      // Effect 1 shows a period of 166 milliseconds which appears to refer to the visual and not the tick period
      base_tick_time = ( dot_duration / 4 );

      weapon_power_mod = 0;

      tick_action = new sef_fists_of_fury_tick_t( player );
    }

    // Base tick_time(action_t) is somehow pulling the Owner's base_tick_time instead of the pet's
    // Forcing SEF to use it's own base_tick_time for tick_time.
    timespan_t tick_time( const action_state_t* state ) const override
    {
      timespan_t t = base_tick_time;
      if ( channeled || hasted_ticks )
      {
        t *= state->haste;
      }
      return t;
    }

    timespan_t composite_dot_duration( const action_state_t* s ) const override
    {
      if ( channeled )
        return dot_duration * ( tick_time( s ) / base_tick_time );

      return dot_duration;
    }
  };

  struct sef_spinning_crane_kick_tick_t : public sef_tick_action_t
  {
    sef_spinning_crane_kick_tick_t( storm_earth_and_fire_pet_t* p )
      : sef_tick_action_t( "spinning_crane_kick_tick", p, p->o()->spec.spinning_crane_kick->effectN( 1 ).trigger() )
    {
      aoe = as<int>( p->o()->spec.spinning_crane_kick->effectN( 1 ).base_value() );
    }
  };

  struct sef_spinning_crane_kick_t : public sef_melee_attack_t
  {
    sef_spinning_crane_kick_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "spinning_crane_kick", player, player->o()->spec.spinning_crane_kick )
    {
      tick_zero = hasted_ticks = interrupt_auto_attack = true;
      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      weapon_power_mod = 0;

      tick_action = new sef_spinning_crane_kick_tick_t( player );
    }
  };

  struct sef_rushing_jade_wind_tick_t : public sef_tick_action_t
  {
    sef_rushing_jade_wind_tick_t( storm_earth_and_fire_pet_t* p )
      : sef_tick_action_t( "rushing_jade_wind_tick", p, p->o()->talent.rushing_jade_wind->effectN( 1 ).trigger() )
    {
      aoe = -1;
    }
  };

  struct sef_rushing_jade_wind_t : public sef_melee_attack_t
  {
    sef_rushing_jade_wind_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "rushing_jade_wind", player, player->o()->talent.rushing_jade_wind )
    {
      dual = true;

      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      weapon_power_mod = 0;

      if ( !player->active_actions.rushing_jade_wind_sef )
      {
        player->active_actions.rushing_jade_wind_sef        = new sef_rushing_jade_wind_tick_t( player );
        player->active_actions.rushing_jade_wind_sef->stats = stats;
      }
    }

    void execute() override
    {
      sef_melee_attack_t::execute();

      p()->buff.rushing_jade_wind_sef->trigger();
    }
  };

  struct sef_whirling_dragon_punch_tick_t : public sef_tick_action_t
  {
    sef_whirling_dragon_punch_tick_t( storm_earth_and_fire_pet_t* p )
      : sef_tick_action_t( "whirling_dragon_punch_tick", p, p->o()->passives.whirling_dragon_punch_tick )
    {
      aoe = -1;
    }
  };

  struct sef_whirling_dragon_punch_t : public sef_melee_attack_t
  {
    sef_whirling_dragon_punch_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "whirling_dragon_punch", player, player->o()->talent.whirling_dragon_punch )
    {
      channeled = true;

      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      weapon_power_mod = 0;

      tick_action = new sef_whirling_dragon_punch_tick_t( player );
    }
  };

  struct sef_fist_of_the_white_tiger_oh_t : public sef_melee_attack_t
  {
    sef_fist_of_the_white_tiger_oh_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "fist_of_the_white_tiger_offhand", player, player->o()->talent.fist_of_the_white_tiger )
    {
      may_dodge = may_parry = may_block = may_miss = true;
      dual                                         = true;

      energize_type = action_energize::NONE;
    }

    void impact( action_state_t* state ) override
    {
      sef_melee_attack_t::impact( state );

      if ( result_is_hit( state->result ) )
      {
        if ( o()->spec.spinning_crane_kick_2_ww->ok() )
          o()->trigger_mark_of_the_crane( state );
      }
    }
  };

  struct sef_fist_of_the_white_tiger_t : public sef_melee_attack_t
  {
    sef_fist_of_the_white_tiger_t( storm_earth_and_fire_pet_t* player )
      : sef_melee_attack_t( "fist_of_the_white_tiger", player,
                            player->o()->talent.fist_of_the_white_tiger->effectN( 2 ).trigger() )
    {
      may_dodge = may_parry = may_block = may_miss = true;
      dual                                         = true;
    }
  };

  struct sef_chi_wave_damage_t : public sef_spell_t
  {
    sef_chi_wave_damage_t( storm_earth_and_fire_pet_t* player )
      : sef_spell_t( "chi_wave_damage", player, player->o()->passives.chi_wave_damage )
    {
      dual = true;
    }
  };

  // SEF Chi Wave skips the healing ticks, delivering damage on every second
  // tick of the ability for simplicity.
  struct sef_chi_wave_t : public sef_spell_t
  {
    sef_chi_wave_damage_t* wave;

    sef_chi_wave_t( storm_earth_and_fire_pet_t* player )
      : sef_spell_t( "chi_wave", player, player->o()->talent.chi_wave ), wave( new sef_chi_wave_damage_t( player ) )
    {
      may_crit = may_miss = hasted_ticks = false;
      tick_zero = tick_may_crit = true;
    }

    void tick( dot_t* d ) override
    {
      if ( d->current_tick % 2 == 0 )
      {
        sef_spell_t::tick( d );
        wave->target = d->target;
        wave->schedule_execute();
      }
    }
  };

  struct sef_crackling_jade_lightning_t : public sef_spell_t
  {
    sef_crackling_jade_lightning_t( storm_earth_and_fire_pet_t* player )
      : sef_spell_t( "crackling_jade_lightning", player, player->o()->spec.crackling_jade_lightning )
    {
      tick_may_crit = true;
      channeled = tick_zero = true;
      hasted_ticks          = false;
      interrupt_auto_attack = true;
      dot_duration          = data().duration();
    }

    // Base tick_time(action_t) is somehow pulling the Owner's base_tick_time instead of the pet's
    // Forcing SEF to use it's own base_tick_time for tick_time.
    timespan_t tick_time( const action_state_t* state ) const override
    {
      timespan_t t = base_tick_time;
      if ( channeled || hasted_ticks )
      {
        t *= state->haste;
      }
      return t;
    }

    double cost_per_tick( resource_e ) const override
    {
      return 0;
    }
  };

  // Storm, Earth, and Fire abilities end ===================================

  std::vector<sef_melee_attack_t*> attacks;
  std::vector<sef_spell_t*> spells;

public:
  // SEF applies the Cyclone Strike debuff as well

  bool sticky_target;  // When enabled, SEF pets will stick to the target they have

  struct
  {
    action_t* rushing_jade_wind_sef = nullptr;
  } active_actions;

  struct
  {
    buff_t* bok_proc_sef          = nullptr;
    buff_t* hit_combo_sef         = nullptr;
    buff_t* pressure_point_sef    = nullptr;
    buff_t* rushing_jade_wind_sef = nullptr;
  } buff;

  storm_earth_and_fire_pet_t( util::string_view name, monk_t* owner, bool dual_wield, weapon_e weapon_type )
    : monk_pet_t( owner, name, PET_NONE, true, true ),
      attacks( SEF_ATTACK_MAX ),
      spells( SEF_SPELL_MAX - SEF_SPELL_MIN ),
      sticky_target( false ),
      active_actions(),
      buff()
  {
    // Storm, Earth, and Fire pets have to become "Windwalkers", so we can get
    // around some sanity checks in the action execution code, that prevents
    // abilities meant for a certain specialization to be executed by actors
    // that do not have the specialization.
    _spec = MONK_WINDWALKER;

    main_hand_weapon.type       = weapon_type;
    main_hand_weapon.swing_time = timespan_t::from_seconds( dual_wield ? 2.6 : 3.6 );

    if ( dual_wield )
    {
      off_hand_weapon.type       = weapon_type;
      off_hand_weapon.swing_time = timespan_t::from_seconds( 2.6 );
    }

    if ( name == "fire_spirit" )
      npc_id = 69791;
    else if ( name == "earth_spirit" )
      npc_id = 69792;

    owner_coeff.ap_from_ap = 1.0;
  }

  // Reset SEF target to default settings
  void reset_targeting()
  {
    target        = owner->target;
    sticky_target = false;
  }

  timespan_t available() const override
  {
    return sim->expected_iteration_time * 2;
  }

  void init_spells() override
  {
    monk_pet_t::init_spells();

    attacks.at( SEF_TIGER_PALM )                 = new sef_tiger_palm_t( this );
    attacks.at( SEF_BLACKOUT_KICK )              = new sef_blackout_kick_t( this );
    attacks.at( SEF_RISING_SUN_KICK )            = new sef_rising_sun_kick_t( this );
    attacks.at( SEF_FISTS_OF_FURY )              = new sef_fists_of_fury_t( this );
    attacks.at( SEF_SPINNING_CRANE_KICK )        = new sef_spinning_crane_kick_t( this );
    attacks.at( SEF_RUSHING_JADE_WIND )          = new sef_rushing_jade_wind_t( this );
    attacks.at( SEF_WHIRLING_DRAGON_PUNCH )      = new sef_whirling_dragon_punch_t( this );
    attacks.at( SEF_FIST_OF_THE_WHITE_TIGER )    = new sef_fist_of_the_white_tiger_t( this );
    attacks.at( SEF_FIST_OF_THE_WHITE_TIGER_OH ) = new sef_fist_of_the_white_tiger_oh_t( this );

    spells.at( sef_spell_index( SEF_CHI_WAVE ) )                 = new sef_chi_wave_t( this );
    spells.at( sef_spell_index( SEF_CRACKLING_JADE_LIGHTNING ) ) = new sef_crackling_jade_lightning_t( this );
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack";

    monk_pet_t::init_action_list();
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return monk_pet_t::create_action( name, options_str );
  }

  void summon( timespan_t duration = timespan_t::zero() ) override
  {
    monk_pet_t::summon( duration );

    o()->buff.storm_earth_and_fire->trigger( 1, buff_t::DEFAULT_VALUE(), 1, duration );

    if ( o()->buff.bok_proc->up() )
      buff.bok_proc_sef->trigger( 1, buff_t::DEFAULT_VALUE(), 1, o()->buff.bok_proc->remains() );

    if ( o()->buff.hit_combo->up() )
      buff.hit_combo_sef->trigger( o()->buff.hit_combo->stack() );

    if ( o()->buff.rushing_jade_wind->up() )
      buff.rushing_jade_wind_sef->trigger( 1, buff_t::DEFAULT_VALUE(), 1, o()->buff.rushing_jade_wind->remains() );

    sticky_target = false;
  }

  void dismiss( bool expired = false ) override
  {
    monk_pet_t::dismiss( expired );

    o()->buff.storm_earth_and_fire->decrement();
  }

  void create_buffs() override
  {
    monk_pet_t::create_buffs();

    buff.bok_proc_sef =
        make_buff( this, "bok_proc_sef", o()->passives.bok_proc )
            ->set_quiet( true );  // In-game does not show this buff but I would like to use it for background stuff;

    buff.rushing_jade_wind_sef = make_buff( this, "rushing_jade_wind_sef", o()->talent.rushing_jade_wind )
                                     ->set_can_cancel( true )
                                     ->set_tick_zero( true )
                                     ->set_cooldown( timespan_t::zero() )
                                     ->set_period( o()->talent.rushing_jade_wind->effectN( 1 ).period() )
                                     ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
                                     ->set_duration( sim->expected_iteration_time * 2 )
                                     ->set_tick_behavior( buff_tick_behavior::CLIP )
                                     ->set_tick_callback( [ this ]( buff_t* d, int, timespan_t ) {
                                       if ( o()->buff.rushing_jade_wind->up() )
                                         active_actions.rushing_jade_wind_sef->execute();
                                       else
                                         d->expire( timespan_t::from_millis( 1 ) );
                                     } );

    buff.hit_combo_sef = make_buff( this, "hit_combo_sef", o()->passives.hit_combo )
                             ->set_default_value_from_effect( 1 )
                             ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  void trigger_attack( sef_ability_e ability, const action_t* source_action )
  {
    if ( o()->buff.combo_strikes->up() && o()->talent.hit_combo->ok() )
      buff.hit_combo_sef->trigger();

    if ( ability >= SEF_SPELL_MIN )
    {
      auto spell_index = sef_spell_index( ability );
      assert( spells[ spell_index ] );

      spells[ spell_index ]->source_action = source_action;
      spells[ spell_index ]->execute();
    }
    else
    {
      assert( attacks[ ability ] );
      attacks[ ability ]->source_action = source_action;
      attacks[ ability ]->execute();
    }
  }
};

// ==========================================================================
// Xuen Pet
// ==========================================================================
struct xuen_pet_t : public monk_pet_t
{
private:
  struct melee_t : public pet_melee_t
  {
    melee_t( util::string_view n, xuen_pet_t* player, weapon_t* weapon ) : pet_melee_t( n, player, weapon )
    {
    }

    void impact( action_state_t* s ) override
    {
      o()->trigger_empowered_tiger_lightning( s );
      o()->trigger_bonedust_brew( s );

      pet_melee_attack_t::impact( s );
    }
  };

  struct crackling_tiger_lightning_tick_t : public pet_spell_t
  {
    crackling_tiger_lightning_tick_t( xuen_pet_t* p )
      : pet_spell_t( "crackling_tiger_lightning_tick", p, p->o()->passives.crackling_tiger_lightning )
    {
      dual = direct_tick = background = may_crit = true;
    }

    void impact( action_state_t* s ) override
    {
      auto owner = o();
      owner->trigger_empowered_tiger_lightning( s );
      owner->trigger_bonedust_brew( s );

      pet_spell_t::impact( s );
    }
  };

  struct crackling_tiger_lightning_t : public pet_spell_t
  {
    crackling_tiger_lightning_t( xuen_pet_t* p, util::string_view options_str )
      : pet_spell_t( "crackling_tiger_lightning", p, p->o()->passives.crackling_tiger_lightning )
    {
      parse_options( options_str );

      // for future compatibility, we may want to grab Xuen and our tick spell and build this data from those (Xuen
      // summon duration, for example)
      dot_duration        = p->o()->spec.invoke_xuen->duration();
      hasted_ticks        = true;
      may_miss            = false;
      dynamic_tick_action = true;
      base_tick_time =
          p->o()->passives.crackling_tiger_lightning_driver->effectN( 1 ).period();  // trigger a tick every second
      cooldown->duration      = p->o()->spec.invoke_xuen->cooldown();                // we're done after 25 seconds
      attack_power_mod.direct = 0.0;
      attack_power_mod.tick   = 0.0;

      tick_action = new crackling_tiger_lightning_tick_t( p );
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( xuen_pet_t* player, util::string_view options_str ) : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      player->main_hand_attack = new melee_t( "melee_main_hand", player, &( player->main_hand_weapon ) );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    }
  };

public:
  xuen_pet_t( monk_t* owner ) : monk_pet_t( owner, "xuen_the_white_tiger", PET_XUEN, true, true )
  {
    npc_id                      = 63508;
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );
    owner_coeff.ap_from_ap      = 1.00;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double cpm = owner->cache.player_multiplier( school );

    if ( o()->buff.hit_combo->up() )
      cpm *= 1 + o()->buff.hit_combo->stack_value();

    if ( o()->conduit.xuens_bond->ok() )
      cpm *= 1 + o()->conduit.xuens_bond.percent();

    return cpm;
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack";
    action_list_str += "/crackling_tiger_lightning";

    pet_t::init_action_list();
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "crackling_tiger_lightning" )
      return new crackling_tiger_lightning_t( this, options_str );

    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Niuzao Pet
// ==========================================================================
struct niuzao_pet_t : public monk_pet_t
{
private:
  struct melee_t : public pet_melee_t
  {
    melee_t( util::string_view n, niuzao_pet_t* player, weapon_t* weapon ) : pet_melee_t( n, player, weapon )
    {
    }

    void impact( action_state_t* s ) override
    {
      o()->trigger_bonedust_brew( s );

      pet_melee_t::impact( s );
    }
  };

  struct stomp_t : public pet_melee_attack_t
  {
    stomp_t( niuzao_pet_t* p, util::string_view options_str ) : pet_melee_attack_t( "stomp", p, p->o()->passives.stomp )
    {
      parse_options( options_str );

      aoe      = -1;
      may_crit = true;
      // technically the base damage doesn't split. practically, the base damage
      // is ass and totally irrelevant. the r2 hot trub effect (which does
      // split) is by far the dominating factor in any aoe sim.
      //
      // if i knew more about simc, i'd implement a separate effect for that,
      // but i'm not breaking something that (mostly) works in pursuit of that
      // goal.
      //
      //  - emallson
      split_aoe_damage = true;
    }

    double bonus_da( const action_state_t* s ) const override
    {
      double b = pet_melee_attack_t::bonus_da( s );

      auto purify_amount = o()->buff.recent_purifies->value();
      auto actual_damage = purify_amount * o()->spec.invoke_niuzao_2->effectN( 1 ).percent();
      b += actual_damage;
      o()->sim->print_debug( "applying bonus purify damage (original: {}, reduced: {})", purify_amount, actual_damage );

      return b;
    }

    double action_multiplier() const override
    {
      double am = pet_melee_attack_t::action_multiplier();

      if ( o()->conduit.walk_with_the_ox->ok() )
        am *= 1 + o()->conduit.walk_with_the_ox.percent();

      return am;
    }

    void execute() override
    {
      pet_melee_attack_t::execute();
      // canceling the purify buff goes here so that in aoe all hits see the
      // purified damage that needs to be split. this occurs after all damage
      // has been dealt
      o()->buff.recent_purifies->cancel();
    }

    void impact( action_state_t* s ) override
    {
      o()->trigger_bonedust_brew( s );

      pet_melee_attack_t::impact( s );
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( niuzao_pet_t* player, util::string_view options_str ) : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      player->main_hand_attack = new melee_t( "melee_main_hand", player, &( player->main_hand_weapon ) );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    }
  };

public:
  niuzao_pet_t( monk_t* owner ) : monk_pet_t( owner, "niuzao_the_black_ox", PET_NIUZAO, true, true )
  {
    npc_id                      = 73967;
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    owner_coeff.ap_from_ap      = 1;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double cpm = pet_t::composite_player_multiplier( school );

    cpm *= 1 + o()->spec.brewmaster_monk->effectN( 3 ).percent();

    return cpm;
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack";
    action_list_str += "/stomp";

    pet_t::init_action_list();
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "stomp" )
      return new stomp_t( this, options_str );

    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Chi-Ji Pet
// ==========================================================================
struct chiji_pet_t : public monk_pet_t
{
private:
  struct melee_t : public pet_melee_t
  {
    melee_t( util::string_view n, chiji_pet_t* player, weapon_t* weapon ) : pet_melee_t( n, player, weapon )
    {
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( chiji_pet_t* player, util::string_view options_str ) : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      player->main_hand_attack = new melee_t( "melee_main_hand", player, &( player->main_hand_weapon ) );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    }
  };

public:
  chiji_pet_t( monk_t* owner ) : monk_pet_t( owner, "chiji_the_red_crane", PET_CHIJI, true, true )
  {
    npc_id                      = 166949;
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );
    owner_coeff.ap_from_ap      = o()->spec.mistweaver_monk->effectN( 4 ).percent();
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double cpm = monk_pet_t::composite_player_multiplier( school );

    return cpm;
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack";

    monk_pet_t::init_action_list();
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return monk_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Yu'lon Pet
// ==========================================================================
struct yulon_pet_t : public monk_pet_t
{
public:
  yulon_pet_t( monk_t* owner ) : monk_pet_t( owner, "yulon_the_jade_serpent", PET_YULON, true, true )
  {
    npc_id                      = 165374;
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    owner_coeff.ap_from_ap      = o()->spec.mistweaver_monk->effectN( 4 ).percent();
  }
};

// ==========================================================================
// Fallen Monk - Windwalker (Venthyr)
// ==========================================================================
struct fallen_monk_ww_pet_t : public monk_pet_t
{
private:
  struct melee_t : public pet_melee_t
  {
    melee_t( util::string_view n, fallen_monk_ww_pet_t* player, weapon_t* weapon ) : pet_melee_t( n, player, weapon )
    {
      // TODO: check why this is here
      base_hit -= 0.19;
    }

    // Copy melee code from Storm, Earth and Fire
    double composite_attack_power() const override
    {
      double ap  = pet_melee_t::composite_attack_power();
      auto owner = o();

      if ( owner->main_hand_weapon.group() == WEAPON_2H )
      {
        ap += owner->main_hand_weapon.dps * 3.5;
      }
      else
      {
        // 1h/dual wield equation. Note, this formula is slightly off (~3%) for
        // owner dw/pet dw variation.
        double total_dps = owner->main_hand_weapon.dps;
        double dw_mul    = 1.0;
        if ( owner->off_hand_weapon.group() != WEAPON_NONE )
        {
          total_dps += owner->off_hand_weapon.dps * 0.5;
          dw_mul = 0.898882275;
        }

        ap += total_dps * 3.5 * dw_mul;
      }

      return ap;
    }

    void impact( action_state_t* s ) override
    {
      o()->trigger_empowered_tiger_lightning( s );

      pet_melee_t::impact( s );
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( fallen_monk_ww_pet_t* player, util::string_view options_str ) : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      player->main_hand_attack = new melee_t( "melee_main_hand", player, &( player->main_hand_weapon ) );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    }
  };

public:
  struct
  {
    buff_t* hit_combo_fm_ww = nullptr;
  } buff;

  fallen_monk_ww_pet_t( monk_t* owner )
    : monk_pet_t( owner, "fallen_monk_windwalker", PET_FALLEN_MONK, true, true ), buff()
  {
    npc_id                      = 168033;
    main_hand_weapon.type       = WEAPON_1H;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2 );

    off_hand_weapon.type       = WEAPON_1H;
    off_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    off_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    off_hand_weapon.damage     = ( off_hand_weapon.min_dmg + off_hand_weapon.max_dmg ) / 2;
    off_hand_weapon.swing_time = timespan_t::from_seconds( 2 );

    switch ( owner->specialization() )
    {
      case MONK_WINDWALKER:
      case MONK_BREWMASTER:
        owner_coeff.ap_from_ap = 0.3333;
        owner_coeff.sp_from_ap = 0.32;
        break;
      case MONK_MISTWEAVER:
      {
        owner_coeff.ap_from_ap = 0.3333;
        owner_coeff.sp_from_sp = 0.3333;
        break;
      }
      default:
        break;
    }
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double cpm = o()->cache.player_multiplier( school );

    if ( o()->conduit.imbued_reflections->ok() )
      cpm *= 1 + o()->conduit.imbued_reflections.percent();

    return cpm;
  }

  void summon( timespan_t duration = timespan_t::zero() ) override
  {
    monk_pet_t::summon( duration );

    if ( o()->buff.hit_combo->up() )
      buff.hit_combo_fm_ww->trigger( o()->buff.hit_combo->stack() );
  }

  void create_buffs() override
  {
    monk_pet_t::create_buffs();

    buff.hit_combo_fm_ww = make_buff( this, "hit_combo_fo_ww", o()->passives.hit_combo )
                               ->set_default_value_from_effect( 1 )
                               ->set_quiet( true )
                               ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  struct fallen_monk_fists_of_fury_tick_t : public pet_melee_attack_t
  {
    fallen_monk_fists_of_fury_tick_t( fallen_monk_ww_pet_t* p )
      : pet_melee_attack_t( "fists_of_fury_tick_fo", p, p->o()->passives.fallen_monk_fists_of_fury_tick )
    {
      background              = true;
      aoe                     = 1 + (int)o()->passives.fallen_monk_fists_of_fury->effectN( 1 ).base_value();
      attack_power_mod.direct = o()->passives.fallen_monk_fists_of_fury->effectN( 5 ).ap_coeff();
      ap_type                 = attack_power_type::WEAPON_BOTH;
      dot_duration            = timespan_t::zero();
      trigger_gcd             = timespan_t::zero();
    }

    double composite_aoe_multiplier( const action_state_t* state ) const override
    {
      double cam = pet_melee_attack_t::composite_aoe_multiplier( state );

      if ( state->target != target )
        cam *= o()->passives.fallen_monk_fists_of_fury->effectN( 6 ).percent();

      return cam;
    }

    double action_multiplier() const override
    {
      double am = pet_melee_attack_t::action_multiplier();

      // monk_t* o = static_cast<monk_t*>( player );
      if ( o()->conduit.inner_fury->ok() )
        am *= 1 + o()->conduit.inner_fury.percent();

      return am;
    }

    void impact( action_state_t* s ) override
    {
      o()->trigger_empowered_tiger_lightning( s );

      pet_melee_attack_t::impact( s );
    }
  };

  struct fallen_monk_fists_of_fury_t : public pet_melee_attack_t
  {
    fallen_monk_fists_of_fury_t( fallen_monk_ww_pet_t* p, util::string_view options_str )
      : pet_melee_attack_t( "fists_of_fury_fo", p, p->o()->passives.fallen_monk_fists_of_fury )
    {
      parse_options( options_str );

      channeled = tick_zero = true;
      interrupt_auto_attack = true;

      attack_power_mod.direct = 0;
      attack_power_mod.tick   = 0;
      weapon_power_mod        = 0;

      // Effect 2 shows a period of 166 milliseconds which appears to refer to the visual and not the tick period
      base_tick_time = dot_duration / 4;
      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      tick_action = new fallen_monk_fists_of_fury_tick_t( p );
    }

    double action_multiplier() const override
    {
      return 0;
    }
  };

  struct fallen_monk_tiger_palm_t : public pet_melee_attack_t
  {
    fallen_monk_tiger_palm_t( fallen_monk_ww_pet_t* p, util::string_view options_str )
      : pet_melee_attack_t( "tiger_palm_fo", p, p->o()->passives.fallen_monk_tiger_palm )
    {
      parse_options( options_str );

      may_miss = may_block = may_dodge = may_parry = callbacks = false;

      // We only want the monk to cast Tiger Palm 2 times during the duration.
      // Increase the cooldown for non-windwalkers so that it only casts 2 times.
      if ( o()->specialization() == MONK_WINDWALKER )
        cooldown->duration = timespan_t::from_seconds( 2.5 );
      else
        cooldown->duration = timespan_t::from_seconds( 3.1 );
    }

    double cost() const override
    {
      return 0;
    }

    void impact( action_state_t* s ) override
    {
      o()->trigger_empowered_tiger_lightning( s );

      pet_melee_attack_t::impact( s );
    }
  };

  void init_action_list() override
  {
    action_list_str = "auto_attack";
    // Only cast Fists of Fury for Windwalker specialization
    if ( owner->specialization() == MONK_WINDWALKER )
      action_list_str += "/fists_of_fury";
    action_list_str += "/tiger_palm";

    monk_pet_t::init_action_list();
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    if ( name == "fists_of_fury" )
      return new fallen_monk_fists_of_fury_t( this, options_str );

    if ( name == "tiger_palm" )
      return new fallen_monk_tiger_palm_t( this, options_str );

    return monk_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Fallen Monk - Brewmaster (Venthyr)
// ==========================================================================
struct fallen_monk_brm_pet_t : public monk_pet_t
{
private:
  struct melee_t : public pet_melee_t
  {
    melee_t( util::string_view n, fallen_monk_brm_pet_t* player, weapon_t* weapon ) : pet_melee_t( n, player, weapon )
    {
    }

    void impact( action_state_t* s ) override
    {
      o()->trigger_empowered_tiger_lightning( s );

      pet_melee_t::impact( s );
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( fallen_monk_brm_pet_t* player, util::string_view options_str ) : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      player->main_hand_attack = new melee_t( "melee_main_hand", player, &( player->main_hand_weapon ) );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    }
  };

public:
  struct
  {
    buff_t* hit_combo_fm_brm = nullptr;
  } buff;

  fallen_monk_brm_pet_t( monk_t* owner )
    : monk_pet_t( owner, "fallen_monk_brewmaster", PET_FALLEN_MONK, true, true ), buff()
  {
    npc_id                      = 168073;
    main_hand_weapon.type       = WEAPON_2H;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1 );

    switch ( owner->specialization() )
    {
      case MONK_WINDWALKER:
      case MONK_BREWMASTER:
        owner_coeff.ap_from_ap = 0.3333;
        owner_coeff.sp_from_ap = 0.32;
        break;
      case MONK_MISTWEAVER:
      {
        owner_coeff.ap_from_ap = 0.3333;
        owner_coeff.sp_from_sp = 0.3333;
        break;
      }
      default:
        break;
    }
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double cpm = o()->cache.player_multiplier( school );

    if ( o()->conduit.imbued_reflections->ok() )
      cpm *= 1 + o()->conduit.imbued_reflections.percent();

    return cpm;
  }

  struct fallen_monk_keg_smash_t : public pet_melee_attack_t
  {
    fallen_monk_keg_smash_t( fallen_monk_brm_pet_t* p, util::string_view options_str )
      : pet_melee_attack_t( "keg_smash_fo", p, p->o()->passives.fallen_monk_keg_smash )
    {
      parse_options( options_str );

      aoe                     = -1;
      attack_power_mod.direct = p->o()->passives.fallen_monk_keg_smash->effectN( 2 ).ap_coeff();
      radius                  = p->o()->passives.fallen_monk_keg_smash->effectN( 2 ).radius();

      if ( o()->specialization() == MONK_BREWMASTER )
        cooldown->duration = timespan_t::from_seconds( 6.0 );
      else
        cooldown->duration = timespan_t::from_seconds( 9.0 );
      trigger_gcd = timespan_t::from_seconds( 1.5 );
    }

    // For more than 5 targets damage is based on a Sqrt(5/x)
    double composite_aoe_multiplier( const action_state_t* state ) const override
    {
      double cam = pet_melee_attack_t::composite_aoe_multiplier( state );

      if ( state->n_targets > o()->spec.keg_smash->effectN( 7 ).base_value() )
        // this is the closest we can come up without Blizzard flat out giving us the function
        // Primary takes the 100% damage
        // Secondary targets get reduced damage
        if ( state->target != target )
          cam *= std::sqrt( 5 / state->n_targets );

      return cam;
    }

    double action_multiplier() const override
    {
      double am = pet_melee_attack_t::action_multiplier();

      if ( o()->legendary.stormstouts_last_keg->ok() )
        am *= 1 + o()->legendary.stormstouts_last_keg->effectN( 1 ).percent();

      if ( o()->conduit.scalding_brew->ok() )
      {
        if ( o()->get_target_data( player->target )->dots.breath_of_fire->is_ticking() )
          am *= 1 + o()->conduit.scalding_brew.percent();
      }

      return am;
    }

    void impact( action_state_t* s ) override
    {
      o()->trigger_empowered_tiger_lightning( s );

      pet_melee_attack_t::impact( s );

      o()->get_target_data( s->target )->debuff.fallen_monk_keg_smash->trigger();
    }
  };

  struct fallen_monk_breath_of_fire_t : public pet_spell_t
  {
    struct fallen_monk_breath_of_fire_tick_t : public pet_spell_t
    {
      fallen_monk_breath_of_fire_tick_t( fallen_monk_brm_pet_t* p )
        : pet_spell_t( "breath_of_fire_dot_fo", p, p->o()->passives.breath_of_fire_dot )
      {
        background    = true;
        tick_may_crit = may_crit = true;
        hasted_ticks             = false;
      }

      // Initial damage does Square Root damage
      double composite_aoe_multiplier( const action_state_t* state ) const override
      {
        double cam = pet_spell_t::composite_aoe_multiplier( state );

        if ( state->target != target )
          return cam / std::sqrt( state->n_targets );

        return cam;
      }

      void impact( action_state_t* s ) override
      {
        o()->trigger_empowered_tiger_lightning( s );

        pet_spell_t::impact( s );
      }
    };

    fallen_monk_breath_of_fire_tick_t* dot_action;
    fallen_monk_breath_of_fire_t( fallen_monk_brm_pet_t* p, util::string_view options_str )
      : pet_spell_t( "breath_of_fire_fo", p, p->o()->passives.fallen_monk_breath_of_fire ),
        dot_action( new fallen_monk_breath_of_fire_tick_t( p ) )
    {
      parse_options( options_str );
      cooldown->duration = timespan_t::from_seconds( 9 );
      trigger_gcd        = timespan_t::from_seconds( 2 );

      add_child( dot_action );
    }

    void impact( action_state_t* s ) override
    {
      pet_spell_t::impact( s );

      if ( o()->get_target_data( s->target )->debuff.keg_smash->up() ||
           o()->get_target_data( s->target )->debuff.fallen_monk_keg_smash->up() )
      {
        dot_action->target = s->target;
        dot_action->execute();
      }
    }
  };

  struct fallen_monk_clash_t : public pet_spell_t
  {
    fallen_monk_clash_t( fallen_monk_brm_pet_t* p, util::string_view options_str )
      : pet_spell_t( "clash_fo", p, p->o()->passives.fallen_monk_clash )
    {
      parse_options( options_str );
      gcd_type = gcd_haste_type::NONE;

      cooldown->duration = timespan_t::from_seconds( 9 );
      trigger_gcd        = timespan_t::from_seconds( 2 );
    }
  };

  void init_action_list() override
  {
    action_list_str = "auto_attack";
    action_list_str += "/clash";
    action_list_str += "/keg_smash";
    // Only cast Breath of Fire for Brewmaster specialization
    if ( o()->specialization() == MONK_BREWMASTER )
      action_list_str += "/breath_of_fire";

    monk_pet_t::init_action_list();
  }

  void summon( timespan_t duration = timespan_t::zero() ) override
  {
    monk_pet_t::summon( duration );

    if ( o()->buff.hit_combo->up() )
      buff.hit_combo_fm_brm->trigger( o()->buff.hit_combo->stack() );
  }

  void create_buffs() override
  {
    monk_pet_t::create_buffs();

    buff.hit_combo_fm_brm = make_buff( this, "hit_combo_fo_brm", o()->passives.hit_combo )
                                ->set_default_value_from_effect( 1 )
                                ->set_quiet( true )
                                ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    if ( name == "clash" )
      return new fallen_monk_clash_t( this, options_str );

    if ( name == "keg_smash" )
      return new fallen_monk_keg_smash_t( this, options_str );

    if ( name == "breath_of_fire" )
      return new fallen_monk_breath_of_fire_t( this, options_str );

    return monk_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Fallen Monk - Mistweaver (Venthyr)
// ==========================================================================
struct fallen_monk_mw_pet_t : public monk_pet_t
{
public:
  fallen_monk_mw_pet_t( monk_t* owner ) : monk_pet_t( owner, "fallen_monk_mistweaver", PET_FALLEN_MONK, true, true )
  {
    npc_id                      = 168074;
    main_hand_weapon.type       = WEAPON_1H;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1 );

    owner_coeff.sp_from_ap = 0.98;
    owner_coeff.ap_from_ap = 0.98;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double cpm = o()->cache.player_multiplier( school );

    if ( o()->conduit.imbued_reflections->ok() )
      cpm *= 1 + o()->conduit.imbued_reflections.percent();

    return cpm;
  }

  struct fallen_monk_enveloping_mist_t : public pet_heal_t
  {
    fallen_monk_enveloping_mist_t( fallen_monk_mw_pet_t* p, util::string_view options_str )
      : pet_heal_t( "enveloping_mist_fo", p, p->o()->passives.fallen_monk_enveloping_mist )
    {
      parse_options( options_str );

      may_miss = false;

      dot_duration = data().duration();
      target       = p->o();
    }

    double cost() const override
    {
      return 0;
    }
  };

  struct fallen_monk_soothing_mist_t : public pet_heal_t
  {
    fallen_monk_soothing_mist_t( fallen_monk_mw_pet_t* p, util::string_view options_str )
      : pet_heal_t( "soothing_mist_fo", p, p->o()->passives.fallen_monk_soothing_mist )
    {
      parse_options( options_str );

      may_miss  = false;
      channeled = tick_zero = true;
      interrupt_auto_attack = true;

      dot_duration       = data().duration();
      trigger_gcd        = timespan_t::from_millis( 750 );
      cooldown->duration = timespan_t::from_seconds( 5 );
      cooldown->hasted   = true;
      target             = p->o();
    }
  };

  void init_action_list() override
  {
    action_list_str = "";
    // Only cast Enveloping Mist for Mistweaver specialization
    if ( o()->specialization() == MONK_MISTWEAVER )
      action_list_str += "/enveloping_mist";
    action_list_str += "/soothing_mist";

    monk_pet_t::init_action_list();
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "enveloping_mist" )
      return new fallen_monk_enveloping_mist_t( this, options_str );

    if ( name == "soothing_mist" )
      return new fallen_monk_soothing_mist_t( this, options_str );

    return monk_pet_t::create_action( name, options_str );
  }
};
}  // end namespace pets

monk_t::pets_t::pets_t( monk_t* p )
  : sef(),
    fallen_monk_ww( "fallen_monk_windwalker", p, []( monk_t* p ) { return new pets::fallen_monk_ww_pet_t( p ); } ),
    fallen_monk_mw( "fallen_monk_mistweaver", p, []( monk_t* p ) { return new pets::fallen_monk_mw_pet_t( p ); } ),
    fallen_monk_brm( "fallen_monk_brewmaster", p, []( monk_t* p ) { return new pets::fallen_monk_brm_pet_t( p ); } )
{
}

// monk_t::create_pets ======================================================

void monk_t::create_pets()
{
  base_t::create_pets();

  if ( spec.invoke_xuen->ok() && ( find_action( "invoke_xuen" ) || find_action( "invoke_xuen_the_white_tiger" ) ) )
  {
    pets.xuen = new pets::xuen_pet_t( this );
  }

  if ( spec.invoke_niuzao->ok() && ( find_action( "invoke_niuzao" ) || find_action( "invoke_niuzao_the_black_ox" ) ) )
  {
    pets.niuzao = new pets::niuzao_pet_t( this );
  }

  if ( talent.invoke_chi_ji->ok() && ( find_action( "invoke_chiji" ) || find_action( "invoke_chiji_the_red_crane" ) ) )
  {
    pets.chiji = new pets::chiji_pet_t( this );
  }

  if ( spec.invoke_yulon->ok() && !talent.invoke_chi_ji->ok() &&
       ( find_action( "invoke_yulon" ) || find_action( "invoke_yulon_the_jade_serpent" ) ) )
  {
    pets.yulon = new pets::yulon_pet_t( this );
  }

  if ( specialization() == MONK_WINDWALKER && find_action( "storm_earth_and_fire" ) )
  {
    pets.sef[ SEF_FIRE ] = new pets::storm_earth_and_fire_pet_t( "fire_spirit", this, true, WEAPON_SWORD );
    // The player BECOMES the Storm Spirit
    // SEF EARTH was changed from 2-handed user to dual welding in Legion
    pets.sef[ SEF_EARTH ] = new pets::storm_earth_and_fire_pet_t( "earth_spirit", this, true, WEAPON_MACE );
  }
}

void monk_t::trigger_storm_earth_and_fire( const action_t* a, sef_ability_e sef_ability )
{
  if ( !spec.storm_earth_and_fire->ok() )
  {
    return;
  }

  if ( sef_ability == SEF_NONE )
  {
    return;
  }

  if ( !buff.storm_earth_and_fire->up() )
  {
    return;
  }

  pets.sef[ SEF_EARTH ]->trigger_attack( sef_ability, a );
  pets.sef[ SEF_FIRE ]->trigger_attack( sef_ability, a );
  // Trigger pet retargeting if sticky target is not defined, and the Monk used one of the Cyclone
  // Strike triggering abilities
  if ( !pets.sef[ SEF_EARTH ]->sticky_target &&
       ( sef_ability == SEF_TIGER_PALM || sef_ability == SEF_BLACKOUT_KICK || sef_ability == SEF_RISING_SUN_KICK ) )
  {
    retarget_storm_earth_and_fire_pets();
  }
}

void monk_t::storm_earth_and_fire_fixate( player_t* target )
{
  sim->print_debug( "{} storm_earth_and_fire sticky target {} to {} (old={})", *this, *pets.sef[ SEF_EARTH ], *target,
                    *pets.sef[ SEF_EARTH ]->target );

  pets.sef[ SEF_EARTH ]->target        = target;
  pets.sef[ SEF_EARTH ]->sticky_target = true;

  sim->print_debug( "{} storm_earth_and_fire sticky target {} to {} (old={})", *this, *pets.sef[ SEF_FIRE ], *target,
                    *pets.sef[ SEF_FIRE ]->target );

  pets.sef[ SEF_FIRE ]->target        = target;
  pets.sef[ SEF_FIRE ]->sticky_target = true;
}

bool monk_t::storm_earth_and_fire_fixate_ready( player_t* target )
{
  if ( buff.storm_earth_and_fire->check() )
  {
    if ( pets.sef[ SEF_EARTH ]->sticky_target || pets.sef[ SEF_FIRE ]->sticky_target )
    {
      if ( pets.sef[ SEF_EARTH ]->target != target )
        return true;
      else if ( pets.sef[ SEF_FIRE ]->target != target )
        return true;
    }
    else if ( !pets.sef[ SEF_EARTH ]->sticky_target || !pets.sef[ SEF_FIRE ]->sticky_target )
      return true;
  }
  return false;
}

// monk_t::summon_storm_earth_and_fire ================================================

void monk_t::summon_storm_earth_and_fire( timespan_t duration )
{
  auto targets   = create_storm_earth_and_fire_target_list();
  auto n_targets = targets.size();

  auto summon_sef_pet = [ & ]( pets::storm_earth_and_fire_pet_t* pet ) {
    // Start targeting logic from "owner" always
    pet->reset_targeting();
    pet->target        = target;
    pet->sticky_target = false;
    retarget_storm_earth_and_fire( pet, targets, n_targets );
    pet->summon( duration );
  };

  summon_sef_pet( pets.sef[ SEF_EARTH ] );
  summon_sef_pet( pets.sef[ SEF_FIRE ] );
}

// monk_t::retarget_storm_earth_and_fire_pets =======================================

void monk_t::retarget_storm_earth_and_fire_pets() const
{
  if ( pets.sef[ SEF_EARTH ]->sticky_target )
  {
    return;
  }

  auto targets   = create_storm_earth_and_fire_target_list();
  auto n_targets = targets.size();
  retarget_storm_earth_and_fire( pets.sef[ SEF_EARTH ], targets, n_targets );
  retarget_storm_earth_and_fire( pets.sef[ SEF_FIRE ], targets, n_targets );
}

// Callback to retarget Storm Earth and Fire pets when new target appear, or old targets depsawn
// (i.e., die).
void sef_despawn_cb_t::operator()( player_t* )
{
  // No pets up, don't do anything
  if ( !monk->buff.storm_earth_and_fire->check() )
  {
    return;
  }

  auto targets   = monk->create_storm_earth_and_fire_target_list();
  auto n_targets = targets.size();

  // If the active clone's target is sleeping, reset it's targeting, and jump it to a new target.
  // Note that if sticky targeting is used, both targets will jump (since both are going to be
  // stickied to the dead target)
  range::for_each( monk->pets.sef, [ this, &targets, &n_targets ]( pets::storm_earth_and_fire_pet_t* pet ) {
    // Arise time went negative, so the target is sleeping. Can't check "is_sleeping" here, because
    // the callback is called before the target goes to sleep.
    if ( pet->target->arise_time < timespan_t::zero() )
    {
      pet->reset_targeting();
      monk->retarget_storm_earth_and_fire( pet, targets, n_targets );
    }
    else
    {
      // Retarget pets otherwise (a new target has appeared). Note that if the pets are sticky
      // targeted, this will do nothing.
      monk->retarget_storm_earth_and_fire( pet, targets, n_targets );
    }
  } );
}

void monk_t::trigger_storm_earth_and_fire_bok_proc( sef_pet_e sef_pet )
{
  pets.sef[ sef_pet ]->buff.bok_proc_sef->trigger();
}

player_t* monk_t::storm_earth_and_fire_fixate_target( sef_pet_e sef_pet )
{
  if ( pets.sef[ sef_pet ]->sticky_target )
  {
    return pets.sef[ sef_pet ]->target;
  }

  return nullptr;
}
}  // namespace monk