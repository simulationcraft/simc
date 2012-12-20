// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "simulationcraft.hpp"

namespace { // unnamed namespace

static const int WILD_IMP_LIMIT = 25;
static const int META_FURY_MINIMUM = 40;

struct warlock_t;

namespace pets {
struct wild_imp_pet_t;
}

struct warlock_td_t : public actor_pair_t
{
  dot_t*  dots_corruption;
  dot_t*  dots_unstable_affliction;
  dot_t*  dots_agony;
  dot_t*  dots_doom;
  dot_t*  dots_immolate;
  dot_t*  dots_drain_soul;
  dot_t*  dots_shadowflame;
  dot_t*  dots_malefic_grasp;
  dot_t*  dots_seed_of_corruption;
  dot_t*  dots_soulburn_seed_of_corruption;
  dot_t*  dots_haunt;

  buff_t* debuffs_haunt;

  bool ds_started_below_20;
  int shadowflame_stack;
  int agony_stack;
  double soc_trigger, soulburn_soc_trigger;

  warlock_td_t( player_t* target, warlock_t* source );

  void reset()
  {
    ds_started_below_20 = false;
    shadowflame_stack = 1;
    agony_stack = 1;
    soc_trigger = 0;
    soulburn_soc_trigger = 0;
  }

  void init()
  {
    debuffs_haunt -> init();
  }
};

struct warlock_t : public player_t
{
public:

  player_t* havoc_target;

  double kc_movement_reduction, kc_cast_speed_reduction;

  // Active Pet
  struct pets_t
  {
    pet_t* active;
    pet_t* last;
    pets::wild_imp_pet_t* wild_imps[ WILD_IMP_LIMIT ];
  } pets;

  // Buffs
  struct buffs_t
  {
    buff_t* backdraft;
    buff_t* dark_soul;
    buff_t* metamorphosis;
    buff_t* molten_core;
    buff_t* soulburn;
    buff_t* havoc;
    buff_t* grimoire_of_sacrifice;
    buff_t* demonic_calling;
    buff_t* fire_and_brimstone;
    buff_t* soul_swap;
    buff_t* archimondes_vengeance;
    buff_t* kiljaedens_cunning;
    buff_t* demonic_rebirth;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* demonic_calling;
    cooldown_t* infernal;
    cooldown_t* doomguard;
    cooldown_t* imp_swarm;
    cooldown_t* hand_of_guldan;
  } cooldowns;

  // Talents

  struct talents_t
  {
    const spell_data_t* soul_leech;
    const spell_data_t* harvest_life;
    const spell_data_t* mortal_coil;
    const spell_data_t* shadowfury;
    const spell_data_t* soul_link;
    const spell_data_t* grimoire_of_supremacy;
    const spell_data_t* grimoire_of_service;
    const spell_data_t* grimoire_of_sacrifice;
    const spell_data_t* archimondes_vengeance;
    const spell_data_t* kiljaedens_cunning;
  } talents;

  // Specialization Spells
  struct specs_t
  {
    // General
    const spell_data_t* dark_soul;
    const spell_data_t* nethermancy;
    const spell_data_t* fel_armor;
    const spell_data_t* pandemic;
    const spell_data_t* imp_swarm;

    // Affliction
    const spell_data_t* nightfall;

    // Demonology
    const spell_data_t* decimation;
    const spell_data_t* demonic_fury;
    const spell_data_t* metamorphosis;
    const spell_data_t* molten_core;
    const spell_data_t* doom;
    const spell_data_t* touch_of_chaos;
    const spell_data_t* chaos_wave;
    const spell_data_t* demonic_rebirth;
    const spell_data_t* wild_imps;

    // Destruction
    const spell_data_t* aftermath;
    const spell_data_t* backdraft;
    const spell_data_t* burning_embers;
    const spell_data_t* chaotic_energy;
    const spell_data_t* pyroclasm;

  } spec;

  struct mastery_spells_t
  {
    const spell_data_t* potent_afflictions;
    const spell_data_t* master_demonologist;
    const spell_data_t* emberstorm;
  } mastery_spells;

  // Gains
  struct gains_t
  {
    gain_t* life_tap;
    gain_t* soul_leech;
    gain_t* tier13_4pc;
    gain_t* nightfall;
    gain_t* drain_soul;
    gain_t* incinerate;
    gain_t* conflagrate;
    gain_t* rain_of_fire;
    gain_t* immolate;
    gain_t* fel_flame;
    gain_t* shadowburn;
    gain_t* miss_refund;
    gain_t* siphon_life;
    gain_t* seed_of_corruption;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* wild_imp;
  } procs;

  // Random Number Generators
  struct rngs_t
  {
    rng_t* demonic_calling;
    rng_t* molten_core;
    rng_t* nightfall;
    rng_t* ember_gain;
  } rngs;

  struct glyphs_t
  {
    const spell_data_t* conflagrate;
    const spell_data_t* dark_soul;
    const spell_data_t* demon_training;
    const spell_data_t* life_tap;
    const spell_data_t* imp_swarm;
    const spell_data_t* soul_shards;
    const spell_data_t* burning_embers;
    const spell_data_t* soul_swap;
    const spell_data_t* shadow_bolt;
    const spell_data_t* siphon_life;
    const spell_data_t* everlasting_affliction;
  } glyphs;

  struct spells_t
  {
    spell_t* seed_of_corruption_aoe;
    spell_t* soulburn_seed_of_corruption_aoe;
    spell_t* archimondes_vengeance_dmg;
    spell_t* metamorphosis;
    spell_t* melee;
  } spells;

  struct soul_swap_state_t
  {
    player_t* target;
    int agony;
    bool corruption;
    bool unstable_affliction;
    bool seed_of_corruption;
  } soul_swap_state;

  struct demonic_calling_event_t : event_t
  {
    bool initiator;

    demonic_calling_event_t( player_t* p, timespan_t delay, bool init = false ) :
      event_t( p, "demonic_calling" ), initiator( init )
    {
      sim -> add_event( this, delay );
    }

    virtual void execute()
    {
      warlock_t* p = static_cast<warlock_t*>( player );
      p -> demonic_calling_event = new ( sim ) demonic_calling_event_t( player,
          timespan_t::from_seconds( ( p -> spec.wild_imps -> effectN( 1 ).period().total_seconds() + p -> spec.imp_swarm -> effectN( 3 ).base_value() ) * p -> composite_spell_haste() ) );
      if ( ! initiator ) p -> buffs.demonic_calling -> trigger();
    }
  };

  demonic_calling_event_t* demonic_calling_event;

  int initial_burning_embers, initial_demonic_fury;
  std::string default_pet;

  timespan_t ember_react, shard_react;
  double nightfall_chance;

private:
  target_specific_t<warlock_td_t> target_data;
public:
  warlock_t( sim_t* sim, const std::string& name, race_e r = RACE_UNDEAD );

  // Character Definition
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      create_buffs();
  virtual void      init_gains();
  virtual void      init_benefits();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      init_resources( bool force );
  virtual void      reset();
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual bool      create_profile( std::string& profile_str, save_e=SAVE_ALL, bool save_html=false );
  virtual void      copy_from( player_t* source );
  virtual int       decode_set( item_t& );
  virtual resource_e primary_resource() { return RESOURCE_MANA; }
  virtual role_e primary_role()     { return ROLE_SPELL; }
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual double composite_player_multiplier( school_e school, action_t* a );
  virtual double composite_spell_crit();
  virtual double composite_spell_haste();
  virtual double composite_mastery();
  virtual double mana_regen_per_second();
  virtual double composite_armor();
  virtual double composite_movement_speed();
  virtual void moving();
  virtual void halt();
  virtual void combat_begin();
  virtual expr_t* create_expression( action_t* a, const std::string& name_str );
  virtual void assess_damage( school_e, dmg_e, action_state_t* );

  virtual warlock_td_t* get_target_data( player_t* target )
  {
    warlock_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new warlock_td_t( target, this );
      td -> init();
    }
    return td;
  }
};

warlock_td_t::warlock_td_t( player_t* target, warlock_t* p ) :
  actor_pair_t( target, p ),
  ds_started_below_20( false ),
  shadowflame_stack( 1 ),
  agony_stack( 1 ),
  soc_trigger( 0 ),
  soulburn_soc_trigger( 0 )
{
  dots_corruption          = target -> get_dot( "corruption", p );
  dots_unstable_affliction = target -> get_dot( "unstable_affliction", p );
  dots_agony               = target -> get_dot( "agony", p );
  dots_doom                = target -> get_dot( "doom", p );
  dots_immolate            = target -> get_dot( "immolate", p );
  dots_drain_soul          = target -> get_dot( "drain_soul", p );
  dots_shadowflame         = target -> get_dot( "shadowflame", p );
  dots_malefic_grasp       = target -> get_dot( "malefic_grasp", p );
  dots_seed_of_corruption  = target -> get_dot( "seed_of_corruption", p );
  dots_soulburn_seed_of_corruption  = target -> get_dot( "soulburn_seed_of_corruption", p );
  dots_haunt               = target -> get_dot( "haunt", p );

  debuffs_haunt = buff_creator_t( *this, "haunt", source -> find_class_spell( "haunt" ) );
}

warlock_t::warlock_t( sim_t* sim, const std::string& name, race_e r ) :
  player_t( sim, WARLOCK, name, r ),
  havoc_target( 0 ),
  kc_movement_reduction(),
  kc_cast_speed_reduction(),
  pets( pets_t() ),
  buffs( buffs_t() ),
  cooldowns( cooldowns_t() ),
  talents( talents_t() ),
  mastery_spells( mastery_spells_t() ),
  gains( gains_t() ),
  procs( procs_t() ),
  rngs( rngs_t() ),
  glyphs( glyphs_t() ),
  spells( spells_t() ),
  demonic_calling_event( 0 ),
  initial_burning_embers( 1 ),
  initial_demonic_fury( 200 ),
  default_pet( "" ),
  ember_react( ( initial_burning_embers >= 1.0 ) ? timespan_t::zero() : timespan_t::max() ),
  shard_react( timespan_t::zero() ),
  nightfall_chance( 0 )
{
  target_data.init( "target_data", this );

  current.distance = 40;
  initial.distance = 40;

  cooldowns.infernal       = get_cooldown ( "summon_infernal" );
  cooldowns.doomguard      = get_cooldown ( "summon_doomguard" );
  cooldowns.imp_swarm      = get_cooldown ( "imp_swarm" );
  cooldowns.hand_of_guldan = get_cooldown ( "hand_of_guldan" );
}

void parse_spell_coefficient( action_t& a )
{
  for ( size_t i = 1; i <= a.data()._effects -> size(); i++ )
  {
    if ( a.data().effectN( i ).type() == E_SCHOOL_DAMAGE )
      a.direct_power_mod = a.data().effectN( i ).m_average();
    else if ( a.data().effectN( i ).type() == E_APPLY_AURA && a.data().effectN( i ).subtype() == A_PERIODIC_DAMAGE )
      a.tick_power_mod = a.data().effectN( i ).m_average();
  }
}

namespace pets {
// PETS

struct warlock_pet_t : public pet_t
{
  gain_t* owner_fury_gain;
  action_t* special_action;
  melee_attack_t* melee_attack;
  stats_t* summon_stats;
  const spell_data_t* supremacy;

  warlock_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_e pt, bool guardian = false );
  virtual bool ooc_buffs() { return true; }
  virtual void init_base();
  virtual void init_actions();
  virtual void init_spell();
  virtual void init_attack();
  virtual timespan_t available();
  virtual void schedule_ready( timespan_t delta_time=timespan_t::zero(),
                               bool   waiting=false );
  virtual double composite_player_multiplier( school_e school, action_t* a );
  virtual resource_e primary_resource() { return RESOURCE_ENERGY; }
  warlock_t* o() const
  { return static_cast<warlock_t*>( owner ); }
};


namespace { // ANONYMOUS_NAMESPACE

double get_fury_gain( const spell_data_t& data )
{
  if ( data._effects -> size() >= 3 && data.effectN( 3 ).trigger_spell_id() == 104330 )
    return data.effectN( 3 ).base_value();
  else
    return 0;
}


struct warlock_pet_melee_t : public melee_attack_t
{
  struct off_hand_swing : public melee_attack_t
  {
    off_hand_swing( warlock_pet_t* p, const char* name = "melee_oh" ) :
      melee_attack_t( name, p, spell_data_t::nil() )
    {
      school = SCHOOL_PHYSICAL;
      weapon = &( p -> off_hand_weapon );
      base_execute_time = weapon -> swing_time;
      may_crit    = true;
      background  = true;
      base_multiplier = 0.5;
    }
  };

  off_hand_swing* oh;

  warlock_pet_melee_t( warlock_pet_t* p, const char* name = "melee" ) :
    melee_attack_t( name, p, spell_data_t::nil() ), oh( 0 )
  {
    school = SCHOOL_PHYSICAL;
    weapon = &( p -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    may_crit    = true;
    background  = true;
    repeating   = true;

    if ( p -> dual_wield() )
      oh = new off_hand_swing( p );
  }

  virtual void execute()
  {
    if ( ! player -> executing && ! player -> channeling )
    {
      melee_attack_t::execute();
      if ( oh )
      {
        oh -> time_to_execute = time_to_execute;
        oh -> execute();
      }
    }
    else
    {
      schedule_execute();
    }
  }
};


struct warlock_pet_melee_attack_t : public melee_attack_t
{
private:
  void _init_warlock_pet_melee_attack_t()
  {
    weapon = &( player -> main_hand_weapon );
    may_crit   = true;
    special = true;
    generate_fury = get_fury_gain( data() );
  }

public:
  double generate_fury;

  warlock_pet_melee_attack_t( warlock_pet_t* p, const std::string& n ) :
    melee_attack_t( n, p, p -> find_pet_spell( n ) )
  {
    _init_warlock_pet_melee_attack_t();
  }

  warlock_pet_melee_attack_t( const std::string& token, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    melee_attack_t( token, p, s )
  {
    _init_warlock_pet_melee_attack_t();
  }

  warlock_pet_t* p()
  { return static_cast<warlock_pet_t*>( player ); }

  virtual bool ready()
  {
    if ( background == false && current_resource() == RESOURCE_ENERGY && player -> resources.current[ RESOURCE_ENERGY ] < 130 )
      return false;

    return melee_attack_t::ready();
  }

  virtual void execute()
  {
    melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) && p() -> o() -> specialization() == WARLOCK_DEMONOLOGY && generate_fury > 0 )
      p() -> o() -> resource_gain( RESOURCE_DEMONIC_FURY, generate_fury, p() -> owner_fury_gain );
  }
};


struct warlock_pet_spell_t : public spell_t
{
private:
  void _init_warlock_pet_spell_t()
  {
    may_crit = true;
    generate_fury = get_fury_gain( data() );

    parse_spell_coefficient( *this );
  }

public:
  double generate_fury;

  warlock_pet_spell_t( warlock_pet_t* p, const std::string& n ) :
    spell_t( n, p, p -> find_pet_spell( n ) )
  {
    _init_warlock_pet_spell_t();
  }

  warlock_pet_spell_t( const std::string& token, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( token, p, s )
  {
    _init_warlock_pet_spell_t();
  }

  warlock_pet_t* p()
  { return static_cast<warlock_pet_t*>( player ); }

  virtual bool ready()
  {
    if ( background == false && current_resource() == RESOURCE_ENERGY && player -> resources.current[ RESOURCE_ENERGY ] < 130 )
      return false;

    return spell_t::ready();
  }

  virtual void execute()
  {
    spell_t::execute();

    if ( result_is_hit( execute_state -> result ) && p() -> o() -> specialization() == WARLOCK_DEMONOLOGY && generate_fury > 0 )
      p() -> o() -> resource_gain( RESOURCE_DEMONIC_FURY, generate_fury, p() -> owner_fury_gain );
  }
};


struct firebolt_t : public warlock_pet_spell_t
{
  firebolt_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( p, "Firebolt" )
  {
    if ( p -> owner -> bugs )
      min_gcd = timespan_t::from_seconds( 1.5 );
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = warlock_pet_spell_t::execute_time();

    if ( p() -> o() -> glyphs.demon_training -> ok() ) t *= 0.5;

    return t;
  }
};


struct legion_strike_t : public warlock_pet_melee_attack_t
{
  legion_strike_t( warlock_pet_t* p ) :
    warlock_pet_melee_attack_t( p, "Legion Strike" )
  {
    aoe              = -1;
    split_aoe_damage = true;
    weapon           = &( p -> main_hand_weapon );
  }

  virtual bool ready()
  {
    if ( p() -> special_action -> get_dot() -> ticking ) return false;

    return warlock_pet_melee_attack_t::ready();
  }
};


struct felstorm_tick_t : public warlock_pet_melee_attack_t
{
  felstorm_tick_t( warlock_pet_t* p, const spell_data_t& s ) :
    warlock_pet_melee_attack_t( "felstorm_tick", p, s.effectN( 1 ).trigger() )
  {
    aoe         = -1;
    background  = true;
    weapon = &( p -> main_hand_weapon );
  }
};


struct felstorm_t : public warlock_pet_melee_attack_t
{
  felstorm_t( warlock_pet_t* p ) :
    warlock_pet_melee_attack_t( "felstorm", p, p -> find_spell( 89751 ) )
  {
    tick_zero = true;
    hasted_ticks = false;
    may_miss = false;
    may_crit = false;
    weapon_multiplier = 0;

    dynamic_tick_action = true;
    tick_action = new felstorm_tick_t( p, data() );
  }

  virtual void cancel()
  {
    warlock_pet_melee_attack_t::cancel();

    get_dot() -> cancel();
  }

