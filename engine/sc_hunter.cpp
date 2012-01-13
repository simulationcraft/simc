// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Hunter
// ==========================================================================

struct hunter_pet_t;

enum aspect_type { ASPECT_NONE=0, ASPECT_HAWK, ASPECT_FOX, ASPECT_MAX };

struct hunter_targetdata_t : public targetdata_t
{
  dot_t* dots_serpent_sting;

  hunter_targetdata_t( player_t* source, player_t* target )
    : targetdata_t( source, target )
  {
  }
};

void register_hunter_targetdata( sim_t* sim )
{
  player_type t = HUNTER;
  typedef hunter_targetdata_t type;

  REGISTER_DOT( serpent_sting );
}


struct hunter_t : public player_t
{
  // Active
  hunter_pet_t* active_pet;
  int           active_aspect;
  action_t*     active_piercing_shots;
  action_t*     active_vishanka;

  // Buffs
  buff_t* buffs_aspect_of_the_hawk;
  buff_t* buffs_beast_within;
  buff_t* buffs_bombardment;
  buff_t* buffs_call_of_the_wild;
  buff_t* buffs_cobra_strikes;
  buff_t* buffs_culling_the_herd;
  buff_t* buffs_focus_fire;
  buff_t* buffs_improved_steady_shot;
  buff_t* buffs_killing_streak;
  buff_t* buffs_killing_streak_crits;
  buff_t* buffs_lock_and_load;
  buff_t* buffs_master_marksman;
  buff_t* buffs_master_marksman_fire;
  buff_t* buffs_pre_improved_steady_shot;
  buff_t* buffs_rapid_fire;
  buff_t* buffs_sniper_training;
  buff_t* buffs_trueshot_aura;
  buff_t* buffs_tier12_4pc;
  buff_t* buffs_tier13_4pc;

  // Cooldowns
  cooldown_t* cooldowns_explosive_shot;
  cooldown_t* cooldowns_vishanka;

  // Custom Parameters
  std::string summon_pet_str;

  // Gains
  gain_t* gains_glyph_of_arcane_shot;
  gain_t* gains_invigoration;
  gain_t* gains_fervor;
  gain_t* gains_focus_fire;
  gain_t* gains_rapid_recuperation;
  gain_t* gains_roar_of_recovery;
  gain_t* gains_thrill_of_the_hunt;
  gain_t* gains_steady_shot;
  gain_t* gains_glyph_aimed_shot;
  gain_t* gains_cobra_shot;
  gain_t* gains_tier11_4pc;
  gain_t* gains_tier12_4pc;

  // Procs
  proc_t* procs_thrill_of_the_hunt;
  proc_t* procs_wild_quiver;
  proc_t* procs_lock_and_load;
  proc_t* procs_flaming_arrow;
  proc_t* procs_deferred_piercing_shots;
  proc_t* procs_munched_piercing_shots;
  proc_t* procs_rolled_piercing_shots;

  // Random Number Generation
  rng_t* rng_frenzy;
  rng_t* rng_invigoration;
  rng_t* rng_owls_focus;
  rng_t* rng_rabid_power;
  rng_t* rng_thrill_of_the_hunt;
  rng_t* rng_tier11_4pc;
  rng_t* rng_tier12_2pc;

  // Talents
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
    talent_t* focus_fire;
    talent_t* longevity;
    talent_t* killing_streak;
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
    talent_t* sniper_training;
    talent_t* serpent_spread;
    talent_t* black_arrow;
  };
  talents_t talents;

  // Glyphs
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
    glyph_t* bestial_wrath;
    glyph_t* trap_launcher;
  };
  glyphs_t glyphs;

  // Spells
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

  action_t* flaming_arrow;

  double merge_piercing_shots;
  double tier13_4pc_cooldown;
  uint32_t vishanka;

  hunter_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) : player_t( sim, HUNTER, name, r )
  {
    if ( race == RACE_NONE ) race = RACE_NIGHT_ELF;

    tree_type[ HUNTER_BEAST_MASTERY ] = TREE_BEAST_MASTERY;
    tree_type[ HUNTER_MARKSMANSHIP  ] = TREE_MARKSMANSHIP;
    tree_type[ HUNTER_SURVIVAL      ] = TREE_SURVIVAL;

    // Active
    active_pet             = 0;
    active_aspect          = ASPECT_NONE;
    active_piercing_shots  = 0;
    active_vishanka        = 0;

    merge_piercing_shots = 0;

    // Cooldowns
    cooldowns_explosive_shot = get_cooldown( "explosive_shot " );
    cooldowns_vishanka       = get_cooldown( "vishanka"        );

    ranged_attack = 0;
    summon_pet_str = "";
    distance = 40;
    default_distance = 40;
    base_gcd = timespan_t::from_seconds( 1.0 );
    flaming_arrow = NULL;

    tier13_4pc_cooldown = 105.0;
    vishanka = 0;

    create_talents();
    create_glyphs();
    create_options();
  }

  // Character Definition
  virtual targetdata_t* new_targetdata( player_t* source, player_t* target ) {return new hunter_targetdata_t( source, target );}
  virtual void      init_talents();
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_buffs();
  virtual void      init_values();
  virtual void      init_gains();
  virtual void      init_position();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      init_scaling();
  virtual void      init_actions();
  virtual void      register_callbacks();
  virtual void      combat_begin();
  virtual void      reset();
  virtual double    composite_attack_power() const;
  virtual double    composite_attack_power_multiplier() const;
  virtual double    composite_attack_haste() const;
  virtual double    composite_player_multiplier( const school_type school, action_t* a = NULL ) const;
  virtual double    matching_gear_multiplier( const attribute_type attr ) const;
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() const { return RESOURCE_FOCUS; }
  virtual int       primary_role() const     { return ROLE_ATTACK; }
  virtual bool      create_profile( std::string& profile_str, int save_type=SAVE_ALL, bool save_html=false );
  virtual void      copy_from( player_t* source );
  virtual void      armory_extensions( const std::string& r, const std::string& s, const std::string& c, cache::behavior_t );
  virtual void      moving();

  // Event Tracking
  virtual void regen( timespan_t periodicity );
};

// ==========================================================================
// Hunter Pet
// ==========================================================================

struct hunter_pet_t : public pet_t
{
  action_t* kill_command;

  struct talents_t
  {
    // Common Talents
    talent_t* culling_the_herd;
    talent_t* serpent_swiftness;
    talent_t* spiked_collar;
    talent_t* wild_hunt;

    // Cunning
    talent_t* feeding_frenzy;
    talent_t* owls_focus;
    talent_t* roar_of_recovery;
    talent_t* wolverine_bite;

    // Ferocity
    talent_t* call_of_the_wild;
    talent_t* rabid;
    talent_t* shark_attack;
    talent_t* spiders_bite;
  };
  talents_t talents;

  // Buffs
  buff_t* buffs_bestial_wrath;
  buff_t* buffs_call_of_the_wild;
  buff_t* buffs_culling_the_herd;
  buff_t* buffs_frenzy;
  buff_t* buffs_owls_focus;
  buff_t* buffs_rabid;
  buff_t* buffs_rabid_power_stack;
  buff_t* buffs_sic_em;
  buff_t* buffs_wolverine_bite;

  // Gains
  gain_t* gains_fervor;
  gain_t* gains_focus_fire;
  gain_t* gains_go_for_the_throat;

  // Benefits
  benefit_t* benefits_wild_hunt;

  hunter_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name, pet_type_t pt ) :
    pet_t( sim, owner, pet_name, pt )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = rating_t::interpolate( level, 0, 0, 51, 73 ); // FIXME needs level 60 and 70 values
    main_hand_weapon.max_dmg    = rating_t::interpolate( level, 0, 0, 78, 110 ); // FIXME needs level 60 and 70 values
    // Level 85 numbers from Rivkah from EJ, 07.08.2011
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );

    stamina_per_owner = 0.45;

    health_per_stamina *= 1.05; // 3.1.0 change # Cunning, Ferocity and Tenacity pets now all have +5% damage, +5% armor and +5% health bonuses
    initial_armor_multiplier *= 1.05;


    create_talents();
    create_options();
  }

  virtual int group()
  {
    //assert( pet_type > PET_NONE && pet_type < PET_HUNTER );
    if ( pet_type >= PET_CARRION_BIRD && pet_type <= PET_WOLF         ) return PET_TAB_FEROCITY;
    if ( pet_type >= PET_BEAR         && pet_type <= PET_WORM         ) return PET_TAB_TENACITY;
    if ( pet_type >= PET_BAT          && pet_type <= PET_WIND_SERPENT ) return PET_TAB_CUNNING;
    return PET_NONE;
  }

  virtual const char* unique_special()
  {
    switch ( pet_type )
    {
    case PET_CARRION_BIRD: return "demoralizing_screech";
    case PET_CAT:          return "roar_of_courage";
    case PET_CORE_HOUND:   return NULL;
    case PET_DEVILSAUR:    return "furious_howl/monstrous_bite";
    case PET_HYENA:        return "tendon_rip";
    case PET_MOTH:         return NULL;
    case PET_RAPTOR:       return "tear_armor";
    case PET_SPIRIT_BEAST: return "roar_of_courage";
    case PET_TALLSTRIDER:  return NULL;
    case PET_WASP:         return NULL;
    case PET_WOLF:         return "furious_howl";
    case PET_BEAR:         return NULL;
    case PET_BOAR:         return NULL;
    case PET_CRAB:         return NULL;
    case PET_CROCOLISK:    return NULL;
    case PET_GORILLA:      return NULL;
    case PET_RHINO:        return NULL;
    case PET_SCORPID:      return NULL;
    case PET_SHALE_SPIDER: return NULL;
    case PET_TURTLE:       return NULL;
    case PET_WARP_STALKER: return NULL;
    case PET_WORM:         return NULL;
    case PET_BAT:          return NULL;
    case PET_BIRD_OF_PREY: return NULL;
    case PET_CHIMERA:      return "froststorm_breath";
    case PET_DRAGONHAWK:   return "lightning_breath";
    case PET_NETHER_RAY:   return NULL;
    case PET_RAVAGER:      return "ravage";
    case PET_SERPENT:      return "corrosive_spit";
    case PET_SILITHID:     return "qiraji_fortitude";
    case PET_SPIDER:       return NULL;
    case PET_SPOREBAT:     return NULL;
    case PET_WIND_SERPENT: return "lightning_breath";
    case PET_FOX:          return "tailspin";
    default: break;
    }
    return NULL;
  }

  virtual bool ooc_buffs() { return true; }

  virtual void init_base()
  {
    pet_t::init_base();

    hunter_t* o = owner -> cast_hunter();
    attribute_base[ ATTR_STRENGTH  ] = rating_t::interpolate( level, 0, 162, 331, 476 );
    attribute_base[ ATTR_AGILITY   ] = rating_t::interpolate( level, 0, 54, 113, 438 );
    attribute_base[ ATTR_STAMINA   ] = rating_t::interpolate( level, 0, 307, 361 ); // stamina is different for every pet type
    attribute_base[ ATTR_INTELLECT ] = 100; // FIXME: is 61 at lvl 75. Use /script print(UnitStats("pet",x)); 1=str,2=agi,3=stam,4=int,5=spi
    attribute_base[ ATTR_SPIRIT    ] = 100; // FIXME: is 101 at lvl 75. Values are equal for a cat and a gorilla.

    base_attack_power = -20;
    initial_attack_power_per_strength = 2.0;
    initial_attack_crit_per_agility   = rating_t::interpolate( level, 0.01/16.0, 0.01/30.0, 0.01/62.5, 0.01/243.6 );

    base_attack_crit = 0.05; // Assume 5% base crit as for most other pets. 19/10/2011

    resource_base[ RESOURCE_HEALTH ] = rating_t::interpolate( level, 0, 4253, 6373 );
    resource_base[ RESOURCE_FOCUS ] = 100 + o -> talents.kindred_spirits -> effect1().resource( RESOURCE_FOCUS );

    base_focus_regen_per_second  = ( 24.5 / 4.0 );

    base_focus_regen_per_second *= 1.0 + o -> talents.bestial_discipline -> effect1().percent();

    base_gcd = timespan_t::from_seconds( 1.20 );

    infinite_resource[ RESOURCE_FOCUS ] = o -> infinite_resource[ RESOURCE_FOCUS ];

    world_lag = timespan_t::from_seconds( 0.3 ); // Pet AI latency to get 3.3s claw cooldown as confirmed by Rivkah on EJ, August 2011
    world_lag_override = true;
  }

  virtual void init_talents()
  {
    int tab = group();

    // Common Talents
    talents.culling_the_herd  = find_talent( "Culling the Herd",  tab );
    talents.serpent_swiftness = find_talent( "Serpent Swiftness", tab );
    talents.spiked_collar     = find_talent( "Spiked Collar",     tab );
    talents.wild_hunt         = find_talent( "Wild Hunt",         tab );

    // Ferocity
    talents.call_of_the_wild = find_talent( "Call of the Wild", PET_TAB_FEROCITY );
    talents.rabid            = find_talent( "Rabid",            PET_TAB_FEROCITY );
    talents.spiders_bite     = find_talent( "Spider's Bite",    PET_TAB_FEROCITY );
    talents.shark_attack     = find_talent( "Shark Attack",     PET_TAB_FEROCITY );

    // Cunning
    talents.feeding_frenzy   = find_talent( "Feeding Frenzy",   PET_TAB_CUNNING );
    talents.owls_focus       = find_talent( "Owl's Focus",      PET_TAB_CUNNING );
    talents.roar_of_recovery = find_talent( "Roar of Recovery", PET_TAB_CUNNING );
    talents.wolverine_bite   = find_talent( "Wolverine Bite",   PET_TAB_CUNNING );

    pet_t::init_talents();

    int total_points = 0;
    for ( int i=0; i < MAX_TALENT_TREES; i++ )
    {
      size_t size = talent_trees[i].size();
      for ( size_t j=0; j < size; j++ )
        total_points += talent_trees[ i ][ j ] -> rank();
    }

    // default pet talents
    if ( total_points == 0 )
    {
      talents.serpent_swiftness -> set_rank( 2, true );
      talents.culling_the_herd  -> set_rank( 3, true );
      talents.serpent_swiftness -> set_rank( 3, true );
      talents.spiked_collar     -> set_rank( 3, true );

      if ( tab == PET_TAB_FEROCITY )
      {
        talents.call_of_the_wild -> set_rank( 1, true );
        talents.rabid            -> set_rank( 1, true );
        talents.spiders_bite     -> set_rank( 3, true );
        talents.shark_attack     -> set_rank( 1, true );
        talents.wild_hunt        -> set_rank( 1, true );

        if ( owner -> cast_hunter() -> talents.beast_mastery -> rank() )
        {
          talents.shark_attack -> set_rank( 2, true );
          talents.wild_hunt    -> set_rank( 2, true );
        }
      }
      else if ( tab == PET_TAB_CUNNING )
      {
        talents.feeding_frenzy   -> set_rank( 2, true );
        talents.owls_focus       -> set_rank( 2, true );
        talents.roar_of_recovery -> set_rank( 1, true );
        talents.wolverine_bite   -> set_rank( 1, true );
        talents.wild_hunt        -> set_rank( 2, true );
      }
    }
  }

  virtual void init_buffs()
  {
    pet_t::init_buffs();
    buffs_bestial_wrath     = new buff_t( this, 19574, "bestial_wrath" );
    buffs_call_of_the_wild  = new buff_t( this, 53434, "call_of_the_wild" );
    buffs_culling_the_herd  = new buff_t( this, 70893, "culling_the_herd" );
    buffs_frenzy            = new buff_t( this, 19615, "frenzy_effect" );
    buffs_owls_focus        = new buff_t( this, 53515, "owls_focus", talents.owls_focus-> proc_chance() );
    buffs_rabid             = new buff_t( this, 53401, "rabid" );
    buffs_rabid_power_stack = new buff_t( this, 53403, "rabid_power_stack" );
    buffs_sic_em            = new buff_t( this, 89388, "sic_em" );
    buffs_wolverine_bite    = new buff_t( this, "wolverine_bite",    1, timespan_t::from_seconds( 10.0 ), timespan_t::zero );
  }

  virtual void init_gains()
  {
    pet_t::init_gains();

    gains_fervor            = get_gain( "fervor"            );
    gains_focus_fire        = get_gain( "focus_fire"        );
    gains_go_for_the_throat = get_gain( "go_for_the_throat" );
  }

  virtual void init_benefits()
  {
    pet_t::init_benefits();

    benefits_wild_hunt  = get_benefit( "wild_hunt" );
  }

  virtual void init_actions()
  {
    if ( action_list_str.empty() )
    {

      action_list_str += "/auto_attack";
      action_list_str += "/snapshot_stats";

      if ( talents.call_of_the_wild -> rank() )
      {
        action_list_str += "/call_of_the_wild";
      }
      if ( talents.roar_of_recovery -> rank() )
      {
        action_list_str += "/roar_of_recovery,if=!buff.bloodlust.up";
      }
      if ( talents.rabid -> rank() )
      {
        action_list_str += "/rabid";
      }
      const char* special = unique_special();
      if ( special )
      {
        action_list_str += "/";
        action_list_str += special;
      }
      if ( talents.wolverine_bite -> rank() )
      {
        action_list_str += "/wolverine_bite";
      }
      action_list_str += "/claw";
      action_list_str += "/wait_until_ready";
      action_list_default = 1;
    }

    pet_t::init_actions();
  }

  virtual double composite_attack_power() const
  {
    hunter_t* o = owner -> cast_hunter();

    double ap = pet_t::composite_attack_power();

    ap += o -> composite_attack_power() * 0.425;

    return ap;
  }

  virtual double composite_attack_power_multiplier() const
  {
    double mult = pet_t::composite_attack_power_multiplier();

    mult *= 1.0 + buffs_rabid_power_stack -> stack() * buffs_rabid_power_stack -> effect1().percent();

    if ( buffs_call_of_the_wild -> up() )
      mult *= 1.0 + buffs_call_of_the_wild -> effect1().percent();

    return mult;
  }

  virtual double composite_attack_crit() const
  {
    hunter_t* o = owner -> cast_hunter();

    double ac = pet_t::composite_attack_crit();

    ac += o -> composite_attack_crit();

    return ac;
  }

  virtual double composite_attack_haste() const
  {
    hunter_t* o = owner -> cast_hunter();

    double h = o -> composite_attack_haste();

    // Pets do not scale with haste from certain buffs on the owner

    if ( o -> buffs.bloodlust -> check() )
      h *= 1.30;

    h *= 1.0 + o -> buffs_rapid_fire -> check() * o -> buffs_rapid_fire -> current_value;

    return h;
  }

  virtual double composite_attack_hit() const
  {
    hunter_t* o = owner -> cast_hunter();

    // Hunter pets' hit does not round down in cataclysm and scales with Heroic Presence (as of 12/21)
    return o -> composite_attack_hit();
  }

  virtual double composite_attack_expertise() const
  {
    hunter_t* o = owner -> cast_hunter();

    return ( ( 100.0 * o -> attack_hit ) * 26.0 / 8.0 ) / 100.0;
  }

  virtual double composite_spell_hit() const
  {
    return composite_attack_hit() * 17.0 / 8.0;
  }

  virtual void summon( timespan_t duration=timespan_t::zero )
  {
    hunter_t* o = owner -> cast_hunter();

    pet_t::summon( duration );

    o -> active_pet = this;
  }

  virtual void demise()
  {
    hunter_t* o = owner -> cast_hunter();

    pet_t::demise();

    o -> active_pet = 0;
  }

  virtual double composite_player_multiplier( const school_type school, action_t* a ) const
  {
    double m = pet_t::composite_player_multiplier( school, a );

    hunter_t*     o = owner -> cast_hunter();

    if ( o -> passive_spells.master_of_beasts -> ok() )
    {
      m *= 1.0 + o -> passive_spells.master_of_beasts -> effect1().coeff() / 100.0 * o -> composite_mastery();
    }

    m *= 1.0 + talents.shark_attack -> effect1().percent();

    return m;
  }

  virtual int primary_resource() const { return RESOURCE_FOCUS; }

  virtual action_t* create_action( const std::string& name, const std::string& options_str );

  virtual void init_spells();
};

