// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_DBC_HPP
#define SC_DBC_HPP

#include "config.hpp"
#include "util/generic.hpp"
#include "util/timespan.hpp"

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "data_definitions.hh"
#include "data_enums.hh"
#include "specialization.hpp"
#include "sc_enums.hpp"
#include "util/util.hpp"
#include "util/allocator.hpp"

#include "dbc/azerite.hpp"
#include "dbc/rand_prop_points.hpp"
#include "dbc/spell_item_enchantment.hpp"
#include "dbc/gem_data.hpp"
#include "dbc/item_data.hpp"
#include "dbc/item_child.hpp"
#include "dbc/item_armor.hpp"
#include "dbc/item_bonus.hpp"
#include "dbc/item_naming.hpp"
#include "dbc/item_scaling.hpp"
#include "dbc/item_weapon.hpp"
#include "dbc/item_effect.hpp"
#include "dbc/racial_spells.hpp"
#include "dbc/real_ppm_data.hpp"
#include "dbc/talent_data.hpp"
#include "dbc/spell_data.hpp"
#include "dbc/spelltext_data.hpp"

// ==========================================================================
// Forward declaration
// ==========================================================================

class dbc_t;
struct player_t;
struct item_t;

struct stat_data_t
{
  double strength;
  double agility;
  double stamina;
  double intellect;
  double spirit;
};

// ==========================================================================
// General Database
// ==========================================================================

namespace dbc {

// Initialization
void init();
void init_item_data();

// Utility functions
uint32_t get_school_mask( school_e s );
school_e get_school_type( uint32_t school_id );
bool is_school( school_e s, school_e s2 );
bool has_common_school( school_e s1, school_e s2 );
unsigned specialization_max_per_class();
specialization_e spec_by_idx( const player_e c, unsigned idx );
int spec_idx( specialization_e spec );

// Data Access
const char* wow_version( bool ptr );
const char* wow_ptr_status( bool ptr );
specialization_e translate_spec_str   ( player_e ptype, util::string_view spec_str );
const char* specialization_string     ( specialization_e spec );
double fmt_value( double v, effect_type_t type, effect_subtype_t sub_type );
bool valid_gem_color( unsigned color );
double stat_data_to_attribute( const stat_data_t&, attribute_e);

std::string bonus_ids_str( const dbc_t& );

double item_level_squish( unsigned source_ilevel, bool ptr );

// Filtered data access
const dbc_item_data_t& find_consumable( item_subclass_consumable type, bool ptr, const std::function<bool(const dbc_item_data_t*)>& finder );
const dbc_item_data_t& find_gem( util::string_view gem, bool ptr, bool tokenized = true );

// Class / Spec specific passives for an actor
const spell_data_t* get_class_passive( const player_t&, specialization_e );
std::vector<const spell_data_t*> class_passives( const player_t* );

// Retuns a list of all effect subtypes affecting the spell through categories
util::span<const effect_subtype_t> effect_category_subtypes();

} // namespace dbc

struct custom_dbc_data_t
{
  // Creates a tree of cloned spells and effects for a given spell, starting
  // from the potential root spell. If there is no need to clone the tree,
  // return the custom spell instead.
  spell_data_t* clone_spell( const spell_data_t* spell );

  spell_data_t* get_mutable_spell( unsigned spell_id );
  const spell_data_t* find_spell( unsigned spell_id ) const;

  spelleffect_data_t* get_mutable_effect( unsigned effect_id );
  const spelleffect_data_t* find_effect( unsigned effect_id ) const;

  spellpower_data_t* get_mutable_power( unsigned power_id );
  const spellpower_data_t* find_power( unsigned power_id ) const;

  // Copies spells from another custom data
  // Effectively a copy-assignement operator
  void copy_from( const custom_dbc_data_t& other );

private:
  spell_data_t* create_clone( const spell_data_t* s );

  void add_spell( spell_data_t* spell );
  void add_effect( spelleffect_data_t* effect );
  void add_power( spellpower_data_t* power );

  std::vector<spell_data_t*> spells_;
  std::vector<spelleffect_data_t*> effects_;
  std::vector<spellpower_data_t*> powers_;

  util::bump_ptr_allocator_t<> allocator_;
  std::unordered_map<unsigned, util::span<const spell_data_t*>> spell_driver_map_;
};

namespace hotfix
{
  // Manual hotfixing system for simc

  enum hotfix_op_e
  {
    HOTFIX_NONE = 0,
    HOTFIX_SET,
    HOTFIX_ADD,
    HOTFIX_MUL,
    HOTFIX_DIV
  };

