// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "simulationcraft.hpp"
#include "sc_warlock.hpp"

/*
 * To Do:
 * - Seed of Corruption with Soulburn: Trigger Corruptions
 */
// ==========================================================================

#if SC_WARLOCK == 1

warlock_td_t::warlock_td_t( player_t* target, warlock_t* p )
  : target_data_t( target, p ), ds_started_below_20( false ), shadowflame_stack( 0 ), soc_trigger( 0 )
{
  dots_corruption          = target -> get_dot( "corruption", p );
  dots_unstable_affliction = target -> get_dot( "unstable_affliction", p );
  dots_agony               = target -> get_dot( "agony", p );
  dots_doom                = target -> get_dot( "doom", p );
  dots_immolate            = target -> get_dot( "immolate", p );
  dots_drain_life          = target -> get_dot( "drain_life", p );
  dots_drain_soul          = target -> get_dot( "drain_soul", p );
  dots_shadowflame         = target -> get_dot( "shadowflame", p );
  dots_malefic_grasp       = target -> get_dot( "malefic_grasp", p );
  dots_seed_of_corruption  = target -> get_dot( "seed_of_corruption", p );
  dots_rain_of_fire        = target -> get_dot( "rain_of_fire", p );

  debuffs_haunt = buff_creator_t( *this, "haunt", source -> find_class_spell( "haunt" ) );
}

warlock_t::warlock_t( sim_t* sim, const std::string& name, race_type_e r ) :
  player_t( sim, WARLOCK, name, r ),
  pets( pets_t() ),
  buffs( buffs_t() ),
  cooldowns( cooldowns_t() ),
  talents( talents_t() ),
  mastery_spells( mastery_spells_t() ),
  gains( gains_t() ),
  procs( procs_t() ),
  rngs( rngs_t() ),
  glyphs( glyphs_t() ),
  meta_cost_event( 0 ),
  demonic_calling_event( 0 ),
  use_pre_soulburn( 0 ),
  initial_burning_embers( 0 ),
  initial_demonic_fury( 200 ),
  ember_react( timespan_t::max() )
{
  current.distance = 40;
  initial.distance = 40;

  cooldowns.infernal       = get_cooldown ( "summon_infernal" );
  cooldowns.doomguard      = get_cooldown ( "summon_doomguard" );
  cooldowns.imp_swarm      = get_cooldown ( "imp_swarm" );
  cooldowns.hand_of_guldan = get_cooldown ( "hand_of_guldan" );

  create_options();
}


namespace { // ANONYMOUS NAMESPACE ==========================================
  
struct warlock_heal_t : public heal_t
{
  warlock_heal_t( const std::string& n, warlock_t* p, const uint32_t id ) :
    heal_t( n, p, p -> find_spell( id ) )
  { }

  warlock_t* p() const
  { return static_cast<warlock_t*>( player ); }
};


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
    gain_fury = p() -> get_gain( name_str );
  }

public:
  double generate_fury;
  gain_t* gain_fury;

  warlock_spell_t( warlock_t* p, const std::string& n ) :
    spell_t( n, p, p -> find_class_spell( n ) ), generate_fury( 0 )
  {
    _init_warlock_spell_t();
  }

  warlock_spell_t( const std::string& token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( token, p, s ), generate_fury( 0 )
  {
    _init_warlock_spell_t();
  }

  warlock_t* p() const
  { return debug_cast<warlock_t*>( player ); }

  warlock_td_t* td( player_t* t ) const
  { return debug_cast<warlock_td_t*>( target_data( t ) ); }

  virtual void execute()
  {
    spell_t::execute();

    if ( result_is_hit( execute_state -> result ) && p() -> primary_tree() == WARLOCK_DEMONOLOGY
         && generate_fury > 0 && ! p() -> buffs.metamorphosis -> check() )
      p() -> resource_gain( RESOURCE_DEMONIC_FURY, generate_fury, gain_fury );
  }

  virtual bool ready()
  {
    if ( p() -> buffs.metamorphosis -> check() && p() -> resources.current[ RESOURCE_DEMONIC_FURY ] < 40 )
      p() -> cancel_metamorphosis();

    return spell_t::ready();
  }

  static void trigger_seed_of_corruption( warlock_td_t* td, warlock_t* p, double amount );

  virtual void tick( dot_t* d )
  {
    spell_t::tick( d );

    if ( p() -> primary_tree() == WARLOCK_DEMONOLOGY && generate_fury > 0 )
       p() -> resource_gain( RESOURCE_DEMONIC_FURY, generate_fury, gain_fury );

    trigger_seed_of_corruption( td( d -> state -> target ), p(), d -> state -> result_amount );
  }

  virtual void impact_s( action_state_t* s )
  {
    spell_t::impact_s( s );

    trigger_seed_of_corruption( td( s -> target ), p(), s -> result_amount );
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double m = 1.0;

    if ( td( t ) -> debuffs_haunt -> up() )
    {
      m *= 1.0 + td( t ) -> debuffs_haunt -> data().effectN( 3 ).percent();
    }

    return spell_t::composite_target_multiplier( t ) * m;
  }

  virtual resource_type_e current_resource() const
  {
    if ( p() -> buffs.metamorphosis -> check() && data().powerN( POWER_DEMONIC_FURY ).aura_id() == 54879 )
      return RESOURCE_DEMONIC_FURY;
    else
      return spell_t::current_resource();
  }

  virtual timespan_t tick_time( double haste ) const
  {
    timespan_t t = spell_t::tick_time( haste );

    if ( channeled ) return t;

    warlock_td_t* td = this->td( target );

    if ( td -> dots_malefic_grasp -> ticking ||
       ( td -> dots_drain_soul -> ticking && td -> ds_started_below_20 ) )
      t /= ( 1.0 + p() -> spec.malefic_grasp -> effectN( 2 ).percent() );

    return t;
  }

  virtual double action_da_multiplier() const
  {
    double m = spell_t::action_da_multiplier();

    if ( p() -> primary_tree() == WARLOCK_DEMONOLOGY && aoe == 0 )
      m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 6 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  static void trigger_ember_gain( warlock_t* p, double amount, gain_t* gain, double chance = 1.0 )
  {
    if ( ! p -> rngs.ember_gain -> roll( chance ) ) return;

    p -> resource_gain( RESOURCE_BURNING_EMBER, amount, gain );

    // If getting to 10 was a surprise, the player would have to react to it
    if ( p -> resources.current[ RESOURCE_BURNING_EMBER ] == 10 && amount > 1 && chance < 1.0 )
      p -> ember_react = p -> sim -> current_time + p -> total_reaction_time();
    else if ( p -> resources.current[ RESOURCE_BURNING_EMBER ] >= 10 )
      p -> ember_react = p -> sim -> current_time;
    else
      p -> ember_react = timespan_t::max();
  }

  static void refund_embers( warlock_t* p )
  {
    double refund = ceil( ( p -> resources.current[ RESOURCE_BURNING_EMBER ] + 10.0 ) / 4.0 );

    if ( refund > 0 ) p -> resource_gain( RESOURCE_BURNING_EMBER, refund, p -> gains.miss_refund );
  }

  static void trigger_soul_leech( warlock_t* p, double amount )
  {
    if ( p -> talents.soul_leech -> ok() )
    {
      p -> resource_gain( RESOURCE_HEALTH, amount, p -> gains.soul_leech );
    }
  }

  static void trigger_wild_imp( warlock_t* p )
  {
    for ( int i = 0; i < WILD_IMP_LIMIT; i++ )
    {
      if ( p -> pets.wild_imps[ i ] -> current.sleeping )
      {
        p -> pets.wild_imps[ i ] -> summon();
        p -> procs.wild_imp -> occur();
        return;
      }
    }
    assert( false ); // Will only get here if there are no available imps
  }

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


static void extend_dot( dot_t* dot, int ticks, double haste )
{
  if ( dot -> ticking )
  {
    //FIXME: This is roughly how it works, but we need more testing - seems inconsistent for immolate
    int max_ticks = ( int ) ( dot -> action -> hasted_num_ticks( haste ) * 1.667 ) + 1;
    int extend_ticks = std::min( ticks, max_ticks - dot -> ticks() );
    if ( extend_ticks > 0 ) dot -> extend_duration( extend_ticks );
  }
}


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

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( ! sim -> overrides.magic_vulnerability )
        target -> debuffs.magic_vulnerability -> trigger( 1, -1, -1, data().duration() );
    }
  }
};


struct agony_t : public warlock_spell_t
{
  int damage_level;