namespace { // ANONYMOUS NAMESPACE =========================================

// ==========================================================================
// Hunter Attack
// ==========================================================================

struct hunter_attack_t : public attack_t
{
  bool  consumes_tier12_4pc;

  void _init_hunter_attack_t()
  {
    may_crit               = true;
    tick_may_crit          = true;
    normalize_weapon_speed = true;
    consumes_tier12_4pc    = false;
    dot_behavior           = DOT_REFRESH;
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

  virtual double cost() const;
  virtual void   consume_resource();
  virtual void   execute();
  virtual timespan_t execute_time() const;
  virtual double swing_haste() const;
  virtual void   player_buff();
};

// ==========================================================================
// Hunter Spell
// ==========================================================================

struct hunter_spell_t : public spell_t
{
  bool consumes_tier12_4pc;

  void _init_hunter_spell_t()
  {
  }

  hunter_spell_t( const char* n, player_t* p, const school_type s, int t=TREE_NONE ) :
    spell_t( n, p, RESOURCE_FOCUS, s, t ), consumes_tier12_4pc( false )
  {
    _init_hunter_spell_t();
  }
  hunter_spell_t( const char* n, player_t* p, const char* sname ) :
    spell_t( n, sname, p ), consumes_tier12_4pc( false )
  {
    _init_hunter_spell_t();
  }
  hunter_spell_t( const char* n, player_t* p, const uint32_t id ) :
    spell_t( n, id, p ), consumes_tier12_4pc( false )
  {
    _init_hunter_spell_t();
  }

  virtual timespan_t gcd() const;
  virtual double cost() const;
  virtual void consume_resource();
};

// trigger_go_for_the_throat ================================================

static void trigger_go_for_the_throat( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.go_for_the_throat -> rank() )
    return;

  if ( ! p -> active_pet )
    return;

  p -> active_pet -> resource_gain( RESOURCE_FOCUS, p -> talents.go_for_the_throat -> effect1().base_value(), p -> active_pet -> gains_go_for_the_throat );
}

// trigger_piercing_shots ===================================================

static void trigger_piercing_shots( action_t* a, double dmg )
{
  hunter_t* p = a -> player -> cast_hunter();
  sim_t* sim = a -> sim;

  if ( ! p -> talents.piercing_shots -> rank() )
    return;

  struct piercing_shots_t : public attack_t
  {
    piercing_shots_t( player_t* p ) : attack_t( "piercing_shots", 63468, p )
    {
      may_miss      = false;
      may_crit      = true;
      background    = true;
      proc          = true;
      hasted_ticks  = false;
      tick_may_crit = false;
      may_resist    = true;
      dot_behavior  = DOT_REFRESH;

      base_multiplier = 1.0;
      tick_power_mod  = 0;
      num_ticks       = 8;
      base_tick_time  = timespan_t::from_seconds( 1.0 );

      init();
    }

    void player_buff() {}

    void target_debuff( player_t* t, int /* dmg_type */ )
    {
      if ( t -> debuffs.mangle -> up() || t -> debuffs.blood_frenzy_bleed -> up() || t -> debuffs.hemorrhage -> up() || t -> debuffs.tendon_rip -> up() )
      {
        target_multiplier = 1.30;
      }
    }

    virtual void impact( player_t* t, int impact_result, double piercing_shots_dmg )
    {
      attack_t::impact( t, impact_result, 0 );

      // FIXME: Is a is_hit check necessary here?
      base_td = piercing_shots_dmg / dot() -> num_ticks;
    }

    virtual timespan_t travel_time()
    {
      return sim -> gauss( sim -> aura_delay, 0.25 * sim -> aura_delay );
    }

    virtual double total_td_multiplier() const { return target_multiplier; }
  };

  double piercing_shots_dmg = p -> talents.piercing_shots -> effect1().percent() * dmg;

  if ( p -> merge_piercing_shots > 0 ) // Does not report Piercing Shots seperately.
  {
    int result = a -> result;
    a -> result = RESULT_HIT;
    a -> assess_damage( a -> target, piercing_shots_dmg * p -> merge_piercing_shots, DMG_OVER_TIME, a -> result );
    a -> result = result;
    return;
  }

  if ( ! p -> active_piercing_shots ) p -> active_piercing_shots = new piercing_shots_t( p );

  dot_t* dot = p -> active_piercing_shots -> dot();

  if ( dot -> ticking )
  {
    piercing_shots_dmg += p -> active_piercing_shots -> base_td * dot -> ticks();
  }

  if ( timespan_t::from_seconds( 8.0 ) + sim -> aura_delay < dot -> remains() )
  {
    if ( sim -> log ) log_t::output( sim, "Player %s munches Piercing Shots due to Max Piercing Shots Duration.", p -> name() );
    p -> procs_munched_piercing_shots -> occur();
    return;
  }

  if ( p -> active_piercing_shots -> travel_event )
  {
    // There is an SPELL_AURA_APPLIED already in the queue, which will get munched.
    if ( sim -> log ) log_t::output( sim, "Player %s munches previous Piercing Shots due to Aura Delay.", p -> name() );
    p -> procs_munched_piercing_shots -> occur();
  }

  p -> active_piercing_shots -> direct_dmg = piercing_shots_dmg;
  p -> active_piercing_shots -> result = RESULT_HIT;
  p -> active_piercing_shots -> schedule_travel( a -> target );

  if ( p -> active_piercing_shots -> travel_event && dot -> ticking )
  {
    if ( dot -> tick_event -> occurs() < p -> active_piercing_shots -> travel_event -> occurs() )
    {
      // Piercing Shots will tick before SPELL_AURA_APPLIED occurs, which means that the current Piercing Shots will
      // both tick -and- get rolled into the next Piercing Shots.
      if ( sim -> log ) log_t::output( sim, "Player %s rolls Piercing Shots.", p -> name() );
      p -> procs_rolled_piercing_shots -> occur();
    }
  }
}

// trigger_thrill_of_the_hunt ===============================================

static void trigger_thrill_of_the_hunt( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.thrill_of_the_hunt -> rank() )
    return;

  if ( p -> rng_thrill_of_the_hunt -> roll ( p -> talents.thrill_of_the_hunt -> proc_chance() ) )
  {
    double gain = a -> base_cost * p -> talents.thrill_of_the_hunt -> effect1().percent();

    p -> procs_thrill_of_the_hunt -> occur();
    p -> resource_gain( RESOURCE_FOCUS, gain, p -> gains_thrill_of_the_hunt );
  }
}

// trigger_tier12_2pc_melee =================================================

static void trigger_tier12_2pc_melee( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> set_bonus.tier12_2pc_melee() ) return;

  if ( ! a -> weapon ) return;
  if ( a -> weapon -> slot != SLOT_RANGED ) return;
  if ( a -> proc ) return;

  assert( p -> flaming_arrow );

  if ( p -> rng_tier12_2pc -> roll( p -> dbc.spell( 99057 ) -> proc_chance() ) )
  {
    p -> procs_flaming_arrow -> occur();
    p -> flaming_arrow -> execute();
  }
}

// trigger_vishanka =========================================================

static void trigger_vishanka( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> vishanka )
    return;

  if ( p -> cooldowns_vishanka -> remains() > timespan_t::zero )
    return;

  if ( ! p -> active_vishanka )
  {
    struct vishanka_t : public hunter_attack_t
    {
      vishanka_t( hunter_t* p, uint32_t id ) :
        hunter_attack_t( "vishanka_jaws_of_the_earth", p, id )
      {
        background  = true;
        proc        = true;
        trigger_gcd = timespan_t::zero;
        crit_bonus = 0.5; // Only crits for 150% on live
        init();

        // FIX ME: Can this crit, miss, etc?
        may_miss    = false;
      }
    };

    uint32_t id = p -> dbc.spell( p -> vishanka ) -> effect1().trigger_spell_id();

    p -> active_vishanka = new vishanka_t( p, id );
  }

  if ( a -> sim -> roll( p -> dbc.spell( p -> vishanka ) -> proc_chance() ) )
  {
    p -> active_vishanka -> execute();
    p -> cooldowns_vishanka -> duration = timespan_t::from_seconds( 15.0 ); // Assume a ICD until testing proves one way or another
    p -> cooldowns_vishanka -> start();
  }
}

// ==========================================================================
// Hunter Pet Attacks
// ==========================================================================

struct hunter_pet_attack_t : public attack_t
{
  void _init_hunter_pet_attack_t()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    may_crit = true;

    base_crit += p -> talents.spiders_bite -> effect1().percent();

    // Assume happy pet
    base_multiplier *= 1.25;

    // 3.1.0 change # Cunning, Ferocity and Tenacity pets now all have +5% damage, +5% armor and +5% health bonuses
    base_multiplier *= 1.05;
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

  virtual double swing_haste() const
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t* o = p -> owner -> cast_hunter();

    double h = attack_t::swing_haste();

    h *= 1.0 / ( 1.0 + p -> talents.serpent_swiftness -> effect1().percent() );

    h *= 1.0 / ( 1.0 + o -> talents.frenzy -> effect1().percent() * p -> buffs_frenzy -> stack() );

    return h;
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    attack_t::execute();

    if ( result_is_hit() )
    {
      if ( p -> buffs_rabid -> up() )
      {
        p -> buffs_rabid_power_stack -> trigger();
      }
      if ( result == RESULT_CRIT )
      {
        if ( p -> talents.wolverine_bite -> rank() )
        {
          p -> buffs_wolverine_bite -> trigger();
        }
      }
    }
  }

  virtual void player_buff()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    attack_t::player_buff();

    if ( p -> buffs_bestial_wrath -> up() )
      player_multiplier *= 1.0 + p -> buffs_bestial_wrath -> effect2().percent();

    if ( p -> buffs_culling_the_herd -> up() )
    {
      player_multiplier *= 1.0 + ( p -> buffs_culling_the_herd -> effect1().percent() );
    }

  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    attack_t::target_debuff( t, dmg_type );

    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    if ( p -> talents.feeding_frenzy -> rank() && ( t -> health_percentage() < 35 ) )
    {
      player_multiplier *= 1.0 + p -> talents.feeding_frenzy -> effect1().percent();
    }
  }
};

// Pet Melee ================================================================

struct pet_melee_t : public hunter_pet_attack_t
{
  pet_melee_t( hunter_pet_t* player ) :
    hunter_pet_attack_t( "melee", player, 0, false )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    weapon = &( p -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    background        = true;
    repeating         = true;
    school = SCHOOL_PHYSICAL;
    stats -> school = school;

  }
};

// Pet Auto Attack ==========================================================

struct pet_auto_attack_t : public hunter_pet_attack_t
{
  pet_auto_attack_t( hunter_pet_t* player, const std::string& /* options_str */ ) :
    hunter_pet_attack_t( "auto_attack", player, 0 )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    p -> main_hand_attack = new pet_melee_t( player );
    trigger_gcd = timespan_t::zero;
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

// Pet Claw =================================================================

struct claw_t : public hunter_pet_attack_t
{
  claw_t( hunter_pet_t* p, const std::string& options_str ) :
    hunter_pet_attack_t( "claw", 16827, p )
  {
    parse_options( NULL, options_str );
    direct_power_mod = 0.2; // hardcoded into tooltip
    base_multiplier *= 1.0 + p -> talents.spiked_collar -> effect1().percent();
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t* o     = p -> owner -> cast_hunter();
    hunter_pet_attack_t::execute();

    p -> buffs_sic_em -> expire();
    p -> buffs_owls_focus -> expire();
    o -> buffs_cobra_strikes -> decrement();
    if ( o -> talents.frenzy -> rank() )
      p -> buffs_frenzy -> trigger( 1 );
    if ( result == RESULT_CRIT )
    {
      o -> resource_gain( RESOURCE_FOCUS, o -> talents.invigoration -> effect1().resource( RESOURCE_FOCUS ), o -> gains_invigoration );
      if ( p -> talents.culling_the_herd -> rank() )
      {
        p -> buffs_culling_the_herd -> trigger();
        o -> buffs_culling_the_herd -> trigger();
      }
    }

    p -> buffs_owls_focus -> trigger();
  }

