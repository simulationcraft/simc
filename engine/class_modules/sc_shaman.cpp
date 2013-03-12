// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// ==========================================================================
// MoP TODO
// ==========================================================================
//
// Code:
// - Talents Totemic Restoration, Conductivity, Healing Tide Totem
//
// Restoration:
// - Greating Healing Wave, Healing Wave, Chain Heal, Healing Rain
// - Mana Tide Totem
// - Mastery
// - ROLE_HEAL action list
//
// General:
// - Class base stats for 87..90
// - Searing totem base damage scaling
// - Healing Stream Totem
// - Empower to Primal Elementals
// - Echo of the Elements functionality with heals
//
// ==========================================================================
// BUGS
// ==========================================================================
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Shaman
// ==========================================================================

namespace { // UNNAMED NAMESPACE

struct shaman_t;

enum totem_e { TOTEM_NONE=0, TOTEM_AIR, TOTEM_EARTH, TOTEM_FIRE, TOTEM_WATER, TOTEM_MAX };
enum imbue_e { IMBUE_NONE=0, FLAMETONGUE_IMBUE, WINDFURY_IMBUE, EARTHLIVING_IMBUE };

struct shaman_melee_attack_t;
struct shaman_spell_t;
struct shaman_heal_t;
struct shaman_totem_pet_t;
struct totem_pulse_event_t;
struct totem_pulse_action_t;

struct shaman_td_t : public actor_pair_t
{
  struct dots
  {
    dot_t* flame_shock;
  } dot;

  struct debuffs
  {
    buff_t* stormstrike;
    buff_t* unleashed_fury;
  } debuff;

  struct heals
  {
    dot_t* riptide;
    dot_t* earthliving;
  } heal;

  shaman_td_t( player_t* target, shaman_t* p );
};

struct shaman_t : public player_t
{
public:
  // Misc
  timespan_t ls_reset;
  int        active_flame_shocks;

  // Options
  timespan_t wf_delay;
  timespan_t wf_delay_stddev;
  timespan_t uf_expiration_delay;
  timespan_t uf_expiration_delay_stddev;
  double     eoe_proc_chance;
  int        aggregate_stormlash;

  // Active
  action_t* active_lightning_charge;

  // Cached actions
  action_t* action_ancestral_awakening;
  action_t* action_flame_shock;
  action_t* action_improved_lava_lash;
  action_t* action_lightning_strike;

  // Pets
  pet_t* pet_feral_spirit[2];
  pet_t* pet_fire_elemental;
  pet_t* guardian_fire_elemental;
  pet_t* pet_earth_elemental;
  pet_t* guardian_earth_elemental;

  // Totems
  shaman_totem_pet_t* totems[ TOTEM_MAX ];

  // Constants
  struct
  {
    double matching_gear_multiplier;
    double flurry_rating_multiplier;
    double haste_attack_ancestral_swiftness;
    double haste_spell_ancestral_swiftness;
    double haste_elemental_mastery;
    double attack_speed_flurry;
    double attack_speed_unleash_wind;
  } constant;

  // Buffs
  struct
  {
    buff_t* ancestral_swiftness;
    buff_t* ascendance;
    buff_t* elemental_focus;
    buff_t* lava_surge;
    buff_t* lightning_shield;
    buff_t* maelstrom_weapon;
    buff_t* searing_flames;
    buff_t* shamanistic_rage;
    buff_t* spiritwalkers_grace;
    buff_t* unleash_flame;
    buff_t* unleashed_fury_wf;
    buff_t* tidal_waves;
    buff_t* water_shield;

    haste_buff_t* elemental_mastery;
    haste_buff_t* flurry;
    haste_buff_t* tier13_4pc_healer;
    haste_buff_t* unleash_wind;

    stat_buff_t* elemental_blast_agility;
    stat_buff_t* elemental_blast_crit;
    stat_buff_t* elemental_blast_haste;
    stat_buff_t* elemental_blast_mastery;
    stat_buff_t* tier13_2pc_caster;
    stat_buff_t* tier13_4pc_caster;
  } buff;

  // Cooldowns
  struct
  {
    cooldown_t* ancestral_swiftness;
    cooldown_t* ascendance;
    cooldown_t* elemental_totem;
    cooldown_t* echo_of_the_elements;
    cooldown_t* feral_spirits;
    cooldown_t* lava_burst;
    cooldown_t* shock;
    cooldown_t* stormlash;
    cooldown_t* strike;
    cooldown_t* windfury_weapon;
  } cooldown;

  // Gains
  struct
  {
    gain_t* primal_wisdom;
    gain_t* resurgence;
    gain_t* rolling_thunder;
    gain_t* telluric_currents;
    gain_t* thunderstorm;
    gain_t* water_shield;
  } gain;

  // Tracked Procs
  struct
  {
    proc_t* elemental_overload;
    proc_t* lava_surge;
    proc_t* ls_fast;
    proc_t* maelstrom_weapon;
    proc_t* rolling_thunder;
    proc_t* static_shock;
    proc_t* swings_clipped_mh;
    proc_t* swings_clipped_oh;
    proc_t* swings_reset_mh;
    proc_t* swings_reset_oh;
    proc_t* t15_2pc_melee;
    proc_t* wasted_t15_2pc_melee;
    proc_t* wasted_ls;
    proc_t* wasted_ls_shock_cd;
    proc_t* wasted_mw;
    proc_t* windfury;

    proc_t* fulmination[7];
    proc_t* maelstrom_weapon_used[6];

    proc_t* uf_flame_shock;
    proc_t* uf_fire_nova;
    proc_t* uf_lava_burst;
    proc_t* uf_elemental_blast;
    proc_t* uf_wasted;
  } proc;

  // Random Number Generators
  struct
  {
    rng_t* earthliving;
    rng_t* echo_of_the_elements;
    rng_t* elemental_overload;
    rng_t* lava_surge;
    rng_t* primal_wisdom;
    rng_t* rolling_thunder;
    rng_t* searing_flames;
    rng_t* static_shock;
    rng_t* t15_2pc_caster;
    rng_t* windfury_delay;
    rng_t* windfury_weapon;
  } rng;

  // Class Specializations
  struct
  {
    // Generic
    const spell_data_t* mail_specialization;

    // Elemental / Restoration
    const spell_data_t* spiritual_insight;

    // Elemental
    const spell_data_t* elemental_focus;
    const spell_data_t* elemental_precision;
    const spell_data_t* elemental_fury;
    const spell_data_t* fulmination;
    const spell_data_t* lava_surge;
    const spell_data_t* rolling_thunder;
    const spell_data_t* shamanism;

    // Enhancement
    const spell_data_t* dual_wield;
    const spell_data_t* flurry;
    const spell_data_t* mental_quickness;
    const spell_data_t* primal_wisdom;
    const spell_data_t* searing_flames;
    const spell_data_t* shamanistic_rage;
    const spell_data_t* static_shock;
    const spell_data_t* maelstrom_weapon;

    // Restoration
    const spell_data_t* ancestral_awakening;
    const spell_data_t* ancestral_focus;
    const spell_data_t* earth_shield;
    const spell_data_t* meditation;
    const spell_data_t* purification;
    const spell_data_t* resurgence;
    const spell_data_t* riptide;
    const spell_data_t* tidal_waves;
  } spec;

  // Masteries
  struct
  {
    const spell_data_t* elemental_overload;
    const spell_data_t* enhanced_elements;
    const spell_data_t* deep_healing;
  } mastery;

  // Talents
  struct
  {
    const spell_data_t* call_of_the_elements;
    const spell_data_t* totemic_restoration;

    const spell_data_t* elemental_mastery;
    const spell_data_t* ancestral_swiftness;
    const spell_data_t* echo_of_the_elements;

    const spell_data_t* unleashed_fury;
    const spell_data_t* primal_elementalist;
    const spell_data_t* elemental_blast;
  } talent;

  // Glyphs
  struct
  {
    const spell_data_t* chain_lightning;
    const spell_data_t* fire_elemental_totem;
    const spell_data_t* flame_shock;
    const spell_data_t* healing_storm;
    const spell_data_t* lava_lash;
    const spell_data_t* spiritwalkers_grace;
    const spell_data_t* telluric_currents;
    const spell_data_t* thunder;
    const spell_data_t* thunderstorm;
    const spell_data_t* unleashed_lightning;
    const spell_data_t* water_shield;
  } glyph;

  // Misc Spells
  struct
  {
    const spell_data_t* primal_wisdom;
    const spell_data_t* resurgence;
    const spell_data_t* searing_flames;
    const spell_data_t* ancestral_swiftness;
    const spell_data_t* flame_shock;
  } spell;

  // Cached pointer for ascendance / normal white melee
  shaman_melee_attack_t* melee_mh;
  shaman_melee_attack_t* melee_oh;
  shaman_melee_attack_t* ascendance_mh;
  shaman_melee_attack_t* ascendance_oh;

  // Weapon Enchants
  shaman_melee_attack_t* windfury_mh;
  shaman_melee_attack_t* windfury_oh;
  shaman_spell_t*  flametongue_mh;
  shaman_spell_t*  flametongue_oh;
private:
  target_specific_t<shaman_td_t> target_data;
public:
  shaman_t( sim_t* sim, const std::string& name, race_e r = RACE_TAUREN ) :
    player_t( sim, SHAMAN, name, r ),
    ls_reset( timespan_t::zero() ), active_flame_shocks( 0 ),
    wf_delay( timespan_t::from_seconds( 0.95 ) ), wf_delay_stddev( timespan_t::from_seconds( 0.25 ) ),
    uf_expiration_delay( timespan_t::from_seconds( 0.3 ) ), uf_expiration_delay_stddev( timespan_t::from_seconds( 0.05 ) ),
    eoe_proc_chance( 0 ), aggregate_stormlash( 0 ),
    active_lightning_charge( nullptr ),
    action_ancestral_awakening( nullptr ),
    action_flame_shock( nullptr ),
    action_improved_lava_lash( nullptr ),
    action_lightning_strike( nullptr ),
    pet_feral_spirit(),
    pet_fire_elemental( nullptr ),
    guardian_fire_elemental( nullptr ),
    pet_earth_elemental( nullptr ),
    guardian_earth_elemental( nullptr ),
    totems(),
    buff(),
    cooldown(),
    gain(),
    proc(),
    rng(),
    spec(),
    mastery(),
    talent(),
    glyph(),
    spell()
  {
    target_data.init( "target_data", this );

    // Active
    active_lightning_charge   = 0;

    action_flame_shock = 0;
    action_improved_lava_lash = 0;
    action_lightning_strike = 0;

    // Totem tracking
    for ( int i = 0; i < TOTEM_MAX; i++ ) totems[ i ] = 0;

    // Cooldowns
    cooldown.ancestral_swiftness  = get_cooldown( "ancestral_swiftness"   );
    cooldown.ascendance           = get_cooldown( "ascendance"            );
    cooldown.elemental_totem      = get_cooldown( "elemental_totem"       );
    cooldown.echo_of_the_elements = get_cooldown( "echo_of_the_elements"  );
    cooldown.feral_spirits        = get_cooldown( "feral_spirit"          );
    cooldown.lava_burst           = get_cooldown( "lava_burst"            );
    cooldown.shock                = get_cooldown( "shock"                 );
    cooldown.stormlash            = get_cooldown( "stormlash_totem"       );
    cooldown.strike               = get_cooldown( "strike"                );
    cooldown.windfury_weapon      = get_cooldown( "windfury_weapon"       );

    melee_mh = 0;
    melee_oh = 0;
    ascendance_mh = 0;
    ascendance_oh = 0;

    // Weapon Enchants
    windfury_mh    = 0;
    windfury_oh    = 0;
    flametongue_mh = 0;
    flametongue_oh = 0;
  }

  // Character Definition
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      create_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      moving();
  virtual double    composite_attack_hit();
  virtual double    composite_attack_haste();
  virtual double    composite_attack_speed();
  virtual double    composite_spell_haste();
  virtual double    composite_spell_hit();
  virtual double    composite_spell_power( school_e school );
  virtual double    composite_spell_power_multiplier();
  virtual double    composite_player_multiplier( school_e school, action_t* a = NULL );
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual expr_t* create_expression( action_t*, const std::string& name );
  virtual int       decode_set( item_t& );
  virtual resource_e primary_resource() { return RESOURCE_MANA; }
  virtual role_e primary_role();
  virtual void      arise();
  virtual void      reset();

  virtual shaman_td_t* get_target_data( player_t* target )
  {
    shaman_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new shaman_td_t( target, this );
    }
    return td;
  }

  // Event Tracking
  virtual void regen( timespan_t periodicity );
};

shaman_td_t::shaman_td_t( player_t* target, shaman_t* p ) :
  actor_pair_t( target, p )
{
  dot.flame_shock       = target -> get_dot( "flame_shock", p );

  debuff.stormstrike    = buff_creator_t( *this, "stormstrike", p -> find_specialization_spell( "Stormstrike" ) );
  debuff.unleashed_fury = buff_creator_t( *this, "unleashed_fury_ft", p -> find_spell( 118470 ) );
}

struct shaman_action_state_t : public heal_state_t
{
  bool eoe_proc;

  shaman_action_state_t( action_t* spell, player_t* target ) :
    heal_state_t( spell, target ),
    eoe_proc( false )
  { }

  void debug()
  {
    heal_state_t::debug();
    action -> sim -> output( "[NEW] %s %s %s: eoe_proc=%d",
                             action -> player -> name(),
                             action -> name(),
                             target -> name(),
                             eoe_proc );
  }

  void copy_state( const action_state_t* o )
  {
    if ( o == 0 || this == o )
    {
      eoe_proc = false;
      return;
    }

    heal_state_t::copy_state( o );
    const shaman_action_state_t* ss = static_cast< const shaman_action_state_t* >( o );
    eoe_proc = ss -> eoe_proc;
  }
};

// Template for common shaman action code. See priest_action_t.
template <class Base>
struct shaman_action_t : public Base
{
private:
  typedef Base ab; // action base, eg. spell_t
public:
  typedef shaman_action_t base_t;

  // Misc stuff
  bool        totem;
  bool        shock;

  shaman_action_t( const std::string& n, shaman_t* player,
                   const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    totem( false ), shock( false )
  {
    ab::may_crit = true;
  }

  shaman_t* p()
  { return debug_cast< shaman_t* >( ab::player ); }

  shaman_td_t* td( player_t* t = 0 )
  { return p() -> get_target_data( t ? t : ab::target ); }

  action_state_t* new_state()
  { return new shaman_action_state_t( this, ab::target ); }

  double cost()
  {
    double c = ab::cost();
    if ( c == 0 )
      return c;
    c *= 1.0 + cost_reduction();
    if ( c < 0 ) c = 0;
    return c;
  }

  virtual double cost_reduction()
  {
    double c = 0.0;

    if ( ab::base_execute_time == timespan_t::zero() ||
         ( ( ab::data().school_mask() & SCHOOL_MASK_NATURE ) != 0 && 
           p() -> buff.maelstrom_weapon -> check() == p() -> buff.maelstrom_weapon -> max_stack() ) )
      c += p() -> spec.mental_quickness -> effectN( ! shock ? 2 : 3 ).percent();

    if ( p() -> buff.shamanistic_rage -> up() && ( ab::harmful || totem ) )
      c += p() -> buff.shamanistic_rage -> data().effectN( 1 ).percent();

    return c;
  }

  // Both Ancestral Swiftness and Maelstrom Weapon share this conditional for use
  bool instant_eligibility()
  { return ab::data().school_mask() & SCHOOL_MASK_NATURE && ab::base_execute_time != timespan_t::zero(); }
};

template <class Base>
struct shaman_spell_base_t : public shaman_action_t<Base>
{
private:
  typedef shaman_action_t<Base> ab;
public:
  typedef shaman_spell_base_t<Base> base_t;

  // Echo of Elements functionality
  bool        may_proc_eoe;
  bool        eoe_proc;
  double      eoe_proc_chance;
  stats_t*    eoe_stats;
  cooldown_t* eoe_cooldown;

  shaman_spell_base_t( const std::string& n, shaman_t* player,
                       const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    may_proc_eoe( true ), eoe_proc( false ),
    eoe_proc_chance( player -> eoe_proc_chance ),
    eoe_stats( 0 ), eoe_cooldown( 0 )
  { }

  void execute();
  void schedule_execute( action_state_t* state = 0 );
  void impact( action_state_t* state );

  void init()
  {
    ab::init();

    if ( may_proc_eoe )
    {
      shaman_t* p = ab::p();

      std::string eoe_stat_name = ab::name_str;
      eoe_stat_name += "_eoe";

      eoe_stats = p -> get_stats( eoe_stat_name, this );
      eoe_stats -> school = ab::school;

      if ( ab::stats -> parent )
        ab::stats -> parent -> add_child( eoe_stats );
      else
        ab::stats -> add_child( eoe_stats );

      eoe_cooldown = p -> get_cooldown( ab::name_str + "_eoe" );
    }
  }

  void snapshot_state( action_state_t* s, uint32_t flags, dmg_e type )
  {
    ab::snapshot_state( s, flags, type );

    if ( may_proc_eoe )
    {
      shaman_action_state_t* ss = debug_cast< shaman_action_state_t* >( s );
      ss -> eoe_proc = eoe_proc;
    }
  }

  double cost_reduction()
  {
    double c = ab::cost_reduction();
    shaman_t* p = ab::p();

    if ( c > -1.0 && ab::instant_eligibility() )
      c += p -> buff.maelstrom_weapon -> check() * p -> buff.maelstrom_weapon -> data().effectN( 2 ).percent();

    return c;
  }

  timespan_t execute_time()
  {
    timespan_t t = ab::execute_time();

    if ( ab::instant_eligibility() )
    {
      shaman_t* p = ab::p();

      if ( p -> buff.ancestral_swiftness -> up() )
        t *= 1.0 + p -> buff.ancestral_swiftness -> data().effectN( 1 ).percent();
      else
        t *= 1.0 + p -> buff.maelstrom_weapon -> stack() * p -> buff.maelstrom_weapon -> data().effectN( 1 ).percent();
    }

    return t;
  }

  bool use_ancestral_swiftness()
  {
    shaman_t* p = ab::p();
    return ab::instant_eligibility() && p -> buff.ancestral_swiftness -> check() &&
           p -> buff.maelstrom_weapon -> check() * p -> buff.maelstrom_weapon -> data().effectN( 1 ).percent() < 1.0;
  }
};

template <class Base>
struct eoe_execute_event_t : public event_t
{
  shaman_spell_base_t<Base>* spell;

  eoe_execute_event_t( shaman_spell_base_t<Base>* s ) :
    event_t( s -> player, "eoe_execute" ),
    spell( s )
  {
    timespan_t delay_duration = sim.gauss( sim.default_aura_delay / 2, sim.default_aura_delay_stddev / 2 );
    sim.add_event( this, delay_duration );
  }

  void execute()
  {
    assert( spell );

    double base_cost = spell -> base_costs[ RESOURCE_MANA ];
    cooldown_t* tmp_cooldown = spell -> cooldown;
    assert( spell -> stats != spell -> eoe_stats );
    stats_t* tmp_stats = spell -> stats;
    timespan_t tmp_cast_time = spell -> base_execute_time;
    timespan_t tmp_gcd = spell -> trigger_gcd;
    bool tmp_callbacks = spell -> callbacks;
    double tmp_sl_da_mul = spell -> stormlash_da_multiplier;
    double tmp_sl_ta_mul = spell -> stormlash_ta_multiplier;

    spell -> time_to_execute = timespan_t::zero();

    spell -> stormlash_da_multiplier = 0;
    spell -> stormlash_ta_multiplier = 0;
    spell -> callbacks = false;
    spell -> stats = spell -> eoe_stats;
    spell -> base_costs[ RESOURCE_MANA ] = 0;
    spell -> cooldown = spell -> eoe_cooldown;
    spell -> base_execute_time = timespan_t::zero();
    spell -> trigger_gcd = timespan_t::zero();
    spell -> eoe_proc = true;

    spell -> execute();

    spell -> eoe_proc = false;
    spell -> trigger_gcd = tmp_gcd;
    spell -> base_execute_time = tmp_cast_time;
    spell -> cooldown = tmp_cooldown;
    spell -> base_costs[ RESOURCE_MANA ] = base_cost;
    spell -> stats = tmp_stats;
    spell -> callbacks = tmp_callbacks;
    spell -> stormlash_ta_multiplier = tmp_sl_ta_mul;
    spell -> stormlash_da_multiplier = tmp_sl_da_mul;
  }
};

// ==========================================================================
// Shaman Attack
// ==========================================================================

struct shaman_melee_attack_t : public shaman_action_t<melee_attack_t>
{
private:
  typedef shaman_action_t<melee_attack_t> ab;
public:
  bool may_proc_windfury;
  bool may_proc_flametongue;
  bool may_proc_primal_wisdom;

  // Maelstrom Weapon functionality
  bool        may_proc_maelstrom;
  proc_t*     maelstrom_procs;
  proc_t*     maelstrom_procs_wasted;

  shaman_melee_attack_t( const std::string& token, shaman_t* p, const spell_data_t* s ) :
    base_t( token, p, s ),
    may_proc_windfury( false ), may_proc_flametongue( true ), may_proc_primal_wisdom( true ),
    may_proc_maelstrom( p -> spec.maelstrom_weapon -> ok() ),
    maelstrom_procs( 0 ), maelstrom_procs_wasted( 0 )
  {
    special = may_crit = true;
    may_glance = false;
  }

  shaman_melee_attack_t( shaman_t* p, const spell_data_t* s ) :
    base_t( "", p, s ),
    may_proc_windfury( false ), may_proc_flametongue( true ), may_proc_primal_wisdom( true ),
    may_proc_maelstrom( p -> spec.maelstrom_weapon -> ok() ),
    maelstrom_procs( 0 ), maelstrom_procs_wasted( 0 )
  {
    special = may_crit = true;
    may_glance = false;
  }

  void init()
  {
    ab::init();

    if ( may_proc_maelstrom )
    {
      maelstrom_procs = ab::player -> get_proc( ab::name_str + "_maelstrom" );
      maelstrom_procs_wasted = ab::player -> get_proc( ab::name_str + "_maelstrom_wasted" );
    }
  }

  void impact( action_state_t* );
};

// ==========================================================================
// Shaman Offensive Spell
// ==========================================================================

struct shaman_spell_t : public shaman_spell_base_t<spell_t>
{
  // Elemental overload stuff
  bool     overload;
  shaman_spell_t* overload_spell;
  double   overload_chance_multiplier;

  shaman_spell_t( const std::string& token, shaman_t* p,
                  const spell_data_t* s = spell_data_t::nil(), const std::string& options = std::string() ) :
    base_t( token, p, s ),
    overload( false ), overload_spell( 0 ), overload_chance_multiplier( 1.0 )
  {
    parse_options( 0, options );

    crit_bonus_multiplier *= 1.0 + p -> spec.elemental_fury -> effectN( 1 ).percent();
  }

  shaman_spell_t( shaman_t* p, const spell_data_t* s = spell_data_t::nil(), const std::string& options = std::string() ) :
    base_t( "", p, s ),
    overload( false ), overload_spell( 0 ), overload_chance_multiplier( 1.0 )
  {
    parse_options( 0, options );

    crit_bonus_multiplier *= 1.0 + p -> spec.elemental_fury -> effectN( 1 ).percent();
  }

  virtual void   execute();

  virtual void consume_resource()
  {
    base_t::consume_resource();

    if ( harmful && callbacks && ! proc && resource_consumed > 0 && p() -> buff.elemental_focus -> up() )
      p() -> buff.elemental_focus -> decrement();
  }

  virtual void impact( action_state_t* state )
  {
    base_t::impact( state );

    // Overloads dont trigger elemental focus
    if ( ! overload && ! proc && p() -> specialization() == SHAMAN_ELEMENTAL &&
         base_dd_min > 0 && state -> result == RESULT_CRIT )
      p() -> buff.elemental_focus -> trigger( p() -> buff.elemental_focus -> data().initial_stacks() );
  }

  virtual double cost_reduction()
  {
    double c = base_t::cost_reduction();

    if ( harmful && callbacks && ! proc && p() -> buff.elemental_focus -> up() )
      c += p() -> buff.elemental_focus -> data().effectN( 1 ).percent();

    return c;
  }

  virtual bool usable_moving()
  {
    if ( p() -> buff.spiritwalkers_grace -> check() || execute_time() == timespan_t::zero() )
      return true;

    return base_t::usable_moving();
  }

