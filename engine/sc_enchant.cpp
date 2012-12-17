// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

static const stat_e reforge_stats[] =
{
  STAT_SPIRIT,
  STAT_DODGE_RATING,
  STAT_PARRY_RATING,
  STAT_HIT_RATING,
  STAT_CRIT_RATING,
  STAT_HASTE_RATING,
  STAT_EXPERTISE_RATING,
  STAT_MASTERY_RATING,
  STAT_NONE
};

// Weapon Stat Proc Callback ================================================

struct weapon_stat_proc_callback_t : public action_callback_t
{
  weapon_t* weapon;
  buff_t* buff;
  double PPM;
  bool all_damage;
  cooldown_t* cooldown;
  timespan_t last_trigger;

  // NOTE NOTE NOTE: A PPM value of less than zero uses the "real PPM" system
  weapon_stat_proc_callback_t( player_t* p, const std::string& name_str, weapon_t* w, buff_t* b, double ppm=0.0, timespan_t cd=timespan_t::zero(), bool all=false ) :
    action_callback_t( p ), weapon( w ), buff( b ), PPM( ppm ), all_damage( all )
  {
    cooldown = p -> get_cooldown( name_str );
    cooldown -> duration = cd;
  }

  virtual void reset()
  { last_trigger = timespan_t::from_seconds( -10 ); }

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    if ( ! all_damage && a -> proc ) return;
    if ( a -> weapon && weapon && a -> weapon != weapon ) return;

    if ( cooldown -> down() )
      return;

    // Real PPM bails out early on interval of 0
    if ( PPM < 0 && last_trigger == a -> sim -> current_time ) return;

    bool triggered = false;

    double chance = -1.0;

    if ( weapon && PPM > 0 )
      chance = weapon -> proc_chance_on_swing( PPM ); // scales with haste
    else if ( PPM > 0 )
      chance = a -> ppm_proc_chance( PPM );
    // Real PPM
    else if ( PPM < 0 )
    {
      chance = a -> real_ppm_proc_chance( std::fabs( PPM ), last_trigger );
      last_trigger = a -> sim -> current_time;
    }

    if ( chance > 0 )
    {
      triggered = buff -> trigger( 1, 0, chance );
    }
    else
      triggered = buff -> trigger();
    buff -> up();  // track uptime info

    if ( triggered && ( cooldown -> duration > timespan_t::zero() ) )
    {
      cooldown -> start();
    }
  }
};


// Weapon Discharge Proc Callback ===========================================

struct weapon_discharge_proc_callback_t : public action_callback_t
{
  std::string name_str;
  weapon_t* weapon;
  int stacks, max_stacks;
  double fixed_chance, PPM;
  cooldown_t* cooldown;
  spell_t* spell;
  proc_t* proc;
  rng_t* rng;
  timespan_t last_trigger;

  // NOTE NOTE NOTE: A PPM value of less than zero uses the "real PPM" system
  weapon_discharge_proc_callback_t( const std::string& n, player_t* p, weapon_t* w, int ms, school_e school, double dmg, double fc, double ppm=0, cooldown_t* cd = 0 ) :
    action_callback_t( p ),
    name_str( n ), weapon( w ), stacks( 0 ), max_stacks( ms ), fixed_chance( fc ), PPM( ppm ),
    last_trigger( timespan_t::from_seconds( -10 ) )
  {
    struct discharge_spell_t : public spell_t
    {
      discharge_spell_t( const char* n, player_t* p, double dmg, school_e s ) :
        spell_t( n, p, spell_data_t::nil() )
      {
        school = ( s == SCHOOL_DRAIN ) ? SCHOOL_SHADOW : s;
        trigger_gcd = timespan_t::zero();
        base_dd_min = dmg;
        base_dd_max = dmg;
        may_crit = ( s != SCHOOL_DRAIN );
        background  = true;
        proc = true;
        base_spell_power_multiplier = 0;
        init();
      }
    };

    if ( ! cd )
      cd = p -> get_cooldown( n );
    cooldown = cd;

    spell = new discharge_spell_t( name_str.c_str(), p, dmg, school );

    proc = p -> get_proc( name_str.c_str() );
    rng  = p -> get_rng ( name_str.c_str() );
  }

  virtual void reset() { stacks=0; last_trigger = timespan_t::from_seconds( -10 ); }

  virtual void deactivate() { action_callback_t::deactivate(); stacks=0; }

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    if ( a -> proc ) return;
    if ( a -> weapon && weapon && a -> weapon != weapon ) return;

    if ( cooldown -> down() )
      return;

    // Real PPM bails out early on interval of 0
    if ( PPM < 0 && last_trigger == a -> sim -> current_time ) return;

