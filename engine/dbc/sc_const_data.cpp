// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "dbc.hpp"

#include "data_definitions.hh"
#include "item_database.hpp"
#include "client_data.hpp"
#include "specialization_spell.hpp"
#include "class_spells.hpp"
#include "racial_spells.hpp"
#include "trait_data.hpp"

#include "generated/sc_scale_data.inc"
#include "sc_extra_data.inc"

#if SC_USE_PTR
#include "generated/sc_scale_data_ptr.inc"
#include "sc_extra_data_ptr.inc"
#endif

#include "player/player.hpp"
#include "item/item.hpp"

namespace { // ANONYMOUS namespace ==========================================

// Wrapper class to map other data to specific spells, and also to map effects that manipulate that
// data
template <typename V>
class spell_mapping_reference_t
{
  // Map struct T (based on id) to a set of spells
  std::array<std::unordered_map<V, std::vector<const spell_data_t*>>, 2> m_db;
  // Map struct T (based on id) to a set of effects affecting the group T
  std::array<std::unordered_map<V, std::vector<const spelleffect_data_t*>>, 2> m_effects_db;

public:
  void add_spell( V value, const spell_data_t* data, bool ptr = false )
  {
    m_db[ ptr ][ value ].push_back( data );
  }

  void add_effect( V value, const spelleffect_data_t* data, bool ptr = false )
  {
    m_effects_db[ ptr ][ value ].push_back( data );
  }

  util::span<const spell_data_t* const> affects_spells( V value, bool ptr ) const
  {
    auto it = m_db[ ptr ].find( value );

    if ( it != m_db[ ptr ].end() )
    {
      return it -> second;
    }

    return {};
  }

  util::span<const spelleffect_data_t* const> affected_by( V value, bool ptr ) const
  {
    auto it = m_effects_db[ ptr ].find( value );

    if ( it != m_effects_db[ ptr ].end() )
    {
      return it -> second;
    }

    return {};
  }
};

std::array<std::vector< std::vector< const spell_data_t* > >, 2> class_family_index;

// Label -> spell mappings
spell_mapping_reference_t<short> spell_label_index;

// Categories -> spell mappings
spell_mapping_reference_t<unsigned> spell_categories_index;
spell_mapping_reference_t<unsigned> spell_category_mask_index;

} // ANONYMOUS namespace ====================================================

wowv_t wowv( bool ptr )
{ return dbc::client_data_version( ptr ); }

const char* dbc::wow_ptr_status( bool ptr )
#if SC_BETA
{ (void)ptr; return SC_BETA_STR "-BETA"; }
#else
{ return ( maybe_ptr( ptr ) ? "PTR" : "Live" ); }
#endif

static void generate_indices( bool ptr )
{
  for ( const spell_data_t& spell : spell_data_t::data( ptr ) )
  {
    // Make a class family index to speed up some spell query parsing
    if ( spell.class_family() != 0 )
    {
      auto& index = class_family_index[ ptr ];
      if ( index.size() <= spell.class_family() )
        index.resize( spell.class_family() + 1 );

      index[ spell.class_family() ].push_back( &spell );
    }

    if ( spell.category() != 0 )
    {
      spell_categories_index.add_spell( spell.category(), &spell, ptr );

      if ( auto _mask = spell.category_type() )
      {
        for ( unsigned i = 1; _mask; _mask >>= 1, i++ )
        {
          if ( _mask & 1 )
          {
            spell_category_mask_index.add_spell( i, &spell, ptr );
          }
        }
      }
    }

    for ( const spelllabel_data_t& label : spell.labels() )
    {
      spell_label_index.add_spell( label.label(), &spell, ptr );
    }

    for ( const spelleffect_data_t& effect : spell.effects() )
    {
      if ( range::contains( dbc::effect_category_subtypes(), effect.subtype() ) )
      {
        if ( const auto value = as<unsigned>( effect.misc_value1() ) )
          spell_categories_index.add_effect( value, &effect, ptr );
      }

      switch ( effect.subtype() )
      {
        case A_MOD_RECHARGE_RATE_LABEL:
        case A_MOD_TIME_RATE_BY_SPELL_LABEL:
        case A_MOD_DAMAGE_FROM_SPELLS_LABEL:
        case A_MOD_DAMAGE_FROM_CASTER_SPELLS_LABEL:
          if ( const short value = as<short>( effect.misc_value1() ) )
            spell_label_index.add_effect( value, &effect, ptr );
          break;
        case A_ADD_PCT_LABEL_MODIFIER:
        case A_ADD_FLAT_LABEL_MODIFIER:
          if ( const auto value = as<short>( effect.misc_value2() ) )
            spell_label_index.add_effect( value, &effect, ptr );
          break;
        default:
          break;
      }
    }
  }
}

/* Initialize database
 */
void dbc::init()
{
  // Create id-indexes
  init_item_data();

  // runtime linking, eg. from spell_data to all its effects
  spell_data_t::link( false );
  spelleffect_data_t::link( false );

  if ( SC_USE_PTR )
  {
    spell_data_t::link( true );
    spelleffect_data_t::link( true );
  }

  // Generate indices
  generate_indices( false );
  if ( SC_USE_PTR )
  {
    generate_indices( true );
  }
}

/* Validate gem color */
bool dbc::valid_gem_color( unsigned color )
{
  switch ( color )
  {
    case SOCKET_COLOR_NONE:
    case SOCKET_COLOR_META:
    case SOCKET_COLOR_RED:
    case SOCKET_COLOR_YELLOW:
    case SOCKET_COLOR_BLUE:
    case SOCKET_COLOR_ORANGE:
    case SOCKET_COLOR_PURPLE:
    case SOCKET_COLOR_GREEN:
    case SOCKET_COLOR_HYDRAULIC:
    case SOCKET_COLOR_PRISMATIC:
    case SOCKET_COLOR_COGWHEEL:
    case SOCKET_COLOR_IRON:
    case SOCKET_COLOR_BLOOD:
    case SOCKET_COLOR_SHADOW:
    case SOCKET_COLOR_FEL:
    case SOCKET_COLOR_ARCANE:
    case SOCKET_COLOR_FROST:
    case SOCKET_COLOR_FIRE:
    case SOCKET_COLOR_WATER:
    case SOCKET_COLOR_LIFE:
    case SOCKET_COLOR_WIND:
    case SOCKET_COLOR_HOLY:
    case SOCKET_COLOR_RED_PUNCHCARD:
    case SOCKET_COLOR_YELLOW_PUNCHCARD:
    case SOCKET_COLOR_BLUE_PUNCHCARD:
    case SOCKET_COLOR_CRYSTALLIC:
    case SOCKET_COLOR_TINKER:
    case SOCKET_COLOR_PRIMORDIAL:
    case SOCKET_COLOR_FRAGRANCE:
    case SOCKET_COLOR_THUNDER_CITRINE:
    case SOCKET_COLOR_SEA_CITRINE:
    case SOCKET_COLOR_WIND_CITRINE:
    case SOCKET_COLOR_RESHII_FIBER:
      return true;
    default:
      return false;
  }
}

