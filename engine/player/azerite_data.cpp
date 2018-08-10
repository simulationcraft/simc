#include "azerite_data.hpp"

#include "simulationcraft.hpp"

azerite_power_t::azerite_power_t() :
  m_player( nullptr ), m_spell( spell_data_t::not_found() )
{ }

azerite_power_t::azerite_power_t( const player_t* p, const spell_data_t* spell, const std::vector<const item_t*>& items ) :
  m_player( p ), m_spell( spell )
{
  range::for_each( items, [ this ]( const item_t* item ) {
    m_ilevels.push_back( item -> item_level() );
  } );
}

azerite_power_t::azerite_power_t( const player_t* p, const spell_data_t* spell, const std::vector<unsigned>& ilevels ) :
  m_player( p ), m_spell( spell ), m_ilevels( ilevels )
{ }

azerite_power_t::operator const spell_data_t*() const
{ return m_spell; }

const spell_data_t* azerite_power_t::spell() const
{ return m_spell; }

const spell_data_t& azerite_power_t::spell_ref() const
{ return *m_spell; }

bool azerite_power_t::ok() const
{ return m_spell -> ok(); }

bool azerite_power_t::enabled() const
{ return ok(); }

double azerite_power_t::value( size_t index ) const
{
  if ( index > m_spell -> effect_count() || index == 0 )
  {
    return 0.0;
  }

  if ( m_value.size() >= index &&
       m_value[ index - 1 ] != std::numeric_limits<double>::lowest() )
  {
    return m_value[ index - 1 ];
  }

  double sum = 0.0;
  auto budgets = budget();
  for ( size_t budget_index = 0, end = budgets.size(); budget_index < end; ++budget_index )
  {
    auto budget = budgets[ budget_index ];
    auto value = floor( m_spell -> effectN( index ).m_coefficient() * budget + 0.5 );

    unsigned actual_level = m_ilevels[ budget_index ];
    if ( m_spell->max_scaling_level() > 0 && m_spell->max_scaling_level() < actual_level )
    {
      actual_level = m_spell -> max_scaling_level();
    }

    // Apply combat rating penalties consistently
    if ( m_spell -> scaling_class() == PLAYER_SPECIAL_SCALE7 ||
         check_combat_rating_penalty( index ) )
    {
      value = item_database::apply_combat_rating_multiplier( m_player,
          CR_MULTIPLIER_ARMOR, actual_level, value );
    }

    // TODO: Is this floored, or allowed to accumulate with fractions (and floored at the end?)
    sum += floor( value );
  }

  if ( m_value.size() < index )
  {
    m_value.resize( index, std::numeric_limits<double>::lowest() );
  }

  m_value[ index - 1 ] = sum;

  return sum;
}

double azerite_power_t::value( const azerite_value_fn_t& fn ) const
{
  return fn( *this );
}

double azerite_power_t::percent( size_t index ) const
{ return value( index ) * .01; }

timespan_t azerite_power_t::time_value( size_t index, time_type tt ) const
{
  switch ( tt )
  {
    case time_type::S:
      return timespan_t::from_seconds( value( index ) );
    case time_type::MS:
      return timespan_t::from_millis( value( index ) );
    // Should be never reached
    default:
      return timespan_t::zero();
  }
}

std::vector<double> azerite_power_t::budget() const
{ return budget( m_spell ); }

std::vector<double> azerite_power_t::budget( const spell_data_t* scaling_spell ) const
{
  std::vector<double> b;

  if ( m_player == nullptr )
  {
    return {};
  }

  range::for_each( m_ilevels, [ & ]( unsigned ilevel ) {
    unsigned min_ilevel = ilevel;
    if ( scaling_spell->max_scaling_level() > 0 && scaling_spell->max_scaling_level() < min_ilevel )
    {
      min_ilevel = scaling_spell->max_scaling_level();
    }

    auto budget = item_database::item_budget( m_player, min_ilevel );
    if ( scaling_spell->scaling_class() == PLAYER_SPECIAL_SCALE8 )
    {
      const auto& props = m_player->dbc.random_property( min_ilevel );
      budget = props.damage_replace_stat;
    }
    b.push_back( budget );
  } );

  return b;
}

const std::vector<unsigned> azerite_power_t::ilevels() const
{ return m_ilevels; }

unsigned azerite_power_t::n_items() const
{ return as<unsigned>( m_ilevels.size() ); }

// Currently understood rules for the detection, since we don't know how to see it from spell data
// 1) Spell must use scaling type -1
// 2) Effect must be of type "apply combat rating"
// 3) Effect must be a dynamically scaling one (average coefficient > 0)
bool azerite_power_t::check_combat_rating_penalty( size_t index ) const
{
  if ( index == 0 || index > m_spell->effect_count() )
  {
    return false;
  }

  if ( m_spell->scaling_class() != PLAYER_SPECIAL_SCALE )
  {
    return false;
  }

  return m_spell->effectN( index ).subtype() == A_MOD_RATING &&
         m_spell->effectN( index ).m_coefficient() != 0;
}

