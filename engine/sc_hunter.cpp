// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Hunter
// ==========================================================================

struct ferocity_pet_t;
struct hunter_pet_t;

enum aspect_type
{ ASPECT_NONE=0, ASPECT_HAWK, ASPECT_FOX, ASPECT_MAX };

struct hunter_t : public player_t
{
  // Active
  hunter_pet_t* active_pet;
  int           active_aspect;
  action_t*     active_piercing_shots;

  dot_t* dots_serpent_sting;

  cooldown_t* cooldowns_glyph_kill_shot;

  proc_t* procs_wild_quiver;

  // Buffs
  buff_t* buffs_aspect_of_the_hawk;
  buff_t* buffs_beast_within;
  buff_t* buffs_call_of_the_wild;
  buff_t* buffs_cobra_strikes;
  buff_t* buffs_culling_the_herd;
  buff_t* buffs_improved_steady_shot;
  buff_t* buffs_lock_and_load;
  buff_t* buffs_rapid_fire;
  buff_t* buffs_trueshot_aura;
  buff_t* buffs_tier10_2pc;
  buff_t* buffs_tier10_4pc;
  buff_t* buffs_master_marksman;
  buff_t* buffs_pre_improved_steady_shot;
  buff_t* buffs_sniper_training;

  // Gains
  gain_t* gains_glyph_of_arcane_shot;
  gain_t* gains_invigoration;
  gain_t* gains_fervor;
  gain_t* gains_rapid_recuperation;
  gain_t* gains_roar_of_recovery;
  gain_t* gains_thrill_of_the_hunt;
  gain_t* gains_steady_shot;
  gain_t* gains_glyph_aimed_shot;
  gain_t* gains_cobra_shot;
  gain_t* gains_tier11_4pc;


  // Random Number Generation
  rng_t* rng_frenzy;
  rng_t* rng_invigoration;
  rng_t* rng_owls_focus;
  rng_t* rng_rabid_power;
  rng_t* rng_thrill_of_the_hunt;
  rng_t* rng_tier11_4pc;

  // Custom Parameters
  std::string summon_pet_str;

  struct talents_t
  {
    // Beast Mastery
    talent_t* improved_kill_command;
    talent_t* one_with_nature;
    talent_t* bestial_discipline;
    talent_t* pathfinding;
    talent_t* spirit_bond;
    talent_t* frenzy;
    talent_t* improved_mend_pet;
    talent_t* cobra_strikes;
    talent_t* fervor;
    talent_t* focus_fire; // implement
    talent_t* longevity;
    talent_t* killing_streak; // implement
    talent_t* crouching_tiger_hidden_chimera;
    talent_t* bestial_wrath;
    talent_t* ferocious_inspiration;
    talent_t* kindred_spirits;
    talent_t* the_beast_within;
    talent_t* invigoration;
    talent_t* beast_mastery;

    // Marksmanship
    talent_t* go_for_the_throat;
    talent_t* efficiency;
    talent_t* rapid_killing;
    talent_t* sic_em;
    talent_t* improved_steady_shot;
    talent_t* careful_aim;
    talent_t* silencing_shot;
    talent_t* concussive_barrage;
    talent_t* piercing_shots;
    talent_t* bombardment;
    talent_t* trueshot_aura;
    talent_t* termination;
    talent_t* resistance_is_futile;
    talent_t* rapid_recuperation;
    talent_t* master_marksman;
    talent_t* readiness; // check
    talent_t* posthaste;
    talent_t* marked_for_death;
    talent_t* chimera_shot;
    
    // Survival
    talent_t* hunter_vs_wild;
    talent_t* pathing;
    talent_t* improved_serpent_sting;
    talent_t* survival_tactics; // implement
    talent_t* trap_mastery;
    talent_t* entrapment;
    talent_t* point_of_no_escape;
    talent_t* thrill_of_the_hunt;
    talent_t* counterattack;
    talent_t* lock_and_load;
    talent_t* resourcefulness;
    talent_t* mirrored_blades;
    talent_t* tnt;
    talent_t* toxicology;
    talent_t* wyvern_sting;
    talent_t* noxious_stings;
    talent_t* hunting_party;
    talent_t* sniper_training; // implement for helter skelter
    talent_t* serpent_spread;
    talent_t* black_arrow;
  };
  talents_t talents;

  struct glyphs_t
  {
    // Prime
    glyph_t* aimed_shot;
    glyph_t* arcane_shot;
    glyph_t* chimera_shot;
    glyph_t* explosive_shot;
    glyph_t* kill_command;
    glyph_t* kill_shot;
    glyph_t* rapid_fire;
    glyph_t* serpent_sting;
    glyph_t* steady_shot;

    // Major
    glyph_t*  bestial_wrath;
  };
  glyphs_t glyphs;

  struct passive_spells_t
  {
    passive_spell_t* animal_handler;
    passive_spell_t* artisan_quiver;
    passive_spell_t* into_the_wildness;

    mastery_t* master_of_beasts;
    mastery_t* wild_quiver;
    mastery_t* essence_of_the_viper;
  };
  passive_spells_t passive_spells;


  hunter_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) : player_t( sim, HUNTER, name, r )
  {
    tree_type[ HUNTER_BEAST_MASTERY ] = TREE_BEAST_MASTERY;
    tree_type[ HUNTER_MARKSMANSHIP  ] = TREE_MARKSMANSHIP;
    tree_type[ HUNTER_SURVIVAL      ] = TREE_SURVIVAL;

    // Active
    active_pet             = 0;
    active_aspect          = ASPECT_NONE;
    active_piercing_shots  = 0;

    dots_serpent_sting = get_dot("serpent_sting");

    cooldowns_glyph_kill_shot = get_cooldown("cooldowns_glyph_kill_shot");
    cooldowns_glyph_kill_shot -> duration = 6.0;

    ranged_attack = 0;
    summon_pet_str = "cat";
    base_gcd = 1.0;

    create_talents();
    create_options();
  }

  // Character Definition
  virtual void      init_talents();
  virtual void      init_spells();
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
  virtual double    composite_attack_haste() SC_CONST;
  virtual double    agility() SC_CONST;
  virtual double    composite_player_multiplier( const school_type school ) SC_CONST;
  virtual double    matching_gear_multiplier( const attribute_type attr ) SC_CONST;
  virtual cooldown_t* get_cooldown( const std::string& name );
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet( const std::string& name );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_FOCUS; }
  virtual int       primary_role() SC_CONST     { return ROLE_ATTACK; }
  virtual bool      create_profile( std::string& profile_str, int save_type=SAVE_ALL );
  virtual void      armory_extensions( const std::string& r, const std::string& s, const std::string& c );

  // Event Tracking
  virtual void regen( double periodicity );

};

// ==========================================================================
// Hunter Pet
// ==========================================================================

struct hunter_pet_t : public pet_t
{

  // Buffs
  buff_t* buffs_bestial_wrath;
  buff_t* buffs_call_of_the_wild;
  buff_t* buffs_culling_the_herd;
  buff_t* buffs_frenzy;
  buff_t* buffs_monstrous_bite;
  buff_t* buffs_owls_focus;
  buff_t* buffs_rabid;
  buff_t* buffs_rabid_power_stack;
  buff_t* buffs_savage_rend;
  buff_t* buffs_wolverine_bite;
  buff_t* buffs_sic_em;

  // Gains
  gain_t* gains_go_for_the_throat;
  gain_t* gains_fervor;


