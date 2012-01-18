// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Mage
// ==========================================================================

enum rotation_t { ROTATION_NONE=0, ROTATION_DPS, ROTATION_DPM, ROTATION_MAX };

struct mage_targetdata_t : public targetdata_t
{
  dot_t* dots_frostfire_bolt;
  dot_t* dots_ignite;
  dot_t* dots_living_bomb;
  dot_t* dots_pyroblast;

  mage_targetdata_t( player_t* source, player_t* target )
    : targetdata_t( source, target )
  {
  }
};

void register_mage_targetdata( sim_t* sim )
{
  player_type t = MAGE;
  typedef mage_targetdata_t type;

  REGISTER_DOT( frostfire_bolt );
  REGISTER_DOT( ignite );
  REGISTER_DOT( living_bomb );
  REGISTER_DOT( pyroblast );
}

struct mage_t : public player_t
{
  // Active
  spell_t* active_ignite;
  pet_t*   pet_water_elemental;
  pet_t*   pet_tier12_mirror_image;
  pet_t*   pet_mirror_image_3;

  // Buffs
  buff_t* buffs_arcane_blast;
  buff_t* buffs_arcane_missiles;
  buff_t* buffs_arcane_potency;
  buff_t* buffs_arcane_power;
  buff_t* buffs_brain_freeze;
  buff_t* buffs_clearcasting;
  buff_t* buffs_fingers_of_frost;
  buff_t* buffs_focus_magic_feedback;
  buff_t* buffs_frost_armor;
  buff_t* buffs_hot_streak;
  buff_t* buffs_hot_streak_crits;
  buff_t* buffs_icy_veins;
  buff_t* buffs_improved_mana_gem;
  buff_t* buffs_incanters_absorption;
  buff_t* buffs_invocation;
  buff_t* buffs_mage_armor;
  buff_t* buffs_missile_barrage;
  buff_t* buffs_molten_armor;
  buff_t* buffs_presence_of_mind;
  buff_t* buffs_tier13_2pc;

  // Cooldowns
  cooldown_t* cooldowns_deep_freeze;
  cooldown_t* cooldowns_early_frost;
  cooldown_t* cooldowns_evocation;
  cooldown_t* cooldowns_fire_blast;
  cooldown_t* cooldowns_mana_gem;
  cooldown_t* cooldowns_tier12_mirror_image;

  // Gains
  gain_t* gains_clearcasting;
  gain_t* gains_empowered_fire;
  gain_t* gains_evocation;
  gain_t* gains_frost_armor;
  gain_t* gains_mage_armor;
  gain_t* gains_mana_gem;
  gain_t* gains_master_of_elements;

  // Glyphs
  struct glyphs_t
  {
    // Prime
    glyph_t* arcane_barrage;
    glyph_t* arcane_blast;
    glyph_t* arcane_missiles;
    glyph_t* arcane_power;
    glyph_t* cone_of_cold;
    glyph_t* deep_freeze;
    glyph_t* fireball;
    glyph_t* frost_armor;
    glyph_t* frostbolt;
    glyph_t* frostfire;
    glyph_t* ice_lance;
    glyph_t* living_bomb;
    glyph_t* mage_armor;
    glyph_t* molten_armor;
    glyph_t* pyroblast;

    // Major
    glyph_t* dragons_breath;
    glyph_t* mirror_image;

    // Minor
    glyph_t* arcane_brilliance;
    glyph_t* conjuring;
  };
  glyphs_t glyphs;

  // Options
  std::string focus_magic_target_str;
  double merge_ignite;

  // Spell Data
  struct spells_t
  {
    spell_data_t* arcane_blast;
    spell_data_t* arcane_missiles;
    spell_data_t* arcane_power;
    spell_data_t* frost_armor;
    spell_data_t* hot_streak;
    spell_data_t* icy_veins;
    spell_data_t* mage_armor;
    spell_data_t* molten_armor;

    spell_data_t* flashburn;
    spell_data_t* frostburn;
    spell_data_t* mana_adept;

    spell_data_t* arcane_specialization;
    spell_data_t* fire_specialization;
    spell_data_t* frost_specialization;

    spell_data_t* blink;

    spell_data_t* stolen_time;

    spells_t() { memset( ( void* ) this, 0x0, sizeof( spells_t ) ); }
  };
  spells_t spells;

  // Specializations
  struct specializations_t
  {
    double arcane;
    double fire;
    double frost1;
    double frost2;
    double frost3;
    double flashburn;
    double frostburn;
    double mana_adept;
  };
  specializations_t specializations;

  // Procs
  proc_t* procs_deferred_ignite;
  proc_t* procs_munched_ignite;
  proc_t* procs_rolled_ignite;
  proc_t* procs_mana_gem;
  proc_t* procs_early_frost;
  proc_t* procs_tier12_mirror_image;
  proc_t* procs_test_for_crit_hotstreak;
  proc_t* procs_crit_for_hotstreak;
  proc_t* procs_hotstreak;
  proc_t* procs_improved_hotstreak;

  // Random Number Generation
  rng_t* rng_arcane_missiles;
  rng_t* rng_empowered_fire;
  rng_t* rng_enduring_winter;
  rng_t* rng_fire_power;
  rng_t* rng_impact;
  rng_t* rng_improved_freeze;
  rng_t* rng_nether_vortex;
  rng_t* rng_tier12_mirror_image;
  rng_t* rng_mage_armor_start;

  // Rotation (DPS vs DPM)
  struct rotation_t
  {
    int    current;
    double mana_gain;
    double dps_mana_loss;
    double dpm_mana_loss;
    timespan_t dps_time;
    timespan_t dpm_time;
    timespan_t last_time;

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
    talent_t* improved_counterspell;
    talent_t* improved_mana_gem;
    talent_t* invocation;
    talent_t* missile_barrage;
    talent_t* nether_vortex;
    talent_t* netherwind_presence;
    talent_t* presence_of_mind;
    talent_t* slow;
    talent_t* torment_the_weak;

    // Fire
    talent_t* blast_wave;
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
    talent_t* icy_veins;
    talent_t* improved_freeze;
    talent_t* piercing_chill;
    talent_t* piercing_ice;
    talent_t* shatter;

    talents_list_t() { memset( ( void* ) this, 0x0, sizeof( talents_list_t ) ); }
  };
  talents_list_t talents;

  // Up-Times
  benefit_t* uptimes_arcane_blast[ 5 ];
  benefit_t* uptimes_dps_rotation;
  benefit_t* uptimes_dpm_rotation;
  benefit_t* uptimes_water_elemental;

  int mana_gem_charges;
  timespan_t mage_armor_timer;

  mage_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) : player_t( sim, MAGE, name, r )
  {
    if ( race == RACE_NONE ) race = RACE_UNDEAD;

    tree_type[ MAGE_ARCANE ] = TREE_ARCANE;
    tree_type[ MAGE_FIRE   ] = TREE_FIRE;
    tree_type[ MAGE_FROST  ] = TREE_FROST;

    // Active
    active_ignite          = 0;

    // Pets
    pet_water_elemental     = 0;
    pet_tier12_mirror_image = 0;
    pet_mirror_image_3      = 0;

    // Cooldowns
    cooldowns_deep_freeze  = get_cooldown( "deep_freeze" );
    cooldowns_early_frost  = get_cooldown( "early_frost" );
    cooldowns_evocation    = get_cooldown( "evocation"   );
    cooldowns_fire_blast   = get_cooldown( "fire_blast"  );
    cooldowns_mana_gem     = get_cooldown( "mana_gem"    );
    cooldowns_tier12_mirror_image = get_cooldown( "tier12_mirror_image" );
    cooldowns_tier12_mirror_image -> duration = timespan_t::from_seconds( 45.0 );

    distance = 40;
    default_distance = 40;
    mana_gem_charges = 3;
    merge_ignite = 0;

    create_talents();
    create_glyphs();
    create_options();
  }

  // Character Definition
  virtual targetdata_t* new_targetdata( player_t* source, player_t* target ) {return new mage_targetdata_t( source, target );}
  virtual void      init_talents();
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
  virtual void      combat_begin();
  virtual void      reset();
  virtual action_expr_t* create_expression( action_t*, const std::string& name );
  virtual void      create_options();
  virtual bool      create_profile( std::string& profile_str, int save_type=SAVE_ALL, bool save_html=false );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual void      copy_from( player_t* source );
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() const { return RESOURCE_MANA; }
  virtual int       primary_role() const     { return ROLE_SPELL; }
  virtual double    composite_armor_multiplier() const;
  virtual double    composite_mastery() const;
  virtual double    composite_spell_crit() const;
  virtual double    composite_spell_haste() const;
  virtual double    composite_spell_power( const school_type school ) const;
  virtual double    composite_spell_resistance( const school_type school ) const;
  virtual double    matching_gear_multiplier( const attribute_type attr ) const;
  virtual void      stun();

  // Event Tracking
  virtual void   regen( timespan_t periodicity );
  virtual double resource_gain( int resource, double amount, gain_t* source=0, action_t* action=0 );
  virtual double resource_loss( int resource, double amount, action_t* action=0 );
};

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

  void _init_mage_spell_t()
  {
    may_crit      = ( base_dd_min > 0 ) && ( base_dd_max > 0 );
    tick_may_crit = true;
    crit_multiplier *= 1.33;
    may_chill = false;
    may_hot_streak = false;
    may_brain_freeze = false;
    fof_frozen = false;
    consumes_arcane_blast = false;
    dps_rotation = 0;
    dpm_rotation = 0;
    mage_t* p = player -> cast_mage();
    if ( p -> talents.enduring_winter -> rank() )
    {
      base_cost *= 1.0 + p -> talents.enduring_winter -> effect1().percent();
    }
  }

  mage_spell_t( const char* n, player_t* player, const school_type s, int t ) :
    spell_t( n, player, RESOURCE_MANA, s, t )
  {
    _init_mage_spell_t();
  }

  mage_spell_t( const char* n, uint32_t id, player_t* player ) :
    spell_t( n, id, player )
  {
    _init_mage_spell_t();
  }

  mage_spell_t( const char* n, const char* sname, player_t* player ) :
    spell_t( n, sname, player )
  {
    _init_mage_spell_t();
  }

  virtual void   parse_options( option_t*, const std::string& );
  virtual bool   ready();
  virtual double cost() const;
  virtual double haste() const;
  virtual void   execute();
  virtual timespan_t execute_time() const;
  virtual void   impact( player_t* t, int impact_result, double travel_dmg );
  virtual void   consume_resource();
  virtual void   player_buff();
  virtual void   target_debuff( player_t* t, int dmg_type );
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
    freeze_t( player_t* player, const std::string& options_str ):
      spell_t( "freeze", 33395, player )
    {
      parse_options( NULL, options_str );
      aoe = -1;
      may_crit = true;
      crit_multiplier *= 1.33;
      base_cost = 0;
    }

    virtual void player_buff()
    {
      spell_t::player_buff();
      player_spell_power = player -> cast_pet() -> owner -> composite_spell_power( SCHOOL_FROST ) * player -> cast_pet() -> owner -> composite_spell_power_multiplier() * 0.4;
      player_crit = player -> cast_pet() -> owner -> composite_spell_crit(); // Needs testing, but closer than before
    }

    virtual void execute()
    {
      spell_t::execute();
      mage_t* o = player -> cast_pet() -> owner -> cast_mage();

      o -> buffs_fingers_of_frost -> trigger( 2, 1, o -> talents.improved_freeze -> effect1().percent() );
    }

    virtual bool ready()
    {
      mage_t* o = player -> cast_pet() -> owner -> cast_mage();

      if ( ! o -> talents.improved_freeze -> rank() )
        return false;

      return spell_t::ready();
    }
  };

  struct water_bolt_t : public spell_t
  {
    water_bolt_t( player_t* player, const std::string& options_str ):
      spell_t( "water_bolt", 31707, player )
    {
      parse_options( NULL, options_str );
      may_crit = true;
      crit_multiplier *= 1.33;
      direct_power_mod = 0.833;
    }

    virtual void player_buff()
    {
      spell_t::player_buff();
      player_spell_power = player -> cast_pet() -> owner -> composite_spell_power( SCHOOL_FROST ) * player -> cast_pet() -> owner -> composite_spell_power_multiplier() * 0.4;
      player_crit = player -> cast_pet() -> owner -> composite_spell_crit(); // Needs testing, but closer than before
    }
  };

  water_elemental_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "water_elemental" )
  {
    action_list_str = "freeze,if=owner.buff.fingers_of_frost.stack=0/water_bolt";
    create_options();
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

  mirror_image_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "mirror_image_3", true /*guardian*/ ),
    num_images( 3 ), num_rotations( 2 ), sequence_finished( 0 )
  {}

  struct arcane_blast_t : public spell_t
  {
    action_t* next_in_sequence;

    arcane_blast_t( mirror_image_pet_t* mirror_image, action_t* nis ):
      spell_t( "mirror_arcane_blast", 88084, mirror_image ), next_in_sequence( nis )
    {
      may_crit          = true;
      background        = true;
    }

    virtual void player_buff()
    {
      spell_t::player_buff();
      mage_t* o = player -> cast_pet() -> owner -> cast_mage();
      double ab_stack_multiplier = o -> spells.arcane_blast -> effect1().percent();
      double ab_glyph_multiplier = o -> glyphs.arcane_blast -> effect1().percent();
      player_multiplier *= 1.0 + o ->  buffs_arcane_blast -> stack() * ( ab_stack_multiplier + ab_glyph_multiplier );
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

  struct fire_blast_t : public spell_t
  {
    action_t* next_in_sequence;

    fire_blast_t( mirror_image_pet_t* mirror_image, action_t* nis ):
      spell_t( "mirror_fire_blast", 59637, mirror_image ), next_in_sequence( nis )
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
      spell_t( "mirror_fireball", 88082, mirror_image ), next_in_sequence( nis )
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
        mirror_image_pet_t* mi = ( mirror_image_pet_t* ) player;
        mi -> sequence_finished++;
        if ( mi -> sequence_finished == mi -> num_images ) mi -> dismiss();
      }
    }
  };

  struct frostbolt_t : public spell_t
  {
    action_t* next_in_sequence;

    frostbolt_t( mirror_image_pet_t* mirror_image, action_t* nis ):
      spell_t( "mirror_frost_bolt", 59638, mirror_image ), next_in_sequence( nis )
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
        mirror_image_pet_t* mi = ( mirror_image_pet_t* ) player;
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

    health_per_stamina = 7.5;
    mana_per_intellect = 5;
  }

  virtual void init_actions()
  {
    for ( int i=0; i < num_images; i++ )
    {
      action_t* front=0;

      if ( owner -> cast_mage() -> glyphs.mirror_image -> ok() && owner -> cast_mage() -> primary_tree() != TREE_FROST )
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

  virtual double composite_spell_power( const school_type school ) const
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

  virtual void summon( timespan_t duration=timespan_t::zero )
  {
    pet_t::summon( duration );

    mage_t* o = owner -> cast_mage();

    snapshot_arcane_sp = o -> composite_spell_power( SCHOOL_ARCANE );
    snapshot_fire_sp   = o -> composite_spell_power( SCHOOL_FIRE   );
    snapshot_frost_sp  = o -> composite_spell_power( SCHOOL_FROST  );

    sequence_finished = 0;

    distance = o -> distance;

    for ( int i=0; i < num_images; i++ )
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

// ==========================================================================
// Pet Mirror Image Tier 12 set bonus
// ==========================================================================

struct tier12_mirror_image_pet_t : public pet_t
{
  double snapshot_crit;

  tier12_mirror_image_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "tier12_mirror_image", true /*guardian*/ ),
    snapshot_crit( 0 )
  {
    action_list_str += "/snapshot_stats";
    action_list_str += "/fireball";
  }

  virtual void summon( timespan_t duration=timespan_t::zero )
  {
    pet_t::summon( duration );
    // Guardians use snapshots
    snapshot_crit = owner -> composite_spell_crit();
    if ( owner -> bugs )
    {
      snapshot_crit = 0.00; // Rough guess
    }
    distance = owner -> distance;
  }

  virtual double composite_spell_crit() const
  {
    return snapshot_crit;
  }

  virtual double composite_spell_haste() const
  {
    return 1.0;
  }

  struct fireball_t : public spell_t
  {
    fireball_t( tier12_mirror_image_pet_t* mirror_image ):
      spell_t( "fireball", 99062, mirror_image )
    {
      may_crit          = true;
      trigger_gcd = timespan_t::from_seconds( 1.5 );
      if ( mirror_image -> owner -> bugs )
      {
        ability_lag = timespan_t::from_seconds( 0.74 );
        ability_lag_stddev = timespan_t::from_seconds( 0.62 / 2.0 );
      }
    }
  };

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "fireball" ) return new fireball_t( this );

    return pet_t::create_action( name, options_str );
  }
};

