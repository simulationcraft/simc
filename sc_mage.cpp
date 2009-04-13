// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Mage
// ==========================================================================

enum { ROTATION_NONE=0, ROTATION_DPS, ROTATION_DPM, ROTATION_MAX };

struct mage_t : public player_t
{
  // Active
  spell_t* active_ignite;
  pet_t*   active_water_elemental;

  // Buffs
  struct _buffs_t
  {
    int    arcane_blast;
    int    arcane_potency;
    int    arcane_power;
    double brain_freeze;
    double clearcasting;
    int    combustion;
    int    combustion_crits;
    int    fingers_of_frost;
    int    hot_streak;
    double hot_streak_pyroblast;
    int    icy_veins;
    int    mage_armor;
    double missile_barrage;
    int    molten_armor;
    int    shatter_combo;

    void reset() { memset( (void*) this, 0x00, sizeof( _buffs_t ) ); }
    _buffs_t() { reset(); }
  };
  _buffs_t _buffs;

  // Expirations
  struct _expirations_t
  {
    event_t* arcane_blast;
    event_t* hot_streak;
    event_t* missile_barrage;
    event_t* brain_freeze;

    void reset() { memset( (void*) this, 0x00, sizeof( _expirations_t ) ); }
    _expirations_t() { reset(); }
  };
  _expirations_t _expirations;

  // Gains
  gain_t* gains_clearcasting;
  gain_t* gains_evocation;
  gain_t* gains_mana_gem;
  gain_t* gains_master_of_elements;

  // Procs
  proc_t* procs_clearcasting;
  proc_t* procs_deferred_ignite;
  proc_t* procs_hot_streak_pyroblast;

  // Up-Times
  uptime_t* uptimes_arcane_blast[ 4 ];
  uptime_t* uptimes_arcane_power;
  uptime_t* uptimes_dps_rotation;
  uptime_t* uptimes_dpm_rotation;
  uptime_t* uptimes_fingers_of_frost;
  uptime_t* uptimes_focus_magic_feedback;
  uptime_t* uptimes_icy_veins;
  uptime_t* uptimes_missile_barrage;
  uptime_t* uptimes_water_elemental;

  // Options
  std::string focus_magic_target_str;
  std::string armor_type_str;

  // Rotation (DPS vs DPM)
  struct rotation_t
  {
    int    current;
    double mana_gain;
    double dps_mana_loss;
    double dpm_mana_loss;
    double dps_time;
    double dpm_time;

    void reset() { memset( (void*) this, 0x00, sizeof( rotation_t ) ); current = ROTATION_DPS; }
    rotation_t() { reset(); }
  };
  rotation_t rotation;

  struct talents_t
  {
    int  arcane_barrage;
    int  arcane_concentration;
    int  arcane_empowerment;
    int  arcane_flows;
    int  arcane_focus;
    int  arcane_impact;
    int  arcane_instability;
    int  arcane_meditation;
    int  arcane_mind;
    int  arcane_potency;
    int  arcane_power;
    int  arcane_subtlety;
    int  arctic_winds;
    int  brain_freeze;
    int  burning_soul;
    int  burnout;
    int  chilled_to_the_bone;
    int  cold_as_ice;
    int  cold_snap;
    int  combustion;
    int  critical_mass;
    int  deep_freeze;
    int  empowered_arcane_missiles;
    int  empowered_fire;
    int  empowered_frost_bolt;
    int  fingers_of_frost;
    int  fire_power;
    int  focus_magic;
    int  frost_channeling;
    int  frostbite;
    int  hot_streak;
    int  ice_floes;
    int  ice_shards;
    int  icy_veins;
    int  ignite;
    int  improved_fire_ball;
    int  improved_fire_blast;
    int  improved_frost_bolt;
    int  improved_scorch;
    int  improved_water_elemental;
    int  incineration;
    int  living_bomb;
    int  master_of_elements;
    int  mind_mastery;
    int  missile_barrage;
    int  molten_fury;
    int  netherwind_presence;
    int  piercing_ice;
    int  playing_with_fire;
    int  precision;
    int  presence_of_mind;
    int  pyroblast;
    int  pyromaniac;
    int  shatter;
    int  slow;
    int  spell_impact;
    int  spell_power;
    int  student_of_the_mind;
    int  summon_water_elemental;
    int  torment_the_weak;
    int  winters_chill;
    int  winters_grasp;
    int  world_in_flames;
    
    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int arcane_barrage;
    int arcane_blast;
    int arcane_missiles;
    int arcane_power;
    int fire_ball;
    int frost_bolt;
    int ice_lance;
    int improved_scorch;
    int living_bomb;
    int mage_armor;
    int mana_gem;
    int mirror_image;
    int molten_armor;
    int water_elemental;
    int frostfire;
    glyphs_t() { memset( (void*) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  mage_t( sim_t* sim, std::string& name ) : player_t( sim, MAGE, name )
  {
    // Active
    active_ignite          = 0;
    active_water_elemental = 0;

    // Gains
    gains_clearcasting       = get_gain( "clearcasting" );
    gains_evocation          = get_gain( "evocation" );
    gains_mana_gem           = get_gain( "mana_gem" );
    gains_master_of_elements = get_gain( "master_of_elements" );

    // Procs
    procs_clearcasting         = get_proc( "clearcasting" );
    procs_deferred_ignite      = get_proc( "deferred_ignite" );
    procs_hot_streak_pyroblast = get_proc( "hot_streak_pyroblast" );

    // Up-Times
    uptimes_arcane_blast[ 0 ]    = get_uptime( "arcane_blast_0" );
    uptimes_arcane_blast[ 1 ]    = get_uptime( "arcane_blast_1" );
    uptimes_arcane_blast[ 2 ]    = get_uptime( "arcane_blast_2" );
    uptimes_arcane_blast[ 3 ]    = get_uptime( "arcane_blast_3" );
    uptimes_arcane_power         = get_uptime( "arcane_power" );
    uptimes_dps_rotation         = get_uptime( "dps_rotation" );
    uptimes_dpm_rotation         = get_uptime( "dpm_rotation" );
    uptimes_fingers_of_frost     = get_uptime( "fingers_of_frost" );
    uptimes_focus_magic_feedback = get_uptime( "focus_magic_feedback" );
    uptimes_icy_veins            = get_uptime( "icy_veins" );
    uptimes_missile_barrage      = get_uptime( "missile_barrage" );
    uptimes_water_elemental      = get_uptime( "water_elemental" );
  }

  // Character Definition
  virtual void      init_base();
  virtual void      combat_begin();
  virtual void      reset();
  virtual bool      get_talent_trees( std::vector<int*>& arcane, std::vector<int*>& fire, std::vector<int*>& frost );
  virtual bool      parse_talents_mmo( const std::string& talent_string );
  virtual bool      parse_option ( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual int       primary_resource() { return RESOURCE_MANA; }

  // Event Tracking
  virtual void   regen( double periodicity );
  virtual double resource_gain( int resource, double amount, gain_t* source );
  virtual double resource_loss( int resource, double amount );
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// stack_winters_chill =====================================================

static void stack_winters_chill( spell_t* s,
                                 double   chance )
{
  if( s -> school != SCHOOL_FROST &&
      s -> school != SCHOOL_FROSTFIRE ) return;

  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Winters Chill Expiration";
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      if( sim -> log ) report_t::log( sim, "Target %s loses Winters Chill", sim -> target -> name() );
      sim -> target -> debuffs.winters_chill = 0;
      sim -> target -> expirations.winters_chill = 0;
    }
  };

  if( s -> sim -> roll( chance ) )
  {
    target_t* t = s -> sim -> target;

    if( t -> debuffs.winters_chill < 5 ) 
    {
      t -> debuffs.winters_chill += 1;

      if( s -> sim -> log ) 
        report_t::log( s -> sim, "Target %s gains Winters Chill %d", 
                       t -> name(), t -> debuffs.winters_chill );
    }

    event_t*& e = t -> expirations.winters_chill;
    
    if( e )
    {
      e -> reschedule( 15.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim );
    }
  }
}

// ==========================================================================
// Mage Spell
// ==========================================================================

struct mage_spell_t : public spell_t
{
  int dps_rotation;
  int dpm_rotation;
  int arcane_power;
  int icy_veins;

  mage_spell_t( const char* n, player_t* player, int s, int t ) : 
    spell_t( n, player, RESOURCE_MANA, s, t ),
    dps_rotation(0),
    dpm_rotation(0),
    arcane_power(0),
    icy_veins(0)
  {
    mage_t* p = player -> cast_mage();
    base_hit += p -> talents.precision * 0.01;
  }

  virtual void   parse_options( option_t*, const std::string& );
  virtual bool   ready();
  virtual double cost();
  virtual double haste();
  virtual void   execute();
  virtual void   consume_resource();
  virtual void   player_buff();
};

// ==========================================================================
// Pet Water Elemental
// ==========================================================================

struct water_elemental_pet_t : public pet_t
{
  struct water_bolt_t : public spell_t
  {
    water_bolt_t( player_t* player ):
      spell_t( "water_bolt", player, RESOURCE_MANA, SCHOOL_FROST, TREE_FROST ) 
    {
      base_cost          = 0;
      base_execute_time  = 2.5;
      base_dd_min        = 256 + ( player -> level - 50 ) * 11.5;
      base_dd_max        = 328 + ( player -> level - 50 ) * 11.5;
      direct_power_mod   = ( 5.0 / 6.0 );
      may_crit           = true;
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
    // Stolen from Priest's Shadowfiend
    attribute_base[ ATTR_STRENGTH  ] = 145;
    attribute_base[ ATTR_AGILITY   ] =  38;
    attribute_base[ ATTR_STAMINA   ] = 190;
    attribute_base[ ATTR_INTELLECT ] = 133;

    health_per_stamina = 7.5;
    mana_per_intellect = 5;
  }
  virtual void summon()
  {
    pet_t::summon();

    mage_t* o = cast_pet() -> owner -> cast_mage();

    o -> active_water_elemental = this;

    if( o -> talents.improved_water_elemental )
    {
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
        if( p -> buffs.water_elemental == 0 ) p -> aura_gain( "Water Elemental Regen" );
        p -> buffs.water_elemental++;
      }
    }

    schedule_ready();
  }
  virtual void dismiss()
  {
    pet_t::dismiss();

    mage_t* o = cast_pet() -> owner -> cast_mage();

    if( o -> talents.improved_water_elemental )
    {
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
        p -> buffs.water_elemental--;
        if( p -> buffs.water_elemental == 0 ) p -> aura_loss( "Water Elemental Regen" );
      }
    }

    o -> active_water_elemental = 0;
  }
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if( name == "water_bolt" ) return new water_bolt_t( this );

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

  struct mirror_blast_t : public spell_t
  {
    action_t* next_in_sequence;

    mirror_blast_t( mirror_image_pet_t* mirror_image, action_t* nis ):
      spell_t( "mirror_blast", mirror_image, RESOURCE_MANA, SCHOOL_FIRE, TREE_FIRE ),
      next_in_sequence( nis )
    {
      base_cost         = 0;
      base_execute_time = 0;
      base_dd_min       = 92;
      base_dd_max       = 103;
      direct_power_mod  = 0.15;
      may_crit          = true;
      background        = true;
    }
    virtual void player_buff()
    {
      mirror_image_pet_t* p = (mirror_image_pet_t*) player;
      spell_t::player_buff();
      player_spell_power += player -> cast_pet() -> owner -> composite_spell_power( SCHOOL_FIRE ) / p -> num_images;
    }
    virtual void execute()
    {
      mirror_image_pet_t* p = (mirror_image_pet_t*) player;
      mage_t* o = p -> owner -> cast_mage();
      spell_t::execute();
      if( o -> glyphs.mirror_image && result_is_hit() ) 
      {
        stack_winters_chill( this, 1.00 );
      }
      if( next_in_sequence ) next_in_sequence -> schedule_execute();
    }
  };

  struct mirror_bolt_t : public spell_t
  {
    action_t* next_in_sequence;

    mirror_bolt_t( mirror_image_pet_t* mirror_image, action_t* nis ):
      spell_t( "mirror_bolt", mirror_image, RESOURCE_MANA, SCHOOL_FROST, TREE_FROST ),
      next_in_sequence( nis )
    {
      base_cost         = 0;
      base_execute_time = 3.0;
      base_dd_min       = 163;
      base_dd_max       = 169;
      direct_power_mod  = 0.30;
      may_crit          = true;
      background        = true;
    }
    virtual void execute()
    {
      spell_t::execute();
      if( next_in_sequence ) next_in_sequence -> schedule_execute();
    }
    virtual void player_buff()
    {
      mirror_image_pet_t* p = (mirror_image_pet_t*) player;
      spell_t::player_buff();
      player_spell_power += player -> cast_pet() -> owner -> composite_spell_power( SCHOOL_FROST ) / p -> num_images;
    }
  };

  mirror_image_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "mirror_image", true /*guardian*/ ), num_images(3), num_rotations(4)
  {
    for( int i=0; i < num_images; i++ )
    {
      action_t* front=0;

      for( int j=0; j < num_rotations; j++ )
      {
        front = new mirror_bolt_t ( this, front );
	front = new mirror_bolt_t ( this, front );
        front = new mirror_blast_t( this, front );
      }
      sequences.push_back( front );
    }    
  }
  virtual void init_base()
  {
    // Stolen from Priest's Shadowfiend
    attribute_base[ ATTR_STRENGTH  ] = 145;
    attribute_base[ ATTR_AGILITY   ] =  38;
    attribute_base[ ATTR_STAMINA   ] = 190;
    attribute_base[ ATTR_INTELLECT ] = 133;

    health_per_stamina = 7.5;
    mana_per_intellect = 5;
  }
  virtual void summon()
  {
    pet_t::summon();
    for( int i=0; i < num_images; i++ )
    {
      sequences[ i ] -> schedule_execute();
    }
  }
};

// trigger_tier5_4pc ========================================================

static void trigger_tier5_4pc( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Tier5 Buff Expiration";
      player -> aura_gain( "Tier5 Buff" );
      player -> spell_power[ SCHOOL_MAX ] += 70;
      sim -> add_event( this, 6.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Tier5 Buff" );
      player -> spell_power[ SCHOOL_MAX ] -= 70;
      player -> expirations.tier5_4pc = 0;
    }
  };