  virtual double composite_da_multiplier()
  {
    double m = base_t::composite_da_multiplier();

    if ( instant_eligibility() && p() -> buff.maelstrom_weapon -> stack() > 0 )
      m *= 1.0 + p() -> sets -> set( SET_T13_2PC_MELEE ) -> effectN( 1 ).percent();

    if ( p() -> buff.elemental_focus -> up() )
      m *= 1.0 + p() -> buff.elemental_focus -> data().effectN( 2 ).percent();

    return m;
  }
};

// ==========================================================================
// Shaman Heal
// ==========================================================================

struct shaman_heal_t : public shaman_spell_base_t<heal_t>
{
  double elw_proc_high,
         elw_proc_low,
         resurgence_gain;

  bool proc_tidal_waves,
       consume_tidal_waves;

  shaman_heal_t( const std::string& token, shaman_t* p,
                 const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() ) :
    base_t( token, p, s ),
    elw_proc_high( .2 ), elw_proc_low( 1.0 ),
    resurgence_gain( 0 ),
    proc_tidal_waves( false ), consume_tidal_waves( false )
  {
    parse_options( 0, options );
  }

  shaman_heal_t( shaman_t* p, const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() ) :
    base_t( "", p, s ),
    elw_proc_high( .2 ), elw_proc_low( 1.0 ),
    resurgence_gain( 0 ),
    proc_tidal_waves( false ), consume_tidal_waves( false )
  {
    parse_options( 0, options );
  }

  double composite_spell_power()
  {
    double sp = base_t::composite_spell_power();

    if ( p() -> main_hand_weapon.buff_type == EARTHLIVING_IMBUE )
      sp += p() -> main_hand_weapon.buff_value;

    return sp;
  }

  double composite_da_multiplier()
  {
    double m = base_t::composite_da_multiplier();
    m *= 1.0 + p() -> spec.purification -> effectN( 1 ).percent();
    m *= 1.0 + p() -> buff.maelstrom_weapon -> stack() * p() -> glyph.healing_storm -> effectN( 1 ).percent();
    return m;
  }

  double composite_ta_multiplier()
  {
    double m = base_t::composite_da_multiplier();
    m *= 1.0 + p() -> spec.purification -> effectN( 1 ).percent();
    m *= 1.0 + p() -> buff.maelstrom_weapon -> stack() * p() -> glyph.healing_storm -> effectN( 1 ).percent();
    return m;
  }

  double composite_target_multiplier( player_t* target )
  {
    double m = base_t::composite_target_multiplier( target );
    if ( target -> buffs.earth_shield -> up() )
      m *= 1.0 + p() -> spec.earth_shield -> effectN( 2 ).percent();
    return m;
  }

  void impact( action_state_t* s );

  void execute()
  {
    base_t::execute();

    if ( consume_tidal_waves )
      p() -> buff.tidal_waves -> decrement();
  }

  virtual double deep_healing( const action_state_t* s )
  {
    double hpp = ( 1.0 - s -> target -> health_percentage() / 100.0 );

    return 1.0 + hpp * p() -> composite_mastery() * p() -> mastery.deep_healing -> effectN( 1 ).mastery_value();
  }
};

// ==========================================================================
// Pet Spirit Wolf
// ==========================================================================

struct feral_spirit_pet_t : public pet_t
{
  struct melee_t : public melee_attack_t
  {
    melee_t( feral_spirit_pet_t* player ) :
      melee_attack_t( "melee", player, spell_data_t::nil() )
    {
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      background = true;
      repeating = true;
      may_crit = true;
      school      = SCHOOL_PHYSICAL;
    }

    feral_spirit_pet_t* p() { return static_cast<feral_spirit_pet_t*>( player ); }

    void init()
    {
      melee_attack_t::init();

      pet_t* first_pet = p() -> o() -> find_pet( "spirit_wolf" );
      if ( first_pet != player )
        stats = first_pet -> find_stats( name() );
    }

    virtual void impact( action_state_t* state )
    {
      melee_attack_t::impact( state );

      if ( result_is_hit( state -> result ) )
      {
        shaman_t* o = p() -> o();

        if ( sim -> roll( o -> sets -> set( SET_T13_4PC_MELEE ) -> effectN( 1 ).percent() ) )
        {
          int mwstack = o -> buff.maelstrom_weapon -> total_stack();
          o -> buff.maelstrom_weapon -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );
          o -> proc.maelstrom_weapon -> occur();

          if ( mwstack == o -> buff.maelstrom_weapon -> max_stack() )
            o -> proc.wasted_mw -> occur();
        }
      }
    }
  };

  struct spirit_bite_t : public melee_attack_t
  {
    spirit_bite_t( feral_spirit_pet_t* player ) :
      melee_attack_t( "spirit_bite", player, player -> find_spell( 58859 ) )
    {
      may_crit  = true;
      special   = true;
      direct_power_mod = data().extra_coeff();
      cooldown -> duration = timespan_t::from_seconds( 7.0 );

    }

    feral_spirit_pet_t* p()
    { return static_cast<feral_spirit_pet_t*>( player ); }

    void init()
    {
      melee_attack_t::init();
      pet_t* first_pet = p() -> o() -> find_pet( "spirit_wolf" );
      if ( first_pet != player )
        stats = first_pet -> find_stats( name() );
    }

    virtual double composite_da_multiplier()
    {
      double m = melee_attack_t::composite_da_multiplier();

      if ( p() -> o() -> specialization() == SHAMAN_ENHANCEMENT )
        m *= 1.0 + p() -> o() -> composite_mastery() * p() -> o() -> mastery.enhanced_elements -> effectN( 1 ).mastery_value();

      return m;
    }
  };

  melee_t* melee;
  const spell_data_t* command;

  feral_spirit_pet_t( sim_t* sim, shaman_t* owner ) :
    pet_t( sim, owner, "spirit_wolf", true ), melee( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level ) * 0.5;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level ) * 0.5;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );

    owner_coeff.ap_from_ap = 0.50;

    command = owner -> find_spell( 65222 );
  }

  shaman_t* o() { return static_cast<shaman_t*>( owner ); }

  virtual void init_base()
  {
    pet_t::init_base();

    melee = new melee_t( this );
  }

  virtual void init_actions()
  {
    action_list_str = "spirit_bite";

    pet_t::init_actions();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str )
  {
    if ( name == "spirit_bite" ) return new spirit_bite_t( this );

    return pet_t::create_action( name, options_str );
  }

  void schedule_ready( timespan_t delta_time = timespan_t::zero(), bool waiting = false )
  {
    if ( melee && ! melee -> execute_event )
      melee -> schedule_execute();

    pet_t::schedule_ready( delta_time, waiting );
  }

  double composite_player_multiplier( school_e school, action_t* a = 0 )
  {
    double m = pet_t::composite_player_multiplier( school, a );

    if ( owner -> race == RACE_ORC )
      m *= 1.0 + command -> effectN( 1 ).percent();

    return m;
  }
};

struct earth_elemental_pet_t : public pet_t
{
  struct travel_t : public action_t
  {
    travel_t( player_t* player ) : action_t( ACTION_OTHER, "travel", player ) {}
    virtual void execute() { player -> current.distance = 1; }
    virtual timespan_t execute_time() { return timespan_t::from_seconds( player -> current.distance / 10.0 ); }
    virtual bool ready() { return ( player -> current.distance > 1 ); }
    virtual bool usable_moving() { return true; }
  };

  struct auto_melee_attack_t : public melee_attack_t
  {
    auto_melee_attack_t( earth_elemental_pet_t* player ) :
      melee_attack_t( "auto_attack", player )
    {
      assert( player -> main_hand_weapon.type != WEAPON_NONE );
      player -> main_hand_attack = new melee_t( player );

      trigger_gcd = timespan_t::zero();
    }

    virtual void execute()
    {
      player -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready()
    {
      if ( player -> is_moving() ) return false;
      return( player -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

  struct melee_t : public melee_attack_t
  {
    melee_t( earth_elemental_pet_t* player ) :
      melee_attack_t( "earth_melee", player, spell_data_t::nil() )
    {
      school            = SCHOOL_PHYSICAL;
      may_crit          = true;
      background        = true;
      repeating         = true;
      weapon            = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
    }
  };

  struct pulverize_t : public melee_attack_t
  {
    pulverize_t( earth_elemental_pet_t* player ) :
      melee_attack_t( "pulverize", player, player -> find_spell( 118345 ) )
    {
      school            = SCHOOL_PHYSICAL;
      may_crit          = true;
      special           = true;
      weapon            = &( player -> main_hand_weapon );
    }
  };

  const spell_data_t* command;

  earth_elemental_pet_t( sim_t* sim, shaman_t* owner, bool guardian ) :
    pet_t( sim, owner, ( ! guardian ) ? "primal_earth_elemental" : "greater_earth_elemental", guardian /*GUARDIAN*/ )
  {
    stamina_per_owner   = 1.0;

    command = owner -> find_spell( 65222 );
  }

  double composite_player_multiplier( school_e school, action_t* a = 0 )
  {
    double m = pet_t::composite_player_multiplier( school, a );

    if ( owner -> race == RACE_ORC )
      m *= 1.0 + command -> effectN( 1 ).percent();

    return m;
  }

  shaman_t* o() { return static_cast< shaman_t* >( owner ); }
  //timespan_t available() { return sim -> max_time; }

  virtual void init_base()
  {
    pet_t::init_base();

    // Approximated from lvl 85, 86 Earth Elementals
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level ) * 1.3;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level ) * 1.3;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );

    if ( o() -> talent.primal_elementalist -> ok() )
    {
      main_hand_weapon.min_dmg *= 1.5;
      main_hand_weapon.max_dmg *= 1.5;
    }

    resources.base[ RESOURCE_HEALTH ] = 8000; // Approximated from lvl85 earth elemental in game
    resources.base[ RESOURCE_MANA   ] = 0; //

    // Simple as it gets, travel to target, kick off melee
    action_list_str = "travel/auto_attack,moving=0";

    if ( o() -> talent.primal_elementalist -> ok() )
      action_list_str += "/pulverize";

    owner_coeff.ap_from_sp = 1.3;
    if ( o() -> talent.primal_elementalist -> ok() )
      owner_coeff.ap_from_sp *= 1.0 + o() -> talent.primal_elementalist -> effectN( 1 ).percent();
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "travel"      ) return new travel_t( this );
    if ( name == "auto_attack" ) return new auto_melee_attack_t ( this );
    if ( name == "pulverize"   ) return new pulverize_t( this );

    return pet_t::create_action( name, options_str );
  }
};

struct fire_elemental_t : public pet_t
{
  struct travel_t : public action_t
  {
    travel_t( player_t* player ) : action_t( ACTION_OTHER, "travel", player ) {}
    virtual void execute() { player -> current.distance = 1; }
    virtual timespan_t execute_time() { return timespan_t::from_seconds( player -> current.distance / 10.0 ); }
    virtual bool ready() { return ( player -> current.distance > 1 ); }
    virtual timespan_t gcd() { return timespan_t::zero(); }
    virtual bool usable_moving() { return true; }
  };

  struct fire_elemental_spell_t : public spell_t
  {
    fire_elemental_t* p;

    fire_elemental_spell_t( const std::string& t, fire_elemental_t* p, const spell_data_t* s = spell_data_t::nil(), const std::string& options = std::string() ) :
      spell_t( t, p, s ), p( p )
    {
      parse_options( 0, options );

      school                      = SCHOOL_FIRE;
      may_crit                    = true;
      base_costs[ RESOURCE_MANA ] = 0;
      crit_bonus_multiplier      *= 1.0 + p -> o() -> spec.elemental_fury -> effectN( 1 ).percent();
    }

    virtual double composite_da_multiplier()
    {
      double m = spell_t::composite_da_multiplier();

      if ( p -> o() -> specialization() == SHAMAN_ENHANCEMENT )
        m *= 1.0 + p -> o() -> composite_mastery() * p -> o() -> mastery.enhanced_elements -> effectN( 1 ).mastery_value();

      return m;
    }

    virtual double composite_ta_multiplier()
    {
      double m = spell_t::composite_ta_multiplier();

      if ( p -> o() -> specialization() == SHAMAN_ENHANCEMENT )
        m *= 1.0 + p -> o() -> composite_mastery() * p -> o() -> mastery.enhanced_elements -> effectN( 1 ).mastery_value();

      return m;
    }
  };

  struct fire_nova_t  : public fire_elemental_spell_t
  {
    fire_nova_t( fire_elemental_t* player, const std::string& options ) :
      fire_elemental_spell_t( "fire_nova", player, player -> find_spell( 117588 ), options )
    {
      aoe = -1;
      base_dd_min        = p -> dbc.spell_scaling( p -> o() -> type, p -> level ) * .65;
      base_dd_max        = p -> dbc.spell_scaling( p -> o() -> type, p -> level ) * .65;
    }
  };

  struct fire_blast_t : public fire_elemental_spell_t
  {
    fire_blast_t( fire_elemental_t* player, const std::string& options ) :
      fire_elemental_spell_t( "fire_blast", player, player -> find_spell( 57984 ), options )
    {
      base_dd_min        = p -> dbc.spell_scaling( p -> o() -> type, p -> level ) / 1 / p -> level;
      base_dd_max        = p -> dbc.spell_scaling( p -> o() -> type, p -> level ) / 1 / p -> level;
    }

    virtual void execute()
    {
      fire_elemental_spell_t::execute();

      // Reset swing timer
      if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
        player -> main_hand_attack -> execute_event -> reschedule( player -> main_hand_attack -> execute_time() );
    }

    virtual bool usable_moving()
    {
      return true;
    }
  };

  struct immolate_t : public fire_elemental_spell_t
  {
    immolate_t( fire_elemental_t* player, const std::string& options ) :
      fire_elemental_spell_t( "immolate", player, player -> find_spell( 118297 ), options )
    {
      hasted_ticks  = true;
      tick_may_crit = true;
      base_costs[ RESOURCE_MANA ] = 0;
    }
  };

  struct fire_melee_t : public melee_attack_t
  {
    fire_melee_t( fire_elemental_t* player ) :
      melee_attack_t( "fire_melee", player, spell_data_t::nil() )
    {
      special                      = false;
      may_crit                     = true;
      background                   = true;
      repeating                    = true;
      direct_power_mod             = 1.0;
      base_spell_power_multiplier  = 1.0;
      base_attack_power_multiplier = 0.0;
      school                       = SCHOOL_FIRE;
      crit_bonus_multiplier       *= 1.0 + player -> o() -> spec.elemental_fury -> effectN( 1 ).percent();
      weapon_power_mod             = 0;
      base_dd_min                  = player -> dbc.spell_scaling( player -> o() -> type, player -> level );
      base_dd_max                  = player -> dbc.spell_scaling( player -> o() -> type, player -> level );
      if ( player -> o() -> talent.primal_elementalist -> ok() )
      {
        base_dd_min *= 1.5;
        base_dd_max *= 1.5;
      }
    }

    virtual void execute()
    {
      // If we're casting Immolate, we should clip a swing
      if ( time_to_execute > timespan_t::zero() && player -> executing )
        schedule_execute();
      else
        melee_attack_t::execute();
    }

    virtual void impact( action_state_t* state )
    {
      melee_attack_t::impact( state );

      fire_elemental_t* p = static_cast< fire_elemental_t* >( player );
      if ( result_is_hit( state -> result ) && p -> o() -> specialization() == SHAMAN_ENHANCEMENT )
        p -> o() -> buff.searing_flames -> trigger();
    }

    virtual double composite_da_multiplier()
    {
      double m = melee_attack_t::composite_da_multiplier();

      fire_elemental_t* p = static_cast< fire_elemental_t* >( player );
      if ( p -> o() -> specialization() == SHAMAN_ENHANCEMENT )
        m *= 1.0 + p -> o() -> composite_mastery() * p -> o() -> mastery.enhanced_elements -> effectN( 1 ).mastery_value();

      return m;
    }
  };

  struct auto_melee_attack_t : public melee_attack_t
  {
    auto_melee_attack_t( fire_elemental_t* player ) :
      melee_attack_t( "auto_attack", player )
    {
      player -> main_hand_attack = new fire_melee_t( player );
      player -> main_hand_attack -> weapon = &( player -> main_hand_weapon );
      player -> main_hand_attack -> base_execute_time = player -> main_hand_weapon.swing_time;

      trigger_gcd = timespan_t::zero();
    }