double dbc::stat_data_to_attribute(const stat_data_t& s, attribute_e a) {
  switch (a) {
  case ATTR_STRENGTH:
    return s.strength;
  case ATTR_AGILITY:
    return s.agility;
  case ATTR_STAMINA:
    return s.stamina;
  case ATTR_INTELLECT:
    return s.intellect;
  case ATTR_SPIRIT:
    return s.spirit;
  default:
    assert(false);
  }
  return 0.0;
}

std::vector<const spell_data_t*> dbc_t::effect_affects_spells( unsigned family, const spelleffect_data_t* effect ) const
{
  std::vector<const spell_data_t*> affected_spells;

  if ( family == 0 )
    return affected_spells;

  const auto& index = class_family_index[ ptr ];
  if ( family >= index.size() )
    return affected_spells;

  for ( auto s : index[ family ] )
  {
    for ( unsigned int j = 0, vend = NUM_CLASS_FAMILY_FLAGS * 32; j < vend; j++ )
    {
      if ( effect->class_flag( j ) && s->class_flag( j ) && !range::contains( affected_spells, s ) )
      {
        affected_spells.push_back( s );
      }
    }
  }

  return affected_spells;
}

std::vector<const spell_data_t*> dbc_t::family_flag_affects_spells( unsigned family, unsigned flag ) const
{
  std::vector<const spell_data_t*> affected_spells;

  if ( family == 0 )
    return affected_spells;

  const auto& index = class_family_index[ ptr ];
  if ( family >= index.size() )
    return affected_spells;

  for ( auto s : index[ family ] )
  {
    if ( s->class_flag( flag ) )
    {
      affected_spells.push_back( s );
    }
  }

  return affected_spells;
}

std::vector<const spelleffect_data_t*> dbc_t::effect_labels_affecting_spell( const spell_data_t* spell ) const
{
  std::vector<const spelleffect_data_t*> effects;

  range::for_each( spell -> labels(), [ &effects, this ]( const spelllabel_data_t& label ) {
    auto label_effects = spell_label_index.affected_by( label.label(), ptr );

    // Add all effects affecting a specific label to the vector containing all the effects, if the
    // effect is not yet in the vector.
    range::for_each( label_effects, [ &effects ]( const spelleffect_data_t* data ) {
      auto it = range::find_if( effects, [ data ]( const spelleffect_data_t* effect ) {
        return effect -> id() == data -> id();
      } );

      if ( it == effects.end() )
      {
        effects.push_back( data );
      }
    } );
  } );

  return effects;
}

std::vector<const spelleffect_data_t*> dbc_t::effect_labels_affecting_label( short label ) const
{
  std::vector<const spelleffect_data_t*> effects;

  auto label_effects = spell_label_index.affected_by( label, ptr );

  // Add all effects affecting a specific label to the vector containing all the effects, if the
  // effect is not yet in the vector.
  range::for_each( label_effects, [ &effects ]( const spelleffect_data_t* data ) {
    auto it = range::find_if( effects, [ data ]( const spelleffect_data_t* effect ) {
      return effect -> id() == data -> id();
    } );

    if ( it == effects.end() )
    {
      effects.push_back( data );
    }
  } );

  return effects;
}

std::vector<const spelleffect_data_t*> dbc_t::effect_categories_affecting_spell( const spell_data_t* spell ) const
{
  std::vector<const spelleffect_data_t*> effects;

  auto category_effects = spell_categories_index.affected_by( spell -> category(), ptr );

  // Add all effects affecting a specific label to the vector containing all the effects, if the
  // effect is not yet in the vector.
  range::for_each( category_effects, [ &effects ]( const spelleffect_data_t* data ) {
    auto it = range::find_if( effects, [ data ]( const spelleffect_data_t* effect ) {
      return effect -> id() == data -> id();
    } );

    if ( it == effects.end() )
    {
      effects.push_back( data );
    }
  } );

  return effects;
}

std::vector< const spelleffect_data_t* > dbc_t::effects_affecting_spell( const spell_data_t* spell ) const
{
  std::vector< const spelleffect_data_t* > affecting_effects;

  if ( spell -> class_family() == 0 )
    return affecting_effects;

  const auto& index = class_family_index[ ptr ];
  if ( spell -> class_family() >= index.size() )
    return affecting_effects;

  for ( auto s : index[ spell -> class_family() ] )
  {
    // Skip itself
    if ( s -> id() == spell -> id() )
      continue;

    // Loop through all effects in the spell
    for ( size_t idx = 1, idx_end = s -> effect_count(); idx <= idx_end; idx++ )
    {
      const spelleffect_data_t& effect = s -> effectN( idx );

      // Match spell family flags
      for ( unsigned int j = 0, vend = NUM_CLASS_FAMILY_FLAGS * 32; j < vend; j++ )
      {
        if ( ! ( effect.class_flag( j ) && spell -> class_flag( j ) ) )
          continue;

        if ( std::find( affecting_effects.begin(), affecting_effects.end(), &effect ) == affecting_effects.end() )
          affecting_effects.push_back( &effect );
      }
    }
  }

  return affecting_effects;
}

specialization_e dbc::translate_spec_str( player_e ptype, util::string_view )
{
  switch ( ptype )
  {
    case DRUID:
      return SPEC_DRUID;
    case HUNTER:
      return SPEC_HUNTER;
    case MAGE:
      return SPEC_MAGE;
    case PALADIN:
      return SPEC_PALADIN;
    case PRIEST:
      return SPEC_PRIEST;
    case ROGUE:
      return SPEC_ROGUE;
    case SHAMAN:
      return SPEC_SHAMAN;
    case WARLOCK:
      return SPEC_WARLOCK;
    case WARRIOR:
      return SPEC_WARRIOR;
    default:
      break;
  }
  return SPEC_NONE;
}

const char* dbc::specialization_string( specialization_e spec )
{
  switch ( spec )
  {
    case SPEC_NONE:
      return "none";
    case SPEC_MAGE:
      return "mage";
    case SPEC_DRUID:
      return "druid";
    case SPEC_HUNTER:
      return "hunter";
    case SPEC_PALADIN:
      return "paladin";
    case SPEC_PRIEST:
      return "priest";
    case SPEC_ROGUE:
      return "rogue";
    case SPEC_SHAMAN:
      return "shaman";
    case SPEC_WARLOCK:
      return "warlock";
    case SPEC_WARRIOR:
      return "warrior";
    default:
      break;
  }

  return "unknown";
}

