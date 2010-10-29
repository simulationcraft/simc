// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Mage
// ==========================================================================

enum { ROTATION_NONE=0, ROTATION_DPS, ROTATION_DPM, ROTATION_MAX };

struct mage_t : public player_t
{
  // Active
  spell_t* active_ignite;
  pet_t*   active_water_elemental;

  // Active Spells
  struct active_spells_t
  {
    active_spell_t* arcane_barrage;
    active_spell_t* pyroblast;
    active_spell_t* summon_water_elemental;
  };
  active_spells_t active_spells;

  // Buffs
  buff_t* buffs_arcane_blast;
  buff_t* buffs_arcane_potency;
  buff_t* buffs_arcane_power;
  buff_t* buffs_brain_freeze;
  buff_t* buffs_clearcasting;
  buff_t* buffs_combustion;
  buff_t* buffs_fingers_of_frost;
  buff_t* buffs_focus_magic_feedback;
  buff_t* buffs_ghost_charge;
  buff_t* buffs_hot_streak;
  buff_t* buffs_hot_streak_crits;
  buff_t* buffs_icy_veins;
  buff_t* buffs_incanters_absorption;
  buff_t* buffs_mage_armor;
  buff_t* buffs_missile_barrage;
  buff_t* buffs_molten_armor;
  buff_t* buffs_tier10_2pc;
  buff_t* buffs_tier10_4pc;

  // Cooldowns
  struct _cooldowns_t
  {
    double enduring_winter;
    void reset() { memset( ( void* ) this, 0x00, sizeof( _cooldowns_t ) ); }
    _cooldowns_t() { reset(); }
  };
  _cooldowns_t _cooldowns;

  // Events
  event_t* ignite_delay_event;

  // Gains
  gain_t* gains_clearcasting;
  gain_t* gains_empowered_fire;
  gain_t* gains_evocation;
  gain_t* gains_mage_armor;
  gain_t* gains_mana_gem;
  gain_t* gains_master_of_elements;

  // Glyphs
  struct glyphs_t
  {
    // Prime
    int arcane_blast;
    int cone_of_cold;
    int ice_lance;
    int living_bomb;
    int mage_armor;
    int pyroblast;

    // Major
    int arcane_barrage;
    int arcane_missiles;
    int dragons_breath;
    int fireball;
    int frostbolt;
    int frostfire;
    int mirror_image;
    int molten_armor;

    // Minor
    int arcane_brilliance;

    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  // Mastery
  struct mastery_spells_t
  {
    passive_spell_t* flashburn;
    passive_spell_t* frostburn;
    passive_spell_t* mana_adept;
  };
  mastery_spells_t mastery;

  // Options
  std::string focus_magic_target_str;
  std::string armor_type_str;
  double      ghost_charge_pct;
  int         fof_on_cast;

  // Passive Spells
  struct passive_spells_t
  {
    passive_spell_t* arcane_specialization;
    passive_spell_t* fire_specialization;
    passive_spell_t* frost_specialization;
  };
  passive_spells_t passive_spells;

  // Procs
  proc_t* procs_deferred_ignite;
  proc_t* procs_munched_ignite;
  proc_t* procs_rolled_ignite;
  proc_t* procs_mana_gem;

  // Random Number Generation
  rng_t* rng_empowered_fire;
  rng_t* rng_enduring_winter;
  rng_t* rng_ghost_charge;

  // Rotation (DPS vs DPM)
  struct rotation_t
  {
    int    current;
    double mana_gain;
    double dps_mana_loss;
    double dpm_mana_loss;
    double dps_time;
    double dpm_time;

    void reset() { memset( ( void* ) this, 0x00, sizeof( rotation_t ) ); current = ROTATION_DPS; }
    rotation_t() { reset(); }
  };
  rotation_t rotation;

  // Talents
  struct talents_list_t
  {
    // Arcane
    talent_t* arcane_concentration;
    talent_t* arcane_flows;
    talent_t* arcane_potency;
    talent_t* arcane_power;
    talent_t* arcane_tactics;
    talent_t* focus_magic;
    talent_t* improved_arcane_explosion;
    talent_t* improved_arcane_missiles;
    talent_t* improved_blink;
    talent_t* improved_counterspell;
    talent_t* improved_mana_gem;
    talent_t* improved_polymorph;
    talent_t* incanters_absorption;
    talent_t* invocation;
    talent_t* missile_barrage;
    talent_t* nether_vortex;
    talent_t* netherwind_presence;
    talent_t* presence_of_mind;
    talent_t* prismatic_cloak;
    talent_t* slow;
    talent_t* torment_the_weak;

    // Fire
    talent_t* blast_wave;
    talent_t* blazing_speed;
    talent_t* burning_soul;
    talent_t* cauterize;
    talent_t* combustion;
    talent_t* critical_mass;
    talent_t* dragons_breath;
    talent_t* fire_power;
    talent_t* firestarter;
    talent_t* hot_streak;
    talent_t* ignite;
    talent_t* impact;
    talent_t* improved_fire_blast;
    talent_t* improved_flamestrike;
    talent_t* improved_hot_streak;
    talent_t* improved_scorch;
    talent_t* living_bomb;
    talent_t* master_of_elements;
    talent_t* molten_fury;
    talent_t* molten_shields;
    talent_t* pyromaniac;

    // Frost
    talent_t* brain_freeze;
    talent_t* cold_snap;
    talent_t* deep_freeze;
    talent_t* early_frost;
    talent_t* enduring_winter;
    talent_t* fingers_of_frost;
    talent_t* frostfire_orb;
    talent_t* ice_barrier;
    talent_t* ice_floes;
    talent_t* ice_shards;
    talent_t* icy_veins;
    talent_t* improved_cone_of_cold;
    talent_t* improved_freeze;
    talent_t* permafrost;
    talent_t* piercing_chill;
    talent_t* piercing_ice;
    talent_t* reactive_barrier;
    talent_t* shatter;
    talent_t* shattered_barrier;

    talents_list_t() { memset( ( void* ) this, 0x0, sizeof( talents_list_t ) ); }
  };
  talents_list_t talents;

  // Up-Times
  uptime_t* uptimes_arcane_blast[ 5 ];
  uptime_t* uptimes_dps_rotation;
  uptime_t* uptimes_dpm_rotation;
  uptime_t* uptimes_water_elemental;

  mage_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) : player_t( sim, MAGE, name, r )
  {
    tree_type[ MAGE_ARCANE ] = TREE_ARCANE;
    tree_type[ MAGE_FIRE   ] = TREE_FIRE;
    tree_type[ MAGE_FROST  ] = TREE_FROST;

    // Active
    active_ignite          = 0;
    active_water_elemental = 0;

    armor_type_str   = "molten"; // Valid values: molten|mage
    distance         = 40;
    fof_on_cast      = 0;
    ghost_charge_pct = 1.0;
  }

  // Character Definition
  virtual void      init_talents();
  virtual void      init_spells();
  virtual void      init_race();
  virtual void      init_base();
  virtual void      init_glyphs();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_uptimes();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      combat_begin();
  virtual void      reset();
  virtual action_expr_t* create_expression( action_t*, const std::string& name );
  virtual std::vector<talent_translation_t>& get_talent_list();
  virtual std::vector<option_t>& get_options();
  virtual bool      create_profile( std::string& profile_str, int save_type=SAVE_ALL );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_MANA; }
  virtual int       primary_role() SC_CONST     { return ROLE_SPELL; }
  virtual double    composite_spell_power( const school_type school ) SC_CONST;
  virtual double    composite_spell_crit() SC_CONST;
  virtual double    matching_gear_multiplier( const attribute_type attr ) SC_CONST;

  // Event Tracking
  virtual void   regen( double periodicity );
  virtual double resource_gain( int resource, double amount, gain_t* source=0, action_t* action=0 );
  virtual double resource_loss( int resource, double amount, action_t* action=0 );
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Mage Spell
// ==========================================================================

struct mage_spell_t : public spell_t
{
  int dps_rotation;
  int dpm_rotation;
  int arcane_power;
  int icy_veins;
  int min_ab_stack;
  int max_ab_stack;

  mage_spell_t( const char* n, player_t* player, const school_type s, int t ) :
      spell_t( n, player, RESOURCE_MANA, s, t ),
      dps_rotation( 0 ),
      dpm_rotation( 0 )
  {

  }

  mage_spell_t( const char* n, uint32_t id, mage_t* p ) :
    spell_t( n, id, p ),
    dps_rotation( 0 ),
    dpm_rotation( 0 )
  {

  }

  virtual void   parse_options( option_t*, const std::string& );
  virtual bool   ready();
  virtual double cost() SC_CONST;
  virtual double haste() SC_CONST;
  virtual void   execute();
  virtual void   travel( int travel_result, double travel_dmg );
  virtual void   consume_resource();
  virtual void   player_buff();
  virtual void   target_debuff( int dmg_type );
};

// ==========================================================================
// Pet Water Elemental
// ==========================================================================

struct water_elemental_pet_t : public pet_t
{
  struct water_bolt_t : public spell_t
  {
    water_bolt_t( player_t* player ):
      spell_t( "water_bolt", 31707, player )
    {
      may_crit  = true;
    }

    virtual void player_buff()
    {
      spell_t::player_buff();
      player_spell_power += player -> cast_pet() -> owner -> composite_spell_power( SCHOOL_FROST ) / 3.0;
    }
  };

  water_elemental_pet_t( sim_t* sim, player_t* owner ) :
      pet_t( sim, owner, "water_elemental" )
  {
    action_list_str = "water_bolt";
  }

  virtual void init_base()
  {
    pet_t::init_base();

    // Stolen from Priest's Shadowfiend
    attribute_base[ ATTR_STRENGTH  ] = 145;
    attribute_base[ ATTR_AGILITY   ] =  38;
    attribute_base[ ATTR_STAMINA   ] = 190;
    attribute_base[ ATTR_INTELLECT ] = 133;

    health_per_stamina = 7.5;
    mana_per_intellect = 5;
  }

  virtual void summon( double duration=0 )
  {
    pet_t::summon( duration );
    mage_t* o = cast_pet() -> owner -> cast_mage();
    o -> active_water_elemental = this;
  }

  virtual void dismiss()
  {
    pet_t::dismiss();
    mage_t* o = cast_pet() -> owner -> cast_mage();
    o -> active_water_elemental = 0;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "water_bolt" ) return new water_bolt_t( this );

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

  struct arcane_blast_t : public spell_t
  {
    action_t* next_in_sequence;

    arcane_blast_t( mirror_image_pet_t* mirror_image, action_t* nis ):
      spell_t( "arcane_blast", 88084, mirror_image ), next_in_sequence( nis )
    {
      may_crit          = true;
      background        = true;

      // FIXME: This can be removed once it's whitelisted
      base_cost         = 0;
      base_execute_time = 3.0;
      base_dd_min       = 201 / 2;
      base_dd_max       = 233 / 2;
      direct_power_mod  = 0.30;
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
        mirror_image_pet_t* mi = ( mirror_image_pet_t* ) player;
        mi -> sequence_finished++;
        if ( mi -> sequence_finished == mi -> num_images ) mi -> dismiss();
      }
    }