    virtual void execute()
    {
      player -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready()
    {
      if ( player  -> is_moving() ) return false;
      return ( player -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

  shaman_t* o() { return static_cast< shaman_t* >( owner ); }

  const spell_data_t* command;

  fire_elemental_t( sim_t* sim, shaman_t* owner, bool guardian ) :
    pet_t( sim, owner, ( ! guardian ) ? "primal_fire_elemental" : "greater_fire_elemental", guardian /*GUARDIAN*/ )
  {
    stamina_per_owner      = 1.0;
    command = owner -> find_spell( 65222 );
  }

  virtual void init_base()
  {
    pet_t::init_base();

    resources.base[ RESOURCE_HEALTH ] = 32268; // Level 85 value
    resources.base[ RESOURCE_MANA   ] = 8908; // Level 85 value

    //mana_per_intellect               = 4.5;
    //hp_per_stamina = 10.5;

    // FE Swings with a weapon, however the damage is "magical"
    main_hand_weapon.type            = WEAPON_BEAST;
    main_hand_weapon.min_dmg         = 0;
    main_hand_weapon.max_dmg         = 0;
    main_hand_weapon.damage          = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time      = timespan_t::from_seconds( 1.4 );

    owner_coeff.sp_from_sp = 0.4;
    if ( o() -> talent.primal_elementalist -> ok() )
      owner_coeff.sp_from_sp *= 1.5 * 1.2;
  }

  void init_actions()
  {
    action_list_str = "travel/auto_attack/fire_blast/fire_nova,if=active_enemies>=3";
    if ( type == PLAYER_PET )
      action_list_str += "/immolate,if=!ticking";

    pet_t::init_actions();
  }

  virtual resource_e primary_resource() { return RESOURCE_MANA; }

  double composite_player_multiplier( school_e school, action_t* a = 0 )
  {
    double m = pet_t::composite_player_multiplier( school, a );

    if ( owner -> race == RACE_ORC )
      m *= 1.0 + command -> effectN( 1 ).percent();

    return m;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "travel"      ) return new travel_t( this );
    if ( name == "fire_blast"  ) return new fire_blast_t( this, options_str );
    if ( name == "fire_nova"   ) return new fire_nova_t( this, options_str );
    if ( name == "auto_attack" ) return new auto_melee_attack_t( this );
    if ( name == "immolate"    ) return new immolate_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Shaman Ability Triggers
// ==========================================================================

// trigger_maelstrom_weapon =================================================

static bool trigger_maelstrom_weapon( shaman_melee_attack_t* a )
{
  assert( a -> weapon );

  bool procced = false;
  int mwstack = a -> p() -> buff.maelstrom_weapon -> check();

  double chance = a -> weapon -> proc_chance_on_swing( 10.0 );

  if ( a -> p() -> set_bonus.pvp_2pc_melee() )
    chance *= 1.2;

  if ( a -> p() -> specialization() == SHAMAN_ENHANCEMENT &&
       ( procced = a -> p() -> buff.maelstrom_weapon -> trigger( 1, buff_t::DEFAULT_VALUE(), chance ) ) )
  {
    if ( mwstack == a -> p() -> buff.maelstrom_weapon -> total_stack() )
    {
      a -> p() -> proc.wasted_mw -> occur();
      if ( a -> maelstrom_procs_wasted )
        a -> maelstrom_procs_wasted -> occur();
    }

    a -> p() -> proc.maelstrom_weapon -> occur();
    if ( a -> maelstrom_procs )
      a -> maelstrom_procs -> occur();
  }

  return procced;
}

// trigger_flametongue_weapon ===============================================

static void trigger_flametongue_weapon( shaman_melee_attack_t* a )
{
  shaman_t* p = a -> p();

  if ( a -> weapon -> slot == SLOT_MAIN_HAND )
    p -> flametongue_mh -> execute();
  else
    p -> flametongue_oh -> execute();
}

// trigger_windfury_weapon ==================================================

struct windfury_delay_event_t : public event_t
{
  shaman_melee_attack_t* wf;

  windfury_delay_event_t( shaman_melee_attack_t* wf, timespan_t delay ) :
    event_t( wf -> p(), "windfury_delay_event" ), wf( wf )
  {
    sim.add_event( this, delay );
  }

  virtual void execute()
  {
    shaman_t* p = wf -> p();

    p -> proc.windfury -> occur();
    wf -> execute();
    wf -> execute();
    wf -> execute();
  }
};

static bool trigger_windfury_weapon( shaman_melee_attack_t* a )
{
  shaman_t* p = a -> p();
  shaman_melee_attack_t* wf = 0;

  if ( a -> weapon -> slot == SLOT_MAIN_HAND )
    wf = p -> windfury_mh;
  else
    wf = p -> windfury_oh;

  if ( p -> cooldown.windfury_weapon -> down() ) return false;

  if ( p -> rng.windfury_weapon -> roll( wf -> data().proc_chance() ) )
  {
    p -> cooldown.windfury_weapon -> start( timespan_t::from_seconds( 3.0 ) );

    if ( p -> set_bonus.tier15_4pc_melee() )
      p -> cooldown.feral_spirits -> ready -= timespan_t::from_seconds( p -> sets -> set( SET_T15_4PC_MELEE ) -> effectN( 1 ).base_value() );

    // Delay windfury by some time, up to about a second
    new ( *p -> sim ) windfury_delay_event_t( wf, p -> rng.windfury_delay -> gauss( p -> wf_delay, p -> wf_delay_stddev ) );
    return true;
  }
  return false;
}

// trigger_rolling_thunder ==================================================

static bool trigger_rolling_thunder( shaman_spell_t* s )
{
  shaman_t* p = s -> p();

  if ( ! p -> buff.lightning_shield -> check() )
    return false;

  if ( p -> rng.rolling_thunder -> roll( p -> spec.rolling_thunder -> proc_chance() ) )
  {
    if ( p -> buff.lightning_shield -> check() == 1 )
      p -> ls_reset = s -> sim -> current_time;

    p -> resource_gain( RESOURCE_MANA,
                        p -> dbc.spell( 88765 ) -> effectN( 1 ).percent() * p -> resources.max[ RESOURCE_MANA ],
                        p -> gain.rolling_thunder );


    int stacks = ( p -> set_bonus.tier14_4pc_caster() ) ? 2 : 1;
    int wasted_stacks = ( p -> buff.lightning_shield -> check() + stacks ) - p -> buff.lightning_shield -> max_stack();

    for ( int i = 0; i < wasted_stacks; i++ )
    {
      if ( s -> sim -> current_time - p -> ls_reset >= p -> cooldown.shock -> duration )
        p -> proc.wasted_ls -> occur();
      else
        p -> proc.wasted_ls_shock_cd -> occur();
    }

    if ( wasted_stacks > 0 )
    {
      if ( s -> sim -> current_time - p -> ls_reset < p -> cooldown.shock -> duration )
        p -> proc.ls_fast -> occur();
      p -> ls_reset = timespan_t::zero();
    }

    p -> buff.lightning_shield -> trigger( stacks );
    p -> proc.rolling_thunder  -> occur();
    return true;
  }

  return false;
}

// trigger_static_shock =====================================================

static bool trigger_static_shock ( shaman_melee_attack_t* a )
{
  shaman_t* p = a -> p();

  if ( ! p -> buff.lightning_shield -> stack() )
    return false;

  if ( p -> rng.static_shock -> roll( p -> spec.static_shock -> proc_chance() ) )
  {
    p -> active_lightning_charge -> execute();
    p -> proc.static_shock -> occur();
    return true;
  }
  return false;
}

// trigger_improved_lava_lash ===============================================

static bool trigger_improved_lava_lash( shaman_melee_attack_t* a )
{
  struct improved_lava_lash_t : public shaman_spell_t
  {
    rng_t* imp_ll_rng;
    cooldown_t* imp_ll_fs_cd;
    stats_t* fs_dummy_stat;

    improved_lava_lash_t( shaman_t* p ) :
      shaman_spell_t( "improved_lava_lash", p ),
      imp_ll_rng( 0 ), imp_ll_fs_cd( 0 )
    {
      aoe = 4;
      may_miss = may_crit = false;
      proc = true;
      callbacks = false;
      background = true;

      imp_ll_rng = sim -> get_rng( "improved_ll" );
      imp_ll_fs_cd = player -> get_cooldown( "improved_ll_fs_cooldown" );
      imp_ll_fs_cd -> duration = timespan_t::zero();
      fs_dummy_stat = player -> get_stats( "flame_shock_dummy" );
    }

    // Exclude targets with your flame shock on
    size_t available_targets( std::vector< player_t* >& tl )
    {
      for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
      {
        if ( sim -> actor_list[ i ] -> current.sleeping )
          continue;

        if ( ! sim -> actor_list[ i ] -> is_enemy() )
          continue;

        if ( sim -> actor_list[ i ] == target )
          continue;

        if ( td( sim -> actor_list[ i ] ) -> dot.flame_shock -> ticking )
          continue;

        tl.push_back( sim -> actor_list[ i ] );
      }

      return tl.size();
    }

    std::vector< player_t* >& target_list()
    {
      size_t total_targets = available_targets( target_cache );

      // Reduce targets to aoe amount by removing random entries from the
      // target list until it's at aoe amount
      while ( total_targets > static_cast< size_t >( aoe ) )
      {
        target_cache.erase( target_cache.begin() + static_cast< size_t >( imp_ll_rng -> range( 0, total_targets ) ) );
        total_targets--;
      }

      return target_cache;
    }

    // Impact on any target triggers a flame shock; for now, cache the
    // relevant parts of the spell and return them after execute has finished
    void impact( action_state_t* state )
    {
      if ( sim -> debug )
        sim -> output( "%s spreads Flame Shock (off of %s) on %s",
                       player -> name(),
                       target -> name(),
                       state -> target -> name() );

      shaman_t* p = this -> p();

      double dd_min = p -> action_flame_shock -> base_dd_min,
             dd_max = p -> action_flame_shock -> base_dd_max,
             coeff = p -> action_flame_shock -> direct_power_mod,
             real_base_cost = p -> action_flame_shock -> base_costs[ p -> action_flame_shock -> current_resource() ];
      player_t* original_target = p -> action_flame_shock -> target;
      cooldown_t* original_cd = p -> action_flame_shock -> cooldown;
      stats_t* original_stats = p -> action_flame_shock -> stats;

      p -> action_flame_shock -> base_dd_min = 0;
      p -> action_flame_shock -> base_dd_max = 0;
      p -> action_flame_shock -> direct_power_mod = 0;
      p -> action_flame_shock -> background = true;
      p -> action_flame_shock -> callbacks = false;
      p -> action_flame_shock -> proc = true;
      p -> action_flame_shock -> may_crit = false;
      p -> action_flame_shock -> may_miss = false;
      p -> action_flame_shock -> base_costs[ p -> action_flame_shock -> current_resource() ] = 0;
      p -> action_flame_shock -> target = state -> target;
      p -> action_flame_shock -> cooldown = imp_ll_fs_cd;
      p -> action_flame_shock -> stats = fs_dummy_stat;

      p -> action_flame_shock -> execute();

      p -> action_flame_shock -> base_dd_min = dd_min;
      p -> action_flame_shock -> base_dd_max = dd_max;
      p -> action_flame_shock -> direct_power_mod = coeff;
      p -> action_flame_shock -> background = false;
      p -> action_flame_shock -> callbacks = true;
      p -> action_flame_shock -> proc = false;
      p -> action_flame_shock -> may_crit = true;
      p -> action_flame_shock -> may_miss = true;
      p -> action_flame_shock -> base_costs[ p -> action_flame_shock -> current_resource() ] = real_base_cost;
      p -> action_flame_shock -> target = original_target;
      p -> action_flame_shock -> cooldown = original_cd;
      p -> action_flame_shock -> stats = original_stats;

      // Hide the Flame Shock dummy stat and improved_lava_lash from reports
      fs_dummy_stat -> num_executes.clear();
      stats -> num_executes.clear();
    }
  };

  // Do not spread the love when there is only poor Fluffy Pillow against you
  if ( a -> sim -> num_enemies == 1 )
    return false;

  if ( ! a -> td( a -> target ) -> dot.flame_shock -> ticking )
    return false;

  shaman_t* p = a -> p();

  if ( p -> glyph.lava_lash -> ok() )
    return false;

  if ( ! p -> action_flame_shock )
  {
    p -> action_flame_shock = p -> find_action( "flame_shock" );
    assert( p -> action_flame_shock );
  }

  if ( ! p -> action_improved_lava_lash )
  {
    p -> action_improved_lava_lash = new improved_lava_lash_t( p );
    p -> action_improved_lava_lash -> init();
  }

  // Splash from the action's target
  p -> action_improved_lava_lash -> target = a -> target;
  p -> action_improved_lava_lash -> execute();

  return true;
}

static bool trigger_lightning_strike( const action_state_t* s )
{
  struct lightning_strike_t : public shaman_spell_t
  {
    lightning_strike_t( shaman_t* player ) :
      shaman_spell_t( "lightning_strike", player,
                      player -> sets -> set( SET_T15_2PC_CASTER ) -> effectN( 1 ).trigger() )
    {
      proc = background = true;
      callbacks = false;
      aoe = -1;
    }

    double composite_da_multiplier()
    {
      double m = shaman_spell_t::composite_da_multiplier();
      m *= 1 / static_cast< double >( target_cache.size() );
      return m;
    }
  };

  shaman_t* p = debug_cast< shaman_t* >( s -> action -> player );

  if ( !  p -> set_bonus.tier15_2pc_caster() )
    return false;

  if ( ! p -> action_lightning_strike )
  {
    p -> action_lightning_strike = new lightning_strike_t( p );
    p -> action_lightning_strike -> init();
  }

  // Do Echo of the Elements procs trigger this?
  if ( p -> rng.t15_2pc_caster -> roll( p -> sets -> set( SET_T15_2PC_CASTER ) -> proc_chance() ) )
  {
    p -> action_lightning_strike -> execute();
    return true;
  }

  return false;
}

// ==========================================================================
// Shaman Secondary Spells / Attacks
// ==========================================================================

struct lava_burst_overload_t : public shaman_spell_t
{
  lava_burst_overload_t( shaman_t* player ) :
    shaman_spell_t( "lava_burst_overload", player, player -> dbc.spell( 77451 ) )
  {
    overload             = true;
    background           = true;
    base_execute_time    = timespan_t::zero();
  }

  virtual double composite_da_multiplier()
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( p() -> buff.unleash_flame -> up() )
      m *= 1.0 + p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    double m = shaman_spell_t::composite_target_multiplier( target );

    if ( td( target ) -> debuff.unleashed_fury -> up() )
      m *= 1.0 + td( target ) -> debuff.unleashed_fury -> data().effectN( 2 ).percent();

    if ( td( target ) -> dot.flame_shock -> ticking )
      m *= 1.0 + p() -> spell.flame_shock -> effectN( 3 ).percent();

    return m;
  }

  virtual double composite_target_crit( player_t* )
  { return 1.0; }

  void execute()
  {
    shaman_spell_t::execute();

    // FIXME: DBC Value modified in dbc_t::apply_hotfixes()
    if ( p() -> set_bonus.tier15_4pc_caster() )
      p() -> cooldown.ascendance -> ready -= p() -> sets -> set( SET_T15_4PC_CASTER ) -> effectN( 1 ).time_value();
  }
};

struct lightning_bolt_overload_t : public shaman_spell_t
{
  lightning_bolt_overload_t( shaman_t* player ) :
    shaman_spell_t( "lightning_bolt_overload", player, player -> dbc.spell( 45284 ) )
  {
    overload             = true;
    background           = true;
    base_execute_time    = timespan_t::zero();
    base_multiplier     += player -> spec.shamanism -> effectN( 1 ).percent();
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    double m = shaman_spell_t::composite_target_multiplier( target );

    if ( td( target ) -> debuff.unleashed_fury -> up() )
      m *= 1.0 + td( target ) -> debuff.unleashed_fury -> data().effectN( 1 ).percent();

    return m;
  }

  void impact( action_state_t* state )
  {
    shaman_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      trigger_rolling_thunder( this );
      trigger_lightning_strike( state );
    }
  }
};

struct chain_lightning_overload_t : public shaman_spell_t
{
  chain_lightning_overload_t( shaman_t* player ) :
    shaman_spell_t( "chain_lightning_overload", player, player -> dbc.spell( 45297 ) )
  {
    overload             = true;
    background           = true;
    base_execute_time    = timespan_t::zero();
    base_multiplier     += player -> spec.shamanism -> effectN( 2 ).percent();
    aoe                  = 3 + player -> glyph.chain_lightning -> effectN( 1 ).base_value();
    base_add_multiplier  = data().effectN( 1 ).chain_multiplier();
  }

  double composite_da_multiplier()
  {
    double m = shaman_spell_t::composite_da_multiplier();

    m *= 1.0 + p() -> glyph.chain_lightning -> effectN( 2 ).percent();

    return m;
  }

  void impact( action_state_t* state )
  {
    shaman_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      trigger_rolling_thunder( this );

      trigger_lightning_strike( state );
    }
  }
};

struct lava_beam_overload_t : public shaman_spell_t
{
  lava_beam_overload_t( shaman_t* player ) :
    shaman_spell_t( "lava_beam_overload", player, player -> dbc.spell( 114738 ) )
  {
    overload             = true;
    background           = true;
    base_execute_time    = timespan_t::zero();
    base_costs[ RESOURCE_MANA ] = 0;
    base_multiplier     += p() -> spec.shamanism -> effectN( 2 ).percent();
    aoe                  = 5;
    base_add_multiplier  = data().effectN( 1 ).chain_multiplier();
  }

  virtual double composite_da_multiplier()
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( p() -> buff.unleash_flame -> up() )
      m *= 1.0 + p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }

  void impact( action_state_t* state )
  {
    shaman_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      trigger_rolling_thunder( this );
      trigger_lightning_strike( state );
    }
  }
};

struct elemental_blast_overload_t : public shaman_spell_t
{
  elemental_blast_overload_t( shaman_t* player ) :
    shaman_spell_t( "elemental_blast_overload", player, player -> dbc.spell( 120588 ) )
  {
    overload             = true;
    background           = true;
    base_execute_time    = timespan_t::zero();
  }

  virtual double composite_da_multiplier()
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( p() -> buff.unleash_flame -> up() )
      m *= 1.0 + p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }
};

struct lightning_charge_t : public shaman_spell_t
{
  int threshold;

  lightning_charge_t( const std::string& n, shaman_t* player ) :
    shaman_spell_t( n, player, player -> dbc.spell( 26364 ) ),
    threshold( static_cast< int >( player -> spec.fulmination -> effectN( 1 ).base_value() ) )
  {
    // Use the same name "lightning_shield" to make sure the cost of refreshing the shield is included with the procs.
    background       = true;
    may_crit         = true;
    may_proc_eoe     = false;
  }

  double composite_target_crit( player_t* target )
  {
    double c = spell_t::composite_target_crit( target );

    if ( td( target ) -> debuff.stormstrike -> up() )
    {
      c += td( target ) -> debuff.stormstrike -> data().effectN( 1 ).percent();
      c += player -> sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
    }

    return c;
  }

  virtual double composite_da_multiplier()
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( threshold > 0 )
    {
      int consuming_stack =  p() -> buff.lightning_shield -> check() - threshold;
      if ( consuming_stack > 0 )
        m *= consuming_stack;
    }

    return m;
  }
};

struct unleash_flame_t : public shaman_spell_t
{
  unleash_flame_t( shaman_t* player ) :
    shaman_spell_t( "unleash_flame", player, player -> dbc.spell( 73683 ) )
  {
    harmful              = true;
    background           = true;
    //proc                 = true;

    // Don't cooldown here, unleash elements ability will handle it
    cooldown -> duration = timespan_t::zero();
  }

  virtual void execute()
  {
    if ( p() -> main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    {
      shaman_spell_t::execute();
    }
    else if ( p() -> off_hand_weapon.type != WEAPON_NONE &&
              p() -> off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    {
      shaman_spell_t::execute();
    }
  }

  void impact( action_state_t* state )
  {
    shaman_spell_t::impact( state );

    if ( result_is_hit( state -> result ) && p() -> talent.unleashed_fury -> ok() )
      td( state -> target ) -> debuff.unleashed_fury -> trigger();
  }
};

struct flametongue_weapon_spell_t : public shaman_spell_t
{
  // static constant floats aren't allowed by the spec and some compilers
  static double normalize_speed()   { return 2.649845; }
  static double power_coefficient() { return 0.059114; }

  flametongue_weapon_spell_t( const std::string& n, shaman_t* player, weapon_t* w ) :
    shaman_spell_t( n, player, player -> dbc.spell( 8024 ) )
  {
    may_crit           = true;
    background         = true;
    proc               = true;
    direct_power_mod   = 1.0;
    base_costs[ RESOURCE_MANA ] = 0.0;

    base_dd_min = w -> swing_time.total_seconds() / normalize_speed() * ( data().effectN( 2 ).average( player ) / 77.0 + data().effectN( 2 ).average( player ) / 25.0 ) / 2;
    base_dd_max = base_dd_min;

    if ( player -> specialization() == SHAMAN_ENHANCEMENT )
    {
      snapshot_flags               = STATE_AP;
      base_attack_power_multiplier = w -> swing_time.total_seconds() / normalize_speed() * power_coefficient();
      base_spell_power_multiplier  = 0;
    }
    else
    {
      base_attack_power_multiplier = 0;
      base_spell_power_multiplier  = w -> swing_time.total_seconds() / normalize_speed() * power_coefficient();
    }
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    double m = shaman_spell_t::composite_target_multiplier( target );

    m *= 1.0 + p() -> buff.searing_flames -> stack() * 0.08;

    return m;
  }
};

struct ancestral_awakening_t : public shaman_heal_t
{
  ancestral_awakening_t( shaman_t* player ) :
    shaman_heal_t( "ancestral_awakening", player, player -> find_spell( 52752 ) )
  {
    background = proc = true;
  }

  double composite_da_multiplier()
  {
    double m = shaman_heal_t::composite_da_multiplier();
    m *= p() -> spec.ancestral_awakening -> effectN( 1 ).percent();
    return m;
  }

  void execute()
  {
    target = find_lowest_player();
    shaman_heal_t::execute();
  }
};

struct windfury_weapon_melee_attack_t : public shaman_melee_attack_t
{
  windfury_weapon_melee_attack_t( const std::string& n, shaman_t* player, weapon_t* w ) :
    shaman_melee_attack_t( n, player, player -> dbc.spell( 33757 ) )
  {
    weapon           = w;
    school           = SCHOOL_PHYSICAL;
    background       = true;
    //callbacks        = false; // Windfury does not proc any On-Equip procs, apparently
  }

  virtual double composite_attack_power()
  {
    double ap = shaman_melee_attack_t::composite_attack_power();

    return ap + weapon -> buff_value;
  }

  void impact( action_state_t* state )
  {
    shaman_melee_attack_t::impact( state );

    if ( result_is_hit( state -> result ) && p() -> buff.unleashed_fury_wf -> up() )
      trigger_static_shock( this );
  }
};

struct unleash_wind_t : public shaman_melee_attack_t
{
  unleash_wind_t( shaman_t* player ) :
    shaman_melee_attack_t( "unleash_wind", player, player -> dbc.spell( 73681 ) )
  {
    background = true;
    may_proc_primal_wisdom = may_dodge = may_parry = false;

    // Don't cooldown here, unleash elements will handle it
    cooldown -> duration = timespan_t::zero();
  }

  void execute()
  {
    // Figure out weapons
    if ( p() -> main_hand_weapon.buff_type == WINDFURY_IMBUE )
    {
      weapon = &( p() -> main_hand_weapon );
      shaman_melee_attack_t::execute();
    }
    else if ( p() -> off_hand_weapon.type != WEAPON_NONE &&
              p() -> off_hand_weapon.buff_type == WINDFURY_IMBUE )
    {
      weapon = &( p() -> off_hand_weapon );
      shaman_melee_attack_t::execute();
    }
  }
};

struct stormstrike_melee_attack_t : public shaman_melee_attack_t
{
  stormstrike_melee_attack_t( const std::string& n, shaman_t* player, const spell_data_t* s, weapon_t* w ) :
    shaman_melee_attack_t( n, player, s )
  {
    may_proc_windfury = background = true;
    may_miss = may_dodge = may_parry = false;
    weapon = w;
  }

  void impact( action_state_t* s )
  {
    shaman_melee_attack_t::impact( s );

    trigger_static_shock( this );
  }
};

struct windlash_t : public shaman_melee_attack_t
{
  double swing_timer_variance;
  rng_t* swing_time;

  windlash_t( const std::string& n, const spell_data_t* s, shaman_t* player, weapon_t* w, double stv ) :
    shaman_melee_attack_t( n, player, s ), swing_timer_variance( stv )
  {
    may_proc_windfury = background = repeating = may_miss = may_dodge = may_parry = true;
    proc = may_glance = special = false;
    weapon            = w;
    base_execute_time = w -> swing_time;
    trigger_gcd       = timespan_t::zero();

    swing_time = p() -> get_rng( "swing_timer_variance" );
  }

  timespan_t execute_time()
  {
    timespan_t t = shaman_melee_attack_t::execute_time();

    if ( swing_timer_variance > 0 )
    {
      timespan_t st = timespan_t::from_seconds( swing_time -> gauss( t.total_seconds(), t.total_seconds() * swing_timer_variance ) );
      if ( sim -> debug )
        sim -> output( "Swing timer variance for %s, real_time=%.3f swing_timer=%.3f", name(), t.total_seconds(), st.total_seconds() );

      return st;
    }
    else
      return t;
  }

  void execute()
  {
    if ( time_to_execute > timespan_t::zero() && p() -> executing )
    {
      if ( sim -> debug )
        sim_t::output( sim, "Executing '%s' during melee (%s).", p() -> executing -> name(), util::slot_type_string( weapon -> slot ) );

      if ( weapon -> slot == SLOT_OFF_HAND )
        p() -> proc.swings_clipped_oh -> occur();
      else
        p() -> proc.swings_clipped_mh -> occur();

      schedule_execute();
    }
    else
    {
      p() -> buff.flurry -> decrement();
      p() -> buff.unleash_wind -> decrement();

      shaman_melee_attack_t::execute();
    }
  }
};

// ==========================================================================
// Shaman Action / Spell Base
// ==========================================================================

// shaman_spell_base_t::schedule_execute ====================================

template <class Base>
void shaman_spell_base_t<Base>::schedule_execute( action_state_t* state )
{
  shaman_t* p = ab::p();

  if ( p -> sim -> log )
    p -> sim -> output( "%s schedules execute for %s", p -> name(), ab::name() );

  ab::time_to_execute = execute_time();

  ab::execute_event = ab::start_action_execute_event( ab::time_to_execute, state );

  if ( ! ab::background )
  {
    p -> executing = this;
    p -> gcd_ready = p -> sim -> current_time + ab::gcd();
    if ( p -> action_queued && p -> sim -> strict_gcd_queue )
      p -> gcd_ready -= p -> sim -> queue_gcd_reduction;
  }

  if ( ab::instant_eligibility() && ! use_ancestral_swiftness() && p -> specialization() == SHAMAN_ENHANCEMENT )
    p -> proc.maelstrom_weapon_used[ p -> buff.maelstrom_weapon -> check() ] -> occur();
}

// shaman_spell_base_t::execute =============================================

template <class Base>
void shaman_spell_base_t<Base>::execute()
{
  ab::execute();

  bool as_cast = use_ancestral_swiftness();
  bool eligible_for_instant = ab::instant_eligibility();
  shaman_t* p = ab::p();

  // Ancestral swiftness handling, note that MW / AS share the same "conditions" for use
  // Maelstromable nature spell with less than 5 stacks is going to consume ancestral swiftness
  if ( as_cast )
  {
    p -> buff.ancestral_swiftness -> expire();
    p -> cooldown.ancestral_swiftness -> start();
  }

  // Shamans have specialized swing timer reset system, where every cast time spell
  // resets the swing timers, _IF_ the spell is not maelstromable, or the maelstrom
  // weapon stack is zero, or ancient swiftness was not used to cast the spell
  if ( ( ! eligible_for_instant || p -> buff.maelstrom_weapon -> check() == 0 ) &&
       ! as_cast && execute_time() > timespan_t::zero() )
  {
    if ( ab::sim -> debug )
    {
      ab::sim -> output( "Resetting swing timers for '%s', instant_eligibility=%d mw_stacks=%d",
                         ab::name_str.c_str(), eligible_for_instant, p -> buff.maelstrom_weapon -> check() );
    }

    timespan_t time_to_next_hit;

    if ( ab::player -> main_hand_attack && ab::player -> main_hand_attack -> execute_event )
    {
      time_to_next_hit = ab::player -> main_hand_attack -> execute_time();
      ab::player -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      p -> proc.swings_reset_mh -> occur();
    }

    if ( ab::player -> off_hand_attack && ab::player -> off_hand_attack -> execute_event )
    {
      time_to_next_hit = ab::player -> off_hand_attack -> execute_time();
      ab::player -> off_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      p -> proc.swings_reset_oh -> occur();
    }
  }

  if ( eligible_for_instant && ! as_cast && p -> specialization() == SHAMAN_ENHANCEMENT )
    p -> buff.maelstrom_weapon -> expire();

  p -> buff.spiritwalkers_grace -> up();
}

// shaman_spell_base_t::impact ===============================================

template <class Base>
void shaman_spell_base_t<Base>::impact( action_state_t* state )
{
  shaman_action_state_t* ss = debug_cast< shaman_action_state_t* >( state );

  if ( ss -> eoe_proc )
  {
    stats_t* tmp_stats = 0;

    if ( ab::stats != eoe_stats )
    {
      tmp_stats = ab::stats;
      ab::stats = eoe_stats;
    }

    ab::impact( state );

    if ( tmp_stats != 0 )
      ab::stats = tmp_stats;
  }
  else
    ab::impact( state );

  // Triggers wont happen for (EoE) procs or totems
  if ( ab::proc || ! ab::callbacks  || ss -> eoe_proc )
    return;

  shaman_t* p = ab::p();

  if ( may_proc_eoe && ab::result_is_hit( state -> result ) &&
       ab::harmful && ! ab::proc && ab::base_dd_min > 0 &&
       p -> talent.echo_of_the_elements -> ok() &&
       p -> rng.echo_of_the_elements -> roll( eoe_proc_chance ) &&
       p -> cooldown.echo_of_the_elements -> up() )
  {
    if ( ab::sim -> debug ) ab::sim -> output( "Echo of the Elements procs for %s", ab::name() );
    new ( *ab::sim ) eoe_execute_event_t< Base >( this );
    p -> cooldown.echo_of_the_elements -> start( timespan_t::from_seconds( 0.1 ) );
  }
}

// shaman_heal_t::impact ====================================================

void shaman_heal_t::impact( action_state_t* s )
{
  // Todo deep healing to adjust s -> result_amount by x% before impacting
  if ( sim -> debug && p() -> mastery.deep_healing -> ok() )
  {
    sim -> output( "%s Deep Heals %s@%.2f%% mul=%.3f %.0f -> %.0f",
                   player -> name(), s -> target -> name(),
                   s -> target -> health_percentage(), deep_healing( s ),
                   s -> result_amount, s -> result_amount * deep_healing( s ) );
  }

  s -> result_amount *= deep_healing( s );

  base_t::impact( s );

  if ( proc_tidal_waves )
    p() -> buff.tidal_waves -> trigger( p() -> buff.tidal_waves -> data().initial_stacks() );

  if ( s -> result == RESULT_CRIT )
  {
    if ( resurgence_gain > 0 )
      p() -> resource_gain( RESOURCE_MANA, resurgence_gain, p() -> gain.resurgence );

    if ( p() -> spec.ancestral_awakening -> ok() )
    {
      if ( ! p() -> action_ancestral_awakening )
      {
        p() -> action_ancestral_awakening = new ancestral_awakening_t( p() );
        p() -> action_ancestral_awakening -> init();
      }

      const heal_state_t* hs = debug_cast< const heal_state_t* >( s );
      p() -> action_ancestral_awakening -> base_dd_min = hs -> total_result_amount;
      p() -> action_ancestral_awakening -> base_dd_max = hs -> total_result_amount;
    }
  }

  if ( p() -> main_hand_weapon.buff_type == EARTHLIVING_IMBUE )
  {
    double chance = ( s -> target -> resources.pct( RESOURCE_HEALTH ) > .35 ) ? elw_proc_high : elw_proc_low;

    if ( p() -> rng.earthliving -> roll( chance ) )
    {
      // Todo proc earthliving on target
    }
  }
}

// ==========================================================================
// Shaman Attack
// ==========================================================================

// shaman_melee_attack_t::impact ============================================

void shaman_melee_attack_t::impact( action_state_t* state )
{
  base_t::impact( state );

  // Bail out early if the result is a miss/dodge/parry
  if ( ! result_is_hit( state -> result ) )
    return;

  if ( may_proc_maelstrom )
    trigger_maelstrom_weapon( this );

  if ( may_proc_windfury && weapon -> buff_type == WINDFURY_IMBUE )
    trigger_windfury_weapon( this );

  if ( may_proc_flametongue && weapon -> buff_type == FLAMETONGUE_IMBUE )
    trigger_flametongue_weapon( this );

  if ( may_proc_primal_wisdom && p() -> rng.primal_wisdom -> roll( p() -> spec.primal_wisdom -> proc_chance() ) )
  {
    double amount = p() -> spell.primal_wisdom -> effectN( 1 ).percent() * p() -> resources.base[ RESOURCE_MANA ];
    p() -> resource_gain( RESOURCE_MANA, amount, p() -> gain.primal_wisdom );
  }

  if ( state -> result == RESULT_CRIT )
    p() -> buff.flurry -> trigger( p() -> buff.flurry -> data().initial_stacks() );
}

// Melee Attack =============================================================

struct melee_t : public shaman_melee_attack_t
{
  int sync_weapons;
  bool first;
  rng_t* swing_time;
  double swing_timer_variance;

  melee_t( const std::string& name, const spell_data_t* s, shaman_t* player, weapon_t* w, int sw, double stv ) :
    shaman_melee_attack_t( name, player, s ), sync_weapons( sw ),
    first( true ), swing_timer_variance( stv )
  {
    may_proc_windfury = background = repeating = may_glance = true;
    special           = false;
    trigger_gcd       = timespan_t::zero();
    weapon            = w;
    base_execute_time = w -> swing_time;

    if ( p() -> specialization() == SHAMAN_ENHANCEMENT && p() -> dual_wield() ) base_hit -= 0.19;
    swing_time = p() -> get_rng( "swing_timer_variance" );
  }

  void reset()
  {
    shaman_melee_attack_t::reset();

    first = true;
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = shaman_melee_attack_t::execute_time();
    if ( first )
    {
      first = false;
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t/2, timespan_t::from_seconds( 0.01 ) ) : t/2 ) : timespan_t::from_seconds( 0.01 );
    }

    if ( swing_timer_variance > 0 )
    {
      timespan_t st = timespan_t::from_seconds( swing_time -> gauss( t.total_seconds(), t.total_seconds() * swing_timer_variance ) );
      if ( sim -> debug )
        sim -> output( "Swing timer variance for %s, real_time=%.3f swing_timer=%.3f", name(), t.total_seconds(), st.total_seconds() );
      return st;
    }
    else
      return t;
  }

