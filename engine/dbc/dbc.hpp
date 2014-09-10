// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SIMULATIONCRAFT_H
static_assert( 0 , "dbc.hpp included into a file where SIMULATIONCRAFT_H is not defined!" );
/* This Header cannot stand on its own feet.
 * It only works when included into simulationcraft.hpp at a specific place.
 * The purpose (for now) is only to have it sourced out into a separate file.
 */
#endif

#ifndef SC_DBC_HPP
#define SC_DBC_HPP

#include "data_definitions.hh"
#include "data_enums.hh"

// This is a automatically generated header.
#include "specialization.hpp"

static const unsigned NUM_SPELL_FLAGS = 12;
static const unsigned NUM_CLASS_FAMILY_FLAGS = 4;

// ==========================================================================
// General Database
// ==========================================================================

namespace dbc {
// Initialization
void apply_hotfixes();
void init();
void de_init();

// Utily functions
uint32_t get_school_mask( school_e s );
school_e get_school_type( uint32_t school_id );
bool is_school( school_e s, school_e s2 );
unsigned specialization_max_per_class();
specialization_e spec_by_idx( const player_e c, unsigned idx );

// Data Access
int build_level( bool ptr );
const char* wow_version( bool ptr );
const char* wow_ptr_status( bool ptr );
const item_data_t* items( bool ptr );
std::size_t        n_items( bool ptr );
const item_set_bonus_t* set_bonus( bool ptr );
std::size_t             n_set_bonus( bool ptr );
const item_enchantment_data_t* item_enchantments( bool ptr );
std::size_t        n_item_enchantments( bool ptr );
const gem_property_data_t* gem_properties( bool ptr );
specialization_e translate_spec_str   ( player_e ptype, const std::string& spec_str );
std::string specialization_string     ( specialization_e spec );
double fmt_value( double v, effect_type_t type, effect_subtype_t sub_type );
const std::string& get_token( unsigned int id_spell );
bool add_token( unsigned int id_spell, const std::string& token_name, bool ptr );
unsigned int get_token_id( const std::string& token );
bool valid_gem_color( unsigned color );
}

// ==========================================================================
// Spell Power Data - SpellPower.dbc
// ==========================================================================

struct spellpower_data_t
{
public:
  unsigned _id;
  unsigned _spell_id;
  unsigned _aura_id; // Spell id for the aura during which this power type is active
  int      _power_e;
  int      _cost;
  double   _cost_2;
  int      _cost_per_second; // Unsure
  double   _cost_per_second_2;

  resource_e resource() const
  { return util::translate_power_type( type() ); }

  unsigned id() const
  { return _id; }

  unsigned spell_id() const
  { return _spell_id; }

  unsigned aura_id() const
  { return _aura_id; }

  power_e type() const
  { return static_cast< power_e >( _power_e ); }

  double cost_divisor( bool percentage ) const
  {
    switch ( type() )
    {
      case POWER_MANA:
      case POWER_SOUL_SHARDS:
        return 100.0;
      case POWER_RAGE:
      case POWER_RUNIC_POWER:
      case POWER_BURNING_EMBER:
        return 10.0;
      case POWER_DEMONIC_FURY:
        return percentage ? 0.1 : 1.0;  // X% of 1000 ("base" demonic fury) is X divided by 0.1
      default:
        return 1.0;
    }
  }

  double cost() const
  {
    double cost = _cost > 0 ? _cost : _cost_2;

    return cost / cost_divisor( ! ( _cost > 0 ) );
  }

  double cost_per_second() const
  {
    double cost = _cost_per_second > 0 ? _cost_per_second : _cost_per_second_2;

    return cost / cost_divisor( ! ( _cost_per_second > 0 ) );
  }

  static spellpower_data_t* nil();
  static spellpower_data_t* list( bool ptr = false );
  static void               link( bool ptr = false );
};

class spellpower_data_nil_t : public spellpower_data_t
{
public:
  spellpower_data_nil_t() :
    spellpower_data_t()
  {}
  static spellpower_data_nil_t singleton;
};

inline spellpower_data_t* spellpower_data_t::nil()
{ return &spellpower_data_nil_t::singleton; }

// ==========================================================================
// Spell Effect Data - SpellEffect.dbc
// ==========================================================================

