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
  int           active_black_arrow;
  action_t*     active_chimera_serpent;
  action_t*     active_wild_quiver;
  action_t*     active_piercing_shots;
  action_t*     active_scorpid_sting;
  action_t*     active_serpent_sting;
  action_t*     active_viper_sting;

  // Buffs
  struct _buffs_t
  {
    double aspect_of_the_hawk;
    double aspect_of_the_viper;
    int    beast_within;
    int    call_of_the_wild;
    int    cobra_strikes;
    int    expose_weakness;
    int    furious_howl;
    double improved_aspect_of_the_hawk;
    int    improved_steady_shot;
    int    lock_and_load;
    double master_tactician;
    double rapid_fire;
    int    trueshot_aura;

    void reset() { memset( ( void* ) this, 0x00, sizeof( _buffs_t ) ); }
    _buffs_t() { reset(); }
  };
  _buffs_t _buffs;

  // Cooldowns
  struct _cooldowns_t
  {
    double lock_and_load;

    void reset() { memset( ( void* ) this, 0x00, sizeof( _cooldowns_t ) ); }
    _cooldowns_t() { reset(); }
  };
  _cooldowns_t _cooldowns;

  // Expirations
  struct _expirations_t
  {
    event_t* cobra_strikes;
    event_t* expose_weakness;
    event_t* improved_aspect_of_the_hawk;
    event_t* improved_steady_shot;
    event_t* lock_and_load;
    event_t* master_tactician;
    event_t* rapid_fire;

    void reset() { memset( ( void* ) this, 0x00, sizeof( _expirations_t ) ); }
    _expirations_t() { reset(); }
  };
  _expirations_t _expirations;

  // Gains
  gain_t* gains_chimera_viper;
  gain_t* gains_invigoration;
  gain_t* gains_rapid_recuperation;
  gain_t* gains_roar_of_recovery;
  gain_t* gains_thrill_of_the_hunt;
  gain_t* gains_viper_aspect_passive;
  gain_t* gains_viper_aspect_shot;

  // Procs
  proc_t* procs_wild_quiver;
  proc_t* procs_lock_and_load;

  // Uptimes
  uptime_t* uptimes_aspect_of_the_viper;
  uptime_t* uptimes_expose_weakness;
  uptime_t* uptimes_furious_howl;
  uptime_t* uptimes_improved_aspect_of_the_hawk;
  uptime_t* uptimes_improved_steady_shot;
  uptime_t* uptimes_master_tactician;
  uptime_t* uptimes_rapid_fire;

  // Random Number Generation
  rng_t* rng_cobra_strikes;
  rng_t* rng_expose_weakness;
  rng_t* rng_frenzy;
  rng_t* rng_hunting_party;
  rng_t* rng_improved_aoth;
  rng_t* rng_improved_steady_shot;
  rng_t* rng_invigoration;
  rng_t* rng_lock_and_load;
  rng_t* rng_master_tactician;
  rng_t* rng_owls_focus;
  rng_t* rng_rabid_power;
  rng_t* rng_thrill_of_the_hunt;
  rng_t* rng_wild_quiver;

  // Auto-Attack
  attack_t* ranged_attack;

  // Custom Parameters
  double ammo_dps;
  double quiver_haste;
  std::string summon_pet_str;

  struct talents_t
  {
    int  animal_handler;
    int  aimed_shot;
    int  aspect_mastery;
    int  barrage;
    int  beast_mastery;
    int  beast_within;
    int  bestial_wrath;
    int  bestial_discipline;
    int  black_arrow;
    int  careful_aim;
    int  catlike_reflexes;
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
    int  trap_mastery;

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int  aimed_shot;
    int  aspect_of_the_viper;
    int  bestial_wrath;
    int  chimera_shot;
    int  explosive_shot;
    int  hunters_mark;
    int  improved_aspect_of_the_hawk;
    int  kill_shot;
    int  rapid_fire;
    int  serpent_sting;
    int  steady_shot;
    int  trueshot_aura;
    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  hunter_t( sim_t* sim, const std::string& name, int race_type = RACE_NONE ) : player_t( sim, HUNTER, name, race_type )
  {
    // Active
    active_pet             = 0;
    active_aspect          = ASPECT_NONE;
    active_black_arrow     = 0;
    active_chimera_serpent = 0;
    active_wild_quiver     = 0;
    active_piercing_shots  = 0;
    active_scorpid_sting   = 0;
    active_serpent_sting   = 0;
    active_viper_sting     = 0;

    ranged_attack = 0;
    ammo_dps = 67.5;
    quiver_haste = 1.15;
    summon_pet_str = "wolf";
  }

  // Character Definition
  virtual void      init();
  virtual void      init_glyphs();
  virtual void      init_race();
  virtual void      init_base();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_uptimes();
  virtual void      init_rng();
  virtual void      init_scaling();
  virtual void      init_actions();
  virtual void      reset();
  virtual void      interrupt();
  virtual double    composite_attack_power() SC_CONST;
  virtual bool      get_talent_trees( std::vector<int*>& beastmastery, std::vector<int*>& marksmanship, std::vector<int*>& survival );
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet( const std::string& name );
  virtual void      armory( xml_node_t* sheet_xml, xml_node_t* talents_xml );
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_MANA; }
  virtual int       primary_role() SC_CONST     { return ROLE_ATTACK; }
  virtual int       primary_tree() SC_CONST;
  virtual bool      save( FILE*, int save_type=SAVE_ALL );

  // Event Tracking
  virtual void regen( double periodicity );

  // Utility
  void cancel_sting()
  {
    if ( active_scorpid_sting ) active_scorpid_sting -> cancel();
    if ( active_serpent_sting ) active_serpent_sting -> cancel();
    if ( active_viper_sting   ) active_viper_sting   -> cancel();
  }
  action_t* active_sting()
  {
    if ( active_scorpid_sting ) return active_scorpid_sting;
    if ( active_serpent_sting ) return active_serpent_sting;
    if ( active_viper_sting   ) return active_viper_sting;
    return 0;
  }
  double ranged_weapon_specialization_multiplier()
  {
    return 1.0 + util_t::talent_rank( talents.ranged_weapon_specialization, 3, 0.01, 0.03, 0.05 );
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
  struct _buffs_t
  {
    int bestial_wrath;
    int call_of_the_wild;
    int frenzy;
    int furious_howl;
    int kill_command;
    int monstrous_bite;
    int owls_focus;
    int wolverine_bite;
    int rabid;
    int rabid_power_stack;
    int savage_rend;

    void reset() { memset( ( void* ) this, 0x00, sizeof( _buffs_t ) ); }
    _buffs_t() { reset(); }
  };
  _buffs_t _buffs;

  // Expirations
  struct _expirations_t
  {
    event_t* frenzy;
    event_t* monstrous_bite;
    event_t* owls_focus;
    event_t* savage_rend;

    void reset() { memset( ( void* ) this, 0x00, sizeof( _expirations_t ) ); }
    _expirations_t() { reset(); }
  };
  _expirations_t _expirations;

  // Gains
  gain_t* gains_go_for_the_throat;

  // Uptimes
  uptime_t* uptimes_frenzy;
  uptime_t* uptimes_monstrous_bite;
  uptime_t* uptimes_savage_rend;

  // Auto-Attack
  attack_t* main_hand_attack;

  struct talents_t
  {
    int call_of_the_wild;
    int cobra_reflexes;
    int feeding_frenzy;
    int rabid;
    int roar_of_recovery;
    int shark_attack;
    int spiked_collar;
    int spiders_bite;
    int owls_focus;
    int wild_hunt;
    int wolverine_bite;

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  hunter_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name, int pt ) :
      pet_t( sim, owner, pet_name ), pet_type( pt ), main_hand_attack( 0 )
  {
    if ( ! supported( pet_type ) )
    {
      util_t::fprintf( stdout, "simcraft: Pet %s is not yet supported.\n", pet_name.c_str() );
      exit( 0 );
    }

    hunter_t* o = owner -> cast_hunter();

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.damage     = ( 51.0+78.0 )/2; // FIXME only level 80 value
    main_hand_weapon.swing_time = 2.0;

    stamina_per_owner = 0.45;

    health_per_stamina *= 1.05; // 3.1.0 change # Cunning, Ferocity and Tenacity pets now all have +5% damage, +5% armor and +5% health bonuses

    if ( group() == PET_FEROCITY )
    {
      talents.call_of_the_wild = 1;
      talents.cobra_reflexes   = 2;
      talents.rabid            = 1;
      talents.spiders_bite     = 3;
      talents.spiked_collar    = 3;

      talents.shark_attack = ( o -> talents.beast_mastery ) ? 2 : 0;
      talents.wild_hunt    = ( o -> talents.beast_mastery ) ? 2 : 1;

      if ( pet_type == PET_CAT )
      {
        action_list_str = "auto_attack/call_of_the_wild/rabid/rake/claw";
      }
      else if ( pet_type == PET_DEVILSAUR )
      {
        action_list_str = "auto_attack/monstrous_bite/call_of_the_wild/rabid/bite";
      }
      else if ( pet_type == PET_RAPTOR )
      {
        action_list_str = "auto_attack/savage_rend/call_of_the_wild/rabid/claw";
      }
      else if ( pet_type == PET_WOLF )
      {
        action_list_str = "auto_attack/furious_howl/call_of_the_wild/rabid/bite";
      }
      else assert( 0 );
    }
    else if ( group() == PET_CUNNING )
    {
      talents.cobra_reflexes   = 2;
      talents.feeding_frenzy   = 2;
      talents.owls_focus       = 2;
      talents.roar_of_recovery = 1;
      talents.spiked_collar    = 3;
      talents.wolverine_bite   = 1;

      talents.wild_hunt = ( o -> talents.beast_mastery ) ? 2 : 1;

      if ( pet_type == PET_WIND_SERPENT )
      {
        action_list_str = "auto_attack/roar_of_recovery/wolverine_bite/lightning_breath/bite";
      }
      else assert( 0 );
    }
    else // TENACITY
    {
      assert( 0 );
    }

  }

  static bool supported( int family )
  {
    switch ( family )
    {
    case PET_CAT:
    case PET_DEVILSAUR:
    case PET_RAPTOR:
    case PET_WOLF:
    case PET_WIND_SERPENT:
      return true;
    default:
      return false;
    }
  }

  virtual void reset()
  {
    pet_t::reset();

    _buffs.reset();
    _expirations.reset();
  }

  virtual int group()
  {
    if ( pet_type < PET_FEROCITY ) return PET_FEROCITY;
    if ( pet_type < PET_TENACITY ) return PET_TENACITY;
    if ( pet_type < PET_CUNNING  ) return PET_CUNNING;
    return PET_NONE;
  }

  virtual bool ooc_buffs() { return true; }

  virtual void init_base()
  {
    pet_t::init_base();

    hunter_t* o = owner -> cast_hunter();

    attribute_base[ ATTR_STRENGTH  ] = rating_t::interpolate( level, 0, 162, 331 );
    attribute_base[ ATTR_AGILITY   ] = rating_t::interpolate( level, 0, 54, 113 );
    attribute_base[ ATTR_STAMINA   ] = rating_t::interpolate( level, 0, 307, 361 ); // stamina is different for every pet type
    attribute_base[ ATTR_INTELLECT ] = 100; // FIXME
    attribute_base[ ATTR_SPIRIT    ] = 100; // FIXME

    base_attack_power = -20;
    initial_attack_power_per_strength = 2.0;
    initial_attack_crit_per_agility   = rating_t::interpolate( level, 0.01/16.0, 0.01/30.0, 0.01/62.5 );
    initial_attack_power_multiplier *= 1.0 + o -> talents.animal_handler * 0.05;

    base_attack_crit = 0.032 + talents.spiders_bite * 0.03;
    base_attack_crit += o -> talents.ferocity * 0.02;

    resource_base[ RESOURCE_HEALTH ] = rating_t::interpolate( level, 0, 4253, 6373 );
    resource_base[ RESOURCE_FOCUS  ] = 100;

    focus_regen_per_second  = ( 24.5 / 4.0 );
    focus_regen_per_second *= 1.0 + o -> talents.bestial_discipline * 0.50;
  }

  virtual void init_gains()
  {
    pet_t::init_gains();
    gains_go_for_the_throat = get_gain( "go_for_the_throat" );
  }

  virtual void init_uptimes()
  {
    pet_t::init_uptimes();
    uptimes_frenzy         = owner -> get_uptime( "frenzy" );
    uptimes_monstrous_bite = owner -> get_uptime( "monstrous_bite" );
    uptimes_savage_rend    = owner -> get_uptime( "savage_rend" );
  }

  virtual double composite_armor() SC_CONST
  {
    double a = player_t::composite_armor();

    a *= 1.05; // 3.1 change: # Cunning, Ferocity and Tenacity pets now all have +5% damage, +5% armor and +5% health bonuses

    return a;
  }

  virtual double composite_attack_power() SC_CONST
  {
    hunter_t* o = owner -> cast_hunter();

    double ap = player_t::composite_attack_power();

    ap += o -> stamina() * o -> talents.hunter_vs_wild * 0.1;
    ap += o -> composite_attack_power() * 0.22 * ( 1 + talents.wild_hunt * 0.15 );
    ap += _buffs.furious_howl;

    if ( o -> active_aspect == ASPECT_BEAST )
      ap *= 1.1;

    if ( _buffs.rabid_power_stack )
      ap *= 1.0 + _buffs.rabid_power_stack * 0.05;

    if ( _buffs.call_of_the_wild )
      ap *= 1.1;

    return ap;
  }

  virtual double composite_spell_hit() SC_CONST
  {
    return composite_attack_hit() * 17.0 / 8.0;
  }

  virtual void summon( double duration=0 )
  {
    hunter_t* o = owner -> cast_hunter();
    pet_t::summon( duration );
    o -> active_pet = this;
  }

  virtual void interrupt()
  {
    pet_t::interrupt();
    if ( main_hand_attack ) main_hand_attack -> cancel();
  }

  virtual int primary_resource() SC_CONST { return RESOURCE_FOCUS; }

  virtual std::vector<option_t>& get_options()
  {
    if ( options.empty() )
    {
      pet_t::get_options();

      option_t hunter_pet_options[] =
      {
        // Talents
        { "cobra_reflexes",   OPT_INT, &( talents.cobra_reflexes   ) },
        { "owls_focus",       OPT_INT, &( talents.owls_focus       ) },
        { "shark_attack",     OPT_INT, &( talents.shark_attack     ) },
        { "spiked_collar",    OPT_INT, &( talents.spiked_collar    ) },
        { "feeding_frenzy",   OPT_INT, &( talents.feeding_frenzy   ) },
        { "roar_of_recovery", OPT_INT, &( talents.roar_of_recovery ) },
        { "wild_hunt",        OPT_INT, &( talents.wild_hunt        ) },
        { "wolverine_bite",   OPT_INT, &( talents.wolverine_bite   ) },
        { "spiders_bite",     OPT_INT, &( talents.spiders_bite     ) },
        { "call_of_the_wild", OPT_INT, &( talents.call_of_the_wild ) },
        { "rabid",            OPT_INT, &( talents.rabid            ) },
        { NULL, OPT_UNKNOWN, NULL }
      };

      option_t::copy( options, hunter_pet_options );
    }

    return options;
  }

  virtual bool parse_talents_armory( const std::string& talent_string )
  {
    std::vector<int*> tree;

    if ( group() == PET_FEROCITY )
    {
      talent_translation_t translation[] =
      {
        {  1, &( talents.cobra_reflexes         ) },
        {  2, NULL                                },
        {  3, NULL                                },
        {  4, NULL                                },
        {  5, NULL                                },
        {  6, NULL                                },
        {  7, &( talents.spiked_collar          ) },
        {  8, NULL                                },
        {  9, NULL                                },
        { 10, NULL                                },
        { 11, NULL                                },
        { 12, NULL                                },
        { 13, &( talents.spiders_bite           ) },
        { 14, NULL                                },
        { 15, &( talents.rabid                  ) },
        { 16, NULL                                },
        { 17, &( talents.call_of_the_wild       ) },
        { 18, &( talents.shark_attack           ) },
        { 19, &( talents.wild_hunt              ) },
        {  0, NULL                                }
      };
      get_talent_translation( tree, translation );
      return parse_talent_tree( tree, talent_string );
    }
    else if ( group() == PET_CUNNING )
    {
      talent_translation_t translation[] =
      {
        {  1, &( talents.cobra_reflexes         ) },
        {  2, NULL                                },
        {  3, NULL                                },
        {  4, NULL                                },
        {  5, NULL                                },
        {  6, NULL                                },
        {  7, &( talents.owls_focus             ) },
        {  8, &( talents.spiked_collar          ) },
        {  9, NULL                                },
        { 10, NULL                                },
        { 11, NULL                                },
        { 12, NULL                                },
        { 13, NULL                                },
        { 14, &( talents.feeding_frenzy         ) },
        { 15, &( talents.wolverine_bite         ) },
        { 16, &( talents.roar_of_recovery       ) },
        { 17, &( talents.call_of_the_wild       ) },
        { 18, NULL                                },
        { 19, &( talents.wild_hunt              ) },
        { 20, NULL                                },
        {  0, NULL                                }
      };
      get_talent_translation( tree, translation );
      return parse_talent_tree( tree, talent_string );
    }
    else // TENACITY
    {
      return false;
    }
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str );
};

