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
  monk_pet_t( monk_t *owner, util::string_view name, pet_e pet_type, bool guardian, bool dynamic )
    : pet_t( owner->sim, owner, name, pet_type, guardian, dynamic )
  {
  }

  monk_t *o()
  {
    return static_cast<monk_t *>( owner );
  }

  const monk_t *o() const
  {
    return static_cast<monk_t *>( owner );
  }

  void init_assessors() override
  {
    base_t::init_assessors();
  }
};

template <typename BASE, typename PET_TYPE = monk_pet_t>
struct pet_action_base_t : public BASE
{
  using super_t = BASE;
  using base_t  = pet_action_base_t<BASE>;
  bool merge_report;

  pet_action_base_t( util::string_view n, PET_TYPE *p, const spell_data_t *data = spell_data_t::nil() )
    : BASE( n, p, data ), merge_report( true )
  {
    // No costs are needed either
    super_t::base_costs[ RESOURCE_ENERGY ]          = 0;
    super_t::base_costs[ RESOURCE_CHI ]             = 0;
    super_t::base_costs[ RESOURCE_MANA ]            = 0;
    super_t::base_costs_per_tick[ RESOURCE_ENERGY ] = 0;
    super_t::base_costs_per_tick[ RESOURCE_CHI ]    = 0;
    super_t::base_costs_per_tick[ RESOURCE_MANA ]   = 0;
  }

  void init() override
  {
    if ( !this->player->sim->report_pets_separately && merge_report )
    {
      auto it =
          range::find_if( o()->pet_list, [ this ]( pet_t *pet ) { return this->player->name_str == pet->name_str; } );

      if ( it != o()->pet_list.end() && this->player != *it )
      {
        this->stats = ( *it )->get_stats( this->name(), this );
      }
    }

    super_t::init();
  }

  monk_t *o()
  {
    return p()->o();
  }

  const monk_t *o() const
  {
    return p()->o();
  }

  const PET_TYPE *p() const
  {
    return debug_cast<const PET_TYPE *>( this->player );
  }

  PET_TYPE *p()
  {
    return debug_cast<PET_TYPE *>( this->player );
  }

  void impact( action_state_t *s ) override
  {
    super_t::impact( s );

    if ( s->result_type != result_amount_type::DMG_DIRECT && s->result_type != result_amount_type::DMG_OVER_TIME )
      return;

    o()->trigger_empowered_tiger_lightning( s );

    if ( super_t::result_is_miss( s->result ) || s->result_amount <= 0.0 )
      return;

    if ( o()->get_target_data( s->target )->debuff.gale_force->check() &&
         o()->rng().roll( o()->get_target_data( s->target )->debuff.gale_force->default_chance ) )
    {
      double amount =
          s->result_amount * o()->get_target_data( s->target )->debuff.gale_force->data().effectN( 1 ).percent();
      o()->active_actions.gale_force->base_dd_min = o()->active_actions.gale_force->base_dd_max = amount;
      o()->active_actions.gale_force->execute_on_target( s->target );
    }
  }

  void tick( dot_t *dot ) override
  {
    super_t::tick( dot );

    if ( !super_t::result_is_miss( dot->state->result ) &&
         dot->state->result_type == result_amount_type::DMG_OVER_TIME )
      o()->trigger_empowered_tiger_lightning( dot->state );
  }
};

// ==========================================================================
// Base Monk Pet Melee Attack
// ==========================================================================

struct pet_melee_attack_t : public pet_action_base_t<melee_attack_t>
{
  bool trigger_mystic_touch;  // Some pets can trigger Mystic Touch debuff from attacks

  pet_melee_attack_t( util::string_view n, monk_pet_t *p, const spell_data_t *data = spell_data_t::nil() )
    : base_t( n, p, data ), trigger_mystic_touch( false )
  {
    //   base_t::apply_affecting_aura( p->o()->passives.aura_monk );

    //   switch ( p->o()->specialization() )
    //   {
    //     case MONK_WINDWALKER:
    //       base_t::apply_affecting_aura( p->o()->spec.windwalker_monk );
    //       break;

    //     case MONK_BREWMASTER:
    //       base_t::apply_affecting_aura( p->o()->spec.brewmaster_monk );
    //       break;

    //     case MONK_MISTWEAVER:
    //       base_t::apply_affecting_aura( p->o()->baseline.mistweaver.aura );
    //       break;

    //     default:
    //       assert( 0 );
    //       break;
    //   }

    //   if ( p->o()->main_hand_weapon.group() == weapon_e::WEAPON_1H )
    //   {
    //     switch ( p->o()->specialization() )
    //     {
    //       case MONK_BREWMASTER:
    //         base_t::apply_affecting_aura( p->o()->spec.two_hand_adjustment_brm );
    //         break;
    //       case MONK_WINDWALKER:
    //         base_t::apply_affecting_aura( p->o()->spec.two_hand_adjustment_ww );
    //         break;
    //       default:
    //         assert( 0 );
    //         break;
    //     }
    //   }
  }

  // Physical tick_action abilities need amount_type() override, so the
  // tick_action multistrikes are properly physically mitigated.
  result_amount_type amount_type( const action_state_t *state, bool periodic ) const override
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

  void impact( action_state_t *s ) override
  {
    base_t::impact( s );

    if ( trigger_mystic_touch )
      s->target->debuffs.mystic_touch->trigger();
  }
};

struct pet_melee_t : pet_melee_attack_t
{
  pet_melee_t( util::string_view name, monk_pet_t *player, weapon_t *weapon )
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
  bool trigger_mystic_touch;  // Some pets can trigger Mystic Touch debuff from attacks

  pet_auto_attack_t( monk_pet_t *player ) : melee_attack_t( "auto_attack", player ), trigger_mystic_touch( false )
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

  void impact( action_state_t *s ) override
  {
    melee_attack_t::impact( s );

    if ( trigger_mystic_touch )
      s->target->debuffs.mystic_touch->trigger();
  }
};

// ==========================================================================
// Base Monk Pet Spell
// ==========================================================================