  enum hotfix_flags_e
  {
    HOTFIX_FLAG_LIVE  = 0x1,
    HOTFIX_FLAG_PTR   = 0x2,
    HOTFIX_FLAG_QUIET = 0x4,

    HOTFIX_FLAG_DEFAULT = HOTFIX_FLAG_LIVE | HOTFIX_FLAG_PTR
  };

  struct hotfix_entry_t
  {
    std::string group_;
    std::string tag_;
    std::string note_;
    unsigned    flags_;

    hotfix_entry_t() :
      group_(), tag_(), note_(), flags_( 0 )
    { }

    hotfix_entry_t( util::string_view g, util::string_view t, util::string_view n, unsigned f ) :
      group_( g ), tag_( t ), note_( n ), flags_( f )
    { }

    virtual ~hotfix_entry_t() { }

    virtual bool valid() const = 0;

    virtual void apply() { }
    virtual std::string to_str() const;
  };

  struct dbc_hotfix_entry_t : public hotfix_entry_t
  {
    unsigned      id_;    // Spell ID
    std::string   field_name_;  // Field name to override
    hotfix_op_e   operation_;   // Operation to perform on the current DBC value
    double        modifier_;    // Modifier to apply to the operation
    double        orig_value_;  // The value to check against for DBC data changes
    double        dbc_value_;   // The value in the DBC before applying hotfix
    double        hotfix_value_; // The DBC value after hotfix applied

    dbc_hotfix_entry_t() :
      hotfix_entry_t(), id_( 0 ), field_name_(), operation_( HOTFIX_NONE ), modifier_( 0 ),
      orig_value_( -std::numeric_limits<double>::max() ), dbc_value_( 0 ), hotfix_value_( 0 )
    { }

    dbc_hotfix_entry_t( util::string_view g, util::string_view t, unsigned id, util::string_view n, unsigned f ) :
      hotfix_entry_t( g, t, n, f ),
      id_( id ), field_name_(), operation_( HOTFIX_NONE ), modifier_( 0 ),
      orig_value_( -std::numeric_limits<double>::max() ), dbc_value_( 0 ), hotfix_value_( 0 )
    { }

    bool valid() const override
    {
      return orig_value_ == -std::numeric_limits<double>::max() ||
             util::round( orig_value_, 5 ) == util::round( dbc_value_, 5 );
    }

    virtual void apply() override
    {
      if ( flags_ & HOTFIX_FLAG_LIVE )
      {
        apply_hotfix( false );
      }

#if SC_USE_PTR
      if ( flags_ & HOTFIX_FLAG_PTR )
      {
        apply_hotfix( true );
      }
#endif
    }

    dbc_hotfix_entry_t& field( util::string_view fn )
    { field_name_.assign( fn.data(), fn.size() ); return *this; }

    dbc_hotfix_entry_t& operation( hotfix::hotfix_op_e op )
    { operation_ = op; return *this; }

    dbc_hotfix_entry_t& modifier( double m )
    { modifier_ = m; return *this; }

    dbc_hotfix_entry_t& verification_value( double ov )
    { orig_value_ = ov; return *this; }

    private:
    virtual void apply_hotfix( bool ptr = false ) = 0;
  };

  struct spell_hotfix_entry_t : public dbc_hotfix_entry_t
  {
    spell_hotfix_entry_t( util::string_view g, util::string_view t, unsigned id, util::string_view n, unsigned f ) :
      dbc_hotfix_entry_t( g, t, id, n, f )
    { }

    std::string to_str() const override;

    private:
    void apply_hotfix( bool ptr ) override;
  };

  struct effect_hotfix_entry_t : public dbc_hotfix_entry_t
  {
    effect_hotfix_entry_t( util::string_view g, util::string_view t, unsigned id, util::string_view n, unsigned f ) :
      dbc_hotfix_entry_t( g, t, id, n, f )
    { }

    std::string to_str() const override;

    private:
    void apply_hotfix( bool ptr ) override;
  };

  struct power_hotfix_entry_t : public dbc_hotfix_entry_t
  {
    power_hotfix_entry_t( util::string_view g, util::string_view t, unsigned id, util::string_view n, unsigned f ) :
      dbc_hotfix_entry_t( g, t, id, n, f )
    { }

    std::string to_str() const override;

    private:
    void apply_hotfix( bool ptr ) override;
  };

