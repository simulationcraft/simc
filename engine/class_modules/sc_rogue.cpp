// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

using namespace residual_action;

namespace { // UNNAMED NAMESPACE

// ==========================================================================
// Custom Combo Point Impl.
// ==========================================================================

struct rogue_t;
namespace actions
{
struct rogue_attack_state_t;
struct residual_damage_state_t;
struct rogue_poison_t;
struct rogue_attack_t;
struct rogue_poison_buff_t;
struct sinister_calling_t;
struct melee_t;
}

namespace buffs
{
struct insight_buff_t;
}

enum ability_type_e {
  ABILITY_NONE = -1,
  EVISCERATE,
  SINISTER_STRIKE,
  REVEALING_STRIKE,
  AMBUSH,
  HEMORRHAGE,
  BACKSTAB,
  ENVENOM,
  DISPATCH,
  MUTILATE,
  RUPTURE,
  FAN_OF_KNIVES,
  CRIMSON_TEMPEST,
  KILLING_SPREE,
  ABILITY_MAX
};

struct shadow_reflect_event_t : public event_t
{
  int combo_points;
  action_t* source_action;

  shadow_reflect_event_t( int cp, action_t* a ) :
    event_t( *a -> player, "shadow_reflect_event" ),
    combo_points( cp ), source_action( a )
  {
    sim().add_event( this, timespan_t::from_seconds( 8 ) );
  }

  void execute();
};

// ==========================================================================
// Rogue
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
    buff_t* crippling_poison;
    buff_t* leeching_poison;
    buff_t* enhanced_crimson_tempest;
  } debuffs;

  rogue_td_t( player_t* target, rogue_t* source );

  bool sanguinary_veins();
};

struct rogue_t : public player_t
{
  // Venom Rush poison tracking
  unsigned poisoned_enemies;

  // Shadow Reflection stuff
  pet_t* shadow_reflection;
  bool reflection_attack;

  // Premeditation
  core_event_t* event_premeditation;

  // Active
  attack_t* active_blade_flurry;
  actions::rogue_poison_t* active_lethal_poison;
  actions::rogue_poison_t* active_nonlethal_poison;
  action_t* active_main_gauche;
  action_t* active_venomous_wound;

  // Autoattacks
  action_t* auto_attack;
  actions::melee_t* melee_main_hand;
  actions::melee_t* melee_off_hand;

  // Buffs
  struct buffs_t
  {
    buff_t* adrenaline_rush;
    buff_t* bandits_guile;
    buff_t* blade_flurry;
    buff_t* blindside;
    buff_t* burst_of_speed;
    buff_t* deadly_poison;
    buff_t* deadly_proc;
    buff_t* death_from_above;
    buff_t* killing_spree;
    buff_t* master_of_subtlety;
    buff_t* master_of_subtlety_passive;
    buff_t* nightstalker;
    buff_t* recuperate;
    buff_t* shadow_dance;
    buff_t* shadow_reflection;
    buff_t* shadowstep;
    buff_t* shiv;
    buff_t* sleight_of_hand;
    buff_t* sprint;
    buff_t* stealth;
    buff_t* subterfuge;
    buff_t* t16_2pc_melee;
    buff_t* tier13_2pc;
    buff_t* tot_trigger;
    buff_t* vanish;
    buff_t* wound_poison;

    // Insights
    buffs::insight_buff_t* deep_insight;
    buffs::insight_buff_t* moderate_insight;
    buffs::insight_buff_t* shallow_insight;

    // Ticking buffs
    buff_t* envenom;
    buff_t* slice_and_dice;

    // Legendary buffs
    buff_t* fof_fod; // Fangs of the Destroyer
    stat_buff_t* fof_p1; // Phase 1
    stat_buff_t* fof_p2;
    stat_buff_t* fof_p3;
    stat_buff_t* toxicologist;

    buff_t* anticipation;
    buff_t* deceit;
    buff_t* enhanced_vendetta;
    buff_t* quick_blades;
    buff_t* shadow_strikes;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* adrenaline_rush;
    cooldown_t* honor_among_thieves;
    cooldown_t* killing_spree;
    cooldown_t* ruthlessness;
    cooldown_t* seal_fate;
    cooldown_t* sprint;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* adrenaline_rush;
    gain_t* combat_potency;
    gain_t* deceit;
    gain_t* energetic_recovery;
    gain_t* energy_refund;
    gain_t* murderous_intent;
    gain_t* overkill;
    gain_t* recuperate;
    gain_t* relentless_strikes;
    gain_t* ruthlessness;
    gain_t* shadow_strikes;
    gain_t* t17_2pc_assassination;
    gain_t* t17_4pc_assassination;
    gain_t* venomous_wounds;
    gain_t* vitality;

    // CP Gains
    gain_t* honor_among_thieves;
    gain_t* empowered_fan_of_knives;
    gain_t* premeditation;
    gain_t* seal_fate;
    gain_t* legendary_daggers;
  } gains;

  // Spec passives
  struct spec_t
  {
    // Shared
    const spell_data_t* relentless_strikes;

    // Assassination
    const spell_data_t* assassins_resolve;
    const spell_data_t* blindside;
    const spell_data_t* cut_to_the_chase;
    const spell_data_t* improved_poisons;
    const spell_data_t* master_poisoner;
    const spell_data_t* seal_fate;
    const spell_data_t* venomous_wounds;

    // Combat
    const spell_data_t* ambidexterity;
    const spell_data_t* bandits_guile;
    const spell_data_t* blade_flurry;
    const spell_data_t* combat_potency;
    const spell_data_t* killing_spree;
    const spell_data_t* ruthlessness;
    const spell_data_t* vitality;

    // Subtlety
    const spell_data_t* energetic_recovery;
    const spell_data_t* find_weakness;
    const spell_data_t* honor_among_thieves;
    const spell_data_t* master_of_subtlety;
    const spell_data_t* sanguinary_vein;
    const spell_data_t* shadow_dance;
    const spell_data_t* sinister_calling;
  } spec;

  // Spell Data
  struct spells_t
  {
    const spell_data_t* bandits_guile_value;
    const spell_data_t* critical_strikes;
    const spell_data_t* death_from_above;
    const spell_data_t* fan_of_knives;
    const spell_data_t* fleet_footed;
    const spell_data_t* sprint;
    const spell_data_t* relentless_strikes;
    const spell_data_t* ruthlessness;
    const spell_data_t* ruthlessness_cp;
    const spell_data_t* shadow_focus;
    const spell_data_t* tier13_2pc;
    const spell_data_t* tier13_4pc;
    const spell_data_t* tier15_4pc;
    const spell_data_t* venom_rush;
  } spell;

  // Talents
  struct talents_t
  {
    const spell_data_t* nightstalker;
    const spell_data_t* subterfuge;
    const spell_data_t* shadow_focus;

    const spell_data_t* leeching_poison;
    const spell_data_t* burst_of_speed;
    const spell_data_t* shadowstep;

    const spell_data_t* anticipation;
    const spell_data_t* marked_for_death;

    const spell_data_t* venom_rush;
    const spell_data_t* shadow_reflection;
    const spell_data_t* death_from_above;
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
    const spell_data_t* disappearance;
    const spell_data_t* energy;
    const spell_data_t* hemorrhaging_veins;
    const spell_data_t* kick;
    const spell_data_t* sprint;
    const spell_data_t* vanish;
    const spell_data_t* vendetta;
  } glyph;

  struct perks_t
  {
    // Assassination
    const spell_data_t* improved_slice_and_dice;
    const spell_data_t* enhanced_vendetta;
    const spell_data_t* enhanced_crimson_tempest;
    const spell_data_t* empowered_envenom;

    // Combat
    const spell_data_t* empowered_bandits_guile;
    const spell_data_t* swift_poison;
    const spell_data_t* enhanced_blade_flurry;
    const spell_data_t* improved_dual_wield;

    // Subtlety
    const spell_data_t* enhanced_vanish;
    const spell_data_t* enhanced_shadow_dance;
    const spell_data_t* empowered_fan_of_knives;
    const spell_data_t* enhanced_stealth;
  } perk;

  // Procs
  struct procs_t
  {
    proc_t* honor_among_thieves;
    proc_t* seal_fate;
    proc_t* no_revealing_strike;
    proc_t* t16_2pc_melee;

    proc_t* combo_points_wasted;
    proc_t* anticipation_wasted;
  } procs;

  player_t* tot_target;
  action_callback_t* virtual_hat_callback;

  // Options
  uint32_t fof_p1, fof_p2, fof_p3;

