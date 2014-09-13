// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#include "sc_spec_list.inc"
#include "sc_scale_data.inc"
#include "sc_talent_data.inc"
#include "sc_spell_data.inc"
#include "sc_spell_lists.inc"
#include "sc_extra_data.inc"
#include "sc_item_data.inc"

#if SC_USE_PTR
#include "sc_scale_data_ptr.inc"
#include "sc_talent_data_ptr.inc"
#include "sc_spell_data_ptr.inc"
#include "sc_spell_lists_ptr.inc"
#include "sc_extra_data_ptr.inc"
#include "sc_item_data_ptr.inc"
#endif

namespace { // ANONYMOUS namespace ==========================================

// Global spell token map
class spelltoken_t
{
private:
  typedef std::unordered_map<unsigned int, std::string> token_map_t;
  token_map_t map;
  mutex_t mutex;

public:
  const std::string& get( unsigned int id_spell )
  {
    static const std::string empty;

    auto_lock_t lock( mutex );

    token_map_t::iterator it = map.find( id_spell );
    if ( it == map.end() )
      return empty;

    return it -> second;
  }

  unsigned int get_id( const std::string& token )
  {
    auto_lock_t lock( mutex );

    for ( token_map_t::iterator it = map.begin(); it != map.end(); ++it )
      if ( it -> second == token ) return it -> first;

    return 0;
  }

  bool add( unsigned int id_spell, const std::string& token_name )
  {
    auto_lock_t lock( mutex );

    std::pair<token_map_t::iterator, bool> pr =
      map.insert( std::make_pair( id_spell, token_name ) );

    // New entry
    if ( pr.second )
      return true;

    // Already exists with that token
    if ( pr.first -> second == token_name )
      return true;

    // Trying to specify a new token for an existing spell id
    return false;
  }
};
spelltoken_t tokens;

random_suffix_data_t nil_rsd;
item_enchantment_data_t nil_ied;
gem_property_data_t nil_gpd;
item_upgrade_t nil_iu;
item_upgrade_rule_t nil_iur;

// ==========================================================================
// Indices to provide log time, constant space access to spells, effects, and talents by id.
// ==========================================================================

/* id_function_policy and id_member_policy are here to give a standard interface
 * of accessing the id of a data type.
 * Eg. spell_data_t on which the id_function_policy is used has a function 'id()' which returns its id
 * and item_data_t on which id_member_policy is used has a member 'id' which stores its id.
 */
struct id_function_policy
{
  template <typename T> static unsigned id( const T& t )
  { return static_cast<unsigned>( t.id() ); }
};

struct id_member_policy
{
  template <typename T> static unsigned id( const T& t )
  { return static_cast<unsigned>( t.id ); }
};

template <typename T, typename KeyPolicy = id_function_policy>
class dbc_index_t
{
private:
  typedef std::pair<T*, T*> index_t; // first = lowest data; second = highest data
// array of size 1 or 2, depending on whether we have PTR data
#if SC_USE_PTR == 0
  index_t idx[ 1 ];
#else
  index_t idx[ 2 ];
#endif

  /* populate idx with pointer to lowest and highest data from a given list
   */
  void populate( index_t& idx, T* list )
  {
    assert( list );
    idx.first = list;
    for ( unsigned last_id = 0; KeyPolicy::id( *list ); last_id = KeyPolicy::id( *list ), ++list )
    {
      // Validate the input range is in fact sorted by id.
      assert( KeyPolicy::id( *list ) > last_id ); ( void )last_id;
    }
    idx.second = list;
  }

  struct id_compare
  {
    bool operator () ( const T& t, unsigned int id ) const
    { return KeyPolicy::id( t ) < id; }
    bool operator () ( unsigned int id, const T& t ) const
    { return id < KeyPolicy::id( t ); }
    bool operator () ( const T& l, const T& r ) const
    { return KeyPolicy::id( l ) < KeyPolicy::id( r ); }
  };

public:
  // Initialize index from given list
  void init( T* list, bool ptr )
  {
    assert( ! initialized( maybe_ptr( ptr ) ) );
    populate( idx[ maybe_ptr( ptr ) ], list );
  }

  // Initialize index under the assumption that 'T::list( bool ptr )' returns a list of data
  void init()
  {
    init( T::list( false ), false );
    if ( SC_USE_PTR )
      init( T::list( true ), true );
  }

  bool initialized( bool ptr = false ) const
  { return idx[ maybe_ptr( ptr ) ].first != 0; }

  // Return the item with the given id, or NULL.
  // Always returns non-NULL.
  T* get( bool ptr, unsigned id ) const
  {
    assert( initialized( maybe_ptr( ptr ) ) );
    T* p = std::lower_bound( idx[ maybe_ptr( ptr ) ].first, idx[ maybe_ptr( ptr ) ].second, id, id_compare() );
    if ( p != idx[ maybe_ptr( ptr ) ].second && KeyPolicy::id( *p ) == id )
      return p;
    else
      return NULL;
  }
};

dbc_index_t<spell_data_t> spell_data_index;
dbc_index_t<spelleffect_data_t> spelleffect_data_index;
dbc_index_t<talent_data_t> talent_data_index;
dbc_index_t<item_data_t, id_member_policy> item_data_index;
dbc_index_t<item_enchantment_data_t, id_member_policy> item_enchantment_data_index;

std::vector< std::vector< const spell_data_t* > > class_family_index;
std::vector< std::vector< const spell_data_t* > > ptr_class_family_index;

} // ANONYMOUS namespace ====================================================

int dbc::build_level( bool ptr )
{ return maybe_ptr( ptr ) ? 17345 : 18865; }

const char* dbc::wow_version( bool ptr )
{ return maybe_ptr( ptr ) ? "5.4.0" : "6.0.2"; }

