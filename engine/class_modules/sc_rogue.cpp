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


struct combo_points_t
{
  static const int max_combo_points = 5;

  rogue_t* source;
  player_t* target;

  proc_t* proc;
  proc_t* wasted;
  proc_t* proc_anticipation;
  proc_t* wasted_anticipation;

  int count;
  int anticipation_charges;

  combo_points_t( rogue_t* source, player_t* target );

  void add( int num, const char* action = 0 );
  void clear( const char* action = 0, bool anticipation = false );
};

// ==========================================================================
// Rogue
// ==========================================================================

// ==========================================================================
//  New Ability: Redirect, doable once we allow debuffs on multiple targets
//  Review: Ability Scaling
//  Review: Energy Regen (how Adrenaline rush stacks with Blade Flurry / haste)
// ==========================================================================

struct rogue_td_t : public actor_pair_t
{
  struct dots_t
  {
    dot_t* crimson_tempest;
    dot_t* deadly_poison;
    dot_t* garrote;
    dot_t* hemorrhage;
    dot_t* killing_spree; // Strictly speaking, this should probably be on player
    dot_t* rupture;
    dot_t* revealing_strike;
  } dots;

  struct debuffs_t
  {
    buff_t* find_weakness;
    buff_t* vendetta;
    buff_t* wound_poison;
  } debuffs;

  combo_points_t combo_points;

  rogue_td_t( player_t* target, rogue_t* source );

  void reset()
  {
    combo_points.clear( 0, true );
  }

  bool sanguinary_veins();
};

struct rogue_t : public player_t
{
  // Premeditation
  core_event_t* event_premeditation;

  // Active
  attack_t* active_blade_flurry;
  action_t* active_lethal_poison;
  action_t* active_main_gauche;
  action_t* active_venomous_wound;

  // Autoattacks
  attack_t* melee_main_hand;
  attack_t* melee_off_hand;
  attack_t* shadow_blade_main_hand;
  attack_t* shadow_blade_off_hand;

  // Buffs
  struct buffs_t
  {
    buff_t* adrenaline_rush;
    buff_t* bandits_guile;
    buff_t* blade_flurry;
    buff_t* blindside;
    buff_t* deadly_poison;
    buff_t* deadly_proc;
    buff_t* deep_insight;
    buff_t* killing_spree;
    buff_t* master_of_subtlety;
    buff_t* moderate_insight;
    buff_t* recuperate;
    buff_t* shadow_blades;
    buff_t* shadow_dance;
    buff_t* shallow_insight;
    buff_t* shiv;
    buff_t* sleight_of_hand;
    buff_t* stealth;
    buff_t* subterfuge;
    buff_t* t16_2pc_melee;
    buff_t* tier13_2pc;
    buff_t* tot_trigger;
    buff_t* vanish;
    buff_t* wound_poison;

    buff_t* envenom;
    buff_t* slice_and_dice;

    // Legendary buffs
    buff_t* fof_fod; // Fangs of the Destroyer
    stat_buff_t* fof_p1; // Phase 1
    stat_buff_t* fof_p2;
    stat_buff_t* fof_p3;
    stat_buff_t* toxicologist;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* adrenaline_rush;
    cooldown_t* honor_among_thieves;
    cooldown_t* killing_spree;
    cooldown_t* seal_fate;
    cooldown_t* shadow_blades;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* adrenaline_rush;
    gain_t* combat_potency;
    gain_t* energy_refund;
    gain_t* energetic_recovery;
    gain_t* murderous_intent;
    gain_t* overkill;
    gain_t* recuperate;
    gain_t* relentless_strikes;
    gain_t* vitality;
    gain_t* venomous_wounds;
  } gains;

  // Spec passives
  struct spec_t
  {
    // Assassination
    const spell_data_t* assassins_resolve;
    const spell_data_t* improved_poisons;
    const spell_data_t* seal_fate;
    const spell_data_t* venomous_wounds;
    const spell_data_t* cut_to_the_chase;
    const spell_data_t* blindside;

    // Combat
    const spell_data_t* ambidexterity;
    const spell_data_t* blade_flurry;
    const spell_data_t* vitality;
    const spell_data_t* combat_potency;
    const spell_data_t* restless_blades;
    const spell_data_t* bandits_guile;
    const spell_data_t* killing_spree;
    const spell_data_t* ruthlessness;

    // Subtlety
    const spell_data_t* master_of_subtlety;
    const spell_data_t* sinister_calling;
    const spell_data_t* find_weakness;
    const spell_data_t* honor_among_thieves;
    const spell_data_t* sanguinary_vein;
    const spell_data_t* energetic_recovery;
    const spell_data_t* shadow_dance;
  } spec;

  // Spell Data
  struct spells_t
  {
    const spell_data_t* bandits_guile_value;
    const spell_data_t* relentless_strikes;
    const spell_data_t* ruthlessness_cp;
    const spell_data_t* shadow_focus;
    const spell_data_t* tier13_2pc;
    const spell_data_t* tier13_4pc;
    const spell_data_t* tier15_4pc;
  } spell;

  // Talents
  struct talents_t
  {
    const spell_data_t* nightstalker;
    const spell_data_t* subterfuge;
    const spell_data_t* shadow_focus;

    const spell_data_t* anticipation;
    const spell_data_t* marked_for_death;
  } talent;

  // Masteries
  struct masteries_t
  {
    const spell_data_t* executioner;
    const spell_data_t* main_gauche;
    const spell_data_t* potent_poisons;
  } mastery;

  struct glyphs_t
  {
    const spell_data_t* adrenaline_rush;
    const spell_data_t* expose_armor;
    const spell_data_t* hemorrhaging_veins;
    const spell_data_t* kick;
    const spell_data_t* sharp_knives;
    const spell_data_t* vanish;
    const spell_data_t* vendetta;
  } glyph;

  // Procs
  struct procs_t
  {
    proc_t* honor_among_thieves;
    proc_t* seal_fate;
    proc_t* no_revealing_strike;
    proc_t* t16_2pc_melee;
  } procs;

  player_t* tot_target;
  action_callback_t* virtual_hat_callback;

  // Options
  std::string tricks_of_the_trade_target_str;
  timespan_t virtual_hat_interval;
  uint32_t fof_p1, fof_p2, fof_p3;

  rogue_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, ROGUE, name, r ),
    event_premeditation( 0 ),
    active_blade_flurry( 0 ),
    active_lethal_poison( 0 ),
    active_main_gauche( 0 ),
    active_venomous_wound( 0 ),
    melee_main_hand( 0 ), melee_off_hand( 0 ),
    shadow_blade_main_hand( 0 ), shadow_blade_off_hand( 0 ),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    spec( spec_t() ),
    spell( spells_t() ),
    talent( talents_t() ),
    mastery( masteries_t() ),
    glyph( glyphs_t() ),
    procs( procs_t() ),
    tot_target( 0 ),
    virtual_hat_callback( 0 ),
    tricks_of_the_trade_target_str( "" ),
    virtual_hat_interval( timespan_t::min() ),
    fof_p1( 0 ), fof_p2( 0 ), fof_p3( 0 )
  {
    // Cooldowns
    cooldowns.honor_among_thieves = get_cooldown( "honor_among_thieves" );
    cooldowns.seal_fate           = get_cooldown( "seal_fate"           );
    cooldowns.adrenaline_rush     = get_cooldown( "adrenaline_rush"     );
    cooldowns.killing_spree       = get_cooldown( "killing_spree"       );
    cooldowns.shadow_blades       = get_cooldown( "shadow_blades"       );

    base.distance = 3;
  }

  // Character Definition
  virtual void      init_spells();
  virtual void      init_base_stats();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_scaling();
  virtual void      create_buffs();
  virtual void      init_action_list();
  virtual void      register_callbacks();
  virtual void      combat_begin();
  virtual void      reset();
  virtual void      arise();
  virtual void      regen( timespan_t periodicity );
  virtual timespan_t available() const;
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual expr_t*   create_expression( action_t* a, const std::string& name_str );
  virtual set_e       decode_set( const item_t& ) const;
  virtual resource_e primary_resource() const { return RESOURCE_ENERGY; }
  virtual role_e    primary_role() const  { return ROLE_ATTACK; }
  virtual stat_e    convert_hybrid_stat( stat_e s ) const;
  virtual bool      create_profile( std::string& profile_str, save_e = SAVE_ALL, bool save_html = false );
  virtual void      copy_from( player_t* source );

  virtual double    composite_attribute_multiplier( attribute_e attr ) const;
  virtual double    composite_melee_speed() const;
  virtual double    matching_gear_multiplier( attribute_e attr ) const;
  virtual double    composite_attack_power_multiplier() const;
  virtual double    composite_player_multiplier( school_e school ) const;
  virtual double    energy_regen_per_second() const;

  target_specific_t<rogue_td_t*> target_data;

  virtual rogue_td_t* get_target_data( player_t* target ) const
  {
    rogue_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new rogue_td_t( target, const_cast<rogue_t*>(this) );
    }
    return td;
  }
};

inline bool rogue_td_t::sanguinary_veins()
{
  rogue_t* r = debug_cast<rogue_t*>( source );

  return dots.garrote -> is_ticking() ||
         dots.rupture -> is_ticking() ||
         dots.crimson_tempest -> is_ticking() ||
         ( r -> glyph.hemorrhaging_veins -> ok() && dots.hemorrhage -> is_ticking() );
}

namespace actions { // namespace actions

// ==========================================================================
// Rogue Attack
// ==========================================================================

struct rogue_attack_state_t : public action_state_t
{
  int cp;

  rogue_attack_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ), cp( 0 )
  { }

  void initialize()
  { action_state_t::initialize(); cp = 0; }

  std::ostringstream& debug_str( std::ostringstream& s )
  { action_state_t::debug_str( s ) << " cp=" << cp; return s; }

  void copy_state( const action_state_t* o )
  {
    action_state_t::copy_state( o );
    const rogue_attack_state_t* st = debug_cast<const rogue_attack_state_t*>( o );
    cp = st -> cp;
  }
};


struct rogue_attack_t : public melee_attack_t
{
  bool        requires_stealth;
  position_e  requires_position;
  bool        requires_combo_points;
  int         adds_combo_points;
  weapon_e    requires_weapon;

  // we now track how much combo points we spent on an action
  int              combo_points_spent;

  rogue_attack_t( const std::string& token, rogue_t* p,
                  const spell_data_t* s = spell_data_t::nil(),
                  const std::string& options = std::string() ) :
    melee_attack_t( token, p, s ),
    requires_stealth( false ), requires_position( POSITION_NONE ),
    requires_combo_points( false ), adds_combo_points( 0 ),
    requires_weapon( WEAPON_NONE ),
    combo_points_spent( 0 )
  {
    parse_options( 0, options );

    may_crit                  = true;
    may_glance                = false;
    special                   = true;
    tick_may_crit             = true;
    hasted_ticks              = false;

    for ( size_t i = 1; i <= s -> effect_count(); i++ )
    {
      const spelleffect_data_t& effect = s -> effectN( i );

      switch ( effect.type() )
      {
        case E_ADD_COMBO_POINTS:
          adds_combo_points = effect.base_value();
          break;
        default:
          break;
      }

      if ( effect.type() == E_APPLY_AURA && effect.subtype() == A_PERIODIC_DAMAGE )
        base_ta_adder = effect.bonus( player );
      else if ( effect.type() == E_SCHOOL_DAMAGE )
        base_dd_adder = effect.bonus( player );
    }
  }

  virtual void snapshot_state( action_state_t* state, dmg_e rt )
  {
    melee_attack_t::snapshot_state( state, rt );
    // FIXME: Combo points _NEED_ to move to player .. soon.
    // In the meantime, snapshot combo points always off the primary target
    // and pray that nobody makes a rogue ability that has to juggle primary
    // targets around during it's execution. Gulp.
    cast_state( state ) -> cp = td( target ) -> combo_points.count;
  }

  bool stealthed()
  {
    return p() -> buffs.vanish -> check() || p() -> buffs.stealth -> check() || player -> buffs.shadowmeld -> check();
  }

  action_state_t* new_state()
  { return new rogue_attack_state_t( this, target ); }

  static const rogue_attack_state_t* cast_state( const action_state_t* st )
  { return debug_cast< const rogue_attack_state_t* >( st ); }

  static rogue_attack_state_t* cast_state( action_state_t* st )
  { return debug_cast< rogue_attack_state_t* >( st ); }

  rogue_t* cast()
  { return p(); }
  const rogue_t* cast() const
  { return p(); }

  rogue_t* p()
  { return debug_cast< rogue_t* >( player ); }
  const rogue_t* p() const
  { return debug_cast< rogue_t* >( player ); }

