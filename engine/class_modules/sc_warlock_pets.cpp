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
const _stat_list_t pet_base_stats[]=
{
  // str, agi,  sta, int, spi,   hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {  453, 883,  353, 159, 225,    0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 81, {  345, 297,  333, 151, 212,    0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {  314, 226,  328, 150, 209,    0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

const _stat_list_t imp_base_stats[]=
{
  // str, agi,  sta, int, spi,     hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,  5026, 31607,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,  5026, 17628,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

const _stat_list_t felguard_base_stats[]=
{
  // str, agi,  sta, int, spi,     hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,  5395, 19072,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,  5395,  9109,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

const _stat_list_t felhunter_base_stats[]=
{
  // str, agi,  sta, int, spi,     hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,  5395, 19072,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,  5395,  9109,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

const _stat_list_t succubus_base_stats[]=
{
  // str, agi,  sta, int, spi,     hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,  5640, 19072,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,  4530,  9109,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

const _stat_list_t infernal_base_stats[]=
{
  // str, agi,  sta, int, spi,     hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,     0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,     0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

const _stat_list_t doomguard_base_stats[]=
{
  // str, agi,  sta, int, spi,     hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,     0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,     0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

const _stat_list_t ebon_imp_base_stats[]=
{
  // str, agi,  sta, int, spi,     hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,     0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,     0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

const _stat_list_t voidwalker_base_stats[]=
{
  // str, agi,  sta, int, spi,     hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,  6131, 19072,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,  6131,  9109,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

struct _weapon_list_t
{
  int id;
  double min_dmg, max_dmg;
  timespan_t swing_time;
};

const _weapon_list_t imp_weapon[]=
{
  { 81, 116.7, 176.7, timespan_t::from_seconds( 2.0 ) },
  { 0, 0, 0, timespan_t::zero }
};

const _weapon_list_t felguard_weapon[]=
{
  { 85, 926.3, 926.3, timespan_t::from_seconds( 2.0 ) },
  { 81, 848.7, 848.7, timespan_t::from_seconds( 2.0 ) },
  { 80, 824.6, 824.6, timespan_t::from_seconds( 2.0 ) },
  { 0, 0, 0, timespan_t::zero }
};

const _weapon_list_t felhunter_weapon[]=
{
  { 85, 926.3, 926.3, timespan_t::from_seconds( 2.0 ) },
  { 81, 678.4, 1010.4, timespan_t::from_seconds( 2.0 ) },
  { 80, 824.6, 824.6, timespan_t::from_seconds( 2.0 ) },
  { 0, 0, 0, timespan_t::zero }
};

const _weapon_list_t succubus_weapon[]=
{
  { 85, 926.3, 926.3, timespan_t::from_seconds( 2.0 ) },
  { 81, 848.7, 848.7, timespan_t::from_seconds( 2.0 ) },
  { 80, 824.6, 824.6, timespan_t::from_seconds( 2.0 ) },
  { 0, 0, 0, timespan_t::zero }
};

const _weapon_list_t infernal_weapon[]=
{
  { 85, 1072.0, 1072.0, timespan_t::from_seconds( 2.0 ) }, //Rough numbers
  { 80, 924.0, 924.0, timespan_t::from_seconds( 2.0 ) }, //Rough numbers
  { 0, 0, 0, timespan_t::zero }
};

const _weapon_list_t doomguard_weapon[]=
{
  { 0, 0, 0, timespan_t::zero }
};

const _weapon_list_t ebon_imp_weapon[]=
{
  { 85, 1110.0, 1110.0, timespan_t::from_seconds( 2.0 ) }, //Rough numbers
  { 80, 956.0, 956.0, timespan_t::from_seconds( 2.0 ) }, //Rough numbers
  { 0, 0, 0, timespan_t::zero }
};

const _weapon_list_t voidwalker_weapon[]=
{
  { 85, 926.3, 926.3, timespan_t::from_seconds( 2.0 ) },
  { 81, 848.7, 848.7, timespan_t::from_seconds( 2.0 ) },
  { 80, 824.6, 824.6, timespan_t::from_seconds( 2.0 ) },
  { 0, 0, 0, timespan_t::zero }
};

}
namespace warlock_pet_actions {


// ==========================================================================
// Warlock Pet Melee
// ==========================================================================

struct warlock_pet_melee_t : public attack_t
{
  warlock_pet_melee_t( warlock_pet_t* p, const char* name ) :
    attack_t( name, p, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_NONE, false )
  {
    weapon = &( p -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    may_crit    = true;
    background  = true;
    repeating   = true;

    base_multiplier *= p -> damage_modifier;
  }

  warlock_pet_t* p() const
  { return static_cast<warlock_pet_t*>( player ); }
};

// ==========================================================================
// Warlock Pet Attack
// ==========================================================================

struct warlock_pet_attack_t : public attack_t
{
  warlock_pet_attack_t( const char* n, warlock_pet_t* p, resource_type_e r=RESOURCE_MANA, school_type_e s=SCHOOL_PHYSICAL ) :
    attack_t( n, p, r, s, TREE_NONE, true )
  {
    weapon = &( p -> main_hand_weapon );
    may_crit   = true;
    special = true;
  }

  warlock_pet_attack_t( const char* n, warlock_pet_t* player, const char* sname, talent_tree_type_e t = TREE_NONE ) :
    attack_t( n, sname, player, t, true )
  {
    may_crit   = true;
    special = true;
  }

  warlock_pet_attack_t( const char* n, uint32_t id, warlock_pet_t* player, talent_tree_type_e t = TREE_NONE ) :
    attack_t( n, id, player, t, true )
  {
    may_crit   = true;
    special = true;
  }

  warlock_pet_t* p() const
  { return static_cast<warlock_pet_t*>( player ); }

  virtual void player_buff()
  {

    attack_t::player_buff();

    if ( p() -> o() -> buffs.hand_of_guldan -> up() )
    {
      player_crit += 0.10;
    }
  }
};


// ==========================================================================
// Warlock Pet Spell
// ==========================================================================

struct warlock_pet_spell_t : public spell_t
{

  warlock_pet_spell_t( const char* n, warlock_pet_t* p, resource_type_e r=RESOURCE_MANA, school_type_e s=SCHOOL_SHADOW ) :
    spell_t( n, p, r, s )
  {
    may_crit = true;
    crit_multiplier *= 1.33;
  }

  warlock_pet_spell_t( const spell_id_t& s, talent_tree_type_e t = TREE_NONE ) :
    spell_t( s, t )
  {
    may_crit = true;
    crit_multiplier *= 1.33;
  }

  warlock_pet_spell_t( const char* n, warlock_pet_t* p, const char* sname, talent_tree_type_e t = TREE_NONE ) :
    spell_t( n, sname, p, t )
  {
    may_crit = true;
    crit_multiplier *= 1.33;
  }

  warlock_pet_spell_t( const char* n, uint32_t id, warlock_pet_t* p, talent_tree_type_e t = TREE_NONE ) :
    spell_t( n, id, p, t )
  {
    may_crit = true;
    crit_multiplier *= 1.33;
  }

  warlock_pet_t* p() const
  { return static_cast<warlock_pet_t*>( player ); }

  virtual void player_buff()
  {
    spell_t::player_buff();

    if ( p() -> o() -> buffs.hand_of_guldan -> up() )
    {
      player_crit += 0.10;
    }
  }
};

}
namespace imp_spells {

struct firebolt_t : public warlock_pet_actions::warlock_pet_spell_t
{
  firebolt_t( imp_pet_t* p ) :
    warlock_pet_actions::warlock_pet_spell_t( "firebolt", p, "Firebolt" )
  {
    warlock_t*  o = p -> owner -> cast_warlock();

    direct_power_mod = 0.618; // tested in-game as of 2011/05/10
    base_execute_time += o -> talent_dark_arts -> effect1().time_value();
    if ( o -> bugs ) min_gcd = timespan_t::from_seconds( 1.5 );
  }
// imp_pet_t::fire_bolt_t::execute ==========================================

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    warlock_pet_actions::warlock_pet_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      if ( p() -> o() -> buffs.empowered_imp -> trigger() ) p() -> o() -> procs.empowered_imp -> occur();

      warlock_t::trigger_burning_embers ( this, travel_dmg );

      warlock_t::trigger_mana_feed ( this, impact_result );
    }
  }

};
}

namespace felguard_spells
{
struct legion_strike_t : public warlock_pet_actions::warlock_pet_attack_t
{
  legion_strike_t( felguard_pet_t* p ) :
    warlock_pet_actions::warlock_pet_attack_t( "legion_strike", p, "Legion Strike" )
  {
    warlock_t*      o = p -> owner -> cast_warlock();
    aoe               = -1;
    direct_power_mod  = 0.264;
    weapon   = &( p -> main_hand_weapon );
    base_multiplier *= 1.0 + o -> talent_dark_arts -> effect2().percent();
  }

  virtual void execute()
  {
    warlock_pet_actions::warlock_pet_attack_t::execute();

    warlock_t::trigger_mana_feed ( this, result );
  }

  virtual void player_buff()
  {
    warlock_t* o = player -> cast_pet() -> owner -> cast_warlock();
    warlock_pet_actions::warlock_pet_attack_t::player_buff();

    if ( o -> race == RACE_ORC )
    {
      // Glyph is additive with orc racial
      player_multiplier /= 1.05;
      player_multiplier *= 1.05 + o -> glyphs.felguard -> base_value();
    }
    else
    {
      player_multiplier *= 1.0 + o -> glyphs.felguard -> base_value();
    }
  }
};

struct felstorm_tick_t : public warlock_pet_actions::warlock_pet_attack_t
{
  felstorm_tick_t( felguard_pet_t* p ) :
    warlock_pet_actions::warlock_pet_attack_t( "felstorm_tick", 89753, p )
  {
    direct_power_mod = 0.231; // hardcoded from the tooltip
    dual        = true;
    background  = true;
    aoe         = -1;
    direct_tick = true;
  }

  virtual resource_type_e current_resource() const { return RESOURCE_MANA; }
};

struct felstorm_t : public warlock_pet_actions::warlock_pet_attack_t
{
  felstorm_tick_t* felstorm_tick;

  felstorm_t( felguard_pet_t* p ) :
    warlock_pet_actions::warlock_pet_attack_t( "felstorm", 89751, p ), felstorm_tick( 0 )
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
    warlock_pet_actions::warlock_pet_spell_t( "shadow_bite", p, "Shadow Bite" )
  {
    warlock_t*       o = p -> owner -> cast_warlock();

    base_multiplier *= 1.0 + o -> talent_dark_arts -> effect3().percent();
    direct_power_mod = 0.614; // tested in-game as of 2010/12/20
    base_dd_min *= 2.5; // only tested at level 85, applying base damage adjustment as a percentage
    base_dd_max *= 2.5; // modifier in hopes of getting it "somewhat right" for other levels as well

    // FIXME - Naive assumption, needs testing on the 4.1 PTR!
    base_dd_min *= 2;
    base_dd_max *= 2;
    direct_power_mod *= 2;
  }

  virtual void player_buff()
  {
    warlock_pet_actions::warlock_pet_spell_t::player_buff();
    warlock_t*  o = player -> cast_pet() -> owner -> cast_warlock();
    warlock_targetdata_t* td = targetdata_t::get( o, target ) -> cast_warlock();

    player_multiplier *= 1.0 + td -> active_dots() * effect3().percent();
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    warlock_pet_actions::warlock_pet_spell_t::impact( t, impact_result, travel_dmg );
    if ( result_is_hit( impact_result ) )
      warlock_t::trigger_mana_feed ( this, impact_result );
  }
};

}

namespace succubus_spells
{
struct lash_of_pain_t : public warlock_pet_actions::warlock_pet_spell_t
{
  lash_of_pain_t( succubus_pet_t* p ) :
    warlock_pet_actions::warlock_pet_spell_t( "lash_of_pain", p, "Lash of Pain" )
  {
    warlock_t*  o     = p -> owner -> cast_warlock();

    direct_power_mod  = 0.642; // tested in-game as of 2010/12/20

    if ( o -> level == 85 )
    {
      // only tested at level 85
      base_dd_min = 283;
      base_dd_max = 314;
    }

    if ( o -> bugs ) min_gcd = timespan_t::from_seconds( 1.5 );
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    warlock_pet_actions::warlock_pet_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
      warlock_t::trigger_mana_feed ( this, impact_result );
  }

  virtual void player_buff()
  {
    warlock_t* o = player -> cast_pet() -> owner -> cast_warlock();
    warlock_pet_actions::warlock_pet_spell_t::player_buff();

    if ( o -> race == RACE_ORC )
    {
      // Glyph is additive with orc racial
      player_multiplier /= 1.05;
      player_multiplier *= 1.05 + o -> glyphs.lash_of_pain -> base_value();
    }
    else
    {
      player_multiplier *= 1.0 + o -> glyphs.lash_of_pain -> base_value();
    }
  }
};

}

namespace voidwalker_spells
{

struct torment_t : public warlock_pet_actions::warlock_pet_spell_t
{
  torment_t( voidwalker_pet_t* p ) :
    warlock_pet_actions::warlock_pet_spell_t( "torment", p, "Torment" )
  {
    direct_power_mod = 0.512;
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    warlock_pet_actions::warlock_pet_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
      warlock_t::trigger_mana_feed ( this, impact_result ); // untested
  }
};

}

namespace infernal_spells
{

// Immolation Damage Spell ================================================

struct immolation_damage_t : public warlock_pet_actions::warlock_pet_spell_t
{
  immolation_damage_t( infernal_pet_t* p ) :
    warlock_pet_actions::warlock_pet_spell_t( "immolation_dmg", 20153, p )
  {
    dual        = true;
    background  = true;
    aoe         = -1;
    direct_tick = true;
    may_crit    = false;
    stats = p -> get_stats( "infernal_immolation", this );
    direct_power_mod  = 0.4;
  }
};

struct infernal_immolation_t : public warlock_pet_actions::warlock_pet_spell_t
{
  immolation_damage_t* immolation_damage;

  infernal_immolation_t( infernal_pet_t* p, const std::string& options_str ) :
    warlock_pet_actions::warlock_pet_spell_t( "infernal_immolation", 19483, p ), immolation_damage( 0 )
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
    warlock_pet_actions::warlock_pet_spell_t( "doombolt", p, "Doom Bolt" )
  {
    //FIXME: Needs testing, but WoL seems to suggest it has been changed from 2.5 to 3.0 sometime after 4.1.
    base_execute_time = timespan_t::from_seconds( 3.0 );

    //Rough numbers based on report in EJ thread 2011/07/04
    direct_power_mod  = 1.36;
    base_dd_min *= 1.25;
    base_dd_max *= 1.25;

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

double warlock_pet_t::get_attribute_base( const int level, const int stat_type_e, const pet_type_e pet_type )
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
  else if ( pet_type == PET_EBON_IMP   ) pet_list = pet_stats::ebon_imp_base_stats;
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
  else if ( pet_type == PET_EBON_IMP     ) weapon_list = pet_stats::ebon_imp_weapon;
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

  timespan_t r = timespan_t::zero;
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
  if ( r == timespan_t::zero )
    r = timespan_t::from_seconds( 1.0 ); // set swing-time to 1.00 if there is no weapon
  return r;
}

warlock_pet_t::warlock_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_type_e pt, bool guardian ) :
  pet_t( sim, owner, pet_name, pt, guardian ), damage_modifier( 1.0 )
{
  gains_mana_feed = get_gain( "mana_feed" );
  procs_mana_feed = get_proc( "mana_feed" );
  stats_avaiable = 0;
  stats2_avaiable = 0;

  main_hand_weapon.type       = WEAPON_BEAST;
  main_hand_weapon.min_dmg    = get_weapon_min( level, pet_type );
  main_hand_weapon.max_dmg    = get_weapon_max( level, pet_type );
  main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
  main_hand_weapon.swing_time = get_weapon_swing_time( level, pet_type );
  if ( main_hand_weapon.swing_time == timespan_t::zero )
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
  resources.base[ RESOURCE_HEALTH ]  = get_attribute_base( level, BASE_STAT_HEALTH, pet_type );
  resources.base[ RESOURCE_MANA   ]  = get_attribute_base( level, BASE_STAT_MANA, pet_type );
  initial_attack_crit_per_agility   = get_attribute_base( level, BASE_STAT_MELEE_CRIT_PER_AGI, pet_type );
  initial_spell_crit_per_intellect  = get_attribute_base( level, BASE_STAT_SPELL_CRIT_PER_INT, pet_type );
  initial_dodge_per_agility         = get_attribute_base( level, BASE_STAT_DODGE_PER_AGI, pet_type );
  base_spell_crit                   = get_attribute_base( level, BASE_STAT_SPELL_CRIT, pet_type );
  base_attack_crit                  = get_attribute_base( level, BASE_STAT_MELEE_CRIT, pet_type );
  base_mp5                          = get_attribute_base( level, BASE_STAT_MP5, pet_type );

  if ( stats_avaiable != 13 )
    sim -> errorf( "Pet %s has no general base stats avaiable on level=%.i.\n", name(), level );
  if ( stats2_avaiable != 13 )
    sim -> errorf( "Pet %s has no base stats avaiable on level=%.i.\n", name(), level );

  initial_attack_power_per_strength = 2.0; // tested in-game as of 2010/12/20
  base_attack_power = -20; // technically, the first 20 str give 0 ap. - tested
  stamina_per_owner = 0.6496; // level invariant, tested
  intellect_per_owner = 0; // removed in cata, tested

  base_attack_crit                  += 0.0328; // seems to be level invariant, untested
  base_spell_crit                   += 0.0328; // seems to be level invariant, untested
  initial_attack_crit_per_agility   += 0.01 / 52.0; // untested
  initial_spell_crit_per_intellect  += owner -> initial_spell_crit_per_intellect; // untested
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

void warlock_pet_t::summon( timespan_t duration )
{
  warlock_t*  o = owner -> cast_warlock();
  pet_t::summon( duration );
  if ( o -> talent_demonic_pact -> rank() )
    sim -> auras.demonic_pact -> trigger();
}

void warlock_pet_t::dismiss()
{
  pet_t::dismiss();
  /* Commenting this out for now - we never dismiss the real pet during combat
  anyway, and we don't want to accidentally turn off DP when guardians are dismissed
  warlock_t*  o = owner -> cast_warlock();
  if ( o -> talent_demonic_pact -> rank() )
  sim -> auras.demonic_pact -> expire();
   */
}

double warlock_pet_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();
  h *= owner -> spell_haste;

  // According to Issue 881, DI on the owner doesn't increase pet haste, only when cast directly on the pet. 28/09/11
  if ( owner -> buffs.dark_intent -> check() )
    h *= 1.03;

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
  double sp = pet_t::composite_spell_power( school );
  sp += owner -> composite_spell_power( school ) * ( level / 80.0 ) * 0.5 * owner -> composite_spell_power_multiplier();
  return sp;
}

double warlock_pet_t::composite_attack_power() const
{
  double ap = pet_t::composite_attack_power();
  ap += owner -> composite_spell_power( SCHOOL_MAX ) * ( level / 80.0 ) * owner -> composite_spell_power_multiplier();
  return ap;
}

double warlock_pet_t::composite_attack_crit( weapon_t* ) const
{
  double ac = owner -> composite_spell_crit(); // Seems to just use our crit directly, based on very rough numbers, needs more testing.

  // According to Issue 881, FM on the owner doesn't increase pet crit, only when cast directly on the pet. 28/09/11
  if ( owner -> buffs.focus_magic -> check() )
    ac -= 0.03;

  return ac;
}

double warlock_pet_t::composite_spell_crit() const
{
  double sc = owner -> composite_spell_crit(); // Seems to just use our crit directly, based on very rough numbers, needs more testing.

  // According to Issue 881, FM on the owner doesn't increase pet crit, only when cast directly on the pet. 28/09/11
  if ( owner -> buffs.focus_magic -> check() )
    sc -= 0.03;

  return sc;
}

// ==========================================================================
// Warlock Main Pet
// ==========================================================================

warlock_main_pet_t::warlock_main_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_type_e pt ) :
  warlock_pet_t( sim, owner, pet_name, pt )
{}

void warlock_main_pet_t::summon( timespan_t duration )
{
  warlock_t* o = owner -> cast_warlock();
  o -> active_pet = this;
  warlock_pet_t::summon( duration );
}

void warlock_main_pet_t::dismiss()
{
  warlock_t* o = owner -> cast_warlock();
  warlock_pet_t::dismiss();
  o -> active_pet = 0;
}

double warlock_main_pet_t::composite_attack_expertise( weapon_t* ) const
{
  return owner -> spell_hit * 26.0 / 17.0;
}

resource_type_e warlock_main_pet_t::primary_resource() const { return RESOURCE_MANA; }

double warlock_main_pet_t::composite_player_multiplier( const school_type_e school, action_t* a ) const
{
  double m = warlock_pet_t::composite_player_multiplier( school, a );

  warlock_t* o = owner -> cast_warlock();

  double mastery_value = o -> mastery_spells.master_demonologist -> effect_base_value( 3 );

  m *= 1.0 + ( o -> mastery_spells.master_demonologist -> ok() * o -> composite_mastery() * mastery_value / 10000.0 );

  return m;
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

double warlock_guardian_pet_t::composite_attack_crit( weapon_t* ) const
{
  return snapshot_crit;
}

double warlock_guardian_pet_t::composite_attack_expertise( weapon_t* ) const
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

double warlock_guardian_pet_t::composite_spell_power_multiplier() const
{
  double m = warlock_pet_t::composite_spell_power_multiplier();
  warlock_t* o = owner -> cast_warlock();

  // Guardians normally don't gain demonic pact, but when they provide it they also provide it to themselves
  if ( o -> talent_demonic_pact -> rank() ) m *= 1.10;

  return m;
}

// ==========================================================================
// Pet Imp
// ==========================================================================

imp_pet_t::imp_pet_t( sim_t* sim, warlock_t* owner ) :
  warlock_main_pet_t( sim, owner, "imp", PET_IMP )
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

felguard_pet_t::felguard_pet_t( sim_t* sim, warlock_t* owner ) :
  warlock_main_pet_t( sim, owner, "felguard", PET_FELGUARD )
{
  damage_modifier = 1.0;

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

felhunter_pet_t::felhunter_pet_t( sim_t* sim, warlock_t* owner ) :
  warlock_main_pet_t( sim, owner, "felhunter", PET_FELHUNTER )
{
  damage_modifier = 0.8;

  action_list_str += "/snapshot_stats";
  action_list_str += "/shadow_bite";
  action_list_str += "/wait_for_shadow_bite";
}

void felhunter_pet_t::init_base()
{
  warlock_pet_t::init_base();

  // untested in cataclyms!!
  //health_per_stamina = 9.5;
  mana_per_intellect = 11.55;
  //mp5_per_intellect  = 8.0 / 324.0;
  base_mp5 = 11.22;
  // untested in cataclyms!!

  main_hand_attack = new warlock_pet_actions::warlock_pet_melee_t( this, "felhunter_melee" );
}

action_t* felhunter_pet_t::create_action( const std::string& name,
                                          const std::string& options_str )
{
  if ( name == "shadow_bite" ) return new felhunter_spells::shadow_bite_t( this );
  if ( name == "wait_for_shadow_bite" ) return new wait_for_cooldown_t( this, "shadow_bite" );

  return warlock_main_pet_t::create_action( name, options_str );
}

void felhunter_pet_t::summon( timespan_t duration )
{
  sim -> auras.fel_intelligence -> trigger();

  warlock_main_pet_t::summon( duration );
}

void felhunter_pet_t::dismiss()
{
  warlock_main_pet_t::dismiss();

  sim -> auras.fel_intelligence -> expire();
}

// ==========================================================================
// Pet Succubus
// ==========================================================================

succubus_pet_t::succubus_pet_t( sim_t* sim, warlock_t* owner ) :
  warlock_main_pet_t( sim, owner, "succubus", PET_SUCCUBUS )
{
  damage_modifier = 1.025;

  action_list_str += "/snapshot_stats";
  action_list_str += "/lash_of_pain";
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

voidwalker_pet_t::voidwalker_pet_t( sim_t* sim, warlock_t* owner ) :
  warlock_main_pet_t( sim, owner, "voidwalker", PET_VOIDWALKER )
{
  damage_modifier = 0.86;

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


double doomguard_pet_t::composite_player_multiplier( const school_type_e school, action_t* a ) const
{
  //FIXME: This is all untested, but seems to match what people are reporting in forums

  double m = player_t::composite_player_multiplier( school, a );

  warlock_t* o = owner -> cast_warlock();

  if ( o -> race == RACE_ORC )
  {
    m  *= 1.05;
  }

  double mastery_value = o -> mastery_spells.master_demonologist -> effect_base_value( 3 );

  double mastery_gain = ( o -> mastery_spells.master_demonologist -> ok() * snapshot_mastery * mastery_value / 10000.0 );

  m *= 1.0 + mastery_gain;

  return m;
}

// ==========================================================================
// Pet Ebon Imp
// ==========================================================================

ebon_imp_pet_t::ebon_imp_pet_t( sim_t* sim, warlock_t* owner ) :
  warlock_guardian_pet_t( sim, owner, "ebon_imp", PET_EBON_IMP )
{
  action_list_str += "/snapshot_stats";
}

timespan_t ebon_imp_pet_t::available() const
{ return sim -> max_time; }

double ebon_imp_pet_t::composite_attack_power() const
{
  return 0;
}

void ebon_imp_pet_t::init_base()
{
  warlock_guardian_pet_t::init_base();

  main_hand_attack = new warlock_pet_actions::warlock_pet_melee_t( this, "ebon_imp_melee" );
}

#endif // SC_WARLOCK
