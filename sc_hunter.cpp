// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Hunter
// ==========================================================================

struct hunter_pet_t;

enum { ASPECT_NONE=0, ASPECT_HAWK, ASPECT_VIPER, ASPECT_BEAST, ASPECT_MAX };

struct hunter_t : public player_t
{
  // Active
  hunter_pet_t* active_pet;
  int           active_aspect;
  action_t*     active_chimera_serpent;
  action_t*     active_wild_quiver;
  action_t*     active_scorpid_sting;
  action_t*     active_serpent_sting;
  action_t*     active_viper_sting;

  // Buffs
  double buffs_aspect_of_the_hawk;
  double buffs_aspect_of_the_viper;
  int    buffs_beast_within;
  int    buffs_call_of_the_wild;
  int    buffs_cobra_strikes;
  int    buffs_expose_weakness;
  double buffs_improved_aspect_of_the_hawk;
  int    buffs_improved_steady_shot;
  int    buffs_lock_and_load;
  double buffs_master_tactician;
  double buffs_rapid_fire;
  int    buffs_trueshot_aura;

  // Expirations
  event_t*  expirations_cobra_strikes;
  event_t*  expirations_expose_weakness;
  event_t*  expirations_hunting_party;
  event_t*  expirations_improved_aspect_of_the_hawk;
  event_t*  expirations_improved_steady_shot;
  event_t*  expirations_lock_and_load;
  event_t*  expirations_master_tactician;
  event_t*  expirations_rapid_fire;

  // Cooldowns
  double cooldowns_lock_and_load;

  // Gains
  gain_t* gains_chimera_viper;
  gain_t* gains_invigoration;
  gain_t* gains_roar_of_recovery;
  gain_t* gains_thrill_of_the_hunt;
  gain_t* gains_viper_aspect_passive;
  gain_t* gains_viper_aspect_shot;

  // Procs
  proc_t* procs_wild_quiver;

  // Uptimes
  uptime_t* uptimes_aspect_of_the_viper;
  uptime_t* uptimes_expose_weakness;
  uptime_t* uptimes_improved_aspect_of_the_hawk;
  uptime_t* uptimes_improved_steady_shot;
  uptime_t* uptimes_master_tactician;

  // Auto-Attack
  attack_t* ranged_attack;

  // Ammunition
  double ammo_dps;

  double quiver_haste;

  struct talents_t
  {
    int  animal_handler;
    int  aimed_shot;
    int  aspect_mastery;
    int  barrage;
    int  beast_within;
    int  bestial_wrath;
    int  bestial_discipline;
    int  careful_aim;
    int  chimera_shot;
    int  cobra_strikes;
    int  combat_experience;
    int  efficiency;
    int  explosive_shot;
    int  expose_weakness;
    int  ferocious_inspiration;
    int  ferocity;
    int  focused_aim;
    int  focused_fire;
    int  frenzy;
    int  go_for_the_throat;
    int  hunter_vs_wild;
    int  hunting_party;
    int  improved_arcane_shot;
    int  improved_aspect_of_the_hawk;
    int  improved_barrage;
    int  improved_hunters_mark;
    int  improved_steady_shot;
    int  improved_stings;
    int  improved_tracking;
    int  invigoration;
    int  killer_instinct;
    int  kindred_spirits;
    int  lethal_shots;
    int  lightning_reflexes;
    int  lock_and_load;
    int  longevity;
    int  marked_for_death;
    int  master_marksman;
    int  master_tactician;
    int  mortal_shots;
    int  noxious_stings;
    int  piercing_shots;
    int  ranged_weapon_specialization;
    int  rapid_killing;
    int  rapid_recuperation;
    int  readiness;
    int  resourcefulness;
    int  scatter_shot;
    int  serpents_swiftness;
    int  silencing_shot;
    int  sniper_training;
    int  survivalist;
    int  survival_instincts;
    int  thrill_of_the_hunt;
    int  tnt;
    int  trueshot_aura;
    int  unleashed_fury;
    int  wild_quiver;