  rogue_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }


  virtual double cost() const;
  virtual void   execute();
  virtual void   consume_resource();
  virtual bool   ready();
  virtual void   impact( action_state_t* state );

  virtual double calculate_weapon_damage( double attack_power );
  virtual double target_armor( player_t* ) const;

  virtual double attack_direct_power_coefficient( const action_state_t* s ) const
  {
    if ( requires_combo_points )
      return attack_power_mod.direct * cast_state( s ) -> cp;
    return melee_attack_t::attack_direct_power_coefficient( s );
  }

  virtual double attack_tick_power_coefficient( const action_state_t* s ) const
  {
    if ( requires_combo_points )
      return attack_power_mod.tick * cast_state( s ) -> cp;
    return melee_attack_t::attack_tick_power_coefficient( s );
  }

  virtual double spell_direct_power_coefficient( const action_state_t* s ) const
  {
    if ( requires_combo_points )
      return spell_power_mod.direct * cast_state( s ) -> cp;
    return melee_attack_t::spell_direct_power_coefficient( s );
  }

  virtual double spell_tick_power_coefficient( const action_state_t* s ) const
  {
    if ( requires_combo_points )
      return spell_power_mod.tick * cast_state( s ) -> cp;
    return melee_attack_t::spell_tick_power_coefficient( s );
  }

  virtual double bonus_da( const action_state_t* s ) const
  {
    if ( requires_combo_points )
      return base_dd_adder * cast_state( s ) -> cp;
    return melee_attack_t::bonus_da( s );
  }

  virtual double bonus_ta( const action_state_t* s ) const
  {
    if ( requires_combo_points )
      return base_ta_adder * cast_state( s ) -> cp;
    return melee_attack_t::bonus_ta( s );
  }

  virtual timespan_t gcd() const
  {
    timespan_t gcd = melee_attack_t::gcd();

    if ( gcd > timespan_t::zero() &&
         p() -> sets.has_set_bonus( SET_T15_4PC_MELEE ) &&
         p() -> buffs.shadow_blades -> up() )
      gcd += p() -> spell.tier15_4pc -> effectN( 2 ).time_value();

    return gcd;
  }

  virtual double composite_da_multiplier( const action_state_t* state ) const
  {
    double m = melee_attack_t::composite_da_multiplier( state );

    if ( requires_combo_points && p() -> mastery.executioner -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual double composite_ta_multiplier( const action_state_t* state ) const
  {
    double m = melee_attack_t::composite_ta_multiplier( state );

    if ( requires_combo_points && p() -> mastery.executioner -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual double composite_target_multiplier( player_t* target ) const
  {
    double m = melee_attack_t::composite_target_multiplier( target );

    rogue_td_t* td = this -> td( target );
    if ( requires_combo_points )
    {
      if ( td -> dots.revealing_strike -> is_ticking() )
        m *= 1.0 + td -> dots.revealing_strike -> current_action -> data().effectN( 3 ).percent();
      else if ( p() -> specialization() == ROGUE_COMBAT )
        p() -> procs.no_revealing_strike -> occur();
    }

    m *= 1.0 + td -> debuffs.vendetta -> value();

    if ( p() -> spec.sanguinary_vein -> ok() && td -> sanguinary_veins() )
      m *= 1.0 + p() -> spec.sanguinary_vein -> effectN( 2 ).percent();

    return m;
  }

  virtual double action_multiplier() const
  {
    double m = melee_attack_t::action_multiplier();

    if ( p() -> talent.nightstalker -> ok() && p() -> buffs.stealth -> check() )
      m += p() -> talent.nightstalker -> effectN( 2 ).percent();

    return m;
  }

  void trigger_restless_blades()
  {
    if ( ! p() -> spec.restless_blades -> ok() )
      return;

    if ( ! requires_combo_points )
      return;

    if ( combo_points_spent == 0 )
      return;

    if ( ! result_is_hit( execute_state -> result ) )
      return;

    if ( ! harmful )
      return;

    timespan_t reduction = p() -> spec.restless_blades -> effectN( 1 ).time_value() * combo_points_spent;

    p() -> cooldowns.adrenaline_rush -> ready -= reduction;
    p() -> cooldowns.killing_spree   -> ready -= reduction;
    p() -> cooldowns.shadow_blades   -> ready -= reduction;
  }
};

// ==========================================================================
// Static Functions
// ==========================================================================

// break_stealth ============================================================

static void break_stealth( rogue_t* p )
{
  if ( p -> buffs.stealth -> check() )
  {
    p -> buffs.stealth -> expire();
    p -> buffs.master_of_subtlety -> trigger();
  }

  if ( p -> buffs.vanish -> check() )
  {
    p -> buffs.vanish -> expire();
    p -> buffs.master_of_subtlety -> trigger();
  }

  if ( p -> player_t::buffs.shadowmeld -> check() )
    p -> player_t::buffs.shadowmeld -> expire();
}

// trigger_combat_potency ===================================================

static void trigger_combat_potency( rogue_attack_t* a, bool main_gauche )
{
  rogue_t* p = a -> cast();

  if ( ! p -> spec.combat_potency -> ok() )
    return;

  if ( ! a -> weapon )
    return;

  double chance = 0.2;
  if ( ! main_gauche )
    chance *= a -> weapon -> swing_time.total_seconds() / 1.4;

  if ( p -> rng().roll( chance ) )
  {
    // energy gain value is in the proc trigger spell
    p -> resource_gain( RESOURCE_ENERGY,
                        p -> spec.combat_potency -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_ENERGY ),
                        p -> gains.combat_potency );
  }
}

// trigger_energy_refund ====================================================

static void trigger_energy_refund( rogue_attack_t* a )
{
  rogue_t* p = a -> cast();

  double energy_restored = a -> resource_consumed * 0.80;

  p -> resource_gain( RESOURCE_ENERGY, energy_restored, p -> gains.energy_refund );
}

// trigger_relentless_strikes ===============================================

static void trigger_relentless_strikes( rogue_attack_t* a )
{
  rogue_t* p = a -> cast();

  if ( ! p -> spell.relentless_strikes -> ok() )
    return;

  if ( ! a -> requires_combo_points )
    return;

  rogue_td_t* td = a -> td( a -> target );
  double chance = p -> spell.relentless_strikes -> effectN( 1 ).pp_combo_points() / 100.0;
  if ( p -> rng().roll( chance * td -> combo_points.count ) )
  {
    double gain = p -> spell.relentless_strikes -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_ENERGY );
    p -> resource_gain( RESOURCE_ENERGY, gain, p -> gains.relentless_strikes );
  }
}

// trigger_seal_fate ========================================================

void trigger_seal_fate( rogue_attack_t* a )
{
  rogue_t* p = a -> p();

  if ( ! p -> spec.seal_fate -> ok() )
    return;

  if ( a -> aoe != 0 )
    return;

  // This is to prevent dual-weapon special attacks from triggering a double-proc of Seal Fate
  if ( p -> cooldowns.seal_fate -> down() )
    return;

  rogue_td_t* td = a -> td( a -> target );
  td -> combo_points.add( 1, p -> spec.seal_fate -> name_cstr() );

  p -> cooldowns.seal_fate -> start( timespan_t::from_millis( 1 ) );
  p -> procs.seal_fate -> occur();

  if ( p -> buffs.t16_2pc_melee -> trigger() )
    p -> procs.t16_2pc_melee -> occur();
}

// trigger_main_gauche ======================================================

static void trigger_main_gauche( rogue_attack_t* a )
{
  struct main_gauche_t : public rogue_attack_t
  {
    main_gauche_t( rogue_t* p ) :
      rogue_attack_t( "main_gauche", p, p -> find_spell( 86392 ) )
    {
      weapon          = &( p -> main_hand_weapon );
      special         = true;
      background      = true;
      trigger_gcd     = timespan_t::zero();
      may_crit        = true;
      proc = true; // it's proc; therefore it cannot trigger main_gauche for chain-procs
    }

    virtual void impact( action_state_t* state )
    {
      rogue_attack_t::impact( state );

      if ( result_is_hit( state -> result ) )
        trigger_combat_potency( this, true );
    }
  };

  if ( a -> proc )
    return;

  weapon_t* w = a -> weapon;

  if ( ! w || w -> slot != SLOT_MAIN_HAND )
    return;

  rogue_t* p = a -> cast();

  if ( ! p -> mastery.main_gauche -> ok() )
    return;

  double chance = p -> cache.mastery_value();

  if ( p -> rng().roll( chance ) )
  {
    if ( ! p -> active_main_gauche )
    {
      p -> active_main_gauche = new main_gauche_t( p );
      p -> active_main_gauche -> init();
    }

    p -> active_main_gauche -> execute();
  }
}

// trigger_tricks_of_the_trade ==============================================

static void trigger_tricks_of_the_trade( rogue_attack_t* a )
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

      t -> buffs.tricks_of_the_trade -> buff_duration = duration;
      t -> buffs.tricks_of_the_trade -> trigger( 1, value / 100.0 );
    }

    p -> buffs.tot_trigger -> expire();
  }
}

// trigger_venomous_wounds ==================================================

static void trigger_venomous_wounds( rogue_attack_t* a )
{
  rogue_t* p = a -> cast();

  if ( ! p -> spec.venomous_wounds -> ok() )
    return;

  if ( p -> rng().roll( p -> spec.venomous_wounds -> proc_chance() ) )
  {
    assert( p -> active_venomous_wound );

    p -> active_venomous_wound -> execute();

    p -> resource_gain( RESOURCE_ENERGY,
                        p -> spec.venomous_wounds -> effectN( 2 ).base_value(),
                        p -> gains.venomous_wounds );
  }
}

// trigger_blade_flurry =====================================================

static bool trigger_blade_flurry( action_state_t* s )
{
  struct blade_flurry_attack_t : public rogue_attack_t
  {
    blade_flurry_attack_t( rogue_t* p ) :
      rogue_attack_t( "blade_flurry_attack", p, p -> find_spell( 22482 ) )
    {
      may_miss = may_crit = proc = callbacks = may_dodge = may_parry = may_block = false;
      background = true;
      aoe = p -> spec.blade_flurry -> effectN( 4 ).base_value();
    }

    double composite_da_multiplier( const action_state_t* state ) const
    {
      double m = rogue_attack_t::composite_da_multiplier( state );

      m *= p() -> spec.blade_flurry -> effectN( 3 ).percent();

      return m;
    }

    size_t available_targets( std::vector< player_t* >& tl ) const
    {
      tl.clear();

      for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
      {
        player_t* t = sim -> actor_list[ i ];

        if ( ! t -> is_sleeping() && t -> is_enemy() && t != target )
          tl.push_back( t );
      }

      return tl.size();
    }
  };

  rogue_t* p = debug_cast< rogue_t* >( s -> action -> player );

  if ( ! p -> buffs.blade_flurry -> check() )
    return false;

  if ( ! s -> action -> weapon )
    return false;

  if ( ! s -> action -> result_is_hit( s -> result ) )
    return false;

  if ( s -> action -> sim -> active_enemies == 1 )
    return false;

  if ( s -> action -> aoe != 0 )
    return false;

  if ( ! p -> active_blade_flurry )
  {
    p -> active_blade_flurry = new blade_flurry_attack_t( p );
    p -> active_blade_flurry -> init();
  }

  p -> active_blade_flurry -> base_dd_min = s -> result_total;
  p -> active_blade_flurry -> base_dd_max = s -> result_total;
  p -> active_blade_flurry -> execute();

  return true;
}

// ==========================================================================
// Attacks
// ==========================================================================

// rogue_attack_t::impact ===================================================

void rogue_attack_t::impact( action_state_t* state )
{
  melee_attack_t::impact( state );

  if ( result_is_hit( state -> result ) )
  {
    if ( weapon && p() -> active_lethal_poison )
    {
      p() -> active_lethal_poison -> target = state -> target;
      p() -> active_lethal_poison -> execute();
    }

    if ( adds_combo_points && state -> result == RESULT_CRIT )
      trigger_seal_fate( this );

    // Legendary Daggers buff handling
    // Proc rates from: https://github.com/Aldriana/ShadowCraft-Engine/blob/master/shadowcraft/objects/proc_data.py#L504
    // Logic from: http://code.google.com/p/simulationcraft/issues/detail?id=1118
    double fof_chance = ( p() -> specialization() == ROGUE_ASSASSINATION ) ? 0.23139 : ( p() -> specialization() == ROGUE_COMBAT ) ? 0.09438 : 0.28223;
    if ( state -> target && state -> target -> level > 88 )
    {
      fof_chance *= ( 1.0 - 0.1 * ( state -> target -> level - 88 ) );
    }
    if ( rng().roll( fof_chance ) )
    {
      p() -> buffs.fof_p1 -> trigger();
      p() -> buffs.fof_p2 -> trigger();
      p() -> buffs.fof_p3 -> trigger();

      if ( ! p() -> buffs.fof_fod -> check() && p() -> buffs.fof_p3 -> check() > 30 )
      {
        // Trigging FoF and the Stacking Buff are mutually exclusive
        if ( rng().roll( 1.0 / ( 51.0 - p() -> buffs.fof_p3 -> check() ) ) )
        {
          p() -> buffs.fof_fod -> trigger();
          rogue_td_t* td = this -> td( state -> target );
          td -> combo_points.add( 5, "legendary_daggers" );
        }
      }
    }

    if ( p() -> buffs.blade_flurry -> up() )
      trigger_blade_flurry( state );

    // Prevent poisons from proccing it
    if ( ! proc && td( state -> target ) -> debuffs.vendetta -> up() )
      p() -> buffs.toxicologist -> trigger();
  }
}

// rogue_attack_t::armor ====================================================

double rogue_attack_t::target_armor( player_t* t ) const
{
  double a = melee_attack_t::target_armor( t );

  a *= 1.0 - td( t ) -> debuffs.find_weakness -> current_value;

  return a;
}

// rogue_attack_t::cost =====================================================

double rogue_attack_t::cost() const
{
  double c = melee_attack_t::cost();

  if ( c <= 0 )
    return 0;

  if ( p() -> talent.shadow_focus -> ok() &&
       ( p() -> buffs.stealth -> check() || p() -> buffs.vanish -> check() ) )
  {
    c *= 1.0 + p() -> spell.shadow_focus -> effectN( 1 ).percent();
  }

  if ( p() -> sets.has_set_bonus( SET_T15_2PC_MELEE ) && p() -> buffs.tier13_2pc -> check() )
    c *= 1.0 + p() -> spell.tier13_2pc -> effectN( 1 ).percent();

  if ( p() -> sets.set(SET_T15_4PC_MELEE ) -> ok() && p() -> buffs.shadow_blades -> check() )
    c *= 1.0 + p() -> spell.tier15_4pc -> effectN ( 1 ).percent();

  return c;
}

// rogue_attack_t::consume_resource =========================================

void rogue_attack_t::consume_resource()
{
  melee_attack_t::consume_resource();

  rogue_t* p = cast();

  combo_points_spent = 0;

  if ( result_is_hit( execute_state -> result ) )
  {
    trigger_relentless_strikes( this );

    if ( requires_combo_points )
    {
      rogue_td_t* td = this -> td( target );
      combo_points_spent = td -> combo_points.count;

      td -> combo_points.clear( name() );

      if ( p -> buffs.fof_fod -> up() )
        td -> combo_points.add( 5, "legendary_daggers" );
    }
  }
  else if ( resource_consumed > 0 )
    trigger_energy_refund( this );

  if ( combo_points_spent > 0 )
  {
    double chance = p -> spec.ruthlessness -> effectN( 1 ).pp_combo_points() * combo_points_spent / 100.0;
    if ( p -> rng().roll( chance ) )
      td( execute_state -> target ) -> combo_points.add( p -> spell.ruthlessness_cp -> effectN( 1 ).base_value(), "ruthlessness" );
  }
}

// rogue_attack_t::execute ==================================================

void rogue_attack_t::execute()
{
  rogue_td_t* td = this -> td( target );

  melee_attack_t::execute();

  if ( harmful && stealthed() )
  {
    if ( ! p() -> talent.subterfuge -> ok() )
      break_stealth( p() );
    else if ( ! p() -> buffs.subterfuge -> check() )
      p() -> buffs.subterfuge -> trigger();
  }

  if ( result_is_hit( execute_state -> result ) )
  {
    if ( adds_combo_points )
    {
      int points = adds_combo_points;
      if ( p() -> buffs.shadow_blades -> up() )
        points++;
      td -> combo_points.add( points, name() );
    }

    if ( may_crit )  // Rupture and Garrote can't proc MG, issue 581
      trigger_main_gauche( this );

    trigger_tricks_of_the_trade( this );
    trigger_restless_blades();
  }
}

// rogue_attack_t::calculate_weapon_damage ==================================