struct spelleffect_data_t
{
public:
  unsigned         _id;              // Effect id
  unsigned         _flags;           // Unused for now, 0x00 for all
  unsigned         _spell_id;        // Spell this effect belongs to
  unsigned         _index;           // Effect index for the spell
  effect_type_t    _type;            // Effect type
  effect_subtype_t _subtype;         // Effect sub-type
  // SpellScaling.dbc
  double           _m_avg;           // Effect average spell scaling multiplier
  double           _m_delta;         // Effect delta spell scaling multiplier
  double           _m_unk;           // Unused effect scaling multiplier
  //
  double           _sp_coeff;           // Effect coefficient
  double           _ap_coeff;        // Effect attack power coefficient
  double           _amplitude;       // Effect amplitude (e.g., tick time)
  // SpellRadius.dbc
  double           _radius;          // Minimum spell radius
  double           _radius_max;      // Maximum spell radius
  //
  int              _base_value;      // Effect value
  int              _misc_value;      // Effect miscellaneous value
  int              _misc_value_2;    // Effect miscellaneous value 2
  unsigned         _class_flags[NUM_CLASS_FAMILY_FLAGS]; // Class family flags
  unsigned         _trigger_spell_id;// Effect triggers this spell id
  double           _m_chain;         // Effect chain multiplier
  double           _pp_combo_points; // Effect points per combo points
  double           _real_ppl;        // Effect real points per level
  int              _die_sides;       // Effect damage range

  // Pointers for runtime linking
  spell_data_t*    _spell;
  spell_data_t*    _trigger_spell;

  bool ok() const
  { return _id != 0; }

  unsigned id() const
  { return _id; }

  unsigned index() const
  { return _index; }

  unsigned spell_id() const
  { return _spell_id; }

  unsigned spell_effect_num() const
  { return _index; }

  effect_type_t type() const
  { return _type; }

  effect_subtype_t subtype() const
  { return _subtype; }

  int base_value() const
  { return _base_value; }

  double percent() const
  { return _base_value * ( 1 / 100.0 ); }

  timespan_t time_value() const
  { return timespan_t::from_millis( _base_value ); }

  resource_e resource_gain_type() const
  { return util::translate_power_type( static_cast< power_e >( misc_value1() ) ); }

  double resource( resource_e resource_type ) const
  {
    switch ( resource_type )
    {
      case RESOURCE_RUNIC_POWER:
      case RESOURCE_RAGE:
        return base_value() * ( 1 / 10.0 );
      case RESOURCE_MANA:
        return base_value() * ( 1 / 100.0 );
      default:
        return base_value();
    }
  }

  double mastery_value() const
  { return _sp_coeff * ( 1 / 100.0 ); }

  int misc_value1() const
  { return _misc_value; }

  int misc_value2() const
  { return _misc_value_2; }

  unsigned trigger_spell_id() const
  { return _trigger_spell_id; }

  double chain_multiplier() const
  { return _m_chain; }

  double m_average() const
  { return _m_avg; }

  double m_delta() const
  { return _m_delta; }

  double m_unk() const
  { return _m_unk; }

  double sp_coeff() const
  { return _sp_coeff; }

  double ap_coeff() const
  { return _ap_coeff; }

  timespan_t period() const
  { return timespan_t::from_millis( _amplitude ); }

  double radius() const
  { return _radius; }

  double radius_max() const
  { return _radius_max; }

  double pp_combo_points() const
  { return _pp_combo_points; }

  double real_ppl() const
  { return _real_ppl; }

  int die_sides() const
  { return _die_sides; }

  bool class_flag( unsigned flag ) const
  {
    unsigned index = flag / 32;
    unsigned bit = flag % 32;

    assert( index < sizeof_array( _class_flags ) );
    return ( _class_flags[ index ] & ( 1u << bit ) ) != 0;
  }

  unsigned class_flags( unsigned idx ) const
  {
    assert( idx < NUM_CLASS_FAMILY_FLAGS );

    return _class_flags[ idx ];
  }

  double average( const player_t* p, unsigned level = 0 ) const;
  double delta( const player_t* p, unsigned level = 0 ) const;
  double bonus( const player_t* p, unsigned level = 0 ) const;
  double min( const player_t* p, unsigned level = 0 ) const;
  double max( const player_t* p, unsigned level = 0 ) const;

  double average( const item_t* item ) const;
  double delta( const item_t* item ) const;
  double min( const item_t* item ) const;
  double max( const item_t* item ) const;

  double average( const item_t& item ) const { return average( &item ); }
  double delta( const item_t& item ) const { return delta( &item ); }
  double min( const item_t& item ) const { return min( &item ); }
  double max( const item_t& item ) const { return max( &item ); }

  bool override_field( const std::string& field, double value );

  spell_data_t* spell() const;