  spell_hotfix_entry_t& register_spell( util::string_view, util::string_view, util::string_view, unsigned, unsigned = hotfix::HOTFIX_FLAG_DEFAULT );
  effect_hotfix_entry_t& register_effect( util::string_view, util::string_view, util::string_view, unsigned, unsigned = hotfix::HOTFIX_FLAG_DEFAULT );
  power_hotfix_entry_t& register_power( util::string_view, util::string_view, util::string_view, unsigned, unsigned = hotfix::HOTFIX_FLAG_DEFAULT );

  void apply();
  std::string to_str( bool ptr );

  void add_hotfix_spell( spell_data_t* spell, bool ptr = false );
  const spell_data_t* find_spell( const spell_data_t* dbc_spell, bool ptr = false );
  const spelleffect_data_t* find_effect( const spelleffect_data_t* dbc_effect, bool ptr = false );
  const spellpower_data_t* find_power( const spellpower_data_t* dbc_power, bool ptr = false );

  std::vector<const hotfix_entry_t*> hotfix_entries();

} // namespace hotfix

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

  explicit dbc_t( bool ptr = false ) :
    ptr( ptr ) { }

  const char* wow_ptr_status() const
  { return dbc::wow_ptr_status( ptr ); }

  util::span<const util::span<const dbc_item_data_t>> items() const
  { return dbc_item_data_t::data( ptr ); }

  // Gametables removed in Legion
  double melee_crit_base( player_e, unsigned ) const
  { return 0.05; }

  double spell_crit_base( player_e, unsigned ) const
  { return 0.05; }

  // Game data table access
  double combat_rating_multiplier( unsigned item_level, combat_rating_multiplier_type type ) const;
  double stamina_multiplier( unsigned item_level, combat_rating_multiplier_type type ) const;
  double melee_crit_base( pet_e t, unsigned level ) const;
  double spell_crit_base( pet_e t, unsigned level ) const;
  double dodge_base( player_e t ) const;
  double dodge_base( pet_e t ) const;
  double resource_base( player_e t, unsigned level ) const;
  double health_base( player_e t, unsigned level ) const;
  const stat_data_t& attribute_base( player_e t, unsigned level ) const;
  const stat_data_t& attribute_base( pet_e t, unsigned level ) const;
  const stat_data_t& race_base( race_e r ) const;
  const stat_data_t& race_base( pet_e t ) const;
  double parry_factor( player_e t ) const;
  double dodge_factor( player_e t ) const;
  double miss_factor( player_e t ) const;
  double block_factor( player_e t ) const;
  double block_vertical_stretch( player_e t ) const;
  double vertical_stretch( player_e t ) const;
  double horizontal_shift( player_e t ) const;

  double spell_scaling( player_e t, unsigned level ) const;
  double melee_crit_scaling( player_e t, unsigned level ) const;
  double melee_crit_scaling( pet_e t, unsigned level ) const;
  double spell_crit_scaling( player_e t, unsigned level ) const;
  double spell_crit_scaling( pet_e t, unsigned level ) const;
  double health_per_stamina( unsigned level ) const;
  double item_socket_cost( unsigned ilevel ) const;
  double armor_mitigation_constant( unsigned level ) const;
  double npc_armor_value( unsigned level ) const;

  double combat_rating( unsigned combat_rating_id, unsigned level ) const;

  int resolve_item_scaling( unsigned level ) const;
  double resolve_level_scaling( unsigned level ) const;
  double avoid_per_str_agi_by_level( unsigned level ) const;

  unsigned real_ppm_scale( unsigned ) const;
  double real_ppm_modifier( unsigned spell_id, player_t* player, unsigned item_level = 0 ) const;
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

  const spellpower_data_t& power( unsigned power_id ) const
  { return spellpower_data_t::find( power_id, ptr ); }

  // Always returns non-NULL.
  const talent_data_t*           talent( unsigned talent_id ) const
  { return find_by_id<talent_data_t>( talent_id ); }

  const dbc_item_data_t& item( unsigned item_id ) const
  { return dbc_item_data_t::find( item_id, ptr ); }

  const item_enchantment_data_t& item_enchantment( unsigned enchant_id ) const
  { return item_enchantment_data_t::find( enchant_id, ptr ); }

  const item_name_description_t& item_description( unsigned description_id ) const
  { return item_name_description_t::find( description_id, ptr ); }

  const gem_property_data_t&     gem_property( unsigned gem_id ) const
  { return gem_property_data_t::find( gem_id, ptr ); }

  const random_prop_data_t&      random_property( unsigned ilevel ) const
  { return random_prop_data_t::find( ilevel, ptr ); }

  unsigned                       random_property_max_level() const
  { return random_prop_data_t::data( ptr ).back().ilevel; }

  const item_damage_one_hand_data_t& item_damage_1h( unsigned ilevel ) const
  { return item_damage_one_hand_data_t::find( ilevel, ptr ); }

  const item_damage_two_hand_data_t& item_damage_2h( unsigned ilevel ) const
  { return item_damage_two_hand_data_t::find( ilevel, ptr ); }

  const item_damage_one_hand_caster_data_t& item_damage_caster_1h( unsigned ilevel ) const
  { return item_damage_one_hand_caster_data_t::find( ilevel, ptr ); }

  const item_damage_two_hand_caster_data_t& item_damage_caster_2h( unsigned ilevel ) const
  { return item_damage_two_hand_caster_data_t::find( ilevel, ptr ); }

  const item_armor_quality_data_t& item_armor_quality( unsigned ilevel ) const
  { return item_armor_quality_data_t::find( ilevel, ptr ); }

  const item_armor_shield_data_t& item_armor_shield( unsigned ilevel ) const
  { return item_armor_shield_data_t::find( ilevel, ptr ); }

  const item_armor_total_data_t&  item_armor_total( unsigned ilevel ) const
  { return item_armor_total_data_t::find( ilevel, ptr ); }

  const item_armor_location_data_t& item_armor_inv_type( unsigned inv_type ) const
  { return item_armor_location_data_t::find( inv_type, ptr ); }

  util::span<const item_bonus_entry_t> item_bonus( unsigned bonus_id ) const
  { return item_bonus_entry_t::find( bonus_id, ptr ); }

  util::span<const item_effect_t> item_effects( unsigned item_id ) const
  { return item_effect_t::find_for_item( item_id, ptr ); }

  std::vector<const racial_spell_entry_t*> racial_spell( player_e c, race_e r ) const;

  const spelltext_data_t& spell_text( unsigned spell_id ) const
  { return spelltext_data_t::find( spell_id, ptr ); }

  const spelldesc_vars_data_t& spell_desc_vars( unsigned spell_id ) const
  { return spelldesc_vars_data_t::find( spell_id, ptr ); }

  // Derived data access
  unsigned class_max_size() const;

  unsigned specialization_max_per_class() const;
  unsigned specialization_max_class() const;
  bool     ability_specialization( uint32_t spell_id, std::vector<specialization_e>& spec_list ) const;

  // Helper methods
  double   weapon_dps( unsigned item_id, unsigned ilevel = 0 ) const;
  double   weapon_dps( const dbc_item_data_t&, unsigned ilevel = 0 ) const;

  double   effect_average( unsigned effect_id, unsigned level ) const;
  double   effect_average( const spelleffect_data_t* effect, unsigned level ) const;
  double   effect_delta( unsigned effect_id, unsigned level ) const;
  double   effect_delta( const spelleffect_data_t* effect, unsigned level ) const;

  double   effect_min( unsigned effect_id, unsigned level ) const;
  double   effect_min( const spelleffect_data_t* effect, unsigned level ) const;
  double   effect_max( unsigned effect_id, unsigned level ) const;
  double   effect_max( const spelleffect_data_t* effect, unsigned level ) const;
  double   effect_bonus( unsigned effect_id, unsigned level ) const;
  double   effect_bonus( const spelleffect_data_t* effect, unsigned level ) const;

  unsigned talent_ability_id( player_e c, specialization_e spec_id, util::string_view spell_name, bool tokenized = false ) const;
  unsigned class_ability_id( player_e c, specialization_e spec_id, util::string_view spell_name, bool tokenized = false ) const;
  unsigned pet_ability_id( player_e c, util::string_view spell_name, bool tokenized = false ) const;
  unsigned race_ability_id( player_e c, race_e r, util::string_view spell_name ) const;
  unsigned specialization_ability_id( specialization_e spec_id, util::string_view spell_name, util::string_view spell_desc = "" ) const;
  unsigned mastery_ability_id( specialization_e spec ) const;

  bool     is_specialization_ability( uint32_t spell_id ) const;
  bool     is_specialization_ability( specialization_e spec_id, unsigned spell_id ) const;

  specialization_e spec_by_spell( uint32_t spell_id ) const;
  unsigned replace_spell_id( unsigned spell_id ) const;

  bool spec_idx( specialization_e spec_id, uint32_t& class_idx, uint32_t& spec_index ) const;
  specialization_e spec_by_idx( const player_e c, unsigned idx ) const;

  std::vector< const spell_data_t* > effect_affects_spells( unsigned, const spelleffect_data_t* ) const;
  std::vector< const spelleffect_data_t* > effects_affecting_spell( const spell_data_t* ) const;
  std::vector<const spelleffect_data_t*> effect_labels_affecting_spell( const spell_data_t* ) const;
  std::vector<const spelleffect_data_t*> effect_labels_affecting_label( short label ) const;
  std::vector<const spelleffect_data_t*> effect_categories_affecting_spell( const spell_data_t* ) const;

  std::pair<const curve_point_t*, const curve_point_t*> curve_point( unsigned curve_id, double value ) const;

  // Azerite
  const azerite_power_entry_t& azerite_power( unsigned power_id ) const;
  const azerite_power_entry_t& azerite_power( util::string_view name, bool tokenized = false ) const;
  util::span<const azerite_power_entry_t> azerite_powers() const;

  unsigned azerite_item_level( unsigned power_level ) const;

  // Child items
  unsigned child_item( unsigned ) const;
  unsigned parent_item( unsigned ) const;

  // Labeled spells
  util::span<const spell_data_t* const> spells_by_label( size_t label ) const;
  // Categorized spells
  util::span<const spell_data_t* const> spells_by_category( unsigned category ) const;
};