double rogue_attack_t::calculate_weapon_damage( double attack_power )
{
  double dmg = melee_attack_t::calculate_weapon_damage( attack_power );

  if ( dmg == 0 ) return 0;

  if ( weapon -> slot == SLOT_OFF_HAND )
    dmg *= 1.0 + p() -> spec.ambidexterity -> effectN( 1 ).percent();

  return dmg;
}

// rogue_attack_t::ready() ==================================================

bool rogue_attack_t::ready()
{
  rogue_t* p = cast();

  if ( ! melee_attack_t::ready() )
    return false;

  if ( requires_combo_points )
  {
    rogue_td_t* td = this -> td( target );
    if ( ! td -> combo_points.count )
      return false;
  }

  if ( requires_stealth )
  {
    if ( ! p -> buffs.shadow_dance -> check() &&
         ! p -> buffs.stealth -> check() &&
         ! player -> buffs.shadowmeld -> check() &&
         ! p -> buffs.vanish -> check() && 
         ! p -> buffs.subterfuge -> check() )
    {
      return false;
    }
  }

  if ( requires_position != POSITION_NONE )
    if ( p -> position() != requires_position )
      return false;

  if ( requires_weapon != WEAPON_NONE )
    if ( ! weapon || weapon -> type != requires_weapon )
      return false;

  return true;
}

// Melee Attack =============================================================

struct melee_t : public rogue_attack_t
{
  int sync_weapons;
  bool first;

  melee_t( const char* name, rogue_t* p, int sw ) :
    rogue_attack_t( name, p ), sync_weapons( sw ), first( true )
  {
    school          = SCHOOL_PHYSICAL;
    background      = true;
    repeating       = true;
    trigger_gcd     = timespan_t::zero();
    special         = false;
    may_glance      = true;

    if ( p -> dual_wield() )
      base_hit -= 0.19;
  }

  void reset()
  {
    rogue_attack_t::reset();

    first = true;
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = rogue_attack_t::execute_time();
    if ( first )
    {
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 ) : timespan_t::zero();
    }
    return t;
  }

  virtual void execute() override
  {
    if ( first )
    {
      first = false;
    }
    rogue_attack_t::execute();
  }

  virtual void impact( action_state_t* state )
  {
    rogue_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( weapon -> slot == SLOT_OFF_HAND )
        trigger_combat_potency( this, false );
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
      opt_bool( "sync_weapons", sync_weapons ),
      opt_null()
    };
    parse_options( options, options_str );

    assert( p -> main_hand_weapon.type != WEAPON_NONE );

    p -> melee_main_hand = p -> main_hand_attack = new melee_t( "melee_main_hand", p, sync_weapons );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> melee_off_hand = p -> off_hand_attack = new melee_t( "melee_off_hand", p, sync_weapons );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
      p -> off_hand_attack -> id = 1;
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    player -> main_hand_attack -> schedule_execute();

    if ( player -> off_hand_attack )
      player -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    if ( player -> is_moving() )
      return false;

    return ( player -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Adrenaline Rush ==========================================================

struct adrenaline_rush_t : public rogue_attack_t
{
  adrenaline_rush_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "adrenaline_rush", p, p -> find_class_spell( "Adrenaline Rush" ), options_str )
  {
    harmful = may_miss = may_crit = false;
  }

  void execute()
  {
    rogue_attack_t::execute();

    p() -> buffs.adrenaline_rush -> trigger();
  }
};

// Ambush ===================================================================

struct ambush_t : public rogue_attack_t
{
  ambush_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "ambush", p, p -> find_class_spell( "Ambush" ), options_str )
  {
    requires_position = POSITION_BACK;
    requires_stealth  = true;

    if ( weapon -> type == WEAPON_DAGGER )
      weapon_multiplier   *= 1.447; // It'is in the description.
  }

  virtual double cost() const
  {
    double c = rogue_attack_t::cost();

    if ( p() -> buffs.shadow_dance -> check() )
      c += p() -> spec.shadow_dance -> effectN( 2 ).base_value();

    c -= 2 * p() -> buffs.t16_2pc_melee -> stack();

    return c;
  }

  void execute()
  {
    rogue_attack_t::execute();

    p() -> buffs.t16_2pc_melee -> expire();
    p() -> buffs.sleight_of_hand -> expire();
  }

  void impact( action_state_t* state )
  {
    rogue_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      rogue_td_t* td = this -> td( state -> target );

      td -> debuffs.find_weakness -> trigger();
    }
  }

  bool ready()
  {
    bool rd;

    if ( p() -> buffs.sleight_of_hand -> check() )
    {
      // Sigh ....
      requires_stealth = false;
      rd = rogue_attack_t::ready();
      requires_stealth = true;
    }
    else
      rd = rogue_attack_t::ready();

    return rd;
  }
};

// Backstab =================================================================

struct backstab_t : public rogue_attack_t
{
  backstab_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "backstab", p, p -> find_class_spell( "Backstab" ), options_str )
  {
    requires_weapon   = WEAPON_DAGGER;
    requires_position = POSITION_BACK;
  }

  virtual double cost() const
  {
    double c = rogue_attack_t::cost();
    c -= 2 * p() -> buffs.t16_2pc_melee -> stack();
    return c;
  }

  void execute()
  {
    rogue_attack_t::execute();

    p() -> buffs.t16_2pc_melee -> expire();

    if ( result_is_hit( execute_state -> result ) && p() -> sets.has_set_bonus( SET_T16_4PC_MELEE ) )
      p() -> buffs.sleight_of_hand -> trigger();
  }

  double composite_da_multiplier( const action_state_t* state ) const
  {
    double m = rogue_attack_t::composite_da_multiplier( state );

    m *= 1.0 + p() -> sets.set( SET_T14_2PC_MELEE ) -> effectN( 2 ).percent();

    return m;
  }
};

// Blade Flurry =============================================================

struct blade_flurry_t : public rogue_attack_t
{
  blade_flurry_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "blade_flurry", p, p -> find_specialization_spell( "Blade Flurry" ), options_str )
  {
    harmful = may_miss = may_crit = false;
  }

  void execute()
  {
    rogue_attack_t::execute();

    if ( ! p() -> buffs.blade_flurry -> check() )
      p() -> buffs.blade_flurry -> trigger();
    else
      p() -> buffs.blade_flurry -> expire();
  }
};

// Dispatch =================================================================

struct dispatch_t : public rogue_attack_t
{
  dispatch_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "dispatch", p, p -> find_class_spell( "Dispatch" ), options_str )
  {
    if ( p -> main_hand_weapon.type != WEAPON_DAGGER )
    {
      sim -> errorf( "Trying to use %s without a dagger in main-hand", name() );
      background = true;
    }
  }

  double cost() const
  {
    if ( p() -> buffs.blindside -> check() )
      return 0;
    else
    {
      double c = rogue_attack_t::cost();
      c -= 6 * p() -> buffs.t16_2pc_melee -> stack();
      return c;
    }
  }

  void execute()
  {
    rogue_attack_t::execute();

    if ( p() -> buffs.blindside -> check() )
      p() -> buffs.blindside -> expire();
    else
      p() -> buffs.t16_2pc_melee -> expire();
  }

  bool ready()
  {
    if ( ! p() -> buffs.blindside -> check() && target -> health_percentage() > 35 )
      return false;

    return rogue_attack_t::ready();
  }
};

// Envenom ==================================================================

struct envenom_t : public rogue_attack_t
{
  envenom_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "envenom", p, p -> find_class_spell( "Envenom" ), options_str )
  {
    weapon = &( p -> main_hand_weapon );
    requires_combo_points  = true;
    attack_power_mod.direct       = 0.134;
    dot_duration = timespan_t::zero();
    base_dd_min            = base_dd_max = 0.213 * p -> dbc.spell_scaling( p -> type, p -> level );
    weapon_multiplier = weapon_power_mod = 0.0;
  }

  double base_da_min( const action_state_t* s ) const
  { return base_dd_min * rogue_attack_t::cast_state( s ) -> cp; }

  double base_da_max( const action_state_t* s ) const
  { return base_dd_max * rogue_attack_t::cast_state( s ) -> cp; }

  virtual void execute()
  {
    rogue_td_t* td = this -> td( target );

    timespan_t envenom_duration = p() -> buffs.envenom -> buff_period * ( 1 + td -> combo_points.count );

    if ( p() -> sets.has_set_bonus( SET_T15_2PC_MELEE ) )
      envenom_duration += p() -> buffs.envenom -> buff_period;
    p() -> buffs.envenom -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, envenom_duration );

    rogue_attack_t::execute();
  }

  virtual double action_da_multiplier() const
  {
    double m = rogue_attack_t::action_da_multiplier();

    if ( p() -> mastery.potent_poisons -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual void impact( action_state_t* state )
  {
    rogue_attack_t::impact( state );

    if ( p() -> spec.cut_to_the_chase -> ok() && p() -> buffs.slice_and_dice -> check() )
    {
      double snd = p() -> buffs.slice_and_dice -> data().effectN( 1 ).percent();
      if ( p() -> mastery.executioner -> ok() )
        snd *= 1.0 + p() -> cache.mastery_value();
      timespan_t snd_duration = 3 * 6 * p() -> buffs.slice_and_dice -> buff_period;

      p() -> buffs.slice_and_dice -> trigger( 1, snd, -1.0, snd_duration );
    }
  }
};

// Eviscerate ===============================================================

struct eviscerate_t : public rogue_attack_t
{
  eviscerate_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "eviscerate", p, p -> find_class_spell( "Eviscerate" ), options_str )
  {
    requires_combo_points = true;
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier = weapon_power_mod = 0;

    attack_power_mod.direct = 0.18;
  }

  timespan_t gcd() const
  {
    timespan_t t = rogue_attack_t::gcd();

    if ( t != timespan_t::zero() && p() -> buffs.adrenaline_rush -> check() )
      t += p() -> buffs.adrenaline_rush -> data().effectN( 3 ).time_value();

    return t;
  }

  virtual void impact( action_state_t* state )
  {
    rogue_attack_t::impact( state );

    if ( p() -> spec.cut_to_the_chase -> ok() && p() -> buffs.slice_and_dice -> check() )
    {
      double snd = p() -> buffs.slice_and_dice -> data().effectN( 1 ).percent();
      if ( p() -> mastery.executioner -> ok() )
        snd *= 1.0 + p() -> cache.mastery_value();
      timespan_t snd_duration = 3 * 6 * p() -> buffs.slice_and_dice -> buff_period;

      p() -> buffs.slice_and_dice -> trigger( 1, snd, -1.0, snd_duration );
    }
  }
};

// Expose Armor =============================================================

struct expose_armor_t : public rogue_attack_t
{
  expose_armor_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "expose_armor", p, p -> find_class_spell( "Expose Armor" ), options_str )
  { }

  virtual void execute()
  {
    rogue_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      rogue_td_t* td = this -> td( target );
      td -> combo_points.add( 1 );
    }
  };
};

// Fan of Knives ============================================================

struct fan_of_knives_t : public rogue_attack_t
{
  fan_of_knives_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "fan_of_knives", p, p -> find_class_spell( "Fan of Knives" ), options_str )
  {
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = weapon_power_mod = 0;
    aoe              = -1;
  }

};

// Crimson Tempest ==========================================================

struct crimson_tempest_t : public rogue_attack_t
{
  struct crimson_tempest_dot_t : public rogue_attack_t
  {
    crimson_tempest_dot_t( rogue_t * p ) :
      rogue_attack_t( "crimson_tempest_dot", p, p -> find_spell( 122233 ) )
    {
      may_miss = may_dodge = may_parry = may_block = may_crit = tick_may_crit = false;
      background = true;
      dot_behavior = DOT_REFRESH;
    }
  };

  crimson_tempest_dot_t* ct_dot;

  crimson_tempest_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "crimson_tempest", p, p -> find_class_spell( "Crimson Tempest" ), options_str )
  {
    aoe = -1;
    requires_combo_points = true;
    attack_power_mod.direct = 0.0275;
    weapon = &( p -> main_hand_weapon );
    weapon_power_mod = weapon_multiplier = 0;
    ct_dot = new crimson_tempest_dot_t( p );
    add_child( ct_dot );
  }

  void impact( action_state_t* s )
  {
    rogue_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      ct_dot -> pre_execute_state = ct_dot -> get_state( s );
      ct_dot -> target = s -> target;
      ct_dot -> base_td = s -> result_amount * ct_dot -> data().effectN( 1 ).percent() * ct_dot -> dot_duration / ct_dot -> base_tick_time;
      ct_dot -> execute();
    }
  }
};

// Garrote ==================================================================

struct garrote_t : public rogue_attack_t
{
  garrote_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "garrote", p, p -> find_class_spell( "Garrote" ), options_str )
  {
    may_crit          = false;
    requires_stealth  = true;
    attack_power_mod.tick    = 0.078;
  }

  void impact( action_state_t* state )
  {
    rogue_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      rogue_td_t* td = this -> td( state -> target );
      td -> debuffs.find_weakness -> trigger();
    }
  }

  virtual void tick( dot_t* d )
  {
    rogue_attack_t::tick( d );

    rogue_td_t* td = this -> td( d -> state -> target );
    if ( ! td -> dots.rupture -> is_ticking() )
      trigger_venomous_wounds( this );
  }
};

// Hemorrhage ===============================================================

struct hemorrhage_t : public rogue_attack_t
{
  hemorrhage_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "hemorrhage", p, p -> find_class_spell( "Hemorrhage" ), options_str )
  {
    weapon = &( p -> main_hand_weapon );
    if ( weapon -> type == WEAPON_DAGGER )
      weapon_multiplier *= 1.45; // number taken from spell description

    base_tick_time = p -> find_spell( 89775 ) -> effectN( 1 ).period();
    dot_duration = p -> find_spell( 89775 ) -> duration();
    dot_behavior = DOT_REFRESH;
  }

  virtual void impact( action_state_t* state )
  {
    if ( result_is_hit( state -> result ) )
      base_td = state -> result_amount * data().effectN( 4 ).percent() * dot_duration / base_tick_time;

    rogue_attack_t::impact( state );
  }
};

// Kick =====================================================================

struct kick_t : public rogue_attack_t
{
  kick_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "kick", p, p -> find_class_spell( "Kick" ), options_str )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;

    if ( p -> glyph.kick -> ok() )
    {
      // All kicks are assumed to interrupt a cast
      cooldown -> duration -= timespan_t::from_seconds( 2.0 ); // + 4 Duration - 6 on Success = -2
    }
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return rogue_attack_t::ready();
  }
};

// Killing Spree ============================================================

struct killing_spree_tick_t : public rogue_attack_t
{
  killing_spree_tick_t( rogue_t* p, const char* name, const spell_data_t* s ) :
    rogue_attack_t( name, p, s )
  {
    school      = SCHOOL_PHYSICAL;
    background  = true;
    may_crit    = true;
    direct_tick = true;
  }
};

