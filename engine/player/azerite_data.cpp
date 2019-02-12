#include "azerite_data.hpp"

#include "simulationcraft.hpp"

azerite_power_t::azerite_power_t() :
  m_player( nullptr ), m_spell( spell_data_t::not_found() ), m_data( nullptr )
{ }

azerite_power_t::azerite_power_t( const player_t* p, const azerite_power_entry_t* data,
    const std::vector<const item_t*>& items ) :
  m_player( p ), m_spell( p->find_spell( data->spell_id ) ), m_data( data )
{
  range::for_each( items, [ this ]( const item_t* item ) {
    if ( item->item_level() > 0 && item->item_level() <= MAX_ILEVEL )
    {
      m_ilevels.push_back( item -> item_level() );
    }
  } );
}

azerite_power_t::azerite_power_t( const player_t* p, const azerite_power_entry_t* data,
    const std::vector<unsigned>& ilevels ) :
  m_player( p ), m_spell( p->find_spell( data->spell_id ) ), m_ilevels( ilevels ), m_data( data )
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

const azerite_power_entry_t* azerite_power_t::data() const
{ return m_data; }

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
          CR_MULTIPLIER_ARMOR, actual_level, value ) + 0.5;
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
    if ( ilevel < 1 || ilevel > MAX_ILEVEL )
    {
      return;
    }

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

      return { m_player, &power, override_it -> second };
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

      return { m_player, &power, m_items[ id ] };
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

  range::sort( spells );
  auto it = range::unique( spells );

  spells.erase( it, spells.end() );

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
  unique_gear::register_special_effect( 280580, special_effects::combined_might        ); // Combined Might
  unique_gear::register_special_effect( 280625, special_effects::combined_might        ); // Stronger Together
  unique_gear::register_special_effect( 280178, special_effects::relational_normalization_gizmo );
  unique_gear::register_special_effect( 280163, special_effects::barrage_of_many_bombs );
  unique_gear::register_special_effect( 273682, special_effects::meticulous_scheming   );
  unique_gear::register_special_effect( 280174, special_effects::synaptic_spark_capacitor );
  unique_gear::register_special_effect( 280168, special_effects::ricocheting_inflatable_pyrosaw );
  unique_gear::register_special_effect( 266937, special_effects::gutripper             );
  unique_gear::register_special_effect( 280582, special_effects::battlefield_focus_precision );
  unique_gear::register_special_effect( 280627, special_effects::battlefield_focus_precision );
  unique_gear::register_special_effect( 287662, special_effects::endless_hunger        ); // Endless Hunger
  unique_gear::register_special_effect( 287604, special_effects::endless_hunger        ); // Ancient's Bulwark
  unique_gear::register_special_effect( 287631, special_effects::apothecarys_concoctions );
  unique_gear::register_special_effect( 287467, special_effects::shadow_of_elune       );
  unique_gear::register_special_effect( 288953, special_effects::treacherous_covenant  );
  unique_gear::register_special_effect( 288749, special_effects::seductive_power       );
  unique_gear::register_special_effect( 288802, special_effects::bonded_souls          );
  unique_gear::register_special_effect( 287818, special_effects::fight_or_flight       );
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

  // Battlefield Focus / Precision
  sim -> register_target_data_initializer( [] ( actor_target_data_t* td ) {
    auto&& azerite = td -> source -> azerite;
    if ( ! azerite )
    {
      return;
    }

    if ( azerite->is_enabled( "Battlefield Focus" ) )
    {
      td->debuff.battlefield_debuff = make_buff( *td, "battlefield_focus",
          td->source->find_spell( 280817 ) );
      td->debuff.battlefield_debuff->reset();
    }
    else if ( azerite->is_enabled( "Battlefield Precision" ) )
    {
      td->debuff.battlefield_debuff = make_buff( *td, "battlefield_precision",
          td->source->find_spell( 280855 ) );
      td->debuff.battlefield_debuff->reset();
    }
    else
    {
      td -> debuff.battlefield_debuff = make_buff( *td, "battlefield_debuff" );
      td -> debuff.battlefield_debuff->set_quiet( true );
    }
  } );
}

