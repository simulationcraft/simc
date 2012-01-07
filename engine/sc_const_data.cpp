// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

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

random_suffix_data_t nil_rsd;
item_enchantment_data_t nil_ied;
gem_property_data_t nil_gpd;

#if 1
// Indices to provide log time, constant space access to spells, effects, and talents by id.
template <typename T>
class dbc_index_t
{
private:
  typedef std::pair<T*,T*> index_t;
  index_t idx[2];

  void populate( index_t& idx, T* list )
  {
    idx.first = list;
    for ( unsigned last_id = 0; list -> id(); last_id = list -> id(), ++list )
      assert( list -> id() > last_id );
    idx.second = list;
  }

  struct id_compare
  {
    bool operator () ( const T& t, unsigned int id ) const
    { return t.id() < id; }
    bool operator () ( unsigned int id, const T& t ) const
    { return id < t.id(); }
    bool operator () ( const T& l, const T& r ) const
    { return l.id() < r.id(); }
  };

public:
  dbc_index_t() : idx() {}

  void init()
  {
    assert( ! initialized( false ) );
    populate( idx[ false ], T::list( false ) );
    if ( SC_USE_PTR )
      populate( idx[ true ], T::list( true ) );
  }

  bool initialized( bool ptr = false ) const
  { return idx[ ptr ].first != 0; }

  T* get( bool ptr, unsigned id ) const
  {
    assert( initialized( ptr ) );
    T* p = std::lower_bound( idx[ ptr].first, idx[ ptr ].second, id, id_compare() );
    if ( p != idx[ ptr ].second && p -> id() == id )
      return p;
    return T::nil();
  }
};
#else
// Indices to provide constant time, linear space access to spells, effects, and talents by id.
template <typename T>
class dbc_index_t
{
private:
  typedef std::vector<T*> index_t;
  index_t idx[2];

  void populate( index_t& idx, T* list )
  {
    unsigned max_id = 0;
    for ( T* p = list; p -> id(); ++p )
    {
      if ( max_id < p -> id() )
        max_id = p -> id();
    }

    idx.assign( max_id + 1, T::nil() );
    for ( T* p = list; p -> id(); ++p )
      idx[ p -> id() ] = p;
  }

public:
  void init()
  {
    assert( ! initialized( false ) );
    populate( idx[ false ], T::list( false ) );
    if ( SC_USE_PTR )
      populate( idx[ true ], T::list( true ) );
  }

  bool initialized( bool ptr = false ) const
  { return ! idx[ ptr ].empty(); }

  T* get( bool ptr, unsigned id ) const
  {
    assert( initialized( ptr ) );
    if ( id < idx[ ptr ].size() )
      return idx[ ptr ][ id ];
    else
      return T::nil();
  }
};
#endif

dbc_index_t<spell_data_t> idx_sd;
dbc_index_t<spelleffect_data_t> idx_sed;
dbc_index_t<talent_data_t> idx_td;

} // ANONYMOUS namespace ====================================================

const char* dbc_t::build_level( bool ptr )
{ return ( SC_USE_PTR && ptr ) ? "15171" : "15050"; }

const char* dbc_t::wow_version( bool ptr )
{ return ( SC_USE_PTR && ptr ) ? "4.3.2" : "4.3.0"; }

void dbc_t::init()
{
  idx_sd.init();
  idx_sed.init();
  idx_td.init();

  spell_data_t::link( false );
  spelleffect_data_t::link( false );
  talent_data_t::link( false );

  if ( SC_USE_PTR )
  {
    spell_data_t::link( true );
    spelleffect_data_t::link( true );
    talent_data_t::link( true );
  }
}

