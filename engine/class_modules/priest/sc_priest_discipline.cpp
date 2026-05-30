// ==========================================================================
// Discipline Priest Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================
#include "sc_enums.hpp"
#include "sc_priest.hpp"

#include "simulationcraft.hpp"

namespace priestspace
{
namespace actions::spells
{
struct power_word_radiance_t final : public priest_heal_t
{
  timespan_t atonement_duration;

  power_word_radiance_t( priest_t& p, std::string_view name, util::string_view options_str )
    : priest_heal_t( name, p, p.talents.discipline.power_word_radiance )
  {
    parse_options( options_str );
    harmful      = false;
    disc_mastery = true;

    aoe = 1 + as<int>( data().effectN( 3 ).base_value() );

    atonement_duration = data().effectN( 4 ).percent() * p.talents.discipline.atonement_buff->duration();
  }

  power_word_radiance_t( priest_t& p, util::string_view options_str )
    : power_word_radiance_t( p, "power_word_radiance", options_str )
  {
  }

  void execute() override
  {
    priest_heal_t::execute();

    if ( priest().talents.discipline.harsh_discipline.ok() )
    {
      priest().buffs.harsh_discipline->trigger();
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    priest_td_t& td = get_td( s->target );
    td.buffs.atonement->trigger( atonement_duration );
  }

  size_t available_targets( std::vector<player_t*>& target_list ) const override
  {
    target_list.clear();
    target_list.push_back( target );

    /*for ( const auto& t : sim->healing_no_pet_list )
      if ( t != target )
        target_list.push_back( t );

    rng().shuffle( target_list.begin() + 1, target_list.end() );*/

    // Don't include pets for the ease of writing APLs
    if ( as<int>( sim->healing_no_pet_list.size() ) <= n_targets() )
    {
      for ( auto t : sim->healing_no_pet_list )
        if ( t != target && ( t->is_active() || ( t->type == HEALING_ENEMY && !t->is_sleeping() ) ) )
          target_list.push_back( t );

      auto offset = target_list.size();

      for ( auto t : sim->healing_pet_list )
      {
        if ( t != target && ( ( t->type == HEALING_ENEMY && !t->is_sleeping() ) ) )
          target_list.push_back( t );
      }

      if ( std::next( target_list.begin(), offset ) < target_list.end() )
        rng().shuffle( std::next( target_list.begin(), offset ), target_list.end() );

      return target_list.size();
    }

    std::vector<player_t*> helper_list = {};

    for ( auto t : sim->healing_no_pet_list )
    {
      if ( t != target && ( t->is_active() || ( t->type == HEALING_ENEMY && !t->is_sleeping() ) ) )
      {
        if ( !p().find_target_data( t ) || !p().find_target_data( t )->buffs.atonement->check() )
        {
          target_list.push_back( t );
        }
        else
        {
          helper_list.push_back( t );
        }
      }
    }

    if ( as<int>( target_list.size() ) > n_targets() )
    {
      rng().shuffle( target_list.begin() + 1, target_list.end() );
    }

    auto offset = target_list.size();

    for ( auto t : helper_list )
    {
      target_list.push_back( t );
    }

    if ( std::next( target_list.begin(), offset ) < target_list.end() )
      rng().shuffle( std::next( target_list.begin(), offset ), target_list.end() );

    return target_list.size();
  }

  void activate() override
  {
    priest_heal_t::activate();

    priest().allies_with_atonement.register_callback( [ this ]( player_t* ) { target_cache.is_valid = false; } );
  }
};

struct pain_suppression_t final : public priest_spell_t
{
  pain_suppression_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "pain_suppression", p, p.talents.discipline.pain_suppression )
  {
    parse_options( options_str );

    // If we don't specify a target, it's defaulted to the mob, so default to the player instead
    if ( target->is_enemy() || target->is_add() )
    {
      target = &p;
    }

    harmful = false;

    target_debuff = p.talents.discipline.pain_suppression;
  }

  buff_t* create_debuff( player_t* t ) override
  {
    return priest_spell_t::create_debuff( t )
      ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN )
      ->set_cooldown( 0_ms );  // Let the ability handle the CD
  }

