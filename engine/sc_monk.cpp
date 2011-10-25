// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Monk
// ==========================================================================

struct monk_t : public player_t
{
  // Buffs
  //buff_t* buffs_<buffname>;

  // Gains
  //gain_t* gains_<gainname>;

  // Procs
  //proc_t* procs_<procname>;

  // Talents
  struct talents_t
  {
    // TREE_MONK_TANK
    //talent_t* <talentname>;

    // TREE_MONK_DAMAGE

    // TREE_MONK_HEAL

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  // Passives
  struct passives_t
  {
    // TREE_MONK_TANK
    // spell_id_t* mastery/passive spells

    // TREE_MONK_DAMAGE

    // TREE_MONK_HEAL

    passives_t() { memset( ( void* ) this, 0x0, sizeof( passives_t ) ); }
  };
  passives_t passives;

  // Glyphs
  struct glyphs_t
  {
    // Prime
    //glyph_t* <glyphname>;

    // Major

    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  monk_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) :
    player_t( sim, MONK, name, ( r == RACE_NONE ) ? RACE_PANDAREN : r )
  {

    // FIXME
    tree_type[ MONK_BREWMASTER    ] = TREE_BREWMASTER;
    tree_type[ MONK_WINDWALKER  ] = TREE_WINDWALKER;
    tree_type[ MONK_MISTWEAVER    ] = TREE_MISTWEAVER;


    create_talents();
    create_glyphs();
    create_options();
  }

  // Character Definition
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual void      init_talents();
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      combat_begin();
  virtual double    matching_gear_multiplier( const attribute_type attr ) SC_CONST;
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_CHI; }
  virtual int       primary_role() SC_CONST;
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Monk Abilities
// ==========================================================================

struct monk_attack_t : public attack_t
{

  monk_attack_t( const char* n, uint32_t id, monk_t* p, int t=TREE_NONE, bool special = true ) :
    attack_t( n, id, p, t, special )
  {
    _init_monk_attack_t();
  }

  monk_attack_t( const char* n, const char* s_name, monk_t* p ) :
    attack_t( n, s_name, p, TREE_NONE, true )
  {
    _init_monk_attack_t();
  }

  void _init_monk_attack_t()
  {
    may_crit   = true;
    may_glance = false;
  }
};

struct monk_spell_t : public spell_t
{

  monk_spell_t( const char* n, uint32_t id, monk_t* p ) :
    spell_t( n, id, p )
  {
    _init_monk_spell_t();
  }

  monk_spell_t( const char* n, const char* s_name, monk_t* p ) :
    spell_t( n, s_name, p )
  {
    _init_monk_spell_t();
  }

  void _init_monk_spell_t()
  {
    may_crit   = true;
  }
};

struct monk_heal_t : public heal_t
{

  monk_heal_t( const char* n, uint32_t id, monk_t* p ) :
    heal_t( n, p, id )
  {
    _init_monk_heal_t();
  }

  monk_heal_t( const char* n, const char* s_name, monk_t* p ) :
    heal_t( n, p, s_name )
  {
    _init_monk_heal_t();
  }

