// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_DBC_SPELL_DATA_HPP
#define SC_DBC_SPELL_DATA_HPP

#include "config.hpp"

#include "dbc/client_data.hpp"
#include "dbc/client_hotfix_entry.hpp"
#include "dbc/data_enums.hh"
#include "sc_enums.hpp"
#include "util/timespan.hpp"
#include "util/span.hpp"
#include "util/string_view.hpp"

#include <string>
#include <vector>

class dbc_t;
struct item_t;
struct player_t;
struct spell_data_t;
struct spelleffect_data_t;
struct spelllabel_data_t;
struct spellpower_data_t;

constexpr unsigned NUM_SPELL_FLAGS = 15;
constexpr unsigned NUM_CLASS_FAMILY_FLAGS = 4;

// ==========================================================================
// Spell Label Data - SpellLabel.db2
// ==========================================================================

struct spelllabel_data_t
{
  unsigned _id;
  unsigned _id_spell;
  short    _label;

  unsigned id() const
  { return _id; }

  unsigned id_spell() const
  { return _id_spell; }

  short label() const
  { return _label; }

  static util::span<const spelllabel_data_t> data( bool ptr );
};

// ==========================================================================
// Spell Power Data - SpellPower.dbc
// ==========================================================================

struct spellpower_data_t
{
  unsigned _id;
  unsigned _spell_id;
  unsigned _aura_id; // Spell id for the aura during which this power type is active
  int      _power_type;
  int      _cost;
  int      _cost_max;
  int      _cost_per_tick;
  double   _pct_cost;
  double   _pct_cost_max;
  double   _pct_cost_per_tick;

  resource_e resource() const;

  unsigned id() const
  { return _id; }

  unsigned spell_id() const
  { return _spell_id; }

  unsigned aura_id() const
  { return _aura_id; }

  power_e type() const
  { return static_cast< power_e >( _power_type ); }

  int raw_type() const
  { return _power_type; }

  double cost_divisor( bool percentage ) const
  {
    switch ( type() )
    {
      case POWER_MANA:
      case POWER_INSANITY:
        return 100.0;
      case POWER_RAGE:
      case POWER_RUNIC_POWER:
      case POWER_BURNING_EMBER:
      case POWER_ASTRAL_POWER:
      case POWER_PAIN:
      case POWER_SOUL_SHARDS:
        return 10.0;
      case POWER_DEMONIC_FURY:
        return percentage ? 0.1 : 1.0;  // X% of 1000 ("base" demonic fury) is X divided by 0.1
      default:
        return 1.0;
    }
  }

  double cost() const
  {
    double cost = _cost != 0 ? _cost : _pct_cost;

    return cost / cost_divisor( ! ( _cost != 0 ) );
  }

  double max_cost() const
  {
    double cost = _cost_max != 0 ? _cost_max : _pct_cost_max;

    return cost / cost_divisor( ! ( _cost_max != 0 ) );
  }

  double cost_per_tick() const
  {
    double cost = _cost_per_tick != 0 ? _cost_per_tick : _pct_cost_per_tick;

    return cost / cost_divisor( ! ( _cost_per_tick != 0 ) );
  }

  static const spellpower_data_t& nil()
  { return dbc::nil<spellpower_data_t>; }

  static const spellpower_data_t& find( unsigned id, bool ptr = false );
  static util::span<const spellpower_data_t> data( bool ptr = false );
  static util::span<const hotfix::client_hotfix_entry_t> hotfixes( const spellpower_data_t&, bool ptr );

  bool override_field( util::string_view field, double value );
  double get_field( util::string_view field ) const;
};

// ==========================================================================
// Spell Effect Data - SpellEffect.dbc
// ==========================================================================