// calculate_dot_dps ========================================================

static double calculate_dot_dps( dot_t* dot )
{
  if ( ! dot -> ticking ) return 0;

  action_t* a = dot -> action;

  a -> result = RESULT_HIT;

  return ( a -> calculate_tick_damage() / a -> base_tick_time.total_seconds() );
}

// consume_brain_freeze =====================================================

static void consume_brain_freeze( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if ( p -> buffs_brain_freeze -> check() )
  {
    p -> buffs_brain_freeze -> expire();
  }
}

// trigger_brain_freeze =====================================================

static void trigger_brain_freeze( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();
  double chance = 0.0;

  if ( ! p -> talents.brain_freeze -> rank() ) return;

  chance = p -> talents.brain_freeze -> proc_chance();

  if ( p -> set_bonus.tier12_4pc_caster() )
  {
    chance += p -> sets-> set( SET_T12_4PC_CASTER ) -> s_effects[ 1 ] -> percent();
  }

  p -> buffs_brain_freeze -> trigger( 1, -1.0, chance );
}

// trigger_hot_streak =======================================================

static void trigger_hot_streak( mage_spell_t* s )
{
  sim_t* sim = s -> sim;
  mage_t*  p = s -> player -> cast_mage();

  if ( ! s -> may_hot_streak )
    return;

  if ( ! p -> talents.hot_streak -> rank() )
    return;

  int result = s -> result;

  p -> procs_test_for_crit_hotstreak -> occur();

  if ( sim -> smooth_rng )
  {
    // Decouple Hot Streak proc from actual crit to reduce wild swings during RNG smoothing.
    result = sim -> rng -> roll( s -> total_crit() ) ? RESULT_CRIT : RESULT_HIT;
  }
  if ( result == RESULT_CRIT )
  {
    p -> procs_crit_for_hotstreak -> occur();
    // Reference: http://elitistjerks.com/f75/t110326-cataclysm_fire_mage_compendium/p6/#post1831143

    double hot_streak_chance = -2.73 * s -> hot_streak_crit() + 0.95;

    if ( hot_streak_chance < 0.0 ) hot_streak_chance = 0.0;

    if ( p -> set_bonus.tier12_4pc_caster() )
    {
      hot_streak_chance += 0.3; // From testing on the PTR and also the consensus of the EJ thread.
    }

    if ( hot_streak_chance > 0 && p -> buffs_hot_streak -> trigger( 1, 0, hot_streak_chance ) )
    {
      p -> procs_hotstreak -> occur();
      p -> buffs_hot_streak_crits -> expire();
    }
    else if ( p -> talents.improved_hot_streak -> rank() )
    {
      p -> buffs_hot_streak_crits -> trigger();

      if ( p -> buffs_hot_streak_crits -> stack() == 2 )
      {
        p -> buffs_hot_streak_crits -> expire();

        hot_streak_chance = p -> talents.improved_hot_streak -> effect1().percent();

        if ( p -> buffs_hot_streak -> trigger( 1, 0, hot_streak_chance ) )
        {
          p -> procs_improved_hotstreak -> occur();
        }
      }
    }
  }
  else
  {
    p -> buffs_hot_streak_crits -> expire();
  }
}

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
      mage_spell_t( "ignite", 12654, player )
    {
      background    = true;

      // FIXME: Needs verification wheter it triggers trinket tick callbacks or not. It does trigger DTR arcane ticks.
      // proc          = false;

      may_resist    = true;
      tick_may_crit = false;
      hasted_ticks  = false;
      dot_behavior  = DOT_REFRESH;
      init();
    }
    virtual void impact( player_t* t, int impact_result, double ignite_dmg )
    {
      mage_spell_t::impact( t, impact_result, 0 );

      // FIXME: Is a is_hit check necessary here?
      base_td = ignite_dmg / dot() -> num_ticks;
    }
    virtual timespan_t travel_time()
    {
      return sim -> gauss( sim -> aura_delay, 0.25 * sim -> aura_delay );
    }
    virtual double total_td_multiplier() const { return 1.0; }
  };

  double ignite_dmg = dmg * p -> talents.ignite -> effect1().percent();

  if ( p -> specialization == MAGE_FIRE )
  {
    ignite_dmg *= 1.0 + p -> specializations.flashburn * p -> composite_mastery();
  }

  if ( p -> merge_ignite > 0 ) // Does not report Ignite seperately.
  {
    int result = s -> result;
    s -> result = RESULT_HIT;
    s -> assess_damage( s -> target, ignite_dmg * p -> merge_ignite, DMG_OVER_TIME, s -> result );
    s -> result = result;
    return;
  }

  if ( ! p -> active_ignite ) p -> active_ignite = new ignite_t( p );

  dot_t* dot = p -> active_ignite -> dot();

  if ( dot -> ticking )
  {
    ignite_dmg += p -> active_ignite -> base_td * dot -> ticks();
  }

  if ( timespan_t::from_seconds( 4.0 ) + sim -> aura_delay < dot -> remains() )
  {
    if ( sim -> log ) log_t::output( sim, "Player %s munches Ignite due to Max Ignite Duration.", p -> name() );
    p -> procs_munched_ignite -> occur();
    return;
  }

  if ( p -> active_ignite -> travel_event )
  {
    // There is an SPELL_AURA_APPLIED already in the queue, which will get munched.
    if ( sim -> log ) log_t::output( sim, "Player %s munches previous Ignite due to Aura Delay.", p -> name() );
    p -> procs_munched_ignite -> occur();
  }

  p -> active_ignite -> direct_dmg = ignite_dmg;
  p -> active_ignite -> result = RESULT_HIT;
  p -> active_ignite -> schedule_travel( s -> target );

  if ( p -> active_ignite -> travel_event && dot -> ticking )
  {
    if ( dot -> tick_event -> occurs() < p -> active_ignite -> travel_event -> occurs() )
    {
      // Ignite will tick before SPELL_AURA_APPLIED occurs, which means that the current Ignite will
      // both tick -and- get rolled into the next Ignite.
      if ( sim -> log ) log_t::output( sim, "Player %s rolls Ignite.", p -> name() );
      p -> procs_rolled_ignite -> occur();
    }
  }
}

// trigger_master_of_elements ===============================================

static void trigger_master_of_elements( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if ( s -> base_cost == 0 )
    return;

  if ( ! p -> talents.master_of_elements -> rank() )
    return;

  p -> resource_gain( RESOURCE_MANA, s -> base_cost * p -> talents.master_of_elements -> effect1().percent(), p -> gains_master_of_elements );
}

// trigger_replenishment ====================================================

static void trigger_replenishment( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if ( ! p -> talents.enduring_winter -> rank() )
    return;

  if ( ! p -> rng_enduring_winter -> roll( p -> talents.enduring_winter -> sd -> proc_chance() ) )
    return;

  p -> trigger_replenishment();
}

// trigger_tier12_mirror_image ==============================================