  agony_t( warlock_t* p ) :
    warlock_spell_t( p, "Agony" ), damage_level( 0 )
  {
    may_crit = false;
    tick_power_mod = 0.02; // from tooltip
    if ( p -> glyphs.everlasting_affliction -> ok() ) dot_behavior = DOT_EXTEND;
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

  virtual double action_multiplier() const
  {
    double m = warlock_spell_t::action_multiplier();
    
    m *= 1.0 + p() -> composite_mastery() * p() -> mastery_spells.potent_afflictions -> effectN( 1 ).mastery_value();

    return m;
  }
};


struct doom_t : public warlock_spell_t
{
  doom_t( warlock_t* p ) :
    warlock_spell_t( "doom", p, p -> spec.doom )
  {
    may_crit = false;
    tick_power_mod = 1.0; // from tooltip, also tested on beta 2012/04/28
    if ( p -> glyphs.everlasting_affliction -> ok() ) dot_behavior = DOT_EXTEND;
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    if ( d -> state -> result == RESULT_CRIT ) trigger_wild_imp( p() );
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};


struct bane_of_havoc_t : public warlock_spell_t
{
  bane_of_havoc_t( warlock_t* p ) : warlock_spell_t( p, "Bane of Havoc" ) { }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
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


struct shadow_bolt_t : public warlock_spell_t
{
  shadow_bolt_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Shadow Bolt" )
  {
    direct_power_mod = 1.5; // from tooltip
    generate_fury = data().effectN( 2 ).base_value();

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
      trigger_soul_leech( p(), s -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() );
    }
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( p() -> buffs.demonic_calling -> up() )
    {
      trigger_wild_imp( p() );
      p() -> buffs.demonic_calling -> expire();
    }
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};


struct shadowburn_t : public warlock_spell_t
{
  struct mana_event_t : public event_t
  {
    shadowburn_t* spell;
    gain_t* gain;

    mana_event_t( warlock_t* p, shadowburn_t* s ) :
      event_t( p -> sim, p, "shadowburn_mana_return" ), spell( s ), gain( p -> gains.shadowburn )
    {
      sim -> add_event( this, spell -> mana_delay );
    }

    virtual void execute()
    {
      player -> resource_gain( RESOURCE_MANA, player -> resources.max[ RESOURCE_MANA ] * spell -> mana_amount, gain );
    }
  };

  mana_event_t* mana_event;
  double mana_amount;
  timespan_t mana_delay;


  shadowburn_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Shadowburn" ), mana_event( 0 )
  {
    mana_delay  = p -> find_spell( 29314 ) -> duration();
    mana_amount = p -> find_spell( 125882 ) -> effectN( 1 ).percent();

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new shadowburn_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    mana_event = new ( sim ) mana_event_t( p(), this );
  }

  virtual double action_multiplier() const
  {
    double m = spell_t::action_multiplier();

    m *= 1.0 + data().effectN( 1 ).base_value() / 100.0 + p() -> composite_mastery() * p() -> mastery_spells.emberstorm -> effectN( 1 ).mastery_value();

    return m;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( ! result_is_hit( execute_state -> result ) ) refund_embers( p() );
  }

  virtual bool ready()
  {
    if ( target -> health_percentage() >= 20 )
      return false;

    return warlock_spell_t::ready();
  }
};


struct corruption_t : public warlock_spell_t
{
  bool soc_triggered;

  corruption_t( warlock_t* p, bool soc = false ) :
    warlock_spell_t( p, "Corruption" ), soc_triggered( soc )
  {
    may_crit = false;
    tick_power_mod = 0.3; // tested on beta 2012/04/28
    generate_fury = 6;
    if ( p -> glyphs.everlasting_affliction -> ok() ) dot_behavior = DOT_EXTEND;
  };

  virtual timespan_t travel_time() const
  {
    if ( soc_triggered ) return sim -> gauss( sim -> aura_delay, 0.25 * sim -> aura_delay );

    return warlock_spell_t::travel_time();
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    if ( p() -> spec.nightfall -> ok() && p() -> rngs.nightfall -> roll( 0.1 ) && p() -> verify_nightfall() )
    {
      p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.nightfall );
    }
  }

  virtual double action_multiplier() const
  {
    double m = warlock_spell_t::action_multiplier();
    
    m *= 1.0 + p() -> composite_mastery() * p() -> mastery_spells.potent_afflictions -> effectN( 1 ).mastery_value();

    return m;
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};


struct drain_life_heal_t : public warlock_heal_t
{
  const spell_data_t* real_data;
  const spell_data_t* soulburned_data;

  drain_life_heal_t( warlock_t* p ) :
    warlock_heal_t( "drain_life_heal", p, 89653 )
  {
    background = true;
    may_miss = false;
    base_dd_min = base_dd_max = 0; // Is parsed as 2
    real_data       = p -> find_spell( 689 );
    soulburned_data = p -> find_spell( 89420 );
  }

  virtual void execute()
  {
    double heal_pct = real_data -> effectN( 2 ).percent();

    if ( p() -> buffs.soulburn -> up() )
      heal_pct = soulburned_data -> effectN( 2 ).percent();

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
    generate_fury = 10;

    heal = new drain_life_heal_t( p );
  }

  virtual void last_tick( dot_t* d )
  {
    warlock_spell_t::last_tick( d );

    if ( p() -> buffs.soulburn -> check() )
      p() -> buffs.soulburn -> expire();
  }

  virtual void tick( dot_t* d )
  {
    // costs mana per tick
    consume_resource();
    if ( p() -> resources.current[ RESOURCE_MANA ] < base_costs[ RESOURCE_MANA ] )
      d -> current_tick = d -> num_ticks;

    warlock_spell_t::tick( d );

    heal -> execute();
  }
};


struct drain_soul_t : public warlock_spell_t
{
  bool generate_shard;

  drain_soul_t( warlock_t* p ) :
    warlock_spell_t( p, "Drain Soul" ), generate_shard( false )
  {
    channeled    = true;
    hasted_ticks = true; // informative
    may_crit     = false;
    tick_power_mod = 1.5; // tested in beta 2012/05/13
  }

  virtual double composite_target_multiplier( player_t* target ) const
  {
    double m = warlock_spell_t::composite_target_multiplier( target );

    if ( target -> health_percentage() <= data().effectN( 3 ).base_value() )
    {
      m *= 2.0;
    }

    return m;
  }

  virtual void tick( dot_t* d )
  {
    // costs mana per tick
    consume_resource();
    if ( p() -> resources.current[ RESOURCE_MANA ] < base_costs[ RESOURCE_MANA ] )
      d -> current_tick = d -> num_ticks;

    warlock_spell_t::tick( d );

    if ( generate_shard ) p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.drain_soul );
    generate_shard = ! generate_shard;
  }


  virtual void last_tick( dot_t* d )
  {
    if ( td( d -> state -> target ) -> ds_started_below_20 )
    {
      stop_malefic_grasp( this, td( d -> state -> target ) -> dots_agony );
      stop_malefic_grasp( this, td( d -> state -> target ) -> dots_corruption );
      stop_malefic_grasp( this, td( d -> state -> target ) -> dots_doom );
      stop_malefic_grasp( this, td( d -> state -> target ) -> dots_unstable_affliction );
    }

    warlock_spell_t::last_tick( d );
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      generate_shard = false;

      if ( s -> target -> health_percentage() <= data().effectN( 3 ).base_value() )
      {
        td( s -> target ) -> ds_started_below_20 = true;
        start_malefic_grasp( this, td( s -> target ) -> dots_agony );
        start_malefic_grasp( this, td( s -> target ) -> dots_corruption );
        start_malefic_grasp( this, td( s -> target ) -> dots_doom );
        start_malefic_grasp( this, td( s -> target ) -> dots_unstable_affliction );
      }
      else
      {
        td( s -> target ) -> ds_started_below_20 = false;
      }
    }
  }
};


struct unstable_affliction_t : public warlock_spell_t
{
  unstable_affliction_t( warlock_t* p ) :
    warlock_spell_t( p, "Unstable Affliction" )
  {
    may_crit   = false;
    tick_power_mod = 0.2; // from tooltip, also tested on beta 2012/04/28
    if ( p -> glyphs.everlasting_affliction -> ok() ) dot_behavior = DOT_EXTEND;
  }

  virtual double action_multiplier() const
  {
    double m = warlock_spell_t::action_multiplier();
    
    m *= 1.0 + p() -> composite_mastery() * p() -> mastery_spells.potent_afflictions -> effectN( 1 ).mastery_value();

    return m;
  }
};


struct haunt_t : public warlock_spell_t
{
  haunt_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Haunt" )
  {
    // overriding because effect #4 makes the dbc parsing code think this is a dot
    num_ticks = 0;
    base_tick_time = timespan_t::zero();

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
      td( s -> target ) -> debuffs_haunt -> trigger();
    }
  }
};