    // Talents not yet fully implemented
    int  savage_strikes;
    int  survival_tactics;

    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int  aspect_of_the_viper;
    int  bestial_wrath;
    int  hunters_mark;
    int  improved_aspect_of_the_hawk;
    int  rapid_fire;
    int  serpent_sting;
    int  steady_shot;
    int  trueshot_aura;
    glyphs_t() { memset( (void*) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  hunter_t( sim_t* sim, std::string& name ) : player_t( sim, HUNTER, name )
  {
    // Active
    active_pet             = 0;
    active_aspect          = ASPECT_NONE;
    active_chimera_serpent = 0;
    active_wild_quiver     = 0;
    active_scorpid_sting   = 0;
    active_serpent_sting   = 0;
    active_viper_sting     = 0;

    // Buffs
    buffs_aspect_of_the_hawk          = 0;
    buffs_aspect_of_the_viper         = 0;
    buffs_beast_within                = 0;
    buffs_call_of_the_wild            = 0;
    buffs_cobra_strikes               = 0;
    buffs_expose_weakness             = 0;
    buffs_improved_aspect_of_the_hawk = 0;
    buffs_improved_steady_shot        = 0;
    buffs_lock_and_load               = 0;
    buffs_master_tactician            = 0;
    buffs_rapid_fire                  = 0;
    buffs_trueshot_aura               = 0;

    // Expirations
    expirations_cobra_strikes               = 0;
    expirations_expose_weakness             = 0;
    expirations_hunting_party               = 0;
    expirations_improved_aspect_of_the_hawk = 0;
    expirations_improved_steady_shot        = 0;
    expirations_lock_and_load               = 0;
    expirations_master_tactician            = 0;
    expirations_rapid_fire                  = 0;

    // Cooldowns
    cooldowns_lock_and_load = 0;

    // Gains
    gains_chimera_viper        = get_gain( "chimera_viper" );
    gains_invigoration         = get_gain( "invigoration" );
    gains_roar_of_recovery     = get_gain( "roar_of_recovery" );
    gains_thrill_of_the_hunt   = get_gain( "thrill_of_the_hunt" );
    gains_viper_aspect_passive = get_gain( "viper_aspect_passive" );
    gains_viper_aspect_shot    = get_gain( "viper_aspect_shot" );

    // Procs
    procs_wild_quiver = get_proc( "wild_quiver" );

    // Up-Times
    uptimes_aspect_of_the_viper         = get_uptime( "aspect_of_the_viper" );
    uptimes_expose_weakness             = get_uptime( "expose_weakness" );
    uptimes_improved_aspect_of_the_hawk = get_uptime( "improved_aspect_of_the_hawk" );
    uptimes_improved_steady_shot        = get_uptime( "improved_steady_shot" );
    uptimes_master_tactician            = get_uptime( "master_tactician" );

    ammo_dps = 0;

    quiver_haste = 1.0;
  }

  // Character Definition
  virtual void      init_base();
  virtual void      reset();
  virtual double    composite_attack_power();
  virtual bool      get_talent_trees( std::vector<int*>& beastmastery, std::vector<int*>& marksmanship, std::vector<int*>& survival );
  virtual bool      parse_talents_mmo( const std::string& talent_string );
  virtual bool      parse_option( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet( const std::string& name );
  virtual int       primary_resource() { return RESOURCE_MANA; }

  // Event Tracking
  virtual void regen( double periodicity );

  // Utility
  void cancel_sting()
  {
    if( active_scorpid_sting ) active_scorpid_sting -> cancel();
    if( active_serpent_sting ) active_serpent_sting -> cancel();
    if( active_viper_sting   ) active_viper_sting   -> cancel();
  }
  action_t* active_sting()
  {
    if( active_scorpid_sting ) return active_scorpid_sting;
    if( active_serpent_sting ) return active_serpent_sting;
    if( active_viper_sting   ) return active_viper_sting;
    return 0;
  }
};

// ==========================================================================
// Hunter Pet
// ==========================================================================

enum pet_type_t
{
  PET_NONE=0,

  PET_CARRION_BIRD,
  PET_CAT,
  PET_CORE_HOUND,
  PET_DEVILSAUR,
  PET_HYENA,
  PET_MOTH,
  PET_RAPTOR,
  PET_SPIRIT_BEAST,
  PET_TALLSTRIDER,
  PET_WASP,
  PET_WOLF,
  PET_FEROCITY,

  PET_BEAR,
  PET_BOAR,
  PET_CRAB,
  PET_CROCOLISK,
  PET_GORILLA,
  PET_RHINO,
  PET_SCORPID,
  PET_TURTLE,
  PET_WARP_STALKER,
  PET_WORM,
  PET_TENACITY,

  PET_BAT,
  PET_BIRD_OF_PREY,
  PET_CHIMERA,
  PET_DRAGONHAWK,
  PET_NETHER_RAY,
  PET_RAVAGER,
  PET_SERPENT,
  PET_SILITHID,
  PET_SPIDER,
  PET_SPOREBAT,
  PET_WIND_SERPENT,
  PET_CUNNING,

  PET_MAX
};

struct hunter_pet_t : public pet_t
{
  int pet_type;

  // Buffs
  int buffs_bestial_wrath;
  int buffs_call_of_the_wild;
  int buffs_frenzy;
  int buffs_kill_command;
  int buffs_owls_focus;
  int buffs_rabid;
  int buffs_rabid_power_stack;

  // Expirations
  event_t* expirations_frenzy;
  event_t* expirations_owls_focus;

  // Gains
  gain_t* gains_go_for_the_throat;

  // Procs
  proc_t* procs_placeholder;

  // Uptimes
  uptime_t* uptimes_frenzy;

  // Auto-Attack
  attack_t* main_hand_attack;

  struct talents_t
  {
    int call_of_the_wild;
    int cobra_reflexes;
    int feeding_frenzy;
    int rabid;
    int roar_of_recovery;
    int spiked_collar;
    int spiders_bite;
    int owls_focus;

    // Talents not yet implemented
    int wolverine_bite;

    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  hunter_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name, int pt ) :
    pet_t( sim, owner, pet_name ), pet_type(pt), main_hand_attack(0)
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.damage     = (51.0+78.0)/2; // FIXME only level 80 value
    main_hand_weapon.swing_time = 2.0;

    stamina_per_owner = 0.45;

    // Buffs
    buffs_bestial_wrath     = 0;
    buffs_call_of_the_wild  = 0;
    buffs_frenzy            = 0;
    buffs_kill_command      = 0;
    buffs_owls_focus        = 0;
    buffs_rabid             = 0;
    buffs_rabid_power_stack = 0;

    // Expirations
    expirations_frenzy     = 0;
    expirations_owls_focus = 0;

    // Gains
    gains_go_for_the_throat = get_gain( "go_for_the_throat" );

    // Procs
    procs_placeholder = get_proc( "placeholder" );

    // Uptimes
    uptimes_frenzy = get_uptime( "frenzy" );
  }

  virtual void reset()
  {
    pet_t::reset();

    // Buffs
    buffs_bestial_wrath     = 0;
    buffs_call_of_the_wild  = 0;
    buffs_frenzy            = 0;
    buffs_kill_command      = 0;
    buffs_owls_focus        = 0;
    buffs_rabid             = 0;
    buffs_rabid_power_stack = 0;

    // Expirations
    expirations_frenzy     = 0;
    expirations_owls_focus = 0;
  }

  virtual int group()
  {
    if( pet_type < PET_FEROCITY ) return PET_FEROCITY;
    if( pet_type < PET_TENACITY ) return PET_TENACITY;
    if( pet_type < PET_CUNNING  ) return PET_CUNNING;
    return PET_NONE;
  }

  virtual void init_base()
  {
    hunter_t* o = owner -> cast_hunter();

    attribute_base[ ATTR_STRENGTH  ] = rating_t::interpolate( level, 0, 162, 331 );
    attribute_base[ ATTR_AGILITY   ] = rating_t::interpolate( level, 0, 54, 113 );
    //stamina is different for every pet type (at least tenacity have more)
    attribute_base[ ATTR_STAMINA   ] = rating_t::interpolate( level, 0, 307, 361 );
    attribute_base[ ATTR_INTELLECT ] = 100; // FIXME
    attribute_base[ ATTR_SPIRIT    ] = 100; // FIXME

    // AP = (str-10)*2
    base_attack_power = -20;

    initial_attack_power_per_strength = 2.0;
    // FIXME don't know level 60 value
    initial_attack_crit_per_agility   = rating_t::interpolate( level, 0.01/16.0, 0.01/30.0, 0.01/62.5 );

    base_attack_expertise = 0.25 * o -> talents.animal_handler * 0.05;

    base_attack_crit = 0.032 + talents.spiders_bite * 0.03;
    base_attack_crit += o -> talents.ferocity * 0.02;

    resource_base[ RESOURCE_HEALTH ] = rating_t::interpolate( level, 0, 4253, 6373 );
    resource_base[ RESOURCE_FOCUS  ] = 100;

    focus_regen_per_second  = ( 24.5 / 4.0 );
    focus_regen_per_second *= 1.0 + o -> talents.bestial_discipline * 0.50;
  }

  virtual double composite_attack_power()
  {
    hunter_t* o = owner -> cast_hunter();

    double ap = player_t::composite_attack_power();
    ap += o -> stamina() * o -> talents.hunter_vs_wild * 0.1;
    ap += o -> composite_attack_power() * 0.22;

    if( o -> active_aspect == ASPECT_BEAST )
      ap *= 1.1;
    if( buffs_rabid_power_stack )
      ap *= 1.0 + buffs_rabid_power_stack * 0.05;
    if( buffs_call_of_the_wild )
      ap *= 1.1;

    return ap;
  }

  virtual void summon()
  {
    hunter_t* o = owner -> cast_hunter();
    pet_t::summon();
    schedule_ready();
    o -> active_pet = this;
  }

  virtual int primary_resource() { return RESOURCE_FOCUS; }

  virtual bool parse_option( const std::string& name,
                             const std::string& value )
  {
    option_t options[] =
    {
      // Talents
      { "cobra_reflexes",   OPT_INT, &( talents.cobra_reflexes   ) },
      { "owls_focus",       OPT_INT, &( talents.owls_focus       ) },
      { "spiked_collar",    OPT_INT, &( talents.spiked_collar    ) },
      { "feeding_frenzy",   OPT_INT, &( talents.feeding_frenzy   ) },
      { "roar_of_recovery", OPT_INT, &( talents.roar_of_recovery ) },
      { "wolverine_bite",   OPT_INT, &( talents.wolverine_bite   ) },
      { "spiders_bite",     OPT_INT, &( talents.spiders_bite     ) },
      { "call_of_the_wild", OPT_INT, &( talents.call_of_the_wild ) },
      { "rabid",            OPT_INT, &( talents.rabid            ) },
      { NULL, OPT_UNKNOWN }
    };

    if( name.empty() )
    {
      pet_t::parse_option( std::string(), std::string() );
      option_t::print( sim, options );
      return false;
    }

    if( pet_t::parse_option( name, value ) ) return true;

    return option_t::parse( sim, options, name, value );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str );
};

// ==========================================================================
// Hunter Attack
// ==========================================================================

struct hunter_attack_t : public attack_t
{
  hunter_attack_t( const char* n, player_t* player, int s=SCHOOL_PHYSICAL, int t=TREE_NONE ) :
    attack_t( n, player, RESOURCE_MANA, s, t )
  {
    hunter_t* p = player -> cast_hunter();

    base_hit  += p -> talents.focused_aim * 0.01;
    base_crit += p -> talents.master_marksman * 0.01;
    base_crit += p -> talents.killer_instinct * 0.01;

    if( p -> position == POSITION_RANGED )
    {
      base_crit += p -> talents.lethal_shots * 0.01;
    }

    base_multiplier *= 1.0 + p -> talents.improved_tracking * 0.01;
  }

  virtual void add_ammunition()
  {
    hunter_t* p = player -> cast_hunter();

    //Spreadsheet says, it is weapon -> swing_time,
    //even if the ap contribution is normalized (only steady is the exception)
    //But I think that's illogical, dunno if somebody tested it sufficiently
    double weapon_speed = normalize_weapon_speed ? weapon -> normalized_weapon_speed() : weapon -> swing_time;
    double bonus_damage = p -> ammo_dps * weapon_speed * weapon_multiplier;

    base_dd_min += bonus_damage;
    base_dd_max += bonus_damage;
  }

  virtual void add_scope()
  {
    if( weapon -> enchant == SCOPE )
    {
      double bonus_damage = weapon -> enchant_bonus * weapon_multiplier;

      base_dd_min += bonus_damage;
      base_dd_max += bonus_damage;
    }
  }

  virtual double cost();
  virtual void   execute();
  virtual double execute_time();
  virtual void   player_buff();
};

// ==========================================================================
// Hunter Spell
// ==========================================================================

struct hunter_spell_t : public spell_t
{
  hunter_spell_t( const char* n, player_t* p, int s, int t ) :
    spell_t( n, p, RESOURCE_MANA, s, t )
  {
  }

  virtual double cost();
  virtual double gcd();
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// trigger_aspect_of_the_viper ==============================================

static void trigger_aspect_of_the_viper( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if( p -> active_aspect != ASPECT_VIPER )
    return;

  double gain = p -> resource_max[ RESOURCE_MANA ] * p -> ranged_weapon.swing_time / 100.0;

  if( p -> glyphs.aspect_of_the_viper ) gain *= 1.10;

  p -> resource_gain( RESOURCE_MANA, gain, p -> gains_viper_aspect_shot );
}

// trigger_cobra_strikes ===================================================

static void trigger_cobra_strikes( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.cobra_strikes )
    return;
  if ( ! a -> sim -> roll( p -> talents.cobra_strikes * 0.2 ) )
    return;

  struct cobra_strikes_expiration_t : public event_t
  {
    cobra_strikes_expiration_t( sim_t* sim, hunter_t* p ) : event_t( sim, p )
    {
      name = "Cobra Strikes Expiration";
      p -> aura_gain( "Cobra Strikes" );
      p -> buffs_cobra_strikes = 2;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      hunter_t* p = player -> cast_hunter();
      p -> aura_loss( "Cobra Strikes" );
      p -> buffs_cobra_strikes = 0;
      p -> expirations_cobra_strikes = 0;
    }
  };

  event_t*& e = p -> expirations_cobra_strikes;

  if ( e )
  {
    e -> reschedule( 10.0 );
    p -> buffs_cobra_strikes = 2;
  }
  else
  {
    e = new ( a -> sim ) cobra_strikes_expiration_t( a -> sim, p );
  }
}

// consume_cobra_strikes ===================================================

static void consume_cobra_strikes( action_t* a )
{
  hunter_pet_t* p = (hunter_pet_t*) a -> player -> cast_pet();
  hunter_t*     o = p -> owner -> cast_hunter();

  if( o -> buffs_cobra_strikes )
  {
    o -> buffs_cobra_strikes--;
    if( ! o -> buffs_cobra_strikes )
      event_t::early( o -> expirations_cobra_strikes );
  }
}

// trigger_expose_weakness =================================================

static void trigger_expose_weakness( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.expose_weakness )
    return;

  if ( ! a -> sim -> roll( p -> talents.expose_weakness / 3.0 ) )
    return;

  struct expose_weakness_expiration_t : public event_t
  {
    expose_weakness_expiration_t( sim_t* sim, hunter_t* p ) : event_t( sim, p )
    {
      name = "Expose Weakness Expiration";
      p -> aura_gain( "Expose Weakness" );
      p -> buffs_expose_weakness = 1;
      sim -> add_event( this, 7.0 );
    }
    virtual void execute()
    {
      hunter_t* p = player -> cast_hunter();
      p -> aura_loss( "Expose Weakness" );
      p -> buffs_expose_weakness = 0;
      p -> expirations_expose_weakness = 0;
    }
  };

  event_t*& e = p -> expirations_expose_weakness;

  if ( e )
  {
    e -> reschedule( 7.0 );
  }
  else
  {
    e = new ( a -> sim ) expose_weakness_expiration_t( a -> sim, p );
  }
}

// trigger_feeding_frenzy ==================================================

static void trigger_feeding_frenzy( action_t* a )
{
  hunter_pet_t* p = (hunter_pet_t*) a -> player -> cast_pet();

  if( p -> talents.feeding_frenzy )
  {
    double target_pct = a -> sim -> target -> health_percentage();

    if( target_pct < 35 )
    {
      a -> player_multiplier *= 1.0 + p -> talents.feeding_frenzy * 0.06;
    }
  }
}

// trigger_ferocious_inspiration ===================================

static void trigger_ferocious_inspiration( action_t* a )
{
  hunter_t* o = a -> player -> cast_pet() -> owner -> cast_hunter();

  if ( ! o -> talents.ferocious_inspiration ) return;

  // FIXME! Pretending FI is a target debuff helps performance.
  // FIXME! It also bypasses the issue with properly updating pet expirations when they are dismissed.

  struct ferocious_inspiration_expiration_t : public event_t
  {
    ferocious_inspiration_expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Ferocious Inspiration Expiration";
      if( sim -> log ) report_t::log( sim, "Everyone gains Ferocious Inspiration." );
      sim -> target -> debuffs.ferocious_inspiration = 1;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      if( sim -> log ) report_t::log( sim, "Everyone loses Ferocious Inspiration." );
      sim -> target -> debuffs.ferocious_inspiration = 0;
      sim -> target -> expirations.ferocious_inspiration = 0;
    }
  };

  target_t* t = a -> sim -> target;
  event_t*& e = t -> expirations.ferocious_inspiration;

  if ( e )
  {
    e -> reschedule( 10.0 );
  }
  else
  {
    e = new ( a -> sim ) ferocious_inspiration_expiration_t( a -> sim );
  }

}

// trigger_frenzy ==================================================

static void trigger_frenzy( action_t* a )
{
  hunter_pet_t* p = (hunter_pet_t*) a -> player -> cast_pet();
  hunter_t*     o = p -> owner -> cast_hunter();

  if ( ! o -> talents.frenzy ) return;
  if ( ! a -> sim -> roll( o -> talents.frenzy * 0.2 ) ) return;

  struct frenzy_expiration_t : public event_t
  {
    frenzy_expiration_t( sim_t* sim, hunter_pet_t* p ) : event_t( sim, p )
    {
      name = "Frenzy Expiration";
      p -> aura_gain( "Frenzy" );
      p -> buffs_frenzy = 1;
      sim -> add_event( this, 8.0 );
    }
    virtual void execute()
    {
      hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
      p -> aura_loss( "Frenzy" );
      p -> buffs_frenzy = 0;
      p -> expirations_frenzy = 0;
    }
  };

  event_t*& e = p -> expirations_frenzy;

  if ( e )
  {
    e -> reschedule( 8.0 );
  }
  else
  {
    e = new ( a -> sim ) frenzy_expiration_t( a -> sim, p );
  }
}

// trigger_go_for_the_throat ===============================================

static void trigger_go_for_the_throat( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.go_for_the_throat ) return;
  if ( ! p -> active_pet ) return;

  double gain = p -> talents.go_for_the_throat * 25.0;
  p -> active_pet -> resource_gain( RESOURCE_FOCUS, gain,
                                    p -> active_pet -> gains_go_for_the_throat );
}

// trigger_hunting_party ===================================================

static void trigger_hunting_party( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.hunting_party )
    return;

  if ( ! a -> sim -> roll( p -> talents.hunting_party * 0.20 ) )
    return;

  struct hunting_party_expiration_t : public event_t
  {
    hunting_party_expiration_t( sim_t* sim, hunter_t* h ) : event_t( sim, h )
    {
      name = "Hunting Party Expiration";
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
        p -> buffs.replenishment++;
      }
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
        p -> buffs.replenishment--;
      }
      hunter_t* h = player -> cast_hunter();
      h -> expirations_hunting_party = 0;
    }
  };

  event_t*& e = p -> expirations_hunting_party;

  if ( e )
  {
    e -> reschedule( 15.0 );
  }
  else
  {
    e = new ( a -> sim ) hunting_party_expiration_t( a -> sim, p );
  }
}

// trigger_improved_aspect_of_the_hawk =====================================

static void trigger_improved_aspect_of_the_hawk( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.improved_aspect_of_the_hawk )
    return;

  if ( p -> active_aspect != ASPECT_HAWK )
    return;

  if ( ! a -> sim -> roll( 0.10 ) )
    return;

  struct improved_aspect_of_the_hawk_expiration_t : public event_t
  {
    improved_aspect_of_the_hawk_expiration_t( sim_t* sim, hunter_t* p ) : event_t( sim, p )
    {
      name = "Improved Aspect of the Hawk Expiration";
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      hunter_t* p = player -> cast_hunter();
      p -> buffs_improved_aspect_of_the_hawk = 0;
      p -> expirations_improved_aspect_of_the_hawk = 0;
    }
  };

  p -> buffs_improved_aspect_of_the_hawk = 0.03 * p -> talents.improved_aspect_of_the_hawk;

  if( p -> glyphs.improved_aspect_of_the_hawk )
  {
    p -> buffs_improved_aspect_of_the_hawk += 0.06;
  }

  event_t*& e = p -> expirations_improved_aspect_of_the_hawk;

  if ( e )
  {
    e -> reschedule( 12.0 );
  }
  else
  {
    e = new ( a -> sim ) improved_aspect_of_the_hawk_expiration_t( a -> sim, p );
  }
}

