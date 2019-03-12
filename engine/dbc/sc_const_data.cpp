// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#include "data_definitions.hh"
#include "generated/sc_spec_list.inc"
#include "generated/sc_scale_data.inc"
#include "generated/sc_talent_data.inc"
#include "generated/sc_spell_data.inc"
#include "generated/sc_spell_lists.inc"
#include "sc_extra_data.inc"

#if SC_USE_PTR
#include "generated/sc_scale_data_ptr.inc"
#include "generated/sc_talent_data_ptr.inc"
#include "generated/sc_spell_data_ptr.inc"
#include "generated/sc_spell_lists_ptr.inc"
#include "sc_extra_data_ptr.inc"
#endif

namespace { // ANONYMOUS namespace ==========================================

dbc_index_t<spell_data_t> spell_data_index;
dbc_index_t<spelleffect_data_t> spelleffect_data_index;
dbc_index_t<talent_data_t> talent_data_index;
dbc_index_t<spellpower_data_t> power_data_index;

// Wrapper class to map other data to specific spells, and also to map effects that manipulate that
// data
template <typename T, typename V>
class spell_mapping_reference_t
{
  // Map struct T (based on id) to a set of spells
  std::unordered_map<V, std::vector<const spell_data_t*>> m_db[2];
  // Map struct T (based on id) to a set of effects affecting the group T
  std::unordered_map<V, std::vector<const spelleffect_data_t*>> m_effects_db[2];

  // Return the pertinent value being mapped from struct T
  using value_fn_t = std::function<V(const T*)>;
  // Return the spell the struct T is mapped to
  using spell_fn_t = std::function<unsigned(const T*)>;
  // Analyze effect data to determine whether it affects labels.
  using effect_fn_t = std::function<V(const spelleffect_data_t*)>;

  spell_fn_t spell_fn;
  value_fn_t value_fn;
  effect_fn_t effect_fn;

public:
  spell_mapping_reference_t( value_fn_t vfn, spell_fn_t sfn, effect_fn_t efn ) :
    spell_fn( sfn ), value_fn( vfn ), effect_fn( efn )
  { }

  // Initialize database based on an external data entity, value function will give the key for the
  // spell, spell function will give the identifier of the spell
  void init_db( bool ptr = false )
  {
    const T* data = T::list( ptr );
    while ( data -> id() != 0 )
    {
      V value = value_fn( data );
      const auto spell = spell_data_t::find( spell_fn( data ), ptr );
      if ( spell -> id() != spell_fn( data ) )
      {
        ++data;
        continue;
      }

      add_spell( value, spell, ptr );

      ++data;
    }
  }

  // Init database based on spell data itself, value function will give the key for the spell
  void init_db( const spell_data_t* data, bool ptr = false )
  {
    V value = value_fn( data );
    if ( value != 0 )
    {
      add_spell( value, data, ptr );
    }
  }

  void add_spell( V value, const spell_data_t* data, bool ptr = false )
  {
    m_db[ ptr ][ value ].push_back( data );
  }

  void init_effect_db( const spelleffect_data_t* effect, bool ptr = false )
  {
    auto value = effect_fn( effect );
    if ( value != 0 )
    {
      m_effects_db[ ptr ][ value ].push_back( effect );
    }
  }

  std::vector<const spell_data_t*> affects_spells( V value, bool ptr )
  {
    auto it = m_db[ ptr ].find( value );

    if ( it != m_db[ ptr ].end() )
    {
      return it -> second;
    }

    return std::vector<const spell_data_t*>();
  }

  std::vector<const spelleffect_data_t*> affected_by( V value, bool ptr )
  {
    std::vector<const spelleffect_data_t*> effects;
    auto it = m_effects_db[ ptr ].find( value );

    if ( it == m_effects_db[ ptr ].end() )
    {
      return effects;
    }

    range::for_each( m_effects_db[ ptr ][ value ], [ &effects ]( const spelleffect_data_t* e ) {
      effects.push_back( e );
    } );

    return effects;
  }
};

std::vector< std::vector< const spell_data_t* > > class_family_index;
std::vector< std::vector< const spell_data_t* > > ptr_class_family_index;

// Label -> spell mappings
spell_mapping_reference_t<spelllabel_data_t, short> spell_label_index(
  []( const spelllabel_data_t* data ) { return data -> label(); },
  []( const spelllabel_data_t* data ) { return data -> id_spell(); },
  []( const spelleffect_data_t* data ) {
    if ( data -> subtype() == A_ADD_PCT_LABEL_MODIFIER )
    {
      return as<short>( data -> misc_value2() );
    }

    return as<short>( 0 );
  }
);

// Categories -> spell mappings
spell_mapping_reference_t<spell_data_t, unsigned> spell_categories_index(
  []( const spell_data_t* spell ) { return spell -> category(); },
  []( const spell_data_t* spell ) { return spell -> id(); },
  []( const spelleffect_data_t* data ) {
    if ( data -> subtype() == A_HASTED_CATEGORY )
    {
      return as<unsigned>( data -> misc_value1() );
    }

    return 0U;
  }
);
struct class_passives_entry_t
  {
    player_e         type;
    specialization_e spec;
    unsigned         spell_id;
  };

const std::vector<class_passives_entry_t> _class_passives {
  { DEATH_KNIGHT, SPEC_NONE,              137005 },
  { DEATH_KNIGHT, DEATH_KNIGHT_BLOOD,     137008 },
  { DEATH_KNIGHT, DEATH_KNIGHT_UNHOLY,    137007 },
  { DEATH_KNIGHT, DEATH_KNIGHT_FROST,     137006 },
  { DEMON_HUNTER, SPEC_NONE,              212611 },
  { DEMON_HUNTER, DEMON_HUNTER_HAVOC,     212612 },
  { DEMON_HUNTER, DEMON_HUNTER_VENGEANCE, 212613 },
  { DRUID,        SPEC_NONE,              137009 },
  { DRUID,        DRUID_RESTORATION,      137012 },
  { DRUID,        DRUID_FERAL,            137011 },
  { DRUID,        DRUID_BALANCE,          137013 },
  { DRUID,        DRUID_GUARDIAN,         137010 },
  { HUNTER,       SPEC_NONE,              137014 },
  { HUNTER,       HUNTER_SURVIVAL,        137017 },
  { HUNTER,       HUNTER_MARKSMANSHIP,    137016 },
  { HUNTER,       HUNTER_BEAST_MASTERY,   137015 },
  { MAGE,         SPEC_NONE,              137018 },
  { MAGE,         MAGE_ARCANE,            137021 },
  { MAGE,         MAGE_FIRE,              137019 },
  { MAGE,         MAGE_FROST,             137020 },
  { MONK,         SPEC_NONE,              137022 },
  { MONK,         MONK_BREWMASTER,        137023 },
  { MONK,         MONK_MISTWEAVER,        137024 },
  { MONK,         MONK_WINDWALKER,        137025 },
  { PALADIN,      SPEC_NONE,              137026 },
  { PALADIN,      PALADIN_HOLY,           137029 },
  { PALADIN,      PALADIN_PROTECTION,     137028 },
  { PALADIN,      PALADIN_RETRIBUTION,    137027 },
  { PRIEST,       SPEC_NONE,              137030 },
  { PRIEST,       PRIEST_SHADOW,          137033 },
  { PRIEST,       PRIEST_HOLY,            137031 },
  { PRIEST,       PRIEST_DISCIPLINE,      137032 },
  { ROGUE,        SPEC_NONE,              137034 },
  { ROGUE,        ROGUE_OUTLAW,           137036 },
  { ROGUE,        ROGUE_SUBTLETY,         137035 },
  { ROGUE,        ROGUE_ASSASSINATION,    137037 },
  { SHAMAN,       SPEC_NONE,              137038 },
  { SHAMAN,       SHAMAN_ELEMENTAL,       137040 },
  { SHAMAN,       SHAMAN_ENHANCEMENT,     137041 },
  { SHAMAN,       SHAMAN_RESTORATION,     137039 },
  { WARLOCK,      SPEC_NONE,              137042 },
  { WARLOCK,      WARLOCK_AFFLICTION,     137043 },
  { WARLOCK,      WARLOCK_DEMONOLOGY,     137044 },
  { WARLOCK,      WARLOCK_DESTRUCTION,    137046 },
  { WARRIOR,      SPEC_NONE,              137047 },
  { WARRIOR,      WARRIOR_ARMS,           137049 },
  { WARRIOR,      WARRIOR_FURY,           137050 },
  { WARRIOR,      WARRIOR_PROTECTION,     137048 },
};

} // ANONYMOUS namespace ====================================================

int dbc::build_level( bool ptr )
{
  return maybe_ptr( ptr ) ? 29558 : 29683;
}

const char* dbc::wow_version( bool ptr )
{ return maybe_ptr( ptr ) ? "8.1.5" : "8.1.5"; }

const char* dbc::wow_ptr_status( bool ptr )
#if SC_BETA
{ (void)ptr; return SC_BETA_STR "-BETA"; }
#else
{ return ( maybe_ptr( ptr ) ? "PTR" : "Live" ); }
#endif

const item_set_bonus_t* dbc::set_bonus( bool ptr )
{
  ( void ) ptr;

  const item_set_bonus_t* p = __set_bonus_data;
#if SC_USE_PTR
  if ( ptr )
    p = __ptr_set_bonus_data;
#endif
  return p;
}