  virtual void player_buff()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    hunter_pet_attack_t::player_buff();

    if ( o -> buffs_cobra_strikes -> up() )
      player_crit += o -> buffs_cobra_strikes -> effect1().percent();

    if ( p -> talents.wild_hunt -> rank() && ( p -> resource_current[ RESOURCE_FOCUS ] > 50 ) )
    {
      p -> benefits_wild_hunt -> update( true );
      player_multiplier *= 1.0 + p -> talents.wild_hunt -> effect1().percent();
    }
    else
      p -> benefits_wild_hunt -> update( false );

    // Active Benefit-Calculation.
    p -> buffs_owls_focus -> up();
    p -> buffs_sic_em -> up();
  }

  virtual double cost() const
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t* o     = p -> owner -> cast_hunter();

    double basec = hunter_pet_attack_t::cost();

    double c = basec;

    if ( c == 0 )
      return 0;

    if ( p -> buffs_owls_focus -> check() )
      return 0;

    if ( p -> talents.wild_hunt -> rank() && ( p -> resource_current[ RESOURCE_FOCUS ] > 50 ) )
      c *= 1.0 + p -> talents.wild_hunt -> effect2().percent();

    if ( p -> buffs_sic_em -> check() )
      c += basec * o -> talents.sic_em -> effect1().percent();

    if ( c < 0 )
      c = 0;

    return c;
  }
};

// Devilsaur Monstrous Bite =================================================

struct monstrous_bite_t : public hunter_pet_attack_t
{
  monstrous_bite_t( hunter_pet_t* p, const std::string& options_str ) :
    hunter_pet_attack_t( "monstrous_bite", 54680, p )
  {
    hunter_t* o = p -> owner -> cast_hunter();

    parse_options( NULL, options_str );
    cooldown -> duration *=  ( 1.0 + o -> talents.longevity -> effect1().percent() );
    auto_cast = true;
    school = SCHOOL_PHYSICAL;
    stats -> school = SCHOOL_PHYSICAL;
  }
};

// Wolverine Bite ===========================================================

struct wolverine_bite_t : public hunter_pet_attack_t
{
  wolverine_bite_t( hunter_pet_t* p, const std::string& options_str ) :
    hunter_pet_attack_t( "wolverine_bite", p, "Wolverine Bite" )
  {
    hunter_t* o = p -> owner -> cast_hunter();

    parse_options( NULL, options_str );

    base_dd_min = base_dd_max  = 1;
    cooldown -> duration *=  ( 1.0 + o -> talents.longevity -> effect1().percent() );
    auto_cast   = true;
    direct_power_mod = 0.10; // hardcoded into the tooltip

    may_dodge = may_block = may_parry = false;
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    p -> buffs_wolverine_bite -> up();
    p -> buffs_wolverine_bite -> expire();

    hunter_pet_attack_t::execute();
  }

  virtual bool ready()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    if ( ! p -> buffs_wolverine_bite -> check() )
      return false;

    return hunter_pet_attack_t::ready();
  }
};

// Kill Command (pet) =======================================================

struct pet_kill_command_t : public hunter_pet_attack_t
{
  pet_kill_command_t( hunter_pet_t* p ) :
    hunter_pet_attack_t( "kill_command", 83381, p )
  {
    hunter_t*     o = p -> owner -> cast_hunter();
    background = true;
    proc=true;

    base_crit += o -> talents.improved_kill_command -> effect1().percent();
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();
    hunter_pet_attack_t::execute();

    o -> buffs_killing_streak -> expire();

    if ( result == RESULT_CRIT )
    {
      o -> buffs_killing_streak_crits -> increment();
      if ( o ->buffs_killing_streak_crits -> stack() == 2 )
      {
        o -> buffs_killing_streak -> trigger();
        o -> buffs_killing_streak_crits -> expire();
      }
    }
    else
    {
      o -> buffs_killing_streak_crits -> expire();
    }
  }

  virtual void player_buff()
  {
    hunter_pet_attack_t::player_buff();
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    if ( o -> buffs_killing_streak -> up() )
      player_multiplier *= 1.0 + o -> talents.killing_streak -> effect1().percent();
  }
};

// ==========================================================================
// Hunter Pet Spells
// ==========================================================================

struct hunter_pet_spell_t : public spell_t
{
  void _init_hunter_pet_spell_t()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    base_crit += p -> talents.spiders_bite -> effect1().percent();

    base_multiplier *= 1.25; // Assume happy pet

    // 3.1.0 change # Cunning, Ferocity and Tenacity pets now all have +5% damage, +5% armor and +5% health bonuses
    base_multiplier *= 1.05;
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

  virtual void player_buff()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    spell_t::player_buff();

    player_spell_power += 0.125 * o -> composite_attack_power();

    if ( p -> buffs_bestial_wrath -> up() )
      player_multiplier *= 1.0 + p -> buffs_bestial_wrath -> effect2().percent();

    if ( p -> buffs_culling_the_herd -> up() )
    {
      player_multiplier *= 1.0 + ( p -> buffs_culling_the_herd -> effect1().percent() );
    }

  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    spell_t::target_debuff( t, dmg_type );

    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    if ( p -> talents.feeding_frenzy -> rank() && ( t -> health_percentage() < 35 ) )
    {
      player_multiplier *= 1.0 + p -> talents.feeding_frenzy -> effect1().percent();
    }
  }
};

// Call of the Wild =========================================================

struct call_of_the_wild_t : public hunter_pet_spell_t
{
  call_of_the_wild_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "call_of_the_wild", player, "Call of the Wild" )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    parse_options( NULL, options_str );

    cooldown -> duration *=  ( 1.0 + o -> talents.longevity -> effect1().percent() );

    harmful = false;
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

// Rabid ====================================================================

struct rabid_t : public hunter_pet_spell_t
{
  rabid_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "rabid", player, 53401 )
  {
    hunter_t* o = player -> cast_pet() -> owner -> cast_hunter();

    parse_options( NULL, options_str );
    cooldown -> duration *=  ( 1.0 + o -> talents.longevity -> effect1().percent() );
  }

  virtual void execute()
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();

    p -> buffs_rabid_power_stack -> expire();
    p -> buffs_rabid -> trigger();

    hunter_pet_spell_t::execute();
  }
};

// Roar of Recovery =========================================================

struct roar_of_recovery_t : public hunter_pet_spell_t
{
  roar_of_recovery_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "roar_of_recovery", player, "Roar of Recovery" )
  {
    hunter_t* o = player -> cast_pet() -> owner -> cast_hunter();

    parse_options( 0, options_str );

    cooldown -> duration *=  ( 1.0 + o -> talents.longevity -> effect1().percent() );
    auto_cast = true;
    harmful   = false;
  }

  virtual void tick( dot_t* d )
  {
    hunter_t* o = player -> cast_pet() -> owner -> cast_hunter();

    hunter_pet_spell_t::tick( d );

    o -> resource_gain( RESOURCE_FOCUS, effect1().base_value(), o -> gains_roar_of_recovery );
  }

  virtual bool ready()
  {
    hunter_t* o = player -> cast_pet() -> owner -> cast_hunter();

    if ( o -> resource_current[ RESOURCE_FOCUS ] > 50 )
      return false;

    return hunter_pet_spell_t::ready();
  }
};

// ==========================================================================
// Unique Pet Specials
// ==========================================================================

// Wolf/Devilsaur Furious Howl ==============================================

struct furious_howl_t : public hunter_pet_spell_t
{
  furious_howl_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "furious_howl", player, "Furious Howl" )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    parse_options( NULL, options_str );

    cooldown -> duration *=  ( 1.0 + o -> talents.longevity -> effect1().percent() );

    harmful = false;
  }

  virtual void execute()
  {
    hunter_pet_spell_t::execute();

    for ( player_t* pl = sim -> player_list; pl; pl = pl -> next )
    {
      if ( pl -> is_pet() )
        continue;

      pl -> buffs.furious_howl -> trigger();
    }
  }
};

// Cat/Spirit Beast Roar of Courage =========================================

struct roar_of_courage_t : public hunter_pet_spell_t
{
  double bonus;

  roar_of_courage_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "roar_of_courage", player, "Roar of Courage" )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    parse_options( NULL, options_str );

    cooldown -> duration *=  ( 1.0 + o -> talents.longevity -> effect1().percent() );

    harmful = false;
    bonus = effect1().base_value();
  }

  virtual void execute()
  {
    hunter_pet_spell_t::execute();

    if ( ! sim -> overrides.roar_of_courage )
    {
      sim -> auras.roar_of_courage -> buff_duration = duration();
      sim -> auras.roar_of_courage -> trigger( 1, bonus );
    }
  }
};

// Silithid Qiraji Fortitude  ===============================================

struct qiraji_fortitude_t : public hunter_pet_spell_t
{
  double bonus;

  qiraji_fortitude_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "qiraji_fortitude", player, "Qiraji Fortitude" )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    parse_options( NULL, options_str );

    cooldown -> duration *=  ( 1.0 + o -> talents.longevity -> effect1().percent() );

    harmful = false;
    bonus = effect1().base_value();
  }

  virtual void execute()
  {
    hunter_pet_spell_t::execute();

    if ( ! sim -> overrides.qiraji_fortitude )
    {
      sim -> auras.qiraji_fortitude -> trigger( 1, bonus );
    }
  }

  virtual bool ready()
  {
    if ( sim -> auras.qiraji_fortitude -> check() )
      return false;

    return hunter_pet_spell_t::ready();
  }
};

// Wind Serpent Lightning Breath ============================================

struct lightning_breath_t : public hunter_pet_spell_t
{
  lightning_breath_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "lightning_breath", player, "Lightning Breath" )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    parse_options( 0, options_str );

    cooldown -> duration *=  ( 1.0 + o -> talents.longevity -> effect1().percent() );
    auto_cast = true;
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    hunter_pet_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
      t -> debuffs.lightning_breath -> expire();
      t -> debuffs.lightning_breath -> trigger( 1, effect1().base_value() );
      t -> debuffs.lightning_breath -> source = p;
    }
  }

};

// Serpent Corrosive Spit  ==================================================

struct corrosive_spit_t : public hunter_pet_spell_t
{
  corrosive_spit_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "corrosive_spit", player, 95466 )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    parse_options( 0, options_str );

    cooldown -> duration *=  ( 1.0 + o -> talents.longevity -> effect1().percent() );
    auto_cast = true;
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    hunter_pet_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
      t -> debuffs.corrosive_spit -> trigger( 1, -1*effect1().base_value() );
  }
};

// Demoralizing Screech  ====================================================

struct demoralizing_screech_t : public hunter_pet_spell_t
{
  demoralizing_screech_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "demoralizing_screech", player, 24423 )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    parse_options( 0, options_str );

    cooldown -> duration *=  ( 1.0 + o -> talents.longevity -> effect1().percent() );
    auto_cast = true;
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    hunter_pet_spell_t::impact( t, impact_result, travel_dmg );

    //TODO: Is actually an aoe ability
    if ( result_is_hit( impact_result ) )
    {
      hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
      t -> debuffs.demoralizing_screech -> expire();
      t -> debuffs.demoralizing_screech -> trigger( 1, effect1().base_value() );
      t -> debuffs.demoralizing_screech -> source = p;
    }
  }
};

// Ravager Ravage ===========================================================

struct ravage_t : public hunter_pet_spell_t
{
  ravage_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "ravage", player, 50518 )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    parse_options( 0, options_str );

    cooldown -> duration *=  ( 1.0 + o -> talents.longevity -> effect1().percent() );
    auto_cast = true;
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    hunter_pet_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
      t -> debuffs.ravage -> expire();
      t -> debuffs.ravage -> trigger( 1, effect1().base_value() );
      t -> debuffs.ravage -> source = p;
    }
  }
};

// Fox Tailspin  ============================================================

struct tailspin_t : public hunter_pet_spell_t
{
  tailspin_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "tailspin", player, 90314 )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    parse_options( 0, options_str );

    cooldown -> duration *=  ( 1.0 + o -> talents.longevity -> effect1().percent() );
    auto_cast = true;
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    hunter_pet_spell_t::impact( t, impact_result, travel_dmg );

    //TODO: Is actually an aoe ability
    if ( result_is_hit( impact_result ) )
    {
      hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
      t -> debuffs.tailspin -> expire();
      t -> debuffs.tailspin -> trigger( 1, effect1().base_value() );
      t -> debuffs.tailspin -> source = p;
    }
  }
};

// Raptor Tear Armor  =======================================================

struct tear_armor_t : public hunter_pet_spell_t
{
  tear_armor_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "tear_armor", player, 95467 )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    parse_options( 0, options_str );

    cooldown -> duration *=  ( 1.0 + o -> talents.longevity -> effect1().percent() );
    auto_cast = true;
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    hunter_pet_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      t -> debuffs.tear_armor -> trigger( 1, -1*effect1().base_value() );
    }
  }
};

// Hyena Tendon Rip =========================================================

struct tendon_rip_t : public hunter_pet_spell_t
{
  tendon_rip_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "tendon_rip", player, 50271 )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    parse_options( 0, options_str );

    cooldown -> duration *=  ( 1.0 + o -> talents.longevity -> effect1().percent() );
    auto_cast = true;
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    hunter_pet_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
      t -> debuffs.tendon_rip -> expire();
      t -> debuffs.tendon_rip -> trigger( 1, effect1().base_value() );
      t -> debuffs.tendon_rip -> source = p;
    }
  }
};

// Chimera Froststorm Breath ================================================

struct froststorm_breath_t : public hunter_pet_spell_t
{
  struct froststorm_breath_tick_t : public hunter_pet_spell_t
  {
    froststorm_breath_tick_t( player_t* player ) :
      hunter_pet_spell_t( "froststorm_breath_tick", player, 95725 )
    {
      direct_power_mod = 0.24; // hardcoded into tooltip, 17/10/2011
      background  = true;
      direct_tick = true;

      stats = player -> get_stats( "froststorm_breath", this );
    }
  };

  froststorm_breath_tick_t* tick_spell;

  froststorm_breath_t( player_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "froststorm_breath", player, "Froststorm Breath" )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) player -> cast_pet();
    hunter_t*     o = p -> owner -> cast_hunter();

    channeled = true;

    parse_options( NULL, options_str );

    cooldown -> duration *=  ( 1.0 + o -> talents.longevity -> effect1().percent() );
    auto_cast = true;

    tick_spell = new froststorm_breath_tick_t( p );

    add_child( tick_spell );
  }

  virtual void tick( dot_t* d )
  {
    tick_spell -> execute();
    stats -> add_tick( d -> time_to_tick );
  }
};

// ==========================================================================
// Hunter Attacks
// ==========================================================================

// hunter_attack_t::cost ====================================================

double hunter_attack_t::cost() const
{
  hunter_t* p = player -> cast_hunter();

  double c = attack_t::cost();

  if ( c == 0 )
    return 0;

  if ( p -> buffs_tier12_4pc -> check() && consumes_tier12_4pc )
    return 0;

  if ( p -> buffs_beast_within -> check() )
    c *= ( 1.0 + p -> buffs_beast_within -> effect1().percent() );

  return c;
}

// hunter_attack_t::consume_resouce =========================================