struct pet_spell_t : public pet_action_base_t<spell_t>
{
  pet_spell_t( util::string_view n, monk_pet_t *p, const spell_data_t *data = spell_data_t::nil() )
    : base_t( n, p, data )
  {
  }
};

// ==========================================================================
// Base Monk Heal Spell
// ==========================================================================

struct pet_heal_t : public pet_action_base_t<heal_t>
{
  pet_heal_t( util::string_view n, monk_pet_t *p, const spell_data_t *data = spell_data_t::nil() )
    : base_t( n, p, data )
  {
  }
};

// ==========================================================================
// Base Monk Absorb Spell
// ==========================================================================

struct pet_absorb_t : public pet_action_base_t<absorb_t>
{
  pet_absorb_t( util::string_view n, monk_pet_t *p, const spell_data_t *data = spell_data_t::nil() )
    : base_t( n, p, data )
  {
  }
};

namespace buffs
{
// ==========================================================================
// Monk Buffs
// ==========================================================================

template <typename buff_t>
struct monk_pet_buff_t : public buff_t
{
public:
  using base_t = monk_pet_buff_t;

  monk_pet_buff_t( monk_pet_t &p, util::string_view name, const spell_data_t *s = spell_data_t::nil(),
                   const item_t *item = nullptr )
    : buff_t( &p, name, s, item )
  {
  }

  monk_pet_t &p()
  {
    return *debug_cast<monk_pet_t *>( buff_t::source );
  }

  const monk_pet_t &p() const
  {
    return *debug_cast<monk_pet_t *>( buff_t::source );
  }