  spell_data_t* trigger() const;

  static spelleffect_data_t* nil();
  static spelleffect_data_t* find( unsigned, bool ptr = false );
  static spelleffect_data_t* list( bool ptr = false );
  static void                link( bool ptr = false );
private:
  double scaled_average( double budget, unsigned level ) const;
  double scaled_delta( double budget ) const;
  double scaled_min( double avg, double delta ) const;
  double scaled_max( double avg, double delta ) const;
};

// ==========================================================================
// Spell Data
// ==========================================================================

#ifdef __OpenBSD__
#pragma pack(1)
#else
#pragma pack( push, 1 )
#endif
struct spell_data_t
{
private:
  friend void dbc::init();
  friend void dbc::de_init();
  static void link( bool ptr );
public:
  const char* _name;               // Spell name from Spell.dbc stringblock (enGB)
  unsigned    _id;                 // Spell ID in dbc
  unsigned    _flags;              // Unused for now, 0x00 for all
  double      _prj_speed;          // Projectile Speed
  unsigned    _school;             // Spell school mask
  unsigned    _class_mask;         // Class mask for spell
  unsigned    _race_mask;          // Racial mask for the spell
  int         _scaling_type;       // Array index for gtSpellScaling.dbc. -1 means the first non-class-specific sub array, and so on, 0 disabled
  unsigned    _max_scaling_level;  // Max scaling level(?), 0 == no restrictions, otherwise min( player_level, max_scaling_level )
  // SpellLevels.dbc
  unsigned    _spell_level;        // Spell learned on level. NOTE: Only accurate for "class abilities"
  unsigned    _max_level;          // Maximum level for scaling
  // SpellRange.dbc
  double      _min_range;          // Minimum range in yards
  double      _max_range;          // Maximum range in yards
  // SpellCooldown.dbc
  unsigned    _cooldown;           // Cooldown in milliseconds
  unsigned    _gcd;                // GCD in milliseconds
  // SpellCategories.dbc
  unsigned    _category;           // Spell category (for shared cooldowns, effects?)
  // SpellDuration.dbc
  double      _duration;           // Spell duration in milliseconds
  // SpellRuneCost.dbc
  unsigned    _rune_cost;          // Bitmask of rune cost 0x1, 0x2 = Blood | 0x4, 0x8 = Unholy | 0x10, 0x20 = Frost
  unsigned    _runic_power_gain;   // Amount of runic power gained ( / 10 )
  // SpellAuraOptions.dbc
  unsigned    _max_stack;          // Maximum stack size for spell
  unsigned    _proc_chance;        // Spell proc chance in percent
  unsigned    _proc_charges;       // Per proc charge amount
  unsigned    _proc_flags;         // Proc flags
  unsigned    _internal_cooldown;  // ICD
  double      _rppm;               // Base real procs per minute
  // SpellEquippedItems.dbc
  unsigned    _equipped_class;
  unsigned    _equipped_invtype_mask;
  unsigned    _equipped_subclass_mask;
  // SpellScaling.dbc
  int         _cast_min;           // Minimum casting time in milliseconds
  int         _cast_max;           // Maximum casting time in milliseconds
  int         _cast_div;           // A divisor used in the formula for casting time scaling (20 always?)
  double      _c_scaling;          // A scaling multiplier for level based scaling
  unsigned    _c_scaling_level;    // A scaling divisor for level based scaling
  // SpecializationSpells.dbc
  unsigned    _replace_spell_id;
  // Spell.dbc flags
  unsigned    _attributes[NUM_SPELL_FLAGS];// Spell.dbc "flags", record field 1..10, note that 12694 added a field here after flags_7
  unsigned    _class_flags[NUM_CLASS_FAMILY_FLAGS]; // SpellClassOptions.dbc flags
  unsigned    _class_flags_family; // SpellClassOptions.dbc spell family
  const char* _desc;               // Spell.dbc description stringblock
  const char* _tooltip;            // Spell.dbc tooltip stringblock
  // SpellDescriptionVariables.dbc
  const char* _desc_vars;          // Spell description variable stringblock, if present
  // SpellIcon.dbc
  const char* _icon;
  const char* _active_icon;
  const char* _rank_str;

  // Pointers for runtime linking
  std::vector<const spelleffect_data_t*>* _effects;
  std::vector<const spellpower_data_t*>*  _power;

  // Direct member access functions
  uint32_t category() const
  { return _category; }

  uint32_t class_mask() const
  { return _class_mask; }

  timespan_t cooldown() const
  { return timespan_t::from_millis( _cooldown ); }

