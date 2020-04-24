// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "dbc.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <tuple>

using namespace hotfix;

// template machinery for "easy" handling of override & get field methods
namespace {
namespace detail {

template <size_t I>
using size_c = std::integral_constant<size_t, I>;

template <typename T, typename U>
struct data_field_t {
  util::string_view name;
  T U::* pm;
};

template <typename T, typename Fields, typename Handler, typename U>
U handle_field( T*, const Fields&, util::string_view, Handler&&, U deflt, size_c<0> ) {
  return deflt;
}

template <typename T, typename Fields, typename Handler, typename U, size_t I>
U handle_field( T* data, const Fields& fields, util::string_view name, Handler&& handle, U deflt, size_c<I> ) {
  const auto& field = std::get<I - 1>( fields );
  if ( util::str_compare_ci( name, field.name ) )
    return handle( data->*field.pm );
  return handle_field( data, fields, name, std::forward<Handler>( handle ), deflt, size_c<I - 1>{} );
}

} // namespace detail

template <typename T, typename U>
constexpr auto data_field( util::string_view n, T U::* pm ) {
  return detail::data_field_t<T, U>{ n, pm };
}

template <typename T, typename Fields>
bool override_field( T* data, const Fields& fields, util::string_view name, double value ) {
  return detail::handle_field( data, fields, name,
    [ value ] ( auto& field ) {
        field = static_cast<std::remove_reference_t<decltype(field)>>( value );
        return true;
    }, false, detail::size_c<std::tuple_size<Fields>::value>{} );
}

template <typename T, typename Fields>
double get_field( const T* data, const Fields& fields, util::string_view name ) {
  return detail::handle_field( data, fields, name,
    [] ( const auto& field ) {
      return static_cast<double>( field );
    }, -std::numeric_limits<double>::max(), detail::size_c<std::tuple_size<Fields>::value>{} );
}

} // anon namespace

// ==========================================================================
// Spell Data
// ==========================================================================

static constexpr auto spell_data_fields = std::make_tuple(
  data_field( "prj_speed",         &spell_data_t::_prj_speed ),
  data_field( "school",            &spell_data_t::_school ),
  data_field( "scaling_class",     &spell_data_t::_scaling_type ),
  data_field( "spell_level",       &spell_data_t::_spell_level ),
  data_field( "max_level",         &spell_data_t::_max_level ),
  data_field( "req_max_level",     &spell_data_t::_req_max_level ),
  data_field( "min_range",         &spell_data_t::_min_range ),
  data_field( "max_range",         &spell_data_t::_max_range ),
  data_field( "cooldown",          &spell_data_t::_cooldown ),
  data_field( "category_cooldown", &spell_data_t::_category_cooldown ),
  data_field( "internal_cooldown", &spell_data_t::_internal_cooldown ),
  data_field( "gcd",               &spell_data_t::_gcd ),
  data_field( "charges",           &spell_data_t::_charges ),
  data_field( "charge_cooldown",   &spell_data_t::_charge_cooldown ),
  data_field( "duration",          &spell_data_t::_duration ),
  data_field( "max_stack",         &spell_data_t::_max_stack ),
  data_field( "proc_chance",       &spell_data_t::_proc_chance ),
  data_field( "proc_charges",      &spell_data_t::_proc_charges ),
  data_field( "proc_flags",        &spell_data_t::_proc_flags ),
  data_field( "cast_min",          &spell_data_t::_cast_min ),
  data_field( "cast_max",          &spell_data_t::_cast_max ),
  data_field( "rppm",              &spell_data_t::_rppm ),
  data_field( "dmg_class",         &spell_data_t::_dmg_class )
);

bool spell_data_t::override_field( util::string_view field, double value )
{
  return ::override_field( this, spell_data_fields, field, value );
}

double spell_data_t::get_field( util::string_view field ) const
{
  return ::get_field( this, spell_data_fields, field );
}

// ==========================================================================
// Spell Effect Data
// ==========================================================================

