// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Mage
// ==========================================================================

#if SC_MAGE == 1

enum mage_rotation_e { ROTATION_NONE=0, ROTATION_DPS, ROTATION_DPM, ROTATION_MAX };

struct mage_targetdata_t : public targetdata_t
{
  dot_t* dots_frostfire_bolt;
  dot_t* dots_ignite;
  dot_t* dots_living_bomb;
  dot_t* dots_pyroblast;

  buff_t* debuffs_slow;

  mage_targetdata_t( mage_t* source, player_t* target );
};

void register_mage_targetdata( sim_t* sim )
{
  player_type_e t = MAGE;
  typedef mage_targetdata_t type;

  REGISTER_DOT( frostfire_bolt );
  REGISTER_DOT( ignite );
  REGISTER_DOT( living_bomb );
  REGISTER_DOT( pyroblast );

  REGISTER_DEBUFF( slow );
}

struct mage_t : public player_t
{
  // Active
  spell_t* active_ignite;
  struct pets_t
  {
    pet_t* water_elemental;
    pet_t* mirror_image_3;
    pets_t() { memset( this, 0, sizeof( *this ) ); }
  } pets;

  // Buffs
  struct buffs_t
  {
    buff_t* arcane_charge;
    buff_t* arcane_missiles;
    buff_t* arcane_potency;
    buff_t* arcane_power;
    buff_t* brain_freeze;
    buff_t* fingers_of_frost;
    buff_t* frost_armor;
    buff_t* hot_streak;
    buff_t* hot_streak_crits;
    buff_t* icy_veins;
    buff_t* improved_mana_gem;
    buff_t* incanters_absorption;
    buff_t* invocation;
    buff_t* mage_armor;
    buff_t* missile_barrage;
    buff_t* molten_armor;
    buff_t* presence_of_mind;
    buff_t* tier13_2pc;
    buffs_t() { memset( this, 0, sizeof( *this ) ); }
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* deep_freeze;
    cooldown_t* early_frost;
    cooldown_t* evocation;
    cooldown_t* fire_blast;
    cooldown_t* mana_gem;
    cooldowns_t() { memset( this, 0, sizeof( *this ) ); }
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* clearcasting;
    gain_t* empowered_fire;
    gain_t* evocation;
    gain_t* frost_armor;
    gain_t* mage_armor;
    gain_t* mana_gem;
    gain_t* master_of_elements;
    gains_t() { memset( this, 0, sizeof( *this ) ); }
  } gains;

  // Glyphs
  struct glyphs_t
  {
    // Prime
    const spell_data_t* arcane_barrage;
    const spell_data_t* arcane_blast;
    const spell_data_t* arcane_missiles;
    const spell_data_t* arcane_power;
    const spell_data_t* cone_of_cold;
    const spell_data_t* deep_freeze;
    const spell_data_t* fireball;
    const spell_data_t* frost_armor;
    const spell_data_t* frostbolt;
    const spell_data_t* frostfire;
    const spell_data_t* ice_lance;
    const spell_data_t* living_bomb;
    const spell_data_t* mage_armor;
    const spell_data_t* molten_armor;
    const spell_data_t* pyroblast;

    // Major
    const spell_data_t* dragons_breath;
    const spell_data_t* mirror_image;

    // Minor
    const spell_data_t* arcane_brilliance;
    const spell_data_t* conjuring;
    glyphs_t() { memset( this, 0, sizeof( *this ) ); }
  } glyphs;

  // Options
  std::string focus_magic_target_str;
  double merge_ignite;
  timespan_t ignite_sampling_delta;

  // Passives
  struct passives_t
  {
    const spell_data_t* shatter;
    const spell_data_t* improved_counterspell;
    const spell_data_t* nether_attunement;
    const spell_data_t* burning_soul;

    passives_t() { memset( this, 0, sizeof( *this ) ); }
  } passives;

  // Spell Data
  struct spells_t
  {
    const spell_data_t* arcane_missiles;
    const spell_data_t* arcane_power;
    const spell_data_t* frost_armor;
    const spell_data_t* hot_streak;
    const spell_data_t* icy_veins;
    const spell_data_t* mage_armor;
    const spell_data_t* molten_armor;

    const spell_data_t* flashburn;
    const spell_data_t* frostburn;
    const spell_data_t* mana_adept;

    const spell_data_t* arcane_specialization;
    const spell_data_t* fire_specialization;
    const spell_data_t* frost_specialization;

    const spell_data_t* blink;

    const spell_data_t* stolen_time;

    spells_t() { memset( this, 0, sizeof( *this ) ); }
  } spells;

  // Specializations
  struct specializations_t
  {
    // Arcane
    const spell_data_t* arcane_charge;
    const spell_data_t* mana_adept;

    // Fire
    const spell_data_t* critical_mass;
    const spell_data_t* ignite;

    // Frost
    const spell_data_t* brain_freeze;
    const spell_data_t* fingers_of_frost;
    const spell_data_t* frostburn;

    specializations_t() { memset( this, 0, sizeof( *this ) ); }
  } spec;

  // Procs
  struct procs_t
  {
    proc_t* deferred_ignite;
    proc_t* munched_ignite;
    proc_t* rolled_ignite;
    proc_t* mana_gem;
    proc_t* early_frost;
    proc_t* test_for_crit_hotstreak;
    proc_t* crit_for_hotstreak;
    proc_t* hotstreak;
    proc_t* improved_hotstreak;
    procs_t() { memset( this, 0, sizeof( *this ) ); }
  } procs;

  // Random Number Generation
  struct rngs_t
  {
    rng_t* arcane_missiles;
    rng_t* empowered_fire;
    rng_t* enduring_winter;
    rng_t* fire_power;
    rng_t* impact;
    rng_t* improved_freeze;
    rng_t* nether_vortex;
    rng_t* mage_armor_start;
    rngs_t() { memset( this, 0, sizeof( *this ) ); }
  } rngs;

  // Rotation (DPS vs DPM)
  struct rotation_t
  {
    mage_rotation_e current;
    double mana_gain;
    double dps_mana_loss;
    double dpm_mana_loss;
    timespan_t dps_time;
    timespan_t dpm_time;
    timespan_t last_time;

    void reset() { memset( this, 0, sizeof( *this ) ); current = ROTATION_DPS; }
    rotation_t() { reset(); }
  };
  rotation_t rotation;

  // Talents
  struct talents_list_t
  {
    const spell_data_t* presence_of_mind;
    const spell_data_t* scorch;
    const spell_data_t* ice_flows;
    const spell_data_t* temporal_shield;
    const spell_data_t* blazing_speed;
    const spell_data_t* ice_barrier;
    const spell_data_t* ring_of_frost;
    const spell_data_t* ice_ward;
    const spell_data_t* frostjaw;
    const spell_data_t* greater_invis;
    const spell_data_t* cauterize;
    const spell_data_t* cold_snap;
    const spell_data_t* nether_tempest;
    const spell_data_t* living_bomb;
    const spell_data_t* frost_bomb;
    const spell_data_t* invocation;
    const spell_data_t* rune_of_power;
    const spell_data_t* incanters_ward;

    talents_list_t() { memset( this, 0, sizeof( *this ) ); }
  } talents;

  // Up-Times
  struct benefits_t
  {
    benefit_t* arcane_blast[ 5 ];
    benefit_t* dps_rotation;
    benefit_t* dpm_rotation;
    benefit_t* water_elemental;
    benefits_t() { memset( this, 0, sizeof( *this ) ); }
  } benefits;

  int mana_gem_charges;
  timespan_t mage_armor_timer;

  mage_t( sim_t* sim, const std::string& name, race_type_e r = RACE_NIGHT_ELF ) :
    player_t( sim, MAGE, name, r ),
    ignite_sampling_delta( timespan_t::from_seconds( 0.2 ) )
  {
    // Active
    active_ignite = 0;

    // Pets
    pets.water_elemental = 0;
    pets.mirror_image_3  = 0;

    // Cooldowns
    cooldowns.deep_freeze = get_cooldown( "deep_freeze" );
    cooldowns.early_frost = get_cooldown( "early_frost" );
    cooldowns.evocation   = get_cooldown( "evocation"   );
    cooldowns.fire_blast  = get_cooldown( "fire_blast"  );
    cooldowns.mana_gem    = get_cooldown( "mana_gem"    );

    // Options
    distance         = 40;
    default_distance = 40;
    mana_gem_charges = 3;
    merge_ignite     = 0;

    create_options();
  }

  // Character Definition
  virtual mage_targetdata_t* new_targetdata( player_t* target )
  { return new mage_targetdata_t( this, target ); }
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_values();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_benefits();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      reset();
  virtual expr_t*   create_expression( action_t*, const std::string& name );
  virtual void      create_options();
  virtual bool      create_profile( std::string& profile_str, save_type_e=SAVE_ALL, bool save_html=false );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual void      copy_from( player_t* source );
  virtual int       decode_set( const item_t& item ) const;
  virtual resource_type_e primary_resource() const { return RESOURCE_MANA; }
  virtual role_type_e primary_role() const { return ROLE_SPELL; }
  virtual double    composite_armor_multiplier() const;
  virtual double    composite_player_multiplier( school_type_e school, const action_t* a = NULL ) const;
  virtual double    composite_spell_crit() const;
  virtual double    composite_spell_resistance( school_type_e school ) const;
  virtual double    matching_gear_multiplier( attribute_type_e attr ) const;
  virtual void      stun();

  // Event Tracking
  virtual void   regen( timespan_t periodicity );
  virtual double resource_gain( resource_type_e resource, double amount, gain_t* source=0, action_t* action=0 );
  virtual double resource_loss( resource_type_e resource, double amount, action_t* action=0 );
};

mage_targetdata_t::mage_targetdata_t( mage_t* source, player_t* target )
  : targetdata_t( source, target )
{
  debuffs_slow = add_aura( buff_creator_t( this, "slow", source -> find_spell( 31589 ) ) );
}

namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Mage Spell
// ==========================================================================

struct mage_spell_t : public spell_t
{
  bool may_chill, may_hot_streak, may_brain_freeze;
  bool fof_frozen, consumes_arcane_blast;
  int dps_rotation;
  int dpm_rotation;

  mage_spell_t( const std::string& n, mage_t* p,
                const spell_data_t* s = spell_data_t::nil(), school_type_e sc = SCHOOL_NONE ) :
    spell_t( n, p, s, sc ),
    may_chill( false ), may_hot_streak( false ), may_brain_freeze( false ),
    fof_frozen( false ), consumes_arcane_blast( false ),
    dps_rotation( 0 ), dpm_rotation( 0 )
  {
    may_crit      = ( base_dd_min > 0 ) && ( base_dd_max > 0 );
    tick_may_crit = true;
    crit_multiplier *= 1.33;
  }

  mage_t* p() const
  { return static_cast<mage_t*>( player ); }

  virtual void   parse_options( option_t*, const std::string& );
  virtual bool   ready();
  virtual double cost() const;
  virtual double haste() const;
  virtual void   execute();
  virtual timespan_t execute_time() const;
  virtual void   impact( player_t* t, result_type_e impact_result, double travel_dmg );
  virtual void   player_buff();
  virtual double total_crit() const;
  virtual double hot_streak_crit() { return player_crit; }
};

// ==========================================================================
// Pet Water Elemental
// ==========================================================================

struct water_elemental_pet_t : public pet_t
{
  struct freeze_t : public spell_t
  {
    freeze_t( water_elemental_pet_t* player, const std::string& options_str ):
      spell_t( "freeze", player, player -> find_spell( 33395 ) )
    {
      parse_options( NULL, options_str );
      aoe = -1;
      may_crit = true;
      crit_multiplier *= 1.33;
    }

    virtual void player_buff()
    {
      spell_t::player_buff();

      water_elemental_pet_t* p = static_cast<water_elemental_pet_t*>( player );

      player_spell_power = p -> o() -> composite_spell_power( SCHOOL_FROST ) * p -> o() -> composite_spell_power_multiplier() * 0.4;
      player_crit = p -> o() -> composite_spell_crit(); // Needs testing, but closer than before
    }
  };

  struct water_bolt_t : public spell_t
  {
    water_bolt_t( water_elemental_pet_t* player, const std::string& options_str ):
      spell_t( "water_bolt", player, player -> find_spell( 31707 ) )
    {
      parse_options( NULL, options_str );
      may_crit = true;
      crit_multiplier *= 1.33;
      direct_power_mod = 0.833;
    }