void hunter_attack_t::consume_resource()
{
  attack_t::consume_resource();

  hunter_t* p = player -> cast_hunter();

  if ( p -> buffs_tier12_4pc -> up() )
  {
    // Treat the savings like a gain
    double amount = attack_t::cost();
    if ( amount > 0 )
    {
      p -> gains_tier12_4pc -> add( amount );
      p -> gains_tier12_4pc -> type = RESOURCE_FOCUS;
      p -> buffs_tier12_4pc -> expire();
    }
  }
}

// hunter_attack_t::execute =================================================

void hunter_attack_t::execute()
{
  attack_t::execute();
  hunter_t* p = player -> cast_hunter();

  if ( p -> talents.improved_steady_shot -> rank() )
    trigger_improved_steady_shot();

  if ( p -> buffs_pre_improved_steady_shot -> stack() == 2 )
  {
    p -> buffs_improved_steady_shot -> trigger();
    p -> buffs_pre_improved_steady_shot -> expire();
  }

  if ( result_is_hit() )
    trigger_vishanka( this );
}

// hunter_attack_t::execute_time ============================================

timespan_t hunter_attack_t::execute_time() const
{
  timespan_t t = attack_t::execute_time();

  if ( t == timespan_t::zero  || base_execute_time < timespan_t::zero )
    return timespan_t::zero;

  return t;
}

// hunter_attack_t::player_buff =============================================

void hunter_attack_t::player_buff()
{
  hunter_t* p = player -> cast_hunter();

  attack_t::player_buff();

  if (  p -> buffs_beast_within -> up() )
    player_multiplier *= 1.0 + p -> buffs_beast_within -> effect2().percent();

  hunter_targetdata_t* td = targetdata() -> cast_hunter();
  if ( td -> dots_serpent_sting -> ticking )
    player_multiplier *= 1.0 + p -> talents.noxious_stings -> effect1().percent();

  if ( p -> buffs_culling_the_herd -> up() )
    player_multiplier *= 1.0 + ( p -> buffs_culling_the_herd -> effect1().percent() );
}

// hunter_attack_t::swing_haste =============================================

double hunter_attack_t::swing_haste() const
{
  hunter_t* p = player -> cast_hunter();

  double h = attack_t::swing_haste();

  if ( p -> buffs_improved_steady_shot -> up() )
    h *= 1.0/ ( 1.0 + p -> talents.improved_steady_shot -> effect1().percent() );

  return h;
}

// Ranged Attack ============================================================

struct ranged_t : public hunter_attack_t
{
  ranged_t( player_t* player, const char* name="ranged" ) :
    hunter_attack_t( name, player, SCHOOL_PHYSICAL, TREE_NONE, /*special*/true )
  {
    hunter_t* p = player -> cast_hunter();

    weapon = &( p -> ranged_weapon );
    base_execute_time = weapon -> swing_time;

    normalize_weapon_speed=false;
    may_crit    = true;
    background  = true;
    repeating   = true;
  }

  virtual void calculate_result()
  {
    special = true;
    hunter_attack_t::calculate_result();
    special = false;
  }

  virtual timespan_t execute_time() const
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return hunter_attack_t::execute_time();
  }

  virtual void trigger_improved_steady_shot()
  { }

  virtual void execute()
  {
    hunter_attack_t::execute();

    if ( result_is_hit() )
    {
      hunter_t* p = player -> cast_hunter();

      p -> buffs_tier12_4pc -> trigger();

      if ( result == RESULT_CRIT )
        trigger_go_for_the_throat( this );
    }
  }

  virtual void player_buff()
  {
    hunter_attack_t::player_buff();

    hunter_t* p = player -> cast_hunter();

    player_multiplier *= 1.0 + p -> passive_spells.artisan_quiver -> mod_additive( P_GENERIC );
  }
};

// Auto Shot ================================================================

struct auto_shot_t : public hunter_attack_t
{
  auto_shot_t( player_t* player, const std::string& /* options_str */ ) :
    hunter_attack_t( "auto_shot", player )
  {
    hunter_t* p = player -> cast_hunter();

    p -> ranged_attack = new ranged_t( player );

    trigger_gcd = timespan_t::zero;
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

  virtual timespan_t execute_time() const
  {
    double h = 1.0;

    h *= 1.0 / ( 1.0 + std::max( sim -> auras.hunting_party       -> value(),
                       std::max( sim -> auras.windfury_totem      -> value(),
                                 sim -> auras.improved_icy_talons -> value() ) ) );

    return hunter_attack_t::execute_time() * h;
  }
};

// Aimed Shot ===============================================================

struct aimed_shot_t : public hunter_attack_t
{

  // Aimed Shot - Master Marksman ===========================================

  struct aimed_shot_mm_t : public hunter_attack_t
  {
    aimed_shot_mm_t( player_t* player, const std::string& options_str=std::string() ) :
      hunter_attack_t( "aimed_shot_mm", player, 82928 )
    {
      hunter_t* p = player -> cast_hunter();
      check_spec ( TREE_MARKSMANSHIP );
      if ( ! options_str.empty() ) parse_options( NULL, options_str );

      // Don't know why these values aren't 0 in the database.
      base_cost = 0;
      base_execute_time = timespan_t::zero;

      // Hotfix on Feb 18th, 2011: http://blue.mmo-champion.com/topic/157148/patch-406-hotfixes-february-18
      // Testing confirms that the weapon multiplier also affects the RAP coeff
      // and the base damage of the shot. Probably a bug on Blizzard's end.
      direct_power_mod  = 0.724; // hardcoded into tooltip
      direct_power_mod *= weapon_multiplier;

      weapon = &( p -> ranged_weapon );
      assert( weapon -> group() == WEAPON_RANGED );
      base_dd_min *= weapon_multiplier; // Aimed Shot's weapon multiplier applies to the base damage as well
      base_dd_max *= weapon_multiplier;

      normalize_weapon_speed = true;
    }

    virtual void target_debuff( player_t* t, int dmg_type )
    {
      hunter_t* p = player -> cast_hunter();

      hunter_attack_t::target_debuff( t, dmg_type );

      if ( p -> talents.careful_aim -> rank() && t-> health_percentage() > p -> talents.careful_aim -> effect2().base_value() )
      {
        target_crit += p -> talents.careful_aim -> effect1().percent();
      }
    }

    virtual void execute()
    {
      hunter_t* p = player -> cast_hunter();

      hunter_attack_t::execute();

      if ( result == RESULT_CRIT )
      {
        if ( p -> active_pet )
          p -> active_pet -> buffs_sic_em -> trigger();

        p -> resource_gain( RESOURCE_FOCUS, p -> glyphs.aimed_shot -> effect1().resource( RESOURCE_FOCUS ), p -> gains_glyph_aimed_shot );
      }

      p -> buffs_master_marksman_fire -> expire();
    }

    virtual void impact( player_t* t, int impact_result, double travel_dmg )
    {
      hunter_attack_t::impact( t, impact_result, travel_dmg );

      if ( impact_result == RESULT_CRIT )
        trigger_piercing_shots( this, travel_dmg );
    }
  };

  aimed_shot_mm_t* as_mm;
  int casted;

  aimed_shot_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "aimed_shot", player, "Aimed Shot" ), as_mm( 0 )
  {
    hunter_t* p = player -> cast_hunter();
    check_spec ( TREE_MARKSMANSHIP );
    parse_options( NULL, options_str );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );
    normalize_weapon_speed = true;
    base_dd_min *= weapon_multiplier; // Aimed Shot's weapon multiplier applies to the base damage as well
    base_dd_max *= weapon_multiplier;

    casted = 0;

    // Hotfix on Feb 18th, 2011: http://blue.mmo-champion.com/topic/157148/patch-406-hotfixes-february-18
    // Testing confirms that the weapon multiplier also affects the RAP coeff
    // and the base damage of the shot. Probably a bug on Blizzard's end.
    direct_power_mod  = 0.724; // hardcoded into tooltip
    direct_power_mod *= weapon_multiplier;

    as_mm = new aimed_shot_mm_t( p );
    as_mm -> background = true;

    consumes_tier12_4pc = true;
  }

  virtual double cost() const
  {
    hunter_t* p = player -> cast_hunter();

    if ( p -> buffs_master_marksman_fire -> check() )
      return 0;

    return hunter_attack_t::cost();
  }

  virtual timespan_t execute_time() const
  {
    hunter_t* p = player -> cast_hunter();

    if ( p -> buffs_master_marksman_fire -> check() )
      return timespan_t::zero;

    return hunter_attack_t::execute_time();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    hunter_t* p = player -> cast_hunter();

    hunter_attack_t::target_debuff( t, dmg_type );

    if ( p -> talents.careful_aim -> rank() && t -> health_percentage() > p -> talents.careful_aim -> effect2().base_value() )
    {
      target_crit += p -> talents.careful_aim -> effect1().percent();
    }
  }

  virtual void schedule_execute()
  {
    hunter_t* p = player -> cast_hunter();

    hunter_attack_t::schedule_execute();

    if ( time_to_execute > timespan_t::zero )
    {
      p -> ranged_attack -> cancel();
      casted = 1;
    }
    else
    {
      casted = 0;
    }
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();

    if ( !casted )
    {
      as_mm -> execute();
    }
    else
    {
      hunter_attack_t::execute();
      if ( result == RESULT_CRIT )
      {
        if ( p -> active_pet )
          p -> active_pet -> buffs_sic_em -> trigger();
        p -> resource_gain( RESOURCE_FOCUS, p -> glyphs.aimed_shot -> effect1().resource( RESOURCE_FOCUS ), p -> gains_glyph_aimed_shot );
      }
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    hunter_attack_t::impact( t, impact_result, travel_dmg );

    if ( impact_result == RESULT_CRIT )
      trigger_piercing_shots( this, travel_dmg );
  }
};

// Arcane Shot Attack =======================================================

struct arcane_shot_t : public hunter_attack_t
{
  arcane_shot_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "arcane_shot", player, "Arcane Shot" )
  {
    hunter_t* p = player -> cast_hunter();

    parse_options( NULL, options_str );
    base_cost += p -> talents.efficiency -> effect2().resource( RESOURCE_FOCUS );

    // To trigger ppm-based abilities
    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    direct_power_mod = 0.0483; // hardcoded into tooltip

    base_multiplier *= 1.0 + p -> glyphs.arcane_shot -> mod_additive( P_GENERIC );

    consumes_tier12_4pc = true;
  }

  virtual double cost() const
  {
    hunter_t* p = player -> cast_hunter();

    if ( ! p -> dbc.ptr && p -> buffs_lock_and_load -> check() )
      return 0;

    return hunter_attack_t::cost();
  }

  virtual void execute()
  {
    hunter_attack_t::execute();

    hunter_t* p = player -> cast_hunter();

    trigger_thrill_of_the_hunt( this );

    if ( result_is_hit() )
    {
      p -> buffs_cobra_strikes -> trigger( 2 );

      // Needs testing
      p -> buffs_tier13_4pc -> trigger();

      if ( result == RESULT_CRIT && p -> active_pet )
      {
        p -> active_pet -> buffs_sic_em -> trigger();
      }
    }

    if ( p -> dbc.ptr )
    {
      p -> buffs_lock_and_load -> up();
      p -> buffs_lock_and_load -> decrement();
    }
  }
};

// Flaming Arrow Attack =====================================================

struct flaming_arrow_t : public hunter_attack_t
{
  flaming_arrow_t( player_t* player ) :
    hunter_attack_t( "flaming_arrow", player, 99058 )
  {
    background = true;
    repeating = false;
    proc = true;
    normalize_weapon_speed=true;
  }

  virtual void trigger_improved_steady_shot()
  { }
};

// Black Arrow ==============================================================

struct black_arrow_t : public hunter_attack_t
{
  black_arrow_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "black_arrow", player, "Black Arrow" )
  {
    hunter_t* p = player -> cast_hunter();

    check_talent( p -> talents.black_arrow -> rank() );

    parse_options( NULL, options_str );

    may_block  = false;
    may_crit   = false;

    cooldown = p -> get_cooldown( "traps" );
    cooldown -> duration = spell_id_t::cooldown();
    cooldown -> duration += p -> talents.resourcefulness -> effect1().time_value();

    base_multiplier *= 1.0 + p -> talents.trap_mastery -> effect1().percent();
    // Testing shows BA crits for 2.09x dmg with the crit dmg meta gem, this
    // yields the right result
    crit_bonus = 0.5;
    crit_bonus_multiplier *= 1.0 + p -> talents.toxicology -> effect1().percent();

    base_dd_min=base_dd_max=0;
    tick_power_mod=extra_coeff();
  }

  virtual void tick( dot_t* d )
  {
    hunter_attack_t::tick( d );

    hunter_t* p = player -> cast_hunter();

    if ( p -> buffs_lock_and_load -> trigger( 2 ) )
    {
      p -> procs_lock_and_load -> occur();
      p -> cooldowns_explosive_shot -> reset();
    }
  }

  virtual void execute()
  {
    hunter_attack_t::execute();

    trigger_thrill_of_the_hunt( this );
  }
};

// Explosive Trap ===========================================================

struct explosive_trap_effect_t : public hunter_attack_t
{
  explosive_trap_effect_t( hunter_t* player )
    : hunter_attack_t( "explosive_trap", player, 13812 )
  {
    hunter_t* p = player -> cast_hunter();
    aoe = -1;
    background = true;
    tick_power_mod = extra_coeff();

    base_multiplier *= 1.0 + p -> talents.trap_mastery -> effect1().percent();
    may_miss = false;
    may_crit = false;
    tick_may_crit = true;
    crit_bonus = 0.5;
  }

  virtual void tick( dot_t* d )
  {
    hunter_attack_t::tick( d );

    hunter_t* p = player -> cast_hunter();

    if ( p -> buffs_lock_and_load -> trigger( 2 ) )
    {
      p -> procs_lock_and_load -> occur();
      p -> cooldowns_explosive_shot -> reset();
    }
  }
};

struct explosive_trap_t : public hunter_attack_t
{
  attack_t* trap_effect;
  int trap_launcher;

  explosive_trap_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "explosive_trap", player, 13813 ), trap_effect( 0 ),
    trap_launcher( 0 )
  {
    hunter_t* p = player -> cast_hunter();

    option_t options[] =
    {
      // Launched traps have a focus cost
      { "trap_launcher", OPT_BOOL, &trap_launcher },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    //TODO: Split traps cooldown into fire/frost/snakes
    cooldown = p -> get_cooldown( "traps" );
    cooldown -> duration = spell_id_t::cooldown();
    cooldown -> duration += p -> talents.resourcefulness -> effect1().time_value();

    may_miss=false;

    trap_effect = new explosive_trap_effect_t( p );
    add_child( trap_effect );
  }

  virtual void execute()
  {
    hunter_attack_t::execute();

    trap_effect -> execute();
  }

  virtual double cost() const
  {
    hunter_t* p = player -> cast_hunter();

    if ( trap_launcher )
      return 20.0 + p -> glyphs.trap_launcher -> effect1().resource( RESOURCE_FOCUS );

    return hunter_attack_t::cost();
  }
};

// Chimera Shot =============================================================

struct chimera_shot_t : public hunter_attack_t
{
  chimera_shot_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "chimera_shot", player, "Chimera Shot" )
  {
    hunter_t* p = player -> cast_hunter();

    check_talent( p -> talents.chimera_shot -> rank() );

    parse_options( NULL, options_str );

    direct_power_mod = 0.732; // hardcoded into tooltip

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    base_cost += p -> talents.efficiency -> effect1().resource( RESOURCE_FOCUS );

    normalize_weapon_speed = true;

    cooldown -> duration += timespan_t::from_seconds( p -> glyphs.chimera_shot -> mod_additive( P_COOLDOWN ) );

    consumes_tier12_4pc = true;
  }


  virtual void execute()
  {
    hunter_attack_t::execute();

    if ( result_is_hit() )
    {
      hunter_targetdata_t* td = targetdata() -> cast_hunter();
      td -> dots_serpent_sting -> refresh_duration();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    hunter_attack_t::impact( t, impact_result, travel_dmg );

    if ( impact_result == RESULT_CRIT )
      trigger_piercing_shots( this, travel_dmg );
  }
};