    virtual void player_buff()
    {
      mirror_image_pet_t* p = ( mirror_image_pet_t* ) player;
      spell_t::player_buff();
      player_spell_power += player -> cast_pet() -> owner -> composite_spell_power( SCHOOL_FIRE ) / p -> num_images;
    }
  };

  struct fire_blast_t : public spell_t
  {
    action_t* next_in_sequence;

    fire_blast_t( mirror_image_pet_t* mirror_image, action_t* nis ):
      spell_t( "fire_blast", 59637, mirror_image ), next_in_sequence( nis )
    {
      background        = true;
      may_crit          = true;

      // FIXME: This can be removed once 59637 is whitelisted
      base_cost         = 0;
      base_execute_time = 0;
      base_dd_min       = 88;
      base_dd_max       = 98;
      direct_power_mod  = 0.15;
    }

    virtual void player_buff()
    {
      mirror_image_pet_t* p = ( mirror_image_pet_t* ) player;
      spell_t::player_buff();
      player_spell_power += player -> cast_pet() -> owner -> composite_spell_power( SCHOOL_FIRE ) / p -> num_images;
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
        mirror_image_pet_t* mi = ( mirror_image_pet_t* ) player;
        mi -> sequence_finished++;
        if ( mi -> sequence_finished == mi -> num_images ) mi -> dismiss();
      }
    }
  };

  struct fireball_t : public spell_t
  {
    action_t* next_in_sequence;

    fireball_t( mirror_image_pet_t* mirror_image, action_t* nis ):
      spell_t( "fireball", 88082, mirror_image ), next_in_sequence( nis )
    {
      may_crit          = true;
      background        = true;

      // FIXME: This can be removed once it's whitelisted
      base_cost         = 0;
      base_execute_time = 3.0;
      base_dd_min       = 242 / 2;
      base_dd_max       = 308 / 2;
      direct_power_mod  = 0.30;
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
        mirror_image_pet_t* mi = ( mirror_image_pet_t* ) player;
        mi -> sequence_finished++;
        if ( mi -> sequence_finished == mi -> num_images ) mi -> dismiss();
      }
    }

    virtual void player_buff()
    {
      mirror_image_pet_t* p = ( mirror_image_pet_t* ) player;
      spell_t::player_buff();
      player_spell_power += player -> cast_pet() -> owner -> composite_spell_power( SCHOOL_FIRE ) / p -> num_images;
    }
  };

  struct frostbolt_t : public spell_t
  {
    action_t* next_in_sequence;

    frostbolt_t( mirror_image_pet_t* mirror_image, action_t* nis ):
      spell_t( "frost_bolt", 59638, mirror_image ), next_in_sequence( nis )
    {
      may_crit          = true;
      background        = true;

      // FIXME: This can be removed once it's whitelisted
      base_cost         = 0;
      base_execute_time = 3.0;
      base_dd_min       = 163;
      base_dd_max       = 169;
      direct_power_mod  = 0.30;
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
        mirror_image_pet_t* mi = ( mirror_image_pet_t* ) player;
        mi -> sequence_finished++;
        if ( mi -> sequence_finished == mi -> num_images ) mi -> dismiss();
      }
    }

    virtual void player_buff()
    {
      mirror_image_pet_t* p = ( mirror_image_pet_t* ) player;
      spell_t::player_buff();
      player_spell_power += player -> cast_pet() -> owner -> composite_spell_power( SCHOOL_FROST ) / p -> num_images;
    }
  };

  mirror_image_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "mirror_image", true /*guardian*/ ), num_images( 3 ), num_rotations( 2 ), sequence_finished( 0 )
  {}
  virtual void init_base()
  {
    pet_t::init_base();

    // Stolen from Priest's Shadowfiend
    attribute_base[ ATTR_STRENGTH  ] = 145;
    attribute_base[ ATTR_AGILITY   ] =  38;
    attribute_base[ ATTR_STAMINA   ] = 190;
    attribute_base[ ATTR_INTELLECT ] = 133;

    health_per_stamina = 7.5;
    mana_per_intellect = 5;
  }

  virtual void init_actions()
  {
    for ( int i=0; i < num_images; i++ )
    {
      action_t* front=0;

      if ( owner -> cast_mage() -> glyphs.mirror_image && owner -> cast_mage() -> primary_tree() != TREE_FROST )
      {
        // Fire/Arcane Mages cast 9 Fireballs/Arcane Blasts
        num_rotations = 9;    
        for ( int j=0; j < num_rotations; j++ )
        {
          if ( owner -> cast_mage() -> primary_tree() == TREE_FIRE )
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
        num_rotations = 2;
        for ( int j=0; j < num_rotations; j++ )
        {
          // Mirror Image casts 10 Frostbolts, 4 Fire Blasts
          front = new frostbolt_t ( this, front );
          front = new frostbolt_t ( this, front );
          front = new frostbolt_t ( this, front );
          front = new fire_blast_t( this, front );
          front = new frostbolt_t ( this, front );
          front = new frostbolt_t ( this, front );
          front = new fire_blast_t( this, front );
        }
      }
      sequences.push_back( front );
    }
  }

  virtual void summon( double duration=0 )
  {
    pet_t::summon( duration );

    sequence_finished = 0;

    for ( int i=0; i < num_images; i++ )
    {
      sequences[ i ] -> schedule_execute();
    }
  }

  virtual void interrupt()
  {
    pet_t::interrupt();

    dismiss(); // FIXME! Interrupting them is too hard, just dismiss for now.
  }
};

// trigger_ignite ===========================================================

static void trigger_ignite( spell_t* s, double dmg )
{
  if ( s -> school != SCHOOL_FIRE &&
       s -> school != SCHOOL_FROSTFIRE ) return;

  mage_t* p = s -> player -> cast_mage();
  sim_t* sim = s -> sim;

  if ( ! p -> talents.ignite -> rank() ) return;

  struct ignite_t : public mage_spell_t
  {
    ignite_t( player_t* player ) :
        mage_spell_t( "ignite", player, SCHOOL_FIRE, TREE_FIRE )
    {
      base_tick_time = 2.0;
      num_ticks      = 2;
      trigger_gcd    = 0;
      background     = true;
      proc           = true;
      may_resist     = true;
      scale_with_haste = false;
      reset();
    }
    virtual void target_debuff( int dmg_type ) {}
    virtual void player_buff() {}
  };

  double ignite_dmg = dmg * p -> talents.ignite -> effect_base_value( 1 ) / 100.0;

  if ( sim -> merge_ignite )
  {
    int result = s -> result;
    s -> result = RESULT_HIT;
    s -> assess_damage( ignite_dmg, DMG_OVER_TIME );
    s -> update_stats( DMG_OVER_TIME );
    s -> result = result;
    return;
  }

  if ( ! p -> active_ignite ) p -> active_ignite = new ignite_t( p );

  if ( p -> active_ignite -> ticking )
  {
    int number_ticks = p -> active_ignite -> number_ticks;
    int remaining_ticks = number_ticks - p -> active_ignite -> current_tick;

    ignite_dmg += p -> active_ignite -> base_td * remaining_ticks;
  }

  // The Ignite SPELL_AURA_APPLIED does not actually occur immediately.
  // There is a short delay which can result in "munched" or "rolled" ticks.

  if ( sim -> aura_delay == 0 )
  {
    // Do not model the delay, so no munch/roll, just defer.

    if ( p -> active_ignite -> ticking )
    {
      if ( sim -> log ) log_t::output( sim, "Player %s defers Ignite.", p -> name() );
      p -> procs_deferred_ignite -> occur();
      p -> active_ignite -> cancel();
    }
    p -> active_ignite -> base_td = ignite_dmg / 2.0; // Fix hardcoded num_ticks
    //p -> active_ignite -> schedule_tick();
    return;
  }

  struct ignite_delay_t : public event_t
  {
    double ignite_dmg;

    ignite_delay_t( sim_t* sim, player_t* p, double dmg ) : event_t( sim, p ), ignite_dmg( dmg )
    {
      name = "Ignite Delay";
      sim -> add_event( this, sim -> gauss( sim -> aura_delay, sim -> aura_delay * 0.25 ) );
    }
    virtual void execute()
    {
      mage_t* p = player -> cast_mage();

      if ( p -> active_ignite -> ticking )
      {
	if ( sim -> log ) log_t::output( sim, "Player %s defers Ignite.", p -> name() );
	p -> procs_deferred_ignite -> occur();
	p -> active_ignite -> cancel();
      }

      p -> active_ignite -> base_td = ignite_dmg / 2.0; // Fix hardcoded num_ticks
      p -> active_ignite -> schedule_tick();

      if ( p -> ignite_delay_event == this ) p -> ignite_delay_event = 0;
    }
  };

  if ( p -> ignite_delay_event ) 
  {
    // There is an SPELL_AURA_APPLIED already in the queue.
    if ( sim -> log ) log_t::output( sim, "Player %s munches Ignite.", p -> name() );
    p -> procs_munched_ignite -> occur();
  }

  p -> ignite_delay_event = new ( sim ) ignite_delay_t( sim, p, ignite_dmg );

  if ( p -> active_ignite -> ticking )
  {
    if ( p -> active_ignite -> tick_event -> occurs() < 
	       p -> ignite_delay_event -> occurs() )
    {
      // Ignite will tick before SPELL_AURA_APPLIED occurs, which means that the current Ignite will
      // both tick -and- get rolled into the next Ignite.
      if ( sim -> log ) log_t::output( sim, "Player %s rolls Ignite.", p -> name() );
      p -> procs_rolled_ignite -> occur();
    }
  }
}

// trigger_master_of_elements ===============================================

static void trigger_master_of_elements( spell_t* s, double adjust )
{
  mage_t* p = s -> player -> cast_mage();

  if ( s -> resource_consumed == 0 )
    return;

  if ( ! p -> talents.master_of_elements -> rank() )
    return;

  p -> resource_gain( RESOURCE_MANA, adjust * s -> base_cost * p -> talents.master_of_elements -> effect_base_value( 1 ) / 100.0, p -> gains_master_of_elements );
}

// clear_fingers_of_frost ===================================================

static void clear_fingers_of_frost( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  p -> buffs_ghost_charge -> expire();

  if ( s -> may_crit && p -> buffs_fingers_of_frost -> check() )
  {
    p -> buffs_fingers_of_frost -> decrement();

    if ( ! p -> buffs_fingers_of_frost -> check() )
    {
      if ( p -> gcd_ready < p -> sim -> current_time )
      {
        double value = p -> rng_ghost_charge -> roll( p -> ghost_charge_pct ) ? +1.0 : -1.0;

        p -> buffs_ghost_charge -> start( 1, value );
      }
    }
  }
}

// trigger_brain_freeze =====================================================

static void trigger_brain_freeze( spell_t* s )
{
  if ( s -> school != SCHOOL_FROST &&
       s -> school != SCHOOL_FROSTFIRE )
    return;

  // Game hotfixed so that Frostfire Bolt cannot proc Brain Freeze.
  if ( s -> name_str == "frostfire_bolt" )
    return;

  mage_t* p = s -> player -> cast_mage();

  p -> buffs_brain_freeze -> trigger();
}

