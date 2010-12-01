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

enum aspect_type
{ ASPECT_NONE=0, ASPECT_HAWK, ASPECT_FOX, ASPECT_MAX };

struct hunter_t : public player_t
{
  // Active
  hunter_pet_t* active_pet;
  int           active_aspect;
  action_t*     active_wild_quiver;
  action_t*     active_piercing_shots;
  action_t*     active_serpent_sting;

  // Buffs
  buff_t* buffs_aspect_of_the_hawk;
  buff_t* buffs_beast_within;
  buff_t* buffs_call_of_the_wild;
  buff_t* buffs_cobra_strikes;
  buff_t* buffs_culling_the_herd;
  buff_t* buffs_furious_howl;
  buff_t* buffs_improved_steady_shot;
  buff_t* buffs_lock_and_load;
  buff_t* buffs_rapid_fire;
  buff_t* buffs_trueshot_aura;
  buff_t* buffs_tier10_2pc;
  buff_t* buffs_tier10_4pc;
  buff_t* buffs_master_marksman;

  // Gains
  gain_t* gains_glyph_of_arcane_shot;
  gain_t* gains_invigoration;
  gain_t* gains_fervor;
  gain_t* gains_rapid_recuperation;
  gain_t* gains_roar_of_recovery;
  gain_t* gains_thrill_of_the_hunt;
  gain_t* gains_steady_shot;
  gain_t* gains_glyph_aimed_shot;

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
    talent_t* readiness;
    talent_t* posthaste;
    talent_t* marked_for_death;
    talent_t* chimera_shot;
    
    // Survival
    talent_t* hunter_vs_wild;
    talent_t* pathing;
    talent_t* improved_serpent_sting;
    talent_t* survival_tactics;
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

  struct glyphs_t
  {
    glyph_t*  aimed_shot;
    glyph_t*  arcane_shot;
    glyph_t*  bestial_wrath;
    glyph_t*  chimera_shot;
    glyph_t*  explosive_shot;
    glyph_t*  kill_shot;
    glyph_t*  multi_shot;
    glyph_t*  rapid_fire;
    glyph_t*  serpent_sting;
    glyph_t*  steady_shot;
    glyph_t*  the_beast;
    glyph_t*  the_hawk;
    glyph_t*  trueshot_aura;
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
    active_wild_quiver     = 0;
    active_piercing_shots  = 0;
    active_serpent_sting   = 0;

    ranged_attack = 0;
    summon_pet_str = "wolf";
    base_gcd = 1.0;
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
  virtual double    agility() SC_CONST;
  virtual double    composite_player_multiplier( const school_type school ) SC_CONST;
  virtual double    matching_gear_multiplier( const attribute_type attr ) SC_CONST;
  virtual std::vector<talent_translation_t>& get_talent_list();
  virtual std::vector<option_t>& get_options();
  virtual cooldown_t* get_cooldown( const std::string& name );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet( const std::string& name );
  virtual void      create_pets();
  virtual void      armory( xml_node_t* sheet_xml, xml_node_t* talents_xml );
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_FOCUS; }
  virtual int       primary_role() SC_CONST     { return ROLE_ATTACK; }
  virtual bool      create_profile( std::string& profile_str, int save_type=SAVE_ALL );

  // Event Tracking
  virtual void regen( double periodicity );

  // Utility
  void cancel_sting()
  {
    if ( active_serpent_sting ) active_serpent_sting -> cancel();
  }
  action_t* active_sting()
  {
    if ( active_serpent_sting ) return active_serpent_sting;
    return 0;
  }

};

// ==========================================================================
// Hunter Pet
// ==========================================================================

struct hunter_pet_t : public pet_t
{
  int pet_type;

  // Buffs
  buff_t* buffs_bestial_wrath;
  buff_t* buffs_call_of_the_wild;
  buff_t* buffs_culling_the_herd;
  buff_t* buffs_frenzy;
  buff_t* buffs_furious_howl;
  buff_t* buffs_monstrous_bite;
  buff_t* buffs_owls_focus;
  buff_t* buffs_rabid;
  buff_t* buffs_rabid_power_stack;
  buff_t* buffs_savage_rend;
  buff_t* buffs_wolverine_bite;

  // Gains
  gain_t* gains_go_for_the_throat;
  gain_t* gains_fervor;

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
      talents.culling_the_herd = 3;
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

    base_attack_crit = 0.032 + talents.spiders_bite * 0.03;

    resource_base[ RESOURCE_HEALTH ] = rating_t::interpolate( level, 0, 4253, 6373 );
    resource_base[ RESOURCE_FOCUS  ] = 100;