// Cobra Shot Attack ========================================================

struct cobra_shot_t : public hunter_attack_t
{
  double focus_gain;

  cobra_shot_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "cobra_shot", player, "Cobra Shot" )
  {
    hunter_t* p = player -> cast_hunter();

    parse_options( NULL, options_str );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    direct_power_mod = 0.017; // hardcoded into tooltip

    if ( p -> sets -> set ( SET_T11_4PC_MELEE ) -> ok() )
      base_execute_time -= timespan_t::from_seconds( 0.2 );

    focus_gain = p -> dbc.spell( 77443 ) -> effect1().base_value();

    // Needs testing
    if ( p -> set_bonus.tier13_2pc_melee() )
      focus_gain *= 2.0;
  }

  virtual bool usable_moving()
  {
    hunter_t* p = player -> cast_hunter();

    return p -> active_aspect == ASPECT_FOX;
  }

  void execute()
  {
    hunter_attack_t::execute();

    if ( result_is_hit() )
    {
      hunter_t* p = player -> cast_hunter();
      hunter_targetdata_t* td = targetdata() -> cast_hunter();

      td -> dots_serpent_sting -> extend_duration( 2 );

      double focus = focus_gain;
      if ( p -> talents.termination -> rank() )
      {
        if ( target -> health_percentage() <= p -> talents.termination -> effect2().base_value() )
          focus += p -> talents.termination -> effect1().resource( RESOURCE_FOCUS );
      }
      p -> resource_gain( RESOURCE_FOCUS, focus, p -> gains_cobra_shot );
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    hunter_attack_t::impact( t, impact_result, travel_dmg );

    trigger_tier12_2pc_melee( this );
  }

  void player_buff()
  {
    hunter_t* p = player -> cast_hunter();

    hunter_attack_t::player_buff();

    if ( p -> talents.careful_aim -> rank() && target -> health_percentage() > p -> talents.careful_aim -> effect2().base_value() )
    {
      player_crit += p -> talents.careful_aim -> effect1().percent();
    }

    if ( p -> buffs_sniper_training -> up() )
      player_multiplier *= 1.0 + p -> buffs_sniper_training -> effect1().percent();
  }
};

// Explosive Shot ===========================================================

struct explosive_shot_t : public hunter_attack_t
{
  int lock_and_load;
  int non_consecutive;

  explosive_shot_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "explosive_shot", player, "Explosive Shot" ),
    lock_and_load( 0 ), non_consecutive( 0 )
  {
    hunter_t* p = player -> cast_hunter();
    check_spec ( TREE_SURVIVAL );

    option_t options[] =
    {
      { "non_consecutive", OPT_BOOL, &non_consecutive },
      { NULL, OPT_UNKNOWN, NULL}
    };

    parse_options( options, options_str );

    may_block = false;
    may_crit  = false;

    base_cost += p -> talents.efficiency -> effect1().resource( RESOURCE_FOCUS );
    base_crit += p -> glyphs.explosive_shot -> mod_additive( P_CRIT );
    // Testing shows ES crits for 2.09x dmg with the crit dmg meta gem, this
    // yields the right result
    crit_bonus = 0.5;
    crit_bonus_multiplier *= 2.0;

    tick_power_mod = 0.273; // hardcoded into tooltip
    tick_zero = true;

    consumes_tier12_4pc = true;
  }

  virtual double cost() const
  {
    hunter_t* p = player -> cast_hunter();

    if ( p -> buffs_lock_and_load -> check() )
      return 0;

    return hunter_attack_t::cost();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( non_consecutive && p -> last_foreground_action )
    {
      if ( p -> last_foreground_action -> name_str == "explosive_shot" )
        return false;
    }

    return hunter_attack_t::ready();
  }

  virtual void update_ready()
  {
    hunter_t* p = player -> cast_hunter();

    cooldown -> duration = ( p -> buffs_lock_and_load -> check() ? timespan_t::zero : spell_id_t::cooldown() );

    hunter_attack_t::update_ready();
  }

  virtual void execute()
  {
    hunter_attack_t::execute();

    hunter_t* p = player -> cast_hunter();

    p -> buffs_lock_and_load -> up();
    p -> buffs_lock_and_load -> decrement();

    trigger_thrill_of_the_hunt( this );

    if ( result == RESULT_CRIT && p -> active_pet )
      p -> active_pet -> buffs_sic_em -> trigger();
  }
};

// Kill Shot ================================================================

struct kill_shot_t : public hunter_attack_t
{

  cooldown_t* cooldowns_glyph_kill_shot;

  kill_shot_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "kill_shot", player, "Kill Shot" ),
    cooldowns_glyph_kill_shot( 0 )
  {
    hunter_t* p = player -> cast_hunter();

    parse_options( NULL, options_str );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    base_dd_min *= weapon_multiplier; // Kill Shot's weapon multiplier applies to the base damage as well
    base_dd_max *= weapon_multiplier;
    direct_power_mod = 0.45 * weapon_multiplier; // and the coefficient too

    base_crit += p -> talents.sniper_training -> effect2().percent();

    if ( p -> glyphs.kill_shot -> ok() )
    {
      cooldowns_glyph_kill_shot = p -> get_cooldown( "cooldowns_glyph_kill_shot" );
      cooldowns_glyph_kill_shot -> duration = p -> dbc.spell( 90967 ) -> duration();
    }

    normalize_weapon_speed = true;
  }

  virtual void execute()
  {
    hunter_attack_t::execute();

    if ( cooldowns_glyph_kill_shot && cooldowns_glyph_kill_shot -> remains() == timespan_t::zero )
    {
      cooldown -> reset();
      cooldowns_glyph_kill_shot -> start();
    }
  }

  virtual bool ready()
  {
    if ( target -> health_percentage() > 20 )
      return false;

    return hunter_attack_t::ready();
  }
};

// Scatter Shot Attack ======================================================

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
  }
};

// Serpent Sting Attack =====================================================

struct serpent_sting_burst_t : public hunter_attack_t
{
  serpent_sting_burst_t( player_t* player ) :
    hunter_attack_t( "serpent_sting_burst", player, SCHOOL_NATURE, TREE_MARKSMANSHIP )
  {
    proc       = true;
    background = true;
  }
};

struct serpent_sting_t : public hunter_attack_t
{
  serpent_sting_burst_t* serpent_sting_burst;

  serpent_sting_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "serpent_sting", player, "Serpent Sting" )
  {
    hunter_t* p = player -> cast_hunter();

    parse_options( NULL, options_str );

    may_block = false;
    may_crit  = false;

    tick_power_mod = 0.4 / num_ticks; // hardcoded into tooltip

    base_crit += p -> talents.improved_serpent_sting -> mod_additive( P_CRIT );
    base_crit += p -> glyphs.serpent_sting -> mod_additive( P_CRIT );
    base_crit += p -> sets -> set ( SET_T11_2PC_MELEE ) -> effect1().percent();
    // Testing shows SS crits for 2.09x dmg with the crit dmg meta gem, this
    // yields the right result
    crit_bonus = 0.5;
    crit_bonus_multiplier *= 1.0 + p -> talents.toxicology -> effect1().percent();
    serpent_sting_burst = new serpent_sting_burst_t( p );
    add_child( serpent_sting_burst );
  }

  virtual void execute()
  {
    hunter_attack_t::execute();

    hunter_t* p = player -> cast_hunter();

    if ( serpent_sting_burst && p -> talents.improved_serpent_sting -> ok() )
    {
      double t = ( p -> talents.improved_serpent_sting -> effect1().percent() ) *
                 ( ceil( base_td ) * hasted_num_ticks() + total_power() * 0.4 );

      serpent_sting_burst -> base_dd_min = t;
      serpent_sting_burst -> base_dd_max = t;
      serpent_sting_burst -> execute();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    hunter_attack_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
      t -> debuffs.poisoned -> increment();
  }

  virtual void last_tick( dot_t* d )
  {
    hunter_attack_t::last_tick( d );

    // FIXME: change to dot target once they are on the target
    target -> debuffs.poisoned -> decrement();
  }
};

struct serpent_sting_spread_t : public serpent_sting_t
{
  serpent_sting_spread_t( player_t* player, const std::string& options_str ) :
    serpent_sting_t( player, options_str )
  {
    hunter_t* p = player -> cast_hunter();
    num_ticks = 1 + p-> talents.serpent_spread -> rank();
    base_cost = 0.0;
    travel_speed = 0.0;
    background = true;
    aoe = -1;
    serpent_sting_burst -> aoe = -1;
  }

  virtual void execute()
  {
    hunter_attack_t::execute();

    hunter_t* p = player -> cast_hunter();

    if ( serpent_sting_burst && p -> talents.improved_serpent_sting -> ok() )
    {
      double t = ( p -> talents.improved_serpent_sting -> effect1().percent() ) *
                 ( ceil( base_td ) * 5 + total_power() * 0.4 );

      serpent_sting_burst -> base_dd_min = t;
      serpent_sting_burst -> base_dd_max = t;
      serpent_sting_burst -> execute();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    hunter_attack_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
      t -> debuffs.poisoned -> increment();
  }
};

// Multi Shot Attack ========================================================

struct multi_shot_t : public hunter_attack_t
{
  serpent_sting_spread_t* spread_sting;

  multi_shot_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "multi_shot", player, "Multi-Shot" ), spread_sting( 0 )
  {
    hunter_t* p = player -> cast_hunter();
    assert( p -> ranged_weapon.type != WEAPON_NONE );
    parse_options( NULL, options_str );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );
    aoe = -1;

    normalize_weapon_speed = true;
    if ( p -> talents.serpent_spread -> rank() )
      spread_sting = new serpent_sting_spread_t( player, options_str );

    consumes_tier12_4pc = true;
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
    hunter_t* p = player -> cast_hunter();

    if ( p -> buffs_bombardment -> up() && ! p -> dbc.ptr )
      p -> buffs_bombardment -> expire();
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    hunter_t* p = player -> cast_hunter();
    //target_t* q = t -> cast_target();

    hunter_attack_t::impact( t, impact_result, travel_dmg );
    int crit_occurred = 0;

    if ( result_is_hit( impact_result ) )
    {
      if ( spread_sting )
        spread_sting -> execute();
      if ( impact_result == RESULT_CRIT )
        crit_occurred++;
      /*for ( int i=0; i < q -> adds_nearby; i++ ) {
        // Calculate a result for each nearby add to determine whether to proc
        // bombardment
        int delta_level = q -> level - p -> level;
        if ( sim -> real() <= crit_chance( delta_level ) )
          crit_occurred++;
      }*/
    }

    if ( p -> talents.bombardment -> rank() && crit_occurred )
      p -> buffs_bombardment -> trigger();
  }

  virtual double cost() const
  {
    hunter_t* p = player -> cast_hunter();

    if ( p -> buffs_bombardment -> check() )
      return base_cost * ( 1 + p -> buffs_bombardment -> effect1().percent() );

    return hunter_attack_t::cost();
  }
};

// Silencing Shot Attack ====================================================

struct silencing_shot_t : public hunter_attack_t
{
  silencing_shot_t( player_t* player, const std::string& /* options_str */ ) :
    hunter_attack_t( "silencing_shot", player, "Silencing Shot" )
  {
    hunter_t* p = player -> cast_hunter();

    check_talent( p -> talents.silencing_shot -> rank() );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );
    weapon_multiplier = 0.0;

    may_miss = may_resist = may_glance = may_block = may_dodge = may_parry = may_crit = false;

    normalize_weapon_speed = true;
  }
};

// Steady Shot Attack =======================================================

struct steady_shot_t : public hunter_attack_t
{
  double focus_gain;

  steady_shot_t( player_t* player, const std::string& options_str ) :
    hunter_attack_t( "steady_shot", player, "Steady Shot" )
  {
    hunter_t* p = player -> cast_hunter();
    parse_options( NULL, options_str );

    direct_power_mod = 0.021; // hardcoded into tooltip

    if ( p -> sets -> set ( SET_T11_4PC_MELEE ) -> ok() )
      base_execute_time -= timespan_t::from_seconds( 0.2 );

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    focus_gain = p -> dbc.spell( 77443 ) -> effect1().base_value();

    // Needs testing
    if ( p -> set_bonus.tier13_2pc_melee() )
      focus_gain *= 2.0;
  }

  virtual void trigger_improved_steady_shot()
  {
    hunter_t* p = player -> cast_hunter();

    p -> buffs_pre_improved_steady_shot -> trigger( 1 );
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    hunter_t* p = player -> cast_hunter();

    hunter_attack_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) && ! p -> buffs_master_marksman_fire -> check() )
    {
      if ( p -> buffs_master_marksman -> trigger() )
      {
        if ( p -> buffs_master_marksman -> stack() == 5 )
        {
          p -> buffs_master_marksman_fire -> trigger();
          p -> buffs_master_marksman -> expire();
        }
      }
    }

    trigger_tier12_2pc_melee( this );

    if ( impact_result == RESULT_CRIT )
      trigger_piercing_shots( this, travel_dmg );
  }

  virtual bool usable_moving()
  {
    hunter_t* p = player -> cast_hunter();

    return p -> active_aspect == ASPECT_FOX;
  }

  void execute()
  {
    hunter_attack_t::execute();

    if ( result_is_hit() )
    {
      hunter_t* p = player -> cast_hunter();

      double focus = focus_gain;
      if ( target -> health_percentage() <= p -> talents.termination -> effect2().base_value() )
        focus += p -> talents.termination -> effect1().resource( RESOURCE_FOCUS );

      p -> resource_gain( RESOURCE_FOCUS, focus, p -> gains_steady_shot );
    }
  }

  void player_buff()
  {
    hunter_t* p = player -> cast_hunter();

    hunter_attack_t::player_buff();

    player_multiplier *= 1.0 + p -> glyphs.steady_shot -> mod_additive( P_GENERIC );

    if ( p -> talents.careful_aim -> rank() && target -> health_percentage() > p -> talents.careful_aim -> effect2().base_value() )
    {
      player_crit += p -> talents.careful_aim -> effect1().percent();
    }

    if ( p -> buffs_sniper_training -> up() )
      player_multiplier *= 1.0 + p -> buffs_sniper_training -> effect1().percent();
  }
};

// Wild Quiver ==============================================================

struct wild_quiver_shot_t : public ranged_t
{
  wild_quiver_shot_t( hunter_t* p ) : ranged_t( p, "wild_quiver_shot" )
  {
    repeating   = false;
    proc = true;
    normalize_weapon_speed=true;
    init();
  }

  virtual void execute()
  {
    hunter_attack_t::execute();
  }
};

struct wild_quiver_trigger_t : public action_callback_t
{
  attack_t* attack;
  rng_t* rng;

  wild_quiver_trigger_t( player_t* p, attack_t* a ) :
    action_callback_t( p -> sim, p ), attack( a )
  {
    rng = p -> get_rng( "wild_quiver" );
  }

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    hunter_t* p = listener -> cast_hunter();

    if ( ! a -> weapon )
      return;

    if ( a -> weapon -> slot != SLOT_RANGED )
      return;

    if ( a -> proc )
      return;

    if ( rng -> roll( p -> composite_mastery() * p -> passive_spells.wild_quiver -> effect1().coeff() / 100.0 ) )
    {
      attack -> execute();
      p -> procs_wild_quiver -> occur();
    }
  }
};

// ==========================================================================
// Hunter Spells
// ==========================================================================

// hunter_spell_t::gcd()

timespan_t hunter_spell_t::gcd() const
{
  if ( ! harmful && ! player -> in_combat )
    return timespan_t::zero;

  // Hunter gcd unaffected by haste
  return trigger_gcd;
}

// hunter_spell_t::cost =====================================================

