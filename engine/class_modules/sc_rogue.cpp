// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

// ==========================================================================
// Custom Combo Point Impl.
// ==========================================================================

struct rogue_t;

#define SC_ROGUE 0

#if SC_ROGUE == 1

#define COMBO_POINTS_MAX 5

static inline int clamp( int x, int low, int high )
{
  return x < low ? low : ( x > high ? high : x );
}

struct combo_points_t
{
  sim_t* sim;
  player_t* player;

  proc_t* proc;
  proc_t* wasted;

  int count;

  combo_points_t( player_t* p ) :
    sim( p -> sim ), player( p ), proc( 0 ), wasted( 0 ), count( 0 )
  {
    proc   = player -> get_proc( "combo_points" );
    wasted = player -> get_proc( "combo_points_wasted" );
  }

  void add( int num, const char* action = 0 )
  {
    int actual_num = clamp( num, 0, COMBO_POINTS_MAX - count );
    int overflow   = num - actual_num;

    // we count all combo points gained in the proc
    for ( int i = 0; i < num; i++ )
      proc -> occur();

    // add actual combo points
    if ( actual_num > 0 )
      count += actual_num;

    // count wasted combo points
    if ( overflow > 0 )
    {
      for ( int i = 0; i < overflow; i++ )
        wasted -> occur();
    }

    if ( sim -> log )
    {
      if ( action )
        sim -> output( "%s gains %d (%d) combo_points from %s (%d)",
                       player -> name(), actual_num, num, action, count );
      else
        sim -> output( "%s gains %d (%d) combo_points (%d)",
                       player -> name(), actual_num, num, count );
    }
  }

  void clear( const char* action = 0 )
  {
    if ( sim -> log )
    {
      if ( action )
        sim -> output( "%s spends %d combo_points on %s",
                       player -> name(), count, action );
      else
        sim -> output( "%s loses %d combo_points",
                       player -> name(), count );
    }

    count = 0;
  }

  double rank( double* cp_list ) const
  {
    assert( count > 0 );
    return cp_list[ count - 1 ];
  }

  double rank( double cp1, double cp2, double cp3, double cp4, double cp5 ) const
  {
    double cp_list[] = { cp1, cp2, cp3, cp4, cp5 };
    return rank( cp_list );
  }
};

// ==========================================================================
// Rogue
// ==========================================================================

// ==========================================================================
//  New Ability: Redirect, doable once we allow debuffs on multiple targets
//  Review: Ability Scaling
//  Review: Energy Regen (how Adrenaline rush stacks with Blade Flurry / haste)
// ==========================================================================

enum poison_e { POISON_NONE=0, DEADLY_POISON, WOUND_POISON, CRIPPLING_POISON, MINDNUMBING_POISON, LEECHING_POISON, PARALYTIC_POISON };

struct rogue_targetdata_t : public targetdata_t
{
  dot_t* dots_crimson_tempest;
  dot_t* dots_deadly_poison;
  dot_t* dots_garrote;
  dot_t* dots_hemorrhage;
  dot_t* dots_rupture;

  buff_t* debuffs_anticipation_charges;
  buff_t* debuffs_find_weakness;
  buff_t* debuffs_leeching_poison;
  buff_t* debuffs_vendetta;

  combo_points_t* combo_points;

  rogue_targetdata_t( rogue_t* source, player_t* target );

  virtual void reset()
  {
    targetdata_t::reset();

    combo_points->clear();
  }

  ~rogue_targetdata_t()
  {
    delete combo_points;
  }
};

struct rogue_t : public player_t
{
  // Active
  action_t* active_main_gauche;
  action_t* active_venomous_wound;

  // Benefits
  struct benefits_t
  {
    benefit_t* bandits_guile[ 3 ];
    benefit_t* energy_cap;
    benefit_t* poisoned;
  } benefits;

  // Buffs
  struct buffs_t
  {
    buff_t* adrenaline_rush;
    buff_t* bandits_guile;
    buff_t* blade_flurry;
    buff_t* deadly_poison;
    buff_t* deadly_proc;
    buff_t* envenom;
    buff_t* find_weakness;
    buff_t* leeching_poison;
    buff_t* killing_spree;
    buff_t* master_of_subtlety;
    buff_t* recuperate;
    buff_t* revealing_strike;
    buff_t* shadow_dance;
    buff_t* shadowstep;
    buff_t* shiv;
    buff_t* slice_and_dice;
    buff_t* stealthed;
    buff_t* tier13_2pc;
    buff_t* tot_trigger;
    buff_t* vanish;
    buff_t* vendetta;
    buff_t* wound_poison;

    // Legendary buffs
    buff_t* fof_fod; // Fangs of the Destroyer
    buff_t* fof_p1; // Phase 1
    buff_t* fof_p2;
    buff_t* fof_p3;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* adrenaline_rush;
    cooldown_t* honor_among_thieves;
    cooldown_t* killing_spree;
    cooldown_t* seal_fate;
  } cooldowns;

  // Expirations
  struct expirations_t_
  {
    event_t* wound_poison;

    void reset() { memset( ( void* ) this, 0x00, sizeof( expirations_t_ ) ); }
    expirations_t_() { reset(); }
  };
  expirations_t_ expirations_;

  // Gains
  struct gains_t
  {
    gain_t* adrenaline_rush;
    gain_t* combat_potency;
    gain_t* energy_refund;
    gain_t* murderous_intent;
    gain_t* overkill;
    gain_t* recuperate;
    gain_t* relentless_strikes;
    gain_t* vitality;
    gain_t* venomous_vim;
  } gains;

  struct glyphs_t
  {
  } glyphs;

  // Masteries
  struct masteries_t
  {
    const spell_data_t* executioner;
    const spell_data_t* main_gauche;
    const spell_data_t* potent_poisons;
  } masterys;

  // Procs
  struct procs_t
  {
    proc_t* deadly_poison;
    proc_t* wound_poison;
    proc_t* honor_among_thieves;
    proc_t* main_gauche;
    proc_t* seal_fate;
    proc_t* venomous_wounds;
  } procs;

  // Random Number Generation
  struct rngs_t
  {
    rng_t* combat_potency;
    rng_t* cut_to_the_chase;
    rng_t* deadly_poison;
    rng_t* hat_interval;
    rng_t* honor_among_thieves;
    rng_t* main_gauche;
    rng_t* relentless_strikes;
    rng_t* seal_fate;
    rng_t* venomous_wounds;
    rng_t* wound_poison;
  } rngs;

  // Spec passives
  struct spec_t
  {
    const spell_data_t* bandits_guile;
    const spell_data_t* bandits_guile_value;
    const spell_data_t* master_of_subtlety;
  } spec;

  // Spell Data
  struct spells_t
  {
    const spell_data_t* tier13_2pc;
    const spell_data_t* tier13_4pc;

    spells_t() { memset( ( void* ) this, 0x0, sizeof( spells_t ) ); }
  } spells;

  // Talents
  struct talents_t
  {
  } talents;

  action_callback_t* virtual_hat_callback;

  player_t* tot_target;

  // Options
  timespan_t virtual_hat_interval;
  std::string tricks_of_the_trade_target_str;

  uint32_t fof_p1, fof_p2, fof_p3;

  rogue_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, ROGUE, name, r ),
    active_main_gauche( 0 ), active_venomous_wound( 0 ),
    benefits( benefits_t() ),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    glyphs( glyphs_t() ),
    masterys( masteries_t() ),
    procs( procs_t() ),
    rngs( rngs_t() ),
    spec( spec_t() ),
    spells( spells_t() ),
    talents( talents_t() ),
    virtual_hat_callback( 0 ),
    tot_target( 0 ),
    virtual_hat_interval( timespan_t::min() ),
    tricks_of_the_trade_target_str( "" ),
    fof_p1( 0 ), fof_p2( 0 ), fof_p3( 0 )
  {

    // Cooldowns
    cooldowns.honor_among_thieves = get_cooldown( "honor_among_thieves" );
    cooldowns.seal_fate           = get_cooldown( "seal_fate"           );
    cooldowns.adrenaline_rush     = get_cooldown( "adrenaline_rush"     );
    cooldowns.killing_spree       = get_cooldown( "killing_spree"       );

    initial.distance = 3;

  }

  // Character Definition
  virtual rogue_targetdata_t* new_targetdata( player_t* target )
  { return new rogue_targetdata_t( this, target ); }
  virtual void      init_talents();
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_benefits();
  virtual void      init_rng();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_actions();
  virtual void      init_values();
  virtual void      register_callbacks();
  virtual void      combat_begin();
  virtual void      reset();
  virtual void      arise();
  virtual double    energy_regen_per_second() const;
  virtual void      regen( timespan_t periodicity );
  virtual timespan_t available() const;
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual expr_t*   create_expression( action_t* a, const std::string& name_str );
  virtual int       decode_set( const item_t& ) const;
  virtual resource_e primary_resource() const { return RESOURCE_ENERGY; }
  virtual role_e primary_role() const     { return ROLE_ATTACK; }
  virtual bool      create_profile( std::string& profile_str, save_e=SAVE_ALL, bool save_html=false ) const;
  virtual void      copy_from( player_t* source );

  virtual double    composite_attribute_multiplier( attribute_e attr ) const;
  virtual double    composite_attack_speed() const;
  virtual double    matching_gear_multiplier( attribute_e attr ) const;
  virtual double    composite_attack_power_multiplier() const;
  virtual double    composite_player_multiplier( school_e school, const action_t* a = NULL ) const;

  // Temporary
  virtual std::string set_default_talents() const
  {
    switch ( primary_tree() )
    {
    case SPEC_NONE: break;
    default: break;
    }

    return player_t::set_default_talents();
  }

  virtual std::string set_default_glyphs() const
  {
    switch ( primary_tree() )
    {
    case SPEC_NONE: break;
    default: break;
    }

    return player_t::set_default_glyphs();
  }
};

rogue_targetdata_t::rogue_targetdata_t( rogue_t* source, player_t* target ) :
  targetdata_t( source, target )
{
  combo_points = new combo_points_t( target );

  debuffs_anticipation_charges = add_aura( buff_creator_t( this, "anticipation_charges", source -> find_talent_spell( "Anticipation" ) ).max_stack( 5 ) );

  const spell_data_t* fw = source -> find_specialization_spell( "Find Weakness" );
  const spell_data_t* fwt = fw -> effectN( 1 ).trigger();
  debuffs_find_weakness = add_aura( buff_creator_t( this, "find_weakness", fw )
                                    .duration( fwt -> duration() )
                                    .default_value( fw -> effectN( 1 ).percent() ) );

  const spell_data_t* lp = source -> find_talent_spell( "Leeching Poison" );
  debuffs_leeching_poison = add_aura( buff_creator_t( this, "leeching_poison", lp )
                                      .chance( lp -> proc_chance() )
                                      .default_value( lp -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
                                      .duration( lp -> effectN( 1 ).trigger() -> duration() ) );

  const spell_data_t* vd = source -> find_specialization_spell( "Vendetta" );
  debuffs_vendetta = add_aura( buff_creator_t( this, "vendetta", vd )
                               .default_value( vd -> effectN( 1 ).percent() )
                               .duration( vd -> duration() ) );
}

// ==========================================================================
// Rogue Attack
// ==========================================================================

struct rogue_attack_state_t : public action_state_t
{
  int combo_points;

  rogue_attack_state_t( action_t* a, player_t* t ) :
    action_state_t( a, t ), combo_points( 0 )
  { }

  virtual void debug() const
  {
    action_state_t::debug();
    action -> sim -> output( "[NEW] %s %s %s: cp=%d",
                             action -> player -> name(),
                             action -> name(),
                             target -> name(),
                             combo_points );
  }

  virtual void copy_state( const action_state_t* o )
  {
    if ( o == 0 || this == o )
      return;

    action_state_t::copy_state( o );

    const rogue_attack_state_t* ds_ = static_cast< const rogue_attack_state_t* >( o );

    combo_points = ds_ -> combo_points;
  }
};

struct rogue_melee_attack_t : public melee_attack_t
{
  bool             requires_stealth_;
  position_e  requires_position_;
  bool             requires_combo_points;
  int              adds_combo_points;
  double           base_da_bonus;
  double           base_ta_bonus;
  weapon_e    requires_weapon;
  bool             affected_by_killing_spree;

  // we now track how much combo points we spent on an action
  int              combo_points_spent;

  rogue_melee_attack_t( const std::string& token, rogue_t* p,
                        const spell_data_t* s = spell_data_t::nil(),
                        const std::string& options = std::string() ) :
    melee_attack_t( token, p, s ),
    requires_stealth_( false ), requires_position_( POSITION_NONE ),
    requires_combo_points( false ), adds_combo_points( 0 ),
    base_da_bonus( 0.0 ), base_ta_bonus( 0.0 ),
    requires_weapon( WEAPON_NONE ),
    affected_by_killing_spree( true ),
    combo_points_spent( 0 )
  {
    parse_options( 0, options );

    may_crit                  = true;
    may_glance                = false;
    special                   = true;
    tick_may_crit             = true;
    stateless                 = true;
    hasted_ticks              = false;

    for ( size_t i = 1; i <= data()._effects -> size(); i++ )
    {
      switch ( data().effectN( i ).type() )
      {
      case E_ADD_COMBO_POINTS:
        adds_combo_points = data().effectN( i ).base_value();
        break;
      default:
        break;
      }

      if ( data().effectN( i ).type() == E_APPLY_AURA &&
           data().effectN( i ).subtype() == A_PERIODIC_DAMAGE )
        base_ta_bonus = data().effectN( i ).bonus( p );
      else if ( data().effectN( i ).type() == E_SCHOOL_DAMAGE )
        base_da_bonus = data().effectN( i ).bonus( p );
    }
  }

  rogue_t* cast() const
  { return debug_cast<rogue_t*>( player ); }

  rogue_t* p() const
  { return debug_cast<rogue_t*>( player ); }

  rogue_targetdata_t* cast_td( player_t* t = 0 ) const
  { return debug_cast<rogue_targetdata_t*>( action_t::targetdata( t ) ); }

  virtual double cost() const;
  virtual void   execute();
  virtual void   consume_resource();
  virtual bool   ready();

  virtual double calculate_weapon_damage( double /* attack_power */ );
  virtual void   assess_damage( player_t* t, double, dmg_e, result_e );
  virtual double armor() const;

  virtual bool   requires_stealth() const
  {
    if ( p() -> buff.shadow_dance -> check() ||
         p() -> buff.stealthed -> check() ||
         p() -> buff.vanish -> check() )
    {
      return false;
    }

    return requires_stealth_;
  }

  virtual position_e requires_position() const
  {
    return requires_position_;
  }

  action_state_t* get_state( const action_state_t* s )
  {
    action_state_t* s_ = melee_attack_t::get_state( s );

    if ( s == 0 )
    {
      rogue_attack_state_t* ds_ = static_cast< rogue_attack_state_t* >( s_ );
      ds_ -> combo_points = 0;
    }

    return s_;
  }

  action_state_t* new_state()
  {
    return new rogue_attack_state_t( this, target );
  }

  // Combo points need to be snapshot before we travel, they should also not
  // be snapshot during any other event in the stateless system.
  void schedule_travel_s( action_state_t* travel_state )
  {
    if ( result_is_hit( travel_state -> result ) )
    {
      rogue_attack_state_t* ds_ = static_cast< rogue_attack_state_t* >( travel_state );
      ds_ -> combo_points = td( travel_state -> target ) -> combo_points -> count;
    }

    melee_attack_t::schedule_travel_s( travel_state );
  }
};

// ==========================================================================
// Rogue Poison
// ==========================================================================

struct rogue_poison_t : public rogue_melee_attack_t
{
  rogue_poison_t( const std::string& token, rogue_t* p,
                  const spell_data_t* s = spell_data_t::nil() ) :
    rogue_melee_attack_t( token, p, s )
  {
    proc              = true;
    background        = true;
    trigger_gcd       = timespan_t::zero();
    may_dodge         = false;
    may_parry         = false;
    may_block         = false;

    weapon_multiplier = 0;

    // Poisons are spells that use attack power
    base_spell_power_multiplier  = 0.0;
    base_attack_power_multiplier = 1.0;
  }

  virtual double action_multiplier() const
  {
    double cm = rogue_melee_attack_t::action_multiplier();

    if ( p() -> mastery.potent_poisons -> ok() )
      cm *= 1.0 + p() -> mastery.potent_poisons -> effectN( 1 ).mastery_value() * p() -> composite_mastery();

    return cm;
  }
};

// ==========================================================================
// Static Functions
// ==========================================================================

// break_stealth ============================================================

static void break_stealth( rogue_t* p )
{
  if ( p -> buff.stealthed -> check() )
  {
    p -> buff.stealthed -> expire();

    if ( p -> spec.master_of_subtlety -> ok() )
      p -> buff.master_of_subtlety -> trigger();
  }

  if ( p -> buff.vanish -> check() )
  {
    p -> buff.vanish -> expire();

    if ( p -> spec.master_of_subtlety -> ok() )
      p -> buff.master_of_subtlety -> trigger();
  }
}

// trigger_bandits_guile ====================================================

static void trigger_bandits_guile( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();

  if ( ! p -> spec.bandits_guile -> ok() || ! p -> spec.bandits_guile_value -> ok() )
    return;

  int current_stack = p -> buff.bandits_guile -> check();
  if ( current_stack == p -> buff.bandits_guile -> max_stack() )
    return; // we can't refresh the 30% buff

  p -> buff.bandits_guile -> trigger( 1, ( ( current_stack ) / 4 ) * ( p -> spec.bandits_guile_value -> effectN( 1 ).percent() / 3 ) );
}

// trigger_combat_potency ===================================================

static void trigger_combat_potency( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();

  if ( ! p -> spec.combat_potency -> ok() )
    return;

  if ( p -> rng.combat_potency -> roll( p -> spec.combat_potency -> proc_chance() ) )
  {
    // energy gain value is in the proc trigger spell
    p -> resource_gain( RESOURCE_ENERGY,
                        p -> spec.combat_potency -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_ENERGY ),
                        p -> gains.combat_potency );
  }
}