  void execute()
  {
    if ( time_to_execute > timespan_t::zero() && p() -> executing )
    {
      if ( sim -> debug )
        sim_t::output( sim, "Executing '%s' during melee (%s).", p() -> executing -> name(), util::slot_type_string( weapon -> slot ) );

      if ( weapon -> slot == SLOT_OFF_HAND )
        p() -> proc.swings_clipped_oh -> occur();
      else
        p() -> proc.swings_clipped_mh -> occur();

      schedule_execute();
    }
    else
    {
      p() -> buff.flurry -> decrement();
      p() -> buff.unleash_wind -> decrement();

      shaman_melee_attack_t::execute();
    }
  }

  void impact( action_state_t* state )
  {
    shaman_melee_attack_t::impact( state );

    if ( result_is_hit( state -> result ) && p() -> buff.unleashed_fury_wf -> up() )
      trigger_static_shock( this );
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public shaman_melee_attack_t
{
  int sync_weapons;
  double swing_timer_variance;

  auto_attack_t( shaman_t* player, const std::string& options_str ) :
    shaman_melee_attack_t( "auto_attack", player, spell_data_t::nil() ),
    sync_weapons( 0 ), swing_timer_variance( 0.00 )
  {
    option_t options[] =
    {
      opt_bool( "sync_weapons", sync_weapons ),
      opt_float( "swing_timer_variance", swing_timer_variance ),
      opt_null()
    };
    parse_options( options, options_str );

    assert( p() -> main_hand_weapon.type != WEAPON_NONE );

    p() -> melee_mh      = new melee_t( "melee_main_hand", spell_data_t::nil(), player, &( p() -> main_hand_weapon ), sync_weapons, swing_timer_variance );
    p() -> melee_mh      -> school = SCHOOL_PHYSICAL;
    p() -> ascendance_mh = new windlash_t( "windlash_main_hand", player -> find_spell( 114089 ), player, &( p() -> main_hand_weapon ), swing_timer_variance );
    p() -> ascendance_mh -> school = SCHOOL_NATURE;

    p() -> main_hand_attack = p() -> melee_mh;

    if ( p() -> off_hand_weapon.type != WEAPON_NONE && p() -> specialization() == SHAMAN_ENHANCEMENT )
    {
      if ( ! p() -> dual_wield() ) return;

      p() -> melee_oh = new melee_t( "melee_off_hand", spell_data_t::nil(), player, &( p() -> off_hand_weapon ), sync_weapons, swing_timer_variance );
      p() -> melee_oh -> school = SCHOOL_PHYSICAL;
      p() -> ascendance_oh = new windlash_t( "windlash_off_hand", player -> find_spell( 114093 ), player, &( p() -> off_hand_weapon ), swing_timer_variance );
      p() -> ascendance_oh -> school = SCHOOL_NATURE;

      p() -> off_hand_attack = p() -> melee_oh;

      p() -> off_hand_attack -> id = 1;
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    p() -> main_hand_attack -> schedule_execute();
    if ( p() -> off_hand_attack )
      p() -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    if ( p() -> is_moving() ) return false;
    return ( p() -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Lava Lash Attack =========================================================

struct lava_lash_t : public shaman_melee_attack_t
{
  double ft_bonus;
  double sf_bonus;

  lava_lash_t( shaman_t* player, const std::string& options_str ) :
    shaman_melee_attack_t( player, player -> find_class_spell( "Lava Lash" ) ),
    ft_bonus( data().effectN( 2 ).percent() ),
    sf_bonus( player -> spell.searing_flames -> effectN( 1 ).percent() )
  {
    check_spec( SHAMAN_ENHANCEMENT );
    may_proc_windfury = true;
    school = SCHOOL_FIRE;

    base_multiplier += player -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();

    parse_options( NULL, options_str );

    weapon              = &( player -> off_hand_weapon );

    if ( weapon -> type == WEAPON_NONE )
      background        = true; // Do not allow execution.
  }

  // Lava Lash multiplier calculation from
  // http://elitistjerks.com/f79/t110302-enhsim_cataclysm/p11/#post1935780
  // MoP: Vastly simplified, most bonuses are gone
  virtual double action_da_multiplier()
  {
    double m = shaman_melee_attack_t::action_da_multiplier();

    m *= 1.0 + p() -> buff.searing_flames -> check() * sf_bonus +
         ( weapon -> buff_type == FLAMETONGUE_IMBUE ) * ft_bonus;

    return m;
  }

  void impact( action_state_t* state )
  {
    shaman_melee_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      p() -> buff.searing_flames -> expire();

      trigger_static_shock( this );
      if ( td( state -> target ) -> dot.flame_shock -> ticking )
        trigger_improved_lava_lash( this );
    }
  }
};

// Primal Strike Attack =====================================================

struct primal_strike_t : public shaman_melee_attack_t
{
  primal_strike_t( shaman_t* player, const std::string& options_str ) :
    shaman_melee_attack_t( player, player -> find_class_spell( "Primal Strike" ) )
  {
    parse_options( NULL, options_str );
    may_proc_windfury    = true;
    weapon               = &( p() -> main_hand_weapon );
    cooldown             = p() -> cooldown.strike;
    cooldown -> duration = p() -> dbc.spell( id ) -> cooldown();
  }

  void impact( action_state_t* state )
  {
    shaman_melee_attack_t::impact( state );
    if ( result_is_hit( state -> result ) )
      trigger_static_shock( this );
  }
};

// Stormstrike Attack =======================================================

struct stormstrike_t : public shaman_melee_attack_t
{
  stormstrike_melee_attack_t * stormstrike_mh;
  stormstrike_melee_attack_t * stormstrike_oh;

  stormstrike_t( shaman_t* player, const std::string& options_str ) :
    shaman_melee_attack_t( "stormstrike", player, player -> find_class_spell( "Stormstrike" ) ),
    stormstrike_mh( 0 ), stormstrike_oh( 0 )
  {
    check_spec( SHAMAN_ENHANCEMENT );

    parse_options( NULL, options_str );

    weapon               = &( p() -> main_hand_weapon );
    weapon_multiplier    = 0.0;
    may_crit             = false;
    cooldown             = p() -> cooldown.strike;
    cooldown -> duration = p() -> dbc.spell( id ) -> cooldown();

    // Actual damaging attacks are done by stormstrike_melee_attack_t
    // stormstrike_melee_attack_t( std::string& n, shaman_t* player, const spell_data_t* s, weapon_t* w ) :
    stormstrike_mh = new stormstrike_melee_attack_t( "stormstrike_mh", player, data().effectN( 2 ).trigger(), &( player -> main_hand_weapon ) );
    add_child( stormstrike_mh );

    if ( p() -> off_hand_weapon.type != WEAPON_NONE )
    {
      stormstrike_oh = new stormstrike_melee_attack_t( "stormstrike_oh", player, data().effectN( 3 ).trigger(), &( player -> off_hand_weapon ) );
      add_child( stormstrike_oh );
    }
  }

  void execute()
  {
    shaman_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) && p() -> set_bonus.tier15_2pc_melee() )
    {
      int mwstack = p() -> buff.maelstrom_weapon -> total_stack();
      int bonus = p() -> sets -> set( SET_T15_2PC_MELEE ) -> effectN( 1 ).base_value();

      p() -> buff.maelstrom_weapon -> trigger( bonus, buff_t::DEFAULT_VALUE(), 1.0 );

      for ( int i = 0; i < ( mwstack + bonus ) - p() -> buff.maelstrom_weapon -> max_stack(); i++ )
      {
        p() -> proc.wasted_t15_2pc_melee -> occur();
        p() -> proc.wasted_mw -> occur();
      }

      for ( int i = 0; i < bonus; i++ )
      {
        p() -> proc.t15_2pc_melee -> occur();
        p() -> proc.maelstrom_weapon -> occur();
      }
    }
  }

  void impact( action_state_t* state )
  {
    // Bypass shaman-specific attack based procs and co for this spell, the relevant ones
    // are handled by stormstrike_melee_attack_t
    melee_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      td( state -> target ) -> debuff.stormstrike -> trigger();

      stormstrike_mh -> execute();
      if ( stormstrike_oh ) stormstrike_oh -> execute();
    }
  }

  bool ready()
  {
    if ( p() -> buff.ascendance -> check() )
      return false;

    return shaman_melee_attack_t::ready();
  }
};

// Stormblast Attack ========================================================

struct stormblast_t : public shaman_melee_attack_t
{
  stormstrike_melee_attack_t * stormblast_mh;
  stormstrike_melee_attack_t * stormblast_oh;

  stormblast_t( shaman_t* player, const std::string& options_str ) :
    shaman_melee_attack_t( "stormblast", player, player -> find_spell( 115356 ) ),
    stormblast_mh( 0 ), stormblast_oh( 0 )
  {
    check_spec( SHAMAN_ENHANCEMENT );

    parse_options( NULL, options_str );

    school               = SCHOOL_NATURE;
    weapon               = &( p() -> main_hand_weapon );
    weapon_multiplier    = 0.0;
    may_crit             = false;
    cooldown             = p() -> cooldown.strike;
    cooldown -> duration = p() -> dbc.spell( id ) -> cooldown();

    // Actual damaging attacks are done by stormstrike_melee_attack_t
    stormblast_mh = new stormstrike_melee_attack_t( "stormblast_mh", player, data().effectN( 2 ).trigger(), &( player -> main_hand_weapon ) );
    stormblast_mh -> school = SCHOOL_NATURE;
    add_child( stormblast_mh );

    if ( p() -> off_hand_weapon.type != WEAPON_NONE )
    {
      stormblast_oh = new stormstrike_melee_attack_t( "stormblast_oh", player, data().effectN( 3 ).trigger(), &( player -> off_hand_weapon ) );
      stormblast_oh -> school = SCHOOL_NATURE;
      add_child( stormblast_oh );
    }
  }

  void execute()
  {
    shaman_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) && p() -> set_bonus.tier15_2pc_melee() )
    {
      int mwstack = p() -> buff.maelstrom_weapon -> total_stack();
      int bonus = p() -> sets -> set( SET_T15_2PC_MELEE ) -> effectN( 1 ).base_value();

      p() -> buff.maelstrom_weapon -> trigger( bonus, buff_t::DEFAULT_VALUE(), 1.0 );
      for ( int i = 0; i < ( mwstack + bonus ) - p() -> buff.maelstrom_weapon -> max_stack(); i++ )
      {
        p() -> proc.wasted_t15_2pc_melee -> occur();
        p() -> proc.wasted_mw -> occur();
      }

      for ( int i = 0; i < bonus; i++ )
      {
        p() -> proc.t15_2pc_melee -> occur();
        p() -> proc.maelstrom_weapon -> occur();
      }
    }
  }

  void impact( action_state_t* state )
  {
    // Bypass shaman-specific attack based procs and co for this spell, the relevant ones
    // are handled by stormstrike_melee_attack_t
    melee_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      td( state -> target ) -> debuff.stormstrike -> trigger();

      stormblast_mh -> execute();
      if ( stormblast_oh ) stormblast_oh -> execute();
    }
  }

  bool ready()
  {
    if ( ! p() -> buff.ascendance -> check() )
      return false;

    return shaman_melee_attack_t::ready();
  }
};

// ==========================================================================
// Shaman Spell
// ==========================================================================

// shaman_spell_t::execute =================================================

void shaman_spell_t::execute()
{
  base_t::execute();

  // Eoe procs will not go further than this.
  if ( eoe_proc )
    return;

  if ( ! totem && ! proc && data().school_mask() & SCHOOL_MASK_FIRE )
    p() -> buff.unleash_flame -> expire();

  if ( overload_spell )
  {
    double overload_chance = p() -> composite_mastery() *
                             p() -> mastery.elemental_overload -> effectN( 1 ).mastery_value() *
                             overload_chance_multiplier;

    if ( overload_chance && p() -> rng.elemental_overload -> roll( overload_chance ) )
    {
      overload_spell -> execute();
      if ( p() -> set_bonus.tier13_4pc_caster() )
        p() -> buff.tier13_4pc_caster -> trigger();
    }
  }
}

// ==========================================================================
// Shaman Spells
// ==========================================================================

// Bloodlust Spell ==========================================================

struct bloodlust_t : public shaman_spell_t
{
  bloodlust_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Bloodlust" ), options_str )
  {
    harmful = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    for ( size_t i = 0; i < sim -> player_list.size(); ++i )
    {
      player_t* p = sim -> player_list[ i ];
      if ( p -> current.sleeping || p -> buffs.exhaustion -> check() || p -> is_pet() || p -> is_enemy() )
        continue;
      p -> buffs.bloodlust -> trigger();
      p -> buffs.exhaustion -> trigger();
    }
  }

  virtual bool ready()
  {
    if ( p() -> buffs.exhaustion -> check() )
      return false;

    if (  p() -> buffs.bloodlust -> cooldown -> down() )
      return false;

    return shaman_spell_t::ready();
  }
};

// Chain Lightning Spell ====================================================

struct chain_lightning_t : public shaman_spell_t
{
  chain_lightning_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Chain Lightning" ), options_str )
  {
    base_execute_time    += player -> spec.shamanism        -> effectN( 3 ).time_value();
    cooldown -> duration += player -> spec.shamanism        -> effectN( 4 ).time_value();
    base_multiplier      += player -> spec.shamanism        -> effectN( 2 ).percent();
    aoe                   = player -> glyph.chain_lightning -> effectN( 1 ).base_value() + 3;
    base_add_multiplier   = data().effectN( 1 ).chain_multiplier();

    overload_spell        = new chain_lightning_overload_t( player );
    overload_chance_multiplier = 0.3;
    add_child( overload_spell );
  }

  double composite_da_multiplier()
  {
    double m = shaman_spell_t::composite_da_multiplier();

    m *= 1.0 + p() -> glyph.chain_lightning -> effectN( 2 ).percent();

    return m;
  }

  double composite_target_crit( player_t* target )
  {
    double c = spell_t::composite_target_crit( target );

    if ( td( target ) -> debuff.stormstrike -> up() )
    {
      c += td( target ) -> debuff.stormstrike -> data().effectN( 1 ).percent();
      c += player -> sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
    }

    return c;
  }

  void impact( action_state_t* state )
  {
    shaman_spell_t::impact( state );

    const shaman_action_state_t* ss = static_cast< const shaman_action_state_t* >( state );
    if ( ! ss -> eoe_proc && result_is_hit( state -> result ) )
      trigger_rolling_thunder( this );

    if ( result_is_hit( state -> result ) )
      trigger_lightning_strike( state );
  }

  bool ready()
  {
    if ( p() -> specialization() == SHAMAN_ELEMENTAL && p() -> buff.ascendance -> check() )
      return false;

    return shaman_spell_t::ready();
  }
};

// Lava Beam Spell ==========================================================

struct lava_beam_t : public shaman_spell_t
{
  lava_beam_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_spell( 114074 ), options_str )
  {
    check_spec( SHAMAN_ELEMENTAL );

    base_execute_time    += p() -> spec.shamanism        -> effectN( 3 ).time_value();
    base_multiplier      += p() -> spec.shamanism        -> effectN( 2 ).percent();
    aoe                   = 5;
    base_add_multiplier   = data().effectN( 1 ).chain_multiplier();

    overload_spell        = new lava_beam_overload_t( player );
    overload_chance_multiplier = 0.3;
    add_child( overload_spell );
  }

  virtual double composite_da_multiplier()
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( p() -> buff.unleash_flame -> up() )
      m *= 1.0 + p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }

  void impact( action_state_t* state )
  {
    shaman_spell_t::impact( state );

    const shaman_action_state_t* ss = static_cast< const shaman_action_state_t* >( state );
    if ( ! ss -> eoe_proc && result_is_hit( state -> result ) )
    {
      trigger_rolling_thunder( this );
      trigger_lightning_strike( state );
    }
  }

  bool ready()
  {
    if ( ! p() -> buff.ascendance -> check() )
      return false;

    return shaman_spell_t::ready();
  }
};

// Elemental Mastery Spell ==================================================

struct elemental_mastery_t : public shaman_spell_t
{
  elemental_mastery_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "elemental_mastery", player, player -> talent.elemental_mastery, options_str )
  {
    harmful   = false;
    may_crit  = false;
    may_miss  = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.elemental_mastery -> trigger();
    if ( p() -> set_bonus.tier13_2pc_caster() )
      p() -> buff.tier13_2pc_caster -> trigger();
  }
};

// Ancestral Swiftness Spell ================================================

struct ancestral_swiftness_t : public shaman_spell_t
{
  ancestral_swiftness_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "ancestral_swiftness", player, player -> talent.ancestral_swiftness, options_str )
  {
    harmful = may_crit = may_miss = callbacks = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.ancestral_swiftness -> trigger();
  }
};

// Call of the Elements Spell ===============================================

struct call_of_the_elements_t : public shaman_spell_t
{
  call_of_the_elements_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_talent_spell( "Call of the Elements" ), options_str )
  {
    harmful   = false;
    may_crit  = false;
    may_miss  = false;
  }
};

// Fire Nova Spell ==========================================================

struct fire_nova_explosion_t : public shaman_spell_t
{
  fire_nova_explosion_t( shaman_t* player ) :
    shaman_spell_t( "fire_nova_explosion", player, player -> find_spell( 8349 ) )
  {
    check_spec( SHAMAN_ENHANCEMENT );
    aoe        = -1;
    background = true;
    dual = true;
  }

  void init()
  {
    shaman_spell_t::init();

    stats = player -> get_stats( "fire_nova" );
    eoe_stats = player -> get_stats( "fire_nova_eoe" );
  }

  void execute()
  {
    shaman_spell_t::execute();

    if ( eoe_proc )
      stats -> add_execute( time_to_execute );
  }

  double composite_da_multiplier()
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( p() -> buff.unleash_flame -> up() )
      m *= 1.0 + p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }

  // Fire nova does not damage the main target.
  size_t available_targets( std::vector< player_t* >& tl )
  {
    tl.clear();

    for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
    {
      if ( ! sim -> actor_list[ i ] -> current.sleeping &&
             sim -> actor_list[ i ] -> is_enemy() &&
             sim -> actor_list[ i ] != target )
        tl.push_back( sim -> actor_list[ i ] );
    }

    return tl.size();
  }
};

struct fire_nova_t : public shaman_spell_t
{
  fire_nova_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_specialization_spell( "Fire Nova" ), options_str )
  {
    may_proc_eoe = may_crit = may_miss = callbacks = false;
    aoe       = -1;

    impact_action = new fire_nova_explosion_t( player );
  }

  bool ready()
  {
    if ( ! td( target ) -> dot.flame_shock -> ticking )
      return false;

    return shaman_spell_t::ready();
  }

  void execute()
  {
    if ( p() -> buff.unleash_flame -> check() )
      p() -> proc.uf_fire_nova -> occur();

    shaman_spell_t::execute();
  }

  // Fire nova is emitted on all targets with a flame shock from us .. so
  std::vector< player_t* >& target_list()
  {
    target_cache.clear();

    for ( size_t i = 0; i < sim -> target_list.size(); ++i )
    {
      player_t* e = sim -> target_list[ i ];
      if ( e -> current.sleeping || ! e -> is_enemy() )
        continue;

      if ( td( e ) -> dot.flame_shock -> ticking )
        target_cache.push_back( e );
    }

    return target_cache;
  }
};

// Earthquake Spell =========================================================

struct earthquake_rumble_t : public shaman_spell_t
{
  earthquake_rumble_t( shaman_t* player ) :
    shaman_spell_t( "earthquake_rumble", player, player -> find_spell( 77478 ) )
  {
    may_proc_eoe = false;
    harmful = background = true;
    aoe = -1;
    school = SCHOOL_PHYSICAL;
  }

  void init()
  {
    shaman_spell_t::init();

    stats = player -> get_stats( "earthquake" );
    eoe_stats = player -> get_stats( "earthquake_eoe" );
  }

  virtual double composite_spell_power()
  {
    double sp = shaman_spell_t::composite_spell_power();

    sp += player -> composite_spell_power( SCHOOL_NATURE );

    return sp;
  }

  virtual double target_armor( player_t* )
  {
    return 0;
  }

  void execute()
  {
    shaman_spell_t::execute();

    if ( eoe_proc )
      stats -> add_tick( timespan_t::from_seconds( 1.0 ) );
  }
};

struct earthquake_t : public shaman_spell_t
{
  earthquake_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Earthquake" ), options_str )
  {
    hasted_ticks = may_miss = may_crit = may_proc_eoe = false;

    base_td = base_dd_min = base_dd_max = direct_power_mod = 0;
    harmful = true;
    num_ticks = ( int ) data().duration().total_seconds();
    base_tick_time = data().effectN( 2 ).period();

    dynamic_tick_action = true;
    tick_action = new earthquake_rumble_t( player );
  }
};

// Lava Burst Spell =========================================================

struct lava_burst_t : public shaman_spell_t
{
  lava_burst_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Lava Burst" ), options_str )
  {
    stormlash_da_multiplier = 2.0;
    overload_spell          = new lava_burst_overload_t( player );
    add_child( overload_spell );
  }

  virtual double composite_da_multiplier()
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( p() -> buff.unleash_flame -> up() )
      m *= 1.0 + p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }

  virtual double composite_hit()
  {
    double m = shaman_spell_t::composite_hit();

    if ( p() -> specialization() == SHAMAN_RESTORATION )
      m += p() -> spec.spiritual_insight -> effectN( 3 ).percent();

    return m;
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    double m = shaman_spell_t::composite_target_multiplier( target );

    if ( td( target ) -> debuff.unleashed_fury -> up() )
      m *= 1.0 + td( target ) -> debuff.unleashed_fury -> data().effectN( 2 ).percent();

    if ( td( target ) -> dot.flame_shock -> ticking )
      m *= 1.0 + p() -> spell.flame_shock -> effectN( 3 ).percent();

    return m;
  }

  virtual double composite_target_crit( player_t* )
  { return 1.0; }

  virtual void execute()
  {
    if ( p() -> buff.unleash_flame -> check() )
      p() -> proc.uf_lava_burst -> occur();

    shaman_spell_t::execute();

    if ( p() -> buff.lava_surge -> check() )
      p() -> buff.lava_surge -> expire();

    // FIXME: DBC Value modified in dbc_t::apply_hotfixes()
    if ( p() -> set_bonus.tier15_4pc_caster() )
      p() -> cooldown.ascendance -> ready -= p() -> sets -> set( SET_T15_4PC_CASTER ) -> effectN( 1 ).time_value();
  }

  virtual timespan_t execute_time()
  {
    if ( p() -> buff.lava_surge -> up() )
      return timespan_t::zero();

    return shaman_spell_t::execute_time();
  }
};

