// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace  // UNNAMED NAMESPACE
{
/* Forward declarations
 */
class demon_hunter_t;

/**
 * Demon Hunter target data
 * Contains target specific things
 */
class demon_hunter_td_t : public actor_target_data_t
{
public:
  struct dots_t
  {
  } dots;

  struct buffs_t
  {
  } buffs;

  demon_hunter_td_t( player_t* target, demon_hunter_t& p );
};

/* Demon Hunter class definition
 *
 * Derived from player_t. Contains everything that defines the Demon Hunter
 * class.
 */
class demon_hunter_t : public player_t
{
public:
  typedef player_t base_t;

  event_t* blade_dance_driver;
  std::vector<attack_t*> blade_dance_attacks;

  // Buffs
  struct
  {
  } buffs;

  // Talents
  struct
  {
  } talents;

  // Specialization Spells
  struct
  {
    const spell_data_t* blade_dance;
  } spec;

  // Mastery Spells
  struct
  {
  } mastery_spells;

  // Cooldowns
  struct
  {
  } cooldowns;

  // Gains
  struct
  {
  } gains;

  // Benefits
  struct
  {
  } benefits;

  // Procs
  struct
  {
    proc_t* delayed_auto_attack;
  } proc;

  // Special
  struct
  {
  } active_spells;

  // Pets
  struct
  {
  } pets;

  // Options
  struct demon_hunter_options_t
  {
  } options;

  // Glyphs
  struct
  {
  } glyphs;

  demon_hunter_t( sim_t* sim, const std::string& name, race_e r );

  // player_t overrides
  void init_base_stats() override;
  void init_procs() override;
  void init_resources( bool force ) override;
  void init_spells() override;
  void create_buffs() override;
  void init_scaling() override;
  void reset() override;
  void interrupt() override;
  void create_options() override;
  std::string create_profile( save_e = SAVE_ALL ) override;
  action_t* create_action( const std::string& name,
                           const std::string& options ) override;
  pet_t* create_pet( const std::string& name,
                     const std::string& type = std::string() ) override;
  void create_pets() override;
  void copy_from( player_t* source ) override;
  resource_e primary_resource() const override
  {
    return RESOURCE_MANA;
  }
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  void invalidate_cache( cache_e ) override;
  void target_mitigation( school_e, dmg_e, action_state_t* ) override;
  void init_action_list() override;
  demon_hunter_td_t* get_target_data( player_t* target ) const override;
  bool has_t18_class_trinket() const override;

private:
  void create_cooldowns();
  void create_gains();
  void create_benefits();
  void apl_precombat();
  void apl_default();
  demon_hunter_td_t* find_target_data( player_t* target ) const;

  target_specific_t<demon_hunter_td_t> _target_data;
};

namespace pets
{
// ==========================================================================
// Demon Hunter Pets
// ==========================================================================

/* Demon Hunter pet base
 *
 * defines characteristics common to ALL Demon Hunter pets
 */
struct demon_hunter_pet_t : public pet_t
{
  demon_hunter_pet_t( sim_t* sim, demon_hunter_t& owner,
                      const std::string& pet_name, pet_e pt,
                      bool guardian = false )
    : pet_t( sim, &owner, pet_name, pt, guardian )
  {
    base.position = POSITION_BACK;
    base.distance = 3;
  }

  struct _stat_list_t
  {
    int level;
    std::array<double, ATTRIBUTE_MAX> stats;
  };

  void init_base_stats() override
  {
    pet_t::init_base_stats();

    owner_coeff.ap_from_sp = 1.0;
    owner_coeff.sp_from_sp = 1.0;

    // Base Stats, same for all pets. Depends on level
    static const _stat_list_t pet_base_stats[] = {
        //   none, str, agi, sta, int, spi
        {85, {{0, 453, 883, 353, 159, 225}}},
    };

    // Loop from end to beginning to get the data for the highest available
    // level equal or lower than the player level
    int i = as<int>( sizeof_array( pet_base_stats ) );
    while ( --i > 0 )
    {
      if ( pet_base_stats[ i ].level <= level() )
        break;
    }
    if ( i >= 0 )
      base.stats.attribute = pet_base_stats[ i ].stats;
  }

  void schedule_ready( timespan_t delta_time, bool waiting ) override
  {
    if ( main_hand_attack && !main_hand_attack->execute_event )
    {
      main_hand_attack->schedule_execute();
    }

    pet_t::schedule_ready( delta_time, waiting );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    // Orc racial
    m *= 1.0 + o().racials.command->effectN( 1 ).percent();

    return m;
  }

  resource_e primary_resource() const override
  {
    return RESOURCE_ENERGY;
  }

  demon_hunter_t& o() const
  {
    return static_cast<demon_hunter_t&>( *owner );
  }
};

namespace actions
{  // namespace for pet actions

}  // end namespace actions ( for pets )

}  // END pets NAMESPACE

namespace actions
{
/* This is a template for common code for all Demon Hunter actions.
 * The template is instantiated with either spell_t, heal_t or absorb_t as the
 * 'Base' class.
 * Make sure you keep the inheritance hierarchy and use base_t in the derived
 * class,
 * don't skip it and call spell_t/heal_t or absorb_t directly.
 */
template <typename Base>
class demon_hunter_action_t : public Base
{
public:
  gain_t* action_gain;
   
  demon_hunter_action_t( const std::string& n, demon_hunter_t* p,
                         const spell_data_t* s = spell_data_t::nil() )
    : ab( n, p, s ), action_gain( p -> get_gain( n ) )
  {
    ab::may_crit          = true;
    ab::tick_may_crit     = true;
  }

  demon_hunter_t* p()
  {
    return static_cast<demon_hunter_t*>( ab::player );
  }

  const demon_hunter_t* p() const
  {
    return static_cast<const demon_hunter_t*>( ab::player );
  }

  demon_hunter_td_t* get_td( player_t* t ) const
  {
    return ab::player -> get_target_data( t );
  }

protected:
  /// typedef for demon_hunter_action_t<action_base_t>
  typedef demon_hunter_action_t base_t;

private:
  /// typedef for the templated action type, eg. spell_t, attack_t, heal_t
  typedef Base ab;
};

// ==========================================================================
// Demon Hunter heals
// ==========================================================================

struct demon_hunter_heal_t: public demon_hunter_action_t < heal_t >
{
  demon_hunter_heal_t( const std::string& n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    target = p;
  }
};

// ==========================================================================
// Demon Hunter spells -- For spells that deal no damage, usually buffs.
// ==========================================================================

struct demon_hunter_spell_t: public demon_hunter_action_t < spell_t >
{
  demon_hunter_spell_t( const std::string& n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }
};

// ==========================================================================
// Demon Hunter attacks
// ==========================================================================

struct demon_hunter_attack_t: public demon_hunter_action_t < melee_attack_t >
{
  demon_hunter_attack_t( const std::string& n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    special = true;
  }
};

namespace attacks
{
  
// Melee Attack =============================================================

struct melee_t : public demon_hunter_attack_t
{
  bool mh_lost_melee_contact, oh_lost_melee_contact;

  melee_t( const std::string& name, demon_hunter_t* p ) :
    demon_hunter_attack_t( name, p, spell_data_t::nil() ),
    mh_lost_melee_contact( true ), oh_lost_melee_contact( true )
  {
    school = SCHOOL_PHYSICAL;
    special = false;
    background = repeating = auto_attack = may_glance = true;
    trigger_gcd = timespan_t::zero();

    if ( p -> dual_wield() )
      base_hit -= 0.19;
  }

  void reset() override
  {
    demon_hunter_attack_t::reset();

    mh_lost_melee_contact = true;
    oh_lost_melee_contact = true;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = demon_hunter_attack_t::execute_time();

    if ( weapon -> slot == SLOT_MAIN_HAND && mh_lost_melee_contact )
      return timespan_t::zero(); // If contact is lost, the attack is instant.
    else if ( weapon -> slot == SLOT_OFF_HAND && oh_lost_melee_contact ) // Also used for the first attack.
      return timespan_t::zero();
    else
      return t;
  }

  void schedule_execute( action_state_t* s ) override
  {
    demon_hunter_attack_t::schedule_execute( s );

    if ( weapon -> slot == SLOT_MAIN_HAND )
      mh_lost_melee_contact = false;
    else if ( weapon -> slot == SLOT_OFF_HAND )
      oh_lost_melee_contact = false;
  }

  void execute() override
  {
    if ( p() -> current.distance_to_move > 5 )
    {
      // Cancel autoattacks, auto_attack_t will restart them when we're back in range.
      if ( weapon -> slot == SLOT_MAIN_HAND )
      {
        p() -> proc.delayed_auto_attack -> occur();
        mh_lost_melee_contact = true;
        player -> main_hand_attack -> cancel();
      }
      else
      {
        p() -> proc.delayed_auto_attack -> occur();
        oh_lost_melee_contact = true;
        player -> off_hand_attack -> cancel();
      }
      return;
    }

    demon_hunter_attack_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    // FIXME: Pulled this number out of my ass for testing purposes.
    if ( result_is_hit( s -> result ) )
      p() -> resource_gain( RESOURCE_FURY, weapon -> swing_time.total_seconds() * 2, action_gain );
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public demon_hunter_attack_t
{
  auto_attack_t( demon_hunter_t* p, const std::string& options_str ):
    demon_hunter_attack_t( "auto_attack", p )
  {
    parse_options( options_str );
    assert( p -> main_hand_weapon.type != WEAPON_NONE );

    ignore_false_positive = true;
    range = 5;
    trigger_gcd = timespan_t::zero();

    p -> main_hand_attack = new melee_t( "auto_attack_mh", p );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> off_hand_attack = new melee_t( "auto_attack_oh", p );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
      p -> off_hand_attack -> id = 1;
    }
  }

  void execute() override
  {
    if ( p() -> main_hand_attack -> execute_event == nullptr )
    {
      p() -> main_hand_attack -> schedule_execute();
    }

    if ( p() -> off_hand_attack && p() -> off_hand_attack -> execute_event == nullptr )
    {
      p() -> off_hand_attack -> schedule_execute();
    }
  }

  bool ready() override
  {
    bool ready = demon_hunter_attack_t::ready();

    if ( ready ) // Range check
    {
      if ( p() -> main_hand_attack -> execute_event == nullptr )
        return ready;

      // Don't check for execute_event if we don't have an offhand.
      if ( p() -> off_hand_attack && p() -> off_hand_attack -> execute_event == nullptr )
        return ready;
    }

    return false;
  }
};

// Blade Dance =============================================================

struct blade_dance_attack_t: public demon_hunter_attack_t
{
  blade_dance_attack_t( demon_hunter_t* p, const spell_data_t* blade_dance, const std::string& name ):
    demon_hunter_attack_t( name, p, blade_dance )
  {
    dual = true;
  }
};

struct blade_dance_event_t: public event_t
{
  timespan_t duration;
  demon_hunter_t* dh;
  size_t attacks;

  blade_dance_event_t( demon_hunter_t*p, size_t current_attack ):
    event_t( *p -> sim ), dh( p ), attacks( current_attack )
  {
    duration = next_execute();
    add_event( duration );
    if ( sim().debug ) sim().out_debug.printf( "New blade dance event" );
  }

  timespan_t next_execute() const
  {
    return timespan_t::from_millis( dh -> spec.blade_dance -> effectN( attacks + 5 ).misc_value1() - ( attacks > 0 ? dh -> spec.blade_dance -> effectN( attacks + 4 ).misc_value1() : 0 ) );
  }

  void execute() override
  {
    dh -> blade_dance_attacks[ attacks ] -> execute();
    attacks++;
    if ( attacks < dh -> blade_dance_attacks.size() )
    {
      dh -> blade_dance_driver = new ( sim() ) blade_dance_event_t( dh, attacks );
    }
  }
};

struct blade_dance_parent_t: public demon_hunter_attack_t
{
  blade_dance_parent_t( demon_hunter_t* p, const std::string& options_str ):
    demon_hunter_attack_t( "blade_dance", p, p -> spec.blade_dance )
  {
    parse_options( options_str );
    weapon = &( p -> main_hand_weapon );

    for ( size_t i = 0; i < p -> blade_dance_attacks.size(); i++ )
    {
      add_child( p -> blade_dance_attacks[ i ] );
    }
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) ) // If the first attack fails to land, the rest do too. 
    {
      p() -> blade_dance_driver = new ( *sim ) blade_dance_event_t( p(), 0 );
    }
  }

  bool ready() override
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return demon_hunter_attack_t::ready();
  }
};

// Chaos Strike =============================================================

struct chaos_strike_t : public demon_hunter_attack_t
{
  struct chaos_strike_oh_t : public demon_hunter_attack_t
  {
    chaos_strike_oh_t( demon_hunter_t* p ) :
      demon_hunter_attack_t( "chaos_strike", p, p -> find_spell( 199547 ) )
    {
      may_miss = may_parry = may_dodge = false;
      weapon = &( p -> off_hand_weapon );
    }
  };

  chaos_strike_oh_t* off_hand;

  chaos_strike_t( demon_hunter_t* p, const std::string& options_str ) :
    demon_hunter_attack_t( "chaos_strike", p, p -> find_class_spell( "Chaos Strike" ) )
  {
    parse_options( options_str );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      off_hand = new chaos_strike_oh_t( p );
      add_child( off_hand );
    }
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( result_is_hit( s -> result ) && p() -> off_hand_weapon.type != WEAPON_NONE ) // TOCHECK
    {
      off_hand -> target = target;
      off_hand -> execute();
    }
  }

  void consume_resource() override
  {
    if ( execute_state -> result == RESULT_CRIT )
      return;

    demon_hunter_attack_t::consume_resource();
  }
};

