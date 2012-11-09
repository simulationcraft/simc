// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SIMULATIONCRAFT_H
static_assert( 0 , "dbc.hpp included into a file where SIMULATIONCRAFT_H is not defined!" );
#endif

#ifndef SC_DBC_HPP
#define SC_DBC_HPP

#include "data_definitions.hh"
#include "data_enums.hh"

// FIXME! This is a generated header.  Be nice if it was not seperate.
#include "specialization.hpp"

#ifndef NUM_SPELL_FLAGS
#define NUM_SPELL_FLAGS (12)
#endif

// Spell ID class

enum s_e
{
  T_SPELL = 0,
  T_TALENT,
  T_MASTERY,
  T_GLYPH,
  T_CLASS,
  T_RACE,
  T_SPEC,
  T_ITEM
};

// DBC related classes ======================================================

class dbc_t;

// SpellPower.dbc
struct spellpower_data_t
{
  unsigned _id;
  unsigned _spell_id;
  unsigned _aura_id;            // Spell id for the aura during which this power type is active
  int      _power_e;
  int      _cost;
  double   _cost_2;
  int      _cost_per_second;    // Unsure
  double   _cost_per_second_2;

  resource_e resource() const;
  unsigned id() const { return _id; }
  unsigned spell_id() const { return _spell_id; }
  unsigned aura_id() const { return _aura_id; }
  power_e type() const { return static_cast< power_e >( _power_e ); }
  double cost_divisor( bool percentage ) const;
  double cost() const;
  double cost_per_second() const;

  static spellpower_data_t* nil();
  static spellpower_data_t* list( bool ptr = false );
  static void               link( bool ptr = false );
};

class spellpower_data_nil_t : public spellpower_data_t
{
public:
  spellpower_data_nil_t();
  static spellpower_data_nil_t singleton;
};

inline spellpower_data_t* spellpower_data_t::nil()
{ return &spellpower_data_nil_t::singleton; }

// SpellEffect.dbc
struct spelleffect_data_t
{
private:
  static const unsigned int FLAG_USED = 1;
  static const unsigned int FLAG_ENABLED = 2;

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
  double           _coeff;           // Effect coefficient
  double           _amplitude;       // Effect amplitude (e.g., tick time)
  // SpellRadius.dbc
  double           _radius;          // Minimum spell radius
  double           _radius_max;      // Maximum spell radius
  //
  int              _base_value;      // Effect value
  int              _misc_value;      // Effect miscellaneous value
  int              _misc_value_2;    // Effect miscellaneous value 2
  int              _trigger_spell_id;// Effect triggers this spell id
  double           _m_chain;         // Effect chain multiplier
  double           _pp_combo_points; // Effect points per combo points
  double           _real_ppl;        // Effect real points per level
  int              _die_sides;       // Effect damage range

  // Pointers for runtime linking
  spell_data_t*    _spell;
  spell_data_t*    _trigger_spell;

  bool                       ok() const { return _id != 0; }
  unsigned                   id() const { return _id; }
  unsigned                   index() const { return _index; }
  unsigned                   spell_id() const { return _spell_id; }
  unsigned                   spell_effect_num() const { return _index; }

  effect_type_t              type() const { return _type; }
  effect_subtype_t           subtype() const { return _subtype; }

  int                        base_value() const { return _base_value; }
  double                     percent() const { return _base_value * ( 1 / 100.0 ); }
  timespan_t                 time_value() const { return timespan_t::from_millis( _base_value ); }
  resource_e                 resource_gain_type() const;
  double                     resource( resource_e resource_type ) const;
  double                     mastery_value() const { return _coeff * ( 1 / 100.0 ); }

  int                        misc_value1() const { return _misc_value; }
  int                        misc_value2() const { return _misc_value_2; }

  unsigned                   trigger_spell_id() const { return _trigger_spell_id >= 0 ? _trigger_spell_id : 0; }
  double                     chain_multiplier() const { return _m_chain; }

  double                     m_average() const { return _m_avg; }
  double                     m_delta() const { return _m_delta; }
  double                     m_unk() const { return _m_unk; }

  double                     coeff() const { return _coeff; }

  timespan_t                 period() const { return timespan_t::from_millis( _amplitude ); }

  double                     radius() const { return _radius; }
  double                     radius_max() const { return _radius_max; }

  double                     pp_combo_points() const { return _pp_combo_points; }

  double                     real_ppl() const { return _real_ppl; }

  int                        die_sides() const { return _die_sides; }