std::size_t dbc::n_set_bonus( bool ptr )
{
  ( void ) ptr;

  size_t n = SET_BONUS_DATA_SIZE;
#if SC_USE_PTR
  if ( ptr )
    n = PTR_SET_BONUS_DATA_SIZE;
#endif

  return n;
}

static void generate_class_flags_index( bool ptr = false )
{
  std::vector< std::vector< const spell_data_t* > >* l = &( class_family_index );
  if ( ptr )
    l = &( ptr_class_family_index );

  // Make a class family index to speed up some spell query parsing
  const spell_data_t* spell = spell_data_t::list( ptr );
  while ( spell -> id() != 0 )
  {
    if ( spell -> class_family() == 0 )
    {
      spell++;
      continue;
    }

    if ( l -> size() <= spell -> class_family() )
      l -> resize( spell -> class_family() + 1 );

    l -> at( spell -> class_family() ).push_back( spell );

    spell++;
  }
}

/* Initialize database
 */
void dbc::init()
{
  // Create id-indexes
  spell_data_index.init();
  spelleffect_data_index.init();
  talent_data_index.init();
  power_data_index.init();
  init_item_data();

  // runtime linking, eg. from spell_data to all its effects
  spell_data_t::link( false );
  spelleffect_data_t::link( false );
  spellpower_data_t::link( false );
  talent_data_t::link( false );

  if ( SC_USE_PTR )
  {
    spell_data_t::link( true );
    spelleffect_data_t::link( true );
    spellpower_data_t::link( true );
    talent_data_t::link( true );
  }

  // Link client side hotfix data to spells, effects, and power entries after everything else is
  // done
  hotfix::link_hotfix_data( false );
#if SC_USE_PTR
  hotfix::link_hotfix_data( true );
#endif

  generate_class_flags_index();
  spell_label_index.init_db();
  if ( SC_USE_PTR )
  {
    generate_class_flags_index( true );
    spell_label_index.init_db( true );
  }
}

/* De-Initialize database
 */
