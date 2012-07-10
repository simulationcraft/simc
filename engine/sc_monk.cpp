// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Monk
// ==========================================================================

enum monk_stance { STANCE_DRUNKEN_OX=1, STANCE_FIERCE_TIGER, STANCE_HEAL=4 };

struct monk_targetdata_t : public targetdata_t
{
  monk_targetdata_t( player_t* source, player_t* target )
    : targetdata_t( source, target )
  {
  }
};

void register_monk_targetdata( sim_t* /* sim */ )
{
  /* player_type t = MONK; */
  typedef monk_targetdata_t type;
}

struct monk_t : public player_t
{
  int active_stance;

  // Buffs
  //buff_t* buffs_<buffname>;

  // Gains
  gain_t* gains_jab_lf;
  gain_t* gains_jab_df;

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

    active_stance             = STANCE_FIERCE_TIGER;

    create_talents();
    create_glyphs();
    create_options();
  }

  // Character Definition
  virtual targetdata_t* new_targetdata( player_t* source, player_t* target ) {return new monk_targetdata_t( source, target );}
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
  virtual void      init_resources( bool force=false );
  virtual double    matching_gear_multiplier( const attribute_type attr ) const;
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() const;
  virtual int       primary_role() const;
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Monk Abilities
// ==========================================================================

struct monk_attack_t : public attack_t
{
  int stancemask;

  monk_attack_t( const char* n, uint32_t id, monk_t* p, int t=TREE_NONE, bool special = true ) :
    attack_t( n, id, p, t, special ),
    stancemask( STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER|STANCE_HEAL )
  {
    _init_monk_attack_t();
  }

  monk_attack_t( const char* n, const char* s_name, monk_t* p ) :
    attack_t( n, s_name, p, TREE_NONE, true ),
    stancemask( STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER|STANCE_HEAL )
  {
    _init_monk_attack_t();
  }

  void _init_monk_attack_t()
  {
    may_crit   = true;
    may_glance = false;
  }

  virtual bool   ready();
};

struct monk_spell_t : public spell_t
{
  int stancemask;

  monk_spell_t( const char* n, uint32_t id, monk_t* p ) :
    spell_t( n, id, p ),
    stancemask( STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER|STANCE_HEAL )
  {
    _init_monk_spell_t();
  }

  monk_spell_t( const char* n, const char* s_name, monk_t* p ) :
    spell_t( n, s_name, p ),
    stancemask( STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER|STANCE_HEAL )
  {
    _init_monk_spell_t();
  }

  void _init_monk_spell_t()
  {
    may_crit   = true;
  }

  virtual bool   ready();
};

struct monk_heal_t : public heal_t
{
  int stancemask;

  monk_heal_t( const char* n, uint32_t id, monk_t* p ) :
    heal_t( n, p, id ),
    stancemask( STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER|STANCE_HEAL )
  {
    _init_monk_heal_t();
  }

  monk_heal_t( const char* n, const char* s_name, monk_t* p ) :
    heal_t( n, p, s_name ),
    stancemask( STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER|STANCE_HEAL )
  {
    _init_monk_heal_t();
  }

  void _init_monk_heal_t()
  {
    may_crit   = true;
  }

  virtual bool   ready();
};

// monk_attack_t::ready ================================================

bool monk_attack_t::ready()
{
  if ( ! attack_t::ready() )
    return false;

  monk_t* p = player -> cast_monk();

  // Attack available in current stance?
  if ( ( stancemask & p -> active_stance ) == 0 )
    return false;

  return true;
}

struct jab_t : public monk_attack_t
{
  jab_t( monk_t* p, const std::string& options_str ) :
    monk_attack_t( "jab", "Jab", p )
  {
    parse_options( 0, options_str );
    resource = RESOURCE_CHI;
    base_cost = 40;
    stancemask = STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER;
  }

  virtual void execute()
  {
    monk_attack_t::execute();

    monk_t* p = player -> cast_monk();

    p -> resource_gain( RESOURCE_LIGHT_FORCE, 1, p -> gains_jab_lf );
    p -> resource_gain( RESOURCE_DARK_FORCE,  1, p -> gains_jab_df );
  }
};