std::tuple<int, int, int > compute_value( const azerite_power_t& power, const spelleffect_data_t& effect )
{
  int min_ = 0, max_ = 0, avg_ = 0;
  if ( !power.enabled() || effect.m_coefficient() == 0 )
  {
    return std::make_tuple( 0, 0, 0 );
  }

  auto budgets = power.budget( effect.spell() );
  range::for_each( budgets, [&]( double budget ) {
    avg_ += static_cast<int>( budget * effect.m_coefficient() + 0.5 );
    min_ += static_cast<int>( budget * effect.m_coefficient() *
        ( 1.0 - effect.m_delta() / 2 ) + 0.5 );
    max_ += static_cast<int>( budget * effect.m_coefficient() *
        ( 1.0 + effect.m_delta() / 2 ) + 0.5 );
  } );

  return std::make_tuple( min_, avg_, max_ );
}

void parse_blizzard_azerite_information( item_t& item, const rapidjson::Value& data )
{
  if ( data.HasMember( "azeriteEmpoweredItem" ) && data[ "azeriteEmpoweredItem" ].IsObject() )
  {
    const auto& azerite_data = data[ "azeriteEmpoweredItem" ];

    if ( azerite_data.HasMember( "azeritePowers" ) && azerite_data[ "azeritePowers" ].IsArray() )
    {
      for ( rapidjson::SizeType i = 0, end = azerite_data[ "azeritePowers" ].Size(); i < end; i++ )
      {
        const auto& power = azerite_data[ "azeritePowers" ][ i ];
        if ( !power.IsObject() || !power.HasMember( "id" ) )
        {
          continue;
        }

        auto id = power[ "id" ].GetUint();
        if ( id == 0 )
        {
          continue;
        }

        item.parsed.azerite_ids.push_back( id );

        if ( power.HasMember( "bonusListId" ) && power[ "bonusListId" ].GetInt() > 0 )
        {
          item.sim->error( "Player {} has non-zero bonusListId {} for item {}",
              item.player->name(), power[ "bonusListId" ].GetInt(), item.name() );
        }
      }
    }
  }

  if ( data.HasMember( "azeriteItem" ) )
  {
    const auto& azerite_data = data[ "azeriteItem" ];
    if ( azerite_data.HasMember( "azeriteLevel" ) )
    {
      item.parsed.azerite_level = azerite_data[ "azeriteLevel" ].GetUint();
    }
  }
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
      else if ( rolling_thunder -> check() )
      {
        rolling_thunder -> expire();
      }
      else
      {
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
  effect.spell_id = 280383;

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
  effect.spell_id = 273835;

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

  const spell_data_t* driver = power.spell_ref().effectN( 2 ).trigger();
  const spell_data_t* mastery_spell = effect.player -> find_spell( power.spell_ref().id() == 280624 ? 280861 : 280787 );
  const spell_data_t* absorb_spell = effect.player -> find_spell( power.spell_ref().id() == 280624 ? 280862 : 280788 );

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

  //Kills refresh buff
  range::for_each( effect.player -> sim -> actor_list, [effect]( player_t* target ) {
    if ( !target -> is_enemy() )
    {
      return;
    }

    target->callbacks_on_demise.push_back( [effect]( player_t* ) {
      if ( effect.custom_buff -> up( ) )
      {
        effect.custom_buff -> refresh( );
      }
    } );
  } );

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
                             ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 1 ) );
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
                             ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 1 ) )
                             ->add_stat( STAT_MAX_HEALTH, power.value( 2 ) );
  }

  // TODO give to 4 other actors?

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver->id();

  new dbc_proc_callback_t( effect.player, effect );
}

struct reorigination_array_buff_t : public buff_t
{
  // Comes from server side, hardcoded somewhere there
  double stat_value = 75.0;
  stat_e current_stat = STAT_NONE;

  proc_t* proc_crit, *proc_haste, *proc_mastery, *proc_versatility;

  void add_proc( stat_e stat )
  {
    switch ( stat )
    {
      case STAT_CRIT_RATING:
        proc_crit->occur();
        break;
      case STAT_HASTE_RATING:
        proc_haste->occur();
        break;
      case STAT_MASTERY_RATING:
        proc_mastery->occur();
        break;
      case STAT_VERSATILITY_RATING:
        proc_versatility->occur();
        break;
      default:
        break;
    }
  }