struct immolate_t : public warlock_spell_t
{
  immolate_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Immolate" )
  {
    base_costs[ RESOURCE_MANA ] *= 4; // Mysterious 4x mana increase on certain spells for destro

    tick_power_mod = direct_power_mod; // No tick power mod in dbc for some reason
    if ( p -> glyphs.everlasting_affliction -> ok() ) dot_behavior = DOT_EXTEND;

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new immolate_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    if ( ! is_dtr_action && p() -> buffs.fire_and_brimstone -> up() )
    {
      p() -> buffs.fire_and_brimstone -> expire();
      aoe = -1;
    }

    warlock_spell_t::execute();

    aoe = 0;
  }

  virtual double action_da_multiplier() const
  {
    double m = warlock_spell_t::action_da_multiplier();
    
    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 7 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual double action_multiplier() const
  {
    double m = warlock_spell_t::action_multiplier();

    if ( aoe == -1 ) m *= ( 1.0 + p() -> composite_mastery() * p() -> mastery_spells.emberstorm -> effectN( 1 ).mastery_value() ) * 0.4;

    return m;
  }
};


struct conflagrate_t : public warlock_spell_t
{
  conflagrate_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Conflagrate" )
  {
    base_costs[ RESOURCE_MANA ] *= 4; // Mysterious 4x mana increase on certain spells for destro

    if ( p -> glyphs.conflagrate -> ok() )
      cooldown -> charges = 2;

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new conflagrate_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    if ( ! is_dtr_action && p() -> buffs.fire_and_brimstone -> up() )
    {
      p() -> buffs.fire_and_brimstone -> expire();
      aoe = -1;
    }

    warlock_spell_t::execute();

    aoe = 0;
  }

  virtual double action_multiplier() const
  {
    double m = warlock_spell_t::action_multiplier();

    if ( aoe == -1 ) m *= ( 1.0 + p() -> composite_mastery() * p() -> mastery_spells.emberstorm -> effectN( 1 ).mastery_value() ) * 0.4;

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 7 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) && p() -> spec.backdraft -> ok() )
      p() -> buffs.backdraft -> trigger( 3 );
  }
};


struct incinerate_t : public warlock_spell_t
{
  incinerate_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Incinerate" )
  {
    base_costs[ RESOURCE_MANA ] *= 4; // Mysterious 4x mana increase on certain spells for destro

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new incinerate_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual double action_multiplier() const
  {
    double m = warlock_spell_t::action_multiplier();

    if ( aoe == -1 ) m *= ( 1.0 + p() -> composite_mastery() * p() -> mastery_spells.emberstorm -> effectN( 1 ).mastery_value() ) * 0.4;

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 7 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual void execute()
  {
    if ( ! is_dtr_action && p() -> buffs.fire_and_brimstone -> up() )
    {
      p() -> buffs.fire_and_brimstone -> expire();
      aoe = -1;
    }

    warlock_spell_t::execute();

    aoe = 0;

    if ( p() -> buffs.backdraft -> check() && ! is_dtr_action )
    {
      p() -> buffs.backdraft -> decrement();
    }
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );
    
    trigger_ember_gain( p(), s -> result == RESULT_CRIT ? 2 : 1, p() -> gains.incinerate  );

    if ( result_is_hit( s -> result ) )
    {
      trigger_soul_leech( p(), s -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() );
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

  virtual double cost() const
  {
    double c = warlock_spell_t::cost();

    if ( p() -> buffs.backdraft -> check() )
    {
      c *= 1.0 + p() -> buffs.backdraft -> data().effectN( 1 ).percent();
    }

    return c;
  }
};



struct soul_fire_t : public warlock_spell_t
{
  soul_fire_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Soul Fire" )
  {
    base_costs[ RESOURCE_DEMONIC_FURY ] = 100;
    generate_fury = data().effectN( 2 ).base_value();

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new soul_fire_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( p() -> buffs.molten_core -> check() )
      p() -> buffs.molten_core -> decrement();

    if ( result_is_hit( execute_state -> result ) && target -> health_percentage() < p() -> spec.decimation -> effectN( 1 ).base_value() )
      p() -> buffs.molten_core -> trigger();
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
      trigger_soul_leech( p(), s -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() );
  }

  virtual resource_type_e current_resource() const
  {
    if ( p() -> buffs.metamorphosis -> check() )
      return RESOURCE_DEMONIC_FURY;
    else
      return spell_t::current_resource();
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = warlock_spell_t::execute_time();

    if ( p() -> buffs.molten_core -> up() )
      t *= 1.0 + p() -> buffs.molten_core -> data().effectN( 1 ).percent();

    return t;
  }

  virtual result_type_e calculate_result( double crit, unsigned int level )
  {
    result_type_e r = warlock_spell_t::calculate_result( crit, level );

    // Soul fire always crits
    if ( result_is_hit( r ) ) return RESULT_CRIT;

    return r;
  }

  virtual double action_multiplier() const
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> composite_spell_crit();

    return m;
  }

  virtual double cost() const
  {
    double c = warlock_spell_t::cost();

    if ( p() -> buffs.molten_core -> check() )
    {
      c *= 1.0 + p() -> buffs.molten_core -> data().effectN( 1 ).percent();
    }

    return c;
  }
};


struct chaos_bolt_t : public warlock_spell_t
{
  chaos_bolt_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Chaos Bolt" )
  {
    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new chaos_bolt_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual result_type_e calculate_result( double crit, unsigned int level )
  {
    result_type_e r = warlock_spell_t::calculate_result( crit, level );

    // Chaos Bolt always crits
    if ( result_is_hit( r ) ) return RESULT_CRIT;

    return r;
  }

  virtual double action_multiplier() const
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> composite_mastery() * p() -> mastery_spells.emberstorm -> effectN( 1 ).mastery_value();

    m *= 1.0 + p() -> composite_spell_crit();

    return m;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( ! result_is_hit( execute_state -> result ) ) refund_embers( p() );

    if ( p() -> buffs.backdraft -> check() >= 3 )
    {
      p() -> buffs.backdraft -> decrement( 3 );
    }
  }

  virtual timespan_t execute_time() const
  {
    timespan_t h = warlock_spell_t::execute_time();

    if ( p() -> buffs.backdraft -> stack() >= 3 )
    {
      h *= 1.0 + p() -> buffs.backdraft -> data().effectN( 1 ).percent();
    }

    return h;
  }

  virtual double cost() const
  {
    double c = warlock_spell_t::cost();

    // BUG: DTR-copied chaos bolts currently (beta 2012-05-04) cost embers
    if ( is_dtr_action && p() -> bugs )
      c = base_costs[ RESOURCE_BURNING_EMBER ];

    return c;
  }
};


struct life_tap_t : public warlock_spell_t
{
  life_tap_t( warlock_t* p ) :
    warlock_spell_t( p, "Life Tap" )
  {
    harmful = false;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    // FIXME: Implement reduced healing debuff
    if ( ! p() -> glyphs.life_tap -> ok() ) player -> resource_loss( RESOURCE_HEALTH, player -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 3 ).percent() );
    player -> resource_gain( RESOURCE_MANA, player -> resources.max[ RESOURCE_MANA ] * data().effectN( 1 ).percent(), p() -> gains.life_tap );
  }
};


struct touch_of_chaos_t : public attack_t
{
  touch_of_chaos_t( warlock_t* p ) :
    attack_t( "touch_of_chaos", p, p -> find_spell( 103988 ) )
  {
    stateless         = true;
    may_crit          = true;
    background        = true;
    repeating         = true;
    weapon_multiplier            = 0.0;
    base_spell_power_multiplier  = 1.0;
    base_attack_power_multiplier = 0.0;
    base_execute_time = timespan_t::from_seconds( 1 );
  }

  virtual void execute()
  {
    attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      warlock_td_t* td = debug_cast<warlock_td_t*>( target_data( target ) );
      extend_dot( td -> dots_corruption, 2, player -> composite_spell_haste() );
    }
  }

  virtual timespan_t execute_time() const
  {
    return timespan_t::from_seconds( 1 );
  }
};