  virtual void execute()
  {
    warlock_pet_melee_attack_t::execute();

    p() -> melee_attack -> cancel();
  }

  virtual void last_tick( dot_t* d )
  {
    warlock_pet_melee_attack_t::last_tick( d );

    if ( ! p() -> current.sleeping ) p() -> melee_attack -> execute();
  }
};


struct shadow_bite_t : public warlock_pet_spell_t
{
  shadow_bite_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( p, "Shadow Bite" )
  { }
};


struct lash_of_pain_t : public warlock_pet_spell_t
{
  lash_of_pain_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( p, "Lash of Pain" )
  {
    if ( p -> owner -> bugs ) min_gcd = timespan_t::from_seconds( 1.5 );
  }
};


struct whiplash_t : public warlock_pet_spell_t
{
  whiplash_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( p, "Whiplash" )
  {
    aoe = -1;
  }
};


struct torment_t : public warlock_pet_spell_t
{
  torment_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( p, "Torment" )
  { }
};


struct felbolt_t : public warlock_pet_spell_t
{
  felbolt_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( p, "Felbolt" )
  {
    if ( p -> owner -> bugs )
      min_gcd = timespan_t::from_seconds( 1.5 );
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = warlock_pet_spell_t::execute_time();

    if ( p() -> o() -> glyphs.demon_training -> ok() ) t *= 0.5;

    return t;
  }
};


struct mortal_cleave_t : public warlock_pet_melee_attack_t
{
  mortal_cleave_t( warlock_pet_t* p ) :
    warlock_pet_melee_attack_t( p, "Mortal Cleave" )
  {
    aoe = -1;
    weapon = &( p -> main_hand_weapon );
  }

  virtual bool ready()
  {
    if ( p() -> special_action -> get_dot() -> ticking ) return false;

    return warlock_pet_melee_attack_t::ready();
  }
};


struct wrathstorm_tick_t : public warlock_pet_melee_attack_t
{
  wrathstorm_tick_t( warlock_pet_t* p, const spell_data_t& s ) :
    warlock_pet_melee_attack_t( "wrathstorm_tick", p, s.effectN( 1 ).trigger() )
  {
    aoe         = -1;
    background  = true;
    weapon = &( p -> main_hand_weapon );
  }
};


struct wrathstorm_t : public warlock_pet_melee_attack_t
{
  wrathstorm_t( warlock_pet_t* p ) :
    warlock_pet_melee_attack_t( "wrathstorm", p, p -> find_spell( 115831 ) )
  {
    tick_zero = true;
    hasted_ticks = false;
    may_miss = false;
    may_crit = false;
    weapon_multiplier = 0;

    dynamic_tick_action = true;
    tick_action = new wrathstorm_tick_t( p, data() );
  }

  virtual void cancel()
  {
    warlock_pet_melee_attack_t::cancel();

    get_dot() -> cancel();
  }

  virtual void execute()
  {
    warlock_pet_melee_attack_t::execute();

    p() -> melee_attack -> cancel();
  }

  virtual void last_tick( dot_t* d )
  {
    warlock_pet_melee_attack_t::last_tick( d );

    if ( ! p() -> current.sleeping ) p() -> melee_attack -> execute();
  }
};


struct tongue_lash_t : public warlock_pet_spell_t
{
  tongue_lash_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( p, "Tongue Lash" )
  { }
};


struct bladedance_t : public warlock_pet_spell_t
{
  bladedance_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( p, "Bladedance" )
  {
    if ( p -> owner -> bugs ) min_gcd = timespan_t::from_seconds( 1.5 );
  }
};


struct fellash_t : public warlock_pet_spell_t
{
  fellash_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( p, "Fellash" )
  {
    aoe = -1;
  }
};


struct immolation_tick_t : public warlock_pet_spell_t
{
  immolation_tick_t( warlock_pet_t* p, const spell_data_t& s ) :
    warlock_pet_spell_t( "immolation_tick", p, s.effectN( 1 ).trigger() )
  {
    aoe         = -1;
    background  = true;
    may_crit    = false;
  }
};

struct immolation_t : public warlock_pet_spell_t
{
  immolation_t( warlock_pet_t* p, const std::string& options_str ) :
    warlock_pet_spell_t( "immolation", p, p -> find_spell( 19483 ) )
  {
    parse_options( 0, options_str );

    num_ticks    = 1;
    hasted_ticks = false;

    dynamic_tick_action = true;
    tick_action = new immolation_tick_t( p, data() );
  }

  virtual void tick( dot_t* d )
  {
    d -> current_tick = 0; // ticks indefinitely

    warlock_pet_spell_t::tick( d );
  }

  virtual void cancel()
  {
    dot_t* dot = find_dot();
    if ( dot ) dot -> reset();
    action_t::cancel();
  }
};


struct doom_bolt_t : public warlock_pet_spell_t
{
  doom_bolt_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( p, "Doom Bolt" )
  {
  }

  virtual timespan_t execute_time()
  {
    // FIXME: Not actually how it works, but this achieves a consistent 17 casts per summon, which seems to match reality
    return timespan_t::from_seconds( 3.4 );
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    double m = warlock_pet_spell_t::composite_target_multiplier( target );

    if ( target -> health_percentage() < 20 )
    {
      m *= 1.0 + data().effectN( 2 ).percent();
    }

    return m;
  }
};


struct wild_firebolt_t : public warlock_pet_spell_t
{
  wild_firebolt_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( "firebolt", p, p -> find_spell( 104318 ) )
  {
    // FIXME: Exact casting mechanics need testing - this is copied from the old doomguard lag
    if ( p -> owner -> bugs )
    {
      ability_lag = timespan_t::from_seconds( 0.22 );
      ability_lag_stddev = timespan_t::from_seconds( 0.01 );
    }
  }

  virtual double action_multiplier()
  {
    double m = warlock_pet_spell_t::action_multiplier();

    m *= 1.0 + p() -> o() -> talents.grimoire_of_sacrifice -> effectN( 4 ).percent() * p() -> o() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual void impact( action_state_t* s )
  {
    warlock_pet_spell_t::impact( s );

    if ( result_is_hit( s -> result )
         && p() -> o() -> spec.molten_core -> ok()
         && p() -> o() -> rngs.molten_core -> roll( 0.08 ) )
      p() -> o() -> buffs.molten_core -> trigger();


    if ( player -> resources.current[ RESOURCE_ENERGY ] == 0 )
    {
      static_cast<warlock_pet_t*>( player ) -> dismiss();
      return;
    }
  }

  virtual bool ready()
  {
    return spell_t::ready();
  }
};

}

warlock_pet_t::warlock_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_e pt, bool guardian ) :
  pet_t( sim, owner, pet_name, pt, guardian ), special_action( 0 ), melee_attack( 0 ), summon_stats( 0 )
{
  owner_fury_gain = owner -> get_gain( pet_name );
  owner_coeff.ap_from_sp = 3.5;
  owner_coeff.sp_from_sp = 1.0;
  supremacy = find_spell( 115578 );
}

void warlock_pet_t::init_base()
{
  pet_t::init_base();

  resources.base[ RESOURCE_ENERGY ] = 200;
  base_energy_regen_per_second = 10;

  // We only care about intellect - no other primary attribute affects anything interesting
  base.attribute[ ATTR_INTELLECT ] = util::ability_rank( owner -> level, 74, 90, 72, 89, 71, 88, 70, 87, 69, 85, 0, 1 );
  // We don't have the data below level 85, so let's make a rough estimate
  if ( base.attribute[ ATTR_INTELLECT ] == 0 ) base.attribute[ ATTR_INTELLECT ] = floor( 0.81 * owner -> level );

  initial.spell_power_per_intellect = 1;

  intellect_per_owner = 0;
  stamina_per_owner = 0;

  main_hand_weapon.type = WEAPON_BEAST;

  double dmg = dbc.spell_scaling( owner -> type, owner -> level );
  if ( owner -> race == RACE_ORC ) dmg *= 1.0 + owner -> find_spell( 20575 ) -> effectN( 1 ).percent();
  main_hand_weapon.min_dmg = main_hand_weapon.max_dmg = main_hand_weapon.damage = dmg;

  main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
}

void warlock_pet_t::init_actions()
{
  if ( special_action )
  {
    if ( type == PLAYER_PET )
      special_action -> background = true;
    else
      special_action -> action_list = "default";
  }

  pet_t::init_actions();

  if ( summon_stats )
    for ( size_t i = 0; i < action_list.size(); ++i )
      summon_stats -> add_child( action_list[ i ] -> stats );
}

void warlock_pet_t::init_spell()
{
  pet_t::init_spell();
  if ( owner -> race == RACE_ORC ) initial.spell_power_multiplier *= 1.0 + owner -> find_spell( 20575 ) -> effectN( 1 ).percent();
}

void warlock_pet_t::init_attack()
{
  pet_t::init_attack();
  if ( owner -> race == RACE_ORC ) initial.attack_power_multiplier *= 1.0 + owner -> find_spell( 20575 ) -> effectN( 1 ).percent();
}

timespan_t warlock_pet_t::available()
{
  assert( primary_resource() == RESOURCE_ENERGY );
  double energy = resources.current[ RESOURCE_ENERGY ];

  if ( energy >= 130 )
    return timespan_t::from_seconds( 0.1 );

  return std::max(
    timespan_t::from_seconds( ( 130 - energy ) / energy_regen_per_second() ),
    timespan_t::from_seconds( 0.1 )
  );
}

void warlock_pet_t::schedule_ready( timespan_t delta_time, bool waiting )
{
  dot_t* d;
  if ( melee_attack && ! melee_attack -> execute_event && ! ( special_action && ( d = special_action -> get_dot() ) && d -> ticking ) )
  {
    melee_attack -> schedule_execute();
  }

  pet_t::schedule_ready( delta_time, waiting );
}

double warlock_pet_t::composite_player_multiplier( school_e school, action_t* a )
{
  double m = pet_t::composite_player_multiplier( school, a );

  m *= 1.0 + owner -> composite_mastery() * o() -> mastery_spells.master_demonologist -> effectN( 1 ).mastery_value();

  if ( o() -> talents.grimoire_of_supremacy -> ok() && pet_type != PET_WILD_IMP )
    m *= 1.0 + supremacy -> effectN( 1 ).percent(); // The relevant effect is not attatched to the talent spell, weirdly enough

  return m;
}


struct imp_pet_t : public warlock_pet_t
{
  imp_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "imp" ) :
    warlock_pet_t( sim, owner, name, PET_IMP, name != "imp" )
  {
    action_list_str = "firebolt";
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "firebolt" ) return new firebolt_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};


struct felguard_pet_t : public warlock_pet_t
{
  felguard_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "felguard" ) :
    warlock_pet_t( sim, owner, name, PET_FELGUARD, name != "felguard" )
  {
    action_list_str = "legion_strike";
  }

  virtual void init_base()
  {
    warlock_pet_t::init_base();

    melee_attack = new warlock_pet_melee_t( this );
    special_action = new felstorm_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "legion_strike"   ) return new legion_strike_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};


struct felhunter_pet_t : public warlock_pet_t
{
  felhunter_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "felhunter" ) :
    warlock_pet_t( sim, owner, name, PET_FELHUNTER, name != "felhunter" )
  {
    action_list_str = "shadow_bite";
  }

  virtual void init_base()
  {
    warlock_pet_t::init_base();

    melee_attack = new warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "shadow_bite" ) return new shadow_bite_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};


struct succubus_pet_t : public warlock_pet_t
{
  succubus_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "succubus" ) :
    warlock_pet_t( sim, owner, name, PET_SUCCUBUS, name != "succubus" )
  {
    action_list_str = "lash_of_pain";
    owner_coeff.ap_from_sp = 1.667;
  }

  virtual void init_base()
  {
    warlock_pet_t::init_base();

    main_hand_weapon.swing_time = timespan_t::from_seconds( 3.0 );
    melee_attack = new warlock_pet_melee_t( this );
    special_action = new whiplash_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "lash_of_pain" ) return new lash_of_pain_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};


struct voidwalker_pet_t : public warlock_pet_t
{
  voidwalker_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "voidwalker" ) :
    warlock_pet_t( sim, owner, name, PET_VOIDWALKER, name != "voidwalker" )
  {
    action_list_str = "torment";
  }

  virtual void init_base()
  {
    warlock_pet_t::init_base();

    melee_attack = new warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "torment" ) return new torment_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};


struct infernal_pet_t : public warlock_pet_t
{
  infernal_pet_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "infernal", PET_INFERNAL, true )
  {
    action_list_str = "immolation,if=!ticking";
    owner_coeff.ap_from_sp = 0.5;
  }

  virtual void init_base()
  {
    warlock_pet_t::init_base();

    melee_attack = new warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "immolation" ) return new immolation_t( this, options_str );

    return warlock_pet_t::create_action( name, options_str );
  }
};


struct doomguard_pet_t : public warlock_pet_t
{
  doomguard_pet_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "doomguard", PET_DOOMGUARD, true )
  { }

  virtual void init_base()
  {
    warlock_pet_t::init_base();

    action_list_str = "doom_bolt";
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "doom_bolt" ) return new doom_bolt_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};


struct wild_imp_pet_t : public warlock_pet_t
{
  stats_t** firebolt_stats;
  stats_t* regular_stats;
  stats_t* swarm_stats;

  wild_imp_pet_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "wild_imp", PET_WILD_IMP, true ), firebolt_stats( 0 )
  {
    if ( owner -> pets.wild_imps[ 0 ] )
      regular_stats = owner -> pets.wild_imps[ 0 ] -> get_stats( "firebolt" );
    else
      regular_stats = get_stats( "firebolt" );

    swarm_stats = owner -> get_stats( "firebolt" );
  }

  virtual void init_base()
  {
    warlock_pet_t::init_base();

    action_list_str = "firebolt";

    resources.base[ RESOURCE_ENERGY ] = 10;
    base_energy_regen_per_second = 0;
  }

  virtual timespan_t available()
  {
    return timespan_t::from_seconds( 0.1 );
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "firebolt" )
    {
      action_t* a = new wild_firebolt_t( this );
      firebolt_stats = &( a -> stats );
      return a;
    }

    return warlock_pet_t::create_action( name, options_str );
  }

  void trigger( bool swarm = false )
  {
    if ( swarm )
      *firebolt_stats = swarm_stats;
    else
      *firebolt_stats = regular_stats;

    summon();
  }

  virtual void pre_analyze_hook()
  {
    warlock_pet_t::pre_analyze_hook();

    *firebolt_stats = regular_stats;

    if ( this == o() -> pets.wild_imps[ 0 ] )
    {
      ( *firebolt_stats ) -> merge( *swarm_stats );
      swarm_stats -> quiet = 1;
    }
  }
};


struct fel_imp_pet_t : public warlock_pet_t
{
  fel_imp_pet_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "fel_imp", PET_IMP )
  {
    action_list_str = "felbolt";
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "felbolt" ) return new felbolt_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};


struct wrathguard_pet_t : public warlock_pet_t
{
  wrathguard_pet_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "wrathguard", PET_FELGUARD )
  {
    action_list_str = "mortal_cleave";
    owner_coeff.ap_from_sp = 2.333; // FIXME: Retest this later
  }

  virtual void init_base()
  {
    warlock_pet_t::init_base();

    main_hand_weapon.min_dmg = main_hand_weapon.max_dmg = main_hand_weapon.damage = main_hand_weapon.damage * 0.667; // FIXME: Retest this later
    off_hand_weapon = main_hand_weapon;

    melee_attack = new warlock_pet_melee_t( this );
    special_action = new wrathstorm_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "mortal_cleave" ) return new mortal_cleave_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};


struct observer_pet_t : public warlock_pet_t
{
  observer_pet_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "observer", PET_FELHUNTER )
  {
    action_list_str = "tongue_lash";
  }

  virtual void init_base()
  {
    warlock_pet_t::init_base();

    melee_attack = new warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "tongue_lash" ) return new tongue_lash_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};


struct shivarra_pet_t : public warlock_pet_t
{
  shivarra_pet_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "shivarra", PET_SUCCUBUS )
  {
    action_list_str = "bladedance";
    owner_coeff.ap_from_sp = 1.111;
  }

  virtual void init_base()
  {
    warlock_pet_t::init_base();

    main_hand_weapon.swing_time = timespan_t::from_seconds( 3.0 );
    main_hand_weapon.min_dmg = main_hand_weapon.max_dmg = main_hand_weapon.damage = main_hand_weapon.damage * 0.667;
    off_hand_weapon = main_hand_weapon;

    melee_attack = new warlock_pet_melee_t( this );
    special_action = new fellash_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "bladedance" ) return new bladedance_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};


struct voidlord_pet_t : public warlock_pet_t
{
  voidlord_pet_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "voidlord", PET_VOIDWALKER )
  {
    action_list_str = "torment";
  }

  virtual void init_base()
  {
    warlock_pet_t::init_base();

    melee_attack = new warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "torment" ) return new torment_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};


struct abyssal_pet_t : public warlock_pet_t
{
  abyssal_pet_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "abyssal", PET_INFERNAL, true )
  {
    action_list_str = "immolation,if=!ticking";
    owner_coeff.ap_from_sp = 0.5;
  }

  virtual void init_base()
  {
    warlock_pet_t::init_base();

    melee_attack = new warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "immolation" ) return new immolation_t( this, options_str );

    return warlock_pet_t::create_action( name, options_str );
  }
};