// Lightning Bolt Spell =====================================================

struct lightning_bolt_t : public shaman_spell_t
{
  lightning_bolt_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Lightning Bolt" ), options_str )
  {
    stormlash_da_multiplier = 2.0;
    base_multiplier   += player -> spec.shamanism -> effectN( 1 ).percent();
    base_execute_time += player -> spec.shamanism -> effectN( 3 ).time_value();
    base_execute_time *= 1.0 + player -> glyph.unleashed_lightning -> effectN( 2 ).percent();
    overload_spell     = new lightning_bolt_overload_t( player );
    add_child( overload_spell );
  }

  virtual double composite_da_multiplier()
  {
    double m = shaman_spell_t::composite_da_multiplier();

    m *= 1.0 + p() -> sets -> set( SET_T14_2PC_CASTER ) -> effectN( 1 ).percent();

    return m;
  }

  virtual double composite_hit()
  {
    double m = shaman_spell_t::composite_hit();

    if ( p() -> specialization() == SHAMAN_RESTORATION )
      m += p() -> spec.spiritual_insight -> effectN( 3 ).percent();

    return m;
  }

  virtual double composite_target_crit( player_t* target )
  {
    double c = spell_t::composite_target_crit( target );

    if ( td( target ) -> debuff.stormstrike -> up() )
    {
      c += td( target ) -> debuff.stormstrike -> data().effectN( 1 ).percent();
      c += player -> sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
    }

    return c;
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    double m = shaman_spell_t::composite_target_multiplier( target );

    if ( td( target ) -> debuff.unleashed_fury -> up() )
      m *= 1.0 + td( target ) -> debuff.unleashed_fury -> data().effectN( 1 ).percent();

    return m;
  }

  virtual void impact( action_state_t* state )
  {
    shaman_spell_t::impact( state );

    const shaman_action_state_t* ss = debug_cast< const shaman_action_state_t* >( state );
    if ( ! ss -> eoe_proc && result_is_hit( state -> result ) )
    {
      trigger_rolling_thunder( this );

      if ( p() -> glyph.telluric_currents -> ok() )
      {
        double mana_gain = p() -> glyph.telluric_currents -> effectN( 2 ).percent();
        p() -> resource_gain( RESOURCE_MANA,
                              p() -> resources.base[RESOURCE_MANA] * mana_gain,
                              p() -> gain.telluric_currents );
      }
    }

    if ( result_is_hit( state -> result ) )
      trigger_lightning_strike( state );
  }

  virtual bool usable_moving()
  {
    if ( p() -> glyph.unleashed_lightning -> ok() )
      return true;

    return shaman_spell_t::usable_moving();
  }
};

// Elemental Blast Spell ====================================================

struct elemental_blast_t : public shaman_spell_t
{
  rng_t* buff_rng;

  elemental_blast_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "elemental_blast", player, player -> talent.elemental_blast, options_str ),
    buff_rng( 0 )
  {
    may_proc_eoe = true;
    overload_spell = new elemental_blast_overload_t( player );
    add_child( overload_spell );

    if ( p() -> specialization() == SHAMAN_ENHANCEMENT )
      eoe_proc_chance = 0.06;

    buff_rng = player -> get_rng( "elemental_blast_rng" );
  }

  void schedule_travel( action_state_t* s )
  {
    // Aaaaaaaaaaaaand tweak the state, so we get a proper execute_state
    snapshot_state( s, snapshot_flags, DMG_DIRECT );
    if ( sim -> debug )
      s -> debug();

    shaman_spell_t::schedule_travel( s );
  }

  virtual void execute()
  {
    if ( p() -> buff.unleash_flame -> check() )
      p() -> proc.uf_elemental_blast -> occur();

    shaman_spell_t::execute();
  }

  result_e calculate_result( action_state_t* s )
  {
    if ( ! s -> target ) return RESULT_NONE;

    int delta_level = s -> target -> level - player -> level;

    result_e result = RESULT_NONE;

    if ( ( result == RESULT_NONE ) && may_miss )
    {
      if ( rng_result -> roll( miss_chance( composite_hit(), delta_level ) ) )
        result = RESULT_MISS;
    }

    if ( result == RESULT_NONE )
    {
      result = RESULT_HIT;
      unsigned max_buffs = 3 + ( p() -> specialization() == SHAMAN_ENHANCEMENT ? 1 : 0 );

      unsigned b = static_cast< unsigned >( buff_rng -> range( 0, max_buffs ) );
      assert( b < max_buffs );

      p() -> buff.elemental_blast_agility -> expire();
      p() -> buff.elemental_blast_crit -> expire();
      p() -> buff.elemental_blast_haste -> expire();
      p() -> buff.elemental_blast_mastery -> expire();

      if ( b == 0 )
        p() -> buff.elemental_blast_crit -> trigger();
      else if ( b == 1 )
        p() -> buff.elemental_blast_haste -> trigger();
      else if ( b == 2 )
        p() -> buff.elemental_blast_mastery -> trigger();
      else
        p() -> buff.elemental_blast_agility -> trigger();

      if ( rng_result -> roll( crit_chance( composite_crit() + composite_target_crit( s -> target ), delta_level ) ) )
        result = RESULT_CRIT;
    }

    if ( sim -> debug ) sim -> output( "%s result for %s is %s", player -> name(), name(), util::result_type_string( result ) );

    return result;
  }

  double calculate_direct_damage( result_e r, int chain_target, double ap, double sp, double multiplier, player_t* t )
  {
    multiplier = composite_da_multiplier() * composite_target_da_multiplier( t );

    //sim_t::output( sim, "Multiplier %f spellpower %f", multiplier, sp );

    return shaman_spell_t::calculate_direct_damage( r, chain_target, ap, sp, multiplier, t );
  }

  virtual double composite_da_multiplier()
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( p() -> buff.unleash_flame -> up() )
      m *= 1.0 + p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }

  virtual double composite_hit()
  {
    double m = shaman_spell_t::composite_hit();

    if ( p() -> specialization() == SHAMAN_RESTORATION )
      m += p() -> spec.spiritual_insight -> effectN( 3 ).percent();

    return m;
  }
};


// Shamanistic Rage Spell ===================================================

struct shamanistic_rage_t : public shaman_spell_t
{
  shamanistic_rage_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Shamanistic Rage" ), options_str )
  {
    check_spec( SHAMAN_ENHANCEMENT );

    harmful   = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.shamanistic_rage -> trigger();
  }
};

// Spirit Wolf Spell ========================================================

struct feral_spirit_spell_t : public shaman_spell_t
{
  feral_spirit_spell_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Feral Spirit" ), options_str )
  {
    check_spec( SHAMAN_ENHANCEMENT );

    harmful   = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> pet_feral_spirit[ 0 ] -> summon( data().duration() );
    p() -> pet_feral_spirit[ 1 ] -> summon( data().duration() );
  }
};

// Thunderstorm Spell =======================================================

struct thunderstorm_t : public shaman_spell_t
{
  double bonus;

  thunderstorm_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Thunderstorm" ), options_str ), bonus( 0 )
  {
    check_spec( SHAMAN_ELEMENTAL );

    aoe = -1;
    cooldown -> duration += player -> glyph.thunder -> effectN( 1 ).time_value();
    bonus                 = data().effectN( 2 ).percent() +
                            player -> glyph.thunderstorm -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    player -> resource_gain( data().effectN( 2 ).resource_gain_type(),
                             player -> resources.max[ data().effectN( 2 ).resource_gain_type() ] * bonus,
                             p() -> gain.thunderstorm );
  }
};

// Unleash Elements Spell ===================================================

struct unleash_elements_t : public shaman_spell_t
{
  unleash_wind_t*   wind;
  unleash_flame_t* flame;

  unleash_elements_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Unleash Elements" ), options_str ),
    wind( 0 ), flame( 0 )
  {
    may_crit    = false;
    may_miss    = false;

    wind        = new unleash_wind_t( player );
    flame       = new unleash_flame_t( player );

    add_child( wind );
    add_child( flame );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    wind  -> execute();
    flame -> execute();

    // You get the buffs, regardless of hit/miss
    if ( p() -> main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
      p() -> buff.unleash_flame -> trigger();
    else if ( p() -> main_hand_weapon.buff_type == WINDFURY_IMBUE )
    {
      p() -> buff.unleash_wind -> trigger( p() -> buff.unleash_wind -> data().initial_stacks() );
      if ( p() -> talent.unleashed_fury -> ok() )
        p() -> buff.unleashed_fury_wf -> trigger();
    }

    if ( p() -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( p() -> off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
        p() -> buff.unleash_flame -> trigger();
      else if ( p() -> off_hand_weapon.buff_type == WINDFURY_IMBUE )
        p() -> buff.unleash_wind -> trigger( p() -> buff.unleash_wind -> data().initial_stacks() );
    }
  }
};

struct spiritwalkers_grace_t : public shaman_spell_t
{
  spiritwalkers_grace_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Spiritwalker's Grace" ), options_str )
  {
    may_miss = may_crit = harmful = callbacks = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.spiritwalkers_grace -> trigger();
    if ( p() -> set_bonus.tier13_4pc_heal() )
      p() -> buff.tier13_4pc_healer -> trigger();
  }
};

// ==========================================================================
// Shaman Shock Spells
// ==========================================================================

// Earth Shock Spell ========================================================

struct earth_shock_t : public shaman_spell_t
{
  struct lightning_charge_delay_t : public event_t
  {
    buff_t* buff;
    int consume_stacks;
    int consume_threshold;

    lightning_charge_delay_t( shaman_t* p, buff_t* b, int consume, int consume_threshold ) :
      event_t( p, "lightning_charge_delay_t" ), buff( b ),
      consume_stacks( consume ), consume_threshold( consume_threshold )
    {
      sim.add_event( this, timespan_t::from_seconds( 0.001 ) );
    }

    void execute()
    {
      if ( ( buff -> check() - consume_threshold ) > 0 )
        buff -> decrement( consume_stacks );
    }
  };

  int consume_threshold;

  earth_shock_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Earth Shock" ), options_str ),
    consume_threshold( ( int ) player -> spec.fulmination -> effectN( 1 ).base_value() )
  {
    cooldown             = player -> cooldown.shock;
    cooldown -> duration = data().cooldown() + player -> spec.spiritual_insight -> effectN( 3 ).time_value();
    shock = true;

    stats -> add_child ( player -> get_stats( "fulmination" ) );
  }

  double composite_target_crit( player_t* target )
  {
    double c = shaman_spell_t::composite_target_crit( target );

    if ( td( target ) -> debuff.stormstrike -> up() )
    {
      c += td( target ) -> debuff.stormstrike -> data().effectN( 1 ).percent();
      c += player -> sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
    }

    return c;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    if ( consume_threshold == 0 || eoe_proc )
      return;

    if ( result_is_hit( execute_state -> result ) )
    {
      int consuming_stacks = p() -> buff.lightning_shield -> stack() - consume_threshold;
      if ( consuming_stacks > 0 )
      {
        p() -> active_lightning_charge -> execute();
        new ( *p() -> sim ) lightning_charge_delay_t( p(), p() -> buff.lightning_shield, consuming_stacks, consume_threshold );
        p() -> proc.fulmination[ consuming_stacks ] -> occur();
      }
    }
  }

  virtual void impact( action_state_t* state )
  {
    shaman_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( ! sim -> overrides.weakened_blows )
        state -> target -> debuffs.weakened_blows -> trigger();
    }
  }
};

// Flame Shock Spell ========================================================

struct flame_shock_t : public shaman_spell_t
{
  flame_shock_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Flame Shock" ), options_str )
  {
    tick_may_crit         = true;
    dot_behavior          = DOT_REFRESH;
    cooldown              = player -> cooldown.shock;
    cooldown -> duration = data().cooldown() + player -> spec.spiritual_insight -> effectN( 3 ).time_value();
    shock = true;
  }

  double composite_da_multiplier()
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( p() -> buff.unleash_flame -> up() )
      m *= 1.0 + p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }

  double composite_ta_multiplier()
  {
    double m = shaman_spell_t::composite_ta_multiplier();

    if ( p() -> buff.unleash_flame -> up() )
      m *= 1.0 + p() -> buff.unleash_flame -> data().effectN( 3 ).percent();

    return m;
  }

  virtual double composite_hit()
  {
    double m = shaman_spell_t::composite_hit();

    if ( p() -> specialization() == SHAMAN_RESTORATION )
      m += p() -> spec.spiritual_insight -> effectN( 3 ).percent();

    return m;
  }

  void execute()
  {
    if ( ! td( target ) -> dot.flame_shock -> ticking )
      p() -> active_flame_shocks++;

    if ( p() -> buff.unleash_flame -> check() )
      p() -> proc.uf_flame_shock -> occur();

    shaman_spell_t::execute();
  }

  virtual void tick( dot_t* d )
  {
    shaman_spell_t::tick( d );

    if ( p() -> rng.lava_surge -> roll ( p() -> spec.lava_surge -> proc_chance() ) )
    {
      p() -> proc.lava_surge -> occur();
      p() -> cooldown.lava_burst -> reset( true );
    }
  }

  void last_tick( dot_t* d )
  {
    shaman_spell_t::last_tick( d );

    p() -> active_flame_shocks--;
  }
};

// Frost Shock Spell ========================================================

struct frost_shock_t : public shaman_spell_t
{
  frost_shock_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Frost Shock" ), options_str )
  {
    cooldown             = player -> cooldown.shock;
    cooldown -> duration = data().cooldown() + player -> spec.spiritual_insight -> effectN( 3 ).time_value();
    shock = true;
  }
};

// Wind Shear Spell =========================================================

struct wind_shear_t : public shaman_spell_t
{
  wind_shear_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Wind Shear" ), options_str )
  {
    may_miss = may_crit = false;
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() ) return false;
    return shaman_spell_t::ready();
  }
};

// Ascendancy Spell =========================================================

struct ascendance_t : public shaman_spell_t
{
  cooldown_t* strike_cd;

  ascendance_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Ascendance" ), options_str )
  {
    harmful = false;

    strike_cd = p() -> cooldown.strike;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    strike_cd -> reset( false );

    p() -> buff.ascendance -> trigger();
  }
};

// Healing Surge Spell ======================================================

struct healing_surge_t : public shaman_heal_t
{
  healing_surge_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_class_spell( "Healing Surge" ), options_str )
  {
    resurgence_gain = 0.6 * p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }

  double composite_crit()
  {
    double c = shaman_heal_t::composite_crit();

    if ( p() -> buff.tidal_waves -> up() )
    {
      c += p() -> spec.tidal_waves -> effectN( 1 ).percent() +
           p() -> sets -> set( SET_T14_4PC_HEAL ) -> effectN( 1 ).percent();
    }

    return c;
  }
};

// Healing Wave Spell =======================================================

struct healing_wave_t : public shaman_heal_t
{
  healing_wave_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Healing Wave" ), options_str )
  {
    resurgence_gain = p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }

  timespan_t execute_time()
  {
    timespan_t c = shaman_heal_t::execute_time();

    if ( p() -> buff.tidal_waves -> up() )
    {
      c *= 1.0 - ( p() -> spec.tidal_waves -> effectN( 1 ).percent() +
                   p() -> sets -> set( SET_T14_4PC_HEAL ) -> effectN( 1 ).percent() );
    }

    return c;
  }
};

// Greater Healing Wave Spell ===============================================

struct greater_healing_wave_t : public shaman_heal_t
{
  greater_healing_wave_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Greater Healing Wave" ), options_str )
  {
    resurgence_gain = p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }

  timespan_t execute_time()
  {
    timespan_t c = shaman_heal_t::execute_time();

    if ( p() -> buff.tidal_waves -> up() )
    {
      c *= 1.0 - ( p() -> spec.tidal_waves -> effectN( 1 ).percent() +
                   p() -> sets -> set( SET_T14_4PC_HEAL ) -> effectN( 1 ).percent() );
    }

    return c;
  }
};

// Riptide Spell ============================================================

struct riptide_t : public shaman_heal_t
{
  riptide_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Riptide" ), options_str )
  {
    resurgence_gain = 0.6 * p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }
};

// Chain Heal Spell =========================================================

struct chain_heal_t : public shaman_heal_t
{
  chain_heal_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_class_spell( "Chain Heal" ), options_str )
  {
    resurgence_gain = 0.333 * p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }

  double composite_da_multiplier()
  {
    double m = shaman_heal_t::composite_da_multiplier();

    if ( td( target ) -> heal.riptide -> ticking )
      m *= 1.0 + p() -> spec.riptide -> effectN( 3 ).percent();

    return m;
  }
};

// Healing Rain Spell =======================================================

struct healing_rain_t : public shaman_heal_t
{
  struct healing_rain_aoe_tick_t : public shaman_heal_t
  {
    healing_rain_aoe_tick_t( shaman_t* player ) :
      shaman_heal_t( "healing_rain_tick", player, player -> find_spell( 73921 ) )
    {
      background = true;
      aoe = -1;
      // Can proc Echo of the Elements?
    }
  };

  healing_rain_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_class_spell( "Healing Rain" ), options_str )
  {
    base_tick_time = data().effectN( 2 ).period();
    num_ticks = ( int ) ( data().duration() / data().effectN( 2 ).period() );
    hasted_ticks = false;
    tick_action = new healing_rain_aoe_tick_t( player );
  }
};

// ==========================================================================
// Shaman Totem System
// ==========================================================================

struct totem_active_expr_t : public expr_t
{
  pet_t& pet;
  totem_active_expr_t( pet_t& p ) :
    expr_t( "totem_active" ), pet( p ) { }
  virtual double evaluate() { return ! pet.current.sleeping; }
};

struct totem_remains_expr_t : public expr_t
{
  pet_t& pet;
  totem_remains_expr_t( pet_t& p ) :
    expr_t( "totem_remains" ), pet( p ) { }
  virtual double evaluate()
  {
    if ( pet.current.sleeping || ! pet.expiration )
      return 0.0;

    return pet.expiration -> remains().total_seconds();
  }
};

struct shaman_totem_pet_t : public pet_t
{
  totem_e               totem_type;

  // Pulse related functionality
  totem_pulse_action_t* pulse_action;
  event_t*              pulse_event;
  timespan_t            pulse_amplitude;

  // Summon related functionality
  pet_t*                summon_pet;

  shaman_totem_pet_t( shaman_t* p, const std::string& n, totem_e tt ) :
    pet_t( p -> sim, p, n, true ),
    totem_type( tt ),
    pulse_action( 0 ), pulse_event( 0 ), pulse_amplitude( timespan_t::zero() ),
    summon_pet( 0 )
  { }

  virtual void summon( timespan_t = timespan_t::zero() );
  virtual void dismiss();

  shaman_t* o()
  { return debug_cast< shaman_t* >( owner ); }

  virtual double composite_player_multiplier( school_e school, action_t* a )
  { return owner -> composite_player_multiplier( school, a ); }

  virtual double composite_spell_hit()
  { return owner -> composite_spell_hit(); }

  virtual double composite_spell_crit()
  { return owner -> composite_spell_crit(); }

  virtual double composite_spell_power( school_e school )
  { return owner -> composite_spell_power( school ); }

  virtual double composite_spell_power_multiplier()
  { return owner -> composite_spell_power_multiplier(); }

  virtual double composite_spell_speed()
  { return 1.0; }

  virtual expr_t* create_expression( action_t* a, const std::string& name )
  {
    if ( util::str_compare_ci( name, "active" ) )
      return new totem_active_expr_t( *this );
    else if ( util::str_compare_ci( name, "remains" ) )
      return new totem_remains_expr_t( *this );
    else if ( util::str_compare_ci( name, "duration" ) )
      return make_ref_expr( name, duration );

    return pet_t::create_expression( a, name );
  }
};

struct shaman_totem_t : public shaman_spell_t
{
  shaman_totem_pet_t* totem_pet;
  timespan_t totem_duration;

  shaman_totem_t( const std::string& totem_name, shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( totem_name ), options_str ),
    totem_duration( data().duration() )
  {
    totem = true;
    harmful = callbacks = may_miss = may_crit = false;
    totem_pet      = dynamic_cast< shaman_totem_pet_t* >( player -> find_pet( name() ) );
    assert( totem_pet != 0 );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    totem_pet -> summon( totem_duration );
  }

  virtual expr_t* create_expression( const std::string& name )
  {
    if ( util::str_compare_ci( name, "active" ) )
      return new totem_active_expr_t( *totem_pet );
    else if ( util::str_compare_ci( name, "remains" ) )
      return new totem_remains_expr_t( *totem_pet );
    else if ( util::str_compare_ci( name, "duration" ) )
      return make_ref_expr( name, totem_duration );

    return shaman_spell_t::create_expression( name );
  }
};

struct totem_pulse_action_t : public spell_t
{
  shaman_totem_pet_t* totem;

  totem_pulse_action_t( shaman_totem_pet_t* p, const spell_data_t* s ) :
    spell_t( "", p, s ), totem( p )
  {
    may_crit = harmful = background = true;
    callbacks = false;
    crit_bonus_multiplier *= 1.0 + totem -> o() -> spec.elemental_fury -> effectN( 1 ).percent();
  }

  void init()
  {
    spell_t::init();

    // Hacky, but constructor wont work.
    crit_multiplier *= util::crit_multiplier( totem -> o() -> meta_gem );
  }

  double composite_da_multiplier()
  {
    double m = spell_t::composite_da_multiplier();

    if ( totem -> o() -> buff.elemental_focus -> up() )
      m *= 1.0 + totem -> o() -> buff.elemental_focus -> data().effectN( 2 ).percent();

    return m;
  }
};

struct totem_pulse_event_t : public event_t
{
  shaman_totem_pet_t* totem;

  totem_pulse_event_t( shaman_totem_pet_t* t, timespan_t amplitude ) :
    event_t( t, "totem_pulse" ),
    totem( t )
  {
    sim.add_event( this, amplitude );
  }

  virtual void execute()
  {
    if ( totem -> pulse_action )
      totem -> pulse_action -> execute();

    totem -> pulse_event = new ( sim ) totem_pulse_event_t( totem, totem -> pulse_amplitude );
  }
};

void shaman_totem_pet_t::summon( timespan_t duration )
{
  if ( o() -> totems[ totem_type ] )
    o() -> totems[ totem_type ] -> dismiss();

  pet_t::summon( duration );

  o() -> totems[ totem_type ] = this;

  if ( pulse_action )
    pulse_event = new ( *sim ) totem_pulse_event_t( this, pulse_amplitude );

  if ( summon_pet )
    summon_pet -> summon();
}

void shaman_totem_pet_t::dismiss()
{
  pet_t::dismiss();
  event_t::cancel( pulse_event );

  if ( summon_pet )
    summon_pet -> dismiss();

  if ( o() -> totems[ totem_type ] == this )
    o() -> totems[ totem_type ] = 0;
}

// Earth Elemental Totem Spell ==============================================

struct earth_elemental_totem_t : public shaman_totem_pet_t
{
  earth_elemental_totem_t( shaman_t* p ) :
    shaman_totem_pet_t( p, "earth_elemental_totem", TOTEM_EARTH )
  { }

  void init_spells()
  {
    if ( o() -> talent.primal_elementalist -> ok() )
      summon_pet = o() -> find_pet( "primal_earth_elemental" );
    else
      summon_pet = o() -> find_pet( "greater_earth_elemental" );
    assert( summon_pet != 0 );

    shaman_totem_pet_t::init_spells();
  }
};

struct earth_elemental_totem_spell_t : public shaman_totem_t
{
  earth_elemental_totem_spell_t( shaman_t* player, const std::string& options_str ) :
    shaman_totem_t( "Earth Elemental Totem", player, options_str )
  { }

  void execute()
  {
    shaman_totem_t::execute();

    p() -> cooldown.elemental_totem -> start( totem_duration );
  }

  bool ready()
  {
    if ( p() -> cooldown.elemental_totem -> down() )
      return false;

    return shaman_totem_t::ready();
  }

};

// Fire Elemental Totem Spell ===============================================

struct fire_elemental_totem_t : public shaman_totem_pet_t
{
  fire_elemental_totem_t( shaman_t* p ) :
    shaman_totem_pet_t( p, "fire_elemental_totem", TOTEM_FIRE )
  { }

  void init_spells()
  {
    if ( o() -> talent.primal_elementalist -> ok() )
      summon_pet = o() -> find_pet( "primal_fire_elemental" );
    else
      summon_pet = o() -> find_pet( "greater_fire_elemental" );
    assert( summon_pet != 0 );

    shaman_totem_pet_t::init_spells();
  }
};