  const monk_t &o()
  {
    return p().o();
  };
};
}  // namespace buffs

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

    const action_t *source_action;

    sef_action_base_t( util::string_view n, storm_earth_and_fire_pet_t *p,
                       const spell_data_t *data = spell_data_t::nil() )
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
      if ( !source_action )
        for ( const action_t *a : this->o()->action_list )
          if ( ( this->id > 0 && this->id == a->id ) || util::str_compare_ci( this->name_str, a->name_str ) )
          {
            source_action = a;
            break;
          }

      if ( source_action )
      {
        this->update_flags           = source_action->update_flags;
        auto pet_multiplier_snapshot = this->snapshot_flags & STATE_MUL_PET;
        this->snapshot_flags         = source_action->snapshot_flags | pet_multiplier_snapshot;
      }
    }

    void snapshot_internal( action_state_t *state, unsigned flags, result_amount_type rt ) override
    {
      assert( state );
      assert( source_action );

      state->result_type = rt;

      // Note that attack power is not included here, as we will want to (just in case) snapshot
      // AP through the pet's own AP system. This allows us to override the
      // inheritance coefficient if need be in an easy way.

      if ( flags & STATE_AP )
        state->attack_power = this->composite_total_attack_power();

      // Map the rest of the relevant state-related stuff into the source
      // action's methods. In other words, use the owner's data.

      if ( flags & STATE_CRIT )
        state->crit_chance = source_action->composite_crit_chance() * source_action->composite_crit_chance_multiplier();

      if ( flags & STATE_HASTE )
        state->haste = source_action->composite_haste();

      if ( flags & STATE_SP )
        state->spell_power = source_action->composite_total_spell_power();

      if ( flags & STATE_VERSATILITY )
        state->versatility = source_action->composite_versatility( state );

      if ( flags & STATE_MUL_SPELL_DA )
        state->da_multiplier = source_action->composite_da_multiplier( state );

      if ( flags & STATE_MUL_SPELL_TA )
        state->ta_multiplier = source_action->composite_ta_multiplier( state );

      if ( flags & STATE_ROLLING_TA )
        state->rolling_ta_multiplier = source_action->composite_rolling_ta_multiplier( state );

      if ( flags & STATE_MUL_PLAYER_DAM )
        state->player_multiplier = source_action->composite_player_multiplier( state );

      if ( flags & STATE_MUL_PERSISTENT )
        state->persistent_multiplier = source_action->composite_persistent_multiplier( state );

      if ( flags & STATE_TGT_MUL_DA )
        state->target_da_multiplier = source_action->composite_target_da_multiplier( state->target );

      if ( flags & STATE_TGT_MUL_TA )
        state->target_ta_multiplier = source_action->composite_target_ta_multiplier( state->target );

      if ( flags & STATE_TGT_CRIT )
        state->target_crit_chance = source_action->composite_target_crit_chance( state->target ) *
                                    source_action->composite_crit_chance_multiplier();

      if ( flags & STATE_TGT_MITG_DA )
        state->target_mitigation_da_multiplier =
            source_action->composite_target_mitigation( state->target, source_action->get_school() );

      if ( flags & STATE_TGT_MITG_TA )
        state->target_mitigation_ta_multiplier =
            source_action->composite_target_mitigation( state->target, source_action->get_school() );

      if ( flags & STATE_TGT_ARMOR )
        state->target_armor = source_action->composite_target_armor( state->target );
    }

    // added so cjl channel time is sane, mults are correct and extra ticks don't get added
    timespan_t composite_dot_duration( const action_state_t *state ) const override
    {
      return source_action->composite_dot_duration( state );
    }

    // added so cjl channel time is sane, mults are correct and extra ticks don't get added
    timespan_t tick_time( const action_state_t *state ) const override
    {
      return source_action->tick_time( state );
    }

    double total_crit_bonus( const action_state_t *state ) const override
    {
      return source_action->total_crit_bonus( state );
    }

    timespan_t travel_time() const override
    {
      return source_action->travel_time();
    }

    void execute() override
    {
      if ( !source_action->background )
        super_t::target = super_t::player->target;

      super_t::execute();
    }
  };

  struct sef_melee_attack_t : public sef_action_base_t<melee_attack_t>
  {
    sef_melee_attack_t( util::string_view n, storm_earth_and_fire_pet_t *p,
                        const spell_data_t *data = spell_data_t::nil() )
      : base_t( n, p, data )
    {
      school = SCHOOL_PHYSICAL;
    }

    // Physical tick_action abilities need amount_type() override, so the
    // tick_action multistrikes are properly physically mitigated.
    result_amount_type amount_type( const action_state_t *state, bool periodic ) const override
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
    sef_spell_t( util::string_view n, storm_earth_and_fire_pet_t *p, const spell_data_t *data = spell_data_t::nil() )
      : base_t( n, p, data )
    {
    }
  };

  // Auto attack ============================================================

  struct melee_t : public sef_melee_attack_t
  {
    melee_t( util::string_view n, storm_earth_and_fire_pet_t *player, weapon_t *w )
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
    double composite_total_attack_power() const override
    {
      double ap = 0;

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

      return ap * o()->composite_attack_power_multiplier() + sef_melee_attack_t::composite_total_attack_power();
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
    auto_attack_t( storm_earth_and_fire_pet_t *player, util::string_view options_str ) : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      auto *mh = new melee_t( "auto_attack_mh", player, &( player->main_hand_weapon ) );
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
    sef_tiger_palm_t( storm_earth_and_fire_pet_t *player )
      : sef_melee_attack_t( "tiger_palm", player, player->o()->baseline.monk.tiger_palm )
    {
    }

    double action_multiplier() const override
    {
      double am = sef_melee_attack_t::action_multiplier();

      am /= 1.0 + p()->o()->buff.tigers_ferocity->check_stack_value();

      return am;
    }
  };

  struct sef_blackout_kick_totm_proc_t : public sef_melee_attack_t
  {
    sef_blackout_kick_totm_proc_t( storm_earth_and_fire_pet_t *player )
      : sef_melee_attack_t( "blackout_kick_totm_proc", player,
                            player->o()->talent.windwalker.teachings_of_the_monastery_blackout_kick )
    {
      background = dual = true;
      trigger_gcd       = timespan_t::zero();
    }
  };

  struct sef_blackout_kick_t : public sef_melee_attack_t
  {
    sef_blackout_kick_totm_proc_t *bok_totm_proc;

    sef_blackout_kick_t( storm_earth_and_fire_pet_t *player )
      : sef_melee_attack_t( "blackout_kick", player, player->o()->baseline.monk.blackout_kick )
    {
      aoe = 1 + (int)o()->talent.windwalker.shadowboxing_treads->effectN( 1 ).base_value();

      if ( player->o()->talent.windwalker.teachings_of_the_monastery->ok() )
      {
        bok_totm_proc = new sef_blackout_kick_totm_proc_t( player );

        add_child( bok_totm_proc );
      }
    }

    double action_multiplier() const override
    {
      double am = sef_melee_attack_t::action_multiplier();

      if ( p()->o()->talent.windwalker.courageous_impulse->ok() && p()->o()->buff.bok_proc->check() )
        am /= 1.0 + p()->o()->talent.windwalker.courageous_impulse->effectN( 1 ).percent();

      return am;
    }
  };

  struct sef_glory_of_the_dawn_t : public sef_melee_attack_t
  {
    sef_glory_of_the_dawn_t( storm_earth_and_fire_pet_t *player )
      : sef_melee_attack_t( "glory_of_the_dawn", player, player->o()->passives.glory_of_the_dawn_damage )
    {
      background  = true;
      trigger_gcd = timespan_t::zero();
    }
  };

  struct sef_rising_sun_kick_dmg_t : public sef_melee_attack_t
  {
    sef_glory_of_the_dawn_t *glory_of_the_dawn;

    sef_rising_sun_kick_dmg_t( storm_earth_and_fire_pet_t *player )
      : sef_melee_attack_t( "rising_sun_kick_dmg", player,
                            player->o()->talent.monk.rising_sun_kick->effectN( 1 ).trigger() )
    {
      background = true;

      if ( player->o()->talent.windwalker.glory_of_the_dawn->ok() )
      {
        glory_of_the_dawn = new sef_glory_of_the_dawn_t( player );

        add_child( glory_of_the_dawn );
      }
    }

    void impact( action_state_t *state ) override
    {
      sef_melee_attack_t::impact( state );

      if ( o()->baseline.windwalker.combat_conditioning->ok() )
        state->target->debuffs.mortal_wounds->trigger();
    }
  };

  struct sef_rising_sun_kick_t : public sef_melee_attack_t
  {
    sef_rising_sun_kick_t( storm_earth_and_fire_pet_t *player )
      : sef_melee_attack_t( "rising_sun_kick", player, player->o()->talent.monk.rising_sun_kick )
    {
      execute_action = new sef_rising_sun_kick_dmg_t( player );
    }
  };

  struct sef_tick_action_t : public sef_melee_attack_t
  {
    sef_tick_action_t( util::string_view name, storm_earth_and_fire_pet_t *p, const spell_data_t *data )
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
    sef_fists_of_fury_tick_t( storm_earth_and_fire_pet_t *p )
      : sef_tick_action_t( "fists_of_fury_tick", p, p->o()->passives.fists_of_fury_tick )
    {
      aoe                 = -1;
      reduced_aoe_targets = p->o()->talent.windwalker.fists_of_fury->effectN( 1 ).base_value();
      full_amount_targets = 1;
      base_dd_min = base_dd_max = 1.0;  // parse state flags

      dot_duration = timespan_t::zero();
      trigger_gcd  = timespan_t::zero();
    }
  };

  struct sef_fists_of_fury_t : public sef_melee_attack_t
  {
    sef_fists_of_fury_t( storm_earth_and_fire_pet_t *player )
      : sef_melee_attack_t( "fists_of_fury", player, player->o()->talent.windwalker.fists_of_fury )
    {
      channeled = tick_zero = interrupt_auto_attack = true;
      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;
      // Hard code a 10% reduced cast time to not cause any clipping issues.
      // Obtained from logs as of 2022-04-05
      dot_duration = data().duration() / 1.1;
      // Effect 1 shows a period of 166 milliseconds which appears to refer to the visual and not the tick period
      base_tick_time = ( dot_duration / 4 );

      attack_power_mod.direct = 0;
      weapon_power_mod        = 0;

      tick_action = new sef_fists_of_fury_tick_t( player );
    }
  };

  struct sef_chi_explosion_t : public sef_spell_t
  {
    sef_chi_explosion_t( storm_earth_and_fire_pet_t *player )
      : sef_spell_t( "chi_explosion", player, player->o()->passives.chi_explosion )
    {
      dual = background = true;
      aoe               = -1;

      source_action = player->owner->find_action( "chi_explosion" );
    }
  };

  struct sef_spinning_crane_kick_tick_t : public sef_tick_action_t
  {
    sef_spinning_crane_kick_tick_t( storm_earth_and_fire_pet_t *p )
      : sef_tick_action_t( "spinning_crane_kick_tick", p,
                           p->o()->baseline.monk.spinning_crane_kick->effectN( 1 ).trigger() )
    {
      aoe = as<int>( p->o()->baseline.monk.spinning_crane_kick->effectN( 1 ).base_value() );
    }
  };

  struct sef_spinning_crane_kick_t : public sef_melee_attack_t
  {
    sef_chi_explosion_t *chi_explosion;
    sef_spinning_crane_kick_t( storm_earth_and_fire_pet_t *player )
      : sef_melee_attack_t( "spinning_crane_kick", player, player->o()->baseline.monk.spinning_crane_kick ),
        chi_explosion( nullptr )
    {
      tick_zero = hasted_ticks = interrupt_auto_attack = true;
      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      weapon_power_mod = 0;

      tick_action = new sef_spinning_crane_kick_tick_t( player );

      if ( player->o()->talent.windwalker.jade_ignition->ok() )
        chi_explosion = new sef_chi_explosion_t( player );
    }

    void execute() override
    {
      sef_melee_attack_t::execute();

      if ( chi_explosion && o()->buff.chi_energy->up() )
      {
        chi_explosion->set_target( execute_state->target );
        chi_explosion->execute();
      }
    }
  };

  struct sef_rushing_jade_wind_tick_t : public sef_tick_action_t
  {
    sef_rushing_jade_wind_tick_t( storm_earth_and_fire_pet_t *p )
      : sef_tick_action_t( "rushing_jade_wind_tick", p, p->o()->passives.rushing_jade_wind_tick )
    {
      aoe                 = -1;
      reduced_aoe_targets = p->o()->passives.rushing_jade_wind->effectN( 1 ).base_value();
    }
  };

  struct sef_whirling_dragon_punch_aoe_tick_t : public sef_tick_action_t
  {
    timespan_t delay;

    sef_whirling_dragon_punch_aoe_tick_t( storm_earth_and_fire_pet_t *p, timespan_t delay )
      : sef_tick_action_t( "whirling_dragon_punch_aoe_tick", p, p->o()->passives.whirling_dragon_punch_aoe_tick ),
        delay( delay )
    {
      aoe                 = -1;
      reduced_aoe_targets = p->o()->talent.windwalker.whirling_dragon_punch->effectN( 1 ).base_value();
      name_str_reporting  = "sef_wdp_aoe";
    }
  };

  struct sef_whirling_dragon_punch_st_tick_t : public sef_tick_action_t
  {
    timespan_t delay;

    sef_whirling_dragon_punch_st_tick_t( storm_earth_and_fire_pet_t *p )
      : sef_tick_action_t( "whirling_dragon_punch_st_tick", p, p->o()->passives.whirling_dragon_punch_st_tick )
    {
      name_str_reporting = "sef_wdp_st";
    }
  };

  struct sef_whirling_dragon_punch_t : public sef_melee_attack_t
  {
    std::array<sef_whirling_dragon_punch_aoe_tick_t *, 3> aoe_ticks;

    sef_whirling_dragon_punch_st_tick_t *st_tick;

    struct sef_whirling_dragon_punch_tick_event_t : public event_t
    {
      sef_whirling_dragon_punch_aoe_tick_t *tick;

      sef_whirling_dragon_punch_tick_event_t( sef_whirling_dragon_punch_aoe_tick_t *tick, timespan_t delay )
        : event_t( *tick->player, delay ), tick( tick )
      {
      }

      void execute() override
      {
        tick->execute();
      }
    };

    sef_whirling_dragon_punch_t( storm_earth_and_fire_pet_t *player )
      : sef_melee_attack_t( "whirling_dragon_punch", player, player->o()->talent.windwalker.whirling_dragon_punch )
    {
      channeled = false;

      may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

      weapon_power_mod = 0;

      for ( size_t i = 0; i < aoe_ticks.size(); ++i )
      {
        auto delay     = base_tick_time * i;
        aoe_ticks[ i ] = new sef_whirling_dragon_punch_aoe_tick_t( player, delay );

        add_child( aoe_ticks[ i ] );
      }

      st_tick = new sef_whirling_dragon_punch_st_tick_t( player );

      add_child( st_tick );
    }

    void execute() override
    {
      sef_melee_attack_t::execute();

      for ( auto &tick : aoe_ticks )
        make_event<sef_whirling_dragon_punch_tick_event_t>( *sim, tick, tick->delay );

      st_tick->execute();
    }
  };

  struct sef_strike_of_the_windlord_oh_t : public sef_melee_attack_t
  {
    sef_strike_of_the_windlord_oh_t( storm_earth_and_fire_pet_t *player )
      : sef_melee_attack_t( "strike_of_the_windlord_offhand", player,
                            player->o()->talent.windwalker.strike_of_the_windlord->effectN( 4 ).trigger() )
    {
      may_dodge = may_parry = may_block = may_miss = true;
      dual                                         = true;
      aoe                                          = -1;

      energize_type = action_energize::NONE;
    }

    // Damage must be divided on non-main target by the number of targets
    double composite_aoe_multiplier( const action_state_t *state ) const override
    {
      if ( state->target != target )
      {
        return 1.0 / state->n_targets;
      }

      return 1.0;
    }

    void execute() override
    {
      sef_melee_attack_t::execute();
    }
  };

  struct sef_strike_of_the_windlord_t : public sef_melee_attack_t
  {
    sef_strike_of_the_windlord_t( storm_earth_and_fire_pet_t *player )
      : sef_melee_attack_t( "strike_of_the_windlord_mainhand", player,
                            player->o()->talent.windwalker.strike_of_the_windlord->effectN( 3 ).trigger() )
    {
      may_dodge = may_parry = may_block = may_miss = true;
      dual                                         = true;
      aoe                                          = -1;
    }

    // Damage must be divided on non-main target by the number of targets
    double composite_aoe_multiplier( const action_state_t *state ) const override
    {
      if ( state->target != target )
      {
        return 1.0 / state->n_targets;
      }

      return 1.0;
    }

    void execute() override
    {
      sef_melee_attack_t::execute();
    }
  };

  // SEF Chi Wave skips the healing ticks, delivering damage on every second
  // tick of the ability for simplicity.
  struct sef_chi_wave_damage_t : public sef_spell_t
  {
    sef_chi_wave_damage_t( storm_earth_and_fire_pet_t *player )
      : sef_spell_t( "chi_wave_damage", player, player->o()->talent.monk.chi_wave_damage )
    {
      background = dual = true;
    }
  };

  struct sef_crackling_jade_lightning_t : public sef_spell_t
  {
    struct sef_crackling_jade_lightning_aoe_t : public sef_spell_t
    {
      sef_crackling_jade_lightning_aoe_t( storm_earth_and_fire_pet_t *player )
        : sef_spell_t( "crackling_jade_lightning_aoe", player, player->o()->baseline.monk.crackling_jade_lightning )
      {
        dual = background = true;
      }

      void init() override
      {
        // has same id as standard cjl, so we need to look up the correct action
        // independently of auto action resolution
        source_action = o()->find_action( "crackling_jade_lightning_aoe" );

        sef_spell_t::init();
      }
    };

    sef_crackling_jade_lightning_t( storm_earth_and_fire_pet_t *player )
      : sef_spell_t( "crackling_jade_lightning", player, player->o()->baseline.monk.crackling_jade_lightning )
    {
      interrupt_auto_attack = true;
      channeled             = true;
    }
  };

  struct sef_celestial_conduit_tick_t : public sef_tick_action_t
  {
    sef_celestial_conduit_tick_t( storm_earth_and_fire_pet_t *p )
      : sef_tick_action_t( "celestial_conduit_tick", p, p->o()->talent.conduit_of_the_celestials.celestial_conduit_dmg )
    {
      base_dd_min = base_dd_max = 1.0;  // parse state flags
      split_aoe_damage          = true;

      dot_duration = timespan_t::zero();
      trigger_gcd  = timespan_t::zero();
    }
  };

  struct sef_celestial_conduit_t : public sef_melee_attack_t
  {
    sef_celestial_conduit_t( storm_earth_and_fire_pet_t *player )
      : sef_melee_attack_t( "celestial_conduit", player,
                            player->o()->talent.conduit_of_the_celestials.celestial_conduit )
    {
      channeled = tick_zero = interrupt_auto_attack = true;

      attack_power_mod.direct = 0;
      weapon_power_mod        = 0;

      tick_action = new sef_celestial_conduit_tick_t( player );
    }
  };

  // Storm, Earth, and Fire abilities end ===================================

  std::map<actions::sef_ability_e, action_t *> actions;

public:
  bool sticky_target;  // When enabled, SEF pets will stick to the target they have

  storm_earth_and_fire_pet_t( util::string_view name, monk_t *owner, bool dual_wield, weapon_e weapon_type )
    : monk_pet_t( owner, name, PET_NONE, true, true ), actions(), sticky_target( false )
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

    actions.emplace( std::make_pair( actions::sef_ability_e::SEF_TIGER_PALM, new sef_tiger_palm_t( this ) ) );
    actions.emplace( std::make_pair( actions::sef_ability_e::SEF_TIGER_PALM, new sef_tiger_palm_t( this ) ) );
    actions.emplace( std::make_pair( actions::sef_ability_e::SEF_BLACKOUT_KICK, new sef_blackout_kick_t( this ) ) );
    actions.emplace(
        std::make_pair( actions::sef_ability_e::SEF_BLACKOUT_KICK_TOTM, new sef_blackout_kick_totm_proc_t( this ) ) );
    actions.emplace( std::make_pair( actions::sef_ability_e::SEF_RISING_SUN_KICK, new sef_rising_sun_kick_t( this ) ) );
    actions.emplace(
        std::make_pair( actions::sef_ability_e::SEF_GLORY_OF_THE_DAWN, new sef_glory_of_the_dawn_t( this ) ) );
    actions.emplace( std::make_pair( actions::sef_ability_e::SEF_FISTS_OF_FURY, new sef_fists_of_fury_t( this ) ) );
    actions.emplace(
        std::make_pair( actions::sef_ability_e::SEF_SPINNING_CRANE_KICK, new sef_spinning_crane_kick_t( this ) ) );
    actions.emplace(
        std::make_pair( actions::sef_ability_e::SEF_WHIRLING_DRAGON_PUNCH, new sef_whirling_dragon_punch_t( this ) ) );
    actions.emplace( std::make_pair( actions::sef_ability_e::SEF_STRIKE_OF_THE_WINDLORD,
                                     new sef_strike_of_the_windlord_t( this ) ) );
    actions.emplace( std::make_pair( actions::sef_ability_e::SEF_STRIKE_OF_THE_WINDLORD_OH,
                                     new sef_strike_of_the_windlord_oh_t( this ) ) );
    actions.emplace(
        std::make_pair( actions::sef_ability_e::SEF_CELESTIAL_CONDUIT, new sef_celestial_conduit_t( this ) ) );
    actions.emplace( std::make_pair( actions::sef_ability_e::SEF_RJW_TICK, new sef_rushing_jade_wind_tick_t( this ) ) );
    actions.emplace( std::make_pair( actions::sef_ability_e::SEF_CHI_WAVE, new sef_chi_wave_damage_t( this ) ) );
    actions.emplace( std::make_pair( actions::sef_ability_e::SEF_CRACKLING_JADE_LIGHTNING,
                                     new sef_crackling_jade_lightning_t( this ) ) );
    actions.emplace( std::make_pair( actions::sef_ability_e::SEF_CRACKLING_JADE_LIGHTNING_AOE,
                                     new sef_crackling_jade_lightning_t::sef_crackling_jade_lightning_aoe_t( this ) ) );

    if ( o()->talent.windwalker.power_of_the_thunder_king->ok() )
      actions[ actions::sef_ability_e::SEF_CRACKLING_JADE_LIGHTNING ]->add_child(
          actions[ actions::sef_ability_e::SEF_CRACKLING_JADE_LIGHTNING_AOE ] );
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack";

    monk_pet_t::init_action_list();
  }

  action_t *create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return monk_pet_t::create_action( name, options_str );
  }

  void summon( timespan_t duration = timespan_t::zero() ) override
  {
    monk_pet_t::summon( duration );

    o()->buff.storm_earth_and_fire->trigger( 1, buff_t::DEFAULT_VALUE(), 1, duration );

    sticky_target = false;
  }

  void dismiss( bool expired = false ) override
  {
    monk_pet_t::dismiss( expired );

    o()->buff.storm_earth_and_fire->decrement();
  }

  void trigger_attack( actions::sef_ability_e ability, const action_t *source_action, bool combo_strike = false )
  {
    if ( channeling && !source_action->background )
      channeling->cancel();

    action_t *action = actions[ ability ];

    if ( source_action->background )
      action->set_target( source_action->target );

    action->execute();
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
    melee_t( util::string_view n, xuen_pet_t *player, weapon_t *weapon ) : pet_melee_t( n, player, weapon )
    {
    }
  };

  struct crackling_tiger_lightning_tick_t : public pet_spell_t
  {
    crackling_tiger_lightning_tick_t( xuen_pet_t *p )
      : pet_spell_t( "crackling_tiger_lightning_tick", p, p->o()->passives.crackling_tiger_lightning )
    {
      background   = true;
      merge_report = false;
    }
  };

  struct crackling_tiger_lightning_t : public pet_spell_t
  {
    crackling_tiger_lightning_t( xuen_pet_t *p, util::string_view options_str )
      : pet_spell_t( "crackling_tiger_lightning", p, p->o()->passives.crackling_tiger_lightning_driver )
    {
      parse_options( options_str );
      s_data_reporting = p->o()->passives.crackling_tiger_lightning;

      dot_duration = p->o()->talent.windwalker.invoke_xuen_the_white_tiger->duration();
      cooldown->duration =
          p->o()->talent.windwalker.invoke_xuen_the_white_tiger->duration();  // we're done when Xuen despawns

      tick_action = new crackling_tiger_lightning_tick_t( p );
    }

    double last_tick_factor( const dot_t *, timespan_t, timespan_t ) const
    {
      return 0.0;
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( xuen_pet_t *player, util::string_view options_str ) : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      player->main_hand_attack = new melee_t( "melee_main_hand", player, &( player->main_hand_weapon ) );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    }
  };