struct terrorguard_pet_t : public warlock_pet_t
{
  terrorguard_pet_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "terrorguard", PET_DOOMGUARD, true )
  {
    action_list_str = "doom_bolt";
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "doom_bolt" ) return new doom_bolt_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

} // end namespace pets

// SPELLS

namespace { // UNNAMED NAMESPACE ==========================================

struct warlock_heal_t : public heal_t
{
  warlock_heal_t( const std::string& n, warlock_t* p, const uint32_t id ) :
    heal_t( n, p, p -> find_spell( id ) )
  {
    target = p;
  }

  warlock_t* p()
  { return static_cast<warlock_t*>( player ); }
};


struct warlock_spell_t : public spell_t
{
private:
  void _init_warlock_spell_t()
  {
    may_crit      = true;
    tick_may_crit = true;
    dot_behavior  = DOT_REFRESH;
    weapon_multiplier = 0.0;
    gain = player -> get_gain( name_str );
    generate_fury = 0;
    cost_event = 0;
    havoc_override = false;
    fury_in_meta = data().powerN( POWER_DEMONIC_FURY ).aura_id() == 54879;
    ds_tick_stats = player -> get_stats( name_str + "_ds", this );
    ds_tick_stats -> school = school;
    mg_tick_stats = player -> get_stats( name_str + "_mg", this );
    mg_tick_stats -> school = school;

    parse_spell_coefficient( *this );
  }

public:
  double generate_fury;
  gain_t* gain;
  bool havoc_override;
  bool fury_in_meta;
  stats_t* ds_tick_stats;
  stats_t* mg_tick_stats;
  std::vector< player_t* > havoc_targets;

  struct cost_event_t : event_t
  {
    warlock_spell_t* spell;
    resource_e resource;

    cost_event_t( player_t* p, warlock_spell_t* s, resource_e r = RESOURCE_NONE ) :
      event_t( p, "cost_event" ), spell( s ), resource( r )
    {
      if ( resource == RESOURCE_NONE ) resource = spell -> current_resource();
      sim -> add_event( this, timespan_t::from_seconds( 1 ) );
    }

    virtual void execute()
    {
      spell -> cost_event = new ( sim ) cost_event_t( player, spell, resource );
      player -> resource_loss( resource, spell -> costs_per_second[ resource ], spell -> gain );
    }
  };

  cost_event_t* cost_event;

  warlock_spell_t( warlock_t* p, const std::string& n ) :
    spell_t( n, p, p -> find_class_spell( n ) )
  {
    _init_warlock_spell_t();
  }

  warlock_spell_t( const std::string& token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( token, p, s )
  {
    _init_warlock_spell_t();
  }

  warlock_t* p() const { return static_cast<warlock_t*>( player ); }

  warlock_td_t* td( player_t* t ) { return p() -> get_target_data( t ? t : target ); }

  virtual void init()
  {
    spell_t::init();

    if ( harmful && ! tick_action ) trigger_gcd += p() -> spec.chaotic_energy -> effectN( 3 ).time_value();
  }

  virtual void reset()
  {
    spell_t::reset();

    havoc_override = false;
    event_t::cancel( cost_event );
  }

  virtual std::vector< player_t* >& target_list()
  {
    if ( aoe == 2 && p() -> buffs.havoc -> check() && target != p() -> havoc_target )
    {
      std::vector< player_t* >& targets = spell_t::target_list();

      havoc_targets.clear();
      for ( size_t i = 0; i < targets.size(); i++ )
      {
        if ( targets[ i ] == target || targets[ i ] == p() -> havoc_target )
          havoc_targets.push_back( targets[ i ] );
      }
      return havoc_targets;
    }
    else
      return spell_t::target_list();
  }

  virtual void execute()
  {
    bool havoc = false;
    if ( harmful && ! background && aoe == 0 && ! tick_action && p() -> buffs.havoc -> up() && p() -> havoc_target != target )
    {
      aoe = 2;
      havoc = true;
    }

    spell_t::execute();

    if ( result_is_hit( execute_state -> result ) && p() -> specialization() == WARLOCK_DEMONOLOGY
         && generate_fury > 0 && ! p() -> buffs.metamorphosis -> check() )
      p() -> resource_gain( RESOURCE_DEMONIC_FURY, generate_fury, gain );

    if ( havoc )
    {
      p() -> buffs.havoc -> decrement();
      aoe = 0;
    }
  }

  virtual bool ready()
  {
    if ( p() -> buffs.metamorphosis -> check() && p() -> resources.current[ RESOURCE_DEMONIC_FURY ] < META_FURY_MINIMUM )
      p() -> spells.metamorphosis -> cancel();

    return spell_t::ready();
  }

  virtual void tick( dot_t* d )
  {
    spell_t::tick( d );

    if ( p() -> specialization() == WARLOCK_DEMONOLOGY && generate_fury > 0 )
      p() -> resource_gain( RESOURCE_DEMONIC_FURY, generate_fury, gain );

    trigger_seed_of_corruption( td( d -> state -> target ), p(), d -> state -> result_amount );
  }

  virtual void impact( action_state_t* s )
  {
    spell_t::impact( s );

    trigger_seed_of_corruption( td( s -> target ), p(), s -> result_amount );
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double m = 1.0;

    if ( td( t ) -> debuffs_haunt -> up() )
    {
      m *= 1.0 + td( t ) -> debuffs_haunt -> data().effectN( 3 ).percent();
    }

    return spell_t::composite_target_multiplier( t ) * m;
  }

  virtual resource_e current_resource()
  {
    if ( fury_in_meta && p() -> buffs.metamorphosis -> data().ok() )
    {
      if ( p() -> buffs.metamorphosis -> check() )
        return RESOURCE_DEMONIC_FURY;
      else
        return RESOURCE_MANA;
    }
    else
      return spell_t::current_resource();
  }

  virtual timespan_t tick_time( double haste )
  {
    timespan_t t = spell_t::tick_time( haste );

    // FIXME: Find out if this adjusts mid-tick as well, if so we'll have to check the duration on the movement buff
    if ( channeled && p() -> is_moving() && p() -> talents.kiljaedens_cunning -> ok() &&
         ! p() -> buffs.kiljaedens_cunning -> up() && p() -> buffs.kiljaedens_cunning -> cooldown -> up() )
      t *= ( 1.0 + p() -> kc_cast_speed_reduction );

    return t;
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = spell_t::execute_time();

    // FIXME: Find out if this adjusts mid-cast as well, if so we'll have to check the duration on the movement buff
    if ( p() -> is_moving() && p() -> talents.kiljaedens_cunning -> ok() &&
         ! p() -> buffs.kiljaedens_cunning -> up() && p() -> buffs.kiljaedens_cunning -> cooldown -> up() )
      t *= ( 1.0 + p() -> kc_cast_speed_reduction );

    return t;
  }

  virtual bool usable_moving()
  {
    bool um = spell_t::usable_moving();

    if ( p() -> talents.kiljaedens_cunning -> ok() &&
         ( p() -> buffs.kiljaedens_cunning -> up() || p() -> buffs.kiljaedens_cunning -> cooldown -> up() ) )
      um = true;

    return um;
  }

  void trigger_seed_of_corruption( warlock_td_t* td, warlock_t* p, double amount )
  {
    if ( ( ( td -> dots_seed_of_corruption -> current_action && id == td -> dots_seed_of_corruption -> current_action -> id )
           || td -> dots_seed_of_corruption -> ticking ) && td -> soc_trigger > 0 )
    {
      td -> soc_trigger -= amount;
      if ( td -> soc_trigger <= 0 )
      {
        p -> spells.seed_of_corruption_aoe -> execute();
        td -> dots_seed_of_corruption -> cancel();
      }
    }
    if ( ( ( td -> dots_soulburn_seed_of_corruption -> current_action && id == td -> dots_soulburn_seed_of_corruption -> current_action -> id )
           || td -> dots_soulburn_seed_of_corruption -> ticking ) && td -> soulburn_soc_trigger > 0 )
    {
      td -> soulburn_soc_trigger -= amount;
      if ( td -> soulburn_soc_trigger <= 0 )
      {
        p -> spells.soulburn_seed_of_corruption_aoe -> execute();
        td -> dots_soulburn_seed_of_corruption -> cancel();
      }
    }
  }

  void consume_tick_resource( dot_t* d )
  {
    resource_e r = current_resource();
    resource_consumed = costs_per_second[ r ] * base_tick_time.total_seconds();

    player -> resource_loss( r, resource_consumed, 0, this );

    if ( sim -> log )
      sim -> output( "%s consumes %.1f %s for %s tick (%.0f)", player -> name(),
                     resource_consumed, util::resource_type_string( r ),
                     name(), player -> resources.current[ r ] );

    stats -> consume_resource( r, resource_consumed );

    if ( player -> resources.current[ r ] < resource_consumed )
    {
      if ( r == RESOURCE_DEMONIC_FURY && p() -> buffs.metamorphosis -> check() )
        p() -> spells.metamorphosis -> cancel();
      else
        d -> current_action -> cancel();
    }
  }

  void trigger_extra_tick( dot_t* dot, double multiplier )
  {
    if ( dot -> ticking )
    {
      assert( multiplier != 0.0 );
      dot -> state -> ta_multiplier *= multiplier;
      dot -> current_action -> periodic_hit = true;
      stats_t* tmp = dot -> current_action -> stats;
      if ( multiplier > 0.9 )
        dot -> current_action -> stats = ( ( warlock_spell_t* ) dot -> current_action ) -> ds_tick_stats;
      else
        dot -> current_action -> stats = ( ( warlock_spell_t* ) dot -> current_action ) -> mg_tick_stats;
      dot -> current_action -> tick( dot );
      dot -> current_action -> stats -> add_execute( timespan_t::zero() );
      dot -> current_action -> stats = tmp;
      dot -> current_action -> periodic_hit = false;
      dot -> state -> ta_multiplier /= multiplier;
    }
  }

  void extend_dot( dot_t* dot, int ticks )
  {
    if ( dot -> ticking )
    {
      //FIXME: This is roughly how it works, but we need more testing
      int max_ticks = ( int ) util::ceil( dot -> current_action -> hasted_num_ticks( p() -> composite_spell_haste() ) * 1.5 );
      int extend_ticks = std::min( ticks, max_ticks - dot -> ticks() );
      if ( extend_ticks > 0 ) dot -> extend_duration( extend_ticks );
    }
  }

  static void trigger_ember_gain( warlock_t* p, double amount, gain_t* gain, double chance = 1.0 )
  {
    if ( ! p -> rngs.ember_gain -> roll( chance ) ) return;

    p -> resource_gain( RESOURCE_BURNING_EMBER, amount, gain );

    // If getting to 1 full ember was a surprise, the player would have to react to it
    if ( p -> resources.current[ RESOURCE_BURNING_EMBER ] == 1.0 && ( amount > 0.1 || chance < 1.0 ) )
      p -> ember_react = p -> sim -> current_time + p -> total_reaction_time();
    else if ( p -> resources.current[ RESOURCE_BURNING_EMBER ] >= 1.0 )
      p -> ember_react = p -> sim -> current_time;
    else
      p -> ember_react = timespan_t::max();
  }

  static void refund_embers( warlock_t* p )
  {
    double refund = ceil( ( p -> resources.current[ RESOURCE_BURNING_EMBER ] + 1.0 ) / 4.0 );

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
        p -> pets.wild_imps[ i ] -> trigger();
        p -> procs.wild_imp -> occur();
        return;
      }
    }
    p -> sim -> errorf( "Player %s ran out of wild imps.\n", p -> name() );
    assert( false ); // Will only get here if there are no available imps
  }
};


struct curse_of_the_elements_t : public warlock_spell_t
{
  curse_of_the_elements_t( warlock_t* p ) :
    warlock_spell_t( p, "Curse of the Elements" )
  {
    background = ( sim -> overrides.magic_vulnerability != 0 );
    num_ticks = 0;
    may_crit = false;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( ! sim -> overrides.magic_vulnerability )
        target -> debuffs.magic_vulnerability -> trigger( 1, buff_t::DEFAULT_VALUE(), -1, data().duration() );
    }
  }
};


struct agony_t : public warlock_spell_t
{
  agony_t( warlock_t* p ) :
    warlock_spell_t( p, "Agony" )
  {
    may_crit = false;
    if ( p -> spec.pandemic -> ok() ) dot_behavior = DOT_EXTEND;
    num_ticks = ( int ) util::ceil( num_ticks * ( 1.0 + p -> glyphs.everlasting_affliction -> effectN( 1 ).percent() ) );
    base_multiplier *= 1.0 + p -> glyphs.everlasting_affliction -> effectN( 2 ).percent();
    base_multiplier *= 1.0 + p -> set_bonus.tier13_4pc_caster() * p -> sets -> set( SET_T13_4PC_CASTER ) -> effectN( 1 ).percent();
  }

  virtual void last_tick( dot_t* d )
  {
    td( d -> state -> target ) -> agony_stack = 1;
    warlock_spell_t::last_tick( d );
  }

  virtual void tick( dot_t* d )
  {
    if ( td( d -> state -> target ) -> agony_stack < 10 ) td( d -> state -> target ) -> agony_stack++;
    warlock_spell_t::tick( d );
  }

  virtual double calculate_tick_damage( result_e r, double p, double m, player_t* t )
  {
    return warlock_spell_t::calculate_tick_damage( r, p, m, t ) * td( t ) -> agony_stack;
  }

  virtual double action_multiplier()
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
    if ( p -> spec.pandemic -> ok() ) dot_behavior = DOT_EXTEND;
    num_ticks = ( int ) util::ceil( num_ticks * ( 1.0 + p -> glyphs.everlasting_affliction -> effectN( 1 ).percent() ) );
    base_multiplier *= 1.0 + p -> glyphs.everlasting_affliction -> effectN( 2 ).percent();
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    if ( d -> state -> result == RESULT_CRIT ) trigger_wild_imp( p() );

    if ( p() -> glyphs.siphon_life -> ok() )
      p() -> resource_gain( RESOURCE_HEALTH, d -> state -> result_amount * p() -> glyphs.siphon_life -> effectN( 1 ).percent(), p() -> gains.siphon_life );
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};


struct havoc_t : public warlock_spell_t
{
  havoc_t( warlock_t* p ) : warlock_spell_t( p, "Havoc" )
  {
    may_crit = false;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    p() -> buffs.havoc -> trigger( 3 );
    p() -> havoc_target = target;
  }
};


struct shadow_bolt_copy_t : public warlock_spell_t
{
  shadow_bolt_copy_t( warlock_t* p, spell_data_t* sd, warlock_spell_t& sb ) :
    warlock_spell_t( "shadow_bolt", p, sd )
  {
    background = true;
    callbacks  = false;
    direct_power_mod = sb.direct_power_mod;
    base_dd_min      = sb.base_dd_min;
    base_dd_max      = sb.base_dd_max;
    base_multiplier  = sb.base_multiplier;
    if ( data()._effects -> size() > 1 ) generate_fury = data().effectN( 2 ).base_value();
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 4 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }
};

struct shadow_bolt_t : public warlock_spell_t
{
  shadow_bolt_copy_t* glyph_copy_1;
  shadow_bolt_copy_t* glyph_copy_2;