struct fire_elemental_totem_spell_t : public shaman_totem_t
{
  fire_elemental_totem_spell_t( shaman_t* player, const std::string& options_str ) :
    shaman_totem_t( "Fire Elemental Totem", player, options_str )
  {
    cooldown -> duration *= 1.0 + p() -> glyph.fire_elemental_totem -> effectN( 1 ).percent();
    totem_duration       *= 1.0 + p() -> glyph.fire_elemental_totem -> effectN( 2 ).percent();
  }

  void execute()
  {
    shaman_totem_t::execute();

    p() -> cooldown.elemental_totem -> start( totem_duration );
  }

  bool ready()
  {
    if ( p() -> cooldown.elemental_totem -> down() )
      return false;

    return shaman_totem_t::ready();
  }
};

// Magma Totem Spell ========================================================

struct magma_totem_pulse_t : public totem_pulse_action_t
{
  magma_totem_pulse_t( shaman_totem_pet_t* p ) :
    totem_pulse_action_t( p, p -> find_spell( 8187 ) )
  {
    aoe = -1;
  }
};

struct magma_totem_t : public shaman_totem_pet_t
{
  magma_totem_t( shaman_t* p ) :
    shaman_totem_pet_t( p, "magma_totem", TOTEM_FIRE )
  {
    pulse_amplitude = p -> dbc.spell( 8188 ) -> effectN( 1 ).period();
  }

  void init_spells()
  {
    if ( find_spell( 8187 ) -> ok() )
      pulse_action = new magma_totem_pulse_t( this );

    shaman_totem_pet_t::init_spells();
  }
};

// Searing Totem Spell ======================================================

struct searing_totem_pulse_t : public totem_pulse_action_t
{
  searing_totem_pulse_t( shaman_totem_pet_t* p ) :
    totem_pulse_action_t( p, p -> find_spell( 3606 ) )
  {
    base_execute_time = timespan_t::zero();
    // 15851 dbc base min/max damage is approx. 10 points too low, compared
    // to in-game damage ranges
    base_dd_min += 10;
    base_dd_max += 10;
  }

  void impact( action_state_t* state )
  {
    totem_pulse_action_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( totem -> o() -> spec.searing_flames -> ok() )
        totem -> o() -> buff.searing_flames -> trigger();
    }
  }
};

struct searing_totem_t : public shaman_totem_pet_t
{
  searing_totem_t( shaman_t* p ) :
    shaman_totem_pet_t( p, "searing_totem", TOTEM_FIRE )
  {
    pulse_amplitude = timespan_t::from_seconds( 1.6 );
  }

  void init_spells()
  {
    if ( find_spell( 3606 ) -> ok() )
      pulse_action = new searing_totem_pulse_t( this );

    shaman_totem_pet_t::init_spells();
  }
};

// Storm Lash Totem =========================================================

struct stormlash_totem_t : public shaman_totem_pet_t
{
  struct stormlash_aggregate_t : public spell_t
  {
    stormlash_aggregate_t( player_t* p ) :
      spell_t( "stormlash_aggregate", p, spell_data_t::nil() )
    {
      background = true;
      callbacks = false;
    }
  };

  stormlash_aggregate_t* aggregate;

  stormlash_totem_t( shaman_t* p ) :
    shaman_totem_pet_t( p, "stormlash_totem", TOTEM_AIR ),
    aggregate( 0 )
  { }

  void init_spell()
  {
    shaman_totem_pet_t::init_spell();

    aggregate = new stormlash_aggregate_t( this );
  }

  void arise()
  {
    shaman_totem_pet_t::arise();

    for ( size_t i = 0; i < sim -> player_list.size(); ++i )
    {
      player_t* p = sim -> player_list[ i ];
      if ( p -> type == PLAYER_GUARDIAN )
        continue;

      stormlash_buff_t* sb = static_cast< stormlash_buff_t* >( p -> buffs.stormlash );
      if ( ! sb -> stormlash_aggregate && o() -> aggregate_stormlash )
        sb -> stormlash_aggregate = aggregate;
      sb -> trigger();
    }
  }

  void demise()
  {
    shaman_totem_pet_t::demise();

    for ( size_t i = 0; i < sim -> player_list.size(); ++i )
    {
      player_t* p = sim -> player_list[ i ];
      if ( p -> type == PLAYER_GUARDIAN )
        continue;

      stormlash_buff_t* sb = static_cast< stormlash_buff_t* >( p -> buffs.stormlash );
      sb -> expire();
      if ( sb -> stormlash_aggregate == aggregate )
        sb -> stormlash_aggregate = 0;
    }
  }
};

struct stormlash_totem_spell_t : public shaman_totem_t
{
  stormlash_totem_spell_t( shaman_t* player, const std::string& options_str ) :
    shaman_totem_t( "Stormlash Totem", player, options_str )
  { }

  bool ready()
  {
    if ( sim -> overrides.stormlash > 0 )
      return false;

    return shaman_totem_t::ready();
  }
};

// ==========================================================================
// Shaman Weapon Imbues
// ==========================================================================

// Flametongue Weapon Spell =================================================

struct flametongue_weapon_t : public shaman_spell_t
{
  double bonus_power;
  int    main, off;

  flametongue_weapon_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Flametongue Weapon" ) ),
    bonus_power( 0 ), main( 0 ), off( 0 )
  {
    std::string weapon_str;

    option_t options[] =
    {
      opt_string( "weapon", weapon_str ),
      opt_null()
    };
    parse_options( options, options_str );

    if ( weapon_str.empty() )
    {
      main = off = 1;
    }
    else if ( weapon_str == "main" )
    {
      main = 1;
    }
    else if ( weapon_str == "off" )
    {
      off = 1;
    }
    else if ( weapon_str == "both" )
    {
      main = 1;
      off = 1;
    }
    else
    {
      sim -> errorf( "Player %s: flametongue_weapon: weapon option must be one of main/off/both\n", p() -> name() );
      sim -> cancel();
    }

    // Spell damage scaling is defined in "Flametongue Weapon (Passive), id 10400"
    bonus_power  = player -> dbc.spell( 10400 ) -> effectN( 2 ).percent();
    harmful      = false;
    may_miss     = false;

    if ( main )
      player -> flametongue_mh = new flametongue_weapon_spell_t( "flametongue_mh", player, &( player -> main_hand_weapon ) );

    if ( off )
      player -> flametongue_oh = new flametongue_weapon_spell_t( "flametongue_oh", player, &( player -> off_hand_weapon ) );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    if ( main )
    {
      p() -> main_hand_weapon.buff_type  = FLAMETONGUE_IMBUE;
      p() -> main_hand_weapon.buff_value = bonus_power;
    }
    if ( off )
    {
      p() -> off_hand_weapon.buff_type  = FLAMETONGUE_IMBUE;
      p() -> off_hand_weapon.buff_value = bonus_power;
    }
  };

  virtual bool ready()
  {
    if ( main && ( p() -> main_hand_weapon.buff_type != FLAMETONGUE_IMBUE ) )
      return true;

    if ( off && ( p() -> off_hand_weapon.buff_type != FLAMETONGUE_IMBUE ) )
      return true;

    return false;
  }
};

// Windfury Weapon Spell ====================================================

struct windfury_weapon_t : public shaman_spell_t
{
  double bonus_power;
  int    main, off;

  windfury_weapon_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Windfury Weapon" ) ),
    bonus_power( 0 ), main( 0 ), off( 0 )
  {
    check_spec( SHAMAN_ENHANCEMENT );
    std::string weapon_str;

    option_t options[] =
    {
      opt_string( "weapon", weapon_str ),
      opt_null()
    };
    parse_options( options, options_str );

    if ( weapon_str.empty() )
    {
      main = off = 1;
    }
    else if ( weapon_str == "main" )
    {
      main = 1;
    }
    else if ( weapon_str == "off" )
    {
      off = 1;
    }
    else if ( weapon_str == "both" )
    {
      main = 1;
      off = 1;
    }
    else
    {
      sim -> errorf( "Player %s: windfury_weapon: weapon option must be one of main/off/both\n", p() -> name() );
      sim -> cancel();
    }

    bonus_power  = data().effectN( 2 ).average( player );
    harmful      = false;
    may_miss     = false;

    if ( main )
      player -> windfury_mh = new windfury_weapon_melee_attack_t( "windfury_mh", player, &( player -> main_hand_weapon ) );

    if ( off )
      player -> windfury_oh = new windfury_weapon_melee_attack_t( "windfury_oh", player, &( player -> off_hand_weapon ) );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    if ( main )
    {
      p() -> main_hand_weapon.buff_type  = WINDFURY_IMBUE;
      p() -> main_hand_weapon.buff_value = bonus_power;
    }
    if ( off )
    {
      p() -> off_hand_weapon.buff_type  = WINDFURY_IMBUE;
      p() -> off_hand_weapon.buff_value = bonus_power;
    }
  };

  virtual bool ready()
  {
    if ( main && ( p() -> main_hand_weapon.buff_type != WINDFURY_IMBUE ) )
      return true;

    if ( off && ( p() -> off_hand_weapon.buff_type != WINDFURY_IMBUE ) )
      return true;

    return false;
  }
};

struct earthliving_weapon_t : public shaman_spell_t
{
  double bonus_power;

  earthliving_weapon_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_specialization_spell( "Earthliving Weapon" ) ),
    bonus_power( 0 )
  {
    check_spec( SHAMAN_RESTORATION );
    parse_options( 0, options_str );

    bonus_power  = player -> find_spell( 52007 ) -> effectN( 2 ).average( player );
    harmful      = false;
    may_miss     = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> main_hand_weapon.buff_type  = EARTHLIVING_IMBUE;
    p() -> main_hand_weapon.buff_value = bonus_power;
  };

  virtual bool ready()
  {
    if ( p() -> main_hand_weapon.buff_type != EARTHLIVING_IMBUE )
      return true;

    return false;
  }
};

// ==========================================================================
// Shaman Shields
// ==========================================================================

// Lightning Shield Spell ===================================================

struct lightning_shield_t : public shaman_spell_t
{
  lightning_shield_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Lightning Shield" ), options_str )
  {
    harmful = false;

    player -> active_lightning_charge = new lightning_charge_t( player -> specialization() == SHAMAN_ELEMENTAL ? "fulmination" : "lightning_shield", player );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.water_shield     -> expire();
    p() -> buff.lightning_shield -> trigger( data().initial_stacks() );
  }
};

// Water Shield Spell =======================================================

struct water_shield_t : public shaman_spell_t
{
  double bonus;

  water_shield_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Water Shield" ), options_str ),
    bonus( 0.0 )
  {
    harmful      = false;
    bonus        = data().effectN( 2 ).average( player );
    bonus       *= 1.0 + player -> glyph.water_shield -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.lightning_shield -> expire();
    p() -> buff.water_shield -> trigger( data().initial_stacks(), bonus );
  }
};

// ==========================================================================
// Shaman Passive Buffs
// ==========================================================================

struct unleash_flame_buff_t : public buff_t
{
  struct unleash_flame_expiration_delay_t : public event_t
  {
    unleash_flame_buff_t* buff;

    unleash_flame_expiration_delay_t( shaman_t* player, unleash_flame_buff_t* b ) :
      event_t( player, "unleash_flame_expiration_delay" ), buff( b )
    {
      sim.add_event( this, sim.gauss( player -> uf_expiration_delay, player -> uf_expiration_delay_stddev ) );
    }

    virtual void execute()
    {
      // Call real expire after a delay
      buff -> buff_t::expire();
      buff -> expiration_delay = 0;
    }
  };

  event_t* expiration_delay;

  unleash_flame_buff_t( shaman_t* p ) :
    buff_t( buff_creator_t( p, 73683, "unleash_flame" ) ),
    expiration_delay( 0 )
  {}

  void reset()
  {
    buff_t::reset();
    event_t::cancel( expiration_delay );
  }

  void expire_override()
  {
    if ( current_stack == 1 && ! expiration && ! expiration_delay && ! player -> current.sleeping )
    {
      shaman_t* p = debug_cast< shaman_t* >( player );
      p -> proc.uf_wasted -> occur();
    }

    // Active player's Unleash Flame buff has a short aura expiration delay, which allows
    // "Double Casting" with a single buff
    if ( ! player -> current.sleeping )
    {
      if ( current_stack <= 0 ) return;
      if ( expiration_delay ) return;
      // If there's an expiration event going on, we are prematurely canceling the buff, so delay expiration
      if ( expiration )
        expiration_delay = new ( *sim ) unleash_flame_expiration_delay_t( debug_cast< shaman_t* >( player ), this );
      else
        buff_t::expire_override();
    }
    // If the p() is sleeping, make sure the existing buff behavior works (i.e., a call to
    // expire) and additionally, make _absolutely sure_ that any pending expiration delay
    // is canceled
    else
    {
      buff_t::expire_override();
      event_t::cancel( expiration_delay );
    }
  }
};

class ascendance_buff_t : public buff_t
{
  action_t* lava_burst;

  void ascendance( attack_t* mh, attack_t* oh, timespan_t lvb_cooldown )
  {
    // Presume that ascendance trigger and expiration will not reset the swing
    // timer, so we need to cancel and reschedule autoattack with the
    // remaining swing time of main/off hands
    if ( player -> specialization() == SHAMAN_ENHANCEMENT )
    {
      bool executing = false;
      timespan_t time_to_hit = timespan_t::zero();
      if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
      {
        executing = true;
        time_to_hit = player -> main_hand_attack -> execute_event -> remains();
#ifndef NDEBUG
        if ( time_to_hit < timespan_t::zero() )
        {
          sim -> output( "Ascendance %s time_to_hit=%f", player -> main_hand_attack -> name(), time_to_hit.total_seconds() );
          assert( 0 );
        }
#endif
        event_t::cancel( player -> main_hand_attack -> execute_event );
      }

      player -> main_hand_attack = mh;
      if ( executing )
      {
        // Kick off the new main hand attack, by instantly scheduling
        // and rescheduling it to the remaining time to hit. We cannot use
        // normal reschedule mechanism here (i.e., simply use
        // event_t::reschedule() and leave it be), because the rescheduled
        // event would be triggered before the full swing time (of the new
        // auto attack) in most cases.
        player -> main_hand_attack -> base_execute_time = timespan_t::zero();
        player -> main_hand_attack -> schedule_execute();
        player -> main_hand_attack -> base_execute_time = player -> main_hand_attack -> weapon -> swing_time;
        player -> main_hand_attack -> execute_event -> reschedule( time_to_hit );
      }

      if ( player -> off_hand_attack )
      {
        time_to_hit = timespan_t::zero();
        executing = false;

        if ( player -> off_hand_attack -> execute_event )
        {
          executing = true;
          time_to_hit = player -> off_hand_attack -> execute_event -> remains();
#ifndef NDEBUG
          if ( time_to_hit < timespan_t::zero() )
          {
            sim -> output( "Ascendance %s time_to_hit=%f", player -> off_hand_attack -> name(), time_to_hit.total_seconds() );
            assert( 0 );
          }
#endif
          event_t::cancel( player -> off_hand_attack -> execute_event );
        }

        player -> off_hand_attack = oh;
        if ( executing )
        {
          // Kick off the new off hand attack, by instantly scheduling
          // and rescheduling it to the remaining time to hit. We cannot use
          // normal reschedule mechanism here (i.e., simply use
          // event_t::reschedule() and leave it be), because the rescheduled
          // event would be triggered before the full swing time (of the new
          // auto attack) in most cases.
          player -> off_hand_attack -> base_execute_time = timespan_t::zero();
          player -> off_hand_attack -> schedule_execute();
          player -> off_hand_attack -> base_execute_time = player -> off_hand_attack -> weapon -> swing_time;
          player -> off_hand_attack -> execute_event -> reschedule( time_to_hit );
        }
      }
    }
    // Elemental simply changes the Lava Burst cooldown, Lava Beam replacement
    // will be handled by action list and ready() in Chain Lightning / Lava
    // Beam
    else if ( player -> specialization() == SHAMAN_ELEMENTAL )
    {
      if ( lava_burst )
      {
        lava_burst -> cooldown -> duration = lvb_cooldown;
        lava_burst -> cooldown -> reset( false );
      }
    }
  }

public:
  ascendance_buff_t( shaman_t* p ) :
    buff_t( buff_creator_t( p, 114051, "ascendance" ) ),
    lava_burst( 0 )
  {}

  bool trigger( int stacks, double value, double chance, timespan_t duration )
  {
    shaman_t* p = debug_cast< shaman_t* >( player );

    if ( player -> specialization() == SHAMAN_ELEMENTAL && ! lava_burst )
    {
      lava_burst = player -> find_action( "lava_burst" );
    }

    ascendance( p -> ascendance_mh, p -> ascendance_oh, timespan_t::zero() );
    return buff_t::trigger( stacks, value, chance, duration );
  }

  void expire_override()
  {
    shaman_t* p = debug_cast< shaman_t* >( player );

    ascendance( p -> melee_mh, p -> melee_oh, lava_burst ? lava_burst -> data().cooldown() : timespan_t::zero() );
    buff_t::expire_override();
  }
};

// ==========================================================================
// Shaman Character Definition
// ==========================================================================

// shaman_t::create_options =================================================

void shaman_t::create_options()
{
  player_t::create_options();

  option_t shaman_options[] =
  {
    opt_timespan( "wf_delay",                   wf_delay                   ),
    opt_timespan( "wf_delay_stddev",            wf_delay_stddev            ),
    opt_timespan( "uf_expiration_delay",        uf_expiration_delay        ),
    opt_timespan( "uf_expiration_delay_stddev", uf_expiration_delay_stddev ),
    opt_float( "eoe_proc_chance",               eoe_proc_chance ),
    opt_bool( "aggregate_stormlash",            aggregate_stormlash ),
    opt_null()
  };

  option_t::copy( options, shaman_options );
}

// shaman_t::create_action  =================================================

action_t* shaman_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "ancestral_swiftness"     ) return new      ancestral_swiftness_t( this, options_str );
  if ( name == "ascendance"              ) return new               ascendance_t( this, options_str );
  if ( name == "auto_attack"             ) return new              auto_attack_t( this, options_str );
  if ( name == "bloodlust"               ) return new                bloodlust_t( this, options_str );
  if ( name == "call_of_the_elements"    ) return new     call_of_the_elements_t( this, options_str );
  if ( name == "chain_lightning"         ) return new          chain_lightning_t( this, options_str );
  if ( name == "earth_shock"             ) return new              earth_shock_t( this, options_str );
  if ( name == "earthliving_weapon"      ) return new       earthliving_weapon_t( this, options_str );
  if ( name == "earthquake"              ) return new               earthquake_t( this, options_str );
  if ( name == "elemental_blast"         ) return new          elemental_blast_t( this, options_str );
  if ( name == "elemental_mastery"       ) return new        elemental_mastery_t( this, options_str );
  if ( name == "fire_nova"               ) return new                fire_nova_t( this, options_str );
  if ( name == "flame_shock"             ) return new              flame_shock_t( this, options_str );
  if ( name == "flametongue_weapon"      ) return new       flametongue_weapon_t( this, options_str );
  if ( name == "frost_shock"             ) return new              frost_shock_t( this, options_str );
  if ( name == "lava_beam"               ) return new                lava_beam_t( this, options_str );
  if ( name == "lava_burst"              ) return new               lava_burst_t( this, options_str );
  if ( name == "lava_lash"               ) return new                lava_lash_t( this, options_str );
  if ( name == "lightning_bolt"          ) return new           lightning_bolt_t( this, options_str );
  if ( name == "lightning_shield"        ) return new         lightning_shield_t( this, options_str );
  if ( name == "primal_strike"           ) return new            primal_strike_t( this, options_str );
  if ( name == "shamanistic_rage"        ) return new         shamanistic_rage_t( this, options_str );
  if ( name == "stormblast"              ) return new               stormblast_t( this, options_str );
  if ( name == "feral_spirit"            ) return new       feral_spirit_spell_t( this, options_str );
  if ( name == "spiritwalkers_grace"     ) return new      spiritwalkers_grace_t( this, options_str );
  if ( name == "stormstrike"             ) return new              stormstrike_t( this, options_str );
  if ( name == "thunderstorm"            ) return new             thunderstorm_t( this, options_str );
  if ( name == "unleash_elements"        ) return new         unleash_elements_t( this, options_str );
  if ( name == "water_shield"            ) return new             water_shield_t( this, options_str );
  if ( name == "wind_shear"              ) return new               wind_shear_t( this, options_str );
  if ( name == "windfury_weapon"         ) return new          windfury_weapon_t( this, options_str );

  if ( name == "chain_heal"              ) return new               chain_heal_t( this, options_str );
  if ( name == "greater_healing_wave"    ) return new     greater_healing_wave_t( this, options_str );
  if ( name == "healing_rain"            ) return new             healing_rain_t( this, options_str );
  if ( name == "healing_surge"           ) return new            healing_surge_t( this, options_str );
  if ( name == "healing_wave"            ) return new             healing_wave_t( this, options_str );
  if ( name == "riptide"                 ) return new                  riptide_t( this, options_str );

  if ( name == "earth_elemental_totem"   ) return new earth_elemental_totem_spell_t( this, options_str );
  if ( name == "fire_elemental_totem"    ) return new  fire_elemental_totem_spell_t( this, options_str );
  if ( name == "magma_totem"             ) return new                shaman_totem_t( "Magma Totem", this, options_str );
  if ( name == "searing_totem"           ) return new                shaman_totem_t( "Searing Totem", this, options_str );
  if ( name == "stormlash_totem"         ) return new       stormlash_totem_spell_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// shaman_t::create_pet =====================================================

pet_t* shaman_t::create_pet( const std::string& pet_name,
                             const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "feral_spirit"             ) return new feral_spirit_pet_t( sim, this );
  if ( pet_name == "fire_elemental_pet"       ) return new fire_elemental_t( sim, this, false );
  if ( pet_name == "fire_elemental_guardian"  ) return new fire_elemental_t( sim, this, true );
  if ( pet_name == "earth_elemental_pet"      ) return new earth_elemental_pet_t( sim, this, false );
  if ( pet_name == "earth_elemental_guardian" ) return new earth_elemental_pet_t( sim, this, true );

  if ( pet_name == "earth_elemental_totem"   ) return new earth_elemental_totem_t( this );
  if ( pet_name == "fire_elemental_totem"    ) return new fire_elemental_totem_t( this );
  if ( pet_name == "magma_totem"             ) return new magma_totem_t( this );
  if ( pet_name == "searing_totem"           ) return new searing_totem_t( this );
  if ( pet_name == "stormlash_totem"         ) return new stormlash_totem_t( this );

  return 0;
}

// shaman_t::create_pets ====================================================

void shaman_t::create_pets()
{
  pet_feral_spirit[ 0 ]    = create_pet( "feral_spirit"             );
  pet_feral_spirit[ 1 ]    = create_pet( "feral_spirit"             );
  pet_fire_elemental       = create_pet( "fire_elemental_pet"       );
  guardian_fire_elemental  = create_pet( "fire_elemental_guardian"  );
  pet_earth_elemental      = create_pet( "earth_elemental_pet"      );
  guardian_earth_elemental = create_pet( "earth_elemental_guardian" );

  create_pet( "earth_elemental_totem" );
  create_pet( "fire_elemental_totem"  );
  create_pet( "magma_totem"           );
  create_pet( "searing_totem"         );
  create_pet( "stormlash_totem"       );
}

// shaman_t::create_expression ==============================================

expr_t* shaman_t::create_expression( action_t* a, const std::string& name )
{

  std::vector<std::string> splits = util::string_split( name, "." );

  // totem.<kind>.<op>
  if ( splits.size() == 3 && util::str_compare_ci( splits[ 0 ], "totem" ) )
  {
    totem_e totem_type = TOTEM_NONE;
    shaman_totem_pet_t* totem = 0;

    if ( util::str_compare_ci( splits[ 1 ], "earth" ) )
      totem_type = TOTEM_EARTH;
    else if ( util::str_compare_ci( splits[ 1 ], "fire" ) )
      totem_type = TOTEM_FIRE;
    else if ( util::str_compare_ci( splits[ 1 ], "water" ) )
      totem_type = TOTEM_WATER;
    else if ( util::str_compare_ci( splits[ 1 ], "air" ) )
      totem_type = TOTEM_AIR;
    // None of the elements, then a specific totem name?
    else
      totem = static_cast< shaman_totem_pet_t* >( find_pet( splits[ 1 ] ) );

    // Nothing found
    if ( totem_type == TOTEM_NONE && totem == 0 )
      return player_t::create_expression( a, name );
    // A specific totem name given, and found
    else if ( totem != 0 )
      return totem -> create_expression( a, splits[ 2 ] );

    // Otherwise generic totem school, make custom expressions
    if ( util::str_compare_ci( splits[ 2 ], "active" ) )
    {
      struct totem_active_expr_t : public expr_t
      {
        shaman_t& p;
        totem_e totem_type;

        totem_active_expr_t( shaman_t& p, totem_e tt ) :
          expr_t( "totem_active" ), p( p ), totem_type( tt )
        { }

        virtual double evaluate()
        {
          return p.totems[ totem_type ] && ! p.totems[ totem_type ] -> current.sleeping;
        }
      };
      return new totem_active_expr_t( *this, totem_type );
    }
  }

  if ( util::str_compare_ci( name, "active_flame_shock" ) )
  {
    return make_ref_expr( name, active_flame_shocks );
  }

  return player_t::create_expression( a, name );
}

