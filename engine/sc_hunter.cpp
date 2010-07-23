// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Hunter
// ==========================================================================

#define PET_MAX_TALENT_POINTS 20
#define PET_TALENT_ROW_MULTIPLIER 3
#define PET_MAX_TALENT_ROW 6
#define PET_MAX_TALENT_TREES 1
#define PET_MAX_TALENT_COL 4
#define PET_MAX_TALENT_SLOTS (PET_MAX_TALENT_TREES*PET_MAX_TALENT_ROW*PET_MAX_TALENT_COL)

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
  buff_t* buffs_aspect_of_the_hawk;
  buff_t* buffs_aspect_of_the_viper;
  buff_t* buffs_beast_within;
  buff_t* buffs_call_of_the_wild;
  buff_t* buffs_cobra_strikes;
  buff_t* buffs_culling_the_herd;
  buff_t* buffs_expose_weakness;
  buff_t* buffs_furious_howl;
  buff_t* buffs_improved_aspect_of_the_hawk;
  buff_t* buffs_improved_steady_shot;
  buff_t* buffs_lock_and_load;
  buff_t* buffs_master_tactician;
  buff_t* buffs_rapid_fire;
  buff_t* buffs_trueshot_aura;
  buff_t* buffs_tier8_4pc;
  buff_t* buffs_tier10_2pc;
  buff_t* buffs_tier10_4pc;

  // Gains
  gain_t* gains_chimera_viper;
  gain_t* gains_glyph_of_arcane_shot;
  gain_t* gains_invigoration;
  gain_t* gains_rapid_recuperation;
  gain_t* gains_roar_of_recovery;
  gain_t* gains_thrill_of_the_hunt;
  gain_t* gains_viper_aspect_passive;
  gain_t* gains_viper_aspect_shot;

  // Procs
  proc_t* procs_wild_quiver;

  // Random Number Generation
  rng_t* rng_frenzy;
  rng_t* rng_hunting_party;
  rng_t* rng_invigoration;
  rng_t* rng_owls_focus;
  rng_t* rng_rabid_power;
  rng_t* rng_thrill_of_the_hunt;
  rng_t* rng_wild_quiver;

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
    int  endurance_training;
    int  savage_strikes;
    int  survival_tactics;
    int  thick_hide;
    int  trap_mastery;

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int  aimed_shot;
    int  arcane_shot;
    int  aspect_of_the_viper;
    int  bestial_wrath;
    int  chimera_shot;
    int  explosive_shot;
    int  hunters_mark;
    int  kill_shot;
    int  multi_shot;
    int  rapid_fire;
    int  serpent_sting;
    int  steady_shot;
    int  the_beast;
    int  the_hawk;
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
	ammo_dps = 91.5;
    quiver_haste = 1.15;
    summon_pet_str = "wolf";
  }

  // Character Definition
  virtual void      init_glyphs();
  virtual void      init_race();
  virtual void      init_base();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_uptimes();
  virtual void      init_rng();
  virtual void      init_scaling();
  virtual void      init_unique_gear();
  virtual void      init_actions();
  virtual void      combat_begin();
  virtual void      reset();
  virtual void      interrupt();
  virtual double    composite_attack_power() SC_CONST;
  virtual double    composite_attack_power_multiplier() SC_CONST;
  virtual double    composite_attack_hit() SC_CONST;
  virtual double    composite_attack_crit() SC_CONST;
  virtual std::vector<talent_translation_t>& get_talent_list();
  virtual std::vector<option_t>& get_options();
  virtual cooldown_t* get_cooldown( const std::string& name );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet( const std::string& name );
  virtual void      create_pets();
  virtual void      armory( xml_node_t* sheet_xml, xml_node_t* talents_xml );
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_MANA; }
  virtual int       primary_role() SC_CONST     { return ROLE_ATTACK; }
  virtual int       primary_tree() SC_CONST;
  virtual bool      create_profile( std::string& profile_str, int save_type=SAVE_ALL );

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
  buff_t* buffs_bestial_wrath;
  buff_t* buffs_call_of_the_wild;
  buff_t* buffs_culling_the_herd;
  buff_t* buffs_frenzy;
  buff_t* buffs_furious_howl;
  buff_t* buffs_kill_command;
  buff_t* buffs_monstrous_bite;
  buff_t* buffs_owls_focus;
  buff_t* buffs_rabid;
  buff_t* buffs_rabid_power_stack;
  buff_t* buffs_savage_rend;
  buff_t* buffs_wolverine_bite;
  buff_t* buffs_tier9_4pc;

  // Gains
  gain_t* gains_go_for_the_throat;

  struct talents_t
  {
    int call_of_the_wild;
    int cobra_reflexes;
    int culling_the_herd;
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
      pet_t( sim, owner, pet_name ), pet_type( pt )
  {
    if ( ! supported( pet_type ) )
    {
      sim -> errorf( "Pet %s is not yet supported.\n", pet_name.c_str() );
      sim -> cancel();
    }

    hunter_t* o = owner -> cast_hunter();

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 51.0; // FIXME only level 80 value
    main_hand_weapon.max_dmg    = 78.0; // FIXME only level 80 value
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2; // FIXME only level 80 value
    main_hand_weapon.swing_time = 2.0;

    stamina_per_owner = 0.45;

    health_per_stamina *= 1.05; // 3.1.0 change # Cunning, Ferocity and Tenacity pets now all have +5% damage, +5% armor and +5% health bonuses

    if ( group() == PET_FEROCITY )
    {
      talents.call_of_the_wild = 1;
      talents.cobra_reflexes   = 2;
	    talents.culling_the_herd = 3;
      talents.rabid            = 1;
      talents.spiders_bite     = 3;
      talents.spiked_collar    = 3;

      talents.shark_attack = ( o -> talents.beast_mastery ) ? 2 : 0;
      talents.wild_hunt    = ( o -> talents.beast_mastery ) ? 2 : 1;
    }
    else if ( group() == PET_CUNNING )
    {
      talents.cobra_reflexes   = 2;
	    talents.culling_the_herd = 3;
      talents.feeding_frenzy   = 2;
      talents.owls_focus       = 2;
      talents.roar_of_recovery = 1;
      talents.spiked_collar    = 3;
      talents.wolverine_bite   = 1;

      talents.wild_hunt = ( o -> talents.beast_mastery ) ? 2 : 1;
    }
    else // TENACITY
    {
      assert( 0 );
    }

    init_actions();
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

  }

  virtual int group()
  {
    if ( pet_type < PET_FEROCITY ) return PET_FEROCITY;
    if ( pet_type < PET_TENACITY ) return PET_TENACITY;
    if ( pet_type < PET_CUNNING  ) return PET_CUNNING;
    return PET_NONE;
  }

  virtual bool ooc_buffs() { return true; }

  virtual void init_actions()
  {
    if ( group() == PET_FEROCITY )
    {
      if ( pet_type == PET_CAT )
      {
        action_list_str = "auto_attack";
        if ( talents.call_of_the_wild ) action_list_str += "/call_of_the_wild";
        if ( talents.rabid ) action_list_str += "/rabid";
        action_list_str += "/rake/claw";

      }
      else if ( pet_type == PET_DEVILSAUR )
      {
        action_list_str = "auto_attack/monstrous_bite";
        if ( talents.call_of_the_wild ) action_list_str += "/call_of_the_wild";
        if ( talents.rabid ) action_list_str += "/rabid";
        action_list_str += "/bite";
      }
      else if ( pet_type == PET_RAPTOR )
      {
        action_list_str = "auto_attack/savage_rend";
        if ( talents.call_of_the_wild ) action_list_str += "/call_of_the_wild";
        if ( talents.rabid ) action_list_str += "/rabid";
        action_list_str += "/claw";
      }
      else if ( pet_type == PET_WOLF )
      {
        action_list_str = "auto_attack/furious_howl";
        if ( talents.call_of_the_wild ) action_list_str += "/call_of_the_wild";
        if ( talents.rabid ) action_list_str += "/rabid";
        action_list_str += "/claw";
      }
      else assert( 0 );
    }
    else if ( group() == PET_CUNNING )
    {
      if ( pet_type == PET_WIND_SERPENT )
      {
        action_list_str = "auto_attack";
        if ( talents.roar_of_recovery ) action_list_str += "/roar_of_recovery";
        if ( talents.wolverine_bite ) action_list_str += "/wolverine_bite";
        action_list_str += "/lightning_breath/bite";
      }
      else assert( 0 );
    }
    else // TENACITY
    {
      assert( 0 );
    }
  }

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

    base_gcd = 1.20;
  }

  virtual void init_buffs()
  {
    pet_t::init_buffs();
    hunter_pet_t* p = ( hunter_pet_t* ) this -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();
    buffs_bestial_wrath     = new buff_t( this, "bestial_wrath",     1, 10.0 );
    buffs_call_of_the_wild  = new buff_t( this, "call_of_the_wild",  1, 10.0 );
    buffs_culling_the_herd  = new buff_t( this, "culling_the_herd",  1, 10.0, 0.0, talents.culling_the_herd );
    buffs_frenzy            = new buff_t( this, "frenzy",            1,  8.0, 0.0, o -> talents.frenzy * 0.2 );
    buffs_furious_howl      = new buff_t( this, "furious_howl",      1, 20.0 );
    buffs_kill_command      = new buff_t( this, "kill_command",      3, 30.0 );
    buffs_monstrous_bite    = new buff_t( this, "monstrous_bite",    3, 12.0 );
    buffs_owls_focus        = new buff_t( this, "owls_focus",        1,  8.0, 0.0, talents.owls_focus * 0.15 );
    buffs_rabid             = new buff_t( this, "rabid",             1, 20.0 );
    buffs_rabid_power_stack = new buff_t( this, "rabid_power_stack", 1,    0, 0.0, talents.rabid * 0.5 );   // FIXME: Probably a ppm, not flat chance
    buffs_savage_rend       = new buff_t( this, "savage_rend",       1, 30.0 );
    buffs_wolverine_bite    = new buff_t( this, "wolverine_bite",    1, 10.0, 0.0, talents.wolverine_bite );
    buffs_tier9_4pc    = new stat_buff_t( this, "tier9_4pc", STAT_ATTACK_POWER, 600, 1, 15.0, 45.0, ( o -> set_bonus.tier9_4pc_melee() ? 0.35 : 0 ) );
  }

  virtual void init_gains()
  {
    pet_t::init_gains();
    gains_go_for_the_throat = get_gain( "go_for_the_throat" );
  }

  virtual void init_uptimes()
  {
    pet_t::init_uptimes();
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
    ap += buffs_furious_howl -> value();

    return ap;
  }

  virtual double composite_attack_power_multiplier() SC_CONST
  {
    hunter_t* o = owner -> cast_hunter();

    double mult = player_t::composite_attack_power_multiplier();

    if ( o -> active_aspect == ASPECT_BEAST )
      mult *= 1.10 + o -> glyphs.the_beast * 0.02;

    if ( buffs_rabid -> up() )
      mult *= 1.0 + buffs_rabid_power_stack -> stack() * 0.05;

    if ( buffs_call_of_the_wild -> up() )
      mult *= 1.1;

    if ( o -> buffs_tier10_4pc -> up() )
      mult *= 1.2;

    return mult;
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
        { "culling_the_herd", OPT_INT, &( talents.culling_the_herd ) },
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

  virtual std::vector<talent_translation_t>& get_talent_list()
  {
	if(this->talent_list.empty())
	{
		talent_translation_t *translation_table;
		if ( group() == PET_FEROCITY )
		{
		  talent_translation_t group_table[] =
		  {
			{  1, 2, &( talents.cobra_reflexes         ), 0, 0 },
			{  2, 0, NULL                               , 0, 0 },
      {  3, 0, NULL                               , 0, 0 }, // Empty talent, but need for compatibility with wowarmory.
			{  4, 0, NULL                               , 0, 0 },
			{  5, 0, NULL                               , 0, 0 },
			{  6, 0, NULL                               , 1, 0 },
			{  7, 0, NULL                               , 1, 0 },
			{  8, 3, &( talents.spiked_collar          ), 1, 0 },
			{  9, 0, NULL                               , 1, 0 },
      { 10, 0, NULL                               , 1, 0 }, // Empty talent, but need for compatibility with wowarmory.
			{ 11, 3, &( talents.culling_the_herd       ), 2, 0 },
			{ 12, 0, NULL                               , 2, 0 },
			{ 13, 0, NULL                               , 2, 0 },
			{ 14, 0, NULL                               , 3, 7 },
			{ 15, 3, &( talents.spiders_bite           ), 3, 0 },
			{ 16, 0, NULL                               , 3, 0 },
			{ 17, 1, &( talents.rabid                  ), 4, 0 },
			{ 18, 0, NULL                               , 4, 14 },
			{ 19, 1, &( talents.call_of_the_wild       ), 4, 15 },
			{ 20, 2, &( talents.shark_attack           ), 5, 0 },
			{ 21, 2, &( talents.wild_hunt              ), 5, 19 },
			{  0, 0, NULL                                } /// must have talent with index 0 here
		  };
		  translation_table = group_table;
		}
		else if ( group() == PET_CUNNING )
		{
		  talent_translation_t group_table[] =
		  {
			{  1, 2, &( talents.cobra_reflexes         ), 0, 0  },
			{  2, 0, NULL                               , 0, 0  },
      {  3, 0, NULL                               , 0, 0  }, // Empty talent, but need for compatibility with wowarmory.
			{  4, 0, NULL                               , 0, 0  },
			{  5, 0, NULL                               , 0, 0  },
			{  6, 0, NULL                               , 1, 0  },
      {  7, 0, NULL                               , 0, 0  }, // Empty talent, but need for compatibility with wowarmory.
			{  8, 0, NULL                               , 1, 2  },
			{  9, 2, &( talents.owls_focus             ), 1, 0  },
			{ 10, 3, &( talents.spiked_collar          ), 1, 0  },
			{ 11, 3, &( talents.culling_the_herd       ), 2, 0  },
			{ 12, 0, NULL                               , 2, 0  },
			{ 13, 0, NULL                               , 2, 0  },
			{ 14, 0, NULL                               , 3, 0  },
			{ 15, 0, NULL                               , 3, 0  },
			{ 16, 2, &( talents.feeding_frenzy         ), 3, 10 },
			{ 17, 1, &( talents.wolverine_bite         ), 4, 11 },
			{ 18, 1, &( talents.roar_of_recovery       ), 4, 0  },
			{ 19, 0, NULL                               , 4, 0  },
			{ 20, 0, NULL                               , 4, 0  },
			{ 21, 2, &( talents.wild_hunt              ), 5, 17 },
			{ 22, 0, NULL                               , 5, 20 },
			{  0, 0, NULL                                } /// must have talent with index 0 here
		  };
		  translation_table = group_table;
		}
		else // TENACITY
		{
		  return talent_list;
		}

		int count = 0;

	  	for(int i=0; translation_table[i].index;i++)
		{
			if(translation_table[i].index > 0)
			{
				talent_list.push_back(translation_table[i]);
				talent_list[count].tree = 0;
				talent_list[count].index = count+1;
				count++;
			}
		}
	}
    return talent_list;
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

    // The use of "normalized_weapon_damage" and not "normalized_weapon_speed" is intentional.
    double weapon_speed = normalize_weapon_damage ? weapon -> normalized_weapon_speed() : weapon -> swing_time;
    double bonus_damage = p -> ammo_dps * weapon_speed;

    base_dd_min += bonus_damage;
    base_dd_max += bonus_damage;
  }

  virtual void add_scope()
  {
    if ( player -> items[ weapon -> slot ].encoded_enchant_str == "scope" )
    {
      double scope_damage = util_t::ability_rank( player -> level, 15.0,72,  12.0,67,  7.0,0 );

      base_dd_min += scope_damage;
      base_dd_max += scope_damage;
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

// check_pet_type ==========================================================

static void check_pet_type( action_t* a, int pet_type )
{
  hunter_pet_t* p = ( hunter_pet_t* ) a -> player -> cast_pet();
  hunter_t*     o = p -> owner -> cast_hunter();

  if ( p -> pet_type != pet_type )
  {
    a -> sim -> errorf( "Player %s has pet %s attempting to use action %s that is not available to that class of pets.\n",
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

    double t = attack_t::execute_time();
    if ( t == 0 ) return 0;

    if ( p -> buffs_frenzy -> up() ) t *= 1.0 / 1.3;

    return t;
  }

  virtual double cost() SC_CONST
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    if ( p -> buffs_owls_focus -> check() ) return 0;

    return attack_t::cost();
  }

  virtual void consume_resource()
  {
    attack_t::consume_resource();
    if ( special && base_cost > 0 )
    {
      hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
      p -> buffs_owls_focus -> expire();
    }
  }

  virtual void execute()
  {
    attack_t::execute();
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    if ( result_is_hit() )
    {
      if ( p -> buffs_rabid -> check() )
        p -> buffs_rabid_power_stack -> trigger();

      if ( result == RESULT_CRIT )
      {
        p -> buffs_frenzy -> trigger();
        if ( special ) trigger_invigoration( this );
        p -> buffs_wolverine_bite -> trigger();
      }
    }
    if ( special )
    {
      hunter_t* o = p -> owner -> cast_hunter();
      o -> buffs_cobra_strikes -> decrement();
      p -> buffs_kill_command -> decrement();
      p -> buffs_owls_focus -> trigger();
    }
  }

  virtual void player_buff()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    attack_t::player_buff();

    if ( p -> buffs_bestial_wrath -> up() ) player_multiplier *= 1.50;

    if ( p -> buffs_savage_rend -> up() ) player_multiplier *= 1.10;

    player_multiplier *= 1.0 + p -> buffs_monstrous_bite -> stack() * 0.03;

    if ( p -> buffs_culling_the_herd -> up() )
      player_multiplier *= 1.0 + ( p -> buffs_culling_the_herd -> value() * 0.01 );

    if ( p -> sim -> target -> health_percentage() < 35 )
      player_multiplier *= 1.0 + p -> talents.feeding_frenzy * 0.06;

    if ( special )
    {
      if ( o -> buffs_cobra_strikes -> up() ) player_crit += 1.0;

      if ( p -> buffs_kill_command -> up() )
      {
        player_multiplier *= 1.0 + p -> buffs_kill_command -> stack() * 0.20;
        player_crit       += o -> talents.focused_fire * 0.10;
      }
    }

    if ( o -> buffs_tier10_2pc -> up() )
      player_multiplier *= 1.15;
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

    if ( o -> set_bonus.tier7_2pc_melee() ) base_multiplier *= 1.05;
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
    auto_cast = true;
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    hunter_pet_attack_t::execute();

    if ( result == RESULT_CRIT )
    {
      if ( p -> talents.culling_the_herd )
      {
        p -> buffs_culling_the_herd -> trigger( 1, p -> talents.culling_the_herd );
        o -> buffs_culling_the_herd -> trigger( 1, p -> talents.culling_the_herd );
      }
    }
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
    cooldown -> duration = 10 * ( 1.0 - o -> talents.longevity * 0.10 );
    auto_cast       = true;

    // FIXME! Assuming pets are not smart enough to wait for Rake to finish ticking
    dot_behavior = DOT_CLIP;

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
    cooldown -> duration = 10 * ( 1.0 - o -> talents.longevity * 0.10 );
    auto_cast = true;

    id = 55499;
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    hunter_pet_attack_t::execute();

    p -> buffs_monstrous_bite -> trigger();
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
    cooldown -> duration = 60 * ( 1.0 - o -> talents.longevity * 0.10 );
    auto_cast      = true;

    // FIXME! Assuming pets are not smart enough to wait for Rake to finish ticking
    dot_behavior = DOT_CLIP;

    id = 53582;
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    school = SCHOOL_PHYSICAL;
    hunter_pet_attack_t::execute();
    school = SCHOOL_BLEED;

    if ( result == RESULT_CRIT )
    {
      p -> buffs_savage_rend -> trigger();
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
    cooldown -> duration = 10 * ( 1.0 - o -> talents.longevity * 0.10 );
    auto_cast   = true;

    may_dodge = may_block = may_parry = false;

    id = 53508;
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    p -> buffs_wolverine_bite -> expire();
    hunter_pet_attack_t::execute();
  }

  virtual bool ready()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    // This attack is only available after the target dodges
    if ( ! p -> buffs_wolverine_bite -> check() )
      return false;

    return hunter_pet_attack_t::ready();
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
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    base_multiplier *= 1.05; // 3.1.0 change: # Cunning, Ferocity and Tenacity pets now all have +5% damage, +5% armor and +5% health bonuses.

    base_multiplier *= 1.0 + p -> talents.spiked_collar * 0.03;
  }

  virtual double cost() SC_CONST
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    if ( p -> buffs_owls_focus -> check() ) return 0;

    return spell_t::cost();
  }

  virtual void consume_resource()
  {
    spell_t::consume_resource();
    if ( base_cost > 0 )
    {
      hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
      p -> buffs_owls_focus -> expire();
    }
  }

  virtual void execute()
  {
    spell_t::execute();
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    if ( result_is_hit() )
    {
      if ( result == RESULT_CRIT )
      {
        trigger_invigoration( this );
      }
    }
    hunter_t* o = p -> owner -> cast_hunter();
    o -> buffs_cobra_strikes -> decrement();
    p -> buffs_kill_command -> decrement();
    p -> buffs_owls_focus -> trigger();
  }

  virtual void player_buff()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    spell_t::player_buff();

    player_spell_power += 0.125 * o -> composite_attack_power();

    if ( p -> buffs_bestial_wrath -> up() ) player_multiplier *= 1.50;

    if ( o -> buffs_cobra_strikes -> up() ) player_crit += 1.0;

    if ( p -> buffs_culling_the_herd -> up() )
      player_multiplier *= 1.0 + ( p -> buffs_culling_the_herd -> value() * 0.01 );

    if ( p -> buffs_kill_command -> up() )
    {
      player_multiplier *= 1.0 + p -> buffs_kill_command -> current_stack * 0.20;
      player_crit       += o -> talents.focused_fire * 0.10;
    }

    if ( p -> sim -> target -> health_percentage() < 35 )
      player_multiplier *= 1.0 + p -> talents.feeding_frenzy * 0.06;

    if ( o -> buffs_tier10_2pc -> up() )
      player_multiplier *= 1.15;
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
    base_cost = 20;
    direct_power_mod = 1.5 / 3.5;
    cooldown -> duration = 10 * ( 1.0 - o -> talents.longevity * 0.10 );
    auto_cast = true;

    id = 55492;
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
    base_cost = 20;
    direct_power_mod = 1.5 / 3.5;
    cooldown -> duration = 10 * ( 1.0 - o -> talents.longevity * 0.10 );
    auto_cast = true;

    id = 25012;
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
    cooldown -> duration = 5 * 60 * ( 1.0 - o -> talents.longevity * 0.10 );
    trigger_gcd = 0.0;
    auto_cast = true;

    id = 53434;
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();
    o -> buffs_call_of_the_wild -> trigger();
    p -> buffs_call_of_the_wild -> trigger();
    consume_resource();
    update_ready();
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
    cooldown -> duration = 40 * ( 1.0 - o -> talents.longevity * 0.10 );
    trigger_gcd = 0.0;
    auto_cast = true;

    id = 64495;
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();
    double apbonus = 4 * p -> level;
    o -> buffs_furious_howl -> trigger( 1, apbonus );
    p -> buffs_furious_howl -> trigger( 1, apbonus );
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    update_ready();
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
    cooldown -> duration = 45 * ( 1.0 - o -> talents.longevity * 0.10 );
    trigger_gcd = 0.0;
    auto_cast = true;

    id = 53401;
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    p -> buffs_rabid_power_stack -> expire();
    p -> buffs_rabid -> trigger();

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    update_ready();
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
    cooldown -> duration = 360 * ( 1.0 - o -> talents.longevity * 0.10 );
    auto_cast = true;

    id = 53517;
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
  if ( p -> buffs_beast_within -> up() ) c *= 0.50;
  return c;
}

// hunter_attack_t::execute ================================================

void hunter_attack_t::execute()
{
  attack_t::execute();

  hunter_t* p = player -> cast_hunter();
  if ( result_is_hit() )
  {
    trigger_aspect_of_the_viper( this );
    p -> buffs_master_tactician -> trigger( 1, p -> talents.master_tactician * 0.02 );
    if ( p -> active_pet )
      p -> active_pet -> buffs_tier9_4pc -> trigger();

    if ( result == RESULT_CRIT )
    {
      p -> buffs_expose_weakness -> trigger();
      trigger_go_for_the_throat( this );
      trigger_thrill_of_the_hunt( this );
    }
  }
}

// hunter_attack_t::execute_time ============================================

double hunter_attack_t::execute_time() SC_CONST
{
  hunter_t* p = player -> cast_hunter();

  double t = attack_t::execute_time();
  if ( t == 0 ) return 0;

  t *= 1.0 / ( 1.0 + p -> buffs_rapid_fire -> value() );

  t *= 1.0 / p -> quiver_haste;

  t *= 1.0 / ( 1.0 + 0.04 * p -> talents.serpents_swiftness );

  t *= 1.0 / ( 1.0 + p -> buffs_improved_aspect_of_the_hawk -> value() );

  return t;
}

// hunter_attack_t::player_buff =============================================

void hunter_attack_t::player_buff()
{
  hunter_t* p = player -> cast_hunter();
  target_t* t = sim -> target;

  attack_t::player_buff();

  if ( t -> debuffs.hunters_mark  -> check() )
  {
    if ( weapon && weapon -> group() == WEAPON_RANGED )
    {
      // FIXME: This should be all shots, not just weapon-based shots
      player_multiplier *= 1.0 + p -> talents.marked_for_death * 0.01;
    }
  }

  player_multiplier *=  1.0 + p -> buffs_aspect_of_the_viper -> value();

  if ( p -> talents.beast_within > 0 )
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
  player_crit += p -> buffs_master_tactician -> value();

  if ( p -> buffs_culling_the_herd -> up() )
    player_multiplier *= 1.0 + ( p -> buffs_culling_the_herd -> value() * 0.01 );

  if ( p -> buffs_tier10_2pc -> up() )
    player_multiplier *= 1.15;
}

// Ranged Attack ===========================================================

struct ranged_t : public hunter_attack_t
{
  // FIXME! Setting "special=true" would create the desired 2-roll effect,
  // but it would also triger Honor-Among-Thieves procs.......
  double iaoth_bonus;
  ranged_t( player_t* player ) :
      hunter_attack_t( "ranged", player, SCHOOL_PHYSICAL, TREE_NONE, /*special*/false ), iaoth_bonus( 0.0 )
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

    iaoth_bonus = 0.03 * p -> talents.improved_aspect_of_the_hawk;
    if ( p -> glyphs.the_hawk ) iaoth_bonus += 0.06;
  }

  virtual double execute_time() SC_CONST
  {
    if ( ! player -> in_combat ) return 0.01;
    return hunter_attack_t::execute_time();
  }

  virtual void execute()
  {
    hunter_attack_t::execute();

    if ( result_is_hit() )
    {
      hunter_t* p = player -> cast_hunter();
      trigger_wild_quiver( this );
      if ( p -> active_aspect == ASPECT_HAWK )
        p -> buffs_improved_aspect_of_the_hawk -> trigger( 1, iaoth_bonus );

      p -> buffs_tier10_2pc -> trigger();
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
    if ( p -> buffs.moving -> check() ) return false;
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
    init_rank( ranks, 49050 );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    may_crit = true;
    normalize_weapon_speed = true;

    cooldown = p -> get_cooldown( "aimed_multi" );
    cooldown -> duration = 10;

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
      cooldown -> duration -= 2;
    }
  }

  virtual double cost() SC_CONST
  {
    hunter_t* p = player -> cast_hunter();
    double c = hunter_attack_t::cost();
    if ( p -> buffs_improved_steady_shot -> check() ) c *= 0.80;
    return c;
  }

  virtual void player_buff()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::player_buff();
    if ( p -> buffs_trueshot_aura -> check() && p -> glyphs.trueshot_aura ) player_crit += 0.10;
    if ( p -> buffs_improved_steady_shot -> up() ) player_multiplier *= 1.15;
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    hunter_t* p = player -> cast_hunter();
    p -> buffs_improved_steady_shot -> expire();
    if ( result == RESULT_CRIT )
    {
      trigger_piercing_shots( this );
    }
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( improved_steady_shot )
      if ( ! p -> buffs_improved_steady_shot -> check() )
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
    init_rank( ranks, 49045 );

    // To trigger ppm-based JoW
    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );
    weapon_multiplier = 0;

    may_crit = true;

    cooldown = p -> get_cooldown( "arcane_explosive" );
    cooldown -> duration = 6;

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
    if ( p -> buffs_lock_and_load -> check() ) return 0;
    double c = hunter_attack_t::cost();
    if ( p -> buffs_improved_steady_shot -> check() ) c *= 0.80;
    return c;
  }

  void player_buff()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::player_buff();
    if ( p -> buffs_improved_steady_shot -> up() ) player_multiplier *= 1.15;
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    hunter_t* p = player -> cast_hunter();
    if ( result_is_hit() )
    {
      if ( p -> glyphs.arcane_shot && p -> active_sting() )
      {
	p -> resource_gain( RESOURCE_MANA, 0.20 * resource_consumed, p -> gains_glyph_of_arcane_shot );
      }
      if ( result == RESULT_CRIT )
      {
	p -> buffs_cobra_strikes -> trigger( 2 );
	trigger_hunting_party( this );
      }
    }
    p -> buffs_lock_and_load -> decrement();
    p -> buffs_improved_steady_shot -> expire();
  }

  virtual void update_ready()
  {
    hunter_t* p = player -> cast_hunter();
    cooldown -> duration = ( p -> buffs_lock_and_load -> check() ? 0.0 : 6.0 );
    hunter_attack_t::update_ready();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( improved_steady_shot )
      if ( ! p -> buffs_improved_steady_shot -> check() )
        return false;

    if ( lock_and_load )
      if ( ! p -> buffs_lock_and_load -> check() )
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
    init_rank( ranks, 63672 );

    base_tick_time   = 3.0;
    num_ticks        = 5;
    tick_power_mod   = 0.1 / 5.0;

    cooldown = p -> get_cooldown( "traps" );
    cooldown -> duration = 30 - p -> talents.resourcefulness * 2;

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
    hunter_t* p = player -> cast_hunter();
    p -> buffs_lock_and_load -> trigger( 2 );
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

    cooldown -> duration = 10;

    base_multiplier *= p -> ranged_weapon_specialization_multiplier();

    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.mortal_shots     * 0.06 +
                                          p -> talents.marked_for_death * 0.02 );

    add_ammunition();
    add_scope();

    if ( p -> glyphs.chimera_shot )
    {
      cooldown -> duration -= 1;
    }

    id = 53209;
  }

  virtual double cost() SC_CONST
  {
    hunter_t* p = player -> cast_hunter();
    double c = hunter_attack_t::cost();
    if ( p -> buffs_improved_steady_shot -> check() ) c *= 0.80;
    return c;
  }

  void player_buff()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::player_buff();
    if ( p -> buffs_improved_steady_shot -> up() ) player_multiplier *= 1.0 + 0.15;
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

      sting -> may_resist = false;
      double sting_dmg = sting -> num_ticks * sting -> calculate_tick_damage();
      sting -> may_resist = true;

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
    p -> buffs_improved_steady_shot -> expire();
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
      if ( ! p -> buffs_improved_steady_shot -> check() )
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
    init_rank( ranks, 60053 );

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

    cooldown = p -> get_cooldown( "arcane_explosive" );
    cooldown -> duration = 6;

    tick_zero      = true;
    num_ticks      = 2;
    base_tick_time = 1.0;

    explosive_tick = new explosive_tick_t( p );

    id = 60053;
  }

  virtual double cost() SC_CONST
  {
    hunter_t* p = player -> cast_hunter();
    if ( p -> buffs_lock_and_load -> check() ) return 0;
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
    cooldown -> duration = ( p -> buffs_lock_and_load -> check() ? 0.0 : 6.0 );
    hunter_attack_t::update_ready();
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    hunter_t* p = player -> cast_hunter();
    p -> buffs_lock_and_load -> decrement();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( lock_and_load )
      if ( ! p -> buffs_lock_and_load -> check() )
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
      { 80, 3, 325, 325, 0, 0.07 },
      { 75, 2, 250, 250, 0, 0.07 },
      { 71, 1, 205, 205, 0, 0.07 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 61006 );

    parse_options( 0, options_str );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    may_crit               = true;
    normalize_weapon_speed = true;
    weapon_multiplier      = 2.0;
    direct_power_mod       = 0.40;
    cooldown -> duration   = 15;

    base_multiplier *= p -> ranged_weapon_specialization_multiplier();

    base_crit += p -> talents.sniper_training * 0.05;

    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.mortal_shots     * 0.06 +
                                          p -> talents.marked_for_death * 0.02 );

    add_ammunition();
    add_scope();

    if ( p -> glyphs.kill_shot )
    {
      cooldown -> duration -= 6;
    }
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    hunter_t* p = player -> cast_hunter();
    if ( result == RESULT_CRIT )
    {
      p -> buffs_cobra_strikes -> trigger( 2 );
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
    init_rank( ranks, 49048 );

    parse_options( 0, options_str );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    may_crit               = true;
    normalize_weapon_speed = true;

    cooldown = p -> get_cooldown( "aimed_multi" );
    cooldown -> duration = 10 - p -> glyphs.multi_shot;

    base_multiplier *= 1.0 + p -> talents.barrage * 0.04;
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
    cooldown -> duration   = 30;

    weapon_multiplier *= 0.5;

    base_multiplier *= p -> ranged_weapon_specialization_multiplier();

    base_crit_bonus_multiplier *= 1.0 + p -> talents.mortal_shots * 0.06;

    add_ammunition();
    add_scope();

    id = 19503;
  }
};