    double chance = fixed_chance;
    if ( weapon && PPM > 0 )
      chance = weapon -> proc_chance_on_swing( PPM ); // scales with haste
    else if ( PPM > 0 )
      chance = a -> ppm_proc_chance( PPM );
    else if ( PPM < 0 )
    {
      chance = a -> real_ppm_proc_chance( std::fabs( PPM ), last_trigger );
      last_trigger = a -> sim -> current_time;
    }

    if ( chance > 0 )
      if ( ! rng -> roll( chance ) )
        return;

    cooldown -> start();

    if ( ++stacks >= max_stacks )
    {
      stacks = 0;
      spell -> execute();
      proc -> occur();
    }
  }
};

// register_synapse_springs =================================================

void register_synapse_springs( item_t* item )
{
  player_t* p = item -> player;

  if ( p -> profession[ PROF_ENGINEERING ] < 425 )
  {
    item -> sim -> errorf( "Player %s attempting to use synapse springs without 425 in engineering.\n", p -> name() );
    return;
  }

  static const attribute_e attr[] = { ATTR_STRENGTH, ATTR_AGILITY, ATTR_INTELLECT };

  stat_e max_stat = STAT_INTELLECT;
  double max_value = -1;

  for ( unsigned i = 0; i < sizeof_array( attr ); ++i )
  {
    if ( p -> current.attribute[ attr[ i ] ] > max_value )
    {
      max_value = p -> current.attribute[ attr[ i ] ];
      max_stat = stat_from_attr( attr[ i ] );
    }
  }

  item -> use.name_str = "synapse_springs";
  item -> use.stat = max_stat;
  item -> use.stat_amount = 480.0;
  item -> use.duration = timespan_t::from_seconds( 10.0 );
  item -> use.cooldown = timespan_t::from_seconds( 60.0 );
}

// register_synapse_springs_2 =================================================

void register_synapse_springs_2( item_t* item )
{
  player_t* p = item -> player;

  const spell_data_t* spell1 = p -> find_spell( 126734 );
  const spell_data_t* spell2 = p -> find_spell( 96230 );

  if ( p -> profession[ PROF_ENGINEERING ] < 550 )
  {
    item -> sim -> errorf( "Player %s attempting to use synapse springs mk 2 without 500 in engineering.\n", p -> name() );
    return;
  }

  static const attribute_e attr[] = { ATTR_STRENGTH, ATTR_AGILITY, ATTR_INTELLECT };

  stat_e max_stat = STAT_INTELLECT;
  double max_value = -1;

  for ( unsigned i = 0; i < sizeof_array( attr ); ++i )
  {
    if ( p -> current.attribute[ attr[ i ] ] > max_value )
    {
      max_value = p -> current.attribute[ attr[ i ] ];
      max_stat = stat_from_attr( attr[ i ] );
    }
  }

  item -> use.name_str = "synapse_springs_2";
  item -> use.stat = max_stat;
  item -> use.stat_amount = spell1 -> effectN( 1 ).base_value();
  item -> use.duration = spell2 -> duration();
  item -> use.cooldown = spell1 -> cooldown();
}

// register_phase_fingers =================================================

void register_phase_fingers( item_t* item )
{
  player_t* p = item -> player;

  const spell_data_t* spell = p -> find_spell( 108788 );

  if ( p -> profession[ PROF_ENGINEERING ] < 500 )
  {
    item -> sim -> errorf( "Player %s attempting to use phase fingers without 500 in engineering.\n", p -> name() );
    return;
  }
  item -> use.name_str = "phase_fingers";
  item -> use.stat = STAT_DODGE_RATING;
  item -> use.stat_amount = spell -> effectN( 1 ).average( p );
  item -> use.duration = spell -> duration();
  item -> use.cooldown = spell -> cooldown();
}

// register_frag_belt =====================================================

void register_frag_belt( item_t* item )
{
  player_t* p = item -> player;

  const spell_data_t* spell = p -> find_spell( 67890 );

  if ( p -> profession[ PROF_ENGINEERING ] < 380 )
  {
    item -> sim -> errorf( "Player %s attempting to use frag belt without 380 in engineering.\n", p -> name() );
    return;
  }

  item -> use.name_str = "frag_belt";
  item -> use.school = spell -> get_school_type();
  item -> use.discharge_amount = spell -> effectN( 1 ).average( p );
  item -> use.cooldown = spell -> cooldown();
  item -> use.aoe = -1;
}