// trigger_improved_steady_shot =====================================

static void trigger_improved_steady_shot( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.improved_steady_shot )
    return;

  if ( ! a -> sim -> roll( p -> talents.improved_steady_shot * 0.05 ) )
    return;

  struct improved_steady_shot_expiration_t : public event_t
  {
    improved_steady_shot_expiration_t( sim_t* sim, hunter_t* p ) : event_t( sim, p )
    {
      name = "Improved Steady Shot Expiration";
      p -> buffs_improved_steady_shot = 1;
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      hunter_t* p = player -> cast_hunter();
      p -> buffs_improved_steady_shot = 0;
      p -> expirations_improved_steady_shot = 0;
    }
  };

  event_t*& e = p -> expirations_improved_steady_shot;

  if ( e )
  {
    e -> reschedule( 12.0 );
  }
  else
  {
    e = new ( a -> sim ) improved_steady_shot_expiration_t( a -> sim, p );
  }
}

// consume_improved_steady_shot ======================================

static void consume_improved_steady_shot( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();
  event_t::early( p -> expirations_improved_steady_shot );
}

// trigger_invigoration ==============================================

static void trigger_invigoration( action_t* a )
{
  hunter_pet_t* p = (hunter_pet_t*) a -> player -> cast_pet();
  hunter_t*     o = p -> owner -> cast_hunter();

  if( ! o -> talents.invigoration )
    return;

  if( ! a -> sim -> roll( o -> talents.invigoration * 0.50 ) )
    return;

  o -> resource_gain( RESOURCE_MANA, 0.01 * o -> resource_max[ RESOURCE_MANA ], o -> gains_invigoration );
}

// consume_kill_command ==============================================

static void consume_kill_command( action_t* a )
{
  hunter_pet_t* p = (hunter_pet_t*) a -> player -> cast_pet();

  if( p -> buffs_kill_command > 0 )
  {
    p -> buffs_kill_command--;

    if( p -> buffs_kill_command == 0 )
    {
      p -> aura_loss( "Kill Command" );
    }
  }
}

// trigger_lock_and_load =============================================

static void trigger_lock_and_load( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if( ! p -> talents.lock_and_load )
    return;
  if( ! a -> sim -> cooldown_ready( p -> cooldowns_lock_and_load ) )
    return;

  // NB: talent calc says 3%,7%,10%, assuming it's really 10% * (1/3,2/3,3/3)
  if ( ! a -> sim -> roll( p -> talents.lock_and_load * 0.1 / 3 ) )
    return;

  struct lock_and_load_expiration_t : public event_t
  {
    lock_and_load_expiration_t( sim_t* sim, hunter_t* p ) : event_t( sim, p )
    {
      name = "Lock and Load Expiration";
      p -> aura_gain( "Lock and Load" );
      p -> cooldowns_lock_and_load = sim -> current_time + 30;
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      hunter_t* p = player -> cast_hunter();
      p -> aura_loss( "Lock and Load" );
      p -> buffs_lock_and_load = 0;
      p -> expirations_lock_and_load = 0;
    }
  };

  p -> buffs_lock_and_load = 2;

  event_t*& e = p -> expirations_lock_and_load;

  if ( e )
  {
    e -> reschedule( 12.0 );
  }
  else
  {
    e = new ( a -> sim ) lock_and_load_expiration_t( a -> sim, p );
  }
}

// consume_lock_and_load ==================================================

static void consume_lock_and_load( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if( p -> buffs_lock_and_load > 0 )
  {
    p -> buffs_lock_and_load--;

    if( p -> buffs_lock_and_load == 0 )
    {
      event_t::early( p -> expirations_lock_and_load );
    }
  }
}

// trigger_master_tactician ================================================

static void trigger_master_tactician( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.master_tactician )
    return;

  if ( ! a -> sim -> roll( 0.10 ) )
    return;

  struct master_tactician_expiration_t : public event_t
  {
    master_tactician_expiration_t( sim_t* sim, hunter_t* p ) : event_t( sim, p )
    {
      name = "Master Tactician Expiration";
      p -> aura_gain( "Master Tactician" );
      sim -> add_event( this, 8.0 );
    }
    virtual void execute()
    {
      hunter_t* p = player -> cast_hunter();
      p -> aura_loss( "Master Tactician" );
      p -> buffs_master_tactician = 0;
      p -> expirations_master_tactician = 0;
    }
  };

  p -> buffs_master_tactician = p -> talents.master_tactician * 0.02;

  event_t*& e = p -> expirations_master_tactician;

  if ( e )
  {
    e -> reschedule( 8.0 );
  }
  else
  {
    e = new ( a -> sim ) master_tactician_expiration_t( a -> sim, p );
  }
}

// trigger_owls_focus ================================================

static void trigger_owls_focus( action_t* a )
{
  hunter_pet_t* p = (hunter_pet_t*) a -> player -> cast_pet();

  if( ! p -> talents.owls_focus )
    return;

  if( ! a -> sim -> roll( p -> talents.owls_focus * 0.15 ) )
    return;

  struct owls_focus_expiration_t : public event_t
  {
    owls_focus_expiration_t( sim_t* sim, hunter_pet_t* p ) : event_t( sim, p )
    {
      name = "Owls Focus Expiration";
      p -> aura_gain( "Owls Focus" );
      p -> buffs_owls_focus = 1;
      sim -> add_event( this, 8.0 );
    }
    virtual void execute()
    {
      hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
      p -> aura_loss( "Owls Focus" );
      p -> buffs_owls_focus = 0;
      p -> expirations_owls_focus = 0;
    }
  };
  
  event_t*& e = p -> expirations_owls_focus;

  if( e )
  {
    e -> reschedule( 8.0 );
  }
  else
  {
    e = new ( a -> sim ) owls_focus_expiration_t( a -> sim, p );
  }
}

// consume_owls_focus ================================================

static void consume_owls_focus( action_t* a )
{
  hunter_pet_t* p = (hunter_pet_t*) a -> player -> cast_pet();
  event_t::early( p -> expirations_owls_focus );
}

// trigger_rabid_power ===============================================

static void trigger_rabid_power( attack_t* a )
{
  hunter_pet_t* p = (hunter_pet_t*) a -> player -> cast_pet();

  if ( ! p -> talents.rabid )
    return;
  if ( ! p -> buffs_rabid )
    return;
  if ( p -> buffs_rabid_power_stack == 5 || ! a -> sim -> roll( 0.5 ) )
    return;

  p -> buffs_rabid_power_stack++;
}

// trigger_thrill_of_the_hunt ========================================

static void trigger_thrill_of_the_hunt( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.thrill_of_the_hunt )
    return;

  if ( ! a -> sim -> roll( p -> talents.thrill_of_the_hunt / 3.0 ) )
    return;

  p -> resource_gain( RESOURCE_MANA, a -> resource_consumed * 0.40, p -> gains_thrill_of_the_hunt );
}

// trigger_wild_quiver ===============================================

static void trigger_wild_quiver( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if( a -> proc )
    return;

  if( ! p -> talents.wild_quiver )
    return;

  if(  a -> sim -> roll( util_t::talent_rank( p -> talents.wild_quiver, 3, 0.04, 0.07, 0.10 ) ) )
  {
    // FIXME! What hit/crit talents apply?
    // FIXME! Which proc-related talents can it trigger?
    // FIXME! Currently coded to benefit from all talents affecting ranged attacks.
    // FIXME! Talents that do not affect it can filter on "proc=true".

    struct wild_quiver_t : public attack_t
    {
      wild_quiver_t( player_t* p ) : attack_t( "wild_quiver", p, RESOURCE_NONE, SCHOOL_NATURE )
      {
        background  = true;
        proc        = true;
        trigger_gcd = 0;
        base_cost   = 0;
      }
    };

    if( ! p -> active_wild_quiver ) p -> active_wild_quiver = new wild_quiver_t( p );

    p -> procs_wild_quiver -> occur();

    p -> active_wild_quiver -> base_direct_dmg = 0.50 * a -> direct_dmg;
    p -> active_wild_quiver -> execute();
  }
}

} // ANONYMOUS NAMESPACE ===================================================

// =========================================================================
// Hunter Pet Attacks
// =========================================================================

struct hunter_pet_attack_t : public attack_t
{
  bool special;

  hunter_pet_attack_t( const char* n, player_t* player, int r=RESOURCE_FOCUS, int sc=SCHOOL_PHYSICAL, bool sp=true ) :
    attack_t( n, player, r, sc, TREE_BEAST_MASTERY ), special(sp)
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    may_glance = ! special;

    if( p -> group() == PET_FEROCITY ) base_multiplier *= 1.10;
    if( p -> group() == PET_CUNNING  ) base_multiplier *= 1.05;

    // Assume happy pet
    base_multiplier *= 1.25;

    base_multiplier *= 1.0 + p -> talents.spiked_collar * 0.03;
    base_multiplier *= 1.0 + o -> talents.unleashed_fury * 0.03;
    base_multiplier *= 1.0 + o -> talents.kindred_spirits * 0.03;
  }

  virtual double execute_time()
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    double t = attack_t::execute_time();
    if( t == 0 ) return 0;

    if( o -> talents.frenzy )
    {
      if( p -> buffs_frenzy ) t *= 1.0 / 1.3;

      p -> uptimes_frenzy -> update( p -> buffs_frenzy );
    }

    return t;
  }

  virtual double cost()
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    if( p -> buffs_owls_focus ) return 0;
    return attack_t::cost();
  }

  virtual void consume_resource()
  {
    attack_t::consume_resource();
    if( special && base_cost > 0 )
    {
      consume_owls_focus( this );
    }
  }

  virtual void execute()
  {
    attack_t::execute();
    if( result_is_hit() )
    {
      trigger_rabid_power( this );

      if( result == RESULT_CRIT )
      {
        trigger_ferocious_inspiration( this );
        trigger_frenzy( this );
	if( special ) trigger_invigoration( this );
      }
    }
    if( special )
    {
      consume_cobra_strikes( this );
      consume_kill_command( this );
      trigger_owls_focus( this );
    }
  }

  virtual void player_buff()
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    attack_t::player_buff();

    if( p -> buffs_bestial_wrath ) player_multiplier *= 1.5;

    trigger_feeding_frenzy( this );

    if( special )
    {
      if( o -> buffs_cobra_strikes ) player_crit += 1.0;

      if( p -> buffs_kill_command )
      {
	player_multiplier *= 1.0 + p -> buffs_kill_command * 0.20;
	player_crit       += o -> talents.focused_fire * 0.10;
      }
    }
  }
};

// Pet Melee =================================================================

struct pet_melee_t : public hunter_pet_attack_t
{
  pet_melee_t( player_t* player ) :
    hunter_pet_attack_t( "melee", player, RESOURCE_NONE, SCHOOL_PHYSICAL, false )
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    weapon = &( p -> main_hand_weapon );
    base_execute_time  = weapon -> swing_time;
    base_direct_dmg    = 1;
    background         = true;
    repeating          = true;
    may_glance         = true;

    if( p -> talents.cobra_reflexes )
    {
      base_multiplier   *= ( 1.0 - p -> talents.cobra_reflexes * 0.075 );
      base_execute_time *= 1.0 / ( 1.0 + p -> talents.cobra_reflexes * 0.15 );
    }
    base_execute_time *= 1.0 / ( 1.0 + 0.02 * o -> talents.serpents_swiftness );

    if( o -> gear.tier7_2pc ) base_multiplier *= 1.05;
  }
};

// Pet Auto Attack ==========================================================

struct pet_auto_attack_t : public hunter_pet_attack_t
{
  pet_auto_attack_t( player_t* player, const std::string& options_str ) :
    hunter_pet_attack_t( "auto_attack", player )
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    p -> main_hand_attack = new pet_melee_t( player );
  }

  virtual void execute()
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    p -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Focus Dump ================================================================

struct focus_dump_t : public hunter_pet_attack_t
{
  focus_dump_t( player_t* player, const std::string& options_str, const char* n ) :
    hunter_pet_attack_t( n, player, RESOURCE_FOCUS, SCHOOL_PHYSICAL )
  {
    parse_options( 0, options_str );
    base_cost       = 25;
    base_direct_dmg = 143;
  }
};

// Cat Rake ===================================================================