struct metamorphosis_t : public warlock_spell_t
{
  metamorphosis_t( warlock_t* p ) :
    warlock_spell_t( p, "Metamorphosis" )
  {
    trigger_gcd = timespan_t::zero();
    harmful = false;
    if ( ! p -> main_hand_attack ) p -> main_hand_attack = new touch_of_chaos_t( p );
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    p() -> trigger_metamorphosis();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.metamorphosis -> check() ) return false;
    if ( p() -> resources.current[ RESOURCE_DEMONIC_FURY ] <= 40 ) return false;

    return warlock_spell_t::ready();
  }
};


struct cancel_metamorphosis_t : public warlock_spell_t
{
  cancel_metamorphosis_t( warlock_t* p ) :
    warlock_spell_t( "cancel_metamorphosis", p )
  {
    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    p() -> cancel_metamorphosis();
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};

struct shadowflame_state_t : public action_state_t
{
  shadowflame_state_t( action_t* spell, player_t* target ) :
    action_state_t( spell, target )
  { }

  virtual double composite_power() const
  {
    warlock_td_t* td = debug_cast<warlock_td_t*>( action -> target_data( target ) );
    assert( td -> shadowflame_stack >= 1 );

    return action_state_t::composite_power() * td -> shadowflame_stack;
  }
};

struct shadowflame_t : public warlock_spell_t
{
  action_state_t* new_state() { return new shadowflame_state_t( this, target ); }

  shadowflame_t( warlock_t* p ) :
    warlock_spell_t( "shadowflame", p, p -> find_spell( 47960 ) )
  {
    aoe        = -1;
    proc       = true;
    background = true;
    generate_fury = 2;
  }

  virtual timespan_t travel_time() const
  {
    // FIXME: Needs testing
    return timespan_t::from_seconds( 1.5 );
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    if ( p() -> spec.molten_core -> ok() && p() -> rngs.molten_core -> roll( 0.08 ) )
      p() -> buffs.molten_core -> trigger();
  }

  virtual void impact_s( action_state_t* s )
  {
    if ( result_is_hit( s -> result ) )
    {
      if ( td( s -> target ) -> dots_shadowflame -> ticking ) 
        td( s -> target ) -> shadowflame_stack++;
      else
        td( s -> target ) -> shadowflame_stack = 1;
    }

    warlock_spell_t::impact_s( s );
  }
};


struct hand_of_guldan_dmg_t : public warlock_spell_t
{
  hand_of_guldan_dmg_t( warlock_t* p ) :
    warlock_spell_t( "hand_of_guldan_dmg", p, p -> find_spell( 86040 ) )
  {
    proc       = true;
    background = true;
    dual       = true;
  }

  virtual timespan_t travel_time() const
  {
    // FIXME: Needs testing
    return timespan_t::from_seconds( 1.5 );
  }
};


struct hand_of_guldan_t : public warlock_spell_t
{
  shadowflame_t* shadowflame;
  hand_of_guldan_dmg_t* hog_damage;

  hand_of_guldan_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Hand of Gul'dan" )
  {
    cooldown -> duration = timespan_t::from_seconds( 15 );
    cooldown -> charges = 2;

    shadowflame = new shadowflame_t( p );
    hog_damage  = new hand_of_guldan_dmg_t( p );

    add_child( shadowflame );
    add_child( hog_damage );

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new hand_of_guldan_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void init()
  {
    warlock_spell_t::init();
    
    hog_damage  -> stats = stats;
  }

  virtual timespan_t travel_time() const
  {
    return timespan_t::zero();
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      shadowflame -> execute();
      hog_damage  -> execute();
    }
  }
};


struct chaos_wave_dmg_t : public warlock_spell_t
{
  chaos_wave_dmg_t( warlock_t* p ) :
    warlock_spell_t( "chaos_wave_dmg", p, p -> find_spell( 124915 ) )
  {
    proc       = true;
    background = true;
    aoe        = -1;
    dual       = true;
  }

  virtual timespan_t travel_time() const
  {
    // FIXME: Needs testing.
    return timespan_t::from_seconds( 1.5 );
  }
};


struct chaos_wave_t : public warlock_spell_t
{
  chaos_wave_dmg_t* cw_damage;

  chaos_wave_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( "chaos_wave", p, p -> spec.chaos_wave )
  {
    cooldown = p -> cooldowns.hand_of_guldan;

    cw_damage  = new chaos_wave_dmg_t( p );

    add_child( cw_damage );

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new chaos_wave_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void init()
  {
    warlock_spell_t::init();
    
    cw_damage  -> stats = stats;
  }

  virtual timespan_t travel_time() const
  {
    return timespan_t::zero();
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      cw_damage -> execute();
    }
  }
};


struct demonic_slash_t : public warlock_spell_t
{
  demonic_slash_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( "demonic_slash", p, p -> spec.demonic_slash )
  {
    direct_power_mod = 0.8; // from tooltip

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new demonic_slash_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
      trigger_soul_leech( p(), s -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() );
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( p() -> buffs.demonic_calling -> up() )
    {
      trigger_wild_imp( p() );
      p() -> buffs.demonic_calling -> expire();
    }
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};


struct fel_flame_t : public warlock_spell_t
{
  fel_flame_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Fel Flame" )
  {
    if ( p -> primary_tree() == WARLOCK_DESTRUCTION )
      base_costs[ RESOURCE_MANA ] *= 4; // Mysterious 4x mana increase on certain spells for destro

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new fel_flame_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( p() -> primary_tree() == WARLOCK_DESTRUCTION ) trigger_ember_gain( p(), 1, p() -> gains.fel_flame );

    if ( result_is_hit( s -> result ) )
    {
      extend_dot(            td( s -> target ) -> dots_immolate, 2, player -> composite_spell_haste() );
      extend_dot( td( s -> target ) -> dots_unstable_affliction, 2, player -> composite_spell_haste() );
      extend_dot(          td( s -> target ) -> dots_corruption, 2, player -> composite_spell_haste() );
    }
  }

  virtual double action_multiplier() const
  {
    double m = warlock_spell_t::action_multiplier();

    // Exclude demonology because it's already covered by warlock_spell_t::action_dd_multiplier()

    if ( p() -> primary_tree() == WARLOCK_AFFLICTION )
      m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 5 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();
    
    if ( p() -> primary_tree() == WARLOCK_DESTRUCTION )
      m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 7 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};


struct void_ray_t : public warlock_spell_t
{
  void_ray_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Void Ray" )
  {
    aoe = -1;
    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new void_ray_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      extend_dot( td( s -> target ) -> dots_corruption, 2, player -> composite_spell_haste() );
    }
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};


struct malefic_grasp_t : public warlock_spell_t
{
  malefic_grasp_t( warlock_t* p ) :
    warlock_spell_t( p, "Malefic Grasp" )
  {
    channeled    = true;
    hasted_ticks = true;
    may_crit     = false;
  }

  virtual double action_multiplier() const
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 5 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual void last_tick( dot_t* d )
  {    
    stop_malefic_grasp( this, td( d -> state -> target ) -> dots_agony );
    stop_malefic_grasp( this, td( d -> state -> target ) -> dots_corruption );
    stop_malefic_grasp( this, td( d -> state -> target ) -> dots_doom );
    stop_malefic_grasp( this, td( d -> state -> target ) -> dots_unstable_affliction );

    warlock_spell_t::last_tick( d );
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      start_malefic_grasp( this, td( s -> target ) -> dots_agony );
      start_malefic_grasp( this, td( s -> target ) -> dots_corruption );
      start_malefic_grasp( this, td( s -> target ) -> dots_doom );
      start_malefic_grasp( this, td( s -> target ) -> dots_unstable_affliction );
    }
  }

  virtual void tick( dot_t* d )
  {
    // costs mana per tick
    consume_resource();
    if ( p() -> resources.current[ RESOURCE_MANA ] < base_costs[ RESOURCE_MANA ] )
      d -> current_tick = d -> num_ticks;

    warlock_spell_t::tick( d );

    trigger_soul_leech( p(), d -> state -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() * 2 );
  }
};


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


struct soulburn_t : public warlock_spell_t
{
  soulburn_t( warlock_t* p ) :
    warlock_spell_t( p, "Soulburn" )
  {
    harmful = false;
  }

  virtual void execute()
  {
    p() -> buffs.soulburn -> trigger();
    p() -> buffs.tier13_4pc_caster -> trigger();

    warlock_spell_t::execute();
  }
};


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


struct imp_swarm_t : public warlock_spell_t
{
  timespan_t base_cooldown;

  imp_swarm_t( warlock_t* p ) :
    warlock_spell_t( "imp_swarm", p, ( p -> primary_tree() == WARLOCK_DEMONOLOGY && p -> glyphs.imp_swarm -> ok() ) ? p -> find_spell( 104316 ) : spell_data_t::not_found() )
  {
    harmful = false;

    base_cooldown = cooldown -> duration;
  }