  shadow_bolt_t( warlock_t* p ) :
    warlock_spell_t( p, "Shadow Bolt" ), glyph_copy_1( 0 ), glyph_copy_2( 0 )
  {
    base_multiplier *= 1.0 + p -> set_bonus.tier14_2pc_caster() * p -> sets -> set( SET_T14_2PC_CASTER ) -> effectN( 3 ).percent();
    base_multiplier *= 1.0 + p -> set_bonus.tier13_4pc_caster() * p -> sets -> set( SET_T13_4PC_CASTER ) -> effectN( 1 ).percent();

    if ( p -> glyphs.shadow_bolt -> ok() )
    {
      base_multiplier *= 0.333;

      const spell_data_t* sd = p -> find_spell( p -> glyphs.shadow_bolt -> effectN( 1 ).base_value() );
      glyph_copy_1 = new shadow_bolt_copy_t( p, sd -> effectN( 2 ).trigger(), *this );
      glyph_copy_2 = new shadow_bolt_copy_t( p, sd -> effectN( 3 ).trigger(), *this );
    }
    else
    {
      generate_fury = data().effectN( 2 ).base_value();
    }
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 4 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_soul_leech( p(), s -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() );
    }
  }

  virtual void execute()
  {
    // FIXME!!! Ugly hack to ensure we don't proc any on-spellcast trinkets
    if ( p() -> bugs && p() -> glyphs.shadow_bolt -> ok() ) background = true;
    warlock_spell_t::execute();
    background = false;

    if ( p() -> buffs.demonic_calling -> up() )
    {
      trigger_wild_imp( p() );
      p() -> buffs.demonic_calling -> expire();
    }

    if ( result_is_hit( execute_state -> result ) )
    {
      // Only applies molten core if molten core is not already up
      if ( target -> health_percentage() < p() -> spec.decimation -> effectN( 1 ).base_value() && ! p() -> buffs.molten_core -> check() )
        p() -> buffs.molten_core -> trigger();

      if ( p() -> glyphs.shadow_bolt -> ok() )
      {
        assert( glyph_copy_1 && glyph_copy_2 );
        glyph_copy_1 -> execute();
        glyph_copy_2 -> execute();
      }
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
      event_t( p, "shadowburn_mana_return" ), spell( s ), gain( p -> gains.shadowburn )
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


  shadowburn_t( warlock_t* p ) :
    warlock_spell_t( p, "Shadowburn" ), mana_event( 0 )
  {
    min_gcd = timespan_t::from_millis( 500 );

    mana_delay  = data().effectN( 1 ).trigger() -> duration();
    mana_amount = p -> find_spell( data().effectN( 1 ).trigger() -> effectN( 1 ).base_value() ) -> effectN( 1 ).percent();
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    mana_event = new ( sim ) mana_event_t( p(), this );
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> composite_mastery() * p() -> mastery_spells.emberstorm -> effectN( 1 ).mastery_value();

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 5 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( ! result_is_hit( execute_state -> result ) ) refund_embers( p() );
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( target -> health_percentage() >= 20 ) r = false;

    return r;
  }
};


struct corruption_t : public warlock_spell_t
{
  bool soc_triggered;

  corruption_t( warlock_t* p, bool soc = false ) :
    warlock_spell_t( p, "Corruption" ), soc_triggered( soc )
  {
    may_crit = false;
    generate_fury = 4;
    if ( p -> spec.pandemic -> ok() ) dot_behavior = DOT_EXTEND;
    num_ticks = ( int ) util::ceil( num_ticks * ( 1.0 + p -> glyphs.everlasting_affliction -> effectN( 1 ).percent() ) );
    base_multiplier *= 1.0 + p -> glyphs.everlasting_affliction -> effectN( 2 ).percent();
    base_multiplier *= 1.0 + p -> set_bonus.tier14_2pc_caster() * p -> sets -> set( SET_T14_2PC_CASTER ) -> effectN( 1 ).percent();
    base_multiplier *= 1.0 + p -> set_bonus.tier13_4pc_caster() * p -> sets -> set( SET_T13_4PC_CASTER ) -> effectN( 1 ).percent();
  };

  virtual timespan_t travel_time()
  {
    if ( soc_triggered ) return timespan_t::from_seconds( std::max( sim -> gauss( sim -> aura_delay, 0.25 * sim -> aura_delay ).total_seconds() , 0.01 ) );

    return warlock_spell_t::travel_time();
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    if ( p() -> spec.nightfall -> ok() )
    {
      p() -> nightfall_chance += 0.00333; // Confirmed 09/09/2012
      if ( p() -> rngs.nightfall -> roll( p() -> nightfall_chance ) )
      {
        p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.nightfall );
        p() -> nightfall_chance = 0;
        // If going from 0 to 1 shard was a surprise, the player would have to react to it
        if ( p() -> resources.current[ RESOURCE_SOUL_SHARD ] == 1 )
          p() -> shard_react = p() -> sim -> current_time + p() -> total_reaction_time();
        else if ( p() -> resources.current[ RESOURCE_SOUL_SHARD ] >= 1 )
          p() -> shard_react = p() -> sim -> current_time;
        else
          p() -> shard_react = timespan_t::max();
      }
    }

    if ( p() -> glyphs.siphon_life -> ok() )
      p() -> resource_gain( RESOURCE_HEALTH, d -> state -> result_amount * p() -> glyphs.siphon_life -> effectN( 1 ).percent(), p() -> gains.siphon_life );
  }

  virtual double action_multiplier()
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
    channeled      = true;
    hasted_ticks   = false;
    may_crit       = false;
    generate_fury  = 10;

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
    warlock_spell_t::tick( d );

    heal -> execute();

    consume_tick_resource( d );
  }
};


struct drain_soul_t : public warlock_spell_t
{
  bool generate_shard;

  drain_soul_t( warlock_t* p ) :
    warlock_spell_t( p, "Drain Soul" ), generate_shard( false )
  {
    channeled    = true;
    hasted_ticks = false;
    may_crit     = false;
    stormlash_da_multiplier = 0.0;
    stormlash_ta_multiplier = 2.0;
    base_dd_min = base_dd_max = 0; // prevent it from picking up direct damage from that strange absorb effect

    stats -> add_child( p -> get_stats( "agony_ds" ) );
    stats -> add_child( p -> get_stats( "corruption_ds" ) );
    stats -> add_child( p -> get_stats( "unstable_affliction_ds" ) );
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double m = warlock_spell_t::composite_target_multiplier( t );

    if ( t -> health_percentage() <= data().effectN( 3 ).base_value() )
      m *= 1.0 + data().effectN( 6 ).percent();

    return m;
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 3 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    if ( generate_shard )
    {
      p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.drain_soul );
      p() -> shard_react = p() -> sim -> current_time;
    }
    generate_shard = ! generate_shard;

    trigger_soul_leech( p(), d -> state -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() * 2 );

    if ( d -> state -> target -> health_percentage() <= data().effectN( 3 ).base_value() )
    {
      trigger_extra_tick( td( d -> state -> target ) -> dots_agony,               data().effectN( 6 ).percent() * ( 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 3 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack() ) );
      trigger_extra_tick( td( d -> state -> target ) -> dots_corruption,          data().effectN( 6 ).percent() * ( 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 3 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack() ) );
      trigger_extra_tick( td( d -> state -> target ) -> dots_unstable_affliction, data().effectN( 6 ).percent() * ( 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 3 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack() ) );
    }

    consume_tick_resource( d );
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      generate_shard = false;

      if ( s -> target -> health_percentage() <= data().effectN( 3 ).base_value() )
        td( s -> target ) -> ds_started_below_20 = true;
      else
        td( s -> target ) -> ds_started_below_20 = false;
    }
  }
};


struct unstable_affliction_t : public warlock_spell_t
{
  unstable_affliction_t( warlock_t* p ) :
    warlock_spell_t( p, "Unstable Affliction" )
  {
    may_crit   = false;
    if ( p -> spec.pandemic -> ok() ) dot_behavior = DOT_EXTEND;
    num_ticks = ( int ) util::ceil( num_ticks * ( 1.0 + p -> glyphs.everlasting_affliction -> effectN( 1 ).percent() ) );
    base_multiplier *= 1.0 + p -> glyphs.everlasting_affliction -> effectN( 2 ).percent();
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> composite_mastery() * p() -> mastery_spells.potent_afflictions -> effectN( 1 ).mastery_value();

    return m;
  }
};


struct haunt_t : public warlock_spell_t
{
  haunt_t( warlock_t* p ) :
    warlock_spell_t( p, "Haunt" )
  {
    hasted_ticks = false;
    tick_may_crit = false;
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 3 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs_haunt -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, td( s -> target ) -> dots_haunt -> remains() );

      trigger_soul_leech( p(), s -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() * 2 );
    }
  }
};


struct immolate_t : public warlock_spell_t
{
  immolate_t( warlock_t* p ) :
    warlock_spell_t( p, "Immolate" )
  {
    base_costs[ RESOURCE_MANA ] *= 1.0 + p -> spec.chaotic_energy -> effectN( 2 ).percent();

    if ( p -> spec.pandemic -> ok() ) dot_behavior = DOT_EXTEND;
  }

  virtual double cost()
  {
    if ( p() -> buffs.fire_and_brimstone -> check() )
      return 0;

    return warlock_spell_t::cost();
  }

  virtual void execute()
  {
    if ( p() -> buffs.fire_and_brimstone -> check() )
      aoe = -1;

    warlock_spell_t::execute();

    if ( p() -> buffs.fire_and_brimstone -> up() )
    {
      p() -> buffs.fire_and_brimstone -> expire();
      aoe = 0;
    }
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    if ( aoe == -1 ) m *= ( 1.0 + p() -> composite_mastery() * p() -> mastery_spells.emberstorm -> effectN( 1 ).mastery_value() ) * 0.4;

    m *= 1.0 + p() -> mastery_spells.emberstorm -> effectN( 3 ).percent() + p() -> composite_mastery() * p() -> mastery_spells.emberstorm -> effectN( 3 ).mastery_value();

    return m;
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    if ( s -> result == RESULT_CRIT ) trigger_ember_gain( p(), 0.1, p() -> gains.immolate, 1.0 );
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    if ( d -> state -> result == RESULT_CRIT ) trigger_ember_gain( p(), 0.1, p() -> gains.immolate, 1.0 );
  }
};


struct conflagrate_t : public warlock_spell_t
{
  conflagrate_t( warlock_t* p ) :
    warlock_spell_t( p, "Conflagrate" )
  {
    base_costs[ RESOURCE_MANA ] *= 1.0 + p -> spec.chaotic_energy -> effectN( 2 ).percent();

    // FIXME: No longer in the spell data for some reason
    cooldown -> duration = timespan_t::from_seconds( 12.0 );
    cooldown -> charges = 2;
  }

  virtual double cost()
  {
    if ( p() -> buffs.fire_and_brimstone -> check() )
      return 0;

    return warlock_spell_t::cost();
  }

  virtual void execute()
  {
    if ( p() -> buffs.fire_and_brimstone -> check() )
      aoe = -1;

    warlock_spell_t::execute();

    if ( p() -> buffs.fire_and_brimstone -> up() )
    {
      p() -> buffs.fire_and_brimstone -> expire();
      aoe = 0;
    }
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    if ( aoe == -1 )
      m *= ( 1.0 + p() -> composite_mastery() * p() -> mastery_spells.emberstorm -> effectN( 1 ).mastery_value() ) * 0.4;
    else
      m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 5 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    m *= 1.0 + p() -> mastery_spells.emberstorm -> effectN( 3 ).percent() + p() -> composite_mastery() * p() -> mastery_spells.emberstorm -> effectN( 3 ).mastery_value();

    return m;
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    trigger_ember_gain( p(), s -> result == RESULT_CRIT ? 0.2 : 0.1, p() -> gains.conflagrate );

    if ( result_is_hit( s -> result ) && p() -> spec.backdraft -> ok() )
      p() -> buffs.backdraft -> trigger( 3 );
  }
};


struct incinerate_t : public warlock_spell_t
{
  incinerate_t( warlock_t* p ) :
    warlock_spell_t( p, "Incinerate" )
  {
    base_costs[ RESOURCE_MANA ] *= 1.0 + p -> spec.chaotic_energy -> effectN( 2 ).percent();
    base_multiplier *= 1.0 + p -> set_bonus.tier14_2pc_caster() * p -> sets -> set( SET_T14_2PC_CASTER ) -> effectN( 2 ).percent();
    base_multiplier *= 1.0 + p -> set_bonus.tier13_4pc_caster() * p -> sets -> set( SET_T13_4PC_CASTER ) -> effectN( 1 ).percent();
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    if ( aoe == -1 )
      m *= ( 1.0 + p() -> composite_mastery() * p() -> mastery_spells.emberstorm -> effectN( 1 ).mastery_value() ) * 0.4;
    else
      m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 5 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    m *= 1.0 + p() -> mastery_spells.emberstorm -> effectN( 3 ).percent() + p() -> composite_mastery() * p() -> mastery_spells.emberstorm -> effectN( 3 ).mastery_value();

    return m;
  }

  virtual void execute()
  {
    if ( p() -> buffs.fire_and_brimstone -> check() )
      aoe = -1;

    warlock_spell_t::execute();

    if ( p() -> buffs.fire_and_brimstone -> up() )
    {
      p() -> buffs.fire_and_brimstone -> expire();
      aoe = 0;
    }

    if ( p() -> buffs.backdraft -> check() )
    {
      p() -> buffs.backdraft -> decrement();
    }
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    trigger_ember_gain( p(), s -> result == RESULT_CRIT ? 0.2 : 0.1, p() -> gains.incinerate );

    if ( result_is_hit( s -> result ) )
    {
      trigger_soul_leech( p(), s -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() );
    }
  }

  virtual timespan_t execute_time()
  {
    timespan_t h = warlock_spell_t::execute_time();

    if ( p() -> buffs.backdraft -> up() )
    {
      h *= 1.0 + p() -> buffs.backdraft -> data().effectN( 1 ).percent();
    }

    return h;
  }