  rogue_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, ROGUE, name, r ),
    poisoned_enemies( 0 ),
    shadow_reflection( 0 ),
    reflection_attack( false ),
    event_premeditation( 0 ),
    active_blade_flurry( 0 ),
    active_lethal_poison( 0 ),
    active_nonlethal_poison( 0 ),
    active_main_gauche( 0 ),
    active_venomous_wound( 0 ),
    auto_attack( 0 ), melee_main_hand( 0 ), melee_off_hand( 0 ),
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
    fof_p1( 0 ), fof_p2( 0 ), fof_p3( 0 )
  {
    // Cooldowns
    cooldowns.honor_among_thieves = get_cooldown( "honor_among_thieves" );
    cooldowns.seal_fate           = get_cooldown( "seal_fate"           );
    cooldowns.adrenaline_rush     = get_cooldown( "adrenaline_rush"     );
    cooldowns.killing_spree       = get_cooldown( "killing_spree"       );
    cooldowns.ruthlessness        = get_cooldown( "ruthlessness"        );
    cooldowns.sprint              = get_cooldown( "sprint"              );

    base.distance = 3;
    regen_type = REGEN_DYNAMIC;
    regen_caches[CACHE_HASTE] = true;
    regen_caches[CACHE_ATTACK_HASTE] = true;
  }

  // Character Definition
  virtual void      init_spells();
  virtual void      init_base_stats();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_scaling();
  virtual void      init_resources( bool force );
  virtual void      create_buffs();
  virtual void      init_action_list();
  virtual void      register_callbacks();
  virtual void      reset();
  virtual void      arise();
  virtual void      regen( timespan_t periodicity );
  virtual timespan_t available() const;
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual expr_t*   create_expression( action_t* a, const std::string& name_str );
  virtual resource_e primary_resource() const { return RESOURCE_ENERGY; }
  virtual role_e    primary_role() const  { return ROLE_ATTACK; }
  virtual stat_e    convert_hybrid_stat( stat_e s ) const;
  virtual void      create_pets();

  virtual double    composite_rating_multiplier( rating_e rating ) const;
  virtual double    composite_attribute_multiplier( attribute_e attr ) const;
  virtual double    composite_melee_speed() const;
  virtual double    composite_melee_crit() const;
  virtual double    composite_spell_crit() const;
  virtual double    matching_gear_multiplier( attribute_e attr ) const;
  virtual double    composite_attack_power_multiplier() const;
  virtual double    composite_player_multiplier( school_e school ) const;
  virtual double    energy_regen_per_second() const;
  virtual double    passive_movement_modifier() const;
  virtual double    temporary_movement_modifier() const;

  bool poisoned_enemy( player_t* target ) const;

  void trigger_auto_attack( const action_state_t* );
  void trigger_sinister_calling( const action_state_t* );
  void trigger_seal_fate( const action_state_t* );
  void trigger_main_gauche( const action_state_t* );
  void trigger_combat_potency( const action_state_t* );
  void trigger_energy_refund( const action_state_t* );
  void trigger_ruthlessness( const action_state_t* );
  void trigger_relentless_strikes( const action_state_t* );
  void trigger_venomous_wounds( const action_state_t* );
  void trigger_blade_flurry( const action_state_t* );
  void trigger_shadow_reflection( const action_state_t* );
  void trigger_combo_point_gain( const action_state_t*, int = -1, gain_t* gain = 0 );
  void spend_combo_points( const action_state_t* );
  void trigger_t17_4pc_combat( const action_state_t* );
  void trigger_t17_4pc_subtlety( const action_state_t* );

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

  static actions::rogue_attack_t* cast_attack( action_t* action )
  { return debug_cast<actions::rogue_attack_t*>( action ); }

  static const actions::rogue_attack_t* cast_attack( const action_t* action )
  { return debug_cast<const actions::rogue_attack_t*>( action ); }
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
  bool         requires_stealth;
  position_e   requires_position;
  int          adds_combo_points;
  weapon_e     requires_weapon;

  // we now track how much combo points we spent on an action
  int              combo_points_spent;

  // Justshadowreflectthings
  ability_type_e ability_type;

  // Sinister calling
  sinister_calling_t* sinister_calling;

  // Combo point gains
  gain_t* cp_gain;

  rogue_attack_t( const std::string& token, rogue_t* p,
                  const spell_data_t* s = spell_data_t::nil(),
                  const std::string& options = std::string() ) :
    melee_attack_t( token, p, s ),
    requires_stealth( false ), requires_position( POSITION_NONE ),
    adds_combo_points( 0 ),
    requires_weapon( WEAPON_NONE ),
    combo_points_spent( 0 ),
    ability_type( ABILITY_NONE ),
    sinister_calling( 0 )
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

  void init()
  {
    melee_attack_t::init();

    if ( adds_combo_points )
      cp_gain = player -> get_gain( name_str );
  }

  virtual void snapshot_state( action_state_t* state, dmg_e rt )
  {
    melee_attack_t::snapshot_state( state, rt );

    if ( base_costs[ RESOURCE_COMBO_POINT ] > 0 )
      cast_state( state ) -> cp = static_cast<int>( player -> resources.current[RESOURCE_COMBO_POINT] );
  }

  bool stealthed()
  {
    return p() -> buffs.vanish -> check() || p() -> buffs.stealth -> check() || player -> buffs.shadowmeld -> check();
  }

  virtual bool procs_poison() const
  { return weapon != 0; }

  // Adjust poison proc chance
  virtual double composite_poison_flat_modifier( const action_state_t* ) const
  { return 0.0; }

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

  virtual double target_armor( player_t* ) const;

  virtual double attack_direct_power_coefficient( const action_state_t* s ) const
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return attack_power_mod.direct * cast_state( s ) -> cp;
    return melee_attack_t::attack_direct_power_coefficient( s );
  }

  virtual double attack_tick_power_coefficient( const action_state_t* s ) const
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return attack_power_mod.tick * cast_state( s ) -> cp;
    return melee_attack_t::attack_tick_power_coefficient( s );
  }

  virtual double spell_direct_power_coefficient( const action_state_t* s ) const
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return spell_power_mod.direct * cast_state( s ) -> cp;
    return melee_attack_t::spell_direct_power_coefficient( s );
  }

  virtual double spell_tick_power_coefficient( const action_state_t* s ) const
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return spell_power_mod.tick * cast_state( s ) -> cp;
    return melee_attack_t::spell_tick_power_coefficient( s );
  }

  virtual double bonus_da( const action_state_t* s ) const
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return base_dd_adder * cast_state( s ) -> cp;
    return melee_attack_t::bonus_da( s );
  }

  virtual double bonus_ta( const action_state_t* s ) const
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return base_ta_adder * cast_state( s ) -> cp;
    return melee_attack_t::bonus_ta( s );
  }

  virtual double composite_da_multiplier( const action_state_t* state ) const
  {
    double m = melee_attack_t::composite_da_multiplier( state );

    if ( base_costs[ RESOURCE_COMBO_POINT ] && p() -> mastery.executioner -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual double composite_ta_multiplier( const action_state_t* state ) const
  {
    double m = melee_attack_t::composite_ta_multiplier( state );

    if ( base_costs[ RESOURCE_COMBO_POINT ] && p() -> mastery.executioner -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual double composite_target_multiplier( player_t* target ) const
  {
    double m = melee_attack_t::composite_target_multiplier( target );

    rogue_td_t* tdata = td( target );
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
    {
      if ( tdata -> dots.revealing_strike -> is_ticking() )
        m *= 1.0 + tdata -> dots.revealing_strike -> current_action -> data().effectN( 3 ).percent();
      else if ( p() -> specialization() == ROGUE_COMBAT )
        p() -> procs.no_revealing_strike -> occur();
    }

    m *= 1.0 + tdata -> debuffs.vendetta -> value();

    if ( p() -> spec.sanguinary_vein -> ok() && tdata -> sanguinary_veins() )
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

  void trigger_sinister_calling( dot_t* dot );
};

struct sinister_calling_t : public rogue_attack_t
{
  sinister_calling_t( const std::string& token, rogue_t* p, const spell_data_t* s ) :
    rogue_attack_t( token, p, s )
  {
    may_crit = callbacks = may_multistrike = may_miss = false;
    proc = background = true;
  }

  dmg_e report_amount_type( const action_state_t* ) const
  { return DMG_DIRECT; }

  double target_armor( player_t* ) const
  { return 0; }

  // Sinister calling just replicates the damage of the tick
  double calculate_direct_amount( action_state_t* state )
  { return state -> result_amount; }
};

// ==========================================================================
// Poisons
// ==========================================================================

struct rogue_poison_t : public rogue_attack_t
{
  double proc_chance_;

  rogue_poison_t( const std::string& token, rogue_t* p,
                  const spell_data_t* s = spell_data_t::nil() ) :
    actions::rogue_attack_t( token, p, s ),
    proc_chance_( 0 )
  {
    proc              = true;
    background        = true;
    trigger_gcd       = timespan_t::zero();
    may_dodge         = false;
    may_parry         = false;
    may_block         = false;
    callbacks         = false;

    weapon_multiplier = 0;

    proc_chance_  = data().proc_chance();
    proc_chance_ += p -> spec.improved_poisons -> effectN( 1 ).percent();
  }

  timespan_t execute_time() const
  { return timespan_t::zero(); }

  virtual double proc_chance( const action_state_t* source_state )
  {
    double chance = proc_chance_;

    if ( p() -> buffs.envenom -> up() )
      chance += p() -> buffs.envenom -> data().effectN( 2 ).percent();

    const rogue_attack_t* attack = debug_cast< const rogue_attack_t* >( source_state -> action );
    chance += attack -> composite_poison_flat_modifier( source_state );

    return chance;
  }

  virtual void trigger( const action_state_t* source_state )
  {
    bool result = rng().roll( proc_chance( source_state ) );

    if ( sim -> debug )
      sim -> out_debug.printf( "%s attempts to proc %s, target=%s source_action=%s proc_chance=%.3f: %d",
          player -> name(), name(), source_state -> target -> name(), source_state -> action -> name(), proc_chance( source_state ), result );

    if ( ! result )
      return;

    target = source_state -> target;
    execute();
  }

  virtual double composite_target_multiplier( player_t* target ) const
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    if ( td( target ) -> debuffs.enhanced_crimson_tempest -> up() )
      m *= 1.0 + td( target ) -> debuffs.enhanced_crimson_tempest -> data().effectN( 1 ).percent();

    return m;
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

    m *= 1.0 + p() -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 1 ).percent();

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

    void impact( action_state_t* state )
    {
      if ( ! p() -> poisoned_enemy( state -> target ) && result_is_hit( state -> result ) )
        p() -> poisoned_enemies++;

      rogue_poison_t::impact( state );
    }

    void last_tick( dot_t* d )
    {
      player_t* t = d -> state -> target;

      rogue_poison_t::last_tick( d );

      if ( ! p() -> poisoned_enemy( t ) )
        p() -> poisoned_enemies--;
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
      proc_dot -> target = state -> target;
      proc_dot -> execute();
      if ( is_up )
      {
        proc_instant -> target = state -> target;
        proc_instant -> execute();
      }
    }
  }
};

// Instant Poison ===========================================================

struct instant_poison_t : public rogue_poison_t
{
  struct instant_poison_dd_t : public rogue_poison_t
  {
    instant_poison_dd_t( rogue_t* p ) :
      rogue_poison_t( "instant_poison", p, p -> find_spell( 157607 ) )
    {
      harmful = true;
    }
  };

  instant_poison_dd_t* proc_instant;

  instant_poison_t( rogue_t* player ) :
    rogue_poison_t( "instant_poison", player, player -> find_spell( 157584 ) )
  {
    dual = true;
    may_miss = may_crit = false;

    proc_instant = new instant_poison_dd_t( player );
  }

  virtual void impact( action_state_t* state )
  {
    rogue_poison_t::impact( state );

    proc_instant -> target = state -> target;
    proc_instant -> execute();
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

    proc_dd -> target = state -> target;
    proc_dd -> execute();
  }
};

// Crippling poison =========================================================

struct crippling_poison_t : public rogue_poison_t
{
  struct crippling_poison_proc_t : public rogue_poison_t
  {
    crippling_poison_proc_t( rogue_t* rogue ) :
      rogue_poison_t( "crippling_poison", rogue, rogue -> find_spell( 3409 ) )
    { }

    void impact( action_state_t* state )
    {
      rogue_poison_t::impact( state );

      td( state -> target ) -> debuffs.crippling_poison -> trigger();
    }
  };

  crippling_poison_proc_t* proc;

  crippling_poison_t( rogue_t* player ) :
    rogue_poison_t( "crippling_poison_driver", player, player -> find_class_spell( "Crippling Poison" ) ),
    proc( new crippling_poison_proc_t( player ) )
  {
    dual = true;
    may_miss = may_crit = false;
  }

  void impact( action_state_t* state )
  {
    rogue_poison_t::impact( state );

    proc -> target = state -> target;
    proc -> execute();
  }
};

// Leeching poison =========================================================

struct leeching_poison_t : public rogue_poison_t
{
  struct leeching_poison_proc_t : public rogue_poison_t
  {
    leeching_poison_proc_t( rogue_t* rogue ) :
      rogue_poison_t( "leeching_poison", rogue, rogue -> find_spell( 112961 ) )
    { }

    void impact( action_state_t* state )
    {
      rogue_poison_t::impact( state );

      td( state -> target ) -> debuffs.leeching_poison -> trigger();
    }
  };

  leeching_poison_proc_t* proc;

  leeching_poison_t( rogue_t* player ) :
    rogue_poison_t( "leeching_poison_driver", player, player -> find_talent_spell( "Leeching Poison" ) ),
    proc( new leeching_poison_proc_t( player ) )
  {
    dual = true;
    may_miss = may_crit = false;
  }

  void impact( action_state_t* state )
  {
    rogue_poison_t::impact( state );

    proc -> target = state -> target;
    proc -> execute();
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
    INSTANT_POISON,
    CRIPPLING_POISON,
    LEECHING_POISON,
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
      if ( lethal_str == "deadly"    ) lethal_poison = p -> perk.swift_poison -> ok() ? INSTANT_POISON : DEADLY_POISON;
      if ( lethal_str == "instant"   ) lethal_poison = INSTANT_POISON;
      if ( lethal_str == "wound"     ) lethal_poison = WOUND_POISON;

      if ( nonlethal_str == "crippling" ) nonlethal_poison = CRIPPLING_POISON;
      if ( nonlethal_str == "leeching"  ) nonlethal_poison = LEECHING_POISON;
    }

    if ( ! p -> active_lethal_poison )
    {
      if ( lethal_poison == DEADLY_POISON  ) p -> active_lethal_poison = new deadly_poison_t( p );
      if ( lethal_poison == WOUND_POISON   ) p -> active_lethal_poison = new wound_poison_t( p );
      if ( lethal_poison == INSTANT_POISON ) p -> active_lethal_poison = new instant_poison_t( p );
    }

    if ( ! p -> active_nonlethal_poison )
    {
      if ( nonlethal_poison == CRIPPLING_POISON ) p -> active_nonlethal_poison = new crippling_poison_t( p );
      if ( nonlethal_poison == LEECHING_POISON  ) p -> active_nonlethal_poison = new leeching_poison_t( p );
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

// ==========================================================================
// Static Functions
// ==========================================================================

// break_stealth ============================================================

static void break_stealth( rogue_t* p )
{
  if ( p -> buffs.stealth -> check() )
    p -> buffs.stealth -> expire();

  if ( p -> buffs.vanish -> check() )
    p -> buffs.vanish -> expire();

  if ( p -> player_t::buffs.shadowmeld -> check() )
    p -> player_t::buffs.shadowmeld -> expire();
}

// ==========================================================================
// Attacks
// ==========================================================================

// rogue_attack_t::impact ===================================================

void rogue_attack_t::impact( action_state_t* state )
{
  melee_attack_t::impact( state );

  if ( adds_combo_points )
    p() -> trigger_seal_fate( state );

  p() -> trigger_combat_potency( state );
  p() -> trigger_blade_flurry( state );

  if ( result_is_hit( state -> result ) )
  {
    if ( procs_poison() && p() -> active_lethal_poison )
      p() -> active_lethal_poison -> trigger( state );

    if ( procs_poison() && p() -> active_nonlethal_poison )
      p() -> active_nonlethal_poison -> trigger( state );

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
          p() -> trigger_combo_point_gain( state, 5, p() -> gains.legendary_daggers );
        }
      }
    }

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

  if ( p() -> sets.has_set_bonus( SET_MELEE, T15, B2 ) && p() -> buffs.tier13_2pc -> check() )
    c *= 1.0 + p() -> spell.tier13_2pc -> effectN( 1 ).percent();

  if ( p() -> buffs.quick_blades -> check() )
    c *= 1.0 + p() -> buffs.quick_blades -> data().effectN( 1 ).percent();

  if ( c <= 0 )
    c = 0;

  return c;
}

// rogue_attack_t::consume_resource =========================================

void rogue_attack_t::consume_resource()
{
  melee_attack_t::consume_resource();

  p() -> spend_combo_points( execute_state );

  if ( result_is_miss( execute_state -> result ) && resource_consumed > 0 )
    p() -> trigger_energy_refund( execute_state );

  p() -> trigger_relentless_strikes( execute_state );
}

// rogue_attack_t::execute ==================================================

void rogue_attack_t::execute()
{
  melee_attack_t::execute();

  p() -> trigger_auto_attack( execute_state );
  p() -> trigger_main_gauche( execute_state );
  p() -> trigger_shadow_reflection( execute_state );
  p() -> trigger_ruthlessness( execute_state );
  p() -> trigger_combo_point_gain( execute_state );
  p() -> trigger_t17_4pc_combat( execute_state );
  p() -> trigger_t17_4pc_subtlety( execute_state );

  if ( harmful && stealthed() )
  {
    if ( ! p() -> talent.subterfuge -> ok() )
      break_stealth( p() );
    else if ( ! p() -> buffs.subterfuge -> check() )
      p() -> buffs.subterfuge -> trigger();
  }
}

// rogue_attack_t::ready() ==================================================

inline bool rogue_attack_t::ready()
{
  rogue_t* p = cast();

  if ( ! melee_attack_t::ready() )
    return false;

  if ( base_costs[ RESOURCE_COMBO_POINT ] > 0 &&
       player -> resources.current[ RESOURCE_COMBO_POINT ] < base_costs[ RESOURCE_COMBO_POINT ] )
    return false;

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
    auto_attack     = true;
    school          = SCHOOL_PHYSICAL;
    background      = true;
    repeating       = true;
    trigger_gcd     = timespan_t::zero();
    special         = false;
    may_glance      = true;

    if ( p -> dual_wield() && ! p -> perk.improved_dual_wield -> ok() )
      base_hit -= 0.19;

    p -> auto_attack = this;
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

    p -> melee_main_hand = debug_cast<melee_t*>( p -> find_action( "auto_attack_mh" ) );
    if ( ! p -> melee_main_hand )
      p -> melee_main_hand = new melee_t( "auto_attack_mh", p, sync_weapons );

    p -> main_hand_attack = p -> melee_main_hand;
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> melee_off_hand = debug_cast<melee_t*>( p -> find_action( "auto_attack_oh" ) );
      if ( ! p -> melee_off_hand )
        p -> melee_off_hand = new melee_t( "auto_attack_oh", p, sync_weapons );

      p -> off_hand_attack = p -> melee_off_hand;
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
    ability_type      = AMBUSH;
    requires_position = POSITION_BACK;
    requires_stealth  = true;
  }

  double action_multiplier() const
  {
    double m = rogue_attack_t::action_multiplier();

    if ( weapon -> type == WEAPON_DAGGER )
      m *= 1.40;

    return m;
  }

  virtual double cost() const
  {
    double c = rogue_attack_t::cost();

    if ( p() -> buffs.shadow_dance -> check() )
      c += p() -> spec.shadow_dance -> effectN( 2 ).base_value();

    c -= 2 * p() -> buffs.t16_2pc_melee -> stack();

    if ( c < 0 )
      return 0;

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

    if ( result_is_multistrike( state -> result ) )
      p() -> trigger_sinister_calling( state );
  }

  void consume_resource()
  {
    rogue_attack_t::consume_resource();

    p() -> buffs.enhanced_vendetta -> expire();
  }

  double composite_crit() const
  {
    double c = rogue_attack_t::composite_crit();

    if ( ! p() -> reflection_attack && p() -> buffs.enhanced_vendetta -> up() )
      c += p() -> buffs.enhanced_vendetta -> data().effectN( 1 ).percent();

    return c;
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
    ability_type      = BACKSTAB;
    requires_weapon   = WEAPON_DAGGER;
    requires_position = POSITION_BACK;
  }

  virtual double cost() const
  {
    double c = rogue_attack_t::cost();
    c -= 2 * p() -> buffs.t16_2pc_melee -> stack();
    if ( c < 0 )
      c = 0;
    return c;
  }

  void execute()
  {
    rogue_attack_t::execute();

    p() -> buffs.t16_2pc_melee -> expire();

    if ( result_is_hit( execute_state -> result ) && p() -> sets.has_set_bonus( SET_MELEE, T16, B4 ) )
      p() -> buffs.sleight_of_hand -> trigger();
  }

  void impact( action_state_t* state )
  {
    rogue_attack_t::impact( state );

    if ( result_is_multistrike( state -> result ) )
      p() -> trigger_sinister_calling( state );
  }

  double composite_da_multiplier( const action_state_t* state ) const
  {
    double m = rogue_attack_t::composite_da_multiplier( state );

    m *= 1.0 + p() -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 2 ).percent();

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
    ability_type = DISPATCH;

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
      if ( c < 0 )
        c = 0;
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

    if ( p() -> sets.has_set_bonus( ROGUE_ASSASSINATION, T17, B2 ) && execute_state -> result == RESULT_CRIT )
      p() -> resource_gain( RESOURCE_ENERGY,
                            p() -> sets.set( ROGUE_ASSASSINATION, T17, B2 ) -> effectN( 1 ).base_value(),
                            p() -> gains.t17_2pc_assassination,
                            this );
  }

  void consume_resource()
  {
    rogue_attack_t::consume_resource();

    p() -> buffs.enhanced_vendetta -> expire();
  }

  double composite_crit() const
  {
    double c = rogue_attack_t::composite_crit();

    if ( ! p() -> reflection_attack && p() -> buffs.enhanced_vendetta -> up() )
      c += p() -> buffs.enhanced_vendetta -> data().effectN( 1 ).percent();

    return c;
  }

  double action_multiplier() const
  {
    double m = rogue_attack_t::action_multiplier();

    if ( ! p() -> reflection_attack && p() -> perk.empowered_envenom -> ok() && p() -> buffs.envenom -> up() )
      m *= 1.0 + p() -> perk.empowered_envenom -> effectN( 1 ).percent();

    return m;
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
    ability_type = ENVENOM;
    weapon = &( p -> main_hand_weapon );
    base_costs[ RESOURCE_COMBO_POINT ] = 1;
    attack_power_mod.direct       = 0.306;
    dot_duration = timespan_t::zero();
    weapon_multiplier = weapon_power_mod = 0.0;
  }

  double base_da_min( const action_state_t* s ) const
  { return base_dd_min * rogue_attack_t::cast_state( s ) -> cp; }

  double base_da_max( const action_state_t* s ) const
  { return base_dd_max * rogue_attack_t::cast_state( s ) -> cp; }

  void consume_resource()
  {
    rogue_attack_t::consume_resource();

    p() -> buffs.enhanced_vendetta -> expire();

    if ( p() -> sets.has_set_bonus( ROGUE_ASSASSINATION, T17, B4 ) )
      p() -> trigger_combo_point_gain( execute_state, 1, p() -> gains.t17_4pc_assassination );
  }

  double action_multiplier() const
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p() -> buffs.death_from_above -> up() )
      m *= 1.0 + p() -> buffs.death_from_above -> data().effectN( 2 ).percent();

    return m;
  }

  double cost() const
  {
    double c = rogue_attack_t::cost();

    if ( p() -> buffs.death_from_above -> check() )
      c *= 1.0 + p() -> buffs.death_from_above -> data().effectN( 1 ).percent();

    if ( c < 0 )
      c = 0;

    return c;
  }

  double composite_crit() const
  {
    double c = rogue_attack_t::composite_crit();

    if ( ! p() -> reflection_attack && p() -> buffs.enhanced_vendetta -> up() )
      c += p() -> buffs.enhanced_vendetta -> data().effectN( 1 ).percent();

    return c;
  }

  virtual void execute()
  {
    rogue_attack_t::execute();

    timespan_t envenom_duration = p() -> buffs.envenom -> buff_period * ( 1 + cast_state( execute_state ) -> cp );

    if ( p() -> sets.has_set_bonus( SET_MELEE, T15, B2 ) )
      envenom_duration += p() -> buffs.envenom -> buff_period;

    p() -> buffs.envenom -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, envenom_duration );

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

    if ( ! p() -> perk.improved_slice_and_dice -> ok() &&
         p() -> spec.cut_to_the_chase -> ok() &&
         p() -> buffs.slice_and_dice -> check() )
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
    ability_type = EVISCERATE;
    base_costs[ RESOURCE_COMBO_POINT ] = 1;
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier = weapon_power_mod = 0;

    attack_power_mod.direct = 0.577;
  }

  timespan_t gcd() const
  {
    timespan_t t = rogue_attack_t::gcd();

    if ( t != timespan_t::zero() && p() -> buffs.adrenaline_rush -> check() )
      t += p() -> buffs.adrenaline_rush -> data().effectN( 3 ).time_value();

    return t;
  }

  double action_multiplier() const
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p() -> buffs.death_from_above -> up() )
      m *= 1.0 + p() -> buffs.death_from_above -> data().effectN( 2 ).percent();

    return m;
  }

  double cost() const
  {
    double c = rogue_attack_t::cost();

    if ( p() -> buffs.death_from_above -> check() )
      c *= 1.0 + p() -> buffs.death_from_above -> data().effectN( 1 ).percent();

    if ( p() -> buffs.deceit -> check() )
      c *= 1.0 + p() -> buffs.deceit -> data().effectN( 1 ).percent();

    if ( c < 0 )
      c = 0;

    return c;
  }

  void consume_resource()
  {
    rogue_attack_t::consume_resource();

    p() -> buffs.deceit -> expire();
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

// Fan of Knives ============================================================

struct fan_of_knives_t: public rogue_attack_t
{
  fan_of_knives_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "fan_of_knives", p, p -> find_class_spell( "Fan of Knives" ), options_str )
  {
    ability_type = FAN_OF_KNIVES;
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier = 0;
    aoe = -1;
    adds_combo_points = 1;
  }

  void impact( action_state_t* state )
  {
    rogue_attack_t::impact( state );
    if ( p() -> perk.empowered_fan_of_knives -> ok() )
      p() -> trigger_combo_point_gain( state, 1, p() -> gains.empowered_fan_of_knives );
  }
};