  hunter_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name, pet_type_t pt ) :
      pet_t( sim, owner, pet_name, pt )
  {
    if ( ! supported( pet_type ) )
    {
      sim -> errorf( "Pet %s is not yet supported.\n", pet_name.c_str() );
      sim -> cancel();
    }

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 51.0; // FIXME only level 80 value
    main_hand_weapon.max_dmg    = 78.0; // FIXME only level 80 value
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2; // FIXME only level 80 value
    main_hand_weapon.swing_time = 2.0;

    stamina_per_owner = 0.45;

    health_per_stamina *= 1.05; // 3.1.0 change # Cunning, Ferocity and Tenacity pets now all have +5% damage, +5% armor and +5% health bonuses


    if ( group() == PET_CUNNING )
    {


      if ( pet_type == PET_WIND_SERPENT )
      {
        action_list_str = "auto_attack/roar_of_recovery/wolverine_bite/lightning_breath/bite";
      }
      else assert( 0 );
    }

    create_talents();
    create_options();
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

  virtual int group()
  {
    assert( pet_type > PET_NONE && pet_type < PET_HUNTER );
    if ( pet_type >= PET_CARRION_BIRD && pet_type <= PET_WOLF         ) return PET_FEROCITY;
    if ( pet_type >= PET_BEAR         && pet_type <= PET_WORM         ) return PET_TENACITY;
    if ( pet_type >= PET_BAT          && pet_type <= PET_WIND_SERPENT ) return PET_CUNNING;
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

    base_attack_crit = 0.032;

    resource_base[ RESOURCE_HEALTH ] = rating_t::interpolate( level, 0, 4253, 6373 );
    resource_base[ RESOURCE_FOCUS ] = 100 + o -> talents.kindred_spirits -> effect_base_value( 1 );

    base_focus_regen_per_second  = ( 24.5 / 4.0 );

    base_focus_regen_per_second *= 1.0 + o -> talents.bestial_discipline -> effect_base_value( 1 ) / 100.0;

    base_gcd = 1.20;
  }

  virtual void init_buffs()
  {
    pet_t::init_buffs();
    buffs_bestial_wrath     = new buff_t( this, "bestial_wrath",     1, 10.0 );
    buffs_call_of_the_wild  = new buff_t( this, 53434, "call_of_the_wild" );
    buffs_culling_the_herd  = new buff_t( this, 70893, "culling_the_herd" );
    buffs_frenzy            = new buff_t( this, "frenzy",            5, 10.0, 0.0 );
    buffs_monstrous_bite    = new buff_t( this, "monstrous_bite",    3, 12.0 );
    buffs_owls_focus        = new buff_t( this, "owls_focus",        1,  8.0, 0.0, 0 );
    buffs_rabid             = new buff_t( this, "rabid",             1, 20.0 );
    buffs_rabid_power_stack = new buff_t( this, "rabid_power_stack", 1,    0, 0.0);
    buffs_savage_rend       = new buff_t( this, "savage_rend",       1, 30.0 );
    buffs_wolverine_bite    = new buff_t( this, "wolverine_bite",    1, 10.0, 0.0 );
    buffs_sic_em = new buff_t( this, "sic_em", 1, 12.0 );
  }

  virtual void init_gains()
  {
    pet_t::init_gains();
    gains_go_for_the_throat = get_gain( "go_for_the_throat" );
    gains_fervor            = get_gain( "fervor" );
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

    ap += o -> composite_attack_power() * 0.22;

    return ap;
  }

  virtual double composite_attack_power_multiplier() SC_CONST
  {
    hunter_t* o = owner -> cast_hunter();

    double mult = player_t::composite_attack_power_multiplier();

    if ( buffs_rabid -> up() )
      mult *= 1.0 + buffs_rabid_power_stack -> stack() * 0.05;

    if ( buffs_call_of_the_wild -> up() )
      mult *= 1.0 + buffs_call_of_the_wild -> effect_base_value( 1 ) / 100.0;

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

  virtual action_t* create_action( const std::string& name, const std::string& options_str );
  virtual double composite_player_multiplier( const school_type school ) SC_CONST
      {
        double m = pet_t::composite_player_multiplier( school );
        hunter_t*     o = owner -> cast_hunter();
        if ( o -> passive_spells.master_of_beasts -> ok() )
        {
          m *= 1.0 + 1.7 / 100.0 * o -> composite_mastery();
        }
        return m;
      }

};

// Pet Feriocity

struct ferocity_pet_t : public hunter_pet_t
{

  struct talents_t
   {
     talent_t* serpent_swiftness;
     talent_t* dash;
     talent_t* great_stamina;
     talent_t* natural_armor;
     talent_t* improved_cower;
     talent_t* bloodthirsty;
     talent_t* spiked_collar;
     talent_t* boars_speed;
     talent_t* culling_the_herd;
     talent_t* lionhearted;
     talent_t* heart_of_the_phoenix;
     talent_t* spiders_bite;
     talent_t* great_resistance;
     talent_t* rabid;
     talent_t* lick_your_wounds;
     talent_t* call_of_the_wild;
     talent_t* shark_attack;
     talent_t* wild_hunt;
   };
   talents_t talents;

  ferocity_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name, pet_type_t pt ) :
    hunter_pet_t( sim,owner,pet_name,pt )
  {
  }

  virtual void init_base()
  {
    hunter_pet_t::init_base();
  }

  virtual void init_talents()
  {
    talents.serpent_swiftness     = new talent_t ( this, "Serpent Swiftness" );
    talents.dash                  = new talent_t ( this, "Dash" );
    talents.great_stamina         = new talent_t ( this, "Great Stamina" );
    talents.natural_armor         = new talent_t ( this, "Natural Armor" );
    talents.improved_cower        = new talent_t ( this, "Improved Cower" );
    talents.bloodthirsty          = new talent_t ( this, "Bloodthirsty" );
    talents.spiked_collar         = new talent_t ( this, "Spiked Collar" );
    talents.boars_speed           = new talent_t ( this, "Boars Speed" );
    talents.culling_the_herd      = new talent_t ( this, "Culling the Herd" );
    talents.lionhearted           = new talent_t ( this, "Lionhearted" );
    talents.heart_of_the_phoenix  = new talent_t ( this, "Heart of the Phoenix" );
    talents.spiders_bite          = new talent_t ( this, "Spiders Bite" );
    talents.great_resistance      = new talent_t ( this, "Great Resistance" );
    talents.rabid                 = new talent_t ( this, "Rabid" );
    talents.lick_your_wounds      = new talent_t ( this, "Lick your Wounds" );
    talents.call_of_the_wild      = new talent_t ( this, "Call of the Wild" );
    talents.shark_attack          = new talent_t ( this, "Shark Attack" );
    talents.wild_hunt             = new talent_t ( this, "Wild Hunt" );

    hunter_pet_t::init_talents();

    if ( false /* no talents set */ )
    {
      talents.spiked_collar -> set_rank( 3, true );
    }
  }
  virtual void init_buffs()
  {
     hunter_pet_t::init_buffs();
     buffs_culling_the_herd -> current_value = talents.culling_the_herd -> rank();
  }
  virtual void init_actions()
  {
    if ( action_list_str.empty() )
    {
      if ( pet_type == PET_CAT )
      {
	action_list_str = "auto_attack/call_of_the_wild/rabid/claw/wait_until_ready,sec=3";
      }
      else if ( pet_type == PET_DEVILSAUR )
      {
	action_list_str = "auto_attack/monstrous_bite/call_of_the_wild/rabid/bite/wait_until_ready,sec=3";
      }
      else if ( pet_type == PET_RAPTOR )
      {
	action_list_str = "auto_attack/savage_rend/call_of_the_wild/rabid/claw/wait_until_ready,sec=3";
      }
      else if ( pet_type == PET_WOLF )
      {
	action_list_str = "auto_attack/furious_howl/call_of_the_wild/rabid/bite/wait_until_ready,sec=3";
      }
      action_list_default = 1;
    }

    hunter_pet_t::init_actions();
  }


  virtual action_t* create_action( const std::string& name, const std::string& options_str );
};

// ==========================================================================
// Hunter Attack
// ==========================================================================

struct hunter_attack_t : public attack_t
{
  void _init_hunter_attack_t()
  {
    range = -1;
    may_crit               = true;
    tick_may_crit          = true;
    normalize_weapon_speed = true;
  }
  hunter_attack_t( const char* n, player_t* player, const school_type s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true ) :
      attack_t( n, player, RESOURCE_FOCUS, s, t, special )
  {
    _init_hunter_attack_t();
  }
  hunter_attack_t( const char* n, player_t* player, const char* sname, int t=TREE_NONE, bool special=true ) :
      attack_t( n, sname, player, t, special )
  {
    _init_hunter_attack_t();
  }

  hunter_attack_t( const char* n, player_t* player, const uint32_t id, int t=TREE_NONE, bool special=true ) :
      attack_t( n, id, player, t, special )
  {
    _init_hunter_attack_t();
  }
  virtual void trigger_improved_steady_shot()
  {
    hunter_t* p = player -> cast_hunter();
    p -> buffs_pre_improved_steady_shot -> expire();
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
  virtual void execute()
  {
    attack_t::execute();
    hunter_t* p = player -> cast_hunter();

      if ( p -> talents.improved_steady_shot -> rank() )
        trigger_improved_steady_shot();
      if ( p -> buffs_pre_improved_steady_shot -> stack() == 2 )
        p -> buffs_improved_steady_shot -> trigger();

  }

  virtual double cost() SC_CONST;
  virtual double execute_time() SC_CONST;
  virtual void   player_buff();
};

// ==========================================================================
// Hunter Spell
// ==========================================================================

struct hunter_spell_t : public spell_t
{
  hunter_spell_t( const char* n, player_t* p, const school_type s, int t=TREE_NONE ) :
      spell_t( n, p, RESOURCE_FOCUS, s, t )
  {}
  hunter_spell_t( const char* n, player_t* p, const char* sname ) :
      spell_t( n, sname, p )
  {}
  hunter_spell_t( const char* n, player_t* p, const uint32_t id ) :
      spell_t( n, id, p )
  {}

  virtual double gcd() SC_CONST;
};

namespace { // ANONYMOUS NAMESPACE =========================================

// trigger_go_for_the_throat ===============================================

static void trigger_go_for_the_throat( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.go_for_the_throat -> rank() ) return;
  if ( ! p -> active_pet ) return;

  double gain = p -> talents.go_for_the_throat -> rank() * 5.0;

  p -> active_pet -> resource_gain( RESOURCE_FOCUS, gain, p -> active_pet -> gains_go_for_the_throat );
}


// trigger_invigoration ==============================================

static void trigger_invigoration( action_t* a )
{
  if ( a -> special ) return;

  hunter_pet_t* p = ( hunter_pet_t* ) a -> player -> cast_pet();
  hunter_t*     o = p -> owner -> cast_hunter();

  if ( ! o -> talents.invigoration -> rank() )
    return;

  double gain = o -> talents.invigoration -> rank()* 3.0;

  o -> resource_gain( RESOURCE_FOCUS, gain, o -> gains_invigoration );
}

// trigger_piercing_shots

static void trigger_piercing_shots( action_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.piercing_shots -> rank() )
    return;

  struct piercing_shots_t : public attack_t
  {
    piercing_shots_t( player_t* p ) : attack_t( "piercing_shots", p, 63468 )
    {
      may_miss     = false;
      may_crit     = true;
      background   = true;
      proc         = true;
      hasted_ticks = false;

      base_multiplier = 1.0;
      tick_power_mod = 0;
      num_ticks = 8;
      base_tick_time = 1.0;
    }
    void player_buff() {}
    void target_debuff( int dmg_type )
    {
      target_t* t = target;
      if ( t -> debuffs.mangle -> up() || t -> debuffs.blood_frenzy_bleed -> up() )
      {
        target_multiplier = 1.30;
      }
    }
  };

  double dmg = p -> talents.piercing_shots -> rank() * 0.1 * a -> direct_dmg;

  if ( ! p -> active_piercing_shots ) p -> active_piercing_shots = new piercing_shots_t( p );

  dot_t* dot = p -> active_piercing_shots -> dot;

  if ( dot -> ticking )
  {
    dmg += p -> active_piercing_shots -> base_td * dot -> ticks();

    p -> active_piercing_shots -> cancel();
  }
  p -> active_piercing_shots -> base_td = dmg / p -> active_piercing_shots -> num_ticks;
  p -> active_piercing_shots -> execute();
}

// trigger_thrill_of_the_hunt ========================================

static void trigger_thrill_of_the_hunt( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.thrill_of_the_hunt -> rank() )
    return;
  if ( p -> rng_thrill_of_the_hunt -> roll ( p -> talents.thrill_of_the_hunt -> proc_chance() ) )
  {
  double gain = util_t::talent_rank(  p -> talents.thrill_of_the_hunt -> rank(), 3, 0.10, 0.20, 0.40 );
  gain *= a -> resource_consumed;
  
  p -> resource_gain( RESOURCE_FOCUS, gain, p -> gains_thrill_of_the_hunt );
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

} // ANONYMOUS NAMESPACE ====================================================

// =========================================================================
// Hunter Pet Attacks
// =========================================================================

struct hunter_pet_attack_t : public attack_t
{
  void _init_hunter_pet_attack_t()
  {
        hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
        hunter_t*     o = p -> owner -> cast_hunter();
        may_crit = true;

        base_multiplier *= 1.05; // Cunning, Ferocity and Tenacity pets all have +5% damag

        // Orc Command Racial
        if ( o -> race == RACE_ORC )
        {
          base_multiplier *= 1.05;
        }
        //base_multiplier *= 1.0 + p -> cast_ferocity_pet() -> talents.spiked_collar -> effect_base_value( 1 ) / 100.0;

        // Assume happy pet
        base_multiplier *= 1.25;

  }