    virtual void player_buff()
    {
      spell_t::player_buff();

      water_elemental_pet_t* p = static_cast<water_elemental_pet_t*>( player );

      player_spell_power = p -> o() -> composite_spell_power( SCHOOL_FROST ) * p -> o() -> composite_spell_power_multiplier() * 0.4;
      player_crit = p -> o() -> composite_spell_crit(); // Needs testing, but closer than before
    }
  };

  water_elemental_pet_t( sim_t* sim, mage_t* owner ) :
    pet_t( sim, owner, "water_elemental" )
  {
    action_list_str  = "freeze,if=owner.buff.fingers_of_frost.stack=0";
    action_list_str += "/water_bolt";
    create_options();
  }

  mage_t* o() const
  { return static_cast<mage_t*>( owner ); }

  virtual void init_base()
  {
    pet_t::init_base();

    // Stolen from Priest's Shadowfiend
    attribute_base[ ATTR_STRENGTH  ] = 145;
    attribute_base[ ATTR_AGILITY   ] =  38;
    attribute_base[ ATTR_STAMINA   ] = 190;
    attribute_base[ ATTR_INTELLECT ] = 133;

    //health_per_stamina = 7.5;
    mana_per_intellect = 5;
  }

  virtual double composite_spell_haste() const
  {
    double h = player_t::composite_spell_haste();
    h *= owner -> spell_haste;
    return h;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "freeze"     ) return new     freeze_t( this, options_str );
    if ( name == "water_bolt" ) return new water_bolt_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Pet Mirror Image
// ==========================================================================

struct mirror_image_pet_t : public pet_t
{
  int num_images;
  int num_rotations;
  std::vector<action_t*> sequences;
  int sequence_finished;
  double snapshot_arcane_sp;
  double snapshot_fire_sp;
  double snapshot_frost_sp;

  mirror_image_pet_t( sim_t* sim, mage_t* owner ) :
    pet_t( sim, owner, "mirror_image_3", true /*guardian*/ ),
    num_images( 3 ), num_rotations( 2 ), sequence_finished( 0 )
  {}

  mage_t* o() const
  { return static_cast<mage_t*>( owner ); }

  struct arcane_blast_t : public spell_t
  {
    action_t* next_in_sequence;

    arcane_blast_t( mirror_image_pet_t* p, action_t* nis ):
      spell_t( "mirror_arcane_blast", p, p -> find_spell( 88084 ) ), next_in_sequence( nis )
    {
      may_crit          = true;
      background        = true;
    }

    mirror_image_pet_t* p() const
    { return static_cast<mirror_image_pet_t*>( player ); }

    virtual void player_buff()
    {
      spell_t::player_buff();
      double ab_stack_multiplier = p() -> o() -> spec.arcane_charge -> effect1().percent();
      double ab_glyph_multiplier = p() -> o() -> glyphs.arcane_blast -> effect1().percent();
      player_multiplier *= 1.0 + p() -> o() ->  buffs.arcane_charge -> stack() * ( ab_stack_multiplier + ab_glyph_multiplier );
    }

    virtual void execute()
    {
      spell_t::execute();
      if ( next_in_sequence )
      {
        next_in_sequence -> schedule_execute();
      }
      else
      {
        p() -> sequence_finished++;

        if ( p() -> sequence_finished == p() -> num_images )
          p() -> dismiss();
      }
    }
  };

  struct fire_blast_t : public spell_t
  {
    action_t* next_in_sequence;

    fire_blast_t( mirror_image_pet_t* p, action_t* nis ):
      spell_t( "mirror_fire_blast", p, p -> find_spell( 59637 ) ),
      next_in_sequence( nis )
    {
      background        = true;
      may_crit          = true;
    }

    virtual void execute()
    {
      spell_t::execute();
      if ( next_in_sequence )
      {
        next_in_sequence -> schedule_execute();
      }
      else
      {
        mirror_image_pet_t* mi = static_cast<mirror_image_pet_t*>( player );
        mi -> sequence_finished++;
        if ( mi -> sequence_finished == mi -> num_images ) mi -> dismiss();
      }
    }
  };

  struct fireball_t : public spell_t
  {
    action_t* next_in_sequence;

    fireball_t( mirror_image_pet_t* p, action_t* nis ):
      spell_t( "mirror_fireball", p, p -> find_spell( 88082 ) ),
      next_in_sequence( nis )
    {
      may_crit          = true;
      background        = true;
    }

    virtual void execute()
    {
      spell_t::execute();
      if ( next_in_sequence )
      {
        next_in_sequence -> schedule_execute();
      }
      else
      {
        mirror_image_pet_t* mi = static_cast<mirror_image_pet_t*>( player );
        mi -> sequence_finished++;
        if ( mi -> sequence_finished == mi -> num_images ) mi -> dismiss();
      }
    }
  };

  struct frostbolt_t : public spell_t
  {
    action_t* next_in_sequence;

    frostbolt_t( mirror_image_pet_t* p, action_t* nis ):
      spell_t( "mirror_frost_bolt", p, p -> find_spell( 59638 ) ),
      next_in_sequence( nis )
    {
      may_crit          = true;
      background        = true;
    }

    virtual void execute()
    {
      spell_t::execute();
      if ( next_in_sequence )
      {
        next_in_sequence -> schedule_execute();
      }
      else
      {
        mirror_image_pet_t* mi = static_cast<mirror_image_pet_t*>( player );
        mi -> sequence_finished++;
        if ( mi -> sequence_finished == mi -> num_images ) mi -> dismiss();
      }
    }
  };

  virtual void init_base()
  {
    pet_t::init_base();

    // Stolen from Priest's Shadowfiend
    attribute_base[ ATTR_STRENGTH  ] = 145;
    attribute_base[ ATTR_AGILITY   ] =  38;
    attribute_base[ ATTR_STAMINA   ] = 190;
    attribute_base[ ATTR_INTELLECT ] = 133;

    //health_per_stamina = 7.5;
    mana_per_intellect = 5;
  }

  virtual void init_actions()
  {
    for ( int i = 0; i < num_images; i++ )
    {
      action_t* front=0;

      if ( o() -> glyphs.mirror_image -> ok() && o() -> primary_tree() != MAGE_FROST )
      {
        // Fire/Arcane Mages cast 9 Fireballs/Arcane Blasts
        num_rotations = 9;
        for ( int j = 0; j < num_rotations; j++ )
        {
          if ( o() -> primary_tree() == MAGE_FIRE )
          {
            front = new fireball_t ( this, front );
          }
          else
          {
            front = new arcane_blast_t ( this, front );
          }
        }
      }
      else
      {
        // Mirror Image casts 11 Frostbolts, 4 Fire Blasts
        front = new fire_blast_t( this, front );
        front = new frostbolt_t ( this, front );
        front = new frostbolt_t ( this, front );
        front = new frostbolt_t ( this, front );
        front = new fire_blast_t( this, front );
        front = new frostbolt_t ( this, front );
        front = new frostbolt_t ( this, front );
        front = new frostbolt_t ( this, front );
        front = new fire_blast_t( this, front );
        front = new frostbolt_t ( this, front );
        front = new frostbolt_t ( this, front );
        front = new frostbolt_t ( this, front );
        front = new fire_blast_t( this, front );
        front = new frostbolt_t ( this, front );
        front = new frostbolt_t ( this, front );
      }
      sequences.push_back( front );
    }

    pet_t::init_actions();
  }

  virtual double composite_spell_power( school_type_e school ) const
  {
    if ( school == SCHOOL_ARCANE )
    {
      return snapshot_arcane_sp * 0.75;
    }
    else if ( school == SCHOOL_FIRE )
    {
      return snapshot_fire_sp * 0.75;
    }
    else if ( school == SCHOOL_FROST )
    {
      return snapshot_frost_sp * 0.75;
    }

    return 0;
  }

  virtual void summon( timespan_t duration=timespan_t::zero() )
  {
    pet_t::summon( duration );

    snapshot_arcane_sp = o() -> composite_spell_power( SCHOOL_ARCANE );
    snapshot_fire_sp   = o() -> composite_spell_power( SCHOOL_FIRE   );
    snapshot_frost_sp  = o() -> composite_spell_power( SCHOOL_FROST  );

    sequence_finished = 0;

    distance = o() -> distance;

    for ( int i = 0; i < num_images; i++ )
    {
      sequences[ i ] -> schedule_execute();
    }
  }