// shaman_t::init_spells ====================================================

void shaman_t::init_spells()
{
  // New set bonus system
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //   C2P     C4P     M2P     M4P    T2P    T4P     H2P     H4P
    { 105780, 105816, 105866, 105872,     0,     0, 105764, 105876 }, // Tier13
    { 123123, 123124, 123132, 123133,     0,     0, 123134, 123135 }, // Tier14
    { 138145, 138144, 138136, 138141,     0,     0, 138303, 138305 }, // Tier15
  };
  sets = new set_bonus_array_t( this, set_bonuses );

  // Generic
  spec.mail_specialization = find_specialization_spell( "Mail Specialization" );
  constant.matching_gear_multiplier = spec.mail_specialization -> effectN( 1 ).percent();

  // Elemental / Restoration
  spec.spiritual_insight   = find_specialization_spell( "Spiritual Insight" );

  // Elemental
  spec.elemental_focus     = find_specialization_spell( "Elemental Focus" );
  spec.elemental_fury      = find_specialization_spell( "Elemental Fury" );
  spec.elemental_precision = find_specialization_spell( "Elemental Precision" );
  spec.fulmination         = find_specialization_spell( "Fulmination" );
  spec.lava_surge          = find_specialization_spell( "Lava Surge" );
  spec.rolling_thunder     = find_specialization_spell( "Rolling Thunder" );
  spec.shamanism           = find_specialization_spell( "Shamanism" );

  // Enhancement
  spec.dual_wield          = find_specialization_spell( "Dual Wield" );
  spec.flurry              = find_specialization_spell( "Flurry" );
  spec.maelstrom_weapon    = find_specialization_spell( "Maelstrom Weapon" );
  spec.mental_quickness    = find_specialization_spell( "Mental Quickness" );
  spec.primal_wisdom       = find_specialization_spell( "Primal Wisdom" );
  spec.searing_flames      = find_specialization_spell( "Searing Flames" );
  spec.shamanistic_rage    = find_specialization_spell( "Shamanistic Rage" );
  spec.static_shock        = find_specialization_spell( "Static Shock" );

  // Restoration
  spec.ancestral_awakening = find_specialization_spell( "Ancestral Awakening" );
  spec.ancestral_focus     = find_specialization_spell( "Ancestral Focus" );
  spec.earth_shield        = find_specialization_spell( "Earth Shield" );
  spec.meditation          = find_specialization_spell( "Meditation" );
  spec.purification        = find_specialization_spell( "Purification" );
  spec.resurgence          = find_specialization_spell( "Resurgence" );
  spec.riptide             = find_specialization_spell( "Riptide" );
  spec.tidal_waves         = find_specialization_spell( "Tidal Waves" );

  // Masteries
  mastery.elemental_overload         = find_mastery_spell( SHAMAN_ELEMENTAL   );
  mastery.enhanced_elements          = find_mastery_spell( SHAMAN_ENHANCEMENT );
  mastery.deep_healing               = find_mastery_spell( SHAMAN_RESTORATION );

  // Talents
  talent.call_of_the_elements        = find_talent_spell( "Call of the Elements" );
  talent.totemic_restoration         = find_talent_spell( "Totemic Restoration"  );

  talent.elemental_mastery           = find_talent_spell( "Elemental Mastery"    );
  talent.ancestral_swiftness         = find_talent_spell( "Ancestral Swiftness"  );
  talent.echo_of_the_elements        = find_talent_spell( "Echo of the Elements" );

  talent.unleashed_fury              = find_talent_spell( "Unleashed Fury"       );
  talent.primal_elementalist         = find_talent_spell( "Primal Elementalist"  );
  talent.elemental_blast             = find_talent_spell( "Elemental Blast"      );

  // Glyphs
  glyph.chain_lightning              = find_glyph_spell( "Glyph of Chain Lightning" );
  glyph.fire_elemental_totem         = find_glyph_spell( "Glyph of Fire Elemental Totem" );
  glyph.flame_shock                  = find_glyph_spell( "Glyph of Flame Shock" );
  glyph.healing_storm                = find_glyph_spell( "Glyph of Healing Storm" );
  glyph.lava_lash                    = find_glyph_spell( "Glyph of Lava Lash" );
  glyph.spiritwalkers_grace          = find_glyph_spell( "Glyph of Spiritwalker's Grace" );
  glyph.telluric_currents            = find_glyph_spell( "Glyph of Telluric Currents" );
  glyph.thunder                      = find_glyph_spell( "Glyph of Thunder" );
  glyph.thunderstorm                 = find_glyph_spell( "Glyph of Thunderstorm" );
  glyph.unleashed_lightning          = find_glyph_spell( "Glyph of Unleashed Lightning" );
  glyph.water_shield                 = find_glyph_spell( "Glyph of Water Shield" );

  // Misc spells
  spell.ancestral_swiftness          = find_spell( 121617 );
  spell.primal_wisdom                = find_spell( 63375 );
  spell.resurgence                   = find_spell( 101033 );
  spell.searing_flames               = find_spell( 77657 );
  spell.flame_shock                  = find_class_spell( "Flame Shock" );

  // Constants
  constant.haste_attack_ancestral_swiftness = 1.0 / ( 1.0 + spell.ancestral_swiftness -> effectN( 2 ).percent() );
  constant.haste_spell_ancestral_swiftness  = 1.0 / ( 1.0 + spell.ancestral_swiftness -> effectN( 1 ).percent() );

  player_t::init_spells();
}

// shaman_t::init_base ======================================================

void shaman_t::init_base()
{
  player_t::init_base();

  base.attack_power = ( level * 2 ) - 30;
  initial.attack_power_per_strength = 1.0;
  initial.attack_power_per_agility  = 2.0;
  initial.spell_power_per_intellect = 1.0;

  resources.initial_multiplier[ RESOURCE_MANA ] = 1.0 + spec.spiritual_insight -> effectN( 1 ).percent();
  base.mana_regen_per_second *= 1.0 + spec.spiritual_insight -> effectN( 1 ).percent();

  current.distance = ( specialization() == SHAMAN_ENHANCEMENT ) ? 3 : 30;
  initial.distance = current.distance;
  initial.mana_regen_from_spirit_multiplier = spec.meditation -> effectN( 1 ).percent();

  diminished_kfactor    = 0.009880;
  diminished_dodge_cap = 0.006870;
  diminished_parry_cap = 0.006870;

  //if ( specialization() == SHAMAN_ENHANCEMENT )
  //  ready_type = READY_TRIGGER;

  if ( eoe_proc_chance == 0 )
  {
    if ( specialization() == SHAMAN_ENHANCEMENT )
      eoe_proc_chance = 0.30; /// Tested, ~30% (1k LB casts)
    else
      eoe_proc_chance = 0.06; // Tested, ~6% (1k LB casts)
  }

}

// shaman_t::init_scaling ===================================================

void shaman_t::init_scaling()
{
  player_t::init_scaling();

  switch ( specialization() )
  {
  case SHAMAN_ENHANCEMENT:
    scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = true;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = sim -> weapon_speed_scale_factors != 0;
    scales_with[ STAT_HIT_RATING2           ] = true;
    scales_with[ STAT_SPIRIT                ] = false;
    scales_with[ STAT_SPELL_POWER           ] = false;
    scales_with[ STAT_INTELLECT             ] = false;
    break;
  case SHAMAN_ELEMENTAL:
    scales_with[ STAT_SPIRIT                ] = false;
    break;
  case SHAMAN_RESTORATION:
    scales_with[ STAT_MASTERY_RATING ] = false;
    break;
  default:
    break;
  }
}

// shaman_t::init_buffs =====================================================

void shaman_t::create_buffs()
{
  player_t::create_buffs();

  buff.ancestral_swiftness     = buff_creator_t( this, "ancestral_swiftness", talent.ancestral_swiftness )
                                 .cd( timespan_t::zero() );
  buff.ascendance              = new ascendance_buff_t( this );
  buff.elemental_focus         = buff_creator_t( this, "elemental_focus",   spec.elemental_focus -> effectN( 1 ).trigger() )
                                 .activated( false );
  buff.lava_surge              = buff_creator_t( this, "lava_surge",        spec.lava_surge )
                                 .activated( false );
  buff.lightning_shield        = buff_creator_t( this, "lightning_shield", find_class_spell( "Lightning Shield" ) )
                                 .max_stack( ( specialization() == SHAMAN_ELEMENTAL )
                                             ? static_cast< int >( spec.rolling_thunder -> effectN( 1 ).base_value() )
                                             : find_class_spell( "Lightning Shield" ) -> initial_stacks() );
  buff.maelstrom_weapon        = buff_creator_t( this, "maelstrom_weapon",  spec.maelstrom_weapon -> effectN( 1 ).trigger() )
                                 .chance( spec.maelstrom_weapon -> proc_chance() )
                                 .activated( false );
  buff.searing_flames          = buff_creator_t( this, "searing_flames", find_specialization_spell( "Searing Flames" ) )
                                 .chance( find_spell( 77661 ) -> proc_chance() )
                                 .duration( find_spell( 77661 ) -> duration() )
                                 .max_stack( find_spell( 77661 ) -> max_stacks() );
  buff.shamanistic_rage        = buff_creator_t( this, "shamanistic_rage",  spec.shamanistic_rage );
  buff.spiritwalkers_grace     = buff_creator_t( this, "spiritwalkers_grace", find_class_spell( "Spiritwalker's Grace" ) )
                                 .chance( 1.0 )
                                 .duration( find_class_spell( "Spiritwalker's Grace" ) -> duration() +
                                            glyph.spiritwalkers_grace -> effectN( 1 ).time_value() +
                                            sets -> set( SET_T13_4PC_HEAL ) -> effectN( 1 ).time_value() );
  buff.tidal_waves             = buff_creator_t( this, "tidal_waves", spec.tidal_waves -> ok() ? find_spell( 53390 ) : spell_data_t::not_found() );
  buff.unleash_flame           = new unleash_flame_buff_t( this );
  buff.unleashed_fury_wf       = buff_creator_t( this, "unleashed_fury_wf", find_spell( 118472 ) );
  buff.water_shield            = buff_creator_t( this, "water_shield", find_class_spell( "Water Shield" ) );

  // Haste buffs
  buff.elemental_mastery       = haste_buff_creator_t( this, "elemental_mastery", talent.elemental_mastery )
                                 .chance( 1.0 );
  constant.haste_elemental_mastery = 1.0 / ( 1.0 + buff.elemental_mastery -> data().effectN( 1 ).percent() );

  buff.flurry                  = haste_buff_creator_t( this, "flurry", spec.flurry -> effectN( 1 ).trigger() )
                                 .chance( spec.flurry -> proc_chance() )
                                 .activated( false );
  constant.flurry_rating_multiplier = spec.flurry -> effectN( 1 ).trigger() -> effectN( 2 ).percent();
  constant.attack_speed_flurry = 1.0 / ( 1.0 + spec.flurry -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );

  buff.unleash_wind            = haste_buff_creator_t( this, "unleash_wind", find_spell( 73681 ) );
  constant.attack_speed_unleash_wind = 1.0 / ( 1.0 + buff.unleash_wind -> data().effectN( 2 ).percent() );

  buff.tier13_4pc_healer       = haste_buff_creator_t( this, "tier13_4pc_healer", find_spell( 105877 ) );

  // Stat buffs
  buff.elemental_blast_crit    = stat_buff_creator_t( this, "elemental_blast_crit", find_spell( 118522 ) )
                                 .max_stack( 1 )
                                 .add_stat( STAT_CRIT_RATING, find_spell( 118522 ) -> effectN( 1 ).average( this ) );
  buff.elemental_blast_haste   = stat_buff_creator_t( this, "elemental_blast_haste", find_spell( 118522 ) )
                                 .max_stack( 1 )
                                 .add_stat( STAT_HASTE_RATING, find_spell( 118522 ) -> effectN( 2 ).average( this ) );
  buff.elemental_blast_mastery = stat_buff_creator_t( this, "elemental_blast_mastery", find_spell( 118522 ) )
                                 .max_stack( 1 )
                                 .add_stat( STAT_MASTERY_RATING, find_spell( 118522 ) -> effectN( 3 ).average( this ) );
  buff.elemental_blast_agility = stat_buff_creator_t( this, "elemental_blast_agility", find_spell( 118522 ) )
                                 .max_stack( 1 )
                                 .add_stat( STAT_AGILITY, find_spell( 118522 ) -> effectN( 4 ).average( this ) );
  buff.tier13_2pc_caster        = stat_buff_creator_t( this, "tier13_2pc_caster", find_spell( 105779 ) );
  buff.tier13_4pc_caster        = stat_buff_creator_t( this, "tier13_4pc_caster", find_spell( 105821 ) );

}

// shaman_t::init_gains =====================================================

void shaman_t::init_gains()
{
  player_t::init_gains();

  gain.primal_wisdom        = get_gain( "primal_wisdom"     );
  gain.resurgence           = get_gain( "resurgence"        );
  gain.rolling_thunder      = get_gain( "rolling_thunder"   );
  gain.telluric_currents    = get_gain( "telluric_currents" );
  gain.thunderstorm         = get_gain( "thunderstorm"      );
  gain.water_shield         = get_gain( "water_shield"      );
}

// shaman_t::init_procs =====================================================

void shaman_t::init_procs()
{
  player_t::init_procs();

  proc.elemental_overload = get_proc( "elemental_overload"      );
  proc.lava_surge         = get_proc( "lava_surge"              );
  proc.ls_fast            = get_proc( "lightning_shield_too_fast_fill" );
  proc.maelstrom_weapon   = get_proc( "maelstrom_weapon"        );
  proc.static_shock       = get_proc( "static_shock"            );
  proc.swings_clipped_mh  = get_proc( "swings_clipped_mh"       );
  proc.swings_clipped_oh  = get_proc( "swings_clipped_oh"       );
  proc.swings_reset_mh    = get_proc( "swings_reset_mh"         );
  proc.swings_reset_oh    = get_proc( "swings_reset_oh"         );
  proc.rolling_thunder    = get_proc( "rolling_thunder"         );
  proc.uf_flame_shock     = get_proc( "uf_flame_shock"          );
  proc.uf_fire_nova       = get_proc( "uf_fire_nova"            );
  proc.uf_lava_burst      = get_proc( "uf_lava_burst"           );
  proc.uf_elemental_blast = get_proc( "uf_elemental_blast"      );
  proc.uf_wasted          = get_proc( "uf_wasted"               );
  proc.t15_2pc_melee      = get_proc( "t15_2pc_melee"           );
  proc.wasted_t15_2pc_melee = get_proc( "wasted_t15_2pc_melee"  );
  proc.wasted_ls          = get_proc( "wasted_lightning_shield" );
  proc.wasted_ls_shock_cd = get_proc( "wasted_lightning_shield_shock_cd" );
  proc.wasted_mw          = get_proc( "wasted_maelstrom_weapon" );
  proc.windfury           = get_proc( "windfury"                );

  for ( int i = 0; i < 7; i++ )
  {
    proc.fulmination[ i ] = get_proc( "fulmination_" + util::to_string( i ) );
  }

  for ( int i = 0; i < 6; i++ )
  {
    proc.maelstrom_weapon_used[ i ] = get_proc( "maelstrom_weapon_stack_" + util::to_string( i ) );
  }
}

// shaman_t::init_rng =======================================================

void shaman_t::init_rng()
{
  player_t::init_rng();

  rng.earthliving          = get_rng( "earthliving"          );
  rng.echo_of_the_elements = get_rng( "echo_of_the_elements" );
  rng.elemental_overload   = get_rng( "elemental_overload"   );
  rng.lava_surge           = get_rng( "lava_surge"           );
  rng.primal_wisdom        = get_rng( "primal_wisdom"        );
  rng.rolling_thunder      = get_rng( "rolling_thunder"      );
  rng.searing_flames       = get_rng( "searing_flames"       );
  rng.static_shock         = get_rng( "static_shock"         );
  rng.t15_2pc_caster       = get_rng( "t15_2pc_caster"       );
  rng.windfury_delay       = get_rng( "windfury_delay"       );
  rng.windfury_weapon      = get_rng( "windfury_weapon"      );
}

// shaman_t::init_actions ===================================================