  stat_e rating_to_stat( rating_e r ) const
  {
    switch ( r )
    {
      case RATING_MELEE_CRIT: return STAT_CRIT_RATING;
      case RATING_MELEE_HASTE: return STAT_HASTE_RATING;
      case RATING_MASTERY: return STAT_MASTERY_RATING;
      case RATING_DAMAGE_VERSATILITY: return STAT_VERSATILITY_RATING;
      default: return STAT_NONE;
    }
  }

  rating_e stat_to_rating( stat_e s ) const
  {
    switch ( s )
    {
      case STAT_CRIT_RATING: return RATING_MELEE_CRIT;
      case STAT_HASTE_RATING: return RATING_MELEE_HASTE;
      case STAT_MASTERY_RATING: return RATING_MASTERY;
      case STAT_VERSATILITY_RATING: return RATING_DAMAGE_VERSATILITY;
      default: return RATING_MAX;
    }
  }

  stat_e find_highest_stat() const
  {
    // Simc bunches ratings together so it does not matter which we pick here
    static const std::vector<rating_e> ratings {
      RATING_MELEE_CRIT, RATING_MELEE_HASTE, RATING_MASTERY, RATING_DAMAGE_VERSATILITY
    };

    double high = std::numeric_limits<double>::lowest();
    rating_e rating = RATING_MAX;

    for ( auto r : ratings )
    {
      auto stat = rating_to_stat( r );
      auto composite = source->composite_rating( r );
      // Calculate highest stat without reorigination array included
      if ( stat == current_stat )
      {
        composite -= sim->bfa_opts.reorigination_array_stacks * stat_value *
          source->composite_rating_multiplier( r );

        if ( sim->scaling && sim->bfa_opts.reorigination_array_ignore_scale_factors &&
             stat == sim->scaling->scale_stat )
        {
          composite -= sim->scaling->scale_value * source->composite_rating_multiplier( r );
        }
      }

      if ( sim->debug )
      {
        sim->out_debug.print( "{} reorigination_array check_stat current_stat={} stat={} value={}",
            source->name(), util::stat_type_string( current_stat ),
            util::stat_type_string( rating_to_stat( r ) ), composite );
      }

      if ( composite > high )
      {
        high = composite;
        rating = r;
      }
    }

    return rating_to_stat( rating );
  }

  void trigger_dynamic_stat_change()
  {
    auto highest_stat = find_highest_stat();
    if ( highest_stat == current_stat )
    {
      return;
    }

    if ( current_stat != STAT_NONE )
    {
      rating_e current_rating = stat_to_rating( current_stat );

      double current_amount = stat_value * sim->bfa_opts.reorigination_array_stacks;

      source->stat_loss( current_stat, current_amount );

      if ( sim->debug )
      {
        sim->out_debug.print( "{} reorigination_array stat change, current_stat={}, value={}, total={}",
            source->name(), util::stat_type_string( current_stat ), -current_amount,
            source->composite_rating( current_rating ) );
      }
    }

    if ( highest_stat != STAT_NONE )
    {
      rating_e new_rating = stat_to_rating( highest_stat );

      double new_amount = stat_value * sim->bfa_opts.reorigination_array_stacks;

      source->stat_gain( highest_stat, new_amount );

      if ( sim->debug )
      {
        sim->out_debug.print( "{} reorigination_array stat change, new_stat={}, value={}, total={}",
            source->name(), util::stat_type_string( highest_stat ), new_amount,
            source->composite_rating( new_rating ) );
      }

      add_proc( highest_stat );
    }

    current_stat = highest_stat;
  }

  reorigination_array_buff_t( player_t* p, const std::string& name, const special_effect_t& effect ) :
    buff_t( p, name, effect.player->find_spell( 280573 ), effect.item ),
    proc_crit( p->get_proc( "Reorigination Array: Critical Strike" ) ),
    proc_haste( p->get_proc( "Reorigination Array: Haste" ) ),
    proc_mastery( p->get_proc( "Reorigination Array: Mastery" ) ),
    proc_versatility( p->get_proc( "Reorigination Array: Versatility" ) )
  { }

  void reset() override
  {
    buff_t::reset();

    current_stat = STAT_NONE;
  }

