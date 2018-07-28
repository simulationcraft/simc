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
  range::for_each( budgets, [ & ]( unsigned budget ) {
    sum += m_spell -> effectN( index ).m_average() * budget;
  } );

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
{
  std::vector<double> b;

  if ( m_player == nullptr )
  {
    return {};
  }

  range::for_each( m_ilevels, [ & ]( unsigned ilevel ) {
    unsigned min_ilevel = ilevel;
    if ( m_spell -> max_scaling_level() > 0 && m_spell -> max_scaling_level() < min_ilevel )
    {
      min_ilevel = m_spell -> max_scaling_level();
    }

    auto budget = item_database::item_budget( m_player, min_ilevel );
    if ( m_spell -> scaling_class() == PLAYER_SPECIAL_SCALE7 )
    {
      budget = item_database::apply_combat_rating_multiplier( m_player,
          CR_MULTIPLIER_ARMOR, min_ilevel, budget );
    }
    b.push_back( budget );
  } );

  return b;
}

/// List of items associated with this azerite power
const std::vector<unsigned> azerite_power_t::ilevels() const
{ return m_ilevels; }

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
      m_state[ id ] = false;
      m_items[ id ].push_back( &( item ) );
    } );
  });
}

bool azerite_state_t::is_initialized( unsigned id ) const
{
  auto it = m_state.find( id );
  if ( it != m_state.end() )
  {
    return it -> second;
  }

  return false;
}

azerite_power_t azerite_state_t::get_power( unsigned id )
{
  // All azerite disabled
  if ( m_player -> sim -> azerite_status == AZERITE_DISABLED_ALL )
  {
    return {};
  }

  auto it = m_state.find( id );
  if ( it == m_state.end() )
  {
    return {};
  }

  // Already initialized (successful invocation of get_power for this actor with the same id), so
  // don't give out the spell data again
  if ( it -> second )
  {
    m_player -> sim -> errorf( "%s attempting to re-initialize azerite power %u",
        m_player -> name(), id );
    return {};
  }

  it -> second = true;

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

    // Item-related azerite effects are only enabled when "all" is defined
    if ( m_player -> sim -> azerite_status == AZERITE_ENABLED )
    {
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
      // Note, overridden powers should also have init state tracked
      m_state[ power->id ] = false;
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

  for ( const auto& entry : m_state )
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
}
} // Namespace azerite ends

namespace azerite
{
namespace special_effects
{
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

    thunderous_blast_t( special_effect_t& e, const azerite_power_t& power ):
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

  effect.execute_action = effect.player -> find_action( "thunderous_blast" );
  if ( !effect.execute_action )
    effect.execute_action = effect.player -> create_proc_action( "thunderous_blast", effect );
  if ( !effect.execute_action )
    effect.execute_action = new thunderous_blast_t( effect, power );

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = effect.player -> find_spell( 280383 ) -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void filthy_transfusion( special_effect_t& effect )
{
  struct filthy_transfusion_t : public unique_gear::proc_spell_t
  {
    filthy_transfusion_t( special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "filthy_transfusion", e.player, e.player -> find_spell( 273836 ) )
    {
      base_td = power.value( 1 );
      tick_may_crit = false;
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( ! power.enabled() )
    return;

  effect.execute_action = effect.player -> find_action( "filthy_transfusion" );
  if ( !effect.execute_action )
    effect.execute_action = effect.player -> create_proc_action( "filthy_transfusion", effect );
  if ( !effect.execute_action )
    effect.execute_action = new filthy_transfusion_t( effect, power );

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = effect.player -> find_spell( 273835 ) -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

} // Namespace special effects ends
} // Namespace azerite ends
