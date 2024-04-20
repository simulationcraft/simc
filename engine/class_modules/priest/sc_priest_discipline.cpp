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

  power_word_radiance_t( priest_t& p, util::string_view options_str )
    : priest_heal_t( "power_word_radiance", p, p.talents.discipline.power_word_radiance )
  {
    parse_options( options_str );
    harmful      = false;
    disc_mastery = true;

    aoe = 1 + as<int>( data().effectN( 3 ).base_value() );

    apply_affecting_aura( p.talents.discipline.lights_promise );
    apply_affecting_aura( p.talents.discipline.bright_pupil );
    apply_affecting_aura( p.talents.discipline.enduring_luminescence );

    atonement_duration =
        ( data().effectN( 4 ).percent() + p.talents.discipline.enduring_luminescence->effectN( 1 ).percent() ) *
        p.talents.discipline.atonement_buff->duration();
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

    if ( sim->healing_no_pet_list.size() <= n_targets() )
    {
      for ( auto t : sim->healing_no_pet_list )
        if ( t != target && ( t->is_active() || t->type == HEALING_ENEMY && !t->is_sleeping() ) )
          target_list.push_back( t );

      auto offset = target_list.size();

      for ( auto t : sim->healing_pet_list )
      {
        if ( t != target && ( t->is_active() || t->type == HEALING_ENEMY && !t->is_sleeping() ) )
          target_list.push_back( t );
      }
      
      if ( std::next( target_list.begin(), offset ) < target_list.end() )
        rng().shuffle( std::next( target_list.begin(), offset ), target_list.end() );

      return target_list.size();
    }

    std::vector<player_t*> helper_list = {};

    for ( auto t : sim->healing_no_pet_list )
    {
      if ( t != target && ( t->is_active() || t->type == HEALING_ENEMY && !t->is_sleeping() ) )
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

    if ( target_list.size() > n_targets() )
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
  }

  void execute() override
  {
    priest_spell_t::execute();

    target->buffs.pain_suppression->trigger();
  }
};