namespace azerite
{
std::unique_ptr<azerite_state_t> create_state( player_t* p )
{
  return std::unique_ptr<azerite_state_t>( new azerite_state_t( p ) );
}

void initialize_azerite_powers( player_t* actor )
{
  if ( ! actor -> azerite )
  {
    return;
  }

  for ( auto azerite_spell : actor -> azerite -> enabled_spells() )
  {
    const auto spell = actor -> find_spell( azerite_spell );
    if ( ! spell -> ok() )
    {
      continue;
    }

    special_effect_t effect { actor };
    effect.source = SPECIAL_EFFECT_SOURCE_AZERITE;

    unique_gear::initialize_special_effect( effect, azerite_spell );
    // Note, only apply custom special effects for azerite for an abundance of safety
    if ( ! effect.is_custom() )
    {
      continue;
    }

    actor -> special_effects.push_back( new special_effect_t( effect ) );
  }
}

azerite_state_t::azerite_state_t( player_t* p ) : m_player( p )
{ }

void azerite_state_t::initialize()
{
  range::for_each( m_player -> items, [ this ]( const item_t& item ) {
    range::for_each( item.parsed.azerite_ids, [ & ]( unsigned id ) {
      m_items[ id ].push_back( &( item ) );
    } );
  });
}

azerite_power_t azerite_state_t::get_power( unsigned id )
{
  // All azerite disabled
  if ( m_player -> sim -> azerite_status == AZERITE_DISABLED_ALL )
  {
    return {};
  }

  const auto& power = m_player -> dbc.azerite_power( id );

  auto override_it = m_overrides.find( id );
  // Found an override, use the overridden ilevels instead
  if ( override_it != m_overrides.end() )
  {
    // Explicitly disabled, in this case there's only going to be a single ilevel entry in the
    // overrides, and it will alwyas be 0
    if ( override_it -> second[ 0 ] == 0 )
    {
      if ( m_player -> sim -> debug )
      {
        m_player -> sim -> out_debug.printf( "%s disabling overridden azerite power %s",
            m_player -> name(), power.name );
      }

      return {};
    }
    else
    {
      if ( m_player -> sim -> debug )
      {
        std::stringstream s;
        for ( size_t i = 0; i < override_it -> second.size(); ++i )
        {
          s << override_it -> second[ i ];

          if ( i < override_it -> second.size() - 1 )
          {
            s << ", ";
          }
        }

        m_player -> sim -> out_debug.printf( "%s initializing overridden azerite power %s: "
                                             "ilevels=%s",
            m_player -> name(), power.name, s.str().c_str() );
      }

      return { m_player, m_player -> find_spell( power.spell_id ), override_it -> second };
    }
  }
  else
  {
    // Item-related azerite effects are only enabled when "all" is defined
    if ( m_items[ id ].size() > 0 && m_player -> sim -> azerite_status == AZERITE_ENABLED )
    {
      if ( m_player -> sim -> debug )
      {
        std::stringstream s;
        const auto& items = m_items[ power.id ];

        for ( size_t i = 0; i < items.size(); ++i )
        {
          s << items[ i ] -> full_name()
            << " (id=" << items[ i ] -> parsed.data.id
            << ", ilevel=" << items[ i ] -> item_level() << ")";

          if ( i < items.size() - 1 )
          {
            s << ", ";
          }
        }

        m_player -> sim -> out_debug.printf( "%s initializing azerite power %s: %s",
            m_player -> name(), power.name, s.str().c_str() );
      }

      return { m_player, m_player -> find_spell( power.spell_id ), m_items[ id ] };
    }
    else
    {
      return {};
    }
  }
}

azerite_power_t azerite_state_t::get_power( const std::string& name, bool tokenized )
{
  const auto& power = m_player -> dbc.azerite_power( name, tokenized );

  if ( power.id == 0 )
  {
    return {};
  }

  return get_power( power.id );
}

void azerite_state_t::copy_overrides( const std::unique_ptr<azerite_state_t>& other )
{
  if ( other.get() == this )
  {
    return;
  }

  m_overrides = other -> m_overrides;
}

std::string azerite_state_t::overrides_str() const
{
  std::vector<std::string> override_strings;

  // Always normalize the user-given azerite_override string into a specific (power id) order
  std::vector<unsigned> powers;
  for ( const auto& e : m_overrides )
  {
    powers.push_back( e.first );
  }

  range::sort( powers );

  // Iterate over the powers in order, grab all overrides into a vector
  range::for_each( powers, [ & ]( unsigned power_id ) {
      const auto& power = m_player -> dbc.azerite_power( power_id );
      std::string power_name = power.name;
      util::tokenize( power_name );

      const auto& overrides = m_overrides.at( power_id );
      range::for_each( overrides, [ & ]( unsigned ilevel ) {
        override_strings.push_back( power_name + ":" + util::to_string( ilevel ) );
      } );
  } );

  if ( override_strings.size() == 0 )
  {
    return {};
  }

  std::stringstream s;

  // Construct an overall azerite override string
  for ( size_t i = 0; i < override_strings.size(); ++i )
  {
    s << override_strings[ i ];
    if ( i < override_strings.size() - 1 )
    {
      s << "/";
    }
  }

  return s.str();
}

bool azerite_state_t::parse_override( sim_t* sim, const std::string&, const std::string& value )
{
  m_overrides.clear();

  auto override_split = util::string_split( value, "/" );
  for ( const auto& opt : override_split )
  {
    if ( opt.empty() )
    {
      continue;
    }

    auto opt_split = util::string_split( opt, ":" );
    if ( opt_split.size() != 2 )
    {
      sim -> errorf( "%s unknown azerite override string \"%s\"", sim -> active_player -> name(),
          opt.c_str());
      return false;
    }

    std::string power_str = opt_split[ 0 ];
    unsigned ilevel = util::to_unsigned( opt_split[ 1 ] );

    if ( ilevel > MAX_ILEVEL )
    {
      sim -> errorf( "%s too high ilevel %u for azerite override of \"%s\"",
          sim -> active_player -> name(), ilevel, power_str );
      return false;
    }

    const auto* power = &( m_player->dbc.azerite_power( power_str, true ) );
    // Additionally try with an azerite power id if the name-based lookup fails
    if ( power->id == 0 )
    {
      unsigned power_id = util::to_unsigned( opt_split[ 0 ] );
      if ( power_id > 0 )
      {
        power = &( m_player->dbc.azerite_power( power_id ) );
      }
    }

    if ( power->id == 0 )
    {
      sim -> errorf( "%s unknown azerite power \"%s\" for override",
          sim -> active_player -> name(), power_str );
      return false;
    }

    // Ilevel 0 will disable the azerite power fully, no matter its position in the override
    // string
    if ( ilevel == 0 )
    {
      m_overrides[ power->id ] = { 0 };
    }
    else
    {
      // Make sure there's no existing zero ilevel value for the overrides
      if ( m_overrides[ power->id ].size() == 1 && m_overrides[ power->id ][ 0 ] == 0 )
      {
        continue;
      }

      m_overrides[ power->id ].push_back( ilevel );
    }
  }

  return true;
}

size_t azerite_state_t::rank( unsigned id ) const
{
  // All azerite-related effects disabled
  if ( m_player -> sim -> azerite_status == AZERITE_DISABLED_ALL )
  {
    return 0u;
  }

  auto it = m_overrides.find( id );
  // Override found, figure enable status from it
  if ( it != m_overrides.end() )
  {
    if ( it -> second.size() == 1 && it -> second[ 0 ] == 0 )
    {
      return 0u;
    }

    return it -> second.size();
  }

  // Only look at item-based azerite, if all azerite is enabled
  if ( m_player -> sim -> azerite_status == AZERITE_ENABLED )
  {
    auto it = m_items.find( id );
    if ( it != m_items.end() )
    {
      return it -> second.size();
    }
  }

  // Out of options, it can't be enabled
  return 0u;
}

size_t azerite_state_t::rank( const std::string& name, bool tokenized ) const
{
  const auto& power = m_player -> dbc.azerite_power( name, tokenized );
  if ( power.id == 0 )
  {
    return 0u;
  }

  return rank( power.id );
}

bool azerite_state_t::is_enabled( unsigned id ) const
{
  return rank( id ) > 0;
}

bool azerite_state_t::is_enabled( const std::string& name, bool tokenized ) const
{
  const auto& power = m_player -> dbc.azerite_power( name, tokenized );
  if ( power.id == 0 )
  {
    return false;
  }

  return is_enabled( power.id );
}

expr_t* azerite_state_t::create_expression( const std::vector<std::string>& expr_str ) const
{
  if ( expr_str.size() == 1 )
  {
    return nullptr;
  }

  const auto& power = m_player -> dbc.azerite_power( expr_str[ 1 ], true );
  if ( power.id == 0 )
  {
    throw std::invalid_argument( fmt::format( "Unknown azerite power '{}'.", expr_str[ 1 ] ) );
  }

  if ( util::str_compare_ci( expr_str[ 2 ], "enabled" ) )
  {
    return expr_t::create_constant( "azerite_enabled", as<double>( is_enabled( power.id ) ) );
  }
  else if ( util::str_compare_ci( expr_str[ 2 ], "rank" ) )
  {
    return expr_t::create_constant( "azerite_rank", as<double>( rank( power.id ) ) );
  }
  else
  {
    throw std::invalid_argument( fmt::format( "Unsupported azerite expression '{}'.", expr_str[ 2 ] ) );
  }

  return nullptr;
}

std::vector<unsigned> azerite_state_t::enabled_spells() const
{
  std::vector<unsigned> spells;

  for ( const auto& entry : m_items )
  {
    if ( ! is_enabled( entry.first ) )
    {
      continue;
    }

    const auto& power = m_player -> dbc.azerite_power( entry.first );
    if ( power.id == 0 )
    {
      continue;
    }

    spells.push_back( power.spell_id );
  }

  for ( const auto& entry : m_overrides )
  {
    if ( ! is_enabled( entry.first ) )
    {
      continue;
    }

    const auto& power = m_player -> dbc.azerite_power( entry.first );
    if ( power.id == 0 )
    {
      continue;
    }

    spells.push_back( power.spell_id );
  }

  range::unique( spells );

  return spells;
}

void register_azerite_powers()
{
  unique_gear::register_special_effect( 263962, special_effects::resounding_protection );
  unique_gear::register_special_effect( 263984, special_effects::elemental_whirl       );
  unique_gear::register_special_effect( 264108, special_effects::blood_siphon          );
  unique_gear::register_special_effect( 267665, special_effects::lifespeed             );
  unique_gear::register_special_effect( 267879, special_effects::on_my_way             );
  unique_gear::register_special_effect( 280710, special_effects::champion_of_azeroth   );
  unique_gear::register_special_effect( 279899, special_effects::unstable_flames       );
  unique_gear::register_special_effect( 280380, special_effects::thunderous_blast      );
  unique_gear::register_special_effect( 273834, special_effects::filthy_transfusion    );
  unique_gear::register_special_effect( 280407, special_effects::blood_rite            );
  unique_gear::register_special_effect( 280402, special_effects::tidal_surge           );
  unique_gear::register_special_effect( 263987, special_effects::heed_my_call          );
  unique_gear::register_special_effect( 266936, special_effects::azerite_globules      );
  unique_gear::register_special_effect( 266180, special_effects::overwhelming_power    );
  unique_gear::register_special_effect( 279926, special_effects::earthlink             );
  unique_gear::register_special_effect( 273823, special_effects::wandering_soul        ); // Blightborne Infusion
  unique_gear::register_special_effect( 273150, special_effects::wandering_soul        ); // Ruinous Bolt
  unique_gear::register_special_effect( 280429, special_effects::swirling_sands        );
  unique_gear::register_special_effect( 280579, special_effects::retaliatory_fury      ); // Retaliatory Fury
  unique_gear::register_special_effect( 280624, special_effects::retaliatory_fury      ); // Last Gift
  unique_gear::register_special_effect( 280577, special_effects::glory_in_battle       ); // Glory In Battle
  unique_gear::register_special_effect( 280623, special_effects::glory_in_battle       ); // Liberator's Might
  unique_gear::register_special_effect( 280598, special_effects::sylvanas_resolve      ); // Sylvanas' Resolve
  unique_gear::register_special_effect( 280628, special_effects::sylvanas_resolve      ); // Anduin's Determination
  unique_gear::register_special_effect( 281841, special_effects::tradewinds            );
  unique_gear::register_special_effect( 281514, special_effects::unstable_catalyst     );
  unique_gear::register_special_effect( 280626, special_effects::stand_as_one          );  // Stand as One
  unique_gear::register_special_effect( 280581, special_effects::stand_as_one          );  // CollectiveWill
  unique_gear::register_special_effect( 280555, special_effects::archive_of_the_titans );
  unique_gear::register_special_effect( 280559, special_effects::laser_matrix          );
  unique_gear::register_special_effect( 280410, special_effects::incite_the_pack       );
  unique_gear::register_special_effect( 280284, special_effects::dagger_in_the_back    );
  unique_gear::register_special_effect( 273790, special_effects::rezans_fury           );
  unique_gear::register_special_effect( 273829, special_effects::secrets_of_the_deep   );
  unique_gear::register_special_effect( 280580, special_effects::combined_might        );
  unique_gear::register_special_effect( 280178, special_effects::relational_normalization_gizmo );
  unique_gear::register_special_effect( 280163, special_effects::barrage_of_many_bombs );
}


void register_azerite_target_data_initializers( sim_t* sim )
{
  // Azerite Globules
  sim -> register_target_data_initializer( [] ( actor_target_data_t* td ) {
    auto&& azerite = td -> source -> azerite;
    if ( azerite && azerite -> is_enabled( "Azerite Globules" ) )
    {
      assert( !td -> debuff.azerite_globules );

      td -> debuff.azerite_globules = make_buff( *td, "azerite_globules", td -> source -> find_spell( 279956 ) );
      td -> debuff.azerite_globules -> reset();
    }
    else
    {
      td -> debuff.azerite_globules = make_buff( *td, "azerite_globules" );
      td -> debuff.azerite_globules -> set_quiet( true );
    }
  } );
}

std::tuple<int, int, int > compute_value( const azerite_power_t& power, const spelleffect_data_t& effect )
{
  int min_ = 0, max_ = 0, avg_ = 0;
  if ( !power.enabled() || effect.m_coefficient() == 0 )
  {
    return { 0, 0, 0 };
  }

  auto budgets = power.budget( effect.spell() );
  range::for_each( budgets, [&]( double budget ) {
    avg_ += static_cast<int>( budget * effect.m_coefficient() + 0.5 );
    min_ += static_cast<int>( budget * effect.m_coefficient() *
        ( 1.0 - effect.m_coefficient() / 2 ) + 0.5 );
    max_ += static_cast<int>( budget * effect.m_coefficient() *
        ( 1.0 + effect.m_coefficient() / 2 ) + 0.5 );
  } );

  return { min_, avg_, max_ };
}

} // Namespace azerite ends