double dbc::fmt_value( double v, effect_type_t type, effect_subtype_t sub_type )
{
  // Automagically divide by 100.0 for percent based abilities
  switch ( type )
  {
    case E_ENERGIZE_PCT:
    case E_WEAPON_PERCENT_DAMAGE:
      v /= 100.0;
      break;
    case E_APPLY_AURA:
    case E_APPLY_AREA_AURA_PARTY:
    case E_APPLY_AREA_AURA_RAID:
      switch ( sub_type )
      {
        case A_HASTE_ALL:
        case A_MOD_HIT_CHANCE:
        case A_MOD_SPELL_HIT_CHANCE:
        case A_ADD_PCT_MODIFIER:
        case A_MOD_OFFHAND_DAMAGE_PCT:
        case A_MOD_ATTACK_POWER_PCT:
        case A_MOD_RANGED_ATTACK_POWER_PCT:
        case A_MOD_TOTAL_STAT_PERCENTAGE:
        case A_MOD_INCREASES_SPELL_PCT_TO_HIT:
        case A_MOD_CASTING_SPEED_NOT_STACK: // Wrath of Air, note this can go > +-100, but only on NPC (and possibly item) abilities
        case A_MOD_SPELL_DAMAGE_OF_ATTACK_POWER:
        case A_MOD_SPELL_HEALING_OF_ATTACK_POWER:
        case A_MOD_SPELL_DAMAGE_OF_STAT_PERCENT:
        case A_MOD_SPELL_HEALING_OF_STAT_PERCENT:
        case A_MOD_DAMAGE_PERCENT_DONE:
        case A_MOD_DAMAGE_FROM_CASTER: // Hunter's Mark
        case A_MOD_DAMAGE_FROM_CASTER_SPELLS: // vendetta
        case A_MOD_ALL_CRIT_CHANCE:
        case A_MOD_EXPERTISE:
        case A_MOD_MANA_REGEN_INTERRUPT:  // Meditation
        case A_MOD_CRIT_CHANCE_FROM_CASTER_SPELLS: // Increase critical chance of something, Stormstrike, Mind Spike, Holy Word: Serenity
        case A_MOD_SPELL_POWER_PCT: // Totemic Wrath, Flametongue Totem, Demonic Pact, etc ...
        case A_MOD_MELEE_AUTO_ATTACK_SPEED:
        case A_MOD_RANGED_AND_MELEE_AUTO_ATTACK_SPEED:
          v /= 100.0;
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }

  return v;
}

namespace {

// Generate a spec -> index map from the __class_spec_id table at compile time
// Ideally we'd use an std::array instead of a custom struct, but sadly it is
// not fully constexpr even in C++17...
[[maybe_unused]] constexpr int max_spec_index()
{
  int max = -1;
  for ( size_t i = 0; i < std::size( __class_spec_id ); i++ )  // NOLINT(modernize-loop-convert)
  {
    for ( size_t index = 0; index < std::size( __class_spec_id[ i ] ); index++ )
    {
      specialization_e spec = __class_spec_id[ i ][ index ];
      max = static_cast<int>( spec ) > max ? static_cast<int>( spec ) : max;
    }
  }
  return max;
}
#if SC_USE_PTR
constexpr int max_spec_index_ptr()
{
  int max = -1;
  for ( size_t i = 0; i < std::size( __ptr_class_spec_id ); i++ )  // NOLINT(modernize-loop-convert)
  {
    for ( size_t index = 0; index < std::size( __ptr_class_spec_id[ i ] ); index++ )
    {
      specialization_e spec = __ptr_class_spec_id[ i ][ index ];
      max = static_cast<int>( spec ) > max ? static_cast<int>( spec ) : max;
    }
  }
  return max;
}
#endif
struct spec_index_map_t
{
  constexpr spec_index_map_t( [[maybe_unused]] bool ptr = false ) : data_{}
  {
    for ( int8_t& index : data_ )
      index = -1;

      // Keep index based lookup since Visual Studio causes problems otherwise with constexpr evaluation
#if SC_USE_PTR
    for ( size_t i = 0; i < ( ptr ? PTR_MAX_SPEC_CLASS : MAX_SPEC_CLASS ); i++ )  // NOLINT(modernize-loop-convert)
      for ( size_t j = 0; j < ( ptr ? PTR_MAX_SPECS_PER_CLASS : MAX_SPECS_PER_CLASS ); j++ )
        if ( auto spec = ( ptr ? __ptr_class_spec_id[ i ][ j ] : __class_spec_id[ i ][ j ] ); spec != SPEC_NONE )
          data_[ spec ] = static_cast<int8_t>( j );
#else
    for ( size_t i = 0; i < ( MAX_SPEC_CLASS ); i++ )  // NOLINT(modernize-loop-convert)
      for ( size_t j = 0; j < ( MAX_SPECS_PER_CLASS ); j++ )
        if ( auto spec = ( __class_spec_id[ i ][ j ] ); spec != SPEC_NONE )
          data_[ spec ] = static_cast<int8_t>( j );
#endif
  }

  constexpr int8_t operator[]( specialization_e spec ) const
  {
    assert( spec < std::size( data_ ) );
    return data_[ spec ];
  }
#if SC_USE_PTR
  std::array<int8_t, max_spec_index_ptr() + 1> data_;
#else
  std::array<int8_t, max_spec_index() + 1> data_;
#endif
};
} // anon namespace

int dbc::spec_idx( specialization_e spec, [[maybe_unused]] bool ptr )
{
#if SC_USE_PTR
  static constexpr spec_index_map_t spec_index_map;
  static constexpr spec_index_map_t spec_index_map_ptr = spec_index_map_t( true );
  return ptr ? spec_index_map_ptr[ spec ] : spec_index_map[ spec ];
#else
  static constexpr spec_index_map_t spec_index_map;
  return spec_index_map[ spec ];
#endif
}

namespace
{
struct hero_tree_index_map_t
{
  std::array<int8_t, hero_tree_e::HERO_MAX> data;

private:
  std::array<int8_t, MAX_SPEC_CLASS> classes;

public:
  constexpr hero_tree_index_map_t( [[maybe_unused]] bool ptr = false ) : data{}, classes{}
  {
    for ( int8_t& value : data )
      value = -1;
#if SC_USE_PTR
    auto __data = ptr ? __ptr_trait_sub_tree_data : __trait_sub_tree_data;
#else
    auto __data = __trait_sub_tree_data;
#endif
    for ( size_t hero = 0; hero < __data.size(); hero++ )
    {
      size_t hero_index  = std::get<0>( __data[ hero ] );
      size_t class_index = std::get<2>( __data[ hero ] );
      data[ hero_index ] = classes[ class_index ];
      classes[ class_index ]++;
    }
  }

  constexpr int8_t operator[]( hero_tree_e hero_tree ) const
  {
    assert( hero_tree < std::size( data ) );
    return data[ hero_tree ];
  }
};
}  // namespace

int dbc::hero_idx( hero_tree_e hero_talent, [[maybe_unused]] bool ptr )
{
#if SC_USE_PTR
  static constexpr hero_tree_index_map_t hero_tree_index_map;
  static constexpr hero_tree_index_map_t hero_tree_index_map_ptr = hero_tree_index_map_t( true );
  return ptr ? hero_tree_index_map_ptr[ hero_talent ] : hero_tree_index_map[ hero_talent ];
#else
  static constexpr hero_tree_index_map_t hero_tree_index_map;
  return hero_tree_index_map[ hero_talent ];
#endif
}

int dbc::composite_idx( specialization_e spec, hero_tree_e hero, bool ptr )
{
  assert( ( spec != SPEC_NONE ) != ( hero != HERO_NONE ) );
  int index = 0;

  if ( spec == SPEC_NONE )
    index += 4;
  else
    index += spec_idx( spec, ptr );

  if ( hero == HERO_NONE )
    index += 0;
  else
    index += hero_idx( hero, ptr );

  return index;
}

uint32_t dbc::get_school_mask( school_e s )
{
  switch ( s )
  {
    case SCHOOL_PHYSICAL      : return 0x01;
    case SCHOOL_HOLY          : return 0x02;
    case SCHOOL_FIRE          : return 0x04;
    case SCHOOL_NATURE        : return 0x08;
    case SCHOOL_FROST         : return 0x10;
    case SCHOOL_SHADOW        : return 0x20;
    case SCHOOL_ARCANE        : return 0x40;
    case SCHOOL_HOLYSTRIKE    : return 0x03;
    case SCHOOL_FLAMESTRIKE   : return 0x05;
    case SCHOOL_HOLYFIRE      : return 0x06;
    case SCHOOL_STORMSTRIKE   : return 0x09;
    case SCHOOL_HOLYSTORM     : return 0x0a;
    case SCHOOL_FIRESTORM     : return 0x0c;
    case SCHOOL_FROSTSTRIKE   : return 0x11;
    case SCHOOL_HOLYFROST     : return 0x12;
    case SCHOOL_FROSTFIRE     : return 0x14;
    case SCHOOL_FROSTSTORM    : return 0x18;
    case SCHOOL_SHADOWSTRIKE  : return 0x21;
    case SCHOOL_SHADOWLIGHT   : return 0x22;
    case SCHOOL_SHADOWFLAME   : return 0x24;
    case SCHOOL_SHADOWSTORM   : return 0x28;
    case SCHOOL_SHADOWFROST   : return 0x30;
    case SCHOOL_SPELLSTRIKE   : return 0x41;
    case SCHOOL_DIVINE        : return 0x42;
    case SCHOOL_SPELLFIRE     : return 0x44;
    case SCHOOL_ASTRAL        : return 0x48;
    case SCHOOL_SPELLFROST    : return 0x50;
    case SCHOOL_SPELLSHADOW   : return 0x60;
    case SCHOOL_ELEMENTAL     : return 0x1c;
    case SCHOOL_COSMIC        : return 0x6a;
    case SCHOOL_CHROMATIC     : return 0x7c;
    case SCHOOL_CHROMASTRIKE  : return 0x7d;
    case SCHOOL_MAGIC         : return 0x7e;
    case SCHOOL_CHAOS         : return 0x7f;
    default                   : return 0x00;
  }
}

school_e dbc::get_school_type( uint32_t school_id )
{
  switch ( school_id )
  {
    case 0x01: return SCHOOL_PHYSICAL;
    case 0x02: return SCHOOL_HOLY;
    case 0x04: return SCHOOL_FIRE;
    case 0x08: return SCHOOL_NATURE;
    case 0x10: return SCHOOL_FROST;
    case 0x20: return SCHOOL_SHADOW;
    case 0x40: return SCHOOL_ARCANE;
    case 0x03: return SCHOOL_HOLYSTRIKE;
    case 0x05: return SCHOOL_FLAMESTRIKE;
    case 0x06: return SCHOOL_HOLYFIRE;
    case 0x09: return SCHOOL_STORMSTRIKE;
    case 0x0a: return SCHOOL_HOLYSTORM;
    case 0x0c: return SCHOOL_FIRESTORM;
    case 0x11: return SCHOOL_FROSTSTRIKE;
    case 0x12: return SCHOOL_HOLYFROST;
    case 0x14: return SCHOOL_FROSTFIRE;
    case 0x18: return SCHOOL_FROSTSTORM;
    case 0x21: return SCHOOL_SHADOWSTRIKE;
    case 0x22: return SCHOOL_SHADOWLIGHT;
    case 0x24: return SCHOOL_SHADOWFLAME;
    case 0x28: return SCHOOL_SHADOWSTORM;
    case 0x30: return SCHOOL_SHADOWFROST;
    case 0x41: return SCHOOL_SPELLSTRIKE;
    case 0x42: return SCHOOL_DIVINE;
    case 0x44: return SCHOOL_SPELLFIRE;
    case 0x48: return SCHOOL_ASTRAL;
    case 0x50: return SCHOOL_SPELLFROST;
    case 0x60: return SCHOOL_SPELLSHADOW;
    case 0x1c: return SCHOOL_ELEMENTAL;
    case 0x6a: return SCHOOL_COSMIC;
    case 0x7c: return SCHOOL_CHROMATIC;
    case 0x7d: return SCHOOL_CHROMASTRIKE;
    case 0x7e: return SCHOOL_MAGIC;
    case 0x7f: return SCHOOL_CHAOS;
    default:   return SCHOOL_NONE;
  }
}

bool dbc::is_school( school_e s, school_e s2 )
{
  uint32_t mask2 = get_school_mask( s2 );
  return ( get_school_mask( s ) & mask2 ) == mask2;
}

bool dbc::has_common_school( school_e s1, school_e s2 )
{
  return ( get_school_mask( s1 ) & get_school_mask( s2 ) ) != 0;
}

// TODO: adapt for ptr dbc if necessary
player_e dbc::get_class_from_spec( specialization_e spec )
{
  // index 0 is pets
  for ( int i = 1; i < MAX_SPEC_CLASS; i++ )
  {
    if ( range::contains( __class_spec_id[ i ], spec ) )
      return util::translate_class_id( i );
  }

  return PLAYER_NONE;
}

double dbc::item_level_squish( unsigned source_ilevel, [[maybe_unused]] bool ptr )
{
  if ( source_ilevel == 0 )
  {
    return 1;
  }

#if SC_USE_PTR == 1
  if ( ptr )
  {
    assert( std::size( _ptr__item_level_squish ) >= source_ilevel );
    return _ptr__item_level_squish[ source_ilevel - 1 ];
  }
  else
  {
    assert( std::size( __item_level_squish ) >= source_ilevel );
    return __item_level_squish[ source_ilevel - 1 ];
  }
#else
  assert( std::size( __item_level_squish ) >= source_ilevel );
  return __item_level_squish[ source_ilevel - 1 ];
#endif
}

uint32_t dbc_t::replaced_id( uint32_t id_spell ) const
{
  auto it = replaced_ids.find( id_spell );
  if ( it == replaced_ids.end() )
    return 0;

  return ( *it ).second;
}

bool dbc_t::replace_id( uint32_t id_spell, uint32_t replaced_by_id )
{
  std::pair< id_map_t::iterator, bool > pr =
    replaced_ids.insert( std::make_pair( id_spell, replaced_by_id ) );
  // New entry
  if ( pr.second )
    return true;
  // Already exists with that token
  if ( ( pr.first ) -> second == replaced_by_id )
    return true;
  // Trying to specify a repalced_id for an existing spell id
  return false;
}

util::span<const effect_subtype_t> dbc::effect_category_subtypes()
{
  static constexpr effect_subtype_t subtypes[] = {
    A_MOD_CHARGE_RECHARGE_RATE,
    A_MODIFY_CATEGORY_COOLDOWN,
    A_MOD_RECHARGE_TIME_CATEGORY,
    A_MOD_RECHARGE_TIME_PCT_CATEGORY,
    A_MOD_MAX_CHARGES,
    A_HASTED_CATEGORY,
    A_IGNORE_SPELL_CHARGE_COOLDOWN_CATEGORY,
  };
  return util::make_span( subtypes );
}

double dbc_t::combat_rating_multiplier( unsigned item_level, combat_rating_multiplier_type type ) const
{
  assert( item_level > 0 && item_level <= MAX_ILEVEL );
  assert( type < CR_MULTIPLIER_MAX );
#if SC_USE_PTR
  return ptr ? _ptr__combat_ratings_mult_by_ilvl[ type ][ item_level - 1 ]
             : __combat_ratings_mult_by_ilvl[type][ item_level - 1 ];
#else
  return __combat_ratings_mult_by_ilvl[type][ item_level - 1 ];
#endif
}

double dbc_t::stamina_multiplier( unsigned item_level, combat_rating_multiplier_type type ) const
{
  assert( item_level > 0 && item_level <= MAX_ILEVEL );
  assert( type < CR_MULTIPLIER_MAX );
#if SC_USE_PTR
  return ptr ? _ptr__stamina_mult_by_ilvl[ type ][ item_level - 1 ]
             : __stamina_mult_by_ilvl[type][ item_level - 1 ];
#else
  return __stamina_mult_by_ilvl[type][ item_level - 1 ];
#endif
}

double dbc_t::melee_crit_base( pet_e t, unsigned level ) const
{
  return melee_crit_base( util::pet_class_type( t ), level );
}

double dbc_t::spell_crit_base( pet_e t, unsigned level ) const
{
  return spell_crit_base( util::pet_class_type( t ), level );
}

double dbc_t::dodge_base( player_e ) const
{
  // base dodge is now 3.0 for all classes
  return 3.0;
}

double dbc_t::dodge_base( pet_e t ) const
{
  return dodge_base( util::pet_class_type( t ) );
}

const stat_data_t& dbc_t::race_base( race_e r ) const
{
  uint32_t race_id = util::race_id( r );

  const auto data = SC_DBC_GET_DATA( __gt_race_stats, __ptr_gt_race_stats, ptr );
  return data[ race_id ];
}

const stat_data_t& dbc_t::race_base( pet_e /* r */ ) const
{
  return race_base( RACE_NONE );
}

double dbc_t::dodge_factor( player_e t ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < dbc_t::class_max_size() );
#if SC_USE_PTR
  return ptr ? __ptr_gt_DodgeFactor[ class_id ]
             : __gt_DodgeFactor[ class_id ];
#else
  return __gt_DodgeFactor[ class_id ];
#endif
}