// Crimson Tempest ==========================================================

struct crimson_tempest_t : public rogue_attack_t
{
  struct crimson_tempest_dot_t : public residual_periodic_action_t<rogue_attack_t>
  {
    crimson_tempest_dot_t( rogue_t * p ) :
      residual_periodic_action_t<rogue_attack_t>( "crimson_tempest", p, p -> find_spell( 122233 ) )
    {
      if ( p -> spec.sinister_calling -> ok() )
      {
        sinister_calling = new sinister_calling_t( "crimson_tempest_sc", p, p -> find_spell( 168952 ) );
        add_child( sinister_calling );
      }
    }

    action_state_t* new_state()
    { return new residual_periodic_state_t( this, target ); }
  };

  crimson_tempest_dot_t* ct_dot;

  crimson_tempest_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "crimson_tempest", p, p -> find_class_spell( "Crimson Tempest" ), options_str )
  {
    ability_type = CRIMSON_TEMPEST;
    aoe = -1;
    base_costs[ RESOURCE_COMBO_POINT ] = 1;
    attack_power_mod.direct = 0.0602;
    weapon = &( p -> main_hand_weapon );
    weapon_power_mod = weapon_multiplier = 0;
    ct_dot = new crimson_tempest_dot_t( p );
  }

  void impact( action_state_t* s )
  {
    rogue_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      residual_action::trigger( ct_dot, s -> target, s -> result_amount * ct_dot -> data().effectN( 1 ).percent() );

      if ( p() -> perk.enhanced_crimson_tempest -> ok() )
      {
        rogue_td_t* tdata = td( s -> target );
        rogue_attack_state_t* state = debug_cast<rogue_attack_state_t*>( s );
        timespan_t duration = timespan_t::from_seconds( 1 + state -> cp );
        tdata -> debuffs.enhanced_crimson_tempest -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
      }
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

    if ( p -> spec.sinister_calling -> ok() )
    {
      sinister_calling = new sinister_calling_t( "garrote_sc", p, p -> find_spell( 168971 ) );
      add_child( sinister_calling );
    }
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
      p() -> trigger_venomous_wounds( d -> state );
  }
};

// Hemorrhage ===============================================================

struct hemorrhage_t : public rogue_attack_t
{
  hemorrhage_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "hemorrhage", p, p -> find_class_spell( "Hemorrhage" ), options_str )
  {
    ability_type = HEMORRHAGE;
    weapon = &( p -> main_hand_weapon );
  }

  double action_multiplier() const
  {
    double m = rogue_attack_t::action_multiplier();

    if ( weapon -> type == WEAPON_DAGGER )
      m *= 1.4;

    return m;
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
    ability_type = KILLING_SPREE;
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
        m *= std::pow( 1.0 + p() -> sets.set( SET_MELEE, T16, B4 ) -> effectN( 1 ).percent(),
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

    p() -> trigger_seal_fate( state );

    if ( p() -> sets.has_set_bonus( ROGUE_ASSASSINATION, T17, B2 ) && state -> result == RESULT_CRIT )
      p() -> resource_gain( RESOURCE_ENERGY,
                            p() -> sets.set( ROGUE_ASSASSINATION, T17, B2 ) -> effectN( 1 ).base_value(),
                            p() -> gains.t17_2pc_assassination,
                            this );
  }

  double action_multiplier() const
  {
    double m = rogue_attack_t::action_multiplier();

    if ( ! p() -> reflection_attack && p() -> perk.empowered_envenom -> ok() && p() -> buffs.envenom -> up() )
      m *= 1.0 + p() -> perk.empowered_envenom -> effectN( 1 ).percent();

    return m;
  }

  double composite_crit() const
  {
    double c = rogue_attack_t::composite_crit();

    if ( ! p() -> reflection_attack && p() -> buffs.enhanced_vendetta -> up() )
      c += p() -> buffs.enhanced_vendetta -> data().effectN( 1 ).percent();

    return c;
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
    ability_type = MUTILATE;
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

  void consume_resource()
  {
    rogue_attack_t::consume_resource();

    p() -> buffs.enhanced_vendetta -> expire();
  }
  
  double cost() const
  {
    double c = rogue_attack_t::cost();
    if ( p() -> buffs.t16_2pc_melee -> up() )
      c -= 6 * p() -> buffs.t16_2pc_melee -> check();

    if ( c < 0 )
      c = 0;

    return c;
  }

  void execute()
  {
    rogue_attack_t::execute();

    p() -> buffs.t16_2pc_melee -> expire();

    if ( result_is_hit( execute_state -> result ) )
    {
      mh_strike -> execute();
      oh_strike -> execute();
    }
  }

  void impact( action_state_t* state )
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

      p -> resources.current[ RESOURCE_COMBO_POINT ] -= combo_points;
      if ( sim().log )
      {
        sim().out_log.printf( "%s loses %d temporary combo_points from premeditation (%d)",
                    actor -> name(), combo_points, p -> resources.current[ RESOURCE_COMBO_POINT ] );
      }

      assert( p -> resources.current[ RESOURCE_COMBO_POINT ] >= 0 );
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

    double add_points = data().effectN( 1 ).base_value();

    add_points = std::min( add_points, player -> resources.max[ RESOURCE_COMBO_POINT ] - player -> resources.current[ RESOURCE_COMBO_POINT ] );

    if ( add_points > 0 )
      p() -> trigger_combo_point_gain( 0, static_cast<int>( add_points ), p() -> gains.premeditation );

    p() -> event_premeditation = new ( *sim ) premeditation_event_t( *p(), state -> target, data().duration(), static_cast<int>( add_points ) );
  }
};

// Recuperate ===============================================================

struct recuperate_t : public rogue_attack_t
{
  recuperate_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "recuperate", p, p -> find_class_spell( "Recuperate" ), options_str )
  {
    base_costs[ RESOURCE_COMBO_POINT ] = 1;
    dot_behavior = DOT_REFRESH;
    harmful = false;
  }

  virtual timespan_t composite_dot_duration( const action_state_t* s ) const override
  { return 2 * cast_state( s ) -> cp * base_tick_time; }

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
    ability_type = REVEALING_STRIKE;
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
};

// Rupture ==================================================================

struct rupture_t : public rogue_attack_t
{
  rupture_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "rupture", p, p -> find_specialization_spell( "Rupture" ), options_str )
  {
    ability_type          = RUPTURE;
    may_crit              = false;
    base_costs[ RESOURCE_COMBO_POINT ] = 1;
    tick_may_crit         = true;
    dot_behavior          = DOT_REFRESH;
    base_multiplier      *= 1.0 + p -> spec.sanguinary_vein -> effectN( 1 ).percent();

    if ( p -> spec.sinister_calling -> ok() )
    {
      sinister_calling = new sinister_calling_t( "rupture_sc", p, p -> find_spell( 168963 ) );
      add_child( sinister_calling );
    }
  }

  timespan_t gcd() const
  {
    timespan_t t = rogue_attack_t::gcd();

    if ( t != timespan_t::zero() && p() -> buffs.adrenaline_rush -> check() )
      t += p() -> buffs.adrenaline_rush -> data().effectN( 3 ).time_value();

    return t;
  }

  virtual timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t duration = data().duration();

    duration += duration * cast_state( s ) -> cp;
    if ( p() -> sets.has_set_bonus( SET_MELEE, T15, B2 ) )
      duration += data().duration();

    return duration;
  }

  virtual void tick( dot_t* d )
  {
    rogue_attack_t::tick( d );

    p() -> trigger_venomous_wounds( d -> state );
  }
};

// Shadowstep ===============================================================

struct shadowstep_t : public rogue_attack_t
{
  shadowstep_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shadowstep", p, p -> talent.shadowstep, options_str )
  {
    harmful = false;
    dot_duration = timespan_t::zero();
    base_teleport_distance = data().max_range();
    movement_directionality = MOVEMENT_OMNI;
  }

  void execute()
  {
    rogue_attack_t::execute();
    p() -> buffs.shadowstep -> trigger();
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

  bool procs_poison() const
  { return true; }
};

// Sinister Strike ==========================================================

struct sinister_strike_t : public rogue_attack_t
{
  sinister_strike_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "sinister_strike", p, p -> find_class_spell( "Sinister Strike" ), options_str )
  {
    ability_type = SINISTER_STRIKE;
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
    if ( c < 0 )
      c = 0;
    return c;
  }

  double composite_da_multiplier( const action_state_t* state ) const
  {
    double m = rogue_attack_t::composite_da_multiplier( state );

    if ( p() -> fof_p1 || p() -> fof_p2 || p() -> fof_p3 )
      m *= 1.0 + p() -> dbc.spell( 110211 ) -> effectN( 1 ).percent();

    m *= 1.0 + p() -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 2 ).percent();

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
      double proc_chance = td -> dots.revealing_strike -> current_action -> data().effectN( 6 ).percent();
      proc_chance += p() -> sets.set( ROGUE_COMBAT, T17, B2 ) -> effectN( 1 ).percent();

      if ( td -> dots.revealing_strike -> is_ticking() && rng().roll( proc_chance ) )
      {
        p() -> trigger_combo_point_gain( state );
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
    base_costs[ RESOURCE_COMBO_POINT ] = 1;
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
    rogue_attack_t::execute();

    double snd = p() -> buffs.slice_and_dice -> data().effectN( 1 ).percent();
    if ( p() -> mastery.executioner -> ok() )
      snd *= 1.0 + p() -> cache.mastery_value();
    timespan_t snd_duration = 3 * ( cast_state( execute_state ) -> cp + 1 ) * p() -> buffs.slice_and_dice -> buff_period;

    if ( p() -> sets.has_set_bonus( SET_MELEE, T15, B2 ) )
      snd_duration += 3 * p() -> buffs.slice_and_dice -> buff_period;

    p() -> buffs.slice_and_dice -> trigger( 1, snd, -1.0, snd_duration );
  }

  bool ready()
  {
    if ( p() -> perk.improved_slice_and_dice -> ok() )
      return false;

    return rogue_attack_t::ready();
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
    p() -> buffs.quick_blades -> trigger();
  }
};

// Burst of Speed ===========================================================

struct burst_of_speed_t: public rogue_attack_t
{
  burst_of_speed_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "burst_of_speed", p, p -> talent.burst_of_speed, options_str )
  {
    harmful = callbacks = false;
    dot_duration = timespan_t::zero();
    cooldown -> duration = p -> buffs.burst_of_speed -> data().duration();
  }

  void execute()
  {
    rogue_attack_t::execute();

    p() -> buffs.burst_of_speed -> trigger();
  }
};

// Sprint ===================================================================