namespace azerite
{
namespace special_effects
{

static std::string tokenized_name( const spell_data_t* spell )
{
  return ::util::tokenize_fn( spell -> name_cstr() );
}

void resounding_protection( special_effect_t& effect )
{
  class rp_event_t : public event_t
  {
    buff_t*    buff;
    timespan_t period;

public:
    rp_event_t( buff_t* b, const timespan_t& t ) :
      event_t( *b -> source, t ), buff( b ), period( t )
    { }

    void execute() override
    {
      buff -> trigger();
      make_event<rp_event_t>( sim(), buff, period );
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( ! power.enabled() )
  {
    return;
  }

  const spell_data_t* driver = effect.player -> find_spell( 270568 );
  const spell_data_t* absorb = effect.player -> find_spell( 269279 );
  double amount = power.value();
  buff_t* buff = make_buff<absorb_buff_t>( effect.player, "resounding_protection", absorb )
                 -> set_default_value( amount );

  effect.player -> register_combat_begin( [ buff, driver ]( player_t* ) {
    buff -> trigger();
    make_event<rp_event_t>( *buff -> sim, buff, driver -> effectN( 1 ).period() );
  } );
}

void elemental_whirl( special_effect_t& effect )
{
  class ew_proc_cb_t : public dbc_proc_callback_t
  {
    std::vector<buff_t*> buffs;

    public:
    ew_proc_cb_t( const special_effect_t& effect, const std::vector<buff_t*>& b ) :
      dbc_proc_callback_t( effect.player, effect ), buffs( b )
    { }

    void execute( action_t* /* a */, action_state_t* /* state */ ) override
    {
      size_t buff_index = rng().range( 0.0, as<double>( buffs.size() ) );
      buffs[ buff_index ] -> trigger();
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( ! power.enabled() )
  {
    return;
  }

  double amount = power.value();
  const spell_data_t* driver = effect.player -> find_spell( 270667 );
  const spell_data_t* crit_spell = effect.player -> find_spell( 268953 );
  const spell_data_t* hast_spell = effect.player -> find_spell( 268954 );
  const spell_data_t* mast_spell = effect.player -> find_spell( 268955 );
  const spell_data_t* vers_spell = effect.player -> find_spell( 268956 );

  buff_t* crit = make_buff<stat_buff_t>( effect.player, "elemental_whirl_crit", crit_spell )
    -> add_stat( STAT_CRIT_RATING, amount );
  buff_t* versatility = make_buff<stat_buff_t>( effect.player, "elemental_whirl_versatility", vers_spell )
    -> add_stat( STAT_VERSATILITY_RATING, amount );
  buff_t* mastery = make_buff<stat_buff_t>( effect.player, "elemental_whirl_mastery", mast_spell )
    -> add_stat( STAT_MASTERY_RATING, amount );
  buff_t* haste = make_buff<stat_buff_t>( effect.player, "elemental_whirl_haste", hast_spell )
    -> add_stat( STAT_HASTE_RATING, amount );

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver -> id();

  new ew_proc_cb_t( effect, { crit, haste, mastery, versatility } );
}

void blood_siphon( special_effect_t& effect )
{
  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( ! power.enabled() )
  {
    return;
  }

  effect.player -> passive.mastery_rating += power.value( 1 );
  effect.player -> passive.leech_rating += power.value( 2 );
}

void lifespeed(special_effect_t& effect)
{
  azerite_power_t power = effect.player->find_azerite_spell(effect.driver()->name_cstr());
  if (!power.enabled())
  {
    return;
  }

  effect.player->passive.haste_rating += power.value(1);
  effect.player->passive.avoidance_rating += power.value(2);
}

void on_my_way(special_effect_t& effect)
{
  azerite_power_t power = effect.player->find_azerite_spell(effect.driver()->name_cstr());
  if (!power.enabled())
  {
    return;
  }

  effect.player->passive.versatility_rating += power.value(1);
  effect.player->passive.speed_rating += power.value(2);
}

void champion_of_azeroth( special_effect_t& effect )
{
  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( ! power.enabled() )
    return;

  const double amount = power.value();
  const spell_data_t* driver = effect.player -> find_spell( 280712 );
  const spell_data_t* spell = effect.player -> find_spell( 280713 );

  effect.custom_buff = buff_t::find( effect.player, "champion_of_azeroth" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, "champion_of_azeroth", spell )
      -> add_stat( STAT_CRIT_RATING, amount )
      -> add_stat( STAT_VERSATILITY_RATING, amount )
      -> add_stat( STAT_MASTERY_RATING, amount )
      -> add_stat( STAT_HASTE_RATING, amount );
  }

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void unstable_flames( special_effect_t& effect )
{
  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( ! power.enabled() )
    return;

  const double amount = power.value();
  const spell_data_t* driver = effect.player -> find_spell( 279900 );
  const spell_data_t* spell = effect.player -> find_spell( 279902 );

  effect.custom_buff = buff_t::find( effect.player, "unstable_flames" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, "unstable_flames", spell )
      -> add_stat( STAT_CRIT_RATING, amount );
  }

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void thunderous_blast( special_effect_t& effect )
{
  struct thunderous_blast_t : public unique_gear::proc_spell_t
  {
    buff_t* building_pressure = nullptr;
    buff_t* rolling_thunder = nullptr;

    thunderous_blast_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "thunderous_blast", e.player, e.player -> find_spell( 280384 ) )
    {
      base_dd_min = base_dd_max = power.value( 1 );

      building_pressure = buff_t::find( e.player, "building_pressure" );
      if ( !building_pressure )
      {
        building_pressure =
          make_buff( e.player, "building_pressure", data().effectN( 3 ).trigger() )
            -> set_default_value( data().effectN( 3 ).trigger() -> effectN( 1 ).percent() );
      }

      rolling_thunder = buff_t::find( e.player, "rolling_thunder" );
      if ( !rolling_thunder )
      {
        rolling_thunder =
          make_buff( e.player, "rolling_thunder", e.player -> find_spell( 280400 ) )
            -> set_default_value( e.player -> find_spell( 280400 ) -> effectN( 1 ).percent() );
      }
    }

    void execute() override
    {
      proc_spell_t::execute();

      if ( building_pressure -> check() == building_pressure -> max_stack() )
      {
        building_pressure -> expire();
        rolling_thunder -> trigger();
      }
      else
      {
        rolling_thunder -> expire();
        building_pressure -> trigger();
      }
    }

    double action_multiplier() const override
    {
      double am = proc_spell_t::action_multiplier();

      am *= 1.0 + building_pressure -> check_stack_value();
      am *= 1.0 + rolling_thunder -> check_value();

      return am;
    }

    double composite_crit_chance() const override
    {
      double cc = proc_spell_t::composite_crit_chance();

      if ( rolling_thunder -> check() )
        cc += 1.0;

      return cc;
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( ! power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<thunderous_blast_t>( "thunderous_blast", effect, power );
  effect.spell_id = effect.player -> find_spell( 280383 ) -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void filthy_transfusion( special_effect_t& effect )
{
  struct filthy_transfusion_t : public unique_gear::proc_spell_t
  {
    filthy_transfusion_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "filthy_transfusion", e.player, e.player -> find_spell( 273836 ) )
    {
      base_td = power.value( 1 );
      tick_may_crit = false;
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( ! power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<filthy_transfusion_t>( "filthy_transfusion", effect, power );
  effect.spell_id = effect.player -> find_spell( 273835 ) -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void retaliatory_fury( special_effect_t& effect )
{
  class retaliatory_fury_proc_cb_t : public dbc_proc_callback_t
  {
    std::array<buff_t*, 2> buffs;
  public:
    retaliatory_fury_proc_cb_t( const special_effect_t& effect, std::array<buff_t*, 2> b ) :
      dbc_proc_callback_t( effect.player, effect ), buffs( b )
    {}

    void execute( action_t*, action_state_t* ) override
    {
      for ( auto b : buffs )
        b -> trigger();
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();
  const spell_data_t* mastery_spell = effect.player -> find_spell( power.spell_ref().id() == 280624 ? 280861 : 280787 );
  const spell_data_t* absorb_spell = driver -> effectN( 1 ).trigger();

  buff_t* mastery = buff_t::find( effect.player, tokenized_name( mastery_spell ) );
  if ( !mastery )
  {
    mastery = make_buff<stat_buff_t>( effect.player, tokenized_name( mastery_spell ), mastery_spell )
      -> add_stat( STAT_MASTERY_RATING, power.value( 1 ) );
  }

  buff_t* absorb = buff_t::find( effect.player, tokenized_name( absorb_spell ) + "_absorb" );
  if ( !absorb )
  {
    absorb = make_buff<absorb_buff_t>( effect.player, tokenized_name( absorb_spell ) + "_absorb", absorb_spell )
      -> set_absorb_source( effect.player -> get_stats( tokenized_name( absorb_spell ) ) )
      -> set_default_value( power.value( 2 ) );
  }

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver -> id();

  // 07-08-2018: this seems to have 2 rppm in pve
  if ( ! effect.player -> sim -> pvp_crit )
    effect.ppm_ = -2.0;

  new retaliatory_fury_proc_cb_t( effect, { { mastery, absorb } } );
}

void blood_rite( special_effect_t& effect )
{
  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = effect.player -> find_spell( 280408 );
  const spell_data_t* spell = effect.player -> find_spell( 280409 );

  effect.custom_buff = buff_t::find( effect.player, "blood_rite" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, "blood_rite", spell )
      -> add_stat( STAT_HASTE_RATING, power.value( 1 ) );
  }

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver -> id();

  // TODO: add "Killing an enemy will refresh this effect." part
  // ideally something generic so we can model all "on kill" effects

  new dbc_proc_callback_t( effect.player, effect );
}

void glory_in_battle( special_effect_t& effect )
{
  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();

  effect.custom_buff = buff_t::find( effect.player, tokenized_name( spell ) );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, tokenized_name( spell ), spell )
      -> add_stat( STAT_CRIT_RATING, power.value( 1 ) )
      -> add_stat( STAT_HASTE_RATING, power.value( 2 ) );
  }

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver -> id();

  // 07-08-2018: this seems to have 2 rppm in pve
  if ( ! effect.player -> sim -> pvp_crit )
    effect.ppm_ = -2.0;

  new dbc_proc_callback_t( effect.player, effect );
}

void tradewinds( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();
  const spell_data_t* spell  = driver->effectN( 1 ).trigger();

  effect.custom_buff = buff_t::find( effect.player, tokenized_name( spell ) );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, tokenized_name( spell ), spell )
                             ->add_stat( STAT_MASTERY_RATING, power.value( 1 ) );
  }

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver->id();

  // TODO add the "jump to other player" effect
  // TODO check RPPM

  new dbc_proc_callback_t( effect.player, effect );
}

void unstable_catalyst( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();
  const spell_data_t* spell  = driver->effectN( 1 ).trigger();

  effect.custom_buff = buff_t::find( effect.player, tokenized_name( spell ) );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, tokenized_name( spell ), spell )
                             ->add_stat( effect.player->primary_stat(), power.value( 1 ) );
  }

  // TODO assumes the player always stands in the pool - might be unrealistic

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver->id();

  new dbc_proc_callback_t( effect.player, effect );
}

void stand_as_one( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();
  const spell_data_t* spell  = driver->effectN( 1 ).trigger();

  effect.custom_buff = buff_t::find( effect.player, tokenized_name( spell ) );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, tokenized_name( spell ), spell )
                             ->add_stat( effect.player->primary_stat(), power.value( 1 ) )
                             ->add_stat( STAT_MAX_HEALTH, power.value( 2 ) );
  }

  // TODO give to 4 other actors?

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver->id();

  new dbc_proc_callback_t( effect.player, effect );
}

void archive_of_the_titans( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();
  const spell_data_t* spell  = effect.player -> find_spell( 280709 );

  buff_t* buff = buff_t::find( effect.player, tokenized_name( spell ) );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, tokenized_name( spell ), spell )
      ->add_stat( effect.player->primary_stat(), power.value( 1 ) );
  }

  effect.player->register_combat_begin( [ buff, driver ]( player_t* ) {
    buff -> trigger();
    make_repeating_event( *buff -> sim, driver -> effectN( 1 ).period(), [ buff ]() {
      buff -> trigger();
    } );
    // TODO Proc Reorigination Array
  } );
}

void laser_matrix( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();

  struct laser_matrix_t : public unique_gear::proc_spell_t
  {
    laser_matrix_t( const special_effect_t& e, const azerite_power_t& power )
      : proc_spell_t( "laser_matrix", e.player, e.player->find_spell( 280705 ) )
    {
      base_dd_min = base_dd_max = power.value( 1 );
      aoe = -1;
      split_aoe_damage = 1;
      radius = e.player->find_spell( 280703 )->effectN( 1 ).radius();
    }
    // TODO: travel_time?
  };

  effect.execute_action = unique_gear::create_proc_action<laser_matrix_t>( "laser_matrix", effect, power );
  effect.spell_id       = driver->id();

  new dbc_proc_callback_t( effect.player, effect );
}

void incite_the_pack( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();
  const spell_data_t* spell  = driver->effectN( 1 ).trigger();

  effect.custom_buff = buff_t::find( effect.player, tokenized_name( spell ) );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, tokenized_name( spell ), spell )
                             ->add_stat( STAT_MASTERY_RATING, power.value( 1 ) );
  }