static constexpr auto spelleffect_data_fields = std::make_tuple(
  data_field( "coefficient",             &spelleffect_data_t::_m_coeff ),
  data_field( "delta",                   &spelleffect_data_t::_m_delta ),
  data_field( "bonus",                   &spelleffect_data_t::_m_unk ),
  data_field( "sp_coefficient",          &spelleffect_data_t::_sp_coeff ),
  data_field( "ap_coefficient",          &spelleffect_data_t::_ap_coeff ),
  data_field( "period",                  &spelleffect_data_t::_amplitude ),
  data_field( "base_value",              &spelleffect_data_t::_base_value ),
  data_field( "misc_value1",             &spelleffect_data_t::_misc_value ),
  data_field( "misc_value2",             &spelleffect_data_t::_misc_value_2 ),
  data_field( "chain_multiplier",        &spelleffect_data_t::_m_chain ),
  data_field( "points_per_combo_points", &spelleffect_data_t::_pp_combo_points ),
  data_field( "points_per_level",        &spelleffect_data_t::_real_ppl ),
  data_field( "radius",                  &spelleffect_data_t::_radius ),
  data_field( "max_radius",              &spelleffect_data_t::_radius_max ),
  data_field( "chain_target",            &spelleffect_data_t::_chain_target )
);

bool spelleffect_data_t::override_field( util::string_view field, double value )
{
  return ::override_field( this, spelleffect_data_fields, field, value );
}

double spelleffect_data_t::get_field( util::string_view field ) const
{
  return ::get_field( this, spelleffect_data_fields, field );
}


// ==========================================================================
// Spell Power Data
// ==========================================================================

static constexpr auto spellpower_data_fields = std::make_tuple(
  data_field( "cost",              &spellpower_data_t::_cost ),
  data_field( "max_cost",          &spellpower_data_t::_cost_max ),
  data_field( "cost_per_tick",     &spellpower_data_t::_cost_per_tick ),
  data_field( "pct_cost",          &spellpower_data_t::_pct_cost ),
  data_field( "pct_cost_max",      &spellpower_data_t::_pct_cost_max ),
  data_field( "pct_cost_per_tick", &spellpower_data_t::_pct_cost_per_tick )
);

bool spellpower_data_t::override_field( util::string_view field, double value )
{
  return ::override_field( this, spellpower_data_fields, field, value );
}

double spellpower_data_t::get_field( util::string_view field ) const
{
  return ::get_field( this, spellpower_data_fields, field );
}


namespace hotfix
{
static auto_dispose< std::vector< hotfix_entry_t* > > hotfixes_;
static custom_dbc_data_t hotfix_db_;
}

// Very simple comparator, just checks for some equality in the data. There's no need for fanciful
// stuff here.
struct hotfix_comparator_t
{
  util::string_view tag, note;

  hotfix_comparator_t( util::string_view t, util::string_view n ) :
    tag( t ), note( n )
  { }

  bool operator()( const hotfix_entry_t* other )
  { return tag == other -> tag_ && note == other -> note_;  }
};

struct hotfix_sorter_t
{
  bool operator()( const hotfix_entry_t* l, const hotfix_entry_t* r )
  {
    if ( l -> group_ != r -> group_ )
    {
      return l -> group_ > r -> group_;
    }
    else
    {
      return l -> tag_ < r -> tag_;
    }
  }
};

void hotfix::apply()
{
  range::for_each( hotfixes_, []( hotfix_entry_t* entry ) { entry -> apply(); } );
}

// Return a hotfixed spell if available, otherwise return the original dbc-based spell
const spell_data_t* hotfix::find_spell( const spell_data_t* dbc_spell, bool ptr )
{
  if ( const spell_data_t* hotfixed_spell = hotfix_db_.find_spell( dbc_spell -> id(), ptr ) )
  {
    return hotfixed_spell;
  }
  return dbc_spell;
}

const spelleffect_data_t* hotfix::find_effect( const spelleffect_data_t* dbc_effect, bool ptr )
{
  if ( const spelleffect_data_t* hotfixed_effect = hotfix_db_.find_effect( dbc_effect -> id(), ptr ) )
  {
    return hotfixed_effect;
  }
  return dbc_effect;
}

