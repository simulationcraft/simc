// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"
#include "simulationcraft.hpp"
#include "class_modules/apl/apl_evoker.hpp"

namespace
{
// ==========================================================================
// Evoker
// ==========================================================================

// Forward declarations
struct evoker_t;

enum empower_e
{
  EMPOWER_NONE = 0,
  EMPOWER_1 = 1,
  EMPOWER_2,
  EMPOWER_3,
  EMPOWER_4,
  EMPOWER_MAX
};

enum spell_color_e
{
  SPELL_COLOR_NONE = 0,
  SPELL_BLACK,
  SPELL_BLUE,
  SPELL_BRONZE,
  SPELL_GREEN,
  SPELL_RED
};

struct empowered_state_t : public action_state_t
{
  empower_e empower;

  empowered_state_t( action_t* a, player_t* t ) : action_state_t( a, t ), empower( empower_e::EMPOWER_NONE ) {}

  void initialize() override
  {
    action_state_t::initialize();
    empower = empower_e::EMPOWER_NONE;
  }

  void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );
    empower = debug_cast<const empowered_state_t*>( s )->empower;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s ) << " empower_level=" << static_cast<int>( empower );
    return s;
  }
};

struct evoker_td_t : public actor_target_data_t
{
  struct dots_t
  {

  } dots;

  struct debuffs_t
  {

  } debuffs;

  evoker_td_t( player_t* target, evoker_t* source );
};

struct evoker_t : public player_t
{
  // Options
  struct options_t
  {

  } option;

  // Action pointers
  struct actions_t
  {

  } action;

  // Buffs
  struct buffs_t
  {
    // Baseline Abilities
    propagate_const<buff_t*> essence_burst;
    // Class Traits
    // Devastation Traits
    // Preservation Traits
  } buff;

  // Specialization Spell Data
  struct specializations_t
  {
    // Baseline
    const spell_data_t* evoker;        // evoker class aura
    const spell_data_t* devastation;   // devastation class aura
    const spell_data_t* preservation;  // preservation class aura
    // Devastation
    // Preservation
  } spec;

  // Talents
  struct talents_t
  {
    // Class Traits
    // Devastation Traits
    player_talent_t ruby_essence_burst;        // row 2 col 1 
    player_talent_t ruby_embers;  // row 5 col 1 choice 1
    player_talent_t engulfing_blaze;    // row 5 col 1 choice 2
    player_talent_t font_of_magic;  // row 8 col 3
    // Preservation Traits
  } talent;

  // Benefits
  struct benefits_t
  {

  } benefit;

  // Cooldowns
  struct cooldowns_t
  {

  } cooldown;

  // Gains
  struct gains_t
  {

  } gain;

  // Procs
  struct procs_t
  {
    propagate_const<proc_t*> ruby_essence_burst;
  } proc;

  // RPPMs
  struct rppms_t
  {

  } rppm;

  // Uptimes
  struct uptimes_t
  {

  } uptime;

  evoker_t( sim_t * sim, std::string_view name, race_e r = RACE_DRACTHYR_HORDE );

  // Character Definitions
  void init_action_list() override;
  void init_base_stats() override;
  //void init_resources( bool ) override;
  //void init_benefits() override;
  //void init_gains() override;
  void init_procs() override;
  //void init_rng() override;
  //void init_uptimes() override;
  void init_spells() override;
  //void init_finished() override;
  void create_actions() override;
  void create_buffs() override;
  void create_options() override;
  //void arise() override;
  //void combat_begin() override;
  //void combat_end() override;
  //void reset() override;
  void copy_from( player_t* ) override;
  void merge( player_t& ) override;

  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;

  target_specific_t<evoker_td_t> target_data;
  const evoker_td_t* find_target_data( const player_t* target ) const override;
  evoker_td_t* get_target_data( player_t* target ) const override;

  void apply_affecting_auras( action_t& action ) override;
  action_t* create_action( std::string_view name, std::string_view options_str ) override;
  std::unique_ptr<expr_t> create_expression( std::string_view expr_str ) override;

  // Stat & Multiplier overrides
  stat_e convert_hybrid_stat( stat_e ) const override;
  double resource_regen_per_second( resource_e ) const override;