struct sprint_t: public rogue_attack_t
{
  sprint_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "sprint", p, p -> spell.sprint, options_str )
  {
    harmful = callbacks = false;
    cooldown = p -> cooldowns.sprint;
  }

  void execute()
  {
    rogue_attack_t::execute();

    p() -> buffs.sprint -> trigger();
  }
};

// Vanish ===================================================================

struct vanish_t : public rogue_attack_t
{
  vanish_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "vanish", p, p -> find_class_spell( "Vanish" ), options_str )
  {
    may_miss = may_crit = harmful = false;

    cooldown -> duration += p -> perk.enhanced_vanish -> effectN( 1 ).time_value();
    cooldown -> duration += p -> glyph.disappearance -> effectN( 1 ).time_value();
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
    p() -> buffs.enhanced_vendetta -> trigger();
  }
};

// Shadow Reflection

struct shadow_reflection_t : public rogue_attack_t
{
  shadow_reflection_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shadow_reflection", p, p -> find_talent_spell( "Shadow Reflection" ), options_str )
  {
    harmful = may_miss = may_crit = callbacks = false;
  }

  void execute()
  {
    rogue_attack_t::execute();

    p() -> buffs.shadow_reflection -> trigger();
  }
};

// Death From Above

struct death_from_above_t : public rogue_attack_t
{
  envenom_t* envenom;
  eviscerate_t* eviscerate;
  death_from_above_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "death_from_above", p, p -> talent.death_from_above, options_str ),
    envenom( p -> specialization() == ROGUE_ASSASSINATION ? new envenom_t( p, "" ) : 0 ),
    eviscerate( p -> specialization() != ROGUE_ASSASSINATION ? new eviscerate_t( p, "" ) : 0 )
  {
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;
    tick_may_crit = false;

    aoe = -1;
  }

  void execute()
  {
    rogue_attack_t::execute();

    p() -> buffs.death_from_above -> trigger();

    // Apparently DfA is out of range for ~0.8 seconds during the "attack", so
    // ensure that we have a swing timer of at least 800ms on both hands. Note
    // that this can sync autoattacks which also happens in game.
    if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
    {
      if ( player -> main_hand_attack -> execute_event -> remains() < timespan_t::from_seconds( 0.8 ) )
        player -> main_hand_attack -> execute_event -> reschedule( timespan_t::from_seconds( 0.8 ) );
    }

    if ( player -> off_hand_attack && player -> off_hand_attack -> execute_event )
    {
      if ( player -> off_hand_attack -> execute_event -> remains() < timespan_t::from_seconds( 0.8 ) )
        player -> off_hand_attack -> execute_event -> reschedule( timespan_t::from_seconds( 0.8 ) );
    }
  }

  void last_tick( dot_t* d )
  {
    rogue_attack_t::last_tick( d );

    if ( player -> resources.current[ RESOURCE_COMBO_POINT ] == 0 )
      return;

    if ( envenom )
      envenom -> execute();
    else if ( eviscerate )
      eviscerate -> execute();
    else
      assert( 0 );

    p() -> buffs.death_from_above -> expire();
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

// ==========================================================================
// Rogue Secondary Abilities
// ==========================================================================

struct main_gauche_t : public rogue_attack_t
{
  main_gauche_t( rogue_t* p ) :
    rogue_attack_t( "main_gauche", p, p -> find_spell( 86392 ) )
  {
    weapon          = &( p -> off_hand_weapon );
    special         = true;
    background      = true;
    may_crit        = true;
    proc = true; // it's proc; therefore it cannot trigger main_gauche for chain-procs
  }

  bool procs_poison() const
  { return false; }
};

struct blade_flurry_attack_t : public rogue_attack_t
{
  blade_flurry_attack_t( rogue_t* p ) :
    rogue_attack_t( "blade_flurry_attack", p, p -> find_spell( 22482 ) )
  {
    may_miss = may_crit = proc = callbacks = may_dodge = may_parry = may_block = false;
    background = true;
    aoe = p -> spec.blade_flurry -> effectN( 4 ).base_value();
    weapon = &p -> main_hand_weapon;
    weapon_multiplier = 0;
    if ( p -> perk.enhanced_blade_flurry -> ok() )
      aoe = -1;
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

} // end namespace actions

inline bool rogue_t::poisoned_enemy( player_t* target ) const
{
  const rogue_td_t* td = get_target_data( target );

  if ( td -> dots.deadly_poison -> is_ticking() )
    return true;

  if ( td -> debuffs.wound_poison -> check() )
    return true;

  if ( td -> debuffs.crippling_poison -> check() )
    return true;

  if ( td -> debuffs.leeching_poison -> check() )
    return true;

  return false;
}

// ==========================================================================
// Rogue Triggers
// ==========================================================================

void rogue_t::trigger_auto_attack( const action_state_t* state )
{
  if ( main_hand_attack -> execute_event || off_hand_attack -> execute_event )
    return;

  if ( ! state -> action -> harmful )
    return;

  melee_main_hand -> first = true;
  if ( melee_off_hand )
    melee_off_hand -> first = true;

  auto_attack -> execute();
}

inline void actions::rogue_attack_t::trigger_sinister_calling( dot_t* dot )
{
  dot -> current_tick++;
  dot -> tick();
  // Advances the time, so calculate new time to tick, num ticks, last tick factor
  if ( dot -> remains() > dot -> time_to_tick )
  {
    timespan_t remains = dot -> end_event -> remains() - dot -> time_to_tick;
    timespan_t tick_remains = dot -> tick_event ? dot -> tick_event -> remains() : timespan_t::zero();

    core_event_t::cancel( dot -> tick_event );

    // Only start a new tick event, if there are still over one tick left.
    if ( tick_remains > timespan_t::zero() && remains > tick_remains )
      dot -> tick_event = new ( *sim ) dot_tick_event_t( dot, tick_remains );
    // No tick event started, this is the last tick. Recalculate last tick
    // factor, should be 100% for Rogues always, since the dots do not scale
    // with haste
    else
      dot -> last_tick_factor = std::min( 1.0, ( dot -> time_to_tick - tick_remains + remains ) / dot -> time_to_tick );

    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s sinister_calling %s, remains=%.3f tick_remains=%.3f new_remains=%.3f last_tick_factor=%.f",
          player -> name(), dot -> current_action -> name(), dot -> remains().total_seconds(), tick_remains.total_seconds(),
          remains.total_seconds(), dot -> last_tick_factor );
    }

    // Restart a new end-event, now one tick closer
    core_event_t::cancel( dot -> end_event );
    dot -> end_event = new ( *sim ) dot_end_event_t( dot, remains );
  }
  // Last ongoing tick, expire the dot early
  else
    dot -> last_tick();
}

void rogue_t::trigger_sinister_calling( const action_state_t* state )
{
  if ( ! spec.sinister_calling -> ok() )
    return;

  if ( ! state -> action -> result_is_multistrike( state -> result ) )
    return;

  rogue_td_t* tdata = get_target_data( state -> target );
  if ( tdata -> dots.rupture -> is_ticking() )
    cast_attack( tdata -> dots.rupture -> current_action ) -> trigger_sinister_calling( tdata -> dots.rupture );

  if ( tdata -> dots.garrote -> is_ticking() )
    cast_attack( tdata -> dots.garrote -> current_action ) -> trigger_sinister_calling( tdata -> dots.garrote );

  if ( tdata -> dots.hemorrhage -> is_ticking() )
    cast_attack( tdata -> dots.hemorrhage -> current_action ) -> trigger_sinister_calling( tdata -> dots.hemorrhage );

  if ( tdata -> dots.crimson_tempest -> is_ticking() )
    cast_attack( tdata -> dots.crimson_tempest -> current_action ) -> trigger_sinister_calling( tdata -> dots.crimson_tempest );
}

void rogue_t::trigger_seal_fate( const action_state_t* state )
{
  if ( ! spec.seal_fate -> ok() )
    return;

  if ( state -> result != RESULT_CRIT )
    return;

  if ( state -> target != state -> action -> target )
    return;

  if ( cooldowns.seal_fate -> down() )
    return;

  trigger_combo_point_gain( state, 1, gains.seal_fate );

  cooldowns.seal_fate -> start( spec.seal_fate -> internal_cooldown() );
  procs.seal_fate -> occur();

  if ( buffs.t16_2pc_melee -> trigger() )
    procs.t16_2pc_melee -> occur();
}

void rogue_t::trigger_main_gauche( const action_state_t* state )
{
  if ( ! mastery.main_gauche -> ok() )
    return;

  if ( state -> action -> proc || ! state -> action -> callbacks )
    return;

  if ( ! state -> action -> weapon )
    return;

  if ( state -> action -> weapon -> slot != SLOT_MAIN_HAND )
    return;

  if ( ! rng().roll( cache.mastery_value() ) )
    return;

  active_main_gauche -> target = state -> target;
  active_main_gauche -> schedule_execute();
}

void rogue_t::trigger_combat_potency( const action_state_t* state )
{
  if ( ! spec.combat_potency -> ok() )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  if ( ! state -> action -> weapon )
    return;

  if ( state -> action -> weapon -> slot != SLOT_OFF_HAND )
    return;

  double chance = 0.2;
  if ( state -> action != active_main_gauche )
    chance *= state -> action -> weapon -> swing_time.total_seconds() / 1.4;

  if ( ! rng().roll( chance ) )
    return;

  // energy gain value is in the proc trigger spell
  resource_gain( RESOURCE_ENERGY,
                 spec.combat_potency -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_ENERGY ),
                 gains.combat_potency );
}

void rogue_t::trigger_energy_refund( const action_state_t* state )
{
  double energy_restored = state -> action -> resource_consumed * 0.80;

  resource_gain( RESOURCE_ENERGY, energy_restored, gains.energy_refund );
}

void rogue_t::trigger_ruthlessness( const action_state_t* state )
{
  if ( cooldowns.ruthlessness -> down() )
    return;

  if ( ! spec.ruthlessness -> ok() )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  actions::rogue_attack_t* attack = debug_cast<actions::rogue_attack_t*>( state -> action );
  if ( ! attack -> base_costs[ RESOURCE_COMBO_POINT ] )
    return;

  const actions::rogue_attack_state_t* s = actions::rogue_attack_t::cast_state( state );
  if ( s -> cp == 0 )
    return;

  if ( attack -> harmful )
  {
    timespan_t reduction = spell.ruthlessness -> effectN( 3 ).time_value() * s -> cp;

    cooldowns.adrenaline_rush -> ready -= reduction;
    cooldowns.killing_spree   -> ready -= reduction;
    cooldowns.sprint          -> ready -= reduction;
  }

  double cp_chance = spell.ruthlessness -> effectN( 1 ).pp_combo_points() * s -> cp / 100.0;
  if ( rng().roll( cp_chance ) )
  {
    // Ruthlessness does not refund a combo point, if Anticipation has
    // refreshed combo points to max upon using a finisher
    if ( resources.current[ RESOURCE_COMBO_POINT ] < resources.max[ RESOURCE_COMBO_POINT ] )
      trigger_combo_point_gain( state, spell.ruthlessness -> effectN( 1 ).trigger() -> effectN( 1 ).base_value(), gains.ruthlessness );
    else
      procs.combo_points_wasted -> occur();
  }

  double energy_chance = spell.ruthlessness -> effectN( 2 ).pp_combo_points() * s -> cp / 100.0;
  if ( rng().roll( energy_chance ) )
    resource_gain( RESOURCE_ENERGY,
                   spell.ruthlessness -> effectN( 2 ).trigger() -> effectN( 1 ).resource( RESOURCE_ENERGY ),
                   gains.ruthlessness );

  cooldowns.ruthlessness -> start( spell.ruthlessness -> internal_cooldown() );
}

void rogue_t::trigger_relentless_strikes( const action_state_t* state )
{
  if ( ! spell.relentless_strikes -> ok() )
    return;

  actions::rogue_attack_t* attack = debug_cast<actions::rogue_attack_t*>( state -> action );
  if ( ! attack -> base_costs[ RESOURCE_COMBO_POINT ] )
    return;

  const actions::rogue_attack_state_t* s = actions::rogue_attack_t::cast_state( state );
  if ( s -> cp == 0 )
    return;

  double chance = spec.relentless_strikes -> effectN( 1 ).pp_combo_points() * s -> cp / 100.0;
  if ( ! rng().roll( chance ) )
    return;

  resource_gain( RESOURCE_ENERGY,
                 spec.relentless_strikes -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_ENERGY ),
                 gains.relentless_strikes );
}

void rogue_t::trigger_venomous_wounds( const action_state_t* state )
{
  if ( ! spec.venomous_wounds -> ok() )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) && ! state -> action -> result_is_multistrike( state -> result ) )
    return;

  double chance = spec.venomous_wounds -> proc_chance();

  if ( ! rng().roll( chance ) )
    return;

  active_venomous_wound -> target = state -> target;
  active_venomous_wound -> schedule_execute();

  resource_gain( RESOURCE_ENERGY,
                 spec.venomous_wounds -> effectN( 2 ).base_value(),
                 gains.venomous_wounds );
}

void rogue_t::trigger_blade_flurry( const action_state_t* state )
{
  if ( ! buffs.blade_flurry -> check() )
    return;

  if ( ! state -> action -> weapon )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  if ( sim -> active_enemies == 1 )
    return;

  if ( state -> action -> n_targets() != 0 )
    return;

  // Note, unmitigated damage
  active_blade_flurry -> base_dd_min = state -> result_total;
  active_blade_flurry -> base_dd_max = state -> result_total;
  active_blade_flurry -> schedule_execute();
}

void rogue_t::trigger_shadow_reflection( const action_state_t* state )
{
  using namespace actions;

  if ( ! talent.shadow_reflection -> ok() )
    return;

  rogue_attack_t* attack = debug_cast<rogue_attack_t*>( state -> action );
  if ( attack -> ability_type == ABILITY_NONE )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  if ( buffs.shadow_reflection -> remains() < timespan_t::from_seconds( 8 ) )
    return;

  const rogue_attack_state_t* rs = debug_cast<const rogue_attack_state_t*>( state );

  new ( *sim ) shadow_reflect_event_t( rs -> cp, state -> action );
  if ( sim -> debug )
    sim -> out_debug.printf( "%s shadow_reflection recording %s, cp=%d", name(), state -> action -> name(), rs -> cp );
}