double dbc_t::parry_factor( player_e t ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < dbc_t::class_max_size() );
#if SC_USE_PTR
  return ptr ? __ptr_gt_ParryFactor[ class_id ]
             : __gt_ParryFactor[ class_id ];
#else
  return __gt_ParryFactor[ class_id ];
#endif
}

double dbc_t::miss_factor( player_e t ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < dbc_t::class_max_size() );
#if SC_USE_PTR
  return ptr ? __ptr_gt_MissFactor[ class_id ]
             : __gt_MissFactor[ class_id ];
#else
  return __gt_MissFactor[ class_id ];
#endif
}

double dbc_t::block_factor( player_e t ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < dbc_t::class_max_size() );
#if SC_USE_PTR
  return ptr ? __ptr_gt_BlockFactor[ class_id ]
             : __gt_BlockFactor[ class_id ];
#else
  return __gt_BlockFactor[ class_id ];
#endif
}

double dbc_t::block_vertical_stretch( player_e t ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < dbc_t::class_max_size() );
#if SC_USE_PTR
  return ptr ? __ptr_gt_BlockVerticalStretch[ class_id ]
    : __gt_BlockVerticalStretch[ class_id ];
#else
  return __gt_BlockVerticalStretch[ class_id ];
#endif
}

double dbc_t::vertical_stretch( player_e t ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < dbc_t::class_max_size() );
#if SC_USE_PTR
  return ptr ? __ptr_gt_VerticalStretch[ class_id ]
             : __gt_VerticalStretch[ class_id ];