struct spelleffect_data_t
{
  unsigned         _id;              // Effect id
  unsigned         _spell_id;        // Spell this effect belongs to
  unsigned         _index;           // Effect index for the spell
  unsigned         _type;            // Effect type
  unsigned         _subtype;         // Effect sub-type
  // SpellScaling.dbc
  double           _m_coeff;         // Effect average spell scaling multiplier
  double           _m_delta;         // Effect delta spell scaling multiplier
  double           _m_unk;           // Unused effect scaling multiplier
  //
  double           _sp_coeff;        // Effect coefficient
  double           _ap_coeff;        // Effect attack power coefficient
  double           _amplitude;       // Effect amplitude (e.g., tick time)
  // SpellRadius.dbc
  double           _radius;          // Minimum spell radius
  double           _radius_max;      // Maximum spell radius
  //
  double           _base_value;      // Effect value
  int              _misc_value;      // Effect miscellaneous value
  int              _misc_value_2;    // Effect miscellaneous value 2
  unsigned         _class_flags[NUM_CLASS_FAMILY_FLAGS]; // Class family flags
  unsigned         _trigger_spell_id;// Effect triggers this spell id
  double           _m_chain;         // Effect chain multiplier
  double           _pp_combo_points; // Effect points per combo points
  double           _real_ppl;        // Effect real points per level
  unsigned         _mechanic;        // Effect Mechanic
  int              _chain_target;    // Number of targets (for chained spells)
  unsigned         _targeting_1;     // Targeting related field 1
  unsigned         _targeting_2;     // Targeting related field 2
  double           _m_value;         // Misc multiplier used for some spells(?)
  double           _pvp_coeff;       // PvP Coefficient

  // Pointers for runtime linking
  const spell_data_t* _spell;
  const spell_data_t* _trigger_spell;

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
  { return static_cast<effect_type_t>( _type ); }

  unsigned raw_type() const
  { return _type; }

  effect_subtype_t subtype() const
  { return static_cast<effect_subtype_t>( _subtype ); }

  unsigned raw_subtype() const
  { return _subtype; }

  property_type_t property_type() const
  { return static_cast<property_type_t>( _misc_value ); }

  double base_value() const
  { return _base_value; }

  double percent() const
  { return _base_value * ( 1 / 100.0 ); }

  timespan_t time_value() const
  { return timespan_t::from_millis( _base_value ); }

  resource_e resource_gain_type() const;

  double resource( resource_e resource_type ) const
  {
    switch ( resource_type )
    {
      case RESOURCE_RUNIC_POWER:
      case RESOURCE_RAGE:
      case RESOURCE_ASTRAL_POWER:
      case RESOURCE_PAIN:
      case RESOURCE_SOUL_SHARD:
        return base_value() * ( 1 / 10.0 );
      case RESOURCE_INSANITY:
      case RESOURCE_MANA:
        return base_value() * ( 1 / 100.0 );
      default:
        return base_value();
    }
  }

  school_e school_type() const;

  bool has_common_school( school_e ) const;

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

  double m_coefficient() const
  { return _m_coeff; }

  double m_delta() const
  { return _m_delta; }

  double m_unk() const
  { return _m_unk; }

  double sp_coeff() const
  { return _sp_coeff; }

  double ap_coeff() const
  { return _ap_coeff; }

  double pvp_coeff() const
  { return _pvp_coeff; }

  timespan_t period() const
  { return timespan_t::from_millis( _amplitude ); }

  double radius() const
  { return _radius; }

  double radius_max() const
  { return std::max( _radius_max, radius() ); }

  double pp_combo_points() const
  { return _pp_combo_points; }

  double real_ppl() const
  { return _real_ppl; }

  // TODO: Enumize
  unsigned mechanic() const
  { return _mechanic; }

  int chain_target() const
  { return _chain_target; }

  unsigned target_1() const
  { return _targeting_1; }

  unsigned target_2() const
  { return _targeting_2; }

  double m_value() const
  { return _m_value; }