double hunter_spell_t::cost() const
{
  hunter_t* p = player -> cast_hunter();

  double c = spell_t::cost();

  if ( c == 0 )
    return 0;

  if ( p -> buffs_tier12_4pc -> check() && consumes_tier12_4pc )
    return 0;

  if ( p -> buffs_beast_within -> check() )
    c *= ( 1.0 + p -> buffs_beast_within -> effect1().percent() );

  return c;
}

// hunter_spell_t::consume_resource =========================================

void hunter_spell_t::consume_resource()
{
  spell_t::consume_resource();

  hunter_t* p = player -> cast_hunter();

  if ( p -> buffs_tier12_4pc -> up() )
  {
    // Treat the savings like a gain
    double amount = spell_t::cost();
    if ( amount > 0 )
    {
      p -> gains_tier12_4pc -> add( amount );
      p -> gains_tier12_4pc -> type = RESOURCE_FOCUS;
      p -> buffs_tier12_4pc -> expire();
    }
  }
}

// Aspect of the Hawk =======================================================

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
      double value = effect_average( 1 );
      p -> buffs_aspect_of_the_hawk -> trigger( 1, value * ( 1.0 + p -> talents.one_with_nature -> effect1().percent() ) );
    }
    else if ( p -> active_aspect == ASPECT_HAWK )
    {
      p -> active_aspect = ASPECT_NONE;
      p -> buffs_aspect_of_the_hawk -> expire();
    }
    else if ( p -> active_aspect == ASPECT_FOX )
    {
      p -> active_aspect = ASPECT_HAWK;
      double value = effect_average( 1 );
      p -> buffs_aspect_of_the_hawk -> trigger( 1, value * ( 1.0 + p -> talents.one_with_nature -> effect1().percent() ) );
    }

  }
  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( p -> active_aspect == ASPECT_HAWK )
      return false;

    return hunter_spell_t::ready();
  }
};

// Aspect of the Fox ========================================================

struct aspect_of_the_fox_t : public hunter_spell_t
{
  aspect_of_the_fox_t( player_t* player, const std::string& options_str ) :
    hunter_spell_t( "aspect_of_the_fox", player, "Aspect of the Fox" )
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
      p -> active_aspect = ASPECT_FOX;
    }
    else if ( p -> active_aspect == ASPECT_FOX )
    {
      p -> active_aspect = ASPECT_NONE;
    }
    else if ( p -> active_aspect == ASPECT_HAWK )
    {
      p -> buffs_aspect_of_the_hawk -> expire();
      p -> active_aspect = ASPECT_FOX;
    }

  }
  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( p -> active_aspect == ASPECT_FOX )
      return false;

    return hunter_spell_t::ready();
  }
};

// Bestial Wrath ============================================================

struct bestial_wrath_t : public hunter_spell_t
{
  bestial_wrath_t( player_t* player, const std::string& options_str ) :
    hunter_spell_t( "bestial_wrath", player, "Bestial Wrath" )
  {
    hunter_t* p = player -> cast_hunter();
    parse_options( NULL, options_str );
    check_talent( p -> talents.bestial_wrath -> rank() );

    cooldown -> duration += p -> glyphs.bestial_wrath -> effect1().time_value();
    cooldown -> duration *=  ( 1.0 + p -> talents.longevity -> effect1().percent() );
    harmful = false;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();

    hunter_pet_t* pet = p -> active_pet;

    p   -> buffs_beast_within  -> trigger();
    pet -> buffs_bestial_wrath -> trigger();

    hunter_spell_t::execute();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( ! p -> active_pet )
      return false;

    return hunter_spell_t::ready();
  }
};

// Fervor ===================================================================

struct fervor_t : public hunter_spell_t
{
  fervor_t( player_t* player, const std::string& options_str ) :
    hunter_spell_t( "fervor", player, "Fervor" )
  {
    hunter_t* p = player -> cast_hunter();

    check_talent( p -> talents.fervor -> ok() );

    parse_options( NULL, options_str );

    harmful = false;

    trigger_gcd = timespan_t::zero;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();

    if ( p -> active_pet )
      p -> active_pet -> resource_gain( RESOURCE_FOCUS, effect1().base_value(), p -> active_pet -> gains_fervor );

    p -> resource_gain( RESOURCE_FOCUS, effect1().base_value(), p -> gains_fervor );

    hunter_spell_t::execute();
  }
};

// Focus Fire ===============================================================

struct focus_fire_t : public hunter_spell_t
{
  int five_stacks;
  focus_fire_t( player_t* player, const std::string& options_str ) :
    hunter_spell_t( "focus_fire", player, 82692 )
  {
    hunter_t* p = player -> cast_hunter();

    check_talent( p -> talents.focus_fire -> ok() );

    option_t options[] =
    {
      { "five_stacks", OPT_BOOL, &five_stacks },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();

    double value = p -> active_pet -> buffs_frenzy -> stack() * p -> talents.focus_fire -> effect3().percent();
    p -> buffs_focus_fire -> trigger( 1, value );

    double gain = p -> talents.focus_fire -> effect2().resource( RESOURCE_FOCUS );
    p -> active_pet -> resource_gain( RESOURCE_FOCUS, gain, p -> active_pet -> gains_focus_fire );

    hunter_spell_t::execute();

    p -> active_pet -> buffs_frenzy -> expire();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( ! p -> active_pet )
      return false;

    if ( ! p -> active_pet -> buffs_frenzy -> check() )
      return false;

    if ( five_stacks && p -> active_pet -> buffs_frenzy -> check() < 5 )
      return false;

    return hunter_spell_t::ready();
  }
};

// Hunter's Mark ============================================================

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

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    hunter_spell_t::impact( t, impact_result, travel_dmg );

    t -> debuffs.hunters_mark -> trigger( 1, ap_bonus );
    t -> debuffs.hunters_mark -> source = player;
  }

  virtual bool ready()
  {
    if ( ! hunter_spell_t::ready() )
      return false;

    return ap_bonus > target -> debuffs.hunters_mark -> current_value;
  }
};

// Kill Command =============================================================

struct kill_command_t : public hunter_spell_t
{
  kill_command_t( player_t* player, const std::string& options_str ) :
    hunter_spell_t( "kill_command", player, "Kill Command" )
  {
    hunter_t* p = player -> cast_hunter();

    parse_options( NULL, options_str );

    base_spell_power_multiplier    = 0.0;
    base_attack_power_multiplier   = 1.0;
    base_cost += p -> glyphs.kill_command -> mod_additive( P_RESOURCE_COST );

    harmful = false;

    for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
    {
      stats -> children.push_back( pet -> get_stats( "kill_command" ) );
    }

    consumes_tier12_4pc = true;
  }

  virtual double cost() const
  {
    hunter_t* p = player -> cast_hunter();

    double cost = hunter_spell_t::cost();

    if ( p -> buffs_killing_streak -> check() )
      cost -= p -> talents.killing_streak -> effect2().resource( RESOURCE_FOCUS );

    if ( cost < 0.0 )
      return 0;

    return cost;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();

    hunter_spell_t::execute();

    if ( p -> active_pet )
    {
      p -> active_pet -> kill_command -> base_dd_adder = 0.516 * total_power(); // hardcoded into tooltip
      p -> active_pet -> kill_command -> execute();
    }
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( ! p -> active_pet )
      return false;

    return hunter_spell_t::ready();
  }
};

// Rapid Fire ===============================================================

struct rapid_fire_t : public hunter_spell_t
{
  rapid_fire_t( player_t* player, const std::string& options_str ) :
    hunter_spell_t( "rapid_fire", player, "Rapid Fire" )
  {
    hunter_t* p = player -> cast_hunter();
    parse_options( NULL, options_str );

    cooldown -> duration += p -> talents.posthaste -> effect1().time_value();

    harmful = false;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();

    double value = effect1().percent() + p -> glyphs.rapid_fire -> effect1().percent();
    p -> buffs_rapid_fire -> trigger( 1, value );

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
    hunter_spell_t::execute();

    for ( unsigned int i=0; i < cooldown_list.size(); i++ )
    {
      cooldown_list[ i ] -> reset();
    }
  }

  virtual bool ready()
  {

    hunter_t* p = player -> cast_hunter();

    if ( wait_for_rf && ! p -> buffs_rapid_fire -> check() )
      return false;

    return hunter_spell_t::ready();
  }
};

// Summon Pet ===============================================================

struct summon_pet_t : public hunter_spell_t
{
  std::string pet_name;
  pet_t* pet;

  summon_pet_t( player_t* player, const std::string& options_str ) :
    hunter_spell_t( "summon_pet", player, SCHOOL_PHYSICAL, TREE_BEAST_MASTERY ),
    pet( 0 )
  {
    hunter_t* p = player -> cast_hunter();


    harmful = false;

    pet_name = ( options_str.size() > 0 ) ? options_str : p -> summon_pet_str;
    pet = p -> find_pet( pet_name );
    if ( ! pet )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", p -> name(), pet_name.c_str() );
      sim -> cancel();
    }
  }

  virtual void execute()
  {
    hunter_spell_t::execute();

    pet -> summon();

    hunter_t* p = player -> cast_hunter();

    p -> ranged_attack -> cancel();
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( p -> active_pet == pet )
      return false;

    return hunter_spell_t::ready();
  }
};

// Trueshot Aura ============================================================

struct trueshot_aura_t : public hunter_spell_t
{
  trueshot_aura_t( player_t* player, const std::string& /* options_str */ ) :
    hunter_spell_t( "trueshot_aura", player, "Trueshot Aura" )
  {
    hunter_t* p = player -> cast_hunter();
    check_talent( p -> talents.trueshot_aura -> rank() );
    trigger_gcd = timespan_t::zero;
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

    if ( p -> buffs_trueshot_aura -> check() )
      return false;

    return hunter_spell_t::ready();
  }
};

// Event Shedule Sniper Trainig

struct hunter_sniper_training_event_t : public event_t
{
  hunter_sniper_training_event_t ( player_t* player ) :
    event_t( player -> sim, player )
  {
    name = "Sniper_Training_Check";
    sim -> add_event( this, timespan_t::from_seconds( 5.0 ) );
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();

    if ( ! p -> in_combat )
      p -> buffs_sniper_training -> trigger();

    if ( ! p -> buffs.raid_movement -> up() )
    {
      timespan_t finished_moving = p -> buffs.raid_movement -> last_start + p -> buffs.raid_movement -> buff_duration;

      if ( ( sim -> current_time - finished_moving ) > timespan_t::from_seconds( p -> talents.sniper_training -> effect1().base_value() ) )
      {
        p -> buffs_sniper_training -> trigger();
      }
    }

    new ( sim ) hunter_sniper_training_event_t( p );
  }
};

} // ANONYMOUS NAMESPACE ====================================================


// hunter_pet_t::create_action ==============================================

action_t* hunter_pet_t::create_action( const std::string& name,
                                       const std::string& options_str )
{
  if ( name == "auto_attack"           ) return new      pet_auto_attack_t( this, options_str );
  if ( name == "call_of_the_wild"      ) return new     call_of_the_wild_t( this, options_str );
  if ( name == "claw"                  ) return new                 claw_t( this, options_str );
  if ( name == "froststorm_breath"     ) return new    froststorm_breath_t( this, options_str );
  if ( name == "furious_howl"          ) return new         furious_howl_t( this, options_str );
  if ( name == "roar_of_courage"       ) return new      roar_of_courage_t( this, options_str );
  if ( name == "qiraji_fortitude"      ) return new     qiraji_fortitude_t( this, options_str );
  if ( name == "lightning_breath"      ) return new     lightning_breath_t( this, options_str );
  if ( name == "monstrous_bite"        ) return new       monstrous_bite_t( this, options_str );
  if ( name == "rabid"                 ) return new                rabid_t( this, options_str );
  if ( name == "roar_of_recovery"      ) return new     roar_of_recovery_t( this, options_str );
  if ( name == "wolverine_bite"        ) return new       wolverine_bite_t( this, options_str );
  if ( name == "corrosive_spit"        ) return new       corrosive_spit_t( this, options_str );
  if ( name == "demoralizing_screech"  ) return new demoralizing_screech_t( this, options_str );
  if ( name == "ravage"                ) return new               ravage_t( this, options_str );
  if ( name == "tailspin"              ) return new             tailspin_t( this, options_str );
  if ( name == "tear_armor"            ) return new           tear_armor_t( this, options_str );
  if ( name == "tendon_rip"            ) return new           tendon_rip_t( this, options_str );
  if ( name == "corrosive_spit"        ) return new       corrosive_spit_t( this, options_str );

  return pet_t::create_action( name, options_str );
}

// hunter_t::create_action ==================================================

action_t* hunter_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "auto_shot"             ) return new              auto_shot_t( this, options_str );
  if ( name == "aimed_shot"            ) return new             aimed_shot_t( this, options_str );
  if ( name == "arcane_shot"           ) return new            arcane_shot_t( this, options_str );
  if ( name == "aspect_of_the_hawk"    ) return new     aspect_of_the_hawk_t( this, options_str );
  if ( name == "aspect_of_the_fox"     ) return new     aspect_of_the_fox_t( this, options_str );
  if ( name == "bestial_wrath"         ) return new          bestial_wrath_t( this, options_str );
  if ( name == "black_arrow"           ) return new            black_arrow_t( this, options_str );
  if ( name == "chimera_shot"          ) return new           chimera_shot_t( this, options_str );
  if ( name == "explosive_shot"        ) return new         explosive_shot_t( this, options_str );
  if ( name == "explosive_trap"        ) return new         explosive_trap_t( this, options_str );
  if ( name == "fervor"                ) return new                 fervor_t( this, options_str );
  if ( name == "focus_fire"            ) return new             focus_fire_t( this, options_str );
  if ( name == "hunters_mark"          ) return new           hunters_mark_t( this, options_str );
  if ( name == "kill_command"          ) return new           kill_command_t( this, options_str );
  if ( name == "kill_shot"             ) return new              kill_shot_t( this, options_str );
  if ( name == "multi_shot"            ) return new             multi_shot_t( this, options_str );
  if ( name == "rapid_fire"            ) return new             rapid_fire_t( this, options_str );
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

// hunter_t::create_pet =====================================================

pet_t* hunter_t::create_pet( const std::string& pet_name,
                             const std::string& pet_type )
{
  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  pet_type_t type = util_t::parse_pet_type( pet_type );

  if ( type > PET_NONE && type < PET_HUNTER )
  {
    return new hunter_pet_t( sim, this, pet_name, type );
  }
  else
  {
    sim -> errorf( "Player %s with pet %s has unknown type %s\n", name(), pet_name.c_str(), pet_type.c_str() );
    sim -> cancel();
  }

  return 0;
}

// hunter_t::create_pets ====================================================