// ==========================================================================
// Hunter Attack
// ==========================================================================

struct hunter_attack_t : public attack_t
{
  hunter_attack_t( const char* n, player_t* player, int s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true ) :
      attack_t( n, player, RESOURCE_MANA, s, t, special )
  {
    hunter_t* p = player -> cast_hunter();

    base_hit  += p -> talents.focused_aim * 0.01;
    base_crit += p -> talents.master_marksman * 0.01;
    base_crit += p -> talents.killer_instinct * 0.01;

    if ( p -> position == POSITION_RANGED )
    {
      base_crit += p -> talents.lethal_shots * 0.01;
    }

    base_multiplier *= 1.0 + p -> talents.improved_tracking * 0.01;

    range = -1; // unlimited
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
    if ( player -> items[ weapon -> slot ].encoded_enchant_str == "scope" )
    {
      double scope_damage = util_t::ability_rank( player -> level, 15.0,72,  12.0,67,  7.0,0 );

      base_dd_min += scope_damage * weapon_multiplier;
      base_dd_max += scope_damage * weapon_multiplier;
    }
  }

  virtual double cost() SC_CONST;
  virtual void   execute();
  virtual double execute_time() SC_CONST;
  virtual void   player_buff();
};

// ==========================================================================
// Hunter Spell
// ==========================================================================

struct hunter_spell_t : public spell_t
{
  hunter_spell_t( const char* n, player_t* p, int s, int t ) :
      spell_t( n, p, RESOURCE_MANA, s, t )
  {}

  virtual double gcd() SC_CONST;
};

namespace   // ANONYMOUS NAMESPACE ==========================================
{

// trigger_aspect_of_the_viper ==============================================

static void trigger_aspect_of_the_viper( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( p -> active_aspect != ASPECT_VIPER )
    return;

  double gain = p -> resource_max[ RESOURCE_MANA ] * p -> ranged_weapon.swing_time / 100.0;

  if ( p -> glyphs.aspect_of_the_viper ) gain *= 1.10;

  p -> resource_gain( RESOURCE_MANA, gain, p -> gains_viper_aspect_shot );
}

// trigger_cobra_strikes ===================================================

static void trigger_cobra_strikes( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.cobra_strikes )
    return;
  if ( ! p -> rng_cobra_strikes -> roll( p -> talents.cobra_strikes * 0.2 ) )
    return;

  struct cobra_strikes_expiration_t : public event_t
  {
    cobra_strikes_expiration_t( sim_t* sim, hunter_t* p ) : event_t( sim, p )
    {
      name = "Cobra Strikes Expiration";
      p -> aura_gain( "Cobra Strikes" );
      p -> _buffs.cobra_strikes = 2;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      hunter_t* p = player -> cast_hunter();
      p -> aura_loss( "Cobra Strikes" );
      p -> _buffs.cobra_strikes = 0;
      p -> _expirations.cobra_strikes = 0;
    }
  };

  event_t*& e = p -> _expirations.cobra_strikes;

  if ( e )
  {
    e -> reschedule( 10.0 );
    p -> _buffs.cobra_strikes = 2;
  }
  else
  {
    e = new ( a -> sim ) cobra_strikes_expiration_t( a -> sim, p );
  }
}

// consume_cobra_strikes ===================================================

static void consume_cobra_strikes( action_t* a )
{
  hunter_pet_t* p = ( hunter_pet_t* ) a -> player -> cast_pet();
  hunter_t*     o = p -> owner -> cast_hunter();

  if ( o -> _buffs.cobra_strikes )
  {
    o -> _buffs.cobra_strikes--;
    if ( ! o -> _buffs.cobra_strikes )
      event_t::early( o -> _expirations.cobra_strikes );
  }
}

// trigger_expose_weakness =================================================

static void trigger_expose_weakness( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.expose_weakness )
    return;

  if ( ! p -> rng_expose_weakness -> roll( p -> talents.expose_weakness / 3.0 ) )
    return;

  struct expose_weakness_expiration_t : public event_t
  {
    expose_weakness_expiration_t( sim_t* sim, hunter_t* p ) : event_t( sim, p )
    {
      name = "Expose Weakness Expiration";
      p -> aura_gain( "Expose Weakness" );
      p -> _buffs.expose_weakness = 1;
      sim -> add_event( this, 7.0 );
    }
    virtual void execute()
    {
      hunter_t* p = player -> cast_hunter();
      p -> aura_loss( "Expose Weakness" );
      p -> _buffs.expose_weakness = 0;
      p -> _expirations.expose_weakness = 0;
    }
  };

  event_t*& e = p -> _expirations.expose_weakness;

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
  hunter_pet_t* p = ( hunter_pet_t* ) a -> player -> cast_pet();

  if ( p -> talents.feeding_frenzy )
  {
    double target_pct = a -> sim -> target -> health_percentage();

    if ( target_pct < 35 )
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

  struct ferocious_inspiration_expiration_t : public event_t
  {
    ferocious_inspiration_expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Ferocious Inspiration Expiration";
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      for ( player_t* p = sim -> player_list; p; p = p -> next )
      {
        if ( p -> buffs.ferocious_inspiration )
        {
          p -> aura_loss( "Ferocious Inspiration" );
          p -> buffs.ferocious_inspiration = 0;
        }
      }
      sim -> expirations.ferocious_inspiration = 0;
    }
  };

  for ( player_t* p = a -> sim -> player_list; p; p = p -> next )
  {
    if ( p -> sleeping ) continue;
    if ( p -> buffs.ferocious_inspiration == 0 ) p -> aura_gain( "Ferocious Inspiration" );
    p -> buffs.ferocious_inspiration = 1;
  }

  event_t*& e = a -> sim -> expirations.ferocious_inspiration;

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
  hunter_pet_t* p = ( hunter_pet_t* ) a -> player -> cast_pet();
  hunter_t*     o = p -> owner -> cast_hunter();

  if ( ! o -> talents.frenzy ) return;
  if ( ! o -> rng_frenzy -> roll( o -> talents.frenzy * 0.2 ) ) return;

  struct frenzy_expiration_t : public event_t
  {
    frenzy_expiration_t( sim_t* sim, hunter_pet_t* p ) : event_t( sim, p )
    {
      name = "Frenzy Expiration";
      p -> aura_gain( "Frenzy" );
      p -> _buffs.frenzy = 1;
      sim -> add_event( this, 8.0 );
    }
    virtual void execute()
    {
      hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
      p -> aura_loss( "Frenzy" );
      p -> _buffs.frenzy = 0;
      p -> _expirations.frenzy = 0;
    }
  };

  event_t*& e = p -> _expirations.frenzy;

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

  p -> active_pet -> resource_gain( RESOURCE_FOCUS, gain, p -> active_pet -> gains_go_for_the_throat );
}

// trigger_hunting_party ===================================================

static void trigger_hunting_party( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.hunting_party )
    return;

  double chance = p -> talents.hunting_party / 3.0;

  if ( ! p -> rng_hunting_party -> roll( chance ) )
    return;

  p -> trigger_replenishment();
}

// trigger_improved_aspect_of_the_hawk =====================================

static void trigger_improved_aspect_of_the_hawk( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.improved_aspect_of_the_hawk )
    return;

  if ( p -> active_aspect != ASPECT_HAWK )
    return;

  if ( ! p -> rng_improved_aoth -> roll( 0.10 ) )
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
      p -> _buffs.improved_aspect_of_the_hawk = 0;
      p -> _expirations.improved_aspect_of_the_hawk = 0;
    }
  };

  p -> _buffs.improved_aspect_of_the_hawk = 0.03 * p -> talents.improved_aspect_of_the_hawk;

  if ( p -> glyphs.improved_aspect_of_the_hawk )
  {
    p -> _buffs.improved_aspect_of_the_hawk += 0.06;
  }

  event_t*& e = p -> _expirations.improved_aspect_of_the_hawk;

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

  if ( ! p -> rng_improved_steady_shot -> roll( p -> talents.improved_steady_shot * 0.05 ) )
    return;

  struct improved_steady_shot_expiration_t : public event_t
  {
    improved_steady_shot_expiration_t( sim_t* sim, hunter_t* p ) : event_t( sim, p )
    {
      name = "Improved Steady Shot Expiration";
      p -> _buffs.improved_steady_shot = 1;
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      hunter_t* p = player -> cast_hunter();
      p -> _buffs.improved_steady_shot = 0;
      p -> _expirations.improved_steady_shot = 0;
    }
  };

  event_t*& e = p -> _expirations.improved_steady_shot;

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
  event_t::early( p -> _expirations.improved_steady_shot );
}

// trigger_invigoration ==============================================

static void trigger_invigoration( action_t* a )
{
  hunter_pet_t* p = ( hunter_pet_t* ) a -> player -> cast_pet();
  hunter_t*     o = p -> owner -> cast_hunter();

  if ( ! o -> talents.invigoration )
    return;

  if ( ! o -> rng_invigoration -> roll( o -> talents.invigoration * 0.50 ) )
    return;

  o -> resource_gain( RESOURCE_MANA, 0.01 * o -> resource_max[ RESOURCE_MANA ], o -> gains_invigoration );
}

// consume_kill_command ==============================================

static void consume_kill_command( action_t* a )
{
  hunter_pet_t* p = ( hunter_pet_t* ) a -> player -> cast_pet();

  if ( p -> _buffs.kill_command > 0 )
  {
    p -> _buffs.kill_command--;

    if ( p -> _buffs.kill_command == 0 )
    {
      p -> aura_loss( "Kill Command" );
    }
  }
}

// trigger_lock_and_load =============================================

static void trigger_lock_and_load( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.lock_and_load )
    return;
  if ( ! a -> sim -> cooldown_ready( p -> _cooldowns.lock_and_load ) )
    return;

  // NB: talent calc says 3%,7%,10%, assuming it's really 10% * (1/3,2/3,3/3)
  double chance = p -> talents.lock_and_load * 0.1 / 3;

  if ( ! p -> rng_lock_and_load -> roll( chance ) )
    return;

  struct lock_and_load_expiration_t : public event_t
  {
    lock_and_load_expiration_t( sim_t* sim, hunter_t* p ) : event_t( sim, p )
    {
      name = "Lock and Load Expiration";
      p -> aura_gain( "Lock and Load" );
      p -> _cooldowns.lock_and_load = sim -> current_time + 22;
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      hunter_t* p = player -> cast_hunter();
      p -> aura_loss( "Lock and Load" );
      p -> _buffs.lock_and_load = 0;
      p -> _expirations.lock_and_load = 0;
    }
  };

  p -> _buffs.lock_and_load = 2;
  p -> procs_lock_and_load -> occur();

  event_t*& e = p -> _expirations.lock_and_load;

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

  if ( p -> _buffs.lock_and_load > 0 )
  {
    p -> _buffs.lock_and_load--;

    if ( p -> _buffs.lock_and_load == 0 )
    {
      event_t::early( p -> _expirations.lock_and_load );
    }
  }
}

// trigger_master_tactician ================================================

