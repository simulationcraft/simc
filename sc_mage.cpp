// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Mage
// ==========================================================================

struct mage_t : public player_t
{
  // Active
  spell_t* active_ignite;
  spell_t* active_evocation;
  pet_t*   active_water_elemental;

  // Buffs
  int8_t buffs_arcane_blast;
  int8_t buffs_arcane_potency;
  int8_t buffs_arcane_power;
  double buffs_brain_freeze;
  double buffs_clearcasting;
  int8_t buffs_combustion;
  int8_t buffs_combustion_crits;
  int8_t buffs_fingers_of_frost;
  int8_t buffs_hot_streak;
  double buffs_hot_streak_pyroblast;
  int8_t buffs_icy_veins;
  int8_t buffs_mage_armor;
  double buffs_missile_barrage;
  int8_t buffs_molten_armor;
  int8_t buffs_shatter_combo;

  // Expirations
  event_t* expirations_arcane_blast;

  // Gains
  gain_t* gains_clearcasting;
  gain_t* gains_master_of_elements;
  gain_t* gains_evocation;

  // Procs
  proc_t* procs_clearcasting;
  proc_t* procs_hot_streak_pyroblast;
  proc_t* procs_missile_barrage;
  proc_t* procs_deferred_ignite;

  // Up-Times
  uptime_t* uptimes_arcane_blast[ 4 ];
  uptime_t* uptimes_fingers_of_frost;
  uptime_t* uptimes_water_elemental;

  struct talents_t
  {
    int8_t  arcane_barrage;
    int8_t  arcane_concentration;
    int8_t  arcane_empowerment;
    int8_t  arcane_flows;
    int8_t  arcane_focus;
    int8_t  arcane_impact;
    int8_t  arcane_instability;
    int8_t  arcane_meditation;
    int8_t  arcane_mind;
    int8_t  arcane_potency;
    int8_t  arcane_power;
    int8_t  arcane_subtlety;
    int8_t  arctic_winds;
    int8_t  brain_freeze;
    int8_t  burning_soul;
    int8_t  burnout;
    int8_t  chilled_to_the_bone;
    int8_t  cold_as_ice;
    int8_t  cold_snap;
    int8_t  combustion;
    int8_t  critical_mass;
    int8_t  deep_freeze;
    int8_t  elemental_precision;
    int8_t  empowered_arcane_missiles;
    int8_t  empowered_fire_ball;
    int8_t  empowered_frost_bolt;
    int8_t  fingers_of_frost;
    int8_t  fire_power;
    int8_t  focus_magic;
    int8_t  frost_channeling;
    int8_t  frostbite;
    int8_t  hot_streak;
    int8_t  ice_floes;
    int8_t  ice_shards;
    int8_t  icy_veins;
    int8_t  ignite;
    int8_t  improved_fire_ball;
    int8_t  improved_fire_blast;
    int8_t  improved_frost_bolt;
    int8_t  improved_scorch;
    int8_t  improved_water_elemental;
    int8_t  incineration;
    int8_t  living_bomb;
    int8_t  master_of_elements;
    int8_t  mind_mastery;
    int8_t  missile_barrage;
    int8_t  molten_fury;
    int8_t  netherwind_presence;
    int8_t  piercing_ice;
    int8_t  playing_with_fire;
    int8_t  presence_of_mind;
    int8_t  pyroblast;
    int8_t  pyromaniac;
    int8_t  shatter;
    int8_t  slow;
    int8_t  spell_impact;
    int8_t  spell_power;
    int8_t  student_of_the_mind;
    int8_t  torment_the_weak;
    int8_t  water_elemental;
    int8_t  winters_chill;
    int8_t  winters_grasp;
    int8_t  world_in_flames;
    
    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int8_t arcane_power;
    int8_t fire_ball;
    int8_t frost_bolt;
    int8_t improved_scorch;
    int8_t mage_armor;
    int8_t mana_gem;  // FIXME!  Not supported yet.
    int8_t molten_armor;
    int8_t water_elemental;
    glyphs_t() { memset( (void*) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  mage_t( sim_t* sim, std::string& name ) : player_t( sim, MAGE, name )
  {
    // Active
    active_ignite          = 0;
    active_evocation       = 0;
    active_water_elemental = 0;

    // Buffs
    buffs_arcane_blast             = 0;
    buffs_arcane_potency           = 0;
    buffs_arcane_power             = 0;
    buffs_brain_freeze             = 0;
    buffs_clearcasting             = 0;
    buffs_combustion               = 0;
    buffs_combustion_crits         = 0;
    buffs_fingers_of_frost         = 0;
    buffs_hot_streak               = 0;
    buffs_hot_streak_pyroblast     = 0;
    buffs_icy_veins                = 0;
    buffs_mage_armor               = 0;
    buffs_missile_barrage          = 0;
    buffs_molten_armor             = 0;
    buffs_shatter_combo            = 0;

    // Expirations
    expirations_arcane_blast = 0;

    // Gains
    gains_clearcasting       = get_gain( "clearcasting" );
    gains_master_of_elements = get_gain( "master_of_elements" );
    gains_evocation          = get_gain( "evocation" );

    // Procs
    procs_clearcasting         = get_proc( "clearcasting" );
    procs_hot_streak_pyroblast = get_proc( "hot_streak_pyroblast" );
    procs_missile_barrage      = get_proc( "missile_barrage" );
    procs_deferred_ignite      = get_proc( "deferred_ignite" );

    // Up-Times
    uptimes_arcane_blast[ 0 ] = get_uptime( "arcane_blast_0" );
    uptimes_arcane_blast[ 1 ] = get_uptime( "arcane_blast_1" );
    uptimes_arcane_blast[ 2 ] = get_uptime( "arcane_blast_2" );
    uptimes_arcane_blast[ 3 ] = get_uptime( "arcane_blast_3" );
    uptimes_fingers_of_frost  = get_uptime( "fingers_of_frost" );
    uptimes_water_elemental   = get_uptime( "water_elemental" );
  }

  // Character Definition
  virtual void      init_base();
  virtual void      reset();
  virtual void      parse_talents( const std::string& talent_string );
  virtual bool      parse_option ( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual int       primary_resource() { return RESOURCE_MANA; }

  // Event Tracking
  virtual void regen( double periodicity );
};

// ==========================================================================
// Mage Spell
// ==========================================================================

struct mage_spell_t : public spell_t
{
  mage_spell_t( const char* n, player_t* player, int8_t s, int8_t t ) : 
    spell_t( n, player, RESOURCE_MANA, s, t ) 
  {
  }

  virtual double cost();
  virtual double haste();
  virtual void   execute();
  virtual void   consume_resource();
  virtual void   player_buff();

  // Passthru Methods
  virtual void   calculate_damage() { spell_t::calculate_damage(); }
  virtual void   last_tick()        { spell_t::last_tick();        }
  virtual double duration()         { return spell_t::duration();  }
  virtual bool   ready()            { return spell_t::ready();     }
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
      base_execute_time  = 2.5;
      base_dd            = ( 256 + 328 ) / 2;
      base_dd           +=( player -> level - 50 ) * 11.5;
      dd_power_mod       = ( 2.5 / 3.5 );
      may_crit           = true;
      background         = true;
      repeating          = true;
    }
  };

  water_bolt_t* water_bolt;

  water_elemental_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name ) :
    pet_t( sim, owner, pet_name )
  {
    water_bolt = new water_bolt_t( this );
  }
  virtual void init_base()
  {
    // Stolen from Priest's Shadowfiend
    attribute_base[ ATTR_STRENGTH  ] = 153;
    attribute_base[ ATTR_AGILITY   ] = 108;
    attribute_base[ ATTR_STAMINA   ] = 280;
    attribute_base[ ATTR_INTELLECT ] = 133;

    health_per_stamina = 7.5;
    mana_per_intellect = 5;
  }
  virtual void reset()
  {
    player_t::reset();
  }
  virtual void schedule_ready()
  {
    assert(0);
  }
  virtual void summon()
  {
    mage_t* o = cast_pet() -> owner -> cast_mage();

    if( sim -> log ) report_t::log( sim, "%s summons Water Elemental.", o -> name() );

    o -> active_water_elemental = this;

    initial_haste_rating=0;
    for( int i=0; i < ATTRIBUTE_MAX; i++ ) attribute_initial[ i ] = 0;
    for( int i=0; i < RESOURCE_MAX;  i++ )  resource_initial[ i ] = 0;
    initial_spell_power[ SCHOOL_MAX ] = 0;

    // Model the stat-based contribution from the Mage as "gear enchants".
    
    gear.attribute_enchant[ ATTR_STAMINA   ] = (int16_t) ( 0.30 * o -> attribute[ ATTR_STAMINA   ] );
    gear.attribute_enchant[ ATTR_INTELLECT ] = (int16_t) ( 0.30 * o -> attribute[ ATTR_INTELLECT ] );

    gear.spell_power_enchant[ SCHOOL_MAX ] = (int16_t) ( 0.40 * o -> composite_spell_power( SCHOOL_FROST ) );

    init_core();
    init_spell();
    init_resources();

    if( o -> talents.improved_water_elemental )
    {
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
	p -> buffs.water_elemental_regen++;
	if( p -> buffs.water_elemental_regen == 1 ) p -> aura_gain( "Water Elemental Regen" );
      }
    }