  virtual void halt()
  {
    pet_t::halt();
    dismiss(); // FIXME! Interrupting them is too hard, just dismiss for now.
  }
};

// calculate_dot_dps ========================================================

static double calculate_dot_dps( dot_t* dot )
{
  if ( ! dot -> ticking ) return 0;

  action_t* a = dot -> action;

  a -> result = RESULT_HIT;

  return ( a -> calculate_tick_damage( a -> result, a -> total_power(), a -> total_td_multiplier() ) / a -> base_tick_time.total_seconds() );
}

// consume_brain_freeze =====================================================

static void consume_brain_freeze( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if ( p -> buffs.brain_freeze -> check() )
  {
    p -> buffs.brain_freeze -> expire();
  }
}

// trigger_brain_freeze =====================================================

static void trigger_brain_freeze( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();
  double chance = 0.0;

  if ( ! p -> spec.brain_freeze -> ok() ) return;

  chance = p -> spec.brain_freeze -> proc_chance();

  p -> buffs.brain_freeze -> trigger( 1, -1.0, chance );
}

// trigger_hot_streak =======================================================

static void trigger_hot_streak( mage_spell_t* s )
{
  mage_t* p = s -> p();

  if ( ! s -> may_hot_streak )
    return;

  if ( p -> primary_tree() != MAGE_FIRE )
    return;

  int result = s -> result;

  p -> procs.test_for_crit_hotstreak -> occur();

  if ( result == RESULT_CRIT )
  {
    p -> procs.crit_for_hotstreak -> occur();
    // Reference: http://elitistjerks.com/f75/t110326-cataclysm_fire_mage_compendium/p6/#post1831143

    double hot_streak_chance = -2.73 * s -> hot_streak_crit() + 0.95;

    if ( hot_streak_chance < 0.0 ) hot_streak_chance = 0.0;

    if ( hot_streak_chance > 0 && p -> buffs.hot_streak -> trigger( 1, 0, hot_streak_chance ) )
    {
      p -> procs.hotstreak -> occur();
      p -> buffs.hot_streak_crits -> expire();
    }
  }
  else
  {
    p -> buffs.hot_streak_crits -> expire();
  }
}

// trigger_ignite ===========================================================

static void trigger_ignite( mage_spell_t* s, double dmg )
{
  if ( s -> school != SCHOOL_FIRE &&
       s -> school != SCHOOL_FROSTFIRE ) return;

  mage_t* p = s -> p();
  sim_t* sim = s -> sim;

  if ( ! p -> spec.ignite -> ok() ) return;

  struct ignite_sampling_event_t : public event_t
  {
    player_t* target;
    double crit_ignite_bank;
    timespan_t application_delay;
    ignite_sampling_event_t( sim_t* sim, player_t* t, mage_spell_t* a, double crit_ignite_bank ) :
      event_t( sim, a -> player ), target( t ), crit_ignite_bank( crit_ignite_bank )
    {
      name = "Ignite Sampling";

      if ( sim -> debug )
        log_t::output( sim, "New Ignite Sampling Event: %s",
                       player -> name() );

      timespan_t delay = sim -> aura_delay - p() -> ignite_sampling_delta;
      sim -> add_event( this, sim -> gauss( delay, 0.25 * delay ) );
    }

    mage_t* p() const
    { return static_cast<mage_t*>( player ); }

    virtual void execute()
    {
      mage_t* p = ignite_sampling_event_t::p();

      struct ignite_t : public mage_spell_t
      {
        ignite_t( mage_t* player ) :
          mage_spell_t( "ignite", player, player -> find_spell( 12654 ) )
        {
          background    = true;

          // FIXME: Needs verification wheter it triggers trinket tick callbacks or not. It does trigger DTR arcane ticks.
          // proc          = false;

          tick_may_crit = false;
          hasted_ticks  = false;
          dot_behavior  = DOT_REFRESH;
          init();
        }
        virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
        {
          mage_spell_t::impact( t, impact_result, 0 );

          base_td = travel_dmg;
        }
        virtual timespan_t travel_time() const
        {
          mage_t* p = static_cast<mage_t*>( player );
          return sim -> gauss( p -> ignite_sampling_delta, 0.25 * p -> ignite_sampling_delta );
        }
        virtual double total_td_multiplier() const { return 1.0; }
      };

      if ( ! p -> active_ignite ) p -> active_ignite = new ignite_t( p );

      dot_t* dot = p -> active_ignite -> dot();

      double ignite_dmg = crit_ignite_bank;

      if ( dot -> ticking )
      {
        ignite_dmg += p -> active_ignite -> base_td * dot -> ticks();
        ignite_dmg /= 3.0;
      }
      else
      {
        ignite_dmg /= 2.0;
      }

      // TODO: investigate if this can actually happen
      /*if ( timespan_t::from_seconds( 4.0 ) + sim -> aura_delay < dot -> remains() )
      {
        if ( sim -> log ) log_t::output( sim, "Player %s munches Ignite due to Max Ignite Duration.", p -> name() );
        p -> procs.munched_ignite -> occur();
        return;
      }*/

      if ( p -> active_ignite -> travel_event )
      {
        // There is an SPELL_AURA_APPLIED already in the queue, which will get munched.
        if ( sim -> log ) log_t::output( sim, "Player %s munches previous Ignite due to Aura Delay.", p -> name() );
        p -> procs.munched_ignite -> occur();
      }

      p -> active_ignite -> direct_dmg = ignite_dmg;
      p -> active_ignite -> result = RESULT_HIT;
      p -> active_ignite -> schedule_travel( target );

      dot -> prev_tick_amount = ignite_dmg;

      if ( p -> active_ignite -> travel_event && dot -> ticking )
      {
        if ( dot -> tick_event -> occurs() < p -> active_ignite -> travel_event -> occurs() )
        {
          // Ignite will tick before SPELL_AURA_APPLIED occurs, which means that the current Ignite will
          // both tick -and- get rolled into the next Ignite.
          if ( sim -> log ) log_t::output( sim, "Player %s rolls Ignite.", p -> name() );
          p -> procs.rolled_ignite -> occur();
        }
      }
    }
  };

  double ignite_dmg = dmg * p -> spec.ignite -> effect1().percent();

  if ( p -> primary_tree() == MAGE_FIRE )
  {
    ignite_dmg *= 1.0 + p -> spec.ignite -> effectN( 1 ).coeff() * 0.01 * p -> composite_mastery();
  }

  if ( p -> merge_ignite > 0 ) // Does not report Ignite seperately.
  {
    result_type_e result = s -> result;
    s -> result = RESULT_HIT;
    s -> assess_damage( s -> target, ignite_dmg * p -> merge_ignite, DMG_OVER_TIME, s -> result );
    s -> result = result;
    return;
  }

  new ( sim ) ignite_sampling_event_t( sim, s -> target, s, ignite_dmg );
}

// ==========================================================================
// Mage Spell
// ==========================================================================

// mage_spell_t::parse_options ==============================================

void mage_spell_t::parse_options( option_t*          options,
                                  const std::string& options_str )
{
  option_t base_options[] =
  {
    { "dps", OPT_BOOL, &dps_rotation },
    { "dpm", OPT_BOOL, &dpm_rotation },
    { NULL, OPT_UNKNOWN, NULL }
  };
  std::vector<option_t> merged_options;
  spell_t::parse_options( option_t::merge( merged_options, options, base_options ), options_str );
}

// mage_spell_t::ready ======================================================

bool mage_spell_t::ready()
{
  if ( dps_rotation )
    if ( p() -> rotation.current != ROTATION_DPS )
      return false;

  if ( dpm_rotation )
    if ( p() -> rotation.current != ROTATION_DPM )
      return false;

  return spell_t::ready();
}

// mage_spell_t::cost =======================================================

double mage_spell_t::cost() const
{
  double c = spell_t::cost();

  if ( p() -> buffs.arcane_power -> check() )
  {
    double m = 1.0 + p() -> buffs.arcane_power -> effect2().percent();

    c *= m;
  }

  return c;
}

// mage_spell_t::haste ======================================================

double mage_spell_t::haste() const
{
  double h = spell_t::haste();
  if ( p() -> buffs.icy_veins -> up() )
  {
    h *= 1.0 / ( 1.0 + p() -> buffs.icy_veins -> effect1().percent() );
  }
  return h;
}

// mage_spell_t::execute ====================================================

void mage_spell_t::execute()
{
  p() -> benefits.dps_rotation -> update( p() -> rotation.current == ROTATION_DPS );
  p() -> benefits.dpm_rotation -> update( p() -> rotation.current == ROTATION_DPM );

  spell_t::execute();

  if ( background )
    return;

  if ( consumes_arcane_blast )
    p() -> buffs.arcane_charge -> expire();

  if ( ( base_dd_max > 0 || base_td > 0 ) && ! background )
    p() -> buffs.arcane_potency -> decrement();

  if ( ! channeled && spell_t::execute_time() > timespan_t::zero() )
    p() -> buffs.presence_of_mind -> expire();

  if ( fof_frozen )
  {
    p() -> buffs.fingers_of_frost -> decrement();
  }

  if ( may_brain_freeze )
  {
    trigger_brain_freeze( this );
  }

  if ( result_is_hit() )
  {
    trigger_hot_streak( this );
  }

  if ( p() -> primary_tree() != MAGE_FIRE &&
       ! p() -> spec.brain_freeze -> ok() )
  {
    p() -> buffs.arcane_missiles -> trigger();
  }
}

// mage_spell_t::execute_time ===============================================

timespan_t mage_spell_t::execute_time() const
{
  timespan_t t = spell_t::execute_time();

  if ( ! channeled && t > timespan_t::zero() && p() -> buffs.presence_of_mind -> up() )
    return timespan_t::zero();

  return t;
}

// mage_spell_t::impact =====================================================

void mage_spell_t::impact( player_t* t, const result_type_e impact_result, const double travel_dmg )
{
  spell_t::impact( t, impact_result, travel_dmg );

  if ( impact_result == RESULT_CRIT )
  {
    trigger_ignite( this, direct_dmg );
  }

  if ( may_chill )
  {
    if ( result_is_hit( impact_result ) )
    {
      p() -> buffs.fingers_of_frost -> trigger();
    }
  }
}


// mage_spell_t::player_buff ================================================

void mage_spell_t::player_buff()
{
  spell_t::player_buff();

  if ( fof_frozen && p() -> buffs.fingers_of_frost -> up() )
  {
    player_multiplier *= 1.0 + p() -> spec.frostburn -> effectN( 1 ).coeff() * 0.01 * p() -> composite_mastery();
  }

  if ( sim -> debug )
    log_t::output( sim, "mage_spell_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f mult=%.2f",
                   name(), player_hit, player_crit, player_spell_power, player_multiplier );
}

// mage_spell_t::total_crit =================================================

double mage_spell_t::total_crit() const
{
  double crit = spell_t::total_crit();

  if ( fof_frozen && p() -> buffs.fingers_of_frost -> up() )
  {
    if ( p() -> passives.shatter -> ok() )
    {
      // Hardcoded, doubles crit + 40%
      crit *= 2.0;
      crit += 0.4;
     }
  }

  return crit;
}

// ==========================================================================
// Mage Spells
// ==========================================================================

// Arcane Barrage Spell =====================================================

struct arcane_barrage_t : public mage_spell_t
{
  arcane_barrage_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "arcane_barrage", p, p -> find_spell( 44425 ) )
  {
    check_spec( MAGE_ARCANE );
    parse_options( NULL, options_str );

    base_multiplier *= 1.0 + p -> glyphs.arcane_barrage -> effect1().percent();
    consumes_arcane_blast = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new arcane_barrage_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }
};

// Arcane Blast Spell =======================================================

struct arcane_blast_t : public mage_spell_t
{
  arcane_blast_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "arcane_blast", p, p -> find_spell( 30451 ) )
  {
    parse_options( NULL, options_str );

    if ( p -> set_bonus.pvp_4pc_caster() )
      base_multiplier *= 1.05;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new arcane_blast_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual double cost() const
  {
    mage_t* p = player -> cast_mage();

    double c = spell_t::cost();
    double base_cost = c;
    double stack_cost = 0;

    // Arcane Power only affects the initial base cost, it doesn't inflate the stack cost too
    // ( BaseCost * AP ) + ( BaseCost * 1.5 * ABStacks )
    if ( p -> buffs.arcane_power -> check() )
    {
      c *= 1.0 + p -> buffs.arcane_power -> effect2().percent();
    }

    if ( p -> buffs.arcane_charge -> check() )
    {
      stack_cost = base_cost * p -> buffs.arcane_charge -> stack() * p -> spec.arcane_charge -> effect2().percent();
    }

    c += stack_cost;

    return c;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    for ( int i=0; i < 5; i++ )
    {
      p -> benefits.arcane_blast[ i ] -> update( i == p -> buffs.arcane_charge -> stack() );
    }
    mage_spell_t::execute();

    p -> buffs.arcane_charge -> trigger();

    if ( result_is_hit() )
    {
      if ( p -> set_bonus.tier13_2pc_caster() )
        p -> buffs.tier13_2pc -> trigger( 1, -1, 1 );
    }
  }

  virtual void player_buff()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::player_buff();
    double ab_stack_multiplier = p -> spec.arcane_charge -> effect1().percent();
    double ab_glyph_multiplier = p -> glyphs.arcane_blast -> effect1().percent();
    player_multiplier *= 1.0 + p ->  buffs.arcane_charge -> stack() * ( ab_stack_multiplier + ab_glyph_multiplier );
  }
};

// Arcane Brilliance Spell ==================================================

struct arcane_brilliance_t : public mage_spell_t
{
  arcane_brilliance_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_brilliance", p, p -> find_spell( 1459 ) )
  {
    parse_options( NULL, options_str );

    base_costs[ current_resource() ] *= 1.0 + p -> glyphs.arcane_brilliance -> effect1().percent();
    harmful = false;
    background = ( sim -> overrides.spell_power_multiplier != 0 && sim -> overrides.critical_strike != 0 );
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    if ( ! sim -> overrides.spell_power_multiplier )
      sim -> auras.spell_power_multiplier -> trigger();

    if ( ! sim -> overrides.critical_strike )
      sim -> auras.critical_strike -> trigger();
  }
};

// Arcane Explosion Spell ===================================================

struct arcane_explosion_t : public mage_spell_t
{
  arcane_explosion_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_explosion", p, p -> find_spell( 1449 ) )
  {
    parse_options( NULL, options_str );
    aoe = -1;
  }
};

// Arcane Missiles Spell ====================================================

struct arcane_missiles_tick_t : public mage_spell_t
{
  arcane_missiles_tick_t( mage_t* p, bool dtr=false ) :
    mage_spell_t( "arcane_missiles_tick", p, p -> find_spell( 7268 ) )
  {
    dual        = true;
    background  = true;
    direct_tick = true;
    base_crit  += p -> glyphs.arcane_missiles -> effect1().percent();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new arcane_missiles_tick_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }
};

struct arcane_missiles_t : public mage_spell_t
{
  arcane_missiles_tick_t* tick_spell;

  arcane_missiles_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_missiles", p, p -> find_spell( 5143 ) )
  {
    parse_options( NULL, options_str );
    channeled = true;
    may_miss = false;
    hasted_ticks = false;

    tick_spell = new arcane_missiles_tick_t( p );
  }

  virtual  void init()
  {
    mage_spell_t::init();

    tick_spell -> stats = stats;
  }
  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> buffs.arcane_missiles -> up();
    p() -> buffs.arcane_missiles -> expire();

    p() -> buffs.arcane_potency -> decrement();
  }

  virtual void last_tick( dot_t* d )
  {
    mage_spell_t::last_tick( d );

    p() -> buffs.arcane_charge -> expire();
  }

  virtual void tick( dot_t* d )
  {
    tick_spell -> execute();
    stats -> add_tick( d -> time_to_tick );
  }

  virtual bool ready()
  {
    if ( ! p() -> buffs.arcane_missiles -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Arcane Power Buff ========================================================

struct arcane_power_buff_t : public buff_t
{
  arcane_power_buff_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "arcane_power", p -> find_specialization_spell( "Arcane Power" ) ) )
  {
    cooldown -> duration = timespan_t::zero(); // CD is managed by the spell
  }

  virtual void expire()
  {
    buff_t::expire();

    mage_t* p = static_cast<mage_t*>( player );
    p -> buffs.tier13_2pc -> expire();
  }
};

// Arcane Power Spell =======================================================

struct arcane_power_t : public mage_spell_t
{
  timespan_t orig_duration;

  arcane_power_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_power", p, p -> find_spell( 12042 ) ),
    orig_duration( timespan_t::zero() )
  {
    check_spec( MAGE_ARCANE );
    parse_options( NULL, options_str );
    harmful = false;
    orig_duration = cooldown -> duration;
  }