  const char* desc() const
  { return ok() ? _desc : ""; }

  const char* desc_vars() const
  { return ok() ? _desc_vars : ""; }

  timespan_t duration() const
  { return timespan_t::from_millis( _duration ); }

  double extra_coeff() const
  { return 0; }

  timespan_t gcd() const
  { return timespan_t::from_millis( _gcd ); }

  unsigned id() const
  { return _id; }

  uint32_t initial_stacks() const
  { return _proc_charges; }

  uint32_t race_mask() const
  { return _race_mask; }

  uint32_t level() const
  { return _spell_level; }

  const char* name_cstr() const
  { return ok() ? _name : ""; }

  uint32_t max_level() const
  { return _max_level; }

  uint32_t max_stacks() const
  { return _max_stack; }

  double missile_speed() const
  { return _prj_speed; }

  double min_range() const
  { return _min_range; }

  double max_range() const
  { return _max_range; }

  double proc_chance() const
  { return _proc_chance * ( 1 / 100.0 ); }

  unsigned proc_flags() const
  { return _proc_flags; }

  timespan_t internal_cooldown() const
  { return timespan_t::from_millis( _internal_cooldown ); }

  double real_ppm() const
  { return _rppm; }

  const char* rank_str() const
  { return ok() ? _rank_str : ""; }

  unsigned replace_spell_id() const
  { return _replace_spell_id; }

  uint32_t rune_cost() const
  { return _rune_cost; }

  double runic_power_gain() const
  { return _runic_power_gain * ( 1 / 10.0 ); }

  double scaling_multiplier() const
  { return _c_scaling; }

  unsigned scaling_threshold() const
  { return _c_scaling_level; }

  uint32_t school_mask() const
  { return _school; }

  const char* tooltip() const
  { return ok() ? _tooltip : ""; }

  // Helper functions
  size_t effect_count() const
  { assert( _effects ); return _effects -> size(); }

  size_t power_count() const
  { return _power ? _power -> size() : 0; }

  bool found() const
  { return ( this != not_found() ); }

  school_e  get_school_type() const
  { return dbc::get_school_type( as<uint32_t>( _school ) ); }

  bool is_level( uint32_t level ) const
  { return level >= _spell_level; }

  bool in_range( double range ) const
  { return range >= _min_range && range <= _max_range; }

  bool is_race( race_e r ) const
  {
    unsigned mask = util::race_mask( r );
    return ( _race_mask & mask ) == mask;
  }

  bool ok() const
  { return _id != 0; }

  // Composite functions
  const spelleffect_data_t& effectN( size_t idx ) const
  {
    assert( _effects );
    assert( idx > 0 && "effect index must not be zero or less" );

    if ( this == spell_data_t::nil() || this == spell_data_t::not_found() )
      return *spelleffect_data_t::nil();

    assert( idx <= _effects -> size() && "effect index out of bound!" );

    return *( ( *_effects )[ idx - 1 ] );
  }

  const spellpower_data_t& powerN( size_t idx ) const
  {
    if ( _power )
    {
      assert( idx > 0 && _power && idx <= _power -> size() );

      return *_power -> at( idx - 1 );
    }

    return *spellpower_data_t::nil();
  }

  const spellpower_data_t& powerN( power_e pt ) const
  {
    assert( pt >= POWER_HEALTH && pt < POWER_MAX );
    if ( _power )
    {
      for ( size_t i = 0; i < _power -> size(); i++ )
      {
        if ( _power -> at( i ) -> _power_e == pt )
          return *_power -> at( i );
      }
    }

    return *spellpower_data_t::nil();
  }

  bool is_class( player_e c ) const
  {
    if ( ! _class_mask )
      return true;

    unsigned mask = util::class_id_mask( c );
    return ( _class_mask & mask ) == mask;
  }

  player_e scaling_class() const
  {
    switch ( _scaling_type )
    {
      case -4: return PLAYER_SPECIAL_SCALE4;
      case -3: return PLAYER_SPECIAL_SCALE3;
      case -2: return PLAYER_SPECIAL_SCALE2;
      case -1: return PLAYER_SPECIAL_SCALE;
      case 1:  return WARRIOR;
      case 2:  return PALADIN;
      case 3:  return HUNTER;
      case 4:  return ROGUE;
      case 5:  return PRIEST;
      case 6:  return DEATH_KNIGHT;
      case 7:  return SHAMAN;
      case 8:  return MAGE;
      case 9:  return WARLOCK;
      case 10: return MONK;
      case 11: return DRUID;
      default: break;
    }

    return PLAYER_NONE;
  }