  hunter_pet_attack_t( const char* n, hunter_pet_t* player, const char* sname, bool special=true ) :
      attack_t( n, sname, player, TREE_BEAST_MASTERY, special )
  {
    _init_hunter_pet_attack_t();
  }
  hunter_pet_attack_t( const char* n, const uint32_t id, hunter_pet_t* player, bool special=true ) :
      attack_t( n, id, player, TREE_BEAST_MASTERY, special )
  {
    _init_hunter_pet_attack_t();
  }

  virtual double execute_time() SC_CONST
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    double t = attack_t::execute_time();
    if ( t == 0 ) return 0;

    if ( p -> buffs_frenzy -> up() ) t *= 1.0 / 1.3;

    return t;
  }


  virtual void execute()
  {
    attack_t::execute();
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    if ( attack_t::cost() > 0 ) p -> buffs_sic_em -> expire();
    if ( result_is_hit() )
    {
      if ( p -> buffs_rabid -> check() )
        p -> buffs_rabid_power_stack -> trigger();

      if ( result == RESULT_CRIT )
      {
        p -> buffs_frenzy -> trigger();
        trigger_invigoration( this );
        p -> buffs_wolverine_bite -> trigger();
      }



    }
    else
    {
      hunter_t* o = p -> owner -> cast_hunter();
      o -> buffs_cobra_strikes -> decrement();
    }
  }


  virtual void player_buff()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    attack_t::player_buff();

    if ( p -> buffs_bestial_wrath -> up() ) player_multiplier *= 1.50;

    player_multiplier *= 1.0 + p -> buffs_monstrous_bite -> stack() * 0.03;

    if ( p -> buffs_culling_the_herd -> up() )
      player_multiplier *= 1.0 + ( p -> buffs_culling_the_herd -> effect_base_value( 1 ) / 100.0 );

    if ( ! special )
    {
      if ( o -> buffs_cobra_strikes -> up() ) player_crit += o -> buffs_cobra_strikes -> effect_base_value( 1 ) / 100.0;
    }

    if ( o -> buffs_tier10_2pc -> up() )
      player_multiplier *= 1.15;
  }

  virtual double cost() SC_CONST
      {
      hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
      double c = attack_t::cost();
      if ( p -> buffs_sic_em -> up() && c > 0 ) return 0;
      return c;
      }
};

struct ferocity_pet_attack_t : public hunter_pet_attack_t
{
  void _init_ferocity_pet_attack_t()
  {
    ferocity_pet_t* p = (ferocity_pet_t*) player -> cast_pet();

    // Hack for testing talents
    base_multiplier *= 1.0 + p -> talents.spiked_collar -> effect_base_value( 1 ) * 100.0;
  }

  ferocity_pet_attack_t( const char* n, ferocity_pet_t* player, const char* sname, bool special=true ) :
    hunter_pet_attack_t( n, player, sname, special )
  {
    _init_ferocity_pet_attack_t();
  }
  ferocity_pet_attack_t( const char* n, const uint32_t id, ferocity_pet_t* player, bool special=true ) :
    hunter_pet_attack_t( n, id, player, special )
  {
    _init_ferocity_pet_attack_t();
  }

  virtual void execute()
  {
    hunter_pet_attack_t::execute();
    ferocity_pet_t* p = (ferocity_pet_t*) player -> cast_pet();
    if ( p -> talents.culling_the_herd -> rank() && result == RESULT_CRIT )
      p -> buffs_culling_the_herd -> trigger();

  }

};

// Pet Melee =================================================================

struct pet_melee_t : public hunter_pet_attack_t
{
  pet_melee_t( hunter_pet_t* player ) :
    hunter_pet_attack_t( "melee", player, 0, false )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    weapon = &( p -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    base_dd_min       = base_dd_max = 1;
    background        = true;
    repeating         = true;
    school = SCHOOL_PHYSICAL;
    stats -> school = school;

  }
};

// Pet Auto Attack ==========================================================

struct pet_auto_attack_t : public hunter_pet_attack_t
{
  pet_auto_attack_t( hunter_pet_t* player, const std::string& options_str ) :
    hunter_pet_attack_t( "auto_attack", player, 0 )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    p -> main_hand_attack = new pet_melee_t( player );
    trigger_gcd = 0;
    school = SCHOOL_PHYSICAL;
    stats -> school = school;
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

/*
struct bite_t : public hunter_pet_attack_t
{
  bite_t( player_t* player, const std::string& options_str ) :
    hunter_pet_attack_t( "bite", 17253, player )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
    {
     hunter_pet_attack_t::execute();

    }
};
*/
struct claw_t : public ferocity_pet_attack_t
{
  claw_t( ferocity_pet_t* player, const std::string& options_str ) :
    ferocity_pet_attack_t( "claw", 16827, player )
  {
    parse_options( NULL, options_str );
    direct_power_mod=0.4;
  }

  virtual void execute()
    {
    ferocity_pet_attack_t::execute();
    }
};

// Devilsaur Monstrous Bite ===================================================

struct monstrous_bite_t : public ferocity_pet_attack_t
{
  monstrous_bite_t( ferocity_pet_t* player, const std::string& options_str ) :
    ferocity_pet_attack_t( "monstrous_bite", player, "Monstrous Bite" )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    check_pet_type( this, PET_DEVILSAUR );

    parse_options( 0, options_str );

    base_dd_min = base_dd_max = 107;
    base_cost = 20;
    cooldown -> duration *=  ( 1.0 - o -> talents.longevity -> effect_base_value( 1 ) / 100.0 );
    auto_cast = true;

    id = 55499;
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    ferocity_pet_attack_t::execute();

    p -> buffs_monstrous_bite -> trigger();
  }
};

/*
// Wolverine Bite =============================================================

struct wolverine_bite_t : public hunter_pet_attack_t
{
  wolverine_bite_t( player_t* player, const std::string& options_str ) :
      hunter_pet_attack_t( "wolverine_bite", player, "Wolverine Bite" )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();


    parse_options( NULL, options_str );

    base_dd_min = base_dd_max  = 5 * p -> level;
    cooldown -> duration *=  ( 1.0 - o -> talents.longevity -> effect_base_value( 1 ) / 100.0 );
    auto_cast   = true;

    may_dodge = may_block = may_parry = false;

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
*/
// =========================================================================
// Hunter Pet Spells
// =========================================================================

struct hunter_pet_spell_t : public spell_t
{
  void _init_hunter_pet_spell_t()
  {
    base_multiplier *= 1.05; // 3.1.0 change: # Cunning, Ferocity and Tenacity pets now all have +5% damage, +5% armor and +5% health bonuses.

  }
  hunter_pet_spell_t( const char* n, player_t* player, const char* sname ) :
        spell_t( n, sname, player )
    {
      _init_hunter_pet_spell_t();
    }
  hunter_pet_spell_t( const char* n, player_t* player, const uint32_t id ) :
        spell_t( n, id, player )
    {
      _init_hunter_pet_spell_t();
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
    p -> buffs_owls_focus -> trigger();
  }

  virtual void player_buff()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    spell_t::player_buff();

    player_spell_power += 0.125 * o -> composite_attack_power();

    if ( p -> buffs_bestial_wrath -> up() ) player_multiplier *= 1.50;

    if ( p -> buffs_culling_the_herd -> up() )
      player_multiplier *= 1.0 + ( p -> buffs_culling_the_herd -> effect_base_value( 1 ) / 100.0 );


    if ( o -> buffs_tier10_2pc -> up() )
      player_multiplier *= 1.15;
  }
};

struct ferocity_pet_spell_t : public hunter_pet_spell_t
{
  void _init_ferocity_pet_spell_t()
  {

  }

  ferocity_pet_spell_t( const char* n, ferocity_pet_t* player, const char* sname ) :
    hunter_pet_spell_t( n, player, sname )
  {
    _init_ferocity_pet_spell_t();
  }
  ferocity_pet_spell_t( const char* n, const uint32_t id, ferocity_pet_t* player ) :
    hunter_pet_spell_t( n, player, id )
  {
    _init_ferocity_pet_spell_t();
  }

};
// Chimera Froststorm Breath ================================================

struct froststorm_breath_tick_t : public hunter_pet_spell_t
{
  froststorm_breath_tick_t( player_t* player ) :
      hunter_pet_spell_t( "froststorm_breath_tick", player, 95725 )
  {
    base_dd_min = base_dd_max = 150;
    direct_power_mod = 1.5 / 3.5;
    dual        = true;
    background  = true;
    direct_tick = true;
  }
  virtual void execute()
    {
      hunter_pet_spell_t::execute();
      tick_dmg = direct_dmg;
      update_stats( DMG_OVER_TIME );
    }
};

struct froststorm_breath_t : public hunter_pet_spell_t
{
  froststorm_breath_tick_t* froststorm_breath_tick;
  froststorm_breath_t( player_t* player, const std::string& options_str ) :
      hunter_pet_spell_t( "froststorm_breath", player, "Froststorm Breath" )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    check_pet_type( this, PET_CHIMERA );

    parse_options( NULL, options_str );

    cooldown -> duration *=  ( 1.0 - o -> talents.longevity -> effect_base_value( 1 ) / 100.0 );
    auto_cast = true;

    froststorm_breath_tick = new froststorm_breath_tick_t( p );
  }
  virtual void tick()
  {
    froststorm_breath_tick -> execute();
  }
};

// Wind Serpent Lightning Breath ================================================

struct lightning_breath_t : public hunter_pet_spell_t
{
  lightning_breath_t( player_t* player, const std::string& options_str ) :
      hunter_pet_spell_t( "lightning_breath", player, "Lightning Breath" )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    check_pet_type( this, PET_WIND_SERPENT );

    parse_options( 0, options_str );

    base_dd_min = base_dd_max = 100;
    base_cost = 20;
    direct_power_mod = 1.5 / 3.5;
    cooldown -> duration *=  ( 1.0 - o -> talents.longevity -> effect_base_value( 1 ) / 100.0 );
    auto_cast = true;

    id = 25012;
  }
};

// Call of the Wild ===========================================================

struct call_of_the_wild_t : public hunter_pet_spell_t
{
  call_of_the_wild_t( player_t* player, const std::string& options_str ) :
      hunter_pet_spell_t( "call_of_the_wild", player, "Call of the Wild" )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    //check_talent( p -> talents.call_of_the_wild );

    parse_options( NULL, options_str );


    cooldown -> duration *=  ( 1.0 - o -> talents.longevity -> effect_base_value( 1 ) / 100.0 );
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();
    o -> buffs_call_of_the_wild -> trigger();
    p -> buffs_call_of_the_wild -> trigger();
    hunter_pet_spell_t::execute();
  }
};

// Furious Howl ===============================================================