void shaman_t::init_actions()
{
  if ( ! ( primary_role() == ROLE_ATTACK && specialization() == SHAMAN_ENHANCEMENT ) &&
       ! ( primary_role() == ROLE_SPELL  && specialization() == SHAMAN_ELEMENTAL   ) )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s's role (%s) or spec(%s) isn't supported yet.",
                     name(), util::role_type_string( primary_role() ), dbc::specialization_string( specialization() ).c_str() );
    quiet = true;
    return;
  }

  if ( specialization() == SHAMAN_ENHANCEMENT && main_hand_weapon.type == WEAPON_NONE )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  // Restoration isn't supported atm
  if ( specialization() == SHAMAN_RESTORATION && primary_role() == ROLE_HEAL )
  {
    if ( ! quiet )
      sim -> errorf( "Restoration Shaman healing for player %s is not currently supported.", name() );

    quiet = true;
    return;
  }

  if ( ! action_list_str.empty() )
  {
    player_t::init_actions();
    return;
  }

  int hand_addon = -1;

  for ( int i = 0; i < ( int ) items.size(); i++ )
  {
    if ( items[ i ].use.active() && items[ i ].slot == SLOT_HANDS )
    {
      hand_addon = i;
      break;
    }
  }

  clear_action_priority_lists();

  std::vector<std::string> use_items;
  for ( size_t i = 0; i < items.size(); i++ )
  {
    if ( items[ i ].use.active() && ( specialization() != SHAMAN_ELEMENTAL || items[ i ].slot != SLOT_HANDS ) )
      use_items.push_back( "use_item,name=" + items[ i ].name_str() );
  }

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* def       = get_action_priority_list( "default"   );
  action_priority_list_t* single    = get_action_priority_list( "single", "Single target action priority list" );
  action_priority_list_t* aoe       = get_action_priority_list( "aoe", "Multi target action priority list" );

  // Flask
  if ( sim -> allow_flasks && level >= 80 )
  {
    std::string flask_action = "flask,type=";
    if ( primary_role() == ROLE_ATTACK )
      flask_action += ( ( level > 85 ) ? "spring_blossoms" : ( level >= 80 ) ? "winds" : "" );
    else
      flask_action += ( ( level > 85 ) ? "warm_sun" : ( level >= 80 ) ? "draconic_mind" : "" );

    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food && level >= 80 )
  {
    std::string food_action = "food,type=";
    if ( specialization() == SHAMAN_ENHANCEMENT )
      food_action += ( level > 85 ) ? "sea_mist_rice_noodles" : "seafood_magnifique_feast";
    else
      food_action += ( level > 85 ) ? "mogu_fish_stew" : "seafood_magnifique_feast";

    precombat -> add_action( food_action );
  }

  // Weapon Enchants
  if ( specialization() == SHAMAN_ENHANCEMENT && primary_role() == ROLE_ATTACK )
  {
    precombat -> add_action( this, "Windfury Weapon", "weapon=main" );
    if ( off_hand_weapon.type != WEAPON_NONE )
      precombat -> add_action( this, "Flametongue Weapon", "weapon=off" );
  }
  else
  {
    precombat -> add_action( this, "Flametongue Weapon", "weapon=main" );
    if ( specialization() == SHAMAN_ENHANCEMENT && off_hand_weapon.type != WEAPON_NONE )
      precombat -> add_action( this, "Flametongue Weapon", "weapon=off" );
  }

  // Active Shield, presume any non-restoration / healer wants lightning shield
  if ( specialization() != SHAMAN_RESTORATION || primary_role() != ROLE_HEAL )
    precombat -> add_action( this, "Lightning Shield", "if=!buff.lightning_shield.up" );
  else
    precombat -> add_action( this, "Water Shield", "if=!buff.water_shield.up" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  if ( sim -> allow_potions && level >= 80 )
  {
    // Prepotion (work around for now, until snapshot_stats stop putting things into combat)
    if ( primary_role() == ROLE_ATTACK )
      precombat -> add_action( level > 85 ? "virmens_bite_potion" : "tolvir_potion" );
    else
      precombat -> add_action( level > 85 ? "jade_serpent_potion" : "volcanic_potion" );
  }

  // All Shamans Bloodlust and Wind Shear by default
  def -> add_action( this, "Wind Shear" );

  std::string bloodlust_options = "if=";

  if ( sim -> bloodlust_percent > 0 )
    bloodlust_options += "target.health.pct<" + util::to_string( sim -> bloodlust_percent ) + "|";

  if ( sim -> bloodlust_time < timespan_t::zero() )
    bloodlust_options += "target.time_to_die<" + util::to_string( - sim -> bloodlust_time.total_seconds() ) + "|";

  if ( sim -> bloodlust_time > timespan_t::zero() )
    bloodlust_options += "time>" + util::to_string( sim -> bloodlust_time.total_seconds() ) + "|";
  bloodlust_options.erase( bloodlust_options.end() - 1 );

  if ( action_priority_t* a = def -> add_action( this, "Bloodlust", bloodlust_options ) )
    a -> comment( "Bloodlust casting behavior mirrors the simulator settings for proxy bloodlust. See options 'bloodlust_percent', and 'bloodlust_time'. " );

  // Melee turns on auto attack
  if ( primary_role() == ROLE_ATTACK )
    def -> add_action( "auto_attack" );

  // On use stuff and profession abilities
  for ( size_t i = 0; i < use_items.size(); i++ )
    def -> add_action( use_items[ i ] );

  // Need to remove the "/" in front of the profession action(s) for the new default action priority list stuff :/
  if ( ! init_use_profession_actions().empty() )
    def -> add_action( init_use_profession_actions().erase( 0, 1 ) );

  // Stormlash Totem
  if ( action_priority_t* a = def -> add_action( this, "Stormlash Totem", "if=!active&!buff.stormlash.up&(buff.bloodlust.up|time>=60)" ) )
    a -> comment( "Link Stormlash totem cast to early Bloodlust, and ensure that only one Stormlash is used at a time." );

  // In-combat potion
  if ( sim -> allow_potions && level >= 80  )
  {
    std::string potion_action = "";
    if ( primary_role() == ROLE_ATTACK )
      potion_action = level > 85 ? "virmens_bite_potion" : "tolvir_potion";
    else
      potion_action = level > 85 ? "jade_serpent_potion" : "volcanic_potion";

    potion_action += ",if=time>60&(pet.primal_fire_elemental.active|pet.greater_fire_elemental.active|target.time_to_die<=60)";

    def -> add_action( potion_action, "In-combat potion is linked to Primal or Greater Fire Elemental Totem, after the first 60 seconds of combat." );
  }

  if ( specialization() == SHAMAN_ENHANCEMENT && primary_role() == ROLE_ATTACK )
  {
    // Need to remove the "/" in front of the racial action for the new default action priority list stuff :/
    if ( ! init_use_racial_actions().empty() )
      def -> add_action( init_use_racial_actions().erase( 0, 1 ) );

    def -> add_talent( this, "Elemental Mastery", "if=!glyph.fire_elemental_totem.enabled|cooldown.fire_elemental_totem.remains=0|cooldown.fire_elemental_totem.remains>70" );

    if ( glyph.fire_elemental_totem -> ok() )
      def -> add_action( this, "Fire Elemental Totem", "if=!active" );
    else
    {
      def -> add_action( this, "Fire Elemental Totem", "if=!active&(buff.bloodlust.up|buff.lifeblood.up|buff.elemental_mastery.up)|time>=60" );
      def -> add_action( this, "Fire Elemental Totem", "if=!active&target.time_to_die<=totem.fire_elemental_totem.duration+10" );
    }

    def -> add_action( this, "Ascendance", "if=cooldown.strike.remains>=3" );

    def -> add_action( "run_action_list,name=single,if=active_enemies=1", "If only one enemy, priority follows the 'single' action list." );
    def -> add_action( "run_action_list,name=aoe,if=active_enemies>1", "On multiple enemies, the priority follows the 'aoe' action list." );

    single -> add_action( this, "Searing Totem", "if=!totem.fire.active" );
    single -> add_action( this, "Unleash Elements", "if=talent.unleashed_fury.enabled" );
    single -> add_talent( this, "Elemental Blast", "if=buff.maelstrom_weapon.react>=1" );
    single -> add_action( this, spec.maelstrom_weapon, "lightning_bolt", "if=buff.maelstrom_weapon.react=5|(set_bonus.tier13_4pc_melee=1&buff.maelstrom_weapon.react>=4&pet.spirit_wolf.active)" );
    single -> add_action( this, "Feral Spirit", "if=set_bonus.tier15_4pc_melee=1" );
    single -> add_action( this, find_class_spell( "Ascendance" ), "stormblast" );
    single -> add_action( this, "Stormstrike" );
    single -> add_action( this, "Primal Strike" );
    single -> add_action( this, "Flame Shock", "if=buff.unleash_flame.up&!ticking" );
    single -> add_action( this, "Lava Lash" );
    single -> add_action( this, spec.maelstrom_weapon, "lightning_bolt", "if=set_bonus.tier15_2pc_melee=1&buff.maelstrom_weapon.react>=4&!buff.ascendance.up" );
    single -> add_action( this, "Flame Shock", "if=buff.unleash_flame.up|(!buff.unleash_flame.up&!ticking&cooldown.unleash_elements.remains>5)" );
    single -> add_action( this, "Unleash Elements" );
    single -> add_action( this, spec.maelstrom_weapon, "lightning_bolt", "if=buff.maelstrom_weapon.react>=3&!buff.ascendance.up" );
    single -> add_talent( this, "Ancestral Swiftness", "if=buff.maelstrom_weapon.react<2" ) ;
    single -> add_action( this, "Lightning Bolt", "if=buff.ancestral_swiftness.up" );
    single -> add_action( this, "Flame Shock", "if=buff.unleash_flame.up&dot.flame_shock.remains<=3" );
    single -> add_action( this, "Earth Shock" );
    single -> add_action( this, "Feral Spirit" );
    single -> add_action( this, "Earth Elemental Totem", "if=!active&cooldown.fire_elemental_totem.remains>=50" );
    single -> add_action( this, "Spiritwalker's Grace", "moving=1" );
    single -> add_action( this, spec.maelstrom_weapon, "lightning_bolt", "if=buff.maelstrom_weapon.react>1&!buff.ascendance.up" );

    // AoE
    aoe -> add_action( this, "Magma Totem", "if=active_enemies>5&!totem.fire.active" );
    aoe -> add_action( this, "Searing Totem", "if=active_enemies<=5&!totem.fire.active" );
    aoe -> add_action( this, "Fire Nova", "if=(active_enemies<=5&active_flame_shock=active_enemies)|active_flame_shock>=5" );
    aoe -> add_action( this, "Lava Lash", "if=dot.flame_shock.ticking" );
    aoe -> add_action( this, spec.maelstrom_weapon, "chain_lightning", "if=active_enemies>2&buff.maelstrom_weapon.react>=3" );
    aoe -> add_action( this, "Unleash Elements" );
    aoe -> add_action( this, "Flame Shock", "cycle_targets=1,if=!ticking" );
    aoe -> add_action( this, find_class_spell( "Ascendance" ), "stormblast" );
    aoe -> add_action( this, "Stormstrike" );
    aoe -> add_action( this, "Primal Strike" );
    aoe -> add_action( this, spec.maelstrom_weapon, "lightning_bolt", "if=buff.maelstrom_weapon.react=5&cooldown.chain_lightning.remains>=2" );
    aoe -> add_action( this, "Feral Spirit" );
    aoe -> add_action( this, spec.maelstrom_weapon, "chain_lightning", "if=active_enemies>2&buff.maelstrom_weapon.react>1" );
    aoe -> add_action( this, spec.maelstrom_weapon, "lightning_bolt", "if=buff.maelstrom_weapon.react>1&!buff.ascendance.up" );
  }
  else if ( specialization() == SHAMAN_ELEMENTAL && ( primary_role() == ROLE_SPELL || primary_role() == ROLE_DPS ) )
  {
    // Sync berserking with ascendance as they share a cooldown, but making sure
    // that no two haste cooldowns overlap, within reason
    if ( race == RACE_TROLL )
      def -> add_action( "berserking,if=!buff.bloodlust.up&!buff.elemental_mastery.up&buff.ascendance.cooldown_remains=0&(dot.flame_shock.remains>buff.ascendance.duration|level<87)" );
    // Sync blood fury with ascendance or fire elemental as long as one is ready
    // soon after blood fury is.
    else if ( race == RACE_ORC )
      def -> add_action( "blood_fury,if=buff.bloodlust.up|buff.ascendance.up|((cooldown.ascendance.remains>10|level<87)&cooldown.fire_elemental_totem.remains>10)" );
    else if ( ! init_use_racial_actions().empty() )
      def -> add_action( init_use_racial_actions().erase( 0, 1 ) );

    // Use Elemental Mastery after initial Bloodlust ends. Also make sure that
    // Elemental Mastery is not used during Ascendance, if Berserking is up.
    // Finally, after the second Ascendance (time 200+ seconds), start using
    // Elemental Mastery on cooldown.
    def -> add_talent( this, "Elemental Mastery", "if=time>15&((!buff.bloodlust.up&time<120)|(!buff.berserking.up&!buff.bloodlust.up&buff.ascendance.up)|(time>=200&(cooldown.ascendance.remains>30|level<87)))" );

    def -> add_action( this, "Fire Elemental Totem", "if=!active" );

    // Use Ascendance preferably with a haste CD up, but dont overdo the
    // delaying. Make absolutely sure that Ascendance can be used so that
    // only Lava Bursts need to be cast during it's duration
    std::string ascendance_opts = "if=active_enemies>1|(dot.flame_shock.remains>buff.ascendance.duration&(target.time_to_die<20|buff.bloodlust.up";
    if ( race == RACE_TROLL )
      ascendance_opts += "|buff.berserking.up";
    else
      ascendance_opts += "|time>=60";
    ascendance_opts += ")&cooldown.lava_burst.remains>0)";

    def -> add_action( this, "Ascendance", ascendance_opts );

    def -> add_action( "run_action_list,name=single,if=active_enemies=1", "If only one enemy, priority follows the 'single' action list." );
    def -> add_action( "run_action_list,name=aoe,if=active_enemies>1", "On multiple enemies, the priority follows the 'aoe' action list." );

    if ( hand_addon > -1 )
      single -> add_action( "use_item,name=" + items[ hand_addon ].name_str() + ",if=((cooldown.ascendance.remains>10|level<87)&cooldown.fire_elemental_totem.remains>10)|buff.ascendance.up|buff.bloodlust.up|totem.fire_elemental_totem.active" );

    single -> add_talent( this, "Ancestral Swiftness", "if=!buff.ascendance.up" );
    single -> add_action( this, "Unleash Elements", "if=talent.unleashed_fury.enabled&!buff.ascendance.up" );
    single -> add_action( this, "Spiritwalker's Grace", "moving=1,if=buff.ascendance.up" );
    single -> add_action( this, "Lava Burst", "if=dot.flame_shock.remains>cast_time&(buff.ascendance.up|cooldown_react)" );
    single -> add_action( this, "Flame Shock", "if=ticks_remain<2" );
    single -> add_talent( this, "Elemental Blast" );
    single -> add_action( this, spec.fulmination, "earth_shock", "if=buff.lightning_shield.react=buff.lightning_shield.max_stack",
                          "Use Earth Shock if Lightning Shield is at max (7) charges" );
    single -> add_action( this, spec.fulmination, "earth_shock", "if=buff.lightning_shield.react>3&dot.flame_shock.remains>cooldown&dot.flame_shock.remains<cooldown+action.flame_shock.tick_time",
                          "Use Earth Shock if Lightning Shield is above 3 charges and the Flame Shock remaining duration is longer than the shock cooldown but shorter than shock cooldown + tick time interval" );
    single -> add_action( this, "Flame Shock", "if=time>60&remains<=buff.ascendance.duration&cooldown.ascendance.remains+buff.ascendance.duration<duration",
                          "After the initial Ascendance, use Flame Shock pre-emptively just before Ascendance to guarantee Flame Shock staying up for the full duration of the Ascendance buff" );
    single -> add_action( this, "Earth Elemental Totem", "if=!active&cooldown.fire_elemental_totem.remains>=60" );
    single -> add_action( this, "Searing Totem", "if=cooldown.fire_elemental_totem.remains>20&!totem.fire.active",
                          "Keep Searing Totem up, unless Fire Elemental Totem is coming off cooldown in the next 20 seconds" );
    single -> add_action( this, "Spiritwalker's Grace", "moving=1,if=glyph.unleashed_lightning.enabled&((talent.elemental_blast.enabled&cooldown.elemental_blast.remains=0)|(cooldown.lava_burst.remains=0&!buff.lava_surge.react))|(buff.raid_movement.duration>=action.unleash_elements.gcd+action.earth_shock.gcd)" );
    single -> add_action( this, "Unleash Elements", "moving=1,if=!glyph.unleashed_lightning.enabled" );
    single -> add_action( this, "Earth Shock", "moving=1,if=!glyph.unleashed_lightning.enabled&dot.flame_shock.remains>cooldown",
                          "Use Earth Shock when moving if Glyph of Unleashed Lightning is not equipped and there's at least shock cooldown time of Flame Shock duration left" );
    single -> add_action( this, "Lightning Bolt" );

    // AoE
    aoe -> add_action( this, find_class_spell( "Ascendance" ), "lava_beam" );
    aoe -> add_action( this, "Magma Totem", "if=active_enemies>2&!totem.fire.active" );
    aoe -> add_action( this, "Searing Totem", "if=active_enemies<=2&!totem.fire.active" );
    aoe -> add_action( this, "Lava Burst", "if=active_enemies<3&dot.flame_shock.remains>cast_time&cooldown_react" );
    aoe -> add_action( this, "Flame Shock", "cycle_targets=1,if=!ticking&active_enemies<3" );
    aoe -> add_action( this, "Earthquake", "if=active_enemies>4" );
    aoe -> add_action( this, "Thunderstorm", "if=mana.pct_nonproc<80" );
    aoe -> add_action( this, "Chain Lightning", "if=mana.pct_nonproc>10" );
    aoe -> add_action( this, "Lightning Bolt" );
  }
  else if ( primary_role() == ROLE_SPELL )
  {
    def -> add_action( this, "Fire Elemental Totem", "if=!active" );
    def -> add_action( this, "Searing Totem", "if=!totem.fire.active" );
    def -> add_action( this, "Earth Elemental Totem", "if=!active&cooldown.fire_elemental_totem.remains>=50" );
    def -> add_action( this, "Spiritwalker's Grace", "moving=1" );
    def -> add_talent( this, "Elemental Mastery" );
    def -> add_talent( this, "Elemental Blast" );
    def -> add_talent( this, "Ancestral Swiftness" );
    def -> add_action( this, "Flame Shock", "if=!ticking|ticks_remain<2|((buff.bloodlust.react|buff.elemental_mastery.up)&ticks_remain<3)" );
    def -> add_action( this, "Lava Burst", "if=dot.flame_shock.remains>cast_time" );
    def -> add_action( this, "Chain Lightning", "if=target.adds>2&mana.pct>25" );
    def -> add_action( this, "Lightning Bolt" );
  }
  else if ( primary_role() == ROLE_ATTACK )
  {
    def -> add_action( this, "Fire Elemental Totem", "if=!active" );
    def -> add_action( this, "Searing Totem", "if=!totem.fire.active" );
    def -> add_action( this, "Earth Elemental Totem", "if=!active&cooldown.fire_elemental_totem.remains>=50" );
    def -> add_action( this, "Spiritwalker's Grace", "moving=1" );
    def -> add_talent( this, "Elemental Mastery" );
    def -> add_talent( this, "Elemental Blast" );
    def -> add_talent( this, "Ancestral Swiftness" );
    def -> add_talent( this, "Primal Strike" );
    def -> add_action( this, "Flame Shock", "if=!ticking|ticks_remain<2|((buff.bloodlust.react|buff.elemental_mastery.up)&ticks_remain<3)" );
    def -> add_action( this, "Earth Shock" );
    def -> add_action( this, "Lightning Bolt", "moving=1,if=glyph.unleashed_lightning.enabled" );
  }

  action_list_default = 1;

  player_t::init_actions();
}

// shaman_t::moving =========================================================

void shaman_t::moving()
{
  // Spiritwalker's Grace complicates things, as you can cast it while casting
  // anything. So, to model that, if a raid move event comes, we need to check
  // if we can trigger Spiritwalker's Grace. If so, conditionally execute it, to
  // allow the currently executing cast to finish.
  if ( level >= 85 )
  {
    action_t* swg = find_action( "spiritwalkers_grace" );

    // We need to bypass swg -> ready() check here, so whip up a special
    // readiness check that only checks for player skill, cooldown and resource
    // availability
    if ( swg && executing && swg -> ready() )
    {
      // Shaman executes SWG mid-cast during a movement event, if
      // 1) The profile does not have Glyph of Unleashed Lightning and is
      //    casting a Lighting Bolt (non-instant cast)
      // 2) The profile is casting Lava Burst (without Lava Surge)
      // 3) The profile is casting Chain Lightning
      // 4) The profile is casting Elemental Blast
      if ( ( ! glyph.unleashed_lightning -> ok() && executing -> id == 403 ) ||
           ( executing -> id == 51505 ) ||
           ( executing -> id == 421 ) ||
           ( executing -> id == 117014 ) )
      {
        if ( sim -> log )
          sim -> output( "%s spiritwalkers_grace during spell cast, next cast (%s) should finish",
                         name(), executing -> name() );
        swg -> execute();
      }
    }
    else
    {
      interrupt();
    }

    if ( main_hand_attack ) main_hand_attack -> cancel();
    if (  off_hand_attack )  off_hand_attack -> cancel();
  }
  else
  {
    halt();
  }
}

// shaman_t::matching_gear_multiplier =======================================

double shaman_t::matching_gear_multiplier( attribute_e attr )
{
  if ( attr == ATTR_AGILITY || attr == ATTR_INTELLECT )
    return constant.matching_gear_multiplier;

  return 0.0;
}

// shaman_t::composite_spell_haste ==========================================

double shaman_t::composite_spell_haste()
{
  double h = player_t::composite_spell_haste() / spell_haste;
  double hm = 1.0;
  if ( buff.flurry -> up() )
    hm *= 1.0 + constant.flurry_rating_multiplier;
  h *= 1.0 / ( 1.0 + current.haste_rating * hm / rating.spell_haste );

  if ( talent.ancestral_swiftness -> ok() )
    h *= constant.haste_spell_ancestral_swiftness;

  if ( buff.elemental_mastery -> up() )
    h *= constant.haste_elemental_mastery;

  if ( buff.tier13_4pc_healer -> up() )
    h *= 1.0 / ( 1.0 + buff.tier13_4pc_healer -> data().effectN( 1 ).percent() );
  return h;
}

// shaman_t::composite_spell_hit ============================================

double shaman_t::composite_spell_hit()
{
  double hit = player_t::composite_spell_hit();

  hit += ( spec.elemental_precision -> ok() *
           ( spirit() - base.attribute[ ATTR_SPIRIT ] ) ) / rating.spell_hit;

  return hit;
}

// shaman_t::composite_attack_hit ============================================

double shaman_t::composite_attack_hit()
{
  double hit = player_t::composite_attack_hit();

  hit += ( spec.elemental_precision -> ok() *
           ( spirit() - base.attribute[ ATTR_SPIRIT ] ) ) / rating.attack_hit;

  return hit;
}

// shaman_t::composite_attack_haste =========================================

double shaman_t::composite_attack_haste()
{
  double h = player_t::composite_attack_haste() / attack_haste;
  double hm = 1.0;
  if ( buff.flurry -> up() )
    hm *= 1.0 + constant.flurry_rating_multiplier;
  h *= 1.0 / ( 1.0 + current.haste_rating * hm / rating.attack_haste );

  if ( buff.elemental_mastery -> up() )
    h *= constant.haste_elemental_mastery;

  if ( talent.ancestral_swiftness -> ok() )
    h *= constant.haste_attack_ancestral_swiftness;

  if ( buff.tier13_4pc_healer -> up() )
    h *= 1.0 / ( 1.0 + buff.tier13_4pc_healer -> data().effectN( 1 ).percent() );

  return h;
}

// shaman_t::composite_attack_speed =========================================

double shaman_t::composite_attack_speed()
{
  double speed = player_t::composite_attack_speed();

  if ( buff.flurry -> up() )
    speed *= constant.attack_speed_flurry;

  if ( buff.unleash_wind -> up() )
    speed *= constant.attack_speed_unleash_wind;

  return speed;
}

// shaman_t::composite_spell_power ==========================================

double shaman_t::composite_spell_power( school_e school )
{
  double sp = 0;

  if ( specialization() == SHAMAN_ENHANCEMENT )
    sp = composite_attack_power_multiplier() * composite_attack_power() * spec.mental_quickness -> effectN( 1 ).percent();
  else
    sp = player_t::composite_spell_power( school );

  return sp;
}

// shaman_t::composite_spell_power_multiplier ===============================

double shaman_t::composite_spell_power_multiplier()
{
  if ( specialization() == SHAMAN_ENHANCEMENT )
    return 1.0;

  return player_t::composite_spell_power_multiplier();
}

// shaman_t::composite_player_multiplier ====================================

double shaman_t::composite_player_multiplier( school_e school, action_t* a )
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( school != SCHOOL_PHYSICAL )
  {
    if ( main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
      m *= 1.0 + main_hand_weapon.buff_value;
    else if ( off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
      m *= 1.0 + off_hand_weapon.buff_value;
  }

  if ( dbc::is_school( school, SCHOOL_FIRE   ) ||
       dbc::is_school( school, SCHOOL_FROST  ) ||
       dbc::is_school( school, SCHOOL_NATURE ) )
  {
    m *= 1.0 + composite_mastery() * mastery.enhanced_elements -> effectN( 1 ).mastery_value();
  }

  return m;
}

// shaman_t::regen  =========================================================

void shaman_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( buff.water_shield -> up() )
  {
    double water_shield_regen = periodicity.total_seconds() * buff.water_shield -> value() / 5.0;

    resource_gain( RESOURCE_MANA, water_shield_regen, gain.water_shield );
  }
}

// shaman_t::arise() ========================================================

void shaman_t::arise()
{
  player_t::arise();

  assert( main_hand_attack == melee_mh && off_hand_attack == melee_oh );

  if ( ! sim -> overrides.mastery && dbc.spell( 116956 ) -> is_level( level ) )
  {
    double mastery_rating = dbc.spell( 116956 ) -> effectN( 1 ).average( this );
    if ( ! sim -> auras.mastery -> check() || sim -> auras.mastery -> current_value < mastery_rating )
      sim -> auras.mastery -> trigger( 1, mastery_rating );
  }

  if ( ! sim -> overrides.spell_power_multiplier && dbc.spell( 77747 ) -> is_level( level ) )
    sim -> auras.spell_power_multiplier -> trigger();

  if ( specialization() == SHAMAN_ENHANCEMENT && ! sim -> overrides.attack_speed && dbc.spell( 30809 ) -> is_level( level ) )
    sim -> auras.attack_speed -> trigger();

  if ( specialization() == SHAMAN_ELEMENTAL && ! sim -> overrides.spell_haste && dbc.spell( 51470 ) -> is_level( level ) )
    sim -> auras.spell_haste  -> trigger();
}

// shaman_t::reset ==========================================================

void shaman_t::reset()
{
  player_t::reset();

  ls_reset = timespan_t::zero();
  active_flame_shocks = 0;
}

// shaman_t::decode_set =====================================================

int shaman_t::decode_set( item_t& item )
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

  if ( strstr( s, "spiritwalkers" ) )
  {
    bool is_caster = ( strstr( s, "headpiece"     ) ||
                       strstr( s, "shoulderwraps" ) ||
                       strstr( s, "hauberk"       ) ||
                       strstr( s, "kilt"          ) ||
                       strstr( s, "gloves"        ) );

    bool is_melee = ( strstr( s, "helmet"         ) ||
                      strstr( s, "spaulders"      ) ||
                      strstr( s, "cuirass"        ) ||
                      strstr( s, "legguards"      ) ||
                      strstr( s, "grips"          ) );

    bool is_heal  = ( strstr( s, "faceguard"      ) ||
                      strstr( s, "mantle"         ) ||
                      strstr( s, "tunic"          ) ||
                      strstr( s, "legwraps"       ) ||
                      strstr( s, "handwraps"      ) );

    if ( is_caster ) return SET_T13_CASTER;
    if ( is_melee  ) return SET_T13_MELEE;
    if ( is_heal   ) return SET_T13_HEAL;
  }

  if ( strstr( s, "firebirds_" ) )
  {
    bool is_caster = ( strstr( s, "headpiece"     ) ||
                       strstr( s, "shoulderwraps" ) ||
                       strstr( s, "hauberk"       ) ||
                       strstr( s, "kilt"          ) ||
                       strstr( s, "gloves"        ) );

    bool is_melee = ( strstr( s, "helmet"         ) ||
                      strstr( s, "spaulders"      ) ||
                      strstr( s, "cuirass"        ) ||
                      strstr( s, "legguards"      ) ||
                      strstr( s, "grips"          ) );

    bool is_heal  = ( strstr( s, "faceguard"      ) ||
                      strstr( s, "mantle"         ) ||
                      strstr( s, "tunic"          ) ||
                      strstr( s, "legwraps"       ) ||
                      strstr( s, "handwraps"      ) );

    if ( is_caster ) return SET_T14_CASTER;
    if ( is_melee  ) return SET_T14_MELEE;
    if ( is_heal   ) return SET_T14_HEAL;
  }

  if ( strstr( s, "_of_the_witch_doctor" ) )
  {
    bool is_caster = ( strstr( s, "headpiece"     ) ||
                       strstr( s, "shoulderwraps" ) ||
                       strstr( s, "hauberk"       ) ||
                       strstr( s, "kilt"          ) ||
                       strstr( s, "gloves"        ) );

    bool is_melee = ( strstr( s, "helmet"         ) ||
                      strstr( s, "spaulders"      ) ||
                      strstr( s, "cuirass"        ) ||
                      strstr( s, "legguards"      ) ||
                      strstr( s, "grips"          ) );

    bool is_heal  = ( strstr( s, "faceguard"      ) ||
                      strstr( s, "mantle"         ) ||
                      strstr( s, "tunic"          ) ||
                      strstr( s, "legwraps"       ) ||
                      strstr( s, "handwraps"      ) );

    if ( is_caster ) return SET_T15_CASTER;
    if ( is_melee  ) return SET_T15_MELEE;
    if ( is_heal   ) return SET_T15_HEAL;
  }

  if ( strstr( s, "_gladiators_linked_"   ) )     return SET_PVP_MELEE;
  if ( strstr( s, "_gladiators_mail_"     ) )     return SET_PVP_CASTER;
  if ( strstr( s, "_gladiators_ringmail_" ) )     return SET_PVP_MELEE;

  return SET_NONE;
}

// shaman_t::primary_role ===================================================

role_e shaman_t::primary_role()
{
  if ( player_t::primary_role() == ROLE_HEAL )
    return ROLE_HEAL;

  if ( specialization() == SHAMAN_RESTORATION )
  {
    if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_SPELL )
      return ROLE_SPELL;

    return ROLE_SPELL;
  }

  else if ( specialization() == SHAMAN_ENHANCEMENT )
    return ROLE_ATTACK;

  else if ( specialization() == SHAMAN_ELEMENTAL )
    return ROLE_SPELL;

  return player_t::primary_role();
}

// SHAMAN MODULE INTERFACE ================================================

struct shaman_module_t : public module_t
{
  shaman_module_t() : module_t( SHAMAN ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    return new shaman_t( sim, name, r );
  }
  virtual bool valid() const { return true; }
  virtual void init( sim_t* sim ) const
  {
    for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[i];
      p -> buffs.bloodlust  = haste_buff_creator_t( p, "bloodlust", p -> find_spell( 2825 ) )
                              .max_stack( 1 );

      p -> buffs.earth_shield = buff_creator_t( p, "earth_shield", p -> find_spell( 974 ) )
                                .cd( timespan_t::from_seconds( 2.0 ) );

      p -> buffs.exhaustion = buff_creator_t( p, "exhaustion", p -> find_spell( 57723 ) )
                              .max_stack( 1 )
                              .quiet( true );
    }
  }
  virtual void combat_begin( sim_t* ) const {}
  virtual void combat_end( sim_t* ) const {}
};

} // UNNAMED NAMESPACE

const module_t& module_t::shaman_()
{
  static shaman_module_t m = shaman_module_t();
  return m;
}