static void trigger_tier12_mirror_image( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if ( p -> set_bonus.tier12_2pc_caster() && ( p -> cooldowns_tier12_mirror_image -> remains() == timespan_t::zero ) )
  {
    if ( p -> rng_tier12_mirror_image -> roll( p -> sets -> set( SET_T12_2PC_CASTER ) -> proc_chance() ) )
    {
      p -> procs_tier12_mirror_image -> occur();
      p -> pet_tier12_mirror_image -> dismiss();
      p -> pet_tier12_mirror_image -> summon( p -> dbc.spell( 99063 ) -> duration() - timespan_t::from_seconds( 0.01 ) );
      p -> cooldowns_tier12_mirror_image -> start();
    }
  }
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

double mage_spell_t::cost() const
{
  mage_t* p = player -> cast_mage();

  if ( p -> buffs_clearcasting -> check() )
    return 0;

  double c = spell_t::cost();

  if ( p -> buffs_arcane_power -> check() )
  {
    double m = 1.0 + p -> buffs_arcane_power -> effect2().percent();

    if ( p -> set_bonus.tier12_4pc_caster() )
    {
      m += p -> sets -> set( SET_T12_4PC_CASTER ) -> s_effects[ 0 ] -> percent();
    }

    c *= m;
  }

  return c;
}

// mage_spell_t::haste ======================================================

double mage_spell_t::haste() const
{
  mage_t* p = player -> cast_mage();
  double h = spell_t::haste();
  if ( p -> buffs_icy_veins -> up() )
  {
    h *= 1.0 / ( 1.0 + p -> buffs_icy_veins -> effect1().percent() );
  }
  return h;
}

// mage_spell_t::execute ====================================================

void mage_spell_t::execute()
{
  mage_t* p = player -> cast_mage();

  p -> uptimes_dps_rotation -> update( p -> rotation.current == ROTATION_DPS );
  p -> uptimes_dpm_rotation -> update( p -> rotation.current == ROTATION_DPM );

  spell_t::execute();

  if ( result == RESULT_CRIT )
  {
    trigger_master_of_elements( this );
  }

  if ( background ) return;

  if ( consumes_arcane_blast ) p -> buffs_arcane_blast -> expire();

  if ( ( base_dd_max > 0 || base_td > 0 ) && ! background )
    p -> buffs_arcane_potency -> decrement();

  if ( ! channeled && spell_t::execute_time() > timespan_t::zero )
    p -> buffs_presence_of_mind -> expire();

  if ( fof_frozen )
  {
    p -> buffs_fingers_of_frost -> decrement();
  }

  if ( may_brain_freeze )
  {
    trigger_brain_freeze( this );
  }

  if ( result_is_hit() )
  {
    if ( p -> rng_impact -> roll( p -> talents.impact -> proc_chance() ) )
    {
      p -> cooldowns_fire_blast -> reset();
    }

    if ( p -> buffs_clearcasting -> trigger() )
    {
      p -> buffs_arcane_potency -> trigger( 2 );
    }

    trigger_hot_streak( this );
  }

  if ( ! p -> talents.hot_streak   -> ok() &&
       ! p -> talents.brain_freeze -> ok() )
  {
    p -> buffs_arcane_missiles -> trigger();
  }
}

// mage_spell_t::execute_time ===============================================

timespan_t mage_spell_t::execute_time() const
{
  mage_t* p = player -> cast_mage();

  timespan_t t = spell_t::execute_time();

  if ( ! channeled && t > timespan_t::zero && p -> buffs_presence_of_mind -> up() )
    return timespan_t::zero;

  return t;
}

// mage_spell_t::impact =====================================================

void mage_spell_t::impact( player_t* t, int impact_result, double travel_dmg )
{
  mage_t* p = player -> cast_mage();

  spell_t::impact( t, impact_result, travel_dmg );

  if ( impact_result == RESULT_CRIT )
  {
    trigger_ignite( this, direct_dmg );
  }

  if ( may_chill )
  {
    if ( result_is_hit( impact_result ) )
    {
      p -> buffs_fingers_of_frost -> trigger();
    }
  }
}

// mage_spell_t::consume_resource ===========================================

void mage_spell_t::consume_resource()
{
  mage_t* p = player -> cast_mage();
  spell_t::consume_resource();
  if ( p -> buffs_clearcasting -> check() )
  {
    // Treat the savings like a mana gain.
    double amount = spell_t::cost();
    if ( amount > 0 )
    {
      p -> buffs_clearcasting -> expire();
      p -> gains_clearcasting -> add( amount );
      p -> gains_clearcasting -> type = RESOURCE_MANA;
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
    if ( target -> health_percentage() < 35 )
    {
      player_multiplier *= 1.0 + p -> talents.molten_fury -> effect1().percent();
    }
  }
  if ( p -> buffs_invocation -> up() )
  {
    player_multiplier *= 1.0 + p -> talents.invocation -> effect1().percent();
  }
  if ( p -> buffs_arcane_power -> up() )
  {
    player_multiplier *= 1.0 + p -> buffs_arcane_power -> value();
  }
  if ( p -> buffs_arcane_potency -> up() )
  {
    player_crit += p -> talents.arcane_potency -> effect1().percent();
  }

  double mana_pct = player -> resource_current[ RESOURCE_MANA ] / player -> resource_max [ RESOURCE_MANA ];
  player_multiplier *= 1.0 + mana_pct * p -> specializations.mana_adept * p -> composite_mastery();

  if ( school == SCHOOL_ARCANE )
  {
    if ( target -> debuffs.snared() && p -> talents.torment_the_weak -> rank() )
    {
      player_multiplier *= 1.0 + p -> talents.torment_the_weak -> effect1().percent();
    }
    player_multiplier *= 1.0 + p -> specializations.arcane;
  }
  if ( school == SCHOOL_FIRE || school == SCHOOL_FROSTFIRE )
  {
    player_multiplier *= 1.0 + p -> specializations.fire;
    player_multiplier *= 1.0 + p -> talents.fire_power -> effect1().percent();
  }
  if ( school == SCHOOL_FROST || school == SCHOOL_FROSTFIRE )
  {
    player_multiplier *= 1.0 + p -> specializations.frost1;
  }
  if ( fof_frozen && p -> buffs_fingers_of_frost -> up() )
  {
    player_multiplier *= 1.0 + p -> specializations.frostburn * p -> composite_mastery();
  }

  if ( sim -> debug )
    log_t::output( sim, "mage_spell_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f mult=%.2f",
                   name(), player_hit, player_crit, player_spell_power, player_penetration, player_multiplier );
}

// mage_spell_t::target_debuff ==============================================

void mage_spell_t::target_debuff( player_t* t, int dmg_type )
{
  spell_t::target_debuff( t, dmg_type );

  if ( school == SCHOOL_FIRE && dmg_type == DMG_OVER_TIME )
  {
    mage_t* p = player -> cast_mage();
    target_multiplier *= 1.0 + p -> specializations.flashburn * p -> composite_mastery();
  }
}

// mage_spell_t::total_crit =================================================

double mage_spell_t::total_crit() const
{
  mage_t* p = player -> cast_mage();

  double crit = spell_t::total_crit();

  if ( fof_frozen && p -> buffs_fingers_of_frost -> up() )
  {
    crit *= 1.0 + util_t::talent_rank( p -> talents.shatter -> rank(), 2, 1.0, 2.0 );
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
    mage_spell_t( "arcane_barrage", 44425, p )
  {
    check_spec( TREE_ARCANE );
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
    mage_spell_t( "arcane_blast", 30451, p )
  {
    parse_options( NULL, options_str );

    if ( p -> set_bonus.tier11_4pc_caster() ) base_execute_time *= 0.9;
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

    if ( p -> buffs_clearcasting -> check() )
      return 0;

    double c = spell_t::cost();
    double base_cost = c;
    double stack_cost = 0;

    // Arcane Power only affects the initial base cost, it doesn't inflate the stack cost too
    // ( BaseCost * AP ) + ( BaseCost * 1.5 * ABStacks )
    if ( p -> buffs_arcane_power -> check() )
    {
      double m = 1.0 + p -> buffs_arcane_power -> effect2().percent();

      if ( p -> set_bonus.tier12_4pc_caster() )
      {
        m += p -> sets -> set( SET_T12_4PC_CASTER ) -> s_effects[ 0 ] -> percent();
      }

      c *= m;
    }

    if ( p -> buffs_arcane_blast -> check() )
    {
      stack_cost = base_cost * p -> buffs_arcane_blast -> stack() * p -> spells.arcane_blast -> effect2().percent();

      // The T12 4pc causes AP to reduce the base cost of AB in the stack calculation
      // ( BaseCost * AP ) + ( BaseCost * AP * 1.5 * ABStacks )
      if ( p -> set_bonus.tier12_4pc_caster() && p -> buffs_arcane_power -> check() )
      {
        stack_cost *= 1.0 + p -> buffs_arcane_power -> effect2().percent()
                          + p -> sets -> set( SET_T12_4PC_CASTER ) -> s_effects[ 0 ] -> percent();
      }
    }

    c += stack_cost;

    return c;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    for ( int i=0; i < 5; i++ )
    {
      p -> uptimes_arcane_blast[ i ] -> update( i == p -> buffs_arcane_blast -> stack() );
    }
    mage_spell_t::execute();

    p -> buffs_arcane_blast -> trigger();

    if ( result_is_hit() )
    {
      if ( p -> set_bonus.tier13_2pc_caster() )
        p -> buffs_tier13_2pc -> trigger( 1, -1, 1 );

      if ( ! target -> debuffs.snared() )
      {
        if ( p -> rng_nether_vortex -> roll( p -> talents.nether_vortex -> proc_chance() ) )
        {
          target -> debuffs.slow -> trigger();
          target -> debuffs.slow -> source = p;
        }
      }
    }

    trigger_tier12_mirror_image( this );
  }

  virtual timespan_t execute_time() const
  {
    mage_t* p = player -> cast_mage();
    timespan_t t = mage_spell_t::execute_time();
    t += p -> buffs_arcane_blast -> stack() * p -> spells.arcane_blast -> effect3().time_value();
    return t;
  }

  virtual void player_buff()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::player_buff();
    double ab_stack_multiplier = p -> spells.arcane_blast -> effect1().percent();
    double ab_glyph_multiplier = p -> glyphs.arcane_blast -> effect1().percent();
    player_multiplier *= 1.0 + p ->  buffs_arcane_blast -> stack() * ( ab_stack_multiplier + ab_glyph_multiplier );
  }
};

// Arcane Brilliance Spell ==================================================

struct arcane_brilliance_t : public mage_spell_t
{
  double bonus;

  arcane_brilliance_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_brilliance", 1459, p ), bonus( 0 )
  {
    parse_options( NULL, options_str );

    bonus      = p -> dbc.effect_average( p -> dbc.spell( 79058 ) -> effect1().id(), p -> level );
    base_cost *= 1.0 + p -> glyphs.arcane_brilliance -> effect1().percent();
    harmful = false;

    background = ( sim -> overrides.arcane_brilliance != 0 );
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> ooc_buffs() )
      {
        p -> buffs.arcane_brilliance -> trigger( 1, bonus );
      }
    }
  }

  virtual bool ready()
  {
    if ( player -> buffs.arcane_brilliance -> current_value >= bonus )
      return false;
    return mage_spell_t::ready();
  }
};

// Arcane Explosion Spell ===================================================

struct arcane_explosion_t : public mage_spell_t
{
  arcane_explosion_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_explosion", 1449, p )
  {
    check_spec( TREE_ARCANE );
    parse_options( NULL, options_str );
    aoe = -1;
    consumes_arcane_blast = true;

    if ( p -> talents.improved_arcane_explosion -> rank() )
    {
      trigger_gcd += p -> talents.improved_arcane_explosion -> effect1().time_value();
    }
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
  arcane_missiles_tick_t( mage_t* p, bool dtr=false ) :
    mage_spell_t( "arcane_missiles_tick", 7268, p )
  {
    dual        = true;
    background  = true;
    direct_tick = true;
    base_crit  += p -> glyphs.arcane_missiles -> effect1().percent();
    base_crit  += p -> set_bonus.tier11_2pc_caster() * 0.05;
    stats = player -> get_stats( "arcane_missiles", this );

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
    mage_spell_t( "arcane_missiles", 5143, p )
  {
    parse_options( NULL, options_str );
    channeled = true;
    may_miss = may_resist = false;
    num_ticks += p -> talents.improved_arcane_missiles -> rank();
    hasted_ticks = false;

    base_tick_time += p -> talents.missile_barrage -> mod_additive_time( P_TICK_TIME );

    tick_spell = new arcane_missiles_tick_t( p );
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    p -> buffs_arcane_missiles -> up();
    p -> buffs_arcane_missiles -> expire();

    p -> buffs_arcane_potency -> decrement();
  }

  virtual void last_tick( dot_t* d )
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::last_tick( d );
    p -> buffs_arcane_blast -> expire();
  }

  virtual void tick( dot_t* d )
  {
    tick_spell -> execute();
    stats -> add_tick( d -> time_to_tick );
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();
    if ( ! p -> buffs_arcane_missiles -> check() )
      return false;
    return mage_spell_t::ready();
  }
};

// Arcane Power Buff ========================================================

struct arcane_power_buff_t : public buff_t
{
  arcane_power_buff_t( mage_t* p ) :
    buff_t( p, p -> spells.arcane_power, NULL )
  {
    cooldown -> duration = timespan_t::zero; // CD is managed by the spell
  }

  virtual void expire()
  {
    buff_t::expire();
    mage_t* p = player -> cast_mage();
    p -> buffs_tier13_2pc -> expire();
  }
};

// Arcane Power Spell =======================================================

struct arcane_power_t : public mage_spell_t
{
  timespan_t orig_duration;

  arcane_power_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_power", 12042, p ), orig_duration( timespan_t::zero )
  {
    check_talent( p -> talents.arcane_power -> rank() );
    parse_options( NULL, options_str );
    harmful = false;
    cooldown -> duration *= 1.0 + p -> talents.arcane_flows -> effect1().percent();
    orig_duration = cooldown -> duration;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();

    if ( p -> set_bonus.tier13_4pc_caster() )
      cooldown -> duration = orig_duration +
                             p -> buffs_tier13_2pc -> check() * p -> spells.stolen_time -> effect1().time_value() *
                             ( 1.0 + p -> talents.arcane_flows -> effect1().percent() );

    mage_spell_t::execute();
    p -> buffs_arcane_power -> trigger( 1, effect1().percent() );
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    // Can't trigger AP if PoM is up
    if ( p -> buffs_presence_of_mind -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Blast Wave Spell =========================================================

struct blast_wave_t : public mage_spell_t
{
  blast_wave_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "blast_wave", 11113, p )
  {
    parse_options( NULL, options_str );
    aoe = -1;
  }
};

// Blink Spell ==============================================================

struct blink_t : public mage_spell_t
{
  blink_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "blink", 1953, p )
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
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_arcane_power -> check() && p -> glyphs.arcane_power -> ok() ) return timespan_t::zero;
    return mage_spell_t::gcd();
  }
};

// Cold Snap Spell ==========================================================

struct cold_snap_t : public mage_spell_t
{
  std::vector<cooldown_t*> cooldown_list;

  cold_snap_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "cold_snap", 11958, p )
  {
    check_talent( p -> talents.cold_snap -> rank() );
    parse_options( NULL, options_str );

    harmful = false;

    cooldown -> duration *= 1.0 + p -> talents.ice_floes -> effect1().percent();

    cooldown_list.push_back( p -> get_cooldown( "cone_of_cold"  ) );
    cooldown_list.push_back( p -> cooldowns_deep_freeze           );
    cooldown_list.push_back( p -> get_cooldown( "frostfire_orb" ) );
    cooldown_list.push_back( p -> get_cooldown( "icy_veins"     ) );
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    int num_cooldowns = ( int ) cooldown_list.size();
    for ( int i=0; i < num_cooldowns; i++ )
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
    mage_spell_t( "combustion", 11129, p ), orig_duration( timespan_t::zero )
  {
    check_talent( p -> talents.combustion -> rank() );
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
      ignite_dmg /= 1.0 + p -> specializations.flashburn * td -> dots_ignite -> action -> snapshot_mastery;
      ignite_dmg *= 1.0 + p -> specializations.flashburn * p -> composite_mastery();
    }

    base_td = 0;
    base_td += calculate_dot_dps( td -> dots_frostfire_bolt );
    base_td += ignite_dmg;
    base_td += calculate_dot_dps( td -> dots_living_bomb    ) * ( 1.0 + p -> specializations.flashburn * p -> composite_mastery() );
    base_td += calculate_dot_dps( td -> dots_pyroblast      ) * ( 1.0 + p -> specializations.flashburn * p -> composite_mastery() );

    if ( p -> set_bonus.tier13_4pc_caster() )
      cooldown -> duration = orig_duration + p -> buffs_tier13_2pc -> check() * p -> spells.stolen_time -> effect2().time_value();

    mage_spell_t::execute();
  }

  virtual void last_tick( dot_t* d )
  {
    mage_spell_t::last_tick( d );
    mage_t* p = player -> cast_mage();
    p -> buffs_tier13_2pc -> expire();
  }

  virtual double total_td_multiplier() const { return 1.0; } // No double-dipping!
};