// Demon's Bite =============================================================

struct demons_bite_t : public demon_hunter_attack_t
{
  struct {
    double base, range;
  } fury;

  demons_bite_t( demon_hunter_t* p, const std::string& options_str ) :
    demon_hunter_attack_t( "demons_bite", p, p -> find_class_spell( "Demon's Bite" ) )
  {
    parse_options( options_str );

    fury.base  = data().effectN( 3 ).resource( RESOURCE_FURY ) + 1.0;
    fury.range = data().effectN( 3 ).die_sides() - 1.0;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> resource_gain( RESOURCE_FURY, fury.base + rng().real() * fury.range, action_gain );
    }
  }
};

// Eye Beam =================================================================
/* TODO: Eye Beam haste mechanics. Verify # of ticks (spell data says 2 sec duration, 
  200 ms interval, but obviously that doesn't work out to the 9 ticks the tooltip states. */

struct eye_beam_t : public demon_hunter_attack_t
{
  eye_beam_t( demon_hunter_t* p, const std::string& options_str ) :
    demon_hunter_attack_t( "eye_beam", p, p -> find_class_spell( "Eye Beam" ) )
  {
    parse_options( options_str );

    const spell_data_t* damage_spell = p -> find_spell( 198030 );
    attack_power_mod.tick = damage_spell -> effectN( 1 ).ap_coeff();
    school = damage_spell -> get_school_type();

    direct_tick = tick_zero = true;
    aoe = -1;
  }
};


}  // attacks namespace

}  // NAMESPACE actions