  virtual void execute()
  {
    cooldown -> duration = base_cooldown * haste();

    warlock_spell_t::execute();

    p() -> buffs.demonic_calling -> expire();

    int j = 0;

    for ( int i = 0; i < WILD_IMP_LIMIT; i++ )
    {
      if ( p() -> pets.wild_imps[ i ] -> current.sleeping )
      {
        p() -> pets.wild_imps[ i ] -> summon();
        if ( ++j == 5 ) break;
      }
    }

    assert( j == 5 );  // Assert fails if we didn't have enough available wild imps
  }
};


struct fire_and_brimstone_t : public warlock_spell_t
{
  fire_and_brimstone_t( warlock_t* p ) :
    warlock_spell_t( p, "Fire and Brimstone" )
  {
    harmful = false;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    p() -> buffs.fire_and_brimstone -> trigger();
  }
};


// AOE SPELLS

struct seed_of_corruption_aoe_t : public warlock_spell_t
{
  seed_of_corruption_aoe_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( "seed_of_corruption_aoe", p, p -> find_spell( 27285 ) )
  {
    dual       = true;
    background = true;
    aoe        = -1;
    may_miss   = false; // FIXME: Assumed, needs testing

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new seed_of_corruption_aoe_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }
};


struct soulburn_seed_of_corruption_aoe_t : public warlock_spell_t
{
  corruption_t* corruption;

  soulburn_seed_of_corruption_aoe_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( "seed_of_corruption_aoe", p, p -> find_spell( 27285 ) ), corruption( new corruption_t( p, true ) )
  {
    dual       = true;
    background = true;
    aoe        = -1;
    may_miss   = false; // FIXME: Assumed, needs testing
    corruption -> background = true;
    corruption -> dual = true;
    corruption -> proc = true;
    corruption -> may_miss = false; // FIXME: Assumed, needs testing

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new soulburn_seed_of_corruption_aoe_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.seed_of_corruption );
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    corruption -> target = s -> target;
    corruption -> execute();
  }
};


struct soc_state_t : public action_state_t
{
  bool soulburned;

  soc_state_t( action_t* spell, player_t* target ) :
    action_state_t( spell, target ), soulburned( false )
  {
  }

  virtual void copy_state( const action_state_t* s )
  {
    action_state_t::copy_state( s );
    soulburned = ( s != 0 ) ? debug_cast< const soc_state_t* >( s ) -> soulburned : false;
  }
};


void warlock_spell_t::trigger_seed_of_corruption( warlock_td_t* td, warlock_t* p, double amount )
{
  if ( td -> dots_seed_of_corruption -> ticking && td -> soc_trigger > 0 )
  {
    td -> soc_trigger -= amount;
    if ( td -> soc_trigger <= 0 )
    {
      if ( debug_cast< soc_state_t* >( td -> dots_seed_of_corruption -> state ) -> soulburned )
      {
        p -> soulburn_seed_of_corruption_aoe -> execute();
      }
      else
      {
        p -> seed_of_corruption_aoe -> execute();
      }
      td -> dots_seed_of_corruption -> cancel();
    }
  }
}


struct seed_of_corruption_t : public warlock_spell_t
{
  action_state_t* new_state() { return new soc_state_t( this, target ); }

  cooldown_t* soulburn_cooldown;

  seed_of_corruption_t( warlock_t* p ) :
    warlock_spell_t( p, "Seed of Corruption" ), soulburn_cooldown( p -> get_cooldown( "soulburn_seed_of_corruption" ) )
  {
    may_crit = false;
    tick_power_mod = 0.3;
    soulburn_cooldown -> duration = timespan_t::from_seconds( 30 ); // FIXME: Needs testing

    p -> seed_of_corruption_aoe = new seed_of_corruption_aoe_t( p );
    add_child( p -> seed_of_corruption_aoe );
    p -> soulburn_seed_of_corruption_aoe = new soulburn_seed_of_corruption_aoe_t( p );
    add_child( p -> soulburn_seed_of_corruption_aoe );
  }

  virtual void init()
  {
    warlock_spell_t::init();

    p() -> seed_of_corruption_aoe -> stats = stats;
    p() -> soulburn_seed_of_corruption_aoe -> stats = stats;
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
      td( s -> target ) -> soc_trigger = data().effectN( 3 ).base_value() + s -> composite_power() * data().effectN( 3 ).coeff(); 
  }

  virtual void snapshot_state( action_state_t* state, uint32_t flags )
  {
    warlock_spell_t::snapshot_state( state, flags );
    debug_cast< soc_state_t* >( state ) -> soulburned = ( p() -> buffs.soulburn -> up() ) ? true : false;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( p() -> buffs.soulburn -> check() )
    {
      p() -> buffs.soulburn -> expire();
      soulburn_cooldown -> start();
    }
  }

  virtual bool ready()
  {
    if ( p() -> buffs.soulburn -> check() )
      if ( soulburn_cooldown -> remains() > timespan_t::zero() ) return false;

    return warlock_spell_t::ready();
  }
};


struct rain_of_fire_tick_t : public warlock_spell_t
{
  warlock_spell_t* rof_spell;

  rain_of_fire_tick_t( warlock_t* p, warlock_spell_t* s, bool dtr = false ) :
    warlock_spell_t( "rain_of_fire_tick", p, p -> find_spell( 42223 ) ), rof_spell( s )
  {
    background  = true;
    aoe         = -1;
    
    if ( ! dtr )
    {
      dual        = true;
      direct_tick = true;
    }

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new rain_of_fire_tick_t( p, s, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual double cost() const
  {
    if ( ! rof_spell -> channeled ) return 0;

    return warlock_spell_t::cost();
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double m = warlock_spell_t::composite_target_multiplier( t );

    if ( td( t ) -> dots_immolate -> ticking )
      m *= 1.5;

    return m;
  }

  virtual void impact_s( action_state_t* s )
  {
    warlock_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) && td( s -> target ) -> dots_immolate -> ticking ) trigger_ember_gain( p(), 1, p() -> gains.rain_of_fire, 0.50 );
  }
};


struct rain_of_fire_t : public warlock_spell_t
{
  rain_of_fire_tick_t* rain_of_fire_tick;

  rain_of_fire_t( warlock_t* p ) :
    warlock_spell_t( p, "Rain of Fire" ),
    rain_of_fire_tick( 0 )
  {
    dot_behavior = DOT_CLIP;
    harmful = false;
    tick_zero = true;
    hasted_ticks = false;
    channeled = ( p -> find_specialization_spell( "Aftermath" ) -> ok() ) ? false : true;

    // FIXME: Seems to cost 34k mana at 85 for destro - no idea how/why
    if ( ! channeled ) base_costs[ RESOURCE_MANA ] *= 8.5;

    rain_of_fire_tick = new rain_of_fire_tick_t( p, this );

    add_child( rain_of_fire_tick );
  }

  virtual timespan_t travel_time() const
  {
    // FIXME: Estimate, needs testing
    return ( channeled ) ? timespan_t::zero() : timespan_t::from_seconds( 1.5 );
  }

  virtual void init()
  {
    warlock_spell_t::init();

    rain_of_fire_tick -> stats = stats;
    // RoF currently costs mana per tick if it's being channeled
    if ( channeled ) rain_of_fire_tick -> base_costs[ RESOURCE_MANA ] = base_costs[ RESOURCE_MANA ];
  }

  virtual void tick( dot_t* d )
  {
    rain_of_fire_tick -> execute();

    stats -> add_tick( d -> time_to_tick );
  }

  virtual void execute()
  {
    if ( p() -> primary_tree() == WARLOCK_DEMONOLOGY ) channeled = ( p() -> buffs.metamorphosis -> check() ) ? false : true;

    warlock_spell_t::execute();
  }

  virtual bool usable_moving()
  {
    if ( p() -> primary_tree() == WARLOCK_DEMONOLOGY && ! p() -> buffs.metamorphosis -> check() ) return false;

    return warlock_spell_t::usable_moving();
  }
};


struct carrion_swarm_t : public warlock_spell_t
{
  carrion_swarm_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Carrion Swarm" )
  {
    aoe = -1;
    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new carrion_swarm_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};


// PET SPELLS

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

    p() -> pets.active = pet;

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

    summon_doomguard2 -> execute();
  }
};


// TALENT SPELLS

struct shadowfury_t : public warlock_spell_t
{
  shadowfury_t( warlock_t* p ) :
    warlock_spell_t( "shadowfury", p, p -> talents.shadowfury )
  {  }
};


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