    // Kick-off repeating attack.  Eventually, this should go thru schedule_ready().
    water_bolt -> execute();
  }
  virtual void dismiss()
  {
    mage_t* o = cast_pet() -> owner -> cast_mage();

    if( sim -> log ) report_t::log( sim, "%s's Water Elemental dies.", cast_pet() -> owner -> name() );

    if( o -> talents.improved_water_elemental )
    {
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
	p -> buffs.water_elemental_regen--;
	if( p -> buffs.water_elemental_regen == 0 ) p -> aura_loss( "Water Elemental Regen" );
      }
    }

    water_bolt -> cancel();

    o -> active_water_elemental = 0;
  }
};

namespace { // ANONYMOUS NAMESPACE ==========================================

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
      e = new expiration_t( s -> sim, p );
    }
  }
}

// trigger_ignite ===========================================================

static void trigger_ignite( spell_t* s )
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
      base_duration = 4.0;  
      num_ticks     = 2;
      trigger_gcd   = 0;
      background    = true;
      may_resist    = false;
      reset();
    }
    virtual void target_debuff() {}
  };
  
  double ignite_dmg = s -> dd * p -> talents.ignite * 0.08;

  if( s -> sim -> merge_ignite )
  {
    s -> result = RESULT_HIT;
    s -> assess_damage( ( ignite_dmg / 2.0 ), DMG_OVER_TIME );
    s -> result = RESULT_CRIT;
    return;
  }

  if( ! p -> active_ignite ) p -> active_ignite = new ignite_t( p );

  if( p -> active_ignite -> time_remaining > 0 ) 
  {
    p -> procs_deferred_ignite -> occur();

    if( s -> sim -> debug ) report_t::log( s -> sim, "Player %s defers Ignite.", p -> name() );

    int num_ticks = p -> active_ignite -> num_ticks;
    int remaining_ticks = num_ticks - p -> active_ignite -> current_tick;

    ignite_dmg += p -> active_ignite -> dot * remaining_ticks / num_ticks;

    p -> active_ignite -> cancel();
  }

  p -> active_ignite -> dot = ignite_dmg;
  p -> active_ignite -> schedule_tick();
}

// trigger_burnout ==========================================================

static void trigger_burnout( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if( p -> talents.burnout && s -> resource_consumed )
  {
    p -> resource_loss( RESOURCE_MANA, s -> resource_consumed * p -> talents.burnout * 0.01 );
  }
}

// trigger_master_of_elements ===============================================

static void trigger_master_of_elements( spell_t* s )
{
  if( ! sim_t::WotLK )
    if( s -> school != SCHOOL_FIRE  &&
	s -> school != SCHOOL_FROST )
      return;

  mage_t* p = s -> player -> cast_mage();

  p -> resource_gain( RESOURCE_MANA, s -> resource_consumed * p -> talents.master_of_elements * 0.10, p -> gains_master_of_elements );
}

// trigger_molten_fury ======================================================

static void trigger_molten_fury( spell_t* s )
{
  mage_t*   p = s -> player -> cast_mage();
  target_t* t = s -> sim -> target;

  if( ! p -> talents.molten_fury ) return;

  if( t -> initial_health <= 0 ) return;

  static double threshold  = sim_t::WotLK ? 0.35 : 0.20;
  static double multiplier = sim_t::WotLK ? 0.06 : 0.10;

  if( ( t -> current_health / t -> initial_health ) < threshold )
  {
    s -> player_multiplier *= 1.0 + p -> talents.molten_fury * multiplier;
  }
}

// trigger_combustion =======================================================

static void trigger_combustion( spell_t* s )
{
  if( s -> school != SCHOOL_FIRE &&
      s -> school != SCHOOL_FROSTFIRE ) return;

  mage_t* p = s -> player -> cast_mage();

  if( p -> buffs_combustion_crits == 0 ) return;

  p -> buffs_combustion++;

  if( s -> result == RESULT_CRIT ) p -> buffs_combustion_crits--;

  if( p -> buffs_combustion_crits == 0 )
  {
    p -> buffs_combustion = 0;
  }
}

// clear_arcane_potency =====================================================

static void clear_arcane_potency( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if(   p -> buffs_arcane_potency &&
      ! p -> buffs_clearcasting )
  {
    p -> aura_loss( "Arcane Potency" );
    p -> buffs_arcane_potency = 0;
  }
}

// trigger_arcane_concentration =============================================

static void trigger_arcane_concentration( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if( ! p -> talents.arcane_concentration ) return;

  if( rand_t::roll( p -> talents.arcane_concentration * 0.02 ) )
  {
    p -> procs_clearcasting -> occur();
    p -> buffs_clearcasting = s -> sim -> current_time;

    if( p -> talents.arcane_potency ) 
    {
      p -> aura_gain( "Arcane Potency" );
      p -> buffs_arcane_potency = p -> talents.arcane_potency;
    }
  }
}

// stack_winters_chill =====================================================

