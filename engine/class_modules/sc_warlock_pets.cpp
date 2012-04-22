// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "simulationcraft.hpp"
#include "sc_warlock.hpp"

#if SC_WARLOCK == 1

namespace pet_stats { // ====================================================

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
  { 81, {  345, 297,  333, 151, 212,    0,     0,         0,     0,     0,     0,   0,       0 } },
  { 80, {  314, 226,  328, 150, 209,    0,     0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

static const _stat_list_t imp_base_stats[]=
{
  //        str, agi,  sta, int, spi,     hp,  mana, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,  5026, 31607,          0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,  5026, 17628,          0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

static const _stat_list_t felguard_base_stats[]=
{
  //       str, agi,  sta, int, spi,     hp,  mana, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,  5395, 19072,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,  5395,  9109,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

static const _stat_list_t felhunter_base_stats[]=
{
  //       str, agi,  sta, int, spi,     hp,  mana, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,  5395, 19072,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,  5395,  9109,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

static const _stat_list_t succubus_base_stats[]=
{
  //       str, agi,  sta, int, spi,     hp,  mana, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,  5640, 19072,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,  4530,  9109,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

static const _stat_list_t infernal_base_stats[]=
{
  //       str, agi,  sta, int, spi,     hp,  mana, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,     0,     0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,     0,     0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

static const _stat_list_t doomguard_base_stats[]=
{
  //       str, agi,  sta, int, spi,     hp,  mana, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,     0,     0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,     0,     0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

static const _stat_list_t voidwalker_base_stats[]=
{
  //       str, agi,  sta, int, spi,     hp,  mana, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,  6131, 19072,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,  6131,  9109,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

struct _weapon_list_t
{
  int id;
  double min_dmg, max_dmg;
  timespan_t swing_time;
};

static const _weapon_list_t imp_weapon[]=
{
  { 0, 0, 0, timespan_t::zero() }
};

static const _weapon_list_t felguard_weapon[]=
{
  { 85, 937, 937, timespan_t::from_seconds( 2.0 ) },
  { 0, 0, 0, timespan_t::zero() }
};

static const _weapon_list_t felhunter_weapon[]=
{
  { 85, 937, 937, timespan_t::from_seconds( 2.0 ) },
  { 0, 0, 0, timespan_t::zero() }
};

static const _weapon_list_t succubus_weapon[]=
{
  { 85, 937, 937, timespan_t::from_seconds( 2.0 ) },
  { 0, 0, 0, timespan_t::zero() }
};

static const _weapon_list_t infernal_weapon[]=
{
  { 85, 1072.0, 1072.0, timespan_t::from_seconds( 2.0 ) }, //Rough numbers
  { 80, 924.0, 924.0, timespan_t::from_seconds( 2.0 ) }, //Rough numbers
  { 0, 0, 0, timespan_t::zero() }
};

static const _weapon_list_t doomguard_weapon[]=
{
  { 0, 0, 0, timespan_t::zero() }
};

static const _weapon_list_t voidwalker_weapon[]=
{
  { 85, 937, 937, timespan_t::from_seconds( 2.0 ) },
  { 0, 0, 0, timespan_t::zero() }
};

}
namespace warlock_pet_actions {


// ==========================================================================
// Warlock Pet Melee
// ==========================================================================

struct warlock_pet_melee_t : public melee_attack_t
{
  warlock_pet_melee_t( warlock_pet_t* p, const char* name ) :
    melee_attack_t( name, p, spell_data_t::nil(), SCHOOL_PHYSICAL )
  {
    weapon = &( p -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    may_crit    = true;
    background  = true;
    repeating   = true;
    
    if ( p -> owner -> race == RACE_ORC ) base_multiplier = 1.05;
  }

  warlock_pet_t* p() const
  { return static_cast<warlock_pet_t*>( player ); }
};

// ==========================================================================
// Warlock Pet Attack
// ==========================================================================

struct warlock_pet_melee_attack_t : public melee_attack_t
{
  warlock_pet_melee_attack_t( warlock_pet_t* p, const std::string& n, school_type_e sc = SCHOOL_NONE ) : 
    melee_attack_t( n, p, p -> find_pet_spell( n ), sc )
  {
    weapon = &( p -> main_hand_weapon );
    may_crit   = true;
    special = true;
  }

  warlock_pet_melee_attack_t( warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil(), school_type_e sc = SCHOOL_NONE ) : 
    melee_attack_t( s -> name_cstr(), p, s, sc )
  {
    weapon = &( p -> main_hand_weapon );
    may_crit   = true;
    special = true;
  }

  warlock_pet_t* p() const
  { return static_cast<warlock_pet_t*>( player ); }

  virtual bool ready()
  {
    if ( current_resource() == RESOURCE_ENERGY && player -> resources.current[ RESOURCE_ENERGY ] < 100 )
      return false;

    return melee_attack_t::ready();
  }
};


// ==========================================================================
// Warlock Pet Spell
// ==========================================================================

struct warlock_pet_spell_t : public spell_t
{

  warlock_pet_spell_t( warlock_pet_t* p, const std::string& n, school_type_e sc = SCHOOL_NONE ) : 
    spell_t( n, p, p -> find_pet_spell( n ), sc )
  {
    may_crit = true;
    crit_multiplier *= 1.33;
  }

  warlock_pet_spell_t( warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil(), school_type_e sc = SCHOOL_NONE ) : 
    spell_t( s -> name_cstr(), p, s, sc )
  {
    may_crit = true;
    crit_multiplier *= 1.33;
  }

  warlock_pet_t* p() const
  { return static_cast<warlock_pet_t*>( player ); }

  virtual bool ready()
  {
    if ( current_resource() == RESOURCE_ENERGY && player -> resources.current[ RESOURCE_ENERGY ] < 100 )
      return false;

    return spell_t::ready();
  }

};

}


namespace imp_spells {

struct firebolt_t : public warlock_pet_actions::warlock_pet_spell_t
{
  firebolt_t( imp_pet_t* p ) :
   warlock_pet_actions::warlock_pet_spell_t( p, "Firebolt" )
  {
    //FIXME: This stuff needs testing in MoP

    direct_power_mod = 0.618; // tested in-game as of 2011/05/10

    if ( p -> owner -> bugs )
      min_gcd = timespan_t::from_seconds( 1.5 );
  }

};

}

namespace felguard_spells
{

struct legion_strike_t : public warlock_pet_actions::warlock_pet_melee_attack_t
{
  legion_strike_t( felguard_pet_t* p ) :
    warlock_pet_actions::warlock_pet_melee_attack_t( p, "Legion Strike" )
  {
    aoe               = -1;
    direct_power_mod  = 0.264;
    weapon   = &( p -> main_hand_weapon );
  }
  
};

struct felstorm_tick_t : public warlock_pet_actions::warlock_pet_melee_attack_t
{
  felstorm_tick_t( felguard_pet_t* p ) :
    warlock_pet_actions::warlock_pet_melee_attack_t( p, p -> find_spell( 89753, "felstorm_tick" ) )
  {
    direct_power_mod = 0.231; // hardcoded from the tooltip
    dual        = true;
    background  = true;
    aoe         = -1;
    direct_tick = true;
  }

  virtual resource_type_e current_resource() const { return RESOURCE_MANA; }
};

struct felstorm_t : public warlock_pet_actions::warlock_pet_melee_attack_t
{
  felstorm_tick_t* felstorm_tick;

  felstorm_t( felguard_pet_t* p ) :
    warlock_pet_actions::warlock_pet_melee_attack_t( p, "Felstorm" ), felstorm_tick( 0 )
  {
    aoe       = -1;
    harmful   = false;
    tick_zero = true;

    felstorm_tick = new felstorm_tick_t( p );
    felstorm_tick -> weapon = &( p -> main_hand_weapon );
  }

  virtual void tick( dot_t* d )
  {
    felstorm_tick -> execute();

    stats -> add_tick( d -> time_to_tick );
  }
};

struct melee_t : public warlock_pet_actions::warlock_pet_melee_t
{
  melee_t( felguard_pet_t* p ) :
    warlock_pet_actions::warlock_pet_melee_t( p, "melee" )
  { }
};

}

namespace felhunter_spells
{

struct shadow_bite_t : public warlock_pet_actions::warlock_pet_spell_t
{
  shadow_bite_t( felhunter_pet_t* p ) :
    warlock_pet_actions::warlock_pet_spell_t( p, "Shadow Bite" )
  {
  }
};

}

namespace succubus_spells
{

struct lash_of_pain_t : public warlock_pet_actions::warlock_pet_spell_t
{
  lash_of_pain_t( succubus_pet_t* p ) :
    warlock_pet_actions::warlock_pet_spell_t( p, "Lash of Pain" )
  {
    if ( p -> owner -> bugs ) min_gcd = timespan_t::from_seconds( 1.5 );
  }
};

}

namespace voidwalker_spells
{

struct torment_t : public warlock_pet_actions::warlock_pet_spell_t
{
  torment_t( voidwalker_pet_t* p ) :
    warlock_pet_actions::warlock_pet_spell_t( p, "Torment" )
  { }
};

}

namespace infernal_spells
{

// Immolation Damage Spell ================================================

struct immolation_damage_t : public warlock_pet_actions::warlock_pet_spell_t
{
  immolation_damage_t( infernal_pet_t* p ) :
    warlock_pet_actions::warlock_pet_spell_t( p, p -> find_spell( 20153, "immolation_dmg" ) )
  {
    dual        = true;
    background  = true;
    aoe         = -1;
    direct_tick = true;
    may_crit    = false;
    stats = p -> get_stats( "infernal_immolation", this );
  }
};

struct infernal_immolation_t : public warlock_pet_actions::warlock_pet_spell_t
{
  immolation_damage_t* immolation_damage;

  infernal_immolation_t( infernal_pet_t* p, const std::string& options_str ) :
    warlock_pet_actions::warlock_pet_spell_t( p, p -> find_spell( 19483, "immolation" ) ), immolation_damage( 0 )
  {
    parse_options( NULL, options_str );

    callbacks    = false;
    num_ticks    = 1;
    hasted_ticks = false;
    harmful = false;
    trigger_gcd = timespan_t::from_seconds( 1.5 );

    immolation_damage = new immolation_damage_t( p );
  }

  virtual void tick( dot_t* d )
  {
    d -> current_tick = 0; // ticks indefinitely

    immolation_damage -> execute();
  }
};

}

namespace doomguard_spells
{

struct doom_bolt_t : public warlock_pet_actions::warlock_pet_spell_t
{
  doom_bolt_t( doomguard_pet_t* p ) :
    warlock_pet_actions::warlock_pet_spell_t( p, "Doom Bolt" )
  {
    /* commenting out hardcoded stuff, need to re-test in MoP
    //FIXME: Needs testing, but WoL seems to suggest it has been changed from 2.5 to 3.0 sometime after 4.1.
    base_execute_time = timespan_t::from_seconds( 3.0 );

    //Rough numbers based on report in EJ thread 2011/07/04
    direct_power_mod  = 1.36;
    base_dd_min *= 1.25;
    base_dd_max *= 1.25;
    */
    if ( p -> owner -> bugs )
    {
      ability_lag = timespan_t::from_seconds( 0.22 );
      ability_lag_stddev = timespan_t::from_seconds( 0.01 );
    }
  }
};

}

// ==========================================================================
// Warlock Pet
// ==========================================================================

double warlock_pet_t::get_attribute_base( int level, int stat_type_e, pet_type_e pet_type )
{
  double r                      = 0.0;
  const pet_stats::_stat_list_t* base_list = 0;
  const pet_stats::_stat_list_t*  pet_list = 0;


  base_list = pet_stats::pet_base_stats;

  if      ( pet_type == PET_IMP        ) pet_list = pet_stats::imp_base_stats;
  else if ( pet_type == PET_FELGUARD   ) pet_list = pet_stats::felguard_base_stats;
  else if ( pet_type == PET_FELHUNTER  ) pet_list = pet_stats::felhunter_base_stats;
  else if ( pet_type == PET_SUCCUBUS   ) pet_list = pet_stats::succubus_base_stats;
  else if ( pet_type == PET_INFERNAL   ) pet_list = pet_stats::infernal_base_stats;
  else if ( pet_type == PET_DOOMGUARD  ) pet_list = pet_stats::doomguard_base_stats;
  else if ( pet_type == PET_VOIDWALKER ) pet_list = pet_stats::voidwalker_base_stats;

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
        stats_avaiable++;
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
        stats2_avaiable++;
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

const pet_stats::_weapon_list_t* warlock_pet_t::get_weapon( pet_type_e pet_type )
{
  const pet_stats::_weapon_list_t*  weapon_list = 0;

  if      ( pet_type == PET_IMP          ) weapon_list = pet_stats::imp_weapon;
  else if ( pet_type == PET_FELGUARD     ) weapon_list = pet_stats::felguard_weapon;
  else if ( pet_type == PET_FELHUNTER    ) weapon_list = pet_stats::felhunter_weapon;
  else if ( pet_type == PET_SUCCUBUS     ) weapon_list = pet_stats::succubus_weapon;
  else if ( pet_type == PET_INFERNAL     ) weapon_list = pet_stats::infernal_weapon;
  else if ( pet_type == PET_DOOMGUARD    ) weapon_list = pet_stats::doomguard_weapon;
  else if ( pet_type == PET_VOIDWALKER   ) weapon_list = pet_stats::voidwalker_weapon;

  return weapon_list;
}

double warlock_pet_t::get_weapon_min( int level, pet_type_e pet_type )
{
  const pet_stats::_weapon_list_t*  weapon_list = get_weapon( pet_type );

  double r = 0.0;
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

double warlock_pet_t::get_weapon_max( int level, pet_type_e pet_type )
{
  const pet_stats::_weapon_list_t*  weapon_list = get_weapon( pet_type );

  double r = 0.0;
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

timespan_t warlock_pet_t::get_weapon_swing_time( int level, pet_type_e pet_type )
{
  const pet_stats::_weapon_list_t*  weapon_list = get_weapon( pet_type );

  timespan_t r = timespan_t::zero();
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

warlock_pet_t::warlock_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_type_e pt, bool guardian ) :
  pet_t( sim, owner, pet_name, pt, guardian )
{
  gains_mana_feed = get_gain( "mana_feed" );
  procs_mana_feed = get_proc( "mana_feed" );
  stats_avaiable = 0;
  stats2_avaiable = 0;
  ap_per_owner_sp = 3.5;

  main_hand_weapon.type       = WEAPON_BEAST;
  main_hand_weapon.min_dmg    = get_weapon_min( level, pet_type );
  main_hand_weapon.max_dmg    = get_weapon_max( level, pet_type );
  main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
  main_hand_weapon.swing_time = get_weapon_swing_time( level, pet_type );
  if ( main_hand_weapon.swing_time == timespan_t::zero() )
  {
    sim -> errorf( "Pet %s has swingtime == 0.\n", name() );
    assert( 0 );
  }
}

void warlock_pet_t::init_base()
{
  pet_t::init_base();

  attribute_base[ ATTR_STRENGTH  ]  = get_attribute_base( level, BASE_STAT_STRENGTH, pet_type );
  attribute_base[ ATTR_AGILITY   ]  = get_attribute_base( level, BASE_STAT_AGILITY, pet_type );
  attribute_base[ ATTR_STAMINA   ]  = get_attribute_base( level, BASE_STAT_STAMINA, pet_type );
  attribute_base[ ATTR_INTELLECT ]  = get_attribute_base( level, BASE_STAT_INTELLECT, pet_type );
  attribute_base[ ATTR_SPIRIT    ]  = get_attribute_base( level, BASE_STAT_SPIRIT, pet_type );
  resources.base[ RESOURCE_HEALTH ] = get_attribute_base( level, BASE_STAT_HEALTH, pet_type );
  resources.base[ RESOURCE_MANA ]   = get_attribute_base( level, BASE_STAT_MANA, pet_type );
  stats_initial.attack_crit_per_agility   = get_attribute_base( level, BASE_STAT_MELEE_CRIT_PER_AGI, pet_type );
  stats_initial.spell_crit_per_intellect  = get_attribute_base( level, BASE_STAT_SPELL_CRIT_PER_INT, pet_type );
  initial_dodge_per_agility         = get_attribute_base( level, BASE_STAT_DODGE_PER_AGI, pet_type );
  stats_base.spell_crit                   = get_attribute_base( level, BASE_STAT_SPELL_CRIT, pet_type );
  stats_base.attack_crit                  = get_attribute_base( level, BASE_STAT_MELEE_CRIT, pet_type );
  stats_base.mp5                          = get_attribute_base( level, BASE_STAT_MP5, pet_type );

  if ( stats_avaiable != 13 )
    sim -> errorf( "Pet %s has no general base stats avaiable on level=%.i.\n", name(), level );
  if ( stats2_avaiable != 13 )
    sim -> errorf( "Pet %s has no base stats avaiable on level=%.i.\n", name(), level );

  resources.base[ RESOURCE_ENERGY ] = 200;
  base_energy_regen_per_second = 10;

  stats_initial.attack_power_per_strength = 2.0; // tested in-game as of 2010/12/20
  stats_base.attack_power = -20; // technically, the first 20 str give 0 ap. - tested
  stamina_per_owner = 0.6496; // level invariant, tested
  intellect_per_owner = 0; // removed in cata, tested

  stats_base.attack_crit                  += 0.0328; // seems to be level invariant, untested
  stats_base.spell_crit                   += 0.0328; // seems to be level invariant, untested
  stats_initial.attack_crit_per_agility   += 0.01 / 52.0; // untested
  stats_initial.spell_crit_per_intellect  += owner -> stats_initial.spell_crit_per_intellect; // untested
  //health_per_stamina = 10.0; // untested!
  mana_per_intellect = 0; // tested - does not scale with pet int, but with owner int, at level/80 * 7.5 mana per point of owner int that exceeds owner base int
  //mp5_per_intellect  = 2.0 / 3.0; // untested!
}

void warlock_pet_t::init_resources( bool force )
{
  //bool mana_force = ( force || resource_initial[ RESOURCE_MANA ] == 0 );
  player_t::init_resources( force );
  // if ( mana_force ) resource_initial[ RESOURCE_MANA ] += ( owner -> intellect() - owner -> attribute_base[ ATTR_INTELLECT ] ) * ( level / 80.0 ) * 7.5;
  resources.current[ RESOURCE_MANA ] = resources.max[ RESOURCE_MANA ];// = resource_initial[ RESOURCE_MANA ];
}

void warlock_pet_t::schedule_ready( timespan_t delta_time, bool waiting )
{
  if ( main_hand_attack && ! main_hand_attack -> execute_event )
  {
    main_hand_attack -> schedule_execute();
  }

  pet_t::schedule_ready( delta_time, waiting );
}

double warlock_pet_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();
  h *= owner -> spell_haste;
  return h;
}

double warlock_pet_t::composite_attack_haste() const
{
  double h = player_t::composite_attack_haste();
  h *= owner -> spell_haste;
  return h;
}

double warlock_pet_t::composite_spell_power( const school_type_e school ) const
{
  double sp = 59; // FIXME: Mysterious base spell power which is not reflected in the pet pane. Needs more testing/confirmation for all pets.
  sp += owner -> composite_spell_power( school ); // Pet SP is simply exactly the same as owner SP in MoP
  return sp;
}

double warlock_pet_t::composite_attack_power() const
{
  double ap = 0;
  ap += owner -> composite_spell_power( SCHOOL_MAX ) * ap_per_owner_sp; // Appears to simply be 3.5 times owner SP in MoP, needs more testing
  return ap;
}

double warlock_pet_t::composite_attack_crit( const weapon_t* ) const
{
  double ac = owner -> composite_spell_crit(); // Seems to just use our crit directly, based on very rough numbers, needs more testing.

  return ac;
}

double warlock_pet_t::composite_spell_crit() const
{
  double sc = owner -> composite_spell_crit(); // Seems to just use our crit directly, based on very rough numbers, needs more testing.

  return sc;
}

double warlock_pet_t::composite_player_multiplier( school_type_e school, const action_t* a ) const
{
  double m = pet_t::composite_player_multiplier( school, a );

  m *= 1.0 + owner -> composite_mastery() * o() -> mastery_spells.master_demonologist -> effectN( 3 ).base_value() / 10000.0;

  if ( o() -> talents.grimoire_of_supremacy -> ok() )
    m *= 1.0 + o() -> find_spell( 115578 ) -> effectN( 1 ).percent(); // The relevant effect is not attatched to the talent spell, weirdly enough

  return m;
}

// ==========================================================================
// Warlock Main Pet
// ==========================================================================

warlock_main_pet_t::warlock_main_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_type_e pt ) :
  warlock_pet_t( sim, owner, pet_name, pt )
{}

double warlock_main_pet_t::composite_attack_expertise( const weapon_t* ) const
{
  return owner -> stats_current.spell_hit * 26.0 / 17.0;
}

double warlock_main_pet_t::composite_mp5() const
{
  double h = warlock_pet_t::composite_mp5();
  //h += mp5_per_intellect * owner -> intellect();
  return h;
}

// ==========================================================================
// Warlock Guardian Pet
// ==========================================================================

warlock_guardian_pet_t::warlock_guardian_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_type_e pt ) :
  warlock_pet_t( sim, owner, pet_name, pt, true ),
  snapshot_crit( 0 ), snapshot_haste( 0 ), snapshot_sp( 0 ), snapshot_mastery( 0 )
{}

void warlock_guardian_pet_t::summon( timespan_t duration )
{
  reset();
  warlock_pet_t::summon( duration );
  // Guardians use snapshots
  snapshot_crit = owner -> composite_spell_crit();
  snapshot_haste = owner -> spell_haste;
  snapshot_sp = floor( owner -> composite_spell_power( SCHOOL_MAX ) * owner -> composite_spell_power_multiplier() );
  snapshot_mastery = owner -> composite_mastery();
}

double warlock_guardian_pet_t::composite_attack_crit( const weapon_t* ) const
{
  return snapshot_crit;
}

double warlock_guardian_pet_t::composite_attack_expertise( const weapon_t* ) const
{
  return 0;
}

double warlock_guardian_pet_t::composite_attack_haste() const
{
  return snapshot_haste;
}

double warlock_guardian_pet_t::composite_attack_hit() const
{
  return 0;
}

double warlock_guardian_pet_t::composite_attack_power() const
{
  double ap = pet_t::composite_attack_power();
  ap += snapshot_sp * ( level / 80.0 ) * owner -> composite_spell_power_multiplier();
  return ap;
}

double warlock_guardian_pet_t::composite_spell_crit() const
{
  return snapshot_crit;
}

double warlock_guardian_pet_t::composite_spell_haste() const
{
  return snapshot_haste;
}

double warlock_guardian_pet_t::composite_spell_power( const school_type_e school ) const
{
  double sp = pet_t::composite_spell_power( school );
  sp += snapshot_sp * ( level / 80.0 ) * 0.5;
  return sp;
}

// ==========================================================================
// Pet Imp
// ==========================================================================

imp_pet_t::imp_pet_t( sim_t* sim, warlock_t* owner, const std::string& name ) :
  warlock_main_pet_t( sim, owner, name, PET_IMP )
{
  action_list_str += "/snapshot_stats";
  action_list_str += "/firebolt";
}

void imp_pet_t::init_base()
{
  warlock_main_pet_t::init_base();

  // untested !!
  mana_per_intellect = 14.28;
  //mp5_per_intellect  = 5.0 / 6.0;
  // untested !!
}

action_t* imp_pet_t::create_action( const std::string& name,
                                    const std::string& options_str )
{
  if ( name == "firebolt" ) return new imp_spells::firebolt_t( this );

  return warlock_main_pet_t::create_action( name, options_str );
}

// ==========================================================================
// Pet Felguard
// ==========================================================================

felguard_pet_t::felguard_pet_t( sim_t* sim, warlock_t* owner, const std::string& name ) :
  warlock_main_pet_t( sim, owner, name, PET_FELGUARD )
{
  action_list_str += "/snapshot_stats";
  action_list_str += "/felstorm";
  action_list_str += "/legion_strike";
  action_list_str += "/wait_until_ready";
}

void felguard_pet_t::init_base()
{
  warlock_main_pet_t::init_base();

  main_hand_attack = new felguard_spells::melee_t( this );
}

action_t* felguard_pet_t::create_action( const std::string& name,
                                         const std::string& options_str )
{
  if ( name == "legion_strike"   ) return new felguard_spells::legion_strike_t( this );
  if ( name == "felstorm"        ) return new      felguard_spells::felstorm_t( this );

  return warlock_main_pet_t::create_action( name, options_str );
}

// ==========================================================================
// Pet Felhunter
// ==========================================================================

felhunter_pet_t::felhunter_pet_t( sim_t* sim, warlock_t* owner, const std::string& name ) :
  warlock_main_pet_t( sim, owner, name, PET_FELHUNTER )
{
  action_list_str = "snapshot_stats";
  action_list_str += "/shadow_bite";
}

void felhunter_pet_t::init_base()
{
  warlock_pet_t::init_base();

  main_hand_attack = new warlock_pet_actions::warlock_pet_melee_t( this, "felhunter_melee" );
}

action_t* felhunter_pet_t::create_action( const std::string& name,
                                          const std::string& options_str )
{
  if ( name == "shadow_bite" ) return new felhunter_spells::shadow_bite_t( this );

  return warlock_main_pet_t::create_action( name, options_str );
}

// ==========================================================================
// Pet Succubus
// ==========================================================================

succubus_pet_t::succubus_pet_t( sim_t* sim, warlock_t* owner, const std::string& name ) :
  warlock_main_pet_t( sim, owner, name, PET_SUCCUBUS )
{
  action_list_str += "/snapshot_stats";
  action_list_str += "/lash_of_pain";
  ap_per_owner_sp = 1.667;
}

void succubus_pet_t::init_base()
{
  warlock_pet_t::init_base();

  main_hand_attack = new warlock_pet_actions::warlock_pet_melee_t( this, "succubus_melee" );
}

action_t* succubus_pet_t::create_action( const std::string& name,
                                         const std::string& options_str )
{
  if ( name == "lash_of_pain" ) return new succubus_spells::lash_of_pain_t( this );

  return warlock_main_pet_t::create_action( name, options_str );
}

// ==========================================================================
// Pet Voidwalker
// ==========================================================================

voidwalker_pet_t::voidwalker_pet_t( sim_t* sim, warlock_t* owner, const std::string& name ) :
  warlock_main_pet_t( sim, owner, name, PET_VOIDWALKER )
{
  action_list_str += "/snapshot_stats";
  action_list_str += "/torment";
}

void voidwalker_pet_t::init_base()
{
  warlock_main_pet_t::init_base();

  main_hand_attack = new warlock_pet_actions::warlock_pet_melee_t( this, "voidwalker_melee" );
}

action_t* voidwalker_pet_t::create_action( const std::string& name,
                                           const std::string& options_str )
{
  if ( name == "torment" ) return new voidwalker_spells::torment_t( this );

  return warlock_main_pet_t::create_action( name, options_str );
}

// ==========================================================================
// Pet Infernal
// ==========================================================================

infernal_pet_t::infernal_pet_t( sim_t* sim, warlock_t* owner ) :
  warlock_guardian_pet_t( sim, owner, "infernal", PET_INFERNAL )
{
  action_list_str += "/snapshot_stats";
  if ( level >= 50 ) action_list_str += "/immolation,if=!ticking";
}

void infernal_pet_t::init_base()
{
  warlock_guardian_pet_t::init_base();

  main_hand_attack = new warlock_pet_actions::warlock_pet_melee_t( this, "Infernal Melee" );
}

action_t* infernal_pet_t::create_action( const std::string& name,
                                         const std::string& options_str )
{
  if ( name == "immolation" ) return new infernal_spells::infernal_immolation_t( this, options_str );

  return warlock_guardian_pet_t::create_action( name, options_str );
}

// ==========================================================================
// Pet Doomguard
// ==========================================================================

doomguard_pet_t::doomguard_pet_t( sim_t* sim, warlock_t* owner ) :
  warlock_guardian_pet_t( sim, owner, "doomguard", PET_DOOMGUARD )
{ }

void doomguard_pet_t::init_base()
{
  warlock_guardian_pet_t::init_base();

  action_list_str += "/snapshot_stats";
  action_list_str += "/doom_bolt";
}

action_t* doomguard_pet_t::create_action( const std::string& name,
                                          const std::string& options_str )
{
  if ( name == "doom_bolt" ) return new doomguard_spells::doom_bolt_t( this );

  return warlock_guardian_pet_t::create_action( name, options_str );
}

#endif // SC_WARLOCK