static void trigger_master_tactician( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.master_tactician )
    return;

  if ( ! p -> rng_master_tactician -> roll( 0.10 ) )
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
      p -> _buffs.master_tactician = 0;
      p -> _expirations.master_tactician = 0;
    }
  };

  p -> _buffs.master_tactician = p -> talents.master_tactician * 0.02;

  event_t*& e = p -> _expirations.master_tactician;

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
  hunter_pet_t* p = ( hunter_pet_t* ) a -> player -> cast_pet();
  hunter_t* o = p -> owner -> cast_hunter();

  if ( ! p -> talents.owls_focus )
    return;

  if ( ! o -> rng_owls_focus -> roll( p -> talents.owls_focus * 0.15 ) )
    return;

  struct owls_focus_expiration_t : public event_t
  {
    owls_focus_expiration_t( sim_t* sim, hunter_pet_t* p ) : event_t( sim, p )
    {
      name = "Owls Focus Expiration";
      p -> aura_gain( "Owls Focus" );
      p -> _buffs.owls_focus = 1;
      sim -> add_event( this, 8.0 );
    }
    virtual void execute()
    {
      hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
      p -> aura_loss( "Owls Focus" );
      p -> _buffs.owls_focus = 0;
      p -> _expirations.owls_focus = 0;
    }
  };

  event_t*& e = p -> _expirations.owls_focus;

  if ( e )
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
  hunter_pet_t* p = ( hunter_pet_t* ) a -> player -> cast_pet();
  event_t::early( p -> _expirations.owls_focus );
}

// trigger_piercing_shots

static void trigger_piercing_shots( action_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.piercing_shots )
    return;

  struct piercing_shots_t : public attack_t
  {
    piercing_shots_t( player_t* p ) : attack_t( "piercing_shots", p, RESOURCE_NONE, SCHOOL_BLEED )
    {
      may_miss    = false;
      may_crit    = true;
      background  = true;
      proc        = true;
      trigger_gcd = 0;
      base_cost   = 0;

      base_multiplier = 1.0;
      base_tick_time = 1.0;
      num_ticks      = 8;
      tick_power_mod = 0;
    }
    void player_buff() {}
    void target_debuff( int dmg_type )
    {
      target_t* t = sim -> target;
      if ( t -> debuffs.mangle -> up() || t -> debuffs.trauma -> up() )
      {
        target_multiplier = 1.30;
      }
    }
  };

  double dmg = p -> talents.piercing_shots * 0.1 * a -> direct_dmg;

  if ( ! p -> active_piercing_shots ) p -> active_piercing_shots = new piercing_shots_t( p );

  if ( p -> active_piercing_shots -> ticking )
  {
    int num_ticks = p -> active_piercing_shots -> num_ticks;
    int remaining_ticks = num_ticks - p -> active_piercing_shots -> current_tick;

    dmg += p -> active_piercing_shots -> base_td * remaining_ticks;

    p -> active_piercing_shots -> cancel();
  }
  p -> active_piercing_shots -> base_td = dmg / 8;
  p -> active_piercing_shots -> schedule_tick();
}

// trigger_rabid_power ===============================================

static void trigger_rabid_power( attack_t* a )
{
  hunter_pet_t* p = ( hunter_pet_t* ) a -> player -> cast_pet();
  hunter_t* o = p -> owner -> cast_hunter();

  if ( ! p -> talents.rabid )
    return;
  if ( ! p -> _buffs.rabid )
    return;
  // FIXME: Probably a ppm, not flat chance
  if ( p -> _buffs.rabid_power_stack == 5 || ! o -> rng_rabid_power -> roll( 0.5 ) )
    return;

  p -> _buffs.rabid_power_stack++;

  const char* name[] = { 0, "Rabid(1)", "Rabid(2)", "Rabid(3)", "Rabid(4)", "Rabid(5)" };

  p -> aura_gain( name[ p -> _buffs.rabid_power_stack ] );
}

// trigger_thrill_of_the_hunt ========================================

static void trigger_thrill_of_the_hunt( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.thrill_of_the_hunt )
    return;

  if ( ! p -> rng_thrill_of_the_hunt -> roll( p -> talents.thrill_of_the_hunt / 3.0 ) )
    return;

  p -> resource_gain( RESOURCE_MANA, a -> resource_consumed * 0.40, p -> gains_thrill_of_the_hunt );
}

// trigger_tier8_4pc =================================================

static void trigger_tier8_4pc( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> set_bonus.tier8_4pc() )
    return;

  if ( ! a -> sim -> cooldown_ready( p -> cooldowns.tier8_4pc ) )
    return;

  if ( ! p -> rngs.tier8_4pc -> roll( 0.1 ) )
    return;

  struct precision_shots_expiration_t : public event_t
  {
    precision_shots_expiration_t( sim_t* sim, hunter_t* p ) : event_t( sim, p )
    {
      name = "Precision Shots Expiration";
      p -> aura_gain( "Precision Shots" );
      p -> cooldowns.tier8_4pc = sim -> current_time + 45;
      p -> stat_gain( STAT_ATTACK_POWER, 600 );
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      hunter_t* p = player -> cast_hunter();
      p -> aura_loss( "Precision Shots" );
      p -> stat_loss( STAT_ATTACK_POWER, 600 );
      p -> expirations.tier8_4pc = 0;
    }
  };

  p -> procs.tier8_4pc -> occur();
  p -> expirations.tier8_4pc = new ( a -> sim ) precision_shots_expiration_t( a -> sim, p );
}

// trigger_tier9_4pc =================================================

static void trigger_tier9_4pc( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> set_bonus.tier9_4pc() )
    return;

  if ( ! a -> sim -> cooldown_ready( p -> cooldowns.tier9_4pc ) )
    return;

  if ( ! p -> rngs.tier9_4pc -> roll( 0.35 ) )
    return;

  struct greatness_expiration_t : public event_t
  {
    greatness_expiration_t( sim_t* sim, hunter_t* p ) : event_t( sim, p )
    {
      name = "Greatness Expiration";
      p -> active_pet -> aura_gain( "Greatness" );
      p -> cooldowns.tier9_4pc = sim -> current_time + 45;
      p -> active_pet -> stat_gain(STAT_ATTACK_POWER, 600);
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      hunter_t* p = player -> cast_hunter();
      p -> active_pet -> aura_loss( "Greatness" );
      p -> active_pet -> stat_loss(STAT_ATTACK_POWER, 600);
      p -> expirations.tier9_4pc = 0;
    }
  };

  p -> procs.tier9_4pc -> occur();
  p -> expirations.tier9_4pc = new ( a -> sim ) greatness_expiration_t( a -> sim, p );
}

// trigger_wild_quiver ===============================================

static void trigger_wild_quiver( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( a -> proc )
    return;

  if ( ! p -> talents.wild_quiver )
    return;

  double chance = 0;
  chance = p -> talents.wild_quiver * 0.04;

  if ( p -> rng_wild_quiver -> roll( chance ) )
  {
    // FIXME! What hit/crit talents apply? At least Lethal Shots & Master Marksman
    // FIXME! Which proc-related talents can it trigger?
    // FIXME! Currently coded to benefit from all talents affecting ranged attacks.
    // FIXME! Talents that do not affect it can filter on "proc=true".

    struct wild_quiver_t : public hunter_attack_t
    {
      wild_quiver_t( hunter_t* player ) : hunter_attack_t( "wild_quiver", player, SCHOOL_NATURE, TREE_MARKSMANSHIP )
      {
        may_crit    = true;
        background  = true;
        proc        = true;
        trigger_gcd = 0;
        base_cost   = 0;
        base_dd_min = base_dd_max = 0.01;

        weapon = &( player -> ranged_weapon );
        assert( weapon -> group() == WEAPON_RANGED );
        base_multiplier *= player -> ranged_weapon_specialization_multiplier();
        base_multiplier *= 0.80;
        add_ammunition();
        add_scope();
	reset();
      }
    };

    if ( ! p -> active_wild_quiver ) p -> active_wild_quiver = new wild_quiver_t( p );

    p -> procs_wild_quiver -> occur();
    p -> active_wild_quiver -> execute();
  }
}

// trigger_wolverine_bite ==================================================

static void trigger_wolverine_bite( attack_t* a )
{
  hunter_pet_t* p = ( hunter_pet_t* ) a -> player -> cast_pet();

  if ( ! p -> talents.wolverine_bite )
    return;

  p -> _buffs.wolverine_bite = 1;
}

// check_pet_type ==========================================================

static void check_pet_type( action_t* a, int pet_type )
{
  hunter_pet_t* p = ( hunter_pet_t* ) a -> player -> cast_pet();
  hunter_t*     o = p -> owner -> cast_hunter();

  if ( p -> pet_type != pet_type )
  {
    util_t::printf( "\nsimcraft: Player %s has pet %s attempting to use action %s that is not available to that class of pets.\n",
                    o -> name(), p -> name(), a -> name() );
    a -> background = true;
  }
}

} // ANONYMOUS NAMESPACE ===================================================

// =========================================================================
// Hunter Pet Attacks
// =========================================================================

struct hunter_pet_attack_t : public attack_t
{
  hunter_pet_attack_t( const char* n, player_t* player, int r=RESOURCE_FOCUS, int sc=SCHOOL_PHYSICAL, bool special=true ) :
      attack_t( n, player, r, sc, TREE_BEAST_MASTERY, special )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    may_crit = true;

    direct_power_mod = 1.0/14;

    base_multiplier *= 1.05; // Cunning, Ferocity and Tenacity pets all have +5% damag
    base_hit  += p -> owner -> cast_hunter() -> talents.focused_aim * 0.01;

    // Orc Command Racial
    if ( o -> race == RACE_ORC )
    {
      base_multiplier *= 1.05;
    }

    // Assume happy pet
    base_multiplier *= 1.25;

    base_multiplier *= 1.0 + p -> talents.spiked_collar * 0.03;
    base_multiplier *= 1.0 + p -> talents.shark_attack * 0.03;
    base_multiplier *= 1.0 + o -> talents.unleashed_fury * 0.03;
    base_multiplier *= 1.0 + o -> talents.kindred_spirits * 0.04;
  }

  virtual double execute_time() SC_CONST
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    double t = attack_t::execute_time();
    if ( t == 0 ) return 0;

    if ( o -> talents.frenzy )
    {
      if ( p -> _buffs.frenzy ) t *= 1.0 / 1.3;

      p -> uptimes_frenzy -> update( p -> _buffs.frenzy > 0 );
    }

    return t;
  }

  virtual double cost() SC_CONST
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    if ( p -> _buffs.owls_focus ) return 0;

    return attack_t::cost();
  }

  virtual void consume_resource()
  {
    attack_t::consume_resource();
    if ( special && base_cost > 0 )
    {
      consume_owls_focus( this );
    }
  }

  virtual void execute()
  {
    attack_t::execute();
    if ( result_is_hit() )
    {
      trigger_rabid_power( this );

      if ( result == RESULT_CRIT )
      {
        trigger_ferocious_inspiration( this );
        trigger_frenzy( this );
        if ( special ) trigger_invigoration( this );
      }
    }
    else if ( result == RESULT_DODGE )
    {
      trigger_wolverine_bite( this );
    }
    if ( special )
    {
      consume_cobra_strikes( this );
      consume_kill_command( this );
      trigger_owls_focus( this );
    }
  }

  virtual void player_buff()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    attack_t::player_buff();

    if ( p -> _buffs.bestial_wrath ) player_multiplier *= 1.50;

    if ( p -> _buffs.savage_rend   ) player_multiplier *= 1.10;
    p -> uptimes_savage_rend -> update( p -> _buffs.savage_rend != 0 );

    player_multiplier *= 1.0 + p -> _buffs.monstrous_bite * 0.03;
    p -> uptimes_monstrous_bite -> update( p -> _buffs.monstrous_bite == 3 );

    trigger_feeding_frenzy( this );

    if ( special )
    {
      if ( o -> _buffs.cobra_strikes ) player_crit += 1.0;

      if ( p -> _buffs.kill_command )
      {
        player_multiplier *= 1.0 + p -> _buffs.kill_command * 0.20;
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
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    weapon = &( p -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    base_dd_min       = base_dd_max = 1;
    background        = true;
    repeating         = true;
    direct_power_mod  = 0;

    if ( p -> talents.cobra_reflexes )
    {
      base_multiplier   *= ( 1.0 - p -> talents.cobra_reflexes * 0.075 );
      base_execute_time *= 1.0 / ( 1.0 + p -> talents.cobra_reflexes * 0.15 );
    }
    base_execute_time *= 1.0 / ( 1.0 + 0.04 * o -> talents.serpents_swiftness );

    if ( o -> set_bonus.tier7_2pc() ) base_multiplier *= 1.05;
  }
};

// Pet Auto Attack ==========================================================

struct pet_auto_attack_t : public hunter_pet_attack_t
{
  pet_auto_attack_t( player_t* player, const std::string& options_str ) :
      hunter_pet_attack_t( "auto_attack", player )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    p -> main_hand_attack = new pet_melee_t( player );
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    p -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
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
    base_dd_min = base_dd_max = 143;
    base_cost = 25;
  }
};

// Cat Rake ===================================================================

struct rake_t : public hunter_pet_attack_t
{
  rake_t( player_t* player, const std::string& options_str ) :
      hunter_pet_attack_t( "rake", player, RESOURCE_FOCUS, SCHOOL_BLEED )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    check_pet_type( this, PET_CAT );

    parse_options( 0, options_str );

    base_dd_min = base_dd_max = 57;
    base_cost       = 20;
    base_td_init    = 21;
    num_ticks       = 3;
    base_tick_time  = 3;
    tick_power_mod  = 0.0175;
    cooldown        = 10 * ( 1.0 - o -> talents.longevity * 0.10 );

    // FIXME! Assuming pets are not smart enough to wait for Rake to finish ticking
    clip_dot = true;

    id = 59886;
  }

  virtual void execute()
  {
    school = SCHOOL_PHYSICAL;
    hunter_pet_attack_t::execute();
    school = SCHOOL_BLEED;
  }
};

// Devilsaur Monstrous Bite ===================================================