  mage_t* p = s -> player -> cast_mage();

  if( p -> gear.tier5_4pc )
  {
    p -> procs.tier5_4pc -> occur();

    event_t*& e = p -> expirations.tier5_4pc;
    
    if( e )
    {
      e -> reschedule( 6.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

// trigger_tier8_2pc ========================================================

static void trigger_tier8_2pc( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();
  
  if( p -> gear.tier8_2pc == 0 )
    return;
  // http://ptr.wowhead.com/?spell=64867
  if( ! s -> sim -> roll( 0.25 ) )
    return; 
    
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Tier 8 2 Piece Expiration";
      player -> aura_gain( "Tier 8 2 Piece" );
      player -> spell_power[ SCHOOL_MAX ] += 350;
      player -> cooldowns.tier8_2pc        = sim -> current_time + 45;
      player -> buffs.tier8_2pc            = 1;
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Tier 8 2 Piece" );
      player -> spell_power[ SCHOOL_MAX ] -= 350;
      player -> buffs.tier8_2pc            = 0;
      player -> expirations.tier8_2pc      = 0;
    }
  };
  
  if( s -> sim -> cooldown_ready( p -> cooldowns.tier8_2pc ) )
  {
    p -> procs.tier8_2pc -> occur();

    event_t*& e = p -> expirations.tier8_2pc;
    
    if( e )
    {
      e -> reschedule( 15.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

// trigger_tier8_4pc ========================================================

static bool trigger_tier8_4pc( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if( ! p -> gear.tier8_4pc == 1 )
    return false;

  if( ! s -> sim -> roll( 0.10 ) )
    return false;
    
  p -> procs.tier8_4pc -> occur();

  return true;
}

// trigger_ignite ===========================================================

static void trigger_ignite( spell_t* s,
                            double   dmg )
{
  if( s -> school != SCHOOL_FIRE &&
      s -> school != SCHOOL_FROSTFIRE ) return;

  mage_t* p = s -> player -> cast_mage();

  if( p -> talents.ignite == 0 ) return;

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
      may_resist     = false;
      reset();
    }
    virtual void target_debuff() {}
    virtual void player_buff() {}
  };
  
  double ignite_dmg = dmg * p -> talents.ignite * 0.08;

  if( s -> sim -> merge_ignite )
  {
    s -> result = RESULT_HIT;
    s -> assess_damage( ( ignite_dmg ), DMG_OVER_TIME );
    s -> update_stats( DMG_OVER_TIME );
    s -> result = RESULT_CRIT;
    return;
  }

  if( ! p -> active_ignite ) p -> active_ignite = new ignite_t( p );

  if( p -> active_ignite -> ticking ) 
  {
    p -> procs_deferred_ignite -> occur();

    if( s -> sim -> debug ) report_t::log( s -> sim, "Player %s defers Ignite.", p -> name() );

    int num_ticks = p -> active_ignite -> num_ticks;
    int remaining_ticks = num_ticks - p -> active_ignite -> current_tick;

    ignite_dmg += p -> active_ignite -> base_td * remaining_ticks;

    p -> active_ignite -> cancel();
  }

  p -> active_ignite -> base_td = ignite_dmg / 2.0;
  p -> active_ignite -> schedule_tick();
}

// trigger_burnout ==========================================================

static void trigger_burnout( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if( ! p -> talents.burnout )
    return;

  if( ! s -> resource_consumed )
    return;

  p -> resource_loss( RESOURCE_MANA, s -> resource_consumed * p -> talents.burnout * 0.01 );
}

// trigger_master_of_elements ===============================================

static void trigger_master_of_elements( spell_t* s, double adjust )
{
  mage_t* p = s -> player -> cast_mage();

  if( s -> resource_consumed == 0 ) 
    return;

  if( p -> talents.master_of_elements == 0 )
    return;

  p -> resource_gain( RESOURCE_MANA, adjust * s -> base_cost * p -> talents.master_of_elements * 0.10, p -> gains_master_of_elements );
}

// trigger_molten_fury ======================================================

static void trigger_molten_fury( spell_t* s )
{
  mage_t*   p = s -> player -> cast_mage();

  if( ! p -> talents.molten_fury ) return;

  if( s -> sim -> target -> health_percentage() < 35 )
  {
    s -> player_multiplier *= 1.0 + p -> talents.molten_fury * 0.06;
  }
}

// trigger_combustion =======================================================

static void trigger_combustion( spell_t* s )
{
  if( s -> school != SCHOOL_FIRE &&
      s -> school != SCHOOL_FROSTFIRE ) return;

  mage_t* p = s -> player -> cast_mage();

  if( p -> _buffs.combustion_crits == 0 ) return;

  p -> _buffs.combustion++;

  if( s -> result == RESULT_CRIT ) p -> _buffs.combustion_crits--;

  if( p -> _buffs.combustion_crits == 0 )
  {
    p -> _buffs.combustion = 0;
  }
}

// trigger_arcane_potency =====================================================

static void trigger_arcane_potency( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if( p -> talents.arcane_potency ) 
  {
    p -> aura_gain( "Arcane Potency" );
    p -> _buffs.arcane_potency = p -> talents.arcane_potency;
  }
}

// clear_arcane_potency =====================================================

static void clear_arcane_potency( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if(   p -> _buffs.arcane_potency &&
      ! p -> _buffs.clearcasting )
  {
    p -> aura_loss( "Arcane Potency" );
    p -> _buffs.arcane_potency = 0;
  }
}

// trigger_arcane_concentration =============================================

static void trigger_arcane_concentration( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if( ! p -> talents.arcane_concentration ) return;

  if( s -> sim -> roll( p -> talents.arcane_concentration * 0.02 ) )
  {
    p -> aura_gain( "Clearcasting" );
    p -> procs_clearcasting -> occur();
    p -> _buffs.clearcasting = s -> sim -> current_time;
    trigger_arcane_potency( s );
  }
}

// trigger_frostbite ========================================================

static void trigger_frostbite( spell_t* s )
{
  if( s -> school != SCHOOL_FROST &&
      s -> school != SCHOOL_FROSTFIRE ) return;

  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Frozen Expiration";
      if( sim -> log ) report_t::log( sim, "Target %s gains Frozen", sim -> target -> name() );
      sim -> target -> debuffs.frozen = sim -> current_time;
      sim -> add_event( this, 5.0 );
    }
    virtual void execute()
    {
      if( sim -> log ) report_t::log( sim, "Target %s loses Frozen", sim -> target -> name() );
      sim -> target -> debuffs.frozen = 0;
      sim -> target -> expirations.frozen = 0;
    }
  };

  mage_t*   p = s -> player -> cast_mage();
  target_t* t = s -> sim -> target;

  int level_diff = t -> level - p -> level;

  if( ( level_diff <= 1 ) && s -> sim -> roll( p -> talents.frostbite * 0.05 ) )
  {
    event_t*& e = t -> expirations.frozen;
    
    if( e )
    {
      e -> reschedule( 5.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim );
    }
  }
}

// trigger_winters_grasp ========================================================

static void trigger_winters_grasp( spell_t* s )
{
  if( s -> school != SCHOOL_FROST &&
      s -> school != SCHOOL_FROSTFIRE ) return;

  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Winters Grasp Expiration";
      if( sim -> log ) report_t::log( sim, "Target %s gains Winters Grasp", sim -> target -> name() );
      sim -> target -> debuffs.frozen = sim -> current_time;
      sim -> target -> debuffs.winters_grasp = 1;
      sim -> add_event( this, 5.0 );
    }
    virtual void execute()
    {
      if( sim -> log ) report_t::log( sim, "Target %s loses Winters Grasp", sim -> target -> name() );
      sim -> target -> debuffs.frozen = 0;
      sim -> target -> debuffs.winters_grasp = 0;
      sim -> target -> expirations.winters_grasp = 0;
    }
  };

  mage_t* p = s -> player -> cast_mage();

  if( s -> sim -> roll( p -> talents.winters_grasp * 0.05 ) )
  {
    event_t*& e = s -> sim -> target -> expirations.winters_grasp;
    
    if( e )
    {
      e -> reschedule( 5.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim );
    }
  }
}

// clear_fingers_of_frost =========================================================

static void clear_fingers_of_frost( spell_t* s )
{
  if( s -> school != SCHOOL_FROST &&
      s -> school != SCHOOL_FROSTFIRE ) return;

  mage_t* p = s -> player -> cast_mage();

  p -> _buffs.shatter_combo = 0;

  if( p -> _buffs.fingers_of_frost > 0 )
  {
    p -> _buffs.fingers_of_frost--;

    if( s -> time_to_execute         > 0 &&
        p -> _buffs.fingers_of_frost == 0 && 
        p -> talents.shatter         > 0 ) 
    {
      p -> _buffs.shatter_combo = 1;
    }
  }
}

// trigger_fingers_of_frost =======================================================

static void trigger_fingers_of_frost( spell_t* s )
{
  if( s -> school != SCHOOL_FROST &&
      s -> school != SCHOOL_FROSTFIRE ) return;

  mage_t* p = s -> player -> cast_mage();

  if( s -> sim -> roll( p -> talents.fingers_of_frost * 0.15/2 ) )
  {
    p -> _buffs.fingers_of_frost = 2;
    p -> _buffs.shatter_combo = 0;
  }
}

// trigger_brain_freeze ===========================================================

static void trigger_brain_freeze( spell_t* s )
{
  if( s -> school != SCHOOL_FROST &&
      s -> school != SCHOOL_FROSTFIRE ) return;

  mage_t* p = s -> player -> cast_mage();

  if( s -> sim -> roll( p -> talents.brain_freeze * 0.05 ) )
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, mage_t* p ) : event_t( sim, p )
      {
        name = "Brain Freeze Expiration";
        p -> aura_gain( "Brain Freeze" );
        p -> _buffs.brain_freeze = sim -> current_time;
        sim -> add_event( this, 15.0 );
      }
      virtual void execute()
      {
        mage_t* p = player -> cast_mage();
        p -> aura_loss( "Brain Freeze" );
        p -> _buffs.brain_freeze       = 0;
        p -> _expirations.brain_freeze = 0;
      }
    };
    
    event_t*& e = p -> _expirations.brain_freeze;
          
    if( e )
    {
      e -> reschedule( 15.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
    p -> _buffs.brain_freeze = s -> sim -> current_time;
  }
}