// Scorpid Sting Attack =========================================================

struct scorpid_sting_t : public hunter_attack_t
{
  int force;

  scorpid_sting_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "scorpid_sting", player, SCHOOL_PHYSICAL, TREE_MARKSMANSHIP ),
      force( 0 )
  {
    hunter_t* p = player -> cast_hunter();

    option_t options[] =
    {
      { "force", OPT_BOOL, &force },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_crit = false;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.11;
    base_dd_min = base_dd_max = 0;
    base_multiplier = 0;

    observer = &( p -> active_scorpid_sting );

    id = 3043;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    p -> cancel_sting();
    hunter_attack_t::execute();
    if ( result_is_hit() ) sim -> target -> debuffs.scorpid_sting -> trigger();
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
    init_rank( ranks, 49001 );

    base_tick_time   = 3.0;
    num_ticks        = p -> glyphs.serpent_sting ? 7 : 5;
    tick_power_mod   = 0.2 / 5.0;
    base_multiplier *= 1.0 + ( p -> talents.improved_stings     * 0.1 +
                               p -> set_bonus.tier8_2pc_melee() * 0.1 );

    tick_may_crit = ( p -> set_bonus.tier9_2pc_melee() != 0 );
    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.mortal_shots * 0.06 );

    observer = &( p -> active_serpent_sting );
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    p -> cancel_sting();
    hunter_attack_t::execute();
    if ( result_is_hit() ) sim -> target -> debuffs.poisoned -> increment();
  }

  virtual void last_tick()
  {
    hunter_attack_t::last_tick();
    sim -> target -> debuffs.poisoned -> decrement();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( ! force )
      if ( p -> active_sting() )
        return false;

    return hunter_attack_t::ready();
  }

  virtual void tick()
  {
    hunter_t* p = player -> cast_hunter();

    hunter_attack_t::tick();

    if ( tick_dmg > 0 )
    {
      p -> buffs_tier10_4pc-> trigger();
    }
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

    cooldown -> duration   = 20;

    trigger_gcd = 0;

    weapon_multiplier *= 0.5;

    base_multiplier *= p -> ranged_weapon_specialization_multiplier();

    base_crit_bonus_multiplier *= 1.0 + p -> talents.mortal_shots * 0.06;

    add_ammunition();
    add_scope();

    id = 34490;
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
    init_rank( ranks, 49052 );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    normalize_weapon_damage = true;
    normalize_weapon_speed  = true;
    weapon_power_mod        = 0;
    direct_power_mod        = 0.1;
    base_execute_time       = 2.0;

    may_crit = true;

    base_cost *= 1.0 - p -> talents.master_marksman * 0.05;

    base_multiplier *= 1.0 + ( p -> talents.sniper_training       * 0.02 +
			       p -> talents.ferocious_inspiration * 0.03 );

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
      hunter_t* p = player -> cast_hunter();
      p -> buffs_improved_steady_shot -> trigger();
      p -> buffs_tier8_4pc -> trigger();

      if ( result == RESULT_CRIT )
      {
        p -> buffs_cobra_strikes -> trigger( 2 );
        trigger_hunting_party( this );
        trigger_piercing_shots( this );
      }
    }
  }

  void player_buff()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::player_buff();
    if ( (p -> glyphs.steady_shot || p -> talents.marked_for_death > 0 ) && p -> active_sting() )
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

    cooldown -> duration = 1.0;

    trigger_gcd      = 0.0;
    harmful          = false;
    hawk_bonus       = util_t::ability_rank( p -> level, 300,80, 230,74, 155,68,  120,0 );
    viper_multiplier = -0.50;

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
      if ( pet -> buffs_bestial_wrath -> check() ) return ASPECT_BEAST;
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
        p -> buffs_aspect_of_the_viper -> expire();
        p -> buffs_aspect_of_the_hawk -> trigger( 1, hawk_bonus );
      }
      else if ( aspect == ASPECT_VIPER )
      {
        if ( sim -> log ) log_t::output( sim, "%s performs Aspect of the Viper", p -> name() );
        p -> active_aspect = ASPECT_VIPER;
        p -> buffs_aspect_of_the_hawk -> expire();
        p -> buffs_aspect_of_the_viper -> trigger(1, viper_multiplier );
      }
      else if ( aspect == ASPECT_BEAST )
      {
        if ( sim -> log ) log_t::output( sim, "%s performs Aspect of the Beast", p -> name() );
        p -> active_aspect = ASPECT_BEAST;
        p -> buffs_aspect_of_the_hawk -> expire();
        p -> buffs_aspect_of_the_viper -> expire();
      }
      else
      {
        p -> active_aspect = ASPECT_NONE;
        p -> buffs_aspect_of_the_hawk -> expire();
        p -> buffs_aspect_of_the_viper -> expire();
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
    cooldown -> duration = ( 120 - p -> glyphs.bestial_wrath * 20 ) * ( 1 - p -> talents.longevity * 0.1 );

    id = 19574;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_pet_t* pet = p -> active_pet;
    p -> buffs_beast_within -> trigger();
    pet -> buffs_bestial_wrath -> trigger();

    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
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

    id = 53338;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    target_t* t = sim -> target;
    t -> debuffs.hunters_mark -> trigger( 1, ap_bonus );
  }

  virtual bool ready()
  {
    if ( ! hunter_spell_t::ready() )
      return false;

    return ap_bonus > sim -> target -> debuffs.hunters_mark -> current_value;
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
    cooldown -> duration  = 60 - 10 * p -> talents.catlike_reflexes;
    trigger_gcd = 0;

    id = 34026;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> active_pet -> buffs_kill_command -> trigger( 3 );
    consume_resource();
    update_ready();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( ! p -> active_pet )
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
    cooldown -> duration  = 300;
    cooldown -> duration -= p -> talents.rapid_killing * 60;
    trigger_gcd = 0.0;
    harmful = false;

    id = 34026;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );

    p -> buffs_rapid_fire -> trigger( 1, ( p -> glyphs.rapid_fire ? 0.48 : 0.40 ) );

    consume_resource();
    update_ready();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( p -> buffs_rapid_fire -> check() )
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
  int wait_for_rf;
  std::vector<cooldown_t*> cooldown_list;

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

    cooldown -> duration = 180;
    trigger_gcd = 1.0;
    harmful = false;

    cooldown_list.push_back( p -> get_cooldown( "aimed_multi"      ) );
    cooldown_list.push_back( p -> get_cooldown( "arcane_explosive" ) );
    cooldown_list.push_back( p -> get_cooldown( "traps"            ) );
    cooldown_list.push_back( p -> get_cooldown( "chimera_shot"     ) );
    cooldown_list.push_back( p -> get_cooldown( "kill_shot"        ) );
    cooldown_list.push_back( p -> get_cooldown( "scatter_shot"     ) );
    cooldown_list.push_back( p -> get_cooldown( "silencing_shot"   ) );
    cooldown_list.push_back( p -> get_cooldown( "kill_command"     ) );
    cooldown_list.push_back( p -> get_cooldown( "rapid_fire"       ) );

    id = 3045;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    int num_cooldowns = (int) cooldown_list.size();
    for ( int i=0; i < num_cooldowns; i++ )
    {
      cooldown_list[ i ] -> reset();
    }

    update_ready();
  }

  virtual bool ready()
  {
    if ( ! hunter_spell_t::ready() )
      return false;

    hunter_t* p = player -> cast_hunter();
    if ( wait_for_rf && ! p -> buffs_rapid_fire -> check() )
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

    id = 19506;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_trueshot_aura -> trigger();
    sim -> auras.trueshot -> trigger();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();
    return( ! p -> buffs_trueshot_aura -> check() );
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
  if ( name == "scorpid_sting"         ) return new          scorpid_sting_t( this, options_str );
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
  pet_t* p = find_pet( pet_name );
  if ( p ) return p;

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

// hunter_t::create_pets ======================================================

void hunter_t::create_pets()
{
  create_pet( "cat" );
  create_pet( "devilsaur" );
  create_pet( "raptor" );
  create_pet( "wind_serpent" );
  create_pet( "wolf" );
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
	{ 52020, 91.5 }, { 52021, 91.5 },
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

        pet -> init_actions();
      }
    }
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
    else if ( n == "arcane_shot"                 ) glyphs.arcane_shot = 1;
    else if ( n == "aspect_of_the_viper"         ) glyphs.aspect_of_the_viper = 1;
    else if ( n == "bestial_wrath"               ) glyphs.bestial_wrath = 1;
    else if ( n == "chimera_shot"                ) glyphs.chimera_shot = 1;
    else if ( n == "explosive_shot"              ) glyphs.explosive_shot = 1;
    else if ( n == "hunters_mark"                ) glyphs.hunters_mark = 1;
    else if ( n == "improved_aspect_of_the_hawk" ) glyphs.the_hawk = 1;
    else if ( n == "kill_shot"                   ) glyphs.kill_shot = 1;
    else if ( n == "multi_shot"                  ) glyphs.multi_shot = 1;
    else if ( n == "rapid_fire"                  ) glyphs.rapid_fire = 1;
    else if ( n == "serpent_sting"               ) glyphs.serpent_sting = 1;
    else if ( n == "steady_shot"                 ) glyphs.steady_shot = 1;
    else if ( n == "the_beast"                   ) glyphs.the_beast = 1;
    else if ( n == "the_hawk"                    ) glyphs.the_hawk = 1;
    else if ( n == "trueshot_aura"               ) glyphs.trueshot_aura = 1;
    // To prevent warnings....
    else if ( n == "disengage"          ) ;
    else if ( n == "explosive_trap"     ) ;
    else if ( n == "feign_death"        ) ;
    else if ( n == "freezing_trap"      ) ;
    else if ( n == "frost_trap"         ) ;
    else if ( n == "mend_pet"           ) ;
    else if ( n == "mending"            ) ;
    else if ( n == "possessed_strength" ) ;
    else if ( n == "revive_pet"         ) ;
    else if ( n == "scare_beast"        ) ;
    else if ( n == "the_pack"           ) ;
    else if ( n == "volley"             ) ;
    else if ( n == "wyvern_sting"       ) ;
    else if ( ! sim -> parent ) 
    {
      sim -> errorf( "Player %s has unrecognized glyph %s\n", name(), n.c_str() );
    }
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
  attribute_base[ ATTR_STRENGTH  ] = rating_t::get_attribute_base( sim, level, HUNTER, race, BASE_STAT_STRENGTH );
  attribute_base[ ATTR_AGILITY   ] = rating_t::get_attribute_base( sim, level, HUNTER, race, BASE_STAT_AGILITY );
  attribute_base[ ATTR_STAMINA   ] = rating_t::get_attribute_base( sim, level, HUNTER, race, BASE_STAT_STAMINA );
  attribute_base[ ATTR_INTELLECT ] = rating_t::get_attribute_base( sim, level, HUNTER, race, BASE_STAT_INTELLECT );
  attribute_base[ ATTR_SPIRIT    ] = rating_t::get_attribute_base( sim, level, HUNTER, race, BASE_STAT_SPIRIT );
  resource_base[ RESOURCE_HEALTH ] = rating_t::get_attribute_base( sim, level, HUNTER, race, BASE_STAT_HEALTH );
  resource_base[ RESOURCE_MANA   ] = rating_t::get_attribute_base( sim, level, HUNTER, race, BASE_STAT_MANA );
  base_spell_crit                  = rating_t::get_attribute_base( sim, level, HUNTER, race, BASE_STAT_SPELL_CRIT );
  base_attack_crit                 = rating_t::get_attribute_base( sim, level, HUNTER, race, BASE_STAT_MELEE_CRIT );
  initial_spell_crit_per_intellect = rating_t::get_attribute_base( sim, level, HUNTER, race, BASE_STAT_SPELL_CRIT_PER_INT );
  initial_attack_crit_per_agility  = rating_t::get_attribute_base( sim, level, HUNTER, race, BASE_STAT_MELEE_CRIT_PER_AGI );

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