struct rake_t : public hunter_pet_attack_t
{
  rake_t( player_t* player, const std::string& options_str ) :
    hunter_pet_attack_t( "rake", player, RESOURCE_FOCUS, SCHOOL_BLEED )
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    assert( p -> pet_type == PET_CAT );

    parse_options( 0, options_str );

    base_cost       = 20;
    base_direct_dmg = 57;
    base_td_init    = 21;
    num_ticks       = 3;
    base_tick_time  = 3;
    tick_power_mod  = 0.0175;
    cooldown        = 10 * ( 1.0 - o -> talents.longevity * 0.10 );
  }

  virtual void execute()
  {
    // FIXME! Assuming pets are not smart enough to wait for Rake to finish ticking
    if( ticking ) cancel();
    school = SCHOOL_PHYSICAL;
    hunter_pet_attack_t::execute();
    school = SCHOOL_BLEED;
  }

  virtual void update_ready()
  {
    hunter_pet_attack_t::update_ready();
    duration_ready = 0;
  }
};

// =========================================================================
// Hunter Pet Spells
// =========================================================================

struct hunter_pet_spell_t : public spell_t
{
  hunter_pet_spell_t( const char* n, player_t* player, int r=RESOURCE_FOCUS, int s=SCHOOL_PHYSICAL ) :
    spell_t( n, player, r, s )
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();

    if( p -> group() == PET_FEROCITY ) base_multiplier *= 1.10;
    if( p -> group() == PET_CUNNING  ) base_multiplier *= 1.05;

    base_multiplier *= 1.0 + p -> talents.spiked_collar * 0.03;
  }

  virtual double cost()
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    if( p -> buffs_owls_focus ) return 0;
    return spell_t::cost();
  }

  virtual void consume_resource()
  {
    spell_t::consume_resource();
    if( base_cost > 0 )
    {
      consume_owls_focus( this );
    }
  }

  virtual void execute()
  {
    spell_t::execute();
    if( result_is_hit() )
    {
      if( result == RESULT_CRIT )
      {
	trigger_invigoration( this );
      }
    }
    consume_cobra_strikes( this );
    consume_kill_command( this );
    trigger_owls_focus( this );
  }

  virtual void player_buff()
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    spell_t::player_buff();

    player_power += 0.125 * o -> composite_spell_power( SCHOOL_MAX );

    if( p -> buffs_bestial_wrath ) player_multiplier *= 1.5;

    if( o -> buffs_cobra_strikes ) player_crit += 1.0;

    if( p -> buffs_kill_command )
    {
      player_multiplier *= 1.0 + p -> buffs_kill_command * 0.20;
      player_crit       += o -> talents.focused_fire * 0.10;
    }

    trigger_feeding_frenzy( this );
  }
};

// Chimera Froststorm Breath ================================================

struct froststorm_breath_t : public hunter_pet_spell_t
{
  froststorm_breath_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "froststorm_breath", player, RESOURCE_FOCUS, SCHOOL_NATURE )
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    assert( p -> pet_type == PET_CHIMERA );

    parse_options( 0, options_str );

    base_cost        = 20;
    base_direct_dmg  = 150;
    direct_power_mod = 1.5 / 3.5;
    cooldown         = 10 * ( 1.0 - o -> talents.longevity * 0.10 );
  }
};

// Wind Serpent Lightning Breath ================================================

struct lightning_breath_t : public hunter_pet_spell_t
{
  lightning_breath_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "lightning_breath", player, RESOURCE_FOCUS, SCHOOL_NATURE )
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    assert( p -> pet_type == PET_WIND_SERPENT );

    parse_options( 0, options_str );

    base_cost        = 20;
    base_direct_dmg  = 100;
    direct_power_mod = 1.5 / 3.5;
    cooldown         = 10 * ( 1.0 - o -> talents.longevity * 0.10 );
  }
};

// Call of the Wild ===========================================================

struct call_of_the_wild_t : public hunter_pet_spell_t
{
  call_of_the_wild_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "call_of_the_wild", player, RESOURCE_FOCUS )
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    assert( p -> talents.call_of_the_wild );

    parse_options( 0, options_str );

    base_cost = 0;
    cooldown  = 5 * 60 * ( 1.0 - o -> talents.longevity * 0.10 );
    trigger_gcd = 0.0;
  }

  virtual void execute()
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();

    struct cotw_expiration_t : public event_t
    {
      cotw_expiration_t( sim_t* sim, hunter_pet_t* p ) : event_t( sim, p )
      {
        hunter_t*     o = p -> owner -> cast_hunter();
        name = "Call of the Wild Expiration";
        p -> aura_gain( "Call of the Wild" );
        p -> buffs_call_of_the_wild = 1;
        o -> aura_gain( "Call of the Wild" );
        o -> buffs_call_of_the_wild = 1;
        sim -> add_event( this, 20.0 );
      }
      virtual void execute()
      {
        hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
        hunter_t*     o = p -> owner -> cast_hunter();
        p -> aura_loss( "Call of the Wild" );
        p -> buffs_call_of_the_wild = 0;
        o -> aura_loss( "Call of the Wild" );
        o -> buffs_call_of_the_wild = 0;
      }
    };

    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    update_ready();
    player -> action_finish( this );
    new ( sim ) cotw_expiration_t( sim, p );
  }
};

// Rabid ======================================================================

struct rabid_t : public hunter_pet_spell_t
{
  rabid_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "rabid", player, RESOURCE_FOCUS )
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    assert( p -> talents.rabid );

    parse_options( 0, options_str );

    base_cost = 0;
    cooldown  = 45 * ( 1.0 - o -> talents.longevity * 0.10 );
    trigger_gcd = 0.0;
  }

  virtual void execute()
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();

    struct rabid_expiration_t : public event_t
    {
      rabid_expiration_t( sim_t* sim, hunter_pet_t* p ) : event_t( sim, p )
      {
        name = "Rabid Expiration";
        p -> aura_gain( "Rabid" );
        p -> buffs_rabid = 1;
        assert( p -> buffs_rabid_power_stack == 0 );
        sim -> add_event( this, 20.0 );
      }
      virtual void execute()
      {
        hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
        p -> aura_loss( "Rabid" );
        p -> buffs_rabid = 0;
        p -> buffs_rabid_power_stack = 0;
      }
    };

    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    update_ready();
    player -> action_finish( this );
    new ( sim ) rabid_expiration_t( sim, p );
  }
};

// Roar of Recovery ===========================================================

struct roar_of_recovery_t : public hunter_pet_spell_t
{
  roar_of_recovery_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "roar_of_recovery", player, RESOURCE_FOCUS )
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    assert( p -> talents.roar_of_recovery );

    parse_options( 0, options_str );

    trigger_gcd    = 0.0;
    base_cost      = 0;
    num_ticks      = 3;
    base_tick_time = 3;
    cooldown       = 360 * ( 1.0 - o -> talents.longevity * 0.10 );
  }

  virtual void tick()
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    o -> resource_gain( RESOURCE_MANA, 0.10 * o -> resource_max[ RESOURCE_MANA ], o -> gains_roar_of_recovery );
  }

  virtual bool ready()
  {
    hunter_pet_t* p = (hunter_pet_t*) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    if( ( o -> resource_current[ RESOURCE_MANA ] /
	  o -> resource_max    [ RESOURCE_MANA ] ) > 0.50 )
      return false;

    return hunter_pet_spell_t::ready();
  }
};

// hunter_pet_t::create_action =============================================

action_t* hunter_pet_t::create_action( const std::string& name,
                                       const std::string& options_str )
{
  if( name == "auto_attack"       ) return new   pet_auto_attack_t( this, options_str );
  if( name == "bite"              ) return new        focus_dump_t( this, options_str, "bite" );
  if( name == "call_of_the_wild"  ) return new  call_of_the_wild_t( this, options_str );
  if( name == "claw"              ) return new        focus_dump_t( this, options_str, "claw" );
  if( name == "froststorm_breath" ) return new froststorm_breath_t( this, options_str );
  if( name == "lightning_breath"  ) return new  lightning_breath_t( this, options_str );
  if( name == "rabid"             ) return new             rabid_t( this, options_str );
  if( name == "rake"              ) return new              rake_t( this, options_str );
  if( name == "roar_of_recovery"  ) return new  roar_of_recovery_t( this, options_str );
  if( name == "smack"             ) return new        focus_dump_t( this, options_str, "smack" );

  return pet_t::create_action( name, options_str );
}

// =========================================================================
// Hunter Attacks
// =========================================================================

// hunter_attack_t::cost ===================================================

double hunter_attack_t::cost()
{
  hunter_t* p = player -> cast_hunter();
  double c = attack_t::cost();
  if( c == 0 ) return 0;
  c *= 1.0 - p -> talents.efficiency * 0.02;
  if( p -> buffs_rapid_fire   ) c *= 1.0 - p -> talents.rapid_recuperation * 0.3;
  if( p -> buffs_beast_within ) c *= 0.80;
  return c;
}

// hunter_attack_t::execute ================================================

void hunter_attack_t::execute()
{
  attack_t::execute();

  if( result_is_hit() )
  {
    trigger_aspect_of_the_viper( this );
    trigger_master_tactician( this );

    if( result == RESULT_CRIT )
    {
      trigger_expose_weakness( this );
      trigger_go_for_the_throat( this );
      trigger_thrill_of_the_hunt( this );
    }
  }
}

// hunter_attack_t::execute_time ============================================

double hunter_attack_t::execute_time()
{
  hunter_t* p = player -> cast_hunter();

  double t = attack_t::execute_time();
  if( t == 0 ) return 0;

  if( p -> buffs_rapid_fire )
  {
    t *= 1.0 / ( 1.0 + p -> buffs_rapid_fire );
  }

  t *= 1.0 / p -> quiver_haste;

  t *= 1.0 / ( 1.0 + 0.04 * p -> talents.serpents_swiftness );

  t *= 1.0 / ( 1.0 + p -> buffs_improved_aspect_of_the_hawk );

  p -> uptimes_improved_aspect_of_the_hawk -> update( p -> buffs_improved_aspect_of_the_hawk != 0 );

  return t;
}

// hunter_attack_t::player_buff =============================================

void hunter_attack_t::player_buff()
{
  hunter_t* p = player -> cast_hunter();
  target_t* t = sim -> target;

  attack_t::player_buff();

  if( t -> debuffs.hunters_mark )
  {
    if( weapon && weapon -> group() == WEAPON_RANGED )
    {
      // FIXME: This should be all shots, not just weapon-based shots
      player_multiplier *= 1.0 + p -> talents.marked_for_death * 0.01;
    }
  }
  if( p -> buffs_aspect_of_the_viper )
  {
    player_multiplier *= p -> buffs_aspect_of_the_viper;
  }
  if( p -> buffs_beast_within )
  {
    player_multiplier *= 1.10;
  }
  if( p -> active_sting() && p -> talents.noxious_stings )
  {
    player_multiplier *= 1.0 + p -> talents.noxious_stings * 0.01;
  }
  if( p -> active_pet )
  {
    player_multiplier *= 1.0 + p -> talents.focused_fire * 0.01;
  }
  player_crit += p -> buffs_master_tactician;

  p -> uptimes_aspect_of_the_viper -> update( p -> buffs_aspect_of_the_viper != 0 );
  p -> uptimes_master_tactician    -> update( p -> buffs_master_tactician    != 0 );
}

// Ranged Attack ===========================================================

struct ranged_t : public hunter_attack_t
{
  ranged_t( player_t* player ) :
    hunter_attack_t( "ranged", player )
  {
    hunter_t* p = player -> cast_hunter();

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );
    base_execute_time = weapon -> swing_time;

    background  = true;
    repeating   = true;
    trigger_gcd = 0;
    base_cost   = 0;
    base_dd_min = 1;
    base_dd_max = 1;

    base_multiplier *= 1.0 + p -> talents.ranged_weapon_specialization * 0.01;

    add_ammunition();
    add_scope();
  }

  void execute()
  {
    hunter_attack_t::execute();

    if( result_is_hit() )
    {
      trigger_wild_quiver( this );
      trigger_improved_aspect_of_the_hawk( this ); // FIXME! Only procs on auto-shot?
    }
  }
};

// Auto Shot =================================================================

struct auto_shot_t : public hunter_attack_t
{
  auto_shot_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "auto_shot", player )
  {
    hunter_t* p = player -> cast_hunter();
    p -> ranged_attack = new ranged_t( player );
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    p -> ranged_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();
    return( p -> ranged_attack -> execute_event == 0 ); // not swinging
  }
};

// Aimed Shot ================================================================

struct aimed_shot_t : public hunter_attack_t
{
  int improved_steady_shot;