static void stack_winters_chill( spell_t* s )
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

  mage_t* p = s -> player -> cast_mage();

  if( rand_t::roll( p -> talents.winters_chill * ( sim_t::WotLK ? (1.0/3.0) : 0.20 ) ) )
  {
    target_t* t = s -> sim -> target;

    if( t -> debuffs.winters_chill < 10 ) 
    {
      t -> debuffs.winters_chill += 2;

      if( s -> sim -> log ) 
	report_t::log( s -> sim, "Target %s gains Winters Chill %d", 
		       t -> name(), t -> debuffs.winters_chill / 2 );
    }

    event_t*& e = t -> expirations.winters_chill;
    
    if( e )
    {
      e -> reschedule( 15.0 );
    }
    else
    {
      e = new expiration_t( s -> sim );
    }
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

  if( ( level_diff <= 1 ) && 
      rand_t::roll( p -> talents.frostbite * 0.05 ) )
  {
    event_t*& e = t -> expirations.frozen;
    
    if( e )
    {
      e -> reschedule( 5.0 );
    }
    else
    {
      e = new expiration_t( s -> sim );
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

  if( rand_t::roll( p -> talents.winters_grasp * 0.05 ) )
  {
    event_t*& e = s -> sim -> target -> expirations.winters_grasp;
    
    if( e )
    {
      e -> reschedule( 5.0 );
    }
    else
    {
      e = new expiration_t( s -> sim );
    }
  }
}

// clear_fingers_of_frost =========================================================

static void clear_fingers_of_frost( spell_t* s )
{
  if( s -> school != SCHOOL_FROST &&
      s -> school != SCHOOL_FROSTFIRE ) return;

  mage_t* p = s -> player -> cast_mage();

  p -> buffs_shatter_combo = 0;

  if( p -> buffs_fingers_of_frost > 0 )
  {
    p -> buffs_fingers_of_frost--;

    if( s -> time_to_execute         > 0 &&
	p -> buffs_fingers_of_frost == 0 && 
	p -> talents.shatter         > 0 ) 
    {
      p -> buffs_shatter_combo = 1;
    }
  }
}

// trigger_fingers_of_frost =======================================================

static void trigger_fingers_of_frost( spell_t* s )
{
  if( s -> school != SCHOOL_FROST &&
      s -> school != SCHOOL_FROSTFIRE ) return;

  mage_t* p = s -> player -> cast_mage();

  if( rand_t::roll( p -> talents.fingers_of_frost * 0.05 ) )
  {
    p -> buffs_fingers_of_frost = 2;
    p -> buffs_shatter_combo = 0;
  }
}

// trigger_brain_freeze ===========================================================

static void trigger_brain_freeze( spell_t* s )
{
  if( s -> school != SCHOOL_FROST &&
      s -> school != SCHOOL_FROSTFIRE ) return;

  mage_t* p = s -> player -> cast_mage();

  if( ! p -> buffs_brain_freeze &&
      rand_t::roll( p -> talents.brain_freeze * 0.05 ) )
  {
    p -> buffs_brain_freeze = s -> sim -> current_time;
  }
}

// trigger_hot_streak ==============================================================

static void trigger_hot_streak( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if( p -> talents.hot_streak )
  {
    if( s -> result == RESULT_CRIT && 
	rand_t::roll( p -> talents.hot_streak * (1/3.0) ) )
    {
      p -> buffs_hot_streak++;

      if( p -> buffs_hot_streak == 2 )
      {
	p -> buffs_hot_streak_pyroblast = s -> sim -> current_time;
	p -> buffs_hot_streak = 0;
      }
    }
    else
    {
      p -> buffs_hot_streak = 0;
    }
  }
}

// trigger_missile_barrage =========================================================

static void trigger_missile_barrage( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if(   p -> talents.missile_barrage && 
      ! p ->   buffs_missile_barrage )
  {
    if( rand_t::roll( p -> talents.missile_barrage * 0.03 ) )
    {
      p -> buffs_missile_barrage = s -> sim -> current_time;
    }
  }
}

// arcane_blast_filler =============================================================

static bool arcane_blast_filler( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if( p -> buffs_arcane_blast == 0 )
    return false;

  // Need to be able to finish this spell and start Arcane Blast before the buff wears out.

  double cast_time = s -> channeled ? s -> duration() : std::max( s -> execute_time(), s -> gcd() );

  double finished = s -> sim -> current_time + cast_time + s -> sim -> lag;

  return p -> expirations_arcane_blast -> occurs() > finished;
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

  if( p -> gear.ashtongue_talisman && rand_t::roll( 0.50 ) )
  {
    p -> procs.ashtongue_talisman -> occur();

    event_t*& e = p -> expirations.ashtongue_talisman;

    if( e )
    {
      e -> reschedule( 5.0 );
    }
    else
    {
      e = new expiration_t( s -> sim, p );
    }
  }
}

} // ANONYMOUS NAMESPACE ===================================================

// =========================================================================
// Mage Spell
// =========================================================================

// mage_spell_t::cost ======================================================

double mage_spell_t::cost()
{
  mage_t* p = player -> cast_mage();
  if( p -> buffs_clearcasting ) return 0;
  double c = spell_t::cost();
  if( p -> buffs_arcane_power ) c += base_cost * 1.30;
  return c;
}

// mage_spell_t::haste =====================================================

double mage_spell_t::haste()
{
  mage_t* p = player -> cast_mage();
  double h = spell_t::haste();
  if( p -> buffs_icy_veins ) h *= 1.0 / ( 1.0 + 0.20 );
  if( p -> talents.netherwind_presence )
  {
    h *= 1.0 / ( 1.0 + p -> talents.netherwind_presence * 0.02 );
  }
  return h;
}

// mage_spell_t::execute ===================================================

void mage_spell_t::execute()
{
  spell_t::execute();
  clear_fingers_of_frost( this );
  if( result_is_hit() )
  {
    trigger_arcane_concentration( this );
    trigger_frostbite( this );
    trigger_winters_grasp( this );
    trigger_fingers_of_frost( this );
    stack_winters_chill( this );

    if( result == RESULT_CRIT )
    {
      trigger_burnout( this );
      trigger_ignite( this );
      trigger_master_of_elements( this );
      trigger_tier5_4pc( this );
      trigger_ashtongue_talisman( this );
    }
  }
  if( may_crit && num_ticks == 0 )
  {
    trigger_hot_streak( this );
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
  if( p -> buffs_clearcasting )
  {
    // Treat the savings like a mana gain.
    double amount = spell_t::cost();
    if( amount > 0 )
    {
      p -> gains_clearcasting -> add( amount );
      p -> buffs_clearcasting = 0;
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
    player_crit += ( p -> buffs_combustion * 0.10 );
  }
  if( school == SCHOOL_FROST ||
      school == SCHOOL_FROSTFIRE )
  {
    if( sim -> target -> debuffs.frozen )
    {
      player_crit += p -> talents.shatter * 0.10;
    }
    else if( p -> talents.fingers_of_frost )
    {
      if( p -> buffs_fingers_of_frost || ( p -> buffs_shatter_combo && time_to_execute == 0 ) )
      {
	player_crit += p -> talents.shatter * 0.10;
      }
      p -> uptimes_fingers_of_frost -> update( p -> buffs_fingers_of_frost );
    }
  }

  if( p -> buffs_arcane_power ) player_multiplier *= 1.30;

  if( p -> talents.torment_the_weak )
  {
    target_t* t = sim -> target;
    if( t -> debuffs.frozen || t -> debuffs.slowed )
    {
      player_multiplier *= 1.0 + p -> talents.torment_the_weak * 0.02;
    }
  }

  if( p -> talents.playing_with_fire )
  {
    player_multiplier *= 1.0 + p -> talents.playing_with_fire * 0.01;
  }

  if( p -> buffs_arcane_potency )
  {
    player_crit += p -> talents.arcane_potency * ( sim_t::WotLK ? 0.15 : 0.10 );
  }

  if( p -> buffs_molten_armor )
  {
    player_crit += p -> glyphs.molten_armor ? 0.05 : 0.03;
  }

  if( sim -> debug ) 
    report_t::log( sim, "mage_spell_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f", 
		   name(), player_hit, player_crit, player_power, player_penetration );
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
      { "rank",    OPT_INT8, &rank_index },
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
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 0; 
    may_crit          = true;
    dd_power_mod      = (3.0/3.5); 
    cooldown          = 3.0;

    base_cost         = rank -> cost;
    base_cost        *= 1.0 - p -> talents.frost_channeling * ( sim_t::WotLK ? (0.1/3) : 0 );
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_hit         += p -> talents.arcane_focus * ( sim_t::WotLK ? 0.01 : 0.02 );
    base_penetration += p -> talents.arcane_subtlety * 5;
    base_crit_bonus  *= 1.0 + p -> talents.spell_power * 0.25;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    if( result_is_hit() ) trigger_missile_barrage( this );
  }
};

// Arcane Blast Spell =======================================================

struct arcane_blast_t : public mage_spell_t
{
  int8_t ap_burn;
  int8_t reset_buff;
  int8_t max_buff;

  arcane_blast_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "arcane_blast", player, SCHOOL_ARCANE, TREE_ARCANE ), ap_burn(0), reset_buff(0), max_buff(0)
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "rank",    OPT_INT8, &rank_index },
      { "ap_burn", OPT_INT8, &ap_burn    },
      { "reset",   OPT_INT8, &reset_buff },
      { "max",     OPT_INT8, &max_buff   },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 4, 912, 1058, 0, 0.09 },
      { 76, 3, 805,  935, 0, 0.09 },
      { 71, 2, 690,  800, 0, 0.09 },
      { 64, 1, 648,  752, 0, 195  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 2.5; 
    may_crit          = true;
    dd_power_mod      = (2.5/3.5); 

    base_cost         = rank -> cost;
    base_cost        *= 1.0 - p -> talents.frost_channeling * ( sim_t::WotLK ? (0.1/3) : 0 );
    base_multiplier  *= 1.0 + p -> talents.spell_impact * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_impact * 0.02;
    base_hit         += p -> talents.arcane_focus * ( sim_t::WotLK ? 0.01 : 0.02 );
    base_penetration += p -> talents.arcane_subtlety * 5;
    dd_power_mod     += p -> talents.arcane_empowerment * 0.03;
    base_crit_bonus  *= 1.0 + p -> talents.spell_power * 0.25;

    if( p -> gear.tier5_2pc ) base_multiplier *= 1.20;
  }

  virtual double execute_time()
  {
    if( ! sim_t::WotLK )
    {
      mage_t* p = player -> cast_mage();
      base_execute_time = 2.5;
      base_execute_time -= p -> buffs_arcane_blast * ( 1.0 / 3.0 );
    }
    return mage_spell_t::execute_time();
  }
  
  virtual double cost()
  {
    mage_t* p = player -> cast_mage();
    double c = mage_spell_t::cost();
    if( c != 0 )
    {
      if( p -> buffs_arcane_blast ) c += base_cost * 0.75 * p -> buffs_arcane_blast;
      if( p -> gear.tier5_2pc     ) c += base_cost * 0.20;
    }
    return c;
  }

  virtual void execute()
  {
    mage_spell_t::execute(); 

    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
      {
	name = "Arcane Blast Acceleration Expiration";
	sim -> add_event( this, sim_t::WotLK ? 3.0 : 8.0 );
      }
      virtual void execute()
      {
	mage_t* p = player -> cast_mage();
	p -> aura_loss( "Arcane Blast Acceleration" );
	p -> buffs_arcane_blast = 0;
	p -> expirations_arcane_blast = 0;
      }
    };

    mage_t* p = player -> cast_mage();

    for( int i=0; i < 4; i++ ) 
    {
      p -> uptimes_arcane_blast[ i ] -> update( i == p -> buffs_arcane_blast );
    }

    if( p -> buffs_arcane_blast < 3 )
    {
      p -> buffs_arcane_blast++;
    }

    event_t*& e = p -> expirations_arcane_blast;

    if( e )
    {
      e -> reschedule( sim_t::WotLK ? 3.0 : 8.0 );
    }
    else
    {
      e = new expiration_t( sim, p );
    }
  }

  virtual void player_buff()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::player_buff();
    if( sim_t::WotLK ) 
    {
      player_multiplier *= 1.0 + p -> buffs_arcane_blast * 0.25;
    }
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if( ! mage_spell_t::ready() )
      return false;

    if( ap_burn )
      return( p -> buffs_arcane_power );

    if( max_buff > 0 )
      if( p -> buffs_arcane_blast >= max_buff )
	return false;

    if( reset_buff > 0 )
      if( p -> buffs_arcane_blast >= reset_buff )
	if( p -> expirations_arcane_blast -> occurs() >= ( sim -> current_time + execute_time() ) )
	  return false;

    return true;
  }
};

// Arcane Missiles Spell ====================================================

struct arcane_missiles_t : public mage_spell_t
{
  int8_t barrage;
  int8_t clearcast;
  int8_t ab_filler;