// trigger_cut_to_the_chase =================================================

static void trigger_cut_to_the_chase( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();

  if ( ! p -> talents.cut_to_the_chase -> rank() )
    return;

  if ( ! p -> buffs.slice_and_dice -> check() )
    return;

  if ( p -> rng_cut_to_the_chase -> roll( p -> talents.cut_to_the_chase -> proc_chance() ) )
    p -> buffs.slice_and_dice -> trigger( COMBO_POINTS_MAX );
}

// trigger_initiative =======================================================

static void trigger_initiative( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();

  if ( ! p -> talents.initiative -> rank() )
    return;

  if ( p -> rng_initiative -> roll( p -> talents.initiative -> effectN( 1 ).percent() ) )
  {
    rogue_targetdata_t* td = a -> cast_td();
    td -> combo_points -> add( p -> dbc.spell( p -> talents.initiative -> effectN( 1 ).trigger_spell_id() ) -> effectN( 1 ).base_value(),
                               p -> talents.initiative );
  }
}

// trigger_energy_refund ====================================================

static void trigger_energy_refund( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();

  double energy_restored = a -> resource_consumed * 0.80;

  p -> resource_gain( RESOURCE_ENERGY, energy_restored, p -> gains.energy_refund );
}

// trigger_find_weakness ====================================================

static void trigger_find_weakness( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();

  if ( ! p -> talents.find_weakness -> rank() )
    return;

  p -> buffs.find_weakness -> trigger();
};

// trigger_relentless_strikes ===============================================

static void trigger_relentless_strikes( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();

  if ( ! p -> talents.relentless_strikes -> rank() )
    return;

  if ( ! a -> requires_combo_points )
    return;

  rogue_targetdata_t* td = a -> cast_td();
  double chance = p -> talents.relentless_strikes -> effect_pp_combo_points( 1 ) / 100.0;
  if ( p -> rng_relentless_strikes -> roll( chance * td -> combo_points -> count ) )
  {
    double gain = p -> dbc.spell( p -> talents.relentless_strikes -> effectN( 1 ).trigger_spell_id() ) -> effectN( 1 ).resource( RESOURCE_ENERGY );
    p -> resource_gain( RESOURCE_ENERGY, gain, p -> gains.relentless_strikes );
  }
}

// trigger_restless_blades ==================================================

static void trigger_restless_blades( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();

  if ( ! p -> talents.restless_blades -> rank() )
    return;

  if ( ! a -> requires_combo_points )
    return;

  timespan_t reduction = p -> talents.restless_blades -> effectN( 1 ).time_value() * a -> combo_points_spent;

  p -> cooldowns_adrenaline_rush -> ready -= reduction;
  p -> cooldowns_killing_spree -> ready   -= reduction;
}

// trigger_ruthlessness =====================================================

static void trigger_ruthlessness( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();

  if ( ! p -> talents.ruthlessness -> rank() )
    return;

  if ( ! a -> requires_combo_points )
    return;

  if ( p -> rng_ruthlessness -> roll( p -> talents.ruthlessness -> proc_chance() ) )
  {
    rogue_targetdata_t* td = a -> cast_td();
    p -> procs.ruthlessness -> occur();
    td -> combo_points -> add( 1, p -> talents.ruthlessness );
  }
}

// trigger_seal_fate ========================================================

static void trigger_seal_fate( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();

  if ( ! p -> talents.seal_fate -> rank() )
    return;

  if ( ! a -> adds_combo_points )
    return;

  // This is to prevent dual-weapon special attacks from triggering a double-proc of Seal Fate
  if ( p -> cooldowns_seal_fate -> remains() > timespan_t::zero() )
    return;

  if ( p -> rng_seal_fate -> roll( p -> talents.seal_fate -> proc_chance() ) )
  {
    rogue_targetdata_t* td = a -> cast_td();
    p -> cooldowns_seal_fate -> start( timespan_t::from_millis( 1 ) );
    p -> procs.seal_fate -> occur();
    td -> combo_points -> add( 1, p -> talents.seal_fate );
  }
}

// trigger_main_gauche ======================================================

static void trigger_main_gauche( rogue_melee_attack_t* a )
{
  if ( a -> proc )
    return;

  weapon_t* w = a -> weapon;

  if ( ! w || w -> slot != SLOT_MAIN_HAND )
    return;

  rogue_t* p = a -> cast();

  if ( ! p -> mastery_main_gauche -> ok() )
    return;

  double chance = p -> composite_mastery() * p -> mastery_main_gauche -> base_value( E_APPLY_AURA, A_DUMMY, 0 );

  if ( p -> rng_main_gauche -> roll( chance ) )
  {
    if ( ! p -> active_main_gauche )
    {
      struct main_gouche_t : public rogue_melee_attack_t
      {
        main_gouche_t( rogue_t* p ) : rogue_melee_attack_t( "main_gauche", p )
        {
          weapon = &( p -> main_hand_weapon );

          base_dd_min = base_dd_max = 1;
          background      = true;
          trigger_gcd     = timespan_t::zero();
          may_glance      = false; // XXX: does not glance
          may_crit        = true;
          normalize_weapon_speed = true; // XXX: it's normalized
          proc = true; // it's proc; therefore it cannot trigger main_gauche for chain-procs

          init();
        }

        virtual void execute()
        {
          rogue_melee_attack_t::execute();
          if ( result_is_hit() )
          {
            trigger_combat_potency( this );
          }
        }
      };
      p -> active_main_gauche = new main_gouche_t( p );
    }

    p -> active_main_gauche -> execute();
    p -> procs.main_gauche  -> occur();
  }
}

// trigger_tricks_of_the_trade ==============================================

static void trigger_tricks_of_the_trade( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();

  if ( ! p -> buffs.tot_trigger -> check() )
    return;

  player_t* t = p -> tot_target;

  if ( t )
  {
    timespan_t duration = p -> dbc.spell( 57933 ) -> duration();

    if ( t -> buffs.tricks_of_the_trade -> remains_lt( duration ) )
    {
      double value = p -> dbc.spell( 57933 ) -> effectN( 1 ).base_value();
      value += p -> glyphs.tricks_of_the_trade -> mod_additive( P_EFFECT_1 );

      t -> buffs.tricks_of_the_trade -> buff_duration = duration;
      t -> buffs.tricks_of_the_trade -> trigger( 1, value / 100.0 );
    }

    p -> buffs.tot_trigger -> expire();
  }
}

// trigger_venomous_wounds ==================================================

static void trigger_venomous_wounds( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();
  rogue_targetdata_t* td = a -> cast_td();

  if ( ! p -> talents.venomous_wounds -> rank() )
    return;

  // we currently only check for presence of deadly poison
  if ( ! td -> debuffs_poison_doses -> check() )
    return;

  if ( p -> rng_venomous_wounds -> roll( p -> talents.venomous_wounds -> proc_chance() ) )
  {
    if ( ! p -> active_venomous_wound )
    {
      struct venomous_wound_t : public rogue_poison_t
      {
        venomous_wound_t( rogue_t* p ) :
          rogue_poison_t( "venomous_wound", 79136, p )
        {
          background       = true;
          proc             = true;
          direct_power_mod = extra_coeff();
          init();
        }
      };
      p -> active_venomous_wound = new venomous_wound_t( p );
    }

    p -> procs.venomous_wounds -> occur();
    p -> active_venomous_wound -> execute();

    // Venomous Vim (51637) lists only 2 energy for some reason
    p -> resource_gain( RESOURCE_ENERGY, 10, p -> gains.venomous_vim );
  }
}

// apply_poison_debuff ======================================================

static void apply_poison_debuff( rogue_t* p, player_t* t )
{
  if ( ! p -> sim -> overrides.magic_vulnerability )
  {
    const spell_data_t* mp = p -> dbc.spell( 93068 );
    t -> debuffs.magic_vulnerability -> trigger( 1, -1.0, -1.0, mp -> duration() );
  }
}

// remove_poison_debuff =====================================================

static void remove_poison_debuff( rogue_t* p )
{
  player_t* t = p -> sim -> target;
  t -> debuffs.magic_vulnerability -> expire();
}

// ==========================================================================
// Attacks
// ==========================================================================

double rogue_melee_attack_t::armor() const
{
  rogue_t* p = cast();

  double a = melee_attack_t::armor();

  a *= 1.0 - p -> buffs.find_weakness -> value();

  return a;
}

// rogue_melee_attack_t::cost =====================================================

double rogue_melee_attack_t::cost() const
{
  double c = melee_attack_t::cost();

  rogue_t* p = cast();

  if ( c <= 0 )
    return 0;

  if ( p -> set_bonus.tier13_2pc_melee() && p -> buffs.tier13_2pc -> up() )
    c *= 1.0 + p -> spells.tier13_2pc -> effectN( 1 ).percent();

  return c;
}

// rogue_melee_attack_t::consume_resource =========================================

void rogue_melee_attack_t::consume_resource()
{
  melee_attack_t::consume_resource();

  rogue_t* p = cast();

  combo_points_spent = 0;

  if ( result_is_hit() )
  {
    trigger_relentless_strikes( this );

    if ( requires_combo_points )
    {
      rogue_targetdata_t* td = cast_td();
      combo_points_spent = td -> combo_points -> count;

      td -> combo_points -> clear( name() );

      if ( p -> buffs.fof_fod -> up() )
        td -> combo_points -> add( 5, "legendary_daggers" );
    }

    trigger_ruthlessness( this );
  }
  else
    trigger_energy_refund( this );
}

// rogue_melee_attack_t::execute ==================================================

void rogue_melee_attack_t::execute()
{
  rogue_t* p = cast();

  if ( harmful )
    break_stealth( p );

  // simple actions do not invoke any base ::execute()
  // nor any triggers
  if ( simple )
  {
    // if we have a buff attached to the action, trigger it now
    trigger_buff();

    consume_resource();

    if ( sim -> log )
      sim -> output( "%s performs %s", p -> name(), name() );

    add_combo_points();

    update_ready();
  }
  else
  {
    melee_attack_t::execute();

    if ( result_is_hit() )
    {
      add_combo_points();

      if ( result == RESULT_CRIT )
        trigger_seal_fate( this );

      // attached buff get's triggered only on successfull hit
      trigger_buff();

      if ( may_crit )  // Rupture and Garrote can't proc MG, issue 581
        trigger_main_gauche( this );

      trigger_apply_poisons( this );
      trigger_tricks_of_the_trade( this );
    }

    // Prevent non-critting abilities (Rupture, Garrote, Shiv; maybe something else) from eating Cold Blood
    if ( may_crit )
      p -> buffs.cold_blood -> expire();
  }
}

// rogue_melee_attack_t::calculate_weapon_damage ==================================

double rogue_melee_attack_t::calculate_weapon_damage( double attack_power )
{
  double dmg = melee_attack_t::calculate_weapon_damage( attack_power );

  if ( dmg == 0 ) return 0;

  rogue_t* p = cast();

  if ( weapon -> slot == SLOT_OFF_HAND && p -> spec.ambidexterity -> ok() )
  {
    dmg *= 1.0 + p -> spec.ambidexterity -> effectN( 1 ).percent();
  }

  return dmg;
}

// rogue_melee_attack_t::player_buff ==============================================

void rogue_melee_attack_t::player_buff()
{
  melee_attack_t::player_buff();

  rogue_t* p = cast();

  if ( p -> buffs.cold_blood -> check() )
    player_crit += p -> buffs.cold_blood -> effectN( 1 ).percent();
}