  double                     average( const player_t* p, unsigned level=0 ) const;
  double                     delta( const player_t* p, unsigned level=0 ) const;
  double                     bonus( const player_t* p, unsigned level=0 ) const;
  double                     min( const player_t* p, unsigned level=0 ) const;
  double                     max( const player_t* p, unsigned level=0 ) const;

  bool                       override( const std::string& field, double value );

  spell_data_t*              spell()   const;
  spell_data_t*              trigger() const;

  static spelleffect_data_t* nil();
  static spelleffect_data_t* find( unsigned, bool ptr = false );
  static spelleffect_data_t* list( bool ptr = false );
  static void                link( bool ptr = false );
};

class spelleffect_data_nil_t : public spelleffect_data_t
{
public:
  spelleffect_data_nil_t();
  static spelleffect_data_nil_t singleton;
};

inline spelleffect_data_t* spelleffect_data_t::nil()
{ return &spelleffect_data_nil_t::singleton; }

#ifdef __OpenBSD__
#pragma pack(1)
#else
#pragma pack( push, 1 )
#endif
struct spell_data_t
{
private:
  static const unsigned FLAG_USED = 1;
  static const unsigned FLAG_DISABLED = 2;

  friend class dbc_t;
  static void link( bool ptr );

public:
  const char* _name;               // Spell name from Spell.dbc stringblock (enGB)
  unsigned    _id;                 // Spell ID in dbc
  unsigned    _flags;              // Unused for now, 0x00 for all
  double      _prj_speed;          // Projectile Speed
  unsigned    _school;             // Spell school mask
  unsigned    _class_mask;         // Class mask for spell
  unsigned    _race_mask;          // Racial mask for the spell
  int         _scaling_type;       // Array index for gtSpellScaling.dbc. -1 means the last sub-array, 0 disabled
  double      _extra_coeff;        // An "extra" coefficient (used for some spells to indicate AP based coefficient)
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
  const char* _desc;               // Spell.dbc description stringblock
  const char* _tooltip;            // Spell.dbc tooltip stringblock
  // SpellDescriptionVariables.dbc
  const char* _desc_vars;          // Spell description variable stringblock, if present
  // SpellIcon.dbc
  const char* _icon;
  const char* _rank_str;

  // Pointers for runtime linking
  std::vector<const spelleffect_data_t*>* _effects;
  std::vector<const spellpower_data_t*>*  _power;

  const spelleffect_data_t& effectN( unsigned idx ) const
  {
    assert( idx > 0 && ( this == spell_data_t::nil() || this == spell_data_t::not_found() || idx <= _effects -> size() ) );

    if ( idx > _effects -> size() )
      return *spelleffect_data_t::nil();
    else
      return *_effects -> at( idx - 1 );
  }

  const spellpower_data_t& powerN( power_e pt ) const
  {
    assert( pt >= POWER_HEALTH && pt < POWER_MAX );
    if ( _power == 0 )
      return *spellpower_data_t::nil();

    for ( size_t i = 0; i < _power -> size(); i++ )
    {
      if ( _power -> at( i ) -> _power_e == pt )
        return *_power -> at( i );
    }

    return *spellpower_data_t::nil();
  }

  bool                 ok() const { return _id != 0; }
  bool                 found() const { return this != not_found(); }

  unsigned             id() const { return _id; }
  uint32_t             school_mask() const { return _school; }
  static uint32_t      get_school_mask( school_e s );
  school_e        get_school_type() const;
  static bool          is_school( school_e s, school_e s2 );

  bool                 is_class( player_e c ) const;
  uint32_t             class_mask() const { return _class_mask; }

  bool                 is_race( race_e r ) const;
  uint32_t             race_mask() const { return _race_mask; }

  bool                 is_level( uint32_t level ) const { return level >= _spell_level; }
  uint32_t             level() const { return _spell_level; }
  uint32_t             max_level() const { return _max_level; }

  player_e        scaling_class() const;

  double               missile_speed() const { return _prj_speed; }
  double               min_range() const { return _min_range; }
  double               max_range() const { return _max_range; }
  bool                 in_range( double range ) const { return range >= _min_range && range <= _max_range; }

  timespan_t           cooldown() const { return timespan_t::from_millis( _cooldown ); }
  timespan_t           duration() const { return timespan_t::from_millis( _duration ); }
  timespan_t           gcd() const { return timespan_t::from_millis( _gcd ); }
  timespan_t           cast_time( uint32_t level ) const;
  double               cost( power_e ) const;

  uint32_t             category() const { return _category; }

  uint32_t             rune_cost() const { return _rune_cost; }
  double               runic_power_gain() const { return _runic_power_gain * ( 1 / 10.0 ); }