void register_avalanche( player_t* p, const std::string& mh_enchant, const std::string& oh_enchant, weapon_t* mhw, weapon_t* ohw )
{
  if ( mh_enchant == "avalanche" || oh_enchant == "avalanche" )
  {
    if ( mh_enchant == "avalanche" )
    {
      cooldown_t* cd = p -> get_cooldown( "avalanche_mh" );
      cd -> duration = timespan_t::from_seconds( 0.1 );
      action_callback_t* cb  = new weapon_discharge_proc_callback_t( "avalanche_mh", p, mhw, 1, SCHOOL_NATURE, 500, 0, 5.0/*PPM*/, cd/*CD*/ );
      p -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb  );
    }
    if ( oh_enchant == "avalanche" )
    {
      cooldown_t* cd = p -> get_cooldown( "avalanche_oh" );
      cd -> duration = timespan_t::from_seconds( 0.1 );
      action_callback_t* cb  = new weapon_discharge_proc_callback_t( "avalanche_oh", p, ohw, 1, SCHOOL_NATURE, 500, 0, 5.0/*PPM*/, cd/*CD*/ );
      p -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb  );
    }
    // Reference: http://elitistjerks.com/f79/t110302-enhsim_cataclysm/p4/#post1832162
    cooldown_t* cd = p -> get_cooldown( "avalanche_spell" );
    cd -> duration = timespan_t::from_seconds( 10.0 );
    action_callback_t* cb = new weapon_discharge_proc_callback_t( "avalanche_s", p, 0, 1, SCHOOL_NATURE, 500, 0.25/*FIXED*/, 0, cd/*CD*/ );
    p -> callbacks.register_spell_callback ( RESULT_HIT_MASK, cb );
  }
}

void register_executioner( player_t* p, const std::string& mh_enchant, const std::string& oh_enchant, weapon_t* mhw, weapon_t* ohw )
{
  if ( mh_enchant == "executioner" || oh_enchant == "executioner" )
  {
    // MH-OH trigger/refresh the same Executioner buff.  It does not stack.

    stat_buff_t* buff = stat_buff_creator_t( p, "executioner" )
                        .spell( p -> find_spell( 42976 ) )
                        .cd( timespan_t::zero() )
                        .chance( 0 )
                        .activated( false );

    if ( mh_enchant == "executioner" )
    {
      p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, "executioner", mhw,  buff, 1.0/*PPM*/ ) );
    }
    if ( oh_enchant == "executioner" )
    {
      p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, "executioner", ohw,  buff, 1.0/*PPM*/ ) );
    }
  }
}

void register_hurricane( player_t* p, const std::string& mh_enchant, const std::string& oh_enchant, weapon_t* mhw, weapon_t* ohw )
{
  if ( mh_enchant == "hurricane" || oh_enchant == "hurricane" )
  {
    stat_buff_t *mh_buff=0, *oh_buff=0;
    if ( mh_enchant == "hurricane" )
    {
      mh_buff = stat_buff_creator_t( p, "hurricane_mh" )
                .spell( p -> find_spell( 74221 ) )
                .cd( timespan_t::zero() )
                .chance( 0 )
                .activated( false );
      p -> callbacks.register_direct_damage_callback( SCHOOL_ATTACK_MASK, new weapon_stat_proc_callback_t( p, "hurricane", mhw, mh_buff, 1.0/*PPM*/, timespan_t::zero(), true/*ALL*/ ) );
      p -> callbacks.register_tick_damage_callback  ( SCHOOL_ATTACK_MASK, new weapon_stat_proc_callback_t( p, "hurricane", mhw, mh_buff, 1.0/*PPM*/, timespan_t::zero(), true/*ALL*/ ) );
    }
    if ( oh_enchant == "hurricane" )
    {
      oh_buff = stat_buff_creator_t( p, "hurricane_oh" )
                .spell( p -> find_spell( 74221 ) )
                .cd( timespan_t::zero() )
                .chance( 0 )
                .activated( false );
      p -> callbacks.register_direct_damage_callback( SCHOOL_ATTACK_MASK, new weapon_stat_proc_callback_t( p, "hurricane_oh", ohw, oh_buff, 1.0/*PPM*/, timespan_t::zero(), true /*ALL*/ ) );
      p -> callbacks.register_tick_damage_callback  ( SCHOOL_ATTACK_MASK, new weapon_stat_proc_callback_t( p, "hurricane_oh", ohw, oh_buff, 1.0/*PPM*/, timespan_t::zero(), true /*ALL*/ ) );
    }
    // Custom proc is required for spell damage procs.
    // If MH buff is up, then refresh it, else
    // IF OH buff is up, then refresh it, else
    // Trigger a new buff not associated with either MH or OH
    // This means that it is possible to have three stacks
    struct hurricane_spell_proc_callback_t : public action_callback_t
    {
      buff_t *mh_buff, *oh_buff, *s_buff;
      hurricane_spell_proc_callback_t( player_t* p, buff_t* mhb, buff_t* ohb, buff_t* sb ) :
        action_callback_t( p ), mh_buff( mhb ), oh_buff( ohb ), s_buff( sb )
      {
      }
      virtual void trigger( action_t* /* a */, void* /* call_data */ )
      {
        if ( s_buff -> cooldown -> down() ) return;
        if ( ! s_buff -> rng -> roll( 0.15 ) ) return;
        if ( mh_buff && mh_buff -> check() )
        {
          mh_buff -> trigger();
          s_buff -> cooldown -> start();
        }
        else if ( oh_buff && oh_buff -> check() )
        {
          oh_buff -> trigger();
          s_buff -> cooldown -> start();
        }
        else s_buff -> trigger();
      }
    };
    stat_buff_t* s_buff = stat_buff_creator_t( p, "hurricane_s" )
                          .spell( p -> find_spell( 74221 ) )
                          .cd( timespan_t::from_seconds( 45.0 ) )
                          .activated( false );
    p -> callbacks.register_spell_direct_damage_callback( SCHOOL_SPELL_MASK, new hurricane_spell_proc_callback_t( p, mh_buff, oh_buff, s_buff ) );
    p -> callbacks.register_spell_tick_damage_callback  ( SCHOOL_SPELL_MASK, new hurricane_spell_proc_callback_t( p, mh_buff, oh_buff, s_buff ) );
    p -> callbacks.register_direct_heal_callback        ( SCHOOL_SPELL_MASK, new hurricane_spell_proc_callback_t( p, mh_buff, oh_buff, s_buff ) );
    p -> callbacks.register_tick_heal_callback          ( SCHOOL_SPELL_MASK, new hurricane_spell_proc_callback_t( p, mh_buff, oh_buff, s_buff ) );
  }
}