namespace buffs
{
}  // end namespace buffs

// ==========================================================================
// Targetdata Definitions
// ==========================================================================

demon_hunter_td_t::demon_hunter_td_t( player_t* target, demon_hunter_t& p )
  : actor_target_data_t( target, &p ), dots(), buffs()
{
}

// ==========================================================================
// Demon Hunter Definitions
// ==========================================================================

demon_hunter_t::demon_hunter_t( sim_t* sim, const std::string& name, race_e r ) :
  player_t( sim, DEMON_HUNTER, name, r ),
  buffs(),
  talents(),
  spec(),
  mastery_spells(),
  cooldowns(),
  gains(),
  benefits(),
  proc(),
  active_spells(),
  pets(),
  options(),
  glyphs(),
  blade_dance_driver( nullptr ),
  blade_dance_attacks( 0 )
{
  base.distance = 5.0;

  create_cooldowns();
  create_gains();
  create_benefits();

  regen_type = REGEN_DISABLED;
}

/* Construct cooldowns
 *
 */
void demon_hunter_t::create_cooldowns()
{
}

/* Construct gains
 *
 */
void demon_hunter_t::create_gains()
{
}

/* Construct benefits
 *
 */
void demon_hunter_t::create_benefits()
{
}

/* Define the acting role of the Demon Hunter
 */
