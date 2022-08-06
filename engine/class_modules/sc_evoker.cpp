// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"
#include "simulationcraft.hpp"

namespace
{
// ==========================================================================
// Evoker
// ==========================================================================

// Forward declarations
struct evoker_t;

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
  //void init_action_list() override;
  void init_base_stats() override;
  //void init_resources( bool ) override;
  //void init_benefits() override;
  //void init_gains() override;
  //void init_procs() override;
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
  stat_e convert_hybrid_stat( stat_e ) const override;
  void copy_from( player_t* ) override;
  void merge( player_t& ) override;

  double resource_regen_per_second( resource_e ) const override;

  void apply_affecting_auras( action_t& ) override;
  action_t* create_action( std::string_view name, std::string_view options ) override;
  const evoker_td_t* find_target_data( const player_t* target ) const override;
  evoker_td_t* get_target_data( player_t* target ) const override;

  target_specific_t<evoker_td_t> target_data;
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
  evoker_buff_t( evoker_t* player, std::string_view name, const spell_data_t* s_data = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : bb( player, name, s_data, item )
  {}

  evoker_buff_t( evoker_td_t& td, std::string_view name, const spell_data_t* s_data = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : bb( td, name, s_data, item )
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
  evoker_action_t( std::string_view name, evoker_t* player, const spell_data_t* s_data = spell_data_t::nil() )
    : ab( name, player, s_data )
  {

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
struct evoker_spell_t : public evoker_action_t<spell_t>
{
private:
  using ab = evoker_action_t<spell_t>;

public:
  evoker_spell_t( std::string_view name, evoker_t* player, const spell_data_t* s_data = spell_data_t::nil(),
                  std::string_view options_str = {} )
    : ab( name, player, s_data )
  {
    parse_options( options_str );
  }
};

struct disintegrate_t : public evoker_spell_t
{
  disintegrate_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "disintegrate", p, p->find_class_spell( "Disintegrate" ) )
  {
    channeled = true;
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

  // Evoker Talents
  // Class Traits
  // Devastation Traits
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
  // Class Traits
  // Devastation Traits
  // Preservation Traits
}

void evoker_t::create_options()
{
  player_t::create_options();
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
  // Preservation Traits
}

action_t* evoker_t::create_action( std::string_view name, std::string_view options_str )
{
  using namespace spells;

  if ( name == "disintegrate" ) return new disintegrate_t( this, options_str );

  return player_t::create_action( name, options_str );
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

stat_e evoker_t::convert_hybrid_stat( stat_e s ) const
{
  switch ( s )
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
      return s;
  }
}

double evoker_t::resource_regen_per_second( resource_e r ) const
{
  auto rrps = player_t::resource_regen_per_second( r );

  return rrps;
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