// Cone of Cold Spell =======================================================

struct cone_of_cold_t : public mage_spell_t
{
  cone_of_cold_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "cone_of_cold", 120, p )
  {
    parse_options( NULL, options_str );
    aoe = -1;
    base_multiplier *= 1.0 + p -> glyphs.cone_of_cold -> effect1().percent();
    cooldown -> duration *= 1.0 + p -> talents.ice_floes -> effect1().percent();

    may_brain_freeze = true;
    may_chill = true;
  }
};

// Conjure Mana Gem Spell ===================================================

struct conjure_mana_gem_t : public mage_spell_t
{
  conjure_mana_gem_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "conjure_mana_gem", 759, p )
  {
    parse_options( NULL, options_str );

    base_cost += p -> glyphs.conjuring -> mod_additive( P_RESOURCE_COST );
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    mage_t* p = player -> cast_mage();
    p -> mana_gem_charges = 3;
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();
    if ( p -> mana_gem_charges == 3 )
      return false;

    return mage_spell_t::ready();
  }
};

// Counterspell Spell =======================================================

struct counterspell_t : public mage_spell_t
{
  counterspell_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "counterspell", 2139, p )
  {
    parse_options( NULL, options_str );
    may_miss = may_resist = may_crit = false;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    p -> buffs_invocation -> trigger();
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Deep Freeze Spell ========================================================

struct deep_freeze_t : public mage_spell_t
{
  deep_freeze_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "deep_freeze", 71757, p )
  {
    parse_options( NULL, options_str );

    // The spell data is spread across two separate Spell IDs.  Hard code missing for now.
    base_cost = 0.09 * p -> resource_base[ RESOURCE_MANA ];
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

    mage_t* p = player -> cast_mage();

    p -> buffs_fingers_of_frost -> up();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();
    if ( ! p -> buffs_fingers_of_frost -> check() )
      return false;
    return mage_spell_t::ready();
  }
};

// Dragon's Breath Spell ====================================================

struct dragons_breath_t : public mage_spell_t
{
  dragons_breath_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "dragons_breath", 31661, p )
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
    mage_spell_t( "evocation", 12051, p )
  {
    parse_options( NULL, options_str );

    base_tick_time    = timespan_t::from_seconds( 2.0 );
    num_ticks         = ( int ) ( duration() / base_tick_time );
    tick_zero         = true;
    channeled         = true;
    harmful           = false;
    hasted_ticks      = false;

    cooldown -> duration += p -> talents.arcane_flows -> effect2().time_value();
  }

  virtual void tick( dot_t* d )
  {
    mage_spell_t::tick( d );

    mage_t* p = player -> cast_mage();

    double mana = p -> resource_max[ RESOURCE_MANA ] * effect1().percent();
    p -> resource_gain( RESOURCE_MANA, mana, p -> gains_evocation );
  }

  virtual bool ready()
  {
    if ( ! mage_spell_t::ready() )
      return false;

    // FIXME: This should likely be removed in favor of expressions
    return ( player -> resource_current[ RESOURCE_MANA ] /
             player -> resource_max    [ RESOURCE_MANA ] ) < 0.60;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    // evocation automatically causes a switch to dpm rotation
    mage_t* p = player -> cast_mage();
    if ( p -> rotation.current == ROTATION_DPS )
    {
      p -> rotation.dps_time += ( sim -> current_time - p -> rotation.last_time );
    }
    else if ( p -> rotation.current == ROTATION_DPM )
    {
      p -> rotation.dpm_time += ( sim -> current_time - p -> rotation.last_time );
    }
    p -> rotation.last_time = sim -> current_time;
    if ( sim -> log ) log_t::output( sim, "%s switches to DPM spell rotation", p -> name() );
    p -> rotation.current = ROTATION_DPM;
  }
};

// Fire Blast Spell =========================================================

struct fire_blast_t : public mage_spell_t
{
  fire_blast_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "fire_blast", 2136, p )
  {
    parse_options( NULL, options_str );
    base_crit += p -> talents.improved_fire_blast -> effect1().percent();
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
    mage_spell_t( "fireball", 133, p )
  {
    parse_options( NULL, options_str );
    base_crit += p -> glyphs.fireball -> effect1().percent();
    may_hot_streak = true;
    if ( p -> set_bonus.tier11_4pc_caster() ) base_execute_time *= 0.9;
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
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_brain_freeze -> check() )
      return 0;

    return mage_spell_t::cost();
  }

  virtual timespan_t execute_time() const
  {
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_brain_freeze -> check() )
      return timespan_t::zero;
    return mage_spell_t::execute_time();
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    consume_brain_freeze( this );
    trigger_tier12_mirror_image( this );
    if ( result_is_hit() )
    {
      if ( p -> set_bonus.tier13_2pc_caster() )
        p -> buffs_tier13_2pc -> trigger( 1, -1, 0.5 );
    }
  }
};

// Flame Orb Spell ==========================================================

struct flame_orb_explosion_t : public mage_spell_t
{
  flame_orb_explosion_t( mage_t* p ) :
    mage_spell_t( "flame_orb_explosion", 83619, p )
  {
    background = true;
    aoe = -1;
    base_multiplier *= 1.0 + p -> talents.critical_mass -> effect2().percent();
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    // Ticks don't trigger ignite
    spell_t::impact( t, impact_result, travel_dmg );
  }
};

struct flame_orb_tick_t : public mage_spell_t
{
  flame_orb_tick_t( mage_t* p, bool dtr=false ) :
    mage_spell_t( "flame_orb_tick", 82739, p )
  {
    background = true;
    direct_tick = true;
    base_multiplier *= 1.0 + p -> talents.critical_mass -> effect2().percent();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new flame_orb_tick_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    // Ticks don't trigger ignite
    spell_t::impact( t, impact_result, travel_dmg );
    // Trigger Missiles here because the background action wouldn't trigger them otherwise
    mage_t* p = player -> cast_mage();
    if ( ! p -> talents.hot_streak   -> ok() &&
         ! p -> talents.brain_freeze -> ok() )
    {
      p -> buffs_arcane_missiles -> trigger();
    }
  }
};

struct flame_orb_t : public mage_spell_t
{
  flame_orb_explosion_t* explosion_spell;
  flame_orb_tick_t*      tick_spell;

  flame_orb_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "flame_orb", 82731, p )
  {
    parse_options( NULL, options_str );

    num_ticks = 15;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks = false;

    explosion_spell = new flame_orb_explosion_t( p );
    add_child( explosion_spell );

    tick_spell = new flame_orb_tick_t( p );
    tick_spell -> resource = RESOURCE_NONE; // Trickery to make MoE work
    tick_spell -> base_cost = base_cost / num_ticks;
    add_child( tick_spell );
  }

  virtual void tick( dot_t* d )
  {
    tick_spell -> execute();
    stats -> add_tick( d -> time_to_tick );
  }

  virtual void last_tick( dot_t* d )
  {
    mage_spell_t::last_tick( d );
    mage_t* p = player -> cast_mage();

    p -> buffs_arcane_potency -> decrement();

    if ( p -> rng_fire_power -> roll( p -> talents.fire_power -> proc_chance() ) )
    {
      explosion_spell -> execute();
    }
  }
};

// Focus Magic Spell ========================================================

struct focus_magic_t : public mage_spell_t
{
  player_t*          focus_magic_target;
  action_callback_t* focus_magic_cb;

  focus_magic_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "focus_magic", 54646, p ),
    focus_magic_target( 0 ), focus_magic_cb( 0 )
  {
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

      if ( focus_magic_target == 0 || focus_magic_target == p )
      {
        // Warn we didn't find the FM target and default to the player
        sim -> errorf( "Warning couldn't find %s for %s's Focus Magic, defaulting to SomebodySomewhere", target_str.c_str(), p -> name() );
        focus_magic_target = p;
      }
    }

    trigger_gcd = timespan_t::zero;

    struct focus_magic_feedback_callback_t : public action_callback_t
    {
      focus_magic_feedback_callback_t( player_t* p ) : action_callback_t( p -> sim, p, true ) {}

      virtual void trigger( action_t* /* a */, void* /* call_data */ )
      {
        listener -> cast_mage() -> buffs_focus_magic_feedback -> trigger();
      }
    };

    focus_magic_cb = new focus_magic_feedback_callback_t( p );
    focus_magic_cb -> active = false;
    focus_magic_target -> register_spell_callback( RESULT_CRIT_MASK, focus_magic_cb );
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

// Frost Armor Spell ========================================================

struct frost_armor_t : public mage_spell_t
{
  frost_armor_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frost_armor", 7302, p )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> buffs_molten_armor -> expire();
    p -> buffs_mage_armor -> expire();
    p -> buffs_frost_armor -> trigger();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_frost_armor -> check() ) return false;
    return mage_spell_t::ready();
  }
};