void register_landslide( player_t* p, const std::string& enchant, weapon_t* w, const std::string& weapon_appendix )
{
  if ( enchant == "landslide" )
  {
    stat_buff_t* buff = stat_buff_creator_t( p, "landslide" + weapon_appendix )
                        .spell( p -> find_spell( 74245 ) )
                        .activated( false )
                        .add_stat( STAT_ATTACK_POWER, 1000 );
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, "landslide" + weapon_appendix, w, buff, 1.0/*PPM*/ ) );
  }
}

void register_mongoose( player_t* p, const std::string& enchant, weapon_t* w, const std::string& weapon_appendix )
{
  if ( enchant == "mongoose" )
  {
    p -> buffs.mongoose_mh = stat_buff_creator_t( p, "mongoose" + weapon_appendix )
                             .duration( timespan_t::from_seconds( 15 ) )
                             .activated( false )
                             .add_stat( STAT_AGILITY, 120 );
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, "mongoose" + weapon_appendix, w, p -> buffs.mongoose_mh, 1.0/*PPM*/ ) );
  }
}

void register_power_torrent( player_t* p, const std::string& enchant, const std::string& weapon_appendix )
{
  if ( enchant == "power_torrent" )
  {
    stat_buff_t* buff = stat_buff_creator_t( p, "power_torrent" + weapon_appendix )
                        .spell( p -> find_spell( 74241 ) )
                        .cd( timespan_t::from_seconds( 45 ) )
                        .chance( 0.20 )
                        .activated( false )
                        .add_stat( STAT_INTELLECT, 500 );
    weapon_stat_proc_callback_t* cb = new weapon_stat_proc_callback_t( p, "power_torrent", NULL, buff );
    p -> callbacks.register_spell_tick_damage_callback  ( SCHOOL_ALL_MASK, cb );
    p -> callbacks.register_spell_direct_damage_callback( SCHOOL_ALL_MASK, cb );
    p -> callbacks.register_tick_heal_callback          ( SCHOOL_ALL_MASK, cb );
    p -> callbacks.register_direct_heal_callback        ( SCHOOL_ALL_MASK, cb );
  }
}

static bool jade_spirit_check_func( void* d )
{
  player_t* p = static_cast<player_t*>( d );

  if ( p -> resources.max[ RESOURCE_MANA ] <= 0.0 ) return false;

  return ( p -> resources.current[ RESOURCE_MANA ] / p -> resources.max[ RESOURCE_MANA ] < 0.25 );
}

void register_jade_spirit( player_t* p, const std::string& mh_enchant, const std::string& oh_enchant )
{
  const spell_data_t* spell = p -> find_spell( 104993 );

  if ( mh_enchant == "jade_spirit" || oh_enchant == "jade_spirit" )
  {
    stat_buff_t* buff  = stat_buff_creator_t( p, "jade_spirit" )
                         .duration( spell -> duration() )
                         .cd( timespan_t::zero() )
                         .chance( p -> find_spell( 120033 ) -> proc_chance() )
                         .activated( false )
                         .add_stat( STAT_INTELLECT, spell -> effectN( 1 ).base_value() )
                         .add_stat( STAT_SPIRIT,    spell -> effectN( 2 ).base_value(), jade_spirit_check_func );

    weapon_stat_proc_callback_t* cb = new weapon_stat_proc_callback_t( p, "jade_spirit", NULL, buff, -2.0 );

    p -> callbacks.register_spell_tick_damage_callback  ( SCHOOL_ALL_MASK, cb );
    p -> callbacks.register_spell_direct_damage_callback( SCHOOL_ALL_MASK, cb );
    p -> callbacks.register_tick_heal_callback          ( SCHOOL_ALL_MASK, cb );
    p -> callbacks.register_direct_heal_callback        ( SCHOOL_ALL_MASK, cb );
  }
}