std::vector<unsigned> dbc_t::glyphs( int cid, bool ptr )
{
  ( void )ptr;
  std::vector<unsigned> glyph_ids;

  for ( int i=0; i < GLYPH_MAX; i++ )
  {
    for ( int j=0; j < GLYPH_ABILITIES_SIZE; j++ )
    {
#if SC_USE_PTR
      unsigned id = ptr ? __ptr_glyph_abilities_data[ cid ][ i ][ j ] : __glyph_abilities_data[ cid ][ i ][ j ];
#else
      unsigned id = __glyph_abilities_data[ cid ][ i ][ j ];
#endif
      if ( ! id ) break;
      glyph_ids.push_back( id );
    }
  }

  return glyph_ids;
}

double dbc_t::melee_crit_base( player_type t ) const
{
  uint32_t class_id = util_t::class_id( t );

  assert( class_id < CLASS_SIZE );
#if SC_USE_PTR
  return ptr ? __ptr_gt_chance_to_melee_crit_base[ class_id ]
             : __gt_chance_to_melee_crit_base[ class_id ];
#else
  return __gt_chance_to_melee_crit_base[ class_id ];
#endif
}

double dbc_t::melee_crit_base( pet_type_t t ) const
{
  return melee_crit_base( util_t::pet_class_type( t ) );
}

double dbc_t::spell_crit_base( player_type t ) const
{
  uint32_t class_id = util_t::class_id( t );

  assert( class_id < CLASS_SIZE );
#if SC_USE_PTR
  return ptr ? __ptr_gt_chance_to_spell_crit_base[ class_id ]
             : __gt_chance_to_spell_crit_base[ class_id ];
#else
  return __gt_chance_to_spell_crit_base[ class_id ];
#endif
}

double dbc_t::spell_crit_base( pet_type_t t ) const
{
  return spell_crit_base( util_t::pet_class_type( t ) );
}

double dbc_t::dodge_base( player_type t ) const
{
  uint32_t class_id = util_t::class_id( t );

  assert( class_id < CLASS_SIZE );
#if SC_USE_PTR
  return ptr ? __ptr_gt_chance_to_dodge_base[ class_id ]
             : __gt_chance_to_dodge_base[ class_id ];
#else
  return __gt_chance_to_dodge_base[ class_id ];
#endif
}

double dbc_t::dodge_base( pet_type_t t ) const
{
  return dodge_base( util_t::pet_class_type( t ) );
}

stat_data_t& dbc_t::race_base( race_type r ) const
{
  uint32_t race_id = util_t::race_id( r );

  assert( race_id < 24 );
#if SC_USE_PTR
  return ptr ? __ptr_gt_race_stats[ race_id ]
             : __gt_race_stats[ race_id ];
#else
  return __gt_race_stats[ race_id ];
#endif
}

stat_data_t& dbc_t::race_base( pet_type_t /* r */ ) const
{
  return race_base( RACE_NONE );
}