  virtual double cost()
  {
    if ( p() -> buffs.fire_and_brimstone -> check() )
      return 0;

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
  warlock_spell_t* meta_spell;

  soul_fire_t( warlock_t* p, bool meta = false ) :
    warlock_spell_t( meta ? "soul_fire_meta" : "soul_fire", p, meta ? p -> find_spell( 104027 ) : p -> find_spell( 6353 ) ), meta_spell( 0 )
  {
    if ( ! meta )
    {
      generate_fury = data().effectN( 2 ).base_value();
      meta_spell = new soul_fire_t( p, true );
    }
    else
    {
      background = true;
    }
  }

  virtual void parse_options( option_t* o, const std::string& options_str )
  {
    warlock_spell_t::parse_options( o, options_str );
    if ( meta_spell ) meta_spell -> parse_options( o, options_str );
  }

  virtual void execute()
  {
    if ( meta_spell && p() -> buffs.metamorphosis -> check() )
    {
      meta_spell -> time_to_execute = time_to_execute;
      meta_spell -> execute();
    }
    else
    {
      warlock_spell_t::execute();

      if ( p() -> buffs.demonic_calling -> up() )
      {
        trigger_wild_imp( p() );
        p() -> buffs.demonic_calling -> expire();
      }

      if ( p() -> buffs.molten_core -> check() )
        p() -> buffs.molten_core -> decrement();

      if ( result_is_hit( execute_state -> result ) && target -> health_percentage() < p() -> spec.decimation -> effectN( 1 ).base_value() )
        p() -> buffs.molten_core -> trigger();
    }
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      trigger_soul_leech( p(), s -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() );
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = warlock_spell_t::execute_time();

    if ( p() -> buffs.molten_core -> up() )
      t *= 1.0 + p() -> buffs.molten_core -> data().effectN( 1 ).percent();

    return t;
  }

  virtual double crit_chance( double /* crit */, int /* delta_level */ )
  {
    // Soul fire always crits
    return 1.0;
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> composite_spell_crit();

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 4 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual double cost()
  {
    double c = warlock_spell_t::cost();

    if ( p() -> buffs.molten_core -> check() )
      c *= 1.0 + p() -> buffs.molten_core -> data().effectN( 1 ).percent();

    return c;
  }

  virtual bool ready()
  {
    return ( meta_spell && p() -> buffs.metamorphosis -> check() ) ? meta_spell -> ready() : warlock_spell_t::ready();
  }
};


struct chaos_bolt_t : public warlock_spell_t
{
  chaos_bolt_t( warlock_t* p ) :
    warlock_spell_t( p, "Chaos Bolt" )
  {
  }

  virtual std::vector< player_t* >& target_list()
  {
    if ( aoe == 2 && p() -> buffs.havoc -> check() >= 3 && target != p() -> havoc_target )
      return warlock_spell_t::target_list();
    else
      return spell_t::target_list();
  }

  virtual double crit_chance( double /* crit */, int /* delta_level */ )
  {
    // Chaos Bolt always crits
    return 1.0;
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> composite_mastery() * p() -> mastery_spells.emberstorm -> effectN( 1 ).mastery_value();

    m *= 1.0 + p() -> composite_spell_crit();

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 5 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual void execute()
  {
    bool havoc = false;
    if ( p() -> buffs.havoc -> stack() >= 3 && p() -> havoc_target != target )
    {
      aoe = 2;
      havoc = true;
    }

    spell_t::execute();

    if ( ! result_is_hit( execute_state -> result ) ) refund_embers( p() );

    if ( p() -> spec.pyroclasm -> ok() && p() -> buffs.backdraft -> check() >= 3 )
    {
      p() -> buffs.backdraft -> decrement( 3 );
    }

    if ( havoc )
    {
      p() -> buffs.havoc -> decrement();
      aoe = 0;
    }
  }

  virtual timespan_t execute_time()
  {
    timespan_t h = warlock_spell_t::execute_time();

    if ( p() -> spec.pyroclasm -> ok() && p() -> buffs.backdraft -> stack() >= 3 )
    {
      h *= 1.0 + p() -> buffs.backdraft -> data().effectN( 1 ).percent();
    }

    return h;
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

    double health = player -> resources.max[ RESOURCE_HEALTH ];
    // FIXME: This should be implemented as a real health gain, but we don't have an easy way to do temporary percentage-wise resource gains
    if ( p() -> talents.soul_link -> ok() && p() -> buffs.grimoire_of_sacrifice -> up() )
      health *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 7 ).percent();

    // FIXME: Implement reduced healing debuff
    if ( ! p() -> glyphs.life_tap -> ok() ) player -> resource_loss( RESOURCE_HEALTH, health * data().effectN( 3 ).percent() );
    player -> resource_gain( RESOURCE_MANA, health * data().effectN( 1 ).percent(), p() -> gains.life_tap );
  }
};


struct melee_t : public warlock_spell_t
{
  melee_t( warlock_t* p ) :
    warlock_spell_t( "melee", p, p -> find_spell( 103988 ) )
  {
    background        = true;
    repeating         = true;
    base_execute_time = timespan_t::from_seconds( 1 );
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 4 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }
};

struct activate_melee_t : public warlock_spell_t
{
  activate_melee_t( warlock_t* p ) :
    warlock_spell_t( "activate_melee", p, spell_data_t::nil() )
  {
    trigger_gcd = timespan_t::zero();
    harmful = false;

    if ( ! p -> spells.melee ) p -> spells.melee = new melee_t( p );
  }

  virtual void execute()
  {
    p() -> spells.melee -> execute();
  }

  virtual bool ready()
  {
    // FIXME: Properly remove this whole ability later - no time now
    return false;
  }
};


struct metamorphosis_t : public warlock_spell_t
{
  metamorphosis_t( warlock_t* p ) :
    warlock_spell_t( p, "Metamorphosis" )
  {
    trigger_gcd = timespan_t::zero();
    harmful = false;
    background = true;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    assert( cost_event == 0 );
    p() -> buffs.metamorphosis -> trigger();
    cost_event = new ( sim ) cost_event_t( p(), this );
  }

  virtual void cancel()
  {
    warlock_spell_t::cancel();

    if ( p() -> spells.melee ) p() -> spells.melee -> cancel();
    p() -> buffs.metamorphosis -> expire();
    event_t::cancel( cost_event );
  }
};

struct activate_metamorphosis_t : public warlock_spell_t
{
  activate_metamorphosis_t( warlock_t* p ) :
    warlock_spell_t( "activate_metamorphosis", p )
  {
    trigger_gcd = timespan_t::zero();
    harmful = false;

    if ( ! p -> spells.metamorphosis ) p -> spells.metamorphosis = new metamorphosis_t( p );
  }

  virtual void execute()
  {
    if ( ! p() -> buffs.metamorphosis -> check() ) p() -> spells.metamorphosis -> execute();
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> buffs.metamorphosis -> check() ) r = false;
    else if ( p() -> resources.current[ RESOURCE_DEMONIC_FURY ] <= META_FURY_MINIMUM ) r = false;
    else if ( ! p() -> spells.metamorphosis -> ready() ) r = false;

    return r;
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

    p() -> spells.metamorphosis -> cancel();
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};


struct shadowflame_t : public warlock_spell_t
{
  shadowflame_t( warlock_t* p ) :
    warlock_spell_t( "shadowflame", p, p -> find_spell( 47960 ) )
  {
    background = true;
    may_miss   = false;
    generate_fury = 2;
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    if ( p() -> spec.molten_core -> ok() && p() -> rngs.molten_core -> roll( 0.08 ) )
      p() -> buffs.molten_core -> trigger();
  }

  virtual double calculate_tick_damage( result_e r, double p, double m, player_t* t )
  {
    return warlock_spell_t::calculate_tick_damage( r, p, m, t ) * td( t ) -> shadowflame_stack;
  }

  virtual void impact( action_state_t* s )
  {
    if ( result_is_hit( s -> result ) )
    {
      if ( td( s -> target ) -> dots_shadowflame -> ticking )
        td( s -> target ) -> shadowflame_stack++;
      else
        td( s -> target ) -> shadowflame_stack = 1;
    }

    warlock_spell_t::impact( s );
  }
};

struct hand_of_guldan_t : public warlock_spell_t
{
  hand_of_guldan_t( warlock_t* p ) :
    warlock_spell_t( p, "Hand of Gul'dan" )
  {
    aoe = -1;

    cooldown -> duration = timespan_t::from_seconds( 15 );
    cooldown -> charges = 2;

    impact_action = new shadowflame_t( p );

    parse_effect_data( p -> find_spell( 86040 ) -> effectN( 1 ) );

    add_child( impact_action );
  }

  virtual double action_da_multiplier()
  {
    double m = warlock_spell_t::action_da_multiplier();

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 4 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual timespan_t travel_time()
  {
    return timespan_t::from_seconds( 1.5 );
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};


struct chaos_wave_dmg_t : public warlock_spell_t
{
  chaos_wave_dmg_t( warlock_t* p ) :
    warlock_spell_t( "chaos_wave_dmg", p, p -> find_spell( 124915 ) )
  {
    aoe        = -1;
    background = true;
    dual       = true;
  }
};


struct chaos_wave_t : public warlock_spell_t
{
  chaos_wave_dmg_t* cw_damage;

  chaos_wave_t( warlock_t* p ) :
    warlock_spell_t( "chaos_wave", p, p -> spec.chaos_wave ),
    cw_damage( 0 )
  {
    cooldown = p -> cooldowns.hand_of_guldan;

    impact_action  = new chaos_wave_dmg_t( p );
    impact_action -> stats = stats;
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 4 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual timespan_t travel_time()
  {
    return timespan_t::from_seconds( 1.5 );
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};


struct touch_of_chaos_t : public warlock_spell_t
{
  touch_of_chaos_t( warlock_t* p ) :
    warlock_spell_t( "touch_of_chaos", p, p -> spec.touch_of_chaos )
  {
    base_multiplier *= 1.0 + p -> set_bonus.tier14_2pc_caster() * p -> sets -> set( SET_T14_2PC_CASTER ) -> effectN( 3 ).percent();
    base_multiplier *= 1.0 + p -> set_bonus.tier13_4pc_caster() * p -> sets -> set( SET_T13_4PC_CASTER ) -> effectN( 1 ).percent(); // Assumption - need to test whether ToC is affected
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 4 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_soul_leech( p(), s -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() );
      extend_dot( td( s -> target ) -> dots_corruption, 4 );
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

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};


struct fel_flame_t : public warlock_spell_t
{
  fel_flame_t( warlock_t* p ) :
    warlock_spell_t( p, "Fel Flame" )
  {
    if ( p -> specialization() == WARLOCK_DESTRUCTION )
      base_costs[ RESOURCE_MANA ] *= 1.0 + p -> spec.chaotic_energy -> effectN( 2 ).percent();
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    if ( p() -> specialization() == WARLOCK_DESTRUCTION ) trigger_ember_gain( p(), 0.1, p() -> gains.fel_flame );

    if ( result_is_hit( s -> result ) )
    {
      extend_dot(            td( s -> target ) -> dots_immolate, 3 );
      extend_dot( td( s -> target ) -> dots_unstable_affliction, 3 );
      extend_dot(          td( s -> target ) -> dots_corruption, 3 );
    }
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> specialization() == WARLOCK_AFFLICTION )
      m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 3 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    if ( p() -> specialization() == WARLOCK_DEMONOLOGY )
      m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 4 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    if ( p() -> specialization() == WARLOCK_DESTRUCTION )
      m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 5 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    m *= 1.0 + p() -> mastery_spells.emberstorm -> effectN( 3 ).percent() + p() -> composite_mastery() * p() -> mastery_spells.emberstorm -> effectN( 3 ).mastery_value();

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
  void_ray_t( warlock_t* p ) :
    warlock_spell_t( p, "Void Ray" )
  {
    aoe = -1;
    travel_speed = 0;
    direct_power_mod = data().effectN( 1 ).coeff();
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 4 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      extend_dot( td( s -> target ) -> dots_corruption, 3 );
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
    hasted_ticks = false;
    may_crit     = false;
    stormlash_da_multiplier = 0.0;
    stormlash_ta_multiplier = 2.0;

    stats -> add_child( p -> get_stats( "agony_mg" ) );
    stats -> add_child( p -> get_stats( "corruption_mg" ) );
    stats -> add_child( p -> get_stats( "unstable_affliction_mg" ) );
  }

  virtual double action_multiplier()
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 3 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    trigger_soul_leech( p(), d -> state -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() * 2 );

    trigger_extra_tick( td( d -> state -> target ) -> dots_agony,               data().effectN( 3 ).percent() * ( 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 3 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack() ) );
    trigger_extra_tick( td( d -> state -> target ) -> dots_corruption,          data().effectN( 3 ).percent() * ( 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 3 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack() ) );
    trigger_extra_tick( td( d -> state -> target ) -> dots_unstable_affliction, data().effectN( 3 ).percent() * ( 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 3 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack() ) );

    consume_tick_resource( d );
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

    warlock_spell_t::execute();
  }
};


struct dark_soul_t : public warlock_spell_t
{
  dark_soul_t( warlock_t* p ) :
    warlock_spell_t( "dark_soul", p, p -> spec.dark_soul )
  {
    harmful = false;

    cooldown -> duration += p -> set_bonus.tier14_4pc_caster() * p -> sets -> set( SET_T14_4PC_CASTER ) -> effectN( 1 ).time_value();
    p -> buffs.dark_soul -> cooldown -> duration = cooldown -> duration;
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
    warlock_spell_t( "imp_swarm", p, ( p -> specialization() == WARLOCK_DEMONOLOGY && p -> glyphs.imp_swarm -> ok() ) ? p -> find_spell( 104316 ) : spell_data_t::not_found() ),
    base_cooldown( cooldown -> duration )
  {
    harmful = false;

    stats -> add_child( p -> get_stats( "firebolt" ) );
  }

  virtual void execute()
  {
    cooldown -> duration = base_cooldown * composite_haste();

    warlock_spell_t::execute();

    event_t::cancel( p() -> demonic_calling_event );
    p() -> demonic_calling_event = new ( sim ) warlock_t::demonic_calling_event_t( player, cooldown -> duration, true );

    int imp_count = data().effectN( 1 ).base_value();
    int j = 0;

    for ( int i = 0; i < WILD_IMP_LIMIT; i++ )
    {
      if ( p() -> pets.wild_imps[ i ] -> current.sleeping )
      {
        p() -> pets.wild_imps[ i ] -> trigger( true );
        if ( ++j == imp_count ) break;
      }
    }
    if ( j != imp_count ) sim -> errorf( "Player %s ran out of wild imps during imp_swarm.\n", p() -> name() );
    assert( j == imp_count );  // Assert fails if we didn't have enough available wild imps
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
  seed_of_corruption_aoe_t( warlock_t* p ) :
    warlock_spell_t( "seed_of_corruption_aoe", p, p -> find_spell( 27285 ) )
  {
    aoe        = -1;
    dual       = true;
    background = true;
    callbacks  = false;
  }

  virtual void init()
  {
    warlock_spell_t::init();

    p() -> spells.soulburn_seed_of_corruption_aoe -> stats = stats;
  }
};

struct soulburn_seed_of_corruption_aoe_t : public warlock_spell_t
{
  corruption_t* corruption;

  soulburn_seed_of_corruption_aoe_t( warlock_t* p ) :
    warlock_spell_t( "soulburn_seed_of_corruption_aoe", p, p -> find_spell( 87385 ) ), corruption( new corruption_t( p, true ) )
  {
    aoe        = -1;
    dual       = true;
    background = true;
    callbacks  = false;
    corruption -> background = true;
    corruption -> dual = true;
    corruption -> may_miss = false;
    corruption -> base_costs[ RESOURCE_MANA ] = 0;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.seed_of_corruption );
    p() -> shard_react = p() -> sim -> current_time;
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    corruption -> target = s -> target;
    corruption -> execute();
  }
};

struct soulburn_seed_of_corruption_t : public warlock_spell_t
{
  double coefficient;

  soulburn_seed_of_corruption_t( warlock_t* p, double coeff ) :
    warlock_spell_t( "soulburn_seed_of_corruption", p, p -> find_spell( 114790 ) ), coefficient( coeff )
  {
    may_crit = false;
    background = true;
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      td( s -> target ) -> soulburn_soc_trigger = data().effectN( 3 ).average( p() ) + s -> composite_power() * coefficient;
  }
};

struct seed_of_corruption_t : public warlock_spell_t
{
  soulburn_seed_of_corruption_t* soulburn_spell;

  seed_of_corruption_t( warlock_t* p ) :
    warlock_spell_t( "seed_of_corruption", p, p -> find_spell( 27243 ) ), soulburn_spell( new soulburn_seed_of_corruption_t( p, data().effectN( 3 ).coeff() ) )
  {
    may_crit = false;

    if ( ! p -> spells.seed_of_corruption_aoe ) p -> spells.seed_of_corruption_aoe = new seed_of_corruption_aoe_t( p );
    if ( ! p -> spells.soulburn_seed_of_corruption_aoe ) p -> spells.soulburn_seed_of_corruption_aoe = new soulburn_seed_of_corruption_aoe_t( p );

    add_child( p -> spells.seed_of_corruption_aoe );
    soulburn_spell -> stats = stats;
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      td( s -> target ) -> soc_trigger = data().effectN( 3 ).average( p() ) + s -> composite_power() * data().effectN( 3 ).coeff();
  }

  virtual void execute()
  {
    if ( p() -> buffs.soulburn -> up() )
    {
      p() -> buffs.soulburn -> expire();
      soulburn_spell -> target = target;
      soulburn_spell -> time_to_execute = time_to_execute;
      soulburn_spell -> execute();
    }
    else
    {
      warlock_spell_t::execute();
    }
  }
};


struct rain_of_fire_tick_t : public warlock_spell_t
{
  const spell_data_t& parent_data;

  rain_of_fire_tick_t( warlock_t* p, const spell_data_t& pd ) :
    warlock_spell_t( "rain_of_fire_tick", p, pd.effectN( 2 ).trigger() ), parent_data( pd )
  {
    aoe         = -1;
    background  = true;
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) && td( s -> target ) -> dots_immolate -> ticking )
      trigger_ember_gain( p(), 0.1, p() -> gains.rain_of_fire, parent_data.effectN( 2 ).percent() );
  }
};


struct rain_of_fire_t : public warlock_spell_t
{
  rain_of_fire_t( warlock_t* p ) :
    warlock_spell_t( "rain_of_fire", p, ( p -> specialization() == WARLOCK_DESTRUCTION ) ? p -> find_spell( 104232 ) : ( p -> specialization() == WARLOCK_AFFLICTION ) ? p -> find_spell( 5740 ) : spell_data_t::not_found() )
  {
    dot_behavior = DOT_CLIP;
    may_miss = false;
    may_crit = false;
    channeled = ( p -> spec.aftermath -> ok() ) ? false : true;
    tick_zero = ( p -> spec.aftermath -> ok() ) ? false : true;

    base_costs[ RESOURCE_MANA ] *= 1.0 + p -> spec.chaotic_energy -> effectN( 2 ).percent();

    tick_action = new rain_of_fire_tick_t( p, data() );
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    if ( channeled && d -> current_tick != 0 ) consume_tick_resource( d );
  }

  virtual int hasted_num_ticks( double /*haste*/, timespan_t /*d*/ )
  {
    return num_ticks;
  }

  virtual double composite_target_ta_multiplier( player_t* t )
  {
    double m = warlock_spell_t::composite_target_ta_multiplier( t );

    if ( td( t ) -> dots_immolate -> ticking )
      m *= 1.5;

    return m;
  }

};


struct hellfire_tick_t : public warlock_spell_t
{
  hellfire_tick_t( warlock_t* p, const spell_data_t& s ) :
    warlock_spell_t( "hellfire_tick", p, s.effectN( 1 ).trigger() )
  {
    aoe         = -1;
    background  = true;
  }


  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      p() -> resource_gain( RESOURCE_DEMONIC_FURY, 3, gain );
  }
};


struct hellfire_t : public warlock_spell_t
{
  hellfire_t( warlock_t* p ) :
    warlock_spell_t( p, "Hellfire" )
  {
    tick_zero = true;
    may_miss = false;
    channeled = true;
    tick_zero = true;
    may_crit = false;

    tick_power_mod = base_td = 0;

    dynamic_tick_action = true;
    tick_action = new hellfire_tick_t( p, data() );
  }

  virtual bool usable_moving()
  {
    return true;
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    if ( d -> current_tick != 0 ) consume_tick_resource( d );
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }

  virtual int hasted_num_ticks( double /*haste*/, timespan_t /*d*/ )
  {
    return num_ticks;
  }
};


struct immolation_aura_tick_t : public warlock_spell_t
{
  immolation_aura_tick_t( warlock_t* p, const spell_data_t& s ) :
    warlock_spell_t( "immolation_aura_tick", p, s.effectN( 1 ).trigger() )
  {
    aoe         = -1;
    background  = true;
  }
};


struct immolation_aura_t : public warlock_spell_t
{
  immolation_aura_t( warlock_t* p ) :
    warlock_spell_t( p, "Immolation Aura" )
  {
    may_miss = false;
    tick_zero = true;
    may_crit = false;

    dynamic_tick_action = true;
    tick_action = new immolation_aura_tick_t( p, data() );
  }

  virtual void tick( dot_t* d )
  {
    if ( ! p() -> buffs.metamorphosis -> check() || p() -> resources.current[ RESOURCE_DEMONIC_FURY ] < META_FURY_MINIMUM )
    {
      d -> cancel();
      return;
    }

    warlock_spell_t::tick( d );
  }

  virtual void last_tick( dot_t* d )
  {
    warlock_spell_t::last_tick( d );

    event_t::cancel( cost_event );
  }

  virtual void impact( action_state_t* s )
  {
    dot_t* d = get_dot();
    bool add_ticks = ( d -> ticking > 0 ) ? true : false;
    int remaining_ticks = d -> num_ticks - d -> current_tick;

    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( add_ticks )
        d -> num_ticks = ( int ) std::min( remaining_ticks + 0.9 * num_ticks, 2.1 * num_ticks );

      if ( ! cost_event ) cost_event = new ( sim ) cost_event_t( p(), this );
    }
  }

  virtual int hasted_num_ticks( double /*haste*/, timespan_t /*d*/ )
  {
    return num_ticks;
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    dot_t* d = get_dot();
    if ( d -> ticking && d -> num_ticks - d -> current_tick > num_ticks * 1.2 )  r = false;

    return r;
  }
};


struct carrion_swarm_t : public warlock_spell_t
{
  carrion_swarm_t( warlock_t* p ) :
    warlock_spell_t( p, "Carrion Swarm" )
  {
    aoe = -1;
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};


// SOUL SWAP

struct soul_swap_t : public warlock_spell_t
{
  agony_t* agony;
  corruption_t* corruption;
  unstable_affliction_t* unstable_affliction;
  seed_of_corruption_t* seed_of_corruption;

  cooldown_t* glyph_cooldown;

  soul_swap_t( warlock_t* p ) :
    warlock_spell_t( p, "Soul Swap" ),
    agony( new agony_t( p ) ),
    corruption( new corruption_t( p ) ),
    unstable_affliction( new unstable_affliction_t( p ) ),
    seed_of_corruption( new seed_of_corruption_t( p ) ),
    glyph_cooldown( p -> get_cooldown( "glyphed_soul_swap" ) )
  {
    agony               -> background = true;
    agony               -> dual       = true;
    agony               -> base_costs[ RESOURCE_MANA ] = 0;
    corruption          -> background = true;
    corruption          -> dual       = true;
    corruption          -> base_costs[ RESOURCE_MANA ] = 0;
    unstable_affliction -> background = true;
    unstable_affliction -> dual       = true;
    unstable_affliction -> base_costs[ RESOURCE_MANA ] = 0;
    seed_of_corruption  -> background = true;
    seed_of_corruption  -> dual       = true;
    seed_of_corruption  -> base_costs[ RESOURCE_MANA ] = 0;

    if ( p -> glyphs.soul_swap -> ok() )
      glyph_cooldown -> duration = p -> glyphs.soul_swap -> effectN( 2 ).time_value();
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( p() -> buffs.soul_swap -> up() )
    {
      if ( target == p() -> soul_swap_state.target ) return;

      p() -> buffs.soul_swap -> expire();

      if ( p() -> glyphs.soul_swap -> ok() )
        glyph_cooldown -> start();

      if ( p() -> soul_swap_state.agony > 0 )
      {
        agony -> target = target;
        agony -> execute();
        td( target ) -> agony_stack = p() -> soul_swap_state.agony;
      }
      if ( p() -> soul_swap_state.corruption )
      {
        corruption -> target = target;
        corruption -> execute();
      }
      if ( p() -> soul_swap_state.unstable_affliction )
      {
        unstable_affliction -> target = target;
        unstable_affliction -> execute();
      }
      if ( p() -> soul_swap_state.seed_of_corruption )
      {
        seed_of_corruption -> target = target;
        seed_of_corruption -> execute();
      }
    }
    else if ( p() -> buffs.soulburn -> up() )
    {
      p() -> buffs.soulburn -> expire();

      agony -> target = target;
      agony -> execute();

      corruption -> target = target;
      corruption -> execute();

      unstable_affliction -> target = target;
      unstable_affliction -> execute();

      if ( p() -> glyphs.soul_swap -> ok() )
        glyph_cooldown -> start();
    }
    else
    {
      p() -> buffs.soul_swap -> trigger();

      p() -> soul_swap_state.target              = target;

      p() -> soul_swap_state.agony               = td( target ) -> dots_agony -> ticking ? td( target ) -> agony_stack : 0;
      p() -> soul_swap_state.corruption          = td( target ) -> dots_corruption -> ticking > 0;
      p() -> soul_swap_state.unstable_affliction = td( target ) -> dots_unstable_affliction -> ticking > 0;
      p() -> soul_swap_state.seed_of_corruption  = td( target ) -> dots_seed_of_corruption -> ticking > 0;

      if ( ! p() -> glyphs.soul_swap -> ok() )
      {
        td( target ) -> dots_agony -> cancel();
        td( target ) -> dots_corruption -> cancel();
        td( target ) -> dots_unstable_affliction -> cancel();
        td( target ) -> dots_seed_of_corruption -> cancel();
      }
    }
  }

  virtual bool ready()
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> buffs.soul_swap -> check() )
    {
      if ( target == p() -> soul_swap_state.target ) r = false;
    }
    else if ( glyph_cooldown -> down() )
    {
      r = false;
    }
    else if ( ! p() -> buffs.soulburn -> check() )
    {
      if ( ! td( target ) -> dots_agony               -> ticking
           && ! td( target ) -> dots_corruption          -> ticking
           && ! td( target ) -> dots_unstable_affliction -> ticking
           && ! td( target ) -> dots_seed_of_corruption  -> ticking )
        r = false;
    }

    return r;
  }
};


// SUMMONING SPELLS

struct summon_pet_t : public warlock_spell_t
{
  timespan_t summoning_duration;
  pets::warlock_pet_t* pet;

private:
  void _init_summon_pet_t( std::string pet_name )
  {
    harmful = false;

    util::tokenize( pet_name );

    pet = dynamic_cast<pets::warlock_pet_t*>( player -> find_pet( pet_name ) );
    if ( ! pet )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", player -> name(), pet_name.c_str() );
    }
  }

public:
  summon_pet_t( const std::string& n, warlock_t* p, const std::string& sname = "" ) :
    warlock_spell_t( p, sname.empty() ? "Summon " + n : sname ),
    summoning_duration ( timespan_t::zero() ),
    pet( 0 )
  {
    _init_summon_pet_t( n );
  }

  summon_pet_t( const std::string& n, warlock_t* p, int id ) :
    warlock_spell_t( n, p, p -> find_spell( id ) ),
    summoning_duration ( timespan_t::zero() ),
    pet( 0 )
  {
    _init_summon_pet_t( n );
  }

  summon_pet_t( const std::string& n, warlock_t* p, const spell_data_t* sd ) :
    warlock_spell_t( n, p, sd ),
    summoning_duration ( timespan_t::zero() ),
    pet( 0 )
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
  cooldown_t* instant_cooldown;

  summon_main_pet_t( const std::string& n, warlock_t* p ) :
    summon_pet_t( n, p ), instant_cooldown( p -> get_cooldown( "instant_summon_pet" ) )
  {
    instant_cooldown -> duration = timespan_t::from_seconds( 60 );
  }

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

    // FIXME: Currently on beta (2012/05/06) summoning a pet during metamorphosis makes you unable
    //        to summon another one for a full minute, regardless of meta. This may not be intended.
    if ( ( p() -> buffs.soulburn -> check() || p() -> specialization() == WARLOCK_DEMONOLOGY ) && instant_cooldown -> down() )
      return false;

    return summon_pet_t::ready();
  }