  // TODO buff 2 allies with power.value( 2 )?

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver->id();

  new dbc_proc_callback_t( effect.player, effect );
}

void sylvanas_resolve( special_effect_t& effect )
{
  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();

  effect.custom_buff = buff_t::find( effect.player, tokenized_name( spell ) );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, tokenized_name( spell ), spell )
      -> add_stat( effect.player -> primary_stat(), power.value( 1 ) );
  }

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void tidal_surge( special_effect_t& effect )
{
  struct tidal_surge_t : public unique_gear::proc_spell_t
  {
    tidal_surge_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "tidal_surge", e.player, e.player -> find_spell( 280404 ) )
    {
      base_dd_min = base_dd_max = power.value( 1 );
    }
    // TODO: verify travel_time
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<tidal_surge_t>( "tidal_surge", effect, power );
  effect.spell_id = effect.player -> find_spell( 280403 ) -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void heed_my_call( special_effect_t& effect )
{
  struct heed_my_call_aoe_t : public unique_gear::proc_spell_t
  {
    heed_my_call_aoe_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "heed_my_call_aoe", e.player, e.player -> find_spell( 271686 ) )
    {
      base_dd_min = base_dd_max = power.value( 2 );
    }
  };
  struct heed_my_call_t : public unique_gear::proc_spell_t
  {
    heed_my_call_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "heed_my_call", e.player, e.player -> find_spell( 271685 ) )
    {
      base_dd_min = base_dd_max = power.value( 1 );
      execute_action = new heed_my_call_aoe_t( e, power );
      add_child( execute_action );
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<heed_my_call_t>( "heed_my_call", effect, power );
  effect.spell_id = effect.player -> find_spell( 271681 ) -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void azerite_globules( special_effect_t& effect )
{
  struct azerite_globules_t : public unique_gear::proc_spell_t
  {
    azerite_globules_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "azerite_globules", e.player, e.player -> find_spell( 279958 ) )
    {
      base_dd_min = base_dd_max = power.value( 1 );
    }
  };

  class azerite_globules_proc_cb_t : public dbc_proc_callback_t
  {
  public:
    azerite_globules_proc_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.player, effect )
    {}

    void execute( action_t* a, action_state_t* s ) override
    {
      auto td = a -> player -> get_target_data( s -> target );
      assert( td );
      auto debuff = td -> debuff.azerite_globules;
      assert( debuff );

      debuff -> trigger();
      if ( debuff -> stack() == debuff -> max_stack() )
      {
        debuff -> expire();
        proc_action -> set_target( s -> target );
        proc_action -> execute();
      }
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<azerite_globules_t>( "azerite_globules", effect, power );
  effect.spell_id = effect.player -> find_spell( 279955 ) -> id();

  new azerite_globules_proc_cb_t( effect );
}

void overwhelming_power( special_effect_t& effect )
{
  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = effect.player -> find_spell( 271705 );
  const spell_data_t* spell = effect.player -> find_spell( 271711 );

  buff_t* buff = buff_t::find( effect.player, "overwhelming_power" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "overwhelming_power", spell )
      -> add_stat( STAT_HASTE_RATING, power.value( 1 ) )
      -> set_reverse( true );
  }

  effect.custom_buff = buff;
  effect.spell_id = driver -> id();

  new dbc_proc_callback_t( effect.player, effect );

  // TODO: add on damage taken mechanic
  effect.player -> register_combat_begin( [ buff, driver ]( player_t* ) {
    make_repeating_event( *buff -> sim, driver -> effectN( 2 ).period(), [ buff ]() {
      buff -> decrement();
    } );
  } );
}

void earthlink( special_effect_t& effect )
{
  struct earthlink_t : public stat_buff_t
  {
    earthlink_t( player_t* p ) :
      stat_buff_t( p, "earthlink", p -> find_spell( 279928 ) )
    {}

    void bump( int stacks, double value ) override
    {
      if ( check() == max_stack() )
        reverse = true;
      else
        stat_buff_t::bump( stacks, value );
    }

    void decrement( int stacks, double value ) override
    {
      if ( check() == 1 )
        reverse = false;
      else
        stat_buff_t::decrement( stacks, value );
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = effect.player -> find_spell( 279927 );

  buff_t* buff = buff_t::find( effect.player, "earthlink" );
  if ( !buff )
  {
    buff = make_buff<earthlink_t>( effect.player )
      -> add_stat( effect.player -> primary_stat(), power.value( 1 ) )
      -> set_duration( effect.player -> sim -> max_time * 3 )
      -> set_period( driver -> effectN( 1 ).period() );
  }

  effect.player -> register_combat_begin( [ buff ]( player_t* ) {
    buff -> trigger();
  } );
}

void wandering_soul( special_effect_t& effect )
{
  struct ruinous_bolt_t : unique_gear::proc_spell_t
  {
    ruinous_bolt_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "ruinous_bolt", e.player, e.player -> find_spell( 280206 ) )
    {
      aoe = 0;
      base_dd_min = base_dd_max = power.value( 1 );
    }
  };

  const spell_data_t* driver = effect.player -> find_spell( 273825 );

  // Check if we already registered the proc callback
  const bool exists = effect.player -> callbacks.has_callback( [ driver ]( const action_callback_t* cb ) {
    auto dbc_cb = dynamic_cast<const dbc_proc_callback_t*>( cb );
    return dbc_cb && dbc_cb -> effect.driver() == driver;
  } );
  if ( exists )
    return;

  azerite_power_t blightborne_infusion = effect.player -> find_azerite_spell( "Blightborne Infusion" );
  azerite_power_t ruinous_bolt = effect.player -> find_azerite_spell( "Ruinous Bolt" );
  if ( !( blightborne_infusion.enabled() || ruinous_bolt.enabled() ) )
    return;

  const spell_data_t* spell = effect.player -> find_spell( 280204 );
  const std::string spell_name = tokenized_name( spell );

  buff_t* buff = buff_t::find( effect.player, spell_name );
  if ( !buff )
  {
    if ( blightborne_infusion.enabled() )
    {
      buff = make_buff<stat_buff_t>( effect.player, spell_name, spell )
        -> add_stat( STAT_CRIT_RATING, blightborne_infusion.value( 1 ) );
    }
    else
    {
      buff = make_buff<buff_t>( effect.player, spell_name, spell );
    }

    if ( ruinous_bolt.enabled() )
    {
      auto action = unique_gear::create_proc_action<ruinous_bolt_t>( "ruinous_bolt", effect, ruinous_bolt );
      buff -> set_tick_callback( [ action ]( buff_t* b, int, const timespan_t& ) {
        // TODO: review targeting behaviour
        action -> set_target( b -> player -> target );
        action -> execute();
      } );
    }
    else
    {
      buff -> set_period( timespan_t::zero() ); // disable ticking
    }
  }

  effect.custom_buff = buff;
  effect.spell_id = driver -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void swirling_sands( special_effect_t& effect )
{
  struct swirling_sands_buff_t : public stat_buff_t
  {
    dbc_proc_callback_t* extender;
    timespan_t max_extension;
    timespan_t current_extension = timespan_t::zero();

    swirling_sands_buff_t( player_t* p, const azerite_power_t& power, dbc_proc_callback_t* e ):
      stat_buff_t( p, "swirling_sands", p -> find_spell( 280433 ) ),
      extender( e ), max_extension( power.spell_ref().effectN( 3 ).time_value() )
    {
      add_stat( STAT_CRIT_RATING, power.value( 1 ) );
    }

    void extend_duration( player_t* p, timespan_t extra_seconds ) override
    {
      if ( !check() || cooldown -> down() )
        return;

      extra_seconds = std::min( extra_seconds, max_extension - current_extension );
      if ( extra_seconds == timespan_t::zero() )
        return;

      stat_buff_t::extend_duration( p, extra_seconds );
      current_extension += extra_seconds;
      cooldown -> start();
    }

    void start( int stacks, double value, timespan_t duration ) override
    {
      stat_buff_t::start( stacks, value, duration );
      extender -> activate();
      current_extension = timespan_t::zero();
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      stat_buff_t::expire_override( expiration_stacks, remaining_duration );
      extender -> deactivate();
    }

    void reset() override
    {
      stat_buff_t::reset();
      extender -> deactivate();
    }
  };

  struct swirling_sands_extender_t : public dbc_proc_callback_t
  {
    timespan_t extension;

    swirling_sands_extender_t( const special_effect_t& effect, const azerite_power_t& power ) :
      dbc_proc_callback_t( effect.player, effect ),
      extension( power.spell_ref().effectN( 2 ).time_value() )
    {}

    void execute( action_t* a, action_state_t* ) override
    {
      proc_buff -> extend_duration( a -> player, extension );
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = effect.player -> find_spell( 280432 );

  auto secondary = new special_effect_t( effect.player );
  secondary -> name_str = "swirling_sands_extender";
  secondary -> type = effect.type;
  secondary -> source = effect.source;
  secondary -> proc_flags_ = driver -> proc_flags();
  secondary -> proc_flags2_ = PF2_CRIT;
  secondary -> proc_chance_ = 1.0;
  secondary -> cooldown_ = timespan_t::zero();
  effect.player -> special_effects.push_back( secondary );

  auto extender = new swirling_sands_extender_t( *secondary, power );

  buff_t* buff = buff_t::find( effect.player, "swirling_sands" );
  if ( !buff )
    buff = make_buff<swirling_sands_buff_t>( effect.player, power, extender );

  secondary -> custom_buff = buff;
  extender -> deactivate();

  effect.custom_buff = buff;
  effect.spell_id = driver -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void dagger_in_the_back( special_effect_t& effect )
{
  struct dagger_in_the_back_t : public unique_gear::proc_spell_t
  {
    dagger_in_the_back_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "dagger_in_the_back", e.player, e.player -> find_spell( 280286 ) )
    {
      base_td = power.value( 1 );
      tick_may_crit = true;
    }

    // XXX: simply apply twice here for the "from the back" case
    // in-game this happens with a slight delay of about ~250ms (beta realms from eu)
    // and may be better done through custom proc::execute scheduling an action execute event
    void trigger_dot( action_state_t* s ) override
    {
      proc_spell_t::trigger_dot( s );
      if ( player -> position() == POSITION_BACK || player -> position() == POSITION_RANGED_BACK )
        proc_spell_t::trigger_dot( s );
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<dagger_in_the_back_t>( "dagger_in_the_back", effect, power );
  effect.spell_id = power.spell_ref().effectN( 1 ).trigger() -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void rezans_fury( special_effect_t& effect )
{
  struct rezans_fury_t : public unique_gear::proc_spell_t
  {
    rezans_fury_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "rezans_fury", e.player, e.player -> find_spell( 273794 ) )
    {
      base_dd_min = base_dd_max = power.value( 1 );
      base_td = power.value( 2 );
      tick_may_crit = true;
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<rezans_fury_t>( "rezans_fury", effect, power );
  effect.spell_id = power.spell_ref().effectN( 1 ).trigger() -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void secrets_of_the_deep( special_effect_t& effect )
{
  struct sotd_cb_t : public dbc_proc_callback_t
  {
    std::vector<buff_t*> buffs;

    sotd_cb_t( const special_effect_t& effect, const std::vector<buff_t*>& b ) :
      dbc_proc_callback_t( effect.player, effect ), buffs( b )
    { }

    void execute( action_t*, action_state_t* ) override
    {
      size_t index = as<size_t>( rng().roll( listener->sim->bfa_opts.secrets_of_the_deep_chance ) );
      buffs[ index ]->trigger();
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  auto normal_buff = unique_gear::create_buff<stat_buff_t>( effect.player, "secrets_of_the_deep",
    effect.player->find_spell( 273843 ) )
    ->add_stat( effect.player->primary_stat(), power.value( 1 ) )
    ->set_chance( effect.player->sim->bfa_opts.secrets_of_the_deep_collect_chance );

  auto rare_buff = unique_gear::create_buff<stat_buff_t>( effect.player, "secrets_of_the_deep_rare",
    effect.player->find_spell( 273843 ) )
    ->add_stat( effect.player->primary_stat(), power.value( 2 ) )
    ->set_chance( effect.player->sim->bfa_opts.secrets_of_the_deep_collect_chance );

  effect.spell_id = effect.driver()->effectN( 1 ).trigger()->id();

  new sotd_cb_t( effect, { normal_buff, rare_buff } );
}

void combined_might( special_effect_t& effect )
{
  struct combined_might_cb_t : public dbc_proc_callback_t
  {
    std::vector<buff_t*> buffs;

    combined_might_cb_t( const special_effect_t& effect, const std::vector<buff_t*>& b ) :
      dbc_proc_callback_t( effect.player, effect ), buffs( b )
    { }

    void execute( action_t*, action_state_t* ) override
    {
      size_t index = static_cast<size_t>( rng().range( 0, buffs.size() ) );
      buffs[ index ]->trigger();
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  std::vector<buff_t*> buffs;
  if ( util::is_alliance( effect.player->race ) )
  {
    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "strength_of_the_humans",
        effect.player->find_spell( 280866 ) )
      ->add_stat( STAT_ATTACK_POWER, power.value( 1 ) )
      ->add_stat( STAT_INTELLECT, power.value( 1 ) ) );

    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "strength_of_the_night_elves",
        effect.player->find_spell( 280867 ) )
      ->add_stat( STAT_HASTE_RATING, power.value( 3 ) ) );

    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "strength_of_the_dwarves",
        effect.player->find_spell( 280868 ) )
      ->add_stat( STAT_VERSATILITY_RATING, power.value( 3 ) ) );

    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "strength_of_the_draenei",
        effect.player->find_spell( 280869 ) )
      ->add_stat( STAT_CRIT_RATING, power.value( 3 ) ) );

    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "strength_of_the_gnomes",
        effect.player->find_spell( 280870 ) )
      ->add_stat( STAT_MASTERY_RATING, power.value( 3 ) ) );
  }
  else
  {
    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "might_of_the_orcs",
        effect.player->find_spell( 280841 ) )
      ->add_stat( STAT_ATTACK_POWER, power.value( 1 ) )
      ->add_stat( STAT_INTELLECT, power.value( 1 ) ) );

    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "might_of_the_trolls",
        effect.player->find_spell( 280842 ) )
      ->add_stat( STAT_HASTE_RATING, power.value( 3 ) ) );

    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "might_of_the_tauren",
        effect.player->find_spell( 280843 ) )
      ->add_stat( STAT_VERSATILITY_RATING, power.value( 3 ) ) );

    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "might_of_the_forsaken",
        effect.player->find_spell( 280844 ) )
      ->add_stat( STAT_CRIT_RATING, power.value( 3 ) ) );

    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "might_of_the_sindorei",
        effect.player->find_spell( 280845 ) )
      ->add_stat( STAT_MASTERY_RATING, power.value( 3 ) ) );
  }

  effect.spell_id = 280848;

  new combined_might_cb_t( effect, buffs );
}