struct monstrous_bite_t : public hunter_pet_attack_t
{
  monstrous_bite_t( player_t* player, const std::string& options_str ) :
      hunter_pet_attack_t( "monstrous_bite", player, RESOURCE_FOCUS, SCHOOL_PHYSICAL )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    check_pet_type( this, PET_DEVILSAUR );

    parse_options( 0, options_str );

    base_dd_min = base_dd_max = 107;
    base_cost = 20;
    cooldown  = 10 * ( 1.0 - o -> talents.longevity * 0.10 );
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    hunter_pet_attack_t::execute();

    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, hunter_pet_t* p ) : event_t( sim, p )
      {
        name = "Monstrous Bite Expiration";
        p -> aura_gain( "Monstrous Bite" );
        sim -> add_event( this, 12.0 );
      }
      virtual void execute()
      {
        hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
        p -> aura_loss( "Monstrous Bite" );
        p -> _buffs.monstrous_bite = 0;
        p -> _expirations.monstrous_bite = 0;
      }
    };

    if ( p -> _buffs.monstrous_bite < 3 )
    {
      p -> _buffs.monstrous_bite++;
    }

    event_t*& e = p -> _expirations.monstrous_bite;

    if ( e )
    {
      e -> reschedule( 12.0 );
    }
    else
    {
      e = new ( sim ) expiration_t( sim, p );
    }
  }
};

// Raptor Savage Rend =========================================================

struct savage_rend_t : public hunter_pet_attack_t
{
  savage_rend_t( player_t* player, const std::string& options_str ) :
      hunter_pet_attack_t( "savage_rend", player, RESOURCE_FOCUS, SCHOOL_BLEED )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    check_pet_type( this, PET_RAPTOR );

    parse_options( 0, options_str );

    base_dd_min = base_dd_max = 71;
    base_cost      = 20;
    base_td_init   = 24;
    num_ticks      = 3;
    base_tick_time = 5;
    tick_power_mod = 0.0175; // FIXME Check
    cooldown       = 60 * ( 1.0 - o -> talents.longevity * 0.10 );

    // FIXME! Assuming pets are not smart enough to wait for Rake to finish ticking
    clip_dot = true;
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    school = SCHOOL_PHYSICAL;
    hunter_pet_attack_t::execute();
    school = SCHOOL_BLEED;

    if ( result == RESULT_CRIT )
    {
      struct expiration_t : public event_t
      {
        expiration_t( sim_t* sim, hunter_pet_t* p ) : event_t( sim, p )
        {
          name = "Savage Rend Expiration";
          p -> aura_gain( "Savage Rend" );
          p -> _buffs.savage_rend = 1;
          sim -> add_event( this, 30.0 );
        }
        virtual void execute()
        {
          hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
          p -> aura_loss( "Savage Rend" );
          p -> _buffs.savage_rend = 0;
          p -> _expirations.savage_rend = 0;
        }
      };

      event_t*& e = p -> _expirations.savage_rend;

      if ( e )
      {
        e -> reschedule( 30.0 );
      }
      else
      {
        e = new ( sim ) expiration_t( sim, p );
      }
    }
  }
};

// Wolverine Bite =============================================================

struct wolverine_bite_t : public hunter_pet_attack_t
{
  wolverine_bite_t( player_t* player, const std::string& options_str ) :
      hunter_pet_attack_t( "wolverine_bite", player, RESOURCE_FOCUS, SCHOOL_PHYSICAL )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    check_talent( p -> talents.wolverine_bite );

    parse_options( 0, options_str );

    base_dd_min = base_dd_max  = 5 * p -> level;
    base_cost   = 0;
    cooldown    = 10 * ( 1.0 - o -> talents.longevity * 0.10 );

    may_dodge = may_block = may_parry = false;
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    p -> _buffs.wolverine_bite = 0;
    hunter_pet_attack_t::execute();
  }

  virtual bool ready()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    // This attack is only available after the target dodges
    if ( ! p -> _buffs.wolverine_bite )
      return false;

    return hunter_pet_attack_t::ready();
  }
};

// =========================================================================
// Hunter Pet Spells
// =========================================================================

// FIXME: 3.1 is supposed to give pets more spell hit, need to figure out
// the exact mechanics
struct hunter_pet_spell_t : public spell_t
{
  hunter_pet_spell_t( const char* n, player_t* player, int r=RESOURCE_FOCUS, int s=SCHOOL_PHYSICAL ) :
      spell_t( n, player, r, s )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    base_multiplier *= 1.05; // 3.1.0 change: # Cunning, Ferocity and Tenacity pets now all have +5% damage, +5% armor and +5% health bonuses.
    base_hit  += p -> owner -> cast_hunter() -> talents.focused_aim * 0.01;

    base_multiplier *= 1.0 + p -> talents.spiked_collar * 0.03;
  }

  virtual double cost() SC_CONST
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    if ( p -> _buffs.owls_focus ) return 0;

    return spell_t::cost();
  }

  virtual void consume_resource()
  {
    spell_t::consume_resource();
    if ( base_cost > 0 )
    {
      consume_owls_focus( this );
    }
  }

  virtual void execute()
  {
    spell_t::execute();
    if ( result_is_hit() )
    {
      if ( result == RESULT_CRIT )
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
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    spell_t::player_buff();

    player_spell_power += 0.125 * o -> composite_attack_power();

    if ( p -> _buffs.bestial_wrath ) player_multiplier *= 1.50;

    if ( o -> _buffs.cobra_strikes ) player_crit += 1.0;

    if ( p -> _buffs.kill_command )
    {
      player_multiplier *= 1.0 + p -> _buffs.kill_command * 0.20;
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
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    check_pet_type( this, PET_CHIMERA );

    parse_options( 0, options_str );

    base_dd_min = base_dd_max = 150;
    base_cost        = 20;
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
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    check_pet_type( this, PET_WIND_SERPENT );

    parse_options( 0, options_str );

    base_dd_min = base_dd_max = 100;
    base_cost        = 20;
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
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    check_talent( p -> talents.call_of_the_wild );

    parse_options( 0, options_str );

    base_cost = 0;
    cooldown  = 5 * 60 * ( 1.0 - o -> talents.longevity * 0.10 );
    trigger_gcd = 0.0;
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, hunter_pet_t* p ) : event_t( sim, p )
      {
        hunter_t*     o = p -> owner -> cast_hunter();
        name = "Call of the Wild Expiration";
        p -> aura_gain( "Call of the Wild" );
        o -> aura_gain( "Call of the Wild" );
        p -> _buffs.call_of_the_wild = 1;
        o -> _buffs.call_of_the_wild = 1;
        sim -> add_event( this, 20.0 );
      }
      virtual void execute()
      {
        hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
        hunter_t*     o = p -> owner -> cast_hunter();
        p -> aura_loss( "Call of the Wild" );
        o -> aura_loss( "Call of the Wild" );
        p -> _buffs.call_of_the_wild = 0;
        o -> _buffs.call_of_the_wild = 0;
      }
    };

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    update_ready();
    new ( sim ) expiration_t( sim, p );
  }
};

// Furious Howl ===============================================================

struct furious_howl_t : public hunter_pet_spell_t
{
  furious_howl_t( player_t* player, const std::string& options_str ) :
      hunter_pet_spell_t( "furious_howl", player, RESOURCE_FOCUS )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    check_pet_type( this, PET_WOLF );

    parse_options( 0, options_str );

    base_cost = 20;
    cooldown  = 40 * ( 1.0 - o -> talents.longevity * 0.10 );
    trigger_gcd = 0.0;
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, hunter_pet_t* p ) : event_t( sim, p )
      {
        hunter_t*     o = p -> owner -> cast_hunter();
        name = "Call of the Wild Expiration";
        p -> aura_gain( "Furious Howl" );
        o -> aura_gain( "Furious Howl" );
        p -> _buffs.furious_howl = 4 * p -> level;
        o -> _buffs.furious_howl = 4 * p -> level;
        sim -> add_event( this, 20.0 );
      }
      virtual void execute()
      {
        hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
        hunter_t*     o = p -> owner -> cast_hunter();
        p -> aura_loss( "Furious Howl" );
        o -> aura_loss( "Furious Howl" );
        p -> _buffs.furious_howl = 0;
        o -> _buffs.furious_howl = 0;
      }
    };

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    update_ready();
    new ( sim ) expiration_t( sim, p );
  }
};

// Rabid ======================================================================

struct rabid_t : public hunter_pet_spell_t
{
  rabid_t( player_t* player, const std::string& options_str ) :
      hunter_pet_spell_t( "rabid", player, RESOURCE_FOCUS )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    check_talent( p -> talents.rabid );

    parse_options( 0, options_str );

    base_cost = 0;
    cooldown  = 45 * ( 1.0 - o -> talents.longevity * 0.10 );
    trigger_gcd = 0.0;
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    struct rabid_expiration_t : public event_t
    {
      rabid_expiration_t( sim_t* sim, hunter_pet_t* p ) : event_t( sim, p )
      {
        name = "Rabid Expiration";
        p -> aura_gain( "Rabid" );
        p -> _buffs.rabid = 1;
        assert( p -> _buffs.rabid_power_stack == 0 );
        sim -> add_event( this, 20.0 );
      }
      virtual void execute()
      {
        hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
        p -> aura_loss( "Rabid" );
        p -> _buffs.rabid = 0;
        p -> _buffs.rabid_power_stack = 0;
      }
    };

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    update_ready();
    new ( sim ) rabid_expiration_t( sim, p );
  }
};

// Roar of Recovery ===========================================================

struct roar_of_recovery_t : public hunter_pet_spell_t
{
  roar_of_recovery_t( player_t* player, const std::string& options_str ) :
      hunter_pet_spell_t( "roar_of_recovery", player, RESOURCE_FOCUS )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    check_talent( p -> talents.roar_of_recovery );

    parse_options( 0, options_str );

    trigger_gcd    = 0.0;
    base_cost      = 0;
    num_ticks      = 3;
    base_tick_time = 3;
    cooldown       = 360 * ( 1.0 - o -> talents.longevity * 0.10 );
  }

  virtual void tick()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    o -> resource_gain( RESOURCE_MANA, 0.10 * o -> resource_max[ RESOURCE_MANA ], o -> gains_roar_of_recovery );
  }

  virtual bool ready()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    if ( ( o -> resource_current[ RESOURCE_MANA ] /
           o -> resource_max    [ RESOURCE_MANA ] ) > 0.50 )
      return false;

    return hunter_pet_spell_t::ready();
  }
};

// hunter_pet_t::create_action =============================================

action_t* hunter_pet_t::create_action( const std::string& name,
                                       const std::string& options_str )
{
  if ( name == "auto_attack"       ) return new   pet_auto_attack_t( this, options_str );
  if ( name == "bite"              ) return new        focus_dump_t( this, options_str, "bite" );
  if ( name == "call_of_the_wild"  ) return new  call_of_the_wild_t( this, options_str );
  if ( name == "claw"              ) return new        focus_dump_t( this, options_str, "claw" );
  if ( name == "froststorm_breath" ) return new froststorm_breath_t( this, options_str );
  if ( name == "furious_howl"      ) return new      furious_howl_t( this, options_str );
  if ( name == "lightning_breath"  ) return new  lightning_breath_t( this, options_str );
  if ( name == "monstrous_bite"    ) return new    monstrous_bite_t( this, options_str );
  if ( name == "rabid"             ) return new             rabid_t( this, options_str );
  if ( name == "rake"              ) return new              rake_t( this, options_str );
  if ( name == "roar_of_recovery"  ) return new  roar_of_recovery_t( this, options_str );
  if ( name == "savage_rend"       ) return new       savage_rend_t( this, options_str );
  if ( name == "smack"             ) return new        focus_dump_t( this, options_str, "smack" );
  if ( name == "wolverine_bite"    ) return new    wolverine_bite_t( this, options_str );

  return pet_t::create_action( name, options_str );
}

// =========================================================================
// Hunter Attacks
// =========================================================================

// hunter_attack_t::cost ===================================================

double hunter_attack_t::cost() SC_CONST
{
  hunter_t* p = player -> cast_hunter();
  double c = attack_t::cost();
  if ( c == 0 ) return 0;
  c *= 1.0 - p -> talents.efficiency * 0.03;
  if ( p -> _buffs.beast_within ) c *= 0.80;
  return c;
}

// hunter_attack_t::execute ================================================

void hunter_attack_t::execute()
{
  attack_t::execute();

  if ( result_is_hit() )
  {
    trigger_aspect_of_the_viper( this );
    trigger_master_tactician( this );
    trigger_tier9_4pc( this );

    if ( result == RESULT_CRIT )
    {
      trigger_expose_weakness( this );
      trigger_go_for_the_throat( this );
      trigger_thrill_of_the_hunt( this );
    }
  }
  hunter_t* p = player -> cast_hunter();
  p -> uptimes.tier8_4pc -> update( p -> expirations.tier8_4pc != 0 );
}

// hunter_attack_t::execute_time ============================================

double hunter_attack_t::execute_time() SC_CONST
{
  hunter_t* p = player -> cast_hunter();

  double t = attack_t::execute_time();
  if ( t == 0 ) return 0;

  if ( p -> _buffs.rapid_fire )
  {
    t *= 1.0 / ( 1.0 + p -> _buffs.rapid_fire );
  }

  t *= 1.0 / p -> quiver_haste;

  t *= 1.0 / ( 1.0 + 0.04 * p -> talents.serpents_swiftness );

  t *= 1.0 / ( 1.0 + p -> _buffs.improved_aspect_of_the_hawk );

  p -> uptimes_improved_aspect_of_the_hawk -> update( p -> _buffs.improved_aspect_of_the_hawk != 0 );
  p -> uptimes_rapid_fire -> update( p -> _buffs.rapid_fire != 0 );

  return t;
}

// hunter_attack_t::player_buff =============================================