  virtual timespan_t execute_time()
  {
    if ( p() -> buffs.soulburn -> check() || p() -> buffs.demonic_rebirth -> check() || p() -> buffs.metamorphosis -> check() )
      return timespan_t::zero();

    return warlock_spell_t::execute_time();
  }

  virtual void execute()
  {
    summon_pet_t::execute();

    p() -> pets.active = p() -> pets.last = pet;

    if ( p() -> buffs.soulburn -> up() )
    {
      instant_cooldown -> start();
      p() -> buffs.soulburn -> expire();
    }

    if ( p() -> buffs.metamorphosis -> check() )
      instant_cooldown -> start();

    if ( p() -> buffs.demonic_rebirth -> up() )
      p() -> buffs.demonic_rebirth -> expire();

    if ( p() -> buffs.grimoire_of_sacrifice -> check() )
      p() -> buffs.grimoire_of_sacrifice -> expire();
  }
};


struct flames_of_xoroth_t : public warlock_spell_t
{
  gain_t* ember_gain;

  flames_of_xoroth_t( warlock_t* p ) :
    warlock_spell_t( p, "Flames of Xoroth" ), ember_gain( p -> get_gain( "flames_of_xoroth" ) )
  {
    harmful = false;
  }

  virtual double cost()
  {
    if ( p() -> pets.active || p() -> buffs.grimoire_of_sacrifice -> check() )
      return 0;
    else
      return warlock_spell_t::cost();
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    bool gain_ember = false;

    if ( p() -> buffs.grimoire_of_sacrifice -> check() )
    {
      p() -> buffs.grimoire_of_sacrifice -> expire();
      gain_ember = true;
    }
    else if ( p() -> pets.active )
    {
      p() -> pets.active -> dismiss();
      p() -> pets.active = 0;
      gain_ember = true;
    }
    else if ( p() -> pets.last )
    {
      p() -> pets.last -> summon();
      p() -> pets.active = p() -> pets.last;
    }

    if ( gain_ember ) p() -> resource_gain( RESOURCE_BURNING_EMBER, 1, ember_gain );
  }
};


struct infernal_awakening_t : public warlock_spell_t
{
  infernal_awakening_t( warlock_t* p, spell_data_t* spell ) :
    warlock_spell_t( "infernal_awakening", p, spell )
  {
    aoe        = -1;
    background = true;
    dual       = true;
    trigger_gcd= timespan_t::zero();
  }
};


struct summon_infernal_t : public summon_pet_t
{
  infernal_awakening_t* infernal_awakening;

  summon_infernal_t( warlock_t* p  ) :
    summon_pet_t( p -> talents.grimoire_of_supremacy -> ok() ? "abyssal" : "infernal", p ),
    infernal_awakening( 0 )
  {
    harmful = false;

    cooldown -> duration += ( p -> set_bonus.tier13_2pc_caster() ) ? timespan_t::from_millis( p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 3 ).base_value() ) : timespan_t::zero();

    summoning_duration = data().effectN( 2 ).trigger() -> duration();
    summoning_duration += ( p -> set_bonus.tier13_2pc_caster() ) ?
                          ( p -> specialization() == WARLOCK_DEMONOLOGY ?
                            timespan_t::from_seconds( p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 1 ).base_value() ) :
                            timespan_t::from_seconds( p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 2 ).base_value() ) ) : timespan_t::zero();

    infernal_awakening = new infernal_awakening_t( p, data().effectN( 1 ).trigger() );
    infernal_awakening -> stats = stats;
    pet -> summon_stats = stats;
  }

  virtual void execute()
  {
    summon_pet_t::execute();

    p() -> cooldowns.doomguard -> start();
    infernal_awakening -> execute();
  }
};


struct summon_doomguard2_t : public summon_pet_t
{
  summon_doomguard2_t( warlock_t* p, spell_data_t* spell ) :
    summon_pet_t( p -> talents.grimoire_of_supremacy -> ok() ? "terrorguard" : "doomguard", p, spell )
  {
    harmful = false;
    background = true;
    dual       = true;
    callbacks  = false;
    summoning_duration = data().duration();
    summoning_duration += ( p -> set_bonus.tier13_2pc_caster() ) ?
                          ( p -> specialization() == WARLOCK_DEMONOLOGY ?
                            timespan_t::from_seconds( p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 1 ).base_value() ) :
                            timespan_t::from_seconds( p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 2 ).base_value() ) ) : timespan_t::zero();
  }
};


struct summon_doomguard_t : public warlock_spell_t
{
  summon_doomguard2_t* summon_doomguard2;

  summon_doomguard_t( warlock_t* p ) :
    warlock_spell_t( p, p -> talents.grimoire_of_supremacy -> ok() ? "Summon Terrorguard" : "Summon Doomguard" ),
    summon_doomguard2( 0 )
  {
    cooldown -> duration += ( p -> set_bonus.tier13_2pc_caster() ) ? timespan_t::from_millis( p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 3 ).base_value() ) : timespan_t::zero();

    harmful = false;
    summon_doomguard2 = new summon_doomguard2_t( p, data().effectN( 2 ).trigger() );
    summon_doomguard2 -> stats = stats;
    summon_doomguard2 -> pet -> summon_stats = stats;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    p() -> cooldowns.infernal -> start();
    summon_doomguard2 -> execute();
  }
};


// TALENT SPELLS

struct harvest_life_heal_t : public warlock_heal_t
{
  harvest_life_heal_t( warlock_t* p ) :
    warlock_heal_t( "harvest_life_heal", p, 89653 )
  {
    background = true;
    may_miss = false;
  }

  void perform( bool main_target )
  {
    double heal_pct = ( main_target ) ? 0.033 : 0.0025; // FIXME: Does not match tooltip at all, retest later
    base_dd_min = base_dd_max = player -> resources.max[ RESOURCE_HEALTH ] * heal_pct;

    execute();
  }
};

struct harvest_life_tick_t : public warlock_spell_t
{
  harvest_life_heal_t* heal;
  player_t* main_target;

  harvest_life_tick_t( warlock_t* p ) :
    warlock_spell_t( "harvest_life_tick", p, p -> find_spell( 115707 ) ),
    main_target( 0 )
  {
    aoe         = -1;
    background  = true;
    may_miss    = false;
    direct_tick_callbacks = true;
    base_dd_min = base_dd_max = base_td;
    direct_power_mod = tick_power_mod;
    num_ticks = 0;

    heal = new harvest_life_heal_t( p );
  }

  virtual void execute()
  {
    main_target = target;

    warlock_spell_t::execute();
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    heal -> perform( main_target == target );

    if ( p() -> specialization() == WARLOCK_DEMONOLOGY && ! p() -> buffs.metamorphosis -> check() )
      p() -> resource_gain( RESOURCE_DEMONIC_FURY, main_target == target ? 10 : 3, gain ); // FIXME: This may be a bug, retest later
  }
};

struct harvest_life_t : public warlock_spell_t
{
  harvest_life_t( warlock_t* p ) :
    warlock_spell_t( "harvest_life", p, p -> talents.harvest_life )
  {
    // FIXME: Harvest Life is actually an aoe channel, not a channeled spell with aoe ticks
    channeled    = true;
    hasted_ticks = false;
    may_crit     = false;

    tick_power_mod = base_td = 0;

    tick_action = new harvest_life_tick_t( p );
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    consume_tick_resource( d );
  }
};


struct mortal_coil_heal_t : public warlock_heal_t
{
  mortal_coil_heal_t( warlock_t* p, const spell_data_t& s ) :
    warlock_heal_t( "mortal_coil_heal", p, s.effectN( 3 ).trigger_spell_id() )
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
    base_dd_min = base_dd_max = 0;
    heal = new mortal_coil_heal_t( p, data() );
  }

  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      heal -> execute();
  }
};


struct grimoire_of_sacrifice_t : public warlock_spell_t
{
  grimoire_of_sacrifice_t( warlock_t* p ) :
    warlock_spell_t( "grimoire_of_sacrifice", p, p -> talents.grimoire_of_sacrifice )
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
      p() -> pets.active = 0;
      p() -> buffs.grimoire_of_sacrifice -> trigger();

      // FIXME: Demonic rebirth should really trigger on any pet death, but this is the only pet death we care about for now
      if ( p() -> spec.demonic_rebirth -> ok() )
        p() -> buffs.demonic_rebirth -> trigger();

      // FIXME: Add heal event.
    }
  }
};


struct grimoire_of_service_t : public summon_pet_t
{
  grimoire_of_service_t( warlock_t* p, const std::string& pet_name ) :
    summon_pet_t( "service_" + pet_name, p, p -> talents.grimoire_of_service -> ok() ? p -> find_class_spell( "Grimoire: " + pet_name ) : spell_data_t::not_found() )
  {
    cooldown = p -> get_cooldown( "grimoire_of_service" );
    cooldown -> duration = data().cooldown();
    summoning_duration = data().duration();
    if ( pet )
      pet -> summon_stats = stats;
  }
};


struct archimondes_vengeance_dmg_t : public warlock_spell_t
{
  archimondes_vengeance_dmg_t( warlock_t* p ) :
    warlock_spell_t( "archimondes_vengeance_dmg", p )
  {
    // FIXME: Should this have proc=true and/or callbacks=false?
    trigger_gcd = timespan_t::zero();
    may_crit = false;
    may_miss = false;
    background = true;
  }
};

struct archimondes_vengeance_t : public warlock_spell_t
{
  archimondes_vengeance_t( warlock_t* p ) :
    warlock_spell_t( "archimondes_vengeance", p, p -> talents.archimondes_vengeance )
  {
    harmful = false;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    p() -> buffs.archimondes_vengeance -> trigger();
  }
};

} // UNNAMED NAMESPACE ====================================================


double warlock_t::composite_player_multiplier( school_e school, action_t* a )
{
  double m = player_t::composite_player_multiplier( school, a );

  double mastery_value = composite_mastery() * mastery_spells.master_demonologist -> effectN( 1 ).mastery_value();

  if ( buffs.metamorphosis -> up() && a -> id != 172 ) // FIXME: Is corruption an exception, or did they change it so it only applies to a few spells specifically?
  {
    m *= 1.0 + spec.demonic_fury -> effectN( 1 ).percent() * 3 + mastery_value * 3;
  }
  else
  {
    m *= 1.0 + mastery_value;
  }

  return m;
}


double warlock_t::composite_spell_crit()
{
  double sc = player_t::composite_spell_crit();

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    if ( buffs.dark_soul -> up() )
      sc += spec.dark_soul -> effectN( 1 ).percent() * ( 1.0 - glyphs.dark_soul -> effectN( 1 ).percent() );
    else if ( buffs.dark_soul -> cooldown -> up() )
      sc += spec.dark_soul -> effectN( 1 ).percent() * glyphs.dark_soul -> effectN( 1 ).percent();
  }

  return sc;
}


double warlock_t::composite_spell_haste()
{
  double h = player_t::composite_spell_haste();

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    if ( buffs.dark_soul -> up() )
      h *= 1.0 / ( 1.0 + spec.dark_soul -> effectN( 1 ).percent() * ( 1.0 - glyphs.dark_soul -> effectN( 1 ).percent() ) );
    else if ( buffs.dark_soul -> cooldown -> up() )
      h *= 1.0 / ( 1.0 + spec.dark_soul -> effectN( 1 ).percent() * glyphs.dark_soul -> effectN( 1 ).percent() );
  }

  return h;
}


double warlock_t::composite_mastery()
{
  double m = player_t::composite_mastery();

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    if ( buffs.dark_soul -> up() )
      m += spec.dark_soul -> effectN( 1 ).average( this ) * ( 1.0 - glyphs.dark_soul -> effectN( 1 ).percent() ) / rating.mastery;
    else if ( buffs.dark_soul -> cooldown -> up() )
      m += spec.dark_soul -> effectN( 1 ).average( this ) * glyphs.dark_soul -> effectN( 1 ).percent() / rating.mastery;
  }

  return m;
}


double warlock_t::mana_regen_per_second()
{
  double mp5 = player_t::mana_regen_per_second();

  mp5 /= composite_spell_haste();

  return mp5;
}


double warlock_t::composite_armor()
{
  return player_t::composite_armor() + spec.fel_armor -> effectN( 2 ).base_value();
}