void rogue_t::trigger_combo_point_gain( const action_state_t* state, int cp_override, gain_t* gain )
{
  using namespace actions;

  assert( state || cp_override > 0 );

  rogue_attack_t* attack = state ? debug_cast<rogue_attack_t*>( state -> action ) : 0;
  int n_cp = 0;
  if ( cp_override == -1 )
  {
    if ( ! attack -> adds_combo_points )
      return;

    n_cp = attack -> adds_combo_points;
  }
  else
    n_cp = cp_override;

  int fill = static_cast<int>( resources.max[RESOURCE_COMBO_POINT] - resources.current[RESOURCE_COMBO_POINT] );
  int added = std::min( fill, n_cp );
  int overflow = n_cp - added;
  int anticipation_added = 0;
  int anticipation_overflow = 0;
  if ( overflow > 0 && talent.anticipation -> ok() )
  {
    int anticipation_fill =  buffs.anticipation -> max_stack() - buffs.anticipation -> check();
    anticipation_added = std::min( anticipation_fill, overflow );
    anticipation_overflow = overflow - anticipation_added;
    if ( anticipation_added > 0 || anticipation_overflow > 0 )
      overflow = 0;
  }

  gain_t* gain_obj = gain;
  if ( gain_obj == 0 && attack && attack -> cp_gain )
    gain_obj = attack -> cp_gain;

  if ( added > 0 )
  {
    resource_gain( RESOURCE_COMBO_POINT, added,
        gain_obj ? gain_obj : 0,
        state ? state -> action : 0 );
  }

  if ( anticipation_added > 0 )
  {
    buffs.anticipation -> trigger( anticipation_added );
    if ( gain_obj )
      gain_obj -> add( RESOURCE_COMBO_POINT, anticipation_added, 0 );
  }

  for ( int i = 0; i < overflow; i++ )
    procs.combo_points_wasted -> occur();

  for ( int i = 0; i < anticipation_overflow; i++ )
    procs.anticipation_wasted -> occur();

  if ( sim -> log )
  {
    std::string cp_name;
    if ( gain != 0)
      cp_name = gain -> name_str;
    else
      cp_name = state -> action -> name();

    if ( anticipation_added > 0 || anticipation_overflow > 0 )
      sim -> out_log.printf( "%s gains %d (%d) anticipation charges from %s (%d)",
          name(), anticipation_added, anticipation_overflow, cp_name.c_str(), buffs.anticipation -> check() );
  }

  if ( event_premeditation )
    core_event_t::cancel( event_premeditation );

  assert( resources.current[ RESOURCE_COMBO_POINT ] <= 5 );
}

void rogue_t::spend_combo_points( const action_state_t* state )
{
  if ( state -> action -> base_costs[ RESOURCE_COMBO_POINT ] == 0 )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  if ( buffs.fof_fod -> up() )
    return;

  state -> action -> stats -> consume_resource( RESOURCE_COMBO_POINT, resources.current[ RESOURCE_COMBO_POINT ] );
  resource_loss( RESOURCE_COMBO_POINT, resources.current[ RESOURCE_COMBO_POINT ], 0, state ? state -> action : 0 );

  if ( state -> action -> harmful && buffs.anticipation -> stack() > 0 )
  {
    if ( sim -> log )
      sim -> out_log.printf( "%s replenishes %d combo_points through anticipation",
          name(), buffs.anticipation -> check() );

    resource_gain( RESOURCE_COMBO_POINT, buffs.anticipation -> check(), 0, state ? state -> action : 0 );
    buffs.anticipation -> expire();
  }

  if ( event_premeditation )
    core_event_t::cancel( event_premeditation );
}

void rogue_t::trigger_t17_4pc_combat( const action_state_t* state )
{
  using namespace actions;

  if ( ! sets.has_set_bonus( ROGUE_COMBAT, T17, B4 ) )
    return;

  rogue_attack_t* attack = state ? debug_cast<rogue_attack_t*>( state -> action ) : 0;
  if ( attack -> base_costs[ RESOURCE_COMBO_POINT ] == 0 )
    return;

  if ( ! attack -> harmful )
    return;

  if ( ! attack -> result_is_hit( state -> result ) )
    return;

  if ( ! rng().roll( sets.set( ROGUE_COMBAT, T17, B4 ) -> proc_chance() ) )
    return;

  trigger_combo_point_gain( state, buffs.deceit -> data().effectN( 2 ).base_value(), gains.deceit );
  buffs.deceit -> trigger();
}

void rogue_t::trigger_t17_4pc_subtlety( const action_state_t* state )
{
  using namespace actions;

  if ( ! sets.has_set_bonus( ROGUE_SUBTLETY, T17, B4 ) )
    return;

  rogue_attack_t* attack = state ? debug_cast<rogue_attack_t*>( state -> action ) : 0;
  if ( attack -> base_costs[ RESOURCE_COMBO_POINT ] == 0 )
    return;

  if ( ! attack -> result_is_hit( state -> result ) )
    return;

  if ( ! buffs.shadow_strikes -> up() )
    return;

  trigger_combo_point_gain( state, buffs.shadow_strikes-> data().effectN( 1 ).base_value(), gains.shadow_strikes );
}

namespace buffs {
// ==========================================================================
// Buffs
// ==========================================================================

struct shadow_dance_t : public buff_t
{
  shadow_dance_t( rogue_t* p ) :
    buff_t( buff_creator_t( p, "shadow_dance", p -> find_specialization_spell( "Shadow Dance" ) )
            .cd( timespan_t::zero() )
            .duration( p -> find_specialization_spell( "Shadow Dance" ) -> duration() +
                       p -> sets.set( SET_MELEE, T13, B4 ) -> effectN( 1 ).time_value() +
                       p -> perk.enhanced_shadow_dance -> effectN( 1 ).time_value() ) )
  { }

  void expire_override()
  {
    buff_t::expire_override();

    rogue_t* rogue = debug_cast<rogue_t*>( player );
    rogue -> buffs.shadow_strikes -> trigger();
  }
};
struct shadow_reflection_t : public buff_t
{
  shadow_reflection_t( rogue_t* p ) :
    buff_t( buff_creator_t( p, "shadow_reflection", p -> talent.shadow_reflection ).cd( timespan_t::zero() ) )
  { }

  void execute( int stacks, double value, timespan_t duration )
  {
    buff_t::execute( stacks, value, duration );

    rogue_t* rogue = debug_cast<rogue_t*>( player );

    rogue -> shadow_reflection -> target = rogue -> target;
    rogue -> shadow_reflection -> summon( rogue -> talent.shadow_reflection -> duration() );
  }
};

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

struct insight_buff_t : public buff_t
{
  rogue_t* p;
  // Flag to indicate Bandit's Guile is elevating the insight, meaning bandit's
  // guile itself shouldnt be expired. Natural expiration of an insight buff
  // will also expire Bandit's Guile, starting the cycle over.
  bool insight_elevate;

  insight_buff_t( rogue_t* player, const buff_creator_t& creator ) :
    buff_t( creator ), p( player ), insight_elevate( false )
  { }

  void expire_override()
  {
    buff_t::expire_override();

    if ( ! insight_elevate )
      p -> buffs.bandits_guile -> expire();
  }

  void reset()
  {
    buff_t::reset();

    insight_elevate = false;
  }

  void insight_elevate_expire()
  {
    insight_elevate = true;
    expire();
    insight_elevate = false;
  }
};

struct bandits_guile_t : public buff_t
{
  bandits_guile_t( rogue_t* p ) :
    buff_t( buff_creator_t( p, "bandits_guile", p -> spec.bandits_guile )
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
    {
      p -> buffs.shallow_insight -> trigger();
    }
    else if ( current_stack == 8 )
    {
      p -> buffs.shallow_insight -> insight_elevate_expire();
      p -> buffs.moderate_insight -> trigger();
    }
    else if ( current_stack == 12 )
    {
      if ( p -> buffs.deep_insight -> check() )
        return;

      p -> buffs.moderate_insight -> insight_elevate_expire();
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
    actions::break_stealth( rogue );
  }
};

struct vanish_t : public buff_t
{
  rogue_t* rogue;

  vanish_t( rogue_t* r ) :
    buff_t( buff_creator_t( r, "vanish", r -> find_spell( 11327 ) ) ),
    rogue( r )
  {
    buff_duration += r -> glyph.vanish -> effectN( 1 ).time_value();
  }

  void execute( int stacks, double value, timespan_t duration )
  {
    buff_t::execute( stacks, value, duration );

    rogue -> buffs.master_of_subtlety -> expire();
    rogue -> buffs.master_of_subtlety_passive -> trigger();
  }

  void expire_override()
  {
    buff_t::expire_override();

    rogue -> buffs.master_of_subtlety_passive -> expire();
    rogue -> buffs.master_of_subtlety -> trigger();
  }
};

struct stealth_t : public buff_t
{
  rogue_t* rogue;

  stealth_t( rogue_t* r ) :
    buff_t( buff_creator_t( r, "stealth", r -> find_spell( 1784 ) ) ),
    rogue( r )
  { }

  void execute( int stacks, double value, timespan_t duration )
  {
    buff_t::execute( stacks, value, duration );

    rogue -> buffs.master_of_subtlety -> expire();
    rogue -> buffs.master_of_subtlety_passive -> trigger();
  }

  void expire_override()
  {
    buff_t::expire_override();

    rogue -> buffs.master_of_subtlety_passive -> expire();
    rogue -> buffs.master_of_subtlety -> trigger();
  }
};

struct rogue_poison_buff_t : public buff_t
{
  rogue_poison_buff_t( rogue_td_t& r, const std::string& name, const spell_data_t* spell ) :
    buff_t( buff_creator_t( r, name, spell ) )
  { }

  void execute( int stacks, double value, timespan_t duration )
  {
    rogue_t* rogue = debug_cast< rogue_t* >( source );
    if ( ! rogue -> poisoned_enemy( player ) )
      rogue -> poisoned_enemies++;

    buff_t::execute( stacks, value, duration );
  }

  void expire_override()
  {
    buff_t::expire_override();

    rogue_t* rogue = debug_cast< rogue_t* >( source );
    if ( ! rogue -> poisoned_enemy( player ) )
      rogue -> poisoned_enemies--;
  }
};

struct wound_poison_t : public rogue_poison_buff_t
{
  wound_poison_t( rogue_td_t& r ) :
    rogue_poison_buff_t( r, "wound_poison", r.source -> find_spell( 8680 ) )
  { }
};

struct crippling_poison_t : public rogue_poison_buff_t
{
  crippling_poison_t( rogue_td_t& r ) :
    rogue_poison_buff_t( r, "crippling_poison", r.source -> find_spell( 3409 ) )
  { }
};

struct leeching_poison_t : public rogue_poison_buff_t
{
  leeching_poison_t( rogue_td_t& r ) :
    rogue_poison_buff_t( r, "leeching_poison", r.source -> find_spell( 112961 ) )
  { }
};

} // end namespace buffs

// ==========================================================================
// Rogue Targetdata Definitions
// ==========================================================================

rogue_td_t::rogue_td_t( player_t* target, rogue_t* source ) :
  actor_pair_t( target, source ),
  dots( dots_t() ),
  debuffs( debuffs_t() )
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
                                           source -> sets.set( SET_MELEE, T13, B4 ) -> effectN( 3 ).time_value() +
                                           source -> glyph.vendetta -> effectN( 2 ).time_value() )
                               .default_value( vd -> effectN( 1 ).percent() + source -> glyph.vendetta -> effectN( 1 ).percent() );

  debuffs.wound_poison = new buffs::wound_poison_t( *this );
  debuffs.crippling_poison = new buffs::crippling_poison_t( *this );
  debuffs.leeching_poison = new buffs::leeching_poison_t( *this );
  debuffs.enhanced_crimson_tempest = buff_creator_t( *this, "enhanced_crimson_tempest", source -> find_spell( 157561 ) );
}

// ==========================================================================
// Rogue Pet Definition
// ==========================================================================

using namespace actions;

struct shadow_reflection_pet_t : public pet_t
{
  struct shadow_reflection_attack_t : public melee_attack_t
  {
    rogue_attack_t* source_action;
    position_e requires_position;

    shadow_reflection_attack_t( const std::string& name, player_t* p, const spell_data_t* spell, const std::string& = std::string() ) :
      melee_attack_t( name, p, spell ),
      source_action( 0 ),
      requires_position( POSITION_NONE )
    {
      weapon = &( o() -> main_hand_weapon );
      background = may_crit = special = tick_may_crit = true;
      hasted_ticks = may_glance = callbacks = false;

      for ( size_t i = 1; i <= data().effect_count(); i++ )
      {
        const spelleffect_data_t& effect = data().effectN( i );

        if ( effect.type() == E_APPLY_AURA && effect.subtype() == A_PERIODIC_DAMAGE )
          base_ta_adder = effect.bonus( player );
        else if ( effect.type() == E_SCHOOL_DAMAGE )
          base_dd_adder = effect.bonus( player );
      }
    }

    void snapshot_internal( action_state_t* state, uint32_t flags, dmg_e rt )
    {
      assert( source_action );

      // Sooo ... snapshot the state of the ability to be mimiced from the
      // rogue itself. This should get us the correct multipliers etc, so the
      // shadow reflection mimic ability does not need them, making the mimiced
      // abilities programmatically much simpler.
      o() -> reflection_attack = true;
      o() -> cache.invalidate_all();
      source_action -> snapshot_internal( state, flags, rt );

      // Finally, the Shadow Relfection mimic abilities _do not_ get the
      // source's target specific abilities (find weakness, sanguinary vein),
      // so we need to re-snapshot target specific multipliers using the Shadow
      // Reflection's own action.
      if ( flags & STATE_TGT_MUL_DA )
        state -> target_da_multiplier = composite_target_da_multiplier( state -> target );

      if ( flags & STATE_TGT_MUL_TA )
        state -> target_ta_multiplier = composite_target_ta_multiplier( state -> target );

      // For Crit multipliers, note that we presume that any crit multipliers
      // on the owner are valid for the Shadow Reflection
      if ( flags & STATE_TGT_CRIT )
        state -> target_crit = composite_target_crit( state -> target ) * source_action -> composite_crit_multiplier();

      o() -> reflection_attack = false;
      o() -> cache.invalidate_all();
    }

    static rogue_attack_state_t* cast_state( action_state_t* st )
    { return debug_cast< rogue_attack_state_t* >( st ); }

    static const rogue_attack_state_t* cast_state( const action_state_t* st )
    { return debug_cast< const rogue_attack_state_t* >( st ); }

    action_state_t* new_state()
    { return new rogue_attack_state_t( this, target ); }

    rogue_t* o()
    { return debug_cast<rogue_t*>( player -> cast_pet() -> owner ); }

    double attack_direct_power_coefficient( const action_state_t* s ) const
    {
      if ( base_costs[ RESOURCE_COMBO_POINT ] > 0 )
        return attack_power_mod.direct * cast_state( s ) -> cp;
      return melee_attack_t::attack_direct_power_coefficient( s );
    }

    double attack_tick_power_coefficient( const action_state_t* s ) const
    {
      if ( base_costs[ RESOURCE_COMBO_POINT ] > 0 )
        return attack_power_mod.tick * cast_state( s ) -> cp;
      return melee_attack_t::attack_tick_power_coefficient( s );
    }

    double spell_direct_power_coefficient( const action_state_t* s ) const
    {
      if ( base_costs[ RESOURCE_COMBO_POINT ] > 0 )
        return spell_power_mod.direct * cast_state( s ) -> cp;
      return melee_attack_t::spell_direct_power_coefficient( s );
    }

    double spell_tick_power_coefficient( const action_state_t* s ) const
    {
      if ( base_costs[ RESOURCE_COMBO_POINT ] > 0 )
        return spell_power_mod.tick * cast_state( s ) -> cp;
      return melee_attack_t::spell_tick_power_coefficient( s );
    }

    double bonus_da( const action_state_t* s ) const
    {
      if ( base_costs[ RESOURCE_COMBO_POINT ] > 0 )
        return base_dd_adder * cast_state( s ) -> cp;
      return melee_attack_t::bonus_da( s );
    }

    double bonus_ta( const action_state_t* s ) const
    {
      if ( base_costs[ RESOURCE_COMBO_POINT ] > 0 )
        return base_ta_adder * cast_state( s ) -> cp;
      return melee_attack_t::bonus_ta( s );
    }

    // Always ready to serve the master, except when an ability requires a
    // position we dont have!
    bool ready()
    {
      if ( requires_position != POSITION_NONE && player -> position() != requires_position )
        return false;

      return true;
    }