// trigger_hot_streak ==============================================================

static void trigger_hot_streak( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if( p -> talents.hot_streak )
  {
    if( s -> result == RESULT_CRIT && 
        s -> sim -> roll( p -> talents.hot_streak * (1/3.0) ) )
    {
      p -> _buffs.hot_streak++;

      if( p -> _buffs.hot_streak == 2 )
      {
        struct expiration_t : public event_t
        {
          expiration_t( sim_t* sim, mage_t* p ) : event_t( sim, p )
          {
            name = "Hot Streak Expiration";
            p -> aura_gain( "Hot Streak" );
            p -> _buffs.hot_streak_pyroblast = sim -> current_time;
            sim -> add_event( this, 10.0 );
          }
          virtual void execute()
          {
            mage_t* p = player -> cast_mage();
            p -> aura_loss( "Hot Streak" );
            p -> _buffs.hot_streak_pyroblast = 0;
            p -> _expirations.hot_streak     = 0;
          }
        };
        
        p -> _buffs.hot_streak = 0;
        
        event_t*& e = p -> _expirations.hot_streak;
              
        if( e )
        {
          e -> reschedule( 10.0 );
        }
        else
        {
          e = new ( s -> sim ) expiration_t( s -> sim, p );
        }
      }
    }
    else
    {
      p -> _buffs.hot_streak = 0;
    }
  }
}

// stack_improved_scorch =========================================================

static void stack_improved_scorch( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if( ! p -> talents.improved_scorch ) return;

  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Improved Scorch Expiration";
      sim -> add_event( this, 30.0 );
    }
    virtual void execute()
    {
      if( sim -> log ) report_t::log( sim, "%s loses Improved Scorch", sim -> target -> name() );
      sim -> target -> debuffs.improved_scorch = 0;
      sim -> target -> expirations.improved_scorch = 0;
    }
  };

  if( s -> sim -> roll( p -> talents.improved_scorch / 3.0 ) )
  {
    target_t* t = s -> sim -> target;

    if( t -> debuffs.improved_scorch < 5 )
    {
      if( p -> glyphs.improved_scorch )
      {
        t -> debuffs.improved_scorch += ( s -> sim -> P309 ? 3 : 5 );
        if( t -> debuffs.improved_scorch > 5 ) t -> debuffs.improved_scorch = 5;
      }
      else
      {
        t -> debuffs.improved_scorch += 1;
      }
      if( s -> sim -> log ) report_t::log( s -> sim, "%s gains Improved Scorch %d", t -> name(), t -> debuffs.improved_scorch );
    }

    event_t*& e = t -> expirations.improved_scorch;
          
    if( e )
    {
      e -> reschedule( 30.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim );
    }
  }
}

// trigger_missile_barrage =========================================================

static void trigger_missile_barrage( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if( p -> talents.missile_barrage )
  {
    if( s -> sim -> roll( p -> talents.missile_barrage * 0.04 ) )
    {
      struct expiration_t : public event_t
      {
        expiration_t( sim_t* sim, mage_t* p ) : event_t( sim, p )
        {
          name = "Missile Barrage Expiration";
          p -> aura_gain( "Missile Barrage" );
          p -> _buffs.missile_barrage = sim -> current_time;
          sim -> add_event( this, 15.0 );
        }
        virtual void execute()
        {
          mage_t* p = player -> cast_mage();
          p -> aura_loss( "Missile Barrage" );
          p -> _buffs.missile_barrage       = 0;
          p -> _expirations.missile_barrage = 0;
        }
      };
      
      event_t*& e = p -> _expirations.missile_barrage;
            
      if( e )
      {
        e -> reschedule( 15.0 );
      }
      else
      {
        e = new ( s -> sim ) expiration_t( s -> sim, p );
      }
    }
  }
}

// trigger_replenishment ===========================================================

static void trigger_replenishment( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if( s -> sim -> P309 )
    return;

  if( ! p -> talents.improved_water_elemental )
    return;

  if( ! s -> sim -> roll( p -> talents.improved_water_elemental / 3.0 ) )
    return;

  p -> trigger_replenishment();
}

// trigger_ashtongue_talisman ======================================================

static void trigger_ashtongue_talisman( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Ashtongue Talisman Expiration";
      player -> aura_gain( "Ashtongue Talisman" );
      player -> haste_rating += 145;
      player -> recalculate_haste();
      sim -> add_event( this, 5.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Ashtongue Talisman" );
      player -> haste_rating -= 145;
      player -> recalculate_haste();
      player -> expirations.ashtongue_talisman = 0;
    }
  };

  player_t* p = s -> player;

  if( p -> gear.ashtongue_talisman && s -> sim -> roll( 0.50 ) )
  {
    p -> procs.ashtongue_talisman -> occur();

    event_t*& e = p -> expirations.ashtongue_talisman;

    if( e )
    {
      e -> reschedule( 5.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

// =========================================================================
// Mage Spell
// =========================================================================

// mage_spell_t::parse_options =============================================

void mage_spell_t::parse_options( option_t*          options,
                                  const std::string& options_str )
{
  option_t base_options[] =
  {
    { "dps",          OPT_INT, &dps_rotation },
    { "dpm",          OPT_INT, &dpm_rotation },
    { "arcane_power", OPT_INT, &arcane_power },
    { "icy_veins",    OPT_INT, &icy_veins    },
    { NULL }
  };
  std::vector<option_t> merged_options;
  spell_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// mage_spell_t::ready =====================================================

bool mage_spell_t::ready()
{
  mage_t* p = player -> cast_mage();

  if( dps_rotation )
    if( p -> rotation.current != ROTATION_DPS )
      return false;

  if( dpm_rotation )
    if( p -> rotation.current != ROTATION_DPM )
      return false;

  if( arcane_power )
    if( p -> _buffs.arcane_power == 0 )
      return false;

  if( icy_veins )
    if( p -> _buffs.icy_veins == 0 )
      return false;

  return spell_t::ready();
}

// mage_spell_t::cost ======================================================

double mage_spell_t::cost()
{
  mage_t* p = player -> cast_mage();
  if( p -> _buffs.clearcasting ) return 0;
  double c = spell_t::cost();
  if( p -> _buffs.arcane_power ) c += base_cost * 0.20;
  return c;
}

// mage_spell_t::haste =====================================================

double mage_spell_t::haste()
{
  mage_t* p = player -> cast_mage();
  double h = spell_t::haste();
  if( p -> _buffs.icy_veins ) h *= 1.0 / ( 1.0 + 0.20 );
  if( p -> talents.netherwind_presence )
  {
    h *= 1.0 / ( 1.0 + p -> talents.netherwind_presence * 0.02 );
  }
  p -> uptimes_icy_veins -> update( p -> _buffs.icy_veins != 0 );
  return h;
}

// mage_spell_t::execute ===================================================

void mage_spell_t::execute()
{
  mage_t* p = player -> cast_mage();

  p -> uptimes_arcane_power -> update( p -> _buffs.arcane_power != 0 );
  p -> uptimes_dps_rotation -> update( p -> rotation.current == ROTATION_DPS );
  p -> uptimes_dpm_rotation -> update( p -> rotation.current == ROTATION_DPM );
  p -> uptimes.tier8_2pc    -> update( p -> buffs.tier8_2pc == 1 );
  
  spell_t::execute();

  clear_fingers_of_frost( this );
  
  if( result_is_hit() )
  {
    trigger_arcane_concentration( this );
    trigger_frostbite( this );
    trigger_winters_grasp( this );
    trigger_fingers_of_frost( this );
    stack_winters_chill( this, p -> talents.winters_chill / 3.0 );

    if( result == RESULT_CRIT )
    {
      trigger_burnout( this );
      trigger_ignite( this, direct_dmg );
      trigger_master_of_elements( this, 1.0 );
      trigger_tier5_4pc( this );
      trigger_ashtongue_talisman( this );
    }
  }
  trigger_combustion( this );
  trigger_brain_freeze( this );
  clear_arcane_potency( this );
}

// mage_spell_t::consume_resource ==========================================

void mage_spell_t::consume_resource()
{
  mage_t* p = player -> cast_mage();
  spell_t::consume_resource();
  if( p -> _buffs.clearcasting )
  {
    // Treat the savings like a mana gain.
    double amount = spell_t::cost();
    if( amount > 0 )
    {
      p -> aura_loss( "Clearcasting" );
      p -> gains_clearcasting -> add( amount );
      p -> _buffs.clearcasting = 0;
    }
  }
}

// mage_spell_t::player_buff ===============================================

void mage_spell_t::player_buff()
{
  mage_t* p = player -> cast_mage();

  spell_t::player_buff();

  trigger_molten_fury( this );
  
  if( school == SCHOOL_FIRE ||
      school == SCHOOL_FROSTFIRE )
  {
    player_crit += ( p -> _buffs.combustion * 0.10 );
  }
  if( school == SCHOOL_FROST ||
      school == SCHOOL_FROSTFIRE )
  {
    if( sim -> target -> debuffs.frozen )
    {
      player_crit += p -> talents.shatter * 0.5/3;
    }
    else if( p -> talents.fingers_of_frost )
    {
      if( p -> _buffs.fingers_of_frost || ( p -> _buffs.shatter_combo && time_to_execute == 0 ) )
      {
        player_crit += p -> talents.shatter * 0.5/3;
      }
      p -> uptimes_fingers_of_frost -> update( p -> _buffs.fingers_of_frost != 0 );
    }
  }

  if( p -> _buffs.arcane_power ) player_multiplier *= 1.20;

  if( p -> talents.playing_with_fire )
  {
    player_multiplier *= 1.0 + p -> talents.playing_with_fire * 0.01;
  }

  if( p -> _buffs.arcane_potency )
  {
    player_crit += p -> talents.arcane_potency * 0.15;
  }

  if( p -> _buffs.molten_armor )
  {
    if ( sim -> P309)
    {
      player_crit += p -> glyphs.molten_armor ? 0.05 : 0.03;
    }
    else
    {
      player_crit += p -> spirit() * ( p -> glyphs.molten_armor ? 0.55 : 0.35 ) / p -> rating.spell_crit;
    }
  }

  if ( p -> buffs.focus_magic_feedback )
  {
    player_crit += 0.03;
  }
  p -> uptimes_focus_magic_feedback -> update( p -> buffs.focus_magic_feedback != 0 );

  if( sim -> debug ) 
    report_t::log( sim, "mage_spell_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f mult=%.2f", 
                   name(), player_hit, player_crit, player_spell_power, player_penetration, player_multiplier );
}

// =========================================================================
// Mage Spells
// =========================================================================

// Arcane Barrage Spell ====================================================

struct arcane_barrage_t : public mage_spell_t
{
  arcane_barrage_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "arcane_barrage", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();
    assert( p -> talents.arcane_barrage );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 3, 936, 1144, 0, 0.18 },
      { 70, 2, 709,  865, 0, 0.18 },
      { 60, 1, 386,  470, 0, 0.18 },
      { 0, 0 }
    };
    init_rank( ranks );
    
    base_execute_time = 0;
    may_crit          = true;
    direct_power_mod  = (2.5/3.5); 
    cooldown          = 3.0;
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - p -> talents.arcane_focus  * 0.01;
    base_cost        *= 1.0 - p -> glyphs.arcane_barrage * 0.20;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_hit         += p -> talents.arcane_focus * 0.01;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.spell_power * 0.25 ) + 
                                          ( p -> talents.burnout     * 0.10 ) +
                                          ( p -> gear.tier7_4pc ? 0.05 : 0.00 ) );
  }

  virtual void player_buff()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::player_buff();
    if( p -> _buffs.arcane_blast )
    {
      player_multiplier *= 1.0 + p ->  _buffs.arcane_blast * ( 0.15 + ( p -> glyphs.arcane_blast ? 0.03 : 0.00 ) );
    }
    for( int i=0; i < 4; i++ ) 
    {
      p -> uptimes_arcane_blast[ i ] -> update( i == p -> _buffs.arcane_blast );
    }
    int snared = sim -> target -> debuffs.snared() ? 1 : 0;
    player_multiplier *= 1.0 + snared * p -> talents.torment_the_weak * 0.04;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    if( result_is_hit() ) trigger_missile_barrage( this );
    event_t::early( p -> _expirations.arcane_blast );
  }
};