double warlock_t::composite_movement_speed()
{
  double s = player_t::composite_movement_speed();

  // FIXME: This won't really work as it should, since movement speed is just checked once when the movement event starts
  if ( ! buffs.kiljaedens_cunning -> up() && buffs.kiljaedens_cunning -> cooldown -> up() )
    s *= ( 1.0 - kc_movement_reduction );

  return s;
}


void warlock_t::moving()
{
  // FIXME: Handle the situation where the buff is up but the movement duration is longer than the duration of the buff
  if ( ! talents.kiljaedens_cunning -> ok() ||
       ( ! buffs.kiljaedens_cunning -> up() && buffs.kiljaedens_cunning -> cooldown -> down() ) )
    player_t::moving();
}


void warlock_t::halt()
{
  player_t::halt();

  if ( spells.melee ) spells.melee -> cancel();
}


void warlock_t::assess_damage( school_e school,
                               dmg_e    type,
                               action_state_t* s )
{
  player_t::assess_damage( school, type, s );

  double m = talents.archimondes_vengeance -> effectN( 1 ).percent();

  if ( ! buffs.archimondes_vengeance -> up() )
  {
    if ( buffs.archimondes_vengeance -> cooldown -> up() )
      m /= 6.0;
    else
      m = 0;
  }

  if ( m > 0 )
  {
    // FIXME: Needs testing! Should it include absorbed damage? Should it be unaffected by damage buffs?
    spells.archimondes_vengeance_dmg -> base_dd_min = spells.archimondes_vengeance_dmg -> base_dd_max = s -> result_amount * m;
    spells.archimondes_vengeance_dmg -> target = s -> action -> player;
    spells.archimondes_vengeance_dmg -> school = school; // FIXME: Assuming school is the school of the incoming damage for now
    spells.archimondes_vengeance_dmg -> execute();
  }
}


double warlock_t::matching_gear_multiplier( attribute_e attr )
{
  if ( attr == ATTR_INTELLECT )
    return spec.nethermancy -> effectN( 1 ).percent();

  return 0.0;
}

static const std::string supremacy_pet( const std::string pet_name, bool translate = true )
{
  if ( ! translate ) return pet_name;
  if ( pet_name == "felhunter" )  return "observer";
  if ( pet_name == "felguard" )   return "wrathguard";
  if ( pet_name == "succubus" )   return "shivarra";
  if ( pet_name == "voidwalker" ) return "voidlord";
  if ( pet_name == "imp" )        return "fel imp";
  return "";
}

action_t* warlock_t::create_action( const std::string& action_name,
                                    const std::string& options_str )
{
  action_t* a;

  if ( ( action_name == "summon_pet" || action_name == "service_pet" ) && default_pet.empty() )
  {
    sim -> errorf( "Player %s used a generic pet summoning action without specifying a default_pet.\n", name() );
    return 0;
  }

  if      ( action_name == "conflagrate"           ) a = new           conflagrate_t( this );
  else if ( action_name == "corruption"            ) a = new            corruption_t( this );
  else if ( action_name == "agony"                 ) a = new                 agony_t( this );
  else if ( action_name == "doom"                  ) a = new                  doom_t( this );
  else if ( action_name == "chaos_bolt"            ) a = new            chaos_bolt_t( this );
  else if ( action_name == "chaos_wave"            ) a = new            chaos_wave_t( this );
  else if ( action_name == "curse_of_the_elements" ) a = new curse_of_the_elements_t( this );
  else if ( action_name == "touch_of_chaos"        ) a = new        touch_of_chaos_t( this );
  else if ( action_name == "drain_life"            ) a = new            drain_life_t( this );
  else if ( action_name == "drain_soul"            ) a = new            drain_soul_t( this );
  else if ( action_name == "grimoire_of_sacrifice" ) a = new grimoire_of_sacrifice_t( this );
  else if ( action_name == "haunt"                 ) a = new                 haunt_t( this );
  else if ( action_name == "immolate"              ) a = new              immolate_t( this );
  else if ( action_name == "incinerate"            ) a = new            incinerate_t( this );
  else if ( action_name == "life_tap"              ) a = new              life_tap_t( this );
  else if ( action_name == "malefic_grasp"         ) a = new         malefic_grasp_t( this );
  else if ( action_name == "metamorphosis"         ) a = new activate_metamorphosis_t( this );
  else if ( action_name == "cancel_metamorphosis"  ) a = new  cancel_metamorphosis_t( this );
  else if ( action_name == "melee"                 ) a = new        activate_melee_t( this );
  else if ( action_name == "mortal_coil"           ) a = new           mortal_coil_t( this );
  else if ( action_name == "shadow_bolt"           ) a = new           shadow_bolt_t( this );
  else if ( action_name == "shadowburn"            ) a = new            shadowburn_t( this );
  else if ( action_name == "soul_fire"             ) a = new             soul_fire_t( this );
  else if ( action_name == "unstable_affliction"   ) a = new   unstable_affliction_t( this );
  else if ( action_name == "hand_of_guldan"        ) a = new        hand_of_guldan_t( this );
  else if ( action_name == "fel_flame"             ) a = new             fel_flame_t( this );
  else if ( action_name == "void_ray"              ) a = new              void_ray_t( this );
  else if ( action_name == "dark_intent"           ) a = new           dark_intent_t( this );
  else if ( action_name == "dark_soul"             ) a = new             dark_soul_t( this );
  else if ( action_name == "soulburn"              ) a = new              soulburn_t( this );
  else if ( action_name == "havoc"                 ) a = new                 havoc_t( this );
  else if ( action_name == "seed_of_corruption"    ) a = new    seed_of_corruption_t( this );
  else if ( action_name == "rain_of_fire"          ) a = new          rain_of_fire_t( this );
  else if ( action_name == "hellfire"              ) a = new              hellfire_t( this );
  else if ( action_name == "immolation_aura"       ) a = new       immolation_aura_t( this );
  else if ( action_name == "carrion_swarm"         ) a = new         carrion_swarm_t( this );
  else if ( action_name == "imp_swarm"             ) a = new             imp_swarm_t( this );
  else if ( action_name == "fire_and_brimstone"    ) a = new    fire_and_brimstone_t( this );
  else if ( action_name == "soul_swap"             ) a = new             soul_swap_t( this );
  else if ( action_name == "flames_of_xoroth"      ) a = new      flames_of_xoroth_t( this );
  else if ( action_name == "harvest_life"          ) a = new          harvest_life_t( this );
  else if ( action_name == "archimondes_vengeance" ) a = new archimondes_vengeance_t( this );
  else if ( action_name == "summon_infernal"       ) a = new       summon_infernal_t( this );
  else if ( action_name == "summon_doomguard"      ) a = new      summon_doomguard_t( this );
  else if ( action_name == "summon_felhunter"      ) a = new summon_main_pet_t( supremacy_pet( "felhunter",  talents.grimoire_of_supremacy -> ok() ), this );
  else if ( action_name == "summon_felguard"       ) a = new summon_main_pet_t( supremacy_pet( "felguard",   talents.grimoire_of_supremacy -> ok() ), this );
  else if ( action_name == "summon_succubus"       ) a = new summon_main_pet_t( supremacy_pet( "succubus",   talents.grimoire_of_supremacy -> ok() ), this );
  else if ( action_name == "summon_voidwalker"     ) a = new summon_main_pet_t( supremacy_pet( "voidwalker", talents.grimoire_of_supremacy -> ok() ), this );
  else if ( action_name == "summon_imp"            ) a = new summon_main_pet_t( supremacy_pet( "imp",        talents.grimoire_of_supremacy -> ok() ), this );
  else if ( action_name == "summon_pet"            ) a = new summon_main_pet_t( supremacy_pet( default_pet,  talents.grimoire_of_supremacy -> ok() ), this );
  else if ( action_name == "service_felguard"      ) a = new grimoire_of_service_t( this, "felguard" );
  else if ( action_name == "service_felhunter"     ) a = new grimoire_of_service_t( this, "felhunter" );
  else if ( action_name == "service_imp"           ) a = new grimoire_of_service_t( this, "imp" );
  else if ( action_name == "service_succubus"      ) a = new grimoire_of_service_t( this, "succubus" );
  else if ( action_name == "service_voidwalker"    ) a = new grimoire_of_service_t( this, "voidwalker" );
  else if ( action_name == "service_pet"           ) a = new grimoire_of_service_t( this, default_pet );
  else return player_t::create_action( action_name, options_str );

  a -> parse_options( 0, options_str );

  return a;
}


pet_t* warlock_t::create_pet( const std::string& pet_name,
                              const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  using namespace pets;

  if ( pet_name == "felguard"     ) return new    felguard_pet_t( sim, this );
  if ( pet_name == "felhunter"    ) return new   felhunter_pet_t( sim, this );
  if ( pet_name == "imp"          ) return new         imp_pet_t( sim, this );
  if ( pet_name == "succubus"     ) return new    succubus_pet_t( sim, this );
  if ( pet_name == "voidwalker"   ) return new  voidwalker_pet_t( sim, this );
  if ( pet_name == "infernal"     ) return new    infernal_pet_t( sim, this );
  if ( pet_name == "doomguard"    ) return new   doomguard_pet_t( sim, this );

  if ( pet_name == "wrathguard"   ) return new  wrathguard_pet_t( sim, this );
  if ( pet_name == "observer"     ) return new    observer_pet_t( sim, this );
  if ( pet_name == "fel_imp"      ) return new     fel_imp_pet_t( sim, this );
  if ( pet_name == "shivarra"     ) return new    shivarra_pet_t( sim, this );
  if ( pet_name == "voidlord"     ) return new    voidlord_pet_t( sim, this );
  if ( pet_name == "abyssal"      ) return new     abyssal_pet_t( sim, this );
  if ( pet_name == "terrorguard"  ) return new terrorguard_pet_t( sim, this );

  if ( pet_name == "service_felguard"     ) return new    felguard_pet_t( sim, this, pet_name );
  if ( pet_name == "service_felhunter"    ) return new   felhunter_pet_t( sim, this, pet_name );
  if ( pet_name == "service_imp"          ) return new         imp_pet_t( sim, this, pet_name );
  if ( pet_name == "service_succubus"     ) return new    succubus_pet_t( sim, this, pet_name );
  if ( pet_name == "service_voidwalker"   ) return new  voidwalker_pet_t( sim, this, pet_name );

  return 0;
}


void warlock_t::create_pets()
{
  create_pet( "felhunter"  );
  create_pet( "imp"        );
  create_pet( "succubus"   );
  create_pet( "voidwalker" );
  create_pet( "infernal"   );
  create_pet( "doomguard"  );

  create_pet( "observer"    );
  create_pet( "fel_imp"     );
  create_pet( "shivarra"    );
  create_pet( "voidlord"    );
  create_pet( "abyssal"     );
  create_pet( "terrorguard" );

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    create_pet( "felguard"   );
    create_pet( "wrathguard" );

    create_pet( "service_felguard" );

    for ( int i = 0; i < WILD_IMP_LIMIT; i++ )
    {
      pets.wild_imps[ i ] = new pets::wild_imp_pet_t( sim, this );
      if ( i > 0 )
        pets.wild_imps[ i ] -> quiet = 1;
    }
  }

  create_pet( "service_felhunter"  );
  create_pet( "service_imp"        );
  create_pet( "service_succubus"   );
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
    { 123136, 123141,     0,     0,     0,     0,     0,     0 }, // Tier14
    {      0,      0,     0,     0,     0,     0,     0,     0 },
  };
  sets                        = new set_bonus_array_t( this, set_bonuses );

  // General
  spec.nethermancy = find_spell( 86091 );
  spec.fel_armor   = find_spell( 104938 );
  spec.pandemic    = find_spell( 131973 );

  spec.dark_soul = find_specialization_spell( "Dark Soul: Instability", "dark_soul" );
  if ( ! spec.dark_soul -> ok() ) spec.dark_soul = find_specialization_spell( "Dark Soul: Knowledge", "dark_soul" );
  if ( ! spec.dark_soul -> ok() ) spec.dark_soul = find_specialization_spell( "Dark Soul: Misery", "dark_soul" );

  // Affliction
  spec.nightfall     = find_specialization_spell( "Nightfall" );

  // Demonology
  spec.decimation      = find_specialization_spell( "Decimation" );
  spec.demonic_fury    = find_specialization_spell( "Demonic Fury" );
  spec.metamorphosis   = find_specialization_spell( "Metamorphosis" );
  spec.molten_core     = find_specialization_spell( "Molten Core" );
  spec.demonic_rebirth = find_specialization_spell( "Demonic Rebirth" );
  spec.wild_imps       = find_specialization_spell( "Wild Imps" );

  spec.doom           = ( find_specialization_spell( "Metamorphosis: Doom"           ) -> ok() ) ? find_spell( 603 )    : spell_data_t::not_found();
  spec.touch_of_chaos = ( find_specialization_spell( "Metamorphosis: Touch of Chaos" ) -> ok() ) ? find_spell( 103964 ) : spell_data_t::not_found();
  spec.chaos_wave     = ( find_specialization_spell( "Metamorphosis: Chaos Wave"     ) -> ok() ) ? find_spell( 124916 ) : spell_data_t::not_found();

  // Destruction
  spec.aftermath      = find_specialization_spell( "Aftermath" );
  spec.backdraft      = find_specialization_spell( "Backdraft" );
  spec.burning_embers = find_specialization_spell( "Burning Embers" );
  spec.chaotic_energy = find_specialization_spell( "Chaotic Energy" );
  spec.pyroclasm      = find_specialization_spell( "Pyroclasm" );

  mastery_spells.emberstorm          = find_mastery_spell( WARLOCK_DESTRUCTION );
  mastery_spells.potent_afflictions  = find_mastery_spell( WARLOCK_AFFLICTION );
  mastery_spells.master_demonologist = find_mastery_spell( WARLOCK_DEMONOLOGY );

  talents.soul_leech            = find_talent_spell( "Soul Leech" );
  talents.harvest_life          = find_talent_spell( "Harvest Life" );
  talents.mortal_coil           = find_talent_spell( "Mortal Coil" );
  talents.shadowfury            = find_talent_spell( "Shadowfury" );
  talents.soul_link             = find_talent_spell( "Soul Link" );
  talents.grimoire_of_supremacy = find_talent_spell( "Grimoire of Supremacy" );
  talents.grimoire_of_service   = find_talent_spell( "Grimoire of Service" );
  talents.grimoire_of_sacrifice = find_talent_spell( "Grimoire of Sacrifice" );
  talents.archimondes_vengeance = find_talent_spell( "Archimonde's Vengeance" );
  talents.kiljaedens_cunning    = find_talent_spell( "Kil'jaeden's Cunning" );

  glyphs.conflagrate            = find_glyph_spell( "Glyph of Conflagrate" );
  glyphs.dark_soul              = find_glyph_spell( "Glyph of Dark Soul" );
  glyphs.demon_training         = find_glyph_spell( "Glyph of Demon Training" );
  glyphs.life_tap               = find_glyph_spell( "Glyph of Life Tap" );
  glyphs.imp_swarm              = find_glyph_spell( "Glyph of Imp Swarm" );
  glyphs.soul_shards            = find_glyph_spell( "Glyph of Soul Shards" );
  glyphs.burning_embers         = find_glyph_spell( "Glyph of Burning Embers" );
  glyphs.soul_swap              = find_glyph_spell( "Glyph of Soul Swap" );
  glyphs.shadow_bolt            = find_glyph_spell( "Glyph of Shadow Bolt" );
  glyphs.siphon_life            = find_glyph_spell( "Glyph of Siphon Life" );
  glyphs.everlasting_affliction = find_glyph_spell( "Everlasting Affliction" );

  spec.imp_swarm = ( glyphs.imp_swarm -> ok() ) ? find_spell( 104316 ) : spell_data_t::not_found();

  spells.archimondes_vengeance_dmg = new archimondes_vengeance_dmg_t( this );

  kc_movement_reduction = ( talents.kiljaedens_cunning -> ok() ) ? find_spell( 108507 ) -> effectN( 2 ).percent() : 0;
  kc_cast_speed_reduction = ( talents.kiljaedens_cunning -> ok() ) ? find_spell( 108507 ) -> effectN( 1 ).percent() : 0;
}


void warlock_t::init_base()
{
  player_t::init_base();

  base.attack_power = -10;
  initial.attack_power_per_strength = 2.0;
  initial.spell_power_per_intellect = 1.0;

  initial.attribute_multiplier[ ATTR_STAMINA ] *= 1.0 + spec.fel_armor -> effectN( 1 ).percent();

  // If we don't have base mp5 defined for this level in extra_data.inc, approximate:
  if ( base.mana_regen_per_second == 0 ) base.mana_regen_per_second = resources.base[ RESOURCE_MANA ] * 0.01;

  base.mana_regen_per_second *= 1.0 + spec.chaotic_energy -> effectN( 1 ).percent();

  if ( specialization() == WARLOCK_AFFLICTION )  resources.base[ RESOURCE_SOUL_SHARD ]    = 3 + ( ( glyphs.soul_shards -> ok() ) ? 1 : 0 );
  if ( specialization() == WARLOCK_DEMONOLOGY )  resources.base[ RESOURCE_DEMONIC_FURY ]  = 1000;
  if ( specialization() == WARLOCK_DESTRUCTION ) resources.base[ RESOURCE_BURNING_EMBER ] = 3 + ( ( glyphs.burning_embers -> ok() ) ? 1 : 0 );

  if ( default_pet.empty() )
  {
    if ( specialization() == WARLOCK_DEMONOLOGY )
      default_pet = "felguard";
    else
      default_pet = "felhunter";
  }

  diminished_kfactor    = 0.009830;
  diminished_dodge_cap = 0.006650;
  diminished_parry_cap = 0.006650;
}