  virtual void execute()
  {
    if ( p() -> set_bonus.tier13_4pc_caster() )
      cooldown -> duration = orig_duration + p() -> buffs.tier13_2pc -> check() * p() -> spells.stolen_time -> effect1().time_value();

    mage_spell_t::execute();

    p() -> buffs.arcane_power -> trigger( 1, effect1().percent() );
  }

  virtual bool ready()
  {
    // Can't trigger AP if PoM is up
    if ( p() -> buffs.presence_of_mind -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Blink Spell ==============================================================

struct blink_t : public mage_spell_t
{
  blink_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "blink", p, p -> find_spell( 1953 ) )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    player -> buffs.stunned -> expire();
  }

  virtual timespan_t gcd() const
  {
    if ( p() -> buffs.arcane_power -> check() && p() -> glyphs.arcane_power -> ok() )
      return timespan_t::zero();

    return mage_spell_t::gcd();
  }
};

// Cold Snap Spell ==========================================================

struct cold_snap_t : public mage_spell_t
{
  std::vector<cooldown_t*> cooldown_list;

  cold_snap_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "cold_snap", p, p -> find_spell( 11958 ) )
  {
    check_talent( p -> talents.cold_snap -> ok() );
    parse_options( NULL, options_str );

    harmful = false;

    cooldown_list.push_back( p -> get_cooldown( "cone_of_cold"  ) );
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    for ( size_t i = 0; i < cooldown_list.size(); i++ )
    {
      cooldown_list[ i ] -> reset();
    }
  }
};

// Combustion Spell =========================================================

struct combustion_t : public mage_spell_t
{
  timespan_t orig_duration;

  combustion_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "combustion", p, p -> find_spell( 11129 ) ),
    orig_duration( timespan_t::zero() )
  {
    check_spec( MAGE_FIRE );
    parse_options( NULL, options_str );

    // The "tick" portion of spell is specified in the DBC data in an alternate version of Combustion
    num_ticks      = 10;
    base_tick_time = timespan_t::from_seconds( 1.0 );

    orig_duration = cooldown -> duration;

    may_trigger_dtr = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new combustion_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    double ignite_dmg = 0;
    // Apparently, Combustion double-dips mastery....
    // In addition, Ignite contribution uses current mastery
    // http://elitistjerks.com/f75/t110187-cataclysm_mage_simulators_formulators/p3/#post1824829

    mage_t* p = player -> cast_mage();
    mage_targetdata_t* td = targetdata() -> cast_mage();

    if ( td -> dots_ignite -> ticking )
    {
      ignite_dmg += calculate_dot_dps( td -> dots_ignite );
      ignite_dmg *= 1.0 + p -> spec.ignite -> effectN( 1 ).coeff() * 0.01 * p -> composite_mastery();
    }

    base_td = 0;
    base_td += calculate_dot_dps( td -> dots_frostfire_bolt );
    base_td += ignite_dmg;
    base_td += calculate_dot_dps( td -> dots_living_bomb    ) * ( 1.0 + p -> spec.ignite -> effectN( 1 ).coeff() * 0.01 * p -> composite_mastery() );
    base_td += calculate_dot_dps( td -> dots_pyroblast      ) * ( 1.0 + p -> spec.ignite -> effectN( 1 ).coeff() * 0.01 * p -> composite_mastery() );

    if ( p -> set_bonus.tier13_4pc_caster() )
      cooldown -> duration = orig_duration + p -> buffs.tier13_2pc -> check() * p -> spells.stolen_time -> effect2().time_value();

    mage_spell_t::execute();
  }

  virtual void last_tick( dot_t* d )
  {
    mage_spell_t::last_tick( d );

    p() -> buffs.tier13_2pc -> expire();
  }

  virtual double total_td_multiplier() const
  { return 1.0; } // No double-dipping!
};

// Cone of Cold Spell =======================================================

struct cone_of_cold_t : public mage_spell_t
{
  cone_of_cold_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "cone_of_cold", p, p -> find_spell( 120 ) )
  {
    parse_options( NULL, options_str );
    aoe = -1;
    base_multiplier *= 1.0 + p -> glyphs.cone_of_cold -> effect1().percent();

    may_brain_freeze = true;
    may_chill = true;
  }
};

// Conjure Mana Gem Spell ===================================================

struct conjure_mana_gem_t : public mage_spell_t
{
  conjure_mana_gem_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "conjure_mana_gem", p, p -> find_spell( 759 ) )
  {
    parse_options( NULL, options_str );

    base_costs[ current_resource() ] *= 1.0 + p -> glyphs.conjuring -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> mana_gem_charges = 3;
  }

  virtual bool ready()
  {
    if ( p() -> mana_gem_charges == 3 )
      return false;

    return mage_spell_t::ready();
  }
};

// Counterspell Spell =======================================================

struct counterspell_t : public mage_spell_t
{
  counterspell_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "counterspell", p, p -> find_spell( 2139 ) )
  {
    parse_options( NULL, options_str );
    may_miss = may_crit = false;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> buffs.invocation -> trigger();
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Deep Freeze Spell ========================================================

// FIXME: This no longer does damage
struct deep_freeze_t : public mage_spell_t
{
  deep_freeze_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "deep_freeze", p, p -> find_spell( 71757 ) )
  {
    parse_options( NULL, options_str );

    // The spell data is spread across two separate Spell IDs.  Hard code missing for now.
    base_costs[ current_resource() ] = 0.09 * p -> resources.base[ RESOURCE_MANA ];
    cooldown -> duration = timespan_t::from_seconds( 30.0 );

    fof_frozen = true;
    base_multiplier *= 1.0 + p -> glyphs.deep_freeze -> effect1().percent();
    trigger_gcd = p -> base_gcd;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new deep_freeze_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> buffs.fingers_of_frost -> up();
  }

  virtual bool ready()
  {
    if ( ! p() -> buffs.fingers_of_frost -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Dragon's Breath Spell ====================================================

struct dragons_breath_t : public mage_spell_t
{
  dragons_breath_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "dragons_breath", p, p -> find_spell( 31661 ) )
  {
    parse_options( NULL, options_str );
    aoe = -1;
    cooldown -> duration += p -> glyphs.dragons_breath -> effect1().time_value();
  }
};

// Evocation Spell ==========================================================

struct evocation_t : public mage_spell_t
{
  evocation_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "evocation", p, p -> find_spell( 12051 ) )
  {
    parse_options( NULL, options_str );

    base_tick_time    = timespan_t::from_seconds( 2.0 );
    // FIXME: num_ticks         = ( int ) ( duration() / base_tick_time );
    tick_zero         = true;
    channeled         = true;
    harmful           = false;
    hasted_ticks      = false;
  }

  virtual void tick( dot_t* d )
  {
    mage_spell_t::tick( d );

    double mana = player -> resources.max[ RESOURCE_MANA ] * effect1().percent();
    player -> resource_gain( RESOURCE_MANA, mana, p() -> gains.evocation );
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    // evocation automatically causes a switch to dpm rotation
    if ( p() -> rotation.current == ROTATION_DPS )
    {
      p() -> rotation.dps_time += ( sim -> current_time - p() -> rotation.last_time );
    }
    else if ( p() -> rotation.current == ROTATION_DPM )
    {
      p() -> rotation.dpm_time += ( sim -> current_time - p() -> rotation.last_time );
    }
    p() -> rotation.last_time = sim -> current_time;

    if ( sim -> log )
      log_t::output( sim, "%s switches to DPM spell rotation", player -> name() );
    p() -> rotation.current = ROTATION_DPM;
  }
};

// Fire Blast Spell =========================================================

struct fire_blast_t : public mage_spell_t
{
  fire_blast_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "fire_blast", p, p -> find_spell( 2136 ) )
  {
    parse_options( NULL, options_str );
    may_hot_streak = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new fire_blast_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }
};

// Fireball Spell ===========================================================

struct fireball_t : public mage_spell_t
{
  fireball_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "fireball", p, p -> find_spell( 133 ) )
  {
    parse_options( NULL, options_str );
    base_crit += p -> glyphs.fireball -> effect1().percent();
    may_hot_streak = true;
    if ( p -> set_bonus.pvp_4pc_caster() )
      base_multiplier *= 1.05;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new fireball_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual double cost() const
  {
    if ( p() -> buffs.brain_freeze -> check() )
      return 0;

    return mage_spell_t::cost();
  }

  virtual timespan_t execute_time() const
  {
    if ( p() -> buffs.brain_freeze -> check() )
      return timespan_t::zero();

    return mage_spell_t::execute_time();
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    consume_brain_freeze( this );

    if ( result_is_hit() )
    {
      if ( p() -> set_bonus.tier13_2pc_caster() )
        p() -> buffs.tier13_2pc -> trigger( 1, -1, 0.5 );
    }
  }
};

// Frost Armor Spell ========================================================

struct frost_armor_t : public mage_spell_t
{
  frost_armor_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frost_armor", p, p -> find_spell( 7302 ) )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    if ( sim -> log )
      log_t::output( sim, "%s performs %s", player -> name(), name() );

    update_ready();

    p() -> buffs.molten_armor -> expire();
    p() -> buffs.mage_armor -> expire();
    p() -> buffs.frost_armor -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.frost_armor -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Frostbolt Spell ==========================================================

struct frostbolt_t : public mage_spell_t
{
  frostbolt_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "frostbolt", p, p -> find_spell( 116 ) )
  {
    parse_options( NULL, options_str );
    base_crit += p -> glyphs.frostbolt -> effect1().percent();
    may_chill = true;
    may_brain_freeze = true;
    if ( p -> set_bonus.pvp_4pc_caster() )
      base_multiplier *= 1.05;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new frostbolt_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void schedule_execute()
  {
    mage_spell_t::schedule_execute();

    if ( p() -> cooldowns.early_frost -> remains() == timespan_t::zero() )
    {
      p() -> procs.early_frost -> occur();
      p() -> cooldowns.early_frost -> start( timespan_t::from_seconds( 15.0 ) );
    }
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    if ( result_is_hit() )
    {
      if ( p() -> set_bonus.tier13_2pc_caster() )
        p() -> buffs.tier13_2pc -> trigger( 1, -1, 0.5 );
    }
  }
};

// Frostfire Bolt Spell =====================================================

struct frostfire_bolt_t : public mage_spell_t
{
  int dot_stack;

  frostfire_bolt_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "frostfire_bolt", p, p -> find_spell( 44614 ) ),
    dot_stack( 0 )
  {
    parse_options( NULL, options_str );

    may_chill = true;
    may_hot_streak = true;

    if ( p -> glyphs.frostfire -> ok() )
    {
      base_multiplier *= 1.0 + p -> glyphs.frostfire -> effect1().percent();
      num_ticks = 4;
      base_tick_time = timespan_t::from_seconds( 3.0 );
      dot_behavior = DOT_REFRESH;
    }
    if ( p -> set_bonus.pvp_4pc_caster() )
      base_multiplier *= 1.05;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new frostfire_bolt_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void reset()
  {
    mage_spell_t::reset();
    dot_stack=0;
  }

  virtual void last_tick( dot_t* d )
  {
    mage_spell_t::last_tick( d );
    dot_stack=0;
  }

  virtual double cost() const
  {
    if ( p() -> buffs.brain_freeze -> check() )
      return 0;

    return mage_spell_t::cost();
  }

  virtual timespan_t execute_time() const
  {
    if ( p() -> buffs.brain_freeze -> check() )
      return timespan_t::zero();

    return mage_spell_t::execute_time();
  }

  virtual void execute()
  {
    fof_frozen = p() -> buffs.brain_freeze -> up();

    mage_spell_t::execute();

    consume_brain_freeze( this );

    if ( result_is_hit() )
    {
      if ( p() -> set_bonus.tier13_2pc_caster() )
        p() -> buffs.tier13_2pc -> trigger( 1, -1, 0.5 );
    }
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    if ( p() -> glyphs.frostfire -> ok() && result_is_hit( impact_result ) )
    {
      if ( dot_stack < 3 ) dot_stack++;
      result = RESULT_HIT;
      double dot_dmg = calculate_direct_damage( result, 0, t -> level, total_spell_power(), total_attack_power(), total_dd_multiplier() ) * 0.03;
      base_td = dot_stack * dot_dmg / num_ticks;
    }

    mage_spell_t::impact( t, impact_result, travel_dmg );
  }

  virtual double total_td_multiplier() const
  { return 1.0; } // No double-dipping!

  virtual void tick( dot_t* d )
  {
    // Ticks don't benefit from Shatter, which checks for fof_frozen
    // So disable it for the ticks, assuming it was set to true in execute
    fof_frozen = false;
    mage_spell_t::tick( d );
  }
};

// Ice Lance Spell ==========================================================

struct ice_lance_t : public mage_spell_t
{
  ice_lance_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "ice_lance", p, p -> find_spell( 30455 ) )
  {
    parse_options( NULL, options_str );
    base_multiplier *= 1.0 + p -> glyphs.ice_lance -> effect1().percent();
    fof_frozen = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new ice_lance_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    mage_spell_t::player_buff();

    if ( p() -> buffs.fingers_of_frost -> up() )
    {
      player_multiplier *= 1.0 + effect2().percent(); // Built in bonus against frozen targets
      player_multiplier *= 1.25; // Buff from Fingers of Frost
    }
  }
};