// hunter_t::init_buffs =======================================================

void hunter_t::init_buffs()
{
  player_t::init_buffs();

  buffs_aspect_of_the_hawk          = new buff_t( this, "aspect_of_the_hawk",          1,  0.0 );
  buffs_aspect_of_the_viper         = new buff_t( this, "aspect_of_the_viper",         1,  0.0 );
  buffs_beast_within                = new buff_t( this, "beast_within",                1, 10.0,  0.0, talents.beast_within );
  buffs_call_of_the_wild            = new buff_t( this, "call_of_the_wild",            1, 10.0 );
  buffs_cobra_strikes               = new buff_t( this, "cobra_strikes",               3, 10.0,  0.0, talents.cobra_strikes * 0.20 );
  buffs_culling_the_herd            = new buff_t( this, "culling_the_herd",            1, 10.0 );
  buffs_expose_weakness             = new buff_t( this, "expose_weakness",             1,  7.0,  0.0, talents.expose_weakness / 3.0 );
  buffs_furious_howl                = new buff_t( this, "furious_howl",                1, 20.0 );
  buffs_improved_aspect_of_the_hawk = new buff_t( this, "improved_aspect_of_the_hawk", 1, 10.0,  0.0, ( talents.improved_aspect_of_the_hawk ? 0.10 : 0.0 ) );
  buffs_improved_steady_shot        = new buff_t( this, "improved_steady_shot",        1, 10.0,  0.0, talents.improved_steady_shot * 0.05 );
//  buffs_lock_and_load               = new buff_t( this, "lock_and_load",               2, 10.0, 22.0, talents.lock_and_load * 0.02 );
  buffs_lock_and_load               = new buff_t( this, "lock_and_load",               2, 10.0, 22.0, talents.lock_and_load * (0.20/3.0) ); // EJ thread suggests the proc is rate is around 20%

  buffs_master_tactician            = new buff_t( this, "master_tactician",            1, 10.0,  0.0, ( talents.master_tactician ? 0.10 : 0.0 ) );
  buffs_rapid_fire                  = new buff_t( this, "rapid_fire",                  1, 15.0 );

  buffs_tier10_2pc                  = new buff_t( this, "tier10_2pc",                  1, 10.0,  0.0, ( set_bonus.tier10_2pc_melee() ? 0.05 : 0 ) );
  buffs_tier10_4pc                  = new buff_t( this, "tier10_4pc",                  1, 10.0,  0.0, ( set_bonus.tier10_4pc_melee() ? 0.05 : 0 ) );

  buffs_tier8_4pc = new stat_buff_t( this, "tier8_4pc", STAT_ATTACK_POWER, 600, 1, 15.0, 45.0, ( set_bonus.tier8_4pc_melee() ? 0.1  : 0 ) );


  // Own TSA for Glyph of TSA
  buffs_trueshot_aura               = new buff_t( this, "trueshot_aura");
}