  void execute() override
  {
    priest_spell_t::execute();

    get_debuff( target )->trigger();
  }
};

struct evangelism_t final : public priest_heal_t
{
  action_t* evangelism_radiance;
  double radiance_effectiveness;
  evangelism_t( priest_t& p, util::string_view options_str )
    : priest_heal_t( "evangelism", p, p.talents.discipline.evangelism ),
      radiance_effectiveness( data().effectN( 2 ).percent() )
  {
    parse_options( options_str );

    harmful = false;

    evangelism_radiance =
        priest().get_secondary_action<power_word_radiance_t>( "evangelism_radiance", "evangelism_radiance" );

    if ( !evangelism_radiance->stats->parent )
    {
      evangelism_radiance->proc = true;
      evangelism_radiance->dual = true;
      evangelism_radiance->base_multiplier *= radiance_effectiveness;
      add_child( evangelism_radiance );
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );
    if ( result_is_hit( s->result ) )
    {
      evangelism_radiance->execute_on_target( s->target );
    }
  };

  void execute() override
  {
    priest_heal_t::execute();

    priest().buffs.evangelism->trigger();

    if ( priest().talents.discipline.archangel.enabled() )
    {
      priest().buffs.archangel->trigger();
    }

    if ( priest().talents.shared.mindbender.enabled() )
    {
      priest().pets.mindbender.spawn();
    }
  }
};

// Purge the wicked
struct purge_the_wicked_t final : public priest_spell_t
{
  struct purge_the_wicked_dot_t final : public priest_spell_t
  {
    // Manually create the dot effect because "ticking" is not present on
    // primary spell
    purge_the_wicked_dot_t( priest_t& p, util::string_view options_str )
      : priest_spell_t( "purge_the_wicked", p, p.talents.discipline.purge_the_wicked->effectN( 2 ).trigger() )
    {
      parse_options( options_str );
      background = true;
      // TODO: Implement the spreading of Purge the Wicked via penance

      triggers_atonement = true;
    }

    void tick( dot_t* d ) override
    {
      priest_spell_t::tick( d );

      if ( d->state->result_amount > 0 )
      {
        trigger_power_of_the_dark_side();
      }
    }
  };

  purge_the_wicked_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "purge_the_wicked", p, p.talents.discipline.purge_the_wicked )
  {
    parse_options( options_str );
    tick_zero      = false;
    execute_action = new purge_the_wicked_dot_t( p, "" );

    triggers_atonement = true;
  }
};

// ==========================================================================
// Penance & Dark Reprimand
// Penance:
// - Base(47540) ? (47758) -> (47666)
// Dark Reprimand:
// - Base(373129) -> (373130)
// ==========================================================================

// Penance channeled spell
struct penance_base_t : public priest_spell_t
{
protected:
  struct penance_data
  {
    int bolts            = 3;
    double snapshot_mult = 1.0;
  };

  using state_t = priest_action_state_t<penance_data>;
  using ab      = priest_spell_t;

  struct penance_damage_t : public priest_spell_t
  {
    timespan_t dot_extension;

    penance_damage_t( priest_t& p, util::string_view n, const spell_data_t* s ) : priest_spell_t( n, p, s )
    {
      background = dual = direct_tick = tick_may_crit = may_crit = true;
      dot_extension      = priest().talents.discipline.painful_punishment->effectN( 1 ).time_value();
      triggers_atonement = true;
    }

    action_state_t* new_state() override
    {
      return new state_t( this, target );
    }

    state_t* cast_state( action_state_t* s )
    {
      return static_cast<state_t*>( s );
    }

    const state_t* cast_state( const action_state_t* s ) const
    {
      return static_cast<const state_t*>( s );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double d = priest_spell_t::composite_da_multiplier( s );

      d *= cast_state( s )->snapshot_mult;

      return d;
    }

    void impact( action_state_t* s ) override
    {
      priest_spell_t::impact( s );
      priest_td_t& td = get_td( s->target );
      td.dots.shadow_word_pain->adjust_duration( dot_extension );
      td.dots.purge_the_wicked->adjust_duration( dot_extension );
    }