  uint32_t             max_stacks() const { return _max_stack; }
  uint32_t             initial_stacks() const { return _proc_charges; }

  double               proc_chance() const { return _proc_chance * ( 1 / 100.0 ); }

  uint32_t             effect_id( uint32_t effect_num ) const
  {
    assert( effect_num >= 1 && effect_num <= _effects -> size() );
    return _effects -> at( effect_num - 1 ) -> id();
  }

  bool                 flags( spell_attribute_e f ) const;

  const char*          name_cstr() const { return ok() ? _name : ""; }
  const char*          desc() const { return ok() ? _desc : ""; }
  const char*          tooltip() const { return ok() ? _tooltip : ""; }
  const char*          rank_str() const { return ok() ? _rank_str : ""; }

  double               scaling_multiplier() const { return _c_scaling; }
  unsigned             scaling_threshold() const { return _c_scaling_level; }
  double               extra_coeff() const { return _extra_coeff; }
  unsigned             replace_spell_id() const { return _replace_spell_id; }

  bool                 override( const std::string& field, double value );
  std::string          to_str() const;

  static spell_data_t* nil();
  static spell_data_t* not_found();
  static spell_data_t* find( const char* name, bool ptr = false );
  static spell_data_t* find( unsigned id, bool ptr = false );
  static spell_data_t* find( unsigned id, const char* confirmation, bool ptr = false );
  static spell_data_t* list( bool ptr = false );
} SC_PACKED_STRUCT;
#ifdef __OpenBSD__
#pragma pack()
#else
#pragma pack( pop )
#endif

class spell_data_nil_t : public spell_data_t
{
public:
  spell_data_nil_t();
  ~spell_data_nil_t();
  static spell_data_nil_t singleton;
};

inline spell_data_t* spell_data_t::nil()
{ return &spell_data_nil_t::singleton; }

class spell_data_not_found_t : public spell_data_t
{
public:
  spell_data_not_found_t();
  ~spell_data_not_found_t();
  static spell_data_not_found_t singleton;
};

inline spell_data_t* spell_data_t::not_found()
{ return &spell_data_not_found_t::singleton; }



struct talent_data_t
{
public:
  const char * _name;        // Talent name
  unsigned     _id;          // Talent id
  unsigned     _flags;       // Unused for now, 0x00 for all
  unsigned     _m_class;     // Class mask
  unsigned     _m_pet;       // Pet mask
  unsigned     _is_pet;      // Is Pet?
  unsigned     _col;         // Talent column
  unsigned     _row;         // Talent row
  unsigned     _spell_id;    // Talent spell
  unsigned     _replace_id;  // Talent replaces the following spell id

  // Pointers for runtime linking
  spell_data_t* spell1;

  inline unsigned       id() const { return _id; }
  const char*           name_cstr() const { return _name; }
  bool                  is_class( player_e c ) const;
  bool                  is_pet( pet_e p ) const;
  unsigned              col() const { return _col; }
  unsigned              row() const { return _row; }
  unsigned              spell_id() const { return _spell_id; }
  unsigned              replace_id() const { return _replace_id; }
  unsigned              mask_class() const { return _m_class; }
  unsigned              mask_pet() const { return _m_pet; }

  spell_data_t* spell()
  {
    return spell1 ? spell1 : spell_data_t::nil();
  }

  static talent_data_t* nil();
  static talent_data_t* find( unsigned, bool ptr = false );
  static talent_data_t* find( unsigned, const char* confirmation, bool ptr = false );
  static talent_data_t* find( const char* name, bool ptr = false );
  static talent_data_t* find( player_e c, unsigned int row, unsigned int col, bool ptr = false );
  static talent_data_t* list( bool ptr = false );
  static void           link( bool ptr = false );
};

class talent_data_nil_t : public talent_data_t
{
public:
  talent_data_nil_t();
  static talent_data_nil_t singleton;
};