static bool dancing_steel_agi_check_func( void* d )
{
  player_t* p = static_cast<player_t*>( d );

  return ( p -> agility() >= p -> strength() );
}

static bool dancing_steel_str_check_func( void* d )
{
  player_t* p = static_cast<player_t*>( d );

  return ( p -> agility() < p -> strength() );
}

void register_dancing_steel( player_t* p, const std::string& enchant, weapon_t* w, const std::string& weapon_appendix )
{
  if ( ! util::str_compare_ci( enchant, "dancing_steel" ) )
    return;

  const spell_data_t* spell = p -> find_spell( 120032 );

  stat_buff_t* buff  = stat_buff_creator_t( p, "dancing_steel" + weapon_appendix )
                       .duration( spell -> duration() )
                       .activated( false )
                       .add_stat( STAT_STRENGTH, spell -> effectN( 1 ).base_value(), dancing_steel_str_check_func )
                       .add_stat( STAT_AGILITY,  spell -> effectN( 1 ).base_value(), dancing_steel_agi_check_func );

  weapon_stat_proc_callback_t* cb  = new weapon_stat_proc_callback_t( p, "dancing_steel" + weapon_appendix, w, buff, -2.0 );
  p -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
  p -> callbacks.register_spell_callback ( RESULT_HIT_MASK, cb );
  p -> callbacks.register_tick_callback  ( RESULT_HIT_MASK, cb );
  p -> callbacks.register_heal_callback  ( SCHOOL_ALL_MASK, cb );
}


// Windsong Proc Callback ================================================

struct windsong_callback_t : public action_callback_t
{
  stat_buff_t* haste_buff;
  stat_buff_t* crit_buff;
  stat_buff_t* mastery_buff;
  timespan_t last_trigger;

  windsong_callback_t( player_t* p, stat_buff_t* hb, stat_buff_t* cb, stat_buff_t* mb ) :
    action_callback_t( p ),
    haste_buff  ( hb ), crit_buff( cb ), mastery_buff( mb ),
    last_trigger( timespan_t::from_seconds( -10 ) )
  { }

  virtual void reset()
  {
    last_trigger = timespan_t::from_seconds( -10 );
  }

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    if ( a -> proc ) return;

    if ( last_trigger == a -> sim -> current_time )
      return;

    stat_buff_t* buff;

    int p_type = ( int ) ( a -> sim -> default_rng() -> real() * 3.0 );
    switch ( p_type )
    {
    case 0: buff = haste_buff; break;
    case 1: buff = crit_buff; break;
    case 2:
    default:
      buff = mastery_buff; break;
    }

    buff -> trigger( 1, 0, a -> real_ppm_proc_chance( 2.0, last_trigger ) ); // scales with haste

    buff -> up();  // track uptime info

    last_trigger = a -> sim -> current_time;
  }
};


void register_windsong( player_t* p, const std::string& enchant, weapon_t* /* w */, const std::string& enchant_suffix )
{
  if ( ! util::str_compare_ci( enchant, "windsong" ) )
    return;

  const spell_data_t* spell = p -> find_spell( 104509 );
  double amount = spell -> effectN( 1 ).base_value();

  stat_buff_t* haste_buff   = stat_buff_creator_t( p, "windsong_haste" + enchant_suffix )
                              .duration( spell -> duration() )
                              .activated( false )
                              .add_stat( STAT_HASTE_RATING,   amount );
  stat_buff_t* crit_buff    = stat_buff_creator_t( p, "windsong_crit" + enchant_suffix )
                              .duration( spell -> duration() )
                              .activated( false )
                              .add_stat( STAT_CRIT_RATING,    amount );
  stat_buff_t* mastery_buff = stat_buff_creator_t( p, "windsong_mastery" + enchant_suffix )
                              .duration( spell -> duration() )
                              .activated( false )
                              .add_stat( STAT_MASTERY_RATING, amount );

  windsong_callback_t* cb  = new windsong_callback_t( p, haste_buff, crit_buff, mastery_buff );
  p -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
  p -> callbacks.register_spell_callback ( RESULT_HIT_MASK, cb );
  p -> callbacks.register_tick_callback  ( RESULT_HIT_MASK, cb );
  p -> callbacks.register_heal_callback  ( SCHOOL_ALL_MASK, cb );
}