    void execute() override
    {
      priest_spell_t::execute();

      if ( priest().talents.discipline.holy_ray.enabled() )
      {
        priest().buffs.holy_ray->trigger();
      }
    }
  };

private:
  propagate_const<penance_damage_t*> damage;
  unsigned max_spread_targets;
  double default_bolts;

public:
  penance_base_t( priest_t& p, util::string_view name, const spell_data_t* s, const spell_data_t* s_channel,
                  const spell_data_t* s_tick )
    : priest_spell_t( name, p, s ),
      damage( new penance_damage_t( p, std::string( name ) + "_tick", s_tick ) ),
      max_spread_targets( as<unsigned>( 1 + priest().talents.discipline.revel_in_darkness->effectN( 2 ).base_value() ) )
  {
    cooldown = p.cooldowns.penance;
    if ( cooldown->duration != timespan_t::zero() )
    {
      if ( s->charge_cooldown() > timespan_t::zero() )
        cooldown->duration = s->charge_cooldown();
    }
    else
    {
      cooldown->duration = p.specs.penance->cooldown();
    }

    channeled = true;

    may_miss = may_crit = false;
    tick_zero           = true;

    add_child( damage );

    id              = s_channel->id();
    min_travel_time = s_channel->missile_min_duration();

    if ( s_channel->flags( spell_attribute::SX_FIXED_TRAVEL_TIME ) )
      travel_delay = s_channel->missile_speed();
    else
      travel_speed = s_channel->missile_speed();

    // Setup Channel Flags.
    hasted_ticks =
        s_channel->flags( spell_attribute::SX_DOT_HASTED ) || s_channel->flags( spell_attribute::SX_DOT_HASTED_MELEE );
    tick_on_application       = s_channel->flags( spell_attribute::SX_TICK_ON_APPLICATION );
    hasted_dot_duration       = s_channel->flags( spell_attribute::SX_DURATION_HASTED );
    rolling_periodic          = s_channel->flags( spell_attribute::SX_ROLLING_PERIODIC );
    treat_as_periodic         = s_channel->flags( spell_attribute::SX_TREAT_AS_PERIODIC );
    allow_class_ability_procs = s_channel->flags( spell_attribute::SX_ALLOW_CLASS_ABILITY_PROCS );
    not_a_proc                = s_channel->flags( spell_attribute::SX_NOT_A_PROC );

    if ( s_channel->flags( spell_attribute::SX_REFRESH_EXTENDS_DURATION ) )
      dot_behavior = dot_behavior_e::DOT_REFRESH_PANDEMIC;

    for ( const spelleffect_data_t& ed : s_channel->effects() )
    {
      parse_effect_data( ed );
    }

    // One is always tick zero
    default_bolts = floor( dot_duration / base_tick_time );
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  state_t* cast_state( action_state_t* s )
  {
    return static_cast<state_t*>( s );
  }

  const state_t* cast_state( const action_state_t* s ) const
  {
    return static_cast<const state_t*>( s );
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    ab::snapshot_state( s, rt );
    cast_state( s )->bolts         = as<int>( default_bolts + p().buffs.harsh_discipline->check_value() );
    cast_state( s )->snapshot_mult = 1.0 + priest().buffs.power_of_the_dark_side->check_value();
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    timespan_t t = ab::tick_time( s );

    sim->print_debug( "{} default bolts {} state bolts", default_bolts + 1, cast_state( s )->bolts + 1 );

    t *= default_bolts / cast_state( s )->bolts;

    return t;
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( d->get_tick_factor() >= 1.0 )
    {
      if ( priest().talents.discipline.weal_and_woe.enabled() )
      {
        priest().buffs.weal_and_woe->trigger();
      }

      priest().expand_entropic_rift();

      damage->set_target( d->state->target );

      state_t* state       = damage->cast_state( damage->get_state() );
      state->target        = d->state->target;
      state->snapshot_mult = cast_state( d->state )->snapshot_mult;
      damage->snapshot_state( state, damage->amount_type( state ) );

      damage->schedule_execute( state );
    }
  }

  void last_tick( dot_t* d ) override
  {
    priest_spell_t::last_tick( d );
  }

  void move_random_target( std::vector<player_t*>& in, std::vector<player_t*>& out ) const
  {
    auto idx = rng().range( 0U, as<unsigned>( in.size() ) );
    out.push_back( in[ idx ] );
    in.erase( in.begin() + idx );
  }

  static std::string actor_list_str( const std::vector<player_t*>& actors, util::string_view delim = ", " )
  {
    static const auto transform_fn = []( player_t* t ) { return t->name(); };
    std::vector<const char*> tmp;

    range::transform( actors, std::back_inserter( tmp ), transform_fn );

    return tmp.size() ? util::string_join( tmp, delim ) : "none";
  }

  void spread_shadow_word_pain( const action_state_t* state, priest_t& p ) const
  {
    // Exit if PTW isn't ticking
    if ( !td( state->target )->dots.shadow_word_pain->is_ticking() )
    {
      return;
    }
    // Exit if there 1 or fewer targets
    if ( target_list().size() <= 1 )
    {
      return;
    }
    // Targets to spread PTW to
    std::vector<player_t*> targets;

    // Targets without PTW
    std::vector<player_t*> no_swp_targets,
        // Targets that already have PTW
        has_swp_targets;

    // Categorize all available targets (within 8 yards of the main target) based on presence of PTW
    range::for_each( target_list(), [ & ]( player_t* t ) {
      // Ignore main target
      if ( t == state->target )
      {
        return;
      }

      if ( !td( t )->dots.shadow_word_pain->is_ticking() )
      {
        no_swp_targets.push_back( t );
      }
      else if ( td( t )->dots.shadow_word_pain->is_ticking() )
      {
        has_swp_targets.push_back( t );
      }
    } );

    // 1) Randomly select targets without PTW, unless there already are the maximum number of targets with PTW up.
    while ( no_swp_targets.size() > 0 && targets.size() < max_spread_targets )
    {
      move_random_target( no_swp_targets, targets );
    }

    // 2) Randomly select targets that already have PTW on them
    while ( has_swp_targets.size() > 0 && targets.size() < max_spread_targets )
    {
      move_random_target( has_swp_targets, targets );
    }

    sim->print_debug( "{} purge_the_wicked spread selected targets={{ {} }}", player->name(),
                      actor_list_str( targets ) );

    range::for_each(
        targets, [ & ]( player_t* target ) { p.background_actions.purge_the_wicked->execute_on_target( target ); } );
  }

  void spread_purge_the_wicked( const action_state_t* state, priest_t& p ) const
  {
    // Exit if PTW isn't ticking
    if ( !td( state->target )->dots.purge_the_wicked->is_ticking() )
    {
      return;
    }
    // Exit if there 1 or fewer targets
    if ( target_list().size() <= 1 )
    {
      return;
    }
    // Targets to spread PTW to
    std::vector<player_t*> targets;

    // Targets without PTW
    std::vector<player_t*> no_ptw_targets,
        // Targets that already have PTW
        has_ptw_targets;

    // Categorize all available targets (within 8 yards of the main target) based on presence of PTW
    range::for_each( target_list(), [ & ]( player_t* t ) {
      // Ignore main target
      if ( t == state->target )
      {
        return;
      }

      if ( !td( t )->dots.purge_the_wicked->is_ticking() )
      {
        no_ptw_targets.push_back( t );
      }
      else if ( td( t )->dots.purge_the_wicked->is_ticking() )
      {
        has_ptw_targets.push_back( t );
      }
    } );

    // 1) Randomly select targets without PTW, unless there already are the maximum number of targets with PTW up.
    while ( no_ptw_targets.size() > 0 && targets.size() < max_spread_targets )
    {
      move_random_target( no_ptw_targets, targets );
    }

    // 2) Randomly select targets that already have PTW on them
    while ( has_ptw_targets.size() > 0 && targets.size() < max_spread_targets )
    {
      move_random_target( has_ptw_targets, targets );
    }

    sim->print_debug( "{} purge_the_wicked spread selected targets={{ {} }}", player->name(),
                      actor_list_str( targets ) );

    range::for_each(
        targets, [ & ]( player_t* target ) { p.background_actions.purge_the_wicked->execute_on_target( target ); } );
  }

  void execute() override
  {
    priest().buffs.holy_ray->expire();
    priest_spell_t::execute();

    priest().buffs.power_of_the_dark_side->expire();

    priest().buffs.harsh_discipline->decrement();
  }

  void impact( action_state_t* state ) override
  {
    priest_spell_t::impact( state );

    if ( p().talents.discipline.encroaching_shadows.enabled() )
    {
      spread_shadow_word_pain( state, p() );
    }

    priest().trigger_inescapable_torment( state->target );
  }
};

struct penance_t : public penance_base_t
{
  penance_t( priest_t& p, util::string_view options_str )
    : penance_base_t( p, "penance", p.specs.penance, p.specs.penance_channel, p.specs.penance_tick )
  {
    parse_options( options_str );
  }