    // Shadow Reflection has no cooldowns on abilities I imagine ...
    void update_ready( timespan_t )
    { }

    // Shadow Reflection does not consume resources, it has none ..
    void consume_resource()
    { }

    void cancel()
    {
      melee_attack_t::cancel();

      // Bail out early if not a dot.
      if ( dot_duration == timespan_t::zero() )
        return;

      // Cancel ticking dots on targets when SR goes away (basically whenever
      // the action is canceled is sufficient)
      for ( size_t i = 0, end = sim -> target_non_sleeping_list.size(); i < end; i++ )
      {
        dot_t* d = target_specific_dot[ sim -> target_non_sleeping_list[ i ] ];
        if ( d && d -> is_ticking() )
          d -> cancel();
      }
    }
  };

  struct sr_eviscerate_t : public shadow_reflection_attack_t
  {
    sr_eviscerate_t( shadow_reflection_pet_t* p ) :
      shadow_reflection_attack_t( "eviscerate", p, p -> find_spell( 2098 ) )
    {
      base_costs[ RESOURCE_COMBO_POINT ] = 1;
      attack_power_mod.direct = 0.577;
      weapon_multiplier = 0;
    }
  };

  struct sr_sinister_strike_t : public shadow_reflection_attack_t
  {
    sr_sinister_strike_t( shadow_reflection_pet_t* p ) :
      shadow_reflection_attack_t( "sinister_strike", p, p -> find_spell( 1752 ) )
    { }
  };

  struct sr_revealing_strike_t : public shadow_reflection_attack_t
  {
    sr_revealing_strike_t( shadow_reflection_pet_t* p ) :
      shadow_reflection_attack_t( "revealing_strike", p, p -> find_spell( 84617 ) )
    { }
  };

  struct sr_ambush_t : public shadow_reflection_attack_t
  {
    sr_ambush_t( shadow_reflection_pet_t* p ) :
      shadow_reflection_attack_t( "ambush", p, p -> find_spell( 8676 ) )
    {
      requires_position = POSITION_BACK;
    }
  };

  struct sr_hemorrhage_t : public shadow_reflection_attack_t
  {
    sr_hemorrhage_t( shadow_reflection_pet_t* p ) :
      shadow_reflection_attack_t( "hemorrhage", p, p -> find_spell( 16511 ) )
    { }
  };

  struct sr_backstab_t : public shadow_reflection_attack_t
  {
    sr_backstab_t( shadow_reflection_pet_t* p ) :
      shadow_reflection_attack_t( "backstab", p, p -> find_spell( 53 ) )
    {
      requires_position = POSITION_BACK;
    }
  };

  struct sr_dispatch_t : public shadow_reflection_attack_t
  {
    sr_dispatch_t( shadow_reflection_pet_t* p ) :
      shadow_reflection_attack_t( "dispatch", p, p -> find_spell( 111240 ) )
    { }
  };

  struct sr_mutilate_t : public shadow_reflection_attack_t
  {
    struct sr_mutilate_strike_t : public shadow_reflection_attack_t
    {
      sr_mutilate_strike_t( shadow_reflection_pet_t* p, const char* name, const spell_data_t* s ) :
        shadow_reflection_attack_t( name, p, s )
      {
        background = true;
        may_miss = may_dodge = may_parry = false;
      }
    };

    sr_mutilate_strike_t* mh_strike;
    sr_mutilate_strike_t* oh_strike;

    sr_mutilate_t( shadow_reflection_pet_t* p ) :
      shadow_reflection_attack_t( "mutilate", p, p -> find_spell( 1329 ) ),
      mh_strike( 0 ), oh_strike( 0 )
    {
      may_crit = false;

      mh_strike = new sr_mutilate_strike_t( p, "mutilate_mh", data().effectN( 2 ).trigger() );
      mh_strike -> weapon = &( p -> o() -> main_hand_weapon );
      add_child( mh_strike );

      oh_strike = new sr_mutilate_strike_t( p, "mutilate_oh", data().effectN( 3 ).trigger() );
      oh_strike -> weapon = &( p -> o() -> off_hand_weapon );
      add_child( oh_strike );
    }

    void execute()
    {
      shadow_reflection_attack_t::execute();

      if ( result_is_hit( execute_state -> result ) )
      {
        mh_strike -> schedule_execute( mh_strike -> get_state( execute_state ) );
        oh_strike -> schedule_execute( oh_strike -> get_state( execute_state ) );
      }
    }
  };

  struct sr_envenom_t : public shadow_reflection_attack_t
  {
    sr_envenom_t( shadow_reflection_pet_t* p ) :
      shadow_reflection_attack_t( "envenom", p, p -> find_spell( 32645 ) )
    {
      base_costs[ RESOURCE_COMBO_POINT ] = 1;
      attack_power_mod.direct = 0.306;
      dot_duration = timespan_t::zero();
      weapon_multiplier = 0.0;
    }
  };

  struct sr_rupture_t : public shadow_reflection_attack_t
  {
    sr_rupture_t( shadow_reflection_pet_t* p ) :
      shadow_reflection_attack_t( "rupture", p, p -> find_spell( 1943 ) )
    {
      base_costs[ RESOURCE_COMBO_POINT ] = 1;
      may_crit              = false;
      tick_may_crit         = true;
      dot_behavior          = DOT_REFRESH;
      weapon_multiplier = 0.0;
    }

    timespan_t composite_dot_duration( const action_state_t* s ) const override
    {
      timespan_t duration = data().duration();

      const rogue_attack_state_t* state = debug_cast<const rogue_attack_state_t*>( s );

      duration += duration * state -> cp;

      return duration;
    }
  };

  struct sr_fan_of_knives_t : public shadow_reflection_attack_t
  {
    sr_fan_of_knives_t( shadow_reflection_pet_t* p ) :
      shadow_reflection_attack_t( "fan_of_knives", p, p -> find_spell( 51723 ) )
    {
      weapon_multiplier = 0;
      aoe = -1;
    }
  };

  struct sr_crimson_tempest_t : public shadow_reflection_attack_t
  {
    struct sr_crimson_tempest_dot_t : public residual_periodic_action_t<attack_t>
    {
      sr_crimson_tempest_dot_t( shadow_reflection_pet_t* p ) :
        residual_periodic_action_t<attack_t>( "crimson_tempest_dot", p, p -> find_spell( 122233 ) )
      { }

      action_state_t* new_state()
      { return new residual_periodic_state_t( this, target ); }
    };

    sr_crimson_tempest_dot_t* dot;

    sr_crimson_tempest_t( shadow_reflection_pet_t* p ) :
      shadow_reflection_attack_t( "crimson_tempest", p, p -> find_spell( 121411 ) ),
      dot( new sr_crimson_tempest_dot_t( p ) )
    {
      aoe = -1;
      base_costs[ RESOURCE_COMBO_POINT ] = 1;
      attack_power_mod.direct = 0.0602;
      weapon_multiplier = 0;
      add_child( dot );
    }

    void impact( action_state_t* s )
    {
      shadow_reflection_attack_t::impact( s );

      if ( result_is_hit( s -> result ) )
        residual_action::trigger( dot, s -> target, s -> result_amount * dot -> data().effectN( 1 ).percent() );
    }
  };

  struct sr_killing_spree_t : public shadow_reflection_attack_t
  {
    struct sr_ks_tick_t : public shadow_reflection_attack_t
    {
      sr_ks_tick_t( shadow_reflection_pet_t* p, const char* name, const spell_data_t* s ) :
        shadow_reflection_attack_t( name, p, s )
      {
        school      = SCHOOL_PHYSICAL;
        direct_tick = true;
      }
    };

    melee_attack_t* attack_mh, *attack_oh;

    sr_killing_spree_t( shadow_reflection_pet_t* p ) :
      shadow_reflection_attack_t( "killing_spree", p, p -> find_spell( 51690 ) ),
      attack_mh( 0 ), attack_oh( 0 )
    {
      may_miss = may_crit = false;
      channeled = tick_zero = true;
      const spell_data_t* spell = p -> find_spell( 57841 );

      attack_mh = new sr_ks_tick_t( p, "killing_spree_mh", spell );
      add_child( attack_mh );

      if ( p -> o() -> off_hand_weapon.type != WEAPON_NONE )
      {
        attack_oh = new sr_ks_tick_t( p, "killing_spree_oh", spell -> effectN( 2 ).trigger() );
        attack_oh -> weapon = &( p -> o() -> off_hand_weapon );
        add_child( attack_oh );
      }
    }

    timespan_t tick_time( double ) const
    { return base_tick_time; }

    virtual void tick( dot_t* d )
    {
      shadow_reflection_attack_t::tick( d );

      attack_mh -> pre_execute_state = attack_mh -> get_state( d -> state );
      attack_mh -> execute();

      if ( attack_oh && result_is_hit( attack_mh -> execute_state -> result ) )
      {
        attack_oh -> pre_execute_state = attack_oh -> get_state( d -> state );
        attack_oh -> execute();
      }
    }
  };

  std::vector<shadow_reflection_attack_t*> attacks;

  shadow_reflection_pet_t( rogue_t* owner ) :
    pet_t( owner -> sim, owner, "shadow_reflection", true, true ),
    attacks( ABILITY_MAX )
  { regen_type = REGEN_DISABLED; }

  rogue_t* o()
  { return debug_cast<rogue_t*>( owner ); }

  void init_spells()
  {
    pet_t::init_spells();

    // Sigh ...
    attacks[ AMBUSH           ] = new sr_ambush_t( this );
    attacks[ CRIMSON_TEMPEST  ] = new sr_crimson_tempest_t( this );

    _spec = ROGUE_ASSASSINATION;
    attacks[ ENVENOM          ] = new sr_envenom_t( this );
    attacks[ MUTILATE         ] = new sr_mutilate_t( this );
    attacks[ DISPATCH         ] = new sr_dispatch_t( this );
    attacks[ FAN_OF_KNIVES    ] = new sr_fan_of_knives_t( this );

    _spec = ROGUE_COMBAT;
    attacks[ EVISCERATE       ] = new sr_eviscerate_t( this );
    attacks[ SINISTER_STRIKE  ] = new sr_sinister_strike_t( this );
    attacks[ REVEALING_STRIKE ] = new sr_revealing_strike_t( this );
    attacks[ KILLING_SPREE    ] = new sr_killing_spree_t( this );

    _spec = ROGUE_SUBTLETY;
    attacks[ HEMORRHAGE       ] = new sr_hemorrhage_t( this );
    attacks[ BACKSTAB         ] = new sr_backstab_t( this );
    attacks[ RUPTURE          ] = new sr_rupture_t( this );

    _spec = SPEC_NONE;
  }
};

inline void shadow_reflect_event_t::execute()
{
  using namespace actions;

  rogue_t* rogue = debug_cast<rogue_t*>( source_action -> player );
  shadow_reflection_pet_t* sr = debug_cast<shadow_reflection_pet_t*>( rogue -> shadow_reflection );

  if ( sr -> is_sleeping() )
    return;

  rogue_attack_t* rogue_attack = debug_cast<rogue_attack_t*>( source_action );

  if ( sim().debug )
    sim().out_debug.printf( "%s mimics %s, cp=%d", rogue -> name(), source_action -> name(), combo_points );

  // Link the source action to the mimiced action, so snapshot_state() and
  // update_state() actually use the owner's current stats to drive the
  // ability
  shadow_reflection_pet_t::shadow_reflection_attack_t* attack = sr -> attacks[ rogue_attack -> ability_type ];
  attack -> source_action = rogue_attack;

  // Snapshot a state for the mimiced ability, note that this snapshots
  // based on the owner's current stats, and does some adjustments (see
  // shadow_reflection_attack_t::snapshot_internal()).
  action_state_t* our_state = attack -> get_state();
  attack -> snapshot_state( our_state, attack -> amount_type( our_state ) );

  // Aand update combo points for us from the sequence, since shadow
  // reflection has no combo points.
  debug_cast<rogue_attack_state_t*>( our_state ) -> cp = combo_points;

  // Finally, execute the mimiced ability ..
  attack -> schedule_execute( our_state );
}

// ==========================================================================
// Rogue Character Definition
// ==========================================================================

// rogue_t::composite_rating_multiplier =====================================

double rogue_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );
  switch ( rating )
  {
    case RATING_MULTISTRIKE:
      m *= 1.0 + spec.sinister_calling -> effectN( 2 ).percent();
      break;
    case RATING_MASTERY:
      m *= 1.0 + spec.master_poisoner -> effectN( 1 ).percent();
      break;
    case RATING_SPELL_HASTE:
    case RATING_MELEE_HASTE:
    case RATING_RANGED_HASTE:
      m *= 1.0 + spec.combat_potency -> effectN( 2 ).percent();
      break;
    default:
      break;
  }

  return m;
}

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

// rogue_t::composite_melee_crit =========================================

