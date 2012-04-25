// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "simulationcraft.hpp"
#include "sc_warlock.hpp"

/*
 * To Do:
 * - Check if curse_of_elements is counted for affliction_effects in every possible situation
 *  (optimal_raid=1, or with multiple wl's, dk's/druis), it enhances drain soul damage.
 * - Bane of Agony and haste scaling looks strange (and is crazy as hell, we'll leave it at average base_td for now)
 * - Seed of Corruption with Soulburn: Trigger Corruptions
 * - Execute felguard:felstorm by player, not the pet
 * - Verify if the current incinerate bonus calculation is correct
 * - Investigate pet mp5 - probably needs to scale with owner int
 * - Dismissing a pet drops aura -> demonic_pact. Either don't let it dropt if there is another pet alive, or recheck application when dropping it.
 */

// ==========================================================================
// Warlock
// ==========================================================================

#if SC_WARLOCK == 1

int warlock_targetdata_t::affliction_effects()
{
  int effects = 0;
  if ( dots_agony -> ticking                ) effects++;
  if ( dots_doom -> ticking                 ) effects++;
  if ( dots_corruption -> ticking           ) effects++;
  if ( dots_drain_life -> ticking           ) effects++;
  if ( dots_drain_soul -> ticking           ) effects++;
  if ( dots_unstable_affliction -> ticking  ) effects++;
  if ( dots_malefic_grasp -> ticking        ) effects++;
  if ( debuffs_haunt        -> check()      ) effects++;
  return effects;
}

int warlock_targetdata_t::active_dots()
{
  int dots = 0;
  if ( dots_agony -> ticking                    ) dots++;
  if ( dots_doom -> ticking                     ) dots++;
  if ( dots_corruption -> ticking               ) dots++;
  if ( dots_drain_life -> ticking               ) dots++;
  if ( dots_drain_soul -> ticking               ) dots++;
  if ( dots_immolate -> ticking                 ) dots++;
  if ( dots_shadowflame -> ticking              ) dots++;
  if ( dots_unstable_affliction -> ticking      ) dots++;
  if ( dots_malefic_grasp -> ticking            ) dots++;
  return dots;
}


void register_warlock_targetdata( sim_t* sim )
{
  player_type_e t = WARLOCK;
  typedef warlock_targetdata_t type;

  REGISTER_DOT( corruption );
  REGISTER_DOT( unstable_affliction );
  REGISTER_DOT( agony );
  REGISTER_DOT( doom );
  REGISTER_DOT( immolate );
  REGISTER_DOT( drain_life );
  REGISTER_DOT( drain_soul );
  REGISTER_DOT( shadowflame );
  REGISTER_DOT( malefic_grasp );

  REGISTER_DEBUFF( haunt );
}

warlock_targetdata_t::warlock_targetdata_t( warlock_t* p, player_t* target )
  : targetdata_t( p, target )
{
  debuffs_haunt = add_aura( buff_creator_t( this, "haunt", source -> find_spell( "haunt" ) ) );
}

warlock_t::warlock_t( sim_t* sim, const std::string& name, race_type_e r ) :
  player_t( sim, WARLOCK, name, r ),
  pets( pets_t() ),
  buffs( buffs_t() ),
  cooldowns( cooldowns_t() ),
  talents( talents_t() ),
  mastery_spells( mastery_spells_t() ),
  gains( gains_t() ),
  benefits( benefits_t() ),
  procs( procs_t() ),
  rngs( rngs_t() ),
  glyphs( glyphs_t() ),
  meta_cost_event( 0 ),
  use_pre_soulburn( 0 ),
  initial_burning_embers( 0 ),
  initial_demonic_fury( 200 ),  
  ember_react( timespan_t::zero() )
{
  distance = 40;
  default_distance = 40;

  cooldowns.metamorphosis      = get_cooldown ( "metamorphosis" );
  cooldowns.infernal           = get_cooldown ( "summon_infernal" );
  cooldowns.doomguard          = get_cooldown ( "summon_doomguard" );

  create_options();
}


namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Warlock Heal
// ==========================================================================

struct warlock_heal_t : public heal_t
{
  warlock_heal_t( const std::string& n, warlock_t* p, const uint32_t id ) :
    heal_t( n, p, p -> find_spell( id ) )
  { }

  warlock_t* p() const
  { return static_cast<warlock_t*>( player ); }
};

// ==========================================================================
// Warlock Spell
// ==========================================================================

struct warlock_spell_t : public spell_t
{
private:
  void _init_warlock_spell_t()
  {
    may_crit      = true;
    tick_may_crit = true;
    stateless     = true;
    dot_behavior  = DOT_REFRESH;
    weapon_multiplier = 0.0;
  }

public:
  int recharge_seconds, max_charges, current_charges;

  struct recharge_event_t : event_t
  {
    warlock_spell_t* action;

    recharge_event_t( player_t* p, warlock_spell_t* a ) :
      event_t( p -> sim, p, ( a -> name_str + "_recharge" ).c_str() ), action( a )
    {
      sim -> add_event( this, timespan_t::from_seconds( action -> recharge_seconds ) );
    }

    virtual void execute()
    {
      assert( action -> current_charges < action -> max_charges );
      action -> current_charges++;

      if ( action -> current_charges < action -> max_charges )
      {
        action -> recharge_event = new ( sim ) recharge_event_t( player, action );
      }
      else
      {
        action -> recharge_event = 0;
      }
    }
  };

  recharge_event_t* recharge_event;

  warlock_spell_t( warlock_t* p, const std::string& n, school_type_e sc = SCHOOL_NONE ) :
    spell_t( n, p, p -> find_class_spell( n ), sc ), recharge_seconds( 0 ), max_charges( 0 ), current_charges( 0 ), recharge_event( 0 )
  {
    _init_warlock_spell_t();
  }

  warlock_spell_t( const std::string& token, warlock_t* p, const spell_data_t* s = spell_data_t::nil(), school_type_e sc = SCHOOL_NONE ) :
    spell_t( token, p, s, sc ), recharge_seconds( 0 ), max_charges( 0 ), current_charges( 0 ), recharge_event( 0 )
  {
    _init_warlock_spell_t();
  }

  warlock_t* p() const
  { return static_cast<warlock_t*>( player ); }

  virtual void execute()
  {
    if ( max_charges > 0 )
    {
      assert( current_charges > 0 );
      current_charges--;

      if ( current_charges == max_charges - 1 )
      {
        recharge_event = new ( sim ) recharge_event_t( p(), this );
      }
      else if ( current_charges == 0 )
      {
        assert( recharge_event );
        cooldown -> duration = recharge_event -> time - sim -> current_time;
      }
    }
    spell_t::execute();
  }

  virtual void reset()
  {
    spell_t::reset();

    event_t::cancel( recharge_event );
    current_charges = max_charges;
  }

  // warlock_spell_t::composite_target_ta_multiplier ===================================

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double m = 1.0;
    warlock_targetdata_t* td = targetdata_t::get( player, t ) -> cast_warlock();

    if ( td -> debuffs_haunt -> up() )
    {
      m *= 1.0 + td -> debuffs_haunt -> data().effectN( 3 ).percent();
    }

    return spell_t::composite_target_multiplier( t ) * m;
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    double m = spell_t::action_multiplier( s );

    if ( current_resource() == RESOURCE_BURNING_EMBER )
      m *= 1.0 + 0.1 + floor ( ( p() -> composite_mastery() * p() -> mastery_spells.emberstorm -> effectN( 2 ).base_value() / 10000.0 ) * 1000 ) / 1000;

    return m;
  }

  virtual timespan_t tick_time( double haste ) const
  {
    timespan_t t = spell_t::tick_time( haste );

    warlock_targetdata_t* td = targetdata( target ) -> cast_warlock();

    if ( td -> dots_malefic_grasp -> ticking )
      t /= ( 1.0 + td -> dots_malefic_grasp -> action -> data().effectN( 2 ).percent() );

    return t;
  }

  virtual resource_type_e current_resource() const
  {
    if ( data().powerN( POWER_DEMONIC_FURY ).aura_id() == 54879 && p() -> buffs.metamorphosis -> up() )
      return RESOURCE_DEMONIC_FURY;
    else
      return spell_t::current_resource();
  }

  // trigger_soul_leech =====================================================

  static void trigger_soul_leech( warlock_t* p, double dmg )
  {
    if ( p -> talents.soul_leech -> ok() )
    {
      p -> resource_gain( RESOURCE_HEALTH, dmg * p -> talents.soul_leech -> effectN( 1 ).percent(), p -> gains.soul_leech );
    }
  }

  // trigger_decimation =====================================================

  static void trigger_decimation( warlock_spell_t* s, int result )
  {
    warlock_t* p = s -> p();
    if ( ( result != RESULT_HIT ) && ( result != RESULT_CRIT ) ) return;
    if ( s -> target -> health_percentage() > p -> spec.decimation -> effectN( 2 ).base_value() ) return;
    p -> buffs.decimation -> trigger();
  }

  // trigger_malefic_grasp =====================================================

  static void start_malefic_grasp( warlock_spell_t* mg, dot_t* dot )
  {
    if ( dot -> ticking )
    {
      dot -> tick_event -> reschedule( ( mg -> sim -> current_time - dot -> tick_event -> time )
                                       / ( 1.0 + mg -> data().effectN( 2 ).percent() ) );
    }
  }

  static void stop_malefic_grasp( warlock_spell_t* mg, dot_t* dot )
  {
    if ( dot -> ticking )
    {
      dot -> tick_event -> reschedule( ( mg -> sim -> current_time - dot -> tick_event -> time )
                                       * ( 1.0 + mg -> data().effectN( 2 ).percent() ) );
    }
  }
};