// hunter_t::init_gains ======================================================

void hunter_t::init_gains()
{
  player_t::init_gains();
  gains_chimera_viper        = get_gain( "chimera_viper" );
  gains_glyph_of_arcane_shot = get_gain( "glyph_of_arcane_shot" );
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
  procs_wild_quiver   = get_proc( "wild_quiver" );
}

// hunter_t::init_uptimes ====================================================

void hunter_t::init_uptimes()
{
  player_t::init_uptimes();
}

// hunter_t::init_rng ========================================================

void hunter_t::init_rng()
{
  player_t::init_rng();

  rng_invigoration         = get_rng( "invigoration"       );
  rng_owls_focus           = get_rng( "owls_focus"         );
  rng_thrill_of_the_hunt   = get_rng( "thrill_of_the_hunt" );
  rng_wild_quiver          = get_rng( "wild_quiver"        );

  // Overlapping procs require the use of a "distributed" RNG-stream when normalized_roll=1
  // also useful for frequent checks with low probability of proc and timed effect

  rng_frenzy               = get_rng( "frenzy",                      RNG_DISTRIBUTED );
  rng_hunting_party        = get_rng( "hunting_party",               RNG_DISTRIBUTED );
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

// hunter_t::init_unique_gear ================================================

void hunter_t::init_unique_gear()
{
  player_t::init_unique_gear();

  item_t* item = find_item( "zods_repeating_longbow" );

  if ( item )  // FIXME! I wish this could be pulled out...
  {
    item -> unique = true;

    struct zods_quick_shot_t : public hunter_attack_t
    {
      zods_quick_shot_t( hunter_t* p ) : hunter_attack_t( "zods_quick_shot", p, SCHOOL_PHYSICAL )
      {
	weapon = &( p -> ranged_weapon );
	normalize_weapon_speed = true;
	weapon_multiplier *= 0.5;
	may_crit    = true;
	background  = true;
	trigger_gcd = 0;
	base_cost   = 0;
	base_dd_min = 1;
	base_dd_max = 1;
	base_multiplier *= p -> ranged_weapon_specialization_multiplier();
	base_crit_bonus_multiplier *= 1.0 + ( p -> talents.mortal_shots * 0.06 );
	add_ammunition();
	add_scope();
      }
    };

    struct zods_trigger_t : public action_callback_t
    {
      attack_t* attack;
      rng_t* rng;
      zods_trigger_t( player_t* p, attack_t* a ) : action_callback_t( p -> sim, p ), attack(a)
      {
	rng = p -> get_rng( "zods_repeating_longbow" );
      }
      virtual void trigger( action_t* a )
      {
	if ( ! a -> weapon ) return;
	if ( a -> weapon -> slot != SLOT_RANGED ) return;
	if ( rng -> roll( 0.04 ) ) attack -> execute();
      }
    };

    register_attack_result_callback( RESULT_ALL_MASK, new zods_trigger_t( this, new zods_quick_shot_t( this ) ) );
  }
}

// hunter_t::init_actions ====================================================

void hunter_t::init_actions()
{
  if ( ranged_weapon.group() != WEAPON_RANGED )
  {
    sim -> errorf( "Player %s does not have a ranged weapon at the Ranged slot.", name() );
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    action_list_str = "flask,type=endless_rage";
    action_list_str += ( primary_tree() != TREE_SURVIVAL ) ? "/food,type=hearty_rhino" : "/food,type=blackened_dragonfin";
    action_list_str += "/hunters_mark/summon_pet";
    if ( talents.trueshot_aura ) action_list_str += "/trueshot_aura";
    action_list_str += "/speed_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
    action_list_str += "/auto_shot";
    action_list_str += "/snapshot_stats";
    int num_items = ( int ) items.size();
    for ( int i=0; i < num_items; i++ )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
      }
    }
    switch ( race )
    {
    case RACE_ORC:       action_list_str += "/blood_fury,time>=10";     break;
    case RACE_TROLL:     action_list_str += "/berserking,time>=10";     break;
    case RACE_BLOOD_ELF: action_list_str += "/arcane_torrent,time>=10"; break;
    }
    switch ( primary_tree() )
    {
    case TREE_BEAST_MASTERY:
      if ( talents.bestial_wrath )
      {
        action_list_str += "/kill_command,sync=bestial_wrath";
        if ( talents.catlike_reflexes == 0 )
        {
          action_list_str += "/kill_command,if=cooldown.bestial_wrath.remains>=60";
        }
        else if ( talents.catlike_reflexes == 1 )
        {
          action_list_str += "/kill_command,if=cooldown.bestial_wrath.remains>=50";
        }
        else if ( talents.catlike_reflexes == 2 )
        {
          action_list_str += "/kill_command,if=cooldown.bestial_wrath.remains>=40";
        }
        else
        {
          action_list_str += "/kill_command,if=cooldown.bestial_wrath.remains>=30";
        }
        action_list_str += "/bestial_wrath";
      }
      else
      {
        action_list_str += "/kill_command";
      }
      action_list_str += "/aspect";
      if ( talents.rapid_killing == 0 )
      {
        action_list_str += "/rapid_fire,if=buff.bloodlust.react";
      }
      else if ( talents.rapid_killing == 1 )
      {
        action_list_str += "/rapid_fire";
      }
      else
      {
        action_list_str += "/rapid_fire,time<=60";
        action_list_str += "/rapid_fire,if=buff.bloodlust.react";
      }
      action_list_str += "/kill_shot";
      action_list_str += "/serpent_sting";
      if ( talents.aimed_shot )
      {
        action_list_str += "/aimed_shot";
      }
      else
      {
        action_list_str += "/multi_shot";
      }
      if ( ( talents.improved_arcane_shot > 0 ) || ! glyphs.steady_shot || ( initial_stats.armor_penetration_rating < 800 ) ) action_list_str += "/arcane_shot";
      action_list_str += "/steady_shot";
      break;
    case TREE_MARKSMANSHIP:
      action_list_str += "/aspect";
      action_list_str += "/serpent_sting";
      action_list_str += "/rapid_fire";
      action_list_str += "/kill_command";
      if ( talents.silencing_shot ) action_list_str += "/silencing_shot";
      action_list_str += "/kill_shot";
      if ( talents.chimera_shot )
      {
        action_list_str += "/wait,sec=cooldown.chimera_shot.remains,if=cooldown.chimera_shot.remains>0&cooldown.chimera_shot.remains<0.25";
        action_list_str += "/chimera_shot";
      }
      if ( talents.aimed_shot ) 
      {
        action_list_str += "/aimed_shot";
      }
      else
      {
        action_list_str += "/multi_shot";
      }
      if ( ( talents.improved_arcane_shot > 0 ) || ! glyphs.steady_shot || ( initial_stats.armor_penetration_rating < 600 ) ) action_list_str += "/arcane_shot";
      action_list_str += "/readiness,wait_for_rapid_fire=1";
      action_list_str += "/steady_shot";
      break;
    case TREE_SURVIVAL:
      action_list_str += "/aspect";
      action_list_str += "/rapid_fire";
      action_list_str += "/kill_command";
      action_list_str += "/kill_shot";
      if ( talents.explosive_shot && talents.lock_and_load > 0 )
      {
        action_list_str += "/explosive_shot,if=buff.lock_and_load.react";
        action_list_str += "/explosive_shot,if=!buff.lock_and_load.up";
      }
      else if ( talents.explosive_shot && talents.lock_and_load == 0 )
      {
        action_list_str += "/explosive_shot";
      }
      if ( talents.black_arrow ) action_list_str += "/black_arrow";
      action_list_str += "/serpent_sting";
      if ( talents.aimed_shot )
      {
        action_list_str += "/aimed_shot";
      }
      else
      {
        action_list_str += "/multi_shot";
      }
      if ( ! talents.explosive_shot && talents.lock_and_load > 0 ) 
      {
        action_list_str += "/arcane_shot,if=buff.lock_and_load.react";
        action_list_str += "/arcane_shot,if=!buff.lock_and_load.up";
      }
      else if ( ! talents.explosive_shot && talents.lock_and_load == 0 )
      {
        action_list_str += "/arcane_shot";
      }
      action_list_str += "/steady_shot";
      break;
    }

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

// hunter_t::combat_begin =====================================================

void hunter_t::combat_begin()
{
  player_t::combat_begin();

  sim -> auras.ferocious_inspiration -> trigger( 1, talents.ferocious_inspiration, talents.ferocious_inspiration > 0 );
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

  ap += buffs_aspect_of_the_hawk -> value();
  ap += intellect() * talents.careful_aim / 3.0;
  ap += stamina() * talents.hunter_vs_wild * 0.1;

  ap += buffs_furious_howl -> value();

  if ( buffs_expose_weakness -> up() ) ap += agility() * 0.25;

  return ap;
}

// hunter_t::composite_attack_power_multiplier =================================

double hunter_t::composite_attack_power_multiplier() SC_CONST
{
  double mult = player_t::composite_attack_power_multiplier();

  if ( active_aspect == ASPECT_BEAST )
  {
    mult *= 1.10 + glyphs.the_beast * 0.02;
  }
  if ( buffs_tier10_4pc -> up() )
  {
    mult *= 1.20;
  }
  if ( buffs_call_of_the_wild -> up() )
  {
    mult *= 1.1;
  }

  return mult;
}

// hunter_t::composite_attack_hit ============================================

double hunter_t::composite_attack_hit() SC_CONST
{
  double ah = player_t::composite_attack_hit();

  if ( talents.focused_aim ) ah += talents.focused_aim * 0.01;

  return ah;
}

// hunter_t::composite_attack_crit ===========================================

double hunter_t::composite_attack_crit() SC_CONST
{
  double ac = player_t::composite_attack_crit();

  if ( talents.master_marksman ) ac += talents.master_marksman * 0.01;
  if ( talents.killer_instinct ) ac += talents.killer_instinct * 0.01;

  return ac;
}

// hunter_t::regen  =======================================================

void hunter_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  if ( buffs_aspect_of_the_viper -> check() )
  {
    double aspect_of_the_viper_regen = periodicity * 0.04 * resource_max[ RESOURCE_MANA ] / 3.0;

    resource_gain( RESOURCE_MANA, aspect_of_the_viper_regen, gains_viper_aspect_passive );
  }
  if ( buffs_rapid_fire -> check() && talents.rapid_recuperation )
  {
    double rr_regen = periodicity * 0.02 * talents.rapid_recuperation * resource_max[ RESOURCE_MANA ] / 3.0;

    resource_gain( RESOURCE_MANA, rr_regen, gains_rapid_recuperation );
  }
}