void relational_normalization_gizmo( special_effect_t& effect )
{
  struct gizmo_cb_t : public dbc_proc_callback_t
  {
    std::vector<buff_t*> buffs;

    gizmo_cb_t( const special_effect_t& effect, const std::vector<buff_t*>& b ) :
      dbc_proc_callback_t( effect.player, effect ), buffs( b )
    { }

    // TODO: Probability distribution?
    void execute( action_t*, action_state_t* ) override
    {
      size_t index = as<size_t>( rng().roll( 0.5 ) ); // Coin flip
      buffs[ index ]->trigger();
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  // Need to manually calculate the stat amounts for the buff, as they are not grabbed from the
  // azerite power spell, like normal
  auto ilevels = power.ilevels();
  auto budgets = power.budget();

  const spell_data_t* increase_spell = effect.player->find_spell( 280653 );
  const spell_data_t* decrease_spell = effect.player->find_spell( 280654 );

  double haste_amount = 0, stat_amount = 0, health_amount = 0;
  for ( size_t i = 0, end = ilevels.size(); i < end; ++i )
  {
    double value = floor( increase_spell->effectN( 1 ).m_coefficient() * budgets[ i ] + 0.5 );
    haste_amount += floor( item_database::apply_combat_rating_multiplier( effect.player,
        CR_MULTIPLIER_ARMOR, ilevels[ i ], value ) );

    stat_amount += floor( decrease_spell->effectN( 4 ).m_coefficient() * budgets[ i ] + 0.5 );
    health_amount += floor( decrease_spell->effectN( 6 ).m_coefficient() * budgets[ i ] + 0.5 );
  }

  auto increase = unique_gear::create_buff<stat_buff_t>( effect.player, "normalization_increase",
      effect.player->find_spell( 280653 ) )
    ->add_stat( STAT_HASTE_RATING, haste_amount )
    ->add_invalidate( CACHE_RUN_SPEED );

  auto decrease = unique_gear::create_buff<stat_buff_t>( effect.player, "normalization_decrease",
      effect.player->find_spell( 280654 ) )
    ->add_stat( effect.player->primary_stat(), stat_amount )
    ->add_stat( STAT_MAX_HEALTH, health_amount );

  effect.player->buffs.normalization_increase = increase;

  new gizmo_cb_t( effect, { decrease, increase } );
}

void barrage_of_many_bombs( special_effect_t& effect )
{
  struct barrage_damage_t : public unique_gear::proc_spell_t
  {
    barrage_damage_t( const special_effect_t& effect, const azerite_power_t& power ) :
      proc_spell_t( "barrage_of_many_bombs", effect.player, effect.player->find_spell( 280984 ),
          effect.item )
    {
      aoe = -1;
      auto values = compute_value( power, data().effectN( 1 ) );
      base_dd_min = std::get<0>( values );
      base_dd_max = std::get<2>( values );

      // Grab missile speed from the driveer
      travel_speed = power.spell_ref().effectN( 1 ).trigger()->missile_speed();
    }
  };

  struct bomb_cb_t : public dbc_proc_callback_t
  {
    action_t* damage;
    int n_bombs;

    bomb_cb_t( const special_effect_t& effect, const azerite_power_t& power ) :
      dbc_proc_callback_t( effect.player, effect ),
      damage( unique_gear::create_proc_action<barrage_damage_t>( "barrage_of_many_bombs", effect,
              power ) ),
      n_bombs( power.spell_ref().effectN( 3 ).base_value() )
    { }

    void execute( action_t*, action_state_t* state ) override
    {
      damage->set_target( state->target );
      for ( int i = 0; i < n_bombs; ++i )
      {
        damage->execute();
      }
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  new bomb_cb_t( effect, power );
}
} // Namespace special effects ends
} // Namespace azerite ends