const char* dbc::wow_ptr_status( bool ptr )
{ return ( maybe_ptr( ptr ) ?
#if SC_BETA
                     "BETA"
#else
                     "PTR"
#endif
                     : "Live" ); }

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

const item_data_t* dbc::items( bool ptr )
{
  ( void )ptr;

  const item_data_t* p = __item_data;
#if SC_USE_PTR
  if ( ptr )
    p = __ptr_item_data;
#endif
  return p;
}

size_t dbc::n_items( bool ptr )
{
  ( void )ptr;

  size_t n = ITEM_SIZE;
#if SC_USE_PTR
  if ( ptr )
    n = PTR_ITEM_SIZE;
#endif

  return n;
}

const item_enchantment_data_t* dbc::item_enchantments( bool ptr )
{
  ( void )ptr;

  const item_enchantment_data_t* p = __spell_item_ench_data;
#if SC_USE_PTR
  if ( ptr )
    p = __ptr_spell_item_ench_data;
#endif
  return p;
}

size_t dbc::n_item_enchantments( bool ptr )
{
  ( void )ptr;

  size_t n = SPELL_ITEM_ENCH_SIZE;
#if SC_USE_PTR
  if ( ptr )
    n = PTR_SPELL_ITEM_ENCH_SIZE;
#endif

  return n;
}

const gem_property_data_t* dbc::gem_properties( bool ptr )
{
  ( void )ptr;

  const gem_property_data_t* p = __gem_property_data;
#if SC_USE_PTR
  if ( ptr )
    p = __ptr_gem_property_data;
#endif
  return p;
}

/* Here we modify the spell data to match in-game values if the data differs thanks to bugs or hotfixes.
 *
 */