void hunter_attack_t::player_buff()
{
  hunter_t* p = player -> cast_hunter();
  target_t* t = sim -> target;

  attack_t::player_buff();

  if ( t -> debuffs.hunters_mark )
  {
    if ( weapon && weapon -> group() == WEAPON_RANGED )
    {
      // FIXME: This should be all shots, not just weapon-based shots
      player_multiplier *= 1.0 + p -> talents.marked_for_death * 0.01;
    }
  }
  if ( p -> _buffs.aspect_of_the_viper )
  {
    player_multiplier *= p -> _buffs.aspect_of_the_viper;
  }
  if ( p -> _buffs.beast_within )
  {
    player_multiplier *= 1.10;
  }
  if ( p -> active_sting() && p -> talents.noxious_stings )
  {
    player_multiplier *= 1.0 + p -> talents.noxious_stings * 0.01;
  }
  if ( p -> active_pet )
  {
    player_multiplier *= 1.0 + p -> talents.focused_fire * 0.01;
  }
  player_multiplier *= 1.0 + p -> active_black_arrow * 0.06;
  player_crit += p -> _buffs.master_tactician;

  p -> uptimes_aspect_of_the_viper -> update( p -> _buffs.aspect_of_the_viper != 0 );
  p -> uptimes_master_tactician    -> update( p -> _buffs.master_tactician    != 0 );
}

// Ranged Attack ===========================================================

struct ranged_t : public hunter_attack_t
{
  // FIXME! Setting "special=true" would create the desired 2-roll effect,
  // but it would also triger Honor-Among-Thieves procs.......

  ranged_t( player_t* player ) :
      hunter_attack_t( "ranged", player, SCHOOL_PHYSICAL, TREE_NONE, /*special*/false )
  {
    hunter_t* p = player -> cast_hunter();

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );
    base_execute_time = weapon -> swing_time;

    may_crit    = true;
    background  = true;
    repeating   = true;
    trigger_gcd = 0;
    base_cost   = 0;
    base_dd_min = 1;
    base_dd_max = 1;

    base_multiplier *= p -> ranged_weapon_specialization_multiplier();

    add_ammunition();
    add_scope();
  }

  void execute()
  {
    hunter_attack_t::execute();

    if ( result_is_hit() )
    {
      trigger_wild_quiver( this );
      trigger_improved_aspect_of_the_hawk( this );
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
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    p -> ranged_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();
    if ( p -> moving ) return false;
    return( p -> ranged_attack -> execute_event == 0 ); // not swinging
  }
};

// Aimed Shot ================================================================

struct aimed_shot_t : public hunter_attack_t
{
  int improved_steady_shot;

  aimed_shot_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "aimed_shot", player, SCHOOL_PHYSICAL, TREE_MARKSMANSHIP ),
      improved_steady_shot( 0 )
  {
    hunter_t* p = player -> cast_hunter();

    check_talent( p -> talents.aimed_shot );

    option_t options[] =
    {
      { "improved_steady_shot", OPT_BOOL, &improved_steady_shot },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 9, 408, 408, 0, 0.08 },
      { 75, 8, 345, 345, 0, 0.08 },
      { 70, 7, 205, 205, 0, 0.08 },
      { 60, 6, 150, 150, 0, 0.12 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    may_crit = true;
    normalize_weapon_speed = true;

    cooldown = 10;
    cooldown_group = "aimed_multi";

    base_cost *= 1.0 - p -> talents.master_marksman * 0.05;

    base_multiplier *= 1.0 + p -> talents.barrage                      * 0.04;
    base_multiplier *= 1.0 + p -> talents.sniper_training              * 0.02;
    base_multiplier *= p -> ranged_weapon_specialization_multiplier();

    base_crit += p -> talents.improved_barrage * 0.04;

    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.mortal_shots     * 0.06 +
                                          p -> talents.marked_for_death * 0.02 );

    add_ammunition();
    add_scope();

    if ( p -> glyphs.aimed_shot )
    {
      cooldown -= 2;
    }
  }

  virtual double cost() SC_CONST
  {
    hunter_t* p = player -> cast_hunter();
    double c = hunter_attack_t::cost();
    if ( p -> _buffs.improved_steady_shot ) c *= 0.80;
    return c;
  }

  void player_buff()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::player_buff();
    if ( p -> _buffs.trueshot_aura && p -> glyphs.trueshot_aura ) player_crit += 0.10;
    if ( p -> _buffs.improved_steady_shot ) player_multiplier *= 1.15;
    p -> uptimes_improved_steady_shot -> update( p -> _buffs.improved_steady_shot != 0 );
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    consume_improved_steady_shot( this );
    if ( result == RESULT_CRIT )
    {
      trigger_piercing_shots( this );
    }
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( improved_steady_shot )
      if ( ! p -> _buffs.improved_steady_shot )
        return false;

    return hunter_attack_t::ready();
  }
};

// Arcane Shot Attack =========================================================

struct arcane_shot_t : public hunter_attack_t
{
  int improved_steady_shot;
  int lock_and_load;

  arcane_shot_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "arcane_shot", player, SCHOOL_ARCANE, TREE_MARKSMANSHIP ),
      improved_steady_shot( 0 ), lock_and_load( 0 )
  {
    hunter_t* p = player -> cast_hunter();

    option_t options[] =
    {
      { "improved_steady_shot", OPT_BOOL, &improved_steady_shot },
      { "lock_and_load",        OPT_BOOL, &lock_and_load        },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79, 11, 492, 492, 0, 0.05 },
      { 73, 10, 402, 402, 0, 0.05 },
      { 69, 9,  273, 273, 0, 0.05 },
      { 60, 8,  200, 200, 0, 0.07 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    // To trigger ppm-based JoW
    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );
    weapon_multiplier = 0;

    may_crit = true;
    cooldown = 6;
    cooldown_group = "arcane_explosive";

    direct_power_mod = 0.15;

    base_multiplier *= 1.0 + ( p -> talents.improved_arcane_shot  * 0.05 +
                               p -> talents.ferocious_inspiration * 0.03 );
    base_multiplier *= p -> ranged_weapon_specialization_multiplier();

    base_crit += p -> talents.survival_instincts * 0.02;

    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.mortal_shots     * 0.06 +
                                          p -> talents.marked_for_death * 0.02 );
  }

  virtual double cost() SC_CONST
  {
    hunter_t* p = player -> cast_hunter();
    if ( p -> _buffs.lock_and_load ) return 0;
    double c = hunter_attack_t::cost();
    if ( p -> _buffs.improved_steady_shot ) c *= 0.80;
    return c;
  }

  void player_buff()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::player_buff();
    if ( p -> _buffs.improved_steady_shot ) player_multiplier *= 1.15;
    p -> uptimes_improved_steady_shot -> update( p -> _buffs.improved_steady_shot > 0 );
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    if ( result == RESULT_CRIT )
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
    cooldown = p -> _buffs.lock_and_load ? 0.0 : 6.0;
    hunter_attack_t::update_ready();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( improved_steady_shot )
      if ( ! p -> _buffs.improved_steady_shot )
        return false;

    if ( lock_and_load )
      if ( ! p -> _buffs.lock_and_load )
        return false;

    return hunter_attack_t::ready();
  }
};

// Black Arrow =================================================================

struct black_arrow_t : public hunter_attack_t
{
  black_arrow_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "black_arrow", player, SCHOOL_SHADOW, TREE_SURVIVAL )
  {
    hunter_t* p = player -> cast_hunter();

    check_talent( p -> talents.black_arrow );

    static rank_t ranks[] =
    {
      { 80, 6, 0, 0, 2765/5.0, 0.06 },
      { 75, 5, 0, 0, 2240/5.0, 0.06 },
      { 69, 4, 0, 0, 1480/5.0, 0.06 },
      { 63, 3, 0, 0, 1250/5.0, 0.06 },
      { 57, 2, 0, 0,  940/5.0, 0.06 },
      { 50, 1, 0, 0,  785/5.0, 0.06 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    base_tick_time   = 3.0;
    num_ticks        = 5;
    tick_power_mod   = 0.1 / 5.0;
    cooldown         = 30 - p -> talents.resourcefulness * 2;
    cooldown_group   = "traps";

    base_multiplier *= 1.0 + p -> talents.sniper_training * 0.02;
    base_multiplier *= 1.0 + p -> talents.tnt * 0.02;
    base_multiplier *= 1.0 + p -> talents.trap_mastery * 0.10;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::execute();
    if ( result_is_hit() )
      p -> active_black_arrow = 1;
  }

  virtual void tick()
  {
    hunter_attack_t::tick();
    trigger_lock_and_load( this );
  }

  virtual void last_tick()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::last_tick();
    p -> active_black_arrow = 0;
  }
};

// Chimera Shot ================================================================

struct chimera_shot_t : public hunter_attack_t
{
  int active_sting;
  int improved_steady_shot;

  chimera_shot_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "chimera_shot", player, SCHOOL_NATURE, TREE_MARKSMANSHIP ),
      active_sting( 1 ), improved_steady_shot( 0 )
  {
    hunter_t* p = player -> cast_hunter();

    check_talent( p -> talents.chimera_shot );

    option_t options[] =
    {
      { "active_sting",         OPT_BOOL, &active_sting         },
      { "improved_steady_shot", OPT_BOOL, &improved_steady_shot },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    may_crit     = true;
    base_dd_min = 1;
    base_dd_max = 1;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.12;
    base_cost  *= 1.0 - p -> talents.master_marksman * 0.05;

    normalize_weapon_speed = true;
    weapon_multiplier      = 1.25;
    cooldown               = 10;

    base_multiplier *= p -> ranged_weapon_specialization_multiplier();

    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.mortal_shots     * 0.06 +
                                          p -> talents.marked_for_death * 0.02 );

    add_ammunition();
    add_scope();

    if ( p -> glyphs.chimera_shot )
    {
      cooldown -= 1;
    }
  }

  virtual double cost() SC_CONST
  {
    hunter_t* p = player -> cast_hunter();
    double c = hunter_attack_t::cost();
    if ( p -> _buffs.improved_steady_shot ) c *= 0.80;
    return c;
  }

  void player_buff()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::player_buff();
    if ( p -> _buffs.improved_steady_shot ) player_multiplier *= 1.0 + 0.15;
    p -> uptimes_improved_steady_shot -> update( p -> _buffs.improved_steady_shot > 0 );
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();

    hunter_attack_t::execute();

    action_t* sting = p -> active_sting();

    if ( result_is_hit() && sting )
    {
      sting -> refresh_duration();
      sting -> result = RESULT_HIT;

      double sting_dmg = sting -> num_ticks * sting -> calculate_tick_damage();

      if ( p -> active_serpent_sting )
      {
        struct chimera_serpent_t : public hunter_attack_t
        {
          chimera_serpent_t( player_t* p ) : hunter_attack_t( "chimera_serpent", p, SCHOOL_NATURE, TREE_MARKSMANSHIP )
          {
            // FIXME! Which procs can be triggered by this attack?
            may_crit    = true;
            background  = true;
            proc        = true;
            trigger_gcd = 0;
            base_cost   = 0;
            direct_power_mod = 0;
            // This proc can miss.
            may_miss = true;
            may_crit = true;
	    reset();
          }
          virtual double total_multiplier() SC_CONST { return 1.0; }
        };

        if ( ! p -> active_chimera_serpent ) p -> active_chimera_serpent = new chimera_serpent_t( p );
        double base_dd = 0.40 * sting_dmg;
        p -> active_chimera_serpent -> base_dd_min = base_dd;
        p -> active_chimera_serpent -> base_dd_max = base_dd;
        p -> active_chimera_serpent -> execute();

      }
      else if ( p -> active_viper_sting )
      {
        p -> resource_gain( RESOURCE_MANA, 0.60 * sting_dmg, p -> gains_chimera_viper );
      }
    }
    consume_improved_steady_shot( this );
    if ( result == RESULT_CRIT )
    {
      trigger_piercing_shots( this );
    }
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( active_sting )
      if ( ! p -> active_sting() )
        return false;

    if ( improved_steady_shot )
      if ( ! p -> _buffs.improved_steady_shot )
        return false;

    return hunter_attack_t::ready();
  }
};

// Explosive Shot ================================================================

struct explosive_tick_t : public hunter_attack_t
{
  explosive_tick_t( player_t* player ) :
      hunter_attack_t( "explosive_shot", player, SCHOOL_FIRE, TREE_SURVIVAL )
  {
    hunter_t* p = player -> cast_hunter();

    static rank_t ranks[] =
    {
      { 80, 4,  386, 464, 0, 0 },
      { 75, 3,  325, 391, 0, 0 },
      { 70, 2,  221, 265, 0, 0 },
      { 60, 1,  160, 192, 0, 0 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    // To trigger ppm-based JoW
    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );
    weapon_multiplier = 0;

    dual       = true;
    background = true;
    may_crit   = true;
    may_miss   = false;

    direct_power_mod = 0.14;

    base_multiplier *= 1.0 + p -> talents.sniper_training * 0.02;
    base_multiplier *= p -> ranged_weapon_specialization_multiplier();

    base_multiplier *= 1.0 + p -> talents.tnt * 0.02;

    base_crit += p -> talents.survival_instincts * 0.02;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.mortal_shots * 0.06;

    if ( p -> glyphs.explosive_shot )
    {
      base_crit += 0.04;
    }
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    tick_dmg = direct_dmg;
    update_stats( DMG_OVER_TIME );
    if ( result == RESULT_CRIT )
    {
      trigger_hunting_party( this );
    }
  }
};

struct explosive_shot_t : public hunter_attack_t
{
  attack_t* explosive_tick;
  int lock_and_load;

  explosive_shot_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "explosive_shot", player, SCHOOL_FIRE, TREE_SURVIVAL ),
      lock_and_load( 0 )
  {
    hunter_t* p = player -> cast_hunter();

    check_talent( p -> talents.explosive_shot );

    option_t options[] =
    {
      { "lock_and_load", OPT_BOOL, &lock_and_load },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost = 0.07 * p -> resource_base[ RESOURCE_MANA ];

    cooldown = 6;
    cooldown_group = "arcane_explosive";

    tick_zero      = true;
    num_ticks      = 2;
    base_tick_time = 1.0;

    explosive_tick = new explosive_tick_t( p );
  }

  virtual double cost() SC_CONST
  {
    hunter_t* p = player -> cast_hunter();
    if ( p -> _buffs.lock_and_load ) return 0;
    return hunter_attack_t::cost();
  }

  virtual void tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
    explosive_tick -> execute();
    update_time( DMG_OVER_TIME );
  }