role_e demon_hunter_t::primary_role() const
{
  // TODO: handle tank/dps choice

  return ROLE_DPS;
}

/**
 * @brief Convert hybrid stats
 *
 *  Converts hybrid stats that either morph based on spec or only work
 *  for certain specs into the appropriate "basic" stats
 */
stat_e demon_hunter_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  case STAT_STR_AGI_INT:
  case STAT_AGI_INT: 
  case STAT_STR_AGI:
    return STAT_AGILITY;
  case STAT_STR_INT:
    return STAT_NONE;
  case STAT_SPIRIT:
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
      return specialization() == DEMON_HUNTER_VENGEANCE ? s : STAT_NONE;  
  default: return s; 
  }
}

double demon_hunter_t::matching_gear_multiplier( attribute_e attr ) const
{
  // TODO: implement
  (void) attr;

  return 0.0;
}

action_t* demon_hunter_t::create_action( const std::string& name,
                                         const std::string& options_str )
{
  using namespace actions::attacks;

  if ( name == "auto_attack"  ) return new auto_attack_t       ( this, options_str );
  if ( name == "blade_dance"  ) return new blade_dance_parent_t( this, options_str );
  if ( name == "chaos_strike" ) return new chaos_strike_t      ( this, options_str );
  if ( name == "demons_bite"  ) return new demons_bite_t       ( this, options_str );
  if ( name == "eye_beam"     ) return new eye_beam_t          ( this, options_str );

  return base_t::create_action( name, options_str );
}