void register_rivers_song( player_t* p, const std::string& mh_enchant, const std::string& oh_enchant, weapon_t* mhw, weapon_t* ohw )
{
  if ( mh_enchant == "rivers_song" || oh_enchant == "rivers_song" )
  {
    const spell_data_t* spell = p -> find_spell( 116660 );

    stat_buff_t* buff  = stat_buff_creator_t( p, "rivers_song" )
                         .duration( spell -> duration() )
                         .activated( false )
                         .max_stack( spell -> max_stacks() )
                         .add_stat( STAT_DODGE_RATING, spell -> effectN( 1 ).base_value() );

    if ( mh_enchant == "rivers_song" )
    {
      weapon_stat_proc_callback_t* cb = new weapon_stat_proc_callback_t( p, "rivers_song", mhw, buff, -4.0 );

      p -> callbacks.register_attack_callback( RESULT_HIT_MASK | RESULT_DODGE_MASK | RESULT_PARRY_MASK, cb );
    }
    if ( oh_enchant == "rivers_song" )
    {
      weapon_stat_proc_callback_t* cb = new weapon_stat_proc_callback_t( p, "rivers_song", ohw, buff, -4.0 );

      p -> callbacks.register_attack_callback( RESULT_HIT_MASK | RESULT_DODGE_MASK | RESULT_PARRY_MASK, cb );
    }
  }
}

void register_windwalk( player_t* p, const std::string& enchant, weapon_t* w, const std::string& weapon_appendix )
{
  if ( enchant == "windwalk" )
  {
    stat_buff_t* buff = stat_buff_creator_t( p, "windwalk" + weapon_appendix )
                        .duration( timespan_t::from_seconds( 10 ) )
                        .cd( timespan_t::from_seconds( 45 ) )
                        .chance( 0.15 )
                        .activated( false )
                        .add_stat( STAT_DODGE_RATING, 600 );
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, "windwalk" + weapon_appendix, w, buff ) );
  }
}

void register_berserking( player_t* p, const std::string& enchant, weapon_t* w, const std::string& weapon_appendix )
{
  if ( enchant == "berserking" )
  {
    stat_buff_t* buff = stat_buff_creator_t( p, "berserking" + weapon_appendix )
                        .max_stack( 1 )
                        .duration( timespan_t::from_seconds( 15 ) )
                        .cd( timespan_t::zero() )
                        .chance( 0 )
                        .activated( false )
                        .add_stat( STAT_ATTACK_POWER, 400.0 );
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, "berserking" + weapon_appendix, w, buff, 1.0/*PPM*/ ) );
  }
}

void register_gnomish_xray( player_t* p, const std::string& enchant, weapon_t* w )
{
  if ( enchant == "gnomish_xray" )
  {
    //FIXME: 1.0 ppm and 40 second icd seems to roughly match in-game behavior, but we need to verify the exact mechanics
    stat_buff_t* buff = stat_buff_creator_t( p, "xray_targeting" )
                        .spell( p -> find_spell( 95712 ) )
                        .cd( timespan_t::from_seconds( 40 ) )
                        .activated( false );

    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, "xray_targeting", w, buff, 1.0/*PPM*/ ) );
  }
}

void register_lord_blastingtons_scope_of_doom( player_t* p, const std::string& enchant, weapon_t* w )
{
  if ( enchant == "lord_blastingtons_scope_of_doom" )
  {
    stat_buff_t* buff = stat_buff_creator_t( p, "lord_blastingtons_scope_of_doom" )
                        .spell( p -> find_spell( 109085 ) )
                        .activated( false );

    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, "lord_blastingtons_scope_of_doom", w, buff, 1.0/*PPM*/ ) );
  }
}

void register_mirror_scope( player_t* p, const std::string& enchant, weapon_t* w )
{
  if ( enchant == "mirror_scope" )
  {
    stat_buff_t* buff = stat_buff_creator_t( p, "mirror_scope" )
                        .spell( p -> find_spell( 109092 ) )
                        .activated( false );

    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, "mirror_scope", w, buff, 1.0/*PPM*/ ) );
  }
}

void register_elemental_force( player_t* p, const std::string& mh_enchant, const std::string& oh_enchant, weapon_t* /* mhw */, weapon_t* /* ohw */ )
{
  if ( p -> is_enemy() )
    return;

  const spell_data_t* elemental_force_spell = p -> find_spell( 116616 );

  double amount = ( elemental_force_spell -> effectN( 1 ).min( p ) + elemental_force_spell -> effectN( 1 ).max( p ) ) / 2;

  if ( mh_enchant == "elemental_force" )
  {
    action_callback_t* cb  = new weapon_discharge_proc_callback_t( "elemental_force", p, 0, 1, SCHOOL_ELEMENTAL, amount, 0, -10 /* Real PPM*/ );
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
    p -> callbacks.register_spell_callback ( RESULT_HIT_MASK, cb );
    p -> callbacks.register_tick_callback  ( RESULT_HIT_MASK, cb );
  }

  if ( oh_enchant == "elemental_force" )
  {
    action_callback_t* cb  = new weapon_discharge_proc_callback_t( "elemental_force_oh", p, 0, 1, SCHOOL_ELEMENTAL, amount, 0, -10 /* Real PPM*/ );
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
    p -> callbacks.register_spell_callback ( RESULT_HIT_MASK, cb );
    p -> callbacks.register_tick_callback  ( RESULT_HIT_MASK, cb );
    p -> callbacks.register_heal_callback  ( SCHOOL_ALL_MASK, cb );
  }
}