struct killing_spree_t : public rogue_attack_t
{
  melee_attack_t* attack_mh;
  melee_attack_t* attack_oh;

  killing_spree_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "killing_spree", p, p -> find_class_spell( "Killing Spree" ), options_str ),
    attack_mh( 0 ), attack_oh( 0 )
  {
    dot_duration = 6 * base_tick_time;
    may_miss  = false;
    may_crit  = false;
    channeled = true;
    tick_zero = true;

    attack_mh = new killing_spree_tick_t( p, "killing_spree_mh", p -> find_spell( 57841 ) );
    attack_mh -> weapon = &( player -> main_hand_weapon );
    add_child( attack_mh );

    if ( player -> off_hand_weapon.type != WEAPON_NONE )
    {
      attack_oh = new killing_spree_tick_t( p, "killing_spree_oh", p -> find_spell( 57841 ) -> effectN( 2 ).trigger() );
      attack_oh -> weapon = &( player -> off_hand_weapon );
      add_child( attack_oh );
    }
  }

  double composite_target_da_multiplier( player_t* target ) const
  {
    double m = rogue_attack_t::composite_target_da_multiplier( target );

    rogue_td_t* td = this -> td( target );
    if ( td -> dots.killing_spree -> current_tick >= 0 )
        m *= std::pow( 1.0 + p() -> sets.set( SET_T16_4PC_MELEE ) -> effectN( 1 ).percent(),
                       td -> dots.killing_spree -> current_tick + 1 );

    return m;
  }

  timespan_t tick_time( double ) const
  { return base_tick_time; }

  virtual void execute()
  {
    p() -> buffs.killing_spree -> trigger();

    rogue_attack_t::execute();
  }

  virtual void tick( dot_t* d )
  {
    rogue_attack_t::tick( d );

    attack_mh -> pre_execute_state = attack_mh -> get_state( d -> state );
    attack_mh -> execute();

    if ( attack_oh && result_is_hit( attack_mh -> execute_state -> result ) )
    {
      attack_oh -> pre_execute_state = attack_oh -> get_state( d -> state );
      attack_oh -> execute();
    }
  }
};

// Marked for Death =========================================================

struct marked_for_death_t : public rogue_attack_t
{
  marked_for_death_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "marked_for_death", p, p -> find_talent_spell( "Marked for Death" ), options_str )
  {
    may_miss = may_crit = harmful = false;
    adds_combo_points = data().effectN( 1 ).base_value();
  }
};



// Mutilate =================================================================

struct mutilate_strike_t : public rogue_attack_t
{
  mutilate_strike_t( rogue_t* p, const char* name, const spell_data_t* s ) :
    rogue_attack_t( name, p, s )
  {
    background  = true;
    may_miss = may_dodge = may_parry = false;
  }

  void impact( action_state_t* state )
  {
    rogue_attack_t::impact( state );

    if ( state -> result == RESULT_CRIT )
      trigger_seal_fate( this );
  }
};

struct mutilate_t : public rogue_attack_t
{
  rogue_attack_t* mh_strike;
  rogue_attack_t* oh_strike;

  mutilate_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "mutilate", p, p -> find_class_spell( "Mutilate" ), options_str ),
    mh_strike( 0 ), oh_strike( 0 )
  {
    may_crit = false;

    if ( p -> main_hand_weapon.type != WEAPON_DAGGER ||
         p ->  off_hand_weapon.type != WEAPON_DAGGER )
    {
      sim -> errorf( "Player %s attempting to execute Mutilate without two daggers equipped.", p -> name() );
      background = true;
    }

    mh_strike = new mutilate_strike_t( p, "mutilate_mh", data().effectN( 2 ).trigger() );
    mh_strike -> weapon = &( p -> main_hand_weapon );
    add_child( mh_strike );

    oh_strike = new mutilate_strike_t( p, "mutilate_oh", data().effectN( 3 ).trigger() );
    oh_strike -> weapon = &( p -> off_hand_weapon );
    add_child( oh_strike );
  }

  virtual double cost() const
  {
    double c = rogue_attack_t::cost();
    if ( p() -> buffs.t16_2pc_melee -> up() )
      c -= 6 * p() -> buffs.t16_2pc_melee -> check();

    return c;
  }

  virtual void execute()
  {
    rogue_attack_t::execute();

    p() -> buffs.t16_2pc_melee -> expire();

    if ( result_is_hit( execute_state -> result ) )
    {
      mh_strike -> schedule_execute( mh_strike -> get_state( execute_state ) );
      oh_strike -> schedule_execute( oh_strike -> get_state( execute_state ) );
    }
  }

  virtual void impact( action_state_t* state )
  {
    rogue_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
      p() -> buffs.blindside -> trigger();
  }
};

// Premeditation ============================================================

struct premeditation_t : public rogue_attack_t
{
  struct premeditation_event_t : public event_t
  {
    int combo_points;
    player_t* target;

    premeditation_event_t( rogue_t& p, player_t* t, timespan_t duration, int cp ) :
      event_t( p, "premeditation" ),
      combo_points( cp ), target( t )
    {
      add_event( duration );
    }

    void execute()
    {
      rogue_t* p = static_cast< rogue_t* >( actor );
      rogue_td_t* td = p -> get_target_data( target );

      td -> combo_points.count -= combo_points;
      if ( sim().log )
      {
        sim().out_log.printf( "%s loses %d temporary combo_points from premeditation (%d)",
                    td -> combo_points.target -> name(),
                    p -> find_specialization_spell( "Premeditation" ) -> effectN( 1 ).base_value(),
                    td -> combo_points.count );
      }

      assert( td -> combo_points.count >= 0 );
    }
  };

  premeditation_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "premeditation", p, p -> find_specialization_spell( "Premeditation" ), options_str )
  {
    harmful = may_crit = may_miss = false;
    requires_stealth = true;
    // We need special combo points handling here
    adds_combo_points = 0;
  }

  void impact( action_state_t* state )
  {
    rogue_attack_t::impact( state );

    rogue_td_t* td = this -> td( state -> target );
    int add_points = data().effectN( 1 ).base_value();

    add_points = std::min( add_points, combo_points_t::max_combo_points - td -> combo_points.count );

    if ( add_points > 0 )
      td -> combo_points.add( add_points, "premeditation" );

    p() -> event_premeditation = new ( *sim ) premeditation_event_t( *p(), state -> target, data().duration(), data().effectN( 2 ).base_value() );
  }
};

// Recuperate ===============================================================

struct recuperate_t : public rogue_attack_t
{
  recuperate_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "recuperate", p, p -> find_class_spell( "Recuperate" ), options_str )
  {
    requires_combo_points = true;
    dot_behavior = DOT_REFRESH;
    harmful = false;
  }

  virtual timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    rogue_td_t* td = this->td( s->target );
    return 2 * td->combo_points.count * base_tick_time;
  }

  virtual void execute()
  {
    rogue_attack_t::execute();

    p() -> buffs.recuperate -> trigger();
  }

  virtual void last_tick( dot_t* d )
  {
    p() -> buffs.recuperate -> expire();

    rogue_attack_t::last_tick( d );
  }
};

// Revealing Strike =========================================================

struct revealing_strike_t : public rogue_attack_t
{
  revealing_strike_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "revealing_strike", p, p -> find_class_spell( "Revealing Strike" ), options_str )
  {
    dot_behavior = DOT_REFRESH;

    // Legendary buff increases RS damage
    if ( p -> fof_p1 || p -> fof_p2 || p -> fof_p3 )
      base_multiplier *= 1.0 + p -> dbc.spell( 110211 ) -> effectN( 1 ).percent();
  }

  timespan_t gcd() const
  {
    timespan_t t = rogue_attack_t::gcd();

    if ( t != timespan_t::zero() && p() -> buffs.adrenaline_rush -> check() )
      t += p() -> buffs.adrenaline_rush -> data().effectN( 3 ).time_value();

    return t;
  }

  virtual void impact( action_state_t* state )
  {
    rogue_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
      p() -> buffs.bandits_guile -> trigger();
  }
};

// Rupture ==================================================================

struct rupture_t : public rogue_attack_t
{
  double combo_point_tick_power_mod[ combo_points_t::max_combo_points ];

  rupture_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "rupture", p, p -> find_class_spell( "Rupture" ), options_str )
  {
    combo_point_tick_power_mod[ 0 ] = 0.025;
    combo_point_tick_power_mod[ 1 ] = 0.04;
    combo_point_tick_power_mod[ 2 ] = 0.05;
    combo_point_tick_power_mod[ 3 ] = 0.056;
    combo_point_tick_power_mod[ 4 ] = 0.062;

    may_crit              = false;
    requires_combo_points = true;
    tick_may_crit         = true;
    dot_behavior          = DOT_REFRESH;
    base_multiplier      += p -> spec.sanguinary_vein -> effectN( 1 ).percent();
  }

  double tick_power_coefficient( const action_state_t* state ) const
  { return combo_point_tick_power_mod[ rogue_attack_t::cast_state( state ) -> cp - 1 ]; }

  timespan_t gcd() const
  {
    timespan_t t = rogue_attack_t::gcd();

    if ( t != timespan_t::zero() && p() -> buffs.adrenaline_rush -> check() )
      t += p() -> buffs.adrenaline_rush -> data().effectN( 3 ).time_value();

    return t;
  }

  virtual timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    int num_ticks = 2 + 2 * td( s -> target ) -> combo_points.count;
    if ( p() -> sets.has_set_bonus( SET_T15_2PC_MELEE ) )
      num_ticks += 2;

    return num_ticks * base_tick_time;
  }

  virtual void tick( dot_t* d )
  {
    rogue_attack_t::tick( d );

    trigger_venomous_wounds( this );
  }
};

// Shadowstep ===============================================================

struct shadowstep_t : public rogue_attack_t
{
  shadowstep_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shadowstep", p, p -> find_class_spell( "Shadowstep" ), options_str )
  {
    harmful = false;
  }
};

// Shiv =====================================================================

struct shiv_t : public rogue_attack_t
{
  shiv_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shiv", p, p -> find_class_spell( "Shiv" ), options_str )
  {
    weapon = &( p -> off_hand_weapon );
    if ( weapon -> type == WEAPON_NONE )
      background = true; // Do not allow execution.

    may_crit          = false;
  }

  virtual void execute()
  {
    rogue_t* p = cast();

    p -> buffs.shiv -> trigger();
    rogue_attack_t::execute();
    p -> buffs.shiv -> expire();
  }
};

// Shuriken Toss ============================================================

struct shuriken_toss_t : public rogue_attack_t
{
  shuriken_toss_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shuriken_toss", p, p -> find_talent_spell( "Shuriken Toss" ), options_str )
  {
    adds_combo_points = 1; // it has an effect but with no base value :rollseyes:
  }
};

// Sinister Strike ==========================================================

struct sinister_strike_t : public rogue_attack_t
{
  sinister_strike_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "sinister_strike", p, p -> find_class_spell( "Sinister Strike" ), options_str )
  {
    adds_combo_points = 1; // it has an effect but with no base value :rollseyes:
  }

  timespan_t gcd() const
  {
    timespan_t t = rogue_attack_t::gcd();

    if ( t != timespan_t::zero() && p() -> buffs.adrenaline_rush -> check() )
      t += p() -> buffs.adrenaline_rush -> data().effectN( 3 ).time_value();

    return t;
  }

  virtual double cost() const
  {
    double c = rogue_attack_t::cost();
    c -= 15 * p() -> buffs.t16_2pc_melee -> stack();
    return c;
  }

  double composite_da_multiplier( const action_state_t* state ) const
  {
    double m = rogue_attack_t::composite_da_multiplier( state );

    if ( p() -> fof_p1 || p() -> fof_p2 || p() -> fof_p3 )
      m *= 1.0 + p() -> dbc.spell( 110211 ) -> effectN( 1 ).percent();

    m *= 1.0 + p() -> sets.set( SET_T14_2PC_MELEE ) -> effectN( 2 ).percent();

    return m;
  }

  virtual void execute()
  {
    rogue_attack_t::execute();

    p() -> buffs.t16_2pc_melee -> expire();
  }

  virtual void impact( action_state_t* state )
  {
    rogue_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      p() -> buffs.bandits_guile -> trigger();

      rogue_td_t* td = this -> td( state -> target );
      if ( td -> dots.revealing_strike -> is_ticking() &&
          rng().roll( td -> dots.revealing_strike -> current_action -> data().proc_chance() ) )
      {
        td -> combo_points.add( 1, "sinister_strike" );
        if ( p() -> buffs.t16_2pc_melee -> trigger() )
          p() -> procs.t16_2pc_melee -> occur();
      }
    }
  }
};

// Slice and Dice ===========================================================

struct slice_and_dice_t : public rogue_attack_t
{
  slice_and_dice_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "slice_and_dice", p, p -> find_class_spell( "Slice and Dice" ), options_str )
  {
    requires_combo_points = true;
    harmful               = false;
    dot_duration = timespan_t::zero();
  }

  timespan_t gcd() const
  {
    timespan_t t = rogue_attack_t::gcd();

    if ( t != timespan_t::zero() && p() -> buffs.adrenaline_rush -> check() )
      t += p() -> buffs.adrenaline_rush -> data().effectN( 3 ).time_value();

    return t;
  }

  virtual void execute()
  {
    rogue_td_t* td = this -> td( target );
    int action_cp = td -> combo_points.count;

    rogue_attack_t::execute();

    double snd = p() -> buffs.slice_and_dice -> data().effectN( 1 ).percent();
    if ( p() -> mastery.executioner -> ok() )
      snd *= 1.0 + p() -> cache.mastery_value();
    timespan_t snd_duration = 3 * ( action_cp + 1 ) * p() -> buffs.slice_and_dice -> buff_period;

    if ( p() -> sets.has_set_bonus( SET_T15_2PC_MELEE ) )
      snd_duration += 3 * p() -> buffs.slice_and_dice -> buff_period;

    p() -> buffs.slice_and_dice -> trigger( 1, snd, -1.0, snd_duration );
  }
};

// Preparation ==============================================================

struct preparation_t : public rogue_attack_t
{
  std::vector<cooldown_t*> cooldown_list;

  preparation_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "preparation", p, p -> find_class_spell( "Preparation" ), options_str )
  {
    harmful = may_miss = may_crit = false;

    cooldown_list.push_back( p -> get_cooldown( "vanish" ) );
  }

  virtual void execute()
  {
    rogue_attack_t::execute();

    for ( size_t i = 0; i < cooldown_list.size(); ++i )
      cooldown_list[ i ] -> reset( false );
  }
};

// Shadow Dance =============================================================

struct shadow_dance_t : public rogue_attack_t
{
  shadow_dance_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shadow_dance", p, p -> find_class_spell( "Shadow Dance" ), options_str )
  {
    harmful = may_miss = may_crit = false;
  }