// Icy Veins Buff ===========================================================

struct icy_veins_buff_t : public buff_t
{
  icy_veins_buff_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "icy_veins", p -> find_spell( 12472 ) ) )
  {
    cooldown -> duration = timespan_t::zero(); // CD is managed by the spell
  }

  virtual void expire()
  {
    buff_t::expire();

    mage_t* p = static_cast<mage_t*>( player );
    p -> buffs.tier13_2pc -> expire();
  }
};

// Icy Veins Spell ==========================================================

struct icy_veins_t : public mage_spell_t
{
  timespan_t orig_duration;

  icy_veins_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "icy_veins", p, p -> find_spell( 12472 ) ), orig_duration( timespan_t::zero() )
  {
    // check_talent( p -> talents.icy_veins -> rank() );
    parse_options( NULL, options_str );
    orig_duration = cooldown -> duration;
  }

  virtual void execute()
  {
    if ( sim -> log )
      log_t::output( sim, "%s performs %s", player -> name(), name() );

    if ( player -> set_bonus.tier13_4pc_caster() )
      cooldown -> duration = orig_duration + p() -> buffs.tier13_2pc -> check() * p() -> spells.stolen_time -> effect3().time_value();

    consume_resource();
    update_ready();

    p() -> buffs.icy_veins -> trigger();
  }
};

// Living Bomb Spell ========================================================

struct living_bomb_explosion_t : public mage_spell_t
{
  living_bomb_explosion_t( mage_t* p, bool dtr=false ) :
    mage_spell_t( "living_bomb_explosion", p, p -> find_spell( 44461 ) )
  {
    aoe = -1;
    background = true;
    base_multiplier *= 1.0 + ( p -> glyphs.living_bomb -> effect1().percent() );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new living_bomb_explosion_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual resource_type_e current_resource() const
  { return RESOURCE_NONE; }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    // Ticks don't trigger ignite
    spell_t::impact( t, impact_result, travel_dmg );
  }
};

struct living_bomb_t : public mage_spell_t
{
  living_bomb_explosion_t* explosion_spell;

  living_bomb_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "living_bomb", p, p -> find_spell( 44457 ) )
  {
    check_talent( p -> talents.living_bomb -> ok() );
    parse_options( NULL, options_str );

    dot_behavior = DOT_REFRESH;

    explosion_spell = new living_bomb_explosion_t( p );
    // Trickery to make MoE work
    explosion_spell -> base_costs[ explosion_spell -> current_resource() ] = base_costs[ current_resource() ];
    add_child( explosion_spell );
  }

  virtual void target_debuff( player_t* t, dmg_type_e dt )
  {
    // Override the mage_spell_t version to ensure mastery effect is stacked additively.  Someday I will make this cleaner.
    spell_t::target_debuff( t, dt );

    target_multiplier *= 1.0 + ( p() -> glyphs.living_bomb -> effect1().percent() );
  }

  virtual void last_tick( dot_t* d )
  {
    mage_spell_t::last_tick( d );

    explosion_spell -> execute();
  }
};

// Mage Armor Spell =========================================================

struct mage_armor_t : public mage_spell_t
{
  mage_armor_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "mage_armor", p, p -> find_spell( 6117 ) )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    if ( sim -> log )
      log_t::output( sim, "%s performs %s", player -> name(), name() );
    update_ready();

    p() -> buffs.frost_armor -> expire();
    p() -> buffs.molten_armor -> expire();
    p() -> buffs.mage_armor -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.mage_armor -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Mana Gem =================================================================

struct mana_gem_t : public action_t
{
  double min;
  double max;

  mana_gem_t( mage_t* p, const std::string& options_str ) :
    action_t( ACTION_USE, "mana_gem", p ), min ( 0 ), max( 0 )
  {
    parse_options( NULL, options_str );

    min = p -> dbc.effect_min( 16856, p -> level );
    max = p -> dbc.effect_max( 16856, p -> level );

    if ( p -> level <= 80 )
    {
      min = 3330.0;
      max = 3500.0;
    }

    cooldown = p -> cooldowns.mana_gem;
    cooldown -> duration = timespan_t::from_seconds( 120.0 );
    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    mage_t* p = static_cast<mage_t*>( player );

    if ( sim -> log ) log_t::output( sim, "%s uses Mana Gem", p -> name() );

    p -> procs.mana_gem -> occur();
    p -> mana_gem_charges--;

    double gain = sim -> rng -> range( min, max );

    player -> resource_gain( RESOURCE_MANA, gain, p -> gains.mana_gem );

    p -> buffs.improved_mana_gem -> trigger();

    update_ready();
  }

  virtual bool ready()
  {
    mage_t* p = static_cast<mage_t*>( player );

    if ( p -> mana_gem_charges <= 0 )
      return false;

    if ( ( player -> resources.max[ RESOURCE_MANA ] - player -> resources.current[ RESOURCE_MANA ] ) < max )
      return false;

    return action_t::ready();
  }
};

// Mirror Image Spell =======================================================

struct mirror_image_t : public mage_spell_t
{
  mirror_image_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "mirror_image", p, p -> find_spell( 55342 ) )
  {
    parse_options( NULL, options_str );
    harmful = false;

    num_ticks = 0;

    if ( p -> pets.mirror_image_3 )
    {
      stats -> add_child( p -> pets.mirror_image_3 -> get_stats( "mirror_arcane_blast" ) );
      stats -> add_child( p -> pets.mirror_image_3 -> get_stats( "mirror_fire_blast" ) );
      stats -> add_child( p -> pets.mirror_image_3 -> get_stats( "mirror_fireball" ) );
      stats -> add_child( p -> pets.mirror_image_3 -> get_stats( "mirror_frost_bolt" ) );
    }
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> pets.mirror_image_3 -> summon();
  }

  virtual timespan_t gcd() const
  {
    if ( p() -> buffs.arcane_power -> check() && p() -> glyphs.arcane_power -> ok() )
      return timespan_t::zero();

    return mage_spell_t::gcd();
  }
};

// Molten Armor Spell =======================================================

struct molten_armor_t : public mage_spell_t
{
  molten_armor_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "molten_armor", p, p -> find_spell( 30482 ) )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    update_ready();

    p() -> buffs.frost_armor -> expire();
    p() -> buffs.mage_armor -> expire();
    p() -> buffs.molten_armor -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.molten_armor -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Presence of Mind Spell ===================================================

struct presence_of_mind_t : public mage_spell_t
{
  presence_of_mind_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "presence_of_mind", p, p -> find_spell( 12043 ) )
  {
    check_talent( p -> talents.presence_of_mind -> ok() );

    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> buffs.presence_of_mind -> trigger();
    p() -> buffs.arcane_potency -> trigger( 2 );
  }

  virtual bool ready()
  {
    // Can't use PoM while AP is up
    if ( p() -> buffs.arcane_power -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Pyroblast Spell ==========================================================

struct pyroblast_t : public mage_spell_t
{
  pyroblast_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "pyroblast", p, p -> find_spell( 11366 ) )
  {
    check_spec( MAGE_FIRE );
    parse_options( NULL, options_str );
    base_crit += p -> glyphs.pyroblast -> effect1().percent();
    may_hot_streak = true;
    dot_behavior = DOT_REFRESH;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new pyroblast_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    if ( result_is_hit() )
    {
      if ( player -> set_bonus.tier13_2pc_caster() )
        p() -> buffs.tier13_2pc -> trigger( 1, -1, 0.5 );
    }
  }
};

// Pyroblast! Spell =========================================================

struct pyroblast_hs_t : public mage_spell_t
{
  pyroblast_hs_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "pyroblast_hs", p, p -> find_spell( 92315 ) )
  {
    init_dot( "pyroblast" );
    check_spec( MAGE_FIRE );
    parse_options( NULL, options_str );
    base_crit += p -> glyphs.pyroblast -> effect1().percent();
    dot_behavior = DOT_REFRESH;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new pyroblast_hs_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    if ( p() -> buffs.hot_streak -> up() )
    {
      p() -> buffs.hot_streak -> expire();
    }

    mage_spell_t::execute();

    if ( result_is_hit() )
    {
      if ( player -> set_bonus.tier13_2pc_caster() )
        p() -> buffs.tier13_2pc -> trigger( 1, -1, 0.5 );
    }
  }

  virtual bool ready()
  {
    if ( ! mage_spell_t::ready() )
      return false;

    return ( p() -> buffs.hot_streak -> check() > 0 );
  }
};

// Scorch Spell =============================================================

struct scorch_t : public mage_spell_t
{
  int debuff;

  scorch_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "scorch", p, p -> find_spell( 2948 ) ),
    debuff( 0 )
  {
    check_talent( p -> talents.scorch -> ok() );

    parse_options( NULL, options_str );

    may_hot_streak = true;

    if ( p -> set_bonus.pvp_4pc_caster() )
      base_multiplier *= 1.05;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new scorch_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual bool usable_moving()
  { return true; }
};

// Slow Spell ===============================================================

struct slow_t : public mage_spell_t
{
  slow_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "slow", p, p -> find_spell( 31589 ) )
  {
    check_spec( MAGE_ARCANE );
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    mage_targetdata_t* td = targetdata_t::get( player, target ) -> cast_mage();
    td -> debuffs_slow -> trigger();
  }
};

// Time Warp Spell ==========================================================