class dbc_override_t
{
public:
  enum override_e
  {
    OVERRIDE_SPELL = 0,
    OVERRIDE_EFFECT,
    OVERRIDE_POWER
  };

  struct override_entry_t
  {
    override_e  type_;
    unsigned    id_;
    std::string field_;
    double      value_;

    override_entry_t( override_e et, util::string_view f, unsigned i, double v ) :
      type_( et ), id_( i ), field_( f ), value_( v )
    { }
  };

  dbc_override_t() = default;

  explicit dbc_override_t( const dbc_override_t* parent ) : parent_( parent ) {}

  void register_effect( const dbc_t&, unsigned, util::string_view, double );
  void register_spell( const dbc_t&, unsigned, util::string_view, double );
  void register_power( const dbc_t&, unsigned, util::string_view, double );

  void parse( const dbc_t&, util::string_view );

  // Creates a "deep" copy of the current override database
  std::unique_ptr<dbc_override_t> clone() const;

  const spell_data_t* find_spell( unsigned, bool ptr = false ) const;
  const spelleffect_data_t* find_effect( unsigned, bool ptr = false ) const;
  const spellpower_data_t* find_power( unsigned, bool ptr = false ) const;

  const std::vector<override_entry_t>& override_entries( bool ptr = false ) const;

private:
  const dbc_override_t* parent_ = nullptr;
  std::array<custom_dbc_data_t, 2> override_db_;
  std::array<std::vector<override_entry_t>, 2> override_entries_;
};