  virtual void update_ready()
  {
    hunter_t* p = player -> cast_hunter();
    cooldown = p -> _buffs.lock_and_load ? 0.0 : 6.0;
    hunter_attack_t::update_ready();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( lock_and_load )
      if ( ! p -> _buffs.lock_and_load )
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
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    parse_options( 0, options_str );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    may_crit               = true;
    normalize_weapon_speed = true;
    weapon_multiplier      = 2.0;
    direct_power_mod       = 0.40;
    cooldown               = 15;

    base_multiplier *= p -> ranged_weapon_specialization_multiplier();

    base_crit += p -> talents.sniper_training * 0.05;

    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.mortal_shots     * 0.06 +
                                          p -> talents.marked_for_death * 0.02 );

    add_ammunition();
    add_scope();

    if ( p -> glyphs.kill_shot )
    {
      cooldown -= 6;
    }
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    if ( result == RESULT_CRIT )
    {
      trigger_cobra_strikes( this );
    }
  }

  virtual bool ready()
  {
    if ( sim -> target -> health_percentage() > 20 )
      return false;

    return hunter_attack_t::ready();
  }
};

// Multi Shot Attack =========================================================

struct multi_shot_t : public hunter_attack_t
{
  multi_shot_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "multi_shot", player, SCHOOL_PHYSICAL, TREE_MARKSMANSHIP )
  {
    hunter_t* p = player -> cast_hunter();
    assert( p -> ranged_weapon.type != WEAPON_NONE );

    static rank_t ranks[] =
    {
      { 80, 8, 408, 408, 0, 0.09 },
      { 74, 7, 333, 333, 0, 0.09 },
      { 67, 6, 205, 205, 0, 0.09 },
      { 60, 5, 150, 150, 0, 0.13 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    parse_options( 0, options_str );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    may_crit               = true;
    normalize_weapon_speed = true;
    cooldown               = 10;
    cooldown_group         = "aimed_multi";

    base_multiplier *= 1.0 + p -> talents.barrage                      * 0.04;
    base_multiplier *= p -> ranged_weapon_specialization_multiplier();

    base_crit += p -> talents.improved_barrage * 0.04;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.mortal_shots * 0.06;

    add_ammunition();
    add_scope();
  }
};

// Scatter Shot Attack =========================================================

struct scatter_shot_t : public hunter_attack_t
{
  scatter_shot_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "scatter_shot", player, SCHOOL_PHYSICAL, TREE_SURVIVAL )
  {
    hunter_t* p = player -> cast_hunter();

    check_talent( p -> talents.scatter_shot );

    may_crit    = true;
    base_dd_min = 1;
    base_dd_max = 1;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.08;

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    normalize_weapon_speed = true;
    cooldown               = 30;

    weapon_multiplier *= 0.5;

    base_multiplier *= p -> ranged_weapon_specialization_multiplier();

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
      force( 0 )
  {
    hunter_t* p = player -> cast_hunter();

    option_t options[] =
    {
      { "force", OPT_BOOL, &force },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79, 12, 0, 0, 1210/5.0, 0.09 },
      { 73, 11, 0, 0,  990/5.0, 0.09 },
      { 67, 10, 0, 0,  660/5.0, 0.09 },
      { 60, 9,  0, 0,  555/5.0, 0.13 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    base_tick_time   = 3.0;
    num_ticks        = p -> glyphs.serpent_sting ? 7 : 5;
    tick_power_mod   = 0.2 / 5.0;
    base_multiplier *= 1.0 + ( p -> talents.improved_stings * 0.1 +
                               p -> set_bonus.tier8_2pc() * 0.1   );
    tick_may_crit    = ( p -> set_bonus.tier9_2pc() == 1 );

    observer = &( p -> active_serpent_sting );
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    p -> cancel_sting();
    hunter_attack_t::execute();
    if ( result_is_hit() ) sim -> target -> debuffs.poisoned++;
  }

  virtual void last_tick()
  {
    hunter_attack_t::last_tick();
    sim -> target -> debuffs.poisoned--;
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( ! force )
      if ( p -> active_sting() )
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

    check_talent( p -> talents.silencing_shot );

    base_dd_min = 1;
    base_dd_max = 1;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.06;

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    may_crit               = true;
    normalize_weapon_speed = true;
    cooldown               = 20;

    trigger_gcd = 0;

    weapon_multiplier *= 0.5;

    base_multiplier *= p -> ranged_weapon_specialization_multiplier();

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
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    normalize_weapon_damage = true;
    normalize_weapon_speed  = true;
    weapon_power_mod        = 0;
    direct_power_mod        = 0.1;
    base_execute_time       = 2.0;

    may_crit = true;

    base_cost *= 1.0 - p -> talents.master_marksman * 0.05;

    base_multiplier *= 1.0 + p -> talents.sniper_training              * 0.02;
    base_multiplier *= p -> ranged_weapon_specialization_multiplier();

    base_crit += p -> talents.survival_instincts * 0.02;

    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.mortal_shots     * 0.06 +
                                          p -> talents.marked_for_death * 0.02 );

    add_ammunition();
  }

  void execute()
  {
    hunter_attack_t::execute();
    if ( result_is_hit() )
    {
      trigger_improved_steady_shot( this );
      trigger_tier8_4pc( this ); // FIXME: does it need to hit to proc?

      if ( result == RESULT_CRIT )
      {
        trigger_cobra_strikes( this );
        trigger_hunting_party( this );
        trigger_piercing_shots( this );
      }
    }
  }

  void player_buff()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::player_buff();
    if ( p -> glyphs.steady_shot && p -> active_sting() )
    {
      player_multiplier *= 1.10;
    }
  }
};

// =========================================================================
// Hunter Spells
// =========================================================================

// hunter_spell_t::gcd()

double hunter_spell_t::gcd() SC_CONST
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
      beast_during_bw( 0 ), hawk_always( 0 ), viper_start( 5 ), viper_stop( 25 ),
      hawk_bonus( 0 ), viper_multiplier( 0 )
  {
    hunter_t* p = player -> cast_hunter();

    option_t options[] =
    {
      { "beast_during_bw", OPT_BOOL, &beast_during_bw },
      { "hawk_always",     OPT_BOOL, &hawk_always },
      { "viper_start",     OPT_INT,  &viper_start },
      { "viper_stop",      OPT_INT,  &viper_stop  },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown         = 1.0;
    trigger_gcd      = 0.0;
    harmful          = false;
    hawk_bonus       = util_t::ability_rank( p -> level, 300,80, 230,74, 155,68,  120,0 );
    viper_multiplier = 0.50;

    if ( p -> talents.aspect_mastery )
    {
      hawk_bonus       *= 1.30;
      viper_multiplier += 0.10;
    }
  }

  int choose_aspect()
  {
    hunter_t* p = player -> cast_hunter();

    if ( hawk_always ) return ASPECT_HAWK;

    double mana_pct = p -> resource_current[ RESOURCE_MANA ] * 100.0 / p -> resource_max[ RESOURCE_MANA ];

    if ( p -> active_aspect != ASPECT_VIPER && mana_pct < viper_start )
    {
      return ASPECT_VIPER;
    }
    if ( p -> active_aspect == ASPECT_VIPER && mana_pct <= viper_stop )
    {
      return ASPECT_VIPER;
    }
    if ( beast_during_bw && p -> active_pet )
    {
      hunter_pet_t* pet = ( hunter_pet_t* ) p -> active_pet -> cast_pet();
      if ( pet -> _buffs.bestial_wrath ) return ASPECT_BEAST;
    }
    return ASPECT_HAWK;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();

    int aspect = choose_aspect();

    if ( aspect != p -> active_aspect )
    {
      if ( aspect == ASPECT_HAWK )
      {
        if ( sim -> log ) log_t::output( sim, "%s performs Aspect of the Hawk", p -> name() );
        p -> active_aspect = ASPECT_HAWK;
        p -> _buffs.aspect_of_the_hawk = hawk_bonus;
        p -> _buffs.aspect_of_the_viper = 0;
      }
      else if ( aspect == ASPECT_VIPER )
      {
        if ( sim -> log ) log_t::output( sim, "%s performs Aspect of the Viper", p -> name() );
        p -> active_aspect = ASPECT_VIPER;
        p -> _buffs.aspect_of_the_hawk = 0;
        p -> _buffs.aspect_of_the_viper = viper_multiplier;
      }
      else if ( aspect == ASPECT_BEAST )
      {
        if ( sim -> log ) log_t::output( sim, "%s performs Aspect of the Beast", p -> name() );
        p -> active_aspect = ASPECT_BEAST;
        p -> _buffs.aspect_of_the_hawk = 0;
        p -> _buffs.aspect_of_the_viper = 0;
      }
      else
      {
        p -> active_aspect = ASPECT_NONE;
        p -> _buffs.aspect_of_the_hawk = 0;
        p -> _buffs.aspect_of_the_viper = 0;
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

    check_talent( p -> talents.bestial_wrath );

    base_cost = 0.10 * p -> resource_base[ RESOURCE_MANA ];
    cooldown = ( 120 - p -> glyphs.bestial_wrath * 20 ) * ( 1 - p -> talents.longevity * 0.1 );
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
      {
        hunter_t* p = player -> cast_hunter();
        hunter_pet_t* pet = p -> active_pet;
        name = "Bestial Wrath Expiration";
        if ( p -> talents.beast_within )
          p -> _buffs.beast_within = 1;
        pet -> _buffs.bestial_wrath = 1;
        sim -> add_event( this, 18.0 );
      }
      virtual void execute()
      {
        hunter_t* p = player -> cast_hunter();
        hunter_pet_t* pet = p -> active_pet;
        p -> _buffs.beast_within = 0;
        pet -> _buffs.bestial_wrath = 0;
      }
    };

    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    new ( sim ) expiration_t( sim, player );
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();
    if ( ! p -> active_pet ) return false;
    return hunter_spell_t::ready();
  }
};

// Hunter's Mark =========================================================

struct hunters_mark_t : public hunter_spell_t
{
  double ap_bonus;

  hunters_mark_t( player_t* player, const std::string& options_str ) :
      hunter_spell_t( "hunters_mark", player, SCHOOL_ARCANE, TREE_MARKSMANSHIP ), ap_bonus( 0 )
  {
    hunter_t* p = player -> cast_hunter();

    base_cost = 0.02 * p -> resource_base[ RESOURCE_MANA ];
    base_cost *= 1.0 - p -> talents.improved_hunters_mark / 3.0;

    ap_bonus = util_t::ability_rank( p -> level,  500,76,  110,0 );

    ap_bonus *= 1.0 + p -> talents.improved_hunters_mark * 0.10
                + ( p -> glyphs.hunters_mark ? 0.20 : 0 );
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* p, double duration ) :
          event_t( sim, p )
      {
        name = "Hunter's Mark Expiration";
        sim -> add_event( this, duration );
      }
      virtual void execute()
      {
        target_t* t = sim -> target;
        t -> debuffs.hunters_mark = 0;
        t -> expirations.hunters_mark = 0;
      }
    };

    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    consume_resource();

    target_t* t = sim -> target;
    event_t*& e = t -> expirations.hunters_mark;

    t -> debuffs.hunters_mark = ap_bonus;
    double duration = 300.0;
    if ( e )
    {
      e -> reschedule( duration );
    }
    else
    {
      e = new ( sim ) expiration_t( sim, player, duration );
    }
  }

  virtual bool ready()
  {
    if ( ! hunter_spell_t::ready() )
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.03;
    cooldown  = 60 - 10 * p -> talents.catlike_reflexes;
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    assert( p -> active_pet -> _buffs.kill_command == 0 );
    p -> active_pet -> aura_gain( "Kill Command" );
    p -> active_pet -> _buffs.kill_command = 3;
    consume_resource();
    update_ready();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( ! p -> active_pet )
      return false;
    if ( p -> active_pet -> _buffs.kill_command != 0 )
      return false;

    return hunter_spell_t::ready();
  }
};

// Rapid Fire =========================================================

struct rapid_fire_t : public hunter_spell_t
{
  int viper;

  rapid_fire_t( player_t* player, const std::string& options_str ) :
      hunter_spell_t( "rapid_fire", player, SCHOOL_PHYSICAL, TREE_MARKSMANSHIP ), viper( 0 )
  {
    hunter_t* p = player -> cast_hunter();

    option_t options[] =
    {
      { "viper", OPT_BOOL, &viper },
      { NULL, OPT_UNKNOWN, NULL }
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
        player -> cast_hunter() -> _buffs.rapid_fire = 0;
      }
    };

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> _buffs.rapid_fire = p -> glyphs.rapid_fire ? 0.48 : 0.40;
    consume_resource();
    update_ready();
    new ( sim ) expiration_t( sim, p );
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( p -> _buffs.rapid_fire )
      return false;

    if ( viper )
      if ( p -> active_aspect != ASPECT_VIPER )
        return false;

    return hunter_spell_t::ready();
  }
};

// Readiness ================================================================

struct readiness_t : public hunter_spell_t
{
  bool wait_for_rf;

  readiness_t( player_t* player, const std::string& options_str ) :
      hunter_spell_t( "readiness", player, SCHOOL_PHYSICAL, TREE_MARKSMANSHIP ),
      wait_for_rf( false )
  {
    hunter_t* p = player -> cast_hunter();

    check_talent( p -> talents.readiness );

    option_t options[] =
    {
      // Only perform Readiness while Rapid Fire is up, allows the sequence
      // Rapid Fire, Readiness, Rapid Fire, for better RF uptime
      { "wait_for_rapid_fire", OPT_BOOL, &wait_for_rf },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown = 180;
    trigger_gcd = 1.0;
    harmful = false;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    for ( action_t* a = player -> action_list; a; a = a -> next )
    {
      a -> cooldown_ready = 0;
    }

    update_ready();
  }

  virtual bool ready()
  {
    if ( ! hunter_spell_t::ready() )
      return false;

    hunter_t* p = player -> cast_hunter();
    if ( wait_for_rf && ! p -> _buffs.rapid_fire )
      return false;

    return true;
  }
};

// Summon Pet ===============================================================

struct summon_pet_t : public hunter_spell_t
{
  std::string pet_name;