  bool class_flag( unsigned flag ) const
  {
    unsigned index = flag / 32;
    unsigned bit = flag % 32;

    assert( index < range::size( _class_flags ) );
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

  bool override_field( util::string_view field, double value );
  double get_field( util::string_view field ) const;

  const spell_data_t* spell() const
  { assert( _spell ); return _spell; }

  const spell_data_t* trigger() const
  { assert( _trigger_spell ); return _trigger_spell; }

  // Fetch value multiplier to be used based on the spell effect type/subtype
  // TODO: Still needs quite a few additions, test before using!
  double default_multiplier() const;
  // Fetch value dynamically based on the spell effect type/subtype
  double default_value() const
  { return base_value() * default_multiplier(); }

  static const spelleffect_data_t& nil();
  static const spelleffect_data_t* find( unsigned, bool ptr = false );
  static util::span<const spelleffect_data_t> data( bool ptr = false );
  static util::span<const hotfix::client_hotfix_entry_t> hotfixes( const spelleffect_data_t&, bool ptr );

  static void link( bool ptr = false );
private:
  static util::span<spelleffect_data_t> _data( bool ptr );

  double scaled_average( double budget, unsigned level ) const;
  double scaled_delta( double budget ) const;
  double scaled_min( double avg, double delta ) const;
  double scaled_max( double avg, double delta ) const;
};

// ==========================================================================
// Spell Data
// ==========================================================================

struct spell_data_t
{
  const char* _name;               // Spell name from Spell.dbc stringblock (enGB)
  unsigned    _id;                 // Spell ID in dbc
  unsigned    _school;             // Spell school mask
  double      _prj_speed;          // Projectile Speed
  uint64_t    _race_mask;          // Racial mask for the spell
  unsigned    _class_mask;         // Class mask for spell
  int         _scaling_type;       // Array index for gtSpellScaling.dbc. -1 means the first non-class-specific sub array, and so on, 0 disabled
  int         _max_scaling_level;  // Max scaling level(?), 0 == no restrictions, otherwise min( player_level, max_scaling_level )
  // SpellLevels.dbc
  unsigned    _spell_level;        // Spell learned on level. NOTE: Only accurate for "class abilities"
  unsigned    _max_level;          // Maximum level for scaling
  unsigned    _req_max_level;
  // SpellRange.dbc
  double      _min_range;          // Minimum range in yards
  double      _max_range;          // Maximum range in yards
  // SpellCooldown.dbc
  unsigned    _cooldown;           // Cooldown in milliseconds
  unsigned    _gcd;                // GCD in milliseconds
  unsigned    _category_cooldown;  // Category cooldown in milliseconds
  // SpellCategory.dbc
  unsigned    _charges;            // Number of charges
  unsigned    _charge_cooldown;    // Cooldown duration of charges
  // SpellCategories.dbc
  unsigned    _category;           // Spell category (for shared cooldowns, effects?)
  unsigned    _dmg_class;          // Classification for the spell
  int         _max_targets;        // Max number of targets
  // SpellDuration.dbc
  double      _duration;           // Spell duration in milliseconds
  // SpellAuraOptions.dbc
  unsigned    _max_stack;          // Maximum stack size for spell
  unsigned    _proc_chance;        // Spell proc chance in percent
  int         _proc_charges;       // Per proc charge amount
  unsigned    _proc_flags;         // Proc flags
  unsigned    _internal_cooldown;  // ICD
  double      _rppm;               // Base real procs per minute
  // SpellEquippedItems.dbc
  unsigned    _equipped_class;
  unsigned    _equipped_invtype_mask;
  unsigned    _equipped_subclass_mask;
  // SpellScaling.dbc
  int         _cast_time;          // Casting time in milliseconds
  // Spell.dbc flags
  unsigned    _attributes[NUM_SPELL_FLAGS]; // SpellMisc.dbc "flags"
  unsigned    _class_flags[NUM_CLASS_FAMILY_FLAGS]; // SpellClassOptions.dbc flags
  unsigned    _class_flags_family; // SpellClassOptions.dbc spell family
  // SpellShapeshift.db2
  unsigned    _stance_mask;        // Stance mask (used only for druid form restrictions?)
  // SpellMechanic.db2
  unsigned    _mechanic;
  // Azerite stuff
  unsigned    _power_id;           // Azerite power id
  unsigned    _essence_id;         // Azerite essence id

  // Pointers for runtime linking
  const spelleffect_data_t* _effects;
  const spellpower_data_t* _power;
  const spell_data_t* const* _driver; // The triggered spell's driver(s)
  const spelllabel_data_t* _labels; // Applied (known) labels to the spell

  uint8_t _effects_count;
  uint8_t _power_count;
  uint8_t _driver_count;
  uint8_t _labels_count;

  unsigned equipped_class() const
  { return _equipped_class; }

  unsigned equipped_invtype_mask() const
  { return _equipped_invtype_mask; }

  unsigned equipped_subclass_mask() const
  { return _equipped_subclass_mask; }

  // Direct member access functions
  unsigned category() const
  { return _category; }

  unsigned dmg_class() const
  { return _dmg_class; }

  unsigned class_mask() const
  { return _class_mask; }

  timespan_t cooldown() const
  { return timespan_t::from_millis( _cooldown ); }