spell_hotfix_entry_t& hotfix::register_spell( util::string_view group,
                                              util::string_view tag,
                                              util::string_view note,
                                              unsigned          spell_id,
                                              unsigned          flags )
{
  auto i = std::find_if( hotfixes_.begin(),
      hotfixes_.end(), hotfix_comparator_t( tag, note ) );
  if ( i !=  hotfixes_.end() )
  {
    return *static_cast<spell_hotfix_entry_t*>( *i );
  }

  auto  entry = new spell_hotfix_entry_t( group, tag, spell_id, note, flags );
  hotfixes_.push_back( entry );

  range::sort( hotfixes_, hotfix_sorter_t() );

  return *entry;
}

effect_hotfix_entry_t& hotfix::register_effect( util::string_view group,
                                                util::string_view tag,
                                                util::string_view note,
                                                unsigned          effect_id,
                                                unsigned          flags )
{
  auto i = std::find_if( hotfixes_.begin(),
      hotfixes_.end(), hotfix_comparator_t( tag, note ) );
  if ( i !=  hotfixes_.end() )
  {
    return *static_cast<effect_hotfix_entry_t*>( *i );
  }

  auto  entry = new effect_hotfix_entry_t( group, tag, effect_id, note, flags );
  hotfixes_.push_back( entry );

  range::sort( hotfixes_, hotfix_sorter_t() );

  return *entry;
}

power_hotfix_entry_t& hotfix::register_power( util::string_view group,
                                              util::string_view tag,
                                              util::string_view note,
                                              unsigned          power_id,
                                              unsigned          flags )
{
  auto i = std::find_if( hotfixes_.begin(),
      hotfixes_.end(), hotfix_comparator_t( tag, note ) );
  if ( i !=  hotfixes_.end() )
  {
    return *static_cast<power_hotfix_entry_t*>( *i );
  }

  auto  entry = new power_hotfix_entry_t( group, tag, power_id, note, flags );
  hotfixes_.push_back( entry );

  range::sort( hotfixes_, hotfix_sorter_t() );

  return *entry;
}

std::string hotfix::to_str( bool ptr )
{
  std::stringstream s;
  std::string current_group;
  bool first_group = true;

  for ( size_t i = 0; i < hotfixes_.size(); ++i )
  {
    const hotfix_entry_t* entry = hotfixes_[ hotfixes_.size() - 1 - i ];
    if ( entry -> flags_ & HOTFIX_FLAG_QUIET )
    {
      continue;
    }

    if ( ptr && ! ( entry -> flags_ & HOTFIX_FLAG_PTR ) )
    {
      continue;
    }

    if ( ! ptr && ! ( entry -> flags_ & HOTFIX_FLAG_LIVE ) )
    {
      continue;
    }

    if ( current_group != entry -> group_ )
    {
      if ( ! first_group )
      {
        s << std::endl;
      }
      s << entry -> group_ << std::endl;
      first_group = false;
      current_group = entry -> group_;
    }
    s << entry -> to_str() << std::endl;
  }

  return s.str();
}

std::vector<const hotfix_entry_t*> hotfix::hotfix_entries()
{
  std::vector<const hotfix_entry_t*> data;
  for ( size_t i = 0; i < hotfixes_.size(); ++i )
  {
    data.push_back( hotfixes_[ i ] );
  }

  return data;
}

template<typename T>
static void do_hotfix( dbc_hotfix_entry_t* e, T* dbc_data )
{
  bool success;
  switch ( e -> operation_ )
  {
    case HOTFIX_SET:
      success = dbc_data -> override_field( e -> field_name_, e -> modifier_ );
      break;
    case HOTFIX_ADD:
      success = dbc_data -> override_field( e -> field_name_, e -> dbc_value_ + e -> modifier_ );
      break;
    case HOTFIX_MUL:
      success = dbc_data -> override_field( e -> field_name_, e -> dbc_value_ * e -> modifier_ );
      break;
    case HOTFIX_DIV:
      success = dbc_data -> override_field( e -> field_name_, e -> dbc_value_ / e -> modifier_ );
      break;
    default:
      success = false;
      break;
  }
  assert( success && "Could not override field. Wrong field name?" );
  (void) success;

  e -> hotfix_value_ = dbc_data -> get_field( e -> field_name_ );
}