struct tiger_palm_t : public monk_attack_t
{
  tiger_palm_t( monk_t* p, const std::string& options_str ) :
    monk_attack_t( "tiger_palm", "Tiger Palm", p )
  {
    parse_options( 0, options_str );
    resource = RESOURCE_LIGHT_FORCE;
    base_cost = 1;
    stancemask = STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER;
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    monk_attack_t::target_debuff( t, dmg_type );

    if ( t -> health_percentage() > 50.0 )
      target_dd_adder = 0;
    else
      target_dd_adder = 0;

  }
};

struct blackout_kick_t : public monk_attack_t
{
  blackout_kick_t( monk_t* p, const std::string& options_str ) :
    monk_attack_t( "blackout_kick", "Blackout Kick", p )
  {
    parse_options( 0, options_str );
    resource = RESOURCE_DARK_FORCE;
    base_cost = 2;
  }
};

struct spinning_crane_kick_tick_t : public monk_attack_t
{
  spinning_crane_kick_tick_t( monk_t* p ) :
    monk_attack_t( "spinning_crane_kick_tick", ( uint32_t ) 0, p )
  {
    background  = true;
    dual        = true;
    direct_tick = true;
    aoe = -1;
  }
};

struct spinning_crane_kick_t : public monk_attack_t
{
  spinning_crane_kick_tick_t* spinning_crane_kick_tick;

  spinning_crane_kick_t( monk_t* p, const std::string& options_str ) :
    monk_attack_t( "spinning_crane_kick", "Spinning Crane Kick", p ),
    spinning_crane_kick_tick( 0 )
  {
    parse_options( 0, options_str );

    stancemask = STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER;

    base_tick_time = timespan_t::from_seconds( 1.0 );
    num_ticks = 6;
    tick_zero = true;
    channeled = true;

    spinning_crane_kick_tick = new spinning_crane_kick_tick_t( p );
  }

  virtual void init()
  {
    monk_attack_t::init();

    spinning_crane_kick_tick -> stats = stats;
  }

  virtual void tick( dot_t* d )
  {
    if ( spinning_crane_kick_tick )
      spinning_crane_kick_tick -> execute();

    stats -> add_tick( d -> time_to_tick );
  }

  virtual void consume_resource()
  {
    double lf_cost = 2;
    double df_cost = 2;
    player -> resource_loss( RESOURCE_LIGHT_FORCE, lf_cost, this );
    player -> resource_loss( RESOURCE_DARK_FORCE, df_cost, this );

    if ( sim -> log )
      log_t::output( sim, "%s consumes %.1f %s and %.1f %s for %s", player -> name(),
                     lf_cost,
                     util_t::resource_type_string( RESOURCE_LIGHT_FORCE ),
                     df_cost,
                     util_t::resource_type_string( RESOURCE_DARK_FORCE ),
                     name() );

    stats -> consume_resource( lf_cost );
    stats -> consume_resource( df_cost );
  }

  virtual bool ready()
  {
    if ( ! player -> resource_available( RESOURCE_LIGHT_FORCE, 2 ) )
      return false;

    if ( ! player -> resource_available( RESOURCE_DARK_FORCE, 2 ) )
      return false;

    return monk_attack_t::ready();
  }
};

// monk_spell_t::ready ================================================

bool monk_spell_t::ready()
{
  if ( ! spell_t::ready() )
    return false;

  monk_t* p = player -> cast_monk();

  // spell available in current stance?
  if ( ( stancemask & p -> active_stance ) == 0 )
    return false;

  return true;
}

// Stance ===================================================================

struct stance_t : public monk_spell_t
{
  int switch_to_stance;
  std::string stance_str;