// Frostbolt Spell ==========================================================

struct frostbolt_t : public mage_spell_t
{
  frostbolt_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "frostbolt", 116, p )
  {
    parse_options( NULL, options_str );
    base_crit += p -> glyphs.frostbolt -> effect1().percent();
    may_chill = true;
    may_brain_freeze = true;
    if ( p -> set_bonus.tier11_4pc_caster() ) base_execute_time *= 0.9;
    base_multiplier *= 1.0 + p -> specializations.frost3;
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
    mage_t* p = player -> cast_mage();
    mage_spell_t::schedule_execute();
    if ( p -> cooldowns_early_frost -> remains() == timespan_t::zero )
    {
      p -> procs_early_frost -> occur();
      p -> cooldowns_early_frost -> start( timespan_t::from_seconds( 15.0 ) );
    }
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    if ( result_is_hit() )
    {
      trigger_replenishment( this );

      if ( p -> set_bonus.tier13_2pc_caster() )
        p -> buffs_tier13_2pc -> trigger( 1, -1, 0.5 );

      if ( result == RESULT_CRIT )
      {
        /* mage_t* p = player -> cast_mage();
        int max_targets = p -> talents.piercing_chill -> rank();
        if ( target -> is_enemy() )
        {
          target_t* q = target -> cast_target();

          for ( int i=0; i < q -> adds_nearby && i < max_targets; i ++ )
          {
            p -> buffs_fingers_of_frost -> trigger();
          }
        }*/
      }
    }
    trigger_tier12_mirror_image( this );
  }

  virtual timespan_t execute_time() const
  {
    mage_t* p = player -> cast_mage();
    timespan_t ct = mage_spell_t::execute_time();
    if ( p -> talents.early_frost -> rank() )
    {
      if ( p -> cooldowns_early_frost -> remains() == timespan_t::zero )
      {
        ct += p -> talents.early_frost -> effect1().time_value();
      }
    }
    return ct;
  }

  virtual timespan_t gcd() const
  {
    mage_t* p = player -> cast_mage();

    if ( p -> talents.early_frost -> rank() )
      if ( p -> cooldowns_early_frost -> remains() != timespan_t::zero )
        return timespan_t::from_seconds( 1.0 );

    return mage_spell_t::gcd();
  }
};

// Frostfire Bolt Spell =====================================================

struct frostfire_bolt_t : public mage_spell_t
{
  int dot_stack;

  frostfire_bolt_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "frostfire_bolt", 44614, p ), dot_stack( 0 )
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
    if ( p -> set_bonus.tier11_4pc_caster() ) base_execute_time *= 0.9;
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
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_brain_freeze -> check() )
      return 0;

    return mage_spell_t::cost();
  }

  virtual timespan_t execute_time() const
  {
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_brain_freeze -> check() )
      return timespan_t::zero;
    return mage_spell_t::execute_time();
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    fof_frozen = p -> buffs_brain_freeze -> up();
    mage_spell_t::execute();
    consume_brain_freeze( this );
    trigger_tier12_mirror_image( this );
    if ( result_is_hit() )
    {
      if ( p -> set_bonus.tier13_2pc_caster() )
        p -> buffs_tier13_2pc -> trigger( 1, -1, 0.5 );
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    mage_t* p = player -> cast_mage();

    if ( p -> glyphs.frostfire -> ok() && result_is_hit( impact_result ) )
    {
      if ( dot_stack < 3 ) dot_stack++;
      result = RESULT_HIT;
      double dot_dmg = calculate_direct_damage() * 0.03;
      base_td = dot_stack * dot_dmg / num_ticks;
    }
    mage_spell_t::impact( t, impact_result, travel_dmg );
  }

  virtual double total_td_multiplier() const { return 1.0; } // No double-dipping!

  virtual void tick( dot_t* d )
  {
    // Ticks don't benefit from Shatter, which checks for fof_frozen
    // So disable it for the ticks, assuming it was set to true in execute
    fof_frozen = false;
    mage_spell_t::tick( d );
  }
};

// Frostfire Orb Spell ======================================================

struct frostfire_orb_explosion_t : public mage_spell_t
{
  frostfire_orb_explosion_t( mage_t* p ) :
    mage_spell_t( "frostfire_orb_explosion", 83619, p )
  {
    background = true;
    aoe = -1;
    school = SCHOOL_FROSTFIRE; // required since defaults to FIRE
    may_chill = ( p -> talents.frostfire_orb -> rank() == 2 );
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    // Ticks don't trigger ignite
    spell_t::impact( t, impact_result, travel_dmg );

    mage_t* p = player -> cast_mage();

    if ( may_chill && result_is_hit( impact_result ) )
      p -> buffs_fingers_of_frost -> trigger();
  }
};

struct frostfire_orb_tick_t : public mage_spell_t
{
  frostfire_orb_tick_t( mage_t* p, bool dtr=false ) :
    mage_spell_t( "frostfire_orb_tick", 84721, p )
  {
    background = true;
    direct_tick = true;
    may_chill = ( p -> talents.frostfire_orb -> rank() == 2 );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new frostfire_orb_tick_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    // Ticks don't trigger ignite
    spell_t::impact( t, impact_result, travel_dmg );

    mage_t* p = player -> cast_mage();

    if ( may_chill && result_is_hit( impact_result ) )
      p -> buffs_fingers_of_frost -> trigger();
  }
};

struct frostfire_orb_t : public mage_spell_t
{
  frostfire_orb_explosion_t* explosion_spell;
  frostfire_orb_tick_t*      tick_spell;

  frostfire_orb_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frostfire_orb", 92283, p )
  {
    parse_options( NULL, options_str );

    school = SCHOOL_FROSTFIRE; // set as Fire in DBC, coloring the report chart wrong
    num_ticks = 15;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks = false;

    explosion_spell = new frostfire_orb_explosion_t( p );
    add_child( explosion_spell );

    tick_spell = new frostfire_orb_tick_t( p );
    add_child( tick_spell );
  }

  virtual void tick( dot_t* d )
  {
    tick_spell -> execute();
    stats -> add_tick( d -> time_to_tick );

    // Trigger Brain Freeze
    mage_t* p = player -> cast_mage();
    if ( p -> talents.frostfire_orb -> rank() == 2 )
      trigger_brain_freeze( this );
  }

  virtual void last_tick( dot_t* d )
  {
    mage_spell_t::last_tick( d );
    mage_t* p = player -> cast_mage();

    p -> buffs_arcane_potency -> decrement();

    if ( p -> rng_fire_power -> roll( p -> talents.fire_power -> proc_chance() ) )
    {
      explosion_spell -> execute();
    }
  }
};

// Ice Lance Spell ==========================================================

struct ice_lance_t : public mage_spell_t
{
  ice_lance_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "ice_lance", 30455, p )
  {
    parse_options( NULL, options_str );
    base_multiplier *= 1.0 + p -> glyphs.ice_lance -> effect1().percent();
    base_crit  += p -> set_bonus.tier11_2pc_caster() * 0.05;
    fof_frozen = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new ice_lance_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::player_buff();
    if ( p -> buffs_fingers_of_frost -> up() )
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
    buff_t( p, p -> spells.icy_veins, NULL )
  {
    cooldown -> duration = timespan_t::zero; // CD is managed by the spell
  }

  virtual void expire()
  {
    buff_t::expire();
    mage_t* p = player -> cast_mage();
    p -> buffs_tier13_2pc -> expire();
  }
};

// Icy Veins Spell ==========================================================

struct icy_veins_t : public mage_spell_t
{
  timespan_t orig_duration;

  icy_veins_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "icy_veins", 12472, p ), orig_duration( timespan_t::zero )
  {
    check_talent( p -> talents.icy_veins -> rank() );
    parse_options( NULL, options_str );
    cooldown -> duration *= 1.0 + p -> talents.ice_floes -> effect1().percent();
    orig_duration = cooldown -> duration;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    if ( p -> set_bonus.tier13_4pc_caster() )
      cooldown -> duration = orig_duration +
                             p -> buffs_tier13_2pc -> check() * p -> spells.stolen_time -> effect3().time_value() *
                             ( 1.0 + p -> talents.ice_floes -> effect1().percent() );
    consume_resource();
    update_ready();
    p -> buffs_icy_veins -> trigger();
  }
};

// Living Bomb Spell ========================================================

struct living_bomb_explosion_t : public mage_spell_t
{
  living_bomb_explosion_t( mage_t* p, bool dtr=false ) :
    mage_spell_t( "living_bomb_explosion", 44461, p )
  {
    aoe = -1;
    background = true;
    base_multiplier *= 1.0 + ( p -> glyphs.living_bomb    -> effect1().percent() +
                               p -> talents.critical_mass -> effect2().percent() );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new living_bomb_explosion_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    // Ticks don't trigger ignite
    spell_t::impact( t, impact_result, travel_dmg );
  }
};

struct living_bomb_t : public mage_spell_t
{
  living_bomb_explosion_t* explosion_spell;

  living_bomb_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "living_bomb", 44457, p )
  {
    check_talent( p -> talents.living_bomb -> rank() );
    parse_options( NULL, options_str );

    dot_behavior = DOT_REFRESH;

    explosion_spell = new living_bomb_explosion_t( p );
    explosion_spell -> resource = RESOURCE_NONE; // Trickery to make MoE work
    explosion_spell -> base_cost = base_cost;
    add_child( explosion_spell );
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    // Override the mage_spell_t version to ensure mastery effect is stacked additively.  Someday I will make this cleaner.
    mage_t* p = player -> cast_mage();
    spell_t::target_debuff( t, dmg_type );

    target_multiplier *= 1.0 + ( p -> glyphs.living_bomb -> effect1().percent() +
                                 p -> talents.critical_mass -> effect2().percent() +
                                 p -> specializations.flashburn * p -> composite_mastery() );
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
    mage_spell_t( "mage_armor", 6117, p )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> buffs_frost_armor -> expire();
    p -> buffs_molten_armor -> expire();
    p -> buffs_mage_armor -> trigger();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_mage_armor -> check() ) return false;
    return mage_spell_t::ready();
  }
};

// Mage Armor Buff ==========================================================

struct mage_armor_buff_t : public buff_t
{
  struct mage_armor_event_t : public event_t
  {
    mage_armor_event_t( player_t* player, timespan_t tick_time ) :
      event_t( player -> sim, player, "mage_armor" )
    {
      if ( tick_time < timespan_t::zero ) tick_time = timespan_t::zero;
      if ( tick_time > timespan_t::from_seconds( 5 ) ) tick_time = timespan_t::from_seconds( 5 );
      sim -> add_event( this, tick_time );
    }

    virtual void execute()
    {
      mage_t* p = player -> cast_mage();
      if ( p -> buffs_mage_armor -> check() )
      {
        p -> mage_armor_timer = sim -> current_time;
        double gain_amount = p -> resource_max[ RESOURCE_MANA ] * p -> spells.mage_armor -> effect2().percent();
        gain_amount *= 1.0 + p -> glyphs.mage_armor -> effect1().percent();

        p -> resource_gain( RESOURCE_MANA, gain_amount, p -> gains_mage_armor );

        new ( sim ) mage_armor_event_t( player, timespan_t::from_seconds( 5.0 ) );
      }
    }
  };

  mage_armor_buff_t( mage_t* p ) :
    buff_t( p, p -> spells.mage_armor, NULL )
  {}

  virtual void start( int stacks, double value )
  {
    mage_t* p = player -> cast_mage();
    timespan_t d = p -> rng_mage_armor_start -> real() * timespan_t::from_seconds( 5.0 ); // Random start of the first mana regen tick.
    p -> mage_armor_timer = sim -> current_time + d - timespan_t::from_seconds( 5.0 );
    new ( sim ) mage_armor_event_t( player, d );
    buff_t::start( stacks, value );
  }
};

// Mana Gem =================================================================

struct mana_gem_t : public action_t
{
  double min;
  double max;