// rogue_melee_attack_t::total_multiplier =========================================

double rogue_melee_attack_t::total_multiplier() const
{
  // we have to overwrite it because Executioner is additive with talents
  rogue_t* p = cast();

  double add_mult = 0.0;

  if ( requires_combo_points && p -> mastery_executioner -> ok() )
    add_mult = p -> composite_mastery() * p -> mastery_executioner -> effect_coeff( 1 ) / 100.0;

  double m = 1.0;
  // dynamic damage modifiers:

  // Bandit's Guile (combat) - affects all damage done by rogue, stacks are reset when you strike other target with sinister strike/revealing strike
  m *= 1.0 + p -> buffs.bandits_guile -> value();

  // Killing Spree (combat) - affects all direct damage (but you cannot use specials while it is up)
  // affects deadly poison ticks
  // Exception: DOESN'T affect rupture/garrote ticks
  if ( affected_by_killing_spree )
    m *= 1.0 + p -> buffs.killing_spree -> value();

  // Sanguinary Vein (subtlety) - dynamically increases all damage as long as target is bleeding
  if ( target -> debuffs.bleeding -> check() )
    m *= 1.0 + p -> talents.sanguinary_vein -> effectN( 1 ).percent();

  // Vendetta (Assassination) - dynamically increases all damage as long as target is affected by Vendetta
  m *= 1.0 + p -> buffs.vendetta -> value();

  return ( base_multiplier + add_mult ) * player_multiplier * target_multiplier * m;
}

// rogue_melee_attack_t::ready() ==================================================

bool rogue_melee_attack_t::ready()
{
  rogue_t* p = cast();

  if ( requires_combo_points )
  {
    rogue_targetdata_t* td = cast_td();
    if ( ! td -> combo_points -> count )
      return false;
  }

  if ( requires_stealth )
  {
    if ( ! p -> buffs.shadow_dance -> check() &&
         ! p -> buffs.stealthed -> check() &&
         ! p -> buffs.vanish -> check() )
    {
      return false;
    }
  }

  //Killing Spree blocks all rogue actions for duration
  if ( p -> buffs.killing_spree -> check() )
    if ( ( special == true ) && ( proc == false ) )
      return false;

  if ( requires_position != POSITION_NONE )
    if ( p -> position != requires_position )
      return false;

  if ( requires_weapon != WEAPON_NONE )
    if ( ! weapon || weapon -> type != requires_weapon )
      return false;

  return melee_attack_t::ready();
}

// rogue_melee_attack_t::assess_damage ============================================

void rogue_melee_attack_t::assess_damage( player_t* t,
                                          const double amount,
                                          const dmg_e dmg_type,
                                          const result_e impact_result )
{
  melee_attack_t::assess_damage( t, amount, dmg_type, impact_result );

  /*rogue_t* p = cast();

  // XXX: review, as not all of the damage is 'flurried' to an additional target
  // dots for example don't as far as I remember
  if ( t -> is_enemy() )
  {
    target_t* q = t -> cast_target();

    if ( p -> buffs.blade_flurry -> up() && q -> adds_nearby )
      melee_attack_t::additional_damage( q, amount, dmg_e, impact_result );
  }*/
}


// Melee Attack =============================================================

struct melee_t : public rogue_melee_attack_t
{
  int sync_weapons;

  melee_t( const char* name, rogue_t* p, int sw ) :
    rogue_melee_attack_t( name, p ), sync_weapons( sw )
  {
    base_dd_min = base_dd_max = 1;
    background      = true;
    repeating       = true;
    trigger_gcd     = timespan_t::zero();
    may_crit        = true;

    if ( p -> dual_wield() )
      base_hit -= 0.19;
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = rogue_melee_attack_t::execute_time();

    if ( ! player -> in_combat )
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t/2, timespan_t::from_seconds( 0.2 ) ) : t/2 ) : timespan_t::from_seconds( 0.01 );

    return t;
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    if ( result_is_hit() )
    {
      rogue_t* p = cast();
      if ( weapon -> slot == SLOT_OFF_HAND )
      {
        trigger_combat_potency( this );
      }

      // Legendary Daggers buff handling
      // Proc rates from: https://github.com/Aldriana/ShadowCraft-Engine/blob/master/shadowcraft/objects/proc_data.py#L504
      // Logic from: http://code.google.com/p/simulationcraft/issues/detail?id=1118
      double fof_chance = ( p -> primary_tree() == TREE_ASSASSINATION ) ? 0.235 : ( p -> primary_tree() == TREE_COMBAT ) ? 0.095 : 0.275;
      if ( sim -> roll( fof_chance ) )
      {
        p -> buffs.fof_p1 -> trigger();
        p -> buffs.fof_p2 -> trigger();

        if ( ! p -> buffs.fof_fod -> check() && p -> buffs.fof_p3 -> check() > 30 )
        {
          // Trigging FoF and the Stacking Buff are mutually exclusive
          if ( sim -> roll( 1.0 / ( 50.0 - p -> buffs.fof_p3 -> check() ) ) )
          {
            p -> buffs.fof_fod -> trigger();
            rogue_targetdata_t* td = cast_td();
            td -> combo_points -> add( 5, "legendary_daggers" );
          }
          else
          {
            p -> buffs.fof_p3 -> trigger();
          }
        }
        else
        {
          p -> buffs.fof_p3 -> trigger();
        }
      }
    }
  }
};

// Auto Attack ==============================================================

struct auto_melee_attack_t : public action_t
{
  int sync_weapons;

  auto_melee_attack_t( rogue_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "auto_attack", p ),
    sync_weapons( 0 )
  {
    option_t options[] =
    {
      { "sync_weapons", OPT_BOOL, &sync_weapons },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    assert( p -> main_hand_weapon.type != WEAPON_NONE );

    p -> main_hand_attack = new melee_t( "melee_main_hand", p, sync_weapons );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> off_hand_attack = new melee_t( "melee_off_hand", p, sync_weapons );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    rogue_t* p = cast();

    p -> main_hand_attack -> schedule_execute();

    if ( p -> off_hand_attack )
      p -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    rogue_t* p = cast();

    if ( p -> is_moving() )
      return false;

    return ( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Adrenaline Rush ==========================================================

struct adrenaline_rush_t : public rogue_melee_attack_t
{
  adrenaline_rush_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "adrenaline_rush", p -> talents.adrenaline_rush -> spell_id(), p, true )
  {
    check_talent( p -> talents.adrenaline_rush -> ok() );

    add_trigger_buff( p -> buffs.adrenaline_rush );

    parse_options( NULL, options_str );
  }
};

// Ambush ===================================================================

struct ambush_t : public rogue_melee_attack_t
{
  ambush_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "ambush", "Ambush", p )
  {
    parse_options( NULL, options_str );

    requires_position      = POSITION_BACK;
    requires_stealth       = true;

    if ( weapon -> type == WEAPON_DAGGER )
      weapon_multiplier   *= 1.447; // It'is in the description.

    base_costs[ current_resource() ]             += p -> talents.slaughter_from_the_shadows -> effectN( 1 ).resource( RESOURCE_ENERGY );
    base_multiplier       *= 1.0 + ( p -> talents.improved_ambush -> effectN( 2 ).percent() +
                                     p -> talents.opportunity     -> effectN( 1 ).percent() );
    base_crit             += p -> talents.improved_ambush -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    rogue_t* p = cast();

    if ( result_is_hit() )
    {
      trigger_initiative( this );
      trigger_find_weakness( this );
    }

    p -> buffs.shadowstep -> expire();
  }

  virtual void player_buff()
  {
    rogue_melee_attack_t::player_buff();

    rogue_t* p = cast();

    player_multiplier *= 1.0 + p -> buffs.shadowstep -> value();
  }
};

// Backstab =================================================================

struct backstab_t : public rogue_melee_attack_t
{
  backstab_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "backstab", 53, p )
  {
    requires_weapon    = WEAPON_DAGGER;
    requires_position  = POSITION_BACK;

    weapon_multiplier *= 1.0 + p -> spec.sinister_calling -> mod_additive( P_EFFECT_2 );

    base_costs[ current_resource() ]         += p -> talents.slaughter_from_the_shadows -> effectN( 1 ).resource( RESOURCE_ENERGY );
    base_multiplier   *= 1.0 + ( p -> talents.aggression  -> mod_additive( P_GENERIC ) +
                                 p -> talents.opportunity -> mod_additive( P_GENERIC ) );
    base_crit         += p -> talents.puncturing_wounds -> effectN( 1 ).percent();
    crit_bonus_multiplier *= 1.0 + p -> talents.lethality -> mod_additive( P_CRIT_DAMAGE );

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    rogue_t* p = cast();

    if ( result_is_hit() )
    {
      if ( p -> glyphs.backstab -> ok() && result == RESULT_CRIT )
      {
        double amount = p -> glyphs.backstab -> base_value( E_APPLY_AURA );
        p -> resource_gain( RESOURCE_ENERGY, amount, p -> gains.backstab_glyph );
      }

      if ( p -> talents.murderous_intent -> rank() )
      {
        // the first effect of the talent is the amount energy gained
        // and the second one is target health percent apparently
        double health_pct = p -> talents.murderous_intent -> effectN( 2 ).base_value();
        if ( target -> health_percentage() < health_pct )
        {
          double amount = p -> talents.murderous_intent -> effectN( 1 ).resource( RESOURCE_ENERGY );
          p -> resource_gain( RESOURCE_ENERGY, amount, p -> gains.murderous_intent );
        }
      }
    }
  }
};

// Blade Flurry =============================================================

struct blade_flurry_t : public rogue_melee_attack_t
{
  blade_flurry_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "blade_flurry", p -> spec.blade_flurry -> spell_id(), p, true )
  {
    add_trigger_buff( p -> buffs.blade_flurry );

    parse_options( NULL, options_str );
  }
};

// Cold Blood ===============================================================

struct cold_blood_t : public rogue_melee_attack_t
{
  cold_blood_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "cold_blood", p -> talents.cold_blood -> spell_id(), p, true )
  {
    add_trigger_buff( p -> buffs.cold_blood );

    harmful = false;

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    rogue_t* p = cast();

    p -> resource_gain( RESOURCE_ENERGY, base_value( E_ENERGIZE ), p -> gains.cold_blood );
  }
};

// Envenom ==================================================================

struct envenom_t : public rogue_melee_attack_t
{
  double dose_dmg;

  envenom_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "envenom", 32645, p )
  {
    // to trigger poisons
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    requires_combo_points = true;

    base_multiplier *= 1.0 + p -> talents.coup_de_grace -> mod_additive( P_GENERIC );

    dose_dmg = effect_average( 1 );

    option_t options[] =
    {
      { "min_doses", OPT_DEPRECATED, NULL },
      { "no_buff",   OPT_DEPRECATED, NULL },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
  }

  virtual void execute()
  {
    rogue_t* p = cast();
    rogue_targetdata_t* td = cast_td();

    p -> buffs.envenom -> trigger( td -> combo_points -> count );

    rogue_melee_attack_t::execute();

    if ( result_is_hit() )
    {
      int doses_consumed = std::min( td -> debuffs_poison_doses -> current_stack, combo_points_spent );

      if ( ! p -> talents.master_poisoner -> rank() )
        td -> debuffs_poison_doses -> decrement( doses_consumed );


      trigger_cut_to_the_chase( this );
      trigger_restless_blades( this );

      p -> buffs.revealing_strike -> expire();
    }
  }

  virtual void player_buff()
  {
    rogue_t* p = cast();
    rogue_targetdata_t* td = cast_td();

    int doses_consumed = std::min( td -> debuffs_poison_doses -> current_stack,
                                   td -> combo_points -> count );

    direct_power_mod = td -> combo_points -> count * 0.09;

    base_dd_min = base_dd_max = doses_consumed * dose_dmg;

    rogue_melee_attack_t::player_buff();

    if ( p -> buffs.revealing_strike -> up() )
      player_multiplier *= 1.0 + p -> buffs.revealing_strike -> value();
  }

  virtual double total_multiplier() const
  {
    // we have to overwrite it because Potent Poisons is additive with talents
    rogue_t* p = cast();

    double add_mult = 0.0;

    if ( p -> mastery_potent_poisons -> ok() )
      add_mult = p -> composite_mastery() * p -> mastery_potent_poisons -> base_value( E_APPLY_AURA, A_DUMMY );

    double m = 1.0;
    // dynamic damage modifiers:

    // Bandit's Guile (combat) - affects all damage done by rogue, stacks are reset when you strike other target with sinister strike/revealing strike
    if ( p -> buffs.bandits_guile -> check() )
      m *= 1.0 + p -> buffs.bandits_guile -> value();

    // Killing Spree (combat) - affects all direct damage (but you cannot use specials while it is up)
    // affects deadly poison ticks
    // Exception: DOESN'T affect rupture/garrote ticks
    if ( p -> buffs.killing_spree -> check() && affected_by_killing_spree )
      m *= 1.0 + p -> buffs.killing_spree -> value();

    // Sanguinary Vein (subtlety) - dynamically increases all damage as long as target is bleeding
    if ( target -> debuffs.bleeding -> check() )
      m *= 1.0 + p -> talents.sanguinary_vein -> base_value() / 100.0;

    // Vendetta (Assassination) - dynamically increases all damage as long as target is affected by Vendetta
    if ( p -> buffs.vendetta -> check() )
      m *= 1.0 + p -> buffs.vendetta -> value();

    return ( base_multiplier + add_mult ) * player_multiplier * target_multiplier * m;
  }

  virtual bool ready()
  {
    rogue_targetdata_t* td = cast_td();

    // Envenom is not usable when there is no DP on a target
    if ( ! td -> debuffs_poison_doses -> check() )
      return false;

    return rogue_melee_attack_t::ready();
  }
};

// Eviscerate ===============================================================

struct eviscerate_t : public rogue_melee_attack_t
{
  double combo_point_dmg_min[ COMBO_POINTS_MAX ];
  double combo_point_dmg_max[ COMBO_POINTS_MAX ];