struct time_warp_t : public mage_spell_t
{
  time_warp_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "time_warp", p, p -> find_spell( 80353 ) )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> sleeping || p -> buffs.exhaustion -> check() )
        continue;

      p -> buffs.bloodlust -> trigger(); // Bloodlust and Timewarp are the same
      p -> buffs.exhaustion -> trigger();
    }
  }

  virtual bool ready()
  {
    if ( player -> buffs.exhaustion -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Water Elemental Spell ====================================================

struct water_elemental_spell_t : public mage_spell_t
{
  water_elemental_spell_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "water_elemental", p, p -> find_spell( 31687 ) )
  {
    check_spec( MAGE_FROST );
    parse_options( NULL, options_str );
    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> pets.water_elemental -> summon();
  }

  virtual bool ready()
  {
    if ( ! mage_spell_t::ready() )
      return false;

    return ! ( p() -> pets.water_elemental && ! p() -> pets.water_elemental -> sleeping );
  }
};

// Choose Rotation ==========================================================

struct choose_rotation_t : public action_t
{
  double evocation_target_mana_percentage;
  int force_dps;
  int force_dpm;
  timespan_t final_burn_offset;
  double oom_offset;

  choose_rotation_t( mage_t* p, const std::string& options_str ) :
    action_t( ACTION_USE, "choose_rotation", p )
  {
    cooldown -> duration = timespan_t::from_seconds( 10 );
    evocation_target_mana_percentage = 35;
    force_dps = 0;
    force_dpm = 0;
    final_burn_offset = timespan_t::from_seconds( 20 );
    oom_offset = 0;

    option_t options[] =
    {
      { "cooldown", OPT_TIMESPAN, &( cooldown -> duration ) },
      { "evocation_pct", OPT_FLT, &( evocation_target_mana_percentage ) },
      { "force_dps", OPT_INT, &( force_dps ) },
      { "force_dpm", OPT_INT, &( force_dpm ) },
      { "final_burn_offset", OPT_TIMESPAN, &( final_burn_offset ) },
      { "oom_offset", OPT_FLT, &( oom_offset ) },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( cooldown -> duration < timespan_t::from_seconds( 1.0 ) )
    {
      sim -> errorf( "Player %s: choose_rotation cannot have cooldown -> duration less than 1.0sec", p -> name() );
      cooldown -> duration = timespan_t::from_seconds( 1.0 );
    }

    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();

    if ( force_dps || force_dpm )
    {
      if ( p -> rotation.current == ROTATION_DPS )
      {
        p -> rotation.dps_time += ( sim -> current_time - p -> rotation.last_time );
      }
      else if ( p -> rotation.current == ROTATION_DPM )
      {
        p -> rotation.dpm_time += ( sim -> current_time - p -> rotation.last_time );
      }
      p -> rotation.last_time = sim -> current_time;

      if ( sim -> log )
      {
        log_t::output( sim, "%f burn mps, %f time to die", ( p -> rotation.dps_mana_loss / p -> rotation.dps_time.total_seconds() ) - ( p -> rotation.mana_gain / sim -> current_time.total_seconds() ), sim -> target -> time_to_die().total_seconds() );
      }

      if ( force_dps )
      {
        if ( sim -> log ) log_t::output( sim, "%s switches to DPS spell rotation", p -> name() );
        p -> rotation.current = ROTATION_DPS;
      }
      if ( force_dpm )
      {
        if ( sim -> log ) log_t::output( sim, "%s switches to DPM spell rotation", p -> name() );
        p -> rotation.current = ROTATION_DPM;
      }

      update_ready();

      return;
    }

    if ( sim -> log ) log_t::output( sim, "%s Considers Spell Rotation", p -> name() );

    // The purpose of this action is to automatically determine when to start dps rotation.
    // We aim to either reach 0 mana at end of fight or evocation_target_mana_percentage at next evocation.
    // If mana gem has charges we assume it will be used during the next dps rotation burn.
    // The dps rotation should correspond only to the actual burn, not any corrections due to overshooting.

    // It is important to smooth out the regen rate by averaging out the returns from Evocation and Mana Gems.
    // In order for this to work, the resource_gain() method must filter out these sources when
    // tracking "rotation.mana_gain".

    double regen_rate = p -> rotation.mana_gain / sim -> current_time.total_seconds();

    timespan_t ttd = sim -> target -> time_to_die();
    timespan_t tte = p -> cooldowns.evocation -> remains();

    if ( p -> rotation.current == ROTATION_DPS )
    {
      p -> rotation.dps_time += ( sim -> current_time - p -> rotation.last_time );

      // We should only drop out of dps rotation if we break target mana treshold.
      // In that situation we enter a mps rotation waiting for evocation to come off cooldown or for fight to end.
      // The action list should take into account that it might actually need to do some burn in such a case.

      if ( tte < ttd )
      {
        // We're going until target percentage
        if ( p -> resources.current[ RESOURCE_MANA ] / p -> resources.max[ RESOURCE_MANA ] < evocation_target_mana_percentage / 100.0 )
        {
          if ( sim -> log ) log_t::output( sim, "%s switches to DPM spell rotation", p -> name() );

          p -> rotation.current = ROTATION_DPM;
        }
      }
      else
      {
        // We're going until OOM, stop when we can no longer cast full stack AB (approximately, 4 stack with AP can be 6177)
        if ( p -> resources.current[ RESOURCE_MANA ] < 6200 )
        {
          if ( sim -> log ) log_t::output( sim, "%s switches to DPM spell rotation", p -> name() );

          p -> rotation.current = ROTATION_DPM;
        }
      }
    }
    else if ( p -> rotation.current == ROTATION_DPM )
    {
      p -> rotation.dpm_time += ( sim -> current_time - p -> rotation.last_time );

      // Calculate consumption rate of dps rotation and determine if we should start burning.

      double consumption_rate = ( p -> rotation.dps_mana_loss / p -> rotation.dps_time.total_seconds() ) - regen_rate;
      double available_mana = p -> resources.current[ RESOURCE_MANA ];

      // Mana Gem, if we have uses left
      if ( p -> mana_gem_charges > 0 )
      {
        available_mana += p -> dbc.effect_max( 16856, p -> level );
      }

      // If this will be the last evocation then figure out how much of it we can actually burn before end and adjust appropriately.

      timespan_t evo_cooldown = timespan_t::from_seconds( 240.0 );

      timespan_t target_time;
      double target_pct;

      if ( ttd < tte + evo_cooldown )
      {
        if ( ttd < tte + final_burn_offset )
        {
          // No more evocations, aim for OOM
          target_time = ttd;
          target_pct = oom_offset;
        }
        else
        {
          // If we aim for normal evo percentage we'll get the most out of burn/mana adept synergy.
          target_time = tte;
          target_pct = evocation_target_mana_percentage / 100.0;
        }
      }
      else
      {
        // We'll cast more then one evocation, we're aiming for a normal evo target burn.
        target_time = tte;
        target_pct = evocation_target_mana_percentage / 100.0;
      }

      if ( consumption_rate > 0 )
      {
        // Compute time to get to desired percentage.
        timespan_t expected_time = timespan_t::from_seconds( ( available_mana - target_pct * p -> resources.max[ RESOURCE_MANA ] ) / consumption_rate );

        if ( expected_time >= target_time )
        {
          if ( sim -> log ) log_t::output( sim, "%s switches to DPS spell rotation", p -> name() );

          p -> rotation.current = ROTATION_DPS;
        }
      }
      else
      {
        // If dps rotation is regen, then obviously use it all the time.

        if ( sim -> log ) log_t::output( sim, "%s switches to DPS spell rotation", p -> name() );

        p -> rotation.current = ROTATION_DPS;
      }
    }
    p -> rotation.last_time = sim -> current_time;

    update_ready();
  }

  virtual bool ready()
  {
    if ( cooldown -> remains() > timespan_t::zero() )
      return false;

    if ( sim -> current_time < cooldown -> duration )
      return false;

    if ( if_expr && ! if_expr -> success() )
      return false;

    return true;
  }
};

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Mage Character Definition
// ==========================================================================

// mage_t::create_action ====================================================

action_t* mage_t::create_action( const std::string& name,
                                 const std::string& options_str )
{
  if ( name == "arcane_barrage"    ) return new          arcane_barrage_t( this, options_str );
  if ( name == "arcane_blast"      ) return new            arcane_blast_t( this, options_str );
  if ( name == "arcane_brilliance" ) return new       arcane_brilliance_t( this, options_str );
  if ( name == "arcane_explosion"  ) return new        arcane_explosion_t( this, options_str );
  if ( name == "arcane_missiles"   ) return new         arcane_missiles_t( this, options_str );
  if ( name == "arcane_power"      ) return new            arcane_power_t( this, options_str );
  if ( name == "blink"             ) return new                   blink_t( this, options_str );
  if ( name == "choose_rotation"   ) return new         choose_rotation_t( this, options_str );
  if ( name == "cold_snap"         ) return new               cold_snap_t( this, options_str );
  if ( name == "combustion"        ) return new              combustion_t( this, options_str );
  if ( name == "cone_of_cold"      ) return new            cone_of_cold_t( this, options_str );
  if ( name == "conjure_mana_gem"  ) return new        conjure_mana_gem_t( this, options_str );
  if ( name == "counterspell"      ) return new            counterspell_t( this, options_str );
  if ( name == "deep_freeze"       ) return new             deep_freeze_t( this, options_str );
  if ( name == "dragons_breath"    ) return new          dragons_breath_t( this, options_str );
  if ( name == "evocation"         ) return new               evocation_t( this, options_str );
  if ( name == "fire_blast"        ) return new              fire_blast_t( this, options_str );
  if ( name == "fireball"          ) return new                fireball_t( this, options_str );
  if ( name == "frost_armor"       ) return new             frost_armor_t( this, options_str );
  if ( name == "frostbolt"         ) return new               frostbolt_t( this, options_str );
  if ( name == "frostfire_bolt"    ) return new          frostfire_bolt_t( this, options_str );
  if ( name == "ice_lance"         ) return new               ice_lance_t( this, options_str );
  if ( name == "icy_veins"         ) return new               icy_veins_t( this, options_str );
  if ( name == "living_bomb"       ) return new             living_bomb_t( this, options_str );
  if ( name == "mage_armor"        ) return new              mage_armor_t( this, options_str );
  if ( name == "mana_gem"          ) return new                mana_gem_t( this, options_str );
  if ( name == "mirror_image"      ) return new            mirror_image_t( this, options_str );
  if ( name == "molten_armor"      ) return new            molten_armor_t( this, options_str );
  if ( name == "presence_of_mind"  ) return new        presence_of_mind_t( this, options_str );
  if ( name == "pyroblast"         ) return new               pyroblast_t( this, options_str );
  if ( name == "pyroblast_hs"      ) return new            pyroblast_hs_t( this, options_str );
  if ( name == "scorch"            ) return new                  scorch_t( this, options_str );
  if ( name == "slow"              ) return new                    slow_t( this, options_str );
  if ( name == "time_warp"         ) return new               time_warp_t( this, options_str );
  if ( name == "water_elemental"   ) return new   water_elemental_spell_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// mage_t::create_pet =======================================================

pet_t* mage_t::create_pet( const std::string& pet_name,
                           const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "mirror_image_3"  ) return new mirror_image_pet_t   ( sim, this );
  if ( pet_name == "water_elemental" ) return new water_elemental_pet_t( sim, this );

  return 0;
}

// mage_t::create_pets ======================================================

void mage_t::create_pets()
{
  pets.mirror_image_3  = create_pet( "mirror_image_3"  );
  pets.water_elemental = create_pet( "water_elemental" );
}

// mage_t::init_spells ======================================================

void mage_t::init_spells()
{
  player_t::init_spells();

  // Talents
  talents.blazing_speed    = find_talent_spell( "Blazing Speed" );
  talents.cauterize        = find_talent_spell( "Cauterize" );
  talents.cold_snap        = find_talent_spell( "Cold Snap" );
  talents.frostjaw         = find_talent_spell( "Frostjaw" );
  talents.frost_bomb       = find_talent_spell( "Frost Bomb" );
  talents.greater_invis    = find_talent_spell( "Greater Invisibility" );
  talents.ice_barrier      = find_talent_spell( "Ice Barrier" );
  talents.ice_flows        = find_talent_spell( "Ice Flows" );
  talents.ice_ward         = find_talent_spell( "Ice Ward" );
  talents.incanters_ward   = find_talent_spell( "Incanter's Ward" );
  talents.invocation       = find_talent_spell( "Invocation" );
  talents.living_bomb      = find_talent_spell( "Living Bomb" );
  talents.nether_tempest   = find_talent_spell( "Nether Tempest" );
  talents.presence_of_mind = find_talent_spell( "Presence of Mind" );
  talents.ring_of_frost    = find_talent_spell( "Ring of Frost" );
  talents.rune_of_power    = find_talent_spell( "Rune of Power" );
  talents.scorch           = find_talent_spell( "Scorch" );
  talents.temporal_shield  = find_talent_spell( "Temporal Shield" );

  // Passive Spells
  passives.burning_soul          = find_class_spell( "Burning Soul" );
  passives.improved_counterspell = find_class_spell( "Improved Counterspell" );
  passives.nether_attunement     = find_class_spell( "Nether Attunement" );
  passives.shatter               = find_class_spell( "Shatter" );

  // Spec Spells
  spec.arcane_charge    = find_class_spell( "Arcane Charge" );
  spec.brain_freeze     = find_class_spell( "Brain Freeze" );
  spec.critical_mass    = find_class_spell( "Critical Mass" );
  spec.fingers_of_frost = find_class_spell( "Fingers of Frost" );

  // Mastery
  spec.frostburn  = find_mastery_spell( MAGE_FROST );
  spec.ignite     = find_mastery_spell( MAGE_FIRE );
  spec.mana_adept = find_mastery_spell( MAGE_ARCANE );

  spells.stolen_time = spell_data_t::find( 105791, "Stolen Time", dbc.ptr );

  // Glyphs
  glyphs.arcane_barrage    = find_glyph_spell( "Glyph of Arcane Barrage" );
  glyphs.arcane_blast      = find_glyph_spell( "Glyph of Arcane Blast" );
  glyphs.arcane_brilliance = find_glyph_spell( "Glyph of Arcane Brilliance" );
  glyphs.arcane_missiles   = find_glyph_spell( "Glyph of Arcane Missiles" );
  glyphs.arcane_power      = find_glyph_spell( "Glyph of Arcane Power" );
  glyphs.cone_of_cold      = find_glyph_spell( "Glyph of Cone of Cold" );
  glyphs.conjuring         = find_glyph_spell( "Glyph of Conjuring" );
  glyphs.deep_freeze       = find_glyph_spell( "Glyph of Deep Freeze" );
  glyphs.dragons_breath    = find_glyph_spell( "Glyph of Dragon's Breath" );
  glyphs.fireball          = find_glyph_spell( "Glyph of Fireball" );
  glyphs.frost_armor       = find_glyph_spell( "Glyph of Frost Armor" );
  glyphs.frostbolt         = find_glyph_spell( "Glyph of Frostbolt" );
  glyphs.frostfire         = find_glyph_spell( "Glyph of Frostfire" );
  glyphs.ice_lance         = find_glyph_spell( "Glyph of Ice Lance" );
  glyphs.living_bomb       = find_glyph_spell( "Glyph of Living Bomb" );
  glyphs.mage_armor        = find_glyph_spell( "Glyph of Mage Armor" );
  glyphs.mirror_image      = find_glyph_spell( "Glyph of Mirror Image" );
  glyphs.molten_armor      = find_glyph_spell( "Glyph of Molten Armor" );
  glyphs.pyroblast         = find_glyph_spell( "Glyph of Pyroblast" );

  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P    M2P    M4P    T2P    T4P    H2P    H4P
    { 105788, 105790,     0,     0,     0,     0,     0,     0 }, // Tier13
    {      0,      0,     0,     0,     0,     0,     0,     0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// mage_t::init_base ========================================================

void mage_t::init_base()
{
  player_t::init_base();

  initial_spell_power_per_intellect = 1.0;

  base_attack_power = -10;
  initial_attack_power_per_strength = 1.0;

  mana_per_intellect = 15;

  diminished_kfactor    = 0.009830;
  diminished_dodge_capi = 0.006650;
  diminished_parry_capi = 0.006650;
}

// mage_t::init_scaling =====================================================

void mage_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_SPIRIT ] = false;
}

// mage_t::init_values ======================================================

void mage_t::init_values()
{
  player_t::init_values();

  if ( set_bonus.pvp_2pc_caster() )
    attribute_initial[ ATTR_INTELLECT ] += 70;

  if ( set_bonus.pvp_4pc_caster() )
    attribute_initial[ ATTR_INTELLECT ] += 90;
}

// mage_t::init_buffs =======================================================

void mage_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  buffs.arcane_charge        = buff_creator_t( this, "arcane_charge", find_spell( 36032 ) );
  buffs.arcane_missiles      = buff_creator_t( this, "arcane_missiles", find_spell( 79683 ) ).chance( 0.40 );
  buffs.arcane_power         = new arcane_power_buff_t( this );
  buffs.brain_freeze         = buff_creator_t( this, "brain_freeze", find_spell( 44549 ) );
  buffs.fingers_of_frost     = buff_creator_t( this, "fingers_of_frost", find_spell( 112965 ) ).chance( find_spell( 112965 ) -> effectN( 1 ).percent() );
  buffs.frost_armor          = buff_creator_t( this, "frost_armor", find_spell( 7302 ) );
  // FIXME: What is this called now? buffs.hot_streak           = new buff_t( this, spells.hot_streak,            NULL );
  buffs.icy_veins            = new icy_veins_buff_t( this );
  buffs.invocation           = buff_creator_t( this, "invocation", talents.invocation );
  buffs.mage_armor           = buff_creator_t( this, 6117, "mage_armor" );
  buffs.molten_armor         = buff_creator_t( this, "molten_armor", find_spell( 30482 ) );
  buffs.presence_of_mind     = buff_creator_t( this, "presence_of_mind", talents.presence_of_mind );

  buffs.hot_streak_crits     = buff_creator_t( this, "hot_streak_crits" ).max_stack( 2 ).quiet( true );

  buffs.tier13_2pc           = stat_buff_creator_t(
                                 buff_creator_t( this, "tier13_2pc" ).duration( timespan_t::from_seconds( 30.0 ) ).max_stack( 10 )
                               ).stat( STAT_HASTE_RATING ).amount( 50.0 );
}

// mage_t::init_gains =======================================================

void mage_t::init_gains()
{
  player_t::init_gains();

  gains.clearcasting       = get_gain( "clearcasting"       );
  gains.evocation          = get_gain( "evocation"          );
  gains.frost_armor        = get_gain( "frost_armor"        );
  gains.mage_armor         = get_gain( "mage_armor"         );
  gains.mana_gem           = get_gain( "mana_gem"           );
  gains.master_of_elements = get_gain( "master_of_elements" );
}

// mage_t::init_procs =======================================================

void mage_t::init_procs()
{
  player_t::init_procs();

  procs.deferred_ignite         = get_proc( "deferred_ignite"               );
  procs.munched_ignite          = get_proc( "munched_ignite"                );
  procs.rolled_ignite           = get_proc( "rolled_ignite"                 );
  procs.mana_gem                = get_proc( "mana_gem"                      );
  procs.early_frost             = get_proc( "early_frost"                   );
  procs.test_for_crit_hotstreak = get_proc( "test_for_crit_hotstreak"       );
  procs.crit_for_hotstreak      = get_proc( "crit_test_hotstreak"           );
  procs.hotstreak               = get_proc( "normal_hotstreak"              );
  procs.improved_hotstreak      = get_proc( "improved_hotstreak"            );
}

// mage_t::init_uptimes =====================================================

void mage_t::init_benefits()
{
  player_t::init_benefits();

  benefits.arcane_blast[ 0 ]    = get_benefit( "arcane_blast_0"  );
  benefits.arcane_blast[ 1 ]    = get_benefit( "arcane_blast_1"  );
  benefits.arcane_blast[ 2 ]    = get_benefit( "arcane_blast_2"  );
  benefits.arcane_blast[ 3 ]    = get_benefit( "arcane_blast_3"  );
  benefits.arcane_blast[ 4 ]    = get_benefit( "arcane_blast_4"  );
  benefits.dps_rotation         = get_benefit( "dps_rotation"    );
  benefits.dpm_rotation         = get_benefit( "dpm_rotation"    );
  benefits.water_elemental      = get_benefit( "water_elemental" );
}

// mage_t::init_rng =========================================================

void mage_t::init_rng()
{
  player_t::init_rng();

  rngs.arcane_missiles     = get_rng( "arcane_missiles"     );
  rngs.empowered_fire      = get_rng( "empowered_fire"      );
  rngs.enduring_winter     = get_rng( "enduring_winter"     );
  rngs.fire_power          = get_rng( "fire_power"          );
  rngs.impact              = get_rng( "impact"              );
  rngs.improved_freeze     = get_rng( "improved_freeze"     );
  rngs.nether_vortex       = get_rng( "nether_vortex"       );
  rngs.mage_armor_start    = get_rng( "rngs.mage_armor_start" );
}

// mage_t::init_actions =====================================================

void mage_t::init_actions()
{
  if ( action_list_str.empty() )
  {
#if 0 // UNUSED
    // Shard of Woe check for Arcane
    bool has_shard = false;
    for ( int i=0; i < SLOT_MAX; i++ )
    {
      item_t& item = items[ i ];
      if ( strstr( item.name(), "shard_of_woe" ) )
      {
        has_shard = true;
        break;
      }
    }
#endif

    // Flask
    if ( level >= 80 )
      action_list_str = "flask,type=draconic_mind";
    else if ( level >= 75 )
      action_list_str = "flask,type=frost_wyrm";

    // Food
    if ( level >= 80 ) action_list_str += "/food,type=seafood_magnifique_feast";
    else if ( level >= 70 ) action_list_str += "/food,type=fish_feast";

    // Arcane Brilliance
    if ( level >= 58 ) action_list_str += "/arcane_brilliance,if=!aura.spell_power_multiplier.up|!aura.critical_strike.up";

    // Armor
    if ( primary_tree() == MAGE_ARCANE )
    {
      action_list_str += "/mage_armor";
    }
    else if ( primary_tree() == MAGE_FIRE )
    {
      action_list_str += "/molten_armor,if=buff.mage_armor.down&buff.molten_armor.down";
      action_list_str += "/molten_armor,if=mana_pct>45&buff.mage_armor.up";
    }
    else
    {
      action_list_str += "/molten_armor";
    }

    // Water Elemental
    if ( primary_tree() == MAGE_FROST ) action_list_str += "/water_elemental";

    // Snapshot Stats
    action_list_str += "/snapshot_stats";
    // Counterspell
    action_list_str += "/counterspell";
    // Refresh Gem during invuln phases
    if ( level >= 48 ) action_list_str += "/conjure_mana_gem,invulnerable=1,if=mana_gem_charges<3";
    // Arcane Choose Rotation
    if ( primary_tree() == MAGE_ARCANE )
    {
      action_list_str += "/choose_rotation,cooldown=1,evocation_pct=35,if=cooldown.evocation.remains+15>target.time_to_die,final_burn_offset=15";
    }
    // Usable Items
    int num_items = ( int ) items.size();
    for ( int i=0; i < num_items; i++ )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
        //Special trinket handling for Arcane, previously only used for Shard of Woe but seems to be good for all useable trinkets
        if ( primary_tree() == MAGE_ARCANE )
          action_list_str += ",if=buff.improved_mana_gem.up|cooldown.evocation.remains>90|target.time_to_die<=50";
      }
    }
    action_list_str += init_use_profession_actions();
    //Potions
    if ( level > 80 )
    {
      action_list_str += "/volcanic_potion,if=!in_combat";
    }
    if ( level > 80 )
    {
      if ( primary_tree() == MAGE_FROST )
      {
        action_list_str += "/volcanic_potion,if=buff.bloodlust.react|buff.icy_veins.react|target.time_to_die<=40";
      }
      else if ( primary_tree() == MAGE_ARCANE )
      {
        action_list_str += "/volcanic_potion,if=buff.improved_mana_gem.up|target.time_to_die<=50";
      }
      else
      {
        action_list_str += "/volcanic_potion,if=buff.bloodlust.react|target.time_to_die<=40";
      }
    }
    else if ( level >= 70 )
    {
      action_list_str += "/speed_potion,if=!in_combat";
      action_list_str += "/speed_potion,if=buff.bloodlust.react|target.time_to_die<=20";
    }
    // Race Abilities
    if ( race == RACE_TROLL && primary_tree() == MAGE_FIRE )
    {
      action_list_str += "/berserking";
    }
    else if ( race == RACE_BLOOD_ELF && primary_tree() != MAGE_ARCANE )
    {
      action_list_str += "/arcane_torrent,if=mana_pct<91";
    }
    else if ( race == RACE_ORC && primary_tree() != MAGE_ARCANE )
    {
      action_list_str += "/blood_fury";
    }

    // Talents by Spec
    // Arcane
    if ( primary_tree() == MAGE_ARCANE )
    {
      action_list_str += "/evocation,if=((max_mana>max_mana_nonproc&mana_pct_nonproc<=40)|(max_mana=max_mana_nonproc&mana_pct<=35))&target.time_to_die>10";
      if ( level >= 81 ) action_list_str += "/flame_orb,if=target.time_to_die>=10";
      //Special handling for Race Abilities
      if ( race == RACE_ORC )
      {
        action_list_str += "/blood_fury,if=buff.improved_mana_gem.up|target.time_to_die<=50";
      }
      else if ( race == RACE_TROLL )
      {
        action_list_str += "/berserking,if=buff.arcane_power.up|cooldown.arcane_power.remains>20";
      }
      //Conjure Mana Gem
      if ( ! talents.presence_of_mind -> ok() )
      {
        action_list_str += "/conjure_mana_gem,if=cooldown.evocation.remains<20&target.time_to_die>105&mana_gem_charges=0";
      }
      if ( race == RACE_BLOOD_ELF )
      {
        action_list_str += "/arcane_torrent,if=mana_pct<91&(buff.arcane_power.up|target.time_to_die<120)";
      }
      //Mana Gem
      if ( set_bonus.tier13_4pc_caster() )
      {
        action_list_str += "/mana_gem,if=buff.arcane_blast.stack=4&buff.tier13_2pc.stack>=7&(cooldown.arcane_power.remains<=0|target.time_to_die<=50)";
      }
      else
      {
        action_list_str += "/mana_gem,if=buff.arcane_blast.stack=4";
      }
      //Choose Rotation
      action_list_str += "/choose_rotation,cooldown=1,force_dps=1,if=buff.improved_mana_gem.up&dpm=1&cooldown.evocation.remains+15<target.time_to_die";
      action_list_str += "/choose_rotation,cooldown=1,force_dpm=1,if=cooldown.evocation.remains<=20&dps=1&mana_pct<22&(target.time_to_die>60|burn_mps*((target.time_to_die-5)-cooldown.evocation.remains)>max_mana_nonproc)&cooldown.evocation.remains+15<target.time_to_die";
      //Arcane Power
      if ( level >= 62 )
      {
        if ( set_bonus.tier13_4pc_caster() )
        {
          action_list_str += "/arcane_power,if=(buff.improved_mana_gem.up&buff.tier13_2pc.stack>=9)|(buff.tier13_2pc.stack>=10&cooldown.mana_gem.remains>30&cooldown.evocation.remains>10)|target.time_to_die<=50";
        }
        else
        {
          action_list_str += "/arcane_power,if=buff.improved_mana_gem.up|target.time_to_die<=50";
        }
        action_list_str += "/mirror_image,if=buff.arcane_power.up|(cooldown.arcane_power.remains>20&target.time_to_die>15)";
      }
      else
      {
        action_list_str += "/mirror_image";
      }

      if ( talents.presence_of_mind -> ok() )
      {
        action_list_str += "/presence_of_mind";
        action_list_str += "/conjure_mana_gem,if=buff.presence_of_mind.up&target.time_to_die>cooldown.mana_gem.remains&mana_gem_charges=0";
        action_list_str += "/arcane_blast,if=buff.presence_of_mind.up";
      }

      if ( set_bonus.tier13_4pc_caster() )
      {
        action_list_str += "/arcane_blast,if=dps=1|target.time_to_die<20|((cooldown.evocation.remains<=20|buff.improved_mana_gem.up|cooldown.mana_gem.remains<5)&mana_pct>=22)|(buff.arcane_power.up&mana_pct_nonproc>88)";
      }
      else
      {
        action_list_str += "/arcane_blast,if=dps=1|target.time_to_die<20|((cooldown.evocation.remains<=20|buff.improved_mana_gem.up|cooldown.mana_gem.remains<5)&mana_pct>=22)";
      }
      action_list_str += "/arcane_blast,if=buff.arcane_blast.remains<0.8&buff.arcane_blast.stack=4";
      action_list_str += "/arcane_missiles,if=mana_pct_nonproc<92&buff.arcane_missiles.react&mage_armor_timer<=2";
      action_list_str += "/arcane_missiles,if=mana_pct_nonproc<93&buff.arcane_missiles.react&mage_armor_timer>2";
      action_list_str += "/arcane_barrage,if=mana_pct_nonproc<87&buff.arcane_blast.stack=2";
      action_list_str += "/arcane_barrage,if=mana_pct_nonproc<90&buff.arcane_blast.stack=3";
      action_list_str += "/arcane_barrage,if=mana_pct_nonproc<92&buff.arcane_blast.stack=4";
      action_list_str += "/arcane_blast";
      action_list_str += "/arcane_barrage,moving=1"; // when moving
      action_list_str += "/fire_blast,moving=1"; // when moving
      action_list_str += "/ice_lance,moving=1"; // when moving
    }
    // Fire
    else if ( primary_tree() == MAGE_FIRE )
    {
      action_list_str += "/mana_gem,if=mana_deficit>12500";
      if ( level >= 77 )
      {
        action_list_str += "/combustion,if=dot.living_bomb.ticking&dot.ignite.ticking&dot.pyroblast.ticking&dot.ignite.tick_dmg>10000";
      }
      action_list_str += "/mirror_image,if=target.time_to_die>=25";
      if ( talents.living_bomb -> ok() ) action_list_str += "/living_bomb,if=!ticking";
      action_list_str += "/pyroblast_hs,if=buff.hot_streak.react";
      if ( glyphs.frostfire -> ok() )
      {
        action_list_str += "/frostfire_bolt";
      }
      else
      {
        action_list_str += "/fireball";
      }
      action_list_str += "/mage_armor,if=mana_pct<5&buff.mage_armor.down";
      action_list_str += "/scorch"; // This can be free, so cast it last
    }
    // Frost
    else if ( primary_tree() == MAGE_FROST )
    {
      action_list_str += "/evocation,if=mana_pct<40&(buff.icy_veins.react|buff.bloodlust.react)";
      action_list_str += "/mana_gem,if=mana_deficit>12500";
      if ( level >= 50 ) action_list_str += "/mirror_image,if=target.time_to_die>=25";
      if ( race == RACE_TROLL )
      {
        action_list_str += "/berserking,if=buff.icy_veins.down&buff.bloodlust.down";
      }
      if ( level >= 36 )
      {
        action_list_str += "/icy_veins,if=buff.icy_veins.down&buff.bloodlust.down";
        if ( set_bonus.tier13_2pc_caster() && set_bonus.tier13_4pc_caster() )
          action_list_str += "&(buff.tier13_2pc.stack>7|cooldown.cold_snap.remains<22)";
      }
      if ( level >= 77 )
      {
        action_list_str += "/frostfire_bolt,if=buff.brain_freeze.react&buff.fingers_of_frost.react";
      }
      action_list_str += "/ice_lance,if=buff.fingers_of_frost.stack>1";
      action_list_str += "/ice_lance,if=buff.fingers_of_frost.react&pet.water_elemental.cooldown.freeze.remains<gcd";
      action_list_str += "/frostbolt";
      action_list_str += "/ice_lance,moving=1"; // when moving
      action_list_str += "/fire_blast,moving=1"; // when moving
    }

    action_list_default = 1;
  }

  player_t::init_actions();
}