// consume_brain_freeze =====================================================

static void consume_brain_freeze( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if ( p -> buffs_brain_freeze -> check() )
  {
    p -> buffs_brain_freeze -> expire();
    p -> buffs_tier10_2pc -> trigger();
  }
}

// trigger_hot_streak =======================================================

static void trigger_hot_streak( spell_t* s )
{
  sim_t* sim = s -> sim;
  mage_t*  p = s -> player -> cast_mage();

  if ( ! p -> talents.improved_hot_streak -> rank() )
    return;

  int result = s -> result;

  if ( sim -> smooth_rng && s -> result_is_hit() )
  {
    // Decouple Hot Streak proc from actual crit to reduce wild swings during RNG smoothing.
    result = sim -> rng -> roll( s -> total_crit() ) ? RESULT_CRIT : RESULT_HIT;
  }
  if ( result == RESULT_CRIT )
  {
    p -> buffs_hot_streak_crits -> trigger();

    if ( p -> buffs_hot_streak_crits -> stack() == 2 )
    {
      p -> buffs_hot_streak -> trigger();
      p -> buffs_hot_streak_crits -> expire();
    }
  }
  else
  {
    p -> buffs_hot_streak_crits -> expire();
  }
}

// trigger_replenishment ====================================================

static void trigger_replenishment( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if ( ! p -> talents.enduring_winter -> rank() )
    return;

  if ( p -> sim -> current_time < p -> _cooldowns.enduring_winter ) // FIXME: This is no longer listed on the talent, is it still in effect?
    return;

  if ( ! p -> rng_enduring_winter -> roll( p -> talents.enduring_winter -> proc_chance() ) )
    return;

  p -> trigger_replenishment();

  p -> _cooldowns.enduring_winter = p -> sim -> current_time + 6.0;
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
    { "dps",          OPT_BOOL, &dps_rotation },
    { "dpm",          OPT_BOOL, &dpm_rotation },
    { NULL, OPT_UNKNOWN, NULL }
  };
  std::vector<option_t> merged_options;
  spell_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// mage_spell_t::ready ======================================================

bool mage_spell_t::ready()
{
  mage_t* p = player -> cast_mage();

  if ( dps_rotation )
    if ( p -> rotation.current != ROTATION_DPS )
      return false;

  if ( dpm_rotation )
    if ( p -> rotation.current != ROTATION_DPM )
      return false;

  return spell_t::ready();
}

// mage_spell_t::cost =======================================================

double mage_spell_t::cost() SC_CONST
{
  mage_t* p = player -> cast_mage();
  if ( p -> buffs_clearcasting -> check() )
    return 0;
  double c = spell_t::cost();
  if ( p -> buffs_arcane_power -> check() )
    c += base_cost * 0.20;
  return c;
}

// mage_spell_t::haste ======================================================

double mage_spell_t::haste() SC_CONST
{
  mage_t* p = player -> cast_mage();
  double h = spell_t::haste();
  if ( p -> buffs_icy_veins -> up() )
  {
    h *= 1.0 / ( 1.0 + 0.20 );
  }
  if ( p -> talents.netherwind_presence -> rank() )
  {
    h *= 1.0 / ( 1.0 + p -> talents.netherwind_presence -> effect_base_value( 1 ) / 100.0 );
  }
  if ( p -> buffs_tier10_2pc -> up() )
  {
    h *= 1.0 / 1.12;
  }
  return h;
}

// mage_spell_t::execute ====================================================

void mage_spell_t::execute()
{
  mage_t* p = player -> cast_mage();

  p -> uptimes_dps_rotation -> update( p -> rotation.current == ROTATION_DPS );
  p -> uptimes_dpm_rotation -> update( p -> rotation.current == ROTATION_DPM );
  if (p -> buffs_arcane_potency -> up() )
    p -> buffs_arcane_potency -> decrement();

  spell_t::execute();

  if ( result_is_hit() )
  {
    clear_fingers_of_frost( this );
    
    if ( ! dual )
    {
      p -> buffs_clearcasting -> trigger();
    }

    if ( result == RESULT_CRIT )
    {
      trigger_master_of_elements( this, 1.0 );

      if ( time_to_travel == 0 )
      {
	      trigger_ignite( this, direct_dmg );
      }
    }
  }
  trigger_brain_freeze( this );
}

// mage_spell_t::travel =====================================================

void mage_spell_t::travel( int travel_result, double travel_dmg )
{
  spell_t::travel( travel_result, travel_dmg);

  if ( travel_result == RESULT_CRIT )
  {
    trigger_ignite( this, direct_dmg );
  }
}

// mage_spell_t::consume_resource ===========================================

void mage_spell_t::consume_resource()
{
  mage_t* p = player -> cast_mage();
  spell_t::consume_resource();
  if ( p -> buffs_clearcasting -> up() )
  {
    // Treat the savings like a mana gain.
    double amount = spell_t::cost();
    if ( amount > 0 )
    {
      p -> buffs_clearcasting -> expire();
      p -> gains_clearcasting -> add( amount );
    }
  }
}

// mage_spell_t::player_buff ================================================

void mage_spell_t::player_buff()
{
  mage_t* p = player -> cast_mage();

  spell_t::player_buff();

  if ( p -> talents.molten_fury -> rank() )
  {
    if ( sim -> target -> health_percentage() < 35 )
    {
      player_multiplier *= 1.0 + p -> talents.molten_fury -> effect_base_value( 1 ) / 100.0;
    }
  }

  if ( p -> buffs_tier10_4pc -> up() )
  {
    player_multiplier *= 1.18;
  }

  double fire_power_multiplier   = 0;

  if ( school == SCHOOL_ARCANE )
  {
    if ( sim -> target -> debuffs.snared() )
    {
      player_multiplier *= 1.0 + p -> talents.torment_the_weak -> effect_base_value( 1 ) / 100.0;
    }
    
    player_multiplier *= 1.0 + p -> passive_spells.arcane_specialization -> effect_base_value( 1 ) / 100.0;
      
    double mana_pct = player -> resource_current[ RESOURCE_MANA ] / player -> resource_max [ RESOURCE_MANA ];
    player_multiplier *= 1.0 + p -> mastery.mana_adept -> effect_base_value( 2 ) / 10000.0 * mana_pct;  
  }
  else if ( school == SCHOOL_FIRE || school == SCHOOL_FROSTFIRE )
  {
    player_multiplier *= 1.0 + p -> passive_spells.fire_specialization -> effect_base_value( 1 ) / 100.0;

    fire_power_multiplier = p -> talents.fire_power -> effect_base_value( 1 ) / 100.0;
  }
  if ( p -> talents.shatter -> rank() && may_crit )
  {
    if ( p -> talents.fingers_of_frost -> rank() )
    {
      if ( p -> buffs_fingers_of_frost -> may_react() )
      {
        player_crit += util_t::talent_rank( p -> talents.shatter -> rank(), 2, 0.02, 0.03 );
      }
      else if ( time_to_execute == 0 )
      {
        if ( p -> buffs_ghost_charge -> value() > 0 )
        {
          player_crit += util_t::talent_rank( p -> talents.shatter -> rank(), 2, 0.02, 0.03 );
        }
      }
    }
  }
  else if ( school == SCHOOL_FROST )
  {
    player_multiplier *= 1.0 + p -> passive_spells.frost_specialization -> effect_base_value( 1 ) / 100.0;

    if ( p -> buffs_fingers_of_frost -> may_react() )
    {
      player_multiplier *= 1.0 + p -> mastery.frostburn -> effect_base_value( 2 ) / 10000.0 * p -> composite_mastery();
    }
  }

  double arcane_power_multiplier = p -> buffs_arcane_power -> check() ? 0.20 : 0.0;

  player_multiplier *= 1.0 + arcane_power_multiplier + fire_power_multiplier;

  if ( p -> buffs_arcane_potency -> up() && ! dual )
  {
    player_crit += p -> talents.arcane_potency -> effect_base_value( 1 ) / 100.0;
  }

  if ( sim -> debug )
    log_t::output( sim, "mage_spell_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f mult=%.2f",
                   name(), player_hit, player_crit, player_spell_power, player_penetration, player_multiplier );
}

// mage_spell_t::target_debuff ==============================================

void mage_spell_t::target_debuff( int dmg_type )
{
  spell_t::target_debuff( dmg_type );

  if ( school == SCHOOL_FIRE && dmg_type == DMG_OVER_TIME )
  {
    mage_t* p = player -> cast_mage();

    target_multiplier *= 1.0 + p -> mastery.flashburn -> effect_base_value( 2 ) / 10000.0 * p -> composite_mastery();
  }
}

// ==========================================================================
// Mage Spells
// ==========================================================================

// Arcane Barrage Spell =====================================================

struct arcane_barrage_t : public mage_spell_t
{
  arcane_barrage_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "arcane_barrage", 44425, player )
  {
    mage_t* p = player -> cast_mage();
    check_spec( TREE_ARCANE );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    
    base_multiplier *= 1.0 - p -> glyphs.arcane_barrage * 0.04;
    may_crit         = true;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    p -> buffs_arcane_blast -> expire();
  }
};

// Arcane Blast Spell =======================================================

struct arcane_blast_t : public mage_spell_t
{
  arcane_blast_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "arcane_blast", 30451, player )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_crit = true;
  }

  virtual double cost() SC_CONST
  {
    mage_t* p = player -> cast_mage();
    double c = mage_spell_t::cost();
    if ( c != 0 )
    {
      c += base_cost * p -> buffs_arcane_blast -> stack() * 1.75;
    }
    return c;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    p -> buffs_arcane_blast -> trigger();
  }

  virtual void player_buff()
  {
    mage_spell_t::player_buff();
    mage_t* p = player -> cast_mage();
    player_multiplier *= 1.0 + p ->  buffs_arcane_blast -> stack() * ( 0.20 + ( p -> glyphs.arcane_blast ? 0.03 : 0.00 ) );
  }
};

// Arcane Brilliance Spell =================================================

struct arcane_brilliance_t : public mage_spell_t
{
  double bonus;

  arcane_brilliance_t( mage_t* player, const std::string& options_str ) :
      mage_spell_t( "arcane_brilliance", 1459, player ), bonus( 0 )
  {
    mage_t* p = player -> cast_mage();

    bonus      = p -> player_data.effect_min( 79058, p -> level, E_APPLY_AURA, A_MOD_INCREASE_ENERGY );
    base_cost *= 1.0 - p -> glyphs.arcane_brilliance * 0.5;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> ooc_buffs() )
      {
        p -> buffs.arcane_brilliance -> trigger( 1, bonus );
        p -> init_resources( true );
      }
    }
  }

  virtual bool ready()
  {
    return( player -> buffs.arcane_brilliance -> current_value < bonus );
  }
};

// Arcane Explosion Spell ===================================================

struct arcane_explosion_t : public mage_spell_t
{
  arcane_explosion_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "arcane_explosion", 1449, player )
  {
    check_spec( TREE_ARCANE );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    
    aoe      = true;
    may_crit = true;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    p -> buffs_arcane_blast -> expire();
  }
};