  eviscerate_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "eviscerate", 2098, p )
  {
    // to trigger poisons
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    requires_combo_points = true;

    base_multiplier *= 1.0 + ( p -> talents.aggression    -> mod_additive( P_GENERIC ) +
                               p -> talents.coup_de_grace -> mod_additive( P_GENERIC ) );
    base_crit       += p -> glyphs.eviscerate -> mod_additive( P_CRIT );

    double min_     = effect_min( 1 );
    double max_     = effect_max( 1 );
    double cp_bonus = effect_bonus( 1 );

    for ( int i = 0; i < COMBO_POINTS_MAX; i++ )
    {
      combo_point_dmg_min[ i ] = min_ + cp_bonus * ( i + 1 );
      combo_point_dmg_max[ i ] = max_ + cp_bonus * ( i + 1 );
    }

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_t* p = cast();
    rogue_targetdata_t* td = cast_td();

    base_dd_min = td -> combo_points -> rank( combo_point_dmg_min );
    base_dd_max = td -> combo_points -> rank( combo_point_dmg_max );

    direct_power_mod = 0.091 * td -> combo_points -> count;

    rogue_melee_attack_t::execute();

    if ( result_is_hit() )
    {
      trigger_cut_to_the_chase( this );
      trigger_restless_blades( this );

      if ( p -> talents.serrated_blades-> rank() &&
           td -> dots_rupture -> ticking )
      {
        double chance = p -> talents.serrated_blades -> base_value() / 100.0 * combo_points_spent;
        if ( p -> rng_serrated_blades -> roll( chance ) )
        {
          td -> dots_rupture -> refresh_duration();
          p -> procs.serrated_blades -> occur();
        }
      }

      p -> buffs.revealing_strike -> expire();
    }
  }

  virtual void player_buff()
  {
    rogue_melee_attack_t::player_buff();

    rogue_t* p = cast();

    if ( p -> buffs.revealing_strike -> up() )
      player_multiplier *= 1.0 + p -> buffs.revealing_strike -> value();
  }
};

// Expose Armor =============================================================

struct expose_armor_t : public rogue_melee_attack_t
{
  expose_armor_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "expose_armor", 8647, p )
  {
    // to trigger poisons
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    parse_options( 0, options_str );
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    rogue_t* p = cast();

    if ( result_is_hit() )
    {
      if ( ! sim -> overrides.weakened_armor )
        target -> debuffs.weakened_armor -> trigger();

      rogue_targetdata_t* td = cast_td();
      td -> combo_points -> add( 1 );

      p -> buffs.revealing_strike -> expire();
    }
  };
};

// Fan of Knives ============================================================

struct fan_of_knives_t : public rogue_melee_attack_t
{
  fan_of_knives_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "fan_of_knives", 51723, p )
  {
    parse_options( NULL, options_str );

    aoe    = -1;
    weapon = &( p -> ranged_weapon );

    base_costs[ current_resource() ] += p -> talents.slaughter_from_the_shadows -> effectN( 2 ).resource( RESOURCE_ENERGY );
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    rogue_t* p = cast();

    // FIXME: are these independent rolls for each hand?
    if ( p -> rng_vile_poisons -> roll( p -> talents.vile_poisons ->effectN( 3 ).percent() ) )
    {
      if ( &( p -> main_hand_weapon ) )
        trigger_apply_poisons( this, &( p -> main_hand_weapon ) );
      if ( &( p -> off_hand_weapon ) )
        trigger_apply_poisons( this, &( p -> off_hand_weapon ) );
    }
  }

  virtual bool ready()
  {
    rogue_t* p = cast();
    if ( p -> ranged_weapon.type != WEAPON_THROWN )
      return false;

    return rogue_melee_attack_t::ready();
  }
};

// Feint ====================================================================

struct feint_t : public rogue_melee_attack_t
{
  feint_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "feint", 1966, p )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_t* p = cast();

    if ( sim -> log )
      sim -> output( "%s performs %s", p -> name(), name() );

    consume_resource();
    update_ready();
    // model threat eventually......
  }
};

// Garrote ==================================================================

struct garrote_t : public rogue_melee_attack_t
{
  garrote_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "garrote", 703, p )
  {
    affected_by_killing_spree = false;

    // to trigger poisons
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    // to stop seal fate procs
    may_crit          = false;

    requires_position = POSITION_BACK;
    requires_stealth  = true;

    tick_power_mod    = .07;

    base_multiplier  *= 1.0 + p -> talents.opportunity -> mod_additive( P_TICK_DAMAGE );

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    rogue_t* p = cast();

    if ( result_is_hit() )
    {
      trigger_initiative( this );
      trigger_find_weakness( this );
    }

    p -> buffs.shadowstep -> expire();
  }

  virtual void player_buff()
  {
    rogue_melee_attack_t::player_buff();

    rogue_t* p = cast();

    if ( p -> buffs.shadowstep -> check() )
      player_multiplier *= 1.0 + p -> buffs.shadowstep -> value();
  }

  virtual void tick( dot_t* d )
  {
    rogue_melee_attack_t::tick( d );

    trigger_venomous_wounds( this );
  }
};

// Hemorrhage ===============================================================

struct hemorrhage_t : public rogue_melee_attack_t
{
  hemorrhage_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "hemorrhage", p -> talents.hemorrhage -> spell_id(), p )
  {
    // It does not have CP gain effect in spell dbc for soem reason
    adds_combo_points = 1;

    if ( weapon -> type == WEAPON_DAGGER )
      weapon_multiplier *= 1.45; // number taken from spell description

    weapon_multiplier *= 1.0 + p -> spec.sinister_calling -> mod_additive( P_EFFECT_2 );

    base_costs[ current_resource() ]       += p -> talents.slaughter_from_the_shadows -> effectN( 2 ).resource( RESOURCE_ENERGY );
    crit_bonus_multiplier *= 1.0 + p -> talents.lethality -> mod_additive( P_CRIT_DAMAGE );

    parse_options( NULL, options_str );

    if ( p -> glyphs.hemorrhage -> ok() )
    {
      num_ticks = 8;
      base_tick_time = timespan_t::from_seconds( 3.0 );
      dot_behavior = DOT_REFRESH;
    }
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    rogue_t* p = cast();

    if ( result_is_hit() )
    {
      if ( p -> glyphs.hemorrhage -> ok() )
      {
        // Dot can crit and double dips in player multipliers
        // Damage is based off actual damage, not normalized to hits like FFB
        // http://elitistjerks.com/f78/t105429-cataclysm_mechanics_testing/p6/#post1796877
        double dot_dmg = calculate_direct_damage( result, 0, total_attack_power(), total_spell_power(), total_dd_multiplier(), target ) * p -> glyphs.hemorrhage -> base_value() / 100.0;
        base_td = dot_dmg / num_ticks;
      }
    }
  }
};

// Kick =====================================================================

struct kick_t : public rogue_melee_attack_t
{
  kick_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "kick", 1766, p )
  {
    base_dd_min = base_dd_max = 1;
    base_attack_power_multiplier = 0.0;

    parse_options( NULL, options_str );

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;

    if ( p -> glyphs.kick -> ok() )
    {
      // All kicks are assumed to interrupt a cast
      cooldown -> duration -= timespan_t::from_seconds( 2.0 ); // + 4 Duration - 6 on Success = -2
    }
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return rogue_melee_attack_t::ready();
  }
};

// Killing Spree ============================================================

struct killing_spree_tick_t : public rogue_melee_attack_t
{
  killing_spree_tick_t( rogue_t* p, const char* name ) :
    rogue_melee_attack_t( name, p, true )
  {
    background  = true;
    may_crit    = true;
    direct_tick = true;
    normalize_weapon_speed = true;
    base_dd_min = base_dd_max = 1;
  }
};

struct killing_spree_t : public rogue_melee_attack_t
{
  melee_attack_t* attack_mh;
  melee_attack_t* attack_oh;

  killing_spree_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "killing_spree", p -> talents.killing_spree -> spell_id(), p ),
    attack_mh( 0 ), attack_oh( 0 )
  {
    add_trigger_buff( p -> buffs.killing_spree );

    num_ticks = 5;
    base_tick_time = effect_period( 1 );

    may_crit   = false;

    parse_options( NULL, options_str );

    attack_mh = new killing_spree_tick_t( p, "killing_spree_mh" );
    attack_mh -> weapon = &( player -> main_hand_weapon );
    add_child( attack_mh );

    if ( player -> off_hand_weapon.type != WEAPON_NONE )
    {
      attack_oh = new killing_spree_tick_t( p, "killing_spree_oh" );
      attack_oh -> weapon = &( player -> off_hand_weapon );
      add_child( attack_oh );
    }
  }

  virtual void tick( dot_t* d )
  {
    if ( sim -> debug ) sim -> output( "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );

    attack_mh -> execute();

    if ( attack_oh && attack_mh -> result_is_hit() )
    {
      attack_oh -> execute();
    }

    stats -> add_tick( d -> time_to_tick );
  }

  // Killing Spree not modified by haste effects
  virtual double haste() const { return 1.0; }
  virtual double swing_haste() const { return 1.0; }
};

// Mutilate =================================================================

struct mutilate_strike_t : public rogue_melee_attack_t
{
  mutilate_strike_t( rogue_t* p, const char* name ) :
    rogue_melee_attack_t( name, 5374, p )
  {
    background  = true;
    may_miss = may_dodge = may_parry = false;

    base_multiplier  *= 1.0 + p -> talents.opportunity -> mod_additive( P_GENERIC );
    base_crit        += p -> talents.puncturing_wounds -> effectN( 2 ).percent();
    crit_bonus_multiplier *= 1.0 + p -> talents.lethality -> mod_additive( P_CRIT_DAMAGE );
  }
};

struct mutilate_t : public rogue_melee_attack_t
{
  melee_attack_t* mh_strike;
  melee_attack_t* oh_strike;

  mutilate_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "mutilate", p -> spec.mutilate -> spell_id(), p ), mh_strike( 0 ), oh_strike( 0 )
  {
    may_crit = false;

    if ( p -> main_hand_weapon.type != WEAPON_DAGGER ||
         p ->  off_hand_weapon.type != WEAPON_DAGGER )
    {
      sim -> errorf( "Player %s attempting to execute Mutilate without two daggers equipped.", p -> name() );
      sim -> cancel();
    }

    base_costs[ current_resource() ] += p -> glyphs.mutilate -> mod_additive( P_RESOURCE_COST );

    parse_options( NULL, options_str );

    mh_strike = new mutilate_strike_t( p, "mutilate_mh" );
    mh_strike -> weapon = &( p -> main_hand_weapon );
    add_child( mh_strike );

    oh_strike = new mutilate_strike_t( p, "mutilate_oh" );
    oh_strike -> weapon = &( p -> off_hand_weapon );
    add_child( oh_strike );
  }

  virtual void execute()
  {
    melee_attack_t::execute();
    if ( result_is_hit() )
    {
      mh_strike -> execute();
      oh_strike -> execute();

      add_combo_points();

      if ( mh_strike -> result == RESULT_CRIT ||
           oh_strike -> result == RESULT_CRIT )
      {
        trigger_seal_fate( this );
      }
    }
    else
    {
      trigger_energy_refund( this );
    }
  }
};

// Premeditation ============================================================

struct premeditation_t : public rogue_melee_attack_t
{
  premeditation_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "premeditation", p -> talents.premeditation -> spell_id(), p, true )
  {
    harmful = false;
    requires_stealth = true;

    parse_options( NULL, options_str );
  }
};

// Recuperate ===============================================================

struct recuperate_t : public rogue_melee_attack_t
{
  recuperate_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "recuperate", 73651, p )
  {
    requires_combo_points = true;
    base_tick_time = effect_period( 1 );
    parse_options( NULL, options_str );
    dot_behavior = DOT_REFRESH;
    harmful = false;
  }

  virtual void execute()
  {
    rogue_t* p = cast();
    rogue_targetdata_t* td = cast_td();
    num_ticks = 2 * td -> combo_points -> count;
    rogue_melee_attack_t::execute();
    p -> buffs.recuperate -> trigger();
  }

  virtual void tick( dot_t* /* d */ )
  {
    rogue_t* p = cast();
    if ( p -> talents.energetic_recovery -> rank() )
      p -> resource_gain( RESOURCE_ENERGY, p -> talents.energetic_recovery -> base_value(), p -> gains.recuperate );
  }

  virtual void last_tick( dot_t* d )
  {
    rogue_t* p = cast();
    p -> buffs.recuperate -> expire();
    rogue_melee_attack_t::last_tick( d );
  }
};

// Revealing Strike =========================================================

struct revealing_strike_t : public rogue_melee_attack_t
{
  revealing_strike_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "revealing_strike", p -> talents.revealing_strike -> spell_id(), p )
  {
    add_trigger_buff( p -> buffs.revealing_strike );

    parse_options( NULL, options_str );

    // Legendary buff increases RS damage
    if ( p -> fof_p1 || p -> fof_p2 || p -> fof_p3 )
      base_multiplier *= 1.45; // FIX ME: once dbc data exists 1.0 + p -> dbc.spell( 110211 ) -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    if ( result_is_hit() )
    {
      trigger_bandits_guile( this );
    }
  }
};

// Rupture ==================================================================

struct rupture_t : public rogue_melee_attack_t
{
  double combo_point_dmg[ COMBO_POINTS_MAX ];

  rupture_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "rupture", 1943, p )
  {
    affected_by_killing_spree = false;

    // to trigger poisons
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    // to stop seal fate procs
    may_crit              = false;
    requires_combo_points = true;
    dot_behavior          = DOT_REFRESH;

    double base     = effect_average( 1 );
    double cp_bonus = effect_bonus( 1 );

    for ( int i = 0; i < COMBO_POINTS_MAX; i++ )
      combo_point_dmg[ i ] = base + cp_bonus * ( i + 1 );

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_t* p = cast();

    // We have to save these values for refreshes by Serrated Baldes, so
    // we simply reset them to zeroes on each execute and check them in ::player_buff.
    tick_power_mod = 0.0;
    base_td        = 0.0;

    rogue_melee_attack_t::execute();

    if ( result_is_hit() )
    {
      p -> buffs.revealing_strike -> expire();
      trigger_restless_blades( this );
    }
  }

  virtual void impact( player_t* t, result_e impact_result, double travel_dmg )
  {
    rogue_t* p = cast();
    if ( result_is_hit( impact_result ) )
      num_ticks = 3 + combo_points_spent + ( int )( p -> glyphs.rupture -> mod_additive_time( P_DURATION ) / base_tick_time );
    rogue_melee_attack_t::impact( t, impact_result, travel_dmg );
  }

  virtual void player_buff()
  {
    rogue_t* p = cast();

    // If the values are zeroes, we are called from ::execute and we must update.
    // If the values are initialized, we are called from ::refresh_duration and we don't
    // need to update.
    if ( tick_power_mod == 0.0 && base_td == 0.0 )
    {
      rogue_targetdata_t* td = cast_td();
      tick_power_mod = td -> combo_points -> rank( 0.015, 0.024, 0.030, 0.03428571, 0.0375 );
      base_td = td -> combo_points -> rank( combo_point_dmg );
    }

    rogue_melee_attack_t::player_buff();

    if ( p -> buffs.revealing_strike -> up() )
      player_multiplier *= 1.0 + p -> buffs.revealing_strike -> value();
  }

  virtual void tick( dot_t* d )
  {
    rogue_melee_attack_t::tick( d );

    trigger_venomous_wounds( this );
  }
};