double dbc_t::spell_scaling( player_type t, unsigned level ) const
{
  uint32_t class_id = util_t::class_id( t );

  assert( class_id < CLASS_SIZE + 1 && level > 0 && level < MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_spell_scaling[ class_id ][ level ]
             : __gt_spell_scaling[ class_id ][ level ];
#else
  return __gt_spell_scaling[ class_id ][ level ];
#endif
}

double dbc_t::melee_crit_scaling( player_type t, unsigned level ) const
{
  uint32_t class_id = util_t::class_id( t );

  assert( class_id < CLASS_SIZE && level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_chance_to_melee_crit[ class_id ][ level - 1 ]
             : __gt_chance_to_melee_crit[ class_id ][ level - 1 ];
#else
  return __gt_chance_to_melee_crit[ class_id ][ level - 1 ];
#endif
}

double dbc_t::melee_crit_scaling( pet_type_t t, unsigned level ) const
{
  return melee_crit_scaling( util_t::pet_class_type( t ), level );
}

double dbc_t::spell_crit_scaling( player_type t, unsigned level ) const
{
  uint32_t class_id = util_t::class_id( t );

  assert( class_id < CLASS_SIZE && level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_chance_to_spell_crit[ class_id ][ level - 1 ]
             : __gt_chance_to_spell_crit[ class_id ][ level - 1 ];
#else
  return __gt_chance_to_spell_crit[ class_id ][ level - 1 ];
#endif
}

double dbc_t::spell_crit_scaling( pet_type_t t, unsigned level ) const
{
  return spell_crit_scaling( util_t::pet_class_type( t ), level );
}

double dbc_t::dodge_scaling( player_type t, unsigned level ) const
{
  uint32_t class_id = util_t::class_id( t );

  assert( class_id < CLASS_SIZE && level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_dodge_per_agi[ class_id ][ level - 1 ]
             : __gt_dodge_per_agi[ class_id ][ level - 1 ];
#else
  return __gt_dodge_per_agi[ class_id ][ level - 1 ];
#endif
}

double dbc_t::dodge_scaling( pet_type_t t, unsigned level ) const
{
  return dodge_scaling( util_t::pet_class_type( t ), level );
}

double dbc_t::regen_base( player_type t, unsigned level ) const
{
  uint32_t class_id = util_t::class_id( t );

  assert( class_id < CLASS_SIZE && level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_base_mp5[ class_id ][ level - 1 ]
             : __gt_base_mp5[ class_id ][ level - 1 ];
#else
  return __gt_base_mp5[ class_id ][ level - 1 ];
#endif
}

double dbc_t::regen_base( pet_type_t t, unsigned level ) const
{
  return regen_base( util_t::pet_class_type( t ), level );
}

double dbc_t::regen_spirit( player_type t, unsigned level ) const
{
  uint32_t class_id = util_t::class_id( t );

  assert( class_id < CLASS_SIZE && level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_regen_mpper_spt[ class_id ][ level - 1 ]
             : __gt_regen_mpper_spt[ class_id ][ level - 1 ];
#else
  return __gt_regen_mpper_spt[ class_id ][ level - 1 ];
#endif
}

double dbc_t::regen_spirit( pet_type_t t, unsigned level ) const
{
  return regen_spirit( util_t::pet_class_type( t ), level );
}

stat_data_t& dbc_t::attribute_base( player_type t, unsigned level ) const
{
  uint32_t class_id = util_t::class_id( t );

  assert( class_id < CLASS_SIZE && level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_class_stats_by_level[ class_id ][ level - 1 ]
             : __gt_class_stats_by_level[ class_id ][ level - 1 ];
#else
  return __gt_class_stats_by_level[ class_id ][ level - 1 ];
#endif
}

stat_data_t& dbc_t::attribute_base( pet_type_t t, unsigned level ) const
{
  return attribute_base( util_t::pet_class_type( t ), level );
}

double dbc_t::oct_regen_mp( player_type t, unsigned level ) const
{
  uint32_t class_id = util_t::class_id( t );

  assert( class_id < CLASS_SIZE && level > 0 && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_octregen_mp[ class_id ][ level - 1 ]
             : __gt_octregen_mp[ class_id ][ level - 1 ];
#else
  return __gt_octregen_mp[ class_id ][ level - 1 ];
#endif
}

double dbc_t::oct_regen_mp( pet_type_t t, unsigned level ) const
{
  return oct_regen_mp( util_t::pet_class_type( t ), level );
}

double dbc_t::combat_rating( unsigned combat_rating_id, unsigned level ) const
{
  assert( combat_rating_id < RATING_MAX && level <= MAX_LEVEL );
#if SC_USE_PTR
  return ptr ? __ptr_gt_combat_ratings[ combat_rating_id ][ level - 1 ] * 100.0
             : __gt_combat_ratings[ combat_rating_id ][ level - 1 ] * 100.0;
#else
  return __gt_combat_ratings[ combat_rating_id ][ level - 1 ] * 100.0 ;
#endif
}

double dbc_t::oct_combat_rating( unsigned combat_rating_id, player_type t ) const
{
  uint32_t class_id = util_t::class_id( t );

  assert( combat_rating_id < RATING_MAX && class_id < PLAYER_PET + 1 );
#if SC_USE_PTR
  return ptr ? __ptr_gt_octclass_combat_rating_scalar[ combat_rating_id ][ class_id ]
             : __gt_octclass_combat_rating_scalar[ combat_rating_id ][ class_id ];
#else
  return __gt_octclass_combat_rating_scalar[ combat_rating_id ][ class_id ];
#endif
}

unsigned dbc_t::class_ability( unsigned class_id, unsigned tree_id, unsigned n ) const
{
  assert( class_id < CLASS_SIZE && tree_id < CLASS_ABILITY_TREE_SIZE && n < CLASS_ABILITY_SIZE );

#if SC_USE_PTR
  return ptr ? __ptr_class_ability_data[ class_id ][ tree_id ][ n ]
             : __class_ability_data[ class_id ][ tree_id ][ n ];
#else
  return __class_ability_data[ class_id ][ tree_id ][ n ];
#endif
}

unsigned dbc_t::race_ability( unsigned race_id, unsigned class_id, unsigned n ) const
{
  assert( race_id < 24 && class_id < CLASS_SIZE && n < RACE_ABILITY_SIZE );

#if SC_USE_PTR
  return ptr ? __ptr_race_ability_data[ race_id ][ class_id ][ n ]
             : __race_ability_data[ race_id ][ class_id ][ n ];
#else
  return __race_ability_data[ race_id ][ class_id ][ n ];
#endif
}

unsigned dbc_t::specialization_ability( unsigned class_id, unsigned tree_id, unsigned n ) const
{
  assert( class_id < CLASS_SIZE && tree_id < MAX_TALENT_TABS && n < specialization_ability_size() );

#if SC_USE_PTR
  return ptr ? __ptr_tree_specialization_data[ class_id ][ tree_id ][ n ]
             : __tree_specialization_data[ class_id ][ tree_id ][ n ];
#else
  return __tree_specialization_data[ class_id ][ tree_id ][ n ];
#endif
}

unsigned dbc_t::mastery_ability( unsigned class_id, unsigned n ) const
{
  assert( class_id < CLASS_SIZE && n < mastery_ability_size() );

#if SC_USE_PTR
  return ptr ? __ptr_class_mastery_ability_data[ class_id ][ n ]
             : __class_mastery_ability_data[ class_id ][ n ];
#else
  return __class_mastery_ability_data[ class_id ][ n ];
#endif
}

unsigned dbc_t::glyph_spell( unsigned class_id, unsigned glyph_type, unsigned n ) const
{
  assert( class_id < CLASS_SIZE && glyph_type < GLYPH_MAX && n < glyph_spell_size() );

#if SC_USE_PTR
  return ptr ? __ptr_glyph_abilities_data[ class_id ][ glyph_type ][ n ]
             : __glyph_abilities_data[ class_id ][ glyph_type ][ n ];
#else
  return __glyph_abilities_data[ class_id ][ glyph_type ][ n ];
#endif
}

unsigned dbc_t::set_bonus_spell( unsigned class_id, unsigned tier, unsigned n ) const
{
#if SC_USE_PTR
  assert( class_id < CLASS_SIZE && tier < (unsigned) ( ptr ? PTR_TIER_BONUSES_MAX_TIER : TIER_BONUSES_MAX_TIER ) && n < set_bonus_spell_size() );
  return ptr ? __ptr_tier_bonuses_data[ class_id ][ tier ][ n ]
             : __tier_bonuses_data[ class_id ][ tier ][ n ];
#else
  assert( class_id < CLASS_SIZE && tier < TIER_BONUSES_MAX_TIER && n < set_bonus_spell_size() );
  return __tier_bonuses_data[ class_id ][ tier ][ n ];
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

unsigned dbc_t::race_ability_size() const
{
#if SC_USE_PTR
  return ptr ? PTR_RACE_ABILITY_SIZE : RACE_ABILITY_SIZE;
#else
  return RACE_ABILITY_SIZE;
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

const item_armor_type_data_t& dbc_t::item_armor_inv_type( unsigned inv_type ) const
{
  assert( inv_type > 0 && inv_type <= 23 );
#if SC_USE_PTR
  return ptr ? __ptr_armor_slot_data[ inv_type - 1 ] : __armor_slot_data[ inv_type - 1 ];
#else
  return __armor_slot_data[ inv_type - 1 ];
#endif
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
#if SC_USE_PTR
  const item_enchantment_data_t* p = ptr ? __ptr_spell_item_ench_data : __spell_item_ench_data;
#else
  const item_enchantment_data_t* p = __spell_item_ench_data;
#endif

  do
  {
    if ( p -> id == enchant_id )
      return *p;
  }
  while ( ( p++ ) -> id );

  return nil_ied;
}

const item_data_t* dbc_t::item( unsigned item_id ) const
{
  ( void )ptr;

#if SC_USE_PTR
  assert( item_id <= ( ptr ? __ptr_item_data[ PTR_ITEM_SIZE - 1 ].id : __item_data[ ITEM_SIZE - 1 ].id ) );
  const item_data_t* item_data = ( ptr ? __ptr_item_data : __item_data );
#else
  assert( item_id <= __item_data[ ITEM_SIZE - 1 ].id );
  const item_data_t* item_data = __item_data;
#endif

  for ( int i = 0; item_data[ i ].id; i++ )
  {
    if ( item_id == item_data[ i ].id )
      return item_data + i;
  }

  return 0;
}

const item_data_t* dbc_t::items( bool ptr )
{
  ( void )ptr;

#if SC_USE_PTR
  return ptr ? &( __ptr_item_data[ 0 ] ) : &( __item_data[ 0 ] ) ;
#else
  return &( __item_data[ 0 ] ) ;
#endif
}

size_t dbc_t::n_items( bool ptr )
{
  ( void )ptr;

#if SC_USE_PTR
  return ptr ? PTR_ITEM_SIZE : ITEM_SIZE;
#else
  return ITEM_SIZE;
#endif
}

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
{ return idx_sd.get( ptr, spell_id ); }

spell_data_t* spell_data_t::find( unsigned spell_id, const char* confirmation, bool ptr )
{
  ( void )confirmation;

  spell_data_t* p = find( spell_id, ptr );
  assert( ! strcmp( confirmation, p -> name_cstr() ) );
  return p;
}

spell_data_t* spell_data_t::find( const char* name, bool ptr )
{
  for ( spell_data_t* p = spell_data_t::list( ptr ); p -> name_cstr(); ++p )
    if ( ! strcmp ( name, p -> name_cstr() ) )
      return p;

  return 0;
}

spelleffect_data_t* spelleffect_data_t::find( unsigned id, bool ptr )
{ return idx_sed.get( ptr, id ); }

talent_data_t* talent_data_t::find( unsigned id, bool ptr )
{ return idx_td.get( ptr, id ); }

talent_data_t* talent_data_t::find( unsigned id, const char* confirmation, bool ptr )
{
  ( void )confirmation;

  talent_data_t* p = find( id, ptr );
  assert( ! strcmp( confirmation, p -> name_cstr() ) );
  return p;
}

talent_data_t* talent_data_t::find( const char* name, bool ptr )
{
  for ( talent_data_t* p = talent_data_t::list( ptr ); p -> name_cstr(); ++p )
    if ( ! strcmp( name, p -> name_cstr() ) )
      return p;

  return 0;
}

void spell_data_t::link( bool ptr )
{
  spell_data_t* spell_data = spell_data_t::list( ptr );

  for ( int i = 0; spell_data[ i ].name_cstr(); i++ )
  {
    spell_data_t& sd = spell_data[ i ];
    sd._effect1 = spelleffect_data_t::find( sd._effect[ 0 ], ptr );
    sd._effect2 = spelleffect_data_t::find( sd._effect[ 1 ], ptr );
    sd._effect3 = spelleffect_data_t::find( sd._effect[ 2 ], ptr );
  }
}

void spelleffect_data_t::link( bool ptr )
{
  spelleffect_data_t* spelleffect_data = spelleffect_data_t::list( ptr );

  for ( int i=0; spelleffect_data[ i ].id(); i++ )
  {
    spelleffect_data_t& ed = spelleffect_data[ i ];

    ed._spell         = spell_data_t::find( ed.spell_id(), ptr );
    ed._trigger_spell = spell_data_t::find( ed.trigger_spell_id(), ptr );
  }
}

void talent_data_t::link( bool ptr )
{
  talent_data_t* talent_data = talent_data_t::list( ptr );

  for ( int i=0; talent_data[ i ].name_cstr(); i++ )
  {
    talent_data_t& td = talent_data[ i ];

    td.spell1 = spell_data_t::find( td._rank_id[ 0 ], ptr );
    td.spell2 = spell_data_t::find( td._rank_id[ 1 ], ptr );
    td.spell3 = spell_data_t::find( td._rank_id[ 2 ], ptr );
  }
}

/* Generic helper methods */

double dbc_t::effect_average( unsigned effect_id, unsigned level ) const
{
  const spelleffect_data_t* e = effect( effect_id );

  assert( e && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  if ( e -> m_average() != 0 && e -> _spell -> scaling_class() != 0 )
  {
    double m_scale = spell_scaling( e -> _spell -> scaling_class(), level - 1 );

    assert( m_scale != 0 );

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
    double m_scale = spell_scaling( e -> _spell -> scaling_class(), level - 1 );

    assert( m_scale != 0 );

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
    double m_scale = spell_scaling( e -> _spell -> scaling_class(), level - 1 );

    assert( m_scale != 0 );

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
  double avg, delta, result;

  assert( e && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  unsigned c_id = util_t::class_id( e -> _spell -> scaling_class() );
  avg = effect_average( effect_id, level );

  if ( c_id != 0 && ( e -> m_average() != 0 || e -> m_delta() != 0 ) )
  {
    delta = effect_delta( effect_id, level );
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
  double avg, delta, result;

  assert( e && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  unsigned c_id = util_t::class_id( e -> _spell -> scaling_class() );
  avg = effect_average( effect_id, level );

  if ( c_id != 0 && ( e -> m_average() != 0 || e -> m_delta() != 0 ) )
  {
    delta = effect_delta( effect_id, level );

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

unsigned dbc_t::class_ability_id( player_type c, const char* spell_name, int tree ) const
{
  uint32_t cid = util_t::class_id( c );
  unsigned spell_id;

  if ( ( c == PLAYER_PET ) || ( c == PLAYER_GUARDIAN ) ) tree = 3;

  assert( spell_name && spell_name[ 0 ] && ( tree < ( int ) class_ability_tree_size() ) && ( tree >= -1 ) );

  if ( tree < 0 )
  {
    for ( unsigned t = 0; t < class_ability_tree_size(); t++ )
    {
      for ( unsigned n = 0; n < class_ability_size(); n++ )
      {
        if ( ! ( spell_id = class_ability( cid, t, n ) ) )
          break;

        if ( ! spell( spell_id ) -> id() )
          continue;

        if ( util_t::str_compare_ci( spell( spell_id ) -> name_cstr(), spell_name ) )
          return spell_id;
      }
    }
  }
  else
  {
    for ( unsigned n = 0; n < class_ability_size(); n++ )
    {
      if ( ! ( spell_id = class_ability( cid, tree, n ) ) )
        break;

      if ( ! spell( spell_id ) -> id() )
        continue;

      if ( util_t::str_compare_ci( spell( spell_id ) -> name_cstr(), spell_name ) )
        return spell_id;
    }
  }

  return 0;
}

// TODO: Implement racial spell fetch at some point
unsigned dbc_t::race_ability_id( player_type /* c */, race_type /* r */, const char* /* spell_name */ ) const
{
  return 0;
}

unsigned dbc_t::specialization_ability_id( player_type c, const char* spell_name, int tab_name ) const
{
  unsigned cid = util_t::class_id( c );
  unsigned spell_id;

  assert( spell_name && spell_name[ 0 ] && ( tab_name < MAX_TALENT_TABS ) );

  if ( tab_name < 0 )
  {
    for ( unsigned tree = 0; tree < MAX_TALENT_TABS; tree++ )
    {
      for ( unsigned n = 0; n < specialization_ability_size(); n++ )
      {
        if ( ! ( spell_id = specialization_ability( cid, tree, n ) ) )
          break;

        if ( ! spell( spell_id ) -> id() )
          continue;

        if ( util_t::str_compare_ci( spell( spell_id ) -> name_cstr(), spell_name ) )
          return spell_id;
      }
    }
  }
  else
  {
    for ( unsigned n = 0; n < specialization_ability_size(); n++ )
    {
      if ( ! ( spell_id = specialization_ability( cid, tab_name, n ) ) )
        break;

      if ( ! spell( spell_id ) -> id() )
        continue;

      if ( util_t::str_compare_ci( spell( spell_id ) -> name_cstr(), spell_name ) )
        return spell_id;
    }
  }

  return 0;
}

int dbc_t::specialization_ability_tree( player_type c, uint32_t spell_id ) const
{
  uint32_t cid = util_t::class_id( c );

  for ( unsigned tree = 0; tree < MAX_TALENT_TABS; tree++ )
  {
    for ( unsigned n = 0; n < specialization_ability_size(); n++ )
    {
      if ( specialization_ability( cid, tree, n ) == spell_id )
        return tree;
    }
  }

  return -1;
}

int dbc_t::class_ability_tree( player_type c, uint32_t spell_id ) const
{
  uint32_t cid = util_t::class_id( c );

  for ( unsigned tree = 0; tree < class_ability_tree_size(); tree++ )
  {
    for ( unsigned n = 0; n < class_ability_size(); n++ )
    {
      if ( class_ability( cid, tree, n ) == spell_id )
        return tree;
    }
  }

  return -1;
}

unsigned dbc_t::glyph_spell_id( player_type c, const char* spell_name ) const
{
  unsigned cid = util_t::class_id( c );
  unsigned spell_id;

  assert( spell_name && spell_name[ 0 ] );

  for ( unsigned type = 0; type < GLYPH_MAX; type++ )
  {
    for ( unsigned n = 0; n < glyph_spell_size(); n++ )
    {
      if ( ! ( spell_id = glyph_spell( cid, type, n ) ) )
        break;

      if ( ! spell( spell_id ) -> id() )
        continue;

      if ( util_t::str_compare_ci( spell( spell_id ) -> name_cstr(), spell_name ) )
        return spell_id;
    }
  }

  return 0;
}

unsigned dbc_t::set_bonus_spell_id( player_type c, const char* name, int tier ) const
{
  unsigned cid = util_t::class_id( c );
  unsigned spell_id;

  assert( name && name[ 0 ] && tier < 11 );

  if ( tier == -1 ) tier = 11;

  tier -= 11;

  for ( int t = 0; t < tier; t++ )
  {
    for ( unsigned n = 0; n < set_bonus_spell_size(); n++ )
    {
      if ( ! ( spell_id = set_bonus_spell( cid, t, n ) ) )
        break;

      if ( ! spell( spell_id ) -> id() )
        continue;

      if ( util_t::str_compare_ci( spell( spell_id ) -> name_cstr(), name ) )
        return spell_id;
    }
  }

  return 0;
}

unsigned dbc_t::mastery_ability_id( player_type c, const char* spell_name ) const
{
  uint32_t cid = util_t::class_id( c );
  uint32_t spell_id;

  assert( spell_name && spell_name[ 0 ] );

  for ( unsigned n = 0; n < mastery_ability_size(); n++ )
  {
    if ( ! ( spell_id = mastery_ability( cid, n ) ) )
      break;

    if ( ! spell( spell_id ) -> id() )
      continue;

    if ( util_t::str_compare_ci( spell( spell_id ) -> name_cstr(), spell_name ) )
      return spell_id;
  }

  return 0;
}

bool dbc_t::is_class_ability( uint32_t spell_id ) const
{
  for ( unsigned cls = 0; cls < 12; cls++ )
  {
    for ( unsigned tree = 0; tree < class_ability_tree_size(); tree++ )
    {
      for ( unsigned n = 0; n < class_ability_size(); n++ )
      {
        if ( class_ability( cls, tree, n ) == spell_id )
          return true;
      }
    }
  }

  return false;
}

bool dbc_t::is_race_ability( uint32_t /* spell_id */ ) const
{
  return false;
}

bool dbc_t::is_specialization_ability( uint32_t spell_id ) const
{
  for ( unsigned cls = 0; cls < 12; cls++ )
  {
    for ( unsigned tree = 0; tree < MAX_TALENT_TABS; tree++ )
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

bool dbc_t::is_mastery_ability( uint32_t spell_id ) const
{
  for ( unsigned int cls = 0; cls < 12; cls++ )
  {
    for ( unsigned n = 0; n < mastery_ability_size(); n++ )
    {
      if ( mastery_ability( cls, n ) == spell_id )
        return true;
    }
  }

  return false;
}

bool dbc_t::is_glyph_spell( uint32_t spell_id ) const
{
  for ( unsigned cls = 0; cls < 12; cls++ )
  {
    for ( unsigned type = 0; type < GLYPH_MAX; type++ )
    {
      for ( unsigned n = 0; n < glyph_spell_size(); n++ )
      {
        if ( glyph_spell( cls, type, n ) == spell_id )
          return true;
      }
    }
  }

  return false;
}

bool dbc_t::is_set_bonus_spell( uint32_t spell_id ) const
{
  for ( unsigned cls = 0; cls < 12; cls++ )
  {
    for ( unsigned tier = 11; tier < 12; tier++ )
    {
      for ( unsigned n = 0; n < set_bonus_spell_size(); n++ )
      {
        if ( set_bonus_spell( cls, tier - 11, n ) == spell_id )
          return true;
      }
    }
  }

  return false;
}

double dbc_t::weapon_dps( unsigned item_id ) const
{
  const item_data_t* item_data = item( item_id );

  if ( ! item_data ) return 0.0;

  if ( item_data -> quality > 5 ) return 0.0;

  switch ( item_data -> inventory_type )
  {
  case INVTYPE_WEAPON:
  case INVTYPE_WEAPONMAINHAND:
  case INVTYPE_WEAPONOFFHAND:
  {
    if ( item_data -> flags_2 & ITEM_FLAG2_CASTER_WEAPON )
      return item_damage_caster_1h( item_data -> level ).values[ item_data -> quality ];
    else
      return item_damage_1h( item_data -> level ).values[ item_data -> quality ];
    break;
  }
  case INVTYPE_2HWEAPON:
  {
    if ( item_data -> flags_2 & ITEM_FLAG2_CASTER_WEAPON )
      return item_damage_caster_2h( item_data -> level ).values[ item_data -> quality ];
    else
      return item_damage_2h( item_data -> level ).values[ item_data -> quality ];
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
      return item_damage_ranged( item_data -> level ).values[ item_data -> quality ];
      break;
    }
    case ITEM_SUBCLASS_WEAPON_THROWN:
    {
      return item_damage_thrown( item_data -> level ).values[ item_data -> quality ];
      break;
    }
    case ITEM_SUBCLASS_WEAPON_WAND:
    {
      return item_damage_wand( item_data -> level ).values[ item_data -> quality ];
      break;
    }
    default: break;
    }
    break;
  }
  default: break;
  }

  return 0;
}

/* Static helper methods */

double dbc_t::fmt_value( double v, effect_type_t type, effect_subtype_t sub_type )
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