  unsigned max_scaling_level() const
  { return _max_scaling_level; }

  timespan_t cast_time( uint32_t level ) const
  {
    if ( _cast_div < 0 )
    {
      return timespan_t::from_millis( std::max( 0, _cast_min ) );
    }

    if ( level >= as<uint32_t>( _cast_div ) )
      return timespan_t::from_millis( _cast_max );

    return timespan_t::from_millis( _cast_min + ( _cast_max - _cast_min ) * ( level - 1 ) / ( double )( _cast_div - 1 ) );
  }

  double cost( power_e pt ) const
  {
    if ( _power )
    {
      for ( size_t i = 0; i < _power -> size(); i++ )
      {
        if ( ( *_power )[ i ] -> _power_e == pt )
          return ( *_power )[ i ] -> cost();
      }
    }

    return 0.0;
  }

  uint32_t effect_id( uint32_t effect_num ) const
  {
    assert( _effects );
    assert( effect_num >= 1 && effect_num <= _effects -> size() );
    return ( *_effects )[ effect_num - 1 ] -> id();
  }

  bool flags( spell_attribute_e f ) const
  {
    unsigned bit = static_cast<unsigned>( f ) & 0x1Fu;
    unsigned index = ( static_cast<unsigned>( f ) >> 8 ) & 0xFFu;
    uint32_t mask = 1u << bit;

    assert( index < sizeof_array( _attributes ) );

    return ( _attributes[ index ] & mask ) != 0;
  }

  bool class_flag( unsigned flag ) const
  {
    unsigned index = flag / 32;
    unsigned bit = flag % 32;

    assert( index < sizeof_array( _class_flags ) );
    return ( _class_flags[ index ] & ( 1u << bit ) ) != 0;
  }

  unsigned attribute( unsigned idx ) const
  { assert( idx < sizeof_array( _attributes ) ); return _attributes[ idx ]; }

  unsigned class_flags( unsigned idx ) const
  { assert( idx < sizeof_array( _class_flags ) ); return _class_flags[ idx ]; }

  unsigned class_family() const
  { return _class_flags_family; }

  bool override_field( const std::string& field, double value );

  bool valid_item_enchantment( inventory_type inv_type ) const
  {
    if ( ! _equipped_invtype_mask )
      return true;

    unsigned invtype_mask = 1 << inv_type;
    if ( _equipped_invtype_mask & invtype_mask )
      return true;

    return false;
  }

  std::string to_str() const
  {
    std::ostringstream s;

    s << " (ok=" << ( ok() ? "true" : "false" ) << ")";
    s << " id=" << id();
    s << " name=" << name_cstr();
    s << " school=" << util::school_type_string( get_school_type() );
    return s.str();
  }

  bool affected_by( const spell_data_t* ) const;
  bool affected_by( const spelleffect_data_t* ) const;
  bool affected_by( const spelleffect_data_t& ) const;

  // static functions
  static spell_data_t* nil();
  static spell_data_t* not_found();
  static spell_data_t* find( const char* name, bool ptr = false );
  static spell_data_t* find( unsigned id, bool ptr = false );
  static spell_data_t* find( unsigned id, const char* confirmation, bool ptr = false );
  static spell_data_t* list( bool ptr = false );
  static void de_link( bool ptr = false );

} SC_PACKED_STRUCT;
#ifdef __OpenBSD__
#pragma pack()
#else
#pragma pack( pop )
#endif

// ==========================================================================
// Spell Effect Data Nil
// ==========================================================================

class spelleffect_data_nil_t : public spelleffect_data_t
{
public:
  spelleffect_data_nil_t() : spelleffect_data_t()
  { _spell = _trigger_spell = spell_data_t::not_found(); }

  static spelleffect_data_nil_t singleton;
};

inline spelleffect_data_t* spelleffect_data_t::nil()
{ return &spelleffect_data_nil_t::singleton; }

// ==========================================================================
// Spell Data Nil / Not found
// ==========================================================================

/* Empty spell_data container
 */
class spell_data_nil_t : public spell_data_t
{
public:
  spell_data_nil_t() : spell_data_t()
  {
    _effects = new std::vector< const spelleffect_data_t* >();
  }

  ~spell_data_nil_t()
  { delete _effects; }

  static spell_data_nil_t singleton;
};

inline spell_data_t* spell_data_t::nil()
{ return &spell_data_nil_t::singleton; }

/* Empty spell data container, which is used to return a "not found" state
 */