void dbc::de_init()
{
  spell_data_t::de_link( false );

  if ( SC_USE_PTR )
  {
    spell_data_t::de_link( true );
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

std::vector< const spell_data_t* > dbc_t::effect_affects_spells( unsigned family, const spelleffect_data_t* effect ) const
{
  std::vector< const spell_data_t* > affected_spells;

  if ( family == 0 )
    return affected_spells;

  if ( ptr && family >= ptr_class_family_index.size() )
    return affected_spells;
  else if ( family >= class_family_index.size() )
    return affected_spells;

  const std::vector< const spell_data_t* >* l = &( class_family_index[ family ] );
  if ( ptr )
    l = &( ptr_class_family_index[ family ] );

  for (auto s : *l)
  {
    
    for ( unsigned int j = 0, vend = NUM_CLASS_FAMILY_FLAGS * 32; j < vend; j++ )
    {
      if ( effect -> class_flag(j ) && s -> class_flag( j ) )
      {
        if ( std::find( affected_spells.begin(), affected_spells.end(), s ) == affected_spells.end() )
          affected_spells.push_back( s );
      }
    }
  }

  return affected_spells;
}

std::vector<const spelleffect_data_t*> dbc_t::effect_labels_affecting_spell( const spell_data_t* spell ) const
{
  auto labels = spell -> labels();
  std::vector<const spelleffect_data_t*> effects;

  range::for_each( labels, [ &effects, this ]( short label ) {
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

  if ( ptr && spell -> class_family() >= ptr_class_family_index.size() )
    return affecting_effects;
  else if ( spell -> class_family() >= class_family_index.size() )
    return affecting_effects;

  const std::vector< const spell_data_t* >* l = &( class_family_index[ spell -> class_family() ] );
  if ( ptr )
    l = &( ptr_class_family_index[ spell -> class_family() ] );

  for (auto s : *l)
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

// translate_spec_str =======================================================

specialization_e dbc::translate_spec_str( player_e ptype, const std::string& spec_str )
{
  using namespace util;
  switch ( ptype )
  {
    case DEATH_KNIGHT:
    {
      if ( str_compare_ci( spec_str, "blood" ) )
        return DEATH_KNIGHT_BLOOD;
      if ( str_compare_ci( spec_str, "tank" ) )
        return DEATH_KNIGHT_BLOOD;
      else if ( str_compare_ci( spec_str, "frost" ) )
        return DEATH_KNIGHT_FROST;
      else if ( str_compare_ci( spec_str, "unholy" ) )
        return DEATH_KNIGHT_UNHOLY;
      break;
    }
    case DEMON_HUNTER:
    {
      if ( str_compare_ci( spec_str, "havoc" ) )
        return DEMON_HUNTER_HAVOC;
      else if ( str_compare_ci( spec_str, "vengeance" ) )
        return DEMON_HUNTER_VENGEANCE;
      break;
    }
    case DRUID:
    {
      if ( str_compare_ci( spec_str, "balance" ) )
        return DRUID_BALANCE;
      else if ( str_compare_ci( spec_str, "caster" ) )
        return DRUID_BALANCE;
      else if ( str_compare_ci( spec_str, "laserchicken" ) )
        return DRUID_BALANCE;
      else if ( str_compare_ci( spec_str, "feral" ) )
        return DRUID_FERAL;
      else if ( str_compare_ci( spec_str, "feral_combat" ) )
        return DRUID_FERAL;
      else if ( str_compare_ci( spec_str, "cat" ) )
        return DRUID_FERAL;
      else if ( str_compare_ci( spec_str, "melee" ) )
        return DRUID_FERAL;
      else if ( str_compare_ci( spec_str, "guardian" ) )
        return DRUID_GUARDIAN;
      else if ( str_compare_ci( spec_str, "bear" ) )
        return DRUID_GUARDIAN;
      else if ( str_compare_ci( spec_str, "tank" ) )
        return DRUID_GUARDIAN;
      else if ( str_compare_ci( spec_str, "restoration" ) )
        return DRUID_RESTORATION;
      else if ( str_compare_ci( spec_str, "resto" ) )
        return DRUID_RESTORATION;
      else if ( str_compare_ci( spec_str, "healer" ) )
        return DRUID_RESTORATION;
      else if ( str_compare_ci( spec_str, "tree" ) )
        return DRUID_RESTORATION;

      break;
    }
    case HUNTER:
    {
      if ( str_compare_ci( spec_str, "beast_mastery" ) )
        return HUNTER_BEAST_MASTERY;
      if ( str_compare_ci( spec_str, "bm" ) )
        return HUNTER_BEAST_MASTERY;
      else if ( str_compare_ci( spec_str, "marksmanship" ) )
        return HUNTER_MARKSMANSHIP;
      else if ( str_compare_ci( spec_str, "mm" ) )
        return HUNTER_MARKSMANSHIP;
      else if ( str_compare_ci( spec_str, "survival" ) )
        return HUNTER_SURVIVAL;
      else if ( str_compare_ci( spec_str, "sv" ) )
        return HUNTER_SURVIVAL;
      break;
    }
    case MAGE:
    {
      if ( str_compare_ci( spec_str, "arcane" ) )
        return MAGE_ARCANE;
      else if ( str_compare_ci( spec_str, "fire" ) )
        return MAGE_FIRE;
      else if ( str_compare_ci( spec_str, "frost" ) )
        return MAGE_FROST;
      break;
    }
    case MONK:
    {
      if ( str_compare_ci( spec_str, "brewmaster" ) )
        return MONK_BREWMASTER;
      else if ( str_compare_ci( spec_str, "brm" ) )
        return MONK_BREWMASTER;
      else if ( str_compare_ci( spec_str, "tank" ) )
        return MONK_BREWMASTER;
      else if ( str_compare_ci( spec_str, "mistweaver" ) )
        return MONK_MISTWEAVER;
      else if ( str_compare_ci( spec_str, "mw" ) )
        return MONK_MISTWEAVER;
      else if ( str_compare_ci( spec_str, "healer" ) )
        return MONK_MISTWEAVER;
      else if ( str_compare_ci( spec_str, "windwalker" ) )
        return MONK_WINDWALKER;
      else if ( str_compare_ci( spec_str, "ww" ) )
        return MONK_WINDWALKER;
      else if ( str_compare_ci( spec_str, "dps" ) )
        return MONK_WINDWALKER;
      else if ( str_compare_ci( spec_str, "melee" ) )
        return MONK_WINDWALKER;
      break;
    }
    case PALADIN:
    {
      if ( str_compare_ci( spec_str, "holy" ) )
        return PALADIN_HOLY;
      if ( str_compare_ci( spec_str, "healer" ) )
        return PALADIN_HOLY;
      else if ( str_compare_ci( spec_str, "protection" ) )
        return PALADIN_PROTECTION;
      else if ( str_compare_ci( spec_str, "prot" ) )
        return PALADIN_PROTECTION;
      else if ( str_compare_ci( spec_str, "tank" ) )
        return PALADIN_PROTECTION;
      else if ( str_compare_ci( spec_str, "retribution" ) )
        return PALADIN_RETRIBUTION;
      else if ( str_compare_ci( spec_str, "ret" ) )
        return PALADIN_RETRIBUTION;
      else if ( str_compare_ci( spec_str, "dps" ) )
        return PALADIN_RETRIBUTION;
      else if ( str_compare_ci( spec_str, "melee" ) )
        return PALADIN_RETRIBUTION;
      break;
    }
    case PRIEST:
    {
      if ( str_compare_ci( spec_str, "discipline" ) )
        return PRIEST_DISCIPLINE;
      if ( str_compare_ci( spec_str, "disc" ) )
        return PRIEST_DISCIPLINE;
      else if ( str_compare_ci( spec_str, "holy" ) )
        return PRIEST_HOLY;
      else if ( str_compare_ci( spec_str, "shadow" ) )
        return PRIEST_SHADOW;
      else if ( str_compare_ci( spec_str, "caster" ) )
        return PRIEST_SHADOW;
      break;
    }
    case ROGUE:
    {
      if ( str_compare_ci( spec_str, "assassination" ) )
        return ROGUE_ASSASSINATION;
      else if ( str_compare_ci( spec_str, "ass" ) )
        return ROGUE_ASSASSINATION;
      else if ( str_compare_ci( spec_str, "mut" ) )
        return ROGUE_ASSASSINATION;
      else if ( str_compare_ci( spec_str, "outlaw" ) )
        return ROGUE_OUTLAW;
      else if ( str_compare_ci( spec_str, "pirate" ) )
        return ROGUE_OUTLAW;
      else if ( str_compare_ci( spec_str, "swashbuckler" ) )
        return ROGUE_OUTLAW;
      else if ( str_compare_ci( spec_str, "subtlety" ) )
        return ROGUE_SUBTLETY;
      else if ( str_compare_ci( spec_str, "sub" ) )
        return ROGUE_SUBTLETY;
      break;
    }
    case SHAMAN:
    {
      if ( str_compare_ci( spec_str, "elemental" ) )
        return SHAMAN_ELEMENTAL;
      if ( str_compare_ci( spec_str, "ele" ) )
        return SHAMAN_ELEMENTAL;
      if ( str_compare_ci( spec_str, "caster" ) )
        return SHAMAN_ELEMENTAL;
      else if ( str_compare_ci( spec_str, "enhancement" ) )
        return SHAMAN_ENHANCEMENT;
      else if ( str_compare_ci( spec_str, "enh" ) )
        return SHAMAN_ENHANCEMENT;
      else if ( str_compare_ci( spec_str, "melee" ) )
        return SHAMAN_ENHANCEMENT;
      else if ( str_compare_ci( spec_str, "restoration" ) )
        return SHAMAN_RESTORATION;
      else if ( str_compare_ci( spec_str, "resto" ) )
        return SHAMAN_RESTORATION;
      else if ( str_compare_ci( spec_str, "healer" ) )
        return SHAMAN_RESTORATION;
      break;
    }
    case WARLOCK:
    {
      if ( str_compare_ci( spec_str, "affliction" ) )
        return WARLOCK_AFFLICTION;
      if ( str_compare_ci( spec_str, "affl" ) )
        return WARLOCK_AFFLICTION;
      if ( str_compare_ci( spec_str, "aff" ) )
        return WARLOCK_AFFLICTION;
      else if ( str_compare_ci( spec_str, "demonology" ) )
        return WARLOCK_DEMONOLOGY;
      else if ( str_compare_ci( spec_str, "demo" ) )
        return WARLOCK_DEMONOLOGY;
      else if ( str_compare_ci( spec_str, "destruction" ) )
        return WARLOCK_DESTRUCTION;
      else if ( str_compare_ci( spec_str, "destro" ) )
        return WARLOCK_DESTRUCTION;
      break;
    }
    case WARRIOR:
    {
      if ( str_compare_ci( spec_str, "arms" ) )
        return WARRIOR_ARMS;
      else if ( str_compare_ci( spec_str, "fury" ) )
        return WARRIOR_FURY;
      else if ( str_compare_ci( spec_str, "protection" ) )
        return WARRIOR_PROTECTION;
      else if ( str_compare_ci( spec_str, "prot" ) )
        return WARRIOR_PROTECTION;
      else if ( str_compare_ci( spec_str, "tank" ) )
        return WARRIOR_PROTECTION;
      break;
    }
    default: break;
  }
  return SPEC_NONE;
}

// specialization_string ====================================================

std::string dbc::specialization_string( specialization_e spec )
{
  switch ( spec )
  {
    case WARRIOR_ARMS: return "arms";
    case WARRIOR_FURY: return "fury";
    case WARRIOR_PROTECTION: return "protection";
    case PALADIN_HOLY: return "holy";
    case PALADIN_PROTECTION: return "protection";
    case PALADIN_RETRIBUTION: return "retribution";
    case HUNTER_BEAST_MASTERY: return "beast_mastery";
    case HUNTER_MARKSMANSHIP: return "marksmanship";
    case HUNTER_SURVIVAL: return "survival";
    case ROGUE_ASSASSINATION: return "assassination";
    case ROGUE_OUTLAW: return "outlaw";
    case ROGUE_SUBTLETY: return "subtlety";
    case PRIEST_DISCIPLINE: return "discipline";
    case PRIEST_HOLY: return "holy";
    case PRIEST_SHADOW: return "shadow";
    case DEATH_KNIGHT_BLOOD: return "blood";
    case DEATH_KNIGHT_FROST: return "frost";
    case DEATH_KNIGHT_UNHOLY: return "unholy";
    case SHAMAN_ELEMENTAL: return "elemental";
    case SHAMAN_ENHANCEMENT: return "enhancement";
    case SHAMAN_RESTORATION: return "restoration";
    case MAGE_ARCANE: return "arcane";
    case MAGE_FIRE: return "fire";
    case MAGE_FROST: return "frost";
    case WARLOCK_AFFLICTION: return "affliction";
    case WARLOCK_DEMONOLOGY: return "demonology";
    case WARLOCK_DESTRUCTION: return "destruction";
    case MONK_BREWMASTER: return "brewmaster";
    case MONK_MISTWEAVER: return "mistweaver";
    case MONK_WINDWALKER: return "windwalker";
    case DRUID_BALANCE: return "balance";
    case DRUID_FERAL: return "feral";
    case DRUID_GUARDIAN: return "guardian";
    case DRUID_RESTORATION: return "restoration";
    case DEMON_HUNTER_HAVOC: return "havoc";
    case DEMON_HUNTER_VENGEANCE: return "vengeance";
    case PET_FEROCITY: return "ferocity";
    case PET_TENACITY: return "tenacity";
    case PET_CUNNING: return "cunning";
    default: return "unknown";
  }
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
        case A_MOD_RATING_FROM_STAT:
        case A_MOD_CASTING_SPEED_NOT_STACK: // Wrath of Air, note this can go > +-100, but only on NPC (and possibly item) abilities
        case A_MOD_SPELL_DAMAGE_OF_ATTACK_POWER:
        case A_MOD_SPELL_HEALING_OF_ATTACK_POWER:
        case A_MOD_SPELL_DAMAGE_OF_STAT_PERCENT:
        case A_MOD_SPELL_HEALING_OF_STAT_PERCENT:
        case A_MOD_DAMAGE_PERCENT_DONE:
        case A_MOD_DAMAGE_FROM_CASTER: // vendetta
        case A_MOD_ALL_CRIT_CHANCE:
        case A_MOD_EXPERTISE:
        case A_MOD_MANA_REGEN_INTERRUPT:  // Meditation
        case A_308: // Increase critical chance of something, Stormstrike, Mind Spike, Holy Word: Serenity
        case A_317: // Totemic Wrath, Flametongue Totem, Demonic Pact, etc ...
        case A_319: // Windfury Totem
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

unsigned dbc::specialization_max_per_class()
{
  return MAX_SPECS_PER_CLASS;
}

specialization_e dbc::spec_by_idx( const player_e c, unsigned idx )
{
  int cid = util::class_id( c );

  if ( ( cid <= 0 ) || ( cid >= static_cast<int>( MAX_SPEC_CLASS ) ) || ( idx >= MAX_SPECS_PER_CLASS ) )
  {
    return SPEC_NONE;
  }
  return __class_spec_id[ cid ][ idx ];
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
    case SCHOOL_CHROMATIC     : return 0x7c;
    case SCHOOL_MAGIC         : return 0x7e;
    case SCHOOL_CHAOS         : return 0x7f;
    default                   : return 0x00;
  }
}

school_e dbc::get_school_type( uint32_t school_mask )
{
  switch ( school_mask )
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
    case 0x7c: return SCHOOL_CHROMATIC;
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

/**
 * Return class/spec passive spell data.
 * To get class data, use SPEC_NONE
 */
const spell_data_t* dbc::get_class_passive( const player_t& p, specialization_e s )
{
  for ( const auto& entry :  _class_passives )
  {
    if ( entry.type != p.type ) continue;
    if ( s != SPEC_NONE && entry.spec != s ) continue;

    return p.find_spell( entry.spell_id );
  }
  return spell_data_t::not_found();
}

std::vector<const spell_data_t*> dbc::class_passives( const player_t* p )
{

  std::vector<const spell_data_t*> spells;

  range::for_each( _class_passives, [ &spells, p ]( const class_passives_entry_t& entry ) {

    if ( entry.type != p -> type ) return;
    if ( entry.spec != SPEC_NONE && entry.spec != p -> specialization() ) return;

    spells.push_back( p -> find_spell( entry.spell_id ) );
  } );

  return spells;
}

double dbc::item_level_squish( unsigned source_ilevel, bool ptr )
{
  if ( source_ilevel == 0 )
  {
    return 1;
  }

#if SC_USE_PTR == 1
  if ( ptr )
  {
    assert( sizeof_array( _ptr__item_level_squish ) >= source_ilevel );
    return _ptr__item_level_squish[ source_ilevel - 1 ];
  }
  else
  {
    assert( sizeof_array( __item_level_squish ) >= source_ilevel );
    return __item_level_squish[ source_ilevel - 1 ];
  }
#else
  ( void ) ptr;
  assert( sizeof_array( __item_level_squish ) >= source_ilevel );
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

stat_data_t& dbc_t::race_base( race_e r ) const
{
  uint32_t race_id = util::race_id( r );

  assert( race_id < race_ability_tree_size() );
#if SC_USE_PTR
  return ptr ? __ptr_gt_race_stats[ race_id ]
             : __gt_race_stats[ race_id ];
#else
  return __gt_race_stats[ race_id ];
#endif
}

stat_data_t& dbc_t::race_base( pet_e /* r */ ) const
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

  assert( class_id < dbc_t::class_max_size() + 7 && level > 0 && level <= MAX_SCALING_LEVEL );
#if SC_USE_PTR
  return ptr ? _ptr__spell_scaling[ class_id ][ level - 1 ]
             : __spell_scaling[ class_id ][ level - 1 ];
#else
  return __spell_scaling[ class_id ][ level - 1 ];
#endif
}

double dbc_t::melee_crit_scaling( player_e t, unsigned level ) const
{
  uint32_t class_id = util::class_id( t );
  ( void ) class_id; ( void ) level;

  assert( class_id < dbc_t::class_max_size() && level > 0 && level <= MAX_SCALING_LEVEL );
  /*
#if SC_USE_PTR
  return ptr ? __ptr_gt_chance_to_melee_crit[ class_id ][ level - 1 ]
             : __gt_chance_to_melee_crit[ class_id ][ level - 1 ];
#else
  return __gt_chance_to_melee_crit[ class_id ][ level - 1 ];
#endif
  */
  return 0;
}
 
double dbc_t::melee_crit_scaling( pet_e t, unsigned level ) const
{
  return melee_crit_scaling( util::pet_class_type( t ), level );
}
 
double dbc_t::spell_crit_scaling( player_e t, unsigned level ) const
{
  uint32_t class_id = util::class_id( t );
  ( void ) class_id; ( void ) level;

  assert( class_id < dbc_t::class_max_size() && level > 0 && level <= MAX_SCALING_LEVEL );
  /*
#if SC_USE_PTR
  return ptr ? __ptr_gt_chance_to_spell_crit[ class_id ][ level - 1 ]
             : __gt_chance_to_spell_crit[ class_id ][ level - 1 ];
#else
  return __gt_chance_to_spell_crit[ class_id ][ level - 1 ];
#endif
  */
  return 0;
}
 
double dbc_t::spell_crit_scaling( pet_e t, unsigned level ) const
{
  return spell_crit_scaling( util::pet_class_type( t ), level );
}

int dbc_t::resolve_item_scaling( unsigned level ) const
{
  assert( level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_item_scaling[level - 1] 
             : __gt_item_scaling[level - 1];
#else
  return __gt_item_scaling[ level - 1 ];
#endif
}

double dbc_t::resolve_level_scaling( unsigned level ) const
{
  assert( level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? _ptr_gt_resolve_dps_by_level[level - 1]
             : _gt_resolve_dps_by_level[level - 1];
#else
  return _gt_resolve_dps_by_level[level - 1];
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

double dbc_t::health_base( player_e t, unsigned level ) const
{
  uint32_t class_id = util::class_id( t );
  ( void ) class_id; ( void ) level;

  assert( class_id < MAX_CLASS && level > 0 && level <= MAX_SCALING_LEVEL );
  /*
#if SC_USE_PTR
  return ptr ? __ptr_gt_octbase_hpby_class[ class_id ][ level - 1 ]
             : __gt_octbase_hpby_class[ class_id ][ level - 1 ];
#else
  return __gt_octbase_hpby_class[ class_id ][ level - 1 ];
#endif
  */
  return 0;
}

double dbc_t::resource_base( player_e t, unsigned level ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < MAX_CLASS && level > 0 && level <= MAX_SCALING_LEVEL );
#if SC_USE_PTR
  return ptr ? _ptr__base_mp[ class_id ][ level - 1 ]
             : __base_mp[ class_id ][ level - 1 ];
#else
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


stat_data_t& dbc_t::attribute_base( player_e t, unsigned level ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < dbc_t::class_max_size() && level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_class_stats_by_level[ class_id ][ level - 1 ]
             : __gt_class_stats_by_level[ class_id ][ level - 1 ];
#else
  return __gt_class_stats_by_level[ class_id ][ level - 1 ];
#endif
}

stat_data_t& dbc_t::attribute_base( pet_e t, unsigned level ) const
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

unsigned dbc_t::class_ability( unsigned class_id, unsigned tree_id, unsigned n ) const
{
  assert( class_id < dbc_t::class_max_size() && tree_id < class_ability_tree_size() && n < class_ability_size() );

#if SC_USE_PTR
  return ptr ? __ptr_class_ability_data[ class_id ][ tree_id ][ n ]
             : __class_ability_data[ class_id ][ tree_id ][ n ];
#else
  return __class_ability_data[ class_id ][ tree_id ][ n ];
#endif
}

unsigned dbc_t::pet_ability( unsigned class_id, unsigned n ) const
{
  assert( class_id < dbc_t::class_max_size() && n < class_ability_size() );

#if SC_USE_PTR
  return ptr ? __ptr_class_ability_data[ class_id ][ class_ability_tree_size() - 1 ][ n ]
             : __class_ability_data[ class_id ][ class_ability_tree_size() - 1 ][ n ];
#else
  return __class_ability_data[ class_id ][ class_ability_tree_size() - 1 ][ n ];
#endif
}

unsigned dbc_t::race_ability( unsigned race_id, unsigned class_id, unsigned n ) const
{
  assert( race_id < race_ability_tree_size() && class_id < dbc_t::class_max_size() && n < race_ability_size() );

#if SC_USE_PTR
  return ptr ? __ptr_race_ability_data[ race_id ][ class_id ][ n ]
             : __race_ability_data[ race_id ][ class_id ][ n ];
#else
  return __race_ability_data[ race_id ][ class_id ][ n ];
#endif
}

unsigned dbc_t::specialization_ability( unsigned class_id, unsigned tree_id, unsigned n ) const
{
  assert( class_id < dbc_t::class_max_size() );
  assert( tree_id < specialization_max_per_class() );
  assert( n < specialization_ability_size() );

#if SC_USE_PTR
  return ptr ? __ptr_tree_specialization_data[ class_id ][ tree_id ][ n ]
             : __tree_specialization_data[ class_id ][ tree_id ][ n ];
#else
  return __tree_specialization_data[ class_id ][ tree_id ][ n ];
#endif
}

unsigned dbc_t::mastery_ability( unsigned class_id, unsigned specialization, unsigned n ) const
{
  assert( class_id < dbc_t::class_max_size() );
  assert( specialization < specialization_max_per_class() );
  assert( n < mastery_ability_size() );

#if SC_USE_PTR
  return ptr ? __ptr_class_mastery_ability_data[ class_id ][ specialization ][ n ]
             : __class_mastery_ability_data[ class_id ][ specialization ][ n ];
#else
  return __class_mastery_ability_data[ class_id ][ specialization ][ n ];
#endif
}

unsigned dbc_t::azerite_item_level( unsigned power_level ) const
{
  if ( power_level == 0 )
  {
    return 0;
  }

#if SC_USE_PTR
  auto arr = ptr ? _ptr__azerite_level_to_item_level
                 : __azerite_level_to_item_level;
#else
  auto arr = __azerite_level_to_item_level;
#endif

  if ( power_level > MAX_AZERITE_LEVEL )
  {
    return 0;
  }

  return arr[ power_level - 1 ];
}

const azerite_power_entry_t& dbc_t::azerite_power( unsigned power_id ) const
{
  return azerite_power_entry_t::find( power_id, ptr );
}

const azerite_power_entry_t& dbc_t::azerite_power( const std::string& name, bool tokenized ) const
{
  for ( const auto& power : azerite_powers() )
  {
    if ( tokenized )
    {
      std::string tokenized_name = power.name;
      util::tokenize( tokenized_name );
      if ( util::str_compare_ci( name, tokenized_name ) )
      {
        return power;
      }
    }
    else
    {
      if ( util::str_compare_ci( name, power.name ) )
      {
        return power;
      }
    }
  }

  return azerite_power_entry_t::nil();
}

arv::array_view<azerite_power_entry_t> dbc_t::azerite_powers() const
{ return azerite_power_entry_t::data( ptr ); }

unsigned dbc_t::class_max_size() const
{
  return MAX_CLASS;
}

unsigned dbc_t::class_ability_tree_size() const
{
#if SC_USE_PTR
  return ptr ? PTR_CLASS_ABILITY_TREE_SIZE : CLASS_ABILITY_TREE_SIZE;
#else
  return CLASS_ABILITY_TREE_SIZE;
#endif
}

unsigned dbc_t::class_ability_size() const
{
#if SC_USE_PTR
  return ptr ? PTR_CLASS_ABILITY_SIZE : CLASS_ABILITY_SIZE;
#else
  return CLASS_ABILITY_SIZE;
#endif
}

unsigned dbc_t::specialization_max_per_class() const
{
#if SC_USE_PTR
  return ptr ? MAX_SPECS_PER_CLASS : MAX_SPECS_PER_CLASS;
#else
  return MAX_SPECS_PER_CLASS;
#endif
}

unsigned dbc_t::specialization_max_class() const
{
#if SC_USE_PTR
  return ptr ? MAX_SPEC_CLASS : MAX_SPEC_CLASS;
#else
  return MAX_SPEC_CLASS;
#endif
}

unsigned dbc_t::race_ability_size() const
{
#if SC_USE_PTR
  return ptr ? PTR_RACE_ABILITY_SIZE : RACE_ABILITY_SIZE;
#else
  return RACE_ABILITY_SIZE;
#endif
}

unsigned dbc_t::race_ability_tree_size() const
{
#if SC_USE_PTR
  return ptr ? ptr_MAX_RACE : MAX_RACE;
#else
  return MAX_RACE;
#endif
}

unsigned dbc_t::specialization_ability_size() const
{
#if SC_USE_PTR
  return ptr ? PTR_TREE_SPECIALIZATION_SIZE : TREE_SPECIALIZATION_SIZE;
#else
  return TREE_SPECIALIZATION_SIZE;
#endif
}

unsigned dbc_t::mastery_ability_size() const
{
#if SC_USE_PTR
  return ptr ? PTR_CLASS_MASTERY_ABILITY_SIZE : CLASS_MASTERY_ABILITY_SIZE;
#else
  return CLASS_MASTERY_ABILITY_SIZE;
#endif
}

std::vector<const rppm_modifier_t*> dbc_t::real_ppm_modifiers( unsigned spell_id ) const
{
#if SC_USE_PTR
  const rppm_modifier_t* p = ptr ? __ptr_rppmmodifier_data : __rppmmodifier_data;
#else
  const rppm_modifier_t* p = __rppmmodifier_data;
#endif

  std::vector<const rppm_modifier_t*> data;

  while ( p -> spell_id != 0 )
  {
    if ( p -> spell_id == spell_id )
    {
      data.push_back( p );
    }
    p++;
  }

  return data;
}

unsigned dbc_t::real_ppm_scale( unsigned spell_id ) const
{
#if SC_USE_PTR
  const rppm_modifier_t* p = ptr ? __ptr_rppmmodifier_data : __rppmmodifier_data;
#else
  const rppm_modifier_t* p = __rppmmodifier_data;
#endif

  unsigned scale = 0;

  while ( p -> spell_id != 0 )
  {
    if ( p -> spell_id != spell_id )
    {
      p++;
      continue;
    }

    if ( p -> modifier_type == RPPM_MODIFIER_HASTE )
    {
      scale |= RPPM_HASTE;
    }
    else if ( p -> modifier_type == RPPM_MODIFIER_CRIT )
    {
      scale |= RPPM_CRIT;
    }


    p++;
  }

  return scale;
}

double dbc_t::real_ppm_modifier( unsigned spell_id, player_t* player, unsigned item_level ) const
{
#if SC_USE_PTR
  const rppm_modifier_t* p = ptr ? __ptr_rppmmodifier_data : __rppmmodifier_data;
#else
  const rppm_modifier_t* p = __rppmmodifier_data;
#endif

  double modifier = 1.0;

  while ( p -> spell_id != 0 )
  {
    if ( p -> spell_id != spell_id )
    {
      p++;
      continue;
    }

    if ( p -> modifier_type == RPPM_MODIFIER_SPEC &&
         player -> specialization() == static_cast<specialization_e>( p -> type ) )
    {
      modifier *= 1.0 + p -> coefficient;
    }
    // TODO: How does coefficient play into this?
    else if ( p -> modifier_type == RPPM_MODIFIER_ILEVEL )
    {
      assert( item_level > 0 && "Ilevel-based RPPM modifier requires non-zero item level parameter" );
      modifier *= item_database::approx_scale_coefficient( p -> type, item_level );
    }

    p++;
  }

  return modifier;
}

spell_data_t* spell_data_t::list( bool ptr )
{
  ( void )ptr;

#if SC_USE_PTR
  return ptr ? __ptr_spell_data : __spell_data;
#else
  return __spell_data;
#endif
}

spelleffect_data_t* spelleffect_data_t::list( bool ptr )
{
  ( void )ptr;

#if SC_USE_PTR
  return ptr ? __ptr_spelleffect_data : __spelleffect_data;
#else
  return __spelleffect_data;
#endif
}

spellpower_data_t* spellpower_data_t::list( bool ptr )
{
  ( void )ptr;

#if SC_USE_PTR
  return ptr ? __ptr_spellpower_data : __spellpower_data;
#else
  return __spellpower_data;
#endif
}

double spelleffect_data_t::scaled_average( double budget, unsigned level ) const
{
  if ( _m_coeff != 0 && _spell -> scaling_class() != 0 )
    return _m_coeff * budget;
  else if ( _real_ppl != 0 )
  {
    if ( _spell -> max_level() > 0 )
      return _base_value + ( std::min( level, _spell -> max_level() ) - _spell -> level() ) * _real_ppl;
    else
      return _base_value + ( level - _spell -> level() ) * _real_ppl;
  }
  else
    return _base_value;
}

double spelleffect_data_t::average( const player_t* p, unsigned level ) const
{
  assert( level <= MAX_SCALING_LEVEL );

  double m_scale = 0;

  if ( _m_coeff != 0 && _spell -> scaling_class() != 0 )
  {
    unsigned scaling_level = level ? level : p -> level();
    if ( _spell -> max_scaling_level() > 0 )
      scaling_level = std::min( scaling_level, _spell -> max_scaling_level() );
    m_scale = p -> dbc.spell_scaling( _spell -> scaling_class(), scaling_level );
  }

  return scaled_average( m_scale, level );
}

double spelleffect_data_t::average( const item_t* item ) const
{
  if ( ! item )
    return 0;

  auto budget = item_database::item_budget( item, _spell -> max_scaling_level() );
  if ( _spell -> scaling_class() == PLAYER_SPECIAL_SCALE7 )
  {
    budget = item_database::apply_combat_rating_multiplier( *item, budget );
  }
  else if ( _spell -> scaling_class() == PLAYER_SPECIAL_SCALE8 )
  {
    const auto& props = item -> player -> dbc.random_property( item -> item_level() );
    budget = props.damage_replace_stat;
  }

  return _m_coeff * budget;
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
#if SC_USE_PTR
  return ptr ? __ptr_armor_mitigation_constants_data[ level - 1 ]
             : __armor_mitigation_constants_data[ level - 1 ];
#else
  return __armor_mitigation_constants_data[ level - 1 ];
#endif
}

double dbc_t::npc_armor_value( unsigned level ) const
{
  assert( level > 0 && level <= ( MAX_SCALING_LEVEL + 3 ) );
#if SC_USE_PTR
  return ptr ? __ptr_npc_armor_data[ level - 1 ]
             : __npc_armor_data[ level - 1 ];
#else
  return __npc_armor_data[ level - 1 ];
#endif
}

double spelleffect_data_t::scaled_delta( double budget ) const
{
  if ( _m_delta != 0 && budget > 0 )
    return _m_coeff * _m_delta * budget;
  else
    return 0;
}

double spelleffect_data_t::delta( const player_t* p, unsigned level ) const
{
  assert( level <= MAX_SCALING_LEVEL );

  double m_scale = 0;
  if ( _m_delta != 0 && _spell -> scaling_class() != 0 )
  {
    unsigned scaling_level = level ? level : p -> level();
    if ( _spell -> max_scaling_level() > 0 )
      scaling_level = std::min( scaling_level, _spell -> max_scaling_level() );
    m_scale = p -> dbc.spell_scaling( _spell -> scaling_class(), scaling_level );
  }

  return scaled_delta( m_scale );
}

double spelleffect_data_t::delta( const item_t* item ) const
{
  double m_scale = 0;

  if ( ! item )
    return 0;

  if ( _m_delta != 0 )
    m_scale = item_database::item_budget( item, _spell -> max_scaling_level() );

  if ( _spell -> scaling_class() == PLAYER_SPECIAL_SCALE7 )
  {
    m_scale = item_database::apply_combat_rating_multiplier( *item, m_scale );
  }
  else if ( _spell -> scaling_class() == PLAYER_SPECIAL_SCALE8 )
  {
    const auto& props = item -> player -> dbc.random_property( item -> item_level() );
    m_scale = props.damage_replace_stat;
  }

  return scaled_delta( m_scale );
}

double spelleffect_data_t::bonus( const player_t* p, unsigned level ) const
{
  assert( p );
  return p -> dbc.effect_bonus( id(), level ? level : p -> level() );
}

double spelleffect_data_t::scaled_min( double avg, double delta ) const
{
  double result = 0;

  if ( _m_coeff != 0 || _m_delta != 0 )
    result = avg - ( delta / 2 );
  else
  {
    result = avg;
  }

  switch ( _type )
  {
    case E_WEAPON_PERCENT_DAMAGE:
      result *= 0.01;
      break;
    default:
      break;
  }

  return result;
}

double spelleffect_data_t::scaled_max( double avg, double delta ) const
{
  double result = 0;

  if ( _m_coeff != 0 || _m_delta != 0 )
    result = avg + ( delta / 2 );
  else
  {
    result = avg;
  }

  switch ( _type )
  {
    case E_WEAPON_PERCENT_DAMAGE:
      result *= 0.01;
      break;
    default:
      break;
  }

  return result;
}

double spelleffect_data_t::min( const player_t* p, unsigned level ) const
{
  assert( level <= MAX_SCALING_LEVEL );

  return scaled_min( average( p, level ), delta( p, level ) );
}

double spelleffect_data_t::min( const item_t* item ) const
{
  return scaled_min( average( item ), delta( item ) );
}

double spelleffect_data_t::max( const player_t* p, unsigned level ) const
{
  assert( level <= MAX_SCALING_LEVEL );

  return scaled_max( average( p, level ), delta( p, level ) );
}

double spelleffect_data_t::max( const item_t* item ) const
{
  return scaled_max( average( item ), delta( item ) );
}

talent_data_t* talent_data_t::list( bool ptr )
{
  ( void )ptr;

#if SC_USE_PTR
  return ptr ? __ptr_talent_data : __talent_data;
#else
  return __talent_data;
#endif
}

spell_data_t* spell_data_t::find( unsigned spell_id, bool ptr )
{
  spell_data_t* s = spell_data_index.get( ptr, spell_id );
  if ( !s )
    s = spell_data_t::nil();
  return s;
}

spell_data_t* spell_data_t::find( unsigned spell_id, const char* confirmation, bool ptr )
{
  ( void )confirmation;

  spell_data_t* p = find( spell_id, ptr );
  assert( p && ! strcmp( confirmation, p -> name_cstr() ) );
  return p;
}

spell_data_t* spell_data_t::find( const char* name, bool ptr )
{
  for ( spell_data_t* p = spell_data_t::list( ptr ); p -> name_cstr(); ++p )
  {
    if ( ! strcmp ( name, p -> name_cstr() ) )
    {
      return p;
    }
  }
  return nullptr;
}

// Always returns non-NULL
spelleffect_data_t* spelleffect_data_t::find( unsigned id, bool ptr )
{
  spelleffect_data_t* effect = spelleffect_data_index.get( ptr, id );
  if ( ! effect )
    effect = spelleffect_data_t::nil();
  return effect;
}

// Always returns non-NULL
spellpower_data_t* spellpower_data_t::find( unsigned id, bool ptr )
{
  spellpower_data_t* power = power_data_index.get( ptr, id );
  return power ? power : spellpower_data_t::nil();
}

talent_data_t* talent_data_t::find( player_e c, unsigned int row, unsigned int col, specialization_e spec, bool ptr )
{
  talent_data_t* talent_data = talent_data_t::list( ptr );

  for ( unsigned k = 0; talent_data[ k ].name_cstr(); k++ )
  {
    talent_data_t& td = talent_data[ k ];
    if ( td.is_class( c ) && ( row == td.row() ) && ( col == td.col() ) && spec == td.specialization() )
    {
      return &td;
    }
  }

  // Second round to check all talents, either with a none spec, or no spec check at all,
  // depending on what the caller gave in "spec" parameter
  for ( unsigned k = 0; talent_data[ k ].name_cstr(); k++ )
  {
    talent_data_t& td = talent_data[ k ];
    if ( spec != SPEC_NONE )
    {
      if ( td.is_class( c ) && ( row == td.row() ) && ( col == td.col() ) && td.specialization() == SPEC_NONE )
        return &td;
    }
    else
    {
      if ( td.is_class( c ) && ( row == td.row() ) && ( col == td.col() ) )
        return &td;
    }
  }

  return nullptr;
}

talent_data_t* talent_data_t::find( unsigned id, bool ptr )
{
  talent_data_t* t = talent_data_index.get( ptr, id );
  if ( ! t )
    t = talent_data_t::nil();
  return t;
}

talent_data_t* talent_data_t::find( unsigned id, const char* confirmation, bool ptr )
{

  talent_data_t* p = find( id, ptr );
  assert( !confirmation || (strcmp( confirmation, p -> name_cstr() ) == 0) );
  ( void )confirmation;
  return p;
}

talent_data_t* talent_data_t::find( const char* name_cstr, specialization_e spec, bool ptr )
{
  for ( talent_data_t* p = talent_data_t::list( ptr ); p -> name_cstr(); ++p )
  {
    if ( ! strcmp( name_cstr, p -> name_cstr() ) && p -> specialization() == spec )
    {
      return p;
    }
  }

  return nullptr;
}

talent_data_t* talent_data_t::find_tokenized( const char* name, specialization_e spec, bool ptr )
{
  for ( talent_data_t* p = talent_data_t::list( ptr ); p -> name_cstr(); ++p )
  {
    std::string tokenized_name = p -> name_cstr();
    util::tokenize( tokenized_name );
    if ( util::str_compare_ci( name, tokenized_name ) && p -> specialization() == spec )
      return p;
  }

  return nullptr;
}

void spell_data_t::link( bool ptr )
{
  spell_data_t* spell_data = spell_data_t::list( ptr );

  for ( int i = 0; spell_data[ i ].id(); i++ )
  {
    spell_data_t& sd = spell_data[ i ];
    sd._effects = new std::vector<const spelleffect_data_t*>;

    spell_categories_index.init_db( &( sd ), ptr );
  }

  auto label = spelllabel_data_t::list( ptr );
  while ( label -> id() )
  {
    auto spell = spell_data_t::find( label -> id_spell(), ptr );
    if ( spell -> _labels == nullptr )
    {
      spell -> _labels = new std::vector<const spelllabel_data_t*>();
    }

    spell -> _labels -> push_back( label );

    ++label;
  }
}

void spelleffect_data_t::link( bool ptr )
{
  spelleffect_data_t* spelleffect_data = spelleffect_data_t::list( ptr );

  for ( int i = 0; spelleffect_data[ i ].id(); i++ )
  {
    spelleffect_data_t& ed = spelleffect_data[ i ];

    ed._spell         = spell_data_t::find( ed.spell_id(), ptr );
    ed._trigger_spell = spell_data_t::find( ed.trigger_spell_id(), ptr );
    if ( ed._trigger_spell -> id() > 0 )
    {
      if ( ! ed._trigger_spell -> _driver )
      {
        ed._trigger_spell -> _driver = new std::vector<spell_data_t*>;
      }
      if ( range::find( *ed._trigger_spell -> _driver, ed._spell ) == ed._trigger_spell -> _driver -> end() )
      {
        ed._trigger_spell -> _driver -> push_back( ed._spell );
      }
    }

    if ( ed._spell -> _effects -> size() < ( ed.index() + 1 ) )
      ed._spell -> _effects -> resize( ed.index() + 1, spelleffect_data_t::nil() );

    ed._spell -> _effects -> at( ed.index() ) = &ed;

    // Some effects are going to be affecting labels, so map spells here
    spell_label_index.init_effect_db( &( ed ), ptr );

    // Some effects are going to be affecting categories, so map spells here
    spell_categories_index.init_effect_db( &( ed ), ptr );
  }
}

void spell_data_t::de_link( bool ptr )
{
  spell_data_t* spell_data = spell_data_t::list( ptr );

  for ( int i = 0; spell_data[ i ].id(); i++ )
  {
    spell_data_t& sd = spell_data[ i ];

    delete sd._effects;
    delete sd._power;
    delete sd._driver;
    delete sd._labels;
  }
}

void spellpower_data_t::link( bool ptr )
{
  spellpower_data_t* spellpower_data = spellpower_data_t::list( ptr );

  for ( int i = 0; spellpower_data[ i ]._id; i++ )
  {
    spellpower_data_t& pd = spellpower_data[ i ];
    spell_data_t*      sd = spell_data_t::find( pd._spell_id, ptr );

    if ( sd -> _power == nullptr )
      sd -> _power = new std::vector<const spellpower_data_t*>;

    sd -> _power -> push_back( &pd );
  }
}

void talent_data_t::link( bool ptr )
{
  talent_data_t* talent_data = talent_data_t::list( ptr );

  for ( int i = 0; talent_data[ i ].name_cstr(); i++ )
  {
    talent_data_t& td = talent_data[ i ];
    td.spell1 = spell_data_t::find( td.spell_id(), ptr );
  }
}

/* Generic helper methods */

double dbc_t::effect_average( unsigned effect_id, unsigned level ) const
{
  return effect_average( effect( effect_id ), level );
}

double dbc_t::effect_average( const spelleffect_data_t* e, unsigned level ) const
{
  assert( e && ( level > 0 ) && ( level <= MAX_SCALING_LEVEL ) );

  if ( e -> m_coefficient() != 0 && e -> spell() -> scaling_class() != 0 )
  {
    unsigned scaling_level = level;
    if ( e -> spell() -> max_scaling_level() > 0 )
      scaling_level = std::min( scaling_level, e -> spell() -> max_scaling_level() );
    double m_scale = spell_scaling( e -> spell() -> scaling_class(), scaling_level );

    return e -> m_coefficient() * m_scale;
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
  assert( e && ( level > 0 ) && ( level <= MAX_SCALING_LEVEL ) );

  if ( e -> m_delta() != 0 && e -> spell() -> scaling_class() != 0 )
  {
    return  e -> m_delta();
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

  if ( e -> m_unk() != 0 && e -> spell() -> scaling_class() != 0 )
  {
    unsigned scaling_level = level;
    if ( e -> spell() -> max_scaling_level() > 0 )
      scaling_level = std::min( scaling_level, e -> spell() -> max_scaling_level() );
    double m_scale = spell_scaling( e -> spell() -> scaling_class(), scaling_level );

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

  double avg, result;

  assert( e && ( level > 0 ) );
  assert( ( level <= MAX_SCALING_LEVEL ) );

  unsigned c_id = util::class_id( e -> _spell -> scaling_class() );
  avg = effect_average( e, level );

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
  double avg, result;

  assert( e && ( level > 0 ) && ( level <= MAX_SCALING_LEVEL ) );

  unsigned c_id = util::class_id( e -> _spell -> scaling_class() );
  avg = effect_average( e, level );

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

unsigned dbc_t::talent_ability_id( player_e c, specialization_e spec, const char* spell_name, bool name_tokenized ) const
{
  uint32_t cid = util::class_id( c );

  assert( spell_name && spell_name[ 0 ] );

  if ( ! cid )
    return 0;

  talent_data_t* t;
  if ( name_tokenized )
  {
    t = talent_data_t::find_tokenized( spell_name, spec, ptr );
    if ( ! t )
      t = talent_data_t::find_tokenized( spell_name, SPEC_NONE, ptr );
  }
  else
  {
    t = talent_data_t::find( spell_name, spec, ptr ); // first try finding with the given spec
    if ( !t )
      t = talent_data_t::find( spell_name, SPEC_NONE, ptr ); // now with SPEC_NONE
  }

  if ( t && t -> is_class( c ) && ! replaced_id( t -> spell_id() ) )
  {
    return t -> spell_id();
  }

  return 0;
}

unsigned dbc_t::class_ability_id( player_e c, specialization_e spec_id, const char* spell_name ) const
{
  uint32_t cid = util::class_id( c );
  unsigned spell_id;

  assert( spell_name && spell_name[ 0 ] );

  if ( ! cid )
    return 0;

  if ( spec_id != SPEC_NONE )
  {
    unsigned class_idx = -1;
    unsigned spec_index = -1;

    if ( ! spec_idx( spec_id, class_idx, spec_index ) )
      return 0;

    if ( class_idx == 0 ) // pet
    {
      spec_index = class_ability_size() - 1;
    }
    else if ( class_idx != cid )
    {
      return 0;
    }
    else
    {
      spec_index++;
    }

    // Now test spec based class abilities.
    for ( unsigned n = 0; n < class_ability_size(); n++ )
    {
      if ( ! ( spell_id = class_ability( cid, spec_index, n ) ) )
        break;

      if ( ! spell( spell_id ) -> id() )
        continue;

      if ( util::str_compare_ci( spell( spell_id ) -> name_cstr(), spell_name ) )
      {
        // Spell has been replaced by another, so don't return id
        if ( ! replaced_id( spell_id ) )
        {
          return spell_id;
        }
        else
        {
          return 0;
        }
      }
    }
  }

  // Test general spells
  for ( unsigned n = 0; n < class_ability_size(); n++ )
  {
    if ( ! ( spell_id = class_ability( cid, 0, n ) ) )
      break;

    if ( ! spell( spell_id ) -> id() )
      continue;

    if ( util::str_compare_ci( spell( spell_id ) -> name_cstr(), spell_name ) )
    {
      // Spell has been replaced by another, so don't return id
      if ( ! replaced_id( spell_id ) )
      {
        return spell_id;
      }
      else
      {
        return 0;
      }
    }
  }

  return 0;
}


unsigned dbc_t::pet_ability_id( player_e c, const char* spell_name ) const
{
  uint32_t cid = util::class_id( c );

  assert( spell_name && spell_name[ 0 ] );

  if ( ! cid )
    return 0;

  for ( unsigned n = 0; n < class_ability_size(); n++ )
  {
    unsigned spell_id;
    if ( ! ( spell_id = pet_ability( cid, n ) ) )
      break;

    if ( ! spell( spell_id ) -> id() )
      continue;

    if ( util::str_compare_ci( spell( spell_id ) -> name_cstr(), spell_name ) )
    {
      // Spell has been replaced by another, so don't return id
      if ( ! replaced_id( spell_id ) )
      {
        return spell_id;
      }
      else
      {
        return 0;
      }
    }
  }

  return 0;
}

unsigned dbc_t::race_ability_id( player_e c, race_e r, const char* spell_name ) const
{
  unsigned rid = util::race_id( r );
  unsigned cid = util::class_id( c );
  unsigned spell_id;

  assert( spell_name && spell_name[ 0 ] );

  if ( !rid || !cid )
    return 0;

  // First check for class specific racials
  for ( unsigned n = 0; n < race_ability_size(); n++ )
  {
    if ( ! ( spell_id = race_ability( rid, cid, n ) ) )
      break;

    if ( ! spell( spell_id ) -> id() )
      continue;

    if ( util::str_compare_ci( spell( spell_id ) -> name_cstr(), spell_name ) )
    {
      // Spell has been replaced by another, so don't return id
      if ( ! replaced_id( spell_id ) )
        return spell_id;
    }
  }

  // Then check for for generic racials
  for ( unsigned n = 0; n < race_ability_size(); n++ )
  {
    if ( ! ( spell_id = race_ability( rid, 0, n ) ) )
      break;

    if ( ! spell( spell_id ) -> id() )
      continue;

    if ( util::str_compare_ci( spell( spell_id ) -> name_cstr(), spell_name ) )
    {
      // Spell has been replaced by another, so don't return id
      if ( ! replaced_id( spell_id ) )
      {
        return spell_id;
      }
      else
      {
        return 0;
      }
    }
  }

  return 0;
}

unsigned dbc_t::specialization_ability_id( specialization_e spec_id, const char* spell_name ) const
{
  unsigned class_idx = -1;
  unsigned spec_index = -1;

  assert( spell_name && spell_name[ 0 ] );

  if ( ! spec_idx( spec_id, class_idx, spec_index ) )
    return 0;

  assert( ( int )class_idx >= 0 && class_idx < specialization_max_class() && ( int )spec_index >= 0 && spec_index < specialization_max_per_class() );

  for ( unsigned n = 0; n < specialization_ability_size(); n++ )
  {
    unsigned spell_id;
    if ( ! ( spell_id = specialization_ability( class_idx, spec_index, n ) ) )
      break;

    if ( ! spell( spell_id ) -> id() )
      continue;

    if ( util::str_compare_ci( spell( spell_id ) -> name_cstr(), spell_name ) )
    {
      // Spell has been replaced by another, so don't return id
      if ( ! replaced_id( spell_id ) )
      {
        return spell_id;
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
  unsigned s = 0;

  if ( ! spell_id )
    return false;

  for ( unsigned class_idx = 0; class_idx < specialization_max_class(); class_idx++ )
  {
    for ( unsigned spec_index = 0; spec_index < specialization_max_per_class(); spec_index++ )
    {
      for ( unsigned n = 0; n < specialization_ability_size(); n++ )
      {
        if ( ( s = specialization_ability( class_idx, spec_index, n ) ) == spell_id )
        {
          spec_list.push_back( __class_spec_id[ class_idx ][ spec_index ] );
        }
        if ( ! s )
          break;
      }
    }
  }

  return ! spec_list.empty();
}

unsigned dbc_t::mastery_ability_id( specialization_e spec, const char* spell_name ) const
{
  unsigned class_idx = -1;
  unsigned spec_index = -1;

  assert( spell_name && spell_name[ 0 ] );

  if ( ! spec_idx( spec, class_idx, spec_index ) )
    return 0;

  if ( spec == SPEC_NONE )
    return 0;

  for ( unsigned n = 0; n < mastery_ability_size(); n++ )
  {
    uint32_t spell_id;
    if ( ! ( spell_id = mastery_ability( class_idx, spec_index, n ) ) )
      break;

    if ( ! spell( spell_id ) -> id() )
      continue;

    if ( util::str_compare_ci( spell( spell_id ) -> name_cstr(), spell_name ) )
    {
      // Spell has been replaced by another, so don't return id
      if ( ! replaced_id( spell_id ) )
        return spell_id;
      return 0;
    }
  }

  return 0;
}

unsigned dbc_t::mastery_ability_id( specialization_e spec, uint32_t idx ) const
{
  unsigned class_idx = -1;
  unsigned spec_index = -1;
  uint32_t spell_id;

  if ( ! spec_idx( spec, class_idx, spec_index ) )
    return 0;

  if ( spec == SPEC_NONE )
    return 0;

  if ( idx >= mastery_ability_size() )
    return 0;

  spell_id = mastery_ability( class_idx, spec_index, idx );

  if ( ! spell( spell_id ) -> id() )
    return 0;

  // Check if spell has been replaced by another
  if ( ! replaced_id( spell_id ) )
    return spell_id;

  return 0;
}

int dbc_t::mastery_ability_tree( player_e c, uint32_t spell_id ) const
{
  uint32_t cid = util::class_id( c );

  for ( unsigned tree = 0; tree < specialization_max_per_class(); tree++ )
  {
    for ( unsigned n = 0; n < mastery_ability_size(); n++ )
    {
      if ( mastery_ability( cid, tree, n ) == spell_id )
        return tree;
    }
  }

  return -1;
}

bool dbc_t::is_specialization_ability( specialization_e spec, unsigned spell_id ) const
{
  unsigned class_idx = -1;
  unsigned spec_idx_ = -1;

  if ( spec == SPEC_NONE )
    return false;

  if ( ! spec_idx( spec, class_idx, spec_idx_ ) )
    return false;

  for ( unsigned n = 0; n < specialization_ability_size(); n++ )
  {
    if ( specialization_ability( class_idx, spec_idx_, n ) == spell_id )
    {
      return true;
    }
  }

  return false;
}

bool dbc_t::is_specialization_ability( uint32_t spell_id ) const
{
  for ( unsigned cls = 0; cls < dbc_t::class_max_size(); cls++ )
  {
    for ( unsigned tree = 0; tree < specialization_max_per_class(); tree++ )
    {
      for ( unsigned n = 0; n < specialization_ability_size(); n++ )
      {
        if ( specialization_ability( cls, tree, n ) == spell_id )
          return true;
      }
    }
  }

  return false;
}

specialization_e dbc_t::spec_by_spell( uint32_t spell_id ) const
{
  for ( unsigned cls = 0; cls < dbc_t::class_max_size(); cls++ )
  {
    for ( unsigned tree = 0; tree < specialization_max_per_class(); tree++ )
    {
      for ( unsigned n = 0; n < specialization_ability_size(); n++ )
      {
        if ( specialization_ability( cls, tree, n ) == spell_id )
          return spec_by_idx( util::translate_class_id( cls ), tree );
      }
    }
  }

  return SPEC_NONE;
}

double dbc_t::weapon_dps( const item_data_t* item_data, unsigned ilevel ) const
{
  assert( item_data );

  unsigned quality = item_data -> quality;
  if ( quality > 6 )
    quality = 4; // Heirlooms default to epic values?

  unsigned ilvl = ilevel ? ilevel : item_data -> level;

  switch ( item_data -> inventory_type )
  {
    case INVTYPE_WEAPON:
    case INVTYPE_WEAPONMAINHAND:
    case INVTYPE_WEAPONOFFHAND:
    {
      if ( item_data -> flags_2 & ITEM_FLAG2_CASTER_WEAPON )
        return item_damage_caster_1h( ilvl ).values[ quality ];
      else
        return item_damage_1h( ilvl ).values[ quality ];
      break;
    }
    case INVTYPE_2HWEAPON:
    {
      if ( item_data -> flags_2 & ITEM_FLAG2_CASTER_WEAPON )
        return item_damage_caster_2h( ilvl ).values[ quality ];
      else
        return item_damage_2h( ilvl ).values[ quality ];
      break;
    }
    case INVTYPE_RANGED:
    case INVTYPE_THROWN:
    case INVTYPE_RANGEDRIGHT:
    {
      switch ( item_data -> item_subclass )
      {
        // TODO: DBC ItemDamageRanged got removed
        case ITEM_SUBCLASS_WEAPON_BOW:
        case ITEM_SUBCLASS_WEAPON_GUN:
        case ITEM_SUBCLASS_WEAPON_CROSSBOW:
        {
          return item_damage_2h( ilvl ).values[ quality ];
        }
        // TODO: DBC ItemDamageThrown got removed
        case ITEM_SUBCLASS_WEAPON_THROWN:
        {
          return item_damage_1h( ilvl ).values[ quality ];
        }
        // TODO: DBC ItemDamageWand got removed
        case ITEM_SUBCLASS_WEAPON_WAND:
        {
          return item_damage_caster_1h( ilvl ).values[ quality ];
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
  const item_data_t* item_data = item( item_id );

  if ( ! item_data ) return 0.0;

  return weapon_dps( item_data, ilevel );
}

bool dbc_t::spec_idx( specialization_e spec_id, uint32_t& class_idx, uint32_t& spec_index ) const
{
  if ( spec_id == SPEC_NONE )
    return 0;

  for ( unsigned int i = 0; i < specialization_max_class(); i++ )
  {
    for ( unsigned int j = 0; j < specialization_max_per_class(); j++ )
    {
      if ( __class_spec_id[ i ][ j ] == spec_id )
      {
        class_idx = i;
        spec_index = j;
        return 1;
      }
      if ( __class_spec_id[ i ][ j ] == SPEC_NONE )
      {
        break;
      }
    }
  }
  return 0;
}

specialization_e dbc_t::spec_by_idx( const player_e c, unsigned idx ) const
{ return dbc::spec_by_idx( c, idx ); }

// DBC

bool spell_data_t::affected_by( const spell_data_t* spell ) const
{
  if ( class_family() != spell -> class_family() )
    return false;

  for ( size_t flag_idx = 0; flag_idx < NUM_CLASS_FAMILY_FLAGS; flag_idx++ )
  {
    if ( ! class_flags( (int)flag_idx ) )
      continue;

    for ( size_t effect_idx = 1, end = spell -> effect_count(); effect_idx <= end; effect_idx++ )
    {
      const spelleffect_data_t& effect = spell -> effectN( effect_idx );
      if ( ! effect.id() )
        continue;

      if ( class_flags( (int)flag_idx ) & effect.class_flags( (int)flag_idx ) )
        return true;
    }
  }

  return false;
}

bool spell_data_t::affected_by( const spelleffect_data_t* effect ) const
{
  if ( class_family() != effect -> spell() -> class_family() )
    return false;

  for ( size_t flag_idx = 0; flag_idx < NUM_CLASS_FAMILY_FLAGS; flag_idx++ )
  {
    if ( ! class_flags( (int)flag_idx ) )
      continue;

    if ( class_flags( (int)flag_idx ) & effect -> class_flags( (int)flag_idx ) )
      return true;
  }

  return false;
}

bool spell_data_t::affected_by( const spelleffect_data_t& effect ) const
{ return affected_by( &effect ); }

std::vector<const spell_data_t*> dbc_t::spells_by_label( size_t label ) const
{
  return spell_label_index.affects_spells( as<unsigned>( label ), ptr );
}

std::vector<const spell_data_t*> dbc_t::spells_by_category( unsigned category ) const
{
  return spell_categories_index.affects_spells( category, ptr );
}

// Hotfix data handling


size_t hotfix::n_spell_hotfix_entry( bool ptr )
{
#if SC_USE_PTR
  return ptr ? PTR_SPELL_HOTFIX_SIZE : SPELL_HOTFIX_SIZE;
#else
  ( void ) ptr;
  return SPELL_HOTFIX_SIZE;
#endif
}

size_t hotfix::n_effect_hotfix_entry( bool ptr )
{
#if SC_USE_PTR
  return ptr ? PTR_EFFECT_HOTFIX_SIZE : EFFECT_HOTFIX_SIZE;
#else
  ( void ) ptr;
  return EFFECT_HOTFIX_SIZE;
#endif
}

size_t hotfix::n_power_hotfix_entry( bool ptr )
{
#if SC_USE_PTR
  return ptr ? PTR_POWER_HOTFIX_SIZE : POWER_HOTFIX_SIZE;
#else
  ( void ) ptr;
  return POWER_HOTFIX_SIZE;
#endif
}

const hotfix::client_hotfix_entry_t* hotfix::spell_hotfix_entry( bool ptr )
{
#if SC_USE_PTR
  return ptr ? __ptr_spell_hotfix_data : __spell_hotfix_data;
#else
  (void ) ptr;
  return __spell_hotfix_data;
#endif
}

const hotfix::client_hotfix_entry_t* hotfix::effect_hotfix_entry( bool ptr )
{
#if SC_USE_PTR
  return ptr ? __ptr_effect_hotfix_data : __effect_hotfix_data;
#else
  (void ) ptr;
  return __effect_hotfix_data;
#endif
}

const hotfix::client_hotfix_entry_t* hotfix::power_hotfix_entry( bool ptr )
{
#if SC_USE_PTR
  return ptr ? __ptr_power_hotfix_data : __power_hotfix_data;
#else
  (void ) ptr;
  return __power_hotfix_data;
#endif
}

// Link up the hotfix entry pointer to each relevant spell, effect, or power data.
template <typename DATA>
static void link_hotfix_entry_ptr( bool ptr, const std::function<const hotfix::client_hotfix_entry_t*(bool)>& entry_fn )
{
  auto entry = entry_fn( ptr );
  unsigned last_id = entry -> id;
  if ( last_id == 0 )
  {
    return;
  }

  DATA* data = DATA::find( entry -> id, ptr );
  while ( entry -> id != 0 )
  {
    if ( entry -> id != last_id )
    {
      data = DATA::find( entry -> id, ptr );
      last_id = entry -> id;
    }

    if ( ! data || data -> _hotfix_entry )
    {
      ++entry;
      continue;
    }

    data -> _hotfix_entry = entry;
    ++entry;
  }
}

void hotfix::link_hotfix_data( bool ptr )
{
  // First up, link spell hotfix data
  link_hotfix_entry_ptr<spell_data_t>( ptr, spell_hotfix_entry );

  // Next, link effect hotfix data
  link_hotfix_entry_ptr<spelleffect_data_t>( ptr, effect_hotfix_entry );

  // Finally, link power hotfix data
  link_hotfix_entry_ptr<spellpower_data_t>( ptr, power_hotfix_entry );
}

const spelllabel_data_t* spelllabel_data_t::list( bool ptr )
{
#if SC_USE_PTR
  return ptr ? __ptr_spelllabel_data
             : __spelllabel_data;
#else
  (void ) ptr;
  return __spelllabel_data;
#endif
}