// Arcane Blast Spell =======================================================

struct arcane_blast_t : public mage_spell_t
{
  int max_buff;

  arcane_blast_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "arcane_blast", player, SCHOOL_ARCANE, TREE_ARCANE ), max_buff(0)
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "ap_burn", OPT_DEPRECATED, (void*) "arcane_power" },
      { "max",     OPT_INT,        &max_buff              },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 4, 1185, 1377, 0, 0.08 },
      { 76, 3, 1047, 1215, 0, 0.08 },
      { 71, 2, 897,  1041, 0, 0.08 },
      { 64, 1, 842,  978, 0, 195  },
      { 0, 0 }
    };
    init_rank( ranks );
    
    base_execute_time = 2.5; 
    may_crit          = true;
    direct_power_mod  = (2.5/3.5); 
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - p -> talents.arcane_focus  * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_multiplier  *= 1.0 + p -> talents.spell_impact * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_impact * 0.02;
    base_crit        += p -> talents.incineration * 0.02;
    base_hit         += p -> talents.arcane_focus * 0.01;
    direct_power_mod += p -> talents.arcane_empowerment * 0.03;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.spell_power * 0.25   ) + 
                                          ( p -> talents.burnout     * 0.10   ) +
                                          ( p -> gear.tier7_4pc ? 0.05 : 0.00 ) );

    if( p -> gear.tier5_2pc ) base_multiplier *= 1.05;
  }

  virtual double cost()
  {
    mage_t* p = player -> cast_mage();
    double c = mage_spell_t::cost();
    if( c != 0 )
    {
      if( p -> _buffs.arcane_blast ) c += base_cost * p -> _buffs.arcane_blast * 2.00;
      if( p -> gear.tier5_2pc      ) c += base_cost * 0.05;
    }
    return c;
  }

  virtual void execute()
  {
    mage_spell_t::execute(); 

    if( result_is_hit() )
    {
      trigger_missile_barrage( this );
      trigger_tier8_2pc( this );
    }

    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
      {
        name = "Arcane Blast Expiration";
        p -> aura_gain( "Arcane Blast" );
        sim -> add_event( this, 10.0 );
      }
      virtual void execute()
      {
        mage_t* p = player -> cast_mage();
        p -> aura_loss( "Arcane Blast" );
        p -> _buffs.arcane_blast = 0;
        p -> _expirations.arcane_blast = 0;
      }
    };

    mage_t* p = player -> cast_mage();

    if( p -> _buffs.arcane_blast < 3 )
    {
      p -> _buffs.arcane_blast++;

      if( sim -> debug ) report_t::log( sim, "%s gains Arcane Blast %d", p -> name(), p -> _buffs.arcane_blast );
    }

    event_t*& e = p -> _expirations.arcane_blast;

    if( e )
    {
      e -> reschedule( 10.0 );
    }
    else
    {
      e = new ( sim ) expiration_t( sim, p );
    }
  }

  virtual void player_buff()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::player_buff();
    if( p -> _buffs.arcane_blast )
    {
      player_multiplier *= 1.0 + p ->  _buffs.arcane_blast * ( 0.15 + ( p -> glyphs.arcane_blast ? 0.03 : 0.00 ) );
    }
    for( int i=0; i < 4; i++ ) 
    {
      p -> uptimes_arcane_blast[ i ] -> update( i == p -> _buffs.arcane_blast );
    }
    int snared = sim -> target -> debuffs.snared() ? 1 : 0;
    player_multiplier *= 1.0 + snared * p -> talents.torment_the_weak * 0.04;
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if( ! mage_spell_t::ready() )
      return false;

    if( max_buff > 0 )
      if( p -> _buffs.arcane_blast >= max_buff )
        return false;

    return true;
  }
};

// Arcane Missiles Spell ====================================================

struct arcane_missiles_t : public mage_spell_t
{
  spell_t* abar_spell;
  int abar_combo;
  int barrage;
  int clearcast;

  arcane_missiles_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "arcane_missiles", player, SCHOOL_ARCANE, TREE_ARCANE ), abar_combo(0), barrage(0), clearcast(0)
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "abar_combo", OPT_INT, &abar_combo },
      { "barrage",    OPT_INT, &barrage    },
      { "clearcast",  OPT_INT, &clearcast  },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 79, 13, 360, 260, 0, 0.31 },
      { 75, 12, 320, 320, 0, 0.31 },
      { 70, 11, 280, 280, 0, 785  },
      { 69, 10, 260, 260, 0, 740  },
      { 63,  9, 240, 240, 0, 685  },
      { 60,  8, 230, 230, 0, 655  },
      { 0, 0 }
    };
    init_rank( ranks );

    base_execute_time = 0;
    base_tick_time    = 1.0; 
    num_ticks         = 5; 
    may_crit          = true; 
    channeled         = true;
    direct_power_mod  = base_tick_time / 3.5; // bonus per missle
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - p -> talents.arcane_focus  * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_hit         += p -> talents.arcane_focus * 0.01;
    direct_power_mod += p -> talents.arcane_empowerment * 0.03;        // bonus per missle

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.spell_power * 0.25 ) + 
                                          ( p -> talents.burnout     * 0.10 ) +
                                          ( p -> glyphs.arcane_missiles ? 0.25 : 0.00 ) +
                                          ( p -> gear.tier7_4pc         ? 0.05 : 0.00 ) );

    if( abar_combo )
    {
      std::string abar_options;
      abar_spell = new arcane_barrage_t( player, abar_options );
      // prevents scheduling of player_ready events
      abar_spell -> background = true;  
      abar_spell -> proc = true;  
    }

    if( p -> gear.tier6_4pc ) base_multiplier *= 1.05;
  }

  virtual void player_buff()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::player_buff();
    if( p -> _buffs.arcane_blast )
    {
      player_multiplier *= 1.0 + p ->  _buffs.arcane_blast * ( 0.15 + ( p -> glyphs.arcane_blast ? 0.03 : 0.00 ) );
    }
    for( int i=0; i < 4; i++ ) 
    {
      p -> uptimes_arcane_blast[ i ] -> update( i == p -> _buffs.arcane_blast );
    }
    int snared = sim -> target -> debuffs.snared() ? 1 : 0;
    player_multiplier *= 1.0 + snared * p -> talents.torment_the_weak * 0.04;
  }

  // Odd things to handle:
  // (1) Execute is guaranteed.
  // (2) Each "tick" is like an "execute".

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    player_buff();
    p -> uptimes_missile_barrage -> update( p -> _buffs.missile_barrage != 0 );
    if( p -> _buffs.missile_barrage )
    {
      base_tick_time = 0.5;
      if( ! trigger_tier8_4pc( this ) )
      {
        event_t::early( p -> _expirations.missile_barrage );
      }
    }
    else
    {
      base_tick_time = 1.0;
    }
    schedule_tick();
    update_ready();
    direct_dmg = 0;
    update_stats( DMG_DIRECT );
    trigger_arcane_concentration( this );
    p -> action_finish( this );
  }

  virtual void tick() 
  {
    mage_t* p = player -> cast_mage();

    if( current_tick == num_ticks && abar_combo && abar_spell -> ready() )
    {
      if( sim -> debug ) report_t::log( sim, "Skipping last tick of %s to combo with %s", name(), abar_spell -> name() );
      return;
    }

    if( sim -> debug ) report_t::log( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );

    may_resist = false;
    player_buff(); // this oddball spell requires player stat recalculation before each missile.
    target_debuff( DMG_DIRECT );
    calculate_result();
    may_resist = true;

    if( result_is_hit() )
    {
      calculate_direct_damage();
      p -> action_hit( this );
      if( direct_dmg > 0 )
      {
        tick_dmg = direct_dmg;
        assess_damage( tick_dmg, DMG_OVER_TIME );
      }
      if( result == RESULT_CRIT )
      {
        trigger_master_of_elements( this, ( 1.0 / num_ticks ) );
        trigger_tier5_4pc( this );
        trigger_ashtongue_talisman( this );
      }
    }
    else
    {
      if( sim -> log ) report_t::log( sim, "%s avoids %s (%s)", sim -> target -> name(), name(), util_t::result_type_string( result ) );
      p -> action_miss( this );
    }

    update_stats( DMG_OVER_TIME );
  }

  virtual void last_tick()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::last_tick();
    clear_arcane_potency( this );
    if( abar_combo && abar_spell -> ready() ) 
    {
      p -> action_start( abar_spell );
      abar_spell -> execute();
      p -> last_foreground_action = abar_spell;
    }
    event_t::early( p -> _expirations.arcane_blast );
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if( ! mage_spell_t::ready() )
      return false;

    if( barrage )
      if( ! sim -> time_to_think( p -> _buffs.missile_barrage ) )
        return false;

    if( clearcast )
      if( cost() != 0 || ! sim -> time_to_think( p -> _buffs.clearcasting ) )
        return false;

    return true;
  }
};

// Arcane Power Spell ======================================================

struct arcane_power_t : public mage_spell_t
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
    {
      name = "Arcane Power Expiration";
      mage_t* p = player -> cast_mage();
      p -> aura_gain( "Arcane Power" );
      p -> _buffs.arcane_power = 1;
      sim -> add_event( this, p -> glyphs.arcane_power ? 18.0 : 15.0 );
    }
    virtual void execute()
    {
      mage_t* p = player -> cast_mage();
      p -> aura_loss( "Arcane Power" );
      p -> _buffs.arcane_power = 0;
    }
  };
   
  arcane_power_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "arcane_power", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();
    assert( p -> talents.arcane_power );
    trigger_gcd = 0;  
    cooldown = 120.0;
    cooldown *= 1.0 - p -> talents.arcane_flows * 0.15;
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    new ( sim ) expiration_t( sim, player );
  }
};

// Slow Spell ================================================================

struct slow_t : public mage_spell_t
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Slow Expiration";
      if( sim -> log ) report_t::log( sim, "Target %s gains Slow", sim -> target -> name() );
      sim -> target -> debuffs.slow = 1;
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      if( sim -> log ) report_t::log( sim, "Target %s loses Slow", sim -> target -> name() );
      sim -> target -> debuffs.slow = 0;
    }
  };
   
  slow_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "slow", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();
    assert( p -> talents.slow );
    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.12;
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    new ( sim ) expiration_t( sim );
  }

  virtual bool ready()
  {
    if( ! mage_spell_t::ready() )
      return false;

    return( ! sim -> target -> debuffs.snared() );
  }
};

// Focus Magic Spell ========================================================

struct focus_magic_t : public mage_spell_t
{
  player_t* focus_magic_target;

  focus_magic_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "focus_magic", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();
    assert( p -> talents.focus_magic );

    std::string target_str = p -> focus_magic_target_str;
    option_t options[] =
    {
      { "target", OPT_STRING, &target_str },
      { NULL }
    };
    parse_options( options, options_str );

    if( target_str.empty() )
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
  }
   
  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    if( focus_magic_target == p )
    {
      if( sim -> log ) report_t::log( sim, "%s grants SomebodySomewhere Focus Magic", p -> name() );
      p -> buffs.focus_magic_feedback = 1;
    }
    else
    {
      if( sim -> log ) report_t::log( sim, "%s grants %s Focus Magic", p -> name(), focus_magic_target -> name() );
      focus_magic_target -> buffs.focus_magic = p;
    }
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if( focus_magic_target == p )
    {
      return p -> buffs.focus_magic_feedback == 0;
    }
    else
    {
      return focus_magic_target -> buffs.focus_magic == 0;
    }
  }
};

// Evocation Spell ==========================================================

struct evocation_t : public mage_spell_t
{
  evocation_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "evocation", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();

    base_tick_time = 2.0; 
    num_ticks      = 4; 
    channeled      = true;  
    cooldown       = 240;
    cooldown      -= p -> talents.arcane_flows * 60.0;
    harmful        = false;