  timespan_t category_cooldown() const
  { return timespan_t::from_millis( _category_cooldown ); }

  unsigned charges() const
  { return _charges; }

  timespan_t charge_cooldown() const
  { return timespan_t::from_millis( _charge_cooldown ); }

  timespan_t duration() const
  { return timespan_t::from_millis( _duration ); }

  double extra_coeff() const
  { return 0; }

  timespan_t gcd() const
  { return timespan_t::from_millis( _gcd ); }

  unsigned id() const
  { return _id; }

  int initial_stacks() const
  { return _proc_charges; }

  uint64_t race_mask() const
  { return _race_mask; }

  uint32_t level() const
  { return _spell_level; }

  const char* name_cstr() const
  { return ok() ? _name : ""; }

  uint32_t max_level() const
  { return _max_level; }

  unsigned req_max_level() const
  { return _req_max_level; }

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

  uint32_t school_mask() const
  { return _school; }

  unsigned stance_mask() const
  { return _stance_mask; }

  unsigned mechanic() const
  { return _mechanic; }

  int max_targets() const
  { return _max_targets; }

  unsigned power_id() const
  { return _power_id; }

  unsigned essence_id() const
  { return _essence_id; }

  // Helper functions
  size_t effect_count() const
  { return _effects_count; }

  size_t power_count() const
  { return _power_count; }

  size_t driver_count() const
  { return _driver_count; }

  size_t label_count() const
  { return _labels_count; }

  bool found() const
  { return ( this != not_found() ); }

  school_e  get_school_type() const;

  bool is_level( uint32_t level ) const
  { return level >= _spell_level; }

  bool in_range( double range ) const
  { return range >= _min_range && range <= _max_range; }

  bool is_race( race_e r ) const;

  bool ok() const
  { return _id != 0; }

  // Composite functions
  const spelleffect_data_t& effectN( size_t idx ) const
  {
    assert( idx > 0 && "effect index must not be zero or less" );

    if ( this == spell_data_t::nil() || this == spell_data_t::not_found() )
      return spelleffect_data_t::nil();

    assert( idx <= effect_count() && "effect index out of bound!" );

    return effects()[ idx - 1 ];
  }

  const spellpower_data_t& powerN( size_t idx ) const
  {
    const auto powers = this -> powers();
    if ( powers.size() )
    {
      assert( idx > 0 && idx <= powers.size() );

      return powers[ idx - 1 ];
    }

    return spellpower_data_t::nil();
  }

  short labelN( size_t idx ) const
  {
    const auto labels = this -> labels();
    if ( idx > 0 && idx <= labels.size() )
      return labels[ idx - 1 ].label();
    return 0;
  }

  const spellpower_data_t& powerN( power_e pt ) const
  {
    assert( pt >= POWER_HEALTH && pt < POWER_MAX );
    for ( const spellpower_data_t& pd : powers() )
    {
      if ( pd._power_type == pt )
        return pd;
    }

    return spellpower_data_t::nil();
  }

  util::span<const spelleffect_data_t> effects() const
  {
    assert( _effects != nullptr || _effects_count == 0 );
    return { _effects, _effects_count };
  }

  util::span<const spellpower_data_t> powers() const
  {
    assert( _power != nullptr || _power_count == 0 );
    return { _power, _power_count };
  }

  util::span<const spelllabel_data_t> labels() const
  {
    assert( _labels != nullptr || _labels_count == 0 );
    return { _labels, _labels_count };
  }

  util::span<const spell_data_t* const> drivers() const
  {
    assert( _driver != nullptr || _driver_count == 0 );
    return { _driver, _driver_count };
  }

  bool is_class( player_e c ) const;

  player_e scaling_class() const
  {
    switch ( _scaling_type )
    {
      case -8: return PLAYER_SPECIAL_SCALE8;
      case -7: return PLAYER_SPECIAL_SCALE7;
      case -6: return PLAYER_SPECIAL_SCALE6;
      case -5: return PLAYER_SPECIAL_SCALE5;
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
      case 12: return DEMON_HUNTER;
      default: break;
    }

    return PLAYER_NONE;
  }

  unsigned max_scaling_level() const
  { return _max_scaling_level; }

  timespan_t cast_time() const
  { return timespan_t::from_millis( _cast_time ); }