#else
  return __gt_VerticalStretch[ class_id ];
#endif
}

double dbc_t::horizontal_shift( player_e t ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < dbc_t::class_max_size() );
#if SC_USE_PTR
  return ptr ? __ptr_gt_HorizontalShift[ class_id ]
             : __gt_HorizontalShift[ class_id ];
#else
  return __gt_HorizontalShift[ class_id ];
#endif
}

double dbc_t::spell_scaling( player_e t, unsigned level ) const
{
  uint32_t class_id = util::class_id( t );

  // "9" is the number of scaling class columns in sc_scale_data.inc from SpellScaling.txt
  // Adjust this if more tables are added to the scale generator in dbc_extract.py
  assert( class_id < dbc_t::class_max_size() + 9 && level > 0 && level <= MAX_SCALING_LEVEL );
#if SC_USE_PTR
  return ptr ? _ptr__spell_scaling[ class_id ][ level - 1 ]
             : __spell_scaling[ class_id ][ level - 1 ];
#else
  return __spell_scaling[ class_id ][ level - 1 ];
#endif
}

double dbc_t::avoid_per_str_agi_by_level( unsigned level ) const
{
  assert( level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? _ptr_gt_avoid_per_str_agi_by_level[level - 1]
             : _gt_avoid_per_str_agi_by_level[level - 1];
#else
  return _gt_avoid_per_str_agi_by_level[level - 1];
#endif
}

double dbc_t::resource_base( player_e t, unsigned level ) const
{
  auto class_id = util::class_id( t );

#if SC_USE_PTR
  assert( class_id < ( ptr ? PTR_MAX_CLASS : MAX_CLASS ) && level > 0 && level <= MAX_SCALING_LEVEL );
  return ptr ? _ptr__base_mp[ class_id ][ level - 1 ]
             : __base_mp[ class_id ][ level - 1 ];
#else
  assert( class_id < MAX_CLASS && level > 0 && level <= MAX_SCALING_LEVEL );
  return __base_mp[ class_id ][ level - 1 ];
#endif
}

double dbc_t::health_per_stamina( unsigned level ) const
{
  assert( level > 0 && level <= MAX_SCALING_LEVEL );
#if SC_USE_PTR
  return ptr ? _ptr__hp_per_sta[ level - 1 ]
             : __hp_per_sta[ level - 1 ];
#else
  return __hp_per_sta[ level - 1 ];
#endif
}


const stat_data_t& dbc_t::attribute_base( player_e t, unsigned level ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < dbc_t::class_max_size() && level > 0 && level <= MAX_LEVEL );
  const auto data = SC_DBC_GET_DATA( __gt_class_stats_by_level, __ptr_gt_class_stats_by_level, ptr );
  return data[ class_id ][ level - 1 ];
}