    if( p -> gear.tier6_2pc ) num_ticks++;
  }
   
  virtual void tick()
  {
    mage_t* p = player -> cast_mage();
    double mana = p -> resource_max[ RESOURCE_MANA ] * 0.15;
    p -> resource_gain( RESOURCE_MANA, mana, p -> gains_evocation );
  }

  virtual bool ready()
  {
    if( ! mage_spell_t::ready() )
      return false;

    return ( player -> resource_current[ RESOURCE_MANA ] / 
             player -> resource_max    [ RESOURCE_MANA ] ) < 0.30;
  }
};

// Presence of Mind Spell ===================================================

struct presence_of_mind_t : public mage_spell_t
{
  action_t* fast_action;

  presence_of_mind_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "presence_of_mind", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();
    assert( p -> talents.presence_of_mind );

    cooldown = 120.0;
    if( p -> gear.tier4_4pc ) cooldown -= 24.0;
    cooldown *= 1.0 - p -> talents.arcane_flows * 0.15;

    if( options_str.empty() )
    {
      report_t::log( sim, "simcraft: The presence_of_mind action must be coupled with a second action." );
      exit(0);
    }

    std::string spell_name    = options_str;
    std::string spell_options = "";

    std::string::size_type cut_pt = spell_name.find_first_of( "," );       

    if( cut_pt != spell_name.npos )
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
    mage_t* p = player -> cast_mage();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    p -> aura_gain( "Presence of Mind" );
    p -> last_foreground_action = fast_action;
    trigger_arcane_potency( this );
    fast_action -> execute();
    p -> aura_loss( "Presence of Mind" );
    update_ready();
  }

  virtual bool ready()
  {
    return( mage_spell_t::ready() && fast_action -> ready() );
  }
};

// Fire Ball Spell =========================================================

struct fire_ball_t : public mage_spell_t
{
  int brain_freeze;

  fire_ball_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "fire_ball", player, SCHOOL_FIRE, TREE_FIRE ), brain_freeze(0)
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "brain_freeze", OPT_INT, &brain_freeze },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 78, 16, 888, 1132, 0, 0.19 },
      { 74, 15, 783,  997, 0, 0.19 },
      { 70, 14, 717,  913, 0, 0.19 },
      { 66, 13, 633,  805, 0, 425  },
      { 60, 12, 596,  760, 0, 410  },
      { 60, 11, 561,  715, 0, 395  },
      { 0, 0 }
    };
    init_rank( ranks );
    
    base_execute_time = 3.5; 
    may_crit          = true; 
    direct_power_mod  = base_execute_time / 3.5;
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_execute_time -= p -> talents.improved_fire_ball * 0.1;
    base_multiplier   *= 1.0 + p -> talents.fire_power * 0.02;
    base_multiplier   *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_multiplier   *= 1.0 + p -> talents.spell_impact * 0.02;
    base_crit         += p -> talents.arcane_instability * 0.01;
    base_crit         += p -> talents.critical_mass * 0.02;
    base_crit         += p -> talents.pyromaniac * 0.01;
    direct_power_mod  += p -> talents.empowered_fire * 0.05;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.spell_power * 0.25 ) +
                                          ( p -> talents.burnout     * 0.10 ) + 
                                          ( p -> gear.tier7_4pc ? 0.05 : 0.00 ) );

    if( ! sim -> P309 ) base_crit += p -> talents.improved_scorch * 0.01;

    if( p -> gear.tier6_4pc   ) base_multiplier *= 1.05;
    if( p -> glyphs.fire_ball ) base_crit += 0.05;
  }

  virtual void player_buff()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::player_buff();

    int snared = sim -> target -> debuffs.snared() ? 1 : 0;
    player_multiplier *= 1.0 + snared * p -> talents.torment_the_weak * 0.04;
  }

  virtual double cost()
  {
    mage_t* p = player -> cast_mage();
    if( p -> _buffs.brain_freeze ) return 0;
    return mage_spell_t::cost();
  }

  virtual double execute_time()
  {
    mage_t* p = player -> cast_mage();
    if( p -> _buffs.brain_freeze ) return 0;
    return mage_spell_t::execute_time();
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    if( result_is_hit() )
    {
      trigger_missile_barrage( this );
      trigger_tier8_2pc( this );
    }
    trigger_hot_streak( this );
    if( p -> _expirations.brain_freeze )
      if( ! trigger_tier8_4pc( this ) )
        event_t::early( p -> _expirations.brain_freeze );
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if( ! mage_spell_t::ready() )
      return false;

    if( brain_freeze )
      if( ! sim -> time_to_think( p -> _buffs.brain_freeze ) )
        return false;

    return true;
  }
};

// Fire Blast Spell ========================================================

struct fire_blast_t : public mage_spell_t
{
  fire_blast_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "fire_blast", player, SCHOOL_FIRE, TREE_FIRE )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 11, 925, 1095, 0, 0.21 },
      { 74, 10, 760,  900, 0, 0.21 },
      { 70,  9, 664,  786, 0, 465  },
      { 61,  8, 539,  637, 0, 400  },
      { 54,  7, 431,  509, 0, 340  },
      { 0, 0 }
    };
    init_rank( ranks );
    
    base_execute_time = 0; 
    may_crit          = true; 
    cooldown          = 8.0;
    direct_power_mod  = (1.5/3.5); 
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    cooldown         -= p -> talents.improved_fire_blast * 0.5;
    base_multiplier  *= 1.0 + p -> talents.fire_power * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_multiplier  *= 1.0 + p -> talents.spell_impact * 0.02;
    base_crit        += p -> talents.incineration * 0.02;
    base_crit        += p -> talents.critical_mass * 0.02;
    base_crit        += p -> talents.pyromaniac * 0.01;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.spell_power * 0.25 ) +
                                          ( p -> talents.burnout     * 0.10 ) +
                                          ( p -> gear.tier7_4pc ? 0.05 : 0.00 ) );
  }
  virtual void execute()
  {
    mage_spell_t::execute();
    trigger_hot_streak( this );
  }
};

// Living Bomb Spell ========================================================

struct living_bomb_t : public mage_spell_t
{
  double explosion;

  living_bomb_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "living_bomb", player, SCHOOL_FIRE, TREE_FIRE ), explosion(0)
  {
    mage_t* p = player -> cast_mage();
    assert( p -> talents.living_bomb );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 3, 759, 759, 690, 0.22 },
      { 70, 2, 568, 568, 512, 0.22 },
      { 60, 1, 336, 336, 306, 0.22 },
      { 0, 0 }
    };
    init_rank( ranks );
    
    base_execute_time = 0;
    base_tick_time    = 3.0; 
    num_ticks         = 4; 
    direct_power_mod  = 0.40; 
    tick_power_mod    = base_tick_time / 15;
    may_crit          = true;
    tick_may_crit     = p -> glyphs.living_bomb != 0;
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_multiplier  *= 1.0 + p -> talents.fire_power * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.critical_mass * 0.02;
    base_crit        += p -> talents.pyromaniac * 0.01;
    base_crit        += p -> talents.world_in_flames * 0.02;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.spell_power * 0.25 ) +
                                          ( p -> talents.burnout     * 0.10 ) + 
                                          ( p -> gear.tier7_4pc ? 0.05 : 0.00 ) );
  }

  // Odd thing to handle: The direct-damage comes at the last tick instead of the beginning of the spell.

  virtual void last_tick()
  {
    target_debuff( DMG_DIRECT );
    calculate_result();
    if( result_is_hit() )
    {
      calculate_direct_damage();
      direct_dmg = explosion;
      if( direct_dmg > 0 )
      {
        assess_damage( direct_dmg, DMG_DIRECT );
        if( result == RESULT_CRIT ) 
        {
          trigger_burnout( this );
          trigger_ignite( this, direct_dmg );
          trigger_master_of_elements( this, 1.0 );
        }
      }
    }
    update_stats( DMG_DIRECT );
    mage_spell_t::last_tick();
  }

  virtual double calculate_direct_damage()
  {
    mage_spell_t::calculate_direct_damage();
    explosion = direct_dmg;
    direct_dmg = 0;
    return direct_dmg;
  }
};

// Pyroblast Spell ========================================================

struct pyroblast_t : public mage_spell_t
{
  int hot_streak;

  pyroblast_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "pyroblast", player, SCHOOL_FIRE, TREE_FIRE ), hot_streak(0)
  {
    mage_t* p = player -> cast_mage();
    assert( p -> talents.pyroblast );

    option_t options[] =
    {
      { "hot_streak", OPT_INT, &hot_streak },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 77, 12, 1190, 1510, 75, 0.22 },
      { 73, 11, 1014, 1286, 64, 0.22 },
      { 70, 10,  939, 1191, 59, 500  },
      { 66,  9,  846, 1074, 52, 460  },
      { 60,  8,  708,  898, 45, 440  },
      { 0, 0 }
    };
    init_rank( ranks );
    
    base_execute_time = 5.0;
    base_tick_time    = 3.0; 
    num_ticks         = 4; 
    may_crit          = true; 
    direct_power_mod  = 1.15;
    tick_power_mod    = 0.20 / num_ticks;
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_multiplier  *= 1.0 + p -> talents.fire_power * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.critical_mass * 0.02;
    base_crit        += p -> talents.pyromaniac * 0.01;
    base_crit        += p -> talents.world_in_flames * 0.02;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.spell_power * 0.25 ) +
                                          ( p -> talents.burnout     * 0.10 ) + 
                                          ( p -> gear.tier7_4pc ? 0.05 : 0.00 ) );
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    if( p -> _expirations.hot_streak )
    {
      p -> procs_hot_streak_pyroblast -> occur();
      if( ! trigger_tier8_4pc( this ) )
        event_t::early( p -> _expirations.hot_streak );
    }
    // When performing Hot Streak Pyroblasts, do not wait for DoT to complete.
    if( hot_streak ) duration_ready=0;
  }

  virtual double execute_time()
  {
    mage_t* p = player -> cast_mage();
    if( p -> _buffs.hot_streak_pyroblast ) return 0;
    return mage_spell_t::execute_time();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if( ! mage_spell_t::ready() )
      return false;

    if( hot_streak )
      if( ! sim -> time_to_think( p -> _buffs.hot_streak_pyroblast ) )
        return false;

    return true;
  }
};

// Scorch Spell ==========================================================

struct scorch_t : public mage_spell_t
{
  int debuff;

  scorch_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "scorch", player, SCHOOL_FIRE, TREE_FIRE ), debuff( 0 )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "debuff",    OPT_INT, &debuff     },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 78, 11, 376, 444, 0, 0.08 },
      { 73, 10, 321, 379, 0, 0.08 },
      { 70,  9, 305, 361, 0, 180  },
      { 64,  8, 269, 317, 0, 165  },
      { 58,  7, 233, 275, 0, 150  },
      { 0, 0 }
    };
    init_rank( ranks );
    
    base_execute_time = 1.5; 
    may_crit          = true;
    direct_power_mod  = (1.5/3.5); 
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_multiplier  *= 1.0 + p -> talents.fire_power         * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_multiplier  *= 1.0 + p -> talents.spell_impact       * 0.02;

    base_crit        += p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.incineration       * 0.02;
    base_crit        += p -> talents.critical_mass      * 0.02;
    base_crit        += p -> talents.pyromaniac         * 0.01;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.spell_power * 0.25 ) +
                                          ( p -> talents.burnout     * 0.10 ) +
                                          ( p -> gear.tier7_4pc ? 0.05 : 0.00 ) );

    if( ! sim -> P309 ) base_crit += p -> talents.improved_scorch * 0.01;

    if( debuff ) assert( p -> talents.improved_scorch );
  }

  virtual void execute()
  {
    mage_spell_t::execute(); 
    if( result_is_hit() ) stack_improved_scorch( this );
    trigger_hot_streak( this );
  }

  virtual bool ready()
  {
    if( ! mage_spell_t::ready() )
      return false;

    if( debuff )
    {
      target_t* t = sim -> target;

      if( t -> debuffs.improved_scorch >= 5 )
        return false;

      event_t* e = t -> expirations.improved_scorch;

      if( e && sim -> current_time < ( e -> occurs() - 6.0 ) )
        return false;
    }

    return true;
  }
};

// Combustion Spell =========================================================