  double cost( power_e pt ) const
  {
    for ( const spellpower_data_t& pd : powers() )
    {
      if ( pd._power_type == pt )
        return pd.cost();
    }

    return 0.0;
  }

  uint32_t effect_id( uint32_t effect_num ) const
  {
    assert( effect_num >= 1 && effect_num <= effect_count() );
    return effects()[ effect_num - 1 ].id();
  }

  bool flags( spell_attribute attr ) const
  {
    unsigned bit = static_cast<unsigned>( attr ) % 32u;
    unsigned index = static_cast<unsigned>( attr ) / 32u;
    uint32_t mask = 1u << bit;

    assert( index < range::size( _attributes ) );

    return ( _attributes[ index ] & mask ) != 0;
  }

  bool class_flag( unsigned flag ) const
  {
    unsigned index = flag / 32;
    unsigned bit = flag % 32;

    assert( index < range::size( _class_flags ) );
    return ( _class_flags[ index ] & ( 1u << bit ) ) != 0;
  }

  unsigned attribute( unsigned idx ) const
  {
    assert( idx < range::size( _attributes ) );
    return _attributes[ idx ];
  }

  unsigned class_flags( unsigned idx ) const
  {
    assert( idx < range::size( _class_flags ) );
    return _class_flags[ idx ];
  }

  unsigned class_family() const
  { return _class_flags_family; }

  bool override_field( util::string_view field, double value );
  double get_field( util::string_view field ) const;

  bool valid_item_enchantment( inventory_type inv_type ) const
  {
    if ( ! _equipped_invtype_mask )
      return true;

    unsigned invtype_mask = 1 << inv_type;
    if ( _equipped_invtype_mask & invtype_mask )
      return true;

    return false;
  }

  std::string to_str() const;

  bool affected_by_all( const dbc_t& dbc, const spelleffect_data_t& effect ) const;
  bool affected_by_category( const dbc_t& dbc, const spelleffect_data_t& effect ) const;
  bool affected_by_category( const dbc_t& dbc, int category ) const;
  bool affected_by_label( const spelleffect_data_t& effect ) const;
  bool affected_by_label( int label ) const;
  bool affected_by( const spell_data_t* ) const;
  bool affected_by( const spelleffect_data_t* ) const;
  bool affected_by( const spelleffect_data_t& ) const;

  // static functions
  static const spell_data_t* nil();
  static const spell_data_t* not_found();
  static const spell_data_t* find( util::string_view name, bool ptr = false );
  static const spell_data_t* find( unsigned id, bool ptr = false );
  static const spell_data_t* find( unsigned id, util::string_view confirmation, bool ptr = false );
  static util::span<const spell_data_t> data( bool ptr = false );
  static util::span<const hotfix::client_hotfix_entry_t> hotfixes( const spell_data_t&, bool ptr );

  static void link( bool ptr );
private:
  static util::span<spell_data_t> _data( bool ptr );
};

// ==========================================================================
// Spell Data Nil / Not found
// ==========================================================================

// Empty spell_data container
struct spell_data_nil_t : public spell_data_t {
  constexpr spell_data_nil_t() : spell_data_t() {
    _name = "nil";
  };
};
static constexpr const spell_data_nil_t& spell_data_nil_v = meta::static_const_t<spell_data_nil_t>::value;
inline const spell_data_t* spell_data_t::nil() { return &spell_data_nil_v; }

// Empty spell data container, which is used to return a "not found" state
struct spell_data_not_found_t : public spell_data_t {
  constexpr spell_data_not_found_t() : spell_data_t() {
    _name = "not_found";
  };
};
static constexpr const spell_data_not_found_t& spell_data_not_found_v = meta::static_const_t<spell_data_not_found_t>::value;
inline const spell_data_t* spell_data_t::not_found() { return &spell_data_not_found_v; }

// ==========================================================================
// Spell Effect Data Nil
// ==========================================================================

struct spelleffect_data_nil_t : public spelleffect_data_t {
  constexpr spelleffect_data_nil_t() : spelleffect_data_t() {
    _spell = _trigger_spell = &spell_data_not_found_v;
  }
};
static constexpr const spelleffect_data_t& spelleffect_data_nil_v = meta::static_const_t<spelleffect_data_nil_t>::value;
inline const spelleffect_data_t& spelleffect_data_t::nil() { return spelleffect_data_nil_v; }

#endif // SC_DBC_SPELL_DATA_HPP