  arcane_missiles_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "arcane_missiles", player, SCHOOL_ARCANE, TREE_ARCANE ), barrage(0), clearcast(0), ab_filler(0)
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "rank",      OPT_INT8, &rank_index },
      { "barrage",   OPT_INT8, &barrage    },
      { "clearcast", OPT_INT8, &clearcast  },
      { "ab_filler", OPT_INT8, &ab_filler  },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 79, 13, 360, 260, 0, 0.34 },
      { 75, 12, 320, 320, 0, 0.34 },
      { 70, 11, 280, 280, 0, 785  },
      { 69, 10, 260, 260, 0, 740  },
      { 63,  9, 240, 240, 0, 685  },
      { 60,  8, 230, 230, 0, 655  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );

    base_duration = 5.0; 
    num_ticks     = 5; 
    may_crit      = true; 
    channeled     = true;
    dd_power_mod  = (1.0/3.5); // bonus per missle

    base_cost         = rank -> cost;
    base_cost        *= 1.0 + p -> talents.empowered_arcane_missiles * 0.02;
    base_cost        *= 1.0 - p -> talents.frost_channeling * ( sim_t::WotLK ? (0.1/3) : 0 );
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_hit         += p -> talents.arcane_focus * ( sim_t::WotLK ? 0.01 : 0.02 );
    base_penetration += p -> talents.arcane_subtlety * 5;
    dd_power_mod     += p -> talents.empowered_arcane_missiles * 0.03; // bonus per missle
    dd_power_mod     += p -> talents.arcane_empowerment * 0.03;        // bonus per missle
    base_crit_bonus  *= 1.0 + p -> talents.spell_power * 0.25;

    if( p -> gear.tier6_4pc ) base_multiplier *= 1.05;
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
    schedule_tick();
    update_ready();
    dd = 0;
    update_stats( DMG_DIRECT );
    trigger_arcane_concentration( this );
    p -> action_finish( this );
  }

  virtual void tick() 
  {
    mage_t* p = player -> cast_mage();

    if( sim -> debug ) report_t::log( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );

    may_resist = false;
    target_debuff( DMG_DIRECT );
    calculate_result();
    may_resist = true;

    if( result_is_hit() )
    {
      calculate_damage();
      adjust_dd_for_result();
      p -> action_hit( this );
      if( dd > 0 )
      {
	dot_tick = dd;
	assess_damage( dot_tick, DMG_OVER_TIME );
      }
      if( result == RESULT_CRIT )
      {
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

    if( current_tick == num_ticks ) 
    {
      clear_arcane_potency( this );
      p -> buffs_missile_barrage = 0;
    }
  }

  virtual double duration()
  {
    mage_t* p = player -> cast_mage();
    double d = mage_spell_t::duration();
    if( p -> buffs_missile_barrage ) d /= 2;
    return d;
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if( ! mage_spell_t::ready() )
      return false;

    if( barrage )
      if( ! sim -> time_to_think( p -> buffs_missile_barrage ) )
	return false;

    if( clearcast )
      if( cost() != 0 || ! sim -> time_to_think( p -> buffs_clearcasting ) )
	return false;

    if( ab_filler )
      if( ! arcane_blast_filler( this ) )
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
      p -> buffs_arcane_power = 1;
      sim -> add_event( this, p -> glyphs.arcane_power ? 18.0 : 15.0 );
    }
    virtual void execute()
    {
      mage_t* p = player -> cast_mage();
      p -> aura_loss( "Arcane Power" );
      p -> buffs_arcane_power = 0;
    }
  };
   
  arcane_power_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "arcane_power", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();
    assert( p -> talents.arcane_power );
    trigger_gcd = 0;  
    cooldown = 180.0;
    cooldown -= p -> talents.arcane_flows * 30.0;
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    new expiration_t( sim, player );
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
      if( sim -> log ) report_t::log( sim, "Target %s gains Slowed", sim -> target -> name() );
      sim -> target -> debuffs.slowed = 1;
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      if( sim -> log ) report_t::log( sim, "Target %s loses Slowed", sim -> target -> name() );
      sim -> target -> debuffs.slowed = 0;
    }
  };
   
  slow_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "slow", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();
    assert( p -> talents.slow );
    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.20;
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    new expiration_t( sim );
  }

  virtual bool ready()
  {
    if( ! mage_spell_t::ready() )
      return false;

    return( sim -> target -> debuffs.slowed == 0 );
  }
};

// Focus Magic Spell ========================================================

struct focus_magic_t : public mage_spell_t
{
  focus_magic_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "focus_magic", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();
    assert( p -> talents.focus_magic );

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 7, 0, 0, 150, 1000 },
      { 70, 6, 0, 0,  80,  535 },
      { 60, 5, 0, 0,  60,  400 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );

    base_cost  = rank -> cost;
    base_cost *= 1.0 - p -> talents.frost_channeling * ( sim_t::WotLK ? (0.1/3) : 0 );
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    if( sim -> log ) report_t::log( sim, "Target %s gains Focus Magic", sim -> target -> name() );
    consume_resource();
    sim -> target -> debuffs.focus_magic = rank -> dot;
    sim -> target -> debuffs.focus_magic_charges = 50;
  }

  virtual bool ready()
  {
    if( ! mage_spell_t::ready() )
      return false;

    return( sim -> target -> debuffs.focus_magic_charges == 0 );
  }
};

// Evocation Spell ==========================================================

struct evocation_t : public mage_spell_t
{
  evocation_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "evocation", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();

    base_duration = 8.0; 
    num_ticks = 4; 
    channeled = true;  
    cooldown = 480;
    harmful = false;

    if( p -> gear.tier6_2pc ) 
    {
      base_duration += 2.0;
      num_ticks++;
    }

    assert( p -> active_evocation == 0 );
    p -> active_evocation = this;
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
	     player -> resource_max    [ RESOURCE_MANA ] ) < 0.20;
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

    cooldown = 180.0;
    cooldown -= p -> talents.arcane_flows * 30.0;
    if( p -> gear.tier4_4pc ) cooldown -= 24.0;

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
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    player -> aura_gain( "Presence of Mind" );
    fast_action -> stats -> adjust_for_lost_time = true;
    fast_action -> execute();
    player -> aura_loss( "Presence of Mind" );
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
  int8_t ab_filler;
  int8_t brain_freeze;

  fire_ball_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "fire_ball", player, SCHOOL_FIRE, TREE_FIRE ), ab_filler(0), brain_freeze(0)
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "rank",         OPT_INT8, &rank_index   },
      { "ab_filler",    OPT_INT8, &ab_filler    },
      { "brain_freeze", OPT_INT8, &brain_freeze },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 78, 16, 888, 1132, 0, 0.21 },
      { 74, 15, 783,  997, 0, 0.21 },
      { 70, 14, 717,  913, 0, 0.21 },
      { 66, 13, 633,  805, 0, 425  },
      { 60, 12, 596,  760, 0, 410  },
      { 60, 11, 561,  715, 0, 395  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 3.5; 
    may_crit          = true; 
    dd_power_mod      = 1.0; 
      
    base_cost          = rank -> cost;
    base_cost         *= 1.0 - p -> talents.elemental_precision * 0.01;
    base_cost         *= 1.0 - p -> talents.pyromaniac * 0.01;
    base_cost         *= 1.0 - p -> talents.frost_channeling * ( sim_t::WotLK ? (0.1/3) : 0 );
    base_execute_time -= p -> talents.improved_fire_ball * 0.1;
    base_multiplier   *= 1.0 + p -> talents.fire_power * 0.02;
    base_multiplier   *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit         += p -> talents.arcane_instability * 0.01;
    base_crit         += p -> talents.critical_mass * 0.01;
    base_crit         += p -> talents.pyromaniac * 0.01;
    base_hit          += p -> talents.elemental_precision * 0.01;
    base_penetration  += p -> talents.arcane_subtlety * 5;
    dd_power_mod      += p -> talents.empowered_fire_ball * ( sim_t::WotLK ? 0.05 : 0.03 );
    base_crit_bonus   *= 1.0 + ( p -> talents.spell_power * 0.25 +
				 p -> talents.burnout     * 0.10 );

    if( p -> gear.tier6_4pc   ) base_multiplier *= 1.05;
    if( p -> glyphs.fire_ball ) base_multiplier *= 1.05;
  }

  virtual double cost()
  {
    mage_t* p = player -> cast_mage();
    if( p -> buffs_brain_freeze ) return 0;
    return mage_spell_t::cost();
  }

  virtual double execute_time()
  {
    mage_t* p = player -> cast_mage();
    if( p -> buffs_brain_freeze ) return 0;
    return mage_spell_t::execute_time();
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    if( result_is_hit() ) trigger_missile_barrage( this );
    p -> buffs_brain_freeze = 0;
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if( ! mage_spell_t::ready() )
      return false;

    if( ab_filler )
      if( ! arcane_blast_filler( this ) )
	return false;

    if( brain_freeze )
      if( ! sim -> time_to_think( p -> buffs_brain_freeze ) )
	return false;

    return true;
  }
};

// Fire Blast Spell ========================================================