const stat_data_t& dbc_t::attribute_base( pet_e t, unsigned level ) const
{
  return attribute_base( util::pet_class_type( t ), level );
}

double dbc_t::combat_rating( unsigned combat_rating_id, unsigned level ) const
{
  assert( combat_rating_id < RATING_MAX );
  assert( level <= MAX_SCALING_LEVEL );
#if SC_USE_PTR
  return ptr ? _ptr__combat_ratings[ combat_rating_id ][ level - 1 ] * 100.0
             : __combat_ratings[ combat_rating_id ][ level - 1 ] * 100.0;
#else
  return __combat_ratings[ combat_rating_id ][ level - 1 ] * 100.0 ;
#endif
}

unsigned dbc_t::class_max_size() const
{
#if SC_USE_PTR
  return ptr ? PTR_MAX_CLASS : MAX_CLASS;
#else
  return MAX_CLASS;
#endif
}

unsigned dbc_t::specialization_max_per_class() const
{
#if SC_USE_PTR
  return ptr ? PTR_MAX_SPECS_PER_CLASS : MAX_SPECS_PER_CLASS;
#else
  return MAX_SPECS_PER_CLASS;
#endif
}

unsigned dbc_t::specialization_max_class() const
{
#if SC_USE_PTR
  return ptr ? PTR_MAX_SPEC_CLASS : MAX_SPEC_CLASS;
#else
  return MAX_SPEC_CLASS;
#endif
}

unsigned dbc_t::hero_trees_max_per_class() const
{
#if SC_USE_PTR
  return ptr ? PTR_MAX_HERO_TREES_PER_CLASS : MAX_HERO_TREES_PER_CLASS;
#else
  return MAX_HERO_TREES_PER_CLASS;
#endif
}

double dbc_t::item_socket_cost( unsigned ilevel ) const
{
  assert( ilevel > 0 && ( ilevel <= random_property_max_level() ) );
#if SC_USE_PTR
  return ptr ? _ptr__item_socket_cost_per_level[ ilevel - 1 ]
             : __item_socket_cost_per_level[ ilevel - 1 ];
#else
  return __item_socket_cost_per_level[ ilevel - 1 ];
#endif
}

double dbc_t::armor_mitigation_constant( unsigned level ) const
{
  assert( level > 0 && level <= ( MAX_SCALING_LEVEL + 3 ) );
  return expected_stat( level ).armor_constant;
}

double dbc_t::get_armor_constant_mod( difficulty_e diff ) const
{
  return expected_stat_mod( diff, &expected_stat_mod_t::armor_constant );
}

double dbc_t::npc_armor_value( unsigned level ) const
{
  assert( level > 0 && level <= ( MAX_SCALING_LEVEL + 3 ) );
  return expected_stat( level ).creature_armor;
}

double dbc_t::expected_creature_health( unsigned level ) const
{
  assert( level > 0 && level <= ( MAX_SCALING_LEVEL + 3 ) );
  return expected_stat( level ).creature_health;
}

double dbc_t::expected_creature_health_mod( difficulty_e diff ) const
{
  return expected_stat_mod( diff, &expected_stat_mod_t::creature_health );
}

/* Generic helper methods */

double dbc_t::effect_average( unsigned effect_id, unsigned level ) const
{
  return effect_average( effect( effect_id ), level );
}

double dbc_t::effect_average( const spelleffect_data_t* e, unsigned level ) const
{
  assert( e && ( level > 0 ) && ( level <= MAX_SCALING_LEVEL ) );

  auto scale = e->scaling_class();

  if ( scale == PLAYER_NONE && e->spell()->max_scaling_level() > 0 )
    scale = PLAYER_SPECIAL_SCALE8;

  if ( e->m_coefficient() != 0 && scale != PLAYER_NONE )
  {
    if ( e->spell()->max_scaling_level() > 0 )
      level = std::min( level, e->spell()->max_scaling_level() );

    return e->m_coefficient() * spell_scaling( scale, level );
  }
  else if ( e -> real_ppl() != 0 )
  {
    const spell_data_t* s = spell( e -> spell_id() );

    if ( s -> max_level() > 0 )
      return e -> base_value() + ( std::min( level, s -> max_level() ) - s -> level() ) * e -> real_ppl();
    else
      return e -> base_value() + ( level - s -> level() ) * e -> real_ppl();
  }
  else
    return e -> base_value();
}

double dbc_t::effect_delta( unsigned effect_id, unsigned level ) const
{
  return effect_delta( effect( effect_id ), level );
}

double dbc_t::effect_delta( const spelleffect_data_t* e, unsigned level ) const
{
  (void)level; assert( e && ( level > 0 ) && ( level <= MAX_SCALING_LEVEL ) );

  if ( e -> m_delta() != 0 && e->scaling_class() != 0 )
  {
    return  e->m_delta();
  }

  return 0;
}