  summon_pet_t( player_t* player, const std::string& options_str ) :
      hunter_spell_t( "summon_pet", player, SCHOOL_PHYSICAL, TREE_BEAST_MASTERY )
  {
    hunter_t* p = player -> cast_hunter();
    pet_name = ( options_str.size() > 0 ) ? options_str : p -> summon_pet_str;
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
    if ( p -> active_pet ) return false;
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
    check_talent( p -> talents.trueshot_aura );
    trigger_gcd = 0;
    harmful = false;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> _buffs.trueshot_aura = 1;
    sim -> auras.trueshot = 1;
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();
    return( ! p -> _buffs.trueshot_aura );
  }
};

// hunter_t::create_action ====================================================

action_t* hunter_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "auto_shot"             ) return new              auto_shot_t( this, options_str );
  if ( name == "aimed_shot"            ) return new             aimed_shot_t( this, options_str );
  if ( name == "arcane_shot"           ) return new            arcane_shot_t( this, options_str );
  if ( name == "aspect"                ) return new                 aspect_t( this, options_str );
  if ( name == "bestial_wrath"         ) return new          bestial_wrath_t( this, options_str );
  if ( name == "black_arrow"           ) return new            black_arrow_t( this, options_str );
  if ( name == "chimera_shot"          ) return new           chimera_shot_t( this, options_str );
  if ( name == "explosive_shot"        ) return new         explosive_shot_t( this, options_str );
  if ( name == "hunters_mark"          ) return new           hunters_mark_t( this, options_str );
  if ( name == "kill_command"          ) return new           kill_command_t( this, options_str );
  if ( name == "kill_shot"             ) return new              kill_shot_t( this, options_str );
//if ( name == "mongoose_bite"         ) return new          mongoose_bite_t( this, options_str );
  if ( name == "multi_shot"            ) return new             multi_shot_t( this, options_str );
  if ( name == "rapid_fire"            ) return new             rapid_fire_t( this, options_str );
//if ( name == "raptor_strike"         ) return new          raptor_strike_t( this, options_str );
  if ( name == "readiness"             ) return new              readiness_t( this, options_str );
  if ( name == "scatter_shot"          ) return new           scatter_shot_t( this, options_str );
//if ( name == "scorpid_sting"         ) return new          scorpid_sting_t( this, options_str );
  if ( name == "serpent_sting"         ) return new          serpent_sting_t( this, options_str );
  if ( name == "silencing_shot"        ) return new         silencing_shot_t( this, options_str );
  if ( name == "steady_shot"           ) return new            steady_shot_t( this, options_str );
  if ( name == "summon_pet"            ) return new             summon_pet_t( this, options_str );
  if ( name == "trueshot_aura"         ) return new          trueshot_aura_t( this, options_str );
//if( name == "viper_sting"           ) return new            viper_sting_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// hunter_t::create_pet =======================================================

pet_t* hunter_t::create_pet( const std::string& pet_name )
{
  // Ferocity
  if ( pet_name == "carrion_bird" ) return new hunter_pet_t( sim, this, pet_name, PET_CARRION_BIRD );
  if ( pet_name == "cat"          ) return new hunter_pet_t( sim, this, pet_name, PET_CAT          );
  if ( pet_name == "core_hound"   ) return new hunter_pet_t( sim, this, pet_name, PET_CORE_HOUND   );
  if ( pet_name == "devilsaur"    ) return new hunter_pet_t( sim, this, pet_name, PET_DEVILSAUR    );
  if ( pet_name == "hyena"        ) return new hunter_pet_t( sim, this, pet_name, PET_HYENA        );
  if ( pet_name == "moth"         ) return new hunter_pet_t( sim, this, pet_name, PET_MOTH         );
  if ( pet_name == "raptor"       ) return new hunter_pet_t( sim, this, pet_name, PET_RAPTOR       );
  if ( pet_name == "spirit_beast" ) return new hunter_pet_t( sim, this, pet_name, PET_SPIRIT_BEAST );
  if ( pet_name == "tallstrider"  ) return new hunter_pet_t( sim, this, pet_name, PET_TALLSTRIDER  );
  if ( pet_name == "wasp"         ) return new hunter_pet_t( sim, this, pet_name, PET_WASP         );
  if ( pet_name == "wolf"         ) return new hunter_pet_t( sim, this, pet_name, PET_WOLF         );

  // Tenacity
  if ( pet_name == "bear"         ) return new hunter_pet_t( sim, this, pet_name, PET_BEAR         );
  if ( pet_name == "boar"         ) return new hunter_pet_t( sim, this, pet_name, PET_BOAR         );
  if ( pet_name == "crab"         ) return new hunter_pet_t( sim, this, pet_name, PET_CRAB         );
  if ( pet_name == "crocolisk"    ) return new hunter_pet_t( sim, this, pet_name, PET_CROCOLISK    );
  if ( pet_name == "gorilla"      ) return new hunter_pet_t( sim, this, pet_name, PET_GORILLA      );
  if ( pet_name == "rhino"        ) return new hunter_pet_t( sim, this, pet_name, PET_RHINO        );
  if ( pet_name == "scorpid"      ) return new hunter_pet_t( sim, this, pet_name, PET_SCORPID      );
  if ( pet_name == "turtle"       ) return new hunter_pet_t( sim, this, pet_name, PET_TURTLE       );
  if ( pet_name == "warp_stalker" ) return new hunter_pet_t( sim, this, pet_name, PET_WARP_STALKER );
  if ( pet_name == "worm"         ) return new hunter_pet_t( sim, this, pet_name, PET_WORM         );

  // Cunning
  if ( pet_name == "bat"          ) return new hunter_pet_t( sim, this, pet_name, PET_BAT          );
  if ( pet_name == "bird_of_prey" ) return new hunter_pet_t( sim, this, pet_name, PET_BIRD_OF_PREY );
  if ( pet_name == "chimera"      ) return new hunter_pet_t( sim, this, pet_name, PET_CHIMERA      );
  if ( pet_name == "dragonhawk"   ) return new hunter_pet_t( sim, this, pet_name, PET_DRAGONHAWK   );
  if ( pet_name == "nether_ray"   ) return new hunter_pet_t( sim, this, pet_name, PET_NETHER_RAY   );
  if ( pet_name == "ravager"      ) return new hunter_pet_t( sim, this, pet_name, PET_RAVAGER      );
  if ( pet_name == "serpent"      ) return new hunter_pet_t( sim, this, pet_name, PET_SERPENT      );
  if ( pet_name == "silithid"     ) return new hunter_pet_t( sim, this, pet_name, PET_SILITHID     );
  if ( pet_name == "spider"       ) return new hunter_pet_t( sim, this, pet_name, PET_SPIDER       );
  if ( pet_name == "sporebat"     ) return new hunter_pet_t( sim, this, pet_name, PET_SPOREBAT     );
  if ( pet_name == "wind_serpent" ) return new hunter_pet_t( sim, this, pet_name, PET_WIND_SERPENT );

  return 0;
}

// hunter_t::armory ===========================================================

struct ammo_data
{
  int id;
  double dps;
};

void hunter_t::armory( xml_node_t* sheet_xml, xml_node_t* talents_xml )
{
  // Ammo support
  static ammo_data ammo[] =
  {
    { 41164, 67.5 }, { 41165, 67.5 },
    { 30319, 63.5 },
    { 32760, 53.0 }, { 32761, 53.0 }, { 31737, 53.0 }, { 31735, 53.0 },
    { 34581, 46.5 }, { 34582, 46.5 }, { 41584, 46.5 }, { 41586, 46.5 },
    { 23773, 43.0 }, { 33803, 43.0 },
    { 32883, 37.0 }, { 32882, 37.0 }, { 31949, 37.0 },
    { 30612, 34.0 }, { 30611, 34.0 },
    { 28056, 32.0 }, { 28061, 32.0 },
    { 28060, 22.0 }, { 28053, 22.0 },
    { 28060, 22.0 }, { 28053, 22.0 },
    { 0, 0 }
  };
  std::vector<xml_node_t*> item_nodes;
  int num_items = xml_t::get_nodes( item_nodes, sheet_xml, "item" );
  for ( int i=0; i < num_items; i++ )
  {
    int slot;
    int id;

    if ( xml_t::get_value( slot, item_nodes[ i ], "slot" ) &&
         xml_t::get_value( id,   item_nodes[ i ], "id"   ) &&
         slot == -1 )
    {
      for ( int k=0; ammo[k].id > 0; k++ )
      {
        if ( id == ammo[k].id )
        {
          if ( sim -> debug )
            log_t::output( sim, "Setting ammo dps to %.1f (from item id %d)", ammo[k].dps, id );
          ammo_dps = ammo[k].dps;
          break;
        }
      }
    }
  }

  // Pet support
  static pet_type_t pet_types[] =
    { PET_NONE, PET_WOLF, PET_CAT, PET_SPIDER, PET_BEAR,
      /* 5*/ PET_BOAR, PET_CROCOLISK, PET_CARRION_BIRD, PET_CRAB, PET_GORILLA,
      /*10*/ PET_NONE, PET_RAPTOR, PET_TALLSTRIDER, PET_NONE, PET_NONE,
      /*15*/ PET_NONE, PET_NONE, PET_NONE, PET_NONE, PET_NONE,
      /*20*/ PET_SCORPID, PET_TURTLE, PET_NONE, PET_NONE, PET_BAT,
      /*25*/ PET_HYENA, PET_BIRD_OF_PREY, PET_WIND_SERPENT, PET_NONE, PET_NONE,
      /*30*/ PET_DRAGONHAWK, PET_RAVAGER, PET_WARP_STALKER, PET_SPOREBAT, PET_NETHER_RAY,
      /*35*/ PET_SERPENT, PET_NONE, PET_MOTH, PET_CHIMERA, PET_DEVILSAUR,
      /*40*/ PET_NONE, PET_SILITHID, PET_WORM, PET_RHINO, PET_WASP,
      /*45*/ PET_CORE_HOUND, PET_SPIRIT_BEAST
    };

  std::vector<xml_node_t*> pet_nodes;
  int num_pets = xml_t::get_nodes( pet_nodes, talents_xml, "pet" );
  for ( int i=0; i < num_pets; i++ )
  {
    std::string name_str;
    int family_id;

    if ( xml_t::get_value( name_str,  pet_nodes[ i ], "name"     ) &&
         xml_t::get_value( family_id, pet_nodes[ i ], "familyId" ) )
    {
      if ( family_id < 0 || family_id > 46 )
        continue;
      if ( ! hunter_pet_t::supported( pet_types[ family_id ] ) )
        continue;
      hunter_pet_t* pet = new hunter_pet_t( sim, this, name_str, pet_types[ family_id ] );
      std::string talent_str;
      if ( xml_t::get_value( talent_str, pet_nodes[ i ], "talentSpec/value" ) &&
           talent_str != "" )
      {
        pet -> parse_talents_armory( talent_str );
      }
    }
  }
}

// hunter_t::init =============================================================

void hunter_t::init()
{
  player_t::init();

  if ( ! find_pet( summon_pet_str ) )
  {
    create_pet( "cat" );
    create_pet( "devilsaur" );
    create_pet( "raptor" );
    create_pet( "wind_serpent" );
    create_pet( "wolf" );
  }
}

// hunter_t::init_glyphs ===================================================

void hunter_t::init_glyphs()
{
  memset( ( void* ) &glyphs, 0x0, sizeof( glyphs_t ) );

  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );

  for ( int i=0; i < num_glyphs; i++ )
  {
    std::string& n = glyph_names[ i ];

    if      ( n == "aimed_shot"                  ) glyphs.aimed_shot = 1;
    else if ( n == "aspect_of_the_viper"         ) glyphs.aspect_of_the_viper = 1;
    else if ( n == "bestial_wrath"               ) glyphs.bestial_wrath = 1;
    else if ( n == "chimera_shot"                ) glyphs.chimera_shot = 1;
    else if ( n == "explosive_shot"              ) glyphs.explosive_shot = 1;
    else if ( n == "hunters_mark"                ) glyphs.hunters_mark = 1;
    else if ( n == "improved_aspect_of_the_hawk" ) glyphs.improved_aspect_of_the_hawk = 1;
    else if ( n == "kill_shot"                   ) glyphs.kill_shot = 1;
    else if ( n == "rapid_fire"                  ) glyphs.rapid_fire = 1;
    else if ( n == "serpent_sting"               ) glyphs.serpent_sting = 1;
    else if ( n == "steady_shot"                 ) glyphs.steady_shot = 1;
    else if ( n == "the_hawk"                    ) glyphs.improved_aspect_of_the_hawk = 1;
    else if ( n == "trueshot_aura"               ) glyphs.trueshot_aura = 1;
    // To prevent warnings....
    else if ( n == "the_pack"           ) ;
    else if ( n == "mend_pet"           ) ;
    else if ( n == "mending"            ) ;
    else if ( n == "revive_pet"         ) ;
    else if ( n == "explosive_trap"     ) ;
    else if ( n == "feign_death"        ) ;
    else if ( n == "scare_beast"        ) ;
    else if ( n == "possessed_strength" ) ;
    else if ( n == "disengage"          ) ;
    else if ( n == "arcane_shot"        ) ;
    else if ( n == "frost_trap"         ) ;
    else if ( ! sim -> parent ) util_t::printf( "simcraft: Player %s has unrecognized glyph %s\n", name(), n.c_str() );
  }
}

// hunter_t::init_race ======================================================

void hunter_t::init_race()
{
  race = util_t::parse_race_type( race_str );
  switch ( race )
  {
  case RACE_DWARF:
  case RACE_DRAENEI:
  case RACE_NIGHT_ELF:
  case RACE_ORC:
  case RACE_TROLL:
  case RACE_TAUREN:
  case RACE_BLOOD_ELF:
    break;
  default:
    race = RACE_NIGHT_ELF;
    race_str = util_t::race_type_string( race );
  }

  player_t::init_race();
}

// hunter_t::init_base ========================================================