// mage_t::composite_armor_multiplier =======================================

double mage_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  if ( buffs.frost_armor -> check() )
  {
    a *= 1.0 + spells.frost_armor -> effect1().percent();
  }

  return a;
}

// mage_t::composite_player_multipler =======================================

double mage_t::composite_player_multiplier( const school_type_e school, const action_t* a ) const
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( buffs.invocation -> up() )
  {
    m *= 1.0 + talents.invocation -> effect1().percent();
  }

  m *= 1.0 + buffs.arcane_power -> value();

  double mana_pct = resources.current[ RESOURCE_MANA ] / resources.max [ RESOURCE_MANA ];
  m *= 1.0 + mana_pct * spec.mana_adept -> effectN( 1 ).coeff() * 0.01 * composite_mastery();

  return m;
}

// mage_t::composite_spell_crit =============================================

double mage_t::composite_spell_crit() const
{
  double c = player_t::composite_spell_crit();

  // These also increase the water elementals crit chance

  if ( buffs.molten_armor -> up() )
  {
    c += ( spells.molten_armor -> effect3().percent() +
           glyphs.molten_armor -> effect1().percent() );
  }

  return c;
}

// mage_t::composite_spell_resistance =======================================

double mage_t::composite_spell_resistance( const school_type_e school ) const
{
  double sr = player_t::composite_spell_resistance( school );

  if ( buffs.frost_armor -> check() && school == SCHOOL_FROST )
  {
    sr += spells.frost_armor -> effect3().base_value();
  }
  else if ( buffs.mage_armor -> check() )
  {
    sr += spells.mage_armor -> effect1().base_value();
  }

  return sr;
}

