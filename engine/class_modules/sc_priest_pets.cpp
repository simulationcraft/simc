// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "simulationcraft.hpp"
#include "sc_priest.hpp"
#include "sc_class_modules.hpp"

#if SC_PRIEST == 1

namespace priest {

namespace priest_pet_stats { // ====================================================

struct _stat_list_t
{
  int id;
  double stats[ BASE_STAT_MAX ];
};

// Base Stats, same for all pets. Depend on level
static const _stat_list_t pet_base_stats[]=
{
  //       str, agi,  sta, int, spi,   hp,  mana, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {  453, 883,  353, 159, 225,    0,     0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

static const _stat_list_t shadowfiend_base_stats[]=
{
  //        str, agi,  sta, int, spi,     hp,  mana, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,      0,     0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

static const _stat_list_t mindbender_base_stats[]=
{
  //        str, agi,  sta, int, spi,     hp,  mana, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,      0,     0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

static const _stat_list_t none_base_stats[]=
{
  //        str, agi,  sta, int, spi,     hp,  mana, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,      0,     0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};


struct _weapon_list_t
{
  int id;
  double min_dmg, max_dmg;
  double direct_power_mod;
  timespan_t swing_time;
};

static const _weapon_list_t shadowfiend_weapon[]=
{
  { 85, 360, 433,  0.5114705, timespan_t::from_seconds( 1.5 ) },        // direct_power_mod = 0.0060173 * level
  {  0,   0,   0,        0.0, timespan_t::zero() }
};

static const _weapon_list_t mindbender_weapon[]=
{
  { 85, 360, 433, 1.16349445, timespan_t::from_seconds( 1.5 ) },        // direct_power_mod = 0.01368817 * level
  {  0,   0,   0,        0.0, timespan_t::zero() }
};

static const _weapon_list_t none_weapon[]=
{
  { 85,   0,   0,        0.0, timespan_t::from_seconds( 1.5 ) },   
  {  0,   0,   0,        0.0, timespan_t::zero() }
};


double get_attribute_base( int level, int stat_type_e, pet_type_e pet_type, int& stats_available, int& stats2_available )
{
  double r = 0.0;
  const priest_pet_stats::_stat_list_t* base_list = 0;
  const priest_pet_stats::_stat_list_t*  pet_list = 0;


  base_list = priest_pet_stats::pet_base_stats;

  if      ( pet_type == PET_SHADOWFIEND ) pet_list = priest_pet_stats::shadowfiend_base_stats;
  else if ( pet_type == PET_MINDBENDER  ) pet_list = priest_pet_stats::mindbender_base_stats;
  else if ( pet_type == PET_NONE        ) pet_list = priest_pet_stats::none_base_stats;

  if ( stat_type_e < 0 || stat_type_e >= BASE_STAT_MAX )
  {
    return 0.0;
  }

  if ( base_list )
  {
    for ( int i = 0; base_list[ i ].id != 0 ; i++ )
    {
      if ( level == base_list[ i ].id )
      {
        r += base_list[ i ].stats[ stat_type_e ];
        stats_available++;
        break;
      }
      if ( level > base_list[ i ].id )
      {
        r += base_list[ i ].stats[ stat_type_e ];
        break;
      }
    }
  }

  if ( pet_list )
  {
    for ( int i = 0; pet_list[ i ].id != 0 ; i++ )
    {
      if ( level == pet_list[ i ].id )
      {
        r += pet_list[ i ].stats[ stat_type_e ];
        stats2_available++;
        break;
      }
      if ( level > pet_list[ i ].id )
      {
        r += pet_list[ i ].stats[ stat_type_e ];
        break;
      }
    }
  }

  return r;
}

const _weapon_list_t* get_weapon( pet_type_e pet_type )
{
  const _weapon_list_t*  weapon_list = 0;

  if      ( pet_type == PET_SHADOWFIEND ) weapon_list = priest_pet_stats::shadowfiend_weapon;
  else if ( pet_type == PET_MINDBENDER  ) weapon_list = priest_pet_stats::mindbender_weapon;
  else if ( pet_type == PET_NONE        ) weapon_list = priest_pet_stats::none_weapon;

  return weapon_list;
}

double get_weapon_min( int level, pet_type_e pet_type )
{
  const _weapon_list_t*  weapon_list = get_weapon( pet_type );

  double r = 0.0;
  
  if ( ! weapon_list )
    return 0.0;

  for ( int i = 0; weapon_list[ i ].id != 0 ; i++ )
  {
    if ( level == weapon_list[ i ].id )
    {
      r += weapon_list[ i ].min_dmg;
      break;
    }
    if ( level > weapon_list[ i ].id )
    {
      r += weapon_list[ i ].min_dmg;
      break;
    }
  }
  return r;
}

double get_weapon_max( int level, pet_type_e pet_type )
{
  const _weapon_list_t*  weapon_list = get_weapon( pet_type );

  double r = 0.0;
  
  if ( ! weapon_list )
    return 0.0;

  for ( int i = 0; weapon_list[ i ].id != 0 ; i++ )
  {
    if ( level == weapon_list[ i ].id )
    {
      r += weapon_list[ i ].max_dmg;
      break;
    }
    if ( level > weapon_list[ i ].id )
    {
      r += weapon_list[ i ].max_dmg;
      break;
    }
  }
  return r;
}

double get_weapon_direct_power_mod( int level, pet_type_e pet_type )
{
  const _weapon_list_t*  weapon_list = get_weapon( pet_type );

  double r = 0.0;

  if ( ! weapon_list )
    return 0.0;

  for ( int i = 0; weapon_list[ i ].id != 0 ; i++ )
  {
    if ( level == weapon_list[ i ].id )
    {
      r += weapon_list[ i ].direct_power_mod;
      break;
    }
    if ( level > weapon_list[ i ].id )
    {
      r += weapon_list[ i ].direct_power_mod;
      break;
    }
  }
  return r;
}

timespan_t get_weapon_swing_time( int level, pet_type_e pet_type )
{
  const priest_pet_stats::_weapon_list_t*  weapon_list = get_weapon( pet_type );

  timespan_t r = timespan_t::zero();
  
  if ( ! weapon_list )
    return timespan_t::from_seconds( 1.5 );

  for ( int i = 0; weapon_list[ i ].id != 0 ; i++ )
  {
    if ( level == weapon_list[ i ].id )
    {
      r += weapon_list[ i ].swing_time;
      break;
    }
    if ( level > weapon_list[ i ].id )
    {
      r += weapon_list[ i ].swing_time;
      break;
    }
  }
  if ( r == timespan_t::zero() )
    r = timespan_t::from_seconds( 1.0 ); // set swing-time to 1.00 if there is no weapon
  return r;
}

}
namespace priest_pet_actions {


// ==========================================================================
// Priest Pet Melee
// ==========================================================================

struct priest_pet_melee_t : public melee_attack_t
{
  mutable bool first_swing;

  priest_pet_melee_t( priest_pet_t* p, const char* name ) :
    melee_attack_t( name, p, spell_data_t::nil() ),
    first_swing( true )
  {
    school = SCHOOL_SHADOW;
    weapon = &( p -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    may_crit    = true;
    background  = true;
    repeating   = true;
  }

  virtual void reset()
  {
    melee_attack_t::reset();
    first_swing = true;
  }

  priest_pet_t* p() const
  { return static_cast<priest_pet_t*>( player ); }

  virtual timespan_t execute_time() const
  {
    if ( first_swing )
    {
      first_swing = false;
      return timespan_t::from_seconds( 0.0 );
    }
    return melee_attack_t::execute_time();
  }
};

// ==========================================================================
// Priest Pet Spell
// ==========================================================================

struct priest_pet_spell_t : public spell_t
{

  priest_pet_spell_t( priest_pet_t* p, const std::string& n ) :
    spell_t( n, p, p -> find_pet_spell( n ) )
  {
    may_crit = true;
  }

  priest_pet_spell_t( const std::string& token, priest_pet_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( token, p, s )
  {
    may_crit = true;
  }

  priest_pet_t* p() const
  { return static_cast<priest_pet_t*>( player ); }
};

}


namespace fiend_spells {

struct shadowcrawl_t : public priest_pet_actions::priest_pet_spell_t
{
  shadowcrawl_t( base_fiend_pet_t* p ) :
    priest_pet_actions::priest_pet_spell_t( p, "Shadowcrawl" )
  {
    may_miss  = false;
    harmful   = false;
    stateless = true;
  }

  base_fiend_pet_t* p() const
  { return static_cast<base_fiend_pet_t*>( player ); }

  virtual void execute()
  {
    spell_t::execute();

    p() -> buffs.shadowcrawl -> trigger();
  }
};

struct melee_t : public priest_pet_actions::priest_pet_melee_t
{
  melee_t( base_fiend_pet_t* p ) :
    priest_pet_actions::priest_pet_melee_t( p, "melee" )
  {
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;
    base_dd_min       = weapon -> min_dmg;
    base_dd_max       = weapon -> max_dmg;
    direct_power_mod  = p -> direct_power_mod;
    stateless = true;
  }

  base_fiend_pet_t* p() const
  { return static_cast<base_fiend_pet_t*>( player ); }

  virtual void execute()
  {
    priest_pet_actions::priest_pet_melee_t::execute();
  }


  virtual double action_multiplier() const
  {
    double am = priest_pet_actions::priest_pet_melee_t::action_multiplier();

    am *= 1.0 + p() -> buffs.shadowcrawl -> up() * p() -> shadowcrawl -> effectN( 2 ).percent();

    return am;
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_pet_actions::priest_pet_melee_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> o() -> resource_gain( RESOURCE_MANA, p() -> o() -> resources.max[ RESOURCE_MANA ] *
                                   p() -> mana_leech -> effectN( 1 ).percent(),
                                   p() -> gains.fiend );
    }
  }
};

}

namespace lightwell_spells {
struct lightwell_renew_t : public heal_t
  {
    lightwell_renew_t( lightwell_pet_t* player ) :
      heal_t( "lightwell_renew", player, player -> find_spell( 7001 ) )
    {
      may_crit = false;
      tick_may_crit = true;
      stateless = true;

      tick_power_mod = 0.308;
    }

    lightwell_pet_t* p()
    { return static_cast<lightwell_pet_t*>( player ); }

    virtual void execute()
    {
      p() -> charges--;

      target = find_lowest_player();

      heal_t::execute();
    }

    virtual void last_tick( dot_t* d )
    {
      heal_t::last_tick( d );

      if ( p() -> charges <= 0 )
        p() -> dismiss();
    }

    virtual bool ready()
    {
      if ( p() -> charges <= 0 )
        return false;
      return heal_t::ready();
    }
  };
} // END lightwell_spells NAMESPACE

// ==========================================================================
// Priest Pet
// ==========================================================================


priest_pet_t::priest_pet_t( sim_t* sim, priest_t* owner, const std::string& pet_name, pet_type_e pt, bool guardian ) :
  pet_t( sim, owner, pet_name, pt, guardian ),
  ap_per_owner_sp( 1.0 ),
  direct_power_mod( priest_pet_stats::get_weapon_direct_power_mod( level, pt ) )
{
  position                    = POSITION_BACK;
  initial.distance            = 3;
  main_hand_weapon.type       = WEAPON_BEAST;
  main_hand_weapon.min_dmg    = priest_pet_stats::get_weapon_min( level, pet_type );
  main_hand_weapon.max_dmg    = priest_pet_stats::get_weapon_max( level, pet_type );
  main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
  main_hand_weapon.swing_time = priest_pet_stats::get_weapon_swing_time( level, pet_type );
  if ( main_hand_weapon.swing_time == timespan_t::zero() )
  {
    sim -> errorf( "Pet %s has swingtime == 0.\n", name() );
    assert( 0 );
  }
}

void priest_pet_t::init_base()
{
  pet_t::init_base();

  {
    using namespace priest_pet_stats;
    int stats_available = 0, stats2_available = 0;
    base.attribute[ ATTR_STRENGTH  ]  = get_attribute_base( level, BASE_STAT_STRENGTH, pet_type, stats_available, stats2_available );
    base.attribute[ ATTR_AGILITY   ]  = get_attribute_base( level, BASE_STAT_AGILITY, pet_type, stats_available, stats2_available );
    base.attribute[ ATTR_STAMINA   ]  = get_attribute_base( level, BASE_STAT_STAMINA, pet_type, stats_available, stats2_available );
    base.attribute[ ATTR_INTELLECT ]  = get_attribute_base( level, BASE_STAT_INTELLECT, pet_type, stats_available, stats2_available );
    base.attribute[ ATTR_SPIRIT    ]  = get_attribute_base( level, BASE_STAT_SPIRIT, pet_type, stats_available, stats2_available );
    resources.base[ RESOURCE_HEALTH ] = get_attribute_base( level, BASE_STAT_HEALTH, pet_type, stats_available, stats2_available );
    resources.base[ RESOURCE_MANA ]   = get_attribute_base( level, BASE_STAT_MANA, pet_type, stats_available, stats2_available );
    initial.attack_crit_per_agility   = get_attribute_base( level, BASE_STAT_MELEE_CRIT_PER_AGI, pet_type, stats_available, stats2_available );
    initial.spell_crit_per_intellect  = get_attribute_base( level, BASE_STAT_SPELL_CRIT_PER_INT, pet_type, stats_available, stats2_available );
    initial.dodge_per_agility         = get_attribute_base( level, BASE_STAT_DODGE_PER_AGI, pet_type, stats_available, stats2_available );
    base.spell_crit                   = get_attribute_base( level, BASE_STAT_SPELL_CRIT, pet_type, stats_available, stats2_available );
    base.attack_crit                  = get_attribute_base( level, BASE_STAT_MELEE_CRIT, pet_type, stats_available, stats2_available );
    base.mp5                          = get_attribute_base( level, BASE_STAT_MP5, pet_type, stats_available, stats2_available );

    if ( stats_available != 13 )
      sim -> errorf( "Pet %s has no general base stats avaiable on level=%.i.\n", name(), level );
    if ( stats2_available != 13 )
      sim -> errorf( "Pet %s has no base stats avaiable on level=%.i.\n", name(), level );
  }

  resources.base[ RESOURCE_MANA ]   = o() -> resources.max[ RESOURCE_MANA ];
  initial.attack_power_per_strength = 2.0; // tested in-game as of 2010/12/20
  base.attack_power = -20; // technically, the first 20 str give 0 ap. - tested
  stamina_per_owner = 0.6496; // level invariant, tested
  intellect_per_owner = 0; // removed in cata, tested

  initial.attack_crit_per_agility   += 0.01 / 52.0; // untested
  initial.spell_crit_per_intellect  += owner -> initial.spell_crit_per_intellect; // untested
  //health_per_stamina = 10.0; // untested!
  //mana_per_intellect = 0; // tested - does not scale with pet int, but with owner int, at level/80 * 7.5 mana per point of owner int that exceeds owner base int
  //mp5_per_intellect  = 2.0 / 3.0; // untested!
}

void priest_pet_t::schedule_ready( timespan_t delta_time, bool waiting )
{
  if ( main_hand_attack && ! main_hand_attack -> execute_event )
  {
    main_hand_attack -> schedule_execute();
  }

  pet_t::schedule_ready( delta_time, waiting );
}

double priest_pet_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();
  h *= owner -> spell_haste;
  return h;
}

double priest_pet_t::composite_attack_haste() const
{
  double h = player_t::composite_attack_haste();
  h *= owner -> spell_haste;
  return h;
}

double priest_pet_t::composite_spell_power( const school_type_e school ) const
{
  double sp = 0;
  sp += owner -> composite_spell_power( school );
  return sp;
}

double priest_pet_t::composite_attack_power() const
{
  double ap = 0;
  ap += owner -> composite_spell_power( SCHOOL_MAX ) * ap_per_owner_sp;
  return ap;
}

double priest_pet_t::composite_attack_crit( const weapon_t* ) const
{
  double ac = owner -> composite_spell_crit(); // Seems to just use our crit directly, based on very rough numbers, needs more testing.

  return ac;
}

double priest_pet_t::composite_spell_crit() const
{
  double sc = owner -> composite_spell_crit(); // Seems to just use our crit directly, based on very rough numbers, needs more testing.

  return sc;
}

// ==========================================================================
// Priest Guardian Pet
// ==========================================================================

priest_guardian_pet_t::priest_guardian_pet_t( sim_t* sim, priest_t* owner, const std::string& pet_name, pet_type_e pt ) :
  priest_pet_t( sim, owner, pet_name, pt, true )
{}

void priest_guardian_pet_t::summon( timespan_t duration )
{
  reset();
  priest_pet_t::summon( duration );
}

// ==========================================================================
// Pet Shadowfiend/Mindbender Base
// ==========================================================================

base_fiend_pet_t::base_fiend_pet_t( sim_t* sim, priest_t* owner, pet_type_e pt, const std::string& name ) :
  priest_guardian_pet_t( sim, owner, name, pt ),
  buffs( buffs_t() ),
  shadowcrawl( spell_data_t::nil() ), mana_leech( spell_data_t::nil() ),
  shadowcrawl_action( 0 )
{
  action_list_str += "/snapshot_stats";
  action_list_str += "/shadowcrawl";
  action_list_str += "/wait_for_shadowcrawl";
}

void base_fiend_pet_t::init_base()
{
  priest_guardian_pet_t::init_base();

  main_hand_attack = new fiend_spells::melee_t( this );
}

action_t* base_fiend_pet_t::create_action( const std::string& name,
                                           const std::string& options_str )
{
  if ( name == "shadowcrawl" )          { shadowcrawl_action = new fiend_spells::shadowcrawl_t( this ); return shadowcrawl_action; }
  if ( name == "wait_for_shadowcrawl" ) return new wait_for_cooldown_t( this, "shadowcrawl" );

  return priest_guardian_pet_t::create_action( name, options_str );
}

void base_fiend_pet_t::init_spells()
{
  priest_guardian_pet_t::init_spells();

  shadowcrawl = find_pet_spell( "Shadowcrawl" );
}

void base_fiend_pet_t::init_buffs()
{
  priest_guardian_pet_t::init_buffs();

  buffs.shadowcrawl = buff_creator_t( this, "shadowcrawl", shadowcrawl );
}

void base_fiend_pet_t::init_gains()
{
  priest_guardian_pet_t::init_gains();

  if      ( pet_type == PET_SHADOWFIEND )
    gains.fiend = o() -> gains.shadowfiend;
  else if ( pet_type == PET_MINDBENDER  )
    gains.fiend = o() -> gains.mindbender;
  else
    gains.fiend = get_gain( "basefiend" );
}

void base_fiend_pet_t::init_resources( bool force )
{
  priest_guardian_pet_t::init_resources( force );

  resources.initial[ RESOURCE_HEALTH ] = owner -> resources.max[ RESOURCE_HEALTH ] * 0.3;
  resources.initial[ RESOURCE_MANA   ] = owner -> resources.max[ RESOURCE_MANA   ];
  resources.current = resources.max = resources.initial;
}

void base_fiend_pet_t::summon( timespan_t duration )
{
  dismiss();

  priest_guardian_pet_t::summon( duration );

  if ( shadowcrawl_action )
  {
    // Ensure that it gets used after the first melee strike. In the combat logs that happen at the same time, but the melee comes first.
    shadowcrawl_action -> cooldown -> ready = sim -> current_time + timespan_t::from_seconds( 0.001 );
  }
}

// ==========================================================================
// Pet Shadowfiend
// ==========================================================================

shadowfiend_pet_t::shadowfiend_pet_t( sim_t* sim, priest_t* owner, const std::string& name ) :
  base_fiend_pet_t( sim, owner, PET_SHADOWFIEND, name )
{
}

void shadowfiend_pet_t::init_spells()
{
  base_fiend_pet_t::init_spells();

  mana_leech  = find_spell( 34650, "mana_leech" );
}

// ==========================================================================
// Pet Mindbender
// ==========================================================================

mindbender_pet_t::mindbender_pet_t( sim_t* sim, priest_t* owner, const std::string& name ) :
  base_fiend_pet_t( sim, owner, PET_MINDBENDER, name )
{
}

void mindbender_pet_t::init_spells()
{
  base_fiend_pet_t::init_spells();

  mana_leech  = find_spell( 123051, "mana_leech" );
}


// ==========================================================================
// Pet Lightwell
// ==========================================================================


lightwell_pet_t::lightwell_pet_t( sim_t* sim, priest_t* p ) :
  priest_pet_t( sim, p, "lightwell", PET_NONE, true ),
  charges( 0 )
{
  role = ROLE_HEAL;

  action_list_str  = "/snapshot_stats";
  action_list_str += "/lightwell_renew";
  action_list_str += "/wait,sec=cooldown.lightwell_renew.remains";
}

action_t* lightwell_pet_t::create_action( const std::string& name,
                                 const std::string& options_str )
{
  if ( name == "lightwell_renew" ) return new lightwell_spells::lightwell_renew_t( this );

  return priest_pet_t::create_action( name, options_str );
}

void lightwell_pet_t::summon( timespan_t duration )
{
  spell_haste = o() -> spell_haste;
  spell_power[ SCHOOL_HOLY ] = o() -> composite_spell_power( SCHOOL_HOLY ) * o() -> composite_spell_power_multiplier();

  charges = 10 + o() -> glyphs.lightwell -> effectN( 1 ).base_value();

  priest_pet_t::summon( duration );
}

} // END priest NAMESPACE

#endif // SC_PRIEST