  bool action_ready() override
  {
    return penance_base_t::action_ready();
  }
};

struct ultimate_penitence_t : public priest_spell_t
{
protected:
  struct ultimate_penitence_damage_t : public priest_spell_t
  {
    timespan_t dot_extension;

    ultimate_penitence_damage_t( priest_t& p )
      : priest_spell_t( "ultimate_penitence_damage", p, p.find_spell( 421543 ) )
    {
      dot_extension      = priest().talents.discipline.painful_punishment->effectN( 1 ).time_value();
      triggers_atonement = true;
    }

    void impact( action_state_t* s ) override
    {
      priest_spell_t::impact( s );
      priest_td_t& td = get_td( s->target );
      td.dots.shadow_word_pain->adjust_duration( dot_extension );
      td.dots.purge_the_wicked->adjust_duration( dot_extension );
    }
  };

  struct ultimate_penitence_channel_t : public priest_spell_t
  {
    // ultimate_penitence_damage_t
    propagate_const<ultimate_penitence_damage_t*> damage;

    ultimate_penitence_channel_t( priest_t& p, stats_t* parent_stats )
      : priest_spell_t( "ultimate_penitence_channel", p, p.find_spell( 421434 ) )
    {
      damage    = new ultimate_penitence_damage_t( p );
      dual      = true;
      channeled = true;
      tick_zero = true;
      stats     = parent_stats;
      stats->action_list.push_back( this );
    }