// Shadowstep ===============================================================

struct shadowstep_t : public rogue_melee_attack_t
{
  shadowstep_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "shadowstep", p -> spec.shadowstep -> spell_id(), p, true )
  {
    add_trigger_buff( p -> buffs.shadowstep );

    harmful = false;

    parse_options( NULL, options_str );
  }
};

// Shiv =====================================================================

struct shiv_t : public rogue_melee_attack_t
{
  shiv_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "shiv", 5938, p )
  {
    weapon = &( p -> off_hand_weapon );
    if ( weapon -> type == WEAPON_NONE )
      background = true; // Do not allow execution.

    base_costs[ current_resource() ]        += weapon -> swing_time.total_seconds() * 10.0;
    adds_combo_points = 1;
    may_crit          = false;
    base_dd_min = base_dd_max = 1;

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_t* p = cast();

    p -> buffs.shiv -> trigger();
    rogue_melee_attack_t::execute();
    p -> buffs.shiv -> expire();
  }
};

// Sinister Strike ==========================================================

struct sinister_strike_t : public rogue_melee_attack_t
{
  sinister_strike_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "sinister_strike", 1752, p )
  {
    adds_combo_points = 1; // it has an effect but with no base value :rollseyes:

    base_costs[ current_resource() ]       += p -> talents.improved_sinister_strike -> mod_additive( P_RESOURCE_COST );
    base_multiplier *= 1.0 + ( p -> talents.aggression               -> mod_additive( P_GENERIC ) +
                               p -> talents.improved_sinister_strike -> mod_additive( P_GENERIC ) );
    crit_bonus_multiplier *= 1.0 + p -> talents.lethality -> mod_additive( P_CRIT_DAMAGE );

    // Legendary buff increases SS damage
    if ( p -> fof_p1 || p -> fof_p2 || p -> fof_p3 )
      base_multiplier *= 1.45; // FIX ME: once dbc data exists 1.0 + p -> dbc.spell( 110211 ) -> effectN( 1 ).percent();

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    rogue_t* p = cast();

    if ( result_is_hit() )
    {
      trigger_bandits_guile( this );

      if ( p -> rng_sinister_strike_glyph -> roll( p -> glyphs.sinister_strike -> proc_chance() ) )
      {
        rogue_targetdata_t* td = cast_td();
        p -> procs.sinister_strike_glyph -> occur();
        td -> combo_points -> add( 1, p -> glyphs.sinister_strike );
      }
    }
  }
};

// Slice and Dice ===========================================================

struct slice_and_dice_t : public rogue_melee_attack_t
{
  slice_and_dice_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "slice_and_dice", 5171, p )
  {
    requires_combo_points = true;

    harmful = false;

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_t* p = cast();
    rogue_targetdata_t* td = cast_td();
    int action_cp = td -> combo_points -> count;

    rogue_melee_attack_t::execute();

    p -> buffs.slice_and_dice -> trigger ( action_cp );

  }
};

// Pool Energy ==============================================================

struct pool_energy_t : public action_t
{
  timespan_t wait;
  int for_next;
  action_t* next_action;

  pool_energy_t( rogue_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "pool_energy", p ),
    wait( timespan_t::from_seconds( 0.5 ) ), for_next( 0 ),
    next_action( 0 )
  {
    option_t options[] =
    {
      { "wait",     OPT_TIMESPAN, &wait     },
      { "for_next", OPT_BOOL,     &for_next },
      { 0,              OPT_UNKNOWN,  0 }
    };
    parse_options( options, options_str );
  }

  virtual void init()
  {
    action_t::init();

    if ( for_next )
    {
      std::vector<action_t*>::iterator pos = range::find( player -> foreground_action_list, this );
      assert( pos != player -> foreground_action_list.end() );
      if ( ++pos != player -> foreground_action_list.end() )
        next_action = *pos;
      else
      {
        sim -> errorf( "%s: can't find next action.\n", __FUNCTION__ );
        sim -> cancel();
      }
    }
  }

  virtual void execute()
  {
    if ( sim -> log )
      sim -> output( "%s performs %s", player -> name(), name() );
  }

  virtual timespan_t gcd() const
  {
    return wait;
  }

  virtual bool ready()
  {
    if ( next_action )
    {
      if ( next_action -> ready() )
        return false;

      // If the next action in the list would be "ready" if it was not constrained by energy,
      // then this command will pool energy until we have enough.

      player -> resources.current[ RESOURCE_ENERGY ] += 100;

      bool energy_limited = next_action -> ready();

      player -> resources.current[ RESOURCE_ENERGY ] -= 100;

      if ( ! energy_limited )
        return false;
    }

    return action_t::ready();
  }
};

// Preparation ==============================================================

struct preparation_t : public rogue_melee_attack_t
{
  std::vector<cooldown_t*> cooldown_list;

  preparation_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "preparation", p -> talents.preparation -> spell_id(), p, true )
  {
    harmful = false;

    cooldown_list.push_back( p -> get_cooldown( "shadowstep" ) );
    cooldown_list.push_back( p -> get_cooldown( "vanish"     ) );

    if ( p -> glyphs.preparation -> ok() )
      cooldown_list.push_back( p -> get_cooldown( "kick" ) );

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    int num_cooldowns = ( int ) cooldown_list.size();
    for ( int i = 0; i < num_cooldowns; i++ )
      cooldown_list[ i ] -> reset();
  }
};

// Shadow Dance =============================================================

struct shadow_dance_t : public rogue_melee_attack_t
{
  shadow_dance_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "shadow_dance", p -> talents.shadow_dance -> spell_id(), p, true )
  {
    add_trigger_buff( p -> buffs.shadow_dance );

    p -> buffs.shadow_dance -> buff_duration += p -> glyphs.shadow_dance -> mod_additive_time( P_DURATION );
    if ( p -> set_bonus.tier13_4pc_melee() )
      p -> buffs.shadow_dance -> buff_duration += p -> spells.tier13_4pc -> effectN( 1 ).time_value();

    parse_options( NULL, options_str );
  }
};

// Tricks of the Trade ======================================================

struct tricks_of_the_trade_t : public rogue_melee_attack_t
{
  tricks_of_the_trade_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "tricks_of_the_trade", 57934, p )
  {
    parse_options( NULL, options_str );

    if ( p -> glyphs.tricks_of_the_trade -> ok() )
      base_costs[ current_resource() ] = 0;

    if ( ! p -> tricks_of_the_trade_target_str.empty() )
    {
      target_str = p -> tricks_of_the_trade_target_str;
    }

    if ( target_str.empty() )
    {
      target = p;
    }
    else
    {
      if ( target_str == "self" ) // This is only for backwards compatibility
      {
        target = p;
      }
      else
      {
        player_t* q = sim -> find_player( target_str );

        if ( q )
          target = q;
        else
        {
          sim -> errorf( "%s %s: Unable to locate target '%s'.\n", player -> name(), name(), options_str.c_str() );
          target = p;
        }
      }
    }

    p -> tot_target = target;
  }

  virtual void execute()
  {
    rogue_t* p = cast();

    rogue_melee_attack_t::execute();

    p -> buffs.tier13_2pc -> trigger();
    p -> buffs.tot_trigger -> trigger();
  }
};

// Vanish ===================================================================

struct vanish_t : public rogue_melee_attack_t
{
  vanish_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "vanish", 1856, p, true )
  {
    add_trigger_buff( p -> buffs.vanish );

    harmful = false;

    cooldown -> duration += p -> talents.elusiveness -> effectN( 1 ).time_value();

    parse_options( NULL, options_str );
  }

  virtual bool ready()
  {
    rogue_t* p = cast();

    if ( p -> buffs.stealthed -> check() )
      return false;

    return rogue_melee_attack_t::ready();
  }
};

// Vendetta =================================================================

struct vendetta_t : public rogue_melee_attack_t
{
  vendetta_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "vendetta", p -> talents.vendetta -> spell_id(), p, true )
  {
    add_trigger_buff( p -> buffs.vendetta );

    harmful = false;

    parse_options( NULL, options_str );
  }
};

// ==========================================================================
// Poisons
// ==========================================================================

// rogue_poison_t::total_multiplier =========================================

double rogue_poison_t::total_multiplier() const
{
  // we have to overwrite it because Potent Poisons is additive with talents
  rogue_t* p = cast();

  double add_mult = 0.0;

  if ( p -> mastery_potent_poisons -> ok() )
    add_mult = p -> composite_mastery() * p -> mastery_potent_poisons -> base_value( E_APPLY_AURA, A_DUMMY );

  double m = 1.0;
  // dynamic damage modifiers:

  // Bandit's Guile (combat) - affects all damage done by rogue, stacks are reset when you strike other target with sinister strike/revealing strike
  if ( p -> buffs.bandits_guile -> check() )
    m *= 1.0 + p -> buffs.bandits_guile -> value();

  // Killing Spree (combat) - affects all direct damage (but you cannot use specials while it is up)
  // affects deadly poison ticks
  if ( p -> buffs.killing_spree -> check() )
    m *= 1.0 + p -> buffs.killing_spree -> value();

  // Sanguinary Vein (subtlety) - dynamically increases all damage as long as target is bleeding
  if ( target -> debuffs.bleeding -> check() )
    m *= 1.0 + p -> talents.sanguinary_vein -> base_value() / 100.0;

  // Vendetta (Assassination) - dynamically increases all damage as long as target is affected by Vendetta
  if ( p -> buffs.vendetta -> check() )
    m *= 1.0 + p -> buffs.vendetta -> value();

  return ( base_multiplier + add_mult ) * player_multiplier * target_multiplier * m;
}

// Deadly Poison ============================================================

struct deadly_poison_t : public rogue_poison_t
{
  deadly_poison_t( rogue_t* player ) :  rogue_poison_t( "deadly_poison", 2818, player )
  {
    tick_power_mod = extra_coeff();
    may_crit   = false;
  }

  virtual void execute()
  {
    rogue_t* p = cast();

    int success = p -> buffs.shiv -> check() || p -> buffs.deadly_proc -> check();

    if ( ! success )
    {
      double chance = 0.30;

      chance += p -> spec.improved_poisons -> mod_additive( P_PROC_CHANCE );

      if ( p -> buffs.envenom -> up() )
        chance += p -> buffs.envenom -> base_value( E_APPLY_AURA, A_ADD_FLAT_MODIFIER ) / 100.0;

      success = p -> rng_deadly_poison -> roll( chance );
    }

    if ( success )
    {
      p -> procs.deadly_poison -> occur();

      player_buff();
      target_debuff( target, DMG_DIRECT );
      calculate_result( total_crit(), target -> level );

      if ( result_is_hit() )
      {
        rogue_targetdata_t* td = cast_td();
        if ( td -> debuffs_poison_doses -> check() == ( int ) max_stacks() )
        {
          p -> buffs.deadly_proc -> trigger();
          weapon_t* other_w = ( weapon -> slot == SLOT_MAIN_HAND ) ? &( p -> off_hand_weapon ) : &( p -> main_hand_weapon );
          trigger_other_poisons( p, other_w );
          p -> buffs.deadly_proc -> expire();
        }

        td -> debuffs_poison_doses -> trigger();

        if ( sim -> log )
          sim -> output( "%s performs %s (%d)", player -> name(), name(), td -> debuffs_poison_doses -> current_stack );

        if ( dot() -> ticking )
        {
          dot() -> refresh_duration();
        }
        else
        {
          schedule_travel( target );
          apply_poison_debuff( p, target );
        }
      }

      update_ready();
      stats -> add_execute( time_to_execute );
    }
  }

  virtual double calculate_tick_damage( result_e r, double power, double multiplier, player_t* target )
  {
    rogue_targetdata_t* td = cast_td( target );
    return rogue_poison_t::calculate_tick_damage( r, power, multiplier, target ) * td -> debuffs_poison_doses -> stack();
  }

  virtual void last_tick( dot_t* d )
  {
    rogue_poison_t::last_tick( d );

    rogue_t* p = cast();
    rogue_targetdata_t* td = cast_td();

    td -> debuffs_poison_doses -> expire();
    remove_poison_debuff( p );
  }
};

// Instant Poison ===========================================================

struct instant_poison_t : public rogue_poison_t
{
  instant_poison_t( rogue_t* player ) : rogue_poison_t( "instant_poison", 8680, player )
  {
    direct_power_mod = extra_coeff();
  }

  virtual void execute()
  {
    rogue_t* p = cast();
    double chance;

    if ( p -> buffs.deadly_proc -> check() )
    {
      chance = 1.0;
      may_crit = true;
    }
    else if ( p -> buffs.shiv -> check() )
    {
      chance = 1.0;
      may_crit = false;
    }
    else
    {
      double m = 1.0;

      m += p -> spec.improved_poisons -> mod_additive( P_PROC_FREQUENCY );

      if ( p -> buffs.envenom -> up() )
        m += p -> buffs.envenom -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER );

      chance = weapon -> proc_chance_on_swing( 8.57 * m );
      may_crit = true;
    }

    if ( p -> rng_instant_poison -> roll( chance ) )
      rogue_poison_t::execute();
  }
};

// Wound Poison =============================================================

struct wound_poison_t : public rogue_poison_t
{
  wound_poison_t( rogue_t* player ) : rogue_poison_t( "wound_poison", 13218, player )
  {
    direct_power_mod = extra_coeff();
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      player_t* target;
      expiration_t( sim_t* sim, rogue_t* p, player_t* t ) : event_t( sim, p ), target( t )
      {
        name = "Wound Poison Expiration";
        apply_poison_debuff( p, target );
        sim -> add_event( this, timespan_t::from_seconds( 15.0 ) );
      }

      virtual void execute()
      {
        rogue_t* p = cast();
        p -> expirations_.wound_poison = 0;
        remove_poison_debuff( p );
      }
    };

    rogue_t* p = cast();
    double chance;

    if ( p -> buffs.deadly_proc -> check() )
    {
      chance = 1.0;
      may_crit = true;
    }
    else if ( p -> buffs.shiv -> check() )
    {
      chance = 1.0;
      may_crit = false;
    }
    else
    {
      double PPM = 21.43;
      chance = weapon -> proc_chance_on_swing( PPM );
      may_crit = true;
    }

    if ( p -> rng_wound_poison -> roll( chance ) )
    {
      rogue_poison_t::execute();

      if ( result_is_hit() )
      {
        event_t*& e = p -> expirations_.wound_poison;

        if ( e )
          e -> reschedule( timespan_t::from_seconds( 15.0 ) );
        else
          e = new ( sim ) expiration_t( sim, p, target );
      }
    }
  }
};

// Apply Poison =============================================================

struct apply_poison_t : public action_t
{
  int main_hand_poison;
  int  off_hand_poison;
  int    thrown_poison;