  aimed_shot_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "aimed_shot", player, SCHOOL_PHYSICAL, TREE_MARKSMANSHIP ),
    improved_steady_shot(0)
  {
    hunter_t* p = player -> cast_hunter();
    assert( p -> talents.aimed_shot );

    option_t options[] =
    {
      { "improved_steady_shot", OPT_INT, &improved_steady_shot },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 9, 408, 408, 0, 0.08 },
      { 75, 8, 345, 345, 0, 0.08 },
      { 70, 7, 205, 205, 0, 0.08 },
      { 60, 6, 150, 150, 0, 0.12 },
      { 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    normalize_weapon_speed  = true;

    cooldown = 10;
    cooldown_group = "aimed_multi";

    base_multiplier *= 1.0 + p -> talents.barrage                      * 0.04;
    base_multiplier *= 1.0 + p -> talents.sniper_training              * 0.02;
    base_multiplier *= 1.0 + p -> talents.ranged_weapon_specialization * 0.01;

    base_crit += p -> talents.improved_barrage * 0.04;

    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.mortal_shots     * 0.06 +
                                          p -> talents.marked_for_death * 0.02 );

    base_penetration += p -> talents.piercing_shots * 0.02;

    add_ammunition();
    add_scope();
  }

  virtual double cost()
  {
    hunter_t* p = player -> cast_hunter();
    double c = hunter_attack_t::cost();
    if( p -> buffs_improved_steady_shot ) c *= 0.80;
    return c;
  }

  void player_buff()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::player_buff();
    if( p -> buffs_trueshot_aura && p -> glyphs.trueshot_aura ) player_crit += 0.10;
    if( p -> buffs_improved_steady_shot ) player_multiplier *= 1.15;
    p -> uptimes_improved_steady_shot -> update( p -> buffs_improved_steady_shot != 0 );
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    consume_improved_steady_shot( this );
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if( improved_steady_shot )
      if( ! p -> buffs_improved_steady_shot )
        return false;

    return hunter_attack_t::ready();
  }
};

// Arcane Shot Attack =========================================================

struct arcane_shot_t : public hunter_attack_t
{
  int improved_steady_shot;
  int lock_and_load;

  arcane_shot_t(player_t* player, const std::string& options_str ) :
    hunter_attack_t( "arcane_shot", player, SCHOOL_ARCANE, TREE_MARKSMANSHIP ),
    improved_steady_shot(0), lock_and_load(0)
  {
    hunter_t* p = player -> cast_hunter();

    option_t options[] =
    {
      { "improved_steady_shot", OPT_INT, &improved_steady_shot },
      { "lock_and_load",        OPT_INT, &lock_and_load        },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79, 11, 492, 492, 0, 0.05 },
      { 73, 10, 402, 402, 0, 0.05 },
      { 69, 9,  273, 273, 0, 0.05 },
      { 60, 8,  200, 200, 0, 0.07 },
      { 0, 0 }
    };
    init_rank( ranks );

    cooldown = 6;
    cooldown_group = "arcane_explosive";

    direct_power_mod = 0.15;

    base_multiplier *= 1.0 + ( p -> talents.improved_arcane_shot  * 0.05 +
                               p -> talents.ferocious_inspiration * 0.03 );
    base_multiplier *= 1.0 + p -> talents.ranged_weapon_specialization * 0.01;

    base_crit += p -> talents.survival_instincts * 0.02;

    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.mortal_shots     * 0.06 +
                                          p -> talents.marked_for_death * 0.02 );

    // FIXME! Ranged Weapon Specialization excluded due to no weapon damage.  Correct?
  }

  virtual double cost()
  {
    hunter_t* p = player -> cast_hunter();
    if ( p -> buffs_lock_and_load ) return 0;
    double c = hunter_attack_t::cost();
    if( p -> buffs_improved_steady_shot ) c *= 0.80;
    return c;
  }

  void player_buff()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::player_buff();
    if( p -> buffs_improved_steady_shot ) player_multiplier *= 1.15;
    p -> uptimes_improved_steady_shot -> update( p -> buffs_improved_steady_shot );
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    if( result == RESULT_CRIT )
    {
      trigger_cobra_strikes( this );
      trigger_hunting_party( this );
    }
    consume_lock_and_load( this );
    consume_improved_steady_shot( this );
  }

  virtual void update_ready()
  {
    hunter_t* p = player -> cast_hunter();
    cooldown = p -> buffs_lock_and_load ? 0.0 : 6.0;
    hunter_attack_t::update_ready();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if( improved_steady_shot )
      if( ! p -> buffs_improved_steady_shot )
        return false;

    if( lock_and_load )
      if( ! p -> buffs_lock_and_load )
        return false;

    return hunter_attack_t::ready();
  }
};

// Chimera Shot ================================================================

struct chimera_shot_t : public hunter_attack_t
{
  int active_sting;
  int improved_steady_shot;

  chimera_shot_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "chimera_shot", player, SCHOOL_NATURE, TREE_MARKSMANSHIP ),
    active_sting(1), improved_steady_shot(0)
  {
    hunter_t* p = player -> cast_hunter();
    assert( p -> talents.chimera_shot );

    option_t options[] =
    {
      { "active_sting",         OPT_INT, &active_sting         },
      { "improved_steady_shot", OPT_INT, &improved_steady_shot },
      { NULL }
    };
    parse_options( options, options_str );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    base_dd_min = 1;
    base_dd_max = 1;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.16;

    normalize_weapon_speed = true;
    weapon_multiplier      = 1.25;
    cooldown               = 10;

    base_multiplier *= 1.0 + p -> talents.ranged_weapon_specialization * 0.01;

    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.mortal_shots     * 0.06 +
                                          p -> talents.marked_for_death * 0.02 );

    add_ammunition();
    add_scope();
  }

  virtual double cost()
  {
    hunter_t* p = player -> cast_hunter();
    double c = hunter_attack_t::cost();
    if( p -> buffs_improved_steady_shot ) c *= 0.80;
    return c;
  }

  void player_buff()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::player_buff();
    if( p -> buffs_improved_steady_shot ) player_multiplier *= 1.0 + 0.15;
    p -> uptimes_improved_steady_shot -> update( p -> buffs_improved_steady_shot );
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();

    hunter_attack_t::execute();

    action_t* sting = p -> active_sting();

    if( sting )
    {
      sting -> refresh_duration();

      double sting_dmg = sting -> num_ticks * sting -> calculate_tick_damage();

      if( p -> active_serpent_sting )
      {
        struct chimera_serpent_t : public hunter_attack_t
        {
          chimera_serpent_t( player_t* p ) : hunter_attack_t( "chimera_serpent", p, SCHOOL_NATURE, TREE_MARKSMANSHIP )
          {
            // FIXME! Which talents benefit this attack?
            // FIXME! Which procs can be triggered by this attack?
            background  = true;
            proc        = true;
            trigger_gcd = 0;
            base_cost   = 0;
            direct_power_mod = 0;
            // FIXME! Assuming this proc cannot miss.
            may_miss = false;
            may_crit = true;
          }
        };

        if( ! p -> active_chimera_serpent ) p -> active_chimera_serpent = new chimera_serpent_t( p );

        p -> active_chimera_serpent -> base_direct_dmg = 0.40 * sting_dmg;
        p -> active_chimera_serpent -> execute();

      }
      else if( p -> active_viper_sting )
      {
        p -> resource_gain( RESOURCE_MANA, 0.60 * sting_dmg, p -> gains_chimera_viper );
      }
    }
    consume_improved_steady_shot( this );
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if( active_sting )
      if( ! p -> active_sting() )
        return false;

    if( improved_steady_shot )
      if( ! p -> buffs_improved_steady_shot )
        return false;

    return hunter_attack_t::ready();
  }
};

// Explosive Shot ================================================================

struct explosive_shot_t : public hunter_attack_t
{
  int lock_and_load;

  explosive_shot_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "explosive_shot", player, SCHOOL_FIRE, TREE_SURVIVAL ),
    lock_and_load(0)
  {
    hunter_t* p = player -> cast_hunter();
    assert( p -> talents.explosive_shot );

    option_t options[] =
    {
      { "lock_and_load", OPT_INT, &lock_and_load },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 4,  428, 516, 0, 0.07 },
      { 75, 3,  361, 435, 0, 0.07 },
      { 70, 2,  245, 295, 0, 0.07 },
      { 60, 1,  160, 192, 0, 0.10 },
      { 0, 0 }
    };
    init_rank( ranks );

    cooldown = 6;
    cooldown_group = "arcane_explosive";

    num_ticks      = 2;
    base_tick_time = 1.0;
    direct_power_mod = 0.16;
    tick_power_mod = 0.16;

    base_multiplier *= 1.0 + p -> talents.sniper_training * 0.02;
    base_multiplier *= 1.0 + p -> talents.ranged_weapon_specialization * 0.01;

    base_crit += p -> talents.survival_instincts * 0.02;
    base_crit += p -> talents.tnt * 0.03;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.mortal_shots * 0.06;
  }

  virtual double cost()
  {
    // FIXME: ugly workaround to avoid paying triple
    if ( !may_miss ) return 0;

    hunter_t* p = player -> cast_hunter();
    if ( p -> buffs_lock_and_load ) return 0;
    return hunter_attack_t::cost();
  }

  virtual void consume_resource()
  {
    hunter_attack_t::consume_resource();
    // FIXME! Thrill of the Hunt proc reduced to 33% for each tick
    resource_consumed /= 3.0;
  }

  virtual void execute()
  {
    // FIXME! Bypass hunter_attack_t procs and just schedule ticks.  Only initial shot can miss.
    hunter_attack_t::execute();
    consume_lock_and_load( this );
    if( result == RESULT_CRIT )
    {
      trigger_hunting_party( this );
    }
  }

  virtual void tick()
  {
    // FIXME! Explosive Shot appears to be generating "on-hit" and "on-crit" procs for each tick.
    // FIXME! To handle this, each "tick" is modeled as an attack that cannot miss.
    may_miss = false;
    hunter_attack_t::execute();
    if( result == RESULT_CRIT )
    {
      trigger_hunting_party( this );
    }
    may_miss = true;
  }

  virtual void update_ready()
  {
    if( may_miss ) // Explosive charge was just fired
    {
      hunter_t* p = player -> cast_hunter();
      cooldown = p -> buffs_lock_and_load ? 0.0 : 6.0;
      hunter_attack_t::update_ready();
    }
  }

  virtual void update_stats( int type )
  {
      tick_dmg = direct_dmg;
    direct_dmg = 0;

    if( may_miss ) // Explosive charge was just fired
    {
      hunter_attack_t::update_stats( DMG_DIRECT );
    }
    hunter_attack_t::update_stats( DMG_OVER_TIME );
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if( lock_and_load )
      if( ! p -> buffs_lock_and_load )
        return false;

    return hunter_attack_t::ready();
  }
};

// Kill Shot =================================================================

struct kill_shot_t : public hunter_attack_t
{
  kill_shot_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "kill_shot", player, SCHOOL_PHYSICAL, TREE_MARKSMANSHIP )
  {
    hunter_t* p = player -> cast_hunter();

    static rank_t ranks[] =
    {
      { 80, 3, 650, 650, 0, 0.07 },
      { 75, 2, 500, 500, 0, 0.07 },
      { 71, 1, 410, 410, 0, 0.07 },
      { 0, 0 }
    };
    init_rank( ranks );

    parse_options( 0, options_str );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    normalize_weapon_speed = true;
    weapon_multiplier      = 2.0;
    cooldown               = 15;
    trigger_gcd            = 0.0;

    base_multiplier *= 1.0 + p -> talents.ranged_weapon_specialization * 0.01;

    base_crit += p -> talents.sniper_training * 0.05;

    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.mortal_shots     * 0.06 +
                                          p -> talents.marked_for_death * 0.02 );

    add_ammunition();
    add_scope();
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    if( result == RESULT_CRIT )
    {
      trigger_cobra_strikes( this );
    }
  }

  virtual bool ready()
  {
    if( sim -> target -> health_percentage() > 20 )
      return false;

    return hunter_attack_t::ready();
  }
};

// Multi Shot Attack =========================================================