// Arcane Missiles Spell ====================================================

struct arcane_missiles_tick_t : public mage_spell_t
{
  arcane_missiles_tick_t( mage_t* player ) :
    mage_spell_t( "arcane_missiles", 7268, player )
  {
    mage_t* p = player -> cast_mage();

    dual        = true;
    background  = true;
    may_crit    = true;
    direct_tick = true;
    base_crit  += p -> glyphs.arcane_missiles * 0.05;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    tick_dmg = direct_dmg;
    update_stats( DMG_OVER_TIME );
    if ( result == RESULT_CRIT )
    {
      trigger_master_of_elements( this, 1.0 / num_ticks ); // FIXME: Does this change w/ the num of ticks or always 3 (base # of ticks)?
    }
  }
};

struct arcane_missiles_t : public mage_spell_t
{
  arcane_missiles_tick_t* arcane_missiles_tick;

  arcane_missiles_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "arcane_missiles", 5143, player )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    channeled      = true;
    num_ticks      = 3;
    base_tick_time = 0.75;

    arcane_missiles_tick = new arcane_missiles_tick_t( p );
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    //mage_t* p = player -> cast_mage();
    // Decrease Arcane Missiles! buff
  }

  virtual void tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
    arcane_missiles_tick -> execute();
    update_time( DMG_OVER_TIME );
  }

  virtual void last_tick()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::last_tick();
    p -> buffs_arcane_blast -> expire();
    p -> buffs_tier10_2pc -> trigger();
  }
};

// Arcane Power Spell =======================================================

struct arcane_power_t : public mage_spell_t
{
  arcane_power_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "arcane_power", 12042, player )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.arcane_power -> rank() );

    cooldown -> duration *= 1.0 + p -> talents.arcane_flows -> effect_base_value( 1 ) / 100.0;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    mage_t* p = player -> cast_mage();
    p -> buffs_arcane_power -> trigger( 1, effect_base_value( 1 ) / 100.0 );
  }
};

// Blast Wave Spell =========================================================

struct blast_wave_t : public mage_spell_t
{
  blast_wave_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "blast_wave", 11113, player )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    
    aoe      = true;
    may_crit = true;
  }
};

// Cold Snap Spell ==========================================================

struct cold_snap_t : public mage_spell_t
{
  std::vector<cooldown_t*> cooldown_list;

  cold_snap_t( mage_t* player, const std::string& options_str ) :
      mage_spell_t( "cold_snap", 11958, player )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.cold_snap -> rank() );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown_list.push_back( p -> get_cooldown( "deep_freeze"     ) );
    cooldown_list.push_back( p -> get_cooldown( "icy_veins"       ) );
    cooldown_list.push_back( p -> get_cooldown( "water_elemental" ) );
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    int num_cooldowns = (int) cooldown_list.size();
    for ( int i=0; i < num_cooldowns; i++ )
    {
      cooldown_list[ i ] -> reset();
    }
  }
};

// Combustion Spell =========================================================

struct combustion_t : public mage_spell_t
{
  combustion_t( mage_t* player, const std::string& options_str ) :
      mage_spell_t( "combustion", 11129, player )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.combustion -> rank() );
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    // mage_t* p = player -> cast_mage();
    // FIXME: Add Combustion dot
    // The dot takes current tick value of Ignite/Pyro/Living bomb/Glyphed FFB, sums it up to create a new dot
    // That Dot ticks once a second for 10 seconds
  }
};

// Cone of Cold Spell =======================================================

struct cone_of_cold_t : public mage_spell_t
{
  cone_of_cold_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "cone_of_cold", 120, player )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    
    aoe              = true;
    may_crit         = true;
    base_multiplier *= 1.0 + p -> glyphs.cone_of_cold * 0.25;
  }
};

// Counterspell Spell =======================================================

struct counterspell_t : public mage_spell_t
{
  counterspell_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "counterspell", 2139, player )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_miss = may_resist = false;
  }

  virtual bool ready()
  {
    if ( ! sim -> target -> debuffs.casting -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Deep Freeze Spell =========================================================

struct deep_freeze_strike_t : public mage_spell_t
{
  deep_freeze_strike_t( mage_t* player ) :
    mage_spell_t( "deep_freeze", 71757, player )
  {
    background = true;
    dual       = true;
    may_crit   = true;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    update_stats( DMG_DIRECT );
    if ( result == RESULT_CRIT )
    {
      trigger_master_of_elements( this, 1.0 );
    }
  }
};

struct deep_freeze_t : public mage_spell_t
{
  int ghost_charge;
  deep_freeze_strike_t* deep_freeze_strike;

  deep_freeze_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "deep_freeze", 44572, player ), ghost_charge( -1 )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "ghost_charge", OPT_BOOL, &ghost_charge },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    deep_freeze_strike = new deep_freeze_strike_t( p );
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    deep_freeze_strike -> execute();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if ( ! mage_spell_t::ready() )
      return false;

    if ( ghost_charge != -1 )
      if ( ghost_charge != ( p -> buffs_ghost_charge -> check() ? 1 : 0 ) )
        return false;

    if ( ! p -> buffs_fingers_of_frost -> may_react() )
      return false;

    return true;
  }
};

// Dragon's Breath Spell ====================================================

struct dragons_breath_t : public mage_spell_t
{
  dragons_breath_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "dragons_breath", 31661, player )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    
    aoe                   = true;
    may_crit              = true;
    cooldown -> duration -= p -> glyphs.dragons_breath * 3.0;
  }
};

// Evocation Spell ==========================================================

struct evocation_t : public mage_spell_t
{
  evocation_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "evocation", 12051, player )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_execute_time     = 6.0;
    base_tick_time        = 2.0;
    num_ticks             = 3;
    tick_zero             = true;
    channeled             = true;
    harmful               = false;
    cooldown -> duration += p -> talents.arcane_flows -> effect_base_value( 2 ) / 1000.0;
  }

  virtual void tick()
  {
    mage_t* p = player -> cast_mage();
    double mana = p -> resource_max[ RESOURCE_MANA ] * effect_base_value( 1 ) / 100.0;
    p -> resource_gain( RESOURCE_MANA, mana, p -> gains_evocation );
  }

  virtual bool ready()
  {
    if ( ! mage_spell_t::ready() )
      return false;

    return ( player -> resource_current[ RESOURCE_MANA ] /
             player -> resource_max    [ RESOURCE_MANA ] ) < 0.45;
  }
};

// Fire Blast Spell =========================================================

struct fire_blast_t : public mage_spell_t
{
  fire_blast_t( mage_t* player, const std::string& options_str ) :
      mage_spell_t( "fire_blast", 2136, player )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_crit   = true;
    base_crit += p -> talents.improved_fire_blast -> effect_base_value( 1 ) / 100.0;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    trigger_hot_streak( this );
  }
};

// Fireball Spell ===========================================================

struct fireball_t : public mage_spell_t
{
  int frozen;
  int ghost_charge;

  fireball_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "fireball", 133, player ), frozen( -1 ), ghost_charge( -1 )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "frozen",       OPT_BOOL, &frozen       },
      { "ghost_charge", OPT_BOOL, &ghost_charge },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_crit = true;
    base_crit += p -> glyphs.fireball * 0.05;
  }

  virtual double cost() SC_CONST
  {
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_brain_freeze -> check() )
      return 0;

    return mage_spell_t::cost();
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    trigger_hot_streak( this );
    consume_brain_freeze( this );
  }

  virtual double execute_time() SC_CONST
  {
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_brain_freeze -> check() )
      return 0;

    return mage_spell_t::execute_time();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if ( ! mage_spell_t::ready() )
      return false;

    if ( ghost_charge != -1 )
      if ( ghost_charge != ( p -> buffs_ghost_charge -> check() ? 1 : 0 ) )
        return false;

    if ( frozen != -1 )
      if ( frozen != (int) p -> buffs_fingers_of_frost -> may_react() )
        return false;

    return true;
  }
};

// Flame Orb Spell ==========================================================

struct flame_orb_strike_t : public mage_spell_t
{
  flame_orb_strike_t( mage_t* player ) :
    mage_spell_t( "flame_orb", 82739, player )
  {
    dual        = true;
    background  = true;
    may_crit    = true;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    tick_dmg = direct_dmg;
    update_stats( DMG_DIRECT );
    if ( result == RESULT_CRIT )
    {
      trigger_master_of_elements( this, 1.0 );
    }
    trigger_hot_streak( this );
  }
};

struct flame_orb_t : public mage_spell_t
{
  flame_orb_strike_t* flame_orb_strike;

  flame_orb_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "flame_orb", 82731, player )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    flame_orb_strike = new flame_orb_strike_t( p );
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    flame_orb_strike -> execute();
  }
};

// Focus Magic Spell ========================================================

struct focus_magic_t : public mage_spell_t
{
  player_t*          focus_magic_target;
  action_callback_t* focus_magic_cb;

  focus_magic_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "focus_magic", 54646, player ),
    focus_magic_target(0), focus_magic_cb(0)
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.focus_magic -> rank() );

    std::string target_str = p -> focus_magic_target_str;

    option_t options[] =
    {
      { "target", OPT_STRING, &target_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
        
    if ( target_str.empty() )
    {
      // If no target specified, assume 100% up-time by forcing "buffs.focus_magic_feedback = 1"
      focus_magic_target = p;
    }
    else
    {
      focus_magic_target = sim -> find_player( target_str );

      assert ( focus_magic_target != 0 );
      assert ( focus_magic_target != p );
    }

    struct focus_magic_feedback_callback_t : public action_callback_t
    {
      focus_magic_feedback_callback_t( player_t* p ) : action_callback_t( p -> sim, p ) {}

      virtual void trigger( action_t* a )
      {
        listener -> cast_mage() -> buffs_focus_magic_feedback -> trigger();
      }
    };

    focus_magic_cb = new focus_magic_feedback_callback_t( p );
    focus_magic_cb -> active = false;
    focus_magic_target -> register_spell_result_callback( RESULT_CRIT_MASK, focus_magic_cb );
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if ( sim -> log )
      log_t::output( sim, "%s performs %s", p -> name(), name() );

    if ( focus_magic_target == p )
    {
      if ( sim -> log )
        log_t::output( sim, "%s grants SomebodySomewhere Focus Magic", p -> name() );

      p -> buffs_focus_magic_feedback -> override();
    }
    else
    {
      if ( sim -> log )
        log_t::output( sim, "%s grants %s Focus Magic", p -> name(), focus_magic_target -> name() );

      focus_magic_target -> buffs.focus_magic -> trigger();
      focus_magic_cb -> active = true;
    }
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if ( focus_magic_target == p )
    {
      return ! p -> buffs_focus_magic_feedback -> check();
    }
    else
    {
      return ! focus_magic_target -> buffs.focus_magic -> check();
    }
  }
};

// Frostbolt Spell ==========================================================

struct frostbolt_t : public mage_spell_t
{
  int frozen;