public:
  xuen_pet_t( monk_t *owner ) : monk_pet_t( owner, "xuen_the_white_tiger", PET_XUEN, false, true )
  {
    npc_id                      = (int)o()->find_spell( 123904 )->effectN( 1 ).misc_value1();
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );
    owner_coeff.ap_from_ap      = 1.00;
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack";
    action_list_str += "/crackling_tiger_lightning";

    pet_t::init_action_list();
  }

  action_t *create_action( util::string_view name, util::string_view options_str ) override
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
  struct melee_t : public pet_melee_t
  {
    melee_t( niuzao_pet_t *pet, weapon_t *weapon ) : pet_melee_t( "melee_main_hand", pet, weapon )
    {
    }
  };

  struct stomp_t : public pet_melee_attack_t
  {
    stomp_t( niuzao_pet_t *pet, std::string_view options_str )
      : pet_melee_attack_t( "stomp", pet, pet->o()->talent.brewmaster.invoke_niuzao_the_black_ox_stomp )
    {
      parse_options( options_str );
      aoe      = -1;
      may_crit = true;
    }

    double action_multiplier() const override
    {
      double am = pet_melee_attack_t::action_multiplier();
      am *= 1.0 + o()->talent.brewmaster.walk_with_the_ox->effectN( 1 ).percent();
      am *= 1.0 + o()->buff.recent_purifies->check_value();
      return am;
    }

    void execute() override
    {
      pet_melee_attack_t::execute();
      o()->buff.recent_purifies->cancel();
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( niuzao_pet_t *pet, std::string_view options_str ) : pet_auto_attack_t( pet )
    {
      parse_options( options_str );
      player->main_hand_attack                    = new melee_t( pet, &( pet->main_hand_weapon ) );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    }
  };

  niuzao_pet_t( std::string_view name, monk_t *player ) : monk_pet_t( player, name, PET_NIUZAO, false, true )
  {
    npc_id                      = (int)o()->find_spell( 132578 )->effectN( 1 ).misc_value1();
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    owner_coeff.ap_from_ap      = 1;
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack";
    action_list_str += "/stomp";

    pet_t::init_action_list();
  }

  action_t *create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "stomp" )
      return new stomp_t( this, options_str );

    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }
};