void dbc::apply_hotfixes()
{
  /* Spell Data Hotfix Area
   *
   * ALWAYS DOCUMENT YOUR CHANGES, including a DATE and a LINK to the source
   *
   * ALWAYS USE ABSOLUTE VALUES, never relative ones. That means only assignment operators ( = ),
   * no multiplication, addition, etc.
   */

  spell_data_t* s;

  s = spell_data_t::find( 137595, false );
  
  assert( s -> internal_cooldown() != timespan_t::from_seconds( 0.25 ) && "Capacitive Primal Diamond" );
  if ( s && s -> ok() )
    s -> _internal_cooldown = 250;
  
  // Hunter, hotfixes from 2013-09-23

  // Mage

  // Paladin
  // Build Last Checked: 16309
  // Description: Seal of Truth should be replacing Seal of Command but is missing its ReplaceId value
  s = spell_data_t::find( 31801, false );
  if ( s && s -> ok() && s -> effectN( 1 ).ok() )
  {
    const_cast<spell_data_t&>( *s )._replace_spell_id = 105361;
  }
  // For some reason, the proc chance listed for Divine Purpose in the DBC is now 100%.
  // Hotfix it to the value used on the tooltip.
  // FIXME: Use effectN( 1 ).percent() as proc chance instead ...............
  s = spell_data_t::find( 86172, false );
  s -> _proc_chance = s -> effectN( 1 ).base_value();
  if ( SC_USE_PTR )
  {
    s = spell_data_t::find( 86172, true );
    s -> _proc_chance = s -> effectN( 1 ).base_value();
  }

  // Priest

  // Rogue

  // Shaman

  // Warlock

  // Warrior
  s = spell_data_t::find( 96103, false );
  assert( s -> _spell_level == 39 && "Check level on Raging Blow" );
  s -> _spell_level = 30;
  if ( SC_USE_PTR )
  {
    s = spell_data_t::find( 96103, true );
    s -> _spell_level = 30;
  }

  s = spell_data_t::find( 85384, false );
  assert( s -> _spell_level == 39 && "Check level on Raging Blow Off-Hand" );
  s -> _spell_level = 30;
  if ( SC_USE_PTR )
  {
    s = spell_data_t::find( 85384, true );
    s -> _spell_level = 30;
  }

  // Druid
  s = spell_data_t::find( 50288, false ); // Starfall probably doesn't take 30 seconds to hit the target.
  assert( s -> _prj_speed == 0.8 && "Check the speed on Starfall" );
  s -> _prj_speed = 20;
  if ( SC_USE_PTR )
  {
    s = spell_data_t::find( 50288, true );
    s -> _prj_speed = 20;
  }

  // Death Knight

  // Misc

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
  item_data_index.init( __item_data, false );
  item_enchantment_data_index.init( __spell_item_ench_data, false );

#if SC_USE_PTR
  item_data_index.init( __ptr_item_data, true );
  item_enchantment_data_index.init( __ptr_spell_item_ench_data, true );
#endif

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

  // Apply "modifications" to dbc data
  dbc::apply_hotfixes();

  generate_class_flags_index();
  if ( SC_USE_PTR )
    generate_class_flags_index( true );
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
      return true;
    default:
      return false;
  }
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

  for ( size_t i = 0, end = l -> size(); i < end; i++ )
  {
    const spell_data_t* s = l -> at( i );
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

  for ( size_t i = 0, end = l -> size(); i < end; i++ )
  {
    const spell_data_t* s = l -> at( i );

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
      if ( str_compare_ci( spec_str, "tank" ) )
        return MONK_BREWMASTER;
      else if ( str_compare_ci( spec_str, "mistweaver" ) )
        return MONK_MISTWEAVER;
      else if ( str_compare_ci( spec_str, "healer" ) )
        return MONK_MISTWEAVER;
      else if ( str_compare_ci( spec_str, "windwalker" ) )
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
      if ( str_compare_ci( spec_str, "ass" ) )
        return ROGUE_ASSASSINATION;
      if ( str_compare_ci( spec_str, "mut" ) )
        return ROGUE_ASSASSINATION;
      else if ( str_compare_ci( spec_str, "combat" ) )
        return ROGUE_COMBAT;
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
    case ROGUE_COMBAT: return "combat";
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

const std::string& dbc::get_token( unsigned int id_spell )
{
  return tokens.get( id_spell );
}


bool dbc::add_token( unsigned int id_spell, const std::string& token, bool ptr )
{
  spell_data_t* sp = spell_data_index.get( ptr, id_spell );
  if ( sp && sp -> ok() )
  {
    if ( ! token.empty() )
    {
      return tokens.add( id_spell, token );
    }
    else
    {
      std::string t = sp -> name_cstr();
      util::tokenize( t );
      return tokens.add( id_spell, t );
    }
  }
  return false;
}

unsigned int dbc::get_token_id( const std::string& token )
{
  return tokens.get_id( token );
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
    case SCHOOL_SPELLSTORM    : return 0x48;
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
    case 0x48: return SCHOOL_SPELLSTORM;
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

uint32_t dbc_t::replaced_id( uint32_t id_spell ) const
{
  id_map_t::const_iterator it = replaced_ids.find( id_spell );
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

double dbc_t::melee_crit_base( player_e t, unsigned level ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < dbc_t::class_max_size() && level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_chance_to_melee_crit_base[ class_id ][ level - 1 ]
    : __gt_chance_to_melee_crit_base[ class_id ][ level - 1 ];
#else
  return __gt_chance_to_melee_crit_base[ class_id ][ level - 1 ];
#endif
}

double dbc_t::melee_crit_base( pet_e t, unsigned level ) const
{
  return melee_crit_base( util::pet_class_type( t ), level );
}

double dbc_t::spell_crit_base( player_e t, unsigned level ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < dbc_t::class_max_size() && level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_chance_to_spell_crit_base[ class_id ][ level - 1 ]
             : __gt_chance_to_spell_crit_base[ class_id ][ level - 1 ];
#else
  return __gt_chance_to_spell_crit_base[ class_id ][ level - 1 ];
#endif
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

  assert( class_id < dbc_t::class_max_size() + 5 && level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_spell_scaling[ class_id ][ level - 1 ]
             : __gt_spell_scaling[ class_id ][ level - 1 ];
#else
  return __gt_spell_scaling[ class_id ][ level - 1 ];
#endif
}

double dbc_t::melee_crit_scaling( player_e t, unsigned level ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < dbc_t::class_max_size() && level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_chance_to_melee_crit[ class_id ][ level - 1 ]
             : __gt_chance_to_melee_crit[ class_id ][ level - 1 ];
#else
  return __gt_chance_to_melee_crit[ class_id ][ level - 1 ];
#endif
}
 
double dbc_t::melee_crit_scaling( pet_e t, unsigned level ) const
{
  return melee_crit_scaling( util::pet_class_type( t ), level );
}
 
double dbc_t::spell_crit_scaling( player_e t, unsigned level ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < dbc_t::class_max_size() && level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_chance_to_spell_crit[ class_id ][ level - 1 ]
             : __gt_chance_to_spell_crit[ class_id ][ level - 1 ];
#else
  return __gt_chance_to_spell_crit[ class_id ][ level - 1 ];
#endif
}
 
double dbc_t::spell_crit_scaling( pet_e t, unsigned level ) const
{
  return spell_crit_scaling( util::pet_class_type( t ), level );
}

double dbc_t::regen_base( player_e t, unsigned level ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < dbc_t::class_max_size() && level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_base_mp5[ class_id ][ level - 1 ]
             : __gt_base_mp5[ class_id ][ level - 1 ];
#else
  return __gt_base_mp5[ class_id ][ level - 1 ];
#endif
}

double dbc_t::regen_base( pet_e t, unsigned level ) const
{
  return regen_base( util::pet_class_type( t ), level );
}

int dbc_t::resolve_item_scaling( unsigned level ) const
{
  assert( level > 0 && level <= MAX_LEVEL );
  return __gt_item_scaling[ level - 1 ];
}

double dbc_t::resolve_level_scaling( unsigned level ) const
{
  assert( level > 0 && level <= MAX_LEVEL );
  return _gt_resolve_dps_by_level[ level - 1 ];
}

double dbc_t::health_base( player_e t, unsigned level ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < MAX_CLASS && level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_octbase_hpby_class[ class_id ][ level - 1 ]
             : __gt_octbase_hpby_class[ class_id ][ level - 1 ];
#else
  return __gt_octbase_hpby_class[ class_id ][ level - 1 ];
#endif
}

double dbc_t::resource_base( player_e t, unsigned level ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < MAX_CLASS && level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_octbase_mpby_class[ class_id ][ level - 1 ]
             : __gt_octbase_mpby_class[ class_id ][ level - 1 ];
#else
  return __gt_octbase_mpby_class[ class_id ][ level - 1 ];
#endif
}

/* Mana regen per spirit
 */
double dbc_t::regen_spirit( player_e t, unsigned level ) const
{
  uint32_t class_id = util::class_id( t );

  assert( class_id < dbc_t::class_max_size() );
  assert( level > 0 );
  assert( level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_regen_mpper_spt[ class_id ][ level - 1 ]
             : __gt_regen_mpper_spt[ class_id ][ level - 1 ];
#else
  return __gt_regen_mpper_spt[ class_id ][ level - 1 ];
#endif
}

double dbc_t::regen_spirit( pet_e t, unsigned level ) const
{
  return regen_spirit( util::pet_class_type( t ), level );
}

double dbc_t::health_per_stamina( unsigned level ) const
{
  assert( level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_octhp_per_stamina[ level - 1 ]
             : __gt_octhp_per_stamina[ level - 1 ];
#else
  return __gt_octhp_per_stamina[ level - 1 ];
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
  ;
  assert( combat_rating_id < RATING_MAX );
  assert( level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_combat_ratings[ combat_rating_id ][ level - 1 ] * 100.0
             : __gt_combat_ratings[ combat_rating_id ][ level - 1 ] * 100.0;
#else
  return __gt_combat_ratings[ combat_rating_id ][ level - 1 ] * 100.0 ;
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

unsigned dbc_t::perk_ability( unsigned class_id, unsigned tree_id, unsigned n ) const
{
  assert( class_id < dbc_t::class_max_size() );
  assert( tree_id < specialization_max_per_class() );
  assert( n < perk_ability_size() );

#if SC_USE_PTR
  return ptr ? __ptr_perk_data[ class_id ][ tree_id ][ n ]
             : __perk_data[ class_id ][ tree_id ][ n ];
#else
  return __perk_data[ class_id ][ tree_id ][ n ];
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

unsigned dbc_t::glyph_spell( unsigned class_id, unsigned glyph_e, unsigned n ) const
{
  assert( class_id < dbc_t::class_max_size() && glyph_e < GLYPH_MAX && n < glyph_spell_size() );
#if SC_USE_PTR
  return ptr ? __ptr_glyph_abilities_data[ class_id ][ glyph_e ][ n ]
             : __glyph_abilities_data[ class_id ][ glyph_e ][ n ];
#else
  return __glyph_abilities_data[ class_id ][ glyph_e ][ n ];
#endif
}

unsigned dbc_t::set_bonus_spell( unsigned class_id, unsigned tier, unsigned n ) const
{
  assert( class_id < dbc_t::class_max_size() && tier < dbc_t::num_tiers() && n < set_bonus_spell_size() );
#if SC_USE_PTR
  return ptr ? __ptr_tier_bonuses_data[ class_id ][ tier ][ n ]
             : __tier_bonuses_data[ class_id ][ tier ][ n ];
#else
  return __tier_bonuses_data[ class_id ][ tier ][ n ];
#endif
}

unsigned dbc_t::class_max_size() const
{
  return MAX_CLASS;
}

unsigned dbc_t::num_tiers() const
{
#if SC_USE_PTR
  return ptr ? PTR_TIER_BONUSES_MAX_TIER : TIER_BONUSES_MAX_TIER;
#else
  return TIER_BONUSES_MAX_TIER;
#endif
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
  return MAX_RACE;
}

unsigned dbc_t::specialization_ability_size() const
{
#if SC_USE_PTR
  return ptr ? PTR_TREE_SPECIALIZATION_SIZE : TREE_SPECIALIZATION_SIZE;
#else
  return TREE_SPECIALIZATION_SIZE;
#endif
}

unsigned dbc_t::perk_ability_size() const
{
#if SC_USE_PTR
  return ptr ? PTR_PERK_SIZE : PERK_SIZE;
#else
  return PERK_SIZE;
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

unsigned dbc_t::glyph_spell_size() const
{
#if SC_USE_PTR
  return ptr ? PTR_GLYPH_ABILITIES_SIZE : GLYPH_ABILITIES_SIZE;
#else
  return GLYPH_ABILITIES_SIZE;
#endif
}

unsigned dbc_t::set_bonus_spell_size() const
{
#if SC_USE_PTR
  return ptr ? PTR_TIER_BONUSES_SIZE : TIER_BONUSES_SIZE;
#else
  return TIER_BONUSES_SIZE;
#endif
}

int dbc_t::random_property_max_level() const
{
  size_t n = RAND_PROP_POINTS_SIZE;
#if SC_USE_PTR
  if ( ptr )
    n = PTR_RAND_PROP_POINTS_SIZE;
#endif
  assert( n > 0 );
  return as<int>( n );
}

const random_prop_data_t& dbc_t::random_property( unsigned ilevel ) const
{
#if SC_USE_PTR
  assert( ilevel > 0 && ( ( ptr && ilevel <= PTR_RAND_PROP_POINTS_SIZE ) || ( ilevel <= RAND_PROP_POINTS_SIZE ) ) );
  return ptr ? __ptr_rand_prop_points_data[ ilevel - 1 ] : __rand_prop_points_data[ ilevel - 1 ];
#else
  assert( ilevel > 0 && ( ilevel <= RAND_PROP_POINTS_SIZE ) );
  return __rand_prop_points_data[ ilevel - 1 ];
#endif
}

double dbc_t::item_socket_cost( unsigned ilevel ) const
{
  assert( ilevel > 0 && ( ilevel <= RAND_PROP_POINTS_SIZE ) );
#if SC_USE_PTR
  return ptr ? __ptr_gt_item_socket_cost_per_level[ ilevel - 1 ]
             : __gt_item_socket_cost_per_level[ ilevel - 1 ];
#else
  return __gt_item_socket_cost_per_level[ ilevel - 1 ];
#endif
}

const item_scale_data_t& dbc_t::item_damage_1h( unsigned ilevel ) const
{
#if SC_USE_PTR
  assert( ilevel > 0 && ( ( ptr && ilevel <= PTR_RAND_PROP_POINTS_SIZE ) || ( ilevel <= RAND_PROP_POINTS_SIZE ) ) );
  return ptr ? __ptr_itemdamageonehand_data[ ilevel - 1 ] : __itemdamageonehand_data[ ilevel - 1 ];
#else
  assert( ilevel > 0 && ( ilevel <= RAND_PROP_POINTS_SIZE ) );
  return __itemdamageonehand_data[ ilevel - 1 ];
#endif
}

const item_scale_data_t& dbc_t::item_damage_2h( unsigned ilevel ) const
{
#if SC_USE_PTR
  assert( ilevel > 0 && ( ( ptr && ilevel <= PTR_RAND_PROP_POINTS_SIZE ) || ( ilevel <= RAND_PROP_POINTS_SIZE ) ) );
  return ptr ? __ptr_itemdamagetwohand_data[ ilevel - 1 ] : __itemdamagetwohand_data[ ilevel - 1 ];
#else
  assert( ilevel > 0 && ( ilevel <= RAND_PROP_POINTS_SIZE ) );
  return __itemdamagetwohand_data[ ilevel - 1 ];
#endif
}

const item_scale_data_t& dbc_t::item_damage_caster_1h( unsigned ilevel ) const
{
#if SC_USE_PTR
  assert( ilevel > 0 && ( ( ptr && ilevel <= PTR_RAND_PROP_POINTS_SIZE ) || ( ilevel <= RAND_PROP_POINTS_SIZE ) ) );
  return ptr ? __ptr_itemdamageonehandcaster_data[ ilevel - 1 ] : __itemdamageonehandcaster_data[ ilevel - 1 ];
#else
  assert( ilevel > 0 && ( ilevel <= RAND_PROP_POINTS_SIZE ) );
  return __itemdamageonehandcaster_data[ ilevel - 1 ];
#endif
}

const item_scale_data_t& dbc_t::item_damage_caster_2h( unsigned ilevel ) const
{
#if SC_USE_PTR
  assert( ilevel > 0 && ( ( ptr && ilevel <= PTR_RAND_PROP_POINTS_SIZE ) || ( ilevel <= RAND_PROP_POINTS_SIZE ) ) );
  return ptr ? __ptr_itemdamagetwohandcaster_data[ ilevel - 1 ] : __itemdamagetwohandcaster_data[ ilevel - 1 ];
#else
  assert( ilevel > 0 && ( ilevel <= RAND_PROP_POINTS_SIZE ) );
  return __itemdamagetwohandcaster_data[ ilevel - 1 ];
#endif
}

const item_scale_data_t& dbc_t::item_damage_ranged( unsigned ilevel ) const
{
#if SC_USE_PTR
  assert( ilevel > 0 && ( ( ptr && ilevel <= PTR_RAND_PROP_POINTS_SIZE ) || ( ilevel <= RAND_PROP_POINTS_SIZE ) ) );
  return ptr ? __ptr_itemdamageranged_data[ ilevel - 1 ] : __itemdamageranged_data[ ilevel - 1 ];
#else
  return __itemdamageranged_data[ ilevel - 1 ];
#endif
}

const item_scale_data_t& dbc_t::item_damage_thrown( unsigned ilevel ) const
{
#if SC_USE_PTR
  assert( ilevel > 0 && ( ( ptr && ilevel <= PTR_RAND_PROP_POINTS_SIZE ) || ( ilevel <= RAND_PROP_POINTS_SIZE ) ) );
  return ptr ? __ptr_itemdamagethrown_data[ ilevel - 1 ] : __itemdamagethrown_data[ ilevel - 1 ];
#else
  assert( ilevel > 0 && ( ilevel <= RAND_PROP_POINTS_SIZE ) );
  return __itemdamagethrown_data[ ilevel - 1 ];
#endif
}

const item_scale_data_t& dbc_t::item_damage_wand( unsigned ilevel ) const
{
#if SC_USE_PTR
  assert( ilevel > 0 && ( ( ptr && ilevel <= PTR_RAND_PROP_POINTS_SIZE ) || ( ilevel <= RAND_PROP_POINTS_SIZE ) ) );
  return ptr ? __ptr_itemdamagewand_data[ ilevel - 1 ] : __itemdamagewand_data[ ilevel - 1 ];
#else
  assert( ilevel > 0 && ( ilevel <= RAND_PROP_POINTS_SIZE ) );
  return __itemdamagewand_data[ ilevel - 1 ];
#endif
}

const item_scale_data_t& dbc_t::item_armor_quality( unsigned ilevel ) const
{
#if SC_USE_PTR
  assert( ilevel > 0 && ( ( ptr && ilevel <= PTR_RAND_PROP_POINTS_SIZE ) || ( ilevel <= RAND_PROP_POINTS_SIZE ) ) );
  return ptr ? __ptr_itemarmorquality_data[ ilevel - 1 ] : __itemarmorquality_data[ ilevel - 1 ];
#else
  assert( ilevel > 0 && ( ilevel <= RAND_PROP_POINTS_SIZE ) );
  return __itemarmorquality_data[ ilevel - 1 ];
#endif
}

const item_scale_data_t& dbc_t::item_armor_shield( unsigned ilevel ) const
{
#if SC_USE_PTR
  assert( ilevel > 0 && ( ( ptr && ilevel <= PTR_RAND_PROP_POINTS_SIZE ) || ( ilevel <= RAND_PROP_POINTS_SIZE ) ) );
  return ptr ? __ptr_itemarmorshield_data[ ilevel - 1 ] : __itemarmorshield_data[ ilevel - 1 ];
#else
  assert( ilevel > 0 && ( ilevel <= RAND_PROP_POINTS_SIZE ) );
  return __itemarmorshield_data[ ilevel - 1 ];
#endif
}

const item_armor_type_data_t& dbc_t::item_armor_total( unsigned ilevel ) const
{
#if SC_USE_PTR
  assert( ilevel > 0 && ( ( ptr && ilevel <= PTR_RAND_PROP_POINTS_SIZE ) || ( ilevel <= RAND_PROP_POINTS_SIZE ) ) );
  return ptr ? __ptr_itemarmortotal_data[ ilevel - 1 ] : __itemarmortotal_data[ ilevel - 1 ];
#else
  assert( ilevel > 0 && ( ilevel <= RAND_PROP_POINTS_SIZE ) );
  return __itemarmortotal_data[ ilevel - 1 ];
#endif
}

double dbc_t::armor_mitigation_constant( unsigned level ) const
{
  assert( level > 0 && level <= ( MAX_LEVEL + 3 ) );
#if SC_USE_PTR
  return ptr ? __ptr___gt_armor_mitigation_by_lvl[ level - 1 ] : __gt_armor_mitigation_by_lvl[ level - 1 ];
#else
  return __gt_armor_mitigation_by_lvl[ level - 1 ];
#endif
}

const item_armor_type_data_t& dbc_t::item_armor_inv_type( unsigned inv_type ) const
{
  assert( inv_type > 0 && inv_type <= 23 );
#if SC_USE_PTR
  return ptr ? __ptr_armor_slot_data[ inv_type - 1 ] : __armor_slot_data[ inv_type - 1 ];
#else
  return __armor_slot_data[ inv_type - 1 ];
#endif
}

const item_upgrade_t& dbc_t::item_upgrade( unsigned upgrade_id ) const
{
#if SC_USE_PTR
  const item_upgrade_t* p = ptr ? __ptr_item_upgrade_data : __item_upgrade_data;
#else
  const item_upgrade_t* p = __item_upgrade_data;
#endif

  do
  {
    if ( p -> id == upgrade_id )
      return *p;
  }
  while ( ( p++ ) -> id );

  return nil_iu;
}

const item_upgrade_rule_t& dbc_t::item_upgrade_rule( unsigned item_id, unsigned upgrade_level ) const
{
#if SC_USE_PTR
  const item_upgrade_rule_t* p = ptr ? __ptr_item_upgrade_rule_data : __item_upgrade_rule_data;
#else
  const item_upgrade_rule_t* p = __item_upgrade_rule_data;
#endif

  do
  {
    if ( p -> item_id == item_id && p -> upgrade_ilevel == upgrade_level )
      return *p;
  }
  while ( ( p++ ) -> id );

  return nil_iur;
}

const rppm_modifier_t& dbc_t::real_ppm_modifier( specialization_e spec, unsigned spell_id ) const
{
#if SC_USE_PTR
  const rppm_modifier_t* p = ptr ? __ptr_rppmmodifier_data : __rppmmodifier_data;
#else
  const rppm_modifier_t* p = __rppmmodifier_data;
#endif

  while ( p -> spec != SPEC_NONE )
  {
    if ( p -> spec == spec && p -> spell_id == spell_id )
      return *p;
    p++;
  }

  return *p;
}

std::vector<const item_bonus_entry_t*> dbc_t::item_bonus( unsigned bonus_id ) const
{
#if SC_USE_PTR
  const item_bonus_entry_t* p = ptr ? __ptr_item_bonus_data : __item_bonus_data;
#else
  const item_bonus_entry_t* p = __item_bonus_data;
#endif

  std::vector<const item_bonus_entry_t*> entries;

  while ( p -> id != 0 )
  {
    if ( p -> bonus_id == bonus_id )
      entries.push_back(p);
    p++;
  }

  return entries;
}

const random_suffix_data_t& dbc_t::random_suffix( unsigned suffix_id ) const
{
#if SC_USE_PTR
  const random_suffix_data_t* p = ptr ? __ptr_rand_suffix_data : __rand_suffix_data;
#else
  const random_suffix_data_t* p = __rand_suffix_data;
#endif

  do
  {
    if ( p -> id == suffix_id )
      return *p;
  }
  while ( ( p++ ) -> id );

  return nil_rsd;
}

const item_enchantment_data_t& dbc_t::item_enchantment( unsigned enchant_id ) const
{
  if ( const item_enchantment_data_t* p = item_enchantment_data_index.get( maybe_ptr( ptr ), enchant_id ) )
    return *p;
  else
    return nil_ied;
}

const item_data_t* dbc_t::item( unsigned item_id ) const
{ return item_data_index.get( ptr, item_id ); }

const gem_property_data_t& dbc_t::gem_property( unsigned gem_id ) const
{
  ( void )ptr;

#if SC_USE_PTR
  const gem_property_data_t* p = ptr ? __ptr_gem_property_data : __gem_property_data;
#else
  const gem_property_data_t* p = __gem_property_data;
#endif

  do
  {
    if ( p -> id == gem_id )
      return *p;
  }
  while ( ( p++ ) -> id );

  return nil_gpd;
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
  if ( _m_avg != 0 && _spell -> scaling_class() != 0 )
    return _m_avg * budget;
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
  assert( level <= MAX_LEVEL );

  double m_scale = 0;

  if ( _m_avg != 0 && _spell -> scaling_class() != 0 )
  {
    unsigned scaling_level = level ? level : p -> level;
    if ( _spell -> max_scaling_level() > 0 )
      scaling_level = std::min( scaling_level, _spell -> max_scaling_level() );
    m_scale = p -> dbc.spell_scaling( _spell -> scaling_class(), scaling_level );
  }

  return scaled_average( m_scale, level );
}

double spelleffect_data_t::average( const item_t* item ) const
{
  double m_scale = 0;

  if ( _m_avg != 0 && _spell -> scaling_class() != 0 )
    m_scale = item_database::item_budget( item, _spell -> max_scaling_level() );

  return scaled_average( m_scale, item -> item_level() );
}

double spelleffect_data_t::scaled_delta( double budget ) const
{
  if ( _m_delta != 0 && budget > 0 )
    return _m_avg * _m_delta * budget;
  else if ( _m_avg == 0.0 && _m_delta == 0.0 && _die_sides != 0 )
    return _die_sides;
  else
    return 0;
}

double spelleffect_data_t::delta( const player_t* p, unsigned level ) const
{
  assert( level <= MAX_LEVEL );

  double m_scale = 0;
  if ( _m_delta != 0 && _spell -> scaling_class() != 0 )
  {
    unsigned scaling_level = level ? level : p -> level;
    if ( _spell -> max_scaling_level() > 0 )
      scaling_level = std::min( scaling_level, _spell -> max_scaling_level() );
    m_scale = p -> dbc.spell_scaling( _spell -> scaling_class(), scaling_level );
  }

  return scaled_delta( m_scale );
}

double spelleffect_data_t::delta( const item_t* item ) const
{
  double m_scale = 0;

  if ( _m_delta != 0 && _spell -> scaling_class() != 0 )
    m_scale = item_database::item_budget( item, _spell -> max_scaling_level() );

  return scaled_delta( m_scale );
}

double spelleffect_data_t::bonus( const player_t* p, unsigned level ) const
{
  assert( p );
  return p -> dbc.effect_bonus( id(), level ? level : p -> level );
}

double spelleffect_data_t::scaled_min( double avg, double delta ) const
{
  double result = 0;

  if ( _m_avg != 0 || _m_delta != 0 )
    result = avg - ( delta / 2 );
  else
    result = avg - delta;

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

  if ( _m_avg != 0 || _m_delta != 0 )
    result = avg + ( delta / 2 );
  else
    result = avg + delta;

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
  assert( level <= MAX_LEVEL );

  return scaled_min( average( p, level ), delta( p, level ) );
}

double spelleffect_data_t::min( const item_t* item ) const
{
  assert( _spell -> scaling_class() == 0 || _spell -> scaling_class() == -1 );

  return scaled_min( average( item ), delta( item ) );
}

double spelleffect_data_t::max( const player_t* p, unsigned level ) const
{
  assert( level <= MAX_LEVEL );

  return scaled_max( average( p, level ), delta( p, level ) );
}

double spelleffect_data_t::max( const item_t* item ) const
{
  assert( _spell -> scaling_class() == 0 || _spell -> scaling_class() == -1 );

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
  return 0;
}

// Always returns non-NULL
spelleffect_data_t* spelleffect_data_t::find( unsigned id, bool ptr )
{
  spelleffect_data_t* effect = spelleffect_data_index.get( ptr, id );
  if ( ! effect )
    effect = spelleffect_data_t::nil();
  return effect;
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

  return NULL;
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
  ( void )confirmation;

  talent_data_t* p = find( id, ptr );
  assert( ! strcmp( confirmation, p -> name_cstr() ) );
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

  return 0;
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

  return 0;
}

void spell_data_t::link( bool /* ptr */ )
{
}

void spelleffect_data_t::link( bool ptr )
{
  spelleffect_data_t* spelleffect_data = spelleffect_data_t::list( ptr );

  for ( int i = 0; spelleffect_data[ i ].id(); i++ )
  {
    spelleffect_data_t& ed = spelleffect_data[ i ];

    ed._spell         = spell_data_t::find( ed.spell_id(), ptr );
    ed._trigger_spell = spell_data_t::find( ed.trigger_spell_id(), ptr );

    if ( ed._spell -> _effects == 0 )
      ed._spell -> _effects = new std::vector<const spelleffect_data_t*>;

    if ( ed._spell -> _effects -> size() < ( ed.index() + 1 ) )
      ed._spell -> _effects -> resize( ed.index() + 1, spelleffect_data_t::nil() );

    ed._spell -> _effects -> at( ed.index() ) = &ed;
  }
}

void spell_data_t::de_link( bool ptr )
{
  spell_data_t* spell_data = spell_data_t::list( ptr );

  for ( int i = 0; spell_data[ i ].id(); i++ )
  {
    spell_data_t& sd = spell_data[ i ];

    if ( sd._effects )
    {
      // delete dynamically allocated vector with spelleffect_data_t pointers
      delete sd._effects;
      sd._effects = 0;
    }
    if ( sd._power )
    {
      // delete dynamically allocated vector with spellpower_data_t pointers
      delete sd._power;
      sd._power = 0;
    }
  }
}

void spellpower_data_t::link( bool ptr )
{
  spellpower_data_t* spellpower_data = spellpower_data_t::list( ptr );

  for ( int i = 0; spellpower_data[ i ]._id; i++ )
  {
    spellpower_data_t& pd = spellpower_data[ i ];
    spell_data_t*      sd = spell_data_t::find( pd._spell_id, ptr );

    if ( sd -> _power == 0 )
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
  const spelleffect_data_t* e = effect( effect_id );

  assert( e && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  if ( e -> m_average() != 0 && e -> _spell -> scaling_class() != 0 )
  {
    unsigned scaling_level = level;
    if ( e -> _spell -> max_scaling_level() > 0 )
      scaling_level = std::min( scaling_level, e -> _spell -> max_scaling_level() );
    double m_scale = spell_scaling( e -> _spell -> scaling_class(), scaling_level );

    return e -> m_average() * m_scale;
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
  if ( ! effect_id )
    return 0.0;

  const spelleffect_data_t* e = effect( effect_id );

  assert( e && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  if ( e -> m_delta() != 0 && e -> _spell -> scaling_class() != 0 )
  {
    unsigned scaling_level = level;
    if ( e -> _spell -> max_scaling_level() > 0 )
      scaling_level = std::min( scaling_level, e -> _spell -> max_scaling_level() );
    double m_scale = spell_scaling( e -> _spell -> scaling_class(), scaling_level );

    return e -> m_average() * e -> m_delta() * m_scale;
  }
  else if ( ( e -> m_average() == 0.0 ) && ( e -> m_delta() == 0.0 ) && ( e -> die_sides() != 0 ) )
    return e -> die_sides();

  return 0;
}

double dbc_t::effect_bonus( unsigned effect_id, unsigned level ) const
{
  if ( ! effect_id )
    return 0.0;

  const spelleffect_data_t* e = effect( effect_id );

  assert( e && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  if ( e -> m_unk() != 0 && e -> _spell -> scaling_class() != 0 )
  {
    unsigned scaling_level = level;
    if ( e -> _spell -> max_scaling_level() > 0 )
      scaling_level = std::min( scaling_level, e -> _spell -> max_scaling_level() );
    double m_scale = spell_scaling( e -> _spell -> scaling_class(), scaling_level );

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

  const spelleffect_data_t* e = effect( effect_id );
  double avg, result;

  assert( e && ( level > 0 ) );
  assert( ( level <= MAX_LEVEL ) );

  unsigned c_id = util::class_id( e -> _spell -> scaling_class() );
  avg = effect_average( effect_id, level );

  if ( c_id != 0 && ( e -> m_average() != 0 || e -> m_delta() != 0 ) )
  {
    double delta = effect_delta( effect_id, level );
    result = avg - ( delta / 2 );
  }
  else
  {
    int die_sides = e -> die_sides();
    if ( die_sides == 0 )
      result = avg;
    else if ( die_sides == 1 )
      result =  avg + die_sides;
    else
      result = avg + ( die_sides > 1  ? 1 : die_sides );

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
  if ( ! effect_id )
    return 0.0;

  const spelleffect_data_t* e = effect( effect_id );
  double avg, result;

  assert( e && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  unsigned c_id = util::class_id( e -> _spell -> scaling_class() );
  avg = effect_average( effect_id, level );

  if ( c_id != 0 && ( e -> m_average() != 0 || e -> m_delta() != 0 ) )
  {
    double delta = effect_delta( effect_id, level );

    result = avg + ( delta / 2 );
  }
  else
  {
    int die_sides = e -> die_sides();
    if ( die_sides == 0 )
      result = avg;
    else if ( die_sides == 1 )
      result = avg + die_sides;
    else
      result = avg + ( die_sides > 1  ? die_sides : -1 );

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

unsigned dbc_t::perk_ability_id( specialization_e spec_id, size_t perk_idx ) const
{
  unsigned class_idx = -1;
  unsigned spec_index = -1;

  if ( perk_idx >= perk_ability_size() )
    return 0;

  if ( ! spec_idx( spec_id, class_idx, spec_index ) )
    return 0;

  assert( ( int )class_idx >= 0 && class_idx < specialization_max_class() &&
          ( int )spec_index >= 0 && spec_index < specialization_max_per_class() );

  return perk_ability( class_idx, spec_index, static_cast<unsigned int>(perk_idx) );
}

unsigned dbc_t::perk_ability_id( specialization_e spec_id, const char* spell_name ) const
{
  unsigned class_idx = -1;
  unsigned spec_index = -1;

  assert( spell_name && spell_name[ 0 ] );

  if ( ! spec_idx( spec_id, class_idx, spec_index ) )
    return 0;

  assert( ( int )class_idx >= 0 && class_idx < specialization_max_class() &&
          ( int )spec_index >= 0 && spec_index < specialization_max_per_class() );

  for ( unsigned n = 0; n < perk_ability_size(); n++ )
  {
    unsigned spell_id;
    if ( ! ( spell_id = perk_ability( class_idx, spec_index, n ) ) )
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

unsigned dbc_t::glyph_spell_id( unsigned property_id ) const
{
#if SC_USE_PTR
  const glyph_property_data_t* table = ptr ? &__ptr_glyph_property_data[0] : &__glyph_property_data[0];
#else
  const glyph_property_data_t* table = &__glyph_property_data[0];
#endif

  while ( table -> id != 0 )
  {
    if ( table -> id == property_id )
      return table -> spell_id;
    table++;
  }

  return 0;
}

unsigned dbc_t::glyph_spell_id( player_e c, const char* spell_name ) const
{
  unsigned cid = util::class_id( c );
  unsigned spell_id;
  std::string token, token2;

  if ( ! spell_name || ! *spell_name )
    return 0;

  token = spell_name;
  util::glyph_name( token );

  for ( unsigned type = 0; type < GLYPH_MAX; type++ )
  {
    for ( unsigned n = 0; n < glyph_spell_size(); n++ )
    {
      if ( ! ( spell_id = glyph_spell( cid, type, n ) ) )
        break;

      if ( ! spell( spell_id ) -> id() )
        continue;

      token2 = spell( spell_id ) -> name_cstr();
      util::glyph_name( token2 );

      if ( util::str_compare_ci( spell( spell_id ) -> name_cstr(), spell_name ) ||
           util::str_compare_ci( token2, token ) )
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

  return 0;
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

  if ( item_data -> quality > 5 ) return 0.0;

  unsigned ilvl = ilevel ? ilevel : item_data -> level;

  switch ( item_data -> inventory_type )
  {
    case INVTYPE_WEAPON:
    case INVTYPE_WEAPONMAINHAND:
    case INVTYPE_WEAPONOFFHAND:
    {
      if ( item_data -> flags_2 & ITEM_FLAG2_CASTER_WEAPON )
        return item_damage_caster_1h( ilvl ).values[ item_data -> quality ];
      else
        return item_damage_1h( ilvl ).values[ item_data -> quality ];
      break;
    }
    case INVTYPE_2HWEAPON:
    {
      if ( item_data -> flags_2 & ITEM_FLAG2_CASTER_WEAPON )
        return item_damage_caster_2h( ilvl ).values[ item_data -> quality ];
      else
        return item_damage_2h( ilvl ).values[ item_data -> quality ];
      break;
    }
    case INVTYPE_RANGED:
    case INVTYPE_THROWN:
    case INVTYPE_RANGEDRIGHT:
    {
      switch ( item_data -> item_subclass )
      {
        case ITEM_SUBCLASS_WEAPON_BOW:
        case ITEM_SUBCLASS_WEAPON_GUN:
        case ITEM_SUBCLASS_WEAPON_CROSSBOW:
        {
          return item_damage_ranged( ilvl ).values[ item_data -> quality ];
        }
        case ITEM_SUBCLASS_WEAPON_THROWN:
        {
          return item_damage_thrown( ilvl ).values[ item_data -> quality ];
        }
        case ITEM_SUBCLASS_WEAPON_WAND:
        {
          return item_damage_wand( ilvl ).values[ item_data -> quality ];
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

double dbc_t::rppm_coefficient( specialization_e spec, unsigned spell_id ) const
{
  const rppm_modifier_t& rppmm = real_ppm_modifier( spec, spell_id );
  return 1.0 + rppmm.coefficient;
}

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
};

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