// Curse of Elements Spell ==================================================

struct curse_of_elements_t : public warlock_spell_t
{
  curse_of_elements_t( warlock_t* p ) :
    warlock_spell_t( p, "Curse of the Elements" )
  {
    background = ( sim -> overrides.magic_vulnerability != 0 );
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( result_is_hit() )
    {
      if ( ! sim -> overrides.magic_vulnerability )
        target -> debuffs.magic_vulnerability -> trigger( 1, -1, -1, data().duration() );
    }
  }
};

// Bane of Agony Spell ======================================================

struct agony_t : public warlock_spell_t
{
  int damage_level;

  agony_t( warlock_t* p ) :
    warlock_spell_t( p, "Agony" ), damage_level( 0 )
  {
    may_crit = false;
    tick_power_mod = 0.02; // from tooltip
    dot_behavior = DOT_EXTEND;
  }

  virtual void last_tick( dot_t* d )
  {
    warlock_spell_t::last_tick( d );
    damage_level = 0;
  }

  virtual void tick( dot_t* d )
  {
    if ( damage_level < 10 ) damage_level++;
    warlock_spell_t::tick( d );
  }

  virtual double calculate_tick_damage( result_type_e r, double p, double m )
  {
    return warlock_spell_t::calculate_tick_damage( r, p, m ) * ( 70 + 5 * damage_level ) / 12;
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    double m = warlock_spell_t::action_multiplier( s );

    if ( p() -> mastery_spells.potent_afflictions -> ok() )
    {
      m *= 1.0 + floor ( ( p() -> composite_mastery() * p() -> mastery_spells.potent_afflictions -> effectN( 2 ).base_value() / 10000.0 ) * 1000 ) / 1000;
    }

    return m;
  }
};

// Bane of Doom Spell =======================================================

struct doom_t : public warlock_spell_t
{
  // FIXME: Activating the glyph of doom currently puts the spell in the spellbook for all specs, but it's only castable as affliction
  doom_t( warlock_t* p ) :
    warlock_spell_t( "doom", p, ( p -> primary_tree() == WARLOCK_AFFLICTION && p -> glyphs.doom -> ok() ) ? p -> find_spell( 603 ) : spell_data_t::not_found() )
  {
    hasted_ticks = false;
    may_crit = false;
    tick_power_mod = 1.0; // from tooltip
    dot_behavior = DOT_EXTEND;
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    double m = warlock_spell_t::action_multiplier( s );

    if ( p() -> mastery_spells.potent_afflictions -> ok() )
    {
      m *= 1.0 + floor ( ( p() -> composite_mastery() * p() -> mastery_spells.potent_afflictions -> effectN( 2 ).base_value() / 10000.0 ) * 1000 ) / 1000;
    }

    return m;
  }
};

// Bane of Havoc Spell ======================================================

struct bane_of_havoc_t : public warlock_spell_t
{
  bane_of_havoc_t( warlock_t* p ) : warlock_spell_t( p, "Bane of Havoc" ) { }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( result_is_hit() )
    {
      p() -> buffs.bane_of_havoc -> trigger();
    }

  }

  virtual bool ready()
  {
    if ( p() -> buffs.bane_of_havoc -> check() )
      return false;

    return warlock_spell_t::ready();
  }
};

// Shadow Bolt Spell ========================================================

struct shadow_bolt_t : public warlock_spell_t
{
  shadow_bolt_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Shadow Bolt" )
  {
    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new shadow_bolt_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }


  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_decimation( this, s -> result );

      trigger_soul_leech( p(), s -> result_amount );
    }
  }
};

// Shadow Burn Spell ========================================================

struct shadowburn_t : public warlock_spell_t
{
  shadowburn_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Shadowburn" )
  {
    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new shadowburn_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual bool ready()
  {
    if ( target -> health_percentage() >= 20 )
      return false;

    return warlock_spell_t::ready();
  }
};

// Corruption Spell =========================================================

struct corruption_t : public warlock_spell_t
{
  corruption_t( warlock_t* p ) :
    warlock_spell_t( "corruption", p, p -> glyphs.doom -> ok() ? spell_data_t::not_found() : p -> find_class_spell( "Corruption" ) )
  {
    may_crit = false;
    tick_power_mod = 0.2; // from tooltip
    dot_behavior = DOT_EXTEND;
  };

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    if ( p() -> rngs.nightfall -> roll( p() -> spec.nightfall -> proc_chance() ) )
    {
      p() -> procs.shadow_trance -> occur();
      p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.nightfall );
    }
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    double m = warlock_spell_t::action_multiplier( s );

    if ( p() -> mastery_spells.potent_afflictions -> ok() )
    {
      m *= 1.0 + floor ( ( p() -> composite_mastery() * p() -> mastery_spells.potent_afflictions -> effectN( 2 ).base_value() / 10000.0 ) * 1000 ) / 1000;
    }

    return m;
  }
};

// Drain Life Spell =========================================================

struct drain_life_heal_t : public warlock_heal_t
{
  drain_life_heal_t( warlock_t* p ) :
    warlock_heal_t( "drain_life_heal", p, 89653 )
  {
    background = true;
    may_miss = false;
    base_dd_min = base_dd_max = 0; // Is parsed as 2
  }

  virtual void execute()
  {
    double heal_pct = data().effectN( 1 ).percent();
    base_dd_min = base_dd_max = player -> resources.max[ RESOURCE_HEALTH ] * heal_pct;

    warlock_heal_t::execute();
  }
};

struct drain_life_t : public warlock_spell_t
{
  drain_life_heal_t* heal;

  drain_life_t( warlock_t* p ) :
    warlock_spell_t( p, "Drain Life" ), heal( 0 )
  {
    channeled    = true;
    hasted_ticks = false;
    may_crit     = false;

    heal = new drain_life_heal_t( p );
  }

  virtual void last_tick( dot_t* d )
  {
    warlock_spell_t::last_tick( d );

    if ( p() -> buffs.soulburn -> check() )
      p() -> buffs.soulburn -> expire();
  }

  virtual timespan_t tick_time( double haste ) const
  {
    timespan_t t = warlock_spell_t::tick_time( haste );

    if ( p() -> buffs.soulburn -> up() )
      t *= 1.0 - 0.5;

    return t;
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    heal -> execute();
  }
};

// Drain Soul Spell =========================================================

struct drain_soul_t : public warlock_spell_t
{
  drain_soul_t( warlock_t* p ) :
    warlock_spell_t( p, "Drain Soul" )
  {
    channeled    = true;
    hasted_ticks = true; // informative
    may_crit     = false;
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    double m = warlock_spell_t::action_multiplier( s );

    if ( s -> target -> health_percentage() < data().effectN( 3 ).base_value() )
    {
      m *= 2.0;
    }

    return m;
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.drain_soul );
  }
};

// Unstable Affliction Spell ================================================

struct unstable_affliction_t : public warlock_spell_t
{
  unstable_affliction_t( warlock_t* p ) :
    warlock_spell_t( p, "Unstable Affliction" )
  {
    may_crit   = false;
    tick_power_mod = 0.2; // from tooltip
    dot_behavior = DOT_EXTEND;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( result_is_hit() )
    {
      // FIXME: move to impact
      warlock_targetdata_t* td = targetdata( target ) -> cast_warlock();
      if ( td -> dots_immolate -> ticking )
        td -> dots_immolate -> cancel();
    }
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    double m = warlock_spell_t::action_multiplier( s );

    if ( p() -> mastery_spells.potent_afflictions -> ok() )
    {
      m *= 1.0 + floor ( ( p() -> composite_mastery() * p() -> mastery_spells.potent_afflictions -> effectN( 2 ).base_value() / 10000.0 ) * 1000 ) / 1000;
    }

    return m;
  }
};