void warlock_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_SPIRIT ] = 0;
  scales_with[ STAT_STAMINA ] = 0;
}


void warlock_t::create_buffs()
{
  player_t::create_buffs();

  buffs.backdraft             = buff_creator_t( this, "backdraft", spec.backdraft -> effectN( 1 ).trigger() ).max_stack( 6 );
  buffs.dark_soul             = buff_creator_t( this, "dark_soul", spec.dark_soul );
  buffs.metamorphosis         = buff_creator_t( this, "metamorphosis", spec.metamorphosis );
  buffs.molten_core           = buff_creator_t( this, "molten_core", find_spell( 122355 ) ).activated( false ).max_stack( 10 );
  buffs.soulburn              = buff_creator_t( this, "soulburn", find_class_spell( "Soulburn" ) );
  buffs.grimoire_of_sacrifice = buff_creator_t( this, "grimoire_of_sacrifice", talents.grimoire_of_sacrifice );
  buffs.demonic_calling       = buff_creator_t( this, "demonic_calling", spec.wild_imps -> effectN( 1 ).trigger() ).duration( timespan_t::zero() );
  buffs.fire_and_brimstone    = buff_creator_t( this, "fire_and_brimstone", find_class_spell( "Fire and Brimstone" ) );
  buffs.soul_swap             = buff_creator_t( this, "soul_swap", find_spell( 86211 ) );
  buffs.havoc                 = buff_creator_t( this, "havoc", find_class_spell( "Havoc" ) );
  buffs.archimondes_vengeance = buff_creator_t( this, "archimondes_vengeance", talents.archimondes_vengeance );
  buffs.kiljaedens_cunning    = buff_creator_t( this, "kiljaedens_cunning", talents.kiljaedens_cunning );
  buffs.demonic_rebirth       = buff_creator_t( this, "demonic_rebirth", find_spell( 88448 ) ).cd( find_spell( 89140 ) -> duration() );
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
  gains.conflagrate        = get_gain( "conflagrate"  );
  gains.rain_of_fire       = get_gain( "rain_of_fire" );
  gains.immolate           = get_gain( "immolate"     );
  gains.fel_flame          = get_gain( "fel_flame"    );
  gains.shadowburn         = get_gain( "shadowburn"   );
  gains.miss_refund        = get_gain( "miss_refund"  );
  gains.siphon_life        = get_gain( "siphon_life"  );
  gains.seed_of_corruption = get_gain( "seed_of_corruption" );
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

void warlock_t::init_actions()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    std::string& precombat_list = get_action_priority_list( "precombat" ) -> action_list_str;

    if ( sim -> allow_flasks )
    {
      // Flask
      if ( level > 85 )
        precombat_list = "flask,type=warm_sun";
      else if ( level >= 80 )
        precombat_list = "flask,type=draconic_mind";
    }

    if ( sim -> allow_food )
    {
      // Food
      if ( level >= 80 )
      {
        precombat_list += "/food,type=";
        precombat_list += ( level > 85 ) ? "mogu_fish_stew" : "seafood_magnifique_feast";
      }
    }

    add_action( "Dark Intent", "if=!aura.spell_power_multiplier.up", "precombat" );

    precombat_list += "/summon_pet";

    precombat_list += "/snapshot_stats";

    if ( sim -> allow_potions )
    {
      // Pre-potion
      if ( level > 85 )
        precombat_list += "/jade_serpent_potion";
      else if ( level >= 80 )
        precombat_list += "/volcanic_potion";
    }

    add_action( "Curse of the Elements", "if=debuff.magic_vulnerability.down" );

    // Usable Item
    for ( int i = items.size() - 1; i >= 0; i-- )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
      }
    }

    if ( sim -> allow_potions )
    {
      // Potion
      if ( level > 85 )
        action_list_str += "/jade_serpent_potion,if=buff.bloodlust.react|target.health.pct<=20";
      else if ( level > 80 )
        action_list_str += "/volcanic_potion,if=buff.bloodlust.react|target.health.pct<=20";
    }

    action_list_str += init_use_profession_actions();
    action_list_str += init_use_racial_actions();

    add_action( spec.dark_soul );

    action_list_str += "/service_pet,if=talent.grimoire_of_service.enabled";

    action_list_str += "/grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled";
    action_list_str += "/summon_pet,if=talent.grimoire_of_sacrifice.enabled&buff.grimoire_of_sacrifice.down";

    if ( specialization() == WARLOCK_DEMONOLOGY )
    {
      if ( find_class_spell( "Metamorphosis" ) -> ok() ) action_list_str += "/melee";
      action_list_str += "/felguard:felstorm";
      action_list_str += "/wrathguard:wrathstorm";
    }

    int multidot_max = 3;

    switch ( specialization() )
    {
    case WARLOCK_AFFLICTION:  multidot_max = 3; break;
    case WARLOCK_DESTRUCTION: multidot_max = 2; break;
    case WARLOCK_DEMONOLOGY:  multidot_max = 3; break;
    default: break;
    }

    action_list_str += "/run_action_list,name=aoe,if=active_enemies>" + util::to_string( multidot_max );

    add_action( "Summon Doomguard" );
    add_action( "Summon Doomguard", "if=active_enemies<7", "aoe" );
    add_action( "Summon Infernal", "if=active_enemies>=7", "aoe" );

    switch ( specialization() )
    {

    case WARLOCK_AFFLICTION:
      add_action( "Soul Swap",             "if=buff.soulburn.up" );
      add_action( "Haunt",                 "if=!in_flight_to_target&remains<tick_time+travel_time+cast_time&shard_react&miss_react" );
      add_action( "Soul Swap",             "cycle_targets=1,if=active_enemies>1&time<10&glyph.soul_swap.enabled" );
      add_action( "Haunt",                 "cycle_targets=1,if=!in_flight_to_target&remains<tick_time+travel_time+cast_time&soul_shard>1&miss_react" );
      if ( spec.pandemic -> ok() )
      {
        add_action( "Soulburn",            "line_cd=20,if=buff.dark_soul.up&shard_react" );
        add_action( "Soulburn",            "if=(dot.unstable_affliction.ticks_remain<action.unstable_affliction.add_ticks%2|dot.corruption.ticks_remain<action.corruption.add_ticks%2|dot.agony.ticks_remain<action.agony.add_ticks%2)&target.health.pct<=20&shard_react" );
        add_action( "Agony",               "cycle_targets=1,if=ticks_remain<add_ticks%2&target.time_to_die>=8&miss_react" );
        add_action( "Corruption",          "cycle_targets=1,if=ticks_remain<add_ticks%2&target.time_to_die>=6&miss_react" );
        add_action( "Unstable Affliction", "cycle_targets=1,if=ticks_remain<add_ticks%2+1&target.time_to_die>=5&miss_react" );
      }
      else
      {
        add_action( "Soulburn",            "line_cd=15,if=buff.dark_soul.up&shard_react" );
        add_action( "Agony",               "cycle_targets=1,if=(!ticking|remains<=action.drain_soul.new_tick_time*2)&target.time_to_die>=8&miss_react" );
        add_action( "Corruption",          "cycle_targets=1,if=(!ticking|remains<tick_time)&target.time_to_die>=6&miss_react" );
        add_action( "Unstable Affliction", "cycle_targets=1,if=(!ticking|remains<(cast_time+tick_time))&target.time_to_die>=5&miss_react" );
      }
      add_action( "Drain Soul",            "interrupt=1,chain=1,if=target.health.pct<=20" );
      add_action( "Life Tap",              "if=mana.pct<35" );
      add_action( "Malefic Grasp",         "chain=1" );
      add_action( "Life Tap",              "moving=1,if=mana.pct<80&mana.pct<target.health.pct" );
      add_action( "Fel Flame",             "moving=1" );

      // AoE action list
      add_action( "Soulburn",              "cycle_targets=1,if=buff.soulburn.down&!dot.soulburn_seed_of_corruption.ticking&!action.soulburn_seed_of_corruption.in_flight_to_target&shard_react", "aoe" );
      add_action( "Soul Swap",             "if=buff.soulburn.up&!dot.agony.ticking&!dot.corruption.ticking",                     "aoe" );
      add_action( "Soul Swap",             "cycle_targets=1,if=buff.soulburn.up&dot.corruption.ticking&!dot.agony.ticking",      "aoe" );
      add_action( "Seed of Corruption",    "cycle_targets=1,if=(buff.soulburn.down&!in_flight_to_target&!ticking)|(buff.soulburn.up&!dot.soulburn_seed_of_corruption.ticking&!action.soulburn_seed_of_corruption.in_flight_to_target)", "aoe" );
      add_action( "Haunt",                 "cycle_targets=1,if=!in_flight_to_target&debuff.haunt.remains<cast_time+travel_time&shard_react", "aoe" );
      add_action( "Life Tap",              "if=mana.pct<70",                                                                     "aoe" );
      add_action( "Fel Flame",             "cycle_targets=1,if=!in_flight_to_target",                                            "aoe" );
      break;

    case WARLOCK_DESTRUCTION:
      add_action( "Havoc",                 "target=2,if=active_enemies>1" );
      add_action( "Shadowburn",            "if=ember_react" );
      if ( spec.pandemic -> ok() )
        add_action( "Immolate",            "cycle_targets=1,if=ticks_remain<add_ticks%2&target.time_to_die>=5&miss_react" );
      else
        add_action( "Immolate",            "cycle_targets=1,if=(!ticking|remains<(action.incinerate.cast_time+cast_time))&target.time_to_die>=5&miss_react" );
      add_action( "Chaos Bolt",            "if=ember_react&(buff.backdraft.stack<3|level<86)&(burning_ember>3.5|buff.dark_soul.remains>cast_time)&mana.pct<=80" );
      add_action( "Conflagrate" );
      add_action( "Incinerate" );
      add_action( "Chaos Bolt",            "if=burning_ember>2&mana.pct<10" );

      // AoE action list
      add_action( "Rain of Fire",          "if=!ticking&!in_flight",                                 "aoe" );
      add_action( "Fire and Brimstone",    "if=ember_react&buff.fire_and_brimstone.down",            "aoe" );
      add_action( "Immolate",              "if=buff.fire_and_brimstone.up&!ticking",                 "aoe" );
      add_action( "Conflagrate",           "if=ember_react&buff.fire_and_brimstone.up",              "aoe" );
      add_action( "Incinerate",            "if=buff.fire_and_brimstone.up",                          "aoe" );
      add_action( "Immolate",              "cycle_targets=1,if=!ticking",                            "aoe" );
      break;

    case WARLOCK_DEMONOLOGY:
      add_action( "Corruption",            "cycle_targets=1,if=(!ticking|remains<tick_time)&target.time_to_die>=6&miss_react" );
      add_action( spec.doom,               "cycle_targets=1,if=(!ticking|remains<tick_time|(ticks_remain+1<n_ticks&buff.dark_soul.up))&target.time_to_die>=30&miss_react" );
      add_action( "Metamorphosis",         "if=buff.dark_soul.up|dot.corruption.remains<5|demonic_fury>=900|demonic_fury>=target.time_to_die*30" );

      if ( find_class_spell( "Metamorphosis" ) -> ok() )
        action_list_str += "/cancel_metamorphosis,if=dot.corruption.remains>20&buff.dark_soul.down&demonic_fury<=750&target.time_to_die>30";

      add_action( spec.imp_swarm );
      add_action( "Hand of Gul'dan",       "if=!in_flight&dot.shadowflame.remains<travel_time+action.shadow_bolt.cast_time" );
      add_action( spec.touch_of_chaos,     "cycle_targets=1,if=dot.corruption.remains<20" );
      add_action( "Soul Fire",             "if=buff.molten_core.react" );
      add_action( spec.touch_of_chaos );

      add_action( "Life Tap",              "if=mana.pct<50" );
      add_action( "Shadow Bolt" );
      add_action( "Fel Flame",             "moving=1" );

      // AoE action list
      add_action( spec.imp_swarm,          "if=buff.metamorphosis.down",                                                       "aoe" );
      add_action( "Corruption",            "cycle_targets=1,if=(!ticking|remains<tick_time)&target.time_to_die>30&miss_react", "aoe" );
      add_action( "Hand of Gul'dan",       "",                                                                                 "aoe" );
      add_action( "Metamorphosis",         "if=demonic_fury>=1000|demonic_fury>=31*target.time_to_die",                        "aoe" );
      add_action( "Immolation Aura",       "",                                                                                 "aoe" );
      add_action( "Void Ray",              "if=dot.corruption.remains<10",                                                     "aoe" );
      add_action( spec.doom,               "cycle_targets=1,if=(!ticking|remains<40)&target.time_to_die>30&miss_react",        "aoe" );
      add_action( "Void Ray",              "",                                                                                 "aoe" );

      get_action_priority_list( "aoe" ) -> action_list_str += "/harvest_life,chain=1,if=talent.harvest_life.enabled";

      add_action( "Hellfire",              "if=!talent.harvest_life.enabled",                                                  "aoe" );
      add_action( "Life Tap",              "",                                                                                 "aoe" );
      break;

    default:
      add_action( "Corruption",            "if=(!ticking|remains<tick_time)&target.time_to_die>=6&miss_react" );
      add_action( "Shadow Bolt" );

      // AoE action list
      add_action( "Corruption",            "cycle_targets=1,if=!ticking",                                                      "aoe" );
      add_action( "Shadow Bolt",           "",                                                                                 "aoe" );
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

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    buffs.demonic_calling -> trigger();
    demonic_calling_event = new ( sim ) demonic_calling_event_t( this, rngs.demonic_calling -> range( timespan_t::zero(),
        timespan_t::from_seconds( ( spec.wild_imps -> effectN( 1 ).period().total_seconds() + spec.imp_swarm -> effectN( 3 ).base_value() ) * composite_spell_haste() ) ) );
  }
}


void warlock_t::reset()
{
  player_t::reset();

  for ( size_t i=0; i < sim -> actor_list.size(); i++ )
  {
    warlock_td_t* td = target_data[ sim -> actor_list[ i ] ];
    if ( td ) td -> reset();
  }

  pets.active = 0;
  ember_react = ( initial_burning_embers >= 1.0 ) ? timespan_t::zero() : timespan_t::max();
  shard_react = timespan_t::zero();
  nightfall_chance = 0;
  event_t::cancel( demonic_calling_event );
}


void warlock_t::create_options()
{
  player_t::create_options();

  option_t warlock_options[] =
  {
    opt_int( "burning_embers", initial_burning_embers ),
    opt_int( "demonic_fury", initial_demonic_fury ),
    opt_string( "default_pet", default_pet ),
    opt_null()
  };

  option_t::copy( options, warlock_options );
}


bool warlock_t::create_profile( std::string& profile_str, save_e stype, bool save_html )
{
  player_t::create_profile( profile_str, stype, save_html );

  if ( stype == SAVE_ALL )
  {
    if ( initial_burning_embers != 1 ) profile_str += "burning_embers=" + util::to_string( initial_burning_embers ) + "\n";
    if ( initial_demonic_fury != 200 ) profile_str += "demonic_fury="   + util::to_string( initial_demonic_fury ) + "\n";
    if ( ! default_pet.empty() )       profile_str += "default_pet="    + default_pet + "\n";
  }

  return true;
}


void warlock_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warlock_t* p = debug_cast<warlock_t*>( source );

  initial_burning_embers = p -> initial_burning_embers;
  initial_demonic_fury   = p -> initial_demonic_fury;
  default_pet            = p -> default_pet;
}


int warlock_t::decode_set( item_t& item )
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

  if ( strstr( s, "shaskin_"               ) ) return SET_T14_CASTER;

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
      virtual double evaluate() { return player.resources.current[ RESOURCE_BURNING_EMBER ] >= 1 && player.sim -> current_time >= player.ember_react; }
    };
    return new ember_react_expr_t( *this );
  }
  else if ( name_str == "shard_react" )
  {
    struct shard_react_expr_t : public expr_t
    {
      warlock_t& player;
      shard_react_expr_t( warlock_t& p ) :
        expr_t( "shard_react" ), player( p ) { }
      virtual double evaluate() { return player.resources.current[ RESOURCE_SOUL_SHARD ] >= 1 && player.sim -> current_time >= player.shard_react; }
    };
    return new shard_react_expr_t( *this );
  }
  else if ( name_str == "felstorm_is_ticking" )
  {
    struct felstorm_is_ticking_expr_t : public expr_t
    {
      pets::warlock_pet_t* felguard;
      felstorm_is_ticking_expr_t( pets::warlock_pet_t* f ) :
        expr_t( "felstorm_is_ticking" ), felguard( f ) { }
      virtual double evaluate() { return ( felguard ) ? felguard -> special_action -> get_dot() -> ticking : false; }
    };
    return new felstorm_is_ticking_expr_t( debug_cast<pets::warlock_pet_t*>( find_pet( "felguard" ) ) );
  }
  else
  {
    return player_t::create_expression( a, name_str );
  }
}


// WARLOCK MODULE INTERFACE ================================================

struct warlock_module_t : public module_t
{
  warlock_module_t() : module_t( WARLOCK ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    return new warlock_t( sim, name, r );
  }
  virtual bool valid() const { return true; }
  virtual void init        ( sim_t* ) const {}
  virtual void combat_begin( sim_t* ) const {}
  virtual void combat_end  ( sim_t* ) const {}
};

} // end unnamed namespace

const module_t& module_t::warlock_()
{
  static warlock_module_t m = warlock_module_t();
  return m;
}