class spell_data_not_found_t : public spell_data_t
{
public:
  spell_data_not_found_t() : spell_data_t()
  {
    _effects = new std::vector< const spelleffect_data_t* >();
  }

  ~spell_data_not_found_t()
  { delete _effects; }

  static spell_data_not_found_t singleton;
};

inline spell_data_t* spell_data_t::not_found()
{ return &spell_data_not_found_t::singleton; }

inline spell_data_t* spelleffect_data_t::spell() const
{
  return _spell ? _spell : spell_data_t::not_found();
}

inline spell_data_t* spelleffect_data_t::trigger() const
{
  return _trigger_spell ? _trigger_spell : spell_data_t::not_found();
}

// ==========================================================================
// Talent Data
// ==========================================================================

struct talent_data_t
{
public:
  const char * _name;        // Talent name
  unsigned     _id;          // Talent id
  unsigned     _flags;       // Unused for now, 0x00 for all
  unsigned     _m_class;     // Class mask
  unsigned     _spec;        // Specialization
  unsigned     _col;         // Talent column
  unsigned     _row;         // Talent row
  unsigned     _spell_id;    // Talent spell
  unsigned     _replace_id;  // Talent replaces the following spell id

  // Pointers for runtime linking
  spell_data_t* spell1;

  // Direct member access functions
  unsigned id() const
  { return _id; }

  const char* name_cstr() const
  { return _name; }

  unsigned col() const
  { return _col; }

  unsigned row() const
  { return _row; }

  unsigned spell_id() const
  { return _spell_id; }

  unsigned replace_id() const
  { return _replace_id; }

  unsigned mask_class() const
  { return _m_class; }

  unsigned spec() const
  { return _spec; }

  specialization_e specialization() const
  { return static_cast<specialization_e>( _spec ); }

  // composite access functions

  spell_data_t* spell() const
  { return spell1 ? spell1 : spell_data_t::nil(); }

  bool is_class( player_e c ) const
  {
    unsigned mask = util::class_id_mask( c );

    if ( mask == 0 )
      return false;

    return ( ( _m_class & mask ) == mask );
  }

  // static functions
  static talent_data_t* nil();
  static talent_data_t* find( unsigned, bool ptr = false );
  static talent_data_t* find( unsigned, const char* confirmation, bool ptr = false );
  static talent_data_t* find( const char* name, specialization_e spec, bool ptr = false );
  static talent_data_t* find_tokenized( const char* name, specialization_e spec, bool ptr = false );
  static talent_data_t* find( player_e c, unsigned int row, unsigned int col, specialization_e spec, bool ptr = false );
  static talent_data_t* list( bool ptr = false );
  static void           link( bool ptr = false );
};

class talent_data_nil_t : public talent_data_t
{
public:
  talent_data_nil_t() :
    talent_data_t()
  { spell1 = spell_data_t::nil(); }

  static talent_data_nil_t singleton;
};

inline talent_data_t* talent_data_t::nil()
{ return &talent_data_nil_t::singleton; }


// If we don't have any ptr data, don't bother to check if ptr is true
#if SC_USE_PTR
inline bool maybe_ptr( bool ptr ) { return ptr; }
#else
inline bool maybe_ptr( bool ) { return false; }
#endif

// ==========================================================================
// General Database with state
// ==========================================================================

/* Database access with a state ( bool ptr )
 */
class dbc_t
{
public:
  bool ptr;

private:
  typedef std::unordered_map<uint32_t, uint32_t> id_map_t;
  id_map_t replaced_ids;
public:
  uint32_t replaced_id( uint32_t id_spell ) const;
  bool replace_id( uint32_t id_spell, uint32_t replaced_by_id );

  dbc_t( bool ptr = false ) :
    ptr( ptr ) { }

  int build_level() const
  { return dbc::build_level( ptr ); }

  const char* wow_version() const
  { return dbc::wow_version( ptr ); }

  const char* wow_ptr_status() const
  { return dbc::wow_ptr_status( ptr ); }

  const item_data_t* items() const
  { return dbc::items( ptr ); }

  std::size_t n_items() const
  { return dbc::n_items( ptr ); }

  const item_enchantment_data_t* item_enchantments() const
  { return dbc::item_enchantments( ptr ); }

  std::size_t n_item_enchantments() const
  { return dbc::n_item_enchantments( ptr ); }

  const gem_property_data_t* gem_properties() const
  { return dbc::gem_properties( ptr ); }