// Haunt Spell ==============================================================

struct haunt_t : public warlock_spell_t
{
  haunt_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Haunt" )
  {
    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new haunt_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      warlock_targetdata_t* td = targetdata( s -> target ) -> cast_warlock();
      td -> debuffs_haunt -> trigger();
    }
  }
};

// Immolate Spell ===========================================================

struct immolate_t : public warlock_spell_t
{
  immolate_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Immolate" )
  {
    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new immolate_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }
};

// Shadowflame DOT Spell ====================================================

struct shadowflame_t : public warlock_spell_t
{
  shadowflame_t( warlock_t* p ) :
    warlock_spell_t( p, "Shadowflame" )
  {
    proc       = true;
    background = true;
  }
};

// Conflagrate Spell ========================================================

struct conflagrate_t : public warlock_spell_t
{
  conflagrate_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Conflagrate" )
  {
    current_charges = max_charges = 2;
    recharge_seconds = 12;

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new conflagrate_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) && p() -> spec.backdraft -> ok() )
      p() -> buffs.backdraft -> trigger( 3 );
  }
};

// Incinerate Spell =========================================================

struct incinerate_t : public warlock_spell_t
{
  incinerate_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Incinerate" )
  {
    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new incinerate_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    for ( int i=0; i < 4; i++ )
    {
      p() -> benefits.backdraft[ i ] -> update( i == p() -> buffs.backdraft -> check() );
    }

    if ( p() -> buffs.backdraft -> check() )
    {
      p() -> buffs.backdraft -> decrement();
    }
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      warlock_targetdata_t* td = targetdata( s -> target ) -> cast_warlock();

      if ( td -> dots_immolate -> ticking )
      {
        double gain_amount = ( s -> result == RESULT_CRIT ) ? 2 : 1;
        p() -> resource_gain( RESOURCE_BURNING_EMBER, gain_amount, p() -> gains.incinerate );

        // If this gain was a crit that brought us from 8 to 10, that is a surprise the player would have to react to
        if ( gain_amount == 2 && p() -> resources.current[ RESOURCE_BURNING_EMBER ] == 10 )
          p() -> ember_react = sim -> current_time + p() -> total_reaction_time();
        else if ( p() -> resources.current[ RESOURCE_BURNING_EMBER ] >= 10 )
          p() -> ember_react = sim -> current_time;

      }

      trigger_soul_leech( p(), s -> result_amount );
    }
  }

  virtual timespan_t execute_time() const
  {
    timespan_t h = warlock_spell_t::execute_time();

    if ( p() -> buffs.backdraft -> up() )
    {
      h *= 1.0 + p() -> buffs.backdraft -> data().effectN( 1 ).percent();
    }

    return h;
  }
};


// Soul Fire Spell ==========================================================

struct soul_fire_t : public warlock_spell_t
{
  soul_fire_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Soul Fire" )
  {
    if ( p -> mastery_spells.emberstorm -> ok() ) base_costs[ RESOURCE_BURNING_EMBER ] = 10;

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new soul_fire_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( p() -> buffs.soulburn -> check() )
    {
      p() -> buffs.soulburn -> expire();
      if ( p() -> set_bonus.tier13_4pc_caster() )
      {
        p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.tier13_4pc );
      }
    }
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = warlock_spell_t::execute_time();

    if ( p() -> buffs.decimation -> up() )
    {
      t *= 1.0 - p() -> spec.decimation -> effectN( 1 ).percent();
    }
    if ( p() -> buffs.soulburn -> up() )
    {
      t = timespan_t::zero();
    }

    return t;
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    trigger_decimation( this, s -> result );
  }

  virtual resource_type_e current_resource() const
  {
    if ( p() -> mastery_spells.emberstorm -> ok() && p() -> resources.current[ RESOURCE_BURNING_EMBER ] >= base_costs[ RESOURCE_BURNING_EMBER ] )
    {
      return RESOURCE_BURNING_EMBER;
    }
    else
    {
      return warlock_spell_t::current_resource();
    }
  }

  virtual result_type_e calculate_result( double crit, unsigned int level )
  {
    result_type_e r = warlock_spell_t::calculate_result( crit, level );

    // Soul fire always crits
    if ( result_is_hit( r ) ) return RESULT_CRIT;

    return r;
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    double m = warlock_spell_t::action_multiplier( s );

    if ( p() -> buffs.soulburn -> check() )
      m *= p() -> find_spell( 104240 ) -> effectN( 1 ).min( p() ) / data().effectN( 1 ).min( p() );

    // FIXME: This may be a bug, but on beta it's currently (2012/04/24) not scaling with crit when using embers
    if ( current_resource() != RESOURCE_BURNING_EMBER )
      m *= 1.0 + ( p() -> composite_spell_crit() - p() -> intellect() / p() -> stats_current.spell_crit_per_intellect / 100.0 );

    return m;
  }
};

// Life Tap Spell ===========================================================

struct life_tap_t : public warlock_spell_t
{
  life_tap_t( warlock_t* p ) :
    warlock_spell_t( p, "Life Tap" )
  {
    harmful = false;

    if ( p -> glyphs.life_tap -> ok() )
      trigger_gcd += p -> glyphs.life_tap -> effectN( 1 ).time_value();
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    player -> resource_loss( RESOURCE_HEALTH, player -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 3 ).percent() );
    player -> resource_gain( RESOURCE_MANA, player -> resources.max[ RESOURCE_MANA ] * data().effectN( 1 ).percent() / 10, p() -> gains.life_tap );
  }
};


// Metamorphosis Spell ======================================================

struct metamorphosis_t : public warlock_spell_t
{
  metamorphosis_t( warlock_t* p ) :
    warlock_spell_t( p, "Metamorphosis" )
  {
    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    p() -> trigger_metamorphosis();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.metamorphosis -> up() ) return false;
    if ( p() -> resources.current[ RESOURCE_DEMONIC_FURY ] <= 0 ) return false;

    return warlock_spell_t::ready();
  }
};


// Hand of Gul'dan Spell ====================================================

struct hand_of_guldan_t : public warlock_spell_t
{
  hand_of_guldan_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Hand of Gul'dan" )
  {
    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new hand_of_guldan_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual timespan_t travel_time() const
  {
    return timespan_t::from_seconds( 1.5 );
  }
};

// Fel Flame Spell ==========================================================

struct fel_flame_t : public warlock_spell_t
{
  fel_flame_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Fel Flame" )
  {
    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new fel_flame_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      //FIXME: Exact mechanic needs testing - seems inconsistent, particularly for Doom
      warlock_targetdata_t* td = targetdata( s -> target ) -> cast_warlock();
      td -> dots_corruption          -> extend_duration( 2, true );
      td -> dots_doom                -> extend_duration( 1, true );
      td -> dots_immolate            -> extend_duration( 2, true );
      td -> dots_unstable_affliction -> extend_duration( 2, true );
    }
  }
};

// Malefic Grasp Spell ==========================================================

struct malefic_grasp_t : public warlock_spell_t
{
  malefic_grasp_t( warlock_t* p ) :
    warlock_spell_t( p, "Malefic Grasp" )
  {
    channeled    = true;
    hasted_ticks = true;
    may_crit     = false;
  }

  virtual void last_tick( dot_t* d )
  {
    warlock_spell_t::last_tick( d );

    warlock_targetdata_t* td = targetdata( target ) -> cast_warlock();

    stop_malefic_grasp( this, td -> dots_agony );
    stop_malefic_grasp( this, td -> dots_corruption );
    stop_malefic_grasp( this, td -> dots_doom );
    stop_malefic_grasp( this, td -> dots_unstable_affliction );
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      warlock_targetdata_t* td = targetdata( s -> target ) -> cast_warlock();
      start_malefic_grasp( this, td -> dots_agony );
      start_malefic_grasp( this, td -> dots_corruption );
      start_malefic_grasp( this, td -> dots_doom );
      start_malefic_grasp( this, td -> dots_unstable_affliction );
    }
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    trigger_soul_leech( p(), d -> state -> result_amount );
  }
};

// Dark Intent Spell ========================================================

struct dark_intent_t : public warlock_spell_t
{
  dark_intent_t( warlock_t* p ) :
    warlock_spell_t( p, "Dark Intent" )
  {
    harmful = false;
    background = ( sim -> overrides.spell_power_multiplier != 0 );
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( ! sim -> overrides.spell_power_multiplier )
      sim -> auras.spell_power_multiplier -> trigger();
  }
};

// Soulburn Spell ===========================================================

struct soulburn_t : public warlock_spell_t
{
  soulburn_t( warlock_t* p ) :
    warlock_spell_t( p, "Soulburn" )
  {
    harmful = false;
  }