double dbc_t::effect_bonus( unsigned effect_id, unsigned level ) const
{
  return effect_bonus( effect( effect_id ), level );
}

double dbc_t::effect_bonus( const spelleffect_data_t* e, unsigned level ) const
{
  assert( e && ( level > 0 ) && ( level <= MAX_SCALING_LEVEL ) );

  if ( e -> m_unk() != 0 && e->scaling_class() != 0 )
  {
    unsigned scaling_level = level;
    if ( e->spell()->max_scaling_level() > 0 )
      scaling_level = std::min( scaling_level, e->spell()->max_scaling_level() );
    double m_scale = spell_scaling( e->scaling_class(), scaling_level );

    return e -> m_unk() * m_scale;
  }
  else
    return e -> pp_combo_points();

  return 0;
}

double dbc_t::effect_min( unsigned effect_id, unsigned level ) const
{
  if ( ! effect_id )
    return 0.0;

  return effect_min( effect( effect_id ), level );
}

double dbc_t::effect_min( const spelleffect_data_t* e, unsigned level ) const
{
  if ( ! e )
    return 0.0;

  double result;

  assert( e && ( level > 0 ) );
  assert( ( level <= MAX_SCALING_LEVEL ) );

  unsigned c_id = util::class_id( e->scaling_class() );
  double avg = effect_average( e, level );

  if ( c_id != 0 && ( e -> m_coefficient() != 0 || e -> m_delta() != 0 ) )
  {
    double delta = effect_delta( e, level );
    result = avg - ( avg * delta / 2 );
  }
  else
  {
    result = avg;

    switch ( e -> type() )
    {
      case E_WEAPON_PERCENT_DAMAGE :
        result *= 0.01;
        break;
      default:
        break;
    }
  }

  return result;
}

double dbc_t::effect_max( unsigned effect_id, unsigned level ) const
{
  return effect_max( effect( effect_id ), level );
}

double dbc_t::effect_max( const spelleffect_data_t* e, unsigned level ) const
{
  double result;

  assert( e && ( level > 0 ) && ( level <= MAX_SCALING_LEVEL ) );

  unsigned c_id = util::class_id( e->scaling_class() );
  double avg = effect_average( e, level );

  if ( c_id != 0 && ( e -> m_coefficient() != 0 || e -> m_delta() != 0 ) )
  {
    double delta = effect_delta( e, level );

    result = avg + ( avg * delta / 2 );
  }
  else
  {
    result = avg;

    switch ( e -> type() )
    {
      case E_WEAPON_PERCENT_DAMAGE :
        result *= 0.01;
        break;
      default:
        break;
    }
  }

  return result;
}

unsigned dbc_t::class_ability_id( player_e          c,
                                  specialization_e  spec_id,
                                  util::string_view spell_name,
                                  bool              name_tokenized ) const
{
  const active_class_spell_t* active_spell = nullptr;
  if ( spec_id != SPEC_NONE )
  {
    active_spell = &active_class_spell_t::find( spell_name, spec_id, ptr, name_tokenized );

    // Try to find in class-specific general spells
    if ( active_spell->spell_id == 0U )
    {
      unsigned class_idx = 0;
      unsigned spec_index = 0;

      if ( !spec_idx( spec_id, class_idx, spec_index ) )
      {
        return 0U;
      }

      active_spell = &active_class_spell_t::find( spell_name,
          util::translate_class_id( class_idx ), ptr, name_tokenized );
    }
  }
  else if ( c != PLAYER_NONE )
  {
    active_spell = &active_class_spell_t::find( spell_name, c, ptr, name_tokenized );
  }
  else
  {
    active_spell = &active_class_spell_t::find( spell_name, ptr, name_tokenized );
  }

  if ( active_spell->spell_id == 0U )
  {
    return 0U;
  }

  if ( !replaced_id( active_spell->spell_id ) )
  {
    return active_spell->spell_id;
  }
  else
  {
    return 0U;
  }
}

unsigned dbc_t::pet_ability_id( player_e c, util::string_view name, bool tokenized ) const
{
  const active_pet_spell_t* active_spell = nullptr;
  if ( c != PLAYER_NONE )
  {
    active_spell = &active_pet_spell_t::find( name, c, ptr, tokenized );
  }
  else
  {
    active_spell = &active_pet_spell_t::find( name, ptr, tokenized );
  }

  if ( active_spell->spell_id == 0U )
  {
    return 0U;
  }

  if ( !replaced_id( active_spell->spell_id ) )
  {
    return active_spell->spell_id;
  }
  else
  {
    return 0U;
  }
}

unsigned dbc_t::race_ability_id( player_e c, race_e r, util::string_view spell_name ) const
{
  const auto& racial_spell = racial_spell_entry_t::find( spell_name, ptr, r, c );

  if ( racial_spell.spell_id != 0 && !replaced_id( racial_spell.spell_id ) )
  {
    return racial_spell.spell_id;
  }

  return 0;
}

std::vector<const racial_spell_entry_t*> dbc_t::racial_spell( player_e c, race_e r ) const
{
  std::vector<const racial_spell_entry_t*> __data;

  auto race_mask = util::race_mask( r );
  auto class_mask = util::class_id_mask( c );

  for ( const auto& entry : racial_spell_entry_t::data( ptr ) )
  {
    if ( !( entry.mask_race & race_mask  ) )
    {
      continue;
    }

    if ( entry.mask_class != 0 && !( entry.mask_class & class_mask ) )
    {
      continue;
    }

    __data.push_back( &entry );

    // Racials on client data export are sorted in ascending mask order, so anything past
    // the player's own race mask can be safely ignored.
    if ( entry.mask_race > race_mask )
    {
      break;
    }
  }

  return __data;
}

unsigned dbc_t::specialization_ability_id( specialization_e  spec_id,
                                           util::string_view spell_name,
                                           util::string_view spell_desc ) const
{
  unsigned class_idx = -1;
  unsigned spec_index = -1;

  if ( ! spec_idx( spec_id, class_idx, spec_index ) )
    return 0;

  assert( ( int )class_idx >= 0 && class_idx < specialization_max_class() && ( int )spec_index >= 0 && spec_index < specialization_max_per_class() );

  for ( const auto& spec_spell : specialization_spell_entry_t::data( ptr ) )
  {
    if ( spec_id != SPEC_NONE &&
         static_cast<unsigned>( spec_id ) != spec_spell.specialization_id )
    {
      continue;
    }

    // If description string is given, the spell must match it exactly
    if ( !spell_desc.empty() &&
         ( !spec_spell.desc || !util::str_compare_ci( spec_spell.desc, spell_desc ) ) )
    {
      continue;
    }

    // If the user requested no description but the specialization spell has a description
    // (essentially, a Rank string), skip it
    if ( spell_desc.empty() && spec_spell.desc )
    {
      continue;
    }

    if ( util::str_compare_ci( spec_spell.name, spell_name ) )
    {
      // Spell has been replaced by another, so don't return id
      if ( !replaced_id( spec_spell.spell_id ) )
      {
        return spec_spell.spell_id;
      }
      else
      {
        return 0;
      }
    }
  }

  return 0;
}