pet_t* demon_hunter_t::create_pet( const std::string& pet_name,
                                   const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  // Add pets here

  sim -> errorf( "No demon hunter pet with name '%s' available.",
               pet_name.c_str() );

  return nullptr;
}

// demon_hunter_t::create_pets ==============================================

void demon_hunter_t::create_pets()
{
  base_t::create_pets();
}

// demon_hunter_t::init_base_stats ==========================================

void demon_hunter_t::init_base_stats()
{
  base_t::init_base_stats();

  resources.base[ RESOURCE_FURY ] = 100;

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility = 1.0;

  // Avoidance diminishing Returns constants/conversions now handled in player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.

  base_gcd = timespan_t::from_seconds( 1.5 );
}

// demon_hunter_t::init_resources ===========================================

void demon_hunter_t::init_resources( bool force )
{
  base_t::init_resources( force );

  resources.current[ RESOURCE_FURY ] = 0;
}

// demon_hunter_t::init_procs ===============================================

void demon_hunter_t::init_procs()
{
  base_t::init_procs();

  proc.delayed_auto_attack = get_proc( "delayed_auto_attack" );
}

// demon_hunter_t::init_scaling =============================================

void demon_hunter_t::init_scaling()
{
  base_t::init_scaling();

  if ( off_hand_weapon.type != WEAPON_NONE )
    scales_with[ STAT_WEAPON_OFFHAND_DPS ] = true;

  if ( specialization() == DEMON_HUNTER_VENGEANCE )
    scales_with[ STAT_BONUS_ARMOR ] = true;

  scales_with[ STAT_STRENGTH ] = false;
}

// demon_hunter_t::init_spells ==============================================