// mage_t::matching_gear_multiplier =========================================

double mage_t::matching_gear_multiplier( const attribute_type_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return 0.05;

  return 0.0;
}

// mage_t::reset ============================================================

void mage_t::reset()
{
  player_t::reset();

  rotation.reset();
  mana_gem_charges = 3;
}

// mage_t::regen  ===========================================================

void mage_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( glyphs.frost_armor -> ok() && buffs.frost_armor -> up()  )
  {
    double gain_amount = resources.max[ RESOURCE_MANA ] * glyphs.frost_armor -> effect1().percent();
    gain_amount *= periodicity.total_seconds() / 5.0;
    resource_gain( RESOURCE_MANA, gain_amount, gains.frost_armor );
  }

  if ( pets.water_elemental )
    benefits.water_elemental -> update( pets.water_elemental -> sleeping == 0 );
}

// mage_t::resource_gain ====================================================

double mage_t::resource_gain( resource_type_e resource,
                              double    amount,
                              gain_t*   source,
                              action_t* action )
{
  double actual_amount = player_t::resource_gain( resource, amount, source, action );

  if ( resource == RESOURCE_MANA )
  {
    if ( source != gains.evocation &&
         source != gains.mana_gem )
    {
      rotation.mana_gain += actual_amount;
    }
  }

  return actual_amount;
}

// mage_t::resource_loss ====================================================

double mage_t::resource_loss( resource_type_e resource,
                              double    amount,
                              action_t* action )
{
  double actual_amount = player_t::resource_loss( resource, amount, action );

  if ( resource == RESOURCE_MANA )
  {
    if ( rotation.current == ROTATION_DPS )
    {
      rotation.dps_mana_loss += actual_amount;
    }
    else if ( rotation.current == ROTATION_DPM )
    {
      rotation.dpm_mana_loss += actual_amount;
    }
  }

  return actual_amount;
}

// mage_t::stun =============================================================

void mage_t::stun()
{
  //TODO: override this to handle Blink
  player_t::stun();
}

// mage_t::create_expression ================================================

expr_t* mage_t::create_expression( action_t* a, const std::string& name_str )
{
  struct mage_expr_t : public expr_t
  {
    mage_t& mage;
    mage_expr_t( const std::string& n, mage_t& m ) :
      expr_t( n ), mage( m ) {}
  };

  struct rotation_expr_t : public mage_expr_t
  {
    mage_rotation_e rt;
    rotation_expr_t( const std::string& n, mage_t& m, mage_rotation_e r ) :
      mage_expr_t( n, m ), rt( r ) {}
    virtual double evaluate() { return mage.rotation.current == rt; }
  };

  if ( name_str == "dps" )
    return new rotation_expr_t( name_str, *this, ROTATION_DPS );

  if ( name_str == "dpm" )
    return new rotation_expr_t( name_str, *this, ROTATION_DPM );

  if ( name_str == "mana_gem_charges" )
    return make_ref_expr( name_str, mana_gem_charges );

  if ( name_str == "mage_armor_timer" )
  {
    struct mage_armor_timer_expr_t : public mage_expr_t
    {
      mage_armor_timer_expr_t( mage_t& m ) : mage_expr_t( "mage_armor_timer", m ) {}
      virtual double evaluate() { return 5 + ( mage.mage_armor_timer - mage.sim -> current_time ).total_seconds(); }
    };
    return new mage_armor_timer_expr_t( *this );
  }

  if ( name_str == "burn_mps" )
  {
    struct burn_mps_expr_t : public mage_expr_t
    {
      burn_mps_expr_t( mage_t& m ) : mage_expr_t( "burn_mps", m ) {}
      virtual double evaluate()
      {
        timespan_t now = mage.sim -> current_time;
        timespan_t delta = now - mage.rotation.last_time;
        mage.rotation.last_time = now;
        if ( mage.rotation.current == ROTATION_DPS )
          mage.rotation.dps_time += delta;
        else if ( mage.rotation.current == ROTATION_DPM )
          mage.rotation.dpm_time += delta;

        return ( mage.rotation.dps_mana_loss / mage.rotation.dps_time.total_seconds() ) -
            ( mage.rotation.mana_gain / mage.sim -> current_time.total_seconds() );
      }
    };
    return new burn_mps_expr_t( *this );
  }

  if ( name_str == "regen_mps" )
  {
    struct regen_mps_expr_t : public mage_expr_t
    {
      regen_mps_expr_t( mage_t& m ) : mage_expr_t( "regen_mps", m ) {}
      virtual double evaluate()
      {
        return mage.rotation.mana_gain /
            mage.sim -> current_time.total_seconds();
      }
    };
    return new regen_mps_expr_t( *this );
  }

  return player_t::create_expression( a, name_str );
}

// mage_t::create_options ===================================================

void mage_t::create_options()
{
  player_t::create_options();

  option_t mage_options[] =
  {
    { "focus_magic_target",     OPT_STRING,   &( focus_magic_target_str ) },
    { "merge_ignite",           OPT_FLT,      &( merge_ignite           ) },
    { "ignite_sampling_delta",  OPT_TIMESPAN, &( ignite_sampling_delta  ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, mage_options );
}

// mage_t::create_profile ===================================================

bool mage_t::create_profile( std::string& profile_str, save_type_e stype, bool save_html )
{
  player_t::create_profile( profile_str, stype, save_html );

  if ( stype == SAVE_ALL )
  {
    if ( ! focus_magic_target_str.empty() ) profile_str += "focus_magic_target=" + focus_magic_target_str + "\n";
  }

  return true;
}

// mage_t::copy_from ========================================================

void mage_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  mage_t* source_mage = dynamic_cast<mage_t*>( source );
  assert( source_mage );

  focus_magic_target_str = source_mage -> focus_magic_target_str;
  merge_ignite           = source_mage -> merge_ignite;
}

// mage_t::decode_set =======================================================

int mage_t::decode_set( const item_t& item ) const
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

  if ( strstr( s, "time_lords_"   ) ) return SET_T13_CASTER;

  if ( strstr( s, "gladiators_silk_"  ) ) return SET_PVP_CASTER;

  return SET_NONE;
}

#endif // SC_MAGE

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_mage  ===================================================

player_t* player_t::create_mage( sim_t* sim, const std::string& name, race_type_e r )
{
  SC_CREATE_MAGE( sim, name, r );
}

// player_t::mage_init ======================================================

void player_t::mage_init( sim_t* )
{
}

// player_t::mage_combat_begin ==============================================

void player_t::mage_combat_begin( sim_t* )
{
}