  void execute()
  {
    rogue_attack_t::execute();

    p() -> buffs.shadow_dance -> trigger();
  }
};

// Tricks of the Trade ======================================================

struct tricks_of_the_trade_t : public rogue_attack_t
{
  tricks_of_the_trade_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "tricks_of_the_trade", p, p -> find_class_spell( "Tricks of the Trade" ), options_str )
  {
    may_miss = may_crit = harmful = false;

    if ( p -> sim -> solo_raid || ( ! sim -> optimal_raid && p -> tricks_of_the_trade_target_str == "self" ) )
    {
      p -> tot_target = NULL;
      background = true;
      target = NULL;
    }
    else
    {
      if ( ! p -> tricks_of_the_trade_target_str.empty() )
        target_str = p -> tricks_of_the_trade_target_str;
      else
        target = NULL;

      if ( util::str_compare_ci( target_str, "self" ) )
        target = p;
      else if ( target_str.empty() )
        target = NULL;
      else
      {
        player_t* q = sim -> find_player( target_str );

        if ( q )
          target = q;
        else
        {
          sim -> errorf( "%s %s: Unable to locate target '%s'.\n", player -> name(), name(), options_str.c_str() );
          target = NULL;
        }
      }

      p -> tot_target = target;
    }
  }

  virtual void execute()
  {
    rogue_t* p = cast();

    rogue_attack_t::execute();

    p -> buffs.tier13_2pc -> trigger();
    p -> buffs.tot_trigger -> trigger();
  }

  bool ready()
  {
    if ( ! target )
      return false;

    return rogue_attack_t::ready();
  }
};

// Vanish ===================================================================

struct vanish_t : public rogue_attack_t
{
  vanish_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "vanish", p, p -> find_class_spell( "Vanish" ), options_str )
  {
    may_miss = may_crit = harmful = false;
  }

  void execute()
  {
    rogue_attack_t::execute();

    p() -> buffs.vanish -> trigger();

    // Vanish stops autoattacks
    if ( p() -> main_hand_attack && p() -> main_hand_attack -> execute_event )
      core_event_t::cancel( p() -> main_hand_attack -> execute_event );

    if ( p() -> off_hand_attack && p() -> off_hand_attack -> execute_event )
      core_event_t::cancel( p() -> off_hand_attack -> execute_event );
  }
};

// Vendetta =================================================================

struct vendetta_t : public rogue_attack_t
{
  vendetta_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "vendetta", p, p -> find_class_spell( "Vendetta" ), options_str )
  {
    harmful = may_miss = may_crit = false;
  }

  void execute()
  {
    rogue_attack_t::execute();

    rogue_td_t* td = this -> td( execute_state -> target );

    td -> debuffs.vendetta -> trigger();
  }
};

// Shadow Blades ============================================================

struct shadow_blade_t : public rogue_attack_t
{
  shadow_blade_t( rogue_t* p, const spell_data_t* s ) :
    rogue_attack_t( "", p, s )
  {
    school  = SCHOOL_SHADOW;
    special = false;
    repeating = true;
    background = true;
    may_glance = false;
  }
};

struct shadow_blades_t : public rogue_attack_t
{
  shadow_blades_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shadow_blades", p, p -> find_class_spell( "Shadow Blades" ), options_str )
  {
    harmful = may_miss = may_crit = false;

    if ( ! p -> shadow_blade_main_hand )
    {
      p -> shadow_blade_main_hand = new shadow_blade_t( p, data().effectN( 1 ).trigger() );
      p -> shadow_blade_main_hand -> weapon = &( p -> main_hand_weapon );
      p -> shadow_blade_main_hand -> base_execute_time = p -> main_hand_weapon.swing_time;
    }

    if ( ! p -> shadow_blade_off_hand && p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> shadow_blade_off_hand = new shadow_blade_t( p, p -> find_spell( data().effectN( 1 ).misc_value1() ) );
      p -> shadow_blade_off_hand -> weapon = &( p -> off_hand_weapon );
      p -> shadow_blade_off_hand -> base_execute_time = p -> off_hand_weapon.swing_time;
    }
  }

  void execute()
  {
    rogue_attack_t::execute();

    p() -> buffs.shadow_blades -> trigger();
  }
};

// ==========================================================================
// Stealth
// ==========================================================================

struct stealth_t : public spell_t
{
  bool used;

  stealth_t( rogue_t* p, const std::string& options_str ) :
    spell_t( "stealth", p, p -> find_class_spell( "Stealth" ) ), used( false )
  {
    harmful = false;

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_t* p = debug_cast< rogue_t* >( player );

    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", p -> name(), name() );

    p -> buffs.stealth -> trigger();
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

} // end namespace actions

namespace poisons {
// ==========================================================================
// Poisons
// ==========================================================================

struct rogue_poison_t : public actions::rogue_attack_t
{
  double proc_chance;

  rogue_poison_t( const std::string& token, rogue_t* p,
                  const spell_data_t* s = spell_data_t::nil() ) :
    actions::rogue_attack_t( token, p, s ),
    proc_chance( 0 )
  {
    proc              = true;
    background        = true;
    trigger_gcd       = timespan_t::zero();
    may_dodge         = false;
    may_parry         = false;
    may_block         = false;

    weapon_multiplier = 0;

    proc_chance    = data().proc_chance();
    proc_chance   += p -> spec.improved_poisons -> effectN( 1 ).percent();
  }

  virtual double action_da_multiplier() const
  {
    double m = rogue_attack_t::action_da_multiplier();

    if ( p() -> mastery.potent_poisons -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual double action_ta_multiplier() const
  {
    double m = rogue_attack_t::action_ta_multiplier();

    if ( p() -> mastery.potent_poisons -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }
};

// Venomous Wound ===========================================================

struct venomous_wound_t : public rogue_poison_t
{
  venomous_wound_t( rogue_t* p ) :
    rogue_poison_t( "venomous_wound", p, p -> find_spell( 79136 ) )
  {
    background       = true;
    proc             = true;
  }

  double composite_da_multiplier( const action_state_t* state ) const
  {
    double m = rogue_poison_t::composite_da_multiplier( state );

    m *= 1.0 + p() -> sets.set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();

    return m;
  }
};

// Deadly Poison ============================================================

struct deadly_poison_t : public rogue_poison_t
{
  struct deadly_poison_dd_t : public rogue_poison_t
  {
    deadly_poison_dd_t( rogue_t* p ) :
      rogue_poison_t( "deadly_poison_instant", p, p -> find_spell( 113780 ) )
    {
      harmful          = true;
    }
  };

  struct deadly_poison_dot_t : public rogue_poison_t
  {
    deadly_poison_dot_t( rogue_t* p ) :
      rogue_poison_t( "deadly_poison_dot", p, p -> find_class_spell( "Deadly Poison" ) -> effectN( 1 ).trigger() )
    {
      may_crit       = false;
      harmful        = true;
      tick_may_crit  = true;
      dot_behavior   = DOT_REFRESH;
    }
  };

  deadly_poison_dd_t*  proc_instant;
  deadly_poison_dot_t* proc_dot;

  deadly_poison_t( rogue_t* player ) :
    rogue_poison_t( "deadly_poison", player, player -> find_class_spell( "Deadly Poison" ) ),
    proc_instant( 0 ), proc_dot( 0 )
  {
    dual = true;
    may_miss = may_crit = false;

    proc_instant = new deadly_poison_dd_t( player );
    proc_dot     = new deadly_poison_dot_t( player );
  }

  virtual void impact( action_state_t* state )
  {
    bool is_up = ( td( state -> target ) -> dots.deadly_poison -> is_ticking() != 0 );

    rogue_poison_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      double chance = proc_chance;
      if ( p() -> buffs.envenom -> up() )
        chance += p() -> buffs.envenom -> data().effectN( 2 ).percent();

      if ( rng().roll( chance ) )
      {
        proc_dot -> target = state -> target;
        proc_dot -> execute();
        if ( is_up )
        {
          proc_instant -> target = state -> target;
          proc_instant -> execute();
        }
      }
    }
  }
};

// Wound Poison =============================================================

struct wound_poison_t : public rogue_poison_t
{
  struct wound_poison_dd_t : public rogue_poison_t
  {
    wound_poison_dd_t( rogue_t* p ) :
      rogue_poison_t( "wound_poison", p, p -> find_class_spell( "Wound Poison" ) -> effectN( 1 ).trigger() )
    {
      harmful          = true;
    }

    void impact( action_state_t* state )
    {
      rogue_poison_t::impact( state );

      if ( result_is_hit( state -> result ) )
      {
        td( state -> target ) -> debuffs.wound_poison -> trigger();

        if ( ! sim -> overrides.mortal_wounds )
          state -> target -> debuffs.mortal_wounds -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );
      }
    }
  };

  wound_poison_dd_t* proc_dd;

  wound_poison_t( rogue_t* player ) :
    rogue_poison_t( "wound_poison_driver", player, player -> find_class_spell( "Wound Poison" ) )
  {
    dual           = true;
    may_miss = may_crit = false;

    proc_dd = new wound_poison_dd_t( player );
  }

  void impact( action_state_t* state )
  {
    rogue_poison_t::impact( state );

    double chance = proc_chance;
    if ( p() -> buffs.envenom -> up() )
      chance += p() -> buffs.envenom -> data().effectN( 2 ).percent();

    if ( rng().roll( chance ) )
    {
      proc_dd -> target = state -> target;
      proc_dd -> execute();
    }
  }
};

// Apply Poison =============================================================

struct apply_poison_t : public action_t
{
  enum poison_e
  {
    POISON_NONE = 0,
    DEADLY_POISON,
    WOUND_POISON,
    CRIPPLING_POISON,
    MINDNUMBING_POISON,
    LEECHING_POISON,
    PARALYTIC_POISON
  };
  poison_e lethal_poison;
  poison_e nonlethal_poison;
  bool executed;

  apply_poison_t( rogue_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "apply_poison", p ),
    lethal_poison( POISON_NONE ), nonlethal_poison( POISON_NONE ),
    executed( false )
  {
    std::string lethal_str;
    std::string nonlethal_str;

    option_t options[] =
    {
      opt_string( "lethal", lethal_str ),
      opt_string( "nonlethal", nonlethal_str ),
      opt_null()
    };
    parse_options( options, options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;

    if ( p -> main_hand_weapon.type != WEAPON_NONE || p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( lethal_str == "deadly"    ) lethal_poison = DEADLY_POISON;
      if ( lethal_str == "wound"     ) lethal_poison = WOUND_POISON;
    }

    if ( ! p -> active_lethal_poison )
    {
      if ( lethal_poison == DEADLY_POISON ) p -> active_lethal_poison = new deadly_poison_t( p );
      if ( lethal_poison == WOUND_POISON  ) p -> active_lethal_poison = new wound_poison_t( p );
    }
  }

  virtual void execute()
  {
    executed = true;

    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", player -> name(), name() );
  }

  virtual bool ready()
  {
    return ! executed;
  }
};

} // end namespace poisons

namespace buffs {
// ==========================================================================
// Buffs
// ==========================================================================

struct fof_fod_t : public buff_t
{
  fof_fod_t( rogue_t* p ) :
    buff_t( buff_creator_t( p, "legendary_daggers" ).duration( timespan_t::from_seconds( 6.0 ) ).cd( timespan_t::zero() ) )
  { }

  virtual void expire_override()
  {
    buff_t::expire_override();

    rogue_t* p = debug_cast< rogue_t* >( player );
    p -> buffs.fof_p3 -> expire();
  }
};

struct bandits_guile_t : public buff_t
{
  bandits_guile_t( rogue_t* p ) :
    buff_t( buff_creator_t( p, "bandits_guile" )
            .quiet( true )
            .max_stack( 12 )
            .duration( p -> find_spell( 84745 ) -> duration() )
            .chance( p -> find_specialization_spell( "Bandit's Guile" ) -> proc_chance() /* 0 */ ) )
  { }

  void execute( int stacks = 1, double value = buff_t::DEFAULT_VALUE(), timespan_t duration = timespan_t::min() )
  {
    rogue_t* p = debug_cast< rogue_t* >( player );

    if ( current_stack < max_stack() )
      buff_t::execute( stacks, value, duration );

    if ( current_stack == 4 )
      p -> buffs.shallow_insight -> trigger();
    else if ( current_stack == 8 )
    {
      p -> buffs.shallow_insight -> expire();
      p -> buffs.moderate_insight -> trigger();
    }
    else if ( current_stack == 12 )
    {
      if ( p -> buffs.deep_insight -> check() )
        return;

      p -> buffs.moderate_insight -> expire();
      p -> buffs.deep_insight -> trigger();
    }
  }

  void expire_override()
  {
    rogue_t* p = debug_cast< rogue_t* >( player );

    buff_t::expire_override();

    p -> buffs.shallow_insight -> expire();
    p -> buffs.moderate_insight -> expire();
    p -> buffs.deep_insight -> expire();
  }
};

struct shadow_blades_t : public buff_t
{
  shadow_blades_t( rogue_t* p ) :
    buff_t( buff_creator_t( p, "shadow_blades" ) )
  {
    buff_duration = p -> find_class_spell( "Shadow Blades" ) -> duration();
    if ( p -> spec.restless_blades -> ok() )
      buff_duration += timespan_t::from_seconds( p -> sets.set( SET_T14_4PC_MELEE ) -> effectN( 1 ).base_value() / 2 );
    else
      buff_duration += timespan_t::from_seconds( p -> sets.set( SET_T14_4PC_MELEE ) -> effectN( 1 ).base_value() );

    // Cooldown of the Shadow blades buff is handled by the action
    cooldown -> duration = timespan_t::zero();
  }

  void change_auto_attack( attack_t*& hand, attack_t* a )
  {
    if ( hand == 0 )
      return;

    bool executing = hand -> execute_event != 0;
    timespan_t time_to_hit = timespan_t::zero();

    if ( executing )
    {
      time_to_hit = hand -> execute_event -> occurs() - sim -> current_time;
      core_event_t::cancel( hand -> execute_event );
    }

    hand = a;

    // Kick off the new attack, by instantly scheduling and rescheduling it to
    // the remaining time to hit. We cannot use normal reschedule mechanism
    // here (i.e., simply use event_t::reschedule() and leave it be), because
    // the rescheduled event would be triggered before the full swing time
    // (of the new auto attack) in most cases.
    if ( executing )
    {
      timespan_t old_swing_time = hand -> base_execute_time;
      hand -> base_execute_time = timespan_t::zero();
      hand -> schedule_execute();
      hand -> base_execute_time = old_swing_time;
      hand -> execute_event -> reschedule( time_to_hit );
    }
  }