void demon_hunter_t::init_spells()
{
  base_t::init_spells();

  spec.blade_dance = find_class_spell( "Blade Dance" );

  if ( spec.blade_dance -> ok() )
  {
    actions::attacks::blade_dance_attack_t* first = new actions::attacks::blade_dance_attack_t( this, spec.blade_dance -> effectN( 5 ).trigger(), "blade_dance1" );
    actions::attacks::blade_dance_attack_t* second = new actions::attacks::blade_dance_attack_t( this, spec.blade_dance -> effectN( 6 ).trigger(), "blade_dance2" );
    actions::attacks::blade_dance_attack_t* third = new actions::attacks::blade_dance_attack_t( this, spec.blade_dance -> effectN( 7 ).trigger(), "blade_dance3" );
    this -> blade_dance_attacks.push_back( first );
    this -> blade_dance_attacks.push_back( second );
    this -> blade_dance_attacks.push_back( third );
  }
}

// demon_hunter_t::create_buffs =============================================

void demon_hunter_t::create_buffs()
{
  base_t::create_buffs();
}

// ALL Spec Pre-Combat Action Priority List

void demon_hunter_t::apl_precombat()
{
  //action_priority_list_t* precombat = get_action_priority_list( "precombat" );
}

bool demon_hunter_t::has_t18_class_trinket() const
{
  return false;
}

// NO Spec Combat Action Priority List

void demon_hunter_t::apl_default()
{
  //action_priority_list_t* def = get_action_priority_list( "default" );
}

/* Always returns non-null targetdata pointer
 */
demon_hunter_td_t* demon_hunter_t::get_target_data( player_t* target ) const
{
  auto& td = _target_data[ target ];
  if ( !td )
  {
    td = new demon_hunter_td_t( target, const_cast<demon_hunter_t&>( *this ) );
  }
  return td;
}

/* Returns targetdata if found, nullptr otherwise
 */
demon_hunter_td_t* demon_hunter_t::find_target_data( player_t* target ) const
{
  return _target_data[ target ];
}

void demon_hunter_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  apl_precombat();  // PRE-COMBAT

  switch ( specialization() )
  {
    default:
      apl_default();  // DEFAULT
      break;
  }

  use_default_action_list = true;

  base_t::init_action_list();
}

void demon_hunter_t::reset()
{
  base_t::reset();

  blade_dance_driver = nullptr;
}

void demon_hunter_t::interrupt()
{
  if ( blade_dance_driver )
  {
    event_t::cancel( blade_dance_driver );
  }

  base_t::interrupt();
}

void demon_hunter_t::target_mitigation( school_e school, dmg_e dt,
                                        action_state_t* s )
{
  base_t::target_mitigation( school, dt, s );
}

void demon_hunter_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );
}

void demon_hunter_t::create_options()
{
  base_t::create_options();
}

std::string demon_hunter_t::create_profile( save_e type )
{
  std::string profile_str = base_t::create_profile( type );

  // Log all options here

  return profile_str;
}

void demon_hunter_t::copy_from( player_t* source )
{
  base_t::copy_from( source );

  auto source_p = debug_cast<demon_hunter_t*>( source );

  options = source_p -> options;
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class demon_hunter_report_t : public player_report_extension_t
{
public:
  demon_hunter_report_t( demon_hunter_t& player ) : p( player )
  {
  }

  void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void)p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
        << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
        << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }

private:
  demon_hunter_t& p;
};

// MODULE INTERFACE ==================================================

class demon_hunter_module_t : public module_t
{
public:
  demon_hunter_module_t() : module_t( DEMON_HUNTER )
  {
  }

  player_t* create_player( sim_t* sim, const std::string& name,
                           race_e r = RACE_NONE ) const override
  {
    auto p              = new demon_hunter_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>(
        new demon_hunter_report_t( *p ) );
    return p;
  }

  bool valid() const override { return true; }

  void init( player_t* ) const override {}
  void static_init() const override {}

  void register_hotfixes() const override {}

  void combat_begin( sim_t* ) const override {}

  void combat_end( sim_t* ) const override {}
};

}  // UNNAMED NAMESPACE

const module_t* module_t::demon_hunter()
{
  static demon_hunter_module_t m;
  return &m;
}