struct combustion_t : public mage_spell_t
{
  combustion_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "combustion", player, SCHOOL_FIRE, TREE_FIRE )
  {
    mage_t* p = player -> cast_mage();
    assert( p -> talents.combustion );
    cooldown = 180;
  }
   
  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    p -> aura_gain( "Combustion" );
    p -> _buffs.combustion = 0;
    p -> _buffs.combustion_crits = 3;
    update_ready();
  }
};

// Frost Bolt Spell =========================================================

struct frost_bolt_t : public mage_spell_t
{
  frost_bolt_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "frost_bolt", player, SCHOOL_FROST, TREE_FROST )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 79, 16, 799, 861, 0, 0.13 },
      { 75, 15, 702, 758, 0, 0.13 },
      { 70, 14, 630, 680, 0, 0.13 },
      { 68, 13, 597, 644, 0, 330  },
      { 62, 12, 522, 563, 0, 300  },
      { 60, 11, 515, 555, 0, 290  },
      { 56, 10, 429, 463, 0, 260  },
      { 50, 9,  353, 383, 0, 225  },
      { 44, 8,  292, 316, 0, 195  },
      { 0, 0 }
    };
    init_rank( ranks );
    
    base_execute_time = 3.0; 
    may_crit          = true; 
    direct_power_mod  = ( base_execute_time / 3.5 ) * 0.95; 
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_execute_time -= p -> talents.improved_frost_bolt * 0.1;
    base_multiplier   *= 1.0 + p -> talents.piercing_ice * 0.02;
    base_multiplier   *= 1.0 + p -> talents.arctic_winds * 0.01;
    base_multiplier   *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_multiplier   *= 1.0 + p -> talents.chilled_to_the_bone * 0.01;
    base_crit         += p -> talents.arcane_instability * 0.01;
    base_crit         += p -> talents.empowered_frost_bolt * 0.02;
    direct_power_mod  += p -> talents.empowered_frost_bolt * 0.05;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.ice_shards  * 1.0/3 ) +
                                          ( p -> talents.spell_power * 0.25  ) + 
                                          ( p -> talents.burnout     * 0.10  ) +
                                          ( p -> gear.tier7_4pc ? 0.05 : 0.00 ) );

    if( ! sim -> P309 ) base_crit += p -> talents.winters_chill * 0.01;

    if( p -> gear.tier6_4pc    ) base_multiplier *= 1.05;
    if( p -> glyphs.frost_bolt ) base_multiplier *= 1.05;
  }

  virtual void player_buff()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::player_buff();

    int snared = sim -> target -> debuffs.snared() ? 1 : 0;
    player_multiplier *= 1.0 + snared * p -> talents.torment_the_weak * 0.04;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    if( result_is_hit() ) 
    {
      trigger_missile_barrage( this );
      trigger_replenishment( this );
      trigger_tier8_2pc( this );
    }
  }
};

// Ice Lance Spell =========================================================

struct ice_lance_t : public mage_spell_t
{
  int frozen;
  int fb_priority;

  ice_lance_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "ice_lance", player, SCHOOL_FROST, TREE_FROST ), frozen(0), fb_priority(0)
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "frozen",      OPT_INT, &frozen      },
      { "fb_priority", OPT_INT, &fb_priority },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 78, 3, 221, 255, 0, 0.07 },
      { 72, 2, 182, 210, 0, 0.07 },
      { 66, 1, 161, 187, 0, 150  },
      { 0, 0 }
    };
    init_rank( ranks );
    
    base_execute_time = 0.0; 
    may_crit          = true; 
    direct_power_mod  = (0.5/3.5); 
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_multiplier  *= 1.0 + p -> talents.piercing_ice * 0.02;
    base_multiplier  *= 1.0 + p -> talents.spell_impact * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_multiplier  *= 1.0 + p -> talents.arctic_winds * 0.01;
    base_multiplier  *= 1.0 + p -> talents.chilled_to_the_bone * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.ice_shards  * 1.0/3 ) +
                                          ( p -> talents.spell_power * 0.25  ) +
                                          ( p -> talents.burnout     * 0.10  ) +
                                          ( p -> gear.tier7_4pc ? 0.05 : 0.00 ) );
  }

  virtual void player_buff()
  {
    mage_t*   p = player -> cast_mage();
    target_t* t = sim -> target;

    mage_spell_t::player_buff();

    if( p -> _buffs.shatter_combo    ||
        p -> _buffs.fingers_of_frost ||
        t -> debuffs.frozen         ) 
    {
      if( p -> glyphs.ice_lance && t -> level > p -> level )
      {
        player_multiplier *= 4.0;
      }
      else
      {
        player_multiplier *= 3.0;
      }
    }
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if( ! mage_spell_t::ready() )
      return false;

    if( frozen )
    {
      if( ! p -> _buffs.shatter_combo &&
          ! p -> _buffs.fingers_of_frost &&
          ! sim -> time_to_think( sim -> target -> debuffs.frozen ) )
        return false;
    }
    
    if( fb_priority )
    {
      double fb_execute_time = sim -> current_time + 2.5 * haste();

      if( sim -> time_to_think( sim -> target -> debuffs.frozen ) )
        if( fb_execute_time < sim -> target -> expirations.frozen -> occurs() )
          return false;

      if( p -> _buffs.fingers_of_frost )
        return false;
    }

    return true;
  }
};

// Frostfire Bolt Spell ======================================================

struct frostfire_bolt_t : public mage_spell_t
{
  int dot_wait;

  frostfire_bolt_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "frostfire_bolt", player, SCHOOL_FROSTFIRE, TREE_FROST ), dot_wait(0)
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "dot_wait", OPT_INT, &dot_wait   },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 2, 722, 838, 30, 0.14 },
      { 75, 1, 629, 731, 20, 0.14 },
      { 0, 0 }
    };
    init_rank( ranks );
    
    base_execute_time = 3.0; 
    may_crit          = true; 
    direct_power_mod  = base_execute_time / 3.5; 

    if( dot_wait )
    {
      base_tick_time = 3.0;
      num_ticks      = 3;
      tick_power_mod = 0;
    }
      
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_multiplier  *= 1.0 + p -> talents.fire_power * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_multiplier  *= 1.0 + p -> talents.piercing_ice * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arctic_winds * 0.01;
    base_multiplier  *= 1.0 + p -> talents.chilled_to_the_bone * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.critical_mass * 0.02;
    base_crit        += p -> talents.pyromaniac * 0.01;
    direct_power_mod += p -> talents.empowered_fire * 0.05;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.ice_shards  * 1.0/3 ) +
                                          ( p -> talents.spell_power * 0.25  ) +
                                          ( p -> talents.burnout     * 0.10  ) +
                                          ( p -> gear.tier7_4pc ? 0.05 : 0.00 ) );

    if( ! sim -> P309 ) base_crit += p -> talents.improved_scorch * 0.01;

    if ( p -> glyphs.frostfire )
    {
      base_multiplier *= 1.02;
      base_crit += 0.02;
    }
  }

  virtual void player_buff()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::player_buff();

    int snared = sim -> target -> debuffs.snared() ? 1 : 0;
    player_multiplier *= 1.0 + snared * p -> talents.torment_the_weak * 0.04;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    if( result_is_hit() )
    {
      trigger_missile_barrage( this );
      trigger_tier8_2pc( this );
    }
    trigger_hot_streak( this );
  }
};

// Icy Veins Spell =========================================================

struct icy_veins_t : public mage_spell_t
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
    {
      name = "Icy Veins Expiration";
      mage_t* p = player -> cast_mage();
      p -> aura_gain( "Icy Veins" );
      p -> _buffs.icy_veins = 1;
      sim -> add_event( this, 20.0 );
    }
    virtual void execute()
    {
      mage_t* p = player -> cast_mage();
      p -> aura_loss( "Icy Veins" );
      p -> _buffs.icy_veins = 0;
    }
  };
   
  icy_veins_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "icy_veins", player, SCHOOL_FROST, TREE_FROST )
  {
    mage_t* p = player -> cast_mage();
    assert( p -> talents.icy_veins );
    base_cost = 200;
    trigger_gcd = 0;
    cooldown = 180.0;
    cooldown *= 1.0 - p -> talents.ice_floes * ( 0.20 / 3.0 );
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    new ( sim ) expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if( ! mage_spell_t::ready() )
      return false;

    return player -> cast_mage() -> _buffs.icy_veins == 0;
  }
};

// Cold Snap Spell ==========================================================

struct cold_snap_t : public mage_spell_t
{
  cold_snap_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "cold_snap", player, SCHOOL_FROST, TREE_FROST )
  {
    mage_t* p = player -> cast_mage();
    assert( p -> talents.cold_snap );
    cooldown = 600;
    cooldown *= 1.0 - p -> talents.cold_as_ice * 0.10;
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );

    for( action_t* a = player -> action_list; a; a = a -> next )
    {
      if( a -> school == SCHOOL_FROST )
      {
        a -> cooldown_ready = 0;
      }
    }

    update_ready();
  }
};

// Molten Armor Spell =====================================================

struct molten_armor_t : public mage_spell_t
{
  molten_armor_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "molten_armor", player, SCHOOL_FIRE, TREE_FIRE )
  {
    trigger_gcd = 0;
  }
   
  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    p -> _buffs.mage_armor   = 0;
    p -> _buffs.molten_armor = 1;
  }

  virtual bool ready()
  {
    return( player -> cast_mage() -> _buffs.molten_armor == 0 );
  }
};

// Mage Armor Spell =======================================================

struct mage_armor_t : public mage_spell_t
{
  mage_armor_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "mage_armor", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    trigger_gcd = 0;
  }
   
  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    p -> _buffs.mage_armor   = 1;
    p -> _buffs.molten_armor = 0;
  }

  virtual bool ready()
  {
    return( player -> cast_mage() -> _buffs.mage_armor == 0 );
  }
};

// Arcane Brilliance Spell =================================================

struct arcane_brilliance_t : public mage_spell_t
{
  double bonus;

  arcane_brilliance_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "arcane_brilliance", player, SCHOOL_ARCANE, TREE_ARCANE ), bonus(0)
  {
    trigger_gcd = 0;

    bonus = util_t::ability_rank( player -> level,  60.0,80,  40.0,70,  31.0,0 );
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );

    for( player_t* p = sim -> player_list; p; p = p -> next )
    {
      p -> buffs.arcane_brilliance = bonus;
      p -> init_resources( true );      
    }
  }

  virtual bool ready()
  {
    return( player -> buffs.arcane_brilliance < bonus );
  }
};

// Water Elemental Spell ================================================

struct water_elemental_spell_t : public mage_spell_t
{
  struct water_elemental_expiration_t : public event_t
  {
    water_elemental_expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
    {
      mage_t* p = player -> cast_mage();
      p -> summon_pet( "water_elemental" );
      sim -> add_event( this, 45.0 + p -> talents.improved_water_elemental * 5.0 );
    }
    virtual void execute()
    {
      player -> dismiss_pet( "water_elemental" );
    }
  };

  water_elemental_spell_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "water_elemental", player, SCHOOL_FROST, TREE_FROST )
  {
    mage_t* p = player -> cast_mage();
    assert( p -> talents.summon_water_elemental );
    
    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.16;
    base_cost *= 1.0 - p -> talents.frost_channeling * (0.1/3);
    cooldown   = p -> glyphs.water_elemental ? 150 : 180;
    cooldown  *= 1.0 - p -> talents.cold_as_ice * 0.10;
    harmful    = false;
  }

  virtual void execute() 
  {
    consume_resource();
    update_ready();
    new ( sim ) water_elemental_expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if( ! mage_spell_t::ready() )
      return false;

    return player -> cast_mage() -> active_water_elemental == 0;
  }
};

// Mirror Image Spell ======================================================

struct mirror_image_spell_t : public mage_spell_t
{
  mirror_image_spell_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "mirror_image", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();
    
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.10;
    base_cost  *= 1.0 - p -> talents.frost_channeling * (0.1/3);
    cooldown    = 180;
    trigger_gcd = 0;
    harmful     = false;
  }

  virtual void execute() 
  {
    consume_resource();
    update_ready();
    player -> summon_pet( "mirror_image" );
  }
};

// Mana Gem =================================================================

struct mana_gem_t : public action_t
{
  double trigger;
  double min;
  double max;