  virtual void execute()
  {
    if ( p() -> use_pre_soulburn || player -> in_combat )
    {
      p() -> buffs.soulburn -> trigger();
      p() -> buffs.tier13_4pc_caster -> trigger();
      // If this was a pre-combat soulburn, ensure we model the 3 seconds needed to regenerate the soul shard
      if ( ! player -> in_combat )
      {
        p() -> buffs.soulburn -> extend_duration( player, timespan_t::from_seconds( -3 ) );
        if ( p() -> buffs.tier13_4pc_caster -> check() )
          p() -> buffs.tier13_4pc_caster -> extend_duration( player, timespan_t::from_seconds( -3 ) );
      }
    }

    warlock_spell_t::execute();
  }
};

// Dark Soul Spell ===========================================================

struct dark_soul_t : public warlock_spell_t
{
  dark_soul_t( warlock_t* p ) :
    warlock_spell_t( "dark_soul", p, p -> spec.dark_soul )
  {
    harmful = false;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    p() -> buffs.dark_soul -> trigger();
  }
};

// AOE SPELLS

// Hellfire Effect Spell ====================================================

struct hellfire_tick_t : public warlock_spell_t
{
  hellfire_tick_t( warlock_t* p ) :
    warlock_spell_t( "hellfire_tick", p, p -> find_spell( 5857 ) )
  {
    dual        = true;
    background  = true;
    aoe         = -1;
    direct_tick = true;
  }
};

// Hellfire Spell ===========================================================

struct hellfire_t : public warlock_spell_t
{
  hellfire_tick_t* hellfire_tick;

  hellfire_t( warlock_t* p ) :
    warlock_spell_t( p, "Hellfire" ), hellfire_tick( 0 )
  {
    // Hellfire has it's own damage effect, which is actually the damage to the player himself, so harmful is set to false.
    harmful = false;

    channeled    = true;
    hasted_ticks = false;

    hellfire_tick = new hellfire_tick_t( p );
  }

  virtual void init()
  {
    warlock_spell_t::init();

    hellfire_tick -> stats = stats;
  }

  virtual bool usable_moving()
  { return false; }

  virtual void tick( dot_t* /* d */ )
  {
    hellfire_tick -> execute();
  }
};
// Seed of Corruption AOE Spell =============================================

struct seed_of_corruption_aoe_t : public warlock_spell_t
{
  seed_of_corruption_aoe_t( warlock_t* p ) :
    warlock_spell_t( "seed_of_corruption_aoe", p, p -> find_spell( 27285 ) )
  {
    proc       = true;
    background = true;
    aoe        = -1;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( p() -> buffs.soulburn -> check() )
    {
      // Trigger Multiple Corruptions
      p() -> buffs.soulburn -> expire();
    }
  }
};

// Seed of Corruption Spell =================================================

struct seed_of_corruption_t : public warlock_spell_t
{
  seed_of_corruption_aoe_t* seed_of_corruption_aoe;
  double dot_damage_done;

  seed_of_corruption_t( warlock_t* p ) :
    warlock_spell_t( p, "Seed of Corruption" ),
    seed_of_corruption_aoe( 0 ), dot_damage_done( 0 )
  {
    seed_of_corruption_aoe = new seed_of_corruption_aoe_t( p );
    add_child( seed_of_corruption_aoe );
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      warlock_targetdata_t* td = targetdata( s -> target ) -> cast_warlock();
      dot_damage_done = s -> target -> iteration_dmg_taken;
      if ( td -> dots_corruption -> ticking )
      {
        td -> dots_corruption -> cancel();
      }
    }
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    if ( target -> iteration_dmg_taken - dot_damage_done > data().effectN( 2 ).base_value() )
    {
      dot_damage_done=0.0;
      seed_of_corruption_aoe -> execute();
      spell_t::cancel();
    }
  }
};

// Rain of Fire Tick Spell ==================================================

struct rain_of_fire_tick_t : public warlock_spell_t
{
  rain_of_fire_tick_t( warlock_t* p ) :
    warlock_spell_t( "rain_of_fire_tick", p, p -> find_spell( 42223 ) )
  {
    background  = true;
    aoe         = -1;
    direct_tick = true;
  }
};

// Rain of Fire Spell =======================================================

struct rain_of_fire_t : public warlock_spell_t
{
  rain_of_fire_tick_t* rain_of_fire_tick;

  rain_of_fire_t( warlock_t* p ) :
    warlock_spell_t( p, "Rain of Fire" ),
    rain_of_fire_tick( 0 )
  {
    harmful = false;
    channeled = true;

    rain_of_fire_tick = new rain_of_fire_tick_t( p );

    add_child( rain_of_fire_tick );
  }

  virtual void init()
  {
    warlock_spell_t::init();

    rain_of_fire_tick -> stats = stats;
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    rain_of_fire_tick -> execute();
  }
};

// PET SPELLS

// Summon Pet Spell =========================================================

struct summon_pet_t : public warlock_spell_t
{
  timespan_t summoning_duration;
  pet_t* pet;

private:
  void _init_summon_pet_t( const std::string& pet_name )
  {
    harmful = false;

    pet = player -> find_pet( pet_name );
    if ( ! pet )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", player -> name(), pet_name.c_str() );
    }
  }

public:
  summon_pet_t( const std::string& n, warlock_t* p, const std::string& sname ) :
    warlock_spell_t( p, sname ), summoning_duration ( timespan_t::zero() ), pet( 0 )
  {
    _init_summon_pet_t( n );
  }

  summon_pet_t( const std::string& n, warlock_t* p, int id ) :
    warlock_spell_t( n, p, p -> find_spell( id ) ), summoning_duration ( timespan_t::zero() ), pet( 0 )
  {
    _init_summon_pet_t( n );
  }

  summon_pet_t( const std::string& n, warlock_t* p, const spell_data_t* sd = spell_data_t::nil() ) :
    warlock_spell_t( n, p, sd ), summoning_duration ( timespan_t::zero() ), pet( 0 )
  {
    _init_summon_pet_t( n );
  }

  virtual void execute()
  {
    pet -> summon( summoning_duration );

    warlock_spell_t::execute();
  }
};

// Summon Main Pet Spell ====================================================

struct summon_main_pet_t : public summon_pet_t
{
  summon_main_pet_t( const char* n, warlock_t* p, const char* sname ) :
    summon_pet_t( n, p, sname )
  { }

  virtual void schedule_execute()
  {
    warlock_spell_t::schedule_execute();

    if ( p() -> pets.active )
    {
      p() -> pets.active -> dismiss();
      p() -> pets.active = 0;
    }
  }

  virtual bool ready()
  {
    if ( p() -> pets.active == pet )
      return false;

    return summon_pet_t::ready();
  }

  virtual timespan_t execute_time() const
  {
    if ( p() -> buffs.soulburn -> up() )
      return timespan_t::zero();

    return warlock_spell_t::execute_time();
  }

  virtual void execute()
  {
    summon_pet_t::execute();

    p() -> pets.active = ( warlock_main_pet_t* ) pet;

    if ( p() -> buffs.soulburn -> check() )
      p() -> buffs.soulburn -> expire();

    if ( p() -> buffs.grimoire_of_sacrifice -> check() )
      p() -> buffs.grimoire_of_sacrifice -> expire();
  }
};

struct summon_felhunter_t : public summon_main_pet_t
{
  summon_felhunter_t( warlock_t* p ) :
    summon_main_pet_t( "felhunter", p, "Summon Felhunter" )
  { }
};

struct summon_felguard_t : public summon_main_pet_t
{
  summon_felguard_t( warlock_t* p ) :
    summon_main_pet_t( "felguard", p, "Summon Felguard" )
  { }
};

struct summon_succubus_t : public summon_main_pet_t
{
  summon_succubus_t( warlock_t* p ) :
    summon_main_pet_t( "succubus", p, "Summon Succubus" )
  { }
};

struct summon_imp_t : public summon_main_pet_t
{
  summon_imp_t( warlock_t* p ) :
    summon_main_pet_t( "imp", p, "Summon Imp" )
  { }
};

struct summon_voidwalker_t : public summon_main_pet_t
{
  summon_voidwalker_t( warlock_t* p ) :
    summon_main_pet_t( "voidwalker", p, "Summon Voidwalker" )
  { }
};

// Infernal Awakening =======================================================

struct infernal_awakening_t : public warlock_spell_t
{
  infernal_awakening_t( warlock_t* p ) :
    warlock_spell_t( "infernal_awakening", p, p -> find_spell( 22703 ) )
  {
    aoe        = -1;
    background = true;
    proc       = true;
    trigger_gcd= timespan_t::zero();
  }
};

// Summon Infernal Spell ====================================================