  // Utility functions
  const spelleffect_data_t* find_spelleffect( const spell_data_t* spell,
                                              effect_subtype_t subtype,
                                              int misc_value = P_GENERIC,
                                              const spell_data_t* affected = spell_data_t::nil(),
                                              effect_type_t type = E_APPLY_AURA );
  const spell_data_t* find_spell_override( const spell_data_t* base, const spell_data_t* passive );

  std::vector<action_t*> secondary_action_list;

  template <typename T, typename... Ts>
  T* get_secondary_action( std::string_view n, Ts&&... args )
  {
    auto it = range::find( secondary_action_list, n, &action_t::name_str );
    if ( it != secondary_action_list.cend() )
      return dynamic_cast<T*>( *it );

    auto a = new T( this, std::forward<Ts>( args )... );
    a->background = true;
    secondary_action_list.push_back( a );
    return a;
  }
};

namespace buffs
{
// Template for base evoker buffs
template <class Base>
struct evoker_buff_t : public Base
{
private:
  using bb = Base;  // buff base, buff_t/stat_buff_t/etc.

protected:
  using base_t = evoker_buff_t<buff_t>;  // shorthand

public:
  evoker_buff_t( evoker_t* player, std::string_view name, const spell_data_t* spell = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : bb( player, name, spell, item )
  {}

  evoker_buff_t( evoker_td_t& td, std::string_view name, const spell_data_t* spell = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : bb( td, name, spell, item )
  {}

  evoker_t* p()
  { return static_cast<evoker_t*>( bb::source ); }

  const evoker_t* p() const
  { return static_cast<evoker_t*>( bb::source ); }
};
}  // namespace buffs

// Template for base evoker action code.
template <class Base>
struct evoker_action_t : public Base
{
private:
  using ab = Base;  // action base, spell_t/heal_t/etc.

public:
  spell_color_e spell_color;

  evoker_action_t( std::string_view name, evoker_t* player, const spell_data_t* spell = spell_data_t::nil() )
    : ab( name, player, spell ), spell_color( SPELL_COLOR_NONE )
  {
    // TODO: find out if there is a better data source for the spell color
    std::string_view desc = player->dbc->spell_text( ab::data().id() ).rank();
    if ( !desc.empty() )
    {
      if ( util::str_compare_ci( desc, "Black" ) )
        spell_color = SPELL_BLACK;
      else if ( util::str_compare_ci( desc, "Blue" ) )
        spell_color = SPELL_BLUE;
      else if ( util::str_compare_ci( desc, "Bronze" ) )
        spell_color = SPELL_BRONZE;
      else if ( util::str_compare_ci( desc, "Green" ) )
        spell_color = SPELL_GREEN;
      else if ( util::str_compare_ci( desc, "Red" ) )
        spell_color = SPELL_RED;
    }
  }

  evoker_t* p()
  { return static_cast<evoker_t*>( ab::player ); }

  const evoker_t* p() const
  { return static_cast<evoker_t*>( ab::player ); }

  evoker_td_t* td( player_t* t ) const
  { return p()->get_target_data( t ); }

  const evoker_td_t* find_td( const player_t* t ) const
  { return p()->find_target_data( t ); }
};

namespace spells
{
// Base Classes =============================================================

struct evoker_spell_t : public evoker_action_t<spell_t>
{
private:
  using ab = evoker_action_t<spell_t>;

public:
  evoker_spell_t( std::string_view name, evoker_t* player, const spell_data_t* spell = spell_data_t::nil(),
                  std::string_view options_str = {} )
    : ab( name, player, spell )
  {
    parse_options( options_str );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t );

    // Preliminary testing shows this is linear with target hp %.
    // TODO: confirm this applies only to all evoker offensive spells
    tm *= 1.0 + ( p()->cache.mastery_value() * t->health_percentage() / 100 );

    return tm;
  }
};

struct empowered_spell_t : public evoker_spell_t
{
  std::string empower_to_str;
  empower_e max_empower;