struct grimoire_of_service_t : public summon_pet_t
{
  grimoire_of_service_t( warlock_t* p, const std::string& pet_name ) :
    summon_pet_t( pet_name, p, p -> talents.grimoire_of_service )
  {
    summoning_duration = timespan_t::from_seconds( 30 );
  }
};

} // ANONYMOUS NAMESPACE ====================================================


double warlock_t::composite_spell_power_multiplier() const
{
  double m = player_t::composite_spell_power_multiplier();

  if ( buffs.tier13_4pc_caster -> up() )
  {
    m *= 1.0 + sets -> set ( SET_T13_4PC_CASTER ) -> effect1().percent();
  }

  return m;
}


double warlock_t::composite_player_multiplier( school_type_e school, const action_t* a ) const
{
  double m = player_t::composite_player_multiplier( school, a );

  double mastery_value = composite_mastery() * mastery_spells.master_demonologist -> effectN( 1 ).mastery_value();

  if ( buffs.metamorphosis -> up() )
  {
    m *= 1.0 + spec.demonic_fury -> effectN( 1 ).percent() * 3 + mastery_value * 3;
  }
  else
  {
    m *= 1.0 + mastery_value;
  }

  return m;
}


double warlock_t::composite_spell_crit() const
{
  double sc = player_t::composite_spell_crit();

  if ( primary_tree() == WARLOCK_DESTRUCTION )
  {
    if ( buffs.dark_soul -> up() )
      sc += spec.dark_soul -> effectN( 1 ).percent() * ( 1.0 - glyphs.dark_soul -> effectN( 1 ).percent() );
    else if ( buffs.dark_soul -> cooldown -> remains() == timespan_t::zero() )
      sc += spec.dark_soul -> effectN( 1 ).percent() * glyphs.dark_soul -> effectN( 1 ).percent();
  }

  return sc;
}


double warlock_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( primary_tree() == WARLOCK_AFFLICTION )
  {
    if ( buffs.dark_soul -> up() )
      h *= 1.0 / ( 1.0 + spec.dark_soul -> effectN( 1 ).percent() * ( 1.0 - glyphs.dark_soul -> effectN( 1 ).percent() ) );
    else if ( buffs.dark_soul -> cooldown -> remains() == timespan_t::zero() )
      h *= 1.0 / ( 1.0 + spec.dark_soul -> effectN( 1 ).percent() * glyphs.dark_soul -> effectN( 1 ).percent() );
  }

  return h;
}


double warlock_t::composite_mastery() const
{
  double m = player_t::composite_mastery();

  if ( primary_tree() == WARLOCK_DEMONOLOGY )
  {
    if ( buffs.dark_soul -> up() )
      m += spec.dark_soul -> effectN( 1 ).average( this ) * ( 1.0 - glyphs.dark_soul -> effectN( 1 ).percent() ) / rating.mastery;
    else if ( buffs.dark_soul -> cooldown -> remains() == timespan_t::zero() )
      m += spec.dark_soul -> effectN( 1 ).average( this ) * glyphs.dark_soul -> effectN( 1 ).percent() / rating.mastery;
  }

  return m;
}


double warlock_t::composite_mp5() const
{
  double mp5 = player_t::composite_mp5();

  if ( spec.chaotic_energy -> ok() ) mp5 /= composite_spell_haste();

  return mp5;
}


double warlock_t::matching_gear_multiplier( const attribute_type_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return 0.05;

  return 0.0;
}


action_t* warlock_t::create_action( const std::string& name,
                                    const std::string& options_str )
{
  action_t* a;

  if      ( name == "conflagrate"           ) a = new           conflagrate_t( this );
  else if ( name == "corruption"            ) a = new            corruption_t( this );
  else if ( name == "agony"                 ) a = new                 agony_t( this );
  else if ( name == "doom"                  ) a = new                  doom_t( this );
  else if ( name == "chaos_bolt"            ) a = new            chaos_bolt_t( this );
  else if ( name == "chaos_wave"            ) a = new            chaos_wave_t( this );
  else if ( name == "curse_of_elements"     ) a = new     curse_of_elements_t( this );
  else if ( name == "demonic_slash"         ) a = new         demonic_slash_t( this );
  else if ( name == "drain_life"            ) a = new            drain_life_t( this );
  else if ( name == "drain_soul"            ) a = new            drain_soul_t( this );
  else if ( name == "grimoire_of_sacrifice" ) a = new grimoire_of_sacrifice_t( this );
  else if ( name == "haunt"                 ) a = new                 haunt_t( this );
  else if ( name == "immolate"              ) a = new              immolate_t( this );
  else if ( name == "incinerate"            ) a = new            incinerate_t( this );
  else if ( name == "life_tap"              ) a = new              life_tap_t( this );
  else if ( name == "malefic_grasp"         ) a = new         malefic_grasp_t( this );
  else if ( name == "metamorphosis"         ) a = new         metamorphosis_t( this );
  else if ( name == "cancel_metamorphosis"  ) a = new  cancel_metamorphosis_t( this );
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
  else if ( name == "void_ray"              ) a = new              void_ray_t( this );
  else if ( name == "dark_intent"           ) a = new           dark_intent_t( this );
  else if ( name == "dark_soul"             ) a = new             dark_soul_t( this );
  else if ( name == "soulburn"              ) a = new              soulburn_t( this );
  else if ( name == "bane_of_havoc"         ) a = new         bane_of_havoc_t( this );
  else if ( name == "seed_of_corruption"    ) a = new    seed_of_corruption_t( this );
  else if ( name == "rain_of_fire"          ) a = new          rain_of_fire_t( this );
  else if ( name == "carrion_swarm"         ) a = new         carrion_swarm_t( this );
  else if ( name == "imp_swarm"             ) a = new             imp_swarm_t( this );
  else if ( name == "fire_and_brimstone"    ) a = new    fire_and_brimstone_t( this );
  else if ( name == "service_felguard"      ) a = new grimoire_of_service_t( this, name );
  else if ( name == "service_felhunter"     ) a = new grimoire_of_service_t( this, name );
  else if ( name == "service_imp"           ) a = new grimoire_of_service_t( this, name );
  else if ( name == "service_succubus"      ) a = new grimoire_of_service_t( this, name );
  else if ( name == "service_voidwalker"    ) a = new grimoire_of_service_t( this, name );
  else return player_t::create_action( name, options_str );

  a -> parse_options( NULL, options_str );
  if ( a -> dtr_action ) a -> dtr_action -> parse_options( NULL, options_str );

  return a;
}


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

  if ( pet_name == "service_felguard"     ) return new    felguard_pet_t( sim, this, pet_name );
  if ( pet_name == "service_felhunter"    ) return new   felhunter_pet_t( sim, this, pet_name );
  if ( pet_name == "service_imp"          ) return new         imp_pet_t( sim, this, pet_name );
  if ( pet_name == "service_succubus"     ) return new    succubus_pet_t( sim, this, pet_name );
  if ( pet_name == "service_voidwalker"   ) return new  voidwalker_pet_t( sim, this, pet_name );

  return 0;
}