  stance_t( monk_t* p, const std::string& options_str ) :
    monk_spell_t( "stance", ( uint32_t ) 0, p ),
    switch_to_stance( 0 ), stance_str( "" )
  {
    option_t options[] =
    {
      { "choose",  OPT_STRING, &stance_str     },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( ! stance_str.empty() )
    {
      if ( stance_str == "drunken_ox" )
        switch_to_stance = STANCE_DRUNKEN_OX;
      else if ( stance_str == "fierce_tiger" )
        switch_to_stance = STANCE_FIERCE_TIGER;
      else if ( stance_str == "heal" )
        switch_to_stance = STANCE_HEAL;
    }
    else
    {
      // Default to Fierce Tiger Stance
      switch_to_stance = STANCE_FIERCE_TIGER;
    }

    harmful = false;
    trigger_gcd = timespan_t::zero;
    cooldown -> duration = timespan_t::from_seconds( 1.0 );
    resource    = RESOURCE_CHI;
  }

  virtual void execute()
  {
    monk_t* p = player -> cast_monk();

    monk_spell_t::execute();

    p -> active_stance = switch_to_stance;
  }

  virtual bool ready()
  {
    monk_t* p = player -> cast_monk();

    if ( p -> active_stance == switch_to_stance )
      return false;

    return monk_spell_t::ready();
  }
};

// monk_heal_t::ready ================================================

bool monk_heal_t::ready()
{
  if ( ! heal_t::ready() )
    return false;

  monk_t* p = player -> cast_monk();

  // heal available in current stance?
  if ( ( stancemask & p -> active_stance ) == 0 )
    return false;

  return true;
}

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Monk Character Definition
// ==========================================================================

// monk_t::create_action ====================================================

action_t* monk_t::create_action( const std::string& name,
                                 const std::string& options_str )
{
  if ( name == "jab"                 ) return new                 jab_t( this, options_str );
  if ( name == "tiger_palm"          ) return new          tiger_palm_t( this, options_str );
  if ( name == "blackout_kick"       ) return new       blackout_kick_t( this, options_str );
  if ( name == "spinning_crane_kick" ) return new spinning_crane_kick_t( this, options_str );
  if ( name == "stance"              ) return new              stance_t( this, options_str );

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

  default_distance = ( tree == TREE_MISTWEAVER ) ? 40 : 3;
  distance = default_distance;

  base_gcd = timespan_t::from_seconds( 1.0 ); // FIXME: assumption

  resource_base[  RESOURCE_CHI  ] = 100; // FIXME: placeholder
  resource_base[  RESOURCE_LIGHT_FORCE ] = 4;
  resource_base[  RESOURCE_DARK_FORCE  ] = 4;

  base_chi_regen_per_second = 10; // FIXME: placeholder ( identical to rogue )

  if ( tree == TREE_MISTWEAVER )
    active_stance = STANCE_HEAL;
  else if ( tree == TREE_BREWMASTER )
    active_stance = STANCE_DRUNKEN_OX;


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

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )

}

// monk_t::init_gains =======================================================

void monk_t::init_gains()
{
  player_t::init_gains();

  gains_jab_lf = get_gain( "jab_light_force" );
  gains_jab_df = get_gain( "jab_dark_force" );
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

    action_list_str += "/tiger_palm";
    action_list_str += "/blackout_kick";
    action_list_str += "/jab";

    action_list_default = 1;
  }

  player_t::init_actions();
}

// monk_t::reset ==================================================

void monk_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resource_current[ RESOURCE_LIGHT_FORCE ] = 0;
  resource_current[ RESOURCE_DARK_FORCE  ] = 0;

}

// monk_t::matching_gear_multiplier =========================================

double monk_t::matching_gear_multiplier( const attribute_type attr ) const
{
  if ( primary_tree() == TREE_MISTWEAVER )
  {
    if ( attr == ATTR_INTELLECT )
      return 0.05;
  }
  else if ( attr == ATTR_AGILITY )
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

int monk_t::primary_resource() const
{
  // FIXME: change to healing stance
  if ( primary_tree() == TREE_MISTWEAVER )
    return RESOURCE_MANA;

  return RESOURCE_CHI;
}

// monk_t::primary_role ==================================================

int monk_t::primary_role() const
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

player_t* player_t::create_monk( sim_t* sim, const std::string& /* name */, race_type /* r */ )
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