  empowered_spell_t( std::string_view name, evoker_t* p, const spell_data_t* spell, std::string_view options_str = {} )
    : evoker_spell_t( name, p, p->find_spell_override( spell, p->talent.font_of_magic ) ),
      max_empower( p->talent.font_of_magic.ok() ? empower_e::EMPOWER_4 : empower_e::EMPOWER_3 )
  {
    // TODO: convert to full empower expression support
    add_option( opt_string( "empower_to", empower_to_str ) );
    parse_options( options_str );
  }

  action_state_t* new_state() override
  { return new empowered_state_t( this, target ); }

  empower_e empower_level( const action_state_t* s ) const
  { return debug_cast<const empowered_state_t*>( s )->empower; }

  int empower_value( const action_state_t* s ) const
  { return static_cast<int>( debug_cast<const empowered_state_t*>( s )->empower ); }

  virtual empower_e empower_level() const
  {
    // TODO: return the current empowerment level based on elapsed cast/channel time
    return empower_e::EMPOWER_3;
  }

  timespan_t time_to_empower( empower_e empower ) const
  {
    // TODO: confirm these values and determine if they're set values or adjust based on a formula
    // Currently all empowered spells are 2.5s base and 3.25s with empower 4
    switch ( std::min( empower, max_empower ) )
    {
      case empower_e::EMPOWER_1: return 1000_ms;
      case empower_e::EMPOWER_2: return 1750_ms;
      case empower_e::EMPOWER_3: return 2500_ms;
      case empower_e::EMPOWER_4: return 3250_ms;
      default: break;
    }

    return 0_ms;
  }

  timespan_t max_hold_time() const
  { return time_to_empower( max_empower ) + 2_s; }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    evoker_spell_t::snapshot_internal( s, flags, rt );
    debug_cast<empowered_state_t*>( s )->empower = empower_level();
  }
};

// Spells ===================================================================

struct disintegrate_t : public evoker_spell_t
{
  disintegrate_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "disintegrate", p, p->find_class_spell( "Disintegrate" ) )
  {
    channeled = true;
  }
};

struct living_flame_t : public evoker_spell_t
{
  struct living_flame_damage_t : public evoker_spell_t
  {
    living_flame_damage_t( evoker_t* p ) : evoker_spell_t( "living_flame_damage", p, p->find_spell( 361500 ) )
    {
      dual = true;

      dot_duration = p->talent.ruby_embers.ok() ? dot_duration : 0_ms;
    }
  };

  evoker_spell_t* damage;

  living_flame_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "living_flame", p, p->find_class_spell( "Living Flame" ) )
  {

    damage        = p->get_secondary_action<living_flame_damage_t>( "living_flame_damage" );
    damage->stats = stats;
  }

  void execute() override
  {
    evoker_spell_t::execute();

    if ( p()->talent.ruby_essence_burst.ok() && rng().roll( p()->talent.ruby_essence_burst->effectN(1).percent() ) )
    {
      p()->buff.essence_burst->trigger();
      p()->proc.ruby_essence_burst->occur();
    }
  }

  void impact( action_state_t* s ) override
  {
    evoker_spell_t::impact( s );

    damage->schedule_execute();
  }
};

struct fire_breath_t : public empowered_spell_t
{
  struct fire_breath_damage_t : public empowered_spell_t
  {
    fire_breath_damage_t( evoker_t* p ) : empowered_spell_t( "fire_breath_damage", p, p->find_spell( 357209 ) )
    {
      dual = true;
    }

    timespan_t composite_dot_duration( const action_state_t* s ) const override
    {
      return empowered_spell_t::composite_dot_duration( s ) * empower_value( s ) / 3.0;
    }
  };

  empowered_spell_t* damage;

  fire_breath_t( evoker_t* p, std::string_view options_str )
    : empowered_spell_t( "fire_breath", p, p->find_class_spell( "Fire Breath" ) )
  {
    damage = p->get_secondary_action<fire_breath_damage_t>( "fire_breath_damage" );
    damage->stats = stats;
  }

  void impact( action_state_t* s ) override
  {
    empowered_spell_t::impact( s );

    auto emp_state = damage->get_state();
    emp_state->target = s->target;
    damage->snapshot_state( emp_state, damage->amount_type( emp_state ) );
    debug_cast<empowered_state_t*>( emp_state )->empower = empower_level();

    damage->schedule_execute( emp_state );
  }
};
}  // end namespace spells