struct summon_infernal_t : public summon_pet_t
{
  infernal_awakening_t* infernal_awakening;

  summon_infernal_t( warlock_t* p  ) :
    summon_pet_t( "infernal", p, "Summon Infernal" ),
    infernal_awakening( 0 )
  {
    cooldown -> duration += ( p -> set_bonus.tier13_2pc_caster() ) ? timespan_t::from_millis( p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 3 ).base_value() ) : timespan_t::zero();

    summoning_duration = timespan_t::from_seconds( 60 );
    summoning_duration += ( p -> set_bonus.tier13_2pc_caster() ) ?
                          ( p -> primary_tree() == WARLOCK_DEMONOLOGY ?
                            timespan_t::from_seconds( p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 1 ).base_value() ) :
                            timespan_t::from_seconds( p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 2 ).base_value() ) ) : timespan_t::zero();

    infernal_awakening = new infernal_awakening_t( p );
  }

  virtual void execute()
  {
    if ( infernal_awakening )
      infernal_awakening -> execute();

    p() -> cooldowns.doomguard -> start();

    summon_pet_t::execute();
  }
};

// Summon Doomguard2 Spell ==================================================

struct summon_doomguard2_t : public summon_pet_t
{
  summon_doomguard2_t( warlock_t* p ) :
    summon_pet_t( "doomguard", p, 60478 )
  {
    harmful = false;
    background = true;
    summoning_duration = timespan_t::from_seconds( 60 );
    summoning_duration += ( p -> set_bonus.tier13_2pc_caster() ) ?
                          ( p -> primary_tree() == WARLOCK_DEMONOLOGY ?
                            timespan_t::from_seconds( p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 1 ).base_value() ) :
                            timespan_t::from_seconds( p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 2 ).base_value() ) ) : timespan_t::zero();
  }

  virtual void execute()
  {
    p() -> cooldowns.infernal -> start();

    summon_pet_t::execute();
  }
};

// Summon Doomguard Spell ===================================================

struct summon_doomguard_t : public warlock_spell_t
{
  summon_doomguard2_t* summon_doomguard2;

  summon_doomguard_t( warlock_t* p ) :
    warlock_spell_t( p, "Summon Doomguard" ),
    summon_doomguard2( 0 )
  {
    cooldown -> duration += ( p -> set_bonus.tier13_2pc_caster() ) ? timespan_t::from_millis( p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 3 ).base_value() ) : timespan_t::zero();

    harmful = false;
    summon_doomguard2 = new summon_doomguard2_t( p );
  }

  virtual void execute()
  {
    consume_resource();
    update_ready();

    p() -> cooldowns.infernal -> start();

    summon_doomguard2 -> execute();
  }
};

// TALENT SPELLS

// Shadowfury Spell =========================================================

struct shadowfury_t : public warlock_spell_t
{
  shadowfury_t( warlock_t* p ) :
    warlock_spell_t( "shadowfury", p, p -> talents.shadowfury )
  {  }
};

// Mortal Coil Spell =========================================================

struct mortal_coil_heal_t : public warlock_heal_t
{
  mortal_coil_heal_t( warlock_t* p ) :
    warlock_heal_t( "mortal_coil_heal", p, 108396 )
  {
    background = true;
    may_miss = false;
  }

  virtual void execute()
  {
    double heal_pct = data().effectN( 1 ).percent();
    base_dd_min = base_dd_max = player -> resources.max[ RESOURCE_HEALTH ] * heal_pct;

    warlock_heal_t::execute();
  }
};

struct mortal_coil_t : public warlock_spell_t
{
  mortal_coil_heal_t* heal;

  mortal_coil_t( warlock_t* p ) :
    warlock_spell_t( "mortal_coil", p, p -> talents.mortal_coil ), heal( 0 )
  {
    heal = new mortal_coil_heal_t( p );
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
      heal -> execute();
  }
};

// Grimoire of Sacrifice ====================================================

struct grimoire_of_sacrifice_t : public warlock_spell_t
{
  struct decrement_event_t : public event_t
  {
    buff_t* buff;

    decrement_event_t( warlock_t* p, buff_t* b ) :
      event_t( p -> sim, p, "grimoire_of_sacrifice_decrement" ), buff( b )
    {
      sim -> add_event( this, timespan_t::from_seconds( 15 ) );
    }

    virtual void execute()
    {
      if ( buff -> stack() == 2 ) buff -> decrement();
    }
  };

  decrement_event_t* decrement_event;

  grimoire_of_sacrifice_t( warlock_t* p ) :
    warlock_spell_t( "grimoire_of_sacrifice", p, p -> talents.grimoire_of_sacrifice ), decrement_event( 0 )
  {
    harmful = false;
  }

  virtual bool ready()
  {
    if ( ! p() -> pets.active ) return false;

    return warlock_spell_t::ready();
  }

  virtual void execute()
  {
    if ( p() -> pets.active )
    {
      warlock_spell_t::execute();

      p() -> pets.active -> dismiss();
      p() -> buffs.grimoire_of_sacrifice -> trigger( 2 );
      decrement_event = new ( sim ) decrement_event_t( p(), p() -> buffs.grimoire_of_sacrifice );
    }
  }
};

// Grimoire of Service ====================================================

struct grimoire_of_service_t : public summon_pet_t
{
  grimoire_of_service_t( warlock_t* p, const std::string& pet_name ) :
    summon_pet_t( pet_name, p, p -> talents.grimoire_of_service )
  {
    summoning_duration = timespan_t::from_seconds( 30 );
  }
};

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Warlock Character Definition
// ==========================================================================

// warlock_t::composite_spell_power_multiplier ==============================

double warlock_t::composite_spell_power_multiplier() const
{
  double m = player_t::composite_spell_power_multiplier();

  if ( buffs.tier13_4pc_caster -> up() )
  {
    m *= 1.0 + sets -> set ( SET_T13_4PC_CASTER ) -> effect1().percent();
  }

  return m;
}

// warlock_t::composite_player_multiplier ===================================

double warlock_t::composite_player_multiplier( school_type_e school, const action_t* a ) const
{
  double player_multiplier = player_t::composite_player_multiplier( school, a );

  double mastery_value = mastery_spells.master_demonologist -> effectN( 3 ).base_value();

  if ( buffs.metamorphosis -> up() )
  {
    player_multiplier *= 1.0 + spec.metamorphosis -> effectN( 3 ).percent()
                         + ( buffs.metamorphosis -> value() * mastery_value / 10000.0 );
  }

  player_multiplier *= 1.0 + talents.grimoire_of_sacrifice -> effectN( 2 ).percent() * buffs.grimoire_of_sacrifice -> stack();

  return player_multiplier;
}

double warlock_t::composite_spell_crit() const
{
  double sc = player_t::composite_spell_crit();

  if ( primary_tree() == WARLOCK_DESTRUCTION )
  {
    if ( buffs.dark_soul -> up() )
      sc += spec.dark_soul -> effectN( 1 ).percent();
    else if ( buffs.dark_soul -> cooldown -> remains() == timespan_t::zero() )
      sc += spec.dark_soul -> effectN( 1 ).percent() / 6;
  }

  return sc;
}

double warlock_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( primary_tree() == WARLOCK_AFFLICTION )
  {
    if ( buffs.dark_soul -> up() )
      h *= 1.0 / ( 1.0 + spec.dark_soul -> effectN( 1 ).percent() );
    else if ( buffs.dark_soul -> cooldown -> remains() == timespan_t::zero() )
      h *= 1.0 / ( 1.0 + spec.dark_soul -> effectN( 1 ).percent() / 6 );
  }

  return h;
}

double warlock_t::composite_mastery() const
{
  double m = player_t::composite_mastery();

  if ( primary_tree() == WARLOCK_DEMONOLOGY )
  {
    if ( buffs.dark_soul -> up() )
      m += spec.dark_soul -> effectN( 1 ).base_value();
    else if ( buffs.dark_soul -> cooldown -> remains() == timespan_t::zero() )
      m += spec.dark_soul -> effectN( 1 ).base_value() / 6;
  }

  return m;
}

// warlock_t::matching_gear_multiplier ======================================

double warlock_t::matching_gear_multiplier( const attribute_type_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return 0.05;

  return 0.0;
}

// warlock_t::create_action =================================================