    void tick( dot_t* d ) override
    {
      priest_spell_t::tick( d );

      if ( damage && d->get_tick_factor() >= 1.0 )
      {
        damage->execute_on_target( d->target );
      }

      if ( priest().talents.discipline.weal_and_woe.enabled() )
      {
        priest().buffs.weal_and_woe->trigger();
      }

      priest().expand_entropic_rift();
    }
  };

  propagate_const<ultimate_penitence_channel_t*> channel;

public:
  ultimate_penitence_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "ultimate_penitence", p, p.talents.discipline.ultimate_penitence )
  {
    parse_options( options_str );
    // Channel = 421434
    // Damage bolt = 421543

    channel = new ultimate_penitence_channel_t( p, stats );
    add_child( channel->damage );
  }

  void execute() override
  {
    priest_spell_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    channel->execute_on_target( s->target );
  }
};
}  // namespace actions::spells

namespace buffs
{
}  // namespace buffs

void priest_t::create_buffs_discipline()
{
  buffs.power_of_the_dark_side =
      make_buff( this, "power_of_the_dark_side", talents.discipline.power_of_the_dark_side->effectN( 1 ).trigger() )
          ->set_default_value_from_effect( 1, 0.01 );

  buffs.harsh_discipline = make_buff( this, "harsh_discipline", find_spell( 373183 ) )
                               ->set_default_value( talents.discipline.harsh_discipline->effectN( 2 ).base_value() );

  buffs.borrowed_time = make_buff( this, "borrowed_time", find_spell( 390692 ) )->add_invalidate( CACHE_HASTE );

  if ( talents.discipline.borrowed_time.ok() )
  {
    buffs.borrowed_time->set_default_value( talents.discipline.borrowed_time->effectN( 2 ).percent() );
  }

  buffs.weal_and_woe = make_buff( this, "weal_and_woe", talents.discipline.weal_and_woe_buff );

  buffs.archangel = make_buff( this, "archangel", talents.discipline.archangel_buff );

  buffs.holy_ray = make_buff( this, "holy_ray", talents.discipline.holy_ray_buff );

  buffs.evangelism = make_buff( this, "evangelism", talents.discipline.evangelism )
                         ->set_cooldown( 0_s )
                         ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
                         ->set_initial_stack_to_max_stack();

  buffs.greater_smite = make_buff( this, "greater_smite", talents.discipline.greater_smite_buff );
}