// ==========================================================================
// Evoker Character Definitions
// ==========================================================================

evoker_td_t::evoker_td_t( player_t* target, evoker_t* evoker )
  : actor_target_data_t( target, evoker ),
    dots(),
    debuffs()
{

}

evoker_t::evoker_t( sim_t* sim, std::string_view name, race_e r )
  : player_t( sim, EVOKER, name, r ),
    option(),
    action(),
    buff(),
    spec(),
    talent(),
    benefit(),
    cooldown(),
    gain(),
    proc(),
    rppm(),
    uptime()
{
  resource_regeneration = regen_type::DYNAMIC;
  regen_caches[ CACHE_HASTE ] = true;
  regen_caches[ CACHE_SPELL_HASTE ] = true;
}

void evoker_t::init_base_stats()
{
  player_t::init_base_stats();

  base.spell_power_per_intellect = 1.0;

  resources.base[ RESOURCE_ESSENCE ] = 5;
  // TODO: confirm base essence regen. currently estimated at 1 per 5s base
  resources.base_regen_per_second[ RESOURCE_ESSENCE ] = 0.2;
}

void evoker_t::init_spells()
{
  player_t::init_spells();

  auto CT = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::CLASS, n ); };
  auto ST = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::SPECIALIZATION, n ); };
  // Evoker Talents
  // Class Traits
  // Devastation Traits
  talent.ruby_essence_burst     = ST( "Ruby Essence Burst" );
  talent.ruby_embers = ST( "Ruby Embers" );
  talent.engulfing_blaze = ST( "Engulfing Blaze" );
  talent.font_of_magic = ST( "Font of Magic" );
  // Preservation Traits

  // Evoker Specialization Spells
  // Baseline
  spec.evoker = find_spell( 353167 );  // TODO: confirm this is the class aura
  spec.devastation = find_specialization_spell( "Devastation Evoker" );
  spec.preservation = find_specialization_spell( "Preservation Evoker" );
  // Devastation
  // Preservation
}

void evoker_t::create_actions()
{
  player_t::create_actions();
}

void evoker_t::create_buffs()
{
  player_t::create_buffs();

  using namespace buffs;

  // Baseline Abilities
  if ( specialization() == EVOKER_DEVASTATION)
  {
    buff.essence_burst = make_buff( this, "essence_burst", find_spell( 359618 ) );
  }
  else
  {
    buff.essence_burst = make_buff( this, "essence_burst", find_spell( 369299 ) );
  }
  // Class Traits
  // Devastation Traits
  // Preservation Traits
}

void evoker_t::create_options()
{
  player_t::create_options();
}

void evoker_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  option = debug_cast<evoker_t*>( source )->option;
}

void evoker_t::merge( player_t& other )
{
  player_t::merge( other );
}

const evoker_td_t* evoker_t::find_target_data( const player_t* target ) const
{
  return target_data[ target ];
}

evoker_td_t* evoker_t::get_target_data( player_t* target ) const
{
  evoker_td_t*& td = target_data[ target ];
  if ( !td )
    td = new evoker_td_t( target, const_cast<evoker_t*>( this ) );
  return td;
}

void evoker_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  // Baseline Auras
  action.apply_affecting_aura( spec.evoker );
  action.apply_affecting_aura( spec.devastation );
  action.apply_affecting_aura( spec.preservation );

  // Class Traits
  // Devastaion Traits
  // TODO: Coonfirm if this works properly with Scarlet Adaptation
  action.apply_affecting_aura( talent.engulfing_blaze );
  // Preservation Traits
}

action_t* evoker_t::create_action( std::string_view name, std::string_view options_str )
{
  using namespace spells;

  if ( name == "disintegrate" ) return new disintegrate_t( this, options_str );
  if ( name == "fire_breath" ) return new fire_breath_t( this, options_str );
  if ( name == "living_flame" ) return new living_flame_t( this, options_str );

  return player_t::create_action( name, options_str );
}

std::unique_ptr<expr_t> evoker_t::create_expression( std::string_view expr_str )
{
  auto splits = util::string_split<std::string_view>( expr_str, "." );

  if ( util::str_compare_ci( expr_str, "essense" ) || util::str_compare_ci( expr_str, "essences" ) )
    return make_ref_expr( expr_str, resources.current[ RESOURCE_ESSENCE ] );

  return player_t::create_expression( expr_str );
}