action_t* warlock_t::create_action( const std::string& name,
                                    const std::string& options_str )
{
  action_t* a;

  if      ( name == "conflagrate"           ) a = new           conflagrate_t( this );
  else if ( name == "corruption"            ) a = new            corruption_t( this );
  else if ( name == "agony"                 ) a = new                 agony_t( this );
  else if ( name == "doom"                  ) a = new                  doom_t( this );
  else if ( name == "curse_of_elements"     ) a = new     curse_of_elements_t( this );
  else if ( name == "drain_life"            ) a = new            drain_life_t( this );
  else if ( name == "drain_soul"            ) a = new            drain_soul_t( this );
  else if ( name == "grimoire_of_sacrifice" ) a = new grimoire_of_sacrifice_t( this );
  else if ( name == "haunt"                 ) a = new                 haunt_t( this );
  else if ( name == "immolate"              ) a = new              immolate_t( this );
  else if ( name == "incinerate"            ) a = new            incinerate_t( this );
  else if ( name == "life_tap"              ) a = new              life_tap_t( this );
  else if ( name == "malefic_grasp"         ) a = new         malefic_grasp_t( this );
  else if ( name == "metamorphosis"         ) a = new         metamorphosis_t( this );
  else if ( name == "mortal_coil"           ) a = new           mortal_coil_t( this );
  else if ( name == "shadow_bolt"           ) a = new           shadow_bolt_t( this );
  else if ( name == "shadowburn"            ) a = new            shadowburn_t( this );
  else if ( name == "shadowfury"            ) a = new            shadowfury_t( this );
  else if ( name == "soul_fire"             ) a = new             soul_fire_t( this );
  else if ( name == "summon_felhunter"      ) a = new      summon_felhunter_t( this );
  else if ( name == "summon_felguard"       ) a = new       summon_felguard_t( this );
  else if ( name == "summon_succubus"       ) a = new       summon_succubus_t( this );
  else if ( name == "summon_voidwalker"     ) a = new     summon_voidwalker_t( this );
  else if ( name == "summon_imp"            ) a = new            summon_imp_t( this );
  else if ( name == "summon_infernal"       ) a = new       summon_infernal_t( this );
  else if ( name == "summon_doomguard"      ) a = new      summon_doomguard_t( this );
  else if ( name == "unstable_affliction"   ) a = new   unstable_affliction_t( this );
  else if ( name == "hand_of_guldan"        ) a = new        hand_of_guldan_t( this );
  else if ( name == "fel_flame"             ) a = new             fel_flame_t( this );
  else if ( name == "dark_intent"           ) a = new           dark_intent_t( this );
  else if ( name == "dark_soul"             ) a = new             dark_soul_t( this );
  else if ( name == "soulburn"              ) a = new              soulburn_t( this );
  else if ( name == "bane_of_havoc"         ) a = new         bane_of_havoc_t( this );
  else if ( name == "hellfire"              ) a = new              hellfire_t( this );
  else if ( name == "seed_of_corruption"    ) a = new    seed_of_corruption_t( this );
  else if ( name == "rain_of_fire"          ) a = new          rain_of_fire_t( this );
  else if ( name == "grimoire_of_service_felguard"   ) a = new grimoire_of_service_t( this, name );
  else if ( name == "grimoire_of_service_felhunter"  ) a = new grimoire_of_service_t( this, name );
  else if ( name == "grimoire_of_service_imp"        ) a = new grimoire_of_service_t( this, name );
  else if ( name == "grimoire_of_service_succubus"   ) a = new grimoire_of_service_t( this, name );
  else if ( name == "grimoire_of_service_voidwalker" ) a = new grimoire_of_service_t( this, name );
  else return player_t::create_action( name, options_str );

  a -> parse_options( NULL, options_str );
  if ( a -> dtr_action ) a -> dtr_action -> parse_options( NULL, options_str );

  return a;
}

// warlock_t::create_pet ====================================================

pet_t* warlock_t::create_pet( const std::string& pet_name,
                              const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "felguard"     ) return new    felguard_pet_t( sim, this );
  if ( pet_name == "felhunter"    ) return new   felhunter_pet_t( sim, this );
  if ( pet_name == "imp"          ) return new         imp_pet_t( sim, this );
  if ( pet_name == "succubus"     ) return new    succubus_pet_t( sim, this );
  if ( pet_name == "voidwalker"   ) return new  voidwalker_pet_t( sim, this );
  if ( pet_name == "infernal"     ) return new    infernal_pet_t( sim, this );
  if ( pet_name == "doomguard"    ) return new   doomguard_pet_t( sim, this );

  if ( pet_name == "grimoire_of_service_felguard"     ) return new    felguard_pet_t( sim, this, pet_name );
  if ( pet_name == "grimoire_of_service_felhunter"    ) return new   felhunter_pet_t( sim, this, pet_name );
  if ( pet_name == "grimoire_of_service_imp"          ) return new         imp_pet_t( sim, this, pet_name );
  if ( pet_name == "grimoire_of_service_succubus"     ) return new    succubus_pet_t( sim, this, pet_name );
  if ( pet_name == "grimoire_of_service_voidwalker"   ) return new  voidwalker_pet_t( sim, this, pet_name );

  return 0;
}

// warlock_t::create_pets ===================================================

void warlock_t::create_pets()
{
  create_pet( "felguard"  );
  create_pet( "felhunter" );
  create_pet( "imp"       );
  create_pet( "succubus"  );
  create_pet( "voidwalker" );
  create_pet( "infernal"  );
  create_pet( "doomguard" );

  create_pet( "grimoire_of_service_felguard"  );
  create_pet( "grimoire_of_service_felhunter" );
  create_pet( "grimoire_of_service_imp"       );
  create_pet( "grimoire_of_service_succubus"  );
  create_pet( "grimoire_of_service_voidwalker" );
}

// warlock_t::init_talents ==================================================

// warlock_t::init_spells ===================================================

void warlock_t::init_spells()
{
  player_t::init_spells();

  // New set bonus system
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P    M2P    M4P    T2P    T4P    H2P    H4P
    { 105888, 105787,     0,     0,     0,     0,     0,     0 }, // Tier13
    {      0,      0,     0,     0,     0,     0,     0,     0 },
  };
  sets                        = new set_bonus_array_t( this, set_bonuses );

  // Spec spells =========================================================

  // General
  spec.nethermancy = find_specialization_spell( "Nethermancy" );

  spec.dark_soul = find_specialization_spell( "Dark Soul: Instability" );
  if ( ! spec.dark_soul -> ok() ) spec.dark_soul = find_specialization_spell( "Dark Soul: Knowledge" );
  if ( ! spec.dark_soul -> ok() ) spec.dark_soul = find_specialization_spell( "Dark Soul: Misery" );

  // Affliction
  spec.nightfall = find_specialization_spell( "Nightfall" );

  // Demonology
  spec.decimation    = find_specialization_spell( "Decimation" );
  spec.metamorphosis = find_specialization_spell( "Metamorphosis" );
  spec.molten_core   = find_specialization_spell( "Molten Core" );

  // Destruction
  spec.backdraft      = find_specialization_spell( "Backdraft" );
  spec.burning_embers = find_specialization_spell( "Burning Embers" );
  spec.chaotic_energy = find_specialization_spell( "Chaotic Energy" );

  // Mastery
  mastery_spells.emberstorm          = find_mastery_spell( "Emberstorm" );
  mastery_spells.potent_afflictions  = find_mastery_spell( "Potent Afflictions" );
  mastery_spells.master_demonologist = find_mastery_spell( "Master Demonologist" );

  // Talents
  talents.dark_regeneration     = find_talent_spell( "Dark Regeneration" );
  talents.soul_leech            = find_talent_spell( "Soul Leech" );
  talents.harvest_life          = find_talent_spell( "Harvest Life" );

  talents.howl_of_terror        = find_talent_spell( "Howl of Terror" );
  talents.mortal_coil           = find_talent_spell( "Mortal Coil" );
  talents.shadowfury            = find_talent_spell( "Shadowfury" );

  talents.soul_link             = find_talent_spell( "Soul Link" );
  talents.sacrificial_pact      = find_talent_spell( "Sacrificial Pact" );
  talents.dark_bargain          = find_talent_spell( "Dark Bargain" );

  talents.blood_fear            = find_talent_spell( "Blood Fear" );
  talents.burning_rush          = find_talent_spell( "Burning Rush" );
  talents.unbound_will          = find_talent_spell( "Unbound Will" );

  talents.grimoire_of_supremacy = find_talent_spell( "Grimoire of Supremacy" );
  talents.grimoire_of_service   = find_talent_spell( "Grimoire of Service" );
  talents.grimoire_of_sacrifice = find_talent_spell( "Grimoire of Sacrifice" );

  talents.archimondes_vengeance = find_talent_spell( "Archimonde's Vengeance" );
  talents.kiljaedens_cunning    = find_talent_spell( "Kil'jaeden's Cunning" );
  talents.mannoroths_fury       = find_talent_spell( "Mannoroth's Fury" );

  // Major
  glyphs.life_tap             = find_glyph_spell( "Glyph of Life Tap" );
  glyphs.demon_training       = find_glyph_spell( "Glyph of Demon Training" );
  glyphs.doom                 = find_glyph_spell( "Glyph of Doom" );
}

