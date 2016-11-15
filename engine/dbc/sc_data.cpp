// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

using namespace hotfix;

// ==========================================================================
// Spell Data
// ==========================================================================

spell_data_nil_t spell_data_nil_t::singleton;

spell_data_not_found_t spell_data_not_found_t::singleton;

// spell_data_t::override ===================================================

bool spell_data_t::override_field( const std::string& field, double value )
{
  if ( util::str_compare_ci( field, "prj_speed" ) )
    _prj_speed = value;
  else if ( util::str_compare_ci( field, "school" ) )
    _school = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "scaling_class" ) )
    _scaling_type = ( int ) value;
  else if ( util::str_compare_ci( field, "spell_level" ) )
    _spell_level = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "max_level" ) )
    _max_level = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "min_range" ) )
    _min_range = value;
  else if ( util::str_compare_ci( field, "max_range" ) )
    _max_range = value;
  else if ( util::str_compare_ci( field, "cooldown" ) )
    _cooldown = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "category_cooldown" ) )
    _category_cooldown = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "internal_cooldown" ) )
    _internal_cooldown = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "gcd" ) )
    _gcd = ( unsigned ) value;
  else if ( util::str_compare_ci(field, "charges") )
    _charges = ( unsigned ) value;
  else if ( util::str_compare_ci(field, "charge_cooldown") )
    _charge_cooldown = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "duration" ) )
    _duration = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "max_stack" ) )
    _max_stack = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "proc_chance" ) )
    _proc_chance = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "proc_charges" ) )
    _proc_charges = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "proc_flags") )
    _proc_flags = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "cast_min" ) )
    _cast_min = ( int ) value;
  else if ( util::str_compare_ci( field, "cast_max" ) )
    _cast_max = ( int ) value;
  else if ( util::str_compare_ci( field, "rppm" ) )
    _rppm = value;
  else
    return false;
  return true;
}

double spell_data_t::get_field( const std::string& field ) const
{
  if ( util::str_compare_ci( field, "prj_speed" ) )
    return _prj_speed;
  else if ( util::str_compare_ci( field, "school" ) )
    return static_cast<double>( _school );
  else if ( util::str_compare_ci( field, "scaling_class" ) )
    return static_cast<double>( _scaling_type );
  else if ( util::str_compare_ci( field, "spell_level" ) )
    return static_cast<double>( _spell_level );
  else if ( util::str_compare_ci( field, "max_level" ) )
    return static_cast<double>( _max_level );
  else if ( util::str_compare_ci( field, "min_range" ) )
    return _min_range;
  else if ( util::str_compare_ci( field, "max_range" ) )
    return _max_range;
  else if ( util::str_compare_ci( field, "cooldown" ) )
    return static_cast<double>( _cooldown );
  else if ( util::str_compare_ci( field, "category_cooldown" ) )
    return static_cast<double>( _category_cooldown );
  else if ( util::str_compare_ci( field, "internal_cooldown" ) )
    return static_cast<double>( _internal_cooldown );
  else if ( util::str_compare_ci( field, "gcd" ) )
    return static_cast<double>( _gcd );
  else if ( util::str_compare_ci( field, "charges" ) )
    return static_cast<double>( _charges );
  else if ( util::str_compare_ci( field, "charge_cooldown" ) )
    return static_cast<double>( _charge_cooldown );
  else if ( util::str_compare_ci( field, "duration" ) )
    return static_cast<double>( _duration );
  else if ( util::str_compare_ci( field, "max_stack" ) )
    return static_cast<double>( _max_stack );
  else if ( util::str_compare_ci( field, "proc_chance" ) )
    return static_cast<double>( _proc_chance );
  else if ( util::str_compare_ci( field, "proc_charges" ) )
    return static_cast<double>( _proc_charges );
  else if ( util::str_compare_ci( field, "cast_min" ) )
    return static_cast<double>( _cast_min );
  else if ( util::str_compare_ci( field, "cast_max" ) )
    return static_cast<double>( _cast_max );
  else if ( util::str_compare_ci( field, "rppm" ) )
    return _rppm;

  return -std::numeric_limits<double>::max();
}

// ==========================================================================
// Spell Effect Data
// ==========================================================================

spelleffect_data_nil_t spelleffect_data_nil_t::singleton;