void priest_t::init_rng_discipline()
{
  deck_rng.master_of_darkness = get_shuffled_rng( "master_of_darkness", 1, 3 );
}

void priest_t::init_background_actions_discipline()
{
  if ( talents.discipline.purge_the_wicked.enabled() )
  {
    background_actions.purge_the_wicked = new actions::spells::purge_the_wicked_t( *this, "" );
  }
}

void priest_t::init_spells_discipline()
{
  auto ST = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::SPECIALIZATION, n ); };

  // Talents
  // Row 1
  talents.discipline.atonement       = ST( "Atonement" );
  talents.discipline.atonement_buff  = find_spell( 194384 );
  talents.discipline.atonement_spell = find_spell( 94472 );
  // Row 2
  talents.discipline.power_word_radiance    = ST( "Power Word: Radiance" );
  talents.discipline.pain_suppression       = ST( "Pain Suppression" );
  talents.discipline.power_of_the_dark_side = ST( "Power of the Dark Side" );
  // Row 3
  talents.discipline.lights_promise         = ST( "Light's Promise" );
  talents.discipline.sanctuary              = ST( "Sanctuary" );
  talents.discipline.pain_transformation    = ST( "Pain Transformation" );
  talents.discipline.protector_of_the_frail = ST( "Protector of the Frail" );
  talents.discipline.dark_indulgence        = ST( "Dark Indulgence" );
  talents.discipline.encroaching_shadows    = ST( "Encroaching Shadows" );  // NYI
  // Row 4
  talents.discipline.bright_pupil          = ST( "Bright Pupil" );
  talents.discipline.enduring_luminescence = ST( "Enduring Luminescence" );
  talents.discipline.shield_discipline     = ST( "Shield Discipline" );
  talents.discipline.ultimate_penitence    = ST( "Ultimate Penitence" );
  talents.discipline.power_word_barrier    = ST( "Power Word: Barrier" );
  talents.discipline.painful_punishment    = ST( "Painful Punishment" );
  talents.discipline.revel_in_darkness     = ST( "Revel in Darkness" );
  // Row 5
  talents.discipline.holy_ray            = ST( "Holy Ray" );  // NYI
  talents.discipline.holy_ray_buff       = find_spell( 1235193 );
  talents.discipline.lenience            = ST( "Lenience" );
  talents.discipline.shadow_tap          = ST( "Shadow Tap" );  // NYI
  talents.discipline.encroaching_shadows = ST( "Encroaching Shadows" );
  // Row 6
  talents.discipline.purge_the_wicked   = ST( "Purge the Wicked" );
  talents.discipline.castigation        = ST( "Castigation" );
  talents.discipline.indemnity          = ST( "Indemnity" );
  talents.discipline.pain_and_suffering = ST( "Pain and Suffering" );
  talents.discipline.occultist          = ST( "Occultist" );  // NYI
  // Row 7
  talents.discipline.harsh_discipline      = ST( "Harsh Discipline" );
  talents.discipline.harsh_discipline_buff = find_spell( 373183 );
  talents.discipline.evangelism            = ST( "Evangelism" );
  talents.discipline.abyssal_reverie       = ST( "Abyssal Reverie" );
  // Row 8
  talents.discipline.divine_procession = ST( "Divine Procession" );
  talents.discipline.inner_focus       = ST( "Inner Focus" );
  talents.discipline.archangel         = ST( "Archangel" );
  talents.discipline.archangel_buff    = find_spell( 81700 );
  talents.discipline.shadow_mend       = ST( "Shadow Mend" );  // NYI
  // Row 9
  talents.discipline.greater_smite      = ST( "Greater Smite" );  // NYI
  talents.discipline.greater_smite_buff = find_spell( 1253725 );
  talents.discipline.divine_aegis       = ST( "Divine Aegis" );
  talents.discipline.divine_aegis_buff  = find_spell( 47753 );
  talents.discipline.borrowed_time      = ST( "Borrowed Time" );
  talents.discipline.blaze_of_light     = ST( "Blaze of Light" );
  // Row 10
  talents.discipline.eternal_barrier   = ST( "Eternal Barrier" );
  talents.discipline.weal_and_woe      = ST( "Weal and Woe" );
  talents.discipline.weal_and_woe_buff = find_spell( 390787 );
  talents.discipline.searing_light     = ST( "Searing Light" );  // NYI
  talents.discipline.searing_light_dot = find_spell( 1280134 );
  talents.discipline.expiation         = ST( "Expiation" );
  // Apex
  talents.discipline.master_the_darkness_1 = find_talent_spell( talent_tree::SPECIALIZATION, 1253591 );
  talents.discipline.void_shield           = find_spell( 1253593 );
  talents.discipline.void_shield_reflect   = find_spell( 1253828 );
  talents.discipline.master_the_darkness_2 = find_talent_spell( talent_tree::SPECIALIZATION, 1253845 );
  talents.discipline.master_the_darkness_3 = find_talent_spell( talent_tree::SPECIALIZATION, 1253827 );

  // General Spells
  specs.penance         = find_spell( 47540 );
  specs.penance_channel = find_spell( 47758 );   // Channel spell, triggered by 47540, executes 47666 every tick
  specs.penance_tick    = find_spell( 47666 );   // Not triggered from 47540, only 47758

  specs.plea = find_spell( 200829 );

  specs.contrition_heal      = find_spell( 270501 );
  specs.contrition_heal_crit = find_spell( 281469 );
}