  apply_poison_t( rogue_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "apply_poison", p ),
    main_hand_poison( 0 ), off_hand_poison( 0 ), thrown_poison( 0 )
  {
    std::string main_hand_str;
    std::string  off_hand_str;
    std::string    thrown_str;

    option_t options[] =
    {
      { "main_hand", OPT_STRING, &main_hand_str },
      {  "off_hand", OPT_STRING,  &off_hand_str },
      {    "thrown", OPT_STRING,    &thrown_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( main_hand_str.empty() &&
         off_hand_str.empty() &&
         thrown_str.empty() )
    {
      sim -> errorf( "Player %s: At least one of 'main_hand/off_hand/thrown' must be specified in 'apply_poison'.  Accepted values are 'deadly/instant/wound'.\n", p -> name() );
      sim -> cancel();
    }

    trigger_gcd = timespan_t::zero();

    if ( p -> main_hand_weapon.type != WEAPON_NONE )
    {
      if ( main_hand_str == "deadly"    ) main_hand_poison =     DEADLY_POISON;
      if ( main_hand_str == "wound"     ) main_hand_poison =      WOUND_POISON;
    }

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( off_hand_str == "deadly"     ) off_hand_poison =  DEADLY_POISON;
      if ( off_hand_str == "wound"      ) off_hand_poison =   WOUND_POISON;
    }
    if ( p -> ranged_weapon.type == WEAPON_THROWN )
    {
      if ( thrown_str == "deadly"       ) thrown_poison =  DEADLY_POISON;
      if ( thrown_str == "wound"        ) thrown_poison =   WOUND_POISON;
    }
  }

  virtual void execute()
  {
    rogue_t* p = cast();

    if ( sim -> log )
      sim -> output( "%s performs %s", player -> name(), name() );

    if ( main_hand_poison ) p -> main_hand_weapon.buff_type = main_hand_poison;
    if (  off_hand_poison ) p ->  off_hand_weapon.buff_type =  off_hand_poison;
    if (    thrown_poison ) p ->    ranged_weapon.buff_type =    thrown_poison;
  };

  virtual bool ready()
  {
    rogue_t* p = cast();

    if ( p -> main_hand_weapon.type != WEAPON_NONE )
      if ( main_hand_poison )
        return( main_hand_poison != p -> main_hand_weapon.buff_type );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      if ( off_hand_poison )
        return( off_hand_poison != p -> off_hand_weapon.buff_type );

    if ( p -> ranged_weapon.type == WEAPON_THROWN )
      if ( thrown_poison )
        return ( thrown_poison != p -> ranged_weapon.buff_type );

    return false;
  }
};

// ==========================================================================
// Stealth
// ==========================================================================

struct stealth_t : public spell_t
{
  bool used;

  stealth_t( rogue_t* p, const std::string& options_str ) :
    spell_t( "stealth", p ), used( false )
  {
    trigger_gcd = timespan_t::zero();
    id = 1784;

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_t* p = cast();

    if ( sim -> log )
      sim -> output( "%s performs %s", p -> name(), name() );

    p -> buffs.stealthed -> trigger();
    used = true;
  }

  virtual bool ready()
  {
    return ! used;
  }

  virtual void reset()
  {
    spell_t::reset();
    used = false;
  }
};

// ==========================================================================
// Buffs
// ==========================================================================

struct adrenaline_rush_buff_t : public buff_t
{
  adrenaline_rush_buff_t( rogue_t* p, uint32_t id ) :
    buff_t( p, id, "adrenaline_rush" )
  {
    // we track the cooldown in the actual action
    // and because of restless blades have to remove it here
    cooldown -> duration = timespan_t::zero();
    buff_duration += p -> glyphs.adrenaline_rush -> mod_additive_time( P_DURATION );
    if ( p -> set_bonus.tier13_4pc_melee() )
      buff_duration += p -> spells.tier13_4pc -> effectN( 2 ).time_value();
  }

  virtual bool trigger( int, double, double, timespan_t )
  {
    // we keep haste % as current_value
    return buff_t::trigger( 1, base_value( E_APPLY_AURA, A_319 ) );
  }
};

struct envenom_buff_t : public buff_t
{
  envenom_buff_t( rogue_t* p ) :
    buff_t( p, 32645, "envenom" ) { }

  virtual bool trigger( int cp, double, double, timespan_t )
  {
    timespan_t new_duration = timespan_t::from_seconds( 1.0 ) + timespan_t::from_seconds( cp );

    if ( remains_lt( new_duration ) )
    {
      buff_duration = new_duration;
      return buff_t::trigger();
    }
    else
      return false;
  }
};

struct find_weakness_buff_t : public buff_t
{
  find_weakness_buff_t( rogue_t* p, uint32_t id ) :
    buff_t( p, id, "find_weakness" )
  {
    if ( ! p -> dbc.spell( id ) )
      return;

    // Duration is specified in the actual debuff (or is it a buff?) placed on the target
    buff_duration = p -> dbc.spell( 91021 ) -> duration();

    init_buff_shared();
  }

  virtual bool trigger( int, double, double, timespan_t )
  {
    return buff_t::trigger( 1, effectN( 1 ).percent() );
  }
};

struct fof_fod_buff_t : public buff_t
{
  fof_fod_buff_t( rogue_t* p, int fof_p3 ) :
    buff_t( p, "legendary_daggers", 1, timespan_t::from_seconds( 6.0 ), timespan_t::zero(), fof_p3 )
  {
    if ( ! fof_p3 )
      return;
  }

  virtual void expire()
  {
    buff_t::expire();

    rogue_t* p = cast();
    p -> buffs.fof_p3 -> expire();
  }
};

struct killing_spree_buff_t : public buff_t
{
  killing_spree_buff_t( rogue_t* p, uint32_t id ) :
    buff_t( p, id, "killing_spree" )
  {
    // we track the cooldown in the actual action
    // and because of restless blades have to remove it here
    cooldown -> duration = timespan_t::zero();
  }

  virtual bool trigger( int, double, double, timespan_t )
  {
    rogue_t* p = cast();

    double value = 0.20; // XXX
    value += p -> glyphs.killing_spree -> mod_additive( P_EFFECT_3 ) / 100.0;

    return buff_t::trigger( 1, value );
  }
};

struct master_of_subtlety_buff_t : public buff_t
{
  master_of_subtlety_buff_t( rogue_t* p, uint32_t id ) :
    buff_t( p, id, "master_of_subtlety" )
  {
    buff_duration = timespan_t::from_seconds( 6.0 );
  }

  virtual bool trigger( int, double, double, timespan_t )
  {
    return buff_t::trigger( 1, effectN( 1 ).percent() );
  }
};

struct revealing_strike_buff_t : public buff_t
{
  revealing_strike_buff_t( rogue_t* p, uint32_t id ) :
    buff_t( p, id, "revealing_strike" ) { }

  virtual bool trigger( int, double, double, timespan_t )
  {
    rogue_t* p = cast();

    double value = base_value( E_APPLY_AURA, A_DUMMY ) / 100.0;
    value += p -> glyphs.revealing_strike -> mod_additive( P_EFFECT_3 ) / 100.0;

    return buff_t::trigger( 1, value );
  }
};

struct shadowstep_buff_t : public buff_t
{
  shadowstep_buff_t( rogue_t* p, uint32_t id ) :
    buff_t( p, id, "shadowstep" ) { }

  virtual bool trigger( int, double, double, timespan_t )
  {
    return buff_t::trigger( 1, base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER ) );
  }
};

struct slice_and_dice_buff_t : public buff_t
{
  uint32_t id;

  slice_and_dice_buff_t( rogue_t* p ) :
    buff_t( p, 5171, "slice_and_dice" ), id( 5171 ) { }

  virtual bool trigger( int cp, double, double, timespan_t )
  {
    rogue_t* p = cast();

    timespan_t new_duration = p -> dbc.spell( id ) -> duration();
    new_duration += timespan_t::from_seconds( 3.0 ) * cp;
    new_duration += p -> glyphs.slice_and_dice -> mod_additive_time( P_DURATION );
    new_duration *= 1.0 + p -> talents.improved_slice_and_dice -> mod_additive( P_DURATION );

    if ( remains_lt( new_duration ) )
    {
      buff_duration = new_duration;
      return buff_t::trigger( 1,
        base_value( E_APPLY_AURA, A_319 ) * ( 1.0 + p -> composite_mastery() * p -> mastery_executioner -> effect_coeff( 1 ) / 100.0 ) );
    }
    else
      return false;
  }
};

struct vendetta_buff_t : public buff_t
{
  vendetta_buff_t( rogue_t* p, uint32_t id ) :
    buff_t( p, id, "vendetta" )
  {
    buff_duration *= 1.0 + p -> glyphs.vendetta -> mod_additive( P_DURATION );
    if ( p -> set_bonus.tier13_4pc_melee() )
      buff_duration += p -> spells.tier13_4pc -> effectN( 3 ).time_value();
  }

  virtual bool trigger( int, double, double, timespan_t )
  {
    return buff_t::trigger( 1, base_value( E_APPLY_AURA, A_MOD_DAMAGE_FROM_CASTER ) );
  }
};

// ==========================================================================
// Rogue Character Definition
// ==========================================================================

// rogue_t::composite_attribute_multiplier ==================================

double rogue_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_AGILITY )
    m *= 1.0 + spec.sinister_calling -> base_value( E_APPLY_AURA, A_MOD_TOTAL_STAT_PERCENTAGE );

  return m;
}

// rogue_t::composite_attack_speed ==========================================

double rogue_t::composite_attack_speed() const
{
  double h = player_t::composite_attack_speed();

  if ( talents.lightning_reflexes -> rank() )
    h *= 1.0 / ( 1.0 + talents.lightning_reflexes -> effectN( 2 ).percent() );

  if ( buffs.slice_and_dice -> check() )
    h *= 1.0 / ( 1.0 + buffs.slice_and_dice -> value() );

  if ( buffs.adrenaline_rush -> check() )
    h *= 1.0 / ( 1.0 + buffs.adrenaline_rush -> value() );

  return h;
}

// rogue_t::matching_gear_multiplier ========================================

double rogue_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_AGILITY )
    return 0.05;

  return 0.0;
}

// rogue_t::composite_attack_power_multiplier ===============================

double rogue_t::composite_attack_power_multiplier() const
{
  double m = player_t::composite_attack_power_multiplier();

  m *= 1.0 + talents.savage_combat -> base_value( E_APPLY_AURA, A_MOD_ATTACK_POWER_PCT );

  m *= 1.0 + spec.vitality -> base_value( E_APPLY_AURA, A_MOD_ATTACK_POWER_PCT );

  return m;
}

// rogue_t::composite_player_multiplier =====================================

double rogue_t::composite_player_multiplier( const school_e school, const action_t* a ) const
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( buffs.master_of_subtlety -> check() ||
       ( spec.master_of_subtlety -> ok() && ( buffs.stealthed -> check() || buffs.vanish -> check() ) ) )
    m *= 1.0 + buffs.master_of_subtlety -> value();

  return m;
}

// rogue_t::init_actions ====================================================