struct furious_howl_t : public hunter_pet_spell_t
{
  furious_howl_t( player_t* player, const std::string& options_str ) :
      hunter_pet_spell_t( "furious_howl", player, "Furious Howl" )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    check_pet_type( this, PET_WOLF );

    parse_options( NULL, options_str );

    cooldown -> duration *=  ( 1.0 - o -> talents.longevity -> effect_base_value( 1 ) / 100.0 );

    harmful = false;

  }

  virtual void execute()
  {
    for ( player_t* pl = sim -> player_list; pl; pl = pl -> next )
    {
      pl -> buffs.furious_howl -> trigger();
    }
    hunter_pet_spell_t::execute();
  }
};

// Rabid ======================================================================

struct rabid_t : public hunter_pet_spell_t
{
  rabid_t( player_t* player, const std::string& options_str ) :
      hunter_pet_spell_t( "rabid", player, 53401 )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    //check_talent( p -> talents.rabid );

    parse_options( NULL, options_str );
    cooldown -> duration *=  ( 1.0 - o -> talents.longevity -> effect_base_value( 1 ) / 100.0 );

  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    p -> buffs_rabid_power_stack -> expire();
    p -> buffs_rabid -> trigger();

   hunter_pet_spell_t::execute();
  }
};

// Roar of Recovery ===========================================================

struct roar_of_recovery_t : public hunter_pet_spell_t
{
  roar_of_recovery_t( player_t* player, const std::string& options_str ) :
      hunter_pet_spell_t( "roar_of_recovery", player, "Roar of Recovery" )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    //check_talent( p -> talents.roar_of_recovery );

    parse_options( 0, options_str );

    trigger_gcd    = 0.0;
    base_cost      = 0;
    num_ticks      = 3;
    base_tick_time = 3;
    cooldown -> duration *=  ( 1.0 - o -> talents.longevity -> effect_base_value( 1 ) / 100.0 );
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



// =========================================================================
// Hunter Attacks
// =========================================================================

// hunter_attack_t::cost ===================================================

double hunter_attack_t::cost() SC_CONST
{
  hunter_t* p = player -> cast_hunter();
  double c = attack_t::cost();
  if ( c == 0 ) return 0;
  if ( p -> buffs_beast_within -> up() ) c *= ( 1.0 + p -> buffs_beast_within -> effect_base_value( 1 ) / 100.0 );
  return c;
}


// hunter_attack_t::execute_time ============================================

double hunter_attack_t::execute_time() SC_CONST
{
  hunter_t* p = player -> cast_hunter();

  double t = attack_t::execute_time();
  if ( t == 0  || base_execute_time < 0 ) return 0;


  t *= 1.0 / ( 1.0 + p -> buffs_rapid_fire -> value() );

  return t;
}

// hunter_attack_t::player_buff =============================================

void hunter_attack_t::player_buff()
{
  hunter_t* p = player -> cast_hunter();
  //target_t* t = target;

  attack_t::player_buff();


  if (  p -> buffs_beast_within -> up() )
  {
    player_multiplier *= 1.0 + p -> buffs_beast_within -> effect_base_value( 2 ) / 100.0;
  }
  if ( p -> dots_serpent_sting -> ticking && p -> talents.noxious_stings -> rank() )
  {
    player_multiplier *= 1.0 + p -> talents.noxious_stings -> rank() * 0.05;
  }

  if ( p -> buffs_culling_the_herd -> up() )
    player_multiplier *= 1.0 + ( p -> buffs_culling_the_herd -> effect_base_value( 1 ) / 100.0 );

  if ( p -> buffs_tier10_2pc -> up() )
    player_multiplier *= 1.15;
}

// Ranged Attack ===========================================================

struct ranged_t : public hunter_attack_t
{
  // FIXME! Setting "special=true" would create the desired 2-roll effect,
  // but it would also triger Honor-Among-Thieves procs.......
  ranged_t( player_t* player, const char* name="ranged" ) :
      hunter_attack_t( name, player, SCHOOL_PHYSICAL, TREE_NONE, /*special*/false )
  {
    hunter_t* p = player -> cast_hunter();

        weapon = &( p -> ranged_weapon );
        base_execute_time = weapon -> swing_time;

        may_crit    = true;
        background  = true;
        repeating   = true;
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
      p -> buffs_tier10_2pc -> trigger();
      if ( result == RESULT_CRIT ) 
        trigger_go_for_the_throat( this );
    }
  }

  virtual void player_buff()
  {
    hunter_attack_t::player_buff();
    hunter_t* p = player -> cast_hunter();
    if ( p -> passive_spells.artisan_quiver -> ok() )
    {
      player_multiplier *= 1.15;
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

  virtual double execute_time() SC_CONST
  {
    double h = 1.0;
    h *= 1.0 / ( 1.0 + std::max( sim -> auras.windfury_totem -> value(), sim -> auras.improved_icy_talons -> value() ) );
    return hunter_attack_t::execute_time() * h;
  }
};

// Aimed Shot ================================================================

struct aimed_shot_t : public hunter_attack_t
{
  aimed_shot_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "aimed_shot", player, "Aimed Shot" )
  {
    hunter_t* p = player -> cast_hunter();

    assert ( p -> primary_tree() == TREE_MARKSMANSHIP );

    parse_options( NULL, options_str );

    direct_power_mod = 0.48;

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );
    weapon_multiplier = effect_average( 2 ) / 100.0;

    normalize_weapon_speed = true;
    // FIXME! #1 haste lowers cd!
    // FIXME! #2 stop autoattacking during cast!
  }

  virtual void player_buff()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::player_buff();
    if ( p -> talents.careful_aim -> rank() && target -> health_percentage() > 80 )
    {
      player_crit += p -> talents.careful_aim -> effect_base_value( 1 ) / 100.0;
    }
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
     hunter_t* p = player -> cast_hunter();
    if ( result == RESULT_CRIT )
    {
      p -> active_pet -> buffs_sic_em -> trigger();
      trigger_piercing_shots( this );
      p -> resource_gain( RESOURCE_FOCUS, p -> glyphs.aimed_shot -> effect_base_value( 1 ), p -> gains_glyph_aimed_shot );
    }
    if ( p -> buffs_master_marksman -> check() == 5 )
      p -> buffs_master_marksman -> expire();
  }

  virtual double cost() SC_CONST
  {
    hunter_t* p = player -> cast_hunter();
    if ( p -> buffs_master_marksman -> check() == 5 )
      return 0;
    return hunter_attack_t::cost();
  }
  virtual double execute_time() SC_CONST
      {
    hunter_t* p = player -> cast_hunter();
    if ( p -> buffs_master_marksman -> check() == 5 )
      return 0;
    return hunter_attack_t::execute_time();
      }
};

// Arcane Shot Attack =========================================================

struct arcane_shot_t : public hunter_attack_t
{
  arcane_shot_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "arcane_shot", player, "Arcane Shot" )
  {
    hunter_t* p = player -> cast_hunter();

    parse_options( NULL, options_str );

    base_cost   -= p -> talents.efficiency -> rank();

    // To trigger ppm-based abilities
    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );
    weapon_multiplier = effect_average( 2 ) / 100.0;

    direct_power_mod = 0.042;

    player_multiplier *= 1.0 + p -> glyphs.arcane_shot -> effect_base_value( 1 ) / 100.0;
  }

  virtual double cost() SC_CONST
  {
    hunter_t* p = player -> cast_hunter();
    if ( p -> buffs_lock_and_load -> check() ) return 0;
    return hunter_attack_t::cost();
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    hunter_t* p = player -> cast_hunter();
    trigger_thrill_of_the_hunt( this );
    if ( result == RESULT_CRIT )
    {
      p -> active_pet -> buffs_sic_em -> trigger();
      p -> buffs_cobra_strikes -> trigger( 2 );
    }
    p -> buffs_lock_and_load -> decrement();
  }
};

// Black Arrow =================================================================

struct black_arrow_t : public hunter_attack_t
{
  black_arrow_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "black_arrow", player, "Black Arrow" )
  {
    hunter_t* p = player -> cast_hunter();

    check_talent( p -> talents.black_arrow -> rank() );

    parse_options( NULL, options_str );

    cooldown = p -> get_cooldown( "traps" );
    cooldown -> duration = spell_id_t::cooldown();
    cooldown -> duration -= p -> talents.resourcefulness -> rank() * 2;

    base_multiplier *= 1.0 + p -> talents.trap_mastery -> rank() * 0.10;
    base_crit_bonus_multiplier *= 0.5;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.toxicology -> effect_base_value( 1 ) / 100.0;

    if ( p -> sets -> set ( SET_T11_4PC_MELEE ) -> ok() ) tick_zero = true;

    base_dd_min=base_dd_max=0;
    tick_power_mod=extra_coeff();

    // Figure out how damage is calculated.
  }

  virtual void tick()
  {
    hunter_attack_t::tick();
    hunter_t* p = player -> cast_hunter();
    p -> buffs_lock_and_load -> trigger( 2 );
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    trigger_thrill_of_the_hunt( this );
  }
};

// Chimera Shot ================================================================

struct chimera_shot_t : public hunter_attack_t
{
 chimera_shot_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "chimera_shot", player, "Chimera Shot" )
  {
    hunter_t* p = player -> cast_hunter();

    check_talent( p -> talents.chimera_shot -> rank() );

    parse_options( NULL, options_str );

    direct_power_mod = 0.488;

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    base_cost   -= p -> talents.efficiency -> rank() * 2;

    normalize_weapon_speed = true;

    cooldown -> duration += p -> glyphs.chimera_shot -> effect_base_value( 1 ) / 1000.0;

  }


  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();

    hunter_attack_t::execute();

    if ( p -> sets -> set ( SET_T11_4PC_MELEE ) -> ok() )
    {
      if ( p -> rng_tier11_4pc -> roll( p -> sets -> set ( SET_T11_4PC_MELEE ) -> proc_chance() ) )
        p -> resource_gain( RESOURCE_FOCUS, 25, p -> gains_tier11_4pc );
    }
    if ( result_is_hit() )
    {
      if ( result == RESULT_CRIT )
      {
        trigger_piercing_shots( this );
      }
      if ( p -> dots_serpent_sting -> ticking )
      {
	p -> dots_serpent_sting -> action -> refresh_duration();
      }
    }
  }


};

// Cobra Shot Attack ==========================================================