struct evangelism_t final : public priest_spell_t
{
  timespan_t atonement_extend;
  evangelism_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "evangelism", p, p.talents.discipline.evangelism ),
      atonement_extend( timespan_t::from_seconds( p.talents.discipline.evangelism->effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );

    target = &p;

    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    target->buffs.pain_suppression->trigger();

    for ( auto ally : p().allies_with_atonement )
    {
      p().find_target_data( ally )->buffs.atonement->extend_duration( &p(), atonement_extend );
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
      // 3% / 5% damage increase
      apply_affecting_aura( priest().talents.throes_of_pain );
      // 5% damage increase
      // TODO: Implement the spreading of Purge the Wicked via penance
      apply_affecting_aura( p.talents.discipline.revel_in_purity );
      // 8% / 15% damage increase
      apply_affecting_aura( priest().talents.discipline.pain_and_suffering );

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

    double composite_persistent_multiplier( const action_state_t* s ) const override
    {
      auto m = priest_spell_t::composite_persistent_multiplier( s );

      m *= 1 + p().buffs.twilight_equilibrium_holy_amp->check_value();

      return m;
    }
  };

  purge_the_wicked_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "purge_the_wicked", p, p.talents.discipline.purge_the_wicked )
  {
    parse_options( options_str );
    tick_zero      = false;
    execute_action = new purge_the_wicked_dot_t( p, "" );
    // 3% / 5% damage increase
    apply_affecting_aura( priest().talents.throes_of_pain );
    // 5% damage increase
    apply_affecting_aura( priest().talents.discipline.revel_in_purity );
    // 8% / 15% damage increase
    apply_affecting_aura( priest().talents.discipline.pain_and_suffering );

    if ( priest().sets->has_set_bonus( PRIEST_DISCIPLINE, T30, B2 ) )
    {
      apply_affecting_aura( p.sets->set( PRIEST_DISCIPLINE, T30, B2 ) );
    }

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
struct penance_channel_t final : public priest_spell_t
{
protected:  
  struct penance_data
  {
    int bolts = 3;
    double snapshot_mult = 1.0;
  };

  using state_t = priest_action_state_t<penance_data>;
  using ab = priest_spell_t;

  struct penance_damage_t : public priest_spell_t
  {
    timespan_t dot_extension;

    penance_damage_t( priest_t& p, util::string_view n, const spell_data_t* s ) : priest_spell_t( n, p, s )
    {
      background = dual = direct_tick = tick_may_crit = may_crit = true;
      dot_extension = priest().talents.discipline.painful_punishment->effectN( 1 ).time_value();

      // This is not found in the affected spells for Shadow Covenant, overriding it manually
      // Final two params allow us to override the 25% damage buff when twilight corruption is selected (25% -> 35%)
      force_effect( p.buffs.shadow_covenant, 1, IGNORE_STACKS, USE_DEFAULT, p.talents.discipline.twilight_corruption );

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
  };

private:
  propagate_const<penance_damage_t*> damage;
  timespan_t manipulation_cdr;
  timespan_t void_summoner_cdr;
  unsigned max_spread_targets;
  double default_bolts;

public:
  penance_channel_t( priest_t& p, util::string_view n, const spell_data_t* s, const spell_data_t* s_tick )
    : priest_spell_t( n, p, s ), damage( new penance_damage_t( p, std::string( n ) + "_tick", s_tick ) ),
      manipulation_cdr( timespan_t::from_seconds( priest().talents.manipulation->effectN( 1 ).base_value() / 2 ) ),
      void_summoner_cdr( priest().talents.discipline.void_summoner->effectN( priest().talents.shared.mindbender.enabled() ? 2 : 1 ).time_value() ),
      max_spread_targets( as<unsigned>( 1 + priest().talents.discipline.revel_in_purity->effectN( 2 ).base_value() ) )
  {
    channeled = true;
    dual      = false;
    may_miss = may_crit = false;
    tick_zero           = true;

    cooldown->duration  = 0_s;
    add_child( damage );
    
    apply_affecting_aura( priest().talents.discipline.castigation );

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
    cast_state( s )->bolts      = default_bolts + p().buffs.harsh_discipline->check_stack_value();
    cast_state( s )->snapshot_mult = 1.0 + priest().buffs.power_of_the_dark_side->check_value();

    if ( ( dbc::get_school_mask( s->action->school ) & SCHOOL_MASK_SHADOW ) == SCHOOL_MASK_SHADOW )
    {
      cast_state( s )->snapshot_mult *= 1.0 + priest().buffs.twilight_equilibrium_shadow_amp->check_value();
    }
    else if ( ( dbc::get_school_mask( s->action->school ) & SCHOOL_MASK_HOLY ) == SCHOOL_MASK_HOLY )
    {
      cast_state( s )->snapshot_mult *= 1.0 + priest().buffs.twilight_equilibrium_holy_amp->check_value();
    }
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t full_duration = dot_duration * s->haste;

    return full_duration;
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    timespan_t t = ab::tick_time( s );
    
    sim->print_debug( "{} default bolts {} state bolts", default_bolts, cast_state( s )->bolts );

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

      state_t* state                   = damage->cast_state( damage->get_state() );
      state->target                    = d->state->target;
      state->snapshot_mult                = cast_state( d->state )->snapshot_mult;
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
    auto idx = static_cast<unsigned>( rng().range( 0, in.size() ) );
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
    priest_spell_t::execute();

    if ( priest().talents.manipulation.enabled() )
    {
      priest().cooldowns.mindgames->adjust( -manipulation_cdr );
    }

    if ( priest().talents.discipline.void_summoner.enabled() )
    {
      priest().cooldowns.fiend->adjust( void_summoner_cdr );
    }

    priest().buffs.power_of_the_dark_side->expire();

    priest().buffs.harsh_discipline->expire();
  }

  void impact( action_state_t* state ) override
  {
    priest_spell_t::impact( state );

    if ( p().talents.discipline.purge_the_wicked.enabled() )
    {
      spread_purge_the_wicked( state, p() );
    }

    priest().trigger_inescapable_torment( state->target );
  }
};

// Main penance action spell
struct penance_t : public priest_spell_t
{
private:
  propagate_const<action_t*> channel;
  propagate_const<action_t*> shadow_covenant_channel;

public:
  penance_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "penance_cast", p, p.specs.penance ),
      channel( new penance_channel_t( p, "penance", p.specs.penance_channel, p.specs.penance_tick ) ),
      shadow_covenant_channel( new penance_channel_t( p, "dark_reprimand", p.talents.discipline.dark_reprimand,
                                                      p.talents.discipline.dark_reprimand->effectN( 2 ).trigger() ) )
  {
    parse_options( options_str );
    cooldown           = p.cooldowns.penance;
    cooldown->duration = p.specs.penance->cooldown();
    school             = SCHOOL_NONE;
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( p().buffs.shadow_covenant->up() )
    {
      shadow_covenant_channel->execute();
    }
    else
    {
      channel->execute();
    }
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
  };

  struct ultimate_penitence_channel_t : public priest_spell_t
  {
    // ultimate_penitence_damage_t
    propagate_const<ultimate_penitence_damage_t*> damage;

    ultimate_penitence_channel_t( priest_t& p )
      : priest_spell_t( "ultimate_penitence_channel", p, p.find_spell( 421434 ) )
    {
      damage = new ultimate_penitence_damage_t( p );
      channeled = true;
      tick_zero = true;

    }

    void tick( dot_t* d ) override
    {
      priest_spell_t::tick( d );

      if ( damage && d->get_tick_factor() >= 1.0 )
      {
        damage->execute_on_target( d->target );
      }
    }
  };

  propagate_const<ultimate_penitence_channel_t*> channel;

public:
  ultimate_penitence_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "ultimate_penitence", p, p.talents.discipline.ultimate_penance )
  {
    parse_options( options_str );
    // Channel = 421434
    // Damage bolt = 421543

    channel = new ultimate_penitence_channel_t( p );
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

  buffs.shadow_covenant = make_buff( this, "shadow_covenant", talents.discipline.shadow_covenant_buff );

  if ( talents.discipline.shadow_covenant.enabled() )
  {
    double scov_amp          = 0;
    timespan_t scov_duration = 15_s;
    if ( talents.shared.mindbender.enabled() )
    {
      scov_amp = 0.1;
      scov_duration = talents.shared.mindbender->duration();
    }
    else
    {
      scov_amp = 0.25;
      scov_duration = talents.shadowfiend->duration();
    }
    buffs.shadow_covenant->set_default_value( scov_amp );
    buffs.shadow_covenant->set_duration( scov_duration );
  }

  // 280391 has the correct 40% damage increase value, but does not apply it to any spells.
  // 280398 applies the damage to the correct spells, but does not contain the correct value (12% instead of 40%).
  // That 12% represents the damage REDUCTION taken from the 40% buff for each attonement that has been applied.
  // TODO: Add support for atonement reductions
  buffs.sins_of_the_many = make_buff( this, "sins_of_the_many", specs.sins_of_the_many )
                               ->set_default_value( find_spell( 280391 )->effectN( 1 ).percent() );

  buffs.twilight_equilibrium_holy_amp =
      make_buff( this, "twilight_equilibrium_holy_amp", talents.discipline.twilight_equilibrium_holy_amp )
          ->set_default_value_from_effect( 1, 0.01 );

  buffs.twilight_equilibrium_shadow_amp =
      make_buff( this, "twilight_equilibrium_shadow_amp", talents.discipline.twilight_equilibrium_shadow_amp )
          ->set_default_value_from_effect( 1, 0.01 );

  buffs.harsh_discipline = make_buff( this, "harsh_discipline", find_spell( 373183 ) )
                               ->set_default_value( talents.discipline.harsh_discipline->effectN( 2 ).base_value() );

  buffs.borrowed_time = make_buff( this, "borrowed_time", find_spell( 390692 ) )->add_invalidate( CACHE_HASTE );
  
  if ( talents.discipline.borrowed_time.ok() )
  {
    buffs.borrowed_time->set_default_value( talents.discipline.borrowed_time->effectN( 2 ).percent() );
  }

  buffs.weal_and_woe = make_buff( this, "weal_and_woe", talents.discipline.weal_and_woe_buff );

  // Discipline T29 2-piece bonus
  buffs.light_weaving = make_buff( this, "light_weaving", find_spell( 394609 ) );
}

void priest_t::init_rng_discipline()
{
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
  talents.discipline.schism                 = ST( "Schism" );
  talents.discipline.schism_debuff          = find_spell( 214621 );
  // Row 4
  talents.discipline.bright_pupil          = ST( "Bright Pupil" );
  talents.discipline.enduring_luminescence = ST( "Enduring Luminescence" );
  talents.discipline.shield_discipline     = ST( "Shield Discipline" );
  talents.discipline.luminous_barrier      = ST( "Luminous Barrier" );
  talents.discipline.power_word_barrier    = ST( "Power Word: Barrier" );
  talents.discipline.painful_punishment    = ST( "Painful Punishment" );
  talents.discipline.malicious_intent      = ST( "Malicious Intent" );
  // Row 5
  talents.discipline.purge_the_wicked     = ST( "Purge the Wicked" );
  talents.discipline.rapture              = ST( "Rapture" );
  talents.discipline.shadow_covenant      = ST( "Shadow Covenant" );
  talents.discipline.shadow_covenant_buff = find_spell( 322105 );
  talents.discipline.dark_reprimand       = find_spell( 373129 );
  // Row 6
  talents.discipline.revel_in_purity     = ST( "Revel in Purity" );
  talents.discipline.contrition          = ST( "Contrition" );
  talents.discipline.exaltation          = ST( "Exaltation" );
  talents.discipline.indemnity           = ST( "Indemnity" );
  talents.discipline.pain_and_suffering  = ST( "Pain and Suffering" );
  talents.discipline.twilight_corruption = ST( "Twilight Corruption" ); //373065
  // Row
  talents.discipline.borrowed_time   = ST( "Borrowed Time" );
  talents.discipline.castigation     = ST( "Castigation" );
  talents.discipline.abyssal_reverie = ST( "Abyssal Reverie" );
  // Row 8
  talents.discipline.train_of_thought = ST( "Train of Thought" );
  talents.discipline.ultimate_penance = ST( "Ultimate Penitence" );
  talents.discipline.lenience         = ST( "Lenience" );
  talents.discipline.evangelism       = ST( "Evangelism" );
  talents.discipline.void_summoner    = ST( "Void Summoner" );
  // Row 9
  talents.discipline.divine_aegis          = ST( "Divine Aegis" );
  talents.discipline.divine_aegis_buff     = find_spell( 47753 );
  talents.discipline.blaze_of_light        = ST( "Blaze of Light" );
  talents.discipline.heavens_wrath         = ST( "Heaven's Wrath" );
  talents.discipline.harsh_discipline      = ST( "Harsh Discipline" );
  talents.discipline.harsh_discipline_buff = find_spell( 373183 );
  talents.discipline.expiation             = ST( "Expiation" );
  // talents.discipline.inescapable_torment   = ST( "Inescapable Torment" ); - Shared Talent
  // Row 10
  talents.discipline.aegis_of_wrath                  = ST( "Aegis of Wrath" );
  talents.discipline.weal_and_woe                    = ST( "Weal and Woe" );
  talents.discipline.weal_and_woe_buff               = find_spell( 390787 );
  talents.discipline.overloaded_with_light           = ST( "Overloded with Light" );
  talents.discipline.twilight_equilibrium            = ST( "Twilight Equilibrium" );
  talents.discipline.twilight_equilibrium_holy_amp   = find_spell( 390706 );
  talents.discipline.twilight_equilibrium_shadow_amp = find_spell( 390707 );
  // talents.discipline.mindbender                      = ST( "Mindbender" ); - Shared Talent

  // General Spells
  specs.sins_of_the_many = find_spell( 280398 );
  specs.penance          = find_spell( 47540 );
  specs.penance_channel  = find_spell( 47758 );  // Channel spell, triggered by 47540, executes 47666 every tick
  specs.penance_tick     = find_spell( 47666 );  // Not triggered from 47540, only 47758
  specs.smite_t31        = find_spell( 425529 ); // T31 Shadow Smite
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
  if ( name == "ultimate_penitence" or name == "uppies" )
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