  bool add_token( unsigned int id_spell, const std::string& token_name ) const
  { return dbc::add_token( id_spell, token_name, ptr ); }

  // Game data table access
  double melee_crit_base( player_e t, unsigned level ) const;
  double melee_crit_base( pet_e t, unsigned level ) const;
  double spell_crit_base( player_e t, unsigned level ) const;
  double spell_crit_base( pet_e t, unsigned level ) const;
  double dodge_base( player_e t ) const;
  double dodge_base( pet_e t ) const;
  double regen_base( player_e t, unsigned level ) const;
  double regen_base( pet_e t, unsigned level ) const;
  double resource_base( player_e t, unsigned level ) const;
  double health_base( player_e t, unsigned level ) const;
  stat_data_t& attribute_base( player_e t, unsigned level ) const;
  stat_data_t& attribute_base( pet_e t, unsigned level ) const;
  stat_data_t& race_base( race_e r ) const;
  stat_data_t& race_base( pet_e t ) const;
  double parry_factor( player_e t ) const;
  double dodge_factor( player_e t ) const;
  double miss_factor( player_e t ) const;
  double block_factor( player_e t ) const;
  double vertical_stretch( player_e t ) const;
  double horizontal_shift( player_e t ) const;

  double spell_scaling( player_e t, unsigned level ) const;
  double melee_crit_scaling( player_e t, unsigned level ) const;
  double melee_crit_scaling( pet_e t, unsigned level ) const;
  double spell_crit_scaling( player_e t, unsigned level ) const;
  double spell_crit_scaling( pet_e t, unsigned level ) const;
  double regen_spirit( player_e t, unsigned level ) const;
  double regen_spirit( pet_e t, unsigned level ) const;
  double health_per_stamina( unsigned level ) const;
  double item_socket_cost( unsigned ilevel ) const;
  double real_ppm_coefficient( specialization_e, unsigned ) const;
  double armor_mitigation_constant( unsigned level ) const;

  double combat_rating( unsigned combat_rating_id, unsigned level ) const;
  double oct_combat_rating( unsigned combat_rating_id, player_e t ) const;

  int resolve_item_scaling( unsigned level ) const;
  double resolve_level_scaling( unsigned level ) const;

private:
  template <typename T>
  const T* find_by_id( unsigned id ) const
  {
    const T* item = T::find( id, ptr );
    assert( item && ( item -> id() == id || item -> id() == 0 ) );
    return item;
  }

public:
  // Always returns non-NULL.
  const spell_data_t*            spell( unsigned spell_id ) const
  { return find_by_id<spell_data_t>( spell_id ); }

  // Always returns non-NULL.
  const spelleffect_data_t*      effect( unsigned effect_id ) const
  { return find_by_id<spelleffect_data_t>( effect_id ); }

  // Always returns non-NULL.
  const talent_data_t*           talent( unsigned talent_id ) const
  { return find_by_id<talent_data_t>( talent_id ); }

  const item_data_t*             item( unsigned item_id ) const;
  const random_suffix_data_t&    random_suffix( unsigned suffix_id ) const;
  const item_enchantment_data_t& item_enchantment( unsigned enchant_id ) const;
  const gem_property_data_t&     gem_property( unsigned gem_id ) const;

  const random_prop_data_t&      random_property( unsigned ilevel ) const;
  int                            random_property_max_level() const;
  const item_scale_data_t&       item_damage_1h( unsigned ilevel ) const;
  const item_scale_data_t&       item_damage_2h( unsigned ilevel ) const;
  const item_scale_data_t&       item_damage_caster_1h( unsigned ilevel ) const;
  const item_scale_data_t&       item_damage_caster_2h( unsigned ilevel ) const;
  const item_scale_data_t&       item_damage_ranged( unsigned ilevel ) const;
  const item_scale_data_t&       item_damage_thrown( unsigned ilevel ) const;
  const item_scale_data_t&       item_damage_wand( unsigned ilevel ) const;

  const item_scale_data_t&       item_armor_quality( unsigned ilevel ) const;
  const item_scale_data_t&       item_armor_shield( unsigned ilevel ) const;
  const item_armor_type_data_t&  item_armor_total( unsigned ilevel ) const;
  const item_armor_type_data_t&  item_armor_inv_type( unsigned inv_type ) const;

  const item_upgrade_t&          item_upgrade( unsigned upgrade_id ) const;
  const item_upgrade_rule_t&     item_upgrade_rule( unsigned item_id, unsigned upgrade_level ) const;
  const rppm_modifier_t&         real_ppm_modifier( specialization_e spec, unsigned spell_id ) const;