  frostbolt_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "frostbolt", 116, player ), frozen( -1 )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "frozen", OPT_BOOL, &frozen },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_crit   = true;
    base_crit += p -> glyphs.frostbolt * 0.05;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    int fof_on_cast = ( p -> fof_on_cast && ( p -> buffs_fingers_of_frost -> check() != 1 ) ); // Proc-Munching!
    mage_spell_t::execute();
    if ( result_is_hit() )
    {
      trigger_replenishment( this );
      if ( fof_on_cast )
      {
        p -> buffs_fingers_of_frost -> trigger( 2 );
      }
    }
  }

  virtual void travel( int travel_result, double travel_dmg)
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::travel( travel_result, travel_dmg);
    if ( ! p -> fof_on_cast )
      p -> buffs_fingers_of_frost -> trigger( 2 );
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if ( frozen != -1 )
      if ( frozen != (int) p -> buffs_fingers_of_frost -> may_react() )
        return false;

    return mage_spell_t::ready();
  }
};

// Frostfire Bolt Spell =====================================================

struct frostfire_bolt_t : public mage_spell_t
{
  int frozen;
  int ghost_charge;

  frostfire_bolt_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "frostfire_bolt", 44614, player ), frozen( -1 ), ghost_charge( -1 )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "frozen",       OPT_BOOL, &frozen       },
      { "ghost_charge", OPT_BOOL, &ghost_charge },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_crit          = true;

    if ( p -> glyphs.frostfire )
    {
      base_multiplier *= 1.15;
      // FIXME: Add in dot 3% over 12 sec, stacks 3x
    }
  }

  virtual double cost() SC_CONST
  {
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_brain_freeze -> check() )
      return 0;

    return mage_spell_t::cost();
  }

  virtual double execute_time() SC_CONST
  {
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_brain_freeze -> up() )
      return 0;

    return mage_spell_t::execute_time();
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    int fof_on_cast = ( p -> fof_on_cast && ( p -> buffs_fingers_of_frost -> check() != 1 ) ); // Proc-Munching!
    mage_spell_t::execute();
    if ( result_is_hit() )
    {
      if ( fof_on_cast )
        p -> buffs_fingers_of_frost -> trigger( 2 );
    }
    trigger_hot_streak( this );
    consume_brain_freeze( this );
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if ( ghost_charge != -1 )
      if ( ghost_charge != ( p -> buffs_ghost_charge -> check() ? 1 : 0 ) )
        return false;

    if ( frozen != -1 )
      if ( frozen != (int) p -> buffs_fingers_of_frost -> may_react() )
        return false;

    return mage_spell_t::ready();
  }

  virtual void travel(int travel_result, double travel_dmg )
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::travel( travel_result, travel_dmg);
    if ( ! p -> fof_on_cast )
      p -> buffs_fingers_of_frost -> trigger( 2 );
  }
};

// Ice Lance Spell ==========================================================

struct ice_lance_t : public mage_spell_t
{
  int frozen;
  int ghost_charge;

  ice_lance_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "ice_lance", 30455, player ), frozen( -1 ), ghost_charge( -1 )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "frozen",       OPT_BOOL, &frozen       },
      { "ghost_charge", OPT_BOOL, &ghost_charge },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_crit          = true;
    base_multiplier *= 1.0 + p -> glyphs.ice_lance * 0.05;
  }

  virtual void player_buff()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::player_buff();

    if ( p -> buffs_ghost_charge -> value() > 0 || p -> buffs_fingers_of_frost -> up() )
    {
      player_multiplier *= 2.0;
    }
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if ( ! mage_spell_t::ready() )
      return false;

    if ( ghost_charge != -1 )
      if ( ghost_charge != ( p -> buffs_ghost_charge -> check() ? 1 : 0 ) )
        return false;

    if ( frozen != -1 )
      if ( frozen != (int) p -> buffs_fingers_of_frost -> may_react() )
        return false;

    return true;
  }
};

// Icy Veins Spell ==========================================================

struct icy_veins_t : public mage_spell_t
{
  icy_veins_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "icy_veins", 12472, player )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.icy_veins -> rank() );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown -> duration *= 1.0 + p -> talents.ice_floes -> effect_base_value( 1 ) / 100.0;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if ( sim -> log )
      log_t::output( sim, "%s performs %s", p -> name(), name() );

    consume_resource();
    update_ready();
    p -> buffs_icy_veins -> trigger();
  }
};

// Living Bomb Spell ========================================================

struct living_bomb_explosion_t : public mage_spell_t
{
  living_bomb_explosion_t( mage_t* player ) :
    mage_spell_t( "living_bomb", 44461, player )
  {
    mage_t* p = player -> cast_mage();

    aoe              = true;
    dual             = true;
    background       = true;
    may_crit         = true;
    base_multiplier *= 1.0 + p -> glyphs.living_bomb * 0.03;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    if ( result_is_hit() )
    {
      if ( result == RESULT_CRIT )
      {
        trigger_ignite( this, direct_dmg );
        trigger_master_of_elements( this, 1.0 );
      }
      trigger_hot_streak( this );
    }
    update_result( DMG_DIRECT );
  }
};

struct living_bomb_t : public mage_spell_t
{
  living_bomb_explosion_t* living_bomb_explosion;

  living_bomb_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "living_bomb", 44457, player )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.living_bomb -> rank() );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_crit         = true;
    base_multiplier *= 1.0 + p -> glyphs.living_bomb * 0.03;

    living_bomb_explosion = new living_bomb_explosion_t( p );
  }

  virtual void last_tick()
  {
    mage_spell_t::last_tick();
    living_bomb_explosion -> execute();
  }
};

// Mage Armor Spell =========================================================

struct mage_armor_t : public mage_spell_t
{
  mage_armor_t( mage_t* player, const std::string& options_str ) :
      mage_spell_t( "mage_armor", 6117, player )
  {
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if ( sim -> log )
      log_t::output( sim, "%s performs %s", p -> name(), name() );

    p -> buffs_molten_armor -> expire();
    p -> buffs_mage_armor -> trigger();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();
    return ! p -> buffs_mage_armor -> check();
  }
};

// Mana Gem =================================================================

struct mana_gem_t : public action_t
{
  // FIXME: Anyway to derive the value of this?
  double trigger;
  double min;
  double max;
  int charges;
  int used;

  mana_gem_t( player_t* player, const std::string& options_str ) :
      action_t( ACTION_USE, "mana_gem", player ),
      trigger( 0 ), min( 3330 ), max( 3500 ), charges( 3 ), used( 0 )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "min",     OPT_FLT, &min     },
      { "max",     OPT_FLT, &max     },
      { "trigger", OPT_FLT, &trigger },
      { "charges", OPT_FLT, &charges },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( min == 0 && max == 0 ) min = max = trigger;

    if ( min > max ) std::swap( min, max );

    if ( max == 0 ) max = trigger;
    if ( trigger == 0 ) trigger = max;
    assert( max > 0 && trigger > 0 );

    cooldown = p -> get_cooldown( "rune" );
    cooldown -> duration = 120.0;
    trigger_gcd = 0;
    harmful = false;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();

    if ( sim -> log ) log_t::output( sim, "%s uses Mana Gem", p -> name() );

    p -> procs_mana_gem -> occur();
    used++;

    double gain = sim -> rng -> range( min, max );

    p -> resource_gain( RESOURCE_MANA, gain, p -> gains_mana_gem );

    update_ready();
  }

  virtual bool ready()
  {
    if ( cooldown -> remains() > 0 )
      return false;

    if ( used >= charges )
      return false;

    return( ( player -> resource_max    [ RESOURCE_MANA ] -
              player -> resource_current[ RESOURCE_MANA ] ) > trigger );
  }

  virtual void reset()
  {
    action_t::reset();
    used = 0;
  }
};

// Mirror Image Spell =======================================================

struct mirror_image_t : public mage_spell_t
{
  mirror_image_t( mage_t* player, const std::string& options_str ) :
      mage_spell_t( "mirror_image", 55342, player )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    consume_resource();
    update_ready();
    p -> summon_pet( "mirror_image" );
    p -> buffs_tier10_4pc -> trigger();
  }
};

// Molten Armor Spell =======================================================

struct molten_armor_t : public mage_spell_t
{
  molten_armor_t( mage_t* player, const std::string& options_str ) :
      mage_spell_t( "molten_armor", 30482, player )
  {
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if ( sim -> log )
      log_t::output( sim, "%s performs %s", p -> name(), name() );

    p -> buffs_mage_armor -> expire();
    p -> buffs_molten_armor -> trigger();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();
    return ! p -> buffs_molten_armor -> check();
  }
};

// Presence of Mind Spell ===================================================

struct presence_of_mind_t : public mage_spell_t
{
  action_t* fast_action;

  presence_of_mind_t( mage_t* player, const std::string& options_str ) :
      mage_spell_t( "presence_of_mind", 12043, player )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.presence_of_mind -> rank() );

    cooldown -> duration *= 1.0 + p -> talents.arcane_flows -> effect_base_value( 1 ) / 100.0;

    if ( options_str.empty() )
    {
      sim -> errorf( "Player %s: The presence_of_mind action must be coupled with a second action.", p -> name() );
      sim -> cancel();
    }

    std::string spell_name    = options_str;
    std::string spell_options = "";

    std::string::size_type cut_pt = spell_name.find_first_of( "," );

    if ( cut_pt != spell_name.npos )
    {
      spell_options = spell_name.substr( cut_pt + 1 );
      spell_name    = spell_name.substr( 0, cut_pt );
    }

    fast_action = p -> create_action( spell_name.c_str(), spell_options.c_str() );
    fast_action -> base_execute_time = 0;
    fast_action -> background = true;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    mage_t* p = player -> cast_mage();
    p -> aura_gain( "Presence of Mind" );
    p -> last_foreground_action = fast_action;
    p -> buffs_arcane_potency -> trigger();
    fast_action -> execute();
    p -> aura_loss( "Presence of Mind" );
    update_ready();
  }

  virtual bool ready()
  {
    return( mage_spell_t::ready() && fast_action -> ready() );
  }
};

// Pyroblast Spell ==========================================================

struct pyroblast_t : public mage_spell_t
{
  pyroblast_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "pyroblast", 92315, player )
  {
    mage_t* p = player -> cast_mage();
    check_spec( TREE_FIRE );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_crit      = true;
    tick_may_crit = true;
    base_crit    += p -> glyphs.pyroblast * 0.05;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();

    mage_spell_t::execute();

    if ( p -> buffs_hot_streak -> check() )
    {
      p -> buffs_hot_streak -> expire();
      p -> buffs_tier10_2pc -> trigger();
    }

    // When performing Hot Streak Pyroblasts, do not wait for DoT to complete.
    if ( p -> buffs_hot_streak -> check() )
      dot -> ready = 0;
  }

  virtual double execute_time() SC_CONST
  {
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_hot_streak -> up() )
      return 0;

    return mage_spell_t::execute_time();
  }
};

// Scorch Spell =============================================================

struct scorch_t : public mage_spell_t
{
  int debuff;