struct multi_shot_t : public hunter_attack_t
{
  multi_shot_t(player_t* player, const std::string& options_str ) :
    hunter_attack_t("multi_shot", player, SCHOOL_PHYSICAL, TREE_MARKSMANSHIP )
  {
    hunter_t* p = player -> cast_hunter();
    assert( p -> ranged_weapon.type != WEAPON_NONE );

    static rank_t ranks[] =
    {
      { 80, 8, 408, 408, 0, 0.09 },
      { 74, 7, 333, 333, 0, 0.09 },
      { 67, 6, 205, 205, 0, 0.09 },
      { 60, 5, 150, 150, 0, 0.13 },
      { 0, 0 }
    };
    init_rank( ranks );

    parse_options( 0, options_str );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    normalize_weapon_speed = true;
    cooldown               = 10;
    cooldown_group         = "aimed_multi";

    base_multiplier *= 1.0 + p -> talents.barrage                      * 0.04;
    base_multiplier *= 1.0 + p -> talents.ranged_weapon_specialization * 0.01;

    base_crit += p -> talents.improved_barrage * 0.04;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.mortal_shots * 0.06;

    add_ammunition();
    add_scope();
  }
};

// Scatter Shot Attack =========================================================

struct scatter_shot_t : public hunter_attack_t
{
  scatter_shot_t(player_t* player, const std::string& options_str ) :
    hunter_attack_t("scatter_shot", player, SCHOOL_PHYSICAL, TREE_SURVIVAL )
  {
    hunter_t* p = player -> cast_hunter();
    assert( p -> talents.scatter_shot );

    base_dd_min = 1;
    base_dd_max = 1;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.08;

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    normalize_weapon_speed = true;
    cooldown               = 30;

    weapon_multiplier *= 0.5;

    base_multiplier *= 1.0 + p -> talents.ranged_weapon_specialization * 0.01;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.mortal_shots * 0.06;

    add_ammunition();
    add_scope();
  }
};


// Serpent Sting Attack =========================================================

struct serpent_sting_t : public hunter_attack_t
{
  int force;

  serpent_sting_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "serpent_sting", player, SCHOOL_NATURE, TREE_MARKSMANSHIP ),
    force(0)
  {
    hunter_t* p = player -> cast_hunter();

    option_t options[] =
    {
      { "force", OPT_INT, &force },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79, 12, 0, 0, 1210/5.0, 0.09 },
      { 73, 11, 0, 0,  990/5.0, 0.09 },
      { 67, 10, 0, 0,  660/5.0, 0.09 },
      { 60, 9,  0, 0,  555/5.0, 0.13 },
      { 0, 0 }
    };
    init_rank( ranks );

    base_tick_time   = 3.0;
    num_ticks        = p -> glyphs.serpent_sting ? 7 : 5;
    tick_power_mod   = 0.2 / 5.0;
    base_multiplier *= 1.0 + p -> talents.improved_stings * 0.1;

    observer = &( p -> active_serpent_sting );
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    p -> cancel_sting();
    hunter_attack_t::execute();
    if( result_is_hit() ) sim -> target -> debuffs.poisoned++;
  }

  virtual void tick()
  {
    hunter_attack_t::tick();
    trigger_lock_and_load( this );
  }

  virtual void last_tick()
  {
    hunter_attack_t::last_tick();
    sim -> target -> debuffs.poisoned--;
  }

  virtual void cancel()
  {
    if( tick_event ) sim -> target -> debuffs.poisoned--;
    hunter_attack_t::cancel();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if( ! force )
      if( p -> active_sting() )
        return false;

    return hunter_attack_t::ready();
  }
};

// Silencing Shot Attack =========================================================

struct silencing_shot_t : public hunter_attack_t
{
  silencing_shot_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "silencing_shot", player, SCHOOL_PHYSICAL, TREE_MARKSMANSHIP )
  {
    hunter_t* p = player -> cast_hunter();
    assert( p -> talents.silencing_shot );

    base_dd_min = 1;
    base_dd_max = 1;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.06;

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    normalize_weapon_speed = true;
    cooldown               = 20;

    weapon_multiplier *= 0.5;

    base_multiplier *= 1.0 + p -> talents.ranged_weapon_specialization * 0.01;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.mortal_shots * 0.06;

    add_ammunition();
    add_scope();
  }
};

// Steady Shot Attack =========================================================

struct steady_shot_t : public hunter_attack_t
{
  steady_shot_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "steady_shot", player, SCHOOL_PHYSICAL, TREE_MARKSMANSHIP )
  {
    hunter_t* p = player -> cast_hunter();

    static rank_t ranks[] =
    {
      { 77, 4, 252, 252, 0, 0.05 },
      { 71, 3, 198, 198, 0, 0.05 },
      { 62, 2, 108, 108, 0, 0.05 },
      { 50, 1,  45,  45, 0, 0.05 },
      { 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    normalize_weapon_damage = true;
    normalize_weapon_speed  = true;
    direct_power_mod        = 0.5/14.0;
    base_execute_time       = 2.0;

    base_cost *= 1.0 - p -> talents.master_marksman * 0.05;

    base_multiplier *= 1.0 + p -> talents.sniper_training              * 0.02;
    base_multiplier *= 1.0 + p -> talents.ranged_weapon_specialization * 0.01;

    base_crit += p -> talents.survival_instincts * 0.02;

    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.mortal_shots     * 0.06 +
                                          p -> talents.marked_for_death * 0.02 );

    base_penetration += p -> talents.piercing_shots * 0.02;

    add_ammunition();
  }

  void execute()
  {
    hunter_attack_t::execute();
    if( result_is_hit() )
    {
      trigger_improved_steady_shot( this );

      if( result == RESULT_CRIT )
      {
        trigger_cobra_strikes( this );
        trigger_hunting_party( this );
      }
    }
  }

  void player_buff()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::player_buff();
    if( p -> glyphs.steady_shot && p -> active_sting() )
    {
      player_multiplier *= 1.10;
    }
  }
};

// =========================================================================
// Hunter Spells
// =========================================================================

// hunter_spell_t::cost ====================================================

double hunter_spell_t::cost()
{
  hunter_t* p = player -> cast_hunter();
  double c = spell_t::cost();
  if ( p -> buffs_rapid_fire ) c *= 1.0 - p -> talents.rapid_recuperation * 0.3;
  return c;
}

// hunter_spell_t::gcd()

double hunter_spell_t::gcd()
{
  // Hunter gcd unaffected by haste
  return trigger_gcd;
}

// Aspect ==================================================================

struct aspect_t : public hunter_spell_t
{
  int    beast_during_bw;
  int    hawk_always;
  int    viper_start;
  int    viper_stop;
  double hawk_bonus;
  double viper_multiplier;

  aspect_t( player_t* player, const std::string& options_str ) :
    hunter_spell_t( "aspect", player, SCHOOL_NATURE, TREE_BEAST_MASTERY ),
    beast_during_bw(0), hawk_always(0), viper_start(5), viper_stop(25),
    hawk_bonus(0), viper_multiplier(0)
  {
    hunter_t* p = player -> cast_hunter();

    option_t options[] =
    {
      { "beast_during_bw", OPT_INT, &beast_during_bw },
      { "hawk_always",     OPT_INT, &hawk_always },
      { "viper_start",     OPT_INT, &viper_start },
      { "viper_stop",      OPT_INT, &viper_stop  },
      { NULL }
    };
    parse_options( options, options_str );

    cooldown         = 1.0;
    trigger_gcd      = 0.0;
    harmful          = false;
    hawk_bonus       = util_t::ability_rank( p -> level, 300,80, 230,74, 155,68,  120,0 );
    viper_multiplier = 0.50;

    if( p -> talents.aspect_mastery )
    {
      hawk_bonus       *= 1.30;
      viper_multiplier += 0.10;
    }
  }

  int choose_aspect()
  {
    hunter_t* p = player -> cast_hunter();

    if( hawk_always ) return ASPECT_HAWK;

    double mana_pct = p -> resource_current[ RESOURCE_MANA ] * 100.0 / p -> resource_max[ RESOURCE_MANA ];

    if( p -> active_aspect != ASPECT_VIPER && mana_pct < viper_start )
    {
      return ASPECT_VIPER;
    }
    if( p -> active_aspect == ASPECT_VIPER && mana_pct <= viper_stop )
    {
      return ASPECT_VIPER;
    }
    if( beast_during_bw && p -> active_pet )
    {
      hunter_pet_t* pet = (hunter_pet_t*) p -> active_pet -> cast_pet();
      if( pet -> buffs_bestial_wrath ) return ASPECT_BEAST;
    }
    return ASPECT_HAWK;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();

    int aspect = choose_aspect();

    if( aspect != p -> active_aspect )
    {
      if( aspect == ASPECT_HAWK )
      {
        if( sim -> log ) report_t::log( sim, "%s performs Aspect of the Hawk", p -> name() );
        p -> active_aspect = ASPECT_HAWK;
        p -> buffs_aspect_of_the_hawk = hawk_bonus;
        p -> buffs_aspect_of_the_viper = 0;
      }
      else if( aspect == ASPECT_VIPER )
      {
        if( sim -> log ) report_t::log( sim, "%s performs Aspect of the Viper", p -> name() );
        p -> active_aspect = ASPECT_VIPER;
        p -> buffs_aspect_of_the_hawk = 0;
        p -> buffs_aspect_of_the_viper = viper_multiplier;
      }
      else if( aspect == ASPECT_BEAST )
      {
        if( sim -> log ) report_t::log( sim, "%s performs Aspect of the Beast", p -> name() );
        p -> active_aspect = ASPECT_BEAST;
        p -> buffs_aspect_of_the_hawk = 0;
        p -> buffs_aspect_of_the_viper = 0;
      }
      else
      {
        p -> active_aspect = ASPECT_NONE;
        p -> buffs_aspect_of_the_hawk = 0;
        p -> buffs_aspect_of_the_viper = 0;
      }
    }
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    return choose_aspect() != p -> active_aspect;
  }
};

// Bestial Wrath =========================================================

struct bestial_wrath_t : public hunter_spell_t
{
  bestial_wrath_t( player_t* player, const std::string& options_str ) :
    hunter_spell_t( "bestial_wrath", player, SCHOOL_PHYSICAL, TREE_BEAST_MASTERY )
  {
    hunter_t* p = player -> cast_hunter();
    assert( p -> talents.bestial_wrath );

    base_cost = 0.10 * p -> resource_base[ RESOURCE_MANA ];
    cooldown = (120 - p -> glyphs.bestial_wrath * 20) * (1 - p -> talents.longevity * 0.1);
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    assert( p -> active_pet );

    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
      {
        hunter_t* p = player -> cast_hunter();
        hunter_pet_t* pet = p -> active_pet;
        name = "Bestial Wrath Expiration";
        if( p -> talents.beast_within )
          p -> buffs_beast_within = 1;
        pet -> buffs_bestial_wrath = 1;
        sim -> add_event( this, 18.0 );
      }
      virtual void execute()
      {
        hunter_t* p = player -> cast_hunter();
        hunter_pet_t* pet = p -> active_pet;
        p -> buffs_beast_within = 0;
        pet -> buffs_bestial_wrath = 0;
      }
    };

    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    player -> action_finish( this );
    new ( sim ) expiration_t( sim, player );
  }
};

// Hunter's Mark =========================================================

struct hunters_mark_t : public hunter_spell_t
{
  double ap_bonus;

  hunters_mark_t( player_t* player, const std::string& options_str ) :
    hunter_spell_t( "hunters_mark", player, SCHOOL_ARCANE, TREE_MARKSMANSHIP ), ap_bonus(0)
  {
    hunter_t* p = player -> cast_hunter();

    base_cost = 0.02 * p -> resource_base[ RESOURCE_MANA ];
    base_cost *= 1.0 - p -> talents.improved_hunters_mark / 3.0;

    ap_bonus = util_t::ability_rank( p -> level,  300,76,  110,0 );

    ap_bonus *= 1.0 + p -> talents.improved_hunters_mark * 0.10
                  + ( p -> glyphs.hunters_mark ? 0.20 : 0 );
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim ) :
        event_t( sim, 0 )
      {
        name = "Hunter's Mark Expiration";
        sim -> add_event( this, 120.0 );
      }
      virtual void execute()
      {
        target_t* t = sim -> target;
        t -> debuffs.hunters_mark = 0;
        t -> expirations.hunters_mark = 0;
      }
    };

    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );

    consume_resource();

    target_t* t = sim -> target;
    event_t*& e = t -> expirations.hunters_mark;

    t -> debuffs.hunters_mark = ap_bonus;

    if( e )
    {
      e -> reschedule( 120.0 );
    }
    else
    {
      e = new ( sim ) expiration_t( sim );
    }

    player -> action_finish( this );
  }

  virtual bool ready()
  {
    if( ! hunter_spell_t::ready() )
      return false;

    return ap_bonus > sim -> target -> debuffs.hunters_mark;
  }
};

// Kill Command =======================================================