bool spelleffect_data_t::override_field( const std::string& field, double value )
{
  if ( util::str_compare_ci( field, "average" ) )
    _m_avg = value;
  else if ( util::str_compare_ci( field, "delta" ) )
    _m_delta = value;
  else if ( util::str_compare_ci( field, "bonus" ) )
    _m_unk = value;
  else if ( util::str_compare_ci( field, "sp_coefficient" ) )
    _sp_coeff = value;
  else if ( util::str_compare_ci( field, "ap_coefficient" ) )
    _ap_coeff = value;
  else if ( util::str_compare_ci( field, "period" ) )
    _amplitude = value;
  else if ( util::str_compare_ci( field, "base_value" ) )
    _base_value = ( int ) value;
  else if ( util::str_compare_ci( field, "misc_value1" ) )
    _misc_value = ( int ) value;
  else if ( util::str_compare_ci( field, "misc_value2" ) )
    _misc_value_2 = ( int ) value;
  else if ( util::str_compare_ci( field, "chain_multiplier" ) )
    _m_chain = value;
  else if ( util::str_compare_ci( field, "points_per_combo_points" ) )
    _pp_combo_points = value;
  else if ( util::str_compare_ci( field, "points_per_level" ) )
    _real_ppl = value;
  else if ( util::str_compare_ci( field, "die_sides" ) )
    _die_sides = ( int ) value;
  else if ( util::str_compare_ci( field, "radius" ) )
    _radius = value;
  else if ( util::str_compare_ci( field, "max_radius" ) )
    _radius_max = value;
  else if ( util::str_compare_ci( field, "chain_target" ) )
    _chain_target = ( unsigned ) value;
  else
    return false;
  return true;
}

double spelleffect_data_t::get_field( const std::string& field ) const
{
  if ( util::str_compare_ci( field, "average" ) )
    return _m_avg;
  else if ( util::str_compare_ci( field, "delta" ) )
    return _m_delta;
  else if ( util::str_compare_ci( field, "bonus" ) )
    return _m_unk;
  else if ( util::str_compare_ci( field, "sp_coefficient" ) )
    return _sp_coeff;
  else if ( util::str_compare_ci( field, "ap_coefficient" ) )
    return _ap_coeff;
  else if ( util::str_compare_ci( field, "period" ) )
    return _amplitude;
  else if ( util::str_compare_ci( field, "base_value" ) )
    return static_cast<double>( _base_value );
  else if ( util::str_compare_ci( field, "misc_value1" ) )
    return static_cast<double>( _misc_value );
  else if ( util::str_compare_ci( field, "misc_value2" ) )
    return static_cast<double>( _misc_value_2 );
  else if ( util::str_compare_ci( field, "chain_multiplier" ) )
    return _m_chain;
  else if ( util::str_compare_ci( field, "points_per_combo_points" ) )
    return _pp_combo_points;
  else if ( util::str_compare_ci( field, "points_per_level" ) )
    return _real_ppl;
  else if ( util::str_compare_ci( field, "die_sides" ) )
    return static_cast<double>( _die_sides );
  else if ( util::str_compare_ci( field, "radius" ) )
    return _radius;
  else if ( util::str_compare_ci( field, "max_radius" ) )
    return _radius_max;
  else if ( util::str_compare_ci( field, "chain_target" ) )
    return static_cast<double>( _chain_target );

  return -std::numeric_limits<double>::max();
}


// ==========================================================================
// Spell Power Data
// ==========================================================================

spellpower_data_nil_t spellpower_data_nil_t::singleton;

// ==========================================================================
// Talent Data
// ==========================================================================

talent_data_nil_t talent_data_nil_t::singleton;

namespace hotfix
{
static auto_dispose< std::vector< hotfix_entry_t* > > hotfixes_;
static custom_dbc_data_t hotfix_db_;
}

// Very simple comparator, just checks for some equality in the data. There's no need for fanciful
// stuff here.
struct hotfix_comparator_t
{
  const std::string& tag, &note;