void warlock_t::create_pets()
{
  create_pet( "felguard"  );
  create_pet( "felhunter" );
  create_pet( "imp"       );
  create_pet( "succubus"  );
  create_pet( "voidwalker" );
  create_pet( "infernal"  );
  create_pet( "doomguard" );

  pets.wild_imps[ 0 ] = new wild_imp_pet_t( sim, this );

  for ( int i = 1; i < WILD_IMP_LIMIT; i++ )
  {
    pets.wild_imps[ i ] = new wild_imp_pet_t( sim, this, pets.wild_imps[ 0 ] );
  }

  create_pet( "service_felguard"  );
  create_pet( "service_felhunter" );
  create_pet( "service_imp"       );
  create_pet( "service_succubus"  );
  create_pet( "service_voidwalker" );
}



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

  spec.dark_soul = find_specialization_spell( "Dark Soul: Instability", "dark_soul" );
  if ( ! spec.dark_soul -> ok() ) spec.dark_soul = find_specialization_spell( "Dark Soul: Knowledge", "dark_soul" );
  if ( ! spec.dark_soul -> ok() ) spec.dark_soul = find_specialization_spell( "Dark Soul: Misery", "dark_soul" );

  // Affliction
  spec.nightfall     = find_specialization_spell( "Nightfall" );
  spec.malefic_grasp = find_specialization_spell( "Malefic Grasp" );

  // Demonology
  spec.decimation    = find_specialization_spell( "Decimation" );
  spec.demonic_fury  = find_specialization_spell( "Demonic Fury" );
  spec.metamorphosis = find_specialization_spell( "Metamorphosis" );
  spec.molten_core   = find_specialization_spell( "Molten Core" );
  
  spec.doom          = ( find_specialization_spell( "Metamorphosis: Doom"          ) -> ok() ) ? find_spell( 603 )    : spell_data_t::not_found();
  spec.demonic_slash = ( find_specialization_spell( "Metamorphosis: Demonic Slash" ) -> ok() ) ? find_spell( 103964 ) : spell_data_t::not_found();
  spec.chaos_wave    = ( find_specialization_spell( "Metamorphosis: Chaos Wave"    ) -> ok() ) ? find_spell( 124916 ) : spell_data_t::not_found();

  // Destruction
  spec.backdraft      = find_specialization_spell( "Backdraft" );
  spec.burning_embers = find_specialization_spell( "Burning Embers" );
  spec.chaotic_energy = find_specialization_spell( "Chaotic Energy" );

  // Mastery
  mastery_spells.emberstorm          = find_mastery_spell( WARLOCK_DESTRUCTION );
  mastery_spells.potent_afflictions  = find_mastery_spell( WARLOCK_AFFLICTION );
  mastery_spells.master_demonologist = find_mastery_spell( WARLOCK_DEMONOLOGY );

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
  glyphs.conflagrate            = find_glyph_spell( "Glyph of Conflagrate" );
  glyphs.dark_soul              = find_glyph_spell( "Glyph of Dark Soul" );
  glyphs.demon_training         = find_glyph_spell( "Glyph of Demon Training" );
  glyphs.life_tap               = find_glyph_spell( "Glyph of Life Tap" );
  glyphs.imp_swarm              = find_glyph_spell( "Glyph of Imp Swarm" );
  glyphs.everlasting_affliction = find_glyph_spell( "Everlasting Affliction" );
  glyphs.soul_shards            = find_glyph_spell( "Glyph of Soul Shards" );
  glyphs.burning_embers         = find_glyph_spell( "Glyph of Burning Embers" );
}


void warlock_t::init_base()
{
  player_t::init_base();

  base.attack_power = -10;
  initial.attack_power_per_strength = 2.0;
  initial.spell_power_per_intellect = 1.0;

  base.mp5 *= 1.0 + spec.chaotic_energy -> effectN( 1 ).percent();

  if ( primary_tree() == WARLOCK_AFFLICTION )  resources.base[ RESOURCE_SOUL_SHARD ]    = 3 + ( ( glyphs.soul_shards -> ok() ) ? 1 : 0 );
  if ( primary_tree() == WARLOCK_DEMONOLOGY )  resources.base[ RESOURCE_DEMONIC_FURY ]  = 1000;
  if ( primary_tree() == WARLOCK_DESTRUCTION ) resources.base[ RESOURCE_BURNING_EMBER ] = 30 + ( ( glyphs.burning_embers -> ok() ) ? 10 : 0 );

  diminished_kfactor    = 0.009830;
  diminished_dodge_capi = 0.006650;
  diminished_parry_capi = 0.006650;
}


void warlock_t::init_scaling()
{
  player_t::init_scaling();
  scales_with[ STAT_SPIRIT ] = 0;
  scales_with[ STAT_STAMINA ] = 0;
}


void warlock_t::init_buffs()
{
  player_t::init_buffs();

  buffs.backdraft             = buff_creator_t( this, "backdraft", find_spell( 117828 ) ).max_stack( 6 ); // FIXME: May be a bug, not sure
  buffs.dark_soul             = buff_creator_t( this, "dark_soul", spec.dark_soul );
  buffs.metamorphosis         = buff_creator_t( this, "metamorphosis", spec.metamorphosis );
  buffs.molten_core           = buff_creator_t( this, "molten_core", find_spell( 122355 ) ).max_stack( 99 ); // FIXME: May be a bug, not sure
  buffs.soulburn              = buff_creator_t( this, "soulburn", find_class_spell( "Soulburn" ) );
  buffs.grimoire_of_sacrifice = buff_creator_t( this, "grimoire_of_sacrifice", talents.grimoire_of_sacrifice );
  buffs.demonic_calling       = buff_creator_t( this, "demonic_calling", find_spell( 114925 ) ).duration( timespan_t::zero() );
  buffs.fire_and_brimstone    = buff_creator_t( this, "fire_and_brimstone", find_class_spell( "Fire and Brimstone" ) );
  buffs.tier13_4pc_caster     = buff_creator_t( this, "tier13_4pc_caster", find_spell( 105786 ) );
}


void warlock_t::init_values()
{
  player_t::init_values();

  if ( set_bonus.pvp_2pc_caster() )
    initial.attribute[ ATTR_INTELLECT ] += 70;

  if ( set_bonus.pvp_4pc_caster() )
    initial.attribute[ ATTR_INTELLECT ] += 90;
}


void warlock_t::init_gains()
{
  player_t::init_gains();

  gains.life_tap           = get_gain( "life_tap"     );
  gains.soul_leech         = get_gain( "soul_leech"   );
  gains.tier13_4pc         = get_gain( "tier13_4pc"   );
  gains.nightfall          = get_gain( "nightfall"    );
  gains.drain_soul         = get_gain( "drain_soul"   );
  gains.incinerate         = get_gain( "incinerate"   );
  gains.rain_of_fire       = get_gain( "rain_of_fire" );
  gains.fel_flame          = get_gain( "fel_flame"    );
  gains.seed_of_corruption = get_gain( "seed_of_corruption" );
  gains.shadowburn         = get_gain( "shadowburn" );
  gains.miss_refund        = get_gain( "miss_refund" );
}


void warlock_t::init_benefits()
{
  player_t::init_benefits();
}


void warlock_t::init_procs()
{
  player_t::init_procs();

  procs.wild_imp = get_proc( "wild_imp" );
}


void warlock_t::init_rng()
{
  player_t::init_rng();
  
  rngs.demonic_calling = get_rng( "demonic_calling" );
  rngs.molten_core     = get_rng( "molten_core" );
  rngs.nightfall       = get_rng( "nightfall" );
  rngs.ember_gain      = get_rng( "ember_gain" );
}

void warlock_t::add_action( std::string action, std::string options, std::string alist )
{
  add_action( find_class_spell( action ), options, alist );
}

void warlock_t::add_action( const spell_data_t* s, std::string options, std::string alist )
{
  std::string *str = ( alist == "default" ) ? &action_list_str : &( get_action_priority_list( alist ) -> action_list_str );
  if ( s -> ok() )
  {
    *str += "/" + dbc_t::get_token( s -> id() );
    if ( ! options.empty() ) *str += "," + options;
  }
}