struct kill_command_t : public hunter_spell_t
{
  kill_command_t( player_t* player, const std::string& options_str ) :
    hunter_spell_t( "kill_command", player, SCHOOL_PHYSICAL, TREE_BEAST_MASTERY )
  {
    hunter_t* p = player -> cast_hunter();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );

    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.03;
    cooldown  = 60;
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    assert( p -> active_pet -> buffs_kill_command == 0 );
    p -> active_pet -> aura_gain( "Kill Command" );
    p -> active_pet -> buffs_kill_command = 3;
    consume_resource();
    update_ready();
    player -> action_finish( this );
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if( ! p -> active_pet )
      return false;

    return hunter_spell_t::ready();
  }
};

// Rapid Fire =========================================================

struct rapid_fire_t : public hunter_spell_t
{
  int viper;

  rapid_fire_t( player_t* player, const std::string& options_str ) :
    hunter_spell_t( "rapid_fire", player, SCHOOL_PHYSICAL, TREE_MARKSMANSHIP ), viper(0)
  {
    hunter_t* p = player -> cast_hunter();

    option_t options[] =
    {
      { "viper", OPT_INT, &viper },
      { NULL }
    };
    parse_options( options, options_str );

    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.03;
    cooldown  = 300;
    cooldown -= p -> talents.rapid_killing * 60;
    trigger_gcd = 0.0;
    harmful = false;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();

    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
      {
        name = "Rapid Fire Expiration";
        p -> aura_gain( "Rapid Fire" );
        sim -> add_event( this, 15.0 );
      }
      virtual void execute()
      {
        player -> aura_loss( "Rapid Fire" );
        player -> cast_hunter() -> buffs_rapid_fire = 0;
      }
    };

    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_rapid_fire = p -> glyphs.rapid_fire ? 0.48 : 0.40;
    consume_resource();
    update_ready();
    player -> action_finish( this );
    new ( sim ) expiration_t( sim, p );
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if( p -> buffs_rapid_fire )
      return false;

    if( viper )
      if( p -> active_aspect != ASPECT_VIPER )
        return false;

    return hunter_spell_t::ready();
  }
};

// Readiness ================================================================

struct readiness_t : public hunter_spell_t
{
  readiness_t( player_t* player, const std::string& options_str ) :
    hunter_spell_t( "readiness", player, SCHOOL_PHYSICAL, TREE_MARKSMANSHIP )
  {
    hunter_t* p = player -> cast_hunter();
    assert( p -> talents.readiness );
    cooldown = 180;
    trigger_gcd = 1.0;
    harmful = false;
  }

  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );

    for( action_t* a = player -> action_list; a; a = a -> next )
    {
      a -> cooldown_ready = 0;
    }

    update_ready();
  }
};

// Summon Pet ===============================================================

struct summon_pet_t : public hunter_spell_t
{
  std::string pet_name;

  summon_pet_t( player_t* player, const std::string& options_str ) :
    hunter_spell_t( "summon_pet", player, SCHOOL_PHYSICAL, TREE_BEAST_MASTERY )
  {
    pet_name = options_str;
    harmful = false;
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    player -> summon_pet( pet_name.c_str() );
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();
    if( p -> active_pet ) return false;
    return true;
  }
};

// Trueshot Aura ===========================================================

struct trueshot_aura_t : public hunter_spell_t
{
  trueshot_aura_t( player_t* player, const std::string& options_str ) :
    hunter_spell_t( "trueshot_aura", player, SCHOOL_ARCANE, TREE_MARKSMANSHIP )
  {
    hunter_t* p = player -> cast_hunter();
    assert( p -> talents.trueshot_aura );
    trigger_gcd = 0;
    harmful = false;
  }

  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );

    player -> cast_hunter() -> buffs_trueshot_aura = 1;

    for( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if( p -> buffs.trueshot_aura == 0 )
      {
        p -> aura_gain( "Trueshot Aura" );
      }
      p -> buffs.trueshot_aura++;
    }
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();
    return( ! p -> buffs_trueshot_aura );
  }
};

// hunter_t::create_action ====================================================

action_t* hunter_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if( name == "auto_shot"             ) return new              auto_shot_t( this, options_str );
  if( name == "aimed_shot"            ) return new             aimed_shot_t( this, options_str );
  if( name == "arcane_shot"           ) return new            arcane_shot_t( this, options_str );
  if( name == "aspect"                ) return new                 aspect_t( this, options_str );
  if( name == "bestial_wrath"         ) return new          bestial_wrath_t( this, options_str );
  if( name == "chimera_shot"          ) return new           chimera_shot_t( this, options_str );
  if( name == "explosive_shot"        ) return new         explosive_shot_t( this, options_str );
  if( name == "hunters_mark"          ) return new           hunters_mark_t( this, options_str );
  if( name == "kill_command"          ) return new           kill_command_t( this, options_str );
  if( name == "kill_shot"             ) return new              kill_shot_t( this, options_str );
//if( name == "mongoose_bite"         ) return new          mongoose_bite_t( this, options_str );
  if( name == "multi_shot"            ) return new             multi_shot_t( this, options_str );
  if( name == "rapid_fire"            ) return new             rapid_fire_t( this, options_str );
//if( name == "raptor_strike"         ) return new          raptor_strike_t( this, options_str );
  if( name == "readiness"             ) return new              readiness_t( this, options_str );
  if( name == "scatter_shot"          ) return new           scatter_shot_t( this, options_str );
//if( name == "scorpid_sting"         ) return new          scorpid_sting_t( this, options_str );
  if( name == "serpent_sting"         ) return new          serpent_sting_t( this, options_str );
  if( name == "silencing_shot"        ) return new         silencing_shot_t( this, options_str );
  if( name == "steady_shot"           ) return new            steady_shot_t( this, options_str );
  if( name == "summon_pet"            ) return new             summon_pet_t( this, options_str );
  if( name == "trueshot_aura"         ) return new          trueshot_aura_t( this, options_str );
//if( name == "viper_sting"           ) return new            viper_sting_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// hunter_t::create_pet =======================================================

pet_t* hunter_t::create_pet( const std::string& pet_name )
{
  // Ferocity
  if( pet_name == "carrion_bird" ) return new hunter_pet_t( sim, this, pet_name, PET_CARRION_BIRD );
  if( pet_name == "cat"          ) return new hunter_pet_t( sim, this, pet_name, PET_CAT          );
  if( pet_name == "core_hound"   ) return new hunter_pet_t( sim, this, pet_name, PET_CORE_HOUND   );
  if( pet_name == "devilsaur"    ) return new hunter_pet_t( sim, this, pet_name, PET_DEVILSAUR    );
  if( pet_name == "hyena"        ) return new hunter_pet_t( sim, this, pet_name, PET_HYENA        );
  if( pet_name == "moth"         ) return new hunter_pet_t( sim, this, pet_name, PET_MOTH         );
  if( pet_name == "raptor"       ) return new hunter_pet_t( sim, this, pet_name, PET_RAPTOR       );
  if( pet_name == "spirit_beast" ) return new hunter_pet_t( sim, this, pet_name, PET_SPIRIT_BEAST );
  if( pet_name == "tallstrider"  ) return new hunter_pet_t( sim, this, pet_name, PET_TALLSTRIDER  );
  if( pet_name == "wasp"         ) return new hunter_pet_t( sim, this, pet_name, PET_WASP         );
  if( pet_name == "wolf"         ) return new hunter_pet_t( sim, this, pet_name, PET_WOLF         );

  // Tenacity
  if( pet_name == "bear"         ) return new hunter_pet_t( sim, this, pet_name, PET_BEAR         );
  if( pet_name == "boar"         ) return new hunter_pet_t( sim, this, pet_name, PET_BOAR         );
  if( pet_name == "crab"         ) return new hunter_pet_t( sim, this, pet_name, PET_CRAB         );
  if( pet_name == "crocolisk"    ) return new hunter_pet_t( sim, this, pet_name, PET_CROCOLISK    );
  if( pet_name == "gorilla"      ) return new hunter_pet_t( sim, this, pet_name, PET_GORILLA      );
  if( pet_name == "rhino"        ) return new hunter_pet_t( sim, this, pet_name, PET_RHINO        );
  if( pet_name == "scorpid"      ) return new hunter_pet_t( sim, this, pet_name, PET_SCORPID      );
  if( pet_name == "turtle"       ) return new hunter_pet_t( sim, this, pet_name, PET_TURTLE       );
  if( pet_name == "warp_stalker" ) return new hunter_pet_t( sim, this, pet_name, PET_WARP_STALKER );
  if( pet_name == "worm"         ) return new hunter_pet_t( sim, this, pet_name, PET_WORM         );

  // Cunning
  if( pet_name == "bat"          ) return new hunter_pet_t( sim, this, pet_name, PET_BAT          );
  if( pet_name == "bird_of_prey" ) return new hunter_pet_t( sim, this, pet_name, PET_BIRD_OF_PREY );
  if( pet_name == "chimera"      ) return new hunter_pet_t( sim, this, pet_name, PET_CHIMERA      );
  if( pet_name == "dragonhawk"   ) return new hunter_pet_t( sim, this, pet_name, PET_DRAGONHAWK   );
  if( pet_name == "nether_ray"   ) return new hunter_pet_t( sim, this, pet_name, PET_NETHER_RAY   );
  if( pet_name == "ravager"      ) return new hunter_pet_t( sim, this, pet_name, PET_RAVAGER      );
  if( pet_name == "serpent"      ) return new hunter_pet_t( sim, this, pet_name, PET_SERPENT      );
  if( pet_name == "silithid"     ) return new hunter_pet_t( sim, this, pet_name, PET_SILITHID     );
  if( pet_name == "spider"       ) return new hunter_pet_t( sim, this, pet_name, PET_SPIDER       );
  if( pet_name == "sporebat"     ) return new hunter_pet_t( sim, this, pet_name, PET_SPOREBAT     );
  if( pet_name == "wind_serpent" ) return new hunter_pet_t( sim, this, pet_name, PET_WIND_SERPENT );

  return 0;
}

// hunter_t::init_base ========================================================

void hunter_t::init_base()
{
  // FIXME! Make this level-specific.
  attribute_base[ ATTR_STRENGTH  ] =  76;
  attribute_base[ ATTR_AGILITY   ] = 177;
  attribute_base[ ATTR_STAMINA   ] = 131;
  attribute_base[ ATTR_INTELLECT ] =  87;
  attribute_base[ ATTR_SPIRIT    ] =  94;

  attribute_multiplier_initial[ ATTR_INTELLECT ] *= 1.0 + talents.combat_experience * 0.02;
  attribute_multiplier_initial[ ATTR_AGILITY ]   *= 1.0 + talents.combat_experience * 0.02;
  attribute_multiplier_initial[ ATTR_AGILITY ]   *= 1.0 + talents.lightning_reflexes * 0.03;
  attribute_multiplier_initial[ ATTR_STAMINA ]   *= 1.0 + talents.survivalist * 0.02;

  base_attack_power = level * 2 - 10;
  base_attack_crit  = -0.0153;

  initial_attack_power_per_strength = 0.0;
  initial_attack_power_per_agility  = 1.0;
  initial_attack_crit_per_agility   = rating_t::interpolate( level, 0.01/33.0, 0.01/40.0, 0.01/83.33 );

  resource_base[ RESOURCE_HEALTH ] = 4579;
  resource_base[ RESOURCE_MANA   ] = rating_t::interpolate( level, 1500, 3383, 5046 );

  health_per_stamina = 10;
  mana_per_intellect = 15;

  position = POSITION_RANGED;
}

// hunter_t::reset ===========================================================

void hunter_t::reset()
{
  player_t::reset();

  // Active
  active_pet           = 0;
  active_aspect        = ASPECT_NONE;
  active_scorpid_sting = 0;
  active_serpent_sting = 0;
  active_viper_sting   = 0;

  // Buffs
  buffs_aspect_of_the_hawk          = 0;
  buffs_aspect_of_the_viper         = 0;
  buffs_beast_within                = 0;
  buffs_call_of_the_wild            = 0;
  buffs_cobra_strikes               = 0;
  buffs_expose_weakness             = 0;
  buffs_improved_aspect_of_the_hawk = 0;
  buffs_improved_steady_shot        = 0;
  buffs_lock_and_load               = 0;
  buffs_master_tactician            = 0;
  buffs_rapid_fire                  = 0;
  buffs_trueshot_aura               = 0;

  // Expirations
  expirations_cobra_strikes               = 0;
  expirations_expose_weakness             = 0;
  expirations_hunting_party               = 0;
  expirations_improved_aspect_of_the_hawk = 0;
  expirations_improved_steady_shot        = 0;
  expirations_lock_and_load               = 0;
  expirations_master_tactician            = 0;
  expirations_rapid_fire                  = 0;

  // Cooldowns
  cooldowns_lock_and_load = 0;
}