struct invoke_niuzao_pet_t : public niuzao_pet_t
{
  invoke_niuzao_pet_t( monk_t *player ) : niuzao_pet_t( "invoke_niuzao_the_black_ox", player )
  {
  }
};

struct call_to_arms_niuzao_pet_t : public niuzao_pet_t
{
  call_to_arms_niuzao_pet_t( monk_t *player ) : niuzao_pet_t( "call_to_arms_niuzao_the_black_ox", player )
  {
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
    melee_t( util::string_view n, chiji_pet_t *player, weapon_t *weapon ) : pet_melee_t( n, player, weapon )
    {
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( chiji_pet_t *player, util::string_view options_str ) : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      player->main_hand_attack = new melee_t( "melee_main_hand", player, &( player->main_hand_weapon ) );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    }
  };

public:
  chiji_pet_t( monk_t *owner ) : monk_pet_t( owner, "chiji_the_red_crane", PET_CHIJI, false, true )
  {
    npc_id                      = (int)o()->find_spell( 325197 )->effectN( 1 ).misc_value1();
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );
    owner_coeff.ap_from_ap      = o()->baseline.mistweaver.aura->effectN( 4 ).percent();
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack";

    monk_pet_t::init_action_list();
  }

  action_t *create_action( util::string_view name, util::string_view options_str ) override
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
  yulon_pet_t( monk_t *owner ) : monk_pet_t( owner, "yulon_the_jade_serpent", PET_YULON, false, true )
  {
    npc_id                      = (int)o()->find_spell( 322118 )->effectN( 1 ).misc_value1();
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    owner_coeff.ap_from_ap      = o()->baseline.mistweaver.aura->effectN( 4 ).percent();
  }
};