// Stat & Multiplier overrides ==============================================

stat_e evoker_t::convert_hybrid_stat( stat_e stat ) const
{
  switch ( stat )
  {
    case STAT_STR_AGI_INT:
    case STAT_AGI_INT:
    case STAT_STR_INT:
      return STAT_INTELLECT;
    case STAT_STR_AGI:
    case STAT_SPIRIT:
    case STAT_BONUS_ARMOR:
      return STAT_NONE;
    default:
      return stat;
  }
}


std::string evoker_t::default_potion() const
{
  return evoker_apl::potion( this );
}

std::string evoker_t::default_flask() const
{
  return evoker_apl::flask( this );
}

std::string evoker_t::default_food() const
{
  return evoker_apl::food( this );
}

std::string evoker_t::default_rune() const
{
  return evoker_apl::rune( this );
}

std::string evoker_t::default_temporary_enchant() const
{
  return evoker_apl::temporary_enchant( this );
}

void evoker_t::init_action_list()
{
  // 2022-08-07: Healing is not supported
  if ( !sim->allow_experimental_specializations && primary_role() == ROLE_HEAL )
  {
    if ( !quiet )
      sim->error( "Role heal for evoker '{}' is currently not supported.", name() );

    quiet = true;
    return;
  }

  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  switch ( specialization() )
  {
    case EVOKER_DEVASTATION:
      evoker_apl::devastation( this );
      break;
    case EVOKER_PRESERVATION:
      evoker_apl::preservation( this );
      break;
    default:
      evoker_apl::no_spec( this );
      break;
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

void evoker_t::init_procs()
{
  proc.ruby_essence_burst = get_proc( "Ruby Essence Burst Procs" );

  player_t::init_procs();
}

double evoker_t::resource_regen_per_second( resource_e resource ) const
{
  auto rrps = player_t::resource_regen_per_second( resource );

  return rrps;
}

// Utility functions ========================================================

// Utility function to search spell data for matching effect.
// NOTE: This will return the FIRST effect that matches parameters.
const spelleffect_data_t* evoker_t::find_spelleffect( const spell_data_t* spell, effect_subtype_t subtype,
                                                      int misc_value, const spell_data_t* affected, effect_type_t type )
{
  for ( size_t i = 1; i <= spell->effect_count(); i++ )
  {
    const auto& eff = spell->effectN( i );

    if ( affected->ok() && !affected->affected_by_all( eff ) )
      continue;

    if ( eff.type() == type && eff.subtype() == subtype )
    {
      if ( misc_value != 0 )
      {
        if ( eff.misc_value1() == misc_value )
          return &eff;
      }
      else
        return &eff;
    }
  }

  return &spelleffect_data_t::nil();
}

// Return the appropriate spell when `base` is overriden to another spell by `passive`
const spell_data_t* evoker_t::find_spell_override( const spell_data_t* base, const spell_data_t* passive )
{
  if ( !passive->ok() )
    return base;

  auto id = as<unsigned>( find_spelleffect( passive, A_OVERRIDE_ACTION_SPELL, base->id() )->base_value() );
  if ( !id )
    return base;

  return find_spell( id );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class evoker_report_t : public player_report_extension_t
{
public:
  evoker_report_t( evoker_t& player ) : p( player ) {}

  void html_customsection( report::sc_html_stream& os ) override {}

private:
  evoker_t& p;
};

// EVOKER MODULE INTERFACE ==================================================
struct evoker_module_t : public module_t
{
  evoker_module_t() : module_t( EVOKER ) {}

  player_t* create_player( sim_t* sim, std::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new evoker_t( sim, name, r );
    p->report_extension = std::make_unique<evoker_report_t>( *p );
    return p;
  }
  bool valid() const override { return true; }
  void init( player_t* p ) const override {}
  void static_init() const override {}
  void register_hotfixes() const override {}
  void combat_begin( sim_t* ) const override {}
  void combat_end( sim_t* ) const override {}
};
}

const module_t* module_t::evoker()
{
  static evoker_module_t m;
  return &m;
}