void rogue_t::init_actions()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    if ( level >= 80 )
    {
      // Flask
      action_list_str += "/flask,type=";
      action_list_str += ( level > 85 ) ? "spring_blossoms" : "winds";
      action_list_str += ",precombat=1";

      // Food
      action_list_str += "/food,type=";
      action_list_str += ( level > 85 ) ? "sea_mist_rice_noodles" : "seafood_magnifique_feast";
      action_list_str += ",precombat=1";
    }

    action_list_str += "/apply_poison,main_hand=instant,off_hand=deadly";
    action_list_str += "/snapshot_stats,precombat=1";

    // Prepotion
    action_list_str += ( level > 85 ) ? "/virmens_bite_potion" : "/tolvir_potion";
    action_list_str += ",precombat=1";

    // Potion use
    action_list_str += ( level > 85 ) ? "/virmens_bite_potion" : "/tolvir_potion";
    action_list_str += ",if=buff.bloodlust.react|target.time_to_die<=40";

    action_list_str += "/auto_attack";

    if ( talents.overkill -> rank() || spec.master_of_subtlety -> ok() )
      action_list_str += "/stealth";

    action_list_str += "/kick";

    if ( primary_tree() == TREE_ASSASSINATION )
    {
      action_list_str += init_use_item_actions();

      action_list_str += init_use_profession_actions();

      action_list_str += init_use_racial_actions();

      /* Putting this here for now but there is likely a better place to put it */
      action_list_str += "/tricks_of_the_trade,if=set_bonus.tier13_2pc_melee";

      action_list_str += "/garrote";
      action_list_str += "/slice_and_dice,if=buff.slice_and_dice.down";
      if ( ! talents.cut_to_the_chase -> rank() )
      {
        action_list_str += "/pool_energy,if=energy<60&buff.slice_and_dice.remains<5";
        action_list_str += "/slice_and_dice,if=combo_points>=3&buff.slice_and_dice.remains<2";
      }
      if ( talents.vendetta -> rank() )
      {
        action_list_str += "/rupture,if=(!ticking|ticks_remain<2)&time<6";
        action_list_str += "/vendetta";
      }
      action_list_str += "/rupture,if=(!ticking|ticks_remain<2)&buff.slice_and_dice.remains>6";
      if ( talents.cold_blood -> rank() )
        action_list_str += "/cold_blood,sync=envenom";
      action_list_str += "/envenom,if=combo_points>=4&buff.envenom.down";
      action_list_str += "/envenom,if=combo_points>=4&energy>90";
      action_list_str += "/envenom,if=combo_points>=2&buff.slice_and_dice.remains<3";
      action_list_str += "/backstab,if=combo_points<5&target.health_pct<35";
      action_list_str += "/mutilate,if=position_front&combo_points<5&target.health_pct<35";
      action_list_str += "/mutilate,if=combo_points<4&target.health_pct>=35";
      if ( talents.overkill -> rank() )
        action_list_str += "/vanish,if=time>30&energy>50";
    }
    else if ( primary_tree() == TREE_COMBAT )
    {
      action_list_str += init_use_item_actions();

      action_list_str += init_use_profession_actions();

      action_list_str += init_use_racial_actions();

      /* Putting this here for now but there is likely a better place to put it */
      action_list_str += "/tricks_of_the_trade,if=set_bonus.tier13_2pc_melee";

      // TODO: Add Blade Flurry
      action_list_str += "/slice_and_dice,if=buff.slice_and_dice.down";
      action_list_str += "/slice_and_dice,if=buff.slice_and_dice.remains<2";
      if ( talents.killing_spree -> rank() )
        action_list_str += "/killing_spree,if=energy<35&buff.slice_and_dice.remains>4&buff.adrenaline_rush.down";
      if ( talents.adrenaline_rush -> rank() )
        action_list_str += "/adrenaline_rush,if=energy<35";
      if ( talents.bandits_guile -> rank() )
        action_list_str += "/eviscerate,if=combo_points=5&buff.bandits_guile.stack>=12";
      action_list_str += "/rupture,if=!ticking&combo_points=5&target.time_to_die>10";
      action_list_str += "/eviscerate,if=combo_points=5";
      if ( talents.revealing_strike -> rank() )
        action_list_str += "/revealing_strike,if=combo_points=4&buff.revealing_strike.down";
      action_list_str += "/sinister_strike,if=combo_points<5";
    }
    else if ( primary_tree() == TREE_SUBTLETY )
    {
      /* Putting this here for now but there is likely a better place to put it */
      action_list_str += "/tricks_of_the_trade,if=set_bonus.tier13_2pc_melee";

      if ( talents.shadow_dance -> rank() )
      {
        action_list_str += "/pool_energy,for_next=1";
        action_list_str += "/shadow_dance,if=energy>85&combo_points<5&buff.stealthed.down";
      }

      int num_items = ( int ) items.size();
      int hand_enchant_found = -1;
      int found_item = -1;

      for ( int i=0; i < num_items; i++ )
      {
        if ( items[ i ].use.active() )
        {
          if ( items[ i ].slot == SLOT_HANDS )
          {
            hand_enchant_found = i;
            continue;
          }
          action_list_str += "/use_item,name=";
          action_list_str += items[ i ].name();
          if ( found_item < 0 )
          {
            action_list_str += ",if=buff.shadow_dance.up";
            found_item = i;
          }
          else
          {
            action_list_str += ",if=buff.shadow_dance.cooldown_remains>20";
          }
        }
      }
      if ( hand_enchant_found >= 0 )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ hand_enchant_found ].name();
        if ( found_item < 0 )
        {
          action_list_str += ",if=buff.shadow_dance.up";
        }
        else
        {
          action_list_str += ",if=buff.shadow_dance.cooldown_remains>20";
        }
      }

      action_list_str += init_use_profession_actions( ( found_item >= 0 ) ? "" : ",if=buff.shadow_dance.up|position_front" );

      action_list_str += init_use_racial_actions( ",if=buff.shadow_dance.up" );

      action_list_str += "/pool_energy,for_next=1";
      action_list_str += "/vanish,if=time>10&energy>60&combo_points<=1&cooldowns.shadowstep.remains<=0&!buff.shadow_dance.up&!buff.master_of_subtlety.up&!buff.find_weakness.up";

      action_list_str += "/shadowstep,if=buff.stealthed.up|buff.shadow_dance.up";
      if ( talents.premeditation -> rank() )
        action_list_str += "/premeditation,if=(combo_points<=3&cooldowns.honor_among_thieves.remains>1.75)|combo_points<=2";

      action_list_str += "/ambush,if=combo_points<=4";

      if ( talents.preparation -> rank() )
        action_list_str += "/preparation,if=cooldowns.vanish.remains>60";

      action_list_str += "/slice_and_dice,if=buff.slice_and_dice.remains<3&combo_points=5";

      action_list_str += "/rupture,if=combo_points=5&!ticking";

      if ( talents.energetic_recovery -> rank() )
        action_list_str += "/recuperate,if=combo_points=5&remains<3";

      action_list_str += "/eviscerate,if=combo_points=5&dot.rupture.remains>1";

      if ( talents.hemorrhage -> rank() )
      {
        action_list_str += "/hemorrhage,if=combo_points<4&(dot.hemorrhage.remains<4|position_front)";
        action_list_str += "/hemorrhage,if=combo_points<5&energy>80&(dot.hemorrhage.remains<4|position_front)";
      }

      action_list_str += "/backstab,if=combo_points<4";
      action_list_str += "/backstab,if=combo_points<5&energy>80";
    }
    else
    {
      action_list_str += init_use_item_actions();

      action_list_str += init_use_racial_actions();

      /* Putting this here for now but there is likely a better place to put it */

      action_list_str += "/pool_energy,if=energy<60&buff.slice_and_dice.remains<5";
      action_list_str += "/slice_and_dice,if=combo_points>=3&buff.slice_and_dice.remains<2";
      action_list_str += "/sinister_strike,if=combo_points<5";
    }

    action_list_default = 1;
  }

  player_t::init_actions();
}

// rogue_t::create_action  ==================================================

action_t* rogue_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  if ( name == "auto_attack"         ) return new auto_melee_attack_t        ( this, options_str );
  if ( name == "adrenaline_rush"     ) return new adrenaline_rush_t    ( this, options_str );
  if ( name == "ambush"              ) return new ambush_t             ( this, options_str );
  if ( name == "apply_poison"        ) return new apply_poison_t       ( this, options_str );
  if ( name == "backstab"            ) return new backstab_t           ( this, options_str );
  if ( name == "blade_flurry"        ) return new blade_flurry_t       ( this, options_str );
  if ( name == "cold_blood"          ) return new cold_blood_t         ( this, options_str );
  if ( name == "envenom"             ) return new envenom_t            ( this, options_str );
  if ( name == "eviscerate"          ) return new eviscerate_t         ( this, options_str );
  if ( name == "expose_armor"        ) return new expose_armor_t       ( this, options_str );
  if ( name == "feint"               ) return new feint_t              ( this, options_str );
  if ( name == "fan_of_knives"       ) return new fan_of_knives_t      ( this, options_str );
  if ( name == "garrote"             ) return new garrote_t            ( this, options_str );
  if ( name == "hemorrhage"          ) return new hemorrhage_t         ( this, options_str );
  if ( name == "kick"                ) return new kick_t               ( this, options_str );
  if ( name == "killing_spree"       ) return new killing_spree_t      ( this, options_str );
  if ( name == "mutilate"            ) return new mutilate_t           ( this, options_str );
  if ( name == "pool_energy"         ) return new pool_energy_t        ( this, options_str );
  if ( name == "premeditation"       ) return new premeditation_t      ( this, options_str );
  if ( name == "preparation"         ) return new preparation_t        ( this, options_str );
  if ( name == "recuperate"          ) return new recuperate_t         ( this, options_str );
  if ( name == "revealing_strike"    ) return new revealing_strike_t   ( this, options_str );
  if ( name == "rupture"             ) return new rupture_t            ( this, options_str );
  if ( name == "shadow_dance"        ) return new shadow_dance_t       ( this, options_str );
  if ( name == "shadowstep"          ) return new shadowstep_t         ( this, options_str );
  if ( name == "shiv"                ) return new shiv_t               ( this, options_str );
  if ( name == "sinister_strike"     ) return new sinister_strike_t    ( this, options_str );
  if ( name == "slice_and_dice"      ) return new slice_and_dice_t     ( this, options_str );
  if ( name == "stealth"             ) return new stealth_t            ( this, options_str );
  if ( name == "vanish"              ) return new vanish_t             ( this, options_str );
  if ( name == "vendetta"            ) return new vendetta_t           ( this, options_str );
  if ( name == "tricks_of_the_trade" ) return new tricks_of_the_trade_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// rogue_t::create_expression ===============================================

expr_t* rogue_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "combo_points" )
  {
    struct combo_points_expr_t : public expr_t
    {
      const action_t& action;
      combo_points_expr_t( action_t* a ) : expr_t( "combo_points" ),
        action( a ) {}
      virtual int evaluate()
      {
        rogue_targetdata_t* td = action -> cast_td();
        result_num = td -> combo_points -> count;
        return TOK_NUM;
      }
    };
    return new combo_points_expr_t( a );
  }

  return player_t::create_expression( a, name_str );
}

// rogue_t::init_base =======================================================

void rogue_t::init_base()
{
  player_t::init_base();

  base.attack_power = ( level * 2 ) - 20;
  initial.attack_power_per_strength = 1.0;
  initial.attack_power_per_agility  = 2.0;

  resources.base[ RESOURCE_ENERGY ] = 100 + spec.assassins_resolve -> base_value( E_APPLY_AURA, A_MOD_INCREASE_ENERGY );

  base_energy_regen_per_second = 10 + spec.vitality -> base_value( E_APPLY_AURA, A_MOD_POWER_REGEN_PERCENT ) / 10.0;

  base_gcd = timespan_t::from_seconds( 1.0 );

  diminished_kfactor    = 0.009880;
  diminished_dodge_capi = 0.006870;
  diminished_parry_capi = 0.006870;
}

// rogue_t::init_talents ====================================================

void rogue_t::init_talents()
{
  // Talents
#if 0
  // Assasination
  talents.cold_blood                 = find_talent( "Cold Blood" );
  talents.coup_de_grace              = find_talent( "Coup de Grace" );
  talents.cut_to_the_chase           = find_talent( "Cut to the Chase" );
  talents.improved_expose_armor      = find_talent( "Improved Expose Armor" );
  talents.lethality                  = find_talent( "Lethality" );
  talents.master_poisoner            = find_talent( "Master Poisoner" );
  talents.murderous_intent           = find_talent( "Murderous Intent" );
  talents.overkill                   = find_talent( "Overkill" );
  talents.puncturing_wounds          = find_talent( "Puncturing Wounds" );
  talents.quickening                 = find_talent( "Quickening" );
  talents.ruthlessness               = find_talent( "Ruthlessness" );
  talents.seal_fate                  = find_talent( "Seal Fate" );
  talents.vendetta                   = find_talent( "Vendetta" );
  talents.venomous_wounds            = find_talent( "Venomous Wounds" );
  talents.vile_poisons               = find_talent( "Vile Poisons" );

  // Combat
  talents.adrenaline_rush            = find_talent( "Adrenaline Rush" );
  talents.aggression                 = find_talent( "Aggression" );
  talents.bandits_guile              = find_talent( "Bandit's Guile" );
  talents.combat_potency             = find_talent( "Combat Potency" );
  talents.improved_sinister_strike   = find_talent( "Improved Sinister Strike" );
  talents.improved_slice_and_dice    = find_talent( "Improved Slice and Dice" );
  talents.killing_spree              = find_talent( "Killing Spree" );
  talents.lightning_reflexes         = find_talent( "Lightning Reflexes" );
  talents.precision                  = find_talent( "Precision" );
  talents.restless_blades            = find_talent( "Restless Blades" );
  talents.revealing_strike           = find_talent( "Revealing Strike" );
  talents.savage_combat              = find_talent( "Savage Combat" );

  // Subtlety
  talents.elusiveness                = find_talent( "Elusiveness" );
  talents.energetic_recovery         = find_talent( "Energetic Recovery" );
  talents.find_weakness              = find_talent( "Find Weakness" );
  talents.hemorrhage                 = find_talent( "Hemorrhage" );
  talents.honor_among_thieves        = find_talent( "Honor Among Thieves" );
  talents.improved_ambush            = find_talent( "Improved Ambush" );
  talents.initiative                 = find_talent( "Initiative" );
  talents.opportunity                = find_talent( "Opportunity" );
  talents.premeditation              = find_talent( "Premeditation" );
  talents.preparation                = find_talent( "Preparation" );
  talents.relentless_strikes         = find_talent( "Relentless Strikes" );
  talents.sanguinary_vein            = find_talent( "Sanguinary Vein" );
  talents.serrated_blades            = find_talent( "Serrated Blades" );
  talents.shadow_dance               = find_talent( "Shadow Dance" );
  talents.slaughter_from_the_shadows = find_talent( "Slaughter from the Shadows" );
  talents.waylay                     = find_talent( "Waylay" );
#endif
  player_t::init_talents();
}

// rogue_t::init_spells =====================================================