  mana_gem_t( player_t* player, const std::string& options_str ) : 
    action_t( ACTION_USE, "mana_gem", player ), trigger(0), min(3330), max(3500)
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "min",     OPT_FLT, &min     },
      { "max",     OPT_FLT, &max     },
      { "trigger", OPT_FLT, &trigger },
      { NULL }
    };
    parse_options( options, options_str );

    if( min == 0 && max == 0) min = max = trigger;

    if( min > max ) std::swap( min, max );

    if( max == 0 ) max = trigger;
    if( trigger == 0 ) trigger = max;
    assert( max > 0 && trigger > 0 );

    if( p -> glyphs.mana_gem ) 
    {
      min *= 1.40;
      max *= 1.40;
      trigger *= 1.40;
    }
    if( p -> gear.tier7_2pc ) 
    {
      min *= 1.40;
      max *= 1.40;
      trigger *= 1.40;
    }

    cooldown = 120.0;
    cooldown_group = "rune";
    trigger_gcd = 0;
    harmful = false;
  }
  
  virtual void execute()
  {
    mage_t* p = player -> cast_mage();

    if( sim -> log ) report_t::log( sim, "%s uses Mana Gem", p -> name() );

    double gain = sim -> rng -> range( min, max );

    if( p -> gear.tier7_2pc ) 
    {
      struct expiration_t : public event_t
      {
        expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
        {
          name = "Tier7 2-Piece Expiration";
          player -> aura_gain( "Tier7 2-Piece" );
          player -> spell_power[ SCHOOL_MAX ] += 225;
          sim -> add_event( this, 15.0 );
        }
        virtual void execute()
        {
          player -> aura_loss( "Tier7 2-Piece" );
          player -> spell_power[ SCHOOL_MAX ] -= 225;
        }
      };
    
      new ( sim ) expiration_t( sim, p );
    }

    p -> resource_gain( RESOURCE_MANA, gain, p -> gains_mana_gem );
    p -> share_cooldown( cooldown_group, cooldown );
  }

  virtual bool ready()
  {
    if( cooldown_ready > sim -> current_time ) 
      return false;

    return( player -> resource_max    [ RESOURCE_MANA ] - 
            player -> resource_current[ RESOURCE_MANA ] ) > trigger;
  }
};

// Choose Rotation ==========================================================

struct choose_rotation_t : public action_t
{
  double last_time;

  choose_rotation_t( player_t* p, const std::string& options_str ) : 
    action_t( ACTION_USE, "choose_rotation", p ), last_time(0)
  {
    cooldown = 10;

    option_t options[] =
    {
      { "cooldown", OPT_FLT, &cooldown },
      { NULL }
    };
    parse_options( options, options_str );

    if( cooldown < 1.0 )
    {
      printf( "simcraft: choose_rotation cannot have cooldown less than 1.0sec\n" );
      exit(0);
    }

    trigger_gcd = 0;
    harmful = false;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();

    if( sim -> log ) report_t::log( sim, "%s Considers Spell Rotation", p -> name() );

    // It is important to smooth out the regen rate by averaging out the returns from Evocation and Mana Gems.
    // In order for this to work, the resource_gain() method must filter out these sources when
    // tracking "rotation.mana_gain".

    double regen_rate = p -> rotation.mana_gain / sim -> current_time;

    // Evocation
    regen_rate += p -> resource_max[ RESOURCE_MANA ] * 0.60 / ( 240.0 - p -> talents.arcane_flows * 60.0 );

    // Mana Gem
    regen_rate += 3400 * ( 1.0 + p -> glyphs.mana_gem * 0.40 ) * ( 1.0 + p -> gear.tier7_2pc * 0.40 ) / 120.0;

    if( p -> rotation.current == ROTATION_DPS )
    {
      p -> rotation.dps_time += ( sim -> current_time - last_time );

      double consumption_rate = ( p -> rotation.dps_mana_loss / p -> rotation.dps_time ) - regen_rate;

      if( consumption_rate > 0 )
      {
        double oom_time = p -> resource_current[ RESOURCE_MANA ] / consumption_rate;

        if( oom_time < sim -> target -> time_to_die() )
        {
          if( sim -> log ) report_t::log( sim, "%s switches to DPM spell rotation", p -> name() );

          p -> rotation.current = ROTATION_DPM;
        }
      }
    }
    else if( p -> rotation.current == ROTATION_DPM )
    {
      p -> rotation.dpm_time += ( sim -> current_time - last_time );

      double consumption_rate = ( p -> rotation.dpm_mana_loss / p -> rotation.dpm_time ) - regen_rate;

      if( consumption_rate > 0 )
      {
        double oom_time = p -> resource_current[ RESOURCE_MANA ] / consumption_rate;

        if( oom_time > sim -> target -> time_to_die() )
        {
          if( sim -> log ) report_t::log( sim, "%s switches to DPS spell rotation", p -> name() );

          p -> rotation.current = ROTATION_DPS;
        }
      }
      else
      {
        if( sim -> log ) report_t::log( sim, "%s switches to DPS rotation (negative consumption)", p -> name() );

        p -> rotation.current = ROTATION_DPS;
      }
    }
    last_time = sim -> current_time;

    update_ready();
  }

  virtual bool ready()
  {
    return( sim -> current_time >= cooldown_ready &&
            sim -> current_time >= cooldown );
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
  if( name == "arcane_barrage"    ) return new        arcane_barrage_t( this, options_str );
  if( name == "arcane_blast"      ) return new          arcane_blast_t( this, options_str );
  if( name == "arcane_brilliance" ) return new     arcane_brilliance_t( this, options_str );
  if( name == "arcane_missiles"   ) return new       arcane_missiles_t( this, options_str );
  if( name == "arcane_power"      ) return new          arcane_power_t( this, options_str );
  if( name == "choose_rotation"   ) return new       choose_rotation_t( this, options_str );
  if( name == "cold_snap"         ) return new             cold_snap_t( this, options_str );
  if( name == "combustion"        ) return new            combustion_t( this, options_str );
  if( name == "evocation"         ) return new             evocation_t( this, options_str );
  if( name == "fire_ball"         ) return new             fire_ball_t( this, options_str );
  if( name == "fire_blast"        ) return new            fire_blast_t( this, options_str );
  if( name == "focus_magic"       ) return new           focus_magic_t( this, options_str );
  if( name == "frost_bolt"        ) return new            frost_bolt_t( this, options_str );
  if( name == "frostfire_bolt"    ) return new        frostfire_bolt_t( this, options_str );
  if( name == "ice_lance"         ) return new             ice_lance_t( this, options_str );
  if( name == "icy_veins"         ) return new             icy_veins_t( this, options_str );
  if( name == "living_bomb"       ) return new           living_bomb_t( this, options_str );
  if( name == "mage_armor"        ) return new            mage_armor_t( this, options_str );
  if( name == "mirror_image"      ) return new    mirror_image_spell_t( this, options_str );
  if( name == "molten_armor"      ) return new          molten_armor_t( this, options_str );
  if( name == "presence_of_mind"  ) return new      presence_of_mind_t( this, options_str );
  if( name == "pyroblast"         ) return new             pyroblast_t( this, options_str );
  if( name == "scorch"            ) return new                scorch_t( this, options_str );
  if( name == "slow"              ) return new                  slow_t( this, options_str );
  if( name == "water_elemental"   ) return new water_elemental_spell_t( this, options_str );
  if( name == "mana_gem"          ) return new              mana_gem_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// mage_t::create_pet ======================================================

pet_t* mage_t::create_pet( const std::string& pet_name )
{
  pet_t* p = find_pet( pet_name );

  if( p ) return p;

  if( pet_name == "water_elemental" ) return new water_elemental_pet_t( sim, this );
  if( pet_name == "mirror_image"    ) return new mirror_image_pet_t   ( sim, this );

  return 0;
}

// mage_t::init_base =======================================================

void mage_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] =  35;
  attribute_base[ ATTR_AGILITY   ] =  41;
  attribute_base[ ATTR_STAMINA   ] =  60;
  attribute_base[ ATTR_INTELLECT ] = 179;
  attribute_base[ ATTR_SPIRIT    ] = 179;

  attribute_multiplier_initial[ ATTR_INTELLECT ] *= 1.0 + talents.arcane_mind * 0.03;
  attribute_multiplier_initial[ ATTR_SPIRIT    ] *= 1.0 + talents.student_of_the_mind * ( 0.1 / 3.0 );

  base_spell_crit = 0.00907381;
  initial_spell_crit_per_intellect = rating_t::interpolate( level, 0.01/60.0, 0.01/80.0, 0.01/166.79732 );
  initial_spell_power_per_intellect = talents.mind_mastery * 0.03;

  base_attack_power = -10;
  base_attack_crit  = 0.0345777;
  initial_attack_power_per_strength = 1.0;
  initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/16.0, 0.01/24.9, 0.01/51.84598 );

  // FIXME! Make this level-specific.
  resource_base[ RESOURCE_HEALTH ] = 3200;
  resource_base[ RESOURCE_MANA   ] = rating_t::interpolate( level, 1183, 2241, 3268 );

  health_per_stamina = 10;
  mana_per_intellect = 15;
}

// mage_t::combat_begin ====================================================

void mage_t::combat_begin() 
{
  player_t::combat_begin();

  if( ! armor_type_str.empty() )
  {
    if( sim -> log ) report_t::log( sim, "%s equips %s armor", name(), armor_type_str.c_str() );

    if( armor_type_str == "mage" )
    {
      _buffs.mage_armor = 1;
    }
    else if( armor_type_str == "molten" ) 
    {
      _buffs.molten_armor = 1;
    }
    else
    {
      printf( "simcraft: Unknown armor type '%s' for player %s\n", armor_type_str.c_str(), name() );
      exit(0);
    }
  }
}

// mage_t::reset ===========================================================

void mage_t::reset()
{
  player_t::reset();

  // Active
  active_water_elemental = 0;

  _buffs.reset();
  _expirations.reset();

  rotation.reset();
}

// mage_t::regen  ==========================================================

void mage_t::regen( double periodicity )
{
  mana_regen_while_casting = 0;

  if( sim -> P309 )
  {
    mana_regen_while_casting += ( talents.arcane_meditation * 0.10 +
                                  talents.pyromaniac        * 0.10 );
  }
  else
  {
    mana_regen_while_casting += util_t::talent_rank( talents.arcane_meditation, 3, 0.17, 0.33, 0.50 );
    mana_regen_while_casting += util_t::talent_rank( talents.pyromaniac,        3, 0.17, 0.33, 0.50 );
  }

  if( _buffs.mage_armor )
  {
    mana_regen_while_casting += ( sim -> P309 ? 0.30 : 0.50 ) + ( glyphs.mage_armor ? 0.20 : 0.00 );
  }

  player_t::regen( periodicity );

  uptimes_water_elemental -> update( active_water_elemental != 0 );
}

// mage_t::resource_gain ===================================================

double mage_t::resource_gain( int     resource,
                              double  amount,
                              gain_t* source )
{
  double actual_amount = player_t::resource_gain( resource, amount, source );

  if( source != gains_evocation &&
      source != gains_mana_gem )
  {
    rotation.mana_gain += actual_amount;
  }

  return actual_amount;
}

// mage_t::resource_loss ===================================================

double mage_t::resource_loss( int     resource,
                              double  amount )
{
  double actual_amount = player_t::resource_loss( resource, amount );

  if( rotation.current == ROTATION_DPS )
  {
    rotation.dps_mana_loss += actual_amount;
  }
  else if( rotation.current == ROTATION_DPM )
  {
    rotation.dpm_mana_loss += actual_amount;
  }

  return actual_amount;
}

// mage_t::get_talent_trees ================================================