  scorch_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "scorch", 2948, player ), debuff( 0 )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "debuff",    OPT_BOOL, &debuff     },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_crit = true;
    base_cost *= 1.0 + p -> talents.improved_scorch -> effect_base_value( 1 );

    if ( debuff )
      check_talent( p -> talents.critical_mass -> rank() );
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    if ( result_is_hit() ) 
    {
      sim -> target -> debuffs.critical_mass -> trigger( 1, 1.0, p -> talents.critical_mass -> proc_chance() );
    }
    trigger_hot_streak( this );
  }

  virtual bool ready()
  {
    if ( ! mage_spell_t::ready() )
      return false;

    if ( debuff )
    {
      target_t* t = sim -> target;

      if ( t -> debuffs.improved_shadow_bolt -> check() )
        return false;

      if ( t -> debuffs.critical_mass -> remains_gt( 6.0 ) )
        return false;
    }

    return true;
  }
};

// Slow Spell ================================================================

struct slow_t : public mage_spell_t
{
  slow_t( mage_t* player, const std::string& options_str ) :
      mage_spell_t( "slow", 31589, player )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.slow -> rank() );
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    sim -> target -> debuffs.slow -> trigger();
  }

  virtual bool ready()
  {
    if ( sim -> target -> debuffs.snared() )
      return false;

    return mage_spell_t::ready();
  }
};

// Time Warp Spell ==========================================================

struct time_warp_t : public mage_spell_t
{
  time_warp_t( mage_t* player, const std::string& options_str ) :
    mage_spell_t( "time_warp", 80353, player )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    
    harmful = false;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    
    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> sleeping ) 
        continue;

      p -> buffs.bloodlust -> trigger(); // Bloodlust and Timewarp are the same
    }
  }

  virtual bool ready()
  {
    if ( player -> buffs.bloodlust -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Water Elemental Spell ================================================

struct water_elemental_spell_t : public mage_spell_t
{
  water_elemental_spell_t( mage_t* player, const std::string& options_str ) :
      mage_spell_t( "water_elemental", 31687, player )
  {
    check_spec( TREE_FROST );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();

    p -> summon_pet( "water_elemental", 45.0 );
  }

  virtual bool ready()
  {
    if ( ! mage_spell_t::ready() )
      return false;

    return player -> cast_mage() -> active_water_elemental == 0;
  }
};

// Choose Rotation ==========================================================

struct choose_rotation_t : public action_t
{
  double last_time;

  choose_rotation_t( player_t* p, const std::string& options_str ) :
      action_t( ACTION_USE, "choose_rotation", p ), last_time( 0 )
  {
    cooldown -> duration = 10;

    option_t options[] =
    {
      { "cooldown", OPT_FLT, &( cooldown -> duration ) },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( cooldown -> duration < 1.0 )
    {
      sim -> errorf( "Player %s: choose_rotation cannot have cooldown -> duration less than 1.0sec", p -> name() );
      cooldown -> duration = 1.0;
    }

    trigger_gcd = 0;
    harmful = false;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();

    if ( sim -> log ) log_t::output( sim, "%s Considers Spell Rotation", p -> name() );

    // It is important to smooth out the regen rate by averaging out the returns from Evocation and Mana Gems.
    // In order for this to work, the resource_gain() method must filter out these sources when
    // tracking "rotation.mana_gain".

    double regen_rate = p -> rotation.mana_gain / sim -> current_time;

    // Evocation
    regen_rate += p -> resource_max[ RESOURCE_MANA ] * 0.60 / ( 240.0 - p -> talents.arcane_flows -> rank() * 60.0 );

    // Mana Gem
    regen_rate += 3400 / 120.0; // FIXME: Possible to derive mana gem value?

    if ( p -> rotation.current == ROTATION_DPS )
    {
      p -> rotation.dps_time += ( sim -> current_time - last_time );

      double consumption_rate = ( p -> rotation.dps_mana_loss / p -> rotation.dps_time ) - regen_rate;

      if ( consumption_rate > 0 )
      {
        double oom_time = p -> resource_current[ RESOURCE_MANA ] / consumption_rate;

        if ( oom_time < sim -> target -> time_to_die() )
        {
          if ( sim -> log ) log_t::output( sim, "%s switches to DPM spell rotation", p -> name() );

          p -> rotation.current = ROTATION_DPM;
        }
      }
    }
    else if ( p -> rotation.current == ROTATION_DPM )
    {
      p -> rotation.dpm_time += ( sim -> current_time - last_time );

      double consumption_rate = ( p -> rotation.dpm_mana_loss / p -> rotation.dpm_time ) - regen_rate;

      if ( consumption_rate > 0 )
      {
        double oom_time = p -> resource_current[ RESOURCE_MANA ] / consumption_rate;

        if ( oom_time > sim -> target -> time_to_die() )
        {
          if ( sim -> log ) log_t::output( sim, "%s switches to DPS spell rotation", p -> name() );

          p -> rotation.current = ROTATION_DPS;
        }
      }
      else
      {
        if ( sim -> log ) log_t::output( sim, "%s switches to DPS rotation (negative consumption)", p -> name() );

        p -> rotation.current = ROTATION_DPS;
      }
    }
    last_time = sim -> current_time;

    update_ready();
  }

  virtual bool ready()
  {
    if ( cooldown -> remains() > 0 )
      return false;

    if ( sim -> current_time < cooldown -> duration )
      return false;

    return true;
  }

  virtual void reset()
  {
    action_t::reset();
    last_time=0;
  }
};

} // ANONYMOUS NAMESPACE ===================================================

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
  if ( name == "blast_wave"        ) return new              blast_wave_t( this, options_str );
  if ( name == "choose_rotation"   ) return new         choose_rotation_t( this, options_str );
  if ( name == "cold_snap"         ) return new               cold_snap_t( this, options_str );
  if ( name == "cone_of_cold"      ) return new            cone_of_cold_t( this, options_str );
  if ( name == "combustion"        ) return new              combustion_t( this, options_str );
  if ( name == "counterspell"      ) return new            counterspell_t( this, options_str );
  if ( name == "deep_freeze"       ) return new             deep_freeze_t( this, options_str );
  if ( name == "dragons_breath"    ) return new          dragons_breath_t( this, options_str );
  if ( name == "evocation"         ) return new               evocation_t( this, options_str );
  if ( name == "fire_blast"        ) return new              fire_blast_t( this, options_str );
  if ( name == "fireball"          ) return new                fireball_t( this, options_str );
  if ( name == "flame_orb"         ) return new               flame_orb_t( this, options_str );
  if ( name == "focus_magic"       ) return new             focus_magic_t( this, options_str );
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
  if ( name == "scorch"            ) return new                  scorch_t( this, options_str );
  if ( name == "slow"              ) return new                    slow_t( this, options_str );
  if ( name == "time_warp"         ) return new               time_warp_t( this, options_str );
  if ( name == "water_elemental"   ) return new   water_elemental_spell_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// mage_t::create_pet ======================================================

pet_t* mage_t::create_pet( const std::string& pet_name )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "mirror_image"    ) return new mirror_image_pet_t   ( sim, this );
  if ( pet_name == "water_elemental" ) return new water_elemental_pet_t( sim, this );

  return 0;
}

// mage_t::create_pets =====================================================

void mage_t::create_pets()
{
  create_pet( "mirror_image"    );
  create_pet( "water_elemental" );  
}

// mage_t::init_glyphs ====================================================

void mage_t::init_glyphs()
{
  memset( ( void* ) &glyphs, 0x0, sizeof( glyphs_t ) );

  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );

  for ( int i=0; i < num_glyphs; i++ )
  {
    std::string& n = glyph_names[ i ];

    if      ( n == "arcane_barrage"    ) glyphs.arcane_barrage = 1;
    else if ( n == "arcane_blast"      ) glyphs.arcane_blast = 1;
    else if ( n == "arcane_brilliance" ) glyphs.arcane_brilliance = 1;
    else if ( n == "arcane_missiles"   ) glyphs.arcane_missiles = 1;
    else if ( n == "cone_of_cold"      ) glyphs.cone_of_cold = 1;
    else if ( n == "dragons_breath"    ) glyphs.dragons_breath = 1;
    else if ( n == "fireball"          ) glyphs.fireball = 1;
    else if ( n == "frostbolt"         ) glyphs.frostbolt = 1;
    else if ( n == "frostfire"         ) glyphs.frostfire = 1;
    else if ( n == "ice_lance"         ) glyphs.ice_lance = 1;
    else if ( n == "living_bomb"       ) glyphs.living_bomb = 1;
    else if ( n == "mage_armor"        ) glyphs.mage_armor = 1;
    else if ( n == "mirror_image"      ) glyphs.mirror_image = 1;
    else if ( n == "molten_armor"      ) glyphs.molten_armor = 1;
    else if ( n == "pyroblast"         ) glyphs.pyroblast = 1;
    // To prevent warnings....
    else if ( n == "arcane_power" ) ;
    else if ( n == "armors"       ) ;
    else if ( n == "blast_wave"   ) ;
    else if ( n == "blink"        ) ;
    else if ( n == "conjuring"    ) ;
    else if ( n == "deep_freeze"  ) ;
    else if ( n == "evocation"    ) ;
    else if ( n == "frost_nova"   ) ;
    else if ( n == "ice_barrier"  ) ;
    else if ( n == "ice_block"    ) ;
    else if ( n == "icy_veins"    ) ;
    else if ( n == "invisibility" ) ;
    else if ( n == "mana_shield"  ) ;
    else if ( n == "monkey"       ) ;
    else if ( n == "penguin"      ) ;
    else if ( n == "polymorph"    ) ;
    else if ( n == "slow"         ) ;
    else if ( n == "slow_fall"    ) ;
    else if ( ! sim -> parent ) 
    {
      sim -> errorf( "Player %s has unrecognized glyph %s\n", name(), n.c_str() );
    }
  }
}

// mage_t::init_talents ===================================================