struct fire_blast_t : public mage_spell_t
{
  int8_t ab_filler;
  fire_blast_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "fire_blast", player, SCHOOL_FIRE, TREE_FIRE ), ab_filler(0)
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "rank",      OPT_INT8, &rank_index },
      { "ab_filler", OPT_INT8, &ab_filler  },
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
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 0; 
    may_crit          = true; 
    cooldown          = 8.0;
    dd_power_mod      = (1.5/3.5); 
      
    base_cost         = rank -> cost;
    base_cost        *= 1.0 - p -> talents.elemental_precision * 0.01;
    base_cost        *= 1.0 - p -> talents.pyromaniac * 0.01;
    base_cost        *= 1.0 - p -> talents.frost_channeling * ( sim_t::WotLK ? (0.1/3) : 0 );
    cooldown         -= p -> talents.improved_fire_blast * 0.5;
    base_multiplier  *= 1.0 + p -> talents.fire_power * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_multiplier  *= 1.0 + p -> talents.spell_impact * 0.02;
    base_crit        += p -> talents.incineration * 0.02;
    base_crit        += p -> talents.critical_mass * 0.01;
    base_crit        += p -> talents.pyromaniac * 0.01;
    base_hit         += p -> talents.elemental_precision * 0.01;
    base_penetration += p -> talents.arcane_subtlety * 5;
    base_crit_bonus  *= 1.0 + ( p -> talents.spell_power * 0.25 +
				p -> talents.burnout     * 0.10 );
  }

  virtual bool ready()
  {
    if( ! mage_spell_t::ready() )
      return false;

    if( ab_filler )
      if( ! arcane_blast_filler( this ) )
	return false;

    return true;
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
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 3, 690, 690, 1380, 0.31 },
      { 73, 2, 512, 512, 1024, 0.31 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 0; 
    base_duration     = 12.0; 
    num_ticks         = 4; 
    dd_power_mod      = 0.40; 
    dot_power_mod     = 0.80;
    may_crit          = true;

    base_cost         = rank -> cost;
    base_cost        *= 1.0 - p -> talents.elemental_precision * 0.01;
    base_cost        *= 1.0 - p -> talents.pyromaniac * 0.01;
    base_cost        *= 1.0 - p -> talents.frost_channeling * ( sim_t::WotLK ? (0.1/3) : 0 );
    base_multiplier  *= 1.0 + p -> talents.fire_power * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.critical_mass * 0.01;
    base_crit        += p -> talents.pyromaniac * 0.01;
    base_crit        += p -> talents.world_in_flames * 0.02;
    base_hit         += p -> talents.elemental_precision * 0.01;
    base_penetration += p -> talents.arcane_subtlety * 5;
    base_crit_bonus  *= 1.0 + ( p -> talents.spell_power * 0.25 +
				p -> talents.burnout     * 0.10 );
  }

  // Odd thing to handle: The direct-damage comes at the last tick instead of the beginning of the spell.

  virtual void last_tick()
  {
    target_debuff( DMG_DIRECT );
    calculate_result();
    if( result_is_hit() )
    {
      calculate_damage();
      dd = explosion;
      adjust_dd_for_result();
      if( dd > 0 )
      {
	dot_tick = dd;
	assess_damage( dot_tick, DMG_OVER_TIME );
	if( result == RESULT_CRIT ) trigger_ignite( this );
      }
    }
    update_stats( DMG_OVER_TIME );
    mage_spell_t::last_tick();
  }

  virtual void calculate_damage()
  {
    mage_spell_t::calculate_damage();
    explosion = dd;
    dd = 0;
  }
};

// Pyroblast Spell ========================================================

struct pyroblast_t : public mage_spell_t
{
  int8_t hot_streak;

  pyroblast_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "pyroblast", player, SCHOOL_FIRE, TREE_FIRE ), hot_streak(0)
  {
    mage_t* p = player -> cast_mage();
    assert( p -> talents.pyroblast );

    option_t options[] =
    {
      { "rank",       OPT_INT8, &rank_index },
      { "hot_streak", OPT_INT8, &hot_streak },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 77, 12, 1190, 1510, 452, 0.22 },
      { 73, 11, 1014, 1286, 384, 0.22 },
      { 70, 10,  939, 1191, 356, 500  },
      { 66,  9,  846, 1074, 312, 460  },
      { 60,  8,  708,  898, 268, 440  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 6.0; 
    base_duration     = 12.0; 
    num_ticks         = 6; 
    may_crit          = true; 
    dd_power_mod      = 1.0; 
    dot_power_mod     = 0.71;

    base_cost         = rank -> cost;
    base_cost        *= 1.0 - p -> talents.elemental_precision * 0.01;
    base_cost        *= 1.0 - p -> talents.pyromaniac * 0.01;
    base_cost        *= 1.0 - p -> talents.frost_channeling * ( sim_t::WotLK ? (0.1/3) : 0 );
    base_multiplier  *= 1.0 + p -> talents.fire_power * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.critical_mass * 0.01;
    base_crit        += p -> talents.pyromaniac * 0.01;
    base_hit         += p -> talents.elemental_precision * 0.01;
    base_penetration += p -> talents.arcane_subtlety * 5;
    base_crit_bonus  *= 1.0 + ( p -> talents.spell_power * 0.25 +
				p -> talents.burnout     * 0.10 );
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    if( p -> buffs_hot_streak_pyroblast ) p -> procs_hot_streak_pyroblast -> occur();
    p -> buffs_hot_streak_pyroblast = 0;
  }

  virtual double execute_time()
  {
    mage_t* p = player -> cast_mage();
    if( p -> buffs_hot_streak_pyroblast ) return 0;
    return mage_spell_t::execute_time();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if( ! mage_spell_t::ready() )
      return false;

    if( hot_streak )
      if( ! sim -> time_to_think( p -> buffs_hot_streak_pyroblast ) )
	return false;

    return true;
  }
};

// Scorch Spell ==========================================================

struct scorch_t : public mage_spell_t
{
  int8_t increment;
  int8_t debuff;
  int8_t ab_filler;

  scorch_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "scorch", player, SCHOOL_FIRE, TREE_FIRE ), debuff( 0 ), ab_filler( 0 )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "rank",      OPT_INT8, &rank_index },
      { "debuff",    OPT_INT8, &debuff     },
      { "ab_filler", OPT_INT8, &ab_filler  },
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
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 1.5; 
    may_crit          = true;
    dd_power_mod      = (1.5/3.5); 
      
    base_cost         = rank -> cost;
    base_cost        *= 1.0 - p -> talents.elemental_precision * 0.01;
    base_cost        *= 1.0 - p -> talents.pyromaniac * 0.01;
    base_cost        *= 1.0 - p -> talents.frost_channeling * ( sim_t::WotLK ? (0.1/3) : 0 );
    base_multiplier  *= 1.0 + p -> talents.fire_power * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.incineration * 0.02;
    base_crit        += p -> talents.critical_mass * 0.01;
    base_crit        += p -> talents.pyromaniac * 0.01;
    base_hit         += p -> talents.elemental_precision * 0.01;
    base_penetration += p -> talents.arcane_subtlety * 5;
    base_crit_bonus  *= 1.0 + ( p -> talents.spell_power * 0.25 +
				p -> talents.burnout     * 0.10 );

    if( debuff ) assert( p -> talents.improved_scorch );

    increment = sim_t::WotLK ? 2 : 3;
  }

  virtual void execute()
  {
    mage_spell_t::execute(); 
    mage_t* p = player -> cast_mage();
    if( result_is_hit() && p -> talents.improved_scorch )
    {
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

      if( rand_t::roll( p -> talents.improved_scorch * (1.0/3.0) ) )
      {
	target_t* t = sim -> target;

	if( t -> debuffs.improved_scorch < ( 5 * increment ) ) 
	{
	  if( p -> glyphs.improved_scorch )
	  {
	    t -> debuffs.improved_scorch = 5 * increment;
	  }
	  else
	  {
	    t -> debuffs.improved_scorch += increment;
	  }
	  if( sim -> log ) report_t::log( sim, "%s gains Improved Scorch %d", t -> name(), t -> debuffs.improved_scorch / increment );
	}

	event_t*& e = t -> expirations.improved_scorch;
    
	if( e )
	{
	  e -> reschedule( 30.0 );
	}
	else
	{
	  e = new expiration_t( sim );
	}
      }
    }
  }

  virtual bool ready()
  {
    if( ! mage_spell_t::ready() )
      return false;

    if( debuff )
    {
      target_t* t = sim -> target;

      if( t -> debuffs.improved_scorch < ( 5 * increment ) )
	return true;

      event_t* e = t -> expirations.improved_scorch;

      if( e && sim -> current_time < ( e -> occurs() - 6.0 ) )
	return false;
    }

    if( ab_filler )
      if( ! arcane_blast_filler( this ) )
	return false;

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
    p -> buffs_combustion = 0;
    p -> buffs_combustion_crits = 3;
    update_ready();
  }
};

// Frost Bolt Spell =========================================================

struct frost_bolt_t : public mage_spell_t
{
  int8_t ab_filler;

  frost_bolt_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "frost_bolt", player, SCHOOL_FROST, TREE_FROST ), ab_filler(0)
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "rank",      OPT_INT8, &rank_index },
      { "ab_filler", OPT_INT8, &ab_filler  },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 79, 16, 799, 861, 0, 0.15 },
      { 75, 15, 702, 758, 0, 0.15 },
      { 70, 14, 630, 680, 0, 0.15 },
      { 68, 13, 597, 644, 0, 330  },
      { 62, 12, 522, 563, 0, 300  },
      { 60, 11, 515, 555, 0, 290  },
      { 56, 10, 429, 463, 0, 260  },
      { 50, 9,  353, 383, 0, 225  },
      { 44, 8,  292, 316, 0, 195  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time   = 3.0; 
    may_crit         = true; 
    dd_power_mod = (3.0/3.5); 
      
    base_cost          = rank -> cost;
    base_cost         *= 1.0 - p -> talents.elemental_precision * 0.01;
    base_cost         *= 1.0 - p -> talents.frost_channeling * ( sim_t::WotLK ? (0.1/3) : 0.05 );
    base_execute_time -= p -> talents.improved_frost_bolt * 0.1;
    base_multiplier   *= 1.0 + p -> talents.piercing_ice * 0.02;
    base_multiplier   *= 1.0 + p -> talents.arctic_winds * 0.01;
    base_multiplier   *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_multiplier   *= 1.0 + p -> talents.chilled_to_the_bone * 0.01;
    base_crit         += p -> talents.arcane_instability * 0.01;
    base_crit         += p -> talents.empowered_frost_bolt * ( sim_t::WotLK ? 0.02 : 0.01 );
    base_hit          += p -> talents.elemental_precision * 0.01;
    base_penetration  += p -> talents.arcane_subtlety * 5;
    dd_power_mod      += p -> talents.empowered_frost_bolt * ( sim_t::WotLK ? 0.05 : 0.02 );
    base_crit_bonus   *= 1.0 + ( p -> talents.ice_shards  * 0.20 +
				 p -> talents.spell_power * 0.25 );

    if( p -> gear.tier6_4pc    ) base_multiplier *= 1.05;
    if( p -> glyphs.frost_bolt ) base_multiplier *= 1.05;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    if( result_is_hit() ) trigger_missile_barrage( this );
  }

  virtual bool ready()
  {
    if( ! mage_spell_t::ready() )
      return false;

    if( ab_filler )
      if( ! arcane_blast_filler( this ) )
	return false;

    return true;
  }
};