  mana_gem_t( mage_t* p, const std::string& options_str ) :
    action_t( ACTION_USE, "mana_gem", p )
  {
    parse_options( NULL, options_str );

    min = p -> dbc.effect_min( 16856, p -> level );
    max = p -> dbc.effect_max( 16856, p -> level );

    if ( p -> level <= 80 )
    {
      min = 3330.0;
      max = 3500.0;
    }

    cooldown = p -> cooldowns_mana_gem;
    cooldown -> duration = timespan_t::from_seconds( 120.0 );
    trigger_gcd = timespan_t::zero;
    harmful = false;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();

    if ( sim -> log ) log_t::output( sim, "%s uses Mana Gem", p -> name() );

    p -> procs_mana_gem -> occur();
    p -> mana_gem_charges--;

    double gain = sim -> rng -> range( min, max );

    p -> resource_gain( RESOURCE_MANA, gain, p -> gains_mana_gem );

    p -> buffs_improved_mana_gem -> trigger();

    update_ready();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if ( p -> mana_gem_charges <= 0 )
      return false;

    if ( ( player -> resource_max[ RESOURCE_MANA ] - player -> resource_current[ RESOURCE_MANA ] ) < max )
      return false;

    return action_t::ready();
  }
};

// Mirror Image Spell =======================================================

struct mirror_image_t : public mage_spell_t
{
  mirror_image_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "mirror_image", 55342, p )
  {
    parse_options( NULL, options_str );
    harmful = false;

    num_ticks = 0;

    if ( p -> pet_mirror_image_3 )
    {
      stats -> add_child( p -> pet_mirror_image_3 -> get_stats( "mirror_arcane_blast" ) );
      stats -> add_child( p -> pet_mirror_image_3 -> get_stats( "mirror_fire_blast" ) );
      stats -> add_child( p -> pet_mirror_image_3 -> get_stats( "mirror_fireball" ) );
      stats -> add_child( p -> pet_mirror_image_3 -> get_stats( "mirror_frost_bolt" ) );
    }
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    p -> pet_mirror_image_3 -> summon();
  }

  virtual timespan_t gcd() const
  {
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_arcane_power -> check() && p -> glyphs.arcane_power -> ok() ) return timespan_t::zero;
    return mage_spell_t::gcd();
  }
};

// Molten Armor Spell =======================================================

struct molten_armor_t : public mage_spell_t
{
  molten_armor_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "molten_armor", 30482, p )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> buffs_frost_armor -> expire();
    p -> buffs_mage_armor -> expire();
    p -> buffs_molten_armor -> trigger();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_molten_armor -> check() ) return false;
    return mage_spell_t::ready();
  }
};

// Presence of Mind Spell ===================================================

struct presence_of_mind_t : public mage_spell_t
{
  presence_of_mind_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "presence_of_mind", 12043, p )
  {
    check_talent( p -> talents.presence_of_mind -> rank() );

    parse_options( NULL, options_str );
    harmful = false;

    cooldown -> duration *= 1.0 + p -> talents.arcane_flows -> effect1().percent();
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    mage_t* p = player -> cast_mage();
    p -> buffs_presence_of_mind -> trigger();
    p -> buffs_arcane_potency -> trigger( 2 );
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    // Can't use PoM while AP is up
    if ( p -> buffs_arcane_power -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Pyroblast Spell ==========================================================

struct pyroblast_t : public mage_spell_t
{
  pyroblast_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "pyroblast", 11366, p )
  {
    check_spec( TREE_FIRE );
    parse_options( NULL, options_str );
    base_crit += p -> glyphs.pyroblast -> effect1().percent();
    base_crit += p -> set_bonus.tier11_2pc_caster() * 0.05;
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
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    if ( result_is_hit() )
    {
      target -> debuffs.critical_mass -> trigger( 1, 1.0, p -> talents.critical_mass -> proc_chance() );
      target -> debuffs.critical_mass -> source = p;

      if ( p -> set_bonus.tier13_2pc_caster() )
        p -> buffs_tier13_2pc -> trigger( 1, -1, 0.5 );
    }
  }
};

// Pyroblast! Spell =========================================================

struct pyroblast_hs_t : public mage_spell_t
{
  pyroblast_hs_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "pyroblast_hs", 92315, p )
  {
    init_dot( "pyroblast" );
    check_spec( TREE_FIRE );
    parse_options( NULL, options_str );
    base_crit += p -> glyphs.pyroblast -> effect1().percent();
    base_crit += p -> set_bonus.tier11_2pc_caster() * 0.05;
    dot_behavior = DOT_REFRESH;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new pyroblast_hs_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_hot_streak -> up() )
    {
      p -> buffs_hot_streak -> expire();
    }
    mage_spell_t::execute();
    if ( result_is_hit() )
    {
      target -> debuffs.critical_mass -> trigger( 1, 1.0, p -> talents.critical_mass -> proc_chance() );
      target -> debuffs.critical_mass -> source = p;

      if ( p -> set_bonus.tier13_2pc_caster() )
        p -> buffs_tier13_2pc -> trigger( 1, -1, 0.5 );
    }
  }

  virtual bool ready()
  {
    if ( ! mage_spell_t::ready() )
      return false;

    mage_t* p = player -> cast_mage();

    return ( p -> buffs_hot_streak -> check() > 0 );
  }
};

// Scorch Spell =============================================================

struct scorch_t : public mage_spell_t
{
  int debuff;

  scorch_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "scorch", 2948, p ), debuff( 0 )
  {
    option_t options[] =
    {
      { "debuff",    OPT_BOOL, &debuff     },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost *= 1.0 + p -> talents.improved_scorch -> effect1().percent();
    may_hot_streak = true;

    if ( debuff )
      check_talent( p -> talents.critical_mass -> rank() );

    if ( p -> set_bonus.pvp_4pc_caster() )
      base_multiplier *= 1.05;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new scorch_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual bool usable_moving()
  {
    mage_t* p = player -> cast_mage();
    return p -> talents.firestarter -> rank() != 0;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    if ( result_is_hit() )
    {
      target -> debuffs.critical_mass -> trigger( 1, 1.0, p -> talents.critical_mass -> proc_chance() );
      target -> debuffs.critical_mass -> source = p;
    }
  }

  virtual bool ready()
  {
    if ( ! mage_spell_t::ready() )
      return false;

    if ( debuff )
    {

      if ( target -> debuffs.shadow_and_flame -> check() )
        return false;

      if ( target -> debuffs.critical_mass -> remains_gt( timespan_t::from_seconds( 6.0 ) ) )
        return false;
    }

    return true;
  }
};

// Slow Spell ===============================================================

struct slow_t : public mage_spell_t
{
  slow_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "slow", 31589, p )
  {
    check_talent( p -> talents.slow -> rank() );
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    target -> debuffs.slow -> trigger();
  }

  virtual bool ready()
  {
    if ( target -> debuffs.snared() )
      return false;
    return mage_spell_t::ready();
  }
};

// Time Warp Spell ==========================================================