namespace dbc
{

// Wrapper for fetching spell data through various spell data variants
template <typename T>
const spell_data_t* find_spell( const T* obj, const spell_data_t* spell )
{
  if ( const spell_data_t* override_spell = obj->dbc_override->find_spell( spell -> id(), obj -> dbc->ptr ) )
  {
    return override_spell;
  }

  if ( ! obj -> disable_hotfixes )
  {
    return hotfix::find_spell( spell, obj -> dbc->ptr );
  }

  return spell;
}

template <typename T>
const spell_data_t* find_spell( const T* obj, unsigned spell_id )
{
  if ( const spell_data_t* override_spell = obj->dbc_override->find_spell( spell_id, obj -> dbc->ptr ) )
  {
    return override_spell;
  }

  if ( ! obj -> disable_hotfixes )
  {
    return hotfix::find_spell( obj -> dbc->spell( spell_id ), obj -> dbc->ptr );
  }

  return obj -> dbc->spell( spell_id );
}

template <typename T>
const spelleffect_data_t* find_effect( const T* obj, unsigned effect_id )
{
  if ( const spelleffect_data_t* override_effect = obj->dbc_override->find_effect( effect_id, obj -> dbc->ptr ) )
  {
    return override_effect;
  }

  if ( ! obj -> disable_hotfixes )
  {
    return hotfix::find_effect( obj -> dbc->effect( effect_id ), obj -> dbc->ptr );
  }

  return obj -> dbc->effect( effect_id );
}

template <typename T>
const spellpower_data_t* find_power( const T* obj, unsigned power_id )
{
  if ( const spellpower_data_t* override_power = obj->dbc_override->find_power( power_id, obj -> dbc->ptr ) )
  {
    return override_power;
  }

  if ( ! obj -> disable_hotfixes )
  {
    return hotfix::find_power( & obj -> dbc->power( power_id ), obj -> dbc->ptr );
  }

  return & obj -> dbc->power( power_id );
}


} // dbc namespace ends

#endif // SC_DBC_HPP