// warlock_t::init_base =====================================================

void warlock_t::init_base()
{
  player_t::init_base();

  stats_base.attack_power = -10;
  stats_initial.attack_power_per_strength = 2.0;
  stats_initial.spell_power_per_intellect = 1.0;

  if ( spec.chaotic_energy -> ok() ) stats_base.mp5 *= 1.0 + spec.chaotic_energy -> effectN( 1 ).percent();

  if ( primary_tree() == WARLOCK_AFFLICTION )  resources.base[ RESOURCE_SOUL_SHARD ]    = 3;
  if ( primary_tree() == WARLOCK_DEMONOLOGY )  resources.base[ RESOURCE_DEMONIC_FURY ]  = 1000;
  if ( primary_tree() == WARLOCK_DESTRUCTION ) resources.base[ RESOURCE_BURNING_EMBER ] = 30;

  diminished_kfactor    = 0.009830;
  diminished_dodge_capi = 0.006650;
  diminished_parry_capi = 0.006650;
}

// warlock_t::init_scaling ==================================================

void warlock_t::init_scaling()
{
  player_t::init_scaling();
  scales_with[ STAT_SPIRIT ] = 0;
  scales_with[ STAT_STAMINA ] = 0;
}

// warlock_t::init_buffs ====================================================

void warlock_t::init_buffs()
{
  player_t::init_buffs();

  buffs.backdraft             = buff_creator_t( this, "backdraft", find_spell( 117828 ) );
  buffs.dark_soul             = buff_creator_t( this, "dark_soul", spec.dark_soul );
  buffs.decimation            = buff_creator_t( this, "decimation", spec.decimation );
  buffs.metamorphosis         = buff_creator_t( this, "metamorphosis", spec.metamorphosis );
  buffs.molten_core           = buff_creator_t( this, "molten_core", spec.molten_core );
  buffs.soulburn              = buff_creator_t( this, "soulburn", find_class_spell( "Soulburn" ) );
  buffs.grimoire_of_sacrifice = buff_creator_t( this, "grimoire_of_sacrifice", talents.grimoire_of_sacrifice );
  buffs.tier13_4pc_caster     = buff_creator_t( this, "tier13_4pc_caster", find_spell( 105786 ) );
}

// warlock_t::init_values ======================================================

void warlock_t::init_values()
{
  player_t::init_values();

  if ( set_bonus.pvp_2pc_caster() )
    stats_initial.attribute[ ATTR_INTELLECT ] += 70;

  if ( set_bonus.pvp_4pc_caster() )
    stats_initial.attribute[ ATTR_INTELLECT ] += 90;
}

// warlock_t::init_gains ====================================================

void warlock_t::init_gains()
{
  player_t::init_gains();

  gains.life_tap               = get_gain( "life_tap"   );
  gains.soul_leech             = get_gain( "soul_leech" );
  gains.tier13_4pc             = get_gain( "tier13_4pc" );
  gains.nightfall              = get_gain( "nightfall"  );
  gains.drain_soul             = get_gain( "drain_soul" );
  gains.incinerate             = get_gain( "incinerate" );
}

// warlock_t::init_uptimes ==================================================

void warlock_t::init_benefits()
{
  player_t::init_benefits();

  for ( size_t i = 0; i < 4; ++i )
    benefits.backdraft[ i ] = get_benefit( "backdraft_" + util_t::to_string( i ) );
}

// warlock_t::init_procs ====================================================

void warlock_t::init_procs()
{
  player_t::init_procs();

  procs.shadow_trance    = get_proc( "shadow_trance" );
}

// warlock_t::init_rng ======================================================

void warlock_t::init_rng()
{
  player_t::init_rng();

  rngs.nightfall               = get_rng( "nightfall" );
}

// warlock_t::init_actions ==================================================