  hotfix_comparator_t( const std::string& t, const std::string& n ) :
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

bool hotfix::register_hotfix( const std::string& group,
                              const std::string& tag,
                              const std::string& note,
                              unsigned           flags )
{
  if ( std::find_if( hotfixes_.begin(), hotfixes_.end(), hotfix_comparator_t( tag, note ) ) !=
       hotfixes_.end() )
  {
    return false;
  }

  hotfixes_.push_back( new hotfix_entry_t( group, tag, note, flags ) );

  return true;
}

void hotfix::apply()
{
  for ( size_t i = 0; i < hotfixes_.size(); ++i )
  {
    hotfixes_[ i ] -> apply();
  }
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

spell_hotfix_entry_t& hotfix::register_spell( const std::string& group,
                                              const std::string& tag,
                                              const std::string& note,
                                              unsigned           spell_id,
                                              unsigned           flags )
{
  auto i = std::find_if( hotfixes_.begin(),
      hotfixes_.end(), hotfix_comparator_t( tag, note ) );
  if ( i !=  hotfixes_.end() )
  {
    return *static_cast<spell_hotfix_entry_t*>( *i );
  }

  auto  entry = new spell_hotfix_entry_t( group, tag, spell_id, note, flags );
  hotfixes_.push_back( entry );

  std::sort( hotfixes_.begin(), hotfixes_.end(), hotfix_sorter_t() );

  return *entry;
}

effect_hotfix_entry_t& hotfix::register_effect( const std::string& group,
                                                const std::string& tag,
                                                const std::string& note,
                                                unsigned           effect_id,
                                                unsigned           flags )
{
  auto i = std::find_if( hotfixes_.begin(),
      hotfixes_.end(), hotfix_comparator_t( tag, note ) );
  if ( i !=  hotfixes_.end() )
  {
    return *static_cast<effect_hotfix_entry_t*>( *i );
  }

  auto  entry = new effect_hotfix_entry_t( group, tag, effect_id, note, flags );
  hotfixes_.push_back( entry );

  std::sort( hotfixes_.begin(), hotfixes_.end(), hotfix_sorter_t() );

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

void spell_hotfix_entry_t::apply_hotfix( bool ptr )
{
  spell_data_t* s = hotfix_db_.clone_spell( id_, ptr );

  assert( s && "Could not clone spell to apply hotfix" );

  // Record original DBC value before overwriting it
  dbc_value_ = s -> get_field( field_name_ );

  if ( orig_value_ != -std::numeric_limits<double>::max() &&
       util::round( orig_value_, 5 ) != util::round( dbc_value_, 5 ) )
  {
    std::cerr << "[" << tag_ << "]: " << ( ptr ? "PTR-" : "" ) << "Hotfix \"" << note_ << "\" for spell \"" << s -> name_cstr() <<
                 "\" does not match verification value.";
    std::cerr << " Field: " << field_name_;
    std::cerr << ", DBC: " << util::round( dbc_value_, 5 );
    std::cerr << ", Verify: " << util::round( orig_value_, 5 );
    std::cerr << std::endl;
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
  if ( orig_value_ != -std::numeric_limits<double>::max() &&
       util::round( orig_value_, 5 ) != util::round( dbc_value_, 5 ) )
  {
    std::cerr << "[" << tag_ << "]: " << ( ptr ? "PTR-" : "" ) << "Hotfix \"" << note_ << "\" for spell \"" << s -> name_cstr() <<
                 "\" effect #" << ( e -> index() + 1 ) << " does not match verification value.";
    std::cerr << " Field: " << field_name_;
    std::cerr << ", DBC: " << std::setprecision( 5 ) << util::round( dbc_value_, 5 );
    std::cerr << ", Verify: " << std::setprecision( 5 ) << util::round( orig_value_, 5 );
    std::cerr << std::endl;
  }

  do_hotfix( this, e );
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

static void collect_base_spells( const spell_data_t* spell, std::vector<const spell_data_t*>& roots )
{
  if ( ! spell -> _driver )
  {
    if ( range::find( roots, spell ) == roots.end() )
    {
      roots.push_back( spell );
    }
  }
  else
  {
    for ( auto driver_spell : *spell -> _driver )
    {
      // Safeguard infinite recursions
      if ( range::find( roots, driver_spell ) != roots.end() )
      {
        continue;
      }

      collect_base_spells( driver_spell, roots );
    }
  }
}

spell_data_t* custom_dbc_data_t::create_clone( const spell_data_t* source, bool ptr )
{
  spell_data_t* clone = get_mutable_spell( source -> id(), ptr );
  if ( ! clone )
  {
    clone = new spell_data_t( *source );
    // TODO: Power, not overridable atm so we can use the static data, and the static data vector
    // too.
    clone -> _effects = new std::vector<const spelleffect_data_t*>( clone -> effect_count(), spelleffect_data_t::nil() );
    // Drivers are set up in the parent's cloning of the trigger spell
    clone -> _driver = 0;
    add_spell( clone, ptr );
  }

  // Clone effects
  for ( size_t i = 0; i < source -> _effects -> size(); ++i )
  {
    if ( source -> _effects -> at( i ) -> id() == 0 )
    {
      continue;
    }

    const spelleffect_data_t* e_source = source -> _effects -> at( i );
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
    if ( e_source -> trigger() -> id() == 0 )
    {
      continue;
    }

    // Clone the trigger, and re-link drivers in the trigger spell so they also point to cloned
    // data. This is necessary because Blizzard re-uses trigger spells in multiple drivers.
    e_clone -> _trigger_spell = create_clone( e_source -> trigger(), ptr );
    assert( e_source -> trigger() -> _driver );
    if ( ! e_clone -> _trigger_spell -> _driver )
    {
      e_clone -> _trigger_spell -> _driver = new std::vector<spell_data_t*>( e_source -> trigger() -> n_drivers(), spell_data_t::nil() );
    }

    for ( size_t driver_idx = 0; driver_idx < e_source -> trigger() -> n_drivers(); ++driver_idx )
    {
      const spell_data_t* driver = e_source -> trigger() -> driver( driver_idx );
      if ( driver -> id() == clone -> id() )
      {
        e_clone -> _trigger_spell -> _driver -> at( driver_idx ) = clone;
        break;
      }
    }
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
  assert( c );
  // Return the cloned spell
  return c;
}

custom_dbc_data_t::~custom_dbc_data_t()
{
  for ( size_t i = 0; i < spells_[ 0 ].size(); ++i )
  {
    range::for_each( *spells_[ 0 ][ i ] -> _effects, []( const spelleffect_data_t* e ) {
      if ( e && e -> _trigger_spell -> id() > 0 )
      {
        delete e -> _trigger_spell -> _driver;
        e -> _trigger_spell -> _driver = nullptr;
      }
    } );
    delete spells_[ 0 ][ i ] -> _effects;
  }

  for ( size_t i = 0; i < spells_[ 1 ].size(); ++i )
  {
    range::for_each( *spells_[ 1 ][ i ] -> _effects, []( const spelleffect_data_t* e ) {
      if ( e && e -> _trigger_spell -> id() > 0 )
      {
        delete e -> _trigger_spell -> _driver;
        e -> _trigger_spell -> _driver = nullptr;
      }
    } );
    delete spells_[ 1 ][ i ] -> _effects;
  }
}

namespace dbc_override
{
  custom_dbc_data_t override_db_;
  std::vector<dbc_override_entry_t> override_entries_;
}

// Applies overrides immediately, and records an entry
bool dbc_override::register_spell( dbc_t& dbc, unsigned spell_id, const std::string& field, double v )
{
  spell_data_t* spell = override_db_.get_mutable_spell( spell_id, dbc.ptr );
  if ( ! spell )
  {
    spell = override_db_.clone_spell( spell_id, dbc.ptr );
  }

  assert( spell );
  spell -> override_field( field, v );

  override_entries_.push_back( dbc_override_entry_t( DBC_OVERRIDE_SPELL, field, spell_id, v ) );

  return true;
}

bool dbc_override::register_effect( dbc_t& dbc, unsigned effect_id, const std::string& field, double v )
{
  spelleffect_data_t* effect = override_db_.get_mutable_effect( effect_id, dbc.ptr );
  if ( ! effect )
  {
    const spelleffect_data_t* dbc_effect = dbc.effect( effect_id );
    override_db_.clone_spell( dbc_effect -> spell() -> id(), dbc.ptr );
    effect = override_db_.get_mutable_effect( effect_id, dbc.ptr );
  }

  assert( effect );

  effect -> override_field( field, v );

  override_entries_.push_back( dbc_override_entry_t( DBC_OVERRIDE_EFFECT, field, effect_id, v ) );

  return true;
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