void hunter_t::create_pets()
{
  if ( ! pet_list )
  {
    create_pet( "cat",          "cat"          );
    create_pet( "devilsaur",    "devilsaur"    );
    create_pet( "raptor",       "raptor"       );
    create_pet( "wind_serpent", "wind_serpent" );
    create_pet( "wolf",         "wolf"         );
  }
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

// hunter_t::init_spells ====================================================

void hunter_t::init_spells()
{
  player_t::init_spells();

  passive_spells.animal_handler    = new passive_spell_t( this, "animal_handler", "Animal Handler" );
  passive_spells.artisan_quiver    = new passive_spell_t( this, "artisan_quiver", "Artisan Quiver" );
  passive_spells.into_the_wildness = new passive_spell_t( this, "into_the_wildness", "Into the Wilderness" );

  passive_spells.master_of_beasts     = new mastery_t( this, "master_of_beasts", "Master of Beasts", TREE_BEAST_MASTERY );
  passive_spells.wild_quiver          = new mastery_t( this, "wild_quiver", "Wild Quiver", TREE_MARKSMANSHIP );
  passive_spells.essence_of_the_viper = new mastery_t( this, "essence_of_the_viper", "Essence of the Viper", TREE_SURVIVAL );

  glyphs.aimed_shot     = find_glyph( "Glyph of Aimed Shot"     );
  glyphs.arcane_shot    = find_glyph( "Glyph of Arcane Shot"    );
  glyphs.bestial_wrath  = find_glyph( "Glyph of Bestial Wrath"  );
  glyphs.chimera_shot   = find_glyph( "Glyph of Chimera Shot"   );
  glyphs.explosive_shot = find_glyph( "Glyph of Explosive Shot" );
  glyphs.kill_shot      = find_glyph( "Glyph of Kill Shot"      );
  glyphs.rapid_fire     = find_glyph( "Glyph of Rapid Fire"     );
  glyphs.serpent_sting  = find_glyph( "Glyph of Serpent Sting"  );
  glyphs.steady_shot    = find_glyph( "Glyph of Steady Shot"    );
  glyphs.trap_launcher  = find_glyph( "Glyph of Trap Launcher"  );
  glyphs.kill_command   = find_glyph( "Glyph of Kill Command"   );

  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P     M2P     M4P    T2P    T4P     H2P    H4P
    {     0,     0,  89923,  96411,     0,     0,     0,     0 }, // Tier11
    {     0,     0,  99057,  99059,     0,     0,     0,     0 }, // Tier12
    {     0,     0, 105732, 105921,     0,     0,     0,     0 }, // Tier13
    {     0,     0,      0,      0,     0,     0,     0,     0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// hunter_t::init_spells ====================================================

void hunter_pet_t::init_spells()
{
  pet_t::init_spells();

  kill_command = new pet_kill_command_t( this );
}

// hunter_t::init_base ======================================================

void hunter_t::init_base()
{
  player_t::init_base();

  attribute_multiplier_initial[ ATTR_AGILITY ]   *= 1.0 + talents.hunting_party -> effect1().percent();
  attribute_multiplier_initial[ ATTR_AGILITY ]   *= 1.0 + passive_spells.into_the_wildness -> effect1().percent();

  attribute_multiplier_initial[ ATTR_STAMINA ]   *= 1.0 + talents.hunter_vs_wild -> effect1().percent();

  base_attack_power = level * 2;

  initial_attack_power_per_strength = 0.0; // Prevents scaling from strength. Will need to separate melee and ranged AP if this is needed in the future.
  initial_attack_power_per_agility  = 2.0;

  // FIXME!
  base_focus_regen_per_second = 4;

  resource_base[ RESOURCE_FOCUS ] = 100 + talents.kindred_spirits -> effect1().resource( RESOURCE_FOCUS );

  diminished_kfactor    = 0.009880;
  diminished_dodge_capi = 0.006870;
  diminished_parry_capi = 0.006870;

  flaming_arrow = new flaming_arrow_t( this );
}

// hunter_t::init_buffs =====================================================

void hunter_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )

  buffs_aspect_of_the_hawk          = new buff_t( this, 13165, "aspect_of_the_hawk" );
  buffs_beast_within                = new buff_t( this, 34471, "beast_within", talents.the_beast_within -> rank() );
  buffs_bombardment                 = new buff_t( this, talents.bombardment -> rank() == 2 ? 35110 : talents.bombardment -> rank() == 1 ? 35104 : 0, "bombardment" );
  buffs_call_of_the_wild            = new buff_t( this, 53434, "call_of_the_wild" );
  buffs_cobra_strikes               = new buff_t( this, 53257, "cobra_strikes", talents.cobra_strikes -> proc_chance() );
  buffs_culling_the_herd            = new buff_t( this, 70893, "culling_the_herd" );
  buffs_focus_fire                  = new buff_t( this, 82692, "focus_fire" );
  buffs_improved_steady_shot        = new buff_t( this, 53220, "improved_steady_shot", talents.improved_steady_shot -> ok() );
  buffs_killing_streak              = new buff_t( this, "killing_streak", 1, timespan_t::from_seconds( 8 ), timespan_t::zero, talents.killing_streak -> ok() );
  buffs_killing_streak_crits        = new buff_t( this, "killing_streak_crits", 2, timespan_t::zero, timespan_t::zero, 1.0, true );
  buffs_lock_and_load               = new buff_t( this, 56453, "lock_and_load", talents.tnt -> effect1().percent() );
  if ( bugs ) buffs_lock_and_load -> cooldown -> duration = timespan_t::from_seconds( 10.0 ); // http://elitistjerks.com/f74/t65904-hunter_dps_analyzer/p31/#post2050744
  buffs_master_marksman             = new buff_t( this, 82925, "master_marksman", talents.master_marksman -> proc_chance() );
  buffs_master_marksman_fire        = new buff_t( this, 82926, "master_marksman_fire", 1 );
  buffs_sniper_training             = new buff_t( this, talents.sniper_training -> rank() == 3 ? 64420 : talents.sniper_training -> rank() == 2 ? 64419 : talents.sniper_training -> rank() == 1 ? 64418 : 0, "sniper_training", talents.sniper_training -> rank() );

  buffs_rapid_fire                  = new buff_t( this, 3045, "rapid_fire" );
  buffs_rapid_fire -> cooldown -> duration = timespan_t::zero;
  buffs_pre_improved_steady_shot    = new buff_t( this, "pre_improved_steady_shot",    2, timespan_t::zero, timespan_t::zero, 1, true );

  buffs_tier12_4pc                  = new buff_t( this, "tier12_4pc", 1, dbc.spell( 99060 ) -> duration(), timespan_t::zero, dbc.spell( 99059 ) -> proc_chance() * set_bonus.tier12_4pc_melee() );
  buffs_tier13_4pc                  = new buff_t( this, 105919, "tier13_4pc", sets -> set( SET_T13_4PC_MELEE ) -> proc_chance(), timespan_t::from_seconds( tier13_4pc_cooldown ) );

  // Own TSA for Glyph of TSA
  buffs_trueshot_aura               = new buff_t( this, 19506, "trueshot_aura" );
}

// hunter_t::init_values ====================================================

void hunter_t::init_values()
{
  player_t::init_values();

  if ( set_bonus.pvp_2pc_melee() )
    attribute_initial[ ATTR_AGILITY ]   += 70;

  if ( set_bonus.pvp_4pc_melee() )
    attribute_initial[ ATTR_AGILITY ]   += 90;
}

// hunter_t::init_gains =====================================================

void hunter_t::init_gains()
{
  player_t::init_gains();

  gains_glyph_of_arcane_shot = get_gain( "glyph_of_arcane_shot" );
  gains_invigoration         = get_gain( "invigoration"         );
  gains_fervor               = get_gain( "fervor"               );
  gains_focus_fire           = get_gain( "focus_fire"           );
  gains_rapid_recuperation   = get_gain( "rapid_recuperation"   );
  gains_roar_of_recovery     = get_gain( "roar_of_recovery"     );
  gains_thrill_of_the_hunt   = get_gain( "thrill_of_the_hunt"   );
  gains_steady_shot          = get_gain( "steady_shot"          );
  gains_glyph_aimed_shot     = get_gain( "glyph_aimed_shot"     );
  gains_cobra_shot           = get_gain( "cobra_shot"           );
  gains_tier11_4pc           = get_gain( "tier11_4pc"           );
  gains_tier12_4pc           = get_gain( "tier12_4pc"           );
}

// hunter_t::init_position ==================================================

void hunter_t::init_position()
{
  player_t::init_position();

  if ( position == POSITION_FRONT )
  {
    position = POSITION_RANGED_FRONT;
    position_str = util_t::position_type_string( position );
  }
  else if ( position == POSITION_BACK )
  {
    position = POSITION_RANGED_BACK;
    position_str = util_t::position_type_string( position );
  }
}

// hunter_t::init_procs =====================================================

void hunter_t::init_procs()
{
  player_t::init_procs();

  procs_thrill_of_the_hunt = get_proc( "thrill_of_the_hunt" );
  procs_wild_quiver        = get_proc( "wild_quiver"        );
  procs_lock_and_load      = get_proc( "lock_and_load"      );
  procs_flaming_arrow      = get_proc( "flaming_arrow"      );
  procs_deferred_piercing_shots = get_proc( "deferred_piercing_shots" );
  procs_munched_piercing_shots  = get_proc( "munched_piercing_shotse" );
  procs_rolled_piercing_shots   = get_proc( "rolled_piercing_shots"   );
}

// hunter_t::init_rng =======================================================

void hunter_t::init_rng()
{
  player_t::init_rng();

  rng_invigoration         = get_rng( "invigoration"       );
  rng_owls_focus           = get_rng( "owls_focus"         );
  rng_thrill_of_the_hunt   = get_rng( "thrill_of_the_hunt" );
  rng_tier11_4pc           = get_rng( "tier11_4pc" );
  rng_tier12_2pc           = get_rng( "tier12_2pc" );

  // Overlapping procs require the use of a "distributed" RNG-stream when normalized_roll=1
  // also useful for frequent checks with low probability of proc and timed effect

  rng_frenzy               = get_rng( "frenzy",                      RNG_DISTRIBUTED );
  rng_rabid_power          = get_rng( "rabid_power",                 RNG_DISTRIBUTED );
}

// hunter_t::init_scaling ===================================================

void hunter_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_STRENGTH ]         = 0;
  scales_with[ STAT_EXPERTISE_RATING ] = 0;
}

// hunter_t::init_actions ===================================================

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

    action_list_str += "/hunters_mark";
    action_list_str += "/summon_pet";
    if ( talents.trueshot_aura -> rank() )
      action_list_str += "/trueshot_aura";
    action_list_str += "/snapshot_stats";
    action_list_str += "/tolvir_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
    if ( glyphs.rapid_fire -> ok() )
      action_list_str += "|buff.rapid_fire.react";

    if ( primary_tree() == TREE_SURVIVAL )
      action_list_str += init_use_racial_actions();
    action_list_str += init_use_item_actions();
    action_list_str += init_use_profession_actions();
    action_list_str += "/aspect_of_the_hawk,moving=0";
    action_list_str += "/aspect_of_the_fox,moving=1";
    action_list_str += "/auto_shot";
    action_list_str += "/explosive_trap,if=target.adds>0";

    switch ( primary_tree() )
    {
    // BEAST MASTERY
    case TREE_BEAST_MASTERY:
      if ( talents.focus_fire -> ok() )
      {
        action_list_str += "/focus_fire,five_stacks=1";
      }
      action_list_str += "/serpent_sting,if=!ticking";

      action_list_str += init_use_racial_actions();
      if ( talents.bestial_wrath -> rank() )
        action_list_str += "/bestial_wrath,if=focus>60";
      action_list_str += "/multi_shot,if=target.adds>5";
      action_list_str += "/cobra_shot,if=target.adds>5";
      action_list_str += "/kill_shot";
      action_list_str += "/rapid_fire,if=!buff.bloodlust.up&!buff.beast_within.up";
      action_list_str += "/kill_command";

      if ( talents.fervor -> ok() )
        action_list_str += "/fervor,if=focus<=37";

      action_list_str += "/arcane_shot,if=focus>=59|buff.beast_within.up";
      if ( level >= 81 )
        action_list_str += "/cobra_shot";
      else
        action_list_str += "/steady_shot";
      break;

    // MAKRMANSHIP
    case TREE_MARKSMANSHIP:
      action_list_str += init_use_racial_actions();
      action_list_str += "/multi_shot,if=target.adds>5";
      action_list_str += "/steady_shot,if=target.adds>5";
      action_list_str += "/serpent_sting,if=!ticking&target.health_pct<=90";
      if ( talents.chimera_shot -> rank() )
        action_list_str += "/chimera_shot,if=target.health_pct<=90";
      action_list_str += "/rapid_fire,if=!buff.bloodlust.up|target.time_to_die<=30";
      action_list_str += "/readiness,wait_for_rapid_fire=1";
      action_list_str += "/steady_shot,if=buff.pre_improved_steady_shot.up&buff.improved_steady_shot.remains<3";
      action_list_str += "/kill_shot";
      action_list_str += "/aimed_shot,if=buff.master_marksman_fire.react";
      if ( set_bonus.tier13_4pc_melee() )
      {
        action_list_str += "/arcane_shot,if=(focus>=66|cooldown.chimera_shot.remains>=4)&(target.health_pct<90&!buff.rapid_fire.up&!buff.bloodlust.react&!buff.berserking.up&!buff.tier13_4pc.react&cooldown.buff_tier13_4pc.remains<=0)";
        action_list_str += "/aimed_shot,if=(cooldown.chimera_shot.remains>5|focus>=80)&(buff.bloodlust.react|buff.tier13_4pc.react|cooldown.buff_tier13_4pc.remains>0)|buff.rapid_fire.up|target.health_pct>90";
      }
      else
      {
        if ( ! glyphs.arcane_shot -> ok() )
          action_list_str += "/aimed_shot,if=cooldown.chimera_shot.remains>5|focus>=80|buff.rapid_fire.up|buff.bloodlust.react|target.health_pct>90";
        else
        {
          action_list_str += "/aimed_shot,if=target.health_pct>90|buff.rapid_fire.up|buff.bloodlust.react";
          if ( race == RACE_TROLL )
            action_list_str += "|buff.berserking.up";
          action_list_str += "/arcane_shot,if=(focus>=66|cooldown.chimera_shot.remains>=5)&(target.health_pct<90&!buff.rapid_fire.up&!buff.bloodlust.react";
          if ( race == RACE_TROLL )
            action_list_str += "&!buff.berserking.up)";
          else
            action_list_str += ")";
        }
      }
      action_list_str += "/steady_shot";
      break;

    // SURVIVAL
    case TREE_SURVIVAL:
      action_list_str += "/multi_shot,if=target.adds>2";
      action_list_str += "/cobra_shot,if=target.adds>2";
      action_list_str += "/serpent_sting,if=!ticking";
      action_list_str += "/explosive_shot,if=(remains<2.0)";
      action_list_str += "/explosive_trap";
      action_list_str += "/kill_shot";

//    Not used on fights like Patchwerk where the boss stands still.
//      if ( talents.black_arrow -> rank() ) action_list_str += "/black_arrow,if=!ticking";

      action_list_str += "/rapid_fire";
      action_list_str += "/arcane_shot,if=focus>=50&!buff.lock_and_load.react";
      if ( level >=81 )
        action_list_str += "/cobra_shot";
      else
        action_list_str += "/steady_shot";
      if ( summon_pet_str.empty() )
        summon_pet_str = "cat";
      break;

    // DEFAULT
    default: break;
    }

    if ( summon_pet_str.empty() )
      summon_pet_str = "cat";

    action_list_default = 1;
  }

  player_t::init_actions();
}

// hunter_t::register_callbacks ==============================================

void hunter_t::register_callbacks()
{
  player_t::register_callbacks();

  if ( passive_spells.wild_quiver -> ok() )
  {
    register_attack_callback( RESULT_ALL_MASK, new wild_quiver_trigger_t( this, new wild_quiver_shot_t( this ) ) );
  }
}

// hunter_t::combat_begin ===================================================

void hunter_t::combat_begin()
{
  player_t::combat_begin();

  if ( talents.ferocious_inspiration -> rank() )
    sim -> auras.ferocious_inspiration -> trigger();

  if ( talents.hunting_party -> rank() )
    sim -> auras.hunting_party -> trigger( 1, talents.hunting_party -> effect2().percent(), 1 );

  if ( talents.sniper_training -> rank() )
  {
    buffs_sniper_training -> trigger();
    new ( sim ) hunter_sniper_training_event_t( this );
  }
}

// hunter_t::reset ==========================================================

void hunter_t::reset()
{
  player_t::reset();

  // Active
  active_pet            = 0;
  active_aspect         = ASPECT_NONE;
}

// hunter_t::composite_attack_power =========================================

double hunter_t::composite_attack_power() const
{
  double ap = player_t::composite_attack_power();

  ap += buffs_aspect_of_the_hawk -> value();

  ap *= 1.0 + passive_spells.animal_handler -> effect1().percent();

  return ap;
}

// hunter_t::composite_attack_power_multiplier ==============================