void warlock_t::init_actions()
{
  if ( action_list_str.empty() )
  {

    // Trinket check
    bool has_mwc = false;
    bool has_wou = false;
    for ( int i=0; i < SLOT_MAX; i++ )
    {
      item_t& item = items[ i ];
      if ( strstr( item.name(), "moonwell_chalice" ) )
      {
        has_mwc = true;
      }
      if ( strstr( item.name(), "will_of_unbinding" ) )
      {
        has_wou = true;
      }
    }

    // Flask
    if ( level >= 80 )
      action_list_str = "flask,type=draconic_mind";
    else if ( level >= 75 )
      action_list_str = "flask,type=frost_wyrm";

    // Food
    if ( level >= 80 ) action_list_str += "/food,type=seafood_magnifique_feast";
    else if ( level >= 70 ) action_list_str += "/food,type=fish_feast";

    // Armor
    if ( level >= 62 ) action_list_str += "/fel_armor";

    // Choose Pet
    if ( primary_tree() == WARLOCK_DESTRUCTION )
    {
      action_list_str += "/summon_imp";
    }
    else if ( primary_tree() == WARLOCK_DEMONOLOGY )
    {
      if ( has_mwc )
        action_list_str += "/summon_felguard,if=cooldown.demon_soul.remains<5&cooldown.metamorphosis.remains<5&!pet.felguard.active";
      else
        action_list_str += "/summon_felguard,if=!in_combat";
    }
    else if ( primary_tree() == WARLOCK_AFFLICTION )
    {
      action_list_str += "/summon_felhunter";
    }
    else
      action_list_str += "/summon_imp";

    // Dark Intent
    if ( level >= 83 )
      action_list_str += "/dark_intent,if=!aura.spell_power_multiplier.up";

    // Pre soulburn
    if ( use_pre_soulburn && !set_bonus.tier13_4pc_caster() )
      action_list_str += "/soulburn,if=!in_combat";

    // Snapshot Stats
    action_list_str += "/snapshot_stats";

    // Usable Item
    int num_items = ( int ) items.size();
    for ( int i = num_items - 1; i >= 0; i-- )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
        if ( primary_tree() == WARLOCK_DEMONOLOGY )
          action_list_str += ",if=cooldown.metamorphosis.remains=0|cooldown.metamorphosis.remains>cooldown";
      }
    }

    action_list_str += init_use_profession_actions();
    action_list_str += init_use_racial_actions();

    // Choose Potion
    if ( level >= 80 )
    {
      if ( primary_tree() == WARLOCK_DEMONOLOGY )
        action_list_str += "/volcanic_potion,if=buff.metamorphosis.up|!in_combat";
      else
        action_list_str += "/volcanic_potion,if=buff.bloodlust.react|!in_combat|target.health_pct<=20";
    }
    else if ( level >= 70 )
    {
      if ( primary_tree() == WARLOCK_AFFLICTION )
        action_list_str += "/speed_potion,if=buff.bloodlust.react|!in_combat";
      else
        action_list_str += "/wild_magic_potion,if=buff.bloodlust.react|!in_combat";
    }

    switch ( primary_tree() )
    {

    case WARLOCK_AFFLICTION:
      if ( level >= 85 ) action_list_str += "/demon_soul";
      if ( set_bonus.tier13_4pc_caster() ) action_list_str += "/soulburn";
      action_list_str += "/corruption,if=(!ticking|remains<tick_time)&miss_react";
      action_list_str += "/unstable_affliction,if=(!ticking|remains<(cast_time+tick_time))&target.time_to_die>=5&miss_react";
      if ( level >= 12 ) action_list_str += "/bane_of_doom,if=target.time_to_die>15&!ticking&miss_react";
      action_list_str += "/haunt";
      if ( level >= 50 ) action_list_str += "/summon_doomguard,if=time>10";
      action_list_str += "/drain_soul,interrupt=1,if=target.health_pct<=25";
      // If the profile has the Will of Unbinding, we need to make sure the stacks don't drop during execute phase
      if ( has_wou )
      {
        // Attempt to account for non-default channel_lag settings
        char delay = ( char ) ( sim -> channel_lag.total_seconds() * 20 + 48 );
        if ( delay > 57 ) delay = 57;
        action_list_str += ",interrupt_if=buff.will_of_unbinding.up&cooldown.haunt.remains<tick_time&buff.will_of_unbinding.remains<action.haunt.cast_time+tick_time+0.";
        action_list_str += delay;
      }
      if ( level >= 75 ) action_list_str += "/shadowflame";
      if ( set_bonus.tier13_4pc_caster() ) action_list_str += "/soul_fire,if=buff.soulburn.up";
      action_list_str += "/life_tap,mana_percentage<=35";
      if ( ! set_bonus.tier13_4pc_caster() )
      {
        action_list_str += "/soulburn,if=buff.demon_soul_felhunter.down";
        action_list_str += "/soul_fire,if=buff.soulburn.up";
      }
      action_list_str += "/shadow_bolt";
      break;

    case WARLOCK_DESTRUCTION:
      if ( level >= 85 ) action_list_str += "/demon_soul";
      if ( set_bonus.tier13_4pc_caster() )
      {
        action_list_str += "/soulburn";
        action_list_str += "/soul_fire,if=buff.soulburn.up&!in_combat";
      }
      else
      {
        action_list_str += "/soulburn,if=buff.bloodlust.down";
        if ( level >= 54 )
        {
          action_list_str += "/soul_fire,if=buff.soulburn.up&!in_combat";
        }
      }
      action_list_str += "/immolate,if=(remains<cast_time+gcd|!ticking)&target.time_to_die>=4&miss_react";
      action_list_str += "/conflagrate";
      action_list_str += "/immolate,if=buff.bloodlust.react&buff.bloodlust.remains>32&cooldown.conflagrate.remains<=3&remains<12";
      if ( level >= 20 ) action_list_str += "/bane_of_doom,if=!ticking&target.time_to_die>=15&miss_react";
      action_list_str += "/corruption,if=(!ticking|dot.corruption.remains<tick_time)&miss_react";
      if ( level >= 75 ) action_list_str += "/shadowflame";
      if ( level >= 50 ) action_list_str += "/summon_doomguard,if=time>10";
      if ( set_bonus.tier13_4pc_caster() ) action_list_str += "/soul_fire,if=buff.soulburn.up";
      action_list_str += "/shadowburn";
      if ( level >= 64 ) action_list_str += "/incinerate"; else action_list_str += "/shadow_bolt";

      break;

    case WARLOCK_DEMONOLOGY:
      action_list_str += "/metamorphosis";
      if ( has_mwc ) action_list_str += ",if=buff.moonwell_chalice.up&pet.felguard.active";
      if ( level >= 85 )
      {
        action_list_str += "/demon_soul";
        if ( has_mwc ) action_list_str += ",if=buff.metamorphosis.up";
      }
      if ( level >= 20 )
      {
        action_list_str += "/bane_of_doom,if=!ticking&time<10";
      }
      if ( level >= 50 ) action_list_str += "/summon_doomguard,if=time>10";
      action_list_str += "/felguard:felstorm";
      action_list_str += "/soulburn,if=pet.felguard.active&!pet.felguard.dot.felstorm.ticking";
      action_list_str += "/summon_felhunter,if=!pet.felguard.dot.felstorm.ticking&pet.felguard.active";
      if ( set_bonus.tier13_4pc_caster() )
      {
        action_list_str += "/soulburn,if=pet.felhunter.active";
        if ( has_mwc ) action_list_str += "&cooldown.metamorphosis.remains>60";
        action_list_str += "/soul_fire,if=pet.felhunter.active&buff.soulburn.up";
        if ( has_mwc ) action_list_str += "&cooldown.metamorphosis.remains>60";
      }
      action_list_str += "/immolate,if=!ticking&target.time_to_die>=4&miss_react";
      if ( level >= 20 )
      {
        action_list_str += "/bane_of_doom,if=(!ticking";
        action_list_str += "|(buff.metamorphosis.up&remains<45)";
        action_list_str += ")&target.time_to_die>=15&miss_react";
      }
      action_list_str += "/corruption,if=(remains<tick_time|!ticking)&target.time_to_die>=6&miss_react";
      if ( level >= 75 ) action_list_str += "/shadowflame";
      action_list_str += "/hand_of_guldan";
      if ( level >= 60 ) action_list_str += "/immolation_aura,if=buff.metamorphosis.remains>10";
      if ( level >= 64 ) action_list_str += "/incinerate,if=buff.molten_core.react";
      if ( level >= 54 ) action_list_str += "/soul_fire,if=buff.decimation.up";
      action_list_str += "/life_tap,if=mana_pct<=30&buff.bloodlust.down&buff.metamorphosis.down&buff.demon_soul_felguard.down";
      action_list_str += "/shadow_bolt";

      break;

    default:
      action_list_str += "/bane_of_doom,if=(remains<3|!ticking)&target.time_to_die>=15&miss_react";
      action_list_str += "/corruption,if=(!ticking|remains<tick_time)&miss_react";
      action_list_str += "/immolate,if=(!ticking|remains<(cast_time+tick_time))&miss_react";
      if ( level >= 50 ) action_list_str += "/summon_infernal";
      if ( level >= 64 ) action_list_str += "/incinerate"; else action_list_str += "/shadow_bolt";
      if ( sim->debug ) log_t::output( sim, "Using generic action string for %s.", name() );
      break;
    }

    // Movement
    action_list_str += "/life_tap,moving=1,if=mana_pct<80&mana_pct<target.health_pct";
    action_list_str += "/fel_flame,moving=1";

    action_list_str += "/life_tap,if=mana_pct_nonproc<100"; // to use when no mana or nothing else is possible

    action_list_default = 1;
  }

  player_t::init_actions();
}

void warlock_t::init_resources( bool force )
{
  player_t::init_resources( force );

  if ( pets.active )
    pets.active -> init_resources( force );
}

void warlock_t::combat_begin()
{
  player_t::combat_begin();

  resources.current[ RESOURCE_BURNING_EMBER ] = initial_burning_embers;
  resources.current[ RESOURCE_DEMONIC_FURY ] = initial_demonic_fury;
}

// warlock_t::reset =========================================================

void warlock_t::reset()
{
  player_t::reset();

  // Active
  pets.active = 0;
  ember_react = timespan_t::zero();
  event_t::cancel( meta_cost_event );
}

// warlock_t::create_options ================================================

void warlock_t::create_options()
{
  player_t::create_options();

  option_t warlock_options[] =
  {
    { "use_pre_soulburn",  OPT_BOOL,   &( use_pre_soulburn       ) },
    { "burning_embers",     OPT_INT,   &( initial_burning_embers ) },
    { "demonic_fury",       OPT_INT,   &( initial_demonic_fury   ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, warlock_options );
}

// warlock_t::create_profile ================================================

bool warlock_t::create_profile( std::string& profile_str, save_type_e stype, bool save_html ) const
{
  player_t::create_profile( profile_str, stype, save_html );

  if ( stype == SAVE_ALL )
  {
    if ( use_pre_soulburn )            profile_str += "use_pre_soulburn=1\n";
    if ( initial_burning_embers != 0 ) profile_str += "burning_embers=" + util_t::to_string( initial_burning_embers ) + "\n";
    if ( initial_demonic_fury != 200 ) profile_str += "burning_embers=" + util_t::to_string( initial_demonic_fury ) + "\n";
  }

  return true;
}

// warlock_t::copy_from =====================================================

void warlock_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warlock_t* p = debug_cast<warlock_t*>( source );

  use_pre_soulburn       = p -> use_pre_soulburn;
  initial_burning_embers = p -> initial_burning_embers;
  initial_demonic_fury   = p -> initial_demonic_fury;
}

// warlock_t::decode_set ====================================================

int warlock_t::decode_set( const item_t& item ) const
{
  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  const char* s = item.name();

  if ( strstr( s, "_of_the_faceless_shroud" ) ) return SET_T13_CASTER;

  if ( strstr( s, "_gladiators_felweave_"   ) ) return SET_PVP_CASTER;

  return SET_NONE;
}

expr_t* warlock_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "ember_react" )
  {
    struct ember_react_expr_t : public expr_t
    {
      warlock_t& player;
      ember_react_expr_t( warlock_t& p ) :
        expr_t( "ember_react" ), player( p ) { }
      virtual double evaluate() { return player.resources.current[ RESOURCE_BURNING_EMBER ] >= 10 && player.sim -> current_time >= player.ember_react; }
    };
    return new ember_react_expr_t( *this );
  }
  else
  {
    return player_t::create_expression( a, name_str );
  }
}

double warlock_t::resource_loss( resource_type_e resource_type, double amount, gain_t* gain, action_t* action )
{
  double r = player_t::resource_loss( resource_type, amount, gain, action );

  if ( resource_type == RESOURCE_DEMONIC_FURY && resources.current[ RESOURCE_DEMONIC_FURY ] <= 0 )
  {
    assert( buffs.metamorphosis -> up() );
    buffs.metamorphosis -> expire();
    event_t::cancel( meta_cost_event );
  }

  return r;
}

#endif // SC_WARLOCK

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_warlock =================================================

player_t* player_t::create_warlock( sim_t* sim, const std::string& name, race_type_e r )
{
  SC_CREATE_WARLOCK( sim, name, r );
}

// player_t::warlock_init ===================================================

void player_t::warlock_init( sim_t* )
{
}

// player_t::warlock_combat_begin ===========================================

void player_t::warlock_combat_begin( sim_t* )
{
}