void mage_t::init_talents()
{
  // Arcane
  talents.arcane_concentration        = new talent_t( this, "arcane_concentration", "Arcane Concentration" );
  talents.arcane_flows                = new talent_t( this, "arcane_flows", "Arcane Flows" );
  talents.arcane_potency              = new talent_t( this, "arcane_potency", "Arcane Potency" );
  talents.arcane_power                = new talent_t( this, "arcane_power", "Arcane Power" );
  talents.arcane_tactics              = new talent_t( this, "arcane_tactics", "Arcane Tactics" );
  talents.focus_magic                 = new talent_t( this, "focus_magic", "Focus Magic" );
  talents.improved_arcane_explosion   = new talent_t( this, "improved_arcane_explosion", "Improved Arcane Explosion" );
  talents.improved_arcane_missiles    = new talent_t( this, "improved_arcane_missiles", "Improved Arcane Missiles" );
  talents.improved_blink              = new talent_t( this, "improved_blink", "Improved Blink" );
  talents.improved_counterspell       = new talent_t( this, "improved_counterspell", "Improved Counterspell" );
  talents.improved_mana_gem           = new talent_t( this, "improved_mana_gem", "Improved Mana Gem" );
  talents.improved_polymorph          = new talent_t( this, "improved_polymorph", "Improved Polymorph" );
  talents.incanters_absorption        = new talent_t( this, "incanters_absorption", "Incanter's Absorption" );
  talents.invocation                  = new talent_t( this, "invocation", "Invocation" );
  talents.missile_barrage             = new talent_t( this, "missile_barrage", "Missile Barrage" );
  talents.nether_vortex               = new talent_t( this, "nether_vortex", "Nether Vortex" );
  talents.netherwind_presence         = new talent_t( this, "netherwind_presence", "Netherwind Presence" );
  talents.presence_of_mind            = new talent_t( this, "presence_of_mind", "Presence of Mind" );
  talents.prismatic_cloak             = new talent_t( this, "prismatic_cloak", "Prismatic Cloak" );
  talents.slow                        = new talent_t( this, "slow", "Slow" );
  talents.torment_the_weak            = new talent_t( this, "torment_the_weak", "Torment the Weak" );

  // Fire
  talents.blast_wave                  = new talent_t( this, "blast_wave", "Blast Wave" );
  talents.blazing_speed               = new talent_t( this, "blazing_speed", "Blazing Speed" );
  talents.burning_soul                = new talent_t( this, "burning_soul", "Burning Soul" );
  talents.cauterize                   = new talent_t( this, "cauterize", "Cauterize" );
  talents.combustion                  = new talent_t( this, "combustion", "Combustion" );
  talents.critical_mass               = new talent_t( this, "critical_mass", "Critical Mass" );
  talents.dragons_breath              = new talent_t( this, "dragons_breath", "Dragon's Breath" );
  talents.fire_power                  = new talent_t( this, "fire_power", "Fire Power" );
  talents.firestarter                 = new talent_t( this, "firestarter", "Firestarter" );
  talents.hot_streak                  = new talent_t( this, "hot_streak", "Hot Streak" );
  talents.ignite                      = new talent_t( this, "ignite", "Ignite" );
  talents.impact                      = new talent_t( this, "impact", "Impact" );
  talents.improved_fire_blast         = new talent_t( this, "improved_fire_blast", "Improved Fire Blast" );
  talents.improved_flamestrike        = new talent_t( this, "improved_flamestrike", "Improved Flamestrike" );
  talents.improved_hot_streak         = new talent_t( this, "improved_hot_streak", "Improved Hot Streak" );
  talents.improved_scorch             = new talent_t( this, "improved_scorch", "Improved Scorch" );
  talents.living_bomb                 = new talent_t( this, "living_bomb", "Living Bomb" );
  talents.master_of_elements          = new talent_t( this, "master_of_elements", "Master of Elements" );
  talents.molten_fury                 = new talent_t( this, "molten_fury", "Molten Fury" );
  talents.molten_shields              = new talent_t( this, "molten_shields", "Molten Shields" );
  talents.pyromaniac                  = new talent_t( this, "pyromaniac", "Pyromaniac" );

  // Frost
  talents.brain_freeze                = new talent_t( this, "brain_freeze", "Brain Freeze" );
  talents.cold_snap                   = new talent_t( this, "cold_snap", "Cold Snap" );
  talents.deep_freeze                 = new talent_t( this, "deep_freeze", "Deep Freeze" );
  talents.early_frost                 = new talent_t( this, "early_frost", "Early Frost" );
  talents.enduring_winter             = new talent_t( this, "enduring_winter", "Enduring Winter" );
  talents.fingers_of_frost            = new talent_t( this, "fingers_of_frost", "Fingers of Frost" );
  talents.frostfire_orb               = new talent_t( this, "frostfire_orb", "Frostfire Orb" );
  talents.ice_barrier                 = new talent_t( this, "ice_barrier", "Ice Barrier" );
  talents.ice_floes                   = new talent_t( this, "ice_floes", "Ice Floes" );
  talents.ice_shards                  = new talent_t( this, "ice_shards", "Ice Shards" );
  talents.icy_veins                   = new talent_t( this, "icy_veins", "Icy Veins" );
  talents.improved_cone_of_cold       = new talent_t( this, "improved_cone_of_cold", "Improved Cone of Cold" );
  talents.improved_freeze             = new talent_t( this, "improved_freeze", "Improved Freeze" );
  talents.permafrost                  = new talent_t( this, "permafrost", "Permafrost" );
  talents.piercing_chill              = new talent_t( this, "piercing_chill", "Piercing Chill" );
  talents.piercing_ice                = new talent_t( this, "piercing_ice", "Piercing Ice" );
  talents.reactive_barrier            = new talent_t( this, "reactive_barrier", "Reactive Barrier" );
  talents.shatter                     = new talent_t( this, "shatter", "Shatter" );
  talents.shattered_barrier           = new talent_t( this, "shattered_barrier", "Shattered Barrier" );

  player_t::init_talents();
}

// mage_t::init_spells ====================================================

void mage_t::init_spells()
{
  player_t::init_spells();

  // Mastery
  mastery.flashburn                   = new passive_spell_t( this, "flashburn",  76595, MAGE_FIRE,   true );
  mastery.frostburn                   = new passive_spell_t( this, "frostburn",  76613, MAGE_FROST,  true );
  mastery.mana_adept                  = new passive_spell_t( this, "mana_adept", 76547, MAGE_ARCANE, true );

  // Passives
  passive_spells.arcane_specialization = new passive_spell_t( this, "arcane_specialization", "Arcane Specialization", MAGE_ARCANE );
  passive_spells.fire_specialization   = new passive_spell_t( this, "fire_specialization",   "Fire Specialization",   MAGE_FIRE );
  passive_spells.frost_specialization  = new passive_spell_t( this, "frost_specialization",  "Frost Specialization",  MAGE_FROST );
}

// mage_t::init_race ======================================================

void mage_t::init_race()
{
  race = util_t::parse_race_type( race_str );
  switch ( race )
  {
  case RACE_BLOOD_ELF:
  case RACE_DRAENEI:
  case RACE_DWARF:
  case RACE_GOBLIN:
  case RACE_GNOME:
  case RACE_HUMAN:
  case RACE_NIGHT_ELF:
  case RACE_ORC:
  case RACE_TROLL:
  case RACE_UNDEAD:
  case RACE_WORGEN:
    break;
  default:
    race = RACE_UNDEAD;
    race_str = util_t::race_type_string( race );
  }

  player_t::init_race();
}


// mage_t::init_base =======================================================

void mage_t::init_base()
{
  player_t::init_base();

  initial_spell_power_per_intellect = 1.0;

  base_attack_power = -10;
  initial_attack_power_per_strength = 1.0;

  health_per_stamina = 10;
  mana_per_intellect = 15;

  diminished_kfactor    = 0.009830;
  diminished_dodge_capi = 0.006650;
  diminished_parry_capi = 0.006650;
}

// mage_t::init_buffs ======================================================

void mage_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, player, name, max_stack, duration, cooldown, proc_chance, quiet )

  buffs_arcane_blast         = new buff_t( this, "arcane_blast",         4,  6.0 );
  buffs_arcane_power         = new buff_t( this, "arcane_power",         1, 15.0 );
  buffs_brain_freeze         = new buff_t( this, "brain_freeze",         1, 15.0, 2.0, talents.brain_freeze -> rank() * 0.05 );
  buffs_clearcasting         = new buff_t( this, "clearcasting",         1, 10.0, 0, talents.arcane_concentration -> rank() * 0.02 );
  buffs_combustion           = new buff_t( this, "combustion",           3 );
  buffs_fingers_of_frost     = new buff_t( this, "fingers_of_frost",     2,    0, 0, talents.fingers_of_frost -> rank() * 0.15/2 );
  buffs_focus_magic_feedback = new buff_t( this, "focus_magic_feedback", 1, 10.0 );
  buffs_hot_streak_crits     = new buff_t( this, "hot_streak_crits",     2,    0, 0, 1.0, true );
  buffs_hot_streak           = new buff_t( this, "hot_streak",           1, 10.0, 0, talents.improved_hot_streak -> rank() / 2.0 );
  buffs_icy_veins            = new buff_t( this, "icy_veins",            1, 20.0 );
  buffs_incanters_absorption = new buff_t( this, "incanters_absorption", 1, 10.0 );
  buffs_missile_barrage      = new buff_t( this, "missile_barrage",      1, 15.0, 0, talents.missile_barrage -> rank() * 0.04 );
  buffs_tier10_2pc           = new buff_t( this, "tier10_2pc",           1,  5.0, 0, set_bonus.tier10_2pc_caster() );
  buffs_tier10_4pc           = new buff_t( this, "tier10_4pc",           1, 30.0, 0, set_bonus.tier10_4pc_caster() );
  buffs_arcane_potency = new buff_t( this, "arcane_potency", 2);

  buffs_ghost_charge = new buff_t( this, "ghost_charge" );
  buffs_mage_armor   = new buff_t( this, "mage_armor" );
  buffs_molten_armor = new buff_t( this, "molten_armor" );
}

// mage_t::init_gains ======================================================

void mage_t::init_gains()
{
  player_t::init_gains();

  gains_clearcasting       = get_gain( "clearcasting"       );
  gains_evocation          = get_gain( "evocation"          );
  gains_mage_armor         = get_gain( "mage_armor"         );
  gains_mana_gem           = get_gain( "mana_gem"           );
  gains_master_of_elements = get_gain( "master_of_elements" );
}

// mage_t::init_procs ======================================================

void mage_t::init_procs()
{
  player_t::init_procs();

  procs_deferred_ignite = get_proc( "deferred_ignite" );
  procs_munched_ignite  = get_proc( "munched_ignite"  );
  procs_rolled_ignite   = get_proc( "rolled_ignite"   );
  procs_mana_gem        = get_proc( "mana_gem"        );
}

// mage_t::init_uptimes ====================================================

void mage_t::init_uptimes()
{
  player_t::init_uptimes();

  uptimes_arcane_blast[ 0 ]    = get_uptime( "arcane_blast_0"  );
  uptimes_arcane_blast[ 1 ]    = get_uptime( "arcane_blast_1"  );
  uptimes_arcane_blast[ 2 ]    = get_uptime( "arcane_blast_2"  );
  uptimes_arcane_blast[ 3 ]    = get_uptime( "arcane_blast_3"  );
  uptimes_arcane_blast[ 4 ]    = get_uptime( "arcane_blast_4"  );
  uptimes_dps_rotation         = get_uptime( "dps_rotation"    );
  uptimes_dpm_rotation         = get_uptime( "dpm_rotation"    );
  uptimes_water_elemental      = get_uptime( "water_elemental" );
}

// mage_t::init_rng ========================================================

void mage_t::init_rng()
{
  player_t::init_rng();

  rng_empowered_fire  = get_rng( "empowered_fire"  );
  rng_ghost_charge    = get_rng( "ghost_charge"    );
  rng_enduring_winter = get_rng( "enduring_winter" );
}

// mage_t::init_actions ======================================================