void register_colossus( player_t* p, const std::string& mh_enchant, const std::string& oh_enchant, weapon_t* mhw, weapon_t* ohw )
{
    if ( mh_enchant == "colossus" || oh_enchant == "colossus" )
    {
        const spell_data_t* ts = p -> find_spell( 118314 ); // trigger spell

        absorb_buff_t* buff = absorb_buff_creator_t( p, "colossus" )
                              .spell( ts -> effectN( 1 ).trigger() )
                              .activated( false )
                              .source( p -> get_stats( "colossus" ) )
                              .cd( timespan_t::from_seconds( 3.0 ) )
                              .chance( ts -> proc_chance() );
        
        if ( mh_enchant == "colossus" )
        {
            weapon_stat_proc_callback_t* cb = new weapon_stat_proc_callback_t( p, "colossus", mhw, buff, -6.0 /* Real PPM*/);
            
            p -> callbacks.register_attack_callback( RESULT_HIT_MASK | RESULT_DODGE_MASK | RESULT_PARRY_MASK, cb );
        }
        if ( oh_enchant == "colossus" )
        {
            weapon_stat_proc_callback_t* cb = new weapon_stat_proc_callback_t( p, "colossus", ohw, buff, -6.0 /* Real PPM*/);
            
            p -> callbacks.register_attack_callback( RESULT_HIT_MASK | RESULT_DODGE_MASK | RESULT_PARRY_MASK, cb );
        }
    }
}
    
} // END UNNAMED NAMESPACE

// ==========================================================================
// Enchant
// ==========================================================================

// enchant::init ============================================================

void enchant::init( player_t* p )
{
  if ( p -> is_pet() ) return;

  // Special Weapn Enchants
  std::string& mh_enchant     = p -> items[ SLOT_MAIN_HAND ].encoded_enchant_str;
  std::string& oh_enchant     = p -> items[ SLOT_OFF_HAND  ].encoded_enchant_str;

  weapon_t* mhw = &( p -> main_hand_weapon );
  weapon_t* ohw = &( p -> off_hand_weapon );

  register_avalanche( p, mh_enchant, oh_enchant, mhw, ohw );

  register_windsong( p, mh_enchant, mhw, "" );
  register_windsong( p, oh_enchant, ohw, "_oh" );

  register_elemental_force( p, mh_enchant, oh_enchant, mhw, ohw );

  register_executioner( p, mh_enchant, oh_enchant, mhw, ohw );

  register_hurricane( p, mh_enchant, oh_enchant, mhw, ohw );

  register_berserking( p, mh_enchant, mhw, "" );
  register_berserking( p, oh_enchant, ohw, "_oh" );

  register_landslide( p, mh_enchant, mhw, "" );
  register_landslide( p, oh_enchant, ohw, "_oh" );

  register_dancing_steel( p, mh_enchant, mhw, "" );
  register_dancing_steel( p, oh_enchant, ohw, "_oh" );

  register_rivers_song( p, mh_enchant, oh_enchant, mhw, ohw );

  register_colossus( p, mh_enchant, oh_enchant, mhw, ohw );

  register_mongoose( p, mh_enchant, mhw, "" );
  register_mongoose( p, oh_enchant, ohw, "_oh" );

  register_power_torrent( p, mh_enchant, "" );
  register_power_torrent( p, oh_enchant, "_oh" );

  register_jade_spirit( p, mh_enchant, oh_enchant );

  register_windwalk( p, mh_enchant, mhw, "" );
  register_windwalk( p, oh_enchant, ohw, "_oh" );

  register_gnomish_xray( p, mh_enchant, mhw );
  register_lord_blastingtons_scope_of_doom( p, mh_enchant, mhw );
  register_mirror_scope( p, mh_enchant, mhw );

  // Special Meta Gem "Enchants"
  if ( p -> meta_gem == META_THUNDERING_SKYFIRE )
  {
    //FIXME: 0.2 ppm and 40 second icd seems to roughly match in-game behavior, but we need to verify the exact mechanics
    stat_buff_t* buff = stat_buff_creator_t( p, "skyfire_swiftness" )
                        .spell( p -> find_spell( 39959 ) )
                        .cd( timespan_t::from_seconds( 40 ) )
                        .activated( false );
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, "skyfire_swiftness", mhw, buff, 0.2/*PPM*/ ) );
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, "skyfire_swiftness", ohw, buff, 0.2/*PPM*/ ) );
  }
  if ( p -> meta_gem == META_THUNDERING_SKYFLARE )
  {
    stat_buff_t* buff = stat_buff_creator_t( p, "skyflare_swiftness" )
                        .spell( p -> find_spell( 55379 ) )
                        .cd( timespan_t::from_seconds( 40 ) )
                        .activated( false );
    //FIXME: 0.2 ppm and 40 second icd seems to roughly match in-game behavior, but we need to verify the exact mechanics
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, "skyflare_swiftness", mhw, buff, 0.2/*PPM*/ ) );
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, "skyflare_swiftness", ohw, buff, 0.2/*PPM*/ ) );
  }

  // Special Item Enchants
  for ( size_t i = 0; i < p -> items.size(); i++ )
  {
    item_t& item = p -> items[ i ];

    if ( item.enchant.stat && item.enchant.school )
    {
      unique_gear::register_stat_discharge_proc( item, item.enchant );
    }
    else if ( item.enchant.stat )
    {
      unique_gear::register_stat_proc( item, item.enchant );
    }
    else if ( item.enchant.school )
    {
      unique_gear::register_discharge_proc( item, item.enchant );
    }
    else if ( item.encoded_addon_str == "synapse_springs" )
    {
      register_synapse_springs( &item );
      item.unique_addon = true;
    }
    else if ( item.encoded_addon_str == "synapse_springs_2" )
    {
      register_synapse_springs_2( &item );
      item.unique_addon = true;
    }
    else if ( item.encoded_addon_str == "synapse_springs_mark_ii" )
    {
      register_synapse_springs_2( &item );
      item.unique_addon = true;
    }
    else if ( item.encoded_addon_str == "phase_fingers" )
    {
      register_phase_fingers( &item );
      item.unique_addon = true;
    }
    else if ( item.encoded_addon_str == "frag_belt" )
    {
      register_frag_belt( &item );
      item.unique_addon = true;
    }
  }
}