// ==========================================================================
// White Tiger Statue
// ==========================================================================
struct white_tiger_statue_t : public monk_pet_t
{
private:
  struct claw_of_the_white_tiger_t : public pet_spell_t
  {
    claw_of_the_white_tiger_t( white_tiger_statue_t *p, util::string_view options_str )
      : pet_spell_t( "claw_of_the_white_tiger", p, p->o()->talent.monk.claw_of_the_white_tiger )
    {
      parse_options( options_str );
      aoe = -1;
    }
  };

public:
  white_tiger_statue_t( monk_t *owner ) : monk_pet_t( owner, "white_tiger_statue", PET_MONK_STATUE, false, true )
  {
    npc_id                      = owner->talent.monk.summon_white_tiger_statue->effectN( 1 ).misc_value1();
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    owner_coeff.ap_from_ap      = 1.00;
  }

  void init_action_list() override
  {
    action_list_str = "claw_of_the_white_tiger";

    pet_t::init_action_list();
  }

  action_t *create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "claw_of_the_white_tiger" )
      return new claw_of_the_white_tiger_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Fury of Xuen Tiger
// ==========================================================================
struct fury_of_xuen_pet_t : public monk_pet_t
{
private:
  struct melee_t : public pet_melee_t
  {
    melee_t( util::string_view n, fury_of_xuen_pet_t *player, weapon_t *weapon ) : pet_melee_t( n, player, weapon )
    {
    }
  };