std::string hotfix_entry_t::to_str() const
{
  std::stringstream s;
  s << "[" << tag_.substr( 0, 10 ) << "] " << note_;

  return s.str();
}

std::string spell_hotfix_entry_t::to_str() const
{
  std::stringstream s;
  s << hotfix_entry_t::to_str();
  s << std::endl;

  const spell_data_t* spell = hotfix_db_.find_spell( id_, false );
  s << "             ";
  s << "Spell: " << spell -> name_cstr();
  s << " | Field: " << field_name_;
  s << " | Hotfix Value: " << hotfix_value_;
  s << " | DBC Value: " << dbc_value_;
  if ( orig_value_ != -std::numeric_limits<double>::max() &&
       util::round( orig_value_, 6 ) != util::round( dbc_value_, 6 ) )
  {
    s << " [verification fail]";
  }

  return s.str();
}

std::string effect_hotfix_entry_t::to_str() const
{
  std::stringstream s;
  s << hotfix_entry_t::to_str();
  s << std::endl;

  const spelleffect_data_t* e = hotfix_db_.find_effect( id_, false );
  s << "             ";
  s << "Spell: " << e -> spell() -> name_cstr();
  s << " effect#" << ( e -> index() + 1 );
  s << " | Field: " << field_name_;
  s << " | Hotfix Value: " << hotfix_value_;
  s << " | DBC Value: " << dbc_value_;
  if ( orig_value_ != -std::numeric_limits<double>::max() &&
       util::round( orig_value_, 6 ) != util::round( dbc_value_, 6 ) )
  {
    s << " [verification fail]";
  }

  return s.str();
}

std::string power_hotfix_entry_t::to_str() const
{
  std::stringstream s;
  s << hotfix_entry_t::to_str();
  s << std::endl;

  const spellpower_data_t* p = hotfix_db_.find_power( id_, false );
  s << "             ";
  s << "Power: " << p -> id();
  s << " | Field: " << field_name_;
  s << " | Hotfix Value: " << hotfix_value_;
  s << " | DBC Value: " << dbc_value_;
  if ( orig_value_ != -std::numeric_limits<double>::max() &&
       util::round( orig_value_, 6 ) != util::round( dbc_value_, 6 ) )
  {
    s << " [verification fail]";
  }

  return s.str();
}

void spell_hotfix_entry_t::apply_hotfix( bool ptr )
{
  spell_data_t* s = hotfix_db_.clone_spell( id_, ptr );

  assert( s && "Could not clone spell to apply hotfix" );

  // Record original DBC value before overwriting it
  dbc_value_ = s -> get_field( field_name_ );

  if ( ! valid() )
  {
    std::cerr << "[" << tag_ << "]: " << ( ptr ? "PTR-" : "" ) << "Hotfix \"" << note_ << "\" for spell \"" << s -> name_cstr() <<
                 "\" does not match verification value.";
    std::cerr << " Field: " << field_name_;
    std::cerr << ", DBC: " << util::round( dbc_value_, 5 );
    std::cerr << ", Verify: " << util::round( orig_value_, 5 );
    std::cerr << std::endl;
    return;
  }

  do_hotfix( this, s );
}

void effect_hotfix_entry_t::apply_hotfix( bool ptr )
{
  const spelleffect_data_t* source_effect = spelleffect_data_t::find( id_, ptr );

  // Cloning the spell chain will guarantee that the effect is also always cloned
  const spell_data_t* s = hotfix_db_.clone_spell( source_effect -> spell() -> id(), ptr );
  assert( s && "Could not clone spell to apply hotfix" );

  spelleffect_data_t* e = hotfix_db_.get_mutable_effect( id_, ptr );
  if ( !e ) { return; }

  dbc_value_ = e -> get_field( field_name_ );
  if ( ! valid() )
  {
    std::cerr << "[" << tag_ << "]: " << ( ptr ? "PTR-" : "" ) << "Hotfix \"" << note_ << "\" for spell \"" << s -> name_cstr() <<
                 "\" effect #" << ( e -> index() + 1 ) << " does not match verification value.";
    std::cerr << " Field: " << field_name_;
    std::cerr << ", DBC: " << std::setprecision( 5 ) << util::round( dbc_value_, 5 );
    std::cerr << ", Verify: " << std::setprecision( 5 ) << util::round( orig_value_, 5 );
    std::cerr << std::endl;
    return;
  }

  do_hotfix( this, e );
}