// Ice Lance Spell =========================================================

struct ice_lance_t : public mage_spell_t
{
  int8_t frozen;
  int8_t fb_priority;

  ice_lance_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "ice_lance", player, SCHOOL_FROST, TREE_FROST ), frozen(0), fb_priority(0)
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "rank",        OPT_INT8, &rank_index  },
      { "frozen",      OPT_INT8, &frozen      },
      { "fb_priority", OPT_INT8, &fb_priority },
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
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time   = 0.0; 
    may_crit         = true; 
    dd_power_mod = (0.5/3.5); 
      
    base_cost         = rank -> cost;
    base_cost        *= 1.0 - p -> talents.elemental_precision * 0.01;
    base_cost        *= 1.0 - p -> talents.frost_channeling * ( sim_t::WotLK ? (0.1/3) : 0.05 );
    base_multiplier  *= 1.0 + p -> talents.piercing_ice * 0.02;
    base_multiplier  *= 1.0 + p -> talents.spell_impact * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_multiplier  *= 1.0 + p -> talents.arctic_winds * 0.01;
    base_multiplier  *= 1.0 + p -> talents.chilled_to_the_bone * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_hit         += p -> talents.elemental_precision * 0.01;
    base_penetration += p -> talents.arcane_subtlety * 5;
    base_crit_bonus  *= 1.0 + ( p -> talents.ice_shards  * 0.20  +
				p -> talents.spell_power * 0.25 );
  }

  virtual void player_buff()
  {
    mage_t* p = player -> cast_mage();

    mage_spell_t::player_buff();

    if( p -> buffs_shatter_combo       ||
        p -> buffs_fingers_of_frost    ||
	sim -> target -> debuffs.frozen ) 
    {
      player_multiplier *= 3.0;
    }
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if( ! mage_spell_t::ready() )
      return false;

    if( frozen )
    {
      if( ! p -> buffs_shatter_combo &&
	  ! p -> buffs_fingers_of_frost &&
	  ! sim -> time_to_think( sim -> target -> debuffs.frozen ) )
	return false;
    }
    
    if( fb_priority )
    {
      double fb_execute_time = sim -> current_time + 2.5 * haste();

      if( sim -> time_to_think( sim -> target -> debuffs.frozen ) )
	if( fb_execute_time < sim -> target -> expirations.frozen -> occurs() )
	  return false;

      if( p -> buffs_fingers_of_frost )
	return false;
    }

    return true;
  }
};

// Deep Freeze Spell =======================================================

struct deep_freeze_t : public mage_spell_t
{
  int8_t fb_priority;

  deep_freeze_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "deep_freeze", player, SCHOOL_FROST, TREE_FROST ), fb_priority(0)
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "fb_priority", OPT_INT8, &fb_priority },
      { "rank",        OPT_INT8, &rank_index  },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 4, 1319, 1531, 0, 0.08 },
      { 72, 3, 1018, 1182, 0, 0.08 },
      { 66, 2,  787,  913, 0, 0.08 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 0.0; 
    may_crit          = true; 
    dd_power_mod      = (1.0/3.5); 
    cooldown          = 30;
    base_cost         = rank -> cost;
      
    cooldown         *= 1.0 - p -> talents.cold_as_ice * 0.10;
    base_cost        *= 1.0 - p -> talents.elemental_precision * 0.01;
    base_cost        *= 1.0 - p -> talents.frost_channeling * ( sim_t::WotLK ? (0.1/3) : 0.05 );
    base_multiplier  *= 1.0 + p -> talents.piercing_ice * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_multiplier  *= 1.0 + p -> talents.arctic_winds * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_hit         += p -> talents.elemental_precision * 0.01;
    base_penetration += p -> talents.arcane_subtlety * 5;
    base_crit_bonus  *= 1.0 + ( p -> talents.ice_shards  * 0.20 +
				p -> talents.spell_power * 0.25 );
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if( ! mage_spell_t::ready() )
      return false;

    if( ! p -> buffs_shatter_combo    &&
	! p -> buffs_fingers_of_frost &&
	! sim -> time_to_think( sim -> target -> debuffs.frozen ) )
      return false;

    if( fb_priority )
    {
      double fb_execute_time = sim -> current_time + 2.5 * haste();

      if( sim -> time_to_think( sim -> target -> debuffs.frozen ) )
	if( fb_execute_time < sim -> target -> expirations.frozen -> occurs() )
	  return false;

      if( p -> buffs_fingers_of_frost )
	return false;
    }

    return true;
  }
};

// Frostfire Bolt Spell ======================================================

struct frostfire_bolt_t : public mage_spell_t
{
  int8_t dot_wait;

  frostfire_bolt_t( player_t* player, const std::string& options_str ) : 
    mage_spell_t( "frostfire_bolt", player, SCHOOL_FROSTFIRE, TREE_FROST ), dot_wait(0)
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { "rank",     OPT_INT8, &rank_index },
      { "dot_wait", OPT_INT8, &dot_wait   },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 2, 722, 838, 90, 0.16 },
      { 75, 1, 629, 731, 60, 0.16 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 3.0; 
    may_crit          = true; 
    dd_power_mod      = (3.0/3.5); 

    if( dot_wait )
    {
      base_duration = 9.0;
      num_ticks     = 3;
      dot_power_mod = 0.05;
    }
      
    base_cost         = rank -> cost;
    base_cost         *= 1.0 - p -> talents.elemental_precision * 0.01;
    base_cost         *= 1.0 - p -> talents.pyromaniac * 0.01;
    base_cost         *= 1.0 - p -> talents.frost_channeling * ( sim_t::WotLK ? (0.1/3) : 0.05 );
    base_multiplier   *= 1.0 + p -> talents.fire_power * 0.02;
    base_multiplier   *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_multiplier   *= 1.0 + p -> talents.piercing_ice * 0.02;
    base_multiplier   *= 1.0 + p -> talents.arctic_winds * 0.01;
    base_multiplier   *= 1.0 + p -> talents.chilled_to_the_bone * 0.01;
    base_crit         += p -> talents.arcane_instability * 0.01;
    base_crit         += p -> talents.critical_mass * 0.01;
    base_crit         += p -> talents.pyromaniac * 0.01;
    base_hit          += p -> talents.elemental_precision * 0.01;
    base_penetration  += p -> talents.arcane_subtlety * 5;
    base_crit_bonus   *= 1.0 + ( p -> talents.spell_power * 0.25 +
				 p -> talents.burnout     * 0.10 +
				 p -> talents.ice_shards  * 0.20 );
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    if( result_is_hit() ) trigger_missile_barrage( this );
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
      p -> buffs_icy_veins = 1;
      sim -> add_event( this, 20.0 );
    }
    virtual void execute()
    {
      mage_t* p = player -> cast_mage();
      p -> aura_loss( "Icy Veins" );
      p -> buffs_icy_veins = 0;
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
    if( sim_t::WotLK ) cooldown *= 1.0 - p -> talents.ice_floes * ( 0.20 / 3.0 );
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    new expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if( ! mage_spell_t::ready() )
      return false;

    return player -> cast_mage() -> buffs_icy_veins == 0;
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
    cooldown *= 1.0 - ( sim_t::WotLK ? p -> talents.cold_as_ice : p -> talents.ice_floes ) * 0.10;
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
    p -> buffs_molten_armor = 1;
  }

  virtual bool ready()
  {
    return( player -> cast_mage() -> buffs_molten_armor == 0 );
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
    p -> buffs_mage_armor = 1;
  }

  virtual bool ready()
  {
    return( player -> cast_mage() -> buffs_mage_armor == 0 );
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
    assert( p -> talents.water_elemental );
    
    base_cost  = 492;
    base_cost *= 1.0 - p -> talents.frost_channeling * ( sim_t::WotLK ? (0.1/3) : 0.05 );
    cooldown   = p -> glyphs.water_elemental ? 150 : 180;
    cooldown  *= 1.0 - p -> talents.cold_as_ice * 0.10;
    harmful    = false;
  }

  virtual void execute() 
  {
    consume_resource();
    update_ready();
    new water_elemental_expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if( ! mage_spell_t::ready() )
      return false;

    return player -> cast_mage() -> active_water_elemental == 0;
  }
};

// mage_t::create_action ====================================================