  struct crackling_tiger_lightning_tick_t : public pet_spell_t
  {
    crackling_tiger_lightning_tick_t( fury_of_xuen_pet_t *p )
      : pet_spell_t( "crackling_tiger_lightning_tick", p, p->o()->passives.crackling_tiger_lightning )
    {
      background   = true;
      merge_report = false;
    }
  };

  struct crackling_tiger_lightning_t : public pet_spell_t
  {
    crackling_tiger_lightning_t( fury_of_xuen_pet_t *p, util::string_view options_str )
      : pet_spell_t( "crackling_tiger_lightning", p, p->o()->passives.crackling_tiger_lightning_driver )
    {
      parse_options( options_str );
      s_data_reporting = p->o()->passives.crackling_tiger_lightning;

      dot_duration       = p->o()->passives.fury_of_xuen->duration();
      cooldown->duration = p->o()->passives.fury_of_xuen->duration();

      tick_action = new crackling_tiger_lightning_tick_t( p );
    }

    double last_tick_factor( const dot_t *, timespan_t, timespan_t ) const override
    {
      return 0.0;
    }
  };

  struct auto_attack_t : public pet_auto_attack_t
  {
    auto_attack_t( fury_of_xuen_pet_t *player, util::string_view options_str ) : pet_auto_attack_t( player )
    {
      parse_options( options_str );

      player->main_hand_attack = new melee_t( "melee_main_hand", player, &( player->main_hand_weapon ) );
      player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    }
  };

public:
  fury_of_xuen_pet_t( monk_t *owner ) : monk_pet_t( owner, "fury_of_xuen_tiger", PET_XUEN, false, true )
  {
    npc_id                      = o()->passives.fury_of_xuen->effectN( 2 ).misc_value1();
    npc_suffix                  = "fury";
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.max_dmg    = dbc->spell_scaling( o()->type, level() );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );
    owner_coeff.ap_from_ap      = 1.00;
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack";
    action_list_str += "/crackling_tiger_lightning";

    pet_t::init_action_list();
  }

  action_t *create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "crackling_tiger_lightning" )
      return new crackling_tiger_lightning_t( this, options_str );

    if ( name == "auto_attack" )
      return new auto_attack_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }
};

}  // end namespace pets

monk_t::pets_t::pets_t( monk_t *p )
  : sef(),
    xuen( "xuen_the_white_tiger", p, []( monk_t *p ) { return new pets::xuen_pet_t( p ); } ),
    niuzao( "niuzao_the_black_ox", p, []( monk_t *p ) { return new pets::invoke_niuzao_pet_t( p ); } ),
    yulon( "yulon_the_jade_serpent", p, []( monk_t *p ) { return new pets::yulon_pet_t( p ); } ),
    chiji( "chiji_the_red_crane", p, []( monk_t *p ) { return new pets::chiji_pet_t( p ); } ),
    white_tiger_statue( "white_tiger_statue", p, []( monk_t *p ) { return new pets::white_tiger_statue_t( p ); } ),
    fury_of_xuen_tiger( "fury_of_xuen_tiger", p, []( monk_t *p ) { return new pets::fury_of_xuen_pet_t( p ); } ),
    call_to_arms_niuzao( "call_to_arms_niuzao", p,
                         []( monk_t *p ) { return new pets::call_to_arms_niuzao_pet_t( p ); } )
{
}

// monk_t::create_pets ======================================================

void monk_t::create_pets()
{
  base_t::create_pets();

  /*  if ( talent.windwalker.invoke_xuen_the_white_tiger->ok() && ( find_action( "invoke_xuen" ) || find_action(
    "invoke_xuen_the_white_tiger" ) ) )
    {
      pets.xuen = new pets::xuen_pet_t( this );
    }

    if ( spec.invoke_niuzao->ok() && ( find_action( "invoke_niuzao" ) || find_action( "invoke_niuzao_the_black_ox" ) ) )
    {
      pets.niuzao = new pets::niuzao_pet_t( this );
    }

    if ( talent.invoke_chi_ji->ok() && ( find_action( "invoke_chiji" ) || find_action( "invoke_chiji_the_red_crane" ) )
    )
    {
      pets.chiji = new pets::chiji_pet_t( this );
    }

    if ( spec.invoke_yulon->ok() && !talent.invoke_chi_ji->ok() &&
         ( find_action( "invoke_yulon" ) || find_action( "invoke_yulon_the_jade_serpent" ) ) )
    {
      pets.yulon = new pets::yulon_pet_t( this );
    }
  */

  if ( specialization() == MONK_WINDWALKER && find_action( "storm_earth_and_fire" ) )
  {
    pets.sef[ (int)pets::sef_pet_e::SEF_FIRE ] =
        new pets::storm_earth_and_fire_pet_t( "fire_spirit", this, true, WEAPON_SWORD );
    // The player BECOMES the Storm Spirit
    // SEF EARTH was changed from 2-handed user to dual welding in Legion
    pets.sef[ (int)pets::sef_pet_e::SEF_EARTH ] =
        new pets::storm_earth_and_fire_pet_t( "earth_spirit", this, true, WEAPON_MACE );
  }
}