double hunter_t::composite_attack_power_multiplier() const
{
  double mult = player_t::composite_attack_power_multiplier();

  if ( buffs_call_of_the_wild -> up() )
  {
    mult *= 1.0 + buffs_call_of_the_wild -> effect1().percent();
  }

  return mult;
}

// hunter_t::composite_attack_haste =========================================

double hunter_t::composite_attack_haste() const
{
  double h = player_t::composite_attack_haste();

  h *= 1.0 / ( 1.0 + talents.pathing -> effect1().percent() );
  h *= 1.0 / ( 1.0 + buffs_focus_fire -> value() );
  h *= 1.0 / ( 1.0 + buffs_rapid_fire -> value() );
  h *= 1.0 / ( 1.0 + buffs_tier13_4pc -> up() * buffs_tier13_4pc -> effect1().percent() );
  return h;
}

// hunter_t::composite_player_multiplier ====================================

double hunter_t::composite_player_multiplier( const school_type school, action_t* a ) const
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( school == SCHOOL_NATURE || school == SCHOOL_ARCANE || school== SCHOOL_SHADOW || school == SCHOOL_FIRE )
  {
    m *= 1.0 + passive_spells.essence_of_the_viper -> effect1().coeff() / 100.0 * composite_mastery();
  }
  return m;
}

// hunter_t::matching_gear_multiplier =======================================

double hunter_t::matching_gear_multiplier( const attribute_type attr ) const
{
  if ( attr == ATTR_AGILITY )
    return 0.05;

  return 0.0;
}

// hunter_t::regen  =========================================================

void hunter_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );
  periodicity *= 1.0 + haste_rating / rating.attack_haste;
  if ( buffs_rapid_fire -> check() && talents.rapid_recuperation -> rank() )
  {
    // 2/4 focus per sec
    double rr_regen = periodicity.total_seconds() * talents.rapid_recuperation -> effect1().base_value();

    resource_gain( RESOURCE_FOCUS, rr_regen, gains_rapid_recuperation );
  }
}

// hunter_t::create_options =================================================

void hunter_t::create_options()
{
  player_t::create_options();

  option_t hunter_options[] =
  {
    { "summon_pet", OPT_STRING, &( summon_pet_str  ) },
    { "merge_piercing_shots", OPT_FLT, &( merge_piercing_shots ) },
    { "tier13_4pc_cooldown", OPT_FLT, &( tier13_4pc_cooldown ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, hunter_options );
}

// hunter_t::create_profile =================================================

bool hunter_t::create_profile( std::string& profile_str, int save_type, bool save_html )
{
  player_t::create_profile( profile_str, save_type, save_html );

  for ( pet_t* pet = pet_list; pet; pet = pet -> next_pet )
  {
    hunter_pet_t* p = ( hunter_pet_t* ) pet;

    if ( pet -> talents_str.empty() )
    {
      for ( int j=0; j < MAX_TALENT_TREES; j++ )
        for ( int k=0; k < ( int ) pet -> talent_trees[ j ].size(); k++ )
          pet -> talents_str += ( char ) ( pet -> talent_trees[ j ][ k ] -> rank() + ( int ) '0' );
    }

    profile_str += "pet=";
    profile_str += util_t::pet_type_string( p -> pet_type );
    profile_str += ",";
    profile_str += p -> name_str + "\n";
    profile_str += "talents=" + p -> talents_str + "\n";
    profile_str += "active=owner\n";
  }

  profile_str += "summon_pet=" + summon_pet_str + "\n";

  return true;
}

// hunter_t::copy_from ======================================================

void hunter_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  hunter_t* p = source -> cast_hunter();

  summon_pet_str = p -> summon_pet_str;
  merge_piercing_shots           = source -> cast_hunter() -> merge_piercing_shots;

  for ( pet_t* pet = source -> pet_list; pet; pet = pet -> next_pet )
  {
    hunter_pet_t* hp = new hunter_pet_t( sim, this, pet -> name_str, pet -> pet_type );

    hp -> parse_talents_armory( pet -> talents_str );

    for ( int i = 0; i < MAX_TALENT_TREES; i++ )
    {
      for ( int j = 0; j < ( int ) hp -> talent_trees[ i ].size(); j++ )
      {
        hp -> talents_str += ( char ) ( hp -> talent_trees[ i ][ j ] -> rank() + ( int ) '0' );
      }
    }
  }
}

// hunter_t::armory_extensions ==============================================

void hunter_t::armory_extensions( const std::string& region,
                                  const std::string& server,
                                  const std::string& character,
                                  cache::behavior_t  caching )
{
  // Pet support
  static pet_type_t pet_types[] =
  {
    /* 0*/ PET_NONE,         PET_WOLF,         PET_CAT,          PET_SPIDER,   PET_BEAR,
    /* 5*/ PET_BOAR,         PET_CROCOLISK,    PET_CARRION_BIRD, PET_CRAB,     PET_GORILLA,
    /*10*/ PET_NONE,         PET_RAPTOR,       PET_TALLSTRIDER,  PET_NONE,     PET_NONE,
    /*15*/ PET_NONE,         PET_NONE,         PET_NONE,         PET_NONE,     PET_NONE,
    /*20*/ PET_SCORPID,      PET_TURTLE,       PET_NONE,         PET_NONE,     PET_BAT,
    /*25*/ PET_HYENA,        PET_BIRD_OF_PREY, PET_WIND_SERPENT, PET_NONE,     PET_NONE,
    /*30*/ PET_DRAGONHAWK,   PET_RAVAGER,      PET_WARP_STALKER, PET_SPOREBAT, PET_NETHER_RAY,
    /*35*/ PET_SERPENT,      PET_NONE,         PET_MOTH,         PET_CHIMERA,  PET_DEVILSAUR,
    /*40*/ PET_NONE,         PET_SILITHID,     PET_WORM,         PET_RHINO,    PET_WASP,
    /*45*/ PET_CORE_HOUND,   PET_SPIRIT_BEAST, PET_NONE,         PET_NONE,     PET_NONE,
    /*50*/ PET_FOX,          PET_MONKEY,       PET_DOG,          PET_BEETLE,   PET_NONE,
    /*55*/ PET_SHALE_SPIDER, PET_NONE,         PET_NONE,         PET_NONE,     PET_NONE,
    /*60*/ PET_NONE,         PET_NONE,         PET_NONE,         PET_NONE,     PET_NONE,
    /*65*/ PET_NONE,         PET_WASP,         PET_NONE,         PET_NONE,     PET_NONE
  };
  int num_families = sizeof( pet_types ) / sizeof( pet_type_t );

  std::string url = "http://" + region + ".battle.net/wow/en/character/" + server + '/' + character + "/pet";
  xml_node_t* pet_xml = xml_t::get( sim, url, caching );
  if ( sim -> debug ) xml_t::print( pet_xml, sim -> output_file );

  xml_node_t* pet_list_xml = xml_t::get_node( pet_xml, "div", "class", "pets-list" );

  xml_node_t* pet_script_xml = xml_t::get_node( pet_list_xml, "script", "type", "text/javascript" );

  if ( ! pet_script_xml )
  {
    sim -> errorf( "Hunter %s unable to download pet data from Armory\n", name() );
    sim -> cancel();
    return;
  }

  std::string cdata_str;
  if ( xml_t::get_value( cdata_str, pet_script_xml, "cdata" ) )
  {
    std::string::size_type pos = cdata_str.find( '{' );
    if ( pos != std::string::npos ) cdata_str.erase( 0, pos+1 );
    pos = cdata_str.rfind( '}' );
    if ( pos != std::string::npos ) cdata_str.erase( pos );

    js_node_t* pet_js = js_t::create( sim, cdata_str );
    pet_js = js_t::get_node( pet_js, "Pet.data" );
    if ( sim -> debug ) js_t::print( pet_js, sim -> output_file );

    if ( ! pet_js )
    {
      sim -> errorf( "\nHunter %s unable to download pet data from Armory\n", name() );
      sim -> cancel();
      return;
    }

    std::vector<js_node_t*> pet_records;
    int num_pets = js_t::get_children( pet_records, pet_js );
    for ( int i=0; i < num_pets; i++ )
    {
      std::string pet_name, pet_talents;
      int pet_level, pet_family;

      if ( ! js_t::get_value( pet_name,    pet_records[ i ], "name"     ) ||
           ! js_t::get_value( pet_talents, pet_records[ i ], "build"    ) ||
           ! js_t::get_value( pet_level,   pet_records[ i ], "level"    ) ||
           ! js_t::get_value( pet_family,  pet_records[ i ], "familyId" ) )
      {
        sim -> errorf( "\nHunter %s unable to decode pet name/build/level/familyId for pet %s\n", name(), pet_name.c_str() );
        continue;
      }

      // Pets can have spaces in names, replace with underscore ..
      for ( unsigned j=0; j < pet_name.length(); j++ )
      {
        if ( pet_name[ j ] == ' ' )
          pet_name[ j ] = '_';
      }

      // Pets can have the same name, so suffix names with an unique
      // identifier from Battle.net, if one is found
      if ( js_t::get_name( pet_records[ i ] ) )
      {
        pet_name += '_';
        pet_name += js_t::get_name( pet_records[ i ] );
      }

      // Pets can have zero talents also, we should probably support it.
      /*
      bool all_zeros = true;
      for ( int j=pet_talents.size()-1; j >=0 && all_zeros; j-- )
        if ( pet_talents[ j ] != '0' )
          all_zeros = false;
      if ( all_zeros ) continue;
      */

      if ( pet_family > num_families || pet_types[ pet_family ] == PET_NONE )
      {
        sim -> errorf( "\nHunter %s unable to decode pet %s family id %d\n", name(), pet_name.c_str(), pet_family );
        continue;
      }

      hunter_pet_t* pet = new hunter_pet_t( sim, this, pet_name, pet_types[ pet_family ] );

      pet -> parse_talents_armory( pet_talents );

      pet -> talents_str.clear();
      for ( int j=0; j < MAX_TALENT_TREES; j++ )
      {
        for ( int k=0; k < ( int ) pet -> talent_trees[ j ].size(); k++ )
        {
          pet -> talents_str += ( char ) ( pet -> talent_trees[ j ][ k ] -> rank() + ( int ) '0' );
        }
      }
    }

    // If we have valid pets, figure out which to summon by parsing Battle.net
    if ( pet_list )
    {
      std::vector<xml_node_t*> pet_nodes;

      int n_pet_nodes = xml_t::get_nodes( pet_nodes, pet_list_xml, "a", "class", "pet" );
      for ( int i = 0; i < n_pet_nodes; i++ )
      {
        xml_node_t* summoned_node = xml_t::get_node( pet_nodes[ i ], "span", "class", "summoned" );
        std::string summoned_pet_name;
        std::string summoned_pet_id;

        if ( ! summoned_node )
          continue;

        xml_t::get_value( summoned_pet_id, pet_nodes[ i ], "data-id" );

        if ( ! xml_t::get_value( summoned_pet_name, xml_t::get_node( pet_nodes[ i ], "span", "class", "name" ), "." ) )
          continue;

        util_t::html_special_char_decode( summoned_pet_name );
        if ( ! summoned_pet_name.empty() )
        {
          summon_pet_str = summoned_pet_name;
          if ( ! summoned_pet_id.empty() )
            summon_pet_str += '_' + summoned_pet_id;
        }
      }

      // Pick first pet on the list, if no pet is summoned in the battle net profile
      if ( summon_pet_str.empty() )
        summon_pet_str = pet_list -> name_str;
    }
  }
}

// hunter_t::decode_set =====================================================

int hunter_t::decode_set( item_t& item )
{
  const char* s = item.name();

  // Check for Vishanka, Jaws of the Earth
  if ( item.slot == SLOT_RANGED && strstr( s, "vishanka_jaws_of_the_earth" ) )
  {
    // Store the spell id, not just if we have it or not
    vishanka = item.heroic() ? 109859 : item.lfr() ? 109857 : 107822;
  }

  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  if ( strstr( s, "lightningcharged"      ) ) return SET_T11_MELEE;
  if ( strstr( s, "flamewakers"           ) ) return SET_T12_MELEE;
  if ( strstr( s, "wyrmstalkers"          ) ) return SET_T13_MELEE;

  if ( strstr( s, "_gladiators_chain_" ) )    return SET_PVP_MELEE;

  return SET_NONE;
}

// hunter_t::moving() =======================================================

void hunter_t::moving()
{
  player_t::interrupt();

  if ( main_hand_attack ) main_hand_attack -> cancel();
  if (  off_hand_attack )  off_hand_attack -> cancel();
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_hunter  =================================================

player_t* player_t::create_hunter( sim_t* sim, const std::string& name, race_type r )
{
  return new hunter_t( sim, name, r );
}

// player_t::hunter_init ====================================================

void player_t::hunter_init( sim_t* sim )
{
  sim -> auras.trueshot              = new aura_t( sim, "trueshot" );
  sim -> auras.ferocious_inspiration = new aura_t( sim, "ferocious_inspiration" );
  sim -> auras.hunting_party         = new aura_t( sim, "hunting_party" );
  sim -> auras.roar_of_courage       = new aura_t( sim, "roar_of_courage" );
  sim -> auras.qiraji_fortitude      = new aura_t( sim, "qiraji_fortitude" );

  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> debuffs.hunters_mark         = new debuff_t( p, 1130,  "hunters_mark" );
    p -> debuffs.corrosive_spit       = new debuff_t( p, 95466, "corrosive_spit" );
    p -> debuffs.demoralizing_screech = new debuff_t( p, 24423, "demoralizing_screech" );
    p -> debuffs.lightning_breath     = new debuff_t( p, 24844, "lightning_breath" );
    p -> debuffs.ravage               = new debuff_t( p, 50518, "ravage" );
    p -> debuffs.tailspin             = new debuff_t( p, 90315, "tailspin" );
    p -> debuffs.tear_armor           = new debuff_t( p, 95467, "tear_armor" );
    p -> debuffs.tendon_rip           = new debuff_t( p, 50271, "tendon_rip" );
  }
}

// player_t::hunter_combat_begin ============================================

void player_t::hunter_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.trueshot_aura         ) sim -> auras.trueshot -> override();
  if ( sim -> overrides.ferocious_inspiration ) sim -> auras.ferocious_inspiration -> override();
  if ( sim -> overrides.hunting_party         ) sim -> auras.hunting_party -> override( 1, 0.10 );
  if ( sim -> overrides.roar_of_courage       ) sim -> auras.roar_of_courage -> override( 1, sim -> dbc.effect_min( sim -> dbc.spell( 93435 ) -> effect1().id(), sim -> max_player_level ) );
  if ( sim -> overrides.qiraji_fortitude      ) sim -> auras.qiraji_fortitude -> override( 1, sim -> dbc.effect_min( sim -> dbc.spell( 90364 ) -> effect1().id(), sim -> max_player_level ) );

  for ( player_t* t = sim -> target_list; t; t = t -> next )
  {
    double v = sim -> dbc.effect_average( sim -> dbc.spell( 1130 ) -> effect2().id(), sim -> max_player_level );
    if ( sim -> overrides.hunters_mark           ) t -> debuffs.hunters_mark -> override( 1, v );
    if ( sim -> overrides.lightning_breath       ) t -> debuffs.lightning_breath -> override( 1, 8 );
    if ( sim -> overrides.corrosive_spit         ) t -> debuffs.corrosive_spit -> override( 1, 12 );
    if ( sim -> overrides.demoralizing_screech   ) t -> debuffs.demoralizing_screech -> override( 1, 10 );
    if ( sim -> overrides.ravage                 ) t -> debuffs.ravage -> override( 1, 4 );
    if ( sim -> overrides.tailspin               ) t -> debuffs.tailspin -> override( 1, 20 );
    if ( sim -> overrides.tear_armor             ) t -> debuffs.tear_armor -> override( 1, 12 );
    if ( sim -> overrides.tendon_rip             ) t -> debuffs.tendon_rip -> override( 1, 30 );
  }
}