void hunter_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] = rating_t::get_attribute_base( level, HUNTER, race, BASE_STAT_STRENGTH );
  attribute_base[ ATTR_AGILITY   ] = rating_t::get_attribute_base( level, HUNTER, race, BASE_STAT_AGILITY );
  attribute_base[ ATTR_STAMINA   ] = rating_t::get_attribute_base( level, HUNTER, race, BASE_STAT_STAMINA );
  attribute_base[ ATTR_INTELLECT ] = rating_t::get_attribute_base( level, HUNTER, race, BASE_STAT_INTELLECT );
  attribute_base[ ATTR_SPIRIT    ] = rating_t::get_attribute_base( level, HUNTER, race, BASE_STAT_SPIRIT );
  resource_base[ RESOURCE_HEALTH ] = rating_t::get_attribute_base( level, HUNTER, race, BASE_STAT_HEALTH );
  resource_base[ RESOURCE_MANA   ] = rating_t::get_attribute_base( level, HUNTER, race, BASE_STAT_MANA );
  base_spell_crit                  = rating_t::get_attribute_base( level, HUNTER, race, BASE_STAT_SPELL_CRIT );
  base_attack_crit                 = rating_t::get_attribute_base( level, HUNTER, race, BASE_STAT_MELEE_CRIT );
  initial_spell_crit_per_intellect = rating_t::get_attribute_base( level, HUNTER, race, BASE_STAT_SPELL_CRIT_PER_INT );
  initial_attack_crit_per_agility  = rating_t::get_attribute_base( level, HUNTER, race, BASE_STAT_MELEE_CRIT_PER_AGI );

  attribute_multiplier_initial[ ATTR_INTELLECT ] *= 1.0 + talents.combat_experience * 0.02;
  attribute_multiplier_initial[ ATTR_AGILITY ]   *= 1.0 + talents.combat_experience * 0.02;
  attribute_multiplier_initial[ ATTR_AGILITY ]   *= 1.0 + talents.hunting_party * 0.01;
  attribute_multiplier_initial[ ATTR_AGILITY ]   *= 1.0 + talents.lightning_reflexes * 0.03;
  attribute_multiplier_initial[ ATTR_STAMINA ]   *= 1.0 + talents.survivalist * 0.02;

  base_attack_power = level * 2 - 10;

  initial_attack_power_per_strength = 0.0;
  initial_attack_power_per_agility  = 1.0;

  health_per_stamina = 10;
  mana_per_intellect = 15;

  position = POSITION_RANGED;
}

// hunter_t::init_gains ======================================================

void hunter_t::init_gains()
{
  player_t::init_gains();
  gains_chimera_viper        = get_gain( "chimera_viper" );
  gains_invigoration         = get_gain( "invigoration" );
  gains_rapid_recuperation   = get_gain( "rapid_recuperation" );
  gains_roar_of_recovery     = get_gain( "roar_of_recovery" );
  gains_thrill_of_the_hunt   = get_gain( "thrill_of_the_hunt" );
  gains_viper_aspect_passive = get_gain( "viper_aspect_passive" );
  gains_viper_aspect_shot    = get_gain( "viper_aspect_shot" );
}

// hunter_t::init_procs ======================================================

void hunter_t::init_procs()
{
  player_t::init_procs();
  procs_wild_quiver   = get_proc( "wild_quiver",   sim );
  procs_lock_and_load = get_proc( "lock_and_load", sim );
}

// hunter_t::init_uptimes ====================================================

void hunter_t::init_uptimes()
{
  player_t::init_uptimes();
  uptimes_aspect_of_the_viper         = get_uptime( "aspect_of_the_viper" );
  uptimes_expose_weakness             = get_uptime( "expose_weakness" );
  uptimes_furious_howl                = get_uptime( "furious_howl" );
  uptimes_improved_aspect_of_the_hawk = get_uptime( "improved_aspect_of_the_hawk" );
  uptimes_improved_steady_shot        = get_uptime( "improved_steady_shot" );
  uptimes_master_tactician            = get_uptime( "master_tactician" );
  uptimes_rapid_fire                  = get_uptime( "rapid_fire" );
}

// hunter_t::init_rng ========================================================

void hunter_t::init_rng()
{
  player_t::init_rng();

  rng_cobra_strikes        = get_rng( "cobra_strikes"      );
  rng_invigoration         = get_rng( "invigoration"       );
  rng_owls_focus           = get_rng( "owls_focus"         );
  rng_thrill_of_the_hunt   = get_rng( "thrill_of_the_hunt" );
  rng_wild_quiver          = get_rng( "wild_quiver"        );

  // Overlapping procs require the use of a "distributed" RNG-stream when normalized_roll=1
  // also useful for frequent checks with low probability of proc and timed effect

  rng_expose_weakness      = get_rng( "expose_weakness",             RNG_DISTRIBUTED );
  rng_frenzy               = get_rng( "frenzy",                      RNG_DISTRIBUTED );
  rng_hunting_party        = get_rng( "hunting_party",               RNG_DISTRIBUTED );
  rng_improved_aoth        = get_rng( "improved_aspect_of_the_hawk", RNG_DISTRIBUTED );
  rng_improved_steady_shot = get_rng( "improved_steady_shot",        RNG_DISTRIBUTED );
  rng_lock_and_load        = get_rng( "lock_and_load",               RNG_DISTRIBUTED );
  rng_master_tactician     = get_rng( "master_tactician",            RNG_DISTRIBUTED );
  rng_rabid_power          = get_rng( "rabid_power",                 RNG_DISTRIBUTED );
}

// hunter_t::init_scaling ====================================================

void hunter_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_STRENGTH  ] = 0;
  scales_with[ STAT_INTELLECT ] = 1;

  if ( talents.hunter_vs_wild ) scales_with[ STAT_STAMINA ] = 1;

  scales_with[ STAT_EXPERTISE_RATING ] = 0;
}

// hunter_t::init_actions ====================================================

void hunter_t::init_actions()
{
  if ( action_list_str.empty() )
  {
    action_list_str = "flask,type=endless_rage/food,type=blackened_dragonfin/hunters_mark/summon_pet";
    if ( talents.trueshot_aura ) action_list_str += "/trueshot_aura";
    action_list_str += "/auto_shot";
    int num_items = items.size();
    for ( int i=0; i < num_items; i++ )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
      }
    }
    if ( talents.bestial_wrath ) action_list_str += "/kill_command,sync=bestial_wrath/bestial_wrath";
    action_list_str += "/aspect";
    if ( talents.chimera_shot ) action_list_str += "/serpent_sting";
    action_list_str += "/rapid_fire";
    if ( primary_tree() != TREE_MARKSMANSHIP ) action_list_str += "/kill_shot";
    if ( ! talents.bestial_wrath  ) action_list_str += "/kill_command";
    if (   talents.silencing_shot ) action_list_str += "/silencing_shot";
    if (   talents.chimera_shot   ) action_list_str += "/chimera_shot";
    if ( primary_tree() == TREE_MARKSMANSHIP ) action_list_str += "/kill_shot";
    if (   talents.explosive_shot ) action_list_str += "/explosive_shot";
    if (   talents.black_arrow    ) action_list_str += "/black_arrow";
    if ( ! talents.chimera_shot   ) action_list_str += "/serpent_sting";
    if ( primary_tree() == TREE_MARKSMANSHIP )
    {
      if( talents.improved_arcane_shot ) action_list_str += "/arcane_shot";
      if( talents.aimed_shot           ) action_list_str += "/aimed_shot";
      if( talents.readiness            ) action_list_str += "/readiness,wait_for_rapid_fire=1";
    }
    else
    {
      if ( ! talents.explosive_shot ) action_list_str += "/arcane_shot";
      if (   talents.readiness      ) action_list_str += "/readiness,wait_for_rapid_fire=1";
      if (   talents.aimed_shot     ) action_list_str += "/aimed_shot";
    }
    if ( ! talents.aimed_shot     ) action_list_str += "/multi_shot";
    action_list_str += "/steady_shot";

    action_list_default = 1;
  }

  player_t::init_actions();
}

// hunter_t::primary_tree() ==================================================

int hunter_t::primary_tree() SC_CONST
{
  if ( talents.serpents_swiftness || talents.beast_within ) return TREE_BEAST_MASTERY;
  if ( talents.master_marksman ) return TREE_MARKSMANSHIP;
  if ( talents.black_arrow ) return TREE_SURVIVAL;

  return TALENT_TREE_MAX;
}

// hunter_t::reset ===========================================================

void hunter_t::reset()
{
  player_t::reset();

  // Active
  active_pet            = 0;
  active_aspect         = ASPECT_NONE;
  active_black_arrow    = 0;
  active_scorpid_sting  = 0;
  active_serpent_sting  = 0;
  active_viper_sting    = 0;

  _buffs.reset();
  _cooldowns.reset();
  _expirations.reset();
}

// hunter_t::interrupt =======================================================

void hunter_t::interrupt()
{
  player_t::interrupt();

  if ( ranged_attack ) ranged_attack -> cancel();
}

// hunter_t::composite_attack_power ==========================================

double hunter_t::composite_attack_power() SC_CONST
{
  double ap = player_t::composite_attack_power();

  ap += _buffs.aspect_of_the_hawk;
  ap += intellect() * talents.careful_aim / 3.0;
  ap += stamina() * talents.hunter_vs_wild * 0.1;

  ap += _buffs.furious_howl;
  uptimes_furious_howl -> update( _buffs.furious_howl != 0 );

  if ( _buffs.expose_weakness ) ap += agility() * 0.25;
  uptimes_expose_weakness -> update( _buffs.expose_weakness != 0 );

  if ( _buffs.call_of_the_wild )
    ap *= 1.1;

  return ap;
}

// hunter_t::regen  =======================================================

void hunter_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  if ( _buffs.aspect_of_the_viper )
  {
    double aspect_of_the_viper_regen = periodicity * 0.04 * resource_max[ RESOURCE_MANA ] / 3.0;

    resource_gain( RESOURCE_MANA, aspect_of_the_viper_regen, gains_viper_aspect_passive );
  }
  if ( _buffs.rapid_fire && talents.rapid_recuperation )
  {
    double rr_regen = periodicity * 0.02 * talents.rapid_recuperation * resource_max[ RESOURCE_MANA ] / 3.0;

    resource_gain( RESOURCE_MANA, rr_regen, gains_rapid_recuperation );
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
    { {  6, NULL                                     }, {  6, &( talents.mortal_shots                 ) }, {  6, &( talents.trap_mastery             ) } },
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
    { { 19, &( talents.catlike_reflexes            ) }, { 19, &( talents.trueshot_aura                ) }, { 19, &( talents.expose_weakness          ) } },
    { { 20, &( talents.invigoration                ) }, { 20, &( talents.improved_barrage             ) }, { 20, NULL                                  } },
    { { 21, &( talents.serpents_swiftness          ) }, { 21, &( talents.master_marksman              ) }, { 21, &( talents.thrill_of_the_hunt       ) } },
    { { 22, &( talents.longevity                   ) }, { 22, &( talents.rapid_recuperation           ) }, { 22, &( talents.master_tactician         ) } },
    { { 23, &( talents.beast_within                ) }, { 23, &( talents.wild_quiver                  ) }, { 23, &( talents.noxious_stings           ) } },
    { { 24, &( talents.cobra_strikes               ) }, { 24, &( talents.silencing_shot               ) }, { 24, NULL                                  } },
    { { 25, &( talents.kindred_spirits             ) }, { 25, &( talents.improved_steady_shot         ) }, { 25, &( talents.black_arrow              ) } },
    { { 26, &( talents.beast_mastery               ) }, { 26, &( talents.marked_for_death             ) }, { 26, &( talents.sniper_training          ) } },
    { {  0, NULL                                     }, { 27, &( talents.chimera_shot                 ) }, { 27, &( talents.hunting_party            ) } },
    { {  0, NULL                                     }, {  0, NULL                                      }, { 28, &( talents.explosive_shot           ) } },
    { {  0, NULL                                     }, {  0, NULL                                      }, {  0, NULL                                  } }
  };

  return get_talent_translation( beastmastery, marksmanship, survival, translation );
}


// hunter_t::get_options ====================================================

std::vector<option_t>& hunter_t::get_options()
{
  if ( options.empty() )
  {
    player_t::get_options();

    option_t hunter_options[] =
    {
      // @option_doc loc=player/hunter/talents title="Talents"
      { "aimed_shot",                        OPT_INT, &( talents.aimed_shot                   ) },
      { "animal_handler",                    OPT_INT, &( talents.animal_handler               ) },
      { "aspect_mastery",                    OPT_INT, &( talents.aspect_mastery               ) },
      { "barrage",                           OPT_INT, &( talents.barrage                      ) },
      { "beast_mastery",                     OPT_INT, &( talents.beast_mastery                ) },
      { "beast_within",                      OPT_INT, &( talents.beast_within                 ) },
      { "bestial_discipline",                OPT_INT, &( talents.bestial_discipline           ) },
      { "bestial_wrath",                     OPT_INT, &( talents.bestial_wrath                ) },
      { "careful_aim",                       OPT_INT, &( talents.careful_aim                  ) },
      { "catlike_reflexes",                  OPT_INT, &( talents.catlike_reflexes             ) },
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
      // @option_doc loc=player/hunter/misc title="Misc"
      { "ammo_dps",                          OPT_FLT,    &( ammo_dps                          ) },
      { "quiver_haste",                      OPT_DEPRECATED, NULL                               },
      { "summon_pet",                        OPT_STRING, &( summon_pet_str                    ) },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, hunter_options );
  }

  return options;
}

// hunter_t::save ===========================================================

bool hunter_t::save( FILE* file, int save_type )
{
  player_t::save( file, save_type );

  if ( save_type == SAVE_ALL || save_type == SAVE_GEAR )
  {
    if ( ammo_dps != 0 ) util_t::fprintf( file, "ammo_dps=%.2f\n", ammo_dps );
  }

  return true;
}

// hunter_t::decode_set =====================================================

int hunter_t::decode_set( item_t& item )
{
  if ( strstr( item.name(), "cryptstalker"   ) ) return SET_T7;
  if ( strstr( item.name(), "scourgestalker" ) ) return SET_T8;
  if ( strstr( item.name(), "windrunner"     ) ) return SET_T9;
  // Horde T9 name?
  return SET_NONE;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_hunter  =================================================

player_t* player_t::create_hunter( sim_t* sim, const std::string& name, int race_type )
{
  return new hunter_t( sim, name, race_type );
}