// hunter_t::get_cooldown ==================================================

cooldown_t* hunter_t::get_cooldown( const std::string& name )
{
  if ( name == "arcane_shot"    ) return player_t::get_cooldown( "arcane_explosive" );
  if ( name == "explosive_shot" ) return player_t::get_cooldown( "arcane_explosive" );

  if ( name == "aimed_shot" ) return player_t::get_cooldown( "aimed_multi" );
  if ( name == "multi_shot" ) return player_t::get_cooldown( "aimed_multi" );

  return player_t::get_cooldown( name );
}

// hunter_t::get_talent_trees ==============================================

std::vector<talent_translation_t>& hunter_t::get_talent_list()
{
  if(talent_list.empty())
  {
	  talent_translation_t translation_table[][MAX_TALENT_TREES] =
	  {
    { {  1, 5, &( talents.improved_aspect_of_the_hawk ) }, {  1, 0, NULL                                      }, {  1, 5, &( talents.improved_tracking        ) } },
    { {  2, 5, &( talents.endurance_training          ) }, {  2, 3, &( talents.focused_aim                  ) }, {  2, 0, NULL                                  } },
    { {  3, 2, &( talents.focused_fire                ) }, {  3, 5, &( talents.lethal_shots                 ) }, {  3, 2, &( talents.savage_strikes           ) } },
    { {  4, 0, NULL                                     }, {  4, 3, &( talents.careful_aim                  ) }, {  4, 0, NULL                                  } },
    { {  5, 3, &( talents.thick_hide                  ) }, {  5, 3, &( talents.improved_hunters_mark        ) }, {  5, 0, NULL                                  } },
    { {  6, 0, NULL                                     }, {  6, 5, &( talents.mortal_shots                 ) }, {  6, 3, &( talents.trap_mastery             ) } },
    { {  7, 0, NULL                                     }, {  7, 2, &( talents.go_for_the_throat            ) }, {  7, 2, &( talents.survival_instincts       ) } },
    { {  8, 1, &( talents.aspect_mastery              ) }, {  8, 3, &( talents.improved_arcane_shot         ) }, {  8, 5, &( talents.survivalist              ) } },
    { {  9, 5, &( talents.unleashed_fury              ) }, {  9, 1, &( talents.aimed_shot                   ) }, {  9, 1, &( talents.scatter_shot             ) } },
    { { 10, 0, NULL                                     }, { 10, 2, &( talents.rapid_killing                ) }, { 10, 0, NULL                                  } },
    { { 11, 5, &( talents.ferocity                    ) }, { 11, 3, &( talents.improved_stings              ) }, { 11, 2, &( talents.survival_tactics         ) } },
    { { 12, 0, NULL                                     }, { 12, 3, &( talents.efficiency                   ) }, { 12, 3, &( talents.tnt                      ) } },
    { { 13, 0, NULL                                     }, { 13, 0, NULL                                      }, { 13, 3, &( talents.lock_and_load            ) } },
    { { 14, 2, &( talents.bestial_discipline          ) }, { 14, 1, &( talents.readiness                    ) }, { 14, 3, &( talents.hunter_vs_wild           ) } },
    { { 15, 2, &( talents.animal_handler              ) }, { 15, 3, &( talents.barrage                      ) }, { 15, 3, &( talents.killer_instinct          ) } },
    { { 16, 5, &( talents.frenzy                      ) }, { 16, 2, &( talents.combat_experience            ) }, { 16, 0, NULL                                  } },
    { { 17, 3, &( talents.ferocious_inspiration       ) }, { 17, 3, &( talents.ranged_weapon_specialization ) }, { 17, 5, &( talents.lightning_reflexes       ) } },
    { { 18, 1, &( talents.bestial_wrath               ) }, { 18, 3, &( talents.piercing_shots               ) }, { 18, 3, &( talents.resourcefulness          ) } },
    { { 19, 3, &( talents.catlike_reflexes            ) }, { 19, 1, &( talents.trueshot_aura                ) }, { 19, 3, &( talents.expose_weakness          ) } },
    { { 20, 2, &( talents.invigoration                ) }, { 20, 3, &( talents.improved_barrage             ) }, { 20, 0, NULL                                  } },
    { { 21, 5, &( talents.serpents_swiftness          ) }, { 21, 5, &( talents.master_marksman              ) }, { 21, 3, &( talents.thrill_of_the_hunt       ) } },
    { { 22, 3, &( talents.longevity                   ) }, { 22, 2, &( talents.rapid_recuperation           ) }, { 22, 5, &( talents.master_tactician         ) } },
    { { 23, 1, &( talents.beast_within                ) }, { 23, 3, &( talents.wild_quiver                  ) }, { 23, 3, &( talents.noxious_stings           ) } },
    { { 24, 3, &( talents.cobra_strikes               ) }, { 24, 1, &( talents.silencing_shot               ) }, { 24, 0, NULL                                  } },
    { { 25, 5, &( talents.kindred_spirits             ) }, { 25, 3, &( talents.improved_steady_shot         ) }, { 25, 1, &( talents.black_arrow              ) } },
    { { 26, 1, &( talents.beast_mastery               ) }, { 26, 5, &( talents.marked_for_death             ) }, { 26, 3, &( talents.sniper_training          ) } },
    { {  0, 0, NULL                                     }, { 27, 1, &( talents.chimera_shot                 ) }, { 27, 3, &( talents.hunting_party            ) } },
    { {  0, 0, NULL                                     }, {  0, 0, NULL                                      }, { 28, 1, &( talents.explosive_shot           ) } },
    { {  0, 0, NULL                                     }, {  0, 0, NULL                                      }, {  0, 0, NULL                                  } }
  };

    util_t::translate_talent_trees( talent_list, translation_table, sizeof( translation_table) );
  }
  return talent_list;
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
      { "endurance_training",                OPT_INT, &( talents.endurance_training           ) },
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
      { "thick_hide",                        OPT_INT, &( talents.thick_hide                   ) },
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

// hunter_t::create_profile =================================================

bool hunter_t::create_profile( std::string& profile_str, int save_type )
{
  player_t::create_profile( profile_str, save_type );

  if ( save_type == SAVE_ALL || save_type == SAVE_GEAR )
  {
    if ( ammo_dps != 0 ) profile_str += "ammo_dps=" + util_t::to_string( ammo_dps, 2 ) + "\n";
  }

  return true;
}

// hunter_t::decode_set =====================================================

int hunter_t::decode_set( item_t& item )
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

  if ( strstr( s, "cryptstalker"          ) ) return SET_T7_MELEE;
  if ( strstr( s, "scourgestalker"        ) ) return SET_T8_MELEE;
  if ( strstr( s, "windrunners"           ) ) return SET_T9_MELEE;
  if ( strstr( s, "ahnkahar_blood_hunter" ) ) return SET_T10_MELEE;

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


// player_t::hunter_init ====================================================

void player_t::hunter_init( sim_t* sim )
{
  sim -> auras.trueshot              = new aura_t( sim, "trueshot" );
  sim -> auras.ferocious_inspiration = new aura_t( sim, "ferocious_inspiration" );

  target_t* t = sim -> target;
  t -> debuffs.hunters_mark  = new debuff_t( sim, "hunters_mark",  1, 300.0 );
  t -> debuffs.scorpid_sting = new debuff_t( sim, "scorpid_sting", 1, 20.0  );
}

// player_t::hunter_combat_begin ============================================

void player_t::hunter_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.trueshot_aura )         sim -> auras.trueshot -> override();
  if ( sim -> overrides.ferocious_inspiration ) sim -> auras.ferocious_inspiration -> override( 1, 3 );

  target_t* t = sim -> target;
  if ( sim -> overrides.hunters_mark ) t -> debuffs.hunters_mark -> override( 1, 500 * 1.5 );
}