    base_focus_regen_per_second  = ( 24.5 / 4.0 );
    base_focus_regen_per_second *= 1.0 + o -> talents.bestial_discipline -> rank()* 0.10;

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
    buffs_frenzy            = new buff_t( this, "frenzy",            5, 10.0, 0.0, o -> talents.frenzy -> rank() );
    buffs_furious_howl      = new buff_t( this, "furious_howl",      1, 20.0 );
    buffs_monstrous_bite    = new buff_t( this, "monstrous_bite",    3, 12.0 );
    buffs_owls_focus        = new buff_t( this, "owls_focus",        1,  8.0, 0.0, talents.owls_focus * 0.15 );
    buffs_rabid             = new buff_t( this, "rabid",             1, 20.0 );
    buffs_rabid_power_stack = new buff_t( this, "rabid_power_stack", 1,    0, 0.0, talents.rabid * 0.5 );   // FIXME: Probably a ppm, not flat chance
    buffs_savage_rend       = new buff_t( this, "savage_rend",       1, 30.0 );
    buffs_wolverine_bite    = new buff_t( this, "wolverine_bite",    1, 10.0, 0.0, talents.wolverine_bite );
  }

  virtual void init_gains()
  {
    pet_t::init_gains();
    gains_go_for_the_throat = get_gain( "go_for_the_throat" );
    gains_fervor            = get_gain( "fervor" );
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

    ap += o -> composite_attack_power() * 0.22 * ( 1 + talents.wild_hunt * 0.15 );
    ap += buffs_furious_howl -> value();

    return ap;
  }

  virtual double composite_attack_power_multiplier() SC_CONST
  {
    hunter_t* o = owner -> cast_hunter();

    double mult = player_t::composite_attack_power_multiplier();

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
                        {  3, 0, NULL                               , 0, 0 },
                        {  4, 0, NULL                               , 0, 0 },
                        {  5, 0, NULL                               , 1, 0 },
                        {  6, 0, NULL                               , 1, 0 },
                        {  7, 3, &( talents.spiked_collar          ), 1, 0 },
                        {  8, 0, NULL                               , 1, 0 },
                        {  9, 3, &( talents.culling_the_herd       ), 2, 0 },
                        { 10, 0, NULL                               , 2, 0 },
                        { 11, 0, NULL                               , 2, 0 },
                        { 12, 0, NULL                               , 3, 6 },
                        { 13, 3, &( talents.spiders_bite           ), 3, 0 },
                        { 14, 0, NULL                               , 3, 0 },
                        { 15, 1, &( talents.rabid                  ), 4, 9 },
                        { 16, 0, NULL                               , 4, 12 },
                        { 17, 1, &( talents.call_of_the_wild       ), 4, 13 },
                        { 18, 2, &( talents.shark_attack           ), 5, 0 },
                        { 19, 2, &( talents.wild_hunt              ), 5, 17 },
                        {  0, 0, NULL                                } /// must have talent with index 0 here
                  };
                  translation_table = group_table;
                }
                else if ( group() == PET_CUNNING )
                {
                  talent_translation_t group_table[] =
                  {
                        {  1, 2, &( talents.cobra_reflexes         ), 0, 0 },
                        {  2, 0, NULL                               , 0, 0 },
                        {  3, 0, NULL                               , 0, 0 },
                        {  4, 0, NULL                               , 0, 0 },
                        {  5, 0, NULL                               , 1, 0 },
                        {  6, 0, NULL                               , 1, 2 },
                        {  7, 2, &( talents.owls_focus             ), 1, 0 },
                        {  8, 3, &( talents.spiked_collar          ), 1, 0 },
                        {  9, 3, &( talents.culling_the_herd       ), 2, 0 },
                        { 10, 0, NULL                               , 2, 0 },
                        { 11, 0, NULL                               , 2, 0 },
                        { 12, 0, NULL                               , 3, 0 },
                        { 13, 0, NULL                               , 3, 0 },
                        { 14, 2, &( talents.feeding_frenzy         ), 3, 8 },
                        { 15, 1, &( talents.wolverine_bite         ), 4, 9 },
                        { 16, 1, &( talents.roar_of_recovery       ), 4, 0 },
                        { 17, 0, NULL                               , 4, 0 },
                        { 18, 0, NULL                               , 4, 0 },
                        { 19, 2, &( talents.wild_hunt              ), 5, 15 },
                        { 20, 0, NULL                               , 5, 18 },
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

// trigger_hunting_party ===================================================

static void trigger_hunting_party( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();

  if ( ! p -> talents.hunting_party -> rank() )
    return;

  p -> trigger_replenishment();
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

  double gain = util_t::talent_rank(  p -> talents.thrill_of_the_hunt -> rank(), 3, 0.10, 0.20, 0.40 );
  gain *= a -> resource_consumed;
  
  p -> resource_gain( RESOURCE_FOCUS, gain, p -> gains_thrill_of_the_hunt );
}

// trigger_wild_quiver ===============================================

static void trigger_wild_quiver( attack_t* a )
{
  hunter_t* p = a -> player -> cast_hunter();
  
  // FIXME!

  if ( a -> proc )
    return;


  double chance = 0;
  chance = 0; // p -> talents.wild_quiver * 0.04;

  if ( p -> rng_wild_quiver -> roll( chance ) )
  {
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
        base_multiplier *= 0.80;
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

} // ANONYMOUS NAMESPACE ====================================================

// =========================================================================
// Hunter Pet Attacks
// =========================================================================

struct hunter_pet_attack_t : public attack_t
{
  hunter_pet_attack_t( const char* n, player_t* player, int r=RESOURCE_FOCUS, const school_type sc=SCHOOL_PHYSICAL, bool special=true ) :
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
        trigger_invigoration( this );
        p -> buffs_wolverine_bite -> trigger();
      }
    }
    if ( special )
    {
      p -> buffs_owls_focus -> trigger();
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

    if ( p -> buffs_savage_rend -> up() ) player_multiplier *= 1.10;

    player_multiplier *= 1.0 + p -> buffs_monstrous_bite -> stack() * 0.03;

    if ( p -> buffs_culling_the_herd -> up() )
      player_multiplier *= 1.0 + ( p -> buffs_culling_the_herd -> value() * 0.01 );

    if (target -> health_percentage() < 35 )
      player_multiplier *= 1.0 + p -> talents.feeding_frenzy * 0.06;

    if ( ! special )
    {
      if ( o -> buffs_cobra_strikes -> up() ) player_crit += 1.0;
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
    cooldown -> duration = 10 * ( 1.0 - o -> talents.longevity -> rank() * 0.10 );
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
    cooldown -> duration = 10 * ( 1.0 - o -> talents.longevity -> rank() * 0.10 );
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

    parse_options( NULL, options_str );

    base_dd_min = base_dd_max = 71;
    base_cost      = 20;
    base_td_init   = 24;
    num_ticks      = 3;
    base_tick_time = 5;
    tick_power_mod = 0.0175; // FIXME Check
    cooldown -> duration = 60 * ( 1.0 - o -> talents.longevity -> rank() * 0.10 );
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
    cooldown -> duration = 10 * ( 1.0 - o -> talents.longevity -> rank() * 0.10 );
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
  hunter_pet_spell_t( const char* n, player_t* player, int r=RESOURCE_FOCUS, const school_type s=SCHOOL_PHYSICAL ) :
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
      player_multiplier *= 1.0 + ( p -> buffs_culling_the_herd -> value() * 0.01 );

    if ( target -> health_percentage() < 35 )
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

    parse_options( NULL, options_str );

    base_dd_min = base_dd_max = 150;
    base_cost = 20;
    direct_power_mod = 1.5 / 3.5;
    cooldown -> duration = 10 * ( 1.0 - o -> talents.longevity -> rank() * 0.10 );
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
    cooldown -> duration = 10 * ( 1.0 - o -> talents.longevity -> rank() * 0.10 );
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
    cooldown -> duration = 5 * 60 * ( 1.0 - o -> talents.longevity -> rank() * 0.10 );
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
    cooldown -> duration = 40 * ( 1.0 - o -> talents.longevity -> rank() * 0.10 );
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
    cooldown -> duration = 45 * ( 1.0 - o -> talents.longevity -> rank() * 0.10 );
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
    cooldown -> duration = 360 * ( 1.0 - o -> talents.longevity -> rank() * 0.10 );
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
  if ( p -> buffs_beast_within -> up() ) c *= 0.50;
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


  hunter_pet_t* pet = p -> active_pet;
  if ( p -> talents.the_beast_within -> rank() && pet -> buffs_bestial_wrath -> check() )
  {
    player_multiplier *= 1.10;
  }
  if ( p -> active_sting() && p -> talents.noxious_stings -> rank() )
  {
    player_multiplier *= 1.0 + p -> talents.noxious_stings -> rank() * 0.05;
  }

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

    normalize_weapon_speed = true;
    // FIXME! #1 haste lowers cd!
    // FIXME! #2 stop autoattacking during cast!
  }

  virtual void player_buff()
  {
    hunter_t* p = player -> cast_hunter();
    hunter_attack_t::player_buff();
    if ( p -> buffs_trueshot_aura -> check() && p -> glyphs.trueshot_aura -> ok() ) player_crit += 0.10;
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
      trigger_piercing_shots( this );
      p -> resource_gain( RESOURCE_FOCUS, p -> glyphs.aimed_shot -> effect_base_value( 1 ), p -> gains_glyph_aimed_shot );
    }
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
    weapon_multiplier = 0;

    direct_power_mod = 0.042;

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
    if ( result == RESULT_CRIT )
    {
      p -> buffs_cobra_strikes -> trigger( 2 );
      trigger_thrill_of_the_hunt( this );
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
    cooldown -> duration -= p -> talents.resourcefulness -> rank() * 2;

    base_multiplier *= 1.0 + p -> talents.trap_mastery -> rank() * 0.10;
  }

  virtual void tick()
  {
    hunter_attack_t::tick();
    hunter_t* p = player -> cast_hunter();
    p -> buffs_lock_and_load -> trigger( 2 );
  }
};

// Chimera Shot ================================================================

struct chimera_shot_t : public hunter_attack_t
{
  int active_sting;

  chimera_shot_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "chimera_shot", player, "Chimera Shot" ),
      active_sting( 1 )
  {
    hunter_t* p = player -> cast_hunter();

    check_talent( p -> talents.chimera_shot -> rank() );

    option_t options[] =
    {
      { "active_sting",         OPT_BOOL, &active_sting         },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    direct_power_mod = 0.488;

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );

    base_cost   -= p -> talents.efficiency -> rank() * 2;

    normalize_weapon_speed = true;

    cooldown -> duration -= p -> glyphs.chimera_shot -> ok();

  }


  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();

    hunter_attack_t::execute();

    action_t* sting = p -> active_sting();

    if ( result_is_hit() )
    {
      if ( result == RESULT_CRIT )
      {
        trigger_piercing_shots( this );
      }
      if ( sting )
        sting -> refresh_duration();
    }
  }

  virtual bool ready()
  {
    hunter_t* p = player -> cast_hunter();

    if ( active_sting )
      if ( ! p -> active_sting() )
        return false;

    return hunter_attack_t::ready();
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
      trigger_hunting_party( this );
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
  }
};