void rogue_t::init_spells()
{
  player_t::init_spells();

  // Spec passives

  // Spec Actives

  // Masteries
  masterys.potent_poisons  = find_mastery_spell( ROGUE_ASSASSINATION );
  masterys.main_gauche     = find_mastery_spell( ROGUE_COMBAT );
  masterys.executioner     = find_mastery_spell( ROGUE_SUBTLETY );

  spells.tier13_2pc              = find_spell( 105864 );
  spells.tier13_4pc              = find_spell( 105865 );

  spec.bandits_guile       = find_specialization_spell( "Bandit's Guile" );
  spec.bandits_guile_value = spec.bandits_guile -> ok() ? find_spell( 84747 ) : spell_data_t::not_found();
  spec.master_of_subtlety  = find_specialization_spell( "Master of Subtlety" );

  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P    M2P    M4P    T2P    T4P    H2P    H4P
    {     0,     0, 105849, 105865,     0,     0,     0,     0 }, // Tier13
    {     0,     0,      0,      0,     0,     0,     0,     0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// rogue_t::init_gains ======================================================

void rogue_t::init_gains()
{
  player_t::init_gains();

  gains.adrenaline_rush    = get_gain( "adrenaline_rush"    );
  gains.combat_potency     = get_gain( "combat_potency"     );
  gains.energy_refund      = get_gain( "energy_refund"      );
  gains.murderous_intent   = get_gain( "murderous_intent"   );
  gains.overkill           = get_gain( "overkill"           );
  gains.recuperate         = get_gain( "recuperate"         );
  gains.relentless_strikes = get_gain( "relentless_strikes" );
  gains.venomous_vim       = get_gain( "venomous_vim"       );
}

// rogue_t::init_procs ======================================================

void rogue_t::init_procs()
{
  player_t::init_procs();

  procs.deadly_poison            = get_proc( "deadly_poisons"        );
  procs.honor_among_thieves      = get_proc( "honor_among_thieves"   );
  procs.main_gauche              = get_proc( "main_gauche"           );
  procs.ruthlessness             = get_proc( "ruthlessness"          );
  procs.seal_fate                = get_proc( "seal_fate"             );
  procs.serrated_blades          = get_proc( "serrated_blades"       );
  procs.sinister_strike_glyph    = get_proc( "sinister_strike_glyph" );
  procs.venomous_wounds          = get_proc( "venomous_wounds"       );
}

// rogue_t::init_uptimes ====================================================

void rogue_t::init_benefits()
{
  player_t::init_benefits();

  benefits.bandits_guile[ 0 ] = get_benefit( "shallow_insight" );
  benefits.bandits_guile[ 1 ] = get_benefit( "moderate_insight" );
  benefits.bandits_guile[ 2 ] = get_benefit( "deep_insight" );

  benefits.energy_cap = get_benefit( "energy_cap" );
  benefits.poisoned   = get_benefit( "poisoned"   );
}

// rogue_t::init_rng ========================================================

void rogue_t::init_rng()
{
  player_t::init_rng();

  rngs.bandits_guile         = get_rng( "bandits_guile"         );
  rngs.combat_potency        = get_rng( "combat_potency"        );
  rngs.cut_to_the_chase      = get_rng( "cut_to_the_chase"      );
  rngs.deadly_poison         = get_rng( "deadly_poison"         );
  rngs.honor_among_thieves   = get_rng( "honor_among_thieves"   );
  rngs.improved_expose_armor = get_rng( "improved_expose_armor" );
  rngs.initiative            = get_rng( "initiative"            );
  rngs.instant_poison        = get_rng( "instant_poison"        );
  rngs.main_gauche           = get_rng( "main_gauche"           );
  rngs.relentless_strikes    = get_rng( "relentless_strikes"    );
  rngs.ruthlessness          = get_rng( "ruthlessness"          );
  rngs.seal_fate             = get_rng( "seal_fate"             );
  rngs.serrated_blades       = get_rng( "serrated_blades"       );
  rngs.sinister_strike_glyph = get_rng( "sinister_strike_glyph" );
  rngs.venomous_wounds       = get_rng( "venomous_wounds"       );
  rngs.vile_poisons          = get_rng( "vile_poisons"          );
  rngs.wound_poison          = get_rng( "wound_poison"          );
  rngs.hat_interval          = get_rng( "hat_interval" );
}

// rogue_t::init_scaling ====================================================

void rogue_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = true;
  scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = sim -> weapon_speed_scale_factors != 0;
  scales_with[ STAT_HIT_RATING2           ] = true;
}

// rogue_t::init_buffs ======================================================

void rogue_t::init_buffs()
{
  // Handle the Legendary here, as it's called after init_items()
  if ( find_item( "vengeance" ) && find_item( "fear" ) )
  {
    fof_p1 = 1;
  }
  else if ( find_item( "the_sleeper" ) && find_item( "the_dreamer" ) )
  {
    fof_p2 = 1;
  }
  else if ( find_item( "golad_twilight_of_aspects" ) && find_item( "tiriosh_nightmare_of_ages" ) )
  {
    fof_p3 = 1;
  }

  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  buff.bandits_guile       = buff_creator_t( this, "bandits_guile", spec.bandits_guile_value )
                             .duration( spec.bandits_guile_value -> duration() )
                             .max_stack( 12 )
                             .chance( spec.bandits_guile_value -> ok() );

  buff.master_of_subtlety  = buff_creator_t( this, "master_of_subtlety", spec.master_of_subtlety )
                             .duration( timespan_t::from_seconds( 6.0 ) )
                             .default_value( spec.master_of_subtlety -> effectN( 1 ).percent() )
                             .chance( spec.master_of_subtlety -> ok() )



  buffs.bandits_guile      = new buff_t( this, "bandits_guile", 12, timespan_t::from_seconds( 15.0 ), timespan_t::zero(), 1.0, true );
  buffs.deadly_proc        = new buff_t( this, "deadly_proc",   1  );
  buffs.overkill           = new buff_t( this, "overkill",      1, timespan_t::from_seconds( 20.0 ) );
  buffs.recuperate         = new buff_t( this, "recuperate",    1  );
  buffs.shiv               = new buff_t( this, "shiv",          1  );
  buffs.stealthed          = new buff_t( this, "stealthed",     1  );
  buffs.tier13_2pc         = new buff_t( this, "tier13_2pc",    1, spells.tier13_2pc -> duration(), timespan_t::zero(), ( set_bonus.tier13_2pc_melee() ) ? 1.0 : 0 );
  buffs.tot_trigger        = new buff_t( this, 57934, "tricks_of_the_trade_trigger", -1, timespan_t::min(), true );
  buffs.vanish             = new buff_t( this, "vanish",        1, timespan_t::from_seconds( 3.0 ) );

  buffs.blade_flurry       = new buff_t( this, spec.blade_flurry -> spell_id(), "blade_flurry" );

  buffs.envenom            = new envenom_buff_t            ( this );
  buffs.slice_and_dice     = new slice_and_dice_buff_t     ( this );

  // Legendary buffs
  // buffs.fof_p1            = new stat_buff_t( this, 109959, "legendary_daggers_p1", STAT_AGILITY, dbc.spell( 109959 ) -> effectN( 1 ).base_value(), fof_p1 );
  // buffs.fof_p2            = new stat_buff_t( this, 109955, "legendary_daggers_p2", STAT_AGILITY, dbc.spell( 109955 ) -> effectN( 1 ).base_value(), fof_p2 );
  // buffs.fof_p3            = new stat_buff_t( this, 109939, "legendary_daggers_p3", STAT_AGILITY, dbc.spell( 109939 ) -> effectN( 1 ).base_value(), fof_p3 );
  // buffs.fof_fod           = new buff_t( this, 109949, "legendary_daggers", fof_p3 );
  // None of the buffs are currently in the DBC, so define them manually for now
  buffs.fof_p1            = new stat_buff_t( this, "legendary_daggers_p1", STAT_AGILITY,  2.0, 50, timespan_t::from_seconds( 30.0 ), timespan_t::zero(), fof_p1 );
  buffs.fof_p2            = new stat_buff_t( this, "legendary_daggers_p2", STAT_AGILITY,  5.0, 50, timespan_t::from_seconds( 30.0 ), timespan_t::zero(), fof_p2 );
  buffs.fof_p3            = new stat_buff_t( this, "legendary_daggers_p3", STAT_AGILITY, 17.0, 50, timespan_t::from_seconds( 30.0 ), timespan_t::zero(), fof_p3 );
  buffs.fof_fod           = new fof_fod_buff_t( this, fof_p3 );
}

// rogue_t::init_values =====================================================

void rogue_t::init_values()
{
  player_t::init_values();

  if ( set_bonus.pvp_2pc_melee() )
    stats_initial.attribute[ ATTR_AGILITY ]   += 70;

  if ( set_bonus.pvp_4pc_melee() )
    stats_initial.attribute[ ATTR_AGILITY ]   += 90;
}

// trigger_honor_among_thieves ==============================================

struct honor_among_thieves_callback_t : public action_callback_t
{
  honor_among_thieves_callback_t( rogue_t* r ) : action_callback_t( r ) {}

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    rogue_t* rogue = listener -> cast_rogue();

    if ( a )
    {
      // only procs from specials; repeating is here for hunter autoshot, which doesn't proc it either
      if ( ! a -> special || a -> repeating )
        return;

      // doesn't proc from pets (only tested for hunter pets though)
      if ( a -> player -> is_pet() )
        return;

      a -> player -> procs.hat_donor -> occur();
    }

//    if ( sim -> debug )
//      sim -> output( "Eligible For Honor Among Thieves" );

    if ( rogue -> cooldowns_honor_among_thieves -> remains() > timespan_t::zero() )
      return;

    if ( ! rogue -> rng_honor_among_thieves -> roll( rogue -> talents.honor_among_thieves -> proc_chance() ) )
      return;

    rogue_targetdata_t* td = targetdata_t::get( rogue, rogue->target ) -> cast_rogue();

    td -> combo_points -> add( 1, rogue -> talents.honor_among_thieves );

    rogue -> procs.honor_among_thieves -> occur();

    rogue -> cooldowns_honor_among_thieves -> start( timespan_t::from_seconds( 5.0 ) - timespan_t::from_seconds( rogue -> talents.honor_among_thieves -> rank() ) );
  }
};

// rogue_t::register_callbacks ==============================================

void rogue_t::register_callbacks()
{
  player_t::register_callbacks();

  if ( talents.honor_among_thieves -> rank() )
  {
    action_callback_t* cb = new honor_among_thieves_callback_t( this );

    register_attack_callback( RESULT_CRIT_MASK, cb );
    register_spell_callback ( RESULT_CRIT_MASK, cb );
    register_tick_callback( RESULT_CRIT_MASK, cb );

    if ( virtual_hat_interval < timespan_t::zero() )
    {
      virtual_hat_interval = timespan_t::from_seconds( 5.20 ) - timespan_t::from_seconds( talents.honor_among_thieves -> rank() );
    }
    if ( virtual_hat_interval > timespan_t::zero() )
    {
      virtual_hat_callback = cb;
    }
    else
    {
      for ( player_t* p = sim -> player_list; p; p = p -> next )
      {
        if ( p == this     ) continue;
        if ( p -> is_pet() ) continue;

        p -> register_attack_callback( RESULT_CRIT_MASK, cb );
        p -> register_spell_callback ( RESULT_CRIT_MASK, cb );
        p -> register_tick_callback( RESULT_CRIT_MASK, cb );
      }
    }
  }
}

// rogue_t::combat_begin ====================================================

void rogue_t::combat_begin()
{
  player_t::combat_begin();

  if ( talents.honor_among_thieves -> rank() )
  {
    if ( virtual_hat_interval > timespan_t::zero() )
    {
      struct virtual_hat_event_t : public event_t
      {
        action_callback_t* callback;
        timespan_t interval;

        virtual_hat_event_t( sim_t* sim, rogue_t* p, action_callback_t* cb, timespan_t i ) :
          event_t( sim, p ), callback( cb ), interval( i )
        {
          name = "Virtual HAT Event";
          timespan_t cooldown = timespan_t::from_seconds( 5.0 ) - timespan_t::from_seconds( p -> talents.honor_among_thieves -> rank() );
          timespan_t remainder = interval - cooldown;
          if ( remainder < timespan_t::zero() ) remainder = timespan_t::zero();
          timespan_t time = cooldown + p -> rng_hat_interval -> range( remainder*0.5, remainder*1.5 ) + timespan_t::from_seconds( 0.01 );
          sim -> add_event( this, time );
        }
        virtual void execute()
        {
          rogue_t* p = cast();
          callback -> trigger( NULL );
          new ( sim ) virtual_hat_event_t( sim, p, callback, interval );
        }
      };
      new ( sim ) virtual_hat_event_t( sim, this, virtual_hat_callback, virtual_hat_interval );
    }
  }
}

// rogue_t::reset ===========================================================

void rogue_t::reset()
{
  player_t::reset();

  expirations_.reset();
}

// rogue_t::arise ===========================================================

void rogue_t::arise()
{
  player_t::arise();

  if ( ! sim -> overrides.attack_haste && dbc.spell( 113742 ) -> is_level( level ) ) sim -> auras.attack_haste -> trigger();
}

// rogue_t::energy_regen_per_second =========================================

double rogue_t::energy_regen_per_second() const
{
  double r = player_t::energy_regen_per_second();

  if ( buffs.blade_flurry -> up() )
    r *= glyphs.blade_flurry -> ok() ? 0.90 : 0.80;

  return r;
}

// rogue_t::regen ===========================================================

void rogue_t::regen( timespan_t periodicity )
{
  // XXX review how this all stacks (additive/multiplicative)

  player_t::regen( periodicity );

  if ( buffs.adrenaline_rush -> up() )
  {
    if ( ! resources.is_infinite( RESOURCE_ENERGY ) )
    {
      double energy_regen = periodicity.total_seconds() * energy_regen_per_second();

      resource_gain( RESOURCE_ENERGY, energy_regen, gains.adrenaline_rush );
    }
  }

  if ( talents.overkill -> rank() && buffs.overkill -> up() )
  {
    if ( ! resources.is_infinite( RESOURCE_ENERGY ) )
    {
      double energy_regen = periodicity.total_seconds() * energy_regen_per_second() * 0.30;

      resource_gain( RESOURCE_ENERGY, energy_regen, gains.overkill );
    }
  }

  benefits.energy_cap -> update( resources.current[ RESOURCE_ENERGY ] ==
                                 resources.max    [ RESOURCE_ENERGY ] );

  for ( int i = 0; i < 3; i++ )
    benefits.bandits_guile[ i ] -> update( ( buffs.bandits_guile -> current_stack / 4 - 1 ) == i );
}

// rogue_t::available =======================================================

timespan_t rogue_t::available() const
{
  double energy = resources.current[ RESOURCE_ENERGY ];

  if ( energy > 25 )
    return timespan_t::from_seconds( 0.1 );

  return std::max(
    timespan_t::from_seconds( ( 25 - energy ) / energy_regen_per_second() ),
    timespan_t::from_seconds( 0.1 )
  );
}

// rogue_t::copy_options ====================================================

void rogue_t::create_options()
{
  player_t::create_options();

  option_t rogue_options[] =
  {
    { "virtual_hat_interval",       OPT_TIMESPAN, &( virtual_hat_interval         ) },
    { "tricks_of_the_trade_target", OPT_STRING, &( tricks_of_the_trade_target_str ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, rogue_options );
}

// rogue_t::create_profile ==================================================

bool rogue_t::create_profile( std::string& profile_str, save_e stype, bool save_html ) const
{
  player_t::create_profile( profile_str, stype, save_html );

  if ( stype == SAVE_ALL || stype == SAVE_ACTIONS )
  {
    if ( spec.honor_among_thieves -> ok() )
    {
      profile_str += "# These values represent the avg HAT donor interval of the raid.\n";
      profile_str += "# A negative value will make the Rogue use a programmed default interval.\n";
      profile_str += "# A zero value will disable virtual HAT procs and assume a real raid is being simulated.\n";
      profile_str += "virtual_hat_interval=-1\n";  // Force it to generate profiles using programmed default.
    }
  }

  return true;
}

// rogue_t::copy_from =======================================================

void rogue_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  rogue_t* p = dynamic_cast<rogue_t*>( source );
  assert( p );

  virtual_hat_interval = p -> virtual_hat_interval;
}

// rogue_t::decode_set ======================================================

int rogue_t::decode_set( const item_t& item ) const
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

  if ( strstr( s, "blackfang_battleweave" ) ) return SET_T13_MELEE;

  if ( strstr( s, "_gladiators_leather_" ) )  return SET_PVP_CASTER;

  return SET_NONE;
}

#endif // SC_ROGUE

// ROGUE MODULE INTERFACE ================================================

struct rogue_module_t : public module_t
{
  rogue_module_t() : module_t( ROGUE ) {}

  virtual player_t* create_player( sim_t* /*sim*/, const std::string& /*name*/, race_e /*r = RACE_NONE*/ )
  {
    return NULL; // new rogue_t( sim, name, r );
  }
  virtual bool valid() { return false; }
  virtual void init( sim_t* sim )
  {
    sim -> auras.honor_among_thieves = buff_creator_t( sim, "honor_among_thieves" );

    for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[i];
      p -> buffs.tricks_of_the_trade  = buff_creator_t( p, "tricks_of_the_trade" ).max_stack( 1 ).duration( timespan_t::from_seconds( 6.0 ) );
    }
  }
  virtual void combat_begin( sim_t* sim )
  {
    if ( sim -> overrides.honor_among_thieves )
    {
      sim -> auras.honor_among_thieves -> override();
    }
  }
  virtual void combat_end( sim_t* ) {}
};

} // UNNAMED NAMESPACE

module_t* module_t::rogue()
{
  static module_t* m = 0;
  if ( ! m ) m = new rogue_module_t();
  return m;
}
