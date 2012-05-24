// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_class_modules.hpp"

// ==========================================================================
// Monk
// ==========================================================================

namespace { // ANONYMOUS NAMESPACE

struct monk_t;

#if SC_MONK == 1

enum monk_stance_e { STANCE_DRUNKEN_OX=1, STANCE_FIERCE_TIGER, STANCE_HEAL=4 };

struct monk_td_t : public actor_pair_t
{
  monk_td_t( player_t* target, player_t* monk ) :
    actor_pair_t( target, monk )
  {
  }
};

struct monk_t : public player_t
{
  monk_stance_e active_stance;

  // Buffs
  struct buffs_t
  {
    //buff_t* buffs_<buffname>;
  } buffs;

  // Gains
  struct gains_t
  {
    gain_t* chi;
  } gains;

  // Procs
  struct procs_t
  {
    //proc_t* procs_<procname>;
  } procs;

  // Talents
  struct talents_t
  {
    // TREE_MONK_TANK
    //talent_t* <talentname>;

    // TREE_MONK_DAMAGE

    // TREE_MONK_HEAL

  } talents;

  // Passives
  struct passives_t
  {
    // TREE_MONK_TANK
    // spell_id_t* mastery/passive spells

    // TREE_MONK_DAMAGE

    // TREE_MONK_HEAL
  } passives;

  // Glyphs
  struct glyphs_t
  {
    // Prime
    //glyph_t* <glyphname>;

    // Major

  } glyphs;

  target_specific_t<monk_td_t> target_data;

  monk_t( sim_t* sim, const std::string& name, race_e r = RACE_PANDAREN ) :
    player_t( sim, MONK, name, r ),
    buffs( buffs_t() ),
    gains( gains_t() ),
    procs( procs_t() ),
    talents( talents_t() ),
    passives( passives_t() ),
    glyphs( glyphs_t() )
  {
    target_data.init( "target_data", this );

    active_stance             = STANCE_FIERCE_TIGER;

    create_options();
  }

  // Character Definition
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      init_resources( bool force=false );
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual int       decode_set( item_t& );
  virtual resource_e primary_resource();
  virtual role_e primary_role();

  virtual monk_td_t* get_target_data( player_t* target )
  {
    monk_td_t*& td = target_data[ target ];
    if ( ! td ) td = new monk_td_t( target, this );
    return td;
  }

  // Temporary
  virtual std::string set_default_talents()
  {
    switch ( primary_tree() )
    {
    case SPEC_NONE: break;
    default: break;
    }

    return player_t::set_default_talents();
  }

  virtual std::string set_default_glyphs()
  {
    switch ( primary_tree() )
    {
    case SPEC_NONE: break;
    default: break;
    }

    return player_t::set_default_glyphs();
  }
};

// ==========================================================================
// Monk Abilities
// ==========================================================================

struct monk_melee_attack_t : public melee_attack_t
{
  int stancemask;

  monk_melee_attack_t( const std::string& n, monk_t* player,
                       const spell_data_t* s = spell_data_t::nil() ) :
    melee_attack_t( n, player, s ),
    stancemask( STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER|STANCE_HEAL )
  {
    may_crit   = true;
    may_glance = false;
  }

  monk_t* p() { return debug_cast<monk_t*>( player ); }

  monk_td_t* td( player_t* t = 0 ) { return p() -> get_target_data( t ? t : target ); }

  virtual bool   ready();
};

struct monk_spell_t : public spell_t
{
  int stancemask;

  monk_spell_t( const std::string& n, monk_t* player,
                const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( n, player, s ),
    stancemask( STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER|STANCE_HEAL )
  {
    may_crit   = true;
  }

  monk_t* p() { return debug_cast<monk_t*>( player ); }

  monk_td_t* td( player_t* t = 0 ) { return p() -> get_target_data( t ? t : target ); }

  virtual bool   ready();
};

struct monk_heal_t : public heal_t
{
  int stancemask;

  monk_heal_t( const std::string& n, monk_t* player,
               const spell_data_t* s = spell_data_t::nil() ) :
    heal_t( n, player, s ),
    stancemask( STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER|STANCE_HEAL )
  {
    may_crit   = true;
  }

  monk_t* p() { return debug_cast<monk_t*>( player ); }

  monk_td_t* td( player_t* t = 0 ) { return p() -> get_target_data( t ? t : target ); }

  virtual bool   ready();
};

// monk_melee_attack_t::ready ================================================

bool monk_melee_attack_t::ready()
{
  if ( ! melee_attack_t::ready() )
    return false;

  // Attack available in current stance?
  if ( ( stancemask & p() -> active_stance ) == 0 )
    return false;

  return true;
}

struct jab_t : public monk_melee_attack_t
{
  jab_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "jab", p, p -> find_class_spell( "Jab" ) )
  {
    parse_options( 0, options_str );
    stancemask = STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER;
  }

  virtual void execute()
  {
    monk_melee_attack_t::execute();

    player -> resource_gain( RESOURCE_CHI,  data().effectN( 2 ).base_value() , p() -> gains.chi );
  }
};

struct tiger_palm_t : public monk_melee_attack_t
{
  tiger_palm_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "tiger_palm", p, p -> find_class_spell( "Tiger Palm" ) )
  {
    parse_options( 0, options_str );
    stancemask = STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER;
  }

  virtual void target_debuff( player_t* t, dmg_e dt )
  {
    monk_melee_attack_t::target_debuff( t, dt );

    if ( t -> health_percentage() > 50.0 )
      target_dd_adder = 0;
    else
      target_dd_adder = 0;

  }
};