void mage_t::init_actions()
{
  if ( action_list_str.empty() )
  {
    action_list_str = "flask,type=frost_wyrm/food,type=fish_feast";
    action_list_str += "/arcane_brilliance";
    if ( talents.focus_magic ) action_list_str += "/focus_magic";
    action_list_str += "/speed_potion";
    action_list_str += "/snapshot_stats";
    action_list_str += "/counterspell";
    if ( talents.critical_mass -> rank() ) action_list_str += "/scorch,debuff=1";
    int num_items = ( int ) items.size();
    for ( int i=0; i < num_items; i++ )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
      }
    }
    if ( race == RACE_TROLL )
    {
      action_list_str += "/berserking";
    }
    else if ( race == RACE_BLOOD_ELF )
    {
      action_list_str += "/arcane_torrent";
    }
    action_list_str += "/mirror_image";
    if ( primary_tree() == TREE_ARCANE )
    {
      if ( talents.arcane_power -> rank() ) action_list_str += "/arcane_power";
      action_list_str += "/mana_gem";
      action_list_str += "/evocation,if=!buff.arcane_blast.up";
      action_list_str += "/choose_rotation";
      action_list_str += "/arcane_missiles,if=buff.missile_barrage.react";
      if ( talents.presence_of_mind -> rank() )
      {
        action_list_str += "/presence_of_mind,arcane_blast,if=dps";
      }
      action_list_str += "/arcane_blast,if=dpm&buff.arcane_blast.stack<3";
      action_list_str += "/arcane_blast,if=dps";
      action_list_str += "/arcane_missiles";
      if ( primary_tree() == TREE_ARCANE ) action_list_str += "/arcane_barrage,moving=1"; // when moving
      action_list_str += "/fire_blast,moving=1"; // when moving
    }
    else if ( primary_tree() == TREE_FIRE )
    {
      if ( talents.combustion -> rank()   ) action_list_str += "/combustion";
      action_list_str += "/mana_gem";
      if ( talents.hot_streak -> rank()  ) action_list_str += "/pyroblast,if=buff.hot_streak.react";
      if ( talents.living_bomb -> rank() ) action_list_str += "/living_bomb";
      if ( talents.piercing_ice -> rank() && talents.ice_shards -> rank() )
      {
        action_list_str += "/frostfire_bolt";
      }
      else
      {
        action_list_str += "/fireball";
      }
      action_list_str += "/evocation";
      action_list_str += "/fire_blast,moving=1"; // when moving
      action_list_str += "/ice_lance,moving=1";  // when moving
    }
    else if ( primary_tree() == TREE_FROST )
    {
      if ( talents.icy_veins -> rank()    ) action_list_str += "/icy_veins";
      action_list_str += "/water_elemental";
      action_list_str += "/mana_gem";
      action_list_str += "/deep_freeze";
      action_list_str += "/frostbolt,frozen=1";
      if ( talents.cold_snap -> rank() ) action_list_str += "/cold_snap,if=cooldown.deep_freeze.remains>15";
      if ( talents.brain_freeze -> rank() )
      {
	      action_list_str += "/frostfire_bolt,if=buff.brain_freeze.react";
      }
      action_list_str += "/frostbolt";
      action_list_str += "/evocation";
      action_list_str += "/ice_lance,moving=1,frozen=1"; // when moving
      action_list_str += "/fire_blast,moving=1";         // when moving
      action_list_str += "/ice_lance,moving=1";          // when moving
    }
    else action_list_str = "/arcane_missiles/evocation";

    action_list_default = 1;
  }

  player_t::init_actions();
}

// mage_t::composite_spell_power ===========================================

double mage_t::composite_spell_power( const school_type school ) SC_CONST
{
  double sp = player_t::composite_spell_power( school );

  if ( talents.incanters_absorption -> rank() )
  {
    sp += buffs_incanters_absorption -> value();
  }

  return sp;
}

// mage_t::composite_spell_crit ============================================

double mage_t::composite_spell_crit() SC_CONST
{
  double c = player_t::composite_spell_crit();

  if ( buffs_molten_armor -> up() )
  {
    c += 0.03 + glyphs.molten_armor * 0.02;
  }

  if ( buffs_focus_magic_feedback -> up() ) c += 0.03;

  return c;
}

// mage_t::matching_gear_multiplier =====================================

double mage_t::matching_gear_multiplier( const attribute_type attr ) SC_CONST
{
  if ( attr == ATTR_INTELLECT )
    return 0.05;

  return 0.0;
}

// mage_t::combat_begin ====================================================

void mage_t::combat_begin()
{
  player_t::combat_begin();

  if ( sim -> auras.arcane_tactics -> current_value < talents.arcane_tactics -> rank() )
  {
    sim -> auras.arcane_tactics -> trigger( 1, talents.arcane_tactics -> rank() );
  }

  if ( ! armor_type_str.empty() )
  {
    if ( sim -> log ) log_t::output( sim, "%s equips %s armor", name(), armor_type_str.c_str() );

    if ( armor_type_str == "mage" )
    {
      buffs_mage_armor -> trigger();
    }
    else if ( armor_type_str == "molten" )
    {
      buffs_molten_armor -> trigger();
    }
    else
    {
      sim -> errorf( "Unknown armor type '%s' for player %s\n", armor_type_str.c_str(), name() );
      armor_type_str.clear();
    }
  }
}

// mage_t::reset ===========================================================

void mage_t::reset()
{
  player_t::reset();
  active_water_elemental = 0;
  ignite_delay_event = 0;
  rotation.reset();
  _cooldowns.reset();
}

// mage_t::regen  ==========================================================

void mage_t::regen( double periodicity )
{
  mana_regen_while_casting  = 0;

  if ( buffs_mage_armor -> up() )
  {
    double gain_amount = resource_max[ RESOURCE_MANA ] * 0.03 / 5.0;
    gain_amount *= 1.0 + ( glyphs.mage_armor ? 0.20 : 0.00 );

    resource_gain( RESOURCE_MANA, gain_amount, gains_mage_armor );
  }

  player_t::regen( periodicity );

  uptimes_water_elemental -> update( active_water_elemental != 0 );

  for ( int i=0; i < 5; i++ )
  {
    uptimes_arcane_blast[ i ] -> update( i == buffs_arcane_blast -> stack() );
  }
}

// mage_t::resource_gain ===================================================

double mage_t::resource_gain( int       resource,
                              double    amount,
                              gain_t*   source,
                              action_t* action )
{
  double actual_amount = player_t::resource_gain( resource, amount, source, action );

  if ( resource == RESOURCE_MANA )
  {
    if ( source != gains_evocation &&
         source != gains_mana_gem )
    {
      rotation.mana_gain += actual_amount;
    }
  }

  return actual_amount;
}

// mage_t::resource_loss ===================================================

double mage_t::resource_loss( int       resource,
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

// mage_t::create_expression ===============================================

action_expr_t* mage_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "dps" )
  {
    struct dps_rotation_expr_t : public action_expr_t
    {
      dps_rotation_expr_t( action_t* a ) : action_expr_t( a, "dps_rotation" ) { result_type = TOK_NUM; }
      virtual int evaluate() { result_num = ( action -> player -> cast_mage() -> rotation.current == ROTATION_DPS ) ? 1.0 : 0.0; return TOK_NUM; }
    };
    return new dps_rotation_expr_t( a );
  }
  else if ( name_str == "dpm" )
  {
    struct dpm_rotation_expr_t : public action_expr_t
    {
      dpm_rotation_expr_t( action_t* a ) : action_expr_t( a, "dpm_rotation" ) { result_type = TOK_NUM; }
      virtual int evaluate() { result_num = ( action -> player -> cast_mage() -> rotation.current == ROTATION_DPM ) ? 1.0 : 0.0; return TOK_NUM; }
    };
    return new dpm_rotation_expr_t( a );
  }

  return player_t::create_expression( a, name_str );
}

// mage_t::get_talent_trees ================================================

std::vector<talent_translation_t>& mage_t::get_talent_list()
{

  talent_list.clear();
  return talent_list;
}

// mage_t::get_options ====================================================

std::vector<option_t>& mage_t::get_options()
{
  if ( options.empty() )
  {
    player_t::get_options();

    option_t mage_options[] =
    {

      // @option_doc loc=player/mage/misc title="Misc"
      { "armor_type",                OPT_STRING, &( armor_type_str                   ) },
      { "focus_magic_target",        OPT_STRING, &( focus_magic_target_str           ) },
      { "ghost_charge_pct",          OPT_FLT,    &( ghost_charge_pct                 ) },
      { "fof_on_cast",               OPT_BOOL,   &( fof_on_cast                      ) },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, mage_options );
  }

  return options;
}

// mage_t::create_profile ===================================================

bool mage_t::create_profile( std::string& profile_str, int save_type )
{
  player_t::create_profile( profile_str, save_type );

  if ( save_type == SAVE_ALL || save_type == SAVE_ACTIONS )
  {
    if ( ! armor_type_str.empty() ) profile_str += "armor_type=" + armor_type_str + "\n";
  }

  if ( save_type == SAVE_ALL )
  {
    if ( ! focus_magic_target_str.empty() ) profile_str += "focus_magic_target=" + focus_magic_target_str + "\n";
  }

  return true;
}

// mage_t::decode_set =======================================================

int mage_t::decode_set( item_t& item )
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

  if ( strstr( s, "frostfire"   ) ) return SET_T7_CASTER;
  if ( strstr( s, "kirin_tor"   ) ) return SET_T8_CASTER;
  if ( strstr( s, "sunstriders" ) ) return SET_T9_CASTER;
  if ( strstr( s, "khadgars"    ) ) return SET_T9_CASTER;
  if ( strstr( s, "bloodmage"   ) ) return SET_T10_CASTER;
  if ( strstr( s, "firelord"    ) ) return SET_T11_CASTER;

  return SET_NONE;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_mage  ===================================================

player_t* player_t::create_mage( sim_t* sim, const std::string& name, race_type r )
{
  sim -> errorf( "Mage Module isn't avaiable at the moment." );

  return new mage_t( sim, name, r );
  return NULL;
}

// player_t::mage_init ======================================================

void player_t::mage_init( sim_t* sim )
{
  sim -> auras.arcane_tactics = new aura_t( sim, "arcane_tactics", 1, 0.0 );

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> buffs.arcane_brilliance = new stat_buff_t( p, "arcane_brilliance", STAT_MANA, p -> player_data.effect_min( 79058, p -> level, E_APPLY_AURA, A_MOD_INCREASE_ENERGY ), !p -> is_pet() );
    p -> buffs.focus_magic       = new      buff_t( p, "focus_magic", 1 );
  }

  target_t* t = sim -> target;
  t -> debuffs.critical_mass = new debuff_t( sim, "critical_mass", 1, 30.0 );
  t -> debuffs.slow          = new debuff_t( sim, "slow",          1, 15.0 );
}

// player_t::mage_combat_begin ==============================================

void player_t::mage_combat_begin( sim_t* sim )
{
  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> ooc_buffs() )
    {
      if ( sim -> overrides.arcane_brilliance ) p -> buffs.arcane_brilliance -> override( 1, 60.0 );
      if ( sim -> overrides.focus_magic       ) p -> buffs.focus_magic       -> override();
    }
  }

  target_t* t = sim -> target;
  if ( sim -> overrides.critical_mass ) t -> debuffs.critical_mass -> override( 1 );

  if ( sim -> overrides.arcane_tactics ) sim -> auras.arcane_tactics -> override( 1, 3 );
}