struct cobra_shot_t : public hunter_attack_t
{
  cobra_shot_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "cobra_shot", player, "Cobra Shot" )
  {
    hunter_t* p = player -> cast_hunter();

    parse_options( NULL, options_str );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );
    weapon_power_mod        = 0;

    direct_power_mod = 0.017;

  }
  void execute()
  {
    hunter_attack_t::execute();
    if ( result_is_hit() )
    {
      hunter_t* p = player -> cast_hunter();
      if ( p -> dots_serpent_sting -> ticking ) p -> dots_serpent_sting -> action -> extend_duration( 2 );
      double focus = 9;
      if ( target -> health_percentage() <= 25 && p -> talents.termination -> rank() ) focus += p -> talents.termination -> rank() * 3;
      p -> resource_gain( RESOURCE_FOCUS, focus, p -> gains_cobra_shot );
      if ( result == RESULT_CRIT )
      {
        trigger_piercing_shots( this );
      }
    }
  }

  void player_buff()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::player_buff();
    if ( p -> talents.careful_aim -> rank() && target -> health_percentage() > 80 )
    {
      player_crit += p -> talents.careful_aim -> effect_base_value( 1 ) / 100.0;
    }
    if ( p -> buffs_sniper_training -> up() )
      player_multiplier *= 1.0 + p -> buffs_sniper_training -> effect_base_value( 1 ) / 100.0;
  }
};

// Explosive Shot ================================================================

struct explosive_shot_t : public hunter_attack_t
{
  int lock_and_load;

  explosive_shot_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "explosive_shot", player, "Explosive Shot" ),
      lock_and_load( 0 )
  {
    hunter_t* p = player -> cast_hunter();

    assert ( p -> primary_tree() == TREE_SURVIVAL );

    parse_options( NULL, options_str );

    base_cost -= p -> talents.efficiency -> rank() * 2;

    tick_power_mod = 0.273;

    tick_zero      = true;

  }

  virtual double cost() SC_CONST
  {
    hunter_t* p = player -> cast_hunter();
    if ( p -> buffs_lock_and_load -> check() ) return 0;
    return hunter_attack_t::cost();
  }


  virtual void update_ready()
  {
    hunter_t* p = player -> cast_hunter();
    cooldown -> duration = ( p -> buffs_lock_and_load -> check() ? 0.0 : spell_id_t::cooldown() );
    hunter_attack_t::update_ready();
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    hunter_t* p = player -> cast_hunter();
    p -> buffs_lock_and_load -> decrement();
    trigger_thrill_of_the_hunt( this );
    if ( result == RESULT_CRIT ) p -> active_pet -> buffs_sic_em -> trigger();
  }
};

// Kill Shot =================================================================

struct kill_shot_t : public hunter_attack_t
{
  kill_shot_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "kill_shot", player, "Kill Shot" )
  {
    hunter_t* p = player -> cast_hunter();

    parse_options( NULL, options_str );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    weapon_multiplier = effect_average( 2 ) / 100.0;
    direct_power_mod = 0.3;

    base_crit += p -> talents.sniper_training -> effect_base_value( 2 ) / 100.0;

                normalize_weapon_speed = true;
    if ( p -> glyphs.kill_shot -> ok() )
    {
      cooldown -> duration -= 6;
    }
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    hunter_t* p = player -> cast_hunter();
    if ( p -> cooldowns_glyph_kill_shot -> remains() == 0 )
    {
      cooldown -> reset();
      p -> cooldowns_glyph_kill_shot -> start();
    }
  }

  virtual bool ready()
  {
    if ( target -> health_percentage() > 20 )
      return false;

    return hunter_attack_t::ready();
  }
};

// Multi Shot Attack =========================================================

struct multi_shot_t : public hunter_attack_t
{
  multi_shot_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "multi_shot", player, "Multi-Shot" )
  {
    hunter_t* p = player -> cast_hunter();
    assert( p -> ranged_weapon.type != WEAPON_NONE );
    parse_options( NULL, options_str );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    normalize_weapon_speed = true;

  }
};

// Scatter Shot Attack =========================================================

struct scatter_shot_t : public hunter_attack_t
{
  scatter_shot_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "scatter_shot", player, "Scatter Shot" )
  {
    hunter_t* p = player -> cast_hunter();
    parse_options( NULL, options_str );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    normalize_weapon_speed = true;

    weapon_multiplier *= 0.5;

    id = 19503;
  }
};
struct serpent_sting_burst_t : public hunter_attack_t
{
  serpent_sting_burst_t( player_t* player ) :
    hunter_attack_t( "serpent_sting_burst", player, SCHOOL_NATURE, TREE_MARKSMANSHIP )
  {
    dual       = true;
    proc       = true;
    background = true;

    // This helps log file and decouples the sooth RNG from the ticks.
    stats = player -> get_stats( "serpent_sting" );
  }

  virtual void execute()
  {
    hunter_attack_t::execute();

    update_stats( DMG_DIRECT );
  }

};
// Serpent Sting Attack =========================================================

struct serpent_sting_t : public hunter_attack_t
{
  serpent_sting_burst_t* serpent_sting_burst;
  serpent_sting_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "serpent_sting", player, "Serpent Sting" )
  {
    hunter_t* p = player -> cast_hunter();

    parse_options( NULL, options_str );

    tick_power_mod = 0.4;

    base_crit += p -> glyphs.serpent_sting -> effect_base_value( 1 ) / 100.0;
    base_crit += p -> sets -> set ( SET_T11_2PC_MELEE ) -> effect_base_value( 1 ) / 100.0;
    base_crit_bonus_multiplier *= 0.5;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.toxicology -> effect_base_value( 1 ) / 100.0;
    serpent_sting_burst = new serpent_sting_burst_t( p );

  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    if ( result_is_hit() ) target -> debuffs.poisoned -> increment();
    hunter_t* p = player -> cast_hunter();
    if ( serpent_sting_burst && p -> talents.improved_serpent_sting -> rank() )
        {
          double t = p -> talents.improved_serpent_sting -> rank() * 0.30 * ( ceil( base_td ) + total_power() * tick_power_mod );
          double n = hasted_num_ticks();


          serpent_sting_burst -> base_dd_min    = t * n;
          serpent_sting_burst -> base_dd_max    = t * n;
          serpent_sting_burst -> execute();
        }
  }

  virtual void last_tick()
  {
    hunter_attack_t::last_tick();
    target -> debuffs.poisoned -> decrement();
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
      hunter_attack_t( "silencing_shot", player, "Silencing Shot" )
  {
    hunter_t* p = player -> cast_hunter();

    check_talent( p -> talents.silencing_shot -> rank() );


    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );
    weapon_multiplier = 0.0;

    normalize_weapon_speed = true;

  }
};

// Steady Shot Attack =========================================================

struct steady_shot_t : public hunter_attack_t
{
  steady_shot_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "steady_shot", player, "Steady Shot" )
  {
    hunter_t* p = player -> cast_hunter();
    parse_options( NULL, options_str );

                normalize_weapon_damage = true;
    normalize_weapon_speed  = true;
    
    direct_power_mod = 0.021;
    weapon_multiplier = effect_average( 2 ) / 100.0;
    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );
  }
  virtual void trigger_improved_steady_shot()
  {
    hunter_t* p = player -> cast_hunter();
    p -> buffs_pre_improved_steady_shot -> trigger( 1 );
  }
  void execute()
  {
    hunter_attack_t::execute();
    if ( result_is_hit() )
    {
      hunter_t* p = player -> cast_hunter();
      double focus = 9;
            if ( target -> health_percentage() <= 25 && p -> talents.termination -> rank() ) focus += p -> talents.termination -> rank() * 3;

      p -> resource_gain( RESOURCE_FOCUS, focus, p -> gains_steady_shot );
      p -> buffs_master_marksman -> trigger();

      if ( result == RESULT_CRIT )
      {
        trigger_piercing_shots( this );
      }
    }
  }

  void player_buff()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::player_buff();

    if ( p -> glyphs.steady_shot -> ok() )
    {
      player_multiplier *= 1.10;
    }
    if ( p -> talents.careful_aim -> rank() && target -> health_percentage() > 80 )
    {
      player_crit += p -> talents.careful_aim -> effect_base_value( 1 ) / 100.0;
    }
    if ( p -> buffs_sniper_training -> up() )
      player_multiplier *= 1.0 + p -> buffs_sniper_training -> effect_base_value( 1 ) / 100.0;
  }
};

// Wild Quiver

struct wild_quiver_shot_t : public ranged_t
{
  wild_quiver_shot_t( hunter_t* p ) : ranged_t( p, "wild_quiver_shot" )
  {


    repeating   = false;
  }
};

struct wild_quiver_trigger_t : public action_callback_t
{
  attack_t* attack;
  rng_t* rng;
  wild_quiver_trigger_t( player_t* p, attack_t* a ) : action_callback_t( p -> sim, p ), attack(a)
  {
    rng = p -> get_rng( "wild_quiver" );
  }
  virtual void trigger( action_t* a )
  {
    hunter_t* p = listener -> cast_hunter();
    if ( ! a -> weapon ) return;
    if ( a -> weapon -> slot != SLOT_RANGED ) return;
    if ( rng -> roll( p -> composite_mastery() * 1.8 / 100.0 ) )
      {
      attack -> execute();
      p -> procs_wild_quiver -> occur();
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

// Aspect of the Hawk

struct aspect_of_the_hawk_t : public hunter_spell_t
{
  aspect_of_the_hawk_t( player_t* player, const std::string& options_str ) :
    hunter_spell_t( "aspect_of_the_hawk", player, "Aspect of the Hawk" )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    hunter_spell_t::execute();
    hunter_t* p = player -> cast_hunter();

    if ( !p -> active_aspect )
      {
      p -> active_aspect = ASPECT_HAWK;
      p -> buffs_aspect_of_the_hawk -> trigger( 1, effect_average( 1 ) * ( 1.0 + p -> talents.one_with_nature -> effect_base_value( 1 ) / 100.0 ) );
      }
    else if ( p -> active_aspect == ASPECT_HAWK )
      {
      p -> active_aspect = ASPECT_NONE;
      p -> buffs_aspect_of_the_hawk -> expire();
      }

  }
  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();
    if ( p -> active_aspect == ASPECT_HAWK) return false;
    return hunter_spell_t::ready();
  }
};


// Bestial Wrath =========================================================

struct bestial_wrath_t : public hunter_spell_t
{
  bestial_wrath_t( player_t* player, const std::string& options_str ) :
      hunter_spell_t( "bestial_wrath", player, "Bestial Wrath" )
  {
    hunter_t* p = player -> cast_hunter();
    parse_options( NULL, options_str );
    check_talent( p -> talents.bestial_wrath -> rank() );

    cooldown -> duration += p -> glyphs.bestial_wrath -> effect_base_value( 1 ) / 1000.0;
    cooldown -> duration *=  ( 1.0 - p -> talents.longevity -> effect_base_value( 1 ) / 100.0 );
    harmful = false;

  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_pet_t* pet = p -> active_pet;
    p -> buffs_beast_within -> trigger();
    pet -> buffs_bestial_wrath -> trigger();
    hunter_spell_t::execute();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();
    if ( ! p -> active_pet ) return false;
    return hunter_spell_t::ready();
  }
};

// Fervor =============================================================

struct fervor_t : public hunter_spell_t
{
  fervor_t( player_t* player, const std::string& options_str ) :
      hunter_spell_t( "fervor", player, "Fervor" )
  {
    hunter_t* p = player -> cast_hunter();
    
    check_talent( p -> talents.fervor -> rank() );

    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    p -> active_pet -> resource_gain( RESOURCE_FOCUS, 50, p -> active_pet -> gains_fervor );
    p               -> resource_gain( RESOURCE_FOCUS, 50, p               -> gains_fervor );

    p -> buffs_rapid_fire -> trigger( 1, ( p -> glyphs.rapid_fire -> ok() ? 0.50 : 0.40 ) );
    hunter_spell_t::execute();
  }
};

// Hunter's Mark =========================================================

struct hunters_mark_t : public hunter_spell_t
{
  double ap_bonus;