action_t* priest_t::create_action_discipline( util::string_view name, util::string_view options_str )
{
  using namespace actions::spells;

  if ( name == "power_word_radiance" )
  {
    return new power_word_radiance_t( *this, options_str );
  }
  if ( name == "pain_suppression" )
  {
    return new pain_suppression_t( *this, options_str );
  }
  if ( name == "penance" )
  {
    return new penance_t( *this, options_str );
  }
  if ( name == "purge_the_wicked" )
  {
    return new purge_the_wicked_t( *this, options_str );
  }
  if ( name == "evangelism" )
  {
    return new evangelism_t( *this, options_str );
  }
  if ( name == "ultimate_penitence" || name == "uppies" )
  {
    return new ultimate_penitence_t( *this, options_str );
  }

  return nullptr;
}

std::unique_ptr<expr_t> priest_t::create_expression_discipline( util::string_view name_str )
{
  if ( name_str == "active_atonements" )
  {
    if ( !talents.discipline.atonement.enabled() )
      return expr_t::create_constant( name_str, 0 );

    return make_fn_expr( name_str, [ this ]() { return allies_with_atonement.size(); } );
  }

  if ( name_str == "master_of_darkness_positive" )
  {
    if ( !talents.discipline.master_the_darkness_1.enabled() )
      return expr_t::create_constant( name_str, 0 );
    return make_fn_expr( name_str, [ this ]() { return deck_rng.master_of_darkness->count_remains( 1 ); } );
  }

  if ( name_str == "master_of_darkness_negative" )
  {
    if ( !talents.discipline.master_the_darkness_1.enabled() )
      return expr_t::create_constant( name_str, 0 );
    return make_fn_expr( name_str, [ this ]() { return deck_rng.master_of_darkness->count_remains( 0 ); } );
  }

  if ( name_str == "master_of_darkness_cards" )
  {
    if ( !talents.discipline.master_the_darkness_1.enabled() )
      return expr_t::create_constant( name_str, 0 );
    return make_fn_expr( name_str, [ this ]() { return deck_rng.master_of_darkness->entry_remains(); } );
  }

  if ( name_str == "min_active_atonement" )
  {
    if ( !talents.discipline.atonement.enabled() )
      return expr_t::create_constant( name_str, 0 );

    return make_fn_expr( name_str, [ this ]() {
      if ( allies_with_atonement.size() < 1 )
        return 0_s;

      auto min_elem = ( std::min_element(
          allies_with_atonement.begin(), allies_with_atonement.end(), [ this ]( player_t* a, player_t* b ) {
            return get_target_data( a )->buffs.atonement->remains() < get_target_data( b )->buffs.atonement->remains();
          } ) );

      return get_target_data( *min_elem )->buffs.atonement->remains();
    } );
  }

  return nullptr;
}

}  // namespace priestspace