  std::vector<const item_bonus_entry_t*> item_bonus( unsigned bonus_id ) const;

  // Derived data access
  unsigned num_tiers() const;

  unsigned class_ability( unsigned class_id, unsigned tree_id, unsigned n ) const;
  unsigned pet_ability( unsigned class_id, unsigned n ) const;
  unsigned class_ability_tree_size() const;
  unsigned class_ability_size() const;
  unsigned class_max_size() const;

  unsigned race_ability( unsigned race_id, unsigned class_id, unsigned n ) const;
  unsigned race_ability_size() const;
  unsigned race_ability_tree_size() const;

  unsigned specialization_ability( unsigned class_id, unsigned tree_id, unsigned n ) const;
  unsigned specialization_ability_size() const;
  unsigned specialization_max_per_class() const;
  unsigned specialization_max_class() const;
  bool     ability_specialization( uint32_t spell_id, std::vector<specialization_e>& spec_list ) const;

  unsigned perk_ability_size() const;
  unsigned perk_ability( unsigned class_id, unsigned tree_id, unsigned n ) const;

  unsigned mastery_ability( unsigned class_id, unsigned tree_id, unsigned n ) const;
  unsigned mastery_ability_size() const;
  int      mastery_ability_tree( player_e c, uint32_t spell_id ) const;

  unsigned glyph_spell( unsigned class_id, unsigned glyph_e, unsigned n ) const;
  unsigned glyph_spell_size() const;

  unsigned set_bonus_spell( unsigned class_id, unsigned tier, unsigned n ) const;
  unsigned set_bonus_spell_size() const;

  // Helper methods
  double   weapon_dps( unsigned item_id, unsigned ilevel = 0 ) const;
  double   weapon_dps( const item_data_t*, unsigned ilevel = 0 ) const;

  double   effect_average( unsigned effect_id, unsigned level ) const;
  double   effect_delta( unsigned effect_id, unsigned level ) const;

  double   effect_min( unsigned effect_id, unsigned level ) const;
  double   effect_max( unsigned effect_id, unsigned level ) const;
  double   effect_bonus( unsigned effect_id, unsigned level ) const;

  unsigned talent_ability_id( player_e c, specialization_e spec_id, const char* spell_name, bool name_tokenized = false ) const;
  unsigned class_ability_id( player_e c, specialization_e spec_id, const char* spell_name ) const;
  unsigned pet_ability_id( player_e c, const char* spell_name ) const;
  unsigned race_ability_id( player_e c, race_e r, const char* spell_name ) const;
  unsigned specialization_ability_id( specialization_e spec_id, const char* spell_name ) const;
  unsigned perk_ability_id( specialization_e spec_id, const char* spell_name ) const;
  unsigned perk_ability_id( specialization_e spec_id, size_t perk_index ) const;
  unsigned mastery_ability_id( specialization_e spec, const char* spell_name ) const;
  unsigned mastery_ability_id( specialization_e spec, uint32_t idx ) const;
  specialization_e mastery_specialization( const player_e c, uint32_t spell_id ) const;

  unsigned glyph_spell_id( player_e c, const char* spell_name ) const;
  unsigned glyph_spell_id( unsigned property_id ) const;
  unsigned set_bonus_spell_id( player_e c, const char* spell_name, int tier = -1 ) const;

  specialization_e class_ability_specialization( const player_e c, uint32_t spell_id ) const;

  bool     is_class_ability( uint32_t spell_id ) const;
  bool     is_race_ability( uint32_t spell_id ) const;
  bool     is_specialization_ability( uint32_t spell_id ) const;
  bool     is_mastery_ability( uint32_t spell_id ) const;
  bool     is_glyph_spell( uint32_t spell_id ) const;
  bool     is_set_bonus_spell( uint32_t spell_id ) const;

  specialization_e spec_by_spell( uint32_t spell_id ) const;

  bool spec_idx( specialization_e spec_id, uint32_t& class_idx, uint32_t& spec_index ) const;
  specialization_e spec_by_idx( const player_e c, unsigned idx ) const;
  double rppm_coefficient( specialization_e spec, unsigned spell_id ) const;

  unsigned item_upgrade_ilevel( unsigned item_id, unsigned upgrade_level ) const;

  std::vector< const spell_data_t* > effect_affects_spells( unsigned, const spelleffect_data_t* ) const;
  std::vector< const spelleffect_data_t* > effects_affecting_spell( const spell_data_t* ) const;
};

#endif // SC_DBC_HPP