  void execute( int stacks = 1, double value = buff_t::DEFAULT_VALUE(), timespan_t duration = timespan_t::min() )
  {
    buff_t::execute( stacks, value, duration );

    rogue_t* p = debug_cast< rogue_t* >( player );
    change_auto_attack( p -> main_hand_attack, p -> shadow_blade_main_hand );
    if ( p -> off_hand_attack )
      change_auto_attack( p -> off_hand_attack, p -> shadow_blade_off_hand );
  }

  void expire_override()
  {
    buff_t::expire_override();

    rogue_t* p = debug_cast< rogue_t* >( player );
    change_auto_attack( p -> main_hand_attack, p -> melee_main_hand );
    if ( p -> off_hand_attack )
      change_auto_attack( p -> off_hand_attack, p -> melee_off_hand );
  }
};

struct subterfuge_t : public buff_t
{
  rogue_t* rogue;

  subterfuge_t( rogue_t* r ) :
    buff_t( buff_creator_t( r, "subterfuge", r -> find_spell( 115192 ) ) ),
    rogue( r )
  { }

  void expire_override()
  {
    buff_t::expire_override();

    if ( ! rogue -> bugs || ( rogue -> bugs && ( ! rogue -> glyph.vanish -> ok() || ! rogue -> buffs.vanish -> check() ) ) )
      actions::break_stealth( rogue );
  }
};

} // end namespace buffs


// ==========================================================================
// Combo Point System Functions
// ==========================================================================

combo_points_t::combo_points_t( rogue_t* source, player_t* target ) :
  source( source ),
  target( target ),
  proc( 0 ),
  wasted( 0 ),
  count( 0 ),
  anticipation_charges( 0 )
{
  proc   = target -> get_proc( source -> name_str + ": combo_points" );
  wasted = target -> get_proc( source -> name_str + ": combo_points_wasted" );
  proc_anticipation = target -> get_proc( source -> name_str + ": anticipation_charges" );
  wasted_anticipation = target -> get_proc( source -> name_str + ": anticipation_charges_wasted" );
}

void combo_points_t::add( int num, const char* action )
{
  int actual_num = clamp( num, 0, max_combo_points - count );
  int overflow   = num - actual_num;
  int anticipation_num = 0;

  // Premeditation cancel on any added combo points, even if they are
  // already full
  rogue_t* p = source;
  if ( num > 0 && p -> event_premeditation )
    core_event_t::cancel( p -> event_premeditation );

  // we count all combo points gained in the proc
  for ( int i = 0; i < num; i++ )
    proc -> occur();

  // add actual combo points
  if ( actual_num > 0 )
  {
    count += actual_num;
    source -> trigger_ready();
  }

  // count wasted combo points
  if ( overflow > 0 )
  {
    // Anticipation
    if ( p -> talent.anticipation -> ok() )
    {
      int anticipation_overflow = 0;
      anticipation_num = clamp( overflow, 0, max_combo_points - anticipation_charges );
      anticipation_overflow = overflow - anticipation_num;

      for ( int i = 0; i < overflow; i++ )
        proc_anticipation -> occur();

      if ( anticipation_num > 0 )
        anticipation_charges += anticipation_num;

      if ( anticipation_overflow > 0 )
      {
        for ( int i = 0; i < anticipation_overflow; i++ )
          wasted_anticipation -> occur();
      }
    }
    else
    {
      for ( int i = 0; i < overflow; i++ )
        wasted -> occur();
    }
  }

  if ( source -> sim -> log )
  {
    if ( action )
    {
      if ( actual_num > 0 )
        source -> sim -> out_log.printf( "%s gains %d (%d) combo_points from %s (%d)",
                                 target -> name(), actual_num, num, action, count );
      if ( anticipation_num > 0 )
        source -> sim -> out_log.printf( "%s gains %d (%d) anticipation_charges from %s (%d)",
                                 target -> name(), anticipation_num, overflow, action, anticipation_charges );
    }
    else
    {
      if ( actual_num > 0 )
        source -> sim -> out_log.printf( "%s gains %d (%d) combo_points (%d)",
                                 target -> name(), actual_num, num, count );
      if ( anticipation_num > 0 )
        source -> sim -> out_log.printf( "%s gains %d (%d) anticipation_charges (%d)",
                                 target -> name(), anticipation_num, overflow, anticipation_charges );
    }
  }

  assert( ( count == 5 && anticipation_charges >= 0 ) || ( count < 5 && anticipation_charges == 0 ) );
}

void combo_points_t::clear( const char* action, bool anticipation )
{
  if ( source -> sim -> log )
  {
    if ( action )
      source -> sim -> out_log.printf( "%s spends %d combo_points on %s",
                               target -> name(), count, action );
    else
      source -> sim -> out_log.printf( "%s loses %d combo_points",
                               target -> name(), count );
  }

  source -> trigger_ready();

  // Premeditation cancels when you use the combo points
  rogue_t* p = source;
  if ( p -> event_premeditation )
    core_event_t::cancel( p -> event_premeditation );

  count = 0;
  if ( anticipation )
    anticipation_charges = 0;

  if ( anticipation_charges > 0 )
  {
    if ( source -> sim -> log )
      source -> sim -> out_log.printf( "%s replenishes %d combo_points through anticipation", source -> name(), anticipation_charges );
    count = anticipation_charges;
    anticipation_charges = 0;
  }

  assert( ( count == 5 && anticipation_charges >= 0 ) || ( count < 5 && anticipation_charges == 0 ) );
}

// ==========================================================================
// Rogue Targetdata Definitions
// ==========================================================================

rogue_td_t::rogue_td_t( player_t* target, rogue_t* source ) :
  actor_pair_t( target, source ),
  dots( dots_t() ),
  debuffs( debuffs_t() ),
  combo_points( combo_points_t( source, target ) )
{

  dots.crimson_tempest  = target -> get_dot( "crimson_tempest_dot", source );
  dots.deadly_poison    = target -> get_dot( "deadly_poison_dot", source );
  dots.garrote          = target -> get_dot( "garrote", source );
  dots.rupture          = target -> get_dot( "rupture", source );
  dots.hemorrhage       = target -> get_dot( "hemorrhage", source );
  dots.killing_spree    = target -> get_dot( "killing_spree", source );
  dots.revealing_strike = target -> get_dot( "revealing_strike", source );

  const spell_data_t* fw = source -> find_specialization_spell( "Find Weakness" );
  const spell_data_t* fwt = fw -> effectN( 1 ).trigger();
  debuffs.find_weakness = buff_creator_t( *this, "find_weakness", fwt )
                          .default_value( fw -> effectN( 1 ).percent() )
                          .chance( fw -> proc_chance() );

  const spell_data_t* vd = source -> find_specialization_spell( "Vendetta" );
  debuffs.vendetta =           buff_creator_t( *this, "vendetta", vd )
                               .cd( timespan_t::zero() )
                               .duration ( vd -> duration() +
                                           source -> sets.set( SET_T13_4PC_MELEE ) -> effectN( 3 ).time_value() +
                                           source -> glyph.vendetta -> effectN( 2 ).time_value() )
                               .default_value( vd -> effectN( 1 ).percent() + source -> glyph.vendetta -> effectN( 1 ).percent() );

  debuffs.wound_poison = buff_creator_t( *this, "wound_poison", source -> find_spell( 8680 ) );
}

// ==========================================================================
// Rogue Character Definition
// ==========================================================================

// rogue_t::composite_attribute_multiplier ==================================

double rogue_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_AGILITY )
    m *= 1.0 + spec.sinister_calling -> effectN( 1 ).percent();

  return m;
}

// rogue_t::composite_attack_speed ==========================================

double rogue_t::composite_melee_speed() const
{
  double h = player_t::composite_melee_speed();

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

  m *= 1.0 + spec.vitality -> effectN( 2 ).percent();

  return m;
}

// rogue_t::composite_player_multiplier =====================================

double rogue_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buffs.master_of_subtlety -> check() ||
       ( spec.master_of_subtlety -> ok() && ( buffs.stealth -> check() || buffs.vanish -> check() ) ) )
    m *= 1.0 + spec.master_of_subtlety -> effectN( 1 ).percent();

  m *= 1.0 + buffs.shallow_insight -> value();

  m *= 1.0 + buffs.moderate_insight -> value();

  m *= 1.0 + buffs.deep_insight -> value();

  if ( main_hand_weapon.type == WEAPON_DAGGER && off_hand_weapon.type == WEAPON_DAGGER )
    m *= 1.0 + spec.assassins_resolve -> effectN( 2 ).percent();

  return m;
}

// rogue_t::init_actions ====================================================