// enchant::get_reforge_encoding ============================================

bool enchant::get_reforge_encoding( std::string& name,
                                    std::string& encoding,
                                    const std::string& reforge_id )
{
  name.clear();
  encoding.clear();

  if ( reforge_id.empty() || reforge_id == "0" )
    return true;

  int start = 0;
  int target = atoi( reforge_id.c_str() );
  target %= 56;
  if ( target == 0 ) target = 56;
  else if ( target <= start ) return false;

  for ( int i=0; reforge_stats[ i ] != STAT_NONE; i++ )
  {
    for ( int j=0; reforge_stats[ j ] != STAT_NONE; j++ )
    {
      if ( i == j ) continue;
      if ( ++start == target )
      {
        std::string source_stat = util::stat_type_abbrev( reforge_stats[ i ] );
        std::string target_stat = util::stat_type_abbrev( reforge_stats[ j ] );

        name += "Reforge " + source_stat + " to " + target_stat;
        encoding = source_stat + "_" + target_stat;

        return true;
      }
    }
  }

  return false;
}

// enchant::get_reforge_id ==================================================

int enchant::get_reforge_id( stat_e stat_from,
                             stat_e stat_to )
{
  int index_from;
  for ( index_from=0; reforge_stats[ index_from ] != STAT_NONE; index_from++ )
    if ( reforge_stats[ index_from ] == stat_from )
      break;

  int index_to;
  for ( index_to=0; reforge_stats[ index_to ] != STAT_NONE; index_to++ )
    if ( reforge_stats[ index_to ] == stat_to )
      break;

  int id=0;
  for ( int i=0; reforge_stats[ i ] != STAT_NONE; i++ )
  {
    for ( int j=0; reforge_stats[ j ] != STAT_NONE; j++ )
    {
      if ( i == j ) continue;
      id++;
      if ( index_from == i &&
           index_to   == j )
      {
        return id;
      }
    }
  }

  return 0;
}

// enchant::download_reforge ================================================

bool enchant::download_reforge( item_t&            item,
                                const std::string& reforge_id )
{
  item.armory_reforge_str.clear();

  if ( reforge_id.empty() || reforge_id == "0" )
    return true;

  std::string description;
  if ( get_reforge_encoding( description, item.armory_reforge_str, reforge_id ) )
  {
    util::tokenize( item.armory_reforge_str );
    return true;
  }

  return false;
}

// enchant::download_rsuffix ================================================

bool enchant::download_rsuffix( item_t&            item,
                                const std::string& rsuffix_id )
{
  item.armory_random_suffix_str = rsuffix_id;
  return true;
}