bool mage_t::get_talent_trees( std::vector<int*>& arcane,
                               std::vector<int*>& fire,
                               std::vector<int*>& frost )
{
  talent_translation_t translation[][3] =
  {
    { {  1, &( talents.arcane_subtlety      ) }, {  1, &( talents.improved_fire_blast ) }, {  1, &( talents.frostbite                ) } },
    { {  2, &( talents.arcane_focus         ) }, {  2, &( talents.incineration        ) }, {  2, &( talents.improved_frost_bolt      ) } },
    { {  3, NULL                              }, {  3, &( talents.improved_fire_ball  ) }, {  3, &( talents.ice_floes                ) } },
    { {  4, NULL                              }, {  4, &( talents.ignite              ) }, {  4, &( talents.ice_shards               ) } },
    { {  5, NULL                              }, {  5, NULL                             }, {  5, NULL                                  } },
    { {  6, &( talents.arcane_concentration ) }, {  6, &( talents.world_in_flames     ) }, {  6, &( talents.precision                ) } },
    { {  7, NULL                              }, {  7, NULL                             }, {  7, NULL                                  } },
    { {  8, &( talents.spell_impact         ) }, {  8, NULL                             }, {  8, &( talents.piercing_ice             ) } },
    { {  9, &( talents.student_of_the_mind  ) }, {  9, &( talents.pyroblast           ) }, {  9, &( talents.icy_veins                ) } },
    { { 10, &( talents.focus_magic          ) }, { 10, &( talents.burning_soul        ) }, { 10, NULL                                  } },
    { { 11, NULL                              }, { 11, &( talents.improved_scorch     ) }, { 11, NULL                                  } },
    { { 12, NULL                              }, { 12, NULL                             }, { 12, &( talents.frost_channeling         ) } },
    { { 13, &( talents.arcane_meditation    ) }, { 13, &( talents.master_of_elements  ) }, { 13, &( talents.shatter                  ) } },
    { { 14, &( talents.torment_the_weak     ) }, { 14, &( talents.playing_with_fire   ) }, { 14, &( talents.cold_snap                ) } },
    { { 15, NULL                              }, { 15, &( talents.critical_mass       ) }, { 15, NULL                                  } },
    { { 16, &( talents.presence_of_mind     ) }, { 16, NULL                             }, { 16, NULL                                  } },
    { { 17, &( talents.arcane_mind          ) }, { 17, NULL                             }, { 17, &( talents.cold_as_ice              ) } },
    { { 18, NULL                              }, { 18, &( talents.fire_power          ) }, { 18, &( talents.winters_chill            ) } },
    { { 19, &( talents.arcane_instability   ) }, { 19, &( talents.pyromaniac          ) }, { 19, NULL                                  } },
    { { 20, &( talents.arcane_potency       ) }, { 20, &( talents.combustion          ) }, { 20, NULL                                  } },
    { { 21, &( talents.arcane_empowerment   ) }, { 21, &( talents.molten_fury         ) }, { 21, &( talents.arctic_winds             ) } },
    { { 22, &( talents.arcane_power         ) }, { 22, NULL                             }, { 22, &( talents.empowered_frost_bolt     ) } },
    { { 23, NULL                              }, { 23, &( talents.empowered_fire      ) }, { 23, &( talents.fingers_of_frost         ) } },
    { { 24, &( talents.arcane_flows         ) }, { 24, NULL                             }, { 24, &( talents.brain_freeze             ) } },
    { { 25, &( talents.mind_mastery         ) }, { 25, NULL                             }, { 25, &( talents.summon_water_elemental   ) } },
    { { 26, &( talents.slow                 ) }, { 26, &( talents.hot_streak          ) }, { 26, &( talents.improved_water_elemental ) } },
    { { 27, &( talents.missile_barrage      ) }, { 27, &( talents.burnout             ) }, { 27, &( talents.chilled_to_the_bone      ) } },
    { { 28, &( talents.netherwind_presence  ) }, { 28, &( talents.living_bomb         ) }, { 28, NULL                                  } },
    { { 29, &( talents.spell_power          ) }, {  0, NULL                             }, {  0, NULL                                  } },
    { { 30, &( talents.arcane_barrage       ) }, {  0, NULL                             }, {  0, NULL                                  } },
    { {  0, NULL                              }, {  0, NULL                             }, {  0, NULL                                  } }
  };
  
  return player_t::get_talent_trees( arcane, fire, frost, translation );
}

// mage_t::parse_talents_mmo ==============================================

bool mage_t::parse_talents_mmo( const std::string& talent_string )
{
  // mage mmo encoding: Fire-Frost-Arcane

  int size1 = 28;
  int size2 = 28;

  std::string   fire_string( talent_string,     0,  size1 );
  std::string  frost_string( talent_string, size1,  size2 );
  std::string arcane_string( talent_string, size1 + size2 );

  return parse_talents( arcane_string + fire_string + frost_string );
}

// mage_t::parse_option  ==================================================

bool mage_t::parse_option( const std::string& name,
                           const std::string& value )
{
  option_t options[] =
  {
    { "arcane_barrage",            OPT_INT,   &( talents.arcane_barrage            ) },
    { "arcane_concentration",      OPT_INT,   &( talents.arcane_concentration      ) },
    { "arcane_empowerment",        OPT_INT,   &( talents.arcane_empowerment        ) },
    { "arcane_flows",              OPT_INT,   &( talents.arcane_flows              ) },
    { "arcane_focus",              OPT_INT,   &( talents.arcane_focus              ) },
    { "arcane_impact",             OPT_INT,   &( talents.arcane_impact             ) },
    { "arcane_instability",        OPT_INT,   &( talents.arcane_instability        ) },
    { "arcane_meditation",         OPT_INT,   &( talents.arcane_meditation         ) },
    { "arcane_mind",               OPT_INT,   &( talents.arcane_mind               ) },
    { "arcane_potency",            OPT_INT,   &( talents.arcane_potency            ) },
    { "arcane_power",              OPT_INT,   &( talents.arcane_power              ) },
    { "arcane_subtlety",           OPT_INT,   &( talents.arcane_subtlety           ) },
    { "arctic_winds",              OPT_INT,   &( talents.arctic_winds              ) },
    { "brain_freeze",              OPT_INT,   &( talents.brain_freeze              ) },
    { "burning_soul",              OPT_INT,   &( talents.burning_soul              ) },
    { "burnout",                   OPT_INT,   &( talents.burnout                   ) },
    { "chilled_to_the_bone",       OPT_INT,   &( talents.chilled_to_the_bone       ) },
    { "cold_as_ice",               OPT_INT,   &( talents.cold_as_ice               ) },
    { "cold_snap",                 OPT_INT,   &( talents.cold_snap                 ) },
    { "combustion",                OPT_INT,   &( talents.combustion                ) },
    { "critical_mass",             OPT_INT,   &( talents.critical_mass             ) },
    { "deep_freeze",               OPT_INT,   &( talents.deep_freeze               ) },
    { "elemental_precision",       OPT_INT,   &( talents.precision                 ) }, // deprecated
    { "empowered_arcane_missiles", OPT_INT,   &( talents.empowered_arcane_missiles ) },
    { "empowered_fire",            OPT_INT,   &( talents.empowered_fire            ) },
    { "empowered_frost_bolt",      OPT_INT,   &( talents.empowered_frost_bolt      ) },
    { "fingers_of_frost",          OPT_INT,   &( talents.fingers_of_frost          ) },
    { "fire_power",                OPT_INT,   &( talents.fire_power                ) },
    { "focus_magic",               OPT_INT,   &( talents.focus_magic               ) },
    { "frost_channeling",          OPT_INT,   &( talents.frost_channeling          ) },
    { "frostbite",                 OPT_INT,   &( talents.frostbite                 ) },
    { "hot_streak",                OPT_INT,   &( talents.hot_streak                ) },
    { "ice_floes",                 OPT_INT,   &( talents.ice_floes                 ) },
    { "ice_shards",                OPT_INT,   &( talents.ice_shards                ) },
    { "ignite",                    OPT_INT,   &( talents.ignite                    ) },
    { "improved_fire_ball",        OPT_INT,   &( talents.improved_fire_ball        ) },
    { "improved_fire_blast",       OPT_INT,   &( talents.improved_fire_blast       ) },
    { "improved_frost_bolt",       OPT_INT,   &( talents.improved_frost_bolt       ) },
    { "improved_scorch",           OPT_INT,   &( talents.improved_scorch           ) },
    { "improved_water_elemental",  OPT_INT,   &( talents.improved_water_elemental  ) },
    { "incineration",              OPT_INT,   &( talents.incineration              ) },
    { "living_bomb",               OPT_INT,   &( talents.living_bomb               ) },
    { "master_of_elements",        OPT_INT,   &( talents.master_of_elements        ) },
    { "mind_mastery",              OPT_INT,   &( talents.mind_mastery              ) },
    { "missile_barrage",           OPT_INT,   &( talents.missile_barrage           ) },
    { "molten_fury",               OPT_INT,   &( talents.molten_fury               ) },
    { "netherwind_presence",       OPT_INT,   &( talents.netherwind_presence       ) },
    { "piercing_ice",              OPT_INT,   &( talents.piercing_ice              ) },
    { "playing_with_fire",         OPT_INT,   &( talents.playing_with_fire         ) },
    { "precision",                 OPT_INT,   &( talents.precision                 ) },
    { "presence_of_mind",          OPT_INT,   &( talents.presence_of_mind          ) },
    { "pyroblast",                 OPT_INT,   &( talents.pyroblast                 ) },
    { "pyromaniac",                OPT_INT,   &( talents.pyromaniac                ) },
    { "shatter",                   OPT_INT,   &( talents.shatter                   ) },
    { "slow",                      OPT_INT,   &( talents.slow                      ) },
    { "spell_impact",              OPT_INT,   &( talents.spell_impact              ) },
    { "spell_power",               OPT_INT,   &( talents.spell_power               ) },
    { "student_of_the_mind",       OPT_INT,   &( talents.student_of_the_mind       ) },
    { "torment_the_weak",          OPT_INT,   &( talents.torment_the_weak          ) },
    { "water_elemental",           OPT_INT,   &( talents.summon_water_elemental    ) },
    { "winters_chill",             OPT_INT,   &( talents.winters_chill             ) },
    { "winters_grasp",             OPT_INT,   &( talents.winters_grasp             ) },
    { "world_in_flames",           OPT_INT,   &( talents.world_in_flames           ) },
    // Glyphs
    { "glyph_arcane_barrage",      OPT_INT,   &( glyphs.arcane_barrage             ) },
    { "glyph_arcane_blast",        OPT_INT,   &( glyphs.arcane_blast               ) },
    { "glyph_arcane_missiles",     OPT_INT,   &( glyphs.arcane_missiles            ) },
    { "glyph_arcane_power",        OPT_INT,   &( glyphs.arcane_power               ) },
    { "glyph_fire_ball",           OPT_INT,   &( glyphs.fire_ball                  ) },
    { "glyph_frost_bolt",          OPT_INT,   &( glyphs.frost_bolt                 ) },
    { "glyph_ice_lance",           OPT_INT,   &( glyphs.ice_lance                  ) },
    { "glyph_improved_scorch",     OPT_INT,   &( glyphs.improved_scorch            ) },
    { "glyph_living_bomb",         OPT_INT,   &( glyphs.living_bomb                ) },
    { "glyph_mage_armor",          OPT_INT,   &( glyphs.mage_armor                 ) },
    { "glyph_mana_gem",            OPT_INT,   &( glyphs.mana_gem                   ) },
    { "glyph_mirror_image",        OPT_INT,   &( glyphs.mirror_image               ) },
    { "glyph_molten_armor",        OPT_INT,   &( glyphs.molten_armor               ) },
    { "glyph_water_elemental",     OPT_INT,   &( glyphs.water_elemental            ) },
    { "glyph_frostfire",           OPT_INT,   &( glyphs.frostfire                  ) },
    // Options
    { "armor_type",                OPT_STRING, &( armor_type_str                   ) },
    { "focus_magic_target",        OPT_STRING, &( focus_magic_target_str           ) },
    { NULL, OPT_UNKNOWN }
  };
  
  if( name.empty() )
  {
    player_t::parse_option( std::string(), std::string() );
    option_t::print( sim, options );
    return false;
  }

  if( option_t::parse( sim, options, name, value ) ) return true;

  return player_t::parse_option( name, value );
}

// player_t::create_mage  ===================================================

player_t* player_t::create_mage( sim_t*       sim, 
                                 std::string& name ) 
{
  mage_t* p = new mage_t( sim, name );

  new    mirror_image_pet_t( sim, p );
  new water_elemental_pet_t( sim, p );
  
  return p;
}