void rogue_t::init_action_list()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( ! action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  // Note, this only looks at static stats
  stat_e highest_rune_stat = STAT_NONE;
  if ( find_item( "rune_of_reorigination" ) )
  {
    if ( gear.get_stat( STAT_CRIT_RATING ) >= gear.get_stat( STAT_HASTE_RATING ) )
    {
      if ( gear.get_stat( STAT_CRIT_RATING ) >= gear.get_stat( STAT_MASTERY_RATING ) )
        highest_rune_stat = STAT_CRIT_RATING;
      else
        highest_rune_stat = STAT_MASTERY_RATING;
    }
    else if ( gear.get_stat( STAT_HASTE_RATING ) >= gear.get_stat( STAT_MASTERY_RATING ) )
      highest_rune_stat = STAT_HASTE_RATING;
    else
      highest_rune_stat = STAT_MASTERY_RATING;
  }

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* def       = get_action_priority_list( "default"   );

  std::vector<std::string> item_actions = get_item_actions();
  std::vector<std::string> profession_actions = get_profession_actions();
  std::vector<std::string> racial_actions = get_racial_actions();

  clear_action_priority_lists();

  // Flask
  if ( sim -> allow_flasks && level >= 80 )
  {
    std::string flask_action = "flask,type=";
    flask_action += ( level > 85 ) ? "spring_blossoms" : "winds";
    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food && level >= 80 )
  {
    std::string food_action = "food,type=";
    food_action += ( level > 85 ) ? "sea_mist_rice_noodles" : "seafood_magnifique_feast";
    precombat -> add_action( food_action );
  }

  // Lethal poison
  precombat -> add_action( "apply_poison,lethal=deadly" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  if ( sim -> allow_potions && level >= 80 )
    precombat -> add_action( ( level > 85 ) ? "potion,name=virmens_bite" : "potion,name=tolvir" );

  precombat -> add_action( this, "Stealth" );

  // In-combat potion
  if ( sim -> allow_potions )
  {
    std::string potion_str = ( level > 85 ) ? "potion,name=virmens_bite" : "potion,name=tolvir";
    potion_str += ",if=buff.bloodlust.react|target.time_to_die<40";

    def -> add_action( potion_str );
  }

  def -> add_action( "auto_attack" );
  def -> add_action( this, "Kick" );

  if ( specialization() == ROGUE_ASSASSINATION )
  {
    precombat -> add_talent( this, "Marked for Death" );
    precombat -> add_action( this, "Slice and Dice", "if=talent.marked_for_death.enabled" );

    def -> add_action( this, "Preparation", "if=!buff.vanish.up&cooldown.vanish.remains>60" );

    for ( size_t i = 0; i < item_actions.size(); i++ )
      def -> add_action( item_actions[ i ] );

    for ( size_t i = 0; i < profession_actions.size(); i++ )
      def -> add_action( profession_actions[ i ] );

    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[ i ] == "arcane_torrent" )
        def -> add_action( racial_actions[ i ] + ",if=energy<60" );
      else
        def -> add_action( racial_actions[ i ] );
    }

    std::string vanish_expr = "if=time>10&!buff.stealth.up";
    if ( level >= 87 ) vanish_expr += "&!buff.shadow_blades.up";
    def -> add_action( this, "Vanish", vanish_expr );

    def -> add_action( this, "Mutilate", "if=buff.stealth.up" );
    def -> add_action( this, "Shadow Blades", "if=buff.bloodlust.react|time>60" );
    def -> add_action( this, "Slice and Dice", "if=buff.slice_and_dice.remains<2" );

    def -> add_action( this, "Dispatch", "if=dot.rupture.ticks_remain<2&energy>90" );
    def -> add_action( this, "Mutilate", "if=dot.rupture.ticks_remain<2&energy>90" );

    def -> add_talent( this, "Marked for Death", "if=combo_points=0" );

    def -> add_action( this, "Rupture", "if=ticks_remain<2|(combo_points=5&ticks_remain<3)" );
    def -> add_action( this, "Fan of Knives", "if=combo_points<5&active_enemies>=4" );

    def -> add_action( this, "Vendetta" );

    def -> add_action( this, "Envenom", "if=combo_points>4" );
    def -> add_action( this, "Envenom", "if=combo_points>=2&buff.slice_and_dice.remains<3" );

    def -> add_action( this, "Dispatch", "if=combo_points<5" );
    def -> add_action( this, "Mutilate" );

    if ( ! sim -> solo_raid )
      def -> add_action( this, "Tricks of the Trade" );
  }
  // Action list from http://sites.google.com/site/bittensspellflash/simc-profiles
  else if ( specialization() == ROGUE_COMBAT )
  {
    precombat -> add_talent( this, "Marked for Death" );
    precombat -> add_action( this, "Slice and Dice", "if=talent.marked_for_death.enabled" );

    def -> add_action( this, "Preparation", "if=!buff.vanish.up&cooldown.vanish.remains>60" );

    for ( size_t i = 0; i < item_actions.size(); i++ )
      def -> add_action( item_actions[ i ] + ",if=time=0|buff.shadow_blades.up" );

    for ( size_t i = 0; i < profession_actions.size(); i++ )
      def -> add_action( profession_actions[ i ] + ",if=time=0|buff.shadow_blades.up" );

    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[ i ] == "arcane_torrent" )
        def -> add_action( racial_actions[ i ] + ",if=energy<60" );
      else
        def -> add_action( racial_actions[ i ] + ",if=time=0|buff.shadow_blades.up" );
    }

    def -> add_action( this, "Blade Flurry", "if=(active_enemies>=2&!buff.blade_flurry.up)|(active_enemies<2&buff.blade_flurry.up)" );

    def -> add_action( this, "Ambush" );
    def -> add_action( this, "Vanish", "if=time>10&(combo_points<3|(talent.anticipation.enabled&anticipation_charges<3)|(buff.shadow_blades.down&(combo_points<4|(talent.anticipation.enabled&anticipation_charges<4))))&((talent.shadow_focus.enabled&buff.adrenaline_rush.down&energy<20)|(talent.subterfuge.enabled&energy>=90)|(!talent.shadow_focus.enabled&!talent.subterfuge.enabled&energy>=60))" );

    // Cooldowns (No Tier14)
    def -> add_action( this, "Killing Spree", "if=energy<50" );
    def -> add_action( this, "Shadow Blades", "if=time>5" );
    def -> add_action( this, "Adrenaline Rush", "if=energy<35|buff.shadow_blades.up" );

    // Rotation
    def -> add_action( this, "Slice and Dice", "if=buff.slice_and_dice.remains<2|(buff.slice_and_dice.remains<15&buff.bandits_guile.stack=11&combo_points>=4)" );

    def -> add_talent( this, "Marked for Death", "if=combo_points<=1&dot.revealing_strike.ticking" );

    // Generate combo points, or use combo points
    def -> add_action( "run_action_list,name=generator,if=combo_points<5|(talent.anticipation.enabled&anticipation_charges<=4&!dot.revealing_strike.ticking)" );
    if ( level >= 3 )
      def -> add_action( "run_action_list,name=finisher,if=!talent.anticipation.enabled|buff.deep_insight.up|cooldown.shadow_blades.remains<=11|anticipation_charges>=4|(buff.shadow_blades.up&anticipation_charges>=3)" );
    def -> add_action( "run_action_list,name=generator,if=energy>60|buff.deep_insight.down|buff.deep_insight.remains>5-combo_points" );

    // Combo point generators
    action_priority_list_t* gen = get_action_priority_list( "generator", "Combo point generators" );
    gen -> add_action( this, "Fan of Knives", "line_cd=5,if=active_enemies>=4" );
    gen -> add_action( this, "Revealing Strike", "if=ticks_remain<2" );
    gen -> add_action( this, "Sinister Strike" );

    // Combo point finishers
    action_priority_list_t* finisher = get_action_priority_list( "finisher", "Combo point finishers" );
    finisher -> add_action( this, "Rupture", "if=ticks_remain<2&target.time_to_die>=26&(active_enemies<2|!buff.blade_flurry.up)" );
    finisher -> add_action( this, "Crimson Tempest", "if=active_enemies>=7&dot.crimson_tempest_dot.ticks_remain<=2" );
    finisher -> add_action( this, "Eviscerate" );
  }
  else if ( specialization() == ROGUE_SUBTLETY )
  {
    precombat -> add_action( this, "Premeditation" );
    precombat -> add_action( this, "Slice and Dice" );

    for ( size_t i = 0; i < item_actions.size(); i++ )
      def -> add_action( item_actions[ i ] + ",if=buff.shadow_dance.up" );

    for ( size_t i = 0; i < profession_actions.size(); i++ )
      def -> add_action( profession_actions[ i ] + ",if=buff.shadow_dance.up" );

    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[ i ] == "arcane_torrent" )
        def -> add_action( racial_actions[ i ] + ",if=energy<60" );
      else
        def -> add_action( racial_actions[ i ] + ",if=buff.shadow_dance.up" );
    }

    if ( highest_rune_stat != STAT_MASTERY_RATING )
      def -> add_action( this, "Shadow Blades" );
    else
      def -> add_action( this, "Shadow Blades", "if=buff.rune_of_reorigination.down" );

    // Shadow Dancing and Vanishing and Marking for the Deathing
    def -> add_action( this, "Premeditation", "if=combo_points<=4" );
    def -> add_action( this, find_class_spell( "Ambush" ), "pool_resource", "for_next=1" );
    def -> add_action( this, "Ambush", "if=combo_points<5|(talent.anticipation.enabled&anticipation_charges<3)|(buff.sleight_of_hand.up&buff.sleight_of_hand.remains<=gcd)" );
    def -> add_action( this, find_class_spell( "Shadow Dance" ), "pool_resource", "for_next=1,extra_amount=75" );
    def -> add_action( this, "Shadow Dance", "if=energy>=75&buff.stealth.down&buff.vanish.down&debuff.find_weakness.down" );
    def -> add_action( this, find_class_spell( "Vanish" ), "pool_resource", "for_next=1,extra_amount=45" );
    def -> add_action( this, "Vanish", "if=energy>=45&energy<=75&combo_points<=3&buff.shadow_dance.down&buff.master_of_subtlety.down&debuff.find_weakness.down" );
    def -> add_talent( this, "Marked for Death", "if=combo_points=0" );

    // Rotation
    def -> add_action( "run_action_list,name=generator,if=talent.anticipation.enabled&anticipation_charges<4&buff.slice_and_dice.up&dot.rupture.remains>2&(buff.slice_and_dice.remains<6|dot.rupture.remains<4)" );
    if ( highest_rune_stat != STAT_MASTERY_RATING )
      def -> add_action( "run_action_list,name=finisher,if=combo_points=5" );
    else
      def -> add_action( "run_action_list,name=finisher,if=combo_points=5|(buff.rune_of_reorigination.react&combo_points>=4)" );
    def -> add_action( "run_action_list,name=generator,if=combo_points<4|energy>80|talent.anticipation.enabled" );
    def -> add_action( "run_action_list,name=pool" );

    // Combo point generators
    action_priority_list_t* gen = get_action_priority_list( "generator", "Combo point generators" );
    gen -> add_action( this, find_class_spell( "Preparation" ), "run_action_list", "name=pool,if=buff.master_of_subtlety.down&buff.shadow_dance.down&debuff.find_weakness.down&(energy+cooldown.shadow_dance.remains*energy.regen<80|energy+cooldown.vanish.remains*energy.regen<60)" );
    gen -> add_action( this, "Fan of Knives", "if=active_enemies>=4" );
    gen -> add_action( this, "Hemorrhage", "if=remains<3|position_front" );
    gen -> add_talent( this, "Shuriken Toss", "if=energy<65&energy.regen<16" );
    gen -> add_action( this, "Backstab" );
    gen -> add_action( this, find_class_spell( "Preparation" ), "run_action_list", "name=pool" );

    // Combo point finishers
    action_priority_list_t* finisher = get_action_priority_list( "finisher", "Combo point finishers" );
    if ( highest_rune_stat != STAT_MASTERY_RATING )
    {
      finisher -> add_action( this, "Slice and Dice", "if=buff.slice_and_dice.remains<4" );
      finisher -> add_action( this, "Rupture", "if=ticks_remain<2&active_enemies<3" );
    }
    else
    {
      finisher -> add_action( this, "Slice and Dice", "if=buff.slice_and_dice.remains<4|(buff.rune_of_reorigination.react&buff.slice_and_dice.remains<25)" );
      finisher -> add_action( this, "Rupture", "if=ticks_remain<2&active_enemies<3|(buff.rune_of_reorigination.react&ticks_remain<7)" );
    }
  finisher -> add_action( this, "Crimson Tempest", "if=(active_enemies>1&dot.crimson_tempest_dot.ticks_remain<=2&combo_points=5)|active_enemies>=5" );
  finisher -> add_action( this, "Eviscerate", "if=active_enemies<4|(active_enemies>3&dot.crimson_tempest_dot.ticks_remain>=2)" );
    finisher -> add_action( this, find_class_spell( "Preparation" ), "run_action_list", "name=pool" );

    // Resource pooling
    action_priority_list_t* pool = get_action_priority_list( "pool", "Resource pooling" );
    pool -> add_action( this, "Preparation", "if=!buff.vanish.up&cooldown.vanish.remains>60" );
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

// rogue_t::create_action  ==================================================

action_t* rogue_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  using namespace actions;
  using namespace poisons;

  if ( name == "auto_attack"         ) return new auto_melee_attack_t  ( this, options_str );
  if ( name == "adrenaline_rush"     ) return new adrenaline_rush_t    ( this, options_str );
  if ( name == "ambush"              ) return new ambush_t             ( this, options_str );
  if ( name == "apply_poison"        ) return new apply_poison_t       ( this, options_str );
  if ( name == "backstab"            ) return new backstab_t           ( this, options_str );
  if ( name == "blade_flurry"        ) return new blade_flurry_t       ( this, options_str );
  if ( name == "crimson_tempest"     ) return new crimson_tempest_t    ( this, options_str );
  if ( name == "dispatch"            ) return new dispatch_t           ( this, options_str );
  if ( name == "envenom"             ) return new envenom_t            ( this, options_str );
  if ( name == "eviscerate"          ) return new eviscerate_t         ( this, options_str );
  if ( name == "expose_armor"        ) return new expose_armor_t       ( this, options_str );
  if ( name == "fan_of_knives"       ) return new fan_of_knives_t      ( this, options_str );
  if ( name == "garrote"             ) return new garrote_t            ( this, options_str );
  if ( name == "hemorrhage"          ) return new hemorrhage_t         ( this, options_str );
  if ( name == "kick"                ) return new kick_t               ( this, options_str );
  if ( name == "killing_spree"       ) return new killing_spree_t      ( this, options_str );
  if ( name == "marked_for_death"    ) return new marked_for_death_t   ( this, options_str );
  if ( name == "mutilate"            ) return new mutilate_t           ( this, options_str );
  if ( name == "premeditation"       ) return new premeditation_t      ( this, options_str );
  if ( name == "preparation"         ) return new preparation_t        ( this, options_str );
  if ( name == "recuperate"          ) return new recuperate_t         ( this, options_str );
  if ( name == "revealing_strike"    ) return new revealing_strike_t   ( this, options_str );
  if ( name == "rupture"             ) return new rupture_t            ( this, options_str );
  if ( name == "shadow_blades"       ) return new shadow_blades_t      ( this, options_str );
  if ( name == "shadow_dance"        ) return new shadow_dance_t       ( this, options_str );
  if ( name == "shadowstep"          ) return new shadowstep_t         ( this, options_str );
  if ( name == "shiv"                ) return new shiv_t               ( this, options_str );
  if ( name == "shuriken_toss"       ) return new shuriken_toss_t      ( this, options_str );
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
        action( *a )
      {}

      virtual double evaluate()
      {
        rogue_t* p = debug_cast<rogue_t*>( action.player );
        rogue_td_t* td = p -> get_target_data( action.target );
        return td -> combo_points.count;
      }
    };
    return new combo_points_expr_t( a );
  }
  if ( util::str_compare_ci( name_str, "anticipation_charges" ) )
  {
    struct anticipation_charges_expr_t : public expr_t
    {
      const action_t& action;
      anticipation_charges_expr_t( action_t* a ) : expr_t( "anticipation_charges" ),
        action( *a )
      {}

      virtual double evaluate()
      {
        rogue_t* p = debug_cast<rogue_t*>( action.player );
        rogue_td_t* td = p -> get_target_data( action.target );
        return td -> combo_points.anticipation_charges;
      }
    };
    return new anticipation_charges_expr_t( a );
  }

  return player_t::create_expression( a, name_str );
}

// rogue_t::init_base =======================================================

void rogue_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;

  resources.base[ RESOURCE_ENERGY ] = 100;
  if ( main_hand_weapon.type == WEAPON_DAGGER && off_hand_weapon.type == WEAPON_DAGGER )
    resources.base[ RESOURCE_ENERGY ] += spec.assassins_resolve -> effectN( 1 ).base_value();
  if ( sets.has_set_bonus( SET_PVP_2PC_MELEE ) )
    resources.base[ RESOURCE_ENERGY ] += 10;

  base_energy_regen_per_second = 10 * ( 1.0 + spec.vitality -> effectN( 1 ).percent() );

  base_gcd = timespan_t::from_seconds( 1.0 );

}

// rogue_t::init_spells =====================================================

void rogue_t::init_spells()
{
  player_t::init_spells();

  // Assassination
  spec.assassins_resolve    = find_specialization_spell( "Assassin's Resolve" );
  spec.improved_poisons     = find_specialization_spell( "Improved Poisons" );
  spec.seal_fate            = find_specialization_spell( "Seal Fate" );
  spec.venomous_wounds      = find_specialization_spell( "Venomous Wounds" );
  spec.cut_to_the_chase     = find_specialization_spell( "Cut to the Chase" );
  spec.blindside            = find_specialization_spell( "Blindside" );

  // Combat
  spec.ambidexterity        = find_specialization_spell( "Ambidexterity" );
  spec.blade_flurry         = find_specialization_spell( "Blade Flurry" );
  spec.vitality             = find_specialization_spell( "Vitality" );
  spec.combat_potency       = find_specialization_spell( "Combat Potency" );
  spec.restless_blades      = find_specialization_spell( "Restless Blades" );
  spec.bandits_guile        = find_specialization_spell( "Bandit's Guile" );
  spec.killing_spree        = find_specialization_spell( "Killing Spree" );
  spec.ruthlessness         = find_specialization_spell( "Ruthlessness" );

  // Subtlety
  spec.master_of_subtlety   = find_specialization_spell( "Master of Subtlety" );
  spec.sinister_calling     = find_specialization_spell( "Sinister Calling" );
  spec.find_weakness        = find_specialization_spell( "Find Weakness" );
  spec.honor_among_thieves  = find_specialization_spell( "Honor Among Thieves" );
  spec.sanguinary_vein      = find_specialization_spell( "Sanguinary Vein" );
  spec.energetic_recovery   = find_specialization_spell( "Energetic Recovery" );
  spec.shadow_dance         = find_specialization_spell( "Shadow Dance" );

  // Masteries
  mastery.potent_poisons    = find_mastery_spell( ROGUE_ASSASSINATION );
  mastery.main_gauche       = find_mastery_spell( ROGUE_COMBAT );
  mastery.executioner       = find_mastery_spell( ROGUE_SUBTLETY );

  // Misc spells
  spell.relentless_strikes  = find_spell( 58423 );
  spell.ruthlessness_cp     = spec.ruthlessness -> effectN( 1 ).trigger();
  spell.shadow_focus        = find_spell( 112942 );
  spell.tier13_2pc          = find_spell( 105864 );
  spell.tier13_4pc          = find_spell( 105865 );
  spell.tier15_4pc          = find_spell( 138151 );
  spell.bandits_guile_value = find_spell( 84747 );

  // Glyphs
  glyph.adrenaline_rush     = find_glyph_spell( "Glyph of Adrenaline Rush" );
  glyph.expose_armor        = find_glyph_spell( "Glyph of Expose Armor" );
  glyph.hemorrhaging_veins  = find_glyph_spell( "Glyph of Hemorrhaging Veins" );
  glyph.kick                = find_glyph_spell( "Glyph of Kick" );
  glyph.sharp_knives        = find_glyph_spell( "Glyph of Sharp Knives" );
  glyph.vanish              = find_glyph_spell( "Glyph of Vanish" );
  glyph.vendetta            = find_glyph_spell( "Glyph of Vendetta" );

  talent.nightstalker       = find_talent_spell( "Nightstalker" );
  talent.subterfuge         = find_talent_spell( "Subterfuge" );
  talent.shadow_focus       = find_talent_spell( "Shadow Focus" );
  talent.marked_for_death   = find_talent_spell( "Marked for Death" );
  talent.anticipation       = find_talent_spell( "Anticipation" );

  if ( spec.venomous_wounds -> ok() )
  {
    active_venomous_wound = new poisons::venomous_wound_t( this );
  }

  static const set_bonus_description_t set_bonuses =
  {
    //  C2P    C4P    M2P    M4P    T2P    T4P    H2P    H4P
    {     0,     0, 105849, 105865,     0,     0,     0,     0 }, // Tier13
    {     0,     0, 123116, 123122,     0,     0,     0,     0 }, // Tier14
    {     0,     0, 138148, 138150,     0,     0,     0,     0 }, // Tier15
    {     0,     0, 145185, 145210,     0,     0,     0,     0 }, // Tier16
  };

  sets.register_spelldata( set_bonuses );
}