// hunter_t::composite_attack_power ==========================================

double hunter_t::composite_attack_power()
{
  double ap = player_t::composite_attack_power();

  ap += buffs_aspect_of_the_hawk;
  ap += intellect() * talents.careful_aim / 3.0;
  ap += stamina() * talents.hunter_vs_wild * 0.1;

  if( buffs_expose_weakness ) ap += agility() * 0.25;
  uptimes_expose_weakness -> update( buffs_expose_weakness != 0 );

  if( buffs_call_of_the_wild )
    ap *= 1.1;

  return ap;
}

// hunter_t::regen  =======================================================

void hunter_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  if( buffs_aspect_of_the_viper )
  {
    double aspect_of_the_viper_regen = periodicity * 0.04 * resource_max[ RESOURCE_MANA ] / 3.0;

    resource_gain( RESOURCE_MANA, aspect_of_the_viper_regen, gains_viper_aspect_passive );
  }
}

// hunter_t::get_talent_trees ==============================================

bool hunter_t::get_talent_trees( std::vector<int*>& beastmastery,
                                 std::vector<int*>& marksmanship,
                                 std::vector<int*>& survival )
{
  talent_translation_t translation[][3] =
  {
    { {  1, &( talents.improved_aspect_of_the_hawk ) }, {  1, NULL                                      }, {  1, &( talents.improved_tracking        ) } },
    { {  2, NULL                                     }, {  2, &( talents.focused_aim                  ) }, {  2, NULL                                  } },
    { {  3, &( talents.focused_fire                ) }, {  3, &( talents.lethal_shots                 ) }, {  3, &( talents.savage_strikes           ) } },
    { {  4, NULL                                     }, {  4, &( talents.careful_aim                  ) }, {  4, NULL                                  } },
    { {  5, NULL                                     }, {  5, &( talents.improved_hunters_mark        ) }, {  5, NULL                                  } },
    { {  6, NULL                                     }, {  6, &( talents.mortal_shots                 ) }, {  6, NULL                                  } },
    { {  7, NULL                                     }, {  7, &( talents.go_for_the_throat            ) }, {  7, &( talents.survival_instincts       ) } },
    { {  8, &( talents.aspect_mastery              ) }, {  8, &( talents.improved_arcane_shot         ) }, {  8, &( talents.survivalist              ) } },
    { {  9, &( talents.unleashed_fury              ) }, {  9, &( talents.aimed_shot                   ) }, {  9, &( talents.scatter_shot             ) } },
    { { 10, NULL                                     }, { 10, &( talents.rapid_killing                ) }, { 10, NULL                                  } },
    { { 11, &( talents.ferocity                    ) }, { 11, &( talents.improved_stings              ) }, { 11, &( talents.survival_tactics         ) } },
    { { 12, NULL                                     }, { 12, &( talents.efficiency                   ) }, { 12, &( talents.tnt                      ) } },
    { { 13, NULL                                     }, { 13, NULL                                      }, { 13, &( talents.lock_and_load            ) } },
    { { 14, &( talents.bestial_discipline          ) }, { 14, &( talents.readiness                    ) }, { 14, &( talents.hunter_vs_wild           ) } },
    { { 15, &( talents.animal_handler              ) }, { 15, &( talents.barrage                      ) }, { 15, &( talents.killer_instinct          ) } },
    { { 16, &( talents.frenzy                      ) }, { 16, &( talents.combat_experience            ) }, { 16, NULL                                  } },
    { { 17, &( talents.ferocious_inspiration       ) }, { 17, &( talents.ranged_weapon_specialization ) }, { 17, &( talents.lightning_reflexes       ) } },
    { { 18, &( talents.bestial_wrath               ) }, { 18, &( talents.piercing_shots               ) }, { 18, &( talents.resourcefulness          ) } },
    { { 19, NULL                                     }, { 19, &( talents.trueshot_aura                ) }, { 19, &( talents.expose_weakness          ) } },
    { { 20, &( talents.invigoration                ) }, { 20, &( talents.improved_barrage             ) }, { 20, NULL                                  } },
    { { 21, &( talents.serpents_swiftness          ) }, { 21, &( talents.master_marksman              ) }, { 21, &( talents.thrill_of_the_hunt       ) } },
    { { 22, &( talents.longevity                   ) }, { 22, &( talents.rapid_recuperation           ) }, { 22, &( talents.master_tactician         ) } },
    { { 23, &( talents.beast_within                ) }, { 23, &( talents.wild_quiver                  ) }, { 23, &( talents.noxious_stings           ) } },
    { { 24, &( talents.cobra_strikes               ) }, { 24, &( talents.silencing_shot               ) }, { 24, NULL                                  } },
    { { 25, &( talents.kindred_spirits             ) }, { 25, &( talents.improved_steady_shot         ) }, { 25, NULL                                  } },
    { { 26, NULL                                     }, { 26, &( talents.marked_for_death             ) }, { 26, &( talents.sniper_training          ) } },
    { {  0, NULL                                     }, { 27, &( talents.chimera_shot                 ) }, { 27, &( talents.hunting_party            ) } },
    { {  0, NULL                                     }, {  0, NULL                                      }, { 28, &( talents.explosive_shot           ) } },
    { {  0, NULL                                     }, {  0, NULL                                      }, {  0, NULL                                  } }
  };

  return player_t::get_talent_trees( beastmastery, marksmanship, survival, translation );
}


// hunter_t::parse_talents ===============================================

bool hunter_t::parse_talents_mmo( const std::string& talent_string )
{

  // hunter mmo encoding: Beastmastery-Survival-Marksmanship

  int size1 = 26;
  int size2 = 28;

  std::string beastmastery_string( talent_string,     0,  size1 );
  std::string     survival_string( talent_string, size1,  size2 );
  std::string marksmanship_string( talent_string, size1 + size2 );

  return parse_talents( beastmastery_string + marksmanship_string + survival_string );
}

// hunter_t::parse_option  ==================================================

bool hunter_t::parse_option( const std::string& name,
                             const std::string& value )
{
  option_t options[] =
  {
    // Talents
    { "aimed_shot",                        OPT_INT, &( talents.aimed_shot                   ) },
    { "animal_handler",                    OPT_INT, &( talents.animal_handler               ) },
    { "aspect_mastery",                    OPT_INT, &( talents.aspect_mastery               ) },
    { "barrage",                           OPT_INT, &( talents.barrage                      ) },
    { "beast_within",                      OPT_INT, &( talents.beast_within                 ) },
    { "bestial_discipline",                OPT_INT, &( talents.bestial_discipline           ) },
    { "bestial_wrath",                     OPT_INT, &( talents.bestial_wrath                ) },
    { "careful_aim",                       OPT_INT, &( talents.careful_aim                  ) },
    { "chimera_shot",                      OPT_INT, &( talents.chimera_shot                 ) },
    { "cobra_strikes",                     OPT_INT, &( talents.cobra_strikes                ) },
    { "combat_experience",                 OPT_INT, &( talents.combat_experience            ) },
    { "efficiency",                        OPT_INT, &( talents.efficiency                   ) },
    { "explosive_shot",                    OPT_INT, &( talents.explosive_shot               ) },
    { "expose_weakness",                   OPT_INT, &( talents.expose_weakness              ) },
    { "ferocious_inspiration",             OPT_INT, &( talents.ferocious_inspiration        ) },
    { "ferocity",                          OPT_INT, &( talents.ferocity                     ) },
    { "focused_aim",                       OPT_INT, &( talents.focused_aim                  ) },
    { "focused_fire",                      OPT_INT, &( talents.focused_fire                 ) },
    { "frenzy",                            OPT_INT, &( talents.frenzy                       ) },
    { "go_for_the_throat",                 OPT_INT, &( talents.go_for_the_throat            ) },
    { "hunter_vs_wild",                    OPT_INT, &( talents.hunter_vs_wild               ) },
    { "hunting_party",                     OPT_INT, &( talents.hunting_party                ) },
    { "improved_arcane_shot",              OPT_INT, &( talents.improved_arcane_shot         ) },
    { "improved_aspect_of_the_hawk",       OPT_INT, &( talents.improved_aspect_of_the_hawk  ) },
    { "improved_barrage",                  OPT_INT, &( talents.improved_barrage             ) },
    { "improved_hunters_mark",             OPT_INT, &( talents.improved_hunters_mark        ) },
    { "improved_steady_shot",              OPT_INT, &( talents.improved_steady_shot         ) },
    { "improved_stings",                   OPT_INT, &( talents.improved_stings              ) },
    { "improved_tracking",                 OPT_INT, &( talents.improved_tracking            ) },
    { "invigoration",                      OPT_INT, &( talents.invigoration                 ) },
    { "killer_instinct",                   OPT_INT, &( talents.killer_instinct              ) },
    { "kindred_spirits",                   OPT_INT, &( talents.kindred_spirits              ) },
    { "lethal_shots",                      OPT_INT, &( talents.lethal_shots                 ) },
    { "lightning_reflexes",                OPT_INT, &( talents.lightning_reflexes           ) },
    { "lock_and_load",                     OPT_INT, &( talents.lock_and_load                ) },
    { "longevity",                         OPT_INT, &( talents.longevity                    ) },
    { "marked_for_death",                  OPT_INT, &( talents.marked_for_death             ) },
    { "master_marksman",                   OPT_INT, &( talents.master_marksman              ) },
    { "master_tactician",                  OPT_INT, &( talents.master_tactician             ) },
    { "mortal_shots",                      OPT_INT, &( talents.mortal_shots                 ) },
    { "noxious_stings",                    OPT_INT, &( talents.noxious_stings               ) },
    { "piercing_shots",                    OPT_INT, &( talents.piercing_shots               ) },
    { "ranged_weapon_specialization",      OPT_INT, &( talents.ranged_weapon_specialization ) },
    { "rapid_killing",                     OPT_INT, &( talents.rapid_killing                ) },
    { "rapid_recuperation",                OPT_INT, &( talents.rapid_recuperation           ) },
    { "readiness",                         OPT_INT, &( talents.readiness                    ) },
    { "resourcefulness",                   OPT_INT, &( talents.resourcefulness              ) },
    { "savage_strikes",                    OPT_INT, &( talents.savage_strikes               ) },
    { "scatter_shot",                      OPT_INT, &( talents.scatter_shot                 ) },
    { "serpents_swiftness",                OPT_INT, &( talents.serpents_swiftness           ) },
    { "silencing_shot",                    OPT_INT, &( talents.silencing_shot               ) },
    { "sniper_training",                   OPT_INT, &( talents.sniper_training              ) },
    { "survival_instincts",                OPT_INT, &( talents.survival_instincts           ) },
    { "survival_tactics",                  OPT_INT, &( talents.survival_tactics             ) },
    { "survivalist",                       OPT_INT, &( talents.survivalist                  ) },
    { "thrill_of_the_hunt",                OPT_INT, &( talents.thrill_of_the_hunt           ) },
    { "tnt",                               OPT_INT, &( talents.tnt                          ) },
    { "trueshot_aura",                     OPT_INT, &( talents.trueshot_aura                ) },
    { "unleashed_fury",                    OPT_INT, &( talents.unleashed_fury               ) },
    { "wild_quiver",                       OPT_INT, &( talents.wild_quiver                  ) },
    // Glyphs
    { "glyph_aspect_of_the_viper",         OPT_INT, &( glyphs.aspect_of_the_viper           ) },
    { "glyph_bestial_wrath",               OPT_INT, &( glyphs.bestial_wrath                 ) },
    { "glyph_hunters_mark",                OPT_INT, &( glyphs.hunters_mark                  ) },
    { "glyph_improved_aspect_of_the_hawk", OPT_INT, &( glyphs.improved_aspect_of_the_hawk   ) },
    { "glyph_rapid_fire",                  OPT_INT, &( glyphs.rapid_fire                    ) },
    { "glyph_serpent_sting",               OPT_INT, &( glyphs.serpent_sting                 ) },
    { "glyph_steady_shot",                 OPT_INT, &( glyphs.steady_shot                   ) },
    { "glyph_trueshot_aura",               OPT_INT, &( glyphs.trueshot_aura                 ) },
    // Custom
    { "ammo_dps",                          OPT_FLT,  &( ammo_dps                             ) },
    { "quiver_haste",                      OPT_FLT,  &( quiver_haste                         ) },

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

// player_t::create_hunter  =================================================

player_t* player_t::create_hunter( sim_t*       sim,
                                   std::string& name )
{
  return new hunter_t( sim, name );
}