double rogue_t::composite_melee_crit() const
{
  double crit = player_t::composite_melee_crit();

  crit += spell.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// rogue_t::composite_spell_crit =========================================

double rogue_t::composite_spell_crit() const
{
  double crit = player_t::composite_spell_crit();

  crit += spell.critical_strikes -> effectN( 1 ).percent();

  return crit;
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

  if ( ! reflection_attack )
  {
    // TODO-WOD: Enhance Stealth is additive or multiplicative?
    if ( buffs.master_of_subtlety -> check() || buffs.master_of_subtlety_passive -> check() )
      m *= 1.0 + spec.master_of_subtlety -> effectN( 1 ).percent() + perk.enhanced_stealth -> effectN( 1 ).percent();

    m *= 1.0 + buffs.shallow_insight -> value();

    m *= 1.0 + buffs.moderate_insight -> value();

    m *= 1.0 + buffs.deep_insight -> value();

    if ( perk.empowered_bandits_guile -> ok() && buffs.deep_insight -> check() )
      m *= 1.0 + perk.empowered_bandits_guile -> effectN( 1 ).percent();
  }

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

  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* def       = get_action_priority_list( "default" );

  std::vector<std::string> item_actions = get_item_actions();
  std::vector<std::string> profession_actions = get_profession_actions();
  std::vector<std::string> racial_actions = get_racial_actions();

  clear_action_priority_lists();

  // Flask
  if ( sim -> allow_flasks && level >= 80 )
  {
    std::string flask_action = "flask,type=";
    if ( level > 90 )
      flask_action += "greater_draenic_agility_flask";
    else
      flask_action += ( level >= 85 ) ? "spring_blossoms" : ( ( level >= 80 ) ? "winds" : "" );

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
  std::string poison_str = "apply_poison,lethal=deadly,nonlethal=";
  if ( talent.leeching_poison -> ok() )
    poison_str += "leeching";
  else
    poison_str += "crippling";

  precombat -> add_action( poison_str );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  std::string potion_name;
  if ( sim -> allow_potions && level >= 80 )
  {
    if ( level > 90 )
      potion_name = "draenic_agility";
    else if ( level > 85 )
      potion_name = "virmens_bite";
    else
      potion_name = "tolvir";

    precombat -> add_action( "potion,name=" + potion_name );
  }

  precombat -> add_action( this, "Stealth" );

  // In-combat potion
  if ( sim -> allow_potions )
    def -> add_action( "potion,name=" + potion_name + ",if=buff.bloodlust.react|target.time_to_die<40" );

  def -> add_action( this, "Kick" );

  if ( specialization() == ROGUE_ASSASSINATION )
  {
    precombat -> add_talent( this, "Marked for Death" );
    precombat -> add_action( this, "Slice and Dice", "if=talent.marked_for_death.enabled" );

    def -> add_action( this, "Preparation", "if=!buff.vanish.up&cooldown.vanish.remains>60" );

    for ( size_t i = 0; i < item_actions.size(); i++ )
      def -> add_action( item_actions[i] );

    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[i] == "arcane_torrent" )
        def -> add_action( racial_actions[i] + ",if=energy<60" );
      else
        def -> add_action( racial_actions[i] );
    }

    std::string vanish_expr = "if=time>10&!buff.stealth.up";
    def -> add_action( this, "Vanish", vanish_expr );
    def -> add_talent( this, "Shadow Reflection" );
    def -> add_action( this, "Rupture", "if=combo_points=5&ticks_remain<3" );
    def -> add_action( this, "Rupture", "cycle_targets=1,if=active_enemies>1&!ticking&combo_points=5" );
    def -> add_action( this, "Mutilate", "if=buff.stealth.up" );
    def -> add_talent( this, "Marked for Death", "if=combo_points=0" );
    def -> add_action( this, "Fan of Knives", "if=combo_points<5&active_enemies>=4" );
    def -> add_action( this, "Rupture", "if=(remains<2|(combo_points=5&remains<=(duration*0.3)))&active_enemies=1" );
    def -> add_action( this, "Vendetta" );
    def -> add_talent( this, "Death From Above", "if=combo_points>4" );
    def -> add_action( this, "Envenom", "if=(combo_points>4&buff.envenom.remains<2&(cooldown.death_from_above.remains>2|!talent.death_from_above.enabled))&active_enemies<4" );
    def -> add_action( this, "Mutilate", "if=target.health.pct>35&combo_points<5&active_enemies<5" );
    def -> add_action( this, "Dispatch", "if=(combo_points<5|(talent.anticipation.enabled&anticipation_charges<4))&active_enemies<4" );
    def -> add_action( this, "Mutilate", "if=active_enemies<5" );
  }

  else if ( specialization() == ROGUE_COMBAT )
  {
    precombat -> add_talent( this, "Marked for Death" );
    precombat -> add_action( this, "Slice and Dice", "if=talent.marked_for_death.enabled" );

    for ( size_t i = 0; i < item_actions.size(); i++ )
      def -> add_action( item_actions[i] );

    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[i] == "arcane_torrent" )
        def -> add_action( racial_actions[i] + ",if=energy<60" );
      else
        def -> add_action( racial_actions[i] );
    }

    def -> add_action( this, "Blade Flurry", "if=(active_enemies>=2&!buff.blade_flurry.up)|(active_enemies<2&buff.blade_flurry.up)" );
    def -> add_talent( this, "Shadow Reflection", "if=(cooldown.killing_spree.remains<10&combo_points>3)|buff.adrenaline_rush.up" );
    def -> add_action( this, "Ambush" );
    def -> add_action( this, "Killing Spree", "if=energy<50&(!talent.shadow_reflection.enabled|cooldown.shadow_reflection.remains>30|buff.shadow_reflection.remains>3)" );
    def -> add_action( this, "Adrenaline Rush", "if=energy<35" );

    // Rotation
    def -> add_action( this, "Slice and Dice", "if=buff.slice_and_dice.remains<2|(buff.slice_and_dice.remains<15&buff.bandits_guile.stack=11&combo_points>=4)" );
    def -> add_talent( this, "Marked for Death", "if=combo_points<=1&dot.revealing_strike.ticking&(!talent.shadow_reflection.enabled|buff.shadow_reflection.up|cooldown.shadow_reflection.remains>30)" );

    // Generate combo points, or use combo points
    def -> add_action( "call_action_list,name=generator,if=combo_points<5|(talent.anticipation.enabled&anticipation_charges<=4&buff.deep_insight.down)" );
    if ( level >= 3 )
      def -> add_action( "call_action_list,name=finisher,if=combo_points=5&(buff.deep_insight.up|!talent.anticipation.enabled|(talent.anticipation.enabled&anticipation_charges>=4))" );

    // Combo point generators
    action_priority_list_t* gen = get_action_priority_list( "generator", "Combo point generators" );
    gen -> add_action( this, "Revealing Strike", "if=ticks_remain<2|buff.deep_insight.up|(buff.bandits_guile.stack=11&cooldown.adrenaline_rush.remains<10)" );
    gen -> add_action( this, "Sinister Strike" );

    // Combo point finishers
    action_priority_list_t* finisher = get_action_priority_list( "finisher", "Combo point finishers" );
    finisher -> add_action( this, "Crimson Tempest", "if=active_enemies>7&dot.crimson_tempest_dot.ticks_remain<=1" );
    finisher -> add_action( this, "Eviscerate" );
  }
  else if ( specialization() == ROGUE_SUBTLETY )
  {
    precombat -> add_action( this, "Premeditation" );
    precombat -> add_action( this, "Slice and Dice" );

    for ( size_t i = 0; i < item_actions.size(); i++ )
      def -> add_action( item_actions[i] + ",if=buff.shadow_dance.up" );

    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[i] == "arcane_torrent" )
        def -> add_action( racial_actions[i] + ",if=energy<60" );
      else
        def -> add_action( racial_actions[i] + ",if=buff.shadow_dance.up" );
    }

    // Shadow Dancing and Vanishing and Marking for the Deathing
    def -> add_action( this, "Premeditation", "if=combo_points<=4" );
    def -> add_action( this, find_class_spell( "Ambush" ), "pool_resource", "for_next=1" );
    def -> add_action( this, "Ambush", "if=combo_points<5|(talent.anticipation.enabled&anticipation_charges<3)|(buff.sleight_of_hand.up&buff.sleight_of_hand.remains<=gcd)" );
    def -> add_action( this, find_class_spell( "Shadow Dance" ), "pool_resource", "for_next=1,extra_amount=75" );
    def -> add_action( this, "Shadow Dance", "if=energy>=75&buff.stealth.down&buff.vanish.down&debuff.find_weakness.down" );
    def -> add_action( this, find_class_spell( "Vanish" ), "pool_resource", "for_next=1,extra_amount=45" );
    def -> add_action( this, "Vanish", "if=energy>=45&energy<=75&combo_points<=3&buff.shadow_dance.down&buff.master_of_subtlety.down&debuff.find_weakness.down" );
    def -> add_talent( this, "Marked for Death", "if=combo_points=0" );
    def -> add_talent( this, "Shadow Reflection" );

    // Rotation
    def -> add_action( "run_action_list,name=generator,if=talent.anticipation.enabled&anticipation_charges<4&buff.slice_and_dice.up&dot.rupture.remains>2&(buff.slice_and_dice.remains<6|dot.rupture.remains<4)" );
    def -> add_action( "run_action_list,name=finisher,if=combo_points=5" );
    def -> add_action( "run_action_list,name=generator,if=combo_points<4|energy>80|talent.anticipation.enabled" );
    def -> add_action( "run_action_list,name=pool" );

    // Combo point generators
    action_priority_list_t* gen = get_action_priority_list( "generator", "Combo point generators" );
    gen -> add_action( this, find_class_spell( "Preparation" ), "run_action_list", "name=pool,if=buff.master_of_subtlety.down&buff.shadow_dance.down&debuff.find_weakness.down&(energy+cooldown.shadow_dance.remains*energy.regen<80|energy+cooldown.vanish.remains*energy.regen<60)" );
    gen -> add_action( this, "Fan of Knives", "if=active_enemies>=4" );
    gen -> add_action( this, "Hemorrhage", "if=(remains<8&target.time_to_die>10)|position_front" );
    gen -> add_talent( this, "Shuriken Toss", "if=energy<65&energy.regen<16" );
    gen -> add_action( this, "Backstab" );
    gen -> add_action( this, find_class_spell( "Preparation" ), "run_action_list", "name=pool" );

    // Combo point finishers
    action_priority_list_t* finisher = get_action_priority_list( "finisher", "Combo point finishers" );
    finisher -> add_action( this, "Slice and Dice", "if=buff.slice_and_dice.remains<4" );
    finisher -> add_action( this, "Rupture", "if=(!ticking|remains<duration*0.3)&active_enemies<3" );
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

  if ( name == "adrenaline_rush"     ) return new adrenaline_rush_t    ( this, options_str );
  if ( name == "ambush"              ) return new ambush_t             ( this, options_str );
  if ( name == "apply_poison"        ) return new apply_poison_t       ( this, options_str );
  if ( name == "auto_attack"         ) return new auto_melee_attack_t  ( this, options_str );
  if ( name == "backstab"            ) return new backstab_t           ( this, options_str );
  if ( name == "blade_flurry"        ) return new blade_flurry_t       ( this, options_str );
  if ( name == "burst_of_speed"      ) return new burst_of_speed_t     ( this, options_str );
  if ( name == "crimson_tempest"     ) return new crimson_tempest_t    ( this, options_str );
  if ( name == "death_from_above"    ) return new death_from_above_t   ( this, options_str );
  if ( name == "dispatch"            ) return new dispatch_t           ( this, options_str );
  if ( name == "envenom"             ) return new envenom_t            ( this, options_str );
  if ( name == "eviscerate"          ) return new eviscerate_t         ( this, options_str );
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
  if ( name == "shadow_dance"        ) return new shadow_dance_t       ( this, options_str );
  if ( name == "shadow_reflection"   ) return new shadow_reflection_t  ( this, options_str );
  if ( name == "shadowstep"          ) return new shadowstep_t         ( this, options_str );
  if ( name == "shiv"                ) return new shiv_t               ( this, options_str );
  if ( name == "shuriken_toss"       ) return new shuriken_toss_t      ( this, options_str );
  if ( name == "sinister_strike"     ) return new sinister_strike_t    ( this, options_str );
  if ( name == "slice_and_dice"      ) return new slice_and_dice_t     ( this, options_str );
  if ( name == "sprint"              ) return new sprint_t             ( this, options_str );
  if ( name == "stealth"             ) return new stealth_t            ( this, options_str );
  if ( name == "vanish"              ) return new vanish_t             ( this, options_str );
  if ( name == "vendetta"            ) return new vendetta_t           ( this, options_str );

  return player_t::create_action( name, options_str );
}

// rogue_t::create_expression ===============================================

expr_t* rogue_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "combo_points" )
    return make_ref_expr( name_str, resources.current[ RESOURCE_COMBO_POINT ] );
  else if ( util::str_compare_ci( name_str, "anticipation_charges" ) )
    return make_ref_expr( name_str, buffs.anticipation -> current_stack );

  return player_t::create_expression( a, name_str );
}

// rogue_t::init_base =======================================================

void rogue_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;

  resources.base[ RESOURCE_ENERGY ] = 100;
  resources.base[ RESOURCE_COMBO_POINT ] = 5;
  if ( main_hand_weapon.type == WEAPON_DAGGER && off_hand_weapon.type == WEAPON_DAGGER )
    resources.base[ RESOURCE_ENERGY ] += spec.assassins_resolve -> effectN( 1 ).base_value();
  //if ( sets.has_set_bonus( SET_MELEE, PVP, B2 ) )
  //  resources.base[ RESOURCE_ENERGY ] += 10;

  resources.base[ RESOURCE_ENERGY ] += glyph.energy -> effectN( 1 ).base_value();

  resources.base[ RESOURCE_ENERGY ] += talent.venom_rush -> effectN( 1 ).base_value();

  base_energy_regen_per_second = 10 * ( 1.0 + spec.vitality -> effectN( 1 ).percent() );

  base_gcd = timespan_t::from_seconds( 1.0 );
}

// rogue_t::init_spells =====================================================

void rogue_t::init_spells()
{
  player_t::init_spells();

  // Shared
  spec.relentless_strikes   = find_specialization_spell( "Relentless Strikes" );

  // Assassination
  spec.assassins_resolve    = find_specialization_spell( "Assassin's Resolve" );
  spec.blindside            = find_specialization_spell( "Blindside" );
  spec.cut_to_the_chase     = find_specialization_spell( "Cut to the Chase" );
  spec.improved_poisons     = find_specialization_spell( "Improved Poisons" );
  spec.master_poisoner      = find_specialization_spell( "Master Poisoner" );
  spec.seal_fate            = find_specialization_spell( "Seal Fate" );
  spec.venomous_wounds      = find_specialization_spell( "Venomous Wounds" );

  // Combat
  spec.bandits_guile        = find_specialization_spell( "Bandit's Guile" );
  spec.blade_flurry         = find_specialization_spell( "Blade Flurry" );
  spec.combat_potency       = find_specialization_spell( "Combat Potency" );
  spec.killing_spree        = find_specialization_spell( "Killing Spree" );
  spec.ruthlessness         = find_specialization_spell( "Ruthlessness" );
  spec.vitality             = find_specialization_spell( "Vitality" );

  // Subtlety
  spec.energetic_recovery   = find_specialization_spell( "Energetic Recovery" );
  spec.find_weakness        = find_specialization_spell( "Find Weakness" );
  spec.honor_among_thieves  = find_specialization_spell( "Honor Among Thieves" );
  spec.master_of_subtlety   = find_specialization_spell( "Master of Subtlety" );
  spec.sanguinary_vein      = find_specialization_spell( "Sanguinary Vein" );
  spec.shadow_dance         = find_specialization_spell( "Shadow Dance" );
  spec.sinister_calling     = find_specialization_spell( "Sinister Calling" );

  // Masteries
  mastery.potent_poisons    = find_mastery_spell( ROGUE_ASSASSINATION );
  mastery.main_gauche       = find_mastery_spell( ROGUE_COMBAT );
  mastery.executioner       = find_mastery_spell( ROGUE_SUBTLETY );

  // Misc spells
  spell.bandits_guile_value = find_spell( 84747 );
  spell.critical_strikes    = find_spell( 157442 );
  spell.death_from_above    = find_spell( 163786 );
  spell.fan_of_knives       = find_class_spell( "Fan of Knives" );
  spell.fleet_footed        = find_class_spell( "Fleet Footed" );
  spell.sprint              = find_class_spell( "Sprint" );
  spell.relentless_strikes  = find_spell( 58423 );
  spell.ruthlessness        = find_spell( 14161 );
  spell.ruthlessness_cp     = spec.ruthlessness -> effectN( 1 ).trigger();
  spell.shadow_focus        = find_spell( 112942 );
  spell.tier13_2pc          = find_spell( 105864 );
  spell.tier13_4pc          = find_spell( 105865 );
  spell.tier15_4pc          = find_spell( 138151 );
  spell.venom_rush          = find_spell( 156719 );

  // Glyphs
  glyph.disappearance       = find_glyph_spell( "Glyph of Disappearance" );
  glyph.energy              = find_glyph_spell( "Glyph of Energy" );
  glyph.hemorrhaging_veins  = find_glyph_spell( "Glyph of Hemorrhaging Veins" );
  glyph.kick                = find_glyph_spell( "Glyph of Kick" );
  glyph.sprint              = find_glyph_spell( "Glyph of Sprint" );
  glyph.vanish              = find_glyph_spell( "Glyph of Vanish" );
  glyph.vendetta            = find_glyph_spell( "Glyph of Vendetta" );

  talent.anticipation       = find_talent_spell( "Anticipation" );
  talent.burst_of_speed     = find_talent_spell( "Burst of Speed" );
  talent.death_from_above   = find_talent_spell( "Death from Above" );
  talent.leeching_poison    = find_talent_spell( "Leeching Poison" );
  talent.marked_for_death   = find_talent_spell( "Marked for Death" );
  talent.nightstalker       = find_talent_spell( "Nightstalker" );
  talent.shadow_focus       = find_talent_spell( "Shadow Focus" );
  talent.shadow_reflection  = find_talent_spell( "Shadow Reflection" );
  talent.shadowstep         = find_talent_spell( "Shadowstep" );
  talent.subterfuge         = find_talent_spell( "Subterfuge" );
  talent.venom_rush         = find_talent_spell( "Venom Rush" );

  // Assassination
  perk.improved_slice_and_dice     = find_perk_spell( "Improved Slice and Dice" );
  perk.enhanced_vendetta           = find_perk_spell( "Enhanced Vendetta" );
  perk.empowered_envenom           = find_perk_spell( "Empowered Envenom" );
  perk.enhanced_crimson_tempest    = find_perk_spell( "Enhanced Crimson Tempest" );

  // Combat
  perk.empowered_bandits_guile     = find_perk_spell( "Empowered Bandit's Guile" );
  perk.enhanced_blade_flurry       = find_perk_spell( "Enhanced Blade Flurry" );
  perk.improved_dual_wield         = find_perk_spell( "Improved Dual Wield" );
  perk.swift_poison                = find_perk_spell( "Swift Poison" );

  perk.empowered_fan_of_knives     = find_perk_spell( "Empowered Fan of Knives" );
  perk.enhanced_shadow_dance       = find_perk_spell( "Enhanced Shadow Dance" );
  perk.enhanced_stealth            = find_perk_spell( "Enhanced Stealth" );
  perk.enhanced_vanish             = find_perk_spell( "Enhanced Vanish" );

  auto_attack = new auto_melee_attack_t( this, "" );

  if ( spec.venomous_wounds -> ok() )
    active_venomous_wound = new venomous_wound_t( this );

  if ( mastery.main_gauche -> ok() )
    active_main_gauche = new main_gauche_t( this );

  if ( spec.blade_flurry -> ok() )
    active_blade_flurry = new blade_flurry_attack_t( this );
}