// rogue_t::init_gains ======================================================

void rogue_t::init_gains()
{
  player_t::init_gains();

  gains.adrenaline_rush    = get_gain( "adrenaline_rush"    );
  gains.combat_potency     = get_gain( "combat_potency"     );
  gains.energetic_recovery = get_gain( "energetic_recovery" );
  gains.energy_refund      = get_gain( "energy_refund"      );
  gains.murderous_intent   = get_gain( "murderous_intent"   );
  gains.overkill           = get_gain( "overkill"           );
  gains.recuperate         = get_gain( "recuperate"         );
  gains.relentless_strikes = get_gain( "relentless_strikes" );
  gains.venomous_wounds    = get_gain( "venomous_vim"       );
}

// rogue_t::init_procs ======================================================

void rogue_t::init_procs()
{
  player_t::init_procs();

  procs.honor_among_thieves      = get_proc( "honor_among_thieves" );
  procs.seal_fate                = get_proc( "seal_fate"           );
  procs.no_revealing_strike      = get_proc( "no_revealing_strike" );
  procs.t16_2pc_melee            = get_proc( "t16_2pc_melee"       );
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

static void energetic_recovery( buff_t* buff, int, int )
{
  rogue_t* p = debug_cast<rogue_t*>( buff -> player );

  if ( p -> spec.energetic_recovery -> ok() )
    p -> resource_gain( RESOURCE_ENERGY,
                        p -> spec.energetic_recovery -> effectN( 1 ).base_value(),
                        p -> gains.energetic_recovery );
}

void rogue_t::create_buffs()
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

  player_t::create_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  buffs.bandits_guile       = new buffs::bandits_guile_t( this );
  buffs.blade_flurry        = buff_creator_t( this, "blade_flurry", find_spell( 57142 ) );
  buffs.adrenaline_rush     = buff_creator_t( this, "adrenaline_rush", find_class_spell( "Adrenaline Rush" ) )
                              .cd( timespan_t::zero() )
                              .duration( find_class_spell( "Adrenaline Rush" ) -> duration() + sets.set( SET_T13_4PC_MELEE ) -> effectN( 2 ).time_value() )
                              .default_value( find_class_spell( "Adrenaline Rush" ) -> effectN( 2 ).percent() )
                              .add_invalidate( CACHE_ATTACK_SPEED );
  buffs.blindside           = buff_creator_t( this, "blindside", spec.blindside -> effectN( 1 ).trigger() )
                              .chance( spec.blindside -> proc_chance() );
  buffs.master_of_subtlety  = buff_creator_t( this, "master_of_subtlety", spec.master_of_subtlety )
                              .duration( timespan_t::from_seconds( 6.0 ) )
                              .default_value( spec.master_of_subtlety -> effectN( 1 ).percent() )
                              .chance( spec.master_of_subtlety -> ok() )
                              .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  // Killing spree buff has only 2 sec duration, main spell has 3, check.
  buffs.killing_spree       = buff_creator_t( this, "killing_spree", spec.killing_spree )
                              .duration( spec.killing_spree -> duration() + timespan_t::from_seconds( 0.001 ) );
  buffs.shadow_blades      = new buffs::shadow_blades_t( this );
  buffs.shadow_dance       = buff_creator_t( this, "shadow_dance", find_specialization_spell( "Shadow Dance" ) )
                             .cd( timespan_t::zero() )
                             .duration( find_specialization_spell( "Shadow Dance" ) -> duration() + sets.set( SET_T13_4PC_MELEE ) -> effectN( 1 ).time_value() );
  buffs.deadly_proc        = buff_creator_t( this, "deadly_proc" );
  buffs.shallow_insight    = buff_creator_t( this, "shallow_insight", find_spell( 84745 ) )
                             .default_value( find_spell( 84745 ) -> effectN( 1 ).percent() )
                             .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.moderate_insight   = buff_creator_t( this, "moderate_insight", find_spell( 84746 ) )
                             .default_value( find_spell( 84746 ) -> effectN( 1 ).percent() )
                             .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.deep_insight       = buff_creator_t( this, "deep_insight", find_spell( 84747 ) )
                             .default_value( find_spell( 84747 ) -> effectN( 1 ).percent() )
                             .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.recuperate         = buff_creator_t( this, "recuperate" );
  buffs.shiv               = buff_creator_t( this, "shiv" );
  buffs.sleight_of_hand    = buff_creator_t( this, "sleight_of_hand", find_spell( 145211 ) )
                             .chance( sets.set( SET_T16_4PC_MELEE ) -> effectN( 3 ).percent() );
  buffs.stealth            = buff_creator_t( this, "stealth" ).add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.subterfuge         = new buffs::subterfuge_t( this );
  buffs.t16_2pc_melee      = buff_creator_t( this, "silent_blades", find_spell( 145193 ) )
                             .chance( sets.has_set_bonus( SET_T16_2PC_MELEE ) );
  buffs.tier13_2pc         = buff_creator_t( this, "tier13_2pc", spell.tier13_2pc )
                             .chance( sets.has_set_bonus( SET_T13_2PC_MELEE ) ? 1.0 : 0 );
  buffs.tot_trigger        = buff_creator_t( this, "tricks_of_the_trade_trigger", find_spell( 57934 ) )
                             .activated( true );
  buffs.toxicologist       = stat_buff_creator_t( this, "toxicologist", find_spell( 145249 ) )
                             .chance( sets.has_set_bonus( SET_T16_4PC_MELEE ) );
  buffs.vanish             = buff_creator_t( this, "vanish", find_spell( 11327 ) )
                             .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                             .cd( timespan_t::zero() )
                             .duration( find_spell( 11327 ) -> duration() + glyph.vanish -> effectN( 1 ).time_value() );

  // Envenom is controlled by the non-harmful dot applied to player when envenom is used
  buffs.envenom            = buff_creator_t( this, "envenom", find_specialization_spell( "Envenom" ) )
                             .duration( timespan_t::min() )
                             .tick_behavior( BUFF_TICK_REFRESH );
  buffs.slice_and_dice     = buff_creator_t( this, "slice_and_dice", find_class_spell( "Slice and Dice" ) )
                             .duration( timespan_t::min() )
                             .tick_behavior( BUFF_TICK_REFRESH )
                             .tick_callback( energetic_recovery )
                             .add_invalidate( CACHE_ATTACK_SPEED );

  // Legendary buffs
  buffs.fof_p1            = stat_buff_creator_t( this, "suffering", find_spell( 109959 ) )
                            .add_stat( STAT_AGILITY, find_spell( 109959 ) -> effectN( 1 ).base_value() )
                            .chance( fof_p1 );
  buffs.fof_p2            = stat_buff_creator_t( this, "nightmare", find_spell( 109955 ) )
                            .add_stat( STAT_AGILITY, find_spell( 109955 ) -> effectN( 1 ).base_value() )
                            .chance( fof_p2 );
  buffs.fof_p3            = stat_buff_creator_t( this, "shadows_of_the_destroyer", find_spell( 109939 ) -> effectN( 1 ).trigger() )
                            .add_stat( STAT_AGILITY, find_spell( 109939 ) -> effectN( 1 ).trigger() -> effectN( 1 ).base_value() )
                            .chance( fof_p3 );

  buffs.fof_fod           = new buffs::fof_fod_t( this );
}

// trigger_honor_among_thieves ==============================================

struct honor_among_thieves_callback_t : public action_callback_t
{
  honor_among_thieves_callback_t( rogue_t* r ) : action_callback_t( r ) {}

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    rogue_t* rogue = debug_cast< rogue_t* >( listener );

    if ( ! rogue -> in_combat )
      return;

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

    if ( rogue -> cooldowns.honor_among_thieves -> down() )
      return;

    if ( ! rogue -> rng().roll( rogue -> spec.honor_among_thieves -> proc_chance() ) )
      return;

    rogue_td_t* td = rogue -> get_target_data( rogue -> target );

    td -> combo_points.add( 1, rogue -> spec.honor_among_thieves -> name_cstr() );

    rogue -> procs.honor_among_thieves -> occur();

    rogue -> cooldowns.honor_among_thieves -> start( timespan_t::from_seconds( 2.0 ) );

    if ( rogue -> buffs.t16_2pc_melee -> trigger() )
      rogue -> procs.t16_2pc_melee -> occur();
  }
};

// rogue_t::register_callbacks ==============================================

void rogue_t::register_callbacks()
{
  player_t::register_callbacks();

  if ( spec.honor_among_thieves -> ok() && virtual_hat_interval != timespan_t::zero() )
  {
    action_callback_t* cb = new honor_among_thieves_callback_t( this );

    callbacks.register_attack_callback( RESULT_CRIT_MASK, cb );
    callbacks.register_spell_callback ( RESULT_CRIT_MASK, cb );
    callbacks.register_tick_callback( RESULT_CRIT_MASK, cb );

    if ( virtual_hat_interval < timespan_t::zero() )
    {
      if ( ! sim -> solo_raid )
        virtual_hat_interval = timespan_t::from_seconds( 2.20 );
    }

    if ( virtual_hat_interval > timespan_t::zero() )
      virtual_hat_callback = cb;
    else
    {
      for ( size_t i = 0; i < sim -> player_no_pet_list.size(); i++ )
      {
        player_t* p = sim -> player_no_pet_list[ i ];

        p -> callbacks.register_attack_callback( RESULT_CRIT_MASK, cb );
        p -> callbacks.register_spell_callback ( RESULT_CRIT_MASK, cb );
        p -> callbacks.register_tick_callback( RESULT_CRIT_MASK, cb );
      }
    }
  }
}

// rogue_t::combat_begin ====================================================

void rogue_t::combat_begin()
{
  player_t::combat_begin();

  if ( spec.honor_among_thieves -> ok() && virtual_hat_interval > timespan_t::zero() )
  {
    struct virtual_hat_event_t : public event_t
    {
      action_callback_t* callback;
      timespan_t         interval;

      virtual_hat_event_t( rogue_t* p, action_callback_t* cb, timespan_t i ) :
        event_t( *p, "Virtual HAT Event" ),
        callback( cb ), interval( i )
      {
        timespan_t cooldown = timespan_t::from_seconds( 2.0 );
        timespan_t remainder = interval - cooldown;
        if ( remainder < timespan_t::zero() ) remainder = timespan_t::zero();
        timespan_t time = cooldown + p -> rng().range( remainder * 0.5, remainder * 1.5 ) + timespan_t::from_seconds( 0.01 );
        add_event( time );
      }

      virtual void execute()
      {
        rogue_t* p = debug_cast<rogue_t*>( actor );
        callback -> trigger( nullptr, nullptr );
        new ( sim() ) virtual_hat_event_t( p, callback, interval );
      }
    };

    new ( *sim ) virtual_hat_event_t( this, virtual_hat_callback, virtual_hat_interval );
  }
}

// rogue_t::reset ===========================================================

void rogue_t::reset()
{
  player_t::reset();

  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    rogue_td_t* td = target_data[ sim -> actor_list[ i ] ];
    if ( td ) td -> reset();
  }

  core_event_t::cancel( event_premeditation );
}

// rogue_t::arise ===========================================================

void rogue_t::arise()
{
  player_t::arise();

  if ( ! sim -> overrides.haste && dbc.spell( 113742 ) -> is_level( level ) ) sim -> auras.haste -> trigger();
}

// rogue_t::energy_regen_per_second =========================================

double rogue_t::energy_regen_per_second() const
{
  double r = player_t::energy_regen_per_second();

  if ( buffs.blade_flurry -> check() )
    r *= 1.0 + spec.blade_flurry -> effectN( 1 ).percent();

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
    opt_timespan( "virtual_hat_interval", ( virtual_hat_interval           ) ),
    opt_string( "tricks_of_the_trade_target", tricks_of_the_trade_target_str ),
    opt_null()
  };

  option_t::copy( options, rogue_options );
}

// rogue_t::create_profile ==================================================

bool rogue_t::create_profile( std::string& profile_str, save_e stype, bool save_html )
{
  player_t::create_profile( profile_str, stype, save_html );

  if ( stype == SAVE_ALL || stype == SAVE_ACTIONS )
  {
    if ( spec.honor_among_thieves -> ok() && ! sim -> solo_raid )
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
  rogue_t* p = debug_cast<rogue_t*>( source );
  virtual_hat_interval = p -> virtual_hat_interval;
}

// rogue_t::decode_set ======================================================

set_e rogue_t::decode_set( const item_t& item ) const
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

  if ( strstr( s, "thousandfold_blades"   ) ) return SET_T14_MELEE;

  if ( strstr( s, "ninetailed"            ) ) return SET_T15_MELEE;

  if ( strstr( s, "_gladiators_leather_" ) )  return SET_PVP_MELEE;

  if ( util::str_in_str_ci( s, "_barbed_assassin" ) ) return SET_T16_MELEE;

  return SET_NONE;
}

// rogue_t::convert_hybrid_stat ==============================================

stat_e rogue_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  case STAT_AGI_INT: 
    return STAT_AGILITY; 
  case STAT_STR_AGI:
    return STAT_AGILITY;
  // This is a guess at how STR/INT gear will work for Rogues, TODO: confirm  
  // This should probably never come up since rogues can't equip plate, but....
  case STAT_STR_INT:
    return STAT_NONE;
  case STAT_SPIRIT:
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
      return STAT_NONE;     
  default: return s; 
  }
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class rogue_report_t : public player_report_extension_t
{
public:
  rogue_report_t( rogue_t& player ) :
      p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void) p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
        << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
        << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }
private:
  rogue_t& p;
};

// ROGUE MODULE INTERFACE ===================================================

struct rogue_module_t : public module_t
{
  rogue_module_t() : module_t( ROGUE ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    rogue_t* p = new rogue_t( sim, name, r );
    p -> report_extension = std::shared_ptr<player_report_extension_t>( new rogue_report_t( *p ) );
    return p;
  }

  virtual bool valid() const
  { return true; }

  virtual void init( sim_t* sim ) const
  {
    for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[i];
      p -> buffs.tricks_of_the_trade  = buff_creator_t( p, "tricks_of_the_trade" )
                                        .max_stack( 1 )
                                        .duration( timespan_t::from_seconds( 6.0 ) );
    }
  }

  virtual void combat_begin( sim_t* ) const {}

  virtual void combat_end( sim_t* ) const {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::rogue()
{
  static rogue_module_t m;
  return &m;
}