action_t* mage_t::create_action( const std::string& name,
				 const std::string& options_str )
{
  if( name == "arcane_barrage"   ) return new        arcane_barrage_t( this, options_str );
  if( name == "arcane_blast"     ) return new          arcane_blast_t( this, options_str );
  if( name == "arcane_missiles"  ) return new       arcane_missiles_t( this, options_str );
  if( name == "arcane_power"     ) return new          arcane_power_t( this, options_str );
  if( name == "cold_snap"        ) return new             cold_snap_t( this, options_str );
  if( name == "combustion"       ) return new            combustion_t( this, options_str );
  if( name == "deep_freeze"      ) return new           deep_freeze_t( this, options_str );
  if( name == "evocation"        ) return new             evocation_t( this, options_str );
  if( name == "fire_ball"        ) return new             fire_ball_t( this, options_str );
  if( name == "fire_blast"       ) return new            fire_blast_t( this, options_str );
  if( name == "focus_magic"      ) return new           focus_magic_t( this, options_str );
  if( name == "frost_bolt"       ) return new            frost_bolt_t( this, options_str );
  if( name == "frostfire_bolt"   ) return new        frostfire_bolt_t( this, options_str );
  if( name == "ice_lance"        ) return new             ice_lance_t( this, options_str );
  if( name == "icy_veins"        ) return new             icy_veins_t( this, options_str );
  if( name == "living_bomb"      ) return new           living_bomb_t( this, options_str );
  if( name == "mage_armor"       ) return new            mage_armor_t( this, options_str );
  if( name == "molten_armor"     ) return new          molten_armor_t( this, options_str );
  if( name == "presence_of_mind" ) return new      presence_of_mind_t( this, options_str );
  if( name == "pyroblast"        ) return new             pyroblast_t( this, options_str );
  if( name == "scorch"           ) return new                scorch_t( this, options_str );
  if( name == "slow"             ) return new                  slow_t( this, options_str );
  if( name == "water_elemental"  ) return new water_elemental_spell_t( this, options_str );

  return 0;
}

// mage_t::create_pet ======================================================

pet_t* mage_t::create_pet( const std::string& pet_name )
{
  if( pet_name == "water_elemental" ) return new water_elemental_pet_t( sim, this, pet_name );

  return 0;
}

// mage_t::init_base =======================================================

void mage_t::init_base()
{
  // level 80 mage stats:  Str 35,  Agi 41,  Sta 60,  Int 179,  Spi 179,  Armor 82,  Health 4884,  Mana 5673,  Crit 1.98%

  attribute_base[ ATTR_STRENGTH  ] =  40;
  attribute_base[ ATTR_AGILITY   ] =  45;
  attribute_base[ ATTR_STAMINA   ] =  60;
  attribute_base[ ATTR_INTELLECT ] = 145;
  attribute_base[ ATTR_SPIRIT    ] = 155;

  attribute_multiplier_initial[ ATTR_INTELLECT ] *= 1.0 + talents.arcane_mind * 0.03;
  attribute_multiplier_initial[ ATTR_SPIRIT    ] *= 1.0 + talents.student_of_the_mind * ( 0.1 / 3.0 );

  base_spell_crit = 0.0125;
  initial_spell_crit_per_intellect = rating_t::interpolate( level, 0.01/60.0, 0.01/80.0, 0.01/166.6 );
  initial_spell_power_per_intellect = talents.mind_mastery * 0.05;

  base_attack_power = -10;
  base_attack_crit  = 0.03;
  initial_attack_power_per_strength = 1.0;
  initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/16.0, 0.01/24.9, 0.01/52.1 );

  // FIXME! Make this level-specific.
  resource_base[ RESOURCE_HEALTH ] = 3200;
  resource_base[ RESOURCE_MANA   ] = rating_t::interpolate( level, 1183, 2241, 3268 );

  health_per_stamina = 10;
  mana_per_intellect = 15;
}

// mage_t::reset ===========================================================

void mage_t::reset()
{
  player_t::reset();

  // Active
  active_evocation       = 0;
  active_water_elemental = 0;
  
  // Buffs
  buffs_arcane_blast           = 0;
  buffs_arcane_potency         = 0;
  buffs_arcane_power           = 0;
  buffs_brain_freeze           = 0;
  buffs_clearcasting           = 0;
  buffs_combustion             = 0;
  buffs_combustion_crits       = 0;
  buffs_fingers_of_frost       = 0;
  buffs_hot_streak             = 0;
  buffs_hot_streak_pyroblast   = 0;
  buffs_icy_veins              = 0;
  buffs_mage_armor             = 0;
  buffs_missile_barrage        = 0;
  buffs_molten_armor           = 0;
  buffs_shatter_combo          = 0;
  
  // Expirations
  expirations_arcane_blast = 0;
}

// mage_t::regen  ==========================================================

void mage_t::regen( double periodicity )
{
  double spirit_regen = periodicity * spirit_regen_per_second();

  if( buffs.innervate )
  {
    spirit_regen *= 4.0;
  }
  else if( recent_cast() )
  {
    double while_casting = talents.arcane_meditation * 0.10;
    if( buffs_mage_armor ) while_casting += glyphs.mage_armor ? 0.50 : 0.30;
    spirit_regen *= while_casting;
  }

  double mp5_regen = periodicity * mp5 / 5.0;

  resource_gain( RESOURCE_MANA, spirit_regen, gains.spirit_regen );
  resource_gain( RESOURCE_MANA,    mp5_regen, gains.mp5_regen    );

  if( buffs.replenishment )
  {
    double replenishment_regen = periodicity * resource_max[ RESOURCE_MANA ] * 0.005 / 1.0;

    resource_gain( RESOURCE_MANA, replenishment_regen, gains.replenishment );
  }

  if( buffs.water_elemental_regen )
  {
    double water_elemental_regen = periodicity * resource_max[ RESOURCE_MANA ] * 0.006 / 5.0;

    resource_gain( RESOURCE_MANA, water_elemental_regen, gains.water_elemental_regen );
  }

  uptimes_water_elemental -> update( active_water_elemental );
}

// mage_t::parse_talents =================================================

void mage_t::parse_talents( const std::string& talent_string )
{
  if( talent_string.size() == 67 )
  {
    talent_translation_t translation[] =
    {
      {  1,  &( talents.arcane_subtlety           ) },
      {  2,  &( talents.arcane_focus              ) },
      {  6,  &( talents.arcane_concentration      ) },
      {  8,  &( talents.arcane_impact             ) }, 
      { 12,  &( talents.arcane_meditation         ) },
      { 14,  &( talents.presence_of_mind          ) },
      { 15,  &( talents.arcane_mind               ) },
      { 17,  &( talents.arcane_instability        ) },
      { 18,  &( talents.arcane_potency            ) },
      { 19,  &( talents.empowered_arcane_missiles ) },
      { 20,  &( talents.arcane_power              ) },
      { 21,  &( talents.spell_power               ) },
      { 22,  &( talents.mind_mastery              ) },
      { 24,  &( talents.improved_fire_ball        ) },
      { 26,  &( talents.ignite                    ) },
      { 28,  &( talents.improved_fire_blast       ) },
      { 29,  &( talents.incineration              ) },
      { 31,  &( talents.pyroblast                 ) },
      { 33,  &( talents.improved_scorch           ) },
      { 35,  &( talents.master_of_elements        ) },
      { 36,  &( talents.playing_with_fire         ) },
      { 37,  &( talents.critical_mass             ) },
      { 40,  &( talents.fire_power                ) },
      { 41,  &( talents.pyromaniac                ) },
      { 42,  &( talents.combustion                ) },
      { 43,  &( talents.molten_fury               ) },
      { 44,  &( talents.empowered_fire_ball       ) },
      { 47,  &( talents.improved_frost_bolt       ) },
      { 48,  &( talents.elemental_precision       ) },
      { 49,  &( talents.ice_shards                ) },
      { 50,  &( talents.frostbite                 ) },
      { 53,  &( talents.piercing_ice              ) },
      { 54,  &( talents.icy_veins                 ) },
      { 57,  &( talents.frost_channeling          ) },
      { 58,  &( talents.shatter                   ) },
      { 60,  &( talents.cold_snap                 ) },
      { 62,  &( talents.ice_floes                 ) },
      { 63,  &( talents.winters_chill             ) },
      { 65,  &( talents.arctic_winds              ) },
      { 66,  &( talents.empowered_frost_bolt      ) },
      { 67,  &( talents.water_elemental           ) },
      { 0, NULL }
    };
    player_t::parse_talents( translation, talent_string );
  }
  else if( talent_string.size() == 86 )
  {
    talent_translation_t translation[] =
    {
      {  1,  &( talents.arcane_subtlety           ) },
      {  2,  &( talents.arcane_focus              ) },
      {  6,  &( talents.arcane_concentration      ) },
      {  8,  &( talents.spell_impact              ) }, 
      {  9,  &( talents.student_of_the_mind       ) }, 
      { 10,  &( talents.focus_magic               ) }, 
      { 13,  &( talents.arcane_meditation         ) },
      { 14,  &( talents.torment_the_weak          ) },
      { 16,  &( talents.presence_of_mind          ) },
      { 17,  &( talents.arcane_mind               ) },
      { 19,  &( talents.arcane_instability        ) },
      { 20,  &( talents.arcane_potency            ) },
      { 21,  &( talents.arcane_empowerment        ) },
      { 22,  &( talents.arcane_power              ) },
      { 24,  &( talents.arcane_flows              ) },
      { 25,  &( talents.mind_mastery              ) },
      { 26,  &( talents.slow                      ) },
      { 27,  &( talents.missile_barrage           ) },
      { 28,  &( talents.netherwind_presence       ) },
      { 29,  &( talents.spell_power               ) },
      { 30,  &( talents.arcane_barrage            ) },
      { 31,  &( talents.improved_fire_blast       ) },
      { 32,  &( talents.incineration              ) },
      { 33,  &( talents.improved_fire_ball        ) },
      { 34,  &( talents.ignite                    ) },
      { 36,  &( talents.world_in_flames           ) },
      { 39,  &( talents.pyroblast                 ) },
      { 40,  &( talents.burning_soul              ) },
      { 41,  &( talents.improved_scorch           ) },
      { 43,  &( talents.master_of_elements        ) },
      { 44,  &( talents.playing_with_fire         ) },
      { 45,  &( talents.critical_mass             ) },
      { 48,  &( talents.fire_power                ) },
      { 49,  &( talents.pyromaniac                ) },
      { 50,  &( talents.combustion                ) },
      { 51,  &( talents.molten_fury               ) },
      { 53,  &( talents.empowered_fire_ball       ) },
      { 56,  &( talents.hot_streak                ) },
      { 57,  &( talents.burnout                   ) },
      { 58,  &( talents.living_bomb               ) },
      { 60,  &( talents.improved_frost_bolt       ) },
      { 61,  &( talents.ice_floes                 ) },
      { 62,  &( talents.ice_shards                ) },
      { 63,  &( talents.frostbite                 ) },
      { 64,  &( talents.elemental_precision       ) },
      { 66,  &( talents.piercing_ice              ) },
      { 67,  &( talents.icy_veins                 ) },
      { 70,  &( talents.frost_channeling          ) },
      { 71,  &( talents.shatter                   ) },
      { 72,  &( talents.cold_snap                 ) },
      { 75,  &( talents.cold_as_ice               ) },
      { 76,  &( talents.winters_chill             ) },
      { 79,  &( talents.arctic_winds              ) },
      { 80,  &( talents.empowered_frost_bolt      ) },
      { 81,  &( talents.fingers_of_frost          ) },
      { 82,  &( talents.brain_freeze              ) },
      { 83,  &( talents.water_elemental           ) },
      { 84,  &( talents.improved_water_elemental  ) },
      { 85,  &( talents.chilled_to_the_bone       ) },
      { 86,  &( talents.deep_freeze               ) },
      { 0, NULL }
    };
    player_t::parse_talents( translation, talent_string );
  }
  else
  {
    fprintf( sim -> output_file, "Malformed Priest talent string.  Number encoding should have length 67 for Burning Crusade or 86 for Wrath of the Lich King.\n" );
    assert( 0 );
  }
}