// Explosive Shot ================================================================

struct explosive_tick_t : public hunter_attack_t
{
  explosive_tick_t( player_t* player ) :
      hunter_attack_t( "explosive_shot", player, 63854 )
  {
    hunter_t* p = player -> cast_hunter();

    // To trigger ppm-based abilities
    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );
    weapon_multiplier = 0;

    dual       = true;
    background = true;
    may_miss   = false;



    if ( p -> glyphs.explosive_shot -> ok() )
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
      hunter_attack_t( "explosive_shot", player, "Explosive Shot" ),
      lock_and_load( 0 )
  {
    hunter_t* p = player -> cast_hunter();

    assert ( p -> primary_tree() == TREE_SURVIVAL );

    parse_options( NULL, options_str );

    base_cost -= p -> talents.efficiency -> rank() * 2;

    direct_power_mod = 0.273;

    tick_zero      = true;
    explosive_tick = new explosive_tick_t( p );

  }

  virtual double cost() SC_CONST
  {
    hunter_t* p = player -> cast_hunter();
    if ( p -> buffs_lock_and_load -> check() ) return 0;
    return hunter_attack_t::cost();
  }

  virtual void tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), dot -> current_tick, dot -> num_ticks );
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

    weapon_multiplier = 1.5;
    direct_power_mod = 0.3;

                normalize_weapon_speed = true;
    if ( p -> glyphs.kill_shot -> ok() )
    {
      cooldown -> duration -= 6;
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

// Serpent Sting Attack =========================================================

struct serpent_sting_t : public hunter_attack_t
{
  int force;

  serpent_sting_t( player_t* player, const std::string& options_str ) :
      hunter_attack_t( "serpent_sting", player, "Serpent Sting" ),
      force( 0 )
  {
    hunter_t* p = player -> cast_hunter();

    option_t options[] =
    {
      { "force", OPT_BOOL, &force },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    tick_power_mod = 0.4 / num_ticks;

    num_ticks        += p -> glyphs.serpent_sting -> ok() ? 2 : 0;




    observer = &( p -> active_serpent_sting );
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();
    p -> cancel_sting();
    hunter_attack_t::execute();
    if ( result_is_hit() ) target -> debuffs.poisoned -> increment();
  }

  virtual void last_tick()
  {
    hunter_attack_t::last_tick();
    target -> debuffs.poisoned -> decrement();
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

    weapon = &( p -> ranged_weapon );
    assert( weapon -> group() == WEAPON_RANGED );
  }
  void execute()
  {
    hunter_attack_t::execute();
    if ( result_is_hit() )
    {
      hunter_t* p = player -> cast_hunter();
      p -> buffs_improved_steady_shot -> trigger();

      p -> resource_gain( RESOURCE_FOCUS, 9, p -> gains_steady_shot );

      trigger_hunting_party( this );
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
    if ( (p -> glyphs.steady_shot -> ok() || p -> talents.marked_for_death -> rank() ) && p -> active_sting() )
    {
      player_multiplier *= 1.10;
    }
    if ( p -> talents.careful_aim -> rank() && target -> health_percentage() > 80 )
    {
      player_crit += p -> talents.careful_aim -> effect_base_value( 1 ) / 100.0;
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
  int    hawk_always;
  double hawk_bonus;

  aspect_t( player_t* player, const std::string& options_str ) :
      hunter_spell_t( "aspect", player, SCHOOL_NATURE, TREE_BEAST_MASTERY ),
      hawk_always( 0 ), hawk_bonus( 0 )
  {
    hunter_t* p = player -> cast_hunter();

    option_t options[] =
    {
      { "hawk_always",     OPT_BOOL, &hawk_always },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd      = 0.0;
    harmful          = false;
    hawk_bonus       = util_t::ability_rank( p -> level, 300,80, 230,74, 155,68,  120,0 );

  }

  int choose_aspect()
  {
    //hunter_t* p = player -> cast_hunter();

    if ( hawk_always ) return ASPECT_HAWK;

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
        p -> buffs_aspect_of_the_hawk -> trigger( 1, hawk_bonus );
      }
      else
      {
        p -> active_aspect = ASPECT_NONE;
        p -> buffs_aspect_of_the_hawk -> expire();
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
      hunter_spell_t( "bestial_wrath", player, "Bestial Wrath" )
  {
    hunter_t* p = player -> cast_hunter();
    parse_options( NULL, options_str );
    check_talent( p -> talents.bestial_wrath -> rank() );

    cooldown -> duration -= p -> glyphs.bestial_wrath -> ok() * 20;
    cooldown -> duration *= ( 1 - p -> talents.longevity -> rank() * 0.1 );

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
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> active_pet -> resource_gain( RESOURCE_FOCUS, 50, p -> active_pet -> gains_fervor );
    p               -> resource_gain( RESOURCE_FOCUS, 50, p               -> gains_fervor );

    p -> buffs_rapid_fire -> trigger( 1, ( p -> glyphs.rapid_fire -> ok() ? 0.48 : 0.40 ) );

    consume_resource();
    update_ready();
  }
};

// Hunter's Mark =========================================================

struct hunters_mark_t : public hunter_spell_t
{
  double ap_bonus;

  hunters_mark_t( player_t* player, const std::string& options_str ) :
      hunter_spell_t( "hunters_mark", player, "Hunter's Mark" ), ap_bonus( 0 )
  {
    hunter_t* p = player -> cast_hunter();
    parse_options( NULL, options_str );

    ap_bonus = util_t::ability_rank( p -> level,  1772,85, 1600,84, 1500,83, 1200,82, 1000,81, 500,76,  110,0 );


  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();

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
      hunter_spell_t( "rapid_fire", player, "Rapid Fire" ), viper( 0 )
  {
    hunter_t* p = player -> cast_hunter();

    option_t options[] =
    {
      { "viper", OPT_BOOL, &viper },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown -> duration -= p -> talents.rapid_killing -> rank() * 60;
    harmful = false;
  }

  virtual void execute()
  {
    hunter_t* p = player -> cast_hunter();

    p -> buffs_rapid_fire -> trigger( 1, ( p -> glyphs.rapid_fire -> ok() ? 0.48 : 0.40 ) );

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
  if ( name == "serpent_sting"         ) return new          serpent_sting_t( this, options_str );
  if ( name == "silencing_shot"        ) return new         silencing_shot_t( this, options_str );
  if ( name == "steady_shot"           ) return new            steady_shot_t( this, options_str );
  if ( name == "summon_pet"            ) return new             summon_pet_t( this, options_str );
  if ( name == "trueshot_aura"         ) return new          trueshot_aura_t( this, options_str );

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

// hunter_t::init_talents ===================================================

void hunter_t::init_talents()
{

  // Beast Mastery
  talents.improved_kill_command           = new talent_t ( this, "improved_kill_command", "Improved Kill Command" );
  talents.one_with_nature                 = new talent_t ( this, "one_with_nature", "One with Nature" );
  talents.bestial_discipline              = new talent_t ( this, "bestial_discipline", "Bestial Discipline" );
  talents.pathfinding                     = new talent_t ( this, "pathfinding", "Pathfinding" );
  talents.spirit_bond                     = new talent_t ( this, "spirit_bond", "Spirit Bond" );
  talents.frenzy                          = new talent_t ( this, "frenzy", "Frenzy" );
  talents.improved_mend_pet               = new talent_t ( this, "improved_mend_pet", "Improved Mend Pet" );
  talents.cobra_strikes                   = new talent_t ( this, "cobra_strikes", "Cobra Strikes" );
  talents.fervor                          = new talent_t ( this, "fervor", "Fervor" );
  talents.focus_fire                      = new talent_t ( this, "focus_fire", "Focus Fire" );
  talents.longevity                       = new talent_t ( this, "longevity", "Longevity" );
  talents.killing_streak                  = new talent_t ( this, "killing_streak", "Killing Streak" );
  talents.crouching_tiger_hidden_chimera  = new talent_t ( this, "crouching_tiger_hidden_chimera", "Crouching Tiger, Hidden Chimera" );
  talents.bestial_wrath                   = new talent_t ( this, "bestial_wrath", "Bestial Wrath" );
  talents.ferocious_inspiration           = new talent_t ( this, "ferocious_inspiration", "Ferocious Inspiration" );
  talents.kindred_spirits                 = new talent_t ( this, "kindred_spirits", "Kindred Spirits" );
  talents.the_beast_within                = new talent_t ( this, "the_beast_within", "The Beast Within" );
  talents.invigoration                    = new talent_t ( this, "invigoration", "Invigoration" );
  talents.beast_mastery                   = new talent_t ( this, "beast_mastery", "Beast Mastery" );

  // Marksmanship
  talents.go_for_the_throat               = new talent_t ( this, "go_for_the_throat", "Go for the Throat" );
  talents.efficiency                      = new talent_t ( this, "efficiency", "Efficiency" );
  talents.rapid_killing                   = new talent_t ( this, "rapid_killing", "Rapid Killing" );
  talents.sic_em                          = new talent_t ( this, "sic_em", "Sic 'Em!" );
  talents.improved_steady_shot            = new talent_t ( this, "improved_steady_shot", "Improved Steady Shot" );
  talents.careful_aim                     = new talent_t ( this, "careful_aim", "Careful Aim" );
  talents.silencing_shot                  = new talent_t ( this, "silencing_shot", "Silencing Shot" );
  talents.concussive_barrage              = new talent_t ( this, "concussive_barrage", "Concussive Barrage" );
  talents.piercing_shots                  = new talent_t ( this, "piercing_shots", "Piercing Shots" );
  talents.bombardment                     = new talent_t ( this, "bombardment", "Bombardment" );
  talents.trueshot_aura                   = new talent_t ( this, "trueshot_aura", "Trueshot Aura" );
  talents.termination                     = new talent_t ( this, "termination", "Termination" );
  talents.resistance_is_futile            = new talent_t ( this, "resistance_is_futile", "Resistance is Futile" );
  talents.rapid_recuperation              = new talent_t ( this, "rapid_recuperation", "Rapid Recuperation" );
  talents.master_marksman                 = new talent_t ( this, "master_marksman", "Master Marksman" );
  talents.readiness                       = new talent_t ( this, "readiness", "Readiness" );
  talents.posthaste                       = new talent_t ( this, "posthaste", "Posthaste" );
  talents.marked_for_death                = new talent_t ( this, "marked_for_death", "Marked for Death" );
  talents.chimera_shot                    = new talent_t ( this, "chimera_shot", "Chimera Shot" );

  // Survival
  talents.hunter_vs_wild                  = new talent_t ( this, "hunter_vs_wild", "Hunter vs. Wild" );
  talents.pathing                         = new talent_t ( this, "pathing", "Pathing" );
  talents.improved_serpent_sting          = new talent_t ( this, "improved_serpent_sting", "Improved Serpent Sting" );
  talents.survival_tactics                = new talent_t ( this, "survival_tactics", "Survival Tactics" );
  talents.trap_mastery                    = new talent_t ( this, "trap_mastery", "Trap Mastery" );
  talents.entrapment                      = new talent_t ( this, "entrapment", "Entrapment" );
  talents.point_of_no_escape              = new talent_t ( this, "point_of_no_escape", "Point of No Escape" );
  talents.thrill_of_the_hunt              = new talent_t ( this, "thrill_of_the_hunt", "Thrill of the Hunt" );
  talents.counterattack                   = new talent_t ( this, "counterattack", "Counterattack" );
  talents.lock_and_load                   = new talent_t ( this, "lock_and_load", "Lock and Load" );
  talents.resourcefulness                 = new talent_t ( this, "resourcefulness", "Resourcefulness" );
  talents.mirrored_blades                 = new talent_t ( this, "mirrored_blades", "Mirrored Blades" );
  talents.tnt                             = new talent_t ( this, "tnt", "T.N.T." );
  talents.toxicology                      = new talent_t ( this, "toxicology", "Toxicology" );
  talents.wyvern_sting                    = new talent_t ( this, "wyvern_sting", "Wyvern Sting" );
  talents.noxious_stings                  = new talent_t ( this, "noxious_stings", "Noxious Stings" );
  talents.hunting_party                   = new talent_t ( this, "hunting_party", "Hunting Party" );
  talents.sniper_training                 = new talent_t ( this, "sniper_training", "Sniper Training" );
  talents.serpent_spread                  = new talent_t ( this, "serpent_spread", "Serpent Spread" );
  talents.black_arrow                     = new talent_t ( this, "black_arrow", "Black Arrow" );

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
  passive_spells.essence_of_the_viper = new mastery_t( this, "essence_of_the_viper", "essence_of_the_viper", TREE_SURVIVAL );

  glyphs.aimed_shot = new glyph_t(this, "Glyph of Aimed Shot");
  glyphs.arcane_shot = new glyph_t(this, "Glyph of Arcane Shot");
  glyphs.bestial_wrath = new glyph_t(this, "Glyph of Bestial Wrath");
  glyphs.chimera_shot = new glyph_t(this, "Glyph of Chimera Shot");
  glyphs.explosive_shot = new glyph_t(this, "Glyph of Explosive Shot");
  glyphs.kill_shot = new glyph_t(this, "Glyph of Kill Shot");
  glyphs.multi_shot = new glyph_t(this, "Glyph of Multi Shot");
  glyphs.rapid_fire = new glyph_t(this, "Glyph of Rapid Fire");
  glyphs.serpent_sting = new glyph_t(this, "Glyph of Serpent Sting");
  glyphs.steady_shot = new glyph_t(this, "Glyph of Steady Shot");
  glyphs.the_beast = new glyph_t(this, "Glyph of the Beast");
  glyphs.the_hawk = new glyph_t(this, "Glyph of the Hawk");
  glyphs.trueshot_aura = new glyph_t(this, "Glyph of Trueshot Aura");
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
    else if ( n == "multi_shot"                  ) glyphs.multi_shot -> enable();
    else if ( n == "rapid_fire"                  ) glyphs.rapid_fire -> enable();
    else if ( n == "serpent_sting"               ) glyphs.serpent_sting -> enable();
    else if ( n == "steady_shot"                 ) glyphs.steady_shot -> enable();
    else if ( n == "the_beast"                   ) glyphs.the_beast -> enable();
    else if ( n == "the_hawk"                    ) glyphs.the_hawk -> enable();
    else if ( n == "trueshot_aura"               ) glyphs.trueshot_aura -> enable();
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
    else if ( n == "ice_trap"         ) ;
    else if ( n == "scatter_shot"       ) ;
    else if ( n == "trap_launcher"       ) ;
    else if ( n == "misdirection"       ) ;
    else if ( n == "aspect_of_the_pack"       ) ;
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
  
  resource_base[ RESOURCE_FOCUS ] = 100;

  position = POSITION_RANGED;

  diminished_kfactor    = 0.009880;
  diminished_dodge_capi = 0.006870;
  diminished_parry_capi = 0.006870;
}

// hunter_t::init_buffs =======================================================

void hunter_t::init_buffs()
{
  player_t::init_buffs();

  buffs_aspect_of_the_hawk          = new buff_t( this, "aspect_of_the_hawk",          1,  0.0 );
  buffs_beast_within                = new buff_t( this, "beast_within",                1, 10.0,  0.0, talents.the_beast_within -> rank() );
  buffs_call_of_the_wild            = new buff_t( this, "call_of_the_wild",            1, 10.0 );
  buffs_cobra_strikes               = new buff_t( this, "cobra_strikes",               2, 10.0,  0.0, talents.cobra_strikes -> rank() * 0.20 );
  buffs_culling_the_herd            = new buff_t( this, "culling_the_herd",            1, 10.0 );
  buffs_furious_howl                = new buff_t( this, "furious_howl",                1, 20.0 );
  buffs_improved_steady_shot        = new buff_t( this, "improved_steady_shot",        1,  8.0,  0.0, talents.improved_steady_shot -> rank() );
  buffs_lock_and_load               = new buff_t( this, "lock_and_load",               2, 10.0, 22.0, talents.lock_and_load -> rank() * (0.20/3.0) ); // EJ thread suggests the proc is rate is around 20%
  buffs_master_marksman             = new buff_t( this, 82925, "master_marksman", talents.master_marksman -> proc_chance());

  buffs_rapid_fire                  = new buff_t( this, "rapid_fire",                  1, 15.0 );

  buffs_tier10_2pc                  = new buff_t( this, "tier10_2pc",                  1, 10.0,  0.0, ( set_bonus.tier10_2pc_melee() ? 0.05 : 0 ) );
  buffs_tier10_4pc                  = new buff_t( this, "tier10_4pc",                  1, 10.0,  0.0, ( set_bonus.tier10_4pc_melee() ? 0.05 : 0 ) );

  // Own TSA for Glyph of TSA
  buffs_trueshot_aura               = new buff_t( this, "trueshot_aura");
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
    action_list_str = "flask,type=endless_rage";
    action_list_str += ( primary_tree() != TREE_SURVIVAL ) ? "/food,type=hearty_rhino" : "/food,type=blackened_dragonfin";
    action_list_str += "/hunters_mark/summon_pet";
    if ( talents.trueshot_aura -> rank() ) action_list_str += "/trueshot_aura";
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
      action_list_str += "/aspect";
      if ( talents.rapid_killing -> rank() == 0 )
      {
        action_list_str += "/rapid_fire,if=buff.bloodlust.react";
      }
      else if ( talents.rapid_killing -> rank() == 1 )
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
      action_list_str += "/steady_shot";
      break;
    case TREE_MARKSMANSHIP:
      action_list_str += "/aspect";
      action_list_str += "/serpent_sting";
      action_list_str += "/rapid_fire";
      action_list_str += "/kill_command";
      action_list_str += "/kill_shot";
      if ( talents.chimera_shot -> rank() )
      {
        action_list_str += "/wait,sec=cooldown.chimera_shot.remains,if=cooldown.chimera_shot.remains>0&cooldown.chimera_shot.remains<0.25";
        action_list_str += "/chimera_shot";
      }
      action_list_str += "/aimed_shot";
      action_list_str += "/readiness,wait_for_rapid_fire=1";
      action_list_str += "/steady_shot";
      break;
    case TREE_SURVIVAL:
      action_list_str += "/aspect";
      action_list_str += "/rapid_fire";
      action_list_str += "/kill_command";
      action_list_str += "/kill_shot";
      if ( talents.lock_and_load -> rank() )
      {
        action_list_str += "/explosive_shot,if=buff.lock_and_load.react";
        action_list_str += "/explosive_shot,if=!buff.lock_and_load.up";
      }
      else if ( talents.lock_and_load -> rank() == 0 )
      {
        action_list_str += "/explosive_shot";
      }
      if ( talents.black_arrow -> rank() ) action_list_str += "/black_arrow";
      action_list_str += "/serpent_sting";
      action_list_str += "/multi_shot";
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
}

// hunter_t::reset ===========================================================

void hunter_t::reset()
{
  player_t::reset();

  // Active
  active_pet            = 0;
  active_aspect         = ASPECT_NONE;
  active_serpent_sting  = 0;

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

  ap += buffs_furious_howl -> value();

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
    mult *= 1.1;
  }

  return mult;
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
       if ( school == SCHOOL_NATURE && passive_spells.essence_of_the_viper -> ok() )
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

// hunter_t::get_talent_trees ==============================================

std::vector<talent_translation_t>& hunter_t::get_talent_list()
{
  talent_list.clear();
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

      // @option_doc loc=player/hunter/misc title="Misc"
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
  profile_str+= "regen_periodicy=0.25";
  return player_t::create_profile( profile_str, save_type );
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
  sim -> errorf( "Hunter Module isn't avaiable at the moment." );

  //return new hunter_t( sim, name, r );
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