void warlock_t::init_actions()
{
  if ( action_list_str.empty() )
  {
    std::string& precombat_list = get_action_priority_list( "precombat" ) -> action_list_str;

    // Flask
    if ( level > 85 )
      precombat_list = "flask,type=warm_sun";
    else if ( level >= 80 )
      precombat_list = "flask,type=draconic_mind";

    // Food
    if ( level >= 80 ) 
    { 
      precombat_list += "/food,type=";
      precombat_list += ( level > 85 ) ? "great_pandaren_banquet" : "seafood_magnifique_feast";
    }

    add_action( "Dark Intent", "if=!aura.spell_power_multiplier.up", "precombat" );

    std::string pet;

    switch ( primary_tree() )
    {
    case WARLOCK_DEMONOLOGY:
      pet = "felguard";
      break;
    default:
      pet = "felhunter";
    }

    precombat_list += "/summon_" + pet;

    precombat_list += "/snapshot_stats";

    // Pre-potion
    if ( level > 85 )
      precombat_list += "/jinyu_potion";
    else if ( level >= 80 )
      precombat_list += "/volcanic_potion";
 
    if ( talents.grimoire_of_service -> ok() )
      action_list_str += "/service_" + pet;

    add_action( talents.grimoire_of_sacrifice );
    if ( talents.grimoire_of_sacrifice -> ok() )
      action_list_str += "/summon_" + pet + ",if=buff.grimoire_of_sacrifice.down";

    // Usable Item
    for ( int i = items.size() - 1; i >= 0; i-- )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
      }
    }

    action_list_str += init_use_profession_actions();
    action_list_str += init_use_racial_actions();

    // Potion
    if ( level > 85 )
      action_list_str += "/jinyu_potion,if=buff.bloodlust.react|target.health.pct<=20";
    else if ( level > 80 )
      action_list_str += "/volcanic_potion,if=buff.bloodlust.react|target.health.pct<=20";

    add_action( spec.dark_soul );

    action_list_str += "/run_action_list,name=aoe,if=num_targets>1";

    add_action( "Summon Doomguard" );
    add_action( "Summon Infernal", "", "aoe" );

    switch ( primary_tree() )
    {

    case WARLOCK_AFFLICTION:
      add_action( "Drain Soul",            "if=soul_shard=0,interrupt_if=soul_shard!=0" );
      add_action( "Haunt",                 "if=!in_flight&target.debuff.haunt.remains<cast_time+travel_time" );
      add_action( "Agony",                 "if=(!ticking|remains<=action.drain_soul.new_tick_time)&target.time_to_die>=8&miss_react" );
      add_action( "Corruption",            "if=(!ticking|remains<tick_time)&target.time_to_die>=6&miss_react" );
      add_action( "Unstable Affliction",   "if=(!ticking|remains<(cast_time+tick_time))&target.time_to_die>=5&miss_react" );
      if ( glyphs.everlasting_affliction -> ok() )
      {
        add_action( "Agony",               "if=ticks_remain<add_ticks%2&target.time_to_die>=8&miss_react" );
        add_action( "Corruption",          "if=ticks_remain<add_ticks%2&target.time_to_die>=6&miss_react" );
        add_action( "Unstable Affliction", "if=ticks_remain<add_ticks%2+1&target.time_to_die>=5&miss_react" );
      }
      add_action( "Drain Soul",            "interrupt=1,chain=1,if=target.health.pct<=20" );
      add_action( "Life Tap",              "if=mana.pct<=35" );
      add_action( "Malefic Grasp",         "chain=1" );
      add_action( "Life Tap",              "moving=1,if=mana.pct<80&mana.pct<target.health.pct" );
      add_action( "Fel Flame",             "moving=1" );

      // AoE action list
      add_action( "Soulburn",              "if=buff.soulburn.down&cooldown.soulburn_seed_of_corruption.remains=0", "aoe" );
      add_action( "Seed of Corruption",    "cycle_targets=1,if=!in_flight_to_target&!ticking",                     "aoe" );
      add_action( "Life Tap",              "if=mana.pct<70",                                                       "aoe" );
      add_action( "Fel Flame",             "cycle_targets=1,if=!in_flight_to_target",                              "aoe" );
      break;

    case WARLOCK_DESTRUCTION:
      add_action( "Shadowburn",            "if=ember_react" );
      add_action( "Chaos Bolt",            "if=ember_react&buff.backdraft.stack<3" );
      add_action( "Conflagrate",           "if=buff.backdraft.down" );
      add_action( "Immolate",              "if=(!ticking|remains<(action.incinerate.cast_time+cast_time))&target.time_to_die>=5&miss_react" );
      add_action( "Rain of Fire",          "if=!ticking&!in_flight" );
      if ( glyphs.everlasting_affliction -> ok() )
        add_action( "Immolate",            "if=ticks_remain<add_ticks%2&target.time_to_die>=10&miss_react" );
      add_action( "Incinerate" );
      add_action( "Fel Flame",             "moving=1" );

      // AoE action list
      add_action( "Rain of Fire",          "if=!ticking&!in_flight",                                               "aoe" );
      add_action( "Fire and Brimstone",    "if=ember_react&buff.fire_and_brimstone.down",                          "aoe" );
      add_action( "Immolate",              "if=buff.fire_and_brimstone.up&!ticking",                               "aoe" );
      add_action( "Conflagrate",           "if=ember_react&buff.fire_and_brimstone.up",                            "aoe" );
      add_action( "Incinerate",            "if=buff.fire_and_brimstone.up",                                        "aoe" );
      add_action( "Immolate",              "cycle_targets=1,if=!ticking",                                          "aoe" );
      add_action( "Conflagrate",           "",                                                                     "aoe" );
      break;

    case WARLOCK_DEMONOLOGY:
      add_action( "Metamorphosis",         "if=(demonic_fury>=500&action.hand_of_guldan.charges=0)|demonic_fury>=target.time_to_die*8" );

      if ( find_class_spell( "Metamorphosis" ) -> ok() ) 
        action_list_str += "/cancel_metamorphosis,if=action.hand_of_guldan.charges=2";
      if ( glyphs.imp_swarm -> ok() ) 
        add_action( find_spell( 104316 ) );

      add_action( "Corruption",            "if=(!ticking|remains<tick_time)&target.time_to_die>=6&miss_react" );
      add_action( spec.doom,               "if=(!ticking|remains<tick_time)&target.time_to_die>=30&miss_react" );
      add_action( "Hand of Gul'dan",       "if=!in_flight&target.dot.shadowflame.remains<travel_time+action.shadow_bolt.cast_time" );
      add_action( "Soul Fire",             "if=buff.molten_core.react|target.health.pct<25" );
      if ( glyphs.everlasting_affliction -> ok() )
        add_action( spec.doom,             "if=demonic_fury<100&remains<40&miss_react" );
      add_action( spec.demonic_slash );
      add_action( "Shadow Bolt" );
      add_action( "Void Ray",              "moving=1" );
      add_action( "Fel Flame",             "moving=1" );

      // AoE action list
      add_action( "Metamorphosis",         "if=(demonic_fury>=500&action.hand_of_guldan.charges=0)|demonic_fury>=target.time_to_die*8", "aoe" );
      if ( glyphs.imp_swarm -> ok() ) 
        add_action( find_spell( 104316 ),  "",                                                                                          "aoe" );
      add_action( "Hand of Gul'dan",       "if=!in_flight",                                                                             "aoe" );
      add_action( "Chaos Wave",            "",                                                                                          "aoe" );
      add_action( "Rain of Fire",          "",                                                                                          "aoe" );
      add_action( "Life Tap",              "",                                                                                          "aoe" );
      break;

    default:
      add_action( "Corruption",            "if=(!ticking|remains<tick_time)&target.time_to_die>=6&miss_react" );
      add_action( "Shadow Bolt" );

      // AoE action list
      add_action( "Corruption",            "cycle_targets=1,if=!ticking",                                                               "aoe" );
      add_action( "Shadow Bolt",           "",                                                                                          "aoe" );
      break;
    }

    add_action( "Life Tap" );

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

  buffs.demonic_calling -> trigger();
  demonic_calling_event = new (sim) demonic_calling_event_t( this, rngs.demonic_calling -> range( timespan_t::zero(), timespan_t::from_seconds( 20 ) ) );
}


void warlock_t::reset()
{
  player_t::reset();

  // Active
  pets.active = 0;
  ember_react = timespan_t::max();
  event_t::cancel( meta_cost_event );
  event_t::cancel( demonic_calling_event );

  for ( int i = 0; i < NIGHTFALL_LIMIT; i++ )
  {
    nightfall_times[ i ] = timespan_t::min();
  }
  nightfall_index = -1;
}


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


bool warlock_t::create_profile( std::string& profile_str, save_type_e stype, bool save_html )
{
  player_t::create_profile( profile_str, stype, save_html );

  if ( stype == SAVE_ALL )
  {
    if ( use_pre_soulburn )            profile_str += "use_pre_soulburn=1\n";
    if ( initial_burning_embers != 0 ) profile_str += "burning_embers=" + util::to_string( initial_burning_embers ) + "\n";
    if ( initial_demonic_fury != 200 ) profile_str += "burning_embers=" + util::to_string( initial_demonic_fury ) + "\n";
  }

  return true;
}


void warlock_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warlock_t* p = debug_cast<warlock_t*>( source );

  use_pre_soulburn       = p -> use_pre_soulburn;
  initial_burning_embers = p -> initial_burning_embers;
  initial_demonic_fury   = p -> initial_demonic_fury;
}


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

bool warlock_t::verify_nightfall()
{
  int new_index = ( nightfall_index + 1 ) % NIGHTFALL_LIMIT;

  if ( nightfall_times[ new_index ] < sim -> current_time - timespan_t::from_minutes( 1 ) )
  {
    nightfall_times[ new_index ] = sim -> current_time;
    nightfall_index = new_index;
    return true;
  }
  else
  {
    return false;
  }
}

#endif // SC_WARLOCK

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

player_t* class_modules::create::warlock( sim_t* sim, const std::string& name, race_type_e r )
{
  return sc_create_class<warlock_t,SC_WARLOCK>()( "Warlock", sim, name, r );
}


void class_modules::init::warlock( sim_t* )
{
}

void class_modules::combat_begin::warlock( sim_t* )
{
}

void class_modules::combat_end::warlock( sim_t* )
{
}