inline talent_data_t* talent_data_t::nil()
{ return &talent_data_nil_t::singleton; }

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

  dbc_t( bool ptr=false ) : ptr( ptr ) { }

  // Static Initialization
  static void apply_hotfixes();
  static void init();
  static void de_init() {}

  static int build_level( bool ptr = false );
  static const char* wow_version( bool ptr = false );

  static const item_data_t* items( bool ptr = false );
  static std::size_t        n_items( bool ptr = false );

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

  double spell_scaling( player_e t, unsigned level ) const;
  double melee_crit_scaling( player_e t, unsigned level ) const;
  double melee_crit_scaling( pet_e t, unsigned level ) const;
  double spell_crit_scaling( player_e t, unsigned level ) const;
  double spell_crit_scaling( pet_e t, unsigned level ) const;
  double dodge_scaling( player_e t, unsigned level ) const;
  double dodge_scaling( pet_e t, unsigned level ) const;
  double regen_spirit( player_e t, unsigned level ) const;
  double regen_spirit( pet_e t, unsigned level ) const;
  double mp5_per_spirit( player_e t, unsigned level ) const;
  double mp5_per_spirit( pet_e t, unsigned level ) const;
  double health_per_stamina( unsigned level ) const;

  double combat_rating( unsigned combat_rating_id, unsigned level ) const;
  double oct_combat_rating( unsigned combat_rating_id, player_e t ) const;

  const spell_data_t*            spell( unsigned spell_id ) const { return spell_data_t::find( spell_id, ptr ); }
  const spelleffect_data_t*      effect( unsigned effect_id ) const { return spelleffect_data_t::find( effect_id, ptr ); }
  const talent_data_t*           talent( unsigned talent_id ) const { return talent_data_t::find( talent_id, ptr ); }
  const item_data_t*             item( unsigned item_id ) const;

  const random_suffix_data_t&    random_suffix( unsigned suffix_id ) const;
  const item_enchantment_data_t& item_enchantment( unsigned enchant_id ) const;
  const gem_property_data_t&     gem_property( unsigned gem_id ) const;

  const random_prop_data_t&      random_property( unsigned ilevel ) const;
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

  // Derived data access
  unsigned num_tiers() const;
  unsigned first_tier() const;

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

  unsigned mastery_ability( unsigned class_id, unsigned tree_id, unsigned n ) const;
  unsigned mastery_ability_size() const;
  int      mastery_ability_tree( player_e c, uint32_t spell_id ) const;

  unsigned glyph_spell( unsigned class_id, unsigned glyph_e, unsigned n ) const;
  unsigned glyph_spell_size() const;

  unsigned set_bonus_spell( unsigned class_id, unsigned tier, unsigned n ) const;
  unsigned set_bonus_spell_size() const;

  // Helper methods
  double   weapon_dps( unsigned item_id ) const;

  double   effect_average( unsigned effect_id, unsigned level ) const;
  double   effect_delta( unsigned effect_id, unsigned level ) const;

  double   effect_min( unsigned effect_id, unsigned level ) const;
  double   effect_max( unsigned effect_id, unsigned level ) const;
  double   effect_bonus( unsigned effect_id, unsigned level ) const;

  unsigned talent_ability_id( player_e c, const char* spell_name ) const;
  unsigned class_ability_id( player_e c, specialization_e spec_id, const char* spell_name ) const;
  unsigned pet_ability_id( player_e c, const char* spell_name ) const;
  unsigned race_ability_id( player_e c, race_e r, const char* spell_name ) const;
  unsigned specialization_ability_id( specialization_e spec_id, const char* spell_name ) const;
  unsigned mastery_ability_id( specialization_e spec, const char* spell_name ) const;
  unsigned mastery_ability_id( specialization_e spec, uint32_t idx ) const;
  specialization_e mastery_specialization( const player_e c, uint32_t spell_id ) const;

  unsigned glyph_spell_id( player_e c, const char* spell_name ) const;
  unsigned set_bonus_spell_id( player_e c, const char* spell_name, int tier = -1 ) const;

  int      class_ability_tree( player_e c, uint32_t spell_id ) const;
  specialization_e class_ability_specialization( const player_e c, uint32_t spell_id ) const;

  bool     is_class_ability( uint32_t spell_id ) const;
  bool     is_race_ability( uint32_t spell_id ) const;
  bool     is_specialization_ability( uint32_t spell_id ) const;
  bool     is_mastery_ability( uint32_t spell_id ) const;
  bool     is_glyph_spell( uint32_t spell_id ) const;
  bool     is_set_bonus_spell( uint32_t spell_id ) const;

  bool spec_idx( specialization_e spec_id, uint32_t& class_idx, uint32_t& spec_index ) const;
  specialization_e spec_by_idx( const player_e c, uint32_t& idx ) const;

  // Static helper methods
  static double fmt_value( double v, effect_type_t type, effect_subtype_t sub_type );


  static const std::string& get_token( unsigned int id_spell );
  static bool add_token( unsigned int id_spell, const std::string& token_name, bool ptr = false );
  static unsigned int get_token_id( const std::string& token );
};



#endif // SC_DBC_HPP