bool dbc_t::ability_specialization( uint32_t spell_id, std::vector<specialization_e>& spec_list ) const
{
  if ( !spell_id )
    return false;

  for ( const auto& spec_spell : specialization_spell_entry_t::data( ptr ) )
  {
    if ( spec_spell.spell_id == spell_id )
    {
      spec_list.push_back( static_cast<specialization_e>( spec_spell.specialization_id ) );
    }
  }

  return !spec_list.empty();
}

bool dbc_t::is_specialization_ability( specialization_e spec, unsigned spell_id ) const
{
  for ( const auto& spec_spell : specialization_spell_entry_t::data( ptr ) )
  {
    if ( spec != SPEC_NONE && spec_spell.specialization_id != spec )
    {
      continue;
    }

    if ( spec_spell.spell_id == spell_id )
    {
      return true;
    }
  }

  return false;
}

bool dbc_t::is_specialization_ability( uint32_t spell_id ) const
{
  return is_specialization_ability( SPEC_NONE, spell_id );
}

specialization_e dbc_t::spec_by_spell( uint32_t spell_id ) const
{
  for ( const auto& spec_spell : specialization_spell_entry_t::data( ptr ) )
  {
    if ( spec_spell.spell_id == spell_id )
    {
      return static_cast<specialization_e>( spec_spell.specialization_id );
    }
  }

  return SPEC_NONE;
}

unsigned dbc_t::replace_spell_id( unsigned spell_id ) const
{
  auto spec_spells = specialization_spell_entry_t::data( ptr );
  auto spec_entry = range::find( spec_spells, spell_id, &specialization_spell_entry_t::spell_id );
  if ( spec_entry != spec_spells.end() )
    return spec_entry -> override_spell_id;

  return 0;
}

double dbc_t::weapon_dps( const dbc_item_data_t& item_data, unsigned ilevel ) const
{
  assert( item_data.id > 0 );

  unsigned quality = item_data.quality;
  if ( quality > 6 )
    quality = 4; // Heirlooms default to epic values?

  unsigned ilvl = ilevel ? ilevel : item_data.level;

  switch ( item_data.inventory_type )
  {
    case INVTYPE_WEAPON:
    case INVTYPE_WEAPONMAINHAND:
    case INVTYPE_WEAPONOFFHAND:
    {
      if ( item_data.flags_2 & ITEM_FLAG2_CASTER_WEAPON )
        return item_damage_caster_1h( ilvl ).value( quality );
      else
        return item_damage_1h( ilvl ).value( quality );
      break;
    }
    case INVTYPE_2HWEAPON:
    {
      if ( item_data.flags_2 & ITEM_FLAG2_CASTER_WEAPON )
        return item_damage_caster_2h( ilvl ).value( quality );
      else
        return item_damage_2h( ilvl ).value( quality );
      break;
    }
    case INVTYPE_RANGED:
    case INVTYPE_THROWN:
    case INVTYPE_RANGEDRIGHT:
    {
      switch ( item_data.item_subclass )
      {
        // TODO: DBC ItemDamageRanged got removed
        case ITEM_SUBCLASS_WEAPON_BOW:
        case ITEM_SUBCLASS_WEAPON_GUN:
        case ITEM_SUBCLASS_WEAPON_CROSSBOW:
        {
          return item_damage_2h( ilvl ).value( quality );
        }
        // TODO: DBC ItemDamageThrown got removed
        case ITEM_SUBCLASS_WEAPON_THROWN:
        {
          return item_damage_1h( ilvl ).value( quality );
        }
        // TODO: DBC ItemDamageWand got removed
        case ITEM_SUBCLASS_WEAPON_WAND:
        {
          return item_damage_caster_1h( ilvl ).value( quality );
        }
        default: break;
      }
      break;
    }
    default: break;
  }

  return 0;
}

double dbc_t::weapon_dps( unsigned item_id, unsigned ilevel ) const
{
  const auto& item_data = item( item_id );

  if ( item_data.id == 0 ) return 0.0;

  return weapon_dps( item_data, ilevel );
}

bool dbc_t::spec_idx( specialization_e spec_id, uint32_t& class_idx, uint32_t& spec_index ) const
{
  if ( spec_id == SPEC_NONE )
    return false;

  for ( unsigned int i = 0; i < specialization_max_class(); i++ )
  {
    for ( unsigned int j = 0; j < specialization_max_per_class(); j++ )
    {
#if SC_USE_PTR
      auto __spec = ptr ? __ptr_class_spec_id[ i ][ j ] : __class_spec_id[ i ][ j ];
#else
      auto __spec = __class_spec_id[ i ][ j ];
#endif
      if ( __spec == spec_id )
      {
        class_idx = i;
        spec_index = j;
        return true;
      }
      if ( __spec == SPEC_NONE )
      {
        break;
      }
    }
  }
  return false;
}

specialization_e dbc_t::spec_by_idx( const player_e c, unsigned idx ) const
{
  unsigned cid = util::class_id( c );
#if SC_USE_PTR
  if ( !cid || cid >= static_cast<unsigned>( ptr ? PTR_MAX_SPEC_CLASS : MAX_SPEC_CLASS ) ||
       idx >= static_cast<unsigned>( ptr ? PTR_MAX_SPECS_PER_CLASS : MAX_SPECS_PER_CLASS ) )
  {
    return SPEC_NONE;
  }
  return ptr ? __ptr_class_spec_id[ cid ][ idx ] : __class_spec_id[ cid ][ idx ];
#else
  if ( !cid || cid >= MAX_SPEC_CLASS || idx >= MAX_SPECS_PER_CLASS )
  {
    return SPEC_NONE;
  }
  return __class_spec_id[ cid ][ idx ];
#endif
}

// DBC

util::span<const spell_data_t* const> dbc_t::spells_by_label( size_t label ) const
{
  return spell_label_index.affects_spells( as<unsigned>( label ), ptr );
}

util::span<const spell_data_t* const> dbc_t::spells_by_category( unsigned category ) const
{
  return spell_categories_index.affects_spells( category, ptr );
}

util::span<const spell_data_t* const> dbc_t::spells_by_category_mask_bit( unsigned bit ) const
{
  return spell_category_mask_index.affects_spells( bit, ptr );
}