  void execute( int stacks = 1, double value = DEFAULT_VALUE(),
                timespan_t duration = timespan_t::min() ) override
  {
    // Ensure buff only executes once
    if ( current_stack == 0 )
    {
      buff_t::execute( stacks, value, duration );

      trigger_dynamic_stat_change();

      make_repeating_event( *sim, timespan_t::from_seconds( 5.0 ), [ this ]() {
          trigger_dynamic_stat_change();
      } );
    }
  }

  void expire_override( int stacks, timespan_t duration ) override
  {
    buff_t::expire_override( stacks, duration );

    if ( current_stat != STAT_NONE )
    {
      source->stat_loss( current_stat, stat_value * sim->bfa_opts.reorigination_array_stacks );
      current_stat = STAT_NONE;
    }
  }
};

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
      ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 1 ) );
  }

  buff_t* reorg = buff_t::find( effect.player, "reorigination_array" );
  // Reorigination array has to be implemented uniquely, so only Archive of the Titans, or Laser
  // Matrix will properly initialize the system.
  if ( ! reorg && effect.player->sim->bfa_opts.reorigination_array_stacks > 0 )
  {
    reorg = unique_gear::create_buff<reorigination_array_buff_t>( effect.player,
        "reorigination_array", effect );
  }

  effect.player->register_combat_begin( [ buff, driver, reorg ]( player_t* ) {
    if ( buff->sim->bfa_opts.initial_archive_of_the_titans_stacks )
    {
      buff->trigger( buff->sim->bfa_opts.initial_archive_of_the_titans_stacks );
    }

    if ( reorg )
    {
      reorg->trigger( buff->sim->bfa_opts.reorigination_array_stacks );
    }

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
  effect.proc_flags_    = PF_ALL_DAMAGE;

  new dbc_proc_callback_t( effect.player, effect );

  buff_t* reorg = buff_t::find( effect.player, "reorigination_array" );
  // Reorigination array has to be implemented uniquely, so only Archive of the Titans, or Laser
  // Matrix will properly initialize the system.
  if ( ! reorg && effect.player->sim->bfa_opts.reorigination_array_stacks > 0 )
  {
    reorg = unique_gear::create_buff<reorigination_array_buff_t>( effect.player,
        "reorigination_array", effect );

    effect.player->register_combat_begin( [ reorg ]( player_t* ) {
      reorg->trigger( reorg->sim->bfa_opts.reorigination_array_stacks );
    } );
  }
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
      -> add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 1 ) );
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
  effect.spell_id = 280403;

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
  effect.spell_id = 271681;

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
  effect.spell_id = 279955;

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
      -> add_stat( STAT_HASTE_RATING, power.value( 1 ) );
  }

  effect.custom_buff = buff;
  effect.spell_id = driver -> id();

  struct overwhelming_power_cb_t : public dbc_proc_callback_t
  {
    overwhelming_power_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.player, effect )
    { }

    void execute( action_t*, action_state_t* ) override
    {
      proc_buff -> trigger( proc_buff -> max_stack() );
    }
  };

  new overwhelming_power_cb_t( effect );

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
      -> add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 1 ) )
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
      range = data().effectN( 1 ).radius_max();
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
      buff -> set_tick_callback( [ action ]( buff_t*, int, const timespan_t& ) {
        const auto& tl = action -> target_list();
        if ( tl.empty() )
          return;
        action -> set_target( tl[ action -> rng().range( tl.size() ) ] );
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

    timespan_t calculate_dot_refresh_duration(const dot_t*  /* dot */, timespan_t triggered_duration ) const override
    {
      return triggered_duration;
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
    ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 1 ) )
    ->set_chance( effect.player->sim->bfa_opts.secrets_of_the_deep_collect_chance )
    ->set_refresh_behavior( buff_refresh_behavior::EXTEND );

  auto rare_buff = unique_gear::create_buff<stat_buff_t>( effect.player, "secrets_of_the_deep_rare",
    effect.player->find_spell( 273843 ) )
    ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 2 ) )
    ->set_chance( effect.player->sim->bfa_opts.secrets_of_the_deep_collect_chance )
    ->set_refresh_behavior( buff_refresh_behavior::EXTEND );

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
    ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), stat_amount )
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