void monk_t::trigger_storm_earth_and_fire( const action_t *a, actions::sef_ability_e sef_ability,
                                           bool combo_strike = false )
{
  if ( specialization() != MONK_WINDWALKER )
    return;

  // if action has no SEF counterpart...
  if ( !( sef_ability > actions::sef_ability_e::SEF_MIN ) )
    return;

  if ( !talent.windwalker.storm_earth_and_fire->ok() )
    return;

  if ( !buff.storm_earth_and_fire->up() )
    return;

  // Trigger pet retargeting if sticky target is not defined, and the Monk used one of the Cyclone
  // Strike triggering abilities
  if ( !pets.sef[ (int)pets::sef_pet_e::SEF_EARTH ]->sticky_target &&
       ( sef_ability == actions::sef_ability_e::SEF_TIGER_PALM ||
         sef_ability == actions::sef_ability_e::SEF_BLACKOUT_KICK ||
         sef_ability == actions::sef_ability_e::SEF_RISING_SUN_KICK ) )
  {
    retarget_storm_earth_and_fire_pets();
  }

  pets.sef[ (int)pets::sef_pet_e::SEF_EARTH ]->trigger_attack( sef_ability, a, combo_strike );
  pets.sef[ (int)pets::sef_pet_e::SEF_FIRE ]->trigger_attack( sef_ability, a, combo_strike );
}

void monk_t::storm_earth_and_fire_fixate( player_t *target )
{
  auto fixate = [ & ]( pets::sef_pet_e pet ) {
    sim->print_debug( "{} storm_earth_and_fire sticky target {} to {} (old={})", *this, *pets.sef[ (int)pet ], *target,
                      *pets.sef[ (int)pet ]->target );
    pets.sef[ (int)pet ]->target        = target;
    pets.sef[ (int)pet ]->sticky_target = true;
  };

  fixate( pets::sef_pet_e::SEF_EARTH );
  fixate( pets::sef_pet_e::SEF_FIRE );
}

bool monk_t::storm_earth_and_fire_fixate_ready( player_t *target )
{
  if ( buff.storm_earth_and_fire->check() )
    if ( pets.sef[ (int)pets::sef_pet_e::SEF_EARTH ]->sticky_target ||
         pets.sef[ (int)pets::sef_pet_e::SEF_FIRE ]->sticky_target )
      if ( pets.sef[ (int)pets::sef_pet_e::SEF_EARTH ]->target != target ||
           pets.sef[ (int)pets::sef_pet_e::SEF_FIRE ]->target != target )
        return true;
  return false;
}

// monk_t::summon_storm_earth_and_fire ================================================

void monk_t::summon_storm_earth_and_fire( timespan_t duration )
{
  auto targets = create_storm_earth_and_fire_target_list();

  auto summon_sef_pet = [ & ]( pets::storm_earth_and_fire_pet_t *pet ) {
    // Start targeting logic from "owner" always
    pet->reset_targeting();
    pet->target        = target;
    pet->sticky_target = false;
    retarget_storm_earth_and_fire( pet, targets );
    pet->summon( duration );
  };

  summon_sef_pet( pets.sef[ (int)pets::sef_pet_e::SEF_EARTH ] );
  summon_sef_pet( pets.sef[ (int)pets::sef_pet_e::SEF_FIRE ] );
}

// monk_t::retarget_storm_earth_and_fire_pets =======================================

void monk_t::retarget_storm_earth_and_fire_pets() const
{
  if ( pets.sef[ (int)pets::sef_pet_e::SEF_EARTH ]->sticky_target )
    return;

  auto targets = create_storm_earth_and_fire_target_list();
  retarget_storm_earth_and_fire( pets.sef[ (int)pets::sef_pet_e::SEF_EARTH ], targets );
  retarget_storm_earth_and_fire( pets.sef[ (int)pets::sef_pet_e::SEF_FIRE ], targets );
}

// Callback to retarget Storm Earth and Fire pets when new target appear, or old targets depsawn
// (i.e., die).
void sef_despawn_cb_t::operator()( player_t * )
{
  // No pets up, don't do anything
  if ( !monk->buff.storm_earth_and_fire->check() )
    return;

  auto targets = monk->create_storm_earth_and_fire_target_list();

  // If the active clone's target is sleeping, reset it's targeting, and jump it to a new target.
  // Note that if sticky targeting is used, both targets will jump (since both are going to be
  // stickied to the dead target)
  range::for_each( monk->pets.sef, [ this, &targets ]( pets::storm_earth_and_fire_pet_t *pet ) {
    // Arise time went negative, so the target is sleeping. Can't check "is_sleeping" here, because
    // the callback is called before the target goes to sleep.
    if ( pet->target->arise_time < timespan_t::zero() )
    {
      pet->reset_targeting();
      monk->retarget_storm_earth_and_fire( pet, targets );
    }
    else
    {
      // Retarget pets otherwise (a new target has appeared). Note that if the pets are sticky
      // targeted, this will do nothing.
      monk->retarget_storm_earth_and_fire( pet, targets );
    }
  } );
}

player_t *monk_t::storm_earth_and_fire_fixate_target( pets::sef_pet_e sef_pet )
{
  if ( pets.sef[ (int)sef_pet ]->sticky_target )
    return pets.sef[ (int)sef_pet ]->target;

  return nullptr;
}
}  // namespace monk