  hunters_mark_t( player_t* player, const std::string& options_str ) :
      hunter_spell_t( "hunters_mark", player, "Hunter's Mark" ), ap_bonus( 0 )
  {
    parse_options( NULL, options_str );

    ap_bonus = effect_average( 2 );
    harmful = false;

  }

  virtual void execute()
  {
    hunter_spell_t::execute();

    hunter_t* p = player -> cast_hunter();
    target -> debuffs.hunters_mark -> trigger( 1, ap_bonus );
    target -> debuffs.hunters_mark -> source = p;
  }

  virtual bool ready()
  {
    if ( ! hunter_spell_t::ready() )
      return false;

    return ap_bonus > target -> debuffs.hunters_mark -> current_value;
  }
};

// Kill Command =======================================================

struct kill_command_t : public hunter_spell_t
{
  kill_command_t( player_t* player, const std::string& options_str ) :
      hunter_spell_t( "kill_command", player, "Kill Command" )
  {
    hunter_t* p = player -> cast_hunter();

    parse_options( NULL, options_str );

    base_crit += p -> talents.improved_kill_command -> effect_base_value( 1 ) / 100.0;

    base_cost += p -> glyphs.kill_command -> effect_base_value( 1 );
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
   rapid_fire_t( player_t* player, const std::string& options_str ) :
      hunter_spell_t( "rapid_fire", player, "Rapid Fire" )
  {
    hunter_t* p = player -> cast_hunter();
    parse_options( NULL, options_str );

    cooldown -> duration += p -> talents.posthaste -> effect_base_value( 1 ) / 1000.0;

    harmful = false;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();

    p -> buffs_rapid_fire -> trigger( 1, ( p -> glyphs.rapid_fire -> ok() ? 0.50 : 0.40 ) );

    if ( p -> talents.rapid_recuperation -> rank() )
      p -> resource_gain( RESOURCE_FOCUS, p -> talents.rapid_recuperation -> effect_base_value( 2 ), p -> gains_rapid_recuperation );

    hunter_spell_t::execute();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( p -> buffs_rapid_fire -> check() )
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
      hunter_spell_t( "readiness", player, "Readiness" ),
      wait_for_rf( false )
  {
    hunter_t* p = player -> cast_hunter();

    check_talent( p -> talents.readiness -> rank() );

    option_t options[] =
    {
      // Only perform Readiness while Rapid Fire is up, allows the sequence
      // Rapid Fire, Readiness, Rapid Fire, for better RF uptime
      { "wait_for_rapid_fire", OPT_BOOL, &wait_for_rf },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

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
      hunter_spell_t( "trueshot_aura", player, "Trueshot Aura" )
  {
    hunter_t* p = player -> cast_hunter();
    check_talent( p -> talents.trueshot_aura -> rank() );
    trigger_gcd = 0;
    harmful = false;

  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_spell_t::execute();
    p -> buffs_trueshot_aura -> trigger();
    sim -> auras.trueshot -> trigger();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();
    return( ! p -> buffs_trueshot_aura -> check() );
  }
};

// Event Shedule Sniper Trainig

struct snipertraining_hunter_t : public event_t
{
  snipertraining_hunter_t ( player_t* player ) :
    event_t( player -> sim, player )
  {
    name = "Sniper_Training_Check";
    sim -> add_event( this, 5.0 );
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    if ( !p -> in_combat )
      p -> buffs_sniper_training -> trigger();
    if ( !p -> buffs.moving -> up() && ( sim -> current_time - p -> buffs.moving -> last_start - p -> buffs.moving -> buff_duration - p -> talents.sniper_training -> effect_base_value( 1 ) ) > 0 )
    {
      p -> buffs_sniper_training -> trigger();
    }
    new (sim) snipertraining_hunter_t( p );
  }
};

// hunter_pet_t::create_action =============================================

action_t* hunter_pet_t::create_action( const std::string& name,
                                       const std::string& options_str )
{
  if ( name == "auto_attack"       ) return new   pet_auto_attack_t( this, options_str );
  //if ( name == "bite"              ) return new              bite_t( this, options_str );
  if ( name == "call_of_the_wild"  ) return new  call_of_the_wild_t( this, options_str );
  if ( name == "froststorm_breath" ) return new froststorm_breath_t( this, options_str );
  if ( name == "furious_howl"      ) return new      furious_howl_t( this, options_str );
  if ( name == "lightning_breath"  ) return new  lightning_breath_t( this, options_str );
  if ( name == "rabid"             ) return new             rabid_t( this, options_str );
  if ( name == "roar_of_recovery"  ) return new  roar_of_recovery_t( this, options_str );
  //if ( name == "wolverine_bite"    ) return new    wolverine_bite_t( this, options_str );


  return pet_t::create_action( name, options_str );
}

// ferocity_pet_t::create_action =============================================

action_t* ferocity_pet_t::create_action( const std::string& name,
                                       const std::string& options_str )
{
  if ( name == "claw"              ) return new              claw_t( this, options_str );
  if ( name == "monstrous_bite"    ) return new    monstrous_bite_t( this, options_str );


  return hunter_pet_t::create_action( name, options_str );
}

// hunter_t::create_action ====================================================

action_t* hunter_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "auto_shot"             ) return new              auto_shot_t( this, options_str );
  if ( name == "aimed_shot"            ) return new             aimed_shot_t( this, options_str );
  if ( name == "arcane_shot"           ) return new            arcane_shot_t( this, options_str );
  if ( name == "aspect_of_the_hawk"    ) return new     aspect_of_the_hawk_t( this, options_str );
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
  if ( name == "serpent_sting"         ) return new          serpent_sting_t( this, options_str );
  if ( name == "silencing_shot"        ) return new         silencing_shot_t( this, options_str );
  if ( name == "steady_shot"           ) return new            steady_shot_t( this, options_str );
  if ( name == "summon_pet"            ) return new             summon_pet_t( this, options_str );
  if ( name == "trueshot_aura"         ) return new          trueshot_aura_t( this, options_str );
  if ( name == "cobra_shot"            ) return new             cobra_shot_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// hunter_t::create_pet =======================================================

pet_t* hunter_t::create_pet( const std::string& pet_name )
{
  pet_t* p = find_pet( pet_name );
  if ( p ) return p;

  // Ferocity
  if ( pet_name == "carrion_bird" ) return new ferocity_pet_t( sim, this, pet_name, PET_CARRION_BIRD );
  if ( pet_name == "cat"          ) return new ferocity_pet_t( sim, this, pet_name, PET_CAT          );
  if ( pet_name == "core_hound"   ) return new ferocity_pet_t( sim, this, pet_name, PET_CORE_HOUND   );
  if ( pet_name == "devilsaur"    ) return new ferocity_pet_t( sim, this, pet_name, PET_DEVILSAUR    );
  if ( pet_name == "hyena"        ) return new ferocity_pet_t( sim, this, pet_name, PET_HYENA        );
  if ( pet_name == "moth"         ) return new ferocity_pet_t( sim, this, pet_name, PET_MOTH         );
  if ( pet_name == "raptor"       ) return new ferocity_pet_t( sim, this, pet_name, PET_RAPTOR       );
  if ( pet_name == "spirit_beast" ) return new ferocity_pet_t( sim, this, pet_name, PET_SPIRIT_BEAST );
  if ( pet_name == "tallstrider"  ) return new ferocity_pet_t( sim, this, pet_name, PET_TALLSTRIDER  );
  if ( pet_name == "wasp"         ) return new ferocity_pet_t( sim, this, pet_name, PET_WASP         );
  if ( pet_name == "wolf"         ) return new ferocity_pet_t( sim, this, pet_name, PET_WOLF         );

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
  //create_pet( "devilsaur" );
  //create_pet( "raptor" );
  //create_pet( "wind_serpent" );
  //create_pet( "wolf" );
}

// hunter_t::init_talents ===================================================

void hunter_t::init_talents()
{

  // Beast Mastery
  talents.improved_kill_command           = find_talent( "Improved Kill Command" );
  talents.one_with_nature                 = find_talent( "One with Nature" );
  talents.bestial_discipline              = find_talent( "Bestial Discipline" );
  talents.pathfinding                     = find_talent( "Pathfinding" );
  talents.spirit_bond                     = find_talent( "Spirit Bond" );
  talents.frenzy                          = find_talent( "Frenzy" );
  talents.improved_mend_pet               = find_talent( "Improved Mend Pet" );
  talents.cobra_strikes                   = find_talent( "Cobra Strikes" );
  talents.fervor                          = find_talent( "Fervor" );
  talents.focus_fire                      = find_talent( "Focus Fire" );
  talents.longevity                       = find_talent( "Longevity" );
  talents.killing_streak                  = find_talent( "Killing Streak" );
  talents.crouching_tiger_hidden_chimera  = find_talent( "Crouching Tiger, Hidden Chimera" );
  talents.bestial_wrath                   = find_talent( "Bestial Wrath" );
  talents.ferocious_inspiration           = find_talent( "Ferocious Inspiration" );
  talents.kindred_spirits                 = find_talent( "Kindred Spirits" );
  talents.the_beast_within                = find_talent( "The Beast Within" );
  talents.invigoration                    = find_talent( "Invigoration" );
  talents.beast_mastery                   = find_talent( "Beast Mastery" );

  // Marksmanship
  talents.go_for_the_throat               = find_talent( "Go for the Throat" );
  talents.efficiency                      = find_talent( "Efficiency" );
  talents.rapid_killing                   = find_talent( "Rapid Killing" );
  talents.sic_em                          = find_talent( "Sic 'Em!" );
  talents.improved_steady_shot            = find_talent( "Improved Steady Shot" );
  talents.careful_aim                     = find_talent( "Careful Aim" );
  talents.silencing_shot                  = find_talent( "Silencing Shot" );
  talents.concussive_barrage              = find_talent( "Concussive Barrage" );
  talents.piercing_shots                  = find_talent( "Piercing Shots" );
  talents.bombardment                     = find_talent( "Bombardment" );
  talents.trueshot_aura                   = find_talent( "Trueshot Aura" );
  talents.termination                     = find_talent( "Termination" );
  talents.resistance_is_futile            = find_talent( "Resistance is Futile" );
  talents.rapid_recuperation              = find_talent( "Rapid Recuperation" );
  talents.master_marksman                 = find_talent( "Master Marksman" );
  talents.readiness                       = find_talent( "Readiness" );
  talents.posthaste                       = find_talent( "Posthaste" );
  talents.marked_for_death                = find_talent( "Marked for Death" );
  talents.chimera_shot                    = find_talent( "Chimera Shot" );

  // Survival
  talents.hunter_vs_wild                  = find_talent( "Hunter vs. Wild" );
  talents.pathing                         = find_talent( "Pathing" );
  talents.improved_serpent_sting          = find_talent( "Improved Serpent Sting" );
  talents.survival_tactics                = find_talent( "Survival Tactics" );
  talents.trap_mastery                    = find_talent( "Trap Mastery" );
  talents.entrapment                      = find_talent( "Entrapment" );
  talents.point_of_no_escape              = find_talent( "Point of No Escape" );
  talents.thrill_of_the_hunt              = find_talent( "Thrill of the Hunt" );
  talents.counterattack                   = find_talent( "Counterattack" );
  talents.lock_and_load                   = find_talent( "Lock and Load" );
  talents.resourcefulness                 = find_talent( "Resourcefulness" );
  talents.mirrored_blades                 = find_talent( "Mirrored Blades" );
  talents.tnt                             = find_talent( "T.N.T." );
  talents.toxicology                      = find_talent( "Toxicology" );
  talents.wyvern_sting                    = find_talent( "Wyvern Sting" );
  talents.noxious_stings                  = find_talent( "Noxious Stings" );
  talents.hunting_party                   = find_talent( "Hunting Party" );
  talents.sniper_training                 = find_talent( "Sniper Training" );
  talents.serpent_spread                  = find_talent( "Serpent Spread" );
  talents.black_arrow                     = find_talent( "Black Arrow" );

  player_t::init_talents();
}

// hunter_t::init_spells ===================================================

void hunter_t::init_spells()
{
  player_t::init_spells();

  passive_spells.animal_handler = new passive_spell_t( this, "animal_handler", "Animal Handler" );
  passive_spells.artisan_quiver = new passive_spell_t( this, "artisan_quiver", "Artisan Quiver" );
  passive_spells.into_the_wildness = new passive_spell_t( this, "into_the_wildness", "Into the Wildness" );

  passive_spells.master_of_beasts = new mastery_t( this, "master_of_beasts", "Master of Beasts", TREE_BEAST_MASTERY );
  passive_spells.wild_quiver = new mastery_t( this, "wild_quiver", "Wild Quiver", TREE_MARKSMANSHIP );
  passive_spells.essence_of_the_viper = new mastery_t( this, "essence_of_the_viper", "Essence of the Viper", TREE_SURVIVAL );

  glyphs.aimed_shot = new glyph_t(this, "Glyph of Aimed Shot");
  glyphs.arcane_shot = new glyph_t(this, "Glyph of Arcane Shot");
  glyphs.bestial_wrath = new glyph_t(this, "Glyph of Bestial Wrath");
  glyphs.chimera_shot = new glyph_t(this, "Glyph of Chimera Shot");
  glyphs.explosive_shot = new glyph_t(this, "Glyph of Explosive Shot");
  glyphs.kill_shot = new glyph_t(this, "Glyph of Kill Shot");
  glyphs.rapid_fire = new glyph_t(this, "Glyph of Rapid Fire");
  glyphs.serpent_sting = new glyph_t(this, "Glyph of Serpent Sting");
  glyphs.steady_shot = new glyph_t(this, "Glyph of Steady Shot");
  glyphs.kill_command = new glyph_t(this, "Glyph of Kill Command");

  static uint32_t set_bonuses[N_TIER][N_TIER_BONUS] = 
  {
    //  C2P    C4P    M2P    M4P    T2P    T4P
    {     0,     0, 70727, 70730,     0,     0 }, // Tier10
    {     0,     0, 89923, 89925,     0,     0 }, // Tier11
    {     0,     0,     0,     0,     0,     0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// hunter_t::init_glyphs ===================================================

void hunter_t::init_glyphs()
{
  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );

  for ( int i=0; i < num_glyphs; i++ )
  {
    std::string& n = glyph_names[ i ];

    if      ( n == "aimed_shot"                  ) glyphs.aimed_shot -> enable();
    else if ( n == "arcane_shot"                 ) glyphs.arcane_shot -> enable();
    else if ( n == "bestial_wrath"               ) glyphs.bestial_wrath -> enable();
    else if ( n == "chimera_shot"                ) glyphs.chimera_shot -> enable();
    else if ( n == "explosive_shot"              ) glyphs.explosive_shot -> enable();
    else if ( n == "hunters_mark"                ) ;
    else if ( n == "kill_shot"                   ) glyphs.kill_shot -> enable();
    else if ( n == "rapid_fire"                  ) glyphs.rapid_fire -> enable();
    else if ( n == "serpent_sting"               ) glyphs.serpent_sting -> enable();
    else if ( n == "steady_shot"                 ) glyphs.steady_shot -> enable();
    else if ( n == "kill_command"                ) glyphs.kill_command -> enable();
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
    else if ( n == "ice_trap"           ) ;
    else if ( n == "scatter_shot"       ) ;
    else if ( n == "trap_launcher"      ) ;
    else if ( n == "misdirection"       ) ;
    else if ( n == "aspect_of_the_pack" ) ;
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
  case RACE_HUMAN:
  case RACE_DWARF:
  case RACE_DRAENEI:
  case RACE_NIGHT_ELF:
  case RACE_WORGEN:
  case RACE_ORC:
  case RACE_TROLL:
  case RACE_TAUREN:
  case RACE_BLOOD_ELF:
  case RACE_UNDEAD:
  case RACE_GOBLIN:
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
  player_t::init_base();

  attribute_multiplier_initial[ ATTR_AGILITY ]   *= 1.0 + talents.hunting_party -> rank() * 0.02;
  attribute_multiplier_initial[ ATTR_STAMINA ]   *= 1.0 + talents.hunter_vs_wild -> rank() * 0.04;

  base_attack_power = level * 2 - 10;

  initial_attack_power_per_strength = 1.0;
  initial_attack_power_per_agility  = 2.0;

  health_per_stamina = 10;
  // FIXME! 
  base_focus_regen_per_second = 6;
  
  resource_base[ RESOURCE_FOCUS ] = 100 + talents.kindred_spirits -> effect_base_value( 1 );

  position = POSITION_RANGED;

  diminished_kfactor    = 0.009880;
  diminished_dodge_capi = 0.006870;
  diminished_parry_capi = 0.006870;

  if ( passive_spells.wild_quiver -> ok() )
  register_attack_result_callback( RESULT_ALL_MASK, new wild_quiver_trigger_t( this, new wild_quiver_shot_t( this ) ) );
}

// hunter_t::init_buffs =======================================================

void hunter_t::init_buffs()
{
  player_t::init_buffs();

  buffs_aspect_of_the_hawk          = new buff_t( this, 13165, "aspect_of_the_hawk" );
  buffs_beast_within                = new buff_t( this, 34471, "beast_within", talents.the_beast_within -> rank() );
  buffs_call_of_the_wild            = new buff_t( this, 53434, "call_of_the_wild" );
  buffs_cobra_strikes               = new buff_t( this, 53257, "cobra_strikes", talents.cobra_strikes -> rank() * 0.20 );
  buffs_culling_the_herd            = new buff_t( this, 70893, "culling_the_herd" );
  buffs_improved_steady_shot        = new buff_t( this, 53220, "improved_steady_shot", talents.improved_steady_shot -> rank() );
  buffs_lock_and_load               = new buff_t( this, 56453, "lock_and_load", talents.tnt -> effect_base_value( 1 ) / 100.0 );
  buffs_master_marksman             = new buff_t( this, 82925, "master_marksman", talents.master_marksman -> proc_chance());
  buffs_sniper_training             = new buff_t( this, talents.sniper_training -> rank() == 3 ? 64420 : talents.sniper_training -> rank() == 2 ? 64419 : talents.sniper_training -> rank() == 1 ? 64418 : 0, "sniper_training", talents.sniper_training -> rank() );

  buffs_rapid_fire                  = new buff_t( this, 3045, "rapid_fire" );
  buffs_rapid_fire -> cooldown -> duration = 0;
  buffs_pre_improved_steady_shot    = new buff_t( this, "pre_improved_steady_shot",    2 );

  buffs_tier10_2pc                  = new buff_t( this, "tier10_2pc",                  1, 10.0,  0.0, ( set_bonus.tier10_2pc_melee() ? 0.05 : 0 ) );
  buffs_tier10_4pc                  = new buff_t( this, "tier10_4pc",                  1, 10.0,  0.0, ( set_bonus.tier10_4pc_melee() ? 0.05 : 0 ) );

  // Own TSA for Glyph of TSA
  buffs_trueshot_aura               = new buff_t( this, 19506, "trueshot_aura" );
}

// hunter_t::init_gains ======================================================

void hunter_t::init_gains()
{
  player_t::init_gains();
  gains_glyph_of_arcane_shot = get_gain( "glyph_of_arcane_shot" );
  gains_invigoration         = get_gain( "invigoration" );
  gains_fervor               = get_gain( "fervor" );
  gains_rapid_recuperation   = get_gain( "rapid_recuperation" );
  gains_roar_of_recovery     = get_gain( "roar_of_recovery" );
  gains_thrill_of_the_hunt   = get_gain( "thrill_of_the_hunt" );
  gains_steady_shot          = get_gain( "steady_shot" );
  gains_glyph_aimed_shot     = get_gain( "glyph_aimed_shot" );
  gains_cobra_shot           = get_gain( "cobra_shot" );
  gains_tier11_4pc           = get_gain( "tier11_4pc" );
}

// hunter_t::init_procs ======================================================

void hunter_t::init_procs()
{
  player_t::init_procs();
  procs_wild_quiver = get_proc("wild_quiver");
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
  rng_tier11_4pc           = get_rng( "tier11_4pc" );

  // Overlapping procs require the use of a "distributed" RNG-stream when normalized_roll=1
  // also useful for frequent checks with low probability of proc and timed effect

  rng_frenzy               = get_rng( "frenzy",                      RNG_DISTRIBUTED );
  rng_rabid_power          = get_rng( "rabid_power",                 RNG_DISTRIBUTED );

}

// hunter_t::init_scaling ====================================================

void hunter_t::init_scaling()
{
  player_t::init_scaling();

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
    action_list_str = "flask,type=winds";
    action_list_str += "/food,type=seafood_magnifique_feast";
    action_list_str += "/hunters_mark/summon_pet";
    if ( talents.trueshot_aura -> rank() ) action_list_str += "/trueshot_aura";
    action_list_str += "/tolvir_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
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
    default: break;
    }
    switch ( primary_tree() )
    {
    case TREE_BEAST_MASTERY:
      if ( talents.bestial_wrath -> rank() )
      {
        action_list_str += "/kill_command,sync=bestial_wrath";
        action_list_str += "/bestial_wrath";
      }
      else
      {
        action_list_str += "/kill_command";
      }
      action_list_str += "/aspect_of_the_hawk";
      action_list_str += "/rapid_fire";
      action_list_str += "/kill_shot";
      action_list_str += "/serpent_sting,if=!ticking";
      action_list_str += "/steady_shot";
      break;
    case TREE_MARKSMANSHIP:
      action_list_str += "/aspect_of_the_hawk";
      action_list_str += "/serpent_sting,if=!ticking";
      action_list_str += "/rapid_fire";
      //action_list_str += "/kill_command";
      if ( talents.chimera_shot -> rank() )
      {
        action_list_str += "/chimera_shot";
      }
      action_list_str += "/kill_shot";
      action_list_str += "/aimed_shot,if=buff.master_marksman.stack=5";

      action_list_str += "/readiness,wait_for_rapid_fire=1";
      action_list_str += "/arcane_shot,if=focus>=40&buff.improved_steady_shot.up&cooldown.chimera_shot.remains>0";
      action_list_str += "/steady_shot";
      break;
    case TREE_SURVIVAL:
      action_list_str += "/aspect_of_the_hawk";
      action_list_str += "/rapid_fire";
      action_list_str += "/kill_shot";
      if ( talents.lock_and_load -> rank() )
      {
        action_list_str += "/explosive_shot,if=buff.lock_and_load.react&!ticking";
        action_list_str += "/explosive_shot,if=!buff.lock_and_load.up&!ticking";
      }
      else if ( talents.lock_and_load -> rank() == 0 )
      {
        action_list_str += "/explosive_shot,if=!ticking";
      }
      if ( talents.black_arrow -> rank() ) action_list_str += "/black_arrow,if=!ticking";
      action_list_str += "/serpent_sting,if=!ticking";
      if ( level >=81 )
        action_list_str += "/cobra_shot,if=dot.serpent_sting.remains<=8";
      action_list_str += "/arcane_shot,if=focus>=50|buff.lock_and_load.remains<gcd";
      action_list_str += "/steady_shot";
      break;
    default: break;
    }

    action_list_default = 1;
  }

  player_t::init_actions();
}

// hunter_t::combat_begin =====================================================

void hunter_t::combat_begin()
{
  player_t::combat_begin();

  if ( talents.ferocious_inspiration -> rank() )
    sim -> auras.ferocious_inspiration -> trigger();

  buffs_sniper_training -> trigger();
  new (sim) snipertraining_hunter_t( this );
}

// hunter_t::reset ===========================================================

void hunter_t::reset()
{
  player_t::reset();

  // Active
  active_pet            = 0;
  active_aspect         = ASPECT_NONE;

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

  if ( passive_spells.animal_handler -> ok() )
  {
    ap *= 1.15;
  }

  return ap;
}

// hunter_t::composite_attack_power_multiplier =================================

double hunter_t::composite_attack_power_multiplier() SC_CONST
{
  double mult = player_t::composite_attack_power_multiplier();

  if ( buffs_tier10_4pc -> up() )
  {
    mult *= 1.20;
  }
  if ( buffs_call_of_the_wild -> up() )
  {
    mult *= 1.0 + buffs_call_of_the_wild -> effect_base_value( 1 ) / 100.0;
  }

  return mult;
}

double hunter_t::composite_attack_haste() SC_CONST
{
  double h = player_t::composite_attack_haste();

  h *= 1.0 / ( 1.0 + talents.pathing -> effect_base_value( 1 ) / 100.0 );

  return h;
}

double hunter_t::agility() SC_CONST
{
  double agi = player_t::agility();
  if ( passive_spells.into_the_wildness -> ok() )
  {
    agi *= 1.15;
  }
  return agi;
}

double hunter_t::composite_player_multiplier( const school_type school ) SC_CONST
     {
       double m = player_t::composite_player_multiplier( school );
       if ( (school == SCHOOL_NATURE || school == SCHOOL_ARCANE || school== SCHOOL_SHADOW || school == SCHOOL_FIRE )&& passive_spells.essence_of_the_viper -> ok() )
       {
         m *= 1.0 + 1.0 / 100.0 * composite_mastery();
       }
       return m;
     }

// hunter_t::matching_gear_multiplier =====================================

double hunter_t::matching_gear_multiplier( const attribute_type attr ) SC_CONST
{
  if ( attr == ATTR_AGILITY )
    return 0.05;

  return 0.0;
}

// hunter_t::regen  =======================================================

void hunter_t::regen( double periodicity )
{
  player_t::regen( periodicity );
  periodicity *= 1.0 + haste_rating / rating.attack_haste;
  if ( buffs_rapid_fire -> check() && talents.rapid_recuperation -> rank() )
  {
    // 2/4 focus per sec
    double rr_regen = periodicity * 2 * talents.rapid_recuperation -> rank();

    resource_gain( RESOURCE_FOCUS, rr_regen, gains_rapid_recuperation );
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

// hunter_t::create_options =================================================

void hunter_t::create_options()
{
  player_t::create_options();

  option_t hunter_options[] =
  {
    { "summon_pet", OPT_STRING, &( summon_pet_str ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, hunter_options );
}

// hunter_t::create_profile =================================================

bool hunter_t::create_profile( std::string& profile_str, int save_type )
{

  return player_t::create_profile( profile_str, save_type );
}

// hunter_t::armory_extensions ==============================================

void hunter_t::armory_extensions( const std::string& region,
				  const std::string& server,
				  const std::string& character )
{
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

  std::string url = "http://" + region + ".battle.net/wow/en/character/" + server + "/" + character + "/pet";
  xml_node_t* pet_xml = xml_t::download( sim, url, "", -1 );
  if ( sim -> debug ) xml_t::print( pet_xml, sim -> output_file );

  xml_node_t* pet_list = xml_t::get_node( pet_xml, "div", "class", "pets-list" );

  xml_node_t* pet_script = xml_t::get_node( pet_list, "script", "type", "text/javascript" );

  if( ! pet_script )
  {
    sim -> errorf( "Hunter %s unable to download pet data from Armory\n", name() );
    sim -> cancel();
    return;
  }

  std::string cdata_str;
  if( xml_t::get_value( cdata_str, pet_script, "cdata" ) )
  {
    std::string::size_type pos = cdata_str.find( "{" );
    if( pos != std::string::npos ) cdata_str.erase( 0, pos+1 );
    pos = cdata_str.rfind( "}" );
    if( pos != std::string::npos ) cdata_str.erase( pos );

    js_node_t* pet_js = js_t::create( sim, cdata_str );
    pet_js = js_t::get_node( pet_js, "Pets" );
    if ( sim -> debug ) js_t::print( pet_js, sim -> output_file );

    if( ! pet_js )
    {
      sim -> errorf( "Hunter %s unable to download pet data from Armory\n", name() );
      sim -> cancel();
      return;
    }

    std::vector<js_node_t*> pet_records;
    int num_pets = js_t::get_children( pet_records, pet_js );
    for( int i=0; i < num_pets; i++ )
    {
      std::string pet_name, pet_talents;
      int pet_level, pet_family;

      if( ! js_t::get_value( pet_name,    pet_records[ i ], "name"     ) ||
	  ! js_t::get_value( pet_talents, pet_records[ i ], "build"    ) ||
	  ! js_t::get_value( pet_level,   pet_records[ i ], "level"    ) ||
	  ! js_t::get_value( pet_family,  pet_records[ i ], "familyId" ) )
      {
	sim -> errorf( "Hunter %s unable to decode pet name/build/level/familyId\n", name() );
	sim -> cancel();
	return;
      }

      log_t::output( sim, "NOT DOWNLOADING PET YET: %s %d %d %s\n", 
		     pet_name.c_str(), pet_level, pet_family, pet_talents.c_str() );

      // hunter_pet_t* pet = new hunter_pet_t( sim, this, pet_name, pet_types[ pet_family ] );
      // pet -> parse_talents_armory( pet_talents );
      // Remember to filter out pets with all-zero talent strings.
    }
  }
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

  if ( strstr( s, "ahnkahar_blood_hunter" ) ) return SET_T10_MELEE;
  if ( strstr( s, "lightning_charged"     ) ) return SET_T11_MELEE;

  return SET_NONE;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_hunter  =================================================

player_t* player_t::create_hunter( sim_t* sim, const std::string& name, race_type r )
{
  //sim -> errorf( "Hunter Module isn't avaiable at the moment." );

  return new hunter_t( sim, name, r );
  return NULL;
}


// player_t::hunter_init ====================================================

void player_t::hunter_init( sim_t* sim )
{

  sim -> auras.trueshot              = new aura_t( sim, "trueshot" );
  sim -> auras.ferocious_inspiration = new aura_t( sim, "ferocious_inspiration" );

  for ( target_t* t = sim -> target_list; t; t = t -> next )
  {
    t -> debuffs.hunters_mark  = new debuff_t( t, "hunters_mark",  1, 300.0 );
    t -> debuffs.scorpid_sting = new debuff_t( t, "scorpid_sting", 1, 20.0  );
  }
}

// player_t::hunter_combat_begin ============================================

void player_t::hunter_combat_begin( sim_t* sim )
{


  if ( sim -> overrides.trueshot_aura )         sim -> auras.trueshot -> override();
  if ( sim -> overrides.ferocious_inspiration ) sim -> auras.ferocious_inspiration -> override();

  for ( target_t* t = sim -> target_list; t; t = t -> next )
  {
    if ( sim -> overrides.hunters_mark ) t -> debuffs.hunters_mark -> override( 1, sim -> sim_data.effect_min( 1130, sim -> max_player_level, E_APPLY_AURA,A_RANGED_ATTACK_POWER_ATTACKER_BONUS ) * 1.3 );
  }
}