struct time_warp_t : public mage_spell_t
{
  time_warp_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "time_warp", 80353, p )
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
    mage_spell_t( "water_elemental", 31687, p )
  {
    check_spec( TREE_FROST );
    parse_options( NULL, options_str );
    harmful = false;
    trigger_gcd = timespan_t::zero;
    base_cost = 0;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    mage_t* p = player -> cast_mage();
    p -> pet_water_elemental -> summon();
  }

  virtual bool ready()
  {
    if ( ! mage_spell_t::ready() )
      return false;

    mage_t* p = player -> cast_mage();
    return ! ( p -> pet_water_elemental && ! p -> pet_water_elemental -> sleeping );
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

    trigger_gcd = timespan_t::zero;
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
    timespan_t tte = p -> cooldowns_evocation -> remains();

    if ( p -> rotation.current == ROTATION_DPS )
    {
      p -> rotation.dps_time += ( sim -> current_time - p -> rotation.last_time );

      // We should only drop out of dps rotation if we break target mana treshold.
      // In that situation we enter a mps rotation waiting for evocation to come off cooldown or for fight to end.
      // The action list should take into account that it might actually need to do some burn in such a case.

      if ( tte < ttd )
      {
        // We're going until target percentage
        if ( p -> resource_current[ RESOURCE_MANA ] / p -> resource_max[ RESOURCE_MANA ] < evocation_target_mana_percentage / 100.0 )
        {
          if ( sim -> log ) log_t::output( sim, "%s switches to DPM spell rotation", p -> name() );

          p -> rotation.current = ROTATION_DPM;
        }
      }
      else
      {
        // We're going until OOM, stop when we can no longer cast full stack AB (approximately, 4 stack with AP can be 6177)
        if ( p -> resource_current[ RESOURCE_MANA ] < 6200 )
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
      double available_mana = p -> resource_current[ RESOURCE_MANA ];

      // Mana Gem, if we have uses left
      if ( p -> mana_gem_charges > 0 )
      {
        available_mana += p -> dbc.effect_max( 16856, p -> level );
      }

      // If this will be the last evocation then figure out how much of it we can actually burn before end and adjust appropriately.

      timespan_t evo_cooldown = ( timespan_t::from_seconds( 240.0 ) - p -> talents.arcane_flows -> rank() * timespan_t::from_seconds( 60.0 ) );

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
        timespan_t expected_time = timespan_t::from_seconds( ( available_mana - target_pct * p -> resource_max[ RESOURCE_MANA ] ) / consumption_rate );

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
    if ( cooldown -> remains() > timespan_t::zero )
      return false;

    if ( sim -> current_time < cooldown -> duration )
      return false;

    if ( if_expr && ! if_expr -> success() )
      return false;

    return true;
  }

  virtual void reset()
  {
    action_t::reset();
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
  if ( name == "blast_wave"        ) return new              blast_wave_t( this, options_str );
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
  if ( name == "flame_orb"         ) return new               flame_orb_t( this, options_str );
  if ( name == "focus_magic"       ) return new             focus_magic_t( this, options_str );
  if ( name == "frost_armor"       ) return new             frost_armor_t( this, options_str );
  if ( name == "frostbolt"         ) return new               frostbolt_t( this, options_str );
  if ( name == "frostfire_bolt"    ) return new          frostfire_bolt_t( this, options_str );
  if ( name == "frostfire_orb"     ) return new           frostfire_orb_t( this, options_str );
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
  if ( pet_name == "tier12_mirror_image" ) return new tier12_mirror_image_pet_t( sim, this );

  return 0;
}

// mage_t::create_pets ======================================================

void mage_t::create_pets()
{
  pet_mirror_image_3      = create_pet( "mirror_image_3"      );
  pet_water_elemental     = create_pet( "water_elemental"     );
  pet_tier12_mirror_image = create_pet( "tier12_mirror_image" );
}

// mage_t::init_talents =====================================================

void mage_t::init_talents()
{
  player_t::init_talents();

  talent_t* t=0;

  // Arcane
  talents.arcane_concentration        = t = find_talent( "Arcane Concentration" );
  talents.arcane_flows                = t = find_talent( "Arcane Flows" );
  talents.arcane_potency              = t = find_talent( "Arcane Potency" );
  talents.arcane_power                = t = find_talent( "Arcane Power" );
  talents.arcane_tactics              = t = find_talent( "Arcane Tactics" );
  talents.focus_magic                 = t = find_talent( "Focus Magic" );
  talents.improved_arcane_explosion   = t = find_talent( "Improved Arcane Explosion" );
  talents.improved_arcane_missiles    = t = find_talent( "Improved Arcane Missiles" );
  talents.improved_counterspell       = t = find_talent( "Improved Counterspell" );
  talents.improved_mana_gem           = t = find_talent( "Improved Mana Gem" );
  talents.invocation                  = t = find_talent( "Invocation" );
  talents.missile_barrage             = t = find_talent( "Missile Barrage" );
  talents.nether_vortex               = t = find_talent( "Nether Vortex" );
  talents.netherwind_presence         = t = find_talent( "Netherwind Presence" );
  talents.presence_of_mind            = t = find_talent( "Presence of Mind" );
  talents.slow                        = t = find_talent( "Slow" );
  talents.torment_the_weak            = t = find_talent( "Torment the Weak" );

  // Fire
  talents.blast_wave                  = t = find_talent( "Blast Wave" );
  talents.combustion                  = t = find_talent( "Combustion" );
  talents.critical_mass               = t = find_talent( "Critical Mass" );
  talents.dragons_breath              = t = find_talent( "Dragon's Breath" );
  talents.fire_power                  = t = find_talent( "Fire Power" );
  talents.firestarter                 = t = find_talent( "Firestarter" );
  talents.hot_streak                  = t = find_talent( "Hot Streak" );
  talents.ignite                      = t = find_talent( "Ignite" );
  talents.impact                      = t = find_talent( "Impact" );
  talents.improved_fire_blast         = t = find_talent( "Improved Fire Blast" );
  talents.improved_flamestrike        = t = find_talent( "Improved Flamestrike" );
  talents.improved_hot_streak         = t = find_talent( "Improved Hot Streak" );
  talents.improved_scorch             = t = find_talent( "Improved Scorch" );
  talents.living_bomb                 = t = find_talent( "Living Bomb" );
  talents.master_of_elements          = t = find_talent( "Master of Elements" );
  talents.molten_fury                 = t = find_talent( "Molten Fury" );

  // Frost
  talents.brain_freeze     = t = find_talent( "Brain Freeze"     );
  talents.cold_snap        = t = find_talent( "Cold Snap"        );
  talents.deep_freeze      = t = find_talent( "Deep Freeze"      );
  talents.early_frost      = t = find_talent( "Early Frost"      );
  talents.enduring_winter  = t = find_talent( "Enduring Winter"  );
  talents.fingers_of_frost = t = find_talent( "Fingers of Frost" );
  talents.frostfire_orb    = t = find_talent( "Frostfire Orb"    );
  talents.ice_barrier      = t = find_talent( "Ice Barrier"      );
  talents.ice_floes        = t = find_talent( "Ice Floes"        );
  talents.icy_veins        = t = find_talent( "Icy Veins"        );
  talents.improved_freeze  = t = find_talent( "Improved Freeze"  );
  talents.piercing_ice     = t = find_talent( "Piercing Ice"     );
  talents.piercing_chill   = t = find_talent( "Piercing Chill"   );
  talents.shatter          = t = find_talent( "Shatter"          );
}

// mage_t::init_spells ======================================================

void mage_t::init_spells()
{
  player_t::init_spells();

  spells.arcane_blast    = spell_data_t::find( 36032, "Arcane Blast",     dbc.ptr );
  spells.arcane_missiles = spell_data_t::find( 79683, "Arcane Missiles!", dbc.ptr );
  spells.arcane_power    = spell_data_t::find( 12042, "Arcane Power",     dbc.ptr );
  spells.frost_armor     = spell_data_t::find(  7302, "Frost Armor",      dbc.ptr );
  spells.hot_streak      = spell_data_t::find( 48108, "Hot Streak",       dbc.ptr );
  spells.icy_veins       = spell_data_t::find( 12472, "Icy Veins",        dbc.ptr );
  spells.mage_armor      = spell_data_t::find(  6117, "Mage Armor",       dbc.ptr );
  spells.molten_armor    = spell_data_t::find( 30482, "Molten Armor",     dbc.ptr );

  spells.flashburn  = spell_data_t::find( 76595, "Flashburn",  dbc.ptr );
  spells.frostburn  = spell_data_t::find( 76613, "Frostburn",  dbc.ptr );
  spells.mana_adept = spell_data_t::find( 76547, "Mana Adept", dbc.ptr );

  spells.arcane_specialization = spell_data_t::find( 84671, "Arcane Specialization", dbc.ptr );
  spells.fire_specialization   = spell_data_t::find( 84668, "Fire Specialization",   dbc.ptr );
  spells.frost_specialization  = spell_data_t::find( 84669, "Frost Specialization",  dbc.ptr );

  spells.blink = spell_data_t::find( 1953, "Blink", dbc.ptr );

  spells.stolen_time = spell_data_t::find( 105791, "Stolen Time", dbc.ptr );

  memset( ( void* ) &specializations, 0x00, sizeof( specializations_t ) );

  if ( specialization == MAGE_ARCANE )
  {
    specializations.arcane     = spells.arcane_specialization -> effect1().percent();
    specializations.mana_adept = spells.mana_adept -> effect1().coeff() * 0.01;
  }
  else if ( specialization == MAGE_FIRE )
  {
    specializations.fire      = spells.fire_specialization -> effect1().percent();
    specializations.flashburn = spells.flashburn -> effect1().coeff() * 0.01;
  }
  else if ( specialization == MAGE_FROST )
  {
    specializations.frost1    = spells.frost_specialization -> effect1().percent();
    specializations.frost2    = spells.frost_specialization -> effect2().base_value();
    specializations.frost3    = spells.frost_specialization -> effect3().percent();
    specializations.frostburn = spells.frostburn -> effect1().coeff() * 0.01;
  }

  glyphs.arcane_barrage       = find_glyph( "Glyph of Arcane Barrage" );
  glyphs.arcane_blast         = find_glyph( "Glyph of Arcane Blast" );
  glyphs.arcane_brilliance    = find_glyph( "Glyph of Arcane Brilliance" );
  glyphs.arcane_missiles      = find_glyph( "Glyph of Arcane Missiles" );
  glyphs.arcane_power         = find_glyph( "Glyph of Arcane Power" );
  glyphs.cone_of_cold         = find_glyph( "Glyph of Cone of Cold" );
  glyphs.conjuring            = find_glyph( "Glyph of Conjuring" );
  glyphs.deep_freeze          = find_glyph( "Glyph of Deep Freeze" );
  glyphs.dragons_breath       = find_glyph( "Glyph of Dragon's Breath" );
  glyphs.fireball             = find_glyph( "Glyph of Fireball" );
  glyphs.frost_armor          = find_glyph( "Glyph of Frost Armor" );
  glyphs.frostbolt            = find_glyph( "Glyph of Frostbolt" );
  glyphs.frostfire            = find_glyph( "Glyph of Frostfire" );
  glyphs.ice_lance            = find_glyph( "Glyph of Ice Lance" );
  glyphs.living_bomb          = find_glyph( "Glyph of Living Bomb" );
  glyphs.mage_armor           = find_glyph( "Glyph of Mage Armor" );
  glyphs.mirror_image         = find_glyph( "Glyph of Mirror Image" );
  glyphs.molten_armor         = find_glyph( "Glyph of Molten Armor" );
  glyphs.pyroblast            = find_glyph( "Glyph of Pyroblast" );

  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P    M2P    M4P    T2P    T4P    H2P    H4P
    {  90290,  90291,     0,     0,     0,     0,     0,     0 }, // Tier11
    {  99061,  99064,     0,     0,     0,     0,     0,     0 }, // Tier12
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
  scales_with[ STAT_SPIRIT ] = 0;
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

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )

  buffs_arcane_blast         = new buff_t( this, spells.arcane_blast,          NULL );
  buffs_arcane_missiles      = new buff_t( this, spells.arcane_missiles,       "chance", 0.40, NULL );
  buffs_arcane_power         = new arcane_power_buff_t( this );
  buffs_brain_freeze         = new buff_t( this, talents.brain_freeze,         NULL );
  buffs_clearcasting         = new buff_t( this, talents.arcane_concentration, "cooldown", 15.0, NULL );
  buffs_fingers_of_frost     = new buff_t( this, talents.fingers_of_frost,     "chance", talents.fingers_of_frost->effect1().percent(), NULL );
  buffs_frost_armor          = new buff_t( this, spells.frost_armor,           NULL );
  buffs_hot_streak           = new buff_t( this, spells.hot_streak,            NULL );
  buffs_icy_veins            = new icy_veins_buff_t( this );
  buffs_improved_mana_gem    = new buff_t( this, talents.improved_mana_gem,    "duration", 15.0, NULL );
  buffs_invocation           = new buff_t( this, talents.invocation,           NULL );
  buffs_mage_armor           = new mage_armor_buff_t( this );
  buffs_molten_armor         = new buff_t( this, spells.molten_armor,          NULL );

  buffs_arcane_potency       = new buff_t( this, "arcane_potency",       2, timespan_t::zero, timespan_t::zero, talents.arcane_potency ->rank() );
  buffs_focus_magic_feedback = new buff_t( this, "focus_magic_feedback", 1, timespan_t::from_seconds( 10.0 ) );
  buffs_hot_streak_crits     = new buff_t( this, "hot_streak_crits",     2, timespan_t::zero, timespan_t::zero, 1.0, true );
  buffs_presence_of_mind     = new buff_t( this, "presence_of_mind",     1 );

  buffs_tier13_2pc           = new stat_buff_t( this, "tier13_2pc", STAT_HASTE_RATING, 50.0, 10, timespan_t::from_seconds( 30.0 ) );
}

// mage_t::init_gains =======================================================

void mage_t::init_gains()
{
  player_t::init_gains();

  gains_clearcasting       = get_gain( "clearcasting"       );
  gains_evocation          = get_gain( "evocation"          );
  gains_frost_armor        = get_gain( "frost_armor"        );
  gains_mage_armor         = get_gain( "mage_armor"         );
  gains_mana_gem           = get_gain( "mana_gem"           );
  gains_master_of_elements = get_gain( "master_of_elements" );
}

// mage_t::init_procs =======================================================

void mage_t::init_procs()
{
  player_t::init_procs();

  procs_deferred_ignite         = get_proc( "deferred_ignite"               );
  procs_munched_ignite          = get_proc( "munched_ignite"                );
  procs_rolled_ignite           = get_proc( "rolled_ignite"                 );
  procs_mana_gem                = get_proc( "mana_gem"                      );
  procs_early_frost             = get_proc( "early_frost"                   );
  procs_tier12_mirror_image     = get_proc( "tier12_mirror_image"           );
  procs_test_for_crit_hotstreak = get_proc( "test_for_crit_hotstreak"       );
  procs_crit_for_hotstreak      = get_proc( "crit_test_hotstreak"           );
  procs_hotstreak               = get_proc( "normal_hotstreak"              );
  procs_improved_hotstreak      = get_proc( "improved_hotstreak"            );
}

// mage_t::init_uptimes =====================================================

void mage_t::init_benefits()
{
  player_t::init_benefits();

  uptimes_arcane_blast[ 0 ]    = get_benefit( "arcane_blast_0"  );
  uptimes_arcane_blast[ 1 ]    = get_benefit( "arcane_blast_1"  );
  uptimes_arcane_blast[ 2 ]    = get_benefit( "arcane_blast_2"  );
  uptimes_arcane_blast[ 3 ]    = get_benefit( "arcane_blast_3"  );
  uptimes_arcane_blast[ 4 ]    = get_benefit( "arcane_blast_4"  );
  uptimes_dps_rotation         = get_benefit( "dps_rotation"    );
  uptimes_dpm_rotation         = get_benefit( "dpm_rotation"    );
  uptimes_water_elemental      = get_benefit( "water_elemental" );
}

// mage_t::init_rng =========================================================

void mage_t::init_rng()
{
  player_t::init_rng();

  rng_arcane_missiles     = get_rng( "arcane_missiles"     );
  rng_empowered_fire      = get_rng( "empowered_fire"      );
  rng_enduring_winter     = get_rng( "enduring_winter"     );
  rng_fire_power          = get_rng( "fire_power"          );
  rng_impact              = get_rng( "impact"              );
  rng_improved_freeze     = get_rng( "improved_freeze"     );
  rng_nether_vortex       = get_rng( "nether_vortex"       );
  rng_tier12_mirror_image = get_rng( "tier12_mirror_image" );
  rng_mage_armor_start    = get_rng( "rng_mage_armor_start" );
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

    // Focus Magic
    if ( talents.focus_magic -> rank() ) action_list_str += "/focus_magic";
    // Arcane Brilliance
    if ( level >= 58 ) action_list_str += "/arcane_brilliance";

    // Armor
    if ( primary_tree() == TREE_ARCANE )
    {
      action_list_str += "/mage_armor";
    }
    else if ( primary_tree() == TREE_FIRE )
    {
      action_list_str += "/molten_armor,if=buff.mage_armor.down&buff.molten_armor.down";
      action_list_str += "/molten_armor,if=mana_pct>45&buff.mage_armor.up";
    }
    else
    {
      action_list_str += "/molten_armor";
    }

    // Water Elemental
    if ( primary_tree() == TREE_FROST ) action_list_str += "/water_elemental";

    // Snapshot Stats
    action_list_str += "/snapshot_stats";
    // Counterspell
    action_list_str += "/counterspell";
    // Refresh Gem during invuln phases
    if ( level >= 48 ) action_list_str += "/conjure_mana_gem,invulnerable=1,if=mana_gem_charges<3";
    // Arcane Choose Rotation
    if ( primary_tree() == TREE_ARCANE )
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
        if ( primary_tree() == TREE_ARCANE )
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
      if ( primary_tree() == TREE_FROST )
      {
        action_list_str += "/volcanic_potion,if=buff.bloodlust.react|buff.icy_veins.react|target.time_to_die<=40";
      }
      else if ( primary_tree() == TREE_ARCANE )
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
    if ( race == RACE_TROLL && primary_tree() == TREE_FIRE )
    {
      action_list_str += "/berserking";
    }
    else if ( race == RACE_BLOOD_ELF && primary_tree() != TREE_ARCANE )
    {
      action_list_str += "/arcane_torrent,if=mana_pct<91";
    }
    else if ( race == RACE_ORC && primary_tree() != TREE_ARCANE )
    {
      action_list_str += "/blood_fury";
    }

    // Talents by Spec
    // Arcane
    if ( primary_tree() == TREE_ARCANE )
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
      if ( ! talents.presence_of_mind -> rank() )
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
      if ( talents.arcane_power -> rank() )
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

      if ( talents.presence_of_mind -> rank() )
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
    else if ( primary_tree() == TREE_FIRE )
    {
      action_list_str += "/mana_gem,if=mana_deficit>12500";
      if ( talents.critical_mass -> rank() ) action_list_str += "/scorch,debuff=1";
      if ( talents.combustion -> rank()   )
      {
        action_list_str += "/combustion,if=dot.living_bomb.ticking&dot.ignite.ticking&dot.pyroblast.ticking";
      }
      action_list_str += "/mirror_image,if=target.time_to_die>=25";
      if ( talents.living_bomb -> rank() ) action_list_str += "/living_bomb,if=!ticking";
      if ( talents.hot_streak -> rank()  ) action_list_str += "/pyroblast_hs,if=buff.hot_streak.react";
      if ( level >= 81 ) action_list_str += "/flame_orb,if=target.time_to_die>=12";
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
    else if ( primary_tree() == TREE_FROST )
    {
      action_list_str += "/evocation,if=mana_pct<40&(buff.icy_veins.react|buff.bloodlust.react)";
      action_list_str += "/mana_gem,if=mana_deficit>12500";
      if ( talents.cold_snap -> rank() ) action_list_str += "/cold_snap,if=cooldown.deep_freeze.remains>15&cooldown.frostfire_orb.remains>30&cooldown.icy_veins.remains>30";
      if ( talents.frostfire_orb -> rank() && level >= 81 )
      {
        action_list_str += "/frostfire_orb,if=target.time_to_die>=12&!ticking";
      }
      if ( level >= 50 ) action_list_str += "/mirror_image,if=target.time_to_die>=25";
      if ( race == RACE_TROLL )
      {
        action_list_str += "/berserking,if=buff.icy_veins.down&buff.bloodlust.down";
      }
      if ( talents.icy_veins -> rank() )
      {
        action_list_str += "/icy_veins,if=buff.icy_veins.down&buff.bloodlust.down";
        if ( set_bonus.tier13_2pc_caster() && set_bonus.tier13_4pc_caster() )
          action_list_str += "&(buff.tier13_2pc.stack>7|cooldown.cold_snap.remains<22)";
      }
      if ( talents.deep_freeze -> rank() ) action_list_str += "/deep_freeze,if=buff.fingers_of_frost.react";
      if ( talents.brain_freeze -> rank() )
      {
        if ( level >= 56 )
        {
          action_list_str += "/frostfire_bolt,if=buff.brain_freeze.react";
        }
        else
          action_list_str += "/fireball,if=buff.brain_freeze.react";
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

  if ( buffs_frost_armor -> check() )
  {
    a *= 1.0 + spells.frost_armor -> effect1().percent();
  }

  return a;
}

// mage_t::composite_mastery ================================================

double mage_t::composite_mastery() const
{
  double m = player_t::composite_mastery();

  m += specializations.frost2;

  return m;
}

// mage_t::composite_spell_crit =============================================

double mage_t::composite_spell_crit() const
{
  double c = player_t::composite_spell_crit();

  // These also increase the water elementals crit chance

  if ( buffs_molten_armor -> up() )
  {
    c += ( spells.molten_armor -> effect3().percent() +
           glyphs.molten_armor -> effect1().percent() );
  }

  c += talents.piercing_ice -> effect1().percent();

  if ( buffs_focus_magic_feedback -> up() ) c += 0.03;

  return c;
}

// mage_t::composite_spell_haste ============================================

double mage_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( talents.netherwind_presence -> rank() )
  {
    h *= 1.0 / ( 1.0 + talents.netherwind_presence -> effect1().percent() );
  }

  return h;
}

// mage_t::composite_spell_power ============================================

double mage_t::composite_spell_power( const school_type school ) const
{
  double sp = player_t::composite_spell_power( school );

  if ( buffs_improved_mana_gem -> check() )
  {
    sp += resource_max[ RESOURCE_MANA ] * talents.improved_mana_gem->effect1().percent();
  }

  return sp;
}

// mage_t::composite_spell_resistance =======================================

double mage_t::composite_spell_resistance( const school_type school ) const
{
  double sr = player_t::composite_spell_resistance( school );

  if ( buffs_frost_armor -> check() && school == SCHOOL_FROST )
  {
    sr += spells.frost_armor -> effect3().base_value();
  }
  else if ( buffs_mage_armor -> check() )
  {
    sr += spells.mage_armor -> effect1().base_value();
  }

  return sr;
}

// mage_t::matching_gear_multiplier =========================================

double mage_t::matching_gear_multiplier( const attribute_type attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return 0.05;

  return 0.0;
}

// mage_t::combat_begin =====================================================

void mage_t::combat_begin()
{
  player_t::combat_begin();

  if ( talents.arcane_tactics -> rank() ) sim -> auras.arcane_tactics -> trigger();
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

  if ( glyphs.frost_armor -> ok() && buffs_frost_armor -> up()  )
  {
    double gain_amount = resource_max[ RESOURCE_MANA ] * glyphs.frost_armor -> effect1().percent();
    gain_amount *= periodicity.total_seconds() / 5.0;
    resource_gain( RESOURCE_MANA, gain_amount, gains_frost_armor );
  }

  if ( pet_water_elemental )
    uptimes_water_elemental -> update( pet_water_elemental -> sleeping == 0 );
}

// mage_t::resource_gain ====================================================

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

// mage_t::resource_loss ====================================================

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

// mage_t::stun =============================================================

void mage_t::stun()
{
  //TODO: override this to handle Blink
  player_t::stun();
}

// mage_t::create_expression ================================================

action_expr_t* mage_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "dps" )
  {
    struct dps_rotation_expr_t : public action_expr_t
    {
      dps_rotation_expr_t( action_t* a ) : action_expr_t( a, "dps_rotation", TOK_NUM ) {}
      virtual int evaluate() { result_num = ( action -> player -> cast_mage() -> rotation.current == ROTATION_DPS ) ? 1.0 : 0.0; return TOK_NUM; }
    };
    return new dps_rotation_expr_t( a );
  }
  else if ( name_str == "dpm" )
  {
    struct dpm_rotation_expr_t : public action_expr_t
    {
      dpm_rotation_expr_t( action_t* a ) : action_expr_t( a, "dpm_rotation", TOK_NUM ) {}
      virtual int evaluate() { result_num = ( action -> player -> cast_mage() -> rotation.current == ROTATION_DPM ) ? 1.0 : 0.0; return TOK_NUM; }
    };
    return new dpm_rotation_expr_t( a );
  }

  if ( name_str == "mana_gem_charges" )
  {
    struct mana_gem_charges_expr_t : public action_expr_t
    {
      mana_gem_charges_expr_t( action_t* a ) : action_expr_t( a, "mana_gem_charges", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> cast_mage() -> mana_gem_charges; return TOK_NUM; }
    };
    return new mana_gem_charges_expr_t( a );
  }

  if ( name_str == "mage_armor_timer" )
  {
    struct mage_armor_timer_expr_t : public action_expr_t
    {
      mage_armor_timer_expr_t( action_t* a ) : action_expr_t( a, "mage_armor_timer", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> cast_mage() -> mage_armor_timer.total_seconds() + 5 - action -> sim -> current_time.total_seconds(); return TOK_NUM; }
    };
    return new mage_armor_timer_expr_t( a );
  }

  if ( name_str == "burn_mps" )
  {
    struct burn_mps_expr_t : public action_expr_t
    {
      burn_mps_expr_t( action_t* a ) : action_expr_t( a, "burn_mps", TOK_NUM ) {}
      virtual int evaluate()
      {
        mage_t* p = action -> player -> cast_mage();
        if ( p -> rotation.current == ROTATION_DPS )
        {
          p -> rotation.dps_time += ( action -> sim -> current_time - p -> rotation.last_time );
        }
        else if ( p -> rotation.current == ROTATION_DPM )
        {
          p -> rotation.dpm_time += ( action -> sim -> current_time - p -> rotation.last_time );
        }
        p -> rotation.last_time = action -> sim -> current_time;

        result_num = ( p -> rotation.dps_mana_loss / p -> rotation.dps_time.total_seconds() ) - ( p -> rotation.mana_gain / action -> sim -> current_time.total_seconds() );
        return TOK_NUM;
      }
    };
    return new burn_mps_expr_t( a );
  }

  if ( name_str == "regen_mps" )
  {
    struct regen_mps_expr_t : public action_expr_t
    {
      regen_mps_expr_t( action_t* a ) : action_expr_t( a, "regen_mps", TOK_NUM ) {}
      virtual int evaluate()
      {
        mage_t* p = action -> player -> cast_mage();
        result_num = ( p -> rotation.mana_gain / action -> sim -> current_time.total_seconds() );
        return TOK_NUM;
      }
    };
    return new regen_mps_expr_t( a );
  }

  return player_t::create_expression( a, name_str );
}

// mage_t::create_options ===================================================

void mage_t::create_options()
{
  player_t::create_options();

  option_t mage_options[] =
  {
    { "focus_magic_target",  OPT_STRING, &( focus_magic_target_str ) },
    { "merge_ignite",        OPT_FLT,    &( merge_ignite           ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, mage_options );
}

// mage_t::create_profile ===================================================

bool mage_t::create_profile( std::string& profile_str, int save_type, bool save_html )
{
  player_t::create_profile( profile_str, save_type, save_html );

  if ( save_type == SAVE_ALL )
  {
    if ( ! focus_magic_target_str.empty() ) profile_str += "focus_magic_target=" + focus_magic_target_str + "\n";
  }

  return true;
}

// mage_t::copy_from ========================================================

void mage_t::copy_from( player_t* source )
{
  player_t::copy_from( source );
  focus_magic_target_str = source -> cast_mage() -> focus_magic_target_str;
  merge_ignite           = source -> cast_mage() -> merge_ignite;
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

  if ( strstr( s, "firelord"      ) ) return SET_T11_CASTER;
  if ( strstr( s, "firehawk"      ) ) return SET_T12_CASTER;
  if ( strstr( s, "time_lords_"   ) ) return SET_T13_CASTER;

  if ( strstr( s, "gladiators_silk_"  ) ) return SET_PVP_CASTER;

  return SET_NONE;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_mage  ===================================================

player_t* player_t::create_mage( sim_t* sim, const std::string& name, race_type r )
{
  return new mage_t( sim, name, r );
}

// player_t::mage_init ======================================================

void player_t::mage_init( sim_t* sim )
{
  sim -> auras.arcane_tactics = new aura_t( sim, "arcane_tactics", 1, timespan_t::zero );

  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> buffs.arcane_brilliance = new stat_buff_t( p, "arcane_brilliance", STAT_MAX_MANA, p -> level < MAX_LEVEL ? sim -> dbc.effect_average( sim -> dbc.spell( 79058 ) -> effect1().id(), sim -> max_player_level ) : 0, !p -> is_pet() );
    p -> buffs.focus_magic       = new      buff_t( p, 54646, "focus_magic" );
    p -> debuffs.critical_mass   = new debuff_t( p, 22959, "critical_mass" );
    p -> debuffs.slow            = new debuff_t( p, 31589, "slow" );
  }
}

// player_t::mage_combat_begin ==============================================

void player_t::mage_combat_begin( sim_t* sim )
{
  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> ooc_buffs() )
    {
      if ( sim -> overrides.arcane_brilliance ) p -> buffs.arcane_brilliance -> override( 1, sim -> dbc.effect_average( sim -> dbc.spell( 79058 ) -> effect1().id(), sim -> max_player_level ) );
      if ( sim -> overrides.focus_magic       ) p -> buffs.focus_magic       -> override();
    }
  }

  for ( player_t* t = sim -> target_list; t; t = t -> next )
  {
    if ( sim -> overrides.critical_mass ) t -> debuffs.critical_mass -> override( 1 );
  }

  if ( sim -> overrides.arcane_tactics ) sim -> auras.arcane_tactics -> override( 1 );
}