// mage_t::parse_option  ==================================================

bool mage_t::parse_option( const std::string& name,
			   const std::string& value )
{
  option_t options[] =
  {
    { "arcane_barrage",            OPT_INT8,  &( talents.arcane_barrage            ) },
    { "arcane_concentration",      OPT_INT8,  &( talents.arcane_concentration      ) },
    { "arcane_empowerment",        OPT_INT8,  &( talents.arcane_empowerment        ) },
    { "arcane_flows",              OPT_INT8,  &( talents.arcane_flows              ) },
    { "arcane_focus",              OPT_INT8,  &( talents.arcane_focus              ) },
    { "arcane_impact",             OPT_INT8,  &( talents.arcane_impact             ) },
    { "arcane_instability",        OPT_INT8,  &( talents.arcane_instability        ) },
    { "arcane_meditation",         OPT_INT8,  &( talents.arcane_meditation         ) },
    { "arcane_mind",               OPT_INT8,  &( talents.arcane_mind               ) },
    { "arcane_potency",            OPT_INT8,  &( talents.arcane_potency            ) },
    { "arcane_power",              OPT_INT8,  &( talents.arcane_power              ) },
    { "arcane_subtlety",           OPT_INT8,  &( talents.arcane_subtlety           ) },
    { "arctic_winds",              OPT_INT8,  &( talents.arctic_winds              ) },
    { "brain_freeze",              OPT_INT8,  &( talents.brain_freeze              ) },
    { "burning_soul",              OPT_INT8,  &( talents.burning_soul              ) },
    { "burnout",                   OPT_INT8,  &( talents.burnout                   ) },
    { "chilled_to_the_bone",       OPT_INT8,  &( talents.chilled_to_the_bone       ) },
    { "cold_as_ice",               OPT_INT8,  &( talents.cold_as_ice               ) },
    { "cold_snap",                 OPT_INT8,  &( talents.cold_snap                 ) },
    { "combustion",                OPT_INT8,  &( talents.combustion                ) },
    { "critical_mass",             OPT_INT8,  &( talents.critical_mass             ) },
    { "deep_freeze",               OPT_INT8,  &( talents.deep_freeze               ) },
    { "elemental_precision",       OPT_INT8,  &( talents.elemental_precision       ) },
    { "empowered_arcane_missiles", OPT_INT8,  &( talents.empowered_arcane_missiles ) },
    { "empowered_fire_ball",       OPT_INT8,  &( talents.empowered_fire_ball       ) },
    { "empowered_frost_bolt",      OPT_INT8,  &( talents.empowered_frost_bolt      ) },
    { "fingers_of_frost",          OPT_INT8,  &( talents.fingers_of_frost          ) },
    { "fire_power",                OPT_INT8,  &( talents.fire_power                ) },
    { "focus_magic",               OPT_INT8,  &( talents.focus_magic               ) },
    { "frost_channeling",          OPT_INT8,  &( talents.frost_channeling          ) },
    { "frostbite",                 OPT_INT8,  &( talents.frostbite                 ) },
    { "hot_streak",                OPT_INT8,  &( talents.hot_streak                ) },
    { "ice_floes",                 OPT_INT8,  &( talents.ice_floes                 ) },
    { "ice_shards",                OPT_INT8,  &( talents.ice_shards                ) },
    { "ignite",                    OPT_INT8,  &( talents.ignite                    ) },
    { "improved_fire_ball",        OPT_INT8,  &( talents.improved_fire_ball        ) },
    { "improved_fire_blast",       OPT_INT8,  &( talents.improved_fire_blast       ) },
    { "improved_frost_bolt",       OPT_INT8,  &( talents.improved_frost_bolt       ) },
    { "improved_scorch",           OPT_INT8,  &( talents.improved_scorch           ) },
    { "improved_water_elemental",  OPT_INT8,  &( talents.improved_water_elemental  ) },
    { "incineration",              OPT_INT8,  &( talents.incineration              ) },
    { "living_bomb",               OPT_INT8,  &( talents.living_bomb               ) },
    { "master_of_elements",        OPT_INT8,  &( talents.master_of_elements        ) },
    { "mind_mastery",              OPT_INT8,  &( talents.mind_mastery              ) },
    { "missile_barrage",           OPT_INT8,  &( talents.missile_barrage           ) },
    { "molten_fury",               OPT_INT8,  &( talents.molten_fury               ) },
    { "netherwind_presence",       OPT_INT8,  &( talents.netherwind_presence       ) },
    { "piercing_ice",              OPT_INT8,  &( talents.piercing_ice              ) },
    { "playing_with_fire",         OPT_INT8,  &( talents.playing_with_fire         ) },
    { "presence_of_mind",          OPT_INT8,  &( talents.presence_of_mind          ) },
    { "pyroblast",                 OPT_INT8,  &( talents.pyroblast                 ) },
    { "pyromaniac",                OPT_INT8,  &( talents.pyromaniac                ) },
    { "shatter",                   OPT_INT8,  &( talents.shatter                   ) },
    { "slow",                      OPT_INT8,  &( talents.slow                      ) },
    { "spell_impact",              OPT_INT8,  &( talents.spell_impact              ) },
    { "spell_power",               OPT_INT8,  &( talents.spell_power               ) },
    { "student_of_the_mind",       OPT_INT8,  &( talents.student_of_the_mind       ) },
    { "torment_the_weak",          OPT_INT8,  &( talents.torment_the_weak          ) },
    { "water_elemental",           OPT_INT8,  &( talents.water_elemental           ) },
    { "winters_chill",             OPT_INT8,  &( talents.winters_chill             ) },
    { "winters_grasp",             OPT_INT8,  &( talents.winters_grasp             ) },
    { "world_in_flames",           OPT_INT8,  &( talents.world_in_flames           ) },
    // Glyphs
    { "glyph_arcane_power",        OPT_INT8,  &( glyphs.arcane_power               ) },
    { "glyph_fire_ball",           OPT_INT8,  &( glyphs.fire_ball                  ) },
    { "glyph_frost_bolt",          OPT_INT8,  &( glyphs.frost_bolt                 ) },
    { "glyph_improved_scorch",     OPT_INT8,  &( glyphs.improved_scorch            ) },
    { "glyph_mage_armor",          OPT_INT8,  &( glyphs.mage_armor                 ) },
    { "glyph_mana_gem",            OPT_INT8,  &( glyphs.mana_gem                   ) },
    { "glyph_molten_armor",        OPT_INT8,  &( glyphs.molten_armor               ) },
    { "glyph_water_elemental",     OPT_INT8,  &( glyphs.water_elemental            ) },
    { NULL, OPT_UNKNOWN }
  };
  
  if( name.empty() )
  {
    player_t::parse_option( std::string(), std::string() );
    option_t::print( sim, options );
    return false;
  }

  if( player_t::parse_option( name, value ) ) return true;

  return option_t::parse( sim, options, name, value );
}

// player_t::create_mage  ===================================================

player_t* player_t::create_mage( sim_t*       sim, 
				 std::string& name ) 
{
  return new mage_t( sim, name );
}