void power_hotfix_entry_t::apply_hotfix( bool ptr )
{
  const spellpower_data_t& source_power = spellpower_data_t::find( id_, ptr );

  // Cloning the spell chain will guarantee that the effect is also always cloned
  const spell_data_t* s = hotfix_db_.clone_spell( source_power.spell_id(), ptr );
  assert( s && "Could not clone spell to apply hotfix" );

  spellpower_data_t* p = hotfix_db_.get_mutable_power( id_, ptr );
  if ( !p ) { return; }

  dbc_value_ = p -> get_field( field_name_ );
  if ( ! valid() )
  {
    std::cerr << "[" << tag_ << "]: " << ( ptr ? "PTR-" : "" ) << "Hotfix \"" << note_
      << "\" for spell power " << p -> id() << " (spell " << s -> name_cstr() << ") does not match verification value.";
    std::cerr << " Field: " << field_name_;
    std::cerr << ", DBC: " << std::setprecision( 5 ) << util::round( dbc_value_, 5 );
    std::cerr << ", Verify: " << std::setprecision( 5 ) << util::round( orig_value_, 5 );
    std::cerr << std::endl;
    return;
  }

  do_hotfix( this, p );
}

spell_data_t* custom_dbc_data_t::get_mutable_spell( unsigned spell_id, bool ptr )
{
  for ( size_t i = 0, end = spells_[ ptr ].size(); i < end; ++i )
  {
    if ( spells_[ ptr ][ i ] -> id() == spell_id )
    {
      return spells_[ ptr ][ i ];
    }
  }

  return nullptr;
}

const spell_data_t* custom_dbc_data_t::find_spell( unsigned spell_id, bool ptr ) const
{
  for ( size_t i = 0, end = spells_[ ptr ].size(); i < end; ++i )
  {
    if ( spells_[ ptr ][ i ] -> id() == spell_id )
    {
      return spells_[ ptr ][ i ];
    }
  }

  return nullptr;
}

bool custom_dbc_data_t::add_spell( spell_data_t* spell, bool ptr )
{
  if ( find_spell( spell -> id(), ptr ) )
  {
    return false;
  }

  spells_[ ptr ].push_back( spell );

  return true;
}

spelleffect_data_t* custom_dbc_data_t::get_mutable_effect( unsigned effect_id, bool ptr )
{
  for ( size_t i = 0, end = effects_[ ptr ].size(); i < end; ++i )
  {
    if ( effects_[ ptr ][ i ] -> id() == effect_id )
    {
      return effects_[ ptr ][ i ];
    }
  }

  return nullptr;
}

const spelleffect_data_t* custom_dbc_data_t::find_effect( unsigned effect_id, bool ptr ) const
{
  for ( size_t i = 0, end = effects_[ ptr ].size(); i < end; ++i )
  {
    if ( effects_[ ptr ][ i ] -> id() == effect_id )
    {
      return effects_[ ptr ][ i ];
    }
  }

  return nullptr;
}

bool custom_dbc_data_t::add_effect( spelleffect_data_t* effect, bool ptr )
{
  if ( find_effect( effect -> id(), ptr ) )
  {
    return false;
  }

  effects_[ ptr ].push_back( effect );

  return true;
}

spellpower_data_t* custom_dbc_data_t::get_mutable_power( unsigned power_id, bool ptr )
{
  for ( size_t i = 0, end = powers_[ ptr ].size(); i < end; ++i )
  {
    if ( powers_[ ptr ][ i ] -> id() == power_id )
    {
      return powers_[ ptr ][ i ];
    }
  }

  return nullptr;
}

const spellpower_data_t* custom_dbc_data_t::find_power( unsigned power_id, bool ptr ) const
{
  auto it = range::find_if( powers_[ ptr ], [ power_id ]( spellpower_data_t* power ) {
      return power -> id() == power_id;
  } );

  if ( it != powers_[ ptr ].end() )
  {
    return *it;
  }

  return nullptr;
}