// rogue_t::init_gains ======================================================

void rogue_t::init_gains()
{
  player_t::init_gains();

  gains.adrenaline_rush         = get_gain( "adrenaline_rush"    );
  gains.combat_potency          = get_gain( "combat_potency"     );
  gains.deceit                  = get_gain( "deceit" );
  gains.empowered_fan_of_knives = get_gain( "empowered_fan_of_knives" );
  gains.energetic_recovery      = get_gain( "energetic_recovery" );
  gains.energy_refund           = get_gain( "energy_refund"      );
  gains.honor_among_thieves     = get_gain( "honor_among_thieves" );
  gains.legendary_daggers       = get_gain( "legendary_daggers" );
  gains.murderous_intent        = get_gain( "murderous_intent"   );
  gains.overkill                = get_gain( "overkill"           );
  gains.premeditation           = get_gain( "premeditation" );
  gains.recuperate              = get_gain( "recuperate"         );
  gains.relentless_strikes      = get_gain( "relentless_strikes" );
  gains.ruthlessness            = get_gain( "ruthlessness"       );
  gains.ruthlessness            = get_gain( "ruthlessness" );
  gains.seal_fate               = get_gain( "seal_fate" );
  gains.shadow_strikes          = get_gain( "shadow_strikes" );
  gains.t17_2pc_assassination   = get_gain( "t17_2pc_assassination" );
  gains.t17_4pc_assassination   = get_gain( "t17_4pc_assassination" );
  gains.venomous_wounds         = get_gain( "venomous_vim"       );
}

// rogue_t::init_procs ======================================================

void rogue_t::init_procs()
{
  player_t::init_procs();

  procs.anticipation_wasted      = get_proc( "anticipation_wasted" );
  procs.combo_points_wasted      = get_proc( "combo_points_wasted" );
  procs.honor_among_thieves      = get_proc( "honor_among_thieves" );
  procs.no_revealing_strike      = get_proc( "no_revealing_strike" );
  procs.seal_fate                = get_proc( "seal_fate"           );
  procs.t16_2pc_melee            = get_proc( "t16_2pc_melee"       );
}

// rogue_t::init_scaling ====================================================

void rogue_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = true;
  scales_with[STAT_STRENGTH] = false;
}

// rogue_t::init_resources =================================================

void rogue_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_COMBO_POINT ] = 0;
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
                              .duration( find_class_spell( "Adrenaline Rush" ) -> duration() + sets.set( SET_MELEE, T13, B4 ) -> effectN( 2 ).time_value() )
                              .default_value( find_class_spell( "Adrenaline Rush" ) -> effectN( 2 ).percent() )
                              .affects_regen( true )
                              .add_invalidate( CACHE_ATTACK_SPEED );
  buffs.blindside           = buff_creator_t( this, "blindside", spec.blindside -> effectN( 1 ).trigger() )
                              .chance( spec.blindside -> proc_chance() );
  buffs.master_of_subtlety_passive = buff_creator_t( this, "master_of_subtlety_passive", spec.master_of_subtlety )
                                     .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.master_of_subtlety  = buff_creator_t( this, "master_of_subtlety", find_spell( 31666 ) )
                              .default_value( spec.master_of_subtlety -> effectN( 1 ).percent() )
                              .chance( spec.master_of_subtlety -> ok() )
                              .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  // Killing spree buff has only 2 sec duration, main spell has 3, check.
  buffs.killing_spree       = buff_creator_t( this, "killing_spree", spec.killing_spree )
                              .duration( spec.killing_spree -> duration() + timespan_t::from_seconds( 0.001 ) );
  buffs.shadow_dance       = new buffs::shadow_dance_t( this );
  buffs.deadly_proc        = buff_creator_t( this, "deadly_proc" );
  buffs.shallow_insight    = new buffs::insight_buff_t( this, buff_creator_t( this, "shallow_insight", find_spell( 84745 ) )
                             .default_value( find_spell( 84745 ) -> effectN( 1 ).percent() )
                             .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ) );
  buffs.moderate_insight   = new buffs::insight_buff_t( this, buff_creator_t( this, "moderate_insight", find_spell( 84746 ) )
                             .default_value( find_spell( 84746 ) -> effectN( 1 ).percent() )
                             .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ) );
  buffs.deep_insight       = new buffs::insight_buff_t( this, buff_creator_t( this, "deep_insight", find_spell( 84747 ) )
                             .default_value( find_spell( 84747 ) -> effectN( 1 ).percent() )
                             .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ) );
  buffs.recuperate         = buff_creator_t( this, "recuperate" );
  buffs.shiv               = buff_creator_t( this, "shiv" );
  buffs.sleight_of_hand    = buff_creator_t( this, "sleight_of_hand", find_spell( 145211 ) )
                             .chance( sets.set( SET_MELEE, T16, B4 ) -> effectN( 3 ).percent() );
  //buffs.stealth            = buff_creator_t( this, "stealth" ).add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.stealth            = new buffs::stealth_t( this );
  buffs.vanish             = new buffs::vanish_t( this );
  buffs.subterfuge         = new buffs::subterfuge_t( this );
  buffs.t16_2pc_melee      = buff_creator_t( this, "silent_blades", find_spell( 145193 ) )
                             .chance( sets.has_set_bonus( SET_MELEE, T16, B2 ) );
  buffs.tier13_2pc         = buff_creator_t( this, "tier13_2pc", spell.tier13_2pc )
                             .chance( sets.has_set_bonus( SET_MELEE, T13, B2 ) ? 1.0 : 0 );
  buffs.toxicologist       = stat_buff_creator_t( this, "toxicologist", find_spell( 145249 ) )
                             .chance( sets.has_set_bonus( SET_MELEE, T16, B4 ) );
  //buffs.vanish             = buff_creator_t( this, "vanish", find_spell( 11327 ) )
  //                           .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
  //                           .cd( timespan_t::zero() )
  //                          .duration( find_spell( 11327 ) -> duration() + glyph.vanish -> effectN( 1 ).time_value() );

  // Envenom is controlled by the non-harmful dot applied to player when envenom is used
  buffs.envenom            = buff_creator_t( this, "envenom", find_specialization_spell( "Envenom" ) )
                             .duration( timespan_t::min() )
                             .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  buffs.slice_and_dice     = buff_creator_t( this, "slice_and_dice", find_class_spell( "Slice and Dice" ) )
                             .duration( perk.improved_slice_and_dice -> ok() ? timespan_t::zero() : timespan_t::min() )
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

  buffs.enhanced_vendetta = buff_creator_t( this, "enhanced_vendetta", perk.enhanced_vendetta );
  buffs.shadow_reflection = new buffs::shadow_reflection_t( this );
  buffs.death_from_above  = buff_creator_t( this, "death_from_above", spell.death_from_above )
                            .quiet( true );

  buffs.anticipation      = buff_creator_t( this, "anticipation", find_spell( 115189 ) )
                            .chance( talent.anticipation -> ok() );
  buffs.deceit            = buff_creator_t( this, "deceit", sets.set( ROGUE_COMBAT, T17, B4 ) -> effectN( 1 ).trigger() )
                            .chance( sets.has_set_bonus( ROGUE_COMBAT, T17, B4 ) );
  buffs.quick_blades      = buff_creator_t( this, "quick_blades", sets.set( ROGUE_SUBTLETY, T17, B2 ) -> effectN( 1 ).trigger() )
                            .chance( sets.has_set_bonus( ROGUE_SUBTLETY, T17, B2 ) );
  buffs.shadow_strikes    = buff_creator_t( this, "shadow_strikes", find_spell( 166881 ) )
                            .chance( sets.has_set_bonus( ROGUE_SUBTLETY, T17, B4 ) );

  buffs.burst_of_speed    = buff_creator_t( this, "burst_of_speed", find_spell( 137573 ) );
  buffs.sprint            = buff_creator_t( this, "sprint", spell.sprint )
    .cd( timespan_t::zero() );
  buffs.shadowstep        = buff_creator_t( this, "shadowstep", talent.shadowstep )
    .cd( timespan_t::zero() );
}

// trigger_honor_among_thieves ==============================================

struct honor_among_thieves_callback_t : public dbc_proc_callback_t
{
  rogue_t* rogue;

  honor_among_thieves_callback_t( player_t* player, rogue_t* r, const special_effect_t& effect ) :
    dbc_proc_callback_t( player, effect ), rogue( r )
  { }

  void trigger( action_t* a, void* call_data )
  {
    if ( ! rogue -> in_combat )
      return;

    // Only procs from specials; repeating is here for hunter autoshot, which
    // doesn't proc it either .. except in WoD, the rogue's own autoattacks
    // can also proc HaT
    if ( a -> player != rogue && ( ! a -> special || a -> repeating ) )
      return;

    // Doesn't proc from pets (only tested for hunter pets though)
    if ( a -> player -> is_pet() )
      return;

    if ( rogue -> cooldowns.honor_among_thieves -> down() )
      return;

    if ( ! rogue -> rng().roll( rogue -> spec.honor_among_thieves -> proc_chance() ) )
      return;

    listener -> procs.hat_donor -> occur();

    execute( a, static_cast<action_state_t*>( call_data ) );
  }

  void execute( action_t*, action_state_t* )
  {
    rogue -> trigger_combo_point_gain( 0, 1, rogue -> gains.honor_among_thieves );

    rogue -> procs.honor_among_thieves -> occur();

    rogue -> cooldowns.honor_among_thieves -> start( rogue -> spec.honor_among_thieves -> internal_cooldown() );

    if ( rogue -> buffs.t16_2pc_melee -> trigger() )
      rogue -> procs.t16_2pc_melee -> occur();
  }
};

// rogue_t::register_callbacks ==============================================

void rogue_t::register_callbacks()
{
  if ( spec.honor_among_thieves -> ok() )
  {
    for ( size_t i = 0; i < sim -> player_no_pet_list.size(); i++ )
    {
      player_t* p = sim -> player_no_pet_list[ i ];
      special_effect_t effect( p );
      effect.spell_id = spec.honor_among_thieves -> id();
      effect.proc_flags2_ = PF2_CRIT;
      effect.cooldown_ = timespan_t::zero();

      p -> special_effects.push_back( effect );

      new honor_among_thieves_callback_t( p, this, p -> special_effects.back() );
    }
  }

  player_t::register_callbacks();
}

// rogue_t::reset ===========================================================

void rogue_t::reset()
{
  player_t::reset();

  poisoned_enemies = 0;

  core_event_t::cancel( event_premeditation );
}

// rogue_t::arise ===========================================================

void rogue_t::arise()
{
  player_t::arise();

  resources.current[ RESOURCE_COMBO_POINT ] = 0;

  if ( perk.improved_slice_and_dice -> ok() )
    buffs.slice_and_dice -> trigger( 1, buffs.slice_and_dice -> data().effectN( 1 ).percent(), -1.0, timespan_t::zero() );


  if ( ! sim -> overrides.haste && dbc.spell( 113742 ) -> is_level( level ) ) sim -> auras.haste -> trigger();
  if ( ! sim -> overrides.multistrike && dbc.spell( 113742 ) -> is_level( level ) ) sim -> auras.multistrike -> trigger();
}

// rogue_t::energy_regen_per_second =========================================

double rogue_t::energy_regen_per_second() const
{
  double r = player_t::energy_regen_per_second();

  if ( buffs.blade_flurry -> check() )
    r *= 1.0 + spec.blade_flurry -> effectN( 1 ).percent();

  if ( talent.venom_rush -> ok() )
    r *= 1.0 + std::min( poisoned_enemies, 3U ) * spell.venom_rush -> effectN( 1 ).percent();

  return r;
}

// rogue_t::temporary_movement_modifier ==================================

double rogue_t::temporary_movement_modifier() const
{
  double temporary = player_t::temporary_movement_modifier();

  if ( buffs.sprint -> up() )
    temporary = std::max( buffs.sprint -> data().effectN( 1 ).percent() + glyph.sprint -> effectN( 1 ).percent(), temporary );

  if ( buffs.burst_of_speed -> up() )
    temporary = std::max( buffs.burst_of_speed -> data().effectN( 1 ).percent(), temporary );

  if ( buffs.shadowstep -> up() )
    temporary = std::max( buffs.shadowstep -> data().effectN( 2 ).percent(), temporary );

  return temporary;
}

// rogue_t::passive_movement_modifier===================================

double rogue_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  ms += spell.fleet_footed -> effectN( 1 ).percent();

  if ( buffs.stealth -> up() ) // Check if nightstalker is temporary or passive.
    ms += talent.nightstalker -> effectN( 1 ).percent();

  return ms;
}


// rogue_t::regen ===========================================================

void rogue_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( buffs.adrenaline_rush -> up() )
  {
    if ( ! resources.is_infinite( RESOURCE_ENERGY ) )
    {
      double energy_regen = periodicity.total_seconds() * energy_regen_per_second() * buffs.adrenaline_rush -> data().effectN( 1 ).percent();

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

// rogue_t::convert_hybrid_stat ==============================================

stat_e rogue_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  case STAT_STR_AGI_INT:
  case STAT_AGI_INT: 
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

// rogue_t::create_pets ======================================================

void rogue_t::create_pets()
{
  shadow_reflection = new shadow_reflection_pet_t( this );
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

  virtual void init( sim_t* ) const
  { }

  virtual void combat_begin( sim_t* ) const {}

  virtual void combat_end( sim_t* ) const {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::rogue()
{
  static rogue_module_t m;
  return &m;
}