struct blackout_kick_t : public monk_melee_attack_t
{
  blackout_kick_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "blackout_kick", p, p -> find_class_spell( "Blackout Kick" ) )
  {
    parse_options( 0, options_str );
  }
};

struct spinning_crane_kick_tick_t : public monk_melee_attack_t
{
  spinning_crane_kick_tick_t( monk_t* p ) :
    monk_melee_attack_t( "spinning_crane_kick_tick", p )
  {
    background  = true;
    dual        = true;
    direct_tick = true;
    aoe = -1;
  }
};

struct spinning_crane_kick_t : public monk_melee_attack_t
{
  spinning_crane_kick_tick_t* spinning_crane_kick_tick;

  spinning_crane_kick_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "spinning_crane_kick", p, p -> find_class_spell( "Spinning Crane Kick" ) ),
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
    monk_melee_attack_t::init();

    spinning_crane_kick_tick -> stats = stats;
  }

  virtual void tick( dot_t* d )
  {
    if ( spinning_crane_kick_tick )
      spinning_crane_kick_tick -> execute();

    stats -> add_tick( d -> time_to_tick );
  }
};

// monk_spell_t::ready ================================================

bool monk_spell_t::ready()
{
  if ( ! spell_t::ready() )
    return false;

  // spell available in current stance?
  if ( ( stancemask & p() -> active_stance ) == 0 )
    return false;

  return true;
}

// Stance ===================================================================

struct stance_t : public monk_spell_t
{
  monk_stance_e switch_to_stance;
  std::string stance_str;

  stance_t( monk_t* p, const std::string& options_str ) :
    monk_spell_t( "stance", p ),
    switch_to_stance( STANCE_FIERCE_TIGER ), stance_str( "" )
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

    harmful = false;
    trigger_gcd = timespan_t::zero();
    cooldown -> duration = timespan_t::from_seconds( 1.0 );
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    p() -> active_stance = switch_to_stance;
  }

  virtual bool ready()
  {
    if ( p() -> active_stance == switch_to_stance )
      return false;

    return monk_spell_t::ready();
  }
};

// monk_heal_t::ready ================================================

bool monk_heal_t::ready()
{
  if ( ! heal_t::ready() )
    return false;

  // heal available in current stance?
  if ( ( stancemask & p() -> active_stance ) == 0 )
    return false;

  return true;
}

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

// monk_t::init_spells ======================================================

void monk_t::init_spells()
{
  player_t::init_spells();

  // Add Spells & Glyphs

  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P    M2P    M4P    T2P    T4P    H2P    H4P
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

  initial.distance = ( tree == MONK_MISTWEAVER ) ? 40 : 3;

  base_gcd = timespan_t::from_seconds( 1.0 ); // FIXME: assumption

  resources.base[  RESOURCE_CHI  ] = 3; // FIXME: placeholder

  base_chi_regen_per_second = 10; // FIXME: placeholder ( identical to rogue )

  if ( tree == MONK_MISTWEAVER )
    active_stance = STANCE_HEAL;
  else if ( tree == MONK_BREWMASTER )
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

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

}

// monk_t::init_gains =======================================================

void monk_t::init_gains()
{
  player_t::init_gains();

  gains.chi = get_gain( "chi" );
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
  if ( true )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s's class isn't supported yet.", name() );
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    switch ( primary_tree() )
    {
    case MONK_BREWMASTER:
    case MONK_WINDWALKER:
    case MONK_MISTWEAVER:
    default:
      // Flask
      if ( level > 85 )
        action_list_str += "/flask,type=warm_sun,precombat=1";
      else if ( level >= 80 )
        action_list_str += "/flask,type=draconic_mind,precombat=1";

      // Food
      if ( level > 85 )
      {
        action_list_str += "/food,type=great_pandaren_banquet,precombat=1";
      }
      else if ( level > 80 )
      {
        action_list_str += "/food,type=seafood_magnifique_feast,precombat=1";
      }

      action_list_str += "/snapshot_stats,precombat=1";

      action_list_str += "/tiger_palm";
      action_list_str += "/blackout_kick";
      action_list_str += "/jab";
      break;
    }
  }

  player_t::init_actions();
}

// monk_t::reset ==================================================

void monk_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_CHI ] = 0;
}

// monk_t::matching_gear_multiplier =========================================

double monk_t::matching_gear_multiplier( attribute_e attr )
{
  if ( primary_tree() == MONK_MISTWEAVER )
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

resource_e monk_t::primary_resource()
{
  // FIXME: change to healing stance
  if ( primary_tree() == MONK_MISTWEAVER )
    return RESOURCE_MANA;

  return RESOURCE_CHI;
}

// monk_t::primary_role ==================================================

role_e monk_t::primary_role()
{
  if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_HYBRID )
    return ROLE_HYBRID;

  if ( player_t::primary_role() == ROLE_TANK  )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_HEAL )
    return ROLE_HEAL;

  if ( primary_tree() == MONK_BREWMASTER )
    return ROLE_TANK;

  if ( primary_tree() == MONK_MISTWEAVER )
    return ROLE_HEAL;

  return ROLE_HYBRID;
}

#endif // SC_MONK

} // END ANONYMOUS NAMESPACE

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// class_modules::create::monk  ===================================================

player_t* class_modules::create::monk( sim_t* sim, const std::string& name, race_e r )
{
  return sc_create_class<monk_t,SC_MONK>()( "Monk", sim, name, r );
}

// player_t::monk_init ======================================================

void class_modules::init::monk( sim_t* /* sim */ )
{

}

// player_t::monk_combat_begin ==============================================

void class_modules::combat_begin::monk( sim_t* /* sim */ )
{

}

// class_modules::combat_end::monk ==============================================

void class_modules::combat_end::monk( sim_t* /* sim */ )
{

}