bool custom_dbc_data_t::add_power( spellpower_data_t* power, bool ptr )
{
  if ( find_power( power -> id(), ptr ) )
  {
    return false;
  }

  powers_[ ptr ].push_back( power );

  return true;
}

static void collect_base_spells( const spell_data_t* spell, std::vector<const spell_data_t*>& roots )
{
  if ( range::find( roots, spell ) != roots.end() )
    return;

  roots.push_back( spell );

  for ( auto driver_spell : spell -> drivers() )
  {
    // Safeguard infinite recursions
    if ( range::find( roots, driver_spell ) != roots.end() )
    {
      continue;
    }

    collect_base_spells( driver_spell, roots );
  }
}

spell_data_t* custom_dbc_data_t::create_clone( const spell_data_t* source, bool ptr )
{
  spell_data_t* clone = get_mutable_spell( source -> id(), ptr );
  if ( clone )
  {
    return clone;
  }

  clone = new spell_data_t( *source );

  clone -> _effects = new std::vector<const spelleffect_data_t*>( clone -> effect_count(), &spelleffect_data_t::nil() );

  const spellpower_data_t** clone_power = nullptr;
  if ( source -> power_count() > 0 )
  {
    clone_power = allocator_.create_n<const spellpower_data_t*>( source -> power_count(), &spellpower_data_t::nil() );
    clone -> _power = clone_power;
  }

  // Drivers are set up in the parent's cloning of the trigger spell
  clone -> _driver = nullptr;
  clone -> _driver_count = 0;
  add_spell( clone, ptr );

  // Clone effects
  const auto source_effects = source -> effects();
  for ( size_t i = 0; i < source_effects.size(); ++i )
  {
    const spelleffect_data_t* e_source = source_effects[ i ];
    if ( e_source -> id() == 0 )
    {
      continue;
    }

    spelleffect_data_t* e_clone = get_mutable_effect( e_source -> id(), ptr );

    if ( ! e_clone )
    {
      e_clone = new spelleffect_data_t( *spelleffect_data_t::find( e_source -> id(), ptr ) );
      add_effect( e_clone, ptr );
    }

    // Link cloned effect to cloned spell, and cloned spell to cloned effect
    clone -> _effects -> at( i ) = e_clone;
    e_clone -> _spell = clone;

    // No trigger set up in the source effect, so processing for this effect can end here.
    if ( e_source -> trigger() -> id() == 0 || e_source -> trigger() -> drivers().empty() )
    {
      continue;
    }

    // Clone the trigger, and re-link drivers in the trigger spell so they also point to cloned
    // data. This is necessary because Blizzard re-uses trigger spells in multiple drivers.
    e_clone -> _trigger_spell = create_clone( e_source -> trigger(), ptr );
    assert( e_source -> trigger() -> _driver );

    const auto e_source_trigger_drivers = e_source -> trigger() -> drivers();

    auto& e_clone_trigger_drivers = spell_driver_map_[ ptr ][ e_source -> trigger() -> id() ];
    if ( e_clone_trigger_drivers.empty() )
    {
      auto driver_data = allocator_.create_n<const spell_data_t*>( e_source_trigger_drivers.size(), spell_data_t::nil() );
      e_clone_trigger_drivers = { driver_data, e_source_trigger_drivers.size() };
      e_clone -> _trigger_spell -> _driver = e_clone_trigger_drivers.data();
      e_clone -> _trigger_spell -> _driver_count = as<uint8_t>( e_clone_trigger_drivers.size() );
    }

    auto it = range::find( e_source_trigger_drivers, clone -> id(), &spell_data_t::id );
    if ( it != e_source_trigger_drivers.end() )
      e_clone_trigger_drivers[ std::distance( e_source_trigger_drivers.begin(), it ) ] = clone;
  }

  // Clone powers
  const auto source_powers = source -> powers();
  for ( size_t i = 0; i < source_powers.size(); ++i )
  {
    auto p_source = source_powers[ i ];
    if ( p_source -> id() == 0 )
    {
      continue;
    }

    auto p_clone = get_mutable_power( p_source -> id(), ptr );
    if ( p_clone == nullptr )
    {
      p_clone = new spellpower_data_t( spellpower_data_t::find( p_source -> id(), ptr ) );
      add_power( p_clone, ptr );
    }

    clone_power[ i ] = p_clone;
  }

  return clone;
}

