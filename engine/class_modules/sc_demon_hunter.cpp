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
  } specs;

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
  } procs;

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
  void init_resources( bool force ) override;
  void init_spells() override;
  void create_buffs() override;
  void init_scaling() override;
  void reset() override;
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
  void create_procs();
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
  demon_hunter_action_t( const std::string& n, demon_hunter_t& p,
                         const spell_data_t* s = spell_data_t::nil() )
    : ab( n, &p, s )
  {
    ab::may_crit          = true;
    ab::tick_may_crit     = true;
    ab::weapon_multiplier = 0.0;
  }

  demon_hunter_t& demon_hunter()
  {
    return static_cast<demon_hunter_t&>( *ab::player );
  }

  const demon_hunter_t& demon_hunter() const
  {
    return static_cast<const demon_hunter_t&>( *ab::player );
  }

  demon_hunter_td_t& get_td( player_t* t ) const
  {
    return *( ab::player->get_target_data( t ) );
  }

protected:
  /// typedef for demon_hunter_action_t<action_base_t>
  typedef demon_hunter_action_t base_t;

private:
  /// typedef for the templated action type, eg. spell_t, attack_t, heal_t
  typedef Base ab;
};

// ==========================================================================
// Demon Hunter Melee Attack
// ==========================================================================

class demon_hunter_attack_t : public demon_hunter_action_t<melee_attack_t>
{
public:
  demon_hunter_attack_t( const std::string& n, demon_hunter_t& player,
                         const spell_data_t* s )
    : base_t( n, player, s )
  {
  }
};

namespace attacks
{
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

demon_hunter_t::demon_hunter_t( sim_t* sim, const std::string& name, race_e r )
  : player_t( sim, DEMON_HUNTER, name, r ),
    buffs(),
    talents(),
    specs(),
    mastery_spells(),
    cooldowns(),
    gains(),
    benefits(),
    procs(),
    active_spells(),
    pets(),
    options(),
    glyphs()
{
  base.distance = 5.0;

  create_cooldowns();
  create_gains();
  create_procs();
  create_benefits();

  regen_type = REGEN_DISABLED;
}

/* Construct cooldowns
 *
 */
void demon_hunter_t::create_cooldowns()
{
}

/* Construct  gains
 *
 */
void demon_hunter_t::create_gains()
{
}

/* Construct procs
 *
 */
void demon_hunter_t::create_procs()
{
}

/* Constructbenefits
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
  // TODO: implement

  switch ( s )
  {
    default:
      return s;
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

  return base_t::create_action( name, options_str );
}

pet_t* demon_hunter_t::create_pet( const std::string& pet_name,
                                   const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  // Add pets here

  sim->errorf( "No demon hunter pet with name '%s' available.",
               pet_name.c_str() );

  return nullptr;
}

void demon_hunter_t::create_pets()
{
  base_t::create_pets();
}

void demon_hunter_t::init_base_stats()
{
  base_t::init_base_stats();
}

void demon_hunter_t::init_resources( bool force )
{
  base_t::init_resources( force );
}

void demon_hunter_t::init_scaling()
{
  base_t::init_scaling();
}

void demon_hunter_t::init_spells()
{
  base_t::init_spells();
}

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

  options = source_p->options;
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
    p->report_extension = std::unique_ptr<player_report_extension_t>(
        new demon_hunter_report_t( *p ) );
    return p;
  }

  bool valid() const override
  {
    // TODO: enable if module is finished/for development
    return false;
  }

  void init( player_t* ) const override
  {
  }
  void static_init() const override
  {
  }

  void register_hotfixes() const override
  {
  }

  void combat_begin( sim_t* ) const override
  {
  }

  void combat_end( sim_t* ) const override
  {
  }
};

}  // UNNAMED NAMESPACE

const module_t* module_t::demon_hunter()
{
  static demon_hunter_module_t m;
  return &m;
}