void meticulous_scheming( special_effect_t& effect )
{
  // Apparently meticulous scheming bugs and does not proc from some foreground abilities. Blacklist
  // specific abilities here
  static std::unordered_map<unsigned, bool> __spell_blacklist {{
    { 201427, true }, // Demon Hunter: Annihilation
    { 210152, true }, // Demon Hunter: Death Sweep
  } };

  struct seize_driver_t : public dbc_proc_callback_t
  {
    std::vector<unsigned> casts;
    buff_t* base;

    seize_driver_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.player, effect ), base( nullptr )
    { }

    bool trigger_seize() const
    { return casts.size() == as<size_t>( effect.driver()->effectN( 1 ).base_value() ); }

    void execute( action_t* a, action_state_t* ) override
    {
      // Presume the broken spells not affecting Meticulous Scheming is a bug
      if ( listener->bugs && __spell_blacklist.find( a->id ) != __spell_blacklist.end() )
      {
        return;
      }

      auto it = range::find( casts, a->id );
      if ( it == casts.end() )
      {
        casts.push_back( a->id );
      }

      if ( trigger_seize() )
      {
        base->expire();
      }
    }

    void activate() override
    {
      dbc_proc_callback_t::activate();

      casts.clear();
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  auto secondary = new special_effect_t( effect.player );
  secondary->name_str = "meticulous_scheming_driver";
  secondary->spell_id = 273685;
  secondary->type = effect.type;
  secondary->source = effect.source;
  secondary->proc_flags_ = PF_MELEE_ABILITY | PF_RANGED | PF_RANGED_ABILITY | PF_NONE_HEAL |
                           PF_NONE_SPELL | PF_MAGIC_SPELL | PF_MAGIC_HEAL;
  secondary->proc_flags2_ = PF2_CAST | PF2_CAST_DAMAGE | PF2_CAST_HEAL;
  effect.player -> special_effects.push_back( secondary );

  auto meticulous_cb = new seize_driver_t( *secondary );

  auto haste_buff = unique_gear::create_buff<stat_buff_t>( effect.player, "seize_the_moment",
      effect.player->find_spell( 273714 ) )
    ->add_stat( STAT_HASTE_RATING, power.value( 4 ) );

  auto base_buff = unique_gear::create_buff<buff_t>( effect.player, "meticulous_scheming",
      effect.player->find_spell( 273685 ) )
    ->set_stack_change_callback( [meticulous_cb, haste_buff]( buff_t*, int, int new_ ) {
        if ( new_ == 0 )
        {
          meticulous_cb->deactivate();
          if ( meticulous_cb->trigger_seize() )
          {
            haste_buff->trigger();
          }
        }
        else
        {
          meticulous_cb->activate();
        }
    } );

  meticulous_cb->deactivate();
  meticulous_cb->base = base_buff;

  effect.spell_id = 273684;
  effect.custom_buff = base_buff;
  // Spell data has no proc flags for the base spell, so make something up that would resemble
  // "spells and abilities"
  effect.proc_flags_ = PF_MELEE_ABILITY | PF_RANGED | PF_RANGED_ABILITY | PF_NONE_HEAL |
                       PF_NONE_SPELL | PF_MAGIC_SPELL | PF_MAGIC_HEAL | PF_PERIODIC;

  new dbc_proc_callback_t( effect.player, effect );
}

void synaptic_spark_capacitor( special_effect_t& effect )
{
  struct spark_coil_t : public unique_gear::proc_spell_t
  {
    spark_coil_t( const special_effect_t& effect, const azerite_power_t& power ) :
      proc_spell_t( "spark_coil", effect.player, effect.player->find_spell( 280847 ),
          effect.item )
    {
      aoe = -1;
      radius = power.spell_ref().effectN( 1 ).base_value();

      auto values = compute_value( power, power.spell_ref().effectN( 3 ) );
      base_dd_min = std::get<0>( values );
      base_dd_max = std::get<2>( values );

      // The DoT is handled in the driver spell.
      dot_duration = timespan_t::zero();
    }
  };

  // Driver for the spark coil that triggers damage
  // TODO Last partial tick does minimal damage?
  struct spark_coil_driver_t : public unique_gear::proc_spell_t
  {
    action_t* damage;

    spark_coil_driver_t( const special_effect_t& effect, const azerite_power_t& power ) :
      proc_spell_t( "spark_coil_driver", effect.player, effect.player->find_spell( 280655 ),
          effect.item )
    {
      quiet = true;
      damage = unique_gear::create_proc_action<spark_coil_t>( "spark_coil_damage", effect, power );
      dot_duration = data().duration();
      base_tick_time = damage->data().effectN( 2 ).period();
    }

    void tick( dot_t* d ) override
    {
      proc_spell_t::tick( d );

      // TODO: The 6th (partial) tick never happens in game.
      if ( d->get_last_tick_factor() < 1.0 )
        return;

      damage->set_target( d->target );
      damage->execute();
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<spark_coil_driver_t>( "spark_coil",
      effect, power );

  new dbc_proc_callback_t( effect.player, effect );
}

void ricocheting_inflatable_pyrosaw( special_effect_t& effect )
{
  struct rip_t : public unique_gear::proc_spell_t
  {
    rip_t( const special_effect_t& effect, const azerite_power_t& power ) :
      proc_spell_t( "r.i.p.", effect.player, effect.player->find_spell( 280656 ) )
    {
      auto value = compute_value( power, data().effectN( 1 ) );
      base_dd_min = std::get<0>( value );
      base_dd_max = std::get<2>( value );
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<rip_t>( "r.i.p.",
      effect, power );
  // Just the DPS effect for now
  effect.proc_flags_ = PF_MELEE_ABILITY | PF_RANGED_ABILITY | PF_NONE_SPELL | PF_MAGIC_SPELL |
    PF_PERIODIC;

  new dbc_proc_callback_t( effect.player, effect );
}

void gutripper( special_effect_t& effect )
{
  struct gutripper_t : public unique_gear::proc_spell_t
  {
    gutripper_t( const special_effect_t& effect, const azerite_power_t& power ) :
      proc_spell_t( "gutripper", effect.player, effect.player->find_spell( 269031 ) )
    {
      auto value = compute_value( power, power.spell_ref().effectN( 1 ) );
      base_dd_min = std::get<0>( value );
      base_dd_max = std::get<2>( value );
    }
  };

  struct gutripper_cb_t : public dbc_proc_callback_t
  {
    double threshold;

    gutripper_cb_t( const special_effect_t& effect, const azerite_power_t& power ) :
      dbc_proc_callback_t( effect.player, effect ),
      threshold( power.spell_ref().effectN( 2 ).base_value() )
    { }

    void trigger( action_t* a, void* call_data ) override
    {
      auto state = static_cast<action_state_t*>( call_data );
      if ( state->target->health_percentage() < threshold )
      {
        rppm->set_frequency( effect.driver()->real_ppm() );
      }
      else
      {
        rppm->set_frequency( listener->sim->bfa_opts.gutripper_default_rppm );
      }

      dbc_proc_callback_t::trigger( a, call_data );
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  effect.spell_id = 270668;
  effect.execute_action = unique_gear::create_proc_action<gutripper_t>( "gutripper", effect, power );

  new gutripper_cb_t( effect, power );
}

void battlefield_focus_precision( special_effect_t& effect )
{
  struct bf_damage_cb_t : public dbc_proc_callback_t
  {
    bf_damage_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.player, effect )
    { }

    void execute( action_t*, action_state_t* state ) override
    {
      auto td = listener->get_target_data( state->target );
      td->debuff.battlefield_debuff->decrement();
      proc_action->set_target( state->target );
      proc_action->schedule_execute();

      // Self-deactivate when the debuff runs out
      if ( td->debuff.battlefield_debuff->check() == 0 )
      {
        deactivate();
      }
    }
  };

  struct bf_trigger_cb_t : public dbc_proc_callback_t
  {
    action_callback_t* damage_cb;
    unsigned stacks;

    bf_trigger_cb_t( const special_effect_t& effect, action_callback_t* cb, unsigned stacks ) :
      dbc_proc_callback_t( effect.player, effect ), damage_cb( cb ), stacks( stacks )
    { }

    void execute( action_t*, action_state_t* state ) override
    {
      auto td = listener->get_target_data( state->target );
      td->debuff.battlefield_debuff->trigger( stacks );
      damage_cb->activate();
    }
  };

  struct bf_damage_t : public unique_gear::proc_spell_t
  {
    bf_damage_t( const special_effect_t& effect, const azerite_power_t& power ) :
      proc_spell_t( effect.name(), effect.player,
          effect.player->find_spell( effect.spell_id == 280627 ? 282720 : 282724 ) )
    {
      base_dd_min = base_dd_max = power.value( 1 );
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  // Since we are individualizing the Battlefield Debuff proc, we need to create a slightly custom
  // secondary special effect that procs on this actor's damage
  auto secondary = new special_effect_t( effect.player );
  secondary->name_str = "battlefield_debuff_driver";
  secondary->spell_id = effect.spell_id == 280627 ? 280855 : 280817;
  secondary->type = effect.type;
  secondary->source = effect.source;
  // Note, we override the proc flags to be sourced from the actor, instead of the target's "taken"
  secondary->proc_flags_ = PF_ALL_DAMAGE | PF_PERIODIC;
  secondary->execute_action = unique_gear::create_proc_action<bf_damage_t>( effect.name(), effect,
      power );

  effect.player -> special_effects.push_back( secondary );

  auto trigger_cb = new bf_damage_cb_t( *secondary );
  trigger_cb->deactivate();

  // Fix the driver to point to the RPPM base spell
  effect.spell_id = effect.spell_id == 280627 ? 280854 : 280816;

  new bf_trigger_cb_t( effect, trigger_cb, power.spell_ref().effectN( 2 ).base_value() );
}

void endless_hunger( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
  {
    return;
  }

  effect.player->passive.versatility_rating += power.value( 2 );
}

void apothecarys_concoctions( special_effect_t& effect )
{
  struct apothecarys_blight_t : public unique_gear::proc_spell_t
  {
    apothecarys_blight_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "apothecarys_blight", e.player, e.player -> find_spell( 287638 ) )
    {
      base_td = power.value( 1 );
      tick_may_crit = true;
    }

    double action_multiplier() const override
    {
      double am = proc_spell_t::action_multiplier();

      am *= 2.0 - this->target->health_percentage() / 100.0;

      return am;
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;
  
  effect.execute_action = unique_gear::create_proc_action<apothecarys_blight_t>( "apothecarys_concoctions", effect, power );
  effect.spell_id = 287633;

  new dbc_proc_callback_t( effect.player, effect );
}

void shadow_of_elune( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = effect.driver()->effectN( 1 ).trigger();
  const spell_data_t* buff_data = driver->effectN( 1 ).trigger();

  buff_t* buff = buff_t::find( effect.player, "shadow_of_elune" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "shadow_of_elune", buff_data )
      ->add_stat( STAT_HASTE_RATING, power.value( 1 ) );
  }

  effect.custom_buff = buff;
  effect.spell_id = driver->id();

  new dbc_proc_callback_t( effect.player, effect );
}

void treacherous_covenant( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  // This spell selects which buff to trigger depending on player health %.
  const spell_data_t* driver = effect.driver()->effectN( 1 ).trigger();
  const spell_data_t* buff_data = driver->effectN( 1 ).trigger();

  buff_t* buff = buff_t::find( effect.player, "treacherous_covenant" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "treacherous_covenant", buff_data )
      ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 1 ) );
  }

  // TODO: Model losing the buff when droping below 50%.
  effect.player->register_combat_begin( [ buff ] ( player_t* )
  {
    buff->trigger();

    // Simple system to allow for some manipulation of the buff uptime.
    if ( buff->sim->bfa_opts.covenant_chance < 1.0 )
    {
      make_repeating_event( buff->sim, buff->sim->bfa_opts.covenant_period, [ buff ]
      {
        if ( buff->rng().roll( buff->sim->bfa_opts.covenant_chance ) )
          buff->trigger();
        else
          buff->expire();
      } );
    }
  } );
}

void seductive_power( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = effect.driver()->effectN( 1 ).trigger();

  buff_t* buff = buff_t::find( effect.player, "seductive_power" );
  if ( !buff )
  {
    // Doesn't look like we can get from the driver to this buff, hardcode the spell id.
    buff = make_buff<stat_buff_t>( effect.player, "seductive_power", effect.player->find_spell( 288777 ) )
      ->add_stat( STAT_ALL, power.value( 1 ) );
  }

  struct seductive_power_cb_t : public dbc_proc_callback_t
  {
    seductive_power_cb_t( special_effect_t& effect ) :
      dbc_proc_callback_t( effect.player, effect )
    { }

    void execute( action_t*, action_state_t* ) override
    {
      if ( rng().roll( listener->sim->bfa_opts.seductive_power_pickup_chance ) )
        proc_buff->trigger();
      else
        proc_buff->decrement();
    }
  };

  // Spell data is missing proc flags, just pick something that's gonna be close to what the
  // tooltip says.
  effect.proc_flags_ = PF_ALL_DAMAGE | PF_ALL_HEAL;
  effect.spell_id = driver->id();
  effect.custom_buff = buff;

  new seductive_power_cb_t( effect );

  int initial_stacks = effect.player->sim->bfa_opts.initial_seductive_power_stacks;
  if ( initial_stacks > 0 )
  {
    effect.player->register_combat_begin( [ = ] ( player_t* ) { buff->trigger( initial_stacks ); } );
  }
}

void bonded_souls( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = effect.driver()->effectN( 2 ).trigger();
  const spell_data_t* driver_buff_data = driver->effectN( 1 ).trigger();
  const spell_data_t* haste_buff_data = driver_buff_data->effectN( 1 ).trigger();

  buff_t* haste_buff = buff_t::find( effect.player, "bonded_souls" );
  if ( !haste_buff )
  {
    haste_buff = make_buff<stat_buff_t>( effect.player, "bonded_souls", haste_buff_data )
      ->add_stat( STAT_HASTE_RATING, power.value( 1 ) )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION );
  }

  buff_t* driver_buff = buff_t::find( effect.player, "bonded_souls_driver" );
  if ( !driver_buff )
  {
    // TODO: Healing portion of the trait?
    driver_buff = make_buff( effect.player, "bonded_souls_driver", driver_buff_data )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION )
      ->set_tick_behavior( buff_tick_behavior::CLIP )
      ->set_tick_zero( true )
      ->set_tick_callback( [ haste_buff ] ( buff_t*, int, const timespan_t& )
        { haste_buff->trigger(); } );
  }

  // Spell data is missing proc flags, just pick something that's gonna be close to what the
  // tooltip says.
  effect.proc_flags_ = PF_ALL_DAMAGE | PF_ALL_HEAL;
  effect.spell_id = driver->id();
  effect.custom_buff = driver_buff;

  new dbc_proc_callback_t( effect.player, effect );
}

void fight_or_flight( special_effect_t& effect )
{
  // Callback that attempts to trigger buff if target below 35%
  struct fof_cb_t : public dbc_proc_callback_t
  {
    buff_t* fof_buff;
    double threshold;

    fof_cb_t( const special_effect_t& effect, buff_t* buff, const azerite_power_t& power ) :
      dbc_proc_callback_t( effect.player, effect ),
      fof_buff( buff ),
      threshold( power.spell() -> effectN( 1 ).trigger() -> effectN( 1 ).base_value() )
    {}

    void execute( action_t* /* a */, action_state_t* state ) override
    {
      if ( state -> target -> health_percentage() < threshold )
      {
        fof_buff -> trigger();
      }
    }
  };

  effect.proc_flags_ = PF_ALL_DAMAGE;

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  buff_t* fof_buff = buff_t::find( effect.player, "fight_or_flight" );
  if ( !fof_buff )
  {
    fof_buff = make_buff<stat_buff_t>( effect.player, "fight_or_flight", effect.player -> find_spell( 287827 ) )
      -> add_stat( effect.player -> convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 1 ) )
      // The buff's icd is handled by a debuff on the player ingame, we skip that and just use the debuff's duration as cooldown for the buff
      -> set_cooldown( effect.player -> find_spell( 287825 ) -> duration() );
  }

  // TODO: Model gaining the buff when droping below 35%.
  effect.player -> register_combat_begin( [ fof_buff ] ( player_t* )
  {
    // Simple system to allow for some manipulation of the buff uptime.
    if ( fof_buff -> sim -> bfa_opts.fight_or_flight_chance > 0.0 )
    {
      make_repeating_event( fof_buff -> sim, fof_buff -> sim -> bfa_opts.fight_or_flight_period, [ fof_buff ]
      {
        if ( fof_buff -> rng().roll( fof_buff -> sim -> bfa_opts.fight_or_flight_chance ) )
          fof_buff -> trigger();
      } );
    }
  } );

  effect.spell_id = power.spell() -> effectN( 1 ).trigger() -> id();

  new fof_cb_t( effect, fof_buff, power );
}

} // Namespace special effects ends
} // Namespace azerite ends