spell_data_t* custom_dbc_data_t::clone_spell( unsigned clone_spell_id, bool ptr )
{
  spell_data_t* c = get_mutable_spell( clone_spell_id, ptr );
  // If a spell is found, we can be sure that the whole spell chain has already been cloned, so just
  // return the base spell
  if ( c )
  {
    return c;
  }
  else
  {
    c = spell_data_t::find( clone_spell_id, ptr );
  }

  // Get to the root of the potential chain
  std::vector<const spell_data_t*> base_spells;
  collect_base_spells( c, base_spells );
  for ( auto base_spell : base_spells )
  {
    // Create clones of the base spell chains, we don't really care about individual spells for now
    create_clone( base_spell, ptr );
  }

  c = get_mutable_spell( clone_spell_id, ptr );
  // Return the cloned spell
  return c;
}

custom_dbc_data_t::~custom_dbc_data_t()
{
  for ( size_t i = 0; i < spells_[ 0 ].size(); ++i )
  {
    delete spells_[ 0 ][ i ] -> _effects;
  }

  for ( size_t i = 0; i < spells_[ 1 ].size(); ++i )
  {
    delete spells_[ 1 ][ i ] -> _effects;
  }
}

namespace dbc_override
{
  custom_dbc_data_t override_db_;
  std::vector<dbc_override_entry_t> override_entries_;
}

// Applies overrides immediately, and records an entry
void dbc_override::register_spell( dbc_t& dbc, unsigned spell_id, util::string_view field, double v )
{
  spell_data_t* spell = override_db_.clone_spell( spell_id, dbc.ptr );
  if (!spell)
  {
    throw std::invalid_argument("Could not find spell");
  }
  if (!spell -> override_field( field, v ))
  {
    throw std::invalid_argument(fmt::format("Invalid field '{}'.", field));
  }

  override_entries_.emplace_back( DBC_OVERRIDE_SPELL, field, spell_id, v );
}

void dbc_override::register_effect( dbc_t& dbc, unsigned effect_id, util::string_view field, double v )
{
  spelleffect_data_t* effect = override_db_.get_mutable_effect( effect_id, dbc.ptr );
  if ( ! effect )
  {
    const spelleffect_data_t* dbc_effect = dbc.effect( effect_id );
    override_db_.clone_spell( dbc_effect -> spell() -> id(), dbc.ptr );
    effect = override_db_.get_mutable_effect( effect_id, dbc.ptr );
  }
  if (!effect)
  {
    throw std::runtime_error("Could not find effect");
  }

  if (!effect -> override_field( field, v ))
  {
    throw std::invalid_argument(fmt::format("Invalid field '{}'.", field));
  }

  override_entries_.emplace_back( DBC_OVERRIDE_EFFECT, field, effect_id, v );
}

void dbc_override::register_power( dbc_t& dbc, unsigned power_id, util::string_view field, double v )
{
  auto power = override_db_.get_mutable_power( power_id, dbc.ptr );
  if ( power == nullptr )
  {
    const auto& dbc_power = dbc.power( power_id );
    override_db_.clone_spell( dbc_power.spell_id(), dbc.ptr );
    power = override_db_.get_mutable_power( power_id, dbc.ptr );
  }
  if (!power)
  {
    throw std::runtime_error("Could not find power");
  }

  if (!power -> override_field( field, v ))
  {
    throw std::invalid_argument(fmt::format("Invalid field '{}'.", field));
  }

  override_entries_.emplace_back( DBC_OVERRIDE_POWER, field, power_id, v );
}

const spell_data_t* dbc_override::find_spell( unsigned spell_id, bool ptr )
{
  return override_db_.find_spell( spell_id, ptr );
}

const spelleffect_data_t* dbc_override::find_effect( unsigned effect_id, bool ptr )
{
  return override_db_.find_effect( effect_id, ptr );
}

const std::vector<dbc_override::dbc_override_entry_t>& dbc_override::override_entries()
{ return override_entries_; }