  void _init_monk_heal_t()
  {
    may_crit   = true;
  }
};

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Monk Character Definition
// ==========================================================================

// monk_t::create_action ====================================================

action_t* monk_t::create_action( const std::string& name,
                                 const std::string& options_str )
{
  // Add Abilities
  //if ( name == "<abilityname>"    ) return new          <ability>_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// monk_t::init_talents =====================================================

void monk_t::init_talents()
{
  player_t::init_talents();

  // TREE_MONK_TANK
  //talents.<name>        = find_talent( "<talentname>" );

  // TREE_MONK_DAMAGE

  // TREE_MONK_HEAL

}

// monk_t::init_spells ======================================================

void monk_t::init_spells()
{
  player_t::init_spells();

  // Add Spells & Glyphs

  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P    M2P    M4P    T2P    T4P    H2P    H4P
    {      0,      0,     0,     0,     0,     0,     0,     0 }, // Tier11
    {      0,      0,     0,     0,     0,     0,     0,     0 }, // Tier12
    {      0,      0,     0,     0,     0,     0,     0,     0 }, // Tier13
    {      0,      0,     0,     0,     0,     0,     0,     0 }, // Tier14
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// monk_t::init_base ========================================================

void monk_t::init_base()
{
  player_t::init_base();

  int tree = primary_tree();

  default_distance = (tree == TREE_MISTWEAVER ) ? 40 : 3;
  distance = default_distance;

  resource_base[  RESOURCE_CHI  ] = 100; // FIXME: placeholder

  // FIXME: Add defensive constants
  //diminished_kfactor    = 0;
  //diminished_dodge_capi = 0;
  //diminished_parry_capi = 0;
}

// monk_t::init_scaling =====================================================

void monk_t::init_scaling()
{
  player_t::init_scaling();

}

// monk_t::init_buffs =======================================================

void monk_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, cooldown, proc_chance, quiet )
  // buff_t( player, id, name chance, cd, quiet, reverse, rng_type, activated )

}

// monk_t::init_gains =======================================================

void monk_t::init_gains()
{
  player_t::init_gains();

}

// monk_t::init_procs =======================================================

void monk_t::init_procs()
{
  player_t::init_procs();

}

// monk_t::init_rng =========================================================

void monk_t::init_rng()
{
  player_t::init_rng();

}

// monk_t::init_actions =====================================================

void monk_t::init_actions()
{
  if ( action_list_str.empty() )
  {

    // Flask

    // Food
    if ( level >= 80 ) action_list_str += "food,type=seafood_magnifique_feast";
    else if ( level >= 70 ) action_list_str += "food,type=fish_feast";


    action_list_str += "/snapshot_stats";

    action_list_default = 1;
  }

  player_t::init_actions();
}

// monk_t::combat_begin ==================================================

void monk_t::combat_begin()
{
  player_t::combat_begin();

  // Start with 0 Chi
  resource_current[ RESOURCE_CHI ] = 0;
}

// monk_t::matching_gear_multiplier =========================================

double monk_t::matching_gear_multiplier( const attribute_type attr ) SC_CONST
{
  if( primary_tree() == TREE_MISTWEAVER )
  {
    if ( attr == ATTR_INTELLECT )
      return 0.05;
  }
  else if( attr == ATTR_AGILITY )
    return 0.05;

  return 0.0;
}

// monk_t::decode_set =======================================================

int monk_t::decode_set( item_t& item )
{
  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  //const char* s = item.name();

  //if ( strstr( s, "<setname>"      ) ) return SET_T14_TANK;
  //if ( strstr( s, "<setname>"      ) ) return SET_T14_MELEE;
  //if ( strstr( s, "<setname>"      ) ) return SET_T14_HEAL;

  return SET_NONE;
}

// monk_t::primary_role ==================================================

int monk_t::primary_role() SC_CONST
{
  if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_HYBRID )
    return ROLE_HYBRID;

  if ( player_t::primary_role() == ROLE_TANK  )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_HEAL )
    return ROLE_HEAL;

  if ( primary_tree() == TREE_BREWMASTER )
    return ROLE_TANK;

  if ( primary_tree() == TREE_MISTWEAVER )
    return ROLE_HEAL;

  return ROLE_HYBRID;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_monk  ===================================================

player_t* player_t::create_monk( sim_t* sim, const std::string& /* name */ , race_type /* r */ )
{
  sim -> errorf( "Monk Module isn't available at the moment." );

  //return new monk_t( sim, name, r );

  return NULL;
}

// player_t::monk_init ======================================================

void player_t::monk_init( sim_t* /* sim */ )
{
  //for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  //{}
}

// player_t::monk_combat_begin ==============================================

void player_t::monk_combat_begin( sim_t* /* sim */ )
{
  //for ( player_t* p = sim -> player_list; p; p = p -> next )
  //{}
}
